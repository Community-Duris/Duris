#include "player_snapshot_repository.h"

#include "persistence_observability.h"
#include "sql_pool.h"

#include <mysql/mysql.h>

#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace
{
struct query_result
{
	bool ok;
	unsigned int error_code;
};

query_result execute(MYSQL *connection, const std::string &sql)
{
	const uint64_t started = persistence_observability_now_usec();
	const int rc = mysql_real_query(connection, sql.data(), sql.size());
	const uint64_t finished = persistence_observability_now_usec();
	const unsigned int error_code = rc ? mysql_errno(connection) : 0;
	persistence_query_record(PERSISTENCE_QUERY_SITE,
				 PERSISTENCE_QUERY_CONTEXT_PLAYER_SAVE_WORKER,
				 persistence_statement_kind_from_sql(sql.c_str()),
				 finished - started, rc == 0, error_code,
				 rc ? mysql_sqlstate(connection) : "00000");
	return { rc == 0, error_code };
}

bool retryable_error(unsigned int error_code)
{
	return error_code == 1040 || error_code == 1205 || error_code == 1213 ||
	       error_code == 2002 || error_code == 2003 || error_code == 2006 || error_code == 2013;
}

bool connection_error(unsigned int error_code)
{
	return error_code == 2002 || error_code == 2003 || error_code == 2006 || error_code == 2013;
}

player_save_apply_result failure(unsigned int error_code)
{
	return { retryable_error(error_code) ? player_save_apply_outcome::retryable_failure :
					       player_save_apply_outcome::terminal_failure,
		 0, error_code };
}

std::string escape(MYSQL *connection, const std::string &value)
{
	std::string escaped(value.size() * 2 + 1, '\0');
	const unsigned long size =
		mysql_real_escape_string(connection, escaped.data(), value.data(), value.size());
	escaped.resize(size);
	return escaped;
}

std::string quote(MYSQL *connection, const std::string &value)
{
	return "'" + escape(connection, value) + "'";
}

uint64_t integer_value(const player_snapshot_integer &row)
{
	return row.is_unsigned ? row.unsigned_value : static_cast<uint64_t>(row.signed_value);
}

const char *status_column(player_status_field field)
{
	static constexpr std::array<const char *, 61> columns = {
		"m_class",
		"secondary_class",
		"spec",
		"race",
		"racewar",
		"level",
		"sex",
		"weight",
		"height",
		"size",
		"hometown",
		"birthplace",
		"orig_birthplace",
		"birth_time",
		"played_time",
		"base_str",
		"base_dex",
		"base_agi",
		"base_con",
		"base_pow",
		"base_int",
		"base_wis",
		"base_cha",
		"base_kar",
		"base_luk",
		"mana",
		"base_mana",
		"hit_diff",
		"base_hit",
		"vitality",
		"base_vitality",
		"spells_memmed_extra",
		"copper",
		"silver",
		"gold",
		"platinum",
		"exp",
		"epics",
		"epic_skill_points",
		"skillpoints",
		"spell_bind_used",
		"act",
		"act2",
		"act3",
		"vote",
		"alignment",
		"prestige",
		"assoc_id",
		"guild_status",
		"time_left_guild",
		"nb_left_guild",
		"time_unspecced",
		"frags",
		"oldfrags",
		"numb_deaths",
		"echo_toggle",
		"prompt",
		"wiz_invis",
		"wimpy",
		"aggressive",
		"highest_level",
	};
	static constexpr std::array<const char *, 2> tail = { "screen_length", "last_ip" };
	const size_t index = static_cast<size_t>(field);
	if (index < columns.size())
		return columns[index];
	return index - columns.size() < tail.size() ? tail[index - columns.size()] : nullptr;
}

const char *status_string_column(player_status_string_field field)
{
	static constexpr std::array<const char *, 7> columns = {
		"name", "short_descr", "long_descr", "description", "title", "poof_in", "poof_out",
	};
	const size_t index = static_cast<size_t>(field);
	return index < columns.size() ? columns[index] : nullptr;
}

bool status_time_field(player_status_field field)
{
	return field == player_status_field::birth_time ||
	       field == player_status_field::time_left_guild ||
	       field == player_status_field::time_unspecialized;
}

query_result apply_status(MYSQL *connection, const player_snapshot &snapshot)
{
	std::ostringstream sql;
	sql << "UPDATE player_data SET last_room=" << snapshot.room_vnum << ",last_save=NOW()";
	for (const player_snapshot_integer &row : snapshot.status_integers)
	{
		if (row.field == player_status_field::epics ||
		    row.field == player_status_field::copper ||
		    row.field == player_status_field::silver ||
		    row.field == player_status_field::gold ||
		    row.field == player_status_field::platinum)
			continue;
		const char *column = status_column(row.field);
		if (!column)
			return { false, EINVAL };
		sql << ',' << column << '=';
		if (status_time_field(row.field))
			sql << "FROM_UNIXTIME(NULLIF(" << integer_value(row) << ",0))";
		else if (row.is_unsigned)
			sql << row.unsigned_value;
		else
			sql << row.signed_value;
	}
	for (const player_snapshot_string &row : snapshot.status_strings)
	{
		const char *column = status_string_column(row.field);
		if (!column || row.field == player_status_string_field::name)
			continue;
		sql << ',' << column << '=' << quote(connection, row.value);
	}
	for (size_t index = 0; index < snapshot.conditions.size(); ++index)
		sql << ",condition_" << index << '=' << snapshot.conditions[index];
	static constexpr std::array<const char *, 14> quest_columns = {
		"quest_active",	  "quest_mob_vnum",    "quest_type",	      "quest_accomplished",
		"quest_started",  "quest_zone_number", "quest_giver",	      "quest_level",
		"quest_receiver", "quest_shares_left", "quest_kill_how_many", "quest_kill_original",
		"quest_map_room", "quest_map_bought",
	};
	for (size_t index = 0; index < snapshot.quest_values.size(); ++index)
		sql << ',' << quest_columns[index] << '=' << snapshot.quest_values[index];
	sql << " WHERE pid=" << snapshot.pid;
	return execute(connection, sql.str());
}

template <typename Row, typename Append>
query_result replace_rows(MYSQL *connection, int pid, const char *table, const char *columns,
			  const std::vector<Row> &rows, Append append)
{
	query_result result = execute(connection, "DELETE FROM " + std::string(table) +
							  " WHERE pid=" + std::to_string(pid));
	if (!result.ok || rows.empty())
		return result;
	std::ostringstream sql;
	sql << "INSERT INTO " << table << " (pid," << columns << ") VALUES ";
	for (size_t index = 0; index < rows.size(); ++index)
	{
		if (index)
			sql << ',';
		sql << '(' << pid << ',';
		append(sql, rows[index]);
		sql << ')';
	}
	return execute(connection, sql.str());
}

query_result apply_replacement_rows(MYSQL *connection, const player_snapshot &snapshot)
{
	query_result result = { true, 0 };
	if (snapshot.components & PLAYER_COMPONENT_LANGUAGES)
		result = replace_rows(connection, snapshot.pid, "player_languages",
				      "tongue_id,proficiency", snapshot.languages,
				      [](auto &sql, const auto &row)
				      { sql << row.index << ',' << row.value; });
	if (result.ok && (snapshot.components & PLAYER_COMPONENT_INTRODUCTIONS))
		result = replace_rows(connection, snapshot.pid, "player_intros",
				      "intro_index,intro_pid,intro_time", snapshot.introductions,
				      [](auto &sql, const auto &row) {
					      sql << row.index << ',' << row.value
						  << ",FROM_UNIXTIME(NULLIF(" << row.auxiliary
						  << ",0))";
				      });
	if (result.ok && (snapshot.components & PLAYER_COMPONENT_TIMERS))
		result = replace_rows(
			connection, snapshot.pid, "player_timers", "timer_id,timer_value",
			snapshot.timers, [](auto &sql, const auto &row)
			{ sql << row.index << ",FROM_UNIXTIME(NULLIF(" << row.value << ",0))"; });
	if (result.ok && (snapshot.components & PLAYER_COMPONENT_UNDEAD_SLOTS))
		result = replace_rows(connection, snapshot.pid, "player_undead_slots",
				      "circle,slots", snapshot.undead_slots,
				      [](auto &sql, const auto &row)
				      { sql << row.index << ',' << row.value; });
	if (result.ok && (snapshot.components & PLAYER_COMPONENT_FORGED_ITEMS))
		result = replace_rows(connection, snapshot.pid, "player_forged_items",
				      "forge_index,item_vnum", snapshot.forged_items,
				      [](auto &sql, const auto &row)
				      { sql << row.index << ',' << row.value; });
	if (result.ok && (snapshot.components & PLAYER_COMPONENT_GRANTED_COMMANDS))
	{
		result = execute(connection, "DELETE FROM player_granted_cmds WHERE pid=" +
						     std::to_string(snapshot.pid));
		if (result.ok && !snapshot.granted_commands.empty())
		{
			std::ostringstream sql;
			sql << "INSERT INTO player_granted_cmds (pid,cmd_num) VALUES ";
			for (size_t index = 0; index < snapshot.granted_commands.size(); ++index)
				sql << (index ? "," : "") << '(' << snapshot.pid << ','
				    << snapshot.granted_commands[index] << ')';
			result = execute(connection, sql.str());
		}
	}
	return result;
}

query_result apply_skills(MYSQL *connection, const player_snapshot &snapshot)
{
	return replace_rows(connection, snapshot.pid, "player_skills", "skill_id,learned,taught",
			    snapshot.skills,
			    [](auto &sql, const auto &row)
			    {
				    sql << row.skill_id << ','
					<< static_cast<unsigned int>(row.learned) << ','
					<< static_cast<unsigned int>(row.taught);
			    });
}

query_result apply_affects(MYSQL *connection, const player_snapshot &snapshot)
{
	query_result result = execute(connection, "DELETE FROM player_affects WHERE pid=" +
							  std::to_string(snapshot.pid));
	if (!result.ok || snapshot.affects.empty())
		return result;
	std::ostringstream sql;
	sql << "INSERT INTO player_affects (pid,type,duration,flags,modifier,location,level,"
	       "bitvector1,bitvector2,bitvector3,bitvector4,bitvector5,custom_msg_char,"
	       "custom_msg_room) VALUES ";
	for (size_t index = 0; index < snapshot.affects.size(); ++index)
	{
		const auto &row = snapshot.affects[index];
		sql << (index ? "," : "") << '(' << snapshot.pid << ',' << row.type << ','
		    << row.duration << ',' << row.flags << ',' << row.modifier << ','
		    << static_cast<unsigned int>(row.location) << ',' << row.level;
		for (uint64_t bitvector : row.bitvectors)
			sql << ',' << bitvector;
		sql << ','
		    << (row.wear_off_character.empty() ? "NULL" :
							 quote(connection, row.wear_off_character))
		    << ','
		    << (row.wear_off_room.empty() ? "NULL" : quote(connection, row.wear_off_room))
		    << ')';
	}
	return execute(connection, sql.str());
}

std::string optional_item_string(MYSQL *connection, const player_item_snapshot &row, uint8_t mask,
				 const std::string &value)
{
	return row.string_mask & mask ? quote(connection, value) : "NULL";
}

query_result insert_item_rows(MYSQL *connection, const std::vector<player_item_snapshot> &items,
			      int owner_id, bool pet_items)
{
	std::vector<unsigned long long> ids;
	ids.reserve(items.size());
	for (size_t index = 0; index < items.size(); ++index)
	{
		const player_item_snapshot &row = items[index];
		if (row.parent_index >= static_cast<int32_t>(index) || row.parent_index < -1)
			return { false, EINVAL };
		const std::string container =
			row.parent_index < 0 ? "NULL" : std::to_string(ids[row.parent_index]);
		std::ostringstream sql;
		if (pet_items)
			sql << "INSERT INTO player_pet_items (pet_id,vnum,equip_slot,container_id,";
		else
			sql << "INSERT INTO player_items (pid,vnum,equip_slot,container_id,quantity,";
		sql << "weight,cost,timer,extra_flags,wear_flags,item_type,value0,value1,value2,"
		       "value3,value4,value5,value6,value7,name,short_descr,description,action_descr,"
		       "bitvector1,bitvector2,bitvector3,bitvector4,bitvector5,item_material,obj_uid,"
		       "item_condition) VALUES ("
		    << owner_id << ',' << row.vnum << ',' << row.equipment_slot << ',' << container;
		if (!pet_items)
			sql << ",1";
		sql << ',' << row.weight << ',' << row.cost << ',' << row.timers[0] << ','
		    << row.extra_flags << ',' << row.wear_flags << ','
		    << static_cast<int>(row.type);
		for (int32_t value : row.values)
			sql << ',' << value;
		sql << ',' << optional_item_string(connection, row, 1, row.name) << ','
		    << optional_item_string(connection, row, 4, row.short_description) << ','
		    << optional_item_string(connection, row, 2, row.description) << ','
		    << optional_item_string(connection, row, 8, row.action_description);
		for (uint64_t bitvector : row.bitvectors)
			sql << ',' << bitvector;
		sql << ',' << static_cast<int>(row.material) << ',' << row.object_uid << ','
		    << row.condition << ')';
		query_result result = execute(connection, sql.str());
		if (!result.ok)
			return result;
		const unsigned long long item_id = mysql_insert_id(connection);
		if (!item_id)
			return { false, EIO };
		ids.push_back(item_id);

		std::unordered_set<uint64_t> affect_keys;
		for (const auto &affect : row.affects)
		{
			if (!affect[0] && !affect[1])
				continue;
			const uint64_t key =
				(static_cast<uint64_t>(static_cast<uint16_t>(affect[0])) << 32) |
				static_cast<uint32_t>(affect[1]);
			if (!affect_keys.insert(key).second)
				continue;
			result = execute(connection,
					 "INSERT INTO " +
						 std::string(pet_items ? "player_pet_item_affects" :
									 "player_item_affects") +
						 " (item_id,location,modifier) VALUES (" +
						 std::to_string(item_id) + "," +
						 std::to_string(affect[0]) + "," +
						 std::to_string(affect[1]) + ")");
			if (!result.ok)
				return result;
		}
		for (const auto &description : row.extra_descriptions)
		{
			if (description.keyword.empty())
				continue;
			std::string encoded_description = description.description;
			if (description.spellbook)
			{
				std::ostringstream encoded;
				encoded << '[';
				for (size_t spell_index = 0;
				     spell_index < description.spell_ids.size(); ++spell_index)
					encoded << (spell_index ? "," : "")
						<< description.spell_ids[spell_index];
				encoded << ']';
				encoded_description = encoded.str();
			}
			result = execute(connection,
					 "INSERT INTO " +
						 std::string(pet_items ?
								     "player_pet_item_extra_descr" :
								     "player_item_extra_descr") +
						 " (item_id,keyword,description) VALUES (" +
						 std::to_string(item_id) + "," +
						 quote(connection, description.keyword) + "," +
						 quote(connection, encoded_description) + ")");
			if (!result.ok)
				return result;
		}
	}
	return { true, 0 };
}

query_result apply_items(MYSQL *connection, const player_snapshot &snapshot)
{
	const bool equipment = snapshot.components & PLAYER_COMPONENT_EQUIPMENT;
	const bool inventory = snapshot.components & PLAYER_COMPONENT_INVENTORY;
	std::string deletion = "DELETE FROM player_items WHERE pid=" + std::to_string(snapshot.pid);
	if (equipment != inventory)
		deletion += equipment ? " AND equip_slot>0" : " AND equip_slot=0";
	query_result result = execute(connection, deletion);
	return result.ok ? insert_item_rows(connection, snapshot.items, snapshot.pid, false) :
			   result;
}

query_result apply_pets(MYSQL *connection, const player_snapshot &snapshot)
{
	query_result result = execute(connection, "DELETE FROM player_pets WHERE owner_pid=" +
							  std::to_string(snapshot.pid));
	if (!result.ok)
		return result;
	for (const player_pet_snapshot &pet : snapshot.pets)
	{
		std::ostringstream sql;
		sql << "INSERT INTO player_pets (owner_pid,mob_vnum,pet_order,hit,max_hit,mana,"
		       "max_mana,vitality,max_vitality,charm_duration,room_vnum,saved_at) VALUES ("
		    << snapshot.pid << ',' << pet.mob_vnum << ',' << pet.order << ',' << pet.hit
		    << ',' << pet.max_hit << ',' << pet.mana << ',' << pet.max_mana << ','
		    << pet.vitality << ',' << pet.max_vitality << ',' << pet.charm_duration << ','
		    << pet.room_vnum << ",NOW())";
		result = execute(connection, sql.str());
		if (!result.ok)
			return result;
		const unsigned long long pet_id = mysql_insert_id(connection);
		if (!pet_id ||
		    pet_id > static_cast<unsigned long long>(std::numeric_limits<int>::max()))
			return { false, EIO };
		result = insert_item_rows(connection, pet.items, static_cast<int>(pet_id), true);
		if (!result.ok)
			return result;
	}
	return result;
}

query_result apply_shapes(MYSQL *connection, const player_snapshot &snapshot)
{
	return replace_rows(connection, snapshot.pid, "player_shapechanges",
			    "mob_vnum,times_researched,last_researched,last_shapechanged",
			    snapshot.shapes,
			    [](auto &sql, const auto &row)
			    {
				    sql << row.mob_vnum << ',' << row.times_researched
					<< ",FROM_UNIXTIME(NULLIF(" << row.last_researched
					<< ",0)),FROM_UNIXTIME(NULLIF(" << row.last_shapechanged
					<< ",0))";
			    });
}

query_result apply_trophies(MYSQL *connection, const player_snapshot &snapshot)
{
	query_result result = execute(connection, "DELETE FROM zone_trophy WHERE pid=" +
							  std::to_string(snapshot.pid));
	if (!result.ok || snapshot.trophies.empty())
		return result;
	std::ostringstream sql;
	sql << "INSERT INTO zone_trophy (pid,zone_number,exp) VALUES ";
	for (size_t index = 0; index < snapshot.trophies.size(); ++index)
		sql << (index ? "," : "") << '(' << snapshot.pid << ','
		    << snapshot.trophies[index].zone_number << ','
		    << snapshot.trophies[index].experience << ')';
	return execute(connection, sql.str());
}

query_result apply_components(MYSQL *connection, const player_snapshot &snapshot)
{
	query_result result = { true, 0 };
	if (snapshot.components & PLAYER_COMPONENT_STATUS)
		result = apply_status(connection, snapshot);
	if (result.ok)
		result = apply_replacement_rows(connection, snapshot);
	if (result.ok && (snapshot.components & PLAYER_COMPONENT_SKILLS))
		result = apply_skills(connection, snapshot);
	if (result.ok && (snapshot.components & PLAYER_COMPONENT_AFFECTS))
		result = apply_affects(connection, snapshot);
	if (result.ok &&
	    (snapshot.components & (PLAYER_COMPONENT_EQUIPMENT | PLAYER_COMPONENT_INVENTORY)))
		result = apply_items(connection, snapshot);
	if (result.ok && (snapshot.components & PLAYER_COMPONENT_PETS))
		result = apply_pets(connection, snapshot);
	if (result.ok && (snapshot.components & PLAYER_COMPONENT_SHAPECHANGES))
		result = apply_shapes(connection, snapshot);
	if (result.ok && (snapshot.components & PLAYER_COMPONENT_TROPHIES))
		result = apply_trophies(connection, snapshot);
	return result;
}

player_save_apply_result read_durable_revision(MYSQL *connection, int pid)
{
	const query_result query =
		execute(connection,
			"SELECT save_revision FROM player_data WHERE pid=" + std::to_string(pid));
	if (!query.ok)
		return failure(query.error_code);
	MYSQL_RES *result = mysql_store_result(connection);
	if (!result)
		return failure(mysql_errno(connection));
	MYSQL_ROW row = mysql_fetch_row(result);
	if (!row || !row[0])
	{
		mysql_free_result(result);
		return { player_save_apply_outcome::terminal_failure, 0, ENOENT };
	}
	char *end = nullptr;
	errno = 0;
	const unsigned long long revision = std::strtoull(row[0], &end, 10);
	const bool valid = !errno && end && !*end;
	mysql_free_result(result);
	if (!valid)
		return { player_save_apply_outcome::terminal_failure, 0, EINVAL };
	return { player_save_apply_outcome::already_applied, revision, 0 };
}
} // namespace

player_save_apply_result player_snapshot_repository_apply(MYSQL *connection,
							  const player_snapshot &snapshot)
{
	if (!connection || snapshot.pid <= 0 || !snapshot.revision || !snapshot.components ||
	    (snapshot.components & ~PLAYER_CHECKPOINT_COMPONENT_ALL))
		return { player_save_apply_outcome::terminal_failure, 0, EINVAL };

	query_result query = execute(connection, "START TRANSACTION");
	if (!query.ok)
		return failure(query.error_code);
	query = execute(connection, "SELECT save_revision FROM player_data WHERE pid=" +
					    std::to_string(snapshot.pid) + " FOR UPDATE");
	if (!query.ok)
	{
		execute(connection, "ROLLBACK");
		return failure(query.error_code);
	}
	MYSQL_RES *result = mysql_store_result(connection);
	if (!result)
	{
		const unsigned int error_code = mysql_errno(connection);
		execute(connection, "ROLLBACK");
		return failure(error_code);
	}
	MYSQL_ROW row = mysql_fetch_row(result);
	if (!row || !row[0])
	{
		mysql_free_result(result);
		execute(connection, "ROLLBACK");
		return { player_save_apply_outcome::terminal_failure, 0, ENOENT };
	}
	char *end = nullptr;
	errno = 0;
	const unsigned long long durable = std::strtoull(row[0], &end, 10);
	const bool valid_revision = !errno && end && !*end;
	mysql_free_result(result);
	if (!valid_revision)
	{
		execute(connection, "ROLLBACK");
		return { player_save_apply_outcome::terminal_failure, 0, EINVAL };
	}
	if (durable >= snapshot.revision)
	{
		execute(connection, "ROLLBACK");
		return { durable == snapshot.revision ? player_save_apply_outcome::already_applied :
							player_save_apply_outcome::stale_revision,
			 durable, 0 };
	}

	query = apply_components(connection, snapshot);
	if (!query.ok)
	{
		execute(connection, "ROLLBACK");
		player_save_apply_result failed = failure(query.error_code);
		failed.durable_revision = durable;
		return failed;
	}
	query = execute(connection, "UPDATE player_data SET save_revision=" +
					    std::to_string(snapshot.revision) +
					    " WHERE pid=" + std::to_string(snapshot.pid) +
					    " AND save_revision=" + std::to_string(durable));
	if (!query.ok || mysql_affected_rows(connection) != 1)
	{
		const unsigned int error_code = query.ok ? EAGAIN : query.error_code;
		execute(connection, "ROLLBACK");
		return { player_save_apply_outcome::retryable_failure, durable, error_code };
	}
	query = execute(connection, "COMMIT");
	if (!query.ok)
	{
		if (!connection_error(query.error_code))
			execute(connection, "ROLLBACK");
		return { connection_error(query.error_code) ?
				 player_save_apply_outcome::ambiguous_commit :
				 failure(query.error_code).outcome,
			 durable, query.error_code };
	}
	return { player_save_apply_outcome::applied, snapshot.revision, 0 };
}

player_save_apply_result player_snapshot_repository_apply_from_pool(const player_snapshot &snapshot,
								    void *context)
{
	(void)context;
	MYSQL *connection = sql_pool_acquire();
	if (!connection)
		return { player_save_apply_outcome::retryable_failure, 0, ETIMEDOUT };
	player_save_apply_result applied = player_snapshot_repository_apply(connection, snapshot);
	if (applied.outcome == player_save_apply_outcome::ambiguous_commit ||
	    connection_error(applied.error_code))
	{
		MYSQL *replacement = sql_pool_replace_connection(connection);
		if (!replacement)
		{
			sql_pool_release(connection);
			return applied;
		}
		connection = replacement;
	}
	if (applied.outcome == player_save_apply_outcome::ambiguous_commit)
	{
		const player_save_apply_result durable =
			read_durable_revision(connection, snapshot.pid);
		if (durable.error_code == 0)
		{
			if (durable.durable_revision == snapshot.revision)
				applied = { player_save_apply_outcome::already_applied,
					    durable.durable_revision, 0 };
			else if (durable.durable_revision > snapshot.revision)
				applied = { player_save_apply_outcome::stale_revision,
					    durable.durable_revision, 0 };
			else
				applied = { player_save_apply_outcome::retryable_failure,
					    durable.durable_revision, applied.error_code };
		}
	}
	sql_pool_release(connection);
	return applied;
}

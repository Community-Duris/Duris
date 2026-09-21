#include "player/player_snapshot_repository.h"

#include "core/defines.h"
#include "persistence/persistence_observability.h"
#include "player/player_snapshot_codec.h"
#include "sql/item_extra_descr_codec.h"
#include "sql/sql_pool.h"

#include <mysql/mysql.h>

#include <array>
#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <sstream>
#include <string>
#include <strings.h>
#include <unordered_set>
#include <unordered_map>
#include <vector>

namespace
{
struct query_result
{
	bool ok;
	unsigned int error_code;
};

query_result
canonicalize_snapshot_extra_description(const player_item_extra_description_snapshot &description,
					std::string *keyword_out, std::string *description_out)
{
	if (!keyword_out || !description_out)
		return { false, EINVAL };
	if (description.spellbook != (description.keyword == "SPELLBOOK"))
		return { false, EINVAL };
	if (description.spellbook && !description.description.empty() &&
	    !description.spell_ids.empty())
		return { false, EINVAL };

	*keyword_out = description.keyword;
	*description_out = description.description;
	if (!description.spellbook)
	{
		if (!description.spell_ids.empty())
			return { false, EINVAL };
		return { true, 0 };
	}

	std::array<char, (MAX_SKILLS + 1) / 8 + 1> spell_bits = {};
	if (!description.description.empty())
	{
		if (sql_decode_stored_spellbook("SPELLBOOK", description.description.c_str(),
						spell_bits.data(), spell_bits.size()) !=
		    sql_spellbook_decode_status::decoded)
			return { false, EINVAL };
	}
	else
	{
		std::array<bool, MAX_SKILLS> seen = {};
		for (int32_t spell : description.spell_ids)
		{
			if (spell < 0 || spell >= MAX_SKILLS || seen[spell])
				return { false, EINVAL };
			seen[spell] = true;
			const size_t byte = static_cast<size_t>(spell) / 8;
			spell_bits[byte] =
				static_cast<char>(static_cast<unsigned char>(spell_bits[byte]) |
						  static_cast<unsigned char>(1U << (spell % 8)));
		}
	}

	const char marker[] = { 3, 1, 3, 0 };
	char *db_keyword = nullptr;
	char *db_description = nullptr;
	if (!sql_encode_item_extra_descr(marker, spell_bits.data(), &db_keyword, &db_description))
		return { false, ENOMEM };
	if (!db_keyword || !db_description)
	{
		std::free(db_keyword);
		std::free(db_description);
		return { false, ENOMEM };
	}
	*keyword_out = db_keyword;
	*description_out = db_description;
	std::free(db_keyword);
	std::free(db_description);
	return { true, 0 };
}

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
	sql << ",output_preferences=" << quote(connection, snapshot.output_preferences);
	for (const player_snapshot_integer &row : snapshot.status_integers)
	{
		if (row.field == player_status_field::epics ||
		    row.field == player_status_field::frags ||
		    row.field == player_status_field::old_frags ||
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
	       "custom_msg_room,ward_source_uid,ward_full_duration,ward_capacity,"
	       "ward_capacity_max,ward_refresh_remaining,ward_source_type,ward_source_worn,"
	       "ward_active) VALUES ";
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
		    << ',' << row.ward_source_uid
		    << ',' << row.ward_full_duration
		    << ',' << row.ward_capacity
		    << ',' << row.ward_capacity_max
		    << ',' << row.ward_refresh_remaining
		    << ',' << static_cast<unsigned int>(row.ward_source_type)
		    << ',' << static_cast<unsigned int>(row.ward_source_worn)
		    << ',' << static_cast<unsigned int>(row.ward_active)
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
		std::unordered_set<std::string> description_keys;
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
			{
				if (description.spellbook || !description.spell_ids.empty())
					return { false, EINVAL };
				continue;
			}
			std::string encoded_keyword;
			std::string encoded_description;
			const query_result canonical = canonicalize_snapshot_extra_description(
				description, &encoded_keyword, &encoded_description);
			if (!canonical.ok)
				return canonical;
			std::string description_key = encoded_keyword;
			description_key.push_back('\0');
			description_key += encoded_description;
			if (!description_keys.insert(std::move(description_key)).second)
				continue;
			result = execute(connection,
					 "INSERT INTO " +
						 std::string(pet_items ?
								     "player_pet_item_extra_descr" :
								     "player_item_extra_descr") +
						 " (item_id,keyword,description) VALUES (" +
						 std::to_string(item_id) + "," +
						 quote(connection, encoded_keyword) + "," +
						 quote(connection, encoded_description) + ")");
			if (!result.ok)
				return result;
		}
	}
	return { true, 0 };
}

query_result sync_restitution_runtime_state(MYSQL *connection,
					    const std::vector<player_item_snapshot> &items,
					    int owner_id)
{
	if (!connection)
		return { false, EINVAL };

	MYSQL_RES *availability = nullptr;
	query_result available = execute(
		connection,
		"SELECT table_name FROM information_schema.tables WHERE table_schema=DATABASE() "
		"AND table_name IN ('player_death_restitution_delivery',"
		"'player_death_restitution_runtime') ORDER BY table_name");
	if (!available.ok)
		return available;
	availability = mysql_store_result(connection);
	if (!availability)
		return { false, mysql_errno(connection) };
	bool delivery_present = false;
	bool runtime_present = false;
	MYSQL_ROW availability_row;
	while ((availability_row = mysql_fetch_row(availability)) != nullptr)
	{
		if (!availability_row[0])
		{
			mysql_free_result(availability);
			return { false, EINVAL };
		}
		if (!std::strcmp(availability_row[0], "player_death_restitution_delivery"))
			delivery_present = true;
		else if (!std::strcmp(availability_row[0], "player_death_restitution_runtime"))
			runtime_present = true;
	}
	mysql_free_result(availability);
	if (!delivery_present)
		return { true, 0 };

	const std::string owner_filter =
		"own.owner_type=1 AND own.owner_id=" + std::to_string(owner_id) +
		" AND own.owner_context_id=0 AND own.state=1";
	std::unordered_set<uint64_t> requested;
	std::ostringstream uid_list;
	try
	{
		requested.reserve(items.size());
		bool first = true;
		for (const player_item_snapshot &item : items)
			if (item.object_uid && requested.insert(item.object_uid).second)
			{
				uid_list << (first ? "" : ",") << item.object_uid;
				first = false;
			}
	}
	catch (const std::bad_alloc &)
	{
		return { false, ENOMEM };
	}

	// Include both sides of the custody/projection comparison. The requested
	// predicate keeps a delivered UID visible when its current-owner row is
	// missing or foreign; the owner predicate finds an active delivery that the
	// snapshot omitted. Either case must roll back instead of being treated as
	// an empty scoped result.
	const std::string requested_filter =
		requested.empty() ? "0" : "d.item_uid IN (" + uid_list.str() + ")";
	const std::string runtime_select =
		runtime_present ? ",HEX(runtime.state_digest),SHA2(runtime.state_payload,256)" : "";
	const std::string runtime_join =
		runtime_present ?
			" LEFT JOIN player_death_restitution_runtime runtime ON runtime.item_uid=d.item_uid" :
			"";
	const std::string scoped_sql =
		"SELECT d.item_uid,ri.vnum,own.item_uid,own.owner_type,own.owner_id,"
		"own.owner_context_id,own.vnum,own.state" +
		runtime_select +
		" FROM player_death_restitution_delivery d "
		"JOIN player_death_restitution_item ri ON ri.restitution_id=d.restitution_id "
		"AND ri.item_uid=d.item_uid LEFT JOIN item_current_owner own ON "
		"own.item_uid=d.item_uid" +
		runtime_join + " WHERE (" + requested_filter + " OR (" + owner_filter +
		")) ORDER BY d.item_uid FOR UPDATE";
	query_result scoped_query = execute(connection, scoped_sql);
	if (!scoped_query.ok)
		return scoped_query;
	MYSQL_RES *scoped = mysql_store_result(connection);
	if (!scoped)
		return { false, mysql_errno(connection) };
	std::unordered_set<uint64_t> restored;
	try
	{
		restored.reserve(requested.size());
	}
	catch (const std::bad_alloc &)
	{
		mysql_free_result(scoped);
		return { false, ENOMEM };
	}
	MYSQL_ROW row;
	size_t scoped_delivery_count = 0;
	const std::string player_owner_type =
		std::to_string(static_cast<unsigned>(item_owner_type::player));
	const std::string expected_owner_id = std::to_string(owner_id);
	while ((row = mysql_fetch_row(scoped)) != nullptr)
	{
		uint64_t item_uid = 0;
		if (!row[0])
		{
			mysql_free_result(scoped);
			return { false, EINVAL };
		}
		char *end = nullptr;
		errno = 0;
		const unsigned long long parsed_uid = std::strtoull(row[0], &end, 10);
		item_uid = static_cast<uint64_t>(parsed_uid);
		if (errno || end == row[0] || *end || !item_uid || !row[1] || !row[2] ||
		    std::strcmp(row[0], row[2]) != 0 || !row[3] || !row[4] || !row[5] || !row[6] ||
		    !row[7] || player_owner_type != row[3] || expected_owner_id != row[4] ||
		    std::strcmp(row[5], "0") != 0 || std::strcmp(row[7], "1") != 0 ||
		    requested.find(item_uid) == requested.end())
		{
			mysql_free_result(scoped);
			return { false, ENOENT };
		}
		++scoped_delivery_count;
		if (!runtime_present)
		{
			mysql_free_result(scoped);
			return { false, ENOENT };
		}
		if (!row[8] || !row[9] || strcasecmp(row[8], row[9]) != 0)
		{
			mysql_free_result(scoped);
			return { false, ENOENT };
		}
		const auto source = std::find_if(items.begin(), items.end(),
						 [item_uid](const player_item_snapshot &candidate)
						 { return candidate.object_uid == item_uid; });
		if (source == items.end() || std::to_string(source->vnum) != row[1] ||
		    std::to_string(source->vnum) != row[6])
		{
			mysql_free_result(scoped);
			return { false, EINVAL };
		}
		try
		{
			restored.insert(item_uid);
		}
		catch (const std::bad_alloc &)
		{
			mysql_free_result(scoped);
			return { false, ENOMEM };
		}
	}
	mysql_free_result(scoped);
	// Every active delivery owned by this player must be represented exactly
	// once in the requested snapshot.
	if (restored.size() != scoped_delivery_count)
		return { false, ENOENT };

	for (const player_item_snapshot &source : items)
	{
		if (!restored.count(source.object_uid))
			continue;
		player_item_snapshot standalone = source;
		standalone.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		standalone.equipment_slot = -1;
		std::vector<player_item_snapshot> one = { standalone };
		std::vector<uint8_t> encoded;
		if (player_item_snapshot_list_encode(one, &encoded) !=
		    player_snapshot_codec_result::ok)
			return { false, EINVAL };
		const std::string payload(encoded.begin(), encoded.end());
		const std::string payload_sql = quote(connection, payload);
		const query_result result = execute(
			connection,
			"UPDATE player_death_restitution_runtime runtime JOIN "
			"player_death_restitution_delivery delivery ON delivery.item_uid=runtime.item_uid "
			"JOIN player_death_restitution_item restitution ON "
			"restitution.restitution_id=delivery.restitution_id AND "
			"restitution.item_uid=delivery.item_uid JOIN item_current_owner own ON "
			"own.item_uid=runtime.item_uid SET runtime.state_payload=" +
				payload_sql + ",runtime.state_digest=UNHEX(SHA2(" + payload_sql +
				",256)) WHERE runtime.item_uid=" +
				std::to_string(source.object_uid) + " AND " + owner_filter +
				" AND own.vnum=" + std::to_string(source.vnum) +
				" AND restitution.vnum=" + std::to_string(source.vnum));
		if (!result.ok)
			return result;
	}
	return { true, 0 };
}

struct expected_player_item_custody
{
	uint64_t root_item_uid;
	uint64_t parent_item_uid;
	int32_t vnum;
};

bool parse_custody_uint64(const char *text, uint64_t *value)
{
	if (!text || !value || !*text)
		return false;
	char *end = nullptr;
	errno = 0;
	const unsigned long long parsed = std::strtoull(text, &end, 10);
	if (errno || end == text || *end)
		return false;
	*value = static_cast<uint64_t>(parsed);
	return true;
}

/**
 * Prove that a complete replacement payload is an exact projection of active player
 * custody before deleting a single existing payload row.  Inline coin custody is the
 * sole exception: coin_payload is itself an authoritative, independently loadable item
 * payload and therefore does not require a player_items row.
 */
query_result verify_player_item_custody(MYSQL *connection, const player_snapshot &snapshot)
{
	std::unordered_map<uint64_t, expected_player_item_custody> expected;
	try
	{
		expected.reserve(snapshot.items.size());
		for (size_t index = 0; index < snapshot.items.size(); ++index)
		{
			const player_item_snapshot &item = snapshot.items[index];
			if (!item.object_uid || item.vnum <= 0 ||
			    item.parent_index >= static_cast<int32_t>(index) ||
			    item.parent_index < PLAYER_SNAPSHOT_NO_PARENT)
				return { false, PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH };

			uint64_t root_item_uid = item.object_uid;
			uint64_t parent_item_uid = 0;
			if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			{
				const player_item_snapshot &parent =
					snapshot.items[item.parent_index];
				const auto parent_custody = expected.find(parent.object_uid);
				if (parent_custody == expected.end())
					return { false,
						 PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH };
				root_item_uid = parent_custody->second.root_item_uid;
				parent_item_uid = parent.object_uid;
			}
			if (!expected.emplace(item.object_uid,
					      expected_player_item_custody{
						      root_item_uid, parent_item_uid, item.vnum })
				     .second)
				return { false, PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH };
		}
	}
	catch (const std::bad_alloc &)
	{
		return { false, ENOMEM };
	}

	const std::string sql =
		"SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),vnum,"
		"coin_payload IS NOT NULL FROM item_current_owner WHERE owner_type=" +
		std::to_string(static_cast<unsigned>(item_owner_type::player)) +
		" AND owner_id=" + std::to_string(snapshot.pid) +
		" AND owner_context_id=0 AND state=" +
		std::to_string(static_cast<unsigned>(item_custody_state::active)) +
		" ORDER BY item_uid FOR UPDATE";
	query_result query = execute(connection, sql);
	if (!query.ok)
		return query;
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
		return { false, mysql_errno(connection) };

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(rows)) != nullptr)
	{
		uint64_t item_uid = 0, root_item_uid = 0, parent_item_uid = 0;
		const bool valid = parse_custody_uint64(row[0], &item_uid) && item_uid &&
				   parse_custody_uint64(row[1], &root_item_uid) && root_item_uid &&
				   parse_custody_uint64(row[2], &parent_item_uid) && row[3] &&
				   row[4];
		if (!valid)
		{
			mysql_free_result(rows);
			return { false, PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH };
		}

		const auto found = expected.find(item_uid);
		const bool inline_coin_payload = std::strcmp(row[4], "0") != 0;
		if (found == expected.end())
		{
			if (inline_coin_payload)
				continue;
			mysql_free_result(rows);
			return { false, PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH };
		}
		const expected_player_item_custody &item = found->second;
		if (item.root_item_uid != root_item_uid ||
		    item.parent_item_uid != parent_item_uid || std::to_string(item.vnum) != row[3])
		{
			mysql_free_result(rows);
			return { false, PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH };
		}
		expected.erase(found);
	}
	mysql_free_result(rows);
	return expected.empty() ? query_result{ true, 0 } :
				  query_result{ false, PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH };
}

query_result apply_items(MYSQL *connection, const player_snapshot &snapshot)
{
	const bool equipment = snapshot.components & PLAYER_COMPONENT_EQUIPMENT;
	const bool inventory = snapshot.components & PLAYER_COMPONENT_INVENTORY;
	/* New snapshots always replace both halves together.  Keep accepting legacy
	 * component-only journal records, but require every complete replacement to
	 * prove its payload/custody equivalence before the destructive projection. */
	if (equipment && inventory)
	{
		const query_result verified = verify_player_item_custody(connection, snapshot);
		if (!verified.ok)
			return verified;
	}
	std::string deletion = "DELETE FROM player_items WHERE pid=" + std::to_string(snapshot.pid);
	if (equipment != inventory)
		deletion += equipment ? " AND equip_slot>0" : " AND equip_slot=0";
	query_result result = execute(connection, deletion);
	if (!result.ok)
		return result;
	result = insert_item_rows(connection, snapshot.items, snapshot.pid, false);
	if (!result.ok)
		return result;
	return sync_restitution_runtime_state(connection, snapshot.items, snapshot.pid);
}

query_result verify_pet_custody(MYSQL *connection, int pid, const player_pet_snapshot &pet)
{
	if (!pet.pet_uid)
		return { true, 0 };
	struct expected_item
	{
		uint64_t root;
		uint64_t parent;
		int32_t vnum;
	};
	std::unordered_map<uint64_t, expected_item> expected;
	for (size_t index = 0; index < pet.items.size(); ++index)
	{
		const auto &item = pet.items[index];
		if (!item.object_uid || item.parent_index >= static_cast<int32_t>(index) ||
		    item.parent_index < -1)
			return { false, EINVAL };
		uint64_t root = item.object_uid;
		uint64_t parent = 0;
		if (item.parent_index >= 0)
		{
			const auto &ancestor = pet.items[item.parent_index];
			const auto found = expected.find(ancestor.object_uid);
			if (found == expected.end())
				return { false, EINVAL };
			root = found->second.root;
			parent = ancestor.object_uid;
		}
		if (!expected.emplace(item.object_uid, expected_item{ root, parent, item.vnum })
			     .second)
			return { false, EINVAL };
	}
	const std::string sql =
		"SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),vnum FROM "
		"item_current_owner WHERE owner_type=" +
		std::to_string(static_cast<unsigned>(item_owner_type::pet)) +
		" AND owner_id=" + std::to_string(pet.pet_uid) +
		" AND owner_context_id=" + std::to_string(pid) +
		" AND state=1 ORDER BY item_uid FOR UPDATE";
	query_result result = execute(connection, sql);
	if (!result.ok)
		return result;
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
		return { false, mysql_errno(connection) };
	while (MYSQL_ROW row = mysql_fetch_row(rows))
	{
		if (!row[0] || !row[1] || !row[2] || !row[3])
		{
			mysql_free_result(rows);
			return { false, EILSEQ };
		}
		const uint64_t uid = std::strtoull(row[0], nullptr, 10);
		const auto found = expected.find(uid);
		if (found == expected.end() || std::to_string(found->second.root) != row[1] ||
		    std::to_string(found->second.parent) != row[2] ||
		    std::to_string(found->second.vnum) != row[3])
		{
			mysql_free_result(rows);
			return { false, ESTALE };
		}
		expected.erase(found);
	}
	mysql_free_result(rows);
	return expected.empty() ? query_result{ true, 0 } : query_result{ false, ESTALE };
}

query_result pet_has_live_custody(MYSQL *connection, int pid, uint64_t pet_uid, bool *has_custody)
{
	if (!has_custody)
		return { false, EINVAL };
	*has_custody = false;
	if (!pet_uid)
		return { true, 0 };
	const std::string sql =
		"SELECT 1 FROM item_current_owner WHERE owner_type=" +
		std::to_string(static_cast<unsigned>(item_owner_type::pet)) +
		" AND owner_id=" + std::to_string(pet_uid) +
		" AND owner_context_id=" + std::to_string(pid) + " AND state IN (" +
		std::to_string(static_cast<unsigned>(item_custody_state::active)) + "," +
		std::to_string(static_cast<unsigned>(item_custody_state::quarantined)) +
		") LIMIT 1";
	query_result result = execute(connection, sql);
	if (!result.ok)
		return result;
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
		return { false, mysql_errno(connection) };
	MYSQL_ROW row = mysql_fetch_row(rows);
	*has_custody = row != nullptr;
	mysql_free_result(rows);
	return { true, 0 };
}

query_result apply_pets(MYSQL *connection, const player_snapshot &snapshot)
{
	query_result result = { true, 0 };
	std::unordered_set<uint64_t> retained;
	std::unordered_set<uint64_t> pet_uids;
	for (const player_pet_snapshot &pet : snapshot.pets)
	{
		if (pet.pet_uid && !pet_uids.insert(pet.pet_uid).second)
			return { false, EINVAL };
		if (pet.hold_reason == pet_hold_reason::custody_pending)
		{
			bool has_custody = false;
			result = pet_has_live_custody(connection, snapshot.pid, pet.pet_uid,
						      &has_custody);
			if (!result.ok)
				return result;
			if (!has_custody)
				continue;
		}
		uint64_t pet_id = 0;
		if (pet.pet_uid)
		{
			result = execute(connection,
					 "SELECT id FROM player_pets WHERE owner_pid=" +
						 std::to_string(snapshot.pid) + " AND pet_uid=" +
						 std::to_string(pet.pet_uid) + " FOR UPDATE");
			if (!result.ok)
				return result;
			MYSQL_RES *rows = mysql_store_result(connection);
			if (!rows)
				return { false, mysql_errno(connection) };
			MYSQL_ROW row = mysql_fetch_row(rows);
			if (row && row[0])
				pet_id = std::strtoull(row[0], nullptr, 10);
			mysql_free_result(rows);
			if (!pet_id)
				return { false, ESTALE };
			result = verify_pet_custody(connection, snapshot.pid, pet);
			if (!result.ok)
				return result;
		}
		std::ostringstream sql;
		if (pet_id)
			sql << "UPDATE player_pets SET mob_vnum=" << pet.mob_vnum
			    << ",pet_order=" << pet.order << ",hit=" << pet.hit
			    << ",max_hit=" << pet.max_hit << ",mana=" << pet.mana
			    << ",max_mana=" << pet.max_mana << ",vitality=" << pet.vitality
			    << ",max_vitality=" << pet.max_vitality
			    << ",charm_duration=" << pet.charm_duration
			    << ",room_vnum=" << pet.room_vnum << ",saved_at=NOW(),restore_state="
			    << quote(connection, pet.restore_state)
			    << ",hold_reason=" << static_cast<uint32_t>(pet.hold_reason)
			    << " WHERE id=" << pet_id;
		else
			sql << "INSERT INTO player_pets (owner_pid,mob_vnum,pet_order,hit,"
			       "max_hit,mana,max_mana,vitality,max_vitality,charm_duration,room_vnum,"
			       "saved_at,restore_state,hold_reason) VALUES ("
			    << snapshot.pid << ',' << pet.mob_vnum << ',' << pet.order << ','
			    << pet.hit << ',' << pet.max_hit << ',' << pet.mana << ','
			    << pet.max_mana << ',' << pet.vitality << ',' << pet.max_vitality << ','
			    << pet.charm_duration << ',' << pet.room_vnum << ",NOW(),"
			    << quote(connection, pet.restore_state) << ','
			    << static_cast<uint32_t>(pet.hold_reason) << ')';
		result = execute(connection, sql.str());
		if (!result.ok)
			return result;
		if (!pet_id)
			pet_id = mysql_insert_id(connection);
		if (!pet_id ||
		    pet_id > static_cast<unsigned long long>(std::numeric_limits<int>::max()))
			return { false, EIO };
		retained.insert(pet_id);
		if (pet.pet_uid)
		{
			result = execute(connection, "DELETE FROM player_pet_items WHERE pet_id=" +
							     std::to_string(pet_id));
			if (!result.ok)
				return result;
		}
		result = insert_item_rows(connection, pet.items, static_cast<int>(pet_id), true);
		if (!result.ok)
			return result;
	}
	std::string missing = "owner_pid=" + std::to_string(snapshot.pid);
	if (!retained.empty())
	{
		missing += " AND id NOT IN (";
		for (uint64_t id : retained)
			missing += std::to_string(id) + ',';
		missing.back() = ')';
	}
	const std::string custody =
		"EXISTS (SELECT 1 FROM item_current_owner own WHERE own.owner_type=" +
		std::to_string(static_cast<unsigned>(item_owner_type::pet)) +
		" AND own.owner_id=player_pets.pet_uid AND own.owner_context_id=" +
		std::to_string(snapshot.pid) + " AND own.state IN (" +
		std::to_string(static_cast<unsigned>(item_custody_state::active)) + "," +
		std::to_string(static_cast<unsigned>(item_custody_state::quarantined)) + "))";
	result = execute(connection, "UPDATE player_pets SET hold_reason=" +
					     std::to_string(static_cast<uint32_t>(
						     pet_hold_reason::custody_pending)) +
					     ",room_vnum=" + std::to_string(snapshot.room_vnum) +
					     " WHERE " + missing + " AND " + custody);
	if (!result.ok)
		return result;
	return execute(connection,
		       "DELETE FROM player_pets WHERE " + missing + " AND NOT " + custody);
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

std::string hex_operation(const critical_operation_id &operation_id)
{
	static const char digits[] = "0123456789abcdef";
	std::string hex;
	hex.reserve(operation_id.bytes.size() * 2);
	for (uint8_t byte : operation_id.bytes)
	{
		hex.push_back(digits[byte >> 4]);
		hex.push_back(digits[byte & 0x0f]);
	}
	return hex;
}

// The refused assets exist nowhere else once the character is released, so the
// disposition and its custody evidence commit inside the same transaction as
// the death itself. Rewriting the same revision is how a retry stays idempotent.
query_result apply_death(MYSQL *connection, const player_snapshot &snapshot)
{
	if (!snapshot.death)
		return { true, 0 };
	const player_death_snapshot &death = *snapshot.death;
	std::vector<uint8_t> payload;
	if (player_snapshot_encode(snapshot, &payload) != player_snapshot_codec_result::ok)
		return { false, EINVAL };
	const std::string pid = std::to_string(snapshot.pid);
	const std::string revision = std::to_string(snapshot.revision);
	std::ostringstream sql;
	sql << "REPLACE INTO player_death_disposition (pid,save_revision,operation_id,"
	       "corpse_item_uid,corpse_room_vnum,wallet_revision,wallet_copper,wallet_silver,"
	       "wallet_gold,wallet_platinum,wallet_pile_uid,payload) VALUES ("
	    << pid << ',' << revision << ",UNHEX('" << hex_operation(death.operation_id) << "'),"
	    << death.corpse.front().object_uid << ',' << death.corpse_room_vnum << ','
	    << death.wallet_revision;
	for (int32_t amount : death.wallet_before)
		sql << ',' << amount;
	sql << ',' << death.wallet_pile_uid << ','
	    << quote(connection, std::string(payload.begin(), payload.end())) << ')';
	query_result result = execute(connection, sql.str());
	if (!result.ok)
		return result;
	result = execute(connection, "DELETE FROM player_death_custody WHERE pid=" + pid +
					     " AND save_revision=" + revision);
	for (const player_death_custody_snapshot &row : death.custody)
	{
		if (!result.ok)
			return result;
		std::ostringstream custody;
		custody << "INSERT INTO player_death_custody (pid,save_revision,item_uid,"
			   "root_item_uid,parent_item_uid,item_revision,vnum,state,owner_type,"
			   "owner_id,owner_context_id,owner_revision) VALUES ("
			<< pid << ',' << revision << ',' << row.item.item_uid << ','
			<< row.item.root_item_uid << ',' << row.item.parent_item_uid << ','
			<< row.item.expected_item_revision << ',' << row.item.vnum << ','
			<< static_cast<unsigned>(row.item.expected_state) << ','
			<< static_cast<unsigned>(row.owner.type) << ',' << row.owner.id << ','
			<< row.owner.context_id << ',' << row.owner_revision << ')';
		result = execute(connection, custody.str());
	}
	if (!result.ok)
		return result;
	// A rejected handoff leaves custody with the player. Preserve those rows
	// for recovery, but prevent a subsequent load from restoring disputed items.
	const std::string owner =
		"owner_type=" + std::to_string(static_cast<unsigned>(item_owner_type::player)) +
		" AND owner_id=" + pid + " AND owner_context_id=0";
	const std::string active =
		std::to_string(static_cast<unsigned>(item_custody_state::active));
	const std::string death_custody =
		" AND EXISTS (SELECT 1 FROM player_death_custody death_row WHERE death_row.pid=" +
		pid + " AND death_row.save_revision=" + revision +
		" AND (death_row.item_uid=current_item.item_uid OR "
		"death_row.root_item_uid=current_item.root_item_uid))";
	result = execute(
		connection,
		"UPDATE item_owner_revision SET revision=revision+1 WHERE " + owner +
			" AND EXISTS (SELECT 1 FROM item_current_owner current_item WHERE " +
			owner + " AND current_item.state=" + active + death_custody + ")");
	if (result.ok)
		result = execute(
			connection,
			"UPDATE item_current_owner AS current_item SET item_revision=item_revision+1,state=" +
				std::to_string(
					static_cast<unsigned>(item_custody_state::quarantined)) +
				" WHERE " + owner + " AND current_item.state=" + active +
				death_custody);
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

bool player_snapshot_repository_write_pets(MYSQL *connection, const player_snapshot &snapshot)
{
	return connection && snapshot.pid > 0 && apply_pets(connection, snapshot).ok;
}

player_save_apply_result player_snapshot_repository_apply(MYSQL *connection,
							  const player_snapshot &snapshot)
{
	if (!connection || snapshot.pid <= 0 || !snapshot.revision || !snapshot.components ||
	    (snapshot.components & ~PLAYER_CHECKPOINT_COMPONENT_ALL))
		return { player_save_apply_outcome::terminal_failure, 0, EINVAL };
	// Death records require their custody disposition to commit with this save.
	// Never acknowledge one through the ordinary component-only writer.
	if (snapshot.death ? snapshot.schema_version != PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION ||
				     snapshot.death->corpse.empty() :
			     snapshot.schema_version != PLAYER_SNAPSHOT_SCHEMA_VERSION)
		return { player_save_apply_outcome::terminal_failure, 0, ENOTSUP };

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
	if (query.ok)
		query = apply_death(connection, snapshot);
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

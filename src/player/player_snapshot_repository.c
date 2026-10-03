#include "player/player_snapshot_repository.h"

#include "core/defines.h"
#include "item/quest_reward_continuation.h"
#include "persistence/persistence_observability.h"
#include "player/player_snapshot_codec.h"
#include "player/player_save_journal.h"
#include "player/player_quarantine_recovery.h"
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
#include <memory>
#include <sstream>
#include <string>
#include <strings.h>
#include <unordered_set>
#include <unordered_map>
#include <source_location>
#include <utility>
#include <vector>

namespace
{
struct query_result
{
	bool ok;
	unsigned int error_code;
	player_save_custody_diagnosis custody_diagnosis = player_save_custody_diagnosis::none;
	persistence_custody_witness custody_witness = {};
};

query_result
custody_payload_mismatch(player_save_custody_diagnosis diagnosis,
			 const persistence_custody_witness &witness = persistence_custody_witness(),
			 const std::source_location &site = std::source_location::current())
{
	query_result result{ false, PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH, diagnosis,
			     witness };
	result.custody_witness.source_line = site.line();
	return result;
}

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

player_save_apply_result
failure(unsigned int error_code,
	player_save_custody_diagnosis custody_diagnosis = player_save_custody_diagnosis::none)
{
	return { retryable_error(error_code) ? player_save_apply_outcome::retryable_failure :
					       player_save_apply_outcome::terminal_failure,
		 0, error_code, custody_diagnosis };
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
		std::string item_properties;
		if (!pet_items)
		{
			const player_snapshot_codec_result encoded = player_item_properties_encode(
				row.extra2_flags, row.dynamic_affects, &item_properties);
			if (encoded != player_snapshot_codec_result::ok)
				return { false, static_cast<unsigned int>(
							encoded == player_snapshot_codec_result::
										allocation_failure ?
								ENOMEM :
								EINVAL) };
		}
		std::ostringstream sql;
		if (pet_items)
			sql << "INSERT INTO player_pet_items (pet_id,vnum,equip_slot,container_id,";
		else
			sql << "INSERT INTO player_items (pid,vnum,equip_slot,container_id,quantity,";
		sql << "weight,cost,timer,extra_flags,wear_flags,item_type,value0,value1,value2,"
		       "value3,value4,value5,value6,value7,name,short_descr,description,action_descr,"
		       "bitvector1,bitvector2,bitvector3,bitvector4,bitvector5,item_material,obj_uid,"
		       "item_condition";
		if (!pet_items)
			sql << ",item_properties";
		sql << ") VALUES (" << owner_id << ',' << row.vnum << ',' << row.equipment_slot
		    << ',' << container;
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
		    << row.condition;
		if (!pet_items)
			sql << ',' << quote(connection, item_properties);
		sql << ')';
		query_result result = execute(connection, sql.str());
		if (!result.ok)
			return result;
		const unsigned long long item_id = mysql_insert_id(connection);
		if (!item_id)
			return { false, EIO };
		ids.push_back(item_id);
		if (!pet_items)
		{
			player_item_snapshot standalone = row;
			standalone.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
			std::vector<uint8_t> runtime;
			if (player_item_snapshot_list_encode({ standalone }, &runtime) !=
			    player_snapshot_codec_result::ok)
				return { false, EINVAL };
			result = execute(
				connection,
				"INSERT INTO player_item_runtime_state(item_id,payload) VALUES(" +
					std::to_string(item_id) + "," +
					quote(connection,
					      std::string(reinterpret_cast<const char *>(
								  runtime.data()),
							  runtime.size())) +
					")");
			if (!result.ok)
				return result;
		}

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
	size_t snapshot_index;
	uint16_t equipment_slot;
	persistence_custody_witness witness;
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
 * Match a complete payload to active player custody by UID and vnum. If only
 * root/parent/equipment position has drifted, rebuild the payload position from
 * the locked authoritative rows; never create/drop an item or rewrite custody.
 * Inline coin custody is independently loadable and needs no player_items row.
 */
query_result reconcile_player_item_custody(MYSQL *connection, const player_snapshot &snapshot,
					   std::vector<player_item_snapshot> *reconciled_items,
					   bool *topology_reconciled)
{
	if (!reconciled_items || !topology_reconciled)
		return { false, EINVAL };
	reconciled_items->clear();
	*topology_reconciled = false;
	std::unordered_map<uint64_t, expected_player_item_custody> expected;
	std::unordered_set<uint64_t> matched;
	bool topology_mismatch = false;
	try
	{
		expected.reserve(snapshot.items.size());
		matched.reserve(snapshot.items.size());
		for (size_t index = 0; index < snapshot.items.size(); ++index)
		{
			const player_item_snapshot &item = snapshot.items[index];
			persistence_custody_witness witness;
			witness.item_uid = item.object_uid;
			witness.expected_vnum = item.vnum;
			witness.expected_present = true;
			if (item.equipment_slot >= 0)
				witness.expected_slot = item.equipment_slot;
			if (!item.object_uid || item.vnum <= 0 ||
			    item.parent_index >= static_cast<int32_t>(index) ||
			    item.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
			    item.equipment_slot < 0 || item.equipment_slot > MAX_WEAR ||
			    (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT && item.equipment_slot))
				return custody_payload_mismatch(
					player_save_custody_diagnosis::invalid_snapshot_item,
					witness);

			uint64_t root_item_uid = item.object_uid;
			uint64_t parent_item_uid = 0;
			if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			{
				const player_item_snapshot &parent =
					snapshot.items[item.parent_index];
				const auto parent_custody = expected.find(parent.object_uid);
				if (parent_custody == expected.end())
					return custody_payload_mismatch(
						player_save_custody_diagnosis::invalid_snapshot_parent,
						witness);
				root_item_uid = parent_custody->second.root_item_uid;
				parent_item_uid = parent.object_uid;
			}
			witness.expected_root = root_item_uid;
			witness.expected_parent = parent_item_uid;
			if (!expected.emplace(item.object_uid,
					      expected_player_item_custody{
						      root_item_uid, parent_item_uid, item.vnum,
						      index,
						      static_cast<uint16_t>(item.equipment_slot),
						      witness })
				     .second)
				return custody_payload_mismatch(
					player_save_custody_diagnosis::duplicate_snapshot_uid,
					witness);
		}
	}
	catch (const std::bad_alloc &)
	{
		return { false, ENOMEM };
	}

	const std::string sql = "SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),vnum,"
				"coin_payload IS NOT NULL,equipment_slot,"
				"EXISTS(SELECT 1 FROM economic_accounting_item_reference reference "
				"WHERE reference.item_uid=own.item_uid AND "
				"reference.after_revision=own.item_revision),own.item_revision "
				"FROM item_current_owner own WHERE owner_type=" +
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
	std::array<bool, MAX_WEAR + 1> occupied_slots = {};
	try
	{
		while ((row = mysql_fetch_row(rows)) != nullptr)
		{
			uint64_t item_uid = 0, root_item_uid = 0, parent_item_uid = 0;
			uint64_t equipment_slot = 0, slot_evidence = 0;
			const bool valid =
				parse_custody_uint64(row[0], &item_uid) && item_uid &&
				parse_custody_uint64(row[1], &root_item_uid) && root_item_uid &&
				parse_custody_uint64(row[2], &parent_item_uid) && row[3] &&
				row[4] && parse_custody_uint64(row[5], &equipment_slot) &&
				parse_custody_uint64(row[6], &slot_evidence) &&
				slot_evidence <= 1 && equipment_slot <= MAX_WEAR &&
				(!parent_item_uid || !equipment_slot);
			persistence_custody_witness witness;
			witness.item_uid = item_uid;
			witness.observed_present = true;
			witness.observed_root = root_item_uid;
			witness.observed_parent = parent_item_uid;
			witness.observed_slot = static_cast<uint16_t>(equipment_slot);
			uint64_t observed_vnum = 0;
			if (parse_custody_uint64(row[3], &observed_vnum) &&
			    observed_vnum <= INT32_MAX)
				witness.observed_vnum = static_cast<int32_t>(observed_vnum);
			parse_custody_uint64(row[7], &witness.observed_item_revision);
			if (!valid)
			{
				mysql_free_result(rows);
				return custody_payload_mismatch(
					player_save_custody_diagnosis::malformed_active_custody_row,
					witness);
			}
			if (equipment_slot && occupied_slots[equipment_slot])
			{
				mysql_free_result(rows);
				return custody_payload_mismatch(
					player_save_custody_diagnosis::duplicate_equipment_slot,
					witness);
			}
			if (equipment_slot)
				occupied_slots[equipment_slot] = true;

			auto found = expected.find(item_uid);
			const bool inline_coin_payload = std::strcmp(row[4], "0") != 0;
			if (found == expected.end())
			{
				if (inline_coin_payload)
					continue;
				mysql_free_result(rows);
				return custody_payload_mismatch(
					player_save_custody_diagnosis::
						active_custody_absent_from_snapshot,
					witness);
			}
			expected_player_item_custody &item = found->second;
			witness.expected_present = true;
			witness.expected_root = item.root_item_uid;
			witness.expected_parent = item.parent_item_uid;
			witness.expected_vnum = item.vnum;
			witness.expected_slot = item.equipment_slot;
			item.witness = witness;
			if (std::to_string(item.vnum) != row[3])
			{
				mysql_free_result(rows);
				return custody_payload_mismatch(
					player_save_custody_diagnosis::custody_vnum_mismatch,
					witness);
			}
			if (!matched.insert(item_uid).second)
			{
				mysql_free_result(rows);
				return custody_payload_mismatch(
					player_save_custody_diagnosis::duplicate_custody_match,
					witness);
			}
			if (item.root_item_uid != root_item_uid ||
			    item.parent_item_uid != parent_item_uid)
				topology_mismatch = true;
			item.root_item_uid = root_item_uid;
			item.parent_item_uid = parent_item_uid;
			// Legacy custody can predate slot accounting. A retained item
			// reference or a nonzero opening slot makes this position authoritative.
			if (slot_evidence || equipment_slot || parent_item_uid)
			{
				if (item.equipment_slot != equipment_slot)
					topology_mismatch = true;
				item.equipment_slot = static_cast<uint16_t>(equipment_slot);
			}
		}
	}
	catch (const std::bad_alloc &)
	{
		mysql_free_result(rows);
		return { false, ENOMEM };
	}
	mysql_free_result(rows);
	if (matched.size() != expected.size())
	{
		persistence_custody_witness witness;
		for (const auto &[uid, item] : expected)
			if (!matched.count(uid) && (!witness.item_uid || uid < witness.item_uid))
			{
				witness.item_uid = uid;
				witness.expected_present = true;
				witness.expected_root = item.root_item_uid;
				witness.expected_parent = item.parent_item_uid;
				witness.expected_vnum = item.vnum;
				witness.expected_slot = item.equipment_slot;
			}
		return custody_payload_mismatch(
			player_save_custody_diagnosis::snapshot_item_absent_from_custody, witness);
	}
	if (!topology_mismatch)
		return { true, 0 };

	try
	{
		std::vector<std::vector<size_t>> children(snapshot.items.size());
		std::vector<size_t> roots;
		roots.reserve(snapshot.items.size());
		for (size_t index = 0; index < snapshot.items.size(); ++index)
		{
			const uint64_t item_uid = snapshot.items[index].object_uid;
			const auto item = expected.find(item_uid);
			if (item == expected.end())
				return custody_payload_mismatch(
					player_save_custody_diagnosis::invalid_custody_topology);
			if (!item->second.parent_item_uid)
			{
				if (item->second.root_item_uid != item_uid)
					return custody_payload_mismatch(
						player_save_custody_diagnosis::
							invalid_custody_topology,
						item->second.witness);
				roots.push_back(index);
				continue;
			}
			const auto parent = expected.find(item->second.parent_item_uid);
			if (parent == expected.end() ||
			    parent->second.root_item_uid != item->second.root_item_uid ||
			    parent->second.snapshot_index >= snapshot.items.size())
				return custody_payload_mismatch(
					player_save_custody_diagnosis::invalid_custody_topology,
					item->second.witness);
			children[parent->second.snapshot_index].push_back(index);
		}

		std::vector<size_t> order;
		std::vector<size_t> stack;
		std::vector<size_t> depths(snapshot.items.size(), 0);
		std::vector<bool> visited(snapshot.items.size(), false);
		order.reserve(snapshot.items.size());
		stack.reserve(snapshot.items.size());
		for (size_t root : roots)
		{
			depths[root] = 1;
			stack.push_back(root);
			while (!stack.empty())
			{
				const size_t index = stack.back();
				stack.pop_back();
				if (visited[index])
					return custody_payload_mismatch(
						player_save_custody_diagnosis::
							invalid_custody_topology,
						expected.at(snapshot.items[index].object_uid)
							.witness);
				visited[index] = true;
				order.push_back(index);
				for (auto child = children[index].rbegin();
				     child != children[index].rend(); ++child)
				{
					const size_t child_depth = depths[index] + 1;
					if (child_depth > PLAYER_SNAPSHOT_MAX_DEPTH)
						return custody_payload_mismatch(
							player_save_custody_diagnosis::
								invalid_custody_topology,
							expected.at(snapshot.items[*child]
									    .object_uid)
								.witness);
					depths[*child] = child_depth;
					stack.push_back(*child);
				}
			}
		}
		if (order.size() != snapshot.items.size())
			for (size_t index = 0; index < visited.size(); ++index)
				if (!visited[index])
					return custody_payload_mismatch(
						player_save_custody_diagnosis::
							invalid_custody_topology,
						expected.at(snapshot.items[index].object_uid)
							.witness);

		std::unordered_map<uint64_t, int32_t> projected_index;
		projected_index.reserve(order.size());
		reconciled_items->reserve(order.size());
		for (size_t original_index : order)
		{
			player_item_snapshot item = snapshot.items[original_index];
			const auto custody = expected.find(item.object_uid);
			if (custody == expected.end())
				return custody_payload_mismatch(
					player_save_custody_diagnosis::invalid_custody_topology);
			if (custody->second.parent_item_uid)
			{
				const auto parent =
					projected_index.find(custody->second.parent_item_uid);
				if (parent == projected_index.end())
					return custody_payload_mismatch(
						player_save_custody_diagnosis::
							invalid_custody_topology,
						custody->second.witness);
				item.parent_index = parent->second;
			}
			else
				item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
			item.equipment_slot = static_cast<int16_t>(custody->second.equipment_slot);
			if (reconciled_items->size() >
				    static_cast<size_t>(std::numeric_limits<int32_t>::max()) ||
			    !projected_index
				     .emplace(item.object_uid,
					      static_cast<int32_t>(reconciled_items->size()))
				     .second)
				return custody_payload_mismatch(
					player_save_custody_diagnosis::invalid_custody_topology,
					custody->second.witness);
			reconciled_items->push_back(std::move(item));
		}
	}
	catch (const std::bad_alloc &)
	{
		reconciled_items->clear();
		return { false, ENOMEM };
	}
	*topology_reconciled = true;
	return { true, 0 };
}

// A death disposition deliberately has no active inventory snapshot: the live
// items are recorded in its immutable corpse payload instead. Before removing
// the old player_items projection, prove that every stored payload is present
// in that corpse. A custody row with no payload may then be quarantined by its
// captured root without making the normal complete-save check destructive.
query_result verify_player_death_item_payload(MYSQL *connection, const player_snapshot &snapshot)
{
	if (!snapshot.death || !snapshot.items.empty())
		return custody_payload_mismatch(
			player_save_custody_diagnosis::invalid_death_payload);
	std::unordered_map<uint64_t, int32_t> captured;
	try
	{
		captured.reserve(snapshot.death->corpse.size());
		for (const player_item_snapshot &item : snapshot.death->corpse)
			if (!item.object_uid || item.vnum <= 0 ||
			    !captured.emplace(item.object_uid, item.vnum).second)
				return custody_payload_mismatch(
					player_save_custody_diagnosis::invalid_death_payload);
	}
	catch (const std::bad_alloc &)
	{
		return { false, ENOMEM };
	}
	const query_result query =
		execute(connection, "SELECT obj_uid,vnum FROM player_items WHERE pid=" +
					    std::to_string(snapshot.pid) + " FOR UPDATE");
	if (!query.ok)
		return query;
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
		return { false, mysql_errno(connection) };
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(rows)) != nullptr)
	{
		uint64_t item_uid = 0;
		if (!parse_custody_uint64(row[0], &item_uid) || !item_uid || !row[1])
		{
			mysql_free_result(rows);
			return custody_payload_mismatch(
				player_save_custody_diagnosis::malformed_active_custody_row);
		}
		const auto found = captured.find(item_uid);
		if (found == captured.end() || std::to_string(found->second) != row[1])
		{
			mysql_free_result(rows);
			return custody_payload_mismatch(
				player_save_custody_diagnosis::saved_item_absent_from_death_payload);
		}
		captured.erase(found);
	}
	mysql_free_result(rows);
	return { true, 0 };
}

// A load deliberately skips payload rows with no custody. Keep their only
// surviving copy intact until an operator resolves them from frozen evidence.
query_result reject_orphaned_saved_items(MYSQL *connection, int pid, bool pets)
{
	const std::string sql =
		pets ? "SELECT ppi.id,ppi.obj_uid FROM player_pet_items ppi JOIN player_pets pp ON "
		       "pp.id=ppi.pet_id LEFT JOIN item_current_owner own ON "
		       "own.item_uid=ppi.obj_uid WHERE pp.owner_pid=" +
				std::to_string(pid) +
				" AND own.item_uid IS NULL LIMIT 1 FOR UPDATE" :
		       "SELECT pi.id,pi.obj_uid FROM player_items pi LEFT JOIN item_current_owner own ON "
		       "own.item_uid=pi.obj_uid WHERE pi.pid=" +
				std::to_string(pid) +
				" AND own.item_uid IS NULL LIMIT 1 FOR UPDATE";
	query_result query = execute(connection, sql);
	if (!query.ok)
		return query;
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
		return { false, mysql_errno(connection) };
	MYSQL_ROW orphan = mysql_fetch_row(rows);
	const bool orphaned = orphan != nullptr;
	persistence_custody_witness witness;
	if (orphan)
	{
		parse_custody_uint64(orphan[1], &witness.item_uid);
		witness.expected_present = true;
	}
	mysql_free_result(rows);
	return orphaned ? custody_payload_mismatch(
				  pets ? player_save_custody_diagnosis::orphaned_saved_pet_item :
					 player_save_custody_diagnosis::orphaned_saved_item,
				  witness) :
			  query_result{ true, 0 };
}

query_result apply_items(MYSQL *connection, const player_snapshot &snapshot)
{
	query_result orphan_check = reject_orphaned_saved_items(connection, snapshot.pid, false);
	if (!orphan_check.ok)
		return orphan_check;
	const bool equipment = snapshot.components & PLAYER_COMPONENT_EQUIPMENT;
	const bool inventory = snapshot.components & PLAYER_COMPONENT_INVENTORY;
	/* New snapshots always replace both halves together. Keep accepting legacy
	 * component-only journal records, but reconcile a complete item's topology
	 * to locked custody before the destructive projection. UID/vnum conflicts,
	 * missing payloads and invalid authoritative graphs still fail closed. */
	std::vector<player_item_snapshot> reconciled_items;
	const std::vector<player_item_snapshot> *projection_items = &snapshot.items;
	if (equipment && inventory)
	{
		query_result verified = { true, 0 };
		if (snapshot.death)
			verified = verify_player_death_item_payload(connection, snapshot);
		else
		{
			bool topology_reconciled = false;
			verified = reconcile_player_item_custody(
				connection, snapshot, &reconciled_items, &topology_reconciled);
			if (verified.ok && topology_reconciled)
				projection_items = &reconciled_items;
		}
		if (!verified.ok)
			return verified;
	}
	std::string deletion = "DELETE FROM player_items WHERE pid=" + std::to_string(snapshot.pid);
	if (equipment != inventory)
		deletion += equipment ? " AND equip_slot>0" : " AND equip_slot=0";
	query_result result = execute(connection, deletion);
	if (!result.ok)
		return result;
	result = insert_item_rows(connection, *projection_items, snapshot.pid, false);
	if (!result.ok)
		return result;
	return sync_restitution_runtime_state(connection, *projection_items, snapshot.pid);
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
	query_result orphan_check = reject_orphaned_saved_items(connection, snapshot.pid, true);
	if (!orphan_check.ok)
		return orphan_check;
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

query_result apply_components(MYSQL *connection, const player_snapshot &snapshot,
			      bool preserve_item_projections = false)
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
	if (result.ok && !preserve_item_projections &&
	    (snapshot.components & (PLAYER_COMPONENT_EQUIPMENT | PLAYER_COMPONENT_INVENTORY)))
		result = apply_items(connection, snapshot);
	if (result.ok && !preserve_item_projections &&
	    (snapshot.components & PLAYER_COMPONENT_PETS))
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

query_result apply_quest_xp_receipts(MYSQL *connection, const player_snapshot &snapshot)
{
	for (const auto &receipt : snapshot.quest_xp_receipts)
	{
		const std::string operation_hex = hex_operation(receipt.offering_operation);
		const std::string entitlement_sql =
			"SELECT q.continuation,e.applied_at IS NOT NULL,"
			"q.acknowledged_at IS NOT NULL,e.amount "
			"FROM quest_reward_xp_entitlement e JOIN quest_reward_obligation q "
			"ON q.offering_operation_id=e.offering_operation_id "
			"WHERE e.offering_operation_id=UNHEX('" +
			operation_hex +
			"') AND "
			"e.recipient_pid=" +
			std::to_string(snapshot.pid) +
			" AND e.reward_index=" + std::to_string(receipt.reward_index) +
			" FOR UPDATE";
		query_result entitlement_query = execute(connection, entitlement_sql);
		if (!entitlement_query.ok)
			return entitlement_query;
		std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> entitlement_result(
			mysql_store_result(connection), mysql_free_result);
		if (!entitlement_result)
			return { false, mysql_errno(connection) ? mysql_errno(connection) : EIO };
		MYSQL_ROW entitlement_row = mysql_fetch_row(entitlement_result.get());
		unsigned long *entitlement_lengths =
			entitlement_row ? mysql_fetch_lengths(entitlement_result.get()) : nullptr;
		if (entitlement_row)
		{
			uint64_t entitlement_amount = 0;
			if (!entitlement_lengths || !entitlement_row[0] || !entitlement_row[1] ||
			    !entitlement_row[2] ||
			    !parse_custody_uint64(entitlement_row[3], &entitlement_amount) ||
			    entitlement_amount != receipt.amount)
				return { false, EINVAL };
			quest_reward_continuation terms;
			if (!quest_reward_continuation_decode(
				    reinterpret_cast<const uint8_t *>(entitlement_row[0]),
				    entitlement_lengths[0], &terms) ||
			    terms.version < 5 || receipt.reward_index >= terms.reward_count ||
			    terms.rewards[receipt.reward_index].type != 5U)
				return { false, EINVAL };
			bool matching_award = false;
			for (size_t index = 0; index < terms.xp_award_count; ++index)
				matching_award = matching_award ||
						 (terms.xp_awards[index].recipient_pid ==
							  static_cast<uint32_t>(snapshot.pid) &&
						  terms.xp_awards[index].reward_index ==
							  receipt.reward_index &&
						  terms.xp_awards[index].amount == receipt.amount);
			if (!matching_award)
				return { false, EINVAL };
			if (entitlement_row[1][0] == '1')
				continue;
			if (entitlement_row[2][0] != '0')
				return { false, EINVAL };
			const std::string update_entitlement_sql =
				"UPDATE quest_reward_xp_entitlement SET applied_at=CURRENT_TIMESTAMP(6) "
				"WHERE offering_operation_id=UNHEX('" +
				operation_hex +
				"') AND recipient_pid=" + std::to_string(snapshot.pid) +
				" AND reward_index=" + std::to_string(receipt.reward_index) +
				" AND applied_at IS NULL";
			entitlement_query = execute(connection, update_entitlement_sql);
			if (!entitlement_query.ok || mysql_affected_rows(connection) != 1)
				return { false, entitlement_query.ok ?
							EAGAIN :
							entitlement_query.error_code };
			if (terms.player_pid == static_cast<uint32_t>(snapshot.pid))
			{
				const uint64_t bit = UINT64_C(1) << receipt.reward_index;
				const std::string update_owner_mask_sql =
					"UPDATE quest_reward_obligation SET xp_applied_mask="
					"xp_applied_mask | " +
					std::to_string(bit) +
					" WHERE offering_operation_id=UNHEX('" + operation_hex +
					"') AND player_pid=" + std::to_string(snapshot.pid) +
					" AND acknowledged_at IS NULL";
				entitlement_query = execute(connection, update_owner_mask_sql);
				if (!entitlement_query.ok || mysql_affected_rows(connection) != 1)
					return { false, entitlement_query.ok ?
								EAGAIN :
								entitlement_query.error_code };
			}
			continue;
		}
		const std::string select_sql =
			"SELECT continuation,xp_applied_mask,acknowledged_at IS NOT NULL "
			"FROM quest_reward_obligation "
			"WHERE offering_operation_id=UNHEX('" +
			operation_hex + "') AND player_pid=" + std::to_string(snapshot.pid) +
			" FOR UPDATE";
		query_result query = execute(connection, select_sql);
		if (!query.ok)
			return query;
		std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(
			mysql_store_result(connection), mysql_free_result);
		if (!result)
			return { false, mysql_errno(connection) ? mysql_errno(connection) : EIO };
		MYSQL_ROW row = mysql_fetch_row(result.get());
		unsigned long *lengths = row ? mysql_fetch_lengths(result.get()) : nullptr;
		if (!row || !lengths || !row[0] || !row[1] || !row[2])
			return { false, ENOENT };
		quest_reward_continuation terms;
		if (!quest_reward_continuation_decode(reinterpret_cast<const uint8_t *>(row[0]),
						      lengths[0], &terms) ||
		    terms.version < 4 || (terms.version >= 5 && terms.credited_count > 1) ||
		    terms.player_pid != static_cast<uint32_t>(snapshot.pid) ||
		    receipt.reward_index >= terms.reward_count ||
		    terms.rewards[receipt.reward_index].type != 5U ||
		    terms.rewards[receipt.reward_index].frozen_amount != receipt.amount)
			return { false, EINVAL };
		char *end = nullptr;
		errno = 0;
		const unsigned long long applied_mask = std::strtoull(row[1], &end, 10);
		if (errno || !end || *end)
			return { false, EINVAL };
		const uint64_t bit = UINT64_C(1) << receipt.reward_index;
		if (applied_mask & bit)
			continue;
		if (row[2][0] != '0')
			return { false, EINVAL };
		const std::string update_sql =
			"UPDATE quest_reward_obligation SET xp_applied_mask=xp_applied_mask | " +
			std::to_string(bit) + " WHERE offering_operation_id=UNHEX('" +
			operation_hex + "') AND player_pid=" + std::to_string(snapshot.pid) +
			" AND acknowledged_at IS NULL";
		query = execute(connection, update_sql);
		if (!query.ok || mysql_affected_rows(connection) != 1)
			return { false, query.ok ? EAGAIN : query.error_code };
	}
	return { true, 0 };
}

query_result verify_quest_xp_receipts(MYSQL *connection, const player_snapshot &snapshot)
{
	for (const auto &receipt : snapshot.quest_xp_receipts)
	{
		const std::string operation_hex = hex_operation(receipt.offering_operation);
		query_result query = execute(
			connection,
			"SELECT q.continuation,q.xp_applied_mask,e.amount,e.applied_at IS NOT NULL "
			"FROM quest_reward_obligation q LEFT JOIN quest_reward_xp_entitlement e "
			"ON e.offering_operation_id=q.offering_operation_id AND e.recipient_pid=" +
				std::to_string(snapshot.pid) +
				" AND e.reward_index=" + std::to_string(receipt.reward_index) +
				" WHERE q.offering_operation_id=UNHEX('" + operation_hex + "')");
		if (!query.ok)
			return query;
		std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
			mysql_store_result(connection), mysql_free_result);
		if (!rows)
			return { false, mysql_errno(connection) ? mysql_errno(connection) : EIO };
		MYSQL_ROW row = mysql_fetch_row(rows.get());
		const unsigned long *lengths = row ? mysql_fetch_lengths(rows.get()) : nullptr;
		if (!row || !lengths || !row[0] || !row[1])
			return { false, EILSEQ };
		quest_reward_continuation terms;
		if (!quest_reward_continuation_decode(reinterpret_cast<const uint8_t *>(row[0]),
						      lengths[0], &terms) ||
		    terms.version < 4 || receipt.reward_index >= terms.reward_count ||
		    terms.rewards[receipt.reward_index].type != 5U)
			return { false, EILSEQ };
		if (terms.version >= 5)
		{
			bool matching_award = false;
			for (size_t index = 0; index < terms.xp_award_count; ++index)
				matching_award = matching_award ||
						 (terms.xp_awards[index].recipient_pid ==
							  static_cast<uint32_t>(snapshot.pid) &&
						  terms.xp_awards[index].reward_index ==
							  receipt.reward_index &&
						  terms.xp_awards[index].amount == receipt.amount);
			if (!matching_award)
				return { false, EILSEQ };
			if (row[2])
			{
				if (!row[3] || row[3][0] != '1')
					return { false, EILSEQ };
				char *end = nullptr;
				errno = 0;
				const unsigned long amount = std::strtoul(row[2], &end, 10);
				if (errno || !end || *end || amount != receipt.amount)
					return { false, EILSEQ };
				continue;
			}
			if (terms.credited_count != 1)
				return { false, EILSEQ };
		}
		if (terms.player_pid != static_cast<uint32_t>(snapshot.pid) ||
		    terms.rewards[receipt.reward_index].frozen_amount != receipt.amount)
			return { false, EILSEQ };
		char *end = nullptr;
		errno = 0;
		const unsigned long long applied_mask = std::strtoull(row[1], &end, 10);
		if (errno || !end || *end ||
		    !(applied_mask & (UINT64_C(1) << receipt.reward_index)))
			return { false, EILSEQ };
	}
	return { true, 0 };
}

query_result apply_spell_effect_receipts(MYSQL *connection, const player_snapshot &snapshot)
{
	for (const auto &receipt : snapshot.spell_effect_receipts)
	{
		if (!receipt.effect_id ||
		    receipt.effect_id > PLAYER_SPELL_EFFECT_RECEIPT_EFFECT_MAX)
			return { false, EINVAL };
		const std::string operation_hex = hex_operation(receipt.operation_id);
		query_result query =
			execute(connection,
				"SELECT effect_id FROM player_spell_effect_receipt WHERE pid=" +
					std::to_string(snapshot.pid) + " AND operation_id=UNHEX('" +
					operation_hex + "') FOR UPDATE");
		if (!query.ok)
			return query;
		std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(
			mysql_store_result(connection), mysql_free_result);
		if (!result)
			return { false, mysql_errno(connection) ? mysql_errno(connection) : EIO };
		MYSQL_ROW row = mysql_fetch_row(result.get());
		if (row)
		{
			char *end = nullptr;
			errno = 0;
			const unsigned long stored_effect =
				std::strtoul(row[0] ? row[0] : "", &end, 10);
			if (errno || !end || *end || stored_effect != receipt.effect_id)
				return { false, EINVAL };
			continue;
		}
		query = execute(
			connection,
			"INSERT INTO player_spell_effect_receipt (pid,operation_id,effect_id) VALUES (" +
				std::to_string(snapshot.pid) + ",UNHEX('" + operation_hex + "')," +
				std::to_string(receipt.effect_id) + ")");
		if (!query.ok || mysql_affected_rows(connection) != 1)
			return { false, query.ok ? EAGAIN : query.error_code };
	}
	return { true, 0 };
}

query_result verify_spell_effect_receipts(MYSQL *connection, const player_snapshot &snapshot)
{
	for (const auto &receipt : snapshot.spell_effect_receipts)
	{
		const std::string operation_hex = hex_operation(receipt.operation_id);
		const query_result query = execute(
			connection, "SELECT effect_id FROM player_spell_effect_receipt WHERE pid=" +
					    std::to_string(snapshot.pid) +
					    " AND operation_id=UNHEX('" + operation_hex + "')");
		if (!query.ok)
			return query;
		std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
			mysql_store_result(connection), mysql_free_result);
		if (!rows)
			return { false, mysql_errno(connection) ? mysql_errno(connection) : EIO };
		MYSQL_ROW row = mysql_fetch_row(rows.get());
		if (!row || !row[0])
			return { false, EILSEQ };
		char *end = nullptr;
		errno = 0;
		const unsigned long stored_effect = std::strtoul(row[0], &end, 10);
		if (errno || !end || *end || stored_effect != receipt.effect_id)
			return { false, EILSEQ };
	}
	return { true, 0 };
}

query_result craft_receipts(MYSQL *connection, const player_snapshot &snapshot, bool apply)
{
	for (const auto &receipt : snapshot.craft_receipts)
	{
		const std::string operation_hex = hex_operation(receipt.operation_id);
		auto query = execute(
			connection,
			"SELECT c.discipline,c.experience,c.applied_revision,i.status,i.result_code "
			"FROM player_craft_progression c JOIN critical_operation_inbox i "
			"ON i.operation_id=c.operation_id WHERE c.pid=" +
				std::to_string(snapshot.pid) + " AND c.operation_id=UNHEX('" +
				operation_hex + "') FOR UPDATE");
		if (!query.ok)
			return query;
		std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
			mysql_store_result(connection), mysql_free_result);
		if (!rows || mysql_num_rows(rows.get()) != 1 || mysql_num_fields(rows.get()) != 5)
			return { false,
				 rows ? ENOENT :
					(mysql_errno(connection) ? mysql_errno(connection) : EIO) };
		MYSQL_ROW row = mysql_fetch_row(rows.get());
		std::array<uint64_t, 5> values = {};
		for (size_t index = 0; index < values.size(); ++index)
			if (!row || !parse_custody_uint64(row[index], &values[index]))
				return { false, EILSEQ };
		rows.reset();
		if (values[0] != receipt.discipline || values[1] != receipt.experience ||
		    values[2] > snapshot.revision || values[3] != 1 || values[4] != 0)
			return { false, EILSEQ };
		if (values[2])
			continue;
		if (!apply)
			return { false, ENOENT };
		query = execute(connection,
				"UPDATE player_craft_progression SET applied_revision=" +
					std::to_string(snapshot.revision) +
					" WHERE operation_id=UNHEX('" + operation_hex +
					"') AND pid=" + std::to_string(snapshot.pid) +
					" AND applied_revision=0");
		if (!query.ok || mysql_affected_rows(connection) != 1)
			return { false, query.ok ? EAGAIN : query.error_code };
	}
	return { true, 0 };
}

std::string hex_payload(const std::vector<uint8_t> &payload)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string output;
	output.reserve(payload.size() * 2);
	for (uint8_t byte : payload)
	{
		output.push_back(digits[byte >> 4]);
		output.push_back(digits[byte & 15]);
	}
	return output;
}

// Evidence-only records use INSERT and format 10: they must not overwrite an
// earlier disposition or enter the ordinary restitution/ownership path.
query_result record_death(MYSQL *connection, const player_snapshot &snapshot, bool evidence_only)
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
	sql << (evidence_only ? "INSERT" : "REPLACE")
	    << " INTO player_death_disposition (pid,save_revision,operation_id,"
	       "corpse_item_uid,corpse_room_vnum,wallet_revision,wallet_copper,wallet_silver,"
	       "wallet_gold,wallet_platinum,wallet_pile_uid,payload) VALUES ("
	    << pid << ',' << revision << ",UNHEX('" << hex_operation(death.operation_id) << "'),"
	    << death.corpse.front().object_uid << ',' << death.corpse_room_vnum << ','
	    << death.wallet_revision;
	for (int32_t amount : death.wallet_before)
		sql << ',' << amount;
	sql << ',' << death.wallet_pile_uid << ",UNHEX('" << hex_payload(payload) << "'))";
	query_result result = execute(connection, sql.str());
	if (!result.ok)
		return result;
	if (!evidence_only)
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
	return result;
}

// Ordinary death disposition behavior is unchanged. Conflict evidence has a
// separate caller and must never execute these custody-quarantine mutations.
query_result apply_death(MYSQL *connection, const player_snapshot &snapshot)
{
	if (!snapshot.death)
		return { true, 0 };
	query_result result = record_death(connection, snapshot, false);
	if (!result.ok)
		return result;
	const std::string pid = std::to_string(snapshot.pid);
	const std::string revision = std::to_string(snapshot.revision);
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

query_result verify_conflict_archive(MYSQL *connection, const player_snapshot &request,
				     const std::vector<uint8_t> &retained_bytes,
				     const player_revision_t *source_revision)
{
	std::vector<uint8_t> request_bytes;
	if (!request.death || request.death->corpse.empty() ||
	    player_snapshot_encode(request, &request_bytes) != player_snapshot_codec_result::ok)
		return { false, EINVAL };
	std::string sql =
		"SELECT 1 FROM player_death_conflict_evidence WHERE pid=" +
		std::to_string(request.pid) +
		" AND save_revision=" + std::to_string(request.revision) +
		" AND operation_id=UNHEX('" + hex_operation(request.death->operation_id) + "')" +
		" AND corpse_item_uid=" + std::to_string(request.death->corpse.front().object_uid) +
		" AND source_revision<save_revision AND payload=UNHEX('" +
		hex_payload(retained_bytes) +
		"') AND payload_hash=UNHEX(SHA2(payload,256)) AND request_hash=UNHEX(SHA2(UNHEX('" +
		hex_payload(request_bytes) + "'),256))";
	if (source_revision)
		sql += " AND source_revision=" + std::to_string(*source_revision);
	const auto query = execute(connection, sql + " LIMIT 2 FOR UPDATE");
	if (!query.ok)
		return query;
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	if (!rows)
		return { false, mysql_errno(connection) ? mysql_errno(connection) : EIO };
	return mysql_num_rows(rows.get()) == 1 ? query_result{ true, 0 } :
						 query_result{ false, EILSEQ };
}

// A revision counter alone is not a death receipt. Compare the exact immutable
// request; format-10 receipts additionally require the matching durable archive.
query_result verify_death_receipt(MYSQL *connection, const player_snapshot &request)
{
	if (!request.death)
		return { true, 0 };
	if (request.death->corpse.empty())
		return { false, EINVAL };
	std::string sql =
		"SELECT CASE WHEN OCTET_LENGTH(payload)<=" +
		std::to_string(PLAYER_SNAPSHOT_MAX_BYTES) +
		" THEN payload ELSE NULL END FROM player_death_disposition WHERE pid=" +
		std::to_string(request.pid) +
		" AND save_revision=" + std::to_string(request.revision) +
		" AND operation_id=UNHEX('" + hex_operation(request.death->operation_id) + "')" +
		" AND corpse_item_uid=" + std::to_string(request.death->corpse.front().object_uid) +
		" AND corpse_room_vnum=" + std::to_string(request.death->corpse_room_vnum) +
		" AND wallet_revision=" + std::to_string(request.death->wallet_revision) +
		" AND wallet_pile_uid=" + std::to_string(request.death->wallet_pile_uid);
	static constexpr const char *wallet_columns[] = { "wallet_copper", "wallet_silver",
							  "wallet_gold", "wallet_platinum" };
	for (size_t i = 0; i < request.death->wallet_before.size(); ++i)
		sql += " AND " + std::string(wallet_columns[i]) + "=" +
		       std::to_string(request.death->wallet_before[i]);
	const auto query = execute(connection, sql + " LIMIT 2");
	if (!query.ok)
		return query;
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	if (!rows)
		return { false, mysql_errno(connection) ? mysql_errno(connection) : EIO };
	if (mysql_num_rows(rows.get()) != 1)
		return { false, EILSEQ };
	const auto row = mysql_fetch_row(rows.get());
	const auto lengths = mysql_fetch_lengths(rows.get());
	if (!row || !row[0] || !lengths)
		return { false, EILSEQ };
	std::vector<uint8_t> payload(reinterpret_cast<const uint8_t *>(row[0]),
				     reinterpret_cast<const uint8_t *>(row[0]) + lengths[0]);
	rows.reset();
	player_snapshot stored;
	if (player_snapshot_decode(payload.data(), payload.size(), &stored) !=
		    player_snapshot_codec_result::ok ||
	    !stored.death)
		return { false, EILSEQ };
	const bool evidence = player_snapshot_is_death_evidence_schema(stored.schema_version);
	if (evidence)
	{
		const auto archive = verify_conflict_archive(connection, request, payload, nullptr);
		if (!archive.ok)
			return archive;
		stored.schema_version = player_snapshot_death_request_schema(stored.schema_version);
		stored.death->conflict_evidence.reset();
	}
	std::vector<uint8_t> expected, actual;
	if (player_snapshot_encode(request, &expected) != player_snapshot_codec_result::ok ||
	    player_snapshot_encode(stored, &actual) != player_snapshot_codec_result::ok ||
	    expected != actual)
		return { false, EILSEQ };
	return { true, 0 };
}

player_save_apply_result read_durable_revision(MYSQL *connection, const player_snapshot &snapshot)
{
	const query_result query =
		execute(connection, "SELECT save_revision FROM player_data WHERE pid=" +
					    std::to_string(snapshot.pid));
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
	if (snapshot.death && revision >= snapshot.revision)
	{
		const auto receipt = verify_death_receipt(connection, snapshot);
		if (!receipt.ok)
			return failure(receipt.error_code);
	}
	if (revision == snapshot.revision && !snapshot.spell_effect_receipts.empty())
	{
		const auto receipt = verify_spell_effect_receipts(connection, snapshot);
		if (!receipt.ok)
			return failure(receipt.error_code);
	}
	if (revision == snapshot.revision && !snapshot.craft_receipts.empty())
	{
		const auto receipt = craft_receipts(connection, snapshot, false);
		if (!receipt.ok)
			return failure(receipt.error_code);
	}
	if (revision == snapshot.revision && !snapshot.quest_xp_receipts.empty())
	{
		const auto receipt = verify_quest_xp_receipts(connection, snapshot);
		if (!receipt.ok)
			return failure(receipt.error_code);
	}
	return { player_save_apply_outcome::already_applied, revision, 0 };
}
} // namespace

bool player_snapshot_repository_write_pets(MYSQL *connection, const player_snapshot &snapshot)
{
	return connection && snapshot.pid > 0 && apply_pets(connection, snapshot).ok;
}

player_death_terminal_write_result
player_snapshot_repository_write_retained_death(MYSQL *connection, const player_snapshot &request,
						const player_snapshot &retained,
						player_revision_t source_revision)
{
	using outcome = player_death_terminal_write_outcome;
	const auto failed = [](unsigned int code)
	{ return player_death_terminal_write_result{ outcome::failed, code ? code : EIO }; };
	if (!connection)
		return failed(EINVAL);
#ifndef __NO_MYSQL__
	if (!(connection->server_status & SERVER_STATUS_IN_TRANS))
		return failed(EINVAL);
#endif
	if (request.pid <= 0 || !request.death ||
	    !player_snapshot_is_death_request_schema(request.schema_version) ||
	    request.components != PLAYER_CHECKPOINT_COMPONENT_ALL || !request.items.empty() ||
	    source_revision >= request.revision || !retained.death ||
	    !player_snapshot_is_death_evidence_schema(retained.schema_version) ||
	    !retained.death->conflict_evidence)
		return failed(EINVAL);
	// The existing wallet transaction, not a death snapshot, owns conversion.
	// A nonzero or stale wallet remains a hold; this writer never debits it or
	// awards a replacement pile alongside still-spendable money.
	if (request.death->wallet_pile_uid ||
	    std::any_of(request.death->wallet_before.begin(), request.death->wallet_before.end(),
			[](int32_t value) { return value != 0; }))
		return failed(EBUSY);
	std::vector<uint8_t> request_bytes, retained_bytes, original_bytes;
	auto original = retained;
	original.schema_version = player_snapshot_death_request_schema(retained.schema_version);
	original.death->conflict_evidence.reset();
	if (player_snapshot_encode(request, &request_bytes) != player_snapshot_codec_result::ok ||
	    player_snapshot_encode(retained, &retained_bytes) != player_snapshot_codec_result::ok ||
	    player_snapshot_encode(original, &original_bytes) != player_snapshot_codec_result::ok ||
	    request_bytes != original_bytes)
		return failed(EILSEQ);
	auto query = verify_conflict_archive(connection, request, retained_bytes, &source_revision);
	if (!query.ok)
		return failed(query.error_code);
	query = execute(
		connection,
		"SELECT save_revision,wallet_revision,copper,silver,gold,platinum FROM player_data WHERE pid=" +
			std::to_string(request.pid) + " FOR UPDATE");
	if (!query.ok)
		return failed(query.error_code);
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	if (!rows)
		return failed(mysql_errno(connection));
	if (mysql_num_rows(rows.get()) != 1 || mysql_num_fields(rows.get()) != 6)
		return failed(ENOENT);
	const auto row = mysql_fetch_row(rows.get());
	std::array<uint64_t, 6> state = {};
	for (size_t index = 0; index < state.size(); ++index)
		if (!row || !parse_custody_uint64(row[index], &state[index]))
			return failed(EILSEQ);
	rows.reset();
	if (state[1] != request.death->wallet_revision ||
	    std::any_of(state.begin() + 2, state.end(), [](uint64_t value) { return value != 0; }))
		return failed(ESTALE);
	if (state[0] == request.revision)
	{
		query = verify_death_receipt(connection, request);
		if (query.ok)
			query = verify_quest_xp_receipts(connection, request);
		if (query.ok)
			query = verify_spell_effect_receipts(connection, request);
		if (query.ok)
			query = craft_receipts(connection, request, false);
		return query.ok ?
			       player_death_terminal_write_result{ outcome::already_written, 0 } :
			       failed(query.error_code);
	}
	if (state[0] != source_revision)
		return failed(ESTALE);
	query = verify_player_death_item_payload(connection, request);
	if (query.ok || query.error_code != PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH)
		return failed(query.ok ? EAGAIN : query.error_code);
	query = apply_components(connection, request, true);
	if (query.ok)
		query = apply_quest_xp_receipts(connection, request);
	if (query.ok)
		query = apply_spell_effect_receipts(connection, request);
	if (query.ok)
		query = craft_receipts(connection, request, true);
	if (query.ok)
		query = record_death(connection, retained, true);
	if (!query.ok)
		return failed(query.error_code);
	query = execute(connection,
			"UPDATE player_data SET save_revision=" + std::to_string(request.revision) +
				" WHERE pid=" + std::to_string(request.pid) +
				" AND save_revision=" + std::to_string(source_revision));
	if (!query.ok || mysql_affected_rows(connection) != 1)
		return failed(query.ok ? EAGAIN : query.error_code);
	query = verify_death_receipt(connection, request);
	if (query.ok)
		query = verify_quest_xp_receipts(connection, request);
	if (query.ok)
		query = verify_spell_effect_receipts(connection, request);
	if (query.ok)
		query = craft_receipts(connection, request, false);
	return query.ok ? player_death_terminal_write_result{ outcome::written, 0 } :
			  failed(query.error_code);
}

player_save_apply_result player_snapshot_repository_apply(MYSQL *connection,
							  const player_snapshot &snapshot)
{
	if (snapshot.pid > 0 && player_save_journal_pid_quarantined(snapshot.pid))
		return { player_save_apply_outcome::terminal_failure, 0, EPERM };
	if (!connection || snapshot.pid <= 0 || !snapshot.revision || !snapshot.components ||
	    (snapshot.components & ~PLAYER_CHECKPOINT_COMPONENT_ALL))
		return { player_save_apply_outcome::terminal_failure, 0, EINVAL };
	// Death records require their custody disposition to commit with this save.
	// Never acknowledge one through the ordinary component-only writer.
	if (snapshot.death ?
		    !player_snapshot_is_death_request_schema(snapshot.schema_version) ||
			    snapshot.death->corpse.empty() ||
			    snapshot.components != PLAYER_CHECKPOINT_COMPONENT_ALL ||
			    !snapshot.items.empty() :
		    (snapshot.schema_version != PLAYER_SNAPSHOT_SCHEMA_VERSION &&
		     snapshot.schema_version != PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION &&
		     snapshot.schema_version != PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION &&
		     snapshot.schema_version !=
			     PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION))
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
		const auto receipt = verify_death_receipt(connection, snapshot);
		const auto spell_receipt =
			receipt.ok ? verify_spell_effect_receipts(connection, snapshot) : receipt;
		const auto quest_receipt = spell_receipt.ok ?
						   verify_quest_xp_receipts(connection, snapshot) :
						   spell_receipt;
		const auto craft_receipt = quest_receipt.ok ?
						   craft_receipts(connection, snapshot, false) :
						   quest_receipt;
		execute(connection, "ROLLBACK");
		if (!receipt.ok)
			return failure(receipt.error_code, receipt.custody_diagnosis);
		if (!spell_receipt.ok)
			return failure(spell_receipt.error_code);
		if (!quest_receipt.ok)
			return failure(quest_receipt.error_code);
		if (!craft_receipt.ok)
			return failure(craft_receipt.error_code);
		return { durable == snapshot.revision ? player_save_apply_outcome::already_applied :
							player_save_apply_outcome::stale_revision,
			 durable, 0, player_save_custody_diagnosis::none,
			 !snapshot.death && (!snapshot.quest_xp_receipts.empty() ||
					     !snapshot.spell_effect_receipts.empty() ||
					     !snapshot.craft_receipts.empty()) };
	}

	// An unresolved case also fences later checkpoints, not just cold loads.
	// Empty/partial live state must not overwrite preserved authoritative rows.
	query = execute(connection,
			"SELECT operation_id FROM player_death_conflict_evidence WHERE pid=" +
				std::to_string(snapshot.pid) + " LIMIT 1");
	if (!query.ok)
	{
		execute(connection, "ROLLBACK");
		return failure(query.error_code);
	}
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> cases(
		mysql_store_result(connection), mysql_free_result);
	if (!cases || mysql_num_rows(cases.get()))
	{
		const auto code = cases ? EBUSY : mysql_errno(connection);
		cases.reset();
		execute(connection, "ROLLBACK");
		return failure(code ? code : EIO);
	}
	cases.reset();
	query = apply_components(connection, snapshot);
	if (query.ok && !snapshot.quest_xp_receipts.empty())
		query = apply_quest_xp_receipts(connection, snapshot);
	if (query.ok && !snapshot.spell_effect_receipts.empty())
		query = apply_spell_effect_receipts(connection, snapshot);
	if (query.ok && !snapshot.craft_receipts.empty())
		query = craft_receipts(connection, snapshot, true);
	if (query.ok)
		query = apply_death(connection, snapshot);
	if (!query.ok)
	{
		execute(connection, "ROLLBACK");
		player_save_apply_result failed =
			failure(query.error_code, query.custody_diagnosis);
		failed.durable_revision = durable;
		failed.custody_witness = query.custody_witness;
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

player_save_apply_result
player_snapshot_repository_recovery_apply(MYSQL *connection,
					  const player_save_recovery_record &record)
try
{
	if (!connection || record.backend != 2 || !player_save_journal_recovery_matches(record))
		return { player_save_apply_outcome::terminal_failure, 0, EPERM };
#ifndef __NO_MYSQL__
	if (!(connection->server_status & SERVER_STATUS_AUTOCOMMIT) ||
	    (connection->server_status & SERVER_STATUS_IN_TRANS))
		return { player_save_apply_outcome::terminal_failure, 0, EBUSY };
#endif
	const auto &snapshot = record.replacement;
	auto query = execute(connection, "SET TRANSACTION ISOLATION LEVEL REPEATABLE READ");
	if (query.ok)
		query = execute(connection, "START TRANSACTION");
	if (!query.ok)
		return failure(query.error_code);
	const auto rollback_transaction = [](MYSQL *owner)
	{
#ifndef __NO_MYSQL__
		if (!(owner->server_status & SERVER_STATUS_IN_TRANS))
			return;
#endif
		mysql_real_query(owner, "ROLLBACK", 8);
	};
	std::unique_ptr<MYSQL, decltype(rollback_transaction)> rollback_guard(connection,
									      rollback_transaction);
	query = execute(connection, "SELECT pid FROM player_data WHERE pid=" +
					    std::to_string(snapshot.pid) + " FOR UPDATE");
	if (!query.ok)
	{
		execute(connection, "ROLLBACK");
		return failure(query.error_code);
	}
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	if (!rows || mysql_num_rows(rows.get()) != 1)
	{
		rows.reset();
		execute(connection, "ROLLBACK");
		return { player_save_apply_outcome::terminal_failure, 0, ENOENT };
	}
	rows.reset();
	bool present = false;
	const bool receipt =
		player_quarantine_recovery_sql_receipt(connection, record, false, &present);
	const auto current =
		receipt ? player_load_repository_quarantine_inspect(
				  connection, player_quarantine_recovery_request(record), &record) :
			  player_load_result{};
	if (receipt && present && player_quarantine_recovery_state_matches(current, record, true))
	{
		execute(connection, "ROLLBACK");
		return { player_save_apply_outcome::already_applied, snapshot.revision, 0 };
	}
	if (!receipt || present ||
	    !player_quarantine_recovery_state_matches(current, record, false) ||
	    !player_quarantine_recovery_creation_proofs_sql(connection, record))
	{
		execute(connection, "ROLLBACK");
		return { player_save_apply_outcome::terminal_failure, current.snapshot.revision,
			 ESTALE };
	}
	// Reuse the native component/custody writers. Recovery never applies a grant,
	// economic mutation, death disposition, or operation-bearing save receipt.
	query = apply_components(connection, snapshot);
	if (query.ok)
		query = execute(connection, "UPDATE player_data SET save_revision=" +
						    std::to_string(snapshot.revision) +
						    " WHERE pid=" + std::to_string(snapshot.pid) +
						    " AND save_revision=" +
						    std::to_string(record.baseline.revision));
	if (!query.ok || mysql_affected_rows(connection) != 1)
	{
		execute(connection, "ROLLBACK");
		return failure(query.ok ? EAGAIN : query.error_code, query.custody_diagnosis);
	}
	const auto projected = player_load_repository_quarantine_inspect(
		connection, player_quarantine_recovery_request(record), &record);
	if (!player_quarantine_recovery_state_matches(projected, record, true) ||
	    !player_quarantine_recovery_sql_receipt(connection, record, true, &present) || !present)
	{
		execute(connection, "ROLLBACK");
		return { player_save_apply_outcome::terminal_failure, record.baseline.revision,
			 EILSEQ };
	}
	query = execute(connection, "COMMIT");
	if (!query.ok)
	{
		if (!connection_error(query.error_code))
			execute(connection, "ROLLBACK");
		return { connection_error(query.error_code) ?
				 player_save_apply_outcome::ambiguous_commit :
				 failure(query.error_code).outcome,
			 record.baseline.revision, query.error_code };
	}
	return { player_save_apply_outcome::applied, snapshot.revision, 0 };
}
catch (...)
{
	return { player_save_apply_outcome::retryable_failure, 0, ENOMEM };
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
		connection = sql_pool_replace_connection(connection);
		if (!connection)
			return applied;
	}
	if (applied.outcome == player_save_apply_outcome::ambiguous_commit)
	{
		const player_save_apply_result durable =
			read_durable_revision(connection, snapshot);
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

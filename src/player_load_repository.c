#include "player_load_repository.h"

#include "persistence_observability.h"

#include <mysql/mysql.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <sstream>
#include <string>
#include <strings.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace
{
struct status_column
{
	player_status_field field;
	const char *column;
	bool timestamp;
	bool unsigned_value;
};

constexpr std::array<status_column, 63> STATUS_COLUMNS = { {
	{ player_status_field::class_primary, "m_class", false, false },
	{ player_status_field::class_secondary, "secondary_class", false, false },
	{ player_status_field::specialization, "spec", false, false },
	{ player_status_field::race, "race", false, false },
	{ player_status_field::racewar, "racewar", false, false },
	{ player_status_field::level, "level", false, false },
	{ player_status_field::sex, "sex", false, false },
	{ player_status_field::weight, "weight", false, false },
	{ player_status_field::height, "height", false, false },
	{ player_status_field::size, "size", false, false },
	{ player_status_field::hometown, "hometown", false, false },
	{ player_status_field::birthplace, "birthplace", false, false },
	{ player_status_field::original_birthplace, "orig_birthplace", false, false },
	{ player_status_field::birth_time, "birth_time", true, false },
	{ player_status_field::played_time, "played_time", false, false },
	{ player_status_field::base_strength, "base_str", false, false },
	{ player_status_field::base_dexterity, "base_dex", false, false },
	{ player_status_field::base_agility, "base_agi", false, false },
	{ player_status_field::base_constitution, "base_con", false, false },
	{ player_status_field::base_power, "base_pow", false, false },
	{ player_status_field::base_intelligence, "base_int", false, false },
	{ player_status_field::base_wisdom, "base_wis", false, false },
	{ player_status_field::base_charisma, "base_cha", false, false },
	{ player_status_field::base_karma, "base_kar", false, false },
	{ player_status_field::base_luck, "base_luk", false, false },
	{ player_status_field::mana, "mana", false, false },
	{ player_status_field::base_mana, "base_mana", false, false },
	{ player_status_field::hit_difference, "hit_diff", false, false },
	{ player_status_field::base_hit, "base_hit", false, false },
	{ player_status_field::vitality, "vitality", false, false },
	{ player_status_field::base_vitality, "base_vitality", false, false },
	{ player_status_field::extra_memorization, "spells_memmed_extra", false, false },
	{ player_status_field::copper, "copper", false, true },
	{ player_status_field::silver, "silver", false, true },
	{ player_status_field::gold, "gold", false, true },
	{ player_status_field::platinum, "platinum", false, true },
	{ player_status_field::experience, "exp", false, false },
	{ player_status_field::epics, "epics", false, false },
	{ player_status_field::epic_skill_points, "epic_skill_points", false, false },
	{ player_status_field::skill_points, "skillpoints", false, false },
	{ player_status_field::spell_bind_used, "spell_bind_used", false, false },
	{ player_status_field::action_flags, "act", false, true },
	{ player_status_field::action_flags_2, "act2", false, true },
	{ player_status_field::action_flags_3, "act3", false, true },
	{ player_status_field::vote, "vote", false, true },
	{ player_status_field::alignment, "alignment", false, false },
	{ player_status_field::prestige, "prestige", false, false },
	{ player_status_field::guild_id, "assoc_id", false, false },
	{ player_status_field::guild_status, "guild_status", false, false },
	{ player_status_field::time_left_guild, "time_left_guild", true, false },
	{ player_status_field::times_left_guild, "nb_left_guild", false, false },
	{ player_status_field::time_unspecialized, "time_unspecced", true, false },
	{ player_status_field::frags, "frags", false, false },
	{ player_status_field::old_frags, "oldfrags", false, false },
	{ player_status_field::deaths, "numb_deaths", false, true },
	{ player_status_field::echo, "echo_toggle", false, false },
	{ player_status_field::prompt, "prompt", false, false },
	{ player_status_field::wizard_invisibility, "wiz_invis", false, true },
	{ player_status_field::wimpy, "wimpy", false, false },
	{ player_status_field::aggressive, "aggressive", false, false },
	{ player_status_field::highest_level, "highest_level", false, false },
	{ player_status_field::screen_length, "screen_length", false, false },
	{ player_status_field::last_ip, "last_ip", false, true },
} };

constexpr std::array<const char *, 7> STATUS_STRINGS = {
	"name", "short_descr", "long_descr", "description", "title", "poof_in", "poof_out",
};

constexpr std::array<const char *, 5> CONDITION_COLUMNS = {
	"condition_0", "condition_1", "condition_2", "condition_3", "condition_4",
};

constexpr std::array<const char *, 14> QUEST_COLUMNS = {
	"quest_active",	  "quest_mob_vnum",    "quest_type",	      "quest_accomplished",
	"quest_started",  "quest_zone_number", "quest_giver",	      "quest_level",
	"quest_receiver", "quest_shares_left", "quest_kill_how_many", "quest_kill_original",
	"quest_map_room", "quest_map_bought",
};

bool retryable(unsigned int error)
{
	return error == 1040 || error == 1205 || error == 1213 || error == 2002 || error == 2003 ||
	       error == 2006 || error == 2013;
}

player_load_outcome failure_outcome(unsigned int error)
{
	return retryable(error) ? player_load_outcome::retryable_failure :
				  player_load_outcome::component_failure;
}

bool within_budget(const player_load_result &result)
{
	return result.metrics.query_count <= PLAYER_LOAD_QUERY_MAX &&
	       result.metrics.row_count <= PLAYER_SNAPSHOT_MAX_ROWS &&
	       result.metrics.byte_count <= PLAYER_SNAPSHOT_MAX_BYTES;
}

bool before_deadline(const player_load_request &request)
{
	return persistence_observability_now_usec() <= request.deadline_usec;
}

MYSQL_RES *query(MYSQL *connection, const std::string &sql, player_load_result *result)
{
	if (!connection || !result)
		return nullptr;
	const uint64_t started = persistence_observability_now_usec();
	const int rc = mysql_real_query(connection, sql.data(), sql.size());
	const uint64_t finished = persistence_observability_now_usec();
	persistence_query_record(PERSISTENCE_QUERY_SITE,
				 PERSISTENCE_QUERY_CONTEXT_PLAYER_LOAD_WORKER,
				 persistence_statement_kind_from_sql(sql.c_str()),
				 finished - started, rc == 0, rc ? mysql_errno(connection) : 0,
				 rc ? mysql_sqlstate(connection) : "00000");
	++result->metrics.query_count;
	return rc ? nullptr : mysql_store_result(connection);
}

bool execute(MYSQL *connection, const char *sql, player_load_result *result)
{
	if (!connection || !sql || !result)
		return false;
	const uint64_t started = persistence_observability_now_usec();
	const int rc = mysql_real_query(connection, sql, strlen(sql));
	const uint64_t finished = persistence_observability_now_usec();
	persistence_query_record(PERSISTENCE_QUERY_SITE,
				 PERSISTENCE_QUERY_CONTEXT_PLAYER_LOAD_WORKER,
				 persistence_statement_kind_from_sql(sql), finished - started,
				 rc == 0, rc ? mysql_errno(connection) : 0,
				 rc ? mysql_sqlstate(connection) : "00000");
	++result->metrics.query_count;
	return rc == 0;
}

bool add_result_budget(MYSQL_RES *rows, MYSQL_ROW row, player_load_result *result)
{
	if (!rows || !row || !result)
		return false;
	const unsigned int columns = mysql_num_fields(rows);
	const unsigned long *lengths = mysql_fetch_lengths(rows);
	if (!lengths)
		return false;
	++result->metrics.row_count;
	for (unsigned int column = 0; column < columns; ++column)
		result->metrics.byte_count += row[column] ? lengths[column] : 0;
	return within_budget(*result);
}

int64_t signed_value(const char *value)
{
	return value ? strtoll(value, nullptr, 10) : 0;
}

uint64_t unsigned_value(const char *value)
{
	return value ? strtoull(value, nullptr, 10) : 0;
}

bool parse_signed(const char *text, int64_t minimum, int64_t maximum, int64_t *value)
{
	if (!text || !*text || !value)
		return false;
	errno = 0;
	char *end = nullptr;
	const long long parsed = strtoll(text, &end, 10);
	if (errno == ERANGE || end == text || *end || parsed < minimum || parsed > maximum)
		return false;
	*value = parsed;
	return true;
}

bool parse_unsigned(const char *text, uint64_t maximum, uint64_t *value)
{
	if (!text || !*text || !value || *text == '-')
		return false;
	errno = 0;
	char *end = nullptr;
	const unsigned long long parsed = strtoull(text, &end, 10);
	if (errno == ERANGE || end == text || *end || parsed > maximum)
		return false;
	*value = parsed;
	return true;
}

std::string escape(MYSQL *connection, const std::string &value)
{
	std::string escaped(value.size() * 2 + 1, '\0');
	const unsigned long length =
		mysql_real_escape_string(connection, escaped.data(), value.data(), value.size());
	escaped.resize(length);
	return escaped;
}

bool load_status(MYSQL *connection, const player_load_request &request, player_load_result *result)
{
	std::ostringstream sql;
	sql << "SELECT pid,COALESCE(account_name,(SELECT ac.account_name FROM account_characters ac "
	       "WHERE ac.pid=player_data.pid AND ac.deleted_at IS NULL LIMIT 1)),";
	bool first = true;
	auto append = [&](const std::string &column)
	{
		if (!first)
			sql << ',';
		first = false;
		sql << column;
	};
	for (const char *column : STATUS_STRINGS)
		append(column);
	for (const status_column &column : STATUS_COLUMNS)
		append(column.timestamp ? "UNIX_TIMESTAMP(" + std::string(column.column) + ")" :
					  column.column);
	append("last_room");
	append("UNIX_TIMESTAMP(last_save)");
	append("save_revision");
	append("wallet_revision");
	append("epic_revision");
	append("frag_revision");
	for (const char *column : CONDITION_COLUMNS)
		append(column);
	for (const char *column : QUEST_COLUMNS)
		append(column);
	if (request.pid > 0)
		sql << " FROM player_data WHERE pid=" << request.pid << " LIMIT 1";
	else
		sql << " FROM player_data WHERE LOWER(name)=LOWER('"
		    << escape(connection, request.player_name) << "') LIMIT 1";
	MYSQL_RES *rows = query(connection, sql.str(), result);
	if (!rows)
		return false;
	MYSQL_ROW row = mysql_fetch_row(rows);
	if (!row)
	{
		mysql_free_result(rows);
		result->outcome = player_load_outcome::not_found;
		return false;
	}
	if (!add_result_budget(rows, row, result))
	{
		mysql_free_result(rows);
		result->outcome = player_load_outcome::limit_exceeded;
		return false;
	}
	int column = 0;
	result->pid = static_cast<int32_t>(signed_value(row[column++]));
	result->account_name = row[column] ? row[column] : "";
	++column;
	if (result->pid <= 0 || result->account_name.empty() ||
	    result->account_name.size() > PLAYER_LOAD_ACCOUNT_MAX ||
	    (request.pid > 0 &&
	     (result->pid != request.pid ||
	      strcasecmp(result->account_name.c_str(), request.account_name.c_str()))))
	{
		mysql_free_result(rows);
		return false;
	}
	for (size_t index = 0; index < STATUS_STRINGS.size(); ++index)
	{
		const size_t length = row[column] ? strlen(row[column]) : 0;
		if (length > PLAYER_SNAPSHOT_MAX_STRING_BYTES)
		{
			mysql_free_result(rows);
			result->outcome = player_load_outcome::limit_exceeded;
			return false;
		}
		result->snapshot.status_strings.push_back(
			{ static_cast<player_status_string_field>(index),
			  row[column] ? row[column] : "" });
		++column;
	}
	for (const status_column &spec : STATUS_COLUMNS)
	{
		player_snapshot_integer value = { spec.field, signed_value(row[column]),
						  unsigned_value(row[column]),
						  spec.unsigned_value };
		result->snapshot.status_integers.push_back(value);
		++column;
	}
	result->snapshot.room_vnum = static_cast<int32_t>(signed_value(row[column++]));
	result->saved_at = signed_value(row[column++]);
	if (!row[column])
	{
		mysql_free_result(rows);
		return false;
	}
	result->snapshot.revision = unsigned_value(row[column++]);
	result->domains.wallet_revision = unsigned_value(row[column++]);
	result->domains.epic_revision = unsigned_value(row[column++]);
	result->domains.frag_revision = unsigned_value(row[column++]);
	for (int32_t &condition : result->snapshot.conditions)
		condition = static_cast<int32_t>(signed_value(row[column++]));
	for (int32_t &quest : result->snapshot.quest_values)
		quest = static_cast<int32_t>(signed_value(row[column++]));
	result->domains.wallet = {
		unsigned_value(row[9 + 32]),
		unsigned_value(row[9 + 33]),
		unsigned_value(row[9 + 34]),
		unsigned_value(row[9 + 35]),
	};
	result->domains.epics = signed_value(row[9 + 37]);
	result->domains.frags = signed_value(row[9 + 52]);
	result->domains.old_frags = signed_value(row[9 + 53]);
	mysql_free_result(rows);
	return true;
}

template <typename Callback> bool load_rows(MYSQL *connection, const std::string &sql,
					    player_load_result *result, Callback callback)
{
	MYSQL_RES *rows = query(connection, sql, result);
	if (!rows)
		return false;
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(rows)))
	{
		if (!add_result_budget(rows, row, result))
		{
			mysql_free_result(rows);
			result->outcome = player_load_outcome::limit_exceeded;
			return false;
		}
		if (!callback(row))
		{
			mysql_free_result(rows);
			return false;
		}
	}
	mysql_free_result(rows);
	return true;
}

bool load_components(MYSQL *connection, const player_load_request &request,
		     player_load_result *result)
{
	(void)request;
	const std::string pid = std::to_string(result->pid);
	auto index_rows = [&](const char *table, const char *columns,
			      std::vector<player_index_value_snapshot> *target, bool auxiliary)
	{
		return load_rows(connection,
				 "SELECT " + std::string(columns) + " FROM " + table +
					 " WHERE pid=" + pid + " ORDER BY 1",
				 result,
				 [&](MYSQL_ROW row)
				 {
					 target->push_back(
						 { static_cast<int32_t>(signed_value(row[0])),
						   signed_value(row[1]),
						   auxiliary ? unsigned_value(row[2]) : 0 });
					 return true;
				 });
	};
	if (!index_rows("player_languages", "tongue_id,proficiency", &result->snapshot.languages,
			false) ||
	    !index_rows("player_intros", "intro_index,intro_pid,UNIX_TIMESTAMP(intro_time)",
			&result->snapshot.introductions, true) ||
	    !index_rows("player_timers", "timer_id,UNIX_TIMESTAMP(timer_value)",
			&result->snapshot.timers, false) ||
	    !index_rows("player_undead_slots", "circle,slots", &result->snapshot.undead_slots,
			false) ||
	    !index_rows("player_forged_items", "forge_index,item_vnum",
			&result->snapshot.forged_items, false))
		return false;
	if (!load_rows(connection,
		       "SELECT cmd_num FROM player_granted_cmds WHERE pid=" + pid + " ORDER BY id",
		       result,
		       [&](MYSQL_ROW row)
		       {
			       result->snapshot.granted_commands.push_back(
				       static_cast<int32_t>(signed_value(row[0])));
			       return true;
		       }) ||
	    !load_rows(connection,
		       "SELECT skill_id,learned,taught FROM player_skills WHERE pid=" + pid +
			       " ORDER BY skill_id",
		       result,
		       [&](MYSQL_ROW row)
		       {
			       result->snapshot.skills.push_back(
				       { static_cast<int32_t>(signed_value(row[0])),
					 static_cast<uint8_t>(unsigned_value(row[1])),
					 static_cast<uint8_t>(unsigned_value(row[2])) });
			       return true;
		       }) ||
	    !load_rows(connection,
		       "SELECT type,duration,flags,modifier,location,level,bitvector1,bitvector2,"
		       "bitvector3,bitvector4,bitvector5,custom_msg_char,custom_msg_room FROM "
		       "player_affects WHERE pid=" +
			       pid + " ORDER BY id",
		       result,
		       [&](MYSQL_ROW row)
		       {
			       player_affect_snapshot affect = {};
			       affect.type = static_cast<int16_t>(signed_value(row[0]));
			       affect.duration = static_cast<int32_t>(signed_value(row[1]));
			       affect.flags = static_cast<uint32_t>(unsigned_value(row[2]));
			       affect.modifier = static_cast<int32_t>(signed_value(row[3]));
			       affect.location = static_cast<uint8_t>(unsigned_value(row[4]));
			       affect.level = static_cast<uint16_t>(unsigned_value(row[5]));
			       for (size_t index = 0; index < affect.bitvectors.size(); ++index)
				       affect.bitvectors[index] = unsigned_value(row[6 + index]);
			       affect.wear_off_character = row[11] ? row[11] : "";
			       affect.wear_off_room = row[12] ? row[12] : "";
			       if (affect.wear_off_character.size() >
					   PLAYER_SNAPSHOT_MAX_STRING_BYTES ||
				   affect.wear_off_room.size() > PLAYER_SNAPSHOT_MAX_STRING_BYTES)
				       return false;
			       result->snapshot.affects.push_back(std::move(affect));
			       return true;
		       }) ||
	    !load_rows(connection,
		       "SELECT mob_vnum,times_researched,UNIX_TIMESTAMP(last_researched),"
		       "UNIX_TIMESTAMP(last_shapechanged) FROM player_shapechanges WHERE pid=" +
			       pid + " ORDER BY id",
		       result,
		       [&](MYSQL_ROW row)
		       {
			       result->snapshot.shapes.push_back(
				       { static_cast<int32_t>(signed_value(row[0])),
					 static_cast<int32_t>(signed_value(row[1])),
					 signed_value(row[2]), signed_value(row[3]) });
			       return true;
		       }))
		return false;
	return true;
}

bool load_bank(MYSQL *connection, const player_load_request &request, player_load_result *result)
{
	(void)request;
	const std::string sql =
		"SELECT bank_copper,bank_silver,bank_gold,bank_platinum,bank_revision FROM "
		"account_banks WHERE account_name='" +
		escape(connection, result->account_name) + "' AND racewar=" +
		std::to_string(
			[&]
			{
				for (const player_snapshot_integer &entry :
				     result->snapshot.status_integers)
					if (entry.field == player_status_field::racewar)
						return static_cast<int>(entry.signed_value);
				return 0;
			}());
	MYSQL_RES *rows = query(connection, sql, result);
	if (!rows)
		return false;
	MYSQL_ROW row = mysql_fetch_row(rows);
	if (!row)
	{
		mysql_free_result(rows);
		return true;
	}
	if (!add_result_budget(rows, row, result))
	{
		mysql_free_result(rows);
		return false;
	}
	for (size_t index = 0; index < result->domains.bank.size(); ++index)
		result->domains.bank[index] = unsigned_value(row[index]);
	result->domains.bank_revision = unsigned_value(row[4]);
	mysql_free_result(rows);
	return true;
}

bool load_items(MYSQL *connection, player_load_result *result)
{
	const std::string pid = std::to_string(result->pid);
	std::unordered_map<uint64_t, size_t> item_by_database_id;
	std::unordered_map<uint64_t, size_t> item_by_uid;
	try
	{
		item_by_database_id.reserve(PLAYER_LOAD_ITEM_MAX);
		item_by_uid.reserve(PLAYER_LOAD_ITEM_MAX);
	}
	catch (const std::bad_alloc &)
	{
		result->outcome = player_load_outcome::retryable_failure;
		return false;
	}

	const std::string item_sql =
		"SELECT pi.id,pi.vnum,pi.equip_slot,pi.container_id,pi.quantity,pi.weight,"
		"pi.cost,pi.timer,pi.extra_flags,pi.wear_flags,pi.item_type,pi.value0,"
		"pi.value1,pi.value2,pi.value3,pi.value4,pi.value5,pi.value6,pi.value7,"
		"pi.name,pi.short_descr,pi.description,pi.action_descr,pi.bitvector1,"
		"pi.bitvector2,pi.bitvector3,pi.bitvector4,pi.bitvector5,pi.item_material,"
		"pi.obj_uid,pi.item_condition,own.item_uid,own.root_item_uid,"
		"own.parent_item_uid,own.owner_type,own.owner_id,own.owner_context_id,"
		"own.item_revision,own.vnum,own.state,owner_revision.revision FROM player_items pi "
		"LEFT JOIN item_current_owner own ON own.item_uid=pi.obj_uid LEFT JOIN "
		"item_owner_revision owner_revision ON owner_revision.owner_type=own.owner_type "
		"AND owner_revision.owner_id=own.owner_id AND "
		"owner_revision.owner_context_id=own.owner_context_id WHERE pi.pid=" +
		pid + " ORDER BY pi.id";
	if (!load_rows(
		    connection, item_sql, result,
		    [&](MYSQL_ROW row)
		    {
			    if (result->snapshot.items.size() >= PLAYER_LOAD_ITEM_MAX)
			    {
				    result->outcome = player_load_outcome::limit_exceeded;
				    return false;
			    }
			    int64_t signed_field = 0;
			    uint64_t unsigned_field = 0;
			    player_item_snapshot item = {};
			    player_load_item_identity identity = {};
			    if (!parse_unsigned(row[0], UINT64_MAX, &identity.database_id) ||
				!identity.database_id ||
				!parse_signed(row[1], INT32_MIN, INT32_MAX, &signed_field))
				    return false;
			    item.vnum = static_cast<int32_t>(signed_field);
			    if (!parse_signed(row[2], INT16_MIN, INT16_MAX, &signed_field))
				    return false;
			    item.equipment_slot = static_cast<int16_t>(signed_field);
			    if (row[3] &&
				!parse_unsigned(row[3], UINT64_MAX, &identity.serialized_parent_id))
				    return false;
			    if (!parse_unsigned(row[4], UINT32_MAX, &unsigned_field) ||
				unsigned_field != 1)
				    return false;
			    identity.quantity = static_cast<uint32_t>(unsigned_field);
			    if (!parse_signed(row[5], INT32_MIN, INT32_MAX, &signed_field))
				    return false;
			    item.weight = static_cast<int32_t>(signed_field);
			    if (!parse_signed(row[6], INT32_MIN, INT32_MAX, &signed_field))
				    return false;
			    item.cost = static_cast<int32_t>(signed_field);
			    if (!parse_signed(row[7], INT64_MIN, INT64_MAX, &item.timers[0]) ||
				!parse_unsigned(row[8], UINT32_MAX, &unsigned_field))
				    return false;
			    item.extra_flags = static_cast<uint32_t>(unsigned_field);
			    if (row[9])
			    {
				    if (!parse_unsigned(row[9], UINT32_MAX, &unsigned_field))
					    return false;
				    item.wear_flags = static_cast<uint32_t>(unsigned_field);
				    identity.override_mask |= PLAYER_LOAD_ITEM_OVERRIDE_WEAR_FLAGS;
			    }
			    if (row[10])
			    {
				    if (!parse_signed(row[10], INT8_MIN, INT8_MAX, &signed_field))
					    return false;
				    item.type = static_cast<int8_t>(signed_field);
				    identity.override_mask |= PLAYER_LOAD_ITEM_OVERRIDE_TYPE;
			    }
			    for (size_t index = 0; index < item.values.size(); ++index)
			    {
				    if (!parse_signed(row[11 + index], INT32_MIN, INT32_MAX,
						      &signed_field))
					    return false;
				    item.values[index] = static_cast<int32_t>(signed_field);
			    }
			    constexpr std::array<uint8_t, 4> string_masks = { 1, 4, 2, 8 };
			    std::array<std::string *, 4> strings = {
				    &item.name,
				    &item.short_description,
				    &item.description,
				    &item.action_description,
			    };
			    for (size_t index = 0; index < strings.size(); ++index)
				    if (row[19 + index])
				    {
					    if (strlen(row[19 + index]) >
						PLAYER_SNAPSHOT_MAX_STRING_BYTES)
					    {
						    result->outcome =
							    player_load_outcome::limit_exceeded;
						    return false;
					    }
					    *strings[index] = row[19 + index];
					    item.string_mask |= string_masks[index];
				    }
			    constexpr std::array<uint16_t, 5> bitvector_masks = {
				    PLAYER_LOAD_ITEM_OVERRIDE_BITVECTOR1,
				    PLAYER_LOAD_ITEM_OVERRIDE_BITVECTOR2,
				    PLAYER_LOAD_ITEM_OVERRIDE_BITVECTOR3,
				    PLAYER_LOAD_ITEM_OVERRIDE_BITVECTOR4,
				    PLAYER_LOAD_ITEM_OVERRIDE_BITVECTOR5,
			    };
			    for (size_t index = 0; index < item.bitvectors.size(); ++index)
				    if (row[23 + index])
				    {
					    if (!parse_unsigned(row[23 + index], UINT64_MAX,
								&item.bitvectors[index]))
						    return false;
					    identity.override_mask |= bitvector_masks[index];
				    }
			    if (row[28])
			    {
				    if (!parse_signed(row[28], INT8_MIN, INT8_MAX, &signed_field))
					    return false;
				    item.material = static_cast<int8_t>(signed_field);
				    identity.override_mask |= PLAYER_LOAD_ITEM_OVERRIDE_MATERIAL;
			    }
			    if (!parse_unsigned(row[29], UINT64_MAX, &item.object_uid) ||
				!item.object_uid ||
				!parse_signed(row[30], INT16_MIN, INT16_MAX, &signed_field))
				    return false;
			    item.condition = static_cast<int16_t>(signed_field);
			    if (!parse_unsigned(row[31], UINT64_MAX, &identity.item_uid) ||
				identity.item_uid != item.object_uid ||
				!parse_unsigned(row[32], UINT64_MAX, &identity.root_item_uid) ||
				!identity.root_item_uid)
				    return false;
			    if (row[33] &&
				!parse_unsigned(row[33], UINT64_MAX, &identity.parent_item_uid))
				    return false;
			    if (!parse_unsigned(row[34], UINT8_MAX, &unsigned_field))
				    return false;
			    identity.owner.type = static_cast<item_owner_type>(unsigned_field);
			    if (!parse_unsigned(row[35], UINT64_MAX, &identity.owner.id) ||
				!parse_unsigned(row[36], UINT64_MAX, &identity.owner.context_id) ||
				!parse_unsigned(row[37], UINT64_MAX, &identity.item_revision) ||
				!parse_signed(row[38], INT32_MIN, INT32_MAX, &signed_field) ||
				signed_field != item.vnum ||
				!parse_unsigned(row[39], UINT8_MAX, &unsigned_field))
				    return false;
			    identity.state = static_cast<item_custody_state>(unsigned_field);
			    if (!parse_unsigned(row[40], UINT64_MAX, &identity.owner_revision) ||
				identity.owner.type != item_owner_type::player ||
				identity.owner.id != static_cast<uint64_t>(result->pid) ||
				identity.owner.context_id != 0 ||
				identity.state != item_custody_state::active ||
				identity.override_mask & ~PLAYER_LOAD_ITEM_OVERRIDE_ALL ||
				item_by_database_id.find(identity.database_id) !=
					item_by_database_id.end() ||
				item_by_uid.find(identity.item_uid) != item_by_uid.end())
				    return false;
			    try
			    {
				    const size_t index = result->snapshot.items.size();
				    item_by_database_id.emplace(identity.database_id, index);
				    item_by_uid.emplace(identity.item_uid, index);
				    result->snapshot.items.push_back(std::move(item));
				    result->item_identities.push_back(identity);
			    }
			    catch (const std::bad_alloc &)
			    {
				    result->outcome = player_load_outcome::retryable_failure;
				    return false;
			    }
			    return true;
		    }))
		return false;

	for (size_t index = 0; index < result->item_identities.size(); ++index)
	{
		player_load_item_identity &identity = result->item_identities[index];
		player_item_snapshot &item = result->snapshot.items[index];
		if (!identity.serialized_parent_id && !identity.parent_item_uid)
		{
			item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
			continue;
		}
		const auto database_parent =
			item_by_database_id.find(identity.serialized_parent_id);
		const auto uid_parent = item_by_uid.find(identity.parent_item_uid);
		if (!identity.serialized_parent_id || !identity.parent_item_uid ||
		    database_parent == item_by_database_id.end() ||
		    uid_parent == item_by_uid.end() ||
		    database_parent->second != uid_parent->second ||
		    database_parent->second == index ||
		    database_parent->second > static_cast<size_t>(INT32_MAX))
			return false;
		item.parent_index = static_cast<int32_t>(database_parent->second);
	}

	const std::string ownership_summary_sql =
		"SELECT COALESCE(owner_revision.revision,0),COUNT(own.item_uid),"
		"COALESCE(SUM(CASE WHEN own.item_uid IS NOT NULL AND pi.id IS NULL THEN 1 ELSE 0 "
		"END),0),owner_revision.owner_id IS NOT NULL FROM (SELECT 1) singleton LEFT JOIN "
		"item_owner_revision owner_revision ON owner_revision.owner_type=" +
		std::to_string(static_cast<unsigned int>(item_owner_type::player)) +
		" AND owner_revision.owner_id=" + pid +
		" AND owner_revision.owner_context_id=0 LEFT JOIN item_current_owner own ON "
		"own.owner_type=" +
		std::to_string(static_cast<unsigned int>(item_owner_type::player)) +
		" AND own.owner_id=" + pid + " AND own.owner_context_id=0 AND own.state=" +
		std::to_string(static_cast<unsigned int>(item_custody_state::active)) +
		" LEFT JOIN player_items pi ON pi.obj_uid=own.item_uid AND pi.pid=" + pid +
		" GROUP BY owner_revision.revision,owner_revision.owner_id";
	size_t ownership_summary_rows = 0;
	if (!load_rows(connection, ownership_summary_sql, result,
		       [&](MYSQL_ROW row)
		       {
			       uint64_t owned_count = 0;
			       uint64_t missing_count = 0;
			       uint64_t owner_revision_present = 0;
			       ++ownership_summary_rows;
			       return ownership_summary_rows == 1 &&
				      parse_unsigned(row[0], UINT64_MAX,
						     &result->item_owner_revision) &&
				      parse_unsigned(row[1], PLAYER_LOAD_ITEM_MAX, &owned_count) &&
				      parse_unsigned(row[2], PLAYER_LOAD_ITEM_MAX,
						     &missing_count) &&
				      parse_unsigned(row[3], 1, &owner_revision_present) &&
				      missing_count == 0 &&
				      (owner_revision_present || owned_count == 0) &&
				      owned_count == result->snapshot.items.size();
		       }) ||
	    ownership_summary_rows != 1)
		return false;
	for (const player_load_item_identity &identity : result->item_identities)
		if (identity.owner_revision != result->item_owner_revision)
			return false;

	std::vector<std::unordered_set<uint64_t>> affects;
	try
	{
		affects.resize(result->snapshot.items.size());
	}
	catch (const std::bad_alloc &)
	{
		result->outcome = player_load_outcome::retryable_failure;
		return false;
	}
	const std::string metadata_sql =
		"SELECT 0 AS row_kind,ia.id AS metadata_id,ia.item_id,ia.location,"
		"ia.modifier,NULL AS keyword,NULL AS description FROM player_item_affects ia "
		"JOIN player_items pi ON pi.id=ia.item_id WHERE pi.pid=" +
		pid +
		" UNION ALL SELECT 1,ed.id,ed.item_id,0,0,ed.keyword,ed.description FROM "
		"player_item_extra_descr ed JOIN player_items pi ON pi.id=ed.item_id WHERE "
		"pi.pid=" +
		pid + " ORDER BY row_kind,metadata_id,item_id";
	if (!load_rows(
		    connection, metadata_sql, result,
		    [&](MYSQL_ROW row)
		    {
			    uint64_t row_kind = 0;
			    uint64_t database_id = 0;
			    if (!parse_unsigned(row[0], 1, &row_kind) ||
				!parse_unsigned(row[2], UINT64_MAX, &database_id))
				    return false;
			    const auto found = item_by_database_id.find(database_id);
			    if (found == item_by_database_id.end())
				    return false;
			    const size_t index = found->second;
			    player_item_snapshot &item = result->snapshot.items[index];
			    player_load_item_identity &identity = result->item_identities[index];
			    if (row_kind == 0)
			    {
				    int64_t location = 0;
				    int64_t modifier = 0;
				    if (!parse_signed(row[3], 0, UINT8_MAX, &location) ||
					!parse_signed(row[4], INT8_MIN, INT8_MAX, &modifier))
					    return false;
				    const uint64_t key =
					    (static_cast<uint64_t>(static_cast<uint16_t>(location))
					     << 32) |
					    static_cast<uint8_t>(modifier);
				    try
				    {
					    if (!affects[index].insert(key).second)
						    return true;
				    }
				    catch (const std::bad_alloc &)
				    {
					    result->outcome =
						    player_load_outcome::retryable_failure;
					    return false;
				    }
				    if (affects[index].size() > PLAYER_LOAD_ITEM_AFFECT_MAX)
				    {
					    result->outcome = player_load_outcome::limit_exceeded;
					    return false;
				    }
				    const size_t affect_index = affects[index].size() - 1;
				    item.affects[affect_index] = {
					    static_cast<int16_t>(location),
					    static_cast<int16_t>(modifier),
				    };
				    identity.override_mask |= PLAYER_LOAD_ITEM_OVERRIDE_AFFECTS;
				    return true;
			    }
			    if (!row[5] || strlen(row[5]) > PLAYER_SNAPSHOT_MAX_STRING_BYTES ||
				(row[6] && strlen(row[6]) > PLAYER_SNAPSHOT_MAX_STRING_BYTES) ||
				item.extra_descriptions.size() >= PLAYER_LOAD_ITEM_DESCRIPTION_MAX)
			    {
				    result->outcome = player_load_outcome::limit_exceeded;
				    return false;
			    }
			    try
			    {
				    player_item_extra_description_snapshot description = {};
				    description.keyword = row[5];
				    description.description = row[6] ? row[6] : "";
				    description.spellbook = description.keyword == "SPELLBOOK";
				    item.extra_descriptions.push_back(std::move(description));
			    }
			    catch (const std::bad_alloc &)
			    {
				    result->outcome = player_load_outcome::retryable_failure;
				    return false;
			    }
			    return true;
		    }))
		return false;
	return result->snapshot.items.size() == result->item_identities.size();
}
} // namespace

bool player_load_request_valid(const player_load_request &request, uint64_t now_usec)
{
	const bool pid_identity = request.pid > 0 && !request.account_name.empty() &&
				  request.account_name.size() <= PLAYER_LOAD_ACCOUNT_MAX;
	const bool name_identity = request.pid == 0 && !request.player_name.empty() &&
				   request.player_name.size() <= PLAYER_LOAD_NAME_MAX;
	return request.schema_version == PLAYER_LOAD_SCHEMA_VERSION && request.request_id > 0 &&
	       (pid_identity || name_identity) && request.deadline_usec > now_usec &&
	       request.deadline_usec - now_usec <= PLAYER_LOAD_TIMEOUT_USEC;
}

player_load_result player_load_repository_execute(MYSQL *connection,
						  const player_load_request &request)
{
	player_load_result result = {};
	result.request_id = request.request_id;
	result.pid = request.pid;
	const uint64_t started = persistence_observability_now_usec();
	if (!connection || !player_load_request_valid(request, started))
	{
		result.outcome = request.deadline_usec <= started ?
					 player_load_outcome::timed_out :
					 player_load_outcome::component_failure;
		return result;
	}
	if (!execute(connection, "SET TRANSACTION ISOLATION LEVEL REPEATABLE READ", &result) ||
	    !execute(connection, "START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY", &result))
	{
		result.error_code = mysql_errno(connection);
		result.outcome = failure_outcome(result.error_code);
		return result;
	}
	result.snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
	result.snapshot.components = request.include_items ? PLAYER_LOAD_SESSION02_COMPONENTS :
							     PLAYER_LOAD_SESSION01_COMPONENTS;
	bool ok = load_status(connection, request, &result) &&
		  load_components(connection, request, &result) &&
		  (!request.include_items || load_items(connection, &result)) &&
		  load_bank(connection, request, &result) && before_deadline(request) &&
		  within_budget(result);
	result.snapshot.pid = result.pid;
	if (ok && execute(connection, "COMMIT", &result) && before_deadline(request))
		result.outcome = player_load_outcome::applied;
	else
	{
		if (result.outcome == player_load_outcome::component_failure &&
		    !before_deadline(request))
			result.outcome = player_load_outcome::timed_out;
		if (result.outcome == player_load_outcome::component_failure &&
		    mysql_errno(connection))
		{
			result.error_code = mysql_errno(connection);
			result.outcome = failure_outcome(result.error_code);
		}
		execute(connection, "ROLLBACK", &result);
	}
	result.metrics.transaction_usec = persistence_observability_now_usec() - started;
	return result;
}

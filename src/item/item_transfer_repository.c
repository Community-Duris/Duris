#include "item/item_transfer_repository.h"
#include "core/defines.h"
#include "player/player_snapshot_codec.h"
#include "core/structs.h"
#include "persistence/critical_command_repository.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <limits>
#include <mysql.h>
#include <new>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace
{
using mysql_null_indicator = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;

uint64_t wall_now_usec()
{
	return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
					     std::chrono::system_clock::now().time_since_epoch())
					     .count());
}

struct current_item
{
	uint64_t item_uid;
	uint64_t root_item_uid;
	uint64_t parent_item_uid;
	uint8_t owner_type;
	uint64_t owner_id;
	uint64_t owner_context_id;
	uint64_t item_revision;
	int32_t vnum;
	uint8_t state;
};

bool run_sql(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()) == 0)
		return true;
	errno = static_cast<int>(mysql_errno(connection));
	return false;
}

bool one_row(MYSQL *connection, const std::string &sql, std::vector<uint64_t> *values)
{
	if (!run_sql(connection, sql))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
	{
		errno = static_cast<int>(mysql_errno(connection));
		return false;
	}
	MYSQL_ROW row = mysql_fetch_row(rows);
	const bool found = row && mysql_num_rows(rows) == 1;
	if (found)
	{
		values->clear();
		for (unsigned int column = 0; column < mysql_num_fields(rows); ++column)
		{
			if (!row[column])
			{
				mysql_free_result(rows);
				errno = EILSEQ;
				return false;
			}
			char *end = nullptr;
			const unsigned long long value = std::strtoull(row[column], &end, 10);
			if (!end || *end)
			{
				mysql_free_result(rows);
				errno = EILSEQ;
				return false;
			}
			values->push_back(value);
		}
	}
	mysql_free_result(rows);
	if (!found)
		errno = EILSEQ;
	return found;
}

bool move_pet_physical_items(MYSQL *connection, const item_transfer_payload &payload)
{
	const bool give = payload.reason == item_transfer_reason::pet_give;
	const bool take = payload.reason == item_transfer_reason::pet_return;
	if (!give && !take)
		return true;
	const uint64_t player_pid = give ? payload.from_owner.id : payload.to_owner.id;
	const uint64_t pet_uid = give ? payload.to_owner.id : payload.from_owner.id;
	std::vector<uint64_t> pet_row;
	if (!one_row(connection,
		     "SELECT id,hold_reason FROM player_pets WHERE owner_pid=" +
			     std::to_string(player_pid) +
			     " AND pet_uid=" + std::to_string(pet_uid) + " FOR UPDATE",
		     &pet_row) ||
	    pet_row[1] != 0 || payload.target_parent_item_uid || !payload.selected_item_uid)
	{
		errno = EILSEQ;
		return false;
	}
	const std::string source = give ? "player_items" : "player_pet_items";
	const std::string target = give ? "player_pet_items" : "player_items";
	const std::string source_owner = give ? "pid" : "pet_id";
	const std::string target_owner = give ? "pet_id" : "pid";
	const uint64_t source_owner_id = give ? player_pid : pet_row[0];
	const uint64_t target_owner_id = give ? pet_row[0] : player_pid;
	struct physical_row
	{
		uint64_t id;
		uint64_t parent_id;
		int32_t vnum;
	};
	std::unordered_map<uint64_t, physical_row> source_rows;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto &item = payload.items[index];
		std::vector<uint64_t> row;
		if (!one_row(connection,
			     "SELECT id,COALESCE(container_id,0),vnum FROM " + source + " WHERE " +
				     source_owner + "=" + std::to_string(source_owner_id) +
				     " AND obj_uid=" + std::to_string(item.item_uid) +
				     " FOR UPDATE",
			     &row) ||
		    row[2] != static_cast<uint64_t>(item.vnum) ||
		    !source_rows.emplace(item.item_uid, physical_row{ row[0], row[1], item.vnum })
			     .second)
		{
			errno = EILSEQ;
			return false;
		}
		if (!run_sql(connection, "SELECT id FROM " + target + " WHERE obj_uid=" +
						 std::to_string(item.item_uid) + " FOR UPDATE"))
			return false;
		MYSQL_RES *duplicate = mysql_store_result(connection);
		if (!duplicate)
			return false;
		const bool exists = mysql_num_rows(duplicate) != 0;
		mysql_free_result(duplicate);
		if (exists)
		{
			errno = EEXIST;
			return false;
		}
	}
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto &item = payload.items[index];
		const auto parent = source_rows.find(item.parent_item_uid);
		if (item.parent_item_uid && parent == source_rows.end())
		{
			errno = EILSEQ;
			return false;
		}
		const uint64_t expected_parent = item.parent_item_uid ? parent->second.id : 0;
		if (source_rows.at(item.item_uid).parent_id != expected_parent)
		{
			errno = EILSEQ;
			return false;
		}
		if (!run_sql(connection, "SELECT obj_uid FROM " + source + " WHERE container_id=" +
						 std::to_string(source_rows.at(item.item_uid).id) +
						 " FOR UPDATE"))
			return false;
		MYSQL_RES *children = mysql_store_result(connection);
		if (!children)
			return false;
		size_t found_children = 0;
		bool valid_children = true;
		while (MYSQL_ROW row = mysql_fetch_row(children))
		{
			++found_children;
			const uint64_t child_uid = row[0] ? std::strtoull(row[0], nullptr, 10) : 0;
			const auto child = source_rows.find(child_uid);
			if (child == source_rows.end() ||
			    child->second.parent_id != source_rows.at(item.item_uid).id)
				valid_children = false;
		}
		mysql_free_result(children);
		size_t expected_children = 0;
		for (size_t child = 0; child < payload.item_count; ++child)
			expected_children += payload.items[child].parent_item_uid == item.item_uid;
		if (!valid_children || found_children != expected_children)
		{
			errno = EILSEQ;
			return false;
		}
	}
	static const std::string fields =
		"vnum,weight,cost,timer,extra_flags,wear_flags,item_type,value0,value1,"
		"value2,value3,value4,value5,value6,value7,name,short_descr,description,"
		"action_descr,bitvector1,bitvector2,bitvector3,bitvector4,bitvector5,"
		"obj_uid,item_condition,item_material";
	std::unordered_map<uint64_t, uint64_t> copied;
	while (copied.size() < payload.item_count)
	{
		bool advanced = false;
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const auto &item = payload.items[index];
			if (copied.contains(item.item_uid))
				continue;
			uint64_t parent_id = 0;
			if (item.parent_item_uid)
			{
				const auto parent = copied.find(item.parent_item_uid);
				if (parent == copied.end())
					continue;
				parent_id = parent->second;
			}
			const std::string parent = parent_id ? std::to_string(parent_id) : "NULL";
			if (!run_sql(connection,
				     "INSERT INTO " + target + "(" + target_owner +
					     ",equip_slot,container_id," + fields + ") SELECT " +
					     std::to_string(target_owner_id) + ",0," + parent +
					     "," + fields + " FROM " + source + " WHERE id=" +
					     std::to_string(source_rows.at(item.item_uid).id)) ||
			    mysql_affected_rows(connection) != 1)
				return false;
			const uint64_t new_id = mysql_insert_id(connection);
			if (!new_id)
				return false;
			const std::string old_id = std::to_string(source_rows.at(item.item_uid).id);
			for (const auto &metadata :
			     { std::pair{ "affects", "location,modifier" },
			       std::pair{ "extra_descr", "keyword,description" } })
			{
				const std::string source_meta =
					(give ? "player_item_" : "player_pet_item_") +
					std::string(metadata.first);
				const std::string target_meta =
					(give ? "player_pet_item_" : "player_item_") +
					std::string(metadata.first);
				if (!run_sql(connection,
					     "INSERT INTO " + target_meta + "(item_id," +
						     metadata.second + ") SELECT " +
						     std::to_string(new_id) + "," +
						     metadata.second + " FROM " + source_meta +
						     " WHERE item_id=" + old_id))
					return false;
			}
			copied.emplace(item.item_uid, new_id);
			advanced = true;
		}
		if (!advanced)
		{
			errno = EILSEQ;
			return false;
		}
	}
	return run_sql(connection,
		       "DELETE FROM " + source + " WHERE id=" +
			       std::to_string(source_rows.at(payload.selected_item_uid).id)) &&
	       mysql_affected_rows(connection) == 1;
}

std::string quote(MYSQL *connection, const std::string &value)
{
	std::string escaped(value.size() * 2 + 1, '\0');
	const unsigned long length = mysql_real_escape_string(
		connection, escaped.data(), value.data(), static_cast<unsigned long>(value.size()));
	escaped.resize(length);
	return "'" + escaped + "'";
}

bool canonicalize_extra_description(const player_item_extra_description_snapshot &description,
				    std::string *keyword, std::string *text)
{
	if (!keyword || !text || description.spellbook != (description.keyword == "SPELLBOOK") ||
	    (description.spellbook && !description.description.empty() &&
	     !description.spell_ids.empty()))
	{
		errno = EINVAL;
		return false;
	}
	*keyword = description.keyword;
	*text = description.description;
	if (!description.spellbook)
	{
		if (!description.spell_ids.empty())
		{
			errno = EINVAL;
			return false;
		}
		return true;
	}
	*keyword = "SPELLBOOK";
	if (!description.description.empty())
	{
		const char *cursor = description.description.c_str();
		auto whitespace = [&]()
		{
			while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' ||
			       *cursor == '\n')
				++cursor;
		};
		std::array<bool, MAX_SKILLS> seen = {};
		whitespace();
		if (*cursor++ != '[')
		{
			errno = EINVAL;
			return false;
		}
		whitespace();
		if (*cursor == ']')
		{
			++cursor;
			whitespace();
			if (*cursor)
			{
				errno = EINVAL;
				return false;
			}
			return true;
		}
		while (true)
		{
			whitespace();
			if (*cursor < '0' || *cursor > '9')
			{
				errno = EINVAL;
				return false;
			}
			const char *start = cursor;
			unsigned int spell = 0;
			while (*cursor >= '0' && *cursor <= '9')
			{
				const unsigned int digit = static_cast<unsigned int>(*cursor - '0');
				if (spell >
				    static_cast<unsigned int>((MAX_SKILLS - 1 - digit) / 10))
				{
					errno = EINVAL;
					return false;
				}
				spell = spell * 10 + digit;
				++cursor;
			}
			if ((*start == '0' && cursor != start + 1) || spell >= MAX_SKILLS ||
			    seen[spell])
			{
				errno = EINVAL;
				return false;
			}
			seen[spell] = true;
			whitespace();
			if (*cursor == ']')
			{
				++cursor;
				whitespace();
				if (*cursor)
				{
					errno = EINVAL;
					return false;
				}
				return true;
			}
			if (*cursor++ != ',')
			{
				errno = EINVAL;
				return false;
			}
		}
	}
	std::array<bool, MAX_SKILLS> seen = {};
	std::ostringstream encoded;
	encoded << '[';
	for (size_t index = 0; index < description.spell_ids.size(); ++index)
	{
		const int32_t spell = description.spell_ids[index];
		if (spell < 0 || spell >= MAX_SKILLS || seen[spell])
		{
			errno = EINVAL;
			return false;
		}
		seen[spell] = true;
		encoded << (index ? "," : "") << spell;
	}
	encoded << ']';
	*text = encoded.str();
	return true;
}

bool direct_player_projection_reason(item_transfer_reason reason)
{
	// These paths own their physical-row copy and call the item repository inside
	// the same enclosing transaction. Publishing here would create a duplicate.
	return reason != item_transfer_reason::corpse_loot &&
	       reason != item_transfer_reason::corpse_raise_pet &&
	       reason != item_transfer_reason::pet_return;
}

bool materialize_direct_player_items(MYSQL *connection, const item_transfer_payload &payload)
{
	if (payload.to_owner.type != item_owner_type::player ||
	    !direct_player_projection_reason(payload.reason))
		return true;
	// Old journal records did not carry a payload. They remain replayable, but all
	// live movement submissions do carry one and therefore publish atomically.
	if (!payload.item_blob_size)
		return true;

	std::vector<player_item_snapshot> snapshots;
	if (player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
					     &snapshots) != player_snapshot_codec_result::ok ||
	    snapshots.size() != payload.item_count)
	{
		errno = EBADMSG;
		return false;
	}
	std::unordered_map<uint64_t, size_t> snapshot_by_uid;
	std::unordered_map<uint64_t, uint64_t> row_by_uid;
	try
	{
		snapshot_by_uid.reserve(snapshots.size());
		row_by_uid.reserve(snapshots.size());
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return false;
	}
	for (size_t index = 0; index < snapshots.size(); ++index)
	{
		const player_item_snapshot &snapshot = snapshots[index];
		if (!snapshot.object_uid || snapshot.parent_index >= static_cast<int32_t>(index) ||
		    snapshot.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
		    !snapshot_by_uid.emplace(snapshot.object_uid, index).second)
		{
			errno = EBADMSG;
			return false;
		}
	}
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const item_transfer_entry &entry = payload.items[index];
		const auto found = snapshot_by_uid.find(entry.item_uid);
		if (found == snapshot_by_uid.end() || snapshots[found->second].vnum != entry.vnum)
		{
			errno = EBADMSG;
			return false;
		}
	}

	uint64_t external_parent_id = 0;
	if (payload.target_parent_item_uid)
	{
		std::vector<uint64_t> row;
		if (!one_row(connection,
			     "SELECT id FROM player_items WHERE pid=" +
				     std::to_string(payload.to_owner.id) + " AND obj_uid=" +
				     std::to_string(payload.target_parent_item_uid) + " FOR UPDATE",
			     &row))
		{
			errno = ESTALE;
			return false;
		}
		external_parent_id = row[0];
	}

	// Lock and detach every selected physical row before changing owners or
	// parentage. A missing row is repaired from the captured runtime snapshot.
	for (const player_item_snapshot &snapshot : snapshots)
	{
		const std::string select = "SELECT id FROM player_items WHERE obj_uid=" +
					   std::to_string(snapshot.object_uid) + " FOR UPDATE";
		if (!run_sql(connection, select))
			return false;
		MYSQL_RES *rows = mysql_store_result(connection);
		if (!rows)
			return false;
		const my_ulonglong count = mysql_num_rows(rows);
		MYSQL_ROW row = mysql_fetch_row(rows);
		uint64_t item_id = 0;
		if (count == 1 && row && row[0])
			item_id = std::strtoull(row[0], nullptr, 10);
		mysql_free_result(rows);
		if (count > 1 || (count == 1 && !item_id))
		{
			errno = EILSEQ;
			return false;
		}
		if (item_id &&
		    (!run_sql(connection, "UPDATE player_items SET container_id=NULL WHERE id=" +
						  std::to_string(item_id)) ||
		     mysql_affected_rows(connection) > 1))
			return false;
		row_by_uid.emplace(snapshot.object_uid, item_id);
	}

	for (const player_item_snapshot &snapshot : snapshots)
	{
		uint64_t &item_id = row_by_uid.at(snapshot.object_uid);
		std::ostringstream values;
		values << "pid=" << payload.to_owner.id << ",vnum=" << snapshot.vnum
		       << ",equip_slot=" << snapshot.equipment_slot
		       << ",quantity=1,weight=" << snapshot.weight << ",cost=" << snapshot.cost
		       << ",timer=" << snapshot.timers[0] << ",extra_flags=" << snapshot.extra_flags
		       << ",wear_flags=" << snapshot.wear_flags
		       << ",item_type=" << static_cast<unsigned int>(snapshot.type);
		for (size_t value = 0; value < snapshot.values.size(); ++value)
			values << ",value" << value << '=' << snapshot.values[value];
		auto optional = [&](uint8_t mask, const std::string &value)
		{ return snapshot.string_mask & mask ? quote(connection, value) : "NULL"; };
		values << ",name=" << optional(1, snapshot.name)
		       << ",short_descr=" << optional(4, snapshot.short_description)
		       << ",description=" << optional(2, snapshot.description)
		       << ",action_descr=" << optional(8, snapshot.action_description);
		for (size_t bitvector = 0; bitvector < snapshot.bitvectors.size(); ++bitvector)
			values << ",bitvector" << bitvector + 1 << '='
			       << snapshot.bitvectors[bitvector];
		values << ",item_material=" << static_cast<unsigned int>(snapshot.material)
		       << ",obj_uid=" << snapshot.object_uid
		       << ",item_condition=" << snapshot.condition;
		if (item_id)
		{
			if (!run_sql(connection, "UPDATE player_items SET " + values.str() +
							 " WHERE id=" + std::to_string(item_id)) ||
			    mysql_affected_rows(connection) > 1)
				return false;
		}
		else
		{
			if (!run_sql(connection, "INSERT INTO player_items SET " + values.str()))
				return false;
			item_id = mysql_insert_id(connection);
			if (!item_id)
			{
				errno = EIO;
				return false;
			}
		}
		if (!run_sql(connection, "DELETE FROM player_item_affects WHERE item_id=" +
						 std::to_string(item_id)) ||
		    !run_sql(connection, "DELETE FROM player_item_extra_descr WHERE item_id=" +
						 std::to_string(item_id)))
			return false;
		std::unordered_set<uint64_t> affects;
		for (const auto &affect : snapshot.affects)
		{
			if (!affect[0] && !affect[1])
				continue;
			const uint64_t key =
				(static_cast<uint64_t>(static_cast<uint16_t>(affect[0])) << 32) |
				static_cast<uint32_t>(affect[1]);
			if (affects.insert(key).second &&
			    !run_sql(
				    connection,
				    "INSERT INTO player_item_affects(item_id,location,modifier) VALUES(" +
					    std::to_string(item_id) + "," +
					    std::to_string(affect[0]) + "," +
					    std::to_string(affect[1]) + ")"))
				return false;
		}
		std::unordered_set<std::string> descriptions;
		for (const auto &description : snapshot.extra_descriptions)
		{
			if (description.keyword.empty())
			{
				if (description.spellbook || !description.spell_ids.empty())
				{
					errno = EINVAL;
					return false;
				}
				continue;
			}
			std::string keyword;
			std::string text;
			if (!canonicalize_extra_description(description, &keyword, &text))
				return false;
			std::string key = keyword;
			key.push_back('\0');
			key += text;
			if (descriptions.insert(std::move(key)).second &&
			    !run_sql(connection,
				     "INSERT INTO player_item_extra_descr(item_id,keyword,description) "
				     "VALUES(" +
					     std::to_string(item_id) + "," +
					     quote(connection, keyword) + "," +
					     quote(connection, text) + ")"))
				return false;
		}
	}

	for (size_t index = 0; index < snapshots.size(); ++index)
	{
		const player_item_snapshot &snapshot = snapshots[index];
		uint64_t parent_id = external_parent_id;
		if (snapshot.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			parent_id = row_by_uid.at(snapshots[snapshot.parent_index].object_uid);
		const std::string parent = parent_id ? std::to_string(parent_id) : "NULL";
		if (!run_sql(connection,
			     "UPDATE player_items SET container_id=" + parent + " WHERE id=" +
				     std::to_string(row_by_uid.at(snapshot.object_uid))) ||
		    mysql_affected_rows(connection) > 1)
			return false;
	}
	return true;
}

bool prepare(MYSQL_STMT **statement, MYSQL *connection, const char *sql)
{
	*statement = mysql_stmt_init(connection);
	if (!*statement || mysql_stmt_prepare(*statement, sql, strlen(sql)) != 0)
	{
		errno = *statement ? mysql_stmt_errno(*statement) : ENOMEM;
		if (*statement)
			mysql_stmt_close(*statement);
		*statement = nullptr;
		return false;
	}
	return true;
}

bool statement_ok(MYSQL_STMT *statement, bool ok)
{
	if (!ok)
	{
		const unsigned int error = mysql_stmt_errno(statement);
		errno = error ? static_cast<int>(error) : EIO;
	}
	mysql_stmt_close(statement);
	return ok;
}

bool ensure_owner(MYSQL *connection, const item_owner_identity &owner)
{
	static const char SQL[] =
		"INSERT IGNORE INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) "
		"VALUES(?,?,?,0)";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	uint8_t type = static_cast<uint8_t>(owner.type);
	MYSQL_BIND bindings[3] = {};
	bindings[0].buffer_type = MYSQL_TYPE_TINY;
	bindings[0].buffer = &type;
	bindings[0].is_unsigned = true;
	bindings[1].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[1].buffer = const_cast<uint64_t *>(&owner.id);
	bindings[1].is_unsigned = true;
	bindings[2].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[2].buffer = const_cast<uint64_t *>(&owner.context_id);
	bindings[2].is_unsigned = true;
	return statement_ok(statement, mysql_stmt_bind_param(statement, bindings) == 0 &&
					       mysql_stmt_execute(statement) == 0);
}

bool lock_owner(MYSQL *connection, const item_owner_identity &owner, uint64_t *revision)
{
	static const char SQL[] =
		"SELECT revision FROM item_owner_revision WHERE owner_type=? AND owner_id=? "
		"AND owner_context_id=? FOR UPDATE";
	MYSQL_STMT *statement = nullptr;
	if (!revision || !prepare(&statement, connection, SQL))
		return false;
	uint8_t type = static_cast<uint8_t>(owner.type);
	MYSQL_BIND parameters[3] = {};
	parameters[0].buffer_type = MYSQL_TYPE_TINY;
	parameters[0].buffer = &type;
	parameters[0].is_unsigned = true;
	parameters[1].buffer_type = MYSQL_TYPE_LONGLONG;
	parameters[1].buffer = const_cast<uint64_t *>(&owner.id);
	parameters[1].is_unsigned = true;
	parameters[2].buffer_type = MYSQL_TYPE_LONGLONG;
	parameters[2].buffer = const_cast<uint64_t *>(&owner.context_id);
	parameters[2].is_unsigned = true;
	MYSQL_BIND output = {};
	output.buffer_type = MYSQL_TYPE_LONGLONG;
	output.buffer = revision;
	output.is_unsigned = true;
	const bool ok = mysql_stmt_bind_param(statement, parameters) == 0 &&
			mysql_stmt_execute(statement) == 0 &&
			mysql_stmt_bind_result(statement, &output) == 0 &&
			mysql_stmt_fetch(statement) == 0;
	return statement_ok(statement, ok);
}

bool owner_identity_less(const item_owner_identity &left, const item_owner_identity &right)
{
	if (left.type != right.type)
		return left.type < right.type;
	if (left.id != right.id)
		return left.id < right.id;
	return left.context_id < right.context_id;
}

bool load_root(MYSQL *connection, uint64_t root_item_uid, std::vector<current_item> *items,
	       bool append = false)
{
	static const char SQL[] =
		"SELECT item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,"
		"item_revision,vnum,state FROM item_current_owner WHERE root_item_uid=? "
		"ORDER BY item_uid FOR UPDATE";
	if (!items)
		return false;
	if (!append)
		items->clear();
	try
	{
		items->reserve(ITEM_TRANSFER_MAX_ITEMS + 1);
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return false;
	}
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	MYSQL_BIND parameter = {};
	parameter.buffer_type = MYSQL_TYPE_LONGLONG;
	parameter.buffer = &root_item_uid;
	parameter.is_unsigned = true;
	if (mysql_stmt_bind_param(statement, &parameter) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_store_result(statement) != 0)
		return statement_ok(statement, false);
	current_item item = {};
	mysql_null_indicator parent_null = 0;
	MYSQL_BIND output[9] = {};
	output[0].buffer_type = MYSQL_TYPE_LONGLONG;
	output[0].buffer = &item.item_uid;
	output[0].is_unsigned = true;
	output[1].buffer_type = MYSQL_TYPE_LONGLONG;
	output[1].buffer = &item.root_item_uid;
	output[1].is_unsigned = true;
	output[2].buffer_type = MYSQL_TYPE_LONGLONG;
	output[2].buffer = &item.parent_item_uid;
	output[2].is_unsigned = true;
	output[2].is_null = &parent_null;
	output[3].buffer_type = MYSQL_TYPE_TINY;
	output[3].buffer = &item.owner_type;
	output[3].is_unsigned = true;
	output[4].buffer_type = MYSQL_TYPE_LONGLONG;
	output[4].buffer = &item.owner_id;
	output[4].is_unsigned = true;
	output[5].buffer_type = MYSQL_TYPE_LONGLONG;
	output[5].buffer = &item.owner_context_id;
	output[5].is_unsigned = true;
	output[6].buffer_type = MYSQL_TYPE_LONGLONG;
	output[6].buffer = &item.item_revision;
	output[6].is_unsigned = true;
	output[7].buffer_type = MYSQL_TYPE_LONG;
	output[7].buffer = &item.vnum;
	output[8].buffer_type = MYSQL_TYPE_TINY;
	output[8].buffer = &item.state;
	output[8].is_unsigned = true;
	if (mysql_stmt_bind_result(statement, output) != 0)
		return statement_ok(statement, false);
	int fetched = 0;
	while ((fetched = mysql_stmt_fetch(statement)) == 0)
	{
		item.parent_item_uid = parent_null ? 0 : item.parent_item_uid;
		items->push_back(item);
		if (items->size() > ITEM_TRANSFER_MAX_ITEMS)
			break;
		item = {};
		parent_null = 0;
	}
	return statement_ok(statement,
			    fetched == MYSQL_NO_DATA || items->size() > ITEM_TRANSFER_MAX_ITEMS);
}

bool load_owner(MYSQL *connection, const item_owner_identity &owner,
		std::vector<current_item> *items)
{
	static const char SQL[] =
		"SELECT item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,"
		"item_revision,vnum,state FROM item_current_owner WHERE owner_type=? AND owner_id=? "
		"AND owner_context_id=? ORDER BY item_uid FOR UPDATE";
	if (!items)
		return false;
	items->clear();
	try
	{
		items->reserve(ITEM_TRANSFER_MAX_ITEMS + 1);
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return false;
	}
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	uint8_t owner_type = static_cast<uint8_t>(owner.type);
	MYSQL_BIND parameters[3] = {};
	parameters[0].buffer_type = MYSQL_TYPE_TINY;
	parameters[0].buffer = &owner_type;
	parameters[0].is_unsigned = true;
	parameters[1].buffer_type = MYSQL_TYPE_LONGLONG;
	parameters[1].buffer = const_cast<uint64_t *>(&owner.id);
	parameters[1].is_unsigned = true;
	parameters[2].buffer_type = MYSQL_TYPE_LONGLONG;
	parameters[2].buffer = const_cast<uint64_t *>(&owner.context_id);
	parameters[2].is_unsigned = true;
	if (mysql_stmt_bind_param(statement, parameters) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_store_result(statement) != 0)
		return statement_ok(statement, false);
	current_item item = {};
	mysql_null_indicator parent_null = 0;
	MYSQL_BIND output[9] = {};
	output[0].buffer_type = MYSQL_TYPE_LONGLONG;
	output[0].buffer = &item.item_uid;
	output[0].is_unsigned = true;
	output[1].buffer_type = MYSQL_TYPE_LONGLONG;
	output[1].buffer = &item.root_item_uid;
	output[1].is_unsigned = true;
	output[2].buffer_type = MYSQL_TYPE_LONGLONG;
	output[2].buffer = &item.parent_item_uid;
	output[2].is_unsigned = true;
	output[2].is_null = &parent_null;
	output[3].buffer_type = MYSQL_TYPE_TINY;
	output[3].buffer = &item.owner_type;
	output[3].is_unsigned = true;
	output[4].buffer_type = MYSQL_TYPE_LONGLONG;
	output[4].buffer = &item.owner_id;
	output[4].is_unsigned = true;
	output[5].buffer_type = MYSQL_TYPE_LONGLONG;
	output[5].buffer = &item.owner_context_id;
	output[5].is_unsigned = true;
	output[6].buffer_type = MYSQL_TYPE_LONGLONG;
	output[6].buffer = &item.item_revision;
	output[6].is_unsigned = true;
	output[7].buffer_type = MYSQL_TYPE_LONG;
	output[7].buffer = &item.vnum;
	output[8].buffer_type = MYSQL_TYPE_TINY;
	output[8].buffer = &item.state;
	output[8].is_unsigned = true;
	if (mysql_stmt_bind_result(statement, output) != 0)
		return statement_ok(statement, false);
	int fetched = 0;
	while ((fetched = mysql_stmt_fetch(statement)) == 0)
	{
		item.parent_item_uid = parent_null ? 0 : item.parent_item_uid;
		items->push_back(item);
		if (items->size() > ITEM_TRANSFER_MAX_ITEMS)
			break;
		item = {};
		parent_null = 0;
	}
	const bool complete = fetched == MYSQL_NO_DATA;
	const bool within_limit = items->size() <= ITEM_TRANSFER_MAX_ITEMS;
	if (!within_limit)
		errno = EMSGSIZE;
	return statement_ok(statement, complete && within_limit);
}

bool item_exists(MYSQL *connection, uint64_t item_uid, bool *found)
{
	static const char SQL[] =
		"SELECT item_uid FROM item_current_owner WHERE item_uid=? FOR UPDATE";
	if (!found)
		return false;
	*found = false;
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	MYSQL_BIND parameter = {};
	parameter.buffer_type = MYSQL_TYPE_LONGLONG;
	parameter.buffer = &item_uid;
	parameter.is_unsigned = true;
	uint64_t stored_uid = 0;
	MYSQL_BIND output = {};
	output.buffer_type = MYSQL_TYPE_LONGLONG;
	output.buffer = &stored_uid;
	output.is_unsigned = true;
	if (mysql_stmt_bind_param(statement, &parameter) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_bind_result(statement, &output) != 0)
		return statement_ok(statement, false);
	const int fetched = mysql_stmt_fetch(statement);
	if (fetched != 0 && fetched != MYSQL_NO_DATA)
		return statement_ok(statement, false);
	*found = fetched == 0 && stored_uid == item_uid;
	return statement_ok(statement, true);
}

bool load_item(MYSQL *connection, uint64_t item_uid, current_item *item, bool *found)
{
	static const char SQL[] =
		"SELECT item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,"
		"item_revision,vnum,state FROM item_current_owner WHERE item_uid=? FOR UPDATE";
	if (!item || !found)
		return false;
	*item = {};
	*found = false;
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	MYSQL_BIND parameter = {};
	parameter.buffer_type = MYSQL_TYPE_LONGLONG;
	parameter.buffer = &item_uid;
	parameter.is_unsigned = true;
	if (mysql_stmt_bind_param(statement, &parameter) != 0 || mysql_stmt_execute(statement) != 0)
		return statement_ok(statement, false);
	mysql_null_indicator parent_null = 0;
	MYSQL_BIND output[9] = {};
	output[0].buffer_type = MYSQL_TYPE_LONGLONG;
	output[0].buffer = &item->item_uid;
	output[0].is_unsigned = true;
	output[1].buffer_type = MYSQL_TYPE_LONGLONG;
	output[1].buffer = &item->root_item_uid;
	output[1].is_unsigned = true;
	output[2].buffer_type = MYSQL_TYPE_LONGLONG;
	output[2].buffer = &item->parent_item_uid;
	output[2].is_unsigned = true;
	output[2].is_null = &parent_null;
	output[3].buffer_type = MYSQL_TYPE_TINY;
	output[3].buffer = &item->owner_type;
	output[3].is_unsigned = true;
	output[4].buffer_type = MYSQL_TYPE_LONGLONG;
	output[4].buffer = &item->owner_id;
	output[4].is_unsigned = true;
	output[5].buffer_type = MYSQL_TYPE_LONGLONG;
	output[5].buffer = &item->owner_context_id;
	output[5].is_unsigned = true;
	output[6].buffer_type = MYSQL_TYPE_LONGLONG;
	output[6].buffer = &item->item_revision;
	output[6].is_unsigned = true;
	output[7].buffer_type = MYSQL_TYPE_LONG;
	output[7].buffer = &item->vnum;
	output[8].buffer_type = MYSQL_TYPE_TINY;
	output[8].buffer = &item->state;
	output[8].is_unsigned = true;
	if (mysql_stmt_bind_result(statement, output) != 0)
		return statement_ok(statement, false);
	const int fetched = mysql_stmt_fetch(statement);
	if (fetched != 0 && fetched != MYSQL_NO_DATA)
		return statement_ok(statement, false);
	*found = fetched == 0;
	if (*found && parent_null)
		item->parent_item_uid = 0;
	return statement_ok(statement, true);
}

bool insert_created_item(MYSQL *connection, const item_transfer_entry &entry,
			 const item_owner_identity &owner)
{
	static const char SQL[] =
		"INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,"
		"owner_id,owner_context_id,item_revision,vnum,state) VALUES(?,?,NULL,?,?,?,0,?,1)";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	uint8_t type = static_cast<uint8_t>(owner.type);
	MYSQL_BIND bindings[6] = {};
	bindings[0].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[0].buffer = const_cast<uint64_t *>(&entry.item_uid);
	bindings[0].is_unsigned = true;
	bindings[1].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[1].buffer = const_cast<uint64_t *>(&entry.root_item_uid);
	bindings[1].is_unsigned = true;
	bindings[2].buffer_type = MYSQL_TYPE_TINY;
	bindings[2].buffer = &type;
	bindings[2].is_unsigned = true;
	bindings[3].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[3].buffer = const_cast<uint64_t *>(&owner.id);
	bindings[3].is_unsigned = true;
	bindings[4].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[4].buffer = const_cast<uint64_t *>(&owner.context_id);
	bindings[4].is_unsigned = true;
	bindings[5].buffer_type = MYSQL_TYPE_LONG;
	bindings[5].buffer = const_cast<int32_t *>(&entry.vnum);
	return statement_ok(statement, mysql_stmt_bind_param(statement, bindings) == 0 &&
					       mysql_stmt_execute(statement) == 0);
}

bool update_item(MYSQL *connection, const item_transfer_entry &entry, uint64_t target_root_item_uid,
		 uint64_t target_parent_item_uid, const item_owner_identity &owner,
		 uint64_t prior_revision, item_custody_state state)
{
	static const char SQL[] =
		"UPDATE item_current_owner SET root_item_uid=?,parent_item_uid=IF(?=0,NULL,?),"
		"owner_type=?,owner_id=?,owner_context_id=?,item_revision=?,state=? "
		"WHERE item_uid=? AND item_revision=?";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	uint8_t type = static_cast<uint8_t>(owner.type);
	uint8_t state_value = static_cast<uint8_t>(state);
	uint64_t revision = prior_revision + 1;
	MYSQL_BIND bindings[10] = {};
	bindings[0].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[0].buffer = &target_root_item_uid;
	bindings[0].is_unsigned = true;
	bindings[1].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[1].buffer = &target_parent_item_uid;
	bindings[1].is_unsigned = true;
	bindings[2] = bindings[1];
	bindings[3].buffer_type = MYSQL_TYPE_TINY;
	bindings[3].buffer = &type;
	bindings[3].is_unsigned = true;
	bindings[4].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[4].buffer = const_cast<uint64_t *>(&owner.id);
	bindings[4].is_unsigned = true;
	bindings[5].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[5].buffer = const_cast<uint64_t *>(&owner.context_id);
	bindings[5].is_unsigned = true;
	bindings[6].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[6].buffer = &revision;
	bindings[6].is_unsigned = true;
	bindings[7].buffer_type = MYSQL_TYPE_TINY;
	bindings[7].buffer = &state_value;
	bindings[7].is_unsigned = true;
	bindings[8].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[8].buffer = const_cast<uint64_t *>(&entry.item_uid);
	bindings[8].is_unsigned = true;
	bindings[9].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[9].buffer = &prior_revision;
	bindings[9].is_unsigned = true;
	return statement_ok(statement, mysql_stmt_bind_param(statement, bindings) == 0 &&
					       mysql_stmt_execute(statement) == 0 &&
					       mysql_stmt_affected_rows(statement) == 1);
}

bool update_owner_revision(MYSQL *connection, const item_owner_identity &owner,
			   uint64_t prior_revision)
{
	static const char SQL[] =
		"UPDATE item_owner_revision SET revision=? WHERE owner_type=? AND owner_id=? "
		"AND owner_context_id=? AND revision=?";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	uint64_t revision = prior_revision + 1;
	uint8_t type = static_cast<uint8_t>(owner.type);
	MYSQL_BIND bindings[5] = {};
	bindings[0].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[0].buffer = &revision;
	bindings[0].is_unsigned = true;
	bindings[1].buffer_type = MYSQL_TYPE_TINY;
	bindings[1].buffer = &type;
	bindings[1].is_unsigned = true;
	bindings[2].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[2].buffer = const_cast<uint64_t *>(&owner.id);
	bindings[2].is_unsigned = true;
	bindings[3].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[3].buffer = const_cast<uint64_t *>(&owner.context_id);
	bindings[3].is_unsigned = true;
	bindings[4].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[4].buffer = &prior_revision;
	bindings[4].is_unsigned = true;
	return statement_ok(statement, mysql_stmt_bind_param(statement, bindings) == 0 &&
					       mysql_stmt_execute(statement) == 0 &&
					       mysql_stmt_affected_rows(statement) == 1);
}

bool insert_ledger(MYSQL *connection, const critical_command &command,
		   const item_transfer_payload &payload, size_t index, uint16_t event_index_base,
		   uint64_t item_revision, uint64_t from_revision, uint64_t to_revision)
{
	static const char SQL[] =
		"INSERT INTO item_ownership_ledger(operation_id,event_index,item_uid,root_item_uid,"
		"parent_item_uid,from_owner_type,from_owner_id,from_owner_context_id,to_owner_type,"
		"to_owner_id,to_owner_context_id,item_revision,from_owner_revision,to_owner_revision,"
		"reason_type,reason_id,source_site) VALUES(?,?,?,?,IF(?=0,NULL,?),?,?,?,?,?,?,?,?,?,?,?,?)";
	const item_transfer_entry &entry = payload.items[index];
	uint64_t target_root = 0, target_parent = 0;
	if (!item_transfer_target_topology(payload, entry.item_uid, &target_root, &target_parent))
		return false;
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	uint16_t event_index = static_cast<uint16_t>(event_index_base + index);
	uint8_t from_type = static_cast<uint8_t>(payload.from_owner.type);
	uint8_t to_type = static_cast<uint8_t>(payload.to_owner.type);
	uint16_t reason = static_cast<uint16_t>(payload.reason);
	uint16_t source = static_cast<uint16_t>(command.source_site);
	unsigned long operation_length = command.operation_id.bytes.size();
	MYSQL_BIND bindings[18] = {};
	bindings[0].buffer_type = MYSQL_TYPE_BLOB;
	bindings[0].buffer = const_cast<uint8_t *>(command.operation_id.bytes.data());
	bindings[0].buffer_length = operation_length;
	bindings[0].length = &operation_length;
	bindings[1].buffer_type = MYSQL_TYPE_SHORT;
	bindings[1].buffer = &event_index;
	bindings[1].is_unsigned = true;
	uint64_t *unsigned_values[] = {
		const_cast<uint64_t *>(&entry.item_uid),
		&target_root,
		const_cast<uint64_t *>(&target_parent),
		const_cast<uint64_t *>(&target_parent),
	};
	for (size_t value = 0; value < 4; ++value)
	{
		bindings[value + 2].buffer_type = MYSQL_TYPE_LONGLONG;
		bindings[value + 2].buffer = unsigned_values[value];
		bindings[value + 2].is_unsigned = true;
	}
	bindings[6].buffer_type = MYSQL_TYPE_TINY;
	bindings[6].buffer = &from_type;
	bindings[6].is_unsigned = true;
	bindings[7].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[7].buffer = const_cast<uint64_t *>(&payload.from_owner.id);
	bindings[7].is_unsigned = true;
	bindings[8].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[8].buffer = const_cast<uint64_t *>(&payload.from_owner.context_id);
	bindings[8].is_unsigned = true;
	bindings[9].buffer_type = MYSQL_TYPE_TINY;
	bindings[9].buffer = &to_type;
	bindings[9].is_unsigned = true;
	bindings[10].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[10].buffer = const_cast<uint64_t *>(&payload.to_owner.id);
	bindings[10].is_unsigned = true;
	bindings[11].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[11].buffer = const_cast<uint64_t *>(&payload.to_owner.context_id);
	bindings[11].is_unsigned = true;
	bindings[12].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[12].buffer = &item_revision;
	bindings[12].is_unsigned = true;
	bindings[13].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[13].buffer = &from_revision;
	bindings[13].is_unsigned = true;
	bindings[14].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[14].buffer = &to_revision;
	bindings[14].is_unsigned = true;
	bindings[15].buffer_type = MYSQL_TYPE_SHORT;
	bindings[15].buffer = &reason;
	bindings[15].is_unsigned = true;
	bindings[16].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[16].buffer = const_cast<int64_t *>(&payload.reason_id);
	bindings[17].buffer_type = MYSQL_TYPE_SHORT;
	bindings[17].buffer = &source;
	bindings[17].is_unsigned = true;
	return statement_ok(statement, mysql_stmt_bind_param(statement, bindings) == 0 &&
					       mysql_stmt_execute(statement) == 0);
}

bool update_coin_payload(MYSQL *connection, const item_transfer_payload &payload, uint64_t revision)
{
	const uint64_t uid = payload.selected_item_uid;
	std::vector<player_item_snapshot> items;
	if (player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
					     &items) != player_snapshot_codec_result::ok ||
	    items.size() != 1 || items[0].object_uid != uid ||
	    items[0].vnum != payload.items[0].vnum || items[0].type != ITEM_MONEY ||
	    std::any_of(items[0].values.begin(), items[0].values.begin() + 4,
			[](int32_t value) { return value < 0; }))
	{
		errno = EBADMSG;
		return false;
	}
	static const char UPDATE_SQL[] =
		"UPDATE item_current_owner SET coin_payload=? WHERE item_uid=? AND item_revision=?";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, UPDATE_SQL))
		return false;
	MYSQL_BIND bindings[3] = {};
	unsigned long length = payload.item_blob_size;
	mysql_null_indicator removed = payload.to_owner.type == item_owner_type::destruction;
	bindings[0].buffer_type = MYSQL_TYPE_BLOB;
	bindings[0].buffer = const_cast<uint8_t *>(payload.item_blob.data());
	bindings[0].buffer_length = length;
	bindings[0].length = &length;
	bindings[0].is_null = &removed;
	bindings[1].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[1].buffer = const_cast<uint64_t *>(&uid);
	bindings[1].is_unsigned = true;
	bindings[2].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[2].buffer = &revision;
	bindings[2].is_unsigned = true;
	return statement_ok(statement, mysql_stmt_bind_param(statement, bindings) == 0 &&
					       mysql_stmt_execute(statement) == 0);
}

bool update_runtime_payload(MYSQL *connection, uint64_t item_uid,
			    const std::vector<uint8_t> &payload)
{
	static const char UPDATE_SQL[] =
		"UPDATE player_death_restitution_runtime SET state_payload=?,"
		"state_digest=UNHEX(SHA2(?,256)) WHERE item_uid=?";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, UPDATE_SQL))
		return false;
	MYSQL_BIND bindings[3] = {};
	unsigned long length = static_cast<unsigned long>(payload.size());
	bindings[0].buffer_type = MYSQL_TYPE_BLOB;
	bindings[0].buffer = const_cast<uint8_t *>(payload.data());
	bindings[0].buffer_length = length;
	bindings[0].length = &length;
	bindings[1].buffer_type = MYSQL_TYPE_BLOB;
	bindings[1].buffer = const_cast<uint8_t *>(payload.data());
	bindings[1].buffer_length = length;
	bindings[1].length = &length;
	bindings[2].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[2].buffer = &item_uid;
	bindings[2].is_unsigned = true;
	return statement_ok(statement, mysql_stmt_bind_param(statement, bindings) == 0 &&
					       mysql_stmt_execute(statement) == 0);
}

bool sync_restitution_runtime_payload(MYSQL *connection, const item_transfer_payload &payload)
{
	if (!connection)
	{
		errno = EINVAL;
		return false;
	}
	MYSQL_RES *tables = nullptr;
	static const char TABLES_SQL[] =
		"SELECT table_name FROM information_schema.tables WHERE table_schema=DATABASE() "
		"AND table_name IN ('player_death_restitution_delivery',"
		"'player_death_restitution_runtime')";
	if (mysql_real_query(connection, TABLES_SQL, strlen(TABLES_SQL)) != 0 ||
	    !(tables = mysql_store_result(connection)))
	{
		errno = mysql_errno(connection);
		return false;
	}
	bool delivery_present = false;
	bool runtime_present = false;
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(tables)) != nullptr)
	{
		if (!row[0])
		{
			mysql_free_result(tables);
			errno = EINVAL;
			return false;
		}
		if (!strcmp(row[0], "player_death_restitution_delivery"))
			delivery_present = true;
		else if (!strcmp(row[0], "player_death_restitution_runtime"))
			runtime_present = true;
	}
	mysql_free_result(tables);
	if (!delivery_present || !payload.item_count)
		return true;

	std::ostringstream uid_list;
	for (size_t index = 0; index < payload.item_count; ++index)
		uid_list << (index ? "," : "") << payload.items[index].item_uid;
	const std::string delivery_sql =
		"SELECT item_uid FROM player_death_restitution_delivery WHERE item_uid IN (" +
		uid_list.str() + ") ORDER BY item_uid FOR UPDATE";
	if (mysql_real_query(connection, delivery_sql.data(), delivery_sql.size()) != 0 ||
	    !(tables = mysql_store_result(connection)))
	{
		errno = mysql_errno(connection);
		return false;
	}
	std::vector<uint64_t> delivered_uids;
	while ((row = mysql_fetch_row(tables)) != nullptr)
	{
		if (!row[0])
		{
			mysql_free_result(tables);
			errno = EINVAL;
			return false;
		}
		char *end = nullptr;
		errno = 0;
		const unsigned long long parsed = std::strtoull(row[0], &end, 10);
		if (errno || end == row[0] || *end || parsed == 0)
		{
			mysql_free_result(tables);
			errno = EINVAL;
			return false;
		}
		delivered_uids.push_back(static_cast<uint64_t>(parsed));
	}
	mysql_free_result(tables);
	if (delivered_uids.empty())
		return true;
	if (!runtime_present)
	{
		errno = ENOENT;
		return false;
	}

	const std::string runtime_sql =
		"SELECT item_uid FROM player_death_restitution_runtime WHERE item_uid IN (" +
		uid_list.str() + ") ORDER BY item_uid FOR UPDATE";
	if (mysql_real_query(connection, runtime_sql.data(), runtime_sql.size()) != 0 ||
	    !(tables = mysql_store_result(connection)))
	{
		errno = mysql_errno(connection);
		return false;
	}
	std::vector<uint64_t> runtime_uids;
	while ((row = mysql_fetch_row(tables)) != nullptr)
	{
		uint64_t item_uid = 0;
		if (!row[0])
		{
			mysql_free_result(tables);
			errno = EINVAL;
			return false;
		}
		char *end = nullptr;
		errno = 0;
		const unsigned long long parsed = std::strtoull(row[0], &end, 10);
		if (errno || end == row[0] || *end || parsed == 0 || parsed > UINT64_MAX)
		{
			mysql_free_result(tables);
			errno = EINVAL;
			return false;
		}
		item_uid = static_cast<uint64_t>(parsed);
		runtime_uids.push_back(item_uid);
	}
	mysql_free_result(tables);
	for (const uint64_t item_uid : delivered_uids)
		if (std::find(runtime_uids.begin(), runtime_uids.end(), item_uid) ==
		    runtime_uids.end())
		{
			errno = ENOENT;
			return false;
		}

	if (!payload.item_blob_size)
	{
		errno = EBADMSG;
		return false;
	}
	std::vector<player_item_snapshot> snapshots;
	if (player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
					     &snapshots) != player_snapshot_codec_result::ok)
	{
		errno = EBADMSG;
		return false;
	}
	for (const uint64_t item_uid : delivered_uids)
	{
		const auto entry = std::find_if(payload.items.begin(),
						payload.items.begin() + payload.item_count,
						[item_uid](const item_transfer_entry &candidate)
						{ return candidate.item_uid == item_uid; });
		const auto source = std::find_if(snapshots.begin(), snapshots.end(),
						 [item_uid](const player_item_snapshot &candidate)
						 { return candidate.object_uid == item_uid; });
		if (entry == payload.items.begin() + payload.item_count ||
		    source == snapshots.end() || source->vnum != entry->vnum)
		{
			errno = EBADMSG;
			return false;
		}
		player_item_snapshot standalone = *source;
		standalone.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		standalone.equipment_slot = -1;
		std::vector<player_item_snapshot> one = { standalone };
		std::vector<uint8_t> encoded;
		if (player_item_snapshot_list_encode(one, &encoded) !=
			    player_snapshot_codec_result::ok ||
		    !update_runtime_payload(connection, item_uid, encoded))
		{
			if (!errno)
				errno = EBADMSG;
			return false;
		}
	}
	return true;
}
} // namespace

bool item_transfer_repository_ensure_owner(MYSQL *connection, const item_owner_identity &owner)
{
	if (!connection || !item_owner_identity_valid(owner))
	{
		errno = EINVAL;
		return false;
	}
	return ensure_owner(connection, owner);
}

bool item_transfer_repository_lock_owner(MYSQL *connection, const item_owner_identity &owner,
					 uint64_t *revision)
{
	if (!connection || !revision || !item_owner_identity_valid(owner))
	{
		errno = EINVAL;
		return false;
	}
	return lock_owner(connection, owner, revision);
}

bool item_transfer_repository_advance_owner(MYSQL *connection, const item_owner_identity &owner,
					    uint64_t prior_revision)
{
	if (!connection || !item_owner_identity_valid(owner) || prior_revision == UINT64_MAX)
	{
		errno = prior_revision == UINT64_MAX ? ERANGE : EINVAL;
		return false;
	}
	return update_owner_revision(connection, owner, prior_revision);
}

bool item_transfer_repository_execute_at_offset(MYSQL *connection, const critical_command &command,
						uint16_t event_index_base,
						item_transfer_result *result,
						unsigned int *result_code, bool *mutation_applied,
						item_transfer_failure_stage *failure_stage)
{
	item_transfer_payload payload = {};
	if (!connection || !result || !result_code || !mutation_applied ||
	    !item_transfer_command_decode_payload(command, &payload))
	{
		errno = EINVAL;
		return false;
	}
	*result = { item_transfer_result_root(payload), payload.item_count, 0, 0, 0, 0 };
	*result_code = 0;
	*mutation_applied = false;
	if (failure_stage)
		*failure_stage = item_transfer_failure_stage::none;
	if (static_cast<size_t>(event_index_base) + payload.item_count > UINT16_MAX)
	{
		*result_code = E2BIG;
		return true;
	}
	uint64_t from_revision = 0, to_revision = 0;
	const bool same_owner = item_owner_identity_equal(payload.from_owner, payload.to_owner);
	if (same_owner)
	{
		if (!ensure_owner(connection, payload.from_owner) ||
		    !lock_owner(connection, payload.from_owner, &from_revision))
			return false;
		to_revision = from_revision;
	}
	else if (owner_identity_less(payload.from_owner, payload.to_owner))
	{
		if (!ensure_owner(connection, payload.from_owner) ||
		    !ensure_owner(connection, payload.to_owner) ||
		    !lock_owner(connection, payload.from_owner, &from_revision) ||
		    !lock_owner(connection, payload.to_owner, &to_revision))
			return false;
	}
	else if (!ensure_owner(connection, payload.to_owner) ||
		 !ensure_owner(connection, payload.from_owner) ||
		 !lock_owner(connection, payload.to_owner, &to_revision) ||
		 !lock_owner(connection, payload.from_owner, &from_revision))
		return false;
	result->from_owner_revision = from_revision;
	result->to_owner_revision = to_revision;
	if (from_revision != payload.expected_from_revision ||
	    to_revision != payload.expected_to_revision)
	{
		if (failure_stage)
		{
			uint8_t stage = 0;
			if (from_revision != payload.expected_from_revision)
				stage |= static_cast<uint8_t>(
					item_transfer_failure_stage::from_owner_revision);
			if (to_revision != payload.expected_to_revision)
				stage |= static_cast<uint8_t>(
					item_transfer_failure_stage::to_owner_revision);
			*failure_stage = static_cast<item_transfer_failure_stage>(stage);
		}
		*result_code = ESTALE;
		return true;
	}
	std::vector<current_item> current;
	const bool creation = payload.from_owner.type == item_owner_type::system;
	if (creation)
	{
		std::vector<uint64_t> source_roots;
		try
		{
			for (size_t index = 0; index < payload.item_count; ++index)
				source_roots.push_back(payload.items[index].root_item_uid);
			std::sort(source_roots.begin(), source_roots.end());
			source_roots.erase(std::unique(source_roots.begin(), source_roots.end()),
					   source_roots.end());
		}
		catch (const std::bad_alloc &)
		{
			errno = ENOMEM;
			return false;
		}
		for (size_t index = 0; index < source_roots.size(); ++index)
			if (!load_root(connection, source_roots[index], &current, index != 0))
				return false;
	}
	else
	{
		std::vector<uint64_t> source_roots;
		try
		{
			for (size_t index = 0; index < payload.item_count; ++index)
				source_roots.push_back(payload.items[index].root_item_uid);
			std::sort(source_roots.begin(), source_roots.end());
			source_roots.erase(std::unique(source_roots.begin(), source_roots.end()),
					   source_roots.end());
		}
		catch (const std::bad_alloc &)
		{
			errno = ENOMEM;
			return false;
		}
		for (size_t index = 0; index < source_roots.size(); ++index)
			if (!load_root(connection, source_roots[index], &current, index != 0))
				return false;
		std::sort(current.begin(), current.end(),
			  [](const current_item &left, const current_item &right)
			  { return left.item_uid < right.item_uid; });
	}
	if ((creation && !current.empty()) || (!creation && current.empty()))
	{
		*result_code = creation ? EEXIST : ENOENT;
		return true;
	}
	if (creation)
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			bool found = false;
			if (!item_exists(connection, payload.items[index].item_uid, &found))
				return false;
			if (found)
			{
				*result_code = EEXIST;
				return true;
			}
		}
	std::vector<current_item> selected;
	if (!creation)
	{
		std::vector<uint64_t> selected_roots;
		try
		{
			selected.reserve(payload.item_count);
			for (size_t index = 0; index < payload.item_count; ++index)
				if (item_transfer_selected_root(payload,
								payload.items[index].item_uid) ==
				    payload.items[index].item_uid)
					selected_roots.push_back(payload.items[index].item_uid);
		}
		catch (const std::bad_alloc &)
		{
			errno = ENOMEM;
			return false;
		}
		for (const current_item &candidate : current)
		{
			uint64_t ancestor = candidate.item_uid;
			for (size_t depth = 0; depth <= current.size(); ++depth)
			{
				if (std::binary_search(selected_roots.begin(), selected_roots.end(),
						       ancestor))
				{
					selected.push_back(candidate);
					break;
				}
				auto parent = std::find_if(current.begin(), current.end(),
							   [&](const current_item &entry)
							   { return entry.item_uid == ancestor; });
				if (parent == current.end() || !parent->parent_item_uid)
					break;
				ancestor = parent->parent_item_uid;
			}
		}
		if (selected.size() != payload.item_count)
		{
			*result_code = EMSGSIZE;
			return true;
		}
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const current_item &stored = selected[index];
			const item_transfer_entry &expected = payload.items[index];
			const bool item_revision_mismatch = stored.item_revision !=
							    expected.expected_item_revision;
			result->max_item_revision =
				std::max(result->max_item_revision, stored.item_revision);
			if (stored.item_uid != expected.item_uid ||
			    stored.root_item_uid != expected.root_item_uid ||
			    stored.parent_item_uid != expected.parent_item_uid ||
			    stored.owner_type != static_cast<uint8_t>(payload.from_owner.type) ||
			    stored.owner_id != payload.from_owner.id ||
			    stored.owner_context_id != payload.from_owner.context_id ||
			    item_revision_mismatch || stored.vnum != expected.vnum ||
			    stored.state != static_cast<uint8_t>(expected.expected_state))
			{
				if (failure_stage && item_revision_mismatch)
					*failure_stage = static_cast<item_transfer_failure_stage>(
						static_cast<uint8_t>(*failure_stage) |
						static_cast<uint8_t>(
							item_transfer_failure_stage::item_revision));
				*result_code = ESTALE;
				return true;
			}
		}
	}
	if (payload.target_parent_item_uid)
	{
		current_item parent = {};
		bool parent_found = false;
		auto in_current =
			std::find_if(current.begin(), current.end(), [&](const current_item &entry)
				     { return entry.item_uid == payload.target_parent_item_uid; });
		if (in_current != current.end())
		{
			parent = *in_current;
			parent_found = true;
		}
		else if (!load_item(connection, payload.target_parent_item_uid, &parent,
				    &parent_found))
			return false;
		if (!parent_found || parent.root_item_uid != payload.target_root_item_uid ||
		    parent.owner_type != static_cast<uint8_t>(payload.to_owner.type) ||
		    parent.owner_id != payload.to_owner.id ||
		    parent.owner_context_id != payload.to_owner.context_id ||
		    parent.item_revision != payload.expected_target_parent_revision ||
		    parent.state != static_cast<uint8_t>(item_custody_state::active))
		{
			if (failure_stage && parent_found &&
			    parent.item_revision != payload.expected_target_parent_revision)
				*failure_stage = static_cast<item_transfer_failure_stage>(
					static_cast<uint8_t>(*failure_stage) |
					static_cast<uint8_t>(
						item_transfer_failure_stage::target_parent_revision));
			*result_code = ESTALE;
			return true;
		}
	}
	if (from_revision == std::numeric_limits<uint64_t>::max() ||
	    (!same_owner && to_revision == std::numeric_limits<uint64_t>::max()))
	{
		*result_code = ERANGE;
		return true;
	}
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const uint64_t prior_revision = creation ? 0 : selected[index].item_revision;
		if (prior_revision == std::numeric_limits<uint64_t>::max())
		{
			*result_code = ERANGE;
			return true;
		}
		if (creation &&
		    !insert_created_item(connection, payload.items[index], payload.to_owner))
			return false;
		uint64_t target_root = 0, target_parent = 0;
		if (!item_transfer_target_topology(payload, payload.items[index].item_uid,
						   &target_root, &target_parent) ||
		    !update_item(connection, payload.items[index], target_root, target_parent,
				 payload.to_owner, prior_revision,
				 payload.to_owner.type == item_owner_type::destruction ?
					 item_custody_state::destroyed :
					 item_custody_state::active))
			return false;
		const uint64_t item_revision = prior_revision + 1;
		result->max_item_revision = std::max(result->max_item_revision, item_revision);
		if (!insert_ledger(connection, command, payload, index, event_index_base,
				   item_revision, from_revision + 1,
				   same_owner ? from_revision + 1 : to_revision + 1))
			return false;
	}
	if (!move_pet_physical_items(connection, payload))
		return false;
	if (!materialize_direct_player_items(connection, payload))
		return false;
	if (!update_owner_revision(connection, payload.from_owner, from_revision) ||
	    (!same_owner && !update_owner_revision(connection, payload.to_owner, to_revision)))
		return false;
	result->from_owner_revision = from_revision + 1;
	result->to_owner_revision = same_owner ? from_revision + 1 : to_revision + 1;
	if (creation && payload.item_count == 1 && payload.item_blob_size)
	{
		std::vector<player_item_snapshot> snapshots;
		if (player_item_snapshot_list_decode(payload.item_blob.data(),
						     payload.item_blob_size, &snapshots) !=
		    player_snapshot_codec_result::ok)
			return false;
		if (snapshots.size() == 1 && snapshots[0].type == ITEM_MONEY &&
		    !update_coin_payload(connection, payload, result->max_item_revision))
			return false;
	}
	if (!sync_restitution_runtime_payload(connection, payload))
		return false;
	*mutation_applied = true;
	return true;
}

bool item_transfer_repository_execute(MYSQL *connection, const critical_command &command,
				      item_transfer_result *result, unsigned int *result_code,
				      bool *mutation_applied,
				      item_transfer_failure_stage *failure_stage)
{
	return item_transfer_repository_execute_at_offset(
		connection, command, 0, result, result_code, mutation_applied, failure_stage);
}

bool item_transfer_repository_execute_coin(MYSQL *connection, const critical_command &command,
					   const std::array<int32_t, 4> &before,
					   item_transfer_result *result, unsigned int *result_code,
					   bool *mutation_applied,
					   item_transfer_failure_stage *failure_stage)
{
	item_transfer_payload payload = {};
	if (!connection || !result || !result_code || !mutation_applied ||
	    !item_transfer_command_decode_payload(command, &payload) || payload.item_count != 1 ||
	    !payload.item_blob_size)
	{
		errno = EINVAL;
		return false;
	}
	*mutation_applied = false;
	*result_code = 0;
	if (failure_stage)
		*failure_stage = item_transfer_failure_stage::none;
	const uint64_t uid = payload.selected_item_uid;
	if (payload.from_owner.type != item_owner_type::system)
	{
		const std::string query =
			"SELECT coin_payload FROM item_current_owner WHERE item_uid=" +
			std::to_string(uid) + " FOR UPDATE";
		if (mysql_real_query(connection, query.data(), query.size()) != 0)
		{
			errno = mysql_errno(connection);
			return false;
		}
		MYSQL_RES *rows = mysql_store_result(connection);
		if (!rows)
		{
			errno = mysql_errno(connection);
			return false;
		}
		MYSQL_ROW row = mysql_fetch_row(rows);
		const bool found = row != nullptr;
		const bool has_payload = row && row[0];
		if (has_payload)
		{
			const unsigned long *lengths = mysql_fetch_lengths(rows);
			std::vector<player_item_snapshot> snapshots;
			if (!lengths ||
			    player_item_snapshot_list_decode(
				    reinterpret_cast<const uint8_t *>(row[0]), lengths[0],
				    &snapshots) != player_snapshot_codec_result::ok ||
			    snapshots.size() != 1 || snapshots[0].object_uid != uid ||
			    snapshots[0].vnum != payload.items[0].vnum ||
			    snapshots[0].type != ITEM_MONEY)
				*result_code = EBADMSG;
			else if (!std::equal(before.begin(), before.end(),
					     snapshots[0].values.begin()))
			{
				*result_code = ESTALE;
				if (failure_stage)
					*failure_stage =
						item_transfer_failure_stage::coin_payload_revision;
			}
		}
		mysql_free_result(rows);
		if (!found)
			*result_code = ENOENT;
		if (*result_code)
			return true;
		if (!has_payload)
		{
			// Existing piles get a baseline only from a unique payload belonging
			// to their recorded owner. A missing row is not evidence of consumption.
			std::string source;
			const auto &owner = payload.from_owner;
			switch (owner.type)
			{
			case item_owner_type::player:
				source = "player_items p WHERE p.pid=" + std::to_string(owner.id);
				break;
			case item_owner_type::room:
				source = "saved_items p WHERE p.room_vnum=" +
					 std::to_string(owner.id);
				break;
			case item_owner_type::corpse:
				source =
					"corpse_items p JOIN corpses c ON c.id=p.corpse_id "
					"JOIN player_data player ON player.name=c.player_name WHERE player.pid=" +
					std::to_string(owner.id >> 32) + " AND c.save_id=" +
					std::to_string(static_cast<uint32_t>(owner.id));
				break;
			case item_owner_type::locker:
				source = "locker_items p WHERE p.locker_id=" +
					 std::to_string(owner.id) +
					 " AND p.chest_id=" + std::to_string(owner.context_id);
				break;
			default:
				*result_code = EOPNOTSUPP;
				return true;
			}
			const std::string baseline =
				"SELECT p.value0,p.value1,p.value2,p.value3 FROM " + source +
				" AND p.vnum=" + std::to_string(payload.items[0].vnum) +
				" AND p.obj_uid=" + std::to_string(uid) + " FOR UPDATE";
			if (mysql_real_query(connection, baseline.data(), baseline.size()) != 0)
			{
				errno = mysql_errno(connection);
				return false;
			}
			rows = mysql_store_result(connection);
			if (!rows)
			{
				errno = mysql_errno(connection);
				return false;
			}
			row = mysql_fetch_row(rows);
			if (!row || mysql_num_rows(rows) != 1)
				*result_code = row ? EMSGSIZE : ENOENT;
			for (size_t index = 0; !*result_code && index < before.size(); ++index)
			{
				char *end = nullptr;
				errno = 0;
				const int64_t amount = row[index] ? strtoll(row[index], &end, 10) :
								    -1;
				if (!end || end == row[index] || *end || errno ||
				    amount != before[index])
				{
					*result_code = ESTALE;
					if (failure_stage)
						*failure_stage = item_transfer_failure_stage::
							coin_payload_revision;
				}
			}
			mysql_free_result(rows);
			if (*result_code)
				return true;
		}
	}
	item_transfer_failure_stage nested_stage = item_transfer_failure_stage::none;
	if (!item_transfer_repository_execute(connection, command, result, result_code,
					      mutation_applied, &nested_stage))
		return false;
	if (failure_stage && *result_code == ESTALE)
		*failure_stage = nested_stage;
	if (!*mutation_applied)
		return true;

	return payload.from_owner.type == item_owner_type::system ||
	       update_coin_payload(connection, payload, result->max_item_revision);
}

bool item_transfer_repository_destroy_owners(MYSQL *connection, const item_owner_identity *owners,
					     size_t owner_count)
{
	if (!connection || (!owners && owner_count))
	{
		errno = EINVAL;
		return false;
	}
	const item_owner_identity destruction = { item_owner_type::destruction, 0, 0 };
	for (size_t owner_index = 0; owner_index < owner_count; ++owner_index)
	{
		const item_owner_identity &owner = owners[owner_index];
		if (!item_owner_identity_valid(owner) ||
		    item_owner_identity_equal(owner, destruction))
		{
			errno = EINVAL;
			return false;
		}
		if (std::find_if(owners, owners + owner_index, [&](const item_owner_identity &prior)
				 { return item_owner_identity_equal(prior, owner); }) !=
		    owners + owner_index)
			continue;

		uint64_t from_revision = 0, to_revision = 0;
		if (!ensure_owner(connection, owner) || !ensure_owner(connection, destruction) ||
		    !lock_owner(connection, owner, &from_revision) ||
		    !lock_owner(connection, destruction, &to_revision))
			return false;
		std::vector<current_item> current;
		if (!load_owner(connection, owner, &current))
			return false;
		if (current.empty())
			continue;

		item_transfer_payload payload = {};
		payload.from_owner = owner;
		payload.to_owner = destruction;
		payload.reason = item_transfer_reason::destruction;
		payload.expected_from_revision = from_revision;
		payload.expected_to_revision = to_revision;
		payload.multi_root = true;
		payload.item_count = static_cast<uint16_t>(current.size());
		for (size_t item_index = 0; item_index < current.size(); ++item_index)
		{
			const current_item &item = current[item_index];
			payload.items[item_index] = { item.item_uid,
						      item.root_item_uid,
						      item.parent_item_uid,
						      item.item_revision,
						      item.vnum,
						      static_cast<item_custody_state>(item.state) };
		}

		critical_operation_id operation_id = {};
		critical_command command = {};
		if (!critical_operation_id_generate(&operation_id) ||
		    !item_transfer_command_build(&command, operation_id, payload,
						 critical_source_site::operator_repair,
						 critical_deadline_class::terminal))
		{
			errno = EINVAL;
			return false;
		}
		command.accepted_at_usec = wall_now_usec();
		item_transfer_result result = {};
		unsigned int result_code = 0;
		bool mutation_applied = false;
		if (!critical_command_repository_begin_inbox_in_transaction(connection, command) ||
		    !item_transfer_repository_execute(connection, command, &result, &result_code,
						      &mutation_applied) ||
		    result_code || !mutation_applied ||
		    !critical_command_repository_finish_item_transfer_in_transaction(
			    connection, command, result))
		{
			if (result_code)
				errno = static_cast<int>(result_code);
			return false;
		}
	}
	return true;
}

bool item_transfer_repository_revoke_roots_preserving_children(MYSQL *connection,
							       const uint64_t *item_uids,
							       size_t item_count)
{
	if (!connection || (!item_uids && item_count))
	{
		errno = EINVAL;
		return false;
	}
	const item_owner_identity destruction = { item_owner_type::destruction, 0, 0 };
	auto submit = [&](item_transfer_payload *payload) -> bool
	{
		if (!payload)
			return false;
		if (!ensure_owner(connection, payload->from_owner) ||
		    !ensure_owner(connection, payload->to_owner))
			return false;
		if (item_owner_identity_equal(payload->from_owner, payload->to_owner))
		{
			if (!lock_owner(connection, payload->from_owner,
					&payload->expected_from_revision))
				return false;
			payload->expected_to_revision = payload->expected_from_revision;
		}
		else if (owner_identity_less(payload->from_owner, payload->to_owner))
		{
			if (!lock_owner(connection, payload->from_owner,
					&payload->expected_from_revision) ||
			    !lock_owner(connection, payload->to_owner,
					&payload->expected_to_revision))
				return false;
		}
		else if (!lock_owner(connection, payload->to_owner,
				     &payload->expected_to_revision) ||
			 !lock_owner(connection, payload->from_owner,
				     &payload->expected_from_revision))
			return false;

		critical_operation_id operation_id = {};
		critical_command command = {};
		if (!critical_operation_id_generate(&operation_id) ||
		    !item_transfer_command_build(&command, operation_id, *payload,
						 critical_source_site::operator_repair,
						 critical_deadline_class::terminal))
		{
			errno = EINVAL;
			return false;
		}
		command.accepted_at_usec = wall_now_usec();
		item_transfer_result result = {};
		unsigned int result_code = 0;
		bool mutation_applied = false;
		if (!critical_command_repository_begin_inbox_in_transaction(connection, command) ||
		    !item_transfer_repository_execute(connection, command, &result, &result_code,
						      &mutation_applied) ||
		    result_code || !mutation_applied ||
		    !critical_command_repository_finish_item_transfer_in_transaction(
			    connection, command, result))
		{
			if (result_code)
				errno = static_cast<int>(result_code);
			return false;
		}
		return true;
	};

	for (size_t requested = 0; requested < item_count; ++requested)
	{
		if (!item_uids[requested])
			continue;
		current_item reward = {};
		bool found = false;
		if (!load_item(connection, item_uids[requested], &reward, &found))
			return false;
		if (!found || reward.state == static_cast<uint8_t>(item_custody_state::destroyed))
			continue;
		if (reward.state != static_cast<uint8_t>(item_custody_state::active))
		{
			errno = ESTALE;
			return false;
		}
		const item_owner_identity owner = { static_cast<item_owner_type>(reward.owner_type),
						    reward.owner_id, reward.owner_context_id };
		if (!item_owner_identity_valid(owner) ||
		    item_owner_identity_equal(owner, destruction))
		{
			errno = EINVAL;
			return false;
		}

		for (;;)
		{
			std::vector<current_item> tree;
			if (!load_root(connection, reward.root_item_uid, &tree))
				return false;
			auto direct_child = std::find_if(
				tree.begin(), tree.end(), [&](const current_item &candidate)
				{ return candidate.parent_item_uid == reward.item_uid; });
			if (direct_child == tree.end())
				break;

			item_transfer_payload reparent = {};
			reparent.from_owner = owner;
			reparent.to_owner = owner;
			reparent.reason = item_transfer_reason::operator_repair;
			reparent.reason_id = static_cast<int64_t>(reward.item_uid);
			reparent.selected_item_uid = direct_child->item_uid;
			reparent.target_parent_item_uid = reward.parent_item_uid;
			reparent.target_root_item_uid = reward.parent_item_uid ?
								reward.root_item_uid :
								direct_child->item_uid;
			if (reward.parent_item_uid)
			{
				auto parent = std::find_if(
					tree.begin(), tree.end(), [&](const current_item &candidate)
					{ return candidate.item_uid == reward.parent_item_uid; });
				if (parent == tree.end())
				{
					errno = ESTALE;
					return false;
				}
				reparent.expected_target_parent_revision = parent->item_revision;
			}
			for (const current_item &candidate : tree)
			{
				uint64_t ancestor = candidate.item_uid;
				bool selected = false;
				for (size_t depth = 0; depth <= tree.size(); ++depth)
				{
					if (ancestor == direct_child->item_uid)
					{
						selected = true;
						break;
					}
					auto parent = std::find_if(tree.begin(), tree.end(),
								   [&](const current_item &entry) {
									   return entry.item_uid ==
										  ancestor;
								   });
					if (parent == tree.end() || !parent->parent_item_uid)
						break;
					ancestor = parent->parent_item_uid;
				}
				if (!selected)
					continue;
				if (reparent.item_count == ITEM_TRANSFER_MAX_ITEMS)
				{
					errno = E2BIG;
					return false;
				}
				reparent.items[reparent.item_count++] = {
					candidate.item_uid,
					candidate.root_item_uid,
					candidate.parent_item_uid,
					candidate.item_revision,
					candidate.vnum,
					static_cast<item_custody_state>(candidate.state)
				};
			}
			if (!reparent.item_count || !submit(&reparent))
				return false;
		}

		if (!load_item(connection, reward.item_uid, &reward, &found) || !found)
			return false;
		item_transfer_payload retire = {};
		retire.from_owner = owner;
		retire.to_owner = destruction;
		retire.reason = item_transfer_reason::destruction;
		retire.reason_id = static_cast<int64_t>(reward.item_uid);
		retire.selected_item_uid = reward.item_uid;
		retire.item_count = 1;
		retire.items[0] = {
			reward.item_uid,      reward.root_item_uid, reward.parent_item_uid,
			reward.item_revision, reward.vnum,	    item_custody_state::active
		};
		if (!submit(&retire))
			return false;
	}
	return true;
}

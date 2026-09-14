#include "persistence/corpse_lifecycle_repository.h"

#include "core/defines.h"
#include "economy/collector_repository.h"
#include "economy/currency_repository.h"
#include "item/item_transfer_repository.h"
#include "world/vnum.obj.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <mysql.h>
#include <new>
#include <string>
#include <strings.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace
{
constexpr uint8_t artifact_not_in_game = 1;
constexpr uint8_t artifact_on_player = 3;
constexpr uint8_t artifact_on_ground = 4;
constexpr uint8_t artifact_on_corpse = 5;

struct corpse_identity
{
	uint32_t id = 0;
	uint64_t revision = 0;
	int32_t room_vnum = 0;
	std::string player_name;
};

struct physical_item
{
	uint32_t id = 0;
	uint32_t parent_id = 0;
	int32_t vnum = 0;
	int32_t weight = 0;
	int64_t adjusted_weight = 0;
	uint64_t extra_flags = 0;
	std::array<int32_t, 4> money = {};
	uint64_t item_uid = 0;
	uint64_t root_item_uid = 0;
	uint64_t parent_item_uid = 0;
	uint64_t item_revision = 0;
	bool money_item = false;
	bool transient = false;
	bool skipped = false;
	bool artifact = false;
};

struct artifact_state
{
	int32_t vnum = 0;
	uint64_t item_uid = 0;
	uint64_t item_revision = 0;
	uint64_t domain_revision = 0;
	int32_t bind_owner_pid = 0;
	int64_t bind_timer = 0;
};

struct wallet_plan
{
	bool active = false;
	uint32_t pid = 0;
	uint8_t racewar = 0;
	std::array<char, CURRENCY_ACCOUNT_NAME_MAX_BYTES + 1> account_name = {};
	std::array<int32_t, CURRENCY_DENOMINATION_COUNT> before = {};
	std::array<int32_t, CURRENCY_DENOMINATION_COUNT> after = {};
	uint64_t wallet_revision = 0;
	uint64_t bank_revision = 0;
	int64_t reason_id = 0;
};

struct owner_lock
{
	item_owner_identity owner = { item_owner_type::unknown, 0, 0 };
	uint64_t revision = 0;
};

bool execute(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()) == 0)
		return true;
	errno = static_cast<int>(mysql_errno(connection));
	return false;
}

bool parse_u64(const char *text, uint64_t *value)
{
	if (!text || !value)
		return false;
	char *end = nullptr;
	errno = 0;
	const unsigned long long parsed = strtoull(text, &end, 10);
	if (errno || !end || *end)
		return false;
	*value = parsed;
	return true;
}

bool parse_i64(const char *text, int64_t *value)
{
	if (!text || !value)
		return false;
	char *end = nullptr;
	errno = 0;
	const long long parsed = strtoll(text, &end, 10);
	if (errno || !end || *end)
		return false;
	*value = parsed;
	return true;
}

bool parse_u32(const char *text, uint32_t *value)
{
	uint64_t parsed = 0;
	if (!parse_u64(text, &parsed) || parsed > UINT32_MAX)
		return false;
	*value = static_cast<uint32_t>(parsed);
	return true;
}

bool parse_i32(const char *text, int32_t *value)
{
	int64_t parsed = 0;
	if (!parse_i64(text, &parsed) || parsed < INT32_MIN || parsed > INT32_MAX)
		return false;
	*value = static_cast<int32_t>(parsed);
	return true;
}

std::string operation_hex(const critical_operation_id &operation)
{
	static constexpr char hex[] = "0123456789abcdef";
	std::string value(operation.bytes.size() * 2, '0');
	for (size_t index = 0; index < operation.bytes.size(); ++index)
	{
		value[index * 2] = hex[operation.bytes[index] >> 4];
		value[index * 2 + 1] = hex[operation.bytes[index] & 15];
	}
	return value;
}

bool owner_less(const item_owner_identity &left, const item_owner_identity &right)
{
	if (left.type != right.type)
		return left.type < right.type;
	if (left.id != right.id)
		return left.id < right.id;
	return left.context_id < right.context_id;
}

physical_item *find_physical(std::vector<physical_item> *items, uint32_t id)
{
	const auto found = std::find_if(items->begin(), items->end(),
					[&](const physical_item &item) { return item.id == id; });
	return found == items->end() ? nullptr : &*found;
}

const physical_item *find_physical(const std::vector<physical_item> &items, uint32_t id)
{
	const auto found = std::find_if(items.begin(), items.end(),
					[&](const physical_item &item) { return item.id == id; });
	return found == items.end() ? nullptr : &*found;
}

bool load_physical_items(MYSQL *connection, uint32_t corpse_id,
			 std::vector<physical_item> *items, std::array<int32_t, 4> *money,
			 unsigned int *result_code)
{
	if (!execute(connection,
		     "SELECT id,COALESCE(container_id,0),vnum,weight,extra_flags,value0,value1,"
		     "value2,value3,COALESCE(obj_uid,0) FROM corpse_items WHERE corpse_id=" +
			     std::to_string(corpse_id) + " ORDER BY id FOR UPDATE"))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
	{
		errno = static_cast<int>(mysql_errno(connection));
		return false;
	}
	if (mysql_num_rows(rows) > ITEM_TRANSFER_MAX_ITEMS)
	{
		mysql_free_result(rows);
		*result_code = E2BIG;
		return true;
	}
	try
	{
		items->clear();
		items->reserve(static_cast<size_t>(mysql_num_rows(rows)));
		*money = {};
		MYSQL_ROW row = nullptr;
		while ((row = mysql_fetch_row(rows)) != nullptr)
		{
			physical_item item;
			if (!parse_u32(row[0], &item.id) || !item.id ||
			    !parse_u32(row[1], &item.parent_id) || !parse_i32(row[2], &item.vnum) ||
			    item.vnum <= 0 || !parse_i32(row[3], &item.weight) ||
			    !parse_u64(row[4], &item.extra_flags) ||
			    !parse_u64(row[9], &item.item_uid))
			{
				mysql_free_result(rows);
				*result_code = EILSEQ;
				return true;
			}
			for (size_t index = 0; index < item.money.size(); ++index)
				if (!parse_i32(row[index + 5], &item.money[index]) ||
				    item.money[index] < 0)
				{
					mysql_free_result(rows);
					*result_code = EILSEQ;
					return true;
				}
			item.money_item = item.vnum == VOBJ_COINS;
			item.transient = (item.extra_flags & ITEM_TRANSIENT) != 0;
			item.skipped = item.money_item || item.transient;
			item.artifact = !item.skipped && (item.extra_flags & ITEM_ARTIFACT) != 0;
			item.adjusted_weight = item.weight;
			if (item.money_item)
				for (size_t index = 0; index < money->size(); ++index)
				{
					if (item.money[index] > INT32_MAX - (*money)[index])
					{
						mysql_free_result(rows);
						*result_code = EOVERFLOW;
						return true;
					}
					(*money)[index] += item.money[index];
				}
			items->push_back(item);
		}
	}
	catch (const std::bad_alloc &)
	{
		mysql_free_result(rows);
		errno = ENOMEM;
		return false;
	}
	mysql_free_result(rows);

	std::unordered_set<uint32_t> ids;
	std::unordered_set<uint64_t> uids;
	try
	{
		ids.reserve(items->size());
		uids.reserve(items->size());
		for (const physical_item &item : *items)
		{
			if (!ids.insert(item.id).second || item.parent_id == item.id ||
			    (item.parent_id && !find_physical(*items, item.parent_id)) ||
			    (!item.skipped && (!item.item_uid || !uids.insert(item.item_uid).second)))
			{
				*result_code = EILSEQ;
				return true;
			}
		}
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return false;
	}

	for (physical_item &item : *items)
	{
		if (item.skipped)
			continue;
		const physical_item *current = &item;
		for (size_t depth = 0; depth <= items->size(); ++depth)
		{
			if (current->skipped)
			{
				*result_code = EILSEQ;
				return true;
			}
			if (!current->parent_id)
			{
				item.root_item_uid = current->item_uid;
				break;
			}
			current = find_physical(*items, current->parent_id);
			if (!current || depth == items->size())
			{
				*result_code = EILSEQ;
				return true;
			}
		}
		if (!item.root_item_uid)
		{
			*result_code = EILSEQ;
			return true;
		}
		if (item.parent_id)
		{
			const physical_item *parent = find_physical(*items, item.parent_id);
			if (!parent || parent->skipped || !parent->item_uid)
			{
				*result_code = EILSEQ;
				return true;
			}
			item.parent_item_uid = parent->item_uid;
		}
	}

	for (const physical_item &skipped : *items)
	{
		if (!skipped.skipped || !skipped.parent_id)
			continue;
		physical_item *parent = find_physical(items, skipped.parent_id);
		if (!parent || parent->skipped)
			continue;
		for (size_t depth = 0; parent && depth <= items->size(); ++depth)
		{
			parent->adjusted_weight -= skipped.weight;
			if (parent->adjusted_weight < INT32_MIN || parent->adjusted_weight > INT32_MAX)
			{
				*result_code = ERANGE;
				return true;
			}
			parent = parent->parent_id ? find_physical(items, parent->parent_id) : nullptr;
		}
	}
	return true;
}

bool build_transfer_entries(MYSQL *connection, uint64_t corpse_owner_id,
			    std::vector<physical_item> *physical,
			    item_transfer_payload *transfer, bool for_update,
			    unsigned int *result_code)
{
	std::vector<item_transfer_entry> entries;
	try
	{
		entries.reserve(physical->size());
		for (const physical_item &item : *physical)
			if (!item.skipped)
				entries.push_back({ item.item_uid, item.root_item_uid,
						    item.parent_item_uid, 0, item.vnum,
						    item_custody_state::active });
		std::sort(entries.begin(), entries.end(),
			  [](const item_transfer_entry &left, const item_transfer_entry &right)
			  { return left.item_uid < right.item_uid; });
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return false;
	}
	std::string sql =
		"SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),item_revision,vnum,state "
		"FROM item_current_owner WHERE owner_type=" +
		std::to_string(static_cast<unsigned int>(item_owner_type::corpse)) +
		" AND owner_id=" + std::to_string(corpse_owner_id) +
		" AND owner_context_id=0 ORDER BY item_uid";
	if (for_update)
		sql += " FOR UPDATE";
	if (!execute(connection, sql))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
	{
		errno = static_cast<int>(mysql_errno(connection));
		return false;
	}
	if (mysql_num_rows(rows) != entries.size())
	{
		mysql_free_result(rows);
		*result_code = ESTALE;
		return true;
	}
	MYSQL_ROW row = nullptr;
	size_t index = 0;
	while ((row = mysql_fetch_row(rows)) != nullptr)
	{
		uint64_t uid = 0, root = 0, parent = 0, revision = 0, state = 0;
		int32_t vnum = 0;
		if (index >= entries.size() || !parse_u64(row[0], &uid) ||
		    !parse_u64(row[1], &root) || !parse_u64(row[2], &parent) ||
		    !parse_u64(row[3], &revision) || !parse_i32(row[4], &vnum) ||
		    !parse_u64(row[5], &state) || uid != entries[index].item_uid ||
		    root != entries[index].root_item_uid || parent != entries[index].parent_item_uid ||
		    vnum != entries[index].vnum || state != static_cast<uint8_t>(item_custody_state::active) ||
		    !revision)
		{
			mysql_free_result(rows);
			*result_code = ESTALE;
			return true;
		}
		entries[index].expected_item_revision = revision;
		physical_item *item = nullptr;
		for (physical_item &candidate : *physical)
			if (candidate.item_uid == uid)
			{
				item = &candidate;
				break;
			}
		if (!item)
		{
			mysql_free_result(rows);
			*result_code = EILSEQ;
			return true;
		}
		item->item_revision = revision;
		++index;
	}
	mysql_free_result(rows);
	if (entries.size() > UINT16_MAX)
	{
		*result_code = E2BIG;
		return true;
	}
	transfer->item_count = static_cast<uint16_t>(entries.size());
	std::copy(entries.begin(), entries.end(), transfer->items.begin());
	return true;
}

bool load_corpse_identity(MYSQL *connection, const corpse_lifecycle_payload &payload,
			  uint64_t *catalog_revision, corpse_identity *identity,
			  unsigned int *result_code)
{
	if (!execute(connection,
		     "SELECT catalog_revision FROM corpse_catalog_state WHERE state_id=1 FOR UPDATE"))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	MYSQL_ROW row = rows ? mysql_fetch_row(rows) : nullptr;
	const bool catalog_ok = row && mysql_num_rows(rows) == 1 &&
				parse_u64(row[0], catalog_revision) && *catalog_revision &&
				*catalog_revision != UINT64_MAX;
	if (rows)
		mysql_free_result(rows);
	if (!catalog_ok)
	{
		if (mysql_errno(connection))
		{
			errno = static_cast<int>(mysql_errno(connection));
			return false;
		}
		*result_code = EILSEQ;
		return true;
	}

	const std::string query =
		"SELECT id,player_name,corpse_revision,room_vnum,COALESCE(value3,0) FROM corpses "
		"WHERE value3=" +
		std::to_string(payload.owner_pid) + " AND save_id=" +
		std::to_string(payload.save_id) + " FOR UPDATE";
	if (!execute(connection, query))
		return false;
	rows = mysql_store_result(connection);
	if (!rows)
	{
		errno = static_cast<int>(mysql_errno(connection));
		return false;
	}
	if (mysql_num_rows(rows) != 1 || !(row = mysql_fetch_row(rows)))
	{
		const bool missing = mysql_num_rows(rows) == 0;
		mysql_free_result(rows);
		*result_code = missing ? ENOENT : EILSEQ;
		return true;
	}
	uint32_t stored_owner = 0;
	const unsigned long *lengths = mysql_fetch_lengths(rows);
	const bool parsed = lengths && parse_u32(row[0], &identity->id) && identity->id && row[1] &&
			    lengths[1] <= CORPSE_LIFECYCLE_OWNER_NAME_MAX_BYTES &&
			    strlen(row[1]) == lengths[1] && parse_u64(row[2], &identity->revision) &&
			    identity->revision && parse_i32(row[3], &identity->room_vnum) &&
			    parse_u32(row[4], &stored_owner);
	if (parsed)
		identity->player_name.assign(row[1], lengths[1]);
	mysql_free_result(rows);
	if (!parsed || stored_owner != payload.owner_pid ||
	    strcasecmp(identity->player_name.c_str(), payload.owner_name.c_str()))
	{
		*result_code = EILSEQ;
		return true;
	}
	if (identity->revision != payload.expected_corpse_revision ||
	    identity->room_vnum != payload.room_vnum)
	{
		*result_code = ESTALE;
		return true;
	}
	return true;
}

bool prepare_transfer(const corpse_lifecycle_payload &payload, uint64_t corpse_owner_id,
		      item_transfer_payload *transfer, item_owner_identity *old_room,
		      unsigned int *result_code)
{
	*transfer = {};
	*old_room = { item_owner_type::unknown, 0, 0 };
	transfer->from_owner = { item_owner_type::corpse, corpse_owner_id, 0 };
	transfer->reason_id = static_cast<int64_t>(corpse_owner_id);
	transfer->multi_root = true;
	switch (payload.action)
	{
	case corpse_lifecycle_action::release:
		transfer->to_owner = { item_owner_type::room,
					       static_cast<uint64_t>(payload.room_vnum), 0 };
		transfer->reason = item_transfer_reason::player_drop;
		transfer->expected_to_revision = payload.expected_room_revision;
		break;
	case corpse_lifecycle_action::destroy:
		transfer->to_owner = { item_owner_type::destruction, 0, 0 };
		transfer->reason = item_transfer_reason::destruction;
		transfer->expected_to_revision = payload.expected_room_revision;
		break;
	case corpse_lifecycle_action::resurrect:
		transfer->to_owner = { item_owner_type::player, payload.destination_player_pid, 0 };
		transfer->reason = item_transfer_reason::corpse_loot;
		transfer->expected_to_revision = payload.expected_player_revision;
		*old_room = { item_owner_type::room, static_cast<uint64_t>(payload.old_room_vnum), 0 };
		break;
	case corpse_lifecycle_action::raise_follower:
		transfer->to_owner = { item_owner_type::player, payload.destination_player_pid, 0 };
		transfer->reason = item_transfer_reason::corpse_loot;
		transfer->expected_to_revision = payload.expected_player_revision;
		break;
	case corpse_lifecycle_action::release_nested:
		transfer->to_owner = payload.destination_player_pid ?
			item_owner_identity{ item_owner_type::player,
					     payload.destination_player_pid, 0 } :
			item_owner_identity{ item_owner_type::room,
					     static_cast<uint64_t>(payload.room_vnum), 0 };
		transfer->reason = payload.destination_player_pid ?
					   item_transfer_reason::corpse_loot :
					   item_transfer_reason::player_drop;
		transfer->expected_to_revision = payload.destination_player_pid ?
						 payload.expected_player_revision :
						 payload.expected_room_revision;
		transfer->target_root_item_uid = payload.target_root_item_uid;
		transfer->target_parent_item_uid = payload.target_parent_item_uid;
		transfer->expected_target_parent_revision =
			payload.expected_target_parent_revision;
		break;
	case corpse_lifecycle_action::upsert:
	case corpse_lifecycle_action::remove:
		*result_code = EOPNOTSUPP;
		return true;
	}
	if (!item_owner_identity_valid(transfer->from_owner) ||
	    !item_owner_identity_valid(transfer->to_owner) ||
	    (old_room->type != item_owner_type::unknown && !item_owner_identity_valid(*old_room)))
	{
		*result_code = EINVAL;
		return true;
	}
	return true;
}

bool prepare_physical_destination(MYSQL *connection, const corpse_lifecycle_payload &payload,
				  const item_transfer_payload &transfer,
				  const std::vector<physical_item> &items,
				  uint32_t *external_parent_id,
				  unsigned int *result_code)
{
	*external_parent_id = 0;
	if (transfer.to_owner.type == item_owner_type::destruction)
		return true;
	std::string uids;
	for (const physical_item &item : items)
		if (!item.skipped)
			uids += (uids.empty() ? "" : ",") + std::to_string(item.item_uid);
	if (!uids.empty())
	{
		const std::string table = transfer.to_owner.type == item_owner_type::player ?
						  "player_items" :
						  "saved_items";
		if (!execute(connection, "SELECT obj_uid FROM " + table + " WHERE obj_uid IN (" +
						uids + ") FOR UPDATE"))
			return false;
		MYSQL_RES *rows = mysql_store_result(connection);
		if (!rows)
		{
			errno = static_cast<int>(mysql_errno(connection));
			return false;
		}
		const bool duplicate = mysql_num_rows(rows) != 0;
		mysql_free_result(rows);
		if (duplicate)
		{
			*result_code = EEXIST;
			return true;
		}
	}
	if (!payload.target_parent_item_uid)
		return true;
	const std::string table = transfer.to_owner.type == item_owner_type::player ?
					  "player_items" :
					  "saved_items";
	const std::string owner_clause =
		transfer.to_owner.type == item_owner_type::player ?
			"pid=" + std::to_string(payload.destination_player_pid) :
			"room_vnum=" + std::to_string(payload.room_vnum);
	if (!execute(connection, "SELECT id FROM " + table + " WHERE " + owner_clause +
				     " AND obj_uid=" +
				     std::to_string(payload.target_parent_item_uid) + " FOR UPDATE"))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	MYSQL_ROW row = rows ? mysql_fetch_row(rows) : nullptr;
	const bool ok = row && mysql_num_rows(rows) == 1 &&
			parse_u32(row[0], external_parent_id) && *external_parent_id;
	if (rows)
		mysql_free_result(rows);
	if (!ok)
	{
		if (mysql_errno(connection))
		{
			errno = static_cast<int>(mysql_errno(connection));
			return false;
		}
		*result_code = ESTALE;
	}
	return true;
}

bool prepare_artifacts(MYSQL *connection, const corpse_lifecycle_payload &payload,
		       const std::vector<physical_item> &items,
		       std::vector<artifact_state> *artifacts, unsigned int *result_code)
{
	std::unordered_set<int32_t> vnums;
	try
	{
		artifacts->clear();
		for (const physical_item &item : items)
		{
			if (!item.artifact)
				continue;
			if (!vnums.insert(item.vnum).second)
			{
				*result_code = EILSEQ;
				return true;
			}
			const std::string sql =
				"SELECT s.revision,s.bind_owner_pid,s.bind_timer_epoch,"
				"s.item_uid IS NULL,COALESCE(s.item_uid,0),s.item_revision IS NULL,"
				"COALESCE(s.item_revision,0),a.owned,a.locType,COALESCE(a.location,0) "
				"FROM artifact_domain_state s JOIN artifacts a ON a.vnum=s.vnum "
				"WHERE s.vnum=" +
				std::to_string(item.vnum) + " FOR UPDATE";
			if (!execute(connection, sql))
				return false;
			MYSQL_RES *rows = mysql_store_result(connection);
			MYSQL_ROW row = rows ? mysql_fetch_row(rows) : nullptr;
			uint64_t revision = 0, item_uid_null = 0, stored_uid = 0,
				 item_revision_null = 0, stored_item_revision = 0, legacy_type = 0;
			int64_t bind_owner = 0, bind_timer = 0, legacy_location = 0;
			const bool ok =
				row && mysql_num_rows(rows) == 1 && parse_u64(row[0], &revision) &&
				revision != UINT64_MAX && parse_i64(row[1], &bind_owner) &&
				bind_owner >= INT32_MIN && bind_owner <= INT32_MAX &&
				parse_i64(row[2], &bind_timer) && parse_u64(row[3], &item_uid_null) &&
				parse_u64(row[4], &stored_uid) &&
				parse_u64(row[5], &item_revision_null) &&
				parse_u64(row[6], &stored_item_revision) && row[7] &&
				!strcasecmp(row[7], "Y") && parse_u64(row[8], &legacy_type) &&
				legacy_type == artifact_on_corpse &&
				parse_i64(row[9], &legacy_location) &&
				legacy_location == static_cast<int64_t>(payload.owner_pid) &&
				(item_uid_null || stored_uid == item.item_uid) &&
				(item_revision_null || stored_item_revision == item.item_revision);
			if (rows)
				mysql_free_result(rows);
			if (!ok)
			{
				if (mysql_errno(connection))
				{
					errno = static_cast<int>(mysql_errno(connection));
					return false;
				}
				*result_code = ESTALE;
				return true;
			}
			artifacts->push_back({ item.vnum, item.item_uid, item.item_revision, revision,
					       static_cast<int32_t>(bind_owner), bind_timer });
		}
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return false;
	}
	return true;
}

bool escape_text(MYSQL *connection, const char *text, size_t length, std::string *escaped)
{
	try
	{
		escaped->assign(length * 2 + 1, '\0');
		const unsigned long result = mysql_real_escape_string(
			connection, escaped->data(), text, static_cast<unsigned long>(length));
		escaped->resize(result);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return false;
	}
}

bool prepare_wallet(MYSQL *connection, const corpse_lifecycle_payload &payload,
		    const std::array<int32_t, 4> &corpse_money, wallet_plan *plan,
		    unsigned int *result_code)
{
	const bool player_action = payload.action == corpse_lifecycle_action::resurrect ||
				   payload.action == corpse_lifecycle_action::raise_follower ||
				   (payload.action == corpse_lifecycle_action::release_nested &&
				    payload.destination_player_pid);
	*plan = {};
	if (!player_action)
		return true;
	if (!execute(connection,
		     "SELECT account_name,racewar,copper,silver,gold,platinum,wallet_revision "
		     "FROM player_data WHERE pid=" +
			     std::to_string(payload.destination_player_pid) + " FOR UPDATE"))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	MYSQL_ROW row = rows ? mysql_fetch_row(rows) : nullptr;
	const unsigned long *lengths = row ? mysql_fetch_lengths(rows) : nullptr;
	uint64_t racewar = 0, wallet_revision = 0;
	std::array<int64_t, CURRENCY_DENOMINATION_COUNT> wallet = {};
	bool ok = row && mysql_num_rows(rows) == 1 && lengths && row[0] &&
		  lengths[0] <= CURRENCY_ACCOUNT_NAME_MAX_BYTES && strlen(row[0]) == lengths[0] &&
		  parse_u64(row[1], &racewar) && racewar <= UINT8_MAX;
	for (size_t index = 0; ok && index < wallet.size(); ++index)
		ok = parse_i64(row[index + 2], &wallet[index]) && wallet[index] >= 0 &&
		     wallet[index] <= INT32_MAX;
	ok = ok && parse_u64(row[6], &wallet_revision) &&
	     wallet_revision == payload.expected_wallet_revision && wallet_revision != UINT64_MAX;
	if (ok)
		for (size_t index = 0; index < wallet.size(); ++index)
			ok = wallet[index] == payload.money[index];
	if (ok)
	{
		plan->pid = payload.destination_player_pid;
		plan->racewar = static_cast<uint8_t>(racewar);
		std::copy_n(row[0], lengths[0], plan->account_name.begin());
		plan->account_name[lengths[0]] = '\0';
		plan->wallet_revision = wallet_revision;
		plan->reason_id = static_cast<int64_t>(
			item_corpse_owner_id(payload.owner_pid, payload.save_id));
		for (size_t index = 0; index < wallet.size(); ++index)
			plan->before[index] = static_cast<int32_t>(wallet[index]);
	}
	if (rows)
		mysql_free_result(rows);
	if (!ok)
	{
		if (mysql_errno(connection))
		{
			errno = static_cast<int>(mysql_errno(connection));
			return false;
		}
		*result_code = ESTALE;
		return true;
	}

	std::string escaped_account;
	const size_t account_length = strnlen(plan->account_name.data(), plan->account_name.size());
	if (!escape_text(connection, plan->account_name.data(), account_length, &escaped_account))
		return false;
	if (!execute(connection,
		     "INSERT IGNORE INTO account_banks(account_name,racewar) VALUES('" +
			     escaped_account + "'," + std::to_string(plan->racewar) + ")") ||
	    !execute(connection,
		     "SELECT id,bank_copper,bank_silver,bank_gold,bank_platinum,bank_revision "
		     "FROM account_banks WHERE account_name='" +
			     escaped_account + "' AND racewar=" + std::to_string(plan->racewar) +
			     " FOR UPDATE"))
		return false;
	rows = mysql_store_result(connection);
	row = rows ? mysql_fetch_row(rows) : nullptr;
	uint32_t bank_id = 0;
	uint64_t bank_revision = 0;
	std::array<uint64_t, CURRENCY_DENOMINATION_COUNT> bank = {};
	ok = row && mysql_num_rows(rows) == 1 && parse_u32(row[0], &bank_id) && bank_id;
	for (size_t index = 0; ok && index < bank.size(); ++index)
		ok = parse_u64(row[index + 1], &bank[index]) && bank[index] <= INT_MAX;
	ok = ok && parse_u64(row[5], &bank_revision) && bank_revision != UINT64_MAX;
	if (rows)
		mysql_free_result(rows);
	if (!ok)
	{
		if (mysql_errno(connection))
		{
			errno = static_cast<int>(mysql_errno(connection));
			return false;
		}
		*result_code = EILSEQ;
		return true;
	}
	plan->bank_revision = bank_revision;
	for (size_t index = 0; index < plan->after.size(); ++index)
	{
		const int64_t replacement =
			payload.action == corpse_lifecycle_action::resurrect ?
				corpse_money[index] :
				static_cast<int64_t>(plan->before[index]) + corpse_money[index];
		if (replacement < 0 || replacement > INT32_MAX)
		{
			*result_code = EOVERFLOW;
			return true;
		}
		plan->after[index] = static_cast<int32_t>(replacement);
	}
	plan->active = true;
	return true;
}

owner_lock *find_owner_lock(std::vector<owner_lock> *locks,
			    const item_owner_identity &owner)
{
	const auto found = std::find_if(locks->begin(), locks->end(), [&](const owner_lock &entry)
					{ return item_owner_identity_equal(entry.owner, owner); });
	return found == locks->end() ? nullptr : &*found;
}

bool lock_transfer_owners(MYSQL *connection, item_transfer_payload *transfer,
			  const item_owner_identity &old_room,
			  const corpse_lifecycle_payload &payload,
			  std::vector<owner_lock> *locks, unsigned int *result_code)
{
	try
	{
		locks->clear();
		locks->push_back({ transfer->from_owner, 0 });
		locks->push_back({ transfer->to_owner, 0 });
		if (old_room.type != item_owner_type::unknown)
			locks->push_back({ old_room, 0 });
		std::sort(locks->begin(), locks->end(), [](const owner_lock &left, const owner_lock &right)
			  { return owner_less(left.owner, right.owner); });
		if (std::adjacent_find(locks->begin(), locks->end(), [](const owner_lock &left,
								 const owner_lock &right) {
			    return item_owner_identity_equal(left.owner, right.owner);
			}) != locks->end())
		{
			*result_code = EINVAL;
			return true;
		}
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return false;
	}
	for (owner_lock &entry : *locks)
		if (!item_transfer_repository_ensure_owner(connection, entry.owner))
			return false;
	for (owner_lock &entry : *locks)
		if (!item_transfer_repository_lock_owner(connection, entry.owner, &entry.revision))
			return false;
	owner_lock *source = find_owner_lock(locks, transfer->from_owner);
	owner_lock *destination = find_owner_lock(locks, transfer->to_owner);
	owner_lock *room = old_room.type == item_owner_type::unknown ?
				   nullptr :
				   find_owner_lock(locks, old_room);
	if (!source || !destination || destination->revision != transfer->expected_to_revision ||
	    (room && room->revision != payload.expected_room_revision))
	{
		*result_code = ESTALE;
		return true;
	}
	if (source->revision == UINT64_MAX || destination->revision == UINT64_MAX ||
	    (room && room->revision == UINT64_MAX))
	{
		*result_code = ERANGE;
		return true;
	}
	transfer->expected_from_revision = source->revision;
	return true;
}

bool apply_wallet(MYSQL *connection, const critical_command &command, const wallet_plan &plan,
		  currency_command_result *currency, unsigned int *result_code)
{
	if (!plan.active)
	{
		*currency = {};
		return true;
	}
	currency_command_payload payload = {};
	payload.pid = plan.pid;
	payload.racewar = plan.racewar;
	payload.reason = currency_reason_type::corpse_lifecycle;
	payload.reason_id = plan.reason_id;
	payload.account_name = plan.account_name;
	for (size_t index = 0; index < CURRENCY_DENOMINATION_COUNT; ++index)
		payload.wallet_delta.amount[index] =
			static_cast<int64_t>(plan.after[index]) - plan.before[index];
	critical_command wallet_command = {};
	if (!currency_command_build(&wallet_command, command.operation_id, payload,
				    plan.wallet_revision, plan.bank_revision, command.source_site,
				    command.deadline_class))
	{
		errno = EINVAL;
		return false;
	}
	wallet_command.accepted_at_usec = command.accepted_at_usec;
	bool mutation = false;
	if (!currency_repository_execute(connection, wallet_command, currency, result_code,
					  &mutation))
		return false;
	if (*result_code)
		return true;
	if (!mutation || currency->wallet_revision != plan.wallet_revision + 1 ||
	    currency->bank_revision != plan.bank_revision + 1)
	{
		errno = EILSEQ;
		return false;
	}
	for (size_t index = 0; index < CURRENCY_DENOMINATION_COUNT; ++index)
		if (currency->wallet.amount[index] != plan.after[index])
		{
			errno = EILSEQ;
			return false;
		}
	return true;
}

bool apply_artifacts(MYSQL *connection, const corpse_lifecycle_payload &payload,
		     const std::vector<artifact_state> &artifacts)
{
	const bool destroyed = payload.action == corpse_lifecycle_action::destroy;
	const bool player = payload.action == corpse_lifecycle_action::resurrect ||
			    payload.action == corpse_lifecycle_action::raise_follower ||
			    (payload.action == corpse_lifecycle_action::release_nested &&
			     payload.destination_player_pid);
	const uint8_t owned = destroyed ? 0 : 1;
	const uint8_t location_type = destroyed ? artifact_not_in_game :
				      player ? artifact_on_player : artifact_on_ground;
	const int32_t location = destroyed ? -1 :
				 player ? static_cast<int32_t>(payload.destination_player_pid) :
					  payload.room_vnum;
	for (const artifact_state &artifact : artifacts)
	{
		const int32_t bind_owner = destroyed ? -1 : artifact.bind_owner_pid;
		const int64_t bind_timer = destroyed ? 0 : artifact.bind_timer;
		if (artifact.item_revision == UINT64_MAX || artifact.domain_revision == UINT64_MAX)
		{
			errno = ERANGE;
			return false;
		}
		if (!execute(connection,
			     "UPDATE artifact_domain_state SET owned=" + std::to_string(owned) +
				     ",loc_type=" + std::to_string(location_type) +
				     ",location=" + std::to_string(location) +
				     ",bind_owner_pid=" + std::to_string(bind_owner) +
				     ",bind_timer_epoch=" + std::to_string(bind_timer) +
				     ",item_uid=" + std::to_string(artifact.item_uid) +
				     ",item_revision=" +
				     std::to_string(artifact.item_revision + 1) + ",revision=" +
				     std::to_string(artifact.domain_revision + 1) + " WHERE vnum=" +
				     std::to_string(artifact.vnum) + " AND revision=" +
				     std::to_string(artifact.domain_revision)) ||
		    mysql_affected_rows(connection) != 1 ||
		    !execute(connection,
			     "UPDATE artifacts SET owned='" + std::string(owned ? "Y" : "N") +
				     "',locType=" + std::to_string(location_type) +
				     ",location=" + std::to_string(location) +
				     ",lastUpdate=CURRENT_TIMESTAMP(6) WHERE vnum=" +
				     std::to_string(artifact.vnum) + " AND owned='Y' AND locType=" +
				     std::to_string(artifact_on_corpse) + " AND location=" +
				     std::to_string(payload.owner_pid)) ||
		    mysql_affected_rows(connection) != 1 ||
		    !execute(connection,
			     "UPDATE artifacts_mortal SET owned='" +
				     std::string(owned ? "Y" : "N") + ",locType=" +
				     std::to_string(location_type) + ",location=" +
				     std::to_string(location) + " WHERE vnum=" +
				     std::to_string(artifact.vnum)))
			return false;
		if (destroyed &&
		    !execute(connection,
			     "INSERT INTO artifact_bind(vnum,owner_pid,timer) VALUES(" +
				     std::to_string(artifact.vnum) +
				     ",-1,0) ON DUPLICATE KEY UPDATE owner_pid=-1,timer=0"))
			return false;
	}
	return true;
}

bool copy_related_rows(MYSQL *connection, bool player, uint32_t source_id, uint32_t target_id)
{
	const std::string affects = player ? "player_item_affects" : "saved_item_affects";
	const std::string extra = player ? "player_item_extra_descr" : "saved_item_extra_descr";
	return execute(connection,
		       "INSERT INTO " + affects +
			       "(item_id,location,modifier) SELECT " + std::to_string(target_id) +
			       ",location,modifier FROM corpse_item_affects WHERE item_id=" +
			       std::to_string(source_id)) &&
	       execute(connection,
		       "INSERT INTO " + extra +
			       "(item_id,keyword,description) SELECT " +
			       std::to_string(target_id) +
			       ",keyword,description FROM corpse_item_extra_descr WHERE item_id=" +
			       std::to_string(source_id));
}

bool materialize_items(MYSQL *connection, const critical_command &command,
		       const corpse_lifecycle_payload &payload,
		       const item_transfer_payload &transfer,
		       const std::vector<physical_item> &items, uint32_t external_parent_id)
{
	if (transfer.to_owner.type == item_owner_type::destruction)
		return true;
	const bool player = transfer.to_owner.type == item_owner_type::player;
	std::unordered_map<uint32_t, uint32_t> mapped;
	std::unordered_set<uint32_t> completed;
	try
	{
		mapped.reserve(items.size());
		completed.reserve(items.size());
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return false;
	}
	const std::string operation = operation_hex(command.operation_id);
	size_t durable_count = 0;
	for (const physical_item &item : items)
		durable_count += item.skipped ? 0 : 1;
	while (completed.size() < durable_count)
	{
		bool progressed = false;
		for (const physical_item &item : items)
		{
			if (item.skipped || completed.contains(item.id))
				continue;
			uint32_t parent_id = 0;
			if (item.parent_id)
			{
				const auto parent = mapped.find(item.parent_id);
				if (parent == mapped.end())
					continue;
				parent_id = parent->second;
			}
			else
				parent_id = external_parent_id;
			const std::string parent = parent_id ? std::to_string(parent_id) : "NULL";
			std::string sql;
			if (player)
			{
				sql =
					"INSERT INTO player_items(pid,vnum,equip_slot,container_id,quantity,"
					"weight,cost,timer,extra_flags,wear_flags,item_type,value0,value1,"
					"value2,value3,value4,value5,value6,value7,name,short_descr,description,"
					"action_descr,bitvector1,bitvector2,bitvector3,bitvector4,bitvector5,"
					"obj_uid,item_condition,item_material) SELECT " +
					std::to_string(payload.destination_player_pid) + ",vnum,0," +
					parent + ",quantity," + std::to_string(item.adjusted_weight) +
					",cost,timer,extra_flags,wear_flags,item_type,value0,value1,value2,"
					"value3,value4,value5,value6,value7,name,short_descr,description,"
					"action_descr,bitvector1,bitvector2,bitvector3,bitvector4,bitvector5,"
					"obj_uid,item_condition,item_material FROM corpse_items WHERE id=" +
					std::to_string(item.id);
			}
			else
			{
				const std::string key =
					"corpse-" + operation + "-" + std::to_string(item.id);
				sql =
					"INSERT INTO saved_items(item_key,room_vnum,vnum,container_id,quantity,"
					"weight,cost,timer,extra_flags,wear_flags,item_type,value0,value1,"
					"value2,value3,value4,value5,value6,value7,name,short_descr,description,"
					"action_descr,obj_uid,item_material,bitvector1,bitvector2,bitvector3,"
					"bitvector4,bitvector5) SELECT '" +
					key + "'," + std::to_string(payload.room_vnum) + ",vnum," +
					parent + ",quantity," + std::to_string(item.adjusted_weight) +
					",cost,timer,extra_flags,wear_flags,item_type,value0,value1,value2,"
					"value3,value4,value5,value6,value7,name,short_descr,description,"
					"action_descr,obj_uid,item_material,bitvector1,bitvector2,bitvector3,"
					"bitvector4,bitvector5 FROM corpse_items WHERE id=" +
					std::to_string(item.id);
			}
			if (!execute(connection, sql) || mysql_affected_rows(connection) != 1)
				return false;
			const uint64_t inserted = mysql_insert_id(connection);
			if (!inserted || inserted > UINT32_MAX ||
			    !copy_related_rows(connection, player, item.id,
					       static_cast<uint32_t>(inserted)))
			{
				errno = inserted > UINT32_MAX ? ERANGE : EIO;
				return false;
			}
			mapped.emplace(item.id, static_cast<uint32_t>(inserted));
			completed.insert(item.id);
			progressed = true;
		}
		if (!progressed)
		{
			errno = EILSEQ;
			return false;
		}
	}
	return true;
}

bool advance_empty_transfer(MYSQL *connection, const item_transfer_payload &transfer,
			    std::vector<owner_lock> *locks, item_transfer_result *result)
{
	owner_lock *source = find_owner_lock(locks, transfer.from_owner);
	owner_lock *destination = find_owner_lock(locks, transfer.to_owner);
	if (!source || !destination ||
	    !item_transfer_repository_advance_owner(connection, source->owner, source->revision) ||
	    !item_transfer_repository_advance_owner(connection, destination->owner,
						   destination->revision))
		return false;
	*result = { 0, 0, source->revision + 1, destination->revision + 1, 0, 0 };
	return true;
}

bool rollback_domain(MYSQL *connection)
{
	return execute(connection, "ROLLBACK TO SAVEPOINT corpse_lifecycle_domain") &&
	       execute(connection, "RELEASE SAVEPOINT corpse_lifecycle_domain");
}

bool finish_corpse(MYSQL *connection, const corpse_identity &identity,
		   uint64_t catalog_revision)
{
	return execute(connection,
		       "DELETE FROM corpses WHERE id=" + std::to_string(identity.id) +
			       " AND corpse_revision=" + std::to_string(identity.revision)) &&
	       mysql_affected_rows(connection) == 1 &&
	       execute(connection,
		       "UPDATE corpse_catalog_state SET catalog_revision=" +
			       std::to_string(catalog_revision + 1) +
			       " WHERE state_id=1 AND catalog_revision=" +
			       std::to_string(catalog_revision)) &&
	       mysql_affected_rows(connection) == 1;
}
} // namespace

bool corpse_lifecycle_repository_execute(
	MYSQL *connection, const critical_command &command, corpse_lifecycle_result *result,
	unsigned int *result_code, bool *mutation_applied, uint64_t *collector_revision,
	std::vector<collector_command_result> *collector_events)
{
	if (!connection || !result || !result_code || !mutation_applied || !collector_revision ||
	    !collector_events)
	{
		errno = EINVAL;
		return false;
	}
	corpse_lifecycle_payload payload = {};
	if (!corpse_lifecycle_command_decode_payload(command, &payload))
	{
		errno = EINVAL;
		return false;
	}
	*result = {};
	*result_code = 0;
	*mutation_applied = false;
	*collector_revision = 0;
	collector_events->clear();

	uint64_t catalog_revision = 0;
	corpse_identity identity;
	if (!load_corpse_identity(connection, payload, &catalog_revision, &identity, result_code))
		return false;
	if (*result_code)
		return true;
	const uint64_t corpse_owner_id = item_corpse_owner_id(payload.owner_pid, payload.save_id);
	item_transfer_payload transfer = {};
	item_owner_identity old_room = { item_owner_type::unknown, 0, 0 };
	if (!prepare_transfer(payload, corpse_owner_id, &transfer, &old_room, result_code))
		return false;
	if (*result_code)
		return true;

	std::vector<physical_item> physical;
	std::array<int32_t, 4> corpse_money = {};
	if (!load_physical_items(connection, identity.id, &physical, &corpse_money, result_code))
		return false;
	if (*result_code)
		return true;
	if (!build_transfer_entries(connection, corpse_owner_id, &physical, &transfer, false,
				    result_code))
		return false;
	if (*result_code)
		return true;

	collector_item_boundary_repository_plan collector_plan;
	if (transfer.item_count &&
	    !collector_repository_prepare_item_boundary(connection, transfer, &collector_plan,
						       result_code))
		return false;
	if (*result_code)
		return true;

	uint32_t external_parent_id = 0;
	std::vector<artifact_state> artifacts;
	if (!prepare_physical_destination(connection, payload, transfer, physical,
					  &external_parent_id, result_code))
		return false;
	if (*result_code)
		return true;
	if (!prepare_artifacts(connection, payload, physical, &artifacts, result_code))
		return false;
	if (*result_code)
		return true;

	if (!execute(connection, "SAVEPOINT corpse_lifecycle_domain"))
		return false;
	wallet_plan wallet;
	if (!prepare_wallet(connection, payload, corpse_money, &wallet, result_code))
		return false;
	if (*result_code)
		return rollback_domain(connection);

	std::vector<owner_lock> owners;
	if (!lock_transfer_owners(connection, &transfer, old_room, payload, &owners, result_code))
		return false;
	if (*result_code)
		return rollback_domain(connection);
	if (!build_transfer_entries(connection, corpse_owner_id, &physical, &transfer, true,
				    result_code))
		return false;
	if (*result_code)
		return rollback_domain(connection);

	item_transfer_result transfer_result = {};
	if (transfer.item_count)
	{
		critical_command item_command = {};
		if (!item_transfer_command_build(&item_command, command.operation_id, transfer,
					 command.source_site, command.deadline_class))
		{
			errno = EINVAL;
			return false;
		}
		item_command.accepted_at_usec = command.accepted_at_usec;
		bool item_mutation = false;
		if (!item_transfer_repository_execute(connection, item_command, &transfer_result,
						      result_code, &item_mutation))
			return false;
		if (*result_code)
			return rollback_domain(connection);
		if (!item_mutation)
		{
			errno = EILSEQ;
			return false;
		}
	}
	else if (!advance_empty_transfer(connection, transfer, &owners, &transfer_result))
		return false;

	uint64_t room_owner_revision = 0;
	if (old_room.type != item_owner_type::unknown)
	{
		owner_lock *room = find_owner_lock(&owners, old_room);
		if (!room ||
		    !item_transfer_repository_advance_owner(connection, room->owner, room->revision))
			return false;
		room_owner_revision = room->revision + 1;
	}

	if (!collector_plan.entries.empty() &&
	    !collector_repository_apply_item_boundary(connection, command, collector_plan,
						     collector_revision, collector_events))
		return false;
	currency_command_result currency = {};
	if (!apply_wallet(connection, command, wallet, &currency, result_code))
		return false;
	if (*result_code)
		return rollback_domain(connection);
	if (!apply_artifacts(connection, payload, artifacts) ||
	    !materialize_items(connection, command, payload, transfer, physical,
			       external_parent_id) ||
	    !finish_corpse(connection, identity, catalog_revision) ||
	    !execute(connection, "RELEASE SAVEPOINT corpse_lifecycle_domain"))
		return false;

	result->owner_pid = payload.owner_pid;
	result->save_id = payload.save_id;
	result->action = payload.action;
	result->catalog_revision = catalog_revision + 1;
	result->corpse_owner_revision = transfer_result.from_owner_revision;
	result->max_item_revision = transfer_result.max_item_revision;
	result->item_count = transfer_result.item_count;
	result->collector_catalog_changed = !collector_events->empty();
	if (transfer.to_owner.type == item_owner_type::player)
	{
		result->player_owner_revision = transfer_result.to_owner_revision;
		result->wallet_revision = currency.wallet_revision;
		result->bank_revision = currency.bank_revision;
		for (size_t index = 0; index < result->wallet.size(); ++index)
			result->wallet[index] = static_cast<int32_t>(currency.wallet.amount[index]);
	}
	else
		result->room_owner_revision = transfer_result.to_owner_revision;
	if (payload.action == corpse_lifecycle_action::resurrect)
		result->room_owner_revision = room_owner_revision;
	*mutation_applied = true;
	return true;
}

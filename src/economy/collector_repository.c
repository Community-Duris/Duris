#include "economy/collector_repository.h"

#include "economy/collector_custody_boundary.h"
#include "economy/collector_eligibility.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <limits>
#include <mysql.h>
#include <new>
#include <set>
#include <sstream>
#include <string>
#include <strings.h>
#include <unordered_set>
#include <vector>

namespace
{
constexpr std::array<int64_t, CURRENCY_DENOMINATION_COUNT> COIN_VALUES = { 1, 10, 100, 1000 };

struct catalog_state
{
	uint64_t revision = 0;
	uint64_t next_listing = 0;
};

using listing_state = collector_listing_detail;

struct authority_item
{
	uint64_t uid = 0;
	uint64_t root = 0;
	uint64_t parent = 0;
	item_owner_identity owner = { item_owner_type::unknown, 0, 0 };
	uint64_t revision = 0;
	int32_t vnum = 0;
	item_custody_state state = item_custody_state::absent;
};

struct physical_item
{
	uint64_t id = 0;
	uint64_t parent_id = 0;
	uint64_t uid = 0;
	int32_t vnum = 0;
	int64_t weight = 0;
	int64_t cost = 0;
	int16_t condition = 100;
	uint16_t quantity = 0;
};

struct wallet_state
{
	uint32_t pid = 0;
	uint32_t bank_id = 0;
	currency_vector wallet = {};
	currency_vector bank = {};
	uint64_t wallet_revision = 0;
	uint64_t bank_revision = 0;
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
	if (!text || !value || *text == '-')
		return false;
	char *end = nullptr;
	errno = 0;
	const unsigned long long parsed = strtoull(text, &end, 10);
	if (errno || !end || end == text || *end)
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
	if (errno || !end || end == text || *end)
		return false;
	*value = parsed;
	return true;
}

std::string escape(MYSQL *connection, const char *value, size_t size)
{
	std::string escaped(size * 2 + 1, '\0');
	const unsigned long length = mysql_real_escape_string(connection, escaped.data(),
							      value ? value : "", value ? size : 0);
	escaped.resize(length);
	return escaped;
}

std::string quote(MYSQL *connection, const std::string &value)
{
	return "'" + escape(connection, value.data(), value.size()) + "'";
}

std::string hex_encode(const uint8_t *bytes, size_t size)
{
	static constexpr char HEX[] = "0123456789abcdef";
	std::string result(size * 2, '0');
	for (size_t index = 0; index < size; ++index)
	{
		result[index * 2] = HEX[bytes[index] >> 4];
		result[index * 2 + 1] = HEX[bytes[index] & 0xf];
	}
	return result;
}

int hex_digit(char value)
{
	if (value >= '0' && value <= '9')
		return value - '0';
	if (value >= 'a' && value <= 'f')
		return value - 'a' + 10;
	if (value >= 'A' && value <= 'F')
		return value - 'A' + 10;
	return -1;
}

bool hex_decode(const char *encoded, std::vector<uint8_t> *bytes)
{
	if (!encoded || !bytes)
		return false;
	const size_t size = strlen(encoded);
	if (size % 2)
		return false;
	try
	{
		bytes->assign(size / 2, 0);
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return false;
	}
	for (size_t index = 0; index < bytes->size(); ++index)
	{
		const int high = hex_digit(encoded[index * 2]);
		const int low = hex_digit(encoded[index * 2 + 1]);
		if (high < 0 || low < 0)
			return false;
		(*bytes)[index] = static_cast<uint8_t>((high << 4) | low);
	}
	return true;
}

std::string operation_hex(const critical_operation_id &operation)
{
	return hex_encode(operation.bytes.data(), operation.bytes.size());
}

bool candidate_listing_ids(MYSQL *connection, const item_transfer_payload &payload, bool for_update,
			   bool limit_one, std::vector<uint64_t> *listings)
{
	if (!connection || !listings || !payload.item_count)
	{
		errno = EINVAL;
		return false;
	}
	std::string query = "SELECT listing_id,item_uid FROM collector_listings FORCE INDEX "
			    "(idx_collector_item_history) WHERE item_uid IN (";
	for (size_t index = 0; index < payload.item_count; ++index)
		query += (index ? "," : "") + std::to_string(payload.items[index].item_uid);
	query += ") AND status=" +
		 std::to_string(static_cast<unsigned int>(collector::state::candidate)) +
		 " ORDER BY item_uid,listing_id";
	if (limit_one)
		query += " LIMIT 1";
	if (for_update)
		query += " FOR UPDATE";
	if (!execute(connection, query))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
	{
		errno = static_cast<int>(mysql_errno(connection));
		return false;
	}
	std::vector<uint64_t> candidate;
	try
	{
		candidate.reserve(static_cast<size_t>(mysql_num_rows(rows)));
		MYSQL_ROW row = nullptr;
		while ((row = mysql_fetch_row(rows)) != nullptr)
		{
			uint64_t listing = 0, uid = 0;
			if (!parse_u64(row[0], &listing) || !parse_u64(row[1], &uid) || !listing ||
			    !uid)
			{
				mysql_free_result(rows);
				errno = EBADMSG;
				return false;
			}
			const auto item = std::lower_bound(
				payload.items.begin(), payload.items.begin() + payload.item_count,
				uid, [](const item_transfer_entry &entry, uint64_t sought)
				{ return entry.item_uid < sought; });
			if (item == payload.items.begin() + payload.item_count ||
			    item->item_uid != uid)
			{
				mysql_free_result(rows);
				errno = EBADMSG;
				return false;
			}
			candidate.push_back(listing);
		}
		std::sort(candidate.begin(), candidate.end());
		if (std::adjacent_find(candidate.begin(), candidate.end()) != candidate.end())
		{
			mysql_free_result(rows);
			errno = EBADMSG;
			return false;
		}
	}
	catch (const std::bad_alloc &)
	{
		mysql_free_result(rows);
		errno = ENOMEM;
		return false;
	}
	mysql_free_result(rows);
	*listings = std::move(candidate);
	return true;
}

uint64_t record_due_at(const collector::record &entry)
{
	switch (entry.status)
	{
	case collector::state::candidate:
		return entry.collect_at;
	case collector::state::collected:
		return entry.sale_at;
	case collector::state::available:
		return entry.holding_paused ? 0 : entry.expires_at;
	case collector::state::purchased:
	case collector::state::cancelled:
	case collector::state::expired:
		return 0;
	}
	return 0;
}

bool load_catalog(MYSQL *connection, catalog_state *state)
{
	if (!state || !execute(connection,
			       "SELECT catalog_revision,next_listing FROM collector_catalog_state "
			       "WHERE state_id=1 FOR UPDATE"))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	MYSQL_ROW row = rows ? mysql_fetch_row(rows) : nullptr;
	const bool ok = row && mysql_num_rows(rows) == 1 && parse_u64(row[0], &state->revision) &&
			parse_u64(row[1], &state->next_listing) && state->next_listing;
	if (rows)
		mysql_free_result(rows);
	if (!ok)
		errno = EBADMSG;
	return ok;
}

bool projection_matches(const collector::record &entry, uint64_t beneficiary, uint64_t item_uid,
			uint64_t status, uint64_t paused, bool due_null, uint64_t due,
			uint64_t listing_revision, uint64_t item_revision, uint64_t price,
			const char *death_hex)
{
	const uint64_t expected_due = record_due_at(entry);
	return entry.beneficiary == beneficiary && entry.uid == item_uid &&
	       static_cast<uint64_t>(entry.status) == status &&
	       static_cast<uint64_t>(entry.holding_paused ? 1 : 0) == paused &&
	       entry.revision == listing_revision && entry.item_revision == item_revision &&
	       entry.price_value == price && due_null == !expected_due &&
	       (!expected_due || due == expected_due) && death_hex &&
	       !strcasecmp(entry.death_operation.data(), death_hex);
}

bool load_listing(MYSQL *connection, uint64_t listing, bool for_update, listing_state *state,
		  unsigned int *result_code)
{
	const std::string query =
		"SELECT HEX(death_operation_id),beneficiary_pid,item_uid,status,"
		"holding_paused,due_at,due_at IS NULL,listing_revision,item_revision,"
		"price_value,HEX(record_blob),OCTET_LENGTH(item_blob),"
		"LEFT(HEX(item_blob),262146) FROM collector_listings WHERE listing_id=" +
		std::to_string(listing) + (for_update ? " FOR UPDATE" : "");
	if (!state || !result_code || !execute(connection, query))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
	{
		const unsigned int error = mysql_errno(connection);
		errno = static_cast<int>(error ? error : EIO);
		return false;
	}
	MYSQL_ROW row = mysql_fetch_row(rows);
	if (!row)
	{
		mysql_free_result(rows);
		*result_code = ENOENT;
		return true;
	}
	uint64_t beneficiary = 0, item_uid = 0, status = 0, paused = 0, due = 0,
		 listing_revision = 0, item_revision = 0, price = 0, due_is_null = 0,
		 item_blob_size = 0;
	std::vector<uint8_t> record_blob;
	std::vector<uint8_t> item_blob;
	const bool parsed =
		mysql_num_rows(rows) == 1 && row[0] && parse_u64(row[1], &beneficiary) &&
		parse_u64(row[2], &item_uid) && parse_u64(row[3], &status) &&
		parse_u64(row[4], &paused) && (!row[5] || parse_u64(row[5], &due)) &&
		parse_u64(row[6], &due_is_null) && due_is_null <= 1 &&
		parse_u64(row[7], &listing_revision) && parse_u64(row[8], &item_revision) &&
		parse_u64(row[9], &price) && hex_decode(row[10], &record_blob) &&
		((!row[11] && !row[12]) ||
		 (row[11] && row[12] && parse_u64(row[11], &item_blob_size) &&
		  item_blob_size <= COLLECTOR_COMMAND_ITEM_BLOB_MAX_BYTES &&
		  strlen(row[12]) == item_blob_size * 2 && hex_decode(row[12], &item_blob)));
	collector::record entry;
	const bool decoded = parsed &&
			     collector::record_decode(record_blob.data(), record_blob.size(),
						      &entry) == collector::codec_result::ok;
	const bool held = decoded && (entry.status == collector::state::collected ||
				      entry.status == collector::state::available);
	const bool valid = decoded && entry.listing == listing && paused <= 1 &&
			   item_blob.size() <= COLLECTOR_COMMAND_ITEM_BLOB_MAX_BYTES &&
			   (!held || !item_blob.empty()) &&
			   projection_matches(entry, beneficiary, item_uid, status, paused,
					      due_is_null != 0, due, listing_revision,
					      item_revision, price, row[0]);
	mysql_free_result(rows);
	if (!valid)
	{
		*result_code = EBADMSG;
		return true;
	}
	state->entry = entry;
	state->item_blob = std::move(item_blob);
	return true;
}

bool mutate_hint(MYSQL *connection, const collector::record &entry,
		 const collector_command_payload &payload, unsigned int *result_code)
{
	if (!connection || !result_code)
	{
		errno = EINVAL;
		return false;
	}
	if (payload.action == collector_action::hint &&
	    (entry.status != collector::state::available || entry.holding_paused ||
	     payload.observed_at < entry.available_at || payload.observed_at >= entry.expires_at))
	{
		*result_code = ESTALE;
		return true;
	}
	const std::string death_hex(entry.death_operation.data(),
				    collector::death_operation_hex_size);
	if (!execute(connection, "SELECT beneficiary_pid,death_time,hint_state,hint_revision "
				 "FROM collector_deaths WHERE death_operation_id=UNHEX('" +
					 death_hex + "') FOR UPDATE"))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	MYSQL_ROW row = rows ? mysql_fetch_row(rows) : nullptr;
	uint64_t values[4] = {};
	const bool parsed = row && mysql_num_rows(rows) == 1 && parse_u64(row[0], &values[0]) &&
			    parse_u64(row[1], &values[1]) && parse_u64(row[2], &values[2]) &&
			    parse_u64(row[3], &values[3]);
	if (rows)
		mysql_free_result(rows);
	if (!row)
	{
		*result_code = ENOENT;
		return true;
	}
	if (!parsed || values[0] != entry.beneficiary || values[1] != entry.death_time ||
	    values[2] > COLLECTOR_HINT_DELIVERED)
	{
		*result_code = EBADMSG;
		return true;
	}
	const std::string where =
		" WHERE death_operation_id=UNHEX('" + death_hex + "') AND hint_state=" +
		std::to_string(payload.action == collector_action::hint ? COLLECTOR_HINT_NONE :
									  COLLECTOR_HINT_PENDING) +
		" AND hint_revision=" +
		std::to_string(payload.action == collector_action::hint ?
				       0 :
				       payload.expected_listing_revision);
	if (payload.action == collector_action::hint)
	{
		if (values[2] != COLLECTOR_HINT_NONE)
		{
			*result_code = EALREADY;
			return true;
		}
		if (!execute(connection,
			     "UPDATE collector_deaths SET hint_state=" +
				     std::to_string(COLLECTOR_HINT_PENDING) +
				     ",hint_revision=" + std::to_string(entry.revision) + where) ||
		    mysql_affected_rows(connection) != 1)
		{
			errno = ESTALE;
			return false;
		}
		return true;
	}
	if (values[2] == COLLECTOR_HINT_DELIVERED)
	{
		*result_code = EALREADY;
		return true;
	}
	if (values[2] != COLLECTOR_HINT_PENDING || values[3] != payload.expected_listing_revision)
	{
		*result_code = ESTALE;
		return true;
	}
	if (!execute(connection, "UPDATE collector_deaths SET hint_state=" +
					 std::to_string(COLLECTOR_HINT_DELIVERED) + where) ||
	    mysql_affected_rows(connection) != 1)
	{
		errno = ESTALE;
		return false;
	}
	return true;
}

bool owner_less(const item_owner_identity &left, const item_owner_identity &right)
{
	if (left.type != right.type)
		return left.type < right.type;
	if (left.id != right.id)
		return left.id < right.id;
	return left.context_id < right.context_id;
}

bool ensure_owner(MYSQL *connection, const item_owner_identity &owner)
{
	return execute(connection, "INSERT IGNORE INTO item_owner_revision(owner_type,owner_id,"
				   "owner_context_id,revision) VALUES(" +
					   std::to_string(static_cast<unsigned int>(owner.type)) +
					   "," + std::to_string(owner.id) + "," +
					   std::to_string(owner.context_id) + ",0)");
}

bool lock_owner(MYSQL *connection, const item_owner_identity &owner, uint64_t *revision)
{
	if (!revision ||
	    !execute(connection, "SELECT revision FROM item_owner_revision WHERE owner_type=" +
					 std::to_string(static_cast<unsigned int>(owner.type)) +
					 " AND owner_id=" + std::to_string(owner.id) +
					 " AND owner_context_id=" +
					 std::to_string(owner.context_id) + " FOR UPDATE"))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	MYSQL_ROW row = rows ? mysql_fetch_row(rows) : nullptr;
	const bool ok = row && mysql_num_rows(rows) == 1 && parse_u64(row[0], revision);
	if (rows)
		mysql_free_result(rows);
	if (!ok)
		errno = EBADMSG;
	return ok;
}

bool lock_owners(MYSQL *connection, const collector_command_payload &payload,
		 uint64_t *from_revision, uint64_t *to_revision)
{
	if (!from_revision || !to_revision)
		return false;
	const item_owner_identity *first = &payload.from_owner;
	const item_owner_identity *second = &payload.to_owner;
	uint64_t *first_revision = from_revision;
	uint64_t *second_revision = to_revision;
	if (!owner_less(*first, *second))
	{
		std::swap(first, second);
		std::swap(first_revision, second_revision);
	}
	return ensure_owner(connection, *first) && ensure_owner(connection, *second) &&
	       lock_owner(connection, *first, first_revision) &&
	       lock_owner(connection, *second, second_revision);
}

bool load_authority_root(MYSQL *connection, uint64_t root, std::vector<authority_item> *items)
{
	if (!items || !execute(connection,
			       "SELECT item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,"
			       "owner_context_id,item_revision,vnum,state FROM item_current_owner "
			       "WHERE root_item_uid=" +
				       std::to_string(root) + " ORDER BY item_uid FOR UPDATE"))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
		return false;
	try
	{
		items->clear();
		items->reserve(static_cast<size_t>(mysql_num_rows(rows)));
		MYSQL_ROW row = nullptr;
		while ((row = mysql_fetch_row(rows)))
		{
			uint64_t values[9] = {};
			bool ok = parse_u64(row[0], &values[0]) && parse_u64(row[1], &values[1]) &&
				  (!row[2] || parse_u64(row[2], &values[2]));
			for (size_t index = 3; ok && index < 9; ++index)
				ok = parse_u64(row[index], &values[index]);
			if (!ok || values[3] > static_cast<uint8_t>(item_owner_type::collector) ||
			    values[7] > INT32_MAX ||
			    values[8] > static_cast<uint8_t>(item_custody_state::quarantined))
			{
				mysql_free_result(rows);
				errno = EBADMSG;
				return false;
			}
			items->push_back(
				{ values[0],
				  values[1],
				  values[2],
				  { static_cast<item_owner_type>(values[3]), values[4], values[5] },
				  values[6],
				  static_cast<int32_t>(values[7]),
				  static_cast<item_custody_state>(values[8]) });
		}
	}
	catch (const std::bad_alloc &)
	{
		mysql_free_result(rows);
		errno = ENOMEM;
		return false;
	}
	mysql_free_result(rows);
	return true;
}

bool authority_matches_payload(const std::vector<authority_item> &stored,
			       const collector_command_payload &payload, unsigned int *result_code)
{
	if (stored.size() != payload.item_count)
	{
		*result_code = EMSGSIZE;
		return false;
	}
	for (size_t index = 0; index < stored.size(); ++index)
	{
		const authority_item &actual = stored[index];
		const item_transfer_entry &expected = payload.items[index];
		if (actual.uid != expected.item_uid || actual.root != expected.root_item_uid ||
		    actual.parent != expected.parent_item_uid ||
		    !item_owner_identity_equal(actual.owner, payload.from_owner) ||
		    actual.revision != expected.expected_item_revision ||
		    actual.vnum != expected.vnum || actual.state != expected.expected_state)
		{
			*result_code = ESTALE;
			return false;
		}
	}
	return true;
}

bool decode_singleton(const collector_command_payload &payload, player_item_snapshot *item,
		      unsigned int *result_code)
{
	const auto expected =
		std::find_if(payload.items.begin(), payload.items.begin() + payload.item_count,
			     [&](const item_transfer_entry &candidate)
			     { return candidate.item_uid == payload.selected_item_uid; });
	std::vector<player_item_snapshot> decoded;
	const auto outcome = player_item_snapshot_list_decode(payload.item_blob.data(),
							      payload.item_blob_size, &decoded);
	if (outcome == player_snapshot_codec_result::allocation_failure)
	{
		errno = ENOMEM;
		return false;
	}
	if (outcome != player_snapshot_codec_result::ok || decoded.size() != 1 ||
	    decoded[0].parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
	    decoded[0].equipment_slot != 0 || decoded[0].object_uid != payload.selected_item_uid ||
	    expected == payload.items.begin() + payload.item_count ||
	    decoded[0].vnum != expected->vnum)
	{
		*result_code = EBADMSG;
		return true;
	}
	*item = std::move(decoded[0]);
	return true;
}

bool load_physical_rows(MYSQL *connection, const collector_command_payload &payload,
			std::vector<physical_item> *items, std::string *table,
			unsigned int *result_code)
{
	if (!items || !table || !result_code)
		return false;
	items->clear();
	std::string scope;
	if (payload.from_owner.type == item_owner_type::corpse)
	{
		const uint64_t pid = payload.from_owner.id >> 32;
		const uint64_t save_id = static_cast<uint32_t>(payload.from_owner.id);
		const std::string lookup =
			"SELECT p.corpse_id FROM corpse_items p JOIN corpses c ON c.id=p.corpse_id "
			"JOIN player_data d ON d.name=c.player_name WHERE d.pid=" +
			std::to_string(pid) + " AND c.save_id=" + std::to_string(save_id) +
			" AND p.obj_uid=" + std::to_string(payload.selected_item_uid) +
			" FOR UPDATE";
		if (!execute(connection, lookup))
			return false;
		MYSQL_RES *rows = mysql_store_result(connection);
		MYSQL_ROW row = rows ? mysql_fetch_row(rows) : nullptr;
		uint64_t corpse_id = 0;
		const bool found = row && mysql_num_rows(rows) == 1 &&
				   parse_u64(row[0], &corpse_id);
		if (rows)
			mysql_free_result(rows);
		if (!found)
		{
			*result_code = ENOENT;
			return true;
		}
		*table = "corpse_items";
		scope = "corpse_id=" + std::to_string(corpse_id);
	}
	else if (payload.from_owner.type == item_owner_type::room)
	{
		const std::string lookup =
			"SELECT item_key FROM saved_items WHERE room_vnum=" +
			std::to_string(payload.from_owner.id) +
			" AND obj_uid=" + std::to_string(payload.selected_item_uid) + " FOR UPDATE";
		if (!execute(connection, lookup))
			return false;
		MYSQL_RES *rows = mysql_store_result(connection);
		MYSQL_ROW row = rows ? mysql_fetch_row(rows) : nullptr;
		if (!row)
		{
			if (rows)
				mysql_free_result(rows);
			// Most live room objects are not SQL-snapshotted. Custody remains
			// authoritative and the committed item blob is their crash-safe image.
			return true;
		}
		const bool unique = mysql_num_rows(rows) == 1 && row[0];
		const std::string item_key = unique ? row[0] : "";
		if (rows)
			mysql_free_result(rows);
		if (!unique)
		{
			*result_code = EBADMSG;
			return true;
		}
		*table = "saved_items";
		scope = "item_key=" + quote(connection, item_key);
	}
	else
	{
		*result_code = EOPNOTSUPP;
		return true;
	}
	const std::string condition = *table == "corpse_items" ? "item_condition" : "100";
	if (!execute(connection, "SELECT id,container_id,obj_uid,vnum,weight,cost," + condition +
					 ",quantity FROM " + *table + " WHERE " + scope +
					 " ORDER BY id FOR UPDATE"))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
		return false;
	try
	{
		items->reserve(static_cast<size_t>(mysql_num_rows(rows)));
		MYSQL_ROW row = nullptr;
		while ((row = mysql_fetch_row(rows)))
		{
			uint64_t id = 0, parent = 0, uid = 0, vnum = 0, quantity = 0;
			int64_t weight = 0, cost = 0, condition_value = 0;
			if (!parse_u64(row[0], &id) || (row[1] && !parse_u64(row[1], &parent)) ||
			    !row[2] || !parse_u64(row[2], &uid) || !parse_u64(row[3], &vnum) ||
			    !parse_i64(row[4], &weight) || !parse_i64(row[5], &cost) ||
			    !parse_i64(row[6], &condition_value) || !parse_u64(row[7], &quantity) ||
			    !id || !uid || vnum > INT32_MAX || weight < 0 ||
			    condition_value < INT16_MIN || condition_value > INT16_MAX ||
			    quantity > UINT16_MAX)
			{
				mysql_free_result(rows);
				*result_code = EBADMSG;
				return true;
			}
			items->push_back({ id, parent, uid, static_cast<int32_t>(vnum), weight,
					   cost, static_cast<int16_t>(condition_value),
					   static_cast<uint16_t>(quantity) });
		}
	}
	catch (const std::bad_alloc &)
	{
		mysql_free_result(rows);
		errno = ENOMEM;
		return false;
	}
	mysql_free_result(rows);
	return true;
}

const physical_item *physical_by_id(const std::vector<physical_item> &items, uint64_t id)
{
	auto found = std::find_if(items.begin(), items.end(),
				  [&](const physical_item &item) { return item.id == id; });
	return found == items.end() ? nullptr : &*found;
}

const physical_item *physical_by_uid(const std::vector<physical_item> &items, uint64_t uid)
{
	auto found = std::find_if(items.begin(), items.end(),
				  [&](const physical_item &item) { return item.uid == uid; });
	return found == items.end() ? nullptr : &*found;
}

bool physical_root_is(const std::vector<physical_item> &items, const physical_item &item,
		      uint64_t root_uid)
{
	const physical_item *cursor = &item;
	for (size_t depth = 0; depth <= items.size(); ++depth)
	{
		if (!cursor->parent_id)
			return cursor->uid == root_uid;
		cursor = physical_by_id(items, cursor->parent_id);
		if (!cursor)
			return false;
	}
	return false;
}

bool validate_physical_tree(const std::vector<physical_item> &all,
			    const collector_command_payload &payload,
			    const player_item_snapshot &snapshot, int64_t *own_weight,
			    unsigned int *result_code)
{
	if (all.empty())
	{
		*own_weight = snapshot.weight;
		if (snapshot.weight < 0)
			*result_code = EBADMSG;
		return true;
	}
	const uint64_t root_uid = payload.items[0].root_item_uid;
	std::vector<const physical_item *> tree;
	try
	{
		for (const physical_item &item : all)
			if (physical_root_is(all, item, root_uid))
				tree.push_back(&item);
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return false;
	}
	if (tree.size() != payload.item_count)
	{
		*result_code = EMSGSIZE;
		return true;
	}
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const item_transfer_entry &expected = payload.items[index];
		const physical_item *actual = physical_by_uid(all, expected.item_uid);
		const physical_item *parent = actual && actual->parent_id ?
						      physical_by_id(all, actual->parent_id) :
						      nullptr;
		if (!actual || actual->vnum != expected.vnum || actual->quantity != 1 ||
		    (parent ? parent->uid : 0) != expected.parent_item_uid)
		{
			*result_code = ESTALE;
			return true;
		}
	}
	const physical_item *selected = physical_by_uid(all, payload.selected_item_uid);
	if (!selected || selected->cost != snapshot.cost ||
	    (payload.from_owner.type == item_owner_type::corpse &&
	     selected->condition != snapshot.condition))
	{
		*result_code = ESTALE;
		return true;
	}
	int64_t children_weight = 0;
	for (const physical_item &item : all)
		if (item.parent_id == selected->id)
		{
			if (item.weight > INT64_MAX - children_weight)
			{
				*result_code = ERANGE;
				return true;
			}
			children_weight += item.weight;
		}
	if (selected->weight < children_weight)
	{
		*result_code = EBADMSG;
		return true;
	}
	*own_weight = selected->weight - children_weight;
	if (snapshot.weight != *own_weight)
	{
		*result_code = ESTALE;
		return true;
	}
	return true;
}

bool detach_physical_item(MYSQL *connection, const std::vector<physical_item> &items,
			  const std::string &table, uint64_t selected_uid, int64_t own_weight)
{
	if (items.empty())
		return true;
	const physical_item *selected = physical_by_uid(items, selected_uid);
	if (!selected)
	{
		errno = ESTALE;
		return false;
	}
	const std::string parent = selected->parent_id ? std::to_string(selected->parent_id) :
							 "NULL";
	if (!execute(connection, "UPDATE " + table + " SET container_id=" + parent +
					 " WHERE container_id=" + std::to_string(selected->id)))
		return false;
	const physical_item *ancestor =
		selected->parent_id ? physical_by_id(items, selected->parent_id) : nullptr;
	for (size_t depth = 0; ancestor && depth <= items.size(); ++depth)
	{
		if (ancestor->weight < own_weight ||
		    !execute(connection, "UPDATE " + table + " SET weight=weight-" +
						 std::to_string(own_weight) +
						 " WHERE id=" + std::to_string(ancestor->id) +
						 " AND weight>=" + std::to_string(own_weight)) ||
		    mysql_affected_rows(connection) != 1)
		{
			errno = EBADMSG;
			return false;
		}
		ancestor = ancestor->parent_id ? physical_by_id(items, ancestor->parent_id) :
						 nullptr;
	}
	if (!execute(connection,
		     "DELETE FROM " + table + " WHERE id=" + std::to_string(selected->id)) ||
	    mysql_affected_rows(connection) != 1)
		return false;
	return true;
}

bool update_authority_item(MYSQL *connection, const authority_item &item, uint64_t root,
			   uint64_t parent, const item_owner_identity &owner,
			   item_custody_state state)
{
	return execute(connection,
		       "UPDATE item_current_owner SET root_item_uid=" + std::to_string(root) +
			       ",parent_item_uid=" + (parent ? std::to_string(parent) : "NULL") +
			       ",owner_type=" +
			       std::to_string(static_cast<unsigned int>(owner.type)) +
			       ",owner_id=" + std::to_string(owner.id) +
			       ",owner_context_id=" + std::to_string(owner.context_id) +
			       ",item_revision=" + std::to_string(item.revision + 1) +
			       ",state=" + std::to_string(static_cast<unsigned int>(state)) +
			       " WHERE item_uid=" + std::to_string(item.uid) +
			       " AND item_revision=" + std::to_string(item.revision)) &&
	       mysql_affected_rows(connection) == 1;
}

bool update_owner_revision(MYSQL *connection, const item_owner_identity &owner, uint64_t prior)
{
	return execute(connection,
		       "UPDATE item_owner_revision SET revision=" + std::to_string(prior + 1) +
			       " WHERE owner_type=" +
			       std::to_string(static_cast<unsigned int>(owner.type)) +
			       " AND owner_id=" + std::to_string(owner.id) +
			       " AND owner_context_id=" + std::to_string(owner.context_id) +
			       " AND revision=" + std::to_string(prior)) &&
	       mysql_affected_rows(connection) == 1;
}

bool insert_item_ledger(MYSQL *connection, const critical_command &command, size_t event_index,
			const authority_item &item, uint64_t root, uint64_t parent,
			const item_owner_identity &to_owner, uint64_t from_revision,
			uint64_t to_revision, item_transfer_reason reason, uint64_t listing)
{
	return execute(
		connection,
		"INSERT INTO item_ownership_ledger(operation_id,event_index,item_uid,"
		"root_item_uid,parent_item_uid,from_owner_type,from_owner_id,from_owner_context_id,"
		"to_owner_type,to_owner_id,to_owner_context_id,item_revision,from_owner_revision,"
		"to_owner_revision,reason_type,reason_id,source_site) VALUES(UNHEX('" +
			operation_hex(command.operation_id) + "')," + std::to_string(event_index) +
			"," + std::to_string(item.uid) + "," + std::to_string(root) + "," +
			(parent ? std::to_string(parent) : "NULL") + "," +
			std::to_string(static_cast<unsigned int>(item.owner.type)) + "," +
			std::to_string(item.owner.id) + "," +
			std::to_string(item.owner.context_id) + "," +
			std::to_string(static_cast<unsigned int>(to_owner.type)) + "," +
			std::to_string(to_owner.id) + "," + std::to_string(to_owner.context_id) +
			"," + std::to_string(item.revision + 1) + "," +
			std::to_string(from_revision) + "," + std::to_string(to_revision) + "," +
			std::to_string(static_cast<unsigned int>(reason)) + "," +
			std::to_string(listing) + "," +
			std::to_string(static_cast<unsigned int>(command.source_site)) + ")");
}

uint64_t new_root_after_detach(const std::vector<authority_item> &items, const authority_item &item,
			       uint64_t selected_uid)
{
	if (items.empty() || items[0].root != selected_uid || item.uid == selected_uid)
		return item.root;
	const authority_item *cursor = &item;
	for (size_t depth = 0; depth <= items.size(); ++depth)
	{
		if (cursor->parent == selected_uid)
			return cursor->uid;
		auto parent = std::find_if(items.begin(), items.end(),
					   [&](const authority_item &candidate)
					   { return candidate.uid == cursor->parent; });
		if (parent == items.end())
			return 0;
		cursor = &*parent;
	}
	return 0;
}

bool collect_authority(MYSQL *connection, const critical_command &command,
		       const collector_command_payload &payload,
		       const std::vector<authority_item> &items, uint64_t from_revision,
		       uint64_t to_revision)
{
	const authority_item *selected = nullptr;
	for (const authority_item &item : items)
		if (item.uid == payload.selected_item_uid)
			selected = &item;
	if (!selected)
		return false;
	for (size_t index = 0; index < items.size(); ++index)
	{
		const authority_item &item = items[index];
		if (item.uid == selected->uid)
			continue;
		const uint64_t root = new_root_after_detach(items, item, selected->uid);
		const uint64_t parent = item.parent == selected->uid ? selected->parent :
								       item.parent;
		if (!root ||
		    !update_authority_item(connection, item, root, parent, item.owner,
					   item_custody_state::active) ||
		    !insert_item_ledger(connection, command, index, item, root, parent, item.owner,
					from_revision + 1, from_revision + 1,
					item_transfer_reason::collector_collect, payload.listing))
			return false;
	}
	const size_t selected_index = static_cast<size_t>(selected - items.data());
	if (!update_authority_item(connection, *selected, selected->uid, 0, payload.to_owner,
				   item_custody_state::active) ||
	    !insert_item_ledger(connection, command, selected_index, *selected, selected->uid, 0,
				payload.to_owner, from_revision + 1, to_revision + 1,
				item_transfer_reason::collector_collect, payload.listing) ||
	    !update_owner_revision(connection, payload.from_owner, from_revision) ||
	    !update_owner_revision(connection, payload.to_owner, to_revision))
		return false;
	return true;
}

bool held_authority(MYSQL *connection, const critical_command &command,
		    const collector_command_payload &payload, const authority_item &item,
		    uint64_t from_revision, uint64_t to_revision)
{
	const item_transfer_reason reason = payload.action == collector_action::purchase ?
						    item_transfer_reason::collector_buyback :
						    item_transfer_reason::collector_expire;
	return update_authority_item(connection, item, item.uid, 0, payload.to_owner,
				     payload.target_state) &&
	       insert_item_ledger(connection, command, 0, item, item.uid, 0, payload.to_owner,
				  from_revision + 1, to_revision + 1, reason, payload.listing) &&
	       update_owner_revision(connection, payload.from_owner, from_revision) &&
	       update_owner_revision(connection, payload.to_owner, to_revision);
}

int64_t wallet_value(const currency_vector &wallet)
{
	int64_t value = 0;
	for (size_t index = 0; index < wallet.amount.size(); ++index)
	{
		if (wallet.amount[index] < 0 ||
		    wallet.amount[index] > (INT64_MAX - value) / COIN_VALUES[index])
			return -1;
		value += wallet.amount[index] * COIN_VALUES[index];
	}
	return value;
}

currency_vector canonical_wallet(int64_t value)
{
	currency_vector result = {};
	for (size_t index = COIN_VALUES.size(); index-- > 0;)
	{
		result.amount[index] = value / COIN_VALUES[index];
		value %= COIN_VALUES[index];
	}
	return result;
}

bool lock_wallet(MYSQL *connection, const collector_command_payload &payload, wallet_state *state,
		 unsigned int *result_code)
{
	const std::string account =
		escape(connection, payload.account_name.data(),
		       strnlen(payload.account_name.data(), payload.account_name.size()));
	if (!execute(connection,
		     "SELECT account_name,racewar,copper,silver,gold,platinum,wallet_revision "
		     "FROM player_data WHERE pid=" +
			     std::to_string(payload.actor_pid) + " FOR UPDATE"))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	MYSQL_ROW row = rows ? mysql_fetch_row(rows) : nullptr;
	uint64_t values[6] = {};
	bool ok = row && mysql_num_rows(rows) == 1 && row[0] && row[1] &&
		  !strcasecmp(row[0], payload.account_name.data()) &&
		  parse_u64(row[1], &values[0]) && values[0] == payload.racewar;
	for (size_t index = 2; ok && index < 7; ++index)
		ok = parse_u64(row[index], &values[index - 1]);
	if (rows)
		mysql_free_result(rows);
	if (!ok)
	{
		*result_code = ENOENT;
		return true;
	}
	state->pid = payload.actor_pid;
	for (size_t index = 0; index < state->wallet.amount.size(); ++index)
	{
		if (values[index + 1] > INT_MAX)
		{
			*result_code = ERANGE;
			return true;
		}
		state->wallet.amount[index] = static_cast<int64_t>(values[index + 1]);
	}
	state->wallet_revision = values[5];
	if (state->wallet_revision != payload.expected_wallet_revision)
	{
		*result_code = ESTALE;
		return true;
	}
	if (!execute(connection,
		     "SELECT id,bank_copper,bank_silver,bank_gold,bank_platinum,bank_revision "
		     "FROM account_banks WHERE account_name='" +
			     account + "' AND racewar=" + std::to_string(payload.racewar) +
			     " FOR UPDATE"))
		return false;
	rows = mysql_store_result(connection);
	row = rows ? mysql_fetch_row(rows) : nullptr;
	uint64_t bank[6] = {};
	ok = row && mysql_num_rows(rows) == 1;
	for (size_t index = 0; ok && index < 6; ++index)
		ok = parse_u64(row[index], &bank[index]);
	if (rows)
		mysql_free_result(rows);
	if (!ok || bank[0] > UINT32_MAX)
	{
		*result_code = ENOENT;
		return true;
	}
	state->bank_id = static_cast<uint32_t>(bank[0]);
	for (size_t index = 0; index < state->bank.amount.size(); ++index)
	{
		if (bank[index + 1] > INT_MAX)
		{
			*result_code = ERANGE;
			return true;
		}
		state->bank.amount[index] = static_cast<int64_t>(bank[index + 1]);
	}
	state->bank_revision = bank[5];
	if (state->bank_revision != payload.expected_bank_revision)
		*result_code = ESTALE;
	return true;
}

bool apply_wallet_purchase(MYSQL *connection, const critical_command &command,
			   const collector_command_payload &payload, uint64_t price,
			   wallet_state *state)
{
	if (price > static_cast<uint64_t>(INT64_MAX))
	{
		errno = ERANGE;
		return false;
	}
	const int64_t before_value = wallet_value(state->wallet);
	if (before_value < 0 || before_value < static_cast<int64_t>(price) ||
	    state->wallet_revision == UINT64_MAX || state->bank_revision == UINT64_MAX)
	{
		errno = ERANGE;
		return false;
	}
	const currency_vector before = state->wallet;
	const currency_vector after = canonical_wallet(before_value - static_cast<int64_t>(price));
	std::string sql =
		"INSERT IGNORE INTO currency_wallet_baseline(pid,opening_copper,opening_silver,"
		"opening_gold,opening_platinum,opening_revision) VALUES(" +
		std::to_string(state->pid);
	for (int64_t amount : before.amount)
		sql += "," + std::to_string(amount);
	sql += "," + std::to_string(state->wallet_revision) + ")";
	if (!execute(connection, sql))
		return false;
	sql = "INSERT IGNORE INTO currency_bank_baseline(bank_id,opening_copper,opening_silver,"
	      "opening_gold,opening_platinum,opening_revision) VALUES(" +
	      std::to_string(state->bank_id);
	for (int64_t amount : state->bank.amount)
		sql += "," + std::to_string(amount);
	sql += "," + std::to_string(state->bank_revision) + ")";
	if (!execute(connection, sql))
		return false;
	const uint64_t old_wallet_revision = state->wallet_revision++;
	const uint64_t old_bank_revision = state->bank_revision++;
	sql = "UPDATE player_data SET copper=" + std::to_string(after.amount[0]) +
	      ",silver=" + std::to_string(after.amount[1]) +
	      ",gold=" + std::to_string(after.amount[2]) +
	      ",platinum=" + std::to_string(after.amount[3]) +
	      ",wallet_revision=" + std::to_string(state->wallet_revision) +
	      " WHERE pid=" + std::to_string(state->pid) +
	      " AND wallet_revision=" + std::to_string(old_wallet_revision);
	if (!execute(connection, sql) || mysql_affected_rows(connection) != 1)
		return false;
	sql = "UPDATE account_banks SET bank_revision=" + std::to_string(state->bank_revision) +
	      " WHERE id=" + std::to_string(state->bank_id) +
	      " AND bank_revision=" + std::to_string(old_bank_revision);
	if (!execute(connection, sql) || mysql_affected_rows(connection) != 1)
		return false;
	state->wallet = after;
	sql = "INSERT INTO currency_ledger(operation_id,pid,bank_id,wallet_delta_copper,"
	      "wallet_delta_silver,wallet_delta_gold,wallet_delta_platinum,bank_delta_copper,"
	      "bank_delta_silver,bank_delta_gold,bank_delta_platinum,wallet_after_copper,"
	      "wallet_after_silver,wallet_after_gold,wallet_after_platinum,bank_after_copper,"
	      "bank_after_silver,bank_after_gold,bank_after_platinum,wallet_revision,bank_revision,"
	      "reason_type,reason_id,source_site) VALUES(UNHEX('" +
	      operation_hex(command.operation_id) + "')," + std::to_string(state->pid) + "," +
	      std::to_string(state->bank_id);
	for (size_t index = 0; index < before.amount.size(); ++index)
		sql += "," + std::to_string(after.amount[index] - before.amount[index]);
	for (size_t index = 0; index < state->bank.amount.size(); ++index)
		sql += ",0";
	for (int64_t amount : after.amount)
		sql += "," + std::to_string(amount);
	for (int64_t amount : state->bank.amount)
		sql += "," + std::to_string(amount);
	sql += "," + std::to_string(state->wallet_revision) + "," +
	       std::to_string(state->bank_revision) + "," +
	       std::to_string(static_cast<unsigned int>(currency_reason_type::collector_purchase)) +
	       "," + std::to_string(payload.listing) + "," +
	       std::to_string(static_cast<unsigned int>(command.source_site)) + ")";
	return execute(connection, sql);
}

std::string optional_string(MYSQL *connection, const player_item_snapshot &item, uint8_t mask,
			    const std::string &value)
{
	return item.string_mask & mask ? quote(connection, value) : "NULL";
}

bool insert_player_item(MYSQL *connection, uint32_t pid, const player_item_snapshot &item,
			uint32_t *database_id, unsigned int *result_code)
{
	if (!database_id)
	{
		errno = EINVAL;
		return false;
	}
	*database_id = 0;
	if (!execute(connection, "SELECT id FROM player_items WHERE obj_uid=" +
					 std::to_string(item.object_uid) + " FOR UPDATE"))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
		return false;
	const bool exists = mysql_num_rows(rows) != 0;
	mysql_free_result(rows);
	if (exists)
	{
		*result_code = EEXIST;
		return true;
	}
	std::ostringstream sql;
	sql << "INSERT INTO player_items(pid,vnum,equip_slot,container_id,quantity,weight,cost,"
	       "timer,extra_flags,wear_flags,item_type,value0,value1,value2,value3,value4,value5,"
	       "value6,value7,name,short_descr,description,action_descr,bitvector1,bitvector2,"
	       "bitvector3,bitvector4,bitvector5,item_material,obj_uid,item_condition) VALUES("
	    << pid << ',' << item.vnum << ",0,NULL,1," << item.weight << ',' << item.cost << ','
	    << item.timers[0] << ',' << item.extra_flags << ',' << item.wear_flags << ','
	    << static_cast<int>(item.type);
	for (int32_t value : item.values)
		sql << ',' << value;
	sql << ',' << optional_string(connection, item, 1, item.name) << ','
	    << optional_string(connection, item, 4, item.short_description) << ','
	    << optional_string(connection, item, 2, item.description) << ','
	    << optional_string(connection, item, 8, item.action_description);
	for (uint64_t bitvector : item.bitvectors)
		sql << ',' << bitvector;
	sql << ',' << static_cast<int>(item.material) << ',' << item.object_uid << ','
	    << item.condition << ')';
	if (!execute(connection, sql.str()))
		return false;
	const uint64_t item_id = mysql_insert_id(connection);
	if (!item_id || item_id > INT_MAX)
	{
		errno = ERANGE;
		return false;
	}
	*database_id = static_cast<uint32_t>(item_id);
	std::unordered_set<uint64_t> affect_keys;
	for (const auto &affect : item.affects)
		if (affect[0] || affect[1])
		{
			const uint64_t key =
				(static_cast<uint64_t>(static_cast<uint16_t>(affect[0])) << 32) |
				static_cast<uint32_t>(affect[1]);
			if (!affect_keys.insert(key).second)
				continue;
			if (!execute(
				    connection,
				    "INSERT INTO player_item_affects(item_id,location,modifier) VALUES(" +
					    std::to_string(item_id) + "," +
					    std::to_string(affect[0]) + "," +
					    std::to_string(affect[1]) + ")"))
				return false;
		}
	std::unordered_set<std::string> description_keys;
	for (const auto &description : item.extra_descriptions)
	{
		if (description.keyword.empty())
			continue;
		std::string encoded_description = description.description;
		if (description.spellbook)
		{
			std::ostringstream encoded;
			encoded << '[';
			for (size_t index = 0; index < description.spell_ids.size(); ++index)
				encoded << (index ? "," : "") << description.spell_ids[index];
			encoded << ']';
			encoded_description = encoded.str();
		}
		std::string description_key = description.keyword;
		description_key.push_back('\0');
		description_key += encoded_description;
		if (!description_keys.insert(std::move(description_key)).second)
			continue;
		if (!execute(
			    connection,
			    "INSERT INTO player_item_extra_descr(item_id,keyword,description) VALUES(" +
				    std::to_string(item_id) + "," +
				    quote(connection, description.keyword) + "," +
				    quote(connection, encoded_description) + ")"))
			return false;
	}
	return true;
}

unsigned int outcome_code(collector::outcome outcome)
{
	switch (outcome)
	{
	case collector::outcome::applied:
		return 0;
	case collector::outcome::not_due:
		return EAGAIN;
	case collector::outcome::conflict:
		return ESTALE;
	case collector::outcome::invalid:
		return EINVAL;
	case collector::outcome::overflow:
		return ERANGE;
	case collector::outcome::forbidden:
		return EACCES;
	case collector::outcome::insufficient_funds:
		return ENOSPC;
	case collector::outcome::capacity:
		return ENOBUFS;
	}
	return EINVAL;
}

bool persist_listing(MYSQL *connection, const critical_command &command,
		     const collector_command_payload &payload, const collector::record &entry,
		     const std::vector<uint8_t> &item_blob, uint64_t prior_catalog_revision,
		     uint64_t *catalog_revision)
{
	std::array<uint8_t, collector::encoded_record_bytes> record = {};
	if (!catalog_revision ||
	    collector::record_encode(entry, &record) != collector::codec_result::ok ||
	    prior_catalog_revision == UINT64_MAX)
	{
		errno = ERANGE;
		return false;
	}
	*catalog_revision = prior_catalog_revision + 1;
	const uint64_t due = record_due_at(entry);
	const std::string blob_sql =
		item_blob.empty() ?
			"NULL" :
			"UNHEX('" + hex_encode(item_blob.data(), item_blob.size()) + "')";
	std::string sql =
		"UPDATE collector_listings SET status=" +
		std::to_string(static_cast<unsigned int>(entry.status)) +
		",holding_paused=" + std::to_string(entry.holding_paused ? 1 : 0) +
		",due_at=" + (due ? std::to_string(due) : "NULL") +
		",listing_revision=" + std::to_string(entry.revision) +
		",item_revision=" + std::to_string(entry.item_revision) +
		",price_value=" + std::to_string(entry.price_value) + ",record_blob=UNHEX('" +
		hex_encode(record.data(), record.size()) + "'),item_blob=" + blob_sql +
		" WHERE listing_id=" + std::to_string(entry.listing) +
		" AND listing_revision=" + std::to_string(payload.expected_listing_revision);
	if (!execute(connection, sql) || mysql_affected_rows(connection) != 1)
		return false;
	if (!execute(connection, "UPDATE collector_catalog_state SET catalog_revision=" +
					 std::to_string(*catalog_revision) +
					 " WHERE state_id=1 AND catalog_revision=" +
					 std::to_string(prior_catalog_revision)) ||
	    mysql_affected_rows(connection) != 1)
		return false;
	int64_t value_delta = 0;
	if (payload.action == collector_action::purchase)
	{
		if (entry.price_value > static_cast<uint64_t>(INT64_MAX))
		{
			errno = ERANGE;
			return false;
		}
		value_delta = -static_cast<int64_t>(entry.price_value);
	}
	return execute(
		connection,
		"INSERT INTO collector_ledger(operation_id,listing_id,action,catalog_revision,"
		"listing_revision,actor_pid,item_uid,value_delta,closed_reason,source_site) "
		"VALUES(UNHEX('" +
			operation_hex(command.operation_id) + "')," +
			std::to_string(entry.listing) + "," +
			std::to_string(static_cast<unsigned int>(payload.action)) + "," +
			std::to_string(*catalog_revision) + "," + std::to_string(entry.revision) +
			"," + std::to_string(payload.actor_pid) + "," + std::to_string(entry.uid) +
			"," + std::to_string(value_delta) + "," +
			std::to_string(static_cast<unsigned int>(entry.closed_reason)) + "," +
			std::to_string(static_cast<unsigned int>(command.source_site)) + ")");
}
} // namespace

bool collector_repository_prepare_item_boundary(MYSQL *connection,
						const item_transfer_payload &payload,
						collector_item_boundary_repository_plan *plan,
						unsigned int *result_code)
{
	if (!connection || !plan || !result_code)
	{
		errno = EINVAL;
		return false;
	}
	*plan = {};
	*result_code = 0;
	const collector::reason reason = collector_item_transfer_boundary_reason(payload);
	if (reason == collector::reason::none)
		return true;

	// The first read avoids serializing the overwhelmingly common movement that
	// has no collector candidate. The second read is a current locking read on
	// the item-history index. An empty range lock prevents a concurrent death
	// enrollment from inserting a candidate until this transfer commits. If a
	// candidate appeared between reads, retry so the next attempt can follow the
	// normal catalog->listing lock order without an inversion.
	std::vector<uint64_t> observed;
	if (!candidate_listing_ids(connection, payload, false, true, &observed))
		return false;
	catalog_state catalog;
	if (!observed.empty() && !load_catalog(connection, &catalog))
		return false;
	std::vector<uint64_t> listings;
	if (!candidate_listing_ids(connection, payload, true, false, &listings))
		return false;
	if (observed.empty() && !listings.empty())
	{
		errno = EAGAIN;
		return false;
	}
	if (listings.empty())
		return true;
	if (observed.empty())
	{
		errno = EAGAIN;
		return false;
	}

	try
	{
		plan->entries.reserve(listings.size());
		for (uint64_t listing : listings)
		{
			collector_listing_detail prior;
			unsigned int listing_code = 0;
			if (!load_listing(connection, listing, true, &prior, &listing_code))
				return false;
			if (listing_code)
			{
				*result_code = listing_code;
				return true;
			}
			if (payload.collector.present &&
			    !strcasecmp(prior.entry.death_operation.data(),
					operation_hex(payload.collector.death_operation).c_str()))
				continue;
			const auto item = std::lower_bound(
				payload.items.begin(), payload.items.begin() + payload.item_count,
				prior.entry.uid, [](const item_transfer_entry &entry, uint64_t uid)
				{ return entry.item_uid < uid; });
			if (item == payload.items.begin() + payload.item_count ||
			    item->item_uid != prior.entry.uid)
			{
				*result_code = EBADMSG;
				return true;
			}
			uint64_t post_item_revision = 0;
			if (item->expected_state == item_custody_state::absent)
			{
				if (item->expected_item_revision != ITEM_TRANSFER_ABSENT_REVISION)
				{
					*result_code = EBADMSG;
					return true;
				}
				post_item_revision = 1;
			}
			else
			{
				if (item->expected_item_revision == UINT64_MAX)
				{
					*result_code = ERANGE;
					return true;
				}
				post_item_revision = item->expected_item_revision + 1;
			}
			if (post_item_revision < prior.entry.item_revision)
			{
				*result_code = ESTALE;
				return true;
			}
			collector::record cancelled = prior.entry;
			const collector::outcome outcome =
				collector::cancel(&cancelled, prior.entry.revision, reason);
			if (outcome != collector::outcome::applied)
			{
				*result_code = outcome_code(outcome);
				return true;
			}
			cancelled.item_revision = post_item_revision;
			plan->entries.push_back({ std::move(prior), cancelled });
		}
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return false;
	}
	if (plan->entries.empty())
		return true;
	if (catalog.revision > UINT64_MAX - plan->entries.size())
	{
		*result_code = ERANGE;
		plan->entries.clear();
		return true;
	}
	plan->reason = reason;
	plan->actor_pid = collector_item_transfer_actor_pid(payload);
	plan->catalog_revision = catalog.revision;
	return true;
}

bool collector_repository_apply_item_boundary(MYSQL *connection, const critical_command &command,
					      const collector_item_boundary_repository_plan &plan,
					      uint64_t *catalog_revision,
					      std::vector<collector_command_result> *events)
{
	if (!connection || !catalog_revision || !events ||
	    (plan.entries.empty() ? plan.reason != collector::reason::none :
				    plan.reason == collector::reason::none))
	{
		errno = EINVAL;
		return false;
	}
	*catalog_revision = 0;
	events->clear();
	if (plan.entries.empty())
		return true;
	try
	{
		events->reserve(plan.entries.size());
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return false;
	}
	uint64_t revision = plan.catalog_revision;
	for (const auto &change : plan.entries)
	{
		collector_command_payload payload = {};
		payload.action = collector_action::cancel;
		payload.cancel_reason = plan.reason;
		payload.listing = change.prior.entry.listing;
		payload.expected_listing_revision = change.prior.entry.revision;
		payload.actor_pid = plan.actor_pid;
		uint64_t next_revision = 0;
		if (!persist_listing(connection, command, payload, change.cancelled,
				     change.prior.item_blob, revision, &next_revision))
			return false;
		collector_command_result result = {};
		result.action = collector_action::cancel;
		result.record_present = true;
		result.catalog_revision = next_revision;
		result.entry = change.cancelled;
		events->push_back(result);
		revision = next_revision;
	}
	*catalog_revision = revision;
	return true;
}

bool collector_repository_prepare_death_enrollment(MYSQL *connection,
						   const item_transfer_payload &payload,
						   collector_enrollment_repository_plan *plan,
						   unsigned int *result_code)
{
	if (!connection || !plan || !result_code)
	{
		errno = EINVAL;
		return false;
	}
	*plan = {};
	*result_code = 0;
	if (!payload.collector.present)
		return true;

	std::vector<player_item_snapshot> snapshots;
	std::vector<uint64_t> eligible;
	if (!payload.item_blob_size ||
	    player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
					     &snapshots) != player_snapshot_codec_result::ok ||
	    snapshots.size() != payload.item_count)
	{
		*result_code = EBADMSG;
		return true;
	}
	try
	{
		eligible.reserve(snapshots.size());
		for (const player_item_snapshot &snapshot : snapshots)
			if (collector_death_item_snapshot_eligible(snapshot))
				eligible.push_back(snapshot.object_uid);
		std::sort(eligible.begin(), eligible.end());
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return false;
	}
	if (eligible != payload.collector.eligible_item_uids)
	{
		*result_code = EBADMSG;
		return true;
	}

	catalog_state catalog;
	if (!load_catalog(connection, &catalog))
		return false;
	const std::string death_hex = operation_hex(payload.collector.death_operation);
	const std::string death_query =
		"SELECT beneficiary_pid,death_time,collection_delay,sale_delay,holding_duration,"
		"price_percent,minimum_value FROM collector_deaths WHERE death_operation_id=UNHEX('" +
		death_hex + "') FOR UPDATE";
	if (!execute(connection, death_query))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
	{
		errno = static_cast<int>(mysql_errno(connection));
		return false;
	}
	MYSQL_ROW row = mysql_fetch_row(rows);
	collector::rules policy;
	policy.enabled = true;
	bool death_valid = true;
	if (row)
	{
		uint64_t beneficiary = 0, death_time = 0;
		death_valid = mysql_num_rows(rows) == 1 && parse_u64(row[0], &beneficiary) &&
			      parse_u64(row[1], &death_time) &&
			      parse_u64(row[2], &policy.collection_delay) &&
			      parse_u64(row[3], &policy.sale_delay) &&
			      parse_u64(row[4], &policy.holding_duration) &&
			      parse_u64(row[5], &policy.price_percent) &&
			      parse_u64(row[6], &policy.minimum_value) &&
			      beneficiary == payload.collector.beneficiary_pid &&
			      death_time == payload.collector.death_time &&
			      collector::valid_rules(policy);
		plan->death_exists = true;
	}
	else
	{
		policy.collection_delay = payload.collector.policy.collection_delay;
		policy.sale_delay = payload.collector.policy.sale_delay;
		policy.holding_duration = payload.collector.policy.holding_duration;
		policy.price_percent = payload.collector.policy.price_percent;
		policy.minimum_value = payload.collector.policy.minimum_value;
		death_valid = collector::valid_rules(policy);
	}
	mysql_free_result(rows);
	if (!death_valid)
	{
		*result_code = ESTALE;
		return true;
	}

	const std::string existing_query =
		"SELECT item_uid FROM collector_listings WHERE death_operation_id=UNHEX('" +
		death_hex + "') ORDER BY item_uid FOR UPDATE";
	if (!execute(connection, existing_query))
		return false;
	rows = mysql_store_result(connection);
	if (!rows)
	{
		errno = static_cast<int>(mysql_errno(connection));
		return false;
	}
	std::vector<uint64_t> existing;
	try
	{
		existing.reserve(mysql_num_rows(rows));
		while ((row = mysql_fetch_row(rows)) != nullptr)
		{
			uint64_t uid = 0;
			if (!parse_u64(row[0], &uid) || !uid ||
			    (!existing.empty() && existing.back() >= uid))
			{
				mysql_free_result(rows);
				*result_code = EBADMSG;
				return true;
			}
			existing.push_back(uid);
		}
		plan->new_items.reserve(eligible.size());
		for (uint64_t uid : eligible)
		{
			if (std::binary_search(existing.begin(), existing.end(), uid))
				continue;
			const auto item = std::lower_bound(
				payload.items.begin(), payload.items.begin() + payload.item_count,
				uid, [](const item_transfer_entry &entry, uint64_t sought)
				{ return entry.item_uid < sought; });
			if (item == payload.items.begin() + payload.item_count ||
			    item->item_uid != uid)
			{
				mysql_free_result(rows);
				*result_code = EBADMSG;
				return true;
			}
			plan->new_items.push_back(*item);
		}
	}
	catch (const std::bad_alloc &)
	{
		mysql_free_result(rows);
		errno = ENOMEM;
		return false;
	}
	mysql_free_result(rows);
	if ((!plan->new_items.empty() && catalog.revision == UINT64_MAX) ||
	    catalog.next_listing > UINT64_MAX - plan->new_items.size() ||
	    catalog.next_listing - 1 > collector::catalog_max_records - plan->new_items.size())
	{
		*result_code = ENOSPC;
		return true;
	}
	plan->active = true;
	plan->catalog_revision = catalog.revision;
	plan->next_listing = catalog.next_listing;
	plan->policy = policy;
	return true;
}

bool collector_repository_apply_death_enrollment(MYSQL *connection, const critical_command &command,
						 const item_transfer_payload &payload,
						 const item_transfer_result &transfer,
						 const collector_enrollment_repository_plan &plan)
{
	if (!connection || !plan.active || !payload.collector.present ||
	    transfer.item_count != payload.item_count ||
	    (!plan.death_exists &&
	     !critical_operation_id_equal(command.operation_id, payload.collector.death_operation)))
	{
		errno = EINVAL;
		return false;
	}
	const std::string death_hex = operation_hex(payload.collector.death_operation);
	if (!plan.death_exists &&
	    !execute(connection,
		     "INSERT INTO collector_deaths(death_operation_id,beneficiary_pid,death_time,"
		     "collection_delay,sale_delay,holding_duration,price_percent,minimum_value) "
		     "VALUES(UNHEX('" +
			     death_hex + "')," + std::to_string(payload.collector.beneficiary_pid) +
			     "," + std::to_string(payload.collector.death_time) + "," +
			     std::to_string(plan.policy.collection_delay) + "," +
			     std::to_string(plan.policy.sale_delay) + "," +
			     std::to_string(plan.policy.holding_duration) + "," +
			     std::to_string(plan.policy.price_percent) + "," +
			     std::to_string(plan.policy.minimum_value) + ")"))
		return false;

	char death_operation[CRITICAL_COMMAND_ID_HEX_SIZE] = {};
	if (!critical_operation_id_to_hex(payload.collector.death_operation, death_operation,
					  sizeof(death_operation)))
	{
		errno = EINVAL;
		return false;
	}
	for (size_t index = 0; index < plan.new_items.size(); ++index)
	{
		const item_transfer_entry &item = plan.new_items[index];
		if (item.expected_item_revision == UINT64_MAX &&
		    item.expected_state != item_custody_state::absent)
		{
			errno = EINVAL;
			return false;
		}
		const uint64_t item_revision = item.expected_item_revision ==
							       ITEM_TRANSFER_ABSENT_REVISION ?
						       1 :
						       item.expected_item_revision + 1;
		collector::record entry;
		const uint64_t listing = plan.next_listing + index;
		if (collector::enroll(listing, death_operation, payload.collector.beneficiary_pid,
				      item.item_uid, item_revision, payload.collector.death_time,
				      plan.policy, &entry) != collector::outcome::applied)
		{
			errno = ERANGE;
			return false;
		}
		std::array<uint8_t, collector::encoded_record_bytes> encoded = {};
		if (collector::record_encode(entry, &encoded) != collector::codec_result::ok ||
		    !execute(connection,
			     "INSERT INTO collector_listings(listing_id,death_operation_id,"
			     "beneficiary_pid,item_uid,status,holding_paused,due_at,"
			     "listing_revision,item_revision,price_value,record_blob,item_blob) "
			     "VALUES(" +
				     std::to_string(entry.listing) + ",UNHEX('" + death_hex +
				     "')," + std::to_string(entry.beneficiary) + "," +
				     std::to_string(entry.uid) + "," +
				     std::to_string(static_cast<unsigned int>(entry.status)) +
				     ",0," + std::to_string(entry.collect_at) + "," +
				     std::to_string(entry.revision) + "," +
				     std::to_string(entry.item_revision) + ",0,UNHEX('" +
				     hex_encode(encoded.data(), encoded.size()) + "'),NULL)"))
			return false;
	}
	if (plan.new_items.empty())
		return true;
	if (plan.catalog_revision == UINT64_MAX ||
	    !execute(connection,
		     "UPDATE collector_catalog_state SET catalog_revision=" +
			     std::to_string(plan.catalog_revision + 1) + ",next_listing=" +
			     std::to_string(plan.next_listing + plan.new_items.size()) +
			     " WHERE state_id=1 AND catalog_revision=" +
			     std::to_string(plan.catalog_revision) +
			     " AND next_listing=" + std::to_string(plan.next_listing)) ||
	    mysql_affected_rows(connection) != 1)
		return false;
	return true;
}

bool collector_repository_read_bootstrap(MYSQL *connection, collector_bootstrap_snapshot *snapshot)
{
	static_assert(collector::catalog_max_records == 262144,
		      "update the bounded collector catalog query when its limit changes");
	static_assert(static_cast<unsigned int>(item_owner_type::collector) == 10,
		      "update the collector bootstrap ownership predicate");
	static const char QUERY[] =
		"SELECT s.catalog_revision,s.next_listing,l.listing_id,"
		"HEX(l.death_operation_id),l.beneficiary_pid,l.item_uid,l.status,"
		"l.holding_paused,l.due_at,l.due_at IS NULL,l.listing_revision,"
		"l.item_revision,l.price_value,l.record_blob,o.item_uid,o.root_item_uid,"
		"o.parent_item_uid,o.owner_type,o.owner_id,o.owner_context_id,o.item_revision,"
		"o.vnum,o.state,r.revision,h.held_count FROM collector_catalog_state s "
		"CROSS JOIN (SELECT COUNT(*) held_count FROM item_current_owner WHERE owner_type=10) h "
		"LEFT JOIN collector_listings l ON TRUE LEFT JOIN item_current_owner o "
		"ON o.item_uid=l.item_uid AND o.owner_type=10 LEFT JOIN item_owner_revision r "
		"ON r.owner_type=o.owner_type AND r.owner_id=o.owner_id "
		"AND r.owner_context_id=o.owner_context_id "
		"WHERE s.state_id=1 ORDER BY l.listing_id LIMIT 262145";
	static const char DEATH_QUERY[] =
		"SELECT HEX(death_operation_id),beneficiary_pid,death_time,collection_delay,"
		"sale_delay,holding_duration,price_percent,minimum_value,hint_state,hint_revision "
		"FROM collector_deaths ORDER BY beneficiary_pid,death_time,death_operation_id "
		"LIMIT 262145";
	if (!connection || !snapshot)
	{
		errno = EINVAL;
		return false;
	}
	if (!execute(connection, QUERY))
		return false;
	// This bounded result is buffered deliberately. A validation failure can stop
	// parsing early without returning a pooled connection with unread wire data.
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
	{
		const unsigned int error = mysql_errno(connection);
		errno = static_cast<int>(error ? error : EIO);
		return false;
	}
	collector_bootstrap_snapshot candidate;
	bool saw_catalog = false;
	bool saw_empty_projection = false;
	bool saw_held_count = false;
	uint64_t held_count = 0;
	bool valid = true;
	int failure = EBADMSG;
	try
	{
		while (MYSQL_ROW row = mysql_fetch_row(rows))
		{
			const unsigned long *lengths = mysql_fetch_lengths(rows);
			uint64_t row_revision = 0, row_next_listing = 0, row_held_count = 0;
			if (!lengths || !parse_u64(row[0], &row_revision) ||
			    !parse_u64(row[1], &row_next_listing) || !row_next_listing ||
			    !parse_u64(row[24], &row_held_count) ||
			    row_held_count > collector::catalog_max_records ||
			    (saw_catalog && (row_revision != candidate.catalog.revision ||
					     row_next_listing != candidate.catalog.next_listing)) ||
			    (saw_held_count && row_held_count != held_count))
			{
				valid = false;
				if (row_held_count > collector::catalog_max_records)
					failure = E2BIG;
				break;
			}
			if (!saw_catalog)
			{
				candidate.catalog.revision = row_revision;
				candidate.catalog.next_listing = row_next_listing;
				saw_catalog = true;
			}
			if (!saw_held_count)
			{
				held_count = row_held_count;
				saw_held_count = true;
				candidate.held_items.reserve(static_cast<size_t>(held_count));
			}
			if (!row[2])
			{
				bool unexpected = saw_empty_projection ||
						  !candidate.catalog.records.empty();
				for (size_t index = 3; index <= 8; ++index)
					unexpected = unexpected || row[index];
				for (size_t index = 10; index <= 23; ++index)
					unexpected = unexpected || row[index];
				uint64_t due_is_null = 0;
				if (unexpected || !parse_u64(row[9], &due_is_null) ||
				    due_is_null != 1)
				{
					valid = false;
					break;
				}
				saw_empty_projection = true;
				continue;
			}
			if (saw_empty_projection ||
			    candidate.catalog.records.size() >= collector::catalog_max_records)
			{
				valid = false;
				failure = E2BIG;
				break;
			}
			uint64_t listing = 0, beneficiary = 0, item_uid = 0, status = 0, paused = 0,
				 due = 0, due_is_null = 0, listing_revision = 0, item_revision = 0,
				 price = 0;
			collector::record entry;
			const bool parsed =
				row[3] && parse_u64(row[2], &listing) &&
				parse_u64(row[4], &beneficiary) && parse_u64(row[5], &item_uid) &&
				parse_u64(row[6], &status) && parse_u64(row[7], &paused) &&
				(!row[8] || parse_u64(row[8], &due)) &&
				parse_u64(row[9], &due_is_null) && due_is_null <= 1 &&
				parse_u64(row[10], &listing_revision) &&
				parse_u64(row[11], &item_revision) && parse_u64(row[12], &price) &&
				row[13] && lengths[13] == collector::encoded_record_bytes &&
				collector::record_decode(reinterpret_cast<const uint8_t *>(row[13]),
							 lengths[13],
							 &entry) == collector::codec_result::ok;
			if (!parsed || entry.listing != listing || paused > 1 ||
			    !projection_matches(entry, beneficiary, item_uid, status, paused,
						due_is_null != 0, due, listing_revision,
						item_revision, price, row[3]))
			{
				valid = false;
				break;
			}
			const bool should_be_held = entry.status == collector::state::collected ||
						    entry.status == collector::state::available;
			const bool authority_present = row[14] != nullptr;
			if (should_be_held != authority_present)
			{
				valid = false;
				break;
			}
			if (authority_present)
			{
				uint64_t authority_uid = 0, root_uid = 0, owner_type = 0,
					 owner_id = 0, owner_context = 0, authority_revision = 0,
					 custody_state = 0, owner_revision = 0;
				int64_t vnum = 0;
				if (!row[15] || row[16] || !row[17] || !row[18] || !row[19] ||
				    !row[20] || !row[21] || !row[22] || !row[23] ||
				    !parse_u64(row[14], &authority_uid) ||
				    !parse_u64(row[15], &root_uid) ||
				    !parse_u64(row[17], &owner_type) ||
				    !parse_u64(row[18], &owner_id) ||
				    !parse_u64(row[19], &owner_context) ||
				    !parse_u64(row[20], &authority_revision) ||
				    !parse_i64(row[21], &vnum) ||
				    !parse_u64(row[22], &custody_state) ||
				    !parse_u64(row[23], &owner_revision) ||
				    authority_uid != entry.uid || root_uid != entry.uid ||
				    owner_type !=
					    static_cast<uint64_t>(item_owner_type::collector) ||
				    owner_id != item_collector_owner_id(entry.listing) ||
				    owner_context || authority_revision != entry.item_revision ||
				    vnum <= 0 || vnum > INT32_MAX ||
				    custody_state !=
					    static_cast<uint64_t>(item_custody_state::active) ||
				    !owner_revision)
				{
					valid = false;
					break;
				}
				candidate.held_items.push_back(
					{ authority_uid,
					  root_uid,
					  0,
					  { item_owner_type::collector, owner_id, 0 },
					  authority_revision,
					  owner_revision,
					  static_cast<int32_t>(vnum),
					  item_custody_state::active });
			}
			else
				for (size_t index = 14; index <= 23; ++index)
					if (row[index])
					{
						valid = false;
						break;
					}
			if (!valid)
				break;
			candidate.catalog.records.push_back(entry);
		}
	}
	catch (const std::bad_alloc &)
	{
		valid = false;
		failure = ENOMEM;
	}
	const unsigned int read_error = mysql_errno(connection);
	mysql_free_result(rows);
	if (read_error)
	{
		errno = static_cast<int>(read_error);
		return false;
	}
	if (!valid || !saw_catalog || !saw_held_count ||
	    candidate.held_items.size() != held_count ||
	    !collector::valid_catalog(candidate.catalog))
	{
		errno = failure;
		return false;
	}
	if (!execute(connection, DEATH_QUERY))
		return false;
	rows = mysql_store_result(connection);
	if (!rows)
	{
		const unsigned int error = mysql_errno(connection);
		errno = static_cast<int>(error ? error : EIO);
		return false;
	}
	valid = mysql_num_rows(rows) <= collector::catalog_max_records;
	failure = valid ? EBADMSG : E2BIG;
	std::set<std::pair<uint32_t, uint64_t>> death_identities;
	MYSQL_ROW row = nullptr;
	try
	{
		candidate.deaths.reserve(static_cast<size_t>(mysql_num_rows(rows)));
		while (valid && (row = mysql_fetch_row(rows)) != nullptr)
		{
			const unsigned long *lengths = mysql_fetch_lengths(rows);
			uint64_t beneficiary = 0, death_time = 0, hint_state = 0, hint_revision = 0;
			collector_death_snapshot death;
			death.policy.enabled = true;
			if (!lengths || !row[0] || lengths[0] != CRITICAL_COMMAND_ID_HEX_SIZE - 1 ||
			    !critical_operation_id_from_hex(row[0], &death.operation_id) ||
			    !parse_u64(row[1], &beneficiary) || beneficiary > UINT32_MAX ||
			    !parse_u64(row[2], &death_time) ||
			    !parse_u64(row[3], &death.policy.collection_delay) ||
			    !parse_u64(row[4], &death.policy.sale_delay) ||
			    !parse_u64(row[5], &death.policy.holding_duration) ||
			    !parse_u64(row[6], &death.policy.price_percent) ||
			    !parse_u64(row[7], &death.policy.minimum_value) ||
			    !parse_u64(row[8], &hint_state) || hint_state > 2 ||
			    !parse_u64(row[9], &hint_revision) || !beneficiary || !death_time ||
			    !collector::valid_rules(death.policy) ||
			    !death_identities
				     .emplace(static_cast<uint32_t>(beneficiary), death_time)
				     .second)
			{
				valid = false;
				break;
			}
			death.beneficiary_pid = static_cast<uint32_t>(beneficiary);
			death.death_time = death_time;
			death.hint_state = static_cast<uint8_t>(hint_state);
			death.hint_revision = hint_revision;
			candidate.deaths.push_back(death);
		}
	}
	catch (const std::bad_alloc &)
	{
		valid = false;
		failure = ENOMEM;
	}
	const unsigned int death_read_error = mysql_errno(connection);
	mysql_free_result(rows);
	if (death_read_error)
	{
		errno = static_cast<int>(death_read_error);
		return false;
	}
	if (!valid)
	{
		errno = failure;
		return false;
	}
	*snapshot = std::move(candidate);
	return true;
}

bool collector_repository_read_catalog(MYSQL *connection, collector::catalog *catalog)
{
	if (!connection || !catalog)
	{
		errno = EINVAL;
		return false;
	}
	collector_bootstrap_snapshot snapshot;
	if (!collector_repository_read_bootstrap(connection, &snapshot))
		return false;
	*catalog = std::move(snapshot.catalog);
	return true;
}

bool collector_repository_read_listing(MYSQL *connection, uint64_t listing,
				       collector_listing_detail *detail, bool *found)
{
	if (!connection || !listing || !detail || !found)
	{
		errno = EINVAL;
		return false;
	}
	listing_state candidate;
	unsigned int result_code = 0;
	if (!load_listing(connection, listing, false, &candidate, &result_code))
		return false;
	if (result_code == ENOENT)
	{
		*found = false;
		return true;
	}
	if (result_code)
	{
		errno = static_cast<int>(result_code);
		return false;
	}
	*detail = std::move(candidate);
	*found = true;
	return true;
}

bool collector_repository_execute(MYSQL *connection, const critical_command &command,
				  collector_command_result *result, unsigned int *result_code,
				  bool *mutation_applied)
{
	collector_command_payload payload = {};
	if (!connection || !result || !result_code || !mutation_applied ||
	    !collector_command_decode_payload(command, &payload))
	{
		errno = EINVAL;
		return false;
	}
	*result = {};
	result->action = payload.action;
	*result_code = 0;
	*mutation_applied = false;

	catalog_state catalog;
	listing_state listing;
	if (!load_catalog(connection, &catalog) ||
	    !load_listing(connection, payload.listing, true, &listing, result_code))
		return false;
	if (*result_code)
		return true;
	if (payload.listing >= catalog.next_listing)
	{
		*result_code = EBADMSG;
		return true;
	}
	if (payload.action != collector_action::hint_ack &&
	    listing.entry.revision != payload.expected_listing_revision)
	{
		*result_code = ESTALE;
		return true;
	}
	if (payload.action == collector_action::hint ||
	    payload.action == collector_action::hint_ack)
	{
		if (!mutate_hint(connection, listing.entry, payload, result_code))
			return false;
		if (*result_code)
			return true;
		result->record_present = true;
		result->catalog_revision = catalog.revision;
		result->entry = listing.entry;
		*mutation_applied = true;
		return true;
	}

	collector::record updated = listing.entry;
	std::vector<uint8_t> item_blob = listing.item_blob;
	wallet_state wallet;
	std::vector<authority_item> authority;
	std::vector<physical_item> physical;
	std::string physical_table;
	player_item_snapshot exact_item;
	uint64_t from_revision = 0, to_revision = 0;
	int64_t own_weight = 0;

	if (payload.action == collector_action::purchase)
	{
		if (!lock_wallet(connection, payload, &wallet, result_code))
			return false;
		result->wallet = wallet.wallet;
		result->bank = wallet.bank;
		result->wallet_revision = wallet.wallet_revision;
		result->bank_revision = wallet.bank_revision;
		if (*result_code)
			return true;
	}

	if (payload.item_count)
	{
		if (!lock_owners(connection, payload, &from_revision, &to_revision))
			return false;
		result->from_owner_revision = from_revision;
		result->to_owner_revision = to_revision;
		if (from_revision != payload.expected_from_owner_revision ||
		    to_revision != payload.expected_to_owner_revision)
		{
			*result_code = ESTALE;
			return true;
		}
		if (from_revision == UINT64_MAX || to_revision == UINT64_MAX)
		{
			*result_code = ERANGE;
			return true;
		}
		if (!load_authority_root(connection, payload.items[0].root_item_uid, &authority))
			return false;
		if (!authority_matches_payload(authority, payload, result_code))
			return true;
		if (!decode_singleton(payload, &exact_item, result_code))
			return false;
		if (*result_code)
			return true;
		if (payload.action != collector_action::collect &&
		    item_blob.size() == payload.item_blob_size &&
		    !std::equal(item_blob.begin(), item_blob.end(), payload.item_blob.begin()))
		{
			*result_code = ESTALE;
			return true;
		}
		if (payload.action != collector_action::collect &&
		    item_blob.size() != payload.item_blob_size)
		{
			*result_code = ESTALE;
			return true;
		}
	}

	collector::outcome policy = collector::outcome::invalid;
	if (payload.action == collector_action::collect)
	{
		if (!load_physical_rows(connection, payload, &physical, &physical_table,
					result_code))
			return false;
		if (*result_code)
			return true;
		if (!validate_physical_tree(physical, payload, exact_item, &own_weight,
					    result_code))
			return false;
		if (*result_code)
			return true;
		auto selected_authority = std::find_if(
			authority.begin(), authority.end(), [&](const authority_item &item)
			{ return item.uid == payload.selected_item_uid; });
		if (selected_authority == authority.end())
		{
			*result_code = ESTALE;
			return true;
		}
		policy = collector::collect(
			&updated, payload.expected_listing_revision,
			payload.items[static_cast<size_t>(selected_authority - authority.begin())]
				.expected_item_revision,
			selected_authority->revision, true, exact_item.cost, payload.observed_at);
		item_blob.assign(payload.item_blob.begin(),
				 payload.item_blob.begin() + payload.item_blob_size);
	}
	else if (payload.action == collector_action::activate)
		policy = collector::activate(&updated, payload.expected_listing_revision,
					     payload.observed_at);
	else if (payload.action == collector_action::purchase)
	{
		const int64_t carried_value = wallet_value(wallet.wallet);
		if (carried_value < 0)
		{
			*result_code = ERANGE;
			return true;
		}
		policy = collector::purchase(&updated, payload.expected_listing_revision,
					     payload.actor_pid,
					     static_cast<uint64_t>(carried_value),
					     payload.capacity_admitted, payload.observed_at);
	}
	else if (payload.action == collector_action::expire)
		policy = collector::expire(&updated, payload.expected_listing_revision,
					   payload.observed_at);
	else if (payload.action == collector_action::cancel)
		policy = collector::cancel(&updated, payload.expected_listing_revision,
					   payload.cancel_reason);
	else if (payload.action == collector_action::pause)
		policy = collector::pause(&updated, payload.expected_listing_revision,
					  payload.observed_at);
	else if (payload.action == collector_action::resume)
		policy = collector::resume(&updated, payload.expected_listing_revision,
					   payload.observed_at);

	*result_code = outcome_code(policy);
	if (*result_code)
		return true;

	if (payload.action == collector_action::collect)
	{
		if (!detach_physical_item(connection, physical, physical_table,
					  payload.selected_item_uid, own_weight) ||
		    !collect_authority(connection, command, payload, authority, from_revision,
				       to_revision))
			return false;
		result->from_owner_revision = from_revision + 1;
		result->to_owner_revision = to_revision + 1;
	}
	else if (payload.item_count)
	{
		if (payload.action == collector_action::purchase)
		{
			if (!insert_player_item(connection, payload.actor_pid, exact_item,
						&result->materialized_item_id, result_code))
				return false;
			if (*result_code)
				return true;
			if (!apply_wallet_purchase(connection, command, payload,
						   updated.price_value, &wallet))
				return false;
			result->wallet = wallet.wallet;
			result->bank = wallet.bank;
			result->wallet_revision = wallet.wallet_revision;
			result->bank_revision = wallet.bank_revision;
		}
		if (!held_authority(connection, command, payload, authority[0], from_revision,
				    to_revision))
			return false;
		result->from_owner_revision = from_revision + 1;
		result->to_owner_revision = to_revision + 1;
	}

	uint64_t catalog_revision = 0;
	if (!persist_listing(connection, command, payload, updated, item_blob, catalog.revision,
			     &catalog_revision))
		return false;
	result->record_present = true;
	result->catalog_revision = catalog_revision;
	result->entry = updated;
	*mutation_applied = true;
	return true;
}

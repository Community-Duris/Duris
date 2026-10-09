#include "economy/auction_repository.h"

#include "economy/auction_accounting.h"
#include "economy/auction_native_command_context.h"
#include "player/player_snapshot_codec.h"
#include <openssl/sha.h>
#include <unordered_set>
#include <tuple>
#include "economy/auction_listing_accounting.h"
#include "economy/auction_money_claim_accounting.h"
#include "economy/auction_item_claim_accounting.h"
#include "economy/auction_settlement_accounting.h"
#include "item/item_transfer_command.h"
#include "persistence/economic_sql_pending_claim_source.h"
#include "persistence/economic_sql_auction_retained.h"
#include "persistence/economic_sql_auction_source_claim.h"
#include "persistence/shop_item_runtime_payload.h"
#include <map>
#include <set>

// Pure original transform only; no native owner/checkpoint/publication capability.
bool auction_native_expected_player_forest(const auction_command_payload &,
					   std::span<const player_item_snapshot>,
					   std::span<const player_item_snapshot>, bool, uint32_t,
					   std::vector<player_item_snapshot> *) noexcept;

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <climits>
#include <cstring>
#include <ctime>
#include <limits>
#include <memory>
#include <charconv>
#include <type_traits>
#include <utility>
#include <strings.h>
#include <mysql.h>
#include <string>
#include <vector>

namespace
{
#ifndef __NO_MYSQL__
void auction_list171_source_observe(MYSQL *connection, const critical_command &command,
				    unsigned int stage, unsigned int error) noexcept
{
	if (command.operation_id.bytes[0] != 171 || command.payload_version != 1)
		return;
	const int saved_errno = errno;
	const unsigned int saved_mysql_errno = mysql_errno(connection);
	std::fprintf(stderr, "LIST171_SOURCE stage=%u error=%u mysql_errno=%u errno=%d\n", stage,
		     error, saved_mysql_errno, saved_errno);
	errno = saved_errno;
}
#endif

constexpr uint32_t AUCTION_STATUS_OPEN = 1;
constexpr uint32_t AUCTION_STATUS_CLOSED = 2;
constexpr uint32_t AUCTION_STATUS_REMOVED = 3;
constexpr uint8_t AUCTION_CUSTODY_AUTHORITATIVE = 1;
constexpr std::array<int64_t, CURRENCY_DENOMINATION_COUNT> COIN_VALUES = { 1, 10, 100, 1000 };

struct wallet_state
{
	uint32_t pid;
	uint32_t bank_id;
	currency_vector wallet;
	currency_vector bank;
	uint64_t wallet_revision;
	uint64_t bank_revision;
};

struct auction_state
{
	uint32_t id;
	uint32_t seller_pid;
	uint32_t winner_pid;
	uint32_t status;
	uint32_t custody_state;
	int64_t cur_price;
	int64_t buy_price;
	uint64_t revision;
	uint64_t end_time;
	std::string winner_name;
	std::string seller_account;
};

bool execute(MYSQL *connection, const std::string &sql)
{
	return mysql_real_query(connection, sql.data(), sql.size()) == 0;
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

std::string escape(MYSQL *connection, const char *value, size_t size)
{
	std::string escaped(size * 2 + 1, '\0');
	const unsigned long length = mysql_real_escape_string(connection, escaped.data(),
							      value ? value : "", value ? size : 0);
	escaped.resize(length);
	return escaped;
}

std::string operation_hex(const critical_operation_id &operation_id)
{
	static const char HEX[] = "0123456789abcdef";
	std::string result(operation_id.bytes.size() * 2, '0');
	for (size_t index = 0; index < operation_id.bytes.size(); ++index)
	{
		result[index * 2] = HEX[operation_id.bytes[index] >> 4];
		result[index * 2 + 1] = HEX[operation_id.bytes[index] & 15];
	}
	return result;
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

bool lock_wallet(MYSQL *connection, const auction_command_payload &payload, wallet_state *state,
		 unsigned int *result_code)
{
	if (!state || !result_code || !payload.actor_pid)
		return false;
	const std::string account =
		escape(connection, payload.account_name.data(),
		       strnlen(payload.account_name.data(), payload.account_name.size()));
	std::string sql = "SELECT account_name,racewar,copper,silver,gold,platinum,wallet_revision "
			  "FROM player_data WHERE pid=" +
			  std::to_string(payload.actor_pid) + " FOR UPDATE";
	if (!execute(connection, sql))
		return false;
	MYSQL_RES *query = mysql_store_result(connection);
	MYSQL_ROW row = query ? mysql_fetch_row(query) : nullptr;
	uint64_t parsed[6] = {};
	const bool player_ok = row && row[0] && row[1] &&
			       !strcasecmp(row[0], payload.account_name.data()) &&
			       atoi(row[1]) == payload.racewar && parse_u64(row[2], &parsed[0]) &&
			       parse_u64(row[3], &parsed[1]) && parse_u64(row[4], &parsed[2]) &&
			       parse_u64(row[5], &parsed[3]) && parse_u64(row[6], &parsed[4]);
	if (query)
		mysql_free_result(query);
	if (!player_ok)
	{
		*result_code = ENOENT;
		return true;
	}
	state->pid = payload.actor_pid;
	for (size_t index = 0; index < state->wallet.amount.size(); ++index)
		state->wallet.amount[index] = static_cast<int64_t>(parsed[index]);
	state->wallet_revision = parsed[4];
	if (state->wallet_revision != payload.expected_wallet_revision)
	{
		*result_code = ESTALE;
		return true;
	}
	sql = "SELECT id,bank_copper,bank_silver,bank_gold,bank_platinum,bank_revision "
	      "FROM account_banks WHERE account_name='" +
	      account + "' AND racewar=" + std::to_string(payload.racewar) + " FOR UPDATE";
	if (!execute(connection, sql))
		return false;
	query = mysql_store_result(connection);
	row = query ? mysql_fetch_row(query) : nullptr;
	const bool bank_ok = row && parse_u64(row[0], &parsed[0]) &&
			     parse_u64(row[1], &parsed[1]) && parse_u64(row[2], &parsed[2]) &&
			     parse_u64(row[3], &parsed[3]) && parse_u64(row[4], &parsed[4]) &&
			     parse_u64(row[5], &parsed[5]);
	if (query)
		mysql_free_result(query);
	if (!bank_ok)
	{
		*result_code = ENOENT;
		return true;
	}
	state->bank_id = static_cast<uint32_t>(parsed[0]);
	for (size_t index = 0; index < state->bank.amount.size(); ++index)
		state->bank.amount[index] = static_cast<int64_t>(parsed[index + 1]);
	state->bank_revision = parsed[5];
	if (state->bank_revision != payload.expected_bank_revision)
		*result_code = ESTALE;
	return true;
}

uint16_t currency_reason(auction_action action)
{
	switch (action)
	{
	case auction_action::list:
		return static_cast<uint16_t>(currency_reason_type::auction_listing);
	case auction_action::bid:
		return static_cast<uint16_t>(currency_reason_type::auction_bid);
	case auction_action::claim_money:
		return static_cast<uint16_t>(currency_reason_type::auction_claim);
	default:
		return static_cast<uint16_t>(currency_reason_type::operator_adjustment);
	}
}

bool apply_wallet_delta(MYSQL *connection, const critical_command &command,
			const auction_command_payload &payload, int64_t value_delta,
			wallet_state *state, unsigned int *result_code)
{
	const int64_t before_value = wallet_value(state->wallet);
	if (before_value < 0 || (value_delta < 0 && before_value < -value_delta) ||
	    (value_delta > 0 && before_value > INT64_MAX - value_delta))
	{
		*result_code = value_delta < 0 ? ENOSPC : ERANGE;
		return true;
	}
	if (state->wallet_revision == UINT64_MAX || state->bank_revision == UINT64_MAX)
	{
		*result_code = ERANGE;
		return true;
	}
	const currency_vector before = state->wallet;
	const currency_vector after = canonical_wallet(before_value + value_delta);
	const std::string op = operation_hex(command.operation_id);
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
	      op + "')," + std::to_string(state->pid) + "," + std::to_string(state->bank_id);
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
	       std::to_string(currency_reason(payload.action)) + "," +
	       std::to_string(payload.auction_id) + "," +
	       std::to_string(static_cast<uint16_t>(command.source_site)) + ")";
	return execute(connection, sql);
}

bool lock_auction(MYSQL *connection, uint32_t auction_id, auction_state *state,
		  unsigned int *result_code)
{
	const std::string sql =
		"SELECT a.id,a.seller_pid,a.winning_bidder_pid,a.status+0,a.custody_state,"
		"a.cur_price,a.buy_price,a.auction_revision,UNIX_TIMESTAMP(a.end_time),"
		"a.winning_bidder_name,"
		"COALESCE(ac.account_name,'') FROM auctions a LEFT JOIN account_characters ac "
		"ON ac.pid=a.seller_pid WHERE a.id=" +
		std::to_string(auction_id) + " FOR UPDATE";
	if (!execute(connection, sql))
		return false;
	MYSQL_RES *query = mysql_store_result(connection);
	MYSQL_ROW row = query ? mysql_fetch_row(query) : nullptr;
	uint64_t parsed[9] = {};
	const bool ok = row && parse_u64(row[0], &parsed[0]) && parse_u64(row[1], &parsed[1]) &&
			parse_u64(row[2], &parsed[2]) && parse_u64(row[3], &parsed[3]) &&
			parse_u64(row[4], &parsed[4]) && parse_u64(row[5], &parsed[5]) &&
			parse_u64(row[6], &parsed[6]) && parse_u64(row[7], &parsed[7]) &&
			parse_u64(row[8], &parsed[8]);
	if (!ok)
	{
		if (query)
			mysql_free_result(query);
		*result_code = ENOENT;
		return true;
	}
	*state = { .id = static_cast<uint32_t>(parsed[0]),
		   .seller_pid = static_cast<uint32_t>(parsed[1]),
		   .winner_pid = static_cast<uint32_t>(parsed[2]),
		   .status = static_cast<uint32_t>(parsed[3]),
		   .custody_state = static_cast<uint32_t>(parsed[4]),
		   .cur_price = static_cast<int64_t>(parsed[5]),
		   .buy_price = static_cast<int64_t>(parsed[6]),
		   .revision = parsed[7],
		   .end_time = parsed[8],
		   .winner_name = row[9] ? row[9] : "",
		   .seller_account = row[10] ? row[10] : "" };
	mysql_free_result(query);
	return true;
}

bool ensure_owner_revision(MYSQL *connection, item_owner_type type, uint64_t id)
{
	return execute(connection, "INSERT IGNORE INTO item_owner_revision(owner_type,owner_id,"
				   "owner_context_id,revision) VALUES(" +
					   std::to_string(static_cast<unsigned int>(type)) + "," +
					   std::to_string(id) + ",0,0)");
}

bool lock_owner_revision(MYSQL *connection, item_owner_type type, uint64_t id, uint64_t *revision)
{
	if (!ensure_owner_revision(connection, type, id) ||
	    !execute(connection, "SELECT revision FROM item_owner_revision WHERE owner_type=" +
					 std::to_string(static_cast<unsigned int>(type)) +
					 " AND owner_id=" + std::to_string(id) +
					 " AND owner_context_id=0 FOR UPDATE"))
		return false;
	MYSQL_RES *query = mysql_store_result(connection);
	MYSQL_ROW row = query ? mysql_fetch_row(query) : nullptr;
	const bool ok = row && parse_u64(row[0], revision);
	if (query)
		mysql_free_result(query);
	return ok;
}

struct native_tree_participant
{
	std::vector<player_item_snapshot> nodes;
	std::vector<std::pair<size_t, size_t>> roots;
	std::vector<uint64_t> revisions;
};

bool native_tree_shape(const auction_command_payload &payload,
		       std::span<const player_item_snapshot> selected, native_tree_participant *out)
{
	if (!out || selected.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
		return false;
	native_tree_participant value;
	value.nodes.assign(selected.begin(), selected.end());
	std::unordered_set<uint64_t> ids;
	std::array<size_t, PLAYER_SNAPSHOT_MAX_DEPTH> parents{};
	size_t depth = 0;
	for (size_t i = 0; i < selected.size(); ++i)
	{
		const auto &row = selected[i];
		if (!row.object_uid || row.object_uid == UINT64_MAX || row.string_mask != 15 ||
		    row.equipment_slot || !ids.insert(row.object_uid).second)
			return false;
		if (row.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
		{
			size_t slot = value.roots.size();
			if (slot >= payload.item_count ||
			    row.object_uid != payload.items[slot].item_uid ||
			    row.vnum != payload.items[slot].vnum)
				return false;
			if (slot)
				value.roots.back().second = i;
			value.roots.emplace_back(i, selected.size());
			depth = 1;
			parents[0] = i;
		}
		else
		{
			if (row.parent_index < 0 || static_cast<size_t>(row.parent_index) >= i)
				return false;
			while (depth && parents[depth - 1] != static_cast<size_t>(row.parent_index))
				--depth;
			if (!depth || depth == parents.size())
				return false;
			parents[depth++] = i;
		}
	}
	if (value.roots.size() != payload.item_count)
		return false;
	std::vector<uint8_t> canonical;
	if (player_item_snapshot_list_encode(value.nodes, &canonical) !=
	    player_snapshot_codec_result::ok)
		return false;
	value.revisions.resize(selected.size());
	*out = std::move(value);
	return true;
}

// ACT2 custody payload: original complete literal tree and its immutable listing
// after-revisions. It is never a legacy first-root object blob or a live capture.
template <class T> void native_tree_append(std::vector<uint8_t> &bytes, T value)
{
	for (size_t i = 0; i < sizeof(T); ++i)
		bytes.push_back(static_cast<uint8_t>(static_cast<uint64_t>(value) >> (8 * i)));
}
template <class T> bool native_tree_read(std::span<const uint8_t> bytes, size_t &at, T *value)
{
	if (at > bytes.size() || sizeof(T) > bytes.size() - at)
		return false;
	uint64_t number = 0;
	for (size_t i = 0; i < sizeof(T); ++i)
		number |= uint64_t(bytes[at++]) << (8 * i);
	*value = static_cast<T>(number);
	return true;
}
bool native_tree_blob(const native_tree_participant &native, size_t slot, std::vector<uint8_t> *out)
{
	const auto [start, end] = native.roots[slot];
	std::vector<player_item_snapshot> tree(native.nodes.begin() + start,
					       native.nodes.begin() + end);
	for (auto &row : tree)
		if (row.parent_index >= 0)
			row.parent_index -= static_cast<int32_t>(start);
	std::vector<uint8_t> canonical;
	if (player_item_snapshot_list_encode(tree, &canonical) != player_snapshot_codec_result::ok)
		return false;
	std::vector<uint8_t> blob;
	native_tree_append(blob, uint32_t{ 0x32544341 });
	native_tree_append(blob, static_cast<uint32_t>(tree.size()));
	native_tree_append(blob, static_cast<uint32_t>(canonical.size()));
	for (size_t i = start; i < end; ++i)
	{
		if (native.revisions[i] == UINT64_MAX)
			return false;
		native_tree_append(blob, native.nodes[i].object_uid);
		native_tree_append(blob, native.revisions[i] + 1);
	}
	blob.insert(blob.end(), canonical.begin(), canonical.end());
	*out = std::move(blob);
	return true;
}
bool native_tree_bind_custody(std::span<const uint8_t> blob, native_tree_participant &native,
			      size_t slot)
{
	const auto [start, end] = native.roots[slot];
	size_t at = 0;
	uint32_t magic = 0, count = 0, length = 0;
	if (!native_tree_read(blob, at, &magic) || magic != 0x32544341 ||
	    !native_tree_read(blob, at, &count) || count != end - start ||
	    !native_tree_read(blob, at, &length) || count > PLAYER_SNAPSHOT_MAX_OBJECTS ||
	    blob.size() - at < static_cast<size_t>(count) * 16)
		return false;
	std::vector<uint64_t> revisions;
	revisions.reserve(count);
	for (size_t i = start; i < end; ++i)
	{
		uint64_t uid = 0, revision = 0;
		if (!native_tree_read(blob, at, &uid) || !native_tree_read(blob, at, &revision) ||
		    uid != native.nodes[i].object_uid || !revision || revision == UINT64_MAX)
			return false;
		revisions.push_back(revision);
	}
	if (blob.size() - at != length)
		return false;
	std::vector<player_item_snapshot> tree(native.nodes.begin() + start,
					       native.nodes.begin() + end);
	for (auto &row : tree)
		if (row.parent_index >= 0)
			row.parent_index -= static_cast<int32_t>(start);
	std::vector<uint8_t> canonical;
	if (player_item_snapshot_list_encode(tree, &canonical) !=
		    player_snapshot_codec_result::ok ||
	    canonical.size() != length ||
	    !std::equal(canonical.begin(), canonical.end(), blob.begin() + at))
		return false;
	for (size_t i = start; i < end; ++i)
		native.revisions[i] = revisions[i - start];
	return true;
}

bool native_tree_lock(MYSQL *connection, const auction_command_payload &payload,
		      native_tree_participant &native, item_owner_type from_type, uint64_t from_id,
		      bool capture, unsigned int *result_code)
{
	for (const auto &[start, end] : native.roots)
	{
		const uint64_t root = native.nodes[start].object_uid;
		if (!execute(connection,
			     "SELECT COUNT(*) FROM item_current_owner WHERE root_item_uid=" +
				     std::to_string(root)))
			return false;
		MYSQL_RES *query = mysql_store_result(connection);
		MYSQL_ROW row = query ? mysql_fetch_row(query) : nullptr;
		uint64_t count = 0;
		const bool exact = query && mysql_num_fields(query) == 1 &&
				   mysql_num_rows(query) == 1 && row && parse_u64(row[0], &count) &&
				   count == end - start;
		if (query)
			mysql_free_result(query);
		if (!exact)
		{
			*result_code = ESTALE;
			return true;
		}
	}
	std::vector<size_t> order(native.nodes.size());
	for (size_t i = 0; i < order.size(); ++i)
		order[i] = i;
	std::sort(order.begin(), order.end(), [&](size_t a, size_t b)
		  { return native.nodes[a].object_uid < native.nodes[b].object_uid; });
	std::vector<size_t> root_of(native.nodes.size());
	for (size_t r = 0; r < native.roots.size(); ++r)
		for (size_t i = native.roots[r].first; i < native.roots[r].second; ++i)
			root_of[i] = r;
	for (size_t i : order)
	{
		const auto &node = native.nodes[i];
		size_t slot = root_of[i];
		uint64_t root = native.nodes[native.roots[slot].first].object_uid;
		uint64_t parent =
			node.parent_index < 0 ? 0 : native.nodes[node.parent_index].object_uid;
		if (!execute(
			    connection,
			    "SELECT root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,item_revision,vnum,state FROM item_current_owner WHERE item_uid=" +
				    std::to_string(node.object_uid) + " FOR UPDATE"))
			return false;
		MYSQL_RES *query = mysql_store_result(connection);
		MYSQL_ROW row = query ? mysql_fetch_row(query) : nullptr;
		uint64_t n[8]{};
		bool ok = query && mysql_num_fields(query) == 8 && mysql_num_rows(query) == 1 &&
			  row && parse_u64(row[0], &n[0]) &&
			  (parent ? parse_u64(row[1], &n[1]) : !row[1]);
		for (size_t f = 2; ok && f < 8; ++f)
			ok = parse_u64(row[f], &n[f]);
		ok = ok && n[0] == root && n[1] == parent &&
		     n[2] == static_cast<uint8_t>(from_type) && n[3] == from_id && !n[4] &&
		     n[5] != UINT64_MAX && static_cast<int32_t>(n[6]) == node.vnum && n[7] == 1;
		if (ok && i == native.roots[slot].first)
			ok = n[5] == payload.items[slot].expected_item_revision;
		if (ok && !capture)
			ok = n[5] == native.revisions[i];
		if (query)
			mysql_free_result(query);
		if (!ok)
		{
			*result_code = ESTALE;
			return true;
		}
		if (n[5] == UINT64_MAX - 1)
		{
			*result_code = ERANGE;
			return true;
		}
		if (capture)
			native.revisions[i] = n[5];
	}
	return true;
}

bool transition_native_tree(MYSQL *connection, const critical_command &command,
			    const auction_command_payload &payload, uint32_t auction_id,
			    item_owner_type from_type, uint64_t from_id, item_owner_type to_type,
			    uint64_t to_id, native_tree_participant &native,
			    auction_command_result *result, unsigned int *result_code)
{
	uint64_t player_revision = 0, auction_revision = 0;
	uint64_t player_id = from_type == item_owner_type::player ? from_id : to_id;
	if (!lock_owner_revision(connection, item_owner_type::player, player_id,
				 &player_revision) ||
	    !lock_owner_revision(connection, item_owner_type::auction, auction_id,
				 &auction_revision))
		return false;
	if (player_revision == UINT64_MAX || auction_revision == UINT64_MAX)
	{
		*result_code = ERANGE;
		return true;
	}
	if (!native_tree_lock(connection, payload, native, from_type, from_id, false, result_code))
		return false;
	if (*result_code)
		return true;
	++player_revision;
	++auction_revision;
	const uint64_t from_revision = from_type == item_owner_type::player ? player_revision :
									      auction_revision,
		       to_revision = to_type == item_owner_type::player ? player_revision :
									  auction_revision;
	const std::string op = operation_hex(command.operation_id);
	size_t slot = 0;
	for (size_t i = 0; i < native.nodes.size(); ++i)
	{
		if (slot + 1 < native.roots.size() && i == native.roots[slot + 1].first)
			++slot;
		const auto &node = native.nodes[i];
		uint64_t revision = native.revisions[i] + 1,
			 root = native.nodes[native.roots[slot].first].object_uid;
		std::string parent =
			node.parent_index < 0 ?
				"NULL" :
				std::to_string(native.nodes[node.parent_index].object_uid);
		std::string sql = "UPDATE item_current_owner SET owner_type=" +
				  std::to_string(static_cast<unsigned>(to_type)) +
				  ",owner_id=" + std::to_string(to_id) +
				  ",owner_context_id=0,item_revision=" + std::to_string(revision) +
				  " WHERE item_uid=" + std::to_string(node.object_uid) +
				  " AND item_revision=" + std::to_string(native.revisions[i]);
		if (!execute(connection, sql) || mysql_affected_rows(connection) != 1)
			return false;
		sql = "INSERT INTO item_ownership_ledger(operation_id,event_index,item_uid,root_item_uid,parent_item_uid,from_owner_type,from_owner_id,from_owner_context_id,to_owner_type,to_owner_id,to_owner_context_id,item_revision,from_owner_revision,to_owner_revision,reason_type,reason_id,source_site) VALUES(UNHEX('" +
		      op + "')," + std::to_string(i) + "," + std::to_string(node.object_uid) + "," +
		      std::to_string(root) + "," + parent + "," +
		      std::to_string(static_cast<unsigned>(from_type)) + "," +
		      std::to_string(from_id) + ",0," +
		      std::to_string(static_cast<unsigned>(to_type)) + "," + std::to_string(to_id) +
		      ",0," + std::to_string(revision) + "," + std::to_string(from_revision) + "," +
		      std::to_string(to_revision) + "," +
		      std::to_string(
			      static_cast<unsigned>(payload.action == auction_action::list ?
							    item_transfer_reason::auction_list :
							    item_transfer_reason::auction_claim)) +
		      "," + std::to_string(auction_id) + "," +
		      std::to_string(static_cast<unsigned>(command.source_site)) + ")";
		if (!execute(connection, sql))
			return false;
		if (i == native.roots[slot].first)
		{
			result->item_uids[slot] = node.object_uid;
			result->item_revisions[slot] = revision;
		}
	}
	for (const auto &[type, id, revision] :
	     { std::tuple{ item_owner_type::player, player_id, player_revision },
	       std::tuple{ item_owner_type::auction, uint64_t(auction_id), auction_revision } })
		if (!execute(connection, "UPDATE item_owner_revision SET revision=" +
						 std::to_string(revision) + " WHERE owner_type=" +
						 std::to_string(static_cast<unsigned>(type)) +
						 " AND owner_id=" + std::to_string(id) +
						 " AND owner_context_id=0") ||
		    mysql_affected_rows(connection) != 1)
			return false;
	result->player_owner_revision = player_revision;
	result->auction_owner_revision = auction_revision;
	result->item_count = payload.item_count;
	return true;
}

bool transition_items(MYSQL *connection, const critical_command &command,
		      const auction_command_payload &payload, uint32_t auction_id,
		      item_owner_type from_type, uint64_t from_id, item_owner_type to_type,
		      uint64_t to_id, auction_command_result *result, unsigned int *result_code)
{
	uint64_t player_revision = 0, auction_revision = 0;
	if (!lock_owner_revision(connection, item_owner_type::player,
				 from_type == item_owner_type::player ? from_id : to_id,
				 &player_revision) ||
	    !lock_owner_revision(connection, item_owner_type::auction, auction_id,
				 &auction_revision))
		return false;
	uint64_t &from_revision = from_type == item_owner_type::player ? player_revision :
									 auction_revision;
	uint64_t &to_revision = to_type == item_owner_type::player ? player_revision :
								     auction_revision;
	if (from_revision == UINT64_MAX || to_revision == UINT64_MAX)
	{
		*result_code = ERANGE;
		return true;
	}
	std::array<size_t, AUCTION_COMMAND_MAX_ITEMS> order = {};
	for (size_t index = 0; index < payload.item_count; ++index)
		order[index] = index;
	std::sort(order.begin(), order.begin() + payload.item_count, [&](size_t left, size_t right)
		  { return payload.items[left].item_uid < payload.items[right].item_uid; });
	for (size_t position = 0; position < payload.item_count; ++position)
	{
		const auction_item_entry &item = payload.items[order[position]];
		const std::string sql =
			"SELECT root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,"
			"item_revision,vnum,state FROM item_current_owner WHERE item_uid=" +
			std::to_string(item.item_uid) + " FOR UPDATE";
		if (!execute(connection, sql))
			return false;
		MYSQL_RES *query = mysql_store_result(connection);
		MYSQL_ROW row = query ? mysql_fetch_row(query) : nullptr;
		uint64_t parsed[8] = {};
		const bool ok = row && parse_u64(row[0], &parsed[0]) && !row[1] &&
				parse_u64(row[2], &parsed[2]) && parse_u64(row[3], &parsed[3]) &&
				parse_u64(row[4], &parsed[4]) && parse_u64(row[5], &parsed[5]) &&
				parse_u64(row[6], &parsed[6]) && parse_u64(row[7], &parsed[7]) &&
				parsed[0] == item.item_uid &&
				parsed[2] == static_cast<uint8_t>(from_type) &&
				parsed[3] == from_id && parsed[4] == 0 &&
				parsed[5] == item.expected_item_revision &&
				static_cast<int32_t>(parsed[6]) == item.vnum && parsed[7] == 1;
		if (query)
			mysql_free_result(query);
		if (!ok)
		{
			*result_code = ESTALE;
			return true;
		}
	}
	++player_revision;
	++auction_revision;
	const std::string op = operation_hex(command.operation_id);
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auction_item_entry &item = payload.items[index];
		const uint64_t next_item_revision = item.expected_item_revision + 1;
		std::string sql =
			"UPDATE item_current_owner SET owner_type=" +
			std::to_string(static_cast<unsigned int>(to_type)) +
			",owner_id=" + std::to_string(to_id) +
			",owner_context_id=0,item_revision=" + std::to_string(next_item_revision) +
			" WHERE item_uid=" + std::to_string(item.item_uid) +
			" AND item_revision=" + std::to_string(item.expected_item_revision);
		if (!execute(connection, sql) || mysql_affected_rows(connection) != 1)
			return false;
		sql = "INSERT INTO item_ownership_ledger(operation_id,event_index,item_uid,"
		      "root_item_uid,parent_item_uid,from_owner_type,from_owner_id,"
		      "from_owner_context_id,to_owner_type,to_owner_id,to_owner_context_id,"
		      "item_revision,from_owner_revision,to_owner_revision,reason_type,reason_id,"
		      "source_site) VALUES(UNHEX('" +
		      op + "')," + std::to_string(index) + "," + std::to_string(item.item_uid) +
		      "," + std::to_string(item.item_uid) + ",NULL," +
		      std::to_string(static_cast<unsigned int>(from_type)) + "," +
		      std::to_string(from_id) + ",0," +
		      std::to_string(static_cast<unsigned int>(to_type)) + "," +
		      std::to_string(to_id) + ",0," + std::to_string(next_item_revision) + "," +
		      std::to_string(from_revision) + "," + std::to_string(to_revision) + "," +
		      std::to_string(static_cast<unsigned int>(
			      payload.action == auction_action::list ?
				      item_transfer_reason::auction_list :
				      item_transfer_reason::auction_claim)) +
		      "," + std::to_string(auction_id) + "," +
		      std::to_string(static_cast<unsigned int>(command.source_site)) + ")";
		if (!execute(connection, sql))
			return false;
		// The typed accounting owner links this native event after the handoff.
		result->item_uids[index] = item.item_uid;
		result->item_revisions[index] = next_item_revision;
	}
	if (!execute(connection,
		     "UPDATE item_owner_revision SET revision=" + std::to_string(player_revision) +
			     " WHERE owner_type=" +
			     std::to_string(static_cast<unsigned int>(item_owner_type::player)) +
			     " AND owner_id=" +
			     std::to_string(from_type == item_owner_type::player ? from_id :
										   to_id) +
			     " AND owner_context_id=0") ||
	    !execute(connection,
		     "UPDATE item_owner_revision SET revision=" + std::to_string(auction_revision) +
			     " WHERE owner_type=" +
			     std::to_string(static_cast<unsigned int>(item_owner_type::auction)) +
			     " AND owner_id=" + std::to_string(auction_id) +
			     " AND owner_context_id=0"))
		return false;
	result->player_owner_revision = player_revision;
	result->auction_owner_revision = auction_revision;
	result->item_count = payload.item_count;
	return true;
}

bool insert_listing(MYSQL *connection, const critical_command &command,
		    const auction_command_payload &payload, uint32_t *auction_id,
		    const native_tree_participant *native = nullptr)
{
	const std::string seller =
		escape(connection, payload.actor_name.data(),
		       strnlen(payload.actor_name.data(), payload.actor_name.size()));
	const std::string object_short =
		escape(connection, payload.object_short.data(),
		       strnlen(payload.object_short.data(), payload.object_short.size()));
	const std::string keywords =
		escape(connection, payload.id_keywords.data(),
		       strnlen(payload.id_keywords.data(), payload.id_keywords.size()));
	const std::string info =
		escape(connection, payload.object_info.data(),
		       strnlen(payload.object_info.data(), payload.object_info.size()));
	const std::string blob = escape(connection,
					reinterpret_cast<const char *>(payload.object_blob.data()),
					payload.object_blob_size);
	const std::string op = operation_hex(command.operation_id);
	std::string sql =
		"INSERT INTO auctions(seller_pid,seller_name,start_time,end_time,status,cur_price,"
		"buy_price,obj_short,obj_vnum,obj_blob_str,id_keywords,obj_info_text,quantity,"
		"auction_revision,custody_state,listing_operation_id) VALUES(" +
		std::to_string(payload.actor_pid) + ",'" + seller +
		"',CURRENT_TIMESTAMP(),FROM_UNIXTIME(" + std::to_string(payload.end_time) + ")," +
		std::to_string(AUCTION_STATUS_OPEN) + "," + std::to_string(payload.start_price) +
		"," + std::to_string(payload.buy_price) + ",'" + object_short + "'," +
		std::to_string(payload.items[0].vnum) + ",'" + blob + "','" + keywords + "','" +
		info + "'," + std::to_string(payload.item_count) + ",1," +
		std::to_string(AUCTION_CUSTODY_AUTHORITATIVE) + ",UNHEX('" + op + "'))";
	if (!execute(connection, sql))
	{
#ifndef __NO_MYSQL__
		auction_list171_source_observe(connection, command, 10, 0);
#endif
		return false;
	}
	*auction_id = static_cast<uint32_t>(mysql_insert_id(connection));
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auction_item_entry &item = payload.items[index];
		std::string custody_blob = blob;
		if (native)
		{
			std::vector<uint8_t> bytes;
			if (!native_tree_blob(*native, index, &bytes))
				return false;
			custody_blob = escape(connection,
					      reinterpret_cast<const char *>(bytes.data()),
					      bytes.size());
		}
		sql = "INSERT INTO auction_item_custody(auction_id,slot,item_uid,item_revision,vnum,"
		      "obj_blob) VALUES(" +
		      std::to_string(*auction_id) + "," + std::to_string(index) + "," +
		      std::to_string(item.item_uid) + "," +
		      std::to_string(item.expected_item_revision + 1) + "," +
		      std::to_string(item.vnum) + ",'" + custody_blob + "')";
		if (!execute(connection, sql))
		{
#ifndef __NO_MYSQL__
			auction_list171_source_observe(connection, command, 11, 0);
#endif
			return false;
		}
	}
	return true;
}

bool stage_money(MYSQL *connection, uint32_t pid, int64_t amount)
{
	return pid && amount >= 0 && amount <= UINT_MAX &&
	       execute(connection,
		       "INSERT INTO auction_money_pickups(pid,money,claim_revision) VALUES(" +
			       std::to_string(pid) + "," + std::to_string(amount) +
			       ",1) ON DUPLICATE KEY UPDATE money=money+VALUES(money),"
			       "claim_revision=claim_revision+1");
}

bool lock_auction_claim_balance(MYSQL *connection, uint32_t pid, int64_t *money, uint64_t *revision)
{
	if (!pid || !money || !revision ||
	    !execute(connection,
		     "SELECT money,claim_revision FROM auction_money_pickups WHERE pid=" +
			     std::to_string(pid) + " FOR UPDATE"))
		return false;
	MYSQL_RES *query = mysql_store_result(connection);
	MYSQL_ROW row = query ? mysql_fetch_row(query) : nullptr;
	if (!row)
	{
		const bool no_rows = query && mysql_num_rows(query) == 0;
		if (query)
			mysql_free_result(query);
		if (no_rows)
		{
			*money = 0;
			*revision = 0;
			return true;
		}
		return false;
	}
	uint64_t parsed_money = 0;
	const bool valid = parse_u64(row[0], &parsed_money) && parsed_money <= UINT_MAX &&
			   parse_u64(row[1], revision);
	if (query)
		mysql_free_result(query);
	if (!valid)
	{
		errno = EILSEQ;
		return false;
	}
	*money = static_cast<int64_t>(parsed_money);
	return true;
}

bool debit_auction_claim_balance(MYSQL *connection, uint32_t pid, int64_t amount, uint64_t revision)
{
	return pid && amount > 0 && revision != UINT64_MAX &&
	       execute(connection,
		       "UPDATE auction_money_pickups SET money=money-" + std::to_string(amount) +
			       ",claim_revision=claim_revision+1 WHERE pid=" + std::to_string(pid) +
			       " AND money>=" + std::to_string(amount) +
			       " AND claim_revision=" + std::to_string(revision)) &&
	       mysql_affected_rows(connection) == 1;
}

bool stage_items(MYSQL *connection, uint32_t auction_id, uint32_t pid)
{
	return pid && execute(connection,
			      "UPDATE auction_item_custody SET claim_pid=" + std::to_string(pid) +
				      " WHERE auction_id=" + std::to_string(auction_id) +
				      " AND claim_pid IS NULL AND claimed_at IS NULL");
}

bool write_auction_ledger(MYSQL *connection, const critical_command &command,
			  const auction_command_payload &payload,
			  const auction_command_result &result)
{
	return execute(
		connection,
		"INSERT INTO auction_ledger(operation_id,event_type,auction_id,auction_revision,"
		"actor_pid,counterparty_pid,value_delta,final_price,item_count) VALUES(UNHEX('" +
			operation_hex(command.operation_id) + "')," +
			std::to_string(static_cast<unsigned int>(result.event_type)) + "," +
			std::to_string(result.auction_id) + "," +
			std::to_string(result.auction_revision) + "," +
			std::to_string(payload.actor_pid) + "," +
			std::to_string(result.seller_pid) + "," +
			std::to_string(result.wallet_value_delta) + "," +
			std::to_string(result.final_price) + "," +
			std::to_string(result.item_count) + ")");
}
} // namespace

static bool auction_repository_execute_impl(MYSQL *connection, const critical_command &command,
					    auction_command_result *result,
					    unsigned int *result_code, bool *mutation_applied,
					    native_tree_participant *native = nullptr)
{
	if (!connection || !result || !result_code || !mutation_applied)
		return false;
	auction_command_payload payload = {};
	if (!auction_command_decode_payload(command, &payload))
	{
		errno = EINVAL;
		return false;
	}
	*result = {};
	result->action = payload.action;
	*result_code = 0;
	*mutation_applied = false;
	wallet_state wallet = {};
	if (payload.actor_pid && !lock_wallet(connection, payload, &wallet, result_code))
		return false;
	if (*result_code)
		return true;
	result->wallet = wallet.wallet;
	result->bank = wallet.bank;
	result->wallet_revision = wallet.wallet_revision;
	result->bank_revision = wallet.bank_revision;
	if (payload.action == auction_action::list)
	{
		if (payload.listing_fee < 0 || payload.start_price < 0 ||
		    (payload.buy_price && payload.buy_price < payload.start_price) ||
		    !payload.object_blob_size)
		{
			*result_code = EINVAL;
			return true;
		}
		if (wallet_value(wallet.wallet) < payload.listing_fee)
		{
			*result_code = ENOSPC;
			return true;
		}
		uint32_t auction_id = 0;
		if (native &&
		    (!native_tree_lock(connection, payload, *native, item_owner_type::player,
				       payload.actor_pid, true, result_code) ||
		     *result_code))
		{
			errno = *result_code ? *result_code : mysql_errno(connection);
			return false;
		}
		if (!insert_listing(connection, command, payload, &auction_id, native))
			return false;
		if (!(native ? transition_native_tree(connection, command, payload, auction_id,
						      item_owner_type::player, payload.actor_pid,
						      item_owner_type::auction, auction_id, *native,
						      result, result_code) :
			       transition_items(connection, command, payload, auction_id,
						item_owner_type::player, payload.actor_pid,
						item_owner_type::auction, auction_id, result,
						result_code)))
			return false;
		if (*result_code)
		{
			errno = *result_code;
			return false;
		}
		if (!apply_wallet_delta(connection, command, payload, -payload.listing_fee, &wallet,
					result_code))
			return false;
		if (*result_code)
		{
			errno = *result_code;
			return false;
		}
		result->auction_id = auction_id;
		result->status = AUCTION_STATUS_OPEN;
		result->seller_pid = payload.actor_pid;
		result->wallet_value_delta = -payload.listing_fee;
		result->wallet = wallet.wallet;
		result->bank = wallet.bank;
		result->wallet_revision = wallet.wallet_revision;
		result->bank_revision = wallet.bank_revision;
		result->auction_revision = 1;
		result->event_type = auction_event_type::listed;
	}
	else if (payload.action == auction_action::bid)
	{
		auction_state auction = {};
		if (!lock_auction(connection, payload.auction_id, &auction, result_code))
			return false;
		if (*result_code)
			return true;
		if (auction.status != AUCTION_STATUS_OPEN ||
		    auction.custody_state != AUCTION_CUSTODY_AUTHORITATIVE ||
		    auction.seller_pid == payload.actor_pid ||
		    (!auction.seller_account.empty() &&
		     !strcasecmp(auction.seller_account.c_str(), payload.account_name.data())))
		{
			*result_code = EACCES;
			return true;
		}
		int64_t bid = payload.value;
		if (auction.buy_price > 0 && bid >= auction.buy_price)
			bid = auction.buy_price;
		if (bid <= 0 || (!auction.winner_pid && bid < auction.cur_price) ||
		    (auction.winner_pid && bid <= auction.cur_price))
		{
			*result_code = EINVAL;
			return true;
		}
		const int64_t to_pay =
			auction.winner_pid == payload.actor_pid ? bid - auction.cur_price : bid;
		int64_t claim_balance = 0;
		uint64_t claim_revision = 0;
		if (!lock_auction_claim_balance(connection, payload.actor_pid, &claim_balance,
						&claim_revision))
			return false;
		const int64_t claim_credit_used = std::min(to_pay, claim_balance);
		if (claim_credit_used && claim_revision == UINT64_MAX)
		{
			*result_code = ERANGE;
			return true;
		}
		const int64_t wallet_to_pay = to_pay - claim_credit_used;
		if (!apply_wallet_delta(connection, command, payload, -wallet_to_pay, &wallet,
					result_code))
			return false;
		if (*result_code)
			return true;
		if (claim_credit_used &&
		    !debit_auction_claim_balance(connection, payload.actor_pid, claim_credit_used,
						 claim_revision))
		{
			errno = EILSEQ;
			return false;
		}
		const uint32_t previous_bidder = auction.winner_pid;
		if (previous_bidder && previous_bidder != payload.actor_pid &&
		    !stage_money(connection, previous_bidder, auction.cur_price))
			return false;
		const std::string actor =
			escape(connection, payload.actor_name.data(),
			       strnlen(payload.actor_name.data(), payload.actor_name.size()));
		++auction.revision;
		const bool sold = auction.buy_price > 0 && bid >= auction.buy_price;
		std::string sql = "UPDATE auctions SET winning_bidder_pid=" +
				  std::to_string(payload.actor_pid) + ",winning_bidder_name='" +
				  actor + "',cur_price=" + std::to_string(bid) +
				  ",auction_revision=" + std::to_string(auction.revision);
		if (sold)
			sql += ",status=" + std::to_string(AUCTION_STATUS_CLOSED);
		else if (previous_bidder != payload.actor_pid && payload.bid_extension_seconds)
			sql += ",end_time=DATE_ADD(end_time,INTERVAL " +
			       std::to_string(payload.bid_extension_seconds) + " SECOND)";
		sql += " WHERE id=" + std::to_string(auction.id) +
		       " AND auction_revision=" + std::to_string(auction.revision - 1);
		if (!execute(connection, sql) || mysql_affected_rows(connection) != 1 ||
		    !execute(connection,
			     "INSERT INTO auction_bid_history(date,auction_id,bidder_pid,"
			     "bidder_name,bid_amount) VALUES(UNIX_TIMESTAMP()," +
				     std::to_string(auction.id) + "," +
				     std::to_string(payload.actor_pid) + ",'" + actor + "'," +
				     std::to_string(bid) + ")"))
			return false;
		if (sold)
		{
			const int64_t proceeds =
				bid - (bid * payload.closing_fee_basis_points / 10000);
			if (!stage_money(connection, auction.seller_pid, proceeds) ||
			    !stage_items(connection, auction.id, payload.actor_pid))
				return false;
		}
		result->auction_id = auction.id;
		result->status = sold ? AUCTION_STATUS_CLOSED : AUCTION_STATUS_OPEN;
		result->seller_pid = auction.seller_pid;
		result->winner_pid = payload.actor_pid;
		result->previous_bidder_pid = previous_bidder;
		result->final_price = bid;
		result->wallet_value_delta = -wallet_to_pay;
		result->claim_credit_used = claim_credit_used;
		result->wallet = wallet.wallet;
		result->bank = wallet.bank;
		result->wallet_revision = wallet.wallet_revision;
		result->bank_revision = wallet.bank_revision;
		result->auction_revision = auction.revision;
		result->event_type = sold ? auction_event_type::sold :
					    auction_event_type::bid_placed;
	}
	else if (payload.action == auction_action::finalize ||
		 payload.action == auction_action::remove)
	{
		auction_state auction = {};
		if (!lock_auction(connection, payload.auction_id, &auction, result_code))
			return false;
		if (*result_code)
			return true;
		if (auction.status != AUCTION_STATUS_OPEN ||
		    auction.custody_state != AUCTION_CUSTODY_AUTHORITATIVE)
		{
			*result_code = EALREADY;
			return true;
		}
		if (payload.action == auction_action::finalize &&
		    auction.end_time > static_cast<uint64_t>(time(nullptr)))
		{
			*result_code = EAGAIN;
			return true;
		}
		++auction.revision;
		const uint32_t status = payload.action == auction_action::remove ?
						AUCTION_STATUS_REMOVED :
						AUCTION_STATUS_CLOSED;
		if (!execute(connection,
			     "UPDATE auctions SET status=" + std::to_string(status) +
				     ",auction_revision=" + std::to_string(auction.revision) +
				     " WHERE id=" + std::to_string(auction.id) +
				     " AND auction_revision=" +
				     std::to_string(auction.revision - 1)) ||
		    mysql_affected_rows(connection) != 1)
			return false;
		if (!auction.winner_pid || payload.action == auction_action::remove)
		{
			if (!stage_items(connection, auction.id, auction.seller_pid))
				return false;
		}
		else
		{
			const int64_t proceeds =
				auction.cur_price -
				(auction.cur_price * payload.closing_fee_basis_points / 10000);
			if (!stage_money(connection, auction.seller_pid, proceeds) ||
			    !stage_items(connection, auction.id, auction.winner_pid))
				return false;
		}
		result->auction_id = auction.id;
		result->status = status;
		result->seller_pid = auction.seller_pid;
		result->winner_pid = auction.winner_pid;
		result->final_price = auction.cur_price;
		result->auction_revision = auction.revision;
		result->event_type = payload.action == auction_action::remove ?
					     auction_event_type::removed :
				     auction.winner_pid ? auction_event_type::sold :
							  auction_event_type::expired;
	}
	else if (payload.action == auction_action::claim_money)
	{
		if (!execute(connection, "SELECT money FROM auction_money_pickups WHERE pid=" +
						 std::to_string(payload.actor_pid) + " FOR UPDATE"))
			return false;
		MYSQL_RES *query = mysql_store_result(connection);
		MYSQL_ROW row = query ? mysql_fetch_row(query) : nullptr;
		uint64_t money = 0;
		const bool found = row && parse_u64(row[0], &money) && money > 0 &&
				   money <= INT_MAX;
		if (query)
			mysql_free_result(query);
		if (!found)
		{
			*result_code = ENOENT;
			return true;
		}
		if (!apply_wallet_delta(connection, command, payload, static_cast<int64_t>(money),
					&wallet, result_code))
			return false;
		if (*result_code)
			return true;
		if (!execute(connection,
			     "UPDATE auction_money_pickups SET money=0,claim_revision=claim_revision+1 "
			     "WHERE pid=" +
				     std::to_string(payload.actor_pid) +
				     " AND money=" + std::to_string(money)) ||
		    mysql_affected_rows(connection) != 1)
			return false;
		result->wallet_value_delta = static_cast<int64_t>(money);
		result->wallet = wallet.wallet;
		result->bank = wallet.bank;
		result->wallet_revision = wallet.wallet_revision;
		result->bank_revision = wallet.bank_revision;
		result->event_type = auction_event_type::money_claimed;
		result->auction_revision = wallet.wallet_revision;
	}
	else if (payload.action == auction_action::claim_item)
	{
		auction_state auction = {};
		if (!lock_auction(connection, payload.auction_id, &auction, result_code))
			return false;
		if (*result_code)
			return true;
		std::array<size_t, AUCTION_COMMAND_MAX_ITEMS> custody_order{};
		for (size_t i = 0; i < payload.item_count; ++i)
			custody_order[i] = i;
		if (native)
			std::sort(custody_order.begin(), custody_order.begin() + payload.item_count,
				  [&](size_t a, size_t b) {
					  return payload.items[a].item_uid <
						 payload.items[b].item_uid;
				  });
		for (size_t position = 0; position < payload.item_count; ++position)
		{
			const size_t index = custody_order[position];
			if (!execute(
				    connection,
				    (native ?
					     "SELECT item_revision,obj_blob FROM auction_item_custody WHERE auction_id=" :
					     "SELECT item_revision FROM auction_item_custody WHERE auction_id=") +
					    std::to_string(auction.id) + " AND item_uid=" +
					    std::to_string(payload.items[index].item_uid) +
					    " AND claim_pid=" + std::to_string(payload.actor_pid) +
					    " AND claimed_at IS NULL FOR UPDATE"))
				return false;
			MYSQL_RES *query = mysql_store_result(connection);
			MYSQL_ROW row = query ? mysql_fetch_row(query) : nullptr;
			uint64_t revision = 0;
			bool found = query && mysql_num_rows(query) == 1 && row &&
				     parse_u64(row[0], &revision) &&
				     revision == payload.items[index].expected_item_revision;
			if (found && native)
			{
				const unsigned long *lengths = mysql_fetch_lengths(query);
				const bool canonical =
					mysql_num_fields(query) == 2 && row[1] && lengths &&
					native_tree_bind_custody(
						std::span(reinterpret_cast<const uint8_t *>(row[1]),
							  lengths[1]),
						*native, index);
				if (!canonical)
				{
					mysql_free_result(query);
					errno = EILSEQ;
					return false;
				}
			}
			if (query)
				mysql_free_result(query);
			if (!found)
			{
				*result_code = ESTALE;
				return true;
			}
		}
		if (!(native ? transition_native_tree(connection, command, payload, auction.id,
						      item_owner_type::auction, auction.id,
						      item_owner_type::player, payload.actor_pid,
						      *native, result, result_code) :
			       transition_items(connection, command, payload, auction.id,
						item_owner_type::auction, auction.id,
						item_owner_type::player, payload.actor_pid, result,
						result_code)))
			return false;
		if (*result_code)
			return true;
		const std::string op = operation_hex(command.operation_id);
		for (size_t index = 0; index < payload.item_count; ++index)
			if (!execute(connection,
				     "UPDATE auction_item_custody SET item_revision=" +
					     std::to_string(result->item_revisions[index]) +
					     ",claim_operation_id=UNHEX('" + op +
					     "'),claimed_at=CURRENT_TIMESTAMP(6) WHERE auction_id=" +
					     std::to_string(auction.id) + " AND item_uid=" +
					     std::to_string(payload.items[index].item_uid) +
					     " AND claimed_at IS NULL") ||
			    mysql_affected_rows(connection) != 1)
				return false;
		result->auction_id = auction.id;
		result->status = auction.status;
		result->seller_pid = auction.seller_pid;
		result->winner_pid = payload.actor_pid;
		result->auction_revision = auction.revision + 1;
		result->event_type = auction_event_type::item_claimed;
		if (!execute(connection,
			     "UPDATE auctions SET auction_revision=auction_revision+1 WHERE id=" +
				     std::to_string(auction.id) +
				     " AND auction_revision=" + std::to_string(auction.revision)) ||
		    mysql_affected_rows(connection) != 1)
			return false;
	}
	else
	{
		*result_code = EINVAL;
		return true;
	}
	if (!write_auction_ledger(connection, command, payload, *result))
		return false;
	*mutation_applied = true;
	return true;
}

bool auction_repository_execute(MYSQL *connection, const critical_command &command,
				auction_command_result *result, unsigned int *result_code,
				bool *mutation_applied)
{
	if (!critical_command_legacy_execution_supported(command))
	{
		errno = EPROTONOSUPPORT;
		return false;
	}
	return auction_repository_execute_impl(connection, command, result, result_code,
					       mutation_applied);
}

bool auction_repository_execute_accounted(MYSQL *connection, const critical_command &command,
					  auction_command_result *result, unsigned int *result_code,
					  bool *mutation_applied)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)result;
	(void)result_code;
	(void)mutation_applied;
	errno = ENOTSUP;
	return false;
#else
	economic_frozen_intent intent;
	auction_command_payload payload = {};
	if (!connection || !(connection->server_status & SERVER_STATUS_IN_TRANS) ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    command.payload_version != AUCTION_COMMAND_PAYLOAD_VERSION ||
	    !critical_command_envelope_valid(command) ||
	    !auction_command_decode_payload(command, &payload) ||
	    (payload.action != auction_action::list && payload.action != auction_action::bid &&
	     payload.action != auction_action::claim_money &&
	     payload.action != auction_action::claim_item &&
	     payload.action != auction_action::finalize &&
	     payload.action != auction_action::remove) ||
	    economic_intent_decode(command.accounting_intent, &intent) !=
		    economic_accounting_error::ok ||
	    economic_intent_verify_binding(command, intent) != economic_accounting_error::ok ||
	    !((payload.action == auction_action::list &&
	       intent.admission.metadata.writer_id == ECONOMIC_WRITER_AUCTION_LISTING &&
	       intent.admission.metadata.reason == economic_reason::auction_listing) ||
	      (payload.action == auction_action::bid &&
	       intent.admission.metadata.writer_id == ECONOMIC_WRITER_AUCTION_BID &&
	       intent.admission.metadata.reason == economic_reason::auction_bid) ||
	      (payload.action == auction_action::claim_item &&
	       intent.admission.metadata.writer_id == ECONOMIC_WRITER_AUCTION_ITEM_CLAIM &&
	       intent.admission.metadata.reason == economic_reason::auction_claim) ||
	      (payload.action == auction_action::claim_money &&
	       intent.admission.metadata.writer_id == ECONOMIC_WRITER_AUCTION_MONEY_CLAIM &&
	       intent.admission.metadata.reason == economic_reason::auction_claim) ||
	      ((payload.action == auction_action::finalize ||
		payload.action == auction_action::remove) &&
	       intent.admission.metadata.writer_id == ECONOMIC_WRITER_AUCTION_SETTLEMENT &&
	       intent.admission.metadata.reason == (payload.action == auction_action::remove ?
							    economic_reason::auction_cancel :
							    economic_reason::auction_settle))))
	{
		errno = EPROTONOSUPPORT;
		return false;
	}
	return auction_repository_execute_impl(connection, command, result, result_code,
					       mutation_applied);
#endif
}

bool auction_repository_decode_native_tree_blob(std::span<const uint8_t> blob,
						std::vector<player_item_snapshot> *items_out,
						std::vector<uint64_t> *revisions_out) noexcept
{
	try
	{
		if (!items_out || !revisions_out ||
		    blob.size() > PLAYER_SNAPSHOT_MAX_BYTES + 12 + PLAYER_SNAPSHOT_MAX_OBJECTS * 16)
			return false;
		size_t at = 0;
		uint32_t magic = 0, count = 0, length = 0;
		if (!native_tree_read(blob, at, &magic) || magic != 0x32544341 ||
		    !native_tree_read(blob, at, &count) || !count ||
		    count > PLAYER_SNAPSHOT_MAX_OBJECTS || !native_tree_read(blob, at, &length) ||
		    blob.size() - at < static_cast<size_t>(count) * 16)
			return false;
		std::vector<uint64_t> uids, revisions;
		uids.reserve(count);
		revisions.reserve(count);
		for (uint32_t i = 0; i < count; ++i)
		{
			uint64_t uid = 0, revision = 0;
			if (!native_tree_read(blob, at, &uid) || !uid || uid == UINT64_MAX ||
			    !native_tree_read(blob, at, &revision) || !revision ||
			    revision == UINT64_MAX)
				return false;
			uids.push_back(uid);
			revisions.push_back(revision);
		}
		if (blob.size() - at != length)
			return false;
		std::vector<player_item_snapshot> items;
		if (player_item_snapshot_list_decode(blob.data() + at, length, &items) !=
			    player_snapshot_codec_result::ok ||
		    items.size() != count)
			return false;
		auction_command_payload payload{};
		payload.item_count = 1;
		payload.items[0].item_uid = uids[0];
		payload.items[0].vnum = items[0].vnum;
		native_tree_participant shape;
		if (!native_tree_shape(payload, items, &shape))
			return false;
		for (size_t i = 0; i < items.size(); ++i)
			if (items[i].object_uid != uids[i])
				return false;
		std::vector<uint8_t> canonical;
		if (player_item_snapshot_list_encode(items, &canonical) !=
			    player_snapshot_codec_result::ok ||
		    canonical.size() != length ||
		    !std::equal(canonical.begin(), canonical.end(), blob.begin() + at))
			return false;
		items_out->swap(items);
		revisions_out->swap(revisions);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool auction_repository_execute_accounted_native(
	MYSQL *connection, const critical_command &command,
	std::span<const player_item_snapshot> original_selected, auction_command_result *result,
	unsigned int *result_code, bool *mutation_applied)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)original_selected;
	(void)result;
	(void)result_code;
	(void)mutation_applied;
	errno = ENOTSUP;
	return false;
#else
	try
	{
		if (!connection || !result || !result_code || !mutation_applied)
		{
			errno = EINVAL;
			return false;
		}
		using reconnect_flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
		reconnect_flag reconnect = false;
		if (mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) || reconnect)
		{
			errno = EINVAL;
			return false;
		}
		const unsigned long session = mysql_thread_id(connection);
		if (!session || !(connection->server_status & SERVER_STATUS_IN_TRANS))
		{
			errno = ENOTCONN;
			return false;
		}
		auction_native_command_context context;
		if (command.type != critical_command_type::auction ||
		    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
		    command.payload_version != AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION ||
		    !auction_repository_frozen_accounting_valid(command) ||
		    auction_native_command_decode(command, &context) !=
			    economic_accounting_error::ok)
		{
			errno = EPROTONOSUPPORT;
			return false;
		}
		native_tree_participant native;
		if (!native_tree_shape(context.payload, original_selected, &native) ||
		    original_selected.size() != context.selected_node_count ||
		    native.roots.size() != context.selected_root_count)
		{
			errno = EILSEQ;
			return false;
		}
		std::vector<uint8_t> selected_bytes;
		std::array<uint8_t, 32> hash{};
		if (player_item_snapshot_list_encode(native.nodes, &selected_bytes) !=
			    player_snapshot_codec_result::ok ||
		    !SHA256(selected_bytes.data(), selected_bytes.size(), hash.data()) ||
		    hash != context.selected_digest)
		{
			errno = EILSEQ;
			return false;
		}
		auction_command_result prepared{};
		unsigned int code = 0;
		bool mutated = false;
		const bool items = context.payload.action == auction_action::list ||
				   context.payload.action == auction_action::claim_item;
		if (!auction_repository_execute_impl(connection, command, &prepared, &code,
						     &mutated, items ? &native : nullptr))
		{
			if (mysql_thread_id(connection) != session ||
			    !(connection->server_status & SERVER_STATUS_IN_TRANS))
			{
				errno = ENOTCONN;
				return false;
			}
			const unsigned int sql_error = mysql_errno(connection);
			if (sql_error)
				errno = sql_error;
			else if (!errno)
				errno = EIO;
			return false;
		}
		if (mysql_thread_id(connection) != session ||
		    !(connection->server_status & SERVER_STATUS_IN_TRANS))
		{
			errno = ENOTCONN;
			return false;
		}
		*result = prepared;
		*result_code = code;
		*mutation_applied = mutated;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return false;
	}
	catch (...)
	{
		errno = EILSEQ;
		return false;
	}
#endif
}

// Read-only producer capture. These facts confer no execution authority: the
// admitted SQL owners renew every mapping and native prestate in their transaction.
#ifndef __NO_MYSQL__
namespace
{
std::string auction_capture_id(const critical_operation_id &value)
{
	return "UNHEX('" + operation_hex(value) + "')";
}

bool auction_capture_rows(MYSQL *connection, const std::string &sql, size_t fields,
			  std::vector<std::vector<std::string>> *values)
{
	if (!execute(connection, sql))
	{
		errno = mysql_errno(connection) ? static_cast<int>(mysql_errno(connection)) : EIO;
		return false;
	}
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	if (!rows || mysql_num_fields(rows.get()) != fields)
	{
		errno = EILSEQ;
		return false;
	}
	std::vector<std::vector<std::string>> result;
	while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
	{
		const unsigned long *lengths = mysql_fetch_lengths(rows.get());
		if (!lengths)
		{
			errno = EILSEQ;
			return false;
		}
		std::vector<std::string> cells;
		for (size_t index = 0; index < fields; ++index)
		{
			if (!row[index])
			{
				errno = EILSEQ;
				return false;
			}
			cells.emplace_back(row[index], lengths[index]);
		}
		result.push_back(std::move(cells));
	}
	if (mysql_errno(connection))
	{
		errno = static_cast<int>(mysql_errno(connection));
		return false;
	}
	*values = std::move(result);
	return true;
}

bool auction_capture_row(MYSQL *connection, const std::string &sql, size_t fields,
			 std::vector<std::string> *values, bool optional = false)
{
	std::vector<std::vector<std::string>> rows;
	if (!auction_capture_rows(connection, sql, fields, &rows))
		return false;
	if (rows.size() > 1 || (!optional && rows.empty()))
	{
		errno = EILSEQ;
		return false;
	}
	*values = rows.empty() ? std::vector<std::string>{} : std::move(rows[0]);
	return true;
}

bool auction_capture_mapping(MYSQL *connection, const critical_operation_id &lineage,
			     economic_account_kind kind, uint16_t locator, uint64_t native,
			     uint64_t context, economic_account_key *output, bool optional = false)
{
	std::vector<std::string> row;
	if (!auction_capture_row(
		    connection,
		    "SELECT mapping_id FROM economic_account_mapping WHERE lineage=" +
			    auction_capture_id(lineage) +
			    " AND account_kind=" + std::to_string(static_cast<uint16_t>(kind)) +
			    " AND context_id=" + std::to_string(context) +
			    " AND backend_kind=1 AND locator_kind=" + std::to_string(locator) +
			    " AND native_id=" + std::to_string(native) +
			    " AND active_native_id=" + std::to_string(native),
		    1, &row, optional))
		return false;
	economic_account_key key{};
	if (!row.empty())
	{
		uint64_t mapping = 0;
		if (!parse_u64(row[0].c_str(), &mapping) || !mapping)
		{
			errno = EILSEQ;
			return false;
		}
		key = { lineage, kind, mapping, context };
	}
	*output = key;
	return true;
}

bool auction_capture_claim(MYSQL *connection, const critical_operation_id &lineage, uint32_t pid,
			   economic_account_key *key, uint32_t *absent)
{
	std::vector<std::string> row;
	if (!auction_capture_row(
		    connection,
		    "SELECT money,claim_revision FROM auction_money_pickups WHERE pid=" +
			    std::to_string(pid) + " FOR UPDATE",
		    2, &row, true) ||
	    !auction_capture_mapping(connection, lineage, economic_account_kind::pending_claim, 5,
				     pid, 0, key, true))
		return false;
	// Row presence is independent of amount and revision, including an existing zero.
	if (!row.empty())
	{
		uint64_t money = 0, revision = 0;
		if (!key->authority_id || !parse_u64(row[0].c_str(), &money) || money > UINT_MAX ||
		    !parse_u64(row[1].c_str(), &revision))
		{
			errno = EILSEQ;
			return false;
		}
		*absent = 0;
		return true;
	}
	std::vector<std::vector<std::string>> retained;
	const auto scope = auction_capture_id(lineage), native = std::to_string(pid);
	if (key->authority_id ||
	    !auction_capture_rows(
		    connection,
		    "SELECT mapping_id FROM economic_account_mapping WHERE lineage=" + scope +
			    " AND account_kind=5 AND native_id=" + native + " FOR UPDATE",
		    1, &retained))
	{
		if (key->authority_id)
			errno = EILSEQ;
		return false;
	}
	if (!retained.empty())
	{
		errno = EILSEQ;
		return false;
	}
	if (!auction_capture_rows(
		    connection,
		    "SELECT source_slot FROM economic_pending_claim_source WHERE lineage=" + scope +
			    " AND beneficiary_pid=" + native + " FOR UPDATE",
		    1, &retained))
		return false;
	if (!retained.empty())
	{
		errno = EILSEQ;
		return false;
	}
	*absent = pid;
	return true;
}

bool auction_capture_listing(MYSQL *connection, uint32_t auction,
			     auction_settlement_listing *listing)
{
	std::vector<std::string> row;
	if (!auction_capture_row(
		    connection,
		    "SELECT seller_pid,winning_bidder_pid,status+0,custody_state,quantity,cur_price,"
		    "buy_price,auction_revision,UNIX_TIMESTAMP(end_time),HEX(listing_operation_id) "
		    "FROM auctions WHERE id=" +
			    std::to_string(auction) + " FOR UPDATE",
		    10, &row))
		return false;
	uint64_t numbers[9]{};
	for (size_t index = 0; index < 9; ++index)
		if (!parse_u64(row[index].c_str(), &numbers[index]) ||
		    (index < 5 && numbers[index] > UINT32_MAX))
		{
			errno = EILSEQ;
			return false;
		}
	if (numbers[5] > INT64_MAX || numbers[6] > INT64_MAX || row[9].size() != 32 ||
	    !critical_operation_id_from_hex(row[9].c_str(), &listing->listing_operation))
	{
		errno = EILSEQ;
		return false;
	}
	listing->auction_id = auction;
	listing->seller_pid = static_cast<uint32_t>(numbers[0]);
	listing->winner_pid = static_cast<uint32_t>(numbers[1]);
	listing->status = static_cast<uint32_t>(numbers[2]);
	listing->custody_state = static_cast<uint32_t>(numbers[3]);
	listing->quantity = static_cast<uint32_t>(numbers[4]);
	listing->current_price = static_cast<int64_t>(numbers[5]);
	listing->buy_price = static_cast<int64_t>(numbers[6]);
	listing->revision = numbers[7];
	listing->end_time = numbers[8];
	if (!listing->winner_pid)
		return true;
	if (!auction_capture_row(
		    connection,
		    "SELECT HEX(operation_id),actor_pid,final_price FROM auction_ledger WHERE auction_id=" +
			    std::to_string(auction) + " AND auction_revision=" +
			    std::to_string(listing->revision) + " AND event_type=2 FOR UPDATE",
		    3, &row))
		return false;
	uint64_t bidder = 0, price = 0;
	if (row[0].size() != 32 ||
	    !critical_operation_id_from_hex(row[0].c_str(), &listing->winning_bid_operation) ||
	    !parse_u64(row[1].c_str(), &bidder) || bidder != listing->winner_pid ||
	    !parse_u64(row[2].c_str(), &price) ||
	    price != static_cast<uint64_t>(listing->current_price))
	{
		errno = EILSEQ;
		return false;
	}
	return true;
}

bool auction_capture_items(MYSQL *connection, auction_settlement_listing *listing)
{
	if (!listing->quantity || listing->quantity > AUCTION_COMMAND_MAX_ITEMS)
	{
		errno = EOPNOTSUPP;
		return false;
	}
	std::vector<std::vector<std::string>> rows;
	if (!auction_capture_rows(
		    connection,
		    "SELECT item_uid,item_revision,slot,vnum,COALESCE(claim_pid,0),claimed_at IS NOT NULL "
		    "FROM auction_item_custody WHERE auction_id=" +
			    std::to_string(listing->auction_id) + " ORDER BY slot FOR UPDATE",
		    6, &rows))
		return false;
	if (rows.size() != listing->quantity)
	{
		errno = EILSEQ;
		return false;
	}
	listing->item_count = static_cast<uint16_t>(rows.size());
	for (size_t index = 0; index < rows.size(); ++index)
	{
		uint64_t fields[6]{};
		for (size_t field = 0; field < 6; ++field)
			if (!parse_u64(rows[index][field].c_str(), &fields[field]))
			{
				errno = EILSEQ;
				return false;
			}
		if (fields[2] != index || fields[3] > INT32_MAX || fields[4] > UINT32_MAX ||
		    fields[5] > 1)
		{
			errno = EILSEQ;
			return false;
		}
		listing->items[index] = { fields[0],
					  fields[1],
					  static_cast<uint16_t>(index),
					  static_cast<int32_t>(fields[3]),
					  static_cast<uint32_t>(fields[4]),
					  fields[5] != 0 };
	}
	return true;
}

// Fresh three-route facts are read only from the caller's trusted native SQL cut.
// The gameplay owner separately retains the original physical inventory/save body.
bool auction_capture_roots(MYSQL *connection, const auction_command_payload &payload,
			   item_owner_type type, uint64_t owner)
{
	std::vector<size_t> order(payload.item_count);
	for (size_t i = 0; i < order.size(); ++i)
		order[i] = i;
	std::sort(order.begin(), order.end(), [&](size_t a, size_t b)
		  { return payload.items[a].item_uid < payload.items[b].item_uid; });
	for (size_t index : order)
	{
		const auto &item = payload.items[index];
		std::vector<std::string> row;
		if (!auction_capture_row(
			    connection,
			    "SELECT root_item_uid,COALESCE(parent_item_uid,0),owner_type,owner_id,owner_context_id,item_revision,vnum,state FROM item_current_owner WHERE item_uid=" +
				    std::to_string(item.item_uid) + " FOR UPDATE",
			    8, &row))
			return false;
		uint64_t fields[8]{};
		for (size_t j = 0; j < 8; ++j)
			if (!parse_u64(row[j].c_str(), &fields[j]))
			{
				errno = EILSEQ;
				return false;
			}
		if (fields[0] != item.item_uid || fields[1] ||
		    fields[2] != static_cast<unsigned>(type) || fields[3] != owner || fields[4] ||
		    fields[5] != item.expected_item_revision ||
		    fields[6] != static_cast<uint64_t>(item.vnum) ||
		    fields[7] != static_cast<unsigned>(item_custody_state::active))
		{
			errno = ESTALE;
			return false;
		}
		if (!auction_capture_row(
			    connection,
			    "SELECT item_uid FROM item_current_owner WHERE root_item_uid=" +
				    std::to_string(item.item_uid) + " AND item_uid<>" +
				    std::to_string(item.item_uid) + " LIMIT 1 FOR UPDATE",
			    1, &row, true))
			return false;
		if (!row.empty())
		{
			errno = EOPNOTSUPP;
			return false;
		}
	}
	return true;
}

// Fresh native capture proves stored original acknowledged values, never held
// post-admission bodies or a recapture of committed historical item source.
bool auction_capture_forest_digest(const std::vector<player_item_snapshot> &items,
				   const std::array<uint8_t, 32> &expected)
{
	std::vector<uint8_t> encoded;
	std::array<uint8_t, 32> digest{};
	const auto encoded_status = player_item_snapshot_list_encode(items, &encoded);
	if (encoded_status != player_snapshot_codec_result::ok)
	{
		errno = encoded_status == player_snapshot_codec_result::allocation_failure ?
				ENOMEM :
			encoded_status == player_snapshot_codec_result::limit_exceeded ? E2BIG :
											 EILSEQ;
		return false;
	}
	if (!SHA256(encoded.data(), encoded.size(), digest.data()) || digest != expected)
	{
		errno = EILSEQ;
		return false;
	}
	return true;
}

bool auction_capture_native_fence(critical_command *command, uint64_t uid, uint64_t revision,
				  bool root)
{
	if (!uid || uid == UINT64_MAX || !revision || revision == UINT64_MAX)
	{
		errno = EILSEQ;
		return false;
	}
	const critical_entity_key key{ critical_entity_type::item, uid };
	auto found = std::find_if(command->expected_revisions.begin(),
				  command->expected_revisions.end(), [&](const auto &f)
				  { return critical_entity_key_equal(f.key, key); });
	const auto count = std::count_if(command->keys.begin(), command->keys.end(),
					 [&](const auto &k)
					 { return critical_entity_key_equal(k, key); });
	if (found != command->expected_revisions.end())
	{
		if (count != 1 || found->revision != revision)
		{
			errno = ESTALE;
			return false;
		}
		return true;
	}
	if (root || count)
	{
		errno = ESTALE;
		return false;
	}
	command->keys.push_back(key);
	command->expected_revisions.push_back({ key, revision });
	return true;
}

bool auction_capture_native_cut(MYSQL *connection, const auction_native_command_context &native,
				critical_command *command,
				std::span<const economic_item_snapshot> original_source = {},
				std::span<const player_item_snapshot> original_literals = {})
{
	const auto &payload = native.payload;
	std::vector<std::string> row;
	if (payload.auction_id &&
	    !auction_capture_row(connection,
				 "SELECT auction_revision FROM auctions WHERE id=" +
					 std::to_string(payload.auction_id) + " FOR UPDATE",
				 1, &row))
		return false;
	// Stable owner revision locks precede globally UID-ordered item rows and the
	// original physical/sidecar reader. Missing enrollment is never synthesized.
	for (const auto owner :
	     { item_owner_identity{ item_owner_type::player, payload.actor_pid, 0 },
	       item_owner_identity{ item_owner_type::auction, payload.auction_id, 0 } })
	{
		if (!owner.id)
			continue;
		if (!auction_capture_row(
			    connection,
			    "SELECT revision FROM item_owner_revision WHERE owner_type=" +
				    std::to_string(static_cast<unsigned>(owner.type)) +
				    " AND owner_id=" + std::to_string(owner.id) +
				    " AND owner_context_id=0 FOR UPDATE",
			    1, &row,
			    owner.type == item_owner_type::player &&
				    native.before_item_uids.empty()))
			return false;
		if (!row.empty())
		{
			uint64_t revision = 0;
			if (!parse_u64(row[0].c_str(), &revision) || revision == UINT64_MAX)
			{
				errno = EILSEQ;
				return false;
			}
		}
	}
	std::string roots;
	for (size_t i = 0; i < payload.item_count; ++i)
	{
		if (i)
			roots += ',';
		roots += std::to_string(payload.items[i].item_uid);
	}
	for (const auto &source : original_source)
		if (!source.position.parent_uid)
		{
			if (!roots.empty())
				roots += ',';
			roots += std::to_string(source.uid);
		}
	std::string native_uids;
	for (const auto uid : native.before_item_uids)
	{
		if (!native_uids.empty())
			native_uids += ',';
		native_uids += std::to_string(uid);
	}
	for (const auto &source : original_source)
	{
		if (!native_uids.empty())
			native_uids += ',';
		native_uids += std::to_string(source.uid);
	}
	std::string where = "(owner_type=1 AND owner_id=" + std::to_string(payload.actor_pid) +
			    " AND owner_context_id=0 AND state=1 AND coin_payload IS NULL)";
	if (!native_uids.empty())
		where += " OR item_uid IN(" + native_uids + ") OR root_item_uid IN(" + native_uids +
			 ") OR parent_item_uid IN(" + native_uids + ")";
	if (!roots.empty())
		where += " OR root_item_uid IN(" + roots + ")";
	if (payload.auction_id)
		where += " OR (owner_type=" +
			 std::to_string(static_cast<unsigned>(item_owner_type::auction)) +
			 " AND owner_id=" + std::to_string(payload.auction_id) +
			 " AND owner_context_id=0)";
	std::vector<std::vector<std::string>> current;
	if (!auction_capture_rows(
		    connection,
		    "SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),owner_type,owner_id,owner_context_id,item_revision,vnum,state,coin_payload IS NULL,COALESCE(equipment_slot,0) FROM item_current_owner WHERE " +
			    where + " ORDER BY item_uid FOR UPDATE",
		    11, &current))
		return false;
	std::map<uint64_t, std::array<uint64_t, 11>> custody;
	for (const auto &values : current)
	{
		std::array<uint64_t, 11> fields{};
		for (size_t i = 0; i < fields.size(); ++i)
			if (!parse_u64(values[i].c_str(), &fields[i]))
			{
				errno = EILSEQ;
				return false;
			}
		if (!custody.emplace(fields[0], fields).second)
		{
			errno = EILSEQ;
			return false;
		}
	}
	if (!original_source.empty())
	{
		size_t observed = 0;
		for (const auto &[uid, fields] : custody)
		{
			const auto found = std::find_if(original_source.begin(),
							original_source.end(), [&](const auto &w)
							{ return w.uid == uid; });
			const auto source_uid = [&](uint64_t value)
			{
				return std::any_of(original_source.begin(), original_source.end(),
						   [&](const auto &w) { return w.uid == value; });
			};
			if (found == original_source.end())
			{
				if ((fields[3] == static_cast<unsigned>(item_owner_type::auction) &&
				     fields[4] == payload.auction_id) ||
				    source_uid(fields[1]) || source_uid(fields[2]))
				{
					errno = EILSEQ;
					return false;
				}
				continue;
			}
			if (!original_literals.empty())
			{
				const auto literal = std::find_if(original_literals.begin(),
								  original_literals.end(),
								  [&](const auto &v)
								  { return v.object_uid == uid; });
				if (literal == original_literals.end() ||
				    fields[7] != static_cast<uint64_t>(literal->vnum))
				{
					errno = EILSEQ;
					return false;
				}
			}
			++observed;
			const auto &position = found->position;
			if (fields[1] != position.root_uid || fields[2] != position.parent_uid ||
			    fields[3] != static_cast<unsigned>(position.owner.type) ||
			    fields[4] != position.owner.id ||
			    fields[5] != position.owner.context_id ||
			    fields[6] != position.revision ||
			    fields[8] != static_cast<unsigned>(position.state) || !fields[9] ||
			    fields[10] != position.equipment_slot)
			{
				errno = ESTALE;
				return false;
			}
		}
		if (observed != original_source.size())
		{
			errno = EILSEQ;
			return false;
		}
	}
	std::vector<player_item_snapshot> before, selected, after;
	shop_item_runtime_image image;
	if (payload.actor_pid)
	{
		if (!auction_capture_row(connection,
					 "SELECT level,save_revision FROM player_data WHERE pid=" +
						 std::to_string(payload.actor_pid) + " FOR UPDATE",
					 2, &row))
			return false;
		uint64_t level = 0, revision = 0;
		if (!parse_u64(row[0].c_str(), &level) || !parse_u64(row[1].c_str(), &revision) ||
		    level != native.original_level || revision != native.acknowledged_save_revision)
		{
			errno = ESTALE;
			return false;
		}
		if (!shop_item_runtime_lock_player_image(
			    connection, payload.actor_pid,
			    std::span<const uint64_t>(native.before_item_uids), &image))
			return false;
		before.reserve(native.before_item_uids.size());
		for (const auto uid : native.before_item_uids)
		{
			const auto found = image.find(uid);
			if (found == image.end() || !found->second.payload_present)
			{
				errno = EILSEQ;
				return false;
			}
			before.push_back(found->second.item);
		}
	}
	if (!auction_capture_forest_digest(before, native.before_digest))
		return false;
	if (payload.action == auction_action::list)
	{
		for (size_t i = 0; i < payload.item_count; ++i)
		{
			const auto found = image.find(payload.items[i].item_uid);
			if (found == image.end() || found->second.slot ||
			    found->second.item.parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
			    found->second.item.vnum != payload.items[i].vnum)
			{
				errno = ESTALE;
				return false;
			}
			std::vector<player_item_snapshot> tree, remaining;
			const auto extracted = player_item_snapshot_extract_subtree(
				before, payload.items[i].item_uid, &tree, &remaining);
			if (extracted != player_snapshot_codec_result::ok || tree.empty())
			{
				errno = extracted == player_snapshot_codec_result::allocation_failure ?
						ENOMEM :
					extracted == player_snapshot_codec_result::limit_exceeded ?
						E2BIG :
						EILSEQ;
				return false;
			}
			if (i == 0 && tree.size() != 1)
			{
				errno = EILSEQ;
				return false;
			}
			const auto offset = selected.size();
			for (auto &value : tree)
			{
				if (value.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
					value.parent_index += static_cast<int32_t>(offset);
				selected.push_back(std::move(value));
			}
		}
	}
	else if (payload.action == auction_action::claim_item)
	{
		if (!auction_capture_row(connection,
					 "SELECT HEX(listing_operation_id) FROM auctions WHERE id=" +
						 std::to_string(payload.auction_id) + " FOR UPDATE",
					 1, &row))
			return false;
		critical_operation_id listing{};
		if (row[0].size() != 32 ||
		    !critical_operation_id_from_hex(row[0].c_str(), &listing))
		{
			errno = EILSEQ;
			return false;
		}
		// This is the genuine current request's base prefix, not reconstruction of
		// an unavailable historical listing header. Original caller fences stay on
		// the native command; the base request is used only to select entitled roots.
		critical_command request;
		if (!auction_command_build(&request, command->operation_id, payload,
					   command->source_site, command->deadline_class))
		{
			errno = EILSEQ;
			return false;
		}
		const auto error = auction_repository_read_original_native_selected(
			connection, request, listing, &selected);
		if (error)
		{
			errno = error;
			return false;
		}
	}
	if (selected.size() != native.selected_node_count ||
	    (payload.action == auction_action::list ||
			     payload.action == auction_action::claim_item ?
		     payload.item_count != native.selected_root_count :
		     native.selected_root_count != 0) ||
	    !auction_capture_forest_digest(selected, native.selected_digest) ||
	    !auction_native_expected_player_forest(payload, before, selected, false,
						   native.original_level, &after) ||
	    !auction_capture_forest_digest(after, native.after_digest))
	{
		if (!errno)
			errno = EILSEQ;
		return false;
	}
	size_t root = 0;
	uint64_t root_uid = 0;
	for (size_t i = 0; i < selected.size(); ++i)
	{
		const auto &value = selected[i];
		const bool is_root = value.parent_index == PLAYER_SNAPSHOT_NO_PARENT;
		if (is_root)
		{
			if (root >= payload.item_count ||
			    value.object_uid != payload.items[root].item_uid)
			{
				errno = EILSEQ;
				return false;
			}
			root_uid = value.object_uid;
			++root;
		}
		const auto found = custody.find(value.object_uid);
		if (found == custody.end())
		{
			errno = EILSEQ;
			return false;
		}
		const auto &f = found->second;
		const auto owner = payload.action == auction_action::list ?
					   item_owner_type::player :
					   item_owner_type::auction;
		const auto owner_id = payload.action == auction_action::list ? payload.actor_pid :
									       payload.auction_id;
		const uint64_t parent =
			is_root ? 0 : selected[static_cast<size_t>(value.parent_index)].object_uid;
		if (f[1] != root_uid || f[2] != parent || f[3] != static_cast<unsigned>(owner) ||
		    f[4] != owner_id || f[5] || f[7] != static_cast<uint64_t>(value.vnum) ||
		    f[8] != static_cast<unsigned>(item_custody_state::active) || !f[9] ||
		    (is_root && f[6] != payload.items[root - 1].expected_item_revision) ||
		    !auction_capture_native_fence(command, value.object_uid, f[6], is_root))
		{
			if (!errno)
				errno = ESTALE;
			return false;
		}
	}
	// Bid may stage terminal custody; finalize/remove preserve actual complete
	// auction topology. Freeze all current native auction node fences without
	// replacing any stale fence supplied by the original request.
	if (payload.action == auction_action::bid || payload.action == auction_action::finalize ||
	    payload.action == auction_action::remove)
		for (const auto &[uid, f] : custody)
			if (f[3] == static_cast<unsigned>(item_owner_type::auction) &&
			    f[4] == payload.auction_id)
			{
				if (f[5] ||
				    f[8] != static_cast<unsigned>(item_custody_state::active) ||
				    !f[9] ||
				    !auction_capture_native_fence(command, uid, f[6], false))
				{
					if (!errno)
						errno = EILSEQ;
					return false;
				}
			}
	std::sort(command->keys.begin(), command->keys.end(), critical_entity_key_less);
	std::sort(command->expected_revisions.begin(), command->expected_revisions.end(),
		  [](const auto &a, const auto &b)
		  { return critical_entity_key_less(a.key, b.key); });
	if (!critical_command_envelope_valid(*command))
	{
		errno = E2BIG;
		return false;
	}
	return true;
}

unsigned int auction_capture_three(MYSQL *connection, const critical_operation_id &lineage,
				   const critical_operation_id &epoch,
				   const auction_command_payload &payload,
				   critical_command *command)
{
	const auto session = mysql_thread_id(connection);
	std::vector<std::string> cells;
	if (!payload.actor_pid)
		return EINVAL;
	if (!auction_capture_row(
		    connection,
		    "SELECT account_name,racewar,wallet_revision,copper,silver,gold,platinum FROM player_data WHERE pid=" +
			    std::to_string(payload.actor_pid) + " FOR UPDATE",
		    7, &cells))
		return errno ? errno : EINVAL;
	uint64_t race = 0, revision = 0;
	if (cells[0].size() != strnlen(payload.account_name.data(), payload.account_name.size()) ||
	    strcasecmp(cells[0].c_str(), payload.account_name.data()) ||
	    !parse_u64(cells[1].c_str(), &race) || race != payload.racewar ||
	    !parse_u64(cells[2].c_str(), &revision) || revision != payload.expected_wallet_revision)
		return ESTALE;
	for (size_t i = 3; i < 7; ++i)
	{
		uint64_t amount = 0;
		if (!parse_u64(cells[i].c_str(), &amount) || amount > INT64_MAX)
			return EILSEQ;
	}
	const auto account =
		"'" +
		escape(connection, payload.account_name.data(),
		       strnlen(payload.account_name.data(), payload.account_name.size())) +
		"'";
	if (!auction_capture_row(
		    connection,
		    "SELECT id,bank_revision,bank_copper,bank_silver,bank_gold,bank_platinum FROM account_banks WHERE account_name=" +
			    account + " AND racewar=" + std::to_string(payload.racewar) +
			    " FOR UPDATE",
		    6, &cells))
		return errno ? errno : EILSEQ;
	uint64_t bank_id = 0;
	if (!parse_u64(cells[0].c_str(), &bank_id) || !bank_id || bank_id > UINT32_MAX ||
	    !parse_u64(cells[1].c_str(), &revision) || revision != payload.expected_bank_revision)
		return ESTALE;
	for (size_t i = 2; i < 6; ++i)
	{
		uint64_t amount = 0;
		if (!parse_u64(cells[i].c_str(), &amount) || amount > INT64_MAX)
			return EILSEQ;
	}
	economic_account_key wallet{}, bank{}, claim_key{};
	if (!auction_capture_mapping(connection, lineage, economic_account_kind::wallet, 1,
				     payload.actor_pid, 0, &wallet) ||
	    !auction_capture_mapping(connection, lineage, economic_account_kind::bank, 2, bank_id,
				     payload.racewar, &bank))
		return errno ? errno : EILSEQ;
	std::vector<economic_sql_mapping_request> requests{ { wallet, 1, payload.actor_pid },
							    { bank, 2, bank_id } };
	auction_item_claim_state claim;
	auction_money_claim_state money;
	if (payload.action == auction_action::claim_money)
	{
		if (!auction_capture_mapping(connection, lineage,
					     economic_account_kind::pending_claim, 5,
					     payload.actor_pid, 0, &claim_key))
			return errno ? errno : EILSEQ;
		requests.push_back({ claim_key, 5, payload.actor_pid });
		if (!auction_capture_row(
			    connection,
			    "SELECT money,claim_revision FROM auction_money_pickups WHERE pid=" +
				    std::to_string(payload.actor_pid) + " FOR UPDATE",
			    2, &cells))
			return errno ? errno : EILSEQ;
		uint64_t amount = 0;
		if (!parse_u64(cells[0].c_str(), &amount) || !amount || amount > INT_MAX ||
		    !parse_u64(cells[1].c_str(), &money.revision))
			return EILSEQ;
		money.beneficiary_pid = payload.actor_pid;
		money.money = static_cast<int64_t>(amount);
	}
	else if (payload.action == auction_action::claim_item)
	{
		if (!auction_capture_row(
			    connection,
			    "SELECT seller_pid,winning_bidder_pid,status+0,custody_state,auction_revision,HEX(listing_operation_id) FROM auctions WHERE id=" +
				    std::to_string(payload.auction_id) + " FOR UPDATE",
			    6, &cells))
			return errno ? errno : EILSEQ;
		uint64_t fields[5]{};
		for (size_t i = 0; i < 5; ++i)
			if (!parse_u64(cells[i].c_str(), &fields[i]) ||
			    (i < 4 && fields[i] > UINT32_MAX))
				return EILSEQ;
		claim.auction_id = payload.auction_id;
		claim.seller_pid = static_cast<uint32_t>(fields[0]);
		claim.winner_pid = static_cast<uint32_t>(fields[1]);
		claim.status = static_cast<uint32_t>(fields[2]);
		claim.custody_state = static_cast<uint32_t>(fields[3]);
		claim.auction_revision = fields[4];
		claim.claimant_pid = payload.actor_pid;
		if (cells[5].size() != 32 ||
		    !critical_operation_id_from_hex(cells[5].c_str(), &claim.listing_operation))
			return EILSEQ;
		if (!auction_capture_row(
			    connection,
			    "SELECT HEX(operation_id),event_type,auction_revision FROM auction_ledger WHERE auction_id=" +
				    std::to_string(payload.auction_id) +
				    " AND event_type IN (3,4,7) ORDER BY auction_revision DESC LIMIT 1 FOR UPDATE",
			    3, &cells))
			return errno ? errno : EILSEQ;
		uint64_t event = 0, terminal_revision = 0;
		if (cells[0].size() != 32 ||
		    !critical_operation_id_from_hex(cells[0].c_str(),
						    &claim.claim_source_operation) ||
		    !parse_u64(cells[1].c_str(), &event) ||
		    !parse_u64(cells[2].c_str(), &terminal_revision) ||
		    terminal_revision > claim.auction_revision ||
		    (event == 3 && (claim.status != 2 || !claim.winner_pid ||
				    payload.actor_pid != claim.winner_pid)) ||
		    (event == 4 && (claim.status != 2 || claim.winner_pid ||
				    payload.actor_pid != claim.seller_pid)) ||
		    (event == 7 && (claim.status != 3 || payload.actor_pid != claim.seller_pid)) ||
		    (event != 3 && event != 4 && event != 7))
			return ESTALE;
		claim.item_count = payload.item_count;
		for (size_t i = 0; i < payload.item_count; ++i)
		{
			const auto &item = payload.items[i];
			if (!auction_capture_row(
				    connection,
				    "SELECT slot,item_revision,vnum,COALESCE(claim_pid,0),claimed_at IS NOT NULL FROM auction_item_custody WHERE auction_id=" +
					    std::to_string(payload.auction_id) + " AND item_uid=" +
					    std::to_string(item.item_uid) + " FOR UPDATE",
				    5, &cells))
				return errno ? errno : EILSEQ;
			uint64_t row[5]{};
			for (size_t j = 0; j < 5; ++j)
				if (!parse_u64(cells[j].c_str(), &row[j]))
					return EILSEQ;
			if (row[0] > UINT16_MAX || row[1] != item.expected_item_revision ||
			    row[2] != static_cast<uint64_t>(item.vnum) ||
			    row[3] != payload.actor_pid || row[4])
				return ESTALE;
			claim.rows[i] = {
				item.item_uid,	   row[1], static_cast<uint16_t>(row[0]), item.vnum,
				payload.actor_pid, false
			};
		}
		if (command->payload_version != AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION &&
		    !auction_capture_roots(connection, payload, item_owner_type::auction,
					   payload.auction_id))
			return errno ? errno : EILSEQ;
	}
	else if (command->payload_version != AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION &&
		 !auction_capture_roots(connection, payload, item_owner_type::player,
					payload.actor_pid))
		return errno ? errno : EILSEQ;
	economic_sql_authority_snapshot locked;
	auto error = economic_sql_lock_authority(connection, lineage, epoch, requests, &locked);
	if (error)
		return error;
	if (payload.action == auction_action::claim_money)
	{
		std::vector<economic_sql_pending_claim_remaining> sources;
		error = economic_sql_pending_claim_source_remaining(connection, claim_key,
								    payload.actor_pid, &sources);
		if (error)
			return error;
		for (const auto &source : sources)
			money.sources.push_back({ source.operation, source.slot, payload.actor_pid,
						  claim_key.authority_id,
						  source.remaining_amount });
	}
	std::vector<uint8_t> encoded;
	economic_accounting_error status;
	if (payload.action == auction_action::list)
		status = auction_listing_accounting_intent(*command, epoch, wallet, bank, &encoded);
	else if (payload.action == auction_action::claim_item)
		status = auction_item_claim_accounting_intent(*command, epoch, wallet, bank, claim,
							      &encoded);
	else
		status = auction_money_claim_accounting_intent(*command, epoch, wallet, bank,
							       claim_key, money, &encoded);
	if (status != economic_accounting_error::ok)
		return status == economic_accounting_error::capacity ? ENOMEM : EILSEQ;
	critical_command frozen = *command;
	frozen.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	frozen.accounting_intent = std::move(encoded);
	if (!auction_repository_frozen_accounting_valid(frozen))
		return EILSEQ;
	if (mysql_thread_id(connection) != session ||
	    !(connection->server_status & SERVER_STATUS_IN_TRANS))
		return ENOTCONN;
	*command = std::move(frozen);
	return 0;
}

unsigned int auction_capture_failure()
{
	return errno ? static_cast<unsigned int>(errno) : EILSEQ;
}
} // namespace
#endif

bool auction_repository_frozen_accounting_valid(const critical_command &command) noexcept
{
	try
	{
		economic_frozen_intent intent;
		auction_command_payload payload{};
		auction_bid_accounting_listing bid;
		auction_bid_accounting_accounts bid_accounts;
		auction_settlement_listing settlement;
		auction_settlement_accounts settlement_accounts;
		auction_item_claim_state claim;
		economic_account_key wallet, bank, claim_account;
		return auction_bid_accounting_decode(command, &intent, &payload, &bid,
						     &bid_accounts) ==
			       economic_accounting_error::ok ||
		       auction_settlement_accounting_decode(command, &intent, &payload, &settlement,
							    &settlement_accounts) ==
			       economic_accounting_error::ok ||
		       auction_listing_accounting_decode(command, &intent, &payload, &wallet,
							 &bank) == economic_accounting_error::ok ||
		       auction_item_claim_accounting_decode(command, &intent, &payload, &claim,
							    &wallet, &bank) ==
			       economic_accounting_error::ok ||
		       auction_money_claim_accounting_decode(command, &intent, &payload, &wallet,
							     &bank, &claim_account) ==
			       economic_accounting_error::ok;
	}
	catch (...)
	{
		return false;
	}
}

unsigned int
auction_repository_validate_accounted_native_cut(MYSQL *connection,
						 const critical_command &command) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	return ENOTSUP;
#else
	try
	{
		economic_sql_auction_source_claim_detail::session guard{ connection };
		auto error = guard.check(true);
		if (error)
			return error;
		auto validate = [&]() -> unsigned int
		{
			if (command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
			    command.payload_version != AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION ||
			    !auction_repository_frozen_accounting_valid(command))
				return EPROTONOSUPPORT;
			auction_native_command_context native;
			const auto decoded = auction_native_command_decode(command, &native);
			if (decoded != economic_accounting_error::ok)
				return decoded == economic_accounting_error::capacity ? ENOMEM :
											EILSEQ;
			const auto action = native.payload.action;
			if (action != auction_action::bid && action != auction_action::finalize &&
			    action != auction_action::remove &&
			    action != auction_action::claim_money)
				return EPROTONOSUPPORT;
			economic_sql_auction_native_listing_source source;
			if (action != auction_action::claim_money)
			{
				const auto source_error =
					economic_sql_auction_read_native_listing_source(
						connection, command, &source);
				if (source_error)
					return source_error;
			}
			critical_command observed = command;
			errno = 0;
			if (!auction_capture_native_cut(connection, native, &observed, source.items,
							source.literals))
				return auction_capture_failure();
			if (source.auction_id)
			{
				std::vector<std::string> cells;
				if (!auction_capture_row(
					    connection,
					    "SELECT seller_pid,listing_operation_id FROM auctions WHERE id=" +
						    std::to_string(source.auction_id) +
						    " FOR UPDATE",
					    2, &cells))
					return auction_capture_failure();
				economic_frozen_intent intent;
				auction_bid_accounting_listing bid;
				auction_bid_accounting_accounts bid_accounts;
				auction_settlement_listing listing;
				auction_settlement_accounts settlement_accounts;
				auction_command_payload decoded_payload{};
				if (action == auction_action::bid)
				{
					if (auction_bid_accounting_decode(
						    command, &intent, &decoded_payload, &bid,
						    &bid_accounts) != economic_accounting_error::ok)
						return EILSEQ;
					listing.seller_pid = bid.seller_pid;
					listing.listing_operation = bid.listing_operation;
				}
				else if (auction_settlement_accounting_decode(
						 command, &intent, &decoded_payload, &listing,
						 &settlement_accounts) !=
					 economic_accounting_error::ok)
					return EILSEQ;
				uint64_t seller = 0;
				if (!parse_u64(cells[0].c_str(), &seller) ||
				    seller != listing.seller_pid || cells[1].size() != 16 ||
				    !std::equal(listing.listing_operation.bytes.begin(),
						listing.listing_operation.bytes.end(),
						reinterpret_cast<const uint8_t *>(cells[1].data())))
					return EILSEQ;
				std::string roots;
				for (const auto &item : source.items)
					if (!item.position.parent_uid)
					{
						if (!roots.empty())
							roots += ',';
						roots += std::to_string(item.uid);
					}
				std::string source_uids;
				for (const auto &item : source.items)
				{
					if (!source_uids.empty())
						source_uids += ',';
					source_uids += std::to_string(item.uid);
				}
				std::vector<std::vector<std::string>> current;
				if (!auction_capture_rows(
					    connection,
					    "SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),owner_type,owner_id,owner_context_id,item_revision,vnum,state,coin_payload IS NULL FROM item_current_owner WHERE root_item_uid IN(" +
						    roots + ") OR item_uid IN(" + source_uids +
						    ") OR parent_item_uid IN(" + source_uids +
						    ") OR (owner_type=" +
						    std::to_string(static_cast<unsigned>(
							    item_owner_type::auction)) +
						    " AND owner_id=" +
						    std::to_string(source.auction_id) +
						    " AND owner_context_id=0) ORDER BY item_uid FOR UPDATE",
					    10, &current) ||
				    current.size() != source.items.size())
					return auction_capture_failure();
				for (const auto &values : current)
				{
					uint64_t n[10]{};
					for (size_t i = 0; i < 10; ++i)
						if (!parse_u64(values[i].c_str(), &n[i]))
							return EILSEQ;
					const auto found = std::find_if(source.items.begin(),
									source.items.end(),
									[&](const auto &w)
									{ return w.uid == n[0]; });
					if (found == source.items.end() ||
					    n[1] != found->position.root_uid ||
					    n[2] != found->position.parent_uid ||
					    n[3] != static_cast<unsigned>(
							    item_owner_type::auction) ||
					    n[4] != source.auction_id || n[5] ||
					    n[6] != found->position.revision ||
					    n[8] != static_cast<unsigned>(
							    item_custody_state::active) ||
					    !n[9])
						return EILSEQ;
					if (!source.literals.empty())
					{
						const auto literal = std::find_if(
							source.literals.begin(),
							source.literals.end(), [&](const auto &v)
							{ return v.object_uid == n[0]; });
						if (literal == source.literals.end() ||
						    n[7] != static_cast<uint64_t>(literal->vnum))
							return EILSEQ;
					}
					const critical_entity_key key{ critical_entity_type::item,
								       n[0] };
					if (std::count_if(command.expected_revisions.begin(),
							  command.expected_revisions.end(),
							  [&](const auto &f) {
								  return critical_entity_key_equal(
										 f.key, key) &&
									 f.revision == n[6];
							  }) != 1)
						return ESTALE;
				}
				if (static_cast<size_t>(std::count_if(
					    command.keys.begin(), command.keys.end(),
					    [](const auto &k) {
						    return k.type == critical_entity_type::item;
					    })) != source.items.size())
					return EILSEQ;
			}
			// Fresh preparation may append fences. An accepted command is immutable:
			// missing, extra or stale keys never acquire authority during execution.
			if (observed.keys.size() != command.keys.size() ||
			    observed.expected_revisions.size() != command.expected_revisions.size())
				return ESTALE;
			for (size_t i = 0; i < command.keys.size(); ++i)
				if (!critical_entity_key_equal(observed.keys[i], command.keys[i]))
					return ESTALE;
			for (size_t i = 0; i < command.expected_revisions.size(); ++i)
				if (!critical_entity_key_equal(observed.expected_revisions[i].key,
							       command.expected_revisions[i].key) ||
				    observed.expected_revisions[i].revision !=
					    command.expected_revisions[i].revision)
					return ESTALE;
			return 0;
		};
		error = validate();
		const auto session_error = guard.check();
		return session_error ? session_error : error;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EILSEQ;
	}
#endif
}

unsigned int auction_repository_prepare_accounting(MYSQL *connection,
						   const critical_operation_id &lineage,
						   const critical_operation_id &epoch,
						   critical_command *command)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)lineage;
	(void)epoch;
	(void)command;
	return ENOTSUP;
#else
	if (!connection || !command || !(connection->server_status & SERVER_STATUS_IN_TRANS) ||
	    critical_operation_id_is_zero(lineage) || critical_operation_id_is_zero(epoch))
		return EINVAL;
	// Frozen replay belongs to validation/readback, never to this current capture.
	if (command->schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    !command->accounting_intent.empty() || !critical_command_envelope_valid(*command))
		return EPROTONOSUPPORT;
	using client_flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	client_flag reconnect = false;
	if (mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect))
		return mysql_errno(connection) ? mysql_errno(connection) :
						 static_cast<unsigned int>(EIO);
	if (reconnect)
		return EPERM;
	const auto session = mysql_thread_id(connection);
	try
	{
		critical_command candidate = *command;
		auction_command_payload payload{};
		if (candidate.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION)
		{
			auction_native_command_context native;
			const auto decoded = auction_native_command_decode(candidate, &native);
			if (decoded != economic_accounting_error::ok)
				return decoded == economic_accounting_error::capacity ? ENOMEM :
											EILSEQ;
			payload = native.payload;
			economic_sql_auction_native_listing_source source;
			if (payload.action == auction_action::bid ||
			    payload.action == auction_action::finalize ||
			    payload.action == auction_action::remove)
			{
				const auto source_error =
					economic_sql_auction_capture_native_listing_source(
						connection, candidate, lineage, epoch, &source);
				if (source_error)
					return source_error;
			}
			if (!auction_capture_native_cut(connection, native, &candidate,
							source.items, source.literals))
				return auction_capture_failure();
		}
		else if (!auction_command_decode_payload(candidate, &payload))
			return EILSEQ;
		auto *original_output = command;
		command = &candidate;
		if (payload.action == auction_action::list ||
		    payload.action == auction_action::claim_item ||
		    payload.action == auction_action::claim_money)
		{
			const auto capture_error =
				auction_capture_three(connection, lineage, epoch, payload, command);
			if (capture_error)
				return capture_error;
			*original_output = std::move(candidate);
			return 0;
		}
		if ((payload.action != auction_action::bid &&
		     payload.action != auction_action::finalize &&
		     payload.action != auction_action::remove))
			return EPROTONOSUPPORT;
		auction_settlement_listing listing;
		if (!auction_capture_listing(connection, payload.auction_id, &listing))
			return auction_capture_failure();
		std::vector<economic_sql_mapping_request> requests;
		auto mapping = [&](economic_account_kind kind, uint16_t locator, uint64_t native,
				   uint64_t context, economic_account_key *key)
		{
			if (!auction_capture_mapping(connection, lineage, kind, locator, native,
						     context, key))
				return false;
			requests.push_back({ *key, locator, native });
			return true;
		};
		auto claim = [&](uint32_t pid, economic_account_key *key, uint32_t *absent)
		{
			if (!auction_capture_claim(connection, lineage, pid, key, absent))
				return false;
			if (!*absent)
				requests.push_back({ *key, 5, pid });
			return true;
		};
		economic_account_key escrow{}, wallet{}, bank{};
		if (!mapping(economic_account_kind::auction_escrow, 4, payload.auction_id, 0,
			     &escrow))
			return auction_capture_failure();
		if (payload.actor_pid)
		{
			std::vector<std::string> row;
			if (!auction_capture_row(
				    connection,
				    "SELECT account_name,racewar FROM player_data WHERE pid=" +
					    std::to_string(payload.actor_pid) + " FOR UPDATE",
				    2, &row))
				return auction_capture_failure();
			uint64_t race = 0;
			if (row[0].size() != strnlen(payload.account_name.data(),
						     payload.account_name.size()) ||
			    strcasecmp(row[0].c_str(), payload.account_name.data()) ||
			    !parse_u64(row[1].c_str(), &race) || race != payload.racewar)
				return ESTALE;
			const auto account = "'" +
					     escape(connection, payload.account_name.data(),
						    strnlen(payload.account_name.data(),
							    payload.account_name.size())) +
					     "'";
			if (!auction_capture_row(
				    connection,
				    "SELECT id FROM account_banks WHERE account_name=" + account +
					    " AND racewar=" + std::to_string(payload.racewar) +
					    " FOR UPDATE",
				    1, &row))
				return auction_capture_failure();
			uint64_t native_bank = 0;
			if (!parse_u64(row[0].c_str(), &native_bank) || !native_bank ||
			    native_bank > UINT32_MAX)
				return EILSEQ;
			if (!mapping(economic_account_kind::wallet, 1, payload.actor_pid, 0,
				     &wallet) ||
			    !mapping(economic_account_kind::bank, 2, native_bank, payload.racewar,
				     &bank))
				return auction_capture_failure();
		}
		std::vector<uint8_t> encoded;
		economic_accounting_error error;
		if (payload.action == auction_action::bid)
		{
			auction_bid_accounting_listing bid{
				listing.auction_id,	   listing.seller_pid,
				listing.winner_pid,	   listing.status,
				listing.custody_state,	   listing.current_price,
				listing.buy_price,	   listing.revision,
				listing.listing_operation, listing.winning_bid_operation
			};
			auction_bid_accounting_accounts accounts;
			accounts.wallet = wallet;
			accounts.bank = bank;
			accounts.escrow = escrow;
			const bool outbid = listing.winner_pid &&
					    listing.winner_pid != payload.actor_pid;
			const bool sold = listing.buy_price > 0 &&
					  payload.value >= listing.buy_price;
			if (!claim(payload.actor_pid, &accounts.bidder_claim,
				   &accounts.absent_bidder_pid) ||
			    (outbid && !claim(listing.winner_pid, &accounts.previous_claim,
					      &accounts.absent_previous_pid)) ||
			    (sold && !claim(listing.seller_pid, &accounts.seller_claim,
					    &accounts.absent_seller_pid)))
				return auction_capture_failure();
			error = auction_bid_accounting_intent(*command, epoch, bid, accounts,
							      &encoded);
		}
		else
		{
			if (!auction_capture_items(connection, &listing))
				return auction_capture_failure();
			auction_settlement_accounts accounts;
			accounts.escrow = escrow;
			accounts.actor_wallet = wallet;
			accounts.actor_bank = bank;
			if (payload.action == auction_action::finalize && listing.winner_pid &&
			    !claim(listing.seller_pid, &accounts.seller_claim,
				   &accounts.absent_seller_pid))
				return auction_capture_failure();
			error = auction_settlement_accounting_intent(*command, epoch, listing,
								     accounts, &encoded);
		}
		if (error != economic_accounting_error::ok)
			return EILSEQ;
		economic_sql_authority_snapshot locked;
		const auto lock_error =
			economic_sql_lock_authority(connection, lineage, epoch, requests, &locked);
		if (lock_error)
			return lock_error;
		if (!(connection->server_status & SERVER_STATUS_IN_TRANS) ||
		    mysql_thread_id(connection) != session)
			return ENOTCONN;
		critical_command frozen = *command;
		frozen.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
		frozen.accounting_intent = std::move(encoded);
		if (!auction_repository_frozen_accounting_valid(frozen))
			return EILSEQ;
		*original_output = std::move(frozen);
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}

unsigned int auction_repository_readback_endpoints(MYSQL *connection,
						   const critical_command &command,
						   auction_accounting_endpoint_readback *output)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)output;
	return ENOTSUP;
#else
	if (!connection || !output || !(connection->server_status & SERVER_STATUS_IN_TRANS))
		return EINVAL;
	using client_flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	client_flag reconnect = false;
	if (mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) || reconnect)
		return EPERM;
	const auto session = mysql_thread_id(connection);
	try
	{
		economic_frozen_intent intent;
		auction_command_payload payload{};
		auction_bid_accounting_listing bid;
		auction_bid_accounting_accounts accounts;
		auction_settlement_listing settlement;
		auction_settlement_accounts settlement_accounts;
		auction_accounting_endpoint_readback candidate;
		uint32_t bidder = 0, previous = 0, seller = 0;
		if (auction_bid_accounting_decode(command, &intent, &payload, &bid, &accounts) ==
		    economic_accounting_error::ok)
		{
			candidate.bidder_claim = accounts.bidder_claim;
			candidate.previous_claim = accounts.previous_claim;
			candidate.seller_claim = accounts.seller_claim;
			bidder = accounts.absent_bidder_pid;
			previous = accounts.absent_previous_pid;
			seller = accounts.absent_seller_pid;
		}
		else if (auction_settlement_accounting_decode(command, &intent, &payload,
							      &settlement, &settlement_accounts) ==
			 economic_accounting_error::ok)
		{
			candidate.seller_claim = settlement_accounts.seller_claim;
			seller = settlement_accounts.absent_seller_pid;
		}
		else
			return EPROTONOSUPPORT;
		// This endpoint evidence helper is deliberately not a mapped-only receipt verifier.
		if (!bidder && !previous && !seller)
			return ENODATA;
		if (bidder)
		{
			const auto error = economic_sql_pending_claim_endpoint_readback(
				connection, command, bidder, &candidate.bidder_claim);
			if (error != ENODATA)
				return error ? error : EILSEQ;
			candidate.unused_bidder = true;
		}
		for (auto pair : { std::pair{ previous, &candidate.previous_claim },
				   std::pair{ seller, &candidate.seller_claim } })
			if (pair.first)
			{
				const auto error = economic_sql_pending_claim_endpoint_readback(
					connection, command, pair.first, pair.second);
				if (error)
					return error;
			}
		if (!(connection->server_status & SERVER_STATUS_IN_TRANS) ||
		    mysql_thread_id(connection) != session)
			return ENOTCONN;
		*output = std::move(candidate);
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}

namespace
{
struct auction_census_failure
{
	unsigned int code;
};
void auction_census_require(bool value, unsigned int code = EILSEQ)
{
	if (!value)
		throw auction_census_failure{ code };
}
template <class T> std::optional<T> auction_census_number(const std::optional<std::string> &cell)
{
	if (!cell)
		return {};
	T value{};
	const auto parsed = std::from_chars(cell->data(), cell->data() + cell->size(), value);
	if (parsed.ec != std::errc{} || parsed.ptr != cell->data() + cell->size())
		return {};
	std::array<char, 32> canonical{};
	const auto encoded =
		std::to_chars(canonical.data(), canonical.data() + canonical.size(), value);
	if (encoded.ec != std::errc{} ||
	    std::string_view(canonical.data(), encoded.ptr - canonical.data()) != *cell)
		return {};
	return value;
}
std::optional<std::string_view> auction_census_operation(const std::optional<std::string> &cell)
{
	if (!cell || cell->size() != 16 ||
	    std::all_of(cell->begin(), cell->end(), [](char c) { return c == 0; }))
		return {};
	return std::string_view(*cell);
}
using auction_census_cells = std::vector<std::optional<std::string>>;
using auction_census_issue = auction_physical_issue;
struct auction_census_index
{
	size_t row = 0;
	bool ambiguous = false;
};
struct auction_census_worker
{
	auction_census_worker(const economic_sql_physical_source_snapshot &b,
			      const sql_room_item_source_snapshot &s, size_t n)
		: base(b)
		, supplement(s)
		, limit(n)
	{
	}
	const economic_sql_physical_source_snapshot &base;
	const sql_room_item_source_snapshot &supplement;
	size_t limit;
	auction_physical_correspondence report;
	std::map<uint32_t, auction_census_index> auctions;
	std::map<std::pair<uint32_t, uint16_t>, auction_census_index> slots;
	std::map<std::string_view, auction_census_index> inbox, operations;
	std::map<std::string_view, std::vector<size_t>> outbox;
	std::map<std::pair<std::string_view, uint64_t>, auction_census_index> ledgers, references;
	std::map<uint64_t, auction_census_index> custody, equipment;
	std::map<uint64_t, std::vector<size_t>> roots;
	std::map<std::tuple<uint64_t, uint64_t, uint64_t>, auction_census_index> owners;
	std::vector<uint8_t> matched, extras;
	std::map<std::string_view, size_t> retained_receipts;
	std::map<uint32_t, auction_census_index> retained_by_auction;
	const economic_sql_source_table &table(size_t index, bool extra = false) const
	{
		return extra ? supplement.tables[index] : base.source2.tables[index];
	}
	auction_physical_reference ref(size_t index, size_t row, bool extra = false) const
	{
		return { extra, index, row, table(index, extra).rows[row].digest };
	}
	void finding(auction_census_issue issue, const auction_physical_reference &source)
	{
		++report.issue_counts[static_cast<size_t>(issue)];
		++report.diagnostic_count;
		if (report.diagnostics.size() < limit)
			report.diagnostics.push_back({ issue, source });
	}
	void mark(auction_physical_slot &slot, auction_census_issue issue)
	{
		const auto bit = uint32_t{ 1 } << static_cast<size_t>(issue);
		if (!(slot.issue_mask & bit))
		{
			slot.issue_mask |= bit;
			finding(issue, slot.source);
		}
	}
	void mark(auction_physical_listing &listing, auction_census_issue issue)
	{
		const auto bit = uint32_t{ 1 } << static_cast<size_t>(issue);
		if (!(listing.issue_mask & bit))
		{
			listing.issue_mask |= bit;
			finding(issue, listing.source);
		}
	}
	template <class K>
	void index(std::map<K, auction_census_index> &map, const K &key, size_t row)
	{
		auto [at, added] = map.emplace(key, auction_census_index{ row, false });
		if (!added)
			at->second.ambiguous = true;
	}
	std::optional<size_t> lookup(const auto &map, const auto &key) const
	{
		auto found = map.find(key);
		if (found == map.end() || found->second.ambiguous)
			return {};
		return found->second.row;
	}
	template <class T>
	static bool equal(const auction_census_cells &cells, size_t field, T value)
	{
		return auction_census_number<T>(cells[field]) == std::optional<T>(value);
	}
	bool succeeded(size_t row, uint16_t version = 2) const
	{
		const auto &c = table(17).rows[row].cells;
		return equal<uint16_t>(c, 3, 7) && equal<uint16_t>(c, 4, version) &&
		       equal<uint16_t>(c, 5, version) && equal<uint8_t>(c, 6, 1) &&
		       equal<uint32_t>(c, 7, 0) && equal<uint16_t>(c, 8, 0) &&
		       equal<uint8_t>(c, 11, 1) && c[1] && c[1]->size() == 32 && c[2] &&
		       c[2]->size() == 32;
	}
	template <class T>
	bool scalar(const auction_census_cells &c, size_t field, bool nullable = false) const
	{
		return (!c[field] && nullable) || auction_census_number<T>(c[field]).has_value();
	}
	void malformed(const auction_physical_reference &source)
	{
		report.malformed_rows.push_back(source);
		finding(auction_census_issue::invalid_identity, source);
	}
	bool source_site(const std::optional<std::string> &cell) const
	{
		auto site = auction_census_number<uint16_t>(cell);
		return site && *site <= uint16_t(critical_source_site::operator_repair);
	}
	std::optional<auction_command_result> receipt(size_t row, uint16_t version = 2) const
	{
		const auto &i = table(17).rows[row].cells;
		auction_command_result result{};
		std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> canonical{};
		if (!succeeded(row, version) || !i[10] ||
		    !auction_command_decode_result(reinterpret_cast<const uint8_t *>(i[10]->data()),
						   i[10]->size(), &result) ||
		    !auction_command_encode_result(result, &canonical) ||
		    i[10]->size() != canonical.size() ||
		    !std::equal(canonical.begin(), canonical.end(),
				reinterpret_cast<const uint8_t *>(i[10]->data())))
			return {};
		if (result.action != auction_action::list ||
		    result.event_type != auction_event_type::listed || !result.auction_id ||
		    result.status != 1 || !result.seller_pid || result.winner_pid ||
		    result.previous_bidder_pid || result.final_price || result.claim_credit_used ||
		    result.wallet_value_delta > 0 || result.auction_revision != 1 ||
		    result.auction_owner_revision != 1 || !result.player_owner_revision ||
		    !result.item_count || result.item_count > 9 ||
		    !equal<uint64_t>(i, 9,
				     std::max({ result.wallet_revision, result.bank_revision,
						result.auction_revision,
						result.player_owner_revision,
						result.auction_owner_revision })))
			return {};
		std::set<uint64_t> unique;
		for (size_t n = 0; n < result.item_count; ++n)
			if (!result.item_uids[n] || result.item_uids[n] == UINT64_MAX ||
			    !result.item_revisions[n] || result.item_revisions[n] == UINT64_MAX ||
			    !unique.insert(result.item_uids[n]).second)
				return {};
		return result;
	}
	void read_retained_receipts()
	{
		for (size_t r = 0; r < table(17).rows.size(); ++r)
		{
			auto op = auction_census_operation(table(17).rows[r].cells[0]);
			auto decoded = receipt(r);
			if (!op || !decoded)
				continue;
			auction_physical_retained_listing witness;
			witness.inbox = ref(17, r);
			witness.receipt = *decoded;
			auto economic = lookup(operations, *op);
			if (economic)
				witness.operation = ref(4, *economic, true);
			auto outputs = outbox.find(*op);
			size_t exact = 0;
			if (outputs != outbox.end())
				for (size_t o : outputs->second)
				{
					const auto &out = table(18).rows[o].cells;
					if (equal<uint64_t>(out, 2, 0) &&
					    equal<uint16_t>(out, 3, 5) &&
					    equal<uint16_t>(out, 4, 1) &&
					    equal<uint16_t>(out, 5, 1) &&
					    out[6] == table(17).rows[r].cells[10])
					{
						++exact;
						witness.outbox = ref(18, o);
					}
				}
			if (exact != 1)
				witness.outbox.reset();
			if (lookup(inbox, *op) == std::optional<size_t>(r))
				retained_receipts.emplace(*op, report.retained_listings.size());
			index(retained_by_auction, decoded->auction_id,
			      report.retained_listings.size());
			report.retained_listings.push_back(std::move(witness));
		}
	}
	void build_indexes()
	{
		for (size_t r = 0; r < table(17).rows.size(); ++r)
			if (auto op = auction_census_operation(table(17).rows[r].cells[0]))
				index(inbox, *op, r);
		for (size_t r = 0; r < table(18).rows.size(); ++r)
			if (auto op = auction_census_operation(table(18).rows[r].cells[1]))
				outbox[*op].push_back(r);
		for (size_t r = 0; r < table(4, true).rows.size(); ++r)
			if (auto op = auction_census_operation(table(4, true).rows[r].cells[0]))
				index(operations, *op, r);
		for (size_t r = 0; r < table(2, true).rows.size(); ++r)
		{
			const auto &c = table(2, true).rows[r].cells;
			auto op = auction_census_operation(c[0]);
			auto event = auction_census_number<uint64_t>(c[1]);
			if (op && event)
				index(ledgers, std::pair{ *op, *event }, r);
			bool valid = op.has_value();
			for (size_t f = 1; f < c.size(); ++f)
				valid = valid && scalar<uint64_t>(c, f, f == 4);
			if (!valid)
				malformed(ref(2, r, true));
		}
		for (size_t r = 0; r < table(3, true).rows.size(); ++r)
		{
			const auto &c = table(3, true).rows[r].cells;
			auto op = auction_census_operation(c[0]);
			auto event = auction_census_number<uint64_t>(c[2]);
			if (op && event)
				index(references, std::pair{ *op, *event }, r);
			bool valid = op.has_value() &&
				     (!c[7] || auction_census_operation(c[7]).has_value());
			for (size_t f = 1; f < c.size(); ++f)
				if (f != 7)
					valid = valid && scalar<uint64_t>(c, f, f == 8);
			if (!valid)
				malformed(ref(3, r, true));
		}
		for (size_t r = 0; r < base.source2.item_equipment_sources[0].rows.size(); ++r)
		{
			const auto &c = base.source2.item_equipment_sources[0].rows[r].cells;
			if (auto uid = auction_census_number<uint64_t>(c[0]))
				index(equipment, *uid, r);
			if (!scalar<uint64_t>(c, 0) || !scalar<uint16_t>(c, 1))
				malformed({ false, 0, r,
					    base.source2.item_equipment_sources[0].rows[r].digest,
					    true });
		}
		for (size_t r = 0; r < table(11).rows.size(); ++r)
		{
			const auto &c = table(11).rows[r].cells;
			auto uid = auction_census_number<uint64_t>(c[0]);
			auto root = auction_census_number<uint64_t>(c[1]);
			if (uid)
				index(custody, *uid, r);
			if (root)
				roots[*root].push_back(r);
			bool valid = scalar<int32_t>(c, 7);
			for (size_t f = 0; f < 9; ++f)
				if (f != 7)
					valid = valid && scalar<uint64_t>(c, f, f == 2);
			if (!valid)
				malformed(ref(11, r));
			if (equal<uint8_t>(c, 3, static_cast<uint8_t>(item_owner_type::auction)))
				report.auction_custody.push_back(ref(11, r));
		}
		for (size_t r = 0; r < table(11).rows.size(); ++r)
		{
			auto uid = auction_census_number<uint64_t>(table(11).rows[r].cells[0]);
			if (uid && custody[*uid].ambiguous)
				finding(auction_census_issue::duplicate_current_uid, ref(11, r));
		}
		matched.resize(table(11).rows.size());
		extras.resize(matched.size());
		for (size_t r = 0; r < table(12).rows.size(); ++r)
		{
			const auto &c = table(12).rows[r].cells;
			auto t = auction_census_number<uint64_t>(c[0]),
			     id = auction_census_number<uint64_t>(c[1]),
			     context = auction_census_number<uint64_t>(c[2]);
			if (t && id && context)
				index(owners, std::tuple{ *t, *id, *context }, r);
			if (!t || !id || !context || !scalar<uint64_t>(c, 3))
				malformed(ref(12, r));
		}
	}
	void read_listings()
	{
		for (size_t r = 0; r < table(4).rows.size(); ++r)
		{
			const auto &c = table(4).rows[r].cells;
			auction_physical_listing row;
			row.source = ref(4, r);
			row.auction_id = auction_census_number<uint32_t>(c[0]);
			row.quantity = auction_census_number<int64_t>(c[6]);
			if (!row.auction_id || !*row.auction_id)
				mark(row, auction_census_issue::invalid_identity);
			else
				index(auctions, *row.auction_id, report.listings.size());
			auto op = auction_census_operation(c[9]);
			auto receipt_row = op ? lookup(inbox, *op) : std::optional<size_t>{};
			if (receipt_row)
			{
				row.inbox = ref(17, *receipt_row);
				const auto &i = table(17).rows[*receipt_row].cells;
				if (equal<uint16_t>(i, 5, 1))
					row.family = auction_physical_family::legacy_or_v1;
				auction_command_result receipt{};
				std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> canonical{};
				bool bound =
					succeeded(*receipt_row) && i[10] &&
					auction_command_decode_result(
						reinterpret_cast<const uint8_t *>(i[10]->data()),
						i[10]->size(), &receipt) &&
					auction_command_encode_result(receipt, &canonical) &&
					i[10]->size() == canonical.size() &&
					std::equal(
						canonical.begin(), canonical.end(),
						reinterpret_cast<const uint8_t *>(i[10]->data()));
				bound = bound && row.auction_id &&
					receipt.action == auction_action::list &&
					receipt.event_type == auction_event_type::listed &&
					receipt.auction_id == *row.auction_id &&
					receipt.status == 1 && receipt.seller_pid &&
					equal<uint32_t>(c, 1, receipt.seller_pid) &&
					!receipt.winner_pid && !receipt.previous_bidder_pid &&
					!receipt.final_price && !receipt.claim_credit_used &&
					receipt.wallet_value_delta <= 0 &&
					receipt.auction_revision == 1 &&
					receipt.auction_owner_revision == 1 &&
					receipt.player_owner_revision && receipt.item_count &&
					receipt.item_count <= 9 &&
					row.quantity == std::optional<int64_t>(receipt.item_count);
				bound = bound &&
					equal<uint64_t>(
						i, 9,
						std::max({ receipt.wallet_revision,
							   receipt.bank_revision,
							   receipt.auction_revision,
							   receipt.player_owner_revision,
							   receipt.auction_owner_revision }));
				std::set<uint64_t> unique;
				for (size_t n = 0; bound && n < receipt.item_count; ++n)
					bound = receipt.item_uids[n] &&
						receipt.item_uids[n] != UINT64_MAX &&
						receipt.item_revisions[n] &&
						receipt.item_revisions[n] != UINT64_MAX &&
						unique.insert(receipt.item_uids[n]).second;
				if (bound)
				{
					row.family = auction_physical_family::observed_native_v2;
					row.listing_receipt = receipt;
					mark(row,
					     auction_census_issue::original_provenance_unknown);
					auto economic = lookup(operations, *op);
					if (economic)
					{
						row.operation = ref(4, *economic, true);
					}
					size_t output_count = 0;
					auto outputs = outbox.find(*op);
					if (outputs != outbox.end())
						for (size_t o : outputs->second)
						{
							const auto &out = table(18).rows[o].cells;
							if (equal<uint64_t>(out, 2, 0) &&
							    equal<uint16_t>(out, 3, 5) &&
							    equal<uint16_t>(out, 4, 1) &&
							    equal<uint16_t>(out, 5, 1) &&
							    out[6] == i[10])
							{
								++output_count;
								row.outbox = ref(18, o);
							}
						}
					if (output_count != 1)
						row.outbox.reset();
				}
			}
			if (!row.listing_receipt)
				mark(row, auction_census_issue::unbound_listing);
			if (!row.quantity || *row.quantity < 1 || *row.quantity > 9)
				mark(row, auction_census_issue::invalid_slots);
			const bool status = c[2] && (*c[2] == "OPEN" || *c[2] == "CLOSED" ||
						     *c[2] == "REMOVED");
			auto revision = auction_census_number<uint64_t>(c[7]);
			if (!status || !revision || !*revision || !equal<uint8_t>(c, 8, 1))
				mark(row, auction_census_issue::invalid_identity);
			report.listings.push_back(std::move(row));
		}
		for (auto &row : report.listings)
			if (row.auction_id && auctions[*row.auction_id].ambiguous)
				mark(row, auction_census_issue::duplicate_row_key);
		for (auto &witness : report.retained_listings)
		{
			auto parent = lookup(auctions, witness.receipt.auction_id);
			if (parent)
			{
				const auto &listing = report.listings[*parent];
				if (listing.listing_receipt && listing.inbox &&
				    listing.inbox->row == witness.inbox.row)
					witness.parent = listing.source;
			}
			if (!witness.parent)
				finding(auction_census_issue::unbound_listing, witness.inbox);
		}
	}
	void read_slots()
	{
		for (size_t r = 0; r < table(7).rows.size(); ++r)
		{
			const auto &c = table(7).rows[r].cells;
			auction_physical_slot row;
			row.source = ref(7, r);
			row.auction_id = auction_census_number<uint32_t>(c[0]);
			row.slot = auction_census_number<uint16_t>(c[1]);
			row.item_uid = auction_census_number<uint64_t>(c[2]);
			row.row_revision = auction_census_number<uint64_t>(c[3]);
			row.vnum = auction_census_number<int32_t>(c[4]);
			row.claim_pid = auction_census_number<uint32_t>(c[6]);
			auto claimed = auction_census_number<uint8_t>(c[8]);
			if (claimed && *claimed <= 1)
				row.claimed = *claimed != 0;
			if (!row.auction_id || !*row.auction_id || !row.slot || !row.item_uid ||
			    !*row.item_uid || !row.row_revision || !*row.row_revision ||
			    !row.vnum || !row.claimed)
				mark(row, auction_census_issue::invalid_identity);
			if (row.auction_id && row.slot)
			{
				index(slots, std::pair{ *row.auction_id, *row.slot },
				      report.slots.size());
				row.listing_index = lookup(auctions, *row.auction_id);
			}
			if (!row.listing_index)
				mark(row, auction_census_issue::unbound_listing);
			const auto *listing =
				row.listing_index ? &report.listings[*row.listing_index] : nullptr;
			if (row.auction_id)
				row.retained_receipt_index =
					lookup(retained_by_auction, *row.auction_id);
			if (!row.retained_receipt_index)
				mark(row, auction_census_issue::opaque_literal);
			else
			{
				mark(row, auction_census_issue::original_provenance_unknown);
				const auto &receipt =
					report.retained_listings[*row.retained_receipt_index]
						.receipt;
				if (!row.slot || *row.slot >= receipt.item_count)
					mark(row, auction_census_issue::invalid_slots);
				std::vector<player_item_snapshot> items;
				std::vector<uint64_t> revisions;
				if (!c[5] ||
				    !auction_repository_decode_native_tree_blob(
					    { reinterpret_cast<const uint8_t *>(c[5]->data()),
					      c[5]->size() },
					    &items, &revisions))
					mark(row, auction_census_issue::literal_decode_refused);
				else
				{
					row.node_begin = report.nodes.size();
					row.node_count = items.size();
					bool bound =
						row.slot && *row.slot < receipt.item_count &&
						row.item_uid == std::optional<uint64_t>(
									items[0].object_uid) &&
						row.vnum == std::optional<int32_t>(items[0].vnum) &&
						items[0].object_uid ==
							receipt.item_uids[*row.slot] &&
						revisions[0] == receipt.item_revisions[*row.slot];
					if (listing && row.slot && *row.slot == 0)
						bound = bound &&
							equal<int32_t>(
								table(4).rows[listing->source.row]
									.cells,
								10, items[0].vnum);
					if (!bound)
						mark(row,
						     auction_census_issue::listing_literal_mismatch);
					if (row.claimed && !*row.claimed &&
					    row.row_revision !=
						    std::optional<uint64_t>(revisions[0]))
						mark(row,
						     auction_census_issue::listing_literal_mismatch);
					if (row.claimed && *row.claimed &&
					    (!row.row_revision ||
					     *row.row_revision <= revisions[0] || !row.claim_pid ||
					     !*row.claim_pid || !auction_census_operation(c[7])))
						mark(row, auction_census_issue::history_metadata);
					for (size_t n = 0; n < items.size(); ++n)
					{
						auction_physical_node node;
						node.literal = std::move(items[n]);
						node.listing_after_revision = revisions[n];
						node.slot_index = report.slots.size();
						report.nodes.push_back(std::move(node));
					}
				}
			}
			report.slots.push_back(std::move(row));
		}
		for (auto &row : report.slots)
			if (row.auction_id && row.slot &&
			    slots[std::pair{ *row.auction_id, *row.slot }].ambiguous)
				mark(row, auction_census_issue::duplicate_row_key);
		for (auto &listing : report.listings)
			if (listing.auction_id && listing.listing_receipt)
				for (uint16_t slot = 0; slot < listing.listing_receipt->item_count;
				     ++slot)
					if (!lookup(slots, std::pair{ *listing.auction_id, slot }))
						mark(listing, auction_census_issue::invalid_slots);
	}
	void legacy_identities()
	{
		std::map<uint64_t, size_t> live_counts;
		std::map<uint64_t, std::vector<size_t>> children;
		for (size_t row = 0; row < table(11).rows.size(); ++row)
			if (auto parent =
				    auction_census_number<uint64_t>(table(11).rows[row].cells[2]))
				children[*parent].push_back(row);
		std::vector<std::vector<size_t>> by_listing(report.listings.size());
		for (size_t index = 0; index < report.slots.size(); ++index)
		{
			const auto &slot = report.slots[index];
			if (slot.listing_index)
				by_listing[*slot.listing_index].push_back(index);
			if (!slot.claimed || *slot.claimed)
				continue;
			if (slot.node_count)
				for (size_t n = slot.node_begin;
				     n < slot.node_begin + slot.node_count; ++n)
					++live_counts[report.nodes[n].literal.object_uid];
			else if (slot.item_uid && *slot.item_uid)
				++live_counts[*slot.item_uid];
		}
		for (auto &slot : report.slots)
		{
			if (!slot.claimed || *slot.claimed)
				continue;
			bool duplicate = slot.item_uid && live_counts[*slot.item_uid] > 1;
			for (size_t n = slot.node_begin; n < slot.node_begin + slot.node_count; ++n)
				duplicate = duplicate ||
					    live_counts[report.nodes[n].literal.object_uid] > 1;
			if (duplicate)
				mark(slot, auction_census_issue::duplicate_current_uid);
		}
		for (size_t index = 0; index < report.listings.size(); ++index)
		{
			auto &listing = report.listings[index];
			const auto &parent = table(4).rows[listing.source.row].cells;
			if (listing.quantity && *listing.quantity > 0 && by_listing[index].empty())
				mark(listing, auction_census_issue::missing_identity);
			// Declared inventory is independent of receipt family and claim history.
			if (listing.auction_id && listing.quantity && *listing.quantity >= 1 &&
			    *listing.quantity <= 9)
			{
				bool cardinality = by_listing[index].size() ==
						   static_cast<size_t>(*listing.quantity);
				for (uint16_t ordinal = 0; ordinal < *listing.quantity; ++ordinal)
					cardinality = cardinality &&
						      lookup(slots, std::pair{ *listing.auction_id,
									       ordinal })
							      .has_value();
				if (!cardinality)
					mark(listing, auction_census_issue::invalid_slots);
			}
			if (listing.family != auction_physical_family::legacy_or_v1)
				continue;
			auto op = auction_census_operation(parent[9]);
			auto input = op ? lookup(inbox, *op) : std::optional<size_t>{};
			auto decoded = input ? receipt(*input, 1) :
					       std::optional<auction_command_result>{};
			const bool bound = decoded && listing.auction_id &&
					   decoded->auction_id == *listing.auction_id &&
					   equal<uint32_t>(parent, 1, decoded->seller_pid) &&
					   listing.quantity ==
						   std::optional<int64_t>(decoded->item_count);
			if (bound)
				listing.legacy_listing_receipt = *decoded;
			bool complete_slots = bound &&
					      by_listing[index].size() == decoded->item_count;
			if (bound)
				for (uint16_t ordinal = 0; ordinal < decoded->item_count; ++ordinal)
					complete_slots =
						complete_slots &&
						lookup(slots,
						       std::pair{ *listing.auction_id, ordinal })
							.has_value();
			if (!complete_slots)
				mark(listing, auction_census_issue::invalid_slots);
			for (size_t slot_index : by_listing[index])
			{
				auto &slot = report.slots[slot_index];
				const auto &raw = table(7).rows[slot.source.row].cells;
				auction_physical_legacy_identity witness;
				witness.slot_index = slot_index;
				witness.item_uid = slot.item_uid;
				witness.vnum = slot.vnum;
				if (!slot.vnum || *slot.vnum < 0)
					mark(slot, auction_census_issue::invalid_identity);
				witness.parent = listing.source;
				if (input)
					witness.inbox = ref(17, *input);
				slot.legacy_identity_index = report.legacy_identities.size();
				mark(slot, auction_census_issue::original_provenance_unknown);
				bool identity = bound && complete_slots && slot.slot &&
						*slot.slot < decoded->item_count;
				if (identity)
				{
					witness.listing_after_revision =
						decoded->item_revisions[*slot.slot];
					identity =
						slot.item_uid ==
							std::optional<uint64_t>(
								decoded->item_uids[*slot.slot]) &&
						raw[5] && !raw[5]->empty() &&
						raw[5] == parent[11] &&
						(*slot.slot != 0 ||
						 (slot.vnum &&
						  equal<int32_t>(parent, 10, *slot.vnum)));
				}
				if (!identity)
					mark(slot, auction_census_issue::listing_literal_mismatch);
				if (slot.claimed && *slot.claimed)
				{
					if (!witness.listing_after_revision || !slot.row_revision ||
					    *slot.row_revision <= *witness.listing_after_revision ||
					    !slot.claim_pid || !*slot.claim_pid ||
					    !auction_census_operation(raw[7]))
						mark(slot, auction_census_issue::history_metadata);
					report.legacy_identities.push_back(std::move(witness));
					continue;
				}
				auto native = slot.item_uid ? lookup(custody, *slot.item_uid) :
							      std::optional<size_t>{};
				auto gear = slot.item_uid ? lookup(equipment, *slot.item_uid) :
							    std::optional<size_t>{};
				auto owner =
					listing.auction_id ?
						lookup(owners,
						       std::tuple{
							       uint64_t(item_owner_type::auction),
							       uint64_t(*listing.auction_id),
							       uint64_t(0) }) :
						std::optional<size_t>{};
				auto ledger =
					op && slot.slot ?
						lookup(ledgers,
						       std::pair{ *op, uint64_t(*slot.slot) }) :
						std::optional<size_t>{};
				if (native)
					witness.custody = ref(11, *native);
				if (owner)
					witness.owner_revision = ref(12, *owner);
				if (gear)
					witness.equipment = auction_physical_reference{
						false, 0, *gear,
						base.source2.item_equipment_sources[0]
							.rows[*gear]
							.digest,
						true
					};
				if (ledger)
					witness.ledger = ref(2, *ledger, true);
				bool singleton = false;
				if (slot.item_uid)
				{
					auto direct = children.find(*slot.item_uid);
					if (direct != children.end())
						for (size_t row : direct->second)
							if (!extras[row])
							{
								extras[row] = 1;
								report.extra_custody.push_back(
									ref(11, row));
								finding(auction_census_issue::
										extra_custody_descendant,
									ref(11, row));
							}
					auto tree = roots.find(*slot.item_uid);
					singleton = tree != roots.end() &&
						    tree->second.size() == 1 && native &&
						    tree->second[0] == *native &&
						    direct == children.end();
					if (tree != roots.end())
						for (size_t row : tree->second)
							if ((!native || row != *native) &&
							    !extras[row])
							{
								extras[row] = 1;
								report.extra_custody.push_back(
									ref(11, row));
								finding(auction_census_issue::
										extra_custody_descendant,
									ref(11, row));
							}
				}
				bool ledger_match = identity && ledger;
				if (ledger_match)
				{
					const auto &l = table(2, true).rows[*ledger].cells;
					ledger_match =
						equal<uint64_t>(l, 2, *slot.item_uid) &&
						equal<uint64_t>(l, 3, *slot.item_uid) && !l[4] &&
						equal<uint8_t>(l, 5,
							       uint8_t(item_owner_type::player)) &&
						equal<uint64_t>(l, 6, decoded->seller_pid) &&
						equal<uint64_t>(l, 7, 0) &&
						equal<uint8_t>(l, 8,
							       uint8_t(item_owner_type::auction)) &&
						equal<uint64_t>(l, 9, decoded->auction_id) &&
						equal<uint64_t>(l, 10, 0) &&
						equal<uint64_t>(l, 11,
								*witness.listing_after_revision) &&
						equal<uint64_t>(l, 12,
								decoded->player_owner_revision) &&
						equal<uint64_t>(l, 13,
								decoded->auction_owner_revision) &&
						equal<uint16_t>(
							l, 14,
							uint16_t(
								item_transfer_reason::auction_list)) &&
						equal<uint64_t>(l, 15, decoded->auction_id) &&
						source_site(l[16]) && equal<uint16_t>(l, 17, 0) &&
						equal<uint16_t>(l, 18, 0);
				}
				witness.ownership_ledger_proof =
					ledger_match ?
						auction_physical_generic_proof::observed_match :
						(ledger ? auction_physical_generic_proof::conflict :
							  auction_physical_generic_proof::unknown);
				if (!ledger_match)
					mark(slot,
					     ledger ? auction_census_issue::generic_proof_conflict :
						      auction_census_issue::generic_proof_unknown);
				const uint32_t permitted =
					(uint32_t{ 1 } << static_cast<size_t>(
						 auction_census_issue::opaque_literal)) |
					(uint32_t{ 1 } << static_cast<size_t>(
						 auction_census_issue::original_provenance_unknown));
				bool match =
					identity && slot.claimed && !*slot.claimed && native &&
					gear && owner && singleton && ledger_match &&
					!(slot.issue_mask & ~permitted) &&
					!(listing.issue_mask &
					  (uint32_t{ 1 } << static_cast<size_t>(
						   auction_census_issue::invalid_identity))) &&
					!(listing.issue_mask &
					  (uint32_t{ 1 } << static_cast<size_t>(
						   auction_census_issue::duplicate_row_key))) &&
					slot.row_revision == witness.listing_after_revision &&
					slot.vnum &&
					auction_census_number<uint64_t>(
						table(12).rows[*owner].cells[3])
							.value_or(0) > 0;
				if (match)
				{
					const auto &c = table(11).rows[*native].cells;
					match = equal<uint64_t>(c, 1, *slot.item_uid) && !c[2] &&
						equal<uint8_t>(c, 3,
							       uint8_t(item_owner_type::auction)) &&
						equal<uint64_t>(c, 4, *listing.auction_id) &&
						equal<uint64_t>(c, 5, 0) &&
						equal<uint64_t>(c, 6,
								*witness.listing_after_revision) &&
						equal<int32_t>(c, 7, *slot.vnum) &&
						equal<uint8_t>(c, 8, 1) &&
						equal<uint16_t>(base.source2
									.item_equipment_sources[0]
									.rows[*gear]
									.cells,
								1, 0);
				}
				witness.current_field_correspondence = match;
				if (match)
					matched[*native] = 1;
				else
					mark(slot, auction_census_issue::custody_mismatch);
				report.legacy_identities.push_back(std::move(witness));
			}
		}
	}
	void current()
	{
		std::set<uint64_t> observed_roots;
		for (const auto &listing : report.retained_listings)
			for (size_t r = 0; r < listing.receipt.item_count; ++r)
				observed_roots.insert(listing.receipt.item_uids[r]);
		for (const auto &witness : report.legacy_identities)
			if (witness.item_uid)
				observed_roots.insert(*witness.item_uid);
		for (const auto &slot : report.slots)
			if (slot.node_count)
				observed_roots.insert(
					report.nodes[slot.node_begin].literal.object_uid);
		// One base pass retains even a root whose own native root field is wrong.
		for (size_t r = 0; r < table(11).rows.size(); ++r)
		{
			const auto &c = table(11).rows[r].cells;
			auto uid = auction_census_number<uint64_t>(c[0]),
			     root = auction_census_number<uint64_t>(c[1]);
			if ((uid && observed_roots.contains(*uid)) ||
			    (root && observed_roots.contains(*root)) ||
			    (auction_census_number<uint64_t>(c[2]) &&
			     observed_roots.contains(*auction_census_number<uint64_t>(c[2]))))
				report.related_custody.push_back(ref(11, r));
		}
		std::map<uint64_t, std::vector<size_t>> live_uids;
		for (size_t n = 0; n < report.nodes.size(); ++n)
		{
			auto &slot = report.slots[report.nodes[n].slot_index];
			if (slot.claimed && !*slot.claimed)
				live_uids[report.nodes[n].literal.object_uid].push_back(n);
		}
		for (const auto &[uid, observations] : live_uids)
			if (observations.size() > 1)
				for (size_t n : observations)
					mark(report.slots[report.nodes[n].slot_index],
					     auction_census_issue::duplicate_current_uid);
		for (auto &slot : report.slots)
		{
			if (!slot.claimed || *slot.claimed || !slot.node_count ||
			    !slot.listing_index)
				continue;
			const auto observed_root = report.nodes[slot.node_begin].literal.object_uid;
			auto observed_tree = roots.find(observed_root);
			std::set<uint64_t> observed_nodes;
			for (size_t n = slot.node_begin; n < slot.node_begin + slot.node_count; ++n)
				observed_nodes.insert(report.nodes[n].literal.object_uid);
			if (observed_tree != roots.end())
				for (size_t r : observed_tree->second)
				{
					auto uid = auction_census_number<uint64_t>(
						table(11).rows[r].cells[0]);
					if (!uid || !observed_nodes.contains(*uid))
						if (!extras[r])
						{
							extras[r] = 1;
							report.extra_custody.push_back(ref(11, r));
							finding(auction_census_issue::
									extra_custody_descendant,
								ref(11, r));
						}
				}
			const uint32_t blocking =
				slot.issue_mask &
				~(uint32_t{ 1 } << static_cast<size_t>(
					  auction_census_issue::original_provenance_unknown));
			const auto &listing = report.listings[*slot.listing_index];
			const bool bound_parent =
				listing.family == auction_physical_family::observed_native_v2 &&
				listing.listing_receipt && listing.inbox &&
				slot.retained_receipt_index &&
				listing.inbox->row ==
					report.retained_listings[*slot.retained_receipt_index]
						.inbox.row;
			if (!bound_parent)
			{
				mark(slot, auction_census_issue::unbound_listing);
				continue;
			}
			if (blocking || (listing.issue_mask &
					 (uint32_t{ 1 } << static_cast<size_t>(
						  auction_census_issue::invalid_identity))))
				continue;
			const auto id = *slot.auction_id;
			const auto root = report.nodes[slot.node_begin].literal.object_uid;
			auto owner = lookup(owners, std::tuple{ uint64_t(item_owner_type::auction),
								uint64_t(id), uint64_t(0) });
			bool complete = owner && auction_census_number<uint64_t>(
							 table(12).rows[*owner].cells[3])
								 .value_or(0) > 0;
			auto tree = roots.find(root);
			complete = complete && tree != roots.end() &&
				   tree->second.size() == slot.node_count;
			std::set<uint64_t> node_uids;
			for (size_t n = slot.node_begin; n < slot.node_begin + slot.node_count; ++n)
				node_uids.insert(report.nodes[n].literal.object_uid);
			if (tree != roots.end())
				for (size_t row : tree->second)
				{
					auto uid = auction_census_number<uint64_t>(
						table(11).rows[row].cells[0]);
					if (!uid || !node_uids.contains(*uid))
						complete = false;
				}
			for (size_t n = slot.node_begin; n < slot.node_begin + slot.node_count; ++n)
			{
				auto &node = report.nodes[n];
				auto native = lookup(custody, node.literal.object_uid);
				auto gear = lookup(equipment, node.literal.object_uid);
				if (owner)
					node.owner_revision = ref(12, *owner);
				if (gear)
					node.equipment = auction_physical_reference{
						false, 0, *gear,
						base.source2.item_equipment_sources[0]
							.rows[*gear]
							.digest,
						true
					};
				bool match = false;
				if (native)
				{
					node.custody = ref(11, *native);
					const auto &c = table(11).rows[*native].cells;
					const auto expected_parent =
						node.literal.parent_index < 0 ?
							uint64_t(0) :
							report.nodes[slot.node_begin +
								     static_cast<size_t>(
									     node.literal
										     .parent_index)]
								.literal.object_uid;
					const bool parent =
						expected_parent ?
							equal<uint64_t>(c, 2, expected_parent) :
							!c[2];
					match = equal<uint64_t>(c, 1, root) && parent &&
						equal<uint8_t>(c, 3,
							       uint8_t(item_owner_type::auction)) &&
						equal<uint64_t>(c, 4, id) &&
						equal<uint64_t>(c, 5, 0) &&
						equal<uint64_t>(c, 6,
								node.listing_after_revision) &&
						equal<int32_t>(c, 7, node.literal.vnum) &&
						equal<uint8_t>(c, 8, 1) && gear &&
						auction_census_number<uint16_t>(
							base.source2.item_equipment_sources[0]
								.rows[*gear]
								.cells[1]) ==
							std::optional<uint16_t>(0);
				}
				node.current_field_correspondence = complete && match;
				if (node.current_field_correspondence)
					matched[*native] = 1;
				else
					mark(slot, auction_census_issue::custody_mismatch);
			}
		}
		for (const auto &source : report.auction_custody)
			if (!matched[source.row])
			{
				report.unmatched_auction_custody.push_back(source);
				finding(auction_census_issue::unmatched_auction_custody, source);
			}
	}
	bool generic(size_t listing_index, const auction_physical_node &node, uint64_t event,
		     auction_physical_node *bound)
	{
		const auto &listing = report.listings[listing_index];
		const auto &receipt = *listing.listing_receipt;
		auto op = auction_census_operation(table(4).rows[listing.source.row].cells[9]);
		if (!op)
			return false;
		auto ledger = lookup(ledgers, std::pair{ *op, event }),
		     reference = lookup(references, std::pair{ *op, event });
		if (ledger && bound)
			bound->ledger = ref(2, *ledger, true);
		if (reference && bound)
			bound->item_reference = ref(3, *reference, true);
		if (!ledger || !reference || !listing.operation || !listing.outbox)
			return false;
		const auto &o = table(4, true).rows[listing.operation->row].cells;
		if (!equal<uint8_t>(o, 1, 1) || !equal<uint32_t>(o, 2, 0))
			return false;
		const auto &l = table(2, true).rows[*ledger].cells;
		const auto &r = table(3, true).rows[*reference].cells;
		const auto &slot = report.slots[node.slot_index];
		const auto root = report.nodes[slot.node_begin].literal.object_uid;
		const auto parent =
			node.literal.parent_index < 0 ?
				uint64_t(0) :
				report.nodes[slot.node_begin +
					     static_cast<size_t>(node.literal.parent_index)]
					.literal.object_uid;
		bool valid =
			equal<uint64_t>(l, 2, node.literal.object_uid) &&
			equal<uint64_t>(l, 3, root) &&
			(parent ? equal<uint64_t>(l, 4, parent) : !l[4]) &&
			equal<uint8_t>(l, 5, uint8_t(item_owner_type::player)) &&
			equal<uint64_t>(l, 6, receipt.seller_pid) && equal<uint64_t>(l, 7, 0) &&
			equal<uint8_t>(l, 8, uint8_t(item_owner_type::auction)) &&
			equal<uint64_t>(l, 9, receipt.auction_id) && equal<uint64_t>(l, 10, 0) &&
			equal<uint64_t>(l, 11, node.listing_after_revision) &&
			equal<uint64_t>(l, 12, receipt.player_owner_revision) &&
			equal<uint64_t>(l, 13, receipt.auction_owner_revision) &&
			equal<uint16_t>(l, 14, uint16_t(item_transfer_reason::auction_list)) &&
			equal<uint64_t>(l, 15, receipt.auction_id) && source_site(l[16]) &&
			equal<uint16_t>(l, 17, 0) && equal<uint16_t>(l, 18, 0);
		auto before = auction_census_number<uint64_t>(r[5]);
		valid = valid && equal<uint64_t>(r, 1, event) && equal<uint64_t>(r, 2, event) &&
			equal<uint64_t>(r, 3, 0) &&
			equal<uint64_t>(r, 4, node.literal.object_uid) && before &&
			*before != UINT64_MAX && *before + 1 == node.listing_after_revision &&
			equal<uint64_t>(r, 6, node.listing_after_revision) &&
			auction_census_operation(r[7]) == op && equal<uint64_t>(r, 8, event);
		if (bound)
			bound->generic_proof =
				valid ? auction_physical_generic_proof::observed_match :
					auction_physical_generic_proof::conflict;
		return valid;
	}
	void proofs()
	{
		for (size_t a = 0; a < report.listings.size(); ++a)
		{
			auto &listing = report.listings[a];
			if (!listing.listing_receipt || !listing.auction_id)
				continue;
			bool complete = true;
			std::vector<size_t> ordered;
			for (uint16_t slot = 0; slot < listing.listing_receipt->item_count; ++slot)
			{
				auto at = lookup(slots, std::pair{ *listing.auction_id, slot });
				if (!at || !report.slots[*at].node_count)
				{
					complete = false;
					continue;
				}
				const auto &row = report.slots[*at];
				for (size_t n = row.node_begin; n < row.node_begin + row.node_count;
				     ++n)
					ordered.push_back(n);
			}
			if (ordered.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
			{
				complete = false;
				mark(listing, auction_census_issue::native_bound_refused);
			}
			else if (complete)
			{
				std::vector<player_item_snapshot> combined;
				combined.reserve(ordered.size());
				for (uint16_t slot = 0; slot < listing.listing_receipt->item_count;
				     ++slot)
				{
					auto at = lookup(slots,
							 std::pair{ *listing.auction_id, slot });
					const auto &row = report.slots[*at];
					const auto offset = static_cast<int32_t>(combined.size());
					for (size_t n = row.node_begin;
					     n < row.node_begin + row.node_count; ++n)
					{
						combined.push_back(report.nodes[n].literal);
						if (combined.back().parent_index >= 0)
							combined.back().parent_index += offset;
					}
				}
				std::vector<uint8_t> encoded;
				const auto accepted =
					player_item_snapshot_list_encode(combined, &encoded);
				if (accepted == player_snapshot_codec_result::allocation_failure)
					throw std::bad_alloc();
				if (accepted != player_snapshot_codec_result::ok)
				{
					complete = false;
					mark(listing, auction_census_issue::native_bound_refused);
				}
			}
			std::map<uint64_t, std::vector<size_t>> observed;
			for (size_t n : ordered)
				observed[report.nodes[n].literal.object_uid].push_back(n);
			for (const auto &[uid, duplicates] : observed)
				if (duplicates.size() > 1)
				{
					complete = false;
					for (size_t n : duplicates)
						mark(report.slots[report.nodes[n].slot_index],
						     auction_census_issue::listing_literal_mismatch);
				}
			std::optional<uint16_t> site;
			bool conflicting_site = false;
			for (size_t i = 0; i < ordered.size(); ++i)
			{
				auto &node = report.nodes[ordered[i]];
				if (complete && generic(a, node, i, &node))
				{
					const auto observed = auction_census_number<uint16_t>(
						table(2, true).rows[node.ledger->row].cells[16]);
					if (site && site != observed)
						conflicting_site = true;
					site = observed;
				}
				else
					finding(node.generic_proof ==
								auction_physical_generic_proof::
									conflict ?
							auction_census_issue::generic_proof_conflict :
							auction_census_issue::generic_proof_unknown,
						report.slots[node.slot_index].source);
			}
			if (conflicting_site)
				for (size_t n : ordered)
				{
					report.nodes[n].generic_proof =
						auction_physical_generic_proof::conflict;
					finding(auction_census_issue::generic_proof_conflict,
						report.slots[report.nodes[n].slot_index].source);
				}
		}
		std::map<std::string_view, std::optional<uint16_t>> observed_sites;
		std::set<std::string_view> site_conflicts;
		for (size_t r = 0; r < table(2, true).rows.size(); ++r)
		{
			const auto &c = table(2, true).rows[r].cells;
			auto op = auction_census_operation(c[0]);
			if (!op ||
			    !equal<uint16_t>(c, 14, uint16_t(item_transfer_reason::auction_list)))
				continue;
			auto site = auction_census_number<uint16_t>(c[16]);
			auto [at, added] = observed_sites.emplace(*op, site);
			if (!source_site(c[16]) || (!added && at->second != site))
				site_conflicts.insert(*op);
		}
		for (size_t r = 0; r < table(2, true).rows.size(); ++r)
		{
			auto op = auction_census_operation(table(2, true).rows[r].cells[0]);
			if (op && site_conflicts.contains(*op))
				finding(auction_census_issue::generic_proof_conflict,
					ref(2, r, true));
		}
		for (auto &node : report.nodes)
		{
			const auto &slot = report.slots[node.slot_index];
			if (!slot.listing_index)
				continue;
			auto op = auction_census_operation(
				table(4).rows[report.listings[*slot.listing_index].source.row]
					.cells[9]);
			if (op && site_conflicts.contains(*op))
				node.generic_proof = auction_physical_generic_proof::conflict;
		}
		std::map<std::tuple<std::string_view, uint64_t, uint64_t>, size_t> literals;
		for (const auto &node : report.nodes)
		{
			const auto &slot = report.slots[node.slot_index];
			if (!slot.retained_receipt_index)
				continue;
			const auto &receipt =
				report.retained_listings[*slot.retained_receipt_index];
			auto op = auction_census_operation(
				table(17).rows[receipt.inbox.row].cells[0]);
			if (op)
				literals.emplace(std::tuple{ *op, node.literal.object_uid,
							     node.listing_after_revision },
						 node.slot_index);
		}
		for (size_t r = 0; r < table(2, true).rows.size(); ++r)
		{
			const auto &l = table(2, true).rows[r].cells;
			auto op = auction_census_operation(l[0]);
			auto uid = auction_census_number<uint64_t>(l[2]),
			     rev = auction_census_number<uint64_t>(l[11]);
			if (!op || !uid || !*uid || !rev || !*rev)
				continue;
			auto listing = retained_receipts.find(*op);
			if (listing == retained_receipts.end())
				continue;
			const auto &a = report.retained_listings[listing->second];
			if (!a.operation || !a.outbox)
				continue;
			auto event = auction_census_number<uint64_t>(l[1]);
			if (!event)
				continue;
			auto retained_event = lookup(ledgers, std::pair{ *op, *event });
			auto reference = lookup(references, std::pair{ *op, *event });
			if (!retained_event || *retained_event != r || !reference)
				continue;
			const auto &item = table(3, true).rows[*reference].cells;
			const auto &e = table(4, true).rows[a.operation->row].cells;
			const auto &receipt = a.receipt;
			auto before = auction_census_number<uint64_t>(item[5]);
			auto root = auction_census_number<uint64_t>(l[3]);
			const bool generic_bound =
				equal<uint8_t>(e, 1, 1) && equal<uint32_t>(e, 2, 0) && root &&
				*root &&
				std::find(receipt.item_uids.begin(),
					  receipt.item_uids.begin() + receipt.item_count, *root) !=
					receipt.item_uids.begin() + receipt.item_count &&
				equal<uint8_t>(l, 5, uint8_t(item_owner_type::player)) &&
				equal<uint64_t>(l, 6, receipt.seller_pid) &&
				equal<uint64_t>(l, 7, 0) &&
				equal<uint8_t>(l, 8, uint8_t(item_owner_type::auction)) &&
				equal<uint64_t>(l, 9, receipt.auction_id) &&
				equal<uint64_t>(l, 10, 0) &&
				equal<uint64_t>(l, 12, receipt.player_owner_revision) &&
				equal<uint64_t>(l, 13, receipt.auction_owner_revision) &&
				equal<uint16_t>(l, 14,
						uint16_t(item_transfer_reason::auction_list)) &&
				equal<uint64_t>(l, 15, receipt.auction_id) && source_site(l[16]) &&
				equal<uint16_t>(l, 17, 0) && equal<uint16_t>(l, 18, 0) &&
				equal<uint64_t>(item, 1, *event) &&
				equal<uint64_t>(item, 2, *event) && equal<uint64_t>(item, 3, 0) &&
				equal<uint64_t>(item, 4, *uid) && before && *before != UINT64_MAX &&
				*before + 1 == *rev && equal<uint64_t>(item, 6, *rev) &&
				auction_census_operation(item[7]) == op &&
				equal<uint64_t>(item, 8, *event);
			if (generic_bound && !literals.contains(std::tuple{ *op, *uid, *rev }))
			{
				report.missing_literal_events.push_back(ref(2, r, true));
				finding(auction_census_issue::missing_literal, ref(2, r, true));
			}
		}
	}
	void run()
	{
		report.physical_digest = base.digest;
		report.caller_supplied_supplement_digest = supplement.digest;
		for (size_t i = 0; i < 5; ++i)
			report.validated_supplement_table_digests[i] =
				supplement.tables[i].content_digest;
		report.diagnostics.reserve(limit);
		build_indexes();
		read_retained_receipts();
		read_listings();
		read_slots();
		legacy_identities();
		proofs();
		current();
		for (size_t r = 0; r < table(6).rows.size(); ++r)
		{
			const auto &c = table(6).rows[r].cells;
			auction_physical_pickup pickup;
			pickup.source = ref(6, r);
			pickup.row_id = auction_census_number<uint64_t>(c[0]);
			pickup.pid = auction_census_number<uint32_t>(c[1]);
			pickup.retrieved = auction_census_number<int64_t>(c[3]);
			pickup.quantity = auction_census_number<int64_t>(c[4]);
			report.pickups.push_back(pickup);
			finding(auction_census_issue::opaque_literal, pickup.source);
		}
		report.diagnostics_truncated = report.diagnostic_count > report.diagnostics.size();
	}
};
}

unsigned int auction_repository_inspect_physical_sources(
	const economic_sql_physical_source_snapshot &base,
	const sql_room_item_source_snapshot &supplement, const economic_sql_source_limits &limits,
	size_t maximum_diagnostics, auction_physical_correspondence *output) noexcept
{
	try
	{
		auction_census_require(output && maximum_diagnostics && maximum_diagnostics <= 512,
				       EINVAL);
		auction_census_require(supplement.physical_digest == base.digest);
		{
			sql_room_item_source_evidence validated;
			const auto code = sql_room_item_payload_inspect_sources(
				base, supplement.tables, limits, maximum_diagnostics, &validated);
			auction_census_require(!code, code);
		}
		uint64_t rows = base.rows, cells = base.cells, bytes = base.cell_bytes;
		for (const auto &table : supplement.tables)
		{
			auction_census_require(table.rows.size() <= limits.maximum_rows - rows,
					       E2BIG);
			rows += table.rows.size();
			for (const auto &row : table.rows)
			{
				auction_census_require(
					row.cells.size() <= limits.maximum_cells - cells, E2BIG);
				cells += row.cells.size();
				for (const auto &cell : row.cells)
					if (cell)
					{
						auction_census_require(
							cell->size() <=
								limits.maximum_cell_bytes - bytes,
							E2BIG);
						bytes += cell->size();
					}
			}
		}
		auction_census_require(rows == supplement.rows && cells == supplement.cells &&
				       bytes == supplement.cell_bytes);
		auction_census_worker worker{ base, supplement, maximum_diagnostics };
		worker.run();
		*output = std::move(worker.report);
		return 0;
	}
	catch (const auction_census_failure &failure)
	{
		return failure.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EIO;
	}
}

#include "economy/auction_repository.h"

#include "economy/auction_accounting.h"
#include "economy/auction_listing_accounting.h"
#include "economy/auction_money_claim_accounting.h"
#include "economy/auction_item_claim_accounting.h"
#include "economy/auction_settlement_accounting.h"
#include "item/item_transfer_command.h"
#include "persistence/economic_sql_pending_claim_source.h"

#include <algorithm>
#include <array>
#include <cerrno>
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
		    const auction_command_payload &payload, uint32_t *auction_id)
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
		return false;
	*auction_id = static_cast<uint32_t>(mysql_insert_id(connection));
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auction_item_entry &item = payload.items[index];
		sql = "INSERT INTO auction_item_custody(auction_id,slot,item_uid,item_revision,vnum,"
		      "obj_blob) VALUES(" +
		      std::to_string(*auction_id) + "," + std::to_string(index) + "," +
		      std::to_string(item.item_uid) + "," +
		      std::to_string(item.expected_item_revision + 1) + "," +
		      std::to_string(item.vnum) + ",'" + blob + "')";
		if (!execute(connection, sql))
			return false;
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
					    unsigned int *result_code, bool *mutation_applied)
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
		if (!insert_listing(connection, command, payload, &auction_id))
			return false;
		if (!transition_items(connection, command, payload, auction_id,
				      item_owner_type::player, payload.actor_pid,
				      item_owner_type::auction, auction_id, result, result_code))
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
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			if (!execute(
				    connection,
				    "SELECT item_revision FROM auction_item_custody WHERE auction_id=" +
					    std::to_string(auction.id) + " AND item_uid=" +
					    std::to_string(payload.items[index].item_uid) +
					    " AND claim_pid=" + std::to_string(payload.actor_pid) +
					    " AND claimed_at IS NULL FOR UPDATE"))
				return false;
			MYSQL_RES *query = mysql_store_result(connection);
			MYSQL_ROW row = query ? mysql_fetch_row(query) : nullptr;
			uint64_t revision = 0;
			const bool found = row && parse_u64(row[0], &revision) &&
					   revision == payload.items[index].expected_item_revision;
			if (query)
				mysql_free_result(query);
			if (!found)
			{
				*result_code = ESTALE;
				return true;
			}
		}
		if (!transition_items(connection, command, payload, auction.id,
				      item_owner_type::auction, auction.id, item_owner_type::player,
				      payload.actor_pid, result, result_code))
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
		return auction_bid_accounting_decode(command, &intent, &payload, &bid,
						     &bid_accounts) ==
			       economic_accounting_error::ok ||
		       auction_settlement_accounting_decode(command, &intent, &payload, &settlement,
							    &settlement_accounts) ==
			       economic_accounting_error::ok;
	}
	catch (...)
	{
		return false;
	}
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
	if (mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) || reconnect)
		return EPERM;
	const auto session = mysql_thread_id(connection);
	try
	{
		auction_command_payload payload{};
		if (!auction_command_decode_payload(*command, &payload) ||
		    (payload.action != auction_action::bid &&
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
		*command = std::move(frozen);
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

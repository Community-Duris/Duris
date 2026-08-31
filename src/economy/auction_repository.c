#include "economy/auction_repository.h"

#include "item/item_transfer_command.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <climits>
#include <cstring>
#include <ctime>
#include <limits>
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

bool auction_repository_execute(MYSQL *connection, const critical_command &command,
				auction_command_result *result, unsigned int *result_code,
				bool *mutation_applied)
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
		if (!apply_wallet_delta(connection, command, payload, -to_pay, &wallet,
					result_code))
			return false;
		if (*result_code)
			return true;
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
		result->wallet_value_delta = -to_pay;
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

#include "combat/combat_outcome_repository.h"

#include "world/epic_command.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <climits>
#include <cstring>
#include <limits>
#include <mysql.h>
#include <openssl/sha.h>
#include <string>
#include <vector>

namespace
{
constexpr std::array<int64_t, CURRENCY_DENOMINATION_COUNT> COIN_VALUES = { 1, 10, 100, 1000 };

struct player_state
{
	size_t payload_index;
	uint32_t pid;
	std::string account_name;
	uint8_t racewar;
	int64_t frags;
	int64_t epics;
	currency_vector wallet;
	uint64_t frag_revision;
	uint64_t epic_revision;
	uint64_t wallet_revision;
};

struct bank_state
{
	std::string account_name;
	uint8_t racewar;
	uint32_t bank_id;
	currency_vector balance;
	uint64_t revision;
};

bool execute(MYSQL *connection, const std::string &sql)
{
	return mysql_real_query(connection, sql.data(), sql.size()) == 0;
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

std::string bytes_hex(const uint8_t *bytes, size_t size)
{
	static const char HEX[] = "0123456789abcdef";
	std::string result(size * 2, '0');
	for (size_t index = 0; index < size; ++index)
	{
		result[index * 2] = HEX[bytes[index] >> 4];
		result[index * 2 + 1] = HEX[bytes[index] & 15];
	}
	return result;
}

critical_operation_id child_operation(const critical_operation_id &parent, uint8_t domain,
				      uint16_t index)
{
	std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES + 3> input = {};
	std::copy(parent.bytes.begin(), parent.bytes.end(), input.begin());
	input[CRITICAL_COMMAND_ID_BYTES] = domain;
	input[CRITICAL_COMMAND_ID_BYTES + 1] = static_cast<uint8_t>(index);
	input[CRITICAL_COMMAND_ID_BYTES + 2] = static_cast<uint8_t>(index >> 8);
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(input.data(), input.size(), digest.data());
	critical_operation_id result = {};
	std::copy_n(digest.begin(), result.bytes.size(), result.bytes.begin());
	return result;
}

bool insert_child_inbox(MYSQL *connection, const critical_operation_id &child,
			critical_command_type type, uint64_t revision)
{
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(child.bytes.data(), child.bytes.size(), digest.data());
	const std::string operation = bytes_hex(child.bytes.data(), child.bytes.size());
	const std::string hash = bytes_hex(digest.data(), digest.size());
	const std::string sql =
		"INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
		"command_type,schema_version,payload_version,status,result_code,durable_revision,"
		"result_payload,committed_at) VALUES(UNHEX('" +
		operation + "'),UNHEX('" + hash + "'),UNHEX('" + hash + "')," +
		std::to_string(static_cast<uint16_t>(type)) + ",1,1,1,0," +
		std::to_string(revision) + ",'',CURRENT_TIMESTAMP(6))";
	return execute(connection, sql);
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

bool load_player(MYSQL *connection, uint32_t pid, size_t payload_index, player_state *state)
{
	const std::string sql =
		"SELECT account_name,racewar,frags,frag_revision,epics,epic_revision,copper,"
		"silver,gold,platinum,wallet_revision FROM player_data WHERE pid=" +
		std::to_string(pid) + " FOR UPDATE";
	if (!execute(connection, sql))
		return false;
	MYSQL_RES *result = mysql_store_result(connection);
	MYSQL_ROW row = result ? mysql_fetch_row(result) : nullptr;
	int64_t signed_values[6] = {};
	uint64_t revisions[3] = {};
	const bool ok = row && row[0] && row[1] && parse_i64(row[2], &signed_values[0]) &&
			parse_u64(row[3], &revisions[0]) && parse_i64(row[4], &signed_values[1]) &&
			parse_u64(row[5], &revisions[1]) && parse_i64(row[6], &signed_values[2]) &&
			parse_i64(row[7], &signed_values[3]) &&
			parse_i64(row[8], &signed_values[4]) &&
			parse_i64(row[9], &signed_values[5]) && parse_u64(row[10], &revisions[2]);
	if (ok)
	{
		state->payload_index = payload_index;
		state->pid = pid;
		state->account_name = row[0];
		state->racewar = static_cast<uint8_t>(atoi(row[1]));
		state->frags = signed_values[0];
		state->epics = signed_values[1];
		for (size_t index = 0; index < state->wallet.amount.size(); ++index)
			state->wallet.amount[index] = signed_values[index + 2];
		state->frag_revision = revisions[0];
		state->epic_revision = revisions[1];
		state->wallet_revision = revisions[2];
	}
	if (result)
		mysql_free_result(result);
	return ok;
}

bool load_bank(MYSQL *connection, const std::string &account_name, uint8_t racewar,
	       bank_state *state)
{
	const std::string account = escape(connection, account_name.data(), account_name.size());
	const std::string sql =
		"SELECT id,bank_copper,bank_silver,bank_gold,bank_platinum,bank_revision FROM "
		"account_banks WHERE account_name='" +
		account + "' AND racewar=" + std::to_string(racewar) + " FOR UPDATE";
	if (!execute(connection, sql))
		return false;
	MYSQL_RES *result = mysql_store_result(connection);
	MYSQL_ROW row = result ? mysql_fetch_row(result) : nullptr;
	uint64_t values[6] = {};
	bool ok = row;
	for (size_t index = 0; ok && index < 6; ++index)
		ok = parse_u64(row[index], &values[index]);
	if (ok)
	{
		state->account_name = account_name;
		state->racewar = racewar;
		state->bank_id = static_cast<uint32_t>(values[0]);
		for (size_t index = 0; index < state->balance.amount.size(); ++index)
			state->balance.amount[index] = static_cast<int64_t>(values[index + 1]);
		state->revision = values[5];
	}
	if (result)
		mysql_free_result(result);
	return ok;
}

bank_state *find_bank(std::vector<bank_state> *banks, const combat_outcome_participant &entry)
{
	auto found = std::find_if(banks->begin(), banks->end(),
				  [&](const auto &bank)
				  {
					  return bank.racewar == entry.racewar &&
						 !strcasecmp(bank.account_name.c_str(),
							     entry.account_name.data());
				  });
	return found == banks->end() ? nullptr : &*found;
}
} // namespace

bool combat_outcome_repository_execute(MYSQL *connection, const critical_command &command,
				       combat_outcome_result *result, unsigned int *result_code,
				       bool *mutation_applied)
{
	if (!connection || !result || !result_code || !mutation_applied)
		return false;
	combat_outcome_payload payload = {};
	if (!combat_outcome_command_decode_payload(command, &payload))
	{
		errno = EINVAL;
		return false;
	}
	*result = {};
	*result_code = 0;
	*mutation_applied = false;
	std::vector<size_t> order(payload.participant_count);
	for (size_t index = 0; index < order.size(); ++index)
		order[index] = index;
	std::sort(order.begin(), order.end(), [&](size_t left, size_t right)
		  { return payload.participants[left].pid < payload.participants[right].pid; });
	std::vector<player_state> players;
	players.reserve(order.size());
	for (size_t index : order)
	{
		player_state state = {};
		const auto &entry = payload.participants[index];
		if (!load_player(connection, entry.pid, index, &state))
		{
			*result_code = ENOENT;
			return true;
		}
		if (state.frag_revision != entry.expected_frag_revision ||
		    state.epic_revision != entry.expected_epic_revision ||
		    state.wallet_revision != entry.expected_wallet_revision ||
		    (entry.wallet_delta_copper &&
		     (state.racewar != entry.racewar ||
		      strcasecmp(state.account_name.c_str(), entry.account_name.data()))))
		{
			*result_code = ESTALE;
			return true;
		}
		players.push_back(std::move(state));
	}
	std::vector<std::pair<std::string, uint8_t>> bank_keys;
	for (const auto &entry : payload.participants)
		if (entry.wallet_delta_copper)
			bank_keys.emplace_back(entry.account_name.data(), entry.racewar);
	std::sort(bank_keys.begin(), bank_keys.end());
	bank_keys.erase(std::unique(bank_keys.begin(), bank_keys.end()), bank_keys.end());
	std::vector<bank_state> banks;
	for (const auto &[account, racewar] : bank_keys)
	{
		bank_state state = {};
		if (!load_bank(connection, account, racewar, &state))
		{
			*result_code = ENOENT;
			return true;
		}
		banks.push_back(std::move(state));
	}
	for (const auto &entry : payload.participants)
		if (entry.wallet_delta_copper)
		{
			bank_state *bank = find_bank(&banks, entry);
			if (!bank || bank->revision != entry.expected_bank_revision)
			{
				*result_code = ESTALE;
				return true;
			}
		}

	const std::string operation =
		bytes_hex(command.operation_id.bytes.data(), command.operation_id.bytes.size());
	const std::string room =
		escape(connection, payload.room_name.data(),
		       strnlen(payload.room_name.data(), payload.room_name.size()));
	if (!execute(connection, "INSERT INTO pkill_event(stamp,room_vnum,room_name) VALUES("
				 "NOW()," +
					 std::to_string(payload.room_vnum) + ",'" + room + "')"))
		return false;
	const uint64_t event_id = mysql_insert_id(connection);
	if (!execute(connection,
		     "INSERT INTO combat_outcome(operation_id,pkill_event_id,victim_pid,room_vnum,"
		     "participant_count) VALUES(UNHEX('" +
			     operation + "')," + std::to_string(event_id) + "," +
			     std::to_string(payload.victim_pid) + "," +
			     std::to_string(payload.room_vnum) + "," +
			     std::to_string(payload.participant_count) + ")"))
		return false;

	result->event_id = event_id;
	result->participant_count = payload.participant_count;
	for (player_state &state : players)
	{
		const size_t index = state.payload_index;
		const auto &entry = payload.participants[index];
		if ((entry.frag_delta > 0 && state.frags > INT64_MAX - entry.frag_delta) ||
		    (entry.frag_delta < 0 && state.frags < INT64_MIN - entry.frag_delta) ||
		    (entry.epic_delta > 0 && state.epics > INT64_MAX - entry.epic_delta) ||
		    state.frag_revision == UINT64_MAX || state.epic_revision == UINT64_MAX ||
		    state.wallet_revision == UINT64_MAX)
		{
			errno = ERANGE;
			return false;
		}
		if (entry.frag_delta)
		{
			if (!execute(connection,
				     "INSERT IGNORE INTO combat_frag_baseline(pid,opening_frags,"
				     "opening_revision) VALUES(" +
					     std::to_string(entry.pid) + "," +
					     std::to_string(state.frags) + "," +
					     std::to_string(state.frag_revision) + ")"))
				return false;
			const int64_t prior = state.frags;
			state.frags += entry.frag_delta;
			++state.frag_revision;
			if (!execute(connection,
				     "UPDATE player_data SET oldfrags=" + std::to_string(prior) +
					     ",frags=" + std::to_string(state.frags) +
					     ",frag_revision=" +
					     std::to_string(state.frag_revision) + " WHERE pid=" +
					     std::to_string(entry.pid) + " AND frag_revision=" +
					     std::to_string(state.frag_revision - 1)) ||
			    mysql_affected_rows(connection) != 1 ||
			    !execute(connection,
				     "INSERT INTO combat_frag_ledger(operation_id,participant_index,pid,"
				     "delta,frags_after,frag_revision) VALUES(UNHEX('" +
					     operation + "')," + std::to_string(index) + "," +
					     std::to_string(entry.pid) + "," +
					     std::to_string(entry.frag_delta) + "," +
					     std::to_string(state.frags) + "," +
					     std::to_string(state.frag_revision) + ")") ||
			    !execute(connection, "UPDATE frag_leaderboard SET total_frags=" +
							 std::to_string(state.frags) +
							 ",last_updated=NOW() WHERE pid=" +
							 std::to_string(entry.pid) +
							 " AND deleted_at IS NULL") ||
			    !execute(connection, "INSERT INTO progress VALUES(0," +
							 std::to_string(entry.pid) + ",1,NOW()," +
							 std::to_string(entry.frag_delta) + ")"))
				return false;
		}
		if (entry.epic_delta)
		{
			if (!execute(connection,
				     "INSERT IGNORE INTO epic_balance_baseline(pid,opening_balance,"
				     "opening_revision) VALUES(" +
					     std::to_string(entry.pid) + "," +
					     std::to_string(state.epics) + "," +
					     std::to_string(state.epic_revision) + ")"))
				return false;
			state.epics += entry.epic_delta;
			++state.epic_revision;
			const critical_operation_id child =
				child_operation(command.operation_id, 1, index);
			const std::string child_hex =
				bytes_hex(child.bytes.data(), child.bytes.size());
			if (!insert_child_inbox(connection, child, critical_command_type::epic,
						state.epic_revision) ||
			    !execute(connection,
				     "UPDATE player_data SET epics=" + std::to_string(state.epics) +
					     ",epic_revision=" +
					     std::to_string(state.epic_revision) + " WHERE pid=" +
					     std::to_string(entry.pid) + " AND epic_revision=" +
					     std::to_string(state.epic_revision - 1)) ||
			    mysql_affected_rows(connection) != 1 ||
			    !execute(
				    connection,
				    "INSERT INTO epic_ledger(operation_id,pid,delta,balance_after,"
				    "epic_revision,reason_type,reason_id,source_site) VALUES(UNHEX('" +
					    child_hex + "')," + std::to_string(entry.pid) + "," +
					    std::to_string(entry.epic_delta) + "," +
					    std::to_string(state.epics) + "," +
					    std::to_string(state.epic_revision) + "," +
					    std::to_string(static_cast<uint16_t>(
						    epic_reason_type::pvp_award)) +
					    "," + std::to_string(payload.victim_pid) + "," +
					    std::to_string(static_cast<uint16_t>(
						    critical_source_site::combat)) +
					    ")"))
				return false;
		}
		bank_state *bank = nullptr;
		int64_t current_wallet = wallet_value(state.wallet);
		if (entry.wallet_delta_copper)
		{
			bank = find_bank(&banks, entry);
			if (!bank || current_wallet < 0 || entry.wallet_delta_copper < 0 ||
			    current_wallet > INT64_MAX - entry.wallet_delta_copper ||
			    bank->revision == UINT64_MAX)
			{
				errno = ERANGE;
				return false;
			}
			if (!execute(
				    connection,
				    "INSERT IGNORE INTO currency_wallet_baseline(pid,opening_copper,"
				    "opening_silver,opening_gold,opening_platinum,opening_revision) VALUES(" +
					    std::to_string(entry.pid) + "," +
					    std::to_string(state.wallet.amount[0]) + "," +
					    std::to_string(state.wallet.amount[1]) + "," +
					    std::to_string(state.wallet.amount[2]) + "," +
					    std::to_string(state.wallet.amount[3]) + "," +
					    std::to_string(state.wallet_revision) + ")") ||
			    !execute(
				    connection,
				    "INSERT IGNORE INTO currency_bank_baseline(bank_id,opening_copper,"
				    "opening_silver,opening_gold,opening_platinum,opening_revision) VALUES(" +
					    std::to_string(bank->bank_id) + "," +
					    std::to_string(bank->balance.amount[0]) + "," +
					    std::to_string(bank->balance.amount[1]) + "," +
					    std::to_string(bank->balance.amount[2]) + "," +
					    std::to_string(bank->balance.amount[3]) + "," +
					    std::to_string(bank->revision) + ")"))
				return false;
			const currency_vector prior_wallet = state.wallet;
			current_wallet += entry.wallet_delta_copper;
			state.wallet = canonical_wallet(current_wallet);
			currency_vector wallet_delta = {};
			for (size_t denomination = 0; denomination < wallet_delta.amount.size();
			     ++denomination)
				wallet_delta.amount[denomination] =
					state.wallet.amount[denomination] -
					prior_wallet.amount[denomination];
			++state.wallet_revision;
			++bank->revision;
			const critical_operation_id child =
				child_operation(command.operation_id, 2, index);
			const std::string child_hex =
				bytes_hex(child.bytes.data(), child.bytes.size());
			if (!insert_child_inbox(connection, child,
						critical_command_type::account_bank,
						std::max(state.wallet_revision, bank->revision)) ||
			    !execute(connection,
				     "UPDATE player_data SET copper=" +
					     std::to_string(state.wallet.amount[0]) +
					     ",silver=" + std::to_string(state.wallet.amount[1]) +
					     ",gold=" + std::to_string(state.wallet.amount[2]) +
					     ",platinum=" + std::to_string(state.wallet.amount[3]) +
					     ",wallet_revision=" +
					     std::to_string(state.wallet_revision) + " WHERE pid=" +
					     std::to_string(entry.pid) + " AND wallet_revision=" +
					     std::to_string(state.wallet_revision - 1)) ||
			    mysql_affected_rows(connection) != 1 ||
			    !execute(connection,
				     "UPDATE account_banks SET bank_revision=" +
					     std::to_string(bank->revision) + " WHERE id=" +
					     std::to_string(bank->bank_id) + " AND bank_revision=" +
					     std::to_string(bank->revision - 1)) ||
			    mysql_affected_rows(connection) != 1)
				return false;
			std::string ledger =
				"INSERT INTO currency_ledger(operation_id,pid,bank_id,"
				"wallet_delta_copper,wallet_delta_silver,wallet_delta_gold,"
				"wallet_delta_platinum,bank_delta_copper,bank_delta_silver,"
				"bank_delta_gold,bank_delta_platinum,wallet_after_copper,"
				"wallet_after_silver,wallet_after_gold,wallet_after_platinum,"
				"bank_after_copper,bank_after_silver,bank_after_gold,"
				"bank_after_platinum,wallet_revision,bank_revision,reason_type,"
				"reason_id,source_site) VALUES(UNHEX('" +
				child_hex + "')," + std::to_string(entry.pid) + "," +
				std::to_string(bank->bank_id);
			for (int64_t amount : wallet_delta.amount)
				ledger += "," + std::to_string(amount);
			ledger += ",0,0,0,0";
			for (int64_t amount : state.wallet.amount)
				ledger += "," + std::to_string(amount);
			for (int64_t amount : bank->balance.amount)
				ledger += "," + std::to_string(amount);
			ledger += "," + std::to_string(state.wallet_revision) + "," +
				  std::to_string(bank->revision) + "," +
				  std::to_string(static_cast<uint16_t>(
					  currency_reason_type::wallet_reward)) +
				  "," + std::to_string(payload.victim_pid) + "," +
				  std::to_string(
					  static_cast<uint16_t>(critical_source_site::combat)) +
				  ")";
			if (!execute(connection, ledger))
				return false;
		}
		const char *role = entry.role == combat_participant_role::victim ? "VICTIM" :
				   entry.role == combat_participant_role::victim_group ?
										   "VICTIM-GROUP" :
										   "KILLER";
		const std::string description =
			escape(connection, entry.description.data(),
			       strnlen(entry.description.data(), entry.description.size()));
		if (!execute(connection,
			     "INSERT INTO pkill_info(event_id,pid,level,pk_type,player_description,"
			     "equip,log,inroom,leader) VALUES(" +
				     std::to_string(event_id) + "," + std::to_string(entry.pid) +
				     "," + std::to_string(entry.level) + ",'" + role + "','" +
				     description + "','',''," +
				     std::to_string((entry.flags & COMBAT_PARTICIPANT_IN_ROOM) !=
						    0) +
				     "," +
				     std::to_string((entry.flags & COMBAT_PARTICIPANT_LEADER) != 0) +
				     ")"))
			return false;
		const uint64_t bank_revision = bank ? bank->revision : entry.expected_bank_revision;
		if (!execute(
			    connection,
			    "INSERT INTO combat_outcome_participant(operation_id,participant_index,pid,"
			    "role,flags,frag_delta,epic_delta,wallet_delta_copper,frag_after,"
			    "frag_revision,epic_revision,wallet_revision,bank_revision) VALUES(UNHEX('" +
				    operation + "')," + std::to_string(index) + "," +
				    std::to_string(entry.pid) + "," +
				    std::to_string(static_cast<uint8_t>(entry.role)) + "," +
				    std::to_string(entry.flags) + "," +
				    std::to_string(entry.frag_delta) + "," +
				    std::to_string(entry.epic_delta) + "," +
				    std::to_string(entry.wallet_delta_copper) + "," +
				    std::to_string(state.frags) + "," +
				    std::to_string(state.frag_revision) + "," +
				    std::to_string(state.epic_revision) + "," +
				    std::to_string(state.wallet_revision) + "," +
				    std::to_string(bank_revision) + ")"))
			return false;
		result->participants[index] = { .pid = entry.pid,
						.frags = state.frags,
						.epics = state.epics,
						.wallet_value = entry.wallet_delta_copper ?
									current_wallet :
									-1,
						.bank = bank ? bank->balance : currency_vector{},
						.frag_revision = state.frag_revision,
						.epic_revision = state.epic_revision,
						.wallet_revision = state.wallet_revision,
						.bank_revision = bank_revision };
	}
	for (size_t index = 0; index < payload.participant_count; ++index)
		if (payload.participants[index].wallet_delta_copper)
		{
			bank_state *bank = find_bank(&banks, payload.participants[index]);
			if (bank)
				result->participants[index].bank_revision = bank->revision;
		}
	*mutation_applied = true;
	return true;
}

#include "critical_command_repository.h"
#include "currency_command.h"

#include <mysql.h>

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

extern "C" MYSQL *sql_pool_acquire(void)
{
	return nullptr;
}
extern "C" void sql_pool_release(MYSQL *) {}
extern "C" MYSQL *sql_pool_replace_connection(MYSQL *)
{
	return nullptr;
}

namespace
{
MYSQL *connection = nullptr;

void execute(const std::string &sql)
{
	assert(mysql_query(connection, sql.c_str()) == 0);
}

long long scalar(const std::string &sql)
{
	execute(sql);
	MYSQL_RES *result = mysql_store_result(connection);
	assert(result);
	MYSQL_ROW row = mysql_fetch_row(result);
	assert(row && row[0]);
	const long long value = strtoll(row[0], nullptr, 10);
	mysql_free_result(result);
	return value;
}

std::string operation_hex(const critical_operation_id &operation_id)
{
	char value[CRITICAL_COMMAND_ID_HEX_SIZE] = {};
	assert(critical_operation_id_to_hex(operation_id, value, sizeof(value)));
	return value;
}

critical_command command_for(uint32_t pid, const char *account_name,
			     const currency_vector &wallet_delta, const currency_vector &bank_delta,
			     currency_reason_type reason, uint64_t wallet_revision = UINT64_MAX,
			     uint64_t bank_revision = UINT64_MAX)
{
	critical_operation_id operation_id = {};
	assert(critical_operation_id_generate(&operation_id));
	currency_command_payload payload = { .pid = pid,
					     .racewar = 1,
					     .reason = reason,
					     .reason_id = 77,
					     .account_name = {},
					     .wallet_delta = wallet_delta,
					     .bank_delta = bank_delta };
	assert(strlen(account_name) < payload.account_name.size());
	memcpy(payload.account_name.data(), account_name, strlen(account_name));
	critical_command command = {};
	assert(currency_command_build(&command, operation_id, payload, wallet_revision,
				      bank_revision, critical_source_site::command,
				      critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	return command;
}

currency_command_result result_of(const critical_apply_result &applied)
{
	currency_command_result result = {};
	assert(currency_command_decode_result(applied.result_payload.data(), applied.result_size,
					      &result));
	return result;
}
} // namespace

int main()
{
	const char *host = getenv("DB_HOST"), *user = getenv("DB_USER"),
		   *password = getenv("DB_PASSWD"), *database = getenv("CURRENCY_TEST_DB_NAME"),
		   *port_value = getenv("DB_PORT");
	assert(host && user && password && database);
	connection = mysql_init(nullptr);
	assert(connection);
	const unsigned int port = port_value ? static_cast<unsigned int>(atoi(port_value)) : 3306;
	assert(mysql_real_connect(connection, host, user, password, database, port, nullptr, 0));

	const std::string account = "currency_harness_account";
	execute("DELETE FROM currency_wallet_baseline WHERE pid IN (SELECT pid FROM player_data "
		"WHERE name='CurrencyHarness')");
	execute("DELETE FROM currency_bank_baseline WHERE bank_id IN (SELECT id FROM account_banks "
		"WHERE account_name='" +
		account + "')");
	execute("DELETE FROM player_data WHERE name='CurrencyHarness'");
	execute("DELETE FROM account_banks WHERE account_name='" + account + "'");
	execute("DELETE FROM accounts WHERE account_name='" + account + "'");
	execute("INSERT INTO accounts(account_name,password) VALUES('" + account +
		"','') ON DUPLICATE KEY UPDATE account_name=VALUES(account_name)");
	execute("INSERT INTO player_data(name,account_name,racewar,copper,silver,gold,platinum) "
		"VALUES('CurrencyHarness','" +
		account + "',1,9,8,7,6)");
	const uint32_t pid = static_cast<uint32_t>(mysql_insert_id(connection));
	execute("INSERT INTO account_banks(account_name,racewar,bank_copper,bank_silver,bank_gold,"
		"bank_platinum) VALUES('" +
		account + "',1,1,2,3,4)");
	const uint32_t bank_id = static_cast<uint32_t>(mysql_insert_id(connection));

	const currency_vector wallet_deposit = { { -9, -8, -7, -6 } };
	const currency_vector bank_deposit = { { 9, 8, 7, 6 } };
	critical_command deposit = command_for(pid, account.c_str(), wallet_deposit, bank_deposit,
					       currency_reason_type::atm_deposit);
	currency_command_payload decoded_deposit = {};
	assert(currency_command_decode_payload(deposit, &decoded_deposit));
	assert(critical_command_valid(deposit));
	std::vector<std::string> operations = { operation_hex(deposit.operation_id) };
	critical_apply_result applied = critical_command_repository_apply(connection, deposit);
	if (applied.outcome != critical_apply_outcome::applied || applied.error_code != 0)
		fprintf(stderr, "currency deposit failed outcome=%u error=%u mysql=%u %s\n",
			static_cast<unsigned int>(applied.outcome), applied.error_code,
			mysql_errno(connection), mysql_error(connection));
	assert(applied.outcome == critical_apply_outcome::applied && applied.error_code == 0);
	currency_command_result result = result_of(applied);
	const currency_vector empty_balances = {};
	const currency_vector deposited_balances = { { 10, 10, 10, 10 } };
	assert(result.wallet.amount == empty_balances.amount);
	assert(result.bank.amount == deposited_balances.amount);
	assert(result.wallet_revision == 1 && result.bank_revision == 1);
	critical_apply_result duplicate = critical_command_repository_apply(connection, deposit);
	assert(duplicate.outcome == critical_apply_outcome::already_applied);
	assert(result_of(duplicate).bank_revision == 1);

	const currency_vector wallet_withdraw = { { 0, 0, 0, 5 } };
	const currency_vector bank_withdraw = { { 0, 0, 0, -5 } };
	critical_command withdrawal = command_for(pid, account.c_str(), wallet_withdraw,
						  bank_withdraw,
						  currency_reason_type::atm_withdraw);
	operations.push_back(operation_hex(withdrawal.operation_id));
	applied = critical_command_repository_apply(connection, withdrawal);
	assert(applied.outcome == critical_apply_outcome::applied);
	result = result_of(applied);
	assert(result.wallet.amount[3] == 5 && result.bank.amount[3] == 5);

	const currency_vector empty = {};
	const currency_vector excessive = { { 0, 0, 0, -100 } };
	critical_command rejected = command_for(pid, account.c_str(), empty, excessive,
						currency_reason_type::atm_withdraw);
	operations.push_back(operation_hex(rejected.operation_id));
	applied = critical_command_repository_apply(connection, rejected);
	assert(applied.outcome == critical_apply_outcome::terminal_failure &&
	       applied.error_code == ENOSPC);
	duplicate = critical_command_repository_apply(connection, rejected);
	assert(duplicate.outcome == critical_apply_outcome::terminal_failure &&
	       duplicate.error_code == ENOSPC);

	const currency_vector reward = { { 3, 0, 0, 0 } };
	critical_command stale = command_for(pid, account.c_str(), reward, empty,
					     currency_reason_type::wallet_reward, 0, 2);
	operations.push_back(operation_hex(stale.operation_id));
	applied = critical_command_repository_apply(connection, stale);
	assert(applied.outcome == critical_apply_outcome::terminal_failure &&
	       applied.error_code == ESTALE);

	execute("UPDATE account_banks SET bank_copper=2147483647 WHERE id=" +
		std::to_string(bank_id));
	const currency_vector overflow_delta = { { 1, 0, 0, 0 } };
	critical_command overflow = command_for(pid, account.c_str(), empty, overflow_delta,
						currency_reason_type::bank_reward);
	operations.push_back(operation_hex(overflow.operation_id));
	applied = critical_command_repository_apply(connection, overflow);
	assert(applied.outcome == critical_apply_outcome::terminal_failure &&
	       applied.error_code == ERANGE);
	execute("UPDATE account_banks SET bank_copper=10 WHERE id=" + std::to_string(bank_id));

	assert(scalar("SELECT copper FROM player_data WHERE pid=" + std::to_string(pid)) == 0);
	assert(scalar("SELECT platinum FROM player_data WHERE pid=" + std::to_string(pid)) == 5);
	assert(scalar("SELECT bank_platinum FROM account_banks WHERE id=" +
		      std::to_string(bank_id)) == 5);
	assert(scalar("SELECT COUNT(*) FROM currency_ledger WHERE pid=" + std::to_string(pid)) ==
	       2);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox o JOIN currency_ledger l "
		      "ON l.operation_id=o.operation_id WHERE l.pid=" +
		      std::to_string(pid)) == 2);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE operation_id=UNHEX('" +
		      operation_hex(rejected.operation_id) + "')") == 0);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE operation_id=UNHEX('" +
		      operation_hex(overflow.operation_id) + "')") == 0);

	for (const std::string &operation : operations)
	{
		execute("DELETE d FROM critical_outbox_delivery_dedupe d JOIN critical_outbox o "
			"ON o.outbox_id=d.outbox_id WHERE o.operation_id=UNHEX('" +
			operation + "')");
		execute("DELETE FROM critical_outbox WHERE operation_id=UNHEX('" + operation +
			"')");
		execute("DELETE FROM currency_ledger WHERE operation_id=UNHEX('" + operation +
			"')");
		execute("DELETE FROM critical_operation_inbox WHERE operation_id=UNHEX('" +
			operation + "')");
	}
	execute("DELETE FROM currency_wallet_baseline WHERE pid=" + std::to_string(pid));
	execute("DELETE FROM currency_bank_baseline WHERE bank_id=" + std::to_string(bank_id));
	execute("DELETE FROM player_data WHERE pid=" + std::to_string(pid));
	execute("DELETE FROM account_banks WHERE id=" + std::to_string(bank_id));
	execute("DELETE FROM accounts WHERE account_name='" + account + "'");
	mysql_close(connection);
	return 0;
}

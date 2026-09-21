#include "persistence/economic_sql_bank_transaction.h"
#include "persistence/critical_command_repository.h"
#include <openssl/sha.h>

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>
#include <thread>
#include <barrier>

// Test-only link wrapper: let the real server commit, then hide its reply from
// the repository. This exercises its ambiguous outcome and fresh reconciliation.
static MYSQL *lose_commit_reply = nullptr;
static MYSQL *lost_commit_connection = nullptr;
extern "C" int __real_mysql_real_query(MYSQL *, const char *, unsigned long);
extern "C" unsigned int __real_mysql_errno(MYSQL *);
extern "C" int __wrap_mysql_real_query(MYSQL *connection, const char *sql, unsigned long length)
{
	const int result = __real_mysql_real_query(connection, sql, length);
	if (!result && connection == lose_commit_reply && length == 6 && !memcmp(sql, "COMMIT", 6))
	{
		lose_commit_reply = nullptr;
		lost_commit_connection = connection;
		return 1;
	}
	return result;
}
extern "C" unsigned int __wrap_mysql_errno(MYSQL *connection)
{
	return connection == lost_commit_connection ? 2013 : __real_mysql_errno(connection);
}

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
const char *required(const char *name)
{
	const char *value = getenv(name);
	assert(value && *value);
	return value;
}

MYSQL *connect_fixture()
{
	// Guard before constructing any client. No option files or inherited socket.
	assert(!strcmp(required("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA"), "1"));
	assert(!strcmp(required("DB_HOST"), "127.0.0.1"));
	assert(!getenv("DB_SOCKET") || !*getenv("DB_SOCKET"));
	const std::string schema = required("DB_NAME");
	assert(schema.starts_with("economic_schema_test_") &&
	       schema.find_first_not_of(
		       "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") ==
		       std::string::npos);
	char *end = nullptr;
	const auto port = strtoul(required("DB_PORT"), &end, 10);
	assert(end && !*end && port > 0 && port <= 65535);
	auto *connection = mysql_init(nullptr);
	assert(connection);
	unsigned int timeout = 5, protocol = MYSQL_PROTOCOL_TCP;
	assert(!mysql_options(connection, MYSQL_OPT_CONNECT_TIMEOUT, &timeout));
	assert(!mysql_options(connection, MYSQL_OPT_READ_TIMEOUT, &timeout));
	assert(!mysql_options(connection, MYSQL_OPT_WRITE_TIMEOUT, &timeout));
	assert(!mysql_options(connection, MYSQL_OPT_PROTOCOL, &protocol));
	assert(mysql_real_connect(connection, "127.0.0.1", required("DB_USER"),
				  required("DB_PASSWD"), schema.c_str(),
				  static_cast<unsigned int>(port), nullptr, 0));
	return connection;
}

void execute(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()))
	{
		fprintf(stderr, "synthetic authority fixture failed, SQL error %u\n",
			mysql_errno(connection));
		abort();
	}
}

critical_operation_id new_id()
{
	critical_operation_id id = {};
	assert(critical_operation_id_generate(&id));
	return id;
}

std::string literal(const critical_operation_id &id)
{
	char hex[CRITICAL_COMMAND_ID_HEX_SIZE] = {};
	assert(critical_operation_id_to_hex(id, hex, sizeof(hex)));
	return std::string("UNHEX('") + hex + "')";
}
}

namespace
{
std::string binary(std::span<const uint8_t> bytes)
{
	constexpr char digits[] = "0123456789abcdef";
	std::string value = "X'";
	for (auto byte : bytes)
	{
		value += digits[byte >> 4];
		value += digits[byte & 15];
	}
	return value + "'";
}
std::string digest(std::span<const uint8_t> bytes)
{
	std::array<uint8_t, 32> value = {};
	SHA256(bytes.data(), bytes.size(), value.data());
	return binary(value);
}
uint64_t scalar(MYSQL *connection, const std::string &sql)
{
	execute(connection, sql);
	auto *result = mysql_store_result(connection);
	assert(result && mysql_num_rows(result) == 1);
	auto row = mysql_fetch_row(result);
	assert(row && row[0]);
	const auto value = strtoull(row[0], nullptr, 10);
	mysql_free_result(result);
	return value;
}
void pending(MYSQL *connection, const critical_command &command)
{
	std::vector<uint8_t> bytes, keys;
	assert(critical_command_encode(command, &bytes) == critical_command_codec_result::ok);
	for (const auto &key : command.keys)
	{
		keys.push_back(static_cast<uint8_t>(key.type));
		for (size_t index = 0; index < 8; ++index)
			keys.push_back(static_cast<uint8_t>(key.id >> (8 * index)));
	}
	execute(connection,
		"INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,command_type,schema_version,payload_version,status,result_payload) VALUES(" +
			literal(command.operation_id) + "," + digest(bytes) + "," + digest(keys) +
			",3,2,1,0,'')");
}
std::array<uint8_t, CURRENCY_RESULT_PAYLOAD_BYTES>
encoded_result(const economic_sql_bank_transaction &transaction)
{
	std::array<uint8_t, CURRENCY_RESULT_PAYLOAD_BYTES> value = {};
	assert(currency_command_encode_result(transaction.result(), &value));
	return value;
}
void outbox(MYSQL *connection, const critical_command &command,
	    const economic_sql_bank_transaction &transaction)
{
	const auto bytes = encoded_result(transaction);
	assert(critical_command_repository_insert_outbox_event(
		connection, command.operation_id, 0, 3, 1, 1, bytes.data(), bytes.size()));
}
void complete_fixture_root(MYSQL *connection, const critical_command &command,
			   economic_sql_bank_transaction &transaction)
{
	// Component-level fixture; root dispatcher journeys are exercised separately.
	execute(connection,
		"UPDATE critical_operation_inbox SET status=1,result_code=" +
			std::to_string(transaction.result_code()) + ",durable_revision=" +
			std::to_string(std::max(transaction.result().wallet_revision,
						transaction.result().bank_revision)) +
			",result_payload=" + binary(encoded_result(transaction)) +
			" WHERE operation_id=" + literal(command.operation_id) + " AND status=0");
	assert(mysql_affected_rows(connection) == 1);
	assert(transaction.verify_root_completion() == 0);
	assert(transaction.verify_root_completion() == EPERM);
}
}
int main()
{
	static_assert(!std::is_copy_constructible_v<economic_sql_bank_transaction>);
	assert(mysql_library_init(0, nullptr, nullptr) == 0);
	auto *connection = connect_fixture();
	auto *observer = connect_fixture();
	const auto lineage = new_id(), epoch = new_id(), bootstrap = new_id();
	char suffix[CRITICAL_COMMAND_ID_HEX_SIZE] = {};
	assert(critical_operation_id_to_hex(lineage, suffix, sizeof(suffix)));
	const auto name = std::string("Accounting") + std::string(suffix, 8);
	const auto account = std::string("accounting_") + std::string(suffix, 10);
	execute(connection,
		"INSERT INTO accounts(account_name,password) VALUES('" + account + "','')");
	execute(connection,
		"INSERT INTO player_data(name,account_name,racewar,copper,silver,gold,platinum) VALUES('" +
			name + "','" + account + "',1,1000,20,3,1)");
	const auto pid = mysql_insert_id(connection);
	assert(pid <= UINT32_MAX);
	execute(connection, "INSERT INTO account_banks(account_name,racewar,bank_copper) VALUES('" +
				    account + "',1,500)");
	const auto bank_id = mysql_insert_id(connection);
	execute(connection,
		"INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,command_type,schema_version,payload_version,status,result_payload) VALUES(" +
			literal(bootstrap) + ",REPEAT(CHAR(1),32),REPEAT(CHAR(2),32),1,1,1,1,'')");
	execute(connection,
		"INSERT INTO economic_epoch(lineage,epoch,ordinal,transition_kind,transition_digest,creating_operation_id) VALUES(" +
			literal(lineage) + "," + literal(epoch) + ",1,1,REPEAT(CHAR(1),32)," +
			literal(bootstrap) + ")");
	execute(connection, "INSERT INTO economic_lineage_state(lineage,active_epoch) VALUES(" +
				    literal(lineage) + "," + literal(epoch) + ")");
	execute(connection,
		"INSERT INTO economic_account_mapping(lineage,account_kind,context_id,backend_kind,locator_kind,native_id,active_native_id,creating_operation_id) VALUES(" +
			literal(lineage) + ",1,0,1,1," + std::to_string(pid) + "," +
			std::to_string(pid) + "," + literal(bootstrap) + ")");
	const economic_account_key wallet = { lineage, economic_account_kind::wallet,
					      mysql_insert_id(connection), 0 };
	execute(connection,
		"INSERT INTO economic_account_mapping(lineage,account_kind,context_id,backend_kind,locator_kind,native_id,active_native_id,creating_operation_id) VALUES(" +
			literal(lineage) + ",2,1,1,2," + std::to_string(bank_id) + "," +
			std::to_string(bank_id) + "," + literal(bootstrap) + ")");
	const economic_account_key bank = { lineage, economic_account_kind::bank,
					    mysql_insert_id(connection), 1 };
	const auto other_name = name + "B";
	execute(connection, "INSERT INTO player_data(name,account_name,racewar,copper) VALUES('" +
				    other_name + "','" + account + "',1,1000)");
	const auto other_pid = mysql_insert_id(connection);
	assert(other_pid <= UINT32_MAX);
	execute(connection,
		"INSERT INTO economic_account_mapping(lineage,account_kind,context_id,backend_kind,locator_kind,native_id,active_native_id,creating_operation_id) VALUES(" +
			literal(lineage) + ",1,0,1,1," + std::to_string(other_pid) + "," +
			std::to_string(other_pid) + "," + literal(bootstrap) + ")");
	const economic_account_key other_wallet = { lineage, economic_account_kind::wallet,
						    mysql_insert_id(connection), 0 };
	std::vector<critical_operation_id> operations;
	auto command_for = [&](int64_t amount, uint32_t payer = 0,
			       const economic_account_key *payer_wallet = nullptr,
			       uint64_t expected_bank = UINT64_MAX)
	{
		currency_command_payload payload = {};
		payload.pid = payer ? payer : static_cast<uint32_t>(pid);
		payload.racewar = 1;
		payload.reason = amount > 0 ? currency_reason_type::atm_deposit :
					      currency_reason_type::atm_withdraw;
		memcpy(payload.account_name.data(), account.data(), account.size());
		payload.wallet_delta.amount[0] = -amount;
		payload.bank_delta.amount[0] = amount;
		critical_command command;
		assert(currency_command_build(&command, new_id(), payload, UINT64_MAX,
					      expected_bank, critical_source_site::command,
					      critical_deadline_class::interactive));
		command.accepted_at_usec = 1;
		assert(economic_bank_transfer_intent(
			       command, epoch, payer_wallet ? *payer_wallet : wallet, bank,
			       &command.accounting_intent) == economic_accounting_error::ok);
		command.schema_version = 2;
		assert(economic_sql_bank_command_supported(command));
		operations.push_back(command.operation_id);
		return command;
	};
	auto start = [&](const critical_command &command)
	{
		execute(connection, "START TRANSACTION");
		pending(connection, command);
		std::unique_ptr<economic_sql_bank_transaction> transaction;
		const auto error =
			economic_sql_bank_transaction::prepare(connection, command, &transaction);
		if (error)
			fprintf(stderr, "typed prepare failed: %u\n", error);
		assert(!error && transaction);
		return transaction;
	};
	auto committed = [&](int64_t amount)
	{
		auto command = command_for(amount);
		auto transaction = start(command);
		assert(transaction->apply() == 0 && transaction->result_code() == 0);
		outbox(connection, command, *transaction);
		const auto error = transaction->finalize();
		if (error)
			fprintf(stderr, "typed finalize failed: %u\n", error);
		assert(error == 0);
		assert(transaction->finalize() == EPERM && transaction->apply() == EPERM);
		const auto where = " WHERE operation_id=" + literal(command.operation_id);
		assert(scalar(connection,
			      "SELECT COUNT(*) FROM economic_accounting_account_effect" + where) ==
		       2);
		assert(scalar(connection, "SELECT COUNT(*) FROM economic_accounting_coin_posting" +
						  where) == 2);
		assert(scalar(observer,
			      "SELECT COUNT(*) FROM economic_accounting_operation" + where) == 0);
		assert(connection->server_status & SERVER_STATUS_IN_TRANS);
		complete_fixture_root(connection, command, *transaction);
		const auto bytes = encoded_result(*transaction);
		execute(connection, "COMMIT");
		assert(economic_sql_bank_verify_retained(connection, command, 0, bytes) == 0);
		return std::make_pair(command, bytes);
	};
	const auto first = committed(100);
	const auto second = committed(-25);
	(void)second;
	const auto wallet_sql = "SELECT copper FROM player_data WHERE pid=" + std::to_string(pid);
	assert(scalar(connection, wallet_sql) == 925);
	assert(scalar(connection, "SELECT bank_copper FROM account_banks WHERE id=" +
					  std::to_string(bank_id)) == 575);

	// A normal policy rejection retains intent/outcome, never realized postings.
	const auto rejected = command_for(2000);
	auto rejection = start(rejected);
	assert(rejection->result_code() == ENOSPC && rejection->apply() == 0 &&
	       rejection->finalize() == 0);
	const auto rejected_bytes = encoded_result(*rejection);
	complete_fixture_root(connection, rejected, *rejection);
	execute(connection, "COMMIT");
	assert(economic_sql_bank_verify_retained(connection, rejected, ENOSPC, rejected_bytes) ==
	       0);
	assert(scalar(connection, "SELECT COUNT(*) FROM currency_ledger WHERE operation_id=" +
					  literal(rejected.operation_id)) == 0);

	// The actual root owns receipt, domain writes, accounting, outbox and commit.
	const auto root_command = command_for(10);
	const auto root_result = critical_command_repository_apply(connection, root_command);
	assert(root_result.outcome == critical_apply_outcome::applied);
	assert(!(connection->server_status & SERVER_STATUS_IN_TRANS));
	assert(critical_command_repository_apply(connection, root_command).outcome ==
	       critical_apply_outcome::already_applied);
	assert(critical_command_repository_reconcile(connection, root_command).outcome ==
	       critical_apply_outcome::already_applied);
	auto changed_root = root_command;
	++changed_root.accepted_at_usec;
	assert(critical_command_repository_apply(connection, changed_root).error_code == EEXIST);
	assert(critical_command_repository_reconcile(connection, changed_root).error_code ==
	       EEXIST);
	using reconnect_flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	reconnect_flag reconnect = true;
	assert(!mysql_options(connection, MYSQL_OPT_RECONNECT, &reconnect));
	assert(critical_command_repository_apply(connection, root_command).error_code == EPERM);
	assert(critical_command_repository_reconcile(connection, root_command).error_code == EPERM);
	reconnect = false;
	assert(!mysql_options(connection, MYSQL_OPT_RECONNECT, &reconnect));
	const auto root_rejection = command_for(2000);
	const auto root_rejected = critical_command_repository_apply(connection, root_rejection);
	assert(root_rejected.outcome == critical_apply_outcome::terminal_failure &&
	       root_rejected.error_code == ENOSPC);
	assert(critical_command_repository_apply(connection, root_rejection).error_code == ENOSPC);
	assert(critical_command_repository_reconcile(connection, root_rejection).error_code ==
	       ENOSPC);
	// Matching inbox/accounting headers must not authenticate a false decision.
	const auto stale_rejection = command_for(10, 0, nullptr, 0);
	const auto stale_result = critical_command_repository_apply(connection, stale_rejection);
	assert(stale_result.outcome == critical_apply_outcome::terminal_failure &&
	       stale_result.error_code == ESTALE);
	execute(connection, "UPDATE account_banks SET bank_copper=2147483647 WHERE id=" +
				    std::to_string(bank_id));
	const auto overflow_rejection = command_for(10);
	const auto overflow_result =
		critical_command_repository_apply(connection, overflow_rejection);
	assert(overflow_result.outcome == critical_apply_outcome::terminal_failure &&
	       overflow_result.error_code == ERANGE);
	execute(connection,
		"UPDATE account_banks SET bank_copper=585 WHERE id=" + std::to_string(bank_id));
	const std::vector<std::pair<critical_command, critical_apply_result>> retained_rejections = {
		{ root_rejection, root_rejected },
		{ stale_rejection, stale_result },
		{ overflow_rejection, overflow_result }
	};
	for (const auto &[command, original] : retained_rejections)
	{
		currency_command_result before;
		assert(currency_command_decode_result(original.result_payload.data(),
						      original.result_size, &before));
		const auto where = " WHERE operation_id=" + literal(command.operation_id);
		auto store_rejection =
			[&](unsigned int code, const currency_command_result &witness)
		{
			std::array<uint8_t, CURRENCY_RESULT_PAYLOAD_BYTES> bytes;
			assert(currency_command_encode_result(witness, &bytes));
			execute(connection,
				"UPDATE critical_operation_inbox SET result_code=" +
					std::to_string(code) + ",result_payload=" + binary(bytes) +
					",durable_revision=" +
					std::to_string(std::max(witness.wallet_revision,
								witness.bank_revision)) +
					where);
			execute(connection,
				"UPDATE economic_accounting_operation SET result_code=" +
					std::to_string(code) + where);
		};
		auto refuses = [&]
		{
			for (const auto &result :
			     { critical_command_repository_apply(connection, command),
			       critical_command_repository_reconcile(connection, command) })
				assert(result.outcome ==
					       critical_apply_outcome::retryable_failure &&
				       result.error_code == EILSEQ);
			assert(scalar(connection, "SELECT COUNT(*) FROM currency_ledger" + where) ==
			       0);
			assert(scalar(connection, "SELECT COUNT(*) FROM critical_outbox" + where) ==
			       0);
			assert(scalar(connection, wallet_sql) == 915);
		};
		for (unsigned int code : { unsigned(ENOSPC), unsigned(ESTALE), unsigned(ERANGE),
					   unsigned(EIO), unsigned(EILSEQ) })
		{
			if (code == original.error_code)
				continue;
			store_rejection(code, before);
			refuses();
		}
		// Bound all eight retained holdings before stale/funds/overflow checks.
		for (bool wallet_side : { false, true })
			for (size_t part = 0; part < 4; ++part)
				for (int64_t amount : { int64_t(-1), int64_t(INT32_MAX) + 1 })
				{
					auto forged = before;
					(wallet_side ? forged.wallet : forged.bank).amount[part] =
						amount;
					store_rejection(original.error_code, forged);
					refuses();
				}
		// This plausible alternative before-state would authorize success.
		auto sufficient = before;
		sufficient.wallet.amount[0] = 3000;
		sufficient.bank.amount[0] = 500;
		if (original.error_code == ESTALE)
			sufficient.bank_revision = 0;
		store_rejection(original.error_code, sufficient);
		refuses();
		store_rejection(original.error_code, before);
		assert(critical_command_repository_apply(connection, command).error_code ==
		       original.error_code);
		assert(critical_command_repository_reconcile(connection, command).error_code ==
		       original.error_code);
	}
	// Invalid native state reserves no permanent ID and remains retryable.
	const auto invalid_state_command = command_for(2000);
	execute(connection, "UPDATE account_banks SET bank_copper=2147483648 WHERE id=" +
				    std::to_string(bank_id));
	const auto invalid_state =
		critical_command_repository_apply(connection, invalid_state_command);
	assert(invalid_state.outcome == critical_apply_outcome::retryable_failure &&
	       invalid_state.error_code == EILSEQ);
	for (const char *table :
	     { "critical_operation_inbox", "economic_accounting_operation", "currency_ledger" })
		assert(scalar(connection, std::string("SELECT COUNT(*) FROM ") + table +
						  " WHERE operation_id=" +
						  literal(invalid_state_command.operation_id)) ==
		       0);
	execute(connection,
		"UPDATE account_banks SET bank_copper=585 WHERE id=" + std::to_string(bank_id));
	assert(critical_command_repository_apply(connection, invalid_state_command).error_code ==
	       ENOSPC);
	puts("SQL bank rejections: historical decisions, forged codes/witnesses and corrupt native state passed");
	assert(critical_command_repository_apply(connection, command_for(-10)).outcome ==
	       critical_apply_outcome::applied);
	assert(scalar(connection, wallet_sql) == 925);
	// Receipt completion triggers can corrupt any earlier result. Every variant
	// must be detected after UPDATE and leave no part of the root committed.
	const std::vector<std::string> completion_faults = {
		"SET NEW.failure_stage=1", // Valid coin stage is invalid for a bank receipt.
		"UPDATE player_data SET copper=copper+1 WHERE pid=" + std::to_string(pid),
		"UPDATE economic_account_mapping SET revision=revision+1 WHERE mapping_id=" +
			std::to_string(wallet.authority_id),
		"UPDATE economic_accounting_coin_posting SET delta_copper=delta_copper+1 WHERE operation_id=NEW.operation_id",
		"DELETE FROM critical_outbox WHERE operation_id=NEW.operation_id",
		"UPDATE critical_outbox SET status=1 WHERE operation_id=NEW.operation_id",
		"UPDATE critical_outbox SET next_attempt_at=CURRENT_TIMESTAMP(6)+INTERVAL 1 DAY WHERE operation_id=NEW.operation_id",
		"SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='synthetic finish failure'"
	};
	for (const auto &fault : completion_faults)
	{
		const auto command = command_for(10);
		execute(connection,
			"CREATE TRIGGER economic_bank_finish_fault " +
				std::string(fault.starts_with("SET NEW.") ? "BEFORE" : "AFTER") +
				" UPDATE ON critical_operation_inbox FOR EACH ROW " + fault);
		const auto result = critical_command_repository_apply(connection, command);
		assert(result.outcome == critical_apply_outcome::retryable_failure);
		execute(connection, "DROP TRIGGER economic_bank_finish_fault");
		assert(scalar(connection, wallet_sql) == 925);
		for (const char *table :
		     { "critical_operation_inbox", "critical_outbox", "currency_ledger",
		       "economic_accounting_operation", "economic_accounting_account_effect",
		       "economic_accounting_coin_posting" })
			assert(scalar(connection, std::string("SELECT COUNT(*) FROM ") + table +
							  " WHERE operation_id=" +
							  literal(command.operation_id)) == 0);
	}
	execute(connection, "START TRANSACTION");
	assert(critical_command_repository_apply(connection, command_for(10)).error_code == EBUSY);
	assert(critical_command_repository_reconcile(connection, root_command).error_code == EBUSY);
	assert(connection->server_status & SERVER_STATUS_IN_TRANS);
	execute(connection, "ROLLBACK");

	// A lock timeout must retain the original ID for a complete retry.
	const auto timed_out = command_for(10);
	execute(connection, "SET SESSION innodb_lock_wait_timeout=1");
	execute(observer, "START TRANSACTION");
	assert(scalar(observer, "SELECT bank_copper FROM account_banks WHERE id=" +
					std::to_string(bank_id) + " FOR UPDATE") == 575);
	const auto timeout_result = critical_command_repository_apply(connection, timed_out);
	assert(timeout_result.outcome == critical_apply_outcome::retryable_failure &&
	       timeout_result.error_code == 1205);
	execute(observer, "ROLLBACK");
	assert(critical_command_repository_apply(connection, timed_out).outcome ==
	       critical_apply_outcome::applied);
	assert(critical_command_repository_apply(connection, timed_out).outcome ==
	       critical_apply_outcome::already_applied);
	assert(critical_command_repository_apply(connection, command_for(-10)).outcome ==
	       critical_apply_outcome::applied);

	const auto ambiguous = command_for(10);
	lose_commit_reply = connection;
	const auto lost = critical_command_repository_apply(connection, ambiguous);
	assert(lost.outcome == critical_apply_outcome::ambiguous_commit && lost.error_code == 2013);
	assert(scalar(observer,
		      "SELECT COUNT(*) FROM economic_accounting_operation WHERE operation_id=" +
			      literal(ambiguous.operation_id)) == 1);
	mysql_close(connection);
	lost_commit_connection = nullptr;
	connection = connect_fixture();
	assert(critical_command_repository_reconcile(connection, ambiguous).outcome ==
	       critical_apply_outcome::already_applied);
	assert(critical_command_repository_apply(connection, ambiguous).outcome ==
	       critical_apply_outcome::already_applied);
	assert(critical_command_repository_apply(connection, command_for(-10)).outcome ==
	       critical_apply_outcome::applied);
	assert(scalar(connection, wallet_sql) == 925);

	// Distinct wallets sharing a bank serialize under the actual root locks.
	// Wildcard revisions both succeed; a fixed shared revision permits one root.
	for (bool fixed_revision : { false, true })
	{
		const auto revision =
			fixed_revision ?
				scalar(connection,
				       "SELECT bank_revision FROM account_banks WHERE id=" +
					       std::to_string(bank_id)) :
				UINT64_MAX;
		const std::array<critical_command, 2> commands = {
			command_for(10, 0, nullptr, revision),
			command_for(10, static_cast<uint32_t>(other_pid), &other_wallet, revision)
		};
		std::array<critical_apply_result, 2> results = {};
		std::barrier ready(3);
		auto run = [&](size_t index)
		{
			assert(mysql_thread_init() == 0);
			auto *worker = connect_fixture();
			ready.arrive_and_wait();
			results[index] = critical_command_repository_apply(worker, commands[index]);
			mysql_close(worker);
			mysql_thread_end();
		};
		std::thread first_worker(run, 0), second_worker(run, 1);
		ready.arrive_and_wait();
		first_worker.join();
		second_worker.join();
		size_t successful = 0;
		for (size_t index = 0; index < results.size(); ++index)
		{
			if (results[index].outcome == critical_apply_outcome::applied)
			{
				++successful;
				assert(critical_command_repository_apply(connection,
									 commands[index])
					       .outcome == critical_apply_outcome::already_applied);
				const auto refund =
					index ? command_for(-10, static_cast<uint32_t>(other_pid),
							    &other_wallet) :
						command_for(-10);
				assert(critical_command_repository_apply(connection, refund)
					       .outcome == critical_apply_outcome::applied);
			}
			else
			{
				assert(fixed_revision &&
				       results[index].outcome ==
					       critical_apply_outcome::terminal_failure &&
				       results[index].error_code == ESTALE);
				assert(critical_command_repository_reconcile(connection,
									     commands[index])
					       .error_code == ESTALE);
			}
		}
		assert(successful == (fixed_revision ? 1 : 2));
		assert(scalar(connection, wallet_sql) == 925);
		assert(scalar(connection, "SELECT copper FROM player_data WHERE pid=" +
						  std::to_string(other_pid)) == 1000);
		assert(scalar(connection, "SELECT bank_copper FROM account_banks WHERE id=" +
						  std::to_string(bank_id)) == 575);
	}

	// Faults in each domain/evidence write must remain wholly uncommitted.
	const std::vector<std::pair<std::string, std::string>> faults = {
		{ "player_data", "AFTER UPDATE" },
		{ "account_banks", "AFTER UPDATE" },
		{ "currency_ledger", "AFTER INSERT" },
		{ "economic_accounting_operation", "AFTER INSERT" },
		{ "economic_accounting_account_effect", "AFTER INSERT" },
		{ "economic_accounting_coin_posting", "AFTER INSERT" }
	};
	for (const auto &[table, timing] : faults)
	{
		execute(connection,
			"CREATE TRIGGER economic_bank_fault " + timing + " ON " + table +
				" FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='synthetic accounting fault'");
		const auto command = command_for(10);
		auto transaction = start(command);
		auto error = transaction->apply();
		if (!error)
		{
			outbox(connection, command, *transaction);
			error = transaction->finalize();
		}
		assert(error != 0 && transaction->apply() == EPERM &&
		       transaction->finalize() == EPERM);
		execute(connection, "ROLLBACK");
		execute(connection, "DROP TRIGGER economic_bank_fault");
		const auto where = " WHERE operation_id=" + literal(command.operation_id);
		for (const char *table_name :
		     { "critical_operation_inbox", "currency_ledger",
		       "economic_accounting_operation", "economic_accounting_account_effect",
		       "economic_accounting_coin_posting" })
			assert(scalar(observer, std::string("SELECT COUNT(*) FROM ") + table_name +
							where) == 0);
		assert(scalar(connection, wallet_sql) == 925);
	}
	// Fail on the second normalized row after the operation and earlier rows
	// already exist. The prepared outbox must roll back with the financial rows.
	for (const auto &[table, index] : std::vector<std::pair<std::string, std::string>>{
		     { "economic_accounting_account_effect", "account_index" },
		     { "economic_accounting_coin_posting", "line_index" } })
	{
		execute(connection,
			"CREATE TRIGGER economic_bank_fault AFTER INSERT ON " + table +
				" FOR EACH ROW BEGIN IF NEW." + index +
				"=1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='synthetic later row fault'; END IF; END");
		const auto command = command_for(10);
		auto transaction = start(command);
		assert(transaction->apply() == 0);
		outbox(connection, command, *transaction);
		assert(transaction->finalize() != 0);
		execute(connection, "ROLLBACK");
		execute(connection, "DROP TRIGGER economic_bank_fault");
		const auto where = " WHERE operation_id=" + literal(command.operation_id);
		for (const char *table_name :
		     { "critical_operation_inbox", "critical_outbox", "currency_ledger",
		       "economic_accounting_operation", "economic_accounting_account_effect",
		       "economic_accounting_coin_posting" })
			assert(scalar(observer, std::string("SELECT COUNT(*) FROM ") + table_name +
							where) == 0);
		assert(scalar(connection, wallet_sql) == 925);
	}

	// Successful SQL that writes a wrong normalized field is also rejected.
	execute(connection,
		"CREATE TRIGGER economic_bank_fault BEFORE INSERT ON economic_accounting_coin_posting FOR EACH ROW SET NEW.delta_copper=NEW.delta_copper+1");
	auto malformed = start(command_for(10));
	assert(malformed->apply() == 0 && malformed->finalize() != 0);
	execute(connection, "ROLLBACK");
	execute(connection, "DROP TRIGGER economic_bank_fault");

	for (const auto &mutation :
	     { "UPDATE economic_account_mapping SET revision=revision+1 WHERE mapping_id=" +
		       std::to_string(bank.authority_id),
	       std::string(
		       "UPDATE critical_operation_inbox SET command_hash=REPEAT(CHAR(9),32) WHERE operation_id=NEW.operation_id") })
	{
		execute(connection,
			"CREATE TRIGGER economic_bank_fault AFTER INSERT ON economic_accounting_coin_posting FOR EACH ROW " +
				mutation);
		auto changed_during_append = start(command_for(10));
		assert(changed_during_append->apply() == 0 &&
		       changed_during_append->finalize() != 0);
		execute(connection, "ROLLBACK");
		execute(connection, "DROP TRIGGER economic_bank_fault");
		assert(scalar(connection, wallet_sql) == 925);
	}

	for (unsigned int damage = 0; damage < 5; ++damage)
	{
		const auto command = command_for(10);
		auto transaction = start(command);
		assert(transaction->apply() == 0);
		const auto where = " WHERE operation_id=" + literal(command.operation_id);
		if (damage == 0)
			execute(connection, "UPDATE player_data SET copper=copper+1 WHERE pid=" +
						    std::to_string(pid));
		if (damage == 1)
			execute(connection, "DELETE FROM currency_ledger" + where);
		if (damage == 2)
			execute(connection,
				"UPDATE currency_ledger SET reason_id=reason_id+1" + where);
		if (damage == 3)
			execute(connection,
				"UPDATE account_banks SET bank_revision=bank_revision+1 WHERE id=" +
					std::to_string(bank_id));
		if (damage == 4)
			execute(connection,
				"UPDATE economic_account_mapping SET revision=revision+1 WHERE mapping_id=" +
					std::to_string(bank.authority_id));
		assert(transaction->finalize() != 0 && transaction->finalize() == EPERM);
		execute(connection, "ROLLBACK");
		assert(scalar(connection, wallet_sql) == 925);
	}

	// Parent savepoint rollback and rollback/restart both invalidate the receipt.
	auto rollback_command = command_for(10);
	execute(connection, "START TRANSACTION");
	pending(connection, rollback_command);
	execute(connection, "SAVEPOINT parent_scope");
	std::unique_ptr<economic_sql_bank_transaction> rolled;
	assert(economic_sql_bank_transaction::prepare(connection, rollback_command, &rolled) == 0 &&
	       rolled->apply() == 0);
	execute(connection, "ROLLBACK TO SAVEPOINT parent_scope");
	assert(rolled->finalize() == 1305);
	execute(connection, "ROLLBACK");
	rollback_command = command_for(10);
	rolled = start(rollback_command);
	assert(rolled->apply() == 0);
	execute(connection, "ROLLBACK");
	execute(connection, "START TRANSACTION");
	pending(connection, rollback_command);
	assert(rolled->finalize() == 1305);
	execute(connection, "ROLLBACK");

	// Verification uses retained evidence after later writes and retirement.
	execute(connection, "START TRANSACTION");
	execute(connection,
		"UPDATE economic_lineage_state SET active_epoch=NULL,revision=revision+1 WHERE lineage=" +
			literal(lineage));
	execute(connection,
		"UPDATE economic_account_mapping SET active_native_id=NULL,retiring_operation_id=" +
			literal(bootstrap) +
			",revision=revision+1 WHERE lineage=" + literal(lineage));
	assert(economic_sql_bank_verify_retained(connection, first.first, 0, first.second) == 0);
	auto changed = first.first;
	++changed.accepted_at_usec;
	assert(economic_sql_bank_verify_retained(connection, changed, 0, first.second) != 0);
	execute(connection, "SAVEPOINT retained_check");
	execute(connection,
		"UPDATE critical_operation_inbox SET durable_revision=durable_revision+1 WHERE operation_id=" +
			literal(first.first.operation_id));
	assert(economic_sql_bank_verify_retained(connection, first.first, 0, first.second) != 0);
	execute(connection, "ROLLBACK TO SAVEPOINT retained_check");
	execute(connection,
		"UPDATE economic_account_mapping SET native_id=native_id+10000 WHERE mapping_id=" +
			std::to_string(wallet.authority_id));
	assert(economic_sql_bank_verify_retained(connection, first.first, 0, first.second) != 0);
	assert(economic_sql_bank_verify_retained(connection, rejected, ENOSPC, rejected_bytes) ==
	       EILSEQ);
	execute(connection, "ROLLBACK TO SAVEPOINT retained_check");
	execute(connection,
		"UPDATE economic_accounting_coin_posting SET copper_value=copper_value+1 WHERE operation_id=" +
			literal(first.first.operation_id) + " AND line_index=0");
	assert(economic_sql_bank_verify_retained(connection, first.first, 0, first.second) != 0);
	execute(connection, "ROLLBACK");

	// Root replay uses retained evidence even after authorities retire and a
	// publication has been consumed. Corruption remains unresolved/retryable.
	execute(connection,
		"UPDATE economic_lineage_state SET active_epoch=NULL,revision=revision+1 WHERE lineage=" +
			literal(lineage));
	execute(connection,
		"UPDATE economic_account_mapping SET active_native_id=NULL,retiring_operation_id=" +
			literal(bootstrap) +
			",revision=revision+1 WHERE lineage=" + literal(lineage));
	execute(connection, "DELETE FROM critical_outbox WHERE operation_id=" +
				    literal(root_command.operation_id));
	assert(critical_command_repository_apply(connection, root_command).outcome ==
	       critical_apply_outcome::already_applied);
	assert(critical_command_repository_reconcile(connection, root_command).outcome ==
	       critical_apply_outcome::already_applied);
	assert(critical_command_repository_reconcile(connection, root_rejection).error_code ==
	       ENOSPC);
	for (const auto &[command, original] : retained_rejections)
	{
		for (const auto &result :
		     { critical_command_repository_apply(connection, command),
		       critical_command_repository_reconcile(connection, command) })
			assert(result.outcome == critical_apply_outcome::terminal_failure &&
			       result.error_code == original.error_code &&
			       result.durable_revision == original.durable_revision &&
			       result.result_size == original.result_size &&
			       result.result_payload == original.result_payload);
	}

	// A structurally valid coin stage cannot authenticate a retained bank receipt.
	for (const auto &command : { root_command, root_rejection })
	{
		const auto where = " WHERE operation_id=" + literal(command.operation_id);
		execute(connection, "UPDATE critical_operation_inbox SET failure_stage=1" + where);
		for (const auto &result :
		     { critical_command_repository_apply(connection, command),
		       critical_command_repository_reconcile(connection, command) })
			assert(result.outcome == critical_apply_outcome::retryable_failure);
		execute(connection, "UPDATE critical_operation_inbox SET failure_stage=0" + where);
	}
	assert(critical_command_repository_apply(connection, root_command).outcome ==
	       critical_apply_outcome::already_applied);
	assert(critical_command_repository_reconcile(connection, root_rejection).error_code ==
	       ENOSPC);
	puts("SQL bank failure stages: completion rollback and retained tamper checks passed");

	execute(connection,
		"UPDATE economic_accounting_coin_posting SET copper_value=copper_value+1 WHERE operation_id=" +
			literal(root_command.operation_id) + " AND line_index=0");
	assert(critical_command_repository_apply(connection, root_command).outcome ==
	       critical_apply_outcome::retryable_failure);
	assert(critical_command_repository_reconcile(connection, root_command).outcome ==
	       critical_apply_outcome::retryable_failure);
	assert(scalar(connection, wallet_sql) == 925);

	for (const auto &operation : operations)
	{
		const auto where = " WHERE operation_id=" + literal(operation);
		for (const char *table :
		     { "economic_accounting_coin_posting", "economic_accounting_account_effect",
		       "economic_accounting_operation", "critical_outbox", "currency_ledger",
		       "critical_operation_inbox" })
			execute(connection, std::string("DELETE FROM ") + table + where);
	}
	execute(connection,
		"DELETE FROM currency_wallet_baseline WHERE pid=" + std::to_string(pid));
	execute(connection,
		"DELETE FROM currency_bank_baseline WHERE bank_id=" + std::to_string(bank_id));
	execute(connection,
		"DELETE FROM economic_account_mapping WHERE lineage=" + literal(lineage));
	execute(connection, "DELETE FROM economic_lineage_state WHERE lineage=" + literal(lineage));
	execute(connection, "DELETE FROM economic_epoch WHERE lineage=" + literal(lineage));
	execute(connection,
		"DELETE FROM critical_operation_inbox WHERE operation_id=" + literal(bootstrap));
	execute(connection,
		"DELETE FROM currency_wallet_baseline WHERE pid=" + std::to_string(other_pid));
	execute(connection, "DELETE FROM player_data WHERE pid=" + std::to_string(other_pid));
	execute(connection, "DELETE FROM player_data WHERE pid=" + std::to_string(pid));
	execute(connection, "DELETE FROM account_banks WHERE id=" + std::to_string(bank_id));
	execute(connection, "DELETE FROM accounts WHERE account_name='" + account + "'");
	mysql_close(observer);
	mysql_close(connection);
	mysql_library_end();
	puts("SQL typed bank transaction: writes, evidence, rejection, rollback faults and retained verification passed");
}

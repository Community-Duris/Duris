#include "core/prototypes.h"
#include "core/utils.h"
#include "economy/account_bank_balances.h"
#include "economy/currency_transaction.h"
#include "economy/economic_currency_adapter.h"
#include "economy/economic_gameplay_authority.h"
#include "persistence/critical_command_repository.h"
#include "persistence/economic_sql_bank_transaction.h"
#include "sql/sql_player.h"

#include <mysql.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <span>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

// This component owns one synthetic online player, exposed through the lookup
// below. There are no additional descriptors or linkdead characters.
P_desc descriptor_list = nullptr;
P_char character_list = nullptr;

class economic_gameplay_authority_test_access
{
    public:
	static economic_accounting_error
	install(const critical_operation_id &lineage, const critical_operation_id &epoch,
		const critical_operation_id &receipt,
		std::span<const economic_gameplay_wallet_mapping> wallets,
		std::span<const economic_gameplay_bank_mapping> banks)
	{
		return economic_gameplay_authority::install(lineage, epoch, receipt, wallets,
							    banks);
	}
};

namespace
{
constexpr const char *ACCOUNT = "s06_atm_account";
constexpr int64_t INITIAL_WALLET = 100;
constexpr int64_t DEPOSIT = 10;

MYSQL *observer = nullptr;
P_char online_character = nullptr;
pc_only_data pc = {};
char_data character = {};
int bank_publications = 0;
AccountBankBalances published_bank = {};
uint64_t published_bank_revision = 0;
int replayed_commands = 0;

const char *required(const char *name)
{
	const char *value = std::getenv(name);
	assert(value && *value);
	return value;
}

MYSQL *connect_fixture()
{
	assert(std::strcmp(required("ATM_PUBLICATION_DISPOSABLE_SCHEMA"), "1") == 0);
	assert(std::strcmp(required("DB_HOST"), "127.0.0.1") == 0);
	assert(!std::getenv("DB_SOCKET") || !*std::getenv("DB_SOCKET"));
	const std::string schema = required("DB_NAME");
	assert(schema.starts_with("atm_pub_test_") &&
	       schema.find_first_not_of(
		       "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") ==
		       std::string::npos);
	char *end = nullptr;
	const auto port = std::strtoul(required("DB_PORT"), &end, 10);
	assert(end && !*end && port > 0 && port <= 65535);
	auto *connection = mysql_init(nullptr);
	assert(connection);
	unsigned int timeout = 5, protocol = MYSQL_PROTOCOL_TCP;
	assert(!mysql_options(connection, MYSQL_OPT_CONNECT_TIMEOUT, &timeout));
	assert(!mysql_options(connection, MYSQL_OPT_READ_TIMEOUT, &timeout));
	assert(!mysql_options(connection, MYSQL_OPT_WRITE_TIMEOUT, &timeout));
	assert(!mysql_options(connection, MYSQL_OPT_PROTOCOL, &protocol));
	using reconnect_flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	reconnect_flag reconnect = false;
	assert(!mysql_options(connection, MYSQL_OPT_RECONNECT, &reconnect));
	assert(mysql_real_connect(connection, "127.0.0.1", required("DB_USER"),
				  required("DB_PASSWD"), schema.c_str(),
				  static_cast<unsigned int>(port), nullptr, 0));
	return connection;
}

void execute(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()))
	{
		std::fprintf(stderr, "ATM SQL fixture statement failed (mysql error %u)\n",
			     mysql_errno(connection));
		std::abort();
	}
}

uint64_t scalar(const std::string &sql)
{
	execute(observer, sql);
	auto *result = mysql_store_result(observer);
	assert(result && mysql_num_rows(result) == 1 && mysql_num_fields(result) == 1);
	auto row = mysql_fetch_row(result);
	assert(row && row[0]);
	const uint64_t value = std::strtoull(row[0], nullptr, 10);
	mysql_free_result(result);
	return value;
}

std::string scalar_text(const std::string &sql)
{
	execute(observer, sql);
	auto *result = mysql_store_result(observer);
	assert(result && mysql_num_rows(result) == 1 && mysql_num_fields(result) == 1);
	auto row = mysql_fetch_row(result);
	assert(row && row[0]);
	const auto *lengths = mysql_fetch_lengths(result);
	assert(lengths);
	std::string value(row[0], lengths[0]);
	mysql_free_result(result);
	return value;
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

std::string hex_bytes(const uint8_t *data, size_t size)
{
	static constexpr char digits[] = "0123456789ABCDEF";
	std::string value;
	value.reserve(size * 2);
	for (size_t index = 0; index < size; ++index)
	{
		value += digits[data[index] >> 4];
		value += digits[data[index] & 15];
	}
	return value;
}

bool restore_replayed(const critical_command &command, void *)
{
	++replayed_commands;
	return currency_transaction_restore_replayed_command(command);
}

critical_apply_result apply_sql(const critical_command &command, void *)
{
	if (mysql_thread_init() != 0)
		return { critical_apply_outcome::retryable_failure, 0, EIO };
	MYSQL *connection = connect_fixture();
	const auto result = critical_command_repository_apply(connection, command);
	mysql_close(connection);
	mysql_thread_end();
	return result;
}

bool validate_bank_command(const critical_command &command) noexcept
{
	return economic_sql_bank_command_supported(command);
}

} // namespace

P_char find_player_by_pid(int pid)
{
	return online_character && GET_PID(online_character) == pid ? online_character : nullptr;
}

const char *get_account_name_safe(P_char)
{
	return ACCOUNT;
}

void publish_account_bank_balances_revision(const char *account_name, int racewar,
					    const AccountBankBalances *balances, uint64_t revision)
{
	assert(account_name && std::strcmp(account_name, ACCOUNT) == 0);
	assert(racewar == 1 && balances);
	published_bank = *balances;
	published_bank_revision = revision;
	++bank_publications;
}

void gmcp_char_vitals(P_char) {}
void send_to_char(const char *, P_char) {}
void logit(const char *, const char *, ...) {}
void persistence_alert(int, const char *, const char *, const char *, const char *, const char *,
		       const char *, ...)
{
}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	std::abort();
}

namespace
{
void seed_sql_authority(critical_operation_id *lineage, critical_operation_id *epoch,
			critical_operation_id *activation, uint32_t *pid, uint64_t *wallet_mapping,
			uint64_t *bank_mapping, uint64_t *bank_id)
{
	execute(observer,
		"INSERT INTO accounts(account_name,password) VALUES('s06_atm_account','')");
	execute(observer,
		"INSERT INTO player_data(name,account_name,racewar,copper,silver,gold,platinum) "
		"VALUES('S06ATM','s06_atm_account',1,100,0,0,0)");
	*pid = static_cast<uint32_t>(mysql_insert_id(observer));
	execute(observer, "INSERT INTO account_banks(account_name,racewar,bank_copper) "
			  "VALUES('s06_atm_account',1,0)");
	*bank_id = mysql_insert_id(observer);
	execute(observer, "INSERT INTO currency_wallet_baseline(pid,opening_copper,opening_silver,"
			  "opening_gold,opening_platinum,opening_revision) VALUES(" +
				  std::to_string(*pid) + ",100,0,0,0,0)");
	execute(observer,
		"INSERT INTO currency_bank_baseline(bank_id,opening_copper,opening_silver,"
		"opening_gold,opening_platinum,opening_revision) VALUES(" +
			std::to_string(*bank_id) + ",0,0,0,0,0)");

	*lineage = new_id();
	*epoch = new_id();
	*activation = new_id();
	execute(observer,
		"INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
		"command_type,schema_version,payload_version,status,result_payload) VALUES(" +
			literal(*activation) +
			",REPEAT(CHAR(1),32),REPEAT(CHAR(2),32),1,1,1,1,'')");
	execute(observer, "INSERT INTO economic_epoch(lineage,epoch,ordinal,transition_kind,"
			  "transition_digest,creating_operation_id) VALUES(" +
				  literal(*lineage) + "," + literal(*epoch) +
				  ",1,1,REPEAT(CHAR(3),32)," + literal(*activation) + ")");
	execute(observer,
		"INSERT INTO economic_lineage_state(lineage,active_epoch,revision) VALUES(" +
			literal(*lineage) + "," + literal(*epoch) + ",1)");
	execute(observer,
		"INSERT INTO economic_account_mapping(lineage,account_kind,context_id,backend_kind,"
		"locator_kind,native_id,active_native_id,creating_operation_id) VALUES(" +
			literal(*lineage) + ",1,0,1,1," + std::to_string(*pid) + "," +
			std::to_string(*pid) + "," + literal(*activation) + ")");
	*wallet_mapping = mysql_insert_id(observer);
	execute(observer,
		"INSERT INTO economic_account_mapping(lineage,account_kind,context_id,backend_kind,"
		"locator_kind,native_id,active_native_id,creating_operation_id) VALUES(" +
			literal(*lineage) + ",2,1,1,2," + std::to_string(*bank_id) + "," +
			std::to_string(*bank_id) + "," + literal(*activation) + ")");
	*bank_mapping = mysql_insert_id(observer);

	const std::array wallets = { economic_gameplay_wallet_mapping{
		*pid, { *lineage, economic_account_kind::wallet, *wallet_mapping, 0 } } };
	const std::array banks = { economic_gameplay_bank_mapping{
		ACCOUNT, 1, { *lineage, economic_account_kind::bank, *bank_mapping, 1 } } };
	assert(economic_gameplay_authority_test_access::install(*lineage, *epoch, *activation,
								wallets, banks) ==
	       economic_accounting_error::ok);
}

void initialize_character(uint32_t pid, uint64_t wallet_revision, int64_t wallet)
{
	pc = {};
	pc.pid = pid;
	pc.wallet_revision = wallet_revision;
	pc.bank_revision = 0;
	character = {};
	character.only.pc = &pc;
	character.player.racewar = 1;
	GET_COPPER(&character) = static_cast<int>(wallet);
}

bool wait_until(const std::function<bool()> &predicate, unsigned int timeout_ms)
{
	const auto deadline =
		std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
	while (std::chrono::steady_clock::now() < deadline)
	{
		if (predicate())
			return true;
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}
	return predicate();
}

critical_completion await_one_completion(const critical_operation_id &operation_id)
{
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
	while (std::chrono::steady_clock::now() < deadline)
	{
		critical_completion completions[8] = {};
		const size_t count = critical_command_coordinator_pulse(completions, 8);
		if (count)
		{
			assert(count == 1);
			assert(critical_operation_id_equal(completions[0].operation_id,
							   operation_id));
			return completions[0];
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}
	assert(false && "coordinator did not deliver the SQL completion");
	return {};
}

void assert_single_committed_operation(const critical_operation_id &operation_id,
				       const std::string &expected_payload)
{
	const std::string where = "operation_id=" + literal(operation_id);
	assert(scalar("SELECT COUNT(*) FROM critical_operation_inbox WHERE " + where +
		      " AND status=1 AND result_code=0") == 1);
	assert(scalar("SELECT COUNT(*) FROM currency_ledger WHERE " + where) == 1);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE " + where +
		      " AND destination=3 AND event_type=1 AND payload_version=1 AND status=0") ==
	       1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_operation WHERE " + where +
		      " AND outcome=1 AND result_code=0") == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_account_effect WHERE " + where) ==
	       2);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_coin_posting WHERE " + where) == 2);
	assert(scalar_text("SELECT HEX(result_payload) FROM critical_operation_inbox WHERE " +
			   where) == expected_payload);
}
} // namespace

int main()
{
	assert(mysql_library_init(0, nullptr, nullptr) == 0);
	observer = connect_fixture();
	critical_operation_id lineage = {}, epoch = {}, activation = {};
	uint32_t pid = 0;
	uint64_t wallet_mapping = 0, bank_mapping = 0, bank_id = 0;
	seed_sql_authority(&lineage, &epoch, &activation, &pid, &wallet_mapping, &bank_mapping,
			   &bank_id);
	initialize_character(pid, 0, INITIAL_WALLET);
	online_character = &character;

	const std::filesystem::path journal_parent = required("ATM_TEST_JOURNAL_PARENT");
	assert(journal_parent.is_absolute());
	assert(journal_parent.filename().string().starts_with("run-"));
	assert(journal_parent.parent_path() ==
	       std::filesystem::current_path() / "bin/tests/atm-publication-sql");
	const std::filesystem::path journal_path = journal_parent / "journal";
	assert(std::filesystem::create_directory(journal_path));
	assert(chmod(journal_path.c_str(), 0700) == 0);
	const std::string journal_directory = journal_path.string();

	assert(critical_command_coordinator_init(journal_directory.c_str(), apply_sql, nullptr, 1,
						 nullptr, nullptr, validate_bank_command));
	critical_operation_id operation_id = new_id();
	const currency_vector wallet_delta = { { -DEPOSIT, 0, 0, 0 } };
	const currency_vector bank_delta = { { DEPOSIT, 0, 0, 0 } };
	assert(currency_transaction_submit_identified(
		&character, operation_id, wallet_delta, bank_delta,
		currency_reason_type::atm_deposit, 601, critical_source_site::command,
		critical_deadline_class::interactive, nullptr, nullptr, 0));
	assert(critical_command_coordinator_durability(operation_id) ==
		       critical_command_durability::awaiting_durability ||
	       critical_command_coordinator_durability(operation_id) ==
		       critical_command_durability::durable);
	assert(wait_until(
		[&]
		{
			return critical_command_coordinator_durability(operation_id) ==
			       critical_command_durability::durable;
		},
		10000));

	// The SQL transaction must commit while the game-thread publication ACK is
	// deliberately withheld. The live actor and bank projection remain unchanged.
	const std::string op_where = "operation_id=" + literal(operation_id);
	assert(wait_until(
		[&]
		{
			return scalar("SELECT COUNT(*) FROM critical_operation_inbox WHERE " +
				      op_where + " AND status=1 AND result_code=0") == 1;
		},
		20000));
	assert(scalar("SELECT copper FROM player_data WHERE pid=" + std::to_string(pid)) ==
	       INITIAL_WALLET - DEPOSIT);
	assert(scalar("SELECT bank_copper FROM account_banks WHERE id=" +
		      std::to_string(bank_id)) == DEPOSIT);
	const std::string committed_payload = scalar_text(
		"SELECT HEX(result_payload) FROM critical_operation_inbox WHERE " + op_where);
	assert_single_committed_operation(operation_id, committed_payload);
	assert(GET_COPPER(&character) == INITIAL_WALLET && pc.wallet_revision == 0);
	assert(bank_publications == 0);
	critical_completion delayed = await_one_completion(operation_id);
	assert(delayed.outcome == critical_apply_outcome::applied && delayed.error_code == 0);
	assert(delayed.result_size == CURRENCY_RESULT_PAYLOAD_BYTES);
	assert(hex_bytes(delayed.result_payload.data(), delayed.result_size) == committed_payload);
	critical_operation_id fenced = {};
	const critical_entity_key player_key = { critical_entity_type::player, pid };
	assert(critical_command_coordinator_is_fenced(player_key, &fenced));
	assert(critical_operation_id_equal(fenced, operation_id));
	const auto held = critical_command_coordinator_health_copy();
	assert(held.publication_pending == 1 && held.fenced_keys == 2);
	assert(critical_command_journal_health_copy().records == 1);
	assert(critical_command_journal_health_copy().checkpoints == 0);

	// Drop the first in-memory ACK exactly as a process loss would: no publication
	// callback and no journal checkpoint has occurred. Restart from the same local
	// journal and let its exact command ID re-enter the real SQL owner.
	critical_command_coordinator_shutdown();
	currency_transaction_reset_for_tests();
	online_character = nullptr;
	assert(critical_command_coordinator_init(journal_directory.c_str(), apply_sql, nullptr, 1,
						 restore_replayed, nullptr, validate_bank_command));
	assert(replayed_commands == 1);
	const auto replay_journal = critical_command_journal_health_copy();
	assert(replay_journal.replays == 1 && replay_journal.records == 1);
	const critical_completion replayed = await_one_completion(operation_id);
	assert(replayed.outcome == critical_apply_outcome::already_applied);
	assert(replayed.error_code == 0 && replayed.result_size == delayed.result_size);
	assert(replayed.result_payload == delayed.result_payload);
	assert_single_committed_operation(operation_id, committed_payload);
	currency_transaction_handle_completions(&replayed, 1);
	const auto offline = currency_transaction_health_copy();
	assert(offline.pending == 1 && offline.retained_offline == 1);
	assert(offline.committed == 0 && offline.publication_blocked == 0);
	assert(bank_publications == 0);

	// Reconnect with the committed wallet loaded from SQL. The retained receipt
	// publishes its exact bank state and only then checkpoints/releases the fence.
	const auto wallet =
		scalar("SELECT copper FROM player_data WHERE pid=" + std::to_string(pid));
	const auto wallet_revision =
		scalar("SELECT wallet_revision FROM player_data WHERE pid=" + std::to_string(pid));
	const auto bank_revision = scalar("SELECT bank_revision FROM account_banks WHERE id=" +
					  std::to_string(bank_id));
	initialize_character(pid, wallet_revision, static_cast<int64_t>(wallet));
	pc.bank_revision = bank_revision;
	online_character = &character;
	currency_transaction_player_ready(&character);
	assert(GET_COPPER(&character) == INITIAL_WALLET - DEPOSIT);
	assert(pc.wallet_revision == 1 && pc.bank_revision == 1);
	assert(bank_publications == 1 && published_bank_revision == 1);
	assert(published_bank.copper == DEPOSIT && published_bank.silver == 0 &&
	       published_bank.gold == 0 && published_bank.platinum == 0);
	assert(currency_transaction_health_copy().pending == 0);
	assert(currency_transaction_health_copy().committed == 1);
	assert(critical_command_coordinator_health_copy().publication_pending == 0);
	assert(critical_command_coordinator_health_copy().fenced_keys == 0);
	assert(critical_command_journal_health_copy().records == 0);
	assert(critical_command_journal_health_copy().checkpoints == 1);
	assert_single_committed_operation(operation_id, committed_payload);

	critical_command_coordinator_shutdown();
	currency_transaction_reset_for_tests();
	mysql_close(observer);
	observer = nullptr;
	assert(std::filesystem::remove_all(journal_directory) > 0);
	assert(!std::filesystem::exists(journal_directory));
	mysql_library_end();
	char operation_hex[CRITICAL_COMMAND_ID_HEX_SIZE] = {};
	assert(critical_operation_id_to_hex(operation_id, operation_hex, sizeof(operation_hex)));
	std::printf(
		"ATM SQL component: commit=applied replay=already_applied wallet=%lld bank=%lld "
		"revisions=1/1 ledger=1 outbox=1 offline=retained reconnect=published "
		"journal_replays=1 checkpoints=1 operation=%s\n",
		static_cast<long long>(INITIAL_WALLET - DEPOSIT), static_cast<long long>(DEPOSIT),
		operation_hex);
	return 0;
}

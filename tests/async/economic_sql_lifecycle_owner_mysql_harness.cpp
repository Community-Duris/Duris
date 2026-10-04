#include "economy/economic_baseline_adapter.h"
#include "economy/economic_gameplay_authority.h"
#include "economy/economic_sql_source_normalize.h"
#include "core/defines.h"
#include "persistence/economic_sql_accounting_lifecycle_transaction.h"
#include "persistence/economic_sql_lifecycle_guard.h"
#include "persistence/economic_sql_source_snapshot.h"
#include "persistence/critical_command_coordinator.h"
#include "player/player_snapshot_codec.h"
#include "world/vnum.obj.h"
#include <mysql/mysql.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

class economic_gameplay_authority_test_access
{
    public:
	static void reset() { economic_gameplay_authority::reset_for_tests(); }
};

namespace
{
bool stopped_drop_setup = false;
critical_apply_result no_gameplay_apply(const critical_command &, void *)
{
	return { critical_apply_outcome::retryable_failure, 0, EIO };
}

unsigned int verify_synthetic_routes(MYSQL *, const economic_sql_activation_evidence &evidence,
				     const economic_sql_source_snapshot &snapshot) noexcept
{
	if (evidence.route_count != 3 || evidence.verified_route_count != 3 ||
	    evidence.unclassified_route_count || snapshot.tables.empty())
		return ENODATA;
	return 0;
}
const char *required(const char *name)
{
	const char *value = std::getenv(name);
	if (!value || !*value)
		throw std::runtime_error(std::string("missing ") + name);
	return value;
}

MYSQL *connect_fixture()
{
	const std::string host = required("DB_HOST");
	const std::string schema = required("DB_NAME");
	if ((host != "127.0.0.1" && host != "host.docker.internal") || std::getenv("DB_SOCKET") ||
	    std::strcmp(required("ECONOMIC_SQL_LIFECYCLE_DISPOSABLE_SCHEMA"), "1") ||
	    !(stopped_drop_setup ? schema.starts_with("economic_schema_test_rb_") :
				   schema.starts_with("economic_lifecycle_test_")) ||
	    schema.find_first_not_of(
		    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") !=
		    std::string::npos)
		throw std::runtime_error("explicit disposable loopback schema required");
	char *end = nullptr;
	const auto port = std::strtoul(required("DB_PORT"), &end, 10);
	if (!end || *end || !port || port > 65535)
		throw std::runtime_error("invalid disposable DB port");
	auto *connection = mysql_init(nullptr);
	if (!connection)
		throw std::runtime_error("mysql_init failed");
	unsigned int timeout = 5, protocol = MYSQL_PROTOCOL_TCP;
	mysql_options(connection, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
	mysql_options(connection, MYSQL_OPT_READ_TIMEOUT, &timeout);
	mysql_options(connection, MYSQL_OPT_WRITE_TIMEOUT, &timeout);
	mysql_options(connection, MYSQL_OPT_PROTOCOL, &protocol);
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = false;
	mysql_options(connection, MYSQL_OPT_RECONNECT, &reconnect);
	if (!mysql_real_connect(connection, host.c_str(), required("DB_USER"),
				required("DB_PASSWD"), schema.c_str(),
				static_cast<unsigned int>(port), nullptr, 0))
	{
		const std::string error = mysql_error(connection);
		mysql_close(connection);
		throw std::runtime_error("mysql_real_connect: " + error);
	}
	return connection;
}

void execute(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()))
		throw std::runtime_error("SQL error " + std::to_string(mysql_errno(connection)) +
					 ": " + mysql_error(connection));
}

std::vector<std::vector<std::string>> rows(MYSQL *connection, const std::string &sql)
{
	execute(connection, sql);
	MYSQL_RES *result = mysql_store_result(connection);
	if (!result)
		throw std::runtime_error("query returned no result");
	std::vector<std::vector<std::string>> output;
	while (auto row = mysql_fetch_row(result))
	{
		auto lengths = mysql_fetch_lengths(result);
		std::vector<std::string> values;
		for (unsigned int i = 0; i < mysql_num_fields(result); ++i)
			values.emplace_back(row[i] ? std::string(row[i], lengths[i]) : "<NULL>");
		output.push_back(std::move(values));
	}
	mysql_free_result(result);
	if (mysql_errno(connection))
		throw std::runtime_error("result fetch failed");
	return output;
}
std::string scalar(MYSQL *connection, const std::string &sql)
{
	auto result = rows(connection, sql);
	if (result.size() != 1 || result[0].size() != 1)
		throw std::runtime_error("expected one scalar row");
	return result[0][0];
}
std::string sql_id(const critical_operation_id &value)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string output = "X'";
	for (auto byte : value.bytes)
	{
		output += digits[byte >> 4];
		output += digits[byte & 15];
	}
	output += '\'';
	return output;
}
std::string sql_account_key(const economic_account_key &account)
{
	std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> encoded = {};
	if (economic_account_key_encode(account, &encoded) != economic_accounting_error::ok)
		throw std::runtime_error("could not encode expected economic account key");
	static constexpr char digits[] = "0123456789abcdef";
	std::string output = "X'";
	for (const auto byte : encoded)
	{
		output += digits[byte >> 4];
		output += digits[byte & 15];
	}
	output += '\'';
	return output;
}
std::string coin_payload(uint64_t uid,
			 const std::array<int32_t, 8> &values = { 12, 2, 1, 0, 0, 0, 0, 0 })
{
	player_item_snapshot item{};
	item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	item.object_uid = uid;
	item.vnum = VOBJ_COINS;
	item.type = ITEM_MONEY;
	item.values = values;
	std::vector<uint8_t> payload;
	if (player_item_snapshot_list_encode({ item }, &payload) !=
	    player_snapshot_codec_result::ok)
		throw std::runtime_error("could not encode fixture coin pile");
	static constexpr char digits[] = "0123456789abcdef";
	std::string output = "X'";
	for (const auto byte : payload)
	{
		output += digits[byte >> 4];
		output += digits[byte & 15];
	}
	output += '\'';
	return output;
}
critical_operation_id ident(uint8_t seed)
{
	critical_operation_id value{};
	for (size_t index = 0; index < value.bytes.size(); ++index)
		value.bytes[index] = static_cast<uint8_t>(seed + index);
	return value;
}
bool same_account(const economic_account_key &left, const economic_account_key &right)
{
	return left.lineage.bytes == right.lineage.bytes && left.kind == right.kind &&
	       left.authority_id == right.authority_id && left.context_id == right.context_id;
}
void assert_scalar(MYSQL *connection, const std::string &sql, const std::string &expected)
{
	if (scalar(connection, sql) != expected)
		throw std::runtime_error("SQL assertion mismatch");
}
void writer_child(int ready_fd, int continue_fd, uint32_t pid)
{
	try
	{
		MYSQL *connection = connect_fixture();
		{
			economic_sql_currency_writer_guard guard;
			const auto status =
				economic_sql_currency_writer_guard::acquire(connection, &guard);
			if (status)
				_exit(20);
			execute(connection, "START TRANSACTION");
			execute(connection,
				"UPDATE player_data SET copper=copper+3,wallet_revision=wallet_revision+1 WHERE pid=" +
					std::to_string(pid));
			if (mysql_affected_rows(connection) != 1)
				_exit(21);
			const char ready = 'W';
			if (write(ready_fd, &ready, 1) != 1)
				_exit(22);
			char go = 0;
			if (read(continue_fd, &go, 1) != 1 || go != 'C')
				_exit(23);
			execute(connection, "COMMIT");
		}
		mysql_close(connection);
		const char done = 'D';
		if (write(ready_fd, &done, 1) != 1)
			_exit(24);
		_exit(0);
	}
	catch (...)
	{
		_exit(25);
	}
}
void assert_source_registry(MYSQL *connection)
{
	economic_sql_source_snapshot snapshot;
	if (const auto code = economic_sql_capture_sources(connection, {}, &snapshot))
		throw std::runtime_error(
			"full native source capture failed: " + std::to_string(code) +
			" server=" + mysql_get_server_info(connection));
	if (economic_sql_validate_sources(snapshot))
		throw std::runtime_error("source snapshot validation failed");
	for (const char *name :
	     { "player_data", "account_banks", "shopkeepers", "economic_account_mapping",
	       "item_current_owner", "item_uid_allocator" })
	{
		const auto found = std::find_if(snapshot.tables.begin(), snapshot.tables.end(),
						[&](const auto &table)
						{ return table.name == name; });
		if (found == snapshot.tables.end())
			throw std::runtime_error(std::string("source registry missing ") + name);
	}
	const auto wallet = std::find_if(snapshot.tables.begin(), snapshot.tables.end(),
					 [&](const auto &table)
					 { return table.name == "player_data"; });
	const auto bank = std::find_if(snapshot.tables.begin(), snapshot.tables.end(),
				       [&](const auto &table)
				       { return table.name == "account_banks"; });
	const auto shop = std::find_if(snapshot.tables.begin(), snapshot.tables.end(),
				       [&](const auto &table)
				       { return table.name == "shopkeepers"; });
	const auto items = std::find_if(snapshot.tables.begin(), snapshot.tables.end(),
					[&](const auto &table)
					{ return table.name == "item_current_owner"; });
	if (wallet->rows.size() != 2 || bank->rows.size() != 2 || shop->rows.size() != 1 ||
	    items->rows.size() != 3)
		throw std::runtime_error(
			"capture did not enumerate every wallet, shared bank and current item row");
}
void seed(MYSQL *connection)
{
	execute(connection,
		"INSERT INTO accounts(account_name) VALUES('lifecycle_a'),('lifecycle_b')");
	execute(connection,
		"INSERT INTO account_banks(account_name,racewar,bank_copper,bank_silver,bank_gold,bank_platinum,bank_revision) VALUES"
		"('lifecycle_a',0,8,1,0,0,4),('lifecycle_b',1,0,3,0,0,9)");
	execute(connection,
		"INSERT INTO shopkeepers(id,shop_id,mob_vnum,room_vnum,cash,shop_revision,"
		"keeper_roaming) VALUES(9,0,11005,100,73,4,0)");
	execute(connection,
		"INSERT INTO player_data(pid,name,account_name,racewar,copper,silver,gold,platinum,wallet_revision,save_revision) VALUES"
		"(21001,'LifecycleOne','lifecycle_a',0,4,2,0,0,6,12),"
		"(21002,'LifecycleTwo','lifecycle_b',1,0,5,1,0,8,15)");
	execute(connection, "UPDATE item_uid_allocator SET next_uid=6 WHERE allocator_id=1");
	execute(connection,
		"INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) VALUES(1,21001,0,7)");
	execute(connection,
		"INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,item_revision,vnum,state,coin_payload) VALUES"
		"(2,2,NULL,1,21001,0,7," +
			std::to_string(VOBJ_COINS) + ",1," + coin_payload(2) +
			"),"
			"(3,3,NULL,1,21001,0,2,501,1,NULL),"
			"(5,5,NULL,1,21001,0,9," +
			std::to_string(VOBJ_COINS) + ",1," +
			coin_payload(5, { 3, 1, 0, 0, 0, 0, 0, 0 }) + ")");
}
void assert_baseline_witness(MYSQL *connection, const economic_sql_lifecycle_receipt &receipt)
{
	const auto witness_rows =
		rows(connection,
		     "SELECT canonical_witness FROM economic_baseline_witness WHERE operation_id=" +
			     sql_id(receipt.baseline_operation_id));
	if (witness_rows.size() != 1 || witness_rows[0].size() != 1 ||
	    witness_rows[0][0] == "<NULL>")
		throw std::runtime_error("baseline witness receipt missing");
	std::vector<uint8_t> bytes(witness_rows[0][0].begin(), witness_rows[0][0].end());
	std::optional<economic_prepared_baseline> prepared;
	if (economic_baseline_decode(bytes, &prepared) != economic_accounting_error::ok ||
	    !prepared)
		throw std::runtime_error("retained baseline witness did not decode");
	const auto &holdings = prepared->witness().holdings;
	if (holdings.size() != 7)
		throw std::runtime_error(
			"baseline did not retain every wallet, bank, keeper treasury and coin pile");
	for (const auto &wallet : receipt.wallets)
	{
		const auto found =
			std::find_if(holdings.begin(), holdings.end(), [&](const auto &holding)
				     { return same_account(holding.account, wallet.account); });
		if (found == holdings.end())
			throw std::runtime_error("wallet mapping absent from retained baseline");
		if (wallet.pid == 21001 && (found->balance[0] != 7 || found->balance[1] != 2 ||
					    found->native_revision != 7))
			throw std::runtime_error(
				"baseline did not include the serialized legacy wallet write");
		if (wallet.pid == 21002 && (found->balance[0] != 0 || found->balance[1] != 5 ||
					    found->balance[2] != 1 || found->native_revision != 8))
			throw std::runtime_error("second wallet baseline values mismatch");
	}
	for (const auto &bank : receipt.banks)
	{
		const auto found =
			std::find_if(holdings.begin(), holdings.end(), [&](const auto &holding)
				     { return same_account(holding.account, bank.account); });
		if (found == holdings.end())
			throw std::runtime_error("bank mapping absent from retained baseline");
		if (bank.name == "lifecycle_a" &&
		    (found->balance[0] != 8 || found->balance[1] != 1 ||
		     found->native_revision != 4))
			throw std::runtime_error("first shared-bank baseline values mismatch");
		if (bank.name == "lifecycle_b" &&
		    (found->balance[0] != 0 || found->balance[1] != 3 ||
		     found->native_revision != 9))
			throw std::runtime_error("second shared-bank baseline values mismatch");
	}
	if (receipt.treasuries.size() != 1 || receipt.treasuries[0].shop_id != 0 ||
	    receipt.treasuries[0].native_id != 9)
		throw std::runtime_error("keeper treasury mapping missing from receipt");
	const auto keeper = std::find_if(
		holdings.begin(), holdings.end(), [&](const auto &holding)
		{ return same_account(holding.account, receipt.treasuries[0].account); });
	if (keeper == holdings.end() || keeper->balance != economic_coin_vector{ 73, 0, 0, 0 } ||
	    keeper->native_revision != 4)
		throw std::runtime_error("keeper cash absent from retained baseline");
	const auto assert_pile =
		[&](uint64_t uid, const economic_coin_vector &balance, uint64_t revision)
	{
		const economic_account_key account{ receipt.lineage, economic_account_kind::pile,
						    uid, 0 };
		const auto pile = std::find_if(holdings.begin(), holdings.end(),
					       [&](const auto &holding)
					       { return same_account(holding.account, account); });
		if (pile == holdings.end() || pile->balance != balance ||
		    pile->native_revision != revision ||
		    pile->source_digest == economic_sql_source_digest{})
			throw std::runtime_error("UID-keyed coin-pile baseline values mismatch");
	};
	assert_pile(2, { 12, 2, 1, 0 }, 7);
	assert_pile(5, { 3, 1, 0, 0 }, 9);
}
} // namespace

// Disposable, stopped-world Plan 1 acceptance setup. Unlike the default matrix,
// this branch never seeds/replaces players or items and never tears down authority.
static void assert_stopped_native_baseline(MYSQL *connection,
					   const economic_sql_lifecycle_receipt &receipt,
					   const economic_sql_source_snapshot &source)
{
	economic_sql_normalized_sources normalized;
	if (economic_sql_normalize_sources(source, 512, &normalized) !=
	    economic_accounting_error::ok)
		throw std::runtime_error("original stopped native source normalization failed");
	const auto stored =
		rows(connection,
		     "SELECT canonical_witness FROM economic_baseline_witness WHERE operation_id=" +
			     sql_id(receipt.baseline_operation_id));
	if (stored.size() != 1 || stored[0].size() != 1 || stored[0][0] == "<NULL>")
		throw std::runtime_error("retained fixture baseline witness missing");
	const std::vector<uint8_t> bytes(stored[0][0].begin(), stored[0][0].end());
	std::optional<economic_prepared_baseline> prepared;
	if (economic_baseline_decode(bytes, &prepared) != economic_accounting_error::ok ||
	    !prepared)
		throw std::runtime_error("retained fixture baseline witness invalid");
	size_t checked = 0;
	for (const auto &holding : normalized.holdings)
	{
		economic_account_key account{ receipt.lineage, economic_account_kind::pile,
					      holding.native_id, 0 };
		if (holding.kind == economic_sql_holding_kind::wallet)
		{
			const auto found = std::find_if(receipt.wallets.begin(),
							receipt.wallets.end(),
							[&](const auto &value)
							{ return value.pid == holding.native_id; });
			if (found == receipt.wallets.end())
				throw std::runtime_error("original wallet mapping missing");
			account = found->account;
		}
		else if (holding.kind == economic_sql_holding_kind::bank)
		{
			const auto &row =
				source.tables.at(holding.source.table).rows.at(holding.source.row);
			const auto found =
				std::find_if(receipt.banks.begin(), receipt.banks.end(),
					     [&](const auto &value)
					     {
						     return row.cells.at(1) &&
							    value.name == *row.cells[1] &&
							    value.racewar == holding.native_context;
					     });
			if (found == receipt.banks.end())
				throw std::runtime_error("original bank mapping missing");
			account = found->account;
		}
		else if (holding.kind == economic_sql_holding_kind::treasury)
		{
			const auto found =
				std::find_if(receipt.treasuries.begin(), receipt.treasuries.end(),
					     [&](const auto &value)
					     { return value.native_id == holding.native_id; });
			if (found == receipt.treasuries.end())
				throw std::runtime_error("original treasury mapping missing");
			account = found->account;
		}
		else if (holding.kind != economic_sql_holding_kind::pile)
			continue;
		const auto &holdings = prepared->witness().holdings;
		const auto found = std::find_if(holdings.begin(), holdings.end(),
						[&](const auto &value)
						{ return same_account(value.account, account); });
		if (holding.disposition != economic_sql_holding_disposition::current ||
		    !holding.balance || !holding.native_revision || found == holdings.end() ||
		    found->balance != *holding.balance ||
		    found->native_revision != *holding.native_revision ||
		    found->source_digest != holding.source.digest)
			throw std::runtime_error(
				"retained baseline differs from original native holding");
		++checked;
	}
	if (checked != prepared->witness().holdings.size() ||
	    checked != receipt.wallets.size() + receipt.banks.size() + receipt.treasuries.size() +
			       static_cast<size_t>(std::count_if(
				       normalized.holdings.begin(), normalized.holdings.end(),
				       [](const auto &value)
				       { return value.kind == economic_sql_holding_kind::pile; })))
		throw std::runtime_error("baseline/mapping/native source cardinality mismatch");
}

static int setup_stopped_drop_fixture()
{
	MYSQL *connection = nullptr;
	bool coordinator = false;
	try
	{
		if (std::strcmp(required("TEST_DB_DISPOSABLE"), "1") ||
		    std::strcmp(required("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA"), "1") ||
		    std::strcmp(required("DB_HOST"), "127.0.0.1"))
			throw std::runtime_error("explicit stopped disposable fixture required");
		connection = connect_fixture();
		const std::string schema = required("DB_NAME");
		if (schema.size() != std::strlen("economic_schema_test_rb_") + 8 ||
		    std::strlen("duris.player.death.restitution.") + schema.size() > 64 ||
		    required("DB_ALLOWED_TARGETS") != "127.0.0.1/" + schema)
			throw std::runtime_error("stopped fixture target/name mismatch");
		assert_scalar(
			connection,
			"SELECT COUNT(*) FROM information_schema.PROCESSLIST WHERE DB=DATABASE() AND ID<>CONNECTION_ID()",
			"0");
		assert_scalar(connection,
			      "SELECT COUNT(*) FROM economic_sql_lifecycle_installation", "0");
		assert_scalar(connection, "SELECT COUNT(*) FROM economic_sql_global_activation",
			      "0");
		assert_scalar(
			connection,
			"SELECT COUNT(*) FROM economic_lineage_state WHERE active_epoch IS NOT NULL",
			"0");
		if (scalar(connection, "SELECT COUNT(*) FROM player_data") == "0")
			throw std::runtime_error(
				"real character must precede authority installation");
		economic_sql_lifecycle_request request;
		if (!critical_operation_id_generate(&request.operation_id) ||
		    !critical_operation_id_generate(&request.lineage) ||
		    !critical_operation_id_generate(&request.epoch))
			throw std::runtime_error("fixture lifecycle identity allocation failed");
		request.actor_id = 9001;
		request.accepted_at_usec = 123456789;
		economic_sql_lifecycle_receipt receipt;
		economic_sql_source_snapshot original_sources;
		{
			economic_sql_lifecycle_guard guard;
			const auto acquired = economic_sql_lifecycle_guard::acquire_maintenance(
				connection, &guard);
			if (acquired ||
			    economic_sql_capture_sources(connection, {}, &original_sources) ||
			    economic_sql_validate_sources(original_sources))
				throw std::runtime_error(
					"original stopped native source capture failed");
			const auto installed =
				acquired ? acquired :
					   economic_sql_accounting_lifecycle_transaction::install(
						   connection, guard, request, &receipt);
			if (installed || !guard.release())
				throw std::runtime_error(
					"guarded stopped-world source capture/install refused");
		}
		if (receipt.wallets.empty() || !receipt.baseline_revision ||
		    receipt.source_capture_digest == economic_sql_source_digest{} ||
		    receipt.native_boundary_digest == economic_sql_source_digest{})
			throw std::runtime_error("incomplete native baseline receipt");
		assert_stopped_native_baseline(connection, receipt, original_sources);
		assert_scalar(
			connection,
			"SELECT COUNT(*) FROM economic_lineage_state WHERE active_epoch IS NOT NULL",
			"0");
		coordinator = critical_command_coordinator_init(
			required("ECONOMIC_SQL_LIFECYCLE_JOURNAL_DIR"), no_gameplay_apply, nullptr,
			1);
		if (!coordinator)
			throw std::runtime_error(
				"stopped cutover coordinator initialization failed");
		economic_sql_activation_evidence coverage;
		coverage.manifest_digest.fill(0x44);
		coverage.audit_digest.fill(0x55);
		coverage.route_count = coverage.verified_route_count = 3;
		{
			economic_sql_lifecycle_guard guard;
			economic_sql_cutover_capability lease;
			economic_sql_cutover_transaction_owner owner;
			if (economic_sql_lifecycle_guard::acquire_maintenance(connection, &guard) ||
			    !guard.acquire_cutover_capability(3000, &lease) ||
			    !owner.begin(guard, lease))
				throw std::runtime_error("stopped cutover ownership refused");
			const auto activated =
				economic_sql_accounting_lifecycle_transaction::activate_verified(
					connection, owner, request, coverage,
					verify_synthetic_routes);
			if (activated)
			{
				(void)owner.rollback();
				throw std::runtime_error("verified fixture activation refused");
			}
			if (!owner.commit())
				throw std::runtime_error(
					"fixture activation terminal/cleanup refused");
		}
		if (!critical_command_coordinator_shutdown())
			throw std::runtime_error("stopped fixture coordinator shutdown refused");
		coordinator = false;
		{
			economic_sql_lifecycle_guard guard;
			bool active = false;
			if (economic_sql_lifecycle_guard::acquire_runtime(connection, &guard) ||
			    economic_sql_accounting_lifecycle_transaction::recover_runtime(
				    connection, guard, &active) ||
			    !active || !guard.release())
				throw std::runtime_error("native activation readback refused");
		}
		economic_gameplay_authority_test_access::reset();
		mysql_close(connection);
		mysql_library_end();
		puts("PASS STOPPED_DROP_AUTHORITY actual_capture=1 baseline_readback=1 verified_cutover=1 runtime_readback=1 synthetic_routes=3 production_route_qualification=0");
		return 0;
	}
	catch (const std::exception &error)
	{
		if (coordinator)
			(void)critical_command_coordinator_shutdown();
		if (connection)
			mysql_close(connection);
		mysql_library_end();
		fprintf(stderr, "STOPPED-DROP-SETUP-ERROR %s\n", error.what());
		return 2;
	}
}

int main(int argc, char **argv)
{
	if (argc == 2 && !std::strcmp(argv[1], "--setup-stopped-drop-fixture"))
	{
		stopped_drop_setup = true;
		if (mysql_library_init(0, nullptr, nullptr))
			return 2;
		return setup_stopped_drop_fixture();
	}
	if (argc != 1)
		return 2;
	MYSQL *setup = nullptr;
	MYSQL *owner_connection = nullptr;
	MYSQL *runtime_connection = nullptr;
	try
	{
		if (mysql_library_init(0, nullptr, nullptr))
			throw std::runtime_error("mysql_library_init failed");
		setup = connect_fixture();
		seed(setup);
		assert_scalar(
			setup,
			"SELECT COUNT(*) FROM economic_sql_lifecycle_installation WHERE phase IN (1,2)",
			"0");
		assert_scalar(
			setup,
			"SELECT COUNT(*) FROM economic_lineage_state WHERE active_epoch IS NOT NULL",
			"0");
		assert_source_registry(setup);
		owner_connection = connect_fixture();
		runtime_connection = connect_fixture();
		{
			economic_sql_lifecycle_guard runtime;
			if (economic_sql_lifecycle_guard::acquire_runtime(runtime_connection,
									  &runtime))
				throw std::runtime_error("runtime boot guard acquire failed");
			economic_sql_lifecycle_guard denied;
			if (economic_sql_lifecycle_guard::acquire_maintenance(owner_connection,
									      &denied) != EBUSY)
				throw std::runtime_error(
					"maintenance was not refused while runtime authority was held");
			economic_sql_lifecycle_guard same_connection_denied;
			if (economic_sql_lifecycle_guard::acquire_maintenance(
				    runtime_connection, &same_connection_denied) != EBUSY)
				throw std::runtime_error(
					"recursive named lock admitted maintenance on the runtime control session");
		}

		int ready_pipe[2], continue_pipe[2];
		if (pipe(ready_pipe) || pipe(continue_pipe))
			throw std::runtime_error("pipe setup failed");
		const uint32_t writer_pid = 21001;
		const pid_t child = fork();
		if (child < 0)
			throw std::runtime_error("fork failed");
		if (!child)
		{
			close(ready_pipe[0]);
			close(continue_pipe[1]);
			writer_child(ready_pipe[1], continue_pipe[0], writer_pid);
		}
		close(ready_pipe[1]);
		close(continue_pipe[0]);
		char signal = 0;
		if (read(ready_pipe[0], &signal, 1) != 1 || signal != 'W')
			throw std::runtime_error(
				"guarded legacy writer did not reach its uncommitted write");
		std::thread release(
			[fd = continue_pipe[1]]
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(250));
				const char go = 'C';
				if (write(fd, &go, 1) != 1)
					std::abort();
			});
		const auto started = std::chrono::steady_clock::now();
		auto maintenance = std::make_unique<economic_sql_lifecycle_guard>();
		const auto maintenance_status = economic_sql_lifecycle_guard::acquire_maintenance(
			owner_connection, maintenance.get());
		const auto elapsed = std::chrono::steady_clock::now() - started;
		release.join();
		if (maintenance_status || elapsed < std::chrono::milliseconds(100))
			throw std::runtime_error(
				"maintenance fence did not serialize behind a concurrent writer");
		int child_status = 0;
		if (waitpid(child, &child_status, 0) != child || !WIFEXITED(child_status) ||
		    WEXITSTATUS(child_status) != 0)
			throw std::runtime_error("guarded legacy writer child failed");
		if (read(ready_pipe[0], &signal, 1) != 1 || signal != 'D')
			throw std::runtime_error("guarded legacy writer commit was not observed");
		close(ready_pipe[0]);
		close(continue_pipe[1]);

		economic_sql_lifecycle_request invalid_request;
		invalid_request.operation_id = ident(12);
		invalid_request.lineage = ident(72);
		invalid_request.epoch = ident(112);
		invalid_request.actor_id = 9001;
		invalid_request.accepted_at_usec = 123456789;
		// Even a damaged unresolved archive blocks source cutover; raw retained
		// observations must never be counted as an opening inventory/holding.
		execute(setup,
			"INSERT INTO player_death_conflict_evidence(operation_id,pid,save_revision,source_revision,corpse_item_uid,request_hash,payload_hash,payload) VALUES(" +
				sql_id(ident(13)) +
				",21001,2,1,90001,UNHEX(REPEAT('00',32)),UNHEX(REPEAT('00',32)),UNHEX('01'))");
		economic_sql_lifecycle_receipt conflict_output;
		conflict_output.operation_id = ident(211);
		const auto conflict_status = economic_sql_accounting_lifecycle_transaction::install(
			owner_connection, *maintenance, invalid_request, &conflict_output);
		if (conflict_status != EBUSY ||
		    conflict_output.operation_id.bytes != ident(211).bytes)
			throw std::runtime_error(
				"unresolved death archive did not block lifecycle cutover");
		assert_scalar(setup, "SELECT COUNT(*) FROM economic_sql_lifecycle_installation",
			      "0");
		assert_scalar(setup, "SELECT COUNT(*) FROM player_death_conflict_evidence", "1");
		execute(setup, "DELETE FROM player_death_conflict_evidence WHERE operation_id=" +
				       sql_id(ident(13)));
		execute(setup, "UPDATE shopkeepers SET cash=NULL WHERE id=9");
		economic_sql_lifecycle_receipt unknown_keeper_output;
		unknown_keeper_output.operation_id = ident(208);
		const auto unknown_keeper_status =
			economic_sql_accounting_lifecycle_transaction::install(
				owner_connection, *maintenance, invalid_request,
				&unknown_keeper_output);
		if (unknown_keeper_status != EILSEQ ||
		    unknown_keeper_output.operation_id.bytes != ident(208).bytes)
			throw std::runtime_error(
				"unknown keeper cash was admitted to opening baseline");
		assert_scalar(setup,
			      "SELECT COUNT(*) FROM economic_account_mapping WHERE lineage=" +
				      sql_id(invalid_request.lineage),
			      "0");
		execute(setup, "UPDATE shopkeepers SET cash=73 WHERE id=9");
		execute(setup, "UPDATE shopkeepers SET keeper_roaming=NULL WHERE id=9");
		economic_sql_lifecycle_receipt unknown_policy_output;
		unknown_policy_output.operation_id = ident(209);
		const auto unknown_policy_status =
			economic_sql_accounting_lifecycle_transaction::install(
				owner_connection, *maintenance, invalid_request,
				&unknown_policy_output);
		if (unknown_policy_status != EBUSY ||
		    unknown_policy_output.operation_id.bytes != ident(209).bytes)
			throw std::runtime_error(
				"unknown keeper roaming policy was admitted to opening baseline");
		assert_scalar(setup,
			      "SELECT COUNT(*) FROM economic_account_mapping WHERE lineage=" +
				      sql_id(invalid_request.lineage),
			      "0");
		execute(setup, "UPDATE shopkeepers SET keeper_roaming=0 WHERE id=9");
		assert_scalar(setup, "SELECT keeper_roaming FROM shopkeepers WHERE id=9", "0");
		execute(setup,
			"INSERT INTO shopkeeper_items(id,shopkeeper_id,vnum,obj_uid,item_condition) "
			"VALUES(91,9,501,91,NULL)");
		economic_sql_lifecycle_receipt unknown_condition_output;
		unknown_condition_output.operation_id = ident(212);
		const auto unknown_condition_status =
			economic_sql_accounting_lifecycle_transaction::install(
				owner_connection, *maintenance, invalid_request,
				&unknown_condition_output);
		if (unknown_condition_status != EBUSY ||
		    unknown_condition_output.operation_id.bytes != ident(212).bytes)
			throw std::runtime_error(
				"unknown keeper item condition was admitted to opening baseline");
		assert_scalar(setup,
			      "SELECT COUNT(*) FROM economic_account_mapping WHERE lineage=" +
				      sql_id(invalid_request.lineage),
			      "0");
		execute(setup, "DELETE FROM shopkeeper_items WHERE id=91");
		execute(setup, "UPDATE item_current_owner SET coin_payload=NULL WHERE item_uid=2");
		economic_sql_lifecycle_receipt bad_coin_output;
		bad_coin_output.operation_id = ident(210);
		const auto bad_coin_status = economic_sql_accounting_lifecycle_transaction::install(
			owner_connection, *maintenance, invalid_request, &bad_coin_output);
		if (bad_coin_status != EILSEQ ||
		    bad_coin_output.operation_id.bytes != ident(210).bytes)
			throw std::runtime_error(
				"active coin pile without a valid payload was accepted or modified the receipt");
		assert_scalar(
			setup,
			"SELECT COUNT(*) FROM economic_sql_lifecycle_installation WHERE lineage=" +
				sql_id(invalid_request.lineage),
			"0");
		assert_scalar(setup,
			      "SELECT COUNT(*) FROM economic_account_mapping WHERE lineage=" +
				      sql_id(invalid_request.lineage),
			      "0");
		execute(setup, "UPDATE item_current_owner SET coin_payload=" + coin_payload(2) +
				       " WHERE item_uid=2");
		execute(setup, "UPDATE item_current_owner SET state=3 WHERE item_uid=5");
		economic_sql_lifecycle_receipt quarantined_coin_output;
		quarantined_coin_output.operation_id = ident(209);
		const auto quarantined_coin_status =
			economic_sql_accounting_lifecycle_transaction::install(
				owner_connection, *maintenance, invalid_request,
				&quarantined_coin_output);
		if (quarantined_coin_status != EBUSY ||
		    quarantined_coin_output.operation_id.bytes != ident(209).bytes)
			throw std::runtime_error(
				"unresolved coin-pile custody was accepted or modified the receipt");
		assert_scalar(
			setup,
			"SELECT COUNT(*) FROM economic_sql_lifecycle_installation WHERE lineage=" +
				sql_id(invalid_request.lineage),
			"0");
		execute(setup, "UPDATE item_current_owner SET state=1 WHERE item_uid=5");
		execute(setup,
			"INSERT INTO player_data(pid,name,copper,silver,gold,platinum,wallet_revision,save_revision) VALUES(21003,'LifecycleBad',-1,0,0,0,0,0)");
		economic_sql_lifecycle_receipt untouched;
		untouched.operation_id = ident(212);
		const auto invalid_status = economic_sql_accounting_lifecycle_transaction::install(
			owner_connection, *maintenance, invalid_request, &untouched);
		if (invalid_status != ERANGE || untouched.operation_id.bytes != ident(212).bytes)
			throw std::runtime_error(
				"invalid native wallet returned " + std::to_string(invalid_status) +
				" instead of ERANGE or modified the output receipt");
		assert_scalar(
			setup,
			"SELECT COUNT(*) FROM economic_sql_lifecycle_installation WHERE lineage=" +
				sql_id(invalid_request.lineage),
			"0");
		assert_scalar(setup,
			      "SELECT COUNT(*) FROM economic_account_mapping WHERE lineage=" +
				      sql_id(invalid_request.lineage),
			      "0");
		execute(setup, "DELETE FROM player_data WHERE pid=21003");
		// Fail after mappings and baseline control have been inserted. Session
		// variables survive rollback and prove this reached a partial-write boundary.
		execute(owner_connection,
			"SET @lifecycle_partial_mappings=NULL,@lifecycle_partial_control=NULL");
		execute(setup,
			"CREATE TRIGGER lifecycle_installation_fault BEFORE INSERT ON economic_sql_lifecycle_installation FOR EACH ROW BEGIN SET @lifecycle_partial_mappings=(SELECT COUNT(*) FROM economic_account_mapping WHERE lineage=NEW.lineage); SET @lifecycle_partial_control=(SELECT COUNT(*) FROM economic_baseline_control WHERE lineage=NEW.lineage); SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='injected lifecycle installation failure'; END");
		const auto partial_status = economic_sql_accounting_lifecycle_transaction::install(
			owner_connection, *maintenance, invalid_request, &untouched);
		if (!partial_status || untouched.operation_id.bytes != ident(212).bytes)
			throw std::runtime_error(
				"partial installation failure was not retained unchanged");
		assert_scalar(owner_connection, "SELECT @lifecycle_partial_mappings", "5");
		assert_scalar(owner_connection, "SELECT @lifecycle_partial_control", "1");
		for (const char *table :
		     { "economic_lineage_state", "economic_epoch", "economic_account_mapping",
		       "economic_baseline_control", "economic_sql_lifecycle_installation" })
			assert_scalar(setup,
				      "SELECT COUNT(*) FROM " + std::string(table) +
					      " WHERE lineage=" + sql_id(invalid_request.lineage),
				      "0");
		assert_scalar(setup,
			      "SELECT COUNT(*) FROM critical_operation_inbox WHERE operation_id=" +
				      sql_id(invalid_request.operation_id),
			      "0");
		execute(setup, "DROP TRIGGER lifecycle_installation_fault");
		execute(setup,
			"CREATE TRIGGER lifecycle_receipt_before_selection BEFORE UPDATE ON economic_sql_lifecycle_installation FOR EACH ROW BEGIN IF NEW.phase=2 AND (NEW.baseline_operation_id IS NULL OR NOT EXISTS (SELECT 1 FROM critical_operation_inbox WHERE operation_id=NEW.baseline_operation_id AND status=1 AND result_code=0 AND failure_stage=0 AND committed_at IS NOT NULL) OR NOT EXISTS (SELECT 1 FROM economic_baseline_witness WHERE operation_id=NEW.baseline_operation_id AND lineage=NEW.lineage AND epoch=NEW.epoch)) THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='baseline receipt must precede selection'; END IF; END");

		economic_sql_lifecycle_request request;
		request.operation_id = ident(31);
		request.lineage = ident(71);
		request.epoch = ident(111);
		request.actor_id = 9001;
		request.accepted_at_usec = 123456789;
		// Interrupt selection after the baseline is durable, then resume the exact
		// phase-1 operation rather than minting new mappings or another baseline.
		execute(setup,
			"CREATE TRIGGER lifecycle_selection_fault BEFORE UPDATE ON economic_sql_lifecycle_installation FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='injected selection failure'");
		economic_sql_lifecycle_receipt interrupted;
		interrupted.operation_id = ident(212);
		const auto interrupted_status =
			economic_sql_accounting_lifecycle_transaction::install(
				owner_connection, *maintenance, request, &interrupted);
		if (!interrupted_status || interrupted.operation_id.bytes != ident(212).bytes)
			throw std::runtime_error(
				"interrupted selection returned success or changed output");
		assert_scalar(
			setup,
			"SELECT CONCAT(phase,':',selected_epoch IS NULL,':',revision) FROM economic_sql_lifecycle_installation WHERE operation_id=" +
				sql_id(request.operation_id),
			"1:1:0");
		assert_scalar(setup,
			      "SELECT COUNT(*) FROM economic_baseline_witness WHERE lineage=" +
				      sql_id(request.lineage),
			      "1");
		assert_scalar(setup,
			      "SELECT revision FROM economic_baseline_control WHERE lineage=" +
				      sql_id(request.lineage),
			      "1");
		execute(setup, "DROP TRIGGER lifecycle_selection_fault");
		economic_sql_lifecycle_receipt receipt;
		const auto install_status = economic_sql_accounting_lifecycle_transaction::install(
			owner_connection, *maintenance, request, &receipt);
		if (install_status)
			throw std::runtime_error("SQL lifecycle install failed with " +
						 std::to_string(install_status));
		if (receipt.wallets.size() != 2 || receipt.banks.size() != 2 ||
		    receipt.treasuries.size() != 1 || receipt.baseline_revision != 1 ||
		    receipt.source_capture_digest == economic_sql_source_digest{} ||
		    receipt.native_boundary_digest == economic_sql_source_digest{})
			throw std::runtime_error(
				"lifecycle export did not cover the full native wallet/bank set");
		assert_scalar(setup,
			      "SELECT COUNT(*) FROM economic_account_mapping WHERE lineage=" +
				      sql_id(request.lineage),
			      "5");
		assert_scalar(setup,
			      "SELECT COUNT(*) FROM economic_account_mapping WHERE lineage=" +
				      sql_id(request.lineage) + " AND account_kind=3",
			      "0");
		for (const auto &wallet : receipt.wallets)
			assert_scalar(
				setup,
				"SELECT COUNT(*) FROM economic_account_mapping WHERE lineage=" +
					sql_id(request.lineage) +
					" AND account_kind=1 AND locator_kind=1 AND native_id=" +
					std::to_string(wallet.pid) + " AND active_native_id=" +
					std::to_string(wallet.pid) + " AND mapping_id=" +
					std::to_string(wallet.account.authority_id),
				"1");
		for (const auto &bank : receipt.banks)
			assert_scalar(
				setup,
				"SELECT COUNT(*) FROM economic_account_mapping WHERE lineage=" +
					sql_id(request.lineage) +
					" AND account_kind=2 AND locator_kind=2 AND context_id=" +
					std::to_string(bank.racewar) +
					" AND native_id=(SELECT id FROM account_banks WHERE account_name='" +
					bank.name + "' AND racewar=" +
					std::to_string(bank.racewar) + ") AND mapping_id=" +
					std::to_string(bank.account.authority_id),
				"1");
		for (const auto &keeper : receipt.treasuries)
			assert_scalar(
				setup,
				"SELECT COUNT(*) FROM economic_account_mapping WHERE lineage=" +
					sql_id(request.lineage) +
					" AND account_kind=6 AND locator_kind=6 AND native_id=" +
					std::to_string(keeper.native_id) + " AND mapping_id=" +
					std::to_string(keeper.account.authority_id),
				"1");
		assert_scalar(
			setup,
			"SELECT COUNT(*) FROM information_schema.TRIGGERS WHERE TRIGGER_SCHEMA=DATABASE() "
			"AND TRIGGER_NAME='lifecycle_receipt_before_selection'",
			"1");
		assert_scalar(setup,
			      "SELECT COUNT(*) FROM economic_baseline_reservation WHERE lineage=" +
				      sql_id(request.lineage) + " AND epoch=" +
				      sql_id(request.epoch) + " AND identity_kind=1",
			      "7");
		assert_scalar(setup,
			      "SELECT COUNT(*) FROM economic_baseline_reservation WHERE lineage=" +
				      sql_id(request.lineage) +
				      " AND epoch=" + sql_id(request.epoch) +
				      " AND identity_kind=1 AND identity_id IN (2,5)",
			      "2");
		assert_scalar(setup,
			      "SELECT COUNT(*) FROM economic_account_mapping WHERE lineage=" +
				      sql_id(request.lineage) + " AND mapping_id IN (2,5)",
			      "0");
		assert_scalar(setup,
			      "SELECT COUNT(*) FROM economic_baseline_witness WHERE operation_id=" +
				      sql_id(receipt.baseline_operation_id),
			      "1");
		assert_scalar(
			setup,
			"SELECT phase FROM economic_sql_lifecycle_installation WHERE operation_id=" +
				sql_id(request.operation_id),
			"2");
		assert_scalar(
			setup,
			"SELECT selected_epoch=epoch FROM economic_sql_lifecycle_installation WHERE operation_id=" +
				sql_id(request.operation_id),
			"1");
		assert_scalar(
			setup,
			"SELECT active_epoch IS NULL FROM economic_lineage_state WHERE lineage=" +
				sql_id(request.lineage),
			"1");
		assert_baseline_witness(setup, receipt);
		const auto assert_pile_posting =
			[&](uint64_t uid, const std::string &effect, const std::string &posting)
		{
			const economic_account_key pile_account{ request.lineage,
								 economic_account_kind::pile, uid,
								 0 };
			assert_scalar(
				setup,
				"SELECT CONCAT(after_copper,':',after_silver,':',after_gold,':',after_platinum,':',after_revision) FROM economic_accounting_account_effect WHERE operation_id=" +
					sql_id(receipt.baseline_operation_id) +
					" AND account_key=" + sql_account_key(pile_account),
				effect);
			assert_scalar(
				setup,
				"SELECT CONCAT(p.delta_copper,':',p.delta_silver,':',p.delta_gold,':',p.delta_platinum,':',p.copper_value) FROM economic_accounting_coin_posting p JOIN economic_accounting_account_effect e ON e.operation_id=p.operation_id AND e.account_index=p.account_index WHERE p.operation_id=" +
					sql_id(receipt.baseline_operation_id) +
					" AND e.account_key=" + sql_account_key(pile_account),
				posting);
		};
		assert_pile_posting(2, "12:2:1:0:1", "12:2:1:0:132");
		assert_pile_posting(5, "3:1:0:0:1", "3:1:0:0:13");
		const std::string clear_selected =
			"UPDATE economic_sql_lifecycle_installation SET selected_epoch=NULL WHERE operation_id=" +
			sql_id(request.operation_id);
		if (mysql_real_query(setup, clear_selected.data(), clear_selected.size()) == 0 ||
		    std::string(mysql_error(setup)).find("ck_economic_sql_lifecycle_phase") ==
			    std::string::npos)
			throw std::runtime_error(
				"phase constraint permitted a selected installation with no selected epoch");
		assert_scalar(
			setup,
			"SELECT selected_epoch=epoch FROM economic_sql_lifecycle_installation WHERE operation_id=" +
				sql_id(request.operation_id),
			"1");

		const auto before_revision = scalar(
			setup, "SELECT revision FROM economic_baseline_control WHERE lineage=" +
				       sql_id(request.lineage) +
				       " AND epoch=" + sql_id(request.epoch));
		economic_sql_lifecycle_receipt replay;
		if (economic_sql_accounting_lifecycle_transaction::install(
			    owner_connection, *maintenance, request, &replay))
			throw std::runtime_error("exact lifecycle replay failed");
		if (replay.wallets.size() != receipt.wallets.size() ||
		    replay.banks.size() != receipt.banks.size() ||
		    replay.treasuries.size() != receipt.treasuries.size() ||
		    replay.baseline_operation_id.bytes != receipt.baseline_operation_id.bytes ||
		    replay.wallets[0].account.authority_id !=
			    receipt.wallets[0].account.authority_id ||
		    scalar(setup, "SELECT revision FROM economic_baseline_control WHERE lineage=" +
					  sql_id(request.lineage) +
					  " AND epoch=" + sql_id(request.epoch)) != before_revision)
			throw std::runtime_error(
				"exact replay changed durable mappings or baseline revision");
		assert_scalar(setup,
			      "SELECT COUNT(*) FROM economic_baseline_witness WHERE lineage=" +
				      sql_id(request.lineage) +
				      " AND epoch=" + sql_id(request.epoch),
			      "1");

		auto conflict = request;
		++conflict.actor_id;
		economic_sql_lifecycle_receipt unchanged;
		unchanged.operation_id = ident(212);
		if (economic_sql_accounting_lifecycle_transaction::install(
			    owner_connection, *maintenance, conflict, &unchanged) != EEXIST ||
		    unchanged.operation_id.bytes != ident(212).bytes)
			throw std::runtime_error(
				"same operation ID with changed request was not rejected unchanged");

		// Synthetic route coverage exercises the durable decision only in this
		// disposable schema. Production has no Plan 5 verifier registration yet.
		economic_sql_activation_evidence coverage;
		coverage.manifest_digest.fill(0x44);
		coverage.audit_digest.fill(0x55);
		coverage.route_count = 3;
		coverage.verified_route_count = 2;
		maintenance.reset();
		const std::string journal = required("ECONOMIC_SQL_LIFECYCLE_JOURNAL_DIR");
		if (!critical_command_coordinator_init(journal.c_str(), no_gameplay_apply, nullptr,
						       1))
			throw std::runtime_error("coordinator setup failed");
		auto cutover = [&](auto action, bool commit_on_success = false)
		{
			economic_sql_lifecycle_guard guard;
			economic_sql_cutover_capability lease;
			economic_sql_cutover_transaction_owner transaction;
			if (economic_sql_lifecycle_guard::acquire_maintenance(owner_connection,
									      &guard) ||
			    !guard.acquire_cutover_capability(3000, &lease) ||
			    !transaction.begin(guard, lease))
				throw std::runtime_error(
					"cutover owner did not acquire the drained transaction");
			const auto code = action(transaction);
			if (!(code == 0 && commit_on_success ? transaction.commit() :
							       transaction.rollback()))
				throw std::runtime_error(
					"cutover owner could not finish transaction");
			return code;
		};
		if (cutover(
			    [&](auto &owner)
			    {
				    return economic_sql_accounting_lifecycle_transaction::activate(
					    owner_connection, owner, request.lineage);
			    }) != ENODATA)
			throw std::runtime_error("pointer selector accepted no global decision");
		if (cutover(
			    [&](auto &owner)
			    {
				    return economic_sql_accounting_lifecycle_transaction::
					    activate_verified(owner_connection, owner, request,
							      coverage, verify_synthetic_routes);
			    }) != ENODATA)
			throw std::runtime_error("incomplete route manifest activated");
		coverage.verified_route_count = coverage.route_count;
		if (cutover(
			    [&](auto &owner)
			    {
				    return economic_sql_accounting_lifecycle_transaction::
					    activate_verified(owner_connection, owner, request,
							      coverage, nullptr);
			    }) != ENODATA)
			throw std::runtime_error("missing independent verifier activated");
		execute(setup, "UPDATE critical_operation_inbox SET status=0 WHERE operation_id=" +
				       sql_id(request.operation_id));
		if (cutover(
			    [&](auto &owner)
			    {
				    return economic_sql_accounting_lifecycle_transaction::
					    activate_verified(owner_connection, owner, request,
							      coverage, verify_synthetic_routes);
			    }) != EBUSY)
			throw std::runtime_error("incomplete inbox activated");
		execute(setup, "UPDATE critical_operation_inbox SET status=1 WHERE operation_id=" +
				       sql_id(request.operation_id));
		execute(setup, "INSERT INTO critical_outbox(operation_id,event_index,destination,"
			       "event_type,payload_version,payload) VALUES(" +
				       sql_id(request.operation_id) + ",0,1,1,1,X'01')");
		if (cutover(
			    [&](auto &owner)
			    {
				    return economic_sql_accounting_lifecycle_transaction::
					    activate_verified(owner_connection, owner, request,
							      coverage, verify_synthetic_routes);
			    }) != EBUSY)
			throw std::runtime_error("pending outbox activated");
		execute(setup, "DELETE FROM critical_outbox WHERE operation_id=" +
				       sql_id(request.operation_id));
		if (const auto code = cutover(
			    [&](auto &owner)
			    {
				    return economic_sql_accounting_lifecycle_transaction::
					    activate_verified(owner_connection, owner, request,
							      coverage, verify_synthetic_routes);
			    },
			    true))
			throw std::runtime_error("synthetic activation failed: " +
						 std::to_string(code));
		assert_scalar(
			setup,
			"SELECT CONCAT(state,':',revision) FROM economic_sql_global_activation WHERE lineage=" +
				sql_id(request.lineage),
			"1:1");
		assert_scalar(setup,
			      "SELECT active_epoch= " + sql_id(request.epoch) +
				      " FROM economic_lineage_state WHERE lineage=" +
				      sql_id(request.lineage),
			      "1");
		if (cutover(
			    [&](auto &owner)
			    {
				    return economic_sql_accounting_lifecycle_transaction::
					    activate_verified(owner_connection, owner, request,
							      coverage, verify_synthetic_routes);
			    },
			    true))
			throw std::runtime_error("exact active decision retry failed");
		assert_scalar(
			setup,
			"SELECT CONCAT(state,':',revision) FROM economic_sql_global_activation WHERE lineage=" +
				sql_id(request.lineage),
			"1:1");
		critical_command_coordinator_shutdown();
		{
			economic_sql_lifecycle_guard active_runtime;
			if (economic_sql_lifecycle_guard::acquire_runtime(runtime_connection,
									  &active_runtime))
				throw std::runtime_error("active receipt was refused at boot");
			bool active = false;
			if (const auto code =
				    economic_sql_accounting_lifecycle_transaction::recover_runtime(
					    runtime_connection, active_runtime, &active);
			    code || !active || !economic_gameplay_authority::active())
				throw std::runtime_error("runtime cache recovery failed: " +
							 std::to_string(code));
			economic_gameplay_authority_test_access::reset();
		}
		if (!critical_command_coordinator_init(journal.c_str(), no_gameplay_apply, nullptr,
						       1))
			throw std::runtime_error("pause authority setup failed");
		if (cutover(
			    [&](auto &owner)
			    {
				    return economic_sql_accounting_lifecycle_transaction::pause(
					    owner_connection, owner, request.lineage);
			    },
			    true))
			throw std::runtime_error("durable pause failed");
		assert_scalar(
			setup,
			"SELECT CONCAT(state,':',revision) FROM economic_sql_global_activation WHERE lineage=" +
				sql_id(request.lineage),
			"2:2");
		{
			economic_sql_lifecycle_guard paused_runtime;
			if (economic_sql_lifecycle_guard::acquire_runtime(runtime_connection,
									  &paused_runtime) != EPERM)
				throw std::runtime_error("paused decision was admitted at boot");
		}
		if (cutover(
			    [&](auto &owner)
			    {
				    return economic_sql_accounting_lifecycle_transaction::
					    activate_verified(owner_connection, owner, request,
							      coverage, verify_synthetic_routes);
			    },
			    true) ||
		    cutover(
			    [&](auto &owner)
			    {
				    return economic_sql_accounting_lifecycle_transaction::pause(
					    owner_connection, owner, request.lineage);
			    },
			    true))
			throw std::runtime_error("pause/resume was not reversible");
		critical_command_coordinator_shutdown();
		execute(setup, "DELETE FROM economic_sql_global_activation WHERE lineage=" +
				       sql_id(request.lineage));

		MYSQL *legacy_connection = connect_fixture();
		maintenance.reset();
		{
			economic_sql_currency_writer_guard denied;
			if (economic_sql_currency_writer_guard::acquire(legacy_connection,
									&denied) != EPERM)
				throw std::runtime_error(
					"legacy currency writer was not closed after baseline selection");
		}
		mysql_close(legacy_connection);
		assert_scalar(setup, "SELECT copper FROM player_data WHERE pid=21001", "7");
		{
			economic_sql_lifecycle_guard admission_probe;
			const std::string boot_lock_free = std::string("SELECT IS_FREE_LOCK('") +
							   ECONOMIC_SQL_BOOT_MAINTENANCE_LOCK_NAME +
							   "')";
			if (economic_sql_lifecycle_guard::acquire_runtime(
				    runtime_connection, &admission_probe) != EPERM)
				throw std::runtime_error(
					"runtime boot was admitted with an unactivated staged installation");
			assert_scalar(setup, boot_lock_free, "1");
			if (economic_sql_lifecycle_guard::acquire_runtime(
				    runtime_connection, &admission_probe) != EPERM)
				throw std::runtime_error(
					"staged refusal changed the runtime guard output or local admission state");
			assert_scalar(setup, boot_lock_free, "1");

			// An active epoch with no retained installation receipt is not a legacy
			// database: ordinary runtime admission must still fail closed.
			execute(setup,
				"DELETE FROM economic_sql_lifecycle_installation WHERE operation_id=" +
					sql_id(request.operation_id));
			execute(setup, "UPDATE economic_lineage_state SET active_epoch=" +
					       sql_id(request.epoch) +
					       " WHERE lineage=" + sql_id(request.lineage));
			assert_scalar(
				setup,
				"SELECT COUNT(*) FROM economic_sql_lifecycle_installation WHERE phase IN (1,2)",
				"0");
			assert_scalar(
				setup,
				"SELECT COUNT(*) FROM economic_lineage_state WHERE active_epoch IS NOT NULL",
				"1");
			if (economic_sql_lifecycle_guard::acquire_runtime(
				    runtime_connection, &admission_probe) != EPERM)
				throw std::runtime_error(
					"runtime boot was admitted with active_epoch and no staged installation receipt");
			assert_scalar(setup, boot_lock_free, "1");
			if (economic_sql_lifecycle_guard::acquire_runtime(
				    runtime_connection, &admission_probe) != EPERM)
				throw std::runtime_error(
					"active-epoch refusal changed the runtime guard output or local admission state");
			assert_scalar(setup, boot_lock_free, "1");
			execute(setup,
				"UPDATE economic_lineage_state SET active_epoch=NULL WHERE lineage=" +
					sql_id(request.lineage));
			{
				const auto recovered =
					economic_sql_lifecycle_guard::acquire_runtime(
						runtime_connection, &admission_probe);
				if (recovered)
					throw std::runtime_error(
						"runtime guard did not recover after refused admission: " +
						std::to_string(recovered));
			}
		}
		puts("PASS native_wallets=2 shared_banks=2 keeper_treasuries=1 durable_mappings=5 baseline_receipt=verified partial_write_rollback=verified phase_one_resume=verified exact_replay=stable concurrent_legacy_writer=serialized legacy_gate=closed healthy_inactive_runtime=admitted staged_runtime=refused active_epoch_without_receipt=refused failure_cleanup=verified output_preserved=verified");
		mysql_close(runtime_connection);
		mysql_close(owner_connection);
		mysql_close(setup);
		mysql_library_end();
		return 0;
	}
	catch (const std::exception &error)
	{
		if (runtime_connection)
			mysql_close(runtime_connection);
		if (owner_connection)
			mysql_close(owner_connection);
		if (setup)
			mysql_close(setup);
		fprintf(stderr, "HARNESS-ERROR %s\n", error.what());
		mysql_library_end();
		return 2;
	}
}

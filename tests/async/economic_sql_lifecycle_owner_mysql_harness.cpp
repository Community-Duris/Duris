#include "economy/auction_settlement_accounting.h"
#include "economy/auction_repository.h"
#include "economy/auction_item_claim_accounting.h"
#include "economy/auction_money_claim_accounting.h"
#include "persistence/economic_sql_auction_settlement_transaction.h"
#include "persistence/economic_sql_auction_item_claim_transaction.h"
#include "persistence/economic_sql_auction_money_claim_transaction.h"
#include <openssl/sha.h>
#include "economy/economic_baseline_adapter.h"
#include "economy/economic_gameplay_authority.h"
#include "economy/economic_sql_source_normalize.h"
#include "core/defines.h"
#include "persistence/economic_sql_accounting_lifecycle_transaction.h"
#include "persistence/economic_sql_lifecycle_guard.h"
#include "persistence/economic_sql_source_snapshot.h"
#include "persistence/economic_sql_pending_claim_source.h"
#include "persistence/economic_sql_baseline_transaction.h"
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
#include <string_view>
#include <thread>
#include <type_traits>
#include <vector>
#include <tuple>
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

unsigned int verify_synthetic_routes(MYSQL *, const economic_sql_lifecycle_request &,
				     const economic_sql_activation_evidence &evidence,
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

static std::string fixture_bytes(std::span<const uint8_t> data)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string text = "X'";
	for (auto byte : data)
	{
		text += digits[byte >> 4];
		text += digits[byte & 15];
	}
	return text + "'";
}
static std::string fixture_hash(std::span<const uint8_t> data)
{
	std::array<uint8_t, 32> hash{};
	SHA256(data.data(), data.size(), hash.data());
	return fixture_bytes(hash);
}
static void retain_auction_inbox(MYSQL *db, const critical_command &command)
{
	std::vector<uint8_t> bytes, keys;
	if (critical_command_encode(command, &bytes) != critical_command_codec_result::ok)
		throw std::runtime_error("fixture retained command invalid");
	for (const auto &key : command.keys)
	{
		keys.push_back(static_cast<uint8_t>(key.type));
		for (unsigned shift = 0; shift < 8; ++shift)
			keys.push_back(static_cast<uint8_t>(key.id >> (shift * 8)));
	}
	execute(db,
		"INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,command_type,"
		"schema_version,payload_version,status,result_payload) VALUES(" +
			sql_id(command.operation_id) + "," + fixture_hash(bytes) + "," +
			fixture_hash(keys) + "," +
			std::to_string(static_cast<uint16_t>(command.type)) + "," +
			std::to_string(command.schema_version) + "," +
			std::to_string(command.payload_version) + ",0,X'')");
}
static void retain_auction_result(MYSQL *db, const critical_command &command,
				  const auction_command_result &result)
{
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> encoded{};
	if (!auction_command_encode_result(result, &encoded))
		throw std::runtime_error("fixture result invalid");
	execute(db, "UPDATE critical_operation_inbox SET status=1,result_code=0,durable_revision=" +
			    std::to_string(
				    std::max({ result.auction_revision, result.wallet_revision,
					       result.bank_revision, result.player_owner_revision,
					       result.auction_owner_revision })) +
			    ",result_payload=" + fixture_bytes(encoded) +
			    ",committed_at=CURRENT_TIMESTAMP(6) WHERE operation_id=" +
			    sql_id(command.operation_id) + " AND status=0");
	if (mysql_affected_rows(db) != 1)
		throw std::runtime_error("fixture receipt not retained");
}
static critical_command historical_bid()
{
	auction_command_payload payload{};
	payload.action = auction_action::bid;
	payload.auction_id = 7002;
	payload.actor_pid = 21001;
	std::strcpy(payload.account_name.data(), "lifecycle_a");
	std::strcpy(payload.actor_name.data(), "LifecycleOne");
	payload.value = 500;
	payload.expected_wallet_revision = 5;
	payload.expected_bank_revision = 3;
	critical_command command{};
	if (!auction_command_build(&command, ident(3), payload, critical_source_site::command,
				   critical_deadline_class::interactive))
		throw std::runtime_error("original fixture bid invalid");
	command.accepted_at_usec = 123456;
	return command;
}
// Native cells are still at the genuine maintained opening cut here. Publish
// the observed decimal cells only after matching the retained original digest.
static void opening_reference(MYSQL *db, const economic_sql_lifecycle_request &request,
			      const economic_sql_lifecycle_receipt &receipt,
			      const economic_baseline_holding &holding, size_t index, uint64_t pid)
{
	const auto raw = [](const std::string &sql) { return sql.substr(2, sql.size() - 3); };
	const auto number = [](std::vector<uint8_t> &bytes, uint64_t value)
	{
		for (size_t i = 0; i < 8; ++i)
			bytes.push_back(static_cast<uint8_t>(value >> (8 * i)));
	};
	const auto text = [&](std::vector<uint8_t> &bytes, std::string_view value)
	{
		number(bytes, value.size());
		bytes.insert(bytes.end(), value.begin(), value.end());
	};
	const auto cells =
		rows(db, "SELECT pid,money,claim_revision FROM auction_money_pickups WHERE pid=" +
				 std::to_string(pid));
	if (cells.size() != 1 || cells[0].size() != 3 || cells[0][0] != std::to_string(pid) ||
	    cells[0][1] != std::to_string(holding.balance[0]) ||
	    cells[0][2] != std::to_string(holding.native_revision))
		throw std::runtime_error("reference cut differs from original native holding");
	std::vector<uint8_t> definition;
	text(definition, "ESD1");
	text(definition, "auction_money_pickups");
	text(definition, "pid");
	number(definition, 3);
	for (const auto column : { "pid", "money", "claim_revision" })
		text(definition, column);
	std::array<uint8_t, 32> definition_digest{};
	SHA256(definition.data(), definition.size(), definition_digest.data());
	std::vector<uint8_t> row;
	text(row, "ESR1");
	row.insert(row.end(), definition_digest.begin(), definition_digest.end());
	for (const auto &cell : cells[0])
	{
		number(row, 1);
		text(row, cell);
	}
	if (fixture_hash(row) != fixture_bytes(holding.source_digest))
		throw std::runtime_error(
			"native reference row does not bind original source digest");
	std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> account{};
	if (economic_account_key_encode(holding.account, &account) != economic_accounting_error::ok)
		throw std::runtime_error("native reference account encoding failed");
	std::vector<uint8_t> inputs;
	const std::string_view domain = "DURIS-SQL-LIFECYCLE-V2";
	inputs.insert(inputs.end(), domain.begin(), domain.end());
	for (const auto &value : { request.operation_id, request.lineage, request.epoch })
	{
		number(inputs, value.bytes.size());
		inputs.insert(inputs.end(), value.bytes.begin(), value.bytes.end());
	}
	number(inputs, request.actor_id);
	number(inputs, request.accepted_at_usec);
	const auto request_hash = scalar(
		db,
		"SELECT LOWER(HEX(request_digest)) FROM economic_sql_lifecycle_installation WHERE operation_id=" +
			sql_id(request.operation_id));
	if (request_hash != raw(fixture_hash(inputs)) ||
	    request_hash !=
		    scalar(db,
			   "SELECT LOWER(HEX(command_hash)) FROM critical_operation_inbox WHERE operation_id=" +
				   sql_id(request.operation_id)))
		throw std::runtime_error("original V2 lifecycle reference hash mismatch");
	const auto marker = scalar(
		db,
		"SELECT claim_origin_version FROM economic_baseline_witness WHERE operation_id=" +
			sql_id(receipt.baseline_operation_id));
	const auto slot =
		holding.balance[0] ?
			scalar(db,
			       "SELECT source_slot FROM economic_pending_claim_source WHERE source_operation_id=" +
				       sql_id(receipt.baseline_operation_id) +
				       " AND claim_mapping_id=" +
				       std::to_string(holding.account.authority_id)) :
			"null";
	if (marker != "1" || (holding.balance[0] && slot != std::to_string(index + 1)))
		throw std::runtime_error("original reference marker/lot ordinal mismatch");
	std::printf(
		"OPENING_ORIGINAL_REFERENCE {\"account_key\":\"%s\",\"native_revision\":\"%llu\",\"source_digest\":\"%s\",\"pid\":\"%s\",\"money\":\"%s\",\"claim_revision\":\"%s\",\"definition_framed_hex\":\"%s\",\"definition_digest\":\"%s\",\"row_framed_hex\":\"%s\",\"row_digest\":\"%s\",\"lifecycle_inputs_hex\":\"%s\",\"lifecycle_request_hash\":\"%s\",\"preparation_id\":\"%s\",\"lineage\":\"%s\",\"epoch\":\"%s\",\"actor_id\":\"%llu\",\"accepted_at_usec\":\"%llu\",\"baseline_operation\":\"%s\",\"origin_marker\":1,\"lot_slot\":%s}\n",
		raw(fixture_bytes(account)).c_str(),
		static_cast<unsigned long long>(holding.native_revision),
		raw(fixture_bytes(holding.source_digest)).c_str(), cells[0][0].c_str(),
		cells[0][1].c_str(), cells[0][2].c_str(), raw(fixture_bytes(definition)).c_str(),
		raw(fixture_bytes(definition_digest)).c_str(), raw(fixture_bytes(row)).c_str(),
		raw(fixture_hash(row)).c_str(), raw(fixture_bytes(inputs)).c_str(),
		request_hash.c_str(), raw(fixture_bytes(request.operation_id.bytes)).c_str(),
		raw(fixture_bytes(request.lineage.bytes)).c_str(),
		raw(fixture_bytes(request.epoch.bytes)).c_str(),
		static_cast<unsigned long long>(request.actor_id),
		static_cast<unsigned long long>(request.accepted_at_usec),
		raw(fixture_bytes(receipt.baseline_operation_id.bytes)).c_str(), slot.c_str());
	std::fflush(stdout);
}
static void money_recovery_journey(MYSQL *setup, MYSQL *owner,
				   economic_sql_lifecycle_guard &maintenance,
				   const economic_sql_lifecycle_request &request,
				   const economic_sql_lifecycle_receipt &receipt)
{
	if (!maintenance.release())
		throw std::runtime_error("money recovery maintenance release failed");
	const std::string journal = required("ECONOMIC_SQL_LIFECYCLE_JOURNAL_DIR");
	if (!critical_command_coordinator_init(journal.c_str(), no_gameplay_apply, nullptr, 1))
		throw std::runtime_error("money recovery coordinator failed");
	economic_sql_activation_evidence coverage;
	coverage.manifest_digest.fill(0x44);
	coverage.audit_digest.fill(0x55);
	coverage.route_count = coverage.verified_route_count = 3;
	auto cutover = [&](bool pause)
	{
		economic_sql_lifecycle_guard guard;
		economic_sql_cutover_capability lease;
		economic_sql_cutover_transaction_owner transaction;
		if (economic_sql_lifecycle_guard::acquire_maintenance(owner, &guard) ||
		    !guard.acquire_cutover_capability(3000, &lease) ||
		    !transaction.begin(guard, lease))
			throw std::runtime_error("money recovery cutover acquisition failed");
		const auto code =
			pause ? economic_sql_accounting_lifecycle_transaction::pause(
					owner, transaction, request.lineage) :
				economic_sql_accounting_lifecycle_transaction::activate_verified(
					owner, transaction, request, coverage,
					verify_synthetic_routes);
		if (!(code ? transaction.rollback() : transaction.commit()))
			throw std::runtime_error("money recovery cutover finish failed");
		return code;
	};
	if (cutover(false))
		throw std::runtime_error("money opening activation failed");
	size_t recovery_ordinal = 0;
	auto recover = [&](unsigned int expected)
	{
		auto *runtime = connect_fixture();
		{
			economic_sql_lifecycle_guard authority;
			const auto acquisition =
				economic_sql_lifecycle_guard::acquire_runtime(runtime, &authority);
			++recovery_ordinal;
			if (acquisition)
				throw std::runtime_error(
					"money recovery runtime authority refused code=" +
					std::to_string(acquisition) +
					" ordinal=" + std::to_string(recovery_ordinal) +
					" expected=" + std::to_string(expected));
			bool active = false;
			const auto code =
				economic_sql_accounting_lifecycle_transaction::recover_runtime(
					runtime, authority, &active);
			if (code != expected || (!expected && !active) || (expected && active))
				throw std::runtime_error(
					"money recovery code=" + std::to_string(code) +
					" expected=" + std::to_string(expected) +
					" ordinal=" + std::to_string(recovery_ordinal));
			economic_gameplay_authority_test_access::reset();
		}
		mysql_close(runtime);
	};
	auto mapped = [&](uint16_t kind, uint64_t native)
	{
		return economic_account_key{
			request.lineage, static_cast<economic_account_kind>(kind),
			std::stoull(scalar(
				setup,
				"SELECT mapping_id FROM economic_account_mapping WHERE lineage=" +
					sql_id(request.lineage) +
					" AND account_kind=" + std::to_string(kind) +
					" AND native_id=" + std::to_string(native))),
			0
		};
	};
	auto corrupt = [&](const std::string &change, const std::string &repair,
			   unsigned int expected = EILSEQ)
	{
		execute(setup, change);
		recover(expected);
		if (cutover(true))
			throw std::runtime_error("corruption pause failed");
		if (cutover(false) != expected)
			throw std::runtime_error("corruption accepted paused reactivation");
		execute(setup, repair);
		if (cutover(false))
			throw std::runtime_error("repaired paused reactivation failed");
		recover(0);
	};
	recover(0);
	const auto source_root = sql_id(receipt.baseline_operation_id);
	const auto slot = scalar(
		setup,
		"SELECT source_slot FROM economic_pending_claim_source WHERE source_operation_id=" +
			source_root);
	corrupt("UPDATE economic_pending_claim_source SET source_slot=source_slot+100 WHERE source_operation_id=" +
			source_root,
		"UPDATE economic_pending_claim_source SET source_slot=" + slot +
			" WHERE source_operation_id=" + source_root);
	// Run the maintained actual settlement and item-claim owners. Initial rows
	// above are an explicitly seeded historical native boundary, not a listing
	// journey or independent release-verifier claim.
	auction_settlement_listing listing;
	listing.auction_id = 7002;
	listing.seller_pid = 21002;
	listing.winner_pid = 21001;
	listing.status = 1;
	listing.custody_state = 1;
	listing.quantity = listing.item_count = 1;
	listing.current_price = 500;
	listing.revision = 2;
	listing.end_time = 2;
	listing.listing_operation = ident(4);
	listing.winning_bid_operation = ident(3);
	listing.items[0] = { 7, 1, 0, 501, 0, false };
	auction_settlement_accounts keys;
	keys.escrow = mapped(4, 7002);
	keys.seller_claim = mapped(5, 21002);
	auction_command_payload payload{};
	payload.action = auction_action::finalize;
	payload.auction_id = 7002;
	critical_command settle{};
	if (!auction_command_build(&settle, ident(21), payload, critical_source_site::command,
				   critical_deadline_class::interactive))
		throw std::runtime_error("fixture settlement command invalid");
	settle.accepted_at_usec = 234567;
	if (auction_settlement_accounting_intent(settle, request.epoch, listing, keys,
						 &settle.accounting_intent) !=
	    economic_accounting_error::ok)
		throw std::runtime_error("fixture settlement intent invalid");
	settle.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	execute(setup, "START TRANSACTION");
	retain_auction_inbox(setup, settle);
	economic_sql_auction_settlement_context settlement_context;
	auction_command_result result{};
	unsigned int code = 0;
	bool applied = false;
	const auto lock_code =
		economic_sql_auction_settlement_lock(setup, settle, &settlement_context);
	const auto execute_code =
		lock_code ? 0 :
			    economic_sql_auction_settlement_execute_and_record(
				    setup, settle, settlement_context, &result, &code, &applied);
	if (lock_code || execute_code || code || !applied)
		throw std::runtime_error(
			"actual settlement owner failed lock=" + std::to_string(lock_code) +
			" execute=" + std::to_string(execute_code) +
			" result=" + std::to_string(code) + " applied=" + std::to_string(applied) +
			" sql=" + mysql_error(setup));
	retain_auction_result(setup, settle, result);
	execute(setup, "COMMIT");
	recover(0);
	auction_item_claim_state item;
	item.auction_id = 7002;
	item.seller_pid = 21002;
	item.winner_pid = item.claimant_pid = 21001;
	item.status = 2;
	item.custody_state = 1;
	item.auction_revision = 3;
	item.listing_operation = ident(4);
	item.claim_source_operation = settle.operation_id;
	item.item_count = 1;
	item.rows[0] = { 7, 1, 0, 501, 21001, false };
	payload = {};
	payload.action = auction_action::claim_item;
	payload.auction_id = 7002;
	payload.actor_pid = 21001;
	std::strcpy(payload.account_name.data(), "lifecycle_a");
	std::strcpy(payload.actor_name.data(), "LifecycleOne");
	payload.expected_wallet_revision = std::stoull(
		scalar(setup, "SELECT wallet_revision FROM player_data WHERE pid=21001"));
	payload.expected_bank_revision = std::stoull(scalar(
		setup,
		"SELECT bank_revision FROM account_banks WHERE account_name='lifecycle_a' AND racewar=0"));
	payload.item_count = 1;
	payload.items[0] = { 7, 1, 501 };
	critical_command collection{};
	if (!auction_command_build(&collection, ident(22), payload, critical_source_site::command,
				   critical_deadline_class::interactive))
		throw std::runtime_error("fixture item claim invalid");
	collection.accepted_at_usec = 345678;
	if (auction_item_claim_accounting_intent(
		    collection, request.epoch, mapped(1, 21001), receipt.banks[0].account, item,
		    &collection.accounting_intent) != economic_accounting_error::ok)
		throw std::runtime_error("fixture item intent invalid");
	collection.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	execute(setup, "START TRANSACTION");
	retain_auction_inbox(setup, collection);
	economic_sql_auction_item_claim_context item_context;
	const auto item_lock_code =
		economic_sql_auction_item_claim_lock(setup, collection, &item_context);
	const auto item_execute_code =
		item_lock_code ? 0 :
				 economic_sql_auction_item_claim_execute_and_record(
					 setup, collection, item_context, &result, &code, &applied);
	if (item_lock_code || item_execute_code || code || !applied)
		throw std::runtime_error(
			"actual item collection failed lock=" + std::to_string(item_lock_code) +
			" execute=" + std::to_string(item_execute_code) +
			" result=" + std::to_string(code) + " applied=" + std::to_string(applied) +
			" sql=" + mysql_error(setup));
	retain_auction_result(setup, collection, result);
	execute(setup, "COMMIT");
	assert_scalar(setup, "SELECT auction_revision FROM auctions WHERE id=7002", "4");
	recover(0);
	// Genuine committed successor receipt from the actual owner, before corruptions.
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> item_original_bytes{};
	if (!auction_command_encode_result(result, &item_original_bytes) ||
	    result.auction_revision != 4 || result.item_count != 1 || result.item_uids[0] != 7 ||
	    result.item_revisions[0] != 2 || item_original_bytes.back())
		throw std::runtime_error(
			"actual item successor result differs from original native cut");
	const auto item_original_durable =
		std::max({ result.auction_revision, result.wallet_revision, result.bank_revision,
			   result.player_owner_revision, result.auction_owner_revision });
	if (item_original_durable != 8 || item_original_durable <= result.auction_revision)
		throw std::runtime_error("actual successor does not exercise original durable max");
	const auto item_scope = " WHERE operation_id=" + sql_id(collection.operation_id);
	const auto item_original_sql = fixture_bytes(item_original_bytes);
	const auto item_original_hex = item_original_sql.substr(2, item_original_sql.size() - 3);
	assert_scalar(setup,
		      "SELECT LOWER(HEX(result_payload)) FROM critical_operation_inbox" +
			      item_scope,
		      item_original_hex);
	assert_scalar(setup, "SELECT durable_revision FROM critical_operation_inbox" + item_scope,
		      std::to_string(item_original_durable));
	std::printf(
		"ITEMCLAIM_ACTUAL_RETAINED_REFERENCE {\"operation_id\":\"%s\",\"durable_revision\":\"%llu\",\"auction_revision\":4,\"wallet_revision\":\"%llu\",\"bank_revision\":\"%llu\",\"player_owner_revision\":\"%llu\",\"auction_owner_revision\":\"%llu\",\"canonical_result_320_hex\":\"%s\"}\n",
		sql_id(collection.operation_id).c_str(),
		static_cast<unsigned long long>(item_original_durable),
		static_cast<unsigned long long>(result.wallet_revision),
		static_cast<unsigned long long>(result.bank_revision),
		static_cast<unsigned long long>(result.player_owner_revision),
		static_cast<unsigned long long>(result.auction_owner_revision),
		item_original_hex.c_str());
	std::fflush(stdout);
	corrupt("UPDATE critical_operation_inbox SET durable_revision=" +
			std::to_string(result.auction_revision) + item_scope,
		"UPDATE critical_operation_inbox SET durable_revision=" +
			std::to_string(item_original_durable) + item_scope);
	auto bad_original_bytes = item_original_bytes;
	bad_original_bytes.back() = 1;
	const auto restore_original = "UPDATE critical_operation_inbox SET result_payload=" +
				      item_original_sql + item_scope;
	corrupt("UPDATE critical_operation_inbox SET result_payload=" +
			fixture_bytes(bad_original_bytes) + item_scope,
		restore_original);
	for (const bool wrong_uid : { true, false })
	{
		auto bad_original = result;
		if (wrong_uid)
			++bad_original.item_uids[0];
		else
			++bad_original.item_revisions[0];
		if (!auction_command_encode_result(bad_original, &bad_original_bytes))
			throw std::runtime_error("canonical successor corruption did not encode");
		corrupt("UPDATE critical_operation_inbox SET result_payload=" +
				fixture_bytes(bad_original_bytes) + item_scope,
			restore_original);
	}
	assert_scalar(setup,
		      "SELECT LOWER(HEX(result_payload)) FROM critical_operation_inbox" +
			      item_scope,
		      item_original_hex);
	assert_scalar(setup, "SELECT durable_revision FROM critical_operation_inbox" + item_scope,
		      std::to_string(item_original_durable));
	puts("PASS actual itemclaim original durable-max, unused tail, UID and revision corruptions refused with exact restoration");

	if (cutover(true) || cutover(false))
		throw std::runtime_error("settled and collected paused reactivation failed");
	corrupt("UPDATE auctions SET auction_revision=5 WHERE id=7002",
		"UPDATE auctions SET auction_revision=4 WHERE id=7002");
	corrupt("UPDATE auction_ledger SET auction_revision=5 WHERE operation_id=" +
			sql_id(collection.operation_id),
		"UPDATE auction_ledger SET auction_revision=4 WHERE operation_id=" +
			sql_id(collection.operation_id));
	corrupt("UPDATE auction_ledger SET event_type=2 WHERE operation_id=" +
			sql_id(collection.operation_id),
		"UPDATE auction_ledger SET event_type=6 WHERE operation_id=" +
			sql_id(collection.operation_id));
	auction_money_claim_state claim;
	claim.beneficiary_pid = 21001;
	claim.money = 500;
	claim.revision = 3;
	claim.sources = { { receipt.baseline_operation_id, static_cast<uint16_t>(std::stoul(slot)),
			    21001, mapped(5, 21001).authority_id, 500 } };
	payload = {};
	payload.action = auction_action::claim_money;
	payload.actor_pid = 21001;
	std::strcpy(payload.account_name.data(), "lifecycle_a");
	std::strcpy(payload.actor_name.data(), "LifecycleOne");
	payload.expected_wallet_revision = std::stoull(
		scalar(setup, "SELECT wallet_revision FROM player_data WHERE pid=21001"));
	payload.expected_bank_revision = std::stoull(scalar(
		setup,
		"SELECT bank_revision FROM account_banks WHERE account_name='lifecycle_a' AND racewar=0"));
	critical_command cashout{};
	if (!auction_command_build(&cashout, ident(23), payload, critical_source_site::command,
				   critical_deadline_class::interactive))
		throw std::runtime_error("fixture cashout invalid");
	cashout.accepted_at_usec = 456789;
	if (auction_money_claim_accounting_intent(cashout, request.epoch, mapped(1, 21001),
						  receipt.banks[0].account, mapped(5, 21001), claim,
						  &cashout.accounting_intent) !=
	    economic_accounting_error::ok)
		throw std::runtime_error("fixture cashout intent invalid");
	cashout.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	execute(setup, "START TRANSACTION");
	retain_auction_inbox(setup, cashout);
	economic_sql_auction_money_claim_context money_context;
	const auto money_lock_code =
		economic_sql_auction_money_claim_lock(setup, cashout, &money_context);
	const auto money_execute_code =
		money_lock_code ? 0 :
				  economic_sql_auction_money_claim_execute_and_record(
					  setup, cashout, money_context, &result, &code, &applied);
	if (money_lock_code || money_execute_code || code || !applied)
		throw std::runtime_error(
			"actual full cashout failed lock=" + std::to_string(money_lock_code) +
			" execute=" + std::to_string(money_execute_code) +
			" result=" + std::to_string(code) + " applied=" + std::to_string(applied) +
			" sql=" + mysql_error(setup));
	retain_auction_result(setup, cashout, result);
	execute(setup, "COMMIT");
	assert_scalar(setup, "SELECT money FROM auction_money_pickups WHERE pid=21001", "0");
	recover(0);
	const auto witness_scope = " WHERE operation_id=" + source_root;
	corrupt("UPDATE economic_baseline_witness SET claim_origin_version=NULL" + witness_scope,
		"UPDATE economic_baseline_witness SET claim_origin_version=1" + witness_scope,
		ENODATA);
	corrupt("UPDATE economic_baseline_witness SET command_accepted_at_usec=NULL" +
			witness_scope,
		"UPDATE economic_baseline_witness SET command_accepted_at_usec=" +
			std::to_string(request.accepted_at_usec) + witness_scope);
	execute(setup,
		"CREATE TEMPORARY TABLE opening_source_backup AS SELECT * FROM economic_pending_claim_source WHERE source_operation_id=" +
			source_root);
	execute(setup,
		"CREATE TEMPORARY TABLE opening_consumption_backup AS SELECT * FROM economic_pending_claim_consumption WHERE source_operation_id=" +
			source_root);
	execute(setup, "START TRANSACTION");
	execute(setup, "DELETE FROM economic_pending_claim_consumption WHERE source_operation_id=" +
			       source_root);
	execute(setup, "DELETE FROM economic_pending_claim_source WHERE source_operation_id=" +
			       source_root);
	execute(setup, "COMMIT");
	recover(EILSEQ);
	if (cutover(true) || cutover(false) != EILSEQ)
		throw std::runtime_error("omitted consumed origin accepted at reactivation");
	const auto paused_origin_proof = [&]
	{
		auto *runtime = connect_fixture();
		{
			economic_sql_lifecycle_guard refused;
			const auto acquisition =
				economic_sql_lifecycle_guard::acquire_runtime(runtime, &refused);
			if (acquisition != EPERM)
				throw std::runtime_error(
					"paused original runtime guard accepted origin test code=" +
					std::to_string(acquisition));
		}
		mysql_close(runtime);
		execute(setup, "START TRANSACTION");
		economic_baseline_batch retained;
		const auto proof = economic_sql_baseline_verify_known_retained_in_transaction(
			setup, receipt.baseline_operation_id, &retained);
		execute(setup, "ROLLBACK");
		if (proof != ENODATA)
			throw std::runtime_error("paused genuine original lot policy proof code=" +
						 std::to_string(proof));
	};
	execute(setup,
		"UPDATE economic_baseline_witness SET claim_origin_version=NULL" + witness_scope);
	paused_origin_proof();
	if (cutover(false) != ENODATA)
		throw std::runtime_error("NULL marker hid missing fully consumed original lots");
	execute(setup, "UPDATE economic_baseline_witness SET command_accepted_at_usec=NULL" +
			       witness_scope);
	paused_origin_proof();
	if (cutover(false) != ENODATA)
		throw std::runtime_error(
			"NULL timestamp/marker hid genuine money lifecycle origin");
	execute(setup,
		"UPDATE economic_baseline_witness SET claim_origin_version=1,command_accepted_at_usec=" +
			std::to_string(request.accepted_at_usec) + witness_scope);
	execute(setup, "START TRANSACTION");
	execute(setup,
		"INSERT INTO economic_pending_claim_source SELECT * FROM opening_source_backup");
	execute(setup,
		"INSERT INTO economic_pending_claim_consumption SELECT * FROM opening_consumption_backup");
	execute(setup, "COMMIT");
	if (cutover(false))
		throw std::runtime_error("restored consumed opening did not reactivate");
	recover(0);
	critical_command_coordinator_shutdown();
	puts("PASS scoped actual settlement/itemclaim/fullcashout recovery; slot and omitted-consumed-origin refused; independent activation verifier remains separate");
}

static int money_opening_fixture(bool recovery = false)
{
	MYSQL *setup = nullptr, *owner = nullptr;
	try
	{
		if (mysql_library_init(0, nullptr, nullptr))
			throw std::runtime_error("mysql_library_init failed");
		setup = connect_fixture();
		seed(setup);
		assert_scalar(setup,
			      "SELECT COUNT(*) FROM critical_operation_inbox WHERE operation_id=" +
				      sql_id(ident(4)),
			      "0");
		std::printf("NATIVE_LISTING_ORIGINAL_RECEIPT before_count=0 operation=%s\n",
			    sql_id(ident(4)).c_str());
		// The unrelated legacy listing remains an explicitly historical opening boundary.
		execute(setup,
			"INSERT INTO auctions(id,seller_pid,status,winning_bidder_pid,cur_price,buy_price,quantity,auction_revision,obj_blob_str) VALUES(7001,21002,'OPEN',0,9000,10000,1,1,'')");
		execute(setup, "UPDATE item_uid_allocator SET next_uid=8 WHERE allocator_id=1");
		execute(setup,
			"INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) VALUES(1,21002,0,0)");
		execute(setup,
			"INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,item_revision,vnum,state) VALUES(7,7,NULL,1,21002,0,0,501,1)");
		auction_command_payload native_listing{};
		native_listing.action = auction_action::list;
		native_listing.actor_pid = 21002;
		native_listing.racewar = 1;
		std::strcpy(native_listing.account_name.data(), "lifecycle_b");
		std::strcpy(native_listing.actor_name.data(), "LifecycleTwo");
		native_listing.expected_wallet_revision = 8;
		native_listing.expected_bank_revision = 9;
		native_listing.start_price = 500;
		native_listing.listing_fee = 100;
		native_listing.end_time = 2;
		native_listing.item_count = 1;
		native_listing.items[0] = { 7, 0, 501 };
		native_listing.object_blob[0] = 'x';
		native_listing.object_blob_size = 1;
		critical_command listing_command{};
		if (!auction_command_build(&listing_command, ident(4), native_listing,
					   critical_source_site::command,
					   critical_deadline_class::interactive))
			throw std::runtime_error("actual native listing command invalid");
		listing_command.accepted_at_usec = 100000;
		const auto native_apply = [&](const critical_command &command)
		{
			execute(setup, "START TRANSACTION");
			retain_auction_inbox(setup, command);
			auction_command_result actual{};
			unsigned int code = 0;
			bool applied = false;
			if (!auction_repository_execute(setup, command, &actual, &code, &applied) ||
			    code || !applied)
				throw std::runtime_error(
					"actual native auction producer refused code=" +
					std::to_string(code) + " sql=" + mysql_error(setup));
			retain_auction_result(setup, command, actual);
			const auto durable =
				std::max({ actual.auction_revision, actual.wallet_revision,
					   actual.bank_revision, actual.player_owner_revision,
					   actual.auction_owner_revision });
			execute(setup, "UPDATE critical_operation_inbox SET durable_revision=" +
					       std::to_string(durable) + " WHERE operation_id=" +
					       sql_id(command.operation_id));
			execute(setup, "COMMIT");
			return actual;
		};
		const auto listed = native_apply(listing_command);
		if (listed.auction_id != 7002 || listed.auction_revision != 1 ||
		    listed.event_type != auction_event_type::listed ||
		    listed.wallet_value_delta != -100 || listed.item_uids[0] != 7 ||
		    listed.item_revisions[0] != 1)
			throw std::runtime_error(
				"actual native listing result differs from recovery cut");
		assert_scalar(
			setup,
			"SELECT COUNT(*) FROM critical_operation_inbox WHERE operation_id=" +
				sql_id(ident(4)) +
				" AND status=1 AND result_code=0 AND committed_at IS NOT NULL AND OCTET_LENGTH(result_payload)=320",
			"1");
		assert_scalar(setup,
			      "SELECT COUNT(*) FROM auction_ledger WHERE operation_id=" +
				      sql_id(ident(4)) + " AND event_type=1 AND auction_id=7002",
			      "1");
		assert_scalar(setup,
			      "SELECT COUNT(*) FROM currency_ledger WHERE operation_id=" +
				      sql_id(ident(4)) + " AND pid=21002",
			      "1");
		std::printf(
			"NATIVE_LISTING_ORIGINAL_RECEIPT after_count=1 operation=%s auction=7002 item=7 revision=1\n",
			sql_id(ident(4)).c_str());
		// A real native bid uses original claim credit and leaves the exact 500/revision3 opening.
		execute(setup,
			"INSERT INTO auction_money_pickups(pid,money,claim_revision) VALUES(21001,1000,2),(21002,0,0)");
		auto prior_bid = historical_bid();
		auction_command_payload bid_payload{};
		if (!auction_command_decode_payload(prior_bid, &bid_payload))
			throw std::runtime_error("original bid payload invalid");
		bid_payload.expected_wallet_revision = 6;
		bid_payload.expected_bank_revision = 4;
		if (!auction_command_build(&prior_bid, ident(3), bid_payload,
					   critical_source_site::command,
					   critical_deadline_class::interactive))
			throw std::runtime_error("actual prior native bid command invalid");
		prior_bid.accepted_at_usec = 123456;
		const auto bid_result = native_apply(prior_bid);
		if (bid_result.auction_id != 7002 || bid_result.auction_revision != 2 ||
		    bid_result.final_price != 500 || bid_result.claim_credit_used != 500 ||
		    bid_result.event_type != auction_event_type::bid_placed)
			throw std::runtime_error(
				"actual prior native bid differs from opening cut");
		assert_scalar(setup, "SELECT money FROM auction_money_pickups WHERE pid=21001",
			      "500");
		assert_scalar(setup,
			      "SELECT claim_revision FROM auction_money_pickups WHERE pid=21001",
			      "3");

		owner = connect_fixture();
		economic_sql_lifecycle_guard maintenance;
		if (economic_sql_lifecycle_guard::acquire_maintenance(owner, &maintenance))
			throw std::runtime_error("money opening maintenance acquire failed");
		economic_sql_lifecycle_request request;
		request.operation_id = ident(19);
		request.lineage = ident(79);
		request.epoch = ident(119);
		request.actor_id = 9001;
		request.accepted_at_usec = 123456789;
		economic_sql_lifecycle_receipt receipt;
		receipt.operation_id = ident(212);
		// Missing0062 refuses before any staged mapping, receipt or baseline.
		execute(setup,
			"RENAME TABLE economic_pending_claim_consumption TO lifecycle_consumption_missing");
		const auto missing = economic_sql_accounting_lifecycle_transaction::install(
			owner, maintenance, request, &receipt);
		if (missing != 1146 || receipt.operation_id.bytes != ident(212).bytes)
			throw std::runtime_error("missing0062 did not refuse before opening");
		assert_scalar(setup,
			      "SELECT COUNT(*) FROM economic_account_mapping WHERE lineage=" +
				      sql_id(request.lineage),
			      "0");
		execute(setup,
			"RENAME TABLE lifecycle_consumption_missing TO economic_pending_claim_consumption");
		// Origin insertion belongs to the actual baseline root transaction.
		execute(setup,
			"CREATE TRIGGER lifecycle_origin_fault BEFORE INSERT ON economic_pending_claim_source "
			"FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='injected opening origin failure'");
		if (!economic_sql_accounting_lifecycle_transaction::install(owner, maintenance,
									    request, &receipt) ||
		    receipt.operation_id.bytes != ident(212).bytes)
			throw std::runtime_error(
				"opening origin fault did not preserve refusal receipt");
		assert_scalar(setup,
			      "SELECT COUNT(*) FROM economic_baseline_witness WHERE lineage=" +
				      sql_id(request.lineage),
			      "0");
		assert_scalar(setup,
			      "SELECT COUNT(*) FROM economic_accounting_operation WHERE lineage=" +
				      sql_id(request.lineage),
			      "0");
		assert_scalar(setup,
			      "SELECT COUNT(*) FROM economic_pending_claim_source WHERE lineage=" +
				      sql_id(request.lineage),
			      "0");
		assert_scalar(setup, "SELECT money FROM auction_money_pickups WHERE pid=21001",
			      "500");
		execute(setup, "DROP TRIGGER lifecycle_origin_fault");
		if (economic_sql_accounting_lifecycle_transaction::install(owner, maintenance,
									   request, &receipt))
			throw std::runtime_error(
				"complete money opening did not resume original phase1");
		if (receipt.wallets.size() != 2 || receipt.banks.size() != 2 ||
		    receipt.treasuries.size() != 1)
			throw std::runtime_error("domain mappings leaked into wallet exports");
		const auto witness_rows = rows(
			setup,
			"SELECT canonical_witness FROM economic_baseline_witness WHERE operation_id=" +
				sql_id(receipt.baseline_operation_id));
		std::vector<uint8_t> witness_bytes(witness_rows.at(0).at(0).begin(),
						   witness_rows.at(0).at(0).end());
		std::optional<economic_prepared_baseline> prepared;
		if (economic_baseline_decode(witness_bytes, &prepared) !=
			    economic_accounting_error::ok ||
		    !prepared)
			throw std::runtime_error("money witness did not decode");
		const auto &holdings = prepared->witness().holdings;
		if (holdings.size() != 11)
			throw std::runtime_error(
				"opening omitted an ordinary or escrow/claim holding");
		for (const auto &native : std::array<std::tuple<uint16_t, uint32_t, uint64_t>, 4>{
			     std::tuple<uint16_t, uint32_t, uint64_t>{ 4, 7001, 0 },
			     { 4, 7002, 500 },
			     { 5, 21001, 500 },
			     { 5, 21002, 0 } })
		{
			const auto [kind, pid, amount] = native;
			const auto mapped = rows(
				setup,
				"SELECT mapping_id FROM economic_account_mapping WHERE lineage=" +
					sql_id(request.lineage) +
					" AND account_kind=" + std::to_string(kind) +
					" AND native_id=" + std::to_string(pid));
			const auto mapping = std::stoull(mapped.at(0).at(0));
			const auto found = std::find_if(
				holdings.begin(), holdings.end(),
				[&](const auto &h) {
					return static_cast<uint16_t>(h.account.kind) == kind &&
					       h.account.authority_id == mapping;
				});
			if (found == holdings.end() ||
			    found->balance !=
				    economic_coin_vector{ static_cast<int64_t>(amount), 0, 0, 0 })
				throw std::runtime_error(
					"actual escrow/claim opening amount mismatch");
		}
		assert_scalar(
			setup,
			"SELECT COUNT(*) FROM economic_pending_claim_source WHERE source_operation_id=" +
				sql_id(receipt.baseline_operation_id) +
				" AND beneficiary_pid=21001 AND amount=500 AND claim_operation_id IS NULL",
			"1");
		assert_scalar(
			setup,
			"SELECT claim_origin_version FROM economic_baseline_witness WHERE operation_id=" +
				sql_id(receipt.baseline_operation_id),
			"1");
		// The original ESR1 digest fixes the native PID, including zero openings.
		// Matching a changed mapping and positive beneficiary cannot replace it.
		for (size_t index = 0; index < holdings.size(); ++index)
		{
			const auto &holding = holdings[index];
			if (holding.account.kind != economic_account_kind::pending_claim)
				continue;
			const auto map_scope =
				"mapping_id=" + std::to_string(holding.account.authority_id);
			const auto pid = std::stoull(scalar(
				setup, "SELECT native_id FROM economic_account_mapping WHERE " +
					       map_scope));
			opening_reference(setup, request, receipt, holding, index, pid);
			const auto source_scope =
				"source_operation_id=" + sql_id(receipt.baseline_operation_id) +
				" AND claim_mapping_id=" +
				std::to_string(holding.account.authority_id);
			execute(setup, "START TRANSACTION");
			if (economic_sql_pending_claim_source_verify_baseline(
				    setup, receipt.baseline_operation_id, prepared->witness()) ||
			    economic_sql_baseline_verify_known_retained_in_transaction(
				    setup, receipt.baseline_operation_id, nullptr))
				throw std::runtime_error("genuine original claim PID proof failed");
			execute(setup, "UPDATE economic_account_mapping SET native_id=" +
					       std::to_string(pid + 100) + ",active_native_id=" +
					       std::to_string(pid + 100) + " WHERE " + map_scope);
			if (mysql_affected_rows(setup) != 1)
				throw std::runtime_error("original claim mapping mutation missing");
			if (holding.balance[0])
			{
				execute(setup,
					"UPDATE economic_pending_claim_source SET beneficiary_pid=" +
						std::to_string(pid + 100) + " WHERE " +
						source_scope);
				if (mysql_affected_rows(setup) != 1)
					throw std::runtime_error(
						"positive original beneficiary mutation missing");
			}
			else
				assert_scalar(
					setup,
					"SELECT COUNT(*) FROM economic_pending_claim_source WHERE " +
						source_scope,
					"0");
			if (economic_sql_pending_claim_source_verify_baseline(
				    setup, receipt.baseline_operation_id, prepared->witness()) !=
				    EILSEQ ||
			    economic_sql_baseline_verify_known_retained_in_transaction(
				    setup, receipt.baseline_operation_id, nullptr) != EILSEQ)
				throw std::runtime_error(
					"joint claim PID/beneficiary corruption replaced frozen original digest");
			if (rows(setup,
				 "SELECT canonical_witness FROM economic_baseline_witness WHERE operation_id=" +
					 sql_id(receipt.baseline_operation_id)) != witness_rows)
				throw std::runtime_error(
					"PID control changed the original frozen witness");
			execute(setup, "ROLLBACK");
			execute(setup, "START TRANSACTION");
			if (economic_sql_pending_claim_source_verify_baseline(
				    setup, receipt.baseline_operation_id, prepared->witness()) ||
			    economic_sql_baseline_verify_known_retained_in_transaction(
				    setup, receipt.baseline_operation_id, nullptr))
				throw std::runtime_error(
					"restored original claim PID proof failed");
			execute(setup, "ROLLBACK");
		}
		puts("PASS original positive/zero frozen claim PID digest; joint mapping/beneficiary corruption refused");
		economic_sql_lifecycle_receipt replay;
		if (economic_sql_accounting_lifecycle_transaction::install(owner, maintenance,
									   request, &replay) ||
		    replay.baseline_operation_id.bytes != receipt.baseline_operation_id.bytes)
			throw std::runtime_error(
				"money opening original-ID replay changed identity");
		assert_scalar(setup,
			      "SELECT COUNT(*) FROM economic_pending_claim_source WHERE lineage=" +
				      sql_id(request.lineage),
			      "1");
		if (recovery)
		{
			money_recovery_journey(setup, owner, maintenance, request, receipt);
			if (!maintenance.release())
				throw std::runtime_error(
					"money recovery final guard release failed");
			mysql_close(owner);
			mysql_close(setup);
			mysql_library_end();
			return 0;
		}
		execute(setup,
			"UPDATE economic_baseline_witness SET claim_origin_version=NULL WHERE operation_id=" +
				sql_id(receipt.baseline_operation_id));
		replay.operation_id = ident(213);
		if (!economic_sql_accounting_lifecycle_transaction::install(owner, maintenance,
									    request, &replay) ||
		    replay.operation_id.bytes != ident(213).bytes)
			throw std::runtime_error("new-money marker downgrade was accepted");
		assert_scalar(setup, "SELECT money FROM auction_money_pickups WHERE pid=21001",
			      "500");
		assert_scalar(
			setup,
			"SELECT active_epoch IS NULL FROM economic_lineage_state WHERE lineage=" +
				sql_id(request.lineage),
			"1");
		puts("PASS scoped money opening native_owner=actual domains=4 baseline_origin_atomic=1 exact_replay=1 marker_downgrade_refused=1 activation=0");
		if (!maintenance.release())
			throw std::runtime_error("money opening final guard release failed");
		mysql_close(owner);
		mysql_close(setup);
		mysql_library_end();
		return 0;
	}
	catch (const std::exception &error)
	{
		if (owner)
			mysql_close(owner);
		if (setup)
			mysql_close(setup);
		mysql_library_end();
		fprintf(stderr, "MONEY-OPENING-ERROR %s\n", error.what());
		return 2;
	}
}

int main(int argc, char **argv)
{
	if (argc == 2 && !std::strcmp(argv[1], "--money-opening"))
		return money_opening_fixture();
	if (argc == 2 && !std::strcmp(argv[1], "--money-recovery"))
		return money_opening_fixture(true);
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

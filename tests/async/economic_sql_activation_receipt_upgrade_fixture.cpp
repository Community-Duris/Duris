#include "persistence/economic_sql_accounting_lifecycle_transaction.h"
#include "persistence/economic_sql_lifecycle_guard.h"
#include <mysql/mysql.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// Disposable upgrade fixture only: use the genuine lifecycle owner to create
// phase-2 staged baseline rows; this helper never creates a receipt or activates.
namespace
{
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
	if (std::getenv("DB_SOCKET") || std::strcmp(required("ENVIRONMENT"), "test") ||
	    std::strcmp(required("ECONOMIC_SQL_ACTIVATION_RECEIPT_DISPOSABLE_SCHEMA"), "1") ||
	    (host != "127.0.0.1" && host != "localhost" && host != "::1") ||
	    !schema.starts_with("economic_receipt_test_") ||
	    schema.size() <= std::strlen("economic_receipt_test_") || schema.size() > 64 ||
	    schema.find_first_not_of(
		    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") !=
		    std::string::npos)
		throw std::runtime_error("explicit disposable loopback schema required");

	char *end = nullptr;
	const auto port = std::strtoul(required("DB_PORT"), &end, 10);
	if (!end || *end || !port || port > 65535)
		throw std::runtime_error("invalid disposable DB port");

	MYSQL *connection = mysql_init(nullptr);
	if (!connection)
		throw std::runtime_error("mysql_init failed");
	unsigned int timeout = 5;
	unsigned int protocol = MYSQL_PROTOCOL_TCP;
	mysql_options(connection, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
	mysql_options(connection, MYSQL_OPT_READ_TIMEOUT, &timeout);
	mysql_options(connection, MYSQL_OPT_WRITE_TIMEOUT, &timeout);
	mysql_options(connection, MYSQL_OPT_PROTOCOL, &protocol);
	using reconnect_flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	reconnect_flag reconnect = false;
	mysql_options(connection, MYSQL_OPT_RECONNECT, &reconnect);
	if (!mysql_real_connect(connection, host.c_str(), required("DB_USER"),
				required("DB_PASSWD"), schema.c_str(),
				static_cast<unsigned int>(port), nullptr, 0))
	{
		const std::string error = mysql_error(connection);
		mysql_close(connection);
		throw std::runtime_error("fixture connection failed: " + error);
	}
	return connection;
}

void execute(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()))
		throw std::runtime_error("fixture SQL error " +
					 std::to_string(mysql_errno(connection)) + ": " +
					 mysql_error(connection));
}

std::vector<std::vector<std::string>> rows(MYSQL *connection, const std::string &sql)
{
	execute(connection, sql);
	MYSQL_RES *result = mysql_store_result(connection);
	if (!result)
		throw std::runtime_error("fixture query returned no result");
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
		throw std::runtime_error("fixture result fetch failed");
	return output;
}

std::string scalar(MYSQL *connection, const std::string &sql)
{
	auto result = rows(connection, sql);
	if (result.size() != 1 || result[0].size() != 1)
		throw std::runtime_error("fixture expected one scalar row");
	return result[0][0];
}

void assert_scalar(MYSQL *connection, const std::string &sql, const std::string &expected)
{
	if (scalar(connection, sql) != expected)
		throw std::runtime_error("lifecycle owner fixture assertion failed");
}

critical_operation_id ident(uint8_t seed)
{
	critical_operation_id value{};
	for (size_t index = 0; index < value.bytes.size(); ++index)
		value.bytes[index] = static_cast<uint8_t>(seed + index);
	return value;
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

void require_empty_owner_fixture(MYSQL *connection)
{
	for (const char *table :
	     { "accounts", "account_banks", "player_data", "economic_account_mapping",
	       "economic_lineage_state", "economic_epoch", "economic_baseline_control",
	       "economic_baseline_witness", "economic_baseline_reservation",
	       "economic_sql_lifecycle_installation", "critical_operation_inbox" })
		assert_scalar(connection, "SELECT COUNT(*) FROM " + std::string(table), "0");
	assert_scalar(
		connection,
		"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
		"AND table_name='economic_sql_activation_receipt'",
		"0");
}

void seed_native_sources(MYSQL *connection)
{
	execute(connection,
		"INSERT INTO accounts(account_name) VALUES('receipt_upgrade_a'),('receipt_upgrade_b')");
	execute(connection,
		"INSERT INTO account_banks(account_name,racewar,bank_copper,bank_silver,bank_gold,bank_platinum,bank_revision) VALUES"
		"('receipt_upgrade_a',0,8,1,0,0,4),('receipt_upgrade_b',1,0,3,0,0,9)");
	execute(connection,
		"INSERT INTO player_data(pid,name,account_name,racewar,copper,silver,gold,platinum,wallet_revision,save_revision) VALUES"
		"(29001,'ReceiptUpgradeOne','receipt_upgrade_a',0,4,2,0,0,6,12),"
		"(29002,'ReceiptUpgradeTwo','receipt_upgrade_b',1,0,5,1,0,8,15)");
}

void create_staged_baseline(MYSQL *connection, const economic_sql_lifecycle_guard &maintenance)
{
	economic_sql_lifecycle_request request;
	request.operation_id = ident(31);
	request.lineage = ident(71);
	request.epoch = ident(111);
	request.actor_id = 9001;
	request.accepted_at_usec = 123456789;

	economic_sql_lifecycle_receipt receipt;
	const auto status = economic_sql_accounting_lifecycle_transaction::install(
		connection, maintenance, request, &receipt);
	if (status)
		throw std::runtime_error("lifecycle owner refused staged fixture: " +
					 std::to_string(status));
	if (receipt.wallets.size() != 2 || receipt.banks.size() != 2 ||
	    !receipt.baseline_revision ||
	    receipt.baseline_operation_id.bytes == critical_operation_id{}.bytes)
		throw std::runtime_error("lifecycle owner returned incomplete staged baseline");

	const auto operation_id = sql_id(request.operation_id);
	const auto lineage = sql_id(request.lineage);
	const auto epoch = sql_id(request.epoch);
	const auto baseline_operation_id = sql_id(receipt.baseline_operation_id);
	assert_scalar(
		connection,
		"SELECT COUNT(*) FROM economic_sql_lifecycle_installation WHERE operation_id=" +
			operation_id + " AND lineage=" + lineage + " AND epoch=" + epoch +
			" AND baseline_operation_id=" + baseline_operation_id +
			" AND phase=2 AND selected_epoch=epoch",
		"1");
	assert_scalar(connection,
		      "SELECT COUNT(*) FROM economic_baseline_control c "
		      "JOIN economic_baseline_witness w ON w.lineage=c.lineage AND w.epoch=c.epoch "
		      "AND w.book_revision=c.revision WHERE c.lineage=" +
			      lineage + " AND c.epoch=" + epoch +
			      " AND c.revision=" + std::to_string(receipt.baseline_revision) +
			      " AND w.operation_id=" + baseline_operation_id,
		      "1");
	assert_scalar(connection,
		      "SELECT COUNT(*) FROM economic_sql_lifecycle_installation i "
		      "JOIN economic_lineage_state l ON l.lineage=i.lineage "
		      "WHERE i.operation_id=" +
			      operation_id + " AND l.active_epoch IS NOT NULL",
		      "0");
	assert_scalar(connection,
		      "SELECT COUNT(*) FROM economic_baseline_reservation WHERE lineage=" +
			      lineage + " AND epoch=" + epoch,
		      "4");
}
} // namespace

int main()
{
	MYSQL *connection = nullptr;
	bool library_initialized = false;
	try
	{
		if (mysql_library_init(0, nullptr, nullptr))
			throw std::runtime_error("mysql_library_init failed");
		library_initialized = true;
		connection = connect_fixture();
		{
			economic_sql_lifecycle_guard maintenance;
			const auto guard_status = economic_sql_lifecycle_guard::acquire_maintenance(
				connection, &maintenance);
			if (guard_status)
				throw std::runtime_error(
					"lifecycle maintenance guard refused disposable fixture");
			require_empty_owner_fixture(connection);
			seed_native_sources(connection);
			create_staged_baseline(connection, maintenance);
			assert_scalar(
				connection,
				"SELECT COUNT(*) FROM economic_lineage_state WHERE active_epoch IS NOT NULL",
				"0");
		}
		mysql_close(connection);
		connection = nullptr;
		mysql_library_end();
		library_initialized = false;
		puts("PASS fixture=real_lifecycle_owner staged_installation=1 baseline=verified active_epoch=NULL");
		return 0;
	}
	catch (const std::exception &error)
	{
		if (connection)
			mysql_close(connection);
		if (library_initialized)
			mysql_library_end();
		fprintf(stderr, "OWNER-FIXTURE-ERROR %s\n", error.what());
		return 2;
	}
}

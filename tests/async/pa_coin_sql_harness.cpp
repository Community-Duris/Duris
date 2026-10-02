#include "economy/coin_transfer_accounting.h"

#include <cerrno>

// Reuse the established, production-backed coin storage matrix without running
// its unrelated currency/ATM test main.  This translation unit gives the matrix
// its own disposable-database entry point.
#define main pa_coin_unused_currency_harness_main
#include "currency_transaction_mysql_harness.cpp"
#undef main

int main()
{
	const char *host = std::getenv("DB_HOST");
	const char *user = std::getenv("DB_USER");
	const char *password = std::getenv("DB_PASSWD");
	const char *database = std::getenv("CURRENCY_TEST_DB_NAME");
	const char *port_value = std::getenv("DB_PORT");
	assert(host && user && password && database);

	// Exercise the SQL implementation's fail-closed connection boundary.  A
	// positive accounting write needs the typed accounting root that this
	// legacy coin fixture deliberately does not manufacture.
	critical_command root_command = {};
	coin_transfer_result result = {};
	coin_transfer_accounting_context context = {};
	assert(coin_transfer_accounting_record(nullptr, root_command, result, 0, context) ==
	       ENOTCONN);

	connection = mysql_init(nullptr);
	assert(connection);
	const unsigned int port =
		port_value ? static_cast<unsigned int>(std::strtoul(port_value, nullptr, 10)) :
			     3306;
	assert(mysql_real_connect(connection, host, user, password, database, port, nullptr, 0));
	coin_failure_matrix();
	mysql_close(connection);
	return 0;
}

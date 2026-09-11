#include "cmd/information_cache.h"
#include <mysql/mysql.h>
#include <atomic>
#include <cassert>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <iostream>

std::atomic<unsigned> acquisitions{ 0 };
std::atomic<unsigned long long> last_bytes_sent{ 0 };
MYSQL *connect_test()
{
	MYSQL *connection = mysql_init(nullptr);
	assert(connection);
	assert(mysql_real_connect(connection, getenv("TEST_DB_HOST"), "root", "cache-test",
				  "cache_test", 3306, nullptr, 0));
	return connection;
}
extern "C" MYSQL *sql_pool_acquire()
{
	++acquisitions;
	return connect_test();
}
extern "C" void sql_pool_release(MYSQL *connection)
{
	assert(mysql_query(connection, "SHOW SESSION STATUS LIKE 'Bytes_sent'") == 0);
	MYSQL_RES *stats = mysql_store_result(connection);
	assert(stats);
	MYSQL_ROW row = mysql_fetch_row(stats);
	assert(row && row[1]);
	last_bytes_sent = strtoull(row[1], nullptr, 10);
	mysql_free_result(stats);
	mysql_close(connection);
}
int main()
{
	MYSQL *admin = connect_test();
	auto execute = [admin](const std::string &query)
	{
		if (mysql_query(admin, query.c_str()))
		{
			std::cerr << mysql_error(admin) << '\n';
			abort();
		}
	};
	assert(information_cache_refresh());
	information_cache_shutdown();
	assert(!information_cache_get("credits"));
	execute("CREATE TABLE mud_info (name VARCHAR(64), content MEDIUMTEXT)");
	execute("INSERT INTO mud_info VALUES ('credits','authors'),('faq',''),('wizlist','staff'),('lock','create')");
	assert(information_cache_refresh());
	assert(information_cache_refresh());
	information_cache_shutdown();
	assert(*information_cache_get("credits") == "authors");
	assert(information_cache_get("faq")->empty());
	assert(!information_cache_get("lock"));
	auto count = acquisitions.load();
	for (int i = 0; i < 10000; ++i)
		assert(*information_cache_get("wizlist") == "staff");
	assert(acquisitions == count);
	execute("UPDATE mud_info SET content='updated' WHERE name='credits'");
	assert(information_cache_refresh());
	information_cache_shutdown();
	assert(*information_cache_get("credits") == "updated");
	execute("DELETE FROM mud_info WHERE name='wizlist'");
	assert(information_cache_refresh());
	information_cache_shutdown();
	assert(*information_cache_get("wizlist") == "staff");
	execute("INSERT INTO mud_info VALUES ('wizlist', REPEAT('x',131073))");
	assert(information_cache_refresh());
	information_cache_shutdown();
	assert(*information_cache_get("wizlist") == "staff");
	execute("UPDATE mud_info SET content=REPEAT('x',8388608) WHERE name='wizlist'");
	assert(information_cache_refresh());
	information_cache_shutdown();
	assert(*information_cache_get("wizlist") == "staff");
	assert(last_bytes_sent < 4096);
	execute("UPDATE mud_info SET content='staff' WHERE name='wizlist'");
	information_cache_pulse();
	execute("UPDATE mud_info SET content='automatic' WHERE name='credits'");
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(65);
	while (*information_cache_get("credits") != "automatic")
	{
		assert(std::chrono::steady_clock::now() < deadline);
		information_cache_pulse();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	execute("DROP TABLE mud_info");
	assert(information_cache_refresh());
	information_cache_shutdown();
	assert(*information_cache_get("credits") == "automatic");
	mysql_close(admin);
	std::cout
		<< "MySQL information refresh, missing/empty/oversized content, failure retention and zero read acquisitions passed\n";
}

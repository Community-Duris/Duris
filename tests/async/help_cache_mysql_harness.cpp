#include "cmd/help_cache.h"
#include <mysql/mysql.h>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

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
	assert(help_title_matches("Fire shield", "fIrE"));
	assert(help_title_matches("Fire shield", "F%shield"));
	assert(help_title_matches("Fire shield", "Fi_e"));
	assert(!help_title_matches("Fire shield", "ice"));
	assert(help_title_equal("Fire", "fIrE"));
	MYSQL *admin = connect_test();
	auto execute = [admin](const std::string &query)
	{
		if (mysql_query(admin, query.c_str()))
		{
			std::cerr << mysql_error(admin) << '\n';
			abort();
		}
	};
	assert(help_cache_refresh());
	help_cache_shutdown();
	assert(!help_cache_get());
	execute("CREATE TABLE pages (title VARCHAR(256), text MEDIUMTEXT, category_id INT, "
		"last_update VARCHAR(64), last_update_by VARCHAR(64))");
	execute("INSERT INTO pages VALUES ('help','welcome',1,'today',NULL),"
		"('Fire','burn',9,'today','Editor')");
	assert(!help_cache_get());
	assert(help_cache_refresh());
	assert(help_cache_refresh());
	help_cache_shutdown();
	assert(help_cache_get() && help_cache_get()->size() == 2);
	assert(help_cache_get()->back().fields[4] == "Unknown");
	auto count = acquisitions.load();
	for (int i = 0; i < 10000; ++i)
		assert(help_cache_get()->size() == 2);
	assert(acquisitions == count);
	execute("UPDATE pages SET text='updated' WHERE title='help'");
	assert(help_cache_refresh());
	assert(help_cache_get()->back().fields[1] == "welcome");
	help_cache_shutdown();
	assert(help_cache_get()->back().fields[1] == "updated");
	execute("UPDATE pages SET text=REPEAT('x',131073) WHERE title='help'");
	assert(help_cache_refresh());
	help_cache_shutdown();
	assert(help_cache_get()->back().fields[1] == "updated");
	assert(help_cache_status().find("oversized") != std::string::npos);
	// Oversized source values and catalogs must be rejected on the server,
	// before the client receives their payloads (not merely after allocation).
	execute("UPDATE pages SET text=REPEAT('x',8388608) WHERE title='help'");
	assert(help_cache_refresh());
	help_cache_shutdown();
	assert(help_cache_get()->back().fields[1] == "updated");
	assert(last_bytes_sent < 4096);
	execute("UPDATE pages SET text='updated' WHERE title='help'");
	execute("INSERT INTO pages SELECT CONCAT('bulk',seq),REPEAT('x',70000),0,'today','test' FROM seq_1_to_500");
	assert(help_cache_refresh());
	help_cache_shutdown();
	assert(help_cache_get()->size() == 2);
	assert(last_bytes_sent < 65536);
	execute("DELETE FROM pages WHERE title LIKE 'bulk%'");
	execute("INSERT INTO pages SELECT CONCAT('bulk',seq),'x',0,'today','test' FROM seq_1_to_20001");
	assert(help_cache_refresh());
	help_cache_shutdown();
	assert(help_cache_get()->size() == 2);
	assert(last_bytes_sent < 1024 * 1024);
	execute("DELETE FROM pages WHERE title LIKE 'bulk%'");
	help_cache_pulse(); // initialize the periodic refresh clock
	execute("UPDATE pages SET text='automatic' WHERE title='help'");
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(65);
	while (help_cache_get()->back().fields[1] != "automatic")
	{
		assert(std::chrono::steady_clock::now() < deadline);
		help_cache_pulse();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	execute("DROP TABLE pages");
	assert(help_cache_refresh());
	help_cache_shutdown();
	assert(help_cache_get()->back().fields[1] == "automatic");
	assert(help_cache_status().find("query failed") != std::string::npos);
	mysql_close(admin);
	std::cout
		<< "MySQL catalog refresh, metadata, bounds, failure retention and zero read acquisitions passed\n";
}

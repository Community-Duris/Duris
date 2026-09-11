#include "cmd/help_cache.h"
#include <mysql/mysql.h>
#include <atomic>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <thread>

std::atomic<unsigned> acquisitions{ 0 };
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
	assert(!help_cache_refresh());
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
	execute("DROP TABLE pages");
	assert(help_cache_refresh());
	help_cache_shutdown();
	assert(help_cache_get()->back().fields[1] == "updated");
	assert(help_cache_status().find("query failed") != std::string::npos);
	mysql_close(admin);
	std::cout
		<< "MySQL catalog refresh, metadata, bounds, failure retention and zero read acquisitions passed\n";
}

#include "item/artifact_mana_store.h"
#include "sql/sql_pool.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

static const char *test_host;
static const char *test_password;
static unsigned test_port;

extern "C" MYSQL *sql_pool_acquire()
{
	MYSQL *connection = mysql_init(nullptr);
	if (!connection || !mysql_real_connect(connection, test_host, "mana_test", test_password,
					       "mana_test", test_port, nullptr, 0))
	{
		if (connection)
			mysql_close(connection);
		return nullptr;
	}
	return connection;
}

extern "C" void sql_pool_release(MYSQL *connection)
{
	mysql_close(connection);
}

int main(int argc, char **argv)
{
	assert(argc == 4);
	test_host = argv[1];
	test_password = argv[2];
	test_port = static_cast<unsigned>(std::strtoul(argv[3], nullptr, 10));
	assert(test_port);
	const uint64_t uid = static_cast<uint64_t>(getpid()) * 1000 + 42;
	const artifact_mana_profile profile{ 7, 1, 100000, 25, 0 };
	auto record = artifact_mana_empty(uid, profile, 100);
	artifact_mana_record observed;
	assert(artifact_mana_store_read(true, "unused", uid, observed) ==
	       artifact_mana_read::missing);
	assert(artifact_mana_store_write(true, "unused", 0, record));
	const auto empty = record;
	assert(artifact_mana_spend(record, profile, 110, 249, false));
	assert(record.reserve == 1);
	assert(artifact_mana_store_write(true, "unused", 1, record));
	assert(artifact_mana_store_write(true, "unused", 1, record));
	assert(!artifact_mana_store_write(true, "unused", 0, empty));
	auto stale = record;
	stale.reserve = 250;
	assert(!artifact_mana_store_write(true, "unused", 1, stale));
	assert(artifact_mana_spend(record, profile, 120, 50, false));
	assert(artifact_mana_spend(record, profile, 120, 1, false));
	assert(artifact_mana_store_write(true, "unused", 2, record)); // coalesced versions
	assert(artifact_mana_store_read(true, "unused", uid, observed) ==
	       artifact_mana_read::found);
	assert(observed == record);
	assert(!artifact_mana_store_write(true, "unused", 1, stale));
	MYSQL *connection = sql_pool_acquire();
	assert(connection);
	const auto corruption =
		"UPDATE artifact_mana SET reserve=capacity+1 WHERE item_uid=" + std::to_string(uid);
	assert(!mysql_real_query(connection, corruption.data(), corruption.size()));
	sql_pool_release(connection);
	assert(artifact_mana_store_read(true, "unused", uid, observed) ==
	       artifact_mana_read::error);
	assert(!artifact_mana_store_write(true, "unused", 0, empty));
	std::puts(
		"artifact mana SQL enrollment, debit, exact retry, stale CAS and corruption checks passed");
}

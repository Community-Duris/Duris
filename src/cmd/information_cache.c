#include "cmd/information_cache.h"
#include "core/refresh_cache.h"
#include <array>
#include <cstring>
#ifdef __NO_MYSQL__
#include "flatfile/flatfile_help_catalog.h"
#else
#include "sql/sql_pool.h"
#include "sql/sql_thread_init.h"
#endif

namespace
{
constexpr std::array<const char *, 3> names = { "credits", "faq", "wizlist" };
using information_pages = std::array<std::string, 3>;
refresh_cache<information_pages> cache;
constexpr size_t max_page_bytes = 128 * 1024;

bool load_information(information_pages &pages, std::string &error)
{
#ifdef __NO_MYSQL__
	for (size_t i = 0; i < names.size(); ++i)
		if (!flatfile_information_read(".", names[i], &pages[i], &error, max_page_bytes))
			return false;
#else
	if (sql_worker_thread_init() != 0)
	{
		error = "MySQL worker initialization failed";
		return false;
	}
	struct thread_guard
	{
		~thread_guard() { mysql_thread_end(); }
	} thread;
	MYSQL *connection = sql_pool_acquire();
	if (!connection)
	{
		error = "information database connection unavailable";
		return false;
	}
	struct connection_guard
	{
		MYSQL *value;
		~connection_guard() { sql_pool_release(value); }
	} borrowed{ connection };
	const char *query =
		"SELECT name, IF(OCTET_LENGTH(content)<=131072,content,NULL) FROM mud_info WHERE name IN "
		"('credits','faq','wizlist') LIMIT 4";
	if (mysql_real_query(connection, query, strlen(query)))
	{
		error = "information query failed";
		return false;
	}
	MYSQL_RES *rows = mysql_use_result(connection);
	if (!rows)
	{
		error = "information result unavailable";
		return false;
	}
	struct result_guard
	{
		MYSQL_RES *value;
		~result_guard() { mysql_free_result(value); }
	} result{ rows };
	std::array<bool, 3> found = {};
	while (MYSQL_ROW row = mysql_fetch_row(rows))
	{
		const unsigned long *lengths = mysql_fetch_lengths(rows);
		bool accepted = false;
		for (size_t i = 0; i < names.size(); ++i)
			if (row[0] && !strcmp(row[0], names[i]) && row[1] && !found[i] &&
			    lengths[1] <= max_page_bytes)
			{
				pages[i].assign(row[1], lengths[1]);
				found[i] = true;
				accepted = true;
				break;
			}
		if (!accepted)
		{
			error = "invalid or oversized information page";
			return false;
		}
	}
	if (mysql_errno(connection) || !found[0] || !found[1] || !found[2])
	{
		error = "incomplete information catalog";
		return false;
	}
#endif
	for (const auto &page : pages)
		if (page.size() > max_page_bytes)
		{
			error = "oversized information page";
			return false;
		}
	return true;
}
}

bool information_cache_refresh()
{
	return cache.request(load_information);
}
void information_cache_pulse()
{
	cache.poll();
	using clock = std::chrono::steady_clock;
	static auto next_refresh = clock::now() + std::chrono::seconds(60);
	const auto now = clock::now();
	if (now >= next_refresh && !cache.busy())
	{
		cache.request(load_information);
		next_refresh = now + std::chrono::seconds(60);
	}
}
void information_cache_shutdown()
{
	cache.shutdown();
}
std::string information_cache_status()
{
	return cache.status();
}
const std::string *information_cache_get(const std::string &name)
{
	const auto *pages = cache.get();
	if (pages)
		for (size_t i = 0; i < names.size(); ++i)
			if (name == names[i])
				return &(*pages)[i];
	return nullptr;
}

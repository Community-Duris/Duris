#include "cmd/help_cache.h"
#include "core/refresh_cache.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#ifndef __NO_MYSQL__
#include "sql/sql_pool.h"
#include "sql/sql_thread_init.h"
#endif

namespace
{
refresh_cache<help_catalog> cache;
constexpr size_t max_pages = 20000;
constexpr size_t max_bytes = 32 * 1024 * 1024;
constexpr size_t max_page_bytes = 128 * 1024;

bool load_help(help_catalog &catalog, std::string &error)
{
#ifdef __NO_MYSQL__
	(void)catalog;
	error = "flat-file help uses its startup catalog";
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
		error = "help database connection unavailable";
		return false;
	}
	struct connection_guard
	{
		MYSQL *value;
		~connection_guard() { sql_pool_release(value); }
	} borrowed{ connection };
	// Bound the transmitted row before libmysql allocates its receive buffer.
	// The importer commits one InnoDB transaction; this one statement observes
	// either the previous complete catalog or the new complete catalog.
	const char *query =
		"SELECT IF(page_bytes<=131072 AND catalog_bytes<=33554432 AND catalog_count<=20000,title,NULL), IF(page_bytes<=131072 AND catalog_bytes<=33554432 AND catalog_count<=20000,text,NULL), "
		"IF(page_bytes<=131072 AND catalog_bytes<=33554432 AND catalog_count<=20000,category_id,NULL), IF(page_bytes<=131072 AND catalog_bytes<=33554432 AND catalog_count<=20000,last_update,NULL), "
		"IF(page_bytes<=131072 AND catalog_bytes<=33554432 AND catalog_count<=20000,last_update_by,NULL) FROM "
		"(SELECT sized.*,SUM(page_bytes) OVER () AS catalog_bytes,COUNT(*) OVER () AS catalog_count "
		"FROM (SELECT title,text,category_id,last_update,last_update_by, "
		"COALESCE(OCTET_LENGTH(title),0)+COALESCE(OCTET_LENGTH(text),0)+"
		"COALESCE(OCTET_LENGTH(category_id),0)+COALESCE(OCTET_LENGTH(last_update),0)+"
		"COALESCE(OCTET_LENGTH(last_update_by),0) AS page_bytes "
		"FROM pages ORDER BY title ASC LIMIT 20001) AS sized) AS bounded ORDER BY title ASC";
	if (mysql_real_query(connection, query, strlen(query)))
	{
		error = "help catalog query failed";
		return false;
	}
	MYSQL_RES *rows = mysql_use_result(connection);
	if (!rows)
	{
		error = "help catalog result unavailable";
		return false;
	}
	struct result_guard
	{
		MYSQL_RES *value;
		~result_guard() { mysql_free_result(value); }
	} result{ rows };
	size_t bytes = 0;
	while (MYSQL_ROW row = mysql_fetch_row(rows))
	{
		const unsigned long *lengths = mysql_fetch_lengths(rows);
		size_t page_bytes = 0;
		for (size_t i = 0; i < 5; ++i)
			page_bytes += lengths[i];
		if (!row[0] || !*row[0] || lengths[0] > 256 || !row[1] || !row[2] ||
		    catalog.size() >= max_pages || page_bytes > max_page_bytes ||
		    page_bytes > max_bytes - bytes)
		{
			error = "invalid or oversized help catalog";
			return false;
		}
		bytes += page_bytes;
		help_page page;
		for (size_t i = 0; i < 5; ++i)
			page.fields[i] = row[i] ? std::string(row[i], lengths[i]) : "Unknown";
		catalog.push_back(std::move(page));
	}
	if (mysql_errno(connection) || catalog.empty() ||
	    std::none_of(catalog.begin(), catalog.end(),
			 [](const auto &page) { return help_title_equal(page.fields[0], "help"); }))
	{
		error = "incomplete help catalog (default help required)";
		return false;
	}
	return true;
#endif
}
}

bool help_cache_refresh()
{
	return cache.request(load_help);
}
void help_cache_pulse()
{
	cache.poll();
#ifndef __NO_MYSQL__
	using clock = std::chrono::steady_clock;
	static auto next_refresh = clock::now() + std::chrono::seconds(60);
	const auto now = clock::now();
	if (now >= next_refresh && !cache.busy())
	{
		cache.request(load_help);
		next_refresh = now + std::chrono::seconds(60);
	}
#endif
}
void help_cache_shutdown()
{
	cache.shutdown();
}
std::string help_cache_status()
{
	return cache.status();
}
const help_catalog *help_cache_get()
{
	return cache.get();
}
bool help_title_equal(const std::string &left, const std::string &right)
{
	return left.size() == right.size() &&
	       std::equal(left.begin(), left.end(), right.begin(),
			  [](unsigned char a, unsigned char b)
			  { return std::tolower(a) == std::tolower(b); });
}
bool help_title_matches(const std::string &title, const std::string &query)
{
	// Preserve the old LIKE '%query%' wildcard syntax without database work.
	const std::string pattern = "%" + query + "%";
	size_t input = 0, token = 0, star = std::string::npos, retry = 0;
	while (input < title.size())
	{
		if (token < pattern.size() && pattern[token] == '%')
		{
			star = token++;
			retry = input;
		}
		else if (token < pattern.size() &&
			 (pattern[token] == '_' ||
			  std::tolower(static_cast<unsigned char>(pattern[token])) ==
				  std::tolower(static_cast<unsigned char>(title[input]))))
		{
			++input;
			++token;
		}
		else if (star != std::string::npos)
		{
			token = star + 1;
			input = ++retry;
		}
		else
			return false;
	}
	while (token < pattern.size() && pattern[token] == '%')
		++token;
	return token == pattern.size();
}

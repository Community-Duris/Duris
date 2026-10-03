#ifndef DURIS_TEST_ECONOMIC_SQL_COMMIT_REPLY_LOSS_FIXTURE_H
#define DURIS_TEST_ECONOMIC_SQL_COMMIT_REPLY_LOSS_FIXTURE_H

#include <atomic>
#include <cassert>
#include <cstring>
#include <mysql.h>

// Hide one successful native COMMIT reply, then require the pool boundary to
// replace that connection and reconcile the original receipt. This does not
// substitute for a production pool or an actual network-disconnection journey.
namespace economic_sql_commit_reply_loss_fixture
{
inline std::atomic<bool> armed{ false };
inline std::atomic<MYSQL *> selected{ nullptr }, failed{ nullptr };
inline std::atomic<unsigned> commits{ 0 }, replacements{ 0 };

inline void arm()
{
	assert(!armed.load() && !selected.load() && !failed.load());
	commits = 0;
	replacements = 0;
	armed = true;
}

inline void acquired(MYSQL *connection)
{
	if (connection && armed.exchange(false))
		selected = connection;
}

inline int query_result(MYSQL *connection, const char *query, unsigned long length, int result)
{
	if (!result && connection && connection == selected.load() && length == 6 &&
	    !memcmp(query, "COMMIT", 6))
	{
		selected = nullptr;
		failed = connection;
		++commits;
		return 1;
	}
	return result;
}

inline bool lost_reply(MYSQL *connection)
{
	return connection && connection == failed.load();
}

inline void closing(MYSQL *connection, bool replacement)
{
	if (!connection)
		return;
	if (connection == selected.load())
		selected = nullptr;
	if (connection == failed.load())
	{
		failed = nullptr;
		if (replacement)
			++replacements;
	}
}

inline void verify()
{
	assert(commits == 1 && replacements == 1);
	assert(!armed.load() && !selected.load() && !failed.load());
}
} // namespace economic_sql_commit_reply_loss_fixture

#endif

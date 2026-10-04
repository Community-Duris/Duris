#include "account/account_load.h"
#include "net/network_wakeup.h"
#include "sql/sql_exclusion_guard.h"
#include "sql/sql_pool.h"
#include "sql/sql_thread_init.h"
#include "sql/sql_telemetry_account_identity.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cctype>
#include <chrono>
#include <climits>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <sys/stat.h>

#ifndef __NO_MYSQL__
#include <mysql/errmsg.h>

namespace
{
char *escape(MYSQL *connection, const char *name)
{
	const size_t length = strlen(name);
	char *escaped = static_cast<char *>(malloc(length * 2 + 1));
	if (escaped)
		mysql_real_escape_string(connection, escaped, name, length);
	return escaped;
}

bool repair_query(MYSQL *connection, const char *query, uint64_t deadline)
{
	return (!deadline || account_load_now_usec() < deadline) &&
	       duris_sql_exclusion_guard_allows(connection) && mysql_query(connection, query) == 0;
}

struct query_failure
{
	account_load_outcome outcome;
	unsigned int error;
};

void execute(MYSQL *connection, const std::string &query, uint64_t deadline)
{
	if (account_load_now_usec() >= deadline)
		throw query_failure{ account_load_outcome::timed_out, ETIMEDOUT };
	if (!duris_sql_exclusion_guard_allows(connection))
		throw query_failure{ account_load_outcome::load_failed, EACCES };
	if (mysql_query(connection, query.c_str()) != 0)
		throw query_failure{ account_load_outcome::load_failed, mysql_errno(connection) };
}

using owned_rows = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;
owned_rows select_rows(MYSQL *connection, const std::string &query, uint64_t deadline)
{
	execute(connection, query, deadline);
	owned_rows rows(mysql_store_result(connection), mysql_free_result);
	if (!rows)
		throw query_failure{ account_load_outcome::load_failed, mysql_errno(connection) };
	return rows;
}

const char *value(MYSQL_ROW row, size_t column)
{
	return row[column] ? row[column] : "";
}

bool legacy_account_file_exists(std::string name)
{
	const std::string original = name;
	for (char &letter : name)
		letter = static_cast<char>(tolower(static_cast<unsigned char>(letter)));
	struct stat info = {};
	const std::string directory = "Accounts/" + name.substr(0, 1) + "/";
	return stat((directory + name).c_str(), &info) == 0 ||
	       stat((directory + original).c_str(), &info) == 0;
}

struct transaction_guard
{
	MYSQL *connection;
	bool active = false;
	~transaction_guard()
	{
		if (active)
			(void)mysql_query(connection, "ROLLBACK");
	}
};
} // namespace

int account_load_repair(MYSQL *connection, const char *account_name, uint64_t deadline_usec)
{
	if (!connection || !account_name || !account_name[0])
		return -1;

	char *escaped_account = escape(connection, account_name);
	if (!escaped_account)
		return -1;

	char query[4096];
	char eligibility[1024];
	const int eligibility_written = snprintf(
		eligibility, sizeof(eligibility),
		"pd.active=1 AND LOWER(pd.account_name)=LOWER('%s') AND NOT EXISTS ("
		"SELECT 1 FROM account_characters tombstone WHERE tombstone.deleted_at IS NOT NULL "
		"AND (tombstone.pid=pd.pid OR LOWER(tombstone.char_name)=LOWER(pd.name)))",
		escaped_account);
	if (eligibility_written < 0 ||
	    static_cast<size_t>(eligibility_written) >= sizeof(eligibility))
	{
		free(escaped_account);
		return -1;
	}
	int written = snprintf(
		query, sizeof(query),
		"INSERT INTO currency_wallet_baseline(pid,opening_copper,opening_silver,"
		"opening_gold,opening_platinum,opening_revision) "
		"SELECT pd.pid,pd.copper,pd.silver,pd.gold,pd.platinum,pd.wallet_revision "
		"FROM player_data pd WHERE %s AND pd.wallet_revision=0 "
		"AND NOT EXISTS (SELECT 1 FROM currency_ledger ledger WHERE ledger.pid=pd.pid) "
		"AND NOT EXISTS (SELECT 1 FROM currency_wallet_baseline baseline "
		"WHERE baseline.pid=pd.pid)",
		eligibility);
	if (written < 0 || static_cast<size_t>(written) >= sizeof(query) ||
	    !repair_query(connection, query, deadline_usec))
	{
		free(escaped_account);
		return -1;
	}
	written = snprintf(
		query, sizeof(query),
		"INSERT INTO epic_balance_baseline(pid,opening_balance,opening_revision) "
		"SELECT pd.pid,pd.epics,pd.epic_revision FROM player_data pd WHERE %s "
		"AND pd.epic_revision=0 AND NOT EXISTS (SELECT 1 FROM epic_ledger ledger "
		"WHERE ledger.pid=pd.pid) AND NOT EXISTS (SELECT 1 FROM epic_balance_baseline "
		"baseline WHERE baseline.pid=pd.pid)",
		eligibility);
	if (written < 0 || static_cast<size_t>(written) >= sizeof(query) ||
	    !repair_query(connection, query, deadline_usec))
	{
		free(escaped_account);
		return -1;
	}
	written = snprintf(
		query, sizeof(query),
		"INSERT INTO combat_frag_baseline(pid,opening_frags,opening_revision) "
		"SELECT pd.pid,pd.frags,pd.frag_revision FROM player_data pd WHERE %s "
		"AND pd.frag_revision=0 AND NOT EXISTS (SELECT 1 FROM combat_frag_ledger ledger "
		"WHERE ledger.pid=pd.pid) AND NOT EXISTS (SELECT 1 FROM combat_frag_baseline "
		"baseline WHERE baseline.pid=pd.pid)",
		eligibility);
	if (written < 0 || static_cast<size_t>(written) >= sizeof(query) ||
	    !repair_query(connection, query, deadline_usec))
	{
		free(escaped_account);
		return -1;
	}

	written =
		snprintf(query, sizeof(query),
			 "INSERT INTO account_characters "
			 "(id, account_name, pid, char_name, created_at, deleted_at) "
			 "SELECT active_mapping.id, pd.account_name, pd.pid, pd.name, NOW(), NULL "
			 "FROM player_data pd "
			 "LEFT JOIN account_characters active_mapping "
			 "ON active_mapping.pid=pd.pid AND active_mapping.deleted_at IS NULL "
			 "JOIN currency_wallet_baseline wallet ON wallet.pid=pd.pid "
			 "JOIN epic_balance_baseline epic ON epic.pid=pd.pid "
			 "JOIN combat_frag_baseline combat ON combat.pid=pd.pid "
			 "WHERE pd.active=1 AND LOWER(pd.account_name)=LOWER('%s') "
			 "AND NOT EXISTS ("
			 "SELECT 1 FROM account_characters tombstone "
			 "WHERE tombstone.deleted_at IS NOT NULL "
			 "AND (tombstone.pid=pd.pid OR LOWER(tombstone.char_name)=LOWER(pd.name))) "
			 "ON DUPLICATE KEY UPDATE "
			 "account_name=VALUES(account_name), pid=VALUES(pid), "
			 "char_name=VALUES(char_name), deleted_at=NULL",
			 escaped_account);
	free(escaped_account);
	if (written < 0 || static_cast<size_t>(written) >= sizeof(query))
		return -1;
	if (!repair_query(connection, query, deadline_usec))
		return -1;

	const my_ulonglong affected = mysql_affected_rows(connection);
	return affected > static_cast<my_ulonglong>(INT_MAX) ? INT_MAX : static_cast<int>(affected);
}

account_load_result account_load_execute(MYSQL *connection, const account_load_request &request)
{
	account_load_result result = {};
	result.id = request.id;
	result.request_name = request.name;
	if (!connection)
	{
		result.outcome = account_load_outcome::unavailable;
		return result;
	}
	transaction_guard transaction = { connection };
	try
	{
		if ((connection->server_status & SERVER_STATUS_IN_TRANS) ||
		    !(connection->server_status & SERVER_STATUS_AUTOCOMMIT))
			throw query_failure{ account_load_outcome::load_failed, EBUSY };
		std::unique_ptr<char, decltype(&free)> escaped(
			escape(connection, request.name.c_str()), free);
		if (!escaped)
			throw std::bad_alloc();
		const std::string name = escaped.get();
		execute(connection, "SET TRANSACTION ISOLATION LEVEL REPEATABLE READ",
			request.deadline_usec);
		execute(connection, "START TRANSACTION", request.deadline_usec);
		transaction.active = true;
		// Lock the account before repair: deletion/credential/account saves cannot
		// interleave. The first consistent read below sees our own repairs, and all
		// remaining rows come from the same repeatable-read transaction.
		auto rows = select_rows(
			connection,
			"SELECT account_name, email, password, confirmation_code, confirmed, confirmation_sent, "
			"blocked, last_login, last_good_char, last_evil_char, flags1, flags2, flags3, flags4 "
			"FROM accounts WHERE account_name='" +
				name + "' LIMIT 1 FOR UPDATE",
			request.deadline_usec);
		MYSQL_ROW row = mysql_fetch_row(rows.get());
		if (!row)
		{
			if (mysql_errno(connection))
				throw query_failure{ account_load_outcome::load_failed,
						     mysql_errno(connection) };
			result.outcome = legacy_account_file_exists(request.name) ?
						 account_load_outcome::load_failed :
						 account_load_outcome::not_found;
			return result;
		}
		auto &snapshot = result.snapshot;
		size_t bytes = 0;
		auto copy = [&bytes](const char *text)
		{
			bytes += strlen(text);
			if (bytes > ACCOUNT_LOAD_MAX_BYTES)
				throw query_failure{ account_load_outcome::limit_exceeded,
						     EOVERFLOW };
			return std::string(text);
		};
		snapshot.name = copy(value(row, 0));
		snapshot.email = copy(value(row, 1));
		snapshot.password = copy(value(row, 2));
		snapshot.confirmation = copy(value(row, 3));
		snapshot.confirmed = static_cast<char>(atoi(value(row, 4)));
		snapshot.confirmation_sent = static_cast<char>(atoi(value(row, 5)));
		snapshot.blocked = static_cast<char>(atoi(value(row, 6)));
		// Preserve the synchronous loader's persisted scalar interpretation.
		snapshot.last = atol(value(row, 7));
		snapshot.good = atol(value(row, 8));
		snapshot.evil = atol(value(row, 9));
		for (size_t index = 0; index < 4; ++index)
			snapshot.flags[index] = strtoul(value(row, index + 10), nullptr, 10);
		rows.reset();
		result.repaired = account_load_repair(connection, request.name.c_str(),
						      request.deadline_usec);
		if (result.repaired < 0)
			throw query_failure{ account_load_now_usec() >= request.deadline_usec ?
						     account_load_outcome::timed_out :
						     account_load_outcome::repair_failed,
					     mysql_errno(connection) };
		rows = select_rows(
			connection,
			"SELECT hostname, ip_address, count FROM account_ips WHERE account_name='" +
				name + "' LIMIT " + std::to_string(ACCOUNT_LOAD_MAX_IPS + 1),
			request.deadline_usec);
		while ((row = mysql_fetch_row(rows.get())))
		{
			if (snapshot.ips.size() == ACCOUNT_LOAD_MAX_IPS)
				throw query_failure{ account_load_outcome::limit_exceeded,
						     EOVERFLOW };
			snapshot.ips.push_back({ copy(value(row, 0)), copy(value(row, 1)),
						 strtoul(value(row, 2), nullptr, 10) });
		}
		if (mysql_errno(connection))
			throw query_failure{ account_load_outcome::load_failed,
					     mysql_errno(connection) };
		rows.reset();
		rows = select_rows(
			connection,
			"SELECT ac.pid, ac.char_name, ac.login_count, ac.last_login, ac.blocked, ac.racewar, "
			"pd.level, pd.race, pd.m_class, pd.secondary_class, pd.last_room, pd.last_save "
			"FROM account_characters ac LEFT JOIN player_data pd ON ac.pid=pd.pid "
			"WHERE LOWER(ac.account_name)=LOWER('" +
				name + "') AND ac.deleted_at IS NULL LIMIT " +
				std::to_string(ACCOUNT_LOAD_MAX_CHARACTERS + 1),
			request.deadline_usec);
		while ((row = mysql_fetch_row(rows.get())))
		{
			if (snapshot.characters.size() == ACCOUNT_LOAD_MAX_CHARACTERS)
				throw query_failure{ account_load_outcome::limit_exceeded,
						     EOVERFLOW };
			account_load_character character = {};
			character.pid = atoi(value(row, 0));
			character.name = copy(value(row, 1));
			character.count = strtoul(value(row, 2), nullptr, 10);
			character.last = atol(value(row, 3));
			character.blocked = static_cast<char>(atoi(value(row, 4)));
			character.racewar = static_cast<char>(atoi(value(row, 5)));
			character.level = atoi(value(row, 6));
			character.race = atoi(value(row, 7));
			character.primary_class =
				static_cast<unsigned int>(strtoul(value(row, 8), nullptr, 10));
			character.secondary_class =
				static_cast<unsigned int>(strtoul(value(row, 9), nullptr, 10));
			character.last_room = atoi(value(row, 10));
			character.last_save = atol(value(row, 11));
			snapshot.characters.push_back(std::move(character));
		}
		if (mysql_errno(connection))
			throw query_failure{ account_load_outcome::load_failed,
					     mysql_errno(connection) };
		rows.reset();
		if (result.repaired > 0 && snapshot.characters.empty())
			throw query_failure{ account_load_outcome::load_failed, EIO };
		execute(connection, "COMMIT", request.deadline_usec);
		transaction.active = false;
		result.outcome = account_load_outcome::loaded;
		// The account snapshot is committed before optional token preparation.
		// A telemetry outage must not discard valid credentials or block login.
		if (request.telemetry_environment_id && request.telemetry_season_id &&
		    account_load_now_usec() < request.deadline_usec)
		{
			uint64_t token = 0;
			try
			{
				if (sql_prepare_telemetry_account_token_on(
					    connection, snapshot.name.c_str(),
					    request.telemetry_environment_id,
					    request.telemetry_season_id, request.deadline_usec,
					    &token) &&
				    token)
				{
					snapshot.telemetry_account_token = token;
					snapshot.telemetry_environment_id =
						request.telemetry_environment_id;
					snapshot.telemetry_season_id = request.telemetry_season_id;
				}
			}
			catch (...)
			{
				// Retain the committed snapshot with explicitly unknown identity.
			}
		}
	}
	catch (const query_failure &failure)
	{
		result.outcome = failure.outcome;
		result.error = failure.error;
	}
	catch (...)
	{
		result.outcome = account_load_outcome::load_failed;
		result.error = ENOMEM;
	}
	if (result.outcome != account_load_outcome::loaded)
		result.snapshot = {};
	return result;
}
#else
int account_load_repair(MYSQL *, const char *, uint64_t)
{
	return -1;
}
account_load_result account_load_execute(MYSQL *, const account_load_request &request)
{
	account_load_result result = {};
	result.id = request.id;
	result.request_name = request.name;
	result.outcome = account_load_outcome::unavailable;
	return result;
}
#endif

struct account_load_job
{
	account_load_request request;
	account_load_result result;
	bool started = false, done = false, cancelled = false;
};

namespace
{
account_load_result execute_repository(const account_load_request &request, void *)
{
#ifndef __NO_MYSQL__
	MYSQL *connection = sql_pool_acquire();
	if (!connection)
		return account_load_execute(nullptr, request);
	struct connection_guard
	{
		MYSQL *connection;
		~connection_guard() { sql_pool_release(connection); }
	} guard = { connection };
	account_load_result result;
	try
	{
		result = account_load_execute(connection, request);
	}
	catch (...)
	{
		sql_pool_discard_connection(connection);
		throw;
	}
	// A failed rollback/commit or transport error must never leak a transaction
	// or an uncertain result into a subsequent pool borrower.
	if ((connection->server_status & SERVER_STATUS_IN_TRANS) ||
	    !(connection->server_status & SERVER_STATUS_AUTOCOMMIT) ||
	    result.error >= CR_MIN_ERROR || mysql_errno(connection) >= CR_MIN_ERROR)
	{
		sql_pool_discard_connection(connection);
		if (result.outcome != account_load_outcome::loaded)
		{
			result.snapshot = {};
			result.outcome = account_load_outcome::load_failed;
		}
	}
	return result;
#else
	return account_load_execute(nullptr, request);
#endif
}

struct account_worker
{
	std::mutex mutex;
	std::condition_variable available;
	std::deque<std::unique_ptr<account_load_job>> jobs;
	std::thread thread;
	account_load_execute_fn execute = nullptr;
	void *context = nullptr;
	bool stopping = true;
	~account_worker() { shutdown(); }

	account_load_job *next()
	{
		for (const auto &job : jobs)
			if (!job->started)
				return job.get();
		return nullptr;
	}
	void erase(account_load_job *job)
	{
		const auto found = std::find_if(jobs.begin(), jobs.end(), [job](const auto &entry)
						{ return entry.get() == job; });
		if (found != jobs.end())
			jobs.erase(found);
	}
	void run()
	{
		const bool mysql_thread = execute == execute_repository;
		if (mysql_thread && sql_worker_thread_init() != 0)
		{
			std::lock_guard<std::mutex> lock(mutex);
			stopping = true;
			for (auto &job : jobs)
			{
				job->started = job->done = true;
				job->result.id = job->request.id;
				job->result.request_name = std::move(job->request.name);
				job->result.outcome = account_load_outcome::unavailable;
			}
			network_wakeup_notify();
			return;
		}
		std::unique_lock<std::mutex> lock(mutex);
		for (;;)
		{
			available.wait(lock, [this] { return stopping || next(); });
			if (stopping)
				break;
			account_load_job *job = next();
			job->started = true;
			lock.unlock();
			account_load_result result = {};
			try
			{
				if (account_load_now_usec() >= job->request.deadline_usec)
					result.outcome = account_load_outcome::timed_out;
				else
					result = execute(job->request, context);
			}
			catch (...)
			{
				result.outcome = account_load_outcome::load_failed;
			}
			if (result.outcome == account_load_outcome::loaded &&
			    (result.id != job->request.id ||
			     result.request_name != job->request.name))
			{
				result.outcome = account_load_outcome::load_failed;
				result.snapshot = {};
			}
			result.id = job->request.id;
			result.request_name = std::move(job->request.name);
			if (account_load_now_usec() >= job->request.deadline_usec)
			{
				result.outcome = account_load_outcome::timed_out;
				result.snapshot = {};
			}
			lock.lock();
			if (job->cancelled)
				erase(job);
			else
			{
				job->result = std::move(result);
				job->done = true;
				network_wakeup_notify();
			}
		}
		lock.unlock();
#ifndef __NO_MYSQL__
		if (mysql_thread)
			mysql_thread_end();
#endif
	}
	void shutdown()
	{
		{
			std::lock_guard<std::mutex> lock(mutex);
			stopping = true;
			for (auto &job : jobs)
				job->cancelled = true;
			available.notify_one();
		}
		if (thread.joinable())
			thread.join();
		std::lock_guard<std::mutex> lock(mutex);
		jobs.clear();
		execute = nullptr;
		context = nullptr;
	}
};
account_worker worker;
std::atomic<uint64_t> next_id = 1;
} // namespace

uint64_t account_load_now_usec()
{
	return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
					     std::chrono::steady_clock::now().time_since_epoch())
					     .count());
}
uint64_t account_load_next_id()
{
	uint64_t id = next_id.fetch_add(1, std::memory_order_relaxed);
	return id ? id : next_id.fetch_add(1, std::memory_order_relaxed);
}
bool account_load_worker_init(account_load_execute_fn execute, void *context)
{
	std::lock_guard<std::mutex> lock(worker.mutex);
	if (worker.thread.joinable())
		return false;
#ifdef __NO_MYSQL__
	if (!execute)
		return false;
#endif
	worker.execute = execute ? execute : execute_repository;
	worker.context = context;
	worker.stopping = false;
	try
	{
		worker.thread = std::thread([] { worker.run(); });
		return true;
	}
	catch (...)
	{
		worker.stopping = true;
		return false;
	}
}
account_load_job *account_load_submit(account_load_request request)
{
	const uint64_t now = account_load_now_usec();
	if (!request.id || request.name.empty() || request.name.size() > 255 ||
	    request.name.find('\0') != std::string::npos || request.deadline_usec <= now ||
	    request.deadline_usec - now > ACCOUNT_LOAD_TIMEOUT_USEC)
		return nullptr;
	std::lock_guard<std::mutex> lock(worker.mutex);
	if (worker.stopping || worker.jobs.size() >= ACCOUNT_LOAD_MAX_PENDING)
		return nullptr;
	for (const auto &job : worker.jobs)
		if (job->request.id == request.id)
			return nullptr;
	try
	{
		auto job = std::make_unique<account_load_job>();
		job->request = std::move(request);
		account_load_job *handle = job.get();
		worker.jobs.push_back(std::move(job));
		worker.available.notify_one();
		return handle;
	}
	catch (...)
	{
		return nullptr;
	}
}
bool account_load_poll(account_load_job *job, account_load_result *result)
{
	std::lock_guard<std::mutex> lock(worker.mutex);
	if (!job || !job->done || !result)
		return false;
	*result = std::move(job->result);
	return true;
}
void account_load_release(account_load_job *job)
{
	if (!job)
		return;
	std::lock_guard<std::mutex> lock(worker.mutex);
	if (!job->started || job->done)
		worker.erase(job);
	else
		job->cancelled = true;
}
void account_load_worker_shutdown()
{
	worker.shutdown();
}

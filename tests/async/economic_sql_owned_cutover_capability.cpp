#include "persistence/critical_command_coordinator.h"
#include "persistence/economic_sql_lifecycle_guard.h"

#include <mysql/mysql.h>

#include <cassert>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int close_fault = 0;
static std::atomic<unsigned int> commit_attempts = 0;
static std::atomic<unsigned int> rollback_attempts = 0;
static std::atomic<unsigned int> release_lock_query_attempts = 0;
static std::atomic<unsigned int> release_lock_query_failures = 0;
static std::atomic<unsigned int> fail_release_lock_query_at = 0;

struct sql_terminal_barrier
{
	std::mutex mutex;
	std::condition_variable changed;
	bool entered = false;
	bool released = false;
	bool timed_out = false;
};

static std::atomic<sql_terminal_barrier *> active_sql_terminal_barrier = nullptr;

void pause_before_terminal_sql()
{
	sql_terminal_barrier *barrier = active_sql_terminal_barrier.load(std::memory_order_acquire);
	if (!barrier)
		return;
	std::unique_lock<std::mutex> lock(barrier->mutex);
	barrier->entered = true;
	barrier->changed.notify_all();
	barrier->changed.wait(lock, [&] { return barrier->released; });
}

extern "C" decltype(mysql_commit(nullptr)) __real_mysql_commit(MYSQL *);
extern "C" decltype(mysql_rollback(nullptr)) __real_mysql_rollback(MYSQL *);
extern "C" decltype(mysql_commit(nullptr)) __wrap_mysql_commit(MYSQL *connection)
{
	++commit_attempts;
	pause_before_terminal_sql();
	return __real_mysql_commit(connection);
}
extern "C" decltype(mysql_rollback(nullptr)) __wrap_mysql_rollback(MYSQL *connection)
{
	++rollback_attempts;
	pause_before_terminal_sql();
	return __real_mysql_rollback(connection);
}

extern "C" int __real_mysql_real_query(MYSQL *, const char *, unsigned long);
extern "C" int __wrap_mysql_real_query(MYSQL *connection, const char *query, unsigned long length)
{
	static constexpr std::string_view release_lock_prefix = "SELECT RELEASE_LOCK(";
	if (query && std::string_view(query, length).starts_with(release_lock_prefix))
	{
		const unsigned int attempt = ++release_lock_query_attempts;
		if (attempt == fail_release_lock_query_at.load())
			return 1;
		unsigned int pending = release_lock_query_failures.load();
		while (pending &&
		       !release_lock_query_failures.compare_exchange_weak(pending, pending - 1))
		{
		}
		if (pending)
			return 1;
	}
	return __real_mysql_real_query(connection, query, length);
}

extern "C" int __real_close(int);
extern "C" int __wrap_close(int fd)
{
	if (close_fault)
	{
		--close_fault;
		const int result = __real_close(fd);
		assert(result == 0);
		errno = EIO;
		return -1;
	}
	return __real_close(fd);
}

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
	if ((host != "127.0.0.1" && host != "host.docker.internal") || std::getenv("DB_SOCKET") ||
	    std::string(required("ECONOMIC_SQL_LIFECYCLE_DISPOSABLE_SCHEMA")) != "1" ||
	    !schema.starts_with("economic_lifecycle_test_") ||
	    schema.find_first_not_of(
		    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") !=
		    std::string::npos)
		throw std::runtime_error("explicit disposable loopback schema required");

	char *end = nullptr;
	const unsigned long port = std::strtoul(required("DB_PORT"), &end, 10);
	if (!end || *end || !port || port > 65535)
		throw std::runtime_error("invalid disposable DB port");

	MYSQL *connection = mysql_init(nullptr);
	if (!connection)
		throw std::runtime_error("mysql_init failed");
	unsigned int timeout = 5;
	unsigned int protocol = MYSQL_PROTOCOL_TCP;
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = false;
	mysql_options(connection, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
	mysql_options(connection, MYSQL_OPT_READ_TIMEOUT, &timeout);
	mysql_options(connection, MYSQL_OPT_WRITE_TIMEOUT, &timeout);
	mysql_options(connection, MYSQL_OPT_PROTOCOL, &protocol);
	mysql_options(connection, MYSQL_OPT_RECONNECT, &reconnect);
	if (!mysql_real_connect(connection, host.c_str(), required("DB_USER"),
				required("DB_PASSWD"), schema.c_str(),
				static_cast<unsigned int>(port), nullptr, 0))
	{
		const std::string error = mysql_error(connection);
		mysql_close(connection);
		throw std::runtime_error("mysql_real_connect: " + error);
	}
	return connection;
}

void execute(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()))
		throw std::runtime_error("SQL control statement failed");
	MYSQL_RES *result = mysql_store_result(connection);
	if (result)
		mysql_free_result(result);
}

bool reconnect_disabled(MYSQL *connection)
{
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = true;
	return !mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) && !reconnect;
}

bool named_lock_owned_by(MYSQL *connection, const char *name, unsigned long session)
{
	const std::string query = "SELECT IS_USED_LOCK('" + std::string(name) + "')";
	if (mysql_real_query(connection, query.data(), query.size()))
		return false;
	MYSQL_RES *result = mysql_store_result(connection);
	if (!result)
		return false;
	const MYSQL_ROW row = mysql_fetch_row(result);
	const bool owned = row && row[0] && std::string(row[0]) == std::to_string(session);
	mysql_free_result(result);
	return owned;
}

void require(bool condition, const char *message)
{
	if (!condition)
		throw std::runtime_error(message);
}

struct close_fault_reset
{
	~close_fault_reset() { close_fault = 0; }
};

critical_command make_command(uint8_t tag)
{
	critical_command command = {};
	command.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	if (!critical_operation_id_generate(&command.operation_id))
		throw std::runtime_error("operation ID generation failed");
	command.type = critical_command_type::test;
	command.payload_version = 1;
	command.source_site = critical_source_site::command;
	command.deadline_class = critical_deadline_class::interactive;
	command.accepted_at_usec = 1700000000000000ULL + tag;
	command.keys = { { critical_entity_type::player, 91000ULL + tag } };
	command.payload = { tag };
	if (!critical_command_normalize(&command))
		throw std::runtime_error("test command normalization failed");
	return command;
}

critical_apply_result apply(const critical_command &, void *)
{
	return { critical_apply_outcome::applied, 1, 0 };
}

struct apply_barrier
{
	std::mutex mutex;
	std::condition_variable changed;
	bool entered = false;
	bool released = false;
};

critical_apply_result apply_until_released(const critical_command &, void *context)
{
	auto &barrier = *static_cast<apply_barrier *>(context);
	std::unique_lock<std::mutex> lock(barrier.mutex);
	barrier.entered = true;
	barrier.changed.notify_all();
	barrier.changed.wait(lock, [&] { return barrier.released; });
	return { critical_apply_outcome::applied, 1, 0 };
}

bool wait_for_barrier(apply_barrier &barrier)
{
	std::unique_lock<std::mutex> lock(barrier.mutex);
	barrier.changed.wait(lock, [&] { return barrier.entered || barrier.released; });
	return barrier.entered;
}

bool wait_for_issuance(const std::atomic<bool> &acquisition_finished)
{
	while (!acquisition_finished.load(std::memory_order_acquire))
	{
		if (critical_command_coordinator_health_copy().cutover_issuing)
			return true;
		std::this_thread::yield();
	}
	return false;
}

template <typename Predicate> void wait_for(Predicate condition)
{
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (!condition())
	{
		require(std::chrono::steady_clock::now() < deadline,
			"timed out waiting for coordinator");
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void wait_for_publication(const critical_command &command)
{
	wait_for(
		[&]
		{
			critical_completion completion[8] = {};
			critical_command_coordinator_pulse(completion, 8);
			critical_completion retained = {};
			return critical_command_coordinator_health_copy().publication_pending ==
				       1 &&
			       critical_command_coordinator_get_completed(command.operation_id,
									  &retained);
		});
}

void test_publication_hold_refuses_without_clearing(MYSQL *connection, const std::string &directory)
{
	economic_sql_lifecycle_guard guard;
	require(!economic_sql_lifecycle_guard::acquire_runtime(connection, &guard),
		"inactive runtime guard acquisition failed");
	require(critical_command_coordinator_init(directory.c_str(), apply, nullptr, 1),
		"coordinator initialization failed");
	const critical_command command = make_command(1);
	require(critical_command_coordinator_submit_for_publication(command) ==
			critical_submit_result::awaiting_durability,
		"publication command was not admitted");
	wait_for_publication(command);

	economic_sql_cutover_capability capability;
	require(!guard.acquire_cutover_capability(10, &capability),
		"cutover capability admitted publication-held work");
	require(!capability.is_valid_for(guard), "failed acquisition produced a usable capability");
	critical_operation_id fenced_by = {};
	require(critical_command_coordinator_is_fenced({ critical_entity_type::player, 91001 },
						       &fenced_by),
		"publication-held command lost its fence");
	require(critical_operation_id_equal(fenced_by, command.operation_id),
		"publication-held command ID changed");
	critical_completion retained = {};
	require(critical_command_coordinator_get_completed(command.operation_id, &retained) &&
			critical_operation_id_equal(retained.operation_id, command.operation_id),
		"publication-held original ID was not retained");
	require(critical_command_coordinator_health_copy().publication_pending == 1,
		"failed cutover attempt cleared publication-held work");

	// Test-only cleanup happens after the refusal and original-ID assertions;
	// it is not used to make the preflight ready.
	critical_command_coordinator_reset_for_tests();
}

void test_uncertain_append_refuses_without_clearing(MYSQL *connection, const std::string &directory)
{
	economic_sql_lifecycle_guard guard;
	require(!economic_sql_lifecycle_guard::acquire_runtime(connection, &guard),
		"inactive runtime guard acquisition failed");
	require(critical_command_coordinator_init(directory.c_str(), apply, nullptr, 1),
		"coordinator initialization failed");
	const critical_command command = make_command(2);
	close_fault = 2;
	close_fault_reset reset_close_fault;
	require(critical_command_coordinator_submit(command) ==
			critical_submit_result::awaiting_durability,
		"uncertain append command was not admitted");
	wait_for(
		[&]
		{
			return critical_command_coordinator_durability(command.operation_id) ==
			       critical_command_durability::uncertain;
		});
	close_fault = 0;

	economic_sql_cutover_capability capability;
	require(!guard.acquire_cutover_capability(10, &capability),
		"cutover capability admitted an uncertain append");
	require(!capability.is_valid_for(guard), "failed acquisition produced a usable capability");
	critical_operation_id fenced_by = {};
	require(critical_command_coordinator_is_fenced({ critical_entity_type::player, 91002 },
						       &fenced_by),
		"uncertain command lost its fence");
	require(critical_operation_id_equal(fenced_by, command.operation_id),
		"uncertain command ID changed");
	require(critical_command_journal_health_copy().append_uncertain,
		"uncertain journal append was cleared by cutover acquisition");
	require(critical_command_coordinator_durability(command.operation_id) ==
			critical_command_durability::uncertain,
		"uncertain operation identity was not retained");

	// Keep the original uncertain work untouched until all refusal assertions
	// have been made; cleanup only terminates this isolated harness case.
	critical_command_coordinator_reset_for_tests();
}

void test_idle_capability_release_and_generation(MYSQL *connection, const std::string &directory)
{
	require(critical_command_coordinator_init(directory.c_str(), apply, nullptr, 1),
		"idle coordinator initialization failed");
	economic_sql_cutover_capability original;
	{
		economic_sql_lifecycle_guard guard;
		require(!economic_sql_lifecycle_guard::acquire_runtime(connection, &guard),
			"inactive runtime guard acquisition failed");
		require(guard.acquire_cutover_capability(3000, &original),
			"idle capability acquisition failed");
		require(original.is_valid_for(guard), "idle capability failed validation");
		require(!critical_command_coordinator_try_acquire_lifecycle_guard() &&
				original.is_valid_for(guard),
			"application lifecycle crossed or invalidated an idle owner");
		critical_command_coordinator_resume();
		require(!critical_command_coordinator_health_copy().accepting &&
				original.is_valid_for(guard),
			"resume invalidated or reopened an idle held capability");
		require(!critical_command_coordinator_shutdown(),
			"shutdown tore down an idle held capability");
		require(!critical_command_coordinator_init(directory.c_str(), apply, nullptr, 1),
			"reinitialization crossed an idle held capability");
	}
	require(!critical_command_coordinator_health_copy().accepting,
		"idle guard release unexpectedly resumed the coordinator");
	critical_command_coordinator_resume();
	require(critical_command_coordinator_health_copy().accepting,
		"explicit resume failed after idle lease release");
	const critical_command resumed = make_command(11);
	require(critical_command_coordinator_submit(resumed) ==
			critical_submit_result::awaiting_durability,
		"inactive coordinator did not resume ordinary admission");
	require(critical_command_coordinator_drain(3000), "ordinary resumed work did not drain");
	require(critical_command_coordinator_shutdown(), "idle coordinator shutdown failed");
	const std::string reinit_directory = (std::filesystem::path(directory) / "reinit").string();
	require(critical_command_coordinator_init(reinit_directory.c_str(), apply, nullptr, 1),
		"idle coordinator reinitialization failed");
	{
		economic_sql_lifecycle_guard replacement;
		require(!economic_sql_lifecycle_guard::acquire_runtime(connection, &replacement),
			"replacement runtime guard acquisition failed");
		economic_sql_cutover_capability fresh;
		require(replacement.acquire_cutover_capability(3000, &fresh),
			"replacement capability acquisition failed");
		require(!original.is_valid_for(replacement) && fresh.is_valid_for(replacement),
			"stale capability revived after shutdown/reinit");
	}
	critical_command_coordinator_resume();
	require(critical_command_coordinator_shutdown(), "replacement coordinator shutdown failed");
}

void test_issuance_resume_retention(MYSQL *connection, const std::string &directory)
{
	apply_barrier barrier;
	require(critical_command_coordinator_init(directory.c_str(), apply_until_released, &barrier,
						  1),
		"coordinator initialization failed");
	economic_sql_lifecycle_guard guard;
	require(!economic_sql_lifecycle_guard::acquire_maintenance(connection, &guard),
		"inactive maintenance guard acquisition failed");
	const critical_command pending = make_command(3);
	require(critical_command_coordinator_submit(pending) ==
			critical_submit_result::awaiting_durability,
		"barrier command was not admitted");

	bool issuance_observed = false;
	bool resume_kept_closed = false;
	bool submission_denied = false;
	bool lifecycle_refused_with_pending_identity = false;
	std::atomic<bool> acquisition_finished = false;
	std::thread resumer(
		[&]
		{
			if (!wait_for_issuance(acquisition_finished) || !wait_for_barrier(barrier))
				return;
			critical_operation_id fenced_by = {};
			lifecycle_refused_with_pending_identity =
				!critical_command_coordinator_try_acquire_lifecycle_guard() &&
				critical_command_coordinator_is_fenced(pending.keys.front(),
								       &fenced_by) &&
				critical_operation_id_equal(fenced_by, pending.operation_id);
			critical_command_coordinator_resume();
			const critical_coordinator_health state =
				critical_command_coordinator_health_copy();
			issuance_observed = state.cutover_issuing;
			resume_kept_closed = !state.accepting;
			submission_denied = critical_command_coordinator_submit(make_command(4)) ==
					    critical_submit_result::unavailable;
			{
				std::lock_guard<std::mutex> lock(barrier.mutex);
				barrier.released = true;
			}
			barrier.changed.notify_all();
		});

	economic_sql_cutover_capability capability;
	const bool acquired = guard.acquire_cutover_capability(3000, &capability);
	acquisition_finished.store(true, std::memory_order_release);
	{
		std::lock_guard<std::mutex> lock(barrier.mutex);
		barrier.released = true;
	}
	barrier.changed.notify_all();
	resumer.join();
	require(acquired, "cutover issuance failed while resume raced drain");
	require(issuance_observed && resume_kept_closed && submission_denied,
		"resume reopened or admitted work during capability issuance");
	require(lifecycle_refused_with_pending_identity,
		"application lifecycle crossed issuance or lost pending operation identity");
	require(capability.is_valid_for(guard), "issued capability did not validate");
	critical_command_coordinator_resume();
	require(!critical_command_coordinator_health_copy().accepting &&
			capability.is_valid_for(guard),
		"ordinary resume revoked or reopened a held capability");
	require(critical_command_coordinator_submit(make_command(5)) ==
			critical_submit_result::unavailable,
		"held capability admitted work after resume");

	economic_sql_cutover_transaction_owner transaction;
	require(transaction.begin(guard, capability), "transaction owner failed to begin");
	require(transaction.is_valid(), "active transaction authority failed validation");
	require(critical_command_coordinator_health_copy().cutover_transaction_active,
		"coordinator did not retain active transaction ownership");
	require(transaction.commit(), "exact-session commit or terminal release failed");
	require(transaction.terminal() && !transaction.outcome_uncertain(),
		"confirmed commit did not reach owner terminal state");
	require(critical_command_coordinator_health_copy().accepting,
		"coordinator admission did not reopen after terminal commit");
	critical_command_coordinator_reset_for_tests();
}

void test_shutdown_reinit_held_transaction(MYSQL *connection, const std::string &directory)
{
	require(critical_command_coordinator_init(directory.c_str(), apply, nullptr, 1),
		"coordinator initialization failed");
	economic_sql_lifecycle_guard guard;
	require(!economic_sql_lifecycle_guard::acquire_maintenance(connection, &guard),
		"inactive maintenance guard acquisition failed");
	economic_sql_cutover_capability capability;
	require(guard.acquire_cutover_capability(3000, &capability),
		"maintenance capability acquisition failed");
	economic_sql_cutover_transaction_owner transaction;
	require(transaction.begin(guard, capability), "transaction owner failed to begin");
	require(transaction.is_valid(), "held transaction did not validate");
	const unsigned long held_session = mysql_thread_id(connection);
	require(!critical_command_coordinator_try_acquire_lifecycle_guard() &&
			transaction.is_valid() && mysql_thread_id(connection) == held_session,
		"application lifecycle invalidated a held transaction or its exact SQL session");

	critical_command_coordinator_resume();
	require(!critical_command_coordinator_health_copy().accepting,
		"resume reopened admission during a held transaction");
	require(!critical_command_coordinator_shutdown(),
		"shutdown pretended to finish an active transaction");
	require(critical_command_coordinator_health_copy().shutdown_refused,
		"shutdown refusal was not observable");
	require(!critical_command_coordinator_init(directory.c_str(), apply, nullptr, 1),
		"reinitialization crossed a held SQL transaction");
	require(critical_command_coordinator_submit(make_command(6)) ==
			critical_submit_result::unavailable,
		"submission crossed held-transaction shutdown refusal");
	require(transaction.rollback(), "exact-session rollback/terminal release failed");
	require(transaction.terminal() && critical_command_coordinator_health_copy().accepting,
		"rollback did not reopen only after its terminal owner release");
	require(critical_command_coordinator_shutdown(),
		"shutdown failed after transaction terminal");
	require(critical_command_coordinator_init(directory.c_str(), apply, nullptr, 1),
		"reinitialization failed after transaction terminal");
	require(critical_command_coordinator_shutdown(), "final coordinator shutdown failed");
}

void test_lifecycle_guard_exclusion(MYSQL *connection, const std::string &directory,
				    bool was_accepting)
{
	require(critical_command_coordinator_init(directory.c_str(), apply, nullptr, 1),
		"lifecycle-guard coordinator initialization failed");
	if (!was_accepting)
		critical_command_coordinator_quiesce();
	economic_sql_lifecycle_guard guard;
	require(!economic_sql_lifecycle_guard::acquire_maintenance(connection, &guard),
		"lifecycle-guard maintenance acquisition failed");
	require(critical_command_coordinator_try_acquire_lifecycle_guard(),
		"owner-free application lifecycle guard was refused");
	critical_command_coordinator_resume();
	require(!critical_command_coordinator_health_copy().accepting,
		"resume reopened admission through the application lifecycle guard");
	economic_sql_cutover_capability denied;
	require(!guard.acquire_cutover_capability(100, &denied) && !denied.is_valid_for(guard),
		"new cutover issuance crossed application teardown");
	require(!critical_command_coordinator_init(directory.c_str(), apply, nullptr, 1),
		"coordinator reinitialization crossed application teardown");
	critical_command_coordinator_release_lifecycle_guard();
	require(critical_command_coordinator_health_copy().accepting == was_accepting,
		"cancelled application lifecycle did not restore its prior admission state");
	economic_sql_cutover_capability fresh;
	require(guard.acquire_cutover_capability(3000, &fresh),
		"cutover could not acquire after application lifecycle cancellation");
	economic_sql_cutover_transaction_owner transaction;
	require(transaction.begin(guard, fresh) && transaction.rollback(),
		"post-lifecycle cutover did not complete its exact-session rollback");
	require(critical_command_coordinator_health_copy().accepting == was_accepting,
		"terminal rollback did not preserve pre-lifecycle admission state");
	critical_command_coordinator_reset_for_tests();
}

void test_terminal_reopen_order(MYSQL *connection, const std::string &directory, bool commit)
{
	require(critical_command_coordinator_init(directory.c_str(), apply, nullptr, 1),
		"coordinator initialization failed");
	economic_sql_lifecycle_guard guard;
	require(!economic_sql_lifecycle_guard::acquire_maintenance(connection, &guard),
		"inactive maintenance guard acquisition failed");
	economic_sql_cutover_capability capability;
	require(guard.acquire_cutover_capability(3000, &capability),
		"terminal-order capability acquisition failed");
	economic_sql_cutover_transaction_owner transaction;
	require(transaction.begin(guard, capability), "terminal-order transaction did not begin");
	require(transaction.is_valid(), "terminal-order active validation failed");
	sql_terminal_barrier barrier;
	bool exclusion_held_at_sql_boundary = false;
	bool submission_denied_at_sql_boundary = false;
	active_sql_terminal_barrier.store(&barrier, std::memory_order_release);
	std::thread observer(
		[&]
		{
			std::unique_lock<std::mutex> lock(barrier.mutex);
			if (!barrier.changed.wait_for(lock, std::chrono::seconds(5),
						      [&] { return barrier.entered; }))
			{
				barrier.timed_out = true;
				barrier.released = true;
				lock.unlock();
				barrier.changed.notify_all();
				return;
			}
			lock.unlock();
			const critical_coordinator_health state =
				critical_command_coordinator_health_copy();
			exclusion_held_at_sql_boundary = !state.accepting &&
							 state.cutover_transaction_active;
			submission_denied_at_sql_boundary =
				critical_command_coordinator_submit(make_command(
					commit ? 12 : 13)) == critical_submit_result::unavailable;
			lock.lock();
			barrier.released = true;
			lock.unlock();
			barrier.changed.notify_all();
		});
	const bool terminal_result = commit ? transaction.commit() : transaction.rollback();
	observer.join();
	active_sql_terminal_barrier.store(nullptr, std::memory_order_release);
	require(!barrier.timed_out && exclusion_held_at_sql_boundary &&
			submission_denied_at_sql_boundary,
		"coordinator exclusion ended before real terminal SQL returned");
	require(terminal_result, "terminal SQL statement did not complete through its owner");
	require(transaction.terminal() && critical_command_coordinator_health_copy().accepting,
		"admission did not reopen after SQL and owner terminal release");
	const critical_command post_terminal = make_command(commit ? 9 : 10);
	require(critical_command_coordinator_submit(post_terminal) ==
			critical_submit_result::awaiting_durability,
		"admission did not reopen after terminal release");
	require(critical_command_coordinator_drain(3000),
		"post-terminal coordinator work did not drain");
	critical_command_coordinator_reset_for_tests();
}

void test_terminal_cleanup_retry(MYSQL *connection, const std::string &directory, bool commit)
{
	require(critical_command_coordinator_init(directory.c_str(), apply, nullptr, 1),
		"cleanup-retry coordinator initialization failed");
	economic_sql_lifecycle_guard guard;
	require(!economic_sql_lifecycle_guard::acquire_maintenance(connection, &guard),
		"cleanup-retry maintenance acquisition failed");
	economic_sql_cutover_capability capability;
	require(guard.acquire_cutover_capability(3000, &capability),
		"cleanup-retry capability acquisition failed");
	economic_sql_cutover_transaction_owner transaction;
	require(transaction.begin(guard, capability) && transaction.is_valid(),
		"cleanup-retry transaction did not begin valid");
	const unsigned long session = mysql_thread_id(connection);
	commit_attempts = 0;
	rollback_attempts = 0;
	release_lock_query_attempts = 0;
	release_lock_query_failures = 2;

	const bool first_terminal_result = commit ? transaction.commit() : transaction.rollback();
	const auto expected = commit ? economic_sql_cutover_terminal_outcome::committed :
				       economic_sql_cutover_terminal_outcome::rolled_back;
	const critical_coordinator_health pending = critical_command_coordinator_health_copy();
	require(!first_terminal_result, "injected terminal cleanup failure was ignored");
	require(commit_attempts == static_cast<unsigned int>(commit) &&
			rollback_attempts == static_cast<unsigned int>(!commit),
		"terminal SQL attempt count did not match its direction");
	require(release_lock_query_attempts == 1 && release_lock_query_failures == 1,
		"first post-terminal RELEASE_LOCK failure was not injected");
	require(transaction.terminal_outcome() == expected && !transaction.outcome_uncertain() &&
			!transaction.terminal(),
		"known SQL outcome was not exposed while cleanup remained pending");
	require(!pending.accepting && pending.cutover_transaction_active &&
			!pending.cutover_outcome_uncertain,
		"coordinator admission reopened before post-terminal cleanup");
	require(!critical_command_coordinator_try_acquire_lifecycle_guard(),
		"application lifecycle crossed pending post-terminal cleanup");
	require(named_lock_owned_by(connection, ECONOMIC_SQL_BOOT_MAINTENANCE_LOCK_NAME, session) &&
			named_lock_owned_by(connection, "duris:economic_sql_currency_writers",
					    session),
		"injected cleanup failure released a SQL fence");
	require(critical_command_coordinator_submit(make_command(commit ? 14 : 15)) ==
			critical_submit_result::unavailable,
		"admission crossed pending terminal cleanup");

	const bool opposite_result = commit ? transaction.rollback() : transaction.commit();
	require(!opposite_result && transaction.terminal_outcome() == expected &&
			!transaction.terminal(),
		"opposite terminal method claimed a different SQL outcome");
	require(commit_attempts == static_cast<unsigned int>(commit) &&
			rollback_attempts == static_cast<unsigned int>(!commit),
		"opposite terminal method repeated or changed terminal SQL");
	require(release_lock_query_attempts == 1 && release_lock_query_failures == 1,
		"opposite terminal method attempted fence cleanup");
	const bool same_outcome_result = commit ? transaction.commit() : transaction.rollback();
	require(!same_outcome_result && transaction.terminal_outcome() == expected &&
			!transaction.terminal(),
		"matching terminal retry did not retain its resolved outcome");
	require(commit_attempts == static_cast<unsigned int>(commit) &&
			rollback_attempts == static_cast<unsigned int>(!commit),
		"matching terminal retry repeated terminal SQL");
	require(release_lock_query_attempts == 2 && release_lock_query_failures == 0 &&
			!critical_command_coordinator_health_copy().accepting &&
			named_lock_owned_by(connection, ECONOMIC_SQL_BOOT_MAINTENANCE_LOCK_NAME,
					    session) &&
			named_lock_owned_by(connection, "duris:economic_sql_currency_writers",
					    session),
		"failed matching cleanup retry released a fence or reopened admission");

	require(transaction.retry_cleanup(), "cleanup-only retry failed after terminal SQL");
	require(commit_attempts == static_cast<unsigned int>(commit) &&
			rollback_attempts == static_cast<unsigned int>(!commit),
		"cleanup-only retry repeated terminal SQL");
	require(release_lock_query_attempts == 4,
		"cleanup retry did not release both SQL fences exactly once");
	const critical_coordinator_health completed = critical_command_coordinator_health_copy();
	require(transaction.terminal() && transaction.terminal_outcome() == expected &&
			completed.accepting && !completed.cutover_transaction_active,
		"coordinator admission did not wait for complete terminal cleanup");
	require(!named_lock_owned_by(connection, ECONOMIC_SQL_BOOT_MAINTENANCE_LOCK_NAME,
				     session) &&
			!named_lock_owned_by(connection, "duris:economic_sql_currency_writers",
					     session),
		"terminal cleanup retained a SQL fence");
	{
		economic_sql_lifecycle_guard reacquired;
		require(!economic_sql_lifecycle_guard::acquire_maintenance(connection, &reacquired),
			"local or SQL maintenance fence remained after terminal cleanup");
	}
	const critical_command post_terminal = make_command(commit ? 16 : 17);
	require(critical_command_coordinator_submit(post_terminal) ==
			critical_submit_result::awaiting_durability,
		"admission did not reopen after cleanup-only retry");
	require(critical_command_coordinator_drain(3000),
		"post-cleanup coordinator work did not drain");
	critical_command_coordinator_reset_for_tests();
}

void test_retained_publication_handoff(MYSQL *connection, const std::string &directory)
{
	require(critical_command_coordinator_init(directory.c_str(), apply, nullptr, 1),
		"retained-publication coordinator initialization failed");
	economic_sql_lifecycle_guard guard;
	require(!economic_sql_lifecycle_guard::acquire_maintenance(connection, &guard),
		"retained-publication maintenance acquisition failed");
	economic_sql_cutover_capability capability;
	require(guard.acquire_cutover_capability(3000, &capability),
		"retained-publication capability acquisition failed");
	economic_sql_cutover_transaction_owner transaction;
	require(transaction.begin(guard, capability) && transaction.is_valid(),
		"retained-publication transaction did not begin valid");
	const unsigned long session = mysql_thread_id(connection);
	commit_attempts = 0;
	rollback_attempts = 0;
	release_lock_query_attempts = 0;

	require(transaction.commit_and_retain_publication(),
		"retained-publication COMMIT or post-COMMIT fence validation failed");
	require(commit_attempts == 1 && rollback_attempts == 0 &&
			transaction.terminal_outcome() ==
				economic_sql_cutover_terminal_outcome::committed &&
			transaction.publication_pending() && !transaction.outcome_uncertain() &&
			!transaction.terminal() && transaction.is_valid_for_publication(),
		"known COMMIT was conflated with publication completion");
	require(named_lock_owned_by(connection, ECONOMIC_SQL_BOOT_MAINTENANCE_LOCK_NAME, session) &&
			named_lock_owned_by(connection, "duris:economic_sql_currency_writers",
					    session),
		"retained COMMIT released a SQL fence");
	const critical_coordinator_health held = critical_command_coordinator_health_copy();
	require(!held.accepting && held.cutover_transaction_active &&
			!held.cutover_outcome_uncertain &&
			critical_command_coordinator_submit(make_command(31)) ==
				critical_submit_result::unavailable,
		"admission reopened before private publication completion");

	// The original guard was emptied by begin(), so it is the lifetime output.
	// A null/occupied-output refusal must not consume the known COMMIT or release
	// a fence; the source-contract test checks every private guard field preflight.
	require(!transaction.finish_publication(nullptr) && transaction.publication_pending() &&
			!release_lock_query_attempts &&
			named_lock_owned_by(connection, ECONOMIC_SQL_BOOT_MAINTENANCE_LOCK_NAME,
					    session),
		"failed publication output changed the retained owner");

	bool wrong_thread_valid = true;
	bool wrong_thread_finish = true;
	std::thread wrong_thread(
		[&]
		{
			wrong_thread_valid = transaction.is_valid_for_publication();
			wrong_thread_finish = transaction.finish_publication(&guard);
		});
	wrong_thread.join();
	require(!wrong_thread_valid && !wrong_thread_finish && transaction.publication_pending() &&
			!guard.is_maintenance_authority(),
		"same MYSQL owner was accepted from the wrong thread");

	// Stand-in for the future private publication callback; this is not gameplay
	// projection or qualification proof. Only explicit completion transfers fences.
	const bool publication_step_succeeded = true;
	require(publication_step_succeeded && transaction.is_valid_for_publication() &&
			transaction.finish_publication(&guard),
		"explicit publication completion did not hand off retained fences");
	const critical_coordinator_health finished = critical_command_coordinator_health_copy();
	require(transaction.terminal() && !transaction.publication_pending() &&
			transaction.terminal_outcome() ==
				economic_sql_cutover_terminal_outcome::committed &&
			finished.accepting && !finished.cutover_transaction_active &&
			guard.is_maintenance_authority() && guard.is_valid_authority(),
		"successful publication failed to retain exact maintenance lifetime authority");
	require(named_lock_owned_by(connection, ECONOMIC_SQL_BOOT_MAINTENANCE_LOCK_NAME, session) &&
			named_lock_owned_by(connection, "duris:economic_sql_currency_writers",
					    session) &&
			!release_lock_query_attempts && commit_attempts == 1 &&
			rollback_attempts == 0,
		"publication handoff unlocked, reacquired, or retried terminal SQL");
	economic_sql_lifecycle_guard denied;
	require(economic_sql_lifecycle_guard::acquire_maintenance(connection, &denied) != 0,
		"maintenance lifetime fence did not exclude a second guard");
	require(critical_command_coordinator_submit(make_command(32)) ==
				critical_submit_result::awaiting_durability &&
			critical_command_coordinator_drain(3000),
		"coordinator did not resume after explicit publication completion");
	critical_command_coordinator_reset_for_tests();
}

void test_retained_commit_requires_maintenance(MYSQL *connection, const std::string &directory)
{
	require(critical_command_coordinator_init(directory.c_str(), apply, nullptr, 1),
		"retained-maintenance coordinator initialization failed");
	economic_sql_lifecycle_guard guard;
	require(!economic_sql_lifecycle_guard::acquire_runtime(connection, &guard),
		"runtime guard acquisition failed for retained-maintenance refusal");
	economic_sql_cutover_capability capability;
	require(guard.acquire_cutover_capability(3000, &capability),
		"runtime cutover capability acquisition failed");
	economic_sql_cutover_transaction_owner transaction;
	require(transaction.begin(guard, capability), "runtime cutover did not begin");
	commit_attempts = 0;
	require(!transaction.commit_and_retain_publication() &&
			transaction.terminal_outcome() ==
				economic_sql_cutover_terminal_outcome::unresolved &&
			!transaction.publication_pending() && commit_attempts == 0,
		"retained publication accepted non-maintenance authority or issued COMMIT");
	require(transaction.rollback() && transaction.terminal() &&
			critical_command_coordinator_health_copy().accepting,
		"ordinary rollback changed after retained-mode refusal");
	critical_command_coordinator_reset_for_tests();
}

void test_partial_terminal_cleanup_retry(MYSQL *connection, const std::string &directory,
					 bool commit)
{
	require(critical_command_coordinator_init(directory.c_str(), apply, nullptr, 1),
		"partial-cleanup coordinator initialization failed");
	economic_sql_lifecycle_guard guard;
	require(!economic_sql_lifecycle_guard::acquire_maintenance(connection, &guard),
		"partial-cleanup maintenance acquisition failed");
	economic_sql_cutover_capability capability;
	require(guard.acquire_cutover_capability(3000, &capability),
		"partial-cleanup capability acquisition failed");
	economic_sql_cutover_transaction_owner transaction;
	require(transaction.begin(guard, capability) && transaction.is_valid(),
		"partial-cleanup transaction did not begin valid");
	const unsigned long session = mysql_thread_id(connection);
	commit_attempts = 0;
	rollback_attempts = 0;
	release_lock_query_attempts = 0;
	release_lock_query_failures = 0;
	fail_release_lock_query_at = 2;
	const auto expected = commit ? economic_sql_cutover_terminal_outcome::committed :
				       economic_sql_cutover_terminal_outcome::rolled_back;
	require(!(commit ? transaction.commit() : transaction.rollback()),
		"second SQL fence release failure was ignored");
	require(release_lock_query_attempts == 2 && transaction.terminal_outcome() == expected &&
			!transaction.outcome_uncertain() && !transaction.terminal(),
		"partial cleanup lost its known terminal outcome");
	require(!named_lock_owned_by(connection, "duris:economic_sql_currency_writers", session) &&
			named_lock_owned_by(connection, ECONOMIC_SQL_BOOT_MAINTENANCE_LOCK_NAME,
					    session),
		"partial cleanup did not release only the first SQL fence");
	const auto pending = critical_command_coordinator_health_copy();
	require(!pending.accepting && pending.cutover_transaction_active &&
			!critical_command_coordinator_try_acquire_lifecycle_guard(),
		"partial SQL fence cleanup reopened admission or application teardown");
	require(!(commit ? transaction.rollback() : transaction.commit()) &&
			release_lock_query_attempts == 2,
		"opposite outcome performed cleanup after a partial SQL fence release");
	require(transaction.retry_cleanup(), "partial SQL fence cleanup retry failed");
	require(commit_attempts == static_cast<unsigned int>(commit) &&
			rollback_attempts == static_cast<unsigned int>(!commit),
		"partial cleanup retry repeated terminal SQL");
	require(transaction.terminal() && transaction.terminal_outcome() == expected &&
			critical_command_coordinator_health_copy().accepting &&
			!critical_command_coordinator_health_copy().cutover_transaction_active,
		"partial cleanup did not finish before reopening admission");
	require(!named_lock_owned_by(connection, ECONOMIC_SQL_BOOT_MAINTENANCE_LOCK_NAME,
				     session) &&
			!named_lock_owned_by(connection, "duris:economic_sql_currency_writers",
					     session),
		"partial cleanup retry retained a SQL fence");
	{
		economic_sql_lifecycle_guard reacquired;
		require(!economic_sql_lifecycle_guard::acquire_maintenance(connection, &reacquired),
			"partial cleanup retained local exclusion");
	}
	fail_release_lock_query_at = 0;
	critical_command_coordinator_reset_for_tests();
}

int run_session_loss_child(const std::string &directory)
{
	try
	{
		MYSQL *connection = connect_fixture();
		MYSQL *killer = connect_fixture();
		require(critical_command_coordinator_init(directory.c_str(), apply, nullptr, 1),
			"session-loss child coordinator initialization failed");
		economic_sql_lifecycle_guard guard;
		require(!economic_sql_lifecycle_guard::acquire_maintenance(connection, &guard),
			"session-loss maintenance acquisition failed");
		economic_sql_cutover_capability capability;
		require(guard.acquire_cutover_capability(3000, &capability),
			"session-loss capability acquisition failed");
		economic_sql_cutover_transaction_owner transaction;
		require(transaction.begin(guard, capability) && transaction.is_valid(),
			"session-loss transaction did not start valid");
		const unsigned long original_session = mysql_thread_id(connection);
		execute(killer, "KILL CONNECTION " + std::to_string(original_session));
		require(!transaction.is_valid() && transaction.outcome_uncertain(),
			"lost exact session remained valid or was treated as resolved");
		const critical_coordinator_health state =
			critical_command_coordinator_health_copy();
		require(!state.accepting && state.cutover_transaction_active &&
				state.cutover_outcome_uncertain,
			"session loss released coordinator exclusion");
		critical_command_coordinator_resume();
		require(!critical_command_coordinator_health_copy().accepting,
			"resume reopened after exact-session loss");
		require(!transaction.commit() && !transaction.rollback() && !transaction.terminal(),
			"lost SQL session was replaced or falsely committed/rolled back");
		require(reconnect_disabled(connection) && mysql_ping(connection),
			"reconnect-disabled session unexpectedly reconnected");
		return 0;
	}
	catch (const std::exception &error)
	{
		fprintf(stderr, "OWNED-CUTOVER-SESSION-LOSS-ERROR %s\n", error.what());
		return 1;
	}
}

int run_retained_session_loss_child(const std::string &directory)
{
	try
	{
		MYSQL *connection = connect_fixture();
		MYSQL *killer = connect_fixture();
		require(critical_command_coordinator_init(directory.c_str(), apply, nullptr, 1),
			"retained session-loss coordinator initialization failed");
		economic_sql_lifecycle_guard guard;
		require(!economic_sql_lifecycle_guard::acquire_maintenance(connection, &guard),
			"retained session-loss maintenance acquisition failed");
		economic_sql_cutover_capability capability;
		require(guard.acquire_cutover_capability(3000, &capability),
			"retained session-loss capability acquisition failed");
		economic_sql_cutover_transaction_owner transaction;
		require(transaction.begin(guard, capability) &&
				transaction.commit_and_retain_publication(),
			"retained session-loss owner did not reach known COMMIT");
		// The known COMMIT already counts; forbid only additional terminal attempts.
		const auto committed_attempts = commit_attempts.load();
		const auto rollback_attempts_before_loss = rollback_attempts.load();
		const unsigned long session = mysql_thread_id(connection);
		execute(killer, "KILL CONNECTION " + std::to_string(session));
		economic_sql_lifecycle_guard handoff;
		require(!transaction.is_valid_for_publication() &&
				!transaction.finish_publication(&handoff) &&
				transaction.publication_pending() &&
				transaction.terminal_outcome() ==
					economic_sql_cutover_terminal_outcome::committed &&
				!transaction.outcome_uncertain(),
			"lost post-COMMIT session was published or confused with unknown COMMIT");
		require(!transaction.commit() && !transaction.rollback() &&
				!transaction.retry_cleanup() &&
				commit_attempts == committed_attempts &&
				rollback_attempts == rollback_attempts_before_loss,
			"lost retained session retried SQL or released through ordinary cleanup");
		const critical_coordinator_health state =
			critical_command_coordinator_health_copy();
		require(!state.accepting && state.cutover_transaction_active &&
				!state.cutover_outcome_uncertain &&
				critical_command_coordinator_submit(make_command(33)) ==
					critical_submit_result::unavailable &&
				!named_lock_owned_by(
					killer, ECONOMIC_SQL_BOOT_MAINTENANCE_LOCK_NAME, session) &&
				!named_lock_owned_by(killer, "duris:economic_sql_currency_writers",
						     session),
			"lost fence reopened admission or remained owned by the dead SQL session");
		mysql_close(killer);
		mysql_close(connection);
		return 0;
	}
	catch (const std::exception &error)
	{
		fprintf(stderr, "OWNED-CUTOVER-RETAINED-SESSION-LOSS-ERROR %s\n", error.what());
		return 1;
	}
}

int run_retained_destructor_child(const std::string &directory)
{
	try
	{
		MYSQL *connection = connect_fixture();
		require(critical_command_coordinator_init(directory.c_str(), apply, nullptr, 1),
			"retained destructor coordinator initialization failed");
		const unsigned long session = mysql_thread_id(connection);
		{
			economic_sql_lifecycle_guard guard;
			require(!economic_sql_lifecycle_guard::acquire_maintenance(connection,
										   &guard),
				"retained destructor maintenance acquisition failed");
			economic_sql_cutover_capability capability;
			require(guard.acquire_cutover_capability(3000, &capability),
				"retained destructor capability acquisition failed");
			economic_sql_cutover_transaction_owner transaction;
			require(transaction.begin(guard, capability) &&
					transaction.commit_and_retain_publication() &&
					transaction.publication_pending(),
				"retained destructor did not reach publication-pending COMMIT");
		}
		const critical_coordinator_health state =
			critical_command_coordinator_health_copy();
		require(!state.accepting && state.cutover_transaction_active &&
				!state.cutover_outcome_uncertain &&
				critical_command_coordinator_submit(make_command(34)) ==
					critical_submit_result::unavailable &&
				named_lock_owned_by(connection,
						    ECONOMIC_SQL_BOOT_MAINTENANCE_LOCK_NAME,
						    session) &&
				named_lock_owned_by(connection,
						    "duris:economic_sql_currency_writers", session),
			"destruction became an implicit publication recovery or released a fence");
		mysql_close(connection);
		return 0;
	}
	catch (const std::exception &error)
	{
		fprintf(stderr, "OWNED-CUTOVER-RETAINED-DESTRUCTOR-ERROR %s\n", error.what());
		return 1;
	}
}

void test_retained_failure_in_subprocess(const char *program, const std::string &directory,
					 const char *mode)
{
	const pid_t child = fork();
	require(child >= 0, "retained-publication subprocess fork failed");
	if (!child)
	{
		execl(program, program, mode, directory.c_str(), static_cast<char *>(nullptr));
		std::_Exit(127);
	}
	int status = 0;
	require(waitpid(child, &status, 0) == child && WIFEXITED(status) &&
			WEXITSTATUS(status) == 0,
		"retained-publication failure subprocess did not fail closed");
}

void test_session_loss_in_subprocess(const char *program, const std::string &directory)
{
	const pid_t child = fork();
	require(child >= 0, "session-loss subprocess fork failed");
	if (!child)
	{
		execl(program, program, "--session-loss-child", directory.c_str(),
		      static_cast<char *>(nullptr));
		std::_Exit(127);
	}
	int status = 0;
	require(waitpid(child, &status, 0) == child && WIFEXITED(status) &&
			WEXITSTATUS(status) == 0,
		"exact-session loss subprocess did not fail closed");
}
} // namespace

int main(int argc, char **argv)
{
	static_assert(!std::is_copy_constructible_v<economic_sql_cutover_capability>);
	static_assert(!std::is_copy_assignable_v<economic_sql_cutover_capability>);
	static_assert(!std::is_move_constructible_v<economic_sql_cutover_capability>);
	static_assert(!std::is_copy_constructible_v<economic_sql_lifecycle_guard>);
	static_assert(!std::is_copy_constructible_v<economic_sql_cutover_transaction_owner>);
	static_assert(!std::is_move_constructible_v<economic_sql_cutover_transaction_owner>);
	if (argc == 3 && std::string(argv[1]) == "--session-loss-child")
	{
		if (mysql_library_init(0, nullptr, nullptr))
			std::_Exit(2);
		const int result = run_session_loss_child(argv[2]);
		std::fflush(nullptr);
		std::_Exit(result);
	}
	if (argc == 3 && (std::string(argv[1]) == "--retained-session-loss-child" ||
			  std::string(argv[1]) == "--retained-destructor-child"))
	{
		if (mysql_library_init(0, nullptr, nullptr))
			std::_Exit(2);
		const int result = std::string(argv[1]) == "--retained-session-loss-child" ?
					   run_retained_session_loss_child(argv[2]) :
					   run_retained_destructor_child(argv[2]);
		std::fflush(nullptr);
		std::_Exit(result);
	}
	if (argc != 2)
		return 2;

	MYSQL *connection = nullptr;
	if (mysql_library_init(0, nullptr, nullptr))
		return 2;
	try
	{
		const std::filesystem::path root(argv[1]);
		std::filesystem::remove_all(root);
		require(std::filesystem::create_directories(root), "test root creation failed");
		connection = connect_fixture();
		test_publication_hold_refuses_without_clearing(connection,
							       (root / "publication").string());
		puts("OWNED-CUTOVER-PASS publication-held work and original IDs preserved");
		test_uncertain_append_refuses_without_clearing(connection,
							       (root / "uncertain").string());
		puts("OWNED-CUTOVER-PASS uncertain journal work and original IDs preserved");
		test_idle_capability_release_and_generation(connection,
							    (root / "idle-leases").string());
		puts("OWNED-CUTOVER-PASS idle lease release, explicit resume, and generation invalidation");
		test_issuance_resume_retention(connection, (root / "issuance-resume").string());
		puts("OWNED-CUTOVER-PASS issuance and resume retained through exact-session commit");
		test_shutdown_reinit_held_transaction(connection,
						      (root / "shutdown-reinit").string());
		puts("OWNED-CUTOVER-PASS shutdown/reinit refused until held transaction terminal");
		test_lifecycle_guard_exclusion(connection, (root / "lifecycle-open").string(),
					       true);
		test_lifecycle_guard_exclusion(connection, (root / "lifecycle-quiesced").string(),
					       false);
		puts("OWNED-CUTOVER-PASS lifecycle guard excludes owners and preserves prior admission");
		test_terminal_reopen_order(connection, (root / "commit-order").string(), true);
		test_terminal_reopen_order(connection, (root / "rollback-order").string(), false);
		puts("OWNED-CUTOVER-PASS commit/rollback terminal release before coordinator reopen");
		test_terminal_cleanup_retry(connection, (root / "commit-cleanup-retry").string(),
					    true);
		test_terminal_cleanup_retry(connection, (root / "rollback-cleanup-retry").string(),
					    false);
		puts("OWNED-CUTOVER-PASS post-COMMIT/ROLLBACK cleanup retry preserves outcome and fences");
		test_partial_terminal_cleanup_retry(
			connection, (root / "commit-partial-cleanup").string(), true);
		test_partial_terminal_cleanup_retry(
			connection, (root / "rollback-partial-cleanup").string(), false);
		puts("OWNED-CUTOVER-PASS second SQL fence failure preserves terminal outcome until cleanup");
		test_retained_publication_handoff(connection,
						  (root / "retained-publication").string());
		puts("OWNED-CUTOVER-SOURCE retained COMMIT, publication admission, and lifetime handoff");
		test_retained_commit_requires_maintenance(
			connection, (root / "retained-requires-maintenance").string());
		puts("OWNED-CUTOVER-SOURCE retained mode refuses ordinary runtime authority");
		mysql_close(connection);
		connection = nullptr;
		test_session_loss_in_subprocess(argv[0], (root / "session-loss").string());
		puts("OWNED-CUTOVER-PASS exact-session loss/reconnect refusal retained uncertain lease");
		test_retained_failure_in_subprocess(argv[0],
						    (root / "retained-session-loss").string(),
						    "--retained-session-loss-child");
		puts("OWNED-CUTOVER-SOURCE lost post-COMMIT session keeps known outcome and admission closed");
		test_retained_failure_in_subprocess(argv[0],
						    (root / "retained-destructor").string(),
						    "--retained-destructor-child");
		puts("OWNED-CUTOVER-SOURCE unresolved publication destruction is not recovery");
		mysql_library_end();
		return 0;
	}
	catch (const std::exception &error)
	{
		critical_command_coordinator_reset_for_tests();
		if (connection)
			mysql_close(connection);
		mysql_library_end();
		fprintf(stderr, "OWNED-CUTOVER-ERROR %s\n", error.what());
		return 1;
	}
}

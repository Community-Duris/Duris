// Standalone repository tests. SQL execution resets only an acknowledged,
// disposable loopback database. The private connection factory is a fixture
// seam; the real production factory has separate trust-boundary tests.
#include "telemetry/telemetry_repository.h"
#include "persistence/persistence_mode.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <thread>

static const char *case_name = "startup";
#define CHECK(condition)                                                                           \
	do                                                                                         \
	{                                                                                          \
		if (!(condition))                                                                  \
		{                                                                                  \
			std::fprintf(stderr, "FAIL %s:%d: %s\n", case_name, __LINE__, #condition); \
			std::exit(1);                                                              \
		}                                                                                  \
	} while (0)
static persistence_mode mode = PERSISTENCE_MODE_MARIADB_PRIMARY;
persistence_mode persistence_mode_get(void)
{
	return mode;
}

static telemetry_repository_config repository_config()
{
	return { telemetry_storage_backend::sql, 0U, TELEMETRY_SCHEMA_VERSION,
		 TELEMETRY_BATCH_MAX_RECORDS_PROPOSAL, TELEMETRY_BATCH_MAX_BYTES_PROPOSAL };
}

#ifdef __NO_MYSQL__
int main()
{
	case_name = "no-mysql";
	telemetry_repository_shutdown();
	auto config = repository_config();
	config.schema_version = 0;
	CHECK(telemetry_repository_init(config) == telemetry_repository_outcome::invalid_config);
	config = repository_config();
	CHECK(telemetry_repository_init(config) == telemetry_repository_outcome::flatfile_disabled);
	CHECK(telemetry_repository_health_copy().state == telemetry_health_state::disabled);
	CHECK(telemetry_repository_apply(nullptr, 1).outcome == telemetry_batch_outcome::disabled);
	telemetry_repository_request_stop();
	telemetry_repository_shutdown();
	std::puts("SQL-free repository runtime: PASS");
}
#else
#include <mysql.h>
#include <new>
#include <fstream>
#include <string>
#include <vector>

static unsigned int fixture_port = 3306U;
static MYSQL *observer = nullptr;
static MYSQL *sink = nullptr;
static unsigned int factory_calls = 0;
static bool factory_unavailable = false;
static std::atomic<bool> factory_block{ false };
static std::atomic<bool> factory_entered{ false };
static std::atomic<bool> factory_release{ false };
enum class fault_kind
{
	none,
	statement,
	allocation,
	deadlock,
	rollback_lost,
	commit_lost_committed,
	commit_lost_rolled_back
};
static fault_kind fault = fault_kind::none;
static bool rollback_pending = false;
static unsigned int fault_insert_skips = 0;
static MYSQL *injected_handle = nullptr;
static unsigned int injected_error = 0;
static unsigned int query_calls = 0;
static std::atomic<bool> fail_next_allocation{ false };
extern "C" void *__real__Znwm(std::size_t);
extern "C" void *__wrap__Znwm(std::size_t size)
{
	if (fail_next_allocation.exchange(false))
		throw std::bad_alloc();
	return __real__Znwm(size);
}

extern "C" int __real_mysql_real_query(MYSQL *, const char *, unsigned long);
extern "C" unsigned int __real_mysql_errno(MYSQL *);
extern "C" unsigned int __wrap_mysql_errno(MYSQL *connection)
{
	return connection == injected_handle && injected_error ? injected_error :
								 __real_mysql_errno(connection);
}
extern "C" int __wrap_mysql_real_query(MYSQL *connection, const char *query, unsigned long length)
{
	if (connection != sink)
		return __real_mysql_real_query(connection, query, length);
	++query_calls;
	injected_error = 0;
	const std::string sql(query, length);
	if (sql == "COMMIT" && (fault == fault_kind::commit_lost_committed ||
				fault == fault_kind::commit_lost_rolled_back))
	{
		const bool committed = fault == fault_kind::commit_lost_committed;
		fault = fault_kind::none;
		CHECK(__real_mysql_real_query(connection, committed ? "COMMIT" : "ROLLBACK",
					      committed ? 6 : 8) == 0);
		injected_handle = connection;
		injected_error = 2013U;
		return 1;
	}
	if (rollback_pending && sql == "ROLLBACK")
	{
		rollback_pending = false;
		CHECK(__real_mysql_real_query(connection, "ROLLBACK", 8) == 0);
		injected_handle = connection;
		injected_error = 2013U;
		return 1;
	}
	if (sql.find("INSERT INTO telemetry_interval") != std::string::npos &&
	    (fault == fault_kind::statement || fault == fault_kind::deadlock ||
	     fault == fault_kind::rollback_lost || fault == fault_kind::allocation))
	{
		if (fault_insert_skips)
		{
			--fault_insert_skips;
			return __real_mysql_real_query(connection, query, length);
		}
		const auto selected = fault;
		fault = fault_kind::none;
		if (selected == fault_kind::allocation)
			throw std::bad_alloc();
		if (selected == fault_kind::deadlock)
			CHECK(__real_mysql_real_query(connection, "ROLLBACK", 8) == 0);
		rollback_pending = selected == fault_kind::rollback_lost;
		injected_handle = connection;
		injected_error = selected == fault_kind::deadlock ? 1213U : 1205U;
		return 1;
	}
	return __real_mysql_real_query(connection, query, length);
}

static void execute(const std::string &sql)
{
	if (__real_mysql_real_query(observer, sql.c_str(), sql.size()) != 0)
	{
		std::fprintf(stderr, "Fixture SQL failed with numeric error %u\n",
			     __real_mysql_errno(observer));
		std::exit(1);
	}
}
static unsigned long long scalar(const char *sql)
{
	execute(sql);
	MYSQL_RES *result = mysql_store_result(observer);
	CHECK(result != nullptr);
	MYSQL_ROW row = mysql_fetch_row(result);
	CHECK(row != nullptr && row[0] != nullptr);
	const auto number = std::strtoull(row[0], nullptr, 10);
	mysql_free_result(result);
	return number;
}

static void shutdown_fixture()
{
	telemetry_repository_shutdown();
	// mysql_close sends QUIT; server-side advisory lock release can finish
	// after the client returns. Gate fixture restarts on bounded cleanup.
	CHECK(scalar("SELECT GET_LOCK(CONCAT('duris.telemetry.',MD5(DATABASE())),2)") == 1U);
	CHECK(scalar("SELECT RELEASE_LOCK(CONCAT('duris.telemetry.',MD5(DATABASE())))") == 1U);
}

MYSQL *sql_open_telemetry_connection(void)
{
	++factory_calls;
	if (factory_block.load(std::memory_order_acquire))
	{
		factory_entered.store(true, std::memory_order_release);
		while (!factory_release.load(std::memory_order_acquire))
			std::this_thread::yield();
	}
	if (factory_unavailable)
		return nullptr;
	MYSQL *connection = mysql_init(nullptr);
	CHECK(connection != nullptr);
	unsigned int timeout = 2;
	mysql_options(connection, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
	if (!mysql_real_connect(connection, "127.0.0.1", "root", "", "duris_telemetry_test",
				fixture_port, nullptr, 0))
	{
		mysql_close(connection);
		return nullptr;
	}
	for (const char *sql : { "SET SESSION sql_mode='STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION'",
				 "SET SESSION time_zone='+00:00'",
				 "SET SESSION TRANSACTION ISOLATION LEVEL READ COMMITTED" })
		CHECK(__real_mysql_real_query(connection, sql, std::strlen(sql)) == 0);
	sink = connection;
	return connection;
}

static void reset_fixture()
{
	shutdown_fixture();
	sink = nullptr;
	fault = fault_kind::none;
	rollback_pending = false;
	fault_insert_skips = 0;
	injected_error = 0;
	factory_unavailable = false;
	mode = PERSISTENCE_MODE_MARIADB_PRIMARY;
	for (const char *table :
	     { "telemetry_interval", "telemetry_session", "telemetry_config",
	       "telemetry_player_day", "telemetry_cohort_day", "telemetry_rollup_state" })
		execute(std::string("DELETE FROM ") + table);
	CHECK(telemetry_repository_init(repository_config()) ==
	      telemetry_repository_outcome::ready);
}
static telemetry_apply_batch_result expect_one(const telemetry_record &record,
					       telemetry_apply_outcome expected)
{
	const auto result = telemetry_repository_apply(&record, 1U);
	if (result.result_count != 1 || result.results[0].outcome != expected)
		std::fprintf(stderr, "Expected record outcome %u, got %u; batch %u; error %u\n",
			     unsigned(expected), unsigned(result.results[0].outcome),
			     unsigned(result.outcome), result.error_code);
	CHECK(result.result_count == 1U);
	CHECK(result.results[0].outcome == expected);
	return result;
}
#include "telemetry_repository_golden.inc"

static telemetry_record interval_record()
{
	for (const auto &record : normal_interval_records)
		if (record.header.kind == telemetry_record_kind::interval)
			return record;
	CHECK(false);
	return {};
}
static telemetry_record checkpoint_record(unsigned int revision, unsigned long long total,
					  unsigned long long sequence)
{
	const auto interval = interval_record();
	telemetry_record record{};
	record.header = interval.header;
	record.header.kind = telemetry_record_kind::session_checkpoint;
	record.header.key.record_seq = sequence;
	auto &checkpoint = record.payload.checkpoint;
	checkpoint.session = interval.payload.interval.session;
	checkpoint.connection = interval.payload.interval.connection;
	checkpoint.revision = revision;
	checkpoint.at_monotonic_usec = total;
	checkpoint.at_utc_usec = interval.header.occurrence_utc_usec;
	checkpoint.cumulative = { total, total, 0, 0, total, 0 };
	checkpoint.config_id = interval.payload.interval.config_id;
	CHECK(telemetry_record_is_valid(record));
	return record;
}
static void seed_config()
{
	expect_one(normal_interval_configs[0], telemetry_apply_outcome::applied);
}

static void replay_and_isolation_tests()
{
	case_name = "padding and inactive union bytes are not identity";
	reset_fixture();
	seed_config();
	auto original = checkpoint_record(1, 100, 10);
	expect_one(original, telemetry_apply_outcome::applied);
	auto replay = original;
	// Deliberately alter padding between kind/reserved and the 64-bit replay key.
	auto *bytes = reinterpret_cast<unsigned char *>(&replay.header);
	for (std::size_t i =
		     offsetof(telemetry_record_header, reserved) + sizeof(replay.header.reserved);
	     i < offsetof(telemetry_record_header, key); ++i)
		bytes[i] = 0xA5;
	auto *payload = reinterpret_cast<unsigned char *>(&replay.payload);
	for (std::size_t i = sizeof(telemetry_session_checkpoint_payload);
	     i < sizeof(telemetry_record_payload); ++i)
		payload[i] = 0x5A;
	CHECK(std::memcmp(&original, &replay, sizeof(original)) != 0);
	expect_one(replay, telemetry_apply_outcome::duplicate_identical);
	replay.header.occurrence_utc_usec++;
	expect_one(replay, telemetry_apply_outcome::duplicate_conflict);
	CHECK(scalar("SELECT COUNT(*) FROM telemetry_interval WHERE record_kind=3") == 1U);

	case_name = "mixed conflict malformed and valid commit independently";
	auto malformed = interval_record();
	malformed.header.key.record_seq = 11;
	malformed.payload.interval.duration_usec++;
	auto good = interval_record();
	good.header.key.record_seq = 12;
	const telemetry_record batch[] = { replay, malformed, good };
	const auto result = telemetry_repository_apply(batch, std::size(batch));
	CHECK(result.outcome == telemetry_batch_outcome::committed_with_rejections);
	CHECK(result.conflict_count == 1 && result.invalid_count == 1 && result.applied_count == 1);
	CHECK(result.results[0].outcome == telemetry_apply_outcome::duplicate_conflict);
	CHECK(result.results[1].outcome == telemetry_apply_outcome::rejected_invalid);
	CHECK(result.results[2].outcome == telemetry_apply_outcome::applied);
	CHECK(scalar("SELECT COUNT(*) FROM telemetry_interval") == 3U);
}

static void config_and_scope_tests()
{
	case_name = "missing config";
	reset_fixture();
	expect_one(interval_record(), telemetry_apply_outcome::rejected_invalid);
	CHECK(scalar("SELECT COUNT(*) FROM telemetry_session") == 0U);
	CHECK(scalar("SELECT COUNT(*) FROM telemetry_interval") == 0U);
	case_name = "bad configuration SHA-256";
	auto config = normal_interval_configs[0];
	config.payload.configuration.config.fingerprint[0] ^= 1U;
	expect_one(config, telemetry_apply_outcome::rejected_invalid);
	CHECK(scalar("SELECT COUNT(*) FROM telemetry_config") == 0U);
	seed_config();
	case_name = "conflicting configuration identity";
	config = normal_interval_configs[0];
	config.header.key.record_seq += 100;
	config.payload.configuration.config.effective_utc_usec++;
	expect_one(config, telemetry_apply_outcome::duplicate_conflict);
	CHECK(scalar("SELECT COUNT(*) FROM telemetry_config") == 1U);
	for (int change = 0; change < 4; ++change)
	{
		case_name = "config version or scope mismatch";
		auto record = interval_record();
		record.header.key.record_seq += 100 + change;
		if (change == 0)
			record.payload.interval.classifier_version++;
		if (change == 1)
			record.payload.interval.policy_version++;
		if (change == 2)
			record.payload.interval.session.season_id++;
		if (change == 3)
			record.payload.interval.session.environment_id++;
		expect_one(record, telemetry_apply_outcome::rejected_invalid);
	}
	case_name = "session immutable subject and pid";
	expect_one(interval_record(), telemetry_apply_outcome::applied);
	for (int change = 0; change < 2; ++change)
	{
		auto record = interval_record();
		record.header.key.record_seq += 200 + change;
		if (change == 0)
			record.payload.interval.session.subject_id++;
		else
			record.payload.interval.session.pid++;
		expect_one(record, telemetry_apply_outcome::rejected_invalid);
	}
}

static void global_scope_tests()
{
	case_name = "session id scope cannot change even with a matching valid config";
	reset_fixture();
	seed_config();
	expect_one(interval_record(), telemetry_apply_outcome::applied);
	for (const auto &config : { changed_environment_id_config, changed_season_id_config })
	{
		expect_one(config, telemetry_apply_outcome::applied);
		auto changed = interval_record();
		const auto &snapshot = config.payload.configuration.config;
		changed.header.key.record_seq = snapshot.config_id;
		changed.payload.interval.session.environment_id = snapshot.environment_id;
		changed.payload.interval.session.season_id = snapshot.season_id;
		changed.payload.interval.config_id = snapshot.config_id;
		expect_one(changed, telemetry_apply_outcome::rejected_invalid);
	}
	CHECK(scalar("SELECT COUNT(*) FROM telemetry_session") == 1U);
	CHECK(scalar("SELECT COUNT(*) FROM telemetry_interval WHERE record_kind=1") == 1U);
}

static void checkpoint_tests()
{
	case_name = "newer checkpoint cannot regress any cumulative counter";
	for (int field = 0; field < 6; ++field)
	{
		reset_fixture();
		seed_config();
		auto first = checkpoint_record(2, 100, 10);
		first.payload.checkpoint.cumulative = { 100, 40, 30, 30, 120, 20 };
		expect_one(first, telemetry_apply_outcome::applied);
		auto regression = checkpoint_record(3, 200, 11);
		telemetry_cumulative_counters totals[] = {
			{ 99, 40, 30, 29, 120, 21 },  { 200, 39, 80, 81, 240, 40 },
			{ 200, 80, 29, 91, 240, 40 }, { 200, 80, 91, 29, 240, 40 },
			{ 100, 40, 30, 30, 119, 19 }, { 200, 80, 60, 60, 219, 19 }
		};
		regression.payload.checkpoint.cumulative = totals[field];
		CHECK(telemetry_record_is_valid(regression));
		expect_one(regression, telemetry_apply_outcome::duplicate_conflict);
		CHECK(scalar("SELECT latest_revision FROM telemetry_session") == 2U);
		CHECK(scalar("SELECT COUNT(*) FROM telemetry_interval WHERE record_kind=3") == 1U);
	}
	case_name = "historical same revision conflict cannot hide behind newer revision";
	reset_fixture();
	seed_config();
	expect_one(checkpoint_record(2, 200, 10), telemetry_apply_outcome::applied);
	expect_one(checkpoint_record(1, 100, 11), telemetry_apply_outcome::checkpoint_older);
	expect_one(checkpoint_record(1, 101, 12), telemetry_apply_outcome::duplicate_conflict);
	expect_one(checkpoint_record(1, 100, 13), telemetry_apply_outcome::checkpoint_older);
	CHECK(scalar("SELECT latest_revision FROM telemetry_session") == 2U);
	CHECK(scalar("SELECT connected_usec FROM telemetry_session") == 200U);
}

static void fault_tests()
{
	for (auto selected :
	     { fault_kind::statement, fault_kind::deadlock, fault_kind::rollback_lost,
	       fault_kind::commit_lost_committed, fault_kind::commit_lost_rolled_back })
	{
		case_name = "transaction fault retains immutable batch and reconciles";
		reset_fixture();
		// Config and interval share the transaction: failures cannot orphan either side.
		const telemetry_record batch[] = { normal_interval_configs[0], interval_record() };
		fault = selected;
		fault_insert_skips = 1;
		const auto failed = telemetry_repository_apply(batch, std::size(batch));
		CHECK(fault == fault_kind::none);
		CHECK(failed.outcome == telemetry_batch_outcome::retryable_failure ||
		      failed.outcome == telemetry_batch_outcome::commit_ambiguous);
		if (selected == fault_kind::commit_lost_committed ||
		    selected == fault_kind::commit_lost_rolled_back)
			CHECK(failed.outcome == telemetry_batch_outcome::commit_ambiguous);
		CHECK(failed.applied_count == 0 && failed.duplicate_count == 0);
		const auto durable_count = selected == fault_kind::commit_lost_committed ? 2U : 0U;
		CHECK(scalar("SELECT COUNT(*) FROM telemetry_interval") == durable_count);
		CHECK(scalar("SELECT COUNT(*) FROM telemetry_config") == (durable_count ? 1U : 0U));
		auto changed = batch[1];
		changed.header.key.record_seq += 100;
		const unsigned int before_queries = query_calls;
		const auto different = telemetry_repository_apply(&changed, 1);
		CHECK(different.outcome != telemetry_batch_outcome::committed &&
		      different.outcome != telemetry_batch_outcome::committed_with_rejections);
		CHECK(query_calls == before_queries);
		telemetry_record same_keys[] = { batch[0], batch[1] };
		same_keys[1].payload.interval.dimensions.zone_vnum++;
		const auto changed_payload =
			telemetry_repository_apply(same_keys, std::size(same_keys));
		CHECK(changed_payload.outcome == telemetry_batch_outcome::invalid_batch);
		CHECK(query_calls == before_queries);
		telemetry_record reordered[] = { batch[1], batch[0] };
		CHECK(telemetry_repository_apply(reordered, std::size(reordered)).outcome ==
		      telemetry_batch_outcome::invalid_batch);
		CHECK(query_calls == before_queries);
		// The caller may release its original array: only semantic values are retained.
		telemetry_record retry[] = { batch[0], batch[1] };
		auto *padding = reinterpret_cast<unsigned char *>(&retry[1].header);
		for (std::size_t i = offsetof(telemetry_record_header, reserved) +
				     sizeof(retry[1].header.reserved);
		     i < offsetof(telemetry_record_header, key); ++i)
			padding[i] = 0xA5;
		const auto reconciled = telemetry_repository_apply(retry, std::size(retry));
		CHECK(reconciled.outcome == telemetry_batch_outcome::committed);
		CHECK(reconciled.applied_count == (durable_count ? 0U : 2U));
		CHECK(reconciled.duplicate_count == (durable_count ? 2U : 0U));
		CHECK(scalar("SELECT COUNT(*) FROM telemetry_interval") == 2U);
		CHECK(scalar("SELECT COUNT(*) FROM telemetry_config") == 1U);
		CHECK(scalar("SELECT COUNT(*) FROM telemetry_session") == 1U);
		expect_one(changed, telemetry_apply_outcome::applied);
	}
}

static telemetry_apply_batch_result apply_without_allocation_escape(const telemetry_record *records,
								    std::size_t count)
{
	telemetry_apply_batch_result result{};
	bool escaped = false;
	try
	{
		result = telemetry_repository_apply(records, count);
	}
	catch (const std::bad_alloc &)
	{
		escaped = true;
	}
	CHECK(!escaped);
	return result;
}

static void allocation_failure_tests()
{
	for (bool after_first_fact : { false, true })
	{
		case_name = after_first_fact ?
				    "allocation failure after first fact rolls back whole batch" :
				    "first allocation failure preserves bounded immutable batch";
		reset_fixture();
		const telemetry_record batch[] = { normal_interval_configs[0], interval_record() };
		if (after_first_fact)
		{
			fault = fault_kind::allocation;
			fault_insert_skips = 1;
		}
		else
			fail_next_allocation.store(true);
		const auto failed = apply_without_allocation_escape(batch, std::size(batch));
		CHECK(!fail_next_allocation.load());
		CHECK(fault == fault_kind::none);
		CHECK(failed.outcome == telemetry_batch_outcome::retryable_failure);
		CHECK(failed.applied_count == 0 && failed.duplicate_count == 0);
		CHECK(failed.result_count == std::size(batch));
		for (std::size_t i = 0; i < std::size(batch); ++i)
			CHECK(failed.results[i].outcome ==
			      telemetry_apply_outcome::retryable_failure);
		CHECK(scalar("SELECT COUNT(*) FROM telemetry_interval") == 0U);
		CHECK(scalar("SELECT COUNT(*) FROM telemetry_config") == 0U);
		CHECK(scalar("SELECT COUNT(*) FROM telemetry_session") == 0U);
		// A second allocation failure while comparing/retrying pending values is
		// still contained, and must not replace the original retained batch.
		fail_next_allocation.store(true);
		const auto retry_failed = apply_without_allocation_escape(batch, std::size(batch));
		CHECK(!fail_next_allocation.load());
		CHECK(retry_failed.outcome == telemetry_batch_outcome::retryable_failure);
		CHECK(scalar("SELECT COUNT(*) FROM telemetry_interval") == 0U);
		telemetry_record changed[] = { batch[0], batch[1] };
		changed[1].payload.interval.dimensions.zone_vnum++;
		const auto before = query_calls;
		CHECK(telemetry_repository_apply(changed, std::size(changed)).outcome ==
		      telemetry_batch_outcome::invalid_batch);
		CHECK(query_calls == before);
		const auto recovered = telemetry_repository_apply(batch, std::size(batch));
		CHECK(recovered.outcome == telemetry_batch_outcome::committed);
		CHECK(recovered.applied_count == 2U);
		CHECK(scalar("SELECT COUNT(*) FROM telemetry_interval") == 2U);
		CHECK(scalar("SELECT COUNT(*) FROM telemetry_config") == 1U);
		CHECK(scalar("SELECT COUNT(*) FROM telemetry_session") == 1U);
	}
}

static void initialization_stop_tests()
{
	case_name = "stop during blocked initialization must not publish healthy";
	shutdown_fixture();
	factory_unavailable = false;
	factory_entered.store(false);
	factory_release.store(false);
	factory_block.store(true);
	telemetry_repository_outcome outcome = telemetry_repository_outcome::unavailable;
	std::thread worker(
		[&]
		{
			CHECK(mysql_thread_init() == 0);
			outcome = telemetry_repository_init(repository_config());
			mysql_thread_end();
		});
	while (!factory_entered.load(std::memory_order_acquire))
		std::this_thread::yield();
	// Stop must return while the factory is still blocked. The runner timeout
	// bounds a regression that incorrectly waits for initialization to finish.
	CHECK(telemetry_repository_request_stop() == telemetry_repository_outcome::stopping);
	CHECK(!factory_release.load());
	CHECK(telemetry_repository_health_copy().state == telemetry_health_state::stopping);
	factory_release.store(true, std::memory_order_release);
	worker.join();
	factory_block.store(false);
	CHECK(outcome == telemetry_repository_outcome::stopping);
	CHECK(telemetry_repository_health_copy().state == telemetry_health_state::stopping);
	const auto before = factory_calls;
	CHECK(telemetry_repository_init(repository_config()) ==
	      telemetry_repository_outcome::stopping);
	CHECK(factory_calls == before);
	shutdown_fixture();
	CHECK(telemetry_repository_request_stop() == telemetry_repository_outcome::closed);
	CHECK(telemetry_repository_init(repository_config()) ==
	      telemetry_repository_outcome::stopping);
	CHECK(factory_calls == before);
	// Only owner shutdown after worker join establishes a new lifecycle.
	shutdown_fixture();
	CHECK(telemetry_repository_init(repository_config()) ==
	      telemetry_repository_outcome::ready);
	shutdown_fixture();
}

static void startup_fencing_tests()
{
	case_name = "another repository owner prevents a second committed ingest prefix";
	reset_fixture();
	shutdown_fixture();
	CHECK(scalar("SELECT GET_LOCK(CONCAT('duris.telemetry.',MD5(DATABASE())),2)") == 1U);
	CHECK(telemetry_repository_init(repository_config()) ==
	      telemetry_repository_outcome::unavailable);
	CHECK(telemetry_repository_health_copy().state == telemetry_health_state::degraded);
	CHECK(scalar("SELECT RELEASE_LOCK(CONCAT('duris.telemetry.',MD5(DATABASE())))") == 1U);
	CHECK(telemetry_repository_init(repository_config()) ==
	      telemetry_repository_outcome::ready);
	shutdown_fixture();
	case_name = "missing table prevents healthy startup and releases ownership lock";
	execute("RENAME TABLE telemetry_interval TO telemetry_interval_fixture_hidden");
	CHECK(telemetry_repository_init(repository_config()) ==
	      telemetry_repository_outcome::unavailable);
	CHECK(telemetry_repository_health_copy().state == telemetry_health_state::degraded);
	CHECK(scalar("SELECT GET_LOCK(CONCAT('duris.telemetry.',MD5(DATABASE())),2)") == 1U);
	CHECK(scalar("SELECT RELEASE_LOCK(CONCAT('duris.telemetry.',MD5(DATABASE())))") == 1U);
	execute("RENAME TABLE telemetry_interval_fixture_hidden TO telemetry_interval");
	CHECK(telemetry_repository_init(repository_config()) ==
	      telemetry_repository_outcome::ready);
}

static void bounds_and_lifecycle_tests()
{
	case_name = "process-wide zero-scope gap";
	reset_fixture();
	telemetry_record gap{};
	gap.header = { TELEMETRY_SCHEMA_VERSION,
		       telemetry_record_kind::coverage_gap,
		       0,
		       { { 10, 20 }, 1 },
		       -10 };
	gap.payload.gap.reason = telemetry_gap_reason::detail_queue_drop;
	gap.payload.gap.quality_flags = TELEMETRY_QUALITY_QUEUE_DROP;
	CHECK(telemetry_record_is_valid(gap));
	expect_one(gap, telemetry_apply_outcome::applied);
	CHECK(scalar("SELECT COUNT(*) FROM telemetry_session") == 0U);
	CHECK(scalar("SELECT COUNT(*) FROM telemetry_interval WHERE environment_id=0 AND season_id=0 AND session_seq=0") ==
	      1U);
	CHECK(scalar("SELECT COUNT(*) FROM telemetry_interval WHERE config_id IS NULL AND occurrence_utc_usec=-10 AND ingested_utc_usec > 0") ==
	      1U);
	case_name = "batch bounds and maximum batch";
	CHECK(telemetry_repository_apply(nullptr, 1).outcome ==
	      telemetry_batch_outcome::invalid_batch);
	CHECK(telemetry_repository_apply(&gap, TELEMETRY_BATCH_MAX_RECORDS_PROPOSAL + 1).outcome ==
	      telemetry_batch_outcome::invalid_batch);
	telemetry_record batch[TELEMETRY_BATCH_MAX_RECORDS_PROPOSAL];
	for (std::size_t i = 0; i < std::size(batch); ++i)
	{
		batch[i] = gap;
		batch[i].header.key.record_seq = i + 10;
	}
	auto result = telemetry_repository_apply(batch, std::size(batch));
	CHECK(result.outcome == telemetry_batch_outcome::committed &&
	      result.applied_count == std::size(batch));
	CHECK(scalar("SELECT COUNT(*) FROM telemetry_interval") == std::size(batch) + 1);
	shutdown_fixture();
	auto bounded = repository_config();
	bounded.max_batch_bytes = sizeof(telemetry_record);
	CHECK(telemetry_repository_init(bounded) == telemetry_repository_outcome::ready);
	CHECK(telemetry_repository_apply(batch, 2).outcome ==
	      telemetry_batch_outcome::invalid_batch);
	CHECK(telemetry_repository_init(bounded) == telemetry_repository_outcome::already_ready);
	case_name = "health is cached and safe while worker applies";
	reset_fixture();
	std::atomic<bool> finished{ false };
	std::atomic<unsigned int> snapshots{ 0 };
	std::thread reader(
		[&]
		{
			std::uint64_t previous = 0;
			while (!finished.load(std::memory_order_acquire))
			{
				auto health = telemetry_repository_health_copy();
				CHECK(health.applied_records >= previous);
				CHECK(health.schema_version == TELEMETRY_SCHEMA_VERSION);
				previous = health.applied_records;
				++snapshots;
			}
		});
	while (snapshots.load() == 0)
		std::this_thread::yield();
	for (int i = 0; i < 16; ++i)
	{
		gap.header.key.record_seq = i + 1;
		expect_one(gap, telemetry_apply_outcome::applied);
	}
	finished.store(true, std::memory_order_release);
	reader.join();
	CHECK(snapshots.load() > 0);
	const auto calls = query_calls;
	for (int i = 0; i < 100; ++i)
		(void)telemetry_repository_health_copy();
	CHECK(query_calls == calls);
	CHECK(telemetry_repository_health_copy().applied_records == 16U);
	telemetry_repository_request_stop();
	result = telemetry_repository_apply(&gap, 1);
	CHECK(result.outcome == telemetry_batch_outcome::unavailable ||
	      result.outcome == telemetry_batch_outcome::disabled);
	CHECK(query_calls == calls);
	shutdown_fixture();
	CHECK(telemetry_repository_health_copy().state == telemetry_health_state::stopped);
	case_name = "flatfile authority and explicit backend disable never open SQL";
	auto factory_before = factory_calls;
	mode = PERSISTENCE_MODE_FLATFILE_PRIMARY;
	CHECK(telemetry_repository_init(repository_config()) ==
	      telemetry_repository_outcome::flatfile_disabled);
	CHECK(factory_calls == factory_before);
	shutdown_fixture();
	mode = PERSISTENCE_MODE_MARIADB_PRIMARY;
	auto disabled = repository_config();
	disabled.backend = telemetry_storage_backend::flatfile_disabled;
	CHECK(telemetry_repository_init(disabled) ==
	      telemetry_repository_outcome::flatfile_disabled);
	CHECK(factory_calls == factory_before);
	shutdown_fixture();
	case_name = "connection outage has no pool fallback";
	factory_unavailable = true;
	CHECK(telemetry_repository_init(repository_config()) ==
	      telemetry_repository_outcome::unavailable);
	CHECK(factory_calls == factory_before + 1);
	CHECK(telemetry_repository_health_copy().state != telemetry_health_state::healthy);
}

int main(int argc, char **argv)
{
	CHECK(argc == 2);
	const char *ack = std::getenv("TELEMETRY_REPOSITORY_DISPOSABLE");
	CHECK(ack && std::strcmp(ack, "1") == 0);
	const char *port = std::getenv("TELEMETRY_REPOSITORY_PORT");
	CHECK(!port || std::strcmp(port, "3306") == 0 || std::strcmp(port, "3307") == 0);
	fixture_port = port && std::strcmp(port, "3307") == 0 ? 3307U : 3306U;
	observer = mysql_init(nullptr);
	CHECK(observer != nullptr);
	CHECK(mysql_real_connect(observer, "127.0.0.1", "root", "", nullptr, fixture_port, nullptr,
				 0) != nullptr);
	CHECK(__real_mysql_real_query(observer, "SELECT VERSION()", 16) == 0);
	MYSQL_RES *version = mysql_store_result(observer);
	CHECK(version != nullptr);
	MYSQL_ROW version_row = mysql_fetch_row(version);
	CHECK(version_row && version_row[0]);
	std::printf("Repository disposable fixture server: %s (port %u)\n", version_row[0],
		    fixture_port);
	mysql_free_result(version);
	execute("DROP DATABASE IF EXISTS duris_telemetry_test");
	execute("CREATE DATABASE duris_telemetry_test");
	CHECK(mysql_select_db(observer, "duris_telemetry_test") == 0);
	std::ifstream migration(argv[1]);
	CHECK(migration.good());
	std::string schema;
	for (std::string line; std::getline(migration, line);)
		if (line.rfind("--", 0) != 0)
			schema += line + "\n";
	std::size_t start = 0;
	for (std::size_t end = schema.find(';'); end != std::string::npos;
	     end = schema.find(';', start))
	{
		execute(schema.substr(start, end - start));
		start = end + 1;
	}
	initialization_stop_tests();
	allocation_failure_tests();
	golden_tests();
	replay_and_isolation_tests();
	config_and_scope_tests();
	global_scope_tests();
	checkpoint_tests();
	fault_tests();
	startup_fencing_tests();
	bounds_and_lifecycle_tests();
	shutdown_fixture();
	mysql_close(observer);
	std::puts(
		"SQL repository runtime: PASS (10 golden fixtures and focused failure/isolation regressions)");
}
#endif

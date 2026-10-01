// Standalone repository tests. SQL execution resets only an acknowledged,
// disposable loopback database. The private connection factory is a fixture
// seam; the real production factory has separate trust-boundary tests.
#include "telemetry/telemetry_repository.h"
#include "telemetry/telemetry_failure.h"
#include "persistence/persistence_mode.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <thread>
#include <utility>

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

static telemetry_repository_config repository_config(telemetry_producer_id fresh_producer = {})
{
	return { telemetry_storage_backend::sql,     0U,
		 TELEMETRY_SCHEMA_VERSION,	     TELEMETRY_BATCH_MAX_RECORDS_PROPOSAL,
		 TELEMETRY_BATCH_MAX_BYTES_PROPOSAL, fresh_producer };
}

#ifdef __NO_MYSQL__
int main()
{
	case_name = "no-mysql";
	CHECK(telemetry_classify_sql_failure(1213U, telemetry_sql_phase::statement) ==
	      telemetry_failure_class::transient_transaction);
	CHECK(telemetry_classify_sql_failure(1205U, telemetry_sql_phase::statement) ==
	      telemetry_failure_class::transient_transaction);
	CHECK(telemetry_classify_sql_failure(2013U, telemetry_sql_phase::statement) ==
	      telemetry_failure_class::transient_connection);
	CHECK(telemetry_classify_sql_failure(1054U, telemetry_sql_phase::statement) ==
	      telemetry_failure_class::permanent_schema);
	CHECK(telemetry_classify_sql_failure(1146U, telemetry_sql_phase::statement) ==
	      telemetry_failure_class::permanent_schema);
	CHECK(telemetry_classify_sql_failure(1142U, telemetry_sql_phase::statement) ==
	      telemetry_failure_class::permanent_permission);
	CHECK(telemetry_classify_sql_failure(1366U, telemetry_sql_phase::statement) ==
	      telemetry_failure_class::invalid_record);
	CHECK(telemetry_classify_sql_failure(1054U, telemetry_sql_phase::commit) ==
	      telemetry_failure_class::commit_ambiguous);
	telemetry_repository_shutdown();
	auto config = repository_config();
	config.schema_version = 0;
	CHECK(telemetry_repository_init(config) == telemetry_repository_outcome::invalid_config);
	config = repository_config();
	config.fresh_producer = { 1U, 0U };
	CHECK(!telemetry_repository_config_is_bounded(config));
	config.fresh_producer = { 1U, 2U };
	CHECK(telemetry_repository_config_is_bounded(config));
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
#include <string>
#include <vector>

static unsigned int fixture_port = 3306U;
static std::string fixture_host;
static std::string fixture_user;
static std::string fixture_password;
static std::string fixture_database;
static std::string fixture_engine;
static std::string fixture_server_version;
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
	unknown_column,
	permission,
	invalid_data,
	startup_permission,
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
	if (fault == fault_kind::startup_permission &&
	    sql == "SELECT * FROM telemetry_interval LIMIT 0")
	{
		fault = fault_kind::none;
		injected_handle = connection;
		injected_error = 1142U;
		return 1;
	}
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
	     fault == fault_kind::unknown_column || fault == fault_kind::permission ||
	     fault == fault_kind::invalid_data || fault == fault_kind::rollback_lost ||
	     fault == fault_kind::allocation))
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
		injected_error = selected == fault_kind::deadlock	? 1213U :
				 selected == fault_kind::unknown_column ? 1054U :
				 selected == fault_kind::permission	? 1142U :
				 selected == fault_kind::invalid_data	? 1366U :
									  1205U;
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
	unsigned int protocol = MYSQL_PROTOCOL_TCP;
	mysql_options(connection, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
	mysql_options(connection, MYSQL_OPT_PROTOCOL, &protocol);
	if (!mysql_real_connect(connection, fixture_host.c_str(), fixture_user.c_str(),
				fixture_password.c_str(), fixture_database.c_str(), fixture_port,
				nullptr, 0))
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
	for (const char *table : { "telemetry_interval", "telemetry_session", "telemetry_config",
				   "telemetry_player_day", "telemetry_cohort_day",
				   "telemetry_rollup_state", "telemetry_quarantine" })
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
static telemetry_record fixture_record(telemetry_record_kind kind, const telemetry_record *records,
				       std::size_t count)
{
	for (std::size_t index = 0; index < count; ++index)
		if (records[index].header.kind == kind)
			return records[index];
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
static telemetry_record progression_record(unsigned long long sequence, long long applied_xp)
{
	const auto interval = interval_record();
	telemetry_record record{};
	record.header = interval.header;
	record.header.kind = telemetry_record_kind::progression;
	record.header.key.record_seq = sequence;
	auto &progression = record.payload.progression;
	progression.session = interval.payload.interval.session;
	progression.connection = interval.payload.interval.connection;
	progression.at_monotonic_usec = 1200;
	progression.at_utc_usec = interval.header.occurrence_utc_usec + 1200;
	progression.kind = telemetry_progression_kind::experience_observed;
	progression.source = telemetry_progression_source::quest;
	progression.reason = telemetry_progression_reason::earned;
	progression.observation_status = telemetry_progression_observation_status::observed_mutable;
	progression.modifier_flags = TELEMETRY_PROGRESSION_MODIFIER_NONE;
	progression.requested_xp = applied_xp;
	progression.computed_xp = applied_xp;
	progression.applied_xp = applied_xp;
	progression.before_exp = 100;
	progression.after_exp = 100 + applied_xp;
	progression.before_level = 10;
	progression.after_level = 10;
	progression.reserved = 0;
	progression.threshold_xp = 0;
	progression.dimensions = interval.payload.interval.dimensions;
	progression.config_id = interval.payload.interval.config_id;
	progression.classifier_version = interval.payload.interval.classifier_version;
	progression.policy_version = interval.payload.interval.policy_version;
	progression.quality_flags = TELEMETRY_QUALITY_NONE;
	CHECK(telemetry_record_is_valid(record));
	return record;
}
static telemetry_encounter_source encounter_source()
{
	const auto interval = interval_record();
	return { interval.payload.interval.session.environment_id,
		 interval.payload.interval.session.season_id,
		 interval.payload.interval.config_id,
		 interval.payload.interval.classifier_version,
		 interval.payload.interval.policy_version,
		 interval.payload.interval.dimensions.zone_vnum,
		 7001U };
}
static telemetry_record encounter_record(unsigned long long sequence)
{
	const auto interval = interval_record();
	telemetry_record record{};
	record.header = interval.header;
	record.header.kind = telemetry_record_kind::encounter;
	record.header.key.record_seq = sequence;
	auto &encounter = record.payload.encounter;
	encounter.encounter = { { 701U, 702U }, 703U };
	encounter.kind = telemetry_encounter_event_kind::close;
	encounter.mode = telemetry_encounter_mode::pve;
	encounter.outcome = telemetry_encounter_outcome::death;
	encounter.revision = 2U;
	encounter.source = encounter_source();
	encounter.at_monotonic_usec = 5000U;
	encounter.at_utc_usec = 1004000;
	encounter.start_monotonic_usec = 1000U;
	encounter.start_utc_usec = 1000000;
	encounter.elapsed_usec = 4000U;
	encounter.participant_count = 2U;
	encounter.expected_credit_count = 1U;
	encounter.quality_flags = TELEMETRY_QUALITY_LATE;
	CHECK(telemetry_record_is_valid(record));
	return record;
}
static telemetry_record combat_summary_record(unsigned long long sequence)
{
	const auto interval = interval_record();
	telemetry_record record{};
	record.header = interval.header;
	record.header.kind = telemetry_record_kind::combat_summary;
	record.header.key.record_seq = sequence;
	auto &summary = record.payload.combat_summary;
	summary.encounter = { { 701U, 702U }, 703U };
	summary.source = encounter_source();
	summary.mode = telemetry_encounter_mode::pve;
	summary.outcome = telemetry_encounter_outcome::death;
	summary.actor_kind = telemetry_combat_actor_kind::player;
	summary.revision = 2U;
	summary.actor_id = interval.payload.interval.session.subject_id;
	summary.actor_pid = interval.payload.interval.session.pid;
	summary.owner_subject_id = summary.actor_id;
	summary.unique_player_count = 1U;
	summary.participant_count = 2U;
	summary.dropped_participant_count = 1U;
	summary.power_band = 10U;
	summary.opponent_power_band = 12U;
	summary.opponent_count = 3U;
	summary.modifier_flags = TELEMETRY_COMBAT_MODIFIER_SPELL |
				 TELEMETRY_COMBAT_MODIFIER_TANKING;
	summary.start_monotonic_usec = 1000U;
	summary.end_monotonic_usec = 5000U;
	summary.start_utc_usec = 1000000;
	summary.end_utc_usec = 1004000;
	summary.damage_dealt = 111U;
	summary.damage_taken = 112U;
	summary.healing_attempted = 50U;
	summary.effective_healing = 40U;
	summary.overhealing = 10U;
	summary.control_applications = 3U;
	summary.casting_attempts = 5U;
	summary.casting_completions = 3U;
	summary.casting_aborts = 1U;
	summary.casting_elapsed_usec = 600U;
	summary.tanking_usec = 700U;
	summary.quality_flags = TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;
	CHECK(telemetry_record_is_valid(record));
	return record;
}
static void seed_config()
{
	expect_one(normal_interval_configs[0], telemetry_apply_outcome::applied);
}

static telemetry_record record_kind_fixture(telemetry_record_kind kind)
{
	switch (kind)
	{
	case telemetry_record_kind::interval:
		return interval_record();
	case telemetry_record_kind::session_lifecycle:
		return fixture_record(kind, normal_interval_records,
				      std::size(normal_interval_records));
	case telemetry_record_kind::session_checkpoint:
		return checkpoint_record(1U, 100U, 7303U);
	case telemetry_record_kind::coverage_gap:
		return fixture_record(kind, drop_recovery_no_invented_context_records,
				      std::size(drop_recovery_no_invented_context_records));
	case telemetry_record_kind::configuration:
		return normal_interval_configs[0];
	case telemetry_record_kind::progression:
		return progression_record(7306U, 25);
	case telemetry_record_kind::encounter:
		return encounter_record(7307U);
	case telemetry_record_kind::combat_summary:
		return combat_summary_record(7308U);
	default:
		CHECK(false);
		return {};
	}
}

static void change_one_field(telemetry_record &record)
{
	switch (record.header.kind)
	{
	case telemetry_record_kind::interval:
		record.payload.interval.quality_flags |= TELEMETRY_QUALITY_LATE;
		break;
	case telemetry_record_kind::session_lifecycle:
		record.payload.lifecycle.quality_flags |= TELEMETRY_QUALITY_LATE;
		break;
	case telemetry_record_kind::session_checkpoint:
		record.payload.checkpoint.quality_flags |= TELEMETRY_QUALITY_LATE;
		break;
	case telemetry_record_kind::coverage_gap:
		record.payload.gap.quality_flags |= TELEMETRY_QUALITY_LATE;
		break;
	case telemetry_record_kind::configuration:
		++record.header.occurrence_utc_usec;
		break;
	case telemetry_record_kind::progression:
		record.payload.progression.quality_flags |= TELEMETRY_QUALITY_LATE;
		break;
	case telemetry_record_kind::encounter:
		record.payload.encounter.quality_flags |= TELEMETRY_QUALITY_SEQUENCE_GAP;
		break;
	case telemetry_record_kind::combat_summary:
		++record.payload.combat_summary.damage_dealt;
		break;
	default:
		CHECK(false);
	}
	CHECK(telemetry_record_is_valid(record));
}

static void every_record_kind_round_trip_tests()
{
	for (unsigned int number = 1U; number <= 8U; ++number)
	{
		const auto kind = static_cast<telemetry_record_kind>(number);
		std::string label = "record-kind:" + std::to_string(number);
		case_name = label.c_str();
		reset_fixture();
		if (kind != telemetry_record_kind::configuration)
			seed_config();
		const auto original = record_kind_fixture(kind);
		CHECK(telemetry_record_is_valid(original));
		expect_one(original, telemetry_apply_outcome::applied);
		expect_one(original, telemetry_apply_outcome::duplicate_identical);
		auto conflict = original;
		change_one_field(conflict);
		expect_one(conflict, telemetry_apply_outcome::duplicate_conflict);
		const std::string count =
			"SELECT COUNT(*) FROM telemetry_interval WHERE record_kind=" +
			std::to_string(number);
		CHECK(scalar(count.c_str()) == 1U);
	}
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

static void progression_replay_tests()
{
	case_name = "progression replay is idempotent and conflicts are visible";
	reset_fixture();
	seed_config();
	auto original = progression_record(20, 25);
	expect_one(original, telemetry_apply_outcome::applied);
	expect_one(original, telemetry_apply_outcome::duplicate_identical);
	auto conflict = original;
	conflict.payload.progression.applied_xp++;
	conflict.payload.progression.after_exp++;
	expect_one(conflict, telemetry_apply_outcome::duplicate_conflict);
	CHECK(scalar("SELECT COUNT(*) FROM telemetry_interval WHERE record_kind=6") == 1U);
}

static void typed_extension_mapping_tests()
{
	case_name = "typed extension fields use their migrated columns";
	reset_fixture();
	seed_config();
	const auto interval = interval_record();
	const auto progression = progression_record(20, 25);
	const auto encounter = encounter_record(21);
	const auto combat = combat_summary_record(22);
	const telemetry_record batch[] = { interval, progression, encounter, combat };
	const auto result = telemetry_repository_apply(batch, std::size(batch));
	CHECK(result.outcome == telemetry_batch_outcome::committed);
	CHECK(result.applied_count == std::size(batch));
	CHECK(result.duplicate_count == 0U && result.invalid_count == 0U &&
	      result.conflict_count == 0U);

	CHECK(scalar("SELECT COUNT(*) FROM telemetry_interval WHERE record_kind=6 "
		     "AND progression_kind=1 AND progression_source=5 AND progression_reason=1 "
		     "AND progression_observation_status=1 AND progression_modifier_flags=0 "
		     "AND progression_requested_xp=25 AND progression_computed_xp=25 "
		     "AND progression_applied_xp=25 AND progression_before_exp=100 "
		     "AND progression_after_exp=125 AND progression_before_level=10 "
		     "AND progression_after_level=10 AND progression_threshold_xp=0") == 1U);
	CHECK(scalar("SELECT COUNT(*) FROM telemetry_interval WHERE record_kind=7 "
		     "AND encounter_start_monotonic_usec=1000 "
		     "AND encounter_start_utc_usec=1000000 AND encounter_quality_flags=256 "
		     "AND start_monotonic_usec IS NULL AND start_utc_usec IS NULL "
		     "AND quality_flags IS NULL") == 1U);
	CHECK(scalar("SELECT COUNT(*) FROM telemetry_interval WHERE record_kind=8 "
		     "AND combat_start_monotonic_usec=1000 AND combat_end_monotonic_usec=5000 "
		     "AND combat_start_utc_usec=1000000 AND combat_end_utc_usec=1004000 "
		     "AND combat_damage_dealt=111 AND combat_damage_taken=112 "
		     "AND combat_healing_attempted=50 AND combat_effective_healing=40 "
		     "AND combat_overhealing=10 AND combat_control_applications=3 "
		     "AND combat_casting_attempts=5 AND combat_casting_completions=3 "
		     "AND combat_casting_aborts=1 AND combat_casting_elapsed_usec=600 "
		     "AND combat_tanking_usec=700 AND combat_quality_flags=128 "
		     "AND start_monotonic_usec IS NULL AND end_monotonic_usec IS NULL "
		     "AND start_utc_usec IS NULL AND end_utc_usec IS NULL "
		     "AND quality_flags IS NULL") == 1U);

	expect_one(progression, telemetry_apply_outcome::duplicate_identical);
	expect_one(encounter, telemetry_apply_outcome::duplicate_identical);
	expect_one(combat, telemetry_apply_outcome::duplicate_identical);
	auto progression_conflict = progression;
	progression_conflict.payload.progression.applied_xp++;
	progression_conflict.payload.progression.after_exp++;
	expect_one(progression_conflict, telemetry_apply_outcome::duplicate_conflict);
	auto encounter_conflict = encounter;
	encounter_conflict.payload.encounter.quality_flags |= TELEMETRY_QUALITY_SEQUENCE_GAP;
	expect_one(encounter_conflict, telemetry_apply_outcome::duplicate_conflict);
	auto combat_conflict = combat;
	combat_conflict.payload.combat_summary.damage_dealt++;
	expect_one(combat_conflict, telemetry_apply_outcome::duplicate_conflict);
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
	case_name = "same configuration content after process restart";
	shutdown_fixture();
	CHECK(telemetry_repository_init(repository_config()) ==
	      telemetry_repository_outcome::ready);
	config = normal_interval_configs[0];
	config.header.key.producer.boot_id++;
	config.header.key.producer.process_id++;
	config.header.occurrence_utc_usec++;
	config.payload.configuration.config.effective_utc_usec++;
	config.payload.configuration.config.revision++;
	expect_one(config, telemetry_apply_outcome::applied);
	expect_one(config, telemetry_apply_outcome::duplicate_identical);
	CHECK(scalar("SELECT COUNT(*) FROM telemetry_config") == 1U);
	CHECK(scalar("SELECT COUNT(*) FROM telemetry_interval") == 2U);
	CHECK(scalar("SELECT COUNT(DISTINCT config_revision) FROM telemetry_interval") == 2U);
	CHECK(scalar("SELECT COUNT(DISTINCT effective_utc_usec) FROM telemetry_interval") == 2U);
	CHECK(scalar("SELECT revision FROM telemetry_config") ==
	      normal_interval_configs[0].payload.configuration.config.revision);
	CHECK(scalar("SELECT effective_utc_usec FROM telemetry_config") ==
	      static_cast<unsigned long long>(
		      normal_interval_configs[0].payload.configuration.config.effective_utc_usec));

	case_name = "publication metadata remains immutable under the same record key";
	auto changed_publication = config;
	changed_publication.payload.configuration.config.revision++;
	expect_one(changed_publication, telemetry_apply_outcome::duplicate_conflict);
	changed_publication = config;
	changed_publication.payload.configuration.config.effective_utc_usec++;
	expect_one(changed_publication, telemetry_apply_outcome::duplicate_conflict);
	changed_publication = config;
	changed_publication.header.occurrence_utc_usec++;
	expect_one(changed_publication, telemetry_apply_outcome::duplicate_conflict);

	case_name = "different semantic content under shared config identity";
	auto conflicting_content = changed_season_id_config;
	conflicting_content.payload.configuration.config.config_id =
		config.payload.configuration.config.config_id;
	expect_one(conflicting_content, telemetry_apply_outcome::duplicate_conflict);
	CHECK(scalar("SELECT COUNT(*) FROM telemetry_config") == 1U);
	CHECK(scalar("SELECT COUNT(*) FROM telemetry_interval") == 2U);
	// Both producers' exact original facts remain replayable.
	expect_one(normal_interval_configs[0], telemetry_apply_outcome::duplicate_identical);
	expect_one(config, telemetry_apply_outcome::duplicate_identical);
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

static void failure_taxonomy_tests()
{
	for (const auto &test :
	     { std::pair{ fault_kind::unknown_column, telemetry_failure_class::permanent_schema },
	       std::pair{ fault_kind::permission, telemetry_failure_class::permanent_permission } })
	{
		case_name = "permanent statement failures retain identity and open health circuit";
		reset_fixture();
		seed_config();
		const auto record = interval_record();
		fault = test.first;
		const auto failed = telemetry_repository_apply(&record, 1U);
		CHECK(failed.outcome == telemetry_batch_outcome::permanent_failure);
		CHECK(failed.failure_class == test.second);
		CHECK(failed.result_count == 1U);
		CHECK(failed.results[0].outcome == telemetry_apply_outcome::permanent_failure);
		CHECK(failed.results[0].failure_class == test.second);
		CHECK(failed.first_record_seq == record.header.key.record_seq);
		CHECK(failed.last_record_seq == record.header.key.record_seq);
		const auto health = telemetry_repository_health_copy();
		CHECK(health.state == telemetry_health_state::circuit_open);
		CHECK(health.last_failure_class == test.second);
		CHECK(health.last_failure_producer.boot_id == record.header.key.producer.boot_id);
		CHECK(health.last_failure_first_record_seq == record.header.key.record_seq);
		CHECK(scalar("SELECT COUNT(*) FROM telemetry_interval") == 1U);
	}

	case_name = "record data violation is durably quarantined without blocking later records";
	reset_fixture();
	seed_config();
	auto invalid = interval_record();
	fault = fault_kind::invalid_data;
	const auto quarantined = telemetry_repository_apply(&invalid, 1U);
	CHECK(quarantined.outcome == telemetry_batch_outcome::committed_with_rejections);
	CHECK(quarantined.failure_class == telemetry_failure_class::invalid_record);
	CHECK(quarantined.invalid_count == 1U);
	CHECK(quarantined.quarantined_count == 1U);
	CHECK(quarantined.results[0].outcome == telemetry_apply_outcome::quarantined_invalid);
	CHECK(quarantined.results[0].failure_class == telemetry_failure_class::invalid_record);
	CHECK(scalar("SELECT COUNT(*) FROM telemetry_quarantine") == 1U);
	CHECK(scalar("SELECT gap_reason FROM telemetry_quarantine") ==
	      static_cast<unsigned int>(telemetry_gap_reason::record_quarantined));
	CHECK(scalar("SELECT quality_flags FROM telemetry_quarantine") ==
	      TELEMETRY_QUALITY_SEQUENCE_GAP);
	CHECK(scalar("SELECT OCTET_LENGTH(payload_sha256) FROM telemetry_quarantine") == 32U);
	CHECK(scalar("SELECT OCTET_LENGTH(record_payload) FROM telemetry_quarantine") ==
	      sizeof(telemetry_record));
	auto later = invalid;
	++later.header.key.record_seq;
	CHECK(telemetry_repository_apply(&later, 1U).outcome == telemetry_batch_outcome::committed);
	CHECK(scalar("SELECT COUNT(*) FROM telemetry_interval") == 2U);
	CHECK(telemetry_repository_health_copy().quarantined_records == 1U);
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
	      telemetry_repository_outcome::permanent_failure);
	CHECK(telemetry_repository_health_copy().state == telemetry_health_state::circuit_open);
	CHECK(telemetry_repository_health_copy().last_failure_class ==
	      telemetry_failure_class::permanent_schema);
	CHECK(scalar("SELECT GET_LOCK(CONCAT('duris.telemetry.',MD5(DATABASE())),2)") == 1U);
	CHECK(scalar("SELECT RELEASE_LOCK(CONCAT('duris.telemetry.',MD5(DATABASE())))") == 1U);
	execute("RENAME TABLE telemetry_interval_fixture_hidden TO telemetry_interval");
	CHECK(telemetry_repository_init(repository_config()) ==
	      telemetry_repository_outcome::ready);
	shutdown_fixture();
	case_name = "startup permission failure is permanent and releases ownership lock";
	fault = fault_kind::startup_permission;
	CHECK(telemetry_repository_init(repository_config()) ==
	      telemetry_repository_outcome::permanent_failure);
	CHECK(telemetry_repository_health_copy().last_failure_class ==
	      telemetry_failure_class::permanent_permission);
	CHECK(telemetry_repository_health_copy().last_error_code == 1142U);
	CHECK(scalar("SELECT GET_LOCK(CONCAT('duris.telemetry.',MD5(DATABASE())),2)") == 1U);
	CHECK(scalar("SELECT RELEASE_LOCK(CONCAT('duris.telemetry.',MD5(DATABASE())))") == 1U);
	CHECK(telemetry_repository_init(repository_config()) ==
	      telemetry_repository_outcome::ready);
}

static void fresh_producer_tests()
{
	case_name = "existing producer is refused for a new repository lifetime";
	reset_fixture();
	seed_config();
	const auto existing = interval_record();
	const auto reused = existing.header.key.producer;
	expect_one(existing, telemetry_apply_outcome::applied);
	shutdown_fixture();
	CHECK(telemetry_repository_init(repository_config(reused)) ==
	      telemetry_repository_outcome::permanent_failure);
	CHECK(telemetry_repository_health_copy().state == telemetry_health_state::circuit_open);
	CHECK(telemetry_repository_health_copy().last_failure_class ==
	      telemetry_failure_class::permanent_repository);
	CHECK(scalar("SELECT COUNT(*) FROM telemetry_interval") == 2U);

	// A joined owner shutdown resets freshness state, so a different producer
	// can claim the next repository lifetime while old facts remain durable.
	shutdown_fixture();
	auto fresh = reused;
	++fresh.boot_id;
	CHECK(telemetry_repository_init(repository_config(fresh)) ==
	      telemetry_repository_outcome::ready);
	case_name = "fresh producer is accepted and mismatched records are rejected";
	auto fresh_config = normal_interval_configs[0];
	fresh_config.header.key.producer = fresh;
	expect_one(fresh_config, telemetry_apply_outcome::applied);
	auto fresh_interval = existing;
	fresh_interval.header.key.producer = fresh;
	expect_one(fresh_interval, telemetry_apply_outcome::applied);
	auto mismatched = existing;
	mismatched.header.key.record_seq += 1000;
	expect_one(mismatched, telemetry_apply_outcome::rejected_invalid);

	case_name = "same lifetime reconnect retains ambiguous replay";
	auto replay = fresh_interval;
	replay.header.key.record_seq += 1000;
	fault = fault_kind::commit_lost_committed;
	const auto ambiguous = telemetry_repository_apply(&replay, 1U);
	CHECK(ambiguous.outcome == telemetry_batch_outcome::commit_ambiguous);
	CHECK(scalar("SELECT COUNT(*) FROM telemetry_interval") == 5U);
	const auto reconciled = telemetry_repository_apply(&replay, 1U);
	CHECK(reconciled.outcome == telemetry_batch_outcome::committed);
	CHECK(reconciled.applied_count == 0U && reconciled.duplicate_count == 1U);
	CHECK(scalar("SELECT COUNT(*) FROM telemetry_interval") == 5U);
	shutdown_fixture();
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

int main()
{
	const char *ack = std::getenv("TELEMETRY_REPOSITORY_DISPOSABLE");
	CHECK(ack && std::strcmp(ack, "1") == 0);
	auto required = [](const char *name)
	{
		const char *value = std::getenv(name);
		CHECK(value && *value);
		return value;
	};
	fixture_host = required("TELEMETRY_REPOSITORY_HOST");
	fixture_user = required("TELEMETRY_REPOSITORY_USER");
	fixture_password = required("TELEMETRY_REPOSITORY_PASSWORD");
	fixture_database = required("TELEMETRY_REPOSITORY_DATABASE");
	fixture_engine = required("TELEMETRY_REPOSITORY_DB_IMAGE");
	CHECK(fixture_host == "127.0.0.1");
	CHECK(fixture_database.starts_with("duris_telemetry_test_") &&
	      fixture_database.size() >= 33U && fixture_database.size() <= 101U);
	for (char character : fixture_database)
		CHECK((character >= 'a' && character <= 'z') ||
		      (character >= '0' && character <= '9') || character == '_');
	char *port_end = nullptr;
	const unsigned long parsed_port =
		std::strtoul(required("TELEMETRY_REPOSITORY_PORT"), &port_end, 10);
	CHECK(port_end && *port_end == '\0' && parsed_port > 0U && parsed_port <= 65535U);
	fixture_port = static_cast<unsigned int>(parsed_port);
	char *migration_count_end = nullptr;
	const unsigned long expected_migration_count = std::strtoul(
		required("TELEMETRY_REPOSITORY_MIGRATION_COUNT"), &migration_count_end, 10);
	CHECK(migration_count_end && *migration_count_end == '\0' && expected_migration_count > 0U);
	observer = mysql_init(nullptr);
	CHECK(observer != nullptr);
	unsigned int protocol = MYSQL_PROTOCOL_TCP;
	CHECK(mysql_options(observer, MYSQL_OPT_PROTOCOL, &protocol) == 0);
	CHECK(mysql_real_connect(observer, fixture_host.c_str(), fixture_user.c_str(),
				 fixture_password.c_str(), fixture_database.c_str(), fixture_port,
				 nullptr, 0) != nullptr);
	CHECK(__real_mysql_real_query(observer, "SELECT VERSION()", 16) == 0);
	MYSQL_RES *version = mysql_store_result(observer);
	CHECK(version != nullptr);
	MYSQL_ROW version_row = mysql_fetch_row(version);
	CHECK(version_row && version_row[0]);
	fixture_server_version = version_row[0];
	std::printf("Repository disposable fixture engine=%s server=%s database=%s (port %u)\n",
		    fixture_engine.c_str(), fixture_server_version.c_str(),
		    fixture_database.c_str(), fixture_port);
	mysql_free_result(version);
	CHECK(scalar("SELECT COUNT(*) FROM mud_schema_history") == expected_migration_count);
	initialization_stop_tests();
	allocation_failure_tests();
	golden_tests();
	every_record_kind_round_trip_tests();
	replay_and_isolation_tests();
	progression_replay_tests();
	typed_extension_mapping_tests();
	config_and_scope_tests();
	global_scope_tests();
	checkpoint_tests();
	fault_tests();
	failure_taxonomy_tests();
	startup_fencing_tests();
	fresh_producer_tests();
	bounds_and_lifecycle_tests();
	shutdown_fixture();
	mysql_close(observer);
	std::puts("SQL repository runtime: PASS (record kinds 1-8, 10 golden fixtures, and focused "
		  "failure/isolation regressions)");
}
#endif

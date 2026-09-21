#include "telemetry/telemetry_repository.h"
#include "persistence/persistence_mode.h"
#include "sql/sql_telemetry_connection.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cerrno>
#include <new>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef __NO_MYSQL__
#include <openssl/sha.h>
#include "sql/sql_thread_init.h"
#endif

namespace
{
using field = std::pair<std::string, std::string>;
using fields = std::vector<field>;
std::mutex health_mutex;
telemetry_health_snapshot health{};
std::atomic<bool> stop_requested{ false };
telemetry_repository_config settings{};
bool initialized = false;
bool restart_allowed = false; // guarded by health_mutex
#ifndef __NO_MYSQL__
bool freshness_verified = false; // survives connection reconnects; reset after join
#endif

void saturating_add(std::uint64_t &value, std::uint64_t amount)
{
	value += std::min(amount, std::numeric_limits<std::uint64_t>::max() - value);
}

telemetry_monotonic_usec now_usec()
{
	return std::chrono::duration_cast<std::chrono::microseconds>(
		       std::chrono::steady_clock::now().time_since_epoch())
		.count();
}

#ifndef __NO_MYSQL__
struct mysql_thread_guard
{
	bool ready = false;
	mysql_thread_guard() noexcept
	{
		try
		{
			ready = sql_worker_thread_init() == 0;
		}
		catch (...)
		{
			ready = false;
		}
	}
	~mysql_thread_guard()
	{
		if (ready)
			mysql_thread_end();
	}
};
MYSQL *connection = nullptr;
std::array<telemetry_record, TELEMETRY_BATCH_MAX_RECORDS_PROPOSAL> pending{};
std::size_t pending_count = 0;

struct sql_failure
{
	unsigned int code;
};
using result_ptr = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;

result_ptr query(const std::string &statement)
{
	if (mysql_real_query(connection, statement.data(), statement.size()) != 0)
		throw sql_failure{ mysql_errno(connection) };
	MYSQL_RES *result = mysql_store_result(connection);
	if (!result && mysql_field_count(connection) != 0)
		throw sql_failure{ mysql_errno(connection) };
	return result_ptr(result, mysql_free_result);
}

void disconnect()
{
	if (connection)
		mysql_close(connection);
	connection = nullptr;
}

bool open_connection()
{
	try
	{
		connection = sql_open_telemetry_connection();
		if (!connection)
			return false;
		// A second process must not publish a later ingest ID while this
		// connection has an unresolved commit. The lock dies with the handle.
		auto lock = query("SELECT GET_LOCK(CONCAT('duris.telemetry.',MD5(DATABASE())),0)");
		auto row = mysql_fetch_row(lock.get());
		if (!row || !row[0] || std::string(row[0]) != "1")
		{
			disconnect();
			return false;
		}
		for (const char *table :
		     { "telemetry_interval", "telemetry_session", "telemetry_config" })
			query("SELECT * FROM " + std::string(table) + " LIMIT 0");
		return true;
	}
	catch (...)
	{
		disconnect();
		return false;
	}
}

enum class fresh_producer_check : std::uint8_t
{
	clear,
	collision,
	unavailable,
};

fresh_producer_check check_fresh_producer()
{
	if (telemetry_producer_id_is_zero(settings.fresh_producer))
		return fresh_producer_check::clear;
	try
	{
		auto existing =
			query("SELECT 1 FROM telemetry_interval WHERE boot_id=" +
			      std::to_string(settings.fresh_producer.boot_id) + " AND process_id=" +
			      std::to_string(settings.fresh_producer.process_id) + " LIMIT 1");
		return mysql_fetch_row(existing.get()) ? fresh_producer_check::collision :
							 fresh_producer_check::clear;
	}
	catch (...)
	{
		return fresh_producer_check::unavailable;
	}
}

template <typename T> void number(fields &values, const char *name, T value)
{
	if constexpr (std::is_enum_v<T>)
		number(values, name, static_cast<std::underlying_type_t<T>>(value));
	else
		values.emplace_back(name, std::to_string(value));
}

std::string hex(const unsigned char *bytes, std::size_t count)
{
	static constexpr char digits[] = "0123456789ABCDEF";
	std::string value;
	value.reserve(count * 2);
	for (std::size_t i = 0; i < count; ++i)
	{
		value += digits[bytes[i] >> 4U];
		value += digits[bytes[i] & 15U];
	}
	return value;
}

#define FIELD(values, object, name) number(values, #name, (object).name)

void session_fields(fields &values, const telemetry_session_ref &session)
{
	FIELD(values, session, environment_id);
	FIELD(values, session, season_id);
	number(values, "session_boot_id", session.id.producer.boot_id);
	number(values, "session_process_id", session.id.producer.process_id);
	number(values, "session_seq", session.id.session_seq);
	FIELD(values, session, subject_id);
	FIELD(values, session, pid);
}

void connection_fields(fields &values, const telemetry_connection_id &id)
{
	number(values, "connection_boot_id", id.producer.boot_id);
	number(values, "connection_process_id", id.producer.process_id);
	FIELD(values, id, connection_seq);
}

void dimension_fields(fields &values, const telemetry_dimensions &dimensions)
{
	FIELD(values, dimensions, level_band);
	FIELD(values, dimensions, class_id);
	FIELD(values, dimensions, race_id);
	FIELD(values, dimensions, faction_id);
	FIELD(values, dimensions, zone_vnum);
	FIELD(values, dimensions, group_size);
}

void encounter_fields(fields &values, const telemetry_encounter_payload &encounter)
{
	number(values, "encounter_boot_id", encounter.encounter.producer.boot_id);
	number(values, "encounter_process_id", encounter.encounter.producer.process_id);
	number(values, "encounter_seq", encounter.encounter.sequence);
	number(values, "encounter_event", encounter.kind);
	number(values, "encounter_mode", encounter.mode);
	number(values, "encounter_outcome", encounter.outcome);
	number(values, "encounter_revision", encounter.revision);
	number(values, "encounter_environment_id", encounter.source.environment_id);
	number(values, "encounter_season_id", encounter.source.season_id);
	number(values, "encounter_config_id", encounter.source.config_id);
	number(values, "encounter_classifier_version", encounter.source.classifier_version);
	number(values, "encounter_policy_version", encounter.source.policy_version);
	number(values, "encounter_zone_vnum", encounter.source.zone_vnum);
	number(values, "encounter_group_key", encounter.source.group_key);
	number(values, "encounter_participant_subject_id", encounter.participant.subject_id);
	number(values, "encounter_participant_pid", encounter.participant.pid);
	FIELD(values, encounter, at_monotonic_usec);
	FIELD(values, encounter, at_utc_usec);
	number(values, "encounter_start_monotonic_usec", encounter.start_monotonic_usec);
	number(values, "encounter_start_utc_usec", encounter.start_utc_usec);
	FIELD(values, encounter, elapsed_usec);
	FIELD(values, encounter, participant_usec);
	FIELD(values, encounter, participant_count);
	FIELD(values, encounter, expected_credit_count);
	number(values, "encounter_quality_flags", encounter.quality_flags);
}

void combat_summary_fields(fields &values, const telemetry_combat_summary_payload &summary)
{
	number(values, "combat_encounter_boot_id", summary.encounter.producer.boot_id);
	number(values, "combat_encounter_process_id", summary.encounter.producer.process_id);
	number(values, "combat_encounter_seq", summary.encounter.sequence);
	number(values, "combat_mode", summary.mode);
	number(values, "combat_outcome", summary.outcome);
	number(values, "combat_revision", summary.revision);
	number(values, "combat_environment_id", summary.source.environment_id);
	number(values, "combat_season_id", summary.source.season_id);
	number(values, "combat_config_id", summary.source.config_id);
	number(values, "combat_classifier_version", summary.source.classifier_version);
	number(values, "combat_policy_version", summary.source.policy_version);
	number(values, "combat_zone_vnum", summary.source.zone_vnum);
	number(values, "combat_group_key", summary.source.group_key);
	number(values, "combat_actor_id", summary.actor_id);
	number(values, "combat_actor_pid", summary.actor_pid);
	number(values, "combat_owner_subject_id", summary.owner_subject_id);
	number(values, "combat_actor_kind", summary.actor_kind);
	number(values, "combat_unique_player_count", summary.unique_player_count);
	number(values, "combat_participant_count", summary.participant_count);
	number(values, "combat_dropped_participant_count", summary.dropped_participant_count);
	number(values, "combat_power_band", summary.power_band);
	number(values, "combat_opponent_power_band", summary.opponent_power_band);
	number(values, "combat_opponent_count", summary.opponent_count);
	number(values, "combat_modifier_flags", summary.modifier_flags);
	number(values, "combat_start_monotonic_usec", summary.start_monotonic_usec);
	number(values, "combat_end_monotonic_usec", summary.end_monotonic_usec);
	number(values, "combat_start_utc_usec", summary.start_utc_usec);
	number(values, "combat_end_utc_usec", summary.end_utc_usec);
	number(values, "combat_damage_dealt", summary.damage_dealt);
	number(values, "combat_damage_taken", summary.damage_taken);
	number(values, "combat_healing_attempted", summary.healing_attempted);
	number(values, "combat_effective_healing", summary.effective_healing);
	number(values, "combat_overhealing", summary.overhealing);
	number(values, "combat_control_applications", summary.control_applications);
	number(values, "combat_casting_attempts", summary.casting_attempts);
	number(values, "combat_casting_completions", summary.casting_completions);
	number(values, "combat_casting_aborts", summary.casting_aborts);
	number(values, "combat_casting_elapsed_usec", summary.casting_elapsed_usec);
	number(values, "combat_tanking_usec", summary.tanking_usec);
	number(values, "combat_quality_flags", summary.quality_flags);
}

fields counter_fields(const telemetry_cumulative_counters &counters)
{
	fields values;
	FIELD(values, counters, connected_usec);
	FIELD(values, counters, active_usec);
	FIELD(values, counters, idle_usec);
	FIELD(values, counters, unknown_usec);
	FIELD(values, counters, resident_usec);
	FIELD(values, counters, linkdead_usec);
	return values;
}

fields config_fields(const telemetry_config_snapshot &config, bool fact)
{
	fields values;
	FIELD(values, config, environment_id);
	FIELD(values, config, config_id);
	if (!fact)
		FIELD(values, config, schema_version);
	number(values, fact ? "config_revision" : "revision", config.revision);
	FIELD(values, config, build_version);
	FIELD(values, config, content_version);
	FIELD(values, config, property_version);
	FIELD(values, config, classifier_version);
	FIELD(values, config, policy_version);
	FIELD(values, config, season_id);
	values.emplace_back("fingerprint", hex(config.fingerprint, sizeof(config.fingerprint)));
	FIELD(values, config, effective_utc_usec);
	FIELD(values, config, interval_usec);
	FIELD(values, config, checkpoint_interval_usec);
	FIELD(values, config, active_window_usec);
	FIELD(values, config, context_segments_per_minute);
	FIELD(values, config, pulse_slot_count);
	FIELD(values, config, backend);
	FIELD(values, config, enabled);
	return values;
}

bool fingerprint_valid(const telemetry_config_snapshot &config)
{
	std::string canonical;
	auto append = [&canonical](std::uint64_t value, unsigned int bytes)
	{
		for (unsigned int i = bytes; i != 0; --i)
			canonical += static_cast<char>((value >> ((i - 1U) * 8U)) & 255U);
	};
	append(config.schema_version, 2);
	append(config.build_version, 4);
	append(config.content_version, 4);
	append(config.property_version, 4);
	append(config.classifier_version, 4);
	append(config.policy_version, 4);
	append(config.season_id, 8);
	append(config.environment_id, 8);
	append(config.interval_usec, 8);
	append(config.checkpoint_interval_usec, 8);
	append(config.active_window_usec, 8);
	append(config.context_segments_per_minute, 4);
	append(config.pulse_slot_count, 2);
	append(static_cast<std::uint8_t>(config.backend), 1);
	append(config.enabled, 1);
	unsigned char digest[SHA256_DIGEST_LENGTH];
	SHA256(reinterpret_cast<const unsigned char *>(canonical.data()), canonical.size(), digest);
	return std::equal(std::begin(digest), std::end(digest), config.fingerprint);
}

fields record_fields(const telemetry_record &record)
{
	fields values;
	const auto &header = record.header;
	FIELD(values, header.key.producer, boot_id);
	FIELD(values, header.key.producer, process_id);
	FIELD(values, header.key, record_seq);
	FIELD(values, header, schema_version);
	number(values, "record_kind", header.kind);
	FIELD(values, header, occurrence_utc_usec);
	switch (header.kind)
	{
	case telemetry_record_kind::interval:
	{
		const auto &p = record.payload.interval;
		session_fields(values, p.session);
		connection_fields(values, p.connection);
		FIELD(values, p.window, start_monotonic_usec);
		FIELD(values, p.window, end_monotonic_usec);
		FIELD(values, p.window, start_utc_usec);
		FIELD(values, p.window, end_utc_usec);
		FIELD(values, p, duration_usec);
		FIELD(values, p, category);
		FIELD(values, p, context);
		FIELD(values, p, context_quality);
		dimension_fields(values, p.dimensions);
		FIELD(values, p, config_id);
		FIELD(values, p, classifier_version);
		FIELD(values, p, policy_version);
		FIELD(values, p, quality_flags);
		break;
	}
	case telemetry_record_kind::session_lifecycle:
	{
		const auto &p = record.payload.lifecycle;
		session_fields(values, p.session);
		connection_fields(values, p.connection);
		FIELD(values, p, lifecycle);
		FIELD(values, p, end_reason);
		FIELD(values, p, at_monotonic_usec);
		FIELD(values, p, at_utc_usec);
		dimension_fields(values, p.dimensions);
		FIELD(values, p, config_id);
		FIELD(values, p, classifier_version);
		FIELD(values, p, policy_version);
		FIELD(values, p, quality_flags);
		break;
	}
	case telemetry_record_kind::session_checkpoint:
	{
		const auto &p = record.payload.checkpoint;
		session_fields(values, p.session);
		connection_fields(values, p.connection);
		number(values, "checkpoint_revision", p.revision);
		FIELD(values, p, at_monotonic_usec);
		FIELD(values, p, at_utc_usec);
		const auto counters = counter_fields(p.cumulative);
		values.insert(values.end(), counters.begin(), counters.end());
		FIELD(values, p, config_id);
		FIELD(values, p, quality_flags);
		break;
	}
	case telemetry_record_kind::progression:
	{
		const auto &p = record.payload.progression;
		session_fields(values, p.session);
		connection_fields(values, p.connection);
		FIELD(values, p, at_monotonic_usec);
		FIELD(values, p, at_utc_usec);
		number(values, "progression_kind", p.kind);
		number(values, "progression_source", p.source);
		number(values, "progression_reason", p.reason);
		number(values, "progression_observation_status", p.observation_status);
		number(values, "progression_modifier_flags", p.modifier_flags);
		number(values, "progression_requested_xp", p.requested_xp);
		number(values, "progression_computed_xp", p.computed_xp);
		number(values, "progression_applied_xp", p.applied_xp);
		number(values, "progression_before_exp", p.before_exp);
		number(values, "progression_after_exp", p.after_exp);
		number(values, "progression_before_level", p.before_level);
		number(values, "progression_after_level", p.after_level);
		number(values, "progression_threshold_xp", p.threshold_xp);
		dimension_fields(values, p.dimensions);
		FIELD(values, p, config_id);
		FIELD(values, p, classifier_version);
		FIELD(values, p, policy_version);
		FIELD(values, p, quality_flags);
		break;
	}
	case telemetry_record_kind::encounter:
		encounter_fields(values, record.payload.encounter);
		break;
	case telemetry_record_kind::combat_summary:
		combat_summary_fields(values, record.payload.combat_summary);
		break;
	case telemetry_record_kind::coverage_gap:
	{
		const auto &p = record.payload.gap;
		session_fields(values, p.session);
		connection_fields(values, p.connection);
		number(values, "gap_reason", p.reason);
		FIELD(values, p, start_monotonic_usec);
		FIELD(values, p, end_monotonic_usec);
		FIELD(values, p, start_utc_usec);
		FIELD(values, p, end_utc_usec);
		FIELD(values, p, first_missing_record_seq);
		FIELD(values, p, last_missing_record_seq);
		FIELD(values, p, duration_usec);
		FIELD(values, p, dropped_records);
		FIELD(values, p, quality_flags);
		break;
	}
	case telemetry_record_kind::configuration:
	{
		const auto config = config_fields(record.payload.configuration.config, true);
		values.insert(values.end(), config.begin(), config.end());
		break;
	}
	default:
		break;
	}
	return values;
}
#undef FIELD

/* Only compile-time field names and decimal integers or fixed digest hex are
 * assembled as SQL. No string supplied by a player or configuration is SQL text. */
std::string names(const fields &values, bool select = false)
{
	std::string result;
	for (const auto &item : values)
	{
		if (!result.empty())
			result += ',';
		result += select && item.first == "fingerprint" ? "HEX(`fingerprint`)" :
								  "`" + item.first + "`";
	}
	return result;
}

std::string literal(const field &item)
{
	return item.first == "fingerprint" ? "UNHEX('" + item.second + "')" : item.second;
}

std::string where(const fields &values)
{
	std::string result;
	for (const auto &item : values)
	{
		if (!result.empty())
			result += " AND ";
		result += "`" + item.first + "`=" + literal(item);
	}
	return result;
}

void insert(const char *table, const fields &values)
{
	std::string sql = "INSERT INTO " + std::string(table) + " (" + names(values) + ") VALUES (";
	bool first = true;
	for (const auto &item : values)
	{
		if (!first)
			sql += ',';
		first = false;
		sql += literal(item);
	}
	query(sql + ')');
}

void update(const char *table, const fields &values, const std::string &predicate)
{
	std::string sql = "UPDATE " + std::string(table) + " SET ";
	bool first = true;
	for (const auto &item : values)
	{
		if (!first)
			sql += ',';
		first = false;
		sql += "`" + item.first + "`=" + literal(item);
	}
	query(sql + " WHERE " + predicate);
}

bool equal_row(MYSQL_ROW row, const fields &values)
{
	for (std::size_t i = 0; i < values.size(); ++i)
		if (!row[i] || values[i].second != row[i])
			return false;
	return true;
}

std::string signature(const telemetry_record &record)
{
	std::string value = where(record_fields(record));
	value += ";reserved=" + std::to_string(record.header.reserved);
	// Invalid representation remains stable on an unresolved retry, too.
	switch (record.header.kind)
	{
	case telemetry_record_kind::interval:
		value += ':' + std::to_string(record.payload.interval.reserved);
		break;
	case telemetry_record_kind::session_lifecycle:
		value += ':' + std::to_string(record.payload.lifecycle.reserved);
		break;
	case telemetry_record_kind::coverage_gap:
		for (auto byte : record.payload.gap.reserved)
			value += ':' + std::to_string(byte);
		break;
	case telemetry_record_kind::progression:
		value += ':' + std::to_string(record.payload.progression.reserved);
		break;
	case telemetry_record_kind::encounter:
		value += ':' + std::to_string(record.payload.encounter.reserved);
		break;
	case telemetry_record_kind::combat_summary:
		value += ':' + std::to_string(record.payload.combat_summary.reserved);
		break;
	case telemetry_record_kind::configuration:
		value += ':' + std::to_string(record.payload.configuration.config.reserved);
		value += ':' + std::to_string(record.payload.configuration.config.schema_version);
		break;
	default:
		break;
	}
	return value;
}

const telemetry_session_ref *session_of(const telemetry_record &record)
{
	switch (record.header.kind)
	{
	case telemetry_record_kind::interval:
		return &record.payload.interval.session;
	case telemetry_record_kind::session_lifecycle:
		return &record.payload.lifecycle.session;
	case telemetry_record_kind::session_checkpoint:
		return &record.payload.checkpoint.session;
	case telemetry_record_kind::coverage_gap:
		return &record.payload.gap.session;
	case telemetry_record_kind::progression:
		return &record.payload.progression.session;
	default:
		return nullptr;
	}
}

std::string session_identity(const telemetry_session_ref &session)
{
	return "session_boot_id=" + std::to_string(session.id.producer.boot_id) +
	       " AND session_process_id=" + std::to_string(session.id.producer.process_id) +
	       " AND session_seq=" + std::to_string(session.id.session_seq);
}

std::uint64_t unsigned_cell(const char *value)
{
	if (!value || !*value)
		throw sql_failure{ 0 };
	std::uint64_t number = 0;
	for (const char *p = value; *p; ++p)
	{
		if (*p < '0' || *p > '9' || number > (UINT64_MAX - (*p - '0')) / 10)
			throw sql_failure{ 0 };
		number = number * 10 + (*p - '0');
	}
	return number;
}

bool record_producer_matches_fresh_identity(const telemetry_record &record)
{
	const auto &fresh = settings.fresh_producer;
	return telemetry_producer_id_is_zero(fresh) ||
	       (record.header.key.producer.boot_id == fresh.boot_id &&
		record.header.key.producer.process_id == fresh.process_id);
}

telemetry_apply_outcome apply_record(const telemetry_record &record)
{
	if (!record_producer_matches_fresh_identity(record) || !telemetry_record_is_valid(record))
		return telemetry_apply_outcome::rejected_invalid;
	const auto values = record_fields(record);
	const fields replay(values.begin(), values.begin() + 3);
	auto stored = query("SELECT " + names(values, true) + " FROM telemetry_interval WHERE " +
			    where(replay));
	if (auto row = mysql_fetch_row(stored.get()))
		return equal_row(row, values) ? telemetry_apply_outcome::duplicate_identical :
						telemetry_apply_outcome::duplicate_conflict;

	if (record.header.kind == telemetry_record_kind::configuration)
	{
		const auto &config = record.payload.configuration.config;
		if (!fingerprint_valid(config))
			return telemetry_apply_outcome::rejected_invalid;
		const auto columns = config_fields(config, false);
		const fields key(columns.begin(), columns.begin() + 2);
		// config_id identifies semantic content across producers. Publication
		// time and local revision belong to each immutable configuration fact,
		// not to the shared content identity. Retain the first projection row.
		fields identity_columns;
		for (const auto &column : columns)
			if (column.first != "revision" && column.first != "effective_utc_usec")
				identity_columns.push_back(column);
		auto existing = query("SELECT " + names(identity_columns, true) +
				      " FROM telemetry_config WHERE " + where(key) + " FOR UPDATE");
		if (auto row = mysql_fetch_row(existing.get()))
		{
			if (!equal_row(row, identity_columns))
				return telemetry_apply_outcome::duplicate_conflict;
		}
		else
			insert("telemetry_config", columns);
	}
	if (record.header.kind == telemetry_record_kind::encounter)
	{
		const auto &p = record.payload.encounter;
		fields expected;
		number(expected, "season_id", p.source.season_id);
		number(expected, "classifier_version", p.source.classifier_version);
		number(expected, "policy_version", p.source.policy_version);
		auto config = query("SELECT " + names(expected, true) +
				    " FROM telemetry_config WHERE environment_id=" +
				    std::to_string(p.source.environment_id) +
				    " AND config_id=" + std::to_string(p.source.config_id));
		auto row = mysql_fetch_row(config.get());
		if (!row || !equal_row(row, expected))
			return telemetry_apply_outcome::rejected_invalid;
	}
	if (record.header.kind == telemetry_record_kind::combat_summary)
	{
		const auto &p = record.payload.combat_summary;
		fields expected;
		number(expected, "season_id", p.source.season_id);
		number(expected, "classifier_version", p.source.classifier_version);
		number(expected, "policy_version", p.source.policy_version);
		auto config = query("SELECT " + names(expected, true) +
				    " FROM telemetry_config WHERE environment_id=" +
				    std::to_string(p.source.environment_id) +
				    " AND config_id=" + std::to_string(p.source.config_id));
		auto row = mysql_fetch_row(config.get());
		if (!row || !equal_row(row, expected))
			return telemetry_apply_outcome::rejected_invalid;
	}

	const auto *session = session_of(record);
	const bool scoped = session && !telemetry_session_ref_is_zero(*session);
	if (scoped && record.header.kind != telemetry_record_kind::coverage_gap)
	{
		fields expected;
		number(expected, "season_id", session->season_id);
		telemetry_id config_id = 0;
		if (record.header.kind == telemetry_record_kind::interval)
		{
			const auto &p = record.payload.interval;
			config_id = p.config_id;
			number(expected, "classifier_version", p.classifier_version);
			number(expected, "policy_version", p.policy_version);
		}
		else if (record.header.kind == telemetry_record_kind::session_lifecycle)
		{
			const auto &p = record.payload.lifecycle;
			config_id = p.config_id;
			number(expected, "classifier_version", p.classifier_version);
			number(expected, "policy_version", p.policy_version);
		}
		else if (record.header.kind == telemetry_record_kind::progression)
		{
			const auto &p = record.payload.progression;
			config_id = p.config_id;
			number(expected, "classifier_version", p.classifier_version);
			number(expected, "policy_version", p.policy_version);
		}
		else
			config_id = record.payload.checkpoint.config_id;
		auto config = query("SELECT " + names(expected, true) +
				    " FROM telemetry_config WHERE "
				    "environment_id=" +
				    std::to_string(session->environment_id) +
				    " AND config_id=" + std::to_string(config_id));
		auto row = mysql_fetch_row(config.get());
		if (!row || !equal_row(row, expected))
			return telemetry_apply_outcome::rejected_invalid;
	}

	telemetry_apply_outcome outcome = telemetry_apply_outcome::applied;
	fields projection;
	std::string identity;
	if (scoped)
	{
		identity = session_identity(*session);
		fields scope;
		session_fields(scope, *session);
		auto current = query(
			"SELECT " + names(scope) +
			",latest_revision,connected_usec,active_usec,"
			"idle_usec,unknown_usec,resident_usec,linkdead_usec,quality_flags,entered,exited "
			"FROM telemetry_session WHERE " +
			identity + " FOR UPDATE");
		auto row = mysql_fetch_row(current.get());
		if (row && !equal_row(row, scope))
			return telemetry_apply_outcome::rejected_invalid;
		std::uint32_t quality = row ? unsigned_cell(row[14]) :
					      TELEMETRY_QUALITY_UNCLOSED_TAIL;
		if (record.header.kind == telemetry_record_kind::session_checkpoint)
		{
			const auto &p = record.payload.checkpoint;
			const auto counters = counter_fields(p.cumulative);
			// Historical revision checks must not be hidden by a newer projection.
			auto old = query(
				"SELECT " + names(counters) + " FROM telemetry_interval WHERE " +
				identity +
				" AND environment_id=" + std::to_string(session->environment_id) +
				" AND season_id=" + std::to_string(session->season_id) +
				" AND checkpoint_revision=" + std::to_string(p.revision) +
				" LIMIT 1");
			if (auto prior = mysql_fetch_row(old.get());
			    prior && !equal_row(prior, counters))
				return telemetry_apply_outcome::duplicate_conflict;
			const auto latest = row ? unsigned_cell(row[7]) : 0U;
			if (row && p.revision <= latest)
			{
				if (p.revision == latest)
					for (std::size_t i = 0; i < counters.size(); ++i)
						if (counters[i].second != row[8 + i])
							return telemetry_apply_outcome::
								duplicate_conflict;
				outcome = telemetry_apply_outcome::checkpoint_older;
			}
			else
			{
				if (row)
					for (std::size_t i = 0; i < counters.size(); ++i)
						if (unsigned_cell(counters[i].second.c_str()) <
						    unsigned_cell(row[8 + i]))
							return telemetry_apply_outcome::
								duplicate_conflict;
				number(projection, "latest_revision", p.revision);
				projection.insert(projection.end(), counters.begin(),
						  counters.end());
			}
			quality |= p.quality_flags;
		}
		else if (record.header.kind == telemetry_record_kind::session_lifecycle)
		{
			const auto &p = record.payload.lifecycle;
			if (p.lifecycle == telemetry_lifecycle_kind::session_entered)
			{
				number(projection, "entered", 1);
				if (!row)
					quality = 0;
			}
			if (p.lifecycle == telemetry_lifecycle_kind::session_exited)
			{
				number(projection, "exited", 1);
				number(projection, "end_reason", p.end_reason);
			}
			quality |= p.quality_flags;
		}
		else if (record.header.kind == telemetry_record_kind::interval)
			quality |= record.payload.interval.quality_flags;
		else if (record.header.kind == telemetry_record_kind::progression)
			quality |= record.payload.progression.quality_flags;
		else
			quality |= record.payload.gap.quality_flags |
				   TELEMETRY_QUALITY_SEQUENCE_GAP;
		number(projection, "quality_flags", quality);
		if (!row)
			insert("telemetry_session", scope);
	}

	fields fact = values;
	// The sole writer's transaction defines the committed ingest prefix. UTC
	// is assigned by the database, never copied from the occurrence label.
	fact.emplace_back("ingested_utc_usec",
			  "CAST(UNIX_TIMESTAMP(CURRENT_TIMESTAMP(6))*1000000 AS SIGNED)");
	insert("telemetry_interval", fact);
	if (scoped)
	{
		number(projection, "last_ingest_id", mysql_insert_id(connection));
		update("telemetry_session", projection, identity);
	}
	return outcome;
}
void batch_failure(telemetry_apply_batch_result &result, const telemetry_record *records,
		   std::size_t count, bool committing, unsigned int error)
{
	// This path must not allocate, even when failure was memory exhaustion.
	if (connection && !committing)
	{
		try
		{
			mysql_real_query(connection, "ROLLBACK", 8);
		}
		catch (...)
		{ /* Closing below also discards the transaction. */
		}
	}
	disconnect();
	result.applied_count = result.duplicate_count = result.stale_checkpoint_count = 0;
	result.invalid_count = result.conflict_count = 0;
	result.result_count = static_cast<std::uint16_t>(count);
	result.outcome = committing ? telemetry_batch_outcome::commit_ambiguous :
				      telemetry_batch_outcome::retryable_failure;
	result.error_code = error;
	for (std::size_t i = 0; i < count; ++i)
	{
		result.results[i] = {};
		result.results[i].key = records[i].header.key;
		result.results[i].outcome = committing ? telemetry_apply_outcome::commit_ambiguous :
							 telemetry_apply_outcome::retryable_failure;
		result.results[i].error_code = error;
	}
}

#endif

[[maybe_unused]] void publish(const telemetry_apply_batch_result &result)
{
	std::lock_guard<std::mutex> lock(health_mutex);
	health.last_error_code = result.error_code;
	if (result.outcome == telemetry_batch_outcome::committed ||
	    result.outcome == telemetry_batch_outcome::committed_with_rejections)
	{
		saturating_add(health.applied_records, result.applied_count);
		saturating_add(health.duplicate_records, result.duplicate_count);
		saturating_add(health.stale_checkpoint_records, result.stale_checkpoint_count);
		saturating_add(health.invalid_records, result.invalid_count);
		saturating_add(health.conflict_records, result.conflict_count);
		health.last_success_monotonic_usec = now_usec();
		health.state = result.invalid_count || result.conflict_count ?
				       telemetry_health_state::degraded :
				       telemetry_health_state::healthy;
	}
	else
	{
		health.last_failure_monotonic_usec = now_usec();
		health.state = telemetry_health_state::degraded;
		if (result.outcome == telemetry_batch_outcome::commit_ambiguous)
			saturating_add(health.ambiguous_commits, 1);
		if (result.outcome == telemetry_batch_outcome::retryable_failure)
			saturating_add(health.retryable_failures, 1);
	}
	if (stop_requested.load())
		health.state = telemetry_health_state::stopping;
}
}

telemetry_repository_outcome telemetry_repository_init(telemetry_repository_config config)
{
	if (!telemetry_repository_config_is_bounded(config))
		return telemetry_repository_outcome::invalid_config;
	if (initialized)
		return stop_requested.load() ? telemetry_repository_outcome::stopping :
					       telemetry_repository_outcome::already_ready;
	{
		std::lock_guard<std::mutex> lock(health_mutex);
		// Only post-join shutdown establishes a fresh lifecycle. A stop sent
		// before the worker reaches init must not be erased by startup.
		if (stop_requested.load() && !restart_allowed)
		{
			health.state = telemetry_health_state::stopping;
			return telemetry_repository_outcome::stopping;
		}
		restart_allowed = false;
		settings = config;
		stop_requested.store(false);
		health = {};
		health.schema_version = TELEMETRY_SCHEMA_VERSION;
		health.backend = config.backend;
		health.state = telemetry_health_state::starting;
	}
#ifdef __NO_MYSQL__
	const bool disabled = true;
#else
	const bool disabled = config.backend == telemetry_storage_backend::flatfile_disabled ||
			      persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY;
#endif
	if (disabled)
	{
		std::lock_guard<std::mutex> lock(health_mutex);
		if (stop_requested.load())
		{
			health.state = telemetry_health_state::stopping;
			return telemetry_repository_outcome::stopping;
		}
		health.backend = telemetry_storage_backend::flatfile_disabled;
		health.state = telemetry_health_state::disabled;
		health.disabled_reason = telemetry_disabled_reason::flatfile_authority;
		return telemetry_repository_outcome::flatfile_disabled;
	}
#ifndef __NO_MYSQL__
	mysql_thread_guard thread;
	const bool opened = thread.ready && open_connection();
	bool usable = opened;
	if (opened && !freshness_verified)
	{
		if (check_fresh_producer() != fresh_producer_check::clear)
		{
			disconnect();
			usable = false;
		}
		else
			freshness_verified = true;
	}
	initialized = usable;
	pending_count = 0;
	std::lock_guard<std::mutex> lock(health_mutex);
	if (stop_requested.load())
	{
		health.state = telemetry_health_state::stopping;
		return telemetry_repository_outcome::stopping;
	}
	health.state = usable ? telemetry_health_state::healthy : telemetry_health_state::degraded;
	return usable ? telemetry_repository_outcome::ready :
			telemetry_repository_outcome::unavailable;
#endif
}

telemetry_apply_batch_result telemetry_repository_apply(const telemetry_record *records,
							std::size_t count)
{
	telemetry_apply_batch_result result{};
	result.outcome = telemetry_batch_outcome::unavailable;
#ifdef __NO_MYSQL__
	(void)records;
	(void)count;
	result.outcome = telemetry_batch_outcome::disabled;
	return result;
#else
	if (telemetry_repository_health_copy().state == telemetry_health_state::disabled)
	{
		result.outcome = telemetry_batch_outcome::disabled;
		return result;
	}
	if (!initialized || stop_requested.load())
		return result;
	if (!records || count == 0 || count > settings.max_batch_records ||
	    count > settings.max_batch_bytes / sizeof(telemetry_record))
	{
		result.outcome = telemetry_batch_outcome::invalid_batch;
		return result;
	}
	result.input_count = static_cast<std::uint16_t>(count);
	result.first_record_seq = records[0].header.key.record_seq;
	result.last_record_seq = records[count - 1].header.key.record_seq;
	const bool retry = pending_count != 0;
	if (!retry)
	{
		std::copy_n(records, count, pending.begin());
		pending_count = count;
	}
	bool committing = false;
	mysql_thread_guard thread;
	try
	{
		if (retry)
		{
			if (pending_count != count)
			{
				result.outcome = telemetry_batch_outcome::invalid_batch;
				return result;
			}
			for (std::size_t i = 0; i < count; ++i)
				if (signature(pending[i]) != signature(records[i]))
				{
					result.outcome = telemetry_batch_outcome::invalid_batch;
					return result;
				}
		}
		if (!thread.ready)
			throw sql_failure{ 0 };
		if (!connection && !open_connection())
			throw sql_failure{ 0 };
		query("START TRANSACTION");
		for (std::size_t i = 0; i < count; ++i)
		{
			query("SAVEPOINT telemetry_record");
			const auto outcome = apply_record(records[i]);
			if (outcome == telemetry_apply_outcome::rejected_invalid ||
			    outcome == telemetry_apply_outcome::duplicate_conflict)
				query("ROLLBACK TO SAVEPOINT telemetry_record");
			query("RELEASE SAVEPOINT telemetry_record");
			result.results[i].key = records[i].header.key;
			result.results[i].outcome = outcome;
			++result.result_count;
			switch (outcome)
			{
			case telemetry_apply_outcome::applied:
				++result.applied_count;
				break;
			case telemetry_apply_outcome::duplicate_identical:
				++result.duplicate_count;
				break;
			case telemetry_apply_outcome::checkpoint_older:
				++result.stale_checkpoint_count;
				break;
			case telemetry_apply_outcome::rejected_invalid:
				++result.invalid_count;
				break;
			case telemetry_apply_outcome::duplicate_conflict:
				++result.conflict_count;
				break;
			default:
				break;
			}
		}
		committing = true;
		query("COMMIT");
		pending_count = 0;
		result.outcome = result.invalid_count || result.conflict_count ?
					 telemetry_batch_outcome::committed_with_rejections :
					 telemetry_batch_outcome::committed;
	}
	catch (const sql_failure &failure)
	{
		batch_failure(result, records, count, committing, failure.code);
	}
	catch (const std::bad_alloc &)
	{
		batch_failure(result, records, count, committing, ENOMEM);
	}
	catch (...)
	{
		batch_failure(result, records, count, committing, EIO);
	}
	publish(result);
	return result;
#endif
}

telemetry_health_snapshot telemetry_repository_health_copy(void)
{
	std::lock_guard<std::mutex> lock(health_mutex);
	return health;
}

telemetry_repository_outcome telemetry_repository_request_stop(void)
{
	std::lock_guard<std::mutex> lock(health_mutex);
	stop_requested.store(true);
	restart_allowed = false;
	if (health.state == telemetry_health_state::stopped)
		return telemetry_repository_outcome::closed;
	health.state = telemetry_health_state::stopping;
	return telemetry_repository_outcome::stopping;
}

void telemetry_repository_shutdown(void)
{
	// Caller owns and has joined the sole SQL worker before entering here.
#ifndef __NO_MYSQL__
	mysql_thread_guard thread;
	disconnect();
	pending_count = 0;
	pending = {};
	freshness_verified = false;
#endif
	initialized = false;
	stop_requested.store(true);
	std::lock_guard<std::mutex> lock(health_mutex);
	restart_allowed = true;
	health.state = telemetry_health_state::stopped;
}

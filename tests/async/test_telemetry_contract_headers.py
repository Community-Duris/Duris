#!/usr/bin/env python3
"""Standalone C++20 compile/layout checks for the #260 telemetry contract."""

from pathlib import Path
import os
import json
import re
import shlex
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
PUBLIC_HEADERS = (
    "telemetry/telemetry_types.h",
    "telemetry/telemetry_progression.h",
    "telemetry/telemetry_runtime.h",
    "telemetry/telemetry_transport.h",
    "telemetry/telemetry_repository.h",
    "telemetry/telemetry_config.h",
)


POSITIVE_HARNESS = r'''
#include "telemetry/telemetry_types.h"
#include "telemetry/telemetry_progression.h"
#include "telemetry/telemetry_runtime.h"
#include "telemetry/telemetry_transport.h"
#include "telemetry/telemetry_repository.h"
#include "telemetry/telemetry_config.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

static_assert(TELEMETRY_SCHEMA_VERSION == 1U);
static_assert(TELEMETRY_RECORD_MAX_BYTES == 512U);
static_assert(TELEMETRY_CONFIG_FINGERPRINT_BYTES == 32U);
static_assert(TELEMETRY_ACTIVE_WINDOW_USEC_PROPOSAL == 300'000'000ULL);
static_assert(TELEMETRY_ACTIVE_WINDOW_USEC_MAX_PROPOSAL == 3'600'000'000ULL);
static_assert(sizeof(telemetry_record) <= TELEMETRY_RECORD_MAX_BYTES);
static_assert(sizeof(telemetry_config_snapshot::fingerprint) == 32U);
static_assert(std::is_trivially_copyable_v<telemetry_record>);
static_assert(std::is_standard_layout_v<telemetry_record>);
static_assert(std::is_trivially_copyable_v<telemetry_interval_payload>);
static_assert(std::is_trivially_copyable_v<telemetry_session_checkpoint_payload>);
static_assert(std::is_trivially_copyable_v<telemetry_configuration_payload>);
static_assert(std::is_trivially_copyable_v<telemetry_progression_payload>);
static_assert(std::is_trivially_copyable_v<telemetry_connection_transition>);
static_assert(std::is_trivially_copyable_v<telemetry_health_snapshot>);

constexpr telemetry_producer_id producer = {11U, 22U};
constexpr telemetry_session_ref session = {{producer, 33U}, 44U, 55, 66U, 77U};
constexpr telemetry_connection_id connection = {producer, 88U};
constexpr telemetry_connection_id copyover_connection = {{111U, 222U}, 1U};
constexpr telemetry_connection_id detached_connection = {};
constexpr telemetry_cumulative_counters totals = {100U, 40U, 50U, 10U, 150U, 50U};
constexpr telemetry_record_key record_key = {producer, 99U};
constexpr telemetry_record_header header = {
    TELEMETRY_SCHEMA_VERSION, telemetry_record_kind::interval, 0U, record_key, 1234};

static_assert(telemetry_producer_id_is_valid(producer));
static_assert(telemetry_session_ref_is_valid(session));
static_assert(telemetry_connection_id_is_valid(connection));
static_assert(telemetry_connection_id_is_zero(detached_connection));
static_assert(telemetry_connection_reference_is_valid(detached_connection));
static_assert(telemetry_record_header_is_valid(header));
static_assert(telemetry_cumulative_counters_are_valid(totals));
static_assert(telemetry_record_kind_is_control(telemetry_record_kind::session_checkpoint));
static_assert(telemetry_record_kind_is_control(telemetry_record_kind::configuration));
static_assert(!telemetry_record_kind_is_control(telemetry_record_kind::interval));
static_assert(static_cast<std::uint8_t>(telemetry_record_kind::configuration) == 5U);
static_assert(static_cast<std::uint8_t>(telemetry_record_kind::progression) == 6U);
static_assert(static_cast<std::uint8_t>(telemetry_lifecycle_kind::session_entered) == 1U);
static_assert(static_cast<std::uint8_t>(telemetry_lifecycle_kind::session_exited) == 2U);
static_assert(static_cast<std::uint8_t>(telemetry_lifecycle_kind::connection_attached) == 3U);
static_assert(static_cast<std::uint8_t>(telemetry_lifecycle_kind::connection_detached) == 4U);
static_assert(static_cast<std::uint8_t>(telemetry_connection_transition_kind::attached) == 1U);
static_assert(static_cast<std::uint8_t>(telemetry_connection_transition_kind::detached) == 2U);
static_assert(static_cast<std::uint8_t>(telemetry_connection_transition_kind::copyover_resumed) == 3U);
static_assert(static_cast<std::uint8_t>(telemetry_interval_category::resident_linkdead) == 3U);

constexpr telemetry_config_snapshot sql_config = [] {
    telemetry_config_snapshot value{};
    value.schema_version = TELEMETRY_SCHEMA_VERSION;
    value.config_id = 101U;
    value.revision = 1U;
    value.build_version = 2U;
    value.content_version = 3U;
    value.property_version = 4U;
    value.classifier_version = 5U;
    value.policy_version = 6U;
    value.season_id = 66U;
    value.environment_id = 77U;
    value.fingerprint[0] = 0xa5U;
    value.effective_utc_usec = 1234;
    value.interval_usec = TELEMETRY_INTERVAL_USEC_PROPOSAL;
    value.checkpoint_interval_usec = TELEMETRY_INTERVAL_USEC_PROPOSAL;
    value.active_window_usec = TELEMETRY_ACTIVE_WINDOW_USEC_PROPOSAL;
    value.context_segments_per_minute = TELEMETRY_CONTEXT_SEGMENTS_PER_MINUTE_PROPOSAL;
    value.pulse_slot_count = 8U;
    value.backend = telemetry_storage_backend::sql;
    value.enabled = 1U;
    return value;
}();
static_assert(telemetry_config_snapshot_identity_is_valid(sql_config));
static_assert(telemetry_config_validate(sql_config) == telemetry_config_validation::valid_sql);

constexpr telemetry_config_snapshot flatfile_config = [] {
    telemetry_config_snapshot value = sql_config;
    value.config_id = 102U;
    value.revision = 2U;
    value.backend = telemetry_storage_backend::flatfile_disabled;
    value.enabled = 0U;
    return value;
}();
static_assert(telemetry_config_validate(flatfile_config) ==
              telemetry_config_validation::valid_flatfile_disabled);

constexpr telemetry_transport_config transport_config = {
    telemetry_storage_backend::sql,
    0U,
    TELEMETRY_SCHEMA_VERSION,
    static_cast<std::uint32_t>(TELEMETRY_QUEUE_CAPACITY_PROPOSAL),
    static_cast<std::uint32_t>(TELEMETRY_CONTROL_RESERVE_PROPOSAL),
    static_cast<std::uint16_t>(TELEMETRY_BATCH_MAX_RECORDS_PROPOSAL),
    0U,
    static_cast<std::uint32_t>(TELEMETRY_BATCH_MAX_BYTES_PROPOSAL),
    TELEMETRY_FLUSH_OLDEST_USEC_PROPOSAL,
};
static_assert(telemetry_transport_config_is_bounded(transport_config));

constexpr telemetry_repository_config repository_config = {
    telemetry_storage_backend::sql,
    0U,
    TELEMETRY_SCHEMA_VERSION,
    static_cast<std::uint32_t>(TELEMETRY_BATCH_MAX_RECORDS_PROPOSAL),
    static_cast<std::uint32_t>(TELEMETRY_BATCH_MAX_BYTES_PROPOSAL),
};
static_assert(telemetry_repository_config_is_bounded(repository_config));

constexpr telemetry_connection_transition transition = {
    session,
    copyover_connection,
    200U,
    1234,
    telemetry_connection_transition_kind::copyover_resumed,
    {0U, 0U, 0U},
    TELEMETRY_QUALITY_NONE,
};
static_assert(telemetry_connection_transition_is_valid(transition));

constexpr telemetry_interval_payload overflow_context_interval = [] {
    telemetry_interval_payload value{};
    value.session = session;
    value.connection = connection;
    value.window = {100U, 200U, 1000, 1100};
    value.duration_usec = 100U;
    value.category = telemetry_interval_category::connected_active;
    value.context = telemetry_activity_context::overflow_unknown;
    value.context_quality = telemetry_context_quality::overflow;
    value.dimensions = {0U, 0U, 0U, 0U, -1, 0U};
    value.config_id = sql_config.config_id;
    value.classifier_version = sql_config.classifier_version;
    value.policy_version = sql_config.policy_version;
    value.quality_flags = TELEMETRY_QUALITY_CONTEXT_OVERFLOW |
                          TELEMETRY_QUALITY_DIMENSION_UNKNOWN;
    return value;
}();
static_assert(telemetry_interval_payload_is_valid(overflow_context_interval));

constexpr telemetry_interval_payload linkdead_interval = [] {
    telemetry_interval_payload value{};
    value.session = session;
    value.connection = detached_connection;
    value.window = {300U, 350U, 1200, 1250};
    value.duration_usec = 50U;
    value.category = telemetry_interval_category::resident_linkdead;
    value.context = telemetry_activity_context::unknown;
    value.context_quality = telemetry_context_quality::unavailable;
    value.config_id = sql_config.config_id;
    value.classifier_version = sql_config.classifier_version;
    value.policy_version = sql_config.policy_version;
    value.quality_flags = TELEMETRY_QUALITY_NONE;
    return value;
}();
static_assert(telemetry_interval_payload_is_valid(linkdead_interval));

constexpr telemetry_session_checkpoint_payload detached_checkpoint = [] {
    telemetry_session_checkpoint_payload value{};
    value.session = session;
    value.connection = detached_connection;
    value.revision = 7U;
    value.at_monotonic_usec = 350U;
    value.at_utc_usec = 1250;
    value.cumulative = totals;
    value.config_id = sql_config.config_id;
    value.quality_flags = TELEMETRY_QUALITY_NONE;
    return value;
}();
static_assert(telemetry_session_checkpoint_payload_is_valid(detached_checkpoint));

constexpr telemetry_counter_update connected_update = [] {
    telemetry_counter_update value{};
    value.session = session;
    value.connection = connection;
    value.at_monotonic_usec = 200U;
    value.connected_delta_usec = 100U;
    value.active_delta_usec = 40U;
    value.idle_delta_usec = 50U;
    value.unknown_delta_usec = 10U;
    value.resident_delta_usec = 100U;
    value.linkdead_delta_usec = 0U;
    value.quality_flags = TELEMETRY_QUALITY_NONE;
    return value;
}();
static_assert(telemetry_counter_update_is_valid(connected_update));

constexpr telemetry_counter_update linkdead_update = [] {
    telemetry_counter_update value{};
    value.session = session;
    value.connection = detached_connection;
    value.at_monotonic_usec = 350U;
    value.connected_delta_usec = 0U;
    value.active_delta_usec = 0U;
    value.idle_delta_usec = 0U;
    value.unknown_delta_usec = 0U;
    value.resident_delta_usec = 50U;
    value.linkdead_delta_usec = 50U;
    value.quality_flags = TELEMETRY_QUALITY_NONE;
    return value;
}();
static_assert(telemetry_counter_update_is_valid(linkdead_update));

constexpr telemetry_record interval_record = [] {
    telemetry_record value{};
    value.header = header;
    value.payload.interval = overflow_context_interval;
    return value;
}();
static_assert(telemetry_record_is_valid(interval_record));

constexpr telemetry_record configuration_record = [] {
    telemetry_record value{};
    value.header = {TELEMETRY_SCHEMA_VERSION, telemetry_record_kind::configuration, 0U,
                    record_key, 1234};
    value.payload.configuration.config = sql_config;
    return value;
}();
static_assert(telemetry_record_is_valid(configuration_record));

using runtime_init_signature = telemetry_runtime_outcome (*)(telemetry_runtime_options);
using enqueue_signature = telemetry_enqueue_result (*)(telemetry_record);
using apply_signature = telemetry_apply_batch_result (*)(const telemetry_record *, std::size_t);
using transition_signature = telemetry_capture_result (*)(telemetry_connection_transition);
using config_signature = telemetry_capture_result (*)(telemetry_config_snapshot);
using progression_signature = telemetry_capture_result (*)(struct char_data *,
                                                            struct descriptor_data *,
                                                            telemetry_progression_observation);
static_assert(std::is_same_v<decltype(&telemetry_runtime_init), runtime_init_signature>);
static_assert(std::is_same_v<decltype(&telemetry_transport_enqueue), enqueue_signature>);
static_assert(std::is_same_v<decltype(&telemetry_repository_apply), apply_signature>);
static_assert(std::is_same_v<decltype(&telemetry_runtime_connection_transition),
                             transition_signature>);
static_assert(std::is_same_v<decltype(&telemetry_runtime_game_progression),
                             progression_signature>);
static_assert(std::is_same_v<decltype(&telemetry_config_publish), config_signature>);
constexpr telemetry_session_resume resume = [] {
    telemetry_session_resume value{};
    value.handoff = {session, producer, 7U, totals, TELEMETRY_QUALITY_QUEUE_DROP};
    value.entry = {session, copyover_connection, 10U, 1234,
                   {0U, 0U, 0U, 0U, -1, 0U}, sql_config.config_id,
                   sql_config.classifier_version, sql_config.policy_version,
                   TELEMETRY_QUALITY_QUEUE_DROP};
    return value;
}();
static_assert(telemetry_session_resume_is_valid(resume));
static_assert([] {
    auto value = resume;
    value.handoff.last_checkpoint_revision = 0U;
    if (!telemetry_session_resume_is_valid(value)) return false;
    value.handoff.last_checkpoint_revision = std::numeric_limits<std::uint64_t>::max();
    if (telemetry_session_resume_is_valid(value)) return false;
    value = resume;
    value.entry.session.pid++;
    if (telemetry_session_resume_is_valid(value)) return false;
    value = resume;
    value.entry.connection = connection;
    if (telemetry_session_resume_is_valid(value)) return false;
    value = resume;
    value.entry.quality_flags = 0U;
    if (telemetry_session_resume_is_valid(value)) return false;
    value = resume;
    value.handoff.cumulative.resident_usec--;
    return !telemetry_session_resume_is_valid(value);
}());
static_assert([] {
    auto value = overflow_context_interval;
    value.dimensions.zone_vnum = 100;
    return !telemetry_interval_payload_is_valid(value);
}());
using resume_signature = telemetry_capture_result (*)(telemetry_session_resume);
using handoff_signature = telemetry_handoff_result (*)(telemetry_session_ref);
static_assert(std::is_same_v<decltype(&telemetry_runtime_session_resume), resume_signature>);
static_assert(std::is_same_v<decltype(&telemetry_runtime_session_handoff_copy), handoff_signature>);
'''


NEGATIVE_HARNESS = r'''
#include "telemetry/telemetry_types.h"
#include "telemetry/telemetry_runtime.h"
#include "telemetry/telemetry_transport.h"
#include "telemetry/telemetry_repository.h"
#include "telemetry/telemetry_config.h"

#include <cstdint>
#include <limits>

constexpr telemetry_producer_id producer = {11U, 22U};
constexpr telemetry_session_ref session = {{producer, 33U}, 44U, 55, 66U, 77U};
constexpr telemetry_connection_id connection = {producer, 88U};
constexpr telemetry_connection_id zero_connection = {};
constexpr telemetry_record_key record_key = {producer, 99U};

constexpr telemetry_config_snapshot valid_config = [] {
    telemetry_config_snapshot value{};
    value.schema_version = TELEMETRY_SCHEMA_VERSION;
    value.config_id = 101U;
    value.revision = 1U;
    value.build_version = 2U;
    value.content_version = 3U;
    value.property_version = 4U;
    value.classifier_version = 5U;
    value.policy_version = 6U;
    value.season_id = 66U;
    value.environment_id = 77U;
    value.fingerprint[0] = 1U;
    value.interval_usec = TELEMETRY_INTERVAL_USEC_PROPOSAL;
    value.checkpoint_interval_usec = TELEMETRY_INTERVAL_USEC_PROPOSAL;
    value.active_window_usec = TELEMETRY_ACTIVE_WINDOW_USEC_PROPOSAL;
    value.context_segments_per_minute = TELEMETRY_CONTEXT_SEGMENTS_PER_MINUTE_PROPOSAL;
    value.pulse_slot_count = 8U;
    value.backend = telemetry_storage_backend::sql;
    value.enabled = 1U;
    return value;
}();

constexpr telemetry_session_ref bad_pid = [] {
    telemetry_session_ref value = session;
    value.pid = 0;
    return value;
}();
constexpr telemetry_session_ref bad_environment = [] {
    telemetry_session_ref value = session;
    value.environment_id = 0U;
    return value;
}();
static_assert(!telemetry_session_ref_is_valid(bad_pid));
static_assert(!telemetry_session_ref_is_valid(bad_environment));
static_assert(telemetry_connection_id_is_zero(zero_connection));
static_assert(!telemetry_connection_id_is_valid(zero_connection));
static_assert(!telemetry_connection_reference_is_valid(
    telemetry_connection_id{producer, 0U}));

constexpr telemetry_record_header bad_header_reserved = {
    TELEMETRY_SCHEMA_VERSION, telemetry_record_kind::interval, 1U, record_key, 1};
constexpr telemetry_record_header bad_header_enum = {
    TELEMETRY_SCHEMA_VERSION, static_cast<telemetry_record_kind>(99U), 0U, record_key, 1};
static_assert(!telemetry_record_header_is_valid(bad_header_reserved));
static_assert(!telemetry_record_header_is_valid(bad_header_enum));
static_assert(!telemetry_interval_category_is_valid(
    static_cast<telemetry_interval_category>(99U)));
static_assert(!telemetry_connection_transition_kind_is_valid(
    static_cast<telemetry_connection_transition_kind>(99U)));

constexpr telemetry_cumulative_counters bad_sum = {100U, 40U, 50U, 11U, 150U, 50U};
constexpr telemetry_cumulative_counters bad_overflow = {
    std::numeric_limits<std::uint64_t>::max(),
    std::numeric_limits<std::uint64_t>::max(),
    1U,
    0U,
    std::numeric_limits<std::uint64_t>::max(),
    0U,
};
static_assert(!telemetry_cumulative_counters_are_valid(bad_sum));
static_assert(!telemetry_cumulative_counters_are_valid(bad_overflow));

constexpr telemetry_interval_payload bad_connected_zero_connection = [] {
    telemetry_interval_payload value{};
    value.session = session;
    value.connection = zero_connection;
    value.window = {1U, 2U, 1, 2};
    value.duration_usec = 1U;
    value.category = telemetry_interval_category::connected_active;
    value.context = telemetry_activity_context::none;
    value.context_quality = telemetry_context_quality::observed;
    value.config_id = 1U;
    value.classifier_version = 1U;
    value.policy_version = 1U;
    return value;
}();
static_assert(!telemetry_interval_payload_is_valid(bad_connected_zero_connection));

constexpr telemetry_interval_payload bad_linkdead_partial_connection = [] {
    telemetry_interval_payload value{};
    value.session = session;
    value.connection = {producer, 0U};
    value.window = {1U, 2U, 1, 2};
    value.duration_usec = 1U;
    value.category = telemetry_interval_category::resident_linkdead;
    value.context = telemetry_activity_context::unknown;
    value.context_quality = telemetry_context_quality::unavailable;
    value.config_id = 1U;
    value.classifier_version = 1U;
    value.policy_version = 1U;
    return value;
}();
static_assert(!telemetry_interval_payload_is_valid(bad_linkdead_partial_connection));

constexpr telemetry_session_checkpoint_payload bad_checkpoint_revision = [] {
    telemetry_session_checkpoint_payload value{};
    value.session = session;
    value.connection = zero_connection;
    value.revision = 0U;
    value.cumulative = {0U, 0U, 0U, 0U, 0U, 0U};
    value.config_id = 1U;
    return value;
}();
static_assert(!telemetry_session_checkpoint_payload_is_valid(bad_checkpoint_revision));

constexpr telemetry_record bad_active_payload = [] {
    telemetry_record value{};
    value.header = {TELEMETRY_SCHEMA_VERSION, telemetry_record_kind::interval, 0U,
                    record_key, 1};
    value.payload.interval = bad_connected_zero_connection;
    return value;
}();
static_assert(!telemetry_record_is_valid(bad_active_payload));

constexpr telemetry_config_snapshot zero_config_id = [] {
    telemetry_config_snapshot value = valid_config;
    value.config_id = 0U;
    return value;
}();
constexpr telemetry_config_snapshot zero_fingerprint = [] {
    telemetry_config_snapshot value = valid_config;
    for (std::uint8_t &byte : value.fingerprint)
        byte = 0U;
    return value;
}();
constexpr telemetry_config_snapshot zero_classifier = [] {
    telemetry_config_snapshot value = valid_config;
    value.classifier_version = 0U;
    return value;
}();
constexpr telemetry_config_snapshot zero_policy = [] {
    telemetry_config_snapshot value = valid_config;
    value.policy_version = 0U;
    return value;
}();
constexpr telemetry_config_snapshot flatfile_enabled = [] {
    telemetry_config_snapshot value = valid_config;
    value.backend = telemetry_storage_backend::flatfile_disabled;
    value.enabled = 1U;
    return value;
}();
constexpr telemetry_config_snapshot bad_interval_limit = [] {
    telemetry_config_snapshot value = valid_config;
    value.interval_usec = TELEMETRY_INTERVAL_USEC_MAX_PROPOSAL + 1U;
    return value;
}();
constexpr telemetry_config_snapshot bad_checkpoint_limit = [] {
    telemetry_config_snapshot value = valid_config;
    value.checkpoint_interval_usec = TELEMETRY_CHECKPOINT_INTERVAL_USEC_MAX_PROPOSAL + 1U;
    return value;
}();
constexpr telemetry_config_snapshot bad_active_window_limit = [] {
    telemetry_config_snapshot value = valid_config;
    value.active_window_usec = TELEMETRY_ACTIVE_WINDOW_USEC_MAX_PROPOSAL + 1U;
    return value;
}();
static_assert(!telemetry_config_is_valid(zero_config_id));
static_assert(!telemetry_config_is_valid(zero_fingerprint));
static_assert(!telemetry_config_is_valid(zero_classifier));
static_assert(!telemetry_config_is_valid(zero_policy));
static_assert(telemetry_config_validate(flatfile_enabled) ==
              telemetry_config_validation::invalid_backend);
static_assert(telemetry_config_validate(bad_interval_limit) ==
              telemetry_config_validation::invalid_interval);
static_assert(telemetry_config_validate(bad_checkpoint_limit) ==
              telemetry_config_validation::invalid_checkpoint_interval);
static_assert(telemetry_config_validate(bad_active_window_limit) ==
              telemetry_config_validation::invalid_active_window);

constexpr telemetry_transport_config bad_transport_bytes = {
    telemetry_storage_backend::sql, 0U, TELEMETRY_SCHEMA_VERSION, 1U, 1U, 1U, 0U,
    static_cast<std::uint32_t>(sizeof(telemetry_record) - 1U), 1U};
constexpr telemetry_transport_config bad_transport_reserve = {
    telemetry_storage_backend::sql, 0U, TELEMETRY_SCHEMA_VERSION, 2U, 0U, 1U, 0U,
    static_cast<std::uint32_t>(sizeof(telemetry_record)), 1U};
constexpr telemetry_transport_config bad_transport_reserved = {
    telemetry_storage_backend::sql, 1U, TELEMETRY_SCHEMA_VERSION, 2U, 1U, 1U, 0U,
    static_cast<std::uint32_t>(sizeof(telemetry_record)), 1U};
static_assert(!telemetry_transport_config_is_bounded(bad_transport_bytes));
static_assert(!telemetry_transport_config_is_bounded(bad_transport_reserve));
static_assert(!telemetry_transport_config_is_bounded(bad_transport_reserved));

constexpr telemetry_repository_config bad_repository_bytes = {
    telemetry_storage_backend::sql, 0U, TELEMETRY_SCHEMA_VERSION, 1U,
    static_cast<std::uint32_t>(sizeof(telemetry_record) - 1U)};
static_assert(!telemetry_repository_config_is_bounded(bad_repository_bytes));

// These assertions reproduce the independent review's representation bypasses.
constexpr bool config_record_accepts(telemetry_config_snapshot config) {
    telemetry_record record{};
    record.header = {TELEMETRY_SCHEMA_VERSION, telemetry_record_kind::configuration,
                     0U, record_key, 1};
    record.payload.configuration.config = config;
    return telemetry_record_is_valid(record);
}
static_assert(!config_record_accepts(flatfile_enabled));
static_assert(!config_record_accepts(bad_interval_limit));
static_assert(!config_record_accepts(bad_checkpoint_limit));
static_assert(!config_record_accepts(bad_active_window_limit));
static_assert(telemetry_config_validate(zero_classifier) ==
              telemetry_config_validation::invalid_versions);
static_assert(telemetry_config_validate(zero_fingerprint) ==
              telemetry_config_validation::invalid_fingerprint);
static_assert([] {
    constexpr telemetry_storage_backend backends[] = {telemetry_storage_backend::sql,
                         telemetry_storage_backend::flatfile_disabled};
    for (auto backend : backends) {
        auto config = valid_config;
        config.enabled = 0U;
        config.backend = backend;
        if (!telemetry_config_is_valid(config) || !config_record_accepts(config)) return false;
        config.context_segments_per_minute = TELEMETRY_CONFIG_CONTEXT_SEGMENT_CAP_MAX_PROPOSAL + 1U;
        if (telemetry_config_is_valid(config) || config_record_accepts(config)) return false;
        config.context_segments_per_minute = 0U;
        config.pulse_slot_count = TELEMETRY_CONFIG_PULSE_SLOT_COUNT_MAX_PROPOSAL + 1U;
        if (telemetry_config_is_valid(config) || config_record_accepts(config)) return false;
        config.pulse_slot_count = 0U;
        config.interval_usec = TELEMETRY_INTERVAL_USEC_MAX_PROPOSAL + 1U;
        if (telemetry_config_is_valid(config) || config_record_accepts(config)) return false;
    }
    return true;
}());
constexpr telemetry_transport_config oversized_reserve = {
    telemetry_storage_backend::sql, 0U, TELEMETRY_SCHEMA_VERSION, 8192U, 129U, 1U, 0U,
    static_cast<std::uint32_t>(sizeof(telemetry_record)), 1U};
static_assert(!telemetry_transport_config_is_bounded(oversized_reserve));
static_assert([] {
    telemetry_coverage_gap_payload gap{};
    gap.reason = telemetry_gap_reason::sequence_gap;
    if (!telemetry_coverage_gap_payload_is_valid(gap)) return false;
    gap.first_missing_record_seq = 9U;
    if (telemetry_coverage_gap_payload_is_valid(gap)) return false;
    gap.last_missing_record_seq = 8U;
    if (telemetry_coverage_gap_payload_is_valid(gap)) return false;
    gap.last_missing_record_seq = 9U;
    if (!telemetry_coverage_gap_payload_is_valid(gap)) return false;
    gap.first_missing_record_seq = 0U;
    return !telemetry_coverage_gap_payload_is_valid(gap);
}());
static_assert([] {
    auto interval = bad_connected_zero_connection;
    interval.connection = connection;
    interval.dimensions.zone_vnum = -2;
    return !telemetry_interval_payload_is_valid(interval);
}());
'''


def compile_source(source: str, compiler: list[str], flags: list[str], label: str) -> None:
    artifact_root = ROOT / "bin" / "tests"
    artifact_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="telemetry-contract-", dir=artifact_root) as directory:
        directory = Path(directory)
        source_file = directory / f"{label}.cc"
        object_file = directory / f"{label}.o"
        source_file.write_text(source, encoding="utf-8")
        command = compiler + [
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-pedantic",
            *flags,
            "-I",
            str(SRC),
            "-c",
            str(source_file),
            "-o",
            str(object_file),
        ]
        result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
        if result.returncode != 0:
            sys.stderr.write(f"[{label}] compiler command failed:\n{' '.join(command)}\n")
            sys.stderr.write(result.stdout)
            sys.stderr.write(result.stderr)
            raise SystemExit(result.returncode)


def fixture_header_source() -> str:
    """Compile named JSON fields into real values, not a second header model."""
    from test_telemetry_contract_fixtures import FIXTURE_DIR, EXPECTED_COVERAGE
    kinds = {"session_lifecycle": "lifecycle", "session_checkpoint": "checkpoint",
             "coverage_gap": "gap", "interval": "interval", "configuration": "configuration"}
    enums = {"category": "telemetry_interval_category", "context": "telemetry_activity_context",
             "context_quality": "telemetry_context_quality", "backend": "telemetry_storage_backend",
             "lifecycle": "telemetry_lifecycle_kind", "end_reason": "telemetry_session_end_reason",
             "reason": "telemetry_gap_reason"}

    def assignments(path, value):
        if isinstance(value, dict):
            lines = []
            for key, child in value.items():
                assert re.fullmatch(r"[a-z_]+", key), key
                lines.extend(assignments(path + "." + key, child))
            return lines
        if isinstance(value, list):
            return [line for i, child in enumerate(value)
                    for line in assignments(f"{path}[{i}]", child)]
        key = path.rsplit(".", 1)[-1]
        if key == "fingerprint":
            return [f"{path}[{i}] = {byte}U;" for i, byte in enumerate(bytes.fromhex(value))]
        if isinstance(value, str):
            enum = ("telemetry_record_kind" if path == "v.header.kind" else
                    "telemetry_connection_transition_kind" if key == "kind" else enums[key])
            assert re.fullmatch(r"[a-z_]+", value), value
            expression = enum + "::" + value
        elif type(value) is int:
            expression = ("std::numeric_limits<std::int64_t>::min()" if value == -(1 << 63)
                          else str(value) + ("LL" if value < 0 else "ULL"))
        else:
            raise AssertionError(f"unsupported fixture field {path}")
        return [f"{path} = {expression};"]

    sources = ['#include "telemetry/telemetry_runtime.h"']
    paths = sorted(FIXTURE_DIR.glob("*.json"))
    assert {p.stem for p in paths} == set(EXPECTED_COVERAGE)
    counts = {"records": 0, "configs": 0, "transitions": 0}
    for path in paths:
        fixture = json.loads(path.read_text())
        items = [("configs", "telemetry_config_snapshot", v, "telemetry_config_is_valid")
                 for v in fixture["configurations"]]
        items += [("transitions", "telemetry_connection_transition", v,
                   "telemetry_connection_transition_is_valid") for v in fixture.get("transitions", [])]
        items += [("records", "telemetry_record", v, "telemetry_record_is_valid")
                  for v in fixture["records"]]
        for category, cpp_type, value, validator in items:
            lines = [f"{cpp_type} v{{}};"]
            if category == "records":
                kind = value["header"]["kind"]
                member = kinds[kind]
                lines.append(f"v.payload.{member} = {{}};")
                lines += assignments("v.header", value["header"])
                lines += assignments("v.payload." + member, value["payload"][kind])
            else:
                lines += assignments("v", value)
            lines.append(f"return {validator}(v);")
            sources.append('static_assert([] {\n' + '\n'.join(lines) +
                           '\n}(), "' + path.stem + ':' + category + '");')
            counts[category] += 1
    print("JSON/header bridge:", counts)
    return "\n".join(sources)


def main() -> int:
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    fixture_source = fixture_header_source()
    modes = (("sql", []), ("no-mysql", ["-D__NO_MYSQL__"]))
    for mode, flags in modes:
        for header in PUBLIC_HEADERS:
            source = f'#include "{header}"\nstatic_assert(true);\n'
            compile_source(source, compiler, flags, f"standalone-{mode}-{Path(header).stem}")
        compile_source(POSITIVE_HARNESS, compiler, flags, f"positive-{mode}")
        compile_source(NEGATIVE_HARNESS, compiler, flags, f"negative-{mode}")
        compile_source(fixture_source, compiler, flags, f"fixtures-{mode}")
    print("telemetry contract headers standalone/positive/negative compile checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

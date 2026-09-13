#ifndef DURIS_TELEMETRY_REPOSITORY_H
#define DURIS_TELEMETRY_REPOSITORY_H

#include "telemetry/telemetry_config.h"
#include "telemetry/telemetry_types.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

/*
 * The repository owns exactly one private sink connection in the initial
 * design.  Connection credentials, MYSQL handles, SQL text, and pool
 * ownership stay behind this boundary; none is part of a public value.
 * Init runs on the designated worker; shutdown belongs to its off-game-thread
 * lifetime owner after join. apply and internal config materialization are
 * worker-only bounded operations; request_stop is nonblocking and shutdown
 * follows worker join.  Health is a synchronized cached copy, never a caller
 * SQL query.
 */
enum class telemetry_repository_outcome : std::uint8_t
{
	ready = 0,
	already_ready = 1,
	invalid_config = 2,
	flatfile_disabled = 3,
	unavailable = 4,
	stopping = 5,
	closed = 6,
};

struct telemetry_repository_config
{
	telemetry_storage_backend backend;
	std::uint8_t reserved;
	std::uint16_t schema_version;
	std::uint32_t max_batch_records;
	std::uint32_t max_batch_bytes;
};

struct telemetry_record_apply_result
{
	telemetry_record_key key;
	telemetry_apply_outcome outcome;
	std::uint8_t reserved[3];
	std::uint32_t error_code;
};

/*
 * apply() handles one bounded immutable batch.  A retryable or ambiguous
 * outcome leaves the same batch eligible for retry; it must not be rebuilt
 * with new record sequences.  Results is bounded to the candidate batch max.
 */
struct telemetry_apply_batch_result
{
	telemetry_batch_outcome outcome;
	std::uint8_t reserved[3];
	std::uint16_t input_count;
	std::uint16_t result_count;
	std::uint16_t applied_count;
	std::uint16_t duplicate_count;
	std::uint16_t stale_checkpoint_count;
	std::uint16_t invalid_count;
	std::uint16_t conflict_count;
	std::uint16_t reserved2;
	std::uint32_t error_code;
	telemetry_record_sequence first_record_seq;
	telemetry_record_sequence last_record_seq;
	telemetry_record_apply_result results[TELEMETRY_BATCH_MAX_RECORDS_PROPOSAL];
};

constexpr bool
telemetry_repository_config_is_bounded(const telemetry_repository_config &config) noexcept
{
	return config.schema_version == TELEMETRY_SCHEMA_VERSION && config.reserved == 0U &&
	       telemetry_storage_backend_is_valid(config.backend) &&
	       config.max_batch_records > 0U &&
	       config.max_batch_records <= TELEMETRY_BATCH_MAX_RECORDS_PROPOSAL &&
	       config.max_batch_bytes >= sizeof(telemetry_record) &&
	       config.max_batch_bytes <= TELEMETRY_BATCH_MAX_BYTES_PROPOSAL;
}

telemetry_repository_outcome telemetry_repository_init(telemetry_repository_config config);

/*
 * records is a borrowed contiguous array used only during this worker call.
 * count must be <= TELEMETRY_BATCH_MAX_RECORDS_PROPOSAL and the byte sum must
 * fit the configured batch limit.  No pointer is retained after return.
 */
telemetry_apply_batch_result telemetry_repository_apply(const telemetry_record *records,
							std::size_t count);

/* Configuration materialization is internal to apply(): the configuration
 * record and its config row commit in the SAME transaction under its replay key.
 * There is no independently callable keyless config write path. */
telemetry_health_snapshot telemetry_repository_health_copy(void);
/* Nonblocking worker-stop request; shutdown is only legal after worker join. */
telemetry_repository_outcome telemetry_repository_request_stop(void);
void telemetry_repository_shutdown(void);

static_assert(std::is_trivially_copyable_v<telemetry_repository_config>);
static_assert(std::is_standard_layout_v<telemetry_repository_config>);
static_assert(std::is_trivially_copyable_v<telemetry_record_apply_result>);
static_assert(std::is_trivially_copyable_v<telemetry_apply_batch_result>);

#endif

#ifndef DURIS_TELEMETRY_FAILURE_H
#define DURIS_TELEMETRY_FAILURE_H

#include "telemetry/telemetry_types.h"

#include <cstdint>

enum class telemetry_sql_phase : std::uint8_t
{
	connect = 0,
	statement = 1,
	commit = 2,
};

/* Pure, allocation-free MySQL/MariaDB error classification. */
telemetry_failure_class telemetry_classify_sql_failure(std::uint32_t error_code,
						       telemetry_sql_phase phase) noexcept;

constexpr bool telemetry_failure_is_retryable(telemetry_failure_class value) noexcept
{
	return value == telemetry_failure_class::transient_connection ||
	       value == telemetry_failure_class::transient_transaction ||
	       value == telemetry_failure_class::transient_internal;
}

constexpr bool telemetry_failure_is_permanent(telemetry_failure_class value) noexcept
{
	return value == telemetry_failure_class::permanent_schema ||
	       value == telemetry_failure_class::permanent_permission ||
	       value == telemetry_failure_class::permanent_repository;
}

#endif

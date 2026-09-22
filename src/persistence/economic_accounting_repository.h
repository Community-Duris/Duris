#ifndef DURIS_ECONOMIC_ACCOUNTING_REPOSITORY_H
#define DURIS_ECONOMIC_ACCOUNTING_REPOSITORY_H

#include "economy/economic_accounting_types.h"
#include <mysql/mysql.h>
#include <span>
#include <vector>

// Backend numbers are durable mapping identities (SQL=1, flatfile=2).
constexpr uint8_t ECONOMIC_MAPPING_BACKEND_SQL = 1;

struct economic_sql_mapping_request
{
	economic_account_key account;
	uint16_t locator_kind = 0;
	uint64_t native_id = 0;
};

struct economic_sql_locked_mapping
{
	economic_sql_mapping_request request;
	uint64_t revision = 0;
};

// Value snapshot only, not a capability to append financial evidence. Its locks
// belong to the caller's transaction and expire on rollback/commit/disconnect.
struct economic_sql_authority_snapshot
{
	critical_operation_id lineage = {};
	critical_operation_id epoch = {};
	uint64_t lineage_revision = 0;
	std::vector<economic_sql_locked_mapping> mappings;
};

// Borrow an already active transaction with automatic reconnect disabled.
// Locks the lineage shared, then all
// requested ordinary mappings shared in ascending lifetime ID order. Mapping
// retirement and epoch transitions take exclusive locks on these same rows.
// Submit the complete set once, before domain mutation; duplicate IDs fail.
// Never begins/commits/retries a transaction or creates mappings/epochs. On any
// failure the caller must roll back: earlier locks may remain held. Output is
// unchanged on failure; error is an errno value or the original MySQL error.
// Success proves identity only, not balances, typed effects, or inbox ownership.
// Client-free builds return ENOTSUP without accessing connection or output.
unsigned int economic_sql_lock_authority(MYSQL *connection, const critical_operation_id &lineage,
					 const critical_operation_id &expected_epoch,
					 std::span<const economic_sql_mapping_request> requests,
					 economic_sql_authority_snapshot *snapshot);

#endif

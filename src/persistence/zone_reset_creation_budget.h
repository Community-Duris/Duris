#ifndef ZONE_RESET_CREATION_BUDGET_H
#define ZONE_RESET_CREATION_BUDGET_H
#include "persistence/economic_sql_source_snapshot.h"
struct economic_sql_physical_source_snapshot;
struct sql_room_item_source_snapshot;
struct sql_room_creation_source_snapshot;
struct quest_mobile_native_sql_catalog;

struct zone_reset_creation_budget_totals
{
	uint64_t rows = 0, cells = 0, cell_bytes = 0;
};
// Pure accounting of the complete original base+five-room+creation packet.
// Validate original registries/framing/hard caps first; subtract ONLY creation
// deltas from aggregate maxima. Native capture already charges base+five-room.
// Single-cell limit is unchanged. Exhausted/invalid arithmetic refuses and
// leaves output unchanged. Reduced limits are a derived allowance for that
// existing capture call, never separate source/admission/activation authority.
unsigned int zone_reset_creation_budget_prepare(const economic_sql_physical_source_snapshot &,
						const sql_room_item_source_snapshot &,
						const sql_room_creation_source_snapshot &,
						const economic_sql_source_limits &original,
						economic_sql_source_limits *adjusted) noexcept;
// Revalidate using ORIGINAL limits and actual native raw6 bytes, its original
// base+five-room cumulative counters and original five content digests. Charge
// creation deltas once, publish full-union totals only after exact agreement.
// Keep native owner's existing body codec ceiling, not a new generic-cell
// waiver. This proves budget/framing correspondence only; the caller still
// owns actual same-RR/session provenance, typed proofs and complete census.
// No SQL, side effects or copied raw packets. Strong output on every failure.
unsigned int zone_reset_creation_budget_validate_native(
	const economic_sql_physical_source_snapshot &, const sql_room_item_source_snapshot &,
	const sql_room_creation_source_snapshot &, const economic_sql_source_limits &original,
	const quest_mobile_native_sql_catalog &, zone_reset_creation_budget_totals *) noexcept;
#endif

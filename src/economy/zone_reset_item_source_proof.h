#ifndef ZONE_RESET_ITEM_SOURCE_PROOF_H
#define ZONE_RESET_ITEM_SOURCE_PROOF_H

#include "economy/zone_reset_item_accounting.h"
#include "economy/zone_reset_item_origin.h"
#include "economy/zone_reset_item_recovery.h"
#include "persistence/sql_room_creation_source.h"
#include "persistence/sql_room_item_payload.h"

#include <optional>
#include <vector>

enum zone_reset_item_source_flag : uint32_t
{
	ZONE_RESET_SOURCE_MISSING_ORIGIN = 1U << 0,
	ZONE_RESET_SOURCE_AMBIGUOUS_ORIGIN = 1U << 1,
	ZONE_RESET_SOURCE_MALFORMED_ORIGIN = 1U << 2,
	ZONE_RESET_SOURCE_BAD_RETAINED_ROOT = 1U << 3,
};

struct zone_reset_item_source_family
{
	critical_operation_id operation{};
	size_t origin_row = SIZE_MAX;
	uint32_t flags = 0;
	zone_reset_item_retained_origin origin;
	zone_reset_item_image image;
	economic_accounting_plan plan;
	item_transfer_result result{};
	// Actual optional terminal BODY from the same raw cut. NULL is unknown;
	// complete historical service observations grant no current authority.
	std::optional<zone_reset_item_recovery_context> terminal;
};

// Pure historical evidence inspection within a structurally validated captured
// cut. Every recognizable type22/writer16 family and every origin row survives,
// including missing, malformed, ambiguous and unmatched origins. Flags are data,
// not permission to ignore a damaged family. Hashes do not authenticate the SQL
// session: its actual owner must establish the original same-RR provenance.
// Current custody, season, financial head, complete forest construction and
// physical publication are separate obligations. No SQL, adoption or ACK.
// Allocation/bounds/framing failure leaves output unchanged.
unsigned int zone_reset_item_source_inspect(const economic_sql_physical_source_snapshot &,
					    const sql_room_item_source_snapshot &,
					    const sql_room_creation_source_snapshot &,
					    const economic_sql_source_limits &,
					    std::vector<zone_reset_item_source_family> *) noexcept;

#endif

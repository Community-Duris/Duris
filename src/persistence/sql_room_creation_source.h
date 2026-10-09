#ifndef SQL_ROOM_CREATION_SOURCE_H
#define SQL_ROOM_CREATION_SOURCE_H
#include "persistence/economic_sql_source_snapshot.h"
struct sql_room_item_source_snapshot;
struct economic_sql_physical_source_snapshot;

// Owning version2 nine-table raw supplement. Existing EPH1 and five-table room packets
// are borrowed, never copied/recaptured. NULL and binary bytes remain exact.
// These counters charge ONLY additional selected cells, including overlaps
// with older projections. RSC2 binds version2, both borrowed digests and all
// new bytes; the original private six-table RSC1 packet remains a predecessor.
// A matching packet is evidence framing, not committed-origin/publication or
// complete room-source/activation authority; the typed proof owner is separate.
struct sql_room_creation_source_snapshot
{
	uint32_t version = 2;
	std::vector<economic_sql_source_table> tables;
	economic_sql_source_digest physical_digest = {}, room_digest = {}, digest = {};
	uint64_t additional_rows = 0, additional_cells = 0, additional_cell_bytes = 0;
};
// Pure exact registries/counts/framing and shared original hard-budget checks.
// Malformed semantic cells remain raw evidence for the typed proof inspector.
unsigned int sql_room_creation_source_validate_sources(
	const economic_sql_physical_source_snapshot &, const sql_room_item_source_snapshot &,
	const sql_room_creation_source_snapshot &, const economic_sql_source_limits &) noexcept;
// Caller owns one reconnect-disabled, autocommit, IN_TRANS RR consistent cut,
// READ ONLY or writable, and both original borrowed packets from that same cut.
// SELECT ALL rows under MDL/InnoDB with preflight lengths before body allocation.
// No filters, locks, changes, commit/rollback, retry, recovery or second budget.
// Original limits remain hard:262144 rows/4M cells/64MiB bytes/1MiB single cell.
// Failure preserves output and leaves transaction/session disposal to caller.
unsigned int sql_room_creation_source_capture_in_transaction(
	MYSQL *, const economic_sql_source_limits &, const economic_sql_physical_source_snapshot &,
	const sql_room_item_source_snapshot &, sql_room_creation_source_snapshot *) noexcept;
#endif

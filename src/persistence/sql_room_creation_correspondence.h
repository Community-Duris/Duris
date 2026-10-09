#ifndef SQL_ROOM_CREATION_CORRESPONDENCE_H
#define SQL_ROOM_CREATION_CORRESPONDENCE_H

#include "economy/zone_reset_item_source_proof.h"

enum sql_room_creation_source_flag : uint32_t
{
	SQL_ROOM_CREATION_MISSING_CURRENT_BOOK = 1U << 15,
	SQL_ROOM_CREATION_COMPETING_PHYSICAL = 1U << 16,
};
struct sql_room_creation_source_witness
{
	size_t family_index = SIZE_MAX, image_item = SIZE_MAX;
	sql_room_item_source_witness room;
	bool exact_current = false;
};
struct sql_room_creation_correspondence_evidence
{
	std::vector<zone_reset_item_source_family> families;
	std::vector<sql_room_creation_source_witness> witnesses;
	// Indices refer to THIS evidence's witnesses, in native parent-before-child order.
	std::vector<sql_room_item_source_graph> graphs;
	// Indices refer to the independently recomputed original five-table inspector.
	// Only exact authenticated birth literal witnesses are superseded. The caller
	// retains the original evidence and replaces only these affected findings.
	std::vector<size_t> superseded_room_witness_indices;
	std::vector<sql_room_item_source_diagnostic> diagnostics;
	// Global malformed semantic findings survive detail truncation too.
	uint32_t global_flags = 0;
	uint64_t current_season = 0;
	bool season_active = false, diagnostics_truncated = false;
	bool native_root_limit_exceeded = false;
};
// Pure correspondence, never SQL/publication/adoption/activation authority.
// Recompute both historical creation proof and the unchanged original inspector.
// Retain damaged and historical families. Current money requires authentic
// current-book authority from the version2 nine-table packet. Opening/progressed
// heads refuse explicitly rather than treating historical birth as current.
// Original cumulative bounds apply; every failure leaves output unchanged.
unsigned int sql_room_creation_correspondence_inspect(
	const economic_sql_physical_source_snapshot &, const sql_room_item_source_snapshot &,
	const sql_room_creation_source_snapshot &, const economic_sql_source_limits &,
	size_t maximum_diagnostics, sql_room_creation_correspondence_evidence *) noexcept;

#endif

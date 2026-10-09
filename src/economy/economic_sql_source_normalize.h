#ifndef DURIS_ECONOMIC_SQL_SOURCE_NORMALIZE_H
#define DURIS_ECONOMIC_SQL_SOURCE_NORMALIZE_H
#include "persistence/economic_sql_source_snapshot.h"
#include "economy/economic_accounting_types.h"
#include <optional>

// These are native locators, never economic_account_key lifetime identities.
enum class economic_sql_holding_kind : uint8_t
{
	wallet,
	bank,
	treasury,
	ship,
	auction,
	claim,
	pile
};
enum class economic_sql_holding_disposition : uint8_t
{
	current,
	history,
	unresolved,
	not_holding
};
struct economic_sql_source_reference
{
	// SIZE_MAX row denotes a whole-table issue (e.g. an absent allocator row).
	size_t table = 0, row = 0;
	economic_sql_source_digest digest = {};
};
struct economic_sql_native_holding
{
	economic_sql_source_reference source;
	economic_sql_holding_kind kind = {};
	economic_sql_holding_disposition disposition = {};
	uint64_t native_id = 0;
	int64_t native_context = 0;
	std::optional<uint64_t> native_revision;
	// Null/unrepresentable native values never become zero. Negative vectors
	// remain explicit and generate a defect. The original snapshot retains all
	// raw cells, including unsigned amounts above the accounting range.
	std::optional<economic_coin_vector> balance;
};
struct economic_sql_equipment_source_reference
{
	// Addresses item_equipment_sources[0].rows, never snapshot.tables.
	size_t row = 0;
	economic_sql_source_digest digest = {};
};
struct economic_sql_native_item
{
	economic_sql_source_reference source;
	economic_item_snapshot item;
	int32_t vnum = 0;
	std::optional<uint64_t> owner_revision;
	// Absent for legacy v1: position.equipment_slot's default is not evidence
	// of carried custody. A v2 observed zero is distinct from unobserved.
	std::optional<uint16_t> observed_equipment_slot;
	std::optional<economic_sql_equipment_source_reference> equipment_source;
	// Only a successfully decoded, UID/vnum/type-matching native money payload.
	std::optional<economic_coin_vector> coin_values;
};
struct economic_sql_native_owner
{
	economic_sql_source_reference source;
	item_owner_identity owner = {};
	uint64_t revision = 0;
};
enum class economic_sql_normalization_issue : uint8_t
{
	invalid_identity,
	unknown_money,
	accounting_overflow,
	negative_holding,
	unavailable_native_revision,
	unresolved_auction,
	invalid_custody,
	missing_owner_revision,
	unknown_coin_payload,
	invalid_coin_payload,
	quarantined_item,
	open_quarantine,
	incomplete_receipt,
	pending_publication,
	legacy_item_claim,
	allocator_missing_or_invalid,
	uid_outside_allocator,
	unknown_keeper_configuration,
	count
};
struct economic_sql_normalization_diagnostic
{
	economic_sql_normalization_issue issue = {};
	economic_sql_source_reference source;
};
struct economic_sql_normalized_sources
{
	economic_sql_source_digest source_digest = {};
	// ESC2 only for a validated v2 projection; zero for unobserved legacy v1.
	economic_sql_source_digest custody_digest = {};
	std::vector<economic_sql_native_holding> holdings;
	std::vector<economic_sql_native_item> items;
	std::vector<economic_sql_native_owner> owners;
	std::optional<uint64_t> next_uid;
	std::array<uint64_t, static_cast<size_t>(economic_sql_normalization_issue::count)>
		issue_counts = {};
	uint64_t diagnostic_count = 0;
	bool diagnostics_truncated = false;
	std::vector<economic_sql_normalization_diagnostic> diagnostics;
};
// Pure consumer of an owning native capture. Verify version/registry/complete
// hash framing before parsing; do not mistake this for source authentication.
// Retain the original immutable snapshot beside the normalized report. Every
// reference addresses that snapshot; selected history and unresolved holdings
// stay distinct from current holdings, and no aggregate money total is produced.
// Graph/projection consistency, lifetimes, receipt semantics and global boundary
// are separate. Even a defect-free result does not authorize baseline/activation.
// Hard input bounds are the capture defaults. limit=1..512 bounds stored detail,
// not complete issue counts. Output unchanged on all structural/allocation errors;
// native monetary/custody contradictions produce explicit report diagnostics.
economic_accounting_error
economic_sql_normalize_sources(const economic_sql_source_snapshot &, size_t diagnostic_limit,
			       economic_sql_normalized_sources *) noexcept;

enum class economic_sql_physical_registry : uint8_t
{
	source2_tables,
	source2_item_sources,
	source2_item_equipment_sources,
	physical_sources
};
struct economic_sql_physical_reference
{
	economic_sql_physical_registry registry = {};
	size_t table = 0, row = 0;
	economic_sql_source_digest digest = {};
};
enum class economic_sql_physical_issue : uint8_t
{
	invalid_identity,
	unresolved_owner,
	ambiguous_owner,
	missing_uid,
	duplicate_uid,
	unresolved_parent,
	cross_scope_parent,
	invalid_topology,
	unknown_equipment,
	invalid_equipment,
	duplicate_equipment,
	unsupported_equipment,
	unmatched_physical,
	conflicting_custody,
	unmatched_active_custody,
	count
};
struct economic_sql_physical_item
{
	// One record for EVERY raw item row, including duplicate/NULL/orphan rows.
	// References address the retained immutable packet; no raw cells are copied.
	economic_sql_physical_reference source;
	std::optional<uint64_t> row_id, uid, root_uid, parent_uid;
	std::optional<int32_t> vnum;
	std::optional<item_owner_identity> owner;
	std::optional<uint16_t> observed_equipment_slot;
	std::array<std::optional<economic_sql_physical_reference>, 3> mapping_sources;
	std::optional<economic_sql_physical_reference> parent_source, equipment_source;
	// Addresses source2.items below, even when the observation conflicts.
	std::optional<size_t> custody_index;
	uint32_t issue_mask = 0;
	// Exact row fields only; this does not certify ancestors or the native
	// custody forest, custody authority, complete coverage or activation.
	bool exact_custody_match = false;
};
struct economic_sql_physical_diagnostic
{
	economic_sql_physical_issue issue = {};
	economic_sql_physical_reference source;
};
struct economic_sql_normalized_physical_sources
{
	// Original source2 report is preserved once, with its old indices/issues.
	economic_sql_normalized_sources source2;
	economic_sql_source_digest physical_digest = {};
	std::vector<economic_sql_physical_item> items;
	std::array<uint64_t, static_cast<size_t>(economic_sql_physical_issue::count)>
		issue_counts = {};
	uint64_t diagnostic_count = 0;
	bool diagnostics_truncated = false;
	std::vector<economic_sql_physical_diagnostic> diagnostics;
	// ALL active custody rows without an exact counterpart in these eight raw
	// sources, including conflicted observations. This is not global absence:
	// modern room, auction/collector/native-mobile/live providers are separate.
	std::vector<size_t> unmatched_active_custody_indices;
};
// Pure, bounded same-cut physical correspondence. Input framing/aggregate
// budgets are verified first. Canonical numeric encoding/structural failures
// and allocation failures leave output unchanged; semantic defects retain all
// observations and complete counts. Stored diagnostics use limit=1..512.
// NULL identity/equipment is unresolved evidence, never an invented zero.
// No hydration, new writable catalog, baseline/coin mint or activation decision.
// EPH1 binds this same-cut RR capture (caller transaction read-only or writable);
// it is not a stable install-before/after
// digest or ESN5/native_boundary_digest for a writable cutover transaction.
economic_accounting_error
economic_sql_normalize_physical_sources(const economic_sql_physical_source_snapshot &,
					size_t diagnostic_limit,
					economic_sql_normalized_physical_sources *) noexcept;
// Combine the existing persisted correspondence readers. This is not a
// source-complete census: live/reset/mobile sources and cutover authority remain
// with their original owners. All provider diagnostics and histories survive.
#include "economy/auction_repository.h"
#include "economy/collector_repository.h"
#include "persistence/sql_room_item_payload.h"
#include "persistence/quest_mobile_native_sql.h"
#include "persistence/sql_room_creation_correspondence.h"

enum class economic_sql_persisted_provider : uint8_t
{
	physical,
	room,
	auction,
	collector,
	auction_legacy_identity,
	native_mobile_literal,
	room_creation
};
struct economic_sql_persisted_match
{
	economic_sql_persisted_provider provider = {};
	// Index into physical.items, room.witnesses, auction.nodes, or
	// collector.listings, auction.legacy_identities, or native_mobile.items.
	// The fresh lifecycle caller retains the complete native catalog below.
	// Raw source indices remain in the corresponding witness.
	size_t witness_index = 0;
};
struct economic_sql_persisted_custody_match
{
	// Index into physical.source2.items, not a database item UID or SQL row ID.
	size_t custody_index = 0;
	std::vector<economic_sql_persisted_match> matches;
};
struct economic_sql_persisted_correspondence
{
	economic_sql_normalized_physical_sources physical;
	sql_room_item_source_evidence room;
	auction_physical_correspondence auction;
	collector_physical_source_report collector;
	quest_mobile_native_sql_catalog native_mobile;
	sql_room_creation_correspondence_evidence creation;
	bool creation_observed = false;
	// Every normalized custody row remains indexed, including tombstones.
	// Only exact current correspondence adds a match; historical literals do not.
	std::vector<economic_sql_persisted_custody_match> custody;
	std::vector<size_t> unmatched_active_custody_indices;
	std::vector<size_t> multiply_matched_active_custody_indices;
};
// Pure same-cut orchestration over the caller's immutable base/supplement.
// Calls the original bounded readers; no SQL, recapture, hydration, mutation,
// alias erasure, repair or activation decision. Counts/matches do not erase any
// original provider findings or prove native forest loadability, original
// provenance, full writer coverage, quiescence, or a before/after install cut.
// Operational errors preserve output. The original 1..512 diagnostic and raw
// source ceilings remain; a caller cannot widen the physical normalizer's caps.
economic_accounting_error economic_sql_normalize_persisted_correspondence(
	const economic_sql_physical_source_snapshot &, const sql_room_item_source_snapshot &,
	const economic_sql_source_limits &, size_t diagnostic_limit,
	economic_sql_persisted_correspondence *) noexcept;

// Full creation supplement is independently recomputed. Original room findings
// stay in .room; only exact, authenticated creation witnesses can supersede
// their drop-only classification in the same-cut union. No history is occupancy.
economic_accounting_error economic_sql_normalize_persisted_correspondence(
	const economic_sql_physical_source_snapshot &, const sql_room_item_source_snapshot &,
	const sql_room_creation_source_snapshot &, const economic_sql_source_limits &,
	size_t diagnostic_limit, economic_sql_persisted_correspondence *) noexcept;

#endif

#ifndef DURIS_ECONOMIC_SQL_RUNTIME_CACHE_CORRESPONDENCE_H
#define DURIS_ECONOMIC_SQL_RUNTIME_CACHE_CORRESPONDENCE_H
#include "economy/economic_sql_source_normalize.h"
#include "item/item_ownership_runtime.h"

enum class economic_sql_cache_issue : uint8_t
{
	invalid_cache,
	inactive_cache,
	duplicate_cache_uid,
	missing_persisted_uid,
	duplicate_persisted_uid,
	historical_persisted_uid,
	fields_mismatch,
	missing_sql_owner_clock,
	duplicate_sql_owner_clock,
	missing_runtime_owner_clock,
	current_owner_clock_mismatch,
	future_entry_clock,
	missing_provider,
	multiple_providers,
	count
};
struct economic_sql_cache_row
{
	size_t cache_index = 0;
	std::optional<size_t> custody_index, sql_owner_index;
	uint32_t issues = 0;
	bool exact_current_correspondence = false;
};
struct economic_sql_cache_custody_row
{
	size_t custody_index = 0, active_cache_count = 0;
	size_t current_provider_count = 0;
	uint32_t issues = 0;
};
struct economic_sql_cache_finding
{
	economic_sql_cache_issue issue = {};
	size_t cache_index = SIZE_MAX, custody_index = SIZE_MAX;
};
struct economic_sql_runtime_cache_correspondence
{
	std::vector<economic_sql_cache_row> cache;
	// Every persisted custody row, including history and all provider multiplicity.
	std::vector<economic_sql_cache_custody_row> custody;
	std::vector<size_t> historical_custody_indices;
	std::vector<economic_sql_cache_finding> findings, diagnostics;
	std::array<uint64_t, static_cast<size_t>(economic_sql_cache_issue::count)> issue_counts = {};
	bool diagnostics_truncated = false;
};
// Pure comparison of the complete captured active cache and complete prepared
// persisted union. Caller retains both immutable inputs and every original
// provider/history finding; indices borrow them, never copy or erase providers.
// Parallel real peek_owner observations must have exact cache cardinality.
// Current runtime clock must equal the unique SQL clock (observed zero is valid).
// Entry publication clock may be older; it must not exceed either current clock.
// Every active cache row needs unique exact UID/root/parent/owner/state/VNUM/item
// revision and exactly one current provider. Every persisted row retains its
// cache count, including zero for unloaded durable holdings. This is
// one-way cache comparison, not loaded-world coverage; history grants no occupancy.
// Semantic defects publish complete findings; 1..512 diagnostics only cap detail.
// Existing262144 row caps and64MiB temporary/report allocation bound cannot be
// widened. Structural/limit/allocation failure preserves output. No SQL, global
// cache read, hydration, repair, world completeness or activation authority.
economic_accounting_error economic_sql_compare_runtime_cache(
	std::span<const item_ownership_runtime_entry>,
	std::span<const std::optional<uint64_t>> current_runtime_owner_revisions,
	const economic_sql_persisted_correspondence &, const economic_sql_source_limits &,
	size_t diagnostic_limit, economic_sql_runtime_cache_correspondence *) noexcept;
#endif

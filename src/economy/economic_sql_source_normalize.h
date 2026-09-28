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
struct economic_sql_native_item
{
	economic_sql_source_reference source;
	economic_item_snapshot item;
	int32_t vnum = 0;
	std::optional<uint64_t> owner_revision;
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
#endif

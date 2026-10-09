#ifndef DURIS_ECONOMIC_INITIALIZED_WORLD_MONEY_CORRESPONDENCE_H
#define DURIS_ECONOMIC_INITIALIZED_WORLD_MONEY_CORRESPONDENCE_H

#include "economy/economic_sql_source_normalize.h"
#include "world/economic_initialized_world_snapshot.h"

enum class economic_world_money_issue : uint8_t
{
	invalid_identity,
	unobserved_account,
	invalid_descriptor_correlation,
	duplicate_player_pid,
	missing_bank_projection,
	duplicate_bank_projection,
	account_mismatch,
	racewar_mismatch,
	missing_sql_counterpart,
	duplicate_sql_bank_identity,
	unknown_balance,
	negative_balance,
	accounting_overflow,
	unavailable_revision,
	revision_mismatch,
	balance_mismatch,
	conflicting_shared_projection,
	count
};
struct economic_world_wallet_correspondence
{
	size_t body_index = SIZE_MAX;
	std::optional<size_t> holding_index;
	uint32_t issues = 0;
	bool exact_current_correspondence = false;
};
struct economic_world_bank_correspondence
{
	// Every projection survives; equal account/racewar projections share one
	// comparison_index. No balance or account name is copied or summed.
	size_t projection_index = SIZE_MAX;
	std::optional<size_t> comparison_index;
	uint32_t issues = 0;
};
struct economic_world_bank_comparison
{
	size_t first_projection_index = SIZE_MAX, projection_count = 0;
	std::optional<size_t> holding_index;
	uint32_t issues = 0;
	bool exact_current_correspondence = false;
};
struct economic_world_sql_money_correspondence
{
	size_t holding_index = SIZE_MAX, observed_counterpart_count = 0;
	uint32_t issues = 0;
	// Zero counterparts is explicitly unloaded/unobserved, not global absence.
};
struct economic_world_money_diagnostic
{
	economic_world_money_issue issue = {};
	size_t body_index = SIZE_MAX, projection_index = SIZE_MAX, holding_index = SIZE_MAX;
};
struct economic_initialized_world_money_correspondence
{
	// Recomputed by the original normalizer from retained raw source2. Preserve
	// all its other money/item/owner/history findings; none grants world coverage.
	economic_sql_normalized_sources normalized_sources;
	std::vector<economic_world_wallet_correspondence> wallets;
	std::vector<economic_world_bank_correspondence> projections;
	std::vector<economic_world_bank_comparison> banks;
	std::vector<economic_world_sql_money_correspondence> sql_money;
	std::array<uint64_t, static_cast<size_t>(economic_world_money_issue::count)>
		issue_counts = {};
	uint64_t diagnostic_count = 0;
	bool diagnostics_truncated = false;
	std::vector<economic_world_money_diagnostic> diagnostics;
};
// Pure primary PC wallet/bank counterpart comparison. Caller retains the
// genuine initialized-world cut and same-cut raw SQL; DTO framing is not source
// authority. Original source2 normalization authenticates selected field order,
// canonical numeric bytes and hashes. Existing gameplay authority ASCII-lower
// account locators ([a-z0-9_-], original name limit) and exact nonnegative
// signed-TINYINT racewar identify shared banks; every raw name stays retained.
// Switched originals may use ONLY the unique captured
// descriptor's account. Disconnected/unobserved identities remain findings.
// Observed revision zero is valid; missing/UINT64_MAX is unavailable. Every PC
// needs one observed bank projection. Every persisted wallet/bank remains
// indexed, including unloaded holdings; repeated shared projections compare
// once and are NEVER added. No NPC opening mapping, item/full-world coverage,
// independent audit, SQL, mutation, native callback or activation authority.
// Original aggregate source ceilings apply to world plus this raw SQL packet;
// the full caller must additionally charge its other providers. Stored detail
// is 1..512, independent of complete issue counts. Structural/limit/allocation
// failure leaves output unchanged; semantic mismatches publish explicit issues.
economic_accounting_error economic_initialized_world_compare_pc_money(
	const economic_initialized_world_snapshot &, const economic_sql_source_snapshot &,
	const economic_sql_source_limits &, size_t diagnostic_limit,
	economic_initialized_world_money_correspondence *) noexcept;

// Additional current addressed/reset-born NPC comparison; PC declarations above
// remain unchanged. Lifetimes are values borrowed from the original authenticated
// owner, never proofs manufactured by this pure consumer.
#include "persistence/economic_sql_native_mobile_birth_transaction.h"
enum class economic_world_native_money_issue : uint8_t
{
	unresolved_first_opening,
	invalid_native_reference,
	missing_cash_reference,
	mixed_native_reference,
	unknown_current_cash,
	negative_balance,
	accounting_overflow,
	unavailable_cash_revision,
	invalid_raw_row,
	invalid_current_image,
	raw_image_mismatch,
	duplicate_catalog_instance,
	duplicate_observed_instance,
	duplicate_lifetime_instance,
	duplicate_mapping_identity,
	invalid_lifetime,
	mixed_lifetime_lineage,
	missing_catalog_counterpart,
	missing_authenticated_lifetime,
	retired_current_counterpart,
	reference_mismatch,
	namespace_mismatch,
	birth_operation_mismatch,
	current_revision_mismatch,
	current_balance_mismatch,
	unobserved_or_mismatched_prototype,
	count
};
struct economic_world_native_money_body
{
	size_t body_index = SIZE_MAX;
	std::optional<size_t> catalog_index, lifetime_index;
	uint32_t issues = 0;
	bool exact_current_correspondence = false;
};
struct economic_world_native_money_catalog
{
	size_t catalog_index = SIZE_MAX, observed_body_count = 0;
	std::optional<size_t> lifetime_index;
	uint32_t issues = 0;
	// Retired decoded rows remain history and cannot satisfy a loaded body.
	bool retired_history = false, exact_current_lifetime_correspondence = false;
};
struct economic_world_native_money_lifetime
{
	size_t lifetime_index = SIZE_MAX, observed_body_count = 0;
	std::optional<size_t> catalog_index;
	uint32_t issues = 0;
	// Count zero retains an unloaded authenticated lifetime, not global absence.
	bool exact_current_catalog_correspondence = false;
};
struct economic_world_native_money_diagnostic
{
	economic_world_native_money_issue issue = {};
	size_t body_index = SIZE_MAX, catalog_index = SIZE_MAX, lifetime_index = SIZE_MAX;
};
struct economic_initialized_world_native_money_correspondence
{
	std::vector<economic_world_native_money_body> bodies;
	std::vector<economic_world_native_money_catalog> catalog;
	std::vector<economic_world_native_money_lifetime> lifetimes;
	std::array<uint64_t, static_cast<size_t>(economic_world_native_money_issue::count)>
		issue_counts = {};
	uint64_t diagnostic_count = 0;
	bool diagnostics_truncated = false;
	std::vector<economic_world_native_money_diagnostic> diagnostics;
};
// Pure primary current-money correspondence. Reparse all raw6 cells/images using
// the original canonical image/reference codecs; cached decoded catalog fields
// are not proof. Full148-byte body/reference/cash-reference/image must agree.
// Separately join native UID, mapping authority ID, lineage, historical birth
// epoch, creating operation, current cash revision and all four denominations.
// Caller owns genuine cut/catalog/lifetimes and independently authenticated
// historical inbox/plan/result/source/outbox origin BEFORE current locks. This
// report supplies no SQL origin, authority, mutation, issuance or Plan5 audit.
// Historical birth cash never becomes current cash. Originless/unobserved NPCs
// are explicit unresolved first-opening observations, not adopted zero wallets.
// Every raw row/NPC/lifetime remains indexed, including retired history/unloaded
// lifetimes. Existing aggregate ceilings and 1..512 detail cap are unchanged;
// catalog counters already charge physical+room once. Semantic findings preserve
// complete counts; structural/limit/allocation errors leave output unchanged.
economic_accounting_error economic_initialized_world_compare_addressed_npc_money(
	const economic_initialized_world_snapshot &, const quest_mobile_native_sql_catalog &,
	std::span<const economic_sql_native_mobile_wallet_lifetime>,
	const economic_sql_source_limits &, size_t diagnostic_limit,
	economic_initialized_world_native_money_correspondence *) noexcept;

#endif

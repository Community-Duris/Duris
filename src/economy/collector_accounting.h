#ifndef DURIS_COLLECTOR_ACCOUNTING_H
#define DURIS_COLLECTOR_ACCOUNTING_H

#include "economy/collector_command.h"
#include "economy/economic_accounting_intent.h"

// Inactive typed capability for the existing single-item buyback transaction.
constexpr uint32_t ECONOMIC_WRITER_COLLECTOR_PURCHASE = 7;
constexpr uint32_t ECONOMIC_WRITER_COLLECTOR_HELD = 8;
constexpr uint64_t ECONOMIC_COLLECTOR_PURCHASE_SINK_ID = 24;

struct collector_purchase_accounting_authority
{
	critical_operation_id epoch = {};
	economic_account_key wallet_account = {};
	economic_account_key bank_account = {};
	currency_command_result balances_before = {};
	collector::record listing_before = {};
	economic_item_snapshot item_before = {};
	uint64_t catalog_revision_before = 0;
	uint64_t from_owner_revision_before = 0;
	uint64_t to_owner_revision_before = 0;
};

// Freezes the exact listing (including its death-operation source and price)
// and the two durable money lifetime mappings before admission.
economic_accounting_error collector_purchase_accounting_intent(const critical_command &command,
							       const critical_operation_id &epoch,
							       const economic_account_key &wallet,
							       const economic_account_key &bank,
							       const collector::record &listing,
							       std::vector<uint8_t> *encoded);

// Decode a schema-2 purchase only when its original listing and mapping IDs
// reproduce the exact admitted intent. Outputs are unchanged on refusal.
economic_accounting_error collector_purchase_accounting_decode(const critical_command &command,
							       economic_frozen_intent *intent,
							       collector_command_payload *payload,
							       collector::record *listing,
							       economic_account_key *wallet,
							       economic_account_key *bank);

// Pure comparison against evidence obtained under the repository's locks.
// The caller must use this plan and result in the same native transaction;
// this function neither acquires locks nor writes accounting rows.
economic_accounting_error collector_purchase_accounting_plan(
	const critical_command &command, const economic_frozen_intent &intent,
	const collector_purchase_accounting_authority &authority,
	const collector_command_result &result, economic_accounting_plan *plan);

struct collector_held_accounting_authority
{
	critical_operation_id lineage = {};
	critical_operation_id epoch = {};
	collector::record listing_before = {};
	economic_item_snapshot item_before = {};
	uint64_t catalog_revision_before = 0;
	uint64_t from_owner_revision_before = 0;
	uint64_t to_owner_revision_before = 0;
};

// A collected/available singleton can expire into destruction or be cancelled
// into destruction/quarantine. Collection from a source tree has a separate
// topology and is deliberately outside this capability.
economic_accounting_error collector_held_accounting_intent(const critical_command &command,
							   const critical_operation_id &lineage,
							   const critical_operation_id &epoch,
							   const collector::record &listing,
							   std::vector<uint8_t> *encoded);
// Reconstruct an admitted held-item transition from its frozen native listing.
// Outputs are unchanged when the command or intent is refused.
economic_accounting_error collector_held_accounting_decode(const critical_command &command,
							   economic_frozen_intent *intent,
							   collector_command_payload *payload,
							   collector::record *listing);
economic_accounting_error collector_held_accounting_plan(
	const critical_command &command, const economic_frozen_intent &intent,
	const collector_held_accounting_authority &authority,
	const collector_command_result &result, economic_accounting_plan *plan);

// Complete original listing/source/mapping/canonical-intent reconstruction;
// caller owns old outputs and authentic command. Strong outputs on refusal.
economic_accounting_error collector_purchase_accounting_intent_bounded(
	const critical_command &, const critical_operation_id &, const economic_account_key &,
	const economic_account_key &, const collector::record &, std::vector<uint8_t> *,
	bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
economic_accounting_error collector_purchase_accounting_decode_bounded(
	const critical_command &, economic_frozen_intent *, collector_command_payload *,
	collector::record *, economic_account_key *, economic_account_key *,
	bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
size_t collector_purchase_accounting_replay_observer_frame_bytes() noexcept;
bool collector_purchase_accounting_replay_heap_bytes(const economic_frozen_intent &,
						     size_t *) noexcept;

// Complete original purchase proof and canonical-intent reconstruction using
// the genuine fixed binding proof. Caller owns command/prior outputs/siblings.
// Strong output on refusal; prior original/bounded methods stay exact. These
// additive methods remain unselected; full source profiles are a separate join.
economic_accounting_error collector_purchase_accounting_intent_fixed_bounded(
	const critical_command &, const critical_operation_id &, const economic_account_key &,
	const economic_account_key &, const collector::record &, std::vector<uint8_t> *,
	bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
economic_accounting_error collector_purchase_accounting_decode_fixed_bounded(
	const critical_command &, economic_frozen_intent *, collector_command_payload *,
	collector::record *, economic_account_key *, economic_account_key *,
	bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;

// Pure complete route SOURCE/descendant-entry and own actual initial budget
// profiles. Unsupported/null/overflow leaves output unchanged. Inputs/prior
// outputs remain parent-owned; native/emitted/external qualification is separate.
bool collector_purchase_accounting_intent_fixed_source_frame_bytes(size_t *) noexcept;
bool collector_purchase_accounting_intent_fixed_initial_inline_bytes(size_t *) noexcept;
bool collector_purchase_accounting_decode_fixed_source_frame_bytes(size_t *) noexcept;
bool collector_purchase_accounting_decode_fixed_initial_inline_bytes(size_t *) noexcept;
constexpr size_t collector_purchase_accounting_fixed_source_query_frame_bytes() noexcept
{
	// The decode getter calls its genuine intent getter once: two payload query
	// chains, then decode/proof/freeze chains; actual public getter locals,
	// policy results, checked-add formals and each initial getter(P+2B).
	return 2 * collector_command_decode_payload_source_query_frame_bytes() +
	       economic_intent_decode_source_query_frame_bytes() +
	       economic_intent_verify_binding_fixed_source_query_frame_bytes() +
	       economic_intent_freeze_fixed_source_query_frame_bytes() + 10 * sizeof(void *) +
	       14 * sizeof(size_t) + 16 * sizeof(bool);
}

#endif

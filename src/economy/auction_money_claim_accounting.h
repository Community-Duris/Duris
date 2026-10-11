#ifndef DURIS_AUCTION_MONEY_CLAIM_ACCOUNTING_H
#define DURIS_AUCTION_MONEY_CLAIM_ACCOUNTING_H

#include "economy/auction_command.h"
#include "economy/economic_accounting_intent.h"

constexpr uint32_t ECONOMIC_WRITER_AUCTION_MONEY_CLAIM = 13;
constexpr size_t ECONOMIC_AUCTION_CLAIM_MAX_SOURCES = 4096;

struct auction_money_claim_source
{
	critical_operation_id operation = {};
	uint16_t slot = 0;
	uint32_t beneficiary_pid = 0;
	uint64_t claim_mapping_id = 0;
	uint64_t amount = 0;
};

struct auction_money_claim_state
{
	uint32_t beneficiary_pid = 0;
	int64_t money = 0;
	uint64_t revision = 0;
	std::vector<auction_money_claim_source> sources;
};

struct auction_money_claim_authority
{
	critical_operation_id epoch = {};
	economic_account_key wallet = {};
	economic_account_key bank = {};
	economic_account_key claim_account = {};
	currency_command_result balances_before = {};
	auction_money_claim_state claim = {};
};

// Freeze the exact pending source set by an ordered, domain-separated digest.
// Every source row remains individually linked to the successful claim root.
economic_accounting_error auction_money_claim_accounting_intent(
	const critical_command &command, const critical_operation_id &epoch,
	const economic_account_key &wallet, const economic_account_key &bank,
	const economic_account_key &claim_account, const auction_money_claim_state &claim,
	std::vector<uint8_t> *encoded);

// Decode frozen account identities. The source digest must be recomputed from
// the locked native claim and source rows before any mutation.
economic_accounting_error auction_money_claim_accounting_decode(
	const critical_command &command, economic_frozen_intent *intent,
	auction_command_payload *payload, economic_account_key *wallet, economic_account_key *bank,
	economic_account_key *claim_account);

economic_accounting_error auction_money_claim_accounting_plan(
	const critical_command &command, const economic_frozen_intent &intent,
	const auction_money_claim_authority &authority, const auction_command_result &result,
	economic_accounting_plan *plan);

// Genuine complete original auction replay companions. Caller owns inputs,
// prior outputs and sibling retained state in outer_live; callback retains the
// admitted simultaneous peak. Semantic laws and strong outputs remain original.
// No writer/source/execution authority; native qualification remains separate.
economic_accounting_error auction_money_claim_accounting_decode_bounded(
	const critical_command &command, economic_frozen_intent *intent,
	auction_command_payload *payload, economic_account_key *wallet, economic_account_key *bank,
	economic_account_key *claim_account, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept;

// Additive full original money-claim algorithm with fixed binding proof.
// Own SOURCE candidate; full command/native dependency qualification remains open.
bool auction_money_claim_accounting_decode_own_source_frame_bytes(size_t *) noexcept;
bool auction_money_claim_accounting_decode_initial_inline_bytes(size_t *) noexcept;
constexpr size_t auction_money_claim_accounting_decode_own_source_query_frame_bytes() noexcept
{
	// Actual output/result and policy bool, critical valid getter returned N.
	return sizeof(size_t *) + sizeof(size_t) + 2 * sizeof(bool);
}
economic_accounting_error auction_money_claim_accounting_decode_fixed_bounded(
	const critical_command &command, economic_frozen_intent *intent,
	auction_command_payload *payload, economic_account_key *wallet, economic_account_key *bank,
	economic_account_key *claim_account, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept;
bool auction_money_claim_accounting_decode_source_frame_bytes(size_t *) noexcept;
bool auction_money_claim_accounting_decode_source_supplement_frame_bytes(size_t *) noexcept;
constexpr size_t auction_money_claim_accounting_decode_source_query_frame_bytes() noexcept
{
	// Complete real pure getter graph: own and three authentic child getters,
	// output/result, five scalar locals, two actual max comparisons,
	// and one genuine checked addition.
	return auction_money_claim_accounting_decode_own_source_query_frame_bytes() +
	       auction_command_decode_payload_source_query_frame_bytes() +
	       economic_intent_decode_source_query_frame_bytes() +
	       economic_intent_verify_binding_fixed_source_query_frame_bytes() +
	       2 * sizeof(void *) + 6 * sizeof(size_t) + 4 * sizeof(bool);
}

#endif

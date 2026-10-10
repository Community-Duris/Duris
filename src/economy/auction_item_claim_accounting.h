#ifndef DURIS_AUCTION_ITEM_CLAIM_ACCOUNTING_H
#define DURIS_AUCTION_ITEM_CLAIM_ACCOUNTING_H

#include "economy/auction_command.h"
#include "economy/auction_listing_accounting.h"
#include "player/player_snapshot_codec.h"
#include "economy/economic_accounting_intent.h"

constexpr uint32_t ECONOMIC_WRITER_AUCTION_ITEM_CLAIM = 10;
economic_accounting_error
auction_item_claim_accounting_observe_native_facts(const economic_frozen_intent &,
						   auction_accounting_native_facts *) noexcept;

struct auction_item_claim_row
{
	uint64_t uid = 0;
	uint64_t revision = 0;
	uint16_t slot = 0;
	int32_t vnum = 0;
	uint32_t claim_pid = 0;
	bool claimed = false;
};

struct auction_item_claim_state
{
	uint32_t auction_id = 0;
	uint32_t seller_pid = 0;
	uint32_t winner_pid = 0;
	uint32_t claimant_pid = 0;
	uint32_t status = 0;
	uint32_t custody_state = 0;
	uint64_t auction_revision = 0;
	critical_operation_id listing_operation = {};
	critical_operation_id claim_source_operation = {};
	uint16_t item_count = 0;
	std::array<auction_item_claim_row, AUCTION_COMMAND_MAX_ITEMS> rows = {};
};

struct auction_item_claim_accounting_authority
{
	critical_operation_id epoch = {};
	economic_account_key wallet_account = {};
	economic_account_key bank_account = {};
	auction_item_claim_state claim = {};
	currency_command_result balances_before = {};
	uint64_t player_owner_revision_before = 0;
	uint64_t auction_owner_revision_before = 0;
	std::vector<economic_item_snapshot> items_before;
	// Genuine caller-owned acknowledged/source literals, never authority alone.
	std::vector<player_item_snapshot> native_selected_literals;
};

// Freeze the exact staged claim, original UIDs, and money-lifetime identities.
economic_accounting_error auction_item_claim_accounting_intent(
	const critical_command &command, const critical_operation_id &epoch,
	const economic_account_key &wallet, const economic_account_key &bank,
	const auction_item_claim_state &claim, std::vector<uint8_t> *encoded);

// Recover the exact admitted claim and money lifetime IDs from schema 2.
// Outputs remain unchanged when its binding or frozen facts are invalid.
economic_accounting_error auction_item_claim_accounting_decode(const critical_command &command,
							       economic_frozen_intent *intent,
							       auction_command_payload *payload,
							       auction_item_claim_state *claim,
							       economic_account_key *wallet,
							       economic_account_key *bank);

// Pure comparison to repository-locked claim and custody authority. Item
// events use the same order and revisions as native item_ownership_ledger.
economic_accounting_error auction_item_claim_accounting_plan(
	const critical_command &command, const economic_frozen_intent &intent,
	const auction_item_claim_accounting_authority &authority,
	const auction_command_result &result, economic_accounting_plan *plan);

// Genuine complete original auction replay companions. Caller owns inputs,
// prior outputs and sibling retained state in outer_live; callback retains the
// admitted simultaneous peak. Semantic laws and strong outputs remain original.
// No writer/source/execution authority; native qualification remains separate.
economic_accounting_error auction_item_claim_accounting_intent_bounded(
	const critical_command &command, const critical_operation_id &epoch,
	const economic_account_key &wallet, const economic_account_key &bank,
	const auction_item_claim_state &claim, std::vector<uint8_t> *encoded,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept;

economic_accounting_error auction_item_claim_accounting_decode_bounded(
	const critical_command &command, economic_frozen_intent *intent,
	auction_command_payload *payload, auction_item_claim_state *claim,
	economic_account_key *wallet, economic_account_key *bank,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept;

#endif

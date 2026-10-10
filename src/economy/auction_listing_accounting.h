#ifndef DURIS_AUCTION_LISTING_ACCOUNTING_H
#define DURIS_AUCTION_LISTING_ACCOUNTING_H

#include "economy/auction_command.h"
#include "player/player_snapshot_codec.h"
#include "economy/economic_accounting_intent.h"

constexpr uint32_t ECONOMIC_WRITER_AUCTION_LISTING = 12;

// Observations recovered from original EAI facts; no command/header/SQL authority.
struct auction_accounting_native_facts
{
	uint32_t original_level = 0;
	uint64_t acknowledged_save_revision = 0;
	std::array<uint8_t, 32> before_digest{}, after_digest{}, selected_digest{};
	uint32_t selected_node_count = 0;
	uint16_t selected_root_count = 0;
};
economic_accounting_error
auction_listing_accounting_observe_native_facts(const economic_frozen_intent &,
						auction_accounting_native_facts *) noexcept;

struct auction_listing_accounting_authority
{
	critical_operation_id epoch = {};
	economic_account_key wallet = {};
	economic_account_key bank = {};
	economic_account_key escrow = {};
	currency_command_result balances_before = {};
	uint64_t player_owner_revision_before = 0;
	std::vector<economic_item_snapshot> items_before;
	// Genuine caller-owned acknowledged/source literals, never authority alone.
	std::vector<player_item_snapshot> native_selected_literals;
};

// Admission freezes the existing money lifetimes. The new auction ID and its
// escrow mapping are assigned only after the native listing insert, in the same
// transaction, and are verified by the plan before commit.
economic_accounting_error auction_listing_accounting_intent(const critical_command &command,
							    const critical_operation_id &epoch,
							    const economic_account_key &wallet,
							    const economic_account_key &bank,
							    std::vector<uint8_t> *encoded);

// Decode an admitted listing and verify its complete canonical frozen intent.
// Outputs are unchanged on failure.
economic_accounting_error auction_listing_accounting_decode(const critical_command &command,
							    economic_frozen_intent *intent,
							    auction_command_payload *payload,
							    economic_account_key *wallet,
							    economic_account_key *bank);

economic_accounting_error auction_listing_accounting_plan(
	const critical_command &command, const economic_frozen_intent &intent,
	const auction_listing_accounting_authority &authority, const auction_command_result &result,
	economic_accounting_plan *plan);

// Genuine complete original auction replay companions. Caller owns inputs,
// prior outputs and sibling retained state in outer_live; callback retains the
// admitted simultaneous peak. Semantic laws and strong outputs remain original.
// No writer/source/execution authority; native qualification remains separate.
economic_accounting_error auction_listing_accounting_intent_bounded(
	const critical_command &command, const critical_operation_id &epoch,
	const economic_account_key &wallet, const economic_account_key &bank,
	std::vector<uint8_t> *encoded, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live) noexcept;

economic_accounting_error auction_listing_accounting_decode_bounded(
	const critical_command &command, economic_frozen_intent *intent,
	auction_command_payload *payload, economic_account_key *wallet, economic_account_key *bank,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept;

#endif

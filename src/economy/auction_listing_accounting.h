#ifndef DURIS_AUCTION_LISTING_ACCOUNTING_H
#define DURIS_AUCTION_LISTING_ACCOUNTING_H

#include "economy/auction_command.h"
#include "economy/economic_accounting_intent.h"

constexpr uint32_t ECONOMIC_WRITER_AUCTION_LISTING = 12;

struct auction_listing_accounting_authority
{
	critical_operation_id epoch = {};
	economic_account_key wallet = {};
	economic_account_key bank = {};
	economic_account_key escrow = {};
	currency_command_result balances_before = {};
	uint64_t player_owner_revision_before = 0;
	std::vector<economic_item_snapshot> items_before;
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

#endif

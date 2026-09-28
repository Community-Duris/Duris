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

#endif

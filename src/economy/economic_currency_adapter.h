#ifndef DURIS_ECONOMIC_CURRENCY_ADAPTER_H
#define DURIS_ECONOMIC_CURRENCY_ADAPTER_H

#include "economy/currency_command.h"
#include "economy/economic_accounting_intent.h"

// Stable typed writer capabilities; generic metadata numbers grant no capability.
constexpr uint32_t ECONOMIC_WRITER_BANK_DEPOSIT = 1;
constexpr uint32_t ECONOMIC_WRITER_BANK_WITHDRAW = 2;
constexpr size_t ECONOMIC_BANK_FACT_BYTES = 24;

// Populated from the repository's locked authority and retained lifetime mapping.
// The pure adapter cannot establish that a caller actually acquired those locks.
struct economic_currency_authority
{
	critical_operation_id epoch = {};
	economic_account_key wallet_account;
	economic_account_key bank_account;
	critical_entity_key player_fence = {};
	critical_entity_key bank_fence = {};
	currency_command_result state = {};
};

class economic_prepared_currency
{
    public:
	const currency_prepared_mutation &mutation() const { return mutation_; }
	const economic_accounting_plan &plan() const { return plan_; }
	// Compares every canonical field/leg/witness; extra cancelling legs fail.
	economic_accounting_error agrees_with(const economic_accounting_plan &candidate) const;

    private:
	currency_prepared_mutation mutation_;
	economic_accounting_plan plan_;
	std::vector<uint8_t> encoded_;
	economic_prepared_currency(currency_prepared_mutation mutation,
				   economic_accounting_plan plan, std::vector<uint8_t> encoded);
	friend economic_accounting_error
	economic_bank_transfer_prepare(const critical_command &, const economic_frozen_intent &,
				       const economic_currency_authority &,
				       currency_revision_policy,
				       std::optional<economic_prepared_currency> *);
};

// Selects reason/writer/actor internally and freezes both lifetime mappings.
// This pure builder does not acquire authority or authorize a source event.
economic_accounting_error economic_bank_transfer_intent(const critical_command &command,
							const critical_operation_id &epoch,
							const economic_account_key &wallet,
							const economic_account_key &bank,
							std::vector<uint8_t> *encoded);

// Only ATM deposit/withdrawal: no issuance, sink, refund or generic counterparty
// flags. The same prepared after-state must be used for domain storage.
economic_accounting_error economic_bank_transfer_prepare(
	const critical_command &command, const economic_frozen_intent &intent,
	const economic_currency_authority &authority, currency_revision_policy revision_policy,
	std::optional<economic_prepared_currency> *prepared);

#endif

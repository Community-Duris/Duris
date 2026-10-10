#ifndef DURIS_ECONOMIC_CURRENCY_ADAPTER_H
#define DURIS_ECONOMIC_CURRENCY_ADAPTER_H

#include "economy/currency_command.h"
#include "economy/economic_accounting_intent.h"

// Stable typed writer capabilities; generic metadata numbers grant no capability.
constexpr uint32_t ECONOMIC_WRITER_BANK_DEPOSIT = 1;
constexpr uint32_t ECONOMIC_WRITER_BANK_WITHDRAW = 2;
constexpr uint32_t ECONOMIC_WRITER_CHAOS_STARTER_BANK = 3;
constexpr uint32_t ECONOMIC_WRITER_QUEST_WALLET_REWARD = 4;
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
	friend economic_accounting_error economic_chaos_starter_bank_prepare(
		const critical_command &, const economic_frozen_intent &,
		const economic_currency_authority &, currency_revision_policy,
		std::optional<economic_prepared_currency> *);
	friend economic_accounting_error economic_quest_wallet_reward_prepare(
		const critical_command &, const economic_frozen_intent &,
		const economic_currency_authority &, currency_revision_policy,
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

// Inactive typed component for the fixed Chaos starter bank grant. Its logical
// source is derived from the player PID, independent of this operation's epoch.
// Backend owners must prove the persisted pending grant and claim the source in
// the same native/accounting commit before admitting gameplay.
economic_accounting_error economic_chaos_starter_bank_intent(const critical_command &command,
							     const critical_operation_id &epoch,
							     const economic_account_key &wallet,
							     const economic_account_key &bank,
							     std::vector<uint8_t> *encoded);
economic_accounting_error economic_chaos_starter_bank_prepare(
	const critical_command &command, const economic_frozen_intent &intent,
	const economic_currency_authority &authority, currency_revision_policy revision_policy,
	std::optional<economic_prepared_currency> *prepared);

// Wallet-only quest reward issuance. The deterministic child operation ID is
// also the source identity for retries of the consumed offering's reward.
economic_accounting_error economic_quest_wallet_reward_intent(const critical_command &command,
							      const critical_operation_id &epoch,
							      const economic_account_key &wallet,
							      const economic_account_key &bank,
							      std::vector<uint8_t> *encoded);
economic_accounting_error economic_quest_wallet_reward_prepare(
	const critical_command &command, const economic_frozen_intent &intent,
	const economic_currency_authority &authority, currency_revision_policy revision_policy,
	std::optional<economic_prepared_currency> *prepared);

// Full original typed bank-intent builders with bounded fixed-facts freeze.
// Authentic inputs/prior encoded output belong to the caller outer. Complete
// SOURCE/entry profiles are transient before entry; actual local source/facts
// live in the child, with only named uncovered lower supplements retained.
economic_accounting_error
economic_bank_transfer_intent_bounded(const critical_command &, const critical_operation_id &,
				      const economic_account_key &, const economic_account_key &,
				      std::vector<uint8_t> *, bool (*)(size_t, void *) noexcept,
				      void *, size_t outer_live) noexcept;
economic_accounting_error economic_chaos_starter_bank_intent_bounded(
	const critical_command &, const critical_operation_id &, const economic_account_key &,
	const economic_account_key &, std::vector<uint8_t> *, bool (*)(size_t, void *) noexcept,
	void *, size_t outer_live) noexcept;
economic_accounting_error economic_quest_wallet_reward_intent_bounded(
	const critical_command &, const critical_operation_id &, const economic_account_key &,
	const economic_account_key &, std::vector<uint8_t> *, bool (*)(size_t, void *) noexcept,
	void *, size_t outer_live) noexcept;
bool economic_currency_intent_source_frame_bytes(size_t *) noexcept;
bool economic_currency_intent_initial_inline_bytes(size_t *) noexcept;
constexpr size_t economic_currency_intent_source_query_frame_bytes() noexcept
{
	// Complete getter composes three genuine child SOURCE queries; own fixed
	// return/policy/out/local/checked-add carriers and entry getter are explicit.
	// Its constexpr accessor N is separately admitted by the selecting parent.
	return currency_command_decode_payload_source_query_frame_bytes() +
	       economic_intent_freeze_fixed_source_query_frame_bytes() +
	       critical_operation_id_derive_source_query_frame_bytes() + 6 * sizeof(void *) +
	       8 * sizeof(size_t) + 7 * sizeof(bool);
}

#endif

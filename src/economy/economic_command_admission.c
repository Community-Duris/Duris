#include "economy/economic_command_admission.h"
#include "economy/auction_repository.h"
#include "economy/auction_native_command_context.h"
#include "economy/native_mobile_birth_command.h"
#include "economy/economic_currency_adapter.h"
#include "economy/coin_transfer_accounting.h"
#include "economy/item_transfer_accounting.h"
#include "item/native_quest_transport.h"
#include "economy/collector_accounting.h"

#include <new>

namespace
{
uint64_t little_u64(std::span<const uint8_t> input, size_t offset)
{
	uint64_t value = 0;
	for (size_t byte = 0; byte < 8; ++byte)
		value |= uint64_t(input[offset + byte]) << (8 * byte);
	return value;
}
}

bool economic_shop_trade_admission_available() noexcept
{
	// SHOP has no branch in the maintained schema-2 admission allowlist yet.
	// Change alongside that complete route, never as a producer-side override.
	return false;
}

bool economic_command_admission_supported(const critical_command &command) noexcept
{
	using error = economic_accounting_error;
	if (command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return false;
	try
	{
		if (command.type == critical_command_type::auction)
		{
#ifdef __NO_MYSQL__
			return false;
#else
			// Original immutable typed support; current SQL/world authority
			// and guarded publication remain with the real native owner.
			return command.publication_required &&
			       command.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION &&
			       auction_repository_frozen_accounting_valid(command);
#endif
		}
		// Immutable type support only. Original source admission and guarded
		// native publication/ACK remain the private birth owner's obligations.
		if (command.type == critical_command_type::native_mobile_birth)
		{
			quest_mobile_native_image original;
			return native_mobile_birth_command_decode(command, &original) == error::ok;
		}
		economic_frozen_intent intent;
		if (economic_intent_decode(command.accounting_intent, &intent) != error::ok)
			return false;
		const auto &meta = intent.admission.metadata;
		const auto facts = std::span<const uint8_t>(intent.admission.facts);
		// This projection is used only to regenerate the original immutable
		// intent. Execution always receives the complete schema-2 command.
		auto admission = command;
		admission.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		admission.accounting_intent.clear();
		admission.publication_required = false;
		std::vector<uint8_t> expected;
		if (command.type == critical_command_type::account_bank)
		{
			if (facts.size() != ECONOMIC_BANK_FACT_BYTES)
				return false;
			const economic_account_key wallet = { meta.lineage,
							      economic_account_kind::wallet,
							      little_u64(facts, 0), 0 };
			const economic_account_key bank = { meta.lineage,
							    economic_account_kind::bank,
							    little_u64(facts, 8),
							    little_u64(facts, 16) };
			currency_command_payload payload = {};
			if (!currency_command_decode_payload(admission, &payload))
				return false;
			const auto result =
				payload.reason == currency_reason_type::chaos_starter_reward ?
					economic_chaos_starter_bank_intent(
						admission, meta.epoch, wallet, bank, &expected) :
				payload.reason == currency_reason_type::wallet_reward ?
					economic_quest_wallet_reward_intent(
						admission, meta.epoch, wallet, bank, &expected) :
					economic_bank_transfer_intent(admission, meta.epoch, wallet,
								      bank, &expected);
			if (result != error::ok)
				return false;
		}
		else if (command.type == critical_command_type::coin_transfer)
		{
			if (facts.size() != ECONOMIC_COIN_TRANSFER_FACT_BYTES)
				return false;
			coin_transfer_payload payload = {};
			if (!coin_transfer_command_decode_payload(admission, &payload))
				return false;
			auto endpoint_kind = [](const coin_transfer_endpoint &endpoint)
			{
				if (endpoint.change.type == critical_command_type::account_bank)
					return economic_account_kind::wallet;
				if (endpoint.change.type == critical_command_type::item_transfer)
					return economic_account_kind::pile;
				return economic_account_kind{};
			};
			const economic_account_key source = { meta.lineage,
							      endpoint_kind(payload.source),
							      little_u64(facts, 0), 0 };
			const economic_account_key destination = {
				meta.lineage, endpoint_kind(payload.destination),
				little_u64(facts, 8), 0
			};
			if (coin_transfer_accounting_intent(admission, meta.epoch, source,
							    destination, &expected) != error::ok)
				return false;
		}
		else if (command.type == critical_command_type::item_transfer)
		{
			if (native_quest_transport_command(command))
			{
				// Immutable type support only. Rebuild the complete original intent;
				// live held ownership, birth/source and SQL authority remain mandatory.
				if (!command.publication_required ||
				    meta.writer_id != ECONOMIC_WRITER_ITEM_TRANSFER ||
				    meta.actor_kind != economic_actor_kind::domain ||
				    meta.actor_id > INT32_MAX)
					return false;
				admission.accepted_at_usec = 0;
				const auto *source = meta.source_event ? &*meta.source_event :
									 nullptr;
				return item_native_mobile_accounting_intent(
					       admission, meta.lineage, meta.epoch,
					       static_cast<uint32_t>(meta.actor_id), source,
					       &expected) == error::ok &&
				       expected == command.accounting_intent;
			}
			return item_transfer_accounting_command_supported(command);
		}
		else if (command.type == critical_command_type::collector)
		{
			collector_command_payload payload;
			collector::record listing;
			economic_account_key wallet, bank;
			return collector_purchase_accounting_decode(command, &intent, &payload,
								    &listing, &wallet,
								    &bank) == error::ok;
		}
		else
			return false;
		return expected == command.accounting_intent;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool economic_flatfile_command_admission_supported(const critical_command &command) noexcept
{
	// Native acknowledged mutation has no qualified flat-file publication owner.
	if (native_quest_transport_command(command))
		return false;
	if (command.type == critical_command_type::account_bank)
	{
		currency_command_payload payload = {};
		if (!currency_command_decode_payload(command, &payload) ||
		    (payload.reason != currency_reason_type::atm_deposit &&
		     payload.reason != currency_reason_type::atm_withdraw &&
		     payload.reason != currency_reason_type::chaos_starter_reward &&
		     !(payload.reason == currency_reason_type::wallet_reward &&
		       payload.reason_id > 0 &&
		       command.source_site == critical_source_site::recovery &&
		       command.deadline_class == critical_deadline_class::recovery)))
			return false;
	}
	return (command.type == critical_command_type::account_bank ||
		command.type == critical_command_type::item_transfer) &&
	       economic_command_admission_supported(command);
}

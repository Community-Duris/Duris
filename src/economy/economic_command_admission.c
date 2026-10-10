#include "economy/economic_command_admission.h"
#include "economy/auction_repository.h"
#include "economy/auction_native_command_context.h"
#include "economy/native_mobile_birth_command.h"
#include "economy/native_mobile_birth_cash_role_command.h"
#include "economy/zone_reset_item_command.h"
#include "economy/economic_currency_adapter.h"
#include "economy/coin_transfer_accounting.h"
#include "economy/item_transfer_accounting.h"
#include "item/native_quest_transport.h"
#include "economy/collector_accounting.h"
#include "economy/shop_trade_accounting.h"

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
			if (command.payload_version ==
			    NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION)
			{
				std::vector<native_mobile_birth_item_recipe> recipes;
				native_mobile_birth_cash_role_recipe role;
				// Complete original NMB4 binding and ordinary role only. Shared
				// birth has no complete atomic owner and stays unregistered.
				return native_mobile_birth_cash_role_command_decode(
					       command, &original, &recipes, &role) == error::ok &&
				       role.role == native_mobile_birth_cash_role::ordinary_wallet;
			}
			return native_mobile_birth_command_decode(command, &original) == error::ok;
		}
		if (command.type == critical_command_type::zone_reset_item_birth)
		{
#ifdef __NO_MYSQL__
			return false;
#else
			// Immutable SQL type support only. Actual reset source admission,
			// current room/season and native publication stay with their owners.
			zone_reset_item_image original;
			return zone_reset_item_command_decode(command, &original) == error::ok;
#endif
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
#ifdef __NO_MYSQL__
	if (command.type == critical_command_type::shop_trade)
	{
		try
		{
			economic_frozen_intent intent;
			shop_trade_payload payload{};
			economic_account_key wallet, bank, counterparty;
			// Genuine client-free publication retains full v8 forests. Older
			// flat SHOP commands have no admitted native recovery owner.
			return command.publication_required &&
			       command.payload_version == SHOP_TRADE_RECOVERY_PAYLOAD_VERSION &&
			       shop_trade_accounting_decode(command, &intent, &payload, &wallet,
							    &bank, &counterparty) ==
				       economic_accounting_error::ok;
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
	}
	if (command.type == critical_command_type::collector)
	{
		try
		{
			economic_frozen_intent intent;
			collector_command_payload payload{};
			collector::record listing;
			economic_account_key wallet, bank;
			return command.publication_required &&
			       command.schema_version ==
				       CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
			       critical_command_envelope_valid(command) &&
			       collector_purchase_accounting_decode(command, &intent, &payload,
								    &listing, &wallet, &bank) ==
				       economic_accounting_error::ok &&
			       payload.action == collector_action::purchase &&
			       payload.item_count == 1 && payload.actor_pid &&
			       payload.actor_pid <= INT32_MAX && payload.selected_item_uid;
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
	}
#endif
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

bool economic_room_command_admission_supported_bounded(const critical_command &command,
						       bool (*reserve)(size_t, void *) noexcept,
						       void *context, size_t outer) noexcept
{
	// Complete projection of the original SQL ROOM branch only. Other command
	// families are deliberately outside this bounded provider's scope.
	if (!reserve || command.type != critical_command_type::zone_reset_item_birth ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return false;
#ifdef __NO_MYSQL__
	(void)context;
	(void)outer;
	// Exact original SQL ROOM support refusal in the flat build.
	return false;
#else
	if (outer > SIZE_MAX - sizeof(zone_reset_item_image) ||
	    !reserve(outer + sizeof(zone_reset_item_image), context))
		return false;
	zone_reset_item_image original;
	// Owning full decoder retains the original publication/payload/literal,
	// recipe/source/intent binding and complete canonical command comparison.
	return zone_reset_item_command_decode_bounded(command, &original, reserve, context,
						      outer + sizeof(zone_reset_item_image)) ==
	       economic_accounting_error::ok;
#endif
}

bool economic_flatfile_room_command_admission_supported_bounded(
	const critical_command &command, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer) noexcept
{
	if (!reserve || command.type != critical_command_type::zone_reset_item_birth)
		return false;
	// The original flat callback's SHOP/collector/account-bank/native-quest
	// branches do not match ROOM. Its decisive final bank/item allowlist remains
	// unchanged; type22 never reaches a decoder or becomes admitted here.
	return (command.type == critical_command_type::account_bank ||
		command.type == critical_command_type::item_transfer) &&
	       economic_room_command_admission_supported_bounded(command, reserve, context, outer);
}

bool economic_ordinary_native_command_admission_supported_bounded(
	const critical_command &command, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live) noexcept
{
	// Complete original NMB4 ordinary branch, without changing registration.
	// The original selected flat allowlist still decides whether this route is
	// available. A codec proof alone cannot grant submission or activation.
	if (!reserve || command.type != critical_command_type::native_mobile_birth ||
	    command.payload_version != NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return false;
	struct admission_workspace
	{
		quest_mobile_native_image original;
		std::vector<native_mobile_birth_item_recipe> recipes;
		native_mobile_birth_cash_role_recipe role;
	};
	// Charge the genuine simultaneous outputs before constructing them. Caller
	// owns command/context/prior outputs. The full owning decoder admits its
	// real transient allocations and retains all these outputs through return.
	constexpr size_t frame = sizeof(admission_workspace) + sizeof(const critical_command *) +
				 sizeof(reserve) + sizeof(context) + sizeof(outer_live) +
				 sizeof(size_t) + sizeof(bool) + sizeof(economic_accounting_error);
	if (frame > SIZE_MAX - outer_live || !reserve(outer_live + frame, context))
		return false;
	admission_workspace work;
	return native_mobile_birth_cash_role_command_decode_bounded(
		       command, &work.original, &work.recipes, &work.role, reserve, context,
		       outer_live + frame) == economic_accounting_error::ok &&
	       work.role.role == native_mobile_birth_cash_role::ordinary_wallet;
}

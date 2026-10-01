#include "economy/item_transfer_accounting.h"
#include "world/vnum.obj.h"
#include "player/player_snapshot_codec.h"
#include "core/structs.h"
#include "item/craft_pouch_mutation.h"

#include <cassert>
#include <utility>

using error = economic_accounting_error;
static_assert(static_cast<uint16_t>(economic_source_kind::item_action) == 18);

critical_operation_id id(uint8_t byte)
{
	critical_operation_id value = {};
	value.bytes[0] = byte;
	return value;
}

critical_command move(item_transfer_reason reason, item_owner_identity from, item_owner_identity to)
{
	item_transfer_payload payload = {};
	if (reason == item_transfer_reason::creation)
	{
		from = { item_owner_type::system, 0, 0 };
		to = { item_owner_type::player, 10, 0 };
	}
	else if (reason == item_transfer_reason::destruction)
		to = { item_owner_type::destruction, 0, 0 };
	else if (reason == item_transfer_reason::trusted_steal)
	{
		payload.reason_id = static_cast<int64_t>(from.id);
		to = { item_owner_type::player, 10, 0 };
	}
	else if (reason == item_transfer_reason::locker_deposit)
		to = { item_owner_type::locker, 20, 0 };
	else if (reason == item_transfer_reason::auction_list)
		to = { item_owner_type::auction, 20, 0 };
	else if (reason == item_transfer_reason::pet_give)
		payload.reason_id = static_cast<int64_t>(to.id);
	else if (reason == item_transfer_reason::pet_return)
		payload.reason_id = static_cast<int64_t>(from.id);
	payload.from_owner = from;
	payload.to_owner = to;
	payload.reason = reason;
	if (reason != item_transfer_reason::trusted_steal &&
	    reason != item_transfer_reason::pet_give && reason != item_transfer_reason::pet_return)
		payload.reason_id = 7;
	payload.expected_from_revision = 3;
	payload.expected_to_revision = 5;
	payload.selected_item_uid = 100;
	payload.target_root_item_uid = 100;
	payload.item_count = 1;
	const bool creation = reason == item_transfer_reason::creation;
	payload.items[0] = {
		100,  100,
		0,    creation ? ITEM_TRANSFER_ABSENT_REVISION : 2,
		9001, creation ? item_custody_state::absent : item_custody_state::active
	};
	critical_command command = {};
	assert(item_transfer_command_build(&command, id(3), payload, critical_source_site::command,
					   critical_deadline_class::interactive));
	return command;
}

critical_command corpse_move(item_transfer_reason reason, uint32_t actor_pid)
{
	constexpr uint32_t corpse_owner_pid = 10;
	constexpr uint32_t corpse_save_id = 7;
	const item_owner_identity corpse = { item_owner_type::corpse,
					     item_corpse_owner_id(corpse_owner_pid, corpse_save_id),
					     0 };
	item_transfer_payload payload = {};
	payload.from_owner =
		reason == item_transfer_reason::corpse_create ?
			item_owner_identity{ item_owner_type::player, corpse_owner_pid, 0 } :
			corpse;
	payload.to_owner = reason == item_transfer_reason::corpse_create ?
				   corpse :
				   item_owner_identity{ item_owner_type::player, actor_pid, 0 };
	payload.reason = reason;
	payload.reason_id = reason == item_transfer_reason::corpse_create ? corpse_save_id : 100;
	payload.expected_from_revision = 3;
	payload.expected_to_revision = 5;
	payload.multi_root = true;
	payload.item_count = 2;
	payload.items[0] = { 100, 100, 0, 2, 9001, item_custody_state::active };
	payload.items[1] = { 101, 101, 0, 3, 9002, item_custody_state::active };
	payload.corpse.present = true;
	payload.corpse.room_vnum = 50;
	payload.corpse.weight = 10;
	payload.corpse.values[3] = static_cast<int32_t>(corpse_owner_pid);
	payload.corpse.values[5] = 0;
	payload.corpse.values[6] = static_cast<int32_t>(corpse_save_id);
	payload.corpse.owner_name = "CorpseOwner";
	payload.corpse.short_description = "the corpse of CorpseOwner";
	payload.corpse.description = "The corpse lies here.";
	payload.corpse.keywords = "corpse body";
	critical_command command = {};
	assert(item_transfer_command_build(&command, id(4), payload, critical_source_site::command,
					   critical_deadline_class::interactive));
	return command;
}

void ordinary_moves_are_bound_to_actor_and_payload()
{
	const auto lineage = id(1);
	const auto epoch = id(2);
	for (const auto &command :
	     { move(item_transfer_reason::player_get, { item_owner_type::room, 50, 0 },
		    { item_owner_type::player, 10, 0 }),
	       move(item_transfer_reason::player_drop, { item_owner_type::player, 10, 0 },
		    { item_owner_type::room, 50, 0 }),
	       move(item_transfer_reason::player_give, { item_owner_type::player, 10, 0 },
		    { item_owner_type::player, 11, 0 }),
	       move(item_transfer_reason::trusted_steal, { item_owner_type::player, 11, 0 },
		    { item_owner_type::player, 10, 0 }) })
	{
		std::vector<uint8_t> encoded;
		assert(item_transfer_accounting_intent(command, lineage, epoch, 10, &encoded) ==
		       error::ok);
		critical_command admitted = command;
		admitted.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
		admitted.accounting_intent = encoded;
		assert(item_transfer_accounting_command_supported(admitted));
		economic_frozen_intent decoded;
		assert(economic_intent_decode(encoded, &decoded) == error::ok);
		assert(decoded.admission.metadata.lineage.bytes == lineage.bytes);
		assert(decoded.admission.metadata.epoch.bytes == epoch.bytes);
		assert(decoded.admission.metadata.writer_id == ECONOMIC_WRITER_ITEM_TRANSFER);
		assert(decoded.admission.metadata.reason == economic_reason::item_move);
		assert(decoded.admission.metadata.actor_id == 10);
	}
}

void nested_locker_and_pet_moves_bind_actor_to_custody()
{
	const auto lineage = id(1);
	const auto epoch = id(2);
	auto admit = [&](critical_command command)
	{
		std::vector<uint8_t> encoded;
		assert(item_transfer_accounting_intent(command, lineage, epoch, 10, &encoded) ==
		       error::ok);
		assert(item_transfer_accounting_intent(command, lineage, epoch, 11, &encoded) ==
		       error::unauthorized);
		command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
		command.accounting_intent = encoded;
		assert(item_transfer_accounting_command_supported(command));
	};
	auto nested = move(item_transfer_reason::player_drop, { item_owner_type::player, 10, 0 },
			   { item_owner_type::room, 50, 0 });
	item_transfer_payload payload = {};
	assert(item_transfer_command_decode_payload(nested, &payload));
	payload.reason = item_transfer_reason::player_put;
	payload.to_owner = { item_owner_type::player, 10, 0 };
	payload.expected_to_revision = payload.expected_from_revision;
	payload.target_root_item_uid = 101;
	payload.target_parent_item_uid = 101;
	payload.expected_target_parent_revision = 2;
	payload.reason_id = 101;
	assert(item_transfer_command_build(&nested, id(3), payload, critical_source_site::command,
					   critical_deadline_class::interactive));
	admit(nested);

	auto locker = move(item_transfer_reason::locker_deposit, { item_owner_type::player, 10, 0 },
			   { item_owner_type::locker, 20, 30 });
	assert(item_transfer_command_decode_payload(locker, &payload));
	payload.to_owner.context_id = 30;
	payload.reason_id = 30;
	assert(item_transfer_command_build(&locker, id(3), payload, critical_source_site::command,
					   critical_deadline_class::interactive));
	admit(locker);
	admit(move(item_transfer_reason::locker_withdraw, { item_owner_type::locker, 20, 30 },
		   { item_owner_type::player, 10, 0 }));

	for (const auto reason :
	     { item_transfer_reason::pet_give, item_transfer_reason::pet_return })
	{
		const bool give = reason == item_transfer_reason::pet_give;
		auto pet = move(reason,
				give ? item_owner_identity{ item_owner_type::player, 10, 0 } :
				       item_owner_identity{ item_owner_type::pet, 500, 10 },
				give ? item_owner_identity{ item_owner_type::pet, 500, 10 } :
				       item_owner_identity{ item_owner_type::player, 10, 0 });
		assert(item_transfer_command_decode_payload(pet, &payload));
		payload.reason_id = 500;
		assert(item_transfer_command_build(&pet, id(3), payload,
						   critical_source_site::command,
						   critical_deadline_class::interactive));
		admit(pet);
	}
	auto coin = move(item_transfer_reason::player_get, { item_owner_type::room, 50, 0 },
			 { item_owner_type::player, 10, 0 });
	assert(item_transfer_command_decode_payload(coin, &payload));
	payload.items[0].vnum = VOBJ_COINS;
	assert(item_transfer_command_build(&coin, id(3), payload, critical_source_site::command,
					   critical_deadline_class::interactive));
	std::vector<uint8_t> encoded;
	assert(item_transfer_accounting_intent(coin, lineage, epoch, 10, &encoded) ==
	       error::unauthorized);
}

void administrative_storage_moves_are_bounded_to_direct_children()
{
	const auto lineage = id(1);
	const auto epoch = id(2);
	auto storage = move(item_transfer_reason::player_drop, { item_owner_type::player, 10, 0 },
			    { item_owner_type::room, 50, 0 });
	item_transfer_payload payload = {};
	assert(item_transfer_command_decode_payload(storage, &payload));
	payload.from_owner = { item_owner_type::room, 50, 0 };
	payload.to_owner = payload.from_owner;
	payload.reason = item_transfer_reason::operator_repair;
	payload.reason_id = 101;
	payload.expected_to_revision = payload.expected_from_revision;
	payload.items[0].root_item_uid = 101;
	payload.items[0].parent_item_uid = 101;
	assert(item_transfer_command_build(&storage, id(3), payload, critical_source_site::command,
					   critical_deadline_class::interactive));
	std::vector<uint8_t> encoded;
	assert(item_transfer_accounting_intent(storage, lineage, epoch, 10, &encoded) == error::ok);
	storage.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	storage.accounting_intent = encoded;
	assert(item_transfer_accounting_command_supported(storage));
	payload.reason_id = 102;
	assert(item_transfer_command_build(&storage, id(4), payload, critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(item_transfer_accounting_intent(storage, lineage, epoch, 10, &encoded) ==
	       error::unauthorized);
}

void unsupported_or_changed_commands_fail_closed()
{
	const auto lineage = id(1);
	const auto epoch = id(2);
	auto command = move(item_transfer_reason::player_drop, { item_owner_type::player, 10, 0 },
			    { item_owner_type::room, 50, 0 });
	std::vector<uint8_t> encoded;
	assert(item_transfer_accounting_intent(command, lineage, epoch, 11, &encoded) ==
	       error::unauthorized);
	assert(item_transfer_accounting_intent(command, lineage, epoch, 10, &encoded) == error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	command.accounting_intent = encoded;
	assert(item_transfer_accounting_command_supported(command));

	auto altered_actor = command;
	economic_frozen_intent intent;
	assert(economic_intent_decode(altered_actor.accounting_intent, &intent) == error::ok);
	intent.admission.metadata.actor_id = 11;
	assert(economic_intent_encode(intent, &altered_actor.accounting_intent) == error::ok);
	assert(!item_transfer_accounting_command_supported(altered_actor));

	auto altered_payload = command;
	item_transfer_payload payload = {};
	assert(item_transfer_command_decode_payload(altered_payload, &payload));
	payload.to_owner.id = 11;
	assert(item_transfer_command_build(&altered_payload, id(3), payload,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	altered_payload.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	altered_payload.accounting_intent = command.accounting_intent;
	assert(!item_transfer_accounting_command_supported(altered_payload));

	for (const auto reason :
	     { item_transfer_reason::creation, item_transfer_reason::destruction,
	       item_transfer_reason::auction_list, item_transfer_reason::locker_deposit })
	{
		auto special = move(reason,
				    reason == item_transfer_reason::trusted_steal ?
					    item_owner_identity{ item_owner_type::player, 11, 0 } :
					    item_owner_identity{ item_owner_type::player, 10, 0 },
				    { item_owner_type::room, 50, 0 });
		assert(item_transfer_accounting_intent(special, lineage, epoch, 10, &encoded) ==
		       error::unauthorized);
	}
	auto trusted_steal = move(item_transfer_reason::trusted_steal,
				  { item_owner_type::player, 11, 0 },
				  { item_owner_type::player, 10, 0 });
	assert(item_transfer_accounting_intent(trusted_steal, lineage, epoch, 11, &encoded) ==
	       error::unauthorized);
	assert(item_transfer_accounting_intent(command, {}, epoch, 10, &encoded) ==
	       error::invalid_identity);
}

void sourced_creation_grants_are_bound_to_the_item_event()
{
	const auto lineage = id(1);
	const auto epoch = id(2);
	auto command = move(item_transfer_reason::creation, { item_owner_type::system, 0, 0 },
			    { item_owner_type::player, 10, 0 });
	std::vector<uint8_t> encoded;
	assert(item_transfer_accounting_intent(command, lineage, epoch, 10, &encoded) ==
	       error::unauthorized);
	assert(item_transfer_accounting_intent(command, lineage, epoch, 10, &encoded,
					       economic_source_kind::starter_grant) == error::ok);
	economic_frozen_intent decoded;
	assert(economic_intent_decode(encoded, &decoded) == error::ok);
	assert(decoded.admission.metadata.reason == economic_reason::item_create);
	assert(decoded.admission.metadata.source_event.has_value());
	assert(decoded.admission.metadata.source_event->kind ==
	       economic_source_kind::starter_grant);
	assert(decoded.admission.metadata.source_event->source.bytes == lineage.bytes);
	assert(decoded.admission.metadata.source_event->generation.bytes == lineage.bytes);
	assert(decoded.admission.metadata.source_event->sequence == 100);
	assert(decoded.admission.metadata.source_event->slot == 7);
	const auto original_source = *decoded.admission.metadata.source_event;
	item_transfer_payload retried_payload = {};
	assert(item_transfer_command_decode_payload(command, &retried_payload));
	critical_command retried = {};
	assert(item_transfer_command_build(&retried, id(4), retried_payload,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(item_transfer_accounting_intent(retried, lineage, id(5), 10, &encoded,
					       economic_source_kind::starter_grant) == error::ok);
	assert(economic_intent_decode(encoded, &decoded) == error::ok);
	assert(decoded.admission.metadata.source_event->source.bytes ==
	       original_source.source.bytes);
	assert(decoded.admission.metadata.source_event->generation.bytes ==
	       original_source.generation.bytes);
	assert(decoded.admission.metadata.source_event->sequence == original_source.sequence);
	assert(decoded.admission.metadata.source_event->slot == original_source.slot);
	assert(item_transfer_accounting_intent(retried, lineage, epoch, 10, &encoded,
					       economic_source_kind::spell_consumption) ==
	       error::unauthorized);
	assert(item_transfer_accounting_intent(retried, lineage, epoch, 10, &encoded,
					       economic_source_kind::spell_creation) == error::ok);
	assert(economic_intent_decode(encoded, &decoded) == error::ok);
	assert(decoded.admission.metadata.source_event->kind ==
	       economic_source_kind::spell_creation);
	assert(item_transfer_accounting_intent(command, lineage, epoch, 10, &encoded,
					       economic_source_kind::starter_grant) == error::ok);
	assert(economic_intent_decode(encoded, &decoded) == error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	command.accounting_intent = encoded;
	assert(item_transfer_accounting_command_supported(command));

	auto altered_source = command;
	assert(economic_intent_decode(altered_source.accounting_intent, &decoded) == error::ok);
	decoded.admission.metadata.source_event->slot++;
	assert(economic_intent_encode(decoded, &altered_source.accounting_intent) == error::ok);
	assert(!item_transfer_accounting_command_supported(altered_source));

	const auto invalid_actor = move(item_transfer_reason::creation,
					{ item_owner_type::system, 0, 0 },
					{ item_owner_type::player, 10, 0 });
	assert(item_transfer_accounting_intent(invalid_actor, lineage, epoch, 11, &encoded,
					       economic_source_kind::starter_grant) ==
	       error::unauthorized);
}

void quest_reward_source_survives_new_uid_and_command()
{
	const auto lineage = id(1);
	const auto epoch = id(2);
	auto command = move(item_transfer_reason::creation, { item_owner_type::system, 0, 0 },
			    { item_owner_type::player, 10, 0 });
	item_transfer_payload payload = {};
	assert(item_transfer_command_decode_payload(command, &payload));
	payload.reason_id = (int64_t{ 10 } << 32) | 123;
	assert(item_transfer_command_build(&command, id(20), payload, critical_source_site::command,
					   critical_deadline_class::interactive));
	std::vector<uint8_t> encoded;
	assert(item_transfer_accounting_intent(command, lineage, epoch, 10, &encoded,
					       economic_source_kind::quest_completion) ==
	       error::ok);
	economic_frozen_intent decoded;
	assert(economic_intent_decode(encoded, &decoded) == error::ok);
	assert(decoded.admission.metadata.source_event.has_value());
	const auto source = *decoded.admission.metadata.source_event;
	assert(source.kind == economic_source_kind::quest_completion &&
	       source.sequence == static_cast<uint64_t>(payload.reason_id) && source.slot == 0);
	payload.selected_item_uid = 101;
	payload.target_root_item_uid = 101;
	payload.items[0].item_uid = 101;
	payload.items[0].root_item_uid = 101;
	critical_command retry = {};
	assert(item_transfer_command_build(&retry, id(21), payload, critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(item_transfer_accounting_intent(retry, lineage, id(22), 10, &encoded,
					       economic_source_kind::quest_completion) ==
	       error::ok);
	assert(economic_intent_decode(encoded, &decoded) == error::ok);
	assert(decoded.admission.metadata.source_event->source.bytes == source.source.bytes &&
	       decoded.admission.metadata.source_event->generation.bytes ==
		       source.generation.bytes &&
	       decoded.admission.metadata.source_event->sequence == source.sequence &&
	       decoded.admission.metadata.source_event->slot == source.slot);
	payload.reason_id = 0;
	assert(item_transfer_command_build(&retry, id(23), payload, critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(item_transfer_accounting_intent(retry, lineage, epoch, 10, &encoded,
					       economic_source_kind::quest_completion) ==
	       error::unauthorized);
}

void logical_creation_source_survives_new_uid_and_command()
{
	const auto lineage = id(1);
	const auto epoch = id(2);
	auto command = move(item_transfer_reason::creation, { item_owner_type::system, 0, 0 },
			    { item_owner_type::player, 10, 0 });
	item_transfer_payload payload = {};
	assert(item_transfer_command_decode_payload(command, &payload));
	payload.logical_source_id = 90001;
	assert(item_transfer_command_build(&command, id(24), payload, critical_source_site::command,
					   critical_deadline_class::interactive));
	std::vector<uint8_t> encoded;
	assert(item_transfer_accounting_intent(command, lineage, epoch, 10, &encoded,
					       economic_source_kind::world_generation) ==
	       error::ok);
	economic_frozen_intent decoded;
	assert(economic_intent_decode(encoded, &decoded) == error::ok);
	assert(decoded.admission.metadata.source_event.has_value());
	const auto source = *decoded.admission.metadata.source_event;
	assert(source.kind == economic_source_kind::world_generation &&
	       source.source.bytes == lineage.bytes && source.generation.bytes == lineage.bytes &&
	       source.sequence == payload.logical_source_id && source.slot == 0);

	payload.selected_item_uid = 101;
	payload.target_root_item_uid = 101;
	payload.items[0].item_uid = 101;
	payload.items[0].root_item_uid = 101;
	critical_command retry = {};
	assert(item_transfer_command_build(&retry, id(25), payload, critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(item_transfer_accounting_intent(retry, lineage, id(5), 10, &encoded,
					       economic_source_kind::world_generation) ==
	       error::ok);
	assert(economic_intent_decode(encoded, &decoded) == error::ok);
	assert(decoded.admission.metadata.source_event.has_value());
	assert(decoded.admission.metadata.source_event->kind == source.kind &&
	       decoded.admission.metadata.source_event->source.bytes == source.source.bytes &&
	       decoded.admission.metadata.source_event->generation.bytes ==
		       source.generation.bytes &&
	       decoded.admission.metadata.source_event->sequence == source.sequence &&
	       decoded.admission.metadata.source_event->slot == source.slot);

	assert(item_transfer_accounting_intent(retry, lineage, epoch, 10, &encoded,
					       economic_source_kind::spell_creation) == error::ok);
	assert(economic_intent_decode(encoded, &decoded) == error::ok);
	assert(decoded.admission.metadata.source_event->kind ==
	       economic_source_kind::spell_creation);
	payload.logical_source_id = 0;
	assert(item_transfer_command_build(&retry, id(26), payload, critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(item_transfer_accounting_intent(retry, lineage, epoch, 10, &encoded,
					       economic_source_kind::world_generation) ==
	       error::ok);
	assert(economic_intent_decode(encoded, &decoded) == error::ok);
	assert(decoded.admission.metadata.source_event->sequence == 101);
	assert(decoded.admission.metadata.source_event->slot == 7);

	payload.logical_source_id = 90001;
	assert(item_transfer_command_build(&retry, id(27), payload, critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(item_transfer_accounting_intent(retry, lineage, epoch, 10, &encoded,
					       economic_source_kind::quest_completion) ==
	       error::unauthorized);
}

void sourced_room_creation_and_item_retirement_are_bound_to_lifecycle_events()
{
	const auto lineage = id(1);
	const auto epoch = id(2);
	std::vector<uint8_t> encoded;
	auto room_creation = move(item_transfer_reason::creation, { item_owner_type::system, 0, 0 },
				  { item_owner_type::player, 10, 0 });
	item_transfer_payload creation_payload = {};
	assert(item_transfer_command_decode_payload(room_creation, &creation_payload));
	creation_payload.to_owner = { item_owner_type::room, 50, 0 };
	assert(item_transfer_command_build(&room_creation, room_creation.operation_id,
					   creation_payload, critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(item_transfer_accounting_intent(room_creation, lineage, epoch, 10, &encoded,
					       economic_source_kind::administrator) == error::ok);
	assert(item_transfer_accounting_intent(room_creation, lineage, epoch, 10, &encoded,
					       economic_source_kind::item_action) == error::ok);
	economic_frozen_intent decoded;
	assert(economic_intent_decode(encoded, &decoded) == error::ok);
	assert(decoded.admission.metadata.reason == economic_reason::item_create);
	assert(decoded.admission.metadata.source_event->kind == economic_source_kind::item_action);
	assert(decoded.admission.metadata.source_event->sequence == 100);
	room_creation.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	room_creation.accounting_intent = encoded;
	assert(item_transfer_accounting_command_supported(room_creation));

	for (const auto &retirement :
	     { move(item_transfer_reason::destruction, { item_owner_type::player, 11, 0 },
		    { item_owner_type::room, 50, 0 }),
	       move(item_transfer_reason::destruction, { item_owner_type::room, 50, 0 },
		    { item_owner_type::room, 50, 0 }) })
	{
		assert(item_transfer_accounting_intent(retirement, lineage, epoch, 10, &encoded,
						       economic_source_kind::item_action) ==
		       error::ok);
		assert(economic_intent_decode(encoded, &decoded) == error::ok);
		assert(decoded.admission.metadata.reason == economic_reason::item_destroy);
		assert(decoded.admission.metadata.source_event->kind ==
		       economic_source_kind::item_action);
		auto admitted = retirement;
		admitted.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
		admitted.accounting_intent = encoded;
		assert(item_transfer_accounting_command_supported(admitted));
		assert(item_transfer_accounting_intent(
			       retirement, lineage, epoch, 10, &encoded,
			       economic_source_kind::intentional_destruction) == error::ok);
	}
	const auto consumed = move(item_transfer_reason::destruction,
				   { item_owner_type::player, 10, 0 },
				   { item_owner_type::destruction, 0, 0 });
	assert(item_transfer_accounting_intent(consumed, lineage, epoch, 10, &encoded,
					       economic_source_kind::spell_consumption) ==
	       error::ok);
	assert(economic_intent_decode(encoded, &decoded) == error::ok);
	assert(decoded.admission.metadata.source_event->kind ==
	       economic_source_kind::spell_consumption);
	assert(item_transfer_accounting_intent(consumed, lineage, epoch, 10, &encoded,
					       economic_source_kind::intentional_destruction) ==
	       error::ok);
	assert(item_transfer_accounting_intent(consumed, lineage, epoch, 10, &encoded,
					       economic_source_kind::spell_creation) ==
	       error::unauthorized);

	auto missing_source = move(item_transfer_reason::destruction,
				   { item_owner_type::player, 10, 0 },
				   { item_owner_type::room, 50, 0 });
	assert(item_transfer_accounting_intent(missing_source, lineage, epoch, 10, &encoded) ==
	       error::unauthorized);
	auto wrong_source = move(item_transfer_reason::destruction,
				 { item_owner_type::player, 10, 0 },
				 { item_owner_type::room, 50, 0 });
	assert(item_transfer_accounting_intent(wrong_source, lineage, epoch, 10, &encoded,
					       economic_source_kind::lifecycle) ==
	       error::unauthorized);
	auto inactive_item = move(item_transfer_reason::destruction,
				  { item_owner_type::player, 10, 0 },
				  { item_owner_type::room, 50, 0 });
	item_transfer_payload inactive_payload = {};
	assert(item_transfer_command_decode_payload(inactive_item, &inactive_payload));
	inactive_payload.items[0].expected_state = item_custody_state::absent;
	inactive_payload.items[0].expected_item_revision = ITEM_TRANSFER_ABSENT_REVISION;
	assert(!item_transfer_command_build(&inactive_item, inactive_item.operation_id,
					    inactive_payload, critical_source_site::command,
					    critical_deadline_class::interactive));

	auto coin_creation = move(item_transfer_reason::creation, { item_owner_type::system, 0, 0 },
				  { item_owner_type::player, 10, 0 });
	item_transfer_payload coin_creation_payload = {};
	assert(item_transfer_command_decode_payload(coin_creation, &coin_creation_payload));
	coin_creation_payload.items[0].vnum = VOBJ_COINS;
	assert(item_transfer_command_build(&coin_creation, coin_creation.operation_id,
					   coin_creation_payload, critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(item_transfer_accounting_intent(coin_creation, lineage, epoch, 10, &encoded,
					       economic_source_kind::starter_grant) ==
	       error::unauthorized);
	auto coin_retirement = move(item_transfer_reason::destruction,
				    { item_owner_type::player, 10, 0 },
				    { item_owner_type::destruction, 0, 0 });
	item_transfer_payload coin_retirement_payload = {};
	assert(item_transfer_command_decode_payload(coin_retirement, &coin_retirement_payload));
	coin_retirement_payload.items[0].vnum = VOBJ_COINS;
	assert(item_transfer_command_build(&coin_retirement, coin_retirement.operation_id,
					   coin_retirement_payload, critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(item_transfer_accounting_intent(coin_retirement, lineage, epoch, 10, &encoded,
					       economic_source_kind::item_action) ==
	       error::unauthorized);
}

void corpse_custody_roots_bind_the_actor_and_exact_corpse()
{
	const auto lineage = id(1);
	const auto epoch = id(2);
	std::vector<uint8_t> encoded;
	for (const auto &entry : { std::pair{ item_transfer_reason::corpse_create, uint32_t{ 10 } },
				   std::pair{ item_transfer_reason::corpse_loot, uint32_t{ 22 } } })
	{
		auto command = corpse_move(entry.first, entry.second);
		assert(item_transfer_accounting_intent(command, lineage, epoch, entry.second,
						       &encoded) == error::ok);
		economic_frozen_intent decoded;
		assert(economic_intent_decode(encoded, &decoded) == error::ok);
		assert(decoded.admission.metadata.reason == economic_reason::item_move);
		assert(!decoded.admission.metadata.source_event);
		command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
		command.accounting_intent = encoded;
		assert(item_transfer_accounting_command_supported(command));
	}
	const auto create = corpse_move(item_transfer_reason::corpse_create, 10);
	assert(item_transfer_accounting_intent(create, lineage, epoch, 11, &encoded) ==
	       error::unauthorized);
	const auto loot = corpse_move(item_transfer_reason::corpse_loot, 22);
	assert(item_transfer_accounting_intent(loot, lineage, epoch, 23, &encoded) ==
	       error::unauthorized);
	auto bad_revision = corpse_move(item_transfer_reason::corpse_create, 10);
	item_transfer_payload payload = {};
	assert(item_transfer_command_decode_payload(bad_revision, &payload));
	payload.items[1].expected_item_revision = ITEM_TRANSFER_ABSENT_REVISION;
	assert(item_transfer_command_build(&bad_revision, bad_revision.operation_id, payload,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(item_transfer_accounting_intent(bad_revision, lineage, epoch, 10, &encoded) ==
	       error::unauthorized);
}

void craft_intent_and_native_effects_share_one_root()
{
	const auto lineage = id(1), epoch = id(2);
	item_transfer_payload payload = {};
	payload.from_owner = payload.to_owner = { item_owner_type::player, 10, 0 };
	payload.reason = item_transfer_reason::craft;
	payload.reason_id = 551;
	payload.expected_from_revision = payload.expected_to_revision = 3;
	payload.selected_item_uid = 200;
	payload.multi_root = true;
	payload.item_count = 2;
	payload.items[0] = { 100, 100, 0, 2, 9001, item_custody_state::active };
	payload.items[1] = { 101, 100, 100, 4, 9002, item_custody_state::active };
	player_item_snapshot output = {};
	output.object_uid = 200;
	output.vnum = 9003;
	output.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	output.equipment_slot = -1;
	player_item_snapshot child = output;
	child.object_uid = 201;
	child.parent_index = 0;
	auto encode_outputs = [&](const std::vector<player_item_snapshot> &items)
	{
		std::vector<uint8_t> bytes;
		assert(player_item_snapshot_list_encode(items, &bytes) ==
		       player_snapshot_codec_result::ok);
		payload.item_blob_size = bytes.size();
		std::copy(bytes.begin(), bytes.end(), payload.item_blob.begin());
	};
	encode_outputs({ output, child });
	auto command = [&]
	{
		critical_command result = {};
		assert(item_transfer_command_build(&result, id(3), payload,
						   critical_source_site::command,
						   critical_deadline_class::interactive));
		return result;
	};
	auto admitted = command();
	std::vector<uint8_t> encoded;
	assert(item_transfer_accounting_intent(admitted, lineage, epoch, 10, &encoded,
					       economic_source_kind::crafting) == error::ok);
	assert(item_transfer_accounting_intent(admitted, lineage, epoch, 11, &encoded,
					       economic_source_kind::crafting) ==
	       error::unauthorized);
	assert(item_transfer_accounting_intent(admitted, lineage, epoch, 10, &encoded,
					       economic_source_kind::item_action) ==
	       error::unauthorized);
	assert(item_transfer_accounting_intent(admitted, lineage, epoch, 10, &encoded,
					       economic_source_kind::crafting) == error::ok);
	admitted.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	admitted.accounting_intent = encoded;
	assert(item_transfer_accounting_command_supported(admitted));
	economic_frozen_intent intent;
	assert(economic_intent_decode(encoded, &intent) == error::ok);
	assert(intent.admission.metadata.reason == economic_reason::crafting_cost);
	assert(intent.admission.metadata.source_event->sequence == 100);
	std::vector<economic_item_snapshot> inputs = {
		{ 100, { payload.from_owner, 100, 0, 2, item_custody_state::active, 7 } },
		{ 101, { payload.from_owner, 100, 100, 4, item_custody_state::active } }
	};
	economic_accounting_plan plan;
	assert(economic_intent_plan_metadata(admitted, intent, &plan.metadata) == error::ok);
	assert(item_transfer_craft_accounting_effects(payload, inputs, &plan) == error::ok);
	assert(plan.item_events.size() == 4 && plan.items_before.size() == 4);
	assert(plan.item_events[0].after.state == item_custody_state::destroyed);
	assert(plan.item_events[0].before.equipment_slot == 7 &&
	       plan.item_events[0].after.equipment_slot == 0);
	assert(plan.item_events[1].after.parent_uid == 100);
	assert(plan.item_events[2].before.state == item_custody_state::absent);
	assert(plan.item_events[3].after.root_uid == 200 &&
	       plan.item_events[3].after.parent_uid == 200);
	assert(economic_plan_validate_structure(plan) == error::ok);
	auto changed = inputs;
	++changed[0].position.revision;
	assert(item_transfer_craft_accounting_effects(payload, changed, &plan) != error::ok);
	// Intended gameplay failure still consumes the frozen inputs, without output.
	payload.item_blob_size = 0;
	payload.selected_item_uid = 100;
	assert(item_transfer_craft_accounting_effects(payload, inputs, &plan) == error::ok);
	assert(plan.item_events.size() == 2 && plan.items_after.size() == 2);
	// A craft cannot mint a coin payload through the custody-only owner.
	payload.selected_item_uid = 200;
	output.type = ITEM_MONEY;
	encode_outputs({ output });
	assert(item_transfer_accounting_intent(command(), lineage, epoch, 10, &encoded,
					       economic_source_kind::crafting) ==
	       error::unauthorized);
	assert(item_transfer_craft_accounting_effects(payload, inputs, &plan) != error::ok);
}

static void retained_pouch_is_an_update_and_never_a_source_lifetime()
{
	item_transfer_payload payload = {};
	payload.from_owner = { item_owner_type::player, 10, 0 };
	payload.to_owner = payload.from_owner;
	payload.reason = item_transfer_reason::craft;
	payload.reason_id = VOBJ_CHAOS_CRAFT_POUCH;
	payload.expected_from_revision = payload.expected_to_revision = 3;
	payload.selected_item_uid = 100;
	payload.multi_root = true;
	payload.item_count = 2;
	payload.items[0] = { 50, 40, 40, 7, VOBJ_CHAOS_CRAFT_POUCH, item_custody_state::active };
	payload.items[1] = { 100, 100, 0, 2, 400000, item_custody_state::active };
	craft_pouch_mutation pouch;
	pouch.mode = chaos_pouch_usage_mode::collected;
	pouch.usage = { { 400000, 1 } };
	pouch.before.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	pouch.before.object_uid = 50;
	pouch.before.vnum = VOBJ_CHAOS_CRAFT_POUCH;
	assert(chaos_pouch_ledger_prepare(pouch.before, pouch.usage, pouch.mode, &pouch.after) ==
	       chaos_pouch_ledger_result::ok);
	payload.continuation.kind = item_transfer_continuation_kind::craft_pouch_usage;
	assert(craft_pouch_mutation_encode(pouch, &payload.continuation.data));
	critical_command command;
	assert(item_transfer_command_build(&command, id(9), payload, critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(command.payload_version == 10);
	item_transfer_payload decoded = {};
	assert(item_transfer_command_decode_payload(command, &decoded));
	auto old_reader = command;
	old_reader.payload_version = ITEM_TRANSFER_CONTINUATION_PAYLOAD_VERSION;
	assert(!item_transfer_command_decode_payload(old_reader, &decoded));
	std::vector<uint8_t> encoded;
	assert(item_transfer_accounting_intent(command, id(1), id(2), 10, &encoded,
					       economic_source_kind::crafting) == error::ok);
	economic_frozen_intent intent;
	assert(economic_intent_decode(encoded, &intent) == error::ok);
	assert(intent.admission.metadata.source_event->sequence == 100);
	const std::vector<economic_item_snapshot> before = {
		{ 40, { payload.from_owner, 40, 0, 6, item_custody_state::active } },
		{ 50, { payload.from_owner, 40, 40, 7, item_custody_state::active } },
		{ 100, { payload.from_owner, 100, 0, 2, item_custody_state::active, 5 } }
	};
	economic_accounting_plan plan;
	assert(item_transfer_craft_accounting_effects(payload, before, &plan) == error::ok);
	assert(plan.item_events.size() == 2);
	assert(plan.items_before.size() == 3 && plan.items_after.size() == 3);
	assert(economic_item_position_equal(plan.items_before[0].position,
					    plan.items_after[0].position));
	assert(plan.item_events[0].uid == 50 &&
	       plan.item_events[0].after.state == item_custody_state::active);
	assert(plan.item_events[0].after.revision == 8 &&
	       plan.item_events[0].after.parent_uid == 40);
	assert(item_owner_identity_equal(plan.item_events[0].after.owner, payload.from_owner));
	assert(plan.item_events[1].after.state == item_custody_state::destroyed &&
	       plan.item_events[1].after.equipment_slot == 0);
	pouch.usage[0].count = 2;
	assert(chaos_pouch_ledger_prepare(pouch.before, pouch.usage, pouch.mode, &pouch.after) ==
	       chaos_pouch_ledger_result::ok);
	assert(craft_pouch_mutation_encode(pouch, &payload.continuation.data));
	assert(!item_transfer_command_build(&command, id(9), payload, critical_source_site::command,
					    critical_deadline_class::interactive));
}

int main()
{
	ordinary_moves_are_bound_to_actor_and_payload();
	nested_locker_and_pet_moves_bind_actor_to_custody();
	administrative_storage_moves_are_bounded_to_direct_children();
	unsupported_or_changed_commands_fail_closed();
	sourced_creation_grants_are_bound_to_the_item_event();
	quest_reward_source_survives_new_uid_and_command();
	logical_creation_source_survives_new_uid_and_command();
	sourced_room_creation_and_item_retirement_are_bound_to_lifecycle_events();
	corpse_custody_roots_bind_the_actor_and_exact_corpse();
	craft_intent_and_native_effects_share_one_root();
	retained_pouch_is_an_update_and_never_a_source_lifetime();
	return 0;
}

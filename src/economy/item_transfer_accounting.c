#include "economy/item_transfer_accounting.h"
#include "world/vnum.obj.h"

#include <climits>
#include <new>

namespace
{
bool ordinary_player_move(const item_transfer_payload &payload, uint32_t actor_pid)
{
	if (!actor_pid || actor_pid > INT32_MAX || !payload.item_count || payload.corpse.present ||
	    payload.collector.present || !item_owner_identity_valid(payload.from_owner) ||
	    !item_owner_identity_valid(payload.to_owner))
		return false;
	for (size_t index = 0; index < payload.item_count; ++index)
		if (payload.items[index].expected_state != item_custody_state::active ||
		    payload.items[index].expected_item_revision == ITEM_TRANSFER_ABSENT_REVISION ||
		    payload.items[index].vnum == VOBJ_COINS)
			return false;
	switch (payload.reason)
	{
	case item_transfer_reason::player_get:
		return (payload.from_owner.type == item_owner_type::room ||
			payload.from_owner.type == item_owner_type::container ||
			(payload.from_owner.type == item_owner_type::player &&
			 payload.from_owner.id == actor_pid)) &&
		       payload.to_owner.type == item_owner_type::player &&
		       payload.to_owner.id == actor_pid && !payload.target_parent_item_uid;
	case item_transfer_reason::player_drop:
	case item_transfer_reason::combat_fumble:
	case item_transfer_reason::critical_disarm:
		return payload.from_owner.type == item_owner_type::player &&
		       payload.from_owner.id == actor_pid &&
		       payload.to_owner.type == item_owner_type::room &&
		       (!item_transfer_forced_weapon_drop(payload.reason) ||
			(payload.reason_id > 0 &&
			 payload.reason_id <= ITEM_TRANSFER_MAX_EQUIPMENT_SLOT));
	case item_transfer_reason::player_put:
		return payload.from_owner.type == item_owner_type::player &&
		       payload.from_owner.id == actor_pid && payload.target_parent_item_uid &&
		       payload.reason_id == static_cast<int64_t>(payload.target_parent_item_uid) &&
		       ((payload.to_owner.type == item_owner_type::player &&
			 payload.to_owner.id == actor_pid) ||
			payload.to_owner.type == item_owner_type::room ||
			payload.to_owner.type == item_owner_type::container);
	case item_transfer_reason::locker_deposit:
		return payload.from_owner.type == item_owner_type::player &&
		       payload.from_owner.id == actor_pid &&
		       payload.to_owner.type == item_owner_type::locker && payload.to_owner.id &&
		       payload.to_owner.context_id;
	case item_transfer_reason::locker_withdraw:
		return payload.from_owner.type == item_owner_type::locker &&
		       payload.from_owner.id && payload.from_owner.context_id &&
		       payload.to_owner.type == item_owner_type::player &&
		       payload.to_owner.id == actor_pid && !payload.target_parent_item_uid;
	case item_transfer_reason::pet_give:
		return payload.from_owner.type == item_owner_type::player &&
		       payload.from_owner.id == actor_pid &&
		       payload.to_owner.type == item_owner_type::pet && payload.to_owner.id &&
		       payload.to_owner.context_id == actor_pid &&
		       payload.reason_id == static_cast<int64_t>(payload.to_owner.id) &&
		       !payload.multi_root && !payload.target_parent_item_uid;
	case item_transfer_reason::pet_return:
		return payload.from_owner.type == item_owner_type::pet && payload.from_owner.id &&
		       payload.from_owner.context_id == actor_pid &&
		       payload.to_owner.type == item_owner_type::player &&
		       payload.to_owner.id == actor_pid &&
		       payload.reason_id == static_cast<int64_t>(payload.from_owner.id) &&
		       !payload.multi_root && !payload.target_parent_item_uid;
	case item_transfer_reason::player_wear:
	case item_transfer_reason::player_remove:
		return payload.from_owner.type == item_owner_type::player &&
		       payload.from_owner.id == actor_pid &&
		       item_owner_identity_equal(payload.from_owner, payload.to_owner) &&
		       payload.reason_id > 0 &&
		       payload.reason_id <= ITEM_TRANSFER_MAX_EQUIPMENT_SLOT &&
		       payload.selected_item_uid == payload.target_root_item_uid &&
		       !payload.target_parent_item_uid && !payload.multi_root;
	case item_transfer_reason::operator_repair:
	{
		if (payload.from_owner.type != item_owner_type::room ||
		    !item_owner_identity_equal(payload.from_owner, payload.to_owner) ||
		    payload.from_owner.context_id || !payload.from_owner.id ||
		    payload.reason_id <= 0 || !payload.selected_item_uid ||
		    payload.selected_item_uid == static_cast<uint64_t>(payload.reason_id) ||
		    payload.target_root_item_uid != payload.selected_item_uid ||
		    payload.target_parent_item_uid || payload.multi_root)
			return false;
		const uint64_t storage_uid = static_cast<uint64_t>(payload.reason_id);
		for (size_t index = 0; index < payload.item_count; ++index)
			if (payload.items[index].item_uid == payload.selected_item_uid)
				return payload.items[index].root_item_uid == storage_uid &&
				       payload.items[index].parent_item_uid == storage_uid;
		return false;
	}
	case item_transfer_reason::player_give:
		return payload.from_owner.type == item_owner_type::player &&
		       payload.from_owner.id == actor_pid &&
		       payload.to_owner.type == item_owner_type::player &&
		       payload.to_owner.id != actor_pid;
	case item_transfer_reason::trusted_steal:
		return payload.from_owner.type == item_owner_type::player &&
		       payload.from_owner.id != actor_pid && payload.from_owner.id <= INT32_MAX &&
		       payload.to_owner.type == item_owner_type::player &&
		       payload.to_owner.id == actor_pid &&
		       payload.reason_id == static_cast<int64_t>(payload.from_owner.id);
	default:
		return false;
	}
}

bool corpse_player_move(const item_transfer_payload &payload, uint32_t actor_pid)
{
	if (!actor_pid || actor_pid > INT32_MAX || !payload.item_count || !payload.corpse.present ||
	    !item_owner_identity_valid(payload.from_owner) ||
	    !item_owner_identity_valid(payload.to_owner))
		return false;
	for (size_t index = 0; index < payload.item_count; ++index)
		if (payload.items[index].expected_state != item_custody_state::active ||
		    payload.items[index].expected_item_revision == ITEM_TRANSFER_ABSENT_REVISION ||
		    payload.items[index].vnum == VOBJ_COINS)
			return false;

	if (payload.reason == item_transfer_reason::corpse_create)
	{
		const uint32_t corpse_pid = static_cast<uint32_t>(payload.to_owner.id >> 32);
		const uint32_t corpse_save_id = static_cast<uint32_t>(payload.to_owner.id);
		return payload.from_owner.type == item_owner_type::player &&
		       payload.from_owner.id == actor_pid && !payload.from_owner.context_id &&
		       payload.to_owner.type == item_owner_type::corpse &&
		       !payload.to_owner.context_id && corpse_pid == actor_pid && corpse_save_id &&
		       payload.reason_id == static_cast<int64_t>(corpse_save_id);
	}
	if (payload.reason == item_transfer_reason::corpse_loot)
		return !payload.collector.present &&
		       payload.from_owner.type == item_owner_type::corpse &&
		       payload.to_owner.type == item_owner_type::player &&
		       payload.to_owner.id == actor_pid && !payload.to_owner.context_id;
	return false;
}

bool sourced_item_creation(const item_transfer_payload &payload, uint32_t actor_pid,
			   economic_source_kind kind)
{
	if (!actor_pid || actor_pid > INT32_MAX ||
	    payload.reason != item_transfer_reason::creation ||
	    !item_owner_identity_valid(payload.from_owner) ||
	    !item_owner_identity_valid(payload.to_owner) ||
	    payload.from_owner.type != item_owner_type::system || payload.from_owner.id ||
	    payload.from_owner.context_id || payload.to_owner.context_id ||
	    (payload.to_owner.type == item_owner_type::player &&
	     payload.to_owner.id != actor_pid) ||
	    (payload.to_owner.type != item_owner_type::player &&
	     payload.to_owner.type != item_owner_type::room) ||
	    payload.reason_id < 0 ||
	    (kind == economic_source_kind::quest_completion ? payload.reason_id == 0 :
							      payload.reason_id > UINT32_MAX) ||
	    payload.corpse.present || payload.collector.present || !payload.item_count)
		return false;
	for (size_t index = 0; index < payload.item_count; ++index)
		if (payload.items[index].expected_state != item_custody_state::absent ||
		    payload.items[index].expected_item_revision != ITEM_TRANSFER_ABSENT_REVISION ||
		    payload.items[index].vnum == VOBJ_COINS)
			return false;
	return true;
}

bool sourced_item_destruction(const item_transfer_payload &payload)
{
	if (payload.reason != item_transfer_reason::destruction ||
	    !item_owner_identity_valid(payload.from_owner) ||
	    !item_owner_identity_valid(payload.to_owner) ||
	    (payload.from_owner.type != item_owner_type::player &&
	     payload.from_owner.type != item_owner_type::room &&
	     payload.from_owner.type != item_owner_type::container) ||
	    payload.to_owner.type != item_owner_type::destruction || payload.to_owner.id ||
	    payload.to_owner.context_id || payload.reason_id < 0 ||
	    payload.reason_id > UINT32_MAX || payload.corpse.present || payload.collector.present ||
	    !payload.item_count)
		return false;
	for (size_t index = 0; index < payload.item_count; ++index)
		if (payload.items[index].expected_state != item_custody_state::active ||
		    payload.items[index].expected_item_revision == ITEM_TRANSFER_ABSENT_REVISION ||
		    payload.items[index].vnum == VOBJ_COINS)
			return false;
	return true;
}

bool creation_source_valid(economic_source_kind kind)
{
	switch (kind)
	{
	case economic_source_kind::quest_completion:
	case economic_source_kind::npc_generation:
	case economic_source_kind::starter_grant:
	case economic_source_kind::boon:
	case economic_source_kind::achievement:
	case economic_source_kind::world_generation:
	case economic_source_kind::crafting:
	case economic_source_kind::administrator:
	case economic_source_kind::legacy_import:
	case economic_source_kind::shop_stock:
	case economic_source_kind::lifecycle:
	case economic_source_kind::item_action:
	case economic_source_kind::spell_creation:
	case economic_source_kind::loot:
		return true;
	default:
		return false;
	}
}

uint64_t source_item_uid(const item_transfer_payload &payload)
{
	// The sorted item set is part of the frozen command. A UID-lifetime source
	// claim must survive a rebuilt command ID and an epoch change.
	return payload.selected_item_uid ? payload.selected_item_uid : payload.items[0].item_uid;
}

economic_source_event item_lifecycle_source(const item_transfer_payload &payload,
					    economic_source_kind kind,
					    const critical_operation_id &lineage)
{
	if (kind == economic_source_kind::quest_completion)
		return { kind, lineage, lineage, static_cast<uint64_t>(payload.reason_id), 0 };
	return { kind, lineage, lineage, source_item_uid(payload),
		 static_cast<uint32_t>(payload.reason_id) };
}
} // namespace

economic_accounting_error item_transfer_accounting_intent(const critical_command &command,
							  const critical_operation_id &lineage,
							  const critical_operation_id &epoch,
							  uint32_t actor_pid,
							  std::vector<uint8_t> *encoded,
							  economic_source_kind lifecycle_source)
{
	using error = economic_accounting_error;
	if (!encoded || command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    command.type != critical_command_type::item_transfer ||
	    !critical_command_legacy_execution_supported(command) ||
	    critical_operation_id_is_zero(lineage) || critical_operation_id_is_zero(epoch))
		return error::invalid_identity;
	item_transfer_payload payload = {};
	if (!item_transfer_command_decode_payload(command, &payload))
		return error::unauthorized;
	try
	{
		economic_admission_facts facts;
		facts.metadata.lineage = lineage;
		facts.metadata.epoch = epoch;
		facts.metadata.actor_kind = economic_actor_kind::domain;
		facts.metadata.actor_id = actor_pid;
		facts.metadata.writer_id = ECONOMIC_WRITER_ITEM_TRANSFER;
		if (payload.reason == item_transfer_reason::creation)
		{
			if (!creation_source_valid(lifecycle_source) ||
			    !sourced_item_creation(payload, actor_pid, lifecycle_source))
				return error::unauthorized;
			facts.metadata.reason = economic_reason::item_create;
			facts.metadata.source_event =
				item_lifecycle_source(payload, lifecycle_source, lineage);
		}
		else if (payload.reason == item_transfer_reason::destruction)
		{
			if ((lifecycle_source != economic_source_kind::item_action &&
			     lifecycle_source != economic_source_kind::spell_consumption &&
			     lifecycle_source != economic_source_kind::intentional_destruction) ||
			    !sourced_item_destruction(payload))
				return error::unauthorized;
			facts.metadata.reason = economic_reason::item_destroy;
			facts.metadata.source_event =
				item_lifecycle_source(payload, lifecycle_source, lineage);
		}
		else
		{
			if (lifecycle_source != economic_source_kind{} ||
			    (!ordinary_player_move(payload, actor_pid) &&
			     !corpse_player_move(payload, actor_pid)))
				return error::unauthorized;
			facts.metadata.reason = economic_reason::item_move;
		}
		return economic_intent_freeze(command, facts, encoded);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

bool item_transfer_accounting_command_supported(const critical_command &command) noexcept
{
	using error = economic_accounting_error;
	try
	{
		auto projection = command;
		if (!projection.accepted_at_usec)
			projection.accepted_at_usec = 1;
		if (command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
		    !critical_command_envelope_valid(projection) ||
		    command.type != critical_command_type::item_transfer)
			return false;
		economic_frozen_intent intent;
		if (economic_intent_decode(command.accounting_intent, &intent) != error::ok ||
		    intent.admission.metadata.writer_id != ECONOMIC_WRITER_ITEM_TRANSFER ||
		    intent.admission.metadata.actor_kind != economic_actor_kind::domain ||
		    intent.admission.metadata.actor_id > INT32_MAX)
			return false;
		economic_source_kind lifecycle_source = {};
		if (intent.admission.metadata.reason == economic_reason::item_create ||
		    intent.admission.metadata.reason == economic_reason::item_destroy)
		{
			if (!intent.admission.metadata.source_event)
				return false;
			lifecycle_source = intent.admission.metadata.source_event->kind;
		}
		critical_command admission = projection;
		admission.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		admission.accounting_intent.clear();
		admission.publication_required = false;
		std::vector<uint8_t> expected;
		if (item_transfer_accounting_intent(
			    admission, intent.admission.metadata.lineage,
			    intent.admission.metadata.epoch,
			    static_cast<uint32_t>(intent.admission.metadata.actor_id), &expected,
			    lifecycle_source) != error::ok)
			return false;
		return expected == command.accounting_intent;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

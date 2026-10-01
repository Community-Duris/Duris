#include "economy/item_transfer_accounting.h"
#include "world/vnum.obj.h"
#include "player/player_snapshot_codec.h"
#include "core/structs.h"

#include <algorithm>
#include <climits>
#include <new>
#include <unordered_set>

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
	    (kind == economic_source_kind::quest_completion && payload.logical_source_id) ||
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
	if ((payload.reason != item_transfer_reason::destruction &&
	     payload.reason != item_transfer_reason::quest_turnin) ||
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

bool craft_outputs(const item_transfer_payload &payload, uint32_t actor_pid,
		   std::vector<player_item_snapshot> *outputs)
{
	if (!outputs || !actor_pid || actor_pid > INT32_MAX ||
	    payload.reason != item_transfer_reason::craft ||
	    payload.from_owner.type != item_owner_type::player ||
	    payload.from_owner.id != actor_pid || payload.from_owner.context_id ||
	    !item_owner_identity_equal(payload.from_owner, payload.to_owner) ||
	    payload.expected_from_revision != payload.expected_to_revision || !payload.multi_root ||
	    !payload.item_count || payload.item_count > ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS ||
	    payload.target_root_item_uid || payload.target_parent_item_uid ||
	    payload.expected_target_parent_revision || payload.reason_id <= 0 ||
	    payload.reason_id > UINT32_MAX || payload.logical_source_id || payload.corpse.present ||
	    payload.collector.present ||
	    payload.continuation.kind != item_transfer_continuation_kind::none ||
	    !payload.continuation.data.empty() || payload.item_blob_size > payload.item_blob.size())
		return false;
	std::unordered_set<uint64_t> uids;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto &item = payload.items[index];
		if (!item.item_uid || item.expected_state != item_custody_state::active ||
		    item.expected_item_revision == ITEM_TRANSFER_ABSENT_REVISION ||
		    item.vnum == VOBJ_COINS || !uids.insert(item.item_uid).second)
			return false;
	}
	outputs->clear();
	if (payload.item_blob_size &&
	    (player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
					      outputs) != player_snapshot_codec_result::ok ||
	     outputs->empty()))
		return false;
	if (outputs->size() > ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS - payload.item_count)
		return false;
	for (size_t index = 0; index < outputs->size(); ++index)
	{
		const auto &output = (*outputs)[index];
		if (!output.object_uid || output.vnum <= 0 || output.vnum == VOBJ_COINS ||
		    output.type == ITEM_MONEY || !uids.insert(output.object_uid).second ||
		    output.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
		    output.parent_index >= static_cast<int32_t>(index) ||
		    (output.equipment_slot != -1 && output.equipment_slot != 0))
			return false;
	}
	return outputs->empty() || outputs->front().object_uid == payload.selected_item_uid;
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
	if (payload.logical_source_id)
		return { kind, lineage, lineage, payload.logical_source_id, 0 };
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
		else if (payload.reason == item_transfer_reason::destruction ||
			 payload.reason == item_transfer_reason::quest_turnin)
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
		else if (payload.reason == item_transfer_reason::craft)
		{
			std::vector<player_item_snapshot> outputs;
			if (lifecycle_source != economic_source_kind::crafting ||
			    !craft_outputs(payload, actor_pid, &outputs))
				return error::unauthorized;
			facts.metadata.reason = economic_reason::crafting_cost;
			// Input UID lifetimes identify the consumed recipe even if a retry
			// rebuilds its output UID or command ID. An output cannot be its own
			// issuance authority.
			facts.metadata.source_event = { lifecycle_source, lineage, lineage,
							payload.items[0].item_uid,
							static_cast<uint32_t>(payload.reason_id) };
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
		    intent.admission.metadata.reason == economic_reason::item_destroy ||
		    intent.admission.metadata.reason == economic_reason::crafting_cost)
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

economic_accounting_error
item_transfer_craft_accounting_effects(const item_transfer_payload &payload,
				       std::span<const economic_item_snapshot> inputs,
				       economic_accounting_plan *plan)
try
{
	using error = economic_accounting_error;
	std::vector<player_item_snapshot> outputs;
	if (!plan || inputs.size() != payload.item_count || payload.from_owner.id > INT32_MAX ||
	    !craft_outputs(payload, static_cast<uint32_t>(payload.from_owner.id), &outputs))
		return error::unauthorized;
	economic_accounting_plan effects;
	effects.items_before.assign(inputs.begin(), inputs.end());
	std::sort(effects.items_before.begin(), effects.items_before.end(),
		  [](const auto &left, const auto &right) { return left.uid < right.uid; });
	effects.items_after = effects.items_before;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto &expected = payload.items[index];
		const auto &prior = effects.items_before[index];
		if (prior.uid != expected.item_uid ||
		    !item_owner_identity_equal(prior.position.owner, payload.from_owner) ||
		    prior.position.root_uid != expected.root_item_uid ||
		    prior.position.parent_uid != expected.parent_item_uid ||
		    prior.position.revision != expected.expected_item_revision ||
		    prior.position.state != item_custody_state::active)
			return error::corrupt_evidence;
		auto &after = effects.items_after[index].position;
		after.owner = { item_owner_type::destruction, 0, 0 };
		after.state = item_custody_state::destroyed;
		++after.revision;
		after.equipment_slot = 0;
		effects.item_events.push_back(
			{ static_cast<uint32_t>(index), 0, prior.uid, prior.position, after });
	}
	std::vector<uint64_t> roots;
	roots.reserve(outputs.size());
	for (size_t index = 0; index < outputs.size(); ++index)
	{
		const auto &output = outputs[index];
		const bool root = output.parent_index == PLAYER_SNAPSHOT_NO_PARENT;
		const uint64_t root_uid = root ? output.object_uid : roots[output.parent_index];
		const uint64_t parent_uid = root ? 0 : outputs[output.parent_index].object_uid;
		roots.push_back(root_uid);
		const economic_item_position absent = {
			{ item_owner_type::unknown, 0, 0 }, 0, 0, 0, item_custody_state::absent
		};
		const economic_item_position created = { payload.to_owner, root_uid, parent_uid, 1,
							 item_custody_state::active };
		effects.items_before.push_back({ output.object_uid, absent });
		effects.items_after.push_back({ output.object_uid, created });
		effects.item_events.push_back({ static_cast<uint32_t>(payload.item_count + index),
						0, output.object_uid, absent, created });
	}
	for (auto *items : { &effects.items_before, &effects.items_after })
		std::sort(items->begin(), items->end(),
			  [](const auto &left, const auto &right) { return left.uid < right.uid; });
	const auto valid = economic_item_effects_validate(effects.items_before, effects.items_after,
							  effects.item_events, 0);
	if (valid != error::ok)
		return valid;
	plan->items_before = std::move(effects.items_before);
	plan->items_after = std::move(effects.items_after);
	plan->item_events = std::move(effects.item_events);
	return error::ok;
}
catch (const std::bad_alloc &)
{
	return economic_accounting_error::capacity;
}

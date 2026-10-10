#include "economy/item_transfer_accounting.h"
#include "economy/native_mobile_birth_accounting.h"
#include "economy/native_quest_cost_policy.h"
#include "item/craft_pouch_mutation.h"
#include "item/craft_recipe_continuation.h"
#include "world/vnum.obj.h"
#include "player/player_snapshot_codec.h"
#include "core/structs.h"

#include <algorithm>
#include <climits>
#include <new>
#include <unordered_map>
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
	    payload.collector.present || payload.item_blob_size > payload.item_blob.size())
		return false;
	craft_pouch_mutation pouch;
	if (!craft_pouch_mutation_from_payload(payload, &pouch))
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

economic_accounting_error item_transfer_accounting_intent(
	const critical_command &command, const critical_operation_id &lineage,
	const critical_operation_id &epoch, uint32_t actor_pid, std::vector<uint8_t> *encoded,
	economic_source_kind lifecycle_source, const economic_account_key *fresh_player_wallet)
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
			craft_recipe_continuation refine;
			if (craft_refine_from_payload(payload, &refine) &&
			    refine.refine_ore_count != 1 && fresh_player_wallet &&
			    (!economic_account_key_valid(*fresh_player_wallet) ||
			     fresh_player_wallet->kind != economic_account_kind::wallet ||
			     fresh_player_wallet->context_id ||
			     fresh_player_wallet->lineage.bytes != lineage.bytes ||
			     fresh_player_wallet->authority_id !=
				     refine.refine_cost.wallet_mapping_id))
				return error::unauthorized;
			facts.metadata.reason = economic_reason::crafting_cost;
			// Input UID lifetimes identify the consumed recipe even if a retry
			// rebuilds its output UID or command ID. An output cannot be its own
			// issuance authority.
			craft_pouch_mutation pouch;
			if (!craft_pouch_mutation_from_payload(payload, &pouch))
				return error::unauthorized;
			uint64_t consumed_uid = 0;
			for (size_t index = 0; index < payload.item_count; ++index)
				if (payload.items[index].item_uid != pouch.before.object_uid &&
				    (!consumed_uid || payload.items[index].item_uid < consumed_uid))
					consumed_uid = payload.items[index].item_uid;
			if (!consumed_uid)
				return error::unauthorized;
			facts.metadata.source_event = { lifecycle_source, lineage, lineage,
							consumed_uid,
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

// Pure frozen native facts only. This constructor does not extend the current
// admission predicate or install a SQL/flat owner for this native mutation.
economic_accounting_error item_native_mobile_accounting_intent(
	const critical_command &command, const critical_operation_id &lineage,
	const critical_operation_id &epoch, uint32_t actor_pid,
	const economic_source_event *original_quest_event, std::vector<uint8_t> *encoded) noexcept
{
	using error = economic_accounting_error;
	if (!encoded || !actor_pid || actor_pid > INT32_MAX ||
	    command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    !command.accounting_intent.empty() || command.accepted_at_usec ||
	    command.publication_required || command.type != critical_command_type::item_transfer ||
	    (command.payload_version != ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION &&
	     command.payload_version != ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION &&
	     command.payload_version != ITEM_TRANSFER_NATIVE_MOBILE_COST_PAYLOAD_VERSION &&
	     command.payload_version != ITEM_TRANSFER_NATIVE_MOBILE_COST_RECOVERY_PAYLOAD_VERSION &&
	     command.payload_version != ITEM_TRANSFER_NATIVE_MOBILE_MONEY_PAYLOAD_VERSION &&
	     command.payload_version !=
		     ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RECOVERY_PAYLOAD_VERSION) ||
	    critical_operation_id_is_zero(lineage) || critical_operation_id_is_zero(epoch))
		return error::invalid_identity;
	try
	{
		item_transfer_payload payload = {};
		if (!item_transfer_command_decode_payload(command, &payload) ||
		    payload.native_mobile.final_giver_pid != actor_pid)
			return error::unauthorized;
		economic_admission_facts facts;
		facts.metadata.lineage = lineage;
		facts.metadata.epoch = epoch;
		facts.metadata.actor_kind = economic_actor_kind::domain;
		facts.metadata.actor_id = actor_pid;
		facts.metadata.writer_id = ECONOMIC_WRITER_ITEM_TRANSFER;
		if (payload.native_mobile.action == item_native_mobile_action::acceptance)
		{
			if (original_quest_event)
				return error::
					unauthorized; // Acceptance does not create reward authority.
			facts.metadata.reason = payload.native_money.present ?
							economic_reason::coin_transfer :
							economic_reason::item_move;
		}
		else
		{
			if (!original_quest_event ||
			    !economic_source_event_valid(*original_quest_event) ||
			    (original_quest_event->kind != economic_source_kind::quest_action &&
			     original_quest_event->kind !=
				     economic_source_kind::quest_completion) ||
			    (payload.native_cost.present &&
			     original_quest_event->kind != economic_source_kind::quest_action) ||
			    (!payload.native_cost.present &&
			     payload.continuation.kind ==
				     item_transfer_continuation_kind::quest_offering &&
			     original_quest_event->kind != economic_source_kind::quest_completion))
				return error::unauthorized;
			if (payload.native_cost.fee_only &&
			    (original_quest_event->source.bytes != command.operation_id.bytes ||
			     original_quest_event->generation.bytes !=
				     payload.native_mobile.reference.birth_source.generation.bytes ||
			     original_quest_event->sequence !=
				     payload.native_mobile.reference.mobile_revision ||
			     original_quest_event->slot != payload.native_cost.completion_slot))
				return error::unauthorized;
			// One original action source covers every bound attempted fee slot.
			// Frozen successful reward terms retain their genuine later issuer.
			facts.metadata.reason = payload.native_cost.present ?
							economic_reason::quest_cost :
							economic_reason::item_destroy;
			facts.metadata.source_event = *original_quest_event;
		}
		return economic_intent_freeze(command, facts, encoded);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error
item_native_mobile_cost_accounting_effects(const item_transfer_payload &payload,
					   const economic_account_key &wallet,
					   economic_accounting_plan *plan) noexcept
{
	using error = economic_accounting_error;
	if (!plan || !payload.native_cost.present ||
	    payload.native_mobile.action != item_native_mobile_action::consumption ||
	    !economic_account_key_valid(wallet) || wallet.kind != economic_account_kind::wallet ||
	    wallet.context_id != ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT ||
	    wallet.authority_id != payload.native_cost.wallet_mapping_id ||
	    wallet.lineage.bytes != plan->metadata.lineage.bytes ||
	    plan->metadata.reason != economic_reason::quest_cost ||
	    plan->metadata.writer_id != ECONOMIC_WRITER_ITEM_TRANSFER ||
	    plan->metadata.actor_kind != economic_actor_kind::domain ||
	    plan->metadata.actor_id != payload.native_mobile.final_giver_pid ||
	    plan->metadata.policy_version != ECONOMIC_QUEST_REQUIREMENT_POLICY_VERSION ||
	    !plan->metadata.source_event ||
	    plan->metadata.source_event->kind != economic_source_kind::quest_action ||
	    !economic_source_event_valid(*plan->metadata.source_event) || !plan->accounts.empty() ||
	    !plan->postings.empty())
		return error::unauthorized;
	try
	{
		std::vector<uint8_t> exact;
		if (payload.native_cost.projection.attempts.empty() ||
		    native_quest_cost_projection_encode(payload.native_cost.projection, &exact) !=
			    native_quest_cost_projection_result::ok)
			return error::corrupt_evidence;
		const auto &cost = payload.native_cost.projection;
		auto candidate = *plan;
		bool charged = false;
		for (const auto &attempt : cost.attempts)
		{
			if (attempt.outcome != native_quest_cost_attempt_outcome::charged)
				continue;
			if (!charged)
			{
				candidate.accounts.push_back({ wallet, cost.before, cost.after,
							       cost.before_revision,
							       cost.after_revision });
				candidate.accounts.push_back(
					{ { wallet.lineage, economic_account_kind::sink,
					    ECONOMIC_QUEST_REQUIREMENT_SINK_ID,
					    ECONOMIC_QUEST_REQUIREMENT_SINK_CONTEXT },
					  {},
					  {},
					  0,
					  0 });
				charged = true;
			}
			if (candidate.postings.size() > ECONOMIC_ACCOUNTING_MAX_POSTINGS - 2)
				return error::capacity;
			economic_coin_vector debit{}, opposite{};
			auto status = economic_coin_delta(attempt.before, attempt.after, &debit);
			if (status != error::ok)
				return status;
			int64_t copper = 0;
			status = economic_coin_value(debit, &copper);
			if (status != error::ok)
				return status;
			if (copper != -static_cast<int64_t>(attempt.requirement.copper))
				return error::corrupt_evidence;
			for (size_t i = 0; i < debit.size(); ++i)
			{
				if (debit[i] == INT64_MIN)
					return error::overflow;
				opposite[i] = -debit[i];
			}
			candidate.postings.push_back(
				{ static_cast<uint32_t>(candidate.postings.size()), 0, 0, debit,
				  copper });
			candidate.postings.push_back(
				{ static_cast<uint32_t>(candidate.postings.size()), 1, 0, opposite,
				  -copper });
		}
		const auto status = economic_coin_effects_validate(
			candidate.accounts, candidate.postings, candidate.children.size());
		if (status != error::ok)
			return status;
		*plan = std::move(candidate);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error item_native_mobile_money_accounting_effects(
	const item_transfer_payload &payload, const economic_account_key &player_wallet,
	const economic_account_key &native_wallet, economic_accounting_plan *plan) noexcept
{
	using error = economic_accounting_error;
	if (!plan || !payload.native_money.present ||
	    !(payload.native_recovery.present ?
		      item_transfer_native_mobile_recovery_shape_valid(payload) :
		      item_transfer_native_mobile_shape_valid(payload)) ||
	    !economic_account_key_valid(player_wallet) ||
	    !economic_account_key_valid(native_wallet) ||
	    player_wallet.kind != economic_account_kind::wallet || player_wallet.context_id ||
	    native_wallet.kind != economic_account_kind::wallet ||
	    native_wallet.context_id != ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT ||
	    player_wallet.authority_id != payload.native_money.player_wallet_mapping_id ||
	    native_wallet.authority_id != payload.native_money.mobile_wallet_mapping_id ||
	    player_wallet.lineage.bytes != plan->metadata.lineage.bytes ||
	    native_wallet.lineage.bytes != plan->metadata.lineage.bytes ||
	    plan->metadata.writer_id != ECONOMIC_WRITER_ITEM_TRANSFER ||
	    plan->metadata.reason != economic_reason::coin_transfer ||
	    plan->metadata.policy_version != 1 ||
	    plan->metadata.actor_kind != economic_actor_kind::domain ||
	    plan->metadata.actor_id != payload.native_mobile.final_giver_pid ||
	    plan->metadata.source_event || !plan->accounts.empty() || !plan->postings.empty() ||
	    !plan->children.empty() || !plan->items_before.empty() || !plan->items_after.empty() ||
	    !plan->item_events.empty())
		return error::unauthorized;
	try
	{
		auto candidate = *plan;
		const auto &money = payload.native_money.projection;
		candidate.accounts = {
			{ player_wallet, money.player_before, money.player_after,
			  money.player_before_revision, money.player_after_revision },
			{ native_wallet, money.mobile_before, money.mobile_after,
			  money.mobile_before_revision, money.mobile_after_revision }
		};
		economic_coin_vector debit{}, credit{};
		auto status = economic_coin_delta(money.player_before, money.player_after, &debit);
		if (status != error::ok)
			return status;
		status = economic_coin_delta(money.mobile_before, money.mobile_after, &credit);
		if (status != error::ok)
			return status;
		int64_t value = 0;
		status = economic_coin_value(credit, &value);
		if (status != error::ok)
			return status;
		candidate.postings = { { 0, 0, 0, debit, -value }, { 1, 1, 0, credit, value } };
		status = economic_coin_effects_validate(candidate.accounts, candidate.postings, 0);
		if (status != error::ok)
			return status;
		*plan = std::move(candidate);
		return error::ok;
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
	if (!plan || inputs.size() < payload.item_count || payload.from_owner.id > INT32_MAX ||
	    !craft_outputs(payload, static_cast<uint32_t>(payload.from_owner.id), &outputs))
		return error::unauthorized;
	craft_pouch_mutation pouch;
	if (!craft_pouch_mutation_from_payload(payload, &pouch))
		return error::unauthorized;
	economic_accounting_plan effects;
	std::unordered_map<uint64_t, const economic_item_snapshot *> native;
	for (const auto &input : inputs)
		if (!native.emplace(input.uid, &input).second)
			return error::corrupt_evidence;
	std::unordered_set<uint64_t> witnesses;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		uint64_t cursor = payload.items[index].item_uid;
		bool complete = false;
		for (size_t depth = 0; depth <= PLAYER_SNAPSHOT_MAX_DEPTH; ++depth)
		{
			const auto found = native.find(cursor);
			if (found == native.end())
				return error::topology;
			witnesses.insert(cursor);
			cursor = found->second->position.parent_uid;
			if (!cursor)
			{
				complete = true;
				break;
			}
		}
		if (!complete)
			return error::topology;
	}
	for (const uint64_t uid : witnesses)
		effects.items_before.push_back(*native.at(uid));
	std::sort(effects.items_before.begin(), effects.items_before.end(),
		  [](const auto &left, const auto &right) { return left.uid < right.uid; });
	effects.items_after = effects.items_before;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto &expected = payload.items[index];
		const auto found = std::lower_bound(effects.items_before.begin(),
						    effects.items_before.end(), expected.item_uid,
						    [](const auto &item, uint64_t uid)
						    { return item.uid < uid; });
		if (found == effects.items_before.end() || found->uid != expected.item_uid)
			return error::corrupt_evidence;
		const size_t position = static_cast<size_t>(found - effects.items_before.begin());
		const auto &prior = *found;
		if (prior.uid != expected.item_uid ||
		    !item_owner_identity_equal(prior.position.owner, payload.from_owner) ||
		    prior.position.root_uid != expected.root_item_uid ||
		    prior.position.parent_uid != expected.parent_item_uid ||
		    prior.position.revision != expected.expected_item_revision ||
		    prior.position.state != item_custody_state::active)
			return error::corrupt_evidence;
		auto &after = effects.items_after[position].position;
		if (expected.item_uid != pouch.before.object_uid)
		{
			after.owner = { item_owner_type::destruction, 0, 0 };
			after.state = item_custody_state::destroyed;
			after.equipment_slot = 0;
		}
		++after.revision;
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

economic_accounting_error
item_transfer_refine_wallet_accounting_effects(const item_transfer_payload &payload,
					       economic_accounting_plan *plan) noexcept
{
	using error = economic_accounting_error;
	try
	{
		craft_recipe_continuation terms;
		if (!craft_refine_from_payload(payload, &terms))
			return error::ok;
		if (!plan || !plan->accounts.empty() || !plan->postings.empty() ||
		    plan->metadata.reason != economic_reason::crafting_cost ||
		    plan->metadata.writer_id != ECONOMIC_WRITER_ITEM_TRANSFER ||
		    plan->metadata.actor_id != terms.player_pid || !plan->metadata.source_event ||
		    plan->metadata.source_event->kind != economic_source_kind::crafting)
			return error::unauthorized;
		if (terms.refine_ore_count == 1)
			return error::ok;
		const auto &cost = terms.refine_cost;
		economic_accounting_plan candidate = *plan;
		const economic_account_key wallet{ candidate.metadata.lineage,
						   economic_account_kind::wallet,
						   cost.wallet_mapping_id, 0 };
		// Existing reason-scoped expense namespace, like the other shared sinks.
		const economic_account_key sink{
			candidate.metadata.lineage, economic_account_kind::sink,
			static_cast<uint64_t>(economic_reason::crafting_cost), 0
		};
		economic_coin_vector before{}, after{}, debit{}, credit{};
		for (size_t i = 0; i < 4; ++i)
		{
			before[i] = cost.before[i];
			after[i] = cost.after[i];
			debit[i] = after[i] - before[i];
			credit[i] = -debit[i];
		}
		candidate.accounts = { { wallet, before, after, cost.before_revision,
					 cost.after_revision },
				       { sink, {}, {}, 0, 0 } };
		candidate.postings = { { 0, 0, 0, debit, -50000 }, { 1, 1, 0, credit, 50000 } };
		const auto valid = economic_coin_effects_validate(
			candidate.accounts, candidate.postings, candidate.children.size());
		if (valid != error::ok)
			return valid;
		*plan = std::move(candidate);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

#include <type_traits>
namespace
{
bool item_replay_accounting_add(size_t &bytes, size_t extra) noexcept
{
	if (extra > SIZE_MAX - bytes)
		return false;
	bytes += extra;
	return true;
}
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
class item_replay_uid_set final : public std::__uset_hashtable<uint64_t>
{
	using table_type = std::__uset_hashtable<uint64_t>;
	using actual_node = std::__detail::_Hash_node<
		uint64_t, std::__cache_default<uint64_t, std::hash<uint64_t>>::value>;

    public:
	using table_type::table_type;
	bool current_heap(size_t *out) const noexcept
	{
		if (!out || !this->bucket_count())
			return false;
		size_t bytes = 0;
		if (this->bucket_count() > 1 &&
		    (this->bucket_count() > SIZE_MAX / sizeof(std::__detail::_Hash_node_base *) ||
		     !item_replay_accounting_add(bytes,
						 this->bucket_count() *
							 sizeof(std::__detail::_Hash_node_base *))))
			return false;
		if (this->size() > SIZE_MAX / sizeof(actual_node) ||
		    !item_replay_accounting_add(bytes, this->size() * sizeof(actual_node)))
			return false;
		*out = bytes;
		return true;
	}
	bool prospective_insert(uint64_t uid, size_t *out) const noexcept
	{
		if (!out || this->size() == this->max_size())
			return false;
		if (this->find(uid) != this->end())
		{
			*out = 0;
			return true;
		}
		size_t bytes = sizeof(actual_node);
		auto policy = this->__rehash_policy();
		const auto next = policy._M_need_rehash(this->bucket_count(), this->size(), 1);
		if (next.first && next.second > 1 &&
		    (next.second > SIZE_MAX / sizeof(std::__detail::_Hash_node_base *) ||
		     !item_replay_accounting_add(
			     bytes, next.second * sizeof(std::__detail::_Hash_node_base *))))
			return false;
		*out = bytes;
		return true;
	}
};
static_assert(sizeof(item_replay_uid_set) == sizeof(std::unordered_set<uint64_t>));
static_assert(alignof(item_replay_uid_set) == alignof(std::unordered_set<uint64_t>));
static_assert(std::is_same_v<item_replay_uid_set::iterator, std::unordered_set<uint64_t>::iterator>);
#else
using item_replay_uid_set = std::unordered_set<uint64_t>;
#endif
struct item_replay_accounting_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const critical_command *projection = nullptr, *admission = nullptr;
	const economic_frozen_intent *intent = nullptr;
	const item_transfer_payload *payload = nullptr;
	const std::vector<uint8_t> *expected = nullptr;
	const craft_pouch_mutation *pouch = nullptr;
	const craft_recipe_continuation *refine = nullptr;
	const std::vector<player_item_snapshot> *outputs = nullptr;
	const item_replay_uid_set *uids = nullptr;
	mutable bool denied = false;
	static bool forward(size_t bytes, void *context) noexcept
	{
		auto *owner = static_cast<item_replay_accounting_budget *>(context);
		if (!owner || owner->denied || !owner->reserve)
			return false;
		if (!owner->reserve(bytes, owner->context))
		{
			owner->denied = true;
			return false;
		}
		return true;
	}
	bool rows(const std::vector<player_item_snapshot> &value, size_t &bytes) const noexcept
	{
		if (value.capacity() > SIZE_MAX / sizeof(player_item_snapshot) ||
		    !item_replay_accounting_add(bytes,
						value.capacity() * sizeof(player_item_snapshot)))
			return false;
		for (const auto &row : value)
		{
			size_t heap = 0;
			if (!player_item_snapshot_current_heap_bytes(row, &heap) ||
			    !item_replay_accounting_add(bytes, heap))
				return false;
		}
		return true;
	}
	bool prefix(size_t &result, size_t extra = 0) const noexcept
	{
		if (denied)
			return false;
		// Actual prefix/rows/typed observers' argument/local/result/reference
		// carriers; pure leaf copy-frame and full codec profiles are additive.
		constexpr size_t observations =
			18 * sizeof(void *) + 10 * sizeof(size_t) + 8 * sizeof(bool) +
			2 * sizeof(std::vector<player_item_snapshot>::const_iterator);
		size_t bytes = outer, heap = 0;
		if (!item_replay_accounting_add(bytes, sizeof(*this)) ||
		    !item_replay_accounting_add(bytes, frames + observations) ||
		    !item_replay_accounting_add(bytes, critical_command_copy_frame_bytes()) ||
		    !item_replay_accounting_add(bytes, critical_command_valid_frame_bytes()) ||
		    !item_replay_accounting_add(bytes, item_transfer_payload_copy_frame_bytes()))
			return false;
		for (const auto *value : { projection, admission })
			if (value && (!critical_command_current_heap_bytes(*value, &heap) ||
				      !item_replay_accounting_add(bytes, heap)))
				return false;
		if (intent &&
		    !item_replay_accounting_add(bytes, intent->admission.facts.capacity()))
			return false;
		if (payload && (!item_transfer_payload_current_heap_bytes(*payload, &heap) ||
				!item_replay_accounting_add(bytes, heap)))
			return false;
		if (expected && !item_replay_accounting_add(bytes, expected->capacity()))
			return false;
		if (outputs && !rows(*outputs, bytes))
			return false;
		if (pouch)
		{
			if (pouch->usage.capacity() >
				    SIZE_MAX / sizeof(chaos_material_pouch_usage) ||
			    !item_replay_accounting_add(
				    bytes,
				    pouch->usage.capacity() * sizeof(chaos_material_pouch_usage)) ||
			    !player_item_snapshot_current_heap_bytes(pouch->before, &heap) ||
			    !item_replay_accounting_add(bytes, heap) ||
			    !player_item_snapshot_current_heap_bytes(pouch->after, &heap) ||
			    !item_replay_accounting_add(bytes, heap))
				return false;
		}
		if (refine &&
		    (!item_replay_accounting_add(bytes, refine->pouch_mutation.capacity()) ||
		     refine->refine_root_order.capacity() > SIZE_MAX / sizeof(uint64_t) ||
		     !item_replay_accounting_add(bytes, refine->refine_root_order.capacity() *
								sizeof(uint64_t)) ||
		     (refine->refine_material_name.capacity() > 15 &&
		      (refine->refine_material_name.capacity() == SIZE_MAX ||
		       !item_replay_accounting_add(bytes,
						   refine->refine_material_name.capacity() + 1)))))
			return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
		if (uids &&
		    (!uids->current_heap(&heap) || !item_replay_accounting_add(bytes, heap)))
			return false;
#else
		return false;
#endif
		if (!item_replay_accounting_add(bytes, extra))
			return false;
		result = bytes;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t bytes = 0;
		return prefix(bytes, extra) &&
		       forward(bytes, const_cast<item_replay_accounting_budget *>(this));
	}
	bool copy_command(const critical_command &input, critical_command &output) noexcept
	{
		size_t request = 0;
		if (!peak(4 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)) ||
		    !critical_command_fresh_copy_request_bytes(input, &request) || !peak(request))
			return false;
		try
		{
			output = input;
			return true;
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
	}
	bool uid_insert(uint64_t uid) const noexcept
	{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
		constexpr size_t frames = sizeof(std::__detail::_Prime_rehash_policy) +
					  2 * sizeof(std::pair<bool, size_t>) +
					  sizeof(std::__detail::_Prime_rehash_policy::_State) +
					  3 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) +
					  sizeof(std::pair<item_replay_uid_set::iterator, bool>) +
					  sizeof(std::allocator<std::__detail::_Hash_node_base *>) +
					  4 * (3 * sizeof(void *) + sizeof(size_t));
		if (!uids || !peak(frames))
			return false;
		size_t request = 0;
		return uids->prospective_insert(uid, &request) &&
		       item_replay_accounting_add(request, frames) && peak(request);
#else
		(void)uid;
		return false;
#endif
	}
};
bool craft_outputs_bounded(const item_transfer_payload &payload, uint32_t actor_pid,
			   std::vector<player_item_snapshot> *outputs,
			   item_replay_accounting_budget &budget)
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
	    payload.collector.present || payload.item_blob_size > payload.item_blob.size())
		return false;
	const size_t old_frames = budget.frames;
	budget.frames += sizeof(craft_pouch_mutation) + sizeof(item_replay_uid_set) +
			 6 * sizeof(void *) + 3 * sizeof(size_t) + 5 * sizeof(bool);
	if (!budget.peak())
	{
		budget.frames = old_frames;
		return false;
	}
	// Scope cleanup removes only private source ownership, never caller owners.
	struct reset_owner
	{
		item_replay_accounting_budget &budget;
		size_t frames;
		~reset_owner()
		{
			budget.pouch = nullptr;
			budget.uids = nullptr;
			budget.frames = frames;
		}
	} reset{ budget, old_frames };
	craft_pouch_mutation pouch;
	budget.pouch = &pouch;
	size_t nested = 0;
	if ((!budget.prefix(nested) ||
	     !craft_pouch_mutation_from_payload_bounded(
		     payload, &pouch, item_replay_accounting_budget::forward, &budget, nested)))
		return false;
	item_replay_uid_set uids;
	budget.uids = &uids;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto &item = payload.items[index];
		if (!item.item_uid || item.expected_state != item_custody_state::active ||
		    item.expected_item_revision == ITEM_TRANSFER_ABSENT_REVISION ||
		    item.vnum == VOBJ_COINS ||
		    (!budget.uid_insert(item.item_uid) || !uids.insert(item.item_uid).second))
			return false;
	}
	outputs->clear();
	if (payload.item_blob_size &&
	    ((!budget.prefix(nested) ?
		      player_snapshot_codec_result::overflow :
		      player_item_snapshot_list_decode_bounded(
			      payload.item_blob.data(), payload.item_blob_size, outputs,
			      item_replay_accounting_budget::forward, &budget, nested)) !=
		     player_snapshot_codec_result::ok ||
	     outputs->empty()))
		return false;
	if (outputs->size() > ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS - payload.item_count)
		return false;
	for (size_t index = 0; index < outputs->size(); ++index)
	{
		const auto &output = (*outputs)[index];
		if (!output.object_uid || output.vnum <= 0 || output.vnum == VOBJ_COINS ||
		    output.type == ITEM_MONEY ||
		    (!budget.uid_insert(output.object_uid) ||
		     !uids.insert(output.object_uid).second) ||
		    output.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
		    output.parent_index >= static_cast<int32_t>(index) ||
		    (output.equipment_slot != -1 && output.equipment_slot != 0))
			return false;
	}
	return outputs->empty() || outputs->front().object_uid == payload.selected_item_uid;
}

} // namespace
economic_accounting_error item_transfer_accounting_intent_bounded(
	const critical_command &command, const critical_operation_id &lineage,
	const critical_operation_id &epoch, uint32_t actor_pid, std::vector<uint8_t> *encoded,
	economic_source_kind lifecycle_source, const economic_account_key *fresh_player_wallet,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	using error = economic_accounting_error;
	if (!encoded || command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    command.type != critical_command_type::item_transfer ||
	    !critical_command_legacy_execution_supported(command) ||
	    critical_operation_id_is_zero(lineage) || critical_operation_id_is_zero(epoch))
		return error::invalid_identity;
	constexpr size_t frames = 8 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(uint32_t) +
				  5 * sizeof(bool) + 2 * sizeof(economic_source_kind) +
				  sizeof(uint64_t) + sizeof(item_transfer_payload) +
				  sizeof(economic_admission_facts) +
				  sizeof(std::vector<player_item_snapshot>) +
				  sizeof(craft_recipe_continuation) + sizeof(craft_pouch_mutation);
	item_replay_accounting_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return error::capacity;
	item_transfer_payload payload = {};
	budget.payload = &payload;
	size_t nested = 0;
	if ((!budget.prefix(nested) ||
	     !item_transfer_command_decode_payload_bounded(
		     command, &payload, item_replay_accounting_budget::forward, &budget, nested)))
		return budget.denied ? error::capacity : error::unauthorized;
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
			budget.outputs = &outputs;
			if (lifecycle_source != economic_source_kind::crafting ||
			    !craft_outputs_bounded(payload, actor_pid, &outputs, budget))
				return budget.denied ? error::capacity : error::unauthorized;
			craft_recipe_continuation refine;
			budget.refine = &refine;
			if ((payload.continuation.kind ==
				     item_transfer_continuation_kind::craft_recipe &&
			     budget.prefix(nested) &&
			     craft_recipe_continuation_decode_bounded(
				     payload.continuation.data, &refine,
				     item_replay_accounting_budget::forward, &budget, nested) &&
			     refine.discipline == craft_recipe_discipline::refine &&
			     craft_recipe_continuation_matches(refine, payload)) &&
			    refine.refine_ore_count != 1 && fresh_player_wallet &&
			    (!economic_account_key_valid(*fresh_player_wallet) ||
			     fresh_player_wallet->kind != economic_account_kind::wallet ||
			     fresh_player_wallet->context_id ||
			     fresh_player_wallet->lineage.bytes != lineage.bytes ||
			     fresh_player_wallet->authority_id !=
				     refine.refine_cost.wallet_mapping_id))
				return error::unauthorized;
			if (budget.denied)
				return error::capacity;
			facts.metadata.reason = economic_reason::crafting_cost;
			// Input UID lifetimes identify the consumed recipe even if a retry
			// rebuilds its output UID or command ID. An output cannot be its own
			// issuance authority.
			craft_pouch_mutation pouch;
			budget.pouch = &pouch;
			if ((!budget.prefix(nested) ||
			     !craft_pouch_mutation_from_payload_bounded(
				     payload, &pouch, item_replay_accounting_budget::forward,
				     &budget, nested)))
				return budget.denied ? error::capacity : error::unauthorized;
			uint64_t consumed_uid = 0;
			for (size_t index = 0; index < payload.item_count; ++index)
				if (payload.items[index].item_uid != pouch.before.object_uid &&
				    (!consumed_uid || payload.items[index].item_uid < consumed_uid))
					consumed_uid = payload.items[index].item_uid;
			if (!consumed_uid)
				return error::unauthorized;
			facts.metadata.source_event = { lifecycle_source, lineage, lineage,
							consumed_uid,
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
		budget.outputs = nullptr;
		budget.refine = nullptr;
		budget.pouch = nullptr;
		if (!budget.prefix(nested))
			return error::capacity;
		return economic_intent_freeze_fixed_bounded(command, facts, encoded,
							    item_replay_accounting_budget::forward,
							    &budget, nested);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

bool item_transfer_accounting_command_supported_bounded(const critical_command &command,
							bool (*reserve)(size_t, void *) noexcept,
							void *context, size_t outer_live) noexcept
{
	using error = economic_accounting_error;
	try
	{
		constexpr size_t frames =
			7 * sizeof(void *) + 4 * sizeof(size_t) + 4 * sizeof(bool) +
			2 * sizeof(critical_command) + sizeof(economic_frozen_intent) +
			sizeof(std::vector<uint8_t>) + sizeof(economic_source_kind);
		item_replay_accounting_budget budget{ reserve, context, outer_live, frames };
		size_t nested = 0;
		if (!budget.peak())
			return false;
		critical_command projection;
		budget.projection = &projection;
		if (!budget.copy_command(command, projection))
			return false;
		if (!projection.accepted_at_usec)
			projection.accepted_at_usec = 1;
		if (command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
		    !critical_command_envelope_valid(projection) ||
		    command.type != critical_command_type::item_transfer)
			return false;
		economic_frozen_intent intent;
		budget.intent = &intent;
		if ((!budget.prefix(nested) ?
			     error::capacity :
			     economic_intent_decode_bounded(command.accounting_intent, &intent,
							    item_replay_accounting_budget::forward,
							    &budget, nested)) != error::ok ||
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
		critical_command admission;
		budget.admission = &admission;
		if (!budget.copy_command(projection, admission))
			return false;
		admission.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		admission.accounting_intent.clear();
		admission.publication_required = false;
		std::vector<uint8_t> expected;
		budget.expected = &expected;
		if (!budget.prefix(nested) ||
		    item_transfer_accounting_intent_bounded(
			    admission, intent.admission.metadata.lineage,
			    intent.admission.metadata.epoch,
			    static_cast<uint32_t>(intent.admission.metadata.actor_id), &expected,
			    lifecycle_source, nullptr, item_replay_accounting_budget::forward,
			    &budget, nested) != error::ok)
			return false;
		if (!budget.peak(3 * sizeof(void *) + sizeof(size_t) + sizeof(int) + sizeof(bool)))
			return false;
		return expected == command.accounting_intent;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

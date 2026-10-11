#include "item/held_retirement_publication_internal.h"
#include "cmd/lockpick_retirement.h"
#include "item/item_ownership_runtime.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "core/prototypes.h"
#include "core/files.h"
#include "core/utils.h"

#include <utility>

extern P_obj object_list;
extern P_char character_list;
extern P_index obj_index;

bool item_held_retirement_publication_owner::same_items(
	const std::vector<player_item_snapshot> &left,
	const std::vector<player_item_snapshot> &right)
{
	std::vector<uint8_t> a, b;
	return player_item_snapshot_list_encode(left, &a) == player_snapshot_codec_result::ok &&
	       player_item_snapshot_list_encode(right, &b) == player_snapshot_codec_result::ok &&
	       a == b;
}

P_char item_held_retirement_publication_owner::actor_for(uint32_t pid) noexcept
{
	P_char actor = nullptr;
	for (P_char candidate = character_list; candidate; candidate = candidate->next)
		if (IS_PC(candidate) && candidate->only.pc &&
		    GET_PID(candidate) == static_cast<int>(pid) && candidate->runtime_id &&
		    find_character_by_runtime_id(candidate->runtime_id) == candidate)
		{
			if (actor && actor != candidate)
				return nullptr;
			actor = candidate;
		}
	return actor;
}

bool item_held_retirement_publication_owner::native_body(P_char actor,
							 std::vector<player_item_snapshot> *items)
{
	player_snapshot captured;
	if (!items ||
	    player_snapshot_capture_literal_inventory(
		    actor, 1,
		    PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_EQUIPMENT |
			    PLAYER_COMPONENT_INVENTORY,
		    RENT_CRASH, NOWHERE, 0, &captured) != player_snapshot_capture_result::ok)
		return false;
	*items = std::move(captured.items);
	return true;
}

bool item_held_retirement_publication_owner::checkpoint(critical_native_recovery_envelope *original,
							const held_retirement_recovery &context)
{
	if (!original || original->revision == UINT64_MAX)
		return false;
	critical_native_recovery_envelope successor = *original;
	++successor.revision;
	if (!held_retirement_recovery_encode(original->command, context, &successor.attachment) ||
	    !player_save_held_retirement_publication_owner::checkpoint_recovery_context(*original,
											successor))
		return false;
	*original = std::move(successor);
	return true;
}

bool item_held_retirement_publication_owner::native_publish(const critical_command &command,
							    const critical_completion &completion,
							    void *opaque) noexcept
{
#ifdef __NO_MYSQL__
	(void)command;
	(void)completion;
	(void)opaque;
	return false;
#else
	try
	{
		auto *attempt = static_cast<publication_attempt *>(opaque);
		auto *entry = attempt ? attempt->entry : nullptr;
		lockpick_retirement_terms terms;
		item_transfer_payload payload;
		critical_native_recovery_envelope envelope =
			attempt ? attempt->snapshot.envelope : critical_native_recovery_envelope{};
		held_retirement_recovery context;
		if (!entry || !entry->held_command ||
		    !held_retirement_command_identity(command, &payload, &terms) ||
		    !held_retirement_publication_snapshot_valid(command, completion,
								attempt->snapshot) ||
		    !held_retirement_recovery_decode(command, envelope.attachment, &context) ||
		    (context.receipt.present &&
		     !held_retirement_recovery_receipt_matches(context.receipt, completion)))
			return false;
		P_char actor = actor_for(terms.actor_pid);
		if (!actor || IS_SET(actor->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED))
			return false;
		if (attempt->snapshot.mode ==
			    held_retirement_publication_mode::terminal_ack_retry &&
		    context.physical_stage != 2)
			return false;
		const bool never_admitted = completion.disposition ==
					    critical_completion_disposition::never_admitted;
		item_transfer_result result{};
		const bool committed =
			!never_admitted &&
			(completion.outcome == critical_apply_outcome::applied ||
			 completion.outcome == critical_apply_outcome::already_applied);
		if (committed &&
		    (!item_transfer_command_decode_result(completion.result_payload.data(),
							  completion.result_size, &result) ||
		     result.root_item_uid != terms.item_uid || result.item_count != 1 ||
		     !result.max_item_revision || result.max_item_revision == UINT64_MAX))
			return false;
		result.operation_id = command.operation_id;
		held_retirement_current_observation current;
		// No runtime cache or absent physical UID can substitute for the real
		// repository's exact original receipt/current-cut/rollback observation.
		if (!held_retirement_repository_observe_current(command, completion, context,
								&current) ||
		    !current.rollback_confirmed || current.save_revision != context.save_revision ||
		    !same_items(current.items, committed ? context.after : context.before) ||
		    current.selected.item_uid != terms.item_uid ||
		    current.selected.root_item_uid != terms.item_uid ||
		    current.selected.parent_item_uid || current.selected.vnum != terms.item_vnum)
			return false;
		if (committed)
		{
			if (current.selected.state != item_custody_state::destroyed ||
			    current.selected.owner.type != item_owner_type::destruction ||
			    current.selected.owner.id || current.selected.owner.context_id ||
			    current.selected.item_revision != result.max_item_revision ||
			    current.from_owner_revision != result.from_owner_revision ||
			    current.to_owner_revision < result.to_owner_revision)
				return false;
		}
		else if (current.selected.state != item_custody_state::active ||
			 !item_owner_identity_equal(current.selected.owner, payload.from_owner) ||
			 current.selected.item_revision !=
				 payload.items[0].expected_item_revision ||
			 current.from_owner_revision != payload.expected_from_revision ||
			 current.to_owner_revision < payload.expected_to_revision)
			return false;
		std::vector<player_item_snapshot> actual;
		if (!native_body(actor, &actual))
			return false;
		const bool native_before = same_items(actual, context.before),
			   native_after = same_items(actual, context.after);
		if (committed ? (!native_before && !native_after) : !native_before)
			return false;
		size_t selected_count = 0;
		P_obj selected = nullptr;
		for (P_obj object = object_list; object; object = object->next)
			if (object->obj_uid == terms.item_uid)
			{
				selected = object;
				++selected_count;
			}
		if (selected_count > 1 ||
		    (native_before &&
		     (!selected || actor->equipment[HOLD] != selected ||
		      !OBJ_WORN_BY(selected, actor) || selected->type != ITEM_PICK ||
		      selected->contains || OBJ_VNUM(selected) != terms.item_vnum)) ||
		    (committed && native_after && selected_count))
			return false;
		if (!player_save_held_retirement_publication_owner::rebind_recovery_checkpoint(
			    envelope, actor->runtime_id))
			return false;
		entry->actor_runtime_id = actor->runtime_id;
		if (never_admitted)
			return native_before && !context.receipt.present && !context.physical_stage;
		if (!context.receipt.present)
		{
			context.receipt.present = true;
			context.receipt.outcome = completion.outcome;
			context.receipt.durable_revision = completion.durable_revision;
			context.receipt.error_code = completion.error_code;
			context.receipt.failure_stage = completion.failure_stage;
			context.receipt.result_size = completion.result_size;
			context.receipt.result = completion.result_payload;
			if (!checkpoint(&envelope, context))
				return false;
		}
		if (context.physical_stage == 2)
		{
			if (committed ? !native_after : !native_before)
				return false;
		}
		else
		{
			const bool notify = context.physical_stage == 0 && native_before;
			if (context.physical_stage == 0)
			{
				context.physical_stage = 1;
				if (!checkpoint(&envelope, context))
					return false;
			}
			if (!item_ownership_runtime_hydrate_owner(payload.from_owner,
								  current.from_owner_revision) ||
			    !item_ownership_runtime_hydrate_owner(payload.to_owner,
								  current.to_owner_revision) ||
			    !item_ownership_runtime_hydrate_many_atomic(&current.selected, 1) ||
			    !lockpick_retirement_publish_physical(command.operation_id, actor,
								  committed, result, terms, notify))
				return false;
			std::vector<player_item_snapshot> after_native;
			if (!native_body(actor, &after_native) ||
			    !same_items(after_native, committed ? context.after : context.before))
				return false;
			context.physical_stage = 2;
			if (!checkpoint(&envelope, context))
				return false;
		}
		// Reobserve current SQL after physical return and immediately before ACK.
		held_retirement_current_observation final_cut;
		if (!held_retirement_repository_observe_current(command, completion, context,
								&final_cut) ||
		    !final_cut.rollback_confirmed ||
		    final_cut.save_revision != current.save_revision ||
		    !same_items(final_cut.items, current.items) ||
		    final_cut.selected.item_uid != current.selected.item_uid ||
		    final_cut.selected.root_item_uid != current.selected.root_item_uid ||
		    final_cut.selected.parent_item_uid != current.selected.parent_item_uid ||
		    final_cut.selected.vnum != current.selected.vnum ||
		    final_cut.selected.state != current.selected.state ||
		    !item_owner_identity_equal(final_cut.selected.owner, current.selected.owner) ||
		    final_cut.selected.item_revision != current.selected.item_revision ||
		    final_cut.selected.owner_revision != current.selected.owner_revision ||
		    final_cut.from_owner_revision != current.from_owner_revision ||
		    final_cut.to_owner_revision != current.to_owner_revision)
			return false;
		entry->registry_applied = true;
		entry->ordinary_publication_ready = true;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool item_held_retirement_publication_owner::publish(pending_movement &entry) noexcept
{
	if (!entry.held_command)
		return false;
	publication_attempt attempt;
	attempt.entry = &entry;
#ifndef __NO_MYSQL__
	try
	{
		lockpick_retirement_terms terms;
		if (!player_save_held_retirement_publication_owner::copy_publication_context(
			    *entry.held_command, entry.completed, &attempt.snapshot))
			return false;
		const auto &envelope = attempt.snapshot.envelope;
		held_retirement_recovery context;
		if (!held_retirement_command_identity(*entry.held_command, nullptr, &terms) ||
		    !held_retirement_recovery_decode(*entry.held_command, envelope.attachment,
						     &context) ||
		    (context.receipt.present &&
		     !held_retirement_recovery_receipt_matches(context.receipt, entry.completed)))
			return false;
		held_retirement_current_observation current;
		const bool committed =
			entry.completed.disposition == critical_completion_disposition::execution &&
			(entry.completed.outcome == critical_apply_outcome::applied ||
			 entry.completed.outcome == critical_apply_outcome::already_applied);
		if (!held_retirement_repository_observe_current(
			    *entry.held_command, entry.completed, context, &current) ||
		    !current.rollback_confirmed || current.save_revision != context.save_revision ||
		    !same_items(current.items, committed ? context.after : context.before))
			return false;
		P_char actor = actor_for(terms.actor_pid);
		std::vector<player_item_snapshot> native;
		if (!actor || !native_body(actor, &native) ||
		    (committed ? (!same_items(native, context.before) &&
				  !same_items(native, context.after)) :
				 !same_items(native, context.before)) ||
		    !player_save_held_retirement_publication_owner::rebind_recovery_checkpoint(
			    envelope, actor->runtime_id))
			return false;
		entry.actor_runtime_id = actor->runtime_id;
	}
	catch (...)
	{
		return false;
	}
#endif
	return player_save_held_retirement_publication_owner::publish_held_retirement(
		*entry.held_command, entry.completed, attempt.snapshot, native_publish, &attempt);
}

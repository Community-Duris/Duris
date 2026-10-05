#!/usr/bin/env python3
"""Native craft owner/journal ACK ordering with real progression receipt logic.

No services: private journals, controlled world/skill/save leaves, actual native
coordinator, codecs, item owner and craft progression. Supplied source/artifact
paths support immutable before/after evidence without copying executing inputs.
"""

import argparse
import ast
import hashlib
import json
import os
from pathlib import Path
import shlex
import signal
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
ACK_SYMBOL = "_Z52critical_command_coordinator_acknowledge_publicationRK21critical_operation_id"
SOURCES = (
    "account/character_identity.c", "item/item_movement_transaction.c", "item/item_transfer_command.c", "world/quest_mobile_native_reference.c",
    "item/craft_pouch_mutation.c", "combat/chaos_pouch_ledger.c", "combat/chaos_pouch_publication.c",
    "player/player_snapshot_codec.c", "economy/item_transfer_accounting.c",
    "economy/economic_accounting_types.c", "economy/economic_accounting_plan.c", "economy/economic_source_event.c",
    "economy/economic_accounting_intent.c", "economy/economic_gameplay_authority.c",
    "economy/economic_command_admission.c", "economy/economic_currency_adapter.c",
    "economy/currency_command.c", "economy/coin_transfer_accounting.c",
    "economy/coin_transfer_command.c", "persistence/critical_command.c",
    "persistence/critical_command_journal.c", "persistence/critical_command_coordinator.c",
    "economy/craft_progression.c",
)
SCENARIOS = (
    "basic_applied_ack_retry", "basic_rejected_ack_retry", "recipe_applied_ack_retry",
    "recipe_rejected_ack_retry", "recipe_progression_wait", "basic_actor_absent",
    "recipe_actor_absent", "recipe_notify_throw", "basic_completion_throw",
    "recipe_completion_throw", "recipe_both_throw", "recipe_notify_reentrant",
    "basic_completion_reentrant", "recipe_notify_retire_actor",
    "basic_completion_retire_actor", "basic_never_admitted", "recipe_never_admitted",
    "basic_ack_exception", "recipe_ack_exception",
    "basic_malformed_pending_receipt", "basic_changed_failure_pending_receipt",
    "basic_batch_pending_receipt", "recipe_malformed_pending_receipt",
    "recipe_changed_failure_pending_receipt", "recipe_changed_result_pending_receipt",
    "recipe_batch_pending_receipt", "recipe_equivalent_pending_receipt",
    "recipe_malformed_waiting_receipt", "recipe_changed_failure_waiting_receipt",
    "recipe_batch_waiting_receipt",
)


def literal(root, name, constant="HARNESS"):
    for node in ast.parse((root / "tests/async" / name).read_text()).body:
        if isinstance(node, ast.Assign) and any(
            isinstance(target, ast.Name) and target.id == constant for target in node.targets
        ):
            return ast.literal_eval(node.value)
    raise AssertionError(f"Maintained {constant} fixture missing")


NATIVE = r'''

#include "core/files.h"
#include "magic/spells.h"
#include "player/player_save_pipeline.h"
#include "persistence/critical_command_journal.h"
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
static std::map<uint64_t, item_ownership_runtime_entry> custody;
static critical_operation_id original = {};
static std::string scenario;
static P_char current_actor = nullptr;
static pc_only_data player = {};
static std::unique_ptr<char_data> original_actor, replacement_actor;
static int root_callbacks = 0, root_notices = 0, root_acks = 0, root_ack_successes = 0;
static int callback_before_ack = 0, hook_acks = 0, escaped = 0, registry_updates = 0;
int placements = 0;
static int notches = 0, xp = 0, saves = 0, ui_notices = 0, progression_publish_calls = 0;
static bool applied = true, recipe = false, never = false, action_done = false;
static int ack_boundary_faults = 0, ack_exceptions = 0;
static int real_ack_calls = 0, real_ack_false = 0;
static P_obj original_input = nullptr, original_output = nullptr;
static decltype(craft_progression_hooks.publish) real_publish = nullptr;
static decltype(craft_progression_hooks.acknowledged) real_acknowledged = nullptr;
static decltype(craft_progression_hooks.notify) real_notify = nullptr;
static std::vector<std::unique_ptr<char_data>> new_actors;
static std::vector<std::unique_ptr<pc_only_data>> new_players;
static std::vector<std::unique_ptr<obj_data>> new_objects;
extern "C" bool real_publication_ack(const critical_operation_id &) __asm__(
	"__real__Z52critical_command_coordinator_acknowledge_publicationRK21critical_operation_id");
extern "C" bool observe_publication_ack(const critical_operation_id &) __asm__(
	"__wrap__Z52critical_command_coordinator_acknowledge_publicationRK21critical_operation_id");
extern "C" bool observe_publication_ack(const critical_operation_id &operation)
{
	if (operation.bytes == original.bytes)
		++root_acks;
	if (operation.bytes == original.bytes && ack_boundary_faults)
	{
		--ack_boundary_faults;
		++ack_exceptions;
		throw std::bad_alloc(); // Controlled public ACK exception boundary.
	}
	bool success = false;
	try
	{
		if (operation.bytes == original.bytes)
			++real_ack_calls;
		success = real_publication_ack(operation);
		if (!success && operation.bytes == original.bytes)
			++real_ack_false;
	}
	catch (...)
	{
		++ack_exceptions;
		throw;
	}
	if (success && operation.bytes == original.bytes)
		++root_ack_successes;
	return success;
}
static size_t original_records()
{
	size_t count = 0;
	assert(critical_command_journal_replay(
		       [](critical_command command, void *context)
		       {
			       if (command.operation_id.bytes == original.bytes)
				       ++*static_cast<size_t *>(context);
			       return true;
		       },
		       &count) == critical_command_journal_result::ok);
	return count;
}
bool item_ownership_runtime_lookup(uint64_t uid, item_ownership_runtime_entry *entry)
{
	const auto found = custody.find(uid);
	if (!entry || found == custody.end())
		return false;
	*entry = found->second;
	return true;
}
bool item_ownership_runtime_apply(const item_transfer_payload &, const item_transfer_result &)
{
	++registry_updates;
	return true;
}
bool notch_skill(P_char, int skill, float chance)
{
	assert(skill == SKILL_CRAFT && chance == 50);
	++notches;
	return true;
}
bool skill_notch_apply(P_char, int, const skill_notch_outcome &)
{
	std::abort();
}
int gain_exp(P_char, P_char, int amount, int kind)
{
	assert(kind == EXP_BOON);
	xp += amount;
	return 0;
}
player_save_pipeline_result player_save_pipeline_request(P_char actor, player_component_mask_t mask,
							 int, int)
{
	assert(actor == current_actor && mask == CRAFT_PROGRESSION_COMPONENTS);
	++saves;
	return player_save_pipeline_result::queued;
}
void send_to_char_f(P_char, const char *, ...)
{
	++ui_notices;
}
void act(const char *, int, P_char, P_obj, void *, int)
{
	++ui_notices;
}
void wizlog(int, const char *, ...)
{
	++ui_notices;
}
static critical_apply_result execute_craft(const critical_command &command, void *)
{
	++apply_calls;
	item_transfer_payload payload = {};
	assert(critical_command_envelope_valid(command) &&
	       item_transfer_command_decode_payload(command, &payload));
	item_transfer_result result = {};
	result.root_item_uid = payload.selected_item_uid;
	result.item_count = payload.item_count;
	result.from_owner_revision = result.to_owner_revision = result.max_item_revision = 2;
	std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> bytes = {};
	assert(item_transfer_command_encode_result(result, &bytes));
	critical_apply_result completion = { applied ? critical_apply_outcome::applied :
						       critical_apply_outcome::terminal_failure,
					     2, applied ? 0u : unsigned(EINVAL) };
	completion.result_size = bytes.size();
	std::copy(bytes.begin(), bytes.end(), completion.result_payload.begin());
	return completion;
}
static void handle(const critical_completion *completion = nullptr, size_t count = 0)
{
	try
	{
		item_movement_transaction_handle_completions(completion, count);
	}
	catch (...)
	{
		++escaped;
	}
}
static void ready()
{
	try
	{
		item_movement_transaction_player_ready(current_actor);
	}
	catch (...)
	{
		++escaped;
	}
}
static void reentrant_submissions()
{
	for (int index = 0; index < 64; ++index)
	{
		auto pc = std::make_unique<pc_only_data>();
		pc->pid = 2000 + index;
		auto actor = std::make_unique<char_data>();
		actor->only.pc = pc.get();
		actor->runtime_id = 8000 + index;
		fixture_register_character(actor.get());
		actor->next = character_list;
		character_list = actor.get();
		auto object = std::make_unique<obj_data>();
		object->obj_uid = 6000 + index;
		object->R_num = 0;
		object->loc_p = LOC_CARRIED;
		object->loc.carrying = actor.get();
		object->next = object_list;
		object_list = object.get();
		const item_owner_identity owner = { item_owner_type::player, uint64_t(pc->pid), 0 };
		custody[object->obj_uid] = {
			object->obj_uid,	   object->obj_uid, 0, owner, 1, 1, 42,
			item_custody_state::active
		};
		item_movement_reject reject = item_movement_reject::none;
		const bool accepted = item_movement_transaction_submit(
			actor.get(), object.get(), nullptr, owner,
			{ item_owner_type::player, uint64_t(pc->pid + 10000), 0 },
			item_transfer_reason::player_give, pc->pid, nullptr, nullptr, 0, nullptr,
			&reject);
		check(accepted,
		      "post-ACK callback admits a distinct actor root while growing the actual pending map");
		new_players.push_back(std::move(pc));
		new_actors.push_back(std::move(actor));
		new_objects.push_back(std::move(object));
	}
}
static void retire_and_replace()
{
	P_char retired = current_actor;
	fixture_retire_character(retired);
	P_char *link = &character_list;
	while (*link && *link != retired)
		link = &(*link)->next;
	assert(*link == retired);
	*link = retired->next;
	replacement_actor = std::make_unique<char_data>();
	replacement_actor->only.pc = &player;
	replacement_actor->runtime_id = 9001;
	fixture_register_character(replacement_actor.get());
	replacement_actor->next = character_list;
	character_list = replacement_actor.get();
	current_actor = replacement_actor.get();
	original_actor.reset();
}
static void business_action(const char *phase)
{
	if (action_done)
		return;
	if (scenario.find(std::string(phase) + "_reentrant") != std::string::npos)
	{
		action_done = true;
		reentrant_submissions();
	}
	else if (scenario.find(std::string(phase) + "_retire_actor") != std::string::npos)
	{
		action_done = true;
		retire_and_replace();
	}
}
static void physical_boundary()
{
	if (applied && !never)
		check(extractions == 1 && placements == 1 && original_input->obj_uid == 0 &&
			      original_output->obj_uid == 5030 &&
			      original_output->loc_p == LOC_CARRIED &&
			      original_output->loc.carrying == original_actor.get(),
		      "original input retired and original output physically placed before business hooks");
	else
		check(extractions == 1 && placements == 0 && original_output->obj_uid == 0 &&
			      original_input->obj_uid == 5001 &&
			      original_input->loc_p == LOC_CARRIED,
		      "rejected or never-admitted craft discards only staged output and preserves input");
}
static void event_boundary(P_char actor)
{
	check(!critical_command_coordinator_is_fenced({ critical_entity_type::item, 5001 },
						      nullptr) &&
		      original_records() == 0,
	      "business notification follows original durable journal ACK or definite non-admission");
	if (critical_command_coordinator_is_fenced({ critical_entity_type::item, 5001 }, nullptr))
		++callback_before_ack;
	check(!item_movement_transaction_player_busy(current_actor),
	      "original domain owner extracted before business callback");
	check(actor == current_actor && actor &&
		      find_character_by_runtime_id(actor->runtime_id) == actor,
	      "business callback resolves the current authoritative registered actor lifetime");
	if (!action_done)
		physical_boundary();
}
static void completion_callback(P_char actor, bool committed, const item_transfer_result &,
				unsigned int error, const uint8_t *, size_t)
{
	++root_callbacks;
	event_boundary(actor);
	check(committed == (applied && !never) && error == (never   ? unsigned(EIO) :
							    applied ? 0u :
								      unsigned(EINVAL)),
	      "business completion preserves original outcome and error");
	business_action("completion");
	if (scenario.find("completion_throw") != std::string::npos ||
	    scenario.find("both_throw") != std::string::npos)
		throw std::runtime_error("controlled completion failure");
}
static craft_progression_publication_result
progression_publish(const critical_operation_id &operation, P_char actor,
		    const craft_recipe_continuation &terms)
{
	++progression_publish_calls;
	return real_publish(operation, actor, terms);
}
static void progression_acknowledged(const critical_operation_id &operation)
{
	++hook_acks;
	check(operation.bytes == original.bytes && root_ack_successes == 1 &&
		      original_records() == 0 &&
		      !item_movement_transaction_player_busy(current_actor),
	      "actual progression cleanup follows original durable ACK and domain extraction");
	real_acknowledged(operation);
}
static void progression_notify(P_char actor, bool committed, const craft_recipe_continuation &terms)
{
	++root_notices;
	event_boundary(actor);
	real_notify(actor, committed, terms);
	business_action("notify");
	if (scenario.find("notify_throw") != std::string::npos ||
	    scenario.find("both_throw") != std::string::npos)
		throw std::runtime_error("controlled recipe notice failure");
}
static void exercise_receipt_guard(const critical_completion &completion, bool waiting)
{
	const int callbacks_before = root_callbacks, notices_before = root_notices,
		  physical_before = extractions, placed_before = placements,
		  notches_before = notches, xp_before = xp, saves_before = saves,
		  acks_before = root_acks;
	critical_completion changed = completion;
	if (scenario.find("equivalent") != std::string::npos)
	{
		changed.outcome = critical_apply_outcome::already_applied;
		++changed.attempt;
		++changed.queued_at_usec;
		++changed.started_at_usec;
		++changed.completed_at_usec;
		handle(&changed, 1);
		check(root_ack_successes == 1 && original_records() == 0 &&
			      extractions == physical_before && placements == placed_before &&
			      notches == notches_before && xp == xp_before && saves == saves_before,
		      "equivalent applied receipt with changed timing/attempt completes original root without repeated effects");
		return;
	}
	if (scenario.find("malformed") != std::string::npos)
		changed.result_size = 1;
	else if (scenario.find("changed_failure") != std::string::npos)
	{
		changed.outcome = critical_apply_outcome::terminal_failure;
		changed.error_code = EINVAL;
	}
	else
	{
		item_transfer_result different = {};
		assert(item_transfer_command_decode_result(completion.result_payload.data(),
							   completion.result_size, &different));
		++different.max_item_revision;
		std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> bytes = {};
		assert(item_transfer_command_encode_result(different, &bytes));
		std::copy(bytes.begin(), bytes.end(), changed.result_payload.begin());
	}
	if (scenario.find("batch") != std::string::npos)
	{
		const critical_completion contradictory[] = { completion, changed, completion };
		handle(contradictory, 3);
	}
	else
		handle(&changed, 1);
	handle();
	ready();
	handle();
	check(root_acks == acks_before && root_ack_successes == 0 && original_records() == 1 &&
		      item_movement_transaction_health_copy().pending == 1 &&
		      item_movement_transaction_player_busy(current_actor) &&
		      critical_command_coordinator_is_fenced({ critical_entity_type::item, 5001 },
							     nullptr),
	      "malformed or changed semantic receipt cannot ACK/release original owner across null pulse/reconnect or same-batch repair");
	check(root_callbacks == callbacks_before && root_notices == notices_before &&
		      extractions == physical_before && placements == placed_before &&
		      notches == notches_before && xp == xp_before && saves == saves_before,
	      "semantic receipt conflict preserves original physical and progression effects without notifications");
	physical_boundary();
	if (waiting)
		sync_blocked = true;
	handle(&completion, 1);
	if (waiting)
		check(root_ack_successes == 0 && original_records() == 1 &&
			      item_movement_transaction_health_copy().pending == 1 &&
			      notches == notches_before && xp == xp_before && saves == saves_before,
		      "exact original receipt repair retains pending progression save without repeated award or early ACK");
}
int main(int argc, char **argv)
{
	assert(argc == 3 && !std::filesystem::exists(argv[2]));
	fixture_check_runtime_identity_retirement();
	scenario = argv[1];
	recipe = scenario.starts_with("recipe_");
	never = scenario.find("never_admitted") != std::string::npos;
	applied = scenario.find("rejected") == std::string::npos;
	index_data indexes[1] = {};
	indexes[0].virtual_number = 42;
	obj_index = indexes;
	player.pid = 1001;
	original_actor = std::make_unique<char_data>();
	original_actor->only.pc = &player;
	original_actor->runtime_id = 7001;
	original_actor->in_room = -1;
	current_actor = original_actor.get();
	fixture_register_character(current_actor);
	character_list = current_actor;
	obj_data input = {}, output = {};
	input.obj_uid = 5001;
	input.R_num = 0;
	input.loc_p = LOC_CARRIED;
	input.loc.carrying = current_actor;
	output.obj_uid = 5030;
	output.R_num = 0;
	output.loc_p = LOC_NOWHERE;
	output.next = &input;
	object_list = &output;
	original_input = &input;
	original_output = &output;
	custody[5001] = { 5001, 5001, 0,  { item_owner_type::player, 1001, 0 },
			  1,	1,    42, item_custody_state::active };
	craft_progression_initialize();
	real_publish = craft_progression_hooks.publish;
	real_acknowledged = craft_progression_hooks.acknowledged;
	real_notify = craft_progression_hooks.notify;
	craft_progression_hooks.publish = progression_publish;
	craft_progression_hooks.acknowledged = progression_acknowledged;
	craft_progression_hooks.notify = progression_notify;
	assert(!economic_gameplay_authority::active());
	assert(critical_command_coordinator_init(argv[2], execute_craft, nullptr, 1, nullptr,
						 nullptr,
						 item_transfer_accounting_command_supported));
	if (never)
		write_fault = 1;
	P_obj inputs[] = { &input }, outputs[] = { &output };
	craft_recipe_continuation terms;
	terms.player_pid = 1001;
	terms.experience = 7000;
	terms.recipe_vnum = 42;
	terms.output_uid = 5030;
	item_movement_reject reject = item_movement_reject::none;
	assert(item_movement_transaction_submit_craft(
		current_actor, inputs, 1, outputs, 1, recipe ? 42 : 551, completion_callback,
		nullptr, 0, &reject, nullptr, nullptr, 0, chaos_pouch_usage_mode::generated,
		recipe ? &terms : nullptr));
	assert(critical_command_coordinator_is_fenced({ critical_entity_type::item, 5001 },
						      &original));
	const auto completion = native_completion();
	assert(completion.operation_id.bytes == original.bytes);
	if (scenario.find("ack_exception") != std::string::npos)
		ack_boundary_faults = 1;
	if (never)
	{
		assert(completion.disposition == critical_completion_disposition::never_admitted &&
		       critical_completion_disposition_valid(completion));
		assert(!apply_calls && completion.error_code == EIO && original_records() == 0);
		handle(&completion, 1);
		check(root_acks == 0 && !notches && !xp && !saves && !hook_acks &&
			      !progression_publish_calls,
		      "definite never-admission needs no ACK or progression publication/cleanup");
		check(input.obj_uid == 5001 && output.obj_uid == 0 && extractions == 1,
		      "never-admitted craft disposes only detached staged output");
	}
	else
	{
		const player_craft_receipt_snapshot conflict_probe = { original, 2, 7001 };
		sync_blocked = true;
		handle(&completion, 1);
		if (recipe && applied)
		{
			std::vector<player_craft_receipt_snapshot> receipts;
			assert(craft_progression_hooks.pending(1001, &receipts) &&
			       receipts.size() == 1);
			check(notches == 1 && xp == 7000 && saves == 1,
			      "actual progression applies once while exact save receipt is waiting");
			if (scenario.find("progression_wait") != std::string::npos)
			{
				for (int pulse = 0; pulse < 3; ++pulse)
					handle();
				check(!root_callbacks && !root_notices && root_acks == 0 &&
					      notches == 1 && xp == 7000,
				      "unacknowledged actual progression retains original root without repeated effects");
			}
			if (scenario.find("waiting_receipt") != std::string::npos)
			{
				sync_blocked = false;
				exercise_receipt_guard(completion, true);
			}
			craft_progression_hooks.saved(1001, true, receipts.data(), receipts.size());
			ready();
		}
		for (int pulse = 0; pulse < 3; ++pulse)
			handle();
		check(real_ack_calls >= 1 && real_ack_false >= 1,
		      "blocked real journal fsync causes actual delegated ACK calls returning false");
		check(!root_callbacks && !root_notices,
		      "repeated real journal checkpoint failures cannot announce or complete craft");
		physical_boundary();
		if (recipe && applied)
			check(!craft_progression_hooks.recover(1001, &conflict_probe, 1),
			      "controlled conflicting saved receipt is refused while original real progression attempt exists");
		check(root_ack_successes == 0 && original_records() == 1 &&
			      item_movement_transaction_health_copy().pending == 1,
		      "failed ACK retains original durable journal fence and domain owner");
		const int physical_before = extractions, placements_before = placements;
		sync_blocked = false;
		if (scenario.find("pending_receipt") != std::string::npos)
			exercise_receipt_guard(completion, false);
		if (scenario.find("actor_absent") != std::string::npos)
		{
			fixture_retire_character(current_actor);
			assert(character_list == current_actor &&
			       current_actor->runtime_id == 7001 &&
			       !find_character_by_runtime_id(current_actor->runtime_id));
			handle();
			ready();
			check(original_records() == 1 && root_ack_successes == 0 &&
				      item_movement_transaction_health_copy().pending == 1 &&
				      item_movement_transaction_player_busy(current_actor) &&
				      critical_command_coordinator_is_fenced(
					      { critical_entity_type::item, 5001 }, nullptr) &&
				      !root_callbacks && !root_notices,
			      "retired still-linked allocated actor cannot ACK or notify without native registry membership");
			character_list = nullptr;
			handle();
			check(original_records() == 1 && !root_callbacks && !root_notices,
			      "absent actor retains original root until authoritative reconnect");
			current_actor->runtime_id = 9002;
			fixture_register_character(current_actor);
			character_list = current_actor;
		}
		handle();
		check(root_ack_successes == 1 && original_records() == 0,
		      "same original operation reaches one durable publication ACK after repair");
		check(extractions == physical_before && placements == placements_before,
		      "ACK repair does not repeat physical input retirement or output placement");
		check(!recipe || hook_acks == 1,
		      "real recipe acknowledged cleanup runs once for applied and rejected original ACK");
		if (recipe && applied)
			check(craft_progression_hooks.recover(1001, &conflict_probe, 1),
			      "controlled no-effect conflicting saved receipt proves original real progression attempt was erased");
		if (recipe && !applied)
			check(!progression_publish_calls && !notches && !xp && !saves,
			      "ordinary rejected recipe performs no progression publication, awards or save request");
		check(notches == (recipe && applied ? 1 : 0) &&
			      xp == (recipe && applied ? 7000 : 0),
		      "progression effects remain once across ACK retries");
	}
	const int callbacks_settled = root_callbacks, notices_settled = root_notices,
		  acks_settled = root_acks, physical_settled = extractions;
	handle(&completion, 1);
	handle();
	handle();
	const int expected_callbacks = scenario == "recipe_notify_retire_actor" ? 0 : 1;
	check(root_callbacks == expected_callbacks && callbacks_settled == expected_callbacks,
	      "business completion occurs once for a surviving registered actor and is skipped after notification retires it");
	if (scenario.find("reentrant") != std::string::npos)
	{
		check(new_actors.size() == 64 &&
			      item_movement_transaction_health_copy().pending == 64,
		      "all 64 reentrant domain owners survive original owner finalization");
		for (const auto &actor : new_actors)
			check(item_movement_transaction_player_busy(actor.get()),
			      "each distinct reentrant actor remains busy on its admitted owner");
	}
	check(!recipe || (root_notices == 1 && notices_settled == 1),
	      "recipe notice is invoked exactly once");
	check(root_acks == acks_settled && extractions == physical_settled,
	      "duplicate receipt and null pulses cannot repeat ACK or physical disposal");
	check(!callback_before_ack && !escaped,
	      "post-ACK business hooks are independently contained and never escape to publication retry");
	check(ack_exceptions == (scenario.find("ack_exception") != std::string::npos ? 1 : 0),
	      "controlled ACK boundary exception is observed once while original real journal root remains recoverable");
	check(!restores,
	      "no fixture materialization fallback silently replaces original world graph");
	assert(critical_command_coordinator_shutdown());
	item_movement_transaction_reset_for_tests();
	for (auto &actor : new_actors)
		fixture_retire_character(actor.get());
	fixture_retire_character(current_actor);
	character_list = nullptr;
	craft_progression_initialize();
	printf("OBSERVE callbacks=%d notices=%d ack_attempts=%d real_ack_calls=%d real_ack_false=%d ack_successes=%d recipe_cleanup=%d progression_calls=%d notches=%d xp=%d saves=%d physical=%d placed=%d escaped=%d failures=%d\n",
	       root_callbacks, root_notices, root_acks, real_ack_calls, real_ack_false,
	       root_ack_successes, hook_acks, progression_publish_calls, notches, xp, saves,
	       extractions, placements, escaped, failures);
	return failures ? 1 : 0;
}
'''


def harness(root):
    leaves = literal(root, "test_publication_retention_runtime.py")
    leaves = leaves[:leaves.index("critical_apply_result apply_transfer(")]
    leaves = leaves[:leaves.index("bool economic_gameplay_authority::active()")] + \
        leaves[leaves.index("bool collector_death_enrollment_attach("):]
    start = leaves.index("static bool recipe_progression_ready")
    end = leaves.index("void *__malloc", start)
    leaves = leaves[:start] + leaves[end:]
    start = leaves.index("void craft_callback(")
    end = leaves.index("bool item_ownership_runtime_lookup(", start)
    leaves = leaves[:start] + leaves[end:]
    start = leaves.index("bool item_ownership_runtime_lookup(")
    end = leaves.index("bool item_ownership_runtime_owner_revision(", start)
    leaves = leaves[:start] + leaves[end:]
    start = leaves.index("bool item_ownership_runtime_apply(")
    end = leaves.index("bool currency_transaction_coin_item_busy(", start)
    leaves = leaves[:start] + leaves[end:]
    for line in (
        "static item_ownership_runtime_entry runtime_entry = {};",
        "static item_ownership_runtime_entry pouch_runtime = {};",
        "static int publication_attempts = 0;", "static int completion_calls = 0;",
        "static int craft_callbacks = 0;", "static bool accounting_active = false;",
        "static critical_apply_outcome forced_outcome = critical_apply_outcome::applied;",
        "static uint16_t expected_craft_inputs = 1;",
    ):
        leaves = leaves.replace(line, "")
    leaves = leaves.replace("extern const int top_of_world = 1;", "int top_of_world = 1;")
    leaves = leaves.replace("void obj_to_char(P_obj object, P_char actor)", "extern int placements;\nvoid obj_to_char(P_obj object, P_char actor)")
    leaves = leaves.replace("object->loc_p = LOC_CARRIED;", "++placements;\n    object->loc_p = LOC_CARRIED;", 1)
    faults = literal(root, "test_critical_admission_owner_release.py", "FAULTS")
    faults = faults[:faults.index("static size_t replay_count()")]
    return leaves + '\n#include "persistence/critical_command_journal.h"\n#include <cerrno>\n' + faults + NATIVE


def bounded(command, seconds, **kwargs):
    process = subprocess.Popen(command, start_new_session=True, **kwargs)
    try:
        stdout, stderr = process.communicate(timeout=seconds)
    except BaseException:
        try:
            os.killpg(process.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
        try:
            process.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            process.communicate()
        raise
    return subprocess.CompletedProcess(command, process.returncode, stdout, stderr)


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_inputs(root):
    pins = {str(path.relative_to(root)): sha256(path)
            for path in sorted((root / "src").rglob("*"))
            if path.is_file() and path.suffix in (".c", ".h", ".hpp", ".cpp")}
    for name in ("test_publication_retention_runtime.py", "test_critical_admission_owner_release.py",
                 "character_identity_test_fixture.h"):
        path = root / "tests/async" / name
        pins[str(path.relative_to(root))] = sha256(path)
    pins["assigned_runner"] = sha256(Path(__file__))
    return pins


def stable_inputs(root, expected, source, native_hash):
    if source_inputs(root) != expected or sha256(source) != native_hash:
        raise RuntimeError("native source/fixture/runner/harness inputs changed during qualification")


def supplied_artifact(binary, declared_sha, backend, pins, native_hash, root):
    metadata_path = binary.with_suffix(".json")
    metadata = json.loads(metadata_path.read_text())
    if sha256(binary) != declared_sha or metadata.get("binary_sha256") != declared_sha:
        raise RuntimeError("supplied binary SHA256 or adjacent metadata mismatch")
    if (metadata.get("backend") != backend or metadata.get("harness_sha256") != native_hash or
            metadata.get("source_inputs") != pins or metadata.get("source_inputs_after") != pins or
            metadata.get("linked_sources") != list(SOURCES) or metadata.get("scenarios") != list(SCENARIOS)):
        raise RuntimeError("supplied artifact backend/harness/source/fixture identity mismatch")
    if (metadata.get("qualification") != "compiled-stable-inputs" or
            metadata.get("compile_budget_seconds") != 300 or
            not 0 <= metadata.get("compile_seconds", -1) < 300 or
            not isinstance(metadata.get("compile_link_argv"), list) or
            not metadata["compile_link_argv"] or metadata.get("compile_cwd") != str(root) or metadata.get("source_root") != str(root)):
        raise RuntimeError("supplied artifact compile qualification metadata missing or invalid")
    return metadata


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=ROOT)
    parser.add_argument("--flatfile", action="store_true")
    parser.add_argument("--compile-only", type=Path)
    parser.add_argument("--run-binary", type=Path)
    parser.add_argument("--binary-sha256")
    args = parser.parse_args()
    if args.compile_only and args.run_binary:
        parser.error("select one supplied artifact mode")
    if args.run_binary and not args.binary_sha256:
        parser.error("supplied binary requires SHA256")
    root = args.source_root.resolve()
    backend = "flatfile" if args.flatfile else "sql-header"
    with tempfile.TemporaryDirectory(prefix="duris-craft-ack-") as temporary:
        private = Path(temporary)
        source = private / "craft.cpp"
        pins = source_inputs(root)
        native = harness(root)
        source.write_text(native)
        native_hash = hashlib.sha256(native.encode()).hexdigest()
        stable_inputs(root, pins, source, native_hash)
        binary = (args.run_binary or args.compile_only or private / "craft").resolve()
        if not args.run_binary:
            if args.compile_only and (binary.exists() or binary.with_suffix(".json").exists()):
                raise RuntimeError("unique artifact already exists")
            cflags = shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
            libraries = shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
            command = [*shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-g", "-Og",
                       "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread",
                       "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                       "-ffunction-sections", "-fdata-sections",
                       *(["-D__NO_MYSQL__", "-Isrc/no_mysql"] if args.flatfile else []),
                       "-Isrc", "-Itests/async", *cflags, str(source),
                       *[str(root / "src" / name) for name in SOURCES], "-Wl,--gc-sections", "-lz", "-lcrypto",
                       "-Wl,--wrap=write", "-Wl,--wrap=fsync", "-Wl,--wrap=" + ACK_SYMBOL,
                       *libraries, "-o", str(binary)]
            stable_inputs(root, pins, source, native_hash)
            started = time.monotonic()
            try:
                result = bounded(command, 300, cwd=root)
                elapsed = time.monotonic() - started
                if result.returncode:
                    raise subprocess.CalledProcessError(result.returncode, command)
                stable_inputs(root, pins, source, native_hash)
            except BaseException:
                if binary.exists():
                    binary.unlink()
                raise
            if args.compile_only:
                metadata = {"qualification": "compiled-stable-inputs", "binary_sha256": sha256(binary),
                            "compile_seconds": elapsed, "compile_budget_seconds": 300,
                            "source_root": str(root), "source_inputs": pins,
                            "source_inputs_after": source_inputs(root),
                            "harness_sha256": native_hash, "linked_sources": list(SOURCES),
                            "compile_link_argv": command, "compile_cwd": str(root),
                            "backend": backend, "scenarios": list(SCENARIOS),
                            "scope": "21 actual native units: journal/coordinator/item owner/craft progression; controlled execution/custody/graph capture/world/skills/save leaves; no services; matching saved-hook injection is not persistence completion"}
                binary.with_suffix(".json").write_text(json.dumps(metadata, sort_keys=True, indent=2) + "\n")
                print(json.dumps({key: metadata[key] for key in ("binary_sha256", "compile_seconds", "compile_budget_seconds")}))
                return
        else:
            supplied_artifact(binary, args.binary_sha256, backend, pins, native_hash, root)
        failed = []
        for scenario in SCENARIOS:
            stable_inputs(root, pins, source, native_hash)
            result = bounded([str(binary), scenario, str(private / scenario)], 30, text=True,
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            print(f"{scenario}: {'PASS' if result.returncode == 0 else 'FAIL'}\n{result.stdout}{result.stderr}", flush=True)
            if result.returncode:
                failed.append(scenario)
        stable_inputs(root, pins, source, native_hash)
        if args.run_binary:
            supplied_artifact(binary, args.binary_sha256, backend, pins, native_hash, root)
        if failed:
            raise AssertionError("Craft publication ACK regression failed: " + ", ".join(failed))


if __name__ == "__main__":
    main()

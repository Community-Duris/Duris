// Executes the actual extracted Item policy with typed boundary fixtures.
// The fixtures do not implement or qualify SQL, journal durability or ACK locks.
#include "item/held_retirement_publication_internal.h"
#include "cmd/lockpick_retirement.h"
#include "item/item_ownership_runtime.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "core/prototypes.h"
#include "core/utils.h"

#include <cassert>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

P_char character_list = nullptr;
P_obj object_list = nullptr;
P_index obj_index = nullptr;

namespace
{
pc_only_data player{};
char_data actor{};
obj_data pick{};
index_data index_entry{};
item_transfer_payload payload{};
item_transfer_result result{};
lockpick_retirement_terms terms{};
critical_native_recovery_envelope retained{};
std::map<uint8_t, held_retirement_recovery> contexts;
uint8_t next_context = 0;
std::vector<std::string> trace;
bool ack_allowed = true, repository_allowed = true, contradictory_final = false;
bool runtime_registered = true;
int failed_stage = -1;
unsigned effects = 0, notices = 0, physical_calls = 0, observations = 0, acks = 0;
held_retirement_publication_mode mode = held_retirement_publication_mode::execution;

const held_retirement_recovery &context_of(const critical_native_recovery_envelope &envelope)
{
	assert(envelope.attachment.size() == 1);
	return contexts.at(envelope.attachment[0]);
}

void reset(item_movement_detail::pending_movement &entry, bool committed = true)
{
	player = {};
	player.pid = 1001;
	actor = {};
	actor.only.pc = &player;
	actor.runtime_id = 7001;
	pick = {};
	pick.obj_uid = 5001;
	pick.R_num = 0;
	pick.type = ITEM_PICK;
	pick.loc_p = LOC_WORN;
	pick.loc.wearing = &actor;
	actor.equipment[HOLD] = &pick;
	index_entry = {};
	index_entry.virtual_number = 42;
	obj_index = &index_entry;
	character_list = &actor;
	object_list = &pick;
	runtime_registered = true;
	payload = {};
	payload.from_owner = { item_owner_type::player, 1001, 0 };
	payload.to_owner = { item_owner_type::destruction, 0, 0 };
	payload.item_count = 1;
	payload.items[0].expected_item_revision = 1;
	payload.expected_from_revision = 10;
	payload.expected_to_revision = 11;
	result = {};
	result.root_item_uid = 5001;
	result.item_count = 1;
	result.max_item_revision = 2;
	result.from_owner_revision = 10;
	result.to_owner_revision = 11;
	terms = { lockpick_retirement_branch::wear, 5001, 1001, 42 };
	retained = {};
	retained.command.operation_id.bytes[0] = 1;
	retained.revision = 1;
	contexts.clear();
	next_context = 0;
	held_retirement_recovery recovery;
	recovery.save_revision = 7;
	recovery.before.resize(1);
	assert(held_retirement_recovery_encode(retained.command, recovery, &retained.attachment));
	entry = {};
	entry.held_command = std::make_shared<const critical_command>(retained.command);
	entry.completed.operation_id = retained.command.operation_id;
	entry.completed.outcome = committed ? critical_apply_outcome::applied :
					      critical_apply_outcome::terminal_failure;
	entry.completed.durable_revision = 8;
	entry.completed.result_size = committed ? 1 : 0;
	entry.completed.result_payload[0] = committed ? 99 : 0;
	trace.clear();
	ack_allowed = repository_allowed = true;
	contradictory_final = false;
	failed_stage = -1;
	effects = notices = physical_calls = observations = acks = 0;
	mode = held_retirement_publication_mode::execution;
}
}

P_char find_character_by_runtime_id(uint64_t id)
{
	return runtime_registered && id == actor.runtime_id ? &actor : nullptr;
}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	std::abort();
}

player_snapshot_codec_result
player_item_snapshot_list_encode(const std::vector<player_item_snapshot> &items,
				 std::vector<uint8_t> *output)
{
	// This fixture has exactly two forest values, before(one) and after(empty).
	// It deliberately does not stand in for a wire-codec regression.
	assert(items.size() <= 1 && output);
	*output = { static_cast<uint8_t>(items.size()) };
	return player_snapshot_codec_result::ok;
}
player_snapshot_capture_result
player_snapshot_capture_literal_inventory(P_char ch, player_revision_t, player_component_mask_t,
					  int, int, uint64_t, player_snapshot *output)
{
	assert(ch == &actor && output);
	output->items = object_list ? std::vector<player_item_snapshot>(1) :
				      std::vector<player_item_snapshot>{};
	return player_snapshot_capture_result::ok;
}
bool held_retirement_command_identity(const critical_command &command, item_transfer_payload *out,
				      lockpick_retirement_terms *selected) noexcept
{
	if (command.operation_id.bytes != retained.command.operation_id.bytes)
		return false;
	if (out)
		*out = payload;
	if (selected)
		*selected = terms;
	return true;
}
bool held_retirement_recovery_encode(const critical_command &,
				     const held_retirement_recovery &value,
				     std::vector<uint8_t> *output) noexcept
{
	assert(next_context < 250 && output);
	contexts.emplace(++next_context, value);
	*output = { next_context };
	return true;
}
bool held_retirement_recovery_decode(const critical_command &, std::span<const uint8_t> wire,
				     held_retirement_recovery *output) noexcept
{
	if (!output || wire.size() != 1 || !contexts.contains(wire[0]))
		return false;
	*output = contexts.at(wire[0]);
	return true;
}
bool held_retirement_recovery_receipt_matches(const held_retirement_receipt &receipt,
					      const critical_completion &completion) noexcept
{
	return receipt.outcome == completion.outcome &&
	       receipt.durable_revision == completion.durable_revision &&
	       receipt.error_code == completion.error_code &&
	       receipt.failure_stage == completion.failure_stage &&
	       receipt.result_size == completion.result_size &&
	       receipt.result == completion.result_payload;
}
bool held_retirement_publication_snapshot_valid(
	const critical_command &command, const critical_completion &completion,
	const held_retirement_publication_snapshot &snapshot) noexcept
{
	return command.operation_id.bytes == completion.operation_id.bytes &&
	       command.operation_id.bytes == snapshot.envelope.command.operation_id.bytes &&
	       snapshot.envelope.revision != 0;
}
bool item_transfer_command_decode_result(const uint8_t *bytes, size_t size,
					 item_transfer_result *output)
{
	if (!output || size != 1 || bytes[0] != 99)
		return false;
	*output = result;
	return true;
}
bool item_owner_identity_equal(const item_owner_identity &a, const item_owner_identity &b)
{
	return a.type == b.type && a.id == b.id && a.context_id == b.context_id;
}
bool held_retirement_repository_observe_current(const critical_command &,
						const critical_completion &completion,
						const held_retirement_recovery &context,
						held_retirement_current_observation *output) noexcept
{
	if (!repository_allowed || !output)
		return false;
	++observations;
	const bool committed = completion.disposition ==
				       critical_completion_disposition::execution &&
			       (completion.outcome == critical_apply_outcome::applied ||
				completion.outcome == critical_apply_outcome::already_applied);
	*output = {};
	output->rollback_confirmed = true;
	output->save_revision = context.save_revision;
	output->items = committed ? context.after : context.before;
	output->selected.item_uid = output->selected.root_item_uid = terms.item_uid;
	output->selected.vnum = terms.item_vnum;
	output->selected.owner = committed ? payload.to_owner : payload.from_owner;
	output->selected.state = committed ? item_custody_state::destroyed :
					     item_custody_state::active;
	output->selected.item_revision = committed ? result.max_item_revision :
						     payload.items[0].expected_item_revision;
	output->selected.owner_revision = 11;
	output->from_owner_revision = 10;
	output->to_owner_revision = 11;
	if (contradictory_final && observations == 3)
		++output->selected.item_revision;
	return true;
}
bool item_ownership_runtime_hydrate_owner(const item_owner_identity &, uint64_t)
{
	return true;
}
bool item_ownership_runtime_hydrate_many_atomic(const item_ownership_runtime_entry *, size_t count)
{
	assert(count == 1);
	return true;
}
bool lockpick_retirement_publish_physical(const critical_operation_id &, P_char ch, bool committed,
					  const item_transfer_result &,
					  const lockpick_retirement_terms &, bool notify) noexcept
{
	assert(ch == &actor);
	++physical_calls;
	trace.emplace_back(notify ? "physical:notice" : "physical:resume");
	if (notify)
		++notices;
	if (committed && object_list)
	{
		object_list = nullptr;
		actor.equipment[HOLD] = nullptr;
		++effects;
	}
	return true;
}
bool player_save_held_retirement_publication_owner::copy_publication_context(
	const critical_command &, const critical_completion &,
	held_retirement_publication_snapshot *output) noexcept
{
	output->envelope = retained;
	output->mode = mode;
	return true;
}
bool player_save_held_retirement_publication_owner::rebind_recovery_checkpoint(
	const critical_native_recovery_envelope &envelope, uint64_t runtime) noexcept
{
	return runtime == actor.runtime_id &&
	       envelope.command.operation_id.bytes == retained.command.operation_id.bytes;
}
bool player_save_held_retirement_publication_owner::checkpoint_recovery_context(
	const critical_native_recovery_envelope &expected,
	const critical_native_recovery_envelope &successor) noexcept
{
	assert(expected.revision == retained.revision &&
	       successor.revision == expected.revision + 1);
	const auto &next = context_of(successor);
	trace.emplace_back(next.physical_stage == 0 ? "checkpoint:receipt" :
			   next.physical_stage == 1 ? "checkpoint:stage1" :
						      "checkpoint:stage2");
	if (failed_stage == next.physical_stage)
	{
		failed_stage = -1;
		return false;
	}
	retained = successor;
	return true;
}
bool player_save_held_retirement_publication_owner::publish_held_retirement(
	const critical_command &command, const critical_completion &completion,
	const held_retirement_publication_snapshot &,
	bool (*publish)(const critical_command &, const critical_completion &, void *) noexcept,
	void *context) noexcept
{
	if (!publish(command, completion, context))
		return false;
	if (completion.disposition == critical_completion_disposition::never_admitted)
	{
		trace.emplace_back("cleanup:refusal");
		return true;
	}
	trace.emplace_back("ack");
	++acks;
	return ack_allowed;
}

int main()
{
	item_movement_detail::pending_movement entry{};
#ifdef __NO_MYSQL__
	reset(entry);
	assert(!item_held_retirement_publication_owner::publish(entry));
	assert(trace.empty() && effects == 0 && acks == 0);
#else
	reset(entry);
	assert(item_held_retirement_publication_owner::publish(entry));
	assert((trace == std::vector<std::string>{ "checkpoint:receipt", "checkpoint:stage1",
						   "physical:notice", "checkpoint:stage2",
						   "ack" }));
	assert(context_of(retained).physical_stage == 2 && effects == 1 && notices == 1);
	assert(entry.registry_applied && entry.ordinary_publication_ready &&
	       entry.actor_runtime_id == 7001);
	reset(entry);
	failed_stage = 0;
	assert(!item_held_retirement_publication_owner::publish(entry));
	assert(!context_of(retained).receipt.present && !effects && !acks);
	assert(item_held_retirement_publication_owner::publish(entry) && effects == 1 &&
	       notices == 1);
	reset(entry);
	failed_stage = 1;
	assert(!item_held_retirement_publication_owner::publish(entry));
	assert(context_of(retained).receipt.present && context_of(retained).physical_stage == 0 &&
	       !effects);
	assert(item_held_retirement_publication_owner::publish(entry) && effects == 1 &&
	       notices == 1);
	reset(entry);
	failed_stage = 2;
	assert(!item_held_retirement_publication_owner::publish(entry));
	assert(context_of(retained).physical_stage == 1 && effects == 1 && notices == 1 && !acks);
	assert(item_held_retirement_publication_owner::publish(entry));
	assert(context_of(retained).physical_stage == 2 && effects == 1 && notices == 1 &&
	       physical_calls == 2);
	reset(entry);
	ack_allowed = false;
	assert(!item_held_retirement_publication_owner::publish(entry));
	assert(context_of(retained).physical_stage == 2 && effects == 1 && notices == 1 &&
	       acks == 1);
	ack_allowed = true;
	mode = held_retirement_publication_mode::terminal_ack_retry;
	assert(item_held_retirement_publication_owner::publish(entry));
	assert(effects == 1 && notices == 1 && physical_calls == 1 && acks == 2);
	reset(entry);
	mode = held_retirement_publication_mode::terminal_ack_retry;
	assert(!item_held_retirement_publication_owner::publish(entry) && !effects && !acks);
	reset(entry);
	character_list = nullptr;
	assert(!item_held_retirement_publication_owner::publish(entry) && trace.empty());
	character_list = &actor;
	actor.runtime_id = 7002;
	assert(item_held_retirement_publication_owner::publish(entry) &&
	       entry.actor_runtime_id == 7002);
	reset(entry);
	runtime_registered = false;
	assert(!item_held_retirement_publication_owner::publish(entry) && trace.empty());
	reset(entry);
	char_data duplicate = actor;
	actor.next = &duplicate;
	// Only one live runtime-index match: the retained duplicate list pointer is not authority.
	assert(item_held_retirement_publication_owner::publish(entry));
	reset(entry);
	auto bad = context_of(retained);
	bad.receipt.present = true;
	bad.receipt.result_size = 2;
	assert(held_retirement_recovery_encode(retained.command, bad, &retained.attachment));
	assert(!item_held_retirement_publication_owner::publish(entry) && trace.empty());
	reset(entry);
	repository_allowed = false;
	assert(!item_held_retirement_publication_owner::publish(entry) && !effects);
	reset(entry);
	contradictory_final = true;
	assert(!item_held_retirement_publication_owner::publish(entry) && effects == 1 && !acks);
	contradictory_final = false;
	assert(item_held_retirement_publication_owner::publish(entry) && effects == 1 &&
	       notices == 1);
	reset(entry, false);
	assert(item_held_retirement_publication_owner::publish(entry) && effects == 0 &&
	       notices == 1);
	assert(object_list == &pick && context_of(retained).physical_stage == 2 && acks == 1);
	reset(entry, false);
	entry.completed.disposition = critical_completion_disposition::never_admitted;
	entry.completed.durable_revision = 0;
	entry.completed.error_code = 1;
	assert(item_held_retirement_publication_owner::publish(entry));
	assert((trace == std::vector<std::string>{ "cleanup:refusal" }) && !effects && !notices &&
	       !acks);
	assert(!context_of(retained).receipt.present && context_of(retained).physical_stage == 0);
#endif
}

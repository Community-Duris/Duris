#include "item_movement_transaction.h"

#include "item_ownership_runtime.h"
#include "necromancy.h"
#include "player_snapshot_capture.h"
#include "player_snapshot_codec.h"
#include "prototypes.h"
#include "utils.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <new>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

extern P_obj object_list;
extern const int top_of_world;

namespace
{
struct pending_movement
{
	uint32_t actor_pid;
	item_transfer_payload payload;
	item_owner_identity requested_to_owner;
	uint64_t requested_target_parent_uid;
	item_transfer_reason requested_reason;
	int64_t requested_reason_id;
	uint64_t requested_corpse_uid;
	bool adopting;
	item_movement_completion_fn completion;
	std::array<uint8_t, ITEM_MOVEMENT_CONTEXT_MAX_BYTES> context;
	size_t context_size;
	bool completion_ready;
	critical_completion completed;
};

std::unordered_map<std::string, pending_movement> pending;
item_movement_health health = {};

std::string operation_key(const critical_operation_id &operation_id)
{
	return std::string(reinterpret_cast<const char *>(operation_id.bytes.data()),
			   operation_id.bytes.size());
}

bool owner_conflicts(const pending_movement &entry, const item_owner_identity &owner)
{
	return item_owner_identity_equal(entry.payload.from_owner, owner) ||
	       item_owner_identity_equal(entry.payload.to_owner, owner) ||
	       (entry.adopting && item_owner_identity_equal(entry.requested_to_owner, owner));
}

bool movement_conflicts(const item_owner_identity &from_owner, const item_owner_identity &to_owner)
{
	return std::any_of(pending.begin(), pending.end(),
			   [&](const auto &entry) {
				   return owner_conflicts(entry.second, from_owner) ||
					  owner_conflicts(entry.second, to_owner);
			   });
}

bool capture(P_obj object, uint64_t root_uid, uint64_t parent_uid,
	     std::vector<item_transfer_entry> *items)
{
	if (!object || !items || !object->obj_uid || items->size() >= ITEM_TRANSFER_MAX_ITEMS)
		return false;
	item_ownership_runtime_entry runtime = {};
	if (!item_ownership_runtime_lookup(object->obj_uid, &runtime) ||
	    runtime.root_item_uid != root_uid || runtime.parent_item_uid != parent_uid ||
	    runtime.vnum != OBJ_VNUM(object) || runtime.state != item_custody_state::active)
		return false;
	items->push_back({ runtime.item_uid, runtime.root_item_uid, runtime.parent_item_uid,
			   runtime.item_revision, runtime.vnum, runtime.state });
	for (P_obj child = object->contains; child; child = child->next_content)
		if (!capture(child, root_uid, object->obj_uid, items))
			return false;
	return true;
}

bool capture_absent(P_obj object, uint64_t root_uid, uint64_t parent_uid,
		    std::vector<item_transfer_entry> *items)
{
	if (!object || !items || !object->obj_uid || OBJ_VNUM(object) <= 0 ||
	    items->size() >= ITEM_TRANSFER_MAX_ITEMS)
		return false;
	item_ownership_runtime_entry existing = {};
	if (item_ownership_runtime_lookup(object->obj_uid, &existing))
		return false;
	items->push_back({ object->obj_uid, root_uid, parent_uid, ITEM_TRANSFER_ABSENT_REVISION,
			   OBJ_VNUM(object), item_custody_state::absent });
	for (P_obj child = object->contains; child; child = child->next_content)
		if (!capture_absent(child, root_uid, object->obj_uid, items))
			return false;
	return true;
}

bool capture_corpse_metadata(P_char actor, P_obj root, P_obj corpse, item_transfer_reason reason,
			     item_corpse_metadata *metadata)
{
	if (!actor || !root || !corpse || !corpse->obj_uid || !metadata ||
	    GET_ITEM_TYPE(corpse) != ITEM_CORPSE ||
	    !IS_SET(corpse->value[CORPSE_FLAGS], PC_CORPSE) || !corpse->action_description ||
	    !*corpse->action_description || corpse->value[CORPSE_PID] <= 0 ||
	    corpse->value[CORPSE_SAVEID] <= 0 ||
	    (reason != item_transfer_reason::corpse_create &&
	     reason != item_transfer_reason::corpse_loot))
		return false;
	if (reason == item_transfer_reason::corpse_create)
	{
		if (!OBJ_CARRIED_BY(root, actor))
			return false;
	}
	else
	{
		P_obj outer = root;
		while (OBJ_INSIDE(outer) && outer->loc.inside)
			outer = outer->loc.inside;
		if (outer != corpse)
			return false;
	}
	int32_t room_vnum = 0;
	if (OBJ_ROOM(corpse) && corpse->loc.room > NOWHERE && corpse->loc.room <= top_of_world)
		room_vnum = world[corpse->loc.room].number;
	else if (OBJ_CARRIED(corpse) && corpse->loc.carrying &&
		 corpse->loc.carrying->in_room > NOWHERE &&
		 corpse->loc.carrying->in_room <= top_of_world)
		room_vnum = world[corpse->loc.carrying->in_room].number;
	const int64_t weight_delta = reason == item_transfer_reason::corpse_create ?
					     static_cast<int64_t>(GET_OBJ_WEIGHT(root)) :
					     -static_cast<int64_t>(GET_OBJ_WEIGHT(root));
	const int64_t post_weight = static_cast<int64_t>(corpse->weight) + weight_delta;
	if (room_vnum < 0 || post_weight < INT32_MIN || post_weight > INT32_MAX)
		return false;
	metadata->present = true;
	metadata->room_vnum = room_vnum;
	metadata->weight = static_cast<int32_t>(post_weight);
	metadata->actor_racewar = static_cast<uint8_t>(GET_RACEWAR(actor));
	std::copy(std::begin(corpse->value), std::end(corpse->value), metadata->values.begin());
	metadata->owner_name = corpse->action_description;
	metadata->short_description = corpse->short_description ? corpse->short_description : "";
	metadata->description = corpse->description ? corpse->description : "";
	metadata->keywords = corpse->name ? corpse->name : "";
	return true;
}

P_obj find_item(uint64_t uid)
{
	for (P_obj object = object_list; object; object = object->next)
		if (object->obj_uid == uid)
			return object;
	return NULL;
}

void account_health()
{
	health.pending = pending.size();
	health.retained_offline = 0;
	for (const auto &[key, entry] : pending)
	{
		(void)key;
		if (entry.completion_ready)
			++health.retained_offline;
	}
}

void publish(std::unordered_map<std::string, pending_movement>::iterator found, P_char actor)
{
	pending_movement &entry = found->second;
	item_transfer_result result = {};
	const bool decoded = item_transfer_command_decode_result(
		entry.completed.result_payload.data(), entry.completed.result_size, &result);
	const bool committed = decoded &&
			       (entry.completed.outcome == critical_apply_outcome::applied ||
				entry.completed.outcome == critical_apply_outcome::already_applied);
	bool registry_applied = false;
	if (committed)
		registry_applied = item_ownership_runtime_apply(entry.payload, result);
	if (entry.adopting && committed && registry_applied)
	{
		const uint64_t root_uid = entry.payload.selected_item_uid;
		const item_owner_identity source = entry.payload.to_owner;
		const item_owner_identity destination = entry.requested_to_owner;
		const uint64_t target_parent_uid = entry.requested_target_parent_uid;
		const item_transfer_reason reason = entry.requested_reason;
		const int64_t reason_id = entry.requested_reason_id;
		const uint64_t corpse_uid = entry.requested_corpse_uid;
		const item_movement_completion_fn completion_fn = entry.completion;
		const size_t context_size = entry.context_size;
		const auto context = entry.context;
		pending.erase(found);
		++health.committed;
		P_obj root = find_item(root_uid);
		P_obj target_parent = target_parent_uid ? find_item(target_parent_uid) : NULL;
		P_obj corpse = corpse_uid ? find_item(corpse_uid) : NULL;
		if (!root || (target_parent_uid && !target_parent) || (corpse_uid && !corpse) ||
		    !item_movement_transaction_submit(actor, root, target_parent, source,
						      destination, reason, reason_id, completion_fn,
						      context.data(), context_size, corpse))
		{
			++health.submission_failures;
			if (completion_fn)
				completion_fn(actor, false, {}, EAGAIN, context.data(),
					      context_size);
		}
		account_health();
		return;
	}
	const item_movement_completion_fn completion_fn = entry.completion;
	const auto context = entry.context;
	const size_t context_size = entry.context_size;
	const unsigned int error_code = decoded ? entry.completed.error_code : EBADMSG;
	pending.erase(found);
	if (completion_fn)
		completion_fn(actor, committed && registry_applied, result, error_code,
			      context.data(), context_size);
	if (committed && registry_applied)
		++health.committed;
	else
	{
		++health.rejected;
		if (committed)
			++health.stale_publications;
	}
	account_health();
}
}

bool item_movement_transaction_submit(P_char actor, P_obj root, P_obj target_container,
				      const item_owner_identity &from_owner,
				      const item_owner_identity &to_owner,
				      item_transfer_reason reason, int64_t reason_id,
				      item_movement_completion_fn completion, const void *context,
				      size_t context_size, P_obj corpse_context)
{
	const bool corpse_transfer = reason == item_transfer_reason::corpse_create ||
				     reason == item_transfer_reason::corpse_loot;
	if (!actor || IS_NPC(actor) || GET_PID(actor) <= 0 || !root || !root->obj_uid ||
	    context_size > ITEM_MOVEMENT_CONTEXT_MAX_BYTES || (context_size && !context) ||
	    pending.size() >= ITEM_MOVEMENT_PENDING_MAX ||
	    movement_conflicts(from_owner, to_owner) || corpse_transfer != (corpse_context != NULL))
		return false;
	item_ownership_runtime_entry runtime = {};
	item_ownership_runtime_entry target_runtime = {};
	uint64_t from_revision = 0, to_revision = 0;
	const bool adopted = item_ownership_runtime_lookup(root->obj_uid, &runtime);
	if ((adopted && !item_owner_identity_equal(runtime.owner, from_owner)) ||
	    (target_container &&
	     (!target_container->obj_uid ||
	      !item_ownership_runtime_lookup(target_container->obj_uid, &target_runtime) ||
	      !item_owner_identity_equal(target_runtime.owner, to_owner))) ||
	    !item_ownership_runtime_owner_revision(from_owner, &from_revision) ||
	    !item_ownership_runtime_owner_revision(to_owner, &to_revision))
		return false;
	std::vector<item_transfer_entry> items;
	try
	{
		items.reserve(ITEM_TRANSFER_MAX_ITEMS);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (!(adopted ? capture(root, runtime.root_item_uid, runtime.parent_item_uid, &items) :
			capture_absent(root, root->obj_uid, 0, &items)))
		return false;
	std::sort(items.begin(), items.end(), [](const auto &left, const auto &right)
		  { return left.item_uid < right.item_uid; });
	const item_owner_identity system_owner = { item_owner_type::system, 0, 0 };
	uint64_t system_revision = 0;
	if (!adopted && !item_ownership_runtime_owner_revision(system_owner, &system_revision))
		return false;
	item_transfer_payload payload = {
		.from_owner = adopted ? from_owner : system_owner,
		.to_owner = adopted ? to_owner : from_owner,
		.reason = adopted ? reason : item_transfer_reason::creation,
		.reason_id = reason_id,
		.expected_from_revision = adopted ? from_revision : system_revision,
		.expected_to_revision = adopted ? to_revision : from_revision,
		.selected_item_uid = root->obj_uid,
		.target_root_item_uid = adopted && target_container ? target_runtime.root_item_uid :
								      root->obj_uid,
		.target_parent_item_uid = adopted && target_container ? target_container->obj_uid :
									0,
		.expected_target_parent_revision =
			adopted && target_container ? target_runtime.item_revision : 0,
		.item_count = static_cast<uint16_t>(items.size()),
		.items = {},
		.item_blob_size = 0,
		.item_blob = {},
		.corpse = {}
	};
	for (size_t index = 0; index < items.size(); ++index)
		payload.items[index] = items[index];
	std::vector<player_item_snapshot> snapshots;
	std::vector<uint8_t> item_blob;
	if (player_item_snapshot_tree_capture(root, &snapshots, nullptr) !=
		    player_snapshot_capture_result::ok ||
	    snapshots.size() != items.size() ||
	    player_item_snapshot_list_encode(snapshots, &item_blob) !=
		    player_snapshot_codec_result::ok ||
	    item_blob.empty() || item_blob.size() > payload.item_blob.size())
		return false;
	payload.item_blob_size = static_cast<uint32_t>(item_blob.size());
	std::copy(item_blob.begin(), item_blob.end(), payload.item_blob.begin());
	if (adopted && corpse_transfer &&
	    !capture_corpse_metadata(actor, root, corpse_context, reason, &payload.corpse))
		return false;
	critical_operation_id operation_id = {};
	critical_command command = {};
	if (!critical_operation_id_generate(&operation_id) ||
	    !item_transfer_command_build(&command, operation_id, payload,
					 critical_source_site::command,
					 critical_deadline_class::interactive))
		return false;
	pending_movement entry = {
		.actor_pid = static_cast<uint32_t>(GET_PID(actor)),
		.payload = payload,
		.requested_to_owner = to_owner,
		.requested_target_parent_uid = target_container ? target_container->obj_uid : 0,
		.requested_reason = reason,
		.requested_reason_id = reason_id,
		.requested_corpse_uid = corpse_context ? corpse_context->obj_uid : 0,
		.adopting = !adopted,
		.completion = completion,
		.context = {},
		.context_size = context_size,
		.completion_ready = false,
		.completed = {}
	};
	if (context_size)
		memcpy(entry.context.data(), context, context_size);
	const std::string key = operation_key(operation_id);
	try
	{
		pending.emplace(key, entry);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	const critical_submit_result submitted =
		critical_command_coordinator_submit(std::move(command));
	if (submitted != critical_submit_result::accepted &&
	    submitted != critical_submit_result::attached)
	{
		pending.erase(key);
		++health.submission_failures;
		return false;
	}
	++health.submitted;
	account_health();
	return true;
}

void item_movement_transaction_handle_completions(const critical_completion *completions,
						  size_t count)
{
	if (count && !completions)
		return;
	for (size_t index = 0; index < count; ++index)
	{
		auto found = pending.find(operation_key(completions[index].operation_id));
		if (found == pending.end())
			continue;
		found->second.completed = completions[index];
		found->second.completion_ready = true;
		if (P_char actor = find_player_by_pid(found->second.actor_pid))
			publish(found, actor);
	}
	account_health();
}

void item_movement_transaction_player_ready(P_char actor)
{
	if (!actor || IS_NPC(actor) || GET_PID(actor) <= 0)
		return;
	for (;;)
	{
		auto found =
			std::find_if(pending.begin(), pending.end(),
				     [&](const auto &entry)
				     {
					     return entry.second.actor_pid ==
							    static_cast<uint32_t>(GET_PID(actor)) &&
						    entry.second.completion_ready;
				     });
		if (found == pending.end())
			break;
		/* publish may invoke a callback that inserts and rehashes pending. */
		publish(found, actor);
	}
}

bool item_movement_transaction_player_busy(P_char actor)
{
	if (!actor || IS_NPC(actor) || GET_PID(actor) <= 0)
		return false;
	return std::any_of(
		pending.begin(), pending.end(), [&](const auto &entry)
		{ return entry.second.actor_pid == static_cast<uint32_t>(GET_PID(actor)); });
}

item_movement_health item_movement_transaction_health_copy(void)
{
	account_health();
	return health;
}

void item_movement_transaction_reset_for_tests(void)
{
	pending.clear();
	health = {};
}

#include "item/item_movement_transaction.h"

#include "item/item_ownership_runtime.h"
#include "classes/necromancy.h"
#include "persistence/persistence_checkpoint.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "core/prototypes.h"
#include "core/utils.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <deque>
#include <new>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

extern P_obj object_list;
extern P_char character_list;
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
	bool adoption_only;
	item_movement_completion_fn completion;
	std::array<uint8_t, ITEM_MOVEMENT_CONTEXT_MAX_BYTES> context;
	size_t context_size;
	bool completion_ready;
	critical_completion completed;
};

std::unordered_map<std::string, pending_movement> pending;
item_movement_health health = {};

struct pending_creation_grant
{
	uint64_t item_uid;
	uint64_t target_container_uid;
	uint32_t recipient_pid;
	int32_t room;
	bool to_room;
};

struct creation_grant_queue
{
	std::deque<pending_creation_grant> requests;
	bool active = false;
	bool blocks_actor_commands = false;
};

std::unordered_map<uint32_t, creation_grant_queue> creation_grants;

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

P_char find_live_player(uint32_t pid)
{
	for (P_char character = character_list; character; character = character->next)
		if (IS_PC(character) && GET_PID(character) == static_cast<int>(pid))
			return character;
	return NULL;
}

item_owner_identity creation_grant_owner(const pending_creation_grant &request)
{
	return request.to_room ?
		       item_owner_identity{ item_owner_type::room,
					    static_cast<uint64_t>(world[request.room].number), 0 } :
		       item_owner_identity{ item_owner_type::player, request.recipient_pid, 0 };
}

bool creation_grant_request_valid(const pending_creation_grant &request)
{
	P_obj object = find_item(request.item_uid);
	if (!object || !OBJ_NOWHERE(object))
		return false;
	if (request.to_room)
		return request.room > NOWHERE && request.room <= top_of_world;
	P_char recipient = find_player_by_pid(request.recipient_pid);
	if (!recipient)
		return false;
	if (!request.target_container_uid)
		return true;
	P_obj container = find_item(request.target_container_uid);
	return container && OBJ_CARRIED_BY(container, recipient) &&
	       GET_ITEM_TYPE(container) == ITEM_CONTAINER;
}

bool creation_grant_target_available(const creation_grant_queue &queue, P_obj container,
				     P_char recipient)
{
	if (!container || !recipient || GET_ITEM_TYPE(container) != ITEM_CONTAINER)
		return false;
	if (OBJ_CARRIED_BY(container, recipient))
		return true;
	if (!OBJ_NOWHERE(container))
		return false;
	const uint32_t recipient_pid = static_cast<uint32_t>(GET_PID(recipient));
	return std::any_of(queue.requests.begin(), queue.requests.end(),
			   [&](const pending_creation_grant &request)
			   {
				   return !request.to_room && !request.target_container_uid &&
					  request.item_uid == container->obj_uid &&
					  request.recipient_pid == recipient_pid;
			   });
}

void discard_creation_queue(P_char actor, creation_grant_queue &queue)
{
	const bool blocks_actor_commands = queue.blocks_actor_commands;
	for (const pending_creation_grant &request : queue.requests)
		if (P_obj object = find_item(request.item_uid); object && OBJ_NOWHERE(object))
			extract_obj(object, FALSE);
	queue.requests.clear();
	queue.active = false;
	if (actor)
	{
		send_to_char(
			"The ownership authority could not continue the item grant; nothing else was created.\r\n",
			actor);
		if (blocks_actor_commands && actor->desc)
			actor->desc->prompt_mode = TRUE;
	}
}

bool start_creation_grant(P_char actor, creation_grant_queue &queue);

void pump_creation_grants()
{
	for (auto found = creation_grants.begin(); found != creation_grants.end();)
	{
		auto current = found++;
		creation_grant_queue &queue = current->second;
		if (queue.active || queue.requests.empty())
			continue;
		P_char actor = find_player_by_pid(current->first);
		if (!actor)
			continue;
		const pending_creation_grant &request = queue.requests.front();
		if (!creation_grant_request_valid(request))
		{
			discard_creation_queue(actor, queue);
			creation_grants.erase(current);
			continue;
		}
		const item_owner_identity owner = creation_grant_owner(request);
		if (movement_conflicts(owner, owner))
			continue;
		if (!start_creation_grant(actor, queue))
		{
			discard_creation_queue(actor, queue);
			creation_grants.erase(current);
		}
	}
}

void creation_grant_completion(P_char actor, bool committed, const item_transfer_result &,
			       unsigned int error_code, const uint8_t *encoded, size_t encoded_size)
{
	if (!actor || !encoded || encoded_size != sizeof(uint64_t) || IS_NPC(actor) ||
	    GET_PID(actor) <= 0)
		return;
	uint64_t item_uid = 0;
	memcpy(&item_uid, encoded, sizeof(item_uid));
	auto queue_found = creation_grants.find(static_cast<uint32_t>(GET_PID(actor)));
	if (queue_found == creation_grants.end() || queue_found->second.requests.empty() ||
	    !queue_found->second.active ||
	    queue_found->second.requests.front().item_uid != item_uid)
		return;
	creation_grant_queue &queue = queue_found->second;
	const pending_creation_grant request = queue.requests.front();
	P_obj object = find_item(request.item_uid);
	if (!committed)
	{
		if (object && OBJ_NOWHERE(object))
			extract_obj(object, FALSE);
		logit(LOG_FILE, "item creation grant did not commit (uid=%llu error=%u)",
		      (unsigned long long)request.item_uid, error_code);
		send_to_char(
			"The ownership authority did not commit; the granted item was discarded.\r\n",
			actor);
	}
	else if (!object || !OBJ_NOWHERE(object))
	{
		logit(LOG_FILE,
		      "item creation grant committed but live publication was stale (uid=%llu)",
		      (unsigned long long)request.item_uid);
		send_to_char(
			"The ownership authority committed, but live item publication failed.\r\n",
			actor);
	}
	else if (request.to_room)
	{
		if (request.room <= NOWHERE || request.room > top_of_world)
		{
			logit(LOG_FILE,
			      "item creation grant committed to an unavailable room (uid=%llu room=%d)",
			      (unsigned long long)request.item_uid, request.room);
			send_to_char(
				"The ownership authority committed, but the destination room vanished.\r\n",
				actor);
		}
		else
			obj_to_room(object, request.room);
	}
	else
	{
		P_char recipient = find_player_by_pid(request.recipient_pid);
		if (!recipient)
			logit(LOG_FILE,
			      "item creation grant committed to an unavailable player (uid=%llu pid=%u)",
			      (unsigned long long)request.item_uid, request.recipient_pid);
		else
		{
			obj_to_char(object, recipient);
			if (request.target_container_uid)
			{
				P_obj container = find_item(request.target_container_uid);
				if (!container || !OBJ_CARRIED_BY(container, recipient) ||
				    GET_ITEM_TYPE(container) != ITEM_CONTAINER)
					logit(LOG_FILE,
					      "item creation grant could not publish container placement (uid=%llu container_uid=%llu)",
					      (unsigned long long)request.item_uid,
					      (unsigned long long)request.target_container_uid);
				else
				{
					obj_from_char(object);
					obj_to_obj(object, container);
				}
			}
			mark_player_dirty_components(GET_PID(recipient),
						     PLAYER_COMPONENT_EQUIPMENT |
							     PLAYER_COMPONENT_INVENTORY);
		}
	}
	queue.requests.pop_front();
	queue.active = false;
	if (queue.requests.empty())
	{
		const bool blocks_actor_commands = queue.blocks_actor_commands;
		creation_grants.erase(queue_found);
		if (blocks_actor_commands && actor->desc)
		{
			send_to_char("Your starter kit is ready.\r\n", actor);
			actor->desc->prompt_mode = TRUE;
		}
		return;
	}
	const pending_creation_grant &next = queue.requests.front();
	if (!creation_grant_request_valid(next))
	{
		discard_creation_queue(actor, queue);
		creation_grants.erase(queue_found);
		return;
	}
	const item_owner_identity owner = creation_grant_owner(next);
	if (!movement_conflicts(owner, owner) && !start_creation_grant(actor, queue))
	{
		discard_creation_queue(actor, queue);
		creation_grants.erase(queue_found);
	}
}

bool start_creation_grant(P_char actor, creation_grant_queue &queue)
{
	if (!actor || IS_NPC(actor) || GET_PID(actor) <= 0 || queue.active ||
	    queue.requests.empty())
		return false;
	const pending_creation_grant &request = queue.requests.front();
	P_obj object = find_item(request.item_uid);
	if (!creation_grant_request_valid(request))
		return false;
	P_obj target_container =
		request.target_container_uid ? find_item(request.target_container_uid) : NULL;
	const item_owner_identity owner = creation_grant_owner(request);
	item_ownership_runtime_entry runtime = {};
	const bool adopted = item_ownership_runtime_lookup(object->obj_uid, &runtime);
	const item_owner_identity source = adopted ? runtime.owner : owner;
	if (!item_movement_transaction_submit(
		    actor, object, target_container, source, owner,
		    adopted ? item_transfer_reason::operator_repair :
			      item_transfer_reason::creation,
		    object->R_num >= 0 ? obj_index[object->R_num].virtual_number : 0,
		    creation_grant_completion, &request.item_uid, sizeof(request.item_uid)))
		return false;
	queue.active = true;
	return true;
}

bool queue_creation_grant(P_char actor, P_obj object, P_char recipient, int room,
			  P_obj target_container, bool to_room)
{
	if (!actor || IS_NPC(actor) || GET_PID(actor) <= 0 || !object || !object->obj_uid ||
	    !OBJ_NOWHERE(object) ||
	    (to_room ? (room <= NOWHERE || room > top_of_world) :
		       (!recipient || IS_NPC(recipient) || GET_PID(recipient) <= 0)))
		return false;
	const uint32_t actor_pid = static_cast<uint32_t>(GET_PID(actor));
	auto [found, inserted] = creation_grants.try_emplace(actor_pid);
	creation_grant_queue &queue = found->second;
	if (target_container &&
	    (to_room || !creation_grant_target_available(queue, target_container, recipient)))
	{
		if (inserted)
			creation_grants.erase(found);
		return false;
	}
	if (queue.requests.size() >= ITEM_MOVEMENT_PENDING_MAX)
	{
		if (inserted)
			creation_grants.erase(found);
		return false;
	}
	try
	{
		queue.requests.push_back(
			{ object->obj_uid, target_container ? target_container->obj_uid : 0,
			  recipient ? static_cast<uint32_t>(GET_PID(recipient)) : 0, room,
			  to_room });
	}
	catch (const std::bad_alloc &)
	{
		if (inserted)
			creation_grants.erase(found);
		return false;
	}
	if (queue.active || queue.requests.size() > 1)
		return true;
	if (start_creation_grant(actor, queue))
		return true;
	queue.requests.pop_back();
	if (queue.requests.empty())
		creation_grants.erase(found);
	return false;
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
		if (entry.adoption_only)
		{
			const item_movement_completion_fn completion_fn = entry.completion;
			const auto context = entry.context;
			const size_t context_size = entry.context_size;
			pending.erase(found);
			if (completion_fn)
				completion_fn(actor, true, result, 0, context.data(), context_size);
			++health.committed;
			account_health();
			return;
		}
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
		.target_root_item_uid = target_container ? target_runtime.root_item_uid :
							   root->obj_uid,
		.target_parent_item_uid = target_container ? target_container->obj_uid : 0,
		.expected_target_parent_revision = target_container ? target_runtime.item_revision :
								      0,
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
		.adoption_only = !adopted && item_owner_identity_equal(from_owner, to_owner),
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

bool item_creation_grant_submit_to_player(P_char actor, P_obj object, P_char recipient,
					  P_obj target_container)
{
	return queue_creation_grant(actor, object, recipient, NOWHERE, target_container, false);
}

bool item_creation_grant_submit_to_room(P_char actor, P_obj object, int room)
{
	return queue_creation_grant(actor, object, NULL, room, NULL, true);
}

bool item_creation_grant_mark_blocking(P_char actor)
{
	if (!actor || IS_NPC(actor) || GET_PID(actor) <= 0)
		return false;
	auto found = creation_grants.find(static_cast<uint32_t>(GET_PID(actor)));
	if (found == creation_grants.end())
		return false;
	found->second.blocks_actor_commands = true;
	return true;
}

bool item_creation_grant_blocks_commands(P_char actor)
{
	if (!actor || IS_NPC(actor) || GET_PID(actor) <= 0)
		return false;
	auto found = creation_grants.find(static_cast<uint32_t>(GET_PID(actor)));
	return found != creation_grants.end() && found->second.blocks_actor_commands;
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
		if (P_char actor = find_live_player(found->second.actor_pid))
			publish(found, actor);
	}
	pump_creation_grants();
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
	pump_creation_grants();
}

// Busy has to mean the same thing submission does: item_movement_transaction_submit()
// refuses whenever movement_conflicts() sees the player on either side of a pending
// entry, so a movement someone else submitted toward this player - a give, an operator
// creation grant - blocks their corpse handoff just as much as one they submitted
// themselves. Reporting only actor-owned work left death free to run the terminal save
// against an inbound transfer and to log the refused corpse transfer as failed_preserved.
bool item_movement_transaction_player_busy(P_char actor)
{
	if (!actor || IS_NPC(actor) || GET_PID(actor) <= 0)
		return false;
	const uint32_t pid = static_cast<uint32_t>(GET_PID(actor));
	if (creation_grants.find(pid) != creation_grants.end())
		return true;
	for (const auto &[actor_pid, queue] : creation_grants)
	{
		(void)actor_pid;
		for (const pending_creation_grant &request : queue.requests)
			if (!request.to_room && request.recipient_pid == pid)
				return true;
	}
	const item_owner_identity owner = { item_owner_type::player, pid, 0 };
	return std::any_of(
		pending.begin(), pending.end(), [pid, &owner](const auto &entry)
		{ return entry.second.actor_pid == pid || owner_conflicts(entry.second, owner); });
}

item_movement_health item_movement_transaction_health_copy(void)
{
	account_health();
	return health;
}

void item_movement_transaction_reset_for_tests(void)
{
	pending.clear();
	creation_grants.clear();
	health = {};
}

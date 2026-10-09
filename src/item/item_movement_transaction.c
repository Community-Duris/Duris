#include "item/item_movement_transaction.h"
#include "item/craft_pouch_mutation.h"
#include "player/craft_progression_hooks.h"
#include "combat/chaos_pouch_publication.h"

#include "item/item_ownership_runtime.h"
#include "item/item_actions.h"
#include "item/ordinary_drop_recovery.h"
#include "item/held_retirement_recovery.h"
#include "cmd/lockpick_retirement.h"
#include "economy/economic_gameplay_authority.h"
#include "economy/item_transfer_accounting.h"
#include "economy/native_quest_consumption_capture.h"
#include "economy/currency_transaction.h"
#include "economy/collector_catalog_cache.h"
#include "economy/collector_death_enrollment.h"
#include "economy/collector_transaction.h"
#include "classes/necromancy.h"
#include "persistence/persistence_checkpoint.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "player/inert_item_stage.h"
#include "player/player_load_items.h"
#include "player/player_save_pipeline.h"
#include "world/quest_mobile_native_binding.h"
#include "world/quest_mobile_native.h"
#include "world/handler.h"
#include "world/native_quest_frozen_continuation.h"
#include "world/native_quest_recovery_context.h"
#include "account/account.h"
#ifndef __NO_MYSQL__
#include "persistence/critical_command_repository.h"
#include "persistence/economic_sql_item_transfer_transaction.h"
#include "persistence/quest_reward_obligation_repository.h"
#include "player/player_sql_transaction_cleanup.h"
#endif
#include "core/prototypes.h"
#include "core/files.h"
#include "core/utils.h"
#include "account/account_reward.h"
#include "magic/spell_item_lifecycle.h"
#include "magic/spells.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <deque>
#include <new>
#include <memory>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

extern P_obj object_list;
extern P_char character_list;
extern P_desc descriptor_list;
extern const int top_of_world;

namespace
{
enum class publication_state : uint8_t
{
	none,
	ready,
	retrying,
	owner_waiting,
	blocked,
	ack_pending,
};

struct pending_movement
{
	uint32_t actor_pid;
	uint64_t actor_runtime_id;
	item_transfer_payload payload;
	item_owner_identity requested_to_owner;
	uint64_t requested_target_parent_uid;
	item_transfer_reason requested_reason;
	int64_t requested_reason_id;
	uint64_t requested_corpse_uid;
	bool adopting;
	bool adoption_only;
	item_movement_completion_fn completion;
	item_movement_publication_fn publication;
	std::array<uint8_t, ITEM_MOVEMENT_CONTEXT_MAX_BYTES> context;
	size_t context_size;
	bool completion_ready;
	bool publication_failed;
	unsigned int publication_attempts;
	publication_state publication_status;
	bool creation_batch;
	bool registry_applied;
	bool recovered_publication;
	// Preserve completed effects independently of retry/disposition status.
	bool craft_publication_started = false;
	bool ordinary_receipt_sealed = false;
	bool ordinary_publication_ready = false;
	bool restored_sql_drop = false;
	std::shared_ptr<const critical_command> live_drop_command = {};
	player_literal_inventory_token live_drop_token = {};
	ordinary_drop_live_publication_state live_drop_state = {};
	bool live_drop_acknowledged = false;
	bool live_drop_admission_refused = false;
	bool publication_inflight = false;
	std::shared_ptr<const critical_command> held_command = {};
	player_held_retirement_checkpoint_token held_token = {};
	bool craft_publication_ready = false;
	bool collector_invalidated;
	critical_completion completed;
	bool disposition_blocked = false;
	bool disposition_blocked_this_batch = false;
	bool publication_attempted_this_batch = false;
};

std::unordered_map<std::string, pending_movement> pending;
item_movement_health health = {};

struct pending_drop_preparation
{
	bool held_retirement = false;
	player_held_retirement_checkpoint_token held_token = {};
	bool active = false;
	player_literal_inventory_token token = {};
	int32_t room = 0;
	int32_t room_vnum = 0;
	item_movement_completion_fn completion = nullptr;
	std::array<uint8_t, ITEM_MOVEMENT_CONTEXT_MAX_BYTES> context = {};
	size_t context_size = 0;
};
std::array<pending_drop_preparation, ITEM_MOVEMENT_PENDING_MAX> drop_preparations;

struct pending_creation_grant
{
	uint64_t item_uid;
	uint64_t target_container_uid;
	uint32_t recipient_pid;
	int32_t room;
	bool to_room;
	bool allow_pre_entry;
	economic_source_kind source = {};
	uint64_t source_id = 0;
	item_movement_completion_fn completion = nullptr;
	std::array<uint8_t, ITEM_MOVEMENT_CONTEXT_MAX_BYTES> context = {};
	size_t context_size = 0;
	item_creation_grant_completion_fn grant_completion = nullptr;
};

struct creation_grant_queue
{
	item_creation_prepare_fn prepare;
	economic_source_kind source = {};
	uint64_t source_id = 0;
	std::deque<pending_creation_grant> requests;
	std::deque<pending_creation_grant> following_requests;
	bool active = false;
	bool batch_submission = false;
	bool blocks_actor_commands = false;
	bool announce_on_completion = false;
	bool stop_on_failure = false;
	bool publication_failed = false;
};

struct displaced_creation_object
{
	P_obj object;
	uint64_t item_uid;
	size_t depth;
};

std::unordered_map<uint32_t, creation_grant_queue> creation_grants;
std::deque<uint32_t> preparation_order;
const item_owner_identity system_owner_identity = { item_owner_type::system, 0, 0 };

/** Release a finished kit while preserving independent creations queued behind it. */
void finish_creation_queue(uint32_t pid)
{
	auto found = creation_grants.find(pid);
	if (found == creation_grants.end())
		return;
	auto following = std::move(found->second.following_requests);
	if (following.empty())
		creation_grants.erase(found);
	else
	{
		found->second = {};
		found->second.requests = std::move(following);
	}
}

std::string operation_key(const critical_operation_id &operation_id)
{
	return std::string(reinterpret_cast<const char *>(operation_id.bytes.data()),
			   operation_id.bytes.size());
}

bool reject_with(item_movement_reject *reject, item_movement_reject reason)
{
	*reject = reason;
	return false;
}

bool recovered_durable_item_publication(const critical_operation_id &, P_char actor, bool,
					const item_transfer_result &, unsigned int, const uint8_t *,
					size_t)
{
	// The repository or a domain recovery obligation owns the replayed outcome;
	// runtime inventory and room projections are hydrated from that authority.
	return actor != nullptr;
}

bool unresolved_replay_publication(const critical_operation_id &, P_char actor, bool committed,
				   const item_transfer_result &, unsigned int, const uint8_t *,
				   size_t)
{
	if (!actor)
		return false;
	if (!committed)
	{
		send_to_char(
			"The pending item change did not commit; your items remain unchanged.\r\n",
			actor);
		return true;
	}
	send_to_char(
		"A committed item change needs recovery before it can finish. Your item state is retained; please contact staff.\r\n",
		actor);
	return false;
}

bool refuse_active_item_submission(bool accounting_active, bool retained_owner_identity,
				   bool owner_matches_request, bool new_creation,
				   item_movement_reject *reject)
{
	if (!accounting_active)
		return false;
	if (retained_owner_identity && !owner_matches_request)
		return reject_with(reject, item_movement_reject::owner_mismatch);
	if (!retained_owner_identity && !new_creation)
		return reject_with(reject, item_movement_reject::missing_owner_identity);
	// Item movement has no authenticated schema-2 gameplay adapter yet. In
	// particular, system-to-player creation needs a real source-admission path;
	// UID allocation or a missing retained row cannot supply that authority.
	return reject_with(reject, item_movement_reject::active_accounting_unsupported);
}

item_movement_reject coordinator_reject_reason(critical_submit_result result)
{
	switch (result)
	{
	case critical_submit_result::unavailable:
		return item_movement_reject::coordinator_unavailable;
	case critical_submit_result::overloaded:
		return item_movement_reject::coordinator_overloaded;
	case critical_submit_result::invalid:
		return item_movement_reject::coordinator_invalid;
	case critical_submit_result::identity_conflict:
		return item_movement_reject::coordinator_identity_conflict;
	case critical_submit_result::journal_failure:
		return item_movement_reject::coordinator_journal_failure;
	case critical_submit_result::journal_uncertain:
		return item_movement_reject::coordinator_journal_uncertain;
	case critical_submit_result::accepted:
	case critical_submit_result::awaiting_durability:
	case critical_submit_result::attached:
		break;
	}
	return item_movement_reject::coordinator_rejected;
}

bool owner_conflicts(const pending_movement &entry, const item_owner_identity &owner)
{
	return item_owner_identity_equal(entry.payload.from_owner, owner) ||
	       item_owner_identity_equal(entry.payload.to_owner, owner) ||
	       (entry.adopting && item_owner_identity_equal(entry.requested_to_owner, owner));
}

size_t native_quest_pending_count();
bool native_quest_owner_busy(const item_owner_identity &);
void native_quest_reset_for_tests();
void native_quest_handle_completions(const critical_completion *, size_t);

bool movement_conflicts(const item_owner_identity &from_owner, const item_owner_identity &to_owner)
{
	if (native_quest_owner_busy(from_owner) || native_quest_owner_busy(to_owner))
		return true;
	return std::any_of(pending.begin(), pending.end(),
			   [&](const auto &entry) {
				   return owner_conflicts(entry.second, from_owner) ||
					  owner_conflicts(entry.second, to_owner);
			   });
}

bool coin_movement_pending(P_obj object)
{
	if (!object)
		return false;
	item_ownership_runtime_entry runtime = {};
	return currency_transaction_coin_item_busy(object->obj_uid) ||
	       (item_ownership_runtime_lookup(object->obj_uid, &runtime) &&
		currency_transaction_coin_item_busy(runtime.root_item_uid));
}

bool coordinator_item_fenced(P_obj object)
{
	return object && object->obj_uid &&
	       (collector_transaction_item_busy(object->obj_uid) ||
		critical_command_coordinator_is_fenced(
			{ critical_entity_type::item, object->obj_uid }, nullptr));
}

bool capture(P_obj object, uint64_t root_uid, uint64_t parent_uid,
	     std::vector<item_transfer_entry> *items)
{
	if (!object || !items || !object->obj_uid || items->size() >= ITEM_TRANSFER_MAX_ITEMS)
		return false;
	item_ownership_runtime_entry runtime = {};
	if (!item_ownership_runtime_lookup(object->obj_uid, &runtime))
	{
		logit(LOG_FILE,
		      "item_movement: outcome=topology_mismatch cause=missing_ledger_row "
		      "uid=%llu vnum=%d live(root=%llu,parent=%llu)",
		      (unsigned long long)object->obj_uid, OBJ_VNUM(object),
		      (unsigned long long)root_uid, (unsigned long long)parent_uid);
		return false;
	}
	// The nesting the ledger records has to match the nesting the object tree actually
	// has. A command that moved an item between containers without submitting a transfer
	// leaves the two disagreeing, and this is the only place that disagreement surfaces,
	// so name both sides rather than letting the caller guess which predicate failed.
	if (runtime.root_item_uid != root_uid || runtime.parent_item_uid != parent_uid ||
	    runtime.vnum != OBJ_VNUM(object) || runtime.state != item_custody_state::active)
	{
		logit(LOG_FILE,
		      "item_movement: outcome=topology_mismatch uid=%llu "
		      "ledger(root=%llu,parent=%llu,vnum=%d,state=%u) "
		      "live(root=%llu,parent=%llu,vnum=%d)",
		      (unsigned long long)runtime.item_uid,
		      (unsigned long long)runtime.root_item_uid,
		      (unsigned long long)runtime.parent_item_uid, runtime.vnum,
		      (unsigned int)runtime.state, (unsigned long long)root_uid,
		      (unsigned long long)parent_uid, OBJ_VNUM(object));
		return false;
	}
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

bool capture_batch_corpse_metadata(P_char actor, P_obj const *roots, size_t root_count,
				   P_obj corpse, item_transfer_reason reason,
				   item_corpse_metadata *metadata)
{
	if (!actor || !roots || !root_count || !corpse || !metadata ||
	    (reason != item_transfer_reason::corpse_loot &&
	     reason != item_transfer_reason::corpse_create))
		return false;
	int64_t selected_weight = 0;
	for (size_t index = 0; index < root_count; ++index)
	{
		P_obj root = roots[index];
		if (!root || selected_weight > INT64_MAX - GET_OBJ_WEIGHT(root))
			return false;
		P_obj outer = root;
		while (OBJ_INSIDE(outer) && outer->loc.inside)
			outer = outer->loc.inside;
		if (reason == item_transfer_reason::corpse_create ? !OBJ_CARRIED_BY(root, actor) :
								    outer != corpse)
			return false;
		selected_weight += GET_OBJ_WEIGHT(root);
	}
	if (!capture_corpse_metadata(actor, roots[0], corpse, reason, metadata))
		return false;
	const int64_t post_weight = static_cast<int64_t>(corpse->weight) +
				    (reason == item_transfer_reason::corpse_create ?
					     selected_weight :
					     -selected_weight);
	if (post_weight < INT32_MIN || post_weight > INT32_MAX)
		return false;
	metadata->weight = static_cast<int32_t>(post_weight);
	return true;
}

P_obj find_item(uint64_t uid)
{
	for (P_obj object = object_list; object; object = object->next)
		if (object->obj_uid == uid)
			return object;
	return NULL;
}

bool destruction_publication_live_ready(P_char actor, const item_transfer_payload &payload)
{
	if (!actor || !IS_PC(actor) || GET_PID(actor) <= 0 ||
	    (payload.reason != item_transfer_reason::destruction &&
	     payload.reason != item_transfer_reason::quest_turnin) ||
	    payload.from_owner.type != item_owner_type::player ||
	    payload.from_owner.id != static_cast<uint32_t>(GET_PID(actor)) ||
	    payload.to_owner.type != item_owner_type::destruction || !payload.item_count ||
	    payload.item_count > ITEM_TRANSFER_MAX_ITEMS)
		return false;
	for (size_t index = 1; index < payload.item_count; ++index)
		if (payload.items[index - 1].item_uid >= payload.items[index].item_uid)
			return false;
	try
	{
		std::vector<uint8_t> seen(payload.item_count, 0);
		for (P_obj object = object_list; object; object = object->next)
		{
			auto found = std::lower_bound(
				payload.items.begin(), payload.items.begin() + payload.item_count,
				object->obj_uid, [](const item_transfer_entry &entry, uint64_t uid)
				{ return entry.item_uid < uid; });
			if (found != payload.items.begin() + payload.item_count &&
			    found->item_uid == object->obj_uid &&
			    ++seen[static_cast<size_t>(found - payload.items.begin())] != 1)
				return false;
		}
		// A player may reconnect after the committed destruction has already
		// removed every selected row from the saved inventory. There is then no
		// live tree to publish; an entirely absent tree is already published.
		if (std::all_of(seen.begin(), seen.end(), [](uint8_t count) { return count == 0; }))
			return true;
		if (std::any_of(seen.begin(), seen.end(), [](uint8_t count) { return count != 1; }))
			return false;

		std::vector<item_transfer_entry> live;
		live.reserve(payload.item_count);
		size_t root_count = 0;
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const item_transfer_entry &entry = payload.items[index];
			if (entry.parent_item_uid)
			{
				P_obj child = find_item(entry.item_uid);
				P_obj parent = find_item(entry.parent_item_uid);
				if (!child || !parent || !OBJ_INSIDE(child) ||
				    child->loc.inside != parent)
					return false;
				continue;
			}
			P_obj root = find_item(entry.item_uid);
			if (entry.root_item_uid != entry.item_uid || !root)
				return false;
			size_t listed = 0;
			if (OBJ_CARRIED_BY(root, actor))
			{
				size_t scanned = 0;
				for (P_obj held = actor->carrying; held; held = held->next_content)
				{
					if (++scanned > ITEM_TRANSFER_MAX_ITEMS)
						return false;
					listed += held == root;
				}
			}
			else if (OBJ_WORN_BY(root, actor))
				for (size_t slot = 0; slot < MAX_WEAR; ++slot)
					listed += actor->equipment[slot] == root;
			if (listed != 1 || !capture(root, entry.item_uid, 0, &live))
				return false;
			++root_count;
		}
		if (!root_count || live.size() != payload.item_count)
			return false;
		std::sort(live.begin(), live.end(), [](const auto &left, const auto &right)
			  { return left.item_uid < right.item_uid; });
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const item_transfer_entry &actual = live[index];
			const item_transfer_entry &expected = payload.items[index];
			if (actual.item_uid != expected.item_uid ||
			    actual.root_item_uid != expected.root_item_uid ||
			    actual.parent_item_uid != expected.parent_item_uid ||
			    actual.expected_item_revision != expected.expected_item_revision ||
			    actual.vnum != expected.vnum ||
			    actual.expected_state != expected.expected_state)
				return false;
		}
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

P_char find_live_player(uint32_t pid)
{
	for (P_char character = character_list; character; character = character->next)
		if (IS_PC(character) && GET_PID(character) == static_cast<int>(pid))
			return character;
	return NULL;
}

/** Craft publication requires membership in the native character registry. */
P_char find_registered_craft_player(uint32_t pid)
{
	for (P_char character = character_list; character; character = character->next)
		if (IS_PC(character) && GET_PID(character) == static_cast<int>(pid) &&
		    character->runtime_id &&
		    find_character_by_runtime_id(character->runtime_id) == character)
			return character;
	return nullptr;
}

P_char find_live_mobile(uint64_t runtime_id)
{
	P_char character = find_character_by_runtime_id(runtime_id);
	return character && IS_NPC(character) ? character : NULL;
}

bool live_drop_shape(P_char actor, P_obj root, int32_t room, int32_t room_vnum)
{
	if (!actor || !IS_PC(actor) || GET_PID(actor) <= 0 || !actor->runtime_id ||
	    find_character_by_runtime_id(actor->runtime_id) != actor || !root || !root->obj_uid ||
	    !OBJ_CARRIED_BY(root, actor) || actor->in_room != room || room < 0 ||
	    room > top_of_world || world[room].number != room_vnum || room_vnum <= 0 ||
	    IS_ROOM(room, ROOM_LOCKER) || root->type == ITEM_MONEY ||
	    (root->type == ITEM_CORPSE && IS_SET(root->value[1], PC_CORPSE)) ||
	    IS_SET(root->extra_flags, ITEM_TRANSIENT))
		return false;
	// Mirror native relocation/decay/falling eligibility without invoking the
	// RNG-bearing OBJ_FALLING macro or any gameplay handler during preparation.
	if (IS_WATER_ROOM(room) && !IS_SET(root->extra_flags, ITEM_FLOAT) &&
	    root->type != ITEM_BOAT && root->type != ITEM_SHIP &&
	    world[room].sector_type != SECT_UNDERWATER_GR &&
	    (VIRTUAL_EXIT(room, DIR_DOWN) || world[room].sector_type != SECT_UNDERWATER))
		return false;
	return IS_SET(root->extra_flags, ITEM_LEVITATES) ||
	       (world[room].sector_type != SECT_NO_GROUND &&
		world[room].sector_type != SECT_UNDRWLD_NOGROUND && world[room].chance_fall <= 0 &&
		actor->specials.z_cord <= 0);
}

bool live_drop_source_owned(P_obj root, const item_owner_identity &owner,
			    const std::vector<player_item_snapshot> &snapshots)
{
	std::vector<item_ownership_runtime_entry> custody;
	if (!root || snapshots.empty() || snapshots[0].object_uid != root->obj_uid ||
	    !item_ownership_runtime_snapshot_root(root->obj_uid, ITEM_TRANSFER_MAX_ITEMS,
						  &custody) ||
	    custody.size() != snapshots.size())
		return false;
	std::vector<P_obj> objects(custody.size(), nullptr);
	size_t traversed = 0;
	for (P_obj object = object_list; object; object = object->next)
	{
		// Match the recovery owner's bounded global-object observation. A cycle
		// or over-budget world cannot establish a unique native source graph.
		if (++traversed > 1000000)
			return false;
		const auto found = std::lower_bound(custody.begin(), custody.end(), object->obj_uid,
						    [](const auto &entry, uint64_t uid)
						    { return entry.item_uid < uid; });
		if (found == custody.end() || found->item_uid != object->obj_uid)
			continue;
		const auto index = static_cast<size_t>(found - custody.begin());
		if (objects[index])
			return false;
		objects[index] = object;
	}
	for (size_t index = 0; index < snapshots.size(); ++index)
	{
		const auto &item = snapshots[index];
		if (item_actions_object_busy(item.object_uid))
			return false;
		const auto found = std::lower_bound(custody.begin(), custody.end(), item.object_uid,
						    [](const auto &entry, uint64_t uid)
						    { return entry.item_uid < uid; });
		if (found == custody.end() || found->item_uid != item.object_uid ||
		    !item_owner_identity_equal(found->owner, owner) ||
		    found->state != item_custody_state::active || found->vnum != item.vnum ||
		    !found->item_revision || found->item_revision == UINT64_MAX ||
		    (index == 0 ? item.parent_index != -1 :
				  (item.parent_index < 0 ||
				   item.parent_index >= static_cast<int32_t>(index))))
			return false;
		P_obj object = objects[static_cast<size_t>(found - custody.begin())];
		const uint64_t parent = index ? snapshots[item.parent_index].object_uid : 0;
		if (!object || found->parent_item_uid != parent || (index == 0 && object != root))
			return false;
		if (parent)
		{
			const auto parent_row = std::lower_bound(custody.begin(), custody.end(),
								 parent,
								 [](const auto &entry, uint64_t uid)
								 { return entry.item_uid < uid; });
			if (parent_row == custody.end() || parent_row->item_uid != parent ||
			    !OBJ_INSIDE_OBJ(
				    object,
				    objects[static_cast<size_t>(parent_row - custody.begin())]))
				return false;
		}
	}
	return true;
}

[[maybe_unused]] bool live_drop_publication_marker(const critical_operation_id &, P_char, bool,
						   const item_transfer_result &, unsigned int,
						   const uint8_t *, size_t)
{
	// The specialized owner is dispatched before generic registry publication.
	return false;
}

bool trusted_steal_live_ready(P_char actor, uint64_t item_uid)
{
	P_obj object = find_item(item_uid);
	return actor && object && OBJ_CARRIED_BY(object, actor);
}

void retain_trusted_steal_publication(pending_movement &entry, uint64_t item_uid)
{
	if (!entry.publication_failed)
	{
		entry.publication_failed = true;
		++health.stale_publications;
		persistence_alert(AVATAR, "item_movement", "steal_publish", "none", "none",
				  "stale_live_publication", "item_uid=%llu actor_pid=%u",
				  (unsigned long long)item_uid, entry.actor_pid);
	}
	logit(LOG_FILE,
	      "item_movement: command=steal publication retained for retry uid=%llu actor_pid=%u",
	      (unsigned long long)item_uid, entry.actor_pid);
}

bool object_belongs_to_actor(P_obj object, P_char actor)
{
	if (!object || !actor)
		return false;
	for (size_t depth = 0; object && depth <= ITEM_TRANSFER_MAX_ITEMS; ++depth)
	{
		if (OBJ_CARRIED_BY(object, actor) || OBJ_WORN_BY(object, actor))
			return true;
		if (!OBJ_INSIDE(object) || !object->loc.inside)
			return false;
		object = object->loc.inside;
	}
	return false;
}

bool craft_output_roots(const item_transfer_payload &payload,
			std::vector<player_item_snapshot> *outputs, std::vector<uint64_t> *roots)
{
	if (!outputs || !roots)
		return false;
	outputs->clear();
	roots->clear();
	if (!payload.item_blob_size)
		return true;
	try
	{
		if (player_item_snapshot_list_decode(payload.item_blob.data(),
						     payload.item_blob_size,
						     outputs) != player_snapshot_codec_result::ok ||
		    outputs->empty())
			return false;
		for (const player_item_snapshot &output : *outputs)
			if (output.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
				roots->push_back(output.object_uid);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return !roots->empty();
}

bool refine_wallet_matches(P_char actor, const craft_recipe_continuation &terms, bool after)
{
	if (!actor || !IS_PC(actor) || GET_PID(actor) != static_cast<int>(terms.player_pid))
		return false;
	if (terms.refine_ore_count == 1)
		return true;
	const auto &cost = terms.refine_cost;
	const std::array<int32_t, 4> actual = { GET_COPPER(actor), GET_SILVER(actor),
						GET_GOLD(actor), GET_PLATINUM(actor) };
	return actor->only.pc->wallet_revision ==
		       (after ? cost.after_revision : cost.before_revision) &&
	       actual == (after ? cost.after : cost.before);
}

bool craft_live_ready(P_char actor, const pending_movement &entry)
{
	if (!actor)
		return false;
	craft_recipe_continuation refine;
	if (craft_refine_from_payload(entry.payload, &refine) &&
	    !refine_wallet_matches(actor, refine, false) &&
	    !refine_wallet_matches(actor, refine, true))
		return false;
	craft_pouch_mutation pouch;
	if (!craft_pouch_mutation_from_payload(entry.payload, &pouch))
		return false;
	if (pouch.before.object_uid)
	{
		P_obj object = find_item(pouch.before.object_uid);
		if (!object || !object_belongs_to_actor(object, actor))
			return false;
	}
	std::vector<player_item_snapshot> outputs;
	std::vector<uint64_t> output_roots;
	if (!craft_output_roots(entry.payload, &outputs, &output_roots))
		return false;
	std::unordered_set<uint64_t> input_roots;
	try
	{
		for (size_t index = 0; index < entry.payload.item_count; ++index)
			if (entry.payload.items[index].item_uid != pouch.before.object_uid &&
			    item_transfer_selected_root(entry.payload,
							entry.payload.items[index].item_uid) ==
				    entry.payload.items[index].item_uid)
				input_roots.insert(entry.payload.items[index].item_uid);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	for (uint64_t uid : input_roots)
	{
		P_obj object = find_item(uid);
		if (object && !object_belongs_to_actor(object, actor))
			return false;
	}
	for (uint64_t uid : output_roots)
	{
		P_obj object = find_item(uid);
		if (!object || (!OBJ_NOWHERE(object) && !object_belongs_to_actor(object, actor)))
			return false;
	}
	return true;
}

bool restore_craft_outputs(const pending_movement &entry)
{
	std::vector<player_item_snapshot> outputs;
	std::vector<uint64_t> roots;
	if (!craft_output_roots(entry.payload, &outputs, &roots))
		return false;
	std::vector<player_item_snapshot> missing;
	std::vector<int32_t> remap(outputs.size(), -1);
	std::vector<bool> absent(outputs.size(), false);
	for (size_t index = 0; index < outputs.size(); ++index)
	{
		const auto &item = outputs[index];
		absent[index] = item.parent_index < 0 ? !find_item(item.object_uid) :
							absent[item.parent_index];
		if (!absent[index])
		{
			P_obj existing = find_item(item.object_uid);
			if (!existing || OBJ_VNUM(existing) != item.vnum ||
			    (item.parent_index >= 0 &&
			     (!OBJ_INSIDE(existing) ||
			      existing->loc.inside !=
				      find_item(outputs[item.parent_index].object_uid))))
				return false;
			continue;
		}
		if (find_item(item.object_uid))
			return false;
		remap[index] = static_cast<int32_t>(missing.size());
		missing.push_back(item);
		if (item.parent_index >= 0)
			missing.back().parent_index = remap[item.parent_index];
	}
	if (missing.empty())
		return true;
	item_transfer_payload projection = {};
	projection.from_owner = { item_owner_type::system, 0, 0 };
	projection.to_owner = entry.payload.to_owner;
	projection.reason = item_transfer_reason::creation;
	projection.multi_root = true;
	projection.item_count = static_cast<uint16_t>(missing.size());
	std::vector<uint8_t> encoded;
	if (player_item_snapshot_list_encode(missing, &encoded) !=
		    player_snapshot_codec_result::ok ||
	    encoded.size() > projection.item_blob.size())
		return false;
	projection.item_blob_size = static_cast<uint32_t>(encoded.size());
	std::copy(encoded.begin(), encoded.end(), projection.item_blob.begin());
	for (size_t index = 0; index < missing.size(); ++index)
	{
		size_t root = index;
		while (missing[root].parent_index >= 0)
			root = static_cast<size_t>(missing[root].parent_index);
		projection.items[index] = { missing[index].object_uid,
					    missing[root].object_uid,
					    missing[index].parent_index < 0 ?
						    0 :
						    missing[missing[index].parent_index].object_uid,
					    ITEM_TRANSFER_ABSENT_REVISION,
					    missing[index].vnum,
					    item_custody_state::absent };
	}
	item_transfer_result result = {};
	result.root_item_uid = item_transfer_result_root(projection);
	result.item_count = projection.item_count;
	item_transfer_result committed = {};
	if (!item_transfer_command_decode_result(entry.completed.result_payload.data(),
						 entry.completed.result_size, &committed))
		return false;
	result.to_owner_revision = committed.to_owner_revision;
	std::vector<P_obj> staged;
	return player_load_item_graph_materialize_creation(projection, result, &staged);
}

bool publish_craft(const pending_movement &entry, P_char actor)
{
	if (!restore_craft_outputs(entry) || !craft_live_ready(actor, entry))
		return false;
	craft_pouch_mutation pouch;
	if (!craft_pouch_mutation_from_payload(entry.payload, &pouch))
		return false;
	std::vector<player_item_snapshot> outputs;
	std::vector<uint64_t> output_roots;
	if (!craft_output_roots(entry.payload, &outputs, &output_roots))
		return false;
	std::unordered_set<uint64_t> input_roots;
	try
	{
		for (size_t index = 0; index < entry.payload.item_count; ++index)
			if (entry.payload.items[index].item_uid != pouch.before.object_uid &&
			    item_transfer_selected_root(entry.payload,
							entry.payload.items[index].item_uid) ==
				    entry.payload.items[index].item_uid)
				input_roots.insert(entry.payload.items[index].item_uid);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	craft_recipe_continuation refine;
	if (craft_refine_from_payload(entry.payload, &refine) && refine.refine_ore_count != 1)
	{
		// Only this original committed root can publish its frozen wallet after-image.
		// A later or unrelated live wallet never authorizes skipping this obligation.
		if (!refine_wallet_matches(actor, refine, false) &&
		    !refine_wallet_matches(actor, refine, true))
			return false;
		currency_vector wallet = {};
		std::copy(refine.refine_cost.after.begin(), refine.refine_cost.after.end(),
			  wallet.amount.begin());
		if (!currency_transaction_publish_wallet(actor, wallet,
							 refine.refine_cost.after_revision))
			return false;
	}
	if (pouch.before.object_uid &&
	    !chaos_pouch_publish_committed(find_item(pouch.before.object_uid), pouch))
		return false;
	for (uint64_t uid : input_roots)
	{
		P_obj object = find_item(uid);
		if (!object)
			continue;
		if (!object_belongs_to_actor(object, actor))
			return false;
		extract_obj(object);
	}
	for (uint64_t uid : output_roots)
	{
		P_obj object = find_item(uid);
		if (!object)
			return false;
		if (OBJ_NOWHERE(object))
			obj_to_char(object, actor);
		if (!object_belongs_to_actor(object, actor))
			return false;
	}
	return true;
}

void discard_craft_outputs(const pending_movement &entry)
{
	if (entry.payload.reason != item_transfer_reason::craft || !entry.payload.item_blob_size)
		return;
	std::vector<player_item_snapshot> outputs;
	std::vector<uint64_t> roots;
	if (!craft_output_roots(entry.payload, &outputs, &roots))
		return;
	for (uint64_t uid : roots)
	{
		P_obj object = find_item(uid);
		if (object && OBJ_NOWHERE(object))
			extract_obj(object);
	}
}

bool retained_player_transfer_reason(item_transfer_reason reason)
{
	return reason == item_transfer_reason::soulbind || reason == item_transfer_reason::slip;
}

item_owner_identity creation_grant_owner(const pending_creation_grant &request)
{
	return request.to_room ?
		       item_owner_identity{ item_owner_type::room,
					    static_cast<uint64_t>(world[request.room].number), 0 } :
		       item_owner_identity{ item_owner_type::player, request.recipient_pid, 0 };
}

bool creation_grant_conflicts(const pending_creation_grant &request)
{
	if (!request.to_room && player_save_pipeline_sealed_save_pending(request.recipient_pid))
		return true;
	item_ownership_runtime_entry runtime = {};
	const item_owner_identity source =
		item_ownership_runtime_lookup(request.item_uid, &runtime) ? runtime.owner :
									    system_owner_identity;
	return movement_conflicts(source, creation_grant_owner(request));
}

bool creation_grant_tree_available(P_obj object)
{
	if (!object || find_item(object->obj_uid) != object)
		return false;
	for (P_obj child = object->contains; child; child = child->next_content)
		if (!creation_grant_tree_available(child))
			return false;
	return true;
}

bool creation_grant_request_live_ready(P_char actor, const pending_creation_grant &request)
{
	P_obj object = find_item(request.item_uid);
	if (!object || !creation_grant_tree_available(object))
		return false;
	if (OBJ_NOWHERE(object))
		return true;
	if (request.to_room)
		return request.room > NOWHERE && request.room <= top_of_world &&
		       OBJ_IN_ROOM(object, request.room);

	P_char recipient = request.allow_pre_entry && actor &&
					   request.recipient_pid ==
						   static_cast<uint32_t>(GET_PID(actor)) ?
				   actor :
				   find_live_player(request.recipient_pid);
	if (!recipient)
		return false;
	if (OBJ_CARRIED_BY(object, recipient))
		return true;
	if (!request.target_container_uid)
		return false;
	P_obj container = find_item(request.target_container_uid);
	return container && OBJ_CARRIED_BY(container, recipient) &&
	       GET_ITEM_TYPE(container) == ITEM_CONTAINER && OBJ_INSIDE_OBJ(object, container);
}

bool creation_grant_batch_live_ready(P_char actor, const creation_grant_queue &queue)
{
	for (const pending_creation_grant &request : queue.requests)
		if (!creation_grant_request_live_ready(actor, request))
			return false;
	return true;
}

bool creation_grant_batch_published(P_char actor, const creation_grant_queue &queue)
{
	for (const pending_creation_grant &request : queue.requests)
	{
		P_obj object = find_item(request.item_uid);
		if (!object || !OBJ_CARRIED_BY(object, actor) ||
		    !creation_grant_tree_available(object))
			return false;
	}
	return true;
}

size_t creation_payload_depth(const item_transfer_payload &payload, size_t index)
{
	size_t depth = 0;
	uint64_t parent_uid = payload.items[index].parent_item_uid;
	for (size_t step = 0; parent_uid && step < payload.item_count; ++step)
	{
		++depth;
		auto parent = std::find_if(payload.items.begin(),
					   payload.items.begin() + payload.item_count,
					   [&](const item_transfer_entry &entry)
					   { return entry.item_uid == parent_uid; });
		if (parent == payload.items.begin() + payload.item_count)
			break;
		parent_uid = parent->parent_item_uid;
	}
	return depth;
}

bool stage_displaced_creation_objects(const item_transfer_payload &payload,
				      std::vector<displaced_creation_object> *displaced)
{
	if (!displaced)
		return false;
	try
	{
		displaced->clear();
		displaced->reserve(payload.item_count);
		for (P_obj object = object_list; object; object = object->next)
			for (size_t index = 0; index < payload.item_count; ++index)
				if (object->obj_uid == payload.items[index].item_uid)
				{
					displaced->push_back(
						{ object, object->obj_uid,
						  creation_payload_depth(payload, index) });
					break;
				}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	for (displaced_creation_object &entry : *displaced)
		entry.object->obj_uid = 0;
	return true;
}

void restore_displaced_creation_objects(const std::vector<displaced_creation_object> &displaced)
{
	for (const displaced_creation_object &entry : displaced)
		if (entry.object)
			entry.object->obj_uid = static_cast<unsigned long>(entry.item_uid);
}

void extract_creation_roots(const std::vector<P_obj> &roots)
{
	for (P_obj root : roots)
		if (root && find_item(root->obj_uid) == root)
			extract_obj(root, FALSE);
}

void extract_displaced_creation_objects(std::vector<displaced_creation_object> *displaced)
{
	if (!displaced)
		return;
	std::stable_sort(displaced->begin(), displaced->end(),
			 [](const displaced_creation_object &left,
			    const displaced_creation_object &right)
			 { return left.depth > right.depth; });
	for (const displaced_creation_object &entry : *displaced)
		if (entry.object)
			extract_obj(entry.object, FALSE);
}

void note_creation_grant_publication_failure(P_char actor, creation_grant_queue &queue,
					     uint32_t actor_pid)
{
	if (queue.publication_failed)
		return;
	queue.publication_failed = true;
	const pending_creation_grant *request = queue.requests.empty() ? nullptr :
									 &queue.requests.front();
	P_obj object = request ? find_item(request->item_uid) : nullptr;
	const char *kind = queue.batch_submission ? "batch" : "single";
	const uint32_t recipient_pid = request ? request->recipient_pid : actor_pid;
	unsigned int loc_p = object ? object->loc_p : 0;
	uint32_t carrier_pid = 0;
	uint32_t wearer_pid = 0;
	uint64_t container_uid = 0;
	int room = NOWHERE;
	if (object && OBJ_CARRIED(object) && object->loc.carrying && IS_PC(object->loc.carrying))
		carrier_pid = static_cast<uint32_t>(GET_PID(object->loc.carrying));
	if (object && OBJ_WORN(object) && object->loc.wearing && IS_PC(object->loc.wearing))
		wearer_pid = static_cast<uint32_t>(GET_PID(object->loc.wearing));
	if (object && OBJ_INSIDE(object) && object->loc.inside)
		container_uid = object->loc.inside->obj_uid;
	if (object && OBJ_ROOM(object) && object->loc.room > NOWHERE &&
	    object->loc.room <= top_of_world)
		room = world[object->loc.room].number;
	const unsigned long long item_uid = object ? object->obj_uid :
						     (request ? request->item_uid : 0);
	const int vnum = object ? OBJ_VNUM(object) : -1;
	statuslog(56,
		  "&+RALERT&n: committed %s creation grant needs live publication repair "
		  "(actor_pid=%u recipient_pid=%u uid=%llu vnum=%d loc_p=%u carrier_pid=%u "
		  "wearer_pid=%u container_uid=%llu room=%d)",
		  kind, actor_pid, recipient_pid, item_uid, vnum, loc_p, carrier_pid, wearer_pid,
		  static_cast<unsigned long long>(container_uid), room);
	persistence_alert(AVATAR, "item", "redacted", "none", "none", "stale_live_publication",
			  "kind=%s uid=%llu vnum=%d recipient_pid=%u loc_p=%u carrier_pid=%u "
			  "wearer_pid=%u container_uid=%llu room=%d",
			  kind, item_uid, vnum, recipient_pid, loc_p, carrier_pid, wearer_pid,
			  static_cast<unsigned long long>(container_uid), room);
	if (actor)
		send_to_char("Your items are safe but are still being delivered.\r\n"
			     "Please wait a moment or reconnect; do not request them again.\r\n",
			     actor);
}

bool reconcile_creation_grant_batch(P_char actor, pending_movement &entry,
				    creation_grant_queue &queue, const item_transfer_result &result,
				    bool publish_to_actor = true)
{
	bool needs_reconciliation = false;
	const size_t request_count = entry.creation_batch ? queue.requests.size() : 1;
	for (size_t index = 0; index < request_count; ++index)
	{
		const pending_creation_grant &request = queue.requests[index];
		P_obj object = find_item(request.item_uid);
		if (!object || !creation_grant_request_live_ready(actor, request))
			needs_reconciliation = true;
	}
	if (!needs_reconciliation)
		return true;

	std::vector<displaced_creation_object> displaced;
	if (!stage_displaced_creation_objects(entry.payload, &displaced))
		return false;

	std::vector<P_obj> roots;
	item_transfer_payload materialization_payload = entry.payload;
	if (!materialization_payload.multi_root)
	{
		materialization_payload.selected_item_uid = 0;
		materialization_payload.target_root_item_uid = 0;
		materialization_payload.target_parent_item_uid = 0;
		materialization_payload.expected_target_parent_revision = 0;
		materialization_payload.multi_root = true;
	}
	if (!player_load_item_graph_materialize_creation(materialization_payload, result, &roots) ||
	    roots.size() != request_count)
	{
		extract_creation_roots(roots);
		restore_displaced_creation_objects(displaced);
		return false;
	}
	for (P_obj root : roots)
		if (!root || !OBJ_NOWHERE(root))
		{
			extract_creation_roots(roots);
			restore_displaced_creation_objects(displaced);
			return false;
		}

	extract_displaced_creation_objects(&displaced);
	if (publish_to_actor)
		for (P_obj root : roots)
			obj_to_char(root, actor);
	return true;
}

bool creation_grant_request_valid(const pending_creation_grant &request)
{
	P_obj object = find_item(request.item_uid);
	if (!object || !OBJ_NOWHERE(object))
		return false;
	if (request.to_room)
		return request.room > NOWHERE && request.room <= top_of_world;
	if (!request.allow_pre_entry && !find_live_player(request.recipient_pid))
		return false;
	if (request.allow_pre_entry && request.target_container_uid)
		return false;
	if (!request.target_container_uid)
		return true;
	P_char recipient = find_live_player(request.recipient_pid);
	if (!recipient)
		return false;
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
	const auto matches = [&](const pending_creation_grant &request)
	{
		return !request.to_room && !request.target_container_uid &&
		       request.item_uid == container->obj_uid &&
		       request.recipient_pid == recipient_pid;
	};
	return std::any_of(queue.requests.begin(), queue.requests.end(), matches) ||
	       std::any_of(queue.following_requests.begin(), queue.following_requests.end(),
			   matches);
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

bool start_creation_grant(P_char actor, creation_grant_queue &queue, item_movement_reject *reject);

void pump_creation_grants()
{
	for (auto found = creation_grants.begin(); found != creation_grants.end();)
	{
		auto current = found++;
		creation_grant_queue &queue = current->second;
		if (queue.prepare || queue.active || queue.requests.empty())
			continue;
		const pending_creation_grant &request = queue.requests.front();
		P_char actor = find_live_player(current->first);
		if (!actor)
			continue;
		if (!creation_grant_request_valid(request))
		{
			discard_creation_queue(actor, queue);
			finish_creation_queue(current->first);
			continue;
		}
		if (creation_grant_conflicts(request))
			continue;
		item_movement_reject reject = item_movement_reject::none;
		if (!start_creation_grant(actor, queue, &reject))
		{
			if (item_movement_reject_is_transient(reject))
				continue;
			discard_creation_queue(actor, queue);
			finish_creation_queue(current->first);
		}
	}
}

bool publish_creation_grant(P_char actor, const pending_creation_grant &request)
{
	P_obj object = find_item(request.item_uid);
	if (request.to_room)
	{
		if (request.room <= NOWHERE || request.room > top_of_world || !object)
			return false;
		if (OBJ_IN_ROOM(object, request.room))
			return true;
		if (!OBJ_NOWHERE(object))
			return false;
		obj_to_room(object, request.room);
		object = find_item(request.item_uid);
		return object && OBJ_IN_ROOM(object, request.room);
	}

	P_char recipient = request.allow_pre_entry &&
					   request.recipient_pid ==
						   static_cast<uint32_t>(GET_PID(actor)) ?
				   actor :
				   find_live_player(request.recipient_pid);
	if (!recipient)
		return false;

	P_obj container = request.target_container_uid ? find_item(request.target_container_uid) :
							 NULL;
	if (request.target_container_uid && (!container || !OBJ_CARRIED_BY(container, recipient) ||
					     GET_ITEM_TYPE(container) != ITEM_CONTAINER))
		return false;
	if (!object)
		return false;
	if (request.target_container_uid && OBJ_INSIDE_OBJ(object, container))
	{
		mark_player_dirty_components(GET_PID(recipient),
					     PLAYER_COMPONENT_EQUIPMENT |
						     PLAYER_COMPONENT_INVENTORY);
		return true;
	}
	if (!request.target_container_uid && OBJ_CARRIED_BY(object, recipient))
	{
		mark_player_dirty_components(GET_PID(recipient),
					     PLAYER_COMPONENT_EQUIPMENT |
						     PLAYER_COMPONENT_INVENTORY);
		return true;
	}
	if (!OBJ_NOWHERE(object) && !OBJ_CARRIED_BY(object, recipient))
		return false;
	if (OBJ_NOWHERE(object))
		obj_to_char(object, recipient);
	object = find_item(request.item_uid);
	if (!object || !OBJ_CARRIED_BY(object, recipient))
		return false;
	if (request.target_container_uid)
	{
		obj_from_char(object);
		object = find_item(request.item_uid);
		if (!object || !OBJ_NOWHERE(object))
			return false;
		obj_to_obj(object, container);
		object = find_item(request.item_uid);
		if (!object || !OBJ_INSIDE_OBJ(object, container))
			return false;
	}
	mark_player_dirty_components(GET_PID(recipient),
				     PLAYER_COMPONENT_EQUIPMENT | PLAYER_COMPONENT_INVENTORY);
	return true;
}

void creation_grant_completion(P_char actor, bool committed, const item_transfer_result &result,
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
	const auto business_completion = request.completion;
	const auto business_context = request.context;
	const size_t business_context_size = request.context_size;
	const auto grant_completion = request.grant_completion;
	const auto notify_business = [&]()
	{
		if (business_completion)
			business_completion(actor, committed, result, error_code,
					    business_context.data(), business_context_size);
		if (grant_completion)
			grant_completion(actor, request.item_uid, committed, error_code);
	};
	P_obj object = find_item(request.item_uid);
	if (!committed)
	{
		if (object && OBJ_NOWHERE(object))
			extract_obj(object, FALSE);
		logit(LOG_FILE, "item creation grant did not commit (uid=%llu error=%u)",
		      (unsigned long long)request.item_uid, error_code);
		if (!business_completion && !grant_completion)
			send_to_char(
				"The ownership authority did not commit; the granted item was discarded.\r\n",
				actor);
	}
	else if (!publish_creation_grant(actor, request))
	{
		note_creation_grant_publication_failure(actor, queue,
							static_cast<uint32_t>(GET_PID(actor)));
		return;
	}
	queue.requests.pop_front();
	queue.active = false;
	if (!committed && queue.stop_on_failure)
	{
		// Earlier committed roots remain durable. Discard only unpublished
		// tail requests and never report a partial kit as successfully ready.
		discard_creation_queue(actor, queue);
		creation_grants.erase(queue_found);
		notify_business();
		return;
	}
	if (queue.requests.empty())
	{
		const bool blocks_actor_commands = queue.blocks_actor_commands;
		const bool announce_on_completion = queue.announce_on_completion;
		creation_grants.erase(queue_found);
		if (blocks_actor_commands && actor->desc)
		{
			send_to_char(announce_on_completion ?
					     "Your Chaos Equipment has been prepared!!\r\n" :
					     "Your starter kit is ready.\r\n",
				     actor);
			actor->desc->prompt_mode = TRUE;
		}
		else if (announce_on_completion && actor->desc &&
			 actor->desc->connected == CON_PLAYING)
		{
			send_to_char("Your Chaos Equipment has been prepared!!\r\n", actor);
		}
		notify_business();
		return;
	}
	const pending_creation_grant &next = queue.requests.front();
	if (!creation_grant_request_valid(next))
	{
		discard_creation_queue(actor, queue);
		creation_grants.erase(queue_found);
		notify_business();
		return;
	}
	if (creation_grant_conflicts(next))
	{
		notify_business();
		return;
	}
	item_movement_reject reject = item_movement_reject::none;
	if (!start_creation_grant(actor, queue, &reject) &&
	    !item_movement_reject_is_transient(reject))
	{
		discard_creation_queue(actor, queue);
		creation_grants.erase(queue_found);
	}
	notify_business();
}

void creation_grant_batch_completion(P_char actor, bool committed, const item_transfer_result &,
				     unsigned int error_code, const uint8_t *encoded,
				     size_t encoded_size)
{
	if (!actor || !encoded || encoded_size != sizeof(uint32_t) || IS_NPC(actor) ||
	    GET_PID(actor) <= 0)
		return;
	uint32_t actor_pid = 0;
	memcpy(&actor_pid, encoded, sizeof(actor_pid));
	auto queue_found = creation_grants.find(actor_pid);
	if (queue_found == creation_grants.end() || queue_found->second.requests.empty() ||
	    !queue_found->second.active || !queue_found->second.batch_submission)
		return;
	creation_grant_queue &queue = queue_found->second;
	const bool blocks_actor_commands = queue.blocks_actor_commands;
	const bool announce_on_completion = queue.announce_on_completion;
	if (!committed)
	{
		logit(LOG_FILE, "item creation grant batch did not commit (pid=%u error=%u)",
		      actor_pid, error_code);
		discard_creation_queue(actor, queue);
		finish_creation_queue(actor_pid);
		return;
	}

	if (!creation_grant_batch_live_ready(actor, queue))
	{
		note_creation_grant_publication_failure(actor, queue, actor_pid);
		return;
	}
	queue.publication_failed = false;
	for (const pending_creation_grant &request : queue.requests)
	{
		P_obj object = find_item(request.item_uid);
		if (!OBJ_CARRIED_BY(object, actor))
			obj_to_char(object, actor);
	}
	if (!creation_grant_batch_published(actor, queue))
	{
		note_creation_grant_publication_failure(actor, queue, actor_pid);
		return;
	}
	mark_player_dirty_components(actor_pid,
				     PLAYER_COMPONENT_EQUIPMENT | PLAYER_COMPONENT_INVENTORY);
	queue.requests.clear();
	queue.active = false;
	finish_creation_queue(actor_pid);
	if (blocks_actor_commands && actor->desc)
	{
		send_to_char(announce_on_completion ?
				     "Your Chaos Equipment has been prepared!!\r\n" :
				     "Your starter kit is ready.\r\n",
			     actor);
	}
	else if (announce_on_completion && actor->desc && actor->desc->connected == CON_PLAYING)
	{
		send_to_char("Your Chaos Equipment has been prepared!!\r\n", actor);
	}
	if (blocks_actor_commands && actor->desc)
		actor->desc->prompt_mode = TRUE;
}

bool start_creation_grant(P_char actor, creation_grant_queue &queue, item_movement_reject *reject)
{
	item_movement_reject discarded = item_movement_reject::none;
	if (!reject)
		reject = &discarded;
	*reject = item_movement_reject::none;
	if (!actor || IS_NPC(actor) || GET_PID(actor) <= 0 || queue.active ||
	    queue.requests.empty())
		return reject_with(reject, item_movement_reject::invalid_request);
	const pending_creation_grant &request = queue.requests.front();
	if (creation_grant_conflicts(request))
		return reject_with(reject, item_movement_reject::pending_conflict);
	if (queue.batch_submission)
	{
		if (!creation_grant_request_valid(request))
			return reject_with(reject, item_movement_reject::owner_mismatch);
		const uint32_t actor_pid = static_cast<uint32_t>(GET_PID(actor));
		std::vector<P_obj> objects;
		try
		{
			objects.reserve(queue.requests.size());
			for (const pending_creation_grant &candidate : queue.requests)
			{
				P_obj object = find_item(candidate.item_uid);
				if (candidate.to_room || candidate.target_container_uid ||
				    !candidate.allow_pre_entry ||
				    candidate.recipient_pid != actor_pid || !object ||
				    !OBJ_NOWHERE(object) || candidate.source != request.source ||
				    candidate.source_id != request.source_id)
					return reject_with(reject,
							   item_movement_reject::owner_mismatch);
				objects.push_back(object);
			}
		}
		catch (const std::bad_alloc &)
		{
			return reject_with(reject, item_movement_reject::allocation_failure);
		}
		if (!item_movement_transaction_submit_batch(
			    actor, objects.data(), objects.size(), NULL, system_owner_identity,
			    creation_grant_owner(request), item_transfer_reason::creation, 0,
			    creation_grant_batch_completion, &actor_pid, sizeof(actor_pid), NULL,
			    reject, nullptr, request.source, request.source_id))
			return false;
		queue.active = true;
		return true;
	}
	P_obj object = find_item(request.item_uid);
	if (!creation_grant_request_valid(request))
		return reject_with(reject, item_movement_reject::owner_mismatch);
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
		    request.source == economic_source_kind::quest_completion ?
			    static_cast<int64_t>(request.source_id) :
			    (object->R_num >= 0 ? obj_index[object->R_num].virtual_number : 0),
		    creation_grant_completion, &request.item_uid, sizeof(request.item_uid), NULL,
		    reject, nullptr, adopted ? economic_source_kind{} : request.source,
		    adopted || request.source == economic_source_kind::quest_completion ?
			    0 :
			    request.source_id))
		return false;
	queue.active = true;
	return true;
}

bool queue_creation_grant(P_char actor, P_obj object, P_char recipient, int room,
			  P_obj target_container, bool to_room, bool allow_pre_entry,
			  item_movement_completion_fn completion, const void *context,
			  size_t context_size, item_creation_grant_completion_fn grant_completion,
			  economic_source_kind source, uint64_t source_id = 0)
{
	if (!actor || IS_NPC(actor) || GET_PID(actor) <= 0 || !object || !object->obj_uid ||
	    !OBJ_NOWHERE(object) ||
	    (to_room ? (room <= NOWHERE || room > top_of_world) :
		       (!recipient || IS_NPC(recipient) || GET_PID(recipient) <= 0)) ||
	    (allow_pre_entry && (to_room || target_container || recipient != actor)) ||
	    context_size > ITEM_MOVEMENT_CONTEXT_MAX_BYTES || (context_size && !context) ||
	    (context_size && !completion) ||
	    (source == economic_source_kind::quest_completion && source_id > INT64_MAX) ||
	    (source_id && source == economic_source_kind{}) ||
	    (source == economic_source_kind::quest_completion && !source_id))
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
	if (queue.requests.size() + queue.following_requests.size() >=
	    ITEM_CREATION_GRANT_MAX_ROOTS)
	{
		if (inserted)
			creation_grants.erase(found);
		return false;
	}
	if (allow_pre_entry &&
	    (queue.prepare || queue.batch_submission || !queue.requests.empty() || queue.active))
	{
		if (inserted)
			creation_grants.erase(found);
		return false;
	}
	if (allow_pre_entry)
		queue.announce_on_completion = true;
	try
	{
		auto &destination = queue.batch_submission ? queue.following_requests :
							     queue.requests;
		pending_creation_grant request = {
			object->obj_uid,
			target_container ? target_container->obj_uid : 0,
			recipient ? static_cast<uint32_t>(GET_PID(recipient)) : 0,
			room,
			to_room,
			allow_pre_entry
		};
		request.source = source;
		request.source_id = source_id;
		request.completion = completion;
		request.context_size = context_size;
		request.grant_completion = grant_completion;
		if (context_size)
			memcpy(request.context.data(), context, context_size);
		destination.push_back(request);
	}
	catch (const std::bad_alloc &)
	{
		if (inserted)
			creation_grants.erase(found);
		return false;
	}
	if (queue.batch_submission || queue.active || queue.requests.size() > 1)
		return true;
	item_movement_reject reject = item_movement_reject::none;
	if (start_creation_grant(actor, queue, &reject))
		return true;
	if (item_movement_reject_is_transient(reject))
		return true;
	queue.requests.pop_back();
	if (queue.requests.empty())
		creation_grants.erase(found);
	return false;
}

void account_health()
{
	health.pending = pending.size() + native_quest_pending_count();
	health.retained_offline = 0;
	health.publication_retrying = 0;
	health.publication_blocked = 0;
	health.publication_ack_pending = 0;
	health.publication_owner_waiting = 0;
	for (const auto &[key, entry] : pending)
	{
		(void)key;
		if (entry.completion_ready)
			++health.retained_offline;
		if (entry.publication_status == publication_state::retrying)
			++health.publication_retrying;
		else if (entry.publication_status == publication_state::blocked)
			++health.publication_blocked;
		else if (entry.publication_status == publication_state::owner_waiting)
			++health.publication_owner_waiting;
		else if (entry.publication_status == publication_state::ack_pending)
			++health.publication_ack_pending;
	}
}

/** Validate the complete captured topology before publishing a corpse batch. */
bool corpse_batch_live_ready(P_char actor, const pending_movement &entry)
{
	P_obj corpse = find_item(entry.requested_corpse_uid);
	if (!actor || !corpse || GET_ITEM_TYPE(corpse) != ITEM_CORPSE ||
	    item_corpse_owner_id(corpse->value[CORPSE_PID], corpse->value[CORPSE_SAVEID]) !=
		    entry.payload.to_owner.id)
		return false;
	size_t live_children = 0, captured_children = 0;
	for (size_t index = 0; index < entry.payload.item_count; ++index)
	{
		const auto &captured = entry.payload.items[index];
		P_obj object = find_item(captured.item_uid);
		if (!object ||
		    (captured.parent_item_uid ?
			     !OBJ_INSIDE(object) || !object->loc.inside ||
				     object->loc.inside->obj_uid != captured.parent_item_uid :
			     !OBJ_CARRIED_BY(object, actor)))
			return false;
		P_obj linked = captured.parent_item_uid ? object->loc.inside->contains :
							  actor->carrying;
		size_t traversed = 0;
		while (linked && linked != object && traversed++ < ITEM_TRANSFER_MAX_ITEMS)
			linked = linked->next_content;
		if (linked != object)
			return false;
		captured_children += captured.parent_item_uid != 0;
		for (P_obj child = object->contains; child; child = child->next_content)
			if (++live_children > entry.payload.item_count)
				return false;
	}
	return live_children == captured_children;
}

void publish_corpse_batch(const pending_movement &entry)
{
	P_obj corpse = find_item(entry.requested_corpse_uid);
	for (size_t index = 0; index < entry.payload.item_count; ++index)
		if (!entry.payload.items[index].parent_item_uid)
		{
			P_obj root = find_item(entry.payload.items[index].item_uid);
			obj_from_char(root);
			obj_to_obj(root, corpse);
		}
}

/** Retain a failed opt-in publication without spinning or dropping its fence. */
void retain_publication_failure(pending_movement &entry, const char *reason)
{
	if (!entry.publication_failed)
	{
		entry.publication_failed = true;
		++health.stale_publications;
		logit(LOG_FILE, "item movement publication retained (pid=%u reason=%s)",
		      entry.actor_pid, reason ? reason : "unknown");
		persistence_alert(AVATAR, "item", "redacted", "none", "none",
				  "stale_live_publication", "pid=%u reason=%s", entry.actor_pid,
				  reason ? reason : "unknown");
	}
	if (entry.publication_attempts < ITEM_MOVEMENT_PUBLICATION_MAX_ATTEMPTS)
		++entry.publication_attempts;
	entry.publication_status = entry.publication_attempts >=
						   ITEM_MOVEMENT_PUBLICATION_MAX_ATTEMPTS ?
					   publication_state::blocked :
					   publication_state::retrying;
}

/** Bind semantic receipt identity independently of delivery attempt/timestamps. */
bool item_publication_receipt_valid(const critical_completion &completion)
{
	if (!critical_completion_disposition_valid(completion) ||
	    !critical_failure_stage_valid(completion.failure_stage) ||
	    completion.result_size > completion.result_payload.size())
		return false;
	switch (completion.outcome)
	{
	case critical_apply_outcome::applied:
	case critical_apply_outcome::already_applied:
	case critical_apply_outcome::retryable_failure:
	case critical_apply_outcome::ambiguous_commit:
	case critical_apply_outcome::terminal_failure:
		return true;
	}
	return false;
}

/** Keep the authoritative item receipt before projection or notification. */
bool same_item_publication_receipt(const critical_completion &original,
				   const critical_completion &incoming)
{
	const auto durable_success = [](critical_apply_outcome outcome)
	{
		return outcome == critical_apply_outcome::applied ||
		       outcome == critical_apply_outcome::already_applied;
	};
	return original.operation_id.bytes == incoming.operation_id.bytes &&
	       (original.outcome == incoming.outcome ||
		(durable_success(original.outcome) && durable_success(incoming.outcome))) &&
	       original.disposition == incoming.disposition &&
	       original.durable_revision == incoming.durable_revision &&
	       original.error_code == incoming.error_code &&
	       original.failure_stage == incoming.failure_stage &&
	       original.result_size == incoming.result_size &&
	       original.result_payload == incoming.result_payload;
}

/** Complete craft only after its original ACK; detach before calling external hooks. */
void finalize_craft(std::unordered_map<std::string, pending_movement>::iterator found,
		    const item_transfer_result &result, bool committed, unsigned int error_code,
		    bool never_admitted)
{
	pending_movement &entry = found->second;
	entry.publication_status = publication_state::ack_pending;
	P_char actor = find_registered_craft_player(entry.actor_pid);
	if (!actor)
	{
		account_health();
		return;
	}
	const uint64_t actor_runtime_id = actor->runtime_id;
	const bool recipe = entry.payload.continuation.kind ==
			    item_transfer_continuation_kind::craft_recipe;
	craft_recipe_continuation terms;
	try
	{
		// Decode before ACK: a malformed or unallocatable continuation remains owned.
		if (recipe &&
		    !craft_recipe_continuation_decode(entry.payload.continuation.data, &terms))
		{
			retain_publication_failure(entry, "recipe notification continuation");
			account_health();
			return;
		}
		if (!never_admitted && !critical_command_coordinator_acknowledge_publication(
					       entry.completed.operation_id))
		{
			account_health();
			return;
		}
	}
	catch (...)
	{
		// An ACK allocation failure retains the same owner, effects and operation ID.
		account_health();
		return;
	}

	// Extraction does not allocate. No pending-map iterator/reference survives a hook.
	auto settled = pending.extract(found);
	const pending_movement &completed = settled.mapped();
	const critical_operation_id operation_id = completed.completed.operation_id;
	const auto hooks = craft_progression_hooks;
	const auto completion_fn = completed.completion;
	const auto context = completed.context;
	const size_t context_size = completed.context_size;
	const uint32_t actor_pid = completed.actor_pid;
	if (committed)
		++health.committed;
	else
		++health.rejected;
	account_health();

	if (recipe && terms.discipline != craft_recipe_discipline::refine && !never_admitted &&
	    hooks.acknowledged)
	{
		try
		{
			hooks.acknowledged(operation_id);
		}
		catch (...)
		{
			logit(LOG_FILE, "craft post-ACK progression cleanup failed (pid=%u)",
			      actor_pid);
		}
	}
	actor = find_character_by_runtime_id(actor_runtime_id);
	if (recipe && hooks.notify && actor)
	{
		try
		{
			hooks.notify(actor, committed, terms);
		}
		catch (...)
		{
			logit(LOG_FILE, "craft post-ACK notification failed (pid=%u)", actor_pid);
		}
	}
	// A notification may retire the actor. Never reuse its previous pointer.
	actor = find_character_by_runtime_id(actor_runtime_id);
	if (completion_fn && actor)
	{
		try
		{
			completion_fn(actor, committed, result, error_code, context.data(),
				      context_size);
		}
		catch (...)
		{
			logit(LOG_FILE, "craft post-ACK completion failed (pid=%u)", actor_pid);
		}
	}
	else if (completion_fn)
		logit(LOG_FILE, "craft post-ACK completion actor retired (pid=%u)", actor_pid);
}

/** Publish a completion, retaining committed work if the live registry cannot advance. */
void publish_live_drop(std::unordered_map<std::string, pending_movement>::iterator found,
		       const item_transfer_result &result, bool committed, bool never_admitted)
{
	const std::string key = found->first;
	auto &entry = found->second;
	entry.ordinary_receipt_sealed = true;
	const auto original_receipt = entry.completed;
	if (!entry.live_drop_acknowledged && !never_admitted)
	{
		entry.publication_inflight = true;
		const auto physical = ordinary_drop_recovery_publish_live(*entry.live_drop_command,
									  original_receipt,
									  entry.actor_runtime_id,
									  entry.live_drop_state);
		found = pending.find(key);
		if (found == pending.end())
			return;
		found->second.publication_inflight = false;
		if (!same_item_publication_receipt(original_receipt, found->second.completed))
			found->second.disposition_blocked = true;
		if (found->second.disposition_blocked)
		{
			found->second.publication_status = publication_state::blocked;
			return;
		}
		const bool proven =
			committed ?
				(physical.status == ordinary_drop_observation_status::published ||
				 physical.status ==
					 ordinary_drop_observation_status::verified_existing) :
				(physical.status ==
				 ordinary_drop_observation_status::verified_rejected);
		if (!proven)
		{
			// One bounded attempt per completion pulse, without a lifetime cap
			// that would abandon a recoverable original publication obligation.
			found->second.publication_status = publication_state::owner_waiting;
			return;
		}
	}
	auto &current = found->second;
	if (!current.live_drop_acknowledged)
	{
		current.publication_status = publication_state::ack_pending;
		if (!never_admitted && !critical_command_coordinator_acknowledge_publication(
					       original_receipt.operation_id))
			return;
		current.live_drop_acknowledged = true;
	}
	// An ACK that succeeded is never repeated merely because hold release is
	// pending. The original identity remains owned until release also succeeds.
	if (!player_save_pipeline_literal_inventory_release(current.live_drop_token,
							    original_receipt.operation_id))
		return;
	const auto completion_fn = current.completion;
	const auto context = current.context;
	const size_t context_size = current.context_size;
	const uint32_t actor_pid = current.actor_pid;
	const uint64_t actor_runtime_id = current.actor_runtime_id;
	pending.erase(found);
	if (committed)
		++health.committed;
	else
		++health.rejected;
	P_char actor = find_character_by_runtime_id(actor_runtime_id);
	if (completion_fn && actor && IS_PC(actor) && GET_PID(actor) == static_cast<int>(actor_pid))
	{
		try
		{
			completion_fn(actor, committed, result, original_receipt.error_code,
				      context.data(), context_size);
		}
		catch (...)
		{
			logit(LOG_FILE, "ordinary drop post-ACK notification failed (pid=%u)",
			      actor_pid);
		}
	}
}

void publish_held_retirement_entry(std::unordered_map<std::string, pending_movement>::iterator);

void publish(std::unordered_map<std::string, pending_movement>::iterator found, P_char actor)
{
	pending_movement &entry = found->second;
	entry.publication_attempted_this_batch = true;
	if (entry.disposition_blocked || entry.publication_inflight)
		return;
	if (!item_publication_receipt_valid(entry.completed))
	{
		entry.disposition_blocked = true;
		retain_publication_failure(entry, "invalid_completion_disposition");
		entry.publication_status = publication_state::blocked;
		account_health();
		return;
	}
	const bool never_admitted = entry.completed.disposition ==
				    critical_completion_disposition::never_admitted;
	const bool craft = entry.payload.reason == item_transfer_reason::craft;
	const bool retained = entry.publication || craft;
	if (retained && (entry.completed.outcome == critical_apply_outcome::ambiguous_commit ||
			 entry.completed.outcome == critical_apply_outcome::retryable_failure))
	{
		// Exhausted uncertainty remains owned by coordinator recovery. Never
		// tell the command it failed or acknowledge away its replay record.
		account_health();
		return;
	}
	item_transfer_result result = {};
	const bool decoded = item_transfer_command_decode_result(
		entry.completed.result_payload.data(), entry.completed.result_size, &result);
	if (decoded)
		result.operation_id = entry.completed.operation_id;
	const bool committed = decoded &&
			       (entry.completed.outcome == critical_apply_outcome::applied ||
				entry.completed.outcome == critical_apply_outcome::already_applied);
	const bool durable_outcome = entry.completed.outcome == critical_apply_outcome::applied ||
				     entry.completed.outcome ==
					     critical_apply_outcome::already_applied;
	if (retained && durable_outcome && !decoded)
	{
		// A durable success without a decodable result cannot be safely projected.
		// Keep both the movement record and coordinator fence for repair; never
		// reinterpret it as a rejection and release ownership authority.
		retain_publication_failure(entry, "invalid_result");
		account_health();
		return;
	}
	if (entry.held_command)
	{
		publish_held_retirement_entry(found);
		account_health();
		return;
	}
	if (entry.live_drop_command)
	{
		publish_live_drop(found, result, committed, never_admitted);
		account_health();
		return;
	}
	if (entry.restored_sql_drop)
	{
		// This original restored owner publishes and ACKs as one reserved attempt,
		// before generic registry/callback handling, independently of actor login.
		entry.ordinary_receipt_sealed = true;
		entry.publication_inflight = true;
		const bool acknowledged =
			player_save_restored_publication_owner::publish(entry.completed);
		entry.publication_inflight = false;
		if (!acknowledged)
		{
			entry.publication_status = publication_state::owner_waiting;
			account_health();
			return;
		}
		pending.erase(found);
		if (committed)
			++health.committed;
		else
			++health.rejected;
		account_health();
		return;
	}
	if (entry.publication && !craft)
		entry.ordinary_receipt_sealed = true;
	if (craft)
	{
		actor = find_registered_craft_player(entry.actor_pid);
		if (!actor)
		{
			account_health();
			return;
		}
		// Bind the receipt before registry/physical/progression effects can begin.
		entry.craft_publication_started = true;
		if (entry.craft_publication_ready)
		{
			finalize_craft(found, result, committed,
				       decoded || never_admitted ? entry.completed.error_code :
								   EBADMSG,
				       never_admitted);
			return;
		}
	}
	if (retained && (entry.publication_status == publication_state::ack_pending ||
			 entry.ordinary_publication_ready))
	{
		// Receipt conflicts cannot erase already verified physical publication.
		// Canonical repair resumes only the original ACK/notification phase.
		entry.publication_status = publication_state::ack_pending;
		P_char completion_actor = actor;
		if (entry.completion && !completion_actor)
			completion_actor = find_live_player(entry.actor_pid);
		if (entry.completion && !completion_actor)
		{
			account_health();
			return;
		}
		if (never_admitted || critical_command_coordinator_acknowledge_publication(
					      entry.completed.operation_id))
		{
			const bool was_committed = committed;
			const item_movement_completion_fn completion_fn = entry.completion;
			const auto context = entry.context;
			const size_t context_size = entry.context_size;
			const unsigned int error_code =
				decoded || never_admitted ? entry.completed.error_code : EBADMSG;
			pending.erase(found);
			if (was_committed)
				++health.committed;
			else
				++health.rejected;
			if (completion_fn)
				completion_fn(completion_actor, was_committed, result, error_code,
					      context.data(), context_size);
		}
		account_health();
		return;
	}
	if (committed && result.collector_catalog_changed && !entry.collector_invalidated)
	{
		collector_catalog_cache_invalidate();
		entry.collector_invalidated = true;
	}
	const bool corpse_batch = entry.payload.multi_root &&
				  entry.payload.reason == item_transfer_reason::corpse_create;
	if (committed && corpse_batch && !corpse_batch_live_ready(actor, entry))
	{
		if (!entry.publication_failed)
		{
			entry.publication_failed = true;
			++health.stale_publications;
			logit(LOG_FILE, "corpse batch publication retained (pid=%u)",
			      entry.actor_pid);
		}
		account_health();
		return;
	}
	if (committed && entry.publication && !entry.registry_applied &&
	    !entry.recovered_publication &&
	    (entry.payload.reason == item_transfer_reason::destruction ||
	     entry.payload.reason == item_transfer_reason::quest_turnin) &&
	    entry.payload.from_owner.type == item_owner_type::player)
	{
		if (!actor)
		{
			account_health();
			return;
		}
		if (!destruction_publication_live_ready(actor, entry.payload))
		{
			retain_publication_failure(entry, "live_destruction_tree");
			account_health();
			return;
		}
	}
	bool registry_applied = entry.registry_applied;
	if (committed && !registry_applied)
	{
		registry_applied = item_ownership_runtime_apply(entry.payload, result);
		entry.registry_applied = registry_applied;
	}
	if (committed && !registry_applied)
	{
		if (retained)
			retain_publication_failure(entry, "registry");
		else if (!entry.publication_failed)
		{
			entry.publication_failed = true;
			++health.rejected;
			++health.stale_publications;
		}
		account_health();
		return;
	}
	if (craft)
	{
		if (!actor)
		{
			account_health();
			return;
		}
		if (committed && !publish_craft(entry, actor))
		{
			retain_publication_failure(entry, "craft");
			account_health();
			return;
		}
		if (committed && entry.payload.continuation.kind ==
					 item_transfer_continuation_kind::craft_recipe)
		{
			craft_recipe_continuation terms;
			if (!craft_progression_hooks.publish ||
			    !craft_recipe_continuation_decode(entry.payload.continuation.data,
							      &terms))
			{
				retain_publication_failure(entry, "recipe progression");
				return;
			}
			if (terms.discipline != craft_recipe_discipline::refine)
			{
				const auto progression = craft_progression_hooks.publish(
					entry.completed.operation_id, actor, terms);
				if (progression != craft_progression_publication_result::ready)
				{
					if (progression ==
					    craft_progression_publication_result::waiting)
						entry.publication_status =
							publication_state::owner_waiting;
					else
						retain_publication_failure(entry,
									   "recipe progression");
					account_health();
					return;
				}
			}
		}
		if (!committed)
			discard_craft_outputs(entry);
		entry.craft_publication_ready = true;
		finalize_craft(found, result, committed,
			       decoded || never_admitted ? entry.completed.error_code : EBADMSG,
			       never_admitted);
		return;
	}
	if (entry.publication)
	{
		if (!actor)
		{
			account_health();
			return;
		}
		const std::string pending_key = found->first;
		const critical_completion original_receipt = entry.completed;
		const auto context = entry.context;
		const size_t context_size = entry.context_size;
		const unsigned int error_code =
			decoded || never_admitted ? entry.completed.error_code : EBADMSG;
		bool published = false;
		entry.publication_inflight = true;
		try
		{
			published = entry.publication(entry.completed.operation_id, actor,
						      committed, result, error_code, context.data(),
						      context_size);
		}
		catch (...)
		{
			published = false;
		}
		auto current = pending.find(pending_key);
		if (current == pending.end())
			return;
		current->second.publication_inflight = false;
		const bool same_owner_receipt =
			current->second.completion_ready &&
			same_item_publication_receipt(original_receipt, current->second.completed);
		if (!same_owner_receipt)
		{
			current->second.disposition_blocked = true;
			retain_publication_failure(current->second,
						   "changed_callback_owner_receipt");
			current->second.publication_status = publication_state::blocked;
		}
		if (published && same_owner_receipt)
			current->second.ordinary_publication_ready = true;
		// Reentrant delivery may retain a contradiction while the callback runs.
		// Its return cannot overwrite that state or authorize the original ACK.
		if (current->second.disposition_blocked)
		{
			account_health();
			return;
		}
		if (!published)
		{
			if (spell_component_retirement_waiting_for_effect(
				    current->second.completed.operation_id))
			{
				current->second.publication_status =
					publication_state::owner_waiting;
				account_health();
				return;
			}
			retain_publication_failure(current->second, "callback");
			account_health();
			return;
		}
		if (!never_admitted && !critical_command_coordinator_acknowledge_publication(
					       current->second.completed.operation_id))
		{
			current->second.publication_status = publication_state::ack_pending;
			account_health();
			return;
		}
		const item_movement_completion_fn completion_fn = current->second.completion;
		pending.erase(current);
		if (committed)
			++health.committed;
		else
			++health.rejected;
		if (completion_fn)
			completion_fn(actor, committed, result, error_code, context.data(),
				      context_size);
		account_health();
		return;
	}
	if (committed && entry.payload.reason == item_transfer_reason::corpse_create &&
	    entry.payload.collector.present)
	{
		P_obj corpse = entry.requested_corpse_uid ? find_item(entry.requested_corpse_uid) :
							    NULL;
		collector_death_enrollment_note_committed(corpse, entry.payload);
	}
	if (!entry.creation_batch && entry.completion == creation_grant_completion && committed &&
	    entry.payload.reason == item_transfer_reason::creation)
	{
		auto queue_found = creation_grants.find(entry.actor_pid);
		if (queue_found != creation_grants.end() && queue_found->second.active &&
		    !creation_grant_request_live_ready(actor,
						       queue_found->second.requests.front()) &&
		    !reconcile_creation_grant_batch(actor, entry, queue_found->second, result,
						    false))
		{
			note_creation_grant_publication_failure(actor, queue_found->second,
								entry.actor_pid);
			account_health();
			return;
		}
	}

	if (entry.creation_batch)
	{
		if (committed)
		{
			auto queue_found = creation_grants.find(entry.actor_pid);
			if (queue_found != creation_grants.end() && queue_found->second.active &&
			    queue_found->second.batch_submission &&
			    !creation_grant_batch_live_ready(actor, queue_found->second) &&
			    !reconcile_creation_grant_batch(actor, entry, queue_found->second,
							    result))
			{
				note_creation_grant_publication_failure(actor, queue_found->second,
									entry.actor_pid);
				account_health();
				return;
			}
		}
		const item_movement_completion_fn completion_fn = entry.completion;
		const auto context = entry.context;
		const size_t context_size = entry.context_size;
		const unsigned int error_code = decoded ? entry.completed.error_code : EBADMSG;
		if (completion_fn)
			completion_fn(actor, committed, result, error_code, context.data(),
				      context_size);
		if (committed)
		{
			auto queue_found = creation_grants.find(entry.actor_pid);
			if (queue_found != creation_grants.end() && queue_found->second.active &&
			    queue_found->second.batch_submission &&
			    queue_found->second.publication_failed)
			{
				account_health();
				return;
			}
		}
		pending.erase(found);
		if (committed)
			++health.committed;
		else
			++health.rejected;
		account_health();
		return;
	}
	if (entry.adopting && committed && registry_applied)
	{
		if (entry.adoption_only)
		{
			const item_movement_completion_fn completion_fn = entry.completion;
			const auto context = entry.context;
			const size_t context_size = entry.context_size;
			const bool retain_creation_grant = completion_fn ==
							   creation_grant_completion;
			const std::string pending_key = found->first;
			// Admission has published custody. Ordinary continuations (including
			// coin pickup) must be able to submit without conflicting with it.
			if (!retain_creation_grant)
				pending.erase(found);
			if (completion_fn)
				completion_fn(actor, true, result, 0, context.data(), context_size);
			if (retain_creation_grant)
			{
				auto queue_found = creation_grants.find(entry.actor_pid);
				if (queue_found != creation_grants.end() &&
				    queue_found->second.active &&
				    queue_found->second.publication_failed)
				{
					account_health();
					return;
				}
				pending.erase(pending_key);
			}
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
	if (committed && corpse_batch)
		publish_corpse_batch(entry);
	const item_movement_completion_fn completion_fn = entry.completion;
	const auto context = entry.context;
	const size_t context_size = entry.context_size;
	const unsigned int error_code = decoded ? entry.completed.error_code : EBADMSG;
	const bool retain_creation_grant = committed && completion_fn == creation_grant_completion;
	const bool retain_trusted_steal = committed && registry_applied &&
					  entry.requested_reason ==
						  item_transfer_reason::trusted_steal;
	const uint64_t trusted_steal_uid = entry.payload.selected_item_uid;
	const std::string pending_key = found->first;
	/*
	 * A trusted steal has a second publication boundary after the durable
	 * ownership commit: the exact live UID must reach the thief's carrying list.
	 * Keep that entry fenced until the callback proves the boundary, otherwise a
	 * committed ledger row could strand the live object with no retry path.
	 */
	if (!retain_creation_grant && !retain_trusted_steal)
		pending.erase(found);
	if (completion_fn)
		completion_fn(actor, committed && registry_applied, result, error_code,
			      context.data(), context_size);
	if (retain_trusted_steal)
	{
		auto retained = pending.find(pending_key);
		if (retained != pending.end() &&
		    !trusted_steal_live_ready(actor, trusted_steal_uid))
		{
			retain_trusted_steal_publication(retained->second, trusted_steal_uid);
			account_health();
			return;
		}
		if (retained != pending.end())
			pending.erase(retained);
	}
	else if (retain_creation_grant)
	{
		auto queue_found = creation_grants.find(entry.actor_pid);
		if (queue_found != creation_grants.end() && queue_found->second.active &&
		    queue_found->second.publication_failed)
		{
			account_health();
			return;
		}
		pending.erase(pending_key);
	}
	if (committed && registry_applied)
		++health.committed;
	else
		++health.rejected;
	account_health();
}
}

static bool submit_movement(P_char actor, P_obj root, P_obj target_container,
			    const item_owner_identity &from_owner,
			    const item_owner_identity &to_owner, item_transfer_reason reason,
			    int64_t reason_id, item_movement_completion_fn completion,
			    const void *context, size_t context_size, P_obj corpse_context,
			    item_movement_reject *reject, item_movement_publication_fn publication,
			    economic_source_kind lifecycle_source, uint64_t logical_source_id,
			    const item_transfer_continuation &continuation,
			    const player_literal_inventory_token *live_drop_token)
{
	item_movement_reject discarded = item_movement_reject::none;
	if (!reject)
		reject = &discarded;
	*reject = item_movement_reject::none;
	// Only the typed held-retirement owner may admit kind8.
	if (continuation.kind == static_cast<item_transfer_continuation_kind>(8))
		return reject_with(reject, item_movement_reject::active_accounting_unsupported);
	const bool corpse_transfer = reason == item_transfer_reason::corpse_create ||
				     reason == item_transfer_reason::corpse_loot;
	const bool player_actor = actor && IS_PC(actor) && GET_PID(actor) > 0;
	const bool mobile_actor = actor && IS_NPC(actor) && actor->runtime_id &&
				  reason == item_transfer_reason::mobile_claim &&
				  !target_container && !corpse_context &&
				  item_owner_identity_equal(from_owner, to_owner);
	if ((!player_actor && !mobile_actor) || !root || !root->obj_uid ||
	    context_size > ITEM_MOVEMENT_CONTEXT_MAX_BYTES || (context_size && !context) ||
	    corpse_transfer != (corpse_context != NULL) ||
	    (logical_source_id && (reason != item_transfer_reason::creation ||
				   lifecycle_source == economic_source_kind{} ||
				   lifecycle_source == economic_source_kind::quest_completion)))
		return reject_with(reject, item_movement_reject::invalid_request);
	if (pending.size() + native_quest_pending_count() >= ITEM_MOVEMENT_PENDING_MAX)
		return reject_with(reject, item_movement_reject::queue_saturated);
	item_ownership_runtime_entry runtime = {};
	item_ownership_runtime_entry target_runtime = {};
	uint64_t from_revision = 0, to_revision = 0;
	const bool adopted = item_ownership_runtime_lookup(root->obj_uid, &runtime);
	if (adopted && logical_source_id)
		return reject_with(reject, item_movement_reject::invalid_request);
	const bool retained_owner_identity = adopted && item_owner_identity_valid(runtime.owner);
	const bool owner_matches_request = retained_owner_identity &&
					   item_owner_identity_equal(runtime.owner, from_owner);
	const bool accounting_active = economic_gameplay_authority::active();
	if (live_drop_token &&
	    (!accounting_active || !player_actor || !adopted || target_container ||
	     corpse_context || reason != item_transfer_reason::player_drop ||
	     from_owner.type != item_owner_type::player ||
	     from_owner.id != static_cast<uint64_t>(GET_PID(actor)) || from_owner.context_id ||
	     to_owner.type != item_owner_type::room || to_owner.context_id ||
	     reason_id != static_cast<int64_t>(to_owner.id) ||
	     runtime.root_item_uid != root->obj_uid || runtime.parent_item_uid ||
	     live_drop_token->pid != GET_PID(actor) ||
	     live_drop_token->actor_runtime_id != actor->runtime_id ||
	     live_drop_token->root_uid != root->obj_uid || logical_source_id ||
	     continuation.kind != item_transfer_continuation_kind::none ||
	     !continuation.data.empty() ||
	     !live_drop_shape(actor, root, actor->in_room, static_cast<int32_t>(to_owner.id))))
		return reject_with(reject, item_movement_reject::invalid_request);
	const bool sourced_player_creation =
		!adopted && reason == item_transfer_reason::creation &&
		lifecycle_source != economic_source_kind{} &&
		from_owner.type == item_owner_type::player &&
		from_owner.id == static_cast<uint32_t>(GET_PID(actor)) &&
		to_owner.type == item_owner_type::player && to_owner.id == from_owner.id;
	const bool sourced_room_creation = !adopted && reason == item_transfer_reason::creation &&
					   lifecycle_source != economic_source_kind{} &&
					   from_owner.type == item_owner_type::room &&
					   to_owner.type == item_owner_type::room &&
					   item_owner_identity_equal(from_owner, to_owner);
	const bool sourced_creation = sourced_player_creation || sourced_room_creation;
	if (accounting_active && (!adopted || !owner_matches_request) && !sourced_creation)
		return refuse_active_item_submission(true, retained_owner_identity,
						     owner_matches_request,
						     reason == item_transfer_reason::creation,
						     reject);
	const item_owner_identity effective_from = adopted ? from_owner : system_owner_identity;
	const item_owner_identity effective_to = adopted ? to_owner : from_owner;
	// A mobile claim is evidence about an already-authoritative item. Treating an
	// absent item as creation would erase that evidence and could leave a collector
	// candidate live after the mobile received it.
	if (mobile_actor && !adopted)
		return reject_with(reject, item_movement_reject::owner_mismatch);
	if (movement_conflicts(effective_from, effective_to) || coordinator_item_fenced(root) ||
	    coordinator_item_fenced(target_container) || coin_movement_pending(root) ||
	    coin_movement_pending(target_container))
		return reject_with(reject, item_movement_reject::pending_conflict);
	if ((adopted && !item_owner_identity_equal(runtime.owner, from_owner)) ||
	    (target_container &&
	     (!target_container->obj_uid ||
	      !item_ownership_runtime_lookup(target_container->obj_uid, &target_runtime) ||
	      !item_owner_identity_equal(target_runtime.owner, to_owner))))
		return reject_with(reject, item_movement_reject::owner_mismatch);
	if (live_drop_token)
	{
		if (!item_ownership_runtime_peek_owner_revision(from_owner, &from_revision))
			return reject_with(reject, item_movement_reject::missing_owner_revision);
		// A room without a cached counter keeps the legacy optimistic zero
		// expectation. Native SQL must verify it; no cache row is installed here.
		item_ownership_runtime_peek_owner_revision(to_owner, &to_revision);
	}
	else if (!item_ownership_runtime_owner_revision(from_owner, &from_revision) ||
		 !item_ownership_runtime_owner_revision(to_owner, &to_revision))
		return reject_with(reject, item_movement_reject::missing_owner_revision);
	std::vector<player_item_snapshot> snapshots;
	if (live_drop_token &&
	    player_item_snapshot_tree_capture_literal(root, &snapshots, nullptr) !=
		    player_snapshot_capture_result::ok)
		return reject_with(reject, item_movement_reject::snapshot_failure);
	if (live_drop_token && !live_drop_source_owned(root, from_owner, snapshots))
		return reject_with(reject, item_movement_reject::topology_mismatch);
	if (live_drop_token &&
	    ordinary_drop_recovery_eligibility(snapshots.data(), snapshots.size()) !=
		    inert_item_stage_result::ok)
		return reject_with(reject, item_movement_reject::active_accounting_unsupported);
	std::vector<item_transfer_entry> items;
	try
	{
		items.reserve(ITEM_TRANSFER_MAX_ITEMS);
	}
	catch (const std::bad_alloc &)
	{
		return reject_with(reject, item_movement_reject::allocation_failure);
	}
	if (!(adopted ? capture(root, runtime.root_item_uid, runtime.parent_item_uid, &items) :
			capture_absent(root, root->obj_uid, 0, &items)))
		return reject_with(reject, item_movement_reject::topology_mismatch);
	std::sort(items.begin(), items.end(), [](const auto &left, const auto &right)
		  { return left.item_uid < right.item_uid; });
	uint64_t system_revision = 0;
	if (!adopted &&
	    !item_ownership_runtime_owner_revision(system_owner_identity, &system_revision))
		return reject_with(reject, item_movement_reject::missing_owner_revision);
	item_transfer_payload payload = {
		.from_owner = adopted ? from_owner : system_owner_identity,
		.to_owner = adopted ? to_owner : from_owner,
		.reason = adopted ? reason : item_transfer_reason::creation,
		.reason_id = reason_id,
		.logical_source_id = logical_source_id,
		.expected_from_revision = adopted ? from_revision : system_revision,
		.expected_to_revision = adopted ? to_revision : from_revision,
		.selected_item_uid = root->obj_uid,
		.target_root_item_uid = target_container ? target_runtime.root_item_uid :
							   root->obj_uid,
		.target_parent_item_uid = target_container ? target_container->obj_uid : 0,
		.expected_target_parent_revision = target_container ? target_runtime.item_revision :
								      0,
		.multi_root = false,
		.item_count = static_cast<uint16_t>(items.size()),
		.items = {},
		.item_blob_size = 0,
		.item_blob = {},
		.corpse = {},
		.collector = {},
		.continuation = continuation,
		.native_mobile = {},
		.native_recovery = {}
	};
	for (size_t index = 0; index < items.size(); ++index)
		payload.items[index] = items[index];
	std::vector<uint8_t> item_blob;
	if ((!live_drop_token && player_item_snapshot_tree_capture(root, &snapshots, nullptr) !=
					 player_snapshot_capture_result::ok) ||
	    snapshots.size() != items.size())
		return reject_with(reject, item_movement_reject::snapshot_failure);
	if (reason == item_transfer_reason::player_wear ||
	    reason == item_transfer_reason::player_remove)
	{
		if (snapshots.empty() || snapshots[0].object_uid != root->obj_uid ||
		    reason_id <= 0 || reason_id > ITEM_TRANSFER_MAX_EQUIPMENT_SLOT)
			return reject_with(reject, item_movement_reject::invalid_request);
		snapshots[0].equipment_slot = reason == item_transfer_reason::player_wear ?
						      static_cast<int16_t>(reason_id) :
						      0;
	}
	if (player_item_snapshot_list_encode(snapshots, &item_blob) !=
		    player_snapshot_codec_result::ok ||
	    item_blob.empty() || item_blob.size() > payload.item_blob.size())
		return reject_with(reject, item_movement_reject::snapshot_failure);
	payload.item_blob_size = static_cast<uint32_t>(item_blob.size());
	std::copy(item_blob.begin(), item_blob.end(), payload.item_blob.begin());
	if (adopted && corpse_transfer &&
	    !capture_corpse_metadata(actor, root, corpse_context, reason, &payload.corpse))
		return reject_with(reject, item_movement_reject::invalid_request);
	critical_operation_id operation_id = {};
	critical_command command = {};
	if (!critical_operation_id_generate(&operation_id) ||
	    !collector_death_enrollment_attach(actor, corpse_context, operation_id, snapshots,
					       &payload) ||
	    !item_transfer_command_build(&command, operation_id, payload,
					 critical_source_site::command,
					 critical_deadline_class::interactive))
		return reject_with(reject, item_movement_reject::command_build_failure);
	if (accounting_active &&
	    economic_gameplay_authority::prepare_item_transfer(
		    &command, player_actor ? static_cast<uint32_t>(GET_PID(actor)) : 0,
		    lifecycle_source) != economic_accounting_error::ok)
		return reject_with(reject, item_movement_reject::active_accounting_unsupported);
	if (live_drop_token && command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
		return reject_with(reject, item_movement_reject::active_accounting_unsupported);
	if (live_drop_token)
	{
		// Preserve exactly the command admitted by the coordinator, including
		// fields it otherwise supplies after the producer returns.
		command.publication_required = true;
		command.accepted_at_usec =
			std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::system_clock::now().time_since_epoch())
				.count();
		if (!command.accepted_at_usec || !critical_command_envelope_valid(command))
			return reject_with(reject, item_movement_reject::command_build_failure);
	}
	pending_movement entry = {
		.actor_pid = player_actor ? static_cast<uint32_t>(GET_PID(actor)) : 0,
		.actor_runtime_id = actor->runtime_id,
		.payload = payload,
		.requested_to_owner = to_owner,
		.requested_target_parent_uid = target_container ? target_container->obj_uid : 0,
		.requested_reason = reason,
		.requested_reason_id = reason_id,
		.requested_corpse_uid = corpse_context ? corpse_context->obj_uid : 0,
		.adopting = !adopted,
		.adoption_only = !adopted && item_owner_identity_equal(from_owner, to_owner),
		.completion = completion,
		.publication = publication,
		.context = {},
		.context_size = context_size,
		.completion_ready = false,
		.publication_failed = false,
		.publication_attempts = 0,
		.publication_status = publication ? publication_state::ready :
						    publication_state::none,
		.creation_batch = false,
		.registry_applied = false,
		.recovered_publication = false,
		.collector_invalidated = false,
		.completed = {}
	};
	if (context_size)
		memcpy(entry.context.data(), context, context_size);
	const std::string key = operation_key(operation_id);
	try
	{
		if (live_drop_token)
		{
			entry.live_drop_command = std::make_shared<const critical_command>(command);
			entry.live_drop_token = *live_drop_token;
		}
		const auto inserted = pending.emplace(key, entry);
		if (live_drop_token && !inserted.second)
			return reject_with(reject,
					   item_movement_reject::coordinator_identity_conflict);
	}
	catch (const std::bad_alloc &)
	{
		return reject_with(reject, item_movement_reject::allocation_failure);
	}
	if (live_drop_token &&
	    !player_save_pipeline_literal_inventory_hold(*live_drop_token, operation_id))
	{
		pending.erase(key);
		return reject_with(reject, item_movement_reject::pending_conflict);
	}
	critical_submit_result submitted;
	try
	{
		submitted = publication ? critical_command_coordinator_submit_for_publication(
						  std::move(command)) :
					  critical_command_coordinator_submit(std::move(command));
	}
	catch (...)
	{
		if (!live_drop_token)
			throw;
		// Admission may have retained the original. Never release its hold or
		// replace its ID when admission did not return a definitive refusal.
		++health.submission_failures;
		*reject = item_movement_reject::coordinator_journal_uncertain;
		account_health();
		return true;
	}
	if (submitted == critical_submit_result::journal_uncertain)
	{
		++health.submission_failures;
		*reject = coordinator_reject_reason(submitted);
		account_health();
		// The coordinator retained the original operation ID and fence because
		// the journal rollback itself was uncertain. Keep the live pending entry
		// and let the recovery/shutdown path resolve it; never mint a new ID.
		return true;
	}
	if (submitted != critical_submit_result::accepted &&
	    submitted != critical_submit_result::awaiting_durability &&
	    submitted != critical_submit_result::attached)
	{
		if (live_drop_token &&
		    !player_save_pipeline_literal_inventory_release(*live_drop_token, operation_id))
		{
			pending.find(key)->second.live_drop_admission_refused = true;
			pending.find(key)->second.publication_status =
				publication_state::owner_waiting;
			++health.submission_failures;
			*reject = coordinator_reject_reason(submitted);
			account_health();
			return true;
		}
		pending.erase(key);
		++health.submission_failures;
		return reject_with(reject, coordinator_reject_reason(submitted));
	}
	++health.submitted;
	account_health();
	return true;
}

bool item_movement_transaction_submit(
	P_char actor, P_obj root, P_obj target_container, const item_owner_identity &from_owner,
	const item_owner_identity &to_owner, item_transfer_reason reason, int64_t reason_id,
	item_movement_completion_fn completion, const void *context, size_t context_size,
	P_obj corpse_context, item_movement_reject *reject,
	item_movement_publication_fn publication, economic_source_kind lifecycle_source,
	uint64_t logical_source_id, const item_transfer_continuation &continuation)
{
	return submit_movement(actor, root, target_container, from_owner, to_owner, reason,
			       reason_id, completion, context, context_size, corpse_context, reject,
			       publication, lifecycle_source, logical_source_id, continuation,
			       nullptr);
}

class item_held_retirement_preparation_owner final
{
    public:
	static void pulse(pending_drop_preparation &request) noexcept
	{
#ifndef __NO_MYSQL__
		P_char actor = find_character_by_runtime_id(request.held_token.actor_runtime_id);
		P_obj pick = actor ? actor->equipment[HOLD] : nullptr;
		bool retained = false, refused = true;
		critical_operation_id operation{};
		bool held = false;
		std::string pending_key;
		try
		{
			if (actor && IS_PC(actor) && actor->only.pc &&
			    GET_PID(actor) == request.held_token.pid &&
			    actor->runtime_id == request.held_token.actor_runtime_id && pick &&
			    pick->obj_uid == request.held_token.selected_uid &&
			    pick->type == ITEM_PICK && OBJ_WORN_BY(pick, actor) &&
			    !pick->contains && economic_gameplay_authority::active_regular_sql())
			{
				player_held_retirement_checkpoint_stage stage;
				const auto readiness =
					player_save_pipeline_held_retirement_checkpoint_poll(
						request.held_token, actor, &stage);
				if (readiness == player_literal_inventory_state::pending)
					return;
				if (readiness ==
				    player_literal_inventory_state::database_acknowledged)
				{
					if (pending.size() + native_quest_pending_count() >=
					    ITEM_MOVEMENT_PENDING_MAX)
						return;
					if (!critical_operation_id_generate(&operation) ||
					    !player_save_pipeline_held_retirement_checkpoint_hold(
						    request.held_token, operation))
						return;
					held = true;
					std::vector<player_item_snapshot> before, selected,
						remaining;
					if (!player_save_held_retirement_checkpoint_owner::
						    observe_held(request.held_token, actor,
								 operation, &stage, &before) ||
					    player_item_snapshot_extract_subtree(
						    before, pick->obj_uid, &selected, &remaining) !=
						    player_snapshot_codec_result::ok ||
					    selected.size() != 1 ||
					    selected.front().equipment_slot != HOLD + 1)
						throw std::runtime_error(
							"original held source changed");
					item_ownership_runtime_entry original{};
					std::vector<item_ownership_runtime_entry> source_rows;
					const item_owner_identity source = {
						item_owner_type::player,
						static_cast<uint64_t>(request.held_token.pid), 0
					};
					const item_owner_identity destruction = {
						item_owner_type::destruction, 0, 0
					};
					if (!item_ownership_runtime_lookup(pick->obj_uid,
									   &original) ||
					    !item_ownership_runtime_snapshot_active_root(
						    pick->obj_uid, ITEM_TRANSFER_MAX_ITEMS,
						    &source_rows) ||
					    source_rows.size() != 1 ||
					    original.state != item_custody_state::active ||
					    original.root_item_uid != pick->obj_uid ||
					    original.parent_item_uid ||
					    !item_owner_identity_equal(original.owner, source) ||
					    original.vnum != OBJ_VNUM(pick) ||
					    !original.item_revision ||
					    original.item_revision == UINT64_MAX)
						throw std::runtime_error(
							"original custody changed");
					item_transfer_payload payload{};
					payload.from_owner = source;
					payload.to_owner = destruction;
					payload.reason = item_transfer_reason::destruction;
					payload.reason_id = original.vnum;
					payload.selected_item_uid = original.item_uid;
					if (!item_ownership_runtime_peek_owner_revision(
						    source, &payload.expected_from_revision))
						throw std::runtime_error(
							"original owner revision unavailable");
					(void)item_ownership_runtime_peek_owner_revision(
						destruction, &payload.expected_to_revision);
					payload.item_count = 1;
					payload.items[0] = { original.item_uid,
							     original.root_item_uid,
							     0,
							     original.item_revision,
							     original.vnum,
							     item_custody_state::active };
					payload.continuation.kind =
						static_cast<item_transfer_continuation_kind>(8);
					payload.continuation.data.assign(
						request.context.begin(),
						request.context.begin() + request.context_size);
					std::vector<uint8_t> literal;
					if (player_item_snapshot_list_encode(selected, &literal) !=
						    player_snapshot_codec_result::ok ||
					    literal.size() > payload.item_blob.size())
						throw std::runtime_error(
							"original source encoding failed");
					payload.item_blob_size =
						static_cast<uint32_t>(literal.size());
					std::copy(literal.begin(), literal.end(),
						  payload.item_blob.begin());
					critical_command command;
					if (!lockpick_retirement_payload_valid(payload) ||
					    !item_transfer_command_build(
						    &command, operation, payload,
						    critical_source_site::command,
						    critical_deadline_class::interactive) ||
					    economic_gameplay_authority::prepare_item_transfer(
						    &command, request.held_token.pid,
						    economic_source_kind::intentional_destruction) !=
						    economic_accounting_error::ok)
						throw std::runtime_error(
							"typed original command unsupported");
					command.publication_required = true;
					command.accepted_at_usec =
						std::chrono::duration_cast<std::chrono::microseconds>(
							std::chrono::system_clock::now()
								.time_since_epoch())
							.count();
					if (!held_retirement_command_identity(command))
						throw std::runtime_error(
							"original identity invalid");
					pending_movement entry{};
					entry.actor_pid = request.held_token.pid;
					entry.actor_runtime_id = actor->runtime_id;
					entry.payload = payload;
					entry.requested_to_owner = destruction;
					entry.requested_reason = payload.reason;
					entry.requested_reason_id = payload.reason_id;
					entry.publication = lockpick_retirement_publication;
					entry.context = request.context;
					entry.context_size = request.context_size;
					entry.publication_status = publication_state::ready;
					entry.held_command =
						std::make_shared<const critical_command>(command);
					entry.held_token = request.held_token;
					pending_key = operation_key(operation);
					const auto inserted =
						pending.emplace(pending_key, std::move(entry));
					if (!inserted.second)
						throw std::runtime_error(
							"original pending identity conflict");
					bool released = false;
					const auto admitted =
						player_save_held_retirement_checkpoint_owner::
							submit_owned(request.held_token,
								     std::move(command), &released);
					retained = critical_submit_result_keeps_operation(admitted);
					if (retained)
					{
						++health.submitted;
						refused = false;
					}
					else
						pending.erase(inserted.first);
				}
			}
		}
		catch (...)
		{
			// If pending owns the original after admission uncertainty, do not
			// cancel its held checkpoint or mint a replacement operation.
			auto original = pending.find(pending_key);
			if (original != pending.end() && original->second.held_command)
			{
				retained = true;
				refused = false;
			}
		}
		if (!retained)
		{
			if (held)
				(void)player_save_pipeline_held_retirement_checkpoint_release(
					request.held_token, operation);
			else
				(void)player_save_pipeline_held_retirement_checkpoint_cancel(
					request.held_token);
		}
		request = {};
		account_health();
		if (refused && actor && IS_PC(actor) && actor->only.pc &&
		    find_character_by_runtime_id(actor->runtime_id) == actor)
		{
			try
			{
				send_to_char("Your lockpick cracks, but remains intact.\r\n",
					     actor);
			}
			catch (...)
			{
			}
		}
#else
		(void)request;
#endif
	}
};

bool item_movement_transaction_prepare_sql_lockpick_retirement(
	P_char actor, P_obj pick, const item_transfer_continuation &continuation,
	item_movement_publication_fn publication, item_movement_reject *reject)
{
	item_movement_reject ignored{};
	if (!reject)
		reject = &ignored;
	*reject = item_movement_reject::none;
#ifdef __NO_MYSQL__
	(void)actor;
	(void)pick;
	(void)continuation;
	(void)publication;
	return reject_with(reject, item_movement_reject::active_accounting_unsupported);
#else
	lockpick_retirement_terms terms;
	if (!economic_gameplay_authority::active_regular_sql())
		return reject_with(reject, item_movement_reject::active_accounting_unsupported);
	if (!actor || !IS_PC(actor) || !actor->only.pc || GET_PID(actor) <= 0 ||
	    !actor->runtime_id || find_character_by_runtime_id(actor->runtime_id) != actor ||
	    !pick || actor->equipment[HOLD] != pick || !OBJ_WORN_BY(pick, actor) ||
	    pick->contains || pick->type != ITEM_PICK ||
	    continuation.kind != static_cast<item_transfer_continuation_kind>(8) ||
	    !lockpick_retirement_decode(continuation.data, &terms) ||
	    terms.item_uid != pick->obj_uid ||
	    terms.actor_pid != static_cast<uint32_t>(GET_PID(actor)) ||
	    terms.item_vnum != OBJ_VNUM(pick) || publication != lockpick_retirement_publication ||
	    actor->in_room < 0 || actor->in_room > top_of_world)
		return reject_with(reject, item_movement_reject::invalid_request);
	if (item_movement_transaction_player_busy(actor) ||
	    currency_transaction_player_busy(actor) ||
	    player_save_pipeline_sealed_save_pending(GET_PID(actor)) ||
	    coordinator_item_fenced(pick) ||
	    critical_command_coordinator_is_fenced(
		    { critical_entity_type::player, terms.actor_pid }, nullptr))
		return reject_with(reject, item_movement_reject::pending_conflict);
	size_t actual_uid_count = 0;
	for (P_obj object = object_list; object; object = object->next)
		if (object->obj_uid == pick->obj_uid)
		{
			if (object != pick)
				return reject_with(reject, item_movement_reject::topology_mismatch);
			++actual_uid_count;
		}
	if (actual_uid_count != 1)
		return reject_with(reject, item_movement_reject::topology_mismatch);
	auto available = std::find_if(drop_preparations.begin(), drop_preparations.end(),
				      [](const auto &entry) { return !entry.active; });
	if (available == drop_preparations.end())
		return reject_with(reject, item_movement_reject::queue_saturated);
	pending_drop_preparation request;
	request.held_retirement = true;
	request.room = actor->in_room;
	request.room_vnum = world[actor->in_room].number;
	request.context_size = continuation.data.size();
	std::copy(continuation.data.begin(), continuation.data.end(), request.context.begin());
	const auto started = player_save_pipeline_held_retirement_checkpoint_begin(
		actor, pick, request.room_vnum, &request.held_token);
	if (started == player_literal_inventory_state::refused)
		return reject_with(reject, item_movement_reject::snapshot_failure);
	request.token = { request.held_token.pid, request.held_token.actor_runtime_id, 0,
			  request.held_token.generation };
	request.active = true;
	*available = std::move(request);
	return true;
#endif
}

bool item_movement_transaction_prepare_sql_drop(P_char actor, P_obj root,
						item_movement_completion_fn completion,
						const void *context, size_t context_size,
						item_movement_reject *reject)
{
	item_movement_reject ignored = item_movement_reject::none;
	if (!reject)
		reject = &ignored;
	*reject = item_movement_reject::none;
#ifdef __NO_MYSQL__
	(void)actor;
	(void)root;
	(void)completion;
	(void)context;
	(void)context_size;
	return reject_with(reject, item_movement_reject::active_accounting_unsupported);
#else
	if (!economic_gameplay_authority::active())
		return reject_with(reject, item_movement_reject::active_accounting_unsupported);
	if (!actor || actor->in_room < 0 || actor->in_room > top_of_world ||
	    context_size > ITEM_MOVEMENT_CONTEXT_MAX_BYTES || (context_size && !context) ||
	    !live_drop_shape(actor, root, actor->in_room, world[actor->in_room].number))
		return reject_with(reject, item_movement_reject::invalid_request);
	if (item_movement_transaction_player_busy(actor) ||
	    currency_transaction_player_busy(actor) ||
	    player_save_pipeline_sealed_save_pending(GET_PID(actor)) ||
	    coordinator_item_fenced(root) ||
	    critical_command_coordinator_is_fenced({ critical_entity_type::player,
						     static_cast<uint64_t>(GET_PID(actor)) },
						   nullptr))
		return reject_with(reject, item_movement_reject::pending_conflict);
	item_ownership_runtime_entry owner = {};
	if (!item_ownership_runtime_lookup(root->obj_uid, &owner) ||
	    owner.state != item_custody_state::active || owner.root_item_uid != root->obj_uid ||
	    owner.parent_item_uid || owner.owner.type != item_owner_type::player ||
	    owner.owner.id != static_cast<uint64_t>(GET_PID(actor)) || owner.owner.context_id)
		return reject_with(reject, item_movement_reject::owner_mismatch);
	try
	{
		std::vector<player_item_snapshot> snapshots;
		if (player_item_snapshot_tree_capture_literal(root, &snapshots, nullptr) !=
			    player_snapshot_capture_result::ok ||
		    !live_drop_source_owned(root, owner.owner, snapshots))
			return reject_with(reject, item_movement_reject::topology_mismatch);
		if (ordinary_drop_recovery_eligibility(snapshots.data(), snapshots.size()) !=
		    inert_item_stage_result::ok)
			return reject_with(reject,
					   item_movement_reject::active_accounting_unsupported);
	}
	catch (const std::bad_alloc &)
	{
		return reject_with(reject, item_movement_reject::allocation_failure);
	}
	auto available = std::find_if(drop_preparations.begin(), drop_preparations.end(),
				      [](const auto &entry) { return !entry.active; });
	if (available == drop_preparations.end())
		return reject_with(reject, item_movement_reject::queue_saturated);
	pending_drop_preparation request;
	request.room = actor->in_room;
	request.room_vnum = world[actor->in_room].number;
	request.completion = completion;
	request.context_size = context_size;
	if (context_size)
		memcpy(request.context.data(), context, context_size);
	if (player_save_pipeline_literal_inventory_begin(actor, root, request.room_vnum,
							 &request.token) ==
	    player_literal_inventory_state::refused)
		return reject_with(reject, item_movement_reject::snapshot_failure);
	request.active = true;
	*available = request;
	return true;
#endif
}

void item_movement_transaction_cancel_drop_preparations(void)
{
	for (auto &request : drop_preparations)
		if (request.active)
		{
			// Admission transfers the token into pending before clearing this
			// preparation. Cancellation cannot release an admitted or uncertain
			// original: literal_inventory_cancel explicitly refuses held tokens.
			if (request.held_retirement)
				(void)player_save_pipeline_held_retirement_checkpoint_cancel(
					request.held_token);
			else
				(void)player_save_pipeline_literal_inventory_cancel(request.token);
			request = {};
		}
}

void item_movement_transaction_drop_prepare_pulse(void)
{
#ifndef __NO_MYSQL__
	for (auto &request : drop_preparations)
	{
		if (!request.active)
			continue;
		if (request.held_retirement)
		{
			item_held_retirement_preparation_owner::pulse(request);
			continue;
		}
		P_char actor = find_character_by_runtime_id(request.token.actor_runtime_id);
		P_obj root = find_item(request.token.root_uid);
		if (!economic_gameplay_authority::active() || !actor || !IS_PC(actor) ||
		    GET_PID(actor) != request.token.pid ||
		    !live_drop_shape(actor, root, request.room, request.room_vnum))
		{
			player_save_pipeline_literal_inventory_cancel(request.token);
			request = {};
			continue;
		}
		const auto ready =
			player_save_pipeline_literal_inventory_poll(request.token, actor);
		if (ready == player_literal_inventory_state::pending)
			continue;
		item_movement_reject reject = item_movement_reject::snapshot_failure;
		bool submitted = false;
		if (ready == player_literal_inventory_state::database_acknowledged)
		{
			const item_owner_identity source = {
				item_owner_type::player, static_cast<uint64_t>(request.token.pid), 0
			};
			const item_owner_identity destination = {
				item_owner_type::room, static_cast<uint64_t>(request.room_vnum), 0
			};
			try
			{
				submitted = submit_movement(
					actor, root, nullptr, source, destination,
					item_transfer_reason::player_drop, request.room_vnum,
					request.completion, request.context.data(),
					request.context_size, nullptr, &reject,
					live_drop_publication_marker, {}, 0, {}, &request.token);
			}
			catch (const std::bad_alloc &)
			{
				reject = item_movement_reject::allocation_failure;
			}
		}
		if (!submitted && reject == item_movement_reject::pending_conflict &&
		    player_save_pipeline_literal_inventory_poll(request.token, actor) !=
			    player_literal_inventory_state::refused)
			continue;
		if (submitted)
		{
			request = {};
			continue;
		}
		player_save_pipeline_literal_inventory_cancel(request.token);
		// No command was admitted. Retire preparation before a notification
		// can reenter admission; neither source topology nor authority changed.
		request = {};
		send_to_char("The drop could not be admitted; your item remains unchanged.\r\n",
			     actor);
		logit(LOG_FILE, "ordinary drop preparation refused (pid=%d reason=%s)",
		      GET_PID(actor), item_movement_reject_name(reject));
	}
#endif
}

bool item_movement_transaction_submit_batch(
	P_char actor, P_obj const *roots, size_t root_count, P_obj target_container,
	const item_owner_identity &from_owner, const item_owner_identity &to_owner,
	item_transfer_reason reason, int64_t reason_id, item_movement_completion_fn completion,
	const void *context, size_t context_size, P_obj corpse_context,
	item_movement_reject *reject, item_movement_publication_fn publication,
	economic_source_kind lifecycle_source, uint64_t logical_source_id,
	const item_transfer_continuation &continuation)
{
	item_movement_reject discarded = item_movement_reject::none;
	if (!reject)
		reject = &discarded;
	*reject = item_movement_reject::none;
	const bool corpse_transfer = reason == item_transfer_reason::corpse_loot ||
				     reason == item_transfer_reason::corpse_create;
	const bool creation = from_owner.type == item_owner_type::system &&
			      to_owner.type == item_owner_type::player &&
			      reason == item_transfer_reason::creation;
	if (!actor || IS_NPC(actor) || GET_PID(actor) <= 0 || !roots || !root_count ||
	    root_count > ITEM_TRANSFER_MAX_ITEMS ||
	    context_size > ITEM_MOVEMENT_CONTEXT_MAX_BYTES || (context_size && !context) ||
	    corpse_transfer != (corpse_context != NULL) || (creation && target_container) ||
	    (logical_source_id && (!creation || lifecycle_source == economic_source_kind{} ||
				   lifecycle_source == economic_source_kind::quest_completion)))
		return reject_with(reject, item_movement_reject::invalid_request);
	if (pending.size() + native_quest_pending_count() >= ITEM_MOVEMENT_PENDING_MAX)
		return reject_with(reject, item_movement_reject::queue_saturated);
	if (economic_gameplay_authority::active())
	{
		if (creation)
		{
			for (size_t index = 0; index < root_count; ++index)
			{
				P_obj root = roots[index];
				item_ownership_runtime_entry runtime = {};
				if (!root || !root->obj_uid ||
				    item_ownership_runtime_lookup(root->obj_uid, &runtime))
					return reject_with(reject,
							   item_movement_reject::owner_mismatch);
			}
			if (lifecycle_source == economic_source_kind{} ||
			    to_owner.id != static_cast<uint32_t>(GET_PID(actor)))
				return refuse_active_item_submission(true, false, false, true,
								     reject);
		}
		else
		{
			bool retained_owner_identity = true;
			bool owner_matches_request = true;
			for (size_t index = 0; index < root_count; ++index)
			{
				P_obj root = roots[index];
				item_ownership_runtime_entry runtime = {};
				if (!root || !root->obj_uid)
					return reject_with(reject,
							   item_movement_reject::owner_mismatch);
				if (!item_ownership_runtime_lookup(root->obj_uid, &runtime) ||
				    !item_owner_identity_valid(runtime.owner))
				{
					retained_owner_identity = false;
					owner_matches_request = false;
				}
				else if (!item_owner_identity_equal(runtime.owner, from_owner))
				{
					owner_matches_request = false;
				}
			}
			if (!retained_owner_identity || !owner_matches_request)
				return refuse_active_item_submission(true, retained_owner_identity,
								     owner_matches_request, false,
								     reject);
		}
	}
	if (movement_conflicts(from_owner, to_owner) || coordinator_item_fenced(target_container) ||
	    coin_movement_pending(target_container))
		return reject_with(reject, item_movement_reject::pending_conflict);
	item_ownership_runtime_entry target_runtime = {};
	uint64_t from_revision = 0, to_revision = 0;
	if (target_container &&
	    (!target_container->obj_uid ||
	     !item_ownership_runtime_lookup(target_container->obj_uid, &target_runtime) ||
	     !item_owner_identity_equal(target_runtime.owner, to_owner)))
		return reject_with(reject, item_movement_reject::owner_mismatch);
	if (!item_ownership_runtime_owner_revision(from_owner, &from_revision) ||
	    !item_ownership_runtime_owner_revision(to_owner, &to_revision))
		return reject_with(reject, item_movement_reject::missing_owner_revision);

	std::vector<item_transfer_entry> items;
	std::vector<player_item_snapshot> snapshots;
	std::vector<P_obj> ordered_roots;
	try
	{
		items.reserve(ITEM_TRANSFER_MAX_ITEMS);
		snapshots.reserve(ITEM_TRANSFER_MAX_ITEMS);
		ordered_roots.assign(roots, roots + root_count);
		std::sort(ordered_roots.begin(), ordered_roots.end(),
			  [](P_obj left, P_obj right)
			  {
				  if (!left)
					  return right != NULL;
				  return right && left->obj_uid < right->obj_uid;
			  });
		for (size_t root_index = 0; root_index < root_count; ++root_index)
		{
			P_obj root = ordered_roots[root_index];
			item_ownership_runtime_entry runtime = {};
			if (!root || !root->obj_uid)
				return reject_with(reject, item_movement_reject::owner_mismatch);
			if (coordinator_item_fenced(root) || coin_movement_pending(root))
				return reject_with(reject, item_movement_reject::pending_conflict);
			if (creation)
			{
				if (item_ownership_runtime_lookup(root->obj_uid, &runtime) ||
				    !capture_absent(root, root->obj_uid, 0, &items))
					return reject_with(reject,
							   item_movement_reject::owner_mismatch);
			}
			else
			{
				if (!item_ownership_runtime_lookup(root->obj_uid, &runtime) ||
				    !item_owner_identity_equal(runtime.owner, from_owner))
					return reject_with(reject,
							   item_movement_reject::owner_mismatch);
				if (!capture(root, runtime.root_item_uid, runtime.parent_item_uid,
					     &items))
					return reject_with(reject,
							   item_movement_reject::topology_mismatch);
			}

			std::vector<player_item_snapshot> tree;
			if (player_item_snapshot_tree_capture(root, &tree, nullptr) !=
				    player_snapshot_capture_result::ok ||
			    tree.empty() || tree.size() > ITEM_TRANSFER_MAX_ITEMS ||
			    snapshots.size() > ITEM_TRANSFER_MAX_ITEMS - tree.size())
				return reject_with(reject, item_movement_reject::snapshot_failure);
			const size_t offset = snapshots.size();
			for (player_item_snapshot &snapshot : tree)
			{
				if (snapshot.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
					snapshot.parent_index += static_cast<int32_t>(offset);
				snapshots.push_back(std::move(snapshot));
			}
		}
	}
	catch (const std::bad_alloc &)
	{
		return reject_with(reject, item_movement_reject::allocation_failure);
	}
	if (items.empty() || items.size() != snapshots.size())
		return reject_with(reject, item_movement_reject::snapshot_failure);
	std::sort(items.begin(), items.end(), [](const auto &left, const auto &right)
		  { return left.item_uid < right.item_uid; });
	if (std::adjacent_find(items.begin(), items.end(), [](const auto &left, const auto &right)
			       { return left.item_uid == right.item_uid; }) != items.end())
		return reject_with(reject, item_movement_reject::topology_mismatch);

	item_transfer_payload payload = {
		.from_owner = from_owner,
		.to_owner = to_owner,
		.reason = reason,
		.reason_id = reason_id,
		.logical_source_id = logical_source_id,
		.expected_from_revision = from_revision,
		.expected_to_revision = to_revision,
		.selected_item_uid = 0,
		.target_root_item_uid = target_container ? target_runtime.root_item_uid : 0,
		.target_parent_item_uid = target_container ? target_container->obj_uid : 0,
		.expected_target_parent_revision = target_container ? target_runtime.item_revision :
								      0,
		.multi_root = true,
		.item_count = static_cast<uint16_t>(items.size()),
		.items = {},
		.item_blob_size = 0,
		.item_blob = {},
		.corpse = {},
		.collector = {},
		.continuation = {},
		.native_mobile = {},
		.native_recovery = {}
	};
	try
	{
		payload.continuation = continuation;
	}
	catch (const std::bad_alloc &)
	{
		return reject_with(reject, item_movement_reject::allocation_failure);
	}
	for (size_t index = 0; index < items.size(); ++index)
		payload.items[index] = items[index];
	std::vector<uint8_t> item_blob;
	if (player_item_snapshot_list_encode(snapshots, &item_blob) !=
		    player_snapshot_codec_result::ok ||
	    item_blob.empty() || item_blob.size() > payload.item_blob.size())
		return reject_with(reject, item_movement_reject::snapshot_failure);
	payload.item_blob_size = static_cast<uint32_t>(item_blob.size());
	std::copy(item_blob.begin(), item_blob.end(), payload.item_blob.begin());
	if (corpse_transfer &&
	    !capture_batch_corpse_metadata(actor, ordered_roots.data(), ordered_roots.size(),
					   corpse_context, reason, &payload.corpse))
		return reject_with(reject, item_movement_reject::invalid_request);

	critical_operation_id operation_id = {};
	critical_command command = {};
	if (!critical_operation_id_generate(&operation_id) ||
	    !collector_death_enrollment_attach(actor, corpse_context, operation_id, snapshots,
					       &payload) ||
	    !item_transfer_command_build(&command, operation_id, payload,
					 critical_source_site::command,
					 critical_deadline_class::interactive))
		return reject_with(reject, item_movement_reject::command_build_failure);
	if (economic_gameplay_authority::active() &&
	    economic_gameplay_authority::prepare_item_transfer(
		    &command, static_cast<uint32_t>(GET_PID(actor)), lifecycle_source) !=
		    economic_accounting_error::ok)
		return reject_with(reject, item_movement_reject::active_accounting_unsupported);
	pending_movement entry = {
		.actor_pid = static_cast<uint32_t>(GET_PID(actor)),
		.actor_runtime_id = actor->runtime_id,
		.payload = payload,
		.requested_to_owner = to_owner,
		.requested_target_parent_uid = target_container ? target_container->obj_uid : 0,
		.requested_reason = reason,
		.requested_reason_id = reason_id,
		.requested_corpse_uid = corpse_context ? corpse_context->obj_uid : 0,
		.adopting = false,
		.adoption_only = false,
		.completion = completion,
		.publication = publication,
		.context = {},
		.context_size = context_size,
		.completion_ready = false,
		.publication_failed = false,
		.publication_attempts = 0,
		.publication_status = publication ? publication_state::ready :
						    publication_state::none,
		.creation_batch = creation,
		.registry_applied = false,
		.recovered_publication = false,
		.collector_invalidated = false,
		.completed = {}
	};
	if (context_size)
		memcpy(entry.context.data(), context, context_size);
	const std::string key = operation_key(operation_id);
	try
	{
		pending.emplace(key, std::move(entry));
	}
	catch (const std::bad_alloc &)
	{
		return reject_with(reject, item_movement_reject::allocation_failure);
	}
	const critical_submit_result submitted =
		publication ?
			critical_command_coordinator_submit_for_publication(std::move(command)) :
			critical_command_coordinator_submit(std::move(command));
	if (submitted == critical_submit_result::journal_uncertain)
	{
		++health.submission_failures;
		*reject = coordinator_reject_reason(submitted);
		account_health();
		// The coordinator retained the original operation ID and fence because
		// the journal rollback itself was uncertain. Keep the live pending entry
		// and let the recovery/shutdown path resolve it; never mint a new ID.
		return true;
	}
	if (submitted != critical_submit_result::accepted &&
	    submitted != critical_submit_result::awaiting_durability &&
	    submitted != critical_submit_result::attached)
	{
		pending.erase(key);
		++health.submission_failures;
		return reject_with(reject, coordinator_reject_reason(submitted));
	}
	++health.submitted;
	account_health();
	return true;
}

bool item_movement_transaction_submit_batch(
	P_char actor, P_obj const *roots, size_t root_count, P_obj target_container,
	const item_owner_identity &from_owner, const item_owner_identity &to_owner,
	item_transfer_reason reason, int64_t reason_id, item_movement_completion_fn completion,
	const void *context, size_t context_size, P_obj corpse_context,
	item_movement_reject *reject, item_movement_publication_fn publication,
	economic_source_kind lifecycle_source, uint64_t logical_source_id)
{
	return item_movement_transaction_submit_batch(actor, roots, root_count, target_container,
						      from_owner, to_owner, reason, reason_id,
						      completion, context, context_size,
						      corpse_context, reject, publication,
						      lifecycle_source, logical_source_id, {});
}

bool item_movement_transaction_submit_craft(
	P_char actor, P_obj const *inputs, size_t input_count, P_obj const *outputs,
	size_t output_count, int64_t recipe_id, item_movement_completion_fn completion,
	const void *context, size_t context_size, item_movement_reject *reject,
	P_obj retained_pouch, const chaos_material_pouch_usage *pouch_usage,
	size_t pouch_usage_count, chaos_pouch_usage_mode pouch_mode,
	const craft_recipe_continuation *recipe)
{
	item_movement_reject discarded = item_movement_reject::none;
	if (!reject)
		reject = &discarded;
	*reject = item_movement_reject::none;
	const bool valid_actor = actor && IS_PC(actor) && GET_PID(actor) > 0;
	if (!valid_actor || !inputs || !input_count || input_count > ITEM_TRANSFER_MAX_ITEMS ||
	    (output_count && (!outputs || output_count > ITEM_TRANSFER_MAX_ITEMS)) ||
	    (retained_pouch ? !pouch_usage || !pouch_usage_count ||
				      pouch_usage_count > CRAFT_POUCH_MUTATION_MAX_MATERIALS :
			      pouch_usage || pouch_usage_count) ||
	    context_size > ITEM_MOVEMENT_CONTEXT_MAX_BYTES || (context_size && !context))
		return reject_with(reject, item_movement_reject::invalid_request);
	if (recipe && (!craft_progression_hooks.publish ||
		       (craft_recipe_is_alchemy(recipe->discipline) ||
					recipe->discipline == craft_recipe_discipline::refine ?
				recipe->output_count != output_count :
				output_count != 1 || !outputs[0] ||
					recipe->output_uid != outputs[0]->obj_uid) ||
		       recipe->player_pid != static_cast<uint32_t>(GET_PID(actor)) ||
		       recipe->recipe_vnum != recipe_id || !recipe->pouch_mutation.empty()))
		return reject_with(reject, item_movement_reject::invalid_request);
	if (recipe && recipe->frozen_progression &&
	    ((recipe->discipline != craft_recipe_discipline::craft &&
	      recipe->discipline != craft_recipe_discipline::forge) ||
	     actor->only.pc->skills[recipe->discipline == craft_recipe_discipline::craft ?
					    SKILL_CRAFT :
					    SKILL_FORGE]
			     .learned != static_cast<int>(recipe->progression_skill_before)))
		return reject_with(reject, item_movement_reject::invalid_request);
	if (recipe && recipe->discipline == craft_recipe_discipline::refine &&
	    (!craft_refine_terms_valid(*recipe) || retained_pouch ||
	     !refine_wallet_matches(actor, *recipe, false)))
		return reject_with(reject, item_movement_reject::invalid_request);
	if (pending.size() + native_quest_pending_count() >= ITEM_MOVEMENT_PENDING_MAX)
		return reject_with(reject, item_movement_reject::queue_saturated);
	const item_owner_identity owner = { item_owner_type::player,
					    static_cast<uint64_t>(GET_PID(actor)), 0 };
	if (movement_conflicts(owner, owner))
		return reject_with(reject, item_movement_reject::pending_conflict);
	uint64_t owner_revision = 0;
	if (!item_ownership_runtime_owner_revision(owner, &owner_revision))
		return reject_with(reject, item_movement_reject::missing_owner_revision);
	std::vector<item_transfer_entry> items;
	std::vector<player_item_snapshot> output_snapshots;
	std::unordered_set<uint64_t> input_uids;
	std::unordered_set<uint64_t> output_uids;
	try
	{
		items.reserve(ITEM_TRANSFER_MAX_ITEMS);
		output_snapshots.reserve(ITEM_TRANSFER_MAX_ITEMS);
		input_uids.reserve(ITEM_TRANSFER_MAX_ITEMS);
		output_uids.reserve(ITEM_TRANSFER_MAX_ITEMS);
	}
	catch (const std::bad_alloc &)
	{
		return reject_with(reject, item_movement_reject::allocation_failure);
	}
	for (size_t index = 0; index < input_count; ++index)
	{
		P_obj root = inputs[index];
		item_ownership_runtime_entry runtime = {};
		if (!root || !root->obj_uid || !object_belongs_to_actor(root, actor) ||
		    coordinator_item_fenced(root) || coin_movement_pending(root) ||
		    !item_ownership_runtime_lookup(root->obj_uid, &runtime) ||
		    !item_owner_identity_equal(runtime.owner, owner) ||
		    runtime.root_item_uid != root->obj_uid || runtime.parent_item_uid ||
		    !input_uids.insert(root->obj_uid).second ||
		    !capture(root, runtime.root_item_uid, runtime.parent_item_uid, &items))
			return reject_with(reject, item_movement_reject::topology_mismatch);
	}
	uint64_t consumed_selected_uid = 0;
	for (const auto &item : items)
		if (!consumed_selected_uid || item.root_item_uid < consumed_selected_uid)
			consumed_selected_uid = item.root_item_uid;
	item_transfer_continuation pouch_continuation;
	if (retained_pouch)
	{
		item_ownership_runtime_entry runtime = {};
		std::vector<player_item_snapshot> tree;
		if (!retained_pouch->obj_uid || retained_pouch->contains ||
		    !object_belongs_to_actor(retained_pouch, actor) ||
		    coordinator_item_fenced(retained_pouch) ||
		    coin_movement_pending(retained_pouch) ||
		    !item_ownership_runtime_lookup(retained_pouch->obj_uid, &runtime) ||
		    !item_owner_identity_equal(runtime.owner, owner) ||
		    runtime.state != item_custody_state::active ||
		    std::any_of(items.begin(), items.end(), [&](const auto &item)
				{ return item.item_uid == retained_pouch->obj_uid; }) ||
		    player_item_snapshot_tree_capture(retained_pouch, &tree, nullptr) !=
			    player_snapshot_capture_result::ok ||
		    tree.size() != 1)
			return reject_with(reject, item_movement_reject::topology_mismatch);
		craft_pouch_mutation pouch;
		try
		{
			pouch.before = std::move(tree[0]);
			pouch.mode = pouch_mode;
			pouch.usage.assign(pouch_usage, pouch_usage + pouch_usage_count);
			std::sort(pouch.usage.begin(), pouch.usage.end(),
				  [](const auto &left, const auto &right)
				  { return left.vnum < right.vnum; });
			for (size_t index = 1; index < pouch.usage.size();)
			{
				if (pouch.usage[index - 1].vnum != pouch.usage[index].vnum)
				{
					++index;
					continue;
				}
				if (pouch.usage[index - 1].count >
				    UINT64_MAX - pouch.usage[index].count)
					return reject_with(reject,
							   item_movement_reject::snapshot_failure);
				pouch.usage[index - 1].count += pouch.usage[index].count;
				pouch.usage.erase(pouch.usage.begin() + index);
			}
			if (chaos_pouch_ledger_prepare(pouch.before, pouch.usage, pouch.mode,
						       &pouch.after) !=
				    chaos_pouch_ledger_result::ok ||
			    !craft_pouch_mutation_encode(pouch, &pouch_continuation.data))
				return reject_with(reject, item_movement_reject::snapshot_failure);
			pouch_continuation.kind =
				item_transfer_continuation_kind::craft_pouch_usage;
			items.push_back({ runtime.item_uid, runtime.root_item_uid,
					  runtime.parent_item_uid, runtime.item_revision,
					  runtime.vnum, runtime.state });
			input_uids.insert(runtime.item_uid);
		}
		catch (const std::bad_alloc &)
		{
			return reject_with(reject, item_movement_reject::allocation_failure);
		}
	}
	std::sort(items.begin(), items.end(), [](const auto &left, const auto &right)
		  { return left.item_uid < right.item_uid; });
	if (items.empty() || items.size() > ITEM_TRANSFER_MAX_ITEMS ||
	    std::adjacent_find(items.begin(), items.end(), [](const auto &left, const auto &right)
			       { return left.item_uid == right.item_uid; }) != items.end())
		return reject_with(reject, item_movement_reject::topology_mismatch);
	for (size_t index = 0; index < output_count; ++index)
	{
		P_obj root = outputs[index];
		if (!root || !root->obj_uid || !OBJ_NOWHERE(root) ||
		    coordinator_item_fenced(root) || coin_movement_pending(root))
			return reject_with(reject, item_movement_reject::invalid_request);
		std::vector<player_item_snapshot> tree;
		if (player_item_snapshot_tree_capture(root, &tree, nullptr) !=
			    player_snapshot_capture_result::ok ||
		    tree.empty() || tree.size() > ITEM_TRANSFER_MAX_ITEMS ||
		    output_snapshots.size() > ITEM_TRANSFER_MAX_ITEMS - tree.size())
			return reject_with(reject, item_movement_reject::snapshot_failure);
		const size_t offset = output_snapshots.size();
		for (player_item_snapshot &snapshot : tree)
		{
			if (!snapshot.object_uid ||
			    !output_uids.insert(snapshot.object_uid).second ||
			    input_uids.contains(snapshot.object_uid))
				return reject_with(reject, item_movement_reject::topology_mismatch);
			if (snapshot.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
				snapshot.parent_index += static_cast<int32_t>(offset);
			output_snapshots.push_back(std::move(snapshot));
		}
	}
	if ((output_count && (output_snapshots.empty() ||
			      output_snapshots.front().object_uid != outputs[0]->obj_uid)) ||
	    (!output_count && !output_snapshots.empty()))
		return reject_with(reject, item_movement_reject::snapshot_failure);
	std::vector<uint8_t> item_blob;
	if ((!output_snapshots.empty() &&
	     (player_item_snapshot_list_encode(output_snapshots, &item_blob) !=
		      player_snapshot_codec_result::ok ||
	      item_blob.empty() || item_blob.size() > ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES)) ||
	    (output_count && item_blob.empty()))
		return reject_with(reject, item_movement_reject::snapshot_failure);
	if (recipe)
	{
		try
		{
			craft_recipe_continuation terms = *recipe;
			if (craft_recipe_is_alchemy(terms.discipline) ||
			    terms.discipline == craft_recipe_discipline::refine)
				terms.output_uid = output_count ? outputs[0]->obj_uid :
								  consumed_selected_uid;
			terms.pouch_mutation = std::move(pouch_continuation.data);
			if (!craft_recipe_continuation_encode(terms, &pouch_continuation.data))
				return reject_with(reject,
						   item_movement_reject::command_build_failure);
			pouch_continuation.kind = item_transfer_continuation_kind::craft_recipe;
		}
		catch (const std::bad_alloc &)
		{
			return reject_with(reject, item_movement_reject::allocation_failure);
		}
	}
	item_transfer_payload payload = { .from_owner = owner,
					  .to_owner = owner,
					  .reason = item_transfer_reason::craft,
					  .reason_id = recipe_id,
					  .expected_from_revision = owner_revision,
					  .expected_to_revision = owner_revision,
					  .selected_item_uid = output_count ? outputs[0]->obj_uid :
									      consumed_selected_uid,
					  .target_root_item_uid = 0,
					  .target_parent_item_uid = 0,
					  .expected_target_parent_revision = 0,
					  .multi_root = true,
					  .item_count = static_cast<uint16_t>(items.size()),
					  .items = {},
					  .item_blob_size = static_cast<uint32_t>(item_blob.size()),
					  .item_blob = {},
					  .corpse = {},
					  .collector = {},
					  .continuation = std::move(pouch_continuation),
					  .native_mobile = {},
					  .native_recovery = {} };
	for (size_t index = 0; index < items.size(); ++index)
		payload.items[index] = items[index];
	std::copy(item_blob.begin(), item_blob.end(), payload.item_blob.begin());
	critical_operation_id operation_id = {};
	critical_command command = {};
	if (!critical_operation_id_generate(&operation_id) ||
	    !item_transfer_command_build(&command, operation_id, payload,
					 critical_source_site::command,
					 critical_deadline_class::interactive))
		return reject_with(reject, item_movement_reject::command_build_failure);
	if (economic_gameplay_authority::active() &&
	    economic_gameplay_authority::prepare_item_transfer(
		    &command, static_cast<uint32_t>(GET_PID(actor)),
		    economic_source_kind::crafting) != economic_accounting_error::ok)
		return reject_with(reject, item_movement_reject::active_accounting_unsupported);
	pending_movement entry = { .actor_pid = static_cast<uint32_t>(GET_PID(actor)),
				   .actor_runtime_id = actor->runtime_id,
				   .payload = payload,
				   .requested_to_owner = owner,
				   .requested_target_parent_uid = 0,
				   .requested_reason = item_transfer_reason::craft,
				   .requested_reason_id = recipe_id,
				   .requested_corpse_uid = 0,
				   .adopting = false,
				   .adoption_only = false,
				   .completion = completion,
				   .publication = nullptr,
				   .context = {},
				   .context_size = context_size,
				   .completion_ready = false,
				   .publication_failed = false,
				   .publication_attempts = 0,
				   .publication_status = publication_state::ready,
				   .creation_batch = false,
				   .registry_applied = false,
				   .recovered_publication = false,
				   .collector_invalidated = false,
				   .completed = {} };
	if (context_size)
		memcpy(entry.context.data(), context, context_size);
	const std::string key = operation_key(operation_id);
	try
	{
		pending.emplace(key, std::move(entry));
	}
	catch (const std::bad_alloc &)
	{
		return reject_with(reject, item_movement_reject::allocation_failure);
	}
	const critical_submit_result submitted =
		critical_command_coordinator_submit_for_publication(std::move(command));
	if (submitted == critical_submit_result::journal_uncertain)
	{
		++health.submission_failures;
		*reject = coordinator_reject_reason(submitted);
		account_health();
		return true;
	}
	if (submitted != critical_submit_result::accepted &&
	    submitted != critical_submit_result::awaiting_durability &&
	    submitted != critical_submit_result::attached)
	{
		pending.erase(key);
		++health.submission_failures;
		return reject_with(reject, coordinator_reject_reason(submitted));
	}
	++health.submitted;
	account_health();
	return true;
}

const char *item_movement_reject_name(item_movement_reject reason)
{
	switch (reason)
	{
	case item_movement_reject::none:
		return "none";
	case item_movement_reject::invalid_request:
		return "invalid_request";
	case item_movement_reject::queue_saturated:
		return "queue_saturated";
	case item_movement_reject::pending_conflict:
		return "pending_conflict";
	case item_movement_reject::active_accounting_unsupported:
		return "active_accounting_unsupported";
	case item_movement_reject::missing_owner_identity:
		return "missing_owner_identity";
	case item_movement_reject::owner_mismatch:
		return "owner_mismatch";
	case item_movement_reject::missing_owner_revision:
		return "missing_owner_revision";
	case item_movement_reject::topology_mismatch:
		return "topology_mismatch";
	case item_movement_reject::snapshot_failure:
		return "snapshot_failure";
	case item_movement_reject::allocation_failure:
		return "allocation_failure";
	case item_movement_reject::command_build_failure:
		return "command_build_failure";
	case item_movement_reject::coordinator_unavailable:
		return "coordinator_unavailable";
	case item_movement_reject::coordinator_overloaded:
		return "coordinator_overloaded";
	case item_movement_reject::coordinator_invalid:
		return "coordinator_invalid";
	case item_movement_reject::coordinator_identity_conflict:
		return "coordinator_identity_conflict";
	case item_movement_reject::coordinator_journal_failure:
		return "coordinator_journal_failure";
	case item_movement_reject::coordinator_journal_uncertain:
		return "coordinator_journal_uncertain";
	case item_movement_reject::coordinator_rejected:
		return "coordinator_rejected";
	}
	return "unknown";
}

// Transient rejections clear on their own once the in-flight work drains, so telling the
// player to retry is honest. The rest describe ledger state that disagrees with the live
// world and will keep failing until it is reconciled; retry advice there is a lie.
bool item_movement_reject_is_transient(item_movement_reject reason)
{
	return reason == item_movement_reject::pending_conflict ||
	       reason == item_movement_reject::queue_saturated ||
	       reason == item_movement_reject::allocation_failure ||
	       reason == item_movement_reject::coordinator_unavailable ||
	       reason == item_movement_reject::coordinator_overloaded;
}

bool item_creation_grant_submit_to_player(P_char actor, P_obj object, P_char recipient,
					  P_obj target_container, economic_source_kind source,
					  uint64_t source_id)
{
	return queue_creation_grant(actor, object, recipient, NOWHERE, target_container, false,
				    false, nullptr, nullptr, 0, nullptr, source, source_id);
}

bool item_creation_grant_submit_to_player_with_completion(
	P_char actor, P_obj object, P_char recipient, item_movement_completion_fn completion,
	const void *context, size_t context_size, P_obj target_container,
	economic_source_kind source, uint64_t source_id)
{
	if (!completion)
		return false;
	return queue_creation_grant(actor, object, recipient, NOWHERE, target_container, false,
				    false, completion, context, context_size, nullptr, source,
				    source_id);
}

bool item_creation_grant_submit_to_player_with_completion(
	P_char actor, P_obj object, P_char recipient, P_obj target_container,
	item_creation_grant_completion_fn completion, economic_source_kind source,
	uint64_t source_id)
{
	if (!completion)
		return false;
	return queue_creation_grant(actor, object, recipient, NOWHERE, target_container, false,
				    false, nullptr, nullptr, 0, completion, source, source_id);
}

/** Reserve the player before any legacy kit objects or persistence work exist. */
bool item_creation_grant_defer(P_char actor, item_creation_prepare_fn prepare,
			       economic_source_kind source, uint64_t source_id)
{
	if (!actor || IS_NPC(actor) || GET_PID(actor) <= 0 || !prepare ||
	    item_movement_transaction_player_busy(actor) ||
	    creation_grants.size() >= ITEM_MOVEMENT_PENDING_MAX)
		return false;
	const uint32_t pid = static_cast<uint32_t>(GET_PID(actor));
	try
	{
		creation_grant_queue queue;
		queue.prepare = std::move(prepare);
		queue.source = source;
		queue.source_id = source_id;
		queue.batch_submission = true;
		queue.blocks_actor_commands = true;
		queue.stop_on_failure = true;
		preparation_order.push_back(pid);
		try
		{
			creation_grants.emplace(pid, std::move(queue));
		}
		catch (...)
		{
			preparation_order.pop_back();
			throw;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

/** Round-robin preparation: at most eight roots per player and 32 steps per pulse. */
void item_creation_grant_prepare_pulse(void)
{
	size_t budget = 32;
	size_t players = preparation_order.size();
	while (players-- && budget && !preparation_order.empty())
	{
		const uint32_t pid = preparation_order.front();
		preparation_order.pop_front();
		auto found = creation_grants.find(pid);
		if (found == creation_grants.end() || !found->second.prepare)
		{
			--budget;
			continue;
		}
		P_char actor = find_live_player(pid);
		if (!actor)
		{
			--budget;
			preparation_order.push_back(pid);
			continue;
		}
		creation_grant_queue &queue = found->second;
		bool failed = false;
		for (size_t step = 0; step < 8 && budget && queue.prepare; ++step)
		{
			--budget;
			P_obj object = nullptr;
			item_creation_prepare_result result = item_creation_prepare_result::failed;
			try
			{
				result = queue.prepare(actor, &object);
				if (object)
				{
					if (!object->obj_uid || !OBJ_NOWHERE(object) ||
					    queue.requests.size() +
							    queue.following_requests.size() >=
						    ITEM_CREATION_GRANT_MAX_ROOTS)
						result = item_creation_prepare_result::failed;
					else
					{
						pending_creation_grant request = {
							object->obj_uid, 0,	pid,
							NOWHERE,	 false, true
						};
						request.source = queue.source;
						request.source_id = queue.source_id;
						queue.requests.push_back(request);
						object = nullptr;
					}
				}
			}
			catch (const std::bad_alloc &)
			{
				result = item_creation_prepare_result::failed;
			}
			if (object && OBJ_NOWHERE(object))
				extract_obj(object, FALSE);
			if (result == item_creation_prepare_result::failed ||
			    (result == item_creation_prepare_result::ready &&
			     queue.requests.empty()))
			{
				failed = true;
				break;
			}
			if (result == item_creation_prepare_result::ready)
				queue.prepare = {};
		}
		if (failed)
		{
			discard_creation_queue(actor, queue);
			finish_creation_queue(pid);
		}
		else if (queue.prepare)
			preparation_order.push_back(pid);
	}
	pump_creation_grants();
}

bool item_creation_grant_submit_to_player_before_entry(P_char actor, P_obj object, P_char recipient,
						       economic_source_kind source,
						       uint64_t source_id)
{
	return queue_creation_grant(actor, object, recipient, NOWHERE, NULL, false, true, nullptr,
				    nullptr, 0, nullptr, source, source_id);
}

bool item_creation_grant_submit_to_player_before_entry_with_completion(
	P_char actor, P_obj object, P_char recipient, item_creation_grant_completion_fn completion,
	economic_source_kind source, uint64_t source_id)
{
	if (!completion)
		return false;
	return queue_creation_grant(actor, object, recipient, NOWHERE, NULL, false, true, nullptr,
				    nullptr, 0, completion, source, source_id);
}

bool item_creation_grant_submit_batch_to_player_before_entry(P_char actor, P_obj const *objects,
							     size_t count, P_char recipient,
							     economic_source_kind source,
							     uint64_t source_id)
{
	if (!actor || IS_NPC(actor) || GET_PID(actor) <= 0 || recipient != actor || !objects ||
	    !count || count > ITEM_CREATION_GRANT_MAX_ROOTS ||
	    (source_id && (source == economic_source_kind{} ||
			   source == economic_source_kind::quest_completion)))
		return false;
	const uint32_t actor_pid = static_cast<uint32_t>(GET_PID(actor));
	if (creation_grants.find(actor_pid) != creation_grants.end())
		return false;
	for (size_t i = 0; i < count; ++i)
	{
		if (!objects[i] || !objects[i]->obj_uid || !OBJ_NOWHERE(objects[i]))
			return false;
		for (size_t j = 0; j < i; ++j)
			if (objects[i]->obj_uid == objects[j]->obj_uid)
				return false;
	}

	// Stage the complete queue before submission. Admission failures retain
	// all roots with the caller; accepted roots use the existing coordinator.
	try
	{
		creation_grant_queue queue;
		queue.blocks_actor_commands = true;
		queue.batch_submission = true;
		queue.source = source;
		queue.announce_on_completion = true;
		queue.stop_on_failure = true;
		for (size_t i = 0; i < count; ++i)
		{
			pending_creation_grant request = {
				objects[i]->obj_uid, 0, actor_pid, NOWHERE, false, true
			};
			request.source = source;
			request.source_id = source_id;
			queue.requests.push_back(request);
		}
		if (!creation_grants.emplace(actor_pid, std::move(queue)).second)
			return false;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	auto found = creation_grants.find(actor_pid);
	item_movement_reject reject = item_movement_reject::none;
	if (start_creation_grant(actor, found->second, &reject))
		return true;
	if (item_movement_reject_is_transient(reject))
		return true;
	creation_grants.erase(found);
	return false;
}

bool item_creation_grant_submit_to_room(P_char actor, P_obj object, int room,
					economic_source_kind source,
					item_creation_grant_completion_fn completion,
					uint64_t source_id)
{
	return queue_creation_grant(actor, object, NULL, room, NULL, true, false, nullptr, nullptr,
				    0, completion, source, source_id);
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

bool item_creation_grant_batches_pending(void)
{
	return std::any_of(creation_grants.begin(), creation_grants.end(),
			   [](const auto &entry) { return entry.second.stop_on_failure; });
}

void item_creation_grant_cancel_batch_before_entry(P_char actor)
{
	if (!actor || IS_NPC(actor) || GET_PID(actor) <= 0 ||
	    (actor->desc && actor->desc->connected == CON_PLAYING))
		return;
	auto found = creation_grants.find(static_cast<uint32_t>(GET_PID(actor)));
	if (found == creation_grants.end() || !found->second.stop_on_failure)
		return;
	creation_grant_queue &queue = found->second;
	const size_t retained =
		queue.active ? (queue.batch_submission ? queue.requests.size() : 1) : 0;
	size_t extracted = 0;
	while (queue.requests.size() > retained)
	{
		P_obj object = find_item(queue.requests.back().item_uid);
		if (object && OBJ_NOWHERE(object))
		{
			extract_obj(object, FALSE);
			++extracted;
		}
		queue.requests.pop_back();
	}
	logit(LOG_COMM,
	      "item creation grant batch cancelled before entry (pid=%d retained_active=%d "
	      "extracted_tail=%zu)",
	      GET_PID(actor), retained ? 1 : 0, extracted);
	// An active head is already journaled. Keep its deferred completion so
	// durable ownership/revision publication is never silently abandoned.
	queue.stop_on_failure = false;
	queue.blocks_actor_commands = false;
	queue.announce_on_completion = false;
	if (queue.requests.empty())
	{
		preparation_order.erase(std::remove(preparation_order.begin(),
						    preparation_order.end(),
						    static_cast<uint32_t>(GET_PID(actor))),
					preparation_order.end());
		finish_creation_queue(static_cast<uint32_t>(GET_PID(actor)));
	}
}

void retry_publications(void)
{
	std::vector<std::string> retry_keys;
	try
	{
		retry_keys.reserve(pending.size());
		for (const auto &[key, entry] : pending)
			if (entry.live_drop_admission_refused ||
			    ((entry.publication ||
			      entry.payload.reason == item_transfer_reason::craft) &&
			     entry.completion_ready && !entry.disposition_blocked &&
			     !entry.publication_inflight &&
			     !entry.publication_attempted_this_batch &&
			     (entry.publication_status == publication_state::ready ||
			      entry.publication_status == publication_state::retrying ||
			      entry.publication_status == publication_state::owner_waiting ||
			      entry.publication_status == publication_state::ack_pending)))
				retry_keys.push_back(key);
	}
	catch (const std::bad_alloc &)
	{
		return;
	}
	for (const std::string &key : retry_keys)
	{
		auto found = pending.find(key);
		if (found == pending.end() || found->second.disposition_blocked ||
		    found->second.publication_inflight ||
		    found->second.publication_attempted_this_batch)
			continue;
		if (found->second.live_drop_admission_refused)
		{
			// Definitive admission refusal created no critical journal owner.
			// Retry only release of its bound checkpoint, never SQL or ACK.
			auto &entry = found->second;
			if (!player_save_pipeline_literal_inventory_release(
				    entry.live_drop_token, entry.live_drop_command->operation_id))
				continue;
			const auto completion = entry.completion;
			const auto context = entry.context;
			const auto context_size = entry.context_size;
			const auto pid = entry.actor_pid;
			const auto runtime_id = entry.actor_runtime_id;
			pending.erase(found);
			P_char actor = find_character_by_runtime_id(runtime_id);
			if (completion && actor && IS_PC(actor) &&
			    GET_PID(actor) == static_cast<int>(pid))
			{
				try
				{
					completion(actor, false, {}, EAGAIN, context.data(),
						   context_size);
				}
				catch (...)
				{
					logit(LOG_FILE,
					      "ordinary drop refusal notification failed (pid=%u)",
					      pid);
				}
			}
			continue;
		}
		if (found->second.publication_status == publication_state::ack_pending)
		{
			publish(found, nullptr);
			continue;
		}
		if (found->second.held_command || found->second.restored_sql_drop ||
		    found->second.live_drop_command)
		{
			publish(found, nullptr);
			continue;
		}
		if (!found->second.actor_pid)
			continue;
		if (P_char actor = find_live_player(found->second.actor_pid))
			publish(found, actor);
	}
}

void item_movement_transaction_handle_completions(const critical_completion *completions,
						  size_t count)
{
	if (count && !completions)
		return;
	for (auto &[key, entry] : pending)
	{
		(void)key;
		entry.disposition_blocked_this_batch = false;
		entry.publication_attempted_this_batch = false;
	}
	// Validate the complete incoming batch before retry, registry mutation or
	// notification. A contradictory receipt must not race an old ACK-pending
	// result, and a later duplicate in this batch cannot clear that contradiction.
	for (size_t index = 0; index < count; ++index)
	{
		auto found = pending.find(operation_key(completions[index].operation_id));
		if (found == pending.end())
			continue;
		auto &entry = found->second;
		if (entry.disposition_blocked_this_batch)
			continue;
		if (!item_publication_receipt_valid(completions[index]) ||
		    (entry.completion_ready &&
		     entry.completed.disposition != completions[index].disposition))
		{
			entry.disposition_blocked = true;
			entry.disposition_blocked_this_batch = true;
			retain_publication_failure(entry, "invalid_completion_disposition");
			entry.publication_status = publication_state::blocked;
			continue;
		}
		if ((entry.craft_publication_started || entry.ordinary_receipt_sealed) &&
		    !same_item_publication_receipt(entry.completed, completions[index]))
		{
			entry.disposition_blocked = true;
			entry.disposition_blocked_this_batch = true;
			retain_publication_failure(entry,
						   entry.craft_publication_started ?
							   "changed_craft_publication_receipt" :
							   "changed_item_publication_receipt");
			entry.publication_status = publication_state::blocked;
			continue;
		}
		entry.completed = completions[index];
		entry.completion_ready = true;
		entry.disposition_blocked = false;
		// Bind a definitive well-typed receipt during batch validation, before
		// collector/registry/physical effects. Uncertain or malformed success
		// remains repairable and cannot authorize publication.
		if (entry.publication && entry.payload.reason != item_transfer_reason::craft &&
		    entry.completed.outcome != critical_apply_outcome::retryable_failure &&
		    entry.completed.outcome != critical_apply_outcome::ambiguous_commit)
		{
			item_transfer_result sealed_result = {};
			if (entry.completed.disposition ==
				    critical_completion_disposition::never_admitted ||
			    item_transfer_command_decode_result(
				    entry.completed.result_payload.data(),
				    entry.completed.result_size, &sealed_result))
				entry.ordinary_receipt_sealed = true;
		}
	}
	for (size_t index = 0; index < count; ++index)
	{
		auto found = pending.find(operation_key(completions[index].operation_id));
		if (found == pending.end() || found->second.disposition_blocked ||
		    found->second.publication_inflight ||
		    found->second.publication_attempted_this_batch)
			continue;
		const auto &completion = found->second.completed;
		item_transfer_result result = {};
		if (!found->second.collector_invalidated &&
		    (completion.outcome == critical_apply_outcome::applied ||
		     completion.outcome == critical_apply_outcome::already_applied) &&
		    item_transfer_command_decode_result(completion.result_payload.data(),
							completion.result_size, &result) &&
		    result.collector_catalog_changed)
		{
			collector_catalog_cache_invalidate();
			found->second.collector_invalidated = true;
		}
		if (found->second.held_command || found->second.restored_sql_drop ||
		    found->second.live_drop_command)
			publish(found, nullptr);
		else if ((found->second.publication ||
			  found->second.payload.reason == item_transfer_reason::craft) &&
			 (found->second.publication_status == publication_state::ack_pending ||
			  found->second.ordinary_publication_ready))
			publish(found, nullptr);
		else if (found->second.actor_pid)
		{
			P_char actor = find_live_player(found->second.actor_pid);
			if (!actor &&
			    retained_player_transfer_reason(found->second.requested_reason) &&
			    found->second.payload.to_owner.type == item_owner_type::player &&
			    found->second.payload.to_owner.id <= INT32_MAX)
				actor = find_live_player(
					static_cast<uint32_t>(found->second.payload.to_owner.id));
			if (actor)
				publish(found, actor);
		}
		else if (found->second.actor_runtime_id)
			// A vanished mobile cannot receive the live object, but the committed
			// boundary must still advance the authority projection and release its
			// fence. Publishing with a null actor deliberately skips the callback's
			// live move.
			publish(found, find_live_mobile(found->second.actor_runtime_id));
	}
	native_quest_handle_completions(completions, count);
	retry_publications();
	pump_creation_grants();
	account_health();
}

bool item_movement_transaction_restore_replayed_command(const critical_command &command)
{
	if (command.type != critical_command_type::item_transfer || !command.publication_required)
		return true;
	const std::string key = operation_key(command.operation_id);
	if (pending.find(key) != pending.end())
		return true;
	item_transfer_payload payload = {};
	if (!item_transfer_command_decode_payload(command, &payload) ||
	    payload.from_owner.type != item_owner_type::player || !payload.from_owner.id ||
	    payload.from_owner.id > UINT32_MAX)
		return false;
	if (payload.continuation.kind == static_cast<item_transfer_continuation_kind>(8))
		return false; // Requires the original HRT1 envelope and typed hold.
#ifndef __NO_MYSQL__
	const bool ordinary_sql_drop = command.schema_version ==
					       CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
				       payload.reason == item_transfer_reason::player_drop &&
				       payload.to_owner.type == item_owner_type::room;
#else
	const bool ordinary_sql_drop = false;
#endif
	const bool quest_offering = payload.reason == item_transfer_reason::quest_turnin &&
				    payload.continuation.kind ==
					    item_transfer_continuation_kind::quest_offering;
	const bool forced_room_drop = item_transfer_forced_weapon_drop(payload.reason) &&
				      payload.continuation.kind ==
					      item_transfer_continuation_kind::none &&
				      payload.to_owner.type == item_owner_type::room;
	const bool account_reward_retirement =
		payload.continuation.kind ==
		item_transfer_continuation_kind::account_reward_retirement;
	const bool account_reward_duplicate_promotion =
		payload.continuation.kind ==
		item_transfer_continuation_kind::account_reward_duplicate_promotion;
	const bool spell_component_retirement =
		payload.continuation.kind ==
		item_transfer_continuation_kind::spell_component_retirement;
	std::array<uint8_t, ITEM_MOVEMENT_CONTEXT_MAX_BYTES> spell_context = {};
	size_t spell_context_size = 0;
	uint32_t spell_effect_id = 0;
	uint32_t spell_receipt_owner_pid = 0;
	if (spell_component_retirement && !spell_component_retirement_restore_context(
						  payload, &spell_context, &spell_context_size,
						  &spell_effect_id, &spell_receipt_owner_pid))
		return false;
	const item_movement_publication_fn publication =
		account_reward_duplicate_promotion ?
			account_reward_duplicate_promotion_publication :
			(account_reward_retirement ?
				 account_reward_retirement_publication :
				 (spell_component_retirement ?
					  spell_component_retirement_replayed_publication :
					  ((quest_offering || forced_room_drop) ?
						   recovered_durable_item_publication :
						   unresolved_replay_publication)));
	const void *replay_context =
		(account_reward_retirement || account_reward_duplicate_promotion) ?
			payload.continuation.data.data() :
			(spell_component_retirement ? spell_context.data() : nullptr);
	const size_t replay_context_size =
		(account_reward_retirement || account_reward_duplicate_promotion) ?
			payload.continuation.data.size() :
			(spell_component_retirement ? spell_context_size : 0);
	if (!item_movement_transaction_restore_replayed_publication(
		    command, publication, replay_context, replay_context_size))
		return false;
	if (ordinary_sql_drop)
	{
		if (!player_save_pipeline_restore_sql_drop_obligation(command))
		{
			pending.erase(key);
			return false;
		}
		pending.find(key)->second.restored_sql_drop = true;
		pending.find(key)->second.publication_attempts = 0;
	}
	return !spell_component_retirement ||
	       spell_component_retirement_restore_replayed_effect(
		       command.operation_id, static_cast<uint32_t>(payload.from_owner.id),
		       spell_effect_id, spell_receipt_owner_pid);
}

bool item_movement_transaction_pending_craft_progression(
	uint32_t actor_pid, std::vector<critical_operation_id> *operations)
{
	if (!actor_pid || !operations)
		return false;
	try
	{
		std::vector<critical_operation_id> found;
		for (const auto &[key, entry] : pending)
		{
			if (entry.actor_pid != actor_pid ||
			    entry.payload.continuation.kind !=
				    item_transfer_continuation_kind::craft_recipe)
				continue;
			if (key.size() != 16 || found.size() >= PLAYER_CRAFT_RECEIPT_MAX)
				return false;
			critical_operation_id operation = {};
			memcpy(operation.bytes.data(), key.data(), operation.bytes.size());
			found.push_back(operation);
		}
		std::sort(found.begin(), found.end(), [](const auto &left, const auto &right)
			  { return left.bytes < right.bytes; });
		*operations = std::move(found);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool item_movement_transaction_pending_spell_effects(uint32_t actor_pid,
						     std::vector<critical_operation_id> *operations)
{
	if (!actor_pid || !operations)
		return false;
	try
	{
		std::vector<critical_operation_id> found;
		for (const auto &[key, entry] : pending)
		{
			if (entry.actor_pid != actor_pid ||
			    entry.payload.continuation.kind !=
				    item_transfer_continuation_kind::spell_component_retirement)
				continue;
			if (key.size() != 16 || found.size() >= ITEM_MOVEMENT_PENDING_MAX)
				return false;
			critical_operation_id operation_id = {};
			memcpy(operation_id.bytes.data(), key.data(), operation_id.bytes.size());
			found.push_back(operation_id);
		}
		if (!spell_component_retirement_append_owner_operations(actor_pid, &found))
			return false;
		*operations = std::move(found);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool item_movement_transaction_restore_replayed_publication(
	const critical_command &command, item_movement_publication_fn publication,
	const void *context, size_t context_size)
{
	if (command.type != critical_command_type::item_transfer || !command.publication_required ||
	    !publication || context_size > ITEM_MOVEMENT_CONTEXT_MAX_BYTES ||
	    (context_size && !context))
		return false;
	const std::string key = operation_key(command.operation_id);
	if (pending.find(key) != pending.end() ||
	    pending.size() + native_quest_pending_count() >= ITEM_MOVEMENT_PENDING_MAX)
		return false;
	item_transfer_payload payload = {};
	if (!item_transfer_command_decode_payload(command, &payload) ||
	    payload.from_owner.type != item_owner_type::player || !payload.from_owner.id ||
	    payload.from_owner.id > UINT32_MAX)
		return false;
	if (payload.continuation.kind == static_cast<item_transfer_continuation_kind>(8))
		return false; // Requires the original HRT1 envelope and typed hold.
	const item_owner_identity to_owner = payload.to_owner;
	const uint64_t target_parent_uid = payload.target_parent_item_uid;
	const item_transfer_reason reason = payload.reason;
	const int64_t reason_id = payload.reason_id;
	pending_movement entry = {
		.actor_pid = static_cast<uint32_t>(payload.from_owner.id),
		.actor_runtime_id = 0,
		.payload = std::move(payload),
		.requested_to_owner = to_owner,
		.requested_target_parent_uid = target_parent_uid,
		.requested_reason = reason,
		.requested_reason_id = reason_id,
		.requested_corpse_uid = 0,
		.adopting = false,
		.adoption_only = false,
		.completion = nullptr,
		.publication = publication,
		.context = {},
		.context_size = context_size,
		.completion_ready = false,
		.publication_failed = false,
		.publication_attempts =
			(publication == unresolved_replay_publication ||
			 publication == spell_component_retirement_replayed_publication) ?
				ITEM_MOVEMENT_PUBLICATION_MAX_ATTEMPTS - 1 :
				0,
		.publication_status = publication_state::ready,
		.creation_batch = false,
		// The durable item repository already owns the replayed result. Runtime
		// inventory is hydrated from that authority when the owner loads.
		.registry_applied = true,
		.recovered_publication = true,
		.collector_invalidated = false,
		.completed = {}
	};
	if (context_size)
		memcpy(entry.context.data(), context, context_size);
	try
	{
		pending.emplace(key, std::move(entry));
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

/** Publish ready work for a player, stopping if a stale completion must remain held. */
void item_movement_transaction_player_ready(P_char actor)
{
	if (!actor || IS_NPC(actor) || GET_PID(actor) <= 0)
		return;
	for (;;)
	{
		auto found = std::find_if(
			pending.begin(), pending.end(),
			[&](const auto &entry)
			{
				const bool source_ready = entry.second.actor_pid ==
							  static_cast<uint32_t>(GET_PID(actor));
				const bool destination_ready =
					retained_player_transfer_reason(
						entry.second.requested_reason) &&
					entry.second.payload.to_owner.type ==
						item_owner_type::player &&
					entry.second.payload.to_owner.id ==
						static_cast<uint64_t>(GET_PID(actor));
				return !entry.second.disposition_blocked &&
				       (source_ready || destination_ready) &&
				       entry.second.completion_ready;
			});
		if (found == pending.end())
			break;
		/* publish may invoke a callback that inserts and rehashes pending. */
		const std::string key = found->first;
		if ((found->second.publication ||
		     found->second.payload.reason == item_transfer_reason::craft) &&
		    found->second.publication_status == publication_state::blocked)
		{
			// A reconnect/reload is an explicit recovery edge, not a pulse loop.
			found->second.publication_attempts = 0;
			found->second.publication_status = publication_state::ready;
		}
		P_char publisher = find_live_player(found->second.actor_pid);
		if (!publisher && retained_player_transfer_reason(found->second.requested_reason))
			publisher = actor;
		if (!publisher)
			break;
		publish(found, publisher);
		if (pending.find(key) != pending.end())
			break;
	}
	pump_creation_grants();
}

bool item_creation_grant_player_publication_pending(P_char player)
{
	if (!player || IS_NPC(player) || GET_PID(player) <= 0)
		return false;
	const uint32_t pid = static_cast<uint32_t>(GET_PID(player));
	for (const auto &[actor_pid, queue] : creation_grants)
	{
		(void)actor_pid;
		if (!queue.active || queue.requests.empty())
			continue;
		const auto &request = queue.requests.front();
		if (!request.to_room && request.recipient_pid == pid)
			return true;
	}
	return false;
}

// A creation commit can establish custody before its live publication. Ordinary
// saves must wait for actor-owned and inbound grants to publish their snapshots.
bool item_movement_transaction_player_creation_busy(P_char actor)
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
		for (const pending_creation_grant &request : queue.following_requests)
			if (!request.to_room && request.recipient_pid == pid)
				return true;
	}
	return false;
}

bool item_movement_transaction_player_busy(P_char actor)
{
	if (!actor || IS_NPC(actor) || GET_PID(actor) <= 0)
		return false;
	if (item_movement_transaction_player_creation_busy(actor))
		return true;
	const uint32_t pid = static_cast<uint32_t>(GET_PID(actor));
	if (std::any_of(drop_preparations.begin(), drop_preparations.end(), [pid](const auto &entry)
			{ return entry.active && entry.token.pid == static_cast<int32_t>(pid); }))
		return true;
	const item_owner_identity owner = { item_owner_type::player, pid, 0 };
	if (native_quest_owner_busy(owner))
		return true;
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
	drop_preparations = {};
	native_quest_reset_for_tests();
	pending.clear();
	creation_grants.clear();
	preparation_order.clear();
	health = {};
}

namespace
{
struct native_quest_publication_state;
}

class item_native_quest_publication_owner final
{
    public:
	static bool restore(const critical_native_recovery_envelope &) noexcept;
	static void completions(const critical_completion *, size_t) noexcept;

    private:
	friend class item_native_quest_gameplay_publication_owner;
	static bool native_publish(const critical_command &, const critical_completion &,
				   void *) noexcept;
	static bool fee_publish(const critical_command &, const critical_completion &,
				void *) noexcept;
	static bool money_publish(const critical_command &, const critical_completion &,
				  void *) noexcept;
	static bool recovery_initialize(native_quest_publication_state &) noexcept;
	static bool rebind(native_quest_publication_state &) noexcept;
#ifndef __NO_MYSQL__
	static bool rebind_money(native_quest_publication_state &) noexcept;
	static bool rebind_fee(native_quest_publication_state &) noexcept;
#endif
	static bool recovery_checkpoint(native_quest_publication_state &) noexcept;
	static bool recovery_prepare(native_quest_publication_state &, uint16_t) noexcept;
	static bool recovery_begin(native_quest_publication_state &, uint16_t) noexcept;
	static bool recovery_returned(native_quest_publication_state &) noexcept;

#ifndef __NO_MYSQL__
	static bool authenticate(MYSQL *, native_quest_publication_state &,
				 economic_sql_native_quest_publication *, bool require_hold = true);
#endif
};

namespace
{
struct native_quest_publication_state
{
	uint64_t generation = 0, native_runtime_id = 0, player_runtime_id = 0;
	uint32_t player_pid = 0;
	std::shared_ptr<const critical_command> command;
	critical_completion sealed{};
	item_transfer_payload payload{};
	item_transfer_result result{};
	item_native_mobile_money_result money_result{};
	item_native_mobile_fee_result fee_result{};
	std::vector<player_item_snapshot> native_before, native_after, player_before, player_after,
		selected;
	std::vector<uint8_t> root_stages;
	bool completion_ready = false, blocked = false, inflight = false, prepared = false;
	bool detach_started = false, detach_returned = false;
	bool place_started = false, place_returned = false;
	bool registry_started = false, registry_returned = false;
	bool binding_started = false, binding_returned = false, binding_refused = false;
	bool message_started = false, message_returned = false;
	std::array<uint8_t, 3> give_messages{};
	bool acknowledged = false, continuation_started = false, continuation_returned = false;
	std::string recovery_key;
	std::unique_ptr<critical_native_recovery_envelope> recovery, recovery_pending,
		recovery_return;
	size_t recovery_bytes = 0;
	uint16_t recovery_effect = 0;
	bool recovery_not_attempted = false, recovery_pending_ready = true;
	bool recovery_receipt_durable = false, recovery_physically_proven = false;
	bool restored = false, restoration_handed_off = false;

#ifndef __NO_MYSQL__
	std::optional<quest_reward_obligation_record> reward;
#endif
};
struct native_quest_acceptance_preparation
{
	uint64_t generation = 0, native_runtime_id = 0, root_uid = 0;
	player_native_quest_checkpoint_token player;
	quest_mobile_native_reference reference;
	std::vector<uint8_t> native_before;
	std::shared_ptr<const critical_command> command;
	std::shared_ptr<const quest_native_consumption_capture> consumption;
	bool money_only = false, money_preparation_blocked = false;
	quest_mobile_native_cash_reference money_native_before;
	quest_mobile_native_cash_reference cost_native_before;
	native_quest_coin_give_projection money_projection;
	uint64_t money_player_wallet_mapping_id = 0;
	int32_t money_original_room_vnum = 0;
	bool player_held = false, submission_started = false, gameplay_retained = false;
	std::unique_ptr<critical_native_recovery_envelope> restored_original;
	size_t publication_bytes = 0;
	std::shared_ptr<native_quest_publication_state> publication;
};
std::unordered_map<std::string, native_quest_acceptance_preparation> native_quest_acceptances;
uint64_t native_quest_preparation_generation = 0;
size_t native_quest_gameplay_retained_bytes = 0;
size_t native_quest_birth_retained_bytes = 0;
// Nonowning game-thread budget hook, installed only by the genuine flat ROOT.
// The registered observer outlives all guards and reads actual current globals.
// It conveys bytes only, never source/delivery/activation/ACK authority.
bool (*native_quest_flat_global_observer)(size_t *) noexcept = nullptr;
const void *native_quest_flat_global_scope = nullptr;
bool native_quest_flat_literal_pool_owned = false;

size_t native_quest_pending_count()
{
	return native_quest_acceptances.size();
}
bool native_quest_owner_busy(const item_owner_identity &owner)
{
	for (const auto &[key, entry] : native_quest_acceptances)
		if ((owner.type == item_owner_type::player && !owner.context_id &&
		     owner.id == static_cast<uint64_t>(entry.player.pid)) ||
		    (owner.type == item_owner_type::native_mobile && !owner.context_id &&
		     owner.id == entry.reference.mobile_instance_id))
			return true;
	return false;
}
void native_quest_reset_for_tests()
{
	native_quest_acceptances.clear();
	native_quest_preparation_generation = 0;
	native_quest_gameplay_retained_bytes = 0;
	native_quest_birth_retained_bytes = 0;
	native_quest_flat_global_observer = nullptr;
	native_quest_flat_global_scope = nullptr;
	native_quest_flat_literal_pool_owned = false;
}
bool native_quest_preparation_capacity(size_t incoming, bool include_driver = true,
				       bool include_birth = true)
{
	if (include_driver)
	{
		if (incoming > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
		    native_quest_gameplay_retained_bytes >
			    PLAYER_SAVE_PIPELINE_MAX_BYTES - incoming)
			return false;
		incoming += native_quest_gameplay_retained_bytes;
	}
	if (incoming > PLAYER_SAVE_PIPELINE_MAX_BYTES)
		return false;
	if (include_birth)
	{
		if (native_quest_birth_retained_bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES - incoming)
			return false;
		incoming += native_quest_birth_retained_bytes;
	}
	const size_t journal_storage = critical_command_journal_native_rewrite_storage_bytes();
	if (journal_storage > PLAYER_SAVE_PIPELINE_MAX_BYTES - incoming)
		return false;
	incoming += journal_storage;
	// During the one real bounded flat pulse, ROOT scratch already owns the
	// complete initial/CURRENT global allowance in every child outer. Outside
	// that scope count actual persistent globals, including after charge(0).
	// Never invoke new observers on original unregistered SQL/inactive paths.
	if (native_quest_flat_global_observer && !native_quest_flat_global_scope)
	{
		size_t globals = 0;
		if (!nevent_is_game_thread() || !native_quest_flat_global_observer(&globals) ||
		    globals > PLAYER_SAVE_PIPELINE_MAX_BYTES - incoming)
			return false;
		incoming += globals;
	}
	for (const auto &[key, entry] : native_quest_acceptances)
	{
		if (entry.native_before.size() > PLAYER_SAVE_PIPELINE_MAX_BYTES - incoming)
			return false;
		incoming += entry.native_before.size();
		if (entry.publication_bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES - incoming)
			return false;
		incoming += entry.publication_bytes;
		// Reserve the existing maximum immutable command footprint per original
		// preparation, before any command allocation or save queueing.
		if (2 * CRITICAL_COMMAND_MAX_ENCODED_BYTES >
		    PLAYER_SAVE_PIPELINE_MAX_BYTES - incoming)
			return false;
		incoming += 2 * CRITICAL_COMMAND_MAX_ENCODED_BYTES;
	}
	return true;
}

[[maybe_unused]] bool native_quest_mobile_before(P_char actor, P_char mobile, uint64_t runtime,
						 quest_mobile_native_reference *reference,
						 std::vector<uint8_t> *items)
{
	if (!actor || !mobile || !reference || !items || !IS_PC(actor) || !IS_NPC(mobile) ||
	    actor->in_room == NOWHERE || actor->in_room != mobile->in_room ||
	    !quest_mobile_native_reference_copy(mobile, runtime, reference))
		return false;
	std::vector<player_item_snapshot> captured;
	if (quest_mobile_native_items_observe(mobile, *reference, &captured) !=
	    player_snapshot_capture_result::ok)
		return false;
	return player_item_snapshot_list_encode(captured, items) ==
	       player_snapshot_codec_result::ok;
}
}

item_native_quest_preparation_state item_native_quest_preparation_owner::begin_acceptance(
	P_char actor, P_char mobile, P_obj root,
	item_native_quest_preparation_token *output) noexcept
{
	using state = item_native_quest_preparation_state;
#ifdef __NO_MYSQL__
	(void)actor;
	(void)mobile;
	(void)root;
	(void)output;
	return state::refused;
#else
	if (!output || !nevent_is_game_thread() ||
	    !economic_gameplay_authority::active_regular_sql() || !actor || !mobile || !root ||
	    !IS_PC(actor) || !actor->only.pc || !root->obj_uid || !OBJ_CARRIED_BY(root, actor) ||
	    native_quest_acceptances.size() + pending.size() >= ITEM_MOVEMENT_PENDING_MAX ||
	    native_quest_preparation_generation == UINT64_MAX || coordinator_item_fenced(root))
		return state::refused;
	try
	{
		for (const auto &[key, entry] : native_quest_acceptances)
			if (entry.player.pid == GET_PID(actor) ||
			    entry.native_runtime_id == mobile->runtime_id)
				return state::refused;
		native_quest_acceptance_preparation entry;
		entry.native_runtime_id = mobile->runtime_id;
		entry.root_uid = root->obj_uid;
		if (!native_quest_mobile_before(actor, mobile, entry.native_runtime_id,
						&entry.reference, &entry.native_before) ||
		    !native_quest_preparation_capacity(entry.native_before.size() +
						       2 * CRITICAL_COMMAND_MAX_ENCODED_BYTES))
			return state::refused;
		critical_operation_id operation{};
		if (!critical_operation_id_generate(&operation))
			return state::refused;
		entry.generation = ++native_quest_preparation_generation;
		const auto key = operation_key(operation);
		const auto inserted = native_quest_acceptances.emplace(key, std::move(entry));
		if (!inserted.second)
			return state::refused;
		auto &owned = inserted.first->second;
		// Original identities are retained before save enqueue; unresolved enqueue
		// keeps the exact token. No P_char/P_obj is retained across calls/pulses.
		const auto saved = player_save_pipeline_native_quest_checkpoint_begin(
			actor, root, world[actor->in_room].number, &owned.player);
		if (saved == player_literal_inventory_state::refused)
		{
			native_quest_acceptances.erase(inserted.first);
			return state::refused;
		}
		output->operation_ = operation;
		output->generation_ = owned.generation;
		return state::pending;
	}
	catch (...)
	{
		return state::refused;
	}
#endif
}

item_native_quest_preparation_state item_native_quest_preparation_owner::begin_money_acceptance(
	P_char actor, P_char mobile, uint8_t denomination, int32_t quantity,
	item_native_quest_preparation_token *output) noexcept
{
	using state = item_native_quest_preparation_state;
#ifdef __NO_MYSQL__
	(void)actor;
	(void)mobile;
	(void)denomination;
	(void)quantity;
	(void)output;
	return state::refused;
#else
	if (!output || !nevent_is_game_thread() ||
	    !economic_gameplay_authority::active_regular_sql() || !actor || !mobile ||
	    !IS_PC(actor) || !actor->only.pc || GET_PID(actor) <= 0 || !world ||
	    actor->in_room < 0 || actor->in_room > top_of_world ||
	    mobile->in_room != actor->in_room || world[actor->in_room].number < 0 ||
	    native_quest_acceptances.size() + pending.size() >= ITEM_MOVEMENT_PENDING_MAX ||
	    native_quest_preparation_generation == UINT64_MAX)
		return state::refused;
	try
	{
		for (const auto &[key, existing] : native_quest_acceptances)
			if (existing.player.pid == GET_PID(actor) ||
			    existing.native_runtime_id == mobile->runtime_id)
				return state::refused;
		native_quest_acceptance_preparation entry;
		entry.money_only = true;
		entry.money_original_room_vnum = world[actor->in_room].number;
		entry.native_runtime_id = mobile->runtime_id;
		economic_native_money_checkpoint_projection admission;
		const std::array<int64_t, 4> player_cash{ GET_COPPER(actor), GET_SILVER(actor),
							  GET_GOLD(actor), GET_PLATINUM(actor) };
		if (!native_quest_mobile_before(actor, mobile, entry.native_runtime_id,
						&entry.reference, &entry.native_before) ||
		    !quest_mobile_native_cash_reference_copy(mobile, entry.native_runtime_id,
							     &entry.money_native_before) ||
		    !economic_gameplay_authority::observe_native_money_checkpoint(
			    GET_PID(actor), entry.money_native_before, &admission) ||
		    native_quest_coin_give_project(
			    player_cash, actor->only.pc->wallet_revision,
			    entry.money_native_before.denominations,
			    entry.money_native_before.cash_revision, denomination, quantity,
			    &entry.money_projection) != native_quest_coin_give_result::ok ||
		    !native_quest_preparation_capacity(entry.native_before.size() +
						       2 * CRITICAL_COMMAND_MAX_ENCODED_BYTES))
			return state::refused;
		entry.money_player_wallet_mapping_id = admission.player_wallet.authority_id;
		critical_operation_id operation{};
		if (!critical_operation_id_generate(&operation))
			return state::refused;
		entry.generation = ++native_quest_preparation_generation;
		const auto inserted = native_quest_acceptances.emplace(operation_key(operation),
								       std::move(entry));
		if (!inserted.second)
			return state::refused;
		auto &owned = inserted.first->second;
		const auto saved = player_save_pipeline_native_money_checkpoint_begin(
			actor, world[actor->in_room].number, &owned.player);
		if (saved == player_literal_inventory_state::refused)
		{
			native_quest_acceptances.erase(inserted.first);
			return state::refused;
		}
		output->operation_ = operation;
		output->generation_ = owned.generation;
		return state::pending;
	}
	catch (...)
	{
		return state::refused;
	}
#endif
}

item_native_quest_preparation_state item_native_quest_preparation_owner::poll_money_acceptance(
	const item_native_quest_preparation_token &token, P_char actor, P_char mobile) noexcept
{
	using state = item_native_quest_preparation_state;
#ifdef __NO_MYSQL__
	(void)token;
	(void)actor;
	(void)mobile;
	return state::refused;
#else
	if (!nevent_is_game_thread())
		return state::refused;
	try
	{
		const auto found = native_quest_acceptances.find(operation_key(token.operation_));
		if (found == native_quest_acceptances.end() ||
		    found->second.generation != token.generation_ || !found->second.money_only ||
		    !found->second.player.money_only || found->second.root_uid)
			return state::refused;
		return poll_acceptance(token, actor, mobile);
	}
	catch (...)
	{
		return state::refused;
	}
#endif
}

item_native_quest_preparation_state item_native_quest_preparation_owner::poll_acceptance(
	const item_native_quest_preparation_token &token, P_char actor, P_char mobile) noexcept
{
	using state = item_native_quest_preparation_state;
#ifdef __NO_MYSQL__
	(void)token;
	(void)actor;
	(void)mobile;
	return state::refused;
#else
	if (!nevent_is_game_thread() || !economic_gameplay_authority::active_regular_sql())
		return state::refused;
	try
	{
		auto found = native_quest_acceptances.find(operation_key(token.operation_));
		if (found == native_quest_acceptances.end() ||
		    found->second.generation != token.generation_)
			return state::refused;
		auto &entry = found->second;
		if (entry.command && entry.submission_started)
		{
			std::vector<player_item_snapshot> before, after;
			player_native_quest_checkpoint_stage stage;
			return player_save_native_quest_checkpoint_owner::original_held_bodies(
				       *entry.command, &before, &after, &stage) ?
				       state::ready :
				       state::refused;
		}
		quest_mobile_native_reference reference;
		std::vector<uint8_t> current;
		if (!actor || !mobile || !IS_PC(actor) || !actor->only.pc ||
		    actor->runtime_id != entry.player.actor_runtime_id ||
		    GET_PID(actor) != entry.player.pid ||
		    mobile->runtime_id != entry.native_runtime_id ||
		    !native_quest_mobile_before(actor, mobile, entry.native_runtime_id, &reference,
						&current) ||
		    current != entry.native_before)
			return state::refused;
		std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> original_ref{},
			current_ref{};
		if (quest_mobile_native_reference_encode(entry.reference, &original_ref) !=
			    player_snapshot_codec_result::ok ||
		    quest_mobile_native_reference_encode(reference, &current_ref) !=
			    player_snapshot_codec_result::ok ||
		    original_ref != current_ref)
			return state::refused;
		if (entry.money_only)
		{
			quest_mobile_native_cash_reference cash;
			economic_native_money_checkpoint_projection admission;
			const std::array<int64_t, 4> player_cash{ GET_COPPER(actor),
								  GET_SILVER(actor),
								  GET_GOLD(actor),
								  GET_PLATINUM(actor) };
			if (!entry.player.money_only || entry.root_uid || entry.consumption ||
			    player_cash != entry.money_projection.player_before ||
			    actor->only.pc->wallet_revision !=
				    entry.money_projection.player_before_revision ||
			    !quest_mobile_native_cash_reference_copy(
				    mobile, entry.native_runtime_id, &cash) ||
			    cash.lineage.bytes != entry.money_native_before.lineage.bytes ||
			    cash.birth_epoch.bytes != entry.money_native_before.birth_epoch.bytes ||
			    cash.wallet_mapping_id != entry.money_native_before.wallet_mapping_id ||
			    cash.cash_revision != entry.money_native_before.cash_revision ||
			    cash.denominations != entry.money_native_before.denominations ||
			    !economic_gameplay_authority::observe_native_money_checkpoint(
				    entry.player.pid, cash, &admission) ||
			    admission.player_wallet.authority_id !=
				    entry.money_player_wallet_mapping_id)
				return state::refused;
		}
		if (entry.consumption)
		{
			item_transfer_payload captured_cost{};
			if (!item_transfer_command_decode_payload(
				    entry.consumption->original_command(), &captured_cost))
				return state::refused;
			if (captured_cost.native_cost.present)
			{
				quest_mobile_native_cash_reference cash;
				if (!quest_mobile_native_cash_reference_copy(
					    mobile, entry.native_runtime_id, &cash) ||
				    cash.lineage.bytes != entry.cost_native_before.lineage.bytes ||
				    cash.birth_epoch.bytes !=
					    entry.cost_native_before.birth_epoch.bytes ||
				    cash.wallet_mapping_id !=
					    entry.cost_native_before.wallet_mapping_id ||
				    cash.cash_revision != entry.cost_native_before.cash_revision ||
				    cash.denominations != entry.cost_native_before.denominations)
					return state::refused;
			}
		}
		player_native_quest_checkpoint_stage stage;
		if (entry.player_held)
			return player_save_native_quest_checkpoint_owner::observe_held(
				       entry.player, actor, token.operation_, &stage) ?
				       state::ready :
				       state::refused;
		const auto saved = player_save_pipeline_native_quest_checkpoint_poll(entry.player,
										     actor, &stage);
		if (saved == player_literal_inventory_state::pending)
			return state::pending;
		if (saved != player_literal_inventory_state::database_acknowledged ||
		    !player_save_pipeline_native_quest_checkpoint_hold(entry.player,
								       token.operation_))
			return state::refused;
		entry.player_held = true;
		return state::ready;
	}
	catch (...)
	{
		return state::refused;
	}
#endif
}

critical_submit_result item_native_quest_preparation_owner::submit_acceptance(
	const item_native_quest_preparation_token &token, P_char actor, P_char mobile) noexcept
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)actor;
	(void)mobile;
	return critical_submit_result::unavailable;
#else
	if (poll_acceptance(token, actor, mobile) != item_native_quest_preparation_state::ready)
		return critical_submit_result::unavailable;
	try
	{
		auto found = native_quest_acceptances.find(operation_key(token.operation_));
		if (found == native_quest_acceptances.end() ||
		    found->second.generation != token.generation_ || found->second.consumption ||
		    found->second.money_only)
			return critical_submit_result::identity_conflict;
		auto &entry = found->second;
		if (!entry.command)
		{
			P_obj root = find_item(entry.root_uid);
			player_native_quest_checkpoint_stage stage;
			std::vector<player_item_snapshot> original, selected;
			std::vector<uint8_t> original_bytes, selected_bytes;
			std::vector<item_transfer_entry> custody;
			item_ownership_runtime_entry owned{};
			uint64_t player_revision = 0, native_revision = 0;
			const item_owner_identity player{ item_owner_type::player,
							  static_cast<uint64_t>(entry.player.pid),
							  0 };
			const item_owner_identity native{ item_owner_type::native_mobile,
							  entry.reference.mobile_instance_id, 0 };
			if (!root || !OBJ_CARRIED_BY(root, actor) ||
			    coordinator_item_fenced(root) ||
			    !player_save_native_quest_checkpoint_owner::observe_held(
				    entry.player, actor, token.operation_, &stage, &original) ||
			    !item_ownership_runtime_lookup(root->obj_uid, &owned) ||
			    !item_owner_identity_equal(owned.owner, player) ||
			    owned.parent_item_uid ||
			    !item_ownership_runtime_peek_owner_revision(player, &player_revision) ||
			    !item_ownership_runtime_peek_owner_revision(native, &native_revision) ||
			    native_revision != entry.reference.stock_revision ||
			    !capture(root, root->obj_uid, 0, &custody) ||
			    player_item_snapshot_tree_capture_literal(root, &selected, nullptr) !=
				    player_snapshot_capture_result::ok ||
			    player_item_snapshot_list_encode(selected, &selected_bytes) !=
				    player_snapshot_codec_result::ok ||
			    player_item_snapshot_list_encode(original, &original_bytes) !=
				    player_snapshot_codec_result::ok ||
			    selected_bytes.size() > ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES ||
			    selected.size() != custody.size())
				return critical_submit_result::unavailable;
			for (const auto &row : custody)
			{
				item_ownership_runtime_entry actual{};
				if (!item_ownership_runtime_lookup(row.item_uid, &actual) ||
				    !item_owner_identity_equal(actual.owner, player))
					return critical_submit_result::invalid;
			}
			std::sort(custody.begin(), custody.end(), [](const auto &a, const auto &b)
				  { return a.item_uid < b.item_uid; });
			item_transfer_payload payload{};
			payload.from_owner = player;
			payload.to_owner = native;
			payload.reason = item_transfer_reason::quest_offering;
			payload.reason_id = entry.reference.mobile_vnum;
			payload.expected_from_revision = player_revision;
			payload.expected_to_revision = native_revision;
			payload.selected_item_uid = entry.root_uid;
			payload.target_root_item_uid = entry.root_uid;
			payload.item_count = static_cast<uint16_t>(custody.size());
			std::copy(custody.begin(), custody.end(), payload.items.begin());
			payload.item_blob_size = static_cast<uint32_t>(selected_bytes.size());
			std::copy(selected_bytes.begin(), selected_bytes.end(),
				  payload.item_blob.begin());
			payload.native_mobile.present = true;
			payload.native_mobile.action = item_native_mobile_action::acceptance;
			payload.native_mobile.reference = entry.reference;
			payload.native_mobile.final_giver_pid =
				static_cast<uint32_t>(entry.player.pid);
			if (!item_transfer_native_mobile_recovery_freeze(&payload, entry.player.pid,
									 stage.save_revision,
									 original_bytes))
				return critical_submit_result::invalid;
			critical_command command;
			if (!item_transfer_command_build_native_mobile_recovery(
				    &command, token.operation_, payload,
				    critical_source_site::command,
				    critical_deadline_class::interactive) ||
			    economic_gameplay_authority::prepare_native_item_transfer(
				    &command, static_cast<uint32_t>(entry.player.pid), nullptr) !=
				    economic_accounting_error::ok)
				return critical_submit_result::unavailable;
			command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
			const auto now =
				std::chrono::duration_cast<std::chrono::microseconds>(
					std::chrono::system_clock::now().time_since_epoch())
					.count();
			if (now <= 0)
				return critical_submit_result::invalid;
			command.accepted_at_usec = static_cast<uint64_t>(now);
			command.publication_required = true;
			if (!command.accepted_at_usec || !critical_command_envelope_valid(command))
				return critical_submit_result::invalid;
			entry.command =
				std::make_shared<const critical_command>(std::move(command));
		}
		bool released = false;
		const auto result = player_save_native_quest_checkpoint_owner::submit_owned(
			entry.player, *entry.command, entry.native_before, &released);
		if (critical_submit_result_keeps_operation(result))
			entry.submission_started = true;
		if (released)
			native_quest_acceptances.erase(found);
		// No publication callback or readiness bool may acknowledge this hold.
		// Original root/native publication/replay ownership is still required.
		return result;
	}
	catch (...)
	{
		return critical_submit_result::journal_uncertain;
	}
#endif
}

critical_submit_result item_native_quest_preparation_owner::submit_money_acceptance(
	const item_native_quest_preparation_token &token, P_char actor, P_char mobile) noexcept
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)actor;
	(void)mobile;
	return critical_submit_result::unavailable;
#else
	if (poll_money_acceptance(token, actor, mobile) !=
	    item_native_quest_preparation_state::ready)
		return critical_submit_result::unavailable;
	try
	{
		auto found = native_quest_acceptances.find(operation_key(token.operation_));
		if (found == native_quest_acceptances.end() ||
		    found->second.generation != token.generation_ || !found->second.money_only ||
		    !found->second.player.money_only || found->second.root_uid ||
		    found->second.consumption)
			return critical_submit_result::identity_conflict;
		auto &entry = found->second;
		if (!entry.command)
		{
			player_native_quest_checkpoint_stage stage;
			std::vector<player_item_snapshot> original;
			std::vector<uint8_t> original_bytes;
			uint64_t player_revision = 0, native_revision = 0;
			const item_owner_identity player{ item_owner_type::player,
							  static_cast<uint64_t>(entry.player.pid),
							  0 };
			const item_owner_identity native{ item_owner_type::native_mobile,
							  entry.reference.mobile_instance_id, 0 };
			if (!player_save_native_quest_checkpoint_owner::observe_held(
				    entry.player, actor, token.operation_, &stage, &original) ||
			    !item_ownership_runtime_peek_owner_revision(player, &player_revision) ||
			    !item_ownership_runtime_peek_owner_revision(native, &native_revision) ||
			    native_revision != entry.reference.stock_revision ||
			    player_item_snapshot_list_encode(original, &original_bytes) !=
				    player_snapshot_codec_result::ok)
				return critical_submit_result::unavailable;
			item_transfer_payload payload{};
			payload.from_owner = player;
			payload.to_owner = native;
			payload.reason = item_transfer_reason::player_give;
			payload.reason_id = entry.reference.mobile_vnum;
			payload.expected_from_revision = player_revision;
			payload.expected_to_revision = native_revision;
			payload.native_mobile.present = true;
			payload.native_mobile.action = item_native_mobile_action::acceptance;
			payload.native_mobile.reference = entry.reference;
			payload.native_mobile.final_giver_pid =
				static_cast<uint32_t>(entry.player.pid);
			payload.native_money.present = true;
			payload.native_money.original_room_vnum = entry.money_original_room_vnum;
			payload.native_money.player_wallet_mapping_id =
				entry.money_player_wallet_mapping_id;
			payload.native_money.mobile_wallet_mapping_id =
				entry.money_native_before.wallet_mapping_id;
			payload.native_money.projection = entry.money_projection;
			if (!item_transfer_native_mobile_recovery_freeze(&payload, entry.player.pid,
									 stage.save_revision,
									 original_bytes))
				return critical_submit_result::invalid;
			critical_command command;
			if (!item_transfer_command_build_native_mobile_recovery(
				    &command, token.operation_, payload,
				    critical_source_site::command,
				    critical_deadline_class::interactive) ||
			    economic_gameplay_authority::prepare_native_money_transfer(
				    &command, static_cast<uint32_t>(entry.player.pid),
				    &entry.money_native_before) != economic_accounting_error::ok)
				return critical_submit_result::unavailable;
			command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
			const auto now =
				std::chrono::duration_cast<std::chrono::microseconds>(
					std::chrono::system_clock::now().time_since_epoch())
					.count();
			if (now <= 0)
				return critical_submit_result::invalid;
			command.accepted_at_usec = static_cast<uint64_t>(now);
			command.publication_required = true;
			if (!critical_command_envelope_valid(command))
				return critical_submit_result::invalid;
			entry.command =
				std::make_shared<const critical_command>(std::move(command));
		}
		bool released = false;
		const auto result = player_save_native_quest_checkpoint_owner::submit_owned(
			entry.player, *entry.command, entry.native_before, &released);
		if (critical_submit_result_keeps_operation(result))
			entry.submission_started = true;
		if (released)
			native_quest_acceptances.erase(found);
		// Admission still requires the original shared SQL/root/publication owner.
		// Successful submission never grants world cash mutation or acknowledgment.
		return result;
	}
	catch (...)
	{
		return critical_submit_result::journal_uncertain;
	}
#endif
}

bool item_native_quest_preparation_owner::cancel(
	const item_native_quest_preparation_token &token) noexcept
{
	if (!nevent_is_game_thread())
		return false;
	try
	{
		auto found = native_quest_acceptances.find(operation_key(token.operation_));
		if (found == native_quest_acceptances.end())
			return true;
		auto &entry = found->second;
		if (entry.generation != token.generation_ || entry.submission_started)
			return false;
		const bool released =
			entry.player_held ?
				player_save_pipeline_native_quest_checkpoint_release(
					entry.player, token.operation_) :
				player_save_pipeline_native_quest_checkpoint_cancel(entry.player);
		if (!released)
			return false;
		native_quest_acceptances.erase(found);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

item_native_quest_preparation_state item_native_quest_preparation_owner::begin_consumption(
	P_char actor, P_char mobile,
	std::shared_ptr<const quest_native_consumption_capture> original,
	item_native_quest_preparation_token *output) noexcept
{
	using state = item_native_quest_preparation_state;
#ifdef __NO_MYSQL__
	(void)actor;
	(void)mobile;
	(void)original;
	(void)output;
	return state::refused;
#else
	if (!output || !original || !nevent_is_game_thread() ||
	    !economic_gameplay_authority::active_regular_sql() || !actor || !mobile ||
	    !IS_PC(actor) || !actor->only.pc || GET_PID(actor) <= 0 ||
	    static_cast<uint32_t>(GET_PID(actor)) != original->final_giver_pid() ||
	    mobile->runtime_id != original->runtime_generation() ||
	    pending.size() + native_quest_acceptances.size() >= ITEM_MOVEMENT_PENDING_MAX ||
	    native_quest_preparation_generation == UINT64_MAX)
		return state::refused;
	try
	{
		item_transfer_payload payload{};
		const auto &structural = original->original_command();
		if (structural.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
		    structural.accepted_at_usec || structural.publication_required ||
		    !structural.accounting_intent.empty() ||
		    (structural.payload_version != ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION &&
		     structural.payload_version !=
			     ITEM_TRANSFER_NATIVE_MOBILE_COST_PAYLOAD_VERSION) ||
		    !item_transfer_command_decode_payload(structural, &payload) ||
		    !item_transfer_native_mobile_shape_valid(payload) ||
		    payload.native_mobile.action != item_native_mobile_action::consumption)
			return state::refused;
		for (const auto &[key, entry] : native_quest_acceptances)
			if (entry.player.pid == GET_PID(actor) ||
			    entry.native_runtime_id == mobile->runtime_id)
				return state::refused;
		native_quest_acceptance_preparation entry;
		entry.native_runtime_id = original->runtime_generation();
		std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> observed{}, captured{};
		if (!native_quest_mobile_before(actor, mobile, entry.native_runtime_id,
						&entry.reference, &entry.native_before) ||
		    quest_mobile_native_reference_encode(entry.reference, &observed) !=
			    player_snapshot_codec_result::ok ||
		    quest_mobile_native_reference_encode(original->reference(), &captured) !=
			    player_snapshot_codec_result::ok ||
		    observed != captured ||
		    !native_quest_preparation_capacity(entry.native_before.size() +
						       2 * CRITICAL_COMMAND_MAX_ENCODED_BYTES))
			return state::refused;
		if (payload.native_cost.present &&
		    (!quest_mobile_native_cash_reference_copy(mobile, entry.native_runtime_id,
							      &entry.cost_native_before) ||
		     entry.cost_native_before.wallet_mapping_id !=
			     payload.native_cost.wallet_mapping_id ||
		     entry.cost_native_before.cash_revision !=
			     payload.native_cost.projection.before_revision ||
		     entry.cost_native_before.denominations !=
			     payload.native_cost.projection.before))
			return state::refused;
		entry.generation = ++native_quest_preparation_generation;
		entry.consumption = std::move(original);
		const auto operation = structural.operation_id;
		const auto inserted = native_quest_acceptances.emplace(operation_key(operation),
								       std::move(entry));
		if (!inserted.second)
			return state::refused;
		auto &owned = inserted.first->second;
		// Final giver gets a real acknowledged save fence. No selected player
		// tree or empty forest is invented: consumption changes NPC stock only.
		const auto saved = player_save_pipeline_native_quest_checkpoint_begin(
			actor, nullptr, world[actor->in_room].number, &owned.player);
		if (saved == player_literal_inventory_state::refused)
		{
			native_quest_acceptances.erase(inserted.first);
			return state::refused;
		}
		output->operation_ = operation;
		output->generation_ = owned.generation;
		return state::pending;
	}
	catch (...)
	{
		return state::refused;
	}
#endif
}

item_native_quest_preparation_state item_native_quest_preparation_owner::poll_consumption(
	const item_native_quest_preparation_token &token, P_char actor, P_char mobile) noexcept
{
	if (!nevent_is_game_thread())
		return item_native_quest_preparation_state::refused;
	try
	{
		auto found = native_quest_acceptances.find(operation_key(token.operation_));
		if (found == native_quest_acceptances.end() ||
		    found->second.generation != token.generation_ || !found->second.consumption)
			return item_native_quest_preparation_state::refused;
		return poll_acceptance(token, actor, mobile);
	}
	catch (...)
	{
		return item_native_quest_preparation_state::refused;
	}
}

bool item_native_quest_preparation_owner::prepare_consumption_command(
	const item_native_quest_preparation_token &token, P_char actor, P_char mobile,
	std::shared_ptr<const critical_command> *output, critical_submit_result *failure) noexcept
{
	if (!output || !failure)
		return false;
	*failure = critical_submit_result::unavailable;
#ifdef __NO_MYSQL__
	(void)token;
	(void)actor;
	(void)mobile;
	return false;
#else
	if (poll_consumption(token, actor, mobile) != item_native_quest_preparation_state::ready)
		return false;
	try
	{
		auto found = native_quest_acceptances.find(operation_key(token.operation_));
		if (found == native_quest_acceptances.end() ||
		    found->second.generation != token.generation_ || !found->second.consumption)
		{
			*failure = critical_submit_result::identity_conflict;
			return false;
		}
		auto &entry = found->second;
		if (!entry.command)
		{
			player_native_quest_checkpoint_stage stage;
			std::vector<player_item_snapshot> held_player;
			std::vector<uint8_t> held_player_bytes;
			item_transfer_payload payload{};
			if (!player_save_native_quest_checkpoint_owner::observe_held(
				    entry.player, actor, token.operation_, &stage, &held_player) ||
			    !item_transfer_command_decode_payload(
				    entry.consumption->original_command(), &payload) ||
			    (payload.native_cost.fee_only &&
			     player_item_snapshot_list_encode(held_player, &held_player_bytes) !=
				     player_snapshot_codec_result::ok) ||
			    !item_transfer_native_mobile_recovery_freeze(
				    &payload, static_cast<uint32_t>(entry.player.pid),
				    stage.save_revision, held_player_bytes,
				    entry.consumption->consumed_root_order(),
				    entry.consumption->publication_terms()))
			{
				*failure = critical_submit_result::invalid;
				return false;
			}
			critical_command command;
			const auto &original = entry.consumption->original_command();
			if (!item_transfer_command_build_native_mobile_recovery(
				    &command, token.operation_, payload, original.source_site,
				    original.deadline_class) ||
			    economic_gameplay_authority::prepare_native_item_transfer(
				    &command, static_cast<uint32_t>(entry.player.pid),
				    entry.consumption.get()) != economic_accounting_error::ok)
			{
				*failure = critical_submit_result::unavailable;
				return false;
			}
			const auto now =
				std::chrono::duration_cast<std::chrono::microseconds>(
					std::chrono::system_clock::now().time_since_epoch())
					.count();
			if (now <= 0)
			{
				*failure = critical_submit_result::invalid;
				return false;
			}
			command.accepted_at_usec = static_cast<uint64_t>(now);
			command.publication_required = true;
			if (!critical_command_envelope_valid(command))
			{
				*failure = critical_submit_result::invalid;
				return false;
			}
			entry.command =
				std::make_shared<const critical_command>(std::move(command));
		}
		*output = entry.command;
		return true;
	}
	catch (...)
	{
		*failure = critical_submit_result::journal_uncertain;
		return false;
	}
#endif
}

critical_submit_result item_native_quest_preparation_owner::submit_consumption(
	const item_native_quest_preparation_token &token, P_char actor, P_char mobile) noexcept
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)actor;
	(void)mobile;
	return critical_submit_result::unavailable;
#else
	std::shared_ptr<const critical_command> command;
	critical_submit_result failure = critical_submit_result::unavailable;
	if (!prepare_consumption_command(token, actor, mobile, &command, &failure))
		return failure;
	try
	{
		auto found = native_quest_acceptances.find(operation_key(token.operation_));
		if (found == native_quest_acceptances.end() ||
		    found->second.generation != token.generation_ || !found->second.consumption)
			return critical_submit_result::identity_conflict;
		auto &entry = found->second;
		bool released = false;
		const auto result = player_save_native_quest_checkpoint_owner::submit_owned(
			entry.player, *entry.command, entry.native_before, &released);
		if (critical_submit_result_keeps_operation(result))
			entry.submission_started = true;
		if (released)
			native_quest_acceptances.erase(found);
		return result;
	}
	catch (...)
	{
		return critical_submit_result::journal_uncertain;
	}
#endif
}

namespace
{
namespace native_quest_world
{
// Same maintained complete-census budget and physical graph rules as original
// SHOP observation. No keeper/shop identity or custody authority is borrowed.
constexpr size_t world_limit = 1000000;
bool add_tree_estimate(size_t &total, size_t estimate)
{
	if (estimate < sizeof(player_snapshot))
		return false;
	const size_t body = estimate - sizeof(player_snapshot);
	if (body > PLAYER_SNAPSHOT_MAX_BYTES - total)
		return false;
	total += body;
	return true;
}
enum class link_kind : uint8_t
{
	none,
	room,
	inside,
	carried,
	equipment
};
struct physical_link
{
	size_t count = 0;
	link_kind kind = link_kind::none;
	P_char body = nullptr;
	P_obj parent = nullptr;
	int room = -1;
	int slot = 0;
};
struct uid_occurrence
{
	P_obj object = nullptr;
	size_t count = 0;
};
struct census
{
	size_t visits = 0;
	std::unordered_map<P_obj, physical_link> objects;
	std::unordered_map<uint64_t, uid_occurrence> uids;
	std::unordered_set<P_char> seen_bodies;
	std::vector<P_char> bodies;
	bool charge() { return ++visits <= world_limit; }
	bool add_link(P_obj object, physical_link value)
	{
		auto found = objects.find(object);
		if (found == objects.end() || !charge())
			return false;
		value.count = found->second.count + 1;
		if (!found->second.count)
			found->second = value;
		else
			found->second.count = value.count;
		return true;
	}
	bool links(P_obj head, physical_link value)
	{
		std::unordered_set<P_obj> seen;
		for (P_obj object = head; object; object = object->next_content)
			if (!seen.insert(object).second || !add_link(object, value))
				return false;
		return true;
	}
	bool body(P_char ch)
	{
		if (!ch || !seen_bodies.insert(ch).second)
			return true;
		if (!charge() || (IS_NPC(ch) ? !ch->only.npc : !ch->only.pc))
			return false;
		bodies.push_back(ch);
		physical_link link;
		link.kind = link_kind::carried;
		link.body = ch;
		if (!links(ch->carrying, link))
			return false;
		link.kind = link_kind::equipment;
		for (int slot = 0; slot < MAX_WEAR; ++slot)
		{
			if (!ch->equipment[slot])
				continue; // Empty slots do not consume graph budget.
			link.slot = slot + 1;
			if (!add_link(ch->equipment[slot], link))
				return false;
		}
		return true;
	}
	bool scan()
	{
		P_obj previous = nullptr;
		for (P_obj object = object_list; object; object = object->next)
		{
			if (!charge() || object->prev != previous ||
			    !objects.emplace(object, physical_link{}).second)
				return false;
			if (object->obj_uid)
			{
				auto &occurrence = uids[object->obj_uid];
				occurrence.object = object;
				++occurrence.count;
			}
			previous = object;
		}
		for (const auto &[object, ignored] : objects)
		{
			physical_link link;
			link.kind = link_kind::inside;
			link.parent = object;
			if (!links(object->contains, link))
				return false;
		}
		if (!world || top_of_world < 0)
			return false;
		for (int room = 0; room <= top_of_world; ++room)
		{
			physical_link link;
			link.kind = link_kind::room;
			link.room = room;
			if (!charge() || !links(world[room].contents, link))
				return false;
			std::unordered_set<P_char> room_people;
			for (P_char ch = world[room].people; ch; ch = ch->next_in_room)
				if (!charge() || !room_people.insert(ch).second || !body(ch))
					return false;
		}
		std::unordered_set<P_char> chain;
		for (P_char ch = character_list; ch; ch = ch->next)
			if (!chain.insert(ch).second || !body(ch))
				return false;
		std::unordered_set<P_desc> descriptors;
		for (P_desc d = descriptor_list; d; d = d->next)
			if (!charge() || !descriptors.insert(d).second || !body(d->character) ||
			    !body(d->original))
				return false;
		for (size_t index = 0; index < bodies.size(); ++index)
		{
			P_char ch = bodies[index];
			if ((IS_NPC(ch) && !body(ch->only.npc->orig_char)) || !body(GET_PLYR(ch)))
				return false;
		}
		// Detect containment cycles even when each sibling chain alone is finite.
		std::unordered_map<P_obj, uint8_t> colors;
		for (const auto &[object, ignored] : objects)
		{
			std::vector<P_obj> path;
			P_obj current = object;
			while (current && !colors[current])
			{
				if (!charge())
					return false;
				colors[current] = 1;
				path.push_back(current);
				const auto &link = objects.at(current);
				current = link.kind == link_kind::inside ? link.parent : nullptr;
			}
			if (current && colors[current] == 1)
				return false;
			for (P_obj node : path)
				colors[node] = 2;
		}
		return true;
	}
	P_obj unique(uint64_t uid) const
	{
		auto found = uids.find(uid);
		return found != uids.end() && found->second.count == 1 ? found->second.object :
									 nullptr;
	}
	bool reciprocal(P_obj object, const physical_link &link) const
	{
		if (link.count != 1)
			return false;
		switch (link.kind)
		{
		case link_kind::room:
			return object->loc_p == LOC_ROOM && object->loc.room == link.room;
		case link_kind::inside:
			return object->loc_p == LOC_INSIDE && object->loc.inside == link.parent;
		case link_kind::carried:
			return object->loc_p == LOC_CARRIED && object->loc.carrying == link.body;
		case link_kind::equipment:
			return object->loc_p == LOC_WORN && object->loc.wearing == link.body &&
			       !object->next_content;
		default:
			return false;
		}
	}
	bool tree(P_obj object, P_obj parent, size_t depth, std::unordered_set<P_obj> &seen)
	{
		if (depth > PLAYER_SNAPSHOT_MAX_DEPTH ||
		    seen.size() >= PLAYER_SNAPSHOT_MAX_OBJECTS || !seen.insert(object).second)
			return false;
		auto found = objects.find(object);
		if (found == objects.end() ||
		    (object->obj_uid && unique(object->obj_uid) != object))
			return false;
		if (parent &&
		    (found->second.kind != link_kind::inside || found->second.parent != parent ||
		     !reciprocal(object, found->second)))
			return false;
		for (P_obj child = object->contains; child; child = child->next_content)
			if (!tree(child, object, depth + 1, seen))
				return false;
		return true;
	}
	bool forest(P_char ch, size_t *count = nullptr)
	{
		std::unordered_set<P_obj> seen;
		auto root = [&](P_obj object, link_kind kind, int slot)
		{
			auto found = objects.find(object);
			return found != objects.end() && found->second.kind == kind &&
			       found->second.body == ch && found->second.slot == slot &&
			       reciprocal(object, found->second) && tree(object, nullptr, 1, seen);
		};
		for (int slot = 0; slot < MAX_WEAR; ++slot)
			if (ch->equipment[slot] &&
			    !root(ch->equipment[slot], link_kind::equipment, slot + 1))
				return false;
		for (P_obj object = ch->carrying; object; object = object->next_content)
			if (!root(object, link_kind::carried, 0))
				return false;
		if (count)
			*count = seen.size();
		return true;
	}
};
bool append_tree(std::vector<player_item_snapshot> tree, int slot,
		 std::vector<player_item_snapshot> &out)
{
	if (tree.empty() || tree.size() > PLAYER_SNAPSHOT_MAX_OBJECTS - out.size())
		return false;
	const int32_t offset = out.size();
	tree[0].equipment_slot = slot;
	for (auto &row : tree)
	{
		if (row.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			row.parent_index += offset;
		out.push_back(std::move(row));
	}
	return true;
}
bool capture_body(P_char body, bool keeper, std::span<const uint64_t> literal_roots,
		  std::vector<player_item_snapshot> &out)
{
	if (keeper)
	{
		size_t captured_bytes = sizeof(player_snapshot);
		auto root = [&](P_obj object, int slot)
		{
			if (!object)
				return true;
			std::vector<player_item_snapshot> tree;
			size_t bytes = 0;
			if (player_item_snapshot_tree_capture_literal(object, &tree, &bytes) !=
				    player_snapshot_capture_result::ok ||
			    !add_tree_estimate(captured_bytes, bytes))
				return false;
			return append_tree(std::move(tree), slot, out);
		};
		for (int slot = 0; slot < MAX_WEAR; ++slot)
			if (!root(body->equipment[slot], slot + 1))
				return false;
		for (P_obj object = body->carrying; object; object = object->next_content)
			if (!root(object, 0))
				return false;
		return true;
	}
	std::vector<player_item_snapshot> ordinary;
	if (player_item_snapshot_list_capture(body, true, true, true, &ordinary, nullptr) !=
	    player_snapshot_capture_result::ok)
		return false;
	std::unordered_map<uint64_t, size_t> saved;
	for (size_t index = 0; index < ordinary.size(); ++index)
		if (!ordinary[index].object_uid ||
		    !saved.emplace(ordinary[index].object_uid, index).second)
			return false;
	struct node
	{
		P_obj object;
		P_obj parent;
		int slot;
	};
	std::vector<node> ordered;
	auto visit = [&](auto &&self, P_obj object, P_obj parent, int slot, size_t depth) -> bool
	{
		if (depth > PLAYER_SNAPSHOT_MAX_DEPTH ||
		    ordered.size() >= PLAYER_SNAPSHOT_MAX_OBJECTS)
			return false;
		ordered.push_back({ object, parent, slot });
		for (P_obj child = object->contains; child; child = child->next_content)
			if (!self(self, child, object, 0, depth + 1))
				return false;
		return true;
	};
	for (int slot = 0; slot < MAX_WEAR; ++slot)
		if (body->equipment[slot] &&
		    !visit(visit, body->equipment[slot], nullptr, slot + 1, 1))
			return false;
	for (P_obj object = body->carrying; object; object = object->next_content)
		if (!visit(visit, object, nullptr, 0, 1))
			return false;
	std::unordered_map<uint64_t, player_item_snapshot> literals;
	size_t literal_bytes = sizeof(player_snapshot);
	for (uint64_t uid : literal_roots)
	{
		auto found = std::find_if(ordered.begin(), ordered.end(), [uid](const auto &entry)
					  { return entry.object->obj_uid == uid; });
		if (found == ordered.end())
			return false;
		std::vector<player_item_snapshot> tree;
		size_t bytes = 0;
		if (player_item_snapshot_tree_capture_literal(found->object, &tree, &bytes) !=
			    player_snapshot_capture_result::ok ||
		    !add_tree_estimate(literal_bytes, bytes))
			return false;
		for (auto &row : tree)
			if (!literals.emplace(row.object_uid, std::move(row)).second)
				return false;
	}
	std::unordered_map<P_obj, int32_t> positions;
	for (const auto &entry : ordered)
	{
		const auto literal = literals.find(entry.object->obj_uid);
		const auto ordinary_row = saved.find(entry.object->obj_uid);
		if (literal == literals.end() && ordinary_row == saved.end())
			continue;
		player_item_snapshot row = literal != literals.end() ?
						   std::move(literal->second) :
						   ordinary[ordinary_row->second];
		row.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		if (entry.parent)
		{
			auto parent = positions.find(entry.parent);
			if (parent == positions.end())
				return false;
			row.parent_index = parent->second;
		}
		row.equipment_slot = entry.slot;
		positions.emplace(entry.object, out.size());
		out.push_back(std::move(row));
	}
	// Merge literal subtree rows, including a nested produced root, without
	// promoting the destination or its siblings or inventing a persisted parent.
	return true;
}

bool same_items(const std::vector<player_item_snapshot> &actual,
		std::span<const player_item_snapshot> expected)
{
	std::vector<player_item_snapshot> copy(expected.begin(), expected.end());
	std::vector<uint8_t> a, b;
	return player_item_snapshot_list_encode(actual, &a) == player_snapshot_codec_result::ok &&
	       player_item_snapshot_list_encode(copy, &b) == player_snapshot_codec_result::ok &&
	       a == b;
}
}

bool native_quest_reference_equal(const quest_mobile_native_reference &a,
				  const quest_mobile_native_reference &b)
{
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> left{}, right{};
	return quest_mobile_native_reference_encode(a, &left) == player_snapshot_codec_result::ok &&
	       quest_mobile_native_reference_encode(b, &right) ==
		       player_snapshot_codec_result::ok &&
	       left == right;
}

[[maybe_unused]] bool
native_quest_world_observe(native_quest_publication_state &state,
			   const std::vector<player_item_snapshot> &player_items,
			   const std::vector<player_item_snapshot> &native_items,
			   const std::vector<player_item_snapshot> &detached,
			   const quest_mobile_native_reference &reference,
			   const quest_mobile_native_cash &cash, native_quest_world::census *out,
			   P_char *actor_out, P_char *mobile_out, bool rediscover_absent = false)
{
	native_quest_world::census seen;
	if (!seen.scan())
		return false;
	const auto present = [&](uint64_t runtime)
	{
		return runtime &&
		       std::any_of(seen.bodies.begin(), seen.bodies.end(),
				   [&](P_char body) { return body->runtime_id == runtime; });
	};
	// Only the private fresh readback permits rediscovery, and only after
	// the complete census proves the old runtime absent. Present IDs stay pinned.
	const uint64_t player_runtime = rediscover_absent && !present(state.player_runtime_id) ?
						0 :
						state.player_runtime_id;
	const uint64_t native_runtime = rediscover_absent && !present(state.native_runtime_id) ?
						0 :
						state.native_runtime_id;
	P_char actor = nullptr, mobile = nullptr;
	for (P_char body : seen.bodies)
	{
		quest_mobile_native_reference candidate;
		const bool native_match =
			native_runtime ?
				body->runtime_id == native_runtime :
				IS_NPC(body) && body->only.npc && body->runtime_id &&
					quest_mobile_native_reference_copy(body, body->runtime_id,
									   &candidate) &&
					native_quest_reference_equal(candidate, reference);
		if (native_match)
		{
			if (mobile || !IS_NPC(body) || !body->only.npc)
				return false;
			mobile = body;
		}
		if (IS_PC(body) && body->only.pc &&
		    GET_PID(body) == static_cast<int>(state.player_pid))
		{
			if (actor || !body->runtime_id ||
			    (player_runtime && body->runtime_id != player_runtime))
				return false;
			actor = body;
		}
		if (player_runtime && body->runtime_id == player_runtime &&
		    (!IS_PC(body) || !body->only.pc ||
		     GET_PID(body) != static_cast<int>(state.player_pid)))
			return false;
	}
	// A missing body is retained absence, never an empty original forest.
	if (!actor || !mobile || !seen.forest(actor) || !seen.forest(mobile))
		return false;
	quest_mobile_native_reference current;
	std::vector<player_item_snapshot> actual_native, actual_player;
	std::vector<uint64_t> literal_roots;
	if (state.payload.native_mobile.action == item_native_mobile_action::acceptance &&
	    std::any_of(player_items.begin(), player_items.end(), [&](const auto &row)
			{ return row.object_uid == state.payload.selected_item_uid; }))
		literal_roots.push_back(state.payload.selected_item_uid);
	if (!quest_mobile_native_reference_copy(mobile, mobile->runtime_id, &current) ||
	    !native_quest_reference_equal(current, reference) ||
	    quest_mobile_native_items_observe(mobile, current, &actual_native) !=
		    player_snapshot_capture_result::ok ||
	    !native_quest_world::same_items(actual_native, native_items) ||
	    !native_quest_world::capture_body(actor, false, literal_roots, actual_player) ||
	    !native_quest_world::same_items(actual_player, player_items) ||
	    cash.denominations.amount !=
		    std::array<int64_t, 4>{ GET_COPPER(mobile), GET_SILVER(mobile),
					    GET_GOLD(mobile), GET_PLATINUM(mobile) })
		return false;
	std::unordered_set<uint64_t> expected;
	for (const auto *forest : { &player_items, &native_items, &detached })
		for (const auto &row : *forest)
			if (!expected.insert(row.object_uid).second || !seen.unique(row.object_uid))
				return false;
	std::vector<player_item_snapshot> observed_detached;
	for (const auto &row : detached)
	{
		if (row.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			continue;
		P_obj root = seen.unique(row.object_uid);
		std::unordered_set<P_obj> nodes;
		std::vector<player_item_snapshot> tree;
		size_t bytes = 0;
		if (!root || seen.objects.at(root).count || !OBJ_NOWHERE(root) ||
		    root->next_content || !seen.tree(root, nullptr, 1, nodes) ||
		    player_item_snapshot_tree_capture_literal(root, &tree, &bytes) !=
			    player_snapshot_capture_result::ok ||
		    !native_quest_world::append_tree(std::move(tree), 0, observed_detached))
			return false;
	}
	if (!native_quest_world::same_items(observed_detached, detached))
		return false;
	// Complete global absence/uniqueness for every original protected UID,
	// including descendants. An alias outside either body cannot prove AFTER.
	for (const auto *forest : { &state.player_before, &state.native_before, &state.selected })
		for (const auto &row : *forest)
			if (!expected.count(row.object_uid) && seen.uids.count(row.object_uid))
				return false;
	*actor_out = actor;
	*mobile_out = mobile;
	*out = std::move(seen);
	return true;
}

#ifndef __NO_MYSQL__
// Complete original global census plus literal money state. Current SQL values
// authenticate source/cut separately; this observer never publishes or ACKs.
[[maybe_unused]] bool native_money_world_observe(
	native_quest_publication_state &state, const economic_sql_native_money_publication &current,
	bool after, native_quest_world::census *output, P_char *actor_out, P_char *mobile_out,
	bool rediscover_absent = false, bool require_metadata = true)
{
	if (!state.payload.native_money.present || !current.original.native.cash || !output ||
	    !actor_out || !mobile_out || actor_out == mobile_out)
		return false;
	const auto &money = state.payload.native_money.projection;
	auto reference = state.payload.native_mobile.reference;
	if (after)
		++reference.mobile_revision;
	quest_mobile_native_cash cash;
	cash.revision = after ? money.mobile_after_revision : money.mobile_before_revision;
	cash.denominations.amount = after ? money.mobile_after : money.mobile_before;
	native_quest_world::census seen;
	P_char actor = nullptr, mobile = nullptr;
	if (!native_quest_world_observe(state, state.player_before, state.native_before, {},
					reference, cash, &seen, &actor, &mobile, rediscover_absent))
		return false;
	const std::array<int64_t, 4> player_cash{ GET_COPPER(actor), GET_SILVER(actor),
						  GET_GOLD(actor), GET_PLATINUM(actor) };
	quest_mobile_native_cash_reference observed;
	if (player_cash != (after ? money.player_after : money.player_before) ||
	    actor->only.pc->wallet_revision !=
		    (after ? money.player_after_revision : money.player_before_revision) ||
	    (require_metadata &&
	     (!quest_mobile_native_cash_reference_copy(mobile, mobile->runtime_id, &observed) ||
	      observed.wallet_mapping_id != state.payload.native_money.mobile_wallet_mapping_id ||
	      observed.wallet_mapping_id != current.origin.wallet_mapping_id ||
	      observed.lineage.bytes != current.origin.lineage.bytes ||
	      observed.birth_epoch.bytes != current.origin.birth_epoch.bytes ||
	      observed.cash_revision != cash.revision ||
	      observed.denominations != cash.denominations.amount ||
	      !native_quest_reference_equal(observed.reference, reference))))
		return false;
	const item_owner_identity player{ item_owner_type::player, state.player_pid, 0 };
	const item_owner_identity native{ item_owner_type::native_mobile,
					  reference.mobile_instance_id, 0 };
	uint64_t player_revision = 0, native_revision = 0;
	if (!item_ownership_runtime_peek_owner_revision(player, &player_revision) ||
	    !item_ownership_runtime_peek_owner_revision(native, &native_revision) ||
	    player_revision != current.original.player_owner_revision ||
	    player_revision != state.payload.expected_from_revision ||
	    native_revision != current.original.to_owner_revision ||
	    native_revision != state.payload.expected_to_revision)
		return false;
	for (const auto &row : current.original.custody)
	{
		item_ownership_runtime_entry actual;
		if (!item_ownership_runtime_lookup(row.item_uid, &actual) ||
		    !item_owner_identity_equal(actual.owner, row.owner) ||
		    actual.root_item_uid != row.root_item_uid ||
		    actual.parent_item_uid != row.parent_item_uid ||
		    actual.item_revision != row.item_revision ||
		    actual.owner_revision != row.owner_revision || actual.vnum != row.vnum ||
		    actual.state != row.state)
			return false;
	}
	*output = std::move(seen);
	*actor_out = actor;
	*mobile_out = mobile;
	return true;
}

[[maybe_unused]] bool native_fee_world_observe(native_quest_publication_state &state,
					       const economic_sql_native_fee_publication &current,
					       bool after, native_quest_world::census *output,
					       P_char *actor_out, P_char *mobile_out,
					       bool rediscover_absent = false,
					       bool require_metadata = true)
{
	if (!state.payload.native_cost.fee_only || !current.original.native.cash || !output ||
	    !actor_out || !mobile_out || actor_out == mobile_out)
		return false;
	const auto &fee = state.payload.native_cost.projection;
	auto reference = state.payload.native_mobile.reference;
	if (after)
		++reference.mobile_revision;
	quest_mobile_native_cash cash;
	cash.revision = after ? fee.after_revision : fee.before_revision;
	cash.denominations.amount = after ? fee.after : fee.before;
	native_quest_world::census seen;
	P_char actor = nullptr, mobile = nullptr;
	if (!native_quest_world_observe(state, state.player_before, state.native_before, {},
					reference, cash, &seen, &actor, &mobile, rediscover_absent))
		return false;
	quest_mobile_native_cash_reference observed;
	if ((require_metadata &&
	     (!quest_mobile_native_cash_reference_copy(mobile, mobile->runtime_id, &observed) ||
	      observed.wallet_mapping_id != state.payload.native_cost.wallet_mapping_id ||
	      observed.wallet_mapping_id != current.origin.wallet_mapping_id ||
	      observed.lineage.bytes != current.origin.lineage.bytes ||
	      observed.birth_epoch.bytes != current.origin.birth_epoch.bytes ||
	      observed.cash_revision != cash.revision ||
	      observed.denominations != cash.denominations.amount ||
	      !native_quest_reference_equal(observed.reference, reference))))
		return false;
	const item_owner_identity player{ item_owner_type::player, state.player_pid, 0 };
	const item_owner_identity native{ item_owner_type::native_mobile,
					  reference.mobile_instance_id, 0 };
	uint64_t player_revision = 0, native_revision = 0;
	if (!item_ownership_runtime_peek_owner_revision(player, &player_revision) ||
	    !item_ownership_runtime_peek_owner_revision(native, &native_revision) ||
	    player_revision != current.original.player_owner_revision ||
	    player_revision != state.payload.expected_to_revision ||
	    native_revision != current.original.from_owner_revision ||
	    native_revision != state.payload.expected_from_revision)
		return false;

	for (const auto &row : current.original.custody)
	{
		item_ownership_runtime_entry actual;
		if (!item_ownership_runtime_lookup(row.item_uid, &actual) ||
		    !item_owner_identity_equal(actual.owner, row.owner) ||
		    actual.root_item_uid != row.root_item_uid ||
		    actual.parent_item_uid != row.parent_item_uid ||
		    actual.item_revision != row.item_revision ||
		    actual.owner_revision != row.owner_revision || actual.vnum != row.vnum ||
		    actual.state != row.state)
			return false;
	}
	*output = std::move(seen);
	*actor_out = actor;
	*mobile_out = mobile;
	return true;
}
#endif

#ifndef __NO_MYSQL__
bool native_quest_runtime_observe(const native_quest_publication_state &state,
				  const economic_sql_native_quest_publication &current,
				  bool projected)
{
	uint64_t from = 0, to = 0, player_revision = 0;
	const auto &payload = state.payload;
	const bool success = state.sealed.outcome == critical_apply_outcome::applied ||
			     state.sealed.outcome == critical_apply_outcome::already_applied;
	const bool consumption = payload.native_mobile.action ==
				 item_native_mobile_action::consumption;
	const item_owner_identity player_owner{ item_owner_type::player, state.player_pid, 0 };
	if (!item_ownership_runtime_peek_owner_revision(payload.from_owner, &from) ||
	    !item_ownership_runtime_peek_owner_revision(payload.to_owner, &to) ||
	    !item_ownership_runtime_peek_owner_revision(player_owner, &player_revision) ||
	    from != (success && !projected ? payload.expected_from_revision :
					     current.from_owner_revision) ||
	    (consumption && !projected ?
		     (to > current.to_owner_revision ||
		      (success && to < payload.expected_to_revision)) :
		     to != (success && !projected ? payload.expected_to_revision :
						    current.to_owner_revision)) ||
	    (consumption && !projected ?
		     player_revision > current.player_owner_revision :
		     player_revision != (success && !projected ? payload.expected_from_revision :
								 current.player_owner_revision)))
		return false;
	std::unordered_set<uint64_t> required_active;
	for (const auto &expected_current : current.custody)
	{
		auto expected = expected_current;
		const auto selected = std::lower_bound(payload.items.begin(),
						       payload.items.begin() + payload.item_count,
						       expected.item_uid,
						       [](const auto &item, uint64_t uid)
						       { return item.item_uid < uid; });
		if (success && !projected &&
		    selected != payload.items.begin() + payload.item_count &&
		    selected->item_uid == expected.item_uid)
		{
			expected.root_item_uid = selected->root_item_uid;
			expected.parent_item_uid = selected->parent_item_uid;
			expected.owner = payload.from_owner;
			expected.item_revision = selected->expected_item_revision;
			expected.vnum = selected->vnum;
			expected.state = selected->expected_state;
		}
		item_ownership_runtime_entry observed{};
		if (!item_ownership_runtime_lookup(expected.item_uid, &observed) ||
		    observed.root_item_uid != expected.root_item_uid ||
		    observed.parent_item_uid != expected.parent_item_uid ||
		    !item_owner_identity_equal(observed.owner, expected.owner) ||
		    observed.item_revision != expected.item_revision ||
		    observed.vnum != expected.vnum || observed.state != expected.state ||
		    (projected && observed.owner_revision != expected_current.owner_revision))
			return false;
		if (expected.state == item_custody_state::active &&
		    !required_active.insert(expected.item_uid).second)
			return false;
	}
	const item_owner_identity native{ item_owner_type::native_mobile,
					  payload.native_mobile.reference.mobile_instance_id, 0 };
	const item_owner_identity player{ item_owner_type::player, state.player_pid, 0 };
	for (const auto &owner : { native, player })
	{
		std::vector<item_ownership_runtime_entry> all;
		if (!item_ownership_runtime_snapshot_owner(owner, 2 * PLAYER_SNAPSHOT_MAX_OBJECTS,
							   &all))
			return false;
		for (const auto &entry : all)
			if (entry.state == item_custody_state::active &&
			    !required_active.count(entry.item_uid))
				return false;
	}
	return true;
}

bool native_quest_frozen_reward_read(MYSQL *connection, native_quest_publication_state &state)
{
	if (state.payload.continuation.kind == item_transfer_continuation_kind::none)
		return true;
	if (state.payload.native_mobile.action != item_native_mobile_action::consumption)
		return false;
	std::vector<quest_reward_obligation_record> pending_rewards;
	unsigned int error = 0;
	if (quest_reward_obligation_repository_pending(connection, state.player_pid,
						       &pending_rewards, &error) !=
	    quest_reward_obligation_result::ok)
		return false;
	const auto found = std::find_if(
		pending_rewards.begin(), pending_rewards.end(), [&](const auto &record)
		{ return record.offering_operation.bytes == state.command->operation_id.bytes; });
	if (found == pending_rewards.end() ||
	    found->continuation != state.payload.continuation.data || found->terms.version != 5 ||
	    found->terms.player_pid != state.player_pid)
		return false;
	state.reward = *found;
	return true;
}
#endif

void native_quest_handle_completions(const critical_completion *incoming, size_t count)
{
	item_native_quest_publication_owner::completions(incoming, count);
}
}

#ifndef __NO_MYSQL__

namespace
{
constexpr uint16_t NATIVE_RECOVERY_PUBLISHING = 65530, NATIVE_RECOVERY_PROVEN = 65531;
constexpr uint16_t NATIVE_RECOVERY_ROOT = 9;
bool native_recovery_step(native_quest_recovery_context &context, uint16_t step, uint8_t value)
{
	if (step >= 1 && step <= 5)
		context.publication_steps[step - 1] = value;
	else if (step >= 6 && step <= 8)
		context.give_messages[step - 6] = value;
	else if (step >= NATIVE_RECOVERY_ROOT && static_cast<size_t>(step - NATIVE_RECOVERY_ROOT) <
							 context.consumed_root_steps.size())
		context.consumed_root_steps[step - NATIVE_RECOVERY_ROOT] = value;
	else
		return false;
	return true;
}
size_t native_recovery_bytes(const native_quest_publication_state &state)
{
	size_t bytes = 0;
	for (const auto *record :
	     { state.recovery.get(), state.recovery_pending.get(), state.recovery_return.get() })
		if (record)
			bytes += sizeof(*record) + CRITICAL_COMMAND_MAX_ENCODED_BYTES +
				 record->attachment.size();
	return bytes;
}
bool native_recovery_charge(native_quest_publication_state &state, size_t bytes)
{
	const auto found = native_quest_acceptances.find(state.recovery_key);
	if (found == native_quest_acceptances.end() ||
	    found->second.generation != state.generation ||
	    found->second.publication.get() != &state)
		return false;
	if (bytes > state.recovery_bytes)
	{
		const size_t extra = bytes - state.recovery_bytes;
		if (!native_quest_preparation_capacity(extra))
			return false;
		found->second.publication_bytes += extra;
	}
	else
		found->second.publication_bytes -= state.recovery_bytes - bytes;
	state.recovery_bytes = bytes;
	return true;
}
}

bool item_native_quest_publication_owner::recovery_initialize(
	native_quest_publication_state &state) noexcept
{
	if (state.recovery)
		return true;
	try
	{
		auto record = std::make_unique<critical_native_recovery_envelope>();
		native_quest_recovery_context context;
		if (!player_save_native_quest_publication_owner::copy_recovery_context(
			    *state.command, record.get()) ||
		    native_quest_recovery_context_decode(*state.command, record->attachment,
							 &context) !=
			    player_snapshot_codec_result::ok ||
		    !native_quest_world::same_items(context.native_before, state.native_before) ||
		    !native_quest_world::same_items(context.player_before, state.player_before))
			return false;
		if (context.receipt.present)
		{
			const auto success = [](critical_apply_outcome outcome)
			{
				return outcome == critical_apply_outcome::applied ||
				       outcome == critical_apply_outcome::already_applied;
			};
			if (!((success(context.receipt.outcome) && success(state.sealed.outcome)) ||
			      context.receipt.outcome == state.sealed.outcome) ||
			    context.receipt.durable_revision != state.sealed.durable_revision ||
			    context.receipt.error_code != state.sealed.error_code ||
			    context.receipt.failure_stage != state.sealed.failure_stage ||
			    context.receipt.result_size != state.sealed.result_size ||
			    context.receipt.result_payload != state.sealed.result_payload)
				return false;
		}
		const size_t bytes = sizeof(*record) + CRITICAL_COMMAND_MAX_ENCODED_BYTES +
				     record->attachment.size();
		if (!native_recovery_charge(state, bytes))
			return false;
		state.recovery = std::move(record);
		state.recovery_receipt_durable =
			context.receipt.present &&
			context.publication_stage !=
				native_quest_recovery_publication_stage::captured;
		state.recovery_physically_proven =
			context.publication_stage ==
			native_quest_recovery_publication_stage::physically_proven;
		state.detach_started = context.publication_steps[0] != 0;
		state.detach_returned = context.publication_steps[0] == 2;
		state.place_started = context.publication_steps[1] != 0;
		state.place_returned = context.publication_steps[1] == 2;
		state.registry_started = context.publication_steps[2] != 0;
		state.registry_returned = context.publication_steps[2] == 2;
		state.binding_started = context.publication_steps[3] != 0;
		state.binding_returned = context.publication_steps[3] == 2;
		state.message_started = context.publication_steps[4] != 0;
		state.message_returned = context.publication_steps[4] == 2;
		state.give_messages = context.give_messages;
		state.root_stages = std::move(context.consumed_root_steps);
		// Cold started/unreturned has no local known-not-attempted capability.
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool item_native_quest_publication_owner::recovery_checkpoint(
	native_quest_publication_state &state) noexcept
{
	if (!state.recovery_pending)
		return true;
	if (!state.recovery || !state.recovery_pending_ready)
		return false;
	if (!player_save_native_quest_publication_owner::checkpoint_recovery_context(
		    *state.recovery, *state.recovery_pending))
		return false; // Keep both exact immutable records for the identical retry.
	if (state.recovery_effect == NATIVE_RECOVERY_PUBLISHING)
		state.recovery_receipt_durable = true;
	if (state.recovery_effect == NATIVE_RECOVERY_PROVEN)
		state.recovery_physically_proven = true;
	state.recovery = std::move(state.recovery_pending);
	if (!state.recovery_return)
	{
		if (state.recovery_effect == 4 && state.binding_refused)
		{
			// advance(false) returned before its sole nonfallible binding copy.
			// Retry only after this exact returned record is durably settled.
			state.binding_started = false;
			state.binding_refused = false;
		}
		state.recovery_effect = 0;
		state.recovery_not_attempted = false;
	}
	// No encode, allocation, or command equality after journal I/O.
	if (!native_recovery_charge(state, native_recovery_bytes(state)))
	{
		state.blocked = true;
		return false;
	}
	return true;
}

bool item_native_quest_publication_owner::recovery_prepare(native_quest_publication_state &state,
							   uint16_t step) noexcept
{
	if (!state.recovery || state.recovery_pending || state.recovery_return ||
	    state.recovery->revision == UINT64_MAX)
		return false;
	try
	{
		native_quest_recovery_context context;
		if (native_quest_recovery_context_decode(*state.command, state.recovery->attachment,
							 &context) !=
		    player_snapshot_codec_result::ok)
			return false;
		if (step == NATIVE_RECOVERY_PUBLISHING)
		{
			context.receipt = { true,
					    state.sealed.outcome,
					    state.sealed.durable_revision,
					    state.sealed.error_code,
					    state.sealed.failure_stage,
					    state.sealed.result_size,
					    state.sealed.result_payload };
			context.publication_stage =
				native_quest_recovery_publication_stage::publishing;
		}
		else if (step == NATIVE_RECOVERY_PROVEN)
			context.publication_stage =
				native_quest_recovery_publication_stage::physically_proven;
		else if (!native_recovery_step(context, step, 1) ||
			 state.recovery->revision == UINT64_MAX - 1)
			return false;
		auto started = std::make_unique<critical_native_recovery_envelope>(*state.recovery);
		++started->revision;
		if (native_quest_recovery_context_encode(*state.command, context,
							 &started->attachment) !=
		    player_snapshot_codec_result::ok)
			return false;
		std::unique_ptr<critical_native_recovery_envelope> returned;
		if (step < NATIVE_RECOVERY_PUBLISHING)
		{
			if (!native_recovery_step(context, step, 2))
				return false;
			returned = std::make_unique<critical_native_recovery_envelope>(*started);
			++returned->revision;
			if (native_quest_recovery_context_encode(*state.command, context,
								 &returned->attachment) !=
			    player_snapshot_codec_result::ok)
				return false;
		}
		size_t bytes = state.recovery_bytes;
		for (const auto *record : { started.get(), returned.get() })
			if (record)
			{
				const size_t charge = sizeof(*record) +
						      CRITICAL_COMMAND_MAX_ENCODED_BYTES +
						      record->attachment.size();
				if (charge > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes)
					return false;
				bytes += charge;
			}
		if (!native_recovery_charge(state, bytes))
			return false;
		state.recovery_pending = std::move(started);
		state.recovery_return = std::move(returned);
		state.recovery_effect = step;
		state.recovery_not_attempted = step < NATIVE_RECOVERY_PUBLISHING;
		state.recovery_pending_ready = step != NATIVE_RECOVERY_PROVEN;
		return step == NATIVE_RECOVERY_PROVEN || recovery_checkpoint(state);
	}
	catch (...)
	{
		return false;
	}
}

bool item_native_quest_publication_owner::recovery_begin(native_quest_publication_state &state,
							 uint16_t step) noexcept
{
	if (state.recovery_effect == step && state.recovery_not_attempted)
		return recovery_checkpoint(state) && state.recovery_return != nullptr;
	return recovery_prepare(state, step);
}

bool item_native_quest_publication_owner::recovery_returned(
	native_quest_publication_state &state) noexcept
{
	if (!state.recovery_return || !state.recovery_effect)
		return false;
	state.recovery_not_attempted = false;
	state.recovery_pending = std::move(state.recovery_return);
	state.recovery_pending_ready = true;
	return recovery_checkpoint(state);
}

bool item_native_quest_publication_owner::authenticate(
	MYSQL *connection, native_quest_publication_state &state,
	economic_sql_native_quest_publication *current, bool require_hold)
{
	player_native_quest_checkpoint_stage held{};
	std::vector<player_item_snapshot> player_before, player_after;
	if (!require_hold)
	{
		player_before = state.player_before;
		player_after = state.player_after;
		held.save_revision = state.payload.native_recovery.acknowledged_save_revision;
	}
	if ((require_hold && !player_save_native_quest_publication_owner::publication_held_bodies(
				     *state.command, &player_before, &player_after, &held)) ||
	    held.save_revision != state.payload.native_recovery.acknowledged_save_revision ||
	    (state.prepared &&
	     (!native_quest_world::same_items(player_before, state.player_before) ||
	      !native_quest_world::same_items(player_after, state.player_after))) ||
	    economic_sql_native_quest_lock_publication(connection, *state.command, state.sealed,
						       state.native_before, player_before,
						       player_after, held.save_revision, current))
		return false;
	const auto authentic = critical_command_repository_verify_native_quest_in_transaction(
		connection, *state.command);
	const bool authentic_success = authentic.outcome == critical_apply_outcome::applied ||
				       authentic.outcome == critical_apply_outcome::already_applied;
	const bool success = state.sealed.outcome == critical_apply_outcome::applied ||
			     state.sealed.outcome == critical_apply_outcome::already_applied;
	if ((success ? !authentic_success : authentic.outcome != state.sealed.outcome) ||
	    authentic.error_code != state.sealed.error_code ||
	    authentic.failure_stage != state.sealed.failure_stage ||
	    authentic.durable_revision != state.sealed.durable_revision ||
	    authentic.result_size != state.sealed.result_size ||
	    authentic.result_payload != state.sealed.result_payload || !connection ||
	    mysql_thread_id(connection) != current->session_id ||
	    !(connection->server_status & SERVER_STATUS_IN_TRANS) || !current->native.cash ||
	    current->native.state != quest_mobile_lifetime_state::live ||
	    current->acknowledged_save_revision != held.save_revision)
		return false;
	auto reference = state.payload.native_mobile.reference;
	if (success)
	{
		if (reference.mobile_revision == UINT64_MAX ||
		    reference.stock_revision == UINT64_MAX)
			return false;
		++reference.mobile_revision;
		++reference.stock_revision;
		if (current->native.last_transition_operation.bytes !=
		    state.command->operation_id.bytes)
			return false;
	}
	if (!native_quest_reference_equal(current->native.reference, reference) ||
	    !native_quest_world::same_items(current->native.items,
					    success ? state.native_after : state.native_before))
		return false;
	state.player_before = std::move(player_before);
	state.player_after = std::move(player_after);
	return true;
}

#endif

#ifndef __NO_MYSQL__
namespace
{
bool native_quest_envelope_equal(const critical_native_recovery_envelope &a,
				 const critical_native_recovery_envelope &b)
{
	return a.revision == b.revision && a.phase == b.phase && a.attachment == b.attachment &&
	       critical_command_equal(a.command, b.command);
}
bool native_quest_restore_values(native_quest_publication_state &state,
				 const critical_native_recovery_envelope &envelope,
				 native_quest_recovery_context &context)
{
	if (!envelope.revision ||
	    !item_transfer_command_decode_payload(envelope.command, &state.payload) ||
	    !item_transfer_native_mobile_recovery_shape_valid(state.payload) ||
	    native_quest_recovery_context_decode(envelope.command, envelope.attachment, &context) !=
		    player_snapshot_codec_result::ok ||
	    (!state.payload.native_money.present && !state.payload.native_cost.fee_only &&
	     player_item_snapshot_list_decode(state.payload.item_blob.data(),
					      state.payload.item_blob_size,
					      &state.selected) != player_snapshot_codec_result::ok))
		return false;
	state.native_before = context.native_before;
	state.player_before = context.player_before;
	if (state.payload.native_money.present || state.payload.native_cost.fee_only)
	{
		state.native_after = state.native_before;
		state.player_after = state.player_before;
	}
	else if (quest_mobile_native_items_transition(
			 state.native_before, state.payload.native_mobile.reference, state.payload,
			 &state.native_after) != player_snapshot_codec_result::ok)
		return false;
	if (!state.payload.native_money.present && !state.payload.native_cost.fee_only &&
	    state.payload.native_mobile.action == item_native_mobile_action::acceptance)
	{
		std::vector<player_item_snapshot> selected;
		if (player_item_snapshot_extract_subtree(
			    state.player_before, item_transfer_result_root(state.payload),
			    &selected, &state.player_after) != player_snapshot_codec_result::ok ||
		    !native_quest_world::same_items(selected, state.selected))
			return false;
	}
	else
		state.player_after = state.player_before;
	state.player_pid = state.payload.native_recovery.player_pid;
	state.root_stages = context.consumed_root_steps;
	state.give_messages = context.give_messages;
	state.detach_started = context.publication_steps[0] != 0;
	state.detach_returned = context.publication_steps[0] == 2;
	state.place_started = context.publication_steps[1] != 0;
	state.place_returned = context.publication_steps[1] == 2;
	state.registry_started = context.publication_steps[2] != 0;
	state.registry_returned = context.publication_steps[2] == 2;
	state.binding_started = context.publication_steps[3] != 0;
	state.binding_returned = context.publication_steps[3] == 2;
	state.message_started = context.publication_steps[4] != 0;
	state.message_returned = context.publication_steps[4] == 2;
	state.recovery_receipt_durable = context.receipt.present &&
					 context.publication_stage !=
						 native_quest_recovery_publication_stage::captured;
	state.recovery_physically_proven =
		context.publication_stage ==
		native_quest_recovery_publication_stage::physically_proven;
	return true;
}
size_t native_quest_restore_bytes(const native_quest_publication_state &state)
{
	size_t bytes =
		sizeof(state) + state.recovery_key.capacity() + 1 + state.root_stages.capacity() +
		state.payload.continuation.data.capacity() +
		state.payload.native_recovery.consumed_root_order.capacity() * sizeof(uint64_t) +
		state.payload.native_recovery.publication_terms.message.capacity() + 1 +
		state.payload.native_recovery.publication_terms.disappear_message.capacity() + 1 +
		ITEM_TRANSFER_CONTINUATION_MAX_BYTES + QUEST_REWARD_MAX_CHARACTER_NAME_BYTES +
		QUEST_REWARD_MAX_DEFINITION_ID_BYTES;
	const auto charge = [&](size_t value)
	{
		if (bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
		    value > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes)
			return false;
		bytes += value;
		return true;
	};
	for (const auto *forest : { &state.native_before, &state.native_after, &state.player_before,
				    &state.player_after, &state.selected })
	{
		if (!charge(forest->capacity() * sizeof(player_item_snapshot)))
			return PLAYER_SAVE_PIPELINE_MAX_BYTES + 1;
		for (const auto &row : *forest)
		{
			if (!charge(row.name.capacity() + 1) ||
			    !charge(row.short_description.capacity() + 1) ||
			    !charge(row.description.capacity() + 1) ||
			    !charge(row.action_description.capacity() + 1) ||
			    !charge(row.dynamic_affects.capacity() *
				    sizeof(player_item_dynamic_affect_snapshot)) ||
			    !charge(row.extra_descriptions.capacity() *
				    sizeof(player_item_extra_description_snapshot)))
				return PLAYER_SAVE_PIPELINE_MAX_BYTES + 1;
			for (const auto &description : row.extra_descriptions)
				if (!charge(description.keyword.capacity() + 1) ||
				    !charge(description.description.capacity() + 1) ||
				    !charge(description.spell_ids.capacity() * sizeof(int32_t)))
					return PLAYER_SAVE_PIPELINE_MAX_BYTES + 1;
		}
	}
	if (state.recovery && !charge(sizeof(*state.recovery) + CRITICAL_COMMAND_MAX_ENCODED_BYTES +
				      state.recovery->attachment.capacity()))
		return PLAYER_SAVE_PIPELINE_MAX_BYTES + 1;
	return bytes;
}
}
#endif

bool item_native_quest_publication_owner::restore(
	const critical_native_recovery_envelope &envelope) noexcept
{
#ifdef __NO_MYSQL__
	(void)envelope;
	return false;
#else
	try
	{
		if (envelope.phase == critical_native_recovery_phase::continuation_pending)
			return quest_native_frozen_continuation_owner::restore(envelope);
		if (envelope.phase != critical_native_recovery_phase::execution_pending ||
		    !envelope.revision)
			return false;
		const std::string key = operation_key(envelope.command.operation_id);
		auto found = native_quest_acceptances.find(key);
		if (found != native_quest_acceptances.end())
			return found->second.restored_original &&
			       native_quest_envelope_equal(*found->second.restored_original,
							   envelope);
		if (native_quest_preparation_generation == UINT64_MAX ||
		    native_quest_pending_count() + pending.size() >= ITEM_MOVEMENT_PENDING_MAX)
			return false;
		auto state = std::make_shared<native_quest_publication_state>();
		state->command = std::make_shared<const critical_command>(envelope.command);
		state->recovery_key = key;
		native_quest_recovery_context context;
		if (!native_quest_restore_values(*state, envelope, context))
			return false;
		state->restored = true;
		state->prepared = true;
		state->recovery = std::make_unique<critical_native_recovery_envelope>(envelope);
		state->recovery_bytes = sizeof(envelope) + CRITICAL_COMMAND_MAX_ENCODED_BYTES +
					state->recovery->attachment.capacity();
		native_quest_acceptance_preparation entry;
		entry.generation = native_quest_preparation_generation + 1;
		state->generation = entry.generation;
		entry.player.pid = static_cast<int32_t>(state->player_pid);
		entry.money_only = state->payload.native_money.present;
		entry.player.money_only = entry.money_only;
		if (entry.money_only)
		{
			entry.money_projection = state->payload.native_money.projection;
			entry.money_original_room_vnum =
				state->payload.native_money.original_room_vnum;
			entry.money_player_wallet_mapping_id =
				state->payload.native_money.player_wallet_mapping_id;
		}
		entry.root_uid = item_transfer_result_root(state->payload);
		entry.reference = state->payload.native_mobile.reference;
		entry.command = state->command;
		entry.submission_started = true;
		entry.restored_original =
			std::make_unique<critical_native_recovery_envelope>(envelope);
		if (player_item_snapshot_list_encode(state->native_before, &entry.native_before) !=
		    player_snapshot_codec_result::ok)
			return false;
		entry.publication_bytes = native_quest_restore_bytes(*state);
		const size_t extra_record = sizeof(envelope) + CRITICAL_COMMAND_MAX_ENCODED_BYTES +
					    entry.restored_original->attachment.capacity();
		if (entry.publication_bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
		    extra_record > PLAYER_SAVE_PIPELINE_MAX_BYTES - entry.publication_bytes)
			return false;
		entry.publication_bytes += extra_record;
		if (entry.native_before.capacity() >
			    PLAYER_SAVE_PIPELINE_MAX_BYTES - entry.publication_bytes ||
		    2 * CRITICAL_COMMAND_MAX_ENCODED_BYTES >
			    PLAYER_SAVE_PIPELINE_MAX_BYTES - entry.publication_bytes -
				    entry.native_before.capacity() ||
		    !native_quest_preparation_capacity(entry.publication_bytes +
						       entry.native_before.capacity() +
						       2 * CRITICAL_COMMAND_MAX_ENCODED_BYTES))
			return false;
		entry.publication = state;
		native_quest_acceptances.reserve(native_quest_acceptances.size() + 1);
		auto installed = native_quest_acceptances.emplace(key, std::move(entry));
		if (!installed.second)
			return false;
		// Every allocation/domain reservation precedes the original PID hold.
		// A refused hold removes only this unadmitted in-memory candidate.
		if (!player_save_native_quest_publication_owner::restore_recovery_checkpoint(
			    envelope))
		{
			native_quest_acceptances.erase(installed.first);
			return false;
		}
		installed.first->second.player_held = true;
		native_quest_preparation_generation = state->generation;
		return true; // No completion, runtime binding, or native effect is claimed.
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool item_movement_transaction_restore_native_recovery(
	const critical_native_recovery_envelope &envelope) noexcept
{
	return item_native_quest_publication_owner::restore(envelope);
}

#ifndef __NO_MYSQL__
namespace
{
bool native_quest_restored_cut(native_quest_publication_state &state,
			       const economic_sql_native_quest_publication &current, bool success,
			       P_char *actor, P_char *mobile)
{
	auto reference = state.payload.native_mobile.reference;
	auto native_items = state.native_before;
	auto player_items = state.player_before;
	std::vector<player_item_snapshot> detached;
	if ((state.detach_started && !state.detach_returned) ||
	    (state.place_started && !state.place_returned) ||
	    (state.registry_started && !state.registry_returned) ||
	    (state.binding_started && !state.binding_returned) ||
	    (state.message_started && !state.message_returned))
		return false;
	if (success &&
	    state.payload.native_mobile.action == item_native_mobile_action::acceptance &&
	    state.detach_returned)
	{
		player_items = state.player_after;
		if (state.place_returned)
			native_items = state.native_after;
		else
			detached = state.selected;
	}
	if (success && state.payload.native_mobile.action == item_native_mobile_action::consumption)
		for (size_t i = 0; i < state.root_stages.size(); ++i)
		{
			if (state.root_stages[i] == 1)
				return false;
			if (state.root_stages[i] != 2)
				continue;
			std::vector<player_item_snapshot> removed, remaining;
			if (player_item_snapshot_extract_subtree(
				    native_items,
				    state.payload.native_recovery.consumed_root_order[i], &removed,
				    &remaining) != player_snapshot_codec_result::ok)
				return false;
			native_items = std::move(remaining);
		}
	if (state.binding_returned)
		reference = current.native.reference;
	native_quest_world::census seen;
	return native_quest_world_observe(state, player_items, native_items, detached, reference,
					  *current.native.cash, &seen, actor, mobile, true);
}
}
#endif

#ifndef __NO_MYSQL__
bool item_native_quest_publication_owner::rebind_money(
	native_quest_publication_state &state) noexcept
{
#ifdef __NO_MYSQL__
	(void)state;
	return false;
#else
	if (!nevent_is_game_thread() || !state.restored || !state.completion_ready ||
	    !state.recovery || !state.payload.native_money.present ||
	    (state.detach_started && !state.detach_returned) ||
	    std::any_of(state.give_messages.begin(), state.give_messages.end(),
			[](uint8_t v) { return v == 1; }))
		return false;
	try
	{
		MYSQL *connection = sql_pool_acquire();
		player_sql_pool_lease lease(connection);
		if (!connection || player_sql_idle_error(connection))
			return false;
		player_sql_cleanup cleanup;
		player_sql_transaction_cleanup transaction(connection, cleanup);
		transaction.starting();
		bool proven = false;
		economic_sql_native_money_publication current;
		native_quest_world::census seen;
		P_char actor = nullptr, mobile = nullptr;
		const bool success = state.sealed.outcome == critical_apply_outcome::applied ||
				     state.sealed.outcome ==
					     critical_apply_outcome::already_applied;
		const bool after = success && state.detach_returned;
		try
		{
			if (!mysql_real_query(connection, "START TRANSACTION", 17))
				proven = economic_sql_native_money_lock_publication(
						 connection, *state.command, state.sealed,
						 state.native_before, state.player_before,
						 state.player_after,
						 state.payload.native_recovery
							 .acknowledged_save_revision,
						 &current) == 0 &&
					 native_money_world_observe(state, current, after, &seen,
								    &actor, &mobile, true, false) &&
					 transaction.same_session();
		}
		catch (...)
		{
			proven = false;
		}
		transaction.finish();
		lease.reuse(cleanup);
		if (!proven || !cleanup.rollback_confirmed || cleanup.cleanup_error ||
		    cleanup.disposition != player_sql_cleanup_disposition::idle_verified ||
		    !transaction.same_session() || !actor || !mobile)
			return false;
		// Installer only accepts the actual current SQL/native literal image after
		// confirmed original read-only rollback. It cannot manufacture BEFORE from
		// command facts when SQL is AFTER, or repair a missing/mixed lifetime.
		quest_mobile_native_cash_reference original;
		const bool known = quest_mobile_native_cash_reference_copy(
			mobile, mobile->runtime_id, &original);
		if (!known &&
		    !quest_mobile_native_publication_binding::restore_money_metadata(
			    mobile, mobile->runtime_id, current.original.native, current.origin))
			return false;
		if (!native_money_world_observe(state, current, after, &seen, &actor, &mobile,
						true))
			return false;
		auto found = native_quest_acceptances.find(state.recovery_key);
		if (found == native_quest_acceptances.end() ||
		    found->second.publication.get() != &state ||
		    found->second.generation != state.generation ||
		    !found->second.restored_original ||
		    (!state.acknowledged &&
		     !player_save_native_quest_publication_owner::rebind_recovery_checkpoint(
			     *found->second.restored_original, actor->runtime_id)))
			return false;
		state.player_runtime_id = actor->runtime_id;
		state.native_runtime_id = mobile->runtime_id;
		found->second.player.actor_runtime_id = actor->runtime_id;
		found->second.native_runtime_id = mobile->runtime_id;
		if (!quest_mobile_native_cash_reference_copy(mobile, mobile->runtime_id,
							     &found->second.money_native_before))
			return false;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool item_native_quest_publication_owner::rebind_fee(native_quest_publication_state &state) noexcept
{
#ifdef __NO_MYSQL__
	(void)state;
	return false;
#else
	if (!nevent_is_game_thread() || !state.restored || !state.completion_ready ||
	    !state.recovery || !state.payload.native_cost.fee_only ||
	    (state.detach_started && !state.detach_returned) ||
	    (state.message_started && !state.message_returned))
		return false;
	try
	{
		MYSQL *connection = sql_pool_acquire();
		player_sql_pool_lease lease(connection);
		if (!connection || player_sql_idle_error(connection))
			return false;
		player_sql_cleanup cleanup;
		player_sql_transaction_cleanup transaction(connection, cleanup);
		transaction.starting();
		bool proven = false;
		economic_sql_native_fee_publication current;
		native_quest_world::census seen;
		P_char actor = nullptr, mobile = nullptr;
		const bool success = state.sealed.outcome == critical_apply_outcome::applied ||
				     state.sealed.outcome ==
					     critical_apply_outcome::already_applied;
		const bool after = success && state.detach_returned;
		try
		{
			if (!mysql_real_query(connection, "START TRANSACTION", 17))
				proven = economic_sql_native_fee_lock_publication(
						 connection, *state.command, state.sealed,
						 state.native_before, state.player_before,
						 state.player_after,
						 state.payload.native_recovery
							 .acknowledged_save_revision,
						 &current) == 0 &&
					 native_fee_world_observe(state, current, after, &seen,
								  &actor, &mobile, true, false) &&
					 transaction.same_session();
		}
		catch (...)
		{
			proven = false;
		}
		transaction.finish();
		lease.reuse(cleanup);
		if (!proven || !cleanup.rollback_confirmed || cleanup.cleanup_error ||
		    cleanup.disposition != player_sql_cleanup_disposition::idle_verified ||
		    !transaction.same_session() || !actor || !mobile)
			return false;
		// Installer only accepts the actual current SQL/native literal image after
		// confirmed original read-only rollback. It cannot manufacture BEFORE from
		// command facts when SQL is AFTER, or repair a missing/mixed lifetime.
		quest_mobile_native_cash_reference original;
		const bool known = quest_mobile_native_cash_reference_copy(
			mobile, mobile->runtime_id, &original);
		if (!known &&
		    !quest_mobile_native_publication_binding::restore_money_metadata(
			    mobile, mobile->runtime_id, current.original.native, current.origin))
			return false;
		if (!native_fee_world_observe(state, current, after, &seen, &actor, &mobile, true))
			return false;
		auto found = native_quest_acceptances.find(state.recovery_key);
		if (found == native_quest_acceptances.end() ||
		    found->second.publication.get() != &state ||
		    found->second.generation != state.generation ||
		    !found->second.restored_original ||
		    (!state.acknowledged &&
		     !player_save_native_quest_publication_owner::rebind_recovery_checkpoint(
			     *found->second.restored_original, actor->runtime_id)))
			return false;
		state.player_runtime_id = actor->runtime_id;
		state.native_runtime_id = mobile->runtime_id;
		found->second.player.actor_runtime_id = actor->runtime_id;
		found->second.native_runtime_id = mobile->runtime_id;
		if (!quest_mobile_native_cash_reference_copy(mobile, mobile->runtime_id,
							     &found->second.cost_native_before))
			return false;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

#endif

bool item_native_quest_publication_owner::rebind(native_quest_publication_state &state) noexcept
{
#ifdef __NO_MYSQL__
	(void)state;
	return false;
#else
	if (!state.restored || !state.completion_ready || !state.recovery ||
	    !nevent_is_game_thread())
		return false;
	if (state.payload.native_cost.fee_only)
		return rebind_fee(state);
	if (state.payload.native_money.present)
		return rebind_money(state);
	try
	{
		MYSQL *connection = sql_pool_acquire();
		player_sql_pool_lease lease(connection);
		if (!connection || player_sql_idle_error(connection))
			return false;
		player_sql_cleanup cleanup;
		player_sql_transaction_cleanup transaction(connection, cleanup);
		transaction.starting();
		P_char actor = nullptr, mobile = nullptr;
		bool proven = false;
		try
		{
			if (!mysql_real_query(connection, "START TRANSACTION", 17))
			{
				economic_sql_native_quest_publication current;
				proven = authenticate(connection, state, &current, false) &&
					 native_quest_runtime_observe(state, current,
								      state.registry_returned) &&
					 native_quest_restored_cut(
						 state, current,
						 state.sealed.outcome ==
								 critical_apply_outcome::applied ||
							 state.sealed.outcome ==
								 critical_apply_outcome::
									 already_applied,
						 &actor, &mobile) &&
					 transaction.same_session();
			}
		}
		catch (...)
		{
			proven = false;
		}
		transaction.finish();
		lease.reuse(cleanup);
		auto found = native_quest_acceptances.find(state.recovery_key);
		if (!proven || !cleanup.rollback_confirmed || cleanup.cleanup_error ||
		    cleanup.disposition != player_sql_cleanup_disposition::idle_verified ||
		    !actor || !mobile || found == native_quest_acceptances.end() ||
		    found->second.publication.get() != &state ||
		    found->second.generation != state.generation ||
		    !found->second.restored_original ||
		    (!state.acknowledged &&
		     !player_save_native_quest_publication_owner::rebind_recovery_checkpoint(
			     *found->second.restored_original, actor->runtime_id)))
			return false;
		state.player_runtime_id = actor->runtime_id;
		state.native_runtime_id = mobile->runtime_id;
		found->second.player.actor_runtime_id = actor->runtime_id;
		found->second.native_runtime_id = mobile->runtime_id;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool item_native_quest_gameplay_publication_owner::restored_participant(
	const critical_command &command, item_native_quest_preparation_token *token_out,
	P_char *actor_out, P_char *mobile_out) noexcept
{
	if (!token_out || !actor_out || !mobile_out || actor_out == mobile_out ||
	    !nevent_is_game_thread())
		return false;
	try
	{
		auto found = native_quest_acceptances.find(operation_key(command.operation_id));
		if (found == native_quest_acceptances.end() || !found->second.command ||
		    !critical_command_equal(*found->second.command, command))
			return false;
		auto state = found->second.publication;
		if (!state || !state->restored || !state->completion_ready || state->blocked ||
		    !item_native_quest_publication_owner::rebind(*state))
			return false;
		P_char actor = find_character_by_runtime_id(state->player_runtime_id);
		P_char mobile = find_character_by_runtime_id(state->native_runtime_id);
		if (!actor || !mobile || !restored_token(command, token_out))
			return false;
		*actor_out = actor;
		*mobile_out = mobile;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool item_native_quest_gameplay_publication_owner::restore_budget(size_t bytes) noexcept
{
	// Private serialized startup observer only, before workers can execute.
	if (!native_quest_preparation_capacity(bytes, false))
		return false;
	native_quest_gameplay_retained_bytes = bytes;
	return true;
}

bool item_native_quest_gameplay_publication_owner::restored_token(
	const critical_command &command, item_native_quest_preparation_token *output) noexcept
{
	if (!output || !nevent_is_game_thread())
		return false;
	try
	{
		const auto found =
			native_quest_acceptances.find(operation_key(command.operation_id));
		if (found == native_quest_acceptances.end() || !found->second.command ||
		    !critical_command_equal(*found->second.command, command))
			return false;
		item_native_quest_preparation_token token;
		token.operation_ = command.operation_id;
		token.generation_ = found->second.generation;
		found->second.gameplay_retained = true;
		*output = token;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool item_native_quest_gameplay_publication_owner::restored_readback(
	const critical_native_recovery_envelope &envelope, P_char *actor_out,
	P_char *mobile_out) noexcept
{
#ifdef __NO_MYSQL__
	(void)envelope;
	(void)actor_out;
	(void)mobile_out;
	return false;
#else
	if (!actor_out || !mobile_out || actor_out == mobile_out || !nevent_is_game_thread() ||
	    envelope.phase != critical_native_recovery_phase::continuation_pending)
		return false;
	try
	{
		native_quest_publication_state state;
		native_quest_recovery_context context;
		state.command = std::make_shared<const critical_command>(envelope.command);
		if (!native_quest_restore_values(state, envelope, context) ||
		    !context.receipt.present ||
		    context.publication_stage !=
			    native_quest_recovery_publication_stage::physically_proven)
			return false;
		MYSQL *connection = sql_pool_acquire();
		player_sql_pool_lease lease(connection);
		if (!connection || player_sql_idle_error(connection))
			return false;
		player_sql_cleanup cleanup;
		player_sql_transaction_cleanup transaction(connection, cleanup);
		transaction.starting();
		P_char actor = nullptr, mobile = nullptr;
		bool proven = false;
		try
		{
			if (!mysql_real_query(connection, "START TRANSACTION", 17))
			{
				economic_sql_native_quest_publication current;
				proven = !economic_sql_native_quest_lock_recovered_continuation(
						 connection, envelope, state.native_before,
						 state.player_before, state.player_after,
						 state.payload.native_recovery
							 .acknowledged_save_revision,
						 &current) &&
					 native_quest_runtime_observe(state, current, true) &&
					 native_quest_restored_cut(
						 state, current,
						 context.receipt.outcome ==
								 critical_apply_outcome::applied ||
							 context.receipt.outcome ==
								 critical_apply_outcome::
									 already_applied,
						 &actor, &mobile) &&
					 transaction.same_session();
			}
		}
		catch (...)
		{
			proven = false;
		}
		transaction.finish();
		lease.reuse(cleanup);
		if (!proven || !cleanup.rollback_confirmed || cleanup.cleanup_error ||
		    cleanup.disposition != player_sql_cleanup_disposition::idle_verified)
			return false;
		*actor_out = actor;
		*mobile_out = mobile;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool item_native_quest_publication_owner::money_publish(const critical_command &command,
							const critical_completion &completion,
							void *opaque) noexcept
{
#ifdef __NO_MYSQL__
	(void)command;
	(void)completion;
	(void)opaque;
	return false;
#else
	if (!opaque || !nevent_is_game_thread())
		return false;
	auto &state = *static_cast<native_quest_publication_state *>(opaque);
	if (!state.command || !state.payload.native_money.present ||
	    !critical_command_equal(command, *state.command) ||
	    !same_item_publication_receipt(state.sealed, completion))
		return false;
	const bool success = completion.outcome == critical_apply_outcome::applied ||
			     completion.outcome == critical_apply_outcome::already_applied;
	if (!success && completion.outcome != critical_apply_outcome::terminal_failure)
		return false;
	try
	{
		const auto key = operation_key(command.operation_id);
		const auto owner_current = [&]()
		{
			const auto found = native_quest_acceptances.find(key);
			return !state.blocked && state.inflight &&
			       found != native_quest_acceptances.end() &&
			       found->second.publication.get() == &state &&
			       found->second.generation == state.generation &&
			       same_item_publication_receipt(state.sealed, completion);
		};
		if (!recovery_initialize(state) ||
		    (state.recovery_pending_ready && !recovery_checkpoint(state)) ||
		    !owner_current())
			return false;
		// A cold started/unreturned action is uncertain even when endpoints look
		// AFTER. Never restart or independently repair one wallet endpoint.
		if (state.detach_started && !state.detach_returned && !state.recovery_not_attempted)
			return false;
		MYSQL *connection = sql_pool_acquire();
		player_sql_pool_lease lease(connection);
		if (!connection || player_sql_idle_error(connection))
			return false;
		player_sql_cleanup cleanup;
		player_sql_transaction_cleanup transaction(connection, cleanup);
		transaction.starting();
		bool proven = false;
		try
		{
			if (mysql_real_query(connection, "START TRANSACTION", 17))
				throw EIO;
			player_native_quest_checkpoint_stage held;
			std::vector<player_item_snapshot> before, after;
			if (!player_save_native_quest_publication_owner::publication_held_bodies(
				    command, &before, &after, &held) ||
			    !native_quest_world::same_items(before, state.player_before) ||
			    !native_quest_world::same_items(after, state.player_after))
				throw EAGAIN;
			economic_sql_native_money_publication current;
			const auto authenticate = [&](economic_sql_native_money_publication *out)
			{
				return economic_sql_native_money_lock_publication(
					       connection, command, completion, state.native_before,
					       before, after, held.save_revision, out) == 0 &&
				       out->original.session_id == mysql_thread_id(connection) &&
				       owner_current();
			};
			if (!authenticate(&current))
				throw EAGAIN;
			state.prepared = true;
			if (!state.recovery_receipt_durable &&
			    !recovery_prepare(state, NATIVE_RECOVERY_PUBLISHING))
				throw EAGAIN;
			native_quest_world::census seen;
			P_char actor = nullptr, mobile = nullptr;
			const auto observe = [&]()
			{
				return owner_current() &&
				       native_money_world_observe(state, current,
								  success && state.detach_returned,
								  &seen, &actor, &mobile);
			};
			if (!observe())
				throw EAGAIN;
			if (success && !state.detach_returned)
			{
				if (!recovery_begin(state, 1) || !observe())
					throw EAGAIN;
				quest_mobile_native_cash_reference original;
				if (!quest_mobile_native_cash_reference_copy(
					    mobile, state.native_runtime_id, &original) ||
				    original.cash_revision != state.payload.native_money.projection
								      .mobile_before_revision ||
				    original.denominations !=
					    state.payload.native_money.projection.mobile_before)
					throw EAGAIN;
				state.detach_started = true;
				state.recovery_not_attempted = false;
				const bool applied =
					quest_mobile_native_publication_binding::apply_money(
						actor, state.player_runtime_id, state.player_pid,
						mobile, state.native_runtime_id, original,
						state.payload.native_money.projection,
						current.original.native.reference);
				if (!applied)
				{
					state.blocked = true;
					throw EAGAIN;
				}
				state.detach_returned = true;
				if (!recovery_returned(state) || !observe())
					throw EAGAIN;
			}
			economic_sql_native_money_publication final;
			if (!authenticate(&final) ||
			    final.original.session_id != current.original.session_id || !observe())
				throw EAGAIN;
			if (success && state.give_messages[0] != 2)
			{
				if (state.give_messages[0] == 1)
					throw EAGAIN;
				const int room =
					real_room(state.payload.native_money.original_room_vnum);
				if (!world || room < 0 || room > top_of_world ||
				    world[room].number !=
					    state.payload.native_money.original_room_vnum)
					throw EAGAIN;
				if (!recovery_begin(state, 6) || !observe())
					throw EAGAIN;
				state.give_messages[0] = 1;
				state.recovery_not_attempted = false;
				const bool announced = quest_native_coin_give_notice_owner::publish(
					actor, mobile,
					state.payload.native_money.projection.denomination,
					state.payload.native_money.projection.quantity,
					state.payload.native_money.original_room_vnum);
				if (!announced)
				{
					state.blocked = true;
					throw EAGAIN;
				}
				state.give_messages[0] = 2;
				if (!recovery_returned(state) || !observe())
					throw EAGAIN;
			}
			// Original coin feedback only; no quester, item/registry hooks, child
			// continuation or source issuance occurs here.
			if (!state.recovery_physically_proven &&
			    !(state.recovery_pending &&
			      state.recovery_effect == NATIVE_RECOVERY_PROVEN) &&
			    !recovery_prepare(state, NATIVE_RECOVERY_PROVEN))
				throw EAGAIN;
			proven = true;
		}
		catch (...)
		{
			proven = false;
		}
		transaction.finish();
		lease.reuse(cleanup);
		if (!proven || !transaction.same_session() || !cleanup.rollback_confirmed ||
		    cleanup.disposition != player_sql_cleanup_disposition::idle_verified)
			return false;
		state.recovery_pending_ready = true;
		return recovery_checkpoint(state) && state.recovery_physically_proven;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool item_native_quest_publication_owner::fee_publish(const critical_command &command,
						      const critical_completion &completion,
						      void *opaque) noexcept
{
#ifdef __NO_MYSQL__
	(void)command;
	(void)completion;
	(void)opaque;
	return false;
#else
	if (!opaque || !nevent_is_game_thread())
		return false;
	auto &state = *static_cast<native_quest_publication_state *>(opaque);
	if (!state.command || !state.payload.native_cost.fee_only ||
	    !critical_command_equal(command, *state.command) ||
	    !same_item_publication_receipt(state.sealed, completion))
		return false;
	const bool success = completion.outcome == critical_apply_outcome::applied ||
			     completion.outcome == critical_apply_outcome::already_applied;
	if (!success && completion.outcome != critical_apply_outcome::terminal_failure)
		return false;
	try
	{
		const auto key = operation_key(command.operation_id);
		const auto owner_current = [&]()
		{
			const auto found = native_quest_acceptances.find(key);
			return !state.blocked && state.inflight &&
			       found != native_quest_acceptances.end() &&
			       found->second.publication.get() == &state &&
			       found->second.generation == state.generation &&
			       same_item_publication_receipt(state.sealed, completion);
		};
		if (!recovery_initialize(state) ||
		    (state.recovery_pending_ready && !recovery_checkpoint(state)) ||
		    !owner_current())
			return false;
		// A cold started/unreturned action is uncertain even when endpoints look
		// AFTER. Never restart or independently repair one wallet endpoint.
		if (state.detach_started && !state.detach_returned && !state.recovery_not_attempted)
			return false;
		MYSQL *connection = sql_pool_acquire();
		player_sql_pool_lease lease(connection);
		if (!connection || player_sql_idle_error(connection))
			return false;
		player_sql_cleanup cleanup;
		player_sql_transaction_cleanup transaction(connection, cleanup);
		transaction.starting();
		bool proven = false;
		try
		{
			if (mysql_real_query(connection, "START TRANSACTION", 17))
				throw EIO;
			player_native_quest_checkpoint_stage held;
			std::vector<player_item_snapshot> before, after;
			if (!player_save_native_quest_publication_owner::publication_held_bodies(
				    command, &before, &after, &held) ||
			    !native_quest_world::same_items(before, state.player_before) ||
			    !native_quest_world::same_items(after, state.player_after))
				throw EAGAIN;
			economic_sql_native_fee_publication current;
			const auto authenticate = [&](economic_sql_native_fee_publication *out)
			{
				return economic_sql_native_fee_lock_publication(
					       connection, command, completion, state.native_before,
					       before, after, held.save_revision, out) == 0 &&
				       out->original.session_id == mysql_thread_id(connection) &&
				       owner_current();
			};
			if (!authenticate(&current))
				throw EAGAIN;
			state.prepared = true;
			if (!state.recovery_receipt_durable &&
			    !recovery_prepare(state, NATIVE_RECOVERY_PUBLISHING))
				throw EAGAIN;
			native_quest_world::census seen;
			P_char actor = nullptr, mobile = nullptr;
			const auto observe = [&]()
			{
				return owner_current() &&
				       native_fee_world_observe(state, current,
								success && state.detach_returned,
								&seen, &actor, &mobile);
			};
			if (!observe())
				throw EAGAIN;
			// Original preliminary quest message precedes the first SUB_MONEY.
			// Started/unreturned is uncertain and is never repeated on a cold replay.
			if (success && !state.message_returned)
			{
				if (state.message_started && !state.recovery_not_attempted)
					throw EAGAIN;
				const auto &terms = state.payload.native_recovery.publication_terms;
				if (!recovery_begin(state, 5) || !observe())
					throw EAGAIN;
				state.message_started = true;
				state.recovery_not_attempted = false;
				if (!terms.message.empty())
					act(terms.message.c_str(), FALSE, mobile, nullptr, actor,
					    terms.echo_all ? TO_ROOM : TO_VICT);
				state.message_returned = true;
				if (!recovery_returned(state) || !observe())
					throw EAGAIN;
			}
			if (success && !state.detach_returned)
			{
				if (!recovery_begin(state, 1) || !observe())
					throw EAGAIN;
				quest_mobile_native_cash_reference original;
				if (!quest_mobile_native_cash_reference_copy(
					    mobile, state.native_runtime_id, &original) ||
				    original.cash_revision !=
					    state.payload.native_cost.projection.before_revision ||
				    original.denominations !=
					    state.payload.native_cost.projection.before)
					throw EAGAIN;
				state.detach_started = true;
				state.recovery_not_attempted = false;
				const bool applied =
					quest_mobile_native_publication_binding::apply_cost(
						mobile, state.native_runtime_id, original,
						state.payload.native_cost.projection,
						current.original.native.reference);
				if (!applied)
				{
					state.blocked = true;
					throw EAGAIN;
				}
				state.detach_returned = true;
				if (!recovery_returned(state) || !observe())
					throw EAGAIN;
			}
			economic_sql_native_fee_publication final;
			if (!authenticate(&final) ||
			    final.original.session_id != current.original.session_id || !observe())
				throw EAGAIN;
			// No item/custody/GIVE hooks; the genuine v6 reward owner continues
			// separately after actual fee cash and preliminary message proof.
			if (!state.recovery_physically_proven &&
			    !(state.recovery_pending &&
			      state.recovery_effect == NATIVE_RECOVERY_PROVEN) &&
			    !recovery_prepare(state, NATIVE_RECOVERY_PROVEN))
				throw EAGAIN;
			proven = true;
		}
		catch (...)
		{
			proven = false;
		}
		transaction.finish();
		lease.reuse(cleanup);
		if (!proven || !transaction.same_session() || !cleanup.rollback_confirmed ||
		    cleanup.disposition != player_sql_cleanup_disposition::idle_verified)
			return false;
		state.recovery_pending_ready = true;
		return recovery_checkpoint(state) && state.recovery_physically_proven;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool item_native_quest_publication_owner::native_publish(const critical_command &command,
							 const critical_completion &completion,
							 void *opaque) noexcept
{
#ifdef __NO_MYSQL__
	(void)command;
	(void)completion;
	(void)opaque;
	return false;
#else
	if (!opaque || !nevent_is_game_thread())
		return false;
	auto &state = *static_cast<native_quest_publication_state *>(opaque);
	if (!state.command || !same_item_publication_receipt(state.sealed, completion) ||
	    completion.disposition != critical_completion_disposition::execution ||
	    command.operation_id.bytes != state.command->operation_id.bytes)
		return false;
	if (state.payload.native_money.present)
		return money_publish(command, completion, opaque);
	if (state.payload.native_cost.fee_only)
		return fee_publish(command, completion, opaque);
	const bool success = completion.outcome == critical_apply_outcome::applied ||
			     completion.outcome == critical_apply_outcome::already_applied;
	if ((!success && completion.outcome != critical_apply_outcome::terminal_failure) ||
	    // The original whole stock/cash/lifetime retirement owner is required.
	    // Retaining its true decision must never silently omit disappearance.
	    (success && state.payload.native_recovery.publication_terms.disappear))
		return false;
	try
	{
		const std::string original_key = operation_key(state.command->operation_id);
		const auto owner_current = [&]()
		{
			const auto found = native_quest_acceptances.find(original_key);
			return !state.blocked && state.inflight &&
			       same_item_publication_receipt(state.sealed, completion) &&
			       found != native_quest_acceptances.end() &&
			       found->second.generation == state.generation &&
			       found->second.publication.get() == &state;
		};
		if (!recovery_initialize(state) ||
		    (state.recovery_pending_ready && !recovery_checkpoint(state)) ||
		    !owner_current())
			return false;
		MYSQL *connection = sql_pool_acquire();
		player_sql_pool_lease lease(connection);
		if (!connection || player_sql_idle_error(connection))
			return false;
		player_sql_cleanup cleanup;
		player_sql_transaction_cleanup transaction(connection, cleanup);
		transaction.starting();
		bool proven = false;
		try
		{
			if (mysql_real_query(connection, "START TRANSACTION", 17))
				throw EIO;
			economic_sql_native_quest_publication current;
			if (!authenticate(connection, state, &current) ||
			    !native_quest_runtime_observe(state, current, state.registry_returned))
				throw EAGAIN;
			state.prepared = true;
			// The exact original SQL-authenticated receipt is durable before effects.
			if (!state.recovery_receipt_durable &&
			    !recovery_prepare(state, NATIVE_RECOVERY_PUBLISHING))
				throw EAGAIN;
			auto reference = state.payload.native_mobile.reference;
			auto native_items = state.native_before;
			auto player_items = state.player_before;
			std::vector<player_item_snapshot> detached;
			const bool acceptance = state.payload.native_mobile.action ==
						item_native_mobile_action::acceptance;
			if (success && acceptance && state.detach_returned)
			{
				player_items = state.player_after;
				if (!state.place_returned)
					detached = state.selected;
				else
					native_items = state.native_after;
			}
			if (success && !acceptance)
			{
				for (size_t i = 0; i < state.root_stages.size(); ++i)
				{
					if (state.root_stages[i] == 1)
						throw EAGAIN; // Started and unreturned extraction never retries.
					if (state.root_stages[i] != 2)
						continue;
					std::vector<player_item_snapshot> removed, remaining;
					if (player_item_snapshot_extract_subtree(
						    native_items,
						    state.payload.native_recovery
							    .consumed_root_order[i],
						    &removed,
						    &remaining) != player_snapshot_codec_result::ok)
						throw EAGAIN;
					native_items = std::move(remaining);
				}
			}
			if (state.binding_returned)
				reference = current.native.reference;
			native_quest_world::census seen;
			P_char actor = nullptr, mobile = nullptr;
			const auto observe = [&]()
			{
				return owner_current() && transaction.same_session() &&
				       native_quest_world_observe(state, player_items, native_items,
								  detached, reference,
								  *current.native.cash, &seen,
								  &actor, &mobile);
			};
			if ((state.detach_started && !state.detach_returned) ||
			    (state.place_started && !state.place_returned) ||
			    (state.registry_started && !state.registry_returned) ||
			    (state.binding_started && !state.binding_returned) ||
			    (state.message_started && !state.message_returned) || !observe())
				throw EAGAIN;
			if (success && acceptance && !state.detach_returned)
			{
				P_obj root = seen.unique(state.payload.selected_item_uid);
				if (!root || !OBJ_CARRIED_BY(root, actor))
					throw EAGAIN;
				if (!recovery_begin(state, 1) || !observe())
					throw EAGAIN;
				root = seen.unique(state.payload.selected_item_uid);
				if (!root || !OBJ_CARRIED_BY(root, actor))
					throw EAGAIN;
				state.detach_started = true;
				state.recovery_not_attempted = false;
				obj_from_char(root);
				state.detach_returned = true;
				if (!recovery_returned(state))
					throw EAGAIN;
				player_items = state.player_after;
				detached = state.selected;
				if (!observe())
					throw EAGAIN;
			}
			if (success && acceptance)
				for (size_t i = 0; i < state.give_messages.size(); ++i)
				{
					if (state.give_messages[i] == 1)
						throw EAGAIN;
					if (state.give_messages[i] == 2)
						continue;
					if (!observe())
						throw EAGAIN;
					P_obj root = seen.unique(state.payload.selected_item_uid);
					if (!root)
						throw EAGAIN;
					if (!recovery_begin(state, static_cast<uint16_t>(6 + i)) ||
					    !observe())
						throw EAGAIN;
					root = seen.unique(state.payload.selected_item_uid);
					if (!root)
						throw EAGAIN;
					state.give_messages[i] = 1;
					state.recovery_not_attempted = false;
					if (!i)
						act("$n gives $p to $N.", 1, actor, root, mobile,
						    TO_NOTVICT);
					else if (i == 1)
						act("$n gives you $p.", 0, actor, root, mobile,
						    TO_VICT);
					else
						send_to_char("Ok.\r\n", actor);
					state.give_messages[i] = 2;
					if (!recovery_returned(state))
						throw EAGAIN;
					if (!observe())
						throw EAGAIN;
				}
			if (success && acceptance && !state.place_returned)
			{
				P_obj root = seen.unique(state.payload.selected_item_uid);
				if (!root || !OBJ_NOWHERE(root))
					throw EAGAIN;
				if (!recovery_begin(state, 2) || !observe())
					throw EAGAIN;
				root = seen.unique(state.payload.selected_item_uid);
				if (!root || !OBJ_NOWHERE(root))
					throw EAGAIN;
				state.place_started = true;
				state.recovery_not_attempted = false;
				const auto placed = obj_to_char_checked(root, mobile);
				state.place_returned = true;
				if (placed != obj_to_char_result::placed)
					state.blocked = true;
				if (!recovery_returned(state))
					throw EAGAIN;
				if (placed != obj_to_char_result::placed)
				{
					state.blocked = true;
					throw EAGAIN; // Returned refusal/destruction is not a completed target.
				}
				detached.clear();
				native_items = state.native_after;
				if (!observe())
					throw EAGAIN;
			}
			if (success && !acceptance && !state.message_returned)
			{
				const auto &terms = state.payload.native_recovery.publication_terms;
				if (!recovery_begin(state, 5) || !observe())
					throw EAGAIN;
				state.message_started = true;
				state.recovery_not_attempted = false;
				if (!terms.message.empty())
					act(terms.message.c_str(), FALSE, mobile, nullptr, actor,
					    terms.echo_all ? TO_ROOM : TO_VICT);
				state.message_returned = true;
				if (!recovery_returned(state))
					throw EAGAIN;
				if (!observe())
					throw EAGAIN;
			}
			if (success && !acceptance)
				for (size_t i = 0; i < state.root_stages.size(); ++i)
				{
					if (state.root_stages[i] == 2)
						continue;
					if (!observe())
						throw EAGAIN;
					const uint64_t uid =
						state.payload.native_recovery.consumed_root_order[i];
					P_obj root = seen.unique(uid);
					std::vector<player_item_snapshot> removed, remaining;
					if (!root || !OBJ_CARRIED_BY(root, mobile) ||
					    player_item_snapshot_extract_subtree(
						    native_items, uid, &removed, &remaining) !=
						    player_snapshot_codec_result::ok)
						throw EAGAIN;
					if (!recovery_begin(state,
							    static_cast<uint16_t>(
								    NATIVE_RECOVERY_ROOT + i)) ||
					    !observe())
						throw EAGAIN;
					root = seen.unique(uid);
					if (!root || !OBJ_CARRIED_BY(root, mobile))
						throw EAGAIN;
					state.root_stages[i] = 1;
					state.recovery_not_attempted = false;
					extract_obj(root, TRUE);
					state.root_stages[i] = 2;
					if (!recovery_returned(state))
						throw EAGAIN;
					native_items = std::move(remaining);
					if (!observe())
						throw EAGAIN;
				}
			if (!state.registry_returned)
			{
				if (!native_quest_world::same_items(
					    native_items,
					    success ? state.native_after : state.native_before) ||
				    !native_quest_runtime_observe(state, current, false))
					throw EAGAIN;
				if (!recovery_begin(state, 3) || !observe() ||
				    !native_quest_runtime_observe(state, current, false))
					throw EAGAIN;
				state.registry_started = true;
				state.recovery_not_attempted = false;
				const bool applied =
					item_ownership_runtime_native_quest_publication_owner::apply(
						state.payload, state.result,
						success ?
							item_ownership_runtime_native_quest_publication_owner::
								publication_result::applied :
							item_ownership_runtime_native_quest_publication_owner::
								publication_result::rejected,
						current.custody, current.from_owner_revision,
						current.to_owner_revision,
						current.player_owner_revision);
				state.registry_returned = applied;
				if (!applied)
					state.blocked = true;
				if (!recovery_returned(state))
					throw EAGAIN;
				if (!applied)
					throw EAGAIN;
				if (!native_quest_runtime_observe(state, current, true) ||
				    !observe())
					throw EAGAIN;
			}
			if (success && !state.binding_returned)
			{
				if (!recovery_begin(state, 4) || !observe())
					throw EAGAIN;
				state.binding_started = true;
				state.recovery_not_attempted = false;
				const bool advanced =
					quest_mobile_native_publication_binding::advance(
						mobile, state.native_runtime_id,
						state.payload.native_mobile.reference,
						current.native.reference);
				state.binding_returned = advanced;
				state.binding_refused = !advanced;
				if (!recovery_returned(state) || !advanced)
					throw EAGAIN;
				reference = current.native.reference;
				if (!observe())
					throw EAGAIN;
			}
			economic_sql_native_quest_publication final;
			if (!authenticate(connection, state, &final) ||
			    final.session_id != current.session_id ||
			    !native_quest_runtime_observe(state, final, true) || !observe() ||
			    (success && !native_quest_frozen_reward_read(connection, state)))
				throw EAGAIN;
			if (!state.recovery_physically_proven &&
			    !(state.recovery_pending &&
			      state.recovery_effect == NATIVE_RECOVERY_PROVEN) &&
			    !recovery_prepare(state, NATIVE_RECOVERY_PROVEN))
				throw EAGAIN;
			proven = true;
		}
		catch (...)
		{
			proven = false;
		}
		transaction.finish();
		lease.reuse(cleanup);
		if (!proven || !transaction.same_session() || !cleanup.rollback_confirmed ||
		    cleanup.disposition != player_sql_cleanup_disposition::idle_verified)
			return false;
		// Only the final exact original SQL/world/reward proof and confirmed
		// same-session rollback make this preallocated physical fact writable.
		state.recovery_pending_ready = true;
		return recovery_checkpoint(state) && state.recovery_physically_proven;
	}
	catch (...)
	{
		return false;
	}
#endif
}

void item_native_quest_publication_owner::completions(const critical_completion *incoming,
						      size_t count) noexcept
{
	if (!nevent_is_game_thread() || (count && !incoming))
		return;
#ifndef __NO_MYSQL__
	try
	{
		// Existing movement completion ticks own bounded original money
		// preparation; no quest trigger or new unrelated event stream.
		std::vector<std::string> money_preparations;
		for (const auto &[key, entry] : native_quest_acceptances)
			if (entry.money_only && !entry.submission_started &&
			    !entry.money_preparation_blocked)
				money_preparations.push_back(key);
		for (const auto &key : money_preparations)
		{
			auto found = native_quest_acceptances.find(key);
			if (found == native_quest_acceptances.end() || !found->second.money_only ||
			    found->second.submission_started ||
			    found->second.money_preparation_blocked)
				continue;
			item_native_quest_preparation_token token;
			if (key.size() != token.operation_.bytes.size())
				continue;
			std::copy(key.begin(), key.end(), token.operation_.bytes.begin());
			token.generation_ = found->second.generation;
			P_char actor =
				find_character_by_runtime_id(found->second.player.actor_runtime_id);
			P_char mobile =
				find_character_by_runtime_id(found->second.native_runtime_id);
			const auto state =
				item_native_quest_preparation_owner::poll_money_acceptance(
					token, actor, mobile);
			if (state == item_native_quest_preparation_state::pending)
				continue;
			if (state != item_native_quest_preparation_state::ready)
			{
				(void)item_native_quest_preparation_owner::cancel(token);
				continue;
			}
			const auto submitted =
				item_native_quest_preparation_owner::submit_money_acceptance(
					token, actor, mobile);
			found = native_quest_acceptances.find(key);
			if (found != native_quest_acceptances.end() &&
			    found->second.generation == token.generation_ &&
			    (submitted == critical_submit_result::invalid ||
			     submitted == critical_submit_result::identity_conflict))
				found->second.money_preparation_blocked = true;
			// Original uncertain admission retains its token/immutable command;
			// only known unavailability retries, through the original owner.
		}
		// Validate the entire delivered batch before the first native effect.
		for (size_t i = 0; i < count; ++i)
		{
			auto found = native_quest_acceptances.find(
				operation_key(incoming[i].operation_id));
			if (found == native_quest_acceptances.end() || !found->second.command)
				continue;
			auto &entry = found->second;
			if (!entry.publication)
			{
				auto state = std::make_shared<native_quest_publication_state>();
				state->recovery_key = found->first;
				state->generation = entry.generation;
				state->native_runtime_id = entry.native_runtime_id;
				state->player_runtime_id = entry.player.actor_runtime_id;
				state->player_pid = entry.player.pid;
				state->command = entry.command;
				if (!item_transfer_command_decode_payload(*state->command,
									  &state->payload) ||
				    !item_transfer_native_mobile_recovery_shape_valid(
					    state->payload) ||
				    player_item_snapshot_list_decode(entry.native_before.data(),
								     entry.native_before.size(),
								     &state->native_before) !=
					    player_snapshot_codec_result::ok ||
				    (!state->payload.native_money.present &&
				     !state->payload.native_cost.fee_only &&
				     (player_item_snapshot_list_decode(
					      state->payload.item_blob.data(),
					      state->payload.item_blob_size, &state->selected) !=
					      player_snapshot_codec_result::ok ||
				      quest_mobile_native_items_transition(
					      state->native_before, entry.reference, state->payload,
					      &state->native_after) !=
					      player_snapshot_codec_result::ok)))
					continue;
				if (state->payload.native_money.present ||
				    state->payload.native_cost.fee_only)
					state->native_after = state->native_before;
				state->root_stages.resize(
					state->payload.native_recovery.consumed_root_order.size(),
					0);
				size_t retained =
					sizeof(native_quest_publication_state) +
					state->recovery_key.size() +
					state->payload.continuation.data.size() +
					state->payload.native_recovery.consumed_root_order.size() *
						sizeof(uint64_t) +
					state->payload.native_recovery.publication_terms.message
						.size() +
					state->payload.native_recovery.publication_terms
						.disappear_message.size() +
					state->root_stages.size() +
					ITEM_TRANSFER_CONTINUATION_MAX_BYTES +
					QUEST_REWARD_MAX_CHARACTER_NAME_BYTES +
					QUEST_REWARD_MAX_DEFINITION_ID_BYTES;
				for (const auto *forest :
				     { &state->native_before, &state->native_after,
				       &state->selected })
				{
					std::vector<uint8_t> encoded;
					if (player_item_snapshot_list_encode(*forest, &encoded) !=
						    player_snapshot_codec_result::ok ||
					    encoded.size() >
						    PLAYER_SAVE_PIPELINE_MAX_BYTES - retained)
						throw std::bad_alloc();
					retained += encoded.size();
				}
				// Full actual held player bodies are retained before any effect.
				player_native_quest_checkpoint_stage stage{};
				if (!player_save_native_quest_publication_owner::
					    publication_held_bodies(*state->command,
								    &state->player_before,
								    &state->player_after, &stage))
					continue;
				for (const auto *forest :
				     { &state->player_before, &state->player_after })
				{
					std::vector<uint8_t> encoded;
					if (player_item_snapshot_list_encode(*forest, &encoded) !=
						    player_snapshot_codec_result::ok ||
					    encoded.size() >
						    PLAYER_SAVE_PIPELINE_MAX_BYTES - retained)
						throw std::bad_alloc();
					retained += encoded.size();
				}
				if (!native_quest_preparation_capacity(retained))
					continue;
				entry.publication_bytes = retained;
				entry.publication = std::move(state);
			}
			auto state = entry.publication;
			if (!item_publication_receipt_valid(incoming[i]) ||
			    (state->completion_ready &&
			     !same_item_publication_receipt(state->sealed, incoming[i]) &&
			     incoming[i].outcome != critical_apply_outcome::retryable_failure &&
			     incoming[i].outcome != critical_apply_outcome::ambiguous_commit))
			{
				state->blocked = true;
				continue;
			}
			if (incoming[i].outcome == critical_apply_outcome::retryable_failure ||
			    incoming[i].outcome == critical_apply_outcome::ambiguous_commit)
				continue;
			if (!state->completion_ready)
			{
				if (state->restored && state->recovery)
				{
					native_quest_recovery_context retained;
					if (native_quest_recovery_context_decode(
						    *state->command, state->recovery->attachment,
						    &retained) != player_snapshot_codec_result::ok)
						continue;
					if (retained.receipt.present)
					{
						const bool same_outcome =
							retained.receipt.outcome ==
								incoming[i].outcome ||
							((retained.receipt.outcome ==
								  critical_apply_outcome::applied ||
							  retained.receipt.outcome ==
								  critical_apply_outcome::
									  already_applied) &&
							 (incoming[i].outcome ==
								  critical_apply_outcome::applied ||
							  incoming[i].outcome ==
								  critical_apply_outcome::
									  already_applied));
						if (!same_outcome ||
						    retained.receipt.durable_revision !=
							    incoming[i].durable_revision ||
						    retained.receipt.error_code !=
							    incoming[i].error_code ||
						    retained.receipt.failure_stage !=
							    incoming[i].failure_stage ||
						    retained.receipt.result_size !=
							    incoming[i].result_size ||
						    retained.receipt.result_payload !=
							    incoming[i].result_payload)
						{
							state->blocked = true;
							continue;
						}
					}
				}
				state->sealed = incoming[i];
				state->completion_ready = true;
				if (state->sealed.disposition !=
					    critical_completion_disposition::execution ||
				    !(state->payload.native_money.present ?
					      item_native_mobile_money_result_decode(
						      { state->sealed.result_payload.data(),
							state->sealed.result_size },
						      &state->money_result) :
				      state->payload.native_cost.fee_only ?
					      item_native_mobile_fee_result_decode(
						      { state->sealed.result_payload.data(),
							state->sealed.result_size },
						      &state->fee_result) :
					      item_transfer_command_decode_result(
						      state->sealed.result_payload.data(),
						      state->sealed.result_size, &state->result)))
					state->blocked = true;
			}
		}
		std::vector<std::string> keys;
		keys.reserve(native_quest_acceptances.size());
		for (const auto &[key, entry] : native_quest_acceptances)
			if (entry.publication)
				keys.push_back(key);
		for (const auto &key : keys)
		{
			auto found = native_quest_acceptances.find(key);
			if (found == native_quest_acceptances.end())
				continue;
			auto state = found->second.publication; // Keep opaque callback state alive.
			if (!state || !state->completion_ready ||
			    (state->blocked && !state->recovery_pending) || state->inflight)
				continue;
			if (state->restored &&
			    (!state->player_runtime_id || !state->native_runtime_id ||
			     !find_character_by_runtime_id(state->player_runtime_id) ||
			     !find_character_by_runtime_id(state->native_runtime_id)) &&
			    !rebind(*state))
				continue;
			if (!state->acknowledged)
			{
				state->inflight = true;
				const bool published = player_save_native_quest_publication_owner::
					publish_native_quest(*state->command, state->sealed,
							     native_publish, state.get());
				state->inflight = false;
				// Reacquire the map after every callback; no iterator is borrowed.
				found = native_quest_acceptances.find(key);
				if (found == native_quest_acceptances.end() ||
				    found->second.publication != state ||
				    found->second.generation != state->generation)
					continue;
				if (!published)
					continue;
				state->acknowledged =
					true; // Only the private guard consumed its hold.
			}
			if (state->payload.native_money.present)
			{
				state->restoration_handed_off = true;
				state->continuation_returned = true;
			}
			if (state->restored && !state->restoration_handed_off)
			{
				if (!quest_native_frozen_continuation_owner::published(
					    *state->command))
					continue;
				state->restoration_handed_off = true;
				state->continuation_returned = true;
			}
			// The private continuation owner settles its exact pending writer
			// and retains actual started/unreturned effects without repeating them.
			if (!state->continuation_returned)
			{
				if (state->reward &&
				    !quest_native_frozen_continuation_owner::publish(
					    *state->command, *state->reward,
					    state->player_runtime_id, &state->continuation_started))
					continue;
				state->continuation_returned = true;
			}
			found = native_quest_acceptances.find(key);
			if (found != native_quest_acceptances.end() &&
			    found->second.publication == state &&
			    found->second.generation == state->generation &&
			    !found->second.gameplay_retained)
				native_quest_acceptances.erase(found);
		}
	}
	catch (...)
	{
		// Every original hold/state survives allocation and callback uncertainty.
	}
#else
	(void)incoming;
	(void)count;
#endif
}

bool item_native_quest_gameplay_publication_owner::retain(
	const item_native_quest_preparation_token &token) noexcept
{
	if (!nevent_is_game_thread())
		return false;
	try
	{
		auto found = native_quest_acceptances.find(operation_key(token.operation_));
		if (found == native_quest_acceptances.end() ||
		    found->second.generation != token.generation_ ||
		    found->second.submission_started)
			return false;
		found->second.gameplay_retained = true;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

item_native_quest_gameplay_result item_native_quest_gameplay_publication_owner::observe_completed(
	const item_native_quest_preparation_token &token,
	std::shared_ptr<const critical_command> *output) noexcept
{
	using outcome = item_native_quest_gameplay_result;
	if (!output || !nevent_is_game_thread())
		return outcome::unavailable;
	try
	{
		auto found = native_quest_acceptances.find(operation_key(token.operation_));
		if (found == native_quest_acceptances.end() ||
		    found->second.generation != token.generation_ ||
		    !found->second.gameplay_retained)
			return outcome::unavailable;
		const auto &state = found->second.publication;
		if (!state || !state->command || state->generation != token.generation_ ||
		    state->command->operation_id.bytes != token.operation_.bytes ||
		    state->command != found->second.command || !state->acknowledged ||
		    !state->continuation_returned || state->inflight || state->blocked)
			return outcome::pending;
		const bool applied = state->sealed.outcome == critical_apply_outcome::applied ||
				     state->sealed.outcome ==
					     critical_apply_outcome::already_applied;
		const auto result = !applied ? outcome::rejected :
				    state->payload.native_mobile.action ==
						    item_native_mobile_action::acceptance ?
					       outcome::acceptance_applied :
				    state->payload.continuation.kind ==
						    item_transfer_continuation_kind::none ?
					       outcome::prefix_applied :
					       outcome::completion_applied;
		*output = state->command; // Nonallocating; original entry remains retained.
		return result;
	}
	catch (...)
	{
		return outcome::unavailable;
	}
}

item_native_quest_gameplay_result item_native_quest_gameplay_publication_owner::take(
	const item_native_quest_preparation_token &token) noexcept
{
	using outcome = item_native_quest_gameplay_result;
	if (!nevent_is_game_thread())
		return outcome::unavailable;
	try
	{
		auto found = native_quest_acceptances.find(operation_key(token.operation_));
		if (found == native_quest_acceptances.end() ||
		    found->second.generation != token.generation_ ||
		    !found->second.gameplay_retained)
			return outcome::unavailable;
		const auto &state = found->second.publication;
		if (!state || !state->acknowledged || !state->continuation_returned ||
		    state->inflight || state->blocked)
			return outcome::pending;
		const bool applied = state->sealed.outcome == critical_apply_outcome::applied ||
				     state->sealed.outcome ==
					     critical_apply_outcome::already_applied;
		const auto result = !applied ? outcome::rejected :
				    state->payload.native_mobile.action ==
						    item_native_mobile_action::acceptance ?
					       outcome::acceptance_applied :
				    state->payload.continuation.kind ==
						    item_transfer_continuation_kind::none ?
					       outcome::prefix_applied :
					       outcome::completion_applied;
		native_quest_acceptances.erase(found);
		return result;
	}
	catch (...)
	{
		return outcome::unavailable;
	}
}

bool item_native_quest_gameplay_publication_owner::observe_give(
	uint64_t player_runtime, uint32_t pid, uint64_t native_runtime, uint64_t original_root_uid,
	P_char *player, P_char *mobile, P_obj *root) noexcept
{
	if (!nevent_is_game_thread() || !player || !mobile || !root || !player_runtime ||
	    !native_runtime || !pid)
		return false;
	try
	{
		native_quest_world::census seen;
		if (!seen.scan())
			return false;
		P_char actor = find_character_by_runtime_id(player_runtime);
		P_char npc = find_character_by_runtime_id(native_runtime);
		if (!actor || !npc || !IS_PC(actor) || !actor->only.pc || !IS_NPC(npc) ||
		    GET_PID(actor) != static_cast<int>(pid) || !seen.seen_bodies.count(actor) ||
		    !seen.seen_bodies.count(npc))
			return false;
		P_obj selected = original_root_uid ? seen.unique(original_root_uid) : nullptr;
		if (original_root_uid && (!selected || !OBJ_CARRIED_BY(selected, npc)))
			return false;
		*player = actor;
		*mobile = npc;
		*root = selected;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool item_native_quest_birth_budget_owner::retained_budget(size_t actual_birth_bytes) noexcept
{
	if (!nevent_is_game_thread() ||
	    !native_quest_preparation_capacity(actual_birth_bytes, true, false))
		return false;
	native_quest_birth_retained_bytes = actual_birth_bytes;
	return true;
}

bool item_native_quest_gameplay_publication_owner::retained_budget(
	size_t actual_driver_bytes) noexcept
{
	if (!nevent_is_game_thread() ||
	    !native_quest_preparation_capacity(actual_driver_bytes, false))
		return false;
	native_quest_gameplay_retained_bytes = actual_driver_bytes;
	return true;
}

// Original held-retirement native publication and passive restore owner.

class item_held_retirement_publication_owner final
{
	struct publication_attempt
	{
		pending_movement *entry = nullptr;
		held_retirement_publication_snapshot snapshot;
	};
	static bool same_items(const std::vector<player_item_snapshot> &left,
			       const std::vector<player_item_snapshot> &right)
	{
		std::vector<uint8_t> a, b;
		return player_item_snapshot_list_encode(left, &a) ==
			       player_snapshot_codec_result::ok &&
		       player_item_snapshot_list_encode(right, &b) ==
			       player_snapshot_codec_result::ok &&
		       a == b;
	}
	static P_char actor_for(uint32_t pid) noexcept
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
	static bool native_body(P_char actor, std::vector<player_item_snapshot> *items)
	{
		player_snapshot captured;
		if (!items || player_snapshot_capture_literal_inventory(
				      actor, 1,
				      PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_EQUIPMENT |
					      PLAYER_COMPONENT_INVENTORY,
				      RENT_CRASH, NOWHERE, 0,
				      &captured) != player_snapshot_capture_result::ok)
			return false;
		*items = std::move(captured.items);
		return true;
	}
	static bool checkpoint(critical_native_recovery_envelope *original,
			       const held_retirement_recovery &context)
	{
		if (!original || original->revision == UINT64_MAX)
			return false;
		critical_native_recovery_envelope successor = *original;
		++successor.revision;
		if (!held_retirement_recovery_encode(original->command, context,
						     &successor.attachment) ||
		    !player_save_held_retirement_publication_owner::checkpoint_recovery_context(
			    *original, successor))
			return false;
		*original = std::move(successor);
		return true;
	}
	static bool native_publish(const critical_command &command,
				   const critical_completion &completion, void *opaque) noexcept
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
				attempt ? attempt->snapshot.envelope :
					  critical_native_recovery_envelope{};
			held_retirement_recovery context;
			if (!entry || !entry->held_command ||
			    !held_retirement_command_identity(command, &payload, &terms) ||
			    !held_retirement_publication_snapshot_valid(command, completion,
									attempt->snapshot) ||
			    !held_retirement_recovery_decode(command, envelope.attachment,
							     &context) ||
			    (context.receipt.present && !held_retirement_recovery_receipt_matches(
								context.receipt, completion)))
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
								  completion.result_size,
								  &result) ||
			     result.root_item_uid != terms.item_uid || result.item_count != 1 ||
			     !result.max_item_revision || result.max_item_revision == UINT64_MAX))
				return false;
			result.operation_id = command.operation_id;
			held_retirement_current_observation current;
			// No runtime cache or absent physical UID can substitute for the real
			// repository's exact original receipt/current-cut/rollback observation.
			if (!held_retirement_repository_observe_current(command, completion,
									context, &current) ||
			    !current.rollback_confirmed ||
			    current.save_revision != context.save_revision ||
			    !same_items(current.items,
					committed ? context.after : context.before) ||
			    current.selected.item_uid != terms.item_uid ||
			    current.selected.root_item_uid != terms.item_uid ||
			    current.selected.parent_item_uid ||
			    current.selected.vnum != terms.item_vnum)
				return false;
			if (committed)
			{
				if (current.selected.state != item_custody_state::destroyed ||
				    current.selected.owner.type != item_owner_type::destruction ||
				    current.selected.owner.id ||
				    current.selected.owner.context_id ||
				    current.selected.item_revision != result.max_item_revision ||
				    current.from_owner_revision != result.from_owner_revision ||
				    current.to_owner_revision < result.to_owner_revision)
					return false;
			}
			else if (current.selected.state != item_custody_state::active ||
				 !item_owner_identity_equal(current.selected.owner,
							    payload.from_owner) ||
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
			if (!player_save_held_retirement_publication_owner::
				    rebind_recovery_checkpoint(envelope, actor->runtime_id))
				return false;
			entry->actor_runtime_id = actor->runtime_id;
			if (never_admitted)
				return native_before && !context.receipt.present &&
				       !context.physical_stage;
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
				if (!item_ownership_runtime_hydrate_owner(
					    payload.from_owner, current.from_owner_revision) ||
				    !item_ownership_runtime_hydrate_owner(
					    payload.to_owner, current.to_owner_revision) ||
				    !item_ownership_runtime_hydrate_many_atomic(&current.selected,
										1) ||
				    !lockpick_retirement_publish_physical(command.operation_id,
									  actor, committed, result,
									  terms, notify))
					return false;
				std::vector<player_item_snapshot> after_native;
				if (!native_body(actor, &after_native) ||
				    !same_items(after_native,
						committed ? context.after : context.before))
					return false;
				context.physical_stage = 2;
				if (!checkpoint(&envelope, context))
					return false;
			}
			// Reobserve current SQL after physical return and immediately before ACK.
			held_retirement_current_observation final_cut;
			if (!held_retirement_repository_observe_current(command, completion,
									context, &final_cut) ||
			    !final_cut.rollback_confirmed ||
			    final_cut.save_revision != current.save_revision ||
			    !same_items(final_cut.items, current.items) ||
			    final_cut.selected.item_uid != current.selected.item_uid ||
			    final_cut.selected.root_item_uid != current.selected.root_item_uid ||
			    final_cut.selected.parent_item_uid !=
				    current.selected.parent_item_uid ||
			    final_cut.selected.vnum != current.selected.vnum ||
			    final_cut.selected.state != current.selected.state ||
			    !item_owner_identity_equal(final_cut.selected.owner,
						       current.selected.owner) ||
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

    public:
	static bool publish(pending_movement &entry) noexcept
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
			if (!held_retirement_command_identity(*entry.held_command, nullptr,
							      &terms) ||
			    !held_retirement_recovery_decode(*entry.held_command,
							     envelope.attachment, &context) ||
			    (context.receipt.present && !held_retirement_recovery_receipt_matches(
								context.receipt, entry.completed)))
				return false;
			held_retirement_current_observation current;
			const bool committed =
				entry.completed.disposition ==
					critical_completion_disposition::execution &&
				(entry.completed.outcome == critical_apply_outcome::applied ||
				 entry.completed.outcome ==
					 critical_apply_outcome::already_applied);
			if (!held_retirement_repository_observe_current(
				    *entry.held_command, entry.completed, context, &current) ||
			    !current.rollback_confirmed ||
			    current.save_revision != context.save_revision ||
			    !same_items(current.items, committed ? context.after : context.before))
				return false;
			P_char actor = actor_for(terms.actor_pid);
			std::vector<player_item_snapshot> native;
			if (!actor || !native_body(actor, &native) ||
			    (committed ? (!same_items(native, context.before) &&
					  !same_items(native, context.after)) :
					 !same_items(native, context.before)) ||
			    !player_save_held_retirement_publication_owner::
				    rebind_recovery_checkpoint(envelope, actor->runtime_id))
				return false;
			entry.actor_runtime_id = actor->runtime_id;
		}
		catch (...)
		{
			return false;
		}
#endif
		return player_save_held_retirement_publication_owner::publish_held_retirement(
			*entry.held_command, entry.completed, attempt.snapshot, native_publish,
			&attempt);
	}
	static bool restore(const critical_native_recovery_envelope &envelope) noexcept
	{
		try
		{
			lockpick_retirement_terms terms;
			item_transfer_payload payload;
			held_retirement_recovery context;
			if (!envelope.revision ||
			    envelope.phase != critical_native_recovery_phase::execution_pending ||
			    !held_retirement_command_identity(envelope.command, &payload, &terms) ||
			    !held_retirement_recovery_decode(envelope.command, envelope.attachment,
							     &context))
				return false;
			const auto key = operation_key(envelope.command.operation_id);
			auto existing = pending.find(key);
			if (existing != pending.end())
			{
				std::vector<uint8_t> before, after;
				if (!existing->second.held_command ||
				    critical_command_encode(*existing->second.held_command,
							    &before) !=
					    critical_command_codec_result::ok ||
				    critical_command_encode(envelope.command, &after) !=
					    critical_command_codec_result::ok ||
				    before != after)
					return false;
				return player_save_held_retirement_publication_owner::
					restore_recovery_checkpoint(envelope);
			}
			if (pending.size() + native_quest_pending_count() >=
			    ITEM_MOVEMENT_PENDING_MAX)
				return false;
			pending_movement entry{};
			entry.actor_pid = terms.actor_pid;
			entry.payload = payload;
			entry.requested_to_owner = payload.to_owner;
			entry.requested_reason = payload.reason;
			entry.requested_reason_id = payload.reason_id;
			entry.publication = lockpick_retirement_publication;
			entry.context_size = payload.continuation.data.size();
			std::copy(payload.continuation.data.begin(),
				  payload.continuation.data.end(), entry.context.begin());
			entry.publication_status = publication_state::ready;
			entry.recovered_publication = true;
			entry.held_command =
				std::make_shared<const critical_command>(envelope.command);
			// Allocate/map admission before the original guard is installed.
			const auto inserted = pending.emplace(key, std::move(entry));
			if (!inserted.second)
				return false;
			if (!player_save_held_retirement_publication_owner::
				    restore_recovery_checkpoint(envelope))
			{
				pending.erase(inserted.first);
				return false;
			}
			account_health();
			return true;
		}
		catch (...)
		{
			return false;
		}
	}
};

namespace
{
void publish_held_retirement_entry(std::unordered_map<std::string, pending_movement>::iterator found)
{
	auto &entry = found->second;
	const auto original = entry.held_command;
	const auto terms_bytes = entry.context;
	const auto terms_size = entry.context_size;
	entry.publication_inflight = true;
	const bool acknowledged = item_held_retirement_publication_owner::publish(entry);
	entry.publication_inflight = false;
	if (!acknowledged)
	{
		entry.publication_status = publication_state::owner_waiting;
		return;
	}
	const auto completion = entry.completed;
	const auto runtime_id = entry.actor_runtime_id;
	pending.erase(found);
	const bool committed = completion.outcome == critical_apply_outcome::applied ||
			       completion.outcome == critical_apply_outcome::already_applied;
	if (committed)
		++health.committed;
	else
		++health.rejected;
	// Proven no-admission cleanup is complete; notification carries no ACK authority.
	if (completion.disposition == critical_completion_disposition::never_admitted)
	{
		P_char actor = find_character_by_runtime_id(runtime_id);
		(void)lockpick_retirement_publication(original->operation_id, actor, false, {},
						      completion.error_code, terms_bytes.data(),
						      terms_size);
	}
}
}
bool item_movement_transaction_restore_held_retirement_recovery(
	const critical_native_recovery_envelope &envelope) noexcept
{
	return item_held_retirement_publication_owner::restore(envelope);
}

// Scalar-only guard transitions. Actual ROOT clears scratch before ending,
// then its existing charge(0) observes persistent globals outside the scope.
bool item_native_quest_global_budget_scope_owner::begin(const void *actual_guard,
							bool (*current_storage)(size_t *) noexcept,
							bool includes_literal_pool) noexcept
{
	if (!actual_guard || !current_storage || !nevent_is_game_thread() ||
	    native_quest_flat_global_scope ||
	    (native_quest_flat_global_observer &&
	     (native_quest_flat_global_observer != current_storage ||
	      native_quest_flat_literal_pool_owned != includes_literal_pool)))
		return false;
	native_quest_flat_global_observer = current_storage;
	native_quest_flat_literal_pool_owned = includes_literal_pool;
	native_quest_flat_global_scope = actual_guard;
	return true;
}
bool item_native_quest_global_budget_scope_owner::end(const void *actual_guard) noexcept
{
	if (!actual_guard || !nevent_is_game_thread() ||
	    native_quest_flat_global_scope != actual_guard)
		return false;
	native_quest_flat_global_scope = nullptr;
	return true;
}

// Pure ownership observation. No observer callback or native capability follows.
bool item_native_quest_global_budget_scope_owner::literal_pool_owned() noexcept
{
	return nevent_is_game_thread() && native_quest_flat_global_observer &&
	       native_quest_flat_literal_pool_owned;
}

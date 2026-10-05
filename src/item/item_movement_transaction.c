#include "item/item_movement_transaction.h"
#include "item/craft_pouch_mutation.h"
#include "player/craft_progression_hooks.h"
#include "combat/chaos_pouch_publication.h"

#include "item/item_ownership_runtime.h"
#include "item/item_actions.h"
#include "item/ordinary_drop_recovery.h"
#include "economy/economic_gameplay_authority.h"
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
#include "core/prototypes.h"
#include "core/utils.h"
#include "account/account_reward.h"
#include "magic/spell_item_lifecycle.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <deque>
#include <new>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

extern P_obj object_list;
extern P_char character_list;
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

bool movement_conflicts(const item_owner_identity &from_owner, const item_owner_identity &to_owner)
{
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

bool craft_live_ready(P_char actor, const pending_movement &entry)
{
	if (!actor)
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
	health.pending = pending.size();
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

	if (recipe && !never_admitted && hooks.acknowledged)
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
			const auto progression = craft_progression_hooks.publish(
				entry.completed.operation_id, actor, terms);
			if (progression != craft_progression_publication_result::ready)
			{
				if (progression == craft_progression_publication_result::waiting)
					entry.publication_status = publication_state::owner_waiting;
				else
					retain_publication_failure(entry, "recipe progression");
				account_health();
				return;
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
	if (pending.size() >= ITEM_MOVEMENT_PENDING_MAX)
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
		.native_mobile = {}
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
	if (pending.size() >= ITEM_MOVEMENT_PENDING_MAX)
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
		.native_mobile = {}
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
		       (craft_recipe_is_alchemy(recipe->discipline) ?
				recipe->output_count != output_count :
				output_count != 1 || !outputs[0] ||
					recipe->output_uid != outputs[0]->obj_uid) ||
		       recipe->player_pid != static_cast<uint32_t>(GET_PID(actor)) ||
		       recipe->recipe_vnum != recipe_id || !recipe->pouch_mutation.empty()))
		return reject_with(reject, item_movement_reject::invalid_request);
	if (pending.size() >= ITEM_MOVEMENT_PENDING_MAX)
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
			if (craft_recipe_is_alchemy(terms.discipline))
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
					  .native_mobile = {} };
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
		if (found->second.restored_sql_drop || found->second.live_drop_command)
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
		if (found->second.restored_sql_drop || found->second.live_drop_command)
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
	if (pending.find(key) != pending.end() || pending.size() >= ITEM_MOVEMENT_PENDING_MAX)
		return false;
	item_transfer_payload payload = {};
	if (!item_transfer_command_decode_payload(command, &payload) ||
	    payload.from_owner.type != item_owner_type::player || !payload.from_owner.id ||
	    payload.from_owner.id > UINT32_MAX)
		return false;
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
	pending.clear();
	creation_grants.clear();
	preparation_order.clear();
	health = {};
}

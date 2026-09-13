#include "item/item_actions.h"

#include "core/prototypes.h"
#include "core/utils.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <time.h>
#include <vector>

extern unsigned long long ne_event_tick;

namespace
{
struct action_config
{
	bool enabled = false;
	int reaction_pulses = 4;
	int max_pulses = 120;
	int per_wielder = 2;
	int total = 4096;
	bool operator==(const action_config &) const = default;
};

struct ability
{
	const item_action_definition definition;
	const std::shared_ptr<const item_action_adapter> adapter;
	bool enabled = true;
};

struct pending_item_action
{
	const std::shared_ptr<ability> selected;
	const item_action_identity identity;
	P_obj const expected_source;
	nevent_handle event = {};
	uint64_t payload_generation = 0;
	uint64_t deadline_us = 0;
	item_action_consumption consumption = item_action_consumption::rejected;
	bool terminal = false;
	bool resolving = false;
	bool effect_started = false;

	pending_item_action(std::shared_ptr<ability> selected_ability,
			    item_action_identity selected_identity, P_obj source)
		: selected(std::move(selected_ability))
		, identity(selected_identity)
		, expected_source(source)
	{
	}
};

action_config config;
uint64_t next_action_id = 0;
std::map<uint32_t, std::shared_ptr<ability>> abilities;
std::map<uint64_t, std::shared_ptr<pending_item_action>> pending;
std::map<uint64_t, uint64_t> active_actors;

uint64_t monotonic_us()
{
	timespec now = {};
	if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
		return 0;
	return static_cast<uint64_t>(now.tv_sec) * 1000000ULL + now.tv_nsec / 1000;
}

void finish_action(const std::shared_ptr<pending_item_action> &entry, item_action_outcome outcome)
{
	if (entry->terminal)
		return;
	entry->terminal = true;
	pending.erase(entry->identity.action_id);
	if (entry->selected->definition.mode == item_action_mode::active)
		active_actors.erase(entry->identity.actor_id);
	if (entry->effect_started && outcome == item_action_outcome::interrupted)
		outcome = item_action_outcome::partially_resolved;
	if (entry->consumption != item_action_consumption::rejected)
		entry->selected->adapter->finish(entry->identity, entry->consumption, outcome);
}

struct action_payload
{
	std::shared_ptr<pending_item_action> entry;
	uint64_t generation;
	~action_payload()
	{
		// Rearming for the real-time reaction floor transfers ownership to the
		// next payload. Ordinary cancellation/rejection destroys the current one.
		if (entry && generation == entry->payload_generation)
			finish_action(entry, item_action_outcome::interrupted);
	}

	action_payload(std::shared_ptr<pending_item_action> value, uint64_t token)
		: entry(std::move(value))
		, generation(token)
	{
	}
	action_payload(action_payload &&) = default;
};

void cancel_action(const std::shared_ptr<pending_item_action> &entry)
{
	if (entry->terminal)
		return;
	const nevent_handle handle = entry->event;
	finish_action(entry, item_action_outcome::interrupted);
	if (handle.event)
		nevent_cancel(handle);
}

template <typename Predicate> void cancel_matching(Predicate predicate)
{
	std::vector<std::shared_ptr<pending_item_action>> selected;
	for (const auto &[id, entry] : pending)
		if (predicate(*entry))
			selected.push_back(entry);
	for (const auto &entry : selected)
		cancel_action(entry);
}

// Never dereference the saved object pointer until it is found on the live actor.
// Extraction also cancels synchronously, including reuse of the same item UID.
P_obj live_source(const pending_item_action &entry, P_char actor)
{
	if (entry.selected->definition.source == item_action_source::equipped)
	{
		const int slot = entry.identity.source_slot;
		P_obj source = slot >= 0 && slot < MAX_WEAR ? actor->equipment[slot] : nullptr;
		return source && source == entry.expected_source &&
				       source->obj_uid == entry.identity.source_uid ?
			       source :
			       nullptr;
	}
	for (P_obj source = actor->carrying; source; source = source->next_content)
		if (source == entry.expected_source && source->obj_uid == entry.identity.source_uid)
			return source;
	return nullptr;
}

bool live_context(const pending_item_action &entry, P_char &actor, P_char &target, P_obj &source)
{
	if (entry.terminal || !config.enabled || !entry.selected->enabled)
		return false;
	actor = find_character_by_runtime_id(entry.identity.actor_id);
	target = find_character_by_runtime_id(entry.identity.target_id);
	if (!IS_ALIVE(actor) || !IS_ALIVE(target) || actor->in_room != entry.identity.origin_room ||
	    target->in_room != actor->in_room)
		return false;
	source = live_source(entry, actor);
	if (!source)
		return false;
	if (entry.selected->definition.source == item_action_source::equipped)
	{
		if (!OBJ_WORN_BY(source, actor) || entry.identity.source_slot < 0 ||
		    actor->equipment[entry.identity.source_slot] != source)
			return false;
	}
	else if (!OBJ_CARRIED_BY(source, actor))
		return false;
	if (entry.selected->definition.mode == item_action_mode::active &&
	    IS_AFFECTED2(actor, AFF2_CASTING))
		return false;
	return entry.selected->adapter->validate(
		{ entry.identity, entry.selected->definition, actor, target, source });
}

bool schedule_action(const std::shared_ptr<pending_item_action> &entry, P_char actor, P_char target,
		     P_obj source, int delay)
{
	const auto callback = entry->selected->definition.mode == item_action_mode::active ?
				      event_item_action_active :
				      event_item_action_passive;
	const auto scheduled = add_event_owned(callback, delay, actor, target, source, 0,
					       action_payload(entry, ++entry->payload_generation));
	if (!scheduled)
		return false;
	entry->event = scheduled.handle;
	return true;
}

void progress_action(void *data)
{
	auto *payload = static_cast<action_payload *>(data);
	if (!payload || !payload->entry)
		return;
	const auto entry = payload->entry; // Survives extraction/cancellation inside an effect.
	if (entry->terminal || entry->resolving || payload->generation != entry->payload_generation)
		return;
	P_char actor = nullptr, target = nullptr;
	P_obj source = nullptr;
	if (!live_context(*entry, actor, target, source))
	{
		cancel_action(entry);
		return;
	}
	const uint64_t now = monotonic_us();
	if (!now)
	{
		cancel_action(entry);
		return;
	}
	if (now < entry->deadline_us)
	{
		// Scheduler catch-up can advance game ticks without giving the player
		// real reaction time. Keep a monotonic floor as well as the tick deadline.
		const uint64_t remaining = entry->deadline_us - now;
		const int delay = static_cast<int>(std::min<uint64_t>(
			config.max_pulses, (remaining + OPT_USEC - 1) / OPT_USEC));
		schedule_action(entry, actor, target, source, std::max(1, delay));
		return;
	}
	entry->resolving = true;
	for (size_t effect = 0; effect < entry->selected->definition.effect_count; ++effect)
	{
		// Even one effect may kill, move, extract, disable or reload. Reacquire
		// every participant, and never substitute the actor's new opponent.
		if (!live_context(*entry, actor, target, source))
		{
			cancel_action(entry);
			return;
		}
		entry->effect_started = true;
		entry->selected->adapter->resolve({ entry->identity, entry->selected->definition,
						    actor, target, source },
						  entry->selected->definition.effects[effect]);
	}
	finish_action(entry, item_action_outcome::completed);
}

bool integer_property(const char *key, int fallback, int minimum, int maximum, int &value)
{
	const float raw = get_property(key, static_cast<double>(fallback), false);
	if (!std::isfinite(raw) || raw < minimum || raw > maximum || std::floor(raw) != raw)
	{
		logit(LOG_STATUS, "Invalid %s; item actions disabled (expected integer %d..%d)",
		      key, minimum, maximum);
		return false;
	}
	value = static_cast<int>(raw);
	return true;
}
} // namespace

void update_item_action_properties()
{
	if (!nevent_require_game_thread("update_item_action_properties"))
		return;
	action_config updated;
	int enabled = 0;
	bool valid = integer_property("itemActions.enabled", 0, 0, 1, enabled);
	valid &= integer_property("itemActions.reactionPulses", 4, 1, 600, updated.reaction_pulses);
	valid &= integer_property("itemActions.maxPulses", 120, 1, 600, updated.max_pulses);
	valid &= integer_property("itemActions.maxPerWielder", 2, 1, 8, updated.per_wielder);
	valid &= integer_property("itemActions.maxPending", 4096, 1, 4096, updated.total);
	updated.enabled = valid && enabled == 1 && updated.reaction_pulses <= updated.max_pulses;
	if (!(config == updated))
	{
		config = updated;
		cancel_matching([](const pending_item_action &) { return true; });
	}
}

bool item_actions_publish(const item_action_definition &definition,
			  std::unique_ptr<item_action_adapter> adapter)
{
	if (!nevent_require_game_thread("item_actions_publish") || !adapter || !definition.id ||
	    !definition.revision || !definition.effect_count ||
	    definition.effect_count > ITEM_ACTION_MAX_EFFECTS || definition.windup_pulses < 0 ||
	    definition.windup_pulses > 600 ||
	    (definition.mode != item_action_mode::passive &&
	     definition.mode != item_action_mode::active) ||
	    (definition.source != item_action_source::equipped &&
	     definition.source != item_action_source::carried) ||
	    (definition.mode == item_action_mode::passive &&
	     definition.source != item_action_source::equipped))
		return false;
	for (size_t i = 0; i < definition.effect_count; ++i)
	{
		const auto &effect = definition.effects[i];
		if (!effect.id || effect.power < 0 ||
		    (effect.call != item_action_call::weapon &&
		     effect.call != item_action_call::wand &&
		     effect.call != item_action_call::staff &&
		     effect.call != item_action_call::scroll))
			return false;
	}
	const auto previous = abilities.find(definition.id);
	if (previous != abilities.end() &&
	    previous->second->definition.revision >= definition.revision)
		return false;
	// Allocate before invalidating a working revision.
	auto replacement =
		std::make_shared<ability>(ability{ definition, std::move(adapter), true });
	item_actions_disable(definition.id);
	abilities[definition.id] = std::move(replacement);
	return true;
}

void item_actions_disable(uint32_t ability_id)
{
	if (!nevent_require_game_thread("item_actions_disable"))
		return;
	const auto found = abilities.find(ability_id);
	if (found != abilities.end())
		found->second->enabled = false;
	cancel_matching([ability_id](const pending_item_action &entry)
			{ return entry.identity.ability_id == ability_id; });
}

void item_actions_reload()
{
	if (!nevent_require_game_thread("item_actions_reload"))
		return;
	cancel_matching([](const pending_item_action &) { return true; });
	abilities.clear();
}

item_action_start start_item_action(uint32_t ability_id, P_char actor, P_char target, P_obj source)
{
	if (!nevent_require_game_thread("start_item_action"))
		return item_action_start::suppressed;
	const auto found = abilities.find(ability_id);
	if (!config.enabled || found == abilities.end() || !found->second->enabled)
		return item_action_start::legacy;
	const auto selected = found->second;
	if (!IS_ALIVE(actor) || !IS_ALIVE(target) || !source || !source->obj_uid ||
	    !actor->runtime_id || !target->runtime_id || actor->in_room == NOWHERE ||
	    pending.size() >= static_cast<size_t>(config.total) ||
	    next_action_id == std::numeric_limits<uint64_t>::max())
		return item_action_start::suppressed;
	int wielder_count = 0;
	for (const auto &[id, entry] : pending)
	{
		if (entry->identity.source_uid == source->obj_uid)
			return item_action_start::suppressed; // One pending action per physical item.
		if (entry->identity.actor_id == actor->runtime_id)
			++wielder_count;
	}
	if (wielder_count >= config.per_wielder ||
	    (selected->definition.mode == item_action_mode::active &&
	     (item_action_active(actor) || !CAN_ACT(actor) || IS_AFFECTED2(actor, AFF2_CASTING))))
		return item_action_start::suppressed;
	int slot = -1;
	if (selected->definition.source == item_action_source::equipped)
		for (int i = 0; i < MAX_WEAR; ++i)
			if (actor->equipment[i] == source)
			{
				slot = i;
				break;
			}
	const int delay = std::clamp(selected->definition.windup_pulses, config.reaction_pulses,
				     config.max_pulses);
	if (ne_event_tick > std::numeric_limits<unsigned long long>::max() - delay)
		return item_action_start::suppressed;
	item_action_identity identity{ ++next_action_id,     source->obj_uid,
				       actor->runtime_id,    target->runtime_id,
				       ability_id,	     selected->definition.revision,
				       actor->in_room,	     slot,
				       ne_event_tick + delay };
	auto entry = std::make_shared<pending_item_action>(selected, identity, source);
	P_char live_actor = nullptr, live_target = nullptr;
	P_obj live_object = nullptr;
	if (!live_context(*entry, live_actor, live_target, live_object))
		return item_action_start::suppressed;
	pending.emplace(identity.action_id, entry);
	if (selected->definition.mode == item_action_mode::active)
		active_actors.emplace(identity.actor_id, identity.action_id);
	if (!schedule_action(entry, actor, target, source, delay))
		return item_action_start::suppressed;
	const item_action_context context{ identity, selected->definition, actor, target, source };
	entry->consumption = selected->adapter->commit(context);
	if (entry->consumption == item_action_consumption::rejected)
	{
		cancel_action(entry);
		return item_action_start::suppressed;
	}
	selected->adapter->announce(context);
	if (!entry->terminal)
	{
		const uint64_t now = monotonic_us();
		if (!now)
			cancel_action(entry);
		else
			entry->deadline_us = now + static_cast<uint64_t>(delay) * OPT_USEC;
	}
	return item_action_start::scheduled;
}

bool item_action_active(P_char actor)
{
	return actor && active_actors.contains(actor->runtime_id);
}

bool abort_item_action(P_char actor)
{
	if (!nevent_require_game_thread("abort_item_action") || !item_action_active(actor))
		return false;
	const uint64_t actor_id = actor->runtime_id;
	cancel_matching(
		[actor_id](const pending_item_action &entry)
		{
			return entry.identity.actor_id == actor_id &&
			       entry.selected->definition.mode == item_action_mode::active;
		});
	return true;
}

size_t item_actions_pending()
{
	return pending.size();
}

void item_actions_character_leaving(P_char character)
{
	if (!character || pending.empty() ||
	    !nevent_require_game_thread("item_actions_character_leaving"))
		return;
	const uint64_t id = character->runtime_id;
	cancel_matching(
		[id](const pending_item_action &entry)
		{ return entry.identity.actor_id == id || entry.identity.target_id == id; });
}

void item_actions_source_leaving(P_obj source)
{
	if (!source || pending.empty() ||
	    !nevent_require_game_thread("item_actions_source_leaving"))
		return;
	const uint64_t uid = source->obj_uid;
	cancel_matching([uid](const pending_item_action &entry)
			{ return entry.identity.source_uid == uid; });
}

void event_item_action_active(P_char, P_char, P_obj, void *data)
{
	progress_action(data);
}
void event_item_action_passive(P_char, P_char, P_obj, void *data)
{
	progress_action(data);
}

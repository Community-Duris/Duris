#include "item/item_actions.h"

#include "core/prototypes.h"
#include "core/utils.h"
#include "item/objmisc.h"
#include "persistence/latency_trace.h"

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
	bool mana_enabled = false;
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
	const item_action_definition definition;
	P_obj const expected_source;
	nevent_handle event = {};
	uint64_t payload_generation = 0;
	uint64_t deadline_us = 0;
	uint64_t progress_us = 0;
	item_action_consumption consumption = item_action_consumption::rejected;
	bool terminal = false;
	bool resolving = false;
	bool effect_started = false;
	uint8_t effects_invoked = 0;
	bool progress_emitted = false;
	item_action_cancel_reason cleanup_reason = item_action_cancel_reason::runtime_cleanup;

	pending_item_action(std::shared_ptr<ability> selected_ability,
			    item_action_identity selected_identity, P_obj source,
			    item_action_definition invocation)
		: selected(std::move(selected_ability))
		, identity(selected_identity)
		, definition(std::move(invocation))
		, expected_source(source)
	{
	}
};

action_config config;
uint64_t next_action_id = 0;
std::map<uint32_t, std::shared_ptr<ability>> abilities;
std::map<uint64_t, std::shared_ptr<pending_item_action>> pending;
std::map<uint64_t, uint64_t> active_actors;
item_action_telemetry telemetry;
uint64_t telemetry_generation = 0;

void increment(uint64_t &counter, uint64_t amount = 1)
{
	counter += std::min(amount, std::numeric_limits<uint64_t>::max() - counter);
}

uint64_t monotonic_us()
{
	timespec now = {};
	if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
		return 0;
	return static_cast<uint64_t>(now.tv_sec) * 1000000ULL + now.tv_nsec / 1000;
}

void finish_action(const std::shared_ptr<pending_item_action> &entry, item_action_outcome outcome,
		   item_action_cancel_reason reason = item_action_cancel_reason::runtime_cleanup)
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
	{
		// A native final effect can kill/extract its target and synchronously
		// trigger cleanup. Every captured call was still invoked in that case.
		if (outcome == item_action_outcome::completed ||
		    entry->effects_invoked == entry->definition.effect_count)
			item_actions_note(item_action_metric::completed);
		else
		{
			if (telemetry.enabled)
				increment(telemetry.cancelled[static_cast<size_t>(reason)]);
			if (outcome == item_action_outcome::partially_resolved)
			{
				item_actions_note(item_action_metric::partial);
				item_actions_note(item_action_metric::effect_failures);
			}
		}
		entry->selected->adapter->finish(entry->identity, entry->consumption, outcome);
	}
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
			finish_action(entry, item_action_outcome::interrupted,
				      entry->cleanup_reason);
	}

	action_payload(std::shared_ptr<pending_item_action> value, uint64_t token)
		: entry(std::move(value))
		, generation(token)
	{
	}
	action_payload(action_payload &&) = default;
};

void cancel_action(const std::shared_ptr<pending_item_action> &entry,
		   item_action_cancel_reason reason = item_action_cancel_reason::runtime_cleanup)
{
	if (entry->terminal)
		return;
	const nevent_handle handle = entry->event;
	finish_action(entry, item_action_outcome::interrupted, reason);
	if (handle.event)
		nevent_cancel(handle);
}

template <typename Predicate>
void cancel_matching(Predicate predicate,
		     item_action_cancel_reason reason = item_action_cancel_reason::runtime_cleanup)
{
	std::vector<std::shared_ptr<pending_item_action>> selected;
	for (const auto &[id, entry] : pending)
		if (predicate(*entry))
			selected.push_back(entry);
	for (const auto &entry : selected)
		cancel_action(entry, reason);
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
	if (!source || item_restricted_for_player_pet(actor, source))
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
		{ entry.identity, entry.definition, actor, target, source });
}

bool schedule_action(const std::shared_ptr<pending_item_action> &entry, P_char actor, P_char target,
		     P_obj source, int delay)
{
	const auto callback = entry->selected->definition.mode == item_action_mode::active ?
				      event_item_action_active :
				      event_item_action_passive;
	entry->cleanup_reason = item_action_cancel_reason::scheduling_rejected;
	const auto scheduled = add_event_owned(callback, delay, actor, target, source, 0,
					       action_payload(entry, ++entry->payload_generation));
	if (!scheduled)
	{
		item_actions_note(item_action_metric::scheduling_rejected);
		return false;
	}
	entry->event = scheduled.handle;
	entry->cleanup_reason = item_action_cancel_reason::runtime_cleanup;
	return true;
}

struct action_callback_timer
{
	const bool enabled = telemetry.enabled;
	const uint64_t generation = telemetry_generation;
	const uint64_t start = enabled ? latency_trace_monotonic_us() : 0;
	~action_callback_timer()
	{
		if (!enabled || !telemetry.enabled || generation != telemetry_generation)
			return;
		const uint64_t elapsed =
			latency_trace_elapsed_us(start, latency_trace_monotonic_us());
		if (elapsed == LATENCY_TRACE_DURATION_INVALID)
		{
			increment(telemetry.invalid_clock);
			return;
		}
		increment(telemetry.callbacks);
		increment(telemetry.callback_total_us, elapsed);
		telemetry.callback_max_us = std::max(telemetry.callback_max_us, elapsed);
		latency_trace_record_nonblocking("item_action.callback", elapsed, ne_event_tick);
	}
};

void progress_action(void *data)
{
	action_callback_timer timer;
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
		cancel_action(entry, item_action_cancel_reason::invalid_context);
		return;
	}
	const uint64_t now = monotonic_us();
	if (!now)
	{
		cancel_action(entry, item_action_cancel_reason::clock_failure);
		return;
	}
	if (now < entry->deadline_us)
	{
		if (entry->progress_us && !entry->progress_emitted && now >= entry->progress_us)
		{
			entry->progress_emitted = true;
			entry->selected->adapter->progress(
				{ entry->identity, entry->definition, actor, target, source });
			if (entry->terminal)
				return;
		}
		// Scheduler catch-up can advance game ticks without giving the player
		// real reaction time. Keep a monotonic floor as well as the tick deadline.
		const uint64_t next = entry->progress_us && !entry->progress_emitted ?
					      std::min(entry->deadline_us, entry->progress_us) :
					      entry->deadline_us;
		const uint64_t remaining = next - now;
		const int delay = static_cast<int>(std::min<uint64_t>(
			config.max_pulses, (remaining + OPT_USEC - 1) / OPT_USEC));
		schedule_action(entry, actor, target, source, std::max(1, delay));
		return;
	}
	entry->resolving = true;
	for (size_t effect = 0; effect < entry->definition.effect_count; ++effect)
	{
		// Even one effect may kill, move, extract, disable or reload. Reacquire
		// every participant, and never substitute the actor's new opponent.
		if (!live_context(*entry, actor, target, source))
		{
			cancel_action(entry, item_action_cancel_reason::invalid_context);
			return;
		}
		entry->effect_started = true;
		++entry->effects_invoked;
		item_actions_note(item_action_metric::effects_invoked);
		entry->selected->adapter->resolve({ entry->identity, entry->definition, actor,
						    target, source },
						  entry->definition.effects[effect]);
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
	const float observed = get_property("itemActions.telemetry.enabled", 0.0, false);
	const bool observe = observed == 1;
	if (observe != telemetry.enabled)
	{
		telemetry = {};
		telemetry.enabled = observe;
		telemetry.peak_pending = pending.size();
		++telemetry_generation;
	}
	action_config updated;
	// A resource-gate transition is a barrier for already-paid work too.
	// Keep the ledger intact; cancellation follows the adapter's no-refund policy.
	updated.mana_enabled = get_property("itemActions.mana.enabled", 0.0, false) == 1;
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
		cancel_matching([](const pending_item_action &) { return true; },
				item_action_cancel_reason::configuration_change);
	}
}

static bool valid_definition(const item_action_definition &definition)
{
	if (!definition.id || !definition.revision ||
	    (!definition.effect_count && !definition.selected_effects) ||
	    definition.effect_count > ITEM_ACTION_MAX_EFFECTS || definition.windup_pulses < 0 ||
	    definition.windup_pulses > 600 || definition.progress_pulses < 0 ||
	    (definition.progress_pulses &&
	     definition.progress_pulses >= definition.windup_pulses) ||
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
		if (!effect.id || effect.power < 0 || effect.auxiliary < 0 ||
		    (effect.target != item_action_effect_target::original &&
		     effect.target != item_action_effect_target::actor) ||
		    (effect.call != item_action_call::weapon &&
		     effect.call != item_action_call::wand &&
		     effect.call != item_action_call::staff &&
		     effect.call != item_action_call::scroll &&
		     effect.call != item_action_call::spell))
			return false;
	}
	return true;
}

bool item_actions_publish(const item_action_definition &definition,
			  std::unique_ptr<item_action_adapter> adapter)
{
	if (!nevent_require_game_thread("item_actions_publish") || !adapter ||
	    !valid_definition(definition))
		return false;
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
			{ return entry.identity.ability_id == ability_id; },
			item_action_cancel_reason::definition_change);
}

void item_actions_reload()
{
	if (!nevent_require_game_thread("item_actions_reload"))
		return;
	cancel_matching([](const pending_item_action &) { return true; },
			item_action_cancel_reason::reload);
	abilities.clear();
}

bool item_actions_enabled()
{
	return config.enabled;
}

uint64_t item_actions_definition_revision(uint32_t id)
{
	const auto found = abilities.find(id);
	return found != abilities.end() && found->second->enabled ?
		       found->second->definition.revision :
		       0;
}

static item_action_start start_action(uint32_t ability_id, P_char actor, P_char target,
				      P_obj source, const item_action_selection *selection,
				      std::shared_ptr<ability> instance = {},
				      bool immediate = false)
{
	if (!nevent_require_game_thread("start_item_action"))
		return item_action_start::suppressed;
	const auto found = abilities.find(ability_id);
	const auto selected = instance		       ? std::move(instance) :
			      found != abilities.end() ? found->second :
							 nullptr;
	if (!config.enabled || !selected || !selected->enabled)
		return item_action_start::legacy;
	item_actions_note(item_action_metric::selected);
	const auto reject = [](item_action_metric metric = item_action_metric::invalid)
	{
		item_actions_note(metric);
		return item_action_start::suppressed;
	};
	auto invocation = selected->definition;
	if (invocation.selected_effects != (selection != nullptr))
		return reject();
	if (selection)
	{
		if (!selection->effect_count || selection->effect_count > ITEM_ACTION_MAX_EFFECTS)
			return reject();
		invocation.effects = selection->effects;
		invocation.effect_count = selection->effect_count;
		for (size_t i = 0; i < invocation.effect_count; ++i)
		{
			const auto &effect = invocation.effects[i];
			if (!effect.id || effect.power < 0 || effect.auxiliary < 0 ||
			    (effect.target != item_action_effect_target::original &&
			     effect.target != item_action_effect_target::actor) ||
			    (effect.call != item_action_call::weapon &&
			     effect.call != item_action_call::wand &&
			     effect.call != item_action_call::staff &&
			     effect.call != item_action_call::scroll &&
			     effect.call != item_action_call::spell))
				return reject();
		}
	}
	if (!IS_ALIVE(actor) || !IS_ALIVE(target) || !source || !source->obj_uid ||
	    item_restricted_for_player_pet(actor, source) || !actor->runtime_id ||
	    !target->runtime_id || actor->in_room == NOWHERE ||
	    next_action_id == std::numeric_limits<uint64_t>::max())
		return reject();
	if (pending.size() >= static_cast<size_t>(config.total))
		return reject(item_action_metric::busy);
	int wielder_count = 0;
	for (const auto &[id, entry] : pending)
	{
		if (entry->identity.source_uid == source->obj_uid)
			return reject(item_action_metric::busy); // One per physical item.
		if (entry->identity.actor_id == actor->runtime_id)
			++wielder_count;
	}
	if (wielder_count >= config.per_wielder ||
	    (selected->definition.mode == item_action_mode::active &&
	     (item_action_active(actor) || !CAN_ACT(actor) || IS_AFFECTED2(actor, AFF2_CASTING))))
		return reject(item_action_metric::busy);
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
		return reject();
	item_action_identity identity{ ++next_action_id,     source->obj_uid,
				       actor->runtime_id,    target->runtime_id,
				       ability_id,	     selected->definition.revision,
				       actor->in_room,	     slot,
				       ne_event_tick + delay };
	auto entry = std::make_shared<pending_item_action>(selected, identity, source, invocation);
	P_char live_actor = nullptr, live_target = nullptr;
	P_obj live_object = nullptr;
	if (!live_context(*entry, live_actor, live_target, live_object))
		return reject();
	pending.emplace(identity.action_id, entry);
	if (telemetry.enabled)
		telemetry.peak_pending = std::max(telemetry.peak_pending, pending.size());
	if (selected->definition.mode == item_action_mode::active)
		active_actors.emplace(identity.actor_id, identity.action_id);
	const int progress_delay = !invocation.progress_pulses || delay < 2 ? 0 :
				   invocation.progress_pulses >= delay	    ? delay / 2 :
									 invocation.progress_pulses;
	const int first_delay = progress_delay ? progress_delay : delay;
	if (!immediate && !schedule_action(entry, actor, target, source, first_delay))
		return item_action_start::suppressed;
	const item_action_context context{ identity, entry->definition, actor, target, source };
	entry->consumption = selected->adapter->commit(context);
	if (entry->consumption == item_action_consumption::rejected)
	{
		cancel_action(entry);
		return reject(item_action_metric::consumption_rejected);
	}
	item_actions_note(item_action_metric::started);
	selected->adapter->announce(context);
	if (immediate)
	{
		action_callback_timer timer;
		if (!live_context(*entry, live_actor, live_target, live_object))
		{
			cancel_action(entry, item_action_cancel_reason::invalid_context);
			return item_action_start::suppressed;
		}
		entry->effect_started = true;
		++entry->effects_invoked;
		item_actions_note(item_action_metric::effects_invoked);
		selected->adapter->resolve({ identity, entry->definition, live_actor, live_target,
					     live_object },
					   entry->definition.effects[0]);
		finish_action(entry, item_action_outcome::completed);
		return item_action_start::resolved;
	}
	if (!entry->terminal)
	{
		const uint64_t now = monotonic_us();
		if (!now)
			cancel_action(entry, item_action_cancel_reason::clock_failure);
		else
		{
			entry->deadline_us = now + static_cast<uint64_t>(delay) * OPT_USEC;
			if (progress_delay)
				entry->progress_us =
					now + static_cast<uint64_t>(progress_delay) * OPT_USEC;
		}
	}
	return item_action_start::scheduled;
}

item_action_start start_item_action(uint32_t ability_id, P_char actor, P_char target, P_obj source)
{
	return start_action(ability_id, actor, target, source, nullptr);
}

item_action_start start_selected_item_action(uint32_t ability_id, P_char actor, P_char target,
					     P_obj source, const item_action_selection &selection)
{
	return start_action(ability_id, actor, target, source, &selection);
}

item_action_start start_item_action_instance(const item_action_definition &definition,
					     std::unique_ptr<item_action_adapter> adapter,
					     P_char actor, P_char target, P_obj source)
{
	if (!nevent_require_game_thread("start_item_action_instance") || !adapter ||
	    !valid_definition(definition) || definition.selected_effects)
		return item_action_start::suppressed;
	auto instance = std::make_shared<ability>(ability{ definition, std::move(adapter), true });
	return start_action(definition.id, actor, target, source, nullptr, std::move(instance));
}

bool resolve_item_interception(const item_action_definition &definition,
			       std::unique_ptr<item_action_adapter> adapter, P_char defender,
			       P_char target, P_obj source)
{
	if (!nevent_require_game_thread("resolve_item_interception") || !adapter ||
	    !valid_definition(definition) || definition.selected_effects ||
	    definition.mode != item_action_mode::passive ||
	    definition.source != item_action_source::equipped || definition.effect_count != 1 ||
	    definition.windup_pulses || definition.progress_pulses)
		return false;
	auto instance = std::make_shared<ability>(ability{ definition, std::move(adapter), true });
	return start_action(definition.id, defender, target, source, nullptr, std::move(instance),
			    true) == item_action_start::resolved;
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
		},
		item_action_cancel_reason::abort);
	return true;
}

size_t item_actions_pending()
{
	return pending.size();
}

bool item_action_pending(uint64_t id)
{
	return pending.contains(id);
}

bool item_actions_object_busy(uint64_t object_uid)
{
	if (!object_uid || !nevent_is_game_thread())
		return true;
	for (const auto &[id, entry] : pending)
	{
		if (!entry || !entry->selected || !entry->selected->adapter)
			return true;
		if (entry->identity.source_uid == object_uid ||
		    entry->selected->adapter->references_object(object_uid))
			return true;
	}
	return false;
}

bool item_actions_telemetry_enabled()
{
	return nevent_is_game_thread() && telemetry.enabled;
}

void item_actions_note(item_action_metric metric)
{
	const size_t index = static_cast<size_t>(metric);
	if (nevent_is_game_thread() && telemetry.enabled && index < telemetry.counters.size())
		increment(telemetry.counters[index]);
}

item_action_telemetry item_actions_telemetry_snapshot()
{
	if (!nevent_is_game_thread())
		return {};
	auto snapshot = telemetry;
	snapshot.pending = pending.size();
	return snapshot;
}

void item_actions_dump_telemetry(P_char actor)
{
	if (!nevent_is_game_thread() || !actor || !IS_TRUSTED(actor))
		return;
	const auto snapshot = item_actions_telemetry_snapshot();
	char line[256];
	snprintf(line, sizeof(line), "Item actions telemetry: %s; pending=%zu peak=%zu.\r\n",
		 snapshot.enabled ? "enabled" : "disabled", snapshot.pending,
		 snapshot.peak_pending);
	send_to_char(line, actor);
	if (!snapshot.enabled)
		return;
	constexpr std::array<const char *, static_cast<size_t>(item_action_metric::count)> names = {
		"selected",
		"started",
		"completed",
		"partial",
		"busy",
		"invalid",
		"scheduling_rejected",
		"consumption_rejected",
		"insufficient_mana",
		"mana_unavailable",
		"effects_invoked",
		"effect_failures"
	};
	constexpr std::array<const char *, static_cast<size_t>(item_action_cancel_reason::count)>
		reasons = { "runtime_cleanup",
			    "invalid_context",
			    "clock_failure",
			    "actor_departure",
			    "target_departure",
			    "source_departure",
			    "definition_change",
			    "configuration_change",
			    "reload",
			    "abort",
			    "scheduling_rejected" };
	for (size_t i = 0; i < names.size(); ++i)
	{
		snprintf(line, sizeof(line), "  %s=%llu\r\n", names[i],
			 static_cast<unsigned long long>(snapshot.counters[i]));
		send_to_char(line, actor);
	}
	for (size_t i = 0; i < reasons.size(); ++i)
	{
		snprintf(line, sizeof(line), "  cancelled.%s=%llu\r\n", reasons[i],
			 static_cast<unsigned long long>(snapshot.cancelled[i]));
		send_to_char(line, actor);
	}
	snprintf(line, sizeof(line),
		 "  callbacks=%llu total_us=%llu max_us=%llu invalid_clock=%llu\r\n",
		 static_cast<unsigned long long>(snapshot.callbacks),
		 static_cast<unsigned long long>(snapshot.callback_total_us),
		 static_cast<unsigned long long>(snapshot.callback_max_us),
		 static_cast<unsigned long long>(snapshot.invalid_clock));
	send_to_char(line, actor);
}

void item_actions_character_leaving(P_char character)
{
	if (!character || pending.empty() ||
	    !nevent_require_game_thread("item_actions_character_leaving"))
		return;
	const uint64_t id = character->runtime_id;
	std::vector<std::shared_ptr<pending_item_action>> selected;
	for (const auto &[token, entry] : pending)
		if (entry->identity.actor_id == id || entry->identity.target_id == id ||
		    entry->selected->adapter->references_character(id))
			selected.push_back(entry);
	for (const auto &entry : selected)
		cancel_action(entry, entry->identity.actor_id == id ?
					     item_action_cancel_reason::actor_departure :
					     item_action_cancel_reason::target_departure);
}

void item_actions_source_leaving(P_obj source)
{
	if (!source || pending.empty() ||
	    !nevent_require_game_thread("item_actions_source_leaving"))
		return;
	const uint64_t uid = source->obj_uid;
	cancel_matching(
		[uid](const pending_item_action &entry) {
			return entry.identity.source_uid == uid ||
			       entry.selected->adapter->references_object(uid);
		},
		item_action_cancel_reason::source_departure);
}

void event_item_action_active(P_char, P_char, P_obj, void *data)
{
	progress_action(data);
}
void event_item_action_passive(P_char, P_char, P_obj, void *data)
{
	progress_action(data);
}

#include <thread>

// The real read_object shell has a fresh persistence UID and has never been
// offered to item-action selection. This additive companion executes the exact
// original source-leaving body after verifying the corresponding live no-match
// fact. A reachable arbitrary adapter::finish needs its own genuine provider.
bool item_actions_source_leaving_native_birth_shell_bounded(
	P_obj source, uint64_t expected_uid, bool *returned,
	bool (*current_global)(size_t *, void *) noexcept, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	if (!source || !expected_uid || source->obj_uid != expected_uid || !returned || *returned ||
	    !current_global || !reserve || !nevent_is_game_thread())
		return false;
	// The source query and original predicate both call the repository's only
	// references_object override: device_adapter's fixed effect array walk.
	// It is allocation-free. Other adapters inherit the literal false body.
	// Original cancel_matching owns an actual empty selected vector, plus both
	// pending/selected ranges, predicate closure and source-departure enum.
	constexpr size_t frames =
		6 * sizeof(void *) + sizeof(uint64_t) + 3 * sizeof(size_t) + 3 * sizeof(bool) +
		sizeof(std::vector<std::shared_ptr<pending_item_action>>) +
		2 * sizeof(decltype(pending)::iterator) +
		2 * sizeof(std::vector<std::shared_ptr<pending_item_action>>::iterator) +
		// Both original/query unordered-map range receivers/endpoints and
		// structured bindings, predicate entry/closure, device receiver/uid/i.
		11 * sizeof(void *) + 2 * sizeof(uint64_t) + sizeof(size_t) +
		sizeof(item_action_cancel_reason) +
		// selected is genuinely empty: vector/_Vector_base destructors and
		// allocator-aware _Destroy range scopes execute, but no shared_ptr
		// element destructor or allocator deallocation is selected.
		7 * sizeof(void *) +
		// nevent_is_game_thread/require: actual id and equality operands.
		5 * sizeof(std::thread::id) + 3 * sizeof(void *);
	size_t global = 0;
	if (!current_global(&global, context) || frames > SIZE_MAX - outer_live ||
	    global > SIZE_MAX - outer_live - frames ||
	    !reserve(outer_live + frames + global, context))
		return false;
	// Actual live table/adapter query, never a supplied assertion or inventory
	// estimate. Authenticated caller provenance must establish why a match is
	// source-unreachable for the genuine supported constructor initializer.
	if (item_actions_object_busy(expected_uid))
		return false;
	item_actions_source_leaving(source);
	*returned = true;
	return current_global(&global, context);
}

#include <type_traits>

extern P_nevent ne_schedule[PULSES_IN_TICK];
extern P_nevent ne_schedule_tail[PULSES_IN_TICK];
extern P_nevent current_nevent;

namespace
{
using native_action_pending_map = decltype(pending);
using native_action_ability_map = decltype(abilities);
using native_action_actor_map = decltype(active_actors);
using native_action_pending_iterator = native_action_pending_map::const_iterator;
using native_action_ability_iterator = native_action_ability_map::const_iterator;
using native_action_pending_control =
	std::_Sp_counted_ptr_inplace<pending_item_action, std::allocator<void>,
				     __gnu_cxx::__default_lock_policy>;
using native_action_ability_control =
	std::_Sp_counted_ptr_inplace<ability, std::allocator<void>,
				     __gnu_cxx::__default_lock_policy>;
using native_action_adapter_control =
	std::_Sp_counted_deleter<item_action_adapter *, std::default_delete<item_action_adapter>,
				 std::allocator<void>, __gnu_cxx::__default_lock_policy>;
static_assert(std::is_same_v<native_action_pending_map::allocator_type,
			     std::allocator<native_action_pending_map::value_type>>);
static_assert(std::is_same_v<native_action_ability_map::allocator_type,
			     std::allocator<native_action_ability_map::value_type>>);
static_assert(std::is_same_v<native_action_actor_map::allocator_type,
			     std::allocator<native_action_actor_map::value_type>>);
static_assert(std::is_same_v<std::unique_ptr<item_action_adapter>::pointer, item_action_adapter *>);

bool native_action_storage_add(size_t amount, size_t &bytes) noexcept
{
	if (amount > SIZE_MAX - bytes)
		return false;
	bytes += amount;
	return true;
}
bool native_action_event_shape() noexcept
{
	bool current_seen = !current_nevent;
	for (size_t bucket = 0; bucket < PULSES_IN_TICK; ++bucket)
	{
		P_nevent slow = ne_schedule[bucket], fast = slow, previous = nullptr;
		while (fast && fast->next_sched)
		{
			slow = slow->next_sched;
			fast = fast->next_sched->next_sched;
			if (slow == fast)
				return false;
		}
		for (P_nevent event = ne_schedule[bucket]; event; event = event->next_sched)
		{
			if (event->element != bucket || event->prev_sched != previous)
				return false;
			if (event == current_nevent)
				current_seen = true;
			previous = event;
		}
		if (previous != ne_schedule_tail[bucket])
			return false;
	}
	// The authenticated original destroy->payload/adapter/output/free graph
	// cannot invoke the real shell/ROOT selecting caller. Its unlisted interval
	// and post-release current pointer therefore cannot overlap this census.
	// Never dereference a malformed/out-of-contract unlisted current pointer.
	return current_seen;
}
action_payload *native_action_event_payload(P_nevent event,
					    nevent_payload_destroy_type action_destroy) noexcept
{
	if (!event->data || event->data_destroy != action_destroy)
		return nullptr;
	// schedule_action uses genuine add_event_owned<action_payload>. Compare its
	// exact installed typed delete function before the original payload cast.
	// Cancellation preserves data_destroy even after clearing func, so actual
	// queued canceled owners are retained and counted by this same identity.
	return static_cast<action_payload *>(event->data);
}
bool native_action_registry_has(const ability *wanted) noexcept
{
	for (const auto &node : abilities)
		if (node.second.get() == wanted)
			return true;
	return false;
}
bool native_action_registry_before(native_action_ability_iterator limit,
				   const ability *wanted) noexcept
{
	for (auto it = abilities.cbegin(); it != limit; ++it)
		if (it->second.get() == wanted)
			return true;
	return false;
}
bool native_action_pending_before(native_action_pending_iterator limit,
				  const pending_item_action *wanted,
				  const ability *selected) noexcept
{
	for (auto it = pending.cbegin(); it != limit; ++it)
		if (it->second && (it->second.get() == wanted ||
				   (selected && it->second->selected.get() == selected)))
			return true;
	return false;
}
bool native_action_payload_before(P_nevent limit, const pending_item_action *wanted,
				  const ability *selected,
				  nevent_payload_destroy_type action_destroy) noexcept
{
	for (size_t bucket = 0; bucket < PULSES_IN_TICK; ++bucket)
		for (P_nevent event = ne_schedule[bucket]; event; event = event->next_sched)
		{
			if (event == limit)
				return false;
			const action_payload *payload =
				native_action_event_payload(event, action_destroy);
			if (payload && payload->entry &&
			    (payload->entry.get() == wanted ||
			     (selected && payload->entry->selected.get() == selected)))
				return true;
		}
	return false;
}
bool native_action_ability_current(const ability *selected, size_t &bytes) noexcept
{
	size_t adapter_bytes = 0;
	if (!selected || !selected->adapter ||
	    !selected->adapter->current_storage_bytes(&adapter_bytes) ||
	    !native_action_storage_add(sizeof(native_action_ability_control), bytes) ||
	    !native_action_storage_add(sizeof(native_action_adapter_control), bytes) ||
	    !native_action_storage_add(adapter_bytes, bytes))
		return false;
	// Every genuine ability construction receives exclusive unique_ptr ownership;
	// separate abilities cannot share one adapter/control allocation. Existing
	// pending selections share the SAME ability owner and are deduplicated below.
	return true;
}
} // namespace

size_t item_actions_native_birth_storage_profile_source_frame_bytes() noexcept
{
	// Complete finite source closure of the profile query below, including its
	// actual game-thread/event-shape/typed map walks and pure virtual profile
	// getters. Known derived getters contain sizeof-only expressions; unknown
	// default's real false-CURRENT profile is likewise allocation-free.
	return 3 * sizeof(size_t) + sizeof(void *) +
	       // Profile map ranges/bindings and payload wheel bucket/event/payload.
	       4 * sizeof(native_action_pending_iterator) +
	       2 * sizeof(native_action_ability_iterator) + 7 * sizeof(void *) + sizeof(P_nevent) +
	       sizeof(action_payload *) + sizeof(bool) +
	       // event_shape: current_seen,bucket,slow,fast,previous,event.
	       sizeof(bool) + sizeof(size_t) + 4 * sizeof(P_nevent) +
	       // event_payload input and typed result; max operands/receiver/result.
	       sizeof(P_nevent) + sizeof(action_payload *) + 6 * sizeof(void *) +
	       3 * sizeof(size_t) +
	       // map begin/end and tree iterators/increment/comparison; shared_ptr get
	       // receivers/results; adapter profile receiver/return (sizeof-only body).
	       14 * sizeof(void *) + 4 * sizeof(native_action_pending_iterator) +
	       2 * sizeof(native_action_ability_iterator) + sizeof(size_t) +
	       // Actual thread get_id/id construction/equality return/operand scopes.
	       5 * sizeof(std::thread::id) + 4 * sizeof(void *) +
	       // Genuine getter's own pointer return, caller's actual identity local,
	       // typed event_payload argument and profile-getter size_t return.
	       add_event_owned_payload_destroy_observer_frame_bytes<action_payload>() +
	       2 * sizeof(nevent_payload_destroy_type) + sizeof(size_t);
}

bool item_actions_native_birth_storage_observer_frame_bytes(size_t *output) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)output;
	return false;
#else
	if (!output || !nevent_is_game_thread() || !native_action_event_shape())
		return false;
	const nevent_payload_destroy_type action_destroy =
		add_event_owned_payload_destroy<action_payload>();
	size_t adapter_frame = 0;
	for (const auto &node : abilities)
	{
		if (!node.second || !node.second->adapter)
			return false;
		adapter_frame =
			std::max(adapter_frame,
				 node.second->adapter->current_storage_observer_frame_bytes());
	}
	for (const auto &node : pending)
	{
		if (!node.second || !node.second->selected || !node.second->selected->adapter)
			return false;
		adapter_frame = std::max(
			adapter_frame,
			node.second->selected->adapter->current_storage_observer_frame_bytes());
	}
	for (size_t bucket = 0; bucket < PULSES_IN_TICK; ++bucket)
		for (P_nevent event = ne_schedule[bucket]; event; event = event->next_sched)
		{
			const action_payload *payload =
				native_action_event_payload(event, action_destroy);
			if (!payload)
			{
				if (event->func == event_item_action_active ||
				    event->func == event_item_action_passive)
					return false;
				continue;
			}
			if (!payload->entry || !payload->entry->selected ||
			    !payload->entry->selected->adapter)
				return false;
			adapter_frame = std::max(adapter_frame,
						 payload->entry->selected->adapter
							 ->current_storage_observer_frame_bytes());
		}
	// Actual CURRENT declarations and ALL nested duplicate-owner walks. These
	// are iterative even for genuine shared selections, not a new allocation
	// registry/vector or a snapshot of asserted live bytes.
	const size_t own =
		sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(native_action_ability_iterator) +
		2 * sizeof(native_action_pending_iterator) + sizeof(P_nevent) +
		sizeof(action_payload *) +
		// registry_has/before, pending_before, payload_before: their genuine
		// argument pointers/iterator limits, local typed iterators/bucket/event/
		// payload, range bindings and shared pointer receiver/result scopes.
		12 * sizeof(void *) + 4 * sizeof(native_action_ability_iterator) +
		4 * sizeof(native_action_pending_iterator) + sizeof(size_t) + sizeof(P_nevent) +
		sizeof(action_payload *) +
		// ability_current selected/bytes ref/adapter_bytes, add(amount,bytes ref),
		// node-count multiplication request and observer profile return carriers.
		5 * sizeof(void *) + 4 * sizeof(size_t) + 3 * sizeof(bool) +
		// Complete actual map-range/iterator/get helper declared source scopes.
		24 * sizeof(void *) + 8 * sizeof(native_action_pending_iterator) +
		8 * sizeof(native_action_ability_iterator) +
		item_actions_native_birth_storage_profile_source_frame_bytes() +
		// CURRENT's real identity local and both nested payload-walk argument
		// copies, plus genuine getter return/profile-query source carriers.
		4 * sizeof(nevent_payload_destroy_type) + sizeof(size_t);
	if (adapter_frame > SIZE_MAX - own)
		return false;
	*output = own + adapter_frame;
	return true;
#endif
}

bool item_actions_native_birth_current_storage_bytes(size_t *output) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)output;
	return false;
#else
	if (!output || !nevent_is_game_thread() || !native_action_event_shape())
		return false;
	const nevent_payload_destroy_type action_destroy =
		add_event_owned_payload_destroy<action_payload>();
	// Pure true owner inventory, separate from shared pool/wheel/controller G.
	// map nodes use the genuine installed default allocator/rebind type; actual
	// make_shared controls include their owned object inline exactly once.
	size_t bytes = sizeof(config) + sizeof(next_action_id) + sizeof(abilities) +
		       sizeof(pending) + sizeof(active_actors) + sizeof(telemetry) +
		       sizeof(telemetry_generation);
	if (abilities.size() >
		    (SIZE_MAX - bytes) /
			    sizeof(std::_Rb_tree_node<native_action_ability_map::value_type>) ||
	    !native_action_storage_add(
		    abilities.size() *
			    sizeof(std::_Rb_tree_node<native_action_ability_map::value_type>),
		    bytes) ||
	    pending.size() >
		    (SIZE_MAX - bytes) /
			    sizeof(std::_Rb_tree_node<native_action_pending_map::value_type>) ||
	    !native_action_storage_add(
		    pending.size() *
			    sizeof(std::_Rb_tree_node<native_action_pending_map::value_type>),
		    bytes) ||
	    active_actors.size() >
		    (SIZE_MAX - bytes) /
			    sizeof(std::_Rb_tree_node<native_action_actor_map::value_type>) ||
	    !native_action_storage_add(
		    active_actors.size() *
			    sizeof(std::_Rb_tree_node<native_action_actor_map::value_type>),
		    bytes))
		return false;
	for (auto it = abilities.cbegin(); it != abilities.cend(); ++it)
	{
		if (!it->second)
			return false;
		if (!native_action_registry_before(it, it->second.get()) &&
		    !native_action_ability_current(it->second.get(), bytes))
			return false;
	}
	for (auto it = pending.cbegin(); it != pending.cend(); ++it)
	{
		if (!it->second || !it->second->selected)
			return false;
		if (!native_action_pending_before(it, it->second.get(), nullptr) &&
		    !native_action_storage_add(sizeof(native_action_pending_control), bytes))
			return false;
		if (!native_action_registry_has(it->second->selected.get()) &&
		    !native_action_pending_before(it, nullptr, it->second->selected.get()) &&
		    !native_action_ability_current(it->second->selected.get(), bytes))
			return false;
	}
	for (size_t bucket = 0; bucket < PULSES_IN_TICK; ++bucket)
		for (P_nevent event = ne_schedule[bucket]; event; event = event->next_sched)
		{
			const action_payload *payload =
				native_action_event_payload(event, action_destroy);
			if (!payload)
			{
				if (event->func == event_item_action_active ||
				    event->func == event_item_action_passive)
					return false;
				continue;
			}
			if (!payload->entry || !payload->entry->selected ||
			    !native_action_storage_add(sizeof(action_payload), bytes))
				return false;
			if (!native_action_pending_before(pending.cend(), payload->entry.get(),
							  nullptr) &&
			    !native_action_payload_before(event, payload->entry.get(), nullptr,
							  action_destroy) &&
			    !native_action_storage_add(sizeof(native_action_pending_control),
						       bytes))
				return false;
			if (!native_action_registry_has(payload->entry->selected.get()) &&
			    !native_action_pending_before(pending.cend(), nullptr,
							  payload->entry->selected.get()) &&
			    !native_action_payload_before(event, nullptr,
							  payload->entry->selected.get(),
							  action_destroy) &&
			    !native_action_ability_current(payload->entry->selected.get(), bytes))
				return false;
		}
	*output = bytes;
	return true;
#endif
}

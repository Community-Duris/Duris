#include "item/item_actions.h"

#include "core/prototypes.h"
#include "core/utils.h"
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
	    !actor->runtime_id || !target->runtime_id || actor->in_room == NOWHERE ||
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

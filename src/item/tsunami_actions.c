#include "item/native_artifact_actions.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "cmd/interp.h"
#include "item/artifact_mana.h"
#include "magic/spells.h"
#include "net/comm.h"
#include <ctime>
#include <vector>

extern P_room world;
extern P_index obj_index;

namespace
{
constexpr int TSUNAMI = 31514;
class tsunami_adapter final : public item_action_adapter
{
	const native_artifact_config settings;
	const int command, level;
	const time_t selected_at;

    public:
	tsunami_adapter(native_artifact_config config, int cmd, int power, time_t now)
		: settings(config)
		, command(cmd)
		, level(power)
		, selected_at(now)
	{
	}
	bool validate(const item_action_context &c) const noexcept override
	{
		if (!native_artifact_actor(c.actor) || OBJ_VNUM(c.source) != TSUNAMI ||
		    c.actor->equipment[WIELD] != c.source ||
		    (IS_OBJ_STAT2(c.source, ITEM2_ACCOUNT_BOUND) &&
		     !account_bound_reward_owner(c.actor, c.source)))
			return false;
		if (command == CMD_TAP)
			return true;
		if (is_in_safe(c.actor) || IS_AFFECTED5(c.actor, AFF5_NOT_OFFENSIVE) ||
		    affected_by_spell(c.actor, SONG_PEACE) ||
		    affected_by_spell(c.actor, SKILL_GAZE))
			return false;
		return command == CMD_THRUST ?
			       HAS_FOOTING(c.actor) && !IS_WATER_ROOM(c.actor->in_room) :
			       !HAS_FOOTING(c.actor) || IS_WATER_ROOM(c.actor->in_room);
	}
	item_action_consumption commit(const item_action_context &c) const noexcept override
	{
		const int timer = command == CMD_TAP ? 0 : 1;
		const int cooldown = command == CMD_TAP ? 300 : 500;
		// Compare without overflowing a legacy signed timer. Backward time cannot bypass it.
		if (selected_at < 0 || c.source->timer[timer] > selected_at ||
		    selected_at - c.source->timer[timer] < cooldown ||
		    !artifact_mana_debit(c.source, settings.cost, false, c.identity.action_id))
			return item_action_consumption::rejected;
		c.source->timer[timer] = selected_at;
		return item_action_consumption::committed;
	}
	void announce(const item_action_context &c) const noexcept override
	{
		act(command == CMD_TAP ?
			    "Water begins gathering around $p to restore your allies." :
			    "You raise a gathering storm through $p!",
		    FALSE, c.actor, c.source, nullptr, TO_CHAR);
		act(command == CMD_TAP ?
			    "$n's $p begins gathering restorative water." :
			    "$n's $p begins gathering a storm. Move before the wave arrives!",
		    FALSE, c.actor, c.source, nullptr, TO_ROOM);
	}
	void progress(const item_action_context &c) const noexcept override
	{
		act("The water gathering around $p surges higher!", FALSE, c.actor, c.source,
		    nullptr, TO_CHAR);
		act("The water gathering around $n's $p surges higher!", FALSE, c.actor, c.source,
		    nullptr, TO_ROOM);
	}
	void resolve(const item_action_context &c,
		     const item_action_effect &) const noexcept override
	{
		std::vector<uint64_t> targets;
		for (P_char target = world[c.identity.origin_room].people; target;
		     target = target->next_in_room)
		{
			if (targets.size() == 4096)
				return;
			targets.push_back(target->runtime_id);
		}
		for (uint64_t id : targets)
		{
			P_char actor = nullptr;
			P_obj source = nullptr;
			if (!native_artifact_current(c.identity, actor, source))
				return;
			P_char target = find_character_by_runtime_id(id);
			if (!IS_ALIVE(target) || target->in_room != c.identity.origin_room)
				continue;
			if (command == CMD_TAP)
			{
				// Both initial application and refresh are restricted to self/group.
				if (target != actor &&
				    (!actor->group || actor->group != target->group))
					continue;
				if (affected_by_spell(target, SPELL_VITALITY))
				{
					for (auto *affect = target->affected; affect;
					     affect = affect->next)
						if (affect->type == SPELL_VITALITY)
							affect->duration = 15;
				}
				else
					spell_vitality(level, actor, nullptr, SPELL_TYPE_SPELL,
						       target, nullptr);
				continue; // Native spells may remove any participant; reacquire next iteration.
			}
			if (!native_artifact_hostile(actor, target) ||
			    !should_area_hit(actor, target))
				continue;
			const bool water = IS_WATER_ROOM(c.identity.origin_room);
			const bool success = (water && number(0, 2)) ||
					     (HAS_FOOTING(target) && number(0, 1)) ||
					     (!HAS_FOOTING(target) && !water && !number(0, 2));
			if (success)
			{
				act("Tsunami's wave knocks you down!", FALSE, target, nullptr,
				    nullptr, TO_CHAR);
				act("Tsunami's wave knocks $n down!", FALSE, target, nullptr,
				    nullptr, TO_ROOM);
				SET_POS(target, POS_PRONE + GET_STAT(target));
			}
		}
	}
	void finish(const item_action_identity &id, item_action_consumption,
		    item_action_outcome outcome) const noexcept override
	{
		if (outcome != item_action_outcome::completed)
			if (P_char actor = find_character_by_runtime_id(id.actor_id))
				send_to_char("Tsunami's gathering water disperses.\r\n", actor);
	}
};
}

item_action_start begin_tsunami_action(P_obj source, P_char actor, int command)
{
	if (!native_artifact_owns(TSUNAMI))
		return item_action_start::legacy;
	const auto config = native_artifact_settings(TSUNAMI);
	if (!source || !native_artifact_actor(actor) ||
	    (command != CMD_TAP && command != CMD_THRUST && command != CMD_RAISE) ||
	    !native_artifact_prepare(TSUNAMI, config))
		return item_action_start::suppressed;
	item_action_definition definition;
	definition.id = native_artifact_ability(TSUNAMI, command == CMD_TAP ? 1 : 2);
	definition.revision = config.revision;
	definition.mode = item_action_mode::active;
	definition.windup_pulses = config.windup;
	definition.progress_pulses = config.windup / 2;
	definition.effect_count = 1;
	definition.effects[0].id = command == CMD_TAP ? 1 : 2;
	return start_item_action_instance(
		definition,
		std::make_unique<tsunami_adapter>(config, command, GET_LEVEL(actor), time(nullptr)),
		actor, actor, source);
}

#include "item/wonder_actions.h"
#include "item/native_artifact_actions.h"

#include "core/prototypes.h"
#include "core/utils.h"
#include "magic/spells.h"
#include "net/comm.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>

extern P_index obj_index;
extern P_room world;
extern const int top_of_world;
extern Skill skills[];

namespace
{
constexpr int WONDER_VNUM = 41350;
constexpr uint32_t WONDER_ABILITY = 0x80000000U | WONDER_VNUM;
struct wonder_config
{
	bool valid = false, enabled = false;
	int windup = 8;
	bool operator==(const wonder_config &) const = default;
};
wonder_config wonder_settings;
uint64_t wonder_revision = 1;
bool wonder_integer(const char *key, int fallback, int low, int high, int &value)
{
	const float raw = get_property(key, static_cast<double>(fallback), false);
	if (!std::isfinite(raw) || raw < low || raw > high || std::floor(raw) != raw)
		return false;
	value = static_cast<int>(raw);
	return true;
}
struct wonder_selection
{
	int choice = 0, level = 0, aging = 0, gem_count = 0;
	std::array<int, 40> gems{};
};
int wonder_spell(int choice)
{
	switch (choice)
	{
	case 1:
		return SPELL_MINOR_PARALYSIS;
	case 2:
		return SPELL_CYCLONE;
	case 3:
		return SPELL_LIGHTNING_BOLT;
	case 4:
		return SPELL_DARKNESS;
	case 5:
	case 7:
		return SPELL_CONCEALMENT;
	case 6:
		return SPELL_FIREBALL;
	default:
		return 0;
	}
}
bool wonder_harmful(const wonder_selection &selection)
{
	const int spell = wonder_spell(selection.choice);
	return selection.choice == 12 || (spell && IS_AGG_SPELL(spell));
}
bool wonder_actor(P_char actor)
{
	return IS_ALIVE(actor) && IS_AWAKE(actor) && GET_STAT(actor) >= STAT_RESTING &&
	       !IS_IMMOBILE(actor) && !IS_AFFECTED2(actor, AFF2_STUNNED) &&
	       !CHAR_IN_NO_MAGIC_ROOM(actor) && !(IS_NPC(actor) && IS_AFFECTED(actor, AFF_CHARM));
}
class wonder_adapter final : public item_action_adapter
{
	const wonder_selection selection;
	const std::string name;
	bool current(const item_action_identity &identity, P_char &actor, P_char &target,
		     P_obj &source) const noexcept
	{
		if (!item_action_pending(identity.action_id))
			return false;
		actor = find_character_by_runtime_id(identity.actor_id);
		target = find_character_by_runtime_id(identity.target_id);
		if (!wonder_actor(actor) || !IS_ALIVE(target) ||
		    actor->in_room != identity.origin_room ||
		    target->in_room != identity.origin_room)
			return false;
		source = actor->equipment[HOLD];
		return source && source->obj_uid == identity.source_uid &&
		       OBJ_WORN_BY(source, actor) && source->R_num >= 0 &&
		       OBJ_VNUM(source) == WONDER_VNUM;
	}
	void summon(const item_action_identity &identity, int vnum, int count) const noexcept
	{
		for (int i = 0; i < count; ++i)
		{
			P_char actor = nullptr, target = nullptr;
			P_obj source = nullptr;
			if (!current(identity, actor, target, source))
				return;
			P_char creature = read_mobile(vnum, VIRTUAL);
			if (!creature)
				continue;
			if (!current(identity, actor, target, source))
			{
				extract_char(creature);
				return;
			}
			char_to_room(creature, identity.origin_room, 0);
		}
	}

    public:
	wonder_adapter(wonder_selection selected, std::string source_name)
		: selection(std::move(selected))
		, name(std::move(source_name))
	{
	}
	bool validate(const item_action_context &context) const noexcept override
	{
		if (!wonder_actor(context.actor) || OBJ_VNUM(context.source) != WONDER_VNUM ||
		    context.actor->equipment[HOLD] != context.source ||
		    !CAN_SEE(context.actor, context.target) ||
		    (IS_OBJ_STAT2(context.source, ITEM2_ACCOUNT_BOUND) &&
		     !account_bound_reward_owner(context.actor, context.source)))
			return false;
		if (wonder_harmful(selection))
		{
			if (is_in_safe(context.actor) ||
			    IS_AFFECTED5(context.actor, AFF5_NOT_OFFENSIVE) ||
			    affected_by_spell(context.actor, SONG_PEACE) ||
			    affected_by_spell(context.actor, SKILL_GAZE))
				return false;
			if (context.actor != context.target &&
			    (!CanDoFightMove(context.actor, context.target) ||
			     (IS_PC(context.target) &&
			      should_not_kill(context.actor, context.target))))
				return false;
			const int spell = wonder_spell(selection.choice);
			if (spell && IS_SET(skills[spell].targets, TAR_AREA | TAR_IGNORE) &&
			    (GET_MASTER(context.actor) &&
			     should_area_hit(context.actor, GET_MASTER(context.actor))))
				return false;
		}
		return true;
	}
	item_action_consumption commit(const item_action_context &context) const noexcept override
	{
		if (context.source->value[2] <= 0)
			return item_action_consumption::rejected;
		--context.source->value[2];
		return item_action_consumption::committed;
	}
	void announce(const item_action_context &context) const noexcept override
	{
		act("&+WYou begin channeling unpredictable magic through $p!&n", FALSE,
		    context.actor, context.source, context.target, TO_CHAR);
		act("&+W$n begins channeling unpredictable magic through $p!&n", FALSE,
		    context.actor, context.source, context.target, TO_NOTVICT);
		if (context.target != context.actor)
			act(wonder_harmful(selection) ?
				    "&+R$n aims $p's gathering wild magic toward YOU!&n" :
				    "&+W$n directs $p's gathering magic toward you!&n",
			    FALSE, context.actor, context.source, context.target, TO_VICT);
	}
	void progress(const item_action_context &context) const noexcept override
	{
		act("&+WThe wild magic in $p grows brighter!&n", FALSE, context.actor,
		    context.source, nullptr, TO_CHAR);
		act("&+WThe wild magic in $n's $p grows brighter!&n", FALSE, context.actor,
		    context.source, nullptr, TO_ROOM);
	}
	void resolve(const item_action_context &context,
		     const item_action_effect &) const noexcept override
	{
		const auto &identity = context.identity;
		P_char actor = context.actor, target = context.target;
		P_obj source = context.source;
		act("&+WYou release the wild magic in $p!&n", FALSE, actor, source, nullptr,
		    TO_CHAR);
		act("&+W$n releases the wild magic in $p!&n", FALSE, actor, source, nullptr,
		    TO_ROOM);
		const int level = selection.level;
		switch (selection.choice)
		{
		case 1:
			spell_minor_paralysis(level, actor, nullptr, SPELL_TYPE_WAND, target,
					      nullptr);
			break;
		case 2:
			spell_cyclone(level, actor, nullptr, SPELL_TYPE_WAND, target, nullptr);
			break;
		case 3:
			spell_lightning_bolt(level, actor, nullptr, SPELL_TYPE_WAND, target,
					     nullptr);
			break;
		case 4:
			spell_darkness(level, actor, nullptr, SPELL_TYPE_WAND, nullptr, nullptr);
			break;
		case 5:
			spell_concealment(level, actor, nullptr, SPELL_TYPE_WAND, target, nullptr);
			break;
		case 6:
			spell_fireball(level, actor, nullptr, SPELL_TYPE_WAND, target, nullptr);
			break;
		case 7:
			spell_concealment(level, actor, nullptr, SPELL_TYPE_WAND, actor, nullptr);
			break;
		case 8:
		case 9:
			summon(identity, 5710, 1);
			break; // Both legacy branches use 5710.
		case 10:
			summon(identity, 12803, level + 10);
			break;
		case 11:
			summon(identity, 97514, 1);
			break;
		case 12:
			for (int i = 0; i < selection.gem_count; ++i)
			{
				if (!current(identity, actor, target, source))
					return;
				P_obj gem = read_object(selection.gems[i], VIRTUAL);
				if (!current(identity, actor, target, source))
				{
					if (gem)
						extract_obj(gem);
					return;
				}
				if (!gem)
					send_to_char("error: couldn't load gems..  tell god\r\n",
						     actor);
				else
				{
					REMOVE_BIT(source->extra_flags, ITEM_SECRET);
					obj_to_room(gem, identity.origin_room);
				}
			}
			if (!current(identity, actor, target, source))
				return;
			if (!damage(actor, target, selection.gem_count, TYPE_UNDEFINED) &&
			    current(identity, actor, target, source))
				act("Gems shoot forth from $n's wand, striking you in the head!",
				    TRUE, actor, nullptr, nullptr, TO_ROOM);
			break;
		case 13:
			send_to_room("&+GGrass begins growing under your feet.\r\n",
				     identity.origin_room);
			break;
		case 14:
			send_to_room("&+WA thick fog shoots forth from the wand.\r\n",
				     identity.origin_room);
			break;
		case 15:
		case 16:
			AgeChar(actor, selection.aging);
			if (current(identity, actor, target, source))
				send_to_char(
					"Time seems to have caught up with you, aging you considerably.\r\n",
					actor);
			break;
		default:
			send_to_char("Nothing seems to happen.", actor);
			break;
		}
	}
	void finish(const item_action_identity &identity, item_action_consumption,
		    item_action_outcome outcome) const noexcept override
	{
		if (outcome == item_action_outcome::completed)
			return;
		const std::string message =
			"The unfinished wild magic in " + name + "&n fades.\r\n";
		if (identity.origin_room >= 0 && identity.origin_room <= top_of_world)
			send_to_room(message.c_str(), identity.origin_room);
		P_char actor = find_character_by_runtime_id(identity.actor_id);
		if (IS_ALIVE(actor) && actor->in_room != identity.origin_room)
			send_to_char(message.c_str(), actor);
	}
};
} // namespace

void update_wonder_action_properties()
{
	if (!nevent_require_game_thread("update_wonder_action_properties"))
		return;
	wonder_config next;
	int enabled = 0, individual = 1;
	next.valid = wonder_integer("itemActions.wonder.enabled", 0, 0, 1, enabled) &&
		     wonder_integer("itemActions.device.41350.enabled", 1, 0, 1, individual) &&
		     wonder_integer("itemActions.wonder.windupPulses", 8, 4, 600, next.windup);
	next.enabled = enabled == 1 && individual == 1;
	if (next != wonder_settings)
	{
		item_actions_disable(WONDER_ABILITY);
		wonder_settings = next;
		if (wonder_revision != std::numeric_limits<uint64_t>::max())
			++wonder_revision;
	}
}

item_action_start begin_wonder_action(P_obj source, P_char actor, P_char target, int selected_level)
{
	if (!nevent_require_game_thread("begin_wonder_action"))
		return item_action_start::suppressed;
	if (!item_actions_enabled())
		return item_action_start::legacy;
	if (!native_artifact_control_enabled(WONDER_VNUM))
		return item_action_start::suppressed;
	if (!native_artifact_variant_enabled(WONDER_VNUM, actor, "telegraphic"))
		return item_action_start::legacy;
	if (!native_artifact_power_enabled(WONDER_VNUM, "wonder"))
		return item_action_start::suppressed;
	update_wonder_action_properties();
	if (wonder_settings.valid && !wonder_settings.enabled)
		return item_action_start::legacy;
	if (!wonder_settings.valid || wonder_revision == std::numeric_limits<uint64_t>::max() ||
	    !source || !wonder_actor(actor) || !IS_ALIVE(target) || source->R_num < 0 ||
	    OBJ_VNUM(source) != WONDER_VNUM || selected_level < 20 || selected_level > 40)
		return item_action_start::suppressed;
	wonder_selection selection;
	selection.level = std::clamp(
		native_artifact_power_level(WONDER_VNUM, "wonder", selected_level), 20, 40);
	selection.choice = number(1, 20);
	if (selection.choice == 12)
	{
		// Preserve the legacy loop's repeated bound roll, not a uniform one-time
		// gem count. Capture the complete selected native effect before admission.
		for (int i = 0; i < number(10, 40); ++i)
			selection.gems[selection.gem_count++] = 66034 + number(0, 9);
	}
	if (selection.choice == 15 || selection.choice == 16)
		selection.aging = number(1, 50);
	if (selection.choice == 4 || selection.choice == 7 ||
	    (selection.choice >= 8 && selection.choice != 12))
		target = actor;
	item_action_definition definition;
	definition.id = WONDER_ABILITY;
	definition.revision = wonder_revision;
	definition.mode = item_action_mode::active;
	definition.windup_pulses = wonder_settings.windup;
	definition.progress_pulses = wonder_settings.windup / 2;
	definition.effect_count = 1;
	definition.effects[0] = { static_cast<uint32_t>(selection.choice), selected_level,
				  item_action_call::wand };
	return start_item_action_instance(
		definition,
		std::make_unique<wonder_adapter>(
			selection, OBJ_SHORT(source) ? OBJ_SHORT(source) : "the wand of wonder"),
		actor, target, source);
}

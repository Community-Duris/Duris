#include "item/native_artifact_actions.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "cmd/interp.h"
#include "combat/damage.h"
#include "item/artifact_mana.h"
#include "magic/spells.h"
#include "net/comm.h"
#include <algorithm>
#include <array>
#include <ctime>
#include <initializer_list>
#include <limits>
#include <vector>

extern P_index obj_index;
extern P_room world;
void good_evil_poofSword(P_char, P_obj);
int isWieldingVnum(P_char, int);
int attemptToDisengage(P_char, int, char *);
bool has_skin_spell(P_char);

namespace
{
enum class sword_stage : uint32_t
{
	combat = 1,
	defense,
	shield,
	skin,
	challenge,
	deflect,
	flurry
};
struct sword_selection
{
	sword_stage stage = sword_stage::combat;
	int choice = 0;
	int multiplier = 1;
};
using sword_spell = void (*)(int, P_char, char *, int, P_char, P_obj);

bool sword_enemy(P_char actor, P_char target, int vnum)
{
	return native_artifact_hostile(actor, target) && !IS_TRUSTED(target) &&
	       isWieldingVnum(target, vnum == 21 ? 22 : 21);
}

class sword_adapter final : public item_action_adapter
{
	const int vnum;
	const native_artifact_config settings;
	const std::array<sword_selection, 3> selected;
	const size_t count;
	const time_t selected_at;
	const int cursor;
	bool hostile() const
	{
		for (size_t i = 0; i < count; ++i)
			if (selected[i].stage == sword_stage::combat ||
			    selected[i].stage == sword_stage::challenge ||
			    selected[i].stage == sword_stage::flurry)
				return true;
		return false;
	}
	bool current(const item_action_context &c, P_char &actor, P_char &target,
		     P_obj &source) const
	{
		if (!native_artifact_current(c.identity, actor, source))
			return false;
		target = find_character_by_runtime_id(c.identity.target_id);
		return IS_ALIVE(target) && target->in_room == c.identity.origin_room &&
		       validate({ c.identity, c.definition, actor, target, source });
	}
	bool invoke(const item_action_context &c, sword_spell spell, int power, uint64_t recipient,
		    int type = 0, bool penalize_save = false) const
	{
		P_char actor = nullptr, anchor = nullptr;
		P_obj source = nullptr;
		if (!current(c, actor, anchor, source))
			return false;
		P_char target = recipient ? find_character_by_runtime_id(recipient) : nullptr;
		if (recipient && (!IS_ALIVE(target) || target->in_room != c.identity.origin_room))
			return false;
		const int saved =
			penalize_save ? target->specials.apply_saving_throw[SAVING_SPELL] : 0;
		if (penalize_save)
			target->specials.apply_saving_throw[SAVING_SPELL] =
				std::min(saved, std::numeric_limits<int>::max() - 15) + 15;
		spell(power, actor, nullptr, type, target, nullptr);
		// Undo the temporary save adjustment even if the spell moved the target.
		if (penalize_save)
			if (P_char survivor = find_character_by_runtime_id(recipient))
				survivor->specials.apply_saving_throw[SAVING_SPELL] = saved;
		return current(c, actor, anchor, source);
	}
	bool group(const item_action_context &c, std::initializer_list<sword_spell> spells,
		   int level) const
	{
		std::vector<uint64_t> targets;
		for (P_char target = world[c.identity.origin_room].people; target;
		     target = target->next_in_room)
		{
			if (targets.size() == 128)
				return false;
			targets.push_back(target->runtime_id);
		}
		for (uint64_t id : targets)
		{
			P_char actor = nullptr, anchor = nullptr;
			P_obj source = nullptr;
			if (!current(c, actor, anchor, source))
				return false;
			P_char target = find_character_by_runtime_id(id);
			if (!IS_ALIVE(target) || target->in_room != c.identity.origin_room ||
			    (target != actor && (!actor->group || actor->group != target->group)))
				continue;
			for (sword_spell spell : spells)
			{
				actor = find_character_by_runtime_id(c.identity.actor_id);
				target = find_character_by_runtime_id(id);
				if (!IS_ALIVE(actor) || !IS_ALIVE(target) ||
				    (target != actor &&
				     (!actor->group || actor->group != target->group)))
					break;
				if (!invoke(c, spell, level, id))
					return false;
			}
		}
		return true;
	}
	bool group(const item_action_context &c, sword_spell spell, int level) const
	{
		return group(c, { spell }, level);
	}
	void group_heal(const item_action_context &c) const
	{
		P_char actor = nullptr, anchor = nullptr;
		P_obj source = nullptr;
		if (!current(c, actor, anchor, source))
			return;
		int points = 70 + number(0, 10);
		if (GET_SPEC(actor, CLASS_CLERIC, SPEC_HEALER))
			points = 120 + number(0, 20);
		if (GET_CHAR_SKILL(actor, SKILL_DEVOTION) > 0)
			points += GET_CHAR_SKILL(actor, SKILL_DEVOTION) / 5;
		std::vector<uint64_t> targets{ c.identity.actor_id };
		for (P_char target = world[c.identity.origin_room].people; target;
		     target = target->next_in_room)
		{
			if (target == actor)
				continue;
			if (targets.size() == 128)
				return;
			targets.push_back(target->runtime_id);
		}
		for (uint64_t id : targets)
		{
			if (!current(c, actor, anchor, source))
				return;
			P_char target = find_character_by_runtime_id(id);
			if (!IS_ALIVE(target) || target->in_room != c.identity.origin_room ||
			    (target != actor && (!actor->group || actor->group != target->group ||
						 GET_HIT(target) >= GET_MAX_HIT(target))))
				continue;
			heal(target, actor, points, GET_MAX_HIT(target) - number(1, 4));
			if (!current(c, actor, anchor, source))
				return;
			target = find_character_by_runtime_id(id);
			if (!IS_ALIVE(target))
				continue;
			if (target != actor)
				update_pos(target);
			send_to_char("A warm feeling of peace fills your body.\r\n", target);
		}
	}
	void drain(const item_action_context &c) const
	{
		P_char actor = nullptr, target = nullptr;
		P_obj source = nullptr;
		if (!current(c, actor, target, source))
			return;
		int dealt = 0;
		const int available_life = std::max(0, GET_HIT(target));
		spell_damage(actor, target, 50, SPLDAM_NEGATIVE, SPLDAM_NODEFLECT, nullptr, &dealt);
		actor = find_character_by_runtime_id(c.identity.actor_id);
		if (IS_ALIVE(actor) && dealt > 0)
			vamp(actor, std::min(dealt, available_life), GET_MAX_HIT(actor));
		// Healing never credits the item reserve, even in a nemesis fight.
	}

    public:
	sword_adapter(int number, native_artifact_config config,
		      std::array<sword_selection, 3> powers, size_t power_count, time_t now,
		      int state)
		: vnum(number)
		, settings(config)
		, selected(powers)
		, count(power_count)
		, selected_at(now)
		, cursor(state)
	{
	}
	bool validate(const item_action_context &c) const noexcept override
	{
		if (!native_artifact_actor(c.actor) || OBJ_VNUM(c.source) != vnum ||
		    c.actor->equipment[PRIMARY_WEAPON] != c.source ||
		    (IS_OBJ_STAT2(c.source, ITEM2_ACCOUNT_BOUND) &&
		     !account_bound_reward_owner(c.actor, c.source)))
			return false;
		if (!IS_TRUSTED(c.actor) && IS_PC(c.actor) &&
		    ((vnum == 21 && IS_RACEWAR_GOOD(c.actor)) ||
		     (vnum == 22 && IS_RACEWAR_EVIL(c.actor))))
			return false;
		if (hostile() && !native_artifact_hostile(c.actor, c.target))
			return false;
		for (size_t i = 0; i < count; ++i)
		{
			if (selected[i].stage == sword_stage::combat &&
			    (selected[i].choice == 8 || selected[i].choice == 14) &&
			    (IS_ROOM(c.identity.origin_room, ROOM_SINGLE_FILE) ||
			     (GET_MASTER(c.actor) && should_area_hit(c.actor, GET_MASTER(c.actor)))))
				return false;
			if (selected[i].stage == sword_stage::challenge &&
			    !sword_enemy(c.actor, c.target, vnum))
				return false;
			if (selected[i].stage == sword_stage::flurry &&
			    GET_OPPONENT(c.actor) != c.target)
				return false;
			if (selected[i].stage == sword_stage::combat && selected[i].choice == 12 &&
			    ((vnum == 22 && GET_ALIGNMENT(c.actor) <= -350) ||
			     (vnum == 21 && GET_ALIGNMENT(c.actor) >= 350)))
				return false;
		}
		return true;
	}
	item_action_consumption commit(const item_action_context &c) const noexcept override
	{
		uint64_t cost = 0;
		for (size_t i = 0; i < count; ++i)
		{
			cost += uint64_t(settings.cost) * selected[i].multiplier;
			if (selected[i].stage == sword_stage::challenge &&
			    (selected_at < c.source->timer[1] ||
			     selected_at - c.source->timer[1] < 10))
				return item_action_consumption::rejected;
		}
		if (!artifact_mana_debit(c.source, cost, true, c.identity.action_id))
			return item_action_consumption::rejected;
		for (size_t i = 0; i < count; ++i)
		{
			if (selected[i].stage == sword_stage::combat)
				c.source->value[6] = (cursor + 1) % 15;
			if (selected[i].stage == sword_stage::defense)
				c.source->value[6] = (cursor + 1) % 5;
			if (selected[i].stage == sword_stage::skin)
				c.source->timer[0] = selected_at;
			if (selected[i].stage == sword_stage::challenge)
				c.source->timer[1] = selected_at;
		}
		native_artifact_mark_state(c.actor);
		return item_action_consumption::committed;
	}
	void announce(const item_action_context &c) const noexcept override
	{
		act("$p begins gathering its power!", FALSE, c.actor, c.source, c.target, TO_CHAR);
		act(hostile() ? "$n's $p begins gathering power toward YOU!" :
				"$n's $p begins gathering power.",
		    FALSE, c.actor, c.source, c.target, TO_VICT);
		act("$n's $p begins gathering its power!", FALSE, c.actor, c.source, c.target,
		    TO_NOTVICT);
	}
	void progress(const item_action_context &c) const noexcept override
	{
		act("The light gathering in $p intensifies!", FALSE, c.actor, c.source, c.target,
		    TO_CHAR);
		act("The light gathering in $n's $p intensifies!", FALSE, c.actor, c.source,
		    c.target, TO_ROOM);
	}
	void resolve(const item_action_context &c,
		     const item_action_effect &effect) const noexcept override
	{
		const auto &power = selected[effect.auxiliary];
		const uint64_t self = c.identity.actor_id, target = c.identity.target_id;
		switch (power.stage)
		{
		case sword_stage::shield:
			invoke(c, power.choice ? spell_soulshield : spell_fireshield, 55, self,
			       SPELL_TYPE_SPELL);
			return;
		case sword_stage::skin:
			invoke(c, spell_stone_skin, 55, self, SPELL_TYPE_SPELL);
			return;
		case sword_stage::deflect:
			invoke(c, spell_deflect, 60, self, SPELL_TYPE_SPELL);
			return;
		case sword_stage::challenge:
			if (!invoke(c, spell_blur, 60, self) ||
			    !invoke(c, spell_deflect, 60, self, SPELL_TYPE_SPELL) ||
			    !invoke(c, spell_blur, 60, target) ||
			    !invoke(c, spell_deflect, 60, target, SPELL_TYPE_SPELL))
				return;
			{
				P_char actor = nullptr, opponent = nullptr;
				P_obj source = nullptr;
				if (!current(c, actor, opponent, source))
					return;
				source->value[5] = TRUE;
				native_artifact_mark_state(actor);
				if (GET_OPPONENT(actor) != opponent)
					stop_fighting(actor);
				if (!current(c, actor, opponent, source))
					return;
				if (GET_OPPONENT(opponent) != actor)
					stop_fighting(opponent);
				if (!current(c, actor, opponent, source))
					return;
				attack(actor, opponent);
				if (!current(c, actor, opponent, source))
					return;
				attack(opponent, actor);
			}
			return;
		case sword_stage::flurry:
			for (int i = 0; i < power.choice; ++i)
			{
				P_char actor = nullptr, opponent = nullptr;
				P_obj source = nullptr;
				if (!current(c, actor, opponent, source))
					return;
				hit(actor, opponent, source);
			}
			return;
		case sword_stage::defense:
			switch (power.choice)
			{
			case 0:
				group(c, spell_stornogs_spheres, 60);
				return;
			case 1:
				group_heal(c);
				return;
			case 2:
				group(c, spell_vigorize_critic, 50);
				return;
			case 3:
				group(c,
				      { spell_protection_from_cold, spell_protection_from_fire,
					spell_protection_from_acid, spell_protection_from_gas,
					spell_protection_from_lightning },
				      50);
				return;
			case 4:
				group(c, { spell_armor, spell_bless }, 50);
				return;
			case 5:
				group(c, spell_stone_skin, 45);
				return;
			}
			return;
		case sword_stage::combat:
			switch (power.choice)
			{
			case 0:
				return; // Legacy dazzle is a paid visual-only result.
			case 1:
				invoke(c, spell_blindness, 60, target, SPELL_TYPE_SPELL, true);
				return;
			case 2:
				invoke(c, spell_curse, 60, target, SPELL_TYPE_SPELL, true);
				return;
			case 3:
				invoke(c, spell_bigbys_crushing_hand, 60, target, SPELL_TYPE_SPELL);
				return;
			case 4:
			case 9:
			case 13:
				drain(c);
				return;
			case 5:
				if (group(c, spell_heal, 55))
					invoke(c, spell_heal, 20, self);
				return;
			case 6:
				invoke(c, spell_bigbys_clenched_fist, 60, target, SPELL_TYPE_SPELL);
				return;
			case 7:
				invoke(c, spell_immolate, 60, target);
				return;
			case 8:
				invoke(c, spell_earthquake, 60, 0, SPELL_TYPE_SPELL);
				return;
			case 10:
				group(c, spell_stornogs_spheres, 56);
				return;
			case 11:
				invoke(c, spell_poison, 30, target, SPELL_TYPE_SPELL, true);
				return;
			case 12:
				invoke(c, vnum == 22 ? spell_holy_word : spell_unholy_word, 60,
				       target);
				return;
			case 14:
				resolve_nova(c.actor);
				return;
			}
			return;
		}
	}
	void finish(const item_action_identity &id, item_action_consumption,
		    item_action_outcome outcome) const noexcept override
	{
		if (outcome != item_action_outcome::completed)
			if (P_char actor = find_character_by_runtime_id(id.actor_id))
				send_to_char("Your sword's gathering power disperses.\r\n", actor);
	}
};

sword_selection combat_selection(P_obj source)
{
	const int choice = OBJ_VNUM(source) == 22 ? std::clamp(source->value[6], 0, 14) :
						    number(0, 14);
	constexpr int multipliers[15] = { 1, 1, 1, 2, 1, 2, 2, 1, 2, 1, 2, 1, 2, 1, 4 };
	return { sword_stage::combat, choice, multipliers[choice] };
}

void start_sword(P_obj source, P_char actor, P_char target, std::array<sword_selection, 3> selected,
		 size_t count)
{
	if (!count)
		return;
	const int vnum = OBJ_VNUM(source);
	const auto settings = native_artifact_settings(vnum);
	if (!native_artifact_prepare(vnum, settings))
		return;
	item_action_definition definition;
	definition.id = native_artifact_ability(vnum, 1);
	definition.revision = settings.revision;
	definition.windup_pulses = settings.windup;
	for (size_t i = 0; i < count; ++i)
		if (selected[i].stage == sword_stage::combat && selected[i].choice == 14)
		{
			// The global cap must not shorten nova's native minimum preparation.
			if (get_property("itemActions.maxPulses", 120.0, false) <
			    PULSE_VIOLENCE * 2)
				return;
			definition.windup_pulses = std::max(settings.windup, PULSE_VIOLENCE * 2);
		}
	definition.progress_pulses = definition.windup_pulses / 2;
	definition.effect_count = count;
	for (size_t i = 0; i < count; ++i)
		definition.effects[i] = { static_cast<uint32_t>(selected[i].stage), 0,
					  item_action_call::spell,
					  item_action_effect_target::original,
					  static_cast<int>(i) };
	start_item_action_instance(
		definition,
		std::make_unique<sword_adapter>(vnum, settings, selected, count, time(nullptr),
						std::clamp(source->value[6], 0, 14)),
		actor, target, source);
}
}

int advance_sword_artifact(P_obj source, P_char actor, int command, char *arguments)
{
	if (!source || !IS_ALIVE(actor) || actor->equipment[PRIMARY_WEAPON] != source)
		return FALSE;
	const int vnum = OBJ_VNUM(source);
	if (command == CMD_LOOK && arguments && isname(arguments, source->name))
	{
		const auto settings = native_artifact_settings(vnum);
		if (native_artifact_prepare(vnum, settings))
			do_itemmana(actor, arguments, 0);
		else
			send_to_char("Your sword's mana profile is unavailable.\r\n", actor);
		return TRUE;
	}
	if (!IS_TRUSTED(actor) && IS_PC(actor) &&
	    ((vnum == 21 && IS_RACEWAR_GOOD(actor)) || (vnum == 22 && IS_RACEWAR_EVIL(actor))))
	{
		good_evil_poofSword(actor, source);
		return TRUE;
	}
	if (!native_artifact_actor(actor))
		return FALSE;
	P_char opponent = GET_OPPONENT(actor);
	const bool nemesis = sword_enemy(actor, opponent, vnum);
	if (nemesis && !source->value[5])
	{
		source->value[5] = TRUE;
		native_artifact_mark_state(actor);
	}
	else if (!nemesis && source->value[5])
	{
		source->value[5] = FALSE;
		native_artifact_mark_state(actor);
		// Native aftermath is a penalty, never a resource refill.
		spell_dispel_magic(60, actor, nullptr, SPELL_TYPE_SPELL, actor, nullptr);
		return TRUE;
	}
	if (nemesis && attemptToDisengage(actor, command, arguments))
		return TRUE;
	std::array<sword_selection, 3> selected{};
	size_t count = 0;
	if (command == CMD_PERIODIC)
	{
		if (!nemesis)
			for (P_char target = world[actor->in_room].people; target;
			     target = target->next_in_room)
				if (sword_enemy(actor, target, vnum))
				{
					selected[0] = { sword_stage::challenge, 0, 4 };
					start_sword(source, actor, target, selected, 1);
					return TRUE;
				}
		if (!number(0, 29))
			act("$p whispers, 'My power must recover between battles.'", FALSE, actor,
			    source, nullptr, TO_CHAR);
		const bool extreme = GET_ALIGNMENT(actor) <= -900 || GET_ALIGNMENT(actor) >= 900;
		if (extreme ? !IS_AFFECTED2(actor, AFF2_SOULSHIELD) : !FIRESHIELDED(actor))
			selected[count++] = { sword_stage::shield, extreme ? 1 : 0, 1 };
		const time_t now = time(nullptr);
		if (now >= source->timer[0] && now - source->timer[0] >= 10 &&
		    !has_skin_spell(actor))
			selected[count++] = { sword_stage::skin, 0, 1 };
		if (IS_ALIVE(opponent))
			selected[count++] = combat_selection(source);
		else if (!number(0, 14))
		{
			int choice = vnum == 22 ? std::clamp(source->value[6], 0, 14) % 5 :
						  number(0, 4);
			const int multiplier = choice == 0 ? 3 : 1;
			if (choice == 0 && IS_AFFECTED4(actor, AFF4_STORNOGS_SPHERES) &&
			    number(0, 3))
				choice = 5;
			if (choice == 1)
			{
				bool wounded = false;
				for (P_char target = world[actor->in_room].people; target;
				     target = target->next_in_room)
					if ((target == actor ||
					     (actor->group && actor->group == target->group)) &&
					    GET_HIT(target) < GET_MAX_HIT(target))
						wounded = true;
				if (!wounded)
					choice = 2;
			}
			selected[count++] = { sword_stage::defense, choice, multiplier };
		}
		start_sword(source, actor, IS_ALIVE(opponent) ? opponent : actor, selected, count);
		return TRUE;
	}
	if (nemesis &&
	    (command == CMD_MELEE_HIT || command == CMD_GOTHIT || command == CMD_GOTNUKED))
	{
		if (!number(0, 3) && !IS_AFFECTED4(actor, AFF4_DEFLECT))
			selected[count++] = { sword_stage::deflect, 0, 1 };
		else if (!number(0, 2))
			selected[count++] = combat_selection(source);
		else if (!number(0, 3))
			selected[count++] = { sword_stage::flurry, number(3, 5), 4 };
		start_sword(source, actor, opponent, selected, count);
	}
	return FALSE; // Preparing a defense cannot suppress damage already arriving.
}

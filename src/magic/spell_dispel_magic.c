#include "core/prototypes.h"
#include "cmd/interp.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "combat/damage.h"
#include "combat/spell_wards.h"
#include "magic/spells.h"
#include "item/objmisc.h"
#include "world/vnum.obj.h"
#include "world/specs.prototypes.h"
#include "world/events.h"
#include "net/gmcp.h"

#include <algorithm>
#include <set>
#include <vector>

extern P_index obj_index;
extern P_room world;
extern int top_of_objt;
extern const int top_of_world;
extern const int rev_dir[];
extern Skill skills[];

namespace
{
// Duration wear: ten percent remaining, rounded up. Ordinary
// affects and walls use a tick minimum; short-lived portals use one pulse.
int dispel_shortened_duration(int remaining, int minimum_wear)
{
	const int wear = std::max(minimum_wear, remaining / 10 + (remaining % 10 != 0));
	return std::max(0, remaining - wear);
}

enum class duration_wear_result
{
	unchanged,
	weakened,
	exhausted
};

duration_wear_result wear_spell_duration(P_char victim, struct affected_type *af)
{
	P_nevent timer = NULL;
	int remaining = af->duration;
	const bool short_affect = IS_SET(af->flags, AFFTYPE_SHORT);
	if (short_affect)
	{
		LOOP_EVENTS_CH(timer, victim->nevents)
		{
			if (timer->func == event_short_affect && timer->data &&
			    static_cast<event_short_affect_data *>(timer->data)->af == af)
			{
				remaining = ne_event_time(timer);
				break;
			}
		}
	}
	if (remaining < 0)
		return duration_wear_result::unchanged;
	const int shortened =
		dispel_shortened_duration(remaining, short_affect ? PULSES_IN_TICK : 1);
	if (shortened == 0)
		return duration_wear_result::exhausted;
	if (timer && !nevent_reschedule_after(nevent_handle_from_event(timer), shortened))
		return duration_wear_result::unchanged;
	if (short_affect && !timer)
	{
		event_short_affect_data data = { victim, af };
		add_event(event_short_affect, shortened, victim, NULL, NULL, 0, &data,
			  sizeof(data));
	}
	af->duration = shortened;
	return duration_wear_result::weakened;
}

bool dispel_morph(P_char caster, P_char victim, int spell)
{
	if (spell != SPELL_CALL_OF_THE_WILD || !IS_MORPH(victim))
		return false;
	P_char original = victim->only.npc->orig_char;
	act("$n suddenly changes shape, reforming into $N.", FALSE, victim, NULL, original,
	    TO_NOTVICT);
	send_to_char("You suddenly feel yourself going back to normal.\n", victim);
	send_to_char("and you succeed!\n", caster);
	act("and succeeds!", FALSE, caster, NULL, victim, TO_NOTVICT);
	un_morph(victim);
	return true;
}

void announce_spell_wear(P_char caster, P_char victim, int spell, bool exhausted)
{
	const char *name = spell > 0 && spell < MAX_SKILLS && skills[spell].name ?
				   skills[spell].name :
				   "magic";
	const char *outcome = exhausted ? "exhausts" : "weakens";
	const char *ending = exhausted ? "." : ", shortening its remaining duration.";
	char message[256];
	snprintf(message, sizeof(message), "&+YYour dispel %s $N's %.120s%s&n", outcome, name,
		 ending);
	act(message, FALSE, caster, NULL, victim, TO_CHAR);
	snprintf(message, sizeof(message), "&+Y$n's dispel %s your %.120s%s&n", outcome, name,
		 ending);
	act(message, FALSE, caster, NULL, victim, TO_VICT);
	snprintf(message, sizeof(message), "&+Y$n's dispel %s $N's %.120s.&n", outcome, name);
	act(message, FALSE, caster, NULL, victim, TO_NOTVICT);
}

P_obj opposite_dispel_wall(P_obj wall)
{
	if (!OBJ_ROOM(wall) || wall->loc.room < 0 || wall->loc.room > top_of_world)
		return NULL;
	const int room = wall->loc.room;
	const int direction = wall->value[1];
	if (direction < 0 || direction >= static_cast<int>(ARRAY_SIZE(world[room].dir_option)))
		return NULL;
	const auto exit = world[room].dir_option[direction];
	if (!exit || exit->to_room < 0 || exit->to_room > top_of_world)
		return NULL;
	for (P_obj other = world[exit->to_room].contents; other; other = other->next_content)
	{
		if (other != wall && other->R_num == wall->R_num &&
		    other->value[0] == world[room].number &&
		    other->value[1] == rev_dir[direction] && other->value[3] == wall->value[3] &&
		    other->value[5] == wall->value[5])
			return other;
	}
	return NULL;
}

P_nevent dispel_decay_timer(P_obj obj)
{
	if (!obj)
		return NULL;
	P_nevent timer;
	LOOP_EVENTS_OBJ(timer, obj->nevents)
	{
		if (timer->func == event_obj_affect && timer->data)
		{
			const auto af = *static_cast<obj_affect **>(timer->data);
			if (af && af->type == TAG_OBJ_DECAY)
				return timer;
		}
	}
	return NULL;
}

int paired_decay_remaining(P_nevent timer, P_nevent other_timer)
{
	int remaining = timer ? ne_event_time(timer) : -1;
	if (other_timer)
		remaining = remaining < 0 ? ne_event_time(other_timer) :
					    std::min(remaining, ne_event_time(other_timer));
	return remaining;
}

void announce_object_wear(P_char caster, P_obj obj, bool exhausted)
{
	act(exhausted ? "&+YYour dispel exhausts $p's magic.&n" :
			"&+YYour dispel weakens $p, shortening its remaining duration.&n",
	    FALSE, caster, obj, NULL, TO_CHAR);
	act(exhausted ? "&+Y$n's dispel exhausts $p's magic.&n" :
			"&+Y$n's dispel weakens $p, shortening its remaining duration.&n",
	    FALSE, caster, obj, NULL, TO_ROOM);
}

P_obj opposite_dispel_portal(P_obj portal)
{
	if (!OBJ_ROOM(portal) || portal->loc.room < 0 || portal->loc.room > top_of_world)
		return NULL;
	const int destination = real_room(portal->value[0]);
	if (destination < 0 || destination > top_of_world)
		return NULL;
	P_obj counterpart = NULL;
	for (P_obj other = world[destination].contents; other; other = other->next_content)
	{
		if (other == portal || other->R_num != portal->R_num ||
		    other->value[7] != portal->value[7] || !OBJ_ROOM(other) ||
		    other->loc.room != destination ||
		    other->value[0] != world[portal->loc.room].number)
			continue;
		// An ambiguous pair must not select an unrelated portal arbitrarily.
		if (counterpart)
			return NULL;
		counterpart = other;
	}
	return counterpart;
}

void dispel_portal(P_char caster, P_obj portal)
{
	// These are character-level gates, as in the original portal callbacks.
	if (GET_LEVEL(caster) < 46)
	{
		act("$p easily resists your assault!", FALSE, caster, portal, NULL, TO_CHAR);
		return;
	}
	P_obj other = opposite_dispel_portal(portal);
	if (!other)
	{
		act("Your magic cannot reach the other side of $p.", FALSE, caster, portal, NULL,
		    TO_CHAR);
		return;
	}
	if (GET_LEVEL(caster) >= 50 || number(0, 1))
	{
		act("&+YYour magic dispels $p.&n", FALSE, caster, portal, NULL, TO_CHAR);
		act("&+Y$n's magic dispels $p.&n", FALSE, caster, portal, NULL, TO_ROOM);
		Decay(other);
		Decay(portal);
		return;
	}
	P_nevent timer = dispel_decay_timer(portal);
	P_nevent other_timer = dispel_decay_timer(other);
	const int remaining = paired_decay_remaining(timer, other_timer);
	if (remaining < 0)
	{
		act("$p resists your assault!", FALSE, caster, portal, NULL, TO_CHAR);
		return;
	}
	const int shortened = dispel_shortened_duration(remaining, 1);
	if (shortened > 0)
	{
		if (timer && !nevent_reschedule_after(nevent_handle_from_event(timer), shortened))
			return;
		if (other_timer &&
		    !nevent_reschedule_after(nevent_handle_from_event(other_timer), shortened))
			return;
	}
	announce_object_wear(caster, portal, shortened == 0);
	if (shortened == 0)
	{
		Decay(other);
		Decay(portal);
	}
}

bool dispel_stone_anchor(P_char caster, P_obj obj)
{
	P_nevent timer = dispel_decay_timer(obj);
	if (!timer)
	{
		// Conjured anchors are finite. Retain the old one-pulse repair for
		// a missing decay affect, and repair a missing event on an existing one.
		if (auto af = get_obj_affect(obj, TAG_OBJ_DECAY))
			add_event(event_obj_affect, 1, NULL, NULL, obj, 0, &af, sizeof(af));
		else
			set_obj_affected(obj, 1, TAG_OBJ_DECAY, 0);
		announce_object_wear(caster, obj, false);
		return false;
	}
	const int remaining = ne_event_time(timer);
	// Preserve the anchor's existing one-tick cap without extending an
	// earlier deadline. A nearly expired anchor still receives partial wear.
	const int shortened = std::min(PULSES_IN_TICK, dispel_shortened_duration(remaining, 1));
	if (shortened == 0)
	{
		announce_object_wear(caster, obj, true);
		Decay(obj);
		return true;
	}
	if (nevent_reschedule_after(nevent_handle_from_event(timer), shortened))
		announce_object_wear(caster, obj, false);
	return false;
}

enum class object_dispel_result
{
	not_applicable,
	handled,
	continue_enchantment
};

object_dispel_result dispel_custom_object(P_char caster, P_obj obj)
{
	const auto special = obj_index[obj->R_num].func.obj;
	if (special == portal_door || special == portal_wormhole || special == portal_etherportal)
	{
		dispel_portal(caster, obj);
		// Rejection is handled too: never fall through to ordinary item destruction.
		return object_dispel_result::handled;
	}
	if (special == moonstone && (OBJ_VNUM(obj) == 419 || OBJ_VNUM(obj) == 433))
	{
		// Anchors deliberately also used the ordinary enchantment path.
		return dispel_stone_anchor(caster, obj) ?
			       object_dispel_result::handled :
			       object_dispel_result::continue_enchantment;
	}
	return object_dispel_result::not_applicable;
}

typedef struct
{
	ulong bit;
	int spell;
} RemoveableSpellBit;

static RemoveableSpellBit mobAffects[] = {
	{ AFF_MINOR_GLOBE, SPELL_MINOR_GLOBE },
	{ AFF_BIOFEEDBACK, SPELL_BIOFEEDBACK },
	{ AFF_STONE_SKIN, SPELL_STONE_SKIN },
	{ AFF_INFERNAL_FURY, SPELL_INFERNAL_FURY },
	{ AFF_FREEDOM_OF_MVMNT, SPELL_FREEDOM_OF_MOVEMENT },
	{ AFF_INVISIBLE, SPELL_CONCEALMENT },
	{ AFF_DETECT_INVISIBLE, SPELL_DETECT_INVISIBLE },
	{ AFF_HASTE, SPELL_HASTE },
	{ AFF_ARMOR, SPELL_ARMOR },
	{ AFF_SLEEP, SPELL_SLEEP },
	{ AFF_BARKSKIN, SPELL_BARKSKIN },
	{ AFF_LEVITATE, SPELL_LEVITATE },
	{ AFF_FLY, SPELL_FLY },
};

static RemoveableSpellBit mobAffects2[] = {
	{ AFF2_MINOR_PARALYSIS, SPELL_MINOR_PARALYSIS },
	{ AFF2_MAJOR_PARALYSIS, SPELL_MAJOR_PARALYSIS },
	{ AFF2_GLOBE, SPELL_GLOBE },
	{ AFF2_PASSDOOR, SPELL_MOLECULAR_CONTROL },
};

static RemoveableSpellBit mobAffects3[] = {
	{ AFF3_ECTOPLASMIC_FORM, SPELL_ECTOPLASMIC_FORM },
	{ AFF3_SPIRIT_WARD, SPELL_SPIRIT_WARD },
	{ AFF3_GR_SPIRIT_WARD, SPELL_GREATER_SPIRIT_WARD },
	{ AFF3_INERTIAL_BARRIER, SPELL_INERTIAL_BARRIER },
	{ AFF3_TOWER_IRON_WILL, SPELL_TOWER_IRON_WILL },
	{ AFF3_BLUR, SPELL_BLUR },
};

static RemoveableSpellBit mobAffects4[] = {
	{ AFF4_STORNOGS_SPHERES, SPELL_STORNOGS_SPHERES },
	{ AFF4_STORNOGS_GREATER_SPHERES, SPELL_STORNOGS_GREATER_SPHERES },
	{ AFF4_BATTLE_ECSTASY, SPELL_BATTLE_ECSTASY },
	{ AFF4_DAZZLER, SPELL_DAZZLE },
	{ AFF4_DEFLECT, SPELL_DEFLECT },
	{ AFF4_HAWKVISION, SPELL_HAWKVISION },
	{ AFF4_SANCTUARY, SPELL_SANCTUARY },
	{ AFF4_HELLFIRE, SPELL_HELLFIRE },
};

static RemoveableSpellBit mobAffects5[] = {
	{ AFF5_FLESH_ARMOR, SPELL_FLESH_ARMOR },
	{ AFF5_THORNSKIN, SPELL_THORNSKIN },
};

static int CheckMobRemoveableSpellBits(P_char ch, RemoveableSpellBit *spellBits, int countSpellBits,
				       ulong *bitStore, bool nosave, int saveMod, int affectVector)
{
	int success = 0;

	if (!IS_NPC(ch))
	{
		return success;
	}

	for (int i = 0; i < countSpellBits; i++)
	{
		if (IS_SET(*bitStore, spellBits[i].bit) &&
		    !affected_by_spell(ch, spellBits[i].spell) &&
		    (nosave || !NewSaves(ch, SAVING_SPELL, saveMod)))
		{
			success = 1;
			if (!IS_ELITE(ch))
			{
				REMOVE_BIT(*bitStore, spellBits[i].bit);
			}
			else
			{
				struct affected_type *paf = NULL;
				if (!affected_by_spell(ch, TAG_SUPPRESS_PERM_BITS))
				{
					add_tag_to_char(
						ch, TAG_SUPPRESS_PERM_BITS, 0,
						AFFTYPE_SHORT | AFFTYPE_NODISPEL,
						WAIT_SEC *
							get_property(
								"timer.secs.suppressElitePermBits",
								30));
					paf = get_spell_from_char(ch, TAG_SUPPRESS_PERM_BITS);
				}
				else
				{
					paf = get_spell_from_char(ch, TAG_SUPPRESS_PERM_BITS);
				}

				ulong *paffectBits = NULL;
				switch (affectVector)
				{
				case 1:
					paffectBits = &paf->bitvector;
					break;
				case 2:
					paffectBits = &paf->bitvector2;
					break;
				case 3:
					paffectBits = &paf->bitvector3;
					break;
				case 4:
					paffectBits = &paf->bitvector4;
					break;
				case 5:
					paffectBits = &paf->bitvector5;
					break;
				default:
					logit(LOG_EXIT,
					      "Invalid affect vector in CheckMobRemoveableSpellBits");
					return success;
				}
				// add the suppressed bits
				SET_BIT(*paffectBits, spellBits[i].bit);
			}
		}
	}

	return success;
}
} // namespace

void spell_dispel_magic(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			P_char victim, P_obj obj)
{
	int mod, success = 0, nosave = 0;
	bool weakened_spell = false;

	if (!IS_ALIVE(ch))
	{
		return;
	}

	/* victim target... */
	if (victim)
	{
		/*
		 * no save when cast or on consenting target
		 */

		if (ch == victim && level < 57 && !IS_TRUSTED(ch))
		{
			send_to_char("You dispel the very spell you are casting!\n", ch);
			return;
		}

		if (ch == victim || is_linked_to(ch, victim, LNK_CONSENT) || IS_TRUSTED(ch))
		{
			nosave = 1;
		}

		mod = GET_LEVEL(ch) - GET_LEVEL(victim);
		if (IS_NPC(ch) && IS_PC(victim) && mod > 0)
		{
			mod /= 3;
		}

		act("$n tries to dispel your magic!", FALSE, ch, 0, victim, TO_VICT);
		act("You try to dispel $N's magic.", FALSE, ch, 0, victim, TO_CHAR);
		act("$n tries to dispel $N's magic!", FALSE, ch, 0, victim, TO_NOTVICT);
		spell_ward_sync_timers(victim);
		spell_ward_equipment_sync(victim);

		// One check per ordinary spell, and one per active ward source. Snapshot
		// identities because removing a multi-affect spell can unlink later rows.
		std::vector<struct affected_type *> pending;
		std::set<int> checked_spells;
		for (auto af = victim->affected; af; af = af->next)
			pending.push_back(af);
		for (auto candidate : pending)
		{
			auto af = victim->affected;
			while (af && af != candidate)
				af = af->next;
			if (!af)
				continue;

			if (spell_ward_is_managed(af))
			{
				if (!spell_ward_is_active(af))
					continue;
				const bool saved = !nosave &&
						   NewSaves(victim, SAVING_SPELL,
							    IS_ELITE(ch) ? mod + 5 : mod);
				const bool resisted = !nosave && !saved &&
						      resists_spell(ch, victim);
				const bool dispelled = !saved && !resisted;
				// Burning Hands is the comparable second-circle packet. Bound
				// legacy signed spell levels before rolling the same base dice.
				const int wear_level = static_cast<int>(std::clamp(
					level < 0 ? -static_cast<long long>(level) : level, 1LL,
					255LL));
				const int wear = dispelled ? 0 : 4 * dice(5 + wear_level / 10, 6);
				const auto result =
					spell_ward_dispel(ch, victim, af, dispelled, wear);
				if (result == spell_ward_dispel_result::broken)
					success = 1;
				else if (result == spell_ward_dispel_result::weakened)
					weakened_spell = true;
				continue;
			}

			if (!IS_SET(af->flags, AFFTYPE_NODISPEL) && af->type > 0 &&
			    checked_spells.insert(af->type).second)
			{
				const bool saved = !nosave &&
						   NewSaves(victim, SAVING_SPELL,
							    IS_ELITE(ch) ? mod + 5 : mod);
				const bool resisted = !nosave && !saved &&
						      resists_spell(ch, victim);
				if (!saved && !resisted)
				{
					success = 1;
					wear_off_message(victim, af);
					if (dispel_morph(ch, victim, af->type))
						return;
					affect_from_char(victim, af->type);
				}
				else
				{
					const int spell = af->type;
					bool shortened = false;
					for (auto part = victim->affected; part;)
					{
						auto next = part->next;
						if (part->type == spell &&
						    !IS_SET(part->flags, AFFTYPE_NODISPEL))
						{
							const auto result =
								wear_spell_duration(victim, part);
							shortened |=
								result !=
								duration_wear_result::unchanged;
							if (result ==
							    duration_wear_result::exhausted)
							{
								wear_off_message(victim, part);
								if (dispel_morph(ch, victim, spell))
									return;
								affect_remove(victim, part);
								success = 1;
							}
						}
						part = next;
					}
					if (shortened)
					{
						weakened_spell = true;
						announce_spell_wear(ch, victim, spell,
								    !affected_by_spell(victim,
										       spell));
						gmcp_char_affects(victim);
					}
				}
			}
		}

		success = CheckMobRemoveableSpellBits(victim, mobAffects, ARRAY_SIZE(mobAffects),
						      &victim->specials.affected_by, nosave, mod,
						      1) == 0 ?
				  success :
				  1;
		success = CheckMobRemoveableSpellBits(victim, mobAffects2, ARRAY_SIZE(mobAffects2),
						      &victim->specials.affected_by2, nosave, mod,
						      2) == 0 ?
				  success :
				  1;
		success = CheckMobRemoveableSpellBits(victim, mobAffects3, ARRAY_SIZE(mobAffects3),
						      &victim->specials.affected_by3, nosave, mod,
						      3) == 0 ?
				  success :
				  1;
		success = CheckMobRemoveableSpellBits(victim, mobAffects4, ARRAY_SIZE(mobAffects4),
						      &victim->specials.affected_by4, nosave, mod,
						      4) == 0 ?
				  success :
				  1;
		success = CheckMobRemoveableSpellBits(victim, mobAffects5, ARRAY_SIZE(mobAffects5),
						      &victim->specials.affected_by5, nosave, mod,
						      5) == 0 ?
				  success :
				  1;

		if (!success && !weakened_spell)
		{
			send_to_char("&+Land you fail miserably...\n", ch);
			act("&+Land fails miserably...&n", FALSE, ch, 0, victim, TO_NOTVICT);
		}
		else if (success)
		{
			send_to_char("&+Yand you have some &+Gsuccess!\r\n", ch);
			act("&+Yand $e has some &+Gsuccess!&n", FALSE, ch, 0, victim, TO_NOTVICT);
		}

		if (!nosave &&
		    (IS_AFFECTED(ch, AFF_INVISIBLE) || IS_AFFECTED2(ch, AFF2_CONCEALMENT)))
			appear(ch);

		if (!nosave && IS_NPC(victim) && CAN_SEE(victim, ch))
		{
			remember(victim, ch);
			if (!IS_FIGHTING(victim))
				MobStartFight(victim, ch);
		}
		balance_affects(victim);
		return;
	}
	/* okay.. must be an object target! */
	if (!obj || obj->R_num < 0 || obj->R_num > top_of_objt)
		return;

	const auto custom = dispel_custom_object(ch, obj);
	if (custom == object_dispel_result::handled)
		return;
	if (custom == object_dispel_result::not_applicable && obj_index[obj->R_num].func.obj &&
	    invoke_object_special(obj, ch, CMD_DISPEL, NULL))
		return;

	/* second deal with "special" objects (conjurerer wall spells) */
	if (obj->R_num == real_object(VOBJ_WALLS))
	{
		const int wall_level = static_cast<int>(
			std::clamp(level < 0 ? -static_cast<long long>(level) : level, 1LL, 255LL));
		const int dispel_damage = obj->value[3] == WALL_OUTPOST ?
						  1 :
						  number(std::max(1, wall_level / 4), wall_level);
		P_obj other = opposite_dispel_wall(obj);
		const int strength = other ? std::min(obj->value[2], other->value[2]) :
					     obj->value[2];
		P_nevent timer = dispel_decay_timer(obj);
		P_nevent other_timer = dispel_decay_timer(other);
		const int remaining = paired_decay_remaining(timer, other_timer);
		const int shortened =
			remaining < 0 ? -1 : dispel_shortened_duration(remaining, PULSES_IN_TICK);
		const bool dispelled = (number(0, 5) > (obj->value[4] - wall_level)) ||
				       IS_TRUSTED(ch) ||
				       (IS_PC(ch) && obj->value[5] == GET_PID(ch)) ||
				       (IS_NPC(ch) && obj->value[5] == GET_RNUM(ch));
		if (dispelled || dispel_damage >= strength || shortened == 0)
		{
			act(dispelled ? "&+YYour magic dispels $p.&n" :
					"&+YYour dispel breaks $p.&n",
			    FALSE, ch, obj, NULL, TO_CHAR);
			act(dispelled ? "&+Y$n's magic dispels $p.&n" :
					"&+Y$n's dispel breaks $p.&n",
			    FALSE, ch, obj, NULL, TO_ROOM);
			if (other)
				Decay(other);
			Decay(obj);
		}
		else
		{
			obj->value[2] = strength - dispel_damage;
			if (other)
				other->value[2] = obj->value[2];
			if (timer)
				nevent_reschedule_after(nevent_handle_from_event(timer), shortened);
			if (other_timer)
				nevent_reschedule_after(nevent_handle_from_event(other_timer),
							shortened);
			act(timer || other_timer ?
				    "&+YYour dispel weakens $p, shortening its remaining duration.&n" :
				    "&+YYour dispel weakens $p.&n",
			    FALSE, ch, obj, NULL, TO_CHAR);
			act(timer || other_timer ?
				    "&+Y$n's dispel weakens $p, shortening its remaining duration.&n" :
				    "&+Y$n's dispel weakens $p.&n",
			    FALSE, ch, obj, NULL, TO_ROOM);
		}
		return;
	}

	/* casting is on a "normal" object.. only has an effect if the
	   object has a MAGIC flag! */
	if (IS_SET(obj->extra2_flags, ITEM2_MAGIC))
	{
		if (IS_ARTIFACT(obj))
		{
			send_to_char("&+GIt is impossible to dispel an artifact!\r\n", ch);
			return;
		}

		/* Preserve the existing one-in-five chance of destroying the object. */
		if (!number(0, 4) && !IS_TRUSTED(ch))
		{
			send_to_char("&+YUh oh!&N\n", ch);

			Decay(obj);
		}
		else if ((number(25, 59) < level) || IS_TRUSTED(ch))
		{
			act("&+bThe magic aura around&n $p&+b fades.", TRUE, ch, obj, 0, TO_CHAR);
			REMOVE_BIT(obj->extra2_flags, ITEM2_MAGIC);
			obj->affected[0].location = APPLY_NONE;
			obj->affected[0].modifier = 0;
			obj->affected[1].location = APPLY_NONE;
			obj->affected[1].modifier = 0;
		}
	}
}

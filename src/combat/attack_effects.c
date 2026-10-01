/* Attack effects and proc dispatch shared by hit(). */
#include "combat/attack_effects.h"
#include "cmd/interp.h"
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utility.h"
#include "core/utils.h"
#include "combat/attack_continuation.h"
#include "combat/damage.h"
#include "economy/economic_gameplay_authority.h"
#include "item/forced_weapon_drop.h"
#include "item/native_artifact_actions.h"
#include "item/objmisc.h"
#include "item/weapon_actions.h"
#include "magic/spells.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include <stdio.h>
#include <string.h>

extern Skill skills[];

struct attack_hit_type attack_hit_text[] = {
	{ "punch", "punches", "punched" }, /* TYPE_HIT      */
	{ "bludgeon", "bludgeons", "bludgeoned" }, /* TYPE_BLUDGEON */
	{ "pierce", "pierces", "pierced" }, /* TYPE_PIERCE   */
	{ "slash", "slashes", "slashed" }, /* TYPE_SLASH    */
	{ "whip", "whips", "whipped" }, /* TYPE_WHIP     */
	{ "claw", "claws", "clawed" }, /* TYPE_CLAW     */
	{ "bite", "bites", "bitten" }, /* TYPE_BITE     */
	{ "sting", "stings", "stung" }, /* TYPE_STING    */
	{ "crush", "crushes", "crushed" }, /* TYPE_CRUSH    */
	{ "maul", "mauls", "mauled" }, /* TYPE_MAUL     */
	{ "thrash", "thrashes", "thrashed" } /* TYPE_THRASH   */
};

static bool refresh_tainted_blade_pair(P_char &ch, P_char &victim, uint64_t ch_runtime_id,
				       uint64_t victim_runtime_id)
{
	ch = find_character_by_runtime_id(ch_runtime_id);
	victim = find_character_by_runtime_id(victim_runtime_id);
	return ch && IS_ALIVE(ch) && victim && IS_ALIVE(victim);
}

void event_tainted_blade(P_char ch, P_char victim, P_obj /*obj*/, void * /*data*/)
{
	int blade_skill;
	uint64_t ch_runtime_id;
	uint64_t victim_runtime_id;
	struct affected_type *af;
	struct damage_messages tainted_messages = {
		"$N &+Lstruggles against the &+wtaint &+Lcoursing through $S body.&n",
		"Beads of sweat form on your forehead as you battle the taint.",
		"$N &+Lstruggles against the &+wtaint &+Lcoursing through $S body.&n",
		"$N &+Lfalls to the ground thrashing wildly, $S &+wsoul &+Lfinally de&+wvo&+wu&+wr&+Led.",
		"&+LYour screams ar&+we your onl&+Wy thing ke&+weping you compa&+Lny as you fall into oblivion.&n",
		"$N &+Lfalls to the ground thrashing wildly, $S soul finally devoured."
	};
	struct damage_messages holy_messages = {
		"&+rThe wound burns with a &+wsoft white glow.",
		"&+rYour wound &+yburns &+rlike it was on &+Rfire.",
		"$N's &+rwound burns with a &+wsoft white glow.",
		"$N &+Lfalls to the ground thrashing wildly, $S &+wsoul &+Lfinally de&+wvo&+wu&+wr&+Led.",
		"&+LYour screams ar&+we your onl&+Wy thing ke&+weping you compa&+Lny as you fall into oblivion.&n",
		"$N &+Lfalls to the ground thrashing wildly, $S soul finally devoured."
	};
	struct damage_messages *messages;

	if (!char_in_list(ch) || !IS_ALIVE(ch) || !char_in_list(victim) || !IS_ALIVE(victim))
		return;
	ch_runtime_id = ch->runtime_id;
	victim_runtime_id = victim->runtime_id;
	blade_skill = GET_CLASS(ch, CLASS_AVENGER) ? SKILL_HOLY_BLADE : SKILL_TAINTED_BLADE;
	messages = GET_CLASS(ch, CLASS_AVENGER) ? &holy_messages : &tainted_messages;

	af = get_spell_from_char(victim, blade_skill);
	if (!af)
		return;

	// At 56, Min = MAX(40, 3) = 10dam, Simplified Avg = MAX(40, 57) = 14dam, Max = MAX(40, 112) = 28dam
	//  Note: The real average is more complicated since for all X in Y: Y=dice(3, (2*lvl)/3) < 40: X = 40 or whatever.
	if (raw_damage(ch, victim, MAX(40, dice(3, (2 * GET_LEVEL(ch)) / 3)),
		       RAWDAM_DEFAULT ^ RAWDAM_IMPRISON, messages) != DAM_NONEDEAD)
		return;
	if (!refresh_tainted_blade_pair(ch, victim, ch_runtime_id, victim_runtime_id))
		return;
	af = get_spell_from_char(victim, blade_skill);
	if (!af)
		return;

	if (af->modifier-- > 0)
	{
		add_event(event_tainted_blade,
			  (int)(IS_AFFECTED(victim, AFF_SLOW_POISON) ? 1.5 : 1) * PULSE_VIOLENCE,
			  ch, victim, 0, 0, 0, 0);
	}
	else
	{
		affect_remove(victim, af);
	}
}

bool tainted_blade(P_char ch, P_char victim)
{
	int blade_skill;
	uint64_t ch_runtime_id;
	uint64_t victim_runtime_id;
	struct affected_type af, *old_af;
	struct damage_messages *messages;
	int dam_result;

	if (!char_in_list(ch) || !IS_ALIVE(ch) || !char_in_list(victim) || !IS_ALIVE(victim))
		return TRUE;
	ch_runtime_id = ch->runtime_id;
	victim_runtime_id = victim->runtime_id;
	blade_skill = GET_CLASS(ch, CLASS_AVENGER) ? SKILL_HOLY_BLADE : SKILL_TAINTED_BLADE;

	if (IS_CONSTRUCT(victim) || !ch->equipment[WIELD])
		return FALSE;

	struct damage_messages tainted_messages = {
		"$N &+wpales &+Las your &+wtainted &+Lweapon strikes&n $M.",
		"&+LYou &+rscream &+Las&n $n's&+L $q slams into your body.&n",
		"$N &+wpales &+Land &+wshivers &+Las&n $n's &+Ltainted weapon strikes&n $M.&n",
		"$N &+Lfalls to the ground &+wspasming &+Las the &+wtaint &+Lconsumes $S soul.&n",
		"&+LAs the &+wfoul taint &+Lfills your every fiber you feel your hold on &+wlife &+Lslipping away&+w...&n",
		"$N &+Lfalls to the ground &+wspasming &+Las the &+wtaint &+Lconsumes $S soul.&n",
		0,
		ch->equipment[WIELD]
	};
	struct damage_messages holy_messages = {
		"&+WThe holy aura surrounding $q burns into&n $N.",
		"&+WSeering pain flows through you as you are struck by&n $n's $q.",
		"$n's $q &+yglows &+Wbright white&n as it strikes $N.",
		"&+WThe holy aura surrounding $q burns into&n $N.",
		"&+RSeering pain flows through you as you are struck by&n $n's $q.",
		"$n's $q &+yglows &+Wbright white as it strikes&n $N.",
		0,
		ch->equipment[WIELD]
	};
	if (GET_CLASS(ch, CLASS_AVENGER))
		messages = &holy_messages;
	else
		messages = &tainted_messages;

	dam_result = raw_damage(ch, victim, 60, RAWDAM_DEFAULT, messages);
	if (dam_result == DAM_NONEDEAD)
	{
		if (!refresh_tainted_blade_pair(ch, victim, ch_runtime_id, victim_runtime_id))
			return TRUE;
		if ((old_af = get_spell_from_char(victim, blade_skill)))
		{
			old_af->modifier = 1 + GET_CHAR_SKILL(ch, blade_skill) / 33;
			return FALSE;
		}
		memset(&af, 0, sizeof(af));
		af.type = blade_skill;
		af.duration = 1;
		af.modifier = 1 + GET_CHAR_SKILL(ch, blade_skill) / 33;
		affect_to_char(victim, &af);
		add_event(event_tainted_blade, PULSE_VIOLENCE, ch, victim, 0, 0, 0, 0);
	}
	else
	{
		return TRUE;
	}

	return FALSE;
}

/* The caller lends its three message buffers; damage_messages itself only ever
   points at immutable text.  msg_size is the caller's real buffer size: this
   function used to format with MAX_STRING_LENGTH into hit()'s 512-byte
   buffers. */
int anatomy_strike(P_char ch, P_char victim, int msg, struct damage_messages *messages,
		   char *attacker_msg, char *victim_msg, char *room_msg, size_t msg_size, int dam)
{
	int skl = GET_CHAR_SKILL(ch, SKILL_ANATOMY);
	struct affected_type af;

	memset(&af, 0, sizeof(af));
	af.type = SKILL_ANATOMY;
	af.flags = AFFTYPE_NOSHOW | AFFTYPE_NODISPEL | AFFTYPE_SHORT;

	if (IS_CONSTRUCT(victim))
		goto regular;

	switch (number(0, 6))
	{
	case 0:
		if (IS_HUMANOID(victim))
		{
			snprintf(attacker_msg, msg_size,
				 "Your%%s %s hits $N on the torso making $M grimace in pain.",
				 attack_hit_text[msg].singular);
			snprintf(victim_msg, msg_size,
				 "$n's%%s %s hits $N on the torso making $M grimace in pain.",
				 attack_hit_text[msg].singular);
			snprintf(room_msg, msg_size,
				 "$n's%%s %s hits $N on the torso making $M grimace in pain.",
				 attack_hit_text[msg].singular);
			messages->type = DAMMSG_HIT_EFFECT;
		}
		return (int)(dam * 1.1);
	case 1:
		if (!LEGLESS(victim))
		{
			snprintf(attacker_msg, msg_size,
				 "Your%%s %s hits $N across the leg, resulting in a limp stride.",
				 attack_hit_text[msg].singular);
			snprintf(victim_msg, msg_size,
				 "$n's%%s %s hits $N across the leg, resulting in a limp stride.",
				 attack_hit_text[msg].singular);
			snprintf(room_msg, msg_size,
				 "$n's%%s %s hits $N across the leg, resulting in a limp stride.",
				 attack_hit_text[msg].singular);
			messages->type = DAMMSG_HIT_EFFECT;
			GET_VITALITY(victim) -= 5;
		}
		return dam;
	case 2:
		act("You surprise $N with a blow to the back, leaving $M momentarily confused.",
		    FALSE, ch, 0, victim, TO_CHAR);
		act("$n surprises you with a blow to the back, leaving you momentarily confused.",
		    FALSE, ch, 0, victim, TO_VICT);
		act("$n surprises $N with a blow to $S back, leaving $M momentarily confused.",
		    FALSE, ch, 0, victim, TO_NOTVICT);
		victim->specials.combat_tics = (int)victim->specials.base_combat_round;
		goto regular;
	case 3:
		if (IS_HUMANOID(victim))
		{
			snprintf(attacker_msg, msg_size,
				 "Your%%s %s reached $N's arm severing tendons and muscles.",
				 attack_hit_text[msg].singular);
			snprintf(victim_msg, msg_size,
				 "$n's%%s %s reached your arm severing tendons and muscles.",
				 attack_hit_text[msg].singular);
			snprintf(room_msg, msg_size,
				 "$n's%%s %s reached $N's arm severing tendons and muscles.",
				 attack_hit_text[msg].singular);
			messages->type = DAMMSG_HIT_EFFECT;
			af.duration = victim->specials.combat_tics + 1;
			af.modifier = -10 - skl / 10;
			af.location = APPLY_DAMROLL;
			affect_to_char(victim, &af);
		}
		return dam;
	case 4:
		if (IS_HUMANOID(ch))
		{
			act("You strike viciously at $N's wrist causing $M to swing about madly.",
			    FALSE, ch, 0, victim, TO_CHAR);
			act("$n strikes viciously at your wrist causing you to swing about madly.",
			    FALSE, ch, 0, victim, TO_VICT);
			act("$n strikes viciously at $N's wrist causing $M to swing about madly.",
			    FALSE, ch, 0, victim, TO_NOTVICT);
			af.duration = victim->specials.combat_tics + 1;
			af.modifier = -15 - skl / 10;
			af.location = APPLY_HITROLL;
			affect_to_char(victim, &af);
		}
		goto regular;
	case 5:
		if (IS_CASTING(victim) && IS_HUMANOID(victim))
		{
			act("You viciously strike at $N's face, causing them to lose their concentration!",
			    FALSE, ch, 0, victim, TO_CHAR);
			act("$n viciously strikes at your face, causing you to lose your concentration!",
			    FALSE, ch, 0, victim, TO_VICT);
			act("$n viciously strikes at $N's face, causing them to lose their concentration!",
			    FALSE, ch, 0, victim, TO_NOTVICT);
			StopCasting(victim);
		}
		goto regular;
	case 6:
		if (IS_HUMANOID(victim))
		{
			snprintf(attacker_msg, msg_size,
				 "Your%%s %s reached $N's ear causing a gush of blood.",
				 attack_hit_text[msg].singular);
			snprintf(victim_msg, msg_size,
				 "$n's%%s %s reached your ear causing a gush of blood.",
				 attack_hit_text[msg].singular);
			snprintf(room_msg, msg_size,
				 "$n's%%s %s reached $N's ear causing a gush of blood.",
				 attack_hit_text[msg].singular);
			messages->type = DAMMSG_HIT_EFFECT;
			af.duration = 10 * PULSE_VIOLENCE;
			af.bitvector4 = AFF4_DEAF;
			affect_to_char(victim, &af);
		}
		goto regular;
	default:
		goto regular;
	}

regular:

	snprintf(attacker_msg, msg_size, "Your%%s %s %%s.", attack_hit_text[msg].singular);
	snprintf(victim_msg, msg_size, "$n's%%s %s %%s.", attack_hit_text[msg].singular);
	snprintf(room_msg, msg_size, "$n's%%s %s %%s.", attack_hit_text[msg].singular);
	messages->type = DAMMSG_HIT_EFFECT | DAMMSG_TERSE;

	return dam;
}

bool weapon_proc(P_obj obj, P_char ch, P_char victim)
{
	if (item_restricted_for_player_pet(ch, obj))
		return FALSE;
	struct extra_descr_data *ex;
	int spells[3];
	int room;
	int count;
	// These native state machines use values[5..7] for state, not packed spells.
	// A retained positive legacy energy value must not select the generic path.
	if ((OBJ_VNUM(obj) == 21 || OBJ_VNUM(obj) == 22) && native_artifact_owns(OBJ_VNUM(obj)))
	{
		if (obj_index[obj->R_num].func.obj)
			return invoke_object_special(obj, ch, CMD_MELEE_HIT, (char *)victim);
		return FALSE;
	}

	if (!obj->value[5] || obj->value[7] <= 0)
	{
		if (obj_index[obj->R_num].func.obj != NULL)
		{
			return invoke_object_special(obj, ch, CMD_MELEE_HIT, (char *)victim);
		}
		else
		{
			return FALSE;
		}
	}

	// int test = number(0, obj->value[7] - 1);
	// debug( "Val7: %d, Val7-1: %d, Test: %d", obj->value[7], obj->value[7]-1, test );
	if (number(0, obj->value[7] - 1))
		return FALSE;
	if (selected_packed_weapon_action(obj, ch, victim) != item_action_start::legacy)
		return TRUE;

	for (ex = obj->ex_description; ex; ex = ex->next)
		if (isname("_char_msg", ex->keyword))
			act(ex->description, FALSE, ch, obj, victim, TO_CHAR | ACT_NOEOL);
		else if (isname("_victim_msg", ex->keyword))
			act(ex->description, FALSE, ch, obj, victim, TO_VICT | ACT_NOEOL);
		else if (isname("_room_msg", ex->keyword))
			act(ex->description, FALSE, ch, obj, victim, TO_NOTVICT | ACT_NOEOL);

	count = 0;
	room = ch->in_room;
	if ((spells[0] = obj->value[5] % 1000))
		count++;
	if ((spells[1] = obj->value[5] % 1000000 / 1000))
		count++;
	if ((spells[2] = obj->value[5] % 1000000000 / 1000000))
		count++;

	if (!count)
		return FALSE;

	if (obj->value[5] > 999999999)
	{
		count = number(0, count - 1);
		if (skills[spells[count]].spell_pointer)
		{
			if (IS_AGG_SPELL(spells[count]))
			{
				((*skills[spells[count]].spell_pointer)(
					(int)obj->value[6], ch, 0, SPELL_TYPE_SPELL, victim, obj));
			}
			else if (!affected_by_spell(ch, spells[count]))
			{
				((*skills[spells[count]].spell_pointer)((int)obj->value[6], ch, 0,
									SPELL_TYPE_SPELL, ch, obj));
			}
		}
	}
	else
	{
		while (count-- && is_char_in_room(ch, room) && is_char_in_room(victim, room))
		{
			if (skills[spells[count]].spell_pointer)
			{
				const attack_continuation continuation =
					begin_attack_continuation(ch, victim, obj);
				if (IS_AGG_SPELL(spells[count]))
				{
					((*skills[spells[count]].spell_pointer)(
						(int)obj->value[6], ch, 0, SPELL_TYPE_SPELL, victim,
						obj));
				}
				else if (!affected_by_spell(ch, spells[count]))
				{
					((*skills[spells[count]].spell_pointer)(
						(int)obj->value[6], ch, 0, SPELL_TYPE_SPELL, ch,
						obj));
				}
				const attack_continuation_result after_spell =
					check_attack_continuation(continuation);
				if (!after_spell.can_continue())
					return TRUE;
				ch = after_spell.actor;
				victim = after_spell.target;
				obj = after_spell.weapon;
			}
		}
	}
	return TRUE;
}

int battle_frenzy(P_char ch, P_char victim)
{
	int dam;
	struct damage_messages messages1 = {
		"You slam your knee into $N's stomach, winding $M.&N",
		"$n knee's you right in the stomach, knocking the wind out of you.&N",
		"$n knee's $N's stomach, knocking the wind out of $M.&N",
		"As you slam your knee into $N's stomach &+Rblood&n pours from $S mouth.&N",
		"As $n's knee hits your stomach, you spit &+Rblood&n, cough and die.&N",
		"As $n knee's $N's stomach, &+Rblood&n pours from $S mouth.&N"
	};
	struct damage_messages messages2 = {
		"You jam your elbow into $N's side, bruising $M!&N",
		"$n jams $s elbow in your side!&N",
		"$n elbows $N hard, bruising $S side!&N",
		"Your elbow jams into $N's side, breaking $S ribs and crushing $S heart!&n",
		"$n jams $s elbow in your side, breaking your ribs and crushing your heart!&N",
		"$n elbows $N hard, breaking $S ribs and crushing $S heart!&N"
	};

	dam = GET_DAMROLL(ch) * GET_LEVEL(ch) * number(1, 2) / 40;
	if (chance_to_hit(ch, victim, GET_CHAR_SKILL(ch, SKILL_UNARMED_DAMAGE), 0) > number(0, 100))
	{
		if (number(0, 1))
			return melee_damage(ch, victim, dam, PHSDAM_TOUCH, &messages1);
		else
			return melee_damage(ch, victim, dam, PHSDAM_TOUCH, &messages2);
	}
	else
	{
		act("$n attempts to knee you right in the stomach, but came up short.&N", TRUE, ch,
		    NULL, victim, TO_VICT);
		act("$n tries to knee $N, but can't quite reach.&N", TRUE, ch, NULL, victim,
		    TO_NOTVICT);
		act("You attempt to knee $N, but don't quite make it.&N", TRUE, ch, NULL, victim,
		    TO_CHAR);
	}
	return 0;
}

bool monk_critic(P_char ch, P_char victim, int *damAccumulator)
{
	struct affected_type aff, *af;
	uint64_t ch_runtime_id;
	uint64_t victim_runtime_id;
	struct damage_messages messages = {
		"$N screams as you sink five fingers into soft spots in $S shoulder.",
		"You feel on fire as $n's hard fingers strike a nerve in your shoulder.",
		"$N screams as $n sinks five fingers into soft spots in $S shoulder.",
		"$N dies as you sink five fingers into soft spots in $S shoulder.",
		"You feel on fire as $n's hard fingers strike a nerve in your shoulder.",
		"$N dies as $n sinks five fingers into soft spots in $S shoulder."
	};

	if (!char_in_list(ch) || !char_in_list(victim))
		return TRUE;
	if (!IS_ALIVE(ch) || !IS_ALIVE(victim) || IS_CONSTRUCT(victim))
		return FALSE;
	ch_runtime_id = ch->runtime_id;
	victim_runtime_id = victim->runtime_id;

	send_to_char("You sneak in and deliver a strike to a pressure point!\r\n", ch);

	if (GET_SPEC(ch, CLASS_MONK, SPEC_WAYOFSNAKE) ||
	    (GET_CLASS(ch, CLASS_MONK) && IS_NPC(ch) && GET_LEVEL(ch) > 50))
	{
		af = get_spell_from_char(victim, TAG_PRESSURE_POINTS);
		if (!af)
		{
			memset(&aff, 0, sizeof(aff));
			aff.type = TAG_PRESSURE_POINTS;
			aff.flags = AFFTYPE_SHORT | AFFTYPE_NOSHOW | AFFTYPE_NODISPEL |
				    AFFTYPE_NOAPPLY;
			aff.modifier = 1;
			aff.duration = (10 * WAIT_SEC);
			affect_to_char(victim, &aff);
			return FALSE;
		}

		af->modifier++;

		if (af->modifier == 2)
		{
			if (!IS_AFFECTED2(victim, AFF2_SLOW))
			{
				memset(&aff, 0, sizeof(aff));
				aff.type = SPELL_SLOW;
				aff.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL;
				aff.duration = (4 * WAIT_SEC);
				aff.bitvector2 = AFF2_SLOW;
				affect_to_char(victim, &aff);

				act("&+m$n &+mbegins to sllooowwww down.&n", TRUE, victim, 0, 0,
				    TO_ROOM);
				send_to_char("&+mYou feel yourself slowing down.\r\n", victim);
			}
		}

		if (af->modifier == 3 && !IS_BLIND(victim))
			blind(ch, victim, (4 * WAIT_SEC));

		if (af->modifier == 4)
		{
			CharWait(victim, (3 * WAIT_SEC));
			act("$n strikes you hard at the side of the neck.", TRUE, ch, 0, victim,
			    TO_VICT);
			act("$n deals a crippling blow to the side of $N's neck.", TRUE, ch, 0,
			    victim, TO_NOTVICT);
			act("You deal a crippling blow to the side of $N's neck.", TRUE, ch, 0,
			    victim, TO_CHAR);
		}

		if (af->modifier == 5)
		{
			if (DAM_NONEDEAD != melee_damage(ch, victim, 240 + dice(4, 40),
							 PHSDAM_TOUCH | RAWDAM_DEFAULT, &messages,
							 damAccumulator))
			{
				return TRUE;
			}
			ch = find_character_by_runtime_id(ch_runtime_id);
			victim = find_character_by_runtime_id(victim_runtime_id);
			if (!ch || !IS_ALIVE(ch) || !victim || !IS_ALIVE(victim))
				return TRUE;
			af = get_spell_from_char(victim, TAG_PRESSURE_POINTS);
			if (!af)
				return FALSE;
		}
		if (af->modifier == 6)
			affect_from_char(ch, TAG_PRESSURE_POINTS);
	}
	return FALSE;
}

bool critical_attack(P_char ch, P_char victim, int msg)
{
	char attacker_msg[MAX_STRING_LENGTH];
	char victim_msg[MAX_STRING_LENGTH];
	char room_msg[MAX_STRING_LENGTH];
	int random;

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim) || GET_RACE(victim) == RACE_CONSTRUCT)
		return FALSE;

	random = number(0, 3);

	if (random == 0)
	{
		if (!IS_HUMANOID(victim))
		{ // Falls through.
			random = 2;
		}
		else
		{
			snprintf(
				attacker_msg, MAX_STRING_LENGTH,
				"Your attack penetrates $N's defense and strikes to the &+Wbone!&n&N");
			snprintf(victim_msg, MAX_STRING_LENGTH,
				 "$n's attack causes you to gush &+Rblood!&n&N");
			act(victim_msg, TRUE, ch, NULL, victim, TO_VICT);
			snprintf(room_msg, MAX_STRING_LENGTH,
				 "$N's body &+yquivers&n as $n's hit strikes deep!&N");
			act(room_msg, TRUE, ch, NULL, victim, TO_NOTVICT);

			if (GET_VITALITY(victim) > 20)
				victim->points.vitality = (int)(victim->points.vitality * 0.75);

			return TRUE;
		}
	}
	if (random == 1 && !number(0, 2))
	{
		snprintf(room_msg, MAX_STRING_LENGTH,
			 "$n's mighty %s knocks $N's weapon from $S grasp!&n",
			 attack_hit_text[msg].singular);
		snprintf(attacker_msg, MAX_STRING_LENGTH,
			 "Your mighty %s knocks $N's weapon from $S grasp!&n",
			 attack_hit_text[msg].singular);
		snprintf(victim_msg, MAX_STRING_LENGTH,
			 "$n's mighty %s knocks your weapon from your grasp!&n",
			 attack_hit_text[msg].plural);
		if (critical_disarm(ch, victim))
		{
			act(attacker_msg, TRUE, ch, NULL, victim, TO_CHAR);
			act(victim_msg, TRUE, ch, NULL, victim, TO_VICT);
			act(room_msg, TRUE, ch, NULL, victim, TO_NOTVICT);
		}
		else
		{
			return FALSE;
		}

		return TRUE;
	}
	if (random == 2)
	{
		if (affected_by_spell(victim, SPELL_STONE_SKIN))
		{
			snprintf(attacker_msg, MAX_STRING_LENGTH,
				 "Your mighty attack shatters $N's magical protection!&N");
			act(attacker_msg, TRUE, ch, NULL, victim, TO_CHAR);

			snprintf(victim_msg, MAX_STRING_LENGTH,
				 "The magic protecting your body shatters as $n %s you!&N",
				 attack_hit_text[msg].plural);
			act(victim_msg, TRUE, ch, NULL, victim, TO_VICT);

			snprintf(room_msg, MAX_STRING_LENGTH,
				 "$n grins slightly as their %s drops $N's defenses!&N",
				 attack_hit_text[msg].singular);
			act(room_msg, TRUE, ch, NULL, victim, TO_NOTVICT);

			affect_from_char(victim, SPELL_STONE_SKIN);
		}
		else if (affected_by_spell(victim, SPELL_BIOFEEDBACK))
		{
			snprintf(attacker_msg, MAX_STRING_LENGTH,
				 "Your mighty attack shatters $N's magical protection!&N");
			act(attacker_msg, TRUE, ch, NULL, victim, TO_CHAR);

			snprintf(victim_msg, MAX_STRING_LENGTH,
				 "The magic protecting your body shatters as $n %s you!&N",
				 attack_hit_text[msg].plural);
			act(victim_msg, TRUE, ch, NULL, victim, TO_VICT);

			snprintf(room_msg, MAX_STRING_LENGTH,
				 "$n grins slightly as their %s drops $N's defenses!&N",
				 attack_hit_text[msg].singular);
			act(room_msg, TRUE, ch, NULL, victim, TO_NOTVICT);

			affect_from_char(victim, SPELL_BIOFEEDBACK);
		}
		else if (affected_by_spell(victim, SPELL_SHADOW_SHIELD))
		{
			snprintf(attacker_msg, MAX_STRING_LENGTH,
				 "Your mighty attack shatters $N's magical protection!&N");
			act(attacker_msg, TRUE, ch, NULL, victim, TO_CHAR);

			snprintf(victim_msg, MAX_STRING_LENGTH,
				 "The magic protecting your body shatters as $n %s you!&N",
				 attack_hit_text[msg].plural);
			act(victim_msg, TRUE, ch, NULL, victim, TO_VICT);

			snprintf(room_msg, MAX_STRING_LENGTH,
				 "$n grins slightly as $s %s drops $N's defenses!&N",
				 attack_hit_text[msg].singular);
			act(room_msg, TRUE, ch, NULL, victim, TO_NOTVICT);

			affect_from_char(victim, SPELL_SHADOW_SHIELD);
		}

		return TRUE;
	}

	if (random == 3)
	{
		snprintf(attacker_msg, MAX_STRING_LENGTH,
			 "You follow up your %s with a surprise attack!&N",
			 attack_hit_text[msg].singular);
		act(attacker_msg, TRUE, ch, NULL, victim, TO_CHAR);

		snprintf(victim_msg, MAX_STRING_LENGTH,
			 "$n swiftly follows up his devastating %s with a surprise attack!&N",
			 attack_hit_text[msg].singular);
		act(victim_msg, TRUE, ch, NULL, victim, TO_VICT);

		snprintf(
			room_msg, MAX_STRING_LENGTH,
			"$n uses the momentum of $s previous strike against $N to land another attack!&N");
		hit(ch, victim, ch->equipment[SECONDARY_WEAPON]);

		return TRUE;
	}
	return FALSE;
}

bool critical_disarm(P_char ch, P_char victim)
{
	if (!ch || !victim)
		return FALSE;

	P_obj obj = NULL;
	int pos = 0;
	int weapon_positions[] = { WIELD, WIELD2, WIELD3, WIELD4, -1 };

	for (int i = 0; weapon_positions[i] >= 0; i++)
	{
		pos = weapon_positions[i];
		obj = victim->equipment[pos];

		if (obj && obj->type == ITEM_WEAPON)
			break;
	}

	if (!obj || obj->type != ITEM_WEAPON || IS_SET(obj->extra_flags, ITEM_NODROP))
		return FALSE;

	if (!IS_ARTIFACT(obj) &&
	    number(1, 100) < get_property("skill.criticalAttack.disarm.dropChance", 5))
	{
		return forced_weapon_drop(victim, obj, forced_weapon_drop_cause::critical_disarm) !=
		       forced_weapon_drop_result::rejected;
	}
	if (economic_gameplay_authority::active() && IS_PC(victim))
		return FALSE;

	obj = unequip_char(victim, pos);
	if (!obj)
		return FALSE;
	obj_to_char(obj, victim);

	return TRUE;
}

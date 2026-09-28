/*
 * ***************************************************************************
 * *  File: magic.c                                            Part of Duris *
 * *  Usage: procedures to create spell affects
 * * *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 * * *  Copyright 1994 - 2008 - Duris Systems Ltd.
 * *
 * ***************************************************************************
 */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "world/achievements.h"
#include "guild/alliances.h"
#include "guild/assocs.h"
#include "combat/ctf.h"
#include "combat/damage.h"
#include "core/defines.h"
#include "classes/disguise.h"
#include "world/graph.h"
#include "combat/grapple.h"
#include "world/hardcore_config.h"
#include "guild/guildhall.h"
#include "combat/justice.h"
#include "combat/training_dummy.h"
#include "world/map.h"
#include "core/mm.h"
#include "classes/necromancy.h"
#include "persistence/persistence_checkpoint.h"
#include "item/objmisc.h"
#include "item/item_movement_transaction.h"
#include "item/item_ownership_runtime.h"
#include "magic/spell_item_lifecycle.h"
#include "economy/economic_gameplay_authority.h"
#include <array>
#include "kingdom/kingdom_store_piece.h"
#include "world/outposts.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"
#include "magic/spell_words_of_power.h"
#include "sql/sql.h"
#include "world/vnum.obj.h"
#include "world/weather.h"
#include "core/safe_format.h"

/*
 * external variables
 */

extern const char *command[];
extern Skill skills[];
extern char *spells[];
extern P_index obj_index;
extern P_char character_list;
extern P_obj object_list;
extern P_desc descriptor_list;
extern P_char combat_list;
extern P_obj object_list;
extern P_room world;
extern P_index mob_index;
extern const char *apply_types[];
extern const flagDef extra_bits[];
extern const flagDef anti_bits[];
extern const flagDef affected1_bits[];
extern const flagDef affected2_bits[];
extern const flagDef affected3_bits[];
extern const flagDef affected4_bits[];
extern const flagDef affected5_bits[];
extern const char *item_types[];
extern const struct stat_data stat_factor[];
// extern int rev_dir[];
extern int avail_hometowns[][LAST_RACE + 1];
extern int guild_locations[][CLASS_COUNT + 1];
extern int spl_table[TOTALLVLS][MAX_CIRCLE];
extern int hometown[];
extern const int top_of_world;
extern struct str_app_type str_app[];
extern struct con_app_type con_app[];
extern struct time_info_data time_info;
extern struct wis_app_type wis_app[];
extern struct zone_data *zone_table;
extern struct sector_data *sector_table;
extern const struct race_names race_names_table[];
extern const char *carve_part_name[];
extern struct mm_ds *dead_mob_pool;
extern struct mm_ds *dead_pconly_pool;
extern const struct golem_description golem_data[];
extern float exp_mods[EXPMOD_MAX + 1];
extern bool is_dragoon_mounted(P_char ch);
extern bool is_dragoon_mount(P_char mount);
extern bool is_in_dragoon_group(P_char ch, P_char vict);
extern int get_next_dragoon_circle(P_char ch);
extern P_char get_dragoon_mount(P_char ch);
extern void do_point(P_char ch, P_char victim);
extern bool has_skin_spell(P_char ch);

// THE NEXT PERSON THAT OUTRIGHT COPIES A SPELL JUST TO CHANGE THE NAME/MESSAGES
// IT OUTPUTS IS GOING TO BE CASTRATED BY ME AND FORCED TO EAT THEIR OWN GENITALIA.
// There is no reason to do this other than to make a headache for another coder.
// If you feel the need to have a "different" spell than one already in the game
// for racewar purposes or whatever, MAKE CHANGES TO THE ORIGINAL SPELL and call
// THAT SPELL with a command in interp.c.  There is no reason to have 3253232 different
// functions for the exact same spell(transmute/ethereal grounds etc.) - Jexni 3/28/11

void affect_to_end(P_char ch, struct affected_type *af);

void do_nothing_spell(int /*level*/, P_char /*ch*/, char * /*arg*/, int /*type*/, P_char /*victim*/,
		      P_obj /*obj*/)
{
	return;
}

// New function for spell components - Lucrot 31Aug2008
int get_spell_component(P_char ch, int vnum, int max_components)
{
	P_obj t_obj, next_obj;
	int found = 0;
	if (economic_gameplay_authority::active() && ch && IS_PC(ch))
		return 0;

	for (t_obj = ch->carrying; t_obj && found < max_components; t_obj = next_obj)
	{
		next_obj = t_obj->next_content;
		if (obj_index[t_obj->R_num].virtual_number == vnum)
		{
			extract_obj(t_obj, TRUE); // Spell components shouldn't be artis.
			found++;
		}
	}
	return found;
}

namespace
{
struct spell_component_retirement_context
{
	item_movement_completion_fn continuation = nullptr;
	std::array<uint64_t, 8> item_uids = {};
	uint32_t continuation_context_size = 0;
	uint8_t item_count = 0;
	std::array<uint8_t, 48> continuation_context = {};
};

static_assert(sizeof(spell_component_retirement_context) <=
	      ITEM_MOVEMENT_CONTEXT_MAX_BYTES);

P_obj spell_component_by_uid(uint64_t item_uid)
{
	for (P_obj object = object_list; object; object = object->next)
		if (object->obj_uid == item_uid)
			return object;
	return NULL;
}

void spell_component_retirement_completed(P_char actor, bool committed,
					  const item_transfer_result &result,
					  unsigned int error_code, const uint8_t *encoded,
					  size_t encoded_size)
{
	if (!encoded || encoded_size != sizeof(spell_component_retirement_context))
		return;
	spell_component_retirement_context context = {};
	memcpy(&context, encoded, sizeof(context));
	if (committed)
		for (size_t index = 0; index < context.item_count; ++index)
			if (P_obj item = spell_component_by_uid(context.item_uids[index]))
				extract_obj(item);
	if (context.continuation)
		context.continuation(actor, committed, result, error_code,
				     context.continuation_context.data(),
				     context.continuation_context_size);
}
} // namespace

bool spell_consume_components(P_char actor, int vnum, size_t max_components,
			     uint32_t reason_id, item_movement_completion_fn continuation,
			     const void *continuation_context, size_t continuation_context_size)
{
	if (!actor || !IS_PC(actor) || GET_PID(actor) <= 0 || vnum < 0 || !max_components ||
	    max_components > 8 || !reason_id || !continuation ||
	    continuation_context_size > 48 ||
	    (continuation_context_size && !continuation_context) ||
	    !economic_gameplay_authority::active())
		return false;

	P_obj selected[8] = {};
	size_t selected_count = 0;
	for (P_obj object = actor->carrying; object && selected_count < max_components;
	     object = object->next_content)
		if (obj_index[object->R_num].virtual_number == vnum)
			selected[selected_count++] = object;
	if (!selected_count)
		return false;

	spell_component_retirement_context context = {};
	context.continuation = continuation;
	context.continuation_context_size = static_cast<uint32_t>(continuation_context_size);
	context.item_count = static_cast<uint8_t>(selected_count);
	if (continuation_context_size)
		memcpy(context.continuation_context.data(), continuation_context,
		       continuation_context_size);
	for (size_t index = 0; index < selected_count; ++index)
		context.item_uids[index] = selected[index]->obj_uid;

	const item_owner_identity owner = { item_owner_type::player,
					    static_cast<uint64_t>(GET_PID(actor)), 0 };
	const item_owner_identity destruction = { item_owner_type::destruction, 0, 0 };
	item_movement_reject reject = item_movement_reject::none;
	return item_movement_transaction_submit_batch(
		actor, selected, selected_count, NULL, owner, destruction,
		item_transfer_reason::destruction, static_cast<int64_t>(reason_id),
		spell_component_retirement_completed, &context, sizeof(context), NULL, &reject,
		nullptr, economic_source_kind::item_action);
}

/*
 * Offensive Spells
 */

// Old edrain below. -Lucrot Jul09
// Shows exactly why Lucrot shouldn't have been touching code... seriously, nice commenting.
/*
 * Drain XP, MANA, HP - caster gains HP and MANA
 */
// void spell_energy_drain(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
// {
// int xp, mana, dam;
// struct damage_messages messages = {
// "You drain $N of some of $S energy.",
// "You feel less energetic as $n drains you.",
// "$n drains $N - what a waste of energy!",
// "$N crumples as you kill $M by draining $S energy.",
// "As $n drains your last bit of energy, you look forward to the peace of the graveyard.",
// "$n drains the energy of $N who crumbles into a lifeless husk."
// };

// if( !IS_ALIVE(ch) || !IS_ALIVE(victim) || victim == ch )
// {
// return;
// }

// if(resists_spell(ch, victim))
// {
// return;
// }

//  GET_ALIGNMENT(ch) = MAX(-1000, GET_ALIGNMENT(ch) - 2);

// dam = (int) ((level * 2.5) + number(-10, 10));

// if(IS_PC(ch) &&
// !(GET_CLASS(ch, CLASS_NECROMANCER | CLASS_ANTIPALADIN)))
// {
// dam /= 4;
// }

// if(IS_AFFECTED4(victim, AFF4_DEFLECT))
// {
// if(GET_LEVEL(ch) >= 50)
// {
// dam <<= 1;
// }

// spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, 0, &messages);
// return;
// }

// if(saves_spell(victim, SAVING_SPELL))
// {
// dam /= 2;
// }

//  GET_ALIGNMENT(ch) = MAX(-1000, GET_ALIGNMENT(ch) - 4);
// if(GET_LEVEL(victim) <= 2)
// {
// /*
// * Kill the sucker
// */
// act(messages.death_attacker, FALSE, ch, 0, victim, TO_CHAR);
// act(messages.death_victim, FALSE, ch, 0, victim, TO_VICT);
// act(messages.death_room, FALSE, ch, 0, victim, TO_NOTVICT);
// die(victim, ch);
// victim = NULL;
// }
// else
// {
// xp = GET_LEVEL(ch) * 1000;
// xp = MIN(xp, GET_EXP(victim));

// if(GET_LEVEL(ch) >= 50)
// {
// dam <<= 1;
// }

// if(!IS_AFFECTED4(victim, AFF4_NEG_SHIELD))
// {
// mana = MIN(GET_MANA(victim), 100);
// GET_MANA(victim) -= mana;
// StartRegen(victim, regen_resource::mana);
// GET_MANA(ch) += mana >> 1;
// }

// if(IS_AFFECTED4(victim, AFF4_NEG_SHIELD))
// {
// vamp(ch, (int)(dam / 6), (int) (GET_MAX_HIT(ch) * 1.25));
// }
// else
// {
// vamp(ch, (int)(dam / 2), (int) (GET_MAX_HIT(ch) * 1.25));
// }

// if(GET_LEVEL(ch) <= 50)
// {
// send_to_char("&+LYour life energy is drained!\n",
// victim);
// }
// else
// {
// send_to_char("&+LYour life energy is &+rtapped&+L.\n",
// victim);
// }

// if(GET_VITALITY(victim) >= 10 &&
// !IS_AFFECTED4(victim, AFF4_NEG_SHIELD))
// {
// GET_VITALITY(victim) = MAX(10, GET_VITALITY(victim) - 15);
// GET_VITALITY(ch) += 10;
// }

// StartRegen(ch, regen_resource::vitality);
// StartRegen(victim, regen_resource::vitality);

// spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, SPLDAM_NOSHRUG,
// &messages);
// }
// }

/* The conjure_terrain_check() function checks if it is a specialized conjurer  */
/* trying to conjure on terrain appropriate for the elemental type, and handles */
/* appropriate messages. Bonuses/maluses themselves are attributed in the       */
/* conjour_elemental() and conjure_specialized() functions.                     */
/* Return values from this function are:                                        */
/* -1 - bad terrain for the elemental type                                      */
/* 0 - neutral terrain                                                          */
/* 1 - good terrain for the conjurer elemental type                             */

/*
 * cast_as_damage_area passes pointer to the index of hit victim as arg
 */

// ch and victim is backwards so disarm will work right.
/*
 * spells2.c - Not directly offensive spells
 */

/* Seeya!
void spell_healing_blade(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  struct affected_type af;
  P_obj    wpn;

  if(affected_by_spell(ch, SPELL_HEALING_BLADE))
  {
    send_to_char("You are already on &+Rfire!!\n", ch);
    return;
  }

  if(!(wpn = ch->equipment[WIELD]) || !IS_SWORD(wpn))
  {
    send_to_char("You need to be wielding a slashing weapon!\n", ch);
    return;
  }

  bzero(&af, sizeof(af));

  af.type = SPELL_HEALING_BLADE;
  af.duration = level;
  af.location = APPLY_HITROLL;
  af.modifier = number(1, 4);

  affect_to_char(victim, &af);

  send_to_char("&+BA blue aura covers your blade with a healing power!&n\n",
               ch);

}
*/

/* OLD HEAL - 21 Sep 08 -Lucrot
{
   int      healpoints = 100;


  spell_cure_blind(level, ch, NULL, SPELL_TYPE_SPELL, victim, obj);
  if(GET_HIT(victim) > GET_MAX_HIT(victim))
    return;

  heal(victim, ch, healpoints, GET_MAX_HIT(victim) - number(1, 4));
  update_pos(victim);
  if(IS_RACEWAR_UNDEAD(victim))
    send_to_char("&+WYou feel the powers of darkness strengthen you!\n",
                 victim);
  else
    send_to_char("&+WA warm feeling fills your body.\n", victim);
  grapple_heal(victim);

}
*/

void spell_ventriloquate(int /*level*/, P_char /*ch*/, char * /*arg*/, int /*type*/,
			 P_char /*victim*/, P_obj /*obj*/) {
	/*
	 * Not possible!! No argument!
	 */
}

/* void spell_vigorize_light(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  int      movepoints;

  movepoints = number(4, 15);

  if((movepoints + GET_VITALITY(victim)) > GET_MAX_VITALITY(victim))
    GET_VITALITY(victim) = GET_MAX_VITALITY(victim);
  else
    GET_VITALITY(victim) += movepoints;

  update_pos(victim);

  send_to_char("You feel a bit more invigorated!\n", victim);
} */

/* void spell_vigorize_serious(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj obj)
{
  int      movepoints;

  movepoints = dice(3, (level / 3));

  if((movepoints + GET_VITALITY(victim)) > GET_MAX_VITALITY(victim))
    GET_VITALITY(victim) = GET_MAX_VITALITY(victim);
  else
    GET_VITALITY(victim) += movepoints;

  send_to_char("You feel much more invigorated!\n", victim);

  update_pos(victim);
} */

/* void spell_vigorize_critic(int level, P_char ch, char *arg, int type,
                           P_char victim, P_obj obj)
{
  int      movepoints;

  movepoints = (level / 2) + dice(4, (level / 4));

  if((movepoints + GET_VITALITY(victim)) > GET_MAX_VITALITY(victim))
    GET_VITALITY(victim) = GET_MAX_VITALITY(victim);
  else
    GET_VITALITY(victim) += movepoints;

    act("",
       FALSE, ch, 0, 0, TO_ROOM);
    act
      ("&+WThe orb of light begins to take shape... a mounting sense of awe grips the area.",
       FALSE, ch, 0, 0, TO_CHAR);

  send_to_char("You feel invigorated!\n", victim);
} */

/*
void event_plague(P_char ch, P_char vict, P_obj obj, void *data)
{
  P_char target;
  int timer, both = FALSE;
  struct affected_type af;

  if(!affected_by_spell(ch, SPELL_PLAGUE) && !IS_AFFECTED4(ch, AFF4_CARRY_PLAGUE))
    return;

  if(affected_by_spell(ch, SPELL_PLAGUE) && affected_by_spell(ch, SPELL_DISEASE))
  {
    act("$n's &+rin&+Rfec&+rtio&+Rus w&+roun&+Rds&n blister and pop spreading the &+Gplague&n everywhere!", TRUE, ch, 0, 0, TO_ROOM);
    act("Your &+rin&+Rfec&+rtio&+Rus w&+roun&+Rds&n blister and pop spreading the &+Gplague&n everywhere!", TRUE, ch, 0, 0, TO_CHAR);
    both = TRUE;
  }

  if(affected_by_spell(ch, SPELL_PLAGUE) && !affected_by_spell(ch, SPELL_DISEASE) &&
      !IS_AFFECTED4(ch, AFF4_CARRY_PLAGUE))
  {
    spell_disease(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, ch, 0);
  }

  for (target = world[ch->in_room].people; target; target = target->next_in_room)
  {
    if(IS_TRUSTED(target))
      continue;
    if(IS_UNDEADRACE(target))
      continue;

    if(!affected_by_spell(target, SPELL_PLAGUE) &&
        number(0, 100) < ((int)get_property("spell.plague.spread.perc", 30)+(both ? 30 : 0)))
    {
      bzero(&af, sizeof(af));
      af.type = SPELL_PLAGUE;
      af.duration = (int)get_property("spell.plague.duration", 10);
      af.modifier = 500;
      affect_to_char(target, &af);

      if(!get_scheduled(target, event_plague))
        add_event(event_plague, WAIT_SEC * 1, target, 0, 0, 0, 0, 0);
    }
  }

  timer = (int)get_property("spell.plague.eventTime", 60);
  if(affected_by_spell(ch, SPELL_DISEASE))
    timer -= 20;
  timer += number(-10, 10);
  if(IS_FIGHTING(ch))
    timer -= 30;
  if(timer < 5)
    timer = 5;

  add_event(event_plague, WAIT_SEC * timer, ch, 0, 0, 0, 0, 0);
}

*/

/*
void spell_windstrom_blessing(int level, P_char ch, char *arg, int type,
                              P_char victim, P_obj obj)
{
  struct affected_type af;

  if(affected_by_spell(ch, SPELL_WINDSTROM_BLESSING))
  {
    send_to_char("&+CWindstrom&n has blessed your blade already!\n", ch);
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_WINDSTROM_BLESSING;
  af.duration = 5;
  affect_to_char(ch, &af);
}
*/

/*
 * ***************************************************************************
 * *                     NPC spells..
 * * *
 * *************************************************************************
 */

;

// end spell_feeblemind

#ifndef __GRAPPLE_H__
#define __GRAPPLE_H__

#include "structs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "config.h"

/*
 * Affects definitions:
 * SKILL_GRAPPLER_COMBAT- Applied to grappler to use as timer before he can grapple again.
 * TAG_GRAPPLE- Applied to the grappler (can not attack per HOLD_CANT_ATTACK()).
 * SKILL_BEARHUG- Applied to person being held in bearhug (can not attack).
 * TAG_BEARHUG- Applied to person being held in bearhug (can still attack).
 * SKILL_HEADLOCK- Applied to person being held in headlock.
 *  -modifier is set to define HOLD_NORMAL or HOLD_IMPROVED.
 * TAG_HEADLOCK- Applied to person who is knocked out from improved headlock.
 * SKILL_ARMLOCK- Applied to person who is being held in armlock.
 *  -modifier is set to define ARMREFLEX_HOLD or ARMREFLEX_OFFBALANCE
 *  -attacks are reduced by half per fight.c
 * TAG_ARMLOCK- Applied to a person who's arm has been broken.
 *  -attacks are reduced by half per fight.c
 *  -affect is removed by a magical heal type spell
 * SKILL_LEGLOCK- Applied to a person who's in a leglock.
 *  -modifier is set to HOLD_NORMAL or HOLD_IMPROVED.
 * TAG_LEGLOCK- Applied to a person who's leg is broken.
 *  -movement cost is increased by 15.
 * SKILL_GROUNDSLAM- Applied to victim who's been groundslammed.
 *  -Purpose of applying this is to allow for combination into an
 *   improved leglock.
 *
 * Define Definitions:
 * IS_GRAPPLED()- checks if the ch is affected with ANY hold.
 * HOLD_CANT_ATTACK()- Checks if the character is affected with
 *  a hold they can't attack with.
 * IS_*()- Checks if the ch is affected with TAG_* or SKILL_*.
 * IS_S*()- Checks if the ch is affected with SKILL_*.
 * IS_T*()- Checks if the ch is affected with TAG_*.
 *
 * Hold definitions:
 * Bearhug- Hugs the victim, chance to prevent victim from attacking
 *  -main combination opener.
 * Headlock- Grabs the victims head, prevents and stops casting.
 *  -combination into allows chance to knock victim out form lack of air.
 * Armlock- A reflexive skill allowing ch to grab vict's arm.  The proc
 *   reduces attacks by half for the duration.  You can either get your
 *   arm locked, broken, or you will be thrown off balance instead.
 *  -combination into allows chance to break the arm reducing attacks by
 *   half untill he receives a heal type spell.
 * Leglock- ch and victim need to be !POS_STANDING.  This is a hold type
 *   affect that can lag the ch and victim.
 *  -combination into allows chance to break the leg increasing
 *   movement cost by 15.
 * Groundslam- Usable only as combination into from Bearhug or
 *   Armlock (ARMREFLEX_OFFBALANCE).  Places the ch and victim in
 *   POS_PRONE and CharWait's for a duration, opens up combination
 *   into leglock.
 *
 * Current combinations:
 * Bearhug -> Headlock
 * Bearhug -> Groundslam -> Leglock
 * Armlock (ARMREFLEX_OFFBALANCE) -> Groundslam -> Leglock
 */

#define HOLD_NONE     0
#define HOLD_NORMAL   1
#define HOLD_IMPROVED 2

#define ARMREFLEX_HOLD       1
#define ARMREFLEX_OFFBALANCE 2
#define ARMREFLEX_MAX        2

#define IS_GRAPPLED(ch)                                                                                                                                                                                \
	(affected_by_spell(ch, TAG_GRAPPLE) || affected_by_spell(ch, SKILL_BEARHUG) || affected_by_spell(ch, TAG_BEARHUG) || affected_by_spell(ch, SKILL_HEADLOCK) ||                                      \
	 affected_by_spell(ch, SKILL_ARMLOCK) || affected_by_spell(ch, SKILL_LEGLOCK))

#define GRAPPLE_TIMER(ch) (affected_by_spell(ch, SKILL_GRAPPLER_COMBAT))

#define HOLD_CANT_ATTACK(ch) (affected_by_spell(ch, TAG_GRAPPLE) || affected_by_spell(ch, SKILL_BEARHUG))

#define IS_TGRAPPLE(ch) affected_by_spell(ch, TAG_GRAPPLE)

#define IS_BEARHUG(ch)  (affected_by_spell(ch, SKILL_BEARHUG) || affected_by_spell(ch, TAG_BEARHUG))
#define IS_SBEARHUG(ch) affected_by_spell(ch, SKILL_BEARHUG)
#define IS_TBEARHUG(ch) affected_by_spell(ch, TAG_BEARHUG)

#define IS_HEADLOCK(ch)  (affected_by_spell(ch, SKILL_HEADLOCK) || affected_by_spell(ch, TAG_HEADLOCK))
#define IS_SHEADLOCK(ch) affected_by_spell(ch, SKILL_HEADLOCK)
#define IS_THEADLOCK(ch) affected_by_spell(ch, TAG_HEADLOCK)

#define IS_ARMLOCK(ch)  (affected_by_spell(ch, SKILL_ARMLOCK) || affected_by_spell(ch, TAG_ARMLOCK))
#define IS_SARMLOCK(ch) affected_by_spell(ch, SKILL_ARMLOCK)
#define IS_TARMLOCK(ch) affected_by_spell(ch, TAG_ARMLOCK)

#define IS_GROUNDSLAM(ch) affected_by_spell(ch, SKILL_GROUNDSLAM)

#define IS_LEGLOCK(ch)  (affected_by_spell(ch, SKILL_LEGLOCK) || affected_by_spell(ch, TAG_LEGLOCK))
#define IS_SLEGLOCK(ch) affected_by_spell(ch, SKILL_LEGLOCK)
#define IS_TLEGLOCK(ch) affected_by_spell(ch, TAG_LEGLOCK)

int    grapple_check_entrapment(P_char ch);
int    grapple_check_hands(P_char ch);
int    grapple_flee_check(P_char ch);
P_char grapple_attack_check(P_char ch);
int    grapple_misfire_chance(P_char ch, P_char victim, int type);
void   do_bearhug(P_char ch, char *argument, int cmd);
void   event_bearhug(P_char ch, P_char victim, P_obj obj, void *data);
void   do_headlock(P_char ch, char *argument, int cmd);
void   event_headlock(P_char ch, P_char victim, P_obj obj, void *data);
void   armlock_check(P_char attacker, P_char grappler);
void   event_armlock(P_char ch, P_char victim, P_obj obj, void *data);
void   grapple_heal(P_char ch);
void   do_groundslam(P_char ch, char *argument, int cmd);
void   do_leglock(P_char ch, char *argument, int cmd);
void   event_leglock(P_char ch, P_char victim, P_obj obj, void *data);

#endif // __GRAPPLE_H__

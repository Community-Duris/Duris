#include <math.h>
#include <strings.h>
#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "damage.h"
#include "db.h"
#include "dam_mods.h"
#include "hardcore_config.h"
#include "justice.h"
#include "necromancy.h"
#include "paladins.h"
#include "spells.h"
#include "utils.h"

extern float dam_factor[LAST_DF + 1];
extern float racial_spldam_offensive_factor[LAST_RACE + 1][LAST_SPLDAM_TYPE];
extern float racial_spldam_defensive_factor[LAST_RACE + 1][LAST_SPLDAM_TYPE];

dam_mod_predicate spell_damage_modifiers[] = {
	{ MAKE_DAM_MOD_PRED(){ if (get_linked_char(victim, LNK_PET) && IS_PC(caster)){
		dam_mod->mod += get_property("damage.pcs.vs.pets", 2.000) - 1.0;
dam_mod->type = dam_mod_type::More;
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (affected_by_spell(victim, SKILL_BERSERK)){
	  dam_mod->mod = dam_factor[DF_BERSERKSPELL] - 1.0;
dam_mod->type = dam_mod_type::Increased;
if (GET_CLASS(caster, CLASS_BERSERKER))
	dam_mod->mod += dam_factor[DF_BERSERKEREXTRA] - 1.0;
if (affected_by_spell(victim, SKILL_RAGE))
{
	// Being in means taking more spell damage.
	// + 35% ... they are flurried and doing insane damage!
	dam_mod->mod += dam_factor[DF_BERSERKRAGE] - 1.0;
}
}
}
}
, { MAKE_DAM_MOD_PRED(){
	  if (ELEMENTAL_DAM(damageType) && affected_by_spell(victim, SPELL_ENERGY_CONTAINMENT)){
		  dam_mod->mod += dam_factor[DF_ENERGY_CONTAINMENT] - 1.0;
dam_mod->type = dam_mod_type::More;
}
}
}
, { MAKE_DAM_MOD_PRED(){
	  if (ELEMENTAL_DAM(damageType) && has_innate(caster, INNATE_ELEMENTAL_POWER) &&
	      GET_LEVEL(caster) >= 35){ dam_mod->mod += dam_factor[DF_ELEMENTALIST] - 1.0;
dam_mod->type = dam_mod_type::More;
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (GET_CHAR_SKILL(victim, SKILL_ARCANE_BLOCK) > 0 &&
			     !IS_TRUSTED(victim) && !IS_STUNNED(victim) && !IS_IMMOBILE(victim)){
	  if (damage > 15 &&
	      (notch_skill(victim, SKILL_ARCANE_BLOCK, get_property("skill.notch.arcane", 10)) ||
	       number(1, 250) <= (GET_LEVEL(victim) + GET_C_LUK(victim) / 10 +
				  GET_CHAR_SKILL(victim, SKILL_ARCANE_BLOCK)) ||
	       ((IS_ELITE(victim) || IS_GREATER_RACE(victim)) && !number(0, 4)))){
		  dam_mod->mod = -(number(1, get_property("skill.arcane.block.dam.reduction", .4) *
						     GET_CHAR_SKILL(victim, SKILL_ARCANE_BLOCK))) /
				 100.0;
dam_mod->type = dam_mod_type::More;

act("$N raises hands performing an &+Marcane gesture&n and some of $n's &+mspell energy&n is dispersed.",
    TRUE, caster, 0, victim, TO_NOTVICT);
act("$N raises hands performing an &+Marcane gesture&n and some of your &+mspell's energy&n is dispersed.",
    TRUE, caster, 0, victim, TO_CHAR);
act("You perform an &+Marcane gesture&n dispersing some of $n's &+mspell energy.&n", TRUE, caster,
    0, victim, TO_VICT);
}
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (damageType == SPLDAM_GENERIC &&
			     has_innate(victim, MAGICAL_REDUCTION)){ dam_mod->mod += -0.2;
dam_mod->type = dam_mod_type::More;
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (damageType == SPLDAM_FIRE && IS_AFFECTED4(victim, AFF4_ICE_AURA)){
	  dam_mod->mod += dam_factor[DF_VULNFIRE] - 1.0;
dam_mod->type = dam_mod_type::More;
act("&+rYour fiery spell causes&n $N to &+rsmolder and spasm in pain!&n", TRUE, caster, 0, victim,
    TO_CHAR);
act("$n's &+fiery spell causes you smolder and spasm in pain!&n", TRUE, caster, 0, victim, TO_VICT);
act("$n's &+rfiery spell causes&n $N &n&+rto smolder and spasm in pain!&n", TRUE, caster, 0, victim,
    TO_NOTVICT);
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (damageType == SPLDAM_FIRE && caster &&
			     affected_by_spell(caster, TAG_BLOODLUST) && !IS_PC_PET(victim) &&
			     IS_NPC(victim) &&
			     !CHAR_IN_JUSTICE_AREA(caster)){ struct affected_type * paf;
if ((paf = get_spell_from_char(caster, TAG_BLOODLUST)) != NULL)
{
	dam_mod->mod = paf->modifier / 100.0;
	dam_mod->type = dam_mod_type::More;
}
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (damageType != SPLDAM_FIRE) return;

if (has_innate(victim, INNATE_VULN_FIRE))
{
	dam_mod->mod += dam_factor[DF_VULNFIRE] - 1.0;
	dam_mod->type = dam_mod_type::More;

	if (IS_AFFECTED2(victim, AFF2_FIRESHIELD))
		dam_mod->mod += dam_factor[DF_ELSHIELDRED_TROLL] - 1.0;
	else if (IS_AFFECTED(victim, AFF_PROT_FIRE))
		dam_mod->mod += dam_factor[DF_PROTECTION_TROLL] - 1.0;

	if (!affected_by_spell(victim, TAG_TROLL_BURN))
	{
		struct affected_type af;

		bzero(&af, sizeof(af));
		af.type = TAG_TROLL_BURN;
		af.flags = AFFTYPE_SHORT | AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;
		af.duration = WAIT_SEC * 30;
		affect_to_char(victim, &af);
	}
	else
	{
		struct affected_type *af1;
		for (af1 = victim->affected; af1; af1 = af1->next)
		{
			if (af1->type == TAG_TROLL_BURN)
			{
				af1->duration = WAIT_SEC * 30;
				break;
			}
		}
	}
}
else if (IS_AFFECTED2(victim, AFF2_FIRESHIELD))
{
	dam_mod->mod += dam_factor[DF_ELSHIELDRED] - 1.0;
	dam_mod->type = dam_mod_type::More;
}
else if (IS_AFFECTED(victim, AFF_PROT_FIRE))
{
	dam_mod->mod += dam_factor[DF_PROTECTION] - 1.0;
	dam_mod->type = dam_mod_type::More;
}

if (IS_AFFECTED3(victim, AFF3_COLDSHIELD))
{
	dam_mod->type = dam_mod_type::More;
	dam_mod->mod += dam_factor[DF_ELSHIELDINC] - 1.0;
}

if (IS_AFFECTED(victim, AFF_BARKSKIN) || IS_AFFECTED5(victim, AFF5_THORNSKIN))
{
	dam_mod->type = dam_mod_type::More;
	dam_mod->mod += dam_factor[DF_BARKFIRE] - 1.0;
}

if (affected_by_spell(victim, SPELL_IRONWOOD))
{
	dam_mod->type = dam_mod_type::More;
	dam_mod->mod += dam_factor[DF_IRONWOOD] - 1.0;
}

if (IS_AFFECTED5(victim, AFF5_WET))
{
	dam_mod->type = dam_mod_type::More;
	dam_mod->mod += dam_factor[DF_WETFIRE] - 1.0;
	if (ilogb(damage) >
	    number(6, 10)) // not sure ilogb is what this needs to be with this refactor
	{
		make_dry(victim);
		send_to_char("The heat of the spell dried up your clothes completely!\n", victim);
	}
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (damageType != SPLDAM_COLD) return;

if (GET_RACE(victim) == RACE_F_ELEMENTAL || IS_EFREET(victim))
{
	dam_mod->type = dam_mod_type::More;
	dam_mod->mod +=
		(IS_EFREET(victim) ? (.75 * dam_factor[DF_VULNCOLD]) : (dam_factor[DF_VULNCOLD])) -
		1.0;
	act("&+BYour icy spell makes&n $N &+Bwrithe in agony!&n", TRUE, caster, 0, victim, TO_CHAR);
	act("$n's &+Bicy spell causes you to writhe in agony!&n", TRUE, caster, 0, victim, TO_VICT);
	act("$n's &+Bicy spell causes&n $N to writhe in agony!&n", TRUE, caster, 0, victim,
	    TO_NOTVICT);
}
else if (IS_AFFECTED2(victim, AFF2_FIRE_AURA))
{
	dam_mod->type = dam_mod_type::More;
	dam_mod->mod += dam_factor[DF_VULNCOLD] - 1.0;

	act("&+BYour icy spell makes&n $N &+Bwrithe in agony!&n", TRUE, caster, 0, victim, TO_CHAR);
	act("$n's &+Bicy spell causes you to writhe in agony!&n", TRUE, caster, 0, victim, TO_VICT);
	act("$n's &+Bicy spell causes&n $N to writhe in agony!&n", TRUE, caster, 0, victim,
	    TO_NOTVICT);
}

if (IS_AFFECTED3(victim, AFF3_COLDSHIELD))
{
	dam_mod->type = dam_mod_type::More;
	dam_mod->mod += dam_factor[DF_ELSHIELDRED] - 1.0;
}
else if (IS_AFFECTED2(victim, AFF2_PROT_COLD))
{
	dam_mod->type = dam_mod_type::More;
	dam_mod->mod += dam_factor[DF_PROTECTION] - 1.0;
}

if (GET_RACE(victim) == RACE_BARBARIAN)
{
	dam_mod->type = dam_mod_type::More;
	dam_mod->mod += -0.4;
}

if (IS_AFFECTED2(victim, AFF2_FIRESHIELD))
{
	if (GET_CHAR_SKILL(victim, SKILL_FLAME_MASTERY) &&
	    GET_CHAR_SKILL(victim, SKILL_FLAME_MASTERY) > number(1, 101) && !IS_STUNNED(victim) &&
	    CAN_SEE(victim, caster))
	{
		send_to_char("Your shift the &+Yflames&n around your body.\r\n", victim);
	}
	else
	{
		dam_mod->type = dam_mod_type::More;
		dam_mod->mod += dam_factor[DF_ELSHIELDINC] - 1.0;
	}
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (damageType != SPLDAM_GAS) return;

if (IS_AFFECTED3(victim, AFF3_LIGHTNINGSHIELD))
{
	dam_mod->mod += dam_factor[DF_ELSHIELDINC] - 1.0;
	dam_mod->type = dam_mod_type::More;
}
else if (IS_AFFECTED2(victim, AFF2_PROT_GAS))
{
	dam_mod->mod += dam_factor[DF_PROTECTION] - 1.0;
	dam_mod->type = dam_mod_type::More;
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (damageType != SPLDAM_ACID) return;

if (IS_AFFECTED3(victim, AFF3_LIGHTNINGSHIELD))
{
	dam_mod->mod += dam_factor[DF_ELSHIELDINC] - 1.0;
	dam_mod->type = dam_mod_type::More;
}
else if (IS_AFFECTED2(victim, AFF2_PROT_ACID))
{
	dam_mod->mod += dam_factor[DF_PROTECTION] - 1.0;
	dam_mod->type = dam_mod_type::More;
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (damageType != SPLDAM_LIGHTNING) return;

if (IS_AFFECTED3(victim, AFF3_LIGHTNINGSHIELD))
{
	dam_mod->mod += dam_factor[DF_ELSHIELDRED] - 1.0;
	dam_mod->type = dam_mod_type::More;
}
else if (IS_AFFECTED2(victim, AFF2_PROT_LIGHTNING))
{
	dam_mod->mod += dam_factor[DF_PROTECTION] - 1.0;
	dam_mod->type = dam_mod_type::More;
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (damageType != SPLDAM_HOLY) return;

if (victim && IS_UNDEADRACE(victim))
{
	double levelmod = 1.0;
	if (has_innate(victim, INNATE_SACRILEGIOUS_POWER))
	{
		if (GET_LEVEL(victim) >= 46)
			levelmod = 0.75;
		if (GET_LEVEL(victim) >= 51)
			levelmod = 0.5;
		if (GET_LEVEL(victim) >= 56)
			levelmod = 0.25;
	}
	dam_mod->mod += levelmod - 1.0;
	dam_mod->type = dam_mod_type::More;

	if (GET_LEVEL(victim) <
	    56) // Message is not displayed versus level 56 and greater vampires.
	{
		act("$N&+W wavers in agony, as the positive energies purge $S undead essence!&n",
		    FALSE, caster, 0, victim, TO_CHAR);
		act("&+WNooo! The holy power of&n $n&+W is almost too much...&n", FALSE, caster, 0,
		    victim, TO_VICT);
		act("$N&+W wavers in agony, as the positive energies sent by&n $n&+W purge $S essence!&n",
		    FALSE, caster, 0, victim, TO_NOTVICT);
	}

	if (IS_AFFECTED2(victim, AFF2_SOULSHIELD))
	{
		dam_mod->mod += dam_factor[DF_SOULSPELL] - 1.0;
		dam_mod->type = dam_mod_type::More;
	}

	if (IS_AFFECTED4(victim, AFF4_NEG_SHIELD))
	{
		dam_mod->mod += dam_factor[DF_SLSHIELDINCREASE] - 1.0;
		dam_mod->type = dam_mod_type::More;
	}
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (damageType != SPLDAM_PSI) return;

if (IS_AFFECTED3(victim, AFF3_TOWER_IRON_WILL))
{
	dam_mod->mod += dam_factor[DF_IRONWILL] - 1.0;
	dam_mod->type = dam_mod_type::More;
}
if (get_spell_from_char(victim, SKILL_TIGER_PALM))
{
	dam_mod->mod += dam_factor[DF_TIGERPALM] - 1.0;
	dam_mod->type = dam_mod_type::More;
}
if (GET_RACE(victim) == RACE_THRIKREEN)
{
	dam_mod->mod += -0.3;
	dam_mod->type = dam_mod_type::More;
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (damageType != SPLDAM_NEGATIVE) return;

if (victim && IS_ANGEL(victim))
{
	dam_mod->mod += get_property("damage.neg.increase.modifierVsAngel", 1.500) - 1.0;
	dam_mod->type = dam_mod_type::More;
}
if (IS_AFFECTED2(victim, AFF2_SOULSHIELD))
{
	dam_mod->mod += dam_factor[DF_SLSHIELDINCREASE] - 1.0;
	dam_mod->type = dam_mod_type::More;
}
if (IS_AFFECTED4(victim, AFF4_NEG_SHIELD))
{
	dam_mod->mod += dam_factor[DF_NEG_SHIELD_SPELL] - 1.0;
	dam_mod->type = dam_mod_type::More;
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (IS_AFFECTED5(victim, AFF5_JUDICIUM_FIDEI)){
	  dam_mod->mod += dam_factor[DF_JUDICIUM_FIDEI] - 1.0;
dam_mod->type = dam_mod_type::More;
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (!has_innate(victim, MAGIC_VULNERABILITY)) return;

if (GET_RACE(victim) == RACE_OGRE)
{
	dam_mod->mod += 0.1;
	dam_mod->type = dam_mod_type::Increased;
}
else if (GET_RACE(victim) == RACE_FIRBOLG)
{
	dam_mod->mod += 0.1;
	dam_mod->type = dam_mod_type::Increased;
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (affected_by_spell(caster, ACH_DRAGONSLAYER) &&
			     (GET_RACE(victim) == RACE_DRAGON)){ dam_mod->mod += 0.1;
dam_mod->type = dam_mod_type::More;
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (affected_by_spell(caster, ACH_DEMONSLAYER) &&
			     (GET_RACE(victim) == RACE_DEMON)){ dam_mod->mod += 0.1;
dam_mod->type = dam_mod_type::More;
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (affected_by_spell(victim, SPELL_SOULSHIELD) &&
			     (GET_CLASS(victim, CLASS_PALADIN) ||
			      GET_CLASS(victim, CLASS_ANTIPALADIN))){ dam_mod->mod += -0.15;
dam_mod->type = dam_mod_type::More;
}
}
}
, { MAKE_DAM_MOD_PRED(){ double modifier = MAGICRES(victim);
if (modifier >= 1 && !(flags & SPLDAM_NOSHRUG))
{
	float redmod = 100;

	if (modifier <= 20)
		redmod -= (number(1, modifier));
	else if (modifier <= 40)
		redmod -= (number(20, modifier));
	else
		redmod -= (number(40, modifier));

	redmod *= .01;
	dam_mod->mod += redmod - 1.0;
	dam_mod->type = dam_mod_type::More;
}
}
}
, { MAKE_DAM_MOD_PRED(){
	  dam_mod->mod +=
	  ((MAGICDAMBONUS(caster) <= 100 ? 100 : MAGICDAMBONUS(caster) - 10 + number(1, 10)) /
	   100.0) -
	  1.0;
dam_mod->type = dam_mod_type::More;
}
}
, { MAKE_DAM_MOD_PRED(){ struct affected_type *af = NULL;
if ((af = get_spell_from_char(victim, SPELL_ELEM_AFFINITY)) && ELEMENTAL_DAM(damageType))
{
	const char *colors[6] = { "rfire", "Bcold", "Ylightning", "ggas", "Gacid", "yearth" };
	char buf[128];

	if (af->modifier == damageType)
	{
		dam_mod->mod += dam_factor[DF_ELAFFINITY] - 1.0;
		dam_mod->type = dam_mod_type::More;
	}
	else
	{
		if (damageType == SPLDAM_EARTH)
			snprintf(buf, 128, "You feel less vulnerable to &+%s!&n\n", colors[5]);
		else
			snprintf(buf, 128, "You feel less vulnerable to &+%s!&n\n",
				 colors[damageType - 2]);
		send_to_char(buf, victim);
		af->modifier = damageType;
	}
}
}
}
, { MAKE_DAM_MOD_PRED(){
#define HOA_AWE_VNUM 77746

	  bool awe = false;
if (victim->equipment[WEAR_BODY] &&
    (victim->equipment[WEAR_BODY]->R_num == real_object(HOA_AWE_VNUM)))
{
	awe = TRUE;
}

if (ELEMENTAL_DAM(damageType) && IS_AFFECTED4(victim, AFF4_PHANTASMAL_FORM) && !awe)
{
	dam_mod->mod += dam_factor[DF_PHANTFORM] - 1.0;
	dam_mod->type = dam_mod_type::More;
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (has_innate(victim, INNATE_VULN_COLD) && damageType == SPLDAM_COLD){
	  dam_mod->mod += dam_factor[DF_VULNCOLD] - 1.0;
dam_mod->type = dam_mod_type::Increased;
send_to_char("&+CThe freezing cold causes you intense pain!\n", victim);
act("&+CThe freezing cold causes&n $n&+C intense pain!&n", FALSE, victim, 0, 0, TO_ROOM);

if (!NewSaves(victim, SAVING_PARA, 2))
{
	struct affected_type af;

	send_to_char("&+CThe intense cold causes your entire body slooooow doooowwwnn!\n", victim);
	act("&+CThe intense cold causes&n $n&+C's entire body to slooooow doooowwwnn!&n", FALSE,
	    victim, 0, 0, TO_ROOM);

	bzero(&af, sizeof(af));
	af.type = SPELL_SLOW;
	af.flags = AFFTYPE_SHORT;
	af.duration = 60;
	af.bitvector2 = AFF2_SLOW;
	affect_to_char(victim, &af);
}
}
}
}
, { MAKE_DAM_MOD_PRED(){
	  if (parse_chaos_shield(caster, victim)){ dam_mod->mod += dam_factor[DF_CHAOSSHIELD] - 1.0;
dam_mod->type = dam_mod_type::More;
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (affected_by_spell(victim, SKILL_SPELL_PENETRATION)){
	  int damageReductionMod = number(20, 70);
dam_mod->type = dam_mod_type::More;
dam_mod->mod = -(damageReductionMod / 100.0);
// Use this properties file to make fine adjustments to this from now on.
dam_mod->mod += -(get_property("skill.spellPenetration.damageReductionMod", 1.00) / 100.0);
affect_from_char(victim, SKILL_SPELL_PENETRATION);
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (GET_RACE(caster) > RACE_NONE && GET_RACE(caster) <= LAST_RACE &&
			     damageType >= 0 &&
			     damageType < LAST_SPLDAM_TYPE){ dam_mod->type = dam_mod_type::More;
dam_mod->mod += racial_spldam_offensive_factor[GET_RACE(caster)][damageType] - 1.0;
}

if (GET_RACE(victim) > RACE_NONE && GET_RACE(victim) <= LAST_RACE && damageType >= 0 &&
    damageType < LAST_SPLDAM_TYPE)
{
	dam_mod->type = dam_mod_type::More;
	dam_mod->mod += racial_spldam_defensive_factor[GET_RACE(victim)][damageType] - 1.0;
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (IS_NPC(caster) && !affected_by_spell(caster, TAG_CONJURED_PET)){
	  int zone_difficulty = BOUNDED(
		  1, zone_table[world[real_room0(GET_BIRTHPLACE(caster))].zone].difficulty, 10);

if (zone_difficulty > 1)
{
	dam_mod->type = dam_mod_type::More;
	dam_mod->mod +=
		(get_property("damage.zoneDifficulty.spells.factor", 0.05) * zone_difficulty);
}
}
}
}
, { MAKE_DAM_MOD_PRED(){
	  int necropets[] = { 3, 4, 5, 6, 7, 8, 9, 10, 78, 79, 80, 81, 82, 83, 84, 85 };
if (IS_NPC(caster) && IS_PC(victim))
{
	for (int r = 0; r < 16; r++)
	{
		if (mob_index[GET_RNUM(caster)].virtual_number == NECROPET ||
		    mob_index[GET_RNUM(caster)].virtual_number == necropets[r])
		{
			dam_mod->type = dam_mod_type::More;
			dam_mod->mod += -0.5;
			break;
		}
	}
}
}
}
, { MAKE_DAM_MOD_PRED(){ dam_mod->mod += get_property("damage.spell.multiplier", 1.0) - 1.0;
dam_mod->type = dam_mod_type::More;
}
}
, { MAKE_DAM_MOD_PRED(){ if (get_linked_char(victim, LNK_ETHEREAL) ||
			     get_linking_char(victim, LNK_ETHEREAL)){ P_char eth_ch = NULL;

if (get_linked_char(victim, LNK_ETHEREAL))
	eth_ch = get_linked_char(victim, LNK_ETHEREAL);
else if (get_linking_char(victim, LNK_ETHEREAL))
	eth_ch = get_linking_char(victim, LNK_ETHEREAL);

if (IS_ALIVE(eth_ch))
{
	double localDam = damage * 0.5;

	send_to_char(
		"&+cThe damage passes through the &+Wether&+c being absorbed by your alliance!\n",
		victim);
	send_to_char("&+cYou feel weak as your &+Wethereal alliance&+c fills you with pain!\n",
		     eth_ch);

	raw_damage(caster, eth_ch, localDam, RAWDAM_DEFAULT ^ flags, messages);

	dam_mod->type = dam_mod_type::More;
	dam_mod->mod += -0.5;
}
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (affected_by_spell(victim, SKILL_DREADNAUGHT)){
	  struct affected_type *paf = get_spell_from_char(victim, SKILL_DREADNAUGHT);
// 5% to 20% reduction based on skill level (with a chance for a few extra percent when skill is 95+)
double skill = BOUNDED(5, paf->level / 5, 20) + (paf->level > 95 ? number(0, paf->level - 95) : 0);
double reduction = (100.0 - skill) / 100.0;
dam_mod->type = dam_mod_type::Increased;
dam_mod->mod = reduction - 1.0;
}
}
}
, { MAKE_DAM_MOD_PRED(){
	  if (IS_AFFECTED3(victim, AFF3_PALADIN_AURA) &&
	      has_aura(victim, AURA_SPELL_PROTECTION)){ dam_mod->type = dam_mod_type::More;
dam_mod->mod = -(aura_mod(victim, AURA_SPELL_PROTECTION) / 100.0);
}
}
}
,
}
;

static_assert(ARRAY_SIZE(spell_damage_modifiers) == NUM_SPELL_PREDICATES);

dam_mod_predicate raw_damage_modifiers[] = { { MAKE_DAM_MOD_PRED(){ switch (GET_RACEWAR(caster)){
	case RACEWAR_GOOD: dam_mod->mod += dam_factor[DF_GOOD_MODIFIER] - 1.0;
break;
case RACEWAR_EVIL:
dam_mod->mod += dam_factor[DF_EVIL_MODIFIER] - 1.0;
break;
case RACEWAR_UNDEAD:
dam_mod->mod += dam_factor[DF_UNDEAD_MODIFIER] - 1.0;
break;
case RACEWAR_NEUTRAL:
dam_mod->mod += dam_factor[DF_NEUTRAL_MODIFIER] - 1.0;
break;
}
dam_mod->type = dam_mod_type::More;
}
}
, { MAKE_DAM_MOD_PRED(){ if (IS_NPC(caster) && !IS_PC_PET(caster)){
	  dam_mod->mod += get_property("damage.mob.bonus", 1.0) - 1.0;
dam_mod->type = dam_mod_type::More;
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (affected_by_spell(caster, TAG_BLOODLUST) && !IS_PC_PET(victim) &&
			     IS_NPC(victim)){ int dammod = 0;
struct affected_type *findaf, *next_af; // initialize affects
for (findaf = caster->affected; findaf; findaf = next_af)
{
	next_af = findaf->next;
	if ((findaf && findaf->type == TAG_BLOODLUST))
		dammod = findaf->modifier;
}
dam_mod->mod += (dammod / 100.0);
dam_mod->type = dam_mod_type::Increased;
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (IS_HARDCORE(caster)){ dam_mod->mod += 0.09;
dam_mod->type = dam_mod_type::Increased;
}
if (IS_HARDCORE(victim))
{
	dam_mod->mod += -0.09;
	dam_mod->type = dam_mod_type::Increased;
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (IS_HARDCORE(caster)){
	  dam_mod->mod += hardcore_config_get()->bonus_damage_outgoing_multiplier - 1.0f;
dam_mod->type = dam_mod_type::Increased;
}
if (IS_HARDCORE(victim))
{
	dam_mod->mod += hardcore_config_get()->bonus_damage_incoming_multiplier - 1.0f;
	dam_mod->type = dam_mod_type::Increased;
}
}
}
, { MAKE_DAM_MOD_PRED(){
	  if ((IS_GOOD(caster) && affected_by_spell(caster, SPELL_HOLY_SWORD) && IS_EVIL(victim)) ||
	      (IS_EVIL(caster) && affected_by_spell(caster, SPELL_HOLY_SWORD) && IS_GOOD(victim))){
		  dam_mod->mod += dice(2, 6);
dam_mod->type = dam_mod_type::Added;
}
}
}
, { MAKE_DAM_MOD_PRED(){
	  if (IS_AFFECTED4(victim, AFF4_SANCTUARY) && (flags & RAWDAM_SANCTUARY) &&
	      (GET_CLASS(victim, CLASS_PALADIN))){ dam_mod->mod += dam_factor[DF_SANC] - 1.0;
dam_mod->type = dam_mod_type::More;
int group_size = num_group_members_in_room(victim);
double group_mod =
	-(group_size * get_property("damage.reduction.sanctuary.paladin.groupMod", 0.02));
dam_mod->mod += MAX(-0.45, group_mod);
}
}
}
, { MAKE_DAM_MOD_PRED(){
	  if (IS_AFFECTED4(victim, AFF4_SANCTUARY) && (flags & RAWDAM_SANCTUARY) &&
	      GET_CLASS(victim, CLASS_CLERIC)){ dam_mod->mod += dam_factor[DF_SANC] - 1.0;
dam_mod->type = dam_mod_type::More;
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (get_spell_from_room(&world[caster->in_room], SPELL_CONSECRATE_LAND) &&
			     !(flags & PHSDAM_NOREDUCE)){ dam_mod->mod += -0.5;
dam_mod->type = dam_mod_type::More;
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (get_spell_from_room(&world[caster->in_room], SPELL_BINDING_WIND) &&
			     !(flags & PHSDAM_NOREDUCE)){ dam_mod->mod += -0.2;
dam_mod->type = dam_mod_type::More;
}
}
}
, { MAKE_DAM_MOD_PRED(){
	  if (IS_AFFECTED3(victim, AFF3_PROT_ANIMAL) && IS_ANIMAL(caster) &&
	      !(flags & PHSDAM_NOREDUCE)){ dam_mod->mod += dam_factor[DF_PROTANIMAL] - 1.0;
dam_mod->type = dam_mod_type::More;
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (IS_AFFECTED3(caster, AFF3_PALADIN_AURA) &&
			     has_aura(caster, AURA_BATTLELUST) && !(flags & PHSDAM_NOREDUCE)){
	  dam_mod->mod += ((get_property("innate.paladin_aura.battlelust_mod", 0.2) *
			    aura_mod(caster, AURA_BATTLELUST)) /
			   100.0);
dam_mod->type = dam_mod_type::More;
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (has_innate(caster, INNATE_WARCALLERS_FURY)){
	  int group_size = num_group_members_in_room(victim);
double bonus = BOUNDEDF(.02, (group_size / 100.0) + (GET_LEVEL(caster) / 1120.0), (double).15);

dam_mod->mod += bonus;
dam_mod->type = dam_mod_type::More;
if (caster->group)
{
	int count = 0;
	for (struct group_list *gl = caster->group; gl; gl = gl->next)
	{
		if (caster != gl->ch && IS_PC(gl->ch) && caster->in_room == gl->ch->in_room &&
		    has_innate(gl->ch, INNATE_WARCALLERS_FURY))
		{
			count++;
		}
	}
	if (count > 0)
	{
		count = MIN(count, (int)get_property("innate.warcallersfury.maxSteps", 3));
		dam_mod->mod += (count / 30.0);
	}
}
}
}
}
,
	{ MAKE_DAM_MOD_PRED(){ if (!(flags & PHSDAM_NOREDUCE)){ // global mod of 75% reduced damage
								dam_mod->mod += -0.75;
dam_mod->type = dam_mod_type::Increased;
// if (IS_NPC(caster) && !IS_PC_PET(caster) && !IS_MORPH(caster) && (IS_PC(victim) || IS_PC_PET(victim) || IS_MORPH(victim)))
// {
// 	// npcs to pcs do 25% increased damage
// 	dam_mod->mod += 0.25;
// }
}
}
}
, { MAKE_DAM_MOD_PRED(){ if (IS_NPC(caster) && !IS_PC_PET(caster) && !IS_MORPH(caster) &&
			     (IS_PC(victim) || IS_PC_PET(victim) || IS_MORPH(victim))){
	  dam_mod->type = dam_mod_type::More;
dam_mod->mod += ((dam_factor[DF_NPCTOPC] / 2) - 1.0);
if (GET_RACEWAR(victim) == RACEWAR_GOOD)
	dam_mod->mod += (float)get_property("damage.modifier.npcToPc.good", 1.000) - 1.0;
if (GET_RACEWAR(victim) == RACEWAR_EVIL)
	dam_mod->mod += (float)get_property("damage.modifier.npcToPc.evil", 1.000) - 1.0;
}
}
}
,
}
;

static_assert(ARRAY_SIZE(raw_damage_modifiers) == NUM_RAW_PREDICATES);

#ifndef DURIS_ITEM_VALUE_QUOTE_H
#define DURIS_ITEM_VALUE_QUOTE_H

#include "core/structs.h"
#include <math.h>

template <typename Observations> int item_prepare_value(Observations &observations)
{
	double workingvalue = 0;
	double multiplier = 1;
	double mod;

	if (!observations.present())
	{
		return 0;
	}

	if (observations.wear_flag(ITEM_WEAR_EYES))
		multiplier *= 1.3;

	if (observations.wear_flag(ITEM_WEAR_EARRING))
		multiplier *= 1.2;

	if (observations.wear_flag(ITEM_WEAR_FACE))
		multiplier *= 1.3;

	if (observations.wear_flag(ITEM_WEAR_QUIVER))
		multiplier *= 1.1;

	if (observations.wear_flag(ITEM_WEAR_FINGER))
		multiplier *= 1.2;

	if (observations.wear_flag(ITEM_GUILD_INSIGNIA))
		multiplier *= 1.5;

	if (observations.wear_flag(ITEM_WEAR_NECK))
		multiplier *= 1.2;

	if (observations.wear_flag(ITEM_WEAR_WAIST))
		multiplier *= 1.1;

	if (observations.wear_flag(ITEM_WEAR_WRIST))
		multiplier *= 1.1;

	// Aff's add to the base value.
	if (observations.affect_flag1(AFF_STONE_SKIN))
		workingvalue += 125;

	if (observations.affect_flag1(AFF_BIOFEEDBACK))
		workingvalue += 110;

	if (observations.affect_flag1(AFF_FARSEE))
		workingvalue += 45;

	if (observations.affect_flag1(AFF_DETECT_INVISIBLE))
		workingvalue += 90;

	if (observations.affect_flag1(AFF_HASTE))
		workingvalue += 75;

	if (observations.affect_flag1(AFF_INVISIBLE))
		workingvalue += 35;

	if (observations.affect_flag1(AFF_SENSE_LIFE))
		workingvalue += 45;

	if (observations.affect_flag1(AFF_MINOR_GLOBE))
		workingvalue += 28;

	if (observations.affect_flag1(AFF_UD_VISION))
		workingvalue += 40;

	if (observations.affect_flag1(AFF_WATERBREATH))
		workingvalue += 45;

	if (observations.affect_flag1(AFF_PROTECT_EVIL))
		workingvalue += 35;

	if (observations.affect_flag1(AFF_PROTECT_GOOD))
		workingvalue += 35;

	if (observations.affect_flag1(AFF_SLOW_POISON))
		workingvalue += 20;

	if (observations.affect_flag1(AFF_SNEAK))
		workingvalue += 125;

	if (observations.affect_flag1(AFF_BARKSKIN))
		workingvalue += 25;

	if (observations.affect_flag1(AFF_INFRAVISION))
		workingvalue += 7;

	if (observations.affect_flag1(AFF_LEVITATE))
		workingvalue += 13;

	if (observations.affect_flag1(AFF_HIDE))
		workingvalue += 85;

	if (observations.affect_flag1(AFF_FLY))
		workingvalue += 75;

	if (observations.affect_flag1(AFF_AWARE))
		workingvalue += 75;

	if (observations.affect_flag1(AFF_PROT_FIRE))
		workingvalue += 20;

	if (observations.affect_flag2(AFF2_FIRESHIELD))
		workingvalue += 45;

	if (observations.affect_flag2(AFF2_ULTRAVISION))
		workingvalue += 80;

	if (observations.affect_flag2(AFF2_DETECT_EVIL))
		workingvalue += 5;

	if (observations.affect_flag2(AFF2_DETECT_GOOD))
		workingvalue += 5;

	if (observations.affect_flag2(AFF2_DETECT_MAGIC))
		workingvalue += 10;

	if (observations.affect_flag2(AFF2_PROT_COLD))
		workingvalue += 20;

	if (observations.affect_flag2(AFF2_PROT_LIGHTNING))
		workingvalue += 30;

	if (observations.affect_flag2(AFF2_GLOBE))
		workingvalue += 80;

	if (observations.affect_flag2(AFF2_PROT_GAS))
		workingvalue += 30;

	if (observations.affect_flag2(AFF2_PROT_ACID))
		workingvalue += 30;

	if (observations.affect_flag2(AFF2_SOULSHIELD))
		workingvalue += 45;

	if (observations.affect_flag2(AFF2_CONCEALMENT))
		workingvalue += 15;

	if (observations.affect_flag2(AFF2_VAMPIRIC_TOUCH))
		workingvalue += 65;

	if (observations.affect_flag2(AFF2_EARTH_AURA))
		workingvalue += 110;

	if (observations.affect_flag2(AFF2_WATER_AURA))
		workingvalue += 115;

	if (observations.affect_flag2(AFF2_FIRE_AURA))
		workingvalue += 120;

	if (observations.affect_flag2(AFF2_AIR_AURA))
		workingvalue += 130;

	if (observations.affect_flag2(AFF2_PASSDOOR))
		workingvalue += 80;

	if (observations.affect_flag2(AFF2_FLURRY))
		workingvalue += 150;

	if (observations.affect_flag3(AFF3_PROT_ANIMAL))
		workingvalue += 20;

	if (observations.affect_flag3(AFF3_SPIRIT_WARD))
		workingvalue += 35;

	if (observations.affect_flag3(AFF3_GR_SPIRIT_WARD))
	{
		workingvalue += 25;
		multiplier += 1.20;
	}

	if (observations.affect_flag3(AFF3_ENLARGE))
		workingvalue += 120;

	if (observations.affect_flag3(AFF3_REDUCE))
		workingvalue += 120;

	if (observations.affect_flag3(AFF3_INERTIAL_BARRIER))
		workingvalue += 135;

	if (observations.affect_flag3(AFF3_COLDSHIELD))
		workingvalue += 45;

	if (observations.affect_flag3(AFF3_TOWER_IRON_WILL))
		workingvalue += 45;

	if (observations.affect_flag3(AFF3_BLUR))
		workingvalue += 65;

	if (observations.affect_flag3(AFF3_PASS_WITHOUT_TRACE))
		workingvalue += 45;

	if (observations.affect_flag4(AFF4_VAMPIRE_FORM))
		workingvalue += 90;

	if (observations.affect_flag4(AFF4_HOLY_SACRIFICE))
		workingvalue += 105;

	if (observations.affect_flag4(AFF4_BATTLE_ECSTASY))
		workingvalue += 105;

	if (observations.affect_flag4(AFF4_DAZZLER))
		workingvalue += 45;

	if (observations.affect_flag4(AFF4_PHANTASMAL_FORM))
		workingvalue += 105;

	if (observations.affect_flag4(AFF4_NOFEAR))
		workingvalue += 40;

	if (observations.affect_flag4(AFF4_REGENERATION))
		workingvalue += 60;

	if (observations.affect_flag4(AFF4_GLOBE_OF_DARKNESS))
		workingvalue += 15;

	if (observations.affect_flag4(AFF4_HAWKVISION))
		workingvalue += 20;

	if (observations.affect_flag4(AFF4_SANCTUARY))
		workingvalue += 105;

	if (observations.affect_flag4(AFF4_HELLFIRE))
		workingvalue += 110;

	if (observations.affect_flag4(AFF4_SENSE_HOLINESS))
		workingvalue += 15;

	if (observations.affect_flag4(AFF4_PROT_LIVING))
		workingvalue += 45;

	if (observations.affect_flag4(AFF4_DETECT_ILLUSION))
		workingvalue += 40;

	if (observations.affect_flag4(AFF4_ICE_AURA))
		workingvalue += 90;

	if (observations.affect_flag4(AFF4_NEG_SHIELD))
		workingvalue += 45;

	if (observations.affect_flag4(AFF4_WILDMAGIC))
		workingvalue += 240;

	// Has a old school proc (Up to three spells).
	// Can un-comment the debug stuff if you want to modify this.
	if (observations.wear_flag(ITEM_WIELD) && (observations.value(5) > 0))
	{
		int spells[3];
		int spellcirclesum, numspells;

		// val5 : 3 spells + all or one.
		spells[0] = observations.value(5) % 1000;
		spells[1] = observations.value(5) % 1000000 / 1000;
		spells[2] = observations.value(5) % 1000000000 / 1000000;
		//    debug( "Spells0: %d, Spells1: %d, Spells2: %d.", spells[0], spells[1], spells[2] );

		// val6 = level * val7 = chance -> 1/30 chance = 1, 1/60 chance = .5, 1/15 chance = 2, etc.
		mod = ((observations.value(6) > 19) ? observations.value(6) / 10.0 : 1) *
		      (30.0 / observations.value(7));
		//    debug( "mod: %f, objval6/10: %f, 30/objval7: %f", mod, (observations.value(6) > 19) ? observations.value(6) / 10.0 : 1, (30.0 / observations.value(7)) );

		spellcirclesum = observations.minimum_circle(spells[0]);
		spellcirclesum += observations.minimum_circle(spells[1]);
		spellcirclesum += observations.minimum_circle(spells[2]);
		//    debug( "spellcirclesum: %d, circle0: %d, circle1: %d, circle2: %d.", spellcirclesum, observations.minimum_circle(spells[0]), observations.minimum_circle(spells[1]), observations.minimum_circle(spells[2]) );

		// 1 lvl 10 spell  2nd circle 1/60 chance = 1*1* 2*.5 =  1
		// 1 lvl 60 spell  1st circle 1/30 chance = 1*6* 1*1  =  6
		// 1 lvl 40 spell  3rd circle 1/30 chance = 1*4* 3*1  = 12
		// 1 lvl 60 spell 12th circle 1/30 chance = 1*6*12*1  = 72 etc.
		// val5 / 1000000000 -> 1, otherwise casts all.
		if (observations.value(5) / 1000000000)
		{
			// Add up number of spells
			numspells = ((spells[0]) ? 1 : 0) + ((spells[1]) ? 1 : 0) +
				    ((spells[2]) ? 1 : 0);
			// If there are none?!, set to 1 anyway.
			numspells = numspells ? numspells : 1;
			// Compute average circle.
			//      debug( "mod * spellcirclesum / numspells: %d.",(int) (mod * (spellcirclesum / numspells)) );
			workingvalue += (int)(mod * (spellcirclesum / numspells));
		}
		else
		{
			//      debug( "spellcirclesum * mod: %d.", (int) (spellcirclesum * mod) );
			workingvalue += mod * spellcirclesum;
		}
	}

	// Real Obj procs
	if (observations.prototype_has_proc())
	{
		workingvalue += observations.prototype_proc_value();
	}

	//------- A0/A1/A2 -------------
	int i = 0;
	while (i < MAX_OBJ_AFFECT)
	{
		mod = observations.affect_modifier(i);
		// dam/hitroll are normal values
		if ((observations.affect_location(i) == APPLY_DAMROLL) ||
		    (observations.affect_location(i) == APPLY_HITROLL))
		{
			if (observations.item_type() == ITEM_WEAPON)
			{
				// 1:1, 2:2, 3:6, 4:12, 5:20, 6:30, 7:42, 8:56, 9:72, 10:90, 11: 110..
				workingvalue += (mod <= 2) ? mod : mod * (mod - 1);
			}
			else
			{
				// 1:2, 2:5, 3:30, 4:51, 5:78, 6:111
				// 1:2, 2:6, 3:37, 4:63, 5:97 after multiplier (note: wear flag will raise 5 over 100).
				workingvalue += (mod <= 2) ? (3 * mod - 1) : 3 * mod * mod + 3;
			}
			// Translates to 1:1, 2:2, 3:7, 4:15, 5:25, 6:37, 7:52, 8:70, 9:90, 10:112
			multiplier *= 1.25;
			// So a 5/5 weapon is essentially 40 * 1.25 * 1.25 = 62.5 (before adding other stats).
			// A 6/6 item (no other stats) is 93, a 2d2 6/6 sword would be 62 * 1.25^2 = 96, and 5d5 6/6 = 112.
		}

		// Regular stats can be high numbers - half them
		if ((observations.affect_location(i) == APPLY_STR) ||
		    (observations.affect_location(i) == APPLY_DEX) ||
		    (observations.affect_location(i) == APPLY_INT) ||
		    (observations.affect_location(i) == APPLY_WIS) ||
		    (observations.affect_location(i) == APPLY_CON) ||
		    (observations.affect_location(i) == APPLY_AGI))
		{
			// 1:2, 2:4, 3:6, 4:9, 5:16, 6:25, 7:36, 8:49, 9:64, 10:81, 11:100
			workingvalue += (mod <= 3) ? 2 * mod : (mod - 1) * (mod - 1);
		}

		// These are used a little less
		if ((observations.affect_location(i) == APPLY_POW) ||
		    (observations.affect_location(i) == APPLY_CHA) ||
		    (observations.affect_location(i) == APPLY_LUCK))
		{
			// 1:2, 2:4, 3:6, 4:8, 5:10, 6:12, 7:15, 8:26, 9:39, 10:54, 11:71, 12:90, 13: 111
			workingvalue += (mod <= 6) ? 2 * mod : (mod - 2) * (mod - 2) - 10;
		}

		// Hitpoints.
		if (observations.affect_location(i) == APPLY_HIT)
		{
			// 1 : 2, 4 : 8, 5 : 11, 10 : 29, 20 : 65, 30 : 101 (can't be crafted), 32 : 108 (can't be enhanced).
			workingvalue += (mod <= 4) ? 2 * mod : (18 * mod) / 5 - 7;
		}

		// Moves and mana are generally large #'s
		if ((observations.affect_location(i) == APPLY_MOVE) ||
		    (observations.affect_location(i) == APPLY_MANA))
		{
			// Right now, 25 : 25, 35 : 65, 44 : 101, 45 : 105 - not enhanceable.
			workingvalue += (mod <= 25) ? mod : 4 * mod - 75;
		}

		// Hit, move, mana, regen are generally large #'s, but we don't want above 9.
		if ((observations.affect_location(i) == APPLY_HIT_REG) ||
		    (observations.affect_location(i) == APPLY_MOVE_REG) ||
		    (observations.affect_location(i) == APPLY_MANA_REG))
		{
			// 1:1, 2:2, 3:3, 4:5, 5:8, 6:12, 7:16, 8:21, 9:27, 10:33
			// 11:40, 12:48, 13:56, 14:65, 15:75, 16:85, 17:96, 18:108
			workingvalue += (mod < 4) ? mod : (mod * mod) / 3;
		}

		// Racial attributes #'s - Do we still have these?
		if ((observations.affect_location(i) == APPLY_AGI_RACE) ||
		    (observations.affect_location(i) == APPLY_STR_RACE) ||
		    (observations.affect_location(i) == APPLY_CON_RACE) ||
		    (observations.affect_location(i) == APPLY_INT_RACE) ||
		    (observations.affect_location(i) == APPLY_WIS_RACE) ||
		    (observations.affect_location(i) == APPLY_CHA_RACE) ||
		    (observations.affect_location(i) == APPLY_DEX_RACE))
		{
			if (mod < 1 || mod > LAST_RACE)
			{
				observations.invalid_race_notice(i, mod);
				workingvalue += 100;
			}
			else
			{
				switch (observations.affect_location(i))
				{
				// We're looking for the stat vs 100. 75->0pts, 100->50pts, 150->150pts, 200->250pts
				case APPLY_AGI_RACE:
					workingvalue +=
						2 * observations.race_agility((int)mod) - 150;
					break;
				case APPLY_STR_RACE:
					workingvalue +=
						2 * observations.race_strength((int)mod) - 150;
					break;
				case APPLY_CON_RACE:
					workingvalue +=
						2 * observations.race_constitution((int)mod) - 150;
					break;
				case APPLY_INT_RACE:
					workingvalue +=
						2 * observations.race_intelligence((int)mod) - 150;
					break;
				case APPLY_WIS_RACE:
					workingvalue +=
						2 * observations.race_wisdom((int)mod) - 150;
					break;
				case APPLY_CHA_RACE:
					workingvalue +=
						2 * observations.race_charisma((int)mod) - 150;
					break;
				case APPLY_DEX_RACE:
					workingvalue +=
						2 * observations.race_dexterity((int)mod) - 150;
					break;
				// Should never be the case but..
				default:
					observations.bad_race_location_notice(i, mod);
					workingvalue += 100;
					break;
				}
			}
		}

		// AC negative is good, not reducing itemvalue for items that make ac worse.
		if ((observations.affect_location(i) == APPLY_AC) && mod != 0)
		{
			// 1.5 points for each point of armor class.
			// 1 : 1, 2 : 3, 3 : 4, 5 : 7, ... 50 : 75, 67 : 100, 68 : 102 (!craft), 70 : 105 (!enhance).
			if (mod < 0)
			{
				mod *= -1;
			}
			workingvalue += (3 * mod) / 2;
			// +10% at 50ac.
			multiplier += mod / 500.;
		}

		// saving throw values (good) are negative
		if ((observations.affect_location(i) == APPLY_SAVING_PARA) ||
		    (observations.affect_location(i) == APPLY_SAVING_ROD) ||
		    (observations.affect_location(i) == APPLY_SAVING_FEAR) ||
		    (observations.affect_location(i) == APPLY_SAVING_BREATH) ||
		    (observations.affect_location(i) == APPLY_SAVING_SPELL))
		{
			// -1:2, -2:8, -3:18, -4:32, -5:50, -6:72, -7:98, -8:128
			workingvalue += mod * mod * ((mod <= 0) ? 2 : -2);
		}

		// pulse is quite valuable and negative is good
		if ((observations.affect_location(i) == APPLY_COMBAT_PULSE) ||
		    (observations.affect_location(i) == APPLY_SPELL_PULSE))
		{
			multiplier *= 2;
			workingvalue += mod * -75;
		}

		// Max_stats double points
		if ((observations.affect_location(i) == APPLY_STR_MAX) ||
		    (observations.affect_location(i) == APPLY_DEX_MAX) ||
		    (observations.affect_location(i) == APPLY_INT_MAX) ||
		    (observations.affect_location(i) == APPLY_WIS_MAX) ||
		    (observations.affect_location(i) == APPLY_CON_MAX) ||
		    (observations.affect_location(i) == APPLY_CHA_MAX) ||
		    (observations.affect_location(i) == APPLY_AGI_MAX) ||
		    (observations.affect_location(i) == APPLY_POW_MAX) ||
		    (observations.affect_location(i) == APPLY_LUCK_MAX))
		{
			// 1:3, 2:13, 3:24, 4:36, 5:51, 6:66, 7:83, 8:100
			workingvalue += (mod < 2) ? 3.0 * mod : 3.52 * mod * sqrt(mod) + mod;
			multiplier += .15;
		}
		i++;
	}

	if (observations.item_type() == ITEM_WEAPON)
	{
		// Add avg damage.
		workingvalue += (observations.value(1) * observations.value(2));
		// 1d1 = .7%, 5d5 = 17.5%, 10d10 = 70%.
		multiplier += observations.value(1) * observations.value(2) * .005;
		// Backstabbing weapons get a big ival for big dice.
		if (observations.backstabber())
		{
			mod = observations.value(2);
			// workingvalue increases quadratic for every die roll and cubic for dice size.
			// For number of dice: 1:1, 2:1.15, 3:1.4, 4:1.75, 5:2.2, 6:2.75, 7:3.4, 8:4.15, 9:5, 10:5.95
			// For number of die sides: 1:0, 2:1, 3:5, 4:12, 5:25, 6:43, 7: 68, 8:102, 9:145, 10: 200
			// So, 1d8 / 3d7 stabber is !forge and !enhance (115 ival when combined with above).
			workingvalue +=
				((observations.value(1) * observations.value(1) + 19.) / 20.) *
				(mod * mod * mod) / 5.;
		}
	}
	if (observations.item_type() == ITEM_ARMOR)
	{
		mod = observations.value(0);
		// Same as APPLY_AC.  1.5 points for each point of armor class.
		if (mod < 0)
		{
			mod *= -1;
		}
		workingvalue += (3 * mod) / 2;
		// +10% at 50ac.
		multiplier += mod / 500.;
	}

	// Two handed items have less ival.
	if (observations.extra_flag(ITEM_TWOHANDS))
	{
		multiplier *= .80;
	}

	workingvalue *= multiplier;

	if (workingvalue < 1)
	{
		workingvalue = 1;
	}

	if ((!observations.can_take() && observations.item_type() == ITEM_TELEPORT) ||
	    observations.item_type() == ITEM_KEY || observations.item_type() == ITEM_SWITCH ||
	    observations.item_type() == ITEM_VEHICLE || observations.item_type() == ITEM_SHIP ||
	    observations.item_type() == ITEM_STORAGE)
	{
		if (workingvalue != 1)
		{
			observations.forced_value_notice(workingvalue);
		}
		return 1;
	}

	// debug("&+YItem value is: &n%d", workingvalue);
	return workingvalue;
}

#endif

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "world/vnum.obj.h"
#include "item/objmisc.h"
#include "world/achievements.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "core/mm.h"
#include "combat/damage.h"
#include "magic/spells.h"
#include "economy/economic_gameplay_authority.h"
#include "item/item_movement_transaction.h"
#include "magic/spell_item_lifecycle.h"
#include "persistence/critical_command.h"

extern P_obj object_list;
extern P_char character_list;

extern const struct race_names race_names_table[];
extern P_index obj_index;
extern int get_multicast_chars(P_char leader, int m_class, int min_level);
#include <strings.h>
#include <time.h>

int conjure_terrain_check(P_char, P_char);

static P_obj spell_item_by_uid(uint64_t uid)
{
	for (P_obj object = object_list; object; object = object->next)
		if (object->obj_uid == uid)
			return object;
	return NULL;
}

static uint64_t spell_creation_source_id()
{
	critical_operation_id occurrence = {};
	if (!critical_operation_id_generate(&occurrence))
		return 0;
	uint64_t source_id = 0;
	for (size_t index = 0; index < sizeof(source_id); ++index)
		source_id |= static_cast<uint64_t>(occurrence.bytes[index]) << (index * 8);
	return source_id;
}

static void spell_room_creation_completed(P_char actor, uint64_t item_uid, bool committed,
					  unsigned int /*error_code*/)
{
	if (!actor)
		return;
	if (!committed)
	{
		send_to_char("The conjured item could not be recorded and fades away.\r\n", actor);
		return;
	}

	P_obj object = spell_item_by_uid(item_uid);
	if (!object || !OBJ_ROOM(object))
		return;
	if (actor->in_room == object->loc.room)
	{
		act("$p &+Wsuddenly appears.", FALSE, actor, object, 0, TO_ROOM);
		act("$p &+Wsuddenly appears.", FALSE, actor, object, 0, TO_CHAR);
	}
	else
		send_to_room("A conjured item suddenly appears.\r\n", object->loc.room);
}

static bool submit_spell_room_creation(P_char actor, P_obj object)
{
	const uint64_t source_id = IS_PC(actor) ? spell_creation_source_id() : 0;
	if (source_id &&
	    item_creation_grant_submit_to_room(actor, object, actor->in_room,
					       economic_source_kind::spell_creation,
					       spell_room_creation_completed, source_id))
		return true;
	if (OBJ_NOWHERE(object))
		extract_obj(object, FALSE);
	send_to_char("The item cannot be created right now; please try again later.\r\n", actor);
	return false;
}

static void announce_spell_player_item(P_char actor, P_obj object)
{
	if (!actor || !object)
		return;
	switch (OBJ_VNUM(object))
	{
	case 366:
		act("$p &+Warrives in a burst of &n&+rfire.", TRUE, actor, object, 0, TO_ROOM);
		act("$p &+Warrives in a burst of &n&+rfire.", TRUE, actor, object, 0, TO_CHAR);
		break;
	case 368:
		act("$p &+Wslowly materializes.", TRUE, actor, object, 0, TO_ROOM);
		act("$p &+Wslowly materializes.", TRUE, actor, object, 0, TO_CHAR);
		break;
	case 426:
		act("As you call to the &n&+Wh&n&+Yea&n&+Wv&n&+Ye&n&+Wns&n for a weapon to slay the &n&+Lev&n&+ri&n&+Ll&n in the world, an",
		    TRUE, actor, object, 0, TO_CHAR);
		act("&n&+Yang&n&+We&n&+Yl&n&+Ric&n figure &n&+Lmat&n&+wer&n&+Wia&n&+wli&n&+Lzes&n before you, handing you a &n&+Ygolden&n blade. The",
		    TRUE, actor, object, 0, TO_CHAR);
		act("figure recites a short &n&+Yprayer&n before &n&+Wva&n&+wni&n&+Ls&n&+whi&n&+Wng&n as fast as it came.",
		    TRUE, actor, object, 0, TO_CHAR);
		act("As $n calls to the &n&+Wh&n&+Yea&n&+Wv&n&+Ye&n&+Wns&n for a weapon to slay the &n&+Lev&n&+ri&n&+Ll&n in the world,",
		    TRUE, actor, object, 0, TO_ROOM);
		act("An &n&+Yang&n&+We&n&+Yl&n&+Ric&n figure &n&+Lmat&n&+wer&n&+Wia&n&+wli&n&+Lzes&n before you, handing a &n&+Ygolden&n blade to $n.",
		    TRUE, actor, object, 0, TO_ROOM);
		act("The figure recites a short &n&+Yprayer&n before &n&+Wva&n&+wni&n&+Ls&n&+whi&n&+Wng&n as fast as it came.",
		    TRUE, actor, object, 0, TO_ROOM);
		break;
	case 352:
		act("$n &+wplunges $s clenched fist into the &+yground&+w and draws forth $p!&n",
		    TRUE, actor, object, 0, TO_ROOM);
		act("&+wYou plunge your fist into the &+yground&+w and rip out $p!&n", TRUE, actor,
		    object, 0, TO_CHAR);
		break;
	default:
		act("$p appears in your hands.", TRUE, actor, object, 0, TO_ROOM);
		act("$p appears in your hands.", TRUE, actor, object, 0, TO_CHAR);
		break;
	}
}

static void spell_player_creation_completed(P_char actor, uint64_t item_uid, bool committed,
					    unsigned int /*error_code*/)
{
	if (!actor)
		return;
	if (!committed)
	{
		send_to_char("The conjured item could not be delivered and fades away.\r\n", actor);
		return;
	}
	P_obj object = spell_item_by_uid(item_uid);
	if (object && OBJ_CARRIED_BY(object, actor))
		announce_spell_player_item(actor, object);
}

static bool submit_spell_player_creation(P_char actor, P_obj object)
{
	const uint64_t source_id = spell_creation_source_id();
	if (source_id && item_creation_grant_submit_to_player_with_completion(
				 actor, object, actor, NULL, spell_player_creation_completed,
				 economic_source_kind::spell_creation, source_id))
		return true;
	if (OBJ_NOWHERE(object))
		extract_obj(object, FALSE);
	send_to_char("The item cannot be created right now; please try again later.\r\n", actor);
	return false;
}

static bool has_air_staff_arti(P_char ch)
{
	P_obj staff1, staff2;

	if (IS_NPC(ch))
	{
		return false;
	}

	staff1 = ch->equipment[HOLD];
	staff2 = ch->equipment[WIELD];

	if ((staff1 && staff1->R_num == real_object(67207)) ||
	    (staff2 && staff2->R_num == real_object(67207)))
	{
		return true;
	}

	return false;
}

bool can_conjure_lesser_elem(P_char ch, int /*level*/)
{
	struct follow_type *k;
	P_char victim;
	int i;

	for (k = ch->followers, i = 0; k; k = k->next)
	{
		victim = k->follower;
		/*
		    if(IS_ELEMENTAL(victim))
		    {
		      if(!IS_GREATER_ELEMENTAL(victim))
		      {
		        i++;
		      }
		      else
		      {
		        j++;
		      }
		    }*/
		if (IS_ELEMENTAL(victim) || IS_GREATER_ELEMENTAL(victim))
			i++;
	}
	/*
	  if(GET_LEVEL(ch) >= 56)
	  {
	    j--;
	    i--;
	  }

	  if(GET_LEVEL(ch) >= 60)
	  {
	    j--;
	    i--;
	  }

	  if(GET_C_CHA(ch) >= 200)
	  {
	    send_to_char("Your ability to inspire is amazing.\r\n", ch);
	    j--;
	    i--;
	  }

	  if(j && i >= 2)
	    return FALSE;
	*/
	if (i >= 3)
		return FALSE;

	if (GET_LEVEL(ch) >= 41 && i >= 3)
		return FALSE;
	if ((GET_LEVEL(ch) >= 31) && (GET_LEVEL(ch) < 41) && i >= 2)
		return FALSE;
	if ((GET_LEVEL(ch) >= 21) && (GET_LEVEL(ch) < 31) && i >= 1)
		return FALSE;
	if (GET_LEVEL(ch) < 21)
		return FALSE;

	return TRUE;
}

int can_call_woodland_beings(P_char ch, int /*level*/)
{
	struct char_link_data *cld;
	int pets, allowed;

	for (cld = ch->linked, pets = 0; cld; cld = cld->next_linked)
		if (cld->type == LNK_PET)
			pets++;

	switch (world[ch->in_room].sector_type)
	{
	case SECT_CITY:
	case SECT_DESERT:
	case SECT_ROAD:
		allowed = 1;
		break;
	case SECT_FIELD:
	case SECT_HILLS:
	case SECT_MOUNTAIN:
	case SECT_SWAMP:
	case SECT_UNDRWLD_WILD:
	case SECT_UNDRWLD_MOUNTAIN:
	case SECT_UNDRWLD_SLIME:
		allowed = 2;
		break;
	case SECT_FOREST:
	case SECT_SNOWY_FOREST:
	case SECT_UNDRWLD_MUSHROOM:
		allowed = 3;
		break;
	default:
		return FALSE;
	}

	if (GET_LEVEL(ch) >= 51)
		allowed++;

	return pets < allowed;
}

void spell_call_woodland_beings(int level, P_char ch, char * /*arg*/, int /*type*/,
				P_char /*victim*/, P_obj /*obj*/)
{
	P_char mob;
	int sum, mlvl, lvl;

	static struct
	{
		const int mob_number;
		const char *message;
	} summons[] = { { 1144, "$n &+ywanders in from the wilderness&n." },
			{ 1145, "$n &+cflies in, trailed by &+Wsparkles&n." },
			{ 1146, "$n &+ywanders in from the wilderness&n." },
			{ 1147, "$n &+ywanders in from the wilderness&n." },
			{ 1148, "$n &+ywanders in from the wilderness&n." },
			{ 1149, "$n &+ywanders in from the wilderness&n." },
			{ 1150, "$n &+ywanders in from the wilderness&n." },
			{ 1151, "$n &+ywanders in from the wilderness&n." },
			{ 1152, "$n &+ywanders in from the wilderness&n." },
			{ 1051, "$n &+ywanders in from the wilderness&n." },
			{ 1052, "$n &+ywanders in from the wilderness&n." },
			{ 1053, "$n &+ywanders in from the wilderness&n." },
			{ 1054, "$n &+ywanders in from the wilderness&n." },
			{ 1055, "$n &+ywanders in from the wilderness&n." },
			{ 1056, "$n &+ywanders in from the wilderness&n." },
			{ 1057, "$n &+ywanders in from the wilderness&n." },
			{ 1058, "$n &+ywanders in from the wilderness&n." },
			{ 20, "$n &+ywanders in from the wilderness&n." },
			{ 21, "$n &+ywanders in from the wilderness&n." },
			{ 25, "$n &+ywanders in from the wilderness&n." },
			{ 26, "$n &+ywanders in from the wilderness&n." },
			{ 28, "$n &+ywanders in from the wilderness&n." },
			{ 400, "$n &+ywanders in from the wilderness&n." },
			{ 401, "$n &+ywanders in from the wilderness&n." },
			{ 402, "$n &+ywanders in from the wilderness&n." },
			{ 403, "$n &+ywanders in from the wilderness&n." },
			{ 404, "$n &+ywanders in from the wilderness&n." },
			{ 405, "$n &+ywanders in from the wilderness&n." },
			{ 406, "$n &+ywanders in from the wilderness&n." },
			{ 407, "$n &+ywanders in from the wilderness&n." },
			{ 408, "$n &+ywanders in from the wilderness&n." },
			{ 409, "$n &+ywanders in from the wilderness&n." },
			{ 410, "$n &+ywanders in from the wilderness&n." },
			{ 411, "$n &+ywanders in from the wilderness&n." },
			{ 412, "$n &+ywanders in from the wilderness&n." },
			{ 418, "$n &+ywanders in from the wilderness&n." },
			{ 419, "$n &+ywanders in from the wilderness&n." } };

	if (!can_call_woodland_beings(ch, level))
	{
		send_to_char("No more woodland beings will come to your aid!\n", ch);
		return;
	}

	sum = number(0, 36);

	mob = read_mobile(real_mobile(summons[sum].mob_number), REAL);
	if (!mob)
	{
		logit(LOG_DEBUG, "spell_call_woodland(): mob %d not loadable",
		      summons[sum].mob_number);
		send_to_char("Bug in call woodland beings.  Tell a god!\n", ch);
		return;
	}
	GET_SIZE(mob) = SIZE_MEDIUM;
	mob->player.m_class = CLASS_WARRIOR;

	char_to_room(mob, ch->in_room, 0);
	act(summons[sum].message, TRUE, mob, 0, 0, TO_ROOM);

	mlvl = (level / 5) * 2;
	lvl = number(mlvl, mlvl * 3);

	mob->player.level = BOUNDED(10, lvl, 42);

	GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
		dice(GET_LEVEL(mob) / 2, 8) + GET_LEVEL(mob);

	SET_BIT(mob->specials.affected_by, AFF_INFRAVISION);

	mob->points.base_hitroll = mob->points.hitroll = GET_LEVEL(mob) / 3;
	mob->points.base_damroll = mob->points.damroll = GET_LEVEL(mob) / 3;
	MonkSetSpecialDie(mob); /* 2d6 to 4d5 */
	mob->points.damsizedice = (int)(0.5 * mob->points.damsizedice);

	act("$N acts all friendly around $n!'", TRUE, ch, 0, mob, TO_ROOM);
	act("$N acts all friendly around you!'", TRUE, ch, 0, mob, TO_CHAR);
	setup_pet(mob, ch, 100 / STAT_INDEX(GET_C_INT(mob)), PET_NOCASH);
	add_follower(mob, ch);
}

static void event_elemental_swarm_death(P_char ch, P_char /*victim*/, P_obj /*obj*/,
					void * /*data*/)
{
	act("$n &+rdisappears as &+Lquickly&+r as it came, fading back to its home plane!", TRUE,
	    ch, 0, 0, TO_ROOM);
	extract_char(ch);
}

void spell_elemental_swarm(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			   P_obj /*obj*/)
{
	P_char mob;
	int lvl;

	if (CHAR_IN_SAFE_ROOM(ch))
	{
		send_to_char("A mysterious force blocks your conjuring!\n", ch);
		return;
	}

	mob = read_mobile(number(69, 72), VIRTUAL);
	if (!mob)
	{
		logit(LOG_DEBUG, "spell_conjure_elemental(): mob(s) not loadable");
		send_to_char("Bug in conjure elemental.  Tell a god!\n", ch);
		return;
	}
	GET_SIZE(mob) = SIZE_MEDIUM;
	// mob->player.m_class = 0;

	char_to_room(mob, ch->in_room, 0);
	act("$n &+Lgrunts, and comes forth to join the swarm!!.", TRUE, mob, 0, 0, TO_ROOM);

	lvl = number(level - 3, level + 3);
	mob->player.level = BOUNDED(1, lvl, 51);

	SET_BIT(mob->specials.affected_by, AFF_INFRAVISION);
	SET_BIT(mob->specials.act, ACT_SPEC_DIE);
	GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
		dice(GET_LEVEL(mob), 7) + (GET_LEVEL(mob) * 2);
	GET_EXP(mob) = 0;
	mob->points.base_hitroll = mob->points.hitroll = GET_LEVEL(mob) / 2;
	mob->points.base_damroll = mob->points.damroll = GET_LEVEL(mob) + 10;
	MonkSetSpecialDie(mob); /* 2d6 to 4d5 */
	apply_achievement(mob, TAG_CONJURED_PET);

	if (!can_conjure_lesser_elem(ch, level))
	{
		act("$N &+Lis NOT pleased at being suddenly summoned with this many &+Celementals&+L in the room!&n",
		    TRUE, ch, 0, mob, TO_ROOM);
		act("$N &+Lis NOT pleased with you summon $S with this many &+Celementals&+L in the room!",
		    TRUE, ch, 0, mob, TO_CHAR);
		// Poof in 5-10 sec.
		add_event(event_pet_death, (4 + number(1, 6)) * WAIT_SEC, mob, NULL, NULL, 0, NULL,
			  0);
		MobStartFight(mob, ch);
		return;
	}
	else
	{
		int duration = setup_pet(mob, ch, 1, PET_NOCASH | PET_NOORDER | PET_NOAGGRO);
		add_follower(mob, ch);
		/* if the pet will stop being charmed after a bit, also make it suicide 1-10 minutes later */
		if (duration >= 0)
		{
			duration = number(5, 30) * WAIT_SEC;
			add_event(event_elemental_swarm_death, duration, mob, NULL, NULL, 0, NULL,
				  0);
		}
	}

	if (victim)
	{
		MobStartFight(mob, victim);
	}
	group_add_member(ch, mob);
}

void spell_conjour_elemental(int level, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
			     P_obj /*obj*/)
{
	P_char mob;
	int life = GET_CHAR_SKILL(ch, SKILL_INFUSE_LIFE);
	int charisma = GET_C_CHA(ch) + (GET_LEVEL(ch) / 5);
	int sum, lvl, duration, room = ch->in_room;
	int good_terrain = 0;
	static struct
	{
		const int mob_number;
		const char *message;
	} summons[] = { { 1100, "$n &+Rarrives in a burst of fire." },
			{ 1101, "$n &+yforms from beneath your feet." },
			{ 1102, "&+CA gust of wind solidifies into&n $n." },
			{ 1103, "$n &+Bforms from a puddle in front of you." } };

	if (!IS_ALIVE(ch) || !(room))
	{
		return;
	}

	if (IS_PC_PET(ch))
	{
		send_to_char("Your pet can not summon pets.\n\r", get_linked_char(ch, LNK_PET));
		return;
	}

	if (CHAR_IN_SAFE_ROOM(ch))
	{
		send_to_char("A mysterious force blocks your conjuring!\n", ch);
		return;
	}

	if (!can_conjure_lesser_elem(ch, level))
	{
		send_to_char("You cannot control any more elementals!\n", ch);
		return;
	}

	if (GET_RACE(ch) == RACE_LICH)
	{
		return;
	}

	if (IS_SPECIALIZED(ch))
	{
		switch (ch->player.spec)
		{
		case 1:
			sum = 2;
			break;
		case 2:
			sum = 3;
			break;
		case 3:
			sum = 0;
			break;
		case 4:
			sum = 1;
			break;
		default:
			debug("Invalid spec (%d) on char '%s'.", ch->player.spec, J_NAME(ch));
			return;
			break;
		}
	}
	else
	{
		sum = number(0, 3);
	}

	if (IS_SPECIALIZED(ch) && GET_CLASS(ch, CLASS_SUMMONER) && (IS_PC(ch) || IS_PC_PET(ch)))
	{
		send_to_char(
			"Specialized &+Rsummoners&n use the &+cconjure&n command to manage their minions.\r\n",
			ch);
		return;
	}

	mob = read_mobile(real_mobile(summons[sum].mob_number), REAL);

	if (!mob)
	{
		logit(LOG_DEBUG, "spell_conjure_elemental(): mob %d not loadable",
		      summons[sum].mob_number);
		send_to_char("Bug in conjure elemental.  Tell a god!\n", ch);
		return;
	}

	GET_SIZE(mob) = SIZE_MEDIUM;
	mob->player.m_class = CLASS_WARRIOR;

	char_to_room(mob, room, 0);
	act(summons[sum].message, TRUE, mob, 0, 0, TO_ROOM);

	// Reworking level code for conj pets.
	/*
	mlvl = (level / 5) * 2;
	lvl = number(mlvl, mlvl * 3);

	mob->player.level = BOUNDED(10, lvl, 45);
	*/

	if (number(1, 100) < 20)
		lvl = level + number(2, 5);
	else
		lvl = level - number(-1, 5);

	mob->player.level = BOUNDED(10, lvl, 45);

	MonkSetSpecialDie(mob);

	if (!IS_SET(mob->specials.affected_by, AFF_INFRAVISION))
	{
		SET_BIT(mob->specials.affected_by, AFF_INFRAVISION);
	}

	apply_achievement(mob, TAG_CONJURED_PET);

	good_terrain = conjure_terrain_check(ch, mob);

	if (good_terrain == 1)
	{
		if (IS_SET(mob->specials.affected_by2, AFF2_SLOW))
		{
			REMOVE_BIT(mob->specials.affected_by2, AFF2_SLOW);
		}

		if (!IS_SET(mob->specials.affected_by, AFF_HASTE))
		{
			SET_BIT(mob->specials.affected_by, AFF_HASTE);
		}

		mob->points.base_hitroll = mob->points.hitroll = GET_LEVEL(mob) / 2;
		mob->points.base_damroll = mob->points.damroll = GET_LEVEL(mob) / 2;

		mob->base_stats.Str = 100;
		mob->base_stats.Dex = 100;
		mob->base_stats.Agi = 100;
		mob->base_stats.Pow = 100;

		GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
			(int)(dice(GET_LEVEL(mob) / 2, 12) + 6 * GET_LEVEL(mob) + life + charisma);

		if (GET_C_CHA(ch) > number(0, 400))
		{
			GET_SIZE(mob) = SIZE_LARGE;
			mob->player.spec = 2; // Guardian spec
		}
	}
	else
	{
		mob->points.base_hitroll = mob->points.hitroll = GET_LEVEL(mob) / 3;
		mob->points.base_damroll = mob->points.damroll = GET_LEVEL(mob) / 3;
		GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
			dice(GET_LEVEL(mob) / 2, 10) + 3 * GET_LEVEL(mob) + life + charisma;
		mob->points.damsizedice = (int)(0.8 * mob->points.damsizedice);
	}

	if (IS_PC(ch) && GET_LEVEL(mob) > GET_LEVEL(ch) &&
	    charisma <
		    number(10, (int)(get_property("summon.lesser.elemental.charisma", 140.000))) &&
	    !has_air_staff_arti(ch) && GET_LEVEL(ch) < 50)
	{
		act("$N is NOT pleased at being suddenly summoned against $S will!", TRUE, ch, 0,
		    mob, TO_ROOM);
		act("$N is NOT pleased with you at all!", TRUE, ch, 0, mob, TO_CHAR);
		// Poof in 5-10 sec.
		add_event(event_pet_death, (4 + number(1, 6)) * WAIT_SEC, mob, NULL, NULL, 0, NULL,
			  0);
		MobStartFight(mob, ch);
	}
	else
	{ /* Under control */
		act("$N sulkily says 'Your wish is my command, $n!'", TRUE, ch, 0, mob, TO_ROOM);
		act("$N sulkily says 'Your wish is my command, master!'", TRUE, ch, 0, mob,
		    TO_CHAR);
		duration = setup_pet(mob, ch, 400 / STAT_INDEX(GET_C_INT(mob)), PET_NOCASH);
		add_follower(mob, ch);
		/* if the pet will stop being charmed after a bit, also make it suicide 1-10 minutes later */
		if (duration >= 0)
		{
			duration += number(1, 10);
			add_event(event_pet_death, (duration + 1) * 60 * 4, mob, NULL, NULL, 0,
				  NULL, 0);
		}
	}
}

void event_living_stone_death(P_char ch, P_char /*victim*/, P_obj /*obj*/, void * /*data*/)
{
	act("$n &+rdisappears as &+Lquickly&+r as it came, fading into the thin air!", TRUE, ch, 0,
	    0, TO_ROOM);
	extract_char(ch);
}

void spell_living_stone(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			P_obj /*obj*/)
{
	P_char mob;
	int lvl;

	if (!IS_ALIVE(ch))
	{
		return;
	}

	if (CHAR_IN_SAFE_ROOM(ch))
	{
		send_to_char("A mysterious force blocks your conjuring!\n", ch);
		return;
	}

	mob = read_mobile(real_mobile(1104), REAL);
	if (!mob)
	{
		logit(LOG_DEBUG, "spell_living_stone(): mob 1104 not loadable");
		send_to_char("Bug in spell_living_stone.  Tell a god!\n", ch);
		return;
	}
	GET_SIZE(mob) = SIZE_MEDIUM;
	mob->player.m_class = 0;

	char_to_room(mob, ch->in_room, 0);
	act("$n &+Lcomes to life!.", TRUE, mob, 0, 0, TO_ROOM);

	lvl = number(level - 3, level + 3);

	//  GET_LEVEL(mob) = BOUNDED(10, lvl, 45);
	mob->player.level = BOUNDED(1, lvl, 56);

	SET_BIT(mob->specials.affected_by, AFF_INFRAVISION);
	SET_BIT(mob->specials.act, ACT_SPEC_DIE);
	GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
		dice(GET_LEVEL(mob), 3) + (GET_LEVEL(mob) * 2);
	GET_EXP(mob) = 0;
	mob->points.base_hitroll = mob->points.hitroll = GET_LEVEL(mob) / 2;
	mob->points.base_damroll = mob->points.damroll = GET_LEVEL(mob) + 10;
	MonkSetSpecialDie(mob); /* 2d6 to 4d5 */

	if (!can_conjure_lesser_elem(ch, level))
	{
		act("$N &+Lis NOT pleased at being suddenly summoned with this many &+Rstones&+L in the room!&n",
		    TRUE, ch, 0, mob, TO_ROOM);
		act("$N &+Lis NOT pleased with you summon $S with this many &+Rstone&+L in the room!",
		    TRUE, ch, 0, mob, TO_CHAR);
		// Poof in 5-10 sec.
		add_event(event_pet_death, (4 + number(1, 6)) * WAIT_SEC, mob, NULL, NULL, 0, NULL,
			  0);
		MobStartFight(mob, ch);
		return;
	}
	else
	{
		int duration = setup_pet(mob, ch, 1, PET_NOCASH | PET_NOORDER | PET_NOAGGRO);
		add_follower(mob, ch);
		/* if the pet will stop being charmed after a bit, also make it suicide 1-10 minutes later */
		if (duration >= 0)
		{
			duration = number(5, 10) * WAIT_SEC;
			add_event(event_living_stone_death, duration, mob, NULL, NULL, 0, NULL, 0);
		}
	}

	if (victim)
		MobStartFight(mob, victim);
	group_add_member(ch, mob);
}

void spell_greater_living_stone(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
				P_obj /*obj*/)
{
	P_char mob;
	int lvl;

	if (!IS_ALIVE(ch))
	{
		return;
	}

	if (CHAR_IN_SAFE_ROOM(ch))
	{
		send_to_char("A mysterious force blocks your conjuring!\n", ch);
		return;
	}

	mob = read_mobile(real_mobile(1105), REAL);
	if (!mob)
	{
		logit(LOG_DEBUG, "spell_greater_living_stone(): mob 1105 not loadable");
		send_to_char("Bug in spell_greater_living_stone.  Tell a god!\n", ch);
		return;
	}
	GET_SIZE(mob) = SIZE_MEDIUM;
	mob->player.m_class = 0;

	char_to_room(mob, ch->in_room, 0);
	act("$n &+Lcomes to life!.", TRUE, mob, 0, 0, TO_ROOM);

	lvl = number(level - 3, level + 3);

	mob->player.level = BOUNDED(1, lvl, 56);

	SET_BIT(mob->specials.affected_by, AFF_INFRAVISION);
	SET_BIT(mob->specials.act, ACT_SPEC_DIE);

	GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
		dice(GET_LEVEL(mob), 3) + (GET_LEVEL(mob) * 2);

	mob->points.base_hitroll = mob->points.hitroll = GET_LEVEL(mob) / 2;
	mob->points.base_damroll = mob->points.damroll = GET_LEVEL(mob) + 15;
	mob->points.damnodice = 15;
	mob->points.damsizedice = 14;

	int duration = setup_pet(mob, ch, 1, PET_NOCASH | PET_NOORDER | PET_NOAGGRO);
	add_follower(mob, ch);
	/* if the pet will stop being charmed after a bit, also make it suicide 1-10 minutes later */
	if (duration >= 0)
	{
		duration = number(7, 12) * WAIT_SEC;
		add_event(event_living_stone_death, duration, mob, NULL, NULL, 0, NULL, 0);
	}

	if (victim)
		MobStartFight(mob, victim);

	group_add_member(ch, mob);
}

bool can_conjure_greater_elem(P_char ch, int /*level*/)
{
	int j = 0;
	struct follow_type *k;
	P_char victim;

	for (k = ch->followers, j = 0; k; k = k->next)
	{
		victim = k->follower;
		if (IS_ELEMENTAL(victim) || IS_GREATER_ELEMENTAL(victim))
			j++;
	}
	/*
	  if(GET_LEVEL(ch) >= 56)
	    j--;

	  if(GET_LEVEL(ch) >= 60)
	    j--;

	  if(GET_C_CHA(ch) >= 200)
	  {
	    send_to_char("Your ability to inspire is amazing.\r\n", ch);
	    j--;
	  }


	  if(GET_SPEC(ch, CLASS_CONJURER, SPEC_AIR) &&
	     has_air_staff_arti(ch) &&
	     j <= 2)
	  {
	    return true;
	  }
	  else if(IS_SPECIALIZED(ch) &&
	          GET_CLASS(ch, CLASS_CONJURER) &&
	          j <= 1)
	  {
	    return true;
	  }
	  else if(j > 0)
	  {
	    return FALSE;
	  }

	  return TRUE;
	*/
	if (j >= 3)
		return FALSE;

	if (GET_LEVEL(ch) >= 41 && j >= 3)
		return FALSE;
	if ((GET_LEVEL(ch) >= 31) && (GET_LEVEL(ch) < 41) && j >= 2)
		return FALSE;
	if ((GET_LEVEL(ch) >= 21) && (GET_LEVEL(ch) < 31) && j >= 1)
		return FALSE;
	if (GET_LEVEL(ch) < 21)
		return FALSE;

	return TRUE;
}

int conjure_terrain_check(P_char ch, P_char mob)
{
	if (!ch || !mob || !IS_ALIVE(ch))
		return 0;

	const int room = ch->in_room;
	if (room == NOWHERE)
		return 0;
	int specialization = ch->player.spec;
	if (GET_SPEC(ch, CLASS_SUMMONER, SPEC_MENTALIST))
	{
		switch (GET_RACE(mob))
		{
		case RACE_A_ELEMENTAL:
		case RACE_V_ELEMENTAL:
			specialization = 1;
			break;
		case RACE_W_ELEMENTAL:
		case RACE_I_ELEMENTAL:
			specialization = 2;
			break;
		case RACE_F_ELEMENTAL:
			specialization = 3;
			break;
		case RACE_E_ELEMENTAL:
			specialization = 4;
			break;
		default:
			return 0;
		}
	}
	else if (!GET_CLASS(ch, CLASS_CONJURER) || !specialization)
		return 0;

	switch (specialization)
	{
	case 1: /* AIR conjurer*/

		if (world[room].sector_type == SECT_AIR_PLANE)
		{
			act("$N &+Labsorbs vast quantities of &+Cair &+Lfrom the surrounding area!",
			    TRUE, ch, 0, mob, TO_ROOM);
			act("$N &+Labsorbs vast quantities of &+Cair &+Lfrom the surrounding area!",
			    TRUE, ch, 0, mob, TO_CHAR);
			return 1;
		}
		else if (world[room].sector_type == SECT_EARTH_PLANE)
		{
			act("$N &+Lfeebly tries to draw &+Cair &+Lbut there is so little...", TRUE,
			    ch, 0, mob, TO_ROOM);
			act("$N &+Lfeebly tries to draw &+Cair &+Lbut there is so little...", TRUE,
			    ch, 0, mob, TO_CHAR);
			return -1;
		}
		else
		{
			return 0;
		}
		break;

	case 2: /* WATER conjurer*/
		if (IS_FIRE(room) || world[room].sector_type == SECT_FIREPLANE ||
		    world[room].sector_type == SECT_UNDRWLD_LIQMITH ||
		    world[room].sector_type == SECT_LAVA)
		{
			act("$N &+rfeebly tries to draw &+Bwater &+Lbut there is so little...",
			    TRUE, ch, 0, mob, TO_ROOM);
			act("$N &+rfeebly tries to draw &+Bwater &+Lbut there is so little...",
			    TRUE, ch, 0, mob, TO_CHAR);
			return -1;
		}
		else if (IS_WATER_ROOM(room) || world[room].sector_type == SECT_OCEAN)
		{
			act("$N &+Labsorbs vast quantities of &+Bwater &+Lfrom the surrounding area!",
			    TRUE, ch, 0, mob, TO_ROOM);
			act("$N &+Labsorbs vast quantities of &+Bwater &+Lfrom the surrounding area!",
			    TRUE, ch, 0, mob, TO_CHAR);
			return 1;
		}
		else
			return 0;

		break;

	case 3: /* FIRE conjurer*/

		if (IS_FIRE(room) || world[room].sector_type == SECT_FIREPLANE ||
		    world[room].sector_type == SECT_UNDRWLD_LIQMITH ||
		    world[room].sector_type == SECT_LAVA)
		{
			act("$N &+Labsorbs vast quantities of &+Rfire &+Lfrom the surrounding area!",
			    TRUE, ch, 0, mob, TO_ROOM);
			act("$N &+Labsorbs vast quantities of &+Rfire &+Lfrom the surrounding area!",
			    TRUE, ch, 0, mob, TO_CHAR);
			return 1;
		}
		else if (IS_WATER_ROOM(room) || world[room].sector_type == SECT_OCEAN)
		{
			act("$N &+bfeebly tries to draw &+Rfire &+bbut there is so little...", TRUE,
			    ch, 0, mob, TO_ROOM);
			act("$N &+bfeebly tries to draw &+Rfire &+bbut there is so little...", TRUE,
			    ch, 0, mob, TO_CHAR);
			return -1;
		}
		else
			return 0;

		break;

	case 4: /* EARTH conjurer*/

		if (world[room].sector_type == SECT_EARTH_PLANE)
		{
			act("$N &+Labsorbs vast quantities of &+yearth &+Lfrom the surrounding area!",
			    TRUE, ch, 0, mob, TO_ROOM);
			act("$N &+Labsorbs vast quantities of &+yearth &+Lfrom the surrounding area!",
			    TRUE, ch, 0, mob, TO_CHAR);
			return 1;
		}
		else if (world[room].sector_type == SECT_AIR_PLANE)
		{
			act("$N &+Lfeebly tries to draw &+yearth &+Lbut there is so little...",
			    TRUE, ch, 0, mob, TO_ROOM);
			act("$N &+Lfeebly tries to draw &+yearth &+Lbut there is so little...",
			    TRUE, ch, 0, mob, TO_CHAR);
			return -1;
		}
		else
			return 0;

		break;
	}

	return 0;
}

static void conjure_specialized(P_char ch, [[maybe_unused]] int level)
{
	P_char mob;
	int summoned, room;
	int life = GET_CHAR_SKILL(ch, SKILL_INFUSE_LIFE);
	int charisma = GET_C_CHA(ch) + (GET_LEVEL(ch) / 5);
	int good_terrain = 0;
	const char *summons[] = { "&+CA HUGE gust of wind solidifies into&n $n.",
				  "$n &+Bforms from a nearby lake in front of you.",
				  "$n &+Rarrives in a HUGE burst of flames!",
				  "$n &+yforms from a HUGE chunk of earth!" };
	static struct
	{
		int vnum;
		int hits;
		int damroll;
	} pets[] = // If you add or remove mobs, make sure to adjust
		// IS_GREATER_ELEMENTAL define in utils.h -Lucrot
		{
			{ 1130, 500, 20 }, { 1131, 400, 20 }, { 1132, 600, 20 }, // AIR
			{ 1140, 600, 25 }, { 1141, 600, 20 }, { 1142, 600, 20 }, // WATER
			{ 1110, 600, 20 }, { 1111, 700, 25 }, { 1112, 550, 20 }, // FIRE
			{ 1120, 600, 20 }, { 1121, 800, 30 }, { 1122, 700, 25 }, // EARTH
			/* These are reg pets.
	                             {
	                             43, 500, 20},
	                             {
	                             43, 400, 20},
	                             {
	                             43, 600, 20},             // AIR
	                             {
	                             44, 600, 25},
	                             {
	                             44, 600, 20},
	                             {
	                             44, 600, 20},             // WATER
	                             {
	                             41, 600, 20},
	                             {
	                             41, 700, 25},
	                             {
	                             41, 650, 20},             // FIRE
	                             {
	                             42, 600, 20},
	                             {
	                             42, 800, 30},
	                             {
	                             42, 700, 25},             // EARTH
	                         */
		};

	summoned = 3 * (ch->player.spec - 1) + number(0, 2);
	mob = read_mobile(real_mobile(pets[summoned].vnum), REAL);
	if (ch)
	{
		room = ch->in_room;
	}

	if (!IS_ALIVE(ch) || !(mob) || !(room))
	{
		logit(LOG_DEBUG, "conjure_specialized(): mob %d not loadable", pets[summoned].vnum);
		if (ch)
		{
			send_to_char("Bug in conjour greater elemental.  Tell a god!\n", ch);
		}
		// Don't waste memory.
		if (mob)
		{
			extract_char(mob);
		}
		return;
	}

	char_to_room(mob, room, 0);
	act(summons[ch->player.spec - 1], TRUE, mob, 0, 0, TO_ROOM);

	if (!IS_SET(mob->specials.affected_by, AFF_INFRAVISION))
	{
		SET_BIT(mob->specials.affected_by, AFF_INFRAVISION);
	}

	apply_achievement(mob, TAG_CONJURED_PET);

	if (GET_LEVEL(ch) > 55)
	{
		mob->player.level = (ubyte)number(51, 55);
	}
	else
	{
		mob->player.level = (ubyte)number(49, 53);
	}

	// whew, big bonus for high level mobs!
	if (mob->player.level > 53)
	{
		GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
			pets[summoned].hits * 2 + number(0, 50) + (life * 3) + charisma;
	}
	else if (mob->player.level > 49)
	{
		GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
			pets[summoned].hits + number(0, 50) + (life * 3) + charisma;
	}
	else
		GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
			310 + number(0, 50) + (life * 3) + charisma;

	GET_MAX_HIT(mob) = GET_HIT(mob) = (int)(GET_MAX_HIT(mob) * .66);

	mob->points.base_hitroll = mob->points.hitroll = pets[summoned].damroll + number(0, 5);
	mob->points.base_damroll = mob->points.damroll = pets[summoned].damroll + number(0, 5);
	MonkSetSpecialDie(mob);
	mob->points.damsizedice = (int)(0.8 * mob->points.damsizedice);

	good_terrain = conjure_terrain_check(ch, mob);

	if (good_terrain == -1)
	{
		GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
			(int)(50 + number(1, 100) + (life * 2) + (charisma));
		GET_SIZE(mob) = SIZE_MEDIUM;
	}
	else if (good_terrain == 1)
	{
		if (IS_SET(mob->specials.affected_by2, AFF2_SLOW))
		{
			REMOVE_BIT(mob->specials.affected_by2, AFF2_SLOW);
		}

		if (!IS_SET(mob->specials.affected_by, AFF_HASTE))
		{
			SET_BIT(mob->specials.affected_by, AFF_HASTE);
		}

		mob->points.base_hitroll = mob->points.hitroll =
			pets[summoned].damroll + number(20, 30);
		mob->points.base_damroll = mob->points.damroll =
			pets[summoned].damroll + number(20, 30);
		GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
			(int)(GET_LEVEL(ch) * 30 + number(1, 100) + (life * 4) + (charisma * 2));
		GET_SIZE(mob) = SIZE_HUGE;
		mob->base_stats.Str = 100;
		mob->base_stats.Dex = 100;
		mob->base_stats.Agi = 100;
		mob->base_stats.Pow = 100;

		if (!IS_MULTICLASS_NPC(mob) && !IS_SPECIALIZED(mob) &&
		    GET_CLASS(mob, CLASS_WARRIOR))
		{
			if (number(0, 3))
			{
				mob->player.spec = 2; // Guardian
			}
			else if (number(0, 3))
			{
				mob->player.spec = 1; // Swordsman
			}
			else
			{
				mob->player.spec = 3; // Swashbuckler
			}
		}
	}

	if (IS_PC(ch) && !IS_TRUSTED(ch) && !(has_air_staff_arti(ch)) &&
	    charisma <
		    number(10, (int)(get_property("summon.greater.elemental.charisma", 140.000))))
	{
		act("$N is NOT pleased at being suddenly summoned against $S will!", TRUE, ch, 0,
		    mob, TO_ROOM);
		act("$N is NOT pleased with you at all!", TRUE, ch, 0, mob, TO_CHAR);
		// Poof in 5-10 sec.
		add_event(event_pet_death, (4 + number(1, 6)) * WAIT_SEC, mob, NULL, NULL, 0, NULL,
			  0);
		MobStartFight(mob, ch);
	}
	else
	{
		int duration;
		act("$N says 'I shall serve you for a short time $n!'", TRUE, ch, 0, mob, TO_ROOM);
		act("$N says 'I shall serve you for a short time!'", TRUE, ch, 0, mob, TO_CHAR);

		duration = 400 / STAT_INDEX(GET_C_INT(mob));
		if (good_terrain == 1)
		{
			duration = (duration * 4) / 3;
		}

		if (has_air_staff_arti(ch))
		{
			duration *= 2;
		}
		else
		{
			duration = (duration * GET_C_CHA(ch)) / 100;
		}

		duration = setup_pet(mob, ch, duration, PET_NOCASH);

		add_follower(mob, ch);
		/* if the pet will stop being charmed after a bit, also make it suicide 1-10 minutes later */
		if (duration >= 0)
		{
			duration += number(1, 10);
			add_event(event_pet_death, (duration + 1) * 60 * 4, mob, NULL, NULL, 0,
				  NULL, 0);
		}
	}
}

void spell_conjour_greater_elemental(int level, P_char ch, char * /*arg*/, int /*type*/,
				     P_char /*victim*/, P_obj /*obj*/)
{
	P_char mob;
	int sum, duration, room;
	int life = GET_CHAR_SKILL(ch, SKILL_INFUSE_LIFE);
	int charisma = GET_C_CHA(ch) + (GET_LEVEL(ch) / 5);
	static struct
	{
		const int mob_number;
		const char *message;
	} summons[] = { { 41, "$n &+Rarrives in a HUGE burst of flames!" },
			{ 42, "$n &+yforms from a HUGE chunk of earth!" },
			{ 43, "&+CA HUGE gust of wind solidifies into&n $n." },
			{ 44, "$n &+Bforms from a nearby lake in front of you." } };

	if (!IS_ALIVE(ch))
	{
		return;
	}

	if (IS_PC_PET(ch))
	{
		send_to_char("Your pet can not summon pets.\n\r", get_linked_char(ch, LNK_PET));
		return;
	}

	room = ch->in_room;

	if (!(room) || CHAR_IN_SAFE_ROOM(ch))
	{
		send_to_char("A mysterious force blocks your conjuring!\n", ch);
		return;
	}

	if (!can_conjure_greater_elem(ch, level))
	{
		send_to_char("You may not control more HUGE elementals!\n", ch);
		return;
	}

	if (IS_SPECIALIZED(ch) && GET_CLASS(ch, CLASS_SUMMONER) && (IS_PC(ch) || IS_PC_PET(ch)))
	{
		send_to_char(
			"Specialized &+Rsummoners&n use the &+cconjure&n command to manage their minions.\r\n",
			ch);
		return;
	}

	if (GET_CLASS(ch, CLASS_CONJURER) && IS_SPECIALIZED(ch))
	{
		conjure_specialized(ch, level);
		return;
	}

	sum = number(0, 3);

	if (has_air_staff_arti(ch))
	{
		sum = 2;
	}

	mob = read_mobile(real_mobile(summons[sum].mob_number), REAL);

	if (!mob)
	{
		logit(LOG_DEBUG, "spell_conjure_greater_elemental(): mob %d not loadable",
		      summons[sum].mob_number);
		send_to_char("Bug in conjure greater elemental.  Tell a god!\n", ch);
		return;
	}

	GET_SIZE(mob) = SIZE_LARGE;
	char_to_room(mob, room, 0);
	act(summons[sum].message, TRUE, mob, 0, 0, TO_ROOM);

	mob->player.level = number(49, 53);
	if (mob->player.level == 49)
		GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
			450 + number(0, 50) + (life * 3) + charisma;
	else if (mob->player.level == 50)
		GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
			500 + number(0, 50) + (life * 3) + charisma;
	else if (mob->player.level == 51)
		GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
			550 + number(0, 50) + (life * 3) + charisma;
	else if (mob->player.level == 52)
		GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
			600 + number(0, 50) + (life * 3) + charisma;
	else
		// big bonus for highest level pet, since it's rare
		GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
			700 + number(0, 50) + (life * 3) + charisma;

	SET_BIT(mob->specials.affected_by, AFF_INFRAVISION);

	apply_achievement(mob, TAG_CONJURED_PET);

	mob->points.base_hitroll = mob->points.hitroll = GET_LEVEL(mob) / 3;
	mob->points.base_damroll = mob->points.damroll = GET_LEVEL(mob) / 4;

	MonkSetSpecialDie(mob); /* 2d6 to 4d5 */
	mob->points.damsizedice = (int)(0.8 * mob->points.damsizedice);

	if (IS_PC(ch) && !has_air_staff_arti(ch) && !IS_TRUSTED(ch) &&
	    (charisma + number(0, GET_LEVEL(ch)) <
	     number(10, (int)(get_property("summon.greater.elemental.charisma", 140.000)))))
	{
		act("$N is NOT pleased at being suddenly summoned against $S will!", TRUE, ch, 0,
		    mob, TO_ROOM);
		act("$N is NOT pleased with you at all!", TRUE, ch, 0, mob, TO_CHAR);
		// Poof in 5-10 sec.
		add_event(event_pet_death, (4 + number(1, 6)) * WAIT_SEC, mob, NULL, NULL, 0, NULL,
			  0);
		MobStartFight(mob, ch);
	}
	else
	{ /* Under control */
		act("$N says 'I shall serve you for a short time $n!'", TRUE, ch, 0, mob, TO_ROOM);
		act("$N says 'I shall serve you for a short time!'", TRUE, ch, 0, mob, TO_CHAR);

		duration = setup_pet(mob, ch, 400 / STAT_INDEX(GET_C_INT(mob)), PET_NOCASH);
		add_follower(mob, ch);
		/* if the pet will stop being charmed after a bit, also make it suicide 1-10 minutes later */
		if (duration >= 0)
		{
			duration += number(1, 10);
			add_event(event_pet_death, (duration + 1) * 60 * 4, mob, NULL, NULL, 0,
				  NULL, 0);
		}
	}
}

P_char make_mirror(P_char);
void spell_mirror_image(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			P_obj /*obj*/)
{
	P_char image, tmpch, j;

	/*  int numbp = 0, newslot; */
	/*
	   P_char oldchnext, oldtchnext, prev = NULL, prevtch = NULL, tch, prevch = NULL;
	 */
	int numb, i, c, c2, placement;
	struct follow_type *k;

	if (IS_NPC(ch) && (victim = GET_MASTER(ch)))
	{
		act("$n slaps you savagely across the face, and then kicks you in your pubic area.",
		    FALSE, victim, 0, ch, TO_VICT);
		send_to_char("You feel lame now, don't you?  You should.\n\r", victim);
		return;
	}

	if (IS_ROOM(ch->in_room, ROOM_SINGLE_FILE))
	{
		send_to_char("Ain't enough room here to do that, bubba.\n", ch);
		return;
	}
	for (k = ch->followers; k; k = k->next)
	{
		victim = k->follower;
		if (IS_NPC(victim) && GET_RNUM(victim) == real_mobile(250))
		{
			send_to_char("You can only have one set of mirror images at a time.\n", ch);
			return;
		}
	}

	for (tmpch = world[ch->in_room].people; tmpch; tmpch = tmpch->next_in_room)
	{
		if (IS_NPC(tmpch) && (GET_RNUM(tmpch) == real_mobile(250)) &&
		    (GET_RACEWAR(tmpch) == GET_RACEWAR(ch)))
		{
			send_to_char("The area is fairly cluttered as it is.\n", ch);
			return;
		}
	}

	numb = BOUNDED(1, level / 6, 4);

	for (i = 0; i < numb; i++)
	{
		if (!(image = make_mirror(ch)))
		{
			send_to_char("Your mirror ain't imaging tonight bubba.  let a god know.\n",
				     ch);
			return;
		}
		// reset our variables
		c = 0;
		c2 = 0;
		placement = 0;
		// count people in room for random placement
		for (j = world[ch->in_room].people; j; j = j->next_in_room)
		{
			c++;
		}
		placement = number(0, c);

		// If we are placing it after the first person
		if (placement > 0)
		{
			for (j = world[ch->in_room].people; j; j = j->next_in_room)
			{
				c2++;
				if (c2 == placement)
				{
					image->next_in_room = j->next_in_room;
					j->next_in_room = image;
					break;
				}
			}
		}
		else // Otherwise, we are placing at position 1, which is the beginning of the room
		{
			image->next_in_room = world[ch->in_room].people;
			world[ch->in_room].people = image;
		}
		image->in_room = ch->in_room;

		act("A spitting image of $n suddenly rises from the ground!", TRUE, ch, 0, image,
		    TO_NOTVICT);
		send_to_char("A spitting image of you suddenly rises from the ground!\n", ch);
		add_follower(image, ch);
	}
}

// Utility function...  This is not a spell.  -- Dalreth
P_char make_mirror(P_char ch)
{
	if (training_dummy_is(ch))
		return NULL;

	char Gbuf1[512];
	P_char image = NULL;

	image = read_mobile(real_mobile(250), REAL);
	if (!image)
	{
		return image;
	}

	image->specials.act |= ACT_SPEC_DIE;

	int duration = setup_pet(image, ch, 30, PET_NOCASH);
	/* if the pet will stop being charmed after a bit, also make it suicide 1-10 minutes later */
	if (duration >= 0)
	{
		duration += number(1, 10);
		add_event(event_pet_death, (duration + 1) * 60 * 4, image, NULL, NULL, 0, NULL, 0);
	}

	/* string it */
	image->only.npc->str_mask = (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2);
	snprintf(Gbuf1, 512, "image %s %s", GET_NAME(ch), race_names_table[GET_RACE(ch)].normal);
	image->player.name = str_dup(Gbuf1);
	image->player.short_descr = str_dup(ch->player.name);

	snprintf(Gbuf1, 512, "%s stands here.\n", ch->player.name);
	image->player.long_descr = str_dup(Gbuf1);

	if (GET_TITLE(ch))
	{
		image->player.title = str_dup(GET_TITLE(ch));
	}

	GET_RACE(image) = GET_RACE(ch);
	GET_RACEWAR(image) = GET_RACEWAR(ch);
	GET_SEX(image) = GET_SEX(ch);
	GET_ALIGNMENT(image) = GET_ALIGNMENT(ch);
	GET_SIZE(image) = GET_SIZE(ch);
	// Make them ugly!
	GET_C_CHA(image) = 1;

	return image;
}

int Summonable(P_char ch)
{
	int target;

	if (!ch)
		return FALSE;

	if (training_dummy_is(ch))
		return FALSE;

	if (IS_NPC(ch) && (IS_SET(ch->specials.act, ACT_NO_SUMMON) || IS_SHOPKEEPER(ch)))
		return FALSE;

	if (IS_ROOM(ch->in_room, ROOM_NO_SUMMON))
		return FALSE;

	for (target = 0; target < MAX_WEAR; target++)
		if (ch->equipment[target] &&
		    IS_SET(ch->equipment[target]->extra_flags, ITEM_NOSUMMON))
			return FALSE;

	if (P_char rider = GET_RIDER(ch))
		if (IS_PC(rider))
			return FALSE;

	return TRUE;
}

void spell_summon(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type, P_char victim,
		  P_obj /*obj*/)
{
	int target, max_summon_level = 0;
	struct affected_type af;
	P_char t_ch, mob;
	int distance;

	if (!victim)
	{
		send_to_char("&+CYou failed.\n", ch);
		return;
	}

	/*
	 * cannot be summoned or summon to or from a NO_SUMMON room - Allenbri
	 */

	if (IS_PC(ch) && !IS_ROOM(ch->in_room, ROOM_NO_SUMMON) && !number(0, 9))
	{
		mob = read_mobile(real_mobile(200), REAL);
		if (!mob)
		{
			logit(LOG_DEBUG, "spell_summon(): mob 200 (shadow) not loadable");
			send_to_char("Bug in summon. tell a god!\n", ch);
			return;
		}
		if (!IS_SET(mob->specials.act, ACT_MEMORY))
			clearMemory(mob);

		act("A sudden darkness engulfs the room, and a shape coalesces in it..\n", TRUE, ch,
		    0, 0, TO_ROOM);
		CharWait(mob, 2 * PULSE_VIOLENCE);
		char_to_room(mob, ch->in_room, 0);

		remember(mob, ch);

		remove_plushit_bits(mob);

		act("$n appears in a puff of acrid smoke!", TRUE, mob, 0, 0, TO_ROOM);

		bzero(&af, sizeof(af));
		af.type = SPELL_SUMMON;
		af.duration = 4;
		affect_join(mob, &af, TRUE, FALSE);

		return;
	}

	if (IS_ROOM(ch->in_room, ROOM_NO_SUMMON) || IS_ROOM(victim->in_room, ROOM_NO_SUMMON) ||
	    (IS_NPC(victim) && IS_SHOPKEEPER(victim)) ||
	    (IS_PC(victim) && IS_SET(victim->specials.act2, PLR2_NOLOCATE) &&
	     !is_introd(victim, ch)))
	{
		if (!IS_TRUSTED(ch))
		{
			send_to_char("&+CYou failed.\n", ch);
			act("You feel a magical force tugging on you, which slowly dissipates.",
			    FALSE, ch, 0, victim, TO_VICT);
			return;
		}
	}

	distance = (int)(level * 1.5);

	if ((how_close(ch->in_room, victim->in_room, distance) < 0) &&
	    (how_close(victim->in_room, ch->in_room, distance) < 0))
	{
		send_to_char("&+CYou failed.\n", ch);
		act("You feel a wrenching sensation.", FALSE, ch, 0, victim, TO_VICT);
		return;
	}

	if (racewar(ch, victim) || (IS_PC_PET(ch) && IS_PC(victim)))
	{
		send_to_char("&+CYou failed.\n", ch);
		act("You feel a wrenching sensation.", FALSE, ch, 0, victim, TO_VICT);
		return;
	}

	if (!Summonable(victim) && !(is_linked_to(ch, victim, LNK_CONSENT)))
	{
		send_to_char("&+CYou failed.\n", ch);
		send_to_char("You feel a wrenching sensation.\n", victim);
		return;
	}

	if (!IS_TRUSTED(ch) && IS_NPC(victim) && is_aggr_to(victim, ch))
	{
		/*      (IS_SET(victim->specials.act, ACT_AGGRESSIVE) ||
		    (IS_SET(victim->specials.act, ACT_AGGRESSIVE_EVIL) && IS_EVIL(ch)) ||
		    (IS_SET(victim->specials.act, ACT_AGGRESSIVE_GOOD) && IS_GOOD(ch)) ||
		       (IS_SET(victim->specials.act, ACT_AGGRESSIVE_NEUTRAL) &&
		  IS_NEUTRAL(ch)) ||
		     (IS_SET(victim->specials.act, ACT_AGG_RACEEVIL) && IS_RACEWAR_EVIL(ch)) ||
		    (IS_SET(victim->specials.act, ACT_AGG_RACEGOOD) && IS_RACEWAR_GOOD(ch)))) {*/

		send_to_char("You feel a sudden surge of hatred and halt the spell.\n", ch);
		return;
	}

	if (IS_PC(ch))
		CharWait(ch, 48);
	else
		CharWait(ch, 2 * PULSE_VIOLENCE);

	max_summon_level = level + 3;

	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_SUMMON);
	if ((GET_LEVEL(victim) > MIN(MAXLVLMORTAL, max_summon_level)) || CHAR_IN_PRIV_ZONE(ch) ||
	    CHAR_IN_SAFE_ROOM(victim) ||
	    (!is_linked_to(ch, victim, LNK_CONSENT) && NewSaves(victim, SAVING_SPELL, mod)))
	{
		send_to_char("&+CYou failed.\n", ch);
		send_to_char("You feel a wrenching sensation.\n", victim);
		return;
	}

	act("&+W$n is summoned away!", TRUE, victim, 0, 0, TO_ROOM);

	target = ch->in_room;

	if (IS_FIGHTING(victim))
		stop_fighting(victim);

	if (victim->in_room != NOWHERE)
		for (t_ch = world[victim->in_room].people; t_ch; t_ch = t_ch->next)
			if (IS_FIGHTING(t_ch) && (GET_OPPONENT(t_ch) == victim))
				stop_fighting(t_ch);

	if (IS_PC(victim) && IS_RIDING(victim))
		stop_riding(victim);

	if (P_char rider = GET_RIDER(victim))
		stop_riding(rider);

	act("$n &+Whas summoned you!", FALSE, ch, 0, victim, TO_VICT);
	char_from_room(victim);
	char_to_room(victim, target, -1);
	victim->specials.z_cord = ch->specials.z_cord;
	act("$n &+Warrives suddenly.", TRUE, victim, 0, 0, TO_ROOM);
}

void spell_summon_greater_demon(int level, P_char ch, P_char /*victim*/, P_obj /*obj*/)
{
	P_char mob;
	int sum, mlvl, lvl;
	static struct
	{
		const int mob_number;
		const char *message;
	} summons[] = { { 30, "$n &+rappears in a shower of blood!" },
			{ 31, "$n &+Rbreaks through from beneath the ground!" },
			{ 32, "&+RA huge fireball falls from the sky, forming into &N$n" } };

	if (CHAR_IN_SAFE_ROOM(ch))
	{
		send_to_char("A mysterious force blocks your summoning!\n", ch);
		return;
	}
	if (!can_conjure_greater_elem(ch, level))
	{
		send_to_char("You cannot control any more Demons!\n", ch);
		return;
	}
	sum = number(0, 2);

	mob = read_mobile(real_mobile(summons[sum].mob_number), REAL);
	if (!mob)
	{
		logit(LOG_DEBUG, "spell_summon_greater_demon(): mob %d not loadable",
		      summons[sum].mob_number);
		send_to_char("Bug in summon greater demon.  Tell a god!\n", ch);
		return;
	}
	GET_SIZE(mob) = SIZE_LARGE;
	act(summons[sum].message, TRUE, mob, 0, 0, TO_ROOM);
	mob->points.base_mana = 1000;
	mob->points.mana = 1000;
	SET_BIT(mob->specials.act, ACT_SENTINEL);
	SET_BIT(mob->specials.act, ACT_MEMORY);
	if (IS_SET(mob->specials.act, ACT_IGNORE))
	{
		REMOVE_BIT(mob->specials.act, ACT_IGNORE);
	}

	mlvl = (level / 4) * 2;

	lvl = MIN(50, number(mlvl, mlvl * 3));

	mob->player.level = BOUNDED(10, lvl, 50);

	SET_BIT(mob->specials.affected_by, AFF_INFRAVISION);

	mob->points.base_hitroll = mob->points.hitroll = GET_LEVEL(mob) / 2;
	mob->points.base_damroll = mob->points.damroll = GET_LEVEL(mob) / 2;
	MonkSetSpecialDie(mob);

	char_to_room(mob, ch->in_room, 0);

	if (IS_PC(ch) && /*(GET_LEVEL(mob) > number((level - i * 4), level * 3 / 2)) */
	    !has_air_staff_arti(ch) && !number(0, 300) && !IS_TRUSTED(ch))
	{
		act("$N is NOT pleased at being suddenly summoned against $S will!", TRUE, ch, 0,
		    mob, TO_ROOM);
		act("$N is NOT pleased with you at all!", TRUE, ch, 0, mob, TO_CHAR);
		// Poof in 5-10 sec.
		add_event(event_pet_death, (4 + number(1, 6)) * WAIT_SEC, mob, NULL, NULL, 0, NULL,
			  0);
		MobStartFight(mob, ch);
	}
	else
	{ /* Under control */
		act("$N says 'I shall serve you for a short time $n, and then I shall have you!'",
		    TRUE, ch, 0, mob, TO_ROOM);
		act("$N says 'I shall serve you for a short time, before taking your soul!'", TRUE,
		    ch, 0, mob, TO_CHAR);

		int duration = setup_pet(mob, ch, 30, PET_NOCASH);
		add_follower(mob, ch);
		/* if the pet will stop being charmed after a bit, also make it suicide 1-10 minutes later */
		if (duration >= 0)
		{
			duration += number(1, 10);
			add_event(event_pet_death, (duration + 1) * 60 * 4, mob, NULL, NULL, 0,
				  NULL, 0);
		}
	}
}

void spell_channel(int /*level*/, P_char ch, P_char victim, P_obj obj)
{
	P_char vict, avatar;
	struct affected_type new_af;
	snoop_by_data *snoop_by_ptr;

	if (!ch || !victim || !obj)
		return;
	if (economic_gameplay_authority::active())
	{
		send_to_char("That avatar focus cannot advance right now.\r\n", ch);
		return;
	}

	obj->timer[0]++;

	if (obj->timer[0] > 5 && obj->timer[0] < 10)
	{
		act("$p &+Cbegins to form small particles... a shape can now be seen...", FALSE, ch,
		    0, 0, TO_ROOM);
		act("$p &+Cbegins to form small particles... a shape can now be seen...", FALSE, ch,
		    0, 0, TO_CHAR);
	}
	else if (obj->timer[0] > 10 && obj->timer[0] < 20)
	{
		if (IS_EVIL(ch))
		{
			act("&+LThe orb of darkness begins to take shape... a mounting sense of dread grips the area.",
			    FALSE, ch, 0, 0, TO_ROOM);
			act("&+LThe orb of darkness begins to take shape... a mounting sense of dread grips the area.",
			    FALSE, ch, 0, 0, TO_CHAR);
		}
		else
		{
			act("&+WThe orb of light begins to take shape... a mounting sense of awe grips the area.",
			    FALSE, ch, 0, 0, TO_ROOM);
			act("&+WThe orb of light begins to take shape... a mounting sense of awe grips the area.",
			    FALSE, ch, 0, 0, TO_CHAR);
		}
	}
	else
	{
		act("$p &n&+bbriefly flickers into view...", FALSE, ch, 0, 0, TO_ROOM);
		act("$p &n&+bbriefly flickers into view...", FALSE, ch, 0, 0, TO_CHAR);
	}

	if (obj->timer[0] > 13 && ch == victim)
	{
		// The shit hits the fan here

		// Now switch the leader into the avatar!
		if (IS_EVIL(ch))
			avatar = morph(ch, EVIL_AVATAR_MOB, VIRTUAL);
		else
			avatar = morph(ch, GOOD_AVATAR_MOB, VIRTUAL);

		if (!avatar)
		{
			send_to_char("You are unable to complete the channeling.\n", ch);
			return;
		}

		// get rid of avatar obj
		act("$n &+Bis born from the &n$p!!!", FALSE, avatar, obj, victim, TO_ROOM);
		extract_obj(obj);

		act("&+Y$n's spirit leaves $s body and enters &n$N", FALSE, ch, 0, avatar, TO_ROOM);
		bzero(&new_af, sizeof(new_af));
		new_af.type = SPELL_CHANNEL;
		new_af.duration = 24;
		new_af.location = APPLY_NONE;
		new_af.flags = AFFTYPE_NODISPEL;
		affect_join(avatar, &new_af, FALSE, FALSE);

		// Make helpers in room abort spell, and knock them out
		LOOP_THRU_PEOPLE(vict, ch)
		{
			if (ch == vict || (is_linked_to(ch, victim, LNK_CONSENT) &&
					   GET_CLASS(vict, CLASS_CLERIC)))
			{
				struct affected_type af;

				if (IS_CASTING(vict))
					StopCasting(vict);
				act("Severely drained by the summoning, you collapse.", FALSE, vict,
				    0, 0, TO_CHAR);
				act("$n is severely drained by the summoning, and collapses.",
				    FALSE, vict, 0, 0, TO_ROOM);
				stop_fighting(vict);
				StopMercifulAttackers(vict);
				bzero(&af, sizeof(af));
				af.type = SPELL_CHANNEL;
				af.duration = 2;
				af.location = APPLY_NONE;
				af.bitvector = AFF_KNOCKED_OUT;
				af.flags = AFFTYPE_NODISPEL;
				affect_join(vict, &af, FALSE, FALSE);
				SET_POS(vict, POS_PRONE + GET_STAT(vict));
				vict->only.pc->pc_timer[3] = time(NULL);
				if (ch != vict && avatar->desc)
				{
					vict->desc->snoop.snooping = avatar;
					CREATE(snoop_by_ptr, snoop_by_data, 1, MEM_TAG_SNOOP);
					bzero(snoop_by_ptr, sizeof(snoop_by_data));
					snoop_by_ptr->next = avatar->desc->snoop.snoop_by_list;
					snoop_by_ptr->snoop_by = vict;
					avatar->desc->snoop.snoop_by_list = snoop_by_ptr;
				}
			}
		}
	}
}

void cast_channel(int level, P_char ch, char * /*arg*/, int type, P_char /*tar_ch*/,
		  P_obj /*tar_obj*/)
{
	P_char t_ch, is_head = get_linked_char(ch, LNK_CONSENT);
	P_obj t_obj;
	int num_valid_chars = 0, obj_found = FALSE, obj_num;
	int curr_time = time(NULL);

	switch (type)
	{
	case SPELL_TYPE_SPELL:
		if (!IS_PC(ch))
			return;
		if (ch->only.pc->pc_timer[3] + 7200 > curr_time)
		{
			send_to_char(
				"You have not built up enough energy to summon another diety.\r\n",
				ch);
			return;
		}
		if (!is_head)
		{ // caster is the head
			if ((num_valid_chars = get_multicast_chars(ch, CLASS_CLERIC, 51)) < 3)
			{
				send_to_char(
					"You need more participants to begin the channeling.\r\n",
					ch);
				return;
			}
			else
				t_ch = ch;
		}
		else
		{ // caster is a participant, is_head is leader
			if ((num_valid_chars = get_multicast_chars(is_head, CLASS_CLERIC, 51)) < 4)
			{
				send_to_char(
					"Your channeler needs more participants to begin the channeling.\r\n",
					ch);
				return;
			}
			else
				t_ch = is_head;
		}

		// Ok we have the participants, now check for the object
		if (IS_EVIL(ch))
			obj_num = EVIL_AVATAR_OBJ;
		else
			obj_num = GOOD_AVATAR_OBJ;

		for (t_obj = world[ch->in_room].contents; t_obj; t_obj = t_obj->next_content)
		{
			if (obj_index[t_obj->R_num].virtual_number == obj_num)
			{
				obj_found = TRUE;
				break;
			}
		}
		if (obj_found)
		{
			spell_channel(level, ch, t_ch, t_obj);
			return;
		}
		else if (t_ch == ch)
		{
			if (economic_gameplay_authority::active())
			{
				send_to_char("An avatar focus cannot be formed right now.\r\n", ch);
				return;
			}
			if (IS_EVIL(ch))
				t_obj = read_object(EVIL_AVATAR_OBJ, VIRTUAL);
			else
				t_obj = read_object(GOOD_AVATAR_OBJ, VIRTUAL);
			if (!t_obj)
			{
				send_to_char(
					"Avatar summoning object missing, please tell a god.\r\n",
					ch);
				return;
			}
			t_obj->timer[0] = 0;
			obj_to_room(t_obj, ch->in_room);
			act("$n's eyes roll back in $s head as $e begins the incantation... specs of light begin to form in the room.",
			    FALSE, ch, 0, 0, TO_ROOM);
			act("Your eyes roll back in your head as you begin the incantation... specs of light begin to form in the room.",
			    FALSE, ch, 0, 0, TO_CHAR);
			set_obj_affected(t_obj, 500, TAG_OBJ_DECAY, 0);
			spell_channel(level, ch, t_ch, t_obj);
			return;
		}
		send_to_char("The channeler must begin the incantation.\r\n", ch);
		break;
	case SPELL_TYPE_POTION:
		break;
	case SPELL_TYPE_SCROLL:
		break;
	case SPELL_TYPE_WAND:
		break;
	case SPELL_TYPE_STAFF:
		break;
	default:
		logit(LOG_DEBUG, "Serious screw-up in channel!");
		break;
	}
}

void spell_minor_creation(int /*level*/, P_char ch, P_char /*victim*/, P_obj obj)
{
	if (economic_gameplay_authority::active() && !IS_PC(ch))
	{
		(void)submit_spell_room_creation(ch, obj);
		return;
	}
	SET_BIT(obj->extra2_flags, ITEM2_STOREITEM);
	obj->z_cord = ch->specials.z_cord;
	if (economic_gameplay_authority::active() && IS_PC(ch))
	{
		(void)submit_spell_room_creation(ch, obj);
		return;
	}
	obj_to_room(obj, ch->in_room);
	act("$p &+Wsuddenly appears.", FALSE, ch, obj, 0, TO_ROOM);
	act("$p &+Wsuddenly appears.", FALSE, ch, obj, 0, TO_CHAR);
}

void spell_flame_blade(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
		       P_obj /*obj*/)
{
	P_obj blade;
	if (economic_gameplay_authority::active() && !IS_PC(ch))
		return;

	blade = read_object(real_object(366), REAL);
	if (!blade)
	{
		logit(LOG_DEBUG, "spell_flame_blade(): obj 366 not loadable");
		return;
	}

	blade->extra_flags |= ITEM_NORENT;
	blade->bitvector = 0;
	blade->value[6] = GET_LEVEL(ch);

	/* how about some gay de procs for flame blade? Yeah baby! */
	if (GET_LEVEL(ch) >= 51)
	{
		blade->value[5] = 124;
		blade->value[7] = 30; // procs sunray
	}
	else if (GET_LEVEL(ch) >= 41)
	{
		blade->value[5] = 26;
		blade->value[7] = 25; // procs fireball, better chance.
	}
	else if (GET_LEVEL(ch) >= 36)
	{
		blade->value[5] = 26;
		blade->value[7] = 40; // procs fireball
	}
	else if (GET_LEVEL(ch) >= 21)
	{
		blade->value[5] = 195;
		blade->value[7] = 40; // procs flameburst
	}

	if (GET_LEVEL(ch) >= 56)
	{
		SET_BIT(blade->bitvector2, AFF2_FIRE_AURA);
	}
	if (GET_LEVEL(ch) >= 31)
	{
		SET_BIT(blade->bitvector2, AFF2_FIRESHIELD);
	}
	if (GET_LEVEL(ch) >= 26)
	{
		SET_BIT(blade->bitvector, AFF_PROT_FIRE);
	}

	blade->timer[0] = 180;
	if (IS_PC(ch))
		blade->timer[1] = GET_PID(ch);
	else
		blade->timer[1] = -1;
	if (economic_gameplay_authority::active() && IS_PC(ch))
	{
		(void)submit_spell_player_creation(ch, blade);
		return;
	}

	announce_spell_player_item(ch, blade);
	obj_to_char(blade, ch);
}

void spell_shield(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
		  P_obj /*obj*/)
{
	P_obj shield;
	if (economic_gameplay_authority::active() && !IS_PC(ch))
		return;

	shield = read_object(real_object(368), REAL);
	/*
	   if(!hammer) {
	   logit(LOG_DEBUG, "spell_shield(): obj 368 not loadable");
	   return;
	   }
	 */
	if (!shield)
	{
		logit(LOG_DEBUG, "spell_shield(): obj 368 not loadable");
		return;
	}
	shield->timer[0] = 180;
	if (economic_gameplay_authority::active() && IS_PC(ch))
	{
		(void)submit_spell_player_creation(ch, shield);
		return;
	}

	announce_spell_player_item(ch, shield);
	obj_to_char(shield, ch);
}

void spell_create_food(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
		       P_obj /*obj*/)
{
	P_obj food;
	if (economic_gameplay_authority::active() && !IS_PC(ch))
		return;

	food = read_object(real_object(364), REAL);

	if (!food)
	{
		logit(LOG_DEBUG, "spell_create_food(): obj 364 not loadable");
		return;
	}
	SET_BIT(food->extra_flags, ITEM_NOSELL);
	if (economic_gameplay_authority::active() && IS_PC(ch))
	{
		(void)submit_spell_room_creation(ch, food);
		return;
	}
	obj_to_room(food, ch->in_room);
	act("$p &+Wsuddenly appears.", FALSE, ch, food, 0, TO_ROOM);
	act("$p &+Wsuddenly appears.", FALSE, ch, food, 0, TO_CHAR);
}

static void finish_summon_insects(P_char ch, int room)
{
	send_to_char("&+yYou summon the &+minsects&+y of the area.&n\n", ch);
	act("&+y$n sprinkles some food around to summon the &+minsects&+y of the area.&n\n", 0, ch,
	    0, 0, TO_ROOM);
	struct room_affect af = {};
	af.type = SPELL_SUMMON_INSECTS;
	af.duration = 250;
	af.ch = ch;
	affect_to_room(room, &af);
}

spell_component_effect_status
spell_summon_insects_component_completed(const critical_operation_id & /*operation_id*/, P_char ch,
					 bool committed, const item_transfer_result &,
					 unsigned int /*error_code*/, const uint8_t *encoded,
					 size_t encoded_size)
{
	spell_component_context_reader reader(encoded, encoded_size);
	int32_t room = 0;
	if (!ch || !reader.get_i32(&room) || !reader.finished())
		return spell_component_effect_status::retry;
	if (!committed || ch->in_room != room)
	{
		send_to_char("The mandrake is left intact as your spell fails.\r\n", ch);
		return spell_component_effect_status::complete;
	}
	finish_summon_insects(ch, room);
	return spell_component_effect_status::complete;
}

void spell_summon_insects(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			  P_char /*victim*/, P_obj /*obj*/)
{
	P_obj t_obj, next_obj;
	P_obj used_obj = NULL;
	int count;

	if (!ch || get_spell_from_room(&world[ch->in_room], SPELL_SUMMON_INSECTS))
		return;

	for (count = 0, t_obj = ch->carrying; t_obj; t_obj = next_obj)
	{
		next_obj = t_obj->next_content;

		if (obj_index[t_obj->R_num].virtual_number == VOBJ_FORAGE_MANDRAKE)
		{
			used_obj = t_obj;
			break;
		}

		if (++count > 1000)
			break;
	}

	if (!used_obj)
	{
		send_to_char("You must have &+ya mandrake root&n in your inventory.\r\n", ch);
		return;
	}
	if (economic_gameplay_authority::active() && IS_PC(ch))
	{
		spell_component_context_writer context;
		if (!context.put_i32(ch->in_room))
			return;
		if (!spell_consume_components(ch, VOBJ_FORAGE_MANDRAKE, 1, SPELL_SUMMON_INSECTS,
					      item_spell_component_effect::summon_insects,
					      spell_summon_insects_component_completed,
					      context.data(), context.size))
			send_to_char(
				"Your mandrake cannot be consumed right now; please try again.\r\n",
				ch);
		return;
	}

	extract_obj(used_obj);
	finish_summon_insects(ch, ch->in_room);
}

void spell_doom_blade(int /*level*/, P_char ch, char * /*arg*/, int type, P_char /*victim*/,
		      P_obj /*obj*/)
{
	P_obj weapon = NULL;
	if (economic_gameplay_authority::active() && !IS_PC(ch))
		return;

	debug("doom blade (%d): Cast by: '%s' (%d).", type, J_NAME(ch), GET_ID(ch));

	if (GET_CLASS(ch, CLASS_THEURGIST))
	{
		weapon = read_object(426, VIRTUAL);
		if (!weapon)
		{
			logit(LOG_DEBUG, "spell_doom_blade(): obj 426 not loadable");
			return;
		}
	}
	else
	{
		weapon = read_object(352, VIRTUAL);
		if (!weapon)
		{
			logit(LOG_DEBUG, "spell_doom_blade(): obj 352 not loadable");
			return;
		}

		weapon->timer[0] = 1800;
	}
	if (economic_gameplay_authority::active() && IS_PC(ch))
	{
		(void)submit_spell_player_creation(ch, weapon);
		return;
	}

	announce_spell_player_item(ch, weapon);
	obj_to_char(weapon, ch);
}

static int find_dam_type(char *name)
{
	// If we don't have a string, or no ansi in it.
	if (!name || *name == '\0' || !sub_string_cs(name, "&+"))
		return SPLDAM_GENERIC;

	if (sub_string(name, "&+c") && sub_string(name, "&+l"))
		return SPLDAM_SOUND;
	if (sub_string(name, "&+r"))
		return SPLDAM_FIRE;
	if (sub_string(name, "&+b"))
		return SPLDAM_COLD;
	if (sub_string(name, "&+c"))
		return SPLDAM_COLD;
	if (sub_string_cs(name, "&+Y"))
		return SPLDAM_LIGHTNING;
	if (sub_string_cs(name, "&+G"))
		return SPLDAM_ACID;
	if (sub_string_cs(name, "&+g"))
		return SPLDAM_GAS;
	if (sub_string_cs(name, "&+L"))
		return SPLDAM_NEGATIVE;
	if (sub_string_cs(name, "&+W"))
		return SPLDAM_HOLY;
	if (sub_string(name, "&+m"))
		return SPLDAM_PSI;
	if (sub_string_cs(name, "&+w"))
		return SPLDAM_SPIRIT;
	if (sub_string_cs(name, "&+y"))
		return SPLDAM_EARTH;

	return SPLDAM_GENERIC;
}

struct sticks_to_snakes_context
{
	uint64_t victim_runtime_id;
	uint64_t arrow_uids[7];
	int32_t room;
	int32_t snakes;
	int32_t num_dice;
	int32_t num_sides;
	uint8_t arrow_count;
};

static_assert(sizeof(sticks_to_snakes_context) <= ITEM_MOVEMENT_CONTEXT_MAX_BYTES);

static void sticks_to_snakes_retirement_completed(P_char caster, bool committed,
						  const item_transfer_result &,
						  unsigned int /*error_code*/,
						  const uint8_t *encoded, size_t encoded_size)
{
	if (!encoded || encoded_size != sizeof(sticks_to_snakes_context))
		return;
	sticks_to_snakes_context context = {};
	memcpy(&context, encoded, sizeof(context));
	if (!committed)
	{
		if (caster)
			send_to_char("Your spell fails and the arrows remain unchanged.\r\n",
				     caster);
		return;
	}

	P_char victim = find_character_by_runtime_id(context.victim_runtime_id);
	struct damage_messages arrow_messages = {
		"You turn $N's $q into a &+gsnake&n and send it against $M!",
		"Your own $q turns into a &+gsnake&n and bites you, &+Lvanishing afterwards&n!",
		"$N's own $q turns into a &+gsnake&n and bites $M, &+Lvanishing afterwards&n!",
		"You turn $N's $q into a &+gsnake&n and it bites $M to death!",
		"Your own $q turns into a &+gsnake&n and bites you &+rrea&+Rlly &+Lhard...",
		"$N's own $q turns into a &+gsnake&n and it bites $M to &+rdeath&n, &+Lvanishing afterwards&n!"
	};
	struct damage_messages messages = {
		"&+yYou turn a stick into a &+gsnake &+yand send it against $N!",
		"A &+ystick&n turns into a &+gsnake &nand bites you, &+Lvanishing afterwards&n!",
		"A &+ystick&n turns into a &+gsnake &nand bites $N, &+Lvanishing afterwards&n!",
		"&+yYou turn a stick into a &+gsnake &+yand send it against $N!",
		"A &+ystick&n turns into a &+gsnake &nand bites you, &+Lvanishing afterwards&n!",
		"A &+ystick&n turns into a &+gsnake &nand bites $N, &+Lvanishing afterwards&n!"
	};
	for (size_t index = 0; index < context.arrow_count; ++index)
	{
		P_obj arrow = spell_item_by_uid(context.arrow_uids[index]);
		if (victim && arrow && IS_ALIVE(victim) && is_char_in_room(victim, context.room))
		{
			arrow_messages.obj = arrow;
			const int damage_type = find_dam_type(OBJ_SHORT(arrow));
			spell_damage(caster, victim, 5 * dice(arrow->value[1], arrow->value[2]),
				     damage_type, SPLDAM_ALLGLOBES, &arrow_messages);
		}
		if (arrow)
			extract_obj(arrow);
	}
	while (victim && context.snakes && IS_ALIVE(victim) &&
	       is_char_in_room(victim, context.room))
	{
		spell_damage(caster, victim, dice(context.num_dice, context.num_sides),
			     SPLDAM_GENERIC, SPLDAM_ALLGLOBES, &messages);
		--context.snakes;
	}
}

void spell_sticks_to_snakes(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			    P_char victim, P_obj obj)
{
	int snakes, arrowSnakes, room, num_dice, num_sides, dam_type;
	P_obj arrows, inven, next_inven;
	P_obj selected_arrows[7] = {};
	size_t selected_arrow_count = 0;

	struct damage_messages arrow_messages = {
		"You turn $N's $q into a &+gsnake&n and send it against $M!",
		"Your own $q turns into a &+gsnake&n and bites you, &+Lvanishing afterwards&n!",
		"$N's own $q turns into a &+gsnake&n and bites $M, &+Lvanishing afterwards&n!",
		"You turn $N's $q into a &+gsnake&n and it bites $M to death!",
		"Your own $q turns into a &+gsnake&n and bites you &+rrea&+Rlly &+Lhard...",
		"$N's own $q turns into a &+gsnake&n and bites $M to &+rdeath&n, &+Lvanishing afterwards&n!"
	};
	struct damage_messages messages = {
		"&+yYou turn a stick into a &+gsnake &+yand send it against $N!",
		"A &+ystick&n turns into a &+gsnake &nand bites you, &+Lvanishing afterwards&n!",
		"A &+ystick&n turns into a &+gsnake &nand bites $N, &+Lvanishing afterwards&n!",
		"&+yYou turn a stick into a &+gsnake &+yand send it against $N!",
		"A &+ystick&n turns into a &+gsnake &nand bites you, &+Lvanishing afterwards&n!",
		"A &+ystick&n turns into a &+gsnake &nand bites $N, &+Lvanishing afterwards&n!"
	};
	snakes = arrowSnakes = 0;

	room = victim->in_room;

	if (level > 50)
	{
		num_dice = 8;
		num_sides = 8;
	}
	else if (level > 40)
	{
		num_dice = 8;
		num_sides = 7;
	}
	else if (level > 30)
	{
		num_dice = 7;
		num_sides = 7;
	}
	else if (level > 25)
	{
		num_dice = 7;
		num_sides = 6;
	}
	else if (level > 20)
	{
		num_dice = 6;
		num_sides = 6;
	}
	else if (level > 15)
	{
		num_dice = 6;
		num_sides = 5;
	}
	else
	{
		num_dice = 5;
		num_sides = 5;
	}

	switch (world[ch->in_room].sector_type)
	{
	case SECT_CITY:
	case SECT_ROAD:
		snakes = number(1, 2);
		break;
	case SECT_FIELD:
		snakes = number(1, 5);
		break;
	case SECT_FOREST:
		snakes = number(1, 6);
		break;
	case SECT_HILLS:
		snakes = number(1, 4);
		break;
	case SECT_UNDERWATER_GR:
	case SECT_MOUNTAIN:
		snakes = number(1, 3);
		break;
	case SECT_UNDRWLD_WILD:
	case SECT_UNDRWLD_CITY:
	case SECT_UNDRWLD_MOUNTAIN:
	case SECT_UNDRWLD_SLIME:
	case SECT_UNDRWLD_LOWCEIL:
	case SECT_UNDRWLD_LIQMITH:
	case SECT_UNDRWLD_MUSHROOM:
		snakes = number(1, 2);
		break;
	case SECT_INSIDE:
	case SECT_UNDRWLD_INSIDE:
	default:
		snakes = 1;
		break;
	}

	obj = victim->carrying;

	arrows = NULL;
	for (inven = victim->carrying; inven != NULL; inven = next_inven)
	{
		next_inven = inven->next_content;

		// Artifact arrows?
		if (IS_ARTIFACT(inven) || inven->type != ITEM_MISSILE ||
		    inven->value[3] != MISSILE_ARROW)
		{
			continue;
		}
		if (economic_gameplay_authority::active() && IS_PC(victim))
			selected_arrows[selected_arrow_count++] = inven;
		else
		{
			obj_from_char(inven);
			inven->next_content = arrows;
			arrows = inven;
		}
		// Allow 8 total snakes.
		if (++arrowSnakes >= 8 - snakes)
			break;
	}
	if (selected_arrow_count)
	{
		if (!IS_PC(ch) || GET_PID(victim) <= 0)
		{
			send_to_char("The spell cannot consume these arrows right now.\r\n", ch);
			return;
		}
		sticks_to_snakes_context context = {};
		context.victim_runtime_id = victim->runtime_id;
		context.room = room;
		context.snakes = snakes;
		context.num_dice = num_dice;
		context.num_sides = num_sides;
		context.arrow_count = static_cast<uint8_t>(selected_arrow_count);
		for (size_t index = 0; index < selected_arrow_count; ++index)
			context.arrow_uids[index] = selected_arrows[index]->obj_uid;
		const item_owner_identity player_owner = { item_owner_type::player,
							   static_cast<uint64_t>(GET_PID(victim)),
							   0 };
		const item_owner_identity destruction = { item_owner_type::destruction, 0, 0 };
		item_movement_reject reject = item_movement_reject::none;
		if (!item_movement_transaction_submit_batch(
			    ch, selected_arrows, selected_arrow_count, NULL, player_owner,
			    destruction, item_transfer_reason::destruction, SPELL_STICKS_TO_SNAKES,
			    sticks_to_snakes_retirement_completed, &context, sizeof(context), NULL,
			    &reject, nullptr, economic_source_kind::spell_consumption))
		{
			send_to_char("Your spell fails and the arrows remain unchanged.\r\n", ch);
			logit(LOG_FILE,
			      "sticks-to-snakes item retirement refused (pid=%d reason=%s)",
			      GET_PID(ch), item_movement_reject_name(reject));
		}
		return;
	}

	while (arrows && IS_ALIVE(victim))
	{
		arrow_messages.obj = arrows;
		dam_type = find_dam_type(OBJ_SHORT(arrows));
		// Increase the regular arrow damage by 25%.
		spell_damage(ch, victim, 5 * dice(arrows->value[1], arrows->value[2]), dam_type,
			     SPLDAM_ALLGLOBES, &arrow_messages);
		obj = arrows;
		arrows = arrows->next_content;
		obj->next_content = NULL;
		extract_obj(obj);
	}
	while (snakes && is_char_in_room(victim, room))
	{
		spell_damage(ch, victim, dice(num_dice, num_sides), SPLDAM_GENERIC,
			     SPLDAM_ALLGLOBES, &messages);
		snakes--;
	}
}

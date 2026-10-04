#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "combat/justice.h"
#include "economy/economic_gameplay_authority.h"
#include "magic/spells.h"
#include "sql/sql.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

extern P_room world;
extern struct zone_data *zone_table;
int portal_id;

struct portal_data
{
	int pid;
	bool oneway;
};

static void set_up_portals(P_char ch, P_obj p1, P_obj p2, int charge);

void spell_shadow_gate(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		       P_obj /*obj*/)
{
	struct portal_settings set = {
		782, /* portal type  */
		-1, /* from room */
		-1, /* to room */
		0, /* How many can pass before closes */
		0, /* Timeout before anyone can enter after open */
		0, /* Timeout before next person can enter */
		0, /* Lag person gets when steps out portal */
		0 /* Portal decay timer */
	};
	struct portal_create_messages msg = {
		/*ch   */ "&+LThe gate opens for a brief second and then closes.\n",
		/*ch r */ "&+LA shadow gate appears for a brief second, then closes.",
		/*vic  */ 0,
		/*vic r*/ 0,
		/*ch   */ "&+LA pitch black gate rises out of the ground!\n",
		/*ch r */ "&+LA pitch black gate rises out of the ground!\n",
		/*vic  */ "&+LA pitch black gate rises out of the ground!\n",
		/*vic r*/ "&+LA pitch black gate rises out of the ground!\n",
		/*npc  */ "You can only open a shadow gate to another player!\n",
		/*bad  */ 0
	};
	struct affected_type *afp;

	if (!ch)
		return;

	if (!victim)
		victim = ch;

	int specBonus = 0;
	set.to_room = victim->in_room;
	if (victim == ch)
	{
		if (affected_by_spell(ch, SPELL_BLOODSTONE))
		{
			afp = get_spell_from_char(ch, SPELL_BLOODSTONE);
			set.to_room = afp->modifier;
		}
	}
	int maxToPass = get_property("portals.shadowportal.maxToPass", 5);
	set.init_timeout = get_property("portals.shadowportal.initTimeout", 3);
	set.post_enter_timeout = get_property("portals.shadowportal.postEnterTimeout", 0);
	set.post_enter_lag = get_property("portals.shadowportal.postEnterLag", 0);
	set.decay_timer = get_property("portals.shadowportal.decayTimeout", 60 * 2);
	set.throughput = MAX(0, (int)((ch->player.level - 46))) + number(2, maxToPass + specBonus);

	if (!can_do_general_portal(level, ch, victim, &set, &msg)
	    //                || (!IS_TRUSTED(ch)     && (GET_MASTER(ch) && IS_PC(victim)) )
	    //                || (!IS_TRUSTED(ch)     && (!OUTSIDE(ch) || !OUTSIDE(victim)) )
	)
	{
		act(msg.fail_to_caster, FALSE, ch, 0, 0, TO_CHAR);
		act(msg.fail_to_caster_room, FALSE, ch, 0, 0, TO_ROOM);
		return;
	}

	if (IS_NPC(victim) && !IS_TRUSTED(ch))
	{
		send_to_char(msg.npc_target_caster, ch);
		return;
	}

	spell_general_portal(level, ch, victim, &set, &msg);
}

void spell_moonwell(int level, P_char ch, char *arg, int /*type*/, P_char victim, P_obj /*obj*/)
{
	struct affected_type *afp;
	int to_room = NOWHERE;
	struct portal_settings set = {
		751, /* portal type  */
		-1, /* from room */
		-1, /* to room */
		0, /* How many can pass before closes */
		0, /* Timeout before anyone can enter after open */
		0, /* Timeout before next person can enter */
		0, /* Lag person gets when steps out portal */
		0 /* Portal decay timer */
	};
	struct portal_create_messages msg = {
		/*ch   */ "The well opens for a brief second and then closes.\n",
		/*ch r */ "A moonwell appears for a brief second, then closes.\n",
		/*vic  */ 0,
		/*vic r*/ 0,
		/*ch   */
		"&+WSwirling, silvery mists fill the area, slowly forming a pool on the ground..\n",
		/*ch r */
		"&+WSwirling, silvery mists fill the area, slowly forming a pool on the ground..\n",
		/*vic  */
		"&+WSwirling, silvery mists fill the area, slowly forming a pool on the ground..\n",
		/*vic r*/
		"&+WSwirling, silvery mists fill the area, slowly forming a pool on the ground..\n",
		/*npc  */ "You can only open a moonwell to another player!\n",
		/*bad  */ 0
	};

	if (!ch)
		return;

	if (!victim)
		victim = ch;
	else if (IS_NPC(victim) && arg && !str_cmp(arg, "moonstone") &&
		 affected_by_spell(ch, SPELL_MOONSTONE))
		victim = ch;

	if (IS_NPC(ch))
		return;

	if (IS_NPC(victim))
		return;

	if (victim == ch)
	{
		if (affected_by_spell(ch, SPELL_MOONSTONE))
		{
			afp = get_spell_from_char(ch, SPELL_MOONSTONE);
			to_room = afp->modifier;
			/* Removing this for now, as there's no need for checking this.. maybe later
			 * for expanding the spell.
			for (moonstone = world[to_room].contents; moonstone; moonstone = moonstone->next_content)
			{
			  if( moonstone && obj_index[moonstone->R_num].virtual_number == 419 && moonstone->value[0] == GET_PID(ch))
			    break;
			}
		  }
		  if(!moonstone)
			//success = false;
		  else
			success = false;
		  */
		}
	}
	else
	{
		to_room = victim->in_room;
	}
	set.to_room = to_room;
	set.init_timeout = get_property("portals.moonwell.initTimeout", 3);
	set.post_enter_timeout = get_property("portals.moonwell.postEnterTimeout", 0);
	set.post_enter_lag = get_property("portals.moonwell.postEnterLag", 0);
	set.decay_timer = get_property("portals.moonwell.decayTimeout", 60 * 2);

	//--------------------------------
	// spec affected changes
	//--------------------------------
	if (GET_SPEC(ch, CLASS_DRUID, SPEC_WOODLAND) &&
	    world[ch->in_room].sector_type == SECT_FOREST)
	{
		set.decay_timer = (set.decay_timer / 2) * 3;
	}
	//--------------------------------
	// set.throughput = MAX(0, (int)( (ch->player.level-46)/2 )) + number( 2, maxToPass + specBonus);
	set.throughput = 20;

	if (!can_do_general_portal(level, ch, victim, &set, &msg)
	    //                || (!IS_TRUSTED(ch)     && (GET_MASTER(ch) && IS_PC(victim)) )
	    //                || (!IS_TRUSTED(ch)     && (!OUTSIDE(ch) || !OUTSIDE(victim)) )
	)
	{
		act(msg.fail_to_caster, FALSE, ch, 0, 0, TO_CHAR);
		act(msg.fail_to_caster_room, FALSE, ch, 0, 0, TO_ROOM);
		return;
	}

	if (IS_NPC(victim) && !IS_TRUSTED(ch))
	{
		send_to_char(msg.npc_target_caster, ch);
		return;
	}

	spell_general_portal(level, ch, victim, &set, &msg);
}

void spell_moonstone(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		     P_char /*victim*/, P_obj /*tar_obj*/)
{
	P_obj moonstone;
	struct affected_type af, *afp;
	int duration = level * 4 * WAIT_MIN;

	if (IS_NPC(ch))
		return;

	if (IS_ROOM(ch->in_room, ROOM_NO_TELEPORT) || world[ch->in_room].sector_type == SECT_OCEAN)
	{
		send_to_char("The powers of nature ignore your call for serenity.\n", ch);
		return;
	}
	if (economic_gameplay_authority::active())
	{
		send_to_char("A moonstone cannot be formed right now.\r\n", ch);
		return;
	}

	if ((afp = get_spell_from_char(ch, SPELL_MOONSTONE)))
	{
		moonstone = get_obj_in_list_num(real_object(419), world[afp->modifier].contents);
		if (moonstone)
			extract_obj(moonstone);
		afp->modifier = ch->in_room;
	}
	else
	{
		memset(&af, 0, sizeof(af));
		af.type = SPELL_MOONSTONE;
		af.flags = /*AFFTYPE_NOSHOW |*/ AFFTYPE_NOSAVE | AFFTYPE_NODISPEL | AFFTYPE_NOAPPLY;
		// af.flags = /*AFFTYPE_NOSHOW |*/ AFFTYPE_NOSAVE | AFFTYPE_NOAPPLY;
		af.modifier = ch->in_room;
		af.duration = duration / PULSES_IN_TICK;

		affect_to_char(ch, &af);
	}

	moonstone = read_object(real_object(419), REAL);

	if (!moonstone)
	{
		logit(LOG_DEBUG, "spell_moonstone(): obj 419 not loadable");
		return;
	}

	send_to_char(
		"&+BA shimmering stone begins to take shape....\n"
		"&+bThe stone rises into the &+cair&+b briefly, then shoots downward with amazing speed into the ground...\n"
		"&+CYou feel at one with the surroundings.\n",
		ch);
	act("&+BA shimmering stone begins to take shape...\n"
	    "&+bThe stone rises into the air briefly, then shoots downward with amazing speed into the ground.\n"
	    "&+C$n glows with &n&+bpower.",
	    FALSE, ch, 0, 0, TO_ROOM);

	set_obj_affected(moonstone, duration, TAG_OBJ_DECAY, 0);

	moonstone->value[0] = GET_PID(ch);

	obj_to_room(moonstone, ch->in_room);
}

bool can_do_general_portal(int /*level*/, P_char ch, P_char victim,
			   struct portal_settings *settings,
			   struct portal_create_messages * /*messages*/)
{
	int to_room;

	if (!IS_ALIVE(ch))
	{
		return FALSE;
	}

	if (ch == victim && !affected_by_spell(ch, SPELL_MOONSTONE) &&
	    !affected_by_spell(ch, SPELL_THOUGHT_BEACON) &&
	    !affected_by_spell(ch, SPELL_BLOODSTONE))
	{
		return FALSE;
	}

	to_room = settings->to_room;
	if (!IS_TRUSTED(ch) &&
	    // target room check
	    ((to_room == NOWHERE) || (to_room == ch->in_room) ||
	     IS_ROOM(ch->in_room, ROOM_NO_TELEPORT) || IS_HOMETOWN(ch->in_room) ||
	     IS_HOMETOWN(to_room) || IS_ROOM(to_room, ROOM_SINGLE_FILE) ||
	     world[ch->in_room].sector_type == SECT_OCEAN || IS_ROOM(to_room, ROOM_NO_MAGIC) ||
	     IS_ROOM(to_room, ROOM_NO_TELEPORT) ||
	     //        IS_NPC(ch) ||
	     IS_PC_PET(ch) ||
	     // victim check
	     (victim && IS_ELITE(victim) && IS_GREATER_RACE(victim) &&
	      (IS_TRUSTED(victim) || IS_AFFECTED3(victim, AFF3_NON_DETECTION) ||
	       (IS_SET(victim->specials.act2, PLR2_NOLOCATE) &&
		!is_linked_to(ch, victim, LNK_CONSENT)) ||
	       (IS_PC(victim) && IS_SET(victim->specials.act2, PLR2_NOLOCATE) &&
		!is_introd(victim, ch)) ||
	       (racewar(ch, victim) &&
		!race_portal_check(ch, victim))) /* check used if victim is set */
	      )) /* check used if not trusted ch */
	)
	{
		return FALSE;
	}

	return TRUE;
}

static void event_portal_owner_check(P_char /*ch*/, P_char /*vict*/, P_obj obj, void *data)
{
	struct portal_data *pdata = (struct portal_data *)data;
	P_char caster;

	if (!pdata)
	{
		debug("Passed null pointer to portal owner check.");
		return;
	}

	caster = find_player_by_pid(pdata->pid);
	if (!caster)
	{
		Decay(obj);
		return;
	}
	// The event owns obj, but the opposite object may already have decayed.
	// Its destination records the other end without dereferencing a stale pointer.
	if (!OBJ_ROOM(obj) || (obj->loc.room != caster->in_room &&
			       (pdata->oneway || real_room(obj->value[0]) != caster->in_room)))
	{
		Decay(obj);
		return;
	}
	add_event(event_portal_owner_check, WAIT_SEC, 0, 0, obj, 0, data,
		  sizeof(struct portal_data));
}

bool spell_general_portal(int /*level*/, P_char ch, P_char victim, struct portal_settings *settings,
			  struct portal_create_messages *messages, bool isOneWay)
{
	/* Portal obj settings
	 * extra2_flags - not used anymore, was create time
	 * value[0] - room number
	 * value[1] - race
	 * value[2] - throughput
	 * value[3] - creator level
	 * value[4] = init_timeout
	 * value[5] = post_enter_timeout
	 * value[6] = post_enter_lag
	 * value[7] - portal ID
	 * timer[0] - create time
	 * timer[1] - last entry time
	 */
	P_obj portal1 = NULL, portal2 = NULL;
	int to_room;
	char logbuf[500];
	struct portal_data pdata;

	if (!ch)
	{
		return FALSE;
	}

	to_room = settings->to_room;
	// NOWHERE is a bad index, 0 is Limbo
	if (to_room == NOWHERE || to_room == 0)
	{
		if (victim)
		{
			snprintf(logbuf, 500, "Portal(%d) from %s(%d) [%d] to %s(%d) in [%s].",
				 settings->R_num, J_NAME(ch),
				 IS_NPC(ch) ? GET_VNUM(ch) : GET_PID(ch), world[ch->in_room].number,
				 J_NAME(victim),
				 IS_NPC(victim) ? GET_VNUM(victim) : GET_PID(victim),
				 (to_room == NOWHERE) ? "NOWHERE" : "LIMBO");
			logit(LOG_PORTALS, "%s", logbuf);
			send_to_char("Spell messed up. contact someone.\n", ch);
			return FALSE;
		}
		else
		{
			snprintf(logbuf, 500, "Portal(%d) from %s(%d) [%d] to [%s].",
				 settings->R_num, J_NAME(ch),
				 IS_NPC(ch) ? GET_VNUM(ch) : GET_PID(ch), world[ch->in_room].number,
				 (to_room == NOWHERE) ? "NOWHERE" : "LIMBO");
			logit(LOG_PORTALS, "%s", logbuf);
			send_to_char("Spell messed up. contact someone.\n", ch);
			return FALSE;
		}
	}

	if (IS_CASTLE(ch->in_room))
	{
		send_to_char("&+LThe nature of this room prevents you from creating a portal.&n\n",
			     ch);
		return FALSE;
	}
	if (IS_CASTLE(to_room))
	{
		send_to_char("&+LThe nature of that room prevents you from creating a portal.&n\n",
			     ch);
		return FALSE;
	}
	if (economic_gameplay_authority::active())
	{
		send_to_char("A portal cannot be formed right now.\r\n", ch);
		return FALSE;
	}

	portal1 = read_object(settings->R_num, VIRTUAL);
	if (!portal1)
	{
		snprintf(logbuf, 500, "spell_portal(): obj %d not loadable", settings->R_num);
		logit(LOG_DEBUG, "%s", logbuf);
		send_to_char("Spell messed up. contact someone.\n", ch);
		return FALSE;
	}
	if (!isOneWay)
	{
		portal2 = read_object(settings->R_num, VIRTUAL);
		if (!portal2)
		{
			snprintf(logbuf, 500, "spell_portal(): obj %d not loadable",
				 settings->R_num);
			logit(LOG_DEBUG, "%s", logbuf);
			send_to_char("Spell messed up. contact someone.\n", ch);
			if (portal1)
			{
				extract_obj(portal1);
			}
			return FALSE;
		}
	}

	if (victim && !IS_TRUSTED(ch))
	{
		snprintf(logbuf, 500, "Portal(%d) from %s(%d) in [%d] to %s(%d) in [%d].",
			 settings->R_num, J_NAME(ch), IS_NPC(ch) ? GET_VNUM(ch) : GET_PID(ch),
			 world[ch->in_room].number, J_NAME(victim),
			 IS_NPC(victim) ? GET_VNUM(victim) : GET_PID(victim),
			 world[to_room].number);
		logit(LOG_PORTALS, "%s", logbuf);
		sql_log(ch, PLAYERLOG, "Portal (%d) to %s in %d", settings->R_num, J_NAME(victim),
			world[to_room].number);
		// spam immo's if it looks like a possible camped target
		if ((world[to_room].number == GET_HOME(victim)) || (GET_LEVEL(victim) < 10))
		{
			statuslog(57, "%s", logbuf);
		}
	}
	if (world[to_room].people)
	{
		if (victim && (ch != victim))
		{
			act(messages->open_to_victim_room, FALSE, ch, portal1, victim,
			    TO_NOTVICTROOM);
			act(messages->open_to_victim, FALSE, ch, portal1, victim, TO_VICT);
		}
		else
		{
			act(messages->open_to_victim_room, FALSE, world[to_room].people, portal1, 0,
			    TO_ROOM);
			act(messages->open_to_victim_room, FALSE, world[to_room].people, portal1, 0,
			    TO_CHAR);
		}
	}
	act(messages->open_to_caster_room, FALSE, ch, portal1, victim, TO_ROOM);
	act(messages->open_to_caster, FALSE, ch, portal1, victim, TO_CHAR);

	portal1->value[0] = world[to_room].number;

	// set timers
	portal1->value[4] = settings->init_timeout;
	portal1->value[5] = settings->post_enter_timeout;
	portal1->value[6] = settings->post_enter_lag;

	set_obj_affected(portal1, settings->decay_timer, TAG_OBJ_DECAY, 0);

	if (!isOneWay)
	{
		portal2->value[0] = world[ch->in_room].number;
		// set timers
		portal2->value[4] = settings->init_timeout;
		portal2->value[5] = settings->post_enter_timeout;
		portal2->value[6] = settings->post_enter_lag;
		set_obj_affected(portal2, settings->decay_timer, TAG_OBJ_DECAY, 0);
	}

	set_up_portals(ch, portal1, portal2, settings->throughput);

	obj_to_room(portal1, ch->in_room);
	if (!isOneWay)
		obj_to_room(portal2, to_room);

	if (IS_PC(ch) && (settings->R_num != 752))
	{
		pdata.pid = GET_PID(ch);
		pdata.oneway = isOneWay;
		add_event(event_portal_owner_check, WAIT_SEC, 0, 0, portal1, 0, &pdata,
			  sizeof(pdata));
		if (portal2)
			add_event(event_portal_owner_check, WAIT_SEC, 0, 0, portal2, 0, &pdata,
				  sizeof(pdata));
	}

	return TRUE;
}

static void set_up_portals(P_char ch, P_obj p1, P_obj p2, int charge)
{
	// set when portal is created, for initial stabilization
	p1->timer[0] = time(0);
	if (p2)
		p2->timer[0] = p1->timer[0];
	//----------------------------

	// reset entry timer, no stabilization for first entry
	p1->timer[1] = 0;
	if (p2)
		p2->timer[1] = 0;
	//----------------------------

	// [1]: set portal creator race
	p1->value[1] = GET_RACE(ch);
	if (p2)
		p2->value[1] = p1->value[1];
	//----------------------------

	// [2]: set portal charge (how many can pass, if -1 then no limit)
	p1->value[2] = charge;
	if (p2)
		p2->value[2] = charge;
	//----------------------------

	// set caster level
	p1->value[3] = GET_LEVEL(ch);
	if (p2)
		p2->value[3] = p1->value[3];
	//----------------------------

	// [7]: set portal id, relates portal1 with his another side portal2
	p1->value[7] = portal_id;
	if (p2)
		p2->value[7] = portal_id;
	portal_id++;
	//----------------------------
}

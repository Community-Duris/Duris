#include "core/prototypes.h"
#include "cmd/interp.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "core/utils.h"
#include "core/defines.h"
#include "magic/spells.h"
#include "combat/ctf.h"
#include "combat/justice.h"
#include "world/graph.h"
#include "world/map.h"
#include "world/outposts.h"
#include "world/vnum.obj.h"
#include "guild/alliances.h"
#include "guild/assocs.h"
#include "guild/guildhall.h"
#include "economy/economic_gameplay_authority.h"
#include "item/item_command_policy.h"
#include "sql/sql.h"

extern P_room world;
extern struct zone_data *zone_table;
extern struct str_app_type str_app[];
extern const char *command[];
extern P_index obj_index;
extern P_index mob_index;
extern const int top_of_world;
extern int avail_hometowns[][LAST_RACE + 1];
extern int guild_locations[][CLASS_COUNT + 1];

// Immediate native payload. Item-owned windups call this only after their
// source, original target and room have passed the completion checks.
void spell_siren_song(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		      P_obj /*obj*/)
{
	if (!ch || !victim)
		return;

	if (ch->in_room == victim->in_room)
	{
		send_to_char("That person is already near you.\n", ch);
		return;
	}
	if (how_close(ch->in_room, victim->in_room,
		      (int)get_property("spell.siren.song.dist", 30)) <= 0)
	{
		send_to_char("Your voice strains from the distance to reach your target.\n", ch);
		return;
	}
	// CANT CAST FROM OR TO GUILDHALLROOMS
	if ((IS_ROOM(ch->in_room, ROOM_GUILD)) || (IS_ROOM(victim->in_room, ROOM_GUILD)))
	{
		send_to_char("Your siren song stops abruptly!\n", ch);
		return;
	}
	act("&+g$n&+g emits an &+Genchanting &+Csong &+gthat tugs at your &+Csoul.", FALSE, ch, 0,
	    0, TO_ROOM);
	act("&+gYou emit an &+Genchanting &+Csong &+gthat fills the area.", FALSE, ch, 0, 0,
	    TO_CHAR);

	act("&+gAn &+Genchanting &+Csong fills the area... &+git tugs on your &+Csoul.", FALSE,
	    victim, 0, 0, TO_CHAR);
	act("&+gAn &+Genchanting &+Csong fills the area... &+git tugs on your &+Csoul.", FALSE,
	    victim, 0, 0, TO_ROOM);

	if (!IS_FIGHTING(victim) && !IS_PATROL(victim) &&
	    !NewSaves(victim, SAVING_SPELL, GET_C_CHA(ch) - GET_C_POW(victim)))
	{
		send_to_char(
			"&+cYou can resist no longer! The &+Csong draws your nearer, nearer...\n",
			victim);
		act("$n wanders out of the room in a daze!", FALSE, victim, 0, 0, TO_ROOM);
		char_from_room(victim);
		char_to_room(victim, ch->in_room, -1);
		act("$n wanders into the room in a daze!", FALSE, victim, 0, 0, TO_ROOM);
	}
}

void spell_dark_compact(int /*level*/, P_char /*ch*/, char * /*arg*/, int /*type*/,
			P_char /*victim*/, P_obj /*obj*/)
{
	/*
	if(!IS_TRUSTED(ch) && !IS_MAP_ROOM(ch->in_room))
	{
	  send_to_char("You must be on the map to complete this incanation.\n", ch);
	  return;
	}

	if(1)
	{
	  send_to_char("this spell is currently disabled while working on new maps\n", ch);
	  return;
	}

	if(!IS_TRUSTED(ch) && IS_PC(ch))
	  CharWait(ch, 60);
	else
	  CharWait(ch, 5);

	if(IS_ROOM(ch->in_room, ROOM_NO_TELEPORT))
	{
	  send_to_char("The magic in this room prevents you from leaving.\n", ch);
	  return;
	}
	if(world[ch->in_room].sector_type == SECT_OCEAN)
	{
	  send_to_char("While swimming? I think not.\n", ch);
	  return;
	}

	if(world[ch->in_room].zone != world[MAX(0, real_room(210000))].zone)
	{
	  location = real_room0(number(210000, 214000));
	  while ((world[location].sector_type == SECT_OCEAN) ||
	         IS_ROOM(location, ROOM_NO_TELEPORT) ||
	         IS_ROOM(location, ROOM_NO_GATE))
	    location = real_room0(number(210000, 214000));
	}
	else
	{
	  location = real_room0(number(140000, 159999));
	  while ((world[location].sector_type == SECT_OCEAN) ||
	         IS_ROOM(location, ROOM_NO_TELEPORT) ||
	         IS_ROOM(location, ROOM_NO_GATE))
	    location = real_room0(number(140000, 159999));
	}

	act("$n begins chanting softly...", FALSE, ch, 0, 0, TO_ROOM);
	act("&+rA blood red globe appears and $n steps into it.", FALSE, ch, 0, 0,
	    TO_ROOM);
	send_to_char("&+rA blood red globe appears and you step into it.\n", ch);

	if(!number(0, 20) || (location == NOWHERE) ||
	    !can_enter_room(ch, location, FALSE))
	{
	  send_to_char("OH NO!!  Something has gone wrong!  You feel lost!\n", ch);
	  spell_teleport(level, ch, 0, 0, ch, 0);
	  return;
	}
	else
	{
	  char_from_room(ch);
	  char_to_room(ch, location, -1);
	  act("&+rA blood red globe appears and $n steps out of it.", FALSE, ch, 0,
	      0, TO_ROOM);
	}
	*/
}

void spell_dimension_door(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			  P_obj /*obj*/)
{
	int location;
	char buf[256] = { 0 };
	P_char tmp = NULL;
	int distance;

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
		return;

	if (GET_SPEC(ch, CLASS_SORCERER, SPEC_SHADOW))
		CharWait(ch, WAIT_SEC);
	else if (IS_PC(ch))
		CharWait(ch, WAIT_SEC * 9);
	else
		CharWait(ch, WAIT_SEC * 1 + 1);

	if (IS_AFFECTED3(victim, AFF3_NON_DETECTION) || IS_ROOM(ch->in_room, ROOM_NO_TELEPORT) ||
	    IS_HOMETOWN(ch->in_room) || world[ch->in_room].sector_type == SECT_OCEAN ||
	    world[victim->in_room].sector_type == SECT_CASTLE ||
	    world[victim->in_room].sector_type == SECT_CASTLE_WALL ||
	    world[victim->in_room].sector_type == SECT_CASTLE_GATE)
	{
		send_to_char("&+cYou failed.\n", ch);
		return;
	}
	if (IS_PC(victim) && IS_SET(victim->specials.act2, PLR2_NOLOCATE) && !is_introd(victim, ch))
	{
		send_to_char("&+cYou failed.\n", ch);
		return;
	}
	P_char rider = get_linking_char(victim, LNK_RIDING);
	if (IS_NPC(victim) && rider)
	{
		send_to_char("&+cYou failed.\n", ch);
		return;
	}

	if (!IS_TRUSTED(ch) && IS_TRUSTED(victim))
	{
		send_to_char("&+cYou failed.\n", ch);
		return;
	}

	location = victim->in_room;

	if (IS_ROOM(location, ROOM_NO_TELEPORT) || IS_HOMETOWN(location) || racewar(ch, victim) ||
	    world[location].sector_type == SECT_OCEAN || (IS_PC_PET(ch) && IS_PC(victim)))
	{
		send_to_char("&+cYou failed.\n", ch);
		return;
	}

	if (!is_Raidable(ch, 0, 0))
	{
		send_to_char("&+WYou are not raidable. The spell fails!\r\n", ch);
		return;
	}

	if (IS_PC(ch) && IS_PC(victim) && !is_Raidable(victim, 0, 0))
	{
		send_to_char("&+WYour target is not raidable. The spell fails!\r\n", ch);
		return;
	}

	distance = (int)(level * get_property("spell.dim.perlevel.modifier", 1.35));
	if (GET_SPEC(ch, CLASS_SORCERER, SPEC_SHADOW))
		distance += 15;

	if (!IS_TRUSTED(ch) && how_close(ch->in_room, victim->in_room, distance) < 0 &&
	    how_close(victim->in_room, ch->in_room, distance) < 0)
	{
		send_to_char("&+cYou failed.\n", ch);
		return;
	}

#if defined(CTF_MUD) && (CTF_MUD == 1)
	if (ctf_carrying_flag(ch) == CTF_PRIMARY)
	{
		send_to_char("You can't carry that with you.\r\n", ch);
		drop_ctf_flag(ch);
	}
#endif

	for (tmp = world[ch->in_room].people; tmp; tmp = tmp->next_in_room)
	{
		if (IS_AFFECTED(tmp, AFF_BLIND) || (tmp->specials.z_cord != ch->specials.z_cord) ||
		    (tmp == ch))
		{
			continue;
		}
		if (CAN_SEE(tmp, ch))
			act("&+LA black two-dimensional door appears next to $n, who steps into it and vanishes along with the door.",
			    FALSE, ch, 0, tmp, TO_VICT);
		else
			act("&+LA black two-dimensional door appears, then vanishes without a sound.",
			    FALSE, ch, 0, tmp, TO_VICT);
		send_to_char(buf, tmp);
	}

	char_from_room(ch);
	char_to_room(ch, location, -1);

	for (tmp = world[ch->in_room].people; tmp; tmp = tmp->next_in_room)
	{
		if (IS_AFFECTED(tmp, AFF_BLIND) || (tmp->specials.z_cord != ch->specials.z_cord) ||
		    (tmp == ch))
			continue;
		if (CAN_SEE(tmp, ch))
			act("&+LA black rift in space opens next to you, and&n $n &+Lsteps out of it grinning.&n",
			    FALSE, ch, 0, tmp, TO_VICT);
		else
			act("&+LA black two-dimensional door appears, then vanishes without a sound.",
			    FALSE, ch, 0, tmp, TO_VICT);
		send_to_char(buf, tmp);
	}
}

void spell_relocate(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		    P_obj /*obj*/)
{
	if (!IS_ALIVE(ch) || ch->in_room == NOWHERE)
	{
		return;
	}

	if (GET_SPEC(ch, CLASS_SORCERER, SPEC_SHADOW))
		CharWait(ch, 4);
	else if (IS_PC(ch))
		CharWait(ch, 80);
	else
		CharWait(ch, 5);

	if (!can_relocate_to(ch, victim))
	{
		return;
	}

	act("&+W$n starts to become less solid &+Luntil $e fades into nothing!", FALSE, ch, 0, 0,
	    TO_ROOM);
	act("&+WYou start to become less solid, then fade into nothing!", FALSE, ch, 0, 0, TO_CHAR);

#if defined(CTF_MUD) && (CTF_MUD == 1)
	if (ctf_carrying_flag(ch) == CTF_PRIMARY)
	{
		send_to_char("You can't carry that with you.\r\n", ch);
		drop_ctf_flag(ch);
	}
#endif

	char_from_room(ch);
	char_to_room(ch, victim->in_room, -1);

	act("&+WA coalescing of the ethereal substances causes your vision to blur...\n&+WWhen it at last clears&n $n &+Wstands before you!&n",
	    FALSE, ch, 0, 0, TO_ROOM);
	act("&+WYou materialize elsewhere!&n", FALSE, ch, 0, 0, TO_CHAR);
}

void spell_group_teleport(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			  P_obj /*obj*/)
{
	int from_room, to_room, tries; // , dir;
	struct group_list *gl = NULL;
	// int      range = get_property("spell.teleport.range", 30);
	int range = 800000;

	if ((ch && !is_Raidable(ch, 0, 0)) || (victim && !is_Raidable(victim, 0, 0)))
	{
		send_to_char("&+WYou or your target is not raidable. The spell fails!\r\n", ch);
		return;
	}

	// make sure the room allows teleportation
	if (IS_ROOM(ch->in_room, ROOM_NO_TELEPORT) || IS_HOMETOWN(ch->in_room) ||
	    (world[ch->in_room].sector_type == SECT_OCEAN))
	{
		send_to_char("The magic in this room prevents you from leaving.\n", ch);
		return;
	}

	// find a suitable room in the zone to teleport to
	if (IS_MAP_ROOM(ch->in_room))
	{
		to_room = ch->in_room;

		for (int i = 0; i < range; i++)
		{
			tries = 0;
			do
			{
				to_room = number(zone_table[world[ch->in_room].zone].real_bottom,
						 zone_table[world[ch->in_room].zone].real_top);
				tries++;
			} while ((IS_ROOM(to_room, ROOM_PRIVATE) ||
				  IS_ROOM(to_room, ROOM_NO_TELEPORT) || IS_HOMETOWN(to_room) ||
				  (world[to_room].sector_type == SECT_OCEAN)) &&
				 (tries < 1000));
		}
		/*
		do
		      {
		        dir = number(0, 3);
		      } while( tries++ < 20 && !VALID_TELEPORT_EDGE(to_room, dir, ch->in_room) );

		      if( tries < 20 )
		        to_room = TOROOM(to_room, dir);
		*/
	}
	else
	{
		tries = 0;
		do
		{
			to_room = number(zone_table[world[ch->in_room].zone].real_bottom,
					 zone_table[world[ch->in_room].zone].real_top);
			tries++;
		} while ((IS_ROOM(to_room, ROOM_PRIVATE) || IS_ROOM(to_room, ROOM_NO_TELEPORT) ||
			  IS_HOMETOWN(to_room) || (world[to_room].sector_type == SECT_OCEAN)) &&
			 (tries < 1000));
	}

	// if no suitable room was found, teleport back to the same room they're in
	if (tries == 1000)
		to_room = ch->in_room;

	/*
	  // if this zone limits teleports, check to see if the teleportation range is too great
	  if(LIMITED_TELEPORT_ZONE(ch->in_room))
	  {
	    if(how_close(ch->in_room, to_room, 5))
	      send_to_char
	        ("The magic gathers, but somehow fades away before taking effect.\n",
	         ch);
	    return;
	  }
	*/
	// get the room the teleport is taking place so we don't have to move the teleporter last
	from_room = ch->in_room;

	// if the teleporter is grouped
	if (ch->group)
	{
		// teleport the group members in the character's room
		for (gl = ch->group; gl; gl = gl->next)
		{
			if (gl->ch->in_room == from_room)
			{
				// show the room they're fading
				act("$n slowly fades out of existence.", FALSE, gl->ch, 0, 0,
				    TO_ROOM);

				// if they're fighting, break it up
				if (IS_FIGHTING(gl->ch))
					stop_fighting(gl->ch);
				REMOVE_BIT(ch->specials.affected_by, AFF_HIDE);

				// move the char
				char_from_room(gl->ch);
				char_to_room(gl->ch, to_room, -1);

				// show the new room they've arrived
				act("$n slowly fades into existence.", FALSE, gl->ch, 0, 0,
				    TO_ROOM);
			}
		}
	}
	else
	{
		// show the room they're fading
		act("$n slowly fades out of existence.", FALSE, ch, 0, 0, TO_ROOM);

		// if they're fighting, break it up
		if (IS_FIGHTING(ch))
			stop_fighting(ch);
		REMOVE_BIT(ch->specials.affected_by, AFF_HIDE);

		// move the char
		char_from_room(ch);
		char_to_room(ch, to_room, -1);

		// show the new room they've arrived
		act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
	}
}

void spell_teleport(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		    P_obj /*obj*/)
{
	int dir, to_room;
	P_char vict, t_ch;

	if ((IS_ROOM(ch->in_room, ROOM_NO_TELEPORT) || IS_HOMETOWN(ch->in_room) ||
	     world[ch->in_room].sector_type == SECT_OCEAN) &&
	    level < 60)
	{
		send_to_char("The magic in this room prevents you from leaving.\n", ch);
		return;
	}

	if (!victim)
	{
		vict = ch;
	}
	else
	{
		vict = victim;
	}
	if ((ch && !is_Raidable(ch, 0, 0)) || (victim && !is_Raidable(victim, 0, 0)))
	{
		send_to_char("&+WYou or your target is not raidable. The spell fails!\r\n", ch);
		return;
	}

	int range = get_property("spell.teleport.range", 30);
	to_room = vict->in_room;
	if (IS_MAP_ROOM(vict->in_room))
	{
		for (int i = 0; i < range; i++)
		{
			int tries = 0;
			dir = number(0, 3);
			do
			{
				// Give a 67% chance to continue on the direction chosen
				dir = number(0, 2) ? dir : number(0, 3);
			} while (tries++ < 15 && !VALID_TELEPORT_EDGE(to_room, dir, vict->in_room));

			if (tries < 10)
				to_room = TOROOM(to_room, dir);
		}
	}
	else
	{
		int tries = 0;
		do
		{
			to_room = number(zone_table[world[vict->in_room].zone].real_bottom,
					 zone_table[world[vict->in_room].zone].real_top);
			tries++;
		} while ((IS_ROOM(to_room, ROOM_PRIVATE) || IS_ROOM(to_room, ROOM_NO_MAGIC) ||
			  IS_ROOM(to_room, ROOM_NO_TELEPORT) || IS_HOMETOWN(to_room) ||
			  world[to_room].sector_type == SECT_OCEAN) &&
			 tries < 1000);
		if (tries >= 1000)
			to_room = vict->in_room;
	}

	if (LIMITED_TELEPORT_ZONE(vict->in_room))
	{
		if (how_close(vict->in_room, to_room, 5))
		{
			send_to_char(
				"The magic gathers, but somehow fades away before taking effect.\n",
				vict);
		}
		return;
	}
	act("$n slowly fades out of existence.", FALSE, vict, 0, 0, TO_ROOM);
	if (IS_FIGHTING(vict))
		stop_fighting(vict);
	if (vict->in_room != NOWHERE)
		for (t_ch = world[vict->in_room].people; t_ch; t_ch = t_ch->next)
			if (IS_FIGHTING(t_ch) && (GET_OPPONENT(t_ch) == vict))
				stop_fighting(t_ch);
	if (vict->in_room != to_room)
	{
#if defined(CTF_MUD) && (CTF_MUD == 1)
		if (ctf_carrying_flag(ch) == CTF_PRIMARY)
		{
			send_to_char("You can't carry that with you.\r\n", ch);
			drop_ctf_flag(ch);
		}
#endif
		char_from_room(vict);
		char_to_room(vict, to_room, -1);
	}
	act("$n slowly fades into existence.", FALSE, vict, 0, 0, TO_ROOM);
}

void spell_group_recall(int level, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
			P_obj /*obj*/)
{
	struct group_list *gl;

	if (!ch->group)
		return spell_word_of_recall(level, ch, 0, 0, ch, 0);

	if (IS_BACKRANKED(ch))
	{
		send_to_char("How can you do that from back here?!\n", ch);
		return;
	}
	if (IS_FIGHTING(ch))
	{
		send_to_char("You are fighting for your life!\n", ch);
		return;
	}

	if (IS_NPC(ch) && IS_PC_PET(ch))
		return;

	int room = ch->in_room;
	int z_cord = ch->specials.z_cord;
	for (gl = ch->group; gl; gl = gl->next)
	{
		if ((gl->ch->in_room == room) && (gl->ch->specials.z_cord == z_cord))
			spell_word_of_recall(level, ch, 0, 0, gl->ch, 0);
	}
}

void spell_nether_gate(int /*level*/, P_char /*ch*/, P_char /*victim*/, P_obj /*obj*/)
{
	/**
	 ** Main code for Nether gate spell is in spells.c
	 **/
}

void spell_gate(int /*level*/, P_char /*ch*/, P_char /*victim*/, P_obj /*obj*/)
{
	/**
	 ** Main code for Gate spell is in spells.c
	 **/
}

void spell_plane_shift(int /*level*/, P_char /*ch*/, P_char /*victim*/, P_obj /*obj*/)
{
	/**
	 ** Main code for Gate spell is in spells.c
	 **/
}

void spell_blink(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
		 P_obj /*obj*/)
{
	P_char tch;
	int fall_chance;

	if (ch)
	{
		if (!IS_ALIVE(ch))
		{
			return;
		}
		send_to_char("You scatter your atoms to the wind only to reassemble nearby.\n", ch);

		if (IS_FIGHTING(ch))
			stop_fighting(ch);
		for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
		{
			if (GET_OPPONENT(tch) == ch)
			{
				act("Without warning $n simply ceases to be. Doh! Where did he go?",
				    FALSE, ch, 0, tch, TO_ROOM);
				stop_fighting(tch);
				fall_chance = MAX(20, 50 - GET_C_AGI(tch) / 2);
				if (fall_chance > number(1, 100))
				{
					send_to_char(
						"Unable to reverse your swing you lose your balance and crash to your knee.\n",
						tch);
					SET_POS(tch, POS_SITTING + GET_STAT(tch));
					CharWait(tch, PULSE_VIOLENCE);
				}
			}
			else if (ch != tch)
			{
				act("$n suddenly ceases to be only to materialize nearby.", FALSE,
				    ch, 0, tch, TO_VICT);
			}
		}
	}
}

// Universal call to determine if victim can be relocated/warped/dreamed.
bool can_relocate_to(P_char ch, P_char victim)
{
	int location = victim->in_room;

	if (!(victim) || !(location) || !can_enter_room(ch, location, FALSE) ||
	    ((racewar(ch, victim) || IS_NPC(victim) || IS_ROOM(ch->in_room, ROOM_SINGLE_FILE) ||
	      IS_ROOM(victim->in_room, ROOM_SINGLE_FILE)) &&
	     !IS_TRUSTED(ch)))
	{
		send_to_char("&+CYou failed.\n", ch);
		return FALSE;
	}

	if (IS_NPC(ch) && IS_PC_PET(ch))
	{
		return FALSE;
	}

	if (!IS_TRUSTED(ch) && IS_TRUSTED(victim))
	{
		send_to_char("&+CYou failed.\n", ch);
		return FALSE;
	}

	if (IS_AFFECTED3(victim, AFF3_NON_DETECTION) ||
	    (IS_PC(victim) && IS_SET(victim->specials.act2, PLR2_NOLOCATE) &&
	     !is_linked_to(ch, victim, LNK_CONSENT) && !IS_TRUSTED(ch)))
	{
		send_to_char("&+CYou failed.\n", ch);
		return FALSE;
	}

	if (IS_PC(victim) && IS_SET(victim->specials.act2, PLR2_NOLOCATE) &&
	    !is_introd(victim, ch) && !IS_TRUSTED(ch))
	{
		send_to_char("&+CYou failed.\n", ch);
		return FALSE;
	}

	if (IS_ROOM(ch->in_room, ROOM_NO_TELEPORT) || IS_HOMETOWN(ch->in_room))
	{
		send_to_char("The magic in this room prevents you from leaving.\n", ch);
		return FALSE;
	}

	if (IS_PC_PET(ch) && IS_PC(victim))
	{
		send_to_char("&+CYou failed.\n", ch);
		return FALSE;
	}

	if (IS_ROOM(location, ROOM_NO_TELEPORT) || IS_HOMETOWN(location) ||
	    world[location].sector_type == SECT_OCEAN)
	{
		send_to_char("&+CYou failed.\n", ch);
		return FALSE;
	}

	if (!is_Raidable(ch, 0, 0))
	{
		send_to_char("&+WYou are not raidable. The spell fails!\r\n", ch);
		return FALSE;
	}

	if (IS_PC(ch) && IS_PC(victim) && !is_Raidable(victim, 0, 0))
	{
		send_to_char("&+WYour target is not raidable. The spell fails!\r\n", ch);
		return FALSE;
	}

	if (ch->specials.z_cord > 0 || ch->specials.z_cord < 0)
	{
		send_to_char("You must be firmly on the ground.\r\n", ch);
		return FALSE;
	}

	if (victim->specials.z_cord > 0 || victim->specials.z_cord < 0)
	{
		send_to_char("Your target is either swimming or flying too high.\r\n", ch);
		return FALSE;
	}

	return TRUE;
}

/* flag indicates if it's a guild door, 1 if outside door, 2 if
   inside door, other is normal teleporter */

void teleport_to(P_char ch, int to_room, int flag)
{
	if (to_room == NOWHERE)
	{
		send_to_char("teleport_to(): tried to dump you in NOWHERE.  tell a god.\n", ch);
		return;
	}
	if (flag == 1)
		flag = flag;
	/*    act("$n enters a nearby building through the doorway.", FALSE, ch, 0, 0, TO_ROOM); */
	/*  else if(flag == 2)
	    act("$n leaves the building through the doorway.", FALSE, ch, 0, 0, TO_ROOM); */
	else
		act("$n slowly fades out of existence.", FALSE, ch, 0, 0, TO_ROOM);

	char_from_room(ch);
	char_to_room(ch, to_room, 0);

	/* we'd hate to get stuck in an infinite loop, wouldn't we?  4 isn't currently
	   used as a flag value */

	if (flag == 1)
		act("$n enters the building through the doorway.", FALSE, ch, 0, 0, TO_ROOM);
	else if (flag == 2)
		act("$n exits from a nearby building through the doorway.", FALSE, ch, 0, 0,
		    TO_ROOM);
	else
		act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
}

/*
 * A routine for ITEM_TELEPORT objects
 */
int get_room_in_zone(int zone_room, P_char ch)
{
	int to_room, low, high, level, start_room;

	level = 50; /*
	             * give 50 chances
	             */
	start_room = real_room(zone_room);
	low = MAX(2, zone_table[world[start_room].zone].real_bottom);
	high = zone_table[world[start_room].zone].real_top;

	do
	{
		to_room = number(low, high);
	} while ((!can_enter_room(ch, to_room, FALSE) || IS_ROOM(to_room, ROOM_NO_TELEPORT) ||
		  IS_HOMETOWN(to_room)) &&
		 --level);

	if (level)
		start_room = to_room;

	return (start_room);
}

void spell_word_of_recall(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			  P_obj /*obj*/)
{
	int loc_nr, e_pos, heavy;
	int a, b = 0;

	// ch may be in a !recall room like a guildhall by now, thus most checks check victim

	if (!SanityCheck(ch, "spell_word_of_recall") ||
	    !SanityCheck(victim, "spell_word_of_recall"))
		return;

	if (IS_NPC(victim) && (!GET_BIRTHPLACE(victim)))
		return;

	if (IS_NPC(victim) && IS_PC_PET(victim))
		return;

	if (IS_ROOM(victim->in_room, ROOM_SILENT))
	{
		send_to_char("No sound can be heard.\n", ch);
		return;
	}

	if (affected_by_spell(victim, SKILL_BEARHUG))
	{
		send_to_char("You can't seem to get enough breath to speak!", ch);
		return;
	}

	if (IS_ROOM(victim->in_room, ROOM_NO_RECALL))
	//||(world[ch->in_room].sector_type == SECT_OCEAN))
	{
		if (ch == victim)
			act("$n utters a single word.", TRUE, ch, 0, 0, TO_ROOM);
		else
		{
			act("$n utters a single word.", TRUE, ch, 0, victim, TO_NOTVICT);
			act("You utter a single word.", TRUE, ch, 0, victim, TO_CHAR);
		}
		return;
	}
	if (IS_FIGHTING(victim))
	{
		if (IS_PC(victim) && IS_PC(GET_OPPONENT(victim)) && !number(0, 2))
		{
			if (ch == victim)
				act("$n utters a single word.", TRUE, ch, 0, 0, TO_ROOM);
			else
			{
				act("$n utters a single word.", TRUE, ch, 0, victim, TO_NOTVICT);
				act("You utter a single word.", TRUE, ch, 0, victim, TO_CHAR);
			}
			return;
		}
	}
	if (!GET_BIRTHPLACE(ch))
	{
		for (a = 12; (a > 0) && !b; a--)
			if (avail_hometowns[a][(int)GET_RACE(ch)])
				b = a;
		if (!b)
		{
			send_to_char("You don't know any sort of home anymore..snif\n", ch);
			return;
		}
		if (IS_PC(ch))
			a = guild_locations[b][flag2idx(ch->player.m_class)];
		if (a < 0)
		{
			send_to_char("You don't know any sort of home anymore..sniff!f\n", ch);
			return;
		}
	}
	else
		a = GET_BIRTHPLACE(ch);
	loc_nr = real_room(a);
	if ((loc_nr == NOWHERE) || (loc_nr > top_of_world))
	{
		send_to_char("You are completely lost.\n", victim);
		return;
	}
	if (economic_gameplay_authority::active() &&
	    total_carried_weight(victim) > ((CAN_CARRY_W(victim) / 100) * 70))
		for (int slot = 0; slot < MAX_WEAR; ++slot)
			if (victim->equipment[slot])
			{
				send_to_char("Your equipment cannot be left behind right now.\r\n",
					     ch);
				return;
			}
	if (ch == victim)
		act("&+W$n utters a single word and disappears.", TRUE, victim, 0, 0, TO_ROOM);
	else
	{
		act("&+W$n utters a single word and $N disappears.", TRUE, ch, 0, victim,
		    TO_NOTVICT);
		act("&+WYou utter a single word and $N disappears.", TRUE, ch, 0, victim, TO_CHAR);
	}
	if (IS_PC(victim))
		sql_log(victim, PLAYERLOG, "Word of recalled to %d", world[victim->in_room].number);

	/* Exceeding wieght limit? */
	e_pos = heavy = 0;
	do
	{
		if (total_carried_weight(victim) > ((CAN_CARRY_W(victim) / 100) * 70))
			if (victim->equipment[e_pos])
			{
				logit(LOG_RECALL, "WORD OF RECALL: (%s) drops (%s) in [%d].",
				      GET_NAME(victim), victim->equipment[e_pos]->short_description,
				      world[victim->in_room].number);

				logit(LOG_WIZ, "WORD OF RECALL: (%s) drops (%s) in [%d].",
				      GET_NAME(victim), victim->equipment[e_pos]->short_description,
				      world[victim->in_room].number);
				if (IS_PC(victim))
				{
					sql_log(victim, PLAYERLOG,
						"Dropped %s&n [%d] in [%d] while word of recalling.",
						victim->equipment[e_pos]->short_description,
						obj_index[victim->equipment[e_pos]->R_num]
							.virtual_number,
						world[victim->in_room].number);
				}

				obj_to_room(unequip_char(victim, e_pos), victim->in_room);
				heavy = TRUE;
			}
		e_pos++;
	} while (e_pos < MAX_WEAR);

	logit(LOG_RECALL, "WORD OF RECALL: (%s) recalled from [%d].", GET_NAME(victim),
	      world[victim->in_room].number);

#if defined(CTF_MUD) && (CTF_MUD == 1)
	if (ctf_carrying_flag(ch) == CTF_PRIMARY)
	{
		send_to_char("You can't carry that with you.\r\n", ch);
		drop_ctf_flag(ch);
	}
#endif

	char_from_room(victim);

	if (heavy)
	{
		send_to_char(
			"Oof, what an effort that was! Too bad you had to leave something behind.\n",
			ch);
		if (ch != victim)
			send_to_char(
				"&+WOops, seems you was too weak and left some of your stuff behind!&n\n",
				victim);
	}

	char_to_room(victim, loc_nr, -1);
	act("&+W$n suddenly fades into this reality, muttering a word of thanks.", TRUE, victim, 0,
	    0, TO_ROOM);
}

bool check_item_teleport(P_char ch, char *arg, int cmd)
{
	P_obj obj = NULL, obj_next;
	P_char dummy;
	int room, to_room;
	int pos;
	int virt;
	P_Guild guild;
	char Gbuf1[100];

	room = ch->in_room;
	/* Some types are keywordless */
	if ((cmd >= CMD_NORTH && cmd <= CMD_DOWN) || (cmd >= CMD_NORTHWEST && cmd <= CMD_SE))
	{
		for (obj = world[ch->in_room].contents; obj; obj = obj_next)
		{
			obj_next = obj->next_content;
			if (obj->type == ITEM_TELEPORT && obj->value[1] == cmd)
				break;
		}
	}
	else
	{
		generic_find(arg, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM, ch, &dummy, &obj);
	}
	if (!obj)
		return FALSE;

	if ((obj->type != ITEM_TELEPORT) || (obj->value[1] != cmd))
		return FALSE;

	// allow house/guild entrances to always work regardless of no-magic status
	/*
	 if(IS_ROOM(ch->in_room, ROOM_NO_MAGIC) && (vnum != 11001) &&
	     (vnum != 11007) && (vnum != 11008))
	 {
	   send_to_char("That doesn't seem to do anything!\n", ch);
	   return FALSE;
	 }
	 */
	if (400220 == obj_index[obj->R_num].virtual_number && !isname(GET_NAME(ch), obj->name))
	{
		send_to_char("You may not enter someone elses &+Gspirit&n portal!\r\n", ch);
		return TRUE;
	}

	/*
	 * Ignore argument for now...eventually we could "throw dust Dbra"..
	 */
	if (IS_NPC(ch))
	{
		virt = mob_index[GET_RNUM(ch)].virtual_number;
		if (virt == EVIL_AVATAR_MOB || virt == GOOD_AVATAR_MOB)
		{
			send_to_char("You are far too godly to use portals!\n", ch);
			return FALSE;
		}
	}

	to_room = obj->value[0];
	if (to_room == -1)
		to_room = get_room_in_zone(obj->value[3], ch);
	else
		to_room = real_room(to_room);

	// old guildhalls (deprecated)
	//  if(IS_ROOM(ch->in_room, ROOM_ROOM_ATRIUM))
	//  {
	//    if(!House_can_enter(ch, world[ch->in_room].number, -1))
	//    {
	//      send_to_char("You may not enter this private house!\n", ch);
	//      return TRUE;
	//    }
	//  }

	/* alternate (non-automated construction) guild-only teleporter checking */

	//  if(obj->value[7] && (obj->value[7] != GET_A_NUM(ch)) && !IS_TRUSTED(ch))
	//  {
	//    send_to_char("Nothing happens.\n", ch);
	//    return TRUE;
	//  }

	// New Guildhalls
	// If this is a portal to/from a guildhall/outpost.
	if (obj_index[obj->R_num].virtual_number == BUILDING_PORTAL)
	{
		// Get the proper guild id number.
		// If in a guildhall, call find_gh...
		if (IN_GH_ZONE(ch->in_room))
		{
			Guildhall *gh = find_gh_from_vnum(world[ch->in_room].number);
			if (gh)
				guild = gh->guild;
			else
			{
				send_to_char("Buggy guild portal.  Tell a God.\n", ch);
				return TRUE;
			}
		}
		else
		{
			Building *op = get_building_from_room(ch->in_room);
			if (op)
				guild = get_outpost_owner(op);
			else
			{
				send_to_char("Buggy outpost portal.  Tell a God.\n", ch);
				return TRUE;
			}
		}
		// Now find a group member or ch that is in the assoc. or fail.
		struct group_list *tgroup = ch->group;
		P_Alliance alliance;
		// If ch is not in the proper guild,
		if (GET_ASSOC(ch) != guild)
		{
			alliance = (GET_ASSOC(ch) == NULL) ? NULL : GET_ASSOC(ch)->get_alliance();
			if (!(alliance && (alliance->get_forgers() == guild ||
					   alliance->get_joiners() == guild)))
			{
				// Check all group members
				while (tgroup && (GET_ASSOC(tgroup->ch) != guild ||
						  IS_APPLICANT(GET_A_BITS(tgroup->ch))))
				{
					alliance = (GET_ASSOC(tgroup->ch) == NULL) ?
							   NULL :
							   GET_ASSOC(tgroup->ch)->get_alliance();
					if (alliance && (alliance->get_forgers() == guild ||
							 alliance->get_joiners() == guild))
					{
						break;
					}
					tgroup = tgroup->next;
				}
				// If no guildie-group member found..
				if (!tgroup && !IS_TRUSTED(ch))
				{
					send_to_char("The vortex repels your cheesy butt.\n", ch);
					return TRUE;
				}
			}
		}
	}

	// old guildhalls (deprecated)
	//  if(obj_index[obj->R_num].virtual_number == 11001)
	//  {
	//    /* guild teleporter */
	//    house = house_ch_is_in(ch);
	//    if(house)
	//    {
	//      struct group_list *tgroup;
	//
	//      for (tgroup = ch->group; tgroup; tgroup = tgroup->next)
	//        if(GET_A_NUM(tgroup->ch) == house->owner_guild &&
	//            !IS_APPLICANT(GET_A_BITS(tgroup->ch)))
	//          break;
	//      if((GET_A_NUM(ch) != house->owner_guild) && !tgroup &&
	//          !(IS_TRUSTED(ch)))
	//      {                         /* only guildies can use porters */
	//        send_to_char("Nothing happens.\n", ch);
	//        return TRUE;
	//      }
	//    }
	//  }

	if (!obj->value[2] || (IS_ROOM(ch->in_room, ROOM_ARENA) != IS_ROOM(to_room, ROOM_ARENA)))
	{
		send_to_char("Nothing happens.\n\n", ch);
		return TRUE;
	}
	if (obj->value[2] == 1 && economic_gameplay_authority::active() &&
	    item_command_uses_durable_ownership(obj))
	{
		send_to_char("That portal cannot use its final charge right now.\r\n", ch);
		return TRUE;
	}
	if (OBJ_CARRIED_BY(obj, ch))
	{
		act("&+W$p in $n's hands suddenly glows brightly!", FALSE, ch, obj, 0, TO_ROOM);
		act("&+W$p in your hands suddenly glows brightly!", FALSE, ch, obj, 0, TO_CHAR);
	}
	else if ((obj_index[obj->R_num].virtual_number == 11007) || /* GH/house entry/exit points */
		 (obj_index[obj->R_num].virtual_number == 11008))
	{
		if (IS_NPC(ch))
			return FALSE;
		if (obj_index[obj->R_num].virtual_number == 11007)
		{ /* outside door */
			act("$n enters $p via the doorway.", FALSE, ch, obj, 0, TO_ROOM);
			act("You enter $p through the doorway.", FALSE, ch, obj, 0, TO_CHAR);
		}
		else
		{
			act("$n leaves the building through the doorway.", FALSE, ch, obj, 0,
			    TO_ROOM);
			act("You leave the building through the doorway.", FALSE, ch, obj, 0,
			    TO_CHAR);
		}
	}
	else if (obj_index[obj->R_num].virtual_number != 48000)
		act("&+W$p suddenly glows brightly!", FALSE, ch, obj, 0, TO_ROOM);

	/* different messages for guild doors */

	if ((obj_index[obj->R_num].virtual_number == 48000) && IS_FIGHTING(ch))
	{
		act("&+WYou cannot enter a guildhall in combat!", FALSE, ch, obj, 0, TO_CHAR);
		return TRUE;
	}

	if (obj_index[obj->R_num].virtual_number == 11007)
		teleport_to(ch, to_room, 1);
	else if (obj_index[obj->R_num].virtual_number == 11008)
		teleport_to(ch, to_room, 2);
	else
	{
		teleport_to(ch, to_room, 0);
		if (obj_index[obj->R_num].virtual_number == 11001)
		{
			struct follow_type *tfol, *next;
			P_char tch;

			for (tfol = ch->followers; tfol; tfol = next)
			{
				next = tfol->next;
				tch = tfol->follower;

				if (room == tch->in_room)
				{
					act("You follow $N.", FALSE, tch, 0, ch, TO_CHAR);
					send_to_char("\n", tch);
					snprintf(Gbuf1, sizeof Gbuf1, "%s %s", command[cmd - 1],
						 arg);
					command_interpreter(tch, Gbuf1);
				}
			}
		}
	}

	if (obj->value[2] > 0)
	{
		if (!--obj->value[2])
		{
			if (OBJ_CARRIED_BY(obj, ch))
			{
				act("&+W$p in $n's hands shatters and the pieces disappear in smoke.",
				    TRUE, ch, obj, 0, TO_ROOM);
				act("&+W$p in your hands shatters and the pieces disappear in smoke.",
				    TRUE, ch, obj, 0, TO_CHAR);
			}
			else
				act("&+W$p shatters and the pieces disappear in smoke.", TRUE, ch,
				    obj, 0, TO_ROOM);

			if (OBJ_WORN(obj))
			{
				for (pos = 0; pos < MAX_WEAR; pos++)
				{
					if (obj->loc.wearing->equipment[pos] == obj)
					{
						unequip_char(obj->loc.wearing, pos);
						break;
					}
				}
			}
			extract_obj(obj, TRUE); // Could make Dragonnia stone arti.
			obj = NULL;
		}
	}
	return TRUE;
}

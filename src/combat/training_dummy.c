#include "combat/training_dummy.h"

#include "cmd/interp.h"
#include "combat/justice.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "magic/spells.h"
#include "world/db.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <limits>

extern P_room world;
extern int top_of_world;
extern int top_of_objt;
extern P_char character_list;
extern const int guild_locations[][CLASS_COUNT + 1];
extern const struct race_names race_names_table[];
extern const struct class_names class_names_table[];

namespace
{
P_char placement_override = nullptr;
P_char removal_override = nullptr;

enum TrainingDummyGear
{
	TRAINING_DUMMY_GEAR_MIN = 0,
	TRAINING_DUMMY_GEAR_MID = 1,
	TRAINING_DUMMY_GEAR_MAX = 2
};

const char *training_dummy_gear_name(int gear)
{
	switch (gear)
	{
	case TRAINING_DUMMY_GEAR_MIN:
		return "min";
	case TRAINING_DUMMY_GEAR_MAX:
		return "max";
	default:
		return "mid";
	}
}

int training_dummy_gear_ac(int gear)
{
	switch (gear)
	{
	case TRAINING_DUMMY_GEAR_MIN:
		return 100;
	case TRAINING_DUMMY_GEAR_MAX:
		return -200;
	default:
		return 0;
	}
}

int training_dummy_gear_save(int gear)
{
	switch (gear)
	{
	case TRAINING_DUMMY_GEAR_MIN:
		return 15;
	case TRAINING_DUMMY_GEAR_MAX:
		return 0;
	default:
		return 5;
	}
}

bool training_dummy_usable_room(int room)
{
	if (!world || room == NOWHERE || room < 0 || room > top_of_world)
		return false;

	return !IS_SET(world[room].room_flags, ROOM_SAFE | ROOM_NO_MOB | ROOM_NO_MAGIC);
}

P_char training_dummy_in_room(int room)
{
	if (!training_dummy_usable_room(room))
		return nullptr;

	for (P_char person = world[room].people; person; person = person->next_in_room)
	{
		if (training_dummy_is(person))
			return person;
	}
	return nullptr;
}

void training_dummy_replace_string(P_char dummy, char **field, const char *value,
					   unsigned int string_bit)
{
	if (IS_SET(dummy->only.npc->str_mask, string_bit) && *field)
		str_free(*field);
	*field = str_dup(value);
	SET_BIT(dummy->only.npc->str_mask, string_bit);
}

void training_dummy_apply_profile(P_char dummy, int room, int home, int level, int race,
					  unsigned int class_bit, int gear, bool fixed)
{
	/* A prototype mob may carry equipment.  A training dummy is never allowed
	 * to publish, retain, or expose any of that state. */
	for (int slot = 0; slot < MAX_WEAR; ++slot)
	{
		if (dummy->equipment[slot])
		{
			P_obj object = unequip_char(dummy, slot);
			if (object)
				extract_obj(object);
		}
	}
	for (P_obj object = dummy->carrying; object;)
	{
		P_obj next = object->next_content;
		obj_from_char(object);
		extract_obj(object);
		object = next;
	}
	GET_PLATINUM(dummy) = 0;
	GET_GOLD(dummy) = 0;
	GET_SILVER(dummy) = 0;
	GET_COPPER(dummy) = 0;

	/* A reused or malformed mob must not retain any social state from its
	 * prototype or an earlier lifecycle. */
	if (dummy->following)
		stop_follower(dummy);
	while (dummy->followers)
	{
		P_char follower = dummy->followers->follower;
		if (!follower)
			break;
		stop_follower(follower);
	}
	if (dummy->group)
		group_remove_member(dummy);
	clear_all_links(dummy);

	disarm_char_nevents(dummy, nullptr);
	clearMemory(dummy);

	dummy->specials.act = ACT_ISNPC | ACT_SENTINEL;
	dummy->specials.act2 = 0;
	dummy->specials.act3 = 0;
	dummy->specials.position = STAT_NORMAL + POS_STANDING;
	dummy->specials.fighting = nullptr;
	dummy->specials.was_fighting = nullptr;
	dummy->specials.next_fighting = nullptr;
	dummy->following = nullptr;
	dummy->specials.alignment = 0;
	dummy->specials.was_in_room = NOWHERE;
	for (int i = 0; i < 5; ++i)
		dummy->specials.apply_saving_throw[i] =
			static_cast<::byte>(training_dummy_gear_save(gear));

	dummy->player.sex = SEX_NEUTRAL;
	dummy->player.m_class = class_bit;
	dummy->player.secondary_class = 0;
	dummy->player.spec = 0;
	dummy->player.race = static_cast<ubyte>(race);
	dummy->player.racewar = RACEWAR_NEUTRAL;
	dummy->player.level = static_cast<ubyte>(level);
	dummy->player.hometown = home;
	dummy->player.birthplace = 0;
	dummy->player.orig_birthplace = 0;
	dummy->player.size = SIZE_MEDIUM;

	for (int i = 0; i < 10; ++i)
	{
		dummy->base_stats[i] = 50;
		dummy->curr_stats[i] = 50;
	}

	const int hit_points = 1000000;
	dummy->points.base_hit = hit_points;
	dummy->points.hit = hit_points;
	dummy->points.max_hit = hit_points;
	dummy->points.base_armor = static_cast<sh_int>(training_dummy_gear_ac(gear));
	dummy->points.curr_armor = dummy->points.base_armor;
	dummy->points.base_hitroll = level;
	dummy->points.hitroll = level;
	dummy->points.base_damroll = 0;
	dummy->points.damroll = 0;
	dummy->points.damnodice = 0;
	dummy->points.damsizedice = 0;

	dummy->only.npc->home = room;
	dummy->only.npc->default_pos = POS_STANDING;
	dummy->only.npc->aggro_flags = 0;
	dummy->only.npc->aggro2_flags = 0;
	dummy->only.npc->aggro3_flags = 0;
	dummy->only.npc->training_dummy = true;
	dummy->only.npc->training_dummy_fixed = fixed;
	dummy->only.npc->training_dummy_gear = gear;
	dummy->only.npc->training_dummy_damage = 0;
	dummy->only.npc->training_dummy_last_attacker_runtime_id = 0;
	dummy->only.npc->lowest_hit = hit_points;

	training_dummy_replace_string(dummy, &dummy->player.name, "training dummy dummy",
					      STRUNG_KEYS);
	training_dummy_replace_string(dummy, &dummy->player.short_descr, "a training dummy",
					      STRUNG_DESC2);
	training_dummy_replace_string(
		dummy, &dummy->player.long_descr,
		"A training dummy stands here, ready for PvP balance testing.\r\n", STRUNG_DESC1);
	training_dummy_replace_string(
		dummy, &dummy->player.description,
		"This training dummy absorbs incoming damage without fighting back or dying.\r\n",
		STRUNG_DESC3);
}

P_char training_dummy_create(int room, int home, int level, int race, unsigned int class_bit,
				      int gear, bool fixed)
{
	if (!training_dummy_usable_room(room) || training_dummy_in_room(room))
		return nullptr;

	P_char dummy = read_mobile(1255, VIRTUAL);
	if (!dummy)
		dummy = read_mobile(7, VIRTUAL);
	if (!dummy)
		return nullptr;
	if (!dummy->only.npc)
	{
		extract_char(dummy);
		return nullptr;
	}

	training_dummy_apply_profile(dummy, room, home, level, race, class_bit, gear, fixed);
	training_dummy_begin_placement(dummy);
	const bool placed = char_to_room(dummy, room, 0);
	training_dummy_end_placement(dummy);
	if (!placed)
	{
		extract_char(dummy);
		return nullptr;
	}
	return dummy;
}

int training_dummy_parse_level(const char *token)
{
	if (!token || !*token)
		return -1;

	errno = 0;
	char *end = nullptr;
	const long parsed = strtol(token, &end, 10);
	if (errno || end == token || *end || parsed < 1 || parsed > 61)
		return -1;
	return static_cast<int>(parsed);
}

int training_dummy_parse_race(const char *token)
{
	if (!token || !*token)
		return -1;
	if (!str_cmp(token, "grey") || !str_cmp(token, "gray") || !str_cmp(token, "greyelf") ||
	    !str_cmp(token, "grayelf"))
		return RACE_GREY;

	for (int race = 1; race <= LAST_RACE; ++race)
	{
		if (!race_names_table[race].normal)
			continue;
		if (!str_cmp(token, race_names_table[race].normal) ||
		    !str_cmp(token, race_names_table[race].no_spaces) ||
		    !str_cmp(token, race_names_table[race].code))
			return race;
	}
	return -1;
}

int training_dummy_parse_class(const char *token)
{
	if (!token || !*token)
		return 0;

	for (int class_index = 1; class_index <= CLASS_COUNT; ++class_index)
	{
		if (!class_names_table[class_index].normal)
			continue;
		if (!str_cmp(token, class_names_table[class_index].normal) ||
		    !str_cmp(token, class_names_table[class_index].code))
			return static_cast<int>(1u << (class_index - 1));
	}
	return 0;
}

int training_dummy_parse_gear(const char *token)
{
	if (!token || !*token || !str_cmp(token, "mid"))
		return TRAINING_DUMMY_GEAR_MID;
	if (!str_cmp(token, "min"))
		return TRAINING_DUMMY_GEAR_MIN;
	if (!str_cmp(token, "max"))
		return TRAINING_DUMMY_GEAR_MAX;
	return -1;
}

void training_dummy_send_help(P_char ch)
{
	send_to_char(
		"Training dummy commands:\r\n"
		"  dummy              report the dummy in this room\r\n"
		"  dummy report       show AC, saves, and accumulated damage\r\n"
		"  dummy reset        clear the accumulated damage\r\n"
		"  dummy help         show this help\r\n"
		"The dummy is anchored, accepts no items or coins, cannot be charmed,\r\n"
		"followed, or grouped, and never retaliates.\r\n"
		"Immortal-only:\r\n"
		"  dummy spawn [level] [race] [class] [min|mid|max]\r\n"
		"  dummy despawn\r\n",
		ch);
}

void training_dummy_send_report(P_char ch, P_char dummy)
{
	const int class_index = flag2idx(dummy->player.m_class);
	const int ac = calculate_ac(dummy);
	const int para_save = find_save(dummy, SAVING_PARA) +
			      dummy->specials.apply_saving_throw[SAVING_PARA] * 5;
	const int rod_save = find_save(dummy, SAVING_ROD) +
			     dummy->specials.apply_saving_throw[SAVING_ROD] * 5;
	const int fear_save = find_save(dummy, SAVING_FEAR) +
			      dummy->specials.apply_saving_throw[SAVING_FEAR] * 5;
	const int breath_save = find_save(dummy, SAVING_BREATH) +
				 dummy->specials.apply_saving_throw[SAVING_BREATH] * 5;
	const int spell_save = find_save(dummy, SAVING_SPELL) +
			       dummy->specials.apply_saving_throw[SAVING_SPELL] * 5;

	send_to_char_f(ch,
			       "Training dummy report\r\n"
			       "Profile: level %d %s %s\r\n"
			       "Gear: %s (AC %d; saves para %d, rod %d, fear %d, breath %d, spell %d)\r\n"
			       "Damage recorded: %llu\r\n"
		       "The dummy remains at full health, is anchored, accepts no items, and will not retaliate.\r\n",
			       GET_LEVEL(dummy), race_names_table[GET_RACE(dummy)].normal,
			       class_names_table[class_index].normal,
			       training_dummy_gear_name(dummy->only.npc->training_dummy_gear), ac, para_save,
			       rod_save, fear_save, breath_save, spell_save,
			       static_cast<unsigned long long>(dummy->only.npc->training_dummy_damage));
}

P_char training_dummy_from_command_room(P_char ch)
{
	return training_dummy_in_room(ch->in_room);
}

} // namespace

bool training_dummy_is(P_char ch)
{
	return ch && IS_NPC(ch) && ch->only.npc && ch->only.npc->training_dummy;
}

bool training_dummy_target_allowed(P_char attacker, P_char victim)
{
	if (!training_dummy_is(victim))
		return true;

	/* Players can attack the dummy for balance testing.  The explicit pet
	 * exception keeps player-controlled pet testing possible while ordinary
	 * area NPCs can never be used to tank for a player. */
	return attacker && (!IS_NPC(attacker) || IS_PC_PET(attacker));
}

bool training_dummy_spellup_target_allowed(P_char caster, P_char target)
{
	if (!training_dummy_is(target))
		return true;

	/*
	 * This is deliberately an AI target-selection guard, not a spell-effect
	 * guard.  An explicit/admin/system effect can still apply a chosen affect
	 * later; autonomous NPC maintenance simply does not select the dummy.
	 */
	return !caster || !IS_NPC(caster);
}

bool training_dummy_shape_target_allowed(P_char target)
{
	return !training_dummy_is(target);
}

bool training_dummy_clone_target_allowed(P_char target)
{
	return !training_dummy_is(target);
}

bool training_dummy_disguise_target_allowed(P_char target)
{
	return !training_dummy_is(target);
}

bool training_dummy_capture_target_allowed(P_char target)
{
	return !training_dummy_is(target);
}

P_char training_dummy_item_owner(P_obj object)
{
	int remaining = top_of_objt + 1;
	while (object && OBJ_INSIDE(object) && remaining-- > 0)
		object = object->loc.inside;

	if (!object)
		return nullptr;
	if (OBJ_CARRIED(object) && training_dummy_is(object->loc.carrying))
		return object->loc.carrying;
	if (OBJ_WORN(object) && training_dummy_is(object->loc.wearing))
		return object->loc.wearing;
	return nullptr;
}

void training_dummy_note_attacker(P_char dummy, P_char attacker)
{
	if (!training_dummy_is(dummy) || !attacker || !char_in_list(attacker))
		return;

	/* A normal NPC hitting the dummy is not a useful fallback target.  Keep
	 * the player/pet that caused the test interaction instead. */
	if (IS_NPC(attacker) && !IS_PC_PET(attacker))
		return;
	dummy->only.npc->training_dummy_last_attacker_runtime_id = attacker->runtime_id;
}

namespace
{
P_char training_dummy_find_runtime_character(uint64_t runtime_id)
{
	if (!runtime_id)
		return nullptr;
	for (P_char character = character_list; character; character = character->next)
		if (character->runtime_id == runtime_id && char_in_list(character))
			return character;
	return nullptr;
}

bool training_dummy_fallback_candidate(P_char npc, P_char candidate, P_char rejected)
{
	if (!npc || !candidate || candidate == npc || candidate == rejected ||
		training_dummy_is(candidate) || !char_in_list(candidate) || !IS_ALIVE(candidate) ||
		candidate->in_room != npc->in_room ||
		candidate->specials.z_cord != npc->specials.z_cord || !CAN_SEE(npc, candidate))
		return false;
	return training_dummy_target_allowed(npc, candidate);
}

int training_dummy_fallback_score(P_char npc, P_char candidate, P_char previous,
					 P_char recent, P_char rejected)
{
	if (candidate == recent)
		return 1000;
	if (GET_OPPONENT(candidate) == npc)
		return 900;
	if (candidate->specials.was_fighting == npc)
		return 800;
	if (candidate == previous)
		return 700;
	if (aggressive_to(npc, candidate))
		return 500;
	if (IS_FIGHTING(candidate) && GET_OPPONENT(candidate) != rejected)
		return 100;
	return 0;
}
} // namespace

void training_dummy_retarget_nonpet(P_char npc, P_char rejected)
{
	if (!npc || !IS_NPC(npc) || IS_PC_PET(npc) || training_dummy_is(npc) ||
		(rejected && !training_dummy_is(rejected)))
		return;

	P_char previous = GET_OPPONENT(npc);
	/* A player/pet that just damaged the rejected dummy is the strongest
	 * signal that this NPC should be fighting someone else in the room. */
	P_char recent = training_dummy_is(rejected) ?
		training_dummy_find_runtime_character(
			rejected->only.npc->training_dummy_last_attacker_runtime_id) : nullptr;

	if (IS_FIGHTING(npc))
		stop_fighting(npc);

	P_char best = nullptr;
	int best_score = 0;
	if (training_dummy_fallback_candidate(npc, recent, rejected))
		best = recent;

	if (!best && npc->in_room != NOWHERE && npc->in_room >= 0 && npc->in_room <= top_of_world)
	{
		for (P_char candidate = world[npc->in_room].people; candidate;
		     candidate = candidate->next_in_room)
		{
			if (!training_dummy_fallback_candidate(npc, candidate, rejected))
				continue;
			const int score = training_dummy_fallback_score(npc, candidate, previous, recent,
									 rejected);
			if (score > best_score)
			{
				best = candidate;
				best_score = score;
			}
		}
	}

	if (best)
		MobStartFight(npc, best);
}

bool training_dummy_can_enter_room(P_char ch)
{
	return !training_dummy_is(ch) || placement_override == ch;
}

bool training_dummy_can_leave_room(P_char ch)
{
	return !training_dummy_is(ch) || removal_override == ch;
}

void training_dummy_begin_placement(P_char ch)
{
	if (training_dummy_is(ch))
		placement_override = ch;
}

void training_dummy_end_placement(P_char ch)
{
	if (placement_override == ch)
		placement_override = nullptr;
}

void training_dummy_begin_removal(P_char ch)
{
	if (training_dummy_is(ch))
		removal_override = ch;
}

void training_dummy_end_removal(P_char ch)
{
	if (removal_override == ch)
		removal_override = nullptr;
}

void training_dummy_record_damage(P_char ch, int damage)
{
	if (!training_dummy_is(ch) || damage <= 0)
		return;

	const uint64_t amount = static_cast<uint64_t>(damage);
	const uint64_t maximum = std::numeric_limits<uint64_t>::max();
	if (amount > maximum - ch->only.npc->training_dummy_damage)
		ch->only.npc->training_dummy_damage = maximum;
	else
		ch->only.npc->training_dummy_damage += amount;
}

void training_dummy_bootstrap()
{
	int created = 0;
	for (int home = 1; home <= LAST_HOME; ++home)
	{
		const int candidates[] = { guild_locations[home][0], hometowns[home - 1].guard_room[0],
					   hometowns[home - 1].guard_room[1],
					   hometowns[home - 1].guard_room[2],
					   hometowns[home - 1].guard_room[3],
					   hometowns[home - 1].guard_room[4] };
		for (const int candidate : candidates)
		{
			if (candidate <= 0)
				continue;
			const int room = real_room(candidate);
			if (room == NOWHERE || training_dummy_in_room(room))
				continue;
			if (training_dummy_create(room, home, 56, RACE_GREY, CLASS_CLERIC,
						  TRAINING_DUMMY_GEAR_MID, true))
			{
				++created;
				break;
			}
		}
	}

	logit(LOG_STATUS, "Training dummy bootstrap created %d fixed PvP target(s).", created);
}

ACMD(do_training_dummy)
{
	if (IS_NPC(ch))
		return;

	char action[MAX_INPUT_LENGTH];
	argument = one_argument(argument, action);
	if (!*action || !str_cmp(action, "report"))
	{
		P_char dummy = training_dummy_from_command_room(ch);
		if (dummy)
			training_dummy_send_report(ch, dummy);
		else
			training_dummy_send_help(ch);
		return;
	}

	if (!str_cmp(action, "help"))
	{
		training_dummy_send_help(ch);
		return;
	}

	P_char dummy = training_dummy_from_command_room(ch);
	if (!str_cmp(action, "reset"))
	{
		if (!dummy)
		{
			send_to_char("There is no training dummy here.\r\n", ch);
			return;
		}
		dummy->only.npc->training_dummy_damage = 0;
		dummy->only.npc->training_dummy_last_attacker_runtime_id = 0;
		send_to_char("The training dummy's damage meter is reset.\r\n", ch);
		return;
	}

	if (!str_cmp(action, "despawn"))
	{
		if (!IS_TRUSTED(ch))
		{
			send_to_char("Only immortals can despawn training dummies.\r\n", ch);
			return;
		}
		if (!dummy)
		{
			send_to_char("There is no training dummy here.\r\n", ch);
			return;
		}
		extract_char(dummy);
		send_to_char("The training dummy fades away.\r\n", ch);
		return;
	}

	if (str_cmp(action, "spawn"))
	{
		training_dummy_send_help(ch);
		return;
	}

	if (!IS_TRUSTED(ch))
	{
		send_to_char("Only immortals can spawn custom training dummies.\r\n", ch);
		return;
	}
	if (!training_dummy_usable_room(ch->in_room))
	{
		send_to_char("Training dummies cannot be placed in safe, no-mob, or no-magic rooms.\r\n",
			     ch);
		return;
	}
	if (dummy)
	{
		send_to_char("There is already a training dummy here.\r\n", ch);
		return;
	}

	char level_token[MAX_INPUT_LENGTH];
	char race_token[MAX_INPUT_LENGTH];
	char class_token[MAX_INPUT_LENGTH];
	char gear_token[MAX_INPUT_LENGTH];
	argument = one_argument(argument, level_token);
	argument = one_argument(argument, race_token);
	argument = one_argument(argument, class_token);
	argument = one_argument(argument, gear_token);
	if (*argument)
	{
		send_to_char("Usage: dummy spawn [level] [race] [class] [min|mid|max]\r\n", ch);
		return;
	}

	const int level = *level_token ? training_dummy_parse_level(level_token) : 56;
	const int race = *race_token ? training_dummy_parse_race(race_token) : RACE_GREY;
	const int class_bit = *class_token ? training_dummy_parse_class(class_token) : CLASS_CLERIC;
	const int gear = training_dummy_parse_gear(gear_token);
	if (level < 1 || race < 1 || class_bit == 0 || gear < 0)
	{
		send_to_char("Invalid dummy profile. Use level 1-61, a race, a class, and min/mid/max gear.\r\n",
			     ch);
		return;
	}

	dummy = training_dummy_create(ch->in_room, 0, level, race,
				      static_cast<unsigned int>(class_bit), gear, false);
	if (!dummy)
	{
		send_to_char("The training dummy could not be created here.\r\n", ch);
		return;
	}
	send_to_char("A configurable training dummy appears, ready for testing.\r\n", ch);
	training_dummy_send_report(ch, dummy);
}

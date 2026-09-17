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
extern const int guild_locations[][CLASS_COUNT + 1];
extern const struct race_names race_names_table[];
extern const struct class_names class_names_table[];

namespace
{
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
	if (!char_to_room(dummy, room, 0))
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
			       "The dummy remains at full health and will not retaliate.\r\n",
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

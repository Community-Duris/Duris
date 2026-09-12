/*
 * Server-wide difficulty dials.
 *
 * Every dial is a 1..10 setting read from lib/duris.properties by apply_properties(), so
 * 'properties set' and 'properties reload' take effect at once. The curve turns a setting
 * into a multiplier; 5 is always exactly 1.0, which makes a fresh install, or any dial
 * left alone, the game as it was. Dials on something players want use the reciprocal of
 * the curve, so a higher setting is always harder for players.
 */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "world/difficulty.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace
{
struct difficulty_dial_info
{
	const char *key;
	const char *label;
	bool favours_players;
	const char *takes_effect;
};

// One row per enum difficulty_dial, in the same order.
constexpr difficulty_dial_info DIFFICULTY_DIALS[DIFFICULTY_DIAL_COUNT] = {
	{ "mob.hitpoints", "Mob hitpoints", false, "mobs loaded afterwards" },
	{ "mob.melee", "Mob melee damage", false, "immediately" },
	{ "mob.spell", "Mob spell damage", false, "immediately" },
	{ "mob.breath", "Mob breath damage", false, "immediately" },
	{ "mob.accuracy", "Mob chance to hit", false, "immediately" },
	{ "mob.resistance", "Mob saving throws", false, "immediately" },
	{ "mob.recovery", "Mob spell memorisation", false, "immediately" },
	{ "mob.gold", "Coins mobs carry", true, "mobs loaded afterwards" },
	{ "exp.required", "Experience to level", false, "immediately" },
	{ "exp.earned", "Experience earned", true, "immediately" },
	{ "death.penalty", "Death penalty", false, "the next death" },
	{ "player.recovery", "Player regeneration", true, "immediately" },
	{ "loot.drops", "Random loot drop chance", true, "immediately" },
	{ "loot.quality", "Random loot quality", true, "immediately" },
	{ "zone.repop", "Zone repop speed", false, "each zone's next reset" },
	{ "epic.gain", "Epic points earned", true, "immediately" },
	{ "artifact.feeding", "Artefact feeding", true, "immediately" },
};

std::array<double, DIFFICULTY_MAX> difficulty_curve = []
{
	std::array<double, DIFFICULTY_MAX> curve{};
	std::copy(std::begin(DIFFICULTY_DEFAULT_CURVE), std::end(DIFFICULTY_DEFAULT_CURVE),
		  curve.begin());
	return curve;
}();

std::array<int, DIFFICULTY_DIAL_COUNT> difficulty_settings = []
{
	std::array<int, DIFFICULTY_DIAL_COUNT> settings{};
	settings.fill(DIFFICULTY_NEUTRAL);
	return settings;
}();

std::array<double, DIFFICULTY_DIAL_COUNT> difficulty_effective = []
{
	std::array<double, DIFFICULTY_DIAL_COUNT> effective{};
	effective.fill(1.0);
	return effective;
}();

std::string dial_property(int dial)
{
	return std::string("difficulty.dial.") + DIFFICULTY_DIALS[dial].key;
}

int find_dial(const char *key)
{
	for (int dial = 0; dial < DIFFICULTY_DIAL_COUNT; ++dial)
	{
		if (!strcmp(DIFFICULTY_DIALS[dial].key, key))
			return dial;
	}
	return -1;
}

// Accept only a whole number from 1 to 10.
int parse_setting(const char *text)
{
	char *end = nullptr;
	const long value = strtol(text, &end, 10);
	if (!*text || !end || *end || value < DIFFICULTY_MIN || value > DIFFICULTY_MAX)
		return -1;
	return static_cast<int>(value);
}

void show_difficulty(P_char ch)
{
	char line[MAX_STRING_LENGTH];
	send_to_char("&+WServer difficulty dials&n (5 is the game as it was; 10 is hardest for "
		     "players)\r\n\r\n",
		     ch);
	send_to_char("  Dial                 Setting  Multiplier  Takes effect\r\n", ch);
	for (int dial = 0; dial < DIFFICULTY_DIAL_COUNT; ++dial)
	{
		snprintf(line, sizeof line, "  %-20s %7d  %9.2fx  %s\r\n",
			 DIFFICULTY_DIALS[dial].key, difficulty_settings[dial],
			 difficulty_effective[dial], DIFFICULTY_DIALS[dial].takes_effect);
		send_to_char(line, ch);
	}
	std::string curve = "\r\n  Curve:";
	for (int setting = DIFFICULTY_MIN; setting <= DIFFICULTY_MAX; ++setting)
	{
		snprintf(line, sizeof line, " %d=%.2fx", setting,
			 setting == DIFFICULTY_NEUTRAL ? 1.0 : difficulty_curve[setting - 1]);
		curve += line;
	}
	curve += "\r\n  Experience, loot, gold, regeneration, epic points and artefact feeding "
		 "use the inverse,\r\n  so a higher setting always gives players less.\r\n";
	send_to_char(curve.c_str(), ch);
	send_to_char("\r\n  difficulty set <dial> <1-10>   difficulty preset <1-10>   difficulty "
		     "save\r\n",
		     ch);
}
} // namespace

void update_difficulty_dials()
{
	char key[48];
	for (int setting = DIFFICULTY_MIN; setting <= DIFFICULTY_MAX; ++setting)
	{
		snprintf(key, sizeof key, "difficulty.curve.%02d", setting);
		difficulty_curve[setting - 1] =
			get_property(key, DIFFICULTY_DEFAULT_CURVE[setting - 1]);
	}
	for (int dial = 0; dial < DIFFICULTY_DIAL_COUNT; ++dial)
	{
		difficulty_settings[dial] = difficulty_clamp_setting(get_property(
			dial_property(dial).c_str(), static_cast<double>(DIFFICULTY_NEUTRAL)));
		difficulty_effective[dial] = difficulty_effective_multiplier(
			difficulty_curve_multiplier(difficulty_curve.data(),
						    difficulty_settings[dial]),
			DIFFICULTY_DIALS[dial].favours_players);
	}
}

double difficulty_multiplier(difficulty_dial dial)
{
	if (dial < 0 || dial >= DIFFICULTY_DIAL_COUNT)
		return 1.0;
	return difficulty_effective[dial];
}

bool difficulty_world_npc(P_char ch)
{
	return ch && IS_NPC(ch) && !IS_PC_PET(ch) && !IS_MORPH(ch);
}

void difficulty_scale_coins(long *copper, long *silver, long *gold, long *platinum)
{
	const double dial = difficulty_multiplier(DIFFICULTY_MOB_GOLD);
	if (dial == 1.0)
		return;
	for (long *coins : { copper, silver, gold, platinum })
	{
		if (coins)
			*coins = std::max(0L, difficulty_scale_long(*coins, dial));
	}
}

long difficulty_scale_money(long copper)
{
	const double dial = difficulty_multiplier(DIFFICULTY_MOB_GOLD);
	if (dial == 1.0)
		return copper;
	return std::max(0L, difficulty_scale_long(copper, dial));
}

int difficulty_scale_player_regen(P_char ch, int gain)
{
	const double dial = difficulty_multiplier(DIFFICULTY_PLAYER_RECOVERY);
	if (dial == 1.0 || gain <= 0 || !ch || !IS_PC(ch))
		return gain;
	return std::max(1, difficulty_scale_int(gain, dial));
}

int difficulty_pc_corpse_decay_minutes()
{
	const int minutes = get_property("timer.decay.corpse.pc", 120);
	const double dial = difficulty_multiplier(DIFFICULTY_DEATH_PENALTY);
	if (dial == 1.0)
		return minutes;
	return std::max(1, difficulty_scale_int(minutes, 1.0 / dial));
}

// difficulty                      show every dial, its multiplier and when it applies
// difficulty set <dial> <1-10>    change one dial (Forger and up)
// difficulty preset <1-10>        set every dial at once (Forger and up)
// difficulty save                 write the current properties to disk (Forger and up)
void do_difficulty(P_char ch, char *argument, int /*cmd*/)
{
	char command[32] = "", first[64] = "", second[32] = "";
	if (argument)
		sscanf(argument, " %31s %63s %31s", command, first, second);

	if (!*command || !strcmp(command, "show"))
	{
		show_difficulty(ch);
		return;
	}

	if (strcmp(command, "set") && strcmp(command, "preset") && strcmp(command, "save"))
	{
		send_to_char(
			"Usage: difficulty [show | set <dial> <1-10> | preset <1-10> | save]\r\n",
			ch);
		return;
	}
	if (GET_LEVEL(ch) < FORGER)
	{
		send_to_char("Only a Forger or higher can change the difficulty dials.\r\n", ch);
		return;
	}

	char request[128];
	if (!strcmp(command, "save"))
	{
		snprintf(request, sizeof request, "save");
		do_properties(ch, request, 0);
		return;
	}

	if (!strcmp(command, "preset"))
	{
		const int setting = parse_setting(first);
		if (setting < 0)
		{
			send_to_char("Usage: difficulty preset <1-10>\r\n", ch);
			return;
		}
		snprintf(request, sizeof request, "set difficulty.dial.* %d", setting);
	}
	else
	{
		const int dial = find_dial(first);
		const int setting = parse_setting(second);
		if (dial < 0 || setting < 0)
		{
			send_to_char("Usage: difficulty set <dial> <1-10>; 'difficulty' lists the "
				     "dials.\r\n",
				     ch);
			return;
		}
		snprintf(request, sizeof request, "set %s %d", dial_property(dial).c_str(),
			 setting);
	}
	// 'properties set' logs the change, re-applies every property and so re-reads the
	// dials. It changes memory only; 'difficulty save' writes the file.
	do_properties(ch, request, 0);
	show_difficulty(ch);
}

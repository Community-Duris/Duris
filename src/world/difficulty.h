#ifndef DURIS_DIFFICULTY_H
#define DURIS_DIFFICULTY_H

#include "core/structs.h"
#include "world/difficulty_math.h"

// Server-wide difficulty dials. Each is a 1..10 setting in lib/duris.properties
// (difficulty.dial.<name>) and is changed in game with the 'difficulty' command. 5 is the
// game as it was before the dials; 10 is the hardest setting for players. They stack on
// top of each zone's own difficulty. The order here matches the table in difficulty.c.
enum difficulty_dial
{
	DIFFICULTY_MOB_HITPOINTS,
	DIFFICULTY_MOB_MELEE,
	DIFFICULTY_MOB_SPELL,
	DIFFICULTY_MOB_BREATH,
	DIFFICULTY_MOB_ACCURACY,
	DIFFICULTY_MOB_RESISTANCE,
	DIFFICULTY_MOB_RECOVERY,
	DIFFICULTY_MOB_GOLD,
	DIFFICULTY_EXP_REQUIRED,
	DIFFICULTY_EXP_EARNED,
	DIFFICULTY_DEATH_PENALTY,
	DIFFICULTY_PLAYER_RECOVERY,
	DIFFICULTY_LOOT_DROPS,
	DIFFICULTY_LOOT_QUALITY,
	DIFFICULTY_ZONE_REPOP,
	DIFFICULTY_EPIC_GAIN,
	DIFFICULTY_ARTIFACT_FEEDING,
	DIFFICULTY_WORLD_QUEST,
	DIFFICULTY_DIAL_COUNT
};

// Re-reads every dial and the curve; called from apply_properties().
void update_difficulty_dials();

// The multiplier a dial currently applies, direction included. Exactly 1.0 at 5.
double difficulty_multiplier(difficulty_dial dial);

// The creatures the mob dials apply to: NPCs that are not a player's pet or morph.
bool difficulty_world_npc(P_char ch);

// Coins a mob is loaded with, scaled by the mob gold dial.
void difficulty_scale_coins(long *copper, long *silver, long *gold, long *platinum);
long difficulty_scale_money(long copper);

// A player's positive regeneration, scaled by the player recovery dial.
int difficulty_scale_player_regen(P_char ch, int gain);

// timer.decay.corpse.pc, shortened by the death penalty dial.
int difficulty_pc_corpse_decay_minutes();

// Bartender quests under the world quest dial: a dearer fee, fewer quests a day (never
// fewer than one) and more kills per kill quest.
int difficulty_scale_world_quest_fee(int fee);
int difficulty_scale_world_quest_allowance(int allowance);
int difficulty_scale_world_quest_kills(int kills);

#endif // DURIS_DIFFICULTY_H

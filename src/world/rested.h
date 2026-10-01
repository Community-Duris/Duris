#ifndef DURIS_WORLD_RESTED_H
#define DURIS_WORLD_RESTED_H

struct char_data;
struct affected_type;
int get_property(const char *key, int default_value);

// Automatic/purchased bonuses default on. Read live so existing ordinary
// effects become inert immediately when an operator disables the feature.
inline bool rested_bonus_enabled()
{
	return get_property("exp.rested.enabled", 1) != 0;
}

// For rested tags only: AFFTYPE_CUSTOM1 records an explicit staff grant and
// survives normal affect persistence. Staff grants override the automatic gate.
bool rested_bonus_effect_active(const affected_type *affect);
bool has_active_rested_bonus(char_data *ch, int tag);

// Trusted staff spell-up entry point (newbsa/newbsu), not a player-facing spell.
void grant_staff_rested_bonus(char_data *ch, char_data *victim);

#endif

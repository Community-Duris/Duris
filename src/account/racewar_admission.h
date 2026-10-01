#ifndef DURIS_RACEWAR_ADMISSION_H
#define DURIS_RACEWAR_ADMISSION_H

enum account_racewar_side
{
	ACCOUNT_RACEWAR_UNRESTRICTED = 0,
	ACCOUNT_RACEWAR_GOOD = 1,
	ACCOUNT_RACEWAR_EVIL = 2,
	ACCOUNT_RACEWAR_EXEMPT = 3,
};

enum account_racewar_denial
{
	ACCOUNT_RACEWAR_DENIAL_NONE = 0,
	ACCOUNT_RACEWAR_DENIAL_BLOCKED,
	ACCOUNT_RACEWAR_DENIAL_COOLDOWN,
	ACCOUNT_RACEWAR_DENIAL_UNAVAILABLE,
	ACCOUNT_RACEWAR_DENIAL_PERSISTENCE,
};

struct account_racewar_admission
{
	bool allowed;
	enum account_racewar_denial denial;
	enum account_racewar_side side;
	long remaining_seconds;
};

account_racewar_admission account_racewar_evaluate(enum account_racewar_side side, bool blocked,
						   long last_good, long last_evil, long now,
						   long cooldown_seconds);

#endif // DURIS_RACEWAR_ADMISSION_H

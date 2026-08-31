/****************************************************************************
 *  Arena.h
 *  Arena Subsystem for Duris
 ****************************************************************************/

#include "structs.h"

#define DEFAULT_ENABLED 0
#define DEFAULT_MAP 0
#define DEFAULT_TYPE 5
#define DEFAULT_DEATH 3
#define DEFAULT_LIVES 4
#define DEFAULT_SEENAME 0
#define DEFAULT_TIMER_OPEN 300
#define DEFAULT_TIMER_ACCEPT 30
#define DEFAULT_TIMER_MATCH 900
#define DEFAULT_TIMER_AFTERMATH 60

#define MAX_RACES 3
#define MAX_MAP 1
#define MAX_TEAM 20
#define MAX_GOODIE_TEAM 14
#define MAX_EVIL_TEAM 12
#define MAX_UNDEAD_TEAM 15
#define MAX_ARENA_TIMER 2
#define MAX_ARENA_TYPE 6

#define GOODIE 0
#define EVIL 1
#define UNDEAD 2

#define TYPE_CTF 0
#define TYPE_JAILBREAK 1
#define TYPE_KING_OF_THE_HILL 2
#define TYPE_TEAM_DM 3
#define TYPE_IT 4
#define TYPE_DEATHMATCH 5

#define DEATH_WINNER_TAKES_ALL 0
#define DEATH_EVEN_TRADE 1
#define DEATH_TOLL 2
#define DEATH_FREE 3

#define STAGE_OPEN 0
#define STAGE_ACCEPT 1
#define STAGE_BEGIN 2
#define STAGE_MATCH 3
#define STAGE_END 4
#define STAGE_AFTERMATH 5

#define FLAG_ENABLED BIT_1
#define FLAG_BETTING BIT_2
#define FLAG_TOURNAMENT BIT_3
#define FLAG_GOD BIT_4
#define FLAG_SEENAME BIT_5
#define FLAG_SHUTTING_DOWN BIT_6

#define PLAYER_IT BIT_1
#define PLAYER_DEAD BIT_2

struct arena_map
{
	int num;
	int spawn[MAX_RACES];
	int startroom, endroom;
};

struct arena_player
{
	P_char ch;
	int flags;
	int frags;
	int lives;
	char name[256];
};

struct arena_team
{
	struct arena_player player[MAX_TEAM];
	int flags;
	int score;
};

struct arena_data
{
	int flags;
	int type;
	int stage;
	int deathmode;
	int timer[MAX_ARENA_TIMER];
	struct arena_team team[MAX_RACES];
	struct arena_map map;
};

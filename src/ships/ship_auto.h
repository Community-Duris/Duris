#ifndef _SHIP_AUTO_
#define _SHIP_AUTO_

/*
 * Legacy autopilot state shared by ship_auto.c and ShipData.
 *
 * Player autopilot currently uses shipai_data as a short-lived route/order:
 * engage_autopilot() allocates it, autopilot_activity() advances it from the
 * ship heartbeat, and stop_autopilot()/clear_autopilot() release it.  The
 * group links and most AI type/mode constants belong to the older dormant
 * group implementation documented in ship_auto.c; NPC combat uses the
 * separate NPCShipAI type in ship_npc_ai.h.
 */

#include "ships/ships.h"

// Timers
#define AIT_WAIT 0
#define MAXAITIMER 1

// AI Types
#define AI_LINE 0
#define AI_STOP 1
#define AI_PATH 2

// AI Modes
#define AIM_COMBAT 0
#define AIM_SEEK 1
#define AIM_FOLLOW 2
#define AIM_WAIT 3
#define AIM_AUTOPILOT 4
#define AIM_RAMMING 5

// AI Bits
#define AIB_ENABLED BIT_1
#define AIB_AUTOPILOT BIT_2
#define AIB_BATTLER BIT_3
#define AIB_HUNTER BIT_4
#define AIB_MOB BIT_5
#define AIB_RAMMER BIT_6
#define AIB_DRONE BIT_7

struct shipai_data
{
	P_ship ship, target;
	int flags, type, timer[MAXAITIMER], t_room, mode;
	struct shipgroup_data *group;
	struct shipai_data *next;
};

struct shipgroup_data
{
	struct shipai_data *leader;
	struct shipai_data *ai;
	struct shipgroup_data *next;
};

#endif // _SHIP_AUTO_

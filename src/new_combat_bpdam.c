//
// includes functions related to taking damage/losing various
// bodyparts
//

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "justice.h"
#include "kingdom.h"
#include "new_combat.h"
#include "objmisc.h"
#include "spells.h"

//
// victDamagedHead
//

int victDamagedHead(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
// victLostHead
//

int victLostHead(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	send_to_char("lacking a head, you die.\r\n", vict);
	/*  die(vict);*/
	damage(ch, vict, TYPE_UNDEFINED, 30000); // woo!

	return TRUE;
}

//
//
//

int victDamagedEye(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victLostEye(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	/*  send_to_char("damn, you've gone blind!\r\n", vict);
	  SET_BIT(vict->specials.affected_by, AFF_BLIND);*/

	return FALSE;
}

//
//
//

int victDamagedEar(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victLostEar(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victDamagedNeck(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victLostNeck(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	send_to_char("lacking a neck and head, you die.\r\n", vict);
	/*  die(vict);*/
	damage(ch, vict, TYPE_UNDEFINED, 30000); // woo!

	return TRUE;
}

//
//
//

int victDamagedUpperTorso(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victLostUpperTorso(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	send_to_char("lacking an upper torso, you die.\r\n", vict);
	/*  die(vict);*/
	damage(ch, vict, TYPE_UNDEFINED, 30000); // woo!

	return TRUE;
}

//
//
//

int victDamagedLowerTorso(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victLostLowerTorso(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victDamagedUpperArm(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victLostUpperArm(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victDamagedLowerArm(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victLostLowerArm(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victDamagedElbow(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victLostElbow(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victDamagedWrist(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victLostWrist(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victDamagedHand(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victLostHand(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victDamagedUpperLeg(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victLostUpperLeg(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victDamagedLowerLeg(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victLostLowerLeg(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victDamagedKnee(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victLostKnee(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victDamagedFoot(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victLostFoot(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victDamagedAnkle(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victLostAnkle(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victDamagedChin(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victLostChin(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victDamagedShoulder(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victLostShoulder(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victDamagedBody(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victLostBody(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victDamagedWing(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victLostWing(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victDamagedBeak(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victLostBeak(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victDamagedJoint(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victLostJoint(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victDamagedEyestalk(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

//
//
//

int victLostEyestalk(P_char ch, P_char vict, const int loc, const int dam)
{
	int physType;

	physType = GET_PHYS_TYPE(vict);

	return FALSE;
}

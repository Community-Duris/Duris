/* ***************************************************************************
 *  File: events.h                                           Part of Duris *
 *  Usage: shared helpers for nevent callbacks and owner traversal.          *
 *  Copyright  1994, 1995 - John Bashaw and Duris Systems Ltd.             *
 *************************************************************************** */

#ifndef _SOJ_EVENTS_H_
#define _SOJ_EVENTS_H_

#include <stdint.h>
#include <sys/types.h>

#ifndef _SOJ_STRUCTS_H
#ifdef _LINUX_SOURCE
#include "structs.h"
#endif
#endif

enum class regen_resource : uint8_t
{
	hit,
	vitality,
	mana,
	ward
};

/* Current nevent owner-list traversal helpers. */

#define LOOP_EVENTS_CH(var, e_list) for ((var) = (e_list); (var); (var) = (var)->next_char_nev)

#define LOOP_EVENTS_OBJ(var, e_list) for ((var) = (e_list); (var); (var) = (var)->next_obj_nev)

#endif /* _SOJ_EVENTS_H_ */

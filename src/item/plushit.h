/*
   ***************************************************************************
   *  File: plushit.h                                         Part of Duris *
   *  Usage: enforce AFF3_SILVER and AFF3_PLUSONE..PLUSFIVE in melee.      *
   *                                                                        *
   *  Two source files and one two-line guard in fight.c's hit(), with no  *
   *  dependency on anything else new.                                     *
   *                                                                        *
   *  It ships DISABLED.  Both switches read lib/duris.properties at call  *
   *  time, so enabling, tuning or reverting needs no recompile and no     *
   *  reboot:                                                              *
   *      combat.plushit.enforce   0 = off (default), 1 = on               *
   *      combat.silver.enforce    0 = off (default), 1 = on               *
   ***************************************************************************
 */

#ifndef _PLUSHIT_H_
#define _PLUSHIT_H_

#include "structs.h"

/* TRUE  -> ch cannot hurt victim with this weapon; hit() should abort.
   FALSE -> proceed exactly as before.  Never TRUE while both properties
   are 0, so an unconfigured server behaves identically to master. */
int plushit_blocks(P_char ch, P_char victim, P_obj weapon);

#endif /* _PLUSHIT_H_ */

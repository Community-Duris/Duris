#ifndef EPIC_BANK_H
#define EPIC_BANK_H

#include "core/structs.h"
#include "persistence/critical_command.h"

/*
 * Epic points are banked only from epic.bank.minLevel (default 56). From
 * epic.gain.minLevel up to it, epic awards still feed artifacts and guild
 * prestige from their full amount but are paid as experience:
 *
 *   exp = epics x epic.convert.exp.<level>
 *
 * Kept apart from world/epic.h, whose bytes the epic zone seed manifest hashes.
 */

/** The lowest level that keeps epic points. */
int epic_bank_min_level();

/** True when the character's awards are banked as epic points. */
bool epic_level_can_bank(P_char ch);

/** Experience paid for one epic point earned at the given level below the bank level. */
int epic_conversion_exp_per_epic(int level);

/**
 * Pays an award earned below the bank level as experience. The caller has already
 * fed artifacts and guild prestige from the full epic amount.
 */
void epic_pay_converted_award(P_char ch, int type, int data, int amount);

/**
 * Epic points are never held below the bank level. A character below it with a
 * balance -- one who lost level 56 to a death, or is found holding points at login --
 * forfeits the whole balance through the epic ledger (reason bank_level_forfeit).
 * Does nothing for an immortal, a character at or above the bank level, or an empty
 * balance. A debit that cannot be queued is retried at the next login.
 */
void epic_forfeit_below_bank(P_char ch, critical_source_site site);

/** The epic skill price multiplier, epic.skill.costMultiplier (default 5). */
int epic_skill_cost_multiplier();

/** The epic skill refund multiplier, epic.skill.refundMultiplier (default 3). */
int epic_skill_refund_multiplier();

#endif

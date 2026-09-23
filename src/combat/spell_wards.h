#ifndef DURIS_SPELL_WARDS_H
#define DURIS_SPELL_WARDS_H

#include "core/structs.h"

#include <stddef.h>

enum spell_ward_source_type
{
	SPELL_WARD_SOURCE_NONE = 0,
	SPELL_WARD_SOURCE_CAST = 1,
	SPELL_WARD_SOURCE_EQUIPMENT = 2
};

struct spell_ward_absorb_result
{
	double remaining;
	double blocked;
	bool fully_blocked;
};

bool spell_ward_is_managed(const struct affected_type *af);
bool spell_ward_is_equipment(const struct affected_type *af);
bool spell_ward_is_active(const struct affected_type *af);

/* Equipment's legacy permanent bits are source markers, not active defenses. */
void spell_ward_mask_equipment_bits(unsigned long bitvectors[5]);

/* Called by the affect recalculation and save paths. */
void spell_ward_sync_timers(P_char ch);
void spell_ward_equipment_sync(P_char ch);
void spell_ward_cancel_events(P_char ch, struct affected_type *af);

/* Apply or refresh a finite cast-sourced ward. */
struct affected_type *spell_ward_apply_cast(P_char victim,
						   const struct affected_type *prototype,
						   int duration_ticks);

/* Called by event_short_affect for managed wards. */
void spell_ward_expire(P_char ch, struct affected_type *af);

/* Consume at most one eligible ward; overflow is returned to spell_damage. */
spell_ward_absorb_result spell_ward_absorb(P_char attacker, P_char victim, double damage,
							 unsigned int flags);

/* NPC spell selection and explicit object callbacks use this distinction. */
bool spell_ward_has_available(P_char victim, int spell);
bool spell_ward_item_callback_allowed(P_char victim, int spell);

/* Human-readable status for score/GMCP and tests. */
void spell_ward_status(P_char ch, char *buffer, size_t buffer_size);

/* Custom equipment refresh callback. */
void event_spell_ward_refresh(P_char ch, P_char victim, P_obj obj, void *data);

#endif

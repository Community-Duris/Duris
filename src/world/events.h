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
#include "core/structs.h"
#endif
#endif

enum class regen_resource : uint8_t
{
	hit,
	vitality,
	mana,
	ward
};

/* Group wake reschedules without changing deadlines or scheduler ordering. */
class nevent_reschedule_batch
{
    public:
	nevent_reschedule_batch();
	~nevent_reschedule_batch();
	nevent_reschedule_batch(const nevent_reschedule_batch &) = delete;
	nevent_reschedule_batch &operator=(const nevent_reschedule_batch &) = delete;

    private:
	bool active;
};

/* Current nevent owner-list traversal helpers. */

#define LOOP_EVENTS_CH(var, e_list) for ((var) = (e_list); (var); (var) = (var)->next_char_nev)

#define LOOP_EVENTS_OBJ(var, e_list) for ((var) = (e_list); (var); (var) = (var)->next_obj_nev)

// Serialized exact pool descriptor/list node/mmap pages + inline wheel arrays.
// Other scheduler registries, diagnostics and payloads are excluded. Strong output.
bool nevent_object_schedule_pool_storage_bytes(size_t *) noexcept;
// Caller outer includes the current observer EXACTLY ONCE plus all live inputs.
// Retain admitted peak through return and refresh pool allowance on EVERY return,
// including failure: configured chunk capacity belongs to the scheduler globally.
// Reserves capacity only; no event acquisition/callback/UID/publication authority.
bool nevent_reserve_object_schedule_slot_bounded(bool (*)(size_t, void *) noexcept, void *,
						 size_t outer_live) noexcept;
#endif /* _SOJ_EVENTS_H_ */

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

// Complete original fixed-registry integrity inspection and emitted logit
// predicates; actual fixed registry input/handle footprint admitted internally.
// Caller excludes that fixed registry footprint from outer, includes other live
// inputs/persistent allowances, and retains peak through return. Strong verdict.
// Native caller records returned effect before any fallible emitted diagnostic.
bool nevent_periodic_integrity_errors_bounded(bool emit, long *, bool (*)(size_t, void *) noexcept,
					      void *, size_t outer_live) noexcept;

// Complete original check_nevents(false) including emitted problem/summary and
// periodic diagnostics. Every private node/bucket request admitted by actual
// allocator rebind type before allocate; output queues/pager recounted between
// requests. Caller outer includes their initial observer ONCE. Optional verdict
// strong on refusal; return is completion (original scheduling ignores verdict).
// Caller records returned native effect BEFORE invoking any fallible diagnostic.
bool nevent_check_object_schedule_invariants_bounded(bool (*)(size_t, void *) noexcept, void *,
						     size_t outer_live,
						     bool *invariants_valid = nullptr) noexcept;

// Genuine object-only/no-payload scheduling, complete original wheel/owner links
// and full emitted debug diagnostic. Caller includes pool/output/Zombie globals
// and private stage/input retention once; refresh CURRENT retention EVERY return.
// Before action refusal leaves output/returned unchanged. Actual returned marker
// is written before post-enrollment fallible diagnostics; false may mean already
// scheduled, and NEVER permits rerunning the action. No new callback authority.
bool nevent_schedule_object_bounded(event_func_type, int, P_obj, nevent_schedule_result *,
				    bool *returned, bool *succeeded,
				    bool (*)(size_t, void *) noexcept, void *,
				    size_t outer_live) noexcept;
#endif /* _SOJ_EVENTS_H_ */

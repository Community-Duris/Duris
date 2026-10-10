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
// CURRENT genuine pending-reschedule/deferred maps (inline + actual tree nodes)
// and batch depth; other scheduler/pool/output storage excluded. Strong output.
// Caller includes once and refreshes CURRENT on EVERY return, including refusal.
bool nevent_native_reschedule_storage_bytes(size_t *) noexcept;
class nevent_native_reschedule_batch final
{
    public:
	nevent_native_reschedule_batch() noexcept = default;
	~nevent_native_reschedule_batch();
	nevent_native_reschedule_batch(const nevent_native_reschedule_batch &) = delete;
	nevent_native_reschedule_batch &operator=(const nevent_native_reschedule_batch &) = delete;
	// Caller owns inline batch and CURRENT observer once. Finish admits full
	// original allocating flush; on refusal destructor only balances depth and
	// retains original pending state. Already-returned advances must NEVER rerun.
	bool begin_bounded(bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
	bool finish_bounded(bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;

    private:
	bool active_ = false;
};
// Completion/actual returned advance distinguished. Full original handle/catch
// fallback semantics; strong output before invocation, prospective node request.
bool nevent_advance_by_bounded(const nevent_handle &, unsigned long long, bool *,
			       bool (*)(size_t, void *) noexcept, void *,
			       size_t outer_live) noexcept;

// Genuine character-only/no-victim/no-payload scheduling; SAME shared event
// pool and original wheel/character links. Pool/output CURRENT included once.
// Actual output/returned/succeeded are latched BEFORE fallible diagnostics;
// false after returned never authorizes a second scheduling action.
bool nevent_schedule_character_bounded(event_func_type, int, P_char, nevent_schedule_result *,
				       bool *, bool *, bool (*)(size_t, void *) noexcept, void *,
				       size_t) noexcept;
// ONLY actual cancellation vector inline+retained capacity. Does not duplicate
// shared pool or pending/deferred observers. Refresh common CURRENT every return.
bool nevent_character_cancel_storage_bytes(size_t *) noexcept;
// Complete original reschedule-after and actual prospective missing tree node;
// original active-dispatch refusal/ENOMEM fallback preserved. Strong output.
bool nevent_reschedule_after_bounded(nevent_handle, unsigned long long, bool *,
				     bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
// Original maintenance no-payload action only, complete original cancellation
// and prospective pending-cancellation growth; arbitrary payloads refuse.
bool nevent_cancel_character_maintenance_bounded(nevent_handle, nevent_cancel_result *,
						 bool (*)(size_t, void *) noexcept, void *,
						 size_t) noexcept;

// Pure complete original pending-reschedule flush source carriers, including
// genuine per-bucket std::sort introsort recursion and its heap/insertion/
// comparator/move tails. Separate from CURRENT maps, vector-array inline and
// prospective old/new vector heap supplied by original flush working profile.
// Strong output under genuine game-thread exclusion; no mutation or policy.
size_t nevent_native_reschedule_flush_source_observer_frame_bytes() noexcept;
bool nevent_native_reschedule_flush_source_frame_bytes(size_t *) noexcept;

#endif /* _SOJ_EVENTS_H_ */

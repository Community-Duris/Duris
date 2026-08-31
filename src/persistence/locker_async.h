//
// Async locker persistence: main-thread walk+snapshot, worker SQL apply.
// Coalesce per locker and pace at most one snapshot start per pulse.
//
#ifndef __LOCKER_ASYNC_H__
#define __LOCKER_ASYNC_H__

struct char_data;
typedef struct char_data *P_char;

#define LOCKER_ASYNC_SNAPSHOTS_PER_PULSE 1
#define LOCKER_ASYNC_MAX_INFLIGHT 1

#ifdef __cplusplus
extern "C"
{
#endif

	/* Mark a locker dirty. Coalesces multiple request in the same or later pulses.
 * terminal=1: leave/extract path (player leave fencing). terminal=0: in-stay save.
 * Returns 1 if queued/coalesced, 0 if async path unavailable (caller should sync). */
	int locker_async_mark_dirty(P_char chLocker, P_char chUser, int terminal,
				    const char *reason);

	/* True if this locker name has a dirty or in-flight async save. */
	int locker_async_name_busy(const char *locker_name);

	/* True while this player's object-manipulation cmds should be blocked. */
	int locker_async_player_obj_locked(P_char ch);

	/* Main-thread pulse: start at most LOCKER_ASYNC_SNAPSHOTS_PER_PULSE snapshots,
 * apply worker completion results, keep in-flight bounded. */
	void locker_async_pulse(void);

	/* Drain outstanding jobs (copyover/shutdown). Returns 1 if all drained. */
	int locker_async_drain(int wait_ms);

	void locker_async_init(void);
	void locker_async_shutdown(void);

	/* Optional helpers implemented in storage_lockers.c (C linkage). */
	void locker_async_request_resort(P_char chLocker, P_char chUser);
	void locker_async_restore_snapshot_view(P_char chUser);
	/* Run LockerToPFile for the user's active locker before snapshotting. */
	int locker_async_prepare_snapshot(P_char chUser);

#ifdef __cplusplus
}
#endif

#endif

# Takedown failure recovery

Bodyslam, springleap, and trip apply two combat pulses of recovery when a valid
attempt fails through the repaired branches in issue #187:

- Bodyslam receives `TAKEDOWN_PENALTY` from the shared helper.
- Springleap or trip bounces off an immovable horse/spider-body target (trip also
  includes the existing quadruped check).
- Trip encounters a target below or above its supported size window.

This uses the existing bash, maul, springleap, and trip penalty delay. Existing
messages, vitality charges, singing changes, and postures remain as before.
Bodyslam still pays 30 vitality and reaches its existing post-failure fall logic.

`TAKEDOWN_CANCELLED` is an early exit whose recovery is owned by
`takedown_check()`. It is not a universal no-lag result: attacking a sleeping
victim, freedom of movement, and evade can impose their own waits before
returning it. The caller must preserve those waits rather than replace them
with the generic two-combat-pulse penalty. Rejections such as an absent target
continue to return without an added wait.

`tests/async/test_takedown_recovery.py` executes the affected production failure
branches and neighboring penalty/cancellation controls, using production
`CharWait()` and `event_wait()` with a fixture scheduler. It checks deadlines,
wait-flag gating and release, and branch resource/position effects. It does not
simulate a complete combat session. The existing scheduler runtime suite covers
the real queue implementation separately.

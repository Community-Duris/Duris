# Incident Report & Investigation: Player Unable to Send Commands

**Incident Dates/Times:** August 25, 2026, ~14:33 IDT and ~14:53 IDT
**Severity:** High (player unable to play; no crash)
**Status:** Fixed, built, deployed, regression-tested
**Target Subsystems:** Command gate (`comm.c`), wait events (`events.c`), event wheel (`new_events.c`)

---

## 1. Symptom

Player `Amoz` stopped being able to do anything: commands produced no output and no
error, the connection stayed up, and the rest of the game kept running. Immortal
`Zusuk` was unaffected. Nothing appeared in `cmd.debug` for Amoz after the last
working command, even though `cmdlog()` is the first statement in
`command_interpreter()` — so the input was never being dequeued at all.

Occurred twice: after `kill illithid` (14:33) and after `kill mature` while fighting
an ancient roper (14:53).

## 2. Mechanism

`comm.c` gates the descriptor input queue on `CAN_ACT(ch)`, i.e.
`!IS_SET(ch->specials.act2, PLR2_WAIT)`. While that bit is set nothing typed is read.
The bit is set by `CharWait()` and cleared **only** by the `event_wait` event that
`CharWait()` schedules through `add_event()`.

Two independent ways for that to never happen:

1. **`add_event()` refuses the event.** It returns early on a negative delay or a
   dead `ch`, but `CharWait()` has already set the bit. Nothing else clears it, so
   the player is gated for the rest of the session.
2. **The event is scheduled but starved.** The event wheel runs under a per-pulse
   budget (25 ms) and a callback cap. Both were being hit constantly:

   ```
   NEVENT BUDGET: executed=1000 deferred=17379 scheduled=73844   (every pulse)
   ```

   with three compounding defects:
   - `nevent_defer_suffix()` only moved the *leading contiguous run* of due events to
     the next pulse. Any due event sitting behind a not-yet-due one was stranded in
     its ring bucket for a full revolution (300 pulses = 75 s), repeatedly.
   - Events the scan never reached never had their `timer` decremented, so long
     timers silently lost a whole revolution each saturated pulse.
   - The player-event promotion path was gated on `executed < max_callbacks`, so on a
     pulse that had spent its callback budget — which was every pulse — no player
     event was ever promoted. The priority mechanism was inert exactly when needed.

## 3. Fixes

`src/events.c` — `CharWait()`
- clamps a negative delay instead of handing it to `add_event()`
- clears `PLR2_WAIT` if `event_wait` did not actually get scheduled
- records `ch->specials.wait_until_pulse`, an absolute deadline in `ne_event_tick`
  pulses (`delay` + 2 s grace)
- logs any wait longer than a tick, to identify a bad caller

`src/comm.c` — command gate
- before reading input, clears `PLR2_WAIT` if there is no `event_wait` scheduled
  **or** the deadline has passed, and logs which case it was. A player can no longer
  be gated longer than the wait that was actually asked for.

`src/new_events.c` — event wheel
- `nevent_defer_suffix()` rewritten: walks the whole unscanned suffix, moves every
  due event to the next pulse in order, and decrements the timer of every event it
  leaves behind so nothing loses a revolution. Both bucket ends stay consistent.
- player-event promotion is no longer blocked by an exhausted callback cap (costs at
  most one over-cap callback per pulse)
- `NEVENT_MAX_CALLBACKS_DEFAULT` 1000 → 2000. The 25 ms wall-clock budget is the real
  bound; the count cap was ending pulses at roughly half the time budget.

`src/structs.h` — runtime-only `wait_until_pulse` in `char_special_data` (never saved).

## 4. Result

Measured before and after on the live server:

| | executed/pulse | deferred backlog | binding limit |
|---|---|---|---|
| before | 1000 | ~17,400 | callback cap |
| after | ~2000 | ~8,900 | 25 ms time budget |

## 5. Still open

The backlog is smaller but not gone: ~9,000 events are still deferred each pulse,
dominated by `event_mob_mundane`, which is roughly 1 s of systemic event lag. That is
event *volume*, not a correctness defect, and tuning it (mob event frequency, or
`DURIS_NEVENT_BUDGET_USEC` / `DURIS_NEVENT_MAX_CALLBACKS`) is a separate decision.
Player commands are no longer affected either way: the gate deadline is enforced
independently of the event system.

# Live casting and scheduler timing findings — 2026-09-10

## Result

The live logs confirm scheduler deferrals, including late `event_spellcast`
callbacks. The code allows those delays to accumulate across a multi-stage cast.
A separate casting/input-queue mismatch remains present. The one observed game
loop exceeding 250 ms was attributed to account-password handling, not events.
These are distinct mechanisms that can contribute to similar player-visible
symptoms; this review does not establish one cause for every reported incident.

## Evidence and scope

- Source: `fada5007eeb98d0a07e4eaf6ebd5a3d72482b120` (`Add correlated command
  latency attribution (#191)`).
- Running server: PID `841266`, boot ID `1789033805784928-841266`.
- Running executable SHA-256:
  `c17708986af42e2a7d39ad6edb262fe937be0a5934c313b70cc1ebd22bab3935`, matching
  the freshly rebuilt production executable verified during the restart.
- Read-only capture of `logs/log/status` and `logs/latency_trace.log` at
  **10:09:50 UTC**, covering the current boot beginning at approximately
  **09:50:05 UTC**. Status budget records reach scheduler tick `4727`.
- Fifteen complete trace windows cover 4,500 pulses, ending at
  **10:08:53.612053 UTC**. Historical trace blocks from other boots were excluded.
- No `DURIS_NEVENT_*` overrides exist in the running process environment.
  Defaults therefore apply: 25,000 us scheduler budget, 4,000 callback limit,
  up to 5,000 us catch-up extension and 4,000 extra callbacks.
- No production settings, code, or runtime state were changed for this review.
  No live casting reproduction or new regression tests were run. Existing
  local `.vscode`/ignore changes were preserved.

Statistics below were calculated from a single in-memory read of each log.
The files continue growing. Counts describe the captured interval, not the
current contents indefinitely. Raw player/account data is not included here.

## 1. Confirmed: event work repeatedly exhausts its budget

There were **1,985 `NEVENT BUDGET` records** in ticks 0–4727. Excluding the first
300 pulses to separate startup from subsequent operation leaves **1,828 budget
records in ticks 300–4727**—about 41% of that tick interval.

| Measurement, among post-startup budget records | Median | 95th percentile | Maximum |
| --- | ---: | ---: | ---: |
| Event pass duration | 28.216 ms | 31.584 ms | 38.904 ms |
| Deferred events | 462 | 2,109 | 13,309 |
| Maximum lateness of callbacks executed in the pass | 1 pulse | 2 pulses | 3 pulses |
| Slowest individual callback duration | 0.910 ms | 5.164 ms | 17.774 ms |

Percentiles use the sorted sample at `floor(0.95 * count)`. Deferred counts are
per-pass observations: an event can be counted again on subsequent deferrals.
They are not counts of unique lost events. These are conditional statistics
for budget-reporting passes, not all callbacks or all pulses.

`event_mob_mundane` was the slowest callback in 1,079 of these passes and
`event_patrol_move` in 492. This makes their work worth measuring further, but
being the slowest single callback does not establish which callback family
consumes the most aggregate time.

Across all fifteen complete trace windows, approximate sample-weighted means
were **23.756 ms for total loop work** and **22.247 ms for events**. These means
are reconstructed from integer window means. Event work dominates measured
active work, but a 25–30 ms event pass is still well below the nominal 250 ms
pulse interval. Frequent scheduler budget exhaustion is not equivalent to
frequent whole-loop overruns.

Source: [scheduler budgets and dispatcher](../../src/world/new_events.c),
`nevent_budget_usec`, `nevent_max_callbacks`, `ne_events`, `nevent_defer_suffix`.

## 2. Confirmed: spell callbacks execute late; individual player impact is unresolved

After tick 299, **50 budget records name `event_spellcast` as the maximally late
executed callback**: 41 report one pulse late and nine report two pulses late.
At four pulses per second, those are nominal delays of **250–500 ms**.

One concrete example at **09:51:56 UTC**:

```text
boot=1789033805784928-841266 tick=434
max_late_name=event_spellcast max_late_due=432
max_late_ticks=2 max_late_deferral=2
total_us=31631 deferred=2405 slowest=event_mob_mundane slowest_us=618
```

This proves a spell callback was executed two scheduler ticks after its due
tick. The generic report identifies neither caster nor spell. NPCs also use
`event_spellcast`, so it does **not** prove a particular player's spell was
delayed. A pass reports only its maximally late callback; these 50 records are
not a complete count of late spell callbacks. Tick lateness is also not an
exact wall-clock duration when pulse intervals vary.

In [sparser.c](../../src/net/sparser.c), `do_cast` sets `timeleft`, subtracts the
first 1–4-pulse segment, and schedules `event_spellcast`. Each continuation
subtracts another segment and schedules from the **current execution tick**.
In `new_events.c`, `add_event` sets `due_tick = ne_event_tick + delay`.
The continuation does not subtract the delay already incurred from `timeleft`.

Consequently, scheduler lateness can accumulate across a cast. For illustration,
an otherwise 12-pulse cast whose three callbacks each execute two pulses late
would finish in 18 pulses rather than 12, assuming regular pulse spacing.
That is a code-derived example, not a measured six-pulse player delay here.

Player-timed callbacks do receive priority by default, but
`nevent_sorts_before` compares **due tick before priority**. Older background
debt therefore precedes newly due player callbacks. Priority is not a guarantee
that casting bypasses an overdue backlog. Changing that ordering would require
fairness/starvation tests; raising or removing the budget blindly is not justified.

## 3. Confirmed code mismatch: casting can outlive command-queue protection

[comm.c](../../src/net/comm.c), around lines 1646–1662, selects the casting-safe
queue only when **both** `!CAN_ACT(ch)` and `AFF2_CASTING` are true. If casting
remains active but the wait gate is clear, ordinary input can be dequeued through
`get_playing_cmd_from_q`, which protects pending item/currency transactions but
does not independently preserve casting input.

[interp.c](../../src/cmd/interp.c), around line 1566, subsequently rejects that
ordinary command with `You're busy spellcasting!` and returns without requeueing
it. This is an input-loss path, not proof that the spell itself is stuck.

[events.c](../../src/world/events.c) has an independent `event_wait`, and
`CharWait` does not set the wait flag for trusted characters. `comm.c` can also
clear the gate when its event is absent or its deadline expires. These states
can therefore diverge. The current debug log contained **zero** automatic
`command gate: clearing stuck PLR2_WAIT` records, so deadline self-healing is not
an observed cause in this capture.

This confirms that the source mismatch described in the earlier
[spellcasting queue investigation](spellcasting-queue.md) still exists. The
comment claiming the wait flag covers the whole chant is stronger than the
actual queue-selection invariant. A focused repair should preserve ordinary
input based on active casting, while retaining permitted escape commands and
transaction gates. No repair was made in this review.

## 4. Confirmed: the captured whole-loop stall came from password-state dispatch

At **09:52:26 UTC**, boot/tick correlation joins these measurements at tick 554:

| Scope | Duration |
| --- | ---: |
| Nanny operation, state 61 | 258.294 ms |
| Entire command sweep | 258.303 ms |
| Event pass | 4.200 ms |
| Entire measured loop | 263.020 ms |

This was the only `COMMAND OP SLOW` and only `MUD TICK TOOK TOO LONG` record in
the captured boot. State 61 is `CON_GET_ACCT_PASSWD`, which dispatches
`get_account_password` synchronously. That path includes password verification
and may include other account work. Bcrypt verification is a candidate, but the
existing timer does not isolate it; attributing all 258 ms to hashing would
overstate the evidence.

Source: [account nanny](../../src/account/nanny.c),
[password handling](../../src/account/account.c),
[connection-state definitions](../../src/core/structs.h).
Time password verification and subsequent account work separately before choosing
a fix. Do not weaken password hashing to address game-loop latency.

## 5. Startup debt and observability limits

The worst reported lateness was **43 pulses**, for `event_mob_mundane` at tick
77, during startup. Boot logs record failed Redis clean-restart recovery followed
by a normal full zone boot. That is important context for the initial load, but
this review did not diagnose the recovery failure. The extreme startup values
should not be presented as steady-state casting delays; after tick 299 the
largest reported executed-callback lateness was three pulses.

Two existing diagnostic switches are disabled by default and absent from the
live environment:

- `DURIS_NEVENT_TRACE_PLAYER=1`: writes `PLAYER EVENT TIMING` to
  `logs/log/status`, including callback, sequence, player ID, due/actual tick,
  lateness, monotonic callback time, and callback duration. This is an existing
  useful player-timing trace, correcting the omission in the earlier discussion.
- `DURIS_NEVENT_ANALYTICS=1`: writes per-window callback counts/costs/deferrals
  and scheduler lateness distributions to the same file.

Both settings are cached after first use; setting a shell variable beside an
already-running process does not enable them. Neither emitted records in this
capture. Player tracing may produce substantial output and contains player IDs;
use a bounded capture and keep raw logs out of committed findings.

The ordinary 300-pulse latency reports are automatic and go to
`logs/latency_trace.log` and stderr/systemd journal. Their current-boot windows
reported zero dropped-section, dropped-contention, or invalid-clock samples.
They retain summaries and a top ten, not a full record of every operation.
Status logs rotate under `logs/old-logs/<timestamp>/` on supervised boot;
`latency_trace.log` spans boots, so filter by boot ID.

The runbook's example looking for `NEVENT BUDGET` in `logs/log/syslog` is stale:
`LOG_STATUS` resolves to **`logs/log/status`**. There was no current `syslog` file.

## Suggested next measurements

1. Capture a controlled mortal cast with player-event tracing enabled, retaining
   its callback due/actual ticks and monotonic times. Record spell and modifiers,
   expected duration, completion/abort, and queued-input behavior. Current traces
   lack a stable cast ID and end-to-end intended/completed duration; add those
   only if callback correlation proves insufficient.
2. Collect a bounded analytics window to distinguish aggregate NPC work from
   isolated expensive callbacks. Preserve startup versus steady-state separation.
3. Address the independently verified queue-preservation mismatch with a focused
   test covering casting active with the wait gate both set and clear.
4. Instrument the password-state substeps to explain the measured 258 ms stall.

These findings support targeted follow-up, not a blanket conclusion that the
event wheel is broken or that every casting complaint has one root cause.

## Follow-up: measured optimization candidates at 11:04 UTC

A bounded live `debug profile on` capture ran from approximately 11:04:15 to
11:04:53 UTC (152 pulses). Profiling was switched off before saving the results,
and the staff session was logged out. The normal health check passed afterward.
No code, scheduler budget, NPC cadence, or persistent tracing setting changed.
Measurements include profiling overhead and are one short live sample.

| Measured scope | Calls | Elapsed work | Share of callback work |
| --- | ---: | ---: | ---: |
| All event callbacks | 176,821 | 4.215 s | 100% |
| `event_mob_mundane` | 99,877 | 3.446 s | 81.8% |
| `mundane_wander` section within those callbacks | 96,754 | 2.599 s | 61.7% |
| `event_patrol_move` | 749 | 0.303 s | 7.2% |
| `mundane_mobcast` section | 99,146 | 0.343 s | 8.1% |
| `event_balance_affects` | 5,933 | 0.087 s | 2.1% |
| `event_move_regen` | 32,081 | 0.039 s | 0.9% |
| `event_spellcast` | 4,819 | 0.037 s | 0.9% |

Nested sections overlap their enclosing callbacks; percentages must not be
added together. The cast count includes NPCs. The `mundane_wagon` check cost
only 9.3 ms over the entire capture, so its old TODO is not a priority.

### Strongest candidate: avoid repeated global character-list scans during movement

`do_move` in [actmove.c](../../src/cmd/actmove.c), around line 2231, calls
`char_in_list(ch)` after every `do_simple_move`, including failed movement.
[utility.c](../../src/core/utility.c), around line 568, implements that predicate
as a linear scan of the global linked character list. Thus a local NPC step can
pay a cost proportional to the whole world's character population. Patrols also
use `do_move`. `char_to_room` performs another membership scan after executing
a destination room procedure when such a procedure exists.

The unconditional post-move scan was introduced by commit `13fe1f10f` on
2026-09-04 to prevent dereferencing a mover deleted by a destination script.
The safety requirement is valid: removing the check or replacing it with an
unguarded `IS_ALIVE(ch)` would reintroduce an invalid-pointer read.

This is a concrete algorithmic cost in the dominant measured section, making
it a stronger first candidate than changing world behavior. However, the
current profiler brackets all wandering work, not `char_in_list` separately.
The 61.7% figure is **not** a measured saving available from eliminating the
scan. Measure that predicate separately before promising a production speedup.

A replacement must keep membership checks safe without dereferencing a stale
pointer. An indexed membership lookup would need every insertion/removal path
covered, including mobile creation, login/copyover, staff loads, locker/artifact
temporary loads, extraction, and boot reset. Alternatively, a carefully tested
extraction-generation fast path could avoid scans when movement could not have
removed any character. Neither option was implemented in this discovery pass.
Existing movement-extraction contracts in `test_kingdom_contract.py` must stay
effective, and runtime coverage must include room-script extraction, ordinary
and blocked movement, NPCs, and pointer reuse if identity tracking is changed.

### Lower-priority candidates and exclusions

- Ordinary NPC activity already uses a three-times-longer interval when its zone
  has no remembered players. Increasing that slowdown changes world behavior;
  adding the existing optimization again achieves nothing.
- Movement regeneration accounts for 18.1% of callback count but only 0.9% of
  measured callback time. Reducing its callbacks may help dispatch overhead,
  but it is a weaker CPU target and requires checking regeneration semantics.
- NPC spell-up checks account for about 8.1% of callback time and are a second
  profiling target after movement. No specific safe suppression rule was proven.
- Raising event budgets would allow more work per pulse, not remove work. It
  should not substitute for fixing an expensive repeated lookup.

This follow-up completes candidate discovery only. There is no claimed
before/after improvement and no optimization deployed from this capture.

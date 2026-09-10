# Issue 180: command latency attribution — verified findings and plan

Status: implementation complete and validated 2026-09-10. All four planned
steps are implemented. Focused regressions and a local correlated slow-pulse
smoke passed; live playing-state checks are limited by the pre-existing local
test-character data failure recorded below.

Reviewed 2026-09-10 against `origin/master` at
`c9266bf552dce23cdb2632a921b4b96a092168f7`.
Source: [issue #180](https://github.com/Community-Duris/Duris/issues/180), including
the [casting follow-up](https://github.com/Community-Duris/Duris/issues/180#issuecomment-5609189204).
Line references below refer to that revision; symbols are the durable anchors.

## Current implementation state

The first implementation increment replaces lifetime trace summaries with an
atomic snapshot-and-reset API. Each 300-pulse report captures one immutable
window and renders that same snapshot to `logs/latency_trace.log` and stderr,
outside the producer mutex. The snapshot contains boot identity, UTC and
monotonic boundaries, 64-bit counts/durations, per-section aggregates, and an
exact bounded top ten maintained directly for the reporting window. The former
4096-entry recent ring was removed because no consumer remained after the
windowed top-ten path was introduced.
Records produced while output is written belong to the next window. A file-open
failure is reported while stderr still receives the captured snapshot.

Every game-loop sample now uses one `ne_event_tick` value captured at the start
of the pass, including samples written after `nevent_advance_tick`. Slow tick,
event-budget, event-catch-up, and slow-event records include the same boot ID,
absolute tick, and monotonic pulse-start value. Persistence-worker samples use
an explicit unavailable-tick sentinel, rendered as `-`. The unused scope macro
was removed because it had no call sites and depended on a non-standard GNU
statement expression. The process-global trace made the old utility and
persistence dump/reset wrappers redundant, so they were removed. Scalar queue
producers use a nonblocking trace path after releasing their queue mutex;
snapshots disclose samples dropped because the trace mutex was contended. The
fallback file-write sample also uses nonblocking recording after releasing its
application mutex. Section names are copied into fixed 64-byte storage; overlong
names and table saturation increment the dropped-section counter and are excluded
from both section statistics and the top ten. `latency_trace_snapshot_take_and_reset`
makes the consuming window boundary explicit; each snapshot owns its labels.

A native ASan/UBSan regression exercises bounded window reset, 64-bit ticks,
unavailable worker ticks, distinct process identities, identical dual rendering,
empty windows, file-open fallback, exact top ten after more than 4096 records,
section-name equality across translation units, visible section-table saturation,
nonblocking producers, and concurrent producers/snapshots with no summary-count
loss or duplication. The maintained server build and relevant existing tests
pass; commands and legacy profiler behavior are unchanged at this checkpoint.

The second implementation increment replaces `clock_t` profiler state with a
shared monotonic microsecond timer. One checked `timespec` conversion helper is
used by both the profiler and trace clocks, while the game loop and command
tracker use the trace clock and elapsed helper directly. All section and per-event accumulators,
function signatures, saves, averages, and the 50 ms `LONG EVENT` threshold now
use explicit microseconds. Failed or retrograde clock reads are excluded from
profile call counts and trace statistics. Trace windows disclose
`invalid_clock_samples`; genuine zero-microsecond measurements remain valid. Enabling the
debug profiler rebases timer endpoints without clearing accumulated results,
so time spent while profiling is disabled does not enter the next outside-time
sample. If profiling is enabled or reset from inside an otherwise unprofiled
outer command, that unmatched outer end is discarded rather than counted as a
partial sample. Existing scheduler callback/pass analytics were already monotonic and
remain unchanged. `LONG EVENT` now carries the same boot/tick/pulse correlation
fields as the other event diagnostics.

A native ASan/UBSan regression drives the timer with a controlled monotonic
clock through exact elapsed, outside, rebase, disabled, failed-read, and
retrograde-read cases. A real-clock case sleeps while eight worker threads burn
CPU and confirms the measured interval remains wall elapsed time. Source
contracts verify explicit microsecond output and the inclusive 50,000 us event
threshold.

The third implementation increment adds a fixed-size command-sweep tracker. It
times playing dispatch, nanny, pager, editor, SSL negotiation, and the remaining
per-descriptor prologue independently of debug profiling. At 50,000 us it
emits a correlated slow-operation record; at the same sweep threshold it emits
operation counts/totals/maxima plus explicitly labeled `unattributed_sweep_us`
for command gates, queue selection, instrumentation, and enclosing-loop work
outside the explicit operation/prologue scopes. Labels are copied before dispatch,
so commands that extract a character or close a descriptor cannot invalidate
diagnostic data. Playing records contain a canonical command-table label selected by the existing
queued-input lookup, or `unknown` for unrecognized input. Neither arbitrary
first tokens nor arguments are retained. Abbreviations and case variants map to
the table spelling. This labels the input command, not every nested command or
context-specific input handler it may invoke. Nanny, pager, and editor records
never retain their input. Player identity is
bounded and sanitized as well.

Each detailed report is capped at the eight slowest operations plus one summary
and at most six fixed kind summaries. The lines are collected into one bounded
buffer and written with one status-log call, without walking the descriptor list;
each continuation line receives the same wall-clock prefix. During a sustained
incident, writes are limited to one per four pulses. Intervening slow pulses
retain counters and exactly one worst slow operation without rendering or I/O.
The next due pulse, even if healthy, flushes that summary with the original worst
operation tick and pulse start. Unavailable ticks fall back to invocation counts
(one call per pulse). Per-kind totals from suppressed pulses are not retained.
`unreported_slow_operations` denotes operations omitted by the eight-item cap;
`suppressed_reports` denotes whole reports deferred by the throttle. Fast sweeps
without a pending summary emit nothing and do not format a wall-clock timestamp. SSL counts include negotiation attempts that round
to zero microseconds; their totals and maxima, rather than count alone, indicate
cost. The tracker lives in one small module shared by all dispatch paths and the
descriptor-maintenance scope; dequeue and dispatch selection are unchanged.

## Verification and limits

The code confirms missing attribution for work in the descriptor sweep. It does
not identify the operation responsible for the reported production stalls.
The quoted 265 ms and 1.11–1.16 s samples are supplied observations, not fresh
measurements reproduced in this investigation. No production system was
contacted or changed. The final section records a separate loopback-only
development smoke against the local database.

| Issue claim | Verdict and source evidence | Planned response |
| --- | --- | --- |
| 1. No attribution inside `commands_time` | Confirmed. [comm.c](../../src/net/comm.c):1365–1588 brackets descriptor maintenance and input handling. `dispatch_playing_command` routes through paging or the interpreter. [interp.c](../../src/cmd/interp.c):1446 onward has no per-command duration. Pager and editor branches also share this bucket. | Step 1: identify slow dispatches and remaining sweep cost. |
| 2. Profiler registration exists only for events | Confirmed: the only executable `PROFILE_REGISTER_CALL` invocation is [new_events.c](../../src/world/new_events.c):1620. Qualification: `LONG EVENT` depends on `do_profile`, initially false; `NEVENT SLOW` uses separate, already-monotonic callback/pass timing and does not depend on that macro. | Step 1 must work with profiling off; Step 3 corrects the legacy profiler without replacing event analytics. |
| 3. Trace tick values cycle | Confirmed for every explicit loop record in `comm.c` and the convenience macro in [latency_trace.h](../../src/persistence/latency_trace.h). `nevent_bucket_for_tick` and `pulse_update` in `new_events.c`:231,1733 show modulo buckets. Qualification: non-loop producers use zero, not `pulse`; a scheduler tick alone has no wall-clock or restart identity. | Step 2: stable per-pulse identity and explicit correlation metadata. |
| 4. Profiler uses the wrong clock for elapsed latency | Confirmed. [profile.h](../../src/core/profile.h):30–78 uses `clock_t`/`clock()`; `register_func_call` converts CPU ticks using `CLOCKS_PER_SEC`. It can count concurrent worker CPU and miss blocked wall time. Existing game-loop and event-analytics clocks are already monotonic. | Step 3: migrate all profiler units and consumers together. |
| 5. Section summaries never reset | Confirmed. `latency_trace_dump` only reads/sorts; the reset call is at boot in `comm.c`:863. Lifetime min/max/mean and recent-ring top ten describe different periods. The mean remains a valid lifetime mean, but poorly describes a current incident. | Step 4: explicit bounded reporting windows. |
| 6. Sub-250 ms command spikes lack direct reports | Confirmed for command attribution: `comm.c`:1877 gates the slow-tick status report at 250 ms. A 200 ms command can affect trace summary statistics or top ten, but neither names the operation. | Step 1: independent 50 ms dispatch reporting. |
| 7. `cmd.debug` cannot supply missing durations | Confirmed. `interp.c`:1455 gates `cmdlog` on debug mode; [debug.c](../../src/core/debug.c):93–123 uses second-resolution timestamps, no duration, and rewinds at each 500th entry. | Use the new diagnostic path; rewriting `cmd.debug` is unnecessary. |

Additional corrections to the issue's reasoning:

- `connections_time` ends before the descriptor command sweep starts. Its small
  value says nothing about SSL, liveness checks, timeout cleanup, queue scans, or
  wait recovery inside `commands_time`. The sweep executes at most one dequeued
  line per descriptor, not one per server pulse. Multiple moderately slow players
  can sum to a large bucket. A single blocking command is a hypothesis.
- WebSocket `time(0)` calls are conditional on handshake/open state, not one
  unconditional call for every descriptor.
- The ring holds 4096 records shared with persistence producers. The loop emits
  ten records unconditionally and optionally `gmcp_flush`, so its theoretical
  coverage is roughly 372–410 pulses before persistence records consume space,
  not approximately 455. A 300-pulse dump interval does not guarantee that every
  sample survives to a dump. See [persistence_queue.c](../../src/persistence/persistence_queue.c)
  (`scalar_enq_ok/drop`) and [utility.c](../../src/core/utility.c)
  (`fallback_file_write`).
  The implementation removed that ring once the independent window top ten made
  it write-only.
- Scheduler budgets are cooperative. `new_events.c`:1615–1631 executes a whole
  callback before checking limits at 1662–1673. Defaults are 25,000 us and 4,000
  callbacks, with configurable catch-up extensions (default up to 5,000 us and
  4,000 additional callbacks). Cleanup and diagnostics add time too. Thus a
  budget-enabled event pass can exceed 265 ms if one callback blocks. The
  reported small `ne_events` bucket, rather than an absolute budget guarantee,
  supports locating that particular sample's dominant cost outside the pass.

## Casting follow-up

The structural distinction is confirmed. In [sparser.c](../../src/net/sparser.c),
`do_cast` schedules normal casting through `add_event(event_spellcast, ...)`
at 2519. `CMD_INSTACAST` and successful spellweave instead invoke
`event_spellcast` synchronously at 2388 and 2398. Parsing can call world-target
lookups; [handler.c](../../src/world/handler.c)'s `get_char_vis` and `get_obj_vis`
contain world-list searches. This establishes possible command-path work, not
its duration or its contribution to either reported spike cluster. Normal cast
setup is not proven cheap under all world states.

The queue mismatch remains in current source: `comm.c`:1549 requires both
`!CAN_ACT` and `AFF2_CASTING` to select casting-aware dequeue, while the
interpreter rejects ordinary commands whenever casting remains active.
[events.c](../../src/world/events.c)'s `event_wait` and `CharWait` independently
clear/set the action wait, skip setting it for trusted characters, and establish
a deadline with `2 * WAIT_SEC` grace. `event_spellcast` reschedules continuations
relative to actual execution. Commands precede the event pass. Those paths
support input loss when wait clears before casting finishes, but do not prove
that a particular production rejection arose from scheduler deferral or the
self-heal deadline.

The existing [spellcasting queue investigation](spellcasting-queue.md) records
live reproductions and its evidence limits. Its repair and acceptance matrix
remain a separate work item. This latency plan must preserve those queue gates;
it should provide the evidence needed to distinguish pulse stalls from input
loss. Stable syscall-timeout explanations and world-search performance claims
remain hypotheses until dispatch measurements support them.

## Implementation sequence

### 1. Attribute slow descriptor work without enabling debug profiling — complete

- Time the existing playing, nanny, pager, and editor dispatch branches with
  `CLOCK_MONOTONIC`, using the existing loop helper. Emit a slow-operation record
  at elapsed time >= 50,000 us even when the whole pulse is below 250 ms.
- Record operation kind, elapsed microseconds, pre-dispatch connection state,
  copied player identity when present, and the pulse correlation fields from
  Step 2. For playing commands, record a bounded canonical command-table label
  or `unknown`; retain no input arguments. Never record
  nanny input, passwords, editor/mail bodies, or arbitrary command arguments.
  Raw command text from the suggestion is unnecessary to identify a handler.
- Capture required identity/labels before dispatch and use owned values after
  it returns: quit, account transitions, and commands can close descriptors or
  extract characters. Do not dereference those objects for logging afterward.
- Time SSL negotiation separately. Keep the existing aggregate `commands_time`
  for compatibility, add dispatch counts/totals/maxima by kind, and report
  maintenance/unattributed elapsed time as the enclosing sweep minus measured
  operations. This residual includes instrumentation overhead; label it as
  such. Report a slow sweep at the same threshold so many sub-threshold calls
  or maintenance cost are visible even without a single slow dispatch.
- Keep diagnostic output bounded per pulse, with explicit suppressed counts
  and the slowest operation retained if a cap is reached. Avoid a new telemetry
  service, broad interpreter instrumentation, or per-command dynamic registry.

Acceptance: execute controlled 49 ms/50 ms/200 ms cases with profiling off;
verify thresholds and operation labels, aggregate many short calls, delayed SSL
and maintenance, and disconnect/extraction during dispatch under sanitizers.
Assert that nanny/editor/free-text bodies and playing-command arguments never
enter output; unrecognized playing tokens must become `unknown`, while
recognized input uses the command-table spelling. Verify pager/editor and casting/transaction queue selection remain
unchanged. Busy pulses must produce bounded, throttled diagnostics and disclose
both per-pulse and cross-pulse suppression.

### 2. Make trace and status records joinable — complete

- Capture `ne_event_tick` once at pulse start and reuse that value throughout
  the pulse. Keep scheduler `pulse` unchanged. Use an explicit unsigned 64-bit
  tick type through storage, record API, formatting, and relevant helpers.
- Add the same tick to slow-operation, slow-sweep, slow-tick, and event slow/
  budget records. Include a process/boot identity and UTC timestamp mapping in
  diagnostic headers, plus monotonic pulse-start time, so records can be joined
  within a run and distinguished across restarts. A modulo-to-tick replacement
  alone does not supply that mapping.
- Remove the unused, non-standard `LATENCY_TRACE` convenience path. Audit all
  producers: persistence records currently pass zero. Represent their tick as
  unavailable unless a safe explicit context is supplied; do not read mutable
  loop state from workers or silently imply that those records belong to tick 0.

Acceptance: record pulses spanning `PULSES_IN_TICK`, values above 32-bit range,
and two boot identities; verify matching IDs before/after the event pass and
unambiguous non-loop records. Join a synthetic slow command to the corresponding
trace and slow-tick report using emitted fields alone.

### 3. Correct legacy profiler clocks and units — complete

- Replace CPU ticks in `PROFILE_DEFINE/DECLARE/RESET/START/END` with monotonic
  elapsed values. Choose microseconds for stored/output durations and label them
  explicitly. Migrate `register_func_call`, `save_profile_data`,
  `save_func_call_info`, and their conversions in the same change; remove the
  implicit `CLOCKS_PER_SEC` and `/1000` unit assumptions.
- Preserve profiling enable/disable/reset semantics and callback counts; ensure
  baseline timestamps are valid across toggles. Clock-read failures must not
  manufacture enormous or negative elapsed samples. Keep existing monotonic
  `NEVENT SLOW` and scheduler budget accounting intact.

Acceptance: a sleeping callback registers elapsed time, worker CPU is not added
to the measured duration, and the 50 ms `LONG EVENT` threshold uses correct
units. Test reset, toggles, zero calls, and output conversion. Use controlled
clocks where exact boundaries matter; avoid timing-sensitive equality tests.

### 4. Give periodic summaries consistent windows — complete

- Define each report as the samples since the previous snapshot, with window
  start/end metadata. Capture and reset under the trace mutex, then format the
  captured data to both the file and stderr outside the producer lock. Do not
  reset inside each `latency_trace_dump`: the current caller dumps twice and
  would otherwise empty the second destination.
- Retain min/max/count/total and the window's exact top ten using bounded
  storage. This prevents additional persistence records from displacing a worst
  sample before the 300-pulse dump. Remove the unconsumed recent ring, compare
  process-global section labels by content, and disclose samples dropped when
  the bounded section table saturates.
- Handle file-open/write failures explicitly while still delivering the same
  snapshot to stderr. New records arriving during output belong to the next
  window and must not be cleared by a later reset.

Acceptance: two windows with different spikes must not retain the first max;
both output destinations receive identical snapshot content; more than 4096
records retain the window's true worst ten; concurrent record/snapshot activity
loses or duplicates no summary counts. Exercise empty windows and file failure.

Steps 1 and 2 were implemented around shared correlation fields. Steps 3 and 4
were delivered as independent monotonic-profiler and bounded-window increments.
The acceptance notes remain here as the authoritative regression scope.

## Scope review and validation

Plan-ablation review retained each step because it addresses a confirmed gap:
attribution, correlation, wrong elapsed units, or inconsistent reporting windows.
Reuse existing status logging and timing rather than introduce a metrics backend.
Do not rewrite `cmd.debug`, tune scheduler budgets, optimize DB/DNS/world scans,
or duplicate the casting queue repair without measurements establishing need.
A wholesale descriptor-loop refactor remains unnecessary; a bounded scope timer
attributes the existing descriptor-maintenance path without changing its control
flow, while the remaining sweep residual covers instrumentation overhead.

For implementation, add focused behavioral regressions under `tests/async/`
covering the acceptance cases above; source-string checks alone cannot establish
timing correctness, lifecycle safety, or snapshot concurrency. Run
`./scripts/format.sh --check`, `make -C src`, and the relevant existing timing,
queue, and scheduler tests after C/C++ changes. After an authorized development
deployment, smoke-test playing/login/pager/editor paths and collect correlated
slow-operation samples before proposing a root-cause optimization. Production
deployment or invasive production probes are outside this branch's deliverable.

Documentation-stage validation passed:

```bash
python3 -B tests/async/test_documentation_contract.py
python3 -B tests/async/test_tick_latency_instrumentation.py
python3 -B tests/async/test_latency_trace_global_state.py
python3 -B tests/async/test_nevent_budget_contract.py
python3 -B tests/async/test_command_gate_recovery.py
python3 -B tests/async/test_spell_abort_command.py
```

The documentation contract ran 12 checks, including maintained-document links.
The other checks confirm existing source contracts; they do not reproduce the
reported stall, prove the proposed fixes, or refute the casting mismatch. In
particular, the abort contract accepts the existing wait-dependent queue gate.
No server build or full/database regression suite was run for this Markdown-only
change. The staged document also passed `git diff --cached --check` before commit.

### Implementation checkpoint: trace windows and correlation foundation

Passed after the first code increment:

```bash
python3 -B tests/async/test_latency_trace_runtime.py
python3 -B tests/async/test_tick_latency_instrumentation.py
python3 -B tests/async/test_latency_trace_global_state.py
python3 -B tests/async/test_nevent_budget_contract.py
python3 -B tests/async/test_nevent_scheduler_runtime.py
python3 -B tests/async/test_casting_input_queue_runtime.py
python3 -B tests/async/test_command_gate_recovery.py
python3 -B tests/async/test_spell_abort_command.py
python3 -B tests/async/test_documentation_contract.py
./scripts/format.sh --check
make -C src -j2
git diff --check
```

The scheduler and latency runtime suites ran under ASan/UBSan. The build was a
clean dependency-driven rebuild of the maintained MariaDB/development server.
The final development smoke is recorded below.

### Implementation checkpoint: monotonic legacy profiler

Passed after the profiler increment:

```bash
python3 -B tests/async/test_profile_monotonic_runtime.py
python3 -B tests/async/test_nevent_scheduler_runtime.py
python3 -B tests/async/test_latency_trace_runtime.py
python3 -B tests/async/test_tick_latency_instrumentation.py
./scripts/format.sh --check
make -C src -j2
git diff --check
```

The new profiler regression and the scheduler suite ran under ASan/UBSan. The
final development-smoke limitation for the configured test character is
recorded below.

### Implementation checkpoint: command attribution

Passed during the command-attribution increment:

```bash
python3 -B tests/async/test_command_latency_runtime.py
python3 -B tests/async/test_casting_input_queue_runtime.py
python3 -B tests/async/test_command_gate_recovery.py
python3 -B tests/async/test_spell_abort_command.py
python3 -B tests/async/test_tick_latency_instrumentation.py
./scripts/format.sh --check
make -C src -j2
git diff --check
```

The new native test runs under ASan/UBSan. It covers 49,999/50,000/200,000 us
durations with no profiler dependency, all six kinds, aggregate short work,
descriptor maintenance and unattributed sweep time, pre-dispatch copied identity, argument and nanny-input
redaction, control-character sanitization, exact 64-bit correlation fields,
SSL attribution, output capping, suppression counts, and slowest retention.
Existing source contracts confirm pager/editor/playing/nanny selection and the
casting and transaction-aware queue gates remain in place. Final full focused
regression and local runtime results follow.

### Final focused regression

Passed on the completed implementation:

```bash
python3 -B tests/async/test_command_latency_runtime.py
python3 -B tests/async/test_latency_trace_runtime.py
python3 -B tests/async/test_profile_monotonic_runtime.py
python3 -B tests/async/test_tick_latency_instrumentation.py
python3 -B tests/async/test_latency_trace_global_state.py
python3 -B tests/async/test_reported_latency_contract.py
python3 -B tests/async/test_nevent_budget_contract.py
python3 -B tests/async/test_nevent_scheduler_runtime.py
python3 -B tests/async/test_casting_input_queue_runtime.py
python3 -B tests/async/test_command_gate_recovery.py
python3 -B tests/async/test_spell_abort_command.py
python3 -B tests/async/test_documentation_contract.py
python3 -B tests/async/test_minimal_boot.py
./scripts/format.sh --check
make -C src -j2
git diff --check
```

The command-attribution, trace-window, profiler, and scheduler runtime suites
ran under ASan/UBSan. The maintained MariaDB/development server build completed
with its warning-as-error profile.

### PR review hardening

The adversarial PR review follow-up was validated with the focused regression
matrix above and the complete maintained non-database gate:

```bash
make test-all
./scripts/format.sh --check
git diff --check
```

All 437 Python regressions and the native signal-handler gate passed. The build
covered the server, area editor, area generators, migration tools, and flatfile
targets. The focused sanitizer tests additionally cover command-report bounds
and correlation, full/throttled report sequencing, cross-pulse worst-event
retention, bounded multi-line collection, failed and retrograde clock reads,
unavailable tick rendering, content-equal section labels, visible section-table
saturation, concurrent trace snapshots, nonblocking producer accounting, and
the profiler rebase inside an unmatched outer command scope.
Database-container tests were not run; this review hardening changes no schema
or database behavior.

### Local correlated runtime smoke

The standard minimal launcher validated the local environment, applied no
pending migrations, and passed runtime schema compatibility, but stopped before
boot because its backup subprocess could not shell-source one existing unquoted
credential value in `.env`. The file was not edited. For branch qualification,
the built server was started directly in minimal mode with the same local
values loaded literally and the launcher's normal local development database
name resolution. It booted normally on loopback, reported
`{"status":"healthy","persistence":"ready"}`, accepted a TLS connection, and
stopped normally after each run.

The configured account completed account-name, password, login-page, menu, and
character-selection nanny states. Both distinct characters on that account
then failed closed during asynchronous materialization because their existing
local item graphs reference 6 and 58 missing payload rows, respectively.
Repairing those rows is unrelated player-data work, so the live playing, pager,
editor, queue-command, and `debug profile on/save/reset/off` checks were not
run. Their implementation coverage remains the focused sanitizer regressions
and source contracts above.

A valid nanny password check was then run under the same bounded CPU-contention
method used by the profiler qualification: the local server and two CPU workers
temporarily shared one allowed CPU, and the server's original affinity was
restored afterward. This produced a 430,497 us slow nanny operation, a 430,501
us command sweep, and a 430,683 us slow tick. All three records shared boot ID
`1788994073434007-2562719`, tick `374`, and pulse start `175329626420` us. The
next immutable trace window retained `commands=430575 us` and
`total_tick=430683 us` at tick `374` as its two worst samples.

The slow-operation output was exactly one operation, one sweep summary, and one
kind summary. It identified `kind=nanny`, numeric connection state, and no
player or input (`player_id=-1`, `player=-`, `operation=nanny`), with
`suppressed=0` and 4 us of unattributed sweep time. An automated post-run check
joined the operation, sweep, slow tick, and trace window using emitted fields
alone and confirmed that none of the configured account, password, or character
values appeared in any command diagnostic. No production system was contacted.


### Outstanding PR review follow-up (2026-09-10)

The latest adversarial review was checked against PR head `42cc2bd56`.

1. Removed all emitter calls on throttled pulses. Pending evidence flushes on a
   due healthy pulse as well as during sustained incidents.
2. Ended descriptor timing before command gates and dequeue selection, leaving
   a real residual for that otherwise unmeasured sweep work.
3. Reused queued-input resolution for table-owned playing labels and `unknown`;
   raw input tokens no longer cross the logging boundary.
4. Copied trace section/top labels into owned bounded arrays. Rejected section
   samples cannot appear only in the top ten.
5. Renamed the consuming snapshot operation to `take_and_reset` everywhere.
6. Invalid clock intervals are skipped and counted separately from real zero
   samples; slow-tick diagnostics render unavailable durations as `-`.
7. Fallback tracing is nonblocking and runs after the fallback mutex is released.
8. Slow-tick breakdowns now share one correlated line.
9. Timestamp preparation happens only on the first emitted command-report line;
   event correlation lookups happen only in reporting branches. Boot-ID storage
   is explicitly immutable for the process lifetime.
10. Verified that `contract_text.contains` already ignores code whitespace.
    Simplified the call assertion and removed the initialization micro-optimization
    assertion; retained behavior and ordering contracts.
11. Failed profiler intervals no longer increment measured call counts or
    per-function event registrations; genuine zero-duration calls still count. Skipped
    rebase ends refresh the outside-time baseline. Suppression names and retention
    limits are documented, and missing ticks cannot bypass throttling.

Focused sanitizer harnesses cover the changed behavior, including actual
command-table resolution, secret-like unknown tokens, silent emitter counts,
recovery flush, missing ticks, freed/mutated label storage, invalid clocks versus
real zero samples, and snapshot isolation. Queue/casting/gate, scheduler, latency,
documentation, and minimal-boot contracts also pass.

The local player-data limitation documented above remains: live playing, pager,
editor, and debug-profile command execution is not claimed by these harnesses.
No player data or credentials were repaired for this review.


The complete local non-database gate on the primary review fixes passed all 437
Python regressions and the native signal-handler gate. It included real isolated
flatfile account recovery, character creation, combat/reload, full-world boot,
and item-movement prompt journeys. The final per-function profiler validity
follow-up passed the extended profile and scheduler ASan/UBSan regressions and
`make -C src -j4`; final PR CI rechecks the complete gate on that follow-up.

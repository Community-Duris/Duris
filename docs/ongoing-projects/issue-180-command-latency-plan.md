# Issue 180: command latency attribution — verified findings and plan

Status: code implementation complete; final regression and local runtime smoke
validation pending. All four planned steps are implemented on this branch.

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
exact bounded top ten maintained independently of the 4096-entry recent ring.
Records produced while output is written belong to the next window. A file-open
failure is reported while stderr still receives the captured snapshot.

Every game-loop sample now uses one `ne_event_tick` value captured at the start
of the pass, including samples written after `nevent_advance_tick`. Slow tick,
event-budget, event-catch-up, and slow-event records include the same boot ID,
absolute tick, and monotonic pulse-start value. Persistence-worker samples use
an explicit unavailable-tick sentinel, rendered as `-`. The unused scope macro
uses the absolute scheduler tick. The process-global trace made the old utility
and persistence dump/reset wrappers redundant, so they were removed. The
fallback file-write sample also moved from process CPU time to monotonic elapsed
time while its producer was being audited.

A native ASan/UBSan regression exercises bounded window reset, 64-bit ticks,
unavailable worker ticks, distinct process identities, identical dual rendering,
empty windows, file-open fallback, exact top ten after more than 4096 records,
and concurrent producers/snapshots with no summary-count loss or duplication.
The maintained server build and relevant existing tests pass; commands and
legacy profiler behavior are unchanged at this checkpoint.

The second implementation increment replaces `clock_t` profiler state with a
shared monotonic microsecond timer. All section and per-event accumulators,
function signatures, saves, averages, and the 50 ms `LONG EVENT` threshold now
use explicit microseconds. Failed or retrograde clock reads produce a zero
duration rather than an underflow while preserving call counts. Enabling the
debug profiler rebases timer endpoints without clearing accumulated results,
so time spent while profiling is disabled does not enter the next outside-time
sample. Existing scheduler callback/pass analytics were already monotonic and
remain unchanged. `LONG EVENT` now carries the same boot/tick/pulse correlation
fields as the other event diagnostics.

A native ASan/UBSan regression drives the timer with a controlled monotonic
clock through exact elapsed, outside, rebase, disabled, failed-read, and
retrograde-read cases. A real-clock case sleeps while eight worker threads burn
CPU and confirms the measured interval remains wall elapsed time. Source
contracts verify explicit microsecond output and the inclusive 50,000 us event
threshold.

The third implementation increment adds a fixed-size command-sweep tracker. It
times playing dispatch, nanny, pager, editor, and SSL negotiation independently
of debug profiling. At 50,000 us it emits a correlated slow-operation record;
at the same sweep threshold it emits operation counts/totals/maxima plus the
unmeasured descriptor-maintenance residual. Labels are copied before dispatch,
so commands that extract a character or close a descriptor cannot invalidate
diagnostic data. Playing records contain only a bounded sanitized first token;
nanny, pager, and editor records never retain their input. Player identity is
bounded and sanitized as well.

Output is capped at the eight slowest operations plus one summary and at most
five fixed kind summaries per pulse. The slowest operation is always retained,
and the summary discloses how many additional slow operations were suppressed.
Fast sweeps emit nothing. The tracker lives in one small module used by all five
descriptor call sites; the surrounding dequeue and dispatch selection is
unchanged.

## Verification and limits

The code confirms missing attribution for work in the descriptor sweep. It does
not identify the operation responsible for the reported production stalls.
The quoted 265 ms and 1.11–1.16 s samples are supplied observations, not fresh
measurements reproduced in this investigation. No production connection, database
operation, server restart, runtime configuration change, or log collection was
needed for this source-level verification.

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
  Step 2. For playing commands, record a bounded, sanitized command verb;
  optionally retain only explicitly safe diagnostic arguments. Never record
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
Assert no credentials or free-text bodies enter output. Verify pager/editor and
casting/transaction queue selection remain unchanged. A busy pulse must produce
bounded diagnostics and disclose suppression.

### 2. Make trace and status records joinable — complete

- Capture `ne_event_tick` once at pulse start and reuse that value throughout
  the pulse. Keep scheduler `pulse` unchanged. Use an explicit unsigned 64-bit
  tick type through storage, record API, formatting, and relevant helpers.
- Add the same tick to slow-operation, slow-sweep, slow-tick, and event slow/
  budget records. Include a process/boot identity and UTC timestamp mapping in
  diagnostic headers, plus monotonic pulse-start time, so records can be joined
  within a run and distinguished across restarts. A modulo-to-tick replacement
  alone does not supply that mapping.
- Update the unused `LATENCY_TRACE` convenience path consistently. Audit all
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
- Retain min/max/count/total and the window's exact top ten independently of
  ring eviction, using bounded storage. This addresses additional persistence
  records displacing a worst sample before the 300-pulse dump. Use sufficiently
  wide duration totals/counts and document that the ring is a recent sample
  buffer, not a complete window archive.
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
A wholesale descriptor-loop refactor is optional and deferred; residual sweep
measurement covers the immediate attribution requirement.

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
This checkpoint has not yet been smoke-tested in a running development server;
that remains part of the final implementation validation.

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

The new profiler regression and the scheduler suite ran under ASan/UBSan. No
runtime profiling session has been collected yet; final development smoke
testing will exercise `debug profile on`, `save`, `reset`, and `off` through the
configured test character.

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
durations with no profiler dependency, all five kinds, aggregate short work,
maintenance residual, pre-dispatch copied identity, argument and nanny-input
redaction, control-character sanitization, exact 64-bit correlation fields,
SSL attribution, output capping, suppression counts, and slowest retention.
Existing source contracts confirm pager/editor/playing/nanny selection and the
casting and transaction-aware queue gates remain in place. Final full focused
regression and local runtime results will be recorded below.

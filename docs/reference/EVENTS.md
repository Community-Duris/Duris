# Event system (`nevent`)

This is the mechanism reference for Duris's deferred-work scheduler. For the
game-loop context and incident-derived design constraints, see
[ARCHITECTURE.md](ARCHITECTURE.md#event-wheel).

The implementation is split by responsibility:

- `src/world/new_events.c` owns the timer wheel, scheduling, ordering, execution,
  cancellation, overload recovery, diagnostics, and invariant checks.
- `src/world/nevent_periodic.c` owns the process-local registry, uniqueness, cadence,
  and health of recurring jobs.
- `src/world/events.c` contains callbacks and callback-specific helpers such as
  regeneration, command waits, room events, and zone resets. It is not a
  second scheduler.
- `src/world/event_names.c` maps callback addresses to diagnostic names.
- `src/world/events.h` contains only the regeneration selector and the current
  character/object event-list traversal helpers. The old numeric event-type
  scheduler no longer exists.

## Clock and wheel

The scheduler uses a monotonic absolute tick, `ne_event_tick`. There are four
pulses per second and `PULSES_IN_TICK` is 300, so one physical wheel revolution
is 75 seconds. The revolution is an indexing detail, not the duration limit.

Every event records its absolute `due_tick` and is placed in:

```c
bucket = due_tick % PULSES_IN_TICK;
```

The current bucket can therefore contain deadlines from many revolutions.
Eligibility is determined by comparing `due_tick` with `ne_event_tick`; there
is no relative revolution counter to decrement. Long delays remain exact even
when a bucket is deferred under load.

`ne_events()` may run only once for a scheduler tick. A zero-delay event
scheduled before that pass is eligible in the current tick. Scheduling during
or after the pass clamps the deadline to the next tick. The pass also snapshots
the current sequence number, so callbacks cannot grow the active pass by
creating more immediately executable work.

## Event records and ownership

`nevent_data` records the callback, payload, absolute deadline, stable sequence,
priority, lifecycle state, deferral metadata, periodic-job identity, and safe
diagnostic owner identities. An event can be linked into three intrusive,
doubly linked lists at once:

| Links | Purpose |
| --- | --- |
| `prev_sched`, `next_sched` | One wheel bucket. |
| `prev_char_nev`, `next_char_nev` | The owning character's event list. |
| `prev_obj_nev`, `next_obj_nev` | The owning object's event list. |

Each list has a head and tail. Wheel and owner-list removal are O(1) once the
record is known. Owner-specific lookup is O(k) for that owner; the callback-only
lookup scans the wheel and should be reserved for diagnostics or rare paths.

When an event targets a different character, `char_link_data` connects owner
and victim. Breaking that link cancels the event instead of leaving a dangling
victim pointer. Runtime character IDs and the object vnum are captured while
owners are live so diagnostics never need to dereference a stale pointer just
to identify it.

Records come from the `NEVENTS` memory pool. Pool usage, the live-event counter,
and wheel membership are kept equal and are checked at lifecycle boundaries.

## Scheduling API

`add_event()` returns `nevent_schedule_result`, not a nullable record. A success
contains a `nevent_handle` made from the record address and its nonzero sequence;
the sequence prevents a pooled address from making an old handle valid again.
Failure statuses distinguish:

- null callbacks and negative delays;
- dead owners and victims without owners;
- invalid payloads;
- exhausted sequence space;
- invalid replacement targets; and
- calls from outside the game thread.

The raw payload overload copies bytes and accepts only trivially copyable C++
types. Use `add_event_owned()` for typed/non-trivial payloads; it stores the
payload with the matching destructor. Every teardown route invokes the recorded
destructor exactly once.

`nevent_replace()` and `nevent_replace_owned()` arm the successor before
cancelling the old handle. If successor scheduling fails, the original event
remains active. Existing events can be moved with `nevent_reschedule_at()`,
`nevent_reschedule_after()`, or `nevent_advance_by()` when no event pass is
active.

Callers must inspect scheduling failure whenever game state assumes that a
callback will later clear it. `CharWait()` is the command-gate example: it
restores a safe state if the wait event cannot be armed.

## Ordering and fairness

Within a bucket, insertion uses this total order:

1. earlier absolute `due_tick`;
2. higher effective priority; then
3. lower `sequence` (stable FIFO for otherwise equal events).

With `DURIS_NEVENT_PLAYER_PRIORITY=1`, latency-sensitive player callbacks have
player priority: command waits, spell casts, memorization, affect balancing,
and player regeneration. Ordinary events age above player priority after two
deferrals or two ticks of lateness. This keeps visible actions responsive
without allowing an endless player prefix to starve older maintenance work.

## Cancellation and destruction

Use `nevent_cancel(handle)` for one event, or `disarm_char_nevents()` and
`disarm_obj_nevents()` for all matching events owned by an entity. Passing a
null callback filter disarms every event for that owner.

Cancellation is sequence-validated and idempotent. Outside an active event
pass, it unlinks and destroys the record immediately. During a pass, it marks
the event cancellation-pending, clears the callback, detaches owners
immediately, and queues reclamation until iteration is safe. This protects
callbacks that cancel themselves or another record in the bucket while still
releasing all owner-facing references at once.

Destruction unlinks every remaining list, removes a victim link, invokes the
payload destructor, reconciles any deferral debt, decrements the live counter,
and returns the record to the pool. Results distinguish immediate cancellation,
deferred reclamation, stale handles, already-inactive events, invalid handles,
and wrong-thread calls.

## Per-pulse limits and overload recovery

The default base limits are 25 ms and 4,000 executed callbacks per pulse. A
zero value disables that individual limit; setting both to zero intentionally
makes the scheduler unbounded and emits a warning once. Configuration values
are rejected outside their documented range.

Time is checked after each callback and every 64 scans through not-yet-due
records. When either active limit is exhausted, every remaining due event in
the unscanned suffix is moved to the next bucket. Its original `due_tick`
remains authoritative, so lateness stays measurable and no long-delay event
loses a revolution.

The first deferral registers three forms of catch-up debt:

- callback count;
- an EWMA-based callback-cost estimate; and
- the oldest outstanding due tick.

Debt receives a front-loaded quota over a four-pulse repayment window. The
quota can extend both limits, capped by the configured catch-up allowances.
Old deadlines sort ahead of new arrivals, and aged ordinary work outranks new
player-priority work, so sustainable incoming load cannot hold the debt steady
forever. Executing or cancelling a deferred event removes its count, cost, and
due-tick debt exactly once.

A callback itself cannot be preempted. Heavy jobs must expose bounded slices
instead of hiding an unbounded sweep inside one invocation.

## Periodic jobs and continuations

The periodic registry gives each recurring job a unique string key, callback,
initial delay, interval, policy, enabled state, and one owned event handle.
Registering the same definition twice is suppressed; a conflicting key or a
callback registered under another key is rejected.

- `fixed_rate` advances from the prior deadline and counts intervals skipped
  while late.
- `fixed_delay` advances from callback completion.
- `nevent_periodic_retry_after()` records a failure and selects a retry delay.
- `nevent_periodic_next_after()` overrides the next successful interval.
- `nevent_periodic_continue_after()` schedules another bounded slice without
  counting the logical run as complete.

The watchdog runs after every event pass. It re-arms an enabled job that lost
its successor, reports mismatched live handles as corruption, and tracks missed
runs, schedule failures, callback failures, duplicate suppression, and watchdog
rearms. The registry currently owns 11 boot jobs; Redis-dependent jobs remain
registered but can be disabled by configuration.

Maintenance callbacks use one-tick continuations to bound their work. Current
slice caps are eight artifact-bind rows, one artifact-expiry row, four
artifact-war owners, eight dirty-player checkpoints, and four surname players.
Each logical scan uses stable cursors or runtime-ID snapshots so entity removal
between slices cannot invalidate traversal state.

## Restart and copyover durability

The wheel, its records, payloads, handles, and absolute deadlines are
process-local and are never persisted. The event contract has three explicit
durability classes:

- Ephemeral gameplay events include combat actions, casts, regeneration,
  command waits, animation, and similar owner-bound timers. A process restart
  may discard them; continued gameplay and owner-loading paths create new
  records as needed.
- Reconstructible events include room, zone, weather, and loaded-owner timers.
  Boot and load paths rebuild them from authoritative world or player state.
  Copyover combat restoration is a separate recovery path; it does not restore
  old wheel records or their remaining delay.
- Operational periodic jobs are registered on every boot. The registry
  guarantees uniqueness, cadence, and watchdog recovery only within the
  running process. Redis-dependent jobs remain registered but disabled until
  their dependency and recovery phase permit enablement.

Persistence-critical player, ship, artifact, and world state uses its
authoritative MySQL, journal, or Redis recovery pipeline. Those pipelines do
not depend on a one-shot nevent record surviving a restart. There are currently
no durable one-shot nevents. A future deadline that must survive restart must
persist domain state plus the deadline and reconstruct a new process-local
callback after recovery; persisting a wheel pointer, payload address, or
`nevent_handle` is invalid.

## Game-thread ownership

`ne_init_event_pool()` binds the scheduler to the game thread. Scheduling,
cancellation, rescheduling, lookup, event execution, periodic-registry access,
and invariant inspection all require that thread. A debug build treats a
wrong-thread call as corruption; a release build logs and rejects it. Worker
threads must return results through the established game-thread handoff rather
than mutate nevent state directly.

## Diagnostics

The `world events` command reports wheel, counter, pool, invariant, last-pulse,
and periodic-registry health. `world events periodic` shows every registered
job, including armed/running state, calls, completed logical runs, continuation
slices, failures, missed runs, and the next deadline. Supplying a callback name
lists matching records with deadline, lateness, effective priority, deferrals,
sequence, and captured owner/victim/object identities plus live checks.

The optional analytics window uses a dynamic callback map, so distinct callback
functions are never aliased into a fixed slot. It records full scheduler wall
time, callback costs, deferrals, pending peaks, budget exhaustion, and lateness
histograms. Relevant logs are:

| Log prefix | Meaning |
| --- | --- |
| `NEVENT BUDGET` | Work was deferred after a limit was reached. |
| `NEVENT CATCHUP` | Debt was added or a repayment quota was active. |
| `NEVENT SLOW` | Total scheduler work for the pulse reached 50 ms. |
| `NEVENT ANALYTICS WINDOW` | One 300-pulse aggregate window. |
| `NEVENT ANALYTICS CALLBACK` | Per-callback timing and deferral totals. |
| `PLAYER EVENT TIMING` | Optional per-player deadline trace. |

The expensive invariant checker is observation-only. It verifies reciprocal
wheel and owner links, unique sequences, lifecycle and payload ownership,
live-owner identities, victim links, periodic metadata, pool/counter equality,
and exact catch-up debt. It reports corruption but never repairs or severs
links while inspecting them.

## Configuration

| Variable | Default | Allowed | Effect |
| --- | ---: | ---: | --- |
| `DURIS_NEVENT_BUDGET_USEC` | `25000` | `0..1000000` | Base wall-clock limit per pulse; `0` is unlimited. |
| `DURIS_NEVENT_MAX_CALLBACKS` | `4000` | `0..1000000` | Base callback limit per pulse; `0` is unlimited. |
| `DURIS_NEVENT_CATCHUP_MAX_EXTENSION_USEC` | `5000` | `0..1000000` | Maximum time added while repaying debt. |
| `DURIS_NEVENT_CATCHUP_MAX_EXTRA_CALLBACKS` | `4000` | `0..1000000` | Maximum callback capacity added while repaying debt. |
| `DURIS_NEVENT_PLAYER_PRIORITY` | `1` | `0..1` | Enables player-timed priority. |
| `DURIS_NEVENT_TRACE_PLAYER` | `0` | `0..1` | Emits per-player timing logs. |
| `DURIS_NEVENT_ANALYTICS` | `0` | `0..1` | Emits 300-pulse scheduler analytics windows. |

## Core invariants

- One bound game thread owns all scheduler and periodic-registry state.
- `ne_events()` runs exactly once per monotonic tick and is not re-entrant.
- The deadline and sequence cutoff prevent callbacks from extending their own
  active pass.
- Every live record has one wheel membership; each live owner has one matching
  owner-list membership.
- Pool usage, the live counter, and wheel cardinality agree.
- Handles are valid only while both address and sequence match an active record.
- Every accepted payload has exactly one matching destruction path.
- Cancellation detaches owners immediately and reclaims at the first safe point.
- Deferral preserves absolute deadlines and debt metadata until execution or
  cancellation.
- A registered, enabled periodic job is running or owns exactly one successor.
- Heavy maintenance advances through bounded, resumable slices.

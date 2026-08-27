# NEvent System End-to-End Review

Date: 2026-08-27

Status: Implementation in progress; checkpoint 9 periodic registry complete

Last implementation update: 2026-08-28 00:20 IDT

Scope: The current `nevent` scheduler, its callers, event ownership and payloads,
boot/reconstruction behavior, recurring jobs, overload controls, diagnostics,
documentation, and tests.

## Implementation progress

This section is the continuation ledger for the remediation work. A checkpoint
is marked complete only after its focused regression, the existing nevent
contracts, formatting, and the server build pass.

| Checkpoint | Scope | State | Verification |
|---|---|---|---|
| 1 | NEV-01 event-name loader safety | Complete | ASan/UBSan boundary harness, 12 existing nevent contracts, format check, server build |
| 2 | NEV-05 and NEV-10 cancellation/lifetime invariants | Complete | ASan/UBSan cancellation harness, 221 Python regressions, native signal test, format check, server build |
| 3 | NEV-02 and NEV-08 typed/POD hunt payload | Complete | ASan/UBSan ownership and stable-ID harnesses, raw-type compile rejection, 222 Python regressions, native signal test, format check, server build |
| 4 | NEV-03 stable ship-volley references | Complete | ASan/UBSan live, deleted-endpoint, and ABA harness, 223 Python regressions, native signal test, format check, server build |
| 5 | NEV-06 and NEV-07 periodic rearm safety | Complete | ASan/UBSan multi-interval/retry harness, 224 Python regressions, native signal test, format check, server build |
| 6 | NEV-04, NEV-11, NEV-12, and NEV-22 absolute due-tick core, rescheduling, and harness foundation | Complete | ASan/UBSan three-phase boundary matrix and 1,200-tick oracle, 225 Python regressions, native signal test, format check, server build |
| 7 | NEV-09, NEV-13, NEV-15, and NEV-22 priority, aging, catch-up, and range safety | Complete | ASan/UBSan priority-on/off, mixed-deferral, continuous-arrival, convergence, unlimited, and invalid-range modes; 225 Python regressions; native signal test; format check; server build |
| 8 | NEV-19 scheduling results, chronological lookup, and handle API completion | Complete | Typed rejection/success/replace and global/owner chronology in the ASan/UBSan scheduler harness; 225 Python regressions; native signal test; format check; server build |
| 9 | NEV-14, NEV-16 through NEV-18, and NEV-20 load control, observability, durability, and thread boundary | In progress | Core hardening and NEV-17 registry complete: ASan/UBSan owner-link, corruption, thread-boundary, unique-key, cadence, retry, missed-run, conditional-enable, and watchdog cases; 225 Python regressions; native signal test; format check; server build |
| 10 | NEV-21 documentation and legacy cleanup | Pending | Not started |

Checkpoint 1 replaced the fixed 6,000-entry array and sentinel scan with a
dynamically sized, validated address-to-name registry. Duplicate addresses are
coalesced, malformed records and address overflow are reported, failed reloads
preserve the last valid registry, lookup is bounded, and the profiling table now
sizes itself from the loaded registry. The executable regression covers 0,
5,999, 6,000, 6,001, and 6,220 records, unknown lookup, duplicates, malformed
input, overflow, reload failure, and cleanup under ASan/UBSan.

Checkpoint 2 made event destruction scheduler-owned and removed the public raw
teardown primitive. Cancellation now uses sequence-validated handles, reports a
typed result, is idempotent for repeated or stale handles, reconciles deferral
debt, releases the pool record, decrements the pending counter, and preserves
current-iteration safety by deferring reclamation until the callback pass is
safe. Character/object disarm paths and misfire cancellation use this API.
Broken victim links now null the victim immediately and cancel the event instead
of retaining a stale target until its original due time. The executable harness
checks head, middle, tail, current, next, future, deferred, repeated, stale,
character, object, and victim-link cancellation while reconciling wheel, pool,
counter, and debt totals after every case.

Checkpoint 3 added a typed payload path that copy/move-constructs event state and
destroys it through scheduler-owned type erasure; the raw overload now rejects
non-trivially-copyable pointee types at compile time. Every mob-hunt producer and
rearm path uses the typed API, so its cached Dijkstra vector has independent
ownership and a real destructor. Character hunts now store a monotonic,
process-local character identity rather than a raw target pointer, and resolve it
through the live character list, preventing allocator-address reuse from
retargeting an old hunt. A current-excluding lookup makes unable-to-act, wake,
stand, and alert callbacks rearm without mistaking the executing event for a
successor. The regressions exercise payload copy/move/destruction, cancellation
cleanup, stable-ID reuse, raw-type compile rejection, and current-event exclusion
under ASan/UBSan, and audit every hunt call site for the typed path.

Checkpoint 4 replaced each delayed volley's raw ship pointers with a
process-local registry slot and monotonic reuse generation. Successful ship
construction registers the identity, deletion invalidates it before any ship
storage is released, and the impact callback resolves both endpoints before its
first dereference. A missing attacker, missing target, or reused registry slot
now discards the volley. The sanitizer regression copies a scheduled payload,
advances its clock after deleting each endpoint independently, verifies live
delivery, and forces slot reuse to prove that a stale generation cannot resolve
to the replacement ship.

Checkpoint 5 added a scope-bound, fixed-delay rearm guard so a periodic callback
creates exactly one successor when it leaves through any normal return path.
Dirty-player checkpoints now recur independently of Redis, while only the Redis
floor-drop flush remains conditional. All three artifact maintenance callbacks
use the guard; artifact-war and binding query/result failures select a bounded
30-second retry, while empty results retain the normal interval. The sanitizer
harness advances through repeated Redis-off and Redis-on checkpoints and through
artifact query failure, result failure, empty result, and recovery without
losing the job. Unique periodic keys and richer health state remain part of the
NEV-17 registry work in checkpoint 9.

Checkpoint 6 made the monotonic `due_tick` authoritative and removed the
relative revolution counter. The scheduler derives physical buckets,
eligibility, remaining time, and lateness from the same absolute clock; a stable
sequence cutoff prevents callback-created records from joining the active pass.
Tick `N` now remains unchanged for the whole heartbeat: `ne_events` closes its
pre-pass window, and `nevent_advance_tick` advances the absolute tick and bucket
together at the end. Delay zero can run on `N` only when scheduled before the
pass and is otherwise staged for `N+1`. Scheduler-owned `reschedule_at`,
`reschedule_after`, and `advance_by` operations also replaced the offline-affect,
moonstone, and short-affect timer/link mutations, completing NEV-12 early. The
new deterministic harness uses a fake scheduler/profiling clock and the real
insertion, execution, destruction, and rescheduling paths. Under ASan/UBSan it
covers delays 0, 1, 299, 300, 301, 599, 600, and 601 in all three phases;
empty/head/middle/tail and callback-created current-bucket records; multiple
revolutions in one bucket; link/counter/pool balance; and a 1,200-tick randomized
comparison against an absolute-due priority-queue oracle. The complete 225-test
Python regression suite and native signal-handler test also pass with the new
timing model.

Checkpoint 7 made the stored priority authoritative and replaced incidental
list placement plus ad hoc promotion with one deterministic ordering tuple:
`(due_tick, effective_priority, sequence)`. Player priority now affects only
equal deadlines, the environment switch genuinely disables it, ward regeneration
is included, and cast/memorize/balance classification requires a PC. Normal work
ages above player work after two deferrals or late ticks; older deadlines always
precede new arrivals. Every deferral reinserts through the same comparator.
Catch-up debt now records an estimated callback cost and due-tick multiset, so
cancellation, execution, and rescheduling reconcile count, cost, and oldest due
together. Recovery quotas use both count and cost, preserve zero as an explicit
unlimited sentinel, warn once when both limits are disabled, and expose lateness
distribution buckets. Configuration parsing now checks `errno`, enforces fixed
operational ceilings, and uses saturating additions. The sanitizer
harness runs separate processes for priority on/off, bounded aging under player
load, steady arrivals equal to base capacity, four-pulse debt convergence,
unlimited settings, and invalid-range fallback. The server build, native signal
test, focused contracts, and all 225 Python regressions pass.

Checkpoint 8 made scheduling outcomes explicit. `add_event` and the typed owned
payload path now return a status plus a sequence-validated handle for every
accepted event, and distinguish null callbacks, negative delays, dead owners,
invalid victim relationships, malformed payloads, and exhausted sequence space.
The new replace operation schedules a validated successor before canceling its
predecessor, so a rejected request leaves the existing event armed and an
accepted request cannot expose a gap. `CharWait` uses that operation and only
publishes its command gate/deadline after success; commune delay extension uses
the scheduler reschedule API. Global, character, and object lookup now choose the
first event in scheduler order instead of bucket or owner insertion order, with
handle-returning and current-excluding variants. The sanitizer harness exercises
every result status, successful and rejected replacement, chronological global
and owner lookup across buckets/revolutions, and current-event exclusion.

The first checkpoint 9 slice completed the scheduler's core hardening. Callback
analytics now grow with the observed callback set, aggregate allocation
failures, emit only window summaries, include diagnostic logging in measured
wall time, and cache player-tracing configuration. Character and object owner
lists retain their insertion order through explicit tails and reciprocal links,
making both append and known-record cancellation constant-time. Integrity
inspection is observation-only and checks wheel uniqueness, reciprocal links
and tails, owner and victim-link membership, stable character identities,
object/victim liveness, payload state, deferred metadata, and wheel/pool/counter
agreement. Admin output reports authoritative timing,
priority, deferral, sequence, stable identity, liveness, and the last fully
measured scheduler cost without dereferencing unvalidated owners. Scheduler API
boundaries bind and enforce the game thread. The sanitizer harness deliberately
corrupts character, object, and victim links, proves inspection detects but does
not repair them, exercises constant-time middle removal, and verifies worker
thread add, cancel, reschedule, and lookup attempts assert before mutation.

The second checkpoint 9 slice superseded callback-owned rearm guards with a
keyed registry for all eleven operational global jobs: game and astral clocks,
the sliced character sweep, three artifact tasks, outpost upkeep, surname
updates, dirty-player checkpoints, donation polling, and world-state saves. The
registry owns the sole successor while preserving each original callback for
name and profiling attribution. It supports fixed-delay and fixed-rate cadence,
explicit retry or next-delay overrides, conditional enablement, callback and
scheduling failure history, consecutive failures, last success, next deadline,
missed runs, duplicate suppression, and a watchdog that restores a missing
successor without creating a second live job. Registry metadata participates in
the non-mutating integrity pass and is exposed through `world events periodic`.
The sanitizer harness covers uniqueness, conflicting definitions, both cadence
policies, retries, recovery, missed intervals, conditional enablement, and
deliberate successor loss.

The executive assessment and finding evidence below preserve the original
pre-remediation review. Per-finding implementation status and the checkpoint
ledger above are authoritative for the current tree.

## Executive assessment

The `nevent` system is deeply integrated into the server and has several useful
controls: character/object ownership lists, victim links, a per-pulse execution
budget, catch-up accounting, player-event latency telemetry, and focused source
contract tests. Those controls do not yet make the scheduler safe or predictable
end to end.

The review found three critical memory/lifetime defects and several high-impact
correctness failures. The most urgent conclusions are:

1. The checked-in event-name file has 6,220 rows, but the loader has 6,000 slots
   and writes a terminator one element past the array when it reaches capacity.
   The subsequent lookup is unbounded. This is a deterministic out-of-bounds
   write and read path in the current tree.
2. Event payloads are allocated as bytes, copied with `memcpy`, and freed without
   constructors or destructors. `hunt_data` contains `std::vector<int>` and is
   repeatedly sent through this interface. That violates C++ object lifetime
   rules and can produce aliasing, dangling storage, and leaks.
3. Delayed ship volleys store raw `P_ship` pointers in an unowned global event.
   Ships can be freed without canceling those events. A volley that lands after
   either ship is deleted can dereference freed or reused memory.
4. The timing wheel is phase-sensitive. Events scheduled after the current
   bucket has been processed and delayed by exactly a multiple of 300 pulses can
   fire a complete 300-pulse revolution late. At four pulses per second, that is
   75 seconds of additional delay. Several recurring global jobs use exact
   multiples.
5. One production caller invokes the scheduler's teardown helper directly.
   That unlinks the event but does not return its pool record or decrement the
   pending count, creating a permanent slot leak and corrupting accounting.
6. Recurring jobs are self-rearmed inside callbacks. Several early-return paths
   therefore silently stop important maintenance until the next reboot. The
   periodic dirty-player checkpoint also stops after one run whenever Redis is
   disabled, contrary to its boot-time contract.
7. The documented priority/fairness behavior is not implemented as described.
   The priority environment switch is ignored by queue insertion, the overdue
   promotion predicate accepts the first event unconditionally, and deferral can
   place normal work ahead of player events.
8. Existing tests all passed, but the nevent tests are predominantly source-text
   contract checks. They do not execute the timing wheel or cover lifetime,
   cancellation, exact-boundary timing, or overload ordering.

This review recommends an immediate safety pass before further scheduler tuning,
followed by replacing relative wheel arithmetic with an absolute due-tick model,
centralizing cancellation/rescheduling, and introducing a first-class periodic
job registry.

## Severity and confidence

- Critical: Memory corruption, use-after-free, or invalid C++ object lifetime on
  reachable paths.
- High: Silent loss of required work, permanent scheduler resource leakage,
  material timing failure, or starvation/order failure under expected use.
- Medium: Operational fragility, misleading telemetry, scalability risk, or an
  API that makes high-impact mistakes likely.
- Low: Documentation debt, dead interfaces, or bounded inefficiency.

"Confirmed" means the failure follows directly from current source and current
repository data. "Reachable risk" means the unsafe state has a clear source path
but was not reproduced against a live game in this review.

| ID | Severity | Confidence | Finding |
|---|---|---|---|
| NEV-01 | Critical | Confirmed | Event-name table overflows at the current 6,220-row file |
| NEV-02 | Critical | Confirmed | Raw payload ABI is invalid for `hunt_data` and other non-trivial C++ types |
| NEV-03 | Critical | Reachable risk | Ship volley events retain unowned pointers to freeable ships |
| NEV-04 | High | Confirmed | Exact-multiple delays are phase-sensitive and can be 300 pulses late |
| NEV-05 | High | Confirmed | Direct `clear_nevent` use leaks an event pool record and counter entry |
| NEV-06 | High | Confirmed | Dirty-player checkpoints stop after one run when Redis is disabled |
| NEV-07 | High | Confirmed | Artifact maintenance permanently stops on empty/transient DB results |
| NEV-08 | High | Confirmed | Mob-hunt retry sees itself and contains four `&data` payload mistakes |
| NEV-09 | High | Confirmed | Priority disable, overdue promotion, and deferred ordering contracts fail |
| NEV-10 | High | Reachable risk | Broken victim links retain a dangling `victim` until the original due time |
| NEV-11 | Medium | Confirmed | `pulse` and `ne_event_tick` represent different phases of the same heartbeat |
| NEV-12 | Medium | Confirmed | Callers mutate wheel links/timers outside scheduler APIs |
| NEV-13 | Medium | Confirmed | Catch-up accounting does not guarantee debt convergence and mishandles zero budget |
| NEV-14 | Medium | Confirmed | Heavy synchronous callbacks remain unbounded and unpreemptible |
| NEV-15 | Medium | Confirmed | Configuration and duration arithmetic have unchecked overflow cases |
| NEV-16 | Medium | Confirmed | Name lookup, analytics, and intrusive-list operations have avoidable hot-path costs |
| NEV-17 | Medium | Design gap | Queue durability and periodic-job uniqueness are implicit and incomplete |
| NEV-18 | Medium | Confirmed | Diagnostics can mutate state, misreport counts, or dereference stale targets |
| NEV-19 | Medium | Confirmed | Scheduling returns no status/handle and global lookup is not chronological |
| NEV-20 | Medium | Design gap | Main-thread-only operation is assumed but not asserted |
| NEV-21 | Low | Confirmed | Reference documentation and legacy event interfaces are stale |
| NEV-22 | High | Confirmed gap | No deterministic behavioral scheduler test harness exists |

## Scope and method

The review traced:

- `struct nevent_data`, `add_event`, wheel insertion, `ne_events`, deferral,
  catch-up, cancellation, lookup, diagnostics, and event-name loading;
- the main-loop phase around `ne_events`, `pulse`, and `after_events_call`;
- boot scheduling in `ne_init_events` and recurring callback rearming;
- character, victim, object, ship, and payload lifetime paths;
- representative gameplay and maintenance consumers, including regeneration,
  command gates, zone resets, weather, artifact jobs, player persistence,
  mob hunting, object decay, and ship combat;
- documentation in `docs/reference/EVENTS.md` and focused tests under
  `tests/async/`;
- the built executable's exported text symbols and the checked-in
  `lib/misc/event_names` data.

The review used static source tracing, targeted repository searches, source
contract tests, an incremental server build, and deterministic arithmetic/data
probes. It did not start a live server, query the game database, run migrations,
or exercise production data. Findings labeled as reachable risks should be
covered by a sanitizer-backed behavioral harness before deployment, but they do
not require production reproduction to justify correction.

## Current architecture

### Scheduling model

`ne_schedule` is a 300-bucket timing wheel. `pulse` advances every 250 ms, so one
wheel revolution is 75 seconds. An event stores:

- its callback and scheduler-owned payload (raw copies are restricted to
  trivially copyable types, with a typed ownership path for C++ objects);
- optional `ch`, `victim`, and `obj` owners/targets;
- an absolute `due_tick` and physical bucket (`element`);
- priority and deferral metadata;
- a stable insertion sequence and lifecycle state;
- intrusive links for the wheel, character list, object list, and victim link.

`add_event` calculates:

```text
first eligible tick = ne_event_tick + (event pass already started ? 1 : 0)
due tick            = max(ne_event_tick + delay, first eligible tick)
bucket              = due tick % 300
```

The current bucket is scanned once per heartbeat. A stable sequence cutoff is
captured when the pass begins, and a callback runs only when its record belonged
to that snapshot and `due_tick <= ne_event_tick`. Future revolutions can share a
bucket without being mutated during earlier scans. The event remains linked and
visible through lookup APIs while its callback executes. Scheduler-owned
destruction then unlinks owners and wheel state, destroys the payload, releases
the memory-manager pool record, and decrements the global pending count.

### Heartbeat phase

`ne_event_tick` is sampled once for the whole heartbeat, and `pulse` is its
modulo-300 bucket. Calling `ne_events` changes the phase from pre-pass to
during/after-pass without advancing time. Substantial activity and combat work
therefore observe the same tick as the event pass. Near the end of the heartbeat,
`nevent_advance_tick` increments the absolute tick, derives the next bucket, and
reopens the pre-pass phase as one operation.

### Ownership and cancellation

Character-owned events are appended to `ch->nevents`; object-owned events are
prepended to `obj->nevents`. A distinct victim can be protected with a
`char_link_data` link. Cancellation uses sequence-validated handles and defers
reclamation only while an event pass is active. Hunt targets and delayed ship
volleys use stable process-local identities rather than unowned embedded target
pointers.

### Boot and persistence

`ne_init_events` creates the pool, clears the wheel, resets scheduler clocks,
schedules room/zone work, and creates global recurring jobs. The wheel itself is
not persisted. Boot and object/character reconstruction recreate selected event
classes through bespoke paths. This gives three de facto durability classes -
ephemeral, reconstructible, and operationally durable - but the API does not
declare or enforce them.

### Overload behavior

The default pulse budget is 25 ms and 4,000 executed callbacks. When exhausted,
the scheduler attempts one promotion, moves the remaining suffix to the next
bucket, records deferral debt, and permits a limited catch-up extension on later
pulses. The design protects the main loop from a callback burst, but a callback
already in progress cannot be preempted.

## Detailed findings

### NEV-01: Event-name table overflows in the reviewed tree

Implementation status (2026-08-27): Fixed and verified in checkpoint 1. The
historical evidence below describes the pre-fix implementation.

Evidence:

- `MAX_FUNCTIONS` is 6,000 and `function_names` has exactly that many elements
  (`src/new_events.c:39`, `src/new_events.c:100-104`).
- `load_event_names` reads while `i < MAX_FUNCTIONS`, then unconditionally writes
  `function_names[i].func = 0` (`src/new_events.c:1748-1771`).
- When the input has at least 6,000 rows, `i == 6000` and the terminator write is
  one element past the valid array.
- `get_function_name` then scans until it finds a null function without applying
  an array bound (`src/new_events.c:1723-1736`). With a full array, a miss reads
  beyond it.
- The checked-in `lib/misc/event_names` contains 6,220 rows. The freshly built
  `bin/server/dms_new` also exposed 6,220 text symbols under the generation
  command described in the source. This is not only a future capacity concern.
- The `fscanf` format uses unbounded `%s` for a 256-byte local name buffer
  (`src/new_events.c:1752-1765`). Current names fit, but future input can overflow
  it independently.

Impact:

- Definite boot-time memory corruption after a saturated load.
- Unbounded out-of-bounds reads on unknown callback-name lookup.
- Callback labels and analytics are not trustworthy after the corruption.

Recommendation:

- Replace the fixed table with a dynamically sized address-to-name map populated
  with validated records.
- If an interim fixed table is retained, allocate a dedicated sentinel slot,
  stop with an explicit truncation error, bound every lookup, and use `%255s`.
- Add loader tests for 0, 5,999, 6,000, 6,001, and 6,220 records plus unknown
  lookup. Run them under ASan/UBSan.

### NEV-02: Byte-copy payload ABI violates C++ object lifetime

Implementation status (2026-08-27): Fixed and verified in checkpoint 3. Raw
typed calls are constrained to trivially copyable payloads, while non-trivial
payloads use scheduler-owned copy/move construction and typed destruction.
`hunt_data` and all of its producers/rearm paths use the owned API, and character
targets use process-local runtime identities. The evidence below describes the
pre-fix implementation.

Evidence:

- `add_event` allocates `data_size` bytes and uses `memcpy` to copy arbitrary
  payloads (`src/new_events.c:685-690`).
- `clear_nevent` frees those bytes directly and invokes no destructor
  (`src/new_events.c:326-329`).
- `hunt_data` contains `vector<int> path` (`src/structs.h:2018-2032`).
- `event_mob_hunt` invokes `size`, indexing, and Dijkstra operations on that
  copied vector and reschedules the structure repeatedly
  (`src/mobact.c:9623-9660` and subsequent rearm paths).

This is formally undefined behavior. No `hunt_data` object is constructed in
the event buffer, and no destructor runs. A raw copy of `std::vector` duplicates
its internal pointers rather than its owned elements. Depending on the source
and reschedule path, the event can hold dangling storage, share ownership without
coordination, or leak the final allocation.

The hunting target is also embedded as `targ.victim` while hunt events normally
pass a null top-level `victim`. It therefore bypasses `char_link_data` lifetime
tracking. The callback makes defensive character-list checks, but an address
reused for a different character is still an ABA identity error. Stable target
identity is required independently of the vector fix.

Recommendation:

- Make the generic payload contract explicitly trivially copyable and enforce it
  with a typed C++ wrapper and `static_assert`.
- Redesign hunting payloads as POD: stable target ID/room, flags, retry state,
  path step, and a path owned outside the raw scheduler payload; or give event
  payloads typed clone/move/destroy operations.
- Audit all `add_event(..., data, sizeof(type))` call sites for non-trivial types,
  internal pointers, and ownership-sensitive buffers.

### NEV-03: Delayed ship volleys can dereference deleted ships

Implementation status (2026-08-27): Fixed and verified in checkpoint 4.
`VolleyData` now stores slot-and-generation handles, ship deletion invalidates
the registry entry before freeing storage, and the callback safely returns when
either endpoint no longer resolves. The sanitizer harness covers both deletion
orders and same-slot generation reuse. The evidence below describes the pre-fix
implementation.

Evidence:

- `VolleyData` stores raw attacker and target ship pointers
  (`src/ships/ships.h:546-552`).
- `fire_weapon` schedules `volley_hit_event` with no character, victim, or object
  owner; the only ship references are inside copied data
  (`src/ships/ship_combat.c:1467-1473`).
- `volley_hit_event` immediately dereferences both pointers
  (`src/ships/ship_combat.c:547-570`).
- `delete_ship` clears known ship references and frees the ship, but does not find
  or cancel volley events (`src/ships/ship_base.c:673-705`).

If either ship is deleted while a volley is in flight, the callback can access
freed memory. Address reuse can turn this into an ABA error in which the callback
acts on an unrelated later allocation.

Recommendation:

- Put stable ship IDs plus a generation/version in the payload and resolve them
  at callback time; safely discard the volley when either endpoint is gone.
- Alternatively, add first-class ship ownership/cancellation links to nevents.
- Add a regression that schedules a volley, deletes attacker and target in
  separate cases, advances the clock, and verifies a safe no-op under ASan.
- Extend the payload audit to every raw embedded pointer whose pointee can be
  destroyed before the callback; top-level owner links do not protect these.

### NEV-04: Exact-multiple delays depend on scheduling phase

Implementation status (2026-08-27): Fixed and verified in checkpoint 6. Absolute
deadlines now control both bucket placement and execution, and the sanitizer
matrix covers every listed boundary in all heartbeat phases. The historical
evidence below describes the pre-fix wheel arithmetic.

The wheel arithmetic assumes the event will receive a decrement when its target
bucket is next encountered. That assumption is false when the target bucket is
the current bucket and it has already been scanned.

Example with an otherwise empty current bucket:

```text
current pulse p has already run ne_events
add delay 300
bucket = (p + 300) % 300 = p
timer = 300 / 300 + 1 = 2

after 300 pulses: bucket p is scanned, timer becomes 1
after 600 pulses: bucket p is scanned, timer becomes 0 and fires
```

The requested 300-pulse delay becomes 600 pulses. During a callback, behavior is
also queue-position dependent because `next_event` is captured before the
callback (`src/new_events.c:1210-1214`). A newly inserted current-bucket record
may be encountered later if an existing successor leads traversal to it, but it
is missed when the current record was the tail or priority insertion put it
behind an already captured path. Timing must not depend on neighboring records.

The special case at `src/new_events.c:663-666` changes only delay zero. It does
not correct 300, 600, 900, or other exact wheel multiples.

Representative affected recurring calls include:

- zone reset at 300 pulses (`src/events.c:678`);
- game-hour and astral-clock rearming (`src/weather.c:109`,
  `src/weather.c:254`);
- outpost repair (`src/outposts.c:949`);
- autosave at 1,200 pulses (`src/actoth.c:1747`);
- artifact wars at 7,200 pulses (`src/artifact.c:2900`).

`ne_event_time` derives a remaining delay from the same relative representation
(`src/new_events.c:734-745`) and cannot distinguish a current bucket that has
already passed from one that has not. It can therefore report the requested
delay while the actual firing is one revolution later.

Recommendation:

- Make a monotonic `uint64_t due_tick` the scheduling source of truth, not only a
  diagnostic field.
- Insert events into a bucket selected from the absolute due tick and execute a
  stable snapshot of events whose `due_tick <= now`; stage callback-created
  records for a defined next scheduling phase.
- If the wheel representation is retained, encode phase explicitly and derive
  both placement and remaining time through one tested helper.
- Add boundary tests for delays 0, 1, 299, 300, 301, 599, 600, and 601 when
  scheduled before, during, and after the event pass, including empty, head,
  middle, and tail insertion cases.

### NEV-05: External use of `clear_nevent` leaked records in the reviewed tree

Implementation status (2026-08-27): Fixed and verified in checkpoint 2. The raw
teardown function is no longer public or called directly; the historical
evidence below describes the pre-fix implementation.

`clear_nevent` is a teardown primitive. It unlinks owners and the wheel and frees
the payload, but it does not call `mm_release`, decrement `ne_event_counter`, or
reconcile catch-up debt. Those actions are performed separately by `ne_events`
after normal execution (`src/new_events.c:1301-1303`).

`event_remove_misfire_cooldown` calls `clear_nevent` directly and returns
(`src/mobact.c:8785-8801`). The record is no longer reachable by the scheduler,
so it can never reach the missing release/decrement path. The call is reachable
when zone or continent player thresholds change (`src/mobact.c:8810-8844`). Each
successful cancellation leaks a pool record and leaves pending-event accounting
inflated. If the event had deferral debt, that debt can also become phantom debt.

Recommendation:

- Make raw unlink/clear private.
- Expose one cancellation API that owns payload destruction, all links, pool
  release, counter updates, deferral-debt reconciliation, and current-iteration
  safety.
- Return a cancellation result and make repeated cancellation idempotent.
- Add invariant tests asserting wheel count, pool in-use count, global pending
  count, and catch-up debt after canceling current, next, future, deferred, and
  already-canceled records.

### NEV-06: Dirty-player checkpointing stops without Redis

Implementation status (2026-08-27): Fixed and verified in checkpoint 5. Local
checkpointing has an unconditional fixed-delay rearm, and the Redis setting now
guards only Redis floor-drop work. The regression advances three intervals with
Redis disabled and enabled. The evidence below describes the pre-fix
implementation.

Boot explicitly states that revisioned local checkpoints do not depend on Redis
and schedules `event_flush_dirty_players` unconditionally
(`src/new_events.c:1565-1566`). The callback performs the local checkpoint, then
rearms itself only inside `if (redis_enabled)` (`src/redis.c:1059-1066`).

When Redis is disabled, the server gets one checkpoint five seconds after boot
and no further periodic checkpoints. This contradicts the stated durability
contract and can materially widen data-loss exposure.

Recommendation:

- Rearm the checkpoint unconditionally.
- Guard only Redis-specific work with the Redis setting.
- Add tests for both Redis states that advance through at least three intervals
  and assert continued local checkpoint calls.

### NEV-07: Artifact maintenance silently loses recurrence

Implementation status (2026-08-27): Fixed and verified in checkpoint 5. A
scope-bound rearm guard covers every normal return from artifact poof, war, and
binding maintenance. Database failures use a bounded retry for the two long
jobs, while empty results preserve their normal cadence. The first-class keyed
registry and health telemetry remain tracked by NEV-17. The evidence below
describes the pre-fix implementation.

Recurring callbacks are responsible for scheduling their successor. Early
returns before that final call permanently remove the job from the system.

Confirmed examples:

- `event_artifact_wars_sql` returns when no result or no rows are present, before
  its 30-minute rearm (`src/artifact.c:2693-2702`, `src/artifact.c:2900`). Starting
  with no artifacts on PCs disables future checks even if artifacts later appear.
- `event_artifact_check_bind_sql` returns on query failure, result failure, or an
  empty table before its seven-minute rearm (`src/artifact.c:3816-3837`,
  `src/artifact.c:3960-3962`). A transient database outage or initially empty
  table disables the task until reboot.

Recommendation:

- Introduce scheduler-managed periodic registrations where rearming occurs
  outside the callback in guaranteed cleanup logic.
- Define fixed-delay versus fixed-rate behavior, unique job keys, transient
  retry/backoff, last-success time, consecutive failures, and missed-run policy.
- Until that exists, put rearming in a single cleanup path reached by all return
  branches and use a shorter bounded retry after DB failure.

### NEV-08: Mob-hunt retry logic is defeated by current-event visibility

Implementation status (2026-08-27): Fixed and verified in checkpoint 3. Hunt
fallbacks use a lookup that excludes the executing event, and all initial and
successor schedules go through the correctly typed mob-hunt helper. The four
historical stack-pointer payload mistakes are no longer present. The evidence
below describes the pre-fix implementation.

The executing event remains on the character list until its callback returns.
`get_scheduled(ch, event_mob_hunt)` therefore returns the currently executing
event.

In the unable-to-act branch, the code says the mob should try later, but it only
rearms when no hunt event is found (`src/mobact.c:9480-9493`). It finds itself,
does not rearm, returns, and is then cleared by the scheduler. Hunting stops. The
same check distorts the wake/stand/alert fallback branches.

There is a second independent bug in four fallback calls at
`src/mobact.c:9490-9525`: local `data` is already a `hunt_data *`, but those calls
pass `&data` with `sizeof(hunt_data)`. If reached after some action has removed the
current event, `add_event` copies a full `hunt_data` beginning at the address of a
stack pointer variable. That is an out-of-bounds stack read and creates a corrupt
payload. The normal calls later in the function correctly pass `data`.

Recommendation:

- Add an explicit lookup mode that excludes `current_nevent`, or give each owner
  a unique-key/replace operation whose semantics are valid during callbacks.
- Replace all four `&data` arguments with the correctly typed payload path while
  addressing NEV-02.
- Test unable-to-act, wake, stand, and alert branches across several pulses.

### NEV-09: Priority and fairness contracts are internally inconsistent

Implementation status (2026-08-27): Fixed and verified in checkpoint 7. The
wheel is ordered by due tick, authoritative stored/effective priority, and
sequence. Normal events age above equal-deadline player work after two
deferrals/late ticks, all deferrals use the same insertion path, ward regen is
classified, and the disabled-priority mode is FIFO by due tick and sequence.
The evidence below describes the pre-fix implementation.

This is a group of related overload-ordering failures.

#### The player-priority setting is ignored by insertion

`nevent_priority` reads `DURIS_NEVENT_PLAYER_PRIORITY` and stores the result in
`event->priority` (`src/new_events.c:196-203`, `src/new_events.c:676`). Queue
insertion never consults that field. It directly calls `nevent_is_player_timed`
(`src/new_events.c:206-244`). Setting the documented knob to zero therefore does
not disable player-first insertion.

#### Overdue promotion does not identify overdue work

`nevent_overdue_event` returns true for every non-null event and ignores priority,
deferral count, lateness, and class (`src/new_events.c:247-253`). The promotion
loop immediately accepts the first candidate, and if it is already `next_event`,
returns without moving anything (`src/new_events.c:267-272`). The mechanism grants
at most one additional scan/execution opportunity; it does not find or hoist a
repeatedly deferred normal event as its comments and static assertion claim.

#### Deferral can invert player-first ordering

`nevent_defer_suffix` moves the remaining run to the next pulse and prepends the
entire run before that bucket's existing events (`src/new_events.c:1163-1173`). A
normal deferred head can now sit before player events already queued there.
Later priority insertion only recognizes a contiguous player prefix, so it
cannot restore player-first ordering behind that normal head.

#### Classification is incomplete and inconsistent

Player hit, mana, and move regeneration are classified, but ward regeneration is
not (`src/new_events.c:185-193`). Some other callback classes are considered
priority whenever they have any character, not consistently only for PCs. This
makes the meaning of "player timed" hard to reason about.

#### Future records can block due records

Insertion orders primarily by callback class, not due state or revolution count.
A player event in the same bucket but a later revolution can be scanned before a
normal event due now, consuming scan time and potentially contributing to budget
exhaustion before due work.

Recommendation:

- Make `event->priority` authoritative for every insertion, deferral, and
  promotion path.
- Define an ordering tuple such as `(due_tick, effective_priority, sequence)`,
  with bounded aging based on actual lateness/deferrals.
- State a measurable fairness guarantee, for example a maximum lateness bound
  for normal work while retaining a tighter player deadline.
- Test priority enabled/disabled, mixed revolutions in one bucket, deferred
  suffixes, continuous player load, and debt convergence.

### NEV-10: Broken victim links retained stale target pointers

Implementation status (2026-08-27): Fixed and verified in checkpoint 2. Broken
links now clear the target and enter the centralized cancellation lifecycle,
with callback-time reclamation deferred only until iteration is safe.

When a character link breaks, `event_broken` clears the callback and link pointer
but leaves `event->victim` intact and does not expedite cleanup
(`src/new_events.c:1668-1685`). The event can remain in its original future bucket
for an arbitrary time with a stale pointer.

The callback will not execute because `func` is null, but diagnostics can still
dereference the victim. `show_world_events` prints `GET_NAME(ev->victim)` for a
matching callback name (`src/new_events.c:1802-1818`); querying neutered/unknown
records can therefore expose a use-after-free. Other future inspection paths
would inherit the same hazard.

Recommendation:

- Null `victim` immediately when its link breaks.
- Use the centralized cancellation API to reclaim the record promptly when safe,
  or move it to an explicit tombstone queue with no owner pointers.
- Never dereference diagnostic owner pointers without validating liveness.

Ordinary character/object disarming has a related bounded retention cost. It
sets `func = NULL` and `timer = 1` but leaves the event in its original bucket,
so its payload and pool slot can remain allocated for up to one full revolution
(75 seconds). This is iteration-safe, but cancellation bursts can create avoidable
pool pressure. A centralized API can reclaim non-current records immediately and
tombstone only records whose links are part of the active traversal. In contrast,
`event_broken` does not even set `timer = 1`, so its retention can last for the
original arbitrary delay.

## Correctness, API, and operational gaps

### NEV-11: Split clock phase makes due-time telemetry inaccurate

Implementation status (2026-08-27): Fixed and verified in checkpoint 6.
`ne_event_tick` is stable throughout a heartbeat, `pulse` is derived from it, and
the scheduler advances both at the end of the heartbeat. The historical evidence
below describes the pre-fix split phase.

`ne_event_tick` increments at the end of `ne_events`, while `pulse` increments
near the end of the heartbeat. Events created during activity/combat and other
post-event work use the old wheel bucket phase but the next diagnostic tick.
For common non-boundary delays, `scheduled_tick` is one tick later than the
actual wheel due time. Player lateness telemetry can underreport lateness or show
an event as early. Regeneration's elapsed-time floor masks a zero elapsed value
but does not unify the clocks.

Use one monotonic scheduler tick sampled once per heartbeat. Derive placement,
due comparisons, lateness, and remaining time from that value and a documented
phase.

### NEV-12: Callers bypass scheduling invariants

Implementation status (2026-08-27): Fixed and verified in checkpoint 6, ahead of
the original checkpoint 8 allocation. The three known direct-mutation paths now
use cancellation or scheduler-owned `reschedule_at`, `reschedule_after`, and
`advance_by` operations; no caller outside `new_events.c` writes wheel links,
buckets, or deadlines. The historical evidence below describes the old paths.

Two direct mutation patterns were confirmed:

- Login/offline-affect handling manually unlinks, recalculates, and relinks
  events (`src/nanny.c:2588-2703`). It does not update `scheduled_tick` or
  deferral metadata, bypasses priority-aware insertion, and leaves sequence
  semantics implicit, so diagnostics and overload behavior can diverge from the
  new timing.
- Moonstone dispel sets `e->timer = 1` without moving its bucket
  (`src/specs.object.c:14972-14990`). The comment says the object should decay in
  one minute, but the actual wait is time until the original bucket is visited,
  from immediate to one full wheel revolution, rather than the requested
  duration.

Add `reschedule_after`, `reschedule_at`, and `advance_by` APIs that atomically
maintain all wheel links and metadata. Prohibit direct writes to bucket, timer,
and schedule links outside the scheduler implementation.

### NEV-13: Catch-up behavior does not guarantee recovery

Implementation status (2026-08-27): Fixed and verified in checkpoint 7.
Deferred records retain the oldest due positions, debt tracks count, estimated
cost, and oldest due tick, and lifecycle completion reconciles all three.
Count/cost quotas add bounded net recovery capacity without converting an
unlimited base setting into a limit. A steady-arrival sanitizer test proves debt
converges inside the four-pulse window. The evidence below describes the pre-fix
implementation.

- Catch-up extension is a larger general budget, not a reservation for deferred
  work. New and priority work can consume it before debt-bearing records, so debt
  need not converge.
- `DURIS_NEVENT_BUDGET_USEC=0` means no time limit in the normal loop. If callback
  count creates debt, catch-up adds a positive extension to zero
  (`src/new_events.c:884-907`), unexpectedly turning an unlimited time setting
  into a small positive time cap on later pulses.
- Setting both base limits to zero makes the event loop entirely unbounded. That
  may be intentional for diagnostics, but it needs an explicit unsafe-setting
  warning and should not be reachable through an overflowed configuration value.
- Debt is a count of deferred records, not their age or cost. One slow callback
  and many cheap callbacks are not represented proportionally.
- A directly canceled deferred record can leave debt permanently inflated.

Use explicit unlimited sentinels, reserve at least part of recovery capacity for
oldest debt, and track lateness distribution and oldest due tick. Define and test
a convergence property under a bounded arrival rate.

### NEV-14: Heavy callbacks can monopolize the game thread

The pulse budget is checked between callbacks. It cannot interrupt a callback
that performs synchronous database work or large global scans. Representative
callbacks include:

- artifact poof processing: database queries, object/global scans, and offline
  player load/write work in one callback (`src/artifact.c:2197-2623`);
- artifact wars: synchronous query, linked grouping with nested searches, and
  per-artifact updates (`src/artifact.c:2669-2900`);
- artifact binding: query, per-row object/player resolution, and updates
  (`src/artifact.c:3803-3962`);
- surname update: all descriptors plus leaderboard/ship-frag work; the latter
  scans ships repeatedly (`src/drannak.c:232-247`, `src/hardcore.c:291-310`,
  `src/ships/ship_base.c:2605-2632`);
- dirty-player checkpointing: all online PCs in one callback
  (`src/redis.c:1034-1039`).

Move database fetches to the existing worker/completion pattern where possible,
apply results on the game thread in bounded slices, and give bulk callbacks a
continuation cursor. Track wall time including preparation and diagnostic costs.

### NEV-15: Configuration and duration arithmetic are not range-safe

Implementation status (2026-08-27): Fixed and verified across checkpoints 6
and 7. Due arithmetic is saturating unsigned 64-bit, remaining-time/lateness
conversions clamp to their public types, configuration parsing checks `ERANGE`
and operational ceilings, and catch-up budget additions saturate. The runtime
harness verifies an out-of-range value falls back safely. The evidence below
describes the pre-fix implementation.

`nevent_config_limit` uses `strtol` but does not inspect `errno` or enforce an
upper bound (`src/new_events.c:801-815`). Extreme accepted values can overflow
budget additions or `extra_callbacks * average_callback_us`. Separately,
`delay + pulse` uses signed `int` arithmetic and can overflow before modulo, while
`ne_event_time` returns `int` after unsigned timer arithmetic.

Use checked `uint64_t` duration/due arithmetic, reject values outside documented
operational bounds, check `errno`, and use saturating or explicitly checked
budget math.

### NEV-16: Several hot paths scale unnecessarily

Implementation status (2026-08-27): Fixed and verified in the first checkpoint
9 slice. Name lookup was already made hash-based in checkpoint 1. Analytics is
dynamically attributed with explicit allocation-failure accounting and no
per-pulse log, its full logging cost is sampled, player tracing is cached, and
owner/schedule unlinking plus owner insertion use reciprocal links without
predecessor or tail scans. The evidence below describes the pre-fix
implementation.

- `get_function_name` is a linear scan of up to 6,000 entries. Callback label
  lookup occurs before every executed callback (`src/new_events.c:1254-1256`).
  For names outside any small fast path, bursts can perform millions of pointer
  comparisons. This work falls inside total loop time but outside individual
  callback timing, hiding its source in `slowest_us`.
- Analytics has 128 callback slots (`src/new_events.c:50`,
  `src/new_events.c:943-972`) for roughly 200 callback types observed at call
  sites. Broad windows can lose attribution to overflow.
- With analytics enabled, a synchronous status line is written every pulse and
  per-callback lines are emitted at each 300-pulse window. `loop_us` is sampled
  before those logs (`src/new_events.c:1328-1357`), so `NEVENT SLOW` and the
  scheduler's own budget metrics exclude their cost even though the enclosing
  main-loop timing includes it. High log volume can therefore create latency
  that nevent telemetry attributes elsewhere.
- Player tracing reparses its environment variable for every classified event
  (`src/new_events.c:924-927`) instead of caching it.
- Character event insertion walks to the tail for every add
  (`src/new_events.c:697-714`), making event waves quadratic per owner.
- `clear_nevent` has `prev_sched` but still scans from the bucket head to find the
  predecessor (`src/new_events.c:419-458`). Clearing many events behind future
  records can become quadratic.

Use an address hash map/cache for callback names, cache configuration, size
analytics dynamically or aggregate overflow explicitly, keep an owner tail, and
unlink directly through validated intrusive links.

### NEV-17: Durability and recurring uniqueness are implicit

Implementation status (2026-08-28): Fixed and verified in the second checkpoint
9 slice. Operational recurring work is registry-owned, uniquely keyed,
watchdog-rearmed, and health-reporting. Ephemeral combat, animation, and
owner-bound timers remain intentionally process-local; zone, weather, and
loaded-owner timers are reconstructible from authoritative boot/player/world
state. Persistence-critical work already uses journaled save/recovery pipelines
rather than relying on a one-shot nevent deadline, so the audit found no
remaining genuinely durable one-shot that should persist scheduler internals.
The evidence below describes the pre-fix implementation.

The queue is memory-only. Crash/copyover loses remaining delay and ordering;
selected events are recreated ad hoc at boot or from loaded state. This is fine
for ephemeral animation/combat events but not automatically fine for persistence
checkpoints, economic upkeep, artifact state, or other operational maintenance.

Recurring global jobs also have no registry, unique key, or health watchdog. A
callback can accidentally duplicate itself or silently disappear, and there is
no authoritative list of expected jobs, last success, next due time, or missed
runs.

Classify each event type as ephemeral, reconstructible, or durable. Persist
stable IDs and deadlines for genuinely durable one-shot work, or move operational
periodic jobs into a registry that enforces uniqueness and exposes health.

### NEV-18: Integrity checks and admin diagnostics are unsafe/incomplete

Implementation status (2026-08-27): Fixed and verified in the first checkpoint
9 slice. Full validation is non-mutating and reconciles wheel, pool, counters,
owners, links, payloads, and deferred metadata. Admin summaries use those same
totals, while detailed rows use captured identities and explicit liveness checks
before rendering owner state. The evidence below describes the pre-fix
implementation.

- `clear_nevent(NULL)` logs but continues and dereferences the null pointer
  (`src/new_events.c:315-320`). It should return immediately.
- `check_nevents` attempts repair by severing character-list heads/links. It does
  not validate all forward/back links, tails, object/victim lists, wheel count,
  pool count, timer state, or the global counter. A diagnostic can therefore
  orphan records and make a corruption harder to analyze.
- `world events` with no argument counts the wheel directly, while other status
  paths report `ne_event_counter`; there is no continuous reconciliation metric.
- Detailed output omits true remaining time, absolute due tick, lateness,
  priority, deferrals, and sequence, and can dereference stale owner pointers.

Make validation non-mutating by default, add an explicit separately authorized
repair mode, continuously compare wheel/pool/counter totals, and make admin
rendering use stable IDs and liveness validation.

### NEV-19: Scheduling and lookup APIs hide failure and ambiguity

Implementation status (2026-08-27): Fixed and verified in checkpoint 8.
Scheduling returns a typed status and sequence-validated handle, replacement is
atomic with respect to rejected successors, and lookup chooses the chronological
scheduler-order match globally or per owner. Current-excluding lookup remains an
explicit operation. The evidence below describes the pre-fix implementation.

`add_event` returns `void`. It can reject a null callback, negative delay, dead
owner, or invalid victim relationship without giving the caller a handle or
machine-readable failure. Callers cannot reliably roll back state that assumed a
timer was armed. Some paths use bespoke post-schedule lookups as a workaround.

The global `get_scheduled(func)` walks bucket indices 0 through 299, not due-time
order, and its own comment admits the returned event may not be the next to run
(`src/new_events.c:1363-1382`). Owner lookup also includes the currently executing
record, which caused NEV-08.

Return a typed event handle/result with an error enum. Provide explicit
operations for find-next-by-due, find-other-than-current, unique replace,
reschedule, and cancel.

### NEV-20: Main-thread ownership is assumed, not enforced

Implementation status (2026-08-27): Fixed and verified in the first checkpoint
9 slice. Event-pool initialization binds the game thread, and scheduler
schedule, cancel, reschedule, execution, lookup, diagnostic, and compatibility
boundaries reject off-thread access (with corruption assertions in debug
builds). The sanitizer harness verifies representative worker calls leave the
wheel unchanged. The evidence below describes the pre-fix implementation.

Global wheel state, intrusive links, counters, and memory pools have no locking.
The current main-loop call pattern makes this workable only if every add,
reschedule, cancel, lookup, and execution occurs on the game thread. That rule is
not asserted at API boundaries and will become dangerous as database/persistence
workers expand.

Record the game-thread identity at boot, assert it in mutating scheduler APIs in
debug builds, and route worker completions through a bounded game-thread queue.

### NEV-21: Documentation and legacy surfaces no longer match reality

`docs/reference/EVENTS.md` currently overstates or misstates several properties:

- removal is described as O(1), but `clear_nevent` scans the bucket;
- victim linkage is described as preventing dangling targets, but
  `event_broken` retains `victim`;
- the player-priority environment switch is documented, but insertion ignores
  the computed field;
- promotion is described as hoisting overdue normal work, but its predicate
  accepts the first event;
- catch-up is described too strongly as self-correcting despite no recovery
  reservation or convergence bound;
- 300 pulses are labeled as a "minute" in analytics and comments, although at
  four pulses per second they represent 75 seconds.

`src/events.h` and the top of `src/events.c` also retain legacy event types,
macros, counters/name arrays, and old wheel descriptions alongside the nevent
implementation. `EventsFactory` in `src/structs.h` is an unused stub. Keeping two
conceptual APIs obscures which invariants are live.

Update the reference after correctness changes, clearly separate historical
material, and remove or quarantine unused legacy interfaces.

### NEV-22: Tests assert source strings, not scheduler behavior

Implementation status (2026-08-27): In progress. Checkpoints 6 and 7 added the
ASan/UBSan deterministic runtime harness and completed the phase/boundary,
current-bucket, shared-revolution, reschedule/accounting, randomized
absolute-due oracle, priority-on/off, mixed-deferral, continuous-arrival
fairness, and catch-up convergence portions below. Existing executable
regressions cover the listed cancellation, ownership, payload, lifetime,
name-loading, and periodic rearm cases. Periodic uniqueness remains part of
checkpoint 9.

The focused nevent tests passed, but inspection shows they mainly read source
files and assert that particular identifiers or snippets exist. These tests can
remain green while:

- the priority setting has no effect;
- promotion is a no-op;
- a 300-pulse delay fires after 600 pulses;
- direct cancellation leaks a pool record;
- a C++ vector payload has invalid lifetime;
- a recurring job exits without rearming;
- a stale owner pointer is dereferenced.

Create a deterministic scheduler harness with a fake monotonic tick/clock and
small callbacks. It should run the actual insertion, execution, cancellation,
deferral, and catch-up code without booting the full game.

Minimum behavioral matrix:

1. Delays 0, 1, 299, 300, 301, 599, 600, and 601 in every heartbeat phase.
2. Current-bucket insertion with empty, head, middle, tail, and callback-created
   records.
3. Multiple revolution counts sharing one bucket.
4. Player priority enabled and disabled.
5. Deferral into a bucket containing player and normal work.
6. Continuous arrivals plus a proof that bounded old work reaches execution.
7. Cancellation of current, next, future, deferred, and already canceled events.
8. Counter, pool, ownership-list, and catch-up-debt balance after every case.
9. Character, victim, object, and ship destruction before due time.
10. Typed payload copy/move/destruction and rejection of non-trivial raw payloads.
11. Self-rearming/current-exclusion and unique periodic jobs.
12. Periodic success, empty result, transient failure, and retry/backoff.
13. Event-name loads at and beyond configured capacity plus unknown lookup.
14. ASan, UBSan, and a long randomized model test comparing execution with an
    absolute-due priority queue oracle.

## What is working and worth preserving

The scheduler has several sound foundations that should survive a redesign:

- The event loop is single-threaded in current operation, which allows a simpler
  ownership model than a concurrently mutated queue.
- Character and object ownership lists provide useful bulk-cancellation and
  inspection hooks when all references are expressed through them.
- Victim links recognize that callback target lifetime differs from owner
  lifetime; the implementation needs completion, not removal of the concept.
- Budgeted suffix deferral prevents a large due bucket from monopolizing an
  entire heartbeat.
- Regeneration callbacks use elapsed scheduler ticks to compensate for late
  execution, improving player state correctness under overload.
- Command-gate recovery and player latency diagnostics show good awareness of
  the gameplay consequences of delayed callbacks.
- Catch-up debt, callback EWMA, per-callback analytics, and sequence/due metadata
  are useful observability primitives once their semantics are corrected.
- Boot-time staggering of room and zone work avoids an obvious synchronized
  callback spike.

## Recommended remediation plan

### Phase 0: Immediate memory and durability safety

1. Replace or safely bound the event-name table and lookup. Add capacity tests.
2. Prohibit non-trivially-copyable raw payloads; redesign `hunt_data` event state.
3. Replace volley ship pointers with stable IDs/generations and safe lookup.
4. Centralize cancel/destroy and convert the misfire cancellation caller.
5. Null broken victim pointers and safely reclaim tombstoned events.
6. Unconditionally rearm local dirty-player checkpoints.
7. Ensure artifact periodic jobs rearm on empty and failure paths.
8. Fix the mob-hunt self-lookup semantics and all four `&data` arguments.

Exit criteria:

- ASan/UBSan behavioral tests pass for event-name saturation, hunt payloads,
  ship deletion, victim deletion, and cancellation.
- Wheel count, pool in-use count, global counter, and debt reconcile after every
  tested operation.
- Required periodic jobs remain present across empty, disabled-dependency, and
  transient-failure cases.

### Phase 1: Scheduler correctness and API boundary

1. Adopt absolute due ticks and one coherent heartbeat phase.
2. Define callback-created event eligibility explicitly through staging/snapshot
   semantics.
3. Make priority data authoritative and implement measurable bounded aging.
4. Add typed handles/results plus cancel, replace, and reschedule APIs.
5. Remove direct schedule-link/timer mutation from callers.
6. Make current-event inclusion an explicit lookup choice.

Exit criteria:

- The complete boundary matrix fires at the requested tick in every phase.
- Priority-off produces FIFO/due ordering; priority-on meets defined player and
  normal lateness bounds.
- Remaining-time and lateness diagnostics match actual execution.

### Phase 2: Periodic work, load control, and observability

1. Add a unique periodic-job registry with fixed-rate/fixed-delay policy,
   retry/backoff, last success, next due, missed runs, and health status.
2. Slice or offload heavyweight DB/global-scan callbacks.
3. Reserve recovery capacity for debt and define a convergence invariant.
4. Harden all environment parsing and arithmetic bounds.
5. Replace linear callback-name lookup and right-size analytics.
6. Add game-thread assertions and a worker-completion boundary.

Exit criteria:

- Load tests show bounded heartbeat impact and debt recovery under a documented
  sustainable arrival rate.
- No single maintenance callback performs an unbounded database/global workload
  on the game thread.
- Operators can identify every expected recurring job and its health.

### Phase 3: Durability, cleanup, and documentation

1. Classify all event types by durability and define crash/copyover behavior.
2. Persist deadlines/stable references only where business behavior requires it.
3. Replace mutating diagnostics with invariant checks and explicit repair tools.
4. Remove/quarantine legacy event APIs and unused stubs.
5. Rewrite `docs/reference/EVENTS.md` against tested semantics.
6. Retain source-contract tests only as supplements to behavioral tests.

## Proposed core invariants

These invariants should be executable assertions in the harness and, where
cheap, debug/runtime checks:

1. Every live event is in exactly one wheel bucket.
2. Every event with a character/object owner is in exactly that owner's list.
3. Every wheel forward/back link and bucket head/tail pair is reciprocal.
4. Wheel live count equals pool in-use count equals `ne_event_counter`.
5. An event is destroyed exactly once; cancellation is idempotent.
6. Payload destruction matches payload construction exactly once.
7. A callback never receives a dead owner/target unless its contract explicitly
   uses a stable ID and handles absence.
8. An event never fires before `due_tick`; absent overload, it fires exactly at
   `due_tick` regardless of scheduling phase or neighboring records.
9. Deferral changes lateness, not the original due tick.
10. Effective ordering is deterministic from due tick, priority/aging, and
    sequence; it does not depend on incidental linked-list position.
11. Every registered periodic job has at most one active successor and survives
    callback failure according to policy.
12. All scheduler mutation happens on the game thread.

## Validation performed

The following focused tests passed:

```text
tests/async/test_nevent_analytics_contract.py
tests/async/test_nevent_budget_contract.py
tests/async/test_nevent_catchup_contract.py
tests/async/test_nevent_latency_diagnostics_contract.py
tests/async/test_nevent_regen_death_contract.py
tests/async/test_player_event_timing_contract.py
tests/async/test_zone_reset_timing_contract.py
tests/async/test_command_gate_recovery.py
tests/async/test_event_loop_hotspots.py
tests/async/test_patrol_event_size.py
tests/async/test_item_event_parser.py
tests/async/test_scalar_event_idempotency.py
```

Checkpoints 6 through 8 additionally passed
`tests/async/test_nevent_scheduler_runtime.py` under ASan/UBSan, the complete
225-test Python regression suite, and `tests/async/run_signal_handlers.sh`.
The first checkpoint 9 slice extended that harness with reciprocal owner/victim
link checks, non-mutating corruption detection, constant-time middle unlinking,
and debug game-thread ownership cases; the same full validation set passed.
The second slice replaced the earlier rearm harness with
`tests/async/test_nevent_periodic_rearm_runtime.py`, which exercises the real
registry under ASan/UBSan. The complete validation set passed again after all
eleven operational callbacks were migrated.

`make -C src -j2` completed and linked `bin/server/dms_new` successfully. The
built binary is a 64-bit PIE. A symbol/data probe found 6,220 text symbols in the
built executable and 6,220 rows in `lib/misc/event_names`, confirming NEV-01
against current artifacts.

No live server or database was used. No migrations, production operations, or
game-data mutations were performed. Implementation and verification are current
through the periodic-registry checkpoint 9 slice in the ledger above.

## Suggested implementation-session boundaries

To keep changes reviewable, do not combine the scheduler rewrite with all caller
migrations in one patch. Coherent sessions would be:

1. Event-name loader safety and its standalone tests.
2. Central cancellation/destruction invariants plus the misfire caller.
3. Typed/POD payload boundary and mob-hunt migration.
4. Stable ship volley references and destruction regression tests.
5. Periodic rearm fixes and periodic-job behavioral tests.
6. Absolute due-tick core and deterministic scheduler harness.
7. Priority/aging/catch-up policy on top of the new due-tick core.
8. Reschedule APIs and conversion of offline affects/object decay.
9. Heavy callback slicing/worker integration and operational dashboards.
10. Documentation, legacy cleanup, and durability classification.

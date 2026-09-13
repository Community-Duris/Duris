# Pure telemetry session state (#264)

This module implements the pure session-state boundary for #264. It does not
enable telemetry or install server hooks. The shared representation contract
is [CONTRACT.md](CONTRACT.md); #265 owns runtime integration and #266 owns activity
classification and contextual intervals.

## Ownership and bounds

`telemetry_session_state` is a caller-owned fixed-capacity value. Initialize it
outside ordinary per-player work and keep it in stable storage, not a small
per-command stack frame. Storage is `sizeof(telemetry_session_state)` regardless
of the configured number of usable slots. The current hard bound is 256 slots;
`max_slots` must be between one and that bound. This is an implementation limit,
not a measured population/capacity approval.

The private clock, sink and record-key allocator contexts are borrowed for the
state lifetime. Their callbacks execute synchronously; neither callbacks nor
contexts are placed in telemetry records. They must not refer to live game graphs
from a worker, reenter this state machine, block, allocate per event, or perform
SQL/file I/O. Only the game-thread owner calls state-mutating functions.

Use the SAME producer key allocator for session control and #266 interval detail.
The allocator owns unique increasing record sequences across both modules and
must never reuse a rejected admission's key or wrap on exhaustion. The session
module checks returned key shape and producer identity; it is not a second global
sequence authority.

Allocate session IDs at actual telemetry gameplay entry, and connection IDs at
telemetry enter/attach/resume admission, NOT at raw socket acceptance. New local
session sequences and current-producer connection sequences must be strictly
increasing in admission order. Separate process-lifetime high-water marks survive
slot retirement/reuse, so delayed calls cannot recreate discarded identities.
Exact known duplicates may return idempotent; unknown retired identities return
invalid, never recreate zero totals. After reset, use a fresh producer identity
rather than resetting allocation to one in an already-used producer domain. New
copyover connections use the new producer's sequence space.

The sink copies each record before returning. Its boolean
result is bounded RAM admission, not a durable SQL acknowledgement.

## Accounting authority

`telemetry_session_state_update_counters` accepts the shared typed delta. Its
resident delta must exactly equal the advance from the session's accounted
monotonic frontier to `at_monotonic_usec`. The connected/active/idle/unknown and
resident/linkdead identities must also hold. A backwards timestamp, an overlapping
or inflated delta, or arithmetic overflow is rejected without partially changing
totals. An exact repeated last update is idempotent; a changed positive delta at
the same frontier is invalid.

#266 supplies active/idle/unknown classification. Before a checkpoint, detach,
exit or handoff, #265 should flush #266 through the same monotonic cut first. If
that evidence is unavailable, the session module accounts the unclassified
connected remainder as **unknown**, never by extending the last active/idle
category. Detached observed time contributes only resident/linkdead duration.
Once a fallback advances the frontier, a late delta cannot overwrite or double
count that elapsed interval. This conservative loss of classification is preferable
to inventing activity. There is no gameplay reward or anti-idle policy here.

UTC labels never determine elapsed duration. The module tracks a paired
UTC/monotonic sample independently of the counter-only frontier. Backwards UTC
movement, or UTC-vs-monotonic elapsed divergence greater than one second in either
direction, marks clock-discontinuity quality. The one-second skew tolerance is a
private control-record detector, not a guarantee that smaller jumps are absent;
callers may propagate finer clock-discontinuity evidence in quality flags. Each
observation establishes a fresh paired anchor; an unknown UTC observation breaks
the mapping until another known sample. Midnight interval splitting is #266's
responsibility; cumulative session counters do not reset at midnight. Unknown UTC
uses the shared sentinel, not a guessed date. Extreme signed UTC endpoints are
handled without signed overflow.

## Private calls and lifecycle order

- `state_init` validates bounded configuration and initializes caller-owned state.
  It does not open a database, spawn a worker, or install hooks. `state_reset`
  discards observational memory; do not use reset as a durability/drain operation.
- `state_enter` consumes `telemetry_session_enter` on actual gameplay entry and
  attempts one `session_entered` fact. Ordinary reconnect is NOT another enter.
- `state_transition` handles typed attach/detach edges on an existing session.
  Detach names the closing connection; attach names a new current-producer
  connection. Session scope is retained. Copyover uses the resume path instead.
- `state_checkpoint` obtains the injected observation clock; `checkpoint_at`
  accepts the explicit timestamp form. Both publish absolute cumulative totals,
  never replay-additive deltas. Closed slots reject later checkpoints.
- `state_exit` accounts through the terminal cut, attempts the final checkpoint
  and exit fact, and closes the slot even if admission fails. Telemetry cannot
  veto gameplay teardown. Inspect dropped-record counts, not only the final
  lifecycle admission result, when presenting diagnostics.
- `state_handoff_copy` exports the shared bounded session handoff at the injected
  clock cut. It preserves session scope, previous producer, last allocated
  checkpoint revision, cumulative counters and quality. Before exporting, it makes
  one bounded attempt to admit any pending aggregate control-loss gap. Admission
  or key-allocation failure leaves the output all-zero and reports failure; it
  retains loss metadata for another scheduled attempt. It performs no file work.
  #265 must distinguish this from a valid handoff and must NOT serialize an
  all-zero failure as preserved continuity. The caller may retry after transport
  progress, never spin or veto gameplay copyover. If its bounded shutdown budget
  expires, use the absent-handoff/incomplete recovery path instead. Before discarding
  the old queue, #265 owns bounded drain/durability handling; RAM admission alone
  is not proof a gap survived process death. No queue or filesystem ownership is
  transferred by this function.
- `state_resume` imports a valid handoff under a new current producer and new
  monotonic anchor. It emits `connection_attached`, not `session_entered`, and
  never adds process downtime. The all-zero handoff is an explicit absent-state
  recovery path: it requires a new local session identity, emits an unknown-tail
  gap and marks the replacement observation incomplete. Partial handoffs fail.
  A nonzero handoff must name a session from an earlier producer, never the current
  process; its previous producer may differ from the session's original producer
  after repeated copyovers. Only a field-equal entry AND handoff are idempotent:
  changed cumulative totals, revision, previous producer or quality return invalid.
  An exact delayed duplicate never reattaches a detached/closed slot.
- `state_retire_expired` bounds retention of detached observational state and
  emits unclosed-tail evidence before forgetting it. It must never extract or
  otherwise mutate a live game character. After forgetting, integration must not
  reconstruct old cumulative totals from zero under an existing session key.
- `state_copy_view` and `state_stats_copy` return value copies for tests and
  bounded integration decisions. They are not synchronized cross-thread readers.

## Admission loss and completeness

Record admission failure does not roll back gameplay-time accounting. Allocated
checkpoint revisions survive a sink rejection, and a later admitted checkpoint
can recover the total. It cannot recover lost interval context or prove that an
open/close event was durably observed. Exact duplicate calls are not a persistent
queue retry mechanism; the transport separately owns immutable admitted batches.

The module retains a bounded aggregate of control losses. A single scoped loss can
name its session/connection; mixed losses become process-wide instead of being
falsely attributed to the last session. Affected resident slots retain queue-loss
quality after aggregate gap publication. Unknown loss durations remain zero with
explicit gap reason/quality, not an assertion of complete zero activity.

Closed slots are reusable; detached slots have a configured finite retention
limit, bounded by one hour. Lifecycle dimensions, config ID and classifier/policy
versions are the ENTER/RESUME snapshot, not current-context reporting. Do not use
them to attribute later level/zone/config activity. #266 owns context changes and
intervals; #263/#265 own effective config publication and any required integration
update path. This module deliberately has no mutable-context API. A new admission
with new IDs after forgotten state is a new observation, not recovery of the old
totals. The unclosed gap documents that retirement boundary.

## Focused verification and integration checklist

Run `python3 tests/async/test_telemetry_session_state.py` from the repository root.
It compiles and executes the real module in SQL and `__NO_MYSQL__` modes without
SQL/game libraries. Its sink is finite, its clock is controlled, and C++ allocation
attempts abort the harness. Compiled artifacts are temporary under `bin/tests`.
The harness covers reconnect totals, copyover continuity, drops, duplicate calls,
capacity, arithmetic limits, elapsed-time authority, mixed-loss scope, stale IDs,
conflicting resumes, drop-before-copyover and UTC jumps/midnight. The runner also
translates `normal_interval.json`, `detach_reconnect.json` and
`copyover_handoff.json` into typed calls executed by the real C++ module in both
compile modes. The golden harness compares resulting cumulative counters,
logical-session/connection identity, handoff state and emitted checkpoint revisions
against the independent expected fixture fields. These scenarios test the pure
module; repository replay/conflicting immutable record delivery belongs to #262.

#265 must provide stable state/service lifetime, shared sequence allocation,
config-publication ordering, classifier-before-checkpoint ordering, real entry
and terminal hooks, detach/reconnect identity handling, copyover serialization,
bounded retirement scheduling and cached health exposure. Do not edit player
saves, descriptor layouts, `comm.c`, `nanny.c`, `copyover.c`, SQL or build manifests
in this pure-module change. Live login/reconnect/copyover journeys belong to the
later integration slice; this test does not claim those hooks already work.

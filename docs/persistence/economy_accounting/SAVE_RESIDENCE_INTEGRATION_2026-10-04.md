# Save residence integration — 2026-10-04

Status: source implemented and reviewed; **unqualified**. This connects the
existing replay-ownership leaf to save capture, queues, workers and receipt
delivery. Ownership remains disabled in production. Plan 1 is not complete.

## Established missing behavior

At source base `d92474bd6`, pipeline queues retained bare snapshots, worker slots
and undelivered completions had no resident claim, and shutdown cleared pipeline
owners after joining threads. Counted execution permits alone could not exclude
replay while an original capture or its receipt delivery still existed. This is
a source-established integration gap, not an executed runtime failure.

## Changes

- Acquire an exact-PID claim before capture/revision queueing; move it with the
  pending and durable envelope, worker active/pending original, and final owned
  completion. Pinned death retries derive a claim only from their retained exact
  immutable body. Refused worker submissions preserve body and claim.
- Scope journal append/archive and worker apply/ACK/terminal mutation. Final
  completion delivery becomes visible only after worker scopes and nested
  permits unwind. The main pulse retains residence through receipt callbacks,
  recapture decisions and dispatch. Legacy pulse refuses claim-bearing results.
- Use the existing dispatcher and release event to revisit up to all 256 worker
  ownership waiters. Observe before inspecting queues; new work and stop signal
  after publication. Pending append scans skip busy/held PIDs. Ownership busy
  parking does not consume failure retries or install periodic polling.
- Preserve an unjournaled append original in allocation-free retry storage.
  Eligible originals can retry append even if deque allocation remains unavailable.
  Enabled shutdown retains owners, journal namespace and holds and reports
  incomplete; joining threads is not a clean ownership census.
- Include retry and in-flight captures in admission counts. Source review found
  a bounded-scan exhaustion path could erase an unattempted durable capture;
  exhaustion now retains every original instead.

## Source evidence

Maintainer implemented worker C/H; primary integrated pipeline and leaf;
independent architect reviewed the final four files without a remaining blocker
in this disabled-epoch slice. Formatting and `git diff --check` passed.
No compiler, test, AST, native, SQL, gameplay, service or recovery check ran.
Testing remains deferred until major-plan readiness under the user's instruction.

Final raw SHA256 pins (line-ending specific):

| Source | SHA256 |
| --- | --- |
| `src/player/player_save_pipeline.c` | `8009d12d8be3a9cd5aebea0cba9c8ea5dc4ee4abfd3853f16a4435053ad18e0e` |
| `src/player/player_save_replay_ownership.h` | `3477d2978ae2061d6cc044354a720ae75f45578be03a31e54479d51f064e179a` |
| `src/player/player_save_worker.c` | `b46faddd244e5a5d96a213ed4cc4bcf08f54b129e490340ae0a33e3554215a9f` |
| `src/player/player_save_worker.h` | `c67f6ba89c5ca3df19b9da495330e21de3b7c76d5ae4e300827419726498bb80` |

Private BEFORE selected-input manifests preserve four pipeline inputs
(`cba56ebee472c354a24a9bc8be586ef5a149b982dd64c610817304d9ffd8f34c`)
and eight worker inputs; prepared worker manifest
`3746bc4a172585e89a851991cfe851b5d00b76f4ebf62c97a3d27fad71380fa4`
contains two source copies and 27 unexecuted declarations. These are not compiler
closures, executable fixtures or acceptance evidence. No synthetic inventory
closes a plan or qualifies a route.

## Remaining required integration

Keep the epoch disabled until independent native writers and journal mutation
entries participate, lifecycle ownership proves all old unclaimed state absent,
and replay uses an exclusive per-PID reservation and fresh journal observation.
Production ordinary-drop recovery still needs native authority/materialization,
complete clean census, critical ACK, startup ordering and stopped-worker recovery.
Enabled shutdown deliberately does not claim durable handoff or close the epoch.
Receipt callbacks' existing failure behavior still needs actual-owner qualification.

This supplies no new gameplay policy, activation authority, complete writer
coverage or full R1–R8 completion. Inactive behavior and the declined spell-path
change remain preserved; no unqualified milestone was pushed.

## Follow-on journal connection

The journal replay owner now stages exact-PID tickets and reservations before
native callbacks, then rereads active frames. Only successfully reserved PIDs
can apply; busy, held, quarantined or newly observed unreserved PIDs retain their
frames while available PIDs progress. Each apply/quarantine uses its individual
scope; both early and final checkpoint exits use the preallocated aggregate
scope after callback permits/scopes unwind. Reservations outlive checkpointing.

Journal archive/quarantine/recovery mutation entries now borrow the enabled
caller scope before mutation. Recovery resolution retains that permit across
its unlocked verifier callback. Enabled init/shutdown refuse namespace replacement
or clearing; serialization with epoch begin/end is still the lifecycle owner's
required responsibility. No production epoch is enabled by this connection.

Enabled corruption scans refuse without archiving unknown-PID bytes. Source
review also found initialized-file disappearance could be accepted as an empty
journal; enabled ENOENT now fails closed. Legacy disabled-epoch behavior remains.
This is a source-established missing-namespace defect, not an observed test RED.

Raw source pins: journal C
`d5e0c25a3204f6e6c3b8c6ea5873e392956bd390418f2f4aa9eb1a2c2132b81a`,
header `27c6109e8158708f83fbd90236db5af5cfa3ce253ea21ea674dbaafc3a21e7bb`.
Private copies preserve the prior C/H at source base `c96fcb2f1`; they are not a
compiler closure. Independent architect accepted the final source pins with no
remaining blocker in this bounded slice. Formatting/diff hygiene only; no
tests/compiler/native checks.
Extend the maintained journal deferral/unresolved test owners at major-plan
qualification for reservation races, fresh observation, both checkpoint exits,
proof withdrawal, namespace loss/corruption and scoped recovery mutations.
Checkpoint internals may still allocate: failure retains evidence, rather than
establishing allocation-free completion after apply. Full native writer ownership,
clean ACK census, actual revisit/startup and lifecycle completion remain open.

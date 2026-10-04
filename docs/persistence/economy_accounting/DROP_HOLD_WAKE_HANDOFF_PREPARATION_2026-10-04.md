# Original SQL-drop hold release to worker wake — 2026-10-04

Status: source implementation; **UNQUALIFIED**. No compiler, native, SQL,
gameplay, service or recovery check has run for this slice. The user batches
new testing at major-plan readiness. This is not completed production recovery.

## Established missing ownership

The prior post-publication owner clears the exact original restored SQL-drop
checkpoint and execution hold, but supplies no worker notification. The worker's
prior resume API accepts only an already parked job. A dispatch can observe the
hold, the publication owner can release it before worker parking, and the save
can then park without a retained wake. This is a source-traced schedule, not an
executed semantic RED.

## Shared handoff

While pipeline metadata and the original execution hold remain installed, capture
`player_save_deferred_identity` from the current active worker request. The value
binds PID, persisted revision, request generation and worker lifecycle. It carries
no mutation or ACK authority. Capturing invokes no callback. The leaf worker mutex
is acquired under pipeline_mutex; worker append/apply/ACK/terminal callbacks run
outside worker_mutex, and no new reverse callback lock edge is introduced.

Move the original checkpoint out, clear its slot, and release only its exact
original operation/generation. If release fails, restore that checkpoint, preserve
the guard's integrity poison, and emit no notice. On success collect the captured
identity in fixed storage bounded by the same 256-slot checkpoint array.
After releasing pipeline_mutex, notify that identity through the worker's sticky
allocation-free exact-request API. A wake accepted before parking remains on that
request. A later PID replacement or worker lifecycle cannot consume it. If there
was no active request while the hold was installed, no wake is invented; a future
request must acquire the already released guard normally.

The unchanged non-restored checkpoint release does not install an execution hold
and receives no new notification. Every worker dispatch still acquires the native
execution guard. No polling, SQL authority, snapshot rewrite, fabricated result,
PID-wide future credit or journal retirement follows from a notice.

## Source and execution boundaries

Owned source: `src/player/player_save_pipeline.c/.h`. The separate worker slice
owns `src/player/player_save_worker.c/.h`. Primary integrates both reviewed slices.
Immutable pipeline BEFORE files from decf5beac are preserved in
`tmp/drop-hold-wake-before-v1.local/manifest.json`. The existing source has no
production call to the SQL-drop acknowledgement API; current calls are native
fixture owners. This handoff alone does not connect comm startup or the critical
coordinator's durable publication ACK path.

The caller must already have independent native epoch/current graph proof, a
complete affected-PID mutation census and a coordinator-enforced ACK reservation.
Those prerequisites remain open. Existing login, quarantine, save, shutdown and
unsupported route gates remain required. Save-journal replay needs its separate
revisit owner; waking a worker does not restart replay.

## Major-plan qualification cases (not executed)

Preserve the original maintained oracles and budgets. At the integration batch
exercise actual restored pipeline checkpoints, guard holds and worker scheduling:

- Release before first dispatch, between hold observation and park, and after park.
- Release with no active job, pending replacement, duplicate notices and unchanged
  exact request bytes/revision/receipts through resume.
- Wrong operation, repeated original ACK and failed exact guard release: no new
  notice or release of another checkpoint.
- Completion/replacement and shutdown/reinit between identity capture and notice:
  stale identity must not credit a different job or lifecycle.
- Allocator failure during wake acceptance/dispatch, no ready-container growth,
  one guard recheck per notice, held repark without repeated attempts, and unrelated
  PID progress.
- Native SQL apply/COMMIT/journal checkpoint and real critical ACK/hold-clear pauses
  on both SQL engines, absent actor, cold restart twice and copyover/reconnect.

Source pins and the final read-only review are appended after integration. Private
fixture preparation is neither native execution nor full accounting completion.

## Final source review and pins

Read-only architect review found no blocker in the handoff's stated narrow scope.
Worker callbacks remain outside worker_mutex; fixed checkpoint storage bounds
notification collection. No native execution or schedule calibration was performed.

| Input | Raw SHA-256 |
| --- | --- |
| `src/player/player_save_pipeline.c` | `f9c6944608442cc2b4602b9858e41efc89c5510e93527dfbd9e71bf937be4050` |
| `src/player/player_save_pipeline.h` | `93a1a30dbbb9064b4a234ab5ba31a5f0ff08f6a1cbced230f7b407eb384c0999` |
| `tmp/drop-hold-wake-before-v1.local/manifest.json` | `a30b2f56ad5bb3a765db48e7edb1c712ec516043ef1aab41f431ebc7163526a0` |

The dependent worker mechanism and exact identity contract are documented in
[the worker source preparation](DEFERRED_SAVE_WAKE_PREPARATION_2026-10-04.md).
A token is process-local notification identity, not an authenticated native proof.

Dependent worker source is committed separately in aa385e73b. Its preparation
V2 corrects a private BEFORE-header overlay mistake while preserving V1; source
pins and read-only review remain unchanged. The corrected inputs remain unexecuted.

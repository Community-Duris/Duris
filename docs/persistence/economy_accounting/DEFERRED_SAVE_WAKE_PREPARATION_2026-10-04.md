# Deferred original save wake preparation — 2026-10-04

Status: **IMPLEMENTED, SOURCE-REVIEWED, UNQUALIFIED**. The user deferred testing
until each major plan is ready. No compiler, AST, native test, SQL/service,
gameplay or recovery execution was performed for this slice.

## Source-established failure and corrected ownership

Selected BEFORE base: `decf5beacaf51c7caf3453f27efbd20912a78065`.
`player_save_worker_resume_deferred(pid)` required an undispatched, already
parked owner. A one-shot release notification during callback execution or
between a held decision and parking returned false and stored no notification.
The original could remain parked indefinitely unless its caller retried.
This is a source-established race, not executed native RED evidence.

The new process-local `player_save_deferred_identity` binds PID, exact snapshot
revision, monotonic request generation and worker lifecycle. Capture the active
original using `player_save_worker_deferred_identity`; after successful owner
release call `player_save_worker_resume_deferred_exact` outside the owner's
pipeline/journal locks. Refused capture leaves caller output unchanged. The
token neither authenticates a release nor grants repository execution authority.

Notifications set one sticky bit on the exact active request without allocating.
They are accepted during dispatch before parking and duplicates coalesce. The
worker selects notified parked originals directly without growing its ready set
or deque. It consumes the bit before the next execution-guard check. A still-held
request parks again without ACK, callback admission, failure retry or completion;
another external notification is required to recheck it. The controlled apply
callback can separately return deferred under the unchanged gate contract.

When ordinary ready work and notified originals coexist, worker selection
alternates between the classes. Bounded scans use the existing 256 PID limit;
notified originals do not wait for the ordinary FIFO to empty. Ordinary-only
FIFO selection is unchanged. The worker mutex serializes dispatch, so multiple
notices cannot dispatch the same owner concurrently.

Pending, coalesced replacement and promoted bodies retain independent request
generations and never inherit the original's notification. Nondeferred completion,
shutdown and reinitialization clear notifications. Stopped workers reject capture
and notification; initialization increments lifecycle, so old tokens cannot wake
even a resident original after a warm restart. Request/lifecycle counters never
reset, including test reset. Counter exhaustion fails closed: initialization
refuses lifecycle overflow; admission refuses request overflow, retaining an
already journaled frame via the existing durable-spill result. Ordinary counter
values change no capture bytes, revisions, codecs or journal format.

The legacy PID-only API remains a convenience that atomically notifies the
current active original. Its bool now means acceptance, including a coalesced
duplicate or wake before park. It cannot bind a delayed old release to an earlier
request; production cross-owner handoffs must use the exact API. Neither API
substitutes for the execution guard or authorizes SQL/critical publication ACK.

## Producer handoff and lock boundary

Primary owns pipeline integration. The intended chain captures the active token
while the original pipeline slot and guard hold still exist, clears the matching
slot, releases the exact guard generation, then notifies outside pipeline_mutex.
Failed guard release restores the original checkpoint and sends no notice. No
active worker means no token: a later admitted request checks the released guard
normally. Wake capture/notification invoke no callback under worker_mutex; apply,
ACK and terminal hooks remain outside it. This permits the pipeline-to-worker
leaf lookup without introducing a worker-to-pipeline callback inversion.

Notifications are resident process state. Warm restart requires a fresh token and
fresh release handoff; cold restart requires original journal/critical obligation
reconstruction. This patch does not persist wake events, resume cold replay or
establish the complete prepare→restore→start lifecycle.

## Deferred preparation, pins and limitations

Immutable selected-source BEFORE manifest:
`tmp/deferred-save-wake-before-v1.local/manifest.json`, SHA-256
`b77feb41caa46ab05bd8a5ff024264a3f0f3d7eaeecf9054cf6b87e720fe70c9`.

Source pins independently reviewed by `cpp_modernization_architect`:

- `src/player/player_save_worker.c`:
  `0f30ee41568c878c5ef42d631934719e6043bea7af00c9e7ea64298e54a616da`.
- `src/player/player_save_worker.h`:
  `a1a304d0e253896f4ae61f8bf909584412ade24c09c10bd7401bf6f761ea7301`.

Private preparation receipt:
`tmp/deferred-save-wake-prepared-v1/manifest-v2.json`, SHA-256
`b241ea6ad0dd6208f38843e102d9e8491b3eb7d577563b0c0c400553f1473208`.
The original v1 BEFORE-header overlay missed a backslash path and copied the
AFTER header; v1 is preserved as insufficient. Corrected `closure-v2` copies
the exact original worker header and source. A raw inventory comparison confirms
only those two files differ across the final variants. No execution occurred.
Each frozen variant contains 533 files: five maintained production worker/revision/
codec/journal/observability units, a conservative complete `src/*.h` inventory,
and two private fixture files. Only worker `.c/.h` differ between variants; this
is a selected-source comparison, not the entire historical branch. Actual
compiler `.d`, system headers, SQL client and toolchain inputs remain unmeasured.

- `sticky.cpp`: `442b2e91151ecad17a44db9987206aff744f0ecc7b3545217dacc5c6f38c8d8f`.
- `baseline_harness.cpp`: `84b9e6f61b010c2fdc797e21b79408ee1f87b236863abf5d1cd253e0d0ac7635`.

Thirteen cases are prepared: two shared PID-only before-park/pending cases, five
preserved effect controls (parking, coalescing, ACK failure, journal reopen and
warm reinit), and six AFTER-only exact-API controls (allocation-free notification,
still-held repark without polling, lifecycle identity, forged identity,
replacement identity and ready-queue fairness). All remain unexecuted. Missing
new APIs on BEFORE are unsupported cases, never semantic RED. Original compile
300-second and aggregate runtime 120-second budgets remain, with separate
SQL-header and flatfile profiles.

The private callback samples a synthetic held decision before its barrier, then
uses actual production worker/revision/codec/journal units. It does not pause an
actual execution-guard observation, perform SQL COMMIT, grant production critical
ACK or prove gameplay/cold restoration. Full pipeline/guard timing, actual native
persistence, cancellation/lifecycle reconstruction and independent recovery still
require major-plan qualification.

Maintained tests were not edited. Their old wake-race and allocation-walk oracles
expect refusal before parking and set/deque allocation failure; those old
contracts intentionally conflict with sticky acceptance and allocation-free wake.
Preserve their BEFORE originals. Any later AFTER adaptation must establish exact
payload ownership, no duplicate execution, real ACK, unchanged retry/quarantine
behavior and unaffected progress rather than delete the conflicting assertions.

Only formatting and `git diff --check` were executed. Source review found no
blocker within this resident-worker slice. Full R1–R8 completion, restored
producer wiring, complete writer/command census and activation remain open;
`coverage_complete=False` and `release=BLOCKED` are unchanged.

# Full-world save failure investigation

Status: completed with documented historical limits; three reproduced defects fixed and validated. Recorded 2026-09-05.

## Findings

Three reproduced defects are corrected in `5c7d8b662`:

1. Deferred saves and manual completion checks could wait behind world callback
   debt. They now run from the existing bounded persistence pulse.
2. Launcher signal shutdown unnecessarily scheduled a world callback. It now
   selects the existing immediate main-thread transition and retains save/drain
   gates.
3. A successful automatic retry after cancelled camp could leave an acknowledged
   fence that a later camp reused with nonterminal intent. Every terminal request
   now captures a fresh full revision for its current intent and room.

The original PR #151 cancellation's initiating cause remains unproven because
its server-side revision/worker timeline was not retained. The controlled lock
injection explains a reproduced cancellation and exposes the stale-intent bug;
it is not evidence that the historical cancellation had the same trigger.
The investigation log below preserves pre-fix failures, controlled experiments,
backend differences, validation, and limits. Production was not exercised.

## Objective

Determine why the isolated full-world flat-file journey intermittently fails
to complete manual saves or camping under concurrent test load. Establish
whether the failure is delayed observation, delayed durability, or incorrect
save/revision handling; then fix the demonstrated cause with a focused
regression. Passing a rerun alone does not close this investigation.

## Evidence and limits

- PR #150 validation: `test_flatfile_full_world_boot.py` waited 45 seconds
  after `save`, received `Save queued`, and never observed `Save complete`.
- PR #151 validation, head `95b9e56535669a27814e2e34da19db0be435ae1e`:
  `make test-all -j4 TEST_JOBS=4` passed 410/411 tests. In the second server
  process, manual save completed, but subsequent `quit` produced
  `Your character could not be saved, so your camp is cancelled.` The test
  timed out waiting for the account menu.
- Both unchanged tests passed isolated reruns. Concurrent builds/tests and
  CodeQL analysis were running during the PR #151 failure. Load sensitivity
  is a hypothesis, not an established cause.
- An earlier full-world save timeout was also reproduced on the then-current
  unchanged master during PR #147 validation. This does not establish that
  every later failure has the same cause.
- These observations concern synthetic flat-file test instances. They neither
  demonstrate production data loss nor establish production immunity.

Existing local evidence, if still available: `/tmp/duris-pr150-final-test-all.log`,
`/tmp/duris-pr150-final-full-world-rerun.log`,
`/tmp/duris-pr151-test-all.log`, and `/tmp/duris-pr151-full-world-rerun.log`.
These temporary files are discovery aids, not durable investigation records.
Store sanitized findings in this document; do not commit runtime logs or fixtures.

## Relevant behavior at the investigation baseline

- [Full-world journey](../../tests/async/test_flatfile_full_world_boot.py):
  builds a client-free server, runs against temporary authority data, and
  checks player/floor-item recovery across a process restart. Current temporary
  directory cleanup can discard server-side evidence after a failure.
- [Manual and terminal saves](../../src/cmd/actoth.c): manual save status has
  a 30-second deadline, evaluated through scheduled callbacks. The test's
  45-second wait is a separate deadline. Terminal saves request a 2,000 ms
  pipeline wait; flat-file builds disallow journal-only handoff. Failed
  terminal saves schedule a deferred retry.
- [Terminal pipeline](../../src/player/player_save_pipeline.c): the terminal
  call can return invalid, unavailable, timed out, or a successful durability
  outcome. It pulses completions while waiting. A cancelled camp does not
  identify which failure outcome occurred.
- [Camping](../../src/magic/affects.c): a failed terminal save restores the
  previous home, cancels camping, and retains the online character. This is
  protective behavior, but does not prove that the retry later becomes durable.

## Investigation sequence

### 1. Preserve and classify failures

- Use a separate development checkout and synthetic account/authority fixtures.
  Record the commit, backend, compiler, CPU/memory limits, concurrency, and
  scheduler configuration. Never point this test at live player data.
- Add an opt-in failure-artifact mode to the test, retaining the command
  transcript, both server outputs, exit status, and relevant synthetic authority
  state outside tracked paths. Keep normal successful-run cleanup.
- Label each failure by phase: first/second boot, manual save, terminal save,
  restart recovery, or shutdown. Distinguish an assertion deadline from a server
  rejection and from a confirmed persistence failure.

### 2. Reproduce with controlled load

Start with these existing entry points:

```sh
python3 tests/async/test_flatfile_full_world_boot.py
make test-all -j4 TEST_JOBS=4
```

- Run ten isolated journeys, then ten journeys with recorded, bounded CPU
  contention representative of the original concurrent builds. Run at least
  three full suites at the original concurrency. Capture outcomes and phase
  latencies for every attempt, including successes.
- Separate build time from server runtime. If needed, add a test-only way to
  reuse a verified binary from the same source revision so repeated runtime
  trials do not confound compilation with gameplay load.
- Compare immediate post-boot execution with execution after measured startup
  activity settles. Use this as a diagnostic experiment, not an unexplained
  sleep added to the acceptance test.
- If CPU contention does not reproduce it, vary disk contention independently.
  Stop broad repetition once a failure is captured and inspect its timeline.
  If these bounded trials fail to reproduce it, report that limit and retain
  artifact capture for the next occurrence; do not declare the issue fixed.

### 3. Trace one save from request to durable acknowledgement

Use monotonic timestamps and synthetic player/revision identifiers to correlate:

1. Command receipt, event due time, and callback execution.
2. Snapshot capture, target revision/component mask, and terminal fence state.
3. Worker enqueue, queue depth/age, worker start, storage result, and durable
   revision. Identify the backend's actual durability boundary rather than
   treating a queued request or buffered write as durable.
4. Completion publication, pipeline consumption, acknowledged revision, and
   player-facing response.
5. Terminal outcome and deferred retry: scheduling, execution, result, and
   eventual acknowledgement for the same character.

Use existing pipeline/worker health snapshots first. Enable
`DURIS_NEVENT_ANALYTICS=1` and `DURIS_NEVENT_TRACE_PLAYER=1` only for the isolated
fixture where useful; add narrowly scoped metadata diagnostics for gaps.
Measure scheduler delay, worker delay, storage duration, and completion delay
separately. Capture errors and queue saturation as well as timeouts. Diagnostics
must not include credentials or real account/player payloads.

Test competing explanations explicitly: scheduler starvation; slow worker or
storage; blocked completion delivery; stale/mismatched revision or terminal
fence; fixture interference; or a test waiting for the wrong response. Do not
assume the manual-save timeout and cancelled camp share a cause.

### 4. Establish the durability and player-impact boundary

- Confirm a failed camp leaves the character usable and that its deferred save
  eventually persists the intended state without another manual save command.
- In synthetic fixtures only, inject controlled delay/failure at the identified
  boundary. Verify failure messages, retry behavior, and successful subsequent
  logout. Do not weaken the requirement to save before character removal.
- Restart after a confirmed save acknowledgement and assert expected player
  state, item UIDs, and exactly-once ownership across player/floor locations.
- Separately terminate the test server before and after the measured durability
  boundary. Document what pending progress is recoverable; acknowledged state
  must survive. Keep intentional crash trials distinct from clean shutdown tests.
- Assess whether the demonstrated mechanism also affects MariaDB. Its
  journal-handoff behavior differs; use an isolated backend-specific fixture if
  the shared code path warrants it. Do not extrapolate from flat-file results.

### 5. Fix and verify the demonstrated cause

- Make the smallest correction supported by the captured timeline. Add a
  deterministic regression using controlled delays or worker outcomes, not
  uncontrolled CPU pressure alone.
- Do not resolve this by merely increasing the 45-second assertion timeout,
  suppressing save errors, or accepting an isolated rerun. Any deadline change
  needs measured justification and an explicit durability/UX contract.
- Run the focused regression and full-world journey, then repeat the load case
  that reproduced the failure. For C/C++ changes, run `make -C src` and the
  repository formatting check; finish with the full local CI checks.

## Completion criteria

- Each observed failure class has a supported explanation, or is explicitly
  left open with its missing evidence identified.
- The regression fails before the fix and passes afterward; the controlled
  reproducer no longer fails across at least ten repetitions.
- Failed terminal saves preserve the online character and recover through the
  intended retry path. Successful acknowledgements correspond to durable state
  that survives restart without item loss or duplication.
- At least three post-fix full suites pass at the original concurrency. These
  runs supplement causal evidence; they do not prove absence of all races.
- Record the root cause, affected backends, practical player impact, fix commit,
  validation results, and any unresolved limits here. Production intervention
  is outside this plan and requires separate authorization.

## Investigation log: 2026-09-05

Work is in the separate development worktree
`/home/aiwithapex/projects/duris-full-world-save`, branch
`investigate/full-world-save`, based on `0ae67e781`. The original checkout and
its untracked investigation document are preserved. No live environment file,
account, database, or running game server is used by these trials.

Historical logs were inspected directly. PR #150 failed in the first process's
manual-save phase: the response contained `Save queued` and no completion or
explicit failure before the 45-second assertion. PR #151 failed in the second
process's terminal-save phase: the response explicitly cancelled camp, followed
by the 30-second account-menu assertion. These are observation-deadline and
server-rejection classes respectively; neither proves a persistence failure.
Neither historical log contains the necessary worker/revision timeline.

The journey now has opt-in failure capture (`DURIS_FULL_WORLD_ARTIFACT_DIR`),
monotonic command/response and phase timing, server exit statuses, and
`DURIS_FULL_WORLD_REPEATS` to repeat independent fixtures after one build.
Failure bundles contain only synthetic authority, journals, and diagnostics;
TLS keys are excluded and the synthetic password is redacted from transcripts.
Successful fixtures still clean up. The test's gameplay deadlines are unchanged.

Initial baseline command (in progress):

```sh
DURIS_FULL_WORLD_ARTIFACT_DIR=/tmp/duris-save-investigation/isolated \
DURIS_FULL_WORLD_REPEATS=10 DURIS_NEVENT_ANALYTICS=1 \
DURIS_NEVENT_TRACE_PLAYER=1 \
python3 tests/async/test_flatfile_full_world_boot.py
```

Host: Ubuntu g++ 13.3.0, 16 available CPUs, approximately 48 GiB RAM and
16 GiB swap. `/proc/self/cgroup` reports `/init.scope`; the usual root
`cpu.max`/`memory.max` files are unavailable, so no cgroup quota is inferred.
The synthetic server receives an explicit environment with flatfile-primary,
loopback listeners, Redis disabled, and private temporary authority/journals.
Scheduler defaults are 25,000 microseconds and 4,000 callbacks per pulse,
with catch-up extensions of 5,000 microseconds and 4,000 callbacks. Both
analytics switches are enabled for this diagnostic baseline. The existing
unrelated server remains running; no claim of a completely idle host is made.

Storage inspection establishes that the flat-file player repository reports
`applied` only after its atomic write completes: file `fdatasync`, rename, then
containing-directory `fsync`. Worker completion publication follows repository
return (and journal acknowledgement). This is source evidence of the intended
durability boundary; crash/restart trials remain required to verify behavior.

Status remains open. No server defect or fix is claimed yet. Controlled load,
causal tracing, failure/retry and crash-boundary trials, and full-suite validation
remain outstanding.

Trial-control note: a two-job diagnostic build briefly overlapped baseline
attempt 3 before being suspended (process group 899014). Attempt 3 must not
be counted as an isolated-load observation. Additional isolated coverage is
required. The running journey binary predates the new C++ diagnostics.

Diagnostic implementation in progress additionally touches
`src/player/player_save_pipeline.c`, `src/cmd/actoth.c`, and
`src/world/new_events.c`. Under `DURIS_NEVENT_TRACE_PLAYER=1`, pipeline messages
record monotonic capture/submission/completion times, revision/component
identities, worker queued/start/completion times and storage outcomes, and
terminal timeout fence/revision/queue state. Scheduler tracing includes all
player-owned callbacks (retaining the existing ordered-NPC timing coverage).
Rejected terminal saves log their typed outcome and retry request. No deadline,
save acknowledgement rule, or extraction behavior has changed. C++ validation
is pending; its build is suspended until baseline runtime trials end.

Progress checkpoint: first four journeys completed both processes and clean
shutdowns. First-process manual save latencies were 12.009, 12.009, 12.511, and
12.509 seconds; restarted-process manual saves were approximately 2.002 seconds.
Camp/menu latency varied from 0.751 to 14.761 seconds. Attempt 3 remains
contaminated as described above. The fifth journey is still running at this
checkpoint. These successful observations do not close either historical
failure class. Artifact retention/cleanup was checked with a controlled Python
exception; `python3 -m py_compile tests/async/test_flatfile_full_world_boot.py`
and `git diff --check` pass.

### Baseline outcome

The ten-journey baseline exited 0. All twenty processes completed their
manual save, camp, and clean shutdown. Nine journeys are uncontaminated;
attempt 3 is excluded from isolated coverage. Timings below are seconds from
command send to the expected response, not storage latency.

| Attempt | First save | First camp | Restart save | Restart camp |
| --- | ---: | ---: | ---: | ---: |
| 1 | 12.009 | 0.751 | 2.002 | 12.760 |
| 2 | 12.009 | 8.758 | 2.002 | 2.752 |
| 3 | 12.511 | 14.761 | 2.002 | 9.757 |
| 4 | 12.509 | 3.252 | 2.002 | 10.257 |
| 5 | 13.010 | 17.264 | 2.001 | 16.262 |
| 6 | 16.510 | 11.260 | 4.253 | 7.006 |
| 7 | 13.510 | 15.763 | 4.504 | 3.252 |
| 8 | 13.011 | 17.265 | 3.753 | 17.515 |
| 9 | 19.265 | 0.500 | 2.001 | 17.762 |
| 10 | 14.260 | 12.009 | 2.002 | 17.763 |

The baseline binary is retained under ignored `bin/save-investigation/baseline/`
with SHA-256 `9a5b7aa135aeeff41690efa70988608aa4802789aa5d66922faa7dacde7e15ca`.
The suspended diagnostic build was resumed only after the baseline process
exited. The next bounded CPU trial restricts the test and four competing CPU
processes to two CPUs, adding competitors only after compilation completes.
This is a controlled saturation experiment, not a recreation of the historical
host workload.

### Captured manual-save failure under bounded CPU contention

The first CPU trial reproduced the historical manual-save symptom and stopped
further repetition. Both the test and four CPU competitors were restricted to
CPUs 0 and 1; competitors began after the flat-file build. The journey exited 1.
The native diagnostic build and focused checks had completed before this runtime
trial. Binary SHA-256:
`676723c327530515bdcc8a51370ccb542bf7cd061a6600f15bb78c23629a6b89`.
Failure evidence is retained at
`/tmp/duris-save-investigation/cpu/attempt-1-3wf1fd6b`; auxiliary address maps are
in `/tmp/duris-save-investigation/cpu-live/922894.maps`. These paths are local
untracked evidence, not durable deliverables.

Monotonic timeline (microseconds except where noted):

- Revision 17: captured 645241468683, queued 645241550850, worker started
  645241550894, completed 645241558346, consumed 645242035646. Storage plus
  journal acknowledgement took 7,452 us; completion observation lag was
  477,300 us. Revision 2 had already been superseded (submit outcome 5).
- Manual `save` sent at 645249.042936 seconds; the server responded `Save queued`.
- At 645269259722, `event_balance_affects` ran with due tick 29 and actual
  tick 142: 113 pulses late. At pulse 169 the scheduler still had 44,462
  deferred callbacks and oldest due tick 31.
- The manual assertion expired at 645294.104599 seconds (45.061640 seconds).
  There was no manual snapshot capture or player-save completion between the
  queued response and this deadline.
- Disconnect cleanup then captured revision 48 at 645294317600, submitted it
  at 645294319934, and consumed its durable completion at 645294336038.
  Worker storage plus journal acknowledgement took 5,449 us. Cleanup exceeded
  its separate five-second process wait, so the fixture was killed (exit -9).
  This forced cleanup is distinct from the observed manual-save failure.

Supported cause for this reproduced class: manual snapshot capture and status
polling are deferred through the general world scheduler, behind a large NPC
callback backlog. The worker was not slow during the observed saves; the manual
request had not reached it. This is delayed save initiation/observation, not a
confirmed storage write failure. The historical PR #150 log is consistent with
this mechanism but lacks the server-side evidence to prove identical causation.
The historical camp rejection remains a separate open class.

The smallest intended correction is to service pending saves/retries and manual
acknowledgement checks from the existing bounded main-loop persistence pulse,
using their existing fixed-capacity slots and retry policy. This avoids making
persistence progress dependent on NPC scheduler debt. General world scheduling,
terminal durability requirements, and gameplay deadlines remain unchanged.

Validation completed for diagnostics: `make -C src -j2`,
`./scripts/format.sh --check`, `test_player_save_pipeline.py`,
`test_terminal_save_safety.py`, `test_manual_save_feedback_contract.py`, and
`test_nevent_scheduler_runtime.py` (including ASan/UBSan) passed.

### Manual-save correction and deterministic proof (in progress)

Deferred save slots now retain a monotonic due time and the character's runtime
identity. The existing main-loop persistence pulse services up to 32 due save
attempts per call using a round-robin cursor, then checks the existing 512 manual
status slots for acknowledgement or the unchanged 30-second deadline. The old
world events for these two save responsibilities are removed. Requested delays
and retry backoff remain expressed in the same quarter-second units; world
callbacks are no longer responsible for persistence progress. Runtime-identity
lookup preserves character lifetime safety when events no longer own the work.

Additional necessary files: `src/net/comm.c` (existing persistence-pulse call
site), `src/core/prototypes.h` (internal declaration), and the existing manual
feedback/deferred-save tests. No storage format, database schema, or terminal
acknowledgement/extraction rule is changed.

`test_manual_save_feedback_contract.py` now compiles the real save-slot code
with controlled clock, snapshot submission, acknowledgement, and character
lookup. No world callback executes. Against original HEAD's `actoth.c`, the
runtime assertion `saves == 1` fails (SIGABRT) after the requested half-second
delay; against the changed source it passes. It also verifies no premature
success, acknowledgement feedback, retry backoff and recovery, an unacknowledged
30-second failure, and character extraction/storage-reuse safety. This is a
behavioral before/after regression, not a CPU-pressure-only check.

The manual regression, deferred retry/flush tests, player-save pipeline
contracts, terminal safety contracts, and formatting check pass. The current
server rebuild and repeated CPU-load journey are in progress. Camp rejection,
induced storage delay, crash-boundary recovery, and full-suite gates remain open.

### First post-correction CPU trial: save fixed, shutdown deadline exposed

With the same two-CPU/four-competitor configuration, the first manual save
completed in 2.276 seconds and camp reached the menu in 3.548 seconds. Revision
48 was captured at 645806348205 us, submitted at 645806845361, completed by the
worker at 645806855370, and consumed at 645807368048. Terminal revision 49 also
received an applied completion with durable revision 49. These are direct
request-to-ACK observations during the startup backlog.

This journey then failed in **shutdown**, not in saving or camping: the
30-second process wait expired and cleanup killed it after five more seconds.
Evidence: `/tmp/duris-save-investigation/cpu/attempt-1-1knyyjyy`. The server
continued processing a large callback backlog without entering its normal
shutdown path. Inspection finds that the game-thread signal consumer calls
`request_shutdown`, which sets `reboot_time=time(0)`; `timedShutdown` then
schedules a one-tick world event before setting `shutdownflag`. That event can
wait behind the same backlog. The retained trace does not separately timestamp
signal receipt or that global callback's scheduling.

`request_shutdown` now selects the already-existing immediate transition with
`reboot_time=0`. It still runs on the game thread, and the existing terminal-save,
worker-drain, and shutdown-cancellation gates remain in place. This affects
launcher-requested transitions; user-scheduled countdown logic is unchanged.
The existing shutdown contract test now checks this path and its lack of a
world-event hop; the actual signal-handler test also passes. Another complete
bounded CPU trial has been started to validate both corrections together.

Earlier checkpoint (superseded by the results below): `make -C src -j2` passed after the immediate-shutdown
correction. The next controlled CPU run is live (log
`/tmp/duris-save-cpu-after-shutdown.log`, exec session 32815). Its first
journey passed both processes, manual saves, camps, and clean shutdowns.
Further repetitions remain in progress; do not count this checkpoint as ten
verified repetitions or as completion of the investigation.

### Ten post-correction CPU journeys passed

The run in `/tmp/duris-save-cpu-after-shutdown.log` exited 0. All ten
journeys passed both boots, saves, camps, restart recovery, and clean shutdowns.
The same CPUs 0/1 and four bounded CPU competitors were used throughout runtime.

| Attempt | First save | First camp | Restart save | Restart camp |
| --- | ---: | ---: | ---: | ---: |
| 1 | 2.282 | 11.648 | 2.277 | 8.124 |
| 2 | 2.279 | 6.061 | 2.257 | 11.581 |
| 3 | 2.291 | 4.056 | 2.284 | 14.688 |
| 4 | 2.282 | 4.572 | 2.252 | 4.558 |
| 5 | 2.275 | 5.581 | 2.267 | 11.621 |
| 6 | 2.276 | 5.077 | 2.267 | 3.024 |
| 7 | 2.287 | 4.046 | 2.272 | 8.100 |
| 8 | 2.276 | 11.138 | 2.265 | 11.661 |
| 9 | 2.279 | 2.041 | 2.255 | 11.544 |
| 10 | 2.257 | 10.608 | 2.292 | 17.720 |

A source/environment/hash-verified optional server cache and an inspection
mode in the existing flat-file repository harness have been added for subsequent
durability experiments. The journey will check the moved item UID against
player and room authority across restart, rather than relying only on names.
A no-added-load journey with these stronger checks is now building; these new
checks were not part of the preceding ten CPU journeys.

### Controlled storage delay exposed stale terminal intent

The strengthened no-added-load journey passed, bringing uncontaminated isolated
coverage to ten journeys (nine original baseline journeys plus this one). Its
repository-level assertions verified that the dropped mace UID moved from player
to room, survived restart there, then moved back to the player without duplicate
ownership. The binary cache subsequently verified and reused the same server.

Holding the synthetic player's `.player-1.lock` reproduced a deliberate terminal
storage timeout. Evidence:
`/tmp/duris-save-investigation/delayed-camp/attempt-1-kf44h6ag` and
`/tmp/duris-save-delayed-camp.log`.

- Camp revision 49 (intent 6, RENT_CAMPED) was submitted at 646848308506 us.
  At 646850305310 it timed out with fence 49 journaled but unacknowledged,
  acknowledged revision 48, inflight revision 49, and no append failures.
- The cancelled camp left the character online: `look` succeeded after releasing
  the lock. Revision 49 completed, and the deferred retry captured revision 50
  with intent 1 (RENT_CRASH) at 646851597824. The repository confirmed revision
  50 durable without another manual save command.
- The subsequent camp reported success, but `terminal_begin` reused revision 50
  with checkpoint outcome 2 (unchanged) and requested intent 6. No new terminal
  snapshot was captured. The authority still contained intent 1.
- On process restart, the player received `Restoring items and pets from crash
  save info...` instead of the camp-return message. The restart assertion failed.

Root cause: a terminal fence survives a timeout and tracks later nonterminal
retry revisions. A later terminal call reused that fence's acknowledged revision,
allowing removal without capturing its current terminal intent/room. Promotion of
an already-pending full nonterminal snapshot has the same intent mismatch risk.
The common coordinator affects both flat-file and MariaDB builds; journal handoff
must likewise correspond to the current terminal request. This demonstrates an
incorrect post-retry logout state, not item loss in this fixture. It does not
prove that the historical PR #151 cancellation had the same initiating cause.

Correction: every terminal call now marks and captures a fresh full revision for
its requested intent and room, resetting its fence's journal/ACK flags. The old
promotion-only journal-tracking table and helper were removed. Deadlines and
successful durability outcomes remain unchanged.

The existing pipeline test now runs the actual terminal coordinator with controlled
worker acknowledgements. The old coordinator fails its captured-intent assertion
at runtime (SIGABRT); the corrected coordinator passes ten repetitions covering
an ACKed retry, a pending full nonterminal save, timeout/retry/subsequent camp,
and MariaDB-style permitted journal handoff. The real delayed-camp journey is
being repeated, with an added direct assertion that a successful camp persists
RENT_CAMPED before restart. Intentional before/after-ACK crash modes also now
check recovery of a changed saved setting and the same item UID.


### Final terminal fix: real retry and crash boundaries

The corrected delayed-camp journey passed both real processes. With the synthetic
player lock held, camp was rejected and the character remained usable. After
release, the automatic retry persisted without another manual command. The next
camp captured a fresh terminal snapshot; final authority was revision 84, intent
6 (camped), room 22800. Restart took the camp path and UID 49770 remained exactly
once across the observed player/floor owners. Ten fresh delayed-camp repetitions
are running; this single pass is not being counted as ten.

Separate intentional SIGKILL trials also passed:

- Before authority write: while holding the player lock, change wimpy from 10
  to 5, request save, and observe accepted worker submission after journal append.
  The authority still held the old setting at termination. A fresh process
  replayed the pending journal and restored wimpy 5, with the mace UID exactly
  once on the floor. This is recovery of journaled, unacknowledged progress; it
  does not promise durability before the journal append.
- After the player-facing save-complete acknowledgement: change wimpy to 5,
  wait for completion, then SIGKILL without camp or clean shutdown. Restart
  preserved wimpy 5 and exactly-once mace ownership. The second process then
  retrieved the same UID, saved, camped, and exited normally.

These crash runs used the verified current binary cache and ran concurrently
with broad regression/database validation, not the controlled CPU-only setup.
Logs: `/tmp/duris-save-crash-before.log`, `/tmp/duris-save-crash-after.log`, and
`/tmp/duris-save-delayed-camp-fixed.log`. Fault controls are opt-in; ordinary
full-suite journeys retain clean shutdowns and their original observation limits.

### Database gate fixture finding

The first `make test-db` reached runtime compatibility and failed because its
fixture applies/records migrations only through 0008, while the checked-in
manifest requires 0009. Both head and applied-count checks correctly rejected
that fixture. This mismatch predates the save changes. The fixture now applies,
verifies, and records the existing immutable 0009 using the surrounding pattern;
no schema definitions, manifests, or live database were changed. The repeated
runtime-compatibility check passes on MySQL 8; the full database gate is continuing.
The first full suite passed 411/411 in 294.49 seconds at `-j4 TEST_JOBS=4`.


### MariaDB assessment and local database gate

The isolated repository/journal probe passed against MariaDB
`10.11.19-MariaDB-ubu2204`. It linked the real snapshot repository, snapshot codec,
journal, and persistence observability code, using a disposable loopback Docker
container and a private temporary journal. A status snapshot (revision 51, camp
intent 6, room 22800, wimpy 5) was appended while the database remained at revision
50, room 777, wimpy 10. Closing/reopening the journal and replaying through the
actual MariaDB repository committed revision 51 and the expected room/setting.
Reapplying revision 51 was classified already-applied; revision 50 was classified
stale and did not overwrite state. This complements the actual terminal
coordinator's controlled journal-handoff regression. The SQL backend does not
store the flat-file `save_intent` field directly; intent also affects capture
(e.g. pet inclusion), while room is persisted in `player_data.last_room`.

The probe is a repository/journal test, not a full MariaDB game/login or database
crash test. It does not establish power-loss durability for a deployed database.
The common scheduler/coordinator fixes apply to both builds; MariaDB's permitted
journal-only terminal success remains distinct from the flat-file requirement
for authority acknowledgement. No real player data or configured application
database was used. Reproduction driver: `/tmp/duris_save_mariadb_probe.py`;
result: `/tmp/duris-save-mariadb-probe.log`. Its temporary C++ harness and journal
were cleaned up and its container was removed.

The corrected `make test-db` exited 0. All ten disposable MySQL suites passed,
including the runtime compatibility fixture and the 143-step legacy migration
replay. The full output is `/tmp/duris-save-test-db-fixed.log`. The original
failure is retained in `/tmp/duris-save-test-db.log` rather than hidden by a rerun.


### Failure classification and remaining limits

| Observation | Supported explanation | Evidence limit |
| --- | --- | --- |
| Manual save queued but not completed under the controlled two-CPU load | Snapshot initiation and manual status polling waited for world callbacks behind NPC debt; storage itself was fast in the captured revision | Reproduces the user-visible class; the original PR logs lack a matching server timeline |
| Clean signal shutdown exceeded the test deadline after save/camp passed | Immediate signal handling took an unnecessary timed world-event hop; main-loop immediate shutdown removes it | Captured backlog and source path support this; separate signal-receipt/event-scheduling timestamps were not recorded |
| Forced camp cancellation while player storage lock was held | Terminal authority ACK could not arrive within the existing two-second deadline | Deliberately induced storage delay, not proof of the original PR #151 timeout's initiator |
| Subsequent camp succeeded after automatic nonterminal retry but restarted as crash recovery | The old terminal coordinator reused an ACKed nonterminal revision/fence with stale intent and room | Pre-fix real journey and deterministic coordinator regression both fail; the corrected real journey passes |
| Historical PR #151 initiating cancellation | Open | Original rejection has no retained worker/revision/fence metadata, so slow storage, queue/admission, and other timeout causes cannot be distinguished retroactively |

No timeout was increased and no save failure was accepted as successful logout.
The fixed persistence pulse bypasses world-callback debt but cannot guarantee
responsiveness if the operating system starves the whole main thread. Actual
storage delays can still legitimately cancel camp. The terminal fix requires a
fresh snapshot per terminal request, adding capture/write work when older full
snapshots already exist; this is necessary to persist the current terminal state.
Production behavior and data loss were not tested or inferred from these fixtures.
Opt-in metadata tracing and failure capture remain useful for a future occurrence
of the unresolved historical cancellation class.

Local validation includes the full regression/build target, database target,
`make security-check`, changed-line formatting, and the quality workflow's Python
and shell syntax checks. Hosted CodeQL/Trivy actions were not run and are not
represented by the local security-contract result.


### Three final full suites passed

All three serial post-fix invocations of `make test-all -j4 TEST_JOBS=4 TEST_MATCH=`
exited 0 with 411 passed and zero failed. Wall times including the build wrapper
were 294.49, 312.18, and 293.28 seconds. Logs are
`/tmp/duris-save-full-suite-1.log`, `-2.log`, and `-3.log`; controller outcomes are
in `/tmp/duris-save-full-suites.log`. These runs used all three final C++ fixes
and the strengthened normal full-world UID checks. The database fixture repair
and diagnostic docstring were completed during these runs; neither changes the
server or the ordinary full-world test behavior. The separate database rerun
validated that fixture repair.

`make -C src` subsequently exited 0 with the current native build up to date.
`./scripts/format.sh --check`, `git diff --check`, `make security-check`, and the
quality workflow's Python/shell syntax checks passed. Local validation logs:
`/tmp/duris-save-native-final.log`, `/tmp/duris-save-final-format.log`, and
`/tmp/duris-save-security-check.log`.


### Ten real failed-camp/retry journeys passed

`DURIS_FULL_WORLD_DELAY_CAMP=1 DURIS_FULL_WORLD_REPEATS=10` with the verified
current binary cache exited 0. All ten synthetic player-lock timeouts produced
the expected camp cancellation, left the character usable, and recovered via
automatic nonterminal retry after lock release without another manual save.
Subsequent camp persisted fresh intent 6; every restart used the camp path,
retained the same mace UID exactly once, and finished with that UID on the player.
All twenty processes exited cleanly. No assertion failures occurred.

Save timings below are command-to-completion observations; camp timings are the
successful subsequent quit-to-account-menu observations, not the intentionally
cancelled camp. Retry timing is lock-release to observed durable nonterminal
revision. These trials overlapped full-suite/database validation and are distinct
from the controlled CPU-only trials. Full log:
`/tmp/duris-save-delayed-camp-ten.log`.

| Trial | Save 1 (s) | Camp 1 (s) | Save 2 (s) | Camp 2 (s) | Clean exits | Retry (s) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 2.251 | 15.693 | 2.251 | 17.513 | 2 | 1.821 |
| 2 | 2.251 | 15.691 | 2.251 | 3.002 | 2 | 1.824 |
| 3 | 2.252 | 15.707 | 2.251 | 6.004 | 2 | 1.805 |
| 4 | 2.252 | 15.704 | 2.252 | 2.502 | 2 | 1.813 |
| 5 | 2.252 | 15.709 | 2.251 | 13.008 | 2 | 1.805 |
| 6 | 2.252 | 15.701 | 2.252 | 12.013 | 2 | 1.812 |
| 7 | 2.252 | 15.704 | 2.252 | 9.507 | 2 | 1.811 |
| 8 | 2.252 | 15.715 | 2.252 | 7.506 | 2 | 1.800 |
| 9 | 2.252 | 15.707 | 2.252 | 2.502 | 2 | 1.806 |
| 10 | 2.252 | 15.711 | 2.252 | 16.512 | 2 | 1.801 |


### Fix commits

- `5c7d8b662`: persistence-pulse scheduling/manual feedback, immediate launcher
  shutdown, fresh terminal intent/revision, focused regressions, and opt-in
  full-world diagnostics/fault checks.
- `a06676304`: update the isolated runtime-compatibility fixture through existing
  migration 0009, resolving the database gate's stale metadata.

Both commits are on `investigate/full-world-save` in the isolated worktree.
The original master checkout and its untracked plan are unchanged. No merge,
push, deployment, or production intervention has been performed.


### Measured startup comparison

After the broad runs finished, a separate diagnostic watched the actual
`NEVENT BUDGET`/`NEVENT CATCHUP` records before beginning character creation.
Its bounded criterion was ten seconds with no new positive catch-up debt,
within a maximum observation window of 120 seconds. Debt drained to zero between
bursts, but recurring world activity prevented that continuous quiet interval.
At 120.006 seconds the latest debt was 1,823 callbacks; the driver recorded
`startup_not_settled` and proceeded, rather than claiming an idle world.

The delayed-start journey then passed both processes: first manual save 2.251 s,
first camp 8.506 s, second manual save 2.251 s, second camp 4.003 s, both exits 0,
and exactly-once mace UID ownership through restart. Immediate post-boot runs
also pass (the ten fault journeys' first manual saves were 2.251–2.252 s, before
any injected delay). The comparison shows no extra manual-save delay in this
post-fix sample after waiting through startup; it does not isolate every host
load difference or demonstrate a fully quiescent world. A genuinely quiet-window
comparison remains unavailable under this bounded criterion. No startup sleep
or longer timeout was added to the normal acceptance test.

Diagnostic driver: `/tmp/duris_save_settled_probe.py`; result:
`/tmp/duris-save-settled.log`. Its first driver attempt stopped before launching
a server because it expected one matching boot site rather than two; correcting
the first-site insertion allowed the measured journey above to run. The source
under test and its ordinary behavior were unchanged.


### Ten final committed-source CPU journeys passed

The final run at `a06676304` (including save fix `5c7d8b662`) exited 0. Source diff
was empty; the verified flat-file binary SHA-256 was
`fa0a115cd93e0facd1746c9006b77ec2f80658f82d9c2a6f1cbfb3ca0807cb70`.
The test and four CPU competitors were pinned to CPUs 0/1. Competitors started
only after build/inspector completion, were bounded to 2,400 seconds, and were
terminated/reaped when the ten trials finished. Broad regression/database runs
and the delayed-start experiment had finished before this CPU run. The existing
unrelated host server remained, as in earlier trials.

All ten journeys passed both saves, camps, restart recovery with UID ownership,
and clean exits (twenty processes). Manual acknowledgements were 2.249–2.295 s.
No camp cancellation, observation assertion failure, or shutdown timeout occurred.
This repeats the original controlled CPU setup on the final committed fixes;
it supplements, rather than replaces, the deterministic pre-fix failures.
Log: `/tmp/duris-save-final-cpu-ten.log`.

| Trial | Save 1 (s) | Camp 1 (s) | Save 2 (s) | Camp 2 (s) | Clean exits | Retry (s) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 2.259 | 9.056 | 2.285 | 5.562 | 2 | — |
| 2 | 2.249 | 4.042 | 2.274 | 10.636 | 2 | — |
| 3 | 2.276 | 14.676 | 2.265 | 6.570 | 2 | — |
| 4 | 2.280 | 17.708 | 2.263 | 9.601 | 2 | — |
| 5 | 2.295 | 4.047 | 2.284 | 4.552 | 2 | — |
| 6 | 2.295 | 4.546 | 2.288 | 8.578 | 2 | — |
| 7 | 2.287 | 7.596 | 2.268 | 11.663 | 2 | — |
| 8 | 2.280 | 6.088 | 2.252 | 8.074 | 2 | — |
| 9 | 2.291 | 12.144 | 2.276 | 3.038 | 2 | — |
| 10 | 2.282 | 3.049 | 2.264 | 0.509 | 2 | — |


### Startup backlog drained: direct comparison

A second isolated diagnostic used the narrower measured boundary relevant to
startup backlog: observe positive catch-up debt, then begin character creation
only after that debt reaches zero. This happened 0.501 s after game-loop readiness
(debt 89 drained to zero by pulse 2). This is a measured cleared startup backlog,
not a claim that normal recurring world callbacks stop.

That complete journey passed: manual saves 2.252/2.252 s, camps 9.506/13.511 s,
two clean exits, and correct UID ownership through restart. Compared with the
immediate-start fixed-code trials, waiting for the initial backlog to drain did
not change observed manual-save latency in this sample. The earlier 120-second
experiment remains useful evidence that a continuously quiet world is a stronger,
unmet condition; it is not a prerequisite for save progress.

The one-off driver inserts measurement before the existing test's first boot
journey; it does not change normal test waits or production scheduling. Driver:
`/tmp/duris_save_drained_probe.py`; output: `/tmp/duris-save-drained.log`. This run
began after all final CPU competitors were reaped.


### Original-binary baseline accounting closed

The initial baseline had nine uncontaminated runs; its third attempt briefly
overlapped a diagnostic build and remains excluded. The later isolated UID
journey used corrected code, so it is not counted toward ten original-binary
baseline runs. A final separate journey reused the original baseline binary only
after verifying SHA-256
`9a5b7aa135aeeff41690efa70988608aa4802789aa5d66922faa7dacde7e15ca`
(source `0ae67e781`). It ran after all controlled load and other test jobs ended.
Both processes passed: first save 16.260 s, first camp 15.512 s, restart save
4.003 s, restart camp 1.751 s, with two clean exits and correct mace UID ownership.
This supplies the tenth uncontaminated original-binary baseline journey without
relabeling the earlier corrected-code run. Log:
`/tmp/duris-save-baseline-makeup.log`; driver:
`/tmp/duris_save_baseline_makeup.py`. Its configuration identifies the current
helper checkout; the explicit `original_baseline_reused` record identifies the
actual server source and verified binary separately.

## Completion audit

| Requirement | Verified evidence |
| --- | --- |
| Separate checkout, synthetic data, recorded environment | Base `0ae67e781`, isolated worktree/branch, explicit flat-file fixture environment, compiler/CPU/memory/scheduler details above; original master preserved |
| Preserve and classify failures | Opt-in failure bundles exercised; original and reproduced failures distinguished by phase, rejection, and observation deadline; no runtime payloads committed |
| Ten isolated baseline journeys | Nine uncontaminated initial runs plus the hash-verified original-binary makeup run, all passed |
| Controlled load reproduction and ten final repetitions | Original pinned two-CPU/four-competitor failure captured; final committed-source run passed all ten journeys with twenty clean exits |
| Startup comparison and build/runtime separation | Verified source/binary cache; competitors start after compilation; both bounded quiet-window and observed positive-to-zero backlog experiments recorded |
| Trace request through durability and completion | Captured client send/response brackets, scheduler due/execution timing, snapshot revision, journal eligibility, worker start/completion, pipeline consumption, terminal timeout/fence, and retry records; backend durability boundaries inspected |
| Competing explanations | Captured fast-storage/manual-initiation starvation; separately forced blocked storage and stale terminal fence; fixture isolation, exact response matching, queue/error metadata and limits documented |
| Failed camp preserves usability and retries without manual save | Ten real locked-player-storage trials passed cancellation, look, automatic durable retry, subsequent fresh-intent camp, and restart |
| UID ownership and crash boundaries | Exactly-once observed player/floor ownership and changed saved setting survived clean restart and separate before-authority/after-ACK SIGKILL journeys |
| MariaDB assessment | Shared terminal coordinator journal-handoff regression plus real MariaDB 10.11.19 repository/journal reopen/replay and duplicate/stale revision checks; full-game and power-loss limits stated |
| Deterministic pre-fix failure/post-fix success | Actual scheduler and terminal coordinator harnesses fail against old code and pass corrected code; ten terminal-intent repetitions; signal-path and terminal safety checks pass |
| Three full suites at original concurrency | Three `make test-all -j4 TEST_JOBS=4 TEST_MATCH=` runs: 411/411 each |
| Native build, formatting, local CI/database gates | `make -C src`, changed-line formatting, focused regressions, quality syntax checks, `make security-check`, and corrected ten-suite `make test-db` passed |
| Fix and evidence deliverables | Fix `5c7d8b662`, fixture correction `a06676304`, and this durable investigation record; no merge/push/deployment performed |

The historical PR #151 initiating cancellation remains explicitly open for lack
of its original server-side timeline. A continuously quiet world and deployed
power-loss durability were not established. Those limits are not recast as
successful tests or as proof that every historical failure has the same cause.
All requested bounded investigation and validation work is complete.

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

## Independent SQL participants

The retained-death writer and recovery-apply participant now borrow an enabled
caller's scope before codecs/native SQL. They acquire no new claims. Held entry
returns the existing deferred or failed/EAGAIN result; unavailable admission
returns retryable or failed/ENOMEM. Recovery refusal leaves the caller cleanup
output untouched; an admitted permit outlives its cleanup owner. Disabled-epoch
bodies remain unchanged. Outer recovery/death-conflict ownership through transaction,
uncertain-lease disposal and journal resolution is still required.

Maintainer implementation and independent architect source review accepted C pin
`798ec2f6fa6807c9542681da8e34a9451f317c662254a5c957f83873e6af0c19`.
The preserved BEFORE C pin is
`273e7364edce38876c6109844decc3cbc1a5a6f73e0cafb2f89b199550a7fbbc`.
Formatting/diff hygiene only; no compiler, tests, native SQL or qualified push.

## Outer SQL mutation boundaries

Death-conflict retain, borrowed apply and pooled apply now require an enabled
caller scope before native work. The pooled permit outlives writer release,
lease retirement, replacement/readback and lease destruction. Quarantine SQL
prepare/resume similarly borrow before transaction/capture or cleanup-output
reset, retaining admission through replacement, verification, journal resolution
and phase cleanup. The public recovery receipt write is also guarded; reads
remain unchanged. None of these functions self-acquires a new resident owner.

Source-established gap: these independently callable owners could begin native
work before a lower participant refused. Pre-entry refusal now leaves cleanup
evidence intact. Existing typed held/unavailable results are preserved; bool
refusal can still allocate its error string. Borrowed callers must retain their
existing scope through subsequent lease disposal. Disabled paths stay unchanged.

Maintainer owned death-conflict C; primary owned quarantine C/H. Independent
architect accepted all three final source pins with no bounded blocker:
death C `f21e4530741caa532381ffa39f45bbc58eb4dde6befc7182f32ad6217d38aadd`,
quarantine C `545b74212bd9fb65d43d28922fcc482854ea7e8885ad5301a8e60216f4620e7e`,
quarantine H `8d3ce971f9cf6054a686f2adea57cab4a0260472e6d15c5b4edc4619daf8cf4e`.
Private BEFORE source copies are preserved at source base `c30a97622`.
Formatting/diff hygiene only; no compiler/tests/native SQL or qualified push.
Actual caller residence, serial lifecycle, other native writers, complete clean
census, critical ACK, production startup/revisit and major-plan gates remain open.

## Positive-PID synchronous save owner

Source tracing found the synchronous `writeCharacter` path changed money and
equipment before `sql_save_player` began its transaction, and its failed legacy
rollback could clear the local flag without native idle proof. The owner now
retains a resident claim, explicit execution scope and nested permit before that
pre-save work, through native SQL, existing inventory restoration and post-save
hooks. The raw pet participant also borrows the same scope before native mutation.

The outer owner finishes exact-main-session cleanup before its permit/scope/claim
unwind. Literal rollback and same-session idle checks do not trust the old local
transaction flag. Unconfirmed cleanup closes the exact main connection and
latches existing runtime exclusion loss, retaining the separate lifecycle control
and pool. It introduces no reconnect or new recovery framework. A master-save
exception follows the existing failed-source restoration tail; uncertain partial
gameplay tails poison admission and cannot claim complete restoration.

Enabled PID-zero creation, caller-owned transactions, direct migration without a
scope and independent component/locker writers remain incomplete paths. The healthy
inactive body remains the legacy path. Source review by the maintainer and architect
accepted this bounded owner; formatting and diff hygiene passed. No compiler,
tests, native SQL/gameplay/persistence/recovery checks or qualified push ran.
Maintainer preserved the four original source files under
`tmp/synchronous-save-boundary-before-v1.local`; this is not a compiler closure.
Plan1 remains unfinished and ownership is still disabled.

## Restored ordinary-drop production integration

The actual SQL boot path now prepares saves, restores critical commands and their
exact immutable drop holds, then starts save execution. Critical initialization
failure keeps preparation closed. Flatfile immediate startup and healthy inactive
schema1 routes retain their existing path. Movement completion/retry dispatch now
recognizes the validated restored SQL ordinary drop and runs it without an actor,
before generic registry/callback handling.

One private original-slot owner binds frozen command, semantic completion, PID,
hold generation and ownership epoch. It reserves exclusion while the hold remains
installed, grants no save execution scope/permit, and refuses resident/queued/
inflight/worker/revision obligations. Fresh active journal and validated control
file fingerprints include archive, policy, legacy quarantine and recovery evidence;
any affected unresolved frame/history remains held, never declared obsolete from
the drop result. Control digest allocation precedes archive publication.

The owner calls the existing fresh native graph publication helper, requires its
confirmed cleanup, repeats the journal census and supplies a nonconstructible
capability to the coordinator. Coordinator ACK checks original frozen command,
sealed result and its own lifetime before durable critical checkpoint. ID-only ACK
and ID-only release cannot clear a restored obligation. Guarded ACK retains
lifecycle exclusion through exact hold consumption outside coordinator locks,
then emits the exact worker wake and a retained reserved-journal revisit notice.
Same-thread lifecycle draining can finish already accepted work; foreign lifecycle
ownership, shutdown and cutover cannot cross the ACK/consumption interval.

The dispatcher observes release sequence before selecting notices, tracks actual
remaining replay-eligible journal PIDs, and revisits only `replay_deferred` work
once original owners release. Replay itself reacquires reservations and rereads
frames. IO/corruption failures do not gain automatic retry authority. Ownership
integrity failure closes save admission and the load gate. Source reviewers found
and closed a missed-notice sleep race and post-durable fingerprint allocation gap.

This is connected production source, **not completed ordinary-drop recovery**.
Production ownership enable remains absent until all native writers and serialized
shutdown/census are covered. Overlapping retained saves need their legitimate
native disposition; rejected receipts need verified no-publication completion.
Activity-bearing ordinary graph coverage, original live producer integration,
native qualification/cold restarts/copyover and full Plan1 gates remain open.
No compiler, AST, tests, services, native SQL, gameplay or recovery checks ran.
Source review/formatting/diff hygiene passed; the user-requested major-plan test
batch remains deferred. No new fixture/report family or qualified push was added.

## Covered ordinary-save ordering

At `2d6111f8b`, the restored publisher refused every overlapping save-journal
frame, including ordinary saves already superseded by native `save_revision`.
This is a source-established missing connection, not a new runtime RED result.
The economy contract keeps ordinary projections separate from economic receipts;
existing stale-save retirement supplies the required policy without a new ledger.

The restored owner now collects affected originals under its exact held-PID
reservation, reads locked native `player_data.save_revision` on one pooled
session, and confirms rollback/idle cleanup before issuing private coverage proof.
Journal retirement freshly matches encoded originals and removes only receipt-free
ordinary frames covered by that revision. Uncovered, operation-bearing and
unrelated frames remain intact. Fresh namespace/custody publication and guarded
critical ACK still follow. An empty-target census syncs the directory to settle
an earlier compaction whose rename succeeded but directory sync failed.

The five changed source files are repository C/H, journal C/H and pipeline C.
Source review, changed-line formatting and diff hygiene passed. No schema, new
economic operation, fixture family or acceptance gate was added. No compiler,
tests, SQL, services, gameplay or recovery checks ran; qualification stays deferred
to major-plan readiness. Ownership remains disabled and Plan 1 remains unfinished.

## Unsupported maintenance refusal

`c542a2642` closes source-established native ownership bypasses in character
deletion, account deletion and pwipe. Enabled ownership refuses before native
mutation; the account confirmation also refuses before installing its deletion
fence or closing sessions. An already-fenced request retains its state. Epoch-zero
behavior remains unchanged. This supplies Plan 1's permitted unsupported-route
behavior, not R8's complete economic deletion/reset implementation. Source review,
changed-line formatting and diff hygiene passed; milestone tests remain deferred.

## Live ordinary-drop producer integration

The actual ordinary SQL drop previously supplied only a void physical callback
and did not consume the full-literal inventory checkpoint. Generic movement could
advance the runtime registry before proving live placement. These are source-
established missing production connections; no new runtime RED was executed.

The active, already-authoritative single-root drop now owns a bounded preparation
entry containing identities, destination and notification bytes. It captures the
literal inventory, waits for its native acknowledgement and recaptures the complete
root before admission. Canonical command storage and pending-entry allocation
precede binding the original checkpoint hold and coordinator publication submission.
No character/object pointer is retained across preparation pulses. The game loop
drives preparation before completion delivery. Inactive behavior and unsupported
locker, bulk, money, corpse and unowned-root routes retain their existing behavior.

Specialized publication precedes generic registry application. The existing native
owner freshly locks current custody and literal room payloads and verifies the
original receipt. Its complete physical/runtime census proves the carried graph
before calling the existing unlink and placement handlers. Retained handler stages
avoid repeated effects after completed departure or placement; detached/placed
retries can proceed without an actor. Unconfirmed handler completion remains held.
Runtime publication uses the existing atomic hydration operation and an existing
source-owner counter key, followed by fresh exact target verification and confirmed
same-session SQL cleanup. Ordinary older per-item source owner revisions remain
valid under their original owner counter, including moving two separate roots.

The movement owner retains the original command, receipt, checkpoint and operation
through publication, ACK and exact hold release. A successful ACK is not repeated
if release must retry. Definitive admission refusal can retry release alone;
uncertain admission retains the original. Post-ACK callbacks only notify and write
existing floor hints; they do not unlink, place or change custody. Deterministic
placement preflight does not consume the native falling RNG.

The six production source files are actobj C, movement C/H, ordinary-drop recovery
C/H and comm C. Independent source review, changed-line formatting and diff hygiene
passed. No compiler, tests, SQL, gameplay, persistence or recovery checks ran.
Maintained link closures and actual native/player journeys remain for the major-
plan qualification batch. This source slice is unqualified; no new schema,
framework, fixture family or acceptance gate was added. Ownership remains disabled;
clean lifecycle closure, supported native metadata ownership, authentic cutover,
flatfile parity and Plan 1 acceptance remain open.

## Direct login metadata ownership

`sql_save_player_core` is called after the master save owner has returned during
login and modification. Its independent native player/account/frag projections
previously lacked an enabled PID residence and exact main-session cleanup owner.
The source now borrows existing resident claim, execution scope and permit around
that native body; confirmed cleanup or original-session retirement precedes every
owner's destruction, including exceptions. PID-zero creation, caller-owned
transactions, non-game-thread writes, unsafe main sessions and held saves refuse.
The historical same-name update affecting other PIDs is skipped in enabled mode.

Renaming is an unsupported enabled operation and now refuses at both the command
facade and SQL entry, before the trusted declined-name filesystem unlink or any
native/identity change. Epoch-zero behavior retains the original body. The three
changed source files are sql C, sql_player C and modify C. Independent source
review, changed-line formatting and diff hygiene passed. This is source-only;
native/compiler/player/recovery checks remain deferred to major-plan readiness.
No receipt, schema, fixture, framework or acceptance gate was added. Production
ownership enable, clean lifecycle close and Plan 1 acceptance remain unfinished.

## Owned lifecycle drain and close

Enabled pipeline shutdown previously joined execution and always retained an
incomplete epoch; its append-only drain did not prove delivery of worker results,
pending native publication or exact hold release. The production critical drain
also skipped its observer when no new result arrived, preventing an owner waiting
for publication from retrying. These are source-established missing lifecycle
connections, not newly executed failure evidence.

The existing coordinator lifecycle guard now exposes a same-thread observation
for the save owner. Enabled critical draining pumps original save completions
before all gameplay publication retries, including empty completion batches.
Unadmitted drop preparations cancel through the existing literal cancellation;
their ordinary save bodies remain owned by pipeline/workers. Admitted or uncertain
originals and held tokens remain intact. Epoch-zero draining keeps its old behavior.

A lifecycle-specific full drain leaves append-only `player_save_pipeline_drain`
unchanged. Within the original caller's 3,000 ms budget, it proves empty pipeline
queues/inflight/death/literal owners, actual worker slots/results/ready containers,
critical work and the exact epoch's claims/permits/holds. It freshly reuses existing
archive/policy/legacy control fingerprints and the complete active journal scanner;
archive bytes include recovery records. Retained quarantine/policy originals remain
on disk. Existing enabled missing-file refusal still applies; no extra file-proof
format or acceptance gate was added. A failed pre-stop drain can explicitly resume
the existing running process before destructive teardown.

Final closure runs after teardown tails that may attempt saves. The worker's idle
check and stop admission share its mutex; joins occur outside owner locks. The
pipeline joins its dispatcher, stops idle workers, repeats the census, and ends
the same epoch before journal metadata cleanup. Closed save/load admission persists
through epoch end and repeated cleanup. Direct core metadata writes consult that
admission while gameplay authority remains active. A late close refusal exits
uncleanly before normal dependency teardown or global destructors, preserving
durable originals for cold recovery; it does not report clean release. Enabled
copyover explicitly refuses before its flush/serialization effects pending its
required integration. Inactive shutdown/copyover retain their existing behavior.

The thirteen source files are pipeline C/H, worker C/H, replay ownership H,
coordinator C/H, journal C/H, movement C/H, comm C and sql C. Independent source
review, changed-line formatting and diff hygiene passed. No compiler/tests/native
SQL/player/persistence/recovery checks ran; qualification remains deferred until
major-plan readiness. Ownership enable and Plan 1 acceptance remain unfinished.
The existing native lifecycle harness already composes install, incomplete-evidence
refusal, activate, pause and reactivate for independent acceptance; production's
stopped-runtime maintenance caller and full release coverage remain separate work.

## Selected-authority ownership boot connection

The production SQL boot previously prepared save recovery, restored critical
commands and started saves without calling the existing ownership-epoch primitive.
That left the completed residence/reservation connections disabled. This is a
source-established missing caller; runtime failure/fix checks remain deferred.

SQL startup now enables the existing epoch only when runtime recovery has already
selected verified active gameplay authority. Journal preparation and bounded
resolved-recovery revalidation finish at epoch zero; unresolved PIDs retain their
existing fences. The epoch begins before critical outbox/replay initialization,
original hold installation and ordinary worker/dispatcher start. Active failure
at preparation, epoch begin, critical recovery or save execution exits before
gameplay or dependent cleanup. Existing ancillary joinable threads require the
same terminal process boundary already used for unresolved owned shutdown.

Inactive SQL and flatfile startup retain their existing behavior. No setting,
activation decision, maintenance write, journal format or new authority mechanism
was added. Header comments now describe the connected caller. Before source is
preserved in `tmp/ownership-boot-before-20261004/comm.c`. Independent source review,
changed-line formatting and diff hygiene passed. This source connection is
unqualified; no compiler/tests/native SQL/gameplay/recovery ran and no production
accounting was activated. Rejected restored outcomes, complete supported graph
recovery, stopped-runtime maintenance, flatfile parity and major-plan acceptance
remain open; all applicable R1–R8 gates are retained.

## Definitive rejected ordinary-drop disposition

The original recovery owner accepted only successful ordinary-drop results, so a
genuine retained rejection could not discharge its restored hold. The private
coordinator ACK also accepted only success. Live rejected drops previously skipped
native proof. These are source-established missing paths; no new runtime RED ran.

The existing receipt verifier now returns a canonical original rejection only
after checking exact committed inbox identity, command/key hashes, result code,
failure stage and retained body, the rejected accounting root and empty native
movement history, room literals and outbox. The native observation separately
locks the original lineage, season, source-owner row and complete source custody/literals
on one reconnect-disabled session. It accepts an unchanged offline source without
materializing it, or an exact complete carried projection; partial, misplaced,
foreign or conflicting projections remain held. Existing owner counters may advance
for other roots, while exact item revisions/topology and literals must agree.
Existing zero-revision owner rows are valid; missing owner rows remain unavailable.
The destination row is not required for rejection: a first-use room may still
lack it when an earlier collector preflight refuses before the native transfer.
No destination row is created or changed merely to settle that rejection.

Rejection invokes no object handlers, enrollment or movement. Live partial-handler
state refuses rejection. Confirmed original-session cleanup precedes guarded ACK.
The restored owner retains its fresh journal/control census and exact publication
reservation; coordinator compares the whole original command/result and preserves
generation, durable checkpoint and exact hold-release ordering. Live ACK/release
retries retain the original token and do not repeat a successful ACK. Restored
health records rejection rather than success; notification remains after ACK.

The seven changed source files are recovery C/H, movement C, repository C/H,
pipeline C and coordinator C. Original sources are retained in
`tmp/rejected-ordinary-drop-before-20261004` and
`tmp/rejected-drop-proof-before-20261004`. Independent source review, changed-line
formatting and diff hygiene passed. No compiler/tests/native SQL/gameplay/recovery
ran. This slice remains unqualified; supported graph admission/recovery alignment
and major-plan acceptance remain open. No new schema, authority framework or
acceptance gate was added; inactive paths retain their behavior.

## Ordinary-drop admission and recovery eligibility

Live ordinary SQL drop admission previously accepted literal graphs that the
existing all-absent recovery owner could not enroll. The shared bounded classifier
now reuses that owner's existing prototype, activity, parent and inert-literal
checks before literal checkpoint preparation, before hold/submission and before
cold enrollment. Unsupported graphs refuse before mutation. The depth comparison
matches the existing capture/codec limit; source identity and canonical bytes
remain caller duties. Existing-graph observation and rejection behavior are unchanged.

The three source files are recovery C/H and movement C. Original copies remain in
`tmp/ordinary-drop-eligibility-before-20261004`. Final source pins are
`c897665e29b8f7dfb79a2ced7df7b74da71d355ae50e2e2ea1a192aa312397e4`,
`d2fd723751acac041873624fbf3a43d33f6b286533975f831a425abe24d6cca7` and
`dd24c1217387f8c89088e5631838195735474cd849ab8b9239437ad506f6ca0a`, respectively.
Independent source review, changed-line formatting and diff hygiene passed.
No tests, compiler, native SQL, gameplay or recovery checks have run for this slice.

This closes the identified production source prerequisite for Plan 1's existing
independent qualification batch. The primary will now qualify the combined source
using existing owners and both builds, with real coordinator/pool lost-reply and
restart checks, flatfile bank/item parity and the guarded lifecycle procedure.
It adds no new framework, activity support or acceptance gate. Unsupported broader
routes remain required in their owning plans; full R1–R8 and release remain open.

## Plan 1 qualification: strict loader compile correction

The first native strict SQL and flatfile builds both reproduced narrowing of the
recovery-template read error's conditional `EIO`/`EILSEQ` value into its existing
unsigned error field. An explicit unsigned conversion preserves those outcomes
and removes the diagnostic without changing warnings, parser behavior or limits.
The original source is `tmp/plan1-template-loader-before-20261004.local.c`.

Both corrected production builds pass in the combined working source:
SQL 142.924 seconds and flatfile 12.268 seconds, each within the original 600-second
build limit with two jobs. `tmp/plan1-production-builds-20261004-v4.local.json`
binds logs, binaries and immutable native source manifest
`6cbe982a58d0f84bb7194cc51cebf8ebe8e5ee75e34274f0d4b453bab98517f8`.
Earlier failed attempts remain intact. These results qualify the combined source
bytes for compilation, not intermediate Git heads or native gameplay/release.
Other diagnosed compile corrections are recorded separately below. Plan 1's
native, publication, persistence and recovery qualification continues.

## Plan 1 qualification: live drop projection field correction

The SQL build reproduced a nonexistent `vnum` access on the custody identity
record in live publication. Runtime projection now takes it from the corresponding
already-verified durable literal, matching the existing enrollment owner. The
preceding graph comparison proves equal bounded cardinalities and identity/literal
ordering before this indexed access. No identity, revision or publication rule changes.
Original source remains in `tmp/plan1-live-projection-before-20261004.local.c`.
The V4 combined-source strict builds above pass; actual native publication/restart
qualification remains open. This commit records only this diagnosed compile issue.

## Plan 1 qualification: coin account accessor correction

The flatfile strict build reproduced an undefined account-name helper in coin
body identity comparison. It now calls the existing `get_account_name_safe`
accessor already used by this transaction module. Missing disconnected account
identity retains the existing conservative mismatch instead of inventing identity.
PID, racewar, account comparison and receipt/publication rules remain unchanged.
Original source remains in `tmp/plan1-coin-account-before-20261004.local.c`.
Both V4 combined-source strict builds pass. Coin publication/native recovery
checks continue; this compiler fix does not deliver Plan 2's missing cold pile owner.

## Plan 1 qualification: death-conflict refusal error conversion

The SQL strict build reproduced narrowing of conditional held/unavailable error
codes into the existing death-conflict result's unsigned field. The explicit
conversion preserves `EAGAIN` versus `ENOMEM`, refusal before SQL mutation and
the borrowed caller's cleanup ownership. Original source is preserved in
`tmp/plan1-death-error-before-20261004.local.c`. Both V4 strict builds pass with
the corrected translation unit actually compiled. Native retained-death, cleanup
faults and combined Plan 1 acceptance remain separate ongoing qualification.

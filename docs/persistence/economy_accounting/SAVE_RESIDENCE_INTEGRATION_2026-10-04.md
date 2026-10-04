# Save residence integration — 2026-10-04

Status: **Plan1 original independent acceptance complete within the recorded
source-bound evidence**. This connects replay ownership to save capture, queues,
workers and receipt delivery. Production activation and complete writer coverage
remain blocked; Plans2–4 and combined Plan5 remain unfinished.

Latest result: the maintained actual ordinary-drop producer and two-cold-boot
regression pass on both SQL engines against the current production closure.
Original independent Plan1 acceptance is consolidated at the end and in its plan.
Earlier unfinished statements below retain their dated input-specific context.

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

### Plan 1 qualification: coordinator fixture link closure

The existing coordinator owner initially failed to link an unused private pipeline ACK overload. Its isolated harness now uses function/data sections and linker section collection, preserving the actual coordinator, journal, original assertions and fault cases. The corrected native owner passes on source candidate 9fabe54bb (11.224 seconds); log tmp/plan1-test_critical_command_coordinator-20261004-v3.local.log, SHA256 4936f708bb36644db45d619c7d22231f87e24910cad4b391029f48324beb42d2. This component does not qualify the unused private ACK overload or full gameplay/recovery acceptance.

### Plan 1 qualification: controlled lifecycle release contract

The runtime-owner fixture lacked the current bool release API. Its controlled guard now preserves ownership on release failure, releases once after repair, and makes repeated shutdown idempotent. The original native runtime owner passes (1.566 seconds), including the bounded failure/retry sequence; log tmp/plan1-test_economic_sql_runtime_owner-20261004-v3.local.log, SHA256 d6cce9595cf01f22c4eb3ecc3a1ae4798628f384c9bbb018b1cce60d40c0590e. This is controlled-interface evidence; actual SQL named-lock release and session-loss qualification remain separate.

### Plan 1 qualification: flatfile native admission closure

Both existing flatfile owners now link the real economic baseline codec and adapter required by current admission. The gate harness also discards its unused private pipeline ACK overload; no baseline validation stub or assertion was removed. Native ASan/UBSan runs pass: authority gate 176.892 seconds (log SHA256 44b1af482d7261ea6686ca3a9bea3fb7850bfba248f7d6682d7e3232c507ad9d), actual bank admission/publication restart 205.131 seconds (15099389fc38a5e3738aeee8f88ec4a0b5452559fb41a800ea68be8dd1598cb1). Logs are tmp/plan1-test_economic_accounting_flatfile_gate-20261004-v3.local.log and tmp/plan1-test_economic_flatfile_admission_native-20261004-v3.local.log. Original native cases and runtime limits remain unchanged; this does not establish all writer coverage or release readiness.

### Plan 1 qualification: coin retained-publication fixture contract

The controlled publisher previously counted repeated verification callbacks as repeated effects, and expected a lifetime attempt cap for the schema-2 retained owner. It now models one idempotent physical effect with fresh dependency verification per invocation; existing bounded dispatch/retry cases preserve the original ID and fences until recovery, and failed completed-stage verification prevents ACK. All original 14 scenarios pass in 110.970 seconds with strict C++20/ASan/UBSan and unchanged compile/case limits. Log tmp/plan1-test_coin_publication_ack_retention-20261004-v3.local.log SHA256 25a3b03f4ef9e3ad6a80253401f1a71f95e96d4de6f7787b0ca12860b4ba633b. This controlled callback contract is not actual native room-pile materialization, SQL session cleanup or cold-restart qualification. Legacy inactive retry behavior was not changed.

### Plan 1 qualification: retained worker receipt expectations

The worker fixture now expects the immutable original completion mask after craft-receipt sealing while continuing to require narrowed queued/inflight bookkeeping. Its existing consumed-lease source check matches conditional replacement or acquisition after retirement; null-before-replacement ordering remains checked. The complete existing native queue/receipt owner passes (8.246 seconds), log tmp/plan1-test_player_save_worker-20261004-v4.local.log SHA256 e8471d56a832636a8bd6f5791698ca6f35f2d030dd03b1caebd32c47e6782e3f. Separately, all 35 prepared actual worker/journal guard cases pass in SQL-header and flatfile modes under ASan/UBSan (7.247/6.618 seconds); evidence /opt/duris-plan1-qualification-20261004/guard-{sql,flat}-native-20261004/results.json. Those apply callbacks are controlled, and SQL execution/private publication/production replay remain independent open qualification.

### Plan 1 qualification: existing pipeline fixture boundaries

The source oracle now extracts the exact pulse/drain functions instead of including newly adjacent lifecycle functions. It retains every original no-I/O/deadline/failure/ACK check and includes the existing append-retry drain obligation. Extracted native fixtures use the real resident type and current boundary signatures while retaining their original inactive controlled capture/queue model. The complete owner passes (9.272 seconds), log tmp/plan1-test_player_save_pipeline-20261004-v5.local.log SHA256 b7f2c93f1b700acd1de60eab89f2ef2f5bc6d198ac00c137a0221996e6900228. This does not qualify selected-active startup/private ACK. All seven previously failing component owners now have a passing native rerun; the four other existing components retain their earlier source-bound passes. Actual SQL/recovery/full Plan1 acceptance remains open.

### Plan 1 qualification: real pooled bank reply loss and retained ACK

The existing ambiguous-COMMIT bank case now enters the real coordinator with publication_required, hides exactly one real successful COMMIT reply, and requires replacement-session reconciliation, original-ID conflict refusal, retained restart readback and one publication ACK. Its one native wallet/bank effect, root, inbox/outbox and zero-held-lease assertions remain. The strict native bank binary d9e9119a748808c9ff055d1b55968102018f89d5899d561f62aedf658018e802 passes on MariaDB 10.11.14 and MySQL 8.0.46. Results: /opt/duris-plan1-qualification-20261004/p1-mariadb-1f915da1.9zGkki/bank/result.json and p1-mysql-1fcab7d2.TNBkdp/bank/result.json; each owned fixture's schema absence, server stop, identity and port rebind passed. Other SQL families/lifecycle failed separately and remain open; no game-wide or production authority was activated.

### Plan 1 qualification: item topology refusal expectation

The native item owner reproduced the dedicated ITEM_TRANSFER_TOPOLOGY_CARDINALITY
refusal for the existing incomplete root/child handoff. The fixture previously
expected generic EMSGSIZE. It now includes the existing error contract and checks
that exact dedicated result; terminal refusal, all custody assertions and original
cases are preserved. The corrected ASan/UBSan real-pool owner passes on MariaDB
10.11.14 (4 seconds) and MySQL 8.0.46 (5 seconds), within the original 120-second
runtime budget. Binary SHA256:
52216a16da3e72040ba855c3a7519babd6c808e7e85fe9b97d785fe47535578d.
Both native logs hash ab54436bac763fbec2a1705cfbecc601ad8ed6b806a3a7582d23b5881e0a42c1.
Evidence roots: /opt/duris-plan1-qualification-20261004/p1-mariadb-d99e77d6.cmzbPu
and p1-mysql-b96aebe6.taMU9X (item/result.json). The wrapper verified schema removal,
owned process shutdown and both ports reusable. Production bytes remain identical
to the V4 strict-build candidate. This fixture repair does not establish active
ordinary-drop gameplay/restart or complete Plan 1.

### Plan 1 qualification: maintained SQL lifecycle link closure

The second existing lifecycle-owner compiler invocation retained an unused private
pipeline ACK overload and failed linkage. Its function/data sections and linker
section collection now match the first isolated owner. Actual lifecycle/cutover
implementations, strict compiler and sanitizer flags, all original cases and the
1800-second integration supervision remain unchanged. Both maintained native owners
pass on MariaDB 10.11.14 and MySQL 8.0.46 in the same disposable roots above;
lifecycle-exit.log is zero on both. MariaDB lifecycle.log SHA256:
fd1a0dbbf79e516efe72ff3bc4bf33a5aa5f6c8e2d3e66fc4baccc35fdd0e7d3.
MySQL lifecycle.log SHA256:
2e483abf8762581e81530964aca5dab3cc1c6a9ebcf7c98232982eb514ee4b36.
Native evidence covers existing wallet/bank/keeper baselines, exact replay,
rollback, serialized legacy writers, runtime gates and cutover faults. No stub
qualifies the unused private ACK path. Prepared startup and checked-release owners
still need corrected-expectation reruns; Plan 1 remains incomplete.

### Plan 1 qualification: real-pool coin fixture lifetime

The original coin fixture checked pooled session cleanliness only after its legacy
direct-session crash case deliberately killed a native transaction. The existing
borrowed writer guard could not confirm cleanup on that dead session and correctly
latched SQL admission closed; pool reborrow therefore refused despite a free slot.
The fixture now explicitly finishes its real-pool lifecycle before that legacy
matrix. Every original pooled lease/reborrow/autocommit/shutdown assertion and every
legacy direct fault remain; no exclusion flag, production guard or pool behavior
changed. Failure-only numeric diagnostics retain the original strict assertion.

The corrected strict ASan/UBSan executable passes all original coin cases on
MariaDB 10.11.14 (6 seconds) and MySQL 8.0.46 (9 seconds), each within 120 seconds.
Binary SHA256 e129a9f061a5398fb40c879d778b42ba255a590838b533595bad8696461b9b69;
both native logs e2e4b4f3ee7fbec6dea0248c3225439afdab6e32cde456014ffeddeb6d7136a6.
Evidence roots /opt/duris-plan1-qualification-20261004/p1-mariadb-2a4385a1.joztid
and p1-mysql-b21dae2b.bMpFLQ (coin/result.json). Both verify exact owned identity,
schema absence, process shutdown and reusable ports. V6 all1245 production inputs
match V4; tmp/plan1-real-pool-compile-20261004-v6.local.json binds consumed inputs.
Existing failed V5 results remain preserved. This closes real-pool coin component
qualification, not Plan2 restored physical-pile integration or full Plan1 acceptance.

### Plan 1 qualification: checked native release owner

All22 prepared checked-release cases pass on both native engines in3 seconds each,
within the original120-second aggregate limit. The sole failed wrong-thread guard
fixture had expected idle guard cleanup to reopen coordinator admission. Existing
lease cleanup deliberately leaves admission closed until explicit resume, as the
maintained idle-capability oracle already requires. The corrected private owner
retains wrong-thread zero-SQL, exact origin cleanup retry, retained admission and
transaction rollback's existing automatic-resume assertions. No production change.

Evidence roots /opt/duris-plan1-qualification-20261004/p1r-mariadb-98e8269a.yL51pn
and p1r-mysql-95d07855.EBkLm6 (release/result.json), original cases/order and all
inputs verified unchanged. Binary6dea57735872abf5af5dc3e0ac4527b7725ae8f9c1229d2c804cec935f11ed30;
metadata0f9ab2a108a26c0abd71e1fcc144495e41cb939d200b8e7d685ca9f3de7b289c.
Runtime reports b62050e5f5b324033c201c2de6cac0b0ab160f2cfc45db18669b9205f50d36ba
(MariaDB) and b1bad0d9f8ef47ee1c95ecfcca632db73dd71bc4e24f10fd9b3bd9e8e9f57770
(MySQL). Both owned identities, schema removal, server shutdown and port rebind
are verified. Startup owner remains failed on stale post-start registration and
ID-only release assumptions; its private next candidate is recompiling, with all
previous failed candidates preserved. This result does not qualify the unrelated
startup owner, active ordinary-drop route, Plans2-4 or full release.

### Plan 1 qualification: staged startup and restored-hold owner

All14 original prepared startup/held-save cases now pass on MariaDB10.11.14
(2 seconds) and MySQL8.0.46 (6 seconds), within the original120-second aggregate
limit. The private owner expected unavailable instead of the existing invalid
closed request; it then attempted restore registration after execution started
and treated an ID-only API call as authority to release a restored hold. Those
expectations now follow the existing source: no pre-start mutation, registration
closes after start, original holds survive, unrelated replay progresses and
matching IDs alone cannot authorize release. Every original case remains; no
production API, guard, result code or publication rule changed.

Evidence roots /opt/duris-plan1-qualification-20261004/p1r-mariadb-dcd598e1.Zud0kM
and p1r-mysql-e2f51a8a.KkbIro (pipeline/result.json) verify all14 names/order,
unchanged source/owner/binary inputs and owned cleanup. Binary SHA256:
1121ffae363f6492dad515784e309154217c2436b091c43c1a43eb6c6a2066a7.
Metadata6a767ff72de4c3a000e8713a3ca5b4ce36560831e4400f60b3bc6e273fb8ea79;
original runnerd18e0958fc02332ed11a8360a5a6af136bfe9b4b6f7ea78f0154e33fc2465976.
Runtime reports2e57109b1604292912e0cf46bfd9ca8cd6a676d1e5698ca79d8db7df106a1afa
and1e08b9f558e9055aa9c45cb266c8db17395a0f2e2c7d6d33f91233b9ed195c1e.
Original failures and private corrected inputs remain under prepared-v1 through-v4.
These tests exercise actual pipeline/worker/journal/repository/pool with controlled
world and receipt leaves; they do not prove actual guarded gameplay ACK or cold
native graph reconstruction. Those remain assigned acceptance, together with the
already-prepared18 recovery-session cases on the current production source.

### Plan 1 qualification: native recovery-session owner

All18 original prepared recovery cases now pass on MariaDB10.11.14 (13 seconds)
and MySQL8.0.46 (11 seconds), within the original120-second aggregate bound.
The246.98-second compile stays within300 seconds and binds unchanged production
source9fabe54bb. Cases cover original-session rollback/retirement, replacement
apply, creation proof, inspection's second transaction, journal proof and an
already-durable resolution surviving late lease retirement.

The private fixture needed link closure for its unselected flatfile branch,
cleanup of its synthetic healthy-case active selector, and a caller-owned read
transaction for its two record-bound boot inspections. Flatfile sentinels abort
if unexpectedly selected; they provide no recovery proof. Synthetic cleanup runs
after all healthy native/receipt/journal/pool assertions and retains history.
Record inspection now confirms original-session rollback and idle/reconnect
state before the unchanged complete payload/UID/component/wallet assertion.
All18 cases, fault seams, assertions and original budgets remain. Failed V3–V6
attempts are preserved; no production source, guard or loader behavior changed.

Evidence roots /opt/duris-plan1-qualification-20261004/p1-recovery-v7-mariadb-92b12791.lnAOkb
and p1-recovery-v7-mysql-098e0a9b.G6Bfbn contain recovery/result.json, pinned
native-evidence/results.json and final teardown-result.json. Both final reports
verify owned server identity, schema absence, shutdown and successful port rebind.
Binary15a7cce3658a64a5945d6111a51d19540afb86b8852f4c70779d2f11d8d8e932;
metadata bb328c2acaf1faf9d78577a22890b8c705347d82f13a19dca28d8eb0103ee6d7.
Runtime result hashes9bf2599b00cdfc639dae4b1d4bd8313ac74d693627db548ebb4e8d1ee9145f7d
and5ed16c4e8393d128e19c6688a6f60194323bc47be6dc22c42bd56b4c15e9ab28.

The existing active ordinary-drop journey remains unfinished. V5's general UI
message was not an ownership-cache failure: actual native logs identify
`active_accounting_unsupported`. Prototype48 has `god_bp`; trusted inert staging
correctly refuses procedure-bearing objects. Procedure-free prototype377 has a
byte-identical native record. The private V6 fixture changes only the active
outer reset/query/export prototype, retaining child391, note5, all three actual
UIDs, exact bytes/topology and original bounds. Its changed export helper compiles
in221.13 seconds within300. Production source and refusal guards are unchanged.
V6 reaches actual literal checkpoint, durable journal and native source export
on both engines, then times out waiting for gameplay publication (154/168 seconds,
original outer600). Both owned teardowns verify identity, schema absence, shutdown
and port rebind. Preserved evidence does not yet establish successful COMMIT or
the exact silent publication refusal.
No ACK or cold-boot success is claimed. Prior failed attempts remain preserved. Required
guarded gameplay ACK and two full-world cold boots, Plans2–4, combined Plan5
qualification and full release remain open. No new acceptance gate is added.

Plans2 and4 continue independently while Plan1 qualifies. A complete private
12-file SQL room-coin recovery slice now joins the reviewed native owner, exact
retained repository verifier, original held save registration and guarded ACK
dispatch, including its production Makefile entry. Two review findings were
corrected privately: rejected pickup observes current rather than obsolete pile
authority, and an advanced shared bank projects current locked values while the
historical receipt remains exact. Corrected native owner8cc4b52e is source-reviewed,
uncompiled and unqualified; it is not integrated into production. A two-file
flatfile retained verifier is separately prepared; current-state/publication parity
remains open. No new schema, receipt store or acceptance gate is introduced.

The private collector proposal now has its existing mapped-wallet/bank admission
contract and singleton purchase registration connected, with original listing
binding and the real acceptance time frozen before submission. Read-only review
found no blocker in those six deltas. Its submit/save/ACK and native recovery
contracts remain unimplemented; no Plan4 route or writer coverage is qualified.

Completed recovery-session milestone `b878ae837` and the subsequent independent
Plan5 imports are pushed normally through `c8f593617` to canonical
`Community-Duris/Duris` experimental-accounting. Remote readback confirms that
exact head. This publishes incremental review work; Plan1 and full release remain
incomplete. No private Plan2/4 source or production activation was published.

### Plan 1 qualification: complete-world equipment census and actual ordinary drop

V7 diagnostics proved real ordinary-drop COMMIT and the exact retained receipt
on both SQL engines, but native source observation refused with E2BIG before
placement. V8 located `equipment_slot`: visited=1000001, global_objects=3141,
top_of_world=253260, character_visits=16758, equipment_slots=720561,
nonempty_equipment=47. Empty fixed MAX_WEAR slots exhausted the graph budget.
The production correction skips empty slots before charging the existing
counter. MAX_WEAR, the one-million reference cap and all UID/link/cycle/conflict
checks remain unchanged.

Candidate V9 changes exactly this source among the frozen1248 production inputs.
Normalized source SHA256:
`e4f7edefc1b36bf045e09f9af8d1c7d5bb5034ab7ba234f541fc79885ed6cd50`.
Existing qualified objects were reused; the changed module was forced through
strict SQL and flatfile compilation/linking (31.098/26.473 seconds), within the
original600-second bounds. These are incremental production builds, not new
clean-build claims. Changed-line clang-format-18 and diff checks pass.
Strict binaries: `93307022f7f1e573f5faa760b142fae123e030a0f7c4ca5a230102fcb667af35`
(SQL) and `0db024d3215e791dd21bb1bb80c4c24c5733412fdb043fb884a35d3bd71e5da2`
(flatfile).

The full-world gameplay owner now passes real inactive create/get/save, guarded
disposable baseline installation, selected-active ordinary `do_drop`, literal
checkpoint/source export, durable critical journal, exact native ownership,
accounting and payload proof, real guarded ACK and two full native cold boots
per engine. Original UIDs/topology/literal bytes and durable proof remain unchanged;
Redis is disabled. Its synthetic route manifest is not game-wide coverage.

MariaDB10.11.14 completes in135 seconds; MySQL8.0.46 in169 seconds, within the
original600-second journey limit. Boot/drop/shutdown bounds remain unchanged.
V11 retries only an explicit busy response with journal and literal checkpoint
absent; neither successful final run needed a retry. Earlier V9 MariaDB save
conflict, V9 MySQL shutdown failure, V10 save-pending diagnostic and all V6–V8
failures remain preserved. No failed attempt is relabeled passing.

Observer binary:
`4cfa154fd96d5077790a0c964dc8c3601291aff1b0ce884208fe2af19a5e99fa`
(production census correction, existing SQL boot-observer macro only).
Source exporter: `7be13961f976ddb9cbab032ab6091fd144e745375505b1a5cbef34dc2b6632c8`.
Lifecycle helper:
`dffe44ab362e97a10d48761c5a131feb28d0cc79e87492ccea8898ac7af9935a`.
Helpers retain their independently pinned earlier input closures; they are not
claimed rebuilt against all later source. V11 runner:
`4599df9f7c7665ec2fe304f3e45225b6175ce69b16b5b0d4c6467d8e65f6c74a`;
wrapper: `1a62fd54dbec69752551b4a64d228289c8c40c29df1c50044e843820a9261844`.

Owned roots under `/opt/duris-plan1-qualification-20261004/`:
`p1d-mariadb-c2341407.2Tj6yc`, `p1d-mysql-f2565cab.sT1IMn`.
Each `active_drop/result.json` and final teardown verify owned server identity,
schema absence, process shutdown and port reuse. Result SHA256s:
`5828864a08ec4f4e0dd9f23864aa00af6d78ba5025967b54e7ebf27e34d69479`,
`cb7f28d336dced1b726eb356897129c5120c9a2e3139b9ac748b92ca721cef94`.
Native reports: `source-candidate-v4/tmp/sql-room-recovery-7e9c6120d71f499e`
and `sql-room-recovery-32ed58cca8194719`, qualification hashes
`c264e156a3678e83af5ed5ac0b943f0cb468c64819de1b3f4371725eae85b359`,
`17042be4367f678ccf18ecf7224003c22a9ebd6cc01aa47aa0ce4d51a74d408e`.

This solves the complete-world census issue and required bounded drop route.
Maintain the executed gameplay owner and consolidate original independent Plan1
acceptance next. Plans2–4 source integration, full backend/writer/day-one coverage,
combined Plan5 qualification and activation-owner/verifier integration remain.
Coverage remains false, release blocked and production accounting inactive.
No optional framework or acceptance gate was added.

### Plan1 maintained regression and original independent acceptance

The three maintained owners now retain the executed real producer/literal
checkpoint/guarded ACK/two-cold-boot journey, with the default historical inactive
mode preserved. Explicit native pre-admission busy responses may retry only
before any literal checkpoint or journal admission, within the original60-second
drop bound. Both successful maintained runs needed zero retries. The22 negative
self-tests, Python AST/source checks, unchanged C++ tokens versus the qualified
private proposal, changed-line clang18 fixedpoint and diff hygiene pass.

Exact maintained input SHA256:

- lifecycle CPP: ce7e7ef2e81cd95cfc440830ce14e76aabb6265a92372488b4f4da1acf6752d7.
- room CPP: 18f0631657d610f929fa2e1a956394dc3972853fa4ccdc028b8b48a1dfe61292.
- journey: 0e998c4ce4bfc5dc7696d8fab4bb96ca71695273f767a12902c98fb0941bd7f1.

Formatting changed both CPP byte identities, so both helpers were rebuilt instead
of relabeling prior evidence. The frozen V12 source binds all1248 production
inputs to46e37a1fc, differing from the original V4 source only by the published
equipment-census fix. Manifest SHA256
fe0f87664a55d9e34aa483ce3415887b0a6335d6159043f283712b9ace6d2642.
Strict C++20 ASan/UBSan room compilation passes in152.373 seconds within300;
lifecycle passes in106.431 seconds under its unchanged maintained supervision.
Binary SHA256 room dd5db7419c0e6b63a7ceed0d3fe2fae6941ac2d8d1cfb2a5cfb41399c4fdda78,
lifecycle45483b930b02edb0a26193286681748b3bd128965139f8b19e5e537a0aa2a52b.
Current complete-world SQL observer remains the previously qualified quiet V9
binary4cfa154fd96d5077790a0c964dc8c3601291aff1b0ce884208fe2af19a5e99fa.

V12 runtime preparation failed before gameplay because the new frozen tree
omitted existing areas_mini fixture support; both engine failures and complete
cleanup are retained. V13 support-pin validation refused empty ancillary files
before starting services. V14 pins all supplied ancillary support while retaining
the original nonempty complete-world/migration prerequisites; no code, helper,
world, oracle or runtime bound changed. Wrapper SHA256
20b4955cbd0f0883e7b0d573f86adcc5add3f03de77a3de8c3d9af5ca7d4b0c1.

MariaDB10.11.14 passes in113 seconds and MySQL8.0.46 in154, within600 per engine;
boot120/drop60/stop30/helper120 limits are unchanged. Actual ordinary do_drop,
original three-UID graph, all payload bytes, native custody/ledger/accounting,
guarded journal-zero ACK and two full native cold boots pass without Redis,
replacement UIDs or reseeding. Original schema1/inactive behavior is preserved.
Both task-owned engines pass identity, schema absence, process stop and port reuse.

Evidence roots under /opt/duris-plan1-qualification-20261004:

- p1d-mariadb-08d80251.W7ceqt active_drop/result.json SHA256
  fbe6bb99a5bd4bedf792fe3e15b2e25337a4a17d4dcf8a42dd90de18301cfedf.
- p1d-mysql-c682e2b4.IJSNpS active_drop/result.json SHA256
  6ba7b1583d7b47e86a1e7f21db57b650dbb0db5c407bf7d215fe9908536cf9ae.
- V12 native journey qualifications sql-room-recovery-94c4663b81834dd7 and
  sql-room-recovery-0daac0326ddc4087 hash57160bccb2dbb0fd3eccd2f1680b53df70bda873339464640839cfb611acf6f0
  and38254f4352d71c39453d11873c633277b9d786a07056de77cfbb6953c95103cb.
- Private compact receipt tmp/plan1-maintained-native-v14-20261004.local.json
  and static input receipt tmp/plan1-maintained-drop-integration-static-20261004.local.json.

The owning Plan1 acceptance table maps only the original requirements. Actual
flatfile bank and item native evidence already establishes retained original-ID
receipt/publication, changed-ID refusal, one-effect replay and explicit ACK;
the dispatcher double is separate evidence. Existing real SQL family/lifecycle,
strict builds and current-source startup/recovery evidence retain their exact
source scopes above. This completes the independently deliverable Plan1 authority
procedure. It does not complete Plan2 physical coin producers, other item/domain
writers, Plan5 combined release qualification or production activation ownership.
The synthetic three-route manifest proves only its isolated route. Registry
coverage remains false and release blocked; no R1–R8 gate is waived or added.

### October4 coin/collector source integration; qualification pending

The reviewed31-file integration connects typed physical coin recovery and SQL
collector purchase preparation, immutable original domain state, save holds,
genuine coordinator completions, native publication and reserved ACK. Exact
coin/collector retained proofs coexist in the common repository; no dispatcher
or legacy guard was lost in composition. Flat coin borrowed-lock retained/current
proof and cold room literals, covered flat save revisions and direct execution
permits are connected. Central flat coin admission remains closed, and active
flat bootstrap is still missing; these sources cannot establish flat delivery.

Collector accounted purchases now persist and verify the existing item runtime
payload and properties in the original transaction. Original-only cold registration
precedes replay execution. Actorless publication proves full same-PID body and
UID absence, reconciles only nonphysical caches, and confirms SQL cleanup before
guarded ACK; incomplete materialization stays held. Schema1/inactive branches and
the declined spell path are preserved by source review. Ordinary loader zero-key
transformation remains existing behavior, not a newly promised physical guarantee.

Independent source/interface review and diff hygiene pass. New compiler, native,
gameplay, fault/recovery and backend checks remain deferred to the original
major-plan readiness batches, as requested. Plan1's8586ba589 milestone retains
its frozen input proof; later shared inputs must be qualified in those batches.
Remaining writer/item/domain routes, flat bootstrap, executable registry coverage,
combined Plan5 and activation-owner/release evidence remain required. No R1–R8
gate, native limit, source policy, production activation or optional framework
changes. The lifecycle-origin proposal is separate, reviewed but not integrated.

Private source input receipt is
tmp/accounting-native-slices-integration-20261004.local.json. It records31 exact
file pins from base db682ade5, reviewed composition and qualification limits.
The public source is reviewable; this receipt is not gameplay acceptance.

Changed-line clang18 formatting covers all30 C/C++ source/header owners, with
fixedpoint and unchanged lexical tokens against the reviewed composition. Three
byte identities changed; the source receipt and registry candidate pins were
refreshed. The maintained matrix is regenerated, with coverage false and release
blocked. The 2,836 occurrences include 465 unmapped coordinates after binding
the 12 new scoped occurrences to three source-reviewed recovery routes. All
three remain unqualified on every backend. The global net increase is 18;
coordinate changes are not counts of newly discovered writers, and inventory
is not executable qualification. Historical rebinding and all original route
acceptance remain open.

# Plan 1: authority, admission, and cutover

Start from add-double-entry HEAD 49af585c4. This plan can run while Plans 2-5
are unfinished because it keeps the epoch inactive and uses isolated fixtures.
It owns the common coordinator, SQL/flatfile transaction boundary, lifecycle
owner, and publication contract. See [requirements R1, R6, and R8](../REMAINING_REQUIREMENTS.md).

## Result

A supported schema-2 operation accepted by the coordinator reaches its typed
owner, survives lost replies and restart, and publishes once. Unsupported
families refuse before mutation, including flatfile coin roots until Plan 2
qualifies them.
A guarded maintenance procedure can capture a complete opening witness and
make an activation decision using supplied, verifiable route-coverage evidence.
It cannot activate when another plan's coverage is missing.

## Current source implementation cadence

The user batches testing when each major plan is ready. This plan's source is
ready and its existing qualification batch is underway. Preserve original sources
and regression owners for failure/fix proof. Production source `9fabe54bb` passes
both strict builds; focused components and worker/journal guards pass within
their stated scopes, and real pooled bank passes both SQL engines. Remaining
coin/item/lifecycle reruns and actual publication/recovery still prevent Plan 1
completion. Do not expand its independent acceptance or add optional gates.

Keep implementation within the group's R1–R8 contract and this plan's independent
acceptance. Ordinary save projections do not need an additional economic receipt
or historical snapshot ledger. For a retained, receipt-free ordinary save, the
existing locked native `player_data.save_revision` can establish that the frame
is already obsolete. Use that authority under the original held-PID reservation;
the drop receipt's revision cannot substitute for it. Uncovered saves retain
existing custody/replay checks, and operation-bearing saves retain their existing
receipt requirements. Do not add optional frameworks or release gates for this
ordering case. Prioritize connecting existing owners into the production path.

`c542a2642` supplies enabled pre-mutation refusal for unsupported character/account
deletion and pwipe. `1d6043f57` connects the actual single-root ordinary SQL drop
to its literal checkpoint and native publication owner before generic registry
application. Source review passed; tests remain deferred.
`1192d08c5` now owns direct core metadata and refuses enabled rename before
native/filesystem mutation. `480f20ce3` connects full owned lifecycle drain/close,
retaining the original timeout and closed admission after exact epoch end.
`2078eb7d0` connects ownership enable only after selected active SQL authority and
save preparation/revalidation, before critical replay and save workers. Active
boot failure exits before gameplay; inactive/flatfile behavior remains unchanged.
`cec4bd369` connects definitive rejected ordinary-drop disposition using exact
original no-effect native proof and the existing reserved guarded ACK path.
Both are source-reviewed and unqualified; no production authority was activated.
`97fef7a09` aligns ordinary-graph admission/recovery using the existing eligibility
checks before mutation. Independent source review found no remaining production
blocker to starting this major plan's existing qualification batch. Existing
flatfile bank/item parity and SQL coordinator/pool/lifecycle checks enter that batch;
passing source review does not establish acceptance.

The existing native lifecycle APIs and stopped/disposable harness already compose
guarded install/refusal/activate/pause/reactivate. Independent acceptance requires
that procedure's native checks, including incomplete-manifest refusal; it does not
require a new production CLI or online handover. Actual production activation-owner
and Plan5-verifier integration remain required for full-release R6, along with all
five plans' coverage. They do not create another independent Plan 1 test gate.
Keep work within the original acceptance; add no optional frameworks or gates.

Recovery-session cleanup is implemented in `7dc837e29`: original-session rollback
proof covers replacement apply, creation verification, the inspection loader's
second transaction and journal proof; uncertain boot leases retire. Late session
retirement preserves an already-durable resolution result. No migration or
archive encoding changes. Source review passed its bounded scope; native cases,
both engines, gameplay and recovery remain deferred. Extend the original 13-case
owner with second-phase inspection faults and a late-retirement success oracle.

Named-lock SQL query construction and scalar comparisons now avoid temporary
C++ strings in lifecycle acquisition/destruction and transferred cutover release.
Fixed stack buffers refuse formatting overflow. This bounded allocation change
is unqualified. Tentative GET_LOCK ownership, checked same-session/thread release,
coordinator confirmation, retained runtime ownership and pooled writer retirement
are now implemented in `5834fe57c`, with failed-RPC retry correction in
`55525b981`. Source review does not qualify them. Required native fault checks,
maintained link closure, restored-save integration and full recovery remain open.

## Starting files and first checks

Inspect src/persistence/critical_command_coordinator.c,
src/persistence/critical_command_repository.c,
src/persistence/economic_sql_accounting_lifecycle_transaction.c,
src/sql/sql_economic_runtime.c, and
src/flatfile/flatfile_accounting_dispatch.c. Start with
python3 tests/async/test_economic_accounting_admission.py,
python3 tests/async/test_economic_sql_runtime_owner.py, and
python3 tests/async/test_economic_flatfile_dispatch.py. Use the disposable
SQL lifecycle runner for native database behavior.

## Work

1. Trace coordinator admission, pooled apply, direct apply, and reconcile for
   each already-supported bank, coin, and item envelope. Current source already
   dispatches typed coin/item roots and has coordinator-journal component tests.
   Their SQL pool boundaries use fresh fixture connections. The October 3
   native coin/item cases now hide one successful COMMIT reply at the real
   MySQL client call, require replacement-connection reconciliation and retain
   exact journal/result/ACK assertions on both engines. Production pool
   lifecycle and live publication/restart remain separate open qualification. Retain every unsupported-family refusal. Direct owner calls and
   test ACKs do not qualify the actual gameplay publication path.
2. Centralize the same-root completion checks without replacing the existing
   domain repositories: native before/after effects, canonical intent/plan,
   child receipts and references, source claims, inbox/outbox, and savepoint
   rollback must agree before commit. Reuse the existing EAI1/EAP1 and migration
   0031-0033 contracts; add schema only for a demonstrated missing invariant.
3. Finish the lifecycle owner around the existing boot guard and staged
   wallet/bank/pile baseline: quiescence, pending and unpublished work, complete
   source capture, identity mappings, evidence-loss refusal, exact retry,
   activation/pause state, and restart recovery. The selected active epoch must
   be durable and agree with the process admission cache before gameplay.
4. Prove held publication, offline completion, reconnect, restart, and
   acknowledgement for each admitted family. A committed result must not be
   reported as a failed debit or retried under a new operation ID.
5. Make the already-supported flatfile bank/item owners apply the same
   admission and replay contract, with bounded authority-journal recovery and
   no checkpoint that forgets dedupe. Leave flatfile coin implementation to
   Plan 2 and retain its explicit refusal here.

## Independent acceptance

- Disposable MySQL and MariaDB tests drive schema-2 bank, coin, and item roots
  through the real coordinator and pool. Each proves one native effect, one
  exact accounting root, replay without another effect, conflicting-ID refusal,
  rollback, lost commit reply, and restart read-back.
- A staged baseline still refuses runtime boot; a deliberately incomplete
  route manifest refuses activation. A complete synthetic manifest can be
  tested only in a disposable fixture and never presented as game-wide proof.
- Bank/item command fixtures are accepted or refused by flatfile for the same
  reasons; coin roots refuse there until Plan 2. Native journal interruption
  and recovery retain the original receipt and publication obligation.
- Run focused coordinator/lifecycle tests and both server builds after code
  changes. Do not use a production database.

## Boundary and handoff

Plans 2-4 provide route-specific typed intents and transaction adapters. This
plan provides a narrow documented registration interface and fixture for them;
it does not claim their writers are covered. Plan 5 supplies the final route
manifest and independent audit result. No live epoch is selected until all five
plans pass the release gate.


## October 3 current publication gaps

One ordinary player-to-room drop is the next bounded publication route to
qualify. Current production drop supplies only a void completion callback;
stale live topology returns without a held publication obligation. Replay of
an ordinary retained drop falls back to a blocked generic handler. The existing
recovered callback only checks actor presence, so it must not substitute for
native custody and materialization proof.

Scope the typed repair to an already-authoritative ordinary-room single root
and its complete descendants, retaining original UID graph, room vnum, native
result and operation ID. Before acknowledgement, independently verify exact
owner/root/parent/revision/state and command/result agreement, then establish
one live graph or retain the fence. Stage a missing complete graph before
publication; do not allocate replacement UIDs, overwrite newer custody or
accept a partial descendant set. Prove the exact room payload survives ACK and
two cold restarts on both SQL engines; Redis floor hints alone are insufficient.
Legacy commands lacking sufficient proof remain held. Preserve inactive schema-1
behavior and unsupported active refusals. Locker/bulk/pet/money/corpse/adoption
and peer-give semantics remain separate routes. Source-established gaps are
not yet an executed failing player journey.

Coin retained publication has a concrete source-established retention defect:
`currency_transaction.c::publish_coin` ACKs before the physical callback. A false
callback reinserts the node, but the next ACK refuses the already-erased operation,
so the physical callback cannot retry and its journal/fences are already gone.
Restored item endpoints also have a null callback that currently defaults to
successful publication. The existing restart owner test expects this unsafe
ACK without materializing the room pile. These failures are now reproduced by
a native actual-owner component with a controlled irreversible coordinator ACK;
wallet-only replay is a passing control. The bounded fail-closed retention
repair is in qualification and does not establish actual native pile publication.
SQL wallet-root qualification still excludes item endpoints; preserve that
safety boundary and all inactive schema-1 behavior.

Repair physical verification/publication separately from post-ACK messages and
bulk continuation. Keep explicit physical-ready/ACK-pending state and original
operation busy/lifecycle ownership through durable ACK; ACK retries must not
repeat physical effects. Failed or absent native proof retains the blocked entry,
journal and fences even after hot retries stop. Wallet-only recovery remains a
separate supported projection case. Restored piles need exact original UID,
denominations, native object uniqueness, owner/root/parent and item/owner revision
proof; transient placement known only through callback context remains held.
Qualify physical failure then same-ID success, ACK failure after physical success,
actorless/restarted missing proof, consume/update/create and conflict refusal,
legitimate rejection, lifecycle retention and unchanged wallet-only/inactive
behavior. These open R1/R2/R4/R8 requirements are not waived by the SQL component
fixtures.


## October 3 replacement ownership repair

Native shutdown regression reproduced one abandoned borrowed slot when
replacement returned NULL. The pool now consumes the original lease on every
valid replacement attempt, catches factory exceptions, and closes unpublished
fresh sessions before releasing the reservation to shutdown. Success returns
one new borrowed handle. Foreign/unborrowed handles cannot replace another
lease. Snapshot/death-conflict/locker callers always clear their original
pointer, including failure. Real-session rollback/replenishment/shutdown passes
on both engines; pure pool ordering/capacity checks pass under ASan/UBSan.
Coin/item typed coordinator paths linked to this production pool now pass both
engines under ASan/UBSan, including real COMMIT reply loss, exact replay and
clean reborrow/shutdown. Typed bank now also passes the complete actual-pool component matrix on both
engines, including distinct replacement reconciliation and clean reborrow/shutdown.
Boot configuration, mixed workload and actual publication/replay
acceptance remain independent of this repair.


## October 3 bounded historical room recovery qualification

Migration 0055 now retains exact full-literal schema-2 ordinary-drop payloads
beside native custody, and retires the selected player projection atomically.
Actual production-pool fault/reconciliation checks and two complete-world cold
boots on each SQL engine pass exact native UID, topology, payload and custody
readback without Redis. Unsupported, stale and missing evidence still refuses.
This closes the bounded historical room-payload storage/reconstruction gap.
It does not qualify the ordinary gameplay producer: unstrung objects now have an opt-in scoped full-literal checkpoint component,
but the actual drop producer does not use it. The live callback still needs
retained publication proof, and replay must verify an already restored graph
or retain its obligation. These remain the next R1/R4/R8 work. See the October 3 review
status for pinned evidence and the independent current-head broad-run gate.

## Scoped ordinary-drop owner implementation order

The opt-in literal checkpoint component retains the selected graph's capture
policy through newer ordinary saves/coalescing and requires the exact successful
worker revision, fresh live bytes and a clean revision queue. A process-local
PID/runtime/root/generation lease can bind to one original operation ID. Held
leases retain save, terminal/death and target-login obligations; dirty marks
continue. Immediate runtime-map retirement invalidates unheld callers. The native
inactive component passes actual MySQL/MariaDB capture, pipeline, journal,
repository and production-pool qualification. This is a producer prerequisite;
no existing ordinary-drop gameplay route has been upgraded.

1. Add a bounded preparation owner with identities and immutable terms, not live
   pointers. Validate scope and current accounting admission before requesting
   the literal checkpoint. Preserve the SQL wallet-root exclusion and all
   inactive legacy paths. Preparation contributes command busy state but must
   allow the checkpoint to complete its own capture.
2. Refuse native handler side effects outside the admitted shape: water, falling,
   no-ground, positive fall chance, airborne and transient cases. Inspect all
   selected descendants for action source/adapter references through the now
   qualified read-only `item_actions_object_busy(uid)` predicate. Unknown/off-thread
   queries refuse. `obj_from_char` removes the carrying-list link before its
   departure hook can invoke a resource-changing adapter finish. Query before
   any unlink and again before publication. The drop caller is not wired yet.
   Do not consume randomness or call room-placement helpers as a preflight.
3. After exact database ACK, recapture and compare the complete live graph,
   rebuild/freeze the command and bind the original-operation hold before
   coordinator submission. Proven refusal releases it; journal uncertainty
   retains the original ID and hold. Atomic native SQL source preparation still
   supplies authoritative physical/custody proof; ACK is insufficient by itself.
4. Validate live canonical bytes/topology and original runtime custody before
   advancing the registry. Publish once through a retained boolean owner and
   verify resulting room graph/custody. Release the scoped hold only after
   coordinator publication ACK, in normal and ACK-retry paths, independent of
   whether the original actor still exists.
5. Register schema-2 ordinary-drop obligations during critical durable-command
   replay, then verify cold room hydration before ACK. Verify original UIDs, exact bytes/topology
   and after-custody without moving/advancing them again or repeating messages.
   Missing or conflicting graphs remain held. Process-local tokens cannot be
   reconstructed against a room root: a distinct restored save/lifecycle
   obligation must survive until ACK while permitting authoritative hydration.
6. Qualify the actual `do_drop` route, complete unstrung nested graphs, refusal
   cleanup, coalescing, uncertain submission, post-COMMIT payload drift, ACK
   failure, reconnect/copyover and two cold boots on both engines. Preserve
   broader writer, activation, flatfile and workload gates.


Restored-obligation design review found a separate login boundary: current
`save_admitted(pid)==false` makes account/nanny loads request degraded recovery.
A restored ordinary-drop hold must distinguish clean authoritative hydration
from save/lifecycle admission; reuse of that flag would retain a degraded actor
and can strand the fence after ACK. Preserve genuine quarantine, target, death
and degraded gates. The separate authoritative-hydration API and account/nanny
classification are now implemented and component-qualified on both SQL engines;
production replay registration and ACK callers are still unwired. The restored
room verifier must be explicitly actor independent, and original-operation hold
release belongs after successful publication ACK in both the normal and
`ack_pending` branches, before pending-owner erasure. Notifications must not
hold durable ACK waiting for an absent actor.


Boot/copyover ordering is source verified: player-save initialization precedes
synchronous critical replay, then game-loop/copyover materialization. Held
runtime-zero scopes survive pulse cleanup. Register restored obligations without
requiring save-journal replay completion, but only after pipeline initialization.
The exceptional final teardown now retains dependent save/locker owners if
critical shutdown refuses; native branch and normal boot/shutdown evidence are
recorded in the October 3 review. The bounded restore-registration API now
passes both SQL engines, including delayed save-journal replay and runtime-zero
holds, and original-operation release needs no actor or allocation. These APIs
are not yet called by the production replay/publication owners. Copyover and
actual critical replay/ACK qualification remain open.


## Retained ordinary-drop receipt proof

Successful historical ordinary SQL-drop receipts now require the complete
original canonical literal payload plus native ledger/reference agreement.
Both engines qualify actual repository/pool corruption refusal, original
coordinator fences and journal through retry exhaustion, exact repair/restart,
stale REPEATABLE READ locking reads, and later native custody/season changes.
The original 300-second native gate passes with the same tested executable;
strict production backends and actual inactive creation/save/cold-relog pass.
This is component qualification based on 3e828dc0e, not qualification of the
incoming 5c3bc0957 or physical publication, replay-observer registration,
ordinary-drop gameplay, copyover, or the full R1-R8 release gates. Those owners
remain open. Preserve immutable historical proof and the current admission
exclusions while completing them.


## Definite admission failure cleanup

The real coordinator removes a definitely never-durably-admitted command before
delivering its terminal admission-failure completion. Currency's schema-2 ACK
then cannot succeed and its owner remains busy. Add a trusted explicit
never-admitted disposition or a defined coordinator release contract, rather
than interpreting arbitrary ACK=false as release permission. Prove actual
journal quota/definite append refusal, zero domain applications, exactly one
rejection notification/cleanup and no retained owner; uncertain append and
committed publication remain held. This gap is separate from physical-publication retention. Six actual native
currency/coordinator/journal RED cases now reproduce retained domain owners
after definite refusal, while four uncertainty/durable-rejection controls pass.
The trusted disposition and whole-batch validation repair now passes all 140
actual journal/coordinator/domain-owner sanitizer cases in SQL-header and flatfile
modes, existing native/persistence/restore owners, both strict production builds,
and actual inactive creation/save/restart/relog plus three coin-pickup commands.
Queued/full-delivery fallback and zero output capacity preserve the original
operation until exact-once delivery. Only a validated never-admitted receipt
releases without ACK; every uncertain/executed command keeps ordinary ACK rules.
This is a locally solved bounded cleanup issue, not active/native physical route,
combined incoming-remote or full R1-R8 qualification. See October 3 status for
source/binary/declaration pins and preserved failed setup/driver attempts.

The future physical adapter must stage both pile endpoints before changing
either, adjust the destination command after the source's owner revision, and
validate unique native UID/type/vnum/amount/topology/location plus exact result
revision. Newer metadata or an unloaded player is not native proof. Required
corpse/saved-item projection writes belong to the publication durability proof,
not a cosmetic post-ACK notification. Complete runtime item/owner revision
publication needs an atomic primitive; generic placement/extraction hooks and
fallible live string replacement require separate staged verification.

Separate craft ACK-retry cleanup is solved locally with before/after native
proof: all30 valid before-source cases fail semantically; afterward30 cases per
backend pass1990 combined assertions, including real journal ACK refusal/repair,
progression cleanup, runtime identity, reentrant business hooks and bound receipt
conflicts. The shared finalizer waits for the original durable ACK, extracts
before external hooks and rechecks native actor identity. Physical/progression
effects survive retry; definite never-admission disposes only staged output and
skips progression cleanup. Both strict builds,68 existing admission cases and
eight maintained owner suites pass. Actual inactive mortal Craft/Forge retains
UIDs and exact XP through copyover/two cold restarts; inactive creation/relog and
three pickup controls pass. Component saved() injection is not actual SQL save
completion; arbitrary pre-ACK progression hook throw/reentry is outside this
proof. This does not wire ordinary-drop recovery or qualify active accounting,
combined remote source or full R1-R8 gates. October3 review pins final evidence.

## Restored-drop competing save projection dependency

Read-only repository tracing establishes two distinct current boundaries. Normal
combined inventory/equipment SQL snapshots lock and reconcile custody; a dropped
UID still included in such a snapshot is rejected before DELETE/INSERT and the
transaction rolls back. A newer stale full snapshot can still terminal-fail and
quarantine its PID, so queued/inflight save replay requires actual race tests
before restored-drop production wiring.

The accepted legacy equipment-only/inventory-only path in
player_snapshot_repository.c::apply_items skips that custody reconciliation.
After an ordinary drop retires its player projection, a newer inventory-only
snapshot containing the old UID can reinsert player_items/runtime payload while
native custody remains room-owned and the room sidecar remains intact. This is
a source-established competing projection, not proven sidecar/custody overwrite
or an actual-engine RED. Qualify both engines and add a scoped current-custody
check for legacy partial replay without discarding retained snapshots or loosening
quarantine/target-login/death gates. Background dispatch must defer only affected
PIDs and preserve unrelated progress.

Live literal-hold admission already requires completed replay, idle worker PID,
no append or retained queue, exact current/ACK revisions and no dirty/queued/
inflight masks; a compliant new live drop excludes outstanding newer saves.
Restored obligation registration has different startup ordering, so it cannot
use that live-admission proof. Registering a hold or one-shot boot prehydration
alone does not qualify old journal replay or a drop first applied after boot.

Further source tracing confirms full snapshots' SQL transaction locks serialize
with the drop: save locks player_data then active custody; drop locks season,
owner revisions, custody and physical rows. Equal/older save revisions are no-op,
and newer complete stale snapshots roll back before destructive projection.
No compliant-live-producer overwrite is demonstrated. Legacy partial replacement
must validate incoming ownership AND its deletion/cascade scope; blindly filling
in the missing component would silently change compatibility.

Quarantine from a newer stale full snapshot remains a separate recovery concern.
Direct save-journal startup replay bypasses the worker, while already dispatched
work is outside pipeline queue checks. Before wiring restored-drop recovery,
qualify affected-PID deferral across pipeline, worker and journal replay without
acknowledging/discarding frames or exhausting ordinary retry into quarantine.
An in-flight SQL transaction retains native custody checks; any stricter start/
registration boundary needs defined PID ownership and wakeup without holding
pipeline/worker mutexes across SQL or using coordinator cutover drain. Test both
transaction orders and unrelated-PID progress on MySQL and MariaDB.


Legacy partial replacement also has a source-established destructive scope gap:
children of equipped containers have physical equip_slot0, so inventory-only
DELETE currently removes them while leaving their equipment root. The partial
planner must select complete roots from current custody and verified legacy
slot evidence, validate incoming graph and physical deletion/cascade closure,
and reject cross-PID/duplicate/foreign or incomplete closure before any DELETE.
Keep the untouched component's complete payload and metadata unchanged; do not
construct a missing component. Reuse the current custody diagnoses and preserve
revision/no-op, rollback, inline-coin and inactive legacy equipment rules. New
actual-repository/direct-SQL regression preparation is separate from a genuine
pooled schema-2 drop and two-session lock-order proof. Both engines, journal
exact-frame quarantine/restart and unrelated-PID controls remain required.

Actual before-source partial-save qualification now reproduces all eight semantic
failures on both MySQL8.0.46 and MariaDB10.11.14. Existing full/legacy and three
partial controls pass per engine; true postwrite faults prove STATUS/projection
rollback and unrelated-PID replay progress. Exact declaration/log/source/binary
pins and preserved setup failures are recorded in October3 review. Production
implementation and all after-source checks remain in progress. Direct synthetic
room after-state cannot establish real economic drop/pool serialization; a separate
opt-in real-pool/barrier matrix retains original budgets and awaits execution.

The complete partial replacement prerequisite now passes actual paired engines:
34 direct contracts and51 allocation ordinals each, plus six READ COMMITTED
pooled-drop/direct-save cases each with real1205/1452 witnesses. Strict SQL/flat
builds and inactive SQL save/death/crash/restart/copyover journeys pass. Selected
root forest/cascade bounds, legacy position proof and partial restitution scope
preserve opposite rows; allocation failures return through rollback. See October3
review/declaration71055b7b. This does not connect ordinary-drop native publication,
first-boot/replay/hydration or restored-save deferral, qualify every SQL isolation
or allocation boundary, or complete cutover/activation/R1-R8.

### Restored-save admission and release contract

Current source review identifies four bypasses that must be closed together
before wiring an ordinary-drop recovery caller: startup launches save replay
before critical restoration, durable-ready dispatch ignores restored holds,
workers can already own a snapshot, and journal replay invokes the repository
directly. A bounded per-PID apply permit must serialize restored registration
with execution; registration refuses an active permit. SQL and callbacks run
outside worker, journal and pipeline locks. Use opt-in startup suspension until
the restoration census is complete, preserving default inactive behavior.

Deferral is distinct from retryable failure: park the exact worker request
without completion, ACK, retry increment or quarantine; keep its original
identity through the wake-to-dispatch window while coalescing only pending work.
Selective journal replay must retain every exact frame for the held PID and
exclude it from checkpoint/proof maps while unrelated PIDs progress. Deferred
death/operation-bearing frames require a per-PID load fence; a clean global
replay pass does not establish affected-PID hydration eligibility. The dispatcher
needs an explicit retained wake to revisit deferred frames without spinning.

The worker-only parking primitive has bounded native qualification with actual
worker/revision/codec/journal and a controlled apply callback: eight cases per
SQL-header/flatfile mode pass, along with strict builds and unchanged inactive
SQL/flatfile gameplay. It remains unused by normal pipeline selection and does
not qualify restored-save integration. Its explicit resume returns false before
the callback has parked; the owner must retain that wake for retry. The journal
now has a separate bounded selective-replay prerequisite: a deferred PID retains
every exact frame and loses both prior ordinary and exact operation proofs from
that pass, while unrelated PIDs checkpoint. Its appended replay_deferred result
keeps the existing global replay/load fence closed. Nineteen paired native cases
pass per backend mode with a controlled apply callback; actual restored SQL
ownership is not connected. Collection allocation failure is contained before
callbacks, preserving the original journal.
Its resume allocation guarantee does not repair existing allocation gaps in
retry scheduling or pending promotion. Retained initial/pending admission now
has bounded qualification recorded below; result delivery remains separate.

Before-admission source review established input loss from an already-moved
make_unique argument, stranded post-emplace slots after ready allocation, and
replacement failure after a newer uncaptured mark. Empty job/slot/cancelable
readiness are now staged before moves or revision claims, and exact current
revision/unacknowledged mask is checked before replacement. Sixteen paired before
failures/four controls become20 after passes per native mode, with unchanged8
parking regressions each, both strict incremental builds and maintained owners.
Actual inactive MariaDB/MySQL journeys/follow-ups and flatfile restart/relog pass;
final declaration20a99f56 pins exact evidence in October3 review. This closes
retained admission only. Retry/pending promotion now has separate paired component
qualification:11 before failures/four controls become15 passes per backend,
including real native set/deque growth and quest/spell/craft completion receipts.
Cancelable readiness is staged before result consumption or health/revision/receipt
mutation. Failed allocation retains the exact front for a later pulse; successful
staging makes later queue calls allocation-free. Failed promotion removes only
newly staged readiness. Unchanged20 admission/eight parking regressions also pass
per mode. Declaration36ec8996 pins the seven artifacts and every case log; strict
incremental builds, nine maintained owners,14 validations/30 contracts and matrix
pass. Actual inactive MariaDB/MySQL save/death/crash/copyover journeys and native
follow-ups pass; flatfile creation/save/restart/relog passes133.956 seconds. Final
declaration1c6b2a43 pins source/binaries/logs/cases/owned teardown. This bounded
scheduling prerequisite is solved locally.
Separately, result-queue allocation in worker_main can throw
after journal ACK. Ready-queue repairs cannot qualify completion-delivery safety.

Separate source review identifies unresolved-replay proof retention: an earlier
same-PID ordinary durable revision or exact death/receipt proof remains eligible
when a later callback returns retryable/ambiguous. Withdraw both proof stores
for the failed PID before checkpointing unaffected work. A callback bad_alloc
must remain unresolved, with no proof, rather than create a corruption archive;
execution or COMMIT may already have happened. Genuine runtime, custody and death
failures retain their quarantine rules. This journal-only repair now has paired
native qualification:19 before-source failures and four terminal controls become
23 passing cases per backend mode, with unchanged19-case deferral regressions.
Both incremental strict builds and actual inactive SQL/flatfile gameplay pass;
October3 review and declaration9ef8c4b2 pin the exact evidence. General
postcallback allocation safety remains open. Ordinary SQL exception rollback and
pool-lease cleanup are separate: current ordinary apply can escape with a
transaction or lease still held, including ambiguous-commit readback allocation.

Worker submission and pending promotion also overwrite the mask of an already
journaled typed snapshot after an older component ACK. Apply/ACK then differ from
the exact original bytes, and typed required components can disappear. Keep
sealed apply/ACK identity distinct from revision ownership; never acknowledge
original bytes after applying narrowed data. Reapplying redundant item/pet
components also needs current native authority validation, especially flatfile
which lacks SQL apply-time custody comparison. This newly confirmed dependency
is separate from admission allocation and remains unimplemented/unqualified.

Publication ACK must retain the original operation until a complete affected-PID
save census is clean, or every unresolved frame has a legitimate durable
disposition/recovery handoff. Pausing a newer stale frame then blindly resuming
it after publication can quarantine the player. An in-memory residual hold after
critical ACK cannot survive restart because that critical command is checkpointed.
Reserve readiness by original operation/generation while the hold remains,
perform coordinator ACK outside state locks, and clear only the reservation
after success. No frame rewriting, invented revision, drop-result save ACK or
discard is permitted. Actor-independent room recovery must not depend on loading
a player with unresolved save obligations. Both-engine ordering/ACK/restart and
shutdown/copyover checks remain prerequisites to production integration.

Replay ownership must extend through proof publication and journal checkpoint,
or revalidate the restored-owner generation before checkpointing. A callback-only
permit released after SQL cannot protect a hold registered after the PID's final
callback; a later duplicate frame may not invoke a callback at all. Independent
worker ACK and public revision checkpoints also need the same authority fence.
A per-pass journal deferral marker cannot establish a resident hold or load
permission. After reopening, the durable critical owner must reinstall its gate
before any replay; preserving frame bytes alone does not reinstall that gate.

### Ordinary SQL exception ownership: reviewed implementation boundary

Source-only review at local9cfdad368 and fetched3dbb8bc83 confirms ordinary apply
has no exception rollback guard and pooled apply/readback can escape while a
lease is still held. Ward columns/new0056 do not repair this. Keep cleanup
metadata specific to the SQL repository: original session, untouched/verified
idle/retire-required disposition, confirmed rollback and cleanup error. A new
borrowing overload may report that proof; the existing direct API never closes,
replaces or releases caller handles. Reject an existing transaction, autocommit
OFF or reconnect-enabled connection before changing caller state.

Install a nonallocating guard before START, explicitly finalize cleanup using
literal ROLLBACK on the original session, and require successful cleanup plus
same session/idle/autocommit/reconnect-disabled state for pooled reuse. A
best-effort destructor alone is not reuse proof. Own the lease immediately after
acquire; unproven cleanup discards then releases it. Replacement consumes the old
lease even when NULL; own only its returned replacement thereafter. Precommit
caught bad_alloc with confirmed rollback is retryable ENOMEM/no proof; possible
COMMIT followed by readback allocation retains ambiguous outcome. Explicit
returned terminal ENOMEM and genuine custody/death diagnoses stay unchanged.

Death-conflict continuation needs confirmed original rollback before beginning
retained-evidence work, and separate checked cleanup for that evidence
transaction or conservative lease retirement. Ordinary cleanup cannot certify
later work. Also own extra-description malloc buffers before string assignment
and pet MYSQL_RES before allocating comparisons. Actual engine tests must prove
START/DML rollback, successful-COMMIT reply loss/readback OOM, failed rollback,
replacement failure, borrowed transaction/settings refusal, conflict continuation
and resource allocation witnesses. Use calibrated client transaction bits after
SELECT1 for both engines. Force-rebuild changed header consumers/dependent
harnesses from verified .d closure. This is a design boundary, not implemented or
qualified SQL cleanup, and it does not authorize incoming history integration.

Actual13-case ordinary SQL BEFORE evidence now confirms12 failures/one control
on each private MariaDB/MySQL engine (declaration8e986daf, October3 review).
Unsafe borrowed states are admitted and return applied/changed rows; allocation
can retain an open transaction or pooled lease; failed rollback permits unsafe
pooled reuse. Conflict cleanup loses the original custody diagnosis/witness, but
the existing idle gate prevents a second evidence transaction (one START only).
Both native resource ownership seams fail. These findings establish the reviewed
cleanup requirement; private implementation and AFTER verification remain pending.
All source/schema0055 pins and original300/120 bounds are preserved, with first
strict fixture failure archived. Incoming0056 and restored-save wiring remain open.

Worker completion delivery after a real exact journal ACK is now solved locally.
The allocating result deque is replaced by a fixed 256-entry FIFO under the same
mutex, capacity/backpressure and receipt ownership. Nothrow moves preserve
completion delivery when allocation is unavailable after the journal frame is
removed. Five proven BEFORE aborts/seven controls become twelve AFTER passes per
backend, including FIFO wrap, partial/full capacity, shutdown/reopen, exact typed
receipts, real ACK failure/repair and unrelated-PID progress. Unchanged scheduling,
admission and parking owners contribute another 86 AFTER passes (110 total).
Both strict incremental server builds, nine maintained owners,14 validations/30
contracts and nonmutating matrix pass. Actual inactive MariaDB/MySQL save, death,
crash and copyover journeys plus native follow-ups pass; flatfile creation/save/
cold restart/relog passes. Final declaration edd75c103ad576d8d8b0c994696a0108b09914e0533725db536a8cfeec4d9678 verifies all1,232 production
inputs, component artifacts, binaries/logs and owned SQL teardown/port rebind.
Preserved fixture failures remain evidence, not production failures. Ordinary SQL
cleanup, typed exact journal identity, restored-save integration, incoming0056 and
broad qualification remain open. No R1-R8 gate or coverage status is promoted.

### SQL cleanup candidate integration and native qualification

The reviewed five-file SQL cleanup candidate is integrated after committed worker
completion milestone dfc879598. Borrowed idle/autocommit/reconnect-disabled
sessions report original-session cleanup proof; pooled owners retire unconfirmed
connections, preserve consumed replacement ownership and ambiguous COMMIT, and
retain original custody evidence when rollback is unconfirmed. Separate retained
transaction cleanup precedes writer-fence release. Native buffer/result ownership
is scoped by RAII. Recovery_apply and sustained lifecycle lock allocation remain
separate work.

Manifest `tmp/sql-cleanup-production-inputs.local.json`, SHA-256
`efcb65d1faa86b6bd95eac6c576cf3b336033d26a01d0a16b5f46b475425de52`,
freezes1,233 source inputs: four changed repository source/headers and one new
SQL-only helper. All actual dependency-recorded header consumers are forced:
47 SQL/46 flatfile objects. Source preparation first refused Windows path
separator mismatch, then src-relative dependency parsing; both failed setup
attempts are archived before source/native object mutation. Corrected setup
matches exact dependency tokens and retains every consumer check.

Unchanged ordinary13 and retained4 owners are registered as explicit manual SQL
probes. Frozen AFTER drivers preserve original300/120 bounds, canonical0055
schema settings, all source/binary/owner/case pins, three rejection guards and
owned teardown/identity/port rebind. Strict builds and native AFTER qualification
are pending. Source integration is not a solved SQL issue or R1-R8 acceptance.

### Follow-up recovery and lifecycle ownership gaps (source review only)

Recovery must cover the whole borrowing/proof/lease boundary, not only
`player_snapshot_repository_recovery_apply`. Its cleanup is installed after
START, ignores rollback and lacks reconnect-disabled/original-session checks.
`player_quarantine_recovery.c::sql_proof` can accept an otherwise valid row proof
after unconfirmed rollback; journal recovery_resolve can then remove quarantine.
`prepare_sql` shares unchecked cleanup. Boot revalidate_selected chooses pool
reuse by mysql_errno rather than demonstrated original-session cleanup.
Use the SQL-specific cleanup/lease primitives through resume and proof, preserving
all archive/backend-generation/creation/UID/component/state comparisons. Refuse
successful verification when cleanup is unconfirmed; revision alone is not proof.
Native reproduction is pending: actual START reply loss, DML allocation/rollback,
proof allocation plus persistent rollback refusal, valid proof with both rollback
attempts refused, real pooled boot revalidation and ambiguous COMMIT/exact retry.
Exact preparation/archive/journal/SQL rows, quarantine fence and lease witnesses
are required. No ordinary post-successful-COMMIT allocation seam was found; do
not manufacture such a BEFORE claim.

Separate lifecycle work is required in economic_sql_lifecycle_guard.c. Lock and
unlock allocate SQL strings inside uncaught noexcept acquisitions/destructors.
Acquisition publishes local authority before SQL ownership is transferred; a
possibly successful GET_LOCK needs tentative original-session ownership. Unlock
ignores verified release before clearing local exclusion/coordinator ownership.
Use bounded stack SQL/allocation-free parsing and checked same-session release,
keeping SQL outside authority mutexes and SQL/local/coordinator release order.
Uncertain release must retain exclusion and support retry; catch-and-ignore is
insufficient. Native sustained-allocation, partial acquisition, lost lock reply,
failed release, independent lock ownership and unrelated-admission proofs remain
pending. These source findings are separate milestones; current frozen SQL cleanup
candidate and0055 source pins remain unchanged. No schema or R1-R8 gate promotion.

### SQL cleanup public-header regression and corrected qualification

The revised maintained gate passes worker/pipeline/journal/quarantine/recovery,
then diagnostics exposes a real dependency regression: public recovery headers
transitively include the SQL pool helper, whose `<mysql.h>` needs native SQL flags.
The unchanged generic staff diagnostic command intentionally has only `-Isrc`;
adding SQL flags would conceal this coupling. A read-only architect confirms all
public cleanup uses are pointers and concrete cleanup objects occur only in the
two repository implementations/helper.

Both public repository headers now forward-declare player_sql_cleanup; both .c
implementations explicitly include the private helper. Existing mysql/mysql.h,
overload signatures/noexcept and the helper's enum/struct/RAII behavior remain
unchanged. No extra metadata header is added (production inventory stays1,233).
The first candidate source/34 passing actual SQL cases remain immutable evidence
for their original closure, not the corrected source. Header repair preparation
first applied the snapshot pair before refusing mixed-CRLF conflict matching;
second preparation refused the already applied pair. Complete four-file targets
were then checked against pinned original/corrected LF states before completion.
The original failure and integration receipt are preserved.

Corrected-source maintained checks, all affected header-consumer strict builds,
unchanged17 cases per real engine, maintained terminal matrix and actual inactive
gameplay/recovery must pass before this SQL issue is declared solved. No accounting
activation, wallet-root item inclusion, declined spell-path retry or R1-R8 promotion.


### Corrected SQL cleanup regression qualification and integration authorization

All14 maintained owners now pass on the corrected header closure: worker,
pipeline, journal, quarantined dispatcher, phase01 recovery, diagnostics, writer
route/site/coverage contracts, root harness, real death selector, evidence codec,
journal lifecycle and cold-load fence. Root discovery first refused a stale exact
manual-only set after the two explicitly supplied SQL artifact owners were added;
only those two expected names change. Original failure/owner and exact repair
receipt remain archived; discovery exclusion, serialized-resource, timeout,
cancellation and signal tests remain intact (seven behavioral cases pass).

Maintained terminal qualification passes on both actual engines:18 native command
invocations in10 fresh canonical0055 schema groups. The independent declaration
`tmp/sql-cleanup-header-maintained-terminal-qualified-v1.local.json` is SHA-256
`ff2115b6d5840614f9dfdab2e4f3c72f91b0fdca28be02f7940d8b89d53b9d03`.
It binds527 native/source/owner inputs, sanitizer binary, original600-second
compile/group limits, exact group DROP and explicit schema absence, owned process
identity/stop and port rebind. Typed quest/spell internal replay remains distinct
from typed cold restart; this owner uses controlled pool stubs. Current14 contract
fixtures,30 accounting tests and nonmutating matrix check also pass. Seven changed
C/C++ source/header/owner files pass nonmutating clang-format18 fixpoints.
Corrected-source actual inactive gameplay and final milestone declaration remain
pending; these component results do not solve full accounting qualification.

The user explicitly approved the previously blocked local Git merge to integrate
new experimental-accounting commits. Preserve this authorization separately from
GitHub PR merge/deploy/production activation, which remain prohibited. Complete
and commit the bounded SQL cleanup issue first; then refresh and integrate both
histories locally, qualify the combined source, and push normally. Previously
recorded rejection and source-specific qualification remain historical evidence;
no unauthorized history mutation or divergent push was attempted.


### Bounded ordinary/retained SQL cleanup solved locally

Final declaration `tmp/sql-cleanup-header-final-milestone-v1.local.json`, SHA-256
`0a0b5c5a4e683119db303a478c92dea956060a87a85d9782b1ba20e353865aaf`,
rechecks all1,233 current/frozen production inputs, paired BEFORE and corrected
AFTER artifact/binary/owner/log/support pins, strict builds,14 maintained owners,
14 accounting fixtures/30 contracts, matrix and seven format fixpoints. Both
actual engines pass13 ordinary+4 retained fault cases (34 total); the separate
maintained terminal matrix passes18 native invocations/10 fresh schema groups.
Original300/120 fault-owner and600 terminal/build budgets remain unchanged.

The current strict SQL server passes real inactive save/death/crash/livecopyover
journeys plus actual item/partial-forest/OOM/spell/quest persistence follow-ups on
MariaDB7c1b354b (195.605 seconds) and MySQL35ef7379 (297.333), within1200 each.
Journey-owned schema drops leave no wrapper leftovers; independent baseline
schema equality/absence, owned PID/session/executable/datadir stop and port rebind
pass. Current strict flatfile creation/save/coldrestart/relog passes132.529 seconds
within600. Accounting scanner/contract checks ran on Windows; native compilers,
owners/builds and gameplay ran in WSL. A final declaration first refused a raw
Windows log path in WSL; exact scoped alias mapping corrected the verifier without
rerunning or weakening qualification. Original prepared versions/refusal remain
preserved. This bounded transaction/lease/resource cleanup issue is solved locally.

Pooled owners retire unconfirmed original-session cleanup and preserve replacement
ownership; borrowed callers retain responsibility for their handles. Custody
conflict diagnostics survive failed rollback and the separate retained transaction
is cleaned before writer-fence release. Ambiguous COMMIT remains ambiguous.
Recovery preparation/proof/lease, lifecycle release/exclusion, typed exact journal
identity and restored-save production wiring remain separate unfinished work.
Canonical0055 results exclude incoming0056 until integration and requalification.
Coverage remains false/releaseBLOCKED; no R1-R8 or activation gate is promoted.

The user now assigns Plans1-4/shared coordinator/contracts/producers/registry/
matrix/activation to this primary stream. A separate user-coordinated agent owns
Plan5 independent reconciliation/audit/backup-restore/release qualification. Return
narrow interface requests and independently committed slices for integration on
one tested candidate; do not duplicate Plan5 mutation/tooling work. Local Git
integration is explicitly authorized; GitHub PR merge/deploy/production activation
remain prohibited. Normal milestone publication follows combined-source checks.

### Current source preparation under plan-level test batching

Worker sealed identity and lifecycle cleanup propagation are implemented locally
but unqualified. See the October 3 review checkpoint for exact BEFORE/prepared
receipt pins, thread/session ownership, native pooled retirement, direct-DB
fail-closed policy and preserved known/ambiguous outcomes. Update maintained
fixture-double link closures for the new exact pool retirement API before the
Plan 1 batch. Restored-save production admission/hydration/replay wiring, runtime
recovery and current-candidate native/gameplay/SQL/flatfile qualification remain
open. No historical passing test qualifies these newer sources.


### Phased save startup prerequisite; source only

Save pipeline preparation and execution are now separate opt-in APIs. Preparation
opens validated journal/recovery metadata with admission and load replay closed,
without starting persistence workers or the dispatcher. Start preserves restored
metadata, refuses duplicate execution and leaves prepared holds intact on startup
failure; cleanup/join happens outside the pipeline mutex. Resume and the synchronous
save/hydration admission helpers cannot reopen a prepared, unstarted pipeline.
The existing init wrapper retains immediate prepare/start behavior and tears down
on failed start. Production comm boot calls remain unchanged until the complete
restored-save ownership handoff is implemented; this API does not close that race.

Read-only architecture review confirms the coordinator already restores all replay
observers before its workers launch. Durable-save census must scan the actual
journal, including retained/quarantined frames; resident pipeline diagnostics are
not that census. Checkpoint-spanning apply permits, original operation/generation
holds, independent ACK fences, retained wakes, actorless native hydration and
partial-startup dependent-owner cleanup remain required before production wiring.

BEFORE manifest tmp/save-startup-phase-before-v1.local/manifest.json is SHA-256
07923782e8b4dc477e971b44bd8e3b0aea795d366e563c22bdcfd11e96bcdebb,
base65e4f1590872eaf6514663568b7b36bbdb1b73a8. Deferred qualification must cover
prepared admission and shutdown, duplicate start, worker/hook/dispatcher failure,
retry retaining exact holds, legacy inactive initialization and the full overlapping
save/critical recovery journey. No compiler/native/SQL/gameplay tests ran.


Six maintained fixture link boundaries now declare checked pool retirement
explicitly unavailable: critical repository stubs, direct death-conflict owner,
default item/currency/bank fresh-session doubles and standalone cutover owner.
They return false without closing any handle. Real-pool modes retain the actual
production implementation; all original assertions, retry sequences and deadlines
are unchanged. Source-only closure preparation is unqualified. These doubles
cannot prove successful retirement or actual repository receipt preservation.
The separately frozen private V2 baseline remains unchanged.


Validated active-journal observation prerequisite (source only): the new
collect_retained API collects all validated active frames under the journal mutex,
including exact original bytes/record identities and current quarantine/policy
flags. A temporary result is published only after complete collection; allocation
or scan failure returns an empty output without apply/checkpoint. Existing
corruption scanning may archive evidence and latch the global fence. This is an
observation that expires after unlocking, not an apply/checkpoint/ACK permit,
hydration proof or complete archive census. Record ID alone is not exact identity.
The inherited scanner treats a missing file as empty; future durable-absence
claims must independently establish file/generation authority.

BEFORE tmp/save-journal-census-before-v1.local/manifest.json is SHA-256
75d98c2b4ef1a2d5b43c4ff0baac2ece6a3eea4ab2a43072e6c2367b3d6dac58,
base38432876b. Read-only source review found no bounded-API blocker. Deferred
cases: exact bytes/duplicate identities, allocation failure, quarantine/policy
flags, mixed valid/corrupt records, missing-file and concurrent-mutation behavior.
No compiler, native, SQL, gameplay or recovery checks ran. No production caller
uses this prerequisite yet; the full ownership protocol remains open.


Phased-startup V1 owner preparation is frozen PREPARED_UNEXECUTED:9 cases with
24 actual production units plus harness,523 source headers and4 private inputs
(551 inputs total). Private source snapshot tmp/save-startup-phase-prepared-v1/source-inputs
manifest SHA-256c100f85640fc5dca95fe0a3a5c7a2a411ae48a18b401872b4ec9ef27de97d8bf
(LF553d2353656f146482afc0c05432b63f64769aeaaf9850d0efff5ebb1996e085).
Receipt tmp/save-startup-phase-prepared-v1.local.json is SHA-256
56daccad81fda368817da4cd83110bd0a266e8faa32fdec403c493a96de81d37.
Runner1c222a8cc7dc61762a964d21978a04e7a000ec2dd3adeca0fd8030d4987e6d94;
harness218f8baf252b37cb119b85d9b8e862ff028c535413e7eb21d52d4e71be6d54b4.

Cases cover closed preparation/shutdown, duplicate lifecycle, both worker-thread
creation faults, dispatcher fault with exact-hold retry, genuine retained-slot
hook refusal, verification callback failure/retry and legacy immediate startup.
The inherited600-second compile/120-second aggregate bounds are unchanged;
new-owner performance, pthread interposition and native link closure are uncalibrated.
BEFORE uses a labeled actual legacy-init mapping; API absence/compile failure is
not semantic RED. Hook refusal retains its genuine parked slot and does not invent
repair authority. Manual restored registration/release and unrelated-PID replay
are not production critical-ACK wiring or restored-save deferral qualification.
No compiler, native, AST, SQL, services or tests ran.


### October 4 source prerequisites; unqualified

[Current checkpoint](../REVIEW_STATUS_2026-10-04.md) records save execution permits
and exact generation holds80692d52b, authority-bound baseline initialization0aa0bceeb,
allocation-safe coordinator cleanup254a0379d and ordinary receipt progress retention
fd5683abc. These narrow source changes supersede earlier statements that no permit
or marker exists; they do not complete production startup or ACK authority.
Lifecycle staged composition, independent Plan5 marker readers, authentic baseline
source proof, full mutation census, inert actorless publication, ACK reservation,
wake retention and full native/gameplay/recovery qualification remain open.
Keep all historical component scopes and original test budgets. No new checks ran.


Source follow-up2b4591c21 adds private sealed staged lifecycle composition with
preserved original participant signatures, authentic predecessor visibility and
precommit receipt preparation. See [source scope and corrected private V2](../FLATFILE_LIFECYCLE_COMPOSITION_PREPARATION_2026-10-04.md).
Boundary request bool/digest remain external assertions; actual native boundary
producer/capability, full holding/item-source/writer/pending census and activation
remain required. Inert stageb36e9690f is discard-only; graph proof and final atomic
hydration/nonthrowing enrollment are not implemented. No checks or milestone push ran.

## October 4 integration priority

The current source checkpoint is6d42ad788. Replay ownership and the complete
indexed SQL boot recovery catalog are implemented but unqualified; the latter
is consumed by private all-absent reconstruction. Positive-PID synchronous save ownership, phased SQL startup, actor-independent
restored ordinary-drop dispatch, fresh journal/control census, private guarded ACK
and release-driven reserved revisit are now connected source. Ownership stays
disabled pending complete native writers/shutdown, legitimate overlapping-save
and rejected-outcome disposition and full graph/live producer coverage. Finish that actual SQL recovery path
before adding more isolated prerequisites, then the native baseline/cutover/pause
owner and existing flatfile bank/item parity. Keep the stated independent
acceptance: missing Plans2–4 coverage must refuse activation, but those writers
need not all be delivered to prove this plan's guarded authority procedure.
No synthetic coverage manifest may establish game-wide completion.

A reliable remaining duration has not been measured; the number of source helpers
is not a percentage complete. User testing cadence remains major-plan batches.

The user explicitly reaffirmed the group-authored delivery scope: take on no
additional work and prioritize completing the required implementation for group
testing. New changes must trace to this plan's acceptance or R1/R6/R8, with item
identity requirements from R4 where publication touches custody. Avoid optional
infrastructure or a broader recovery framework. Exact-ID replay and preservation
of a confirmed native outcome remain required; choosing whichever inventory is
visible is not evidence of that outcome. Existing ownership prerequisites must
serve the actual production integration rather than become a separate project.

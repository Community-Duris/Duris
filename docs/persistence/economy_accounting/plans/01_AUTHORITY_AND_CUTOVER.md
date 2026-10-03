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
initial worker admission, retry scheduling or pending promotion; those remain
separate qualification work.

Read-only review identifies the distinct failure states for that next work:
make_unique receives an already-moved snapshot temporary before its allocation;
post-emplace ready-queue failure can leave a slot after bytes/revision rollback;
and pulse consumes a result before retry/promotion queue allocations. Promotion
can then lose an original receipt-bearing completion after its real journal ACK.
Stage cancelable queue ownership and an empty job before moving input or changing
revision/receipt state; fault-injected native tests must establish each failure
and exact recovery. Separately, result-queue allocation in worker_main can throw
after journal ACK. Ready-queue repairs cannot qualify completion-delivery safety.

Separate source review identifies unresolved-replay proof retention: an earlier
same-PID ordinary durable revision or exact death/receipt proof remains eligible
when a later callback returns retryable/ambiguous. Withdraw both proof stores
for the failed PID before checkpointing unaffected work. A callback bad_alloc
must remain unresolved, with no proof, rather than create a corruption archive;
execution or COMMIT may already have happened. Genuine runtime, custody and death
failures retain their quarantine rules. Paired native tests are being prepared;
this repair is not yet qualified. Ordinary SQL exception rollback and pool-lease
cleanup are separate: current ordinary apply can escape with a transaction or
lease still held, including allocation during ambiguous-commit readback.

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

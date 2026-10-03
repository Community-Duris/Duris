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

Coin callbacks also require separate review: current ACK ordering and replay's
null callback do not prove pile publication survived restart. These are open
R1/R2/R4/R8 requirements, not waived by the native SQL coordinator component fixtures.


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
5. Restore schema-2 ordinary-drop obligations from durable commands after cold
   room hydration. Verify already-restored original UIDs, exact bytes/topology
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
and degraded gates. This is a proposal, not an implemented bypass. The restored
room verifier must be explicitly actor independent, and original-operation hold
release belongs after successful publication ACK in both the normal and
`ack_pending` branches, before pending-owner erasure. Notifications must not
hold durable ACK waiting for an absent actor.

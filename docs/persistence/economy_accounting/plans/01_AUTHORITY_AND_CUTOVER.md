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
   Their SQL pool boundaries use fresh fixture connections; coin/item ambiguity
   is synthesized after a successful repository return. Qualify actual client
   commit-reply loss, production pool lifecycle and live publication/restart
   separately. Retain every unsupported-family refusal. Direct owner calls and
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
R1/R2/R4/R8 requirements, not waived by the synthetic coordinator fixture.

# Plan 1: authority, admission, and cutover

Execution order and current status: [completion plan](../FINISH_ACCOUNTING_PLAN.md).
Historical status is recoverable from the preserved original history. All domain acceptance below remains required.

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

### Independent acceptance evidence at this milestone

| Original acceptance | Qualified evidence and scope |
| --- | --- |
| Real SQL coordinator/pool for bank, coin and item; replay, changed ID, rollback, lost reply and restart | Existing bank/coin/item native owners pass MariaDB10.11.14 and MySQL8.0.46; exact binaries/logs and faults are in the consolidated report. This is typed-owner qualification, not every writer. |
| Staged boot and incomplete coverage refuse; disposable guarded cutover/pause | Maintained native lifecycle and owned-cutover owners pass both engines, preserving incomplete manifest/verifier/inbox/outbox and staged-boot refusal plus reversible pause. The new active-drop fixture uses only a disposable three-route synthetic manifest. |
| Flatfile bank/item parity and original retained journal/publication | Existing actual flatfile authority/admission owners pass exact-ID one-effect replay, changed-ID refusal, shutdown/init receipt retention and explicit ACK retirement; coin remains refused until Plan2. |
| Focused common-owner checks and both builds | Eleven focused owners,35 worker/journal cases per backend,22 checked-release,14 startup and18 recovery-session cases pass their frozen inputs. Current SQL/flat strict builds include the census correction. |
| Actual ordinary-drop publication and two cold boots | Maintained three-file regression compiled against all1248 current production inputs; both-engine real `do_drop`, native full payload/custody and guarded ACK pass, with no reseeding between two full-world boots. |

See [the consolidated source-bound report](../SAVE_RESIDENCE_INTEGRATION_2026-10-04.md)
for exact inputs, earlier failures, native budgets and cleanup. This table does
not mark registry rows qualified or authorize whole-game activation.

## Boundary and handoff

Plans 2-4 provide route-specific typed intents and transaction adapters. This
plan provides a narrow documented registration interface and fixture for them;
it does not claim their writers are covered. Plan 5 supplies the final route
manifest and independent audit result. No live epoch is selected until all five
plans pass the release gate.

# Plan 5: independently verify SQL baseline zero effects

This slice starts at `a5d26b658247e5c843c9c03b7e6d3bac94b6a711` on
`codex/accounting-plan5`, worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`. Its result
commit and committed input hashes are bound by
`tmp/plan5/sql-baseline-zero-effects-evidence.json`. Publish this lane's branch
for primary-owner integration; do not push directly to experimental-accounting.

The refreshed integration remote remains
`f7d26eaa721cd3b675c0b0c65009a2535813f400`. Native source tree
`d9a9610f3a6c72c14a7d1870d88fed8c018fa87b`, all 1,232 native inputs, and all
236 migration inputs remain unchanged. Native canonical base is
`7d2f8e8153f637c38e19054cf1202436d3f9a28a`. Every fresh SQL check uses
canonical migrations through `0056_spell_ward_durability`. No 0055 result or
isolated component pass qualifies the primary owner's combined candidate.

## Established defect and independent fix

The native baseline owner requires zero child, item-reference, currency-ledger,
ownership-ledger and outbox rows under each baseline operation, and also forbids
using a baseline operation as another root's child. Ordinary audit views exclude
reason 38. The independent origin reader did not verify these zero-effect
invariants after verifying canonical roots and positive projections.

A private owner attached an extra child to a baseline published by the actual
native SQL owner, with normal canonical constraints enabled. Both engines
admitted the audit with unchanged findings. The RED took 162.509 seconds:
one guarded test, two engine failures, zero skips. Exact inputs, binaries and
logs are preserved, and the private fixture restored the extra row.

The origin reader now issues one SELECT with six EXISTS probes, scoped to the
same lineage/epoch and consistent read-only snapshot. It requires absence of:

- `economic_accounting_child.operation_id` and
  `economic_accounting_child.child_operation_id` references to baseline roots;
- `economic_accounting_item_reference.operation_id` rows;
- `currency_ledger.operation_id` and `item_ownership_ledger.operation_id` rows;
- `critical_outbox.operation_id` rows, regardless of outbox status.

The query returns six integer flags in one row and fetches no native ledger or
outbox payloads. Nonzero, missing or invalid flags refuse with
`EAB1 SQL zero-effect mismatch: <table>.<column>`. The reader requires all 12
baseline source/detail tables to be InnoDB. Existing capsule/projection bounds,
REPEATABLE READ, consistent READ ONLY, rollback and cursor-closure semantics
remain intact. Structural bounds do not establish measured workload budgets.

Selected-book disagreement refuses capture. Retained-book disagreement remains
an existing `baseline_source_claim` finding. Cross-epoch child links also
contaminate the selected book and refuse its capture. A retained item-reference
fixture preserves the additional `unattributed_ownership_event` and
`unattributed_ownership_uid` findings from its coupled legacy ledger row.
The audit does not repair authority or retained evidence.

No schema, accounting contract, mutation owner, shared coordinator, producer,
registry/matrix, activation owner or audit output format changed. There is no
new shared interface request. The fixed query lives in the existing independent
reader; synthetic fixtures add only the two previously absent source tables.

## Exact validation and evidence

Environment: `duris-plan5-origin-sql-tools:local`, image ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`;
Ubuntu 24.04, Python 3.12.3, GCC 13.3, PyMySQL 1.0.2-2ubuntu1.1.
MariaDB is `10.11.14-MariaDB-0ubuntu0.24.04.1`; MySQL is
`8.0.46-0ubuntu0.22.04.4`. Docker mounts the checkout read-only at `/workspace`
and only `bin/` separately writable. Disposable daemons use fresh private Unix
sockets with TCP disabled and a clean test environment, without checkout `.env`.

The unchanged native fixture has 13 C++20 inputs, strict warnings, ASan and
UBSan. It initializes, applies, exactly replays and reconciles two nonempty
baseline books with `active_epoch` NULL. SQL binary SHA-256 is
`7985d95effd41456d445a5924e466a5ae5ad25fa9506d141d2ff1dd1168c4ee2`;
client-free is
`efaee7f33a1ebc38fe9b6afe9baaa32b66b8b3db1746679dc58c05cf39a9b027`.
Client-free initialize/apply/reconcile retain `ENOTSUP` refusal.

The final native matrix has 139 cuts per engine: 18 new zero-effect cases and
the previous 121 root/source/book/projection cuts. It has 65 capture refusals
and 74 exact diagnostic cuts. The new cases cover each forbidden family in
selected and retained books, both cross-epoch child directions, and all four
outbox statuses. All new cases keep canonical FK/CHECK/UNIQUE constraints
enabled. The existing 11 damaged-import cases remain distinctly identified;
foreign keys are enabled for every audit and native replay.

Six canonical constraint refusals per engine remain exact: source composite FK,
source operation uniqueness, both reservation foreign-book FKs, posting
line/event equality and allowed reservation kinds. Numeric errors remain 1452,
1062, and CHECK codes 4025 on MariaDB / 3819 on MySQL.

Each audit preserves all 18 captured authority/evidence tables. SELECT-only
credentials reject UPDATE with 1142. The independent native-origin pair also
preserves all 15 source/evidence tables it captures, native flatfile bytes,
permissions and link counts; success/refusal always rolls back and closes the
cursor. After private fixture repair, native replay reproduces the original
result and retained rows.

The intact fixture still reports one evidence-loss, two missing-native-holding
and two missing-native-item findings. It supplies no complete world/native
capture, and these findings remain visible. Accounting stays inactive.

| Exact command | Result and evidence |
| --- | --- |
| `DURIS_RUN_NATIVE_BASELINE_AUDIT=1 python3 -u tests/async/test_native_sql_baseline_audit.py -v` at `/workspace` | Pass: one guarded test, both canonical engines, zero skips, 220.263 seconds; 139 cuts/six constraints each, native replay and full sibling exporter/operator CLI matrix. `tmp/plan5/zero-effects-green2.log`, `bin/tests/plan5-baseline-zero-effects/{mariadb,mysql}.log`. |
| `python3 -m unittest test_reconcile_economy_accounting test_economic_sql_audit_origins -v` at `/workspace/tests/async` | 122 collected: 117 passes, five explicit skips, 11.903 seconds. `tmp/plan5/zero-effects-components.log`. |
| `DURIS_RUN_ECONOMIC_ORIGIN_INTEGRATION=1 python3 -m unittest test_economic_sql_audit_origins.NativeSQLOriginTests -v` at `/workspace/tests/async` | Two passes, zero skips, 204.027 seconds; four authentic native EAB1 batches across two epochs, three captures/three refusals/six rollbacks on each canonical engine. `tmp/plan5/zero-effects-origins.log`. |
| `python tests/async/test_economic_sql_audit_origins.py -v` on Windows | 22 passes, two explicit native-origin skips, 0.026 seconds. `tmp/plan5/zero-effects-host.log`. The explicit native pair executes those skipped cases. |
| Five changed Python files: `python -m py_compile`; unstaged/staged `git diff --check` | Pass, `tmp/plan5/zero-effects-final-checks.log`. No C/C++ input changed; this slice requires no server rebuild or changed-line C++ formatting. |

The first fixed native run took 194.186 seconds and stopped because its retained
item-reference expectation omitted the two valid unattributed findings. The
final fixture requires all three findings exactly. That failed run and its
frozen inputs are preserved. Core/unit/native-origin inputs remain unchanged
after this expectation-only correction. No finding was suppressed or waived.

Exact syntax-check inputs:

```sh
python -m py_compile scripts/economic_sql_audit_origins.py tests/async/test_economic_sql_audit_origins.py tests/async/run_economic_sql_audit_snapshot_mysql.py tests/async/test_native_sql_baseline_audit.py tests/async/run_native_sql_baseline_audit.py
```

The protected manifest binds the result commit, all executable hashes, native
source/migration maps, toolchain, successful and failed commands, artifact paths
and every unchanged preceding-slice artifact. Generated binaries, logs and the
manifest remain uncommitted under `bin/` and `tmp/plan5/`.

## Ownership and remaining gates

Six owned files: `scripts/economic_sql_audit_origins.py`,
`tests/async/test_economic_sql_audit_origins.py`,
`tests/async/run_economic_sql_audit_snapshot_mysql.py`,
`tests/async/run_native_sql_baseline_audit.py`,
`tests/async/test_native_sql_baseline_audit.py`, and this report. The full exporter,
independent canonical decoder/reconciler and all shared/native files remain
unchanged.

The existing primary-owner registration request remains: explicitly register
the native baseline case with `DURIS_RUN_NATIVE_BASELINE_AUDIT=1`, both engines
and zero skips, plus the native-origin pair with its opt-in. Fields are `path`,
`arguments`, `environment`, `required_cases`, `provider`, `engines` and timeout;
consumers are the integration runner and combined release report. Shared
registration stays outside this lane's edits.

Rootless reservation inventory remains independent audit work. Complete native
capture, namespace authority, real writer/player journeys, fault/restart/lost
reply matrices, populated upgrades, full restore/clone qualification, retention/
erasure/export policy, live sweeps and measured mixed workload budgets remain
open. Two near-limit budget cases and one native-stake SQL case are skipped
here and are not requalified. Prior backup/restore and retention results remain
pinned to their own source; the combined candidate is not qualified by this
slice. Release and activation remain blocked.

`AI_CONTEXT.md` remains absent from both checkouts, and no curator capability
or notebook/workflow reference is available. The earlier request is unanswered;
bounded accessible Pages searches returned no target and do not establish
absence. This report and manifest provide the curator handoff, not a claimed
notebook update. Wallet-root item exclusions, declined inactive spell behavior
and active blackjack refusal are preserved. No activation, production access,
merge, deployment or audit auto-correction occurred.

# Plan 5 registration follow-up and retained SHOP link finding

The primary has integrated the exact restore-reader repair and its six mandatory
pure cases. Refreshed contract and reader checks pass on the combined branch.
The primary's separate SHOP fixture-linkage repair still fails its first actual
native SQL owner link: `mob_index` is undefined. The original runner executes
zero cases. This is a primary-owned fixture handoff, not a solved issue or
positive cold SHOP qualification.

## Source and ownership

- Branch: `codex/accounting-plan5`; worktree:
  `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Prior completed qualification result:
  `df9df046269add89a13407c8d22817190eed7c34`.
- Consumed primary: `b754e2962390811b13cde820674684aef9098986`, including
  `a28f235f3afb4e96943cea60102289e5147f0f51`.
- Combined tested base: `0e1e45b4ea1fce363c8dba0087053d4e6ef580a4`.
  This preserves both histories through a local integration merge on the
  requested branch. No PR merge or primary-branch push occurred.
- Native tree: `b00968beadaa72d2e126c11d41a92231017e6d27`.
- Migration tree: `1b0f9a40fef29de409338ba83be015cd3390c9f5`, canonical 0056.
- Owned tracked file for this follow-up: this report only. No primary-owned
  harness, coordinator, producer, accounting contract, registry/matrix or
  activation file is independently edited.
- Evidence manifest: `tmp/plan5/b754-followup-evidence.json`.
- Result/remote/preservation receipt: `tmp/plan5/b754-followup-delivery.json`.

The exact native and migration trees remain identical to the completed
qualification above. Its full 726-unit SQL and 726-unit flatfile builds and
managed recovery receipts remain frozen to their original tested base; this
follow-up does not relabel their process execution as a new run. No C/C++ server
source changed, so those full builds are not repeated for manifest and fixture
changes. This slice performs new native fixture execution and new registration
and reader checks instead. No new database journey is claimed here.

The restore verifier and canonical-audit test retain their prior owned hashes:

```text
scripts/economic_restore_evidence.py
376e5c767e5ce41825170b945612292a5cf267800b3f0e61f657d39d51f5659e
tests/async/test_economic_sql_canonical_audit.py
0a66167e3ff0b2c59040bb3b8a45708f2adac46cec917b19ff6c351fea6cb9b9
```

Earlier branch work remains preserved in this branch's committed ancestry.
Its follow-up is maintained here as requested, including this registration
closure and the original native qualification finding.

## Executed commands and environment

The temporary observation runner selects the existing commands directly:

```text
python3 -u -B tmp/plan5/run-b754-followup.py contracts
python3 -u -B tmp/plan5/run-b754-followup.py shop

python3 -u -B -m unittest -v
  test_economy_writer_coverage_contract test_audit_accounting_invariants
python3 -u -B -m unittest -v
  test_economic_sql_canonical_audit.CanonicalAuditTests
  test_economic_sql_canonical_audit.RestoreProjectionTests
python3 -u -B scripts/validate_economy_accounting.py
python3 -u -B scripts/generate_economy_writer_coverage.py --check
python3 -u -B scripts/validate_economy_accounting.py --release
python3 -u -B tests/async/test_shop_trade_publication_retention.py
```

Both selections use image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
GCC 13.3.0 and Python 3.12.3. Docker has `--network none`, a read-only root and
read-only `/workspace` source; generated outputs are mounted at `/workspace/bin`
from `D:/CodexEvidence/accounting-plan5/bin/bb259-maintained-20261005`.
An explicitly executable private `/tmp` mount uses
`rw,nosuid,nodev,exec,size=4g,mode=1777`; `TMPDIR=/tmp`,
`PYTHONPATH=/workspace/tests/async`, and bytecode writes are disabled.
No privileged capability, live service, SQL connection, `.env` or existing
database is used in this follow-up.

Full input maps cover `src`, `migrations`, `scripts`, `tests` and accounting
documentation before and after each selection. All original mapped bytes remain
unchanged. Complete logs, commands, results and exact executed fixture/runner
source copies are retained below
`bin/tests/p5-b754-followup-20261005/{contracts,shop}`. The failed native attempt
is retained and is not replaced by a synthetic success or weakened retry.

## Registration and pure-reader results

| Selection | Result | Process seconds |
| --- | --- | ---: |
| Writer/audit contracts | 71 methods, exit 0, zero skips | 11.186 |
| Canonical/restore pure classes | 18 methods, exit 0, zero skips | 0.378 |
| Normal validator | Exit 0 | 9.949 |
| Matrix check | Exit 0 | 12.248 |
| Release validator | Exit 1, `writer has no executable evidence` | 0.185 |

These are 89 distinct method executions with zero skips. The six new restore
methods are among the 18 pure methods. The new central row
`plan5_restore_projection_representations_pure` requires those six exact cases.
Semantic comparison verifies all prior 104 rows and their complete policies
are identical; the current manifest contains 105 rows. The standalone policy
receipt is `central-policy-preservation.json`, SHA256
`9c1a69f28daa4e1beea4dd9171cceab1e847c80fcdf5c05f9c1fa26a59caf665`.
The prior six-case registration request is closed by this exact integration.
Normal validation still discloses contract-only validity and incomplete release
coverage. No writer or release status is promoted.

## Actual SHOP link defect and narrow primary handoff

The original SHOP runner exits 1 after 26.971 seconds. Its first policy is
`sql_header_owner`. Compilation reaches the actual linker, which reports:

```text
shop_trade_transaction.c:
  (anonymous namespace)::shop_actor_matches(char_data*, char_data*,
                                          (anonymous namespace)::pending_trade const&)
  undefined reference to `mob_index`
collect2: error: ld returned 1 exit status
```

`shop/shop.log` has SHA256
`d76598e4bec71c0357637ffc57eb112fc4e6ee5b89d9e77d0f618607918cace6`.
The receipt records zero completed compile policies and zero executed cases.
The remaining SQL physical, flatfile owner and flatfile physical policies are
not reached; this is neither their pass nor their observed failure. Their
required cases are not described as skipped passing tests.

The source trace is exact: `shop_actor_matches` at
`src/economy/shop_trade_transaction.c:653` calls `GET_VNUM(keeper)` at line 659;
`src/core/utils.h:375` expands that macro to
`mob_index[(ch)->only.npc->R_num].virtual_number`. `P_index` is
`struct index_data *` (`src/core/structs.h:753`). The full server's provider is
`src/world/db.c:181`; it is not in this bounded component recipe. The physical
fixture already supplies its own `mob_index` and original synthetic mobile
array, while `shop_trade_publication_retention_harness.cpp` does not define
this retained symbol.

Requested primary change: supply the missing fixture-owned `P_index mob_index`
definition in the SQL-only unavailable-root block of
`tests/async/shop_trade_publication_retention_harness.cpp` (currently lines
638–645), following its existing `nullptr` unavailable-root policy. This
request supplies bounded linkage, not native/world census authority. Do not
change the full server's provider or the actual actor/keeper identity check.
If the original cases enter an unavailable capability, retain immediate refusal
or abort; do not manufacture a keeper, successful preparation or recovery proof
to obtain a pass.

Required invariants and consumer checks:

- Preserve all 21 owner and 16 physical case bodies and their complete original
  assertions, the four SQL/flatfile jobs and their 74 required executions.
- Retain all original source providers, strict warning/sanitizer/PIE/GC/crypto
  flags, mysql discovery, CXX choice, 300-second compile limits and 30-second
  case limits. The observed attempt uses those exact limits and flags.
- Keep all outside-scope preparation, native staging, bindings, SQL/world and
  pipeline authority boundaries unavailable. A new link symbol cannot grant
  positive cold restore, native/world proof, publication ACK or restart proof.
- Run the unchanged `tests/async/test_shop_trade_publication_retention.py`
  command on the integrated source and retain all four compile outcomes and
  all 74 actual case outcomes. Diagnose any subsequent failure separately.

No public API, schema, capsule format or accounting data change is requested.
No speculative diagnosis of the three unreached policies is promoted. The
primary's previously documented physical `top_of_objt`/world declaration
concern remains separately unqualified by this first-policy failure.

## Curator handoff and remaining gates

Import this report through the primary's notebook curator with the exact
source, successful registration closure and failed native-link scope intact.
The shared notebook's locality is not a blocker. The primary still owns the
fixture repair, final combined integration and publication.

The missing owner `mob_index` definition is a demonstrated blocker for the
original retained SHOP batch, not for unrelated independent Plan 5 work.
Native EAB2/schema61 installation and measured engine sealing, original
positive cold SHOP callbacks/recovery/ACK and flat parity, original writers and
real player journeys, workload budgets, lifecycle source capture/installation,
erasure and full R1–R8/release remain open. There are no new disposable-database
runs in this follow-up; the preceding report retains the actual both-engine and
flatfile managed evidence at its frozen source.

Accounting stays inactive. Wallet-root item exclusions and the declined
inactive spell-path change remain preserved. No production mutation,
activation, deployment, PR merge or audit autocorrection occurred.

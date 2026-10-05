# Plan 5 original baseline refusal contract and native qualification

The independent SQL origin reader now preserves the original baseline audit's
`EAB1 committed root mismatch` diagnostic when an EAB2 position or forest
decoder refuses evidence. The specific `EvidenceError` catches and promised
`OriginError` boundary from the previous slice remain. Original native baseline
qualification and both-engine origin tests pass on the published canonical0056
source described below. Native EAB2/schema61 and full release stay unqualified.

## Delivery source and ownership

- Branch: `codex/accounting-plan5`; publish only this branch, without force.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Previous published result: `30e8386ae33f45170e13a2b0cd6ed338e4c97f7d`.
- Tested base: `920829257d1d8d9376fc24f625537e5c28c0f62c`.
- Refreshed primary incorporated by a preserving local Git merge:
  `4e84c2784e3af004d030d034226b1920f5d2ac44`.
- Native source tree: `b00968beadaa72d2e126c11d41a92231017e6d27`.
- Migration tree: `1b0f9a40fef29de409338ba83be015cd3390c9f5`, canonical0056.
- Result commit, verified remote tip, final primary refresh, ancestor checks and
  evidence manifest SHA256: `tmp/plan5/baseline-contract-delivery.json`.

Owned files are `scripts/economic_sql_audit_origins.py`,
`tests/async/test_economic_sql_audit_origins.py`, `AUDIT_OPERATIONS.md`, and this
report. No shared source, migration, producer, coordinator, accounting contract,
registry, matrix or activation owner changes. Earlier alternate Plan5 branch
work remains in this branch's ancestry; follow-ups continue here. Historical
reports retain their original source facts and evidence boundaries.

## Established defect and complete fix

The [primary's original qualification handoff](PLAN1_BASELINE_NATIVE_QUALIFICATION_HANDOFF_2026-10-05.md)
specifies the exact original diagnostic. The preceding owned exception repair
used `invalid EAB2 item position`. It correctly refused with `OriginError`, but
did not satisfy the original audit's message assertion. The primary's native
EAB2/schema61 fixture changes remain private; importing its entire proposed test
file would overwrite independent owned changes and misstate installed scope.

With only the owned expectations changed first, all nine `BaselineVersionTests`
ran and eight corruption subtests failed against the previous diagnostic. The
eight failures cover reserved position bytes, an invalid root and an invalid
parent. Exit1, no errors/skips; 7.007 process seconds, 0.131 unittest seconds.
The red log SHA256 is
`11abf7207ddefec49f00bf08f32b8954b859bf8c123e3a0283ceff27518cdcf9`.

The fix changes only two reader message constants and three matching owned test
constants. It keeps specific catches around `position()` and `forest()`, decode
before root validation, controlled selected-book refusal, retained-book unbound
claim findings, rollback/close assertions, and all original native cases. AST
comparison against exact Git-blob preimages proves those constant changes are
the complete Python difference. No broad exception catch or authority repair.

Final reader SHA256:
`03cd00c3735406595846bb1f28b673fb6ad74bb21aae46d6e6408e8ae6e0fa7b`.
This exactly matches the primary's proposed reader. Final owned test SHA256:
`80cec94ca0be926d23fca3e502beca128221e35bd00b12c909bbe3894a51e92d`.
The primary's unpublished whole test file is
not consumed. The stronger `OriginError` helper and additive retained-claim and
negative projection tests from the preceding owned slice remain.

## Executed source, commands and results

All final source maps cover `src`, `migrations`, `scripts` and `tests` before
and after execution. Source stayed unchanged. The pinned image is
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
GCC13.3.0/Python3.12.3, read-only checkout/root, `--network none`. Fresh native
builds explicitly use `DURIS_REGRESSION_BUILD_CACHE=off`, with zero reused objects.

| Actual final selector or command | Result | Process / unittest seconds |
| --- | --- | --- |
| `python3 -u -B -m unittest -v test_reconcile_economy_accounting.ReconciliationTests test_plan5_child_identity.ChildIdentityTests test_economic_sql_audit_origins.ItemRevisionTests test_economic_sql_audit_origins.OriginTests test_economic_sql_audit_origins.BaselineVersionTests test_economic_sql_canonical_audit.CanonicalAuditTests test_economic_sql_canonical_audit.RestoreProjectionTests` | Exit0,191 methods,zero skips | 107.250 /101.881 |
| `python3 -u -B -m unittest -v test_economy_writer_coverage_contract test_audit_accounting_invariants` | Exit0,71 methods,zero skips | 23.273 /13.123 |
| `test_economic_sql_audit_origins.NativeSQLOriginTests` through the retained wrapper | Exit0,2 methods,zero skips | 110.470 /104.250 |
| `test_native_sql_baseline_audit.NativeBaselineAuditTests` through the retained wrapper | Exit0,1 method,zero skips | 421.879 /415.851 |
| `python3 -u -B scripts/validate_economy_accounting.py` | Exit0 | 13.371 |
| `python3 -u -B scripts/generate_economy_writer_coverage.py --check` | Exit0 | 14.337 |
| `python3 -u -B scripts/validate_economy_accounting.py --release` | Expected exit1, `writer has no executable evidence` | 0.144 |

Final total:265 distinct methods/executions,zero skips. Failed red/setup attempts
are outside that total. The105 central registry policies and generated matrix
are unchanged. Isolated passing tests and inventory do not complete release.

### Original native baseline audit, both canonical engines

The original `test_native_sql_baseline_audit.py` and
`run_native_sql_baseline_audit.py` are unchanged. Their SQL and client-free
15-source recipes both compiled freshly with C++20, original strict warnings
and `-Werror`, `-O1 -g`, ASan/UBSan, frame pointers and original link libraries.
Compilation took65.223 seconds SQL and60.769 seconds client-free. Binary SHA256:

- SQL: `9ff48b67a9e4cff9b71e5001e9783eaa0f51f82b3efe278248ddc81ad423f7bf`.
- Client-free: `4bd40802a3f1ef04eb10c7d7ff7e066d298af36a52ebdc011b924bee98439a3a`.

Actual private engines are MySQL8.0.46-0ubuntu0.22.04.4 and
MariaDB10.11.14-MariaDB-0ubuntu0.24.04.1, each freshly migrated to
`0056_spell_ward_durability`. Each original audit passes157 corruption cuts,
157 independent restore refusals and seven native database constraint refusals.
Original selected/retained command binding, keys/hash, root, receipt, source,
revision and missing-evidence cases remain. Controlled damaged imports are
repaired by the fixture owner before the next read-only case. Reader credentials
retain the original mutation-denial check; native authority and source inputs
stay unchanged. Original SELECT-only source grammar/policy, money/UID boundary,
operator limits and orphan evidence checks also pass.

Both engines run original dump/import and a cold independent clone. Full
qualifier, exact native replay,18-table projection preservation and two retained
books pass; active epoch remains NULL. Retained dump identities:

| Engine | Bytes | SHA256 |
| --- | --- | --- |
| MariaDB | 332977 | `1ed0c5dc945cd867d1751fe4fc5f4139af3975166e79c0779a31d5971f3946e8` |
| MySQL | 341737 | `31dca46e57e175e3c51562a090352314a467954b5041bc32b016f9dfc55f4522` |

The original per-engine1200-second bound, native120-second limits, dump/import
and clone deadlines, guard prefix and107-byte Unix-socket bound stay intact.
Transport/preservation wrappers replace only explicit temporary `dir="/"`
with short RAM-backed `/plan5-restore-baseline-p5c`; the resulting path still
starts with the original allowed `/plan5-restore-baseline-` prefix. An ignored
child entrypoint applies that same temporary-directory alias and runs the
unchanged original script via `runpy`. Original and preservation dispatch
commands, unchanged timeout values and complete compiler arguments are retained.
No alternative implementation, source transplant, new provider stub or case waiver.

This is original EAB1/canonical0056 native fixture qualification. All original
markers explicitly retain `complete_native_capture=false` and
`complete_world_capture=false`. The primary's private native EAB2/schema61
161-case result is a different, still unpublished candidate.

### Native origins and projected EAB2 refusals

The20-production-source native authority fixture compiled freshly in84.072
seconds with the original strict sanitizer policy. SHA256:
`9e59042874cb751e8c461a9547440b32484ce7ee76da4c4c7660f55717490b9a`.
Both actual engines pass seven original captures, seven original refusals and
14 rollbacks apiece. Eight malformed copied SELECT projections pass through both
origin and snapshot exporters:16 controlled position refusals per engine,
32 total, each using the original diagnostic, exactly one rollback, closed
cursor, read-only SQL and unchanged database rows. These projections exercise
the EAB2 refusal boundary; physical native authority remains EAB1/canonical0056.
They do not qualify native EAB2 production/replay or schema61 installation.

## Preserved failure, evidence and curator handoff

The first separate origin wrapper omitted `DURIS_PLAN5_PRESERVE_ORIGINS=1`.
It selected the flat-file qualifier rather than the two intended origin tests,
then failed its wrapper assertion. Exit1,244.585 process seconds;286 files are
retained and hash verified. Its unrelated fixture output is not counted as
origin qualification or as a release result. The successor `origins-green`
sets the existing selector explicitly, uses a fresh namespace and passes the
two original methods. No tracked source/test policy changes for this wrapper fix.

Evidence namespace: `bin/tests/p5-baseline-contract-20261005` at physical root
`D:/CodexEvidence/accounting-plan5/bin/bb259-maintained-20261005`. Main artifacts:

- `red`, `pure`, `contracts`, `origins`, `origins-green`, `original`, `gates`:
  exact commands/environments, complete logs, before/after maps and result JSON.
- `original/native/evidence.json`, original binaries and both engine logs;
  `original/preserved` holds disposable source and clone databases, dumps and
  qualification outputs copied before normal temporary-directory cleanup.
- `original/native-commands.json` and `child-dispatches.json` preserve both
  original native recipes and both unchanged1200-second dispatches.
- `origins-green/preserved` retains the native fixture, two disposable database
  trees and all32 controlled negative position observations.
- Per-stage `retention.json`, `source-preservation.json`, exact Git-blob preimages,
  wrapper sources and compiler/runtime identities. The sealed aggregate is
  `tmp/plan5/baseline-contract-evidence.json`; the post-publication receipt is
  `tmp/plan5/baseline-contract-delivery.json`.

Import this report through the primary's notebook curator. Its locally maintained
notebook is nonblocking. No additional accounting interface/schema request.
The preceding narrow registration request remains: add
`BaselineVersionTests.test_invalid_positions_preserve_retained_claim_origin_refusals`
to the existing `plan5_baseline_versions_pure` row in
`tests/integration_manifest.json`, retaining all eight original cases/policies.
The Plan5 owner does not independently edit that shared registry.

The primary must integrate this published reader and requalify its exact combined
EAB2/schema61 candidate. Complete lifecycle/world capture, install/erasure,
retention of real authority, original positive cold SHOP/ACK/recovery and flat
parity, real writer/player journeys, measured production budgets and R1–R8/release
remain open. SHOP source/link/build qualification remains primary owned. No local
blocker prevents this completed owned slice from being integrated. Accounting
stays inactive; wallet-root item exclusions and the declined inactive spell
change stay. No activation, deployment, PR merge, production access/mutation or
audit autocorrection occurs.

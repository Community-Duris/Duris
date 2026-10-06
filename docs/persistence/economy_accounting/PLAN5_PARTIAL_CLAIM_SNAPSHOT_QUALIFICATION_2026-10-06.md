# Plan 5 partial claim snapshot qualification — 2026-10-06

The saved SQL snapshot path omitted partial consumption rows, treated the
immutable original amount as current unspent authority, and considered only
legacy auction-claim debit roots. On both fresh engines the original reader
reported8 copper as unspent while the native claim held6, omitting the generic
two-copper spending root. Separate owned fix
`1047e8c48cb9214d1c3fd76984472201ca80a4fd` captures those rows and independently
reconciles remaining authority and whole/partial debit allocations. It closes
this snapshot omission without changing source amounts, mutation logic or
shared accounting contracts. Complete source capture and release remain open.

## Branch, exact commits and ownership

All work remains on remote `codex/accounting-plan5`, in
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.

| Role | Commit |
| --- | --- |
| Raw full-reader qualification source base | `1b4ab18728e10b302d72ecff38650451a4fac53c` plus the six owned code/test overlays recorded below |
| Separate posting-batch fix | `09b7ceac7925fd9e144302219c0709cafbc7aa07` |
| Separate native-stake recipe correction / this fix's Git base | `53c46b132eb3074d5cd60e1c7a503ff498be10d9` |
| This seven-file fix | `1047e8c48cb9214d1c3fd76984472201ca80a4fd` |
| Latest primary observed during qualification | `26d7b66b86e1a38d09430386be257a065fceacd5` |

Owned files:

- `scripts/economic_sql_audit_snapshot.py`;
- `scripts/reconcile_economy_accounting.py`;
- `tests/async/run_economic_sql_audit_snapshot_mysql.py`;
- `tests/async/run_restore_accounting_evidence_mysql.py`;
- `tests/async/test_economic_sql_audit_origins.py`;
- `tests/async/test_reconcile_economy_accounting.py`;
- `docs/persistence/economy_accounting/AUDIT_OPERATIONS.md`.

Native tree `f0ae5c63273e94035552a75a1b70596d5021e54d` and migration tree
`1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5` remain unchanged. The primary's newer
baseline NULL-price repair changes its native tree; this slice does not qualify
that newer combined candidate. No shared implementation, coordinator,
producer, schema, registry/matrix or activation owner is independently edited.
The prior [selector/PID handoff](PLAN5_CLAIM_ORIGIN_SELECTOR_HANDOFF_2026-10-06.md)
remains the only shared interface request: publish independently usable
authenticated original policy/PID reference facts, invariants and genuine
reference/corruption/replay tests. Private primary progress is not a published
qualified reference and does not block this independent component work.

## Complete component behavior and limits

The version1 diagnostic snapshot adds
`native.pending_claim_consumptions` and its exact
`pending_claim_consumption_coverage: {"rows": N}`. Each row retains exactly
`spending_operation_id`, `source_operation_id`, `source_slot` and positive
`amount`. IDs are nonzero lowercase16-byte hex; slots are exact integers
in1..65535; amounts are exact integers in1..2^64-1. The existing100,000-row
collection and32 MiB input bounds remain. Typed aliases, malformed identities,
unpaired coverage, extra fields and incorrect counts refuse. Historical SQL
snapshots lacking this book emit `missing_pending_claim_consumption_coverage`;
absence does not become an empty allocation history.

Residual authority is original amount minus partial consumption, or zero for
a legacy whole-consumption link. Original amounts remain immutable, including
fully consumed lots. Residual source totals must equal native claim balances.
Only a positive residual requires an active original PID; the original
mapping/native PID must always agree. Negative computed remainders and source
overdraw are findings, never clamped or corrected. Split lots are aggregated
per original root/account when validating its original credit. The separate
posting-batch fix retains summaries across all64-pair queries.

The SELECT-only consumer census includes successful pending-claim copper
debits across reasons and epochs. Whole/partial amounts must match each debit
account and root total. Duplicate source/consumption identities, unattached
references, missing or ambiguous sources, mixed whole/partial use, overdraw,
missing/rejected consumer roots, mismatched receipts and wrong-account debit
allocation produce refusals or explicit findings. Both globally unattributed
IDs are retained as an orphan when neither source nor spending root survives;
unknown lineage is not invented. Detail limits0/1/100 preserve whole-input
counts and refusal status. Readers never import mutation logic or repair a row.

Selected-epoch consumer effects must match the snapshot's independently
decoded original EAP1 plan. Auxiliary roots outside that epoch have SQL
metadata/projection coverage; their original EAI1/EAP1 capsules require the
separate canonical SQL audit under release quiescence. Separate successful
reads do not establish a single complete combined cut. Original opening-policy
selection and original PID source-digest authentication remain distinct gates.
The exporter still emits `backend: sql_partial`, `complete: false`.

## Established failures and passing original methods

Corrected pure red source uses base1b4ab plus only the three initial focused
test methods, with the original reconciler. On Windows,
`PYTHONPATH=tests/async`, this command exits1 with five failures and zero skips
in0.437000 seconds:

```text
python -u -B -m unittest -v test_reconcile_economy_accounting.PartialClaimSnapshotTests
```

Native red uses the same base plus only the new restore-driver overlay and the
original readers. The original full restore command exits1 with two engine
subtest failures and zero skips in386.081385 seconds. Each valid partial cut
shows original lots totaling8, native authority6 and an omitted generic debit.
Native EAI1/EAP1 capsules and fixtures are unchanged.

The repaired full-reader source then passes these original methods:

| Exact command | Result / observer seconds | Scope |
| --- | --- | --- |
| `python3 -u -B -m unittest -v test_reconcile_economy_accounting.ReconciliationTests test_reconcile_economy_accounting.PartialClaimSnapshotTests test_economic_sql_audit_origins.ItemRevisionTests test_economic_sql_audit_origins.OriginTests test_economic_sql_audit_origins.BaselineVersionTests test_economic_sql_audit_origins.PartialClaimExportTests` | Exit0 /45.678747;173 methods, zero skips | Pure retained plan, source, representation, budget and exporter controls; six new partial-claim methods and one new exporter method. |
| `python3 -u -B tests/async/test_restore_economic_coin_effects.py` | Exit0 /408.602654; zero skips | Both fresh canonical0062 engines, original SQL/flatfile native fixtures,109 canonical cuts/90 refusals per engine,58 full-entry cuts,259-root pagination. |
| `python3 -u -B tests/async/test_native_sql_baseline_audit.py` | Exit0 /497.930654; zero skips | Original native SQL/client-free baseline, both engines,161 cuts/seven constraints per engine, actual dump/import, six equipment/custody phases and unchanged229 application-table data. |
| `python3 -u -B -m unittest -v test_reconcile_economy_accounting.NativeStakeSQLTests` on the final composed raw source after the separate recipe fix | Exit0 /98.852455; zero skips | Original full native/SQL stake method, both engines,90 read-only stake captures, eight price captures,24 price CLI cases and all original source/count/result/index/density controls. |

The first three methods passed before the fourth encountered its stale0061
guard. Their observer ultimately exits1; it is not described as a successful
four-method pipeline. Per-method source checks complete after each passing
method. Final source differs in exactly the two native-stake guard/output
lines, unused by those passing pure/restore/baseline methods. The original full
stake method is repeated on the exact final composed source and finishes with
a source-unchanged receipt. No identical whole-transport pass is asserted.

Each engine's25 claim cuts include seven valid controls: unspent, partially
consumed, fully consumed, legacy whole, a split original credit and two retired
mapping controls. Both original retained readers and the SELECT-only snapshot
allocation reader examine each cut. Corruption includes wrong amounts, lost
sources, wrong mapping/lineage/PID, missing/mixed/extra/orphan/unattributed
consumption, rejected consumers and258-source/257-consumption pagination.
All23 captured table contents stay unchanged during audit and return exactly
after private owner repairs. This is data preservation, not unchanged
AUTO_INCREMENT metadata or full-world authority.

The separate67-source SQL metadata probe crosses the64-pair batch boundary;
its copied capsules do not authenticate the new root IDs. Five ordinary claim
history roots use the actual native EAI1/EAP1 encoder/decoder, unchanged fixture
hash `2f0c5dc917aef019c60befbd592c9d0e0a1bf6b0c6bbef828996b9ad077bd635`
and both-mode binary hash
`1f1cde33697efe61bfa8a0d79492bddf351e3c6cee980e31493c8382205b14b8`.
These modeled allocations/native codec controls do not establish an actual
opening/claim producer, lost-reply or cold-recovery journey.

The normal validator, generated matrix `--check`, Python syntax and
`git diff --check` pass. Validator output remains `release_ready=False`;
matrix output remains `coverage_complete=False`, `release=BLOCKED`.
Inventory coverage contributes no release completion. No C++ input changes;
prior fresh production builds apply only to the unchanged native dependency
tree recorded above, with no new whole-source production build claim.

Retained nonqualifying attempts are explicit:

1. The first pure red overlay placed a native integration decorator on the new
   class and skipped three methods. Its helper rejected that observation; the
   decorator was restored to its original class before the retained red above.
2. An early supplemental Windows invocation omitted `tests/async` from
   PYTHONPATH and used the old whole-consumer fixture without account keys:
   four import errors and one failure, tool chunk `1b587c`, with no retained
   file log. Corrected native/pure methods are reported separately.
3. First native green attempt passed173 pure methods, then both engines refused
   a private retired-mapping fixture missing `retiring_operation_id`
   (native constraints4025/3819). The repaired fixture supplies the exact
   retiring root along with NULL active PID. No shared schema defect is asserted.
4. The repaired three-method pass then exposed the stale0061 stake recipe.
   [Its independent two-line fix and qualification](PLAN5_NATIVE_STAKE_0062_RECIPE_QUALIFICATION_2026-10-06.md)
   are a separate commit, followed by the full final stake pass above.

## Exact protected evidence and remaining gates

All native observations use offline image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
two CPUs/4 GiB, private workspace/tmp mounts, no network and no checkout `.env`.
Original compiler/sanitizer profiles, assertions and deadlines remain:
300 seconds for pure methods,1,800 for restore,2,400 for baseline/stake.
Native build cache is off and all native objects are freshly compiled.
Connected engines are MariaDB10.11.14-MariaDB-0ubuntu0.24.04.1 and
MySQL8.0.46-0ubuntu0.22.04.4, with all62 immutable receipts and exact0062 head.
Executed preparation/loader helpers and Docker commands retain exact settings:

```text
PYTHONPATH=/workspace/tests/async
PYTHONDONTWRITEBYTECODE=1
DURIS_REGRESSION_BUILD_CACHE=off
DURIS_RUN_RESTORE_COIN_INTEGRATION=1
DURIS_PLAN5_CANONICAL_EVIDENCE=1
DURIS_RUN_NATIVE_BASELINE_AUDIT=1
DURIS_RUN_STAKE_SQL_INTEGRATION=1
```

Protected directories under `D:/CodexEvidence/accounting-plan5/bin/` are
`partial-claim-snapshot-red-01/02-20261006`,
`partial-claim-snapshot-native-red-01-20261006`,
`partial-claim-snapshot-native-green-01/02-20261006`,
`partial-claim-snapshot-native-stake-green-01-20261006` and
`partial-claim-snapshot-local-checks-01-20261006`. Raw archive SHA256s:

- corrected pure red: `5d5bb93f2a6d0cd1ad15f554f158a44b9ae6ba85b8e29e4c497b2609c395a486`;
- actual native red: `0373aea39b57504284c40ec26acb4d60eb6c6eea6f0fb22a6d2603c6c03882cf`;
- first fixture failure: `d7d80650b04ee6435782d5480c58cd4452b6cb35392d0925f862b253232375aa`;
- three passing methods/stale recipe: `e9849bfa66ab2e19fc0ae7f9b7f73ec8c494087d74ecab96eac3244762db2013`;
- final native stake/composed code: `71bde167bedbe8ef70383a845d956af9c13b82736f985239d78faf6b02fc49a8`.

Original passing log SHA256s are
`065a155179f31d945ac6600d9710fe908e958d221d7a58cf06103338e2e53af4` (pure),
`283567b56622544aa1a7cf4173f0725af014d8b4aea391157c35ae880c449721` (restore),
`a32cf0969db79207051aa935bba015c0abece81e543f967b59ed2deebed3d43e` (baseline),
and `cdf4f274dcaa2317c93faa9836776575040b007b2ff668efbc36624640d0a84a` (final stake).

Seal `partial-claim-snapshot-final-seal-01-20261006/evidence.json`, SHA256
`c1d34602979648d99602a8706b637379327fdad26955bb51cf9f57cd73a79440`,
binds3,125 final committed code inputs,580 retained artifacts/940,160,599 bytes,
raw source modes, original commands/logs/markers, exact terminal states and
dependency limits. Evidence size is not release storage-growth measurement.

This owned report, operator guide, seals and expected remote branch form the
primary's locally maintained notebook curator packet; notebook availability is
nonblocking. All earlier branch work remains followed up here, with the seven
preserved tips as ancestors. Complete R6 live-world/native capture and actual
primary activation verifier, authenticated opening policy/PID reference,
financial producers/replay/lost reply/cold recovery, typed erasure, full managed
backup/restore/retention and release-host mixed-root workload budgets remain
gates. No gate is promoted by synthetic inventory or isolated passes. Accounting
remains inactive, wallet-root item exclusions and the declined inactive spell
change remain intact, and no production mutation, audit correction, deployment,
PR merge or direct experimental-accounting push occurs.

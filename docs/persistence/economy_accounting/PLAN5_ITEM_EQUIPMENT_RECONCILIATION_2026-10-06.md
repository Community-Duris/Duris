# Plan5 independent equipment-custody reconciliation

The independent SQL reader omitted native equipment slots and the two retained
UID-history slot projections. The reconciler therefore missed a slot-only
custody change even when an original EAB2 opening recorded its equipped position.
This slice retains those existing fields and reconstructs equipment custody
without importing mutation logic or changing native/schema contracts.

## Branch, exact source and owned files

- Branch/worktree: `codex/accounting-plan5`,
  `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
- Base: `236d20d9b7af62bb2fc8061e76655c20df0bc86e`; tested primary ancestor
  `d37138dee6361825db594bff2afb07091c773540`, canonical schema61. Earlier0055
  execution is not used to qualify this source.
- Result: the separate defect-fix commit containing this report; the delivery
  receipt records its exact SHA, subsequent normal primary merge, remote tip,
  and preservation of all seven earlier branch heads. All follow-ups remain on
  the same Plan5 branch. Only that branch is pushed.
- Owned implementation: `scripts/economic_sql_audit_snapshot.py` and
  `scripts/reconcile_economy_accounting.py`.
- Owned tests: new `tests/async/test_item_equipment_reconciliation.py`, plus
  `test_economic_sql_uid_scope.py`, `test_economic_sql_audit_origins.py`,
  `test_reconcile_economy_accounting.py` and
  `run_economic_sql_audit_snapshot_mysql.py` under `tests/async/`.
- Owned documentation: this report and `AUDIT_OPERATIONS.md`. Shared producer,
  coordinator, accounting contracts, registry/matrix, activation and schema
  files are not independently edited.

Frozen candidate02 contains the completed native/database probe components;
candidate05 changes only the modeled SQL runner's exact fixture declarations and
expectations relative to those components. Candidate06 corrects the new positive
mobile fixture's context1 to native context0 and reruns all197 pure methods.
Every maintained `src`, `migrations`, `scripts` and `tests` input except the new
test is byte-identical between05 and06. All seven owned source files match the
final checkout. Each
archives contain4085 regular source/helper files, no links, credentials or player
data, and verify every byte before and after execution without a live checkout
bind. This is an audit/native component source archive, not the complete world
runtime input archive.

| Qualified frozen source | SHA256 |
| --- | --- |
| Completed candidate02 archive | `6c0987610fa87a6fc88514f7df2cb990340746d46674206c8684590d97f21655` |
| Final candidate05 archive | `eed1edb08771afe309db3fd2ad9b5a66b69ee78d089c94523bcd03c8c55e5d73` |
| Final candidate06 archive | `045fcf6efccae151fd4764f2168d7d934aa85af4349acdffce388be303e3b857` |
| SQL snapshot reader | `decfb164500bcd92efa673b64efa9e554ec49c17282fb631c8501dde813bba00` |
| Independent reconciler | `7304189a6d0639e737b7e839c7b4f5d6abce81f33c58293d93cd201515a6ca5c` |

The seal retains the exact seven file hashes, complete transports, literal
commands, logs, engine versions, observations and container states. Refresh
found primary `169f9733f69573fc86b07c0dd8cf218787587b4c`: it has imported the
earlier coin-reader fix and full-reader report, and registered the UID minimum11.
Its committed native/migration/scripts/tests differ from this slice's base only
in that manifest minimum; its private birth work remains source-only. The normal
merge and its exact-source proof are recorded separately.

## Established defect and complete reader fix

The original reader at236d accepts a changed current slot5→6 while revision3,
root81, null parent, player owner7 and live state remain exact against the native
EAB2 opening. The repaired reader adds exactly one `stale_native_item`. This is
verified on both real SQL engines against the original schema61/native baseline
books, with the current-owner placements explicitly modeled for the probe.

Existing migration0038 fields are consumed without an interface change:
`item_current_owner.equipment_slot` and
`item_ownership_ledger.from_equipment_slot/to_equipment_slot`. The selected
ownership-event projection already carries both ledger fields; the exporter now
also retains them in lineage and unattributed UID-history census projections.
Native items retain the current slot. A proven revision0 absent creation origin
has slot0, while omitted EAB1 openings remain unknown.

Both independent reconstruction paths compare baseline/current slots and final
history/current slots. Recorded from/to discontinuity produces
`broken_item_equipment_history`; malformed history representations produce
`invalid_item_equipment_slot` and missing-evidence findings while retaining the
original-capsule mismatch diagnostic. Openings/current slots require exact
uint16 scalars; nonzero slots require live, parentless player/native-mobile
custody, with mobile slots at most43. Boolean, float, string, null, negative and
overflow values are not coerced. Missing required evidence produces one
`missing_item_equipment_evidence` per UID whenever another projection records a
slot. All-omitted legacy snapshots retain their historical readability and do
not establish complete equipment proof. Slot and other custody drift together
count one stale UID. Bounded provenance retains safe numeric from/to slots and
omits personal aliases and invalid representations. No audit input or authority
is auto-corrected.

Minimal SQL fixtures now declare the existing current-slot column and use an
explicit original ten-column INSERT list, preserving the native default0. Their
three EAB1 item openings remain unchanged; the expected audit includes all three
missing equipment findings. SQL cursor fixtures supply the newly selected known
slot fields. Existing original child/custody assertions remain unchanged.

## Exact executed checks

The pinned offline image is
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
with Python3.12.3, GCC13.3, MariaDB10.11.14 and MySQL8.0.46. Named containers
use no network/host ports, read-only root, two CPUs,4 GiB memory and private RAM
workspace/database directories. The original native input and completed-component
mounts are read-only. Successful candidates05/06 exit0/OOM=false/error empty.
Candidate02's overall exit1 is retained, because its later modeled fixture
assertion failed; its individually completed components are reused only after
matching their source and log hashes.

Preparation and actual component commands:

```text
python -B tmp/plan5/prepare-item-equipment-qualification.py
python3 -u -B tmp/plan5/item-equipment-original-reader.py
PYTHONPATH=tests/async DURIS_RUN_AUDIT_BUDGET=1 DURIS_PLAN5_CHILD_IDENTITY_BUDGET=1 DURIS_PLAN5_CHILD_IDENTITY_BUDGET_ARTIFACTS=/workspace/bin/tests/item-equipment-child-budget python3 -u -B -m unittest -v test_item_equipment_reconciliation test_economic_sql_uid_scope test_economic_sql_audit_origins.ItemRevisionTests test_economic_sql_audit_origins.OriginTests test_economic_sql_audit_origins.BaselineVersionTests test_reconcile_economy_accounting.ReconciliationTests test_reconcile_economy_accounting.AuditBudgetTests test_plan5_child_identity.ChildIdentityTests test_plan5_child_identity.ChildIdentityBudgetTests
python3 -u -B tmp/plan5/retain-item-equipment-native-plan.py
python3 -u -B tmp/plan5/qualify-item-equipment-native.py
python3 -u -B tests/async/run_economic_sql_audit_snapshot_mysql.py
```

The final command runs separately in each engine's fresh disposable database.
Literal wrapper/compiler/native/read-only SQL commands and private paths are in
the receipts; no production environment is used.

| Check | Result and scope |
| --- | --- |
| Original reader vs new seven equipment methods | Expected RED exit1:48 subtest failures and1 error; demonstrates missing drift, continuity, scalar/position and provenance handling. |
| Frozen focused suite | Current197 methods PASS, zero skips,45.023 s unittest time (candidate06). Earlier candidate02 also passed197/zero skips in47.544 s. Includes both original mapping/price budget methods and the original child budget method. |
| Original native plan/intent recipe | Fresh strict C++20/ASan/UBSan SQL and flatfile modes PASS,14 original goldens per mode,78.970 s wrapper time; flags, cases, assertions and recipe unchanged. This does not qualify equipped gameplay. |
| Genuine native baseline + modeled live slot probe | Both engines PASS; original reader misses slot-only drift, repaired reader reports one stale UID; native source1275 and migration246 hashes match. |
| Complete maintained modeled SQL runner | MariaDB PASS7.074 s; MySQL PASS8.251 s, zero selected skips; existing source grammar/policy, original links, orphan/late-commit/interruption, uint64/overflow/topology, prior-epoch provenance and bounded operator CLI checks retained. |
| Windows focused checks |73 methods PASS:7 equipment,55 coverage/activation,11 UID; zero skips. |
| Normal, writer matrix and runtime metadata | PASS. Release validation correctly exits1 for missing executable writer evidence. Metadata does not prove the full integration matrix. |

Fresh native plan executables/includes and the complete original compiler/run
arguments remain in candidate02 `results/native-plan/`. The original SQL native
baseline binary is reused, not freshly compiled for this probe: SHA256
`72d6d5027b5ed550921efd6b11d14b120dacc1b42d53b6ba32406f96b3f54f2c`;
fixture source hash
`b0e6b3915c6251f96cea7635995ad3a2703a9e3f0889107af34471a7f6240df2`.
Its original MariaDB/MySQL dump pins remain
`db4ff1cc1bf440f8e69fc07d312bf090bafc67e9542dd868c77d6292443769e4` /
`a702b2d39c5c21e44efe466f7b58c899480d242e5b8c026c22386e6e55718f9e`.
Native and migration trees remain `bf7a92a728ad9b5b813626462e56533f8ba39c97` /
`2eb9da7bf64bcd86e05f85d2f4bdf60ef113962d`. No maintained C++ is changed;
no fresh full server build is claimed in this slice.

For each engine, four full maintained snapshot captures and four canonical
captures run under a SELECT-only role. Each snapshot uses64 statements and
rolls back/closes its cursor once; each canonical cut verifies two retained
roots and rolls back/closes once. The role's attempted UPDATE refuses1142.
All228 application table row/content hashes remain identical before/after every
reader phase and original native check. The two probe placements are repaired
only by the fixture owner in that disposable database, including their original
timestamps; the final database matches the intact fixture. Original books and
capsules are never resealed or edited.

Each engine's reused native fixture `--reconcile` passes twice, including during
the slot-only drift. That path checks exact apply replay and immutable original
baseline books, not the live current placement. The repaired independent reader
supplies the missing live comparison. All baseline-only reports still preserve
`evidence_loss:1` and `missing_native_holding:2`; no complete capture is inferred.

All three selected component budget methods retain their original30 s/256 MiB
bounds and limit0/1/100 count invariants. The child fixture is exactly32 MiB,
500 modeled roots/32000 children/postings plus padding; observed times are
0.603118/0.599913/0.611902 s and cumulative child peak181488 KiB on the final
candidate06. Earlier candidate02 measurements remain retained separately. These are
synthetic component measurements, not the required release-host/full mixed
authority workload. The attempted raw child input copy returned directory not
found; the original fixture source, measurements and log are retained, but no
raw input artifact preservation is claimed.

## Retained failures, evidence and shared handoff

Evidence directories are
`D:\CodexEvidence\accounting-plan5\bin\item-equipment-reconciliation-01-20261006`
through `-06-20261006`. Failed candidates are retained and not relabeled passing:
01 had two incomplete SQL cursor fixtures, a masked original diagnostic, and
one child-budget skip;02 lacked slot0 in an exact modeled creation-origin
expectation;03 expected one instead of three EAB1 evidence gaps;04 reached later
positional INSERTs lacking an explicit column list. Candidate05 passes both
engine runners; candidate02's verified completed components retain their own
PASS receipts. Initial Windows117 methods failed4/skipped3 and the original
seven-method RED reproduction are separately retained. The first06 preparation
guard refused the newly edited owned documentation path before an archive or
container was created. Its receipt remains retained; the corrected guard allows
that documentation edit while freezing runtime inputs independently.

The final seal `tmp/plan5/item-equipment-evidence-final.json`, SHA256
`4850f2d07a0bfe85a11611c2ffff36ab00106730b1dfd0f512d1f5b62135c5e8`,
verifies216 artifacts across all six directories and links the unchanged prior
seal `item-equipment-evidence-v02.json`. That prior seal preserves the original
seal and records a narrow correction of its three named regression-consumer
paths. The post-publication delivery receipt records exact commit/source
correspondence and the verified remote branch.

Only one new primary-owned interface request remains: add
`test_item_equipment_reconciliation.py` to `tests/regression_manifest.json` with
`profile:fast`, `mode:unittest`, `minimum_cases:7`, `cpu:1`, `memory_mb:256`,
`seconds:0.03`, and purpose describing slot drift, transition continuity,
unknown openings, native scalar grammar and bounded provenance. Preserve every
existing manifest field and all105 integration rows. Consumers are
`tests/regression_inventory.py`, `tests/run_test_entry.py` and
`tests/run_regression_tests.py`. The invariant is that central discovery executes
all seven methods and cannot accept a reduced suite. Registration is a pending
central gate; direct methods pass independently. The earlier UID minimum9→11
request is fulfilled on primary92dfb76fe and carried by the subsequent normal
merge. No new SQL column, wire format, producer contract or public schema is
requested.

The defect is solved and has no local delivery blocker. Full combined native
capture, complete writer/refusal coverage, active erasure, real player/world
journeys, complete recovery/retention qualification, declared release-host
workloads and primary private birth installation/qualification remain open.
This component work does not establish Plan5/R8/release completion. Accounting
remains inactive; wallet-root item exclusions and the declined inactive spell
change remain preserved. No production data mutation, audit autocorrection,
activation, deployment, PR merge or experimental-accounting push occurs.

This report, its exact-source integration follow-up and evidence/delivery
receipts form the required curator packet for the primary's locally maintained
shared project notebook. The user confirms that notebook workflow is nonblocking;
shared notebook/coordinator files are not independently edited here.

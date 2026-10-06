# Equipment audit fix integrated and registered — 2026-10-06

Primary imports the completed Plan5 equipment-custody reader/reconciler fix from
`a781ab425d568f82c31fbea59b909999df1448b9`, including its separate defect commit
`9e91f5986027b628a4b42078f70b5197d52a6c86`. All ten owned source/test/document
blobs are exact; each existing preimage matches the peer's base236d20d9b and the
primary's committed source. No Git history merge or shared native change occurs.

The reader previously missed a current equipment-slot change with otherwise
identical UID, revision and custody. It now retains existing native and ledger
slot fields. Both reconciliation paths detect slot-only drift and discontinuous
from/to history. Invalid scalars refuse, omitted historical evidence stays unknown,
and bounded provenance retains valid slot numbers without personal aliases.
No native/schema/producer contract or authority is changed.

The [original defect and qualification report](PLAN5_ITEM_EQUIPMENT_RECONCILIATION_2026-10-06.md)
preserves the failing original control, final197 Linux methods, both real-engine
read-only/native-baseline probes, strict native component modes, failed candidates
and exact evidence seals. The [peer integration handoff](PLAN5_EQUIPMENT_PRIMARY_INTEGRATION_2026-10-06.md)
preserves its Windows73-method and metadata scope. Primary source correspondence
reuses those results at their qualified component scope; there is no new primary
database, native compiler, gameplay or restart execution in this import.

Primary adds only the requested new regression-manifest row: fast/unittest,
minimum7, CPU1,256MiB, seconds0.03, with the original purpose. All existing
manifest fields remain unchanged, including the UID minimum11. All105 integration
rows and their manifest remain unchanged. Complete discovery now accepts all918
regression entries, including the actual new test; this is inventory evidence,
not a full regression execution.

Actual original central adapter commands:

```text
python tests/run_test_entry.py tests/async/test_item_equipment_reconciliation.py unittest tmp/plan5-equipment-primary-7-cases-20261006.json 7
python tests/run_test_entry.py tests/async/test_economic_sql_uid_scope.py unittest tmp/plan5-equipment-primary-uid-11-cases-20261006.json 11
```

Both exit0:7 equipment methods and11 affected UID methods pass, zero skips. Their
case records retain every collected method and show complete execution. Normal
accounting metadata, generated writer-matrix check and current61 runtime metadata
also pass. The matrix remains `coverage_complete=False`, `release=BLOCKED`;
metadata and these focused cases do not qualify all writers or full accounting.

Exact primary source identities:

| Source | SHA-256 |
| --- | --- |
| SQL snapshot reader | `decfb164500bcd92efa673b64efa9e554ec49c17282fb631c8501dde813bba00` |
| Independent reconciler | `7304189a6d0639e737b7e839c7b4f5d6abce81f33c58293d93cd201515a6ca5c` |
| New seven-method test | `6d12caf058217ee6c58169a1ca7c9cae0c3eb2c65b4672e54fa93aa5fc68df3c` |

`tmp/plan5-equipment-primary-import-20261006.json` records every imported blob,
original preimage and unchanged native/migration tree. Its SHA-256 is
`f5e391fe567caea7f993cffd068e021acd967ce085feb2a4c51daa4903b0f6a0`.
`tmp/plan5-equipment-primary-registration-20261006.json` records registration,
inventory and actual case-result hashes. Maintained native tree remains
`bf7a92a728ad9b5b813626462e56533f8ba39c97`, migrations
`2eb9da7bf64bcd86e05f85d2f4bdf60ef113962d`.

The equipment defect and requested central registration are solved. Complete
native holdings/UID capture, original producer/world journeys, mixed release-host
workloads, lifecycle/restart qualification and all full Plan/R1–R8 gates remain
open. Private birth candidates receive none of this native component evidence.
Inactive behavior, declined spell-path change and unrelated SHOP work are preserved.

## Cold-restore source integration — 2026-10-06

Primary imports six owned blobs byte-exactly from peer
`7c0d6cf6cf475e717aa1e68d207bcb376e939980`, including the original baseline
runner's appended equipment helper and both qualification/integration reports.
The independently solved qualification gap is reported at
`8a9b4c844c130b4d28910d6a97f59528cb84f1ec`.

All primary native, migration and script trees equal the qualified peer;
the only pre-import test differences are the three imported test/helper paths.
After import every code/test input matches that peer composition, and the
entire regression manifest remains byte-identical. Its original integration
owner, case count, markers, provider and900-second central timeout are preserved.
All three Python sources parse. Normal accounting metadata, the matrix check
and current61 runtime metadata exit0 on this exact primary composition.

Import receipt `tmp/plan5-equipment-cold-primary-import-20261006.json`:
SHA256 `cfd3a5adada6b770c2c4960681c9dfe61e111aa86b499d7f630536c93c7ebead`.
Actual primary metadata output receipt
`tmp/plan5-equipment-cold-primary-metadata-20261006.json`:
SHA256 `972333116e66c06d081caa544fea382a06081e6a5f7c5732cc32355204ba0cfd`.

Peer's complete original native recipe passes on both engines, zero skips.
Its eight SELECT-only captures, four new cold dump/imports and24 bounded CLI
observations qualify equipment drift preservation at that recorded scope.
Actual native opening books are retained; current equipment positions are
modeled inputs and the reader preserves partial-capture findings. Protected
peer logs/dumps remain peer-held; primary verifies committed source/report
correspondence and does not claim a new native run or independently reading
those protected logs. This is no complete world backup, service boot, managed
backup generation, active producer journey, private birth qualification or
full Plan/release completion. Existing inactive behavior and gates remain.

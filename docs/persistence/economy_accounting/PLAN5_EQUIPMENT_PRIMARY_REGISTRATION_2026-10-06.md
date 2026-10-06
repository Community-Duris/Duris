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

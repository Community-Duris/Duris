# Plan5 equipment fix: current primary integration and registration handoff

The [independent equipment reconciliation fix](PLAN5_ITEM_EQUIPMENT_RECONCILIATION_2026-10-06.md)
is committed separately as `9e91f5986027b628a4b42078f70b5197d52a6c86` on
`codex/accounting-plan5`, over base
`236d20d9b7af62bb2fc8061e76655c20df0bc86e`. Its owned nine files, final197-method
Linux qualification, two SQL engines, original sanitizer modes, failed attempts
and evidence scope remain recorded in that report.

The refreshed primary tip `169f9733f69573fc86b07c0dd8cf218787587b4c` is normally
merged as `0772f26c0c62eaeb0f481630131876c1e93756f9`. Parents are exactly the
defect-fix commit and that published primary tip. The merge imports six primary
documentation changes and the single existing UID manifest minimum9→11; all
other manifest fields are exact. The manifest bytes match the primary. Native,
migration, maintained scripts and all executable tests remain byte-identical
against the separate defect-fix commit and its frozen candidate06.

Two owned-file conflict blocks arose from the primary's cherry-picked earlier
coin-reader fix overlapping the new native-slot SELECT and explicit fixture
INSERT. The resolutions retain the qualified equipment fields and fixture
column list. The primary's published native/migration/scripts/tests relative to
the defect-fix base differed only in the UID manifest minimum, so no additional
production change was discarded. The resolved files match the exact qualified
source hashes. Shared source, contracts, coordinator, registry/matrix, activation
and schema files were not independently edited.

Branch/worktree remains `codex/accounting-plan5` at
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`. This report is
the only owned file added after that merge. Its publication commit and verified
remote SHA are in the post-publication delivery receipt; all seven older branch
tips remain ancestors. Work and follow-ups remain on this remote branch.

Exact-source and post-merge command:

```text
python -B tmp/plan5/qualify-item-equipment-integration.py
```

The observer verifies the two merge parents, only seven imported paths, all
seven owned code/test hashes, unchanged native/migration/script/test trees except
the manifest, and exact manifest field/primary byte correspondence. It then runs:

```text
python -B tests/async/test_economy_writer_coverage_contract.py
python -B tests/run_test_entry.py tests/async/test_item_equipment_reconciliation.py unittest <evidence>/equipment-cases.json 7
python -B tests/run_test_entry.py tests/async/test_economic_sql_uid_scope.py unittest <evidence>/uid-cases.json 11
python -B scripts/validate_economy_accounting.py
python -B scripts/generate_economy_writer_coverage.py --check
python -B scripts/validate_runtime_compatibility.py
python -B scripts/validate_economy_accounting.py --release
```

All73 focused Windows methods pass with zero skips, including actual test-entry
adapter execution of all7 equipment and11 UID methods. Normal, matrix and runtime
metadata checks pass. Release validation preserves expected exit1:
`writer has no executable evidence`. The actual complete inventory function also
preserves expected exit1 for the sole unclassified new equipment test; no files
are missing. This is a pending primary-owned registration gate, not a successful
central regression run. No database/native rerun is attributed to this merge:
the actual previously qualified components are reused with exact-source proof.
No full integration matrix, complete capture or release completion is claimed.

Primary registration request in `tests/regression_manifest.json`:

```json
"test_item_equipment_reconciliation.py": {
  "profile": "fast",
  "mode": "unittest",
  "minimum_cases": 7,
  "cpu": 1,
  "memory_mb": 256,
  "seconds": 0.03,
  "purpose": "Equipment-slot drift, transition continuity, unknown openings, native scalar grammar and bounded provenance."
}
```

Preserve every existing entry/field and all105 integration rows. Consumers are
`tests/regression_inventory.py`, `tests/run_test_entry.py` and
`tests/run_regression_tests.py`. The invariant is complete discovery and actual
execution of all seven methods; a reduced suite must not satisfy the owner.
The two adapters and their retained case records already prove the requested
7/11 minimum execution behavior. The earlier UID minimum request is closed by
this import. No SQL/schema, producer, wire or accounting-contract change is
requested.

Evidence is
`D:\CodexEvidence\accounting-plan5\bin\item-equipment-primary-integration-20261006`.
The integration seal `tmp/plan5/item-equipment-primary-integration.json`, SHA256
`07467e2924c62640d49a60619b29565c677542e578deea9d3ab713f128862d36`, verifies12
artifacts including complete command logs and actual adapter case records.
The underlying final source seal SHA256 is
`4850f2d07a0bfe85a11611c2ffff36ab00106730b1dfd0f512d1f5b62135c5e8`, covering216
retained artifacts across the original six candidates. The delivery receipt
reverifies those artifacts and records the exact remote result.

There is no local delivery blocker. New-owner registration, complete native
holdings/UID capture, shared writer/refusal and active erasure coverage, original
recovery/retention and real player/world qualification, declared release-host
workloads and the primary's private birth installation/qualification remain
open. Accounting stays inactive; wallet-root item exclusions and the declined
inactive spell-path change remain. No production data mutation, audit
autocorrection, activation, deployment, PR merge or experimental-accounting push
occurs. This report and its receipts supply the required curator packet for the
primary's locally maintained shared notebook, confirmed nonblocking by the user.

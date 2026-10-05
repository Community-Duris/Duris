# Plan 5: selected operation count format validation

Branch `codex/accounting-plan5`; worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Frozen tested base is `6a2428cff3031e1a8d384644667910349cbddbc3`.
Primary refreshed before this slice was
`9155b623419b960c17f117ed8c87d8f783ce42a1`.
Native tree is `8352e470e9d32bee2fc84cb90097d0ecfb87ce2d`; migration tree is
`1b0f9a40fef29de409338ba83be015cd3390c9f5`, including canonical 0056.
The result commit and verified remote SHA are supplied in delivery.

## Established defect and complete owned fix

The independent reader compared selected operation cardinalities with Python
equality without first checking their types or native limits. Equivalent floats
and booleans could match integer evidence lengths. Six serialized synthetic
cuts returned zero findings: four equivalent floats, `child_count=False`, and
`item_event_count=True`. The corrected native commerce/stake reproduction also
admitted six equivalent representations on each canonical0056 database through
all 36 API checks and 72 CLI checks, with no input or source-table writes.

The reader now validates every selected operation row before indexing, including
duplicate rows that would otherwise be dropped. Each field must be a strict
integer in the existing native range:

| Field | Inclusive range |
| --- | --- |
| `account_count` | 0–3072 |
| `posting_count` | 0–6144 |
| `child_count` | 0–64 |
| `item_event_count` | 0–3000 |

These limits already exist in `economic_accounting_types.h:15–18` and the
`economic_accounting_operation` constraint in `migrations/economy_accounting.sql`.
Invalid representations raise `SnapshotError("invalid operation <field>")`.
The CLI returns 2, emits no JSON, and prints only the bounded field label.
Valid integer disagreements retain the existing `evidence_count_mismatch`
finding. The fix does not coerce values or import the mutation implementation.
Input byte/row bounds, output limits, inactive behavior and schema stay intact.
This slice validates the four selected-root count fields; it does not establish
complete validation of every numeric field in all retained projections.

Owned files are `scripts/reconcile_economy_accounting.py`,
`tests/async/test_reconcile_economy_accounting.py`, and this report. No shared
interface, native source, schema, producer, registry or coordinator change is
requested for this repair. Tested Python SHA-256 values are:

- Reader: `b0adb68985912ac4a8975e766d28fecbe51967116f98e7c14af0f957dd4de207`.
- Tests: `5914202eff540e1a755c7a156c84c1a5be92e777362ebc5173d3dff5fd0ba6b1`.

## Exact execution and preserved failures

Docker image `duris-plan5-origin-sql-tools:local` is pinned to
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`:
Ubuntu 24.04.4, GCC 13.3, Python 3.12.3, OpenSSL 3.0.13,
MariaDB 10.11.14 and MySQL 8.0.46. Each invocation uses `--network none`, a
read-only `/workspace` mount and a writable `/workspace/bin` mount. Databases
are fresh private datadirs with TCP disabled; no production environment is read.

The focused pre-fix unit command, with `PYTHONPATH=/workspace/tests/async` and
`PYTHONDONTWRITEBYTECODE=1`, was:

```sh
python3 -m unittest -v \
  test_reconcile_economy_accounting.ReconciliationTests.test_selected_operation_count_types_and_native_bounds_refuse \
  test_reconcile_economy_accounting.ReconciliationTests.test_selected_operation_count_boundaries_keep_cardinality_findings \
  test_reconcile_economy_accounting.ReconciliationTests.test_selected_operation_count_cli_refusal_is_bounded_and_private
```

It exits 1: three tests, 72 expected failing assertions, 6.872 seconds.
The six zero-finding synthetic inputs and their hashes are retained in
`bin/tests/plan5-root-count-types/unit-red-proof.json`.

The first native attempt selected the baseline-owner fixture:
`python3 -u -m unittest -v test_native_sql_baseline_audit.NativeBaselineAuditTests`.
It exits 1 after 281.4355094 seconds (279.112 unittest seconds): both engines
failed in setup with `IndexError` because that fixture's normal selected-operation
export is empty. This run did not demonstrate count refusal or admission. Its
sources, logs and artifacts are preserved; both baseline harness files were
restored byte-for-byte to their base Git blobs and have no result diff.

The corrected native command, used before and after the repair, was:

```sh
python3 -u -m unittest -v tests.async.test_reconcile_economy_accounting.NativeStakeSQLTests
```

Both runs set `DURIS_RUN_STAKE_SQL_INTEGRATION=1`. The RED invocation additionally
sets `DURIS_PLAN5_COUNT_TYPES_RED=1`, `DURIS_PLAN5_COUNT_TYPES_ONLY=1`, and
`DURIS_PLAN5_PRICE_AUDIT_ARTIFACTS=/workspace/bin/tests/plan5-price-view-sql/count-types-red`.
It exits 0 after 199.5914032 seconds (197.231 unittest seconds), proving
`NATIVE_COUNT_TYPES_RED_ADMITTED`: 12 malformed exports, 36 API and 72 CLI checks,
both engines, seven authority tables unchanged and accounting inactive.

The GREEN invocation sets
`DURIS_PLAN5_PRICE_AUDIT_ARTIFACTS=/workspace/bin/tests/plan5-price-view-sql/count-types-green`,
without either RED/ONLY flag. It exits 0 after 208.8994259 seconds
(206.903 unittest seconds), zero skips. All 12 malformed exports now refuse
through 36 API and 72 CLI checks at limits 0, 1 and 100. The existing full matrix
also passes: 90 read-only captures/rollbacks, 72 fault captures, two UPDATE
permission denials, 104 native source grammar cases, 1,107 policy decisions,
six original-link cases, four price roots across two epochs, 24 price CLI cases,
and 16 shared price-projection comparisons. Seven authority tables are compared
before/after every capture and altered export; inputs remain unchanged and
`active_epoch` remains NULL on both engines.

Native probes are freshly compiled in SQL and `__NO_MYSQL__` modes with
`-std=c++20 -Wall -Wextra -Wpedantic -Werror -O1 -g
-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie -Isrc`,
linking `-lcrypto`. The eight actual source inputs are accounting plan, types
and intent; critical command; item transfer command; craft pouch mutation;
chaos pouch ledger; and player snapshot codec. Generated probe source and both
executables are retained in each artifact directory. ASan leak/error stopping
and UBSan error stopping are enabled. Binary hashes are identical across the
two modes within each run:

- RED executable: `c769295a7bbd9cdae9363d6d952e1b50f009509ec8d329ca6f2ae283cd917dfc`.
- GREEN executable: `aae47cd8ad29d45c47e51986082d2ebd52034131f4e8df980830d035c9e93719`.
- Encoded native output, all four executions:
  `1ed10a94436e14271b6bc07d8f17ebf364b10dcd770e1da1c6738df1d02c7281`.

The complete pure unit class passes, zero skips:
`python3 -u -m unittest -v test_reconcile_economy_accounting.ReconciliationTests`:
102 tests, 23.779 unittest seconds, 25.0703686 outer seconds, exit 0. Coverage
includes missing fields, booleans, floats, nulls, containers, private strings,
negative/over-limit counts, nonfinite floats, valid native boundary values,
duplicate rows, CLI privacy and byte-preserving inputs.
`python3 -m py_compile scripts/reconcile_economy_accounting.py
tests/async/test_reconcile_economy_accounting.py` and `git diff --check` pass.

## Evidence and limits

Logs and command metadata are in `bin/tests/plan5-root-count-types/`:
`unit-red.log`, `unit-green.log`, the failed `native-red.log`, corrected
`native-red-corrected.log`, `native-green.log`, their command JSON files,
source snapshots and `source-and-runtime.json`. Native/database artifacts are
in the RED/GREEN directories above; the failed baseline artifacts are in
`bin/tests/plan5-baseline-sql-restore/count-types-red/`. These paths are ignored
and no binaries, logs, datadirs, player data or environment files are committed.

Sealed manifest: `tmp/plan5/root-count-types-evidence.json`, SHA-256
`54bd795b6647b1d6f2601cf758dc5e6f2487e06303e4c77f8cf65fee74399b2f`.
It records 573 fresh artifacts and verifies all 1,490 native/migration inputs
(1,254 native plus 236 migration files), unowned tracked Python/test inputs,
and all 30,354 prior artifacts unchanged. The initial/final preservation
records, rejected fixture sources, corrected RED source and final GREEN source
are retained. The two baseline harness files are listed in the recorder's
temporary edit whitelist but their final contents equal the base blobs.

The native commerce/stake harness uses real native encoded plans and actual
canonical0056 SQL evidence, merged with its independently modeled holdings and
origin oracle. It establishes structural reader behavior and permission/snapshot
discipline. It does not publish through the full runtime writer, capture a real
complete world, run gameplay, prove a complete mutation journey, activate
accounting, or qualify all R1–R8. There are zero skips in the executed GREEN
classes; the setup failure remains a failed attempt, not a passing test.

Primary advanced during this frozen run to
`abd6e32cd0ba41e0fdbcd0b2cd0ee5bbba847cd7`, including the published collector
comparison repair `103fd07c384ac84ba104cbbc0d90167f6b57db5f`. That candidate has
native tree `e018587932abef86ef3bcab9d61a6f3be3afe933` and the same migration tree.
This slice does not qualify that different combined native candidate. Its
deferred maintained SQL build and managed restore checks remain next work.
Full baseline CCM1 authentication still needs the primary-owned original
admission-time field; remaining native capture/mapping coverage, Plans 2–4
producer qualification and complete release acceptance remain open. Synthetic,
inventory and isolated results do not close those gates. Wallet-root item
exclusions and the declined inactive spell-path change are preserved.

This report is an evidence handoff for the primary's local notebook curator.
The shared notebook is maintained on that system; no notebook block or remote
notebook update is claimed here.

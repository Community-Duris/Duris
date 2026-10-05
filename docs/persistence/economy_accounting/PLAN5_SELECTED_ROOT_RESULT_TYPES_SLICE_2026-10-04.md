# Plan 5: selected operation result-code format validation

Branch `codex/accounting-plan5`; worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Frozen tested base is `bfa5d0723e26e4d7c1de18128d80cef0c1d94c84` after the
ordinary import of refreshed primary
`d8319bf616538d564a54ab2b0b15d0f835175a7a`. That primary advance contains
documentation only; native tree remains
`e018587932abef86ef3bcab9d61a6f3be3afe933`, migration tree
`1b0f9a40fef29de409338ba83be015cd3390c9f5`, including canonical 0056.
The base was normally pushed to the Plan 5 branch. Result commit and verified
remote SHA are supplied in delivery.

## Established defect and complete owned repair

Selected operation result codes were compared with zero and the strict-integer
receipt using Python equality, without validating their own representation or
native width. Six serialized synthetic cuts returned zero findings:
committed `0.0`/`False`, rejected `9.0`/`True`, and rejected `-1`/`4294967296`
with matching integer receipts. These are impossible native result-code
representations but were accepted as reconciled evidence. Input objects and
serialized files remained unchanged during the reproduction.

The native contract is already uint32: `flatfile_accounting_record::result_code`
in `src/flatfile/flatfile_accounting_store.h:29` and `INT UNSIGNED` in
`migrations/economy_accounting.sql:85`. The existing outcome constraint requires
zero for committed operations and nonzero for rejected operations.

The reader now checks every selected operation row before indexing, including
duplicates: `result_code` must have `type(value) is int` and lie in
`0 <= value < 2**32`. The three-line repair raises
`SnapshotError("invalid operation result_code")` on malformed or missing values.
The CLI returns 2, prints no JSON and emits only the bounded field label.
Valid uint32 values still produce the existing `invalid_result_code` finding
when inconsistent with the outcome. No coercion, mutation import, adjustment,
new status, schema or envelope change is introduced.

Owned files are `scripts/reconcile_economy_accounting.py`,
`tests/async/test_reconcile_economy_accounting.py` and this report. No shared
interface or schema change is requested. Final tested SHA-256 values are:

- Reader: `952b1fe59c206cec50c2ea4dc7aa678a9855c28ed37164eebd678693273e7fe8`.
- Tests: `6e6e3f16a2656cabf858f61dc78dbcf708bdc8f0d57ee0baa3e486c968e011ec`.

## Exact tests and retained failed attempts

All invocations use Docker image `duris-plan5-origin-sql-tools:local`, immutable
ID `sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
with Ubuntu 24.04.4, GCC 13.3, Python 3.12.3, OpenSSL 3.0.13,
MariaDB 10.11.14 and MySQL 8.0.46. Network is disabled, `/workspace` is read-only
and `/workspace/bin` is writable. Database evidence uses fresh private datadirs
and TCP-disabled sockets; no production environment or data is read.

The pre-fix focused command sets `PYTHONPATH=/workspace/tests/async` and
`PYTHONDONTWRITEBYTECODE=1`:

```sh
python3 -m unittest -v \
  test_reconcile_economy_accounting.ReconciliationTests.test_selected_operation_result_types_and_uint32_bounds_refuse \
  test_reconcile_economy_accounting.ReconciliationTests.test_selected_operation_result_boundaries_keep_outcome_findings \
  test_reconcile_economy_accounting.ReconciliationTests.test_duplicate_operation_cannot_hide_invalid_result_code \
  test_reconcile_economy_accounting.ReconciliationTests.test_selected_operation_result_cli_refusal_is_bounded_and_private
```

Initial RED: exit 1, four tests, 62 failures, 5.531 seconds. The missing-field
checks were then enclosed in individual subtests so a failed missing field did
not stop later outcome/limit cases. Expanded RED: exit 1, four tests,
127 failures, 6.667 seconds. Both logs are retained. The separate six-cut
zero-finding reproduction is `unit-red-proof.json`, with exact input hashes.

The first native invocation stops at a syntax error in the owned harness
extension before test execution: exit 1 after 1.8539739 seconds. Its source and
log are preserved. Indentation was corrected and `py_compile` passed before
the corrected native invocation. No native result is attributed to that failure.

Both corrected native invocations run:

```sh
python3 -u -m unittest -v tests.async.test_reconcile_economy_accounting.NativeStakeSQLTests
```

They set `DURIS_RUN_STAKE_SQL_INTEGRATION=1`. RED additionally sets
`DURIS_PLAN5_RESULT_TYPES_RED=1`, `DURIS_PLAN5_RESULT_TYPES_ONLY=1` and
`DURIS_PLAN5_PRICE_AUDIT_ARTIFACTS=/workspace/bin/tests/plan5-price-view-sql/result-types-red`.
It exits 0 after 217.3956145 outer seconds (214.868 unittest seconds), proving
four false-clean result-code exports from actual canonical0056 SQL evidence
through 12 API and 24 CLI checks on both engines. The previous count repair
also refuses all 12 malformed count exports through 36 API and 72 CLI checks.

GREEN uses artifact directory
`/workspace/bin/tests/plan5-price-view-sql/result-types-green` without either
RESULT RED/ONLY flag. It exits 0 after 212.8304862 outer seconds
(210.700 unittest seconds), zero skips. All four malformed result-code exports
now refuse through 12 API/24 CLI checks at limits 0, 1 and 100. The complete
existing matrix passes: 90 read-only captures and rollbacks, 72 fault captures,
two UPDATE permission denials, 104 native source grammar cases, 1,107 policy
decisions, six original-link cases, four price roots across two epochs,
24 price CLI cases and 16 shared price-projection comparisons. The count proof
also stays green. Seven authority tables and all inputs are unchanged after
every capture/altered export; `active_epoch` remains NULL on both engines.

The native probes are freshly built from the eight accounting/critical-command/
item/pouch/player-codec inputs retained in the test source, in SQL and
`__NO_MYSQL__` modes, with strict C++20 warnings, `-Werror`, ASan/UBSan,
`-O1 -g`, leak/error stopping and frame pointers. Generated C++ and executables
are retained. Within each run both modes have the same executable hash:

- RED: `edb0ffd0cd90c3cfa558af2a83efff1689750e8555eb4a26d9e50921a3f3a817`.
- GREEN: `5afbfe85379fcfc6d174032ea0fb1becc8c58124f65f2f7f906bb544c1455225`.
- Encoded native output, all four executions:
  `1ed10a94436e14271b6bc07d8f17ebf364b10dcd770e1da1c6738df1d02c7281`.

The complete pure unit command passes, zero skips:
`python3 -u -m unittest -v test_reconcile_economy_accounting.ReconciliationTests`:
106 tests, 30.887 unittest seconds, 31.8601986 outer seconds, exit 0. It covers
both outcomes, booleans, floats, negative/overflow integers, null/missing values,
containers, private strings, nonfinite floats, duplicate rows, valid uint32
boundaries, existing outcome findings, bounded CLI privacy and immutable inputs.
`python3 -m py_compile scripts/reconcile_economy_accounting.py
tests/async/test_reconcile_economy_accounting.py` and `git diff --check` pass.

## Evidence and qualification limits

Logs, command JSON, source snapshots, the driver/recorder and source/runtime
metadata are in `bin/tests/plan5-root-result-types/`. Native evidence and altered
exports are in `bin/tests/plan5-price-view-sql/result-types-red/` and
`result-types-green/`, including `result-type-results.json` and
`count-type-results.json`. All artifacts stay ignored; no generated data,
binary, log, datadir, environment or archive is committed.

Sealed manifest: `tmp/plan5/root-result-types-evidence.json`, SHA-256
`e6332501a0a005ace8a69932af8cef51408a7d88f9b5b7f471e63a86eededfed`.
It records 555 fresh artifacts and verifies all 1,490 native/migration inputs
(1,254 native plus 236 migration files), unowned Python/test inputs and all
35,699 prior artifacts unchanged.

`python3 scripts/validate_economy_accounting.py` passes: 14 fixtures,
886 writer routes and 2,843 candidate sites, `release_ready=False`.
`python3 scripts/generate_economy_writer_coverage.py --check` passes:
`coverage_complete=False`, release `BLOCKED`. No unchanged failing release
run or unchanged maintained server build is repeated for this Python repair.

The native harness combines real encoded plans and canonical0056 SQL evidence
with modeled native holdings/origins. This proves structural reader behavior
and read-only discipline, not full runtime producer publication, complete native
capture, gameplay or activation. The synthetic rejected-root range cuts are
distinct from the native committed-root alias proof. The preceding maintained
SQL build/cold-restore report remains qualified only for its exact inputs.
Full baseline CCM1 authentication still needs the primary-owned original
admission-time field. Remaining native capture/mapping coverage, Plans 2–4
producer qualification, combined-candidate acceptance, retention/replica gates
and applicable R1–R8 requirements remain open. Wallet-root item exclusions,
inactive behavior and the declined inactive spell path are preserved.

This report is a handoff for the primary's local notebook curator. The shared
notebook is maintained on that system and does not block this work; no remote
notebook update is claimed.

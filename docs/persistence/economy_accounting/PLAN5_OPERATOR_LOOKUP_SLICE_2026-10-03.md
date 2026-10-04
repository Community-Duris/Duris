# Plan 5: exact operation and native account lookup

Branch: `codex/accounting-plan5` on Community-Duris/Duris.
Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Base: `766784c534539ef7838fab883cecced3f70edbb0`, the published restore-evidence
slice. Refreshed remote `experimental-accounting` remains
`7d2f8e8153f637c38e19054cf1202436d3f9a28a`. The result is the commit containing
this report; the final handoff and ignored `tmp/plan5/operator-lookup-evidence.json`
record its exact result SHA. No push to experimental-accounting or activation
is part of this delivery.

## Defect and implemented behavior

The actual reconciler CLI rejects `--view operation` and has no account-key
filter. The regression suite reproduced that refusal before implementation.
The interrupted `accounting-audit-release` worktree has an uncommitted operator
draft; it was read and preserved. Suitable projection/filter parts were reused
here with focused tests, explicit coverage and global output bounds. Additional
price/supply filters from that draft were omitted because exact operation
records already provide the requested root detail without changing aggregates.

`--view operation --operation-id <ID>` now projects matching root metadata,
account effects, postings, children, item references, receipts, captured source
claims and matching database-wide orphan markers. The root ID must be nonzero
lowercase 32-character hex. All records share one 0..100 output limit; counts
are computed before truncation. Root metadata comes first, remaining records
have deterministic collection/index order, and duplicates remain visible.
Only documented IDs, numeric metadata, denomination vectors, outcome and
receipt-presence flags are projected. Alias fields, raw command/result payloads
and canonical blobs are omitted.

`--view holdings --account-key <KEY>` selects the exact existing version-1
40-byte key from captured native holdings. Malformed IDs, keys, limits and
inappropriate operation/account filter combinations refuse with status 2.
The unfiltered views and retained UID provenance behavior remain available.

Coverage carries lineage, selected epoch, completeness/quiescence and the
whole-audit exception count. Root capture is selected-epoch scope. Source
claims and orphan markers can come from wider captured scopes. A zero result
or zero root count never proves global absence. Filtering preserves the audit
exit status; an unrelated discrepancy or partial export still returns status 1.
Neither view opens the database, changes the snapshot, or applies a correction.

## Ownership and exact source

Owned files:

- `scripts/reconcile_economy_accounting.py`
- `tests/async/test_reconcile_economy_accounting.py`
- `tests/async/run_economic_sql_audit_snapshot_mysql.py`
- `docs/persistence/economy_accounting/AUDIT_OPERATIONS.md`
- This report.

No shared interface, schema, producer, activation, registry/matrix or central
registration change is requested. This extends existing test runners. Native
sources, SQL exporter and migration inputs are unchanged from the base,
including canonical migration 0056. These snapshot tests use deliberately
minimal native SQL tables, not a fresh/upgraded full-schema qualification.

Tested SHA-256 pins:

| Input | SHA-256 |
| --- | --- |
| `scripts/reconcile_economy_accounting.py` | `17f885386371b31a6b81ab0110dafe5eead777c1a7ae5e6d822cb77059e6cccf` |
| `tests/async/test_reconcile_economy_accounting.py` | `21916d0b308df8cf9fd0b271758a922f8c948778fc3fd08ecfe5c4c61258bb3f` |
| `tests/async/run_economic_sql_audit_snapshot_mysql.py` | `1e356c91745166b4fe670208b2cb2bf72f224ba6f62184c6c9a7ad4ca1397df7` |

## Commands, results and evidence

Evidence is retained under ignored `tmp/plan5/`; no logs or credentials are
committed. The RED log is `operator-lookup-unit-red.log`: the previous API has
no lookup parameters and the actual CLI refuses the new view. The first GREEN
attempt also found a test mistake: the all-numeric wallet key was unchanged
by uppercase conversion. The final malformed-key fixture uses a key containing
a hexadecimal letter. Its correction is in the final tested source.

| Command | Result / evidence |
| --- | --- |
| `python tests/async/test_reconcile_economy_accounting.py` | PASS, 69 tests; `operator-lookup-unit-green3.log`. New tests cover exact matching, counts at limits 0/1/100, privacy projection, missing/duplicate/rootless records, malformed inputs, global CLI refusal and byte-identical input. |
| `python tests/async/test_economic_sql_audit_origins.py` | PASS, 11 tests; `operator-lookup-origins.log`. |
| `python tests/async/test_audit_accounting_invariants.py` | PASS, 16 tests; `operator-lookup-invariants.log`. |
| `python3 tests/async/run_economic_sql_audit_snapshot_mysql.py` — MySQL 8.0.46 | PASS; `operator-lookup-mysql-native2.log`. |
| Same native SQL runner — MariaDB 10.11.14 | PASS; `operator-lookup-mariadb-native2.log`. |
| `python scripts/validate_economy_accounting.py` | PASS, 14 fixtures / 868 routes / 2818 sites, `release_ready=False`; `operator-lookup-contract.log`. |
| `python -m py_compile scripts/reconcile_economy_accounting.py tests/async/test_reconcile_economy_accounting.py tests/async/run_economic_sql_audit_snapshot_mysql.py` | PASS. |
| `git diff --check` | PASS. |

Windows commands used Python 3.12.10. Native SQL checks used Python 3.12.3 and
mounted PyMySQL in the existing tools image
`sha256:0232af0d2069d00424bfe14ae109224395050f46282896682913049de0b73b25`.
DB images were MySQL
`sha256:7dcddc01f13bab2f15cde676d44d01f61fc9f99fe7785e86196dfc07d358ae2b`
and MariaDB
`sha256:dbe56e20372fc6d6b8e0e396866ba89c4c7f128c38c4f59aaa54d957db95790c`.
MySQL retains its fixture-only native-AIO-disabled configuration.

For each `$engine` of `mysql`, `mariadb`, after starting its task-owned daemon:

```powershell
$taskWorktree = 'C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max'
$taskPymysql = 'C:\Users\alexa\AppData\Roaming\Python\Python312\site-packages\pymysql'
docker run --rm --network "container:codex-plan5-restore-$engine-01a104cf" `
  --mount "type=bind,source=$taskWorktree,target=/workspace,readonly" `
  --mount "type=bind,source=$taskPymysql,target=/opt/python/pymysql,readonly" `
  --env PYTHONPATH=/opt/python --env ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA=1 `
  --env DB_HOST=127.0.0.1 --env DB_PORT=3306 --env DB_USER=root `
  --env DB_PASSWORD=plan5-disposable-only --entrypoint python3 `
  duris-accounting-restore-tools:local `
  /workspace/tests/async/run_economic_sql_audit_snapshot_mysql.py
```

The runner owns a fresh random schema and reader account, grants SELECT only,
captures through the actual exporter CLI and calls the actual reconciler CLI.
It asserts seven exact operation records, one exact native wallet holding,
whole-audit refusal for the partial cut, counts at 0/1/100, zero aliases,
byte-identical input and exact pre/post captured database rows. Four orphan
families remain queryable when the root is absent. Existing exporter isolation,
orphan, late lower-ID commit, interrupted cut, provenance, revision/value,
native treasury and source/reference checks continue to pass. Owned fixture
schemas/accounts are removed; task daemons are stopped after qualification.

Limits remain 32 MiB input, 100,000 rows per collection and 100 total output
records. Native fixture socket reads retain 20 seconds and CLI subprocesses
30 seconds. These are functional bounds, not a measured release load budget.

## Remaining gates

This completes the exact-ID operator-query slice, not Plan 5 or R1–R8 release.
Fair resumable historical capture, complete native/evidence reconciliation,
full SQL/flatfile lifecycle/restore/retention, real-player and fault journeys,
load/storage budgets and the combined candidate remain required. The primary
owner retains registry/matrix, executable route evidence, producer repairs and
activation. No maintained C/C++ build or gameplay journey was rerun for these
Python view changes. Prior source-specific passes do not qualify the combined
candidate. Inactive native behavior, wallet-root exclusions and the declined
inactive spell-path change are unaffected by this slice.

The missing `AI_CONTEXT.md` and required notebook curator workflow/reference
remain a genuine notebook dependency. The pending user clarification has no
answer; this source report does not substitute a curator update.

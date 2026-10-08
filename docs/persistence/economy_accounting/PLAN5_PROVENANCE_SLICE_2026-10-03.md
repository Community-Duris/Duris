# Plan 5: retained UID provenance operator query

Release and activation remain blocked. This slice fixes a read-only operator
query; it does not qualify native writers, a complete audit, or a release.

## Branch and ownership

- Branch: `codex/accounting-plan5`, published to the same-named remote branch.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Initial refreshed base: `9d0ea2a1dd49cdb3fa132e5d38035c931a080b14`.
- Slice base: `7d2f8e8153f637c38e19054cf1202436d3f9a28a`, after adopting the
  incoming canonical orphan-evidence audit fix. The independent earlier
  reproduction and superseded patch remain under ignored `tmp/plan5/`.
- Owned files: `scripts/reconcile_economy_accounting.py`, its existing
  `tests/async/test_reconcile_economy_accounting.py`, the existing disposable
  `tests/async/run_economic_sql_audit_snapshot_mysql.py`, `AUDIT_OPERATIONS.md`,
  and this handoff. No native source, migration, accounting contract, producer,
  coordinator, writer registry/matrix, or activation-owner edit.

The result commit is the commit containing this handoff and the four owned
implementation/test/documentation paths. Its exact SHA is supplied with the
slice delivery. It is not published directly to `experimental-accounting`.

## Established defect and result

The provenance view read only selected-epoch `ownership_events`, omitting
already-exported lineage and unattributed UID histories. The actual native-SQL
fixture returned zero provenance rows for UID 86 despite retained prior-epoch
creation and unattributed retirement events. Both MySQL and MariaDB reproduced
the assertion before the fix. The pure regression also reproduced lost
history, duplicate projections, missing coverage, and invalid UID acceptance.

The view now uses all three captured event collections, sorts by revision,
operation ID and event index, and deduplicates only identical safe projections.
Conflicting native positions remain visible. Output excludes personal aliases,
validates the requested uint64 UID, retains total counts with `--limit 0`, and
discloses lineage, selected epoch, completeness/quiescence attestations, and
whether each additional history collection is available. SQL history starts
after the retained opening revision; this does not reconstruct earlier history.

The query uses existing exporter fields. There is no shared interface or schema
request for the implementation. The diagnostic JSON adds `coverage` to the
provenance view only; the CLI and exporter input formats retain version 1.

## Exact proof

Tested source is the slice base plus these raw SHA-256-pinned files:

| File | SHA-256 |
| --- | --- |
| `scripts/reconcile_economy_accounting.py` | `af86b98beed5129e4ad8025dccdd7a599dcbd4fde47a2b1dca8dcb04ccbf9718` |
| `tests/async/test_reconcile_economy_accounting.py` | `a567bee32cf43fa45b104aad26ec20c3d2965208326dd48dc05c7660182cea61` |
| `tests/async/run_economic_sql_audit_snapshot_mysql.py` | `876712f942ca7d847b7fc5aa177d3c97196c56f3b192cf1fc0b7d82132d6acd7` |

| Command / target | Result | Evidence under `tmp/plan5/` |
| --- | --- | --- |
| `python tests/async/test_reconcile_economy_accounting.py` (Windows Python 3.12.10) | 65 PASS | `provenance-unit-green.log` |
| `python tests/async/test_economic_sql_audit_origins.py` | 11 PASS | `provenance-exporter-green.log` |
| `python tests/async/test_audit_accounting_invariants.py` | 16 PASS | `provenance-invariants-green.log` |
| `python3 tests/async/run_economic_sql_audit_snapshot_mysql.py` (MySQL 8.0.46, WSL Python 3.10.12) | PASS | `provenance-13651-green.log` |
| Same native fixture (MariaDB 10.6.23, supplemental WSL compatibility run) | PASS | `provenance-13652-green.log` |
| Same native fixture (supported MariaDB 10.11.14 container) | PASS | `provenance-mariadb1011-green.log` |
| `python scripts/validate_economy_accounting.py` | PASS, `release_ready=False` | `provenance-contract-green.log` |
| `python scripts/generate_economy_writer_coverage.py --check` | FAIL, stale matrix | `provenance-matrix-check.log`, `provenance-matrix-drift.json` |
| `python scripts/validate_economy_accounting.py --release` | BLOCKED, writer has no executable evidence | `provenance-release-gate.log` |
| `git diff --check` | PASS | Recorded delivery command |

Native runs set `ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA=1`,
`DB_HOST=127.0.0.1`, `DB_USER=root`, a task-local port (`13651`/`13652` in WSL;
`3306` inside the owned Docker network), and a disposable-only `DB_PASSWORD`.
The fixture creates a fresh UUID-named schema and a SELECT-only audit user,
then removes only its own namespace. Neither project `.env` nor production or
player data is used. MySQL uses `--innodb-use-native-aio=0` following retained
initial AIO exhaustion; this is functional evidence, not workload timing proof.

The supported MariaDB run uses image
`sha256:dbe56e20372fc6d6b8e0e396866ba89c4c7f128c38c4f59aaa54d957db95790c`
and Python tools image
`sha256:16fbfd5f6b30930ac9a6d1189bb1c7fefac0b37f6fad6ea3338dbf1d42c714de`.
Both the worktree and existing PyMySQL package are mounted read-only:

```powershell
docker run --rm --network container:codex-plan5-provenance-mariadb-01a104cf --mount 'type=bind,source=C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max,target=/workspace,readonly' --mount 'type=bind,source=C:\Users\alexa\AppData\Roaming\Python\Python312\site-packages\pymysql,target=/opt/python/pymysql,readonly' --env PYTHONPATH=/opt/python --env ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA=1 --env DB_HOST=127.0.0.1 --env DB_PORT=3306 --env DB_USER=root --env DB_PASSWORD=plan5-disposable-only --entrypoint python3 duris-accounting-test-tools:local /workspace/tests/async/run_economic_sql_audit_snapshot_mysql.py
```

The added native checks drive actual SELECT-only export and provenance CLI at
limits 0, 1 and 100; retain exit 1 for the deliberately incomplete/anomalous
snapshot; verify input bytes and exact pre/post authority reads; and retain
the existing orphan, receipt, money/UID revision, payload, source, consistent
cut and interrupted-capture checks. Unit checks separately cover exact
deduplication, conflicting positions, alias exclusion and invalid query UIDs.
These are synthetic/native database fixtures, not actual player journeys.

RED logs remain `provenance-unit-red.log`, `provenance-13651-red.log`, and
`provenance-13652-red.log`. Source/evidence hashes are recorded in
`provenance-evidence.json`; engine versions in `provenance-versions.json`.

## Shared handoff and remaining gates

The matrix check at this unchanged native source finds 325 line-site additions
and removals against the tracked mapping: 2,818 occurrences / 2,759 unique
sites, 2,434 mapped and 325 unmapped, versus the tracked zero-unmapped claim.
The draft source commit remains `5f542e9cf769dfa7ef89e28ce4845d7495d4e1b3`.
The normal contract validator passes. The primary owner should reconcile the
registry/matrix on its final integrated source; no policy or executable proof
may be inferred from line reanchoring. This slice leaves those files untouched.

Still open: native audit completeness, independent full money/root/source
reconciliation, exact-operation and account-filtered operator queries,
backup/restore continuity, protected retention and erasure policy, flatfile
parity, real gameplay/fault journeys and predeclared workload budgets. The
minimal disposable schemas here do not apply or qualify canonical migration
0056, fresh/upgraded runtime histories, maintained binaries or the combined
candidate. Earlier 0055 evidence is not promoted.

`AI_CONTEXT.md` and the required notebook curator workflow are absent from
this checkout and the tracked remote tree. Their location and the primary
handoff task identity were requested. Notebook maintenance remains pending
that information; this source handoff is not a substitute curator update.

No activation, merge, deployment, production-data change or auto-correction
occurred. Inactive gameplay, wallet-root item exclusions and the declined
inactive spell-path change are unchanged.

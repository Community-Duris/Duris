# Code Review and Repair Report

**Session ID**: `phase03-session12-boot-schema-and-lookup-compatibility`
**Reviewed**: 2026-08-27
**Base Commit**: `ea8bf1054d3020bf797d5cd5556681ddc5a7d581`
**Scope**: All changes since the base commit, including uncommitted and untracked files
**Result**: RESOLVED

## Review Surface

No mid-session commits exist. The 32-file pre-report surface was reviewed:

- Spec state and evidence: `.spec_system/state.json` and all files under
  `.spec_system/specs/phase03-session12-boot-schema-and-lookup-compatibility/`.
- Build and operator docs: `Makefile`, `README.md`, `docs/ACCOUNT_ERASURE.md`,
  `docs/DATABASE.md`, `docs/DATA_LIFECYCLE.md`, `docs/IMMUTABLE_MIGRATIONS.md`,
  `docs/PERSONAL_DATA_EXPORT.md`, `docs/README.md`, `docs/TESTING.md`, and
  `docs/RUNTIME_COMPATIBILITY.md`.
- Schema and policy: `migrations/data_lifecycle_manifest.json`,
  `migrations/migration_manifest.json`, `migrations/immutable/0001_lookup_dataset_state.sql`,
  `migrations/immutable/0001_lookup_dataset_state.sh`,
  `migrations/runtime_compatibility_manifest.json`, and
  `migrations/verify_runtime_compatibility.sh`.
- Runtime and validators: `src/sql.c`, `src/sql.h`,
  `src/runtime_compatibility_contract.h`, `scripts/lifecycle_archive.py`,
  `scripts/validate_data_lifecycle.py`, and `scripts/validate_runtime_compatibility.py`.
- Tests: the changed lifecycle/export/erasure/migration/connection tests plus
  `tests/async/test_runtime_boot_compatibility.py`,
  `tests/async/run_lookup_dataset_mysql.sh`, and
  `tests/async/run_runtime_compatibility_mysql.sh`.

Inventory commands: `git status`, `git log --oneline
ea8bf1054d3020bf797d5cd5556681ddc5a7d581..HEAD`, `git diff
ea8bf1054d3020bf797d5cd5556681ddc5a7d581`, `git diff --cached
ea8bf1054d3020bf797d5cd5556681ddc5a7d581`, and
`git ls-files --others --exclude-standard`.

## Findings by Severity

### Critical

No findings.

### High

- `scripts/lifecycle_archive.py:28` - Canonical archive/export/erasure policy loading
  omitted immutable schema files, so table 171 caused fail-closed coverage errors.
  Fix: added migration `0001` to every canonical schema inventory and updated exact
  188-store tests. Status: FIXED.
- `migrations/verify_runtime_compatibility.sh:38` and
  `migrations/immutable/0001_lookup_dataset_state.sh:8` - The standalone verifier did
  not prove the history checksum, and the step verifier accepted any six columns.
  Fix: verify exact history state plus engine, collation, types, sizes, unsigned flags,
  timestamp behavior, primary key, and check constraint. Regenerated immutable and
  history checksums. Status: FIXED.

### Medium

- `src/sql.c:784` - A matching state row could previously mask manually drifted race
  or class rows. Fix: recompute the live canonical row checksum before unchanged no-op
  and again before state advancement. Status: FIXED.
- `tests/async/run_runtime_compatibility_mysql.sh:7` - Coverage initially proved only
  MySQL 8 and used a readiness probe that could race MariaDB's temporary init server.
  Fix: added stable two-probe readiness and exact MySQL 8.0/MariaDB 10.11 fingerprints,
  migration verification, and drift rejection on both engines. Status: FIXED.
- `src/sql.c:1522` - Drifted metadata could grow the canonical fingerprint buffer
  without an application bound. Fix: fail closed at the compiled 4 MiB metadata
  ceiling and add a source assertion. Status: FIXED.

### Low

- `src/sql.c:936` - Lookup failure logging claimed rollback even when commit outcome
  could be ambiguous. Fix: report failure or ambiguous outcome and abort; next boot
  revalidates state and rows. Status: FIXED.
- `migrations/data_lifecycle_manifest.json` - Lookup state used a pending personal-data
  classification inconsistent with the adjacent non-subject race/class stores. Fix:
  aligned its technical classification, service lifetime, and exclusion rationale.
  Status: FIXED.
- Documentation and source cleanup - removed an unused include, centralized compiled
  connection/dataset constants, and repaired ASCII/readability drift. Status: FIXED.

## Assumptions and Deliberate Non-Fixes

- The sealed Session 11 bootstrap remains exactly 170 tables. This is deliberate:
  fresh and upgraded databases adopt that baseline and then apply immutable migration
  `0001`, yielding the same 171-table runtime state.
- SQL strings built for lookup publication contain only compiled race/class constants,
  numeric IDs, SHA-256 hex, and escaped compiled labels. No player or network input
  reaches this path; replacing it with prepared statements would broaden legacy code
  without changing the reviewed trust boundary.

## Behavior Changes

- Boot now aborts before its first database mutation unless the connection, sealed
  baseline, immutable history, complete schema metadata, engine, and collation match.
- Race/class rows now no-op when unchanged and otherwise publish atomically with state
  advancement last. Failed or ambiguous publication aborts boot.
- Lifecycle inspection remains non-destructive and blocked by policy, now with exact
  coverage of 188 stores.

## Evidence Ledger

| Check | Command or Inspection | Result | Evidence / Blocker |
|-------|-----------------------|--------|--------------------|
| Full tests | `make test-all` | PASS | 209/209 Python regressions; server, areas, formatting, and signal-handler gates passed. |
| MySQL integration | `tests/async/run_lookup_dataset_mysql.sh && tests/async/run_runtime_compatibility_mysql.sh` | PASS | Atomic rollback/publication and full-schema drift rejection passed on MySQL 8.0. |
| MariaDB integration | `RUNTIME_DB_IMAGE=mariadb:10.11 tests/async/run_runtime_compatibility_mysql.sh` | PASS | Exact migration verifier, full schema, and six drift classes passed on MariaDB 10.11. |
| Build | `make -C src -j2` | PASS | C++20 warning-clean server build. |
| Formatter | `./scripts/format.sh --check` | PASS | Changed C/C++ lines match `.clang-format`. |
| Syntax | `bash -n ...` and `python3 -m py_compile ...` | PASS | New shell and Python files parse successfully. |
| Patch hygiene | `git diff --check` and added-line ASCII scan | PASS | No whitespace errors or non-ASCII additions remain. |
| Security/BQC | Targeted inspection of `src/sql.c`, manifests, shell boundaries, and logs | PASS | No hardcoded real secrets, user-controlled SQL, private-value logging, production mutation, partial publication, or unbounded runtime metadata buffer. |
| Final diff re-read | `git diff ea8bf1054d3020bf797d5cd5556681ddc5a7d581` plus every untracked file | PASS | All task areas are present; findings above are repaired; no debug/generated artifacts remain. |

## Summary

All 32 pre-report files since the base commit were reviewed. Findings were 0 critical,
2 high, 3 medium, and 3 low; all are resolved with focused regression or integration
evidence. No external blocker remains.

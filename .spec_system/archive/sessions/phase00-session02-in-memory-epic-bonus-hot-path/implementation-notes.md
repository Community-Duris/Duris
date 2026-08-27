# Implementation Notes

**Session ID**: `phase00-session02-in-memory-epic-bonus-hot-path`  
**Started**: 2026-08-27 00:10  
**Last Updated**: 2026-08-27 00:26

---

## Session Progress

| Metric | Value |
|--------|-------|
| Tasks Completed | 18 / 18 |
| Estimated Remaining | Complete |
| Blockers | 0 |

---

## Task Log

| Task | Completed | Evidence |
|------|-----------|----------|
| T001 | 00:11 | Analyzer selected Session 02 after completed Session 01; inventoried five active caller files and canonical loader/award paths. |
| T002 | 00:11 | Confirmed local mode without values, shipped 5-day window, 10000 cap, unique selection PID, and gain PID index; no migration run. |
| T003 | 00:12 | Added explicit states, 32 buckets, a 31-day window bound, a synchronized type bound, and saturating totals. |
| T004 | 00:13 | Implemented parse-then-publish, reset, ordered/coalesced additions, local expiry, cap math, and validation. |
| T005 | 00:13 | Embedded the trivial state in zero-initialized `pc_only_data` and linked the new translation unit. |
| T006 | 00:15 | Added one grouped hydration query with both cutoffs, bottle exclusion, deterministic ordering, and categorical failures. |
| T007 | 00:15 | Added the named component after principal status/PID load; failure remains nonfatal and explicitly unavailable. |
| T008 | 00:15 | Replaced both legacy lazy queries with player-owned state reads and local expiry. |
| T009 | 00:16 | Added one idempotent upsert; cache state and success copy publish only after query success. |
| T010 | 00:16 | Added final positive non-bottle awards after the existing publication call and preserved strict same-second exclusion. |
| T011 | 00:16 | Audited regeneration, XP, shop, cargo, status/help, and award callers; formulas remain unchanged. |
| T012 | 00:17 | Documented authority, hydration, bounds, expiry, unavailable behavior, and the unchanged durability boundary. |
| T013 | 00:17 | Added standalone coverage for states, expiries, coalescing, reset, bounds, cap, saturation, invalid input, and clock rollback. |
| T014 | 00:18 | Added caller inventory and 17 zero-I/O, hydration, configuration-drift, load-order, selection-order, award-order, and cutoff contracts. |
| T015 | 00:18 | New tests plus epic-save, observability, and SQL persistence regressions passed. |
| T016 | 00:19 | Formatting and the warning-as-error C++20 server build passed. |
| T017 | 00:19 | Local login rendered epic help five times; query calls stayed 1562 before and after. The test server stopped normally. |
| T018 | 00:20 | Full suite, diff whitespace, and review-surface ASCII/LF evidence completed. |

---

## Key Decisions

- Daily buckets preserve the legacy `CURDATE()` window and bound login rows independently of gain history.
- The 32 buckets support integer windows through 31 days because the current day plus 31 prior dates can be eligible. Unsupported configuration disables the cache explicitly.
- Missing selection data is ready-none; dependency, configuration, parse, or capacity failure is unavailable-zero. Neither performs a lazy read.
- The cache follows the current accepted gameplay award boundary. Atomic ledger/balance durability remains Phase 02 work.
- No index was added without representative plan evidence; Phase 03 owns the measured index gate.

## Verification Evidence

- Read-only execution of the exact grouped query shape on the local database: PASS; no syntax or SQL-mode error.
- `python3 tests/async/test_epic_bonus_state.py`: PASS.
- `python3 tests/async/test_epic_bonus_hot_path.py`: PASS, 17/17 contracts.
- `python3 tests/async/test_epic_save_guards.py`: PASS.
- `python3 tests/async/test_persistence_observability.py`: PASS.
- `python3 tests/async/test_sql_persistence_paths.py`: PASS.
- `./scripts/format.sh --check`: PASS.
- `make -C src`: PASS with the warning-as-error profile.
- Local Telnet runtime: PASS; five reads added zero queries (1562 before and after).
- Local test process on port 4000 stopped normally; the separate checkout retained its own websocket listener.
- `make test-all`: PASS; 168/168 Python regressions plus signal-handler checks after all review fixes.
- `git diff --check` and review-surface ASCII/LF scan: PASS.

## Review Repairs

- Expiry now advances on every ready read, including mismatched bonus-type calls, so stale buckets cannot survive a caller mismatch.
- Cap and maximum properties refresh from the in-memory property table; rolling-window drift marks the cache explicitly unavailable until canonical rehydration.
- Hydration groups by the exact legacy expiry boundary, including the midnight edge where strict `gain.time > CURDATE() - INTERVAL N DAY` expires one day earlier.
- The touched diagnostic string in `src/structs.h` was normalized to repository-compatible ASCII.

## Scope Notes

- No schema, migration, Redis, worker, epic-task, artifact, guild, or ledger change was made.
- Configured credentials were used without printing or modifying them.
- All application state mutation remains on the game thread; the helper module receives values only.

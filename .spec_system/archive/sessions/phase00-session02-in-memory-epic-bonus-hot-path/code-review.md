# Code Review: In-Memory Epic Bonus Hot Path

**Reviewed**: 2026-08-27  
**Base commit**: `fd720f3d1dc67c8c18d0e858e661a7151cebca44`  
**Result**: RESOLVED

## Scope

The review covered the complete session diff from the recorded base commit. Before this report was added, the surface contained nine modified tracked files and seven untracked files.

Reviewed groups:

- Session state, specification, checklist, and implementation notes.
- `src/epic_bonus.{c,h}`, `src/epic_bonus_state.{c,h}`, `src/sql_player.{c,h}`, `src/epic.c`, `src/structs.h`, and `src/Makefile`.
- `tests/async/test_epic_bonus_state.py` and `tests/async/test_epic_bonus_hot_path.py`.
- `docs/DATABASE.md`.

## Findings

### Critical

None.

### High

None.

### Medium - resolved

1. A ready-state read with a mismatched bonus type returned before expiry maintenance, allowing stale buckets and `next_expiry` metadata to remain. Every valid ready-state read now expires due buckets before applying the type filter, and the runtime harness covers this path.
2. Live property changes could leave cached cap, maximum modifier, or rolling-window configuration silently stale. Cap and maximum values now refresh from the process-local property table on each read. A rolling-window change marks the cache explicitly unavailable until the next canonical login hydration because discarded history cannot be reconstructed without I/O.
3. Date-only hydration buckets did not exactly match the legacy strict cutoff for gains timestamped at midnight. The grouped query now calculates the exact expiry boundary with a `CASE` expression and groups by that boundary; both midnight and non-midnight cases are covered.

### Low - resolved

1. The specification described a 32-day bound although the supported rolling window is 31 days represented by at most 32 active expiry buckets. The wording now matches the implementation.
2. A touched `src/structs.h` diagnostic contained a legacy non-ASCII comparison symbol. It was normalized to ASCII without changing behavior.

## Deliberate Decisions

- No migration or index was added. Phase 03 owns the representative `EXPLAIN ANALYZE` gate and any evidence-backed index work.
- Rolling-window drift fails explicitly unavailable instead of performing a lazy read or presenting a potentially incorrect value. Re-login is the canonical recovery path.
- Session-timezone enforcement remains assigned to Phase 00 Session 08. The exact query was accepted by the configured local MariaDB instance, and current local database/server calendar behavior was verified.
- Runtime verification launched this checkout's binary directly on game port 4000 because `scripts/start_mud.sh` manages the separately installed checkout. That other service and its websocket listener were not altered.
- Saturating arithmetic prevents wraparound. The fixed 32-bucket representation keeps all hot-path state bounded and allocation-free.

## Behavioral Review

- `get_epic_bonus()` performs no database, Redis, filesystem, allocation, or locking work. It reads only player-owned state and process-local configuration.
- Login hydration performs one grouped, bounded query after the player's durable principal identity is loaded. Missing selection is ready-none; malformed configuration, query failure, malformed rows, or capacity failure is explicit unavailable-zero.
- Selection publishes cache state only after the idempotent upsert succeeds. Failed persistence cannot create a false in-memory selection.
- Gain publication records only the final positive, non-bottle amount and preserves the legacy strict selection-time cutoff, including same-second awards.
- Existing regeneration, XP, shop, cargo, status/help, and award callers retain their formulas and user-visible behavior.
- All cache mutation remains on the game thread; the pure helper module owns no external resource or asynchronous callback.

## Verification Evidence

- `bash .spec_system/scripts/analyze-project.sh --json` - PASS; the current session resolves to Session 02 and the four phase packages remain present.
- Exact grouped hydration query executed read-only against the configured local database - PASS; syntax and SQL mode accepted.
- `python3 tests/async/test_epic_bonus_state.py` - PASS.
- `python3 tests/async/test_epic_bonus_hot_path.py` - PASS; 17/17 source and integration contracts.
- `./scripts/format.sh --check` - PASS.
- `make -C src` - PASS with the repository warning profile.
- `make test-all` - PASS; 168/168 Python regressions plus signal-handler checks.
- `git diff --check` - PASS.
- Review-surface ASCII/LF/final-newline scan - PASS after normalizing `src/structs.h`.
- Local game login with the configured test account - PASS; five `epic bonus` reads added zero observed query calls (1562 before and after).

The repository has no separate configured linter or static type checker beyond its compiler/build, formatting check, source-contract tests, and Python regression suite.

## Conclusion

All review findings are resolved. The implementation matches the session specification and is ready for validation.

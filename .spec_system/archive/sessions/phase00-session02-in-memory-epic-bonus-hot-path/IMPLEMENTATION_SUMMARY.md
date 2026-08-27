# Implementation Summary

**Session ID**: `phase00-session02-in-memory-epic-bonus-hot-path`  
**Completed**: 2026-08-27  
**Duration**: 0.3 hours

---

## Overview

Replaced the synchronous per-read epic-bonus queries with bounded player-owned state. Login now hydrates selection and qualifying gains with one grouped query, while selection, awards, exact calendar expiry, configuration drift, and failure states update locally without adding external work to regeneration, XP, shop, cargo, help, or award reads.

---

## Deliverables

### Files Created

| File | Purpose | Lines |
|------|---------|-------|
| `src/epic_bonus_state.h` | Fixed-capacity state and pure transition contracts | 53 |
| `src/epic_bonus_state.c` | Hydration publication, expiry, addition, saturation, and modifier math | 155 |
| `tests/async/test_epic_bonus_state.py` | Standalone C++20 state-transition runtime harness | 88 |
| `tests/async/test_epic_bonus_hot_path.py` | Caller, I/O-isolation, hydration, and mutation source contracts | 76 |

### Files Modified

| File | Changes |
|------|---------|
| `src/Makefile` | Compiles the pure state translation unit. |
| `src/structs.h` | Embeds zero-safe state in player-only data. |
| `src/epic_bonus.{c,h}` | Adds grouped hydration and converts reads, selections, and gains to explicit in-memory state. |
| `src/sql_player.{c,h}` | Hydrates the named component after durable principal identity load. |
| `src/epic.c` | Records final qualifying non-bottle awards at the existing publication boundary. |
| `docs/DATABASE.md` | Documents authority, bounds, expiry, degraded behavior, and durability limits. |

---

## Technical Decisions

1. **Exact fixed expiry buckets**: At most 32 boundaries represent supported rolling windows through 31 days while preserving the legacy strict midnight cutoff.
2. **Explicit unavailable state**: Query, parse, capacity, and configuration failures return zero without hiding behind lazy external I/O.
3. **Parse then publish**: Hydration replaces live state only after the complete result validates, preventing partial authority.
4. **Conservative live configuration handling**: Cap and maximum refresh locally; rolling-window drift requires canonical re-login because discarded history cannot be reconstructed in memory.
5. **Unchanged durability boundary**: Active memory follows the currently accepted award publication boundary; Phase 02 still owns atomic ledger and balance durability.

---

## Test Results

| Metric | Value |
|--------|-------|
| Python regression tests | 168 |
| Passed | 168 |
| Failed | 0 |
| Hot-path contracts | 17/17 passed |
| Runtime query delta | 0 across five reads |
| Signal-handler checks | PASS |
| Coverage | Not configured |

---

## Lessons Learned

1. Grouping by calendar date is insufficient for a strict day-window predicate at midnight; the cache must group by the exact expiry boundary.
2. Configuration that changes the historical inclusion window cannot be refreshed truthfully from a lossy active cache, so explicit unavailability is safer than a stale result.

---

## Future Considerations

1. Session 08 should establish uniform database and server timezone invariants across every connection.
2. Phase 02 should replace the existing epic award publication boundary with an atomic operation-keyed ledger and balance transaction.
3. Phase 03 should use representative data and `EXPLAIN ANALYZE` before adding or changing the hydration index.

---

## Session Statistics

- **Tasks**: 18 completed
- **Files Created**: 4 implementation/test deliverables
- **Files Modified**: 8 specified deliverables
- **Tests Added**: 2
- **Review Findings**: 5 resolved
- **Blockers**: 0 unresolved

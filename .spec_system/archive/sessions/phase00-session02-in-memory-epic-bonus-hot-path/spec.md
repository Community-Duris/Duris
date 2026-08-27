# Session Specification

**Session ID**: `phase00-session02-in-memory-epic-bonus-hot-path`  
**Phase**: 00 - Correctness and Immediate Lag Removal  
**Status**: Not Started  
**Created**: 2026-08-27  
**Base Commit**: `fd720f3d1dc67c8c18d0e858e661a7151cebca44`  
**Work Window**: One coherent player-owned cache lifecycle spanning bounded login hydration, successful selection and award mutation, rolling expiry, every active bonus consumer, and end-to-end external-I/O isolation. The window ends when regeneration, XP, shop, cargo, help, and epic-award reads are pure memory operations with explicit unavailable behavior.

---

## 1. Session Overview

This session removes the largest confirmed simulation-thread database hotspot. The current `get_epic_bonus()` implementation performs one selection query on every call and a second rolling-sum query when the selected type matches. Hit regeneration alone can invoke this path roughly 800 times per second at the 200-player target, with movement, XP, shops, cargo, help, and epic awards adding more synchronous reads.

The replacement hydrates a bounded rolling state once during the existing player load, owns that state inside `pc_only_data`, and updates it synchronously only after accepted selection and epic-award operations. Existing consumers keep the same API but receive a computed in-memory modifier. Expiry removes daily contribution buckets locally and never falls back to a lazy MySQL or Redis query.

Session 01 is complete and supplies safe call-site evidence for proving the query removal. Phase 02 still owns atomic epic balance/ledger transactions; this session changes read locality and cache truth without claiming new durability guarantees.

---

## 2. Objectives

1. Represent selected type, selection time, hydration state, rolling qualifying contributions, computed modifier inputs, and next expiry in bounded player-owned memory.
2. Hydrate selection and qualifying positive non-bottle gains with one bounded, set-based login query and distinguish ready-empty from unavailable state.
3. Update or reset the cache after successful bonus selection and accepted epic awards while preserving the existing rolling-window, selected-type, positive-gain, bottle-exclusion, and cap semantics.
4. Make every active `get_epic_bonus()` call perform no database, Redis, filesystem, allocation, or blocking synchronization work.
5. Prove selection, hydration, gains, expiry, caps, invalid configuration, and all hot-call-site contracts with focused runtime and source regressions.

---

## 3. Prerequisites

### Required Sessions

- [x] `phase00-session01-redacted-persistence-observability` - supplies redacted query-site metrics and source contracts used to prove hot-path isolation.

### Required Tools Or Knowledge

- C++20 compilation of legacy `.c` files
- MySQL date/time and grouped aggregate semantics
- Legacy `pc_only_data` zero-initialization and player-load lifecycle
- Python standalone runtime and source-contract patterns under `tests/async/`

### Environment Requirements

- Use the local/development database selected by `.env` without printing credentials.
- Do not run migrations or write tests against production; this session requires no schema change.
- Verify C/C++ changes with `./scripts/format.sh --check` and `make -C src`.

---

## 4. Scope

### In Scope (MVP)

- An active player owns a trivially initialized fixed-capacity `EpicBonusState` with explicit uninitialized, ready, and unavailable states.
- Login hydration reads the one `epic_bonus` row and groups qualifying `epic_gain` rows into expiry buckets, returning at most one row per supported rolling-window day in deterministic order.
- Missing selection data hydrates as ready with `EPIC_BONUS_NONE`; query, parse, invalid-type, invalid-cap, invalid-window, or bucket-overflow failures disable the bonus explicitly for that loaded character.
- Successful `epic_bonus_set()` persistence resets selection time and contribution state in memory; failed persistence publishes neither cache success nor success copy.
- Accepted positive non-bottle epic awards add their final post-modifier amount to the current expiry bucket after the durable-write call is issued, matching the current award/publication boundary without introducing a new transaction claim.
- Every existing `get_epic_bonus()` consumer reads only the player-owned state and expires due buckets locally.
- Runtime and source-contract tests cover rolling expiry, cap math, state transitions, zero-I/O callers, and no lazy fallback.

### Outside This Work Window

- Atomic epic balance, immutable ledger, unique operation IDs, and ambiguous-commit reconciliation - Reason: Phase 02 owns the epic transaction boundary.
- Redis caching or cross-process sharing of active epic-bonus state - Reason: active player memory is authoritative for this hot read.
- Reworking epic tasks, artifacts, guild prestige, or reward publication - Reason: those are independent gameplay/persistence domains.
- Adding an index or changing the `epic_bonus`/`epic_gain` schema - Reason: the bounded grouped login read uses the existing `pid_index`; representative index selection is reserved for the Phase 03 query-plan gate.
- Making all current player component loads atomic - Reason: Phase 03 owns the consistent player-load transaction.

---

## 5. Technical Approach

### Architecture

Add a small dependency-free `epic_bonus_state` module containing a fixed number of daily expiry buckets and pure state-transition functions. Embed that trivial state directly in `pc_only_data`, whose allocation paths already zero memory. The module accepts explicit timestamps, cap, and maximum modifier so runtime tests can exercise boundary behavior without a database or full game process.

`epic_bonus.c` remains the integration owner. A new hydration function executes one `LEFT JOIN`/grouped query for a PID, validates every row and configured bound, loads the pure state only after the full result is known good, and otherwise marks the state unavailable. The existing `get_epic_bonus()` API delegates solely to the in-memory calculator. Selection uses the existing database write but updates state and player copy only when that write succeeds. `gain_epic()` records the final qualifying amount in the cache after its existing persistence call.

The rolling predicate remains equivalent to `time > DATE_SUB(CURDATE(), INTERVAL N DAY)` and `time > selection_time`. Contributions are grouped by their local calendar expiry boundary, which preserves current day-based expiry while bounding login rows by the configured window. A documented maximum window rejects unsafe configuration explicitly instead of truncating or issuing an unbounded read.

### Design Patterns

- Player-owned read model: the game thread alone mutates state and consumers need no lock.
- Parse-then-publish hydration: temporary rows are validated before replacing live state, so partial results never leak.
- Fixed-capacity daily buckets: memory and per-read expiry work are bounded independently of historical gain count.
- Explicit degraded state: unavailable hydration yields zero bonus and never causes lazy external I/O.
- Compatibility API: existing gameplay call sites retain `get_epic_bonus()` while its contract becomes pure memory.

---

## 6. Deliverables

### Files To Create

| File | Purpose | Est. Lines |
|------|---------|------------|
| `src/epic_bonus_state.h` | Trivial state, bounds, hydration bucket, and transition contracts | ~100 |
| `src/epic_bonus_state.c` | Pure reset, publish, add, expiry, cap, and modifier behavior | ~220 |
| `tests/async/test_epic_bonus_state.py` | Standalone C++20 runtime coverage for state transitions and math | ~240 |
| `tests/async/test_epic_bonus_hot_path.py` | Source contracts for load integration, mutation ordering, and zero external I/O | ~170 |

### Files To Modify

| File | Changes | Est. Lines |
|------|---------|------------|
| `src/Makefile` | Compile and link the pure epic-bonus state module | ~2 |
| `src/structs.h` | Embed player-owned epic-bonus state in `pc_only_data` | ~5 |
| `src/epic_bonus.h` | Publish hydration, invalidation, award-update, and in-memory read APIs | ~25 |
| `src/epic_bonus.c` | Replace lazy queries with bounded hydration and pure reads; make selection cache publication success-dependent | ~260 |
| `src/sql_player.h` | Declare the epic-bonus player component load contract if required by loader organization | ~5 |
| `src/sql_player.c` | Invoke required hydration after principal player status load and fail explicitly without partial cache state | ~30 |
| `src/epic.c` | Apply qualifying final epic gains to the player-owned rolling state at the existing award boundary | ~15 |
| `docs/DATABASE.md` | Document active-player authority, hydration query, expiry bounds, and durability boundary | ~35 |

---

## 7. Success Criteria

### Functional Requirements

- [ ] `get_epic_bonus()` contains no MySQL, Redis, filesystem, allocation, sleep, or blocking-lock operation and every active consumer reaches only this in-memory contract.
- [ ] Login distinguishes absent selection (ready-none) from dependency or data failure (unavailable-zero) with no lazy query fallback.
- [ ] Hydration preserves selected type, selection-time cutoff, positive non-bottle contribution filtering, rolling-day cutoff, deterministic expiry buckets, and configured modifier cap.
- [ ] A successful selection resets selection time and accumulated contribution state; a failed write leaves the prior cache intact and does not claim success.
- [ ] Accepted positive non-bottle awards update the selected bonus accumulator using the final awarded amount; bottles, non-positive values, NPCs, unavailable state, and mismatched types do not contribute.
- [ ] Expired buckets are removed locally, next expiry remains truthful, and all counter math saturates rather than overflowing.

### Testing Requirements

- [ ] Standalone runtime tests cover ready-empty, unavailable, hydration, selection reset, same-day coalescing, multiple expiries, exact-boundary expiry, caps, invalid bounds, saturation, and clock rollback.
- [ ] Source-contract tests inventory every `get_epic_bonus()` caller and reject database, Redis, filesystem, or lazy hydration work in the read function.
- [ ] Source contracts verify hydration occurs in the player login path and cache mutation follows successful selection and qualifying award paths.
- [ ] Existing epic, regeneration, XP, player-load, SQL observability, and full repository regressions pass.
- [ ] A local game login can display the epic bonus, exercise regeneration, and execute an ordinary bonus read without adding query counts for the former hot site.

### Non-Functional Requirements

- [ ] State storage and expiry work are fixed-capacity and allocation-free for active reads and mutations.
- [ ] No worker traverses or mutates `P_char`; all state mutation stays on the game thread.
- [ ] The login query returns at most one deterministic row per supported rolling-window day and no historical row per gain event.
- [ ] No new schema dependency, persistence guarantee, or cross-process cache claim is introduced.

### Quality Gates

- [ ] All created or modified text files are ASCII with Unix LF endings.
- [ ] Changed C/C++ lines pass `./scripts/format.sh --check`.
- [ ] `make -C src`, focused regressions, and `make test-all` pass.
- [ ] Code and documentation follow repository conventions and preserve redacted diagnostics.

---

## 8. Implementation Notes

### Working Assumptions

- The configured rolling window is day-based, not an exact number of elapsed seconds. Evidence: the current predicate uses `DATE_SUB(CURDATE(), INTERVAL N DAY)`. Grouping qualifying rows by their common calendar expiry preserves that behavior.
- Explicit unavailable-zero behavior is safer than failing the entire legacy login. Evidence: the session stub allows login failure or explicit bonus disable, while current player loads already treat several non-principal components as nonfatal. The state remains visibly distinguishable for diagnostics and tests.
- The current award boundary is retained for Phase 00. Evidence: `log_epic_gain()` has a `void` contract and Phase 02 explicitly owns atomic ledger and spendable-balance durability. This session updates memory after the existing publication call and does not describe it as a durable acknowledgement.
- A fixed maximum rolling window is acceptable when invalid configuration fails explicitly. The shipped value is 5 days; a 31-day window backed by 32 daily buckets provides headroom while preventing configuration from turning a login into an unbounded result.

### Conflict Resolutions

- The stub asks for mutation after a successful epic award, but current epic logging cannot report durable success. The best-supported interpretation is an accepted gameplay award after its existing persistence call, with atomic/durable success deferred exactly as the Phase 02 boundary states.
- The scalability review recommends an index candidate while the session stub contains no migration scope and Phase 03 owns representative query-plan/index gates. This session uses a grouped per-PID login query and records index work for Phase 03 instead of adding an unmeasured migration.

### Key Considerations

- Preserve current gameplay ordering so an epic-point bonus affects the award being calculated, then the final award contributes to future selected-bonus calculations.
- Validate database types, timestamps, bucket counts, cap values, and configured day ranges before publishing hydrated state.
- Avoid copying `P_char` into workers or adding locks around a game-thread-owned cache.
- Keep unavailable, ready-none, selected-zero, and selected-with-contributions distinguishable in code even though each may yield a zero modifier.

### Potential Challenges

- Local midnight and daylight-saving transitions can change expiry spacing: derive calendar expiry consistently and treat timestamps as ordered boundaries rather than assuming every day is 86400 seconds.
- Many same-day gains can overflow an aggregate: use saturating signed contribution totals and cap before floating-point conversion.
- Selection writes currently report success unconditionally: gate both cache reset and player-facing success on the query result, with categorical failure output.
- Loader variants allocate zeroed `pc_only_data`: centralize hydration in the canonical database player loader and keep zero state safely unavailable for non-player utility objects.

### Relevant Considerations

- [P00] **Per-pulse epic lookups exceed the event budget**: the session removes all external calls from the shared read API used by regeneration and XP.
- [P00] **The game thread owns mutable objects**: the cache is embedded in `pc_only_data` and never exposed to a worker.
- [P00] **Do not put external I/O in mutation or pulse callbacks**: active reads and local expiry are pure; the existing explicit selection and award persistence boundaries remain the only writes.
- [P00] **Use focused source-contract regressions**: caller inventory and function-body contracts prevent a future lazy-query regression.
- [P00] **Do not broaden fixes into legacy modernization**: epic task, ledger, guild, artifact, and general login redesign remain outside this window.

### Behavioral Quality Focus

Checklist active: Yes

Top behavioral risks for this session:

- Partial or failed hydration is accidentally published as ready and later presented as authoritative.
- A selection or award updates memory before the corresponding existing write boundary, causing visible success after an immediate failure.
- Expiry, cap, saturation, or clock-boundary math produces a negative, wrapped, stale, or above-cap modifier.

---

## 9. Testing Strategy

### Unit Tests

- Compile the pure state module in a standalone C++20 harness and exercise every hydration state, bucket mutation, expiry, saturation, cap, and invalid-input boundary with explicit timestamps.
- Verify selection reset discards earlier contributions and a failed/unavailable state never accumulates or produces a modifier.

### Integration Tests

- Inspect the final `get_epic_bonus()` body and all callers to prove it cannot reach `qry`, `db_query`, MySQL, Redis, filesystem, sleep, allocation, or hydration functions.
- Verify the canonical `sql_load_player()` path hydrates after PID/status establishment and treats unavailable state explicitly.
- Verify selection publishes only after query success and `gain_epic()` records only final positive non-bottle awards.

### Runtime Verification

- Start the local game on a non-production port, authenticate with the configured test character, render epic-bonus help, allow regeneration callbacks to run, and compare Session 01 query-site snapshots before and after ordinary reads.

### Edge Cases

- No `epic_bonus` row, selected none, invalid selected type, null timestamp, query failure, invalid configured window or cap, too many grouped rows, duplicate expiry groups, non-positive gain, bottle gain, exact expiry timestamp, clock rollback, accumulator saturation, and modifier cap.

---

## 10. Dependencies

### Other Sessions

- Depends on: `phase00-session01-redacted-persistence-observability`.
- Depended by: Phase 00 performance acceptance and Phase 02 epic ledger/balance transaction work.

---

## Next Steps

Run the `implement` workflow step to begin implementation.

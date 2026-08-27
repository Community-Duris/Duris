# Implementation Notes

**Session ID**: `phase00-session03-save-failure-retry-and-terminal-safety`
**Started**: 2026-08-27 00:29
**Last Updated**: 2026-08-27 00:51

---

## Session Progress

| Metric | Value |
|--------|-------|
| Tasks Completed | 22 / 22 |
| Estimated Remaining | Complete |
| Blockers | 0 |

## Implementation Summary

- Deferred character slots now retain retry delay and schedule exactly one event through a single helper. Failures advance a deterministic 4-to-240-pulse capped exponential delay, retain coalesced semantics, and re-arm live work.
- Direct and global flushes return truthful booleans. Success clears a slot; failure retains and schedules it. The terminal helper consumes an existing pending slot without a duplicate full save and queues only a non-destructive crash-save retry after direct terminal failure.
- `writeCharacter()` extracts serialized equipment and inventory only after a successful database result for an inventory-extracting rent type. Every false result restores equipment, carried state, and affects even when a flat fallback record exists.
- Quit, camp, inns, heaven/death, idle/link loss, ghost extraction, artifacts, lockers, copyover, shutdown, and reboot now refuse their destructive completion when persistence fails.
- Copyover performs ship, locker, connected-player, and remaining-player gates before publishing its complete file. Descriptor closure, client FD mutation, compression teardown, and process replacement occur only afterward. Failure returns to the live game loop without the old destructive restart fallback.
- Shutdown and reboot use non-destructive crash saves before workers stop or characters are extracted. A failure resets terminal flags and resumes the loop.
- Locker leave now vetoes departure before room release when the locker character is absent or snapshot preparation fails. Legacy terminal save failure retains the locker character and its inventory.
- Operator documentation describes bounded retry, fallback limits, terminal alerts, shutdown cancellation, and locker leave recovery.

## Verification Evidence

- `bash .spec_system/scripts/analyze-project.sh --json`: PASS; Session 03 is the current Phase 00 candidate at base `4f49a11f`.
- Focused deferred retry, terminal safety, flush, copyover, locker, persistence-status, pwipe, epic, ship, auction, and logging regressions: PASS.
- `./scripts/format.sh --check`: PASS.
- `make -C src`: PASS with the repository C++20 warning-as-error profile.
- Local development-port login: PASS; configured account authenticated without printing credentials, `save` succeeded, and `world persistence` rendered successfully. The test process exited cleanly and the separately installed service was not changed.
- `make test-all`: PASS; 170/170 Python regressions plus signal-handler checks.
- `git diff --check`: PASS.
- Byte scan of all 28 created or modified files: PASS for ASCII, Unix LF, and final newline.

## Review Repairs

- Removed a high-severity fresh-slot scheduling inversion: new slots no longer set `scheduled` before invoking the sole scheduler. A dedicated source contract now rejects that stranded-slot shape.
- Permitted named stat-dead PCs to remain eligible for safe retry and made direct terminal failures create a non-destructive retry slot.
- Moved trusted quit, camp, inn, and heaven success messages after their durability gates; failed camp/inn attempts restore changed home and tupor state where applicable.
- Delayed copyover client FD mutation and progress notices until after complete state publication.
- Replaced the incoherent legacy locker leave fallback with a leave veto that retains the live dynamic room, character, chests, and inventory.
- Updated the stale pwipe shutdown assertion to verify the new boolean pre-destruction gate.

## Scope Notes

- No schema, migration, dependency, production operation, credential, player-data export, or fallback replay guarantee was introduced.
- The fixed deferred table and all live object mutation remain game-thread-owned.
- Runtime failure was not induced by corrupting the development database. Deterministic policy execution and source contracts cover the forced-failure branches.

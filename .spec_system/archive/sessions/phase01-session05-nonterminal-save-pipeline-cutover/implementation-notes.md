# Implementation Notes

**Session ID**: `phase01-session05-nonterminal-save-pipeline-cutover`
**Started**: 2026-08-27
**Last Updated**: 2026-08-27

## Session Progress

| Metric | Value |
|--------|-------|
| Tasks Completed | 18 / 18 |
| Estimated Remaining | Complete |
| Blockers | 0 |

## Implementation Summary

- Added a 128-snapshot/32 MiB coordinator with immutable pending-append and durable-
  ready queues, asynchronous replay and journal append, bounded pulse dispatch, and
  redacted health.
- Kept game-thread work to revision marking, cumulative snapshot capture, bounded moves,
  retained worker submission, and exact completion application.
- Added MySQL per-thread initialization for repository workers and replay dispatch.
- Replaced Redis dirty membership/count/fork flush with local component revisions and
  online dirty-only autosave capture; scheduling no longer depends on Redis enablement.
- Routed ordinary `writeCharacter` and `do_save_silent` before SQL, host files, unequip,
  affect mutation, or flat fallback; ship persistence uses its existing queued route.
- Narrowed audited mutation marks to status, skills, equipment, inventory, pets, and
  timers while preserving cumulative supersets where a legacy operation spans groups.
- Fenced synchronous transactional compatibility saves with a same-transaction durable
  revision advance so older immutable work cannot overwrite critical state.

## Verification Evidence

- `python3 tests/async/test_player_save_pipeline.py`: PASS.
- Worker, journal, Redis, revision, status, deferred, and terminal focused tests: PASS.
- `make -C src`: PASS with the C++20 warning-as-error profile.
- Formatting, whitespace, security source scan, and JSON validation: PASS.
- `make test-all`: PASS; 182/182 Python regressions plus signal-handler checks.

## Scope Notes

- Terminal extraction, bounded drain, copyover, and shutdown promotion remain Session 06.
- Critical transactional callers remain an explicitly fenced compatibility boundary
  until Phase 02 supplies operation IDs and atomic domain transactions.
- No configured database, migration, credential, or runtime player record was accessed.

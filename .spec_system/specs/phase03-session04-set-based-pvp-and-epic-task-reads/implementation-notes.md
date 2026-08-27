# Implementation Notes

**Session ID**: `phase03-session04-set-based-pvp-and-epic-task-reads`
**Started**: 2026-08-27
**Last Updated**: 2026-08-27

## Session Progress

| Metric | Value |
|--------|-------|
| Tasks Completed | 11 / 11 |
| Estimated Remaining | 0 - ready for creview |
| Blockers | 0 |

## Implementation Summary

- Added bounded fixed per-player read state for the latest 20 durable PvP deaths, up
  to 128 provisional transactions, and 1,024 sorted completed epic zones.
- Extended the consistent player-load transaction with two set-based reads: one
  timestamp-ordered PvP join and one deduplicated legacy/ledger completion union.
  Normal login and copyover both require the resulting read mask before publication.
- Replaced heaven-time query loops with exact in-memory window calculation. Accepted
  PvP work is provisional until the transaction callback commits, and rejection
  removes only the matching token.
- Added a validated last-good task-zone catalog refreshed after zone bootstrap.
  Selection is allocation-free reservoir sampling and excludes the player's fixed
  completion state; committed zone awards update membership idempotently.
- Removed `ORDER BY RAND()` and callback-path SQL from epic task selection, raised the
  bounded player-load budget from 20 to exactly 22 queries, and added focused runtime,
  source, and connection-local database fixtures.

## Planning Reconciliation

The initial plan said copyover would preserve live gameplay-read state by opting out.
Inspection showed copyover reconstructs a fresh character and serializes no such state.
The specification and task contract were corrected so both normal login and copyover
hydrate the mandatory read state. Review also tightened the latest-death query to order
by event timestamp with event ID as the deterministic tie-breaker.

## Verification Evidence

- `python3 tests/async/test_set_based_gameplay_reads.py`: PASS.
- `python3 tests/async/test_player_load_items.py`: PASS.
- `python3 tests/async/test_player_load_pets.py`: PASS.
- `python3 tests/async/test_player_load_pipeline.py`: PASS.
- `bash tests/async/run_player_load_repository_mysql.sh`: PASS with connection-local
  shadows, 25-to-20 death truncation, legacy/ledger overlap, and 22-query accounting.
- Combat, epic, and copyover focused Python and guarded schema tests: PASS.
- `make -C src -j2`: PASS with the warning-as-error C++20 profile.
- `./scripts/format.sh --check` and `git diff --check`: PASS.
- `make test-all`: PASS - 201/201 tests plus the signal-handler harness.

No migration, production operation, persistent database fixture, credential, or Phase
04 artifact was created.

## Handoff

Implementation is complete and ready for `creview`. Review must cover every tracked
and untracked change relative to base commit
`6ba79b1c946bbb08f4f42e4b569a660440bc890b`.

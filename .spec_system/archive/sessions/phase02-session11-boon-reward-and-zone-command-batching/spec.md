# Session Specification

**Session ID**: `phase02-session11-boon-reward-and-zone-command-batching`
**Phase**: 02 - Transactional Gameplay Domains
**Status**: Complete
**Created**: 2026-08-27
**Base Commit**: `c07765fa`

## Objectives

1. Replace callback-time boon definition/progress queries with guarded active-state
   hydration and one bounded typed progress command per trigger.
2. Commit boon progress, completion, shop rewards, audit, inbox result, and outbox once,
   while publishing gameplay rewards only from exact completion.
3. Replace epic-stone zone SQL/Redis fan-out with one immutable bounded zone-touch
   command and post-commit publication.
4. Preserve account-bound reward claim safeguards and prove their existing exact grant,
   cooldown, recovery, and concurrent-claim boundaries alongside the new reward path.

## Design Boundary

Active boon definitions and per-player progress are hydrated into game-thread-owned
state. A trigger captures only stable scalar event facts; the worker locks and validates
the selected boon/progress rows, applies all compatible progress set-wise, records each
completion once, and returns a bounded reward list. Presentation and transient effects
run only after exact completion; persistent epic/currency/item effects delegate to their
authoritative Phase 02 domains.

Zone touch capture freezes the toucher, bounded participant set, payout, alignment, and
reset flags before group state changes. The worker owns zone last-touch, immutable touch
history, alignment ledger, inbox, and outbox. Existing account-bound reward commands are
an explicit interactive management boundary rather than a simulation callback; their
transactional grant/summon/recovery contracts remain covered and are not weakened.

## Success Criteria

- [x] Boon hot callbacks perform no synchronous database, Redis, or filesystem I/O.
- [x] Duplicate/replayed boon triggers cannot repeat progress or completion rewards.
- [x] Zone touch current state, history, alignment, and outbox commit atomically.
- [x] Group participants and reward data remain immutable after command capture.
- [x] Authoritative epic/currency/item adapters remain the only owners of those effects.
- [x] Focused MySQL, source, format, build, security, and full-suite gates pass.

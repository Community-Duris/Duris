# Session Specification

**Session ID**: `phase02-session09-pvp-and-combat-outcome-batching`
**Phase**: 02 - Transactional Gameplay Domains
**Status**: Complete
**Created**: 2026-08-27
**Base Commit**: `119b10bf`

## Objectives

1. Capture one pointer-free, bounded PvP outcome before participant/group state can change.
2. Commit the PvP event, participant audit rows, frag/epic/wallet effects, revisions,
   inbox result, and publication outbox as one idempotent transaction.
3. Publish authoritative live balances and player messages only from an exact completion.
4. Preserve current frag, blood-money, epic, heaven-time, and progress calculations.

## Design Boundary

The game thread calculates the immutable outcome from the current room and group graph.
The worker locks all affected player/account rows in normalized command-key order. Derived
epic and currency operation IDs are deterministic children of the combat operation, so
the existing domain ledgers remain authoritative without allowing partial rewards.

The legacy recent-death query used to calculate heaven time is explicitly retained for
Phase 03; no mutation, Redis invalidation, equipment serialization, or player-log read is
performed by the combat callback. PvP audit descriptions are bounded snapshots and the
unbounded legacy equipment/log fields are deliberately stored empty.

## Success Criteria

- [x] Solo and group outcomes commit a complete immutable participant/effect set once.
- [x] Replay cannot duplicate audit, frag, epic, or wallet effects.
- [x] Disconnect and group mutation cannot redirect a captured completion.
- [x] Snapshot/checkpoint writers cannot overwrite revisioned combat balances.
- [x] Focused MySQL, source, format, build, security, and full-suite gates pass.

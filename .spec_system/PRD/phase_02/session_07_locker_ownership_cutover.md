# Session 07: Locker Ownership Cutover

**Session ID**: `phase02-session07-locker-ownership-cutover`
**Status**: Complete
**Work Window**: One locker custody boundary spanning public and private chest entry,
item movement, immutable locker snapshots, terminal exit, ownership ACK, failure
retention, and restart reconciliation.

---

## Objective

Make locker deposits and withdrawals atomic ownership transfers while preserving the
existing immutable locker-worker boundary and preventing terminal exit from publishing
or destroying uncommitted custody.

---

## Scope

### In Scope (MVP)

- Inventory public locker, private chest, locker-character, access, resort, enter, save,
  leave, disconnect, copyover, and shutdown routes after Phase 01.
- Map locker and chest identities to the Session 05 owner taxonomy and carry stable item
  and subtree identities through immutable locker snapshots.
- Convert player-to-locker and locker-to-player movement into atomic ownership commands
  with both player and locker revisions, ledger, outbox, and exact main-thread ACK.
- Integrate ownership transfer completion with locker generation coalescing so a newer
  snapshot cannot erase an unacknowledged movement or clear the wrong generation.
- Require terminal locker exit to drain or durably spill accepted transfers and retain
  live locker/player state whenever neither DB ACK nor permitted journal handoff succeeds.
- Reconcile existing locker rows, private chest rows, live synthetic locker objects,
  and current-owner baselines without deleting ambiguous data.
- Preserve access control and Phase 00 private-chest authentication behavior while
  keeping passwords and item details out of command logs and outbox metadata.

### Out of Scope

- Auction custody or settlement.
- Redesign of locker permissions, rent, sorting, or user interface.
- Phase 03 login/load batching and retention policy.

---

## Prerequisites

- [x] Sessions 05 and 06 ownership contracts are validated.
- [x] Phase 01 terminal drain and immutable locker/player worker lifecycles are
      authoritative.
- [x] Phase 00 private-chest password migration and terminal failure tests pass.

---

## Deliverables

1. Locker/chest owner identity and transfer integration in `src/storage_lockers.c`,
   `src/locker_async.c`, `src/sql_player.c`, and related interfaces.
2. Generation-safe locker snapshot and ownership acknowledgement coordination.
3. Locker baseline/restart reconciliation and redacted operator diagnostics.
4. Focused public/private, nested-item, concurrent access, stale-generation, exit,
   disconnect, copyover, crash, and duplicate-replay regressions under `tests/async/`.

---

## Success Criteria

- [x] Every player/locker item crossing commits one current-owner transition, ledger
      event, both affected revisions, inbox result, and outbox set atomically.
- [x] Locker snapshot coalescing cannot omit or reverse an unacknowledged transfer.
- [x] A failed or stale transfer leaves the item in its prior player or locker custody
      and releases no terminal gate incorrectly.
- [x] Terminal locker exit, disconnect, copyover, and shutdown drain or durably spill
      accepted work without duplicate snapshots or item loss.
- [x] Restart reconciliation converges live locker objects, durable locker rows, and
      current-owner rows or reports a quarantined conflict.
- [x] Worker and diagnostic paths expose no live pointers, chest passwords, or private
      item payloads.
- [x] Focused regressions, formatting checks, and `make -C src` pass.

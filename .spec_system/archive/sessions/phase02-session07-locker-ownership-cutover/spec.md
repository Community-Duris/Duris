# Session Specification

**Session ID**: `phase02-session07-locker-ownership-cutover`
**Phase**: 02 - Transactional Gameplay Domains
**Status**: Complete
**Created**: 2026-08-27
**Base Commit**: `89216f75`

## Objectives

1. Resolve every live public/private locker location to the numeric
   `(locker_id, chest_id)` owner used by the authoritative baseline.
2. Route player deposits and withdrawals through the Session 06 ACK-gated adapter
   without treating synthetic sorting-chest objects or temporary rooms as authority.
3. Seal locker snapshots only after accepted ownership operations complete, and block
   terminal room teardown while a user still has a pending movement completion.
4. Make snapshot SQL preserve authoritative item UID/topology compatibility without
   writing or deleting current-owner, owner-revision, or ownership-ledger rows.
5. Reconcile restored locker rows against numeric authoritative identities and fail
   closed on missing or mismatched custody.

## Design Boundary

The live locker UI is synthetic: temporary rooms and sorting-chest objects are rebuilt
per visit and are not durable ownership identities. Public custody is locker owner type
with `owner_id=lockers.id` and `context_id=public_chest.id`; private custody uses the
same locker ID with its private chest ID. Durable containers inside either chest remain
ordinary item topology under that custody identity.

The game thread resolves cached locker/chest IDs, captures a bounded item subtree, and
submits the existing critical transfer. Publication moves the live object only after
ACK, after which the ordinary player dirty/save hook may seal a locker snapshot. Leave,
disconnect, copyover, and shutdown must not destroy the synthetic room or locker avatar
while a movement completion can still publish into it.

## Success Criteria

- [x] Public and private deposit/withdraw paths use numeric locker/chest identities.
- [x] Synthetic room/chest identifiers never enter current-owner authority.
- [x] Accepted movement completes before locker snapshot generation or terminal teardown.
- [x] Failed/stale movement preserves prior live and durable custody.
- [x] Locker restore hydrates exact current-owner rows and rejects conflicts.
- [x] Worker jobs contain immutable SQL/scalars and no live game pointers.
- [x] Focused tests, guarded reconciliation, format, build, security, and full tests pass.

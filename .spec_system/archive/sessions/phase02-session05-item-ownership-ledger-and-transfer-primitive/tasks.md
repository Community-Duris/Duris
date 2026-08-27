# Task Checklist

**Session ID**: `phase02-session05-item-ownership-ledger-and-transfer-primitive`
**Total Tasks**: 20
**Created**: 2026-08-27

## Inventory And Design

- [x] T001 Confirm Sessions 01-02, Phase 01 inventory revision, and local DB prerequisites.
- [x] T002 Inventory UID assignment, custody schemas, container links, and legacy events.
- [x] T003 Define owner taxonomy, revisions, complete-subtree bound, creation/destruction rules.

## Identity And Schema

- [x] T004 Add guarded allocator, current-owner, owner-revision, baseline, ledger, quarantine schema.
- [x] T005 Synchronize bootstrap and migration runner; add exact schema/boot verification.
- [x] T006 Reserve non-overlapping UID ranges before world load and retire Redis counter authority.
- [x] T007 Add guarded legacy baseline, conflict quarantine, and reconciliation tools.

## Command And Repository

- [x] T008 Add fixed typed owner/item/transfer/result contracts and canonical keys.
- [x] T009 Lock owner revisions then item rows canonically and validate expected state.
- [x] T010 Prove declared subtree completeness and atomically update every item and owner revision.
- [x] T011 Commit immutable per-item ledger, exact inbox result, and outbox in one transaction.
- [x] T012 Handle creation, destruction, duplicate, ambiguity, and restart reconciliation.

## Synthetic Boundary And Tests

- [x] T013 Add pointer-free synthetic transfer submission/publication adapter.
- [x] T014 Prove player-player, custody, complete subtree, creation, and destruction paths.
- [x] T015 Prove stale owner/item, missing identity, incomplete subtree, overflow, and duplicate replay.
- [x] T016 Prove ambiguous baseline quarantine and exact reconciliation.
- [x] T017 Prove generic snapshots cannot overwrite ownership authority or revisions.

## Completion

- [x] T018 Run focused, guarded MySQL, reconciliation, format, build, and security gates.
- [x] T019 Run full repository validation and complete review/security/session records.
- [x] T020 Update PRD/state/version, commit intentionally, and publish the session.

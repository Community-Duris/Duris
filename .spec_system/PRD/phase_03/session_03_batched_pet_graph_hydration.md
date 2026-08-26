# Session 03: Batched Pet Graph Hydration

**Session ID**: `phase03-session03-batched-pet-graph-hydration`
**Status**: Not Started
**Work Window**: One pet recovery boundary from set-based pet/item/metadata reads through
bounded graph validation, game-thread construction, exact recovery acknowledgement,
and atomic publication with the player load.

---

## Objective

Replace per-pet and per-pet-item queries plus the fixed 256-item array with one bounded,
linear, fail-closed pet graph load that cannot silently omit or prematurely consume
recovery state.

---

## Scope

### In Scope (MVP)

- Re-inventory the implemented pet checkpoint, crash recovery, owner/revision, item
  identity, load, deletion/consumption, setup, follow, equipment, and container routes.
- Read all pets for one player and all associated items, authoritative owners, affects,
  and descriptions with a bounded set of queries inside the Session 01 consistent
  transaction.
- Replace fixed local arrays with explicit pet, item, depth, row, and byte limits that
  return a classified error instead of silently stopping at 256 items.
- Use pet/item ID maps and adjacency lists for O(N) metadata attachment and container
  assembly while validating duplicates, cycles, parents, slots, prototypes, and owner
  revisions before creating live objects.
- Construct and equip pets only on the game thread, stage room/follower changes, and
  publish the complete player-plus-pets result atomically or release every staged object
  safely.
- Replace load-time delete-on-read behavior with an exact revision/operation
  acknowledgement or another post-publication idempotent consumption rule compatible
  with Phase 01 recovery.
- Cover crash types, ordinary login, reconnect, copyover, duplicate completion,
  disconnect, restart, and failure after row read but before publication.

### Out of Scope

- New pet gameplay, combat, equipment, or persistence semantics.
- General NPC, shopkeeper, corpse, locker, or saved-item load refactoring.
- Query-index selection, owned by Session 05.

---

## Prerequisites

- [ ] Sessions 01 and 02 consistent loading and linear item assembly are validated.
- [ ] Phase 01 pet/recovery revision and journal behavior is authoritative.
- [ ] Phase 02 item ownership supports pet and pet-container owner identities.

---

## Deliverables

1. Batched pet, pet-item, current-owner, affect, and description read repositories.
2. Bounded typed pet graph DTOs plus O(N) game-thread materialization and rollback.
3. Exact post-publication recovery consumption/acknowledgement integrated with restart,
   reconnect, copyover, and shutdown behavior.
4. Focused zero/many-pet, 256-plus-item, nested, malformed, owner-conflict, partial-read,
   disconnect, crash, duplicate, and query-count regressions under `tests/async/`.

---

## Success Criteria

- [ ] Pet, item, owner, affect, and description query count is bounded independently of
      pet and item count.
- [ ] No fixed silent truncation remains; every configured limit returns an explicit
      failure before partial publication or recovery-record consumption.
- [ ] Pet item graph and metadata assembly are O(N), and each accepted item has exactly
      one valid location and owner.
- [ ] Workers receive no live player, pet, object, room, or prototype pointers.
- [ ] A failed, cancelled, or stale load leaves durable recovery rows retryable and no
      staged pet or item visible in the world.
- [ ] An exact successful publication consumes or advances recovery state once and
      cannot duplicate pets on reconnect, copyover, or replay.
- [ ] Focused regressions, isolated schema tests, formatting checks, and `make -C src`
      pass.

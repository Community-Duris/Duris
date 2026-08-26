# Session 02: Batched Item Ownership and Linear Assembly

**Session ID**: `phase03-session02-batched-item-ownership-and-linear-assembly`
**Status**: Not Started
**Work Window**: The complete player inventory read boundary across item rows,
authoritative owners, affects, descriptions, container validation, O(N) assembly, and
all-or-nothing publication inside the Session 01 load workflow.

---

## Objective

Load and validate a player's complete durable inventory with a bounded number of
set-based queries and linear-time graph assembly, using Phase 02 current ownership as
authority.

---

## Scope

### In Scope (MVP)

- Reconcile the post-Phase-02 player item schema, current-owner taxonomy, item and
  inventory revisions, legacy event history, affects, extra descriptions, equipment,
  and container relationships.
- Fetch player item rows and authoritative current owners set-wise by player owner key,
  then fetch all item affects and descriptions in bounded batches without per-item
  ownership queries.
- Carry stable item, database-row, owner, parent, equipment-slot, and revision identity
  in typed load DTOs returned by the Session 01 transaction.
- Build one ID-to-row/object map and one parent/child adjacency structure so metadata
  attachment, container linking, equipment placement, and weight recalculation are O(N)
  rather than repeated linear scans.
- Validate missing or duplicate IDs, owner mismatch, cycles, excessive depth, missing
  parents, invalid equipment slots, duplicate slots, unknown prototypes, and configured
  item/byte limits before any inventory becomes live.
- Define explicit fail-closed or quarantined handling for every malformed row without
  using legacy latest-event timing to select an owner.
- Add query-count and operation-count assertions plus representative large/nested
  inventory load measurements.

### Out of Scope

- Pet inventory and equipment, owned by Session 03.
- Locker, corpse, auction, or floor browse/load optimization outside player login.
- Deletion or reinterpretation of preserved legacy item-event history.

---

## Prerequisites

- [ ] Session 01 consistent load transaction and staged publication are validated.
- [ ] Phase 02 authoritative current-owner and inventory-revision contracts pass their
      reconciliation gate.
- [ ] Schema and load tests use isolated databases or backed-up development clones.

---

## Deliverables

1. Batched player item/current-owner/affect/description repositories integrated into
   the Session 01 typed result.
2. Linear ID-map and adjacency-based item materialization on the game thread.
3. Explicit inventory row, graph, depth, item-count, and byte-bound validation with
   redacted classified failures.
4. Focused empty, large, nested, owner-mismatch, duplicate, cycle, missing-parent,
   equipment-conflict, over-limit, and query-count regressions under `tests/async/`.

---

## Success Criteria

- [ ] Player inventory ownership and metadata load with a bounded query count that does
      not grow per item.
- [ ] Current-owner rows, not legacy event ordering, determine whether an item belongs
      in the player's load result.
- [ ] Metadata attachment and graph assembly perform O(N) indexed lookups with measured
      operation counts and no nested full-array searches.
- [ ] Every accepted item appears exactly once in one valid owner/container/equipment
      position and carries the expected item and inventory revisions.
- [ ] Malformed, ambiguous, stale, or over-limit inventory fails or quarantines under a
      documented rule before any partial object graph is published.
- [ ] Item rows and private descriptions do not enter diagnostics or failure logs.
- [ ] Focused regressions, isolated schema tests, formatting checks, and `make -C src`
      pass.

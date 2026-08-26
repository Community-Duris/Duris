# Session 02: Immutable Player Snapshot Capture

**Session ID**: `phase01-session02-immutable-player-snapshot-capture`
**Status**: Not Started
**Work Window**: The complete main-thread capture boundary for revisioned player
components, including nested objects, bounds, ownership, and deterministic cleanup.

---

## Objective

Capture every selected player checkpoint component into a bounded immutable typed DTO
without unequipping items, removing or reapplying affects, or exposing live game
objects to later worker execution.

---

## Scope

### In Scope (MVP)

- Define typed snapshot values for all components selected by the Session 01 mask,
  including revision, rent/save intent, room identity, and required schema version.
- Capture status, skills, affects and replacement rows, inventory/equipment/containers,
  pets, shapechanges, trophies, recipes, and other post-Phase-00 checkpoint state
  without mutating gameplay-visible objects.
- Replace global or shared snapshot scratch state such as `save_equip` with snapshot-
  owned values and deterministic cleanup.
- Enforce explicit byte, row, nesting, string, and object-count limits and return a
  classified retryable or terminal capture result without partial jobs.
- Add equivalence and mutation-isolation tests comparing captured values with intended
  current persistence semantics for every component group.

### Out of Scope

- Database apply, queue scheduling, acknowledgements, or retries.
- Journal encoding or filesystem durability.
- Trigger cutover and removal of the legacy save path.
- Phase 03 login batching or load-path optimization beyond revision hydration.

---

## Prerequisites

- [ ] Session 01 revision and component-state contracts are validated.
- [ ] Phase 00 replacement-row and terminal-save fixes remain covered by regressions.

---

## Deliverables

1. Immutable player snapshot DTO, builders, destructors, and bounded value helpers in
   focused `src/` modules.
2. Component capture adapters for the current player, item, pet, trophy, recipe, and
   shapechange persistence domains.
3. Removal of capture-time equipment and affect mutation from the new path, with the
   old path retained only until cutover.
4. Focused capture equivalence, bounds, allocation-failure, and no-live-pointer tests
   under `tests/async/`.

---

## Success Criteria

- [ ] Snapshot payloads contain no `P_char`, `P_obj`, descriptor, room pointer, or
      other live mutable pointer.
- [ ] Capturing a snapshot does not unequip, extract, reorder, or mutate player state,
      affects, inventory, equipment, pets, or containers.
- [ ] Every payload carries PID, revision, component mask, schema version, and bounded
      lengths sufficient for journal and worker validation.
- [ ] Allocation or bound failure produces no partial enqueue and leaves all dirty
      components pending.
- [ ] Component equivalence regressions cover empty, maximum, nested, removed-row, and
      malformed legacy edge cases.
- [ ] Focused regressions, formatting checks, and `make -C src` pass.

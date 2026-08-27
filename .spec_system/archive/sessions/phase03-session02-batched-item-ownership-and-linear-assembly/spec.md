# Session Specification

**Session ID**: `phase03-session02-batched-item-ownership-and-linear-assembly`
**Phase**: 03 - Load Path, Schema, and Retention
**Status**: Complete
**Created**: 2026-08-27
**Base Commit**: `c78bdefc17922a9a2c1775d016172417ecdeb0c5`
**Work Window**: One coherent inventory-read boundary: authoritative ownership and
item metadata enter the Session 01 transaction, are validated and assembled in linear
time, and publish only with the complete character.

---

## 1. Session Overview

Session 01 established the bounded consistent player-load transaction but deliberately
left inventory on the legacy post-publication loader. This session closes that seam for
player-owned items. It adds set-based item, owner, affect, and description reads to the
same snapshot and replaces per-item owner lookup plus repeated array scans with indexed
validation and assembly.

The session does not alter ownership history or schema. Phase 02's
`item_current_owner` and `item_owner_revision` rows are authoritative; `player_items`
remains the serialized object payload. Any mismatch between those representations,
malformed graph, unsupported quantity, unknown prototype, or configured bound fails
the login before the fresh character becomes live.

---

## 2. Objectives

1. Read every player item, current-owner row, owner revision, item affect, and extra
   description with three bounded set-based queries inside the Session 01 snapshot.
2. Prove an exact bijection between active player ownership and serialized item rows,
   including stable UID, vnum, root, parent, owner, item revision, and inventory
   revision identity.
3. Validate the item graph and build prototypes, metadata, containers, equipment, and
   inventory in O(N) indexed operations with explicit operation-count evidence.
4. Remove normal SQL-login item work after character publication while preserving the
   crash/rent compatibility paths outside `rtype == 0`.

---

## 3. Prerequisites

### Required Sessions

- [x] `phase03-session01-consistent-player-load-transaction` - bounded consistent
  repository, exact completion identity, and fresh-character publication.
- [x] `phase02-session05-item-ownership-ledger-and-transfer-primitive` - authoritative
  current-owner and owner-revision tables.
- [x] `phase02-session12-raw-event-queue-retirement-and-domain-gate` - ownership schema
  and reconciliation gate.

### Required Tools Or Knowledge

- C++20 server build and `.clang-format` changed-line policy.
- Existing `player_snapshot` item DTO, object prototype APIs, ownership runtime, and
  local-development MySQL harness.

### Environment Requirements

- Database integration runs only against the configured local development database;
  no production migrations, writes, or load tests.

---

## 4. Scope

### In Scope (MVP)

- Player-owned active rows in `item_current_owner`, their one
  `item_owner_revision`, serialized `player_items`, `player_item_affects`, and
  `player_item_extra_descr` rows.
- Explicit item-count, row, byte, string, affect, description, depth, equipment-slot,
  and assembly-operation bounds.
- Validation of unique database row IDs and item UIDs, quantity 1, exact owner/player
  match, active state, matching vnum, matching owner and item revisions, one root,
  consistent parent relationships, no cycles, no missing parents, and no duplicate
  equipment slots.
- Game-thread construction from known prototypes, safe string/affect/description
  replacement, ownership-runtime hydration, container linking, equipment placement,
  weight recalculation, and cleanup of every unpublished object on failure.
- Normal SQL account and legacy-password login integration; existing non-SQL rent and
  crash recovery behavior remains unchanged.

### Outside This Work Window

- Pet inventory and equipment - owned by Session 03.
- Locker, corpse, auction, room, and floor item loading - not player-login inventory.
- New indexes or schema changes - Session 05 owns representative-clone plan and write
  cost decisions; current schema already supplies the required join keys.
- Legacy event deletion or reinterpretation - current-owner rows remain authoritative
  and history remains preserved.
- Writing quarantine rows during login - the read-only snapshot cannot mutate; this
  session fails closed with a classified metadata-only outcome and leaves repair to the
  existing reconciliation/operator boundary.

---

## 5. Technical Approach

### Architecture

Extend `player_load_result` with bounded ownership metadata aligned to
`snapshot.items`. One repository query returns serialized rows joined to active current
ownership and the player owner revision, while sentinel rows expose either side of an
ownership/payload mismatch. Two more set-based queries return affects and descriptions
by item row ID. The worker converts database row IDs and stable parent UIDs into typed
indices without creating game objects.

On the game thread, a dedicated item materializer validates the complete DTO first,
using hash maps for row ID/UID lookup, parent adjacency, cycle/depth traversal, and
metadata attachment. It then creates each prototype once, applies saved fields, links
containers, equips top-level rows, hydrates runtime ownership, and recalculates weights.
All allocations remain staged; any failure extracts every unpublished object and leaves
the destination character without inventory.

### Design Patterns

- **Authoritative join contract**: current-owner state selects custody; serialized item
  rows supply object payload only when the sets agree exactly.
- **Two-phase materialization**: validate all external data before allocating or
  mutating live object graphs.
- **Indexed graph assembly**: hash maps and adjacency lists replace per-item full-array
  searches and expose deterministic operation counts.
- **Fail-closed publication**: malformed inventory becomes one classified load failure,
  never a partial character or silent skipped row.

---

## 6. Deliverables

### Files To Create

| File | Purpose | Est. Lines |
|------|---------|------------|
| `src/player_load_items.h` | Narrow inventory validation/materialization API and metrics | ~40 |
| `src/player_load_items.c` | Linear graph validation, object construction, ownership hydration, and cleanup | ~500 |
| `tests/async/test_player_load_items.py` | Runtime graph, bounds, failure, complexity, and integration source regressions | ~350 |

### Files To Modify

| File | Changes | Est. Lines |
|------|---------|------------|
| `src/player_load_repository.h` | Add bounded item ownership DTOs, component mask, and query limits | ~70 |
| `src/player_load_repository.c` | Load item/owner rows, affects, and descriptions set-wise in the existing transaction | ~300 |
| `src/player_load_materialize.c` | Require and invoke all-or-nothing item materialization | ~40 |
| `src/item_ownership_runtime.c`, `src/item_ownership_runtime.h` | Atomically hydrate one validated inventory or leave runtime ownership unchanged | ~100 |
| `src/Makefile` | Build the item materializer | ~2 |
| `src/nanny.c` | Remove the normal post-publication SQL item reload while retaining rent/crash paths | ~10 |
| `tests/async/player_load_repository_mysql_harness.cpp` | Add exact ownership/payload fixtures, mismatch cases, and query bounds | ~220 |
| `tests/async/run_player_load_repository_mysql.sh` | Link new item DTO dependencies when required | ~10 |
| `tests/async/test_player_load_pipeline.py` | Update complete-component and query-count source contracts | ~30 |

---

## 7. Success Criteria

### Functional Requirements

- [x] Active player inventory, ownership, affects, and descriptions load in exactly
  three additional queries regardless of item count.
- [x] Current-owner rows, never legacy event ordering, determine accepted custody.
- [x] Every accepted item has one unique row/UID, matching active player owner, vnum,
  root/parent relation, item revision, and player inventory revision.
- [x] Metadata attachment, graph validation, container linking, and placement use O(N)
  indexed operations with an asserted linear operation ceiling.
- [x] Empty, nested, equipped, and large valid inventories publish exactly once; any
  mismatch, duplicate, cycle, missing parent, invalid slot, unknown prototype,
  over-depth, over-count, over-byte, or allocation failure publishes no inventory or
  character.
- [x] Normal SQL login performs no `sql_load_player_items` call after materialization;
  non-SQL rent/crash compatibility behavior remains intact.

### Testing Requirements

- [x] Focused runtime/source item-load regressions pass.
- [x] Local-development MySQL snapshot harness passes valid, empty, mismatch, and bound
  scenarios without touching production.
- [x] Full repository regression gate passes.

### Non-Functional Requirements

- [x] Query count is constant, item assembly operation count is linear, and total rows,
  objects, strings, bytes, and depth remain within explicit `player_snapshot` limits.
- [x] Worker values contain no live pointers; all `P_obj` construction and ownership
  runtime mutation occurs on the game thread.
- [x] Diagnostics and failures expose classifications and aggregate counts only, never
  item names, descriptions, UIDs, player identity, or raw SQL.

### Quality Gates

- [x] All files ASCII-encoded.
- [x] Unix LF line endings.
- [x] Changed C/C++ lines pass `.clang-format`.
- [x] `make -C src` passes with the warning-as-error profile.

---

## 8. Implementation Notes

### Working Assumptions

- `player_items.obj_uid` is the payload-to-custody key and every accepted row has a
  nonzero unique value. Phase 02 created and reconciled `item_current_owner`, and the
  current save path writes one serialized row per object with quantity 1.
- `item_current_owner.parent_item_uid` expresses the stable parent relation while
  `player_items.container_id` expresses the matching database-row relation. Requiring
  both representations to agree catches stale or cross-owner payloads without legacy
  event reads.
- No migration is needed: `player_items` has PID, container, and UID indexes;
  `item_current_owner` has the owner composite, root, parent, and UID keys; and
  `item_owner_revision` has the owner primary key. Session 05 will decide any tuning
  only from representative-clone evidence.

### Conflict Resolutions

- The phase stub allows malformed rows to fail or quarantine, while the Session 01
  repository is read-only and requires all-or-nothing publication. This session fails
  login closed and records only a redacted classification; it does not add a write to
  the read transaction or silently skip the row.
- Legacy login currently reloads items after publishing the character because Session
  01 excluded inventory. The Session 02 component mask becomes authoritative only for
  `rtype == 0`; rent/crash item sources remain unchanged until separately scoped.

### Key Considerations

- Validate every row and the complete graph before `read_object`, `obj_to_obj`,
  `obj_to_char`, or `equip_char` mutates global/game state.
- Cleanup must extract each staged root exactly once without producing ownership events
  or durability side effects.
- Duplicate dynamic metadata must follow one explicit rule; silent truncation at
  `MAX_OBJ_AFFECT` is not acceptable.

### Potential Challenges

- **Prototype defaults versus saved overrides**: apply only persisted nullable fields
  represented by the DTO and preserve prototype values otherwise.
- **Spellbook descriptions**: parse the saved JSON into bounded skill IDs before object
  publication and reject malformed or over-limit data.
- **Weight recalculation order**: traverse containers deepest-first so parent totals are
  correct without repeated whole-graph scans.

### Relevant Considerations

- [P00] **Player save and load are over-broad and inconsistent**: this session removes
  the remaining player-item N+1 owner reads and quadratic assembly from normal login.
- [P00] **The game thread owns mutable objects**: workers return typed rows only.
- [P00] **Do not accept partial or silently truncated loads**: every malformed or
  oversized inventory fails before publication.
- [P00] **Do not tune from tiny local plans**: no index is added in this session.

### Behavioral Quality Focus

Checklist active: Yes

Top behavioral risks for this session:
- Cross-account or stale-owner items entering a character through mismatched payload
  and custody rows.
- Leaked or partially linked global objects when allocation or metadata application
  fails mid-materialization.
- Graph cycles, duplicate equipment slots, invalid prototypes, or extreme nesting
  causing partial publication, recursion blowup, or superlinear login work.

---

## 9. Testing Strategy

### Unit Tests

- Compile a focused item materializer harness with synthetic DTOs and controlled
  prototypes for empty, flat, nested, equipped, large, and every malformed graph case.
- Assert a deterministic linear operation ceiling and exact object cleanup on all
  injected failure points.

### Integration Tests

- Extend the local-development MySQL harness with player item, owner revision, current
  owner, affect, and description fixtures; prove exact values and constant query count.
- Exercise missing owner, foreign owner, inactive custody, vnum mismatch, missing
  payload, duplicate UID/row, bad parent, and unsupported quantity without production
  access.

### Runtime Verification

- Run the normal account/legacy source contracts and verify `rtype == 0` consumes the
  materialized graph without a game-thread SQL reload.
- Run `make -C src`, changed-line formatting, the DB wrapper, and `make test-all`.

### Edge Cases

- Empty inventory; maximum valid inventory; one row beyond every bound; nested depth
  at and beyond the limit; self-cycle and multi-row cycle; duplicate/missing row IDs,
  UIDs, parents, roots, slots, affects, and descriptions; malformed spellbook JSON;
  unknown prototype; allocation failure; stale current owner; zero/max revisions.

---

## 10. Dependencies

### Other Sessions

- Depends on: Phase 02 ownership foundation and Phase 03 Session 01 load transaction.
- Depended by: Phase 03 Session 03 pet graph hydration and Session 14 integrated login
  storm gate.

---

## Next Steps

Run the `implement` workflow step to begin implementation.

# Session Specification

**Session ID**: `phase03-session03-batched-pet-graph-hydration`
**Phase**: 03 - Load Path, Schema, and Retention
**Status**: Validated
**Created**: 2026-08-27
**Base Commit**: `05fdf0163437c31936dcbd50ea86010e7d6629a9`
**Work Window**: One coherent pet recovery boundary: pet rows and all pet-item metadata
join the existing consistent snapshot, all player and pet graphs stage under one
ownership revision, and the complete character family publishes or rolls back together.

---

## 1. Session Overview

Sessions 01 and 02 established a bounded repeatable-read player load and linear player
inventory assembly. The remaining normal login seam is crash-recovery pets: the legacy
loader runs on the game thread, queries per pet and per item, silently truncates each
pet at 256 items, publishes pets incrementally, and deletes durable rows after a
partial-success load.

This session moves pet and pet-item recovery into the same worker snapshot. It reuses
the Session 02 item graph rules, validates the union of player and pet payload against
the player's authoritative item owner revision, stages NPCs and objects before room or
follower mutation, and retains recovery rows as the latest idempotent checkpoint rather
than destructively consuming them on read.

---

## 2. Objectives

1. Read pets, pet items plus authoritative ownership, affects, and descriptions with
   three additional set-based queries independent of pet and item count.
2. Validate explicit pet, item, metadata, graph, row, byte, slot, prototype, and
   operation bounds without fixed truncation or partial acceptance.
3. Construct all player and pet item graphs on the game thread and apply their combined
   ownership-runtime batch only after every graph succeeds.
4. Publish pets with the fresh player exactly once, keep durable recovery rows
   retryable, and exclude SQL pet recovery from copyover and non-crash compatibility
   paths.

---

## 3. Prerequisites

### Required Sessions

- [x] `phase03-session01-consistent-player-load-transaction` - bounded worker snapshot,
  exact completion identity, and fresh-character publication.
- [x] `phase03-session02-batched-item-ownership-and-linear-assembly` - authoritative
  item DTOs, indexed graph validation, staged object cleanup, and normal item cutover.
- [x] `phase01-session06-terminal-drain-and-shutdown-safety` - revisioned checkpoint
  persistence and retryable terminal semantics.
- [x] `phase02-session05-item-ownership-ledger-and-transfer-primitive` - authoritative
  player ownership and container topology.

### Required Tools Or Knowledge

- C++20 server build, existing `player_snapshot` pet DTO, NPC/follower lifecycle APIs,
  Session 02 item materializer, and guarded local MySQL harness.

### Environment Requirements

- Database integration uses only the configured local development database with
  connection-local temporary tables. No production migration, wipe, or recovery
  operation is permitted.

---

## 4. Scope

### In Scope (MVP)

- All `player_pets`, `player_pet_items`, pet item affects/descriptions, and matching
  active `item_current_owner` rows for one player snapshot.
- Unique pet IDs/orders, bounded pet stats, known mobile/object prototypes, valid item
  parent graphs and equipment slots, and exact combined player/pet ownership bijection.
- Shared indexed item graph staging for player and pet owners, one atomic runtime
  ownership batch, and cleanup of every unpublished NPC/object on any failure.
- Fresh-login follower setup and room placement after the player enters its validated
  room; pet equipment enchant activation follows the same post-room rule as player
  equipment.
- Removal of the legacy `sql_load_player_pets` normal crash-recovery call and its
  delete-on-read behavior from the login path; durable checkpoint rows remain until a
  later revisioned save replaces or clears them.
- Focused ordinary/crash/reconnect/copyover, zero/many pet, 256-plus item, malformed,
  stale, duplicate, cancellation, cleanup, query-count, and linear-operation tests.

### Outside This Work Window

- New pet gameplay, combat, equipment, charm, or save semantics.
- General NPC, locker, corpse, auction, or room-item loading.
- New ownership enum values or schema/index changes; Session 05 owns measured index
  decisions.
- Deleting historical checkpoint rows on read; destructive acknowledgement cannot be
  atomic with in-memory publication and would lose pets on an immediate process crash.

---

## 5. Technical Approach

### Architecture

Extend `player_load_result` with pet database identity and item identities aligned to
each `player_pet_snapshot`. The repository adds one ordered pet query, one ordered pet
item/current-owner query, and one unioned metadata query. The existing ownership
summary validates the union of player and pet serialized UIDs against active player
custody, because items carried by a player's pet retain `player(pid,0)` ownership in
the implemented movement ledger.

Refactor the Session 02 item materializer into a reusable staged graph operation. It
attaches roots only to unpublished owner characters and returns ownership entries
without mutating the global runtime map. A pet materializer validates every pet first,
creates NPCs and their graphs, then the top-level player materializer hydrates the one
combined ownership batch and commits follower relationships. `enter_game` places the
already committed followers in the player's room. Any earlier failure frees staged
NPCs and objects and leaves the owner map, room, follower list, and recovery rows
unchanged.

Recovery rows are immutable latest-checkpoint input rather than a consumable queue.
Exact descriptor/request publication prevents duplicate live players, copyover
explicitly excludes SQL pets, and the next revisioned crash or non-crash checkpoint
replaces or clears `player_pets`. This is restart-safe and avoids the legacy gap where
rows were deleted after partial publication.

### Design Patterns

- **Set-based typed repository**: fixed query count, deterministic ordering, strict
  conversion, and no live pointers in worker results.
- **Combined staged aggregate**: player, pets, objects, followers, and ownership commit
  as one game-thread unit.
- **Non-destructive checkpoint read**: durable recovery remains replayable until a
  later revisioned checkpoint supersedes it.
- **Indexed graph assembly**: maps and adjacency lists provide O(N) validation,
  metadata attachment, linking, and operation evidence.

---

## 6. Deliverables

### Files To Create

| File | Purpose | Est. Lines |
|------|---------|------------|
| `src/player_load_pets.h` | Staged pet materialization, placement, cleanup, and metrics API | ~60 |
| `src/player_load_pets.c` | Bounded pet validation, NPC/item staging, follower commit, and room placement | ~450 |
| `tests/async/test_player_load_pets.py` | Runtime, cleanup, source, bound, query, and compatibility regressions | ~450 |

### Files To Modify

| File | Changes | Est. Lines |
|------|---------|------------|
| `src/player_load_repository.h` | Add pet identities, bounds, component mask, and query budget | ~60 |
| `src/player_load_repository.c` | Add three pet queries and combined payload/custody validation | ~350 |
| `src/player_load_items.h`, `src/player_load_items.c` | Expose reusable staged graph and deferred ownership entries | ~140 |
| `src/player_load_materialize.c` | Stage player items and pets, hydrate one combined batch, and commit atomically | ~70 |
| `src/Makefile` | Build the pet materializer | ~2 |
| `src/copyover.c`, `src/nanny.c` | Exclude copyover pets, place snapshot followers, and remove legacy SQL pet load | ~35 |
| `tests/async/player_load_repository_mysql_harness.cpp` | Add connection-local pet graph, mismatch, and bound fixtures | ~240 |
| `tests/async/test_player_load_items.py`, `tests/async/test_player_load_pipeline.py` | Preserve item and complete-component contracts after shared staging | ~80 |

---

## 7. Success Criteria

### Functional Requirements

- [x] Pet, pet-item/owner, and pet-metadata reads add exactly three queries regardless
  of pet and item count.
- [x] The union of player and pet payload has an exact bijection with active player
  custody under one owner revision.
- [x] Zero, multiple, nested, equipped, and more-than-256-item pet graphs stage and
  publish once within explicit bounds; every malformed or oversized case fails closed.
- [x] Player items, pets, pet items, runtime ownership, follower relationships, and room
  placement are all absent after any pre-publication failure.
- [x] Copyover and non-crash rent paths do not duplicate SQL pets; normal crash recovery
  no longer performs per-pet queries or deletes checkpoint rows on read.
- [x] Restart, reconnect, stale completion, and duplicate completion preserve one live
  character family and retryable durable recovery state.

### Testing Requirements

- [x] Focused pet runtime/source and existing item/pipeline regressions pass.
- [x] Guarded local-development MySQL fixtures cover valid, empty, mismatch, ownership,
  metadata, and bound behavior without persistent writes.
- [x] Full repository regression gate passes.

### Non-Functional Requirements

- [x] Worker DTOs contain no live player, pet, object, room, or prototype pointers.
- [x] Query count is constant and pet/item validation plus assembly stays within an
  asserted O(P+I) operation ceiling.
- [x] Diagnostics contain only redacted outcomes and aggregate counts.

### Quality Gates

- [x] All deliverables are ASCII-encoded with Unix LF endings.
- [x] Changed C/C++ lines pass `.clang-format`.
- [x] `make -C src` passes with the warning-as-error C++20 profile.

---

## 8. Implementation Notes

### Working Assumptions

- Pet-carried items retain `item_owner_type::player` with the player's PID and context
  zero. Phase 02 has no pet owner enum, PC-to-NPC equipment movement does not transfer
  durable ownership, and pet database IDs are replaced on each checkpoint; they are
  location identities only.
- `player_pets` is the latest revisioned checkpoint payload. Phase 01 writes it inside
  the guarded player snapshot transaction, while non-crash pet capture is empty and
  therefore clears it on a later successful checkpoint.
- A newly loaded character has no pre-existing followers. Snapshot followers can be
  staged off-room, attached only after the combined aggregate succeeds, and placed
  beside the player during `enter_game`.

### Conflict Resolutions

- The phase stub says Phase 02 supports pet owner identities, but the implemented owner
  enum and movement paths do not define one. The existing player owner is retained;
  inventing owner type 9 would contradict schema checks and expand migration scope.
- The stub permits exact acknowledgement or another post-publication idempotent
  consumption rule. Destructive delete-on-read is rejected because database deletion
  cannot be atomic with live memory publication. Exact fresh-character publication plus
  revisioned checkpoint replacement is the restart-safe idempotent rule.
- Legacy code restores pets only for crash rent types after room entry, while the worker
  request does not carry `rtype`. Rows exist only for crash checkpoints and ordinary
  saves clear them; copyover explicitly opts out, preserving the intended boundary.

### Key Considerations

- All validation that can fail must precede room/follower publication and combined
  ownership hydration.
- NPC cleanup must release carried/equipped objects exactly once whether the graph is
  unlinked, linked, or attached to a staged pet.
- Existing charm, stats, equipment, inventory ordering, and same-room recovery behavior
  remain gameplay-compatible.

### Potential Challenges

- **Aggregate rollback**: Separate player/pet graph helpers must not independently
  publish runtime ownership; collect entries and hydrate once at the top level.
- **Pet lifecycle APIs**: `read_mobile`, `setup_pet`, `add_follower`, `char_to_room`,
  `free_char`, and `extract_char` have different global-state assumptions; stage with
  explicit ownership and test each cleanup phase.
- **Combined bijection**: Serialized row IDs are table-local, while stable object UIDs
  are global; validate database parent IDs within each pet and UIDs across the complete
  player/pet aggregate.

### Relevant Considerations

- [P00] **Player save and load are over-broad and inconsistent**: remove N+1 pet reads,
  fixed truncation, partial success, and delete-on-read.
- [P00] **The game thread owns mutable objects**: workers return typed values only.
- [P00] **Do not accept partial or silently truncated loads**: all bounds fail before
  any character family becomes visible.
- [P00] **Do not tune from tiny local plans**: add no index without Session 05 evidence.

### Behavioral Quality Focus

Checklist active: Yes

Top behavioral risks for this session:
- Partial follower/room/ownership mutation when a later pet or item fails.
- Duplicate pets across reconnect, stale completion, copyover, or retained recovery
  replay.
- Leaks or double extraction across staged NPC, equipment, and nested-container cleanup.

---

## 9. Testing Strategy

### Unit Tests

- Compile the real pet/item materializers with controlled mobile/object, follower,
  room, ownership, and allocation hooks; assert valid aggregate state and exact cleanup.

### Integration Tests

- Extend the local MySQL snapshot harness with temporary pet tables and authoritative
  owner fixtures for zero/many pets, nested/equipped items, metadata, union mismatch,
  over-limit rows, and fixed query count.

### Runtime Verification

- Exercise staged materialization and post-entry placement for a crash checkpoint,
  then prove copyover opts out and no synchronous `sql_load_player_pets` remains.

### Edge Cases

- Duplicate pet order/ID/UID/slot, missing or cross-pet parent, unknown mobile/object,
  cycles, excessive depth/count/bytes, invalid stats/metadata, allocation failure,
  cancellation, stale/duplicate completion, and failure after one complete pet.

---

## 10. Dependencies

### Other Sessions

- Depends on: Phase 03 Sessions 01 and 02, Phase 01 recovery, and Phase 02 ownership.
- Depended by: Phase 03 Sessions 05, 13, and 14.

---

## Next Steps

Run the `implement` workflow step to begin implementation.

# Session 09: PvP and Combat Outcome Batching

**Session ID**: `phase02-session09-pvp-and-combat-outcome-batching`
**Status**: Not Started
**Work Window**: One immutable battle-outcome boundary from participant capture through
set-based PvP rows, frag/progress changes, composed epic/currency effects, revisions,
outbox publication, and exact game-thread completion.

---

## Objective

Persist each qualifying combat or PvP outcome as one bounded typed command instead of
issuing per-participant event, leaderboard, progress, epic, artifact, cache, and player-
save calls from the combat path.

---

## Scope

### In Scope (MVP)

- Inventory post-cutover PvP death, group participant, frag, leaderboard, progress,
  blood-money, epic trigger, artifact trigger, player-log, and notification producers.
- Capture one immutable bounded battle outcome on the game thread, including stable
  participant IDs, roles, room identity, revisions, calculated deltas, and explicitly
  permitted redacted/bounded audit fields without live pointers.
- Insert the PvP event and all participant rows set-wise, then apply frag/progress and
  required Session 03/04 domain effects with stable parent or derived operation IDs.
- Advance all affected player/domain revisions and commit result plus typed cache and
  notification outbox rows before publishing in-memory frags, money, epics, or messages.
- Replace large raw SQL strings and per-recipient direct query fan-out in the write path
  with typed repositories, payload limits, and one bounded worker transaction.
- Handle duplicate combat callbacks, participant disconnect, death extraction, group
  changes after capture, ambiguous commit, and partial outbox delivery deterministically.
- Expose batch size, participant count, command age, apply latency, deadlock/retry, and
  rejection metrics through redacted diagnostics.

### Out of Scope

- Phase 03 rewrite of the recent-death and random-zone read-side N+1 queries.
- Gameplay frag, blood-money, heaven-time, or reward balance changes.
- General combat simulation refactoring.

---

## Prerequisites

- [ ] Sessions 03 and 04 epic/currency command APIs are validated.
- [ ] Session 02 outbox and Session 01 multi-key command ordering are authoritative.
- [ ] Phase 00 post-mutation victim-frag correctness remains covered.

---

## Deliverables

1. Immutable PvP/combat outcome DTO, bounds, command, repository, and completion modules
   under `src/`.
2. Set-based PvP participant, frag/progress, ledger composition, and outbox integration
   replacing audited direct write fan-out.
3. Redacted combat-domain operation, batch, retry, and latency diagnostics.
4. Focused solo/group, duplicate callback, disconnect, death, crash, replay, large-
   payload bound, and cache/publication regressions under `tests/async/`.

---

## Success Criteria

- [ ] One qualifying battle outcome has one stable parent operation and a deterministic
      complete participant/effect set.
- [ ] PvP event rows, participant rows, frag/progress state, required domain deltas,
      revisions, inbox result, and outbox rows commit consistently or not at all.
- [ ] Duplicate callback or ambiguous replay cannot duplicate frags, epics, money, or
      participant audit rows.
- [ ] Participant disconnect or group mutation after capture cannot alter the immutable
      command or redirect its result.
- [ ] Combat callbacks enqueue bounded typed values and perform no database, Redis, or
      filesystem I/O on the simulation thread.
- [ ] Private descriptions and player logs are either excluded or explicitly bounded
      and never exposed in failure logs or unrestricted SQL payloads.
- [ ] Focused regressions, formatting checks, and `make -C src` pass.

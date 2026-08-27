# Implementation Notes

- 2026-08-27: Session opened from `119b10bf` after Session 08 passed and published.
- A maximum-15-participant pointer-free command captures roles, room identity, bounded
  description, frag/epic/wallet deltas, bank vectors, and exact domain revisions before
  the combat graph or descriptor population can change.
- The combat repository locks players in PID order and accounts in normalized order,
  validates every expected revision, and commits the compatibility PvP rows, immutable
  outcome/participant records, frag baseline and ledger, progress/leaderboard state,
  deterministic epic/currency child operations, inbox result, and outbox together.
- Completion frames carry authoritative final frag, epic, wallet, bank, and revision
  state. Publication delegates to the centralized epic and currency ACK APIs; Redis
  invalidation and gameplay messages occur only on the game thread after commit.
- Generic player snapshots no longer own `frags` or `oldfrags`; the new `frag_revision`
  is initialized and loaded with the player while combat transactions own all updates.
- Equipment and player-log snapshots are intentionally excluded. Compatibility
  `pkill_info` rows receive empty bounded fields, while the retained recent-death read
  used by heaven-time calculation remains assigned to Phase 03.
- Additive, guarded schema verification, baseline, and reconciliation scripts are
  mirrored in the fresh bootstrap and the migration runner.
- Phase 03 owns the retained `setHeavenTime` recent-death read-side N+1 query.
- Equipment and player-log snapshots are excluded from the new command; compatibility
  `pkill_info` rows receive empty bounded values instead of reading mutable log/filesystem state.

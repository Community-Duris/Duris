# Implementation Notes

- 2026-08-27: Session opened from `cab7e556` after the Session 05 full gate passed.
- Generic `obj_to_*`/`obj_from_*` helpers remain synchronous because they serve many
  transient NPC and world-generation paths. Durable player-facing cross-owner entry
  points are fenced above those helpers.
- Existing per-item restore ownership checks provide the natural hydration boundary;
  they will read `item_current_owner`, not relative legacy event timing.
- `item_ownership_runtime` hydrates current item/owner revisions during authoritative
  restore and retains no live pointer. Missing or contradictory rows fail closed.
- `item_movement_transaction` captures at most 12 scalar subtree entries, submits one
  critical operation, retains offline completions, and re-resolves live topology only
  on the game thread after exact commit.
- The v2 item payload carries selected-subtree and destination topology. Same-owner
  attach/detach increments one owner revision, while cross-owner movement increments
  both; the MySQL harness proves attach and nested detach behavior.
- The closeout review caught the original same-owner topology omission before publish.
  The repository now locks the complete source root, derives the selected subtree,
  validates the destination parent revision, and updates root/parent state atomically.
- Player get, drop, put, give, corpse loot, and PC death inventory publication occur
  only in completion callbacks. Newly created objects are first adopted from system
  custody, then the originally requested movement is resubmitted.
- Bulk durable item commands currently direct players to use one-at-a-time movement so
  a single command cannot submit sibling operations with the same owner revision.
  Currency objects retain their existing non-item path.
- PC death writes an empty corpse, then chains top-level durable subtrees one ACK at a
  time. Rejection leaves the item on the dead character. Corpse owner identity combines
  player PID and save generation, so same-second deaths cannot collide.
- Redis floor records and legacy item events remain hints/audit evidence. SQL restore,
  Redis restore, and world recovery all consult `item_current_owner` as authority.
- The guarded local cutover and reconciliation completed with zero missing baselines,
  item revision mismatches, owner revision mismatches, or latest-owner mismatches.

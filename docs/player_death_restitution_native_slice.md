# Native death-restitution slice: bounded approval and lock contract

This is the first native live slice for #375. It is an approved, self-contained
per-player command, not a replacement for the maintenance-only bulk Python
apply and not a complete player snapshot write.

## Scope

- One critical command addresses one offline recipient and at most 64 individual
  item records.
- The runtime boundary must prove the recipient is offline, has no pending save,
  and can acquire the recipient-only save/login fence.
- The fence is held until the critical completion is published. No unrelated
  game runtime, player, or database session is quiesced.
- The repository uses the existing immutable restitution tables and one InnoDB
  transaction. It writes receipt/item evidence, exact `player_items` plus child
  rows, delivery rows, mutable runtime rows, source ownership changes, the
  critical outbox event, and the critical inbox completion together.

## Evidence and expected revisions

The command payload carries the immutable death operation/evidence and plan
digests, the original item payload, and the exact item projection. A delivery
is admitted only when all of these locks still match:

1. `player_data.save_revision == expected_recipient_save_revision` for the
   target player row locked `FOR UPDATE`.
2. `player_death_disposition` matches source PID, death revision, death
   operation, and `SHA256(payload) == evidence_digest`.
3. The source `item_owner_revision` exactly equals
   `expected_source_owner_revision`.
4. Every source `item_current_owner` row matches UID, root/parent UID, item
   revision, owner, state, and vnum from the command.
5. No prior `player_death_restitution_delivery` row exists for any UID, and no
   `player_items.obj_uid` projection already exists.

The critical inbox operation ID is also the restitution ID. A duplicate
operation is served by the existing inbox identity/hash check; the delivery
primary key is the cross-death UID guard.

## Deliberate unsupported cases

- Currency, complete snapshots, online recipients, pending saves, wildcard
  revisions, and missing item metadata are refused.
- Artifact commands carry and validate explicit remaining-at-loss timing or a
  UID-specific approval, but this first repository slice refuses delivery until
  the artifact-domain/legacy projections are wired. It never writes a zero or
  full-reset timer as a fallback.

## Current live-boundary status (not complete)

The game-core adapter now connects the exact runtime hooks to the public player
save/login barrier: `recipient_is_offline`, `pending_save_is_empty`,
`acquire_target_save_login_fence`, and `release_target_save_login_fence`.
The restricted `restitution` staff command accepts a canonical command whose
source is `operator_repair`, verifies the staff identity/level and canonical
round trip, and submits it through the adapter. Large payloads use bounded
`begin`/`chunk`/`commit` staging; the fence remains held for ambiguous outcomes.

This is still not a complete restitution feature or a claim that the live
acceptance work is finished. The native SQL/repository rollback verification,
artifact-domain projections, and the final post-delivery snapshot writer remain
separate acceptance work. A full inspect/approve plan editor is not provided;
the command requires an approved canonical payload from the upstream approval
workflow.

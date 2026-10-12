# Accounting requirements retained for completion

The [completion plan](FINISH_ACCOUNTING_PLAN.md) is the execution order. This
document preserves the original R1–R8 acceptance contract and product decisions
from paused source `11ea970dc616a04069cda703d3afe9cc02b4c7d1` without repeated
milestone/status entries. No requirement closes through documentation cleanup.

## Product boundary

Account durable player-facing money and item mutations. Money includes wallets,
shared banks, coin piles, auction escrow, pending claims and real durable
treasuries, with named issuance, expense, restitution and opening counterparties.
Track admitted UID origin, live/historical custody and topology, same-owner
movement and intentional destruction. Record actual trade consideration/fees.
Estimated inventory market values cannot balance money postings.

MySQL/MariaDB is the first qualification milestone; flatfile parity remains
required for full completion. Both backends preserve the same economic decisions
and operation identity. Production migration, deployment, repairs, restitution
and activation require their separate authorization.

## Required invariants

| ID | Requirement and proof required for completion |
| --- | --- |
| R1: one operation | Admit a bounded, policy-versioned intent before mutation. Resolve it under native authority locks. Commit domain state, root/child evidence, source claim, receipt, and outbox as one operation. Preserve exact-ID replay, changed-request rejection, ambiguous-commit recovery, savepoint rollback, and post-commit publication. |
| R2: money | For every value change, record actual denomination before/after vectors, checked copper totals, durable account lifetimes, and postings summing to zero per root. A coin pile's UID is its holding identity; a custody-only move leaves its value unchanged. Missing or unsupported holdings fail before mutation. |
| R3: supply policy | Give each issuance, expense, and restitution a named, versioned authority and durable source event. Enforce sign and counterparty rules; dedupe logical events across retry, restart, and epoch changes. Opening equity is restricted to a witnessed baseline and never funds gameplay. |
| R4: item supply and custody | Link every admitted creation, transfer, same-owner/topology change, and destruction to the existing item UID, current-owner row, immutable ownership event, and accounting root/child. Preserve unique live custody, acyclic topology, never-reused UID lifetimes, historical tombstone topology, and exact source attribution. Loading, staging, cleanup, and projection reconstruction do not mint or destroy items. |
| R5: compound gameplay | Couple the coin and item effects of shops, auctions, collector, crafting/refining, death/resurrection, and restoration at each existing critical commit boundary. Do not invent one transaction across separate death and later resurrection operations. Preserve current prices, fees, failed outcomes, and gameplay rules. |
| R6: coverage and activation | Reach every real writer, including direct fields, SQL, special procedures, admin actions, rewards, world resets, and lifecycle routes. Classify nonwriters/projections explicitly. A supported writer has an executable same-root test; an unsupported writer refuses before mutation in an active epoch. Activate only from a quiesced, source-complete baseline with retained maintenance authority, pending-command resolution, and a reversible pause path. |
| R7: independent audit | Reconstruct money holdings and item custody from native authority and immutable evidence using a separate read-only reconciler. Report unbalanced roots, orphan/missing/duplicate references, unaccounted native effects, unknown legacy origins, and source-event reuse without auto-correcting. Provide bounded operator queries for provenance, issuance/sinks, holdings, realized prices, and exceptions. |
| R8: lifecycle and qualification | Register tables/files in migration, boot compatibility, backup/restore, deletion/erasure, retention, and export paths. Preserve non-personal economic identity after alias erasure. Prove fresh and upgraded databases on MySQL and MariaDB, flatfile recovery, real player journeys, replay/restart/lost-reply faults, and measured workload budgets before full completion. |

## Product decisions and unresolved policy

These decisions must be resolved in the relevant plan before enabling the route;
the safe current behavior is refusal or inactive legacy operation.

- Shopkeepers operate against a shared system sink/issuance account rather than
  individual per-NPC wallets. Keeper VNUM and shop ID are preserved on accounting
  and provenance metadata for tracking.
- Blackjack is permanently deprecated under active epochs (active refusal guard
  is maintained). Code and zone objects will be removed post-release.
- Auction escrow: Outbid funds credit automatically to the bidder's pending claim
  account. Re-bidding applies available claim credit first with structured command
  feedback. Players can cash out via `auction pickup`.
- Death and resurrection: Death and resurrection are separate atomic operations.
  Death extraction must boundedly transition to the account menu and detach
  descriptors cleanly to prevent stranded player instances. Resurrection reverses
  unlooted corpse items back to the player, dropping any currently equipped items
  to the room floor.
- Day 1 non-negotiable gameplay mechanics: NPC item-give quests (sequential turn-ins
  via `quest_turnin`), bartender quests, mob death loot/coin issuance, zone reset
  spawns, uncursing item drops (`remove curse`), and item consumables (potions,
  bandages, keys) must be fully supported with durable UIDs and zero refusals.
- Determine whether the existing aggregate pending-claim store can retain exact
  source-operation attribution for every non-auction and auction claim. Keep it
  if it can; add rows only for a proven gap.
- Decide the governance and operator roles for corrections, restitution,
  financial retention, and export before those paths are enabled. Corrections
  append linked evidence and never edit committed history.


## Required architecture work

The required Save/Coordinator/Item movement, WORLD/reset/birth, SHOP recovery and
backend responsibility boundaries remain part of completion. Follow M3 in
the completion plan: one connected boundary at a time, actual callers and owners,
preserved private capabilities/wire semantics, verifiable storage allowances and
meaningful production/executable checks. Avoid duplicate mutation/recovery owners.

## Release test

Full completion requires current semantic route coverage with zero unclassified
reachable writers; real same-root execution for supported routes and pre-mutation
refusal for unsupported routes; independent reconciliation of admitted holdings
and UIDs; player-visible recovery after restart; MySQL, MariaDB and flatfile
qualification; lifecycle and measured workload/32 MiB host proof. Historical
Plan 1 acceptance remains scoped. Source contracts, codecs, synthetic fixtures,
recipe adoption and older successful binaries do not meet this release test.

Detailed historical status and failures remain available at the
[preserved original requirements](https://github.com/Community-Duris/Duris/blob/11ea970dc616a04069cda703d3afe9cc02b4c7d1/docs/persistence/economy_accounting/REMAINING_REQUIREMENTS.md).

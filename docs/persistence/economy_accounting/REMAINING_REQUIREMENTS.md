# Double-entry economy: remaining requirements

The R1-R8 requirement contract below remains the feature checklist;
[the economy contract](../ECONOMY_ACCOUNTING.md) defines the wire, identity, and
conservation rules. The evidence table and counts below are the historical
2026-09-27 assessment at add-double-entry HEAD `49af585c4`. Use the
[active completion plan](FINISH_ACCOUNTING_PLAN.md) and
[latest review status](REVIEW_STATUS_2026-10-04.md) for current implementation
and qualification status. The older continuation retains historical evidence. The [delivery plan](DELIVERY_PLAN.md) links five
executable work plans. A component test or source reference is not a qualified
player journey.

Plan 1's major qualification batch is underway on production source `9fabe54bb`.
Both strict builds, focused components, worker/journal guard cases and both-engine
bank coordinator/pool checks pass within their stated scopes. Coin/item/lifecycle
reruns and native production publication/recovery remain open. No R1–R8 or
major-plan acceptance gate is waived or marked complete by these partial results.

## Product boundary

Account all durable player-facing money and item economy mutations. Money means
wallets, shared banks, coin piles, auction escrow, pending claims, and real
durable treasuries, with explicit issuance, expense, restitution, and opening
counterparties. Item tracking means each admitted UID's origin, current and
historical custody/topology, same-owner movement, and final destruction. A trade
records the actual consideration and fees alongside the item events. Item
inventories do not acquire an estimated market-value balance in this feature;
such values cannot be used to balance coin postings.

MySQL/MariaDB is the first qualification target. Flatfile remains required for
full-feature completion. Both backends must make the same economic decisions
and preserve operation identity, even if their storage mechanics differ.
Production migration, deployment, data repair, and restitution need separate
authorization.

## Evidence at this head

| State | Evidence | Limit |
| --- | --- | --- |
| Available | EAI1 intent, EAP1 plan, SQL accounting tables, flatfile evidence store, typed ATM owners, staged SQL wallet/bank/pile baseline, and boot-time SQL lifecycle guard. | The staged SQL receipt leaves active_epoch null. There is no gameplay cutover. |
| Connected in components | SQL coin-transfer accounting records balanced postings and pile item references; typed item-transfer owners record custody references and source claims; some producers prepare schema-2 ATM, coin, and item commands. | SQL pooled apply and reconcile currently admit only accounted bank commands. Direct repository and harness coverage does not prove a coordinator journey for coin or item commands. |
| Still legacy or partial | Auction, collector, shop, corpse lifecycle, restitution, many grants/costs, NPC/casino flows, and flatfile coin transfer. | Domain currency and ownership ledgers alone are not operation-level accounting. |
| Inventory | The draft registry has 115 writer rows; the tracked matrix lists 116 reviewed candidates and seven schema-2 producer routes after the corpse item commit. | Its repository_head field still names an older source snapshot and its unified-evidence count omits the later SQL coin component. The contract validator reports writer-census drift. Neither count proves a complete semantic inventory. |

The 2026-09-25 death/resurrection RED observation in
[the journey contract](../../death-resurrection-double-entry-contract.md) is
historical. Later commits added some item custody and SQL coin-pile accounting;
the complete death/resurrection money and item journey has not been requalified
at this head.

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

## Decisions still needed during implementation

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

## Release test

Full completion requires a current route inventory with zero unclassified real
writers, both backends qualified for each supported route, explicit refusal for
every unsupported route, independent reconciliation of every admitted holding
and UID, and player-visible recovery after restart. A green pure codec, source
contract, or synthetic fixture suite is useful component evidence but does not
meet this release test.

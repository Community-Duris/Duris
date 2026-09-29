# Finish accounting implementation plan

Updated: 2026-09-29. **Status: accounting activation and release remain blocked.**
SQL is the first delivery target; flatfile parity follows. This plan tracks the
current work. [Implementation history](FINISH_ACCOUNTING_IMPLEMENTATION_HISTORY.md)
preserves the dated checkpoints and their original evidence.

## Delivery gates

| Gate | Required outcome |
| --- | --- |
| Stable SQL gameplay candidate | Saves, item movement, quest rewards, death, and corpse recovery survive disconnect, database interruption, copyover, and restart on one reproducible build. Accounting stays guarded. |
| Complete SQL accounting candidate | Every supported money and item writer has durable native effects, balanced evidence, publication, and recovery proof. Explicitly unsupported routes refuse before mutation; ordinary core gameplay cannot qualify by refusal. Independent reconciliation and guarded activation pass. |
| Full feature completion | Flatfile reaches the same supported behavior and passes native replay, audit, and recovery journeys. |

## Current state

| Area | Implemented | Still required |
| --- | --- | --- |
| Writer inventory | The generated matrix covers 861 routes, 2,811 lexical occurrences, and 2,753 unique sites with zero unmapped sites. | Executable route evidence and full native coverage; the release validator remains blocked. The scanner alone is not release proof. |
| Quest rewards | Durable offering continuations, item/cash recovery, eligible skills, and SQL-primary XP entitlements use stable operation IDs. Admission freezes in-game group members present in the room. A linkdead character still present receives its reward in the same completion pass. | SQL server restart and save/custody race journeys. Flatfile group XP is refused before offering consumption until parity exists. |
| Item publication | Quest, forced-drop, and soulbind owners have replay paths. Unsupported committed spell effects retain their publication fence. Snapshot schema 12 and migration 0049 provide receipt storage in the same SQL transaction as the owning player save. | Connect effect owners to receipt load/save acknowledgement, then enable safe committed replay. Room and event effects need their own durable application policy. |
| Money and audit | Typed currency/item operations and a read-only SQL cut cover mapped wallets, banks, coin piles, auction escrow, pending claims, and shop cash. UID lineage, sources, and realized prices have partial reconciliation. | Complete native origin, source, holding, and UID-history coverage; prove every supported writer and activation guard. |
| Death and corpse | Terminal fences, conflict evidence, copyover retry scaffolds, and account-reward corpse custody guards exist. | Real-PC death, loot, decay, resurrection, dispute recovery, and restart qualification with exact original UIDs and value. |
| Flatfile | Selected quest and item recovery paths exist. | Equivalent authority, accounting, receipts, audits, lifecycle, and failure recovery. |

**Scope rule:** ordinary XP gain is outside economic double-entry accounting.
Quest XP receipts make progression replay-safe; they are not ledger postings.
A logged-out character cannot join a live quest group. A linkdead character
remaining in the game can participate and is paid without a descriptor check.

## Work order

1. **Close coupled save and publication gaps.** Finish spell-effect owner
   receipts and acknowledgements. Prove that a committed quest consumes its
   offerings once and delivers or visibly retains every reward across process
   crashes, missing NPCs, disconnect, and restart. Repair stale save/custody
   races at their native authority boundary.
2. **Qualify ordinary SQL money and items.** Exercise get, drop, give, equip,
   nested storage, pets, room recovery, banks, coin piles, transfers, and group
   splits. Preserve each item UID and payload, exact denominations, source
   identity, owner revision, and accounting reference through retry and replay.
3. **Qualify death and corpse recovery.** Link wallet conversion, corpse handoff,
   terminal save, and character release. Recover healthy and disputed cases
   without a second corpse or silent asset loss. Verify later loot, decay, and
   resurrection against surviving custody.
4. **Finish compound writers.** Complete shops, collectors, auctions, crafting,
   spell inputs/outputs, world loot/resets, rewards, and staff actions. Couple
   each operation's native changes with its money postings, item references,
   source claims, and recovery obligation. Keep unsupported routes guarded
   before mutation.
5. **Finish independent audit and activation.** Derive holdings and UID custody
   from native stores, reconcile every root and source, and fail specifically on
   missing or conflicting evidence. Qualify schema, backup/restore, deletion,
   retention, and activation on disposable MySQL and MariaDB targets.
6. **Qualify the integrated binary, then flatfile.** Run the full gameplay and
   fault matrix on one build, record its hash and schema, and finish equivalent
   flatfile journeys. Production migration and deployment require separate
   owner authorization.

## Nonnegotiable invariants

- Native authority decides balances and custody; stale snapshots and live
  projections never overwrite newer committed outcomes.
- A command's native effect, required accounting evidence, source claim, and
  recovery obligation share its appropriate durable boundary. Changed-intent
  replay under the same operation ID fails.
- Money postings balance exact value and denominations against real holdings
  or named issuance, expense, opening, or restitution policies. Item custody
  does not acquire an estimated market value.
- Each admitted item UID has one valid live owner or a tombstone, acyclic
  topology, original payload, and traceable creation and retirement.
- A committed result remains recoverable after lost replies, disconnect,
  copyover, and restart. An unresolved conflict retains evidence and a
  player-visible recovery state.

## Proof gates

During implementation, keep checks focused: changed-line formatting,
`make -C src`, the relevant focused executable or disposable SQL fixture, and
`python3 scripts/validate_economy_accounting.py` plus the generated-matrix
`--check`. Run the broader gameplay, crash-point, dual-engine schema, audit,
`make test-all`, and `make test-db` gates on the integrated candidate. The
release validator must pass before accounting activation. Record unsupported
checks as open gates; do not infer runtime correctness from a codec or source
contract alone.

See [remaining requirements](REMAINING_REQUIREMENTS.md) for R1-R8,
[delivery plans](DELIVERY_PLAN.md) for domain contracts, the
[qualification checkpoint](QUALIFICATION_CHECKPOINT_2026-09-29.md) for dated
build and journey evidence, and the
[current writer matrix](writer_coverage_matrix.json) for inventory state.

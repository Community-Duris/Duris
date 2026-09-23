# Item custody implementation status

This is the implementation companion to [the `codex/master-stable` audit](ITEM_OWNERSHIP_PERSISTENCE_MASTER_STABLE.md). The audit records the baseline at `72238a8f594e0841fab7e7a5c6b27e22a57f92dd`; its finding table describes that baseline, not the patched branch. This page tracks what the `codex/item-custody-universal` branch changes and what still needs a separate authority boundary.

## Implemented in this branch

| Area | Change | Boundary or limitation |
| --- | --- | --- |
| Ordinary get, put, drop, give, and bulk variants | Their live publication callbacks now return success/failure to the generic movement coordinator. A failed post-commit publication retains the pending entry and owner fences for retry. Bulk successor actions start after publication acknowledgement. | This uses the existing transfer repository and operation ID; it does not change its SQL schema. |
| Absent item admission | A newly admitted root with a requested second move now publishes only after the final transfer. An intermediate creation under the source owner does not invoke the final callback. | Regression tests cover absent-to-destruction and absent-to-container handoffs. |
| Junk | Active player-owned roots enter one destruction batch. Live removal occurs after commit; the money award starts after publication acknowledgement. Unowned transient roots keep their synchronous behavior. | Destruction and wallet credit are two operations, so a crash between them can lose the award. A composite item-and-wallet command is needed for exactly-once reward delivery. |
| Donation | Active player items transfer to the room-owned well or to destruction for duplicates, with retained live publication. A donation-all chain advances after each acknowledgement. | An unadmitted well cannot receive an owned child. The command now refuses that case; adopting a well that already contains items requires a parent-aware reconciliation. |
| Potion use | Active carried or held potions enter destruction before spell/epic effects. The callback re-resolves durable UIDs and validates held location, and effects run after publication acknowledgement. | This handles the item and its immediate effect in the running process. It does not make a spell effect and custody destruction one durable command. |
| Broken lockpicks | The existing guarded key-break transfer also handles a broken active lockpick. | Unowned temporary picks still use the synchronous removal path. |
| Resurrection transient cleanup | Active transient target items now enter destruction instead of the raw post-claim sweep. A shared subtree predicate catches active descendants beneath temporary roots and refuses an unsafe claim. | Legacy fallback resurrection paths still need conversion. |
| Winterhaven janitor | Pickup and final cleanup skip object trees that already have an active custody row. | The janitor does not gain a generic NPC transfer operation; it leaves those roots in place. |
| Shared classification | `item_tree_has_durable_ownership()` conservatively detects a root or descendant needing custody routing. `item_tree_has_active_custody()` distinguishes already-adopted trees for scripts that may handle unadmitted props. | These predicates are a guard for callers; the low-level object helpers still do not enforce an item-change capability. |

## Work still required

The remaining audit rows have **not** been silently declared safe. They require one of these additional designs before conversion:

1. **Composite item and effect transactions:** salvage, crafting, enhance, alchemy, poison/drink, quest payments, tickets, and devices consume inputs while awarding an output, money, a spell, or a persistent payload edit. A sequence of single-item destructions would permit partial recipes or repeated rewards after a crash. Add a multi-item intent with deterministic locking and one operation ID, then convert those callers.
2. **Actorless world transitions:** decay, scrap, combat or spell forced drops, room scripts, mobile scripts, ships, and world item movement need an asynchronous adapter that can operate from room/NPC/corpse owners without requiring a connected player. The current generic game-thread submit API primarily requires a player actor. Add a typed caller context and durable publication replay before replacing raw helpers.
3. **Other player consumables:** legacy scroll recitation, memorized scrolls, faerie dust, rope, herbs, and similar callers still have direct extraction. These need command-specific continuation data so effects happen after a committed destruction and can resume after a publication retry. The existing device action scheduler also deletes spent scrolls later in a pulse.
4. **Economic publication:** shop and auction database commands already compose custody with money, but their committed live publication paths can discard a failed retry handle. Those coordinators need retained phase state, idempotent wallet/shop/item publication, and a recovery path.
5. **Item payload edits:** repair, damage, affect/keyword changes, charges, and enhancement mutate serializable fields without one owner-aware payload update API. A player dirty bit alone cannot update room, corpse, locker, or shop projections. This is being handled separately from the custody branch and needs integration before claiming universal persistence coverage.
6. **Unadmitted world containers and legacy resurrection:** a static donation well with no active root row cannot host an owned child, and legacy resurrection fallbacks still use direct moves. Both require authoritative parent/lifecycle reconciliation before enabling their old behavior.

## Verification so far

- A full Docker `build` stage compiled and linked the server and area tools with the repository warning profile. Later touched files were also compiled individually in an isolated WSL scratch checkout after the final subtree and janitor edits.
- Focused source and executable tests passed for common movement publication, absent-item handoffs, corpse haul/resurrection, bulk commands, durable put, key break, potion level gate and deferred effects, device actions, and trusted steal.
- `git diff --check` passed. A live database/gameplay journey has not yet been run; no production migration or operational script was executed.

The recommended long-term boundary remains **typed item intent, one authoritative commit, then retained live publication**. The existing transfer repository and coordinator are the starting point. The shared tree predicates and converted commands demonstrate the pattern, while the remaining rows identify the additional composite and actorless capabilities it needs to become universal.

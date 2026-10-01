# NPC alchemist abilities and vial supply (#661)

## Delivery and retained scope

This change targets master independently of the crafting conservation repair in
PR #573. NPC alchemist class identity remains supported. Assassin poison mixing,
Encrust, shared spells/item helpers, authored objects, and persisted class, skill
and command IDs remain intact. Unsupported player `do_mix` and its command entry,
exclusive recipe helpers and NPC stock-generation/count/select helpers are retired.

## Virtual combat

Both `MobCombat` and `MobSpellUp` reach one NPC-only ability. It reads cached potion
prototype spell slots, invokes the existing spell implementations at
`min(NPC level, 50)`, and creates no combat potion object, UID, custody operation,
or inventory/ground drop. Invalid or unavailable prototypes are removed from the
selection pool; an empty pool refuses without spending the action.

The explicit weights below describe intended mixtures, replacing stock-count
and loop-dependent generation. Early tiers preserve their old relative mixtures;
the higher tiers make the lower-tier branches explicit rather than depending on
accidental loop placement. Level 51's profile continues above 65.

| Level | Nitrogen 868 | Dispel 866 | Wither 865 | Slow 863 | Grease 859 | Napalm 857 | Glass 855 | Acid 853 | Greater living stone 850 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 6-10 | 1 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| 11-15 | 4 | 1 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| 16-20 | 10 | 1 | 1 | 0 | 0 | 0 | 0 | 0 | 0 |
| 21-25 | 16 | 2 | 1 | 1 | 0 | 0 | 0 | 0 | 0 |
| 26-30 | 24 | 2 | 1 | 1 | 4 | 0 | 0 | 0 | 0 |
| 31-35 | 8 | 1 | 1 | 2 | 4 | 48 | 0 | 0 | 0 |
| 36-40 | 0 | 1 | 0 | 1 | 4 | 2 | 32 | 0 | 0 |
| 41-50 | 0 | 1 | 0 | 1 | 4 | 0 | 2 | 40 | 0 |
| 51+ | 0 | 3 | 0 | 3 | 0 | 0 | 6 | 9 | 2 |

**Source correction:** greater living stone is an offensive summoned creature,
not a protective skin spell. VNUM 850 contains spell 425; its existing function
summons mobile 1105 and starts it fighting the supplied victim. It therefore
uses the opponent, never the alchemist itself. The summon retains its existing
spell owner and lifecycle; this refactor does not change that shared spell.

Target selection, agility/fumble checks, no-magic/single-file restrictions,
invisibility reveal and wake-up follow the existing applicable throw behavior.
An attempted throw returns action-consumed even when it misses or the room
blocks its effect. Rare old misthrows that could select a bystander or leave a
bottle on the ground become harmless virtual splashes. Empty-stock replenishment
and stock-triggered fleeing disappear. Each spell checks actor/victim membership
before continuing; the melee caller also checks both remain in the room after
`MobCombat` before publishing its attack messages.

## Cadence

The reference is a single-class offensive sorcerer at the same level, with slots,
a valid opponent and no protection that forces a different spell selection:
burning hands (6), acid blast (11), lightning bolt (16), cone of cold (21),
fireball (26-40), and prismatic ray (41+). This is a documented reference cadence,
not a claim that every caster's random spell choice has the same duration.

The ability uses the actual `SpellCastTime` for that reference spell and rounds
it up to a personal melee-round opportunity (`int(base_combat_round) + 1`). Its
next deadline is three such intervals later. Both AI entry points use this same
deadline. Misses also spend it. A short action wait preserves the existing turn
handling without creating another independent random frequency gate.

The executable fixture runs the actual module with actual authored potion slots,
actual skill timing/target definitions and the actual `SpellCastTime` function.
Across 60 scenarios (levels 6/21/41/65/90, spell modifiers -10/0/10, personal
round bases 3/7/15/23), it counts 60,000 scheduler pulses per scenario. Ability
calls occur at melee opportunities and mobile-update opportunities, with a
second immediate call to check the shared deadline. Measured action ratios are
**0.3333-0.3336**; the small excess is the initial action at pulse zero.

This is controlled native cadence evidence, not a full-server combat journey.
Additional stuns, multiclass behavior, random caster choices and unavailable
targets can change observed gameplay rates. The setting controls attempts,
including misses; it does not reduce spell power.

## Real vial supply

Fresh finalized zone spawns (`Y`, `M`, `F`, `R`) get one independent 1-100 roll.
Eligible NPC alchemists load exactly one real VNUM **102** on rolls 1-10. The
decision is marked before allocation, so failed allocation, inventory depletion
and repeated calls do not reroll. Summoned instances and player-owned helpers
are excluded. A normal world NPC subsequently charmed retains its existing loot.
Virtual combat never consumes the vial.

The hook is absent from generic `read_mobile` and restore paths. The runtime
marker is not a new durable spawn identity: restoring an existing NPC must keep
using the existing restore path, without invoking fresh zone-finalization hooks.
A cold world reset creates new spawns and therefore new rolls. Three real copyovers preserved selected NPC identities and exactly one vial.
SQL/Redis cold recovery also preserved a depleted NPC without rerolling its grant.

Eight independently authored VNUM 102 load lines remain in five zone files:
Alatorin (3), Khildarak (2), Llzazan (1), Surfacekeeps (1), Tharnadia (1).
Their supply remains in addition to the new per-spawn chance.

The native fixture moves a granted carried vial to a player inventory and proves
the actual Assassin `get_vial` selector recognizes it. The combined live journey also exercised trusted staff theft, actual NPC death
and corpse loot, and actual `do_mixpoison` using the stolen vial. Ordinary player
stealing is currently disabled, so the theft proof uses the supported staff route.

## Writer inventory amendment

| Route | Master status | Accounting obligation |
| --- | --- | --- |
| Player potion mixing | Executable command and exclusive helpers retired | Mark retired after this change lands on that branch |
| NPC automatic potion stock | Allocation/count/select routes retired | Remove obsolete issuance/consumption evidence after integration |
| Virtual mixture actions | Combat effects; no potion issuance | Existing spell lifecycle owners remain responsible, including stone summons |
| Automatic real VNUM 102 | New fresh-zone-spawn item writer | Durable spawn/source identity, exact UID, admission and retained publication required |
| Authored potion/vial loads | Retained | Existing independently classified world-content writers |
| Assassin poison / Encrust / Harvester | Retained; PR #573 supplies conservation repair | Exact craft receipts and branch-specific source/recovery contracts |

Master has no experimental-accounting admission API. This PR does not port the
new writer to that branch or certify durable accounting replay. That port must
bind the choice and UID to a durable spawn/source identity and refuse unsupported
active issuance **before** allocation/publication. Issue #661 stays open for that
port and its SQL/flat-file restore/replay evidence.

## Qualification

- `python3 tests/async/test_npc_alchemist_runtime.py`: native module, actual skill
  metadata/cast-time/vial selector, level boundaries, high levels, misses,
  restrictions, visibility/wake-up, removal, bad prototypes, cadence, exact
  10/100 boundary, failed allocation, depletion and summon/helper exclusions.
- Full maintained Docker-image server builds, GCC 13.3 warnings as errors:
  `make -C src -j6 PERSISTENCE_BACKEND=mariadb` and `...=flatfile`.
- Changed-line formatting, touched-file format check and `git diff --check`.
- Source guards confirm all four fresh-spawn entry points and removed helper
  unreachability. No area content or migration changes are included.

The combined #573/#663 real-server journey passed on flat-file authority and
isolated MySQL plus Redis. It observes a virtual mixture attack through the real
combat scheduler in controlled NPC combat, steals and mixes an automatically
granted vial, loots another from an actual corpse, retains NPC/player identities
through three copyovers, and verifies unchanged SQL craft runtime bytes after
cold recovery. A depleted recovered NPC does not reroll. Run
`python3 tests/async/run_alchemist_crafting_journey.py <combined-binary> file`
or `... redis` with the documented isolated SQL environment; migration adoption,
application and replay use the normal runner. The cadence fixture covers 60
level/round/modifier scenarios at 0.3333-0.3336 of the reference caster rate.

Accounting integration is a separate branch delivery: active automatic vial
issuance requires a durable zone spawn/source owner. The safe port refuses this
unsupported writer before RNG or UID allocation. Virtual combat remains usable.
No configured or production database was changed.

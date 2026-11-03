# The Underdark (`mobs_underdark`): source story review

**Scope:** review of the area's three native QST contracts, its small
dispersal-room set, and direct reset/object dependencies. No runtime quest,
route-safety, or mobile-spawn qualification was performed.

Checkpoint 288's all-title, graph, bounded typed-reference, history, and
builder-question supplement is recorded in
[MOBS_UNDERDARK_AUDIT_REVIEW.md](MOBS_UNDERDARK_AUDIT_REVIEW.md); the mapped
source-comprehensive overview is
[MOBS_UNDERDARK.md](../zone-stories/MOBS_UNDERDARK.md).

## Source shape

The active `mobs_underdark` area has 15 WLD room records, 23 mobile
prototypes, 19 object prototypes, 566 reset commands, and a QST file with
three native contracts (two `Q`, one `QA`). It has no
`mobs_underdark.story.json` mapping. All 15 room descriptions are empty; their
titles identify dispersal points around the Underdark: Hook Horror to Ghore
Shaft, lizard tunnels, water-creature and fungus areas, Arach/Goblin bubbles,
Roper/Earth dispersals, and a magma-lake point. This makes the zone look like a
spawn/dispersal layer rather than a standalone room-by-room adventure, but the
named QST story is substantial.

## Finding 1: three linked House Zarbonesti contracts

| Giver and source | Native terms | Written story outcome |
| --- | --- | --- |
| Haz'on'wyz, drow fire magus, mobile `24021` ([QST](../../../areas/qst/mobs_underdark.qst#L1), [mobile](../../../areas/mob/mobs_underdark.mob#L303)) | `Q`; give object `1515`; receive half-amulet `24013`; `D` | Haz'on'wyz wants his master Draknah's goblet. He claims to have stolen one ancient amulet half from a Zarbonesti warrior, gives it to the player, and leaves in a pillar of flame. |
| Ozman'dyzus, drow traveller, mobile `24022` ([QST](../../../areas/qst/mobs_underdark.qst#L43), [mobile](../../../areas/mob/mobs_underdark.mob#L318)) | `Q`; give bracelet `4660`; receive the other half `24014` and `C 150000`; `D` | He asks the player to infiltrate Bregnar's settlement, kill the duergar king, and bring his bracelet. He then gives the second amulet half and points the player to Kryz'Kyssik. |
| Kryz'Kyssik, prophetized hunter, mobile `24023` ([QST](../../../areas/qst/mobs_underdark.qst#L105), [mobile](../../../areas/mob/mobs_underdark.mob#L336)) | `QA`; give both halves `24013` and `24014`; receive bronze Zarbonesti seal `24016`, adamantite-hafted flail `24018`, fine drow elven scepter `24017`, and literal `R E 35000`; `D` | He says restoring the amulet restores the order of power in House Zarbonesti. He takes both halves, gives his personal crest and weapons, and departs. |

The table follows the native quest loader's direction: `G` records are offered
inputs and `R` records are received outputs ([`G` parser branch](../../../src/world/quest.c#L2202), [`R` parser branch](../../../src/world/quest.c#L2243)). This identifies the written terms only; it does not qualify live hand-in behavior.

The item chain is explicit in the static contracts: `24013` is both the first
reward and a later input; `24014` is the second reward and a later input. The
named prototypes are local: [amulet half `24013`](../../../areas/obj/mobs_underdark.obj#L212),
[amulet half `24014`](../../../areas/obj/mobs_underdark.obj#L226),
[seal `24016`](../../../areas/obj/mobs_underdark.obj#L259),
[scepter `24017`](../../../areas/obj/mobs_underdark.obj#L278), and
[flail `24018`](../../../areas/obj/mobs_underdark.obj#L298).
The second contract's bracelet link is supported by current Underworld source:
object `4660` is a shiny mithril bracelet
([prototype](../../../areas/obj/underworld.obj#L496)); the Underworld reset
equips it on Bregnar, mobile `4680`, at room `4525`
([reset placement](../../../areas/zon/underworld.zon#L275)). This agrees with
Ozman'dyzus's request to bring Bregnar's bracelet.

The first hand-in is unresolved at the prototype level: QST asks for item
`1515`, described only as Draknah's goblet, but no current `areas/obj/`
prototype with VNUM `1515` was found. Keep this as a builder question; do not
guess a replacement item. The final `R E 35000` term is retained exactly as
written and is not assigned a gameplay meaning by this review.

## Finding 2: QST owner mapping and NPC placement do not line up cleanly

The zone reset header ends at room VNUM `24015`; the three QST/mobile giver
prototypes are `24021–24023`. All three definitions are present in
[`mobs_underdark.mob`](../../../areas/mob/mobs_underdark.mob#L303) and the
three contracts are present in
[`mobs_underdark.qst`](../../../areas/qst/mobs_underdark.qst#L1). However,
only `24021` has a static reset in this zone, at the magma-lake dispersal room
`24015` ([reset](../../../areas/zon/mobs_underdark.zon#L567)). No current
`.zon` source statically resets `24022` or `24023`; another event or spawn
path could exist, so their ordinary availability is unresolved.

The derived area-story inventory assigns quest givers by the zone's room-VNUM
range. Because `24021–24023` exceed the zone's last room VNUM `24015`, its
catalog-generated `requests` list omits these three source contracts. The
row-level source register correctly records the three QST contracts from the
active `mobs_underdark.qst` file. Until ownership is reviewed, direct QST-file
evidence must remain attached to this area instead of being reported as zero
([range-based owner helper](../../../scripts/zone_story_quest_zone_inventory.py#L103),
[coverage row](ZONE_COVERAGE_REGISTER.csv)).

## Finding 3: the area has substantial reset and topology dependencies

The zone file contains 549 mobile resets and 17 give-to-mobile object resets.
The mobile reset counts resolve by current source file as follows:

| Mobile source | Prototypes referenced | Reset rows |
| --- | --- | ---: |
| `mobs_underdark.mob` | 18 prototypes from this local file | 313 |
| `underworld.mob` | `4403`, `4404`, `4405`, `4406`, `4420`, `4430`, `4450`, `4490`, `4530`, `4696` | 187 |
| `connectorzones.mob` | `53610` | 22 |
| `rtun.mob` | `90305` | 17 |
| `ghore.mob` | `11631` | 4 |
| No current mobile prototype | `1301` | 6 |

The six `M 0 1301` rows are at ZON lines 383–388 and have comments saying the
mobile type is not in the local `.mob` file
([reset block](../../../areas/zon/mobs_underdark.zon#L383)). No current
`areas/mob/` prototype `#1301` was found. A different object source has OBJ
VNUM `1301` ([`braddistock.obj`](../../../areas/obj/braddistock.obj#L1)), but an
object with the same number does not satisfy a mobile reset. The 17 `G` rows
include 14 reset commands for connector-zones' dirty coin sack `53610`
([object](../../../areas/obj/connectorzones.obj#L163)) and three for the local
dark, vile potion `24012`
([object](../../../areas/obj/mobs_underdark.obj#L200); [reset file](../../../areas/zon/mobs_underdark.zon#L103)). They are direct cross-file reset dependencies, not quest rewards.

The 15 WLD rooms author 98 exits. The static loader resolves four target
records; 94 positive target VNUMs are unresolved, including 18 that match WLD
sources in unregistered `ud` files. The full source ledger records only two
resolved active cross-area exits out of this area—room `24012` to `ixarkon`
and room `24007` to `underworld3`—with no reverse-direction records and no
active cross-area exits into this area. These are source-graph results, not
player reachability claims; see the [pair ledger](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L287)
and [unresolved-exit ledger](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L55). The
`ud` target references also support retaining those unregistered WLD
components until their typed dependencies are closed; the
[attic plan](ATTIC_MIGRATION_PLAN.md) does not approve any move.

## Builder questions and recommended resolutions

1. **What current item is intended by `G I 1515`?** Confirm the prototype
   identity for Draknah's goblet before considering any QST change.
2. **Are Ozman'dyzus and Kryz'Kyssik intentionally missing static reset rows?**
   Identify any expected dynamic/event placement if one exists; do not promise
   the chain is ordinarily available based only on mobile and QST prototypes.
3. **Should the story-catalog owner mapping include giver VNUMs `24021–24023`?**
   Confirm the source owner and how out-of-range QST givers should be surfaced.
   Keep all three contracts in the coverage register during that review.
4. **What mobile prototype should the six `M 0 1301` reset rows use, if any?**
   Confirm against the reset intent; do not substitute the same-number object.
5. **Are the 94 unresolved dispersion exits intentional mapping/pruning
   targets?** Review the 18 `ud` matches and the two one-way active destinations
   before any route repair or source disposition. Do not infer that a dead-end,
   one-way link, or unreachable helper is safe to remove.

## Safe resolution summary

Keep the active area and its WLD/MOB/OBJ/ZON/QST sources in place. Preserve the
three-contract amulet arc and exact item terms while the goblet and owner
questions are reviewed. No gameplay, access, combat, route, population,
reward, source-placement, or migration change is proposed here.

# Sea Kingdom (Priority 210) repository-wide audit review

This review supplements the [Sea Kingdom source dossier](../zone-stories/SEAKNGDM.md) with the active and inactive WLD sweep, typed cross-area references, compiled prototype uses, bounded title/history review, and migration disposition. The findings describe source data and do not establish installed route admission or player qualification.

## Active map and boundaries

Sea Kingdom contains 225 rooms, #31500–#31724, and 558/558 loader-resolving exits: 557 local and one outbound. The local weak components have sizes 117, 65, 22, 16, and 5. Raw directed traversal from #31500 reaches 113 rooms; 23 rooms have a directed return path to the root. Four rooms carry fall chances and no local room has a current flag. The static graph does not qualify door, switch, portal, hazard, or travel state.

The outbound edge reciprocates with Plane Water: Sea Kingdom #31500 up reaches #23281, and Plane Water #23281 down returns to #31500. A second, one-way active inbound comes from White Harbor #55634 southeast to Sea Kingdom #31668; there is no reciprocal Sea Kingdom WLD edge. The White Harbor source is titled “First teleport room for gnomish key”; controller use and actual access remain unqualified. See the [Sea Kingdom source](../../../areas/wld/seakngdm.wld#L11), [Plane Water source](../../../areas/wld/plane_water_one.wld#L3397), [White Harbor source](../../../areas/wld/wh.wld#L13122), exit-register [row 380](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L380), [row 436](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L436), and [row 806](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L806), plus pair-summary [rows 353](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L353), [422](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L422), and [811](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L811).

## Inactive WLD sweep

All 95 inactive WLD files were parsed: 527,791 rooms and 2,080,773 exits, with no parse errors. There is no inactive room overlap with Sea Kingdom and no inactive WLD exit targets its rooms. The current curated source-only totals remain 4,500 overall: Surf 264, `ud_map1` two, `ud_map2` 20, and Underworld2 eight. No candidate row is added or changed for Sea Kingdom.

## Typed cross-area dependencies

Active Alatorin QST rows reference Sea Kingdom objects #31544, the blue dragonscale tunic, in two `R I` commands, and #31538, the Eye of the Serpent, in a `G I` command. The matching Alatorin story sidecar encodes #31544 in accepted completion keys and #31538 in both the contract and typed `item_vnums` field. See [Alatorin QST lines 5788, 5877, and 6525](../../../areas/qst/alatorin.qst#L5788) and [story lines 3043–3618](../../../areas/story/alatorin.story.json#L3043). Active Heavens QST line 114 has a `G I` reference to Sea Kingdom object #31549, the glowing necklace; see [Heavens QST](../../../areas/qst/heavens.qst#L114) and [object #31549](../../../areas/obj/seakngdm.obj#L839).

Sea Kingdom's active reset table imports Heavens object #359, the rune-covered stone, with a G reset at [seakngdm.zon line 321](../../../areas/zon/seakngdm.zon#L321); the object is assigned `epic_stone` behavior in [specs.assign.c line 1604](../../../src/specs/specs.assign.c#L1604). No external ZON reset-role reference targets a Sea Kingdom prototype. No external SHP product, duplicate external mobile/object prototype, or external type-25 portal target was found; the current source dossier's bounded scan covers 872 type-25 declarations. Sea Kingdom has no local SHP file.

The broad sidecar token scan found ten numeric matches in Alatorin's story file. Typed references are the #31538 item and the #31544 completion-key entries above. Two other matches, #31704 inside the story ID at [line 489](../../../areas/story/alatorin.story.json#L489) and #31619 inside the ID at [line 2178](../../../areas/story/alatorin.story.json#L2178), are string-ID substrings, not Sea Kingdom VNUMs.

## Compiled prototype references

The compiled source uses local Sea Kingdom objects #31517, pauldrons of the tide, and #31527, oceanic cloak, in Chaos equipment arrays at [chaos_eq_data.h lines 81 and 158](../../../src/account/chaos_eq_data.h#L81). Tsunami object #31514 is bound to `SeaKingdom_Tsunami` and the native artifact action path; see [specs.assign.c line 1706](../../../src/specs/specs.assign.c#L1706), [tsunami_actions.c](../../../src/item/tsunami_actions.c#L16), and [specs.underworld.c](../../../src/specs/specs.underworld.c#L4164). Glowing necklace #31549 is bound to its object handler at [specs.assign.c line 1440](../../../src/specs/specs.assign.c#L1440).

Sea Kingdom vault #31724 is also an explicit world-code destination: moonstone fragment seeding places an object there in [ship_shop.c line 3651](../../../src/ships/ship_shop.c#L3651), and eligible water-dropped valuables can be redirected there in [handler.c line 2844](../../../src/world/handler.c#L2844). A raw #31524 match in [native_mobile_birth_recovery.c line 16](../../../src/economy/native_mobile_birth_recovery.c#L16) is part of magic constant `0x31524d4e`, not a Sea Kingdom prototype reference. No direct compiled mobile-prototype use was found.

## Title and bounded history

The normalized title “Sea Kingdom” is unique among 442 scanned `.zon` files. Followed WLD, MOB, OBJ, ZON, and QST histories remain on the seakngdm stem from the 2009 areas-tree move; latest same-stem content commits include WLD and QST in 2013, ZON in 2015, MOB in 2025, and OBJ in 2026. The story sidecar was added in 2026. No alternate-stem rename, distinct predecessor, or whole-zone replacement was found. See the [title source](../../../areas/zon/seakngdm.zon#L2).

## Migration disposition

Keep active `seakngdm` **DO NOT MOVE**. Preserve the reciprocal Plane Water boundary, the one-way White Harbor source edge, Alatorin's item exchange references, Heavens' necklace reference, and Sea Kingdom's Heavens #359 reset and compiled vault/artifact bindings. No inactive-WLD approach or candidate row is added. See [Sea Kingdom coverage row 262](ZONE_COVERAGE_REGISTER.csv#L262), [Plane Water row 234](ZONE_COVERAGE_REGISTER.csv#L234), [Alatorin row 6](ZONE_COVERAGE_REGISTER.csv#L6), [Heavens row 121](ZONE_COVERAGE_REGISTER.csv#L121), and [White Harbor row 345](ZONE_COVERAGE_REGISTER.csv#L345).

## Checkpoint 350: Static reconciliation and builder decisions

The current scan parsed all 351 active WLD contributors. Sea Kingdom has 225 rooms and 558/558 resolved exits: 557 local exits and one outbound. Its five weak components contain 117, 65, 22, 16, and 5 rooms. The reciprocal Plane Water route is Sea Kingdom #31500 up to #23281 and Plane Water #23281 down to #31500. White Harbor #55634 southeast enters #31668 without a reverse Sea Kingdom edge. The White Harbor source room is still titled “First teleport room for gnomish key”; the scan did not establish the controller or player access. See the [Sea Kingdom WLD](../../../areas/wld/seakngdm.wld#L11), [Plane Water WLD](../../../areas/wld/plane_water_one.wld#L3397), and [White Harbor WLD](../../../areas/wld/wh.wld#L13122).

Twenty local D resets each have 100% chance and match twenty of the 24 local exits whose raw exit-info value is nonzero. The D reset state values occur as 1×5, 2×4, 4×3, 6×2, 8×5, and 12×1; these are authored values, not a played qualification of the resulting door state. The remaining four flagged directions form two pairs: #31595 down/#31596 up and #31607 up/#31608 down. Each has raw exit-info 4, which is EX_LOCKED without EX_ISDOOR, and no matching D reset. Generic key/lock and bash paths require EX_ISDOOR; preserve these as explicit builder checks rather than inferring working locks. See [#31595/#31596](../../../areas/wld/seakngdm.wld#L1931), [#31607/#31608](../../../areas/wld/seakngdm.wld#L2136), [exit flag definitions](../../../src/core/defines.h#L442), [lock handling](../../../src/cmd/actnew.c#L3145), and [bash handling](../../../src/classes/innates.c#L2970).

| Comparison root | Raw directed reach / return | After suppressing every nonzero exit-info direction |
| --- | ---: | ---: |
| #31500 | 113 / 23 | 1 / 1 |
| #31501 | 63 / 43 | 3 / 3 |
| #31503 | 113 / 23 | 99 / 22 |
| #31504 | 113 / 23 | 99 / 22 |
| #31505 | 113 / 23 | 99 / 22 |
| #31668, active White Harbor arrival | 63 / 43 | 1 / 3 |

Suppressing every nonzero exit-info direction is a deliberately conservative graph comparison. It treats open door flags, key doors, secret routes, and other flagged exits alike, and does not model switches, portals, travel code, or actual reset/runtime state. The source has four fall markers on living mast rooms #31714–#31717: 40 at #31714/#31716 and 45 at #31715/#31717. It has no current flags. The comprehensive dossier maps eight type-25 travel templates and four ITEM_SWITCH controls; actual access, admission, effects, and return remain unqualified.

The refreshed inactive-WLD sweep parsed 95 files without errors and found no Sea Kingdom room-VNUM overlap or inactive exit target. The source-only totals remain 4,500 overall and 264 for Surf; no Sea Kingdom candidate row is added.

### Builder questions and recommended resolutions

The [source dossier follow-up table](../zone-stories/SEAKNGDM.md#owned-prerequisites-gaps-and-fair-repair-follow-ups) contains 62 detailed source-backed prerequisites across the mapped stories. These twenty grouped questions expose the remaining design decisions; they call for builder intent or future qualification, not an unreviewed source repair.

| # | Builder question | Recommended resolution |
| ---: | --- | --- |
| 1 | Can an arriving player meet the fisherman, supply an eligible loose #31529 half, receive #31531, and observe the native departure exactly once? (ZSQ 01–08) | Keep the native one-card exchange; qualify giver generation, accepted receipt, reward publication, saved custody, replay, and soul outcome separately. |
| 2 | Is the Morkoth source and the fisherman at #31671 available at the intended rarity, despite the declared cap/reset and routes toward no-exit #31672? (ZSQ 03, 08, 11–12) | Confirm intended availability and recovery before changing spawn, movement, or rarity behavior. |
| 3 | What is the intended admission sequence through the nightstone gate from #31500 to #31507, including the reciprocal side and reset state? (ZSQ 19–20) | Document the required key/controller and safe return while preserving the authored challenge and reset lifetime. |
| 4 | Is the reciprocal Plane Water route expected to be a normal player entrance and return? (ZSQ 16) | Qualify a live arrival and surviving return on both ends; retain both typed active edges. |
| 5 | Does White Harbor's “gnomish key” teleport lead still have a maintained controller and intended player access? (ZSQ 17–18) | Decide whether the one-way #55634 to #31668 edge is active, retired, or unfinished before changing it. |
| 6 | Are the four #31595/#31596 and #31607/#31608 directions intended to behave as locked doors even though their raw flags set EX_LOCKED without EX_ISDOOR and they have no D resets? | Confirm the intended door bits and control path; any correction needs a separate named repair and two-sided state validation. |
| 7 | Are the four key sources #31535, #31508, #31553, and #31557 obtainable on the intended branches without bypassing their reset and return costs? (ZSQ 19–23) | Qualify source custody, keyhole fit, open/cross result, reciprocal state, and recovery before changing key placement or locks. |
| 8 | Do the four switches at #31506, #31510, #31511, and #31512 expose the intended blocked/secret passages on both sides? (ZSQ 24–26) | Validate each selected object, action, state mutation, reverse edge, and no-op outcome; keep the current puzzle distinctions. |
| 9 | Which of the eight type-25 templates are intended as practical travel routes, and what are their usable return paths? (ZSQ 27–31) | Qualify actual command selection, object stock, living destination, refusal paths, and return; unlimited charges do not prove a route works for players. |
| 10 | Are the five weak components and the large loss of root reach under conservative door suppression intended map structure? | Identify load-only, portal, switch, or encounter links for each separated branch; do not treat a graph count as a runtime blockage. |
| 11 | Is the water-current prose meant to describe scenery while the source has zero current flags? | Confirm whether any mechanical current is intended; preserve prose and room fields until builders decide. |
| 12 | What is the expected survivable traversal and landing outcome on living mast rooms #31714–#31717 with F values 40 at #31714/#31716 and 45 at #31715/#31717? (ZSQ 36) | Qualify mount, climb, fall, landing, and return outcomes before any safety or route claim. |
| 13 | What attainable action actually protects the sailors threatened by the Morkoths, golems, and Sea Horde? (ZSQ 39–40) | Define an admitted intervention, surviving recipient, and durable result; combat, loot, or followers alone are not rescue. |
| 14 | Are the ghost-to-living Ultranos routes and the mixed port/starboard prose intended to represent one navigable ship state? (ZSQ 32–38) | Set route orientation and a clear restored or concluded voyage outcome before changing prose or topology. |
| 15 | Are the Poseidon scavenger's necklace route and the glowing necklace's inaccessible high-roll branches intentional? (ZSQ 41–44) | Decide the intended pickup, wear/command, chance branches, and useful beneficiary while preserving the current cost and PvP rules. |
| 16 | Which useful, safe player or ship outcomes should the Tsunami and Poseidon's offensive powers support? (ZSQ 45–47) | Keep invocation separate from accepted aid; document target eligibility, ownership, cost, safety, and saved beneficiary before any balance change. |
| 17 | Which source owns each broken-moonstone fragment generation, and when may the vault receive it? (ZSQ 48–53) | Preserve the alternate Morkoth, Olhydra, automaton, and vault paths; qualify source generation, recovery, and duplicate prevention. |
| 18 | Should assembling two fragments and one core consume them only when heart #12001 is safely granted? (ZSQ 54–56) | Define custody, atomic debit, recipient/capacity, refusal, crash recovery, and replay before a named repair. |
| 19 | How should the heart's ring and crew branches settle costs, rewards, and the foreign Winterhaven consumer? (ZSQ 57–61) | Preserve distinct branch ownership; qualify item and money debits, ship entitlement, foreign receipt, and recovery as one accepted outcome. |
| 20 | Which of the eleven mapped stories are intended to award durable progress, and what counts as an accepted result? (ZSQ 62) | Keep new tracking on active READY accounting, define source/controller/beneficiary ownership, and leave daily policy disabled until renewal is qualified. |

No gameplay repair or migration is proposed by these questions. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L262), [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-350-sea-kingdom-reset-aware-connectivity-and-inactive-map-identity), [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-350-sea-kingdom-title-duplicates-and-followed-history), and [retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-350-sea-kingdom-in-place-retention).

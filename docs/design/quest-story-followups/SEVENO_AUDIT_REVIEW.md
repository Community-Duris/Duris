# Sevenoaks repository-wide source audit

This supplement reconciles active and inactive WLD maps, authored title and
history, typed object/reset consumers, and source-backed builder questions for
coverage row 263. It is a static repository review. It does not certify
installed access, successful in-game transport, quest completion, or a
migration plan. The narrative synthesis is in the [Sevenoaks source-story
map](../zone-stories/SEVENOAKS.md).

## Checkpoint 351: Sevenoaks static reconciliation and builder decisions

### Active source and authored story

Active [`areas/AREA`](../../../areas/AREA#L187) contributor `seveno` is headed
`#386` and titled “Sevenoaks”. Its [WLD](../../../areas/wld/seveno.wld#L1)
spans 38600–38676 (77 rooms); the active files provide
54 mobiles, 67 objects and 253 reset commands, with reset mode 0. There is no
local QST, SHP or story JSON file. [`areas/qst/AREA`](../../../areas/qst/AREA#L173) nevertheless lists
`seveno *386 (melkivar)`. No native quest steps or completion are present in
the current local source.

The narrative is a snowy, guarded town, a ring of differentiated streets,
and a tower containing the sisters' studies, vault, fire/air creations and a
mirror maze. Kyz's revenge against Oslyra, the Street of Bravery memorials,
Myrina's trapped reflections, the Aerlyn guards/spectre, and Wyrina's gate
claim are separate leads. The source has an Olystra/Oslyra spelling split and
two different attributions for trapped souls. See the [dossier](../zone-stories/SEVENOAKS.md#narrative-landmarks) for the room and prototype map.

### Active connectivity and reset reconciliation

The parsed active graph has 77 rooms, 222 of 222 loader-resolved exits, 221
local directed exits, one outbound boundary, two active inbound boundary
records and one reciprocal boundary. It has no unresolved positive or
negative WLD targets. Weak components are 31/14/9/8/5/5/5.

| Boundary | Source declaration | Reverse in Sevenoaks | Classification |
| --- | --- | --- | --- |
| Surface arrival | [`surface.wld` #516252 north → #38600](../../../areas/wld/surface.wld#L341217) | [`seveno.wld` #38600 south → #516252](../../../areas/wld/seveno.wld#L1) | Reciprocal active WLD pair |
| White Harbor arrival | [`wh.wld` #55632 northeast → #38632](../../../areas/wld/wh.wld#L12992) | None | Active one-way WLD arrival into the vault |

The conservative all-nonzero-exit-info suppression comparison gives 31
reachable/returnable rooms from the Surface root #38600 and 31 from the
White Harbor arrival #38632 in the raw local graph. Suppressing every local
nonzero-flag direction reduces those comparisons to 3/3 from #38600 and 1/1
from #38632. This is a static boundary comparison: it does not model the
type-25 destinations, establish door behavior, or qualify actual access.

There are 34 local nonzero exit-info directions and 34 D resets, each at
100% chance; every flagged direction has a matching reset. Raw D state values
are 1×6, 2×9, 6×18 and 14×1. The values are reported literally and have not
been observed in a running zone.

| Paired exits (forward; reverse) | Raw flags | Key VNUM | Builder point |
| --- | ---: | ---: | --- |
| #38602 north ↔ #38603 south | 3 | 38656 | Ice gate |
| #38613 northwest ↔ #38663 southeast | 7 | 38617 | Wind-key path |
| #38618 down ↔ #38642 up | 7 | 38627 | Tower/mirror connection |
| #38624 north ↔ #38625 south | 7 | 38635 | Tower entrance |
| #38625 up ↔ #38626 down | 7 | 38620 | Ayrina/Alyza floor |
| #38626 up ↔ #38627 down | 7 | 38615 | Upper study |
| #38627 up ↔ #38628 down | 7 | 38630 | Olystra/Oslyra den |
| #38628 up ↔ #38629 down | 7 | 38642 | Myrina study |
| #38629 up ↔ #38630 down | 7 | 38653 | Fuerina study |
| #38630 up ↔ #38631 down | 7 | 38658 | Vault approach |
| #38631 up ↔ #38632 down | 7 / 15 | 38636 | Vault gate |
| #38636 west ↔ #38643 east | 3 | 38626 | Mirror-maze link |
| #38640 north ↔ #38641 south | 1 | 0 | Mirror and cellar |
| #38644 north ↔ #38645 south | 1 | 0 | Mirror and cellar |
| #38648 north ↔ #38649 south | 1 | 0 | Myrina's laboratory approach |
| #38669 north ↔ #38670 south | 2 | 38654 | Closed-only raw flag pair |
| #38674 north ↔ #38675 south | 3 | 38619 | Ayrina's laboratory approach |

The #38669/#38670 pair has raw flag value 2 on each direction, without the
EX_ISDOOR bit, and does have matching D resets. Confirm whether that flag/reset
combination is intended before relying on ordinary door control. The five
fall-chance rooms and two current-marked rooms remain source counts only;
actual movement, survival and return have not been qualified.

The complete inactive-WLD sweep parsed all 95 files (527,791 rooms and
2,080,773 exits) without parse errors. It found no same-number room overlap
with Sevenoaks and exactly two distinct source-only approaches:

| Existing candidate | Source-only edge into active Sevenoaks | Source evidence | Candidate count after this checkpoint |
| --- | --- | --- | ---: |
| Surf, row 97 | `surf.wld` #209270 north → #38600, from “Partially Frozen Pathway Around the Lake” | [Surf room](../../../areas/wld/surf.wld#L194558) | 265 reviewed source-only records |
| Underworld2, row 116 | `underworld2.wld` #425645 west → #38600, from “Dried Lava of Lost Souls” | [Underworld2 room](../../../areas/wld/underworld2.wld#L68206) | 9 reviewed source-only records |

These edges are not active ingress. Preserve each on its existing candidate
row and keep both source maps in place pending their separate ownership and
tooling decisions.

### Title, VNUM identity and followed history

After color-code removal, trimming, whitespace normalization and
case-folding, “Sevenoaks” occurs once among 442 current `.zon` titles and once
among 351 active contributors. Whole-source same-type scans found no room,
mobile or object VNUM overlap with any other active or source-only area; the
inactive-WLD sweep likewise found no room-number overlap.

Followed WLD and MOB history stays on `seveno` through the 2009 area-tree
move and same-stem edits through 2015-10-13. OBJ and ZON history also stays on
`seveno`; the latest log entry is the 2026-08-01 line-ending normalization,
following the 2026-04-18 ambassador/reward update that names Sevenoaks. No
local QST history or story sidecar is present. The followed paths show no
alternate-stem predecessor or explicit whole-zone replacement. Keep active
`seveno` DO NOT MOVE.

### Typed dependencies and compiled bindings

| Direction | Source reference | VNUM / role | Finding |
| --- | --- | --- | --- |
| Local reset imports | [`seveno.zon` lines 171 and 182](../../../areas/zon/seveno.zon#L171); both placed in room #38632 | [Heavens object #359](../../../areas/obj/heavens.obj#L3725); [White Harbor object #55443](../../../areas/obj/wh.obj#L5239) | Preserve the external vault contents and their owning prototypes. |
| External reset consumer | [`alatorin.zon` line 2108](../../../areas/zon/alatorin.zon#L2108) | Local object #38639, G reset | Active cross-area reset reference. |
| External shop consumer | [`alatorin.shp` line 5](../../../areas/shp/alatorin.shp#L5) | Local object #38639, oaken quarterstaff | Active shop product reference. |
| External QST item consumer | All active QST item commands scanned | Local object range #38600–#38666 | No active external QST item command found. |
| External story JSON | Current typed-reference scan | Local room/mobile/object VNUMs | No external typed story reference found. |
| Cross-area object teleports | 872 type-25 declarations across current object sources | Destinations into rooms #38600–#38676 | No foreign type-25 target into Sevenoaks found; eleven local declarations stay within the area. |
| Compiled item behavior | [`specs.assign.c` lines 2071–2072](../../../src/specs/specs.assign.c#L2071) | Object #38649 assigned `sevenoaks_longsword`; #38647 mace assignment is commented out | Preserve the live callback and distinguish the defined but unassigned mace function. |
| Compiled CHAOS equipment data | [`src/account/chaos_eq_data.h`](../../../src/account/chaos_eq_data.h#L106) | Objects #38610, #38621, #38644, #38663 and #38664 | Preserve these references when considering any object-range or source change. |

The longsword handler is defined in
[`src/specs/specs.underworld.c`](../../../src/specs/specs.underworld.c#L4353);
`sevenoaks_mace` is defined there at line 4462 but its assignment is
commented out. The eleven local type-25 declarations are
38605→38651, 38609→38667, 38612→38606, 38613→38659, 38618→38672,
38622→38633, 38623→38615, 38624→38616, 38625→38636, 38628→38646 and
38633→38606. Declaration is not evidence of a successful use or return.

### Builder questions and recommended decisions

| # | Source question | Recommended decision before any repair or new story tracking |
| ---: | --- | --- |
| 1 | Are Olystra in room #38628 and Oslyra the chaos dragon one character, alternate spellings, or distinct characters? | Confirm canonical identity and preserve all current references until owners agree. |
| 2 | How does Aerlyn, described as a high mage, relate to the sisters, tower guards and trapped spectre? | Record the intended relationship before combining those story threads. |
| 3 | Why was Olystra moved from the holding den, and who is meant to occupy it now? | Confirm the intended room and creature history before editing its prose or reset. |
| 4 | Does the second trapped-soul account intentionally attribute imprisonment to Myrina where another attributes it to Aerlyn? | Confirm whether these are separate captives or inconsistent attribution. |
| 5 | Why does `qst/AREA` list Sevenoaks when no local QST file exists? | Identify any external/retired quest design before creating a new native chain. |
| 6 | What is the complete intended text of object #38616's cut-off note? | Recover builder copy; do not infer the missing clue from the fragment. |
| 7 | Does note #38608's “use your head” clue point to object #38609 or another solution? | Confirm the authored puzzle and success condition before adding tracking. |
| 8 | Is “A key of fire sits here” on wind key #38617 a copy error? | Confirm and correct only with owner-approved text. |
| 9 | Which commands and admission rules are intended for all eleven type-25 objects? | Confirm each destination and return path; do not assume ordinary mirror/portal prose guarantees transport. |
| 10 | Is White Harbor #55632's one-way arrival into vault #38632 intended for players, an internal teleport staging room, or a builder route? | Confirm admission and a useful return route before relying on it. |
| 11 | Is the reciprocal Surface approach to #38600 the intended public town entrance? | Confirm actual boundary access and safe return separately from the matching WLD exits. |
| 12 | Are the key identities and placements for the ice gate, tower and upper floors intentional? | Reconcile keys #38656, #38635 and the medallion-key sequence with builder notes. |
| 13 | Do all 34 matched D resets intentionally initialize all flagged directions at 100% chance? | Confirm each raw reset state and key slot; preserve exact values until reviewed. |
| 14 | Is raw flag 2 without EX_ISDOOR at #38669/#38670 intentional with its matching D resets and key #38654? | Resolve the flag/reset convention before asserting ordinary open/close behavior. |
| 15 | Are the three flag-1 mirror/cellar pairs at #38640, #38644 and #38648 doors, secret passages, or another control? | Confirm intended discovery and return behavior. |
| 16 | How should the mirror maze's seven static components relate to type-25 object routes? | Specify expected connectivity from actual object commands before topology changes. |
| 17 | Does the vault key #38636 drop from the dark cloaked imp as its reset arrangement suggests? | Confirm reset eligibility and reachable custody before describing it as an obtainable quest item. |
| 18 | Are imported Heavens #359 and White Harbor #55443 intended contents of this vault? | Preserve owner and source placement; confirm content before any movement. |
| 19 | Is Alatorin's external reset and shop use of Sevenoaks object #38639 intentional? | Preserve both active consumers and confirm product identity before any item change. |
| 20 | Are the assigned longsword callback, unassigned mace callback, and five CHAOS equipment object references all intended? | Confirm live versus retired behavior and retain each source dependency until decided. |

### Disposition

Keep active `seveno` DO NOT MOVE. Preserve the Surface pair, White Harbor's
one-way vault arrival, both imported vault objects, the Alatorin reset/shop
use of object #38639, the compiled item/equipment references and the two
separate inactive-map approaches. Surf rises from 264 to 265 reviewed
source-only records; Underworld2 rises from eight to nine; the curated total
becomes 4,502. No new candidate row or migration clearance is supported.

See [coverage row 263](ZONE_COVERAGE_REGISTER.csv#L263), the
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-351-sevenoaks-reset-aware-connectivity-and-inactive-map-identity),
[title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-351-sevenoaks-title-vnum-identity-and-followed-history),
and [retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-351-sevenoaks-in-place-retention-and-candidate-approaches).

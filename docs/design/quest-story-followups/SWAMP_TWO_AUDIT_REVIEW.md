# Swamp Two audit review

**Checkpoint 377 — source-comprehensive setting, static connectivity, bounded
typed dependencies, title/history, and builder questions.** The active source
is `area:swamp_two`, zone 261, rooms #26100–#26134. Keep it **DO NOT MOVE**.

## Review boundary

The [story dossier](../zone-stories/SWAMP_TROLL_KING.md) covers all 35 rooms,
the one local mobile and object stub, and all six door, 26 mobile, and seven
object reset commands. `swamp_two` is listed in the active [`areas/AREA`](../../../areas/AREA#L131)
and the separate [`areas/qst/AREA`](../../../areas/qst/AREA#L123), but has no
local QST, story sidecar, or SHP file. Its local shop count is zero; the quest
index entry does not supply a contract. Its two local #26100 prototypes are
“nothing” stubs not used by its reset table.

## Static connectivity and cavern routes

| Measure | Source result |
| --- | --- |
| Active WLD | 35 rooms; 85 authored directions, all 85 resolving; 84 local and one outward. |
| Active boundary | Swamp One #26019 south enters #26100; #26100 north returns to #26019. One inbound, one outbound, one reciprocal pair. |
| Raw local graph | One weak component of 35 rooms. Ignoring locks, directed traversal from #26100 reaches all 35, and each room has a path to the outward edge. |
| Hazard/source checks | No negative or unresolved target, fall marker, current marker, or inactive-WLD approach/room overlap was found. These checks do not establish safety or live movement. |
| #26134 cavern | Text calls the room completely sealed. WLD instead has inbound directions from #26106 north and #26119 east and a northwest exit to #26102 ([direction definition](../../../src/core/defines.h#L435)). The outgoing WLD state is 4, which the loader masks to 0; no D reset closes it ([loader](../../../src/world/db.c#L1552)). |

The #26134 source contradiction is unresolved by this static audit. Preserve the
WLD evidence and ask whether the prose, routes, or a separately implemented
collapse effect represents intended behavior. Do not describe it as a tested
sealed room. See [connectivity checkpoint 377](WORLD_CONNECTIVITY.md#checkpoint-377-swamp-two-connectivity-and-cavern-routes).

## Typed dependencies

| Reference | Source meaning | Preservation |
| --- | --- | --- |
| Swamp Two ZON M #26003–#26006/#26011 | Twenty-six resets in [swamp_two.zon](../../../areas/zon/swamp_two.zon#L34) import Swamp One mobiles: Mog, swamp trolls, guards, king, and elite guards. | Keep Swamp One as prototype owner and Swamp Two as reset owner. Reset caps do not establish a live total. |
| Swamp Two ZON G #26011–#26014 | Four resets beginning at [swamp_two.zon line 55](../../../areas/zon/swamp_two.zon#L55) import Swamp One’s bent key, platemail, Nightbringer, and crystal sphere. #26011 is given to a guard in #26132; #26012–#26014 are given to the king in #26133. | Preserve the typed source IDs and key relationship. Caves’ optional #26013 exchange remains separately owned by Caves; do not promise item supply. |
| Swamp Two ZON G #67238 | The troll king’s [G reset](../../../areas/zon/swamp_two.zon#L59) imports Unique object sleeves of life. | Preserve active `unique` as object owner. See the [Unique row](ZONE_COVERAGE_REGISTER.csv#L328). |
| Swamp Two ZON G #359 | The troll king’s [G reset](../../../areas/zon/swamp_two.zon#L64) imports Heavens’ rune-covered stone. A 2010 tracked ZON edit moved this reset from the ground to the king. | Preserve active Heavens as object owner and the followed reset intent. See the [Heavens row](ZONE_COVERAGE_REGISTER.csv#L121). |
| Swamp Two ZON G #55185 | The troll king’s [G reset](../../../areas/zon/swamp_two.zon#L63) imports White Harbor’s “memory of the Swamp.” White Harbor also defines a mobile #55185 barkeep ([MOB](../../../areas/mob/wh.mob#L2574)). | Preserve the object-type reference; do not conflate it with the same-number mobile. See the [White Harbor row](ZONE_COVERAGE_REGISTER.csv#L345). |
| Bounded inbound scans | No external ZON reset, QST giver/item contract, story-sidecar reference, SHP product, portal target, or exact C++ integer reference points to Swamp Two’s local #26100 prototypes or rooms. | No new local prototype owner or dependency was established. Negative findings are limited to the scanned source set. |

No inactive WLD source-only approach was found, so no attic candidate row or
source-only direction is added. Existing active area membership and all typed
item owners remain in place.

## Title, same-type identity, and followed history

The normalized title “The Lair of the Swamp Troll King” is unique among 442
current `.zon` files and 351 active contributors. Active and source-only
same-type WLD, MOB, and OBJ scans found no collision with this area. The local
MOB and OBJ files each define a #26100 stub, but they are different types and
neither is used by the local ZON.

Followed WLD/OBJ history reaches 2013-11-13; MOB/ZON history reaches
2015-10-13; tracked source history includes the 2009-08-12 area-tree move. No
alternate-stem rename, explicit predecessor, or whole-zone replacement was
established in those paths. Commit `fdad70594` on 2010-10-03 moved object #359
from a ground reset to the troll king, matching its current G reset. No local
QST, SHP, or story-sidecar history exists because those files are absent.
Checkpoint 758 classifies the bounded related-title and cross-area story
references; broader command-target alias ownership remains a separate pending
question. See [identity/history checkpoint
377](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-377-swamp-two-title-same-type-identity-and-history)
and the [semantic identity review below](#checkpoint-758-related-titles-and-cross-area-king-references).

## Checkpoint 758: Related titles and cross-area king references

The normalized title “The Lair of the Swamp Troll King” remains unique among
all 442 current `.zon` titles and its current area key is `swamp_two`. Similar
title vocabulary belongs to separate rostered identities: `swamp_one` (“Swamp”),
`lizard` (“The Lizardman Swamps of Clavikord”), `pods` (“Pharr Valley Swamp”),
and `lab` (“The Swamp Laboratory of Khul'Lor”) use swamp language; `troll_caves`,
`troll_hills`, and `torture` name other troll areas; `gibber` is the separate
“Lair of the Gibberling King.” Creature-specific Lair titles such as Asunwa
Jaguar, Tiamat, and Purple Worm share only the generic word “Lair” with this
title. Shared setting words do not establish another owner of rooms
#26100–#26134.

The troll king’s map and mobile have different source owners. Mobile #26006,
“swamp troll king,” is defined in [swamp_one.mob](../../../areas/mob/swamp_one.mob#L62);
[swamp_two.zon](../../../areas/zon/swamp_two.zon#L58) resets that mobile in the
king’s room #26133. Keep Swamp One as prototype owner and Swamp Two as
reset/map owner.

| Cross-area reference | Source meaning and identity |
| --- | --- |
| White Harbor ambassador | `wh.qst` dialogue calls the NPC “my swamp king.” The structured White Harbor story names “The Lair of the Swamp Troll King” and asks for White Harbor object #55185, whose ZON G reset places it on the king at #26133. White Harbor retains the object and contract; Swamp Two is the map and reset owner. This reference does not identify a second lair. [Dialogue](../../../areas/qst/wh.qst#L3140) · [story step](../../../areas/story/wh.story.json#L1043) · [reset](../../../areas/zon/swamp_two.zon#L63) |
| Caves of Mt. Skelenak monk | Caves QST dialogue mentions “the king of swamp trolls”; its structured optional sphere return consumes Swamp One object #26013, which Swamp Two resets on the king at #26133. Keep the quest with Caves, the prototype with Swamp One, and the reset with Swamp Two. [Dialogue](../../../areas/qst/caves_skelenak.qst#L162) · [story step](../../../areas/story/caves_skelenak.story.json#L428) · [reset](../../../areas/zon/swamp_two.zon#L62) |

These references and the repeated title vocabulary resolve to existing rostered
contributors and typed producer/reset/consumer relationships; none identifies
an alternate map, predecessor, or whole-zone replacement. The missing local
`swamp_two.qst` remains builder question 2 despite its `qst/AREA` listing. No
attic candidate or inactive-WLD approach was found, so the existing
**DO NOT MOVE** disposition and candidate register remain unchanged.

## Twenty builder questions and recommended resolutions

| # | Owner question | Evidence-backed recommendation |
| ---: | --- | --- |
| 1 | Should `swamp_two` remain a separate active area from `swamp_one` despite their shared swamp setting? | Keep both active sources and preserve their distinct room ranges, titles, and roles: trail/hidden swamp on one side and troll lair on the other. Do not merge from thematic similarity. |
| 2 | Is the `qst/AREA` listing intentional when no local QST file or native contract exists? | Preserve the index entry as source data, but do not describe it as a quest. Add a contract only after its giver, steps, reward, and lifecycle have an owner. |
| 3 | Should the Quicksand boundary remain reciprocal, given the quicksand warning at #26019? | Preserve both directions and the warning. Qualify actual traversal and safe return in a gameplay review before changing either the prose or route. |
| 4 | Should #26101 say Mog is gone while the reset places him in #26100? | Confirm whether this is intended post-encounter flavor or should be revised. No dynamic prose update is established by the reviewed sources. |
| 5 | Is Phyla’s dingy key #26005 intended to open the #26100/#26101 gate? | Preserve the key owner, its Swamp One reset on Phyla, and both Swamp Two key fields unless a replacement progression is approved. Acquisition is unqualified. |
| 6 | Is the dirty key #26007 in Swamp One’s hidden room #26036 intended to open the #26101/#26102 maze gate? | Preserve the corpse/key clue and the two matching Swamp Two key fields; verify availability separately before changing the gate. |
| 7 | Is the bent key #26011 on a guard at #26132 intended for the #26132/#26133 king-chamber door? | Preserve the guard’s G reset and both keyed exits as one source relationship. Do not relocate the key without updating both door sources. |
| 8 | Are the first two WLD state-3 doors intended to be pickproof while the final state-2 door uses the legacy pickable bit? | Confirm the intended door policy. Keep the key VNUMs and paired D resets intact until both initial flags and reset behavior have been reviewed together. |
| 9 | Are the identical names and repeated maze descriptions across #26102–#26131 intentional disorientation? | Preserve the repeated prose and all room identities pending an owner answer; the source graph is distinct even where the text is identical. |
| 10 | Is #26130 meant to funnel its incoming maze routes east to #26116 with no authored return? | Confirm the one-way route. Do not add a reverse exit from geometric expectation alone. |
| 11 | Should #26134 say it is “completely sealed” while the WLD supplies routes through the cavern? | Reconcile the prose with the two inbound directions and the northwest exit, or identify the separate collapse mechanism that is meant to gate them. No live collapse behavior was tested. |
| 12 | Are the north/east one-way directions into #26134 and its northwest route to #26102 intentional? | Preserve their source directions for now; if the cavern should truly seal, specify which routes close, when, and how players return before editing. |
| 13 | Are the repeated 19-cap troll reset commands, including duplicates at #26120, #26124, and #26129, intentional? | Preserve each reset line pending reset-semantics review; do not read the cap as a measured population or collapse duplicate commands without owner confirmation. |
| 14 | Is the local #26100 “nothing monster” prototype an intentional placeholder? | Keep it until its authoring/tooling role and any generated or indirect references are checked. It is not placed by the reviewed local ZON. |
| 15 | Is the local #26100 “nothing” object prototype an intentional placeholder? | Keep it until its authoring/tooling role and any generated or indirect references are checked. It is not placed by the reviewed local ZON. |
| 16 | Is it intentional that the active area has no local SHP, QST contract, or story sidecar? | Retain the current zero-shop/no-contract source state unless an owner defines new content and its item/reward ownership. Do not infer a quest from the quest-index listing. |
| 17 | Should the king continue to receive Unique object #67238, the sleeves of life? | Preserve the active Unique prototype and Swamp Two’s typed G reset; do not copy the item into another area. |
| 18 | Should the king continue to receive Heavens object #359, the rune-covered stone? | Preserve Heavens as owner and Swamp Two as reset consumer. The 2010 ZON history records moving this item from the ground to the king. |
| 19 | Should the king continue to receive White Harbor object #55185, the “memory of the Swamp”? | Preserve the White Harbor object reference and mark it as type OBJ; the same VNUM in `wh.mob` is a separate barkeep prototype. |
| 20 | Should the optional Caves exchange for Swamp One’s sphere #26013 and the assigned Nightbringer handler for #26014 remain separate dependencies? | Preserve Caves’ item contract and the maintained handler with their current owners. Do not promise sphere availability from Swamp Two or fold either dependency into a new local quest. See the [Swamp One dependency review](SWAMP_ONE_AUDIT_REVIEW.md#typed-dependency-closure). |

## Retention recommendation

Keep active `swamp_two` **DO NOT MOVE**. Preserve its reciprocal Swamp One
boundary, maze graph, keyed door pairs, reset ownership for Swamp One mobiles
and objects, and typed Unique/Heavens/White Harbor object references. No
inactive WLD approach or same-type collision was found. This review does not
clear a gameplay repair, a live-access claim, or attic migration; see
[retention checkpoint 377](ATTIC_MIGRATION_PLAN.md#checkpoint-377-swamp-two-in-place-retention-and-typed-dependencies).

## Checkpoint 832: Current item routes and prototype roles

An exact typed-source refresh confirms Swamp Two's reset ownership and resolves several cross-area item routes without changing the CP377 graph:

- **Reset imports:** [`swamp_two.zon`](../../../areas/zon/swamp_two.zon#L34) has 26 `M` rows for Swamp One mobiles #26003–#26006/#26011 and seven `G` rows: four Swamp One objects #26011–#26014, Unique object #67238, White Harbor object #55185, and Heavens object #359. The six G rows after the troll king reset place #67238, #26012–#26014, #55185, and #359 in room #26133; #26011 is reset with a guard in #26132.
- **Keyed maze pairs:** The three paired Swamp Two WLD doors use #26005 at #26100/#26101, #26007 at #26101/#26102, and #26011 at #26132/#26133. The six matching D resets remain in `swamp_two.zon`. Swamp One owns the key prototypes and their source resets; these references do not establish availability or successful traversal.
- **White Harbor item route:** [`wh.qst`](../../../areas/qst/wh.qst#L3149) has one `G I 55185` contract operand; [`wh.story.json`](../../../areas/story/wh.story.json#L1043) records one Ambassador request for item #55185, with the same completion signature repeated in its nested contract. Its hint names the troll king as a source, and Swamp Two resets the White Harbor OBJ there. The request and object stay White Harbor-owned; Swamp Two remains the reset/map owner.
- **Caves sphere route:** [`caves_skelenak.qst`](../../../areas/qst/caves_skelenak.qst#L203) has one `G I 26013` operand, and the Caves sidecar gives #26013 and receives #4031. Swamp Two resets the Swamp One sphere on the king, while Caves owns the optional exchange.
- **Shared Heavens stone:** Heavens OBJ #359 is reset on the king here and on Ehkahk in [`smoke.zon`](../../../areas/zon/smoke.zon#L76). The Smoke sidecar calls it the imported epic stone in prose; maintained [`specs.assign.c`](../../../src/specs/specs.assign.c#L1604) binds object #359 to `epic_stone`. This is a shared item route, not a second prototype owner.
- **Typed shop identity:** White Harbor MOB #55185 is a barkeeper and the matching `wh.shp` shop record uses that number as its keeper. Its producing list names other objects; the OBJ #55185 is not listed as shop stock. The shop loader reads producing VNUMs separately from its keeper field ([`wh.shp`](../../../areas/shp/wh.shp#L491), [`shop.c`](../../../src/economy/shop.c#L3374) and [`shop.c`](../../../src/economy/shop.c#L3489)). Do not conflate the same-number MOB, OBJ, shop keeper, and quest item.

No local QST, sidecar, or SHP file exists. The local #26100 MOB and OBJ stubs remain unplaced and have no exact external QST, sidecar, reset, shop-product, portal-target, or C++ integer reference in the bounded scan. No inactive WLD approach or candidate is established. Keep Swamp Two **DO NOT MOVE**; the existing builder questions remain the place to resolve the #26134 sealed-room contradiction and key/quest availability.


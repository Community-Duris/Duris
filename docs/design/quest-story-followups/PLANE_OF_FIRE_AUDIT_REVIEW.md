# Plane of Fire: connectivity, dependencies, history, and builder decisions

Reviewed October 10, 2026 for active `plane_fire_one`, zone 254. The source
dossier is [PLANE_OF_FIRE.md](../zone-stories/PLANE_OF_FIRE.md). This review
covers that 125-room contributor only; City of Brass is a separate area.
The former 250-range `plane_fire_one` source is preserved separately as
unlisted `brass-old-2`; see the [Old 2 audit](BRASS_OLD_2_AUDIT_REVIEW.md#followed-history-and-the-qst-index).

## Active graph and directed reachability

The WLD contains 750 exits and all resolve in the active loader: 748 are
internal and two leave the area. Its undirected view is one 125-room weak
component. Raw directed traversal from the union of active WLD arrival targets
reaches 122 of 125 rooms. Rooms #25418, #25450, and #25518 have no local
inbound WLD exit; each room's authored exits point into the map. #25450 is
Imix's reset room, so a reset location and WLD access are separate facts.
Dynamic travel and mobile behavior were not live-tested.

All authored exit flags and key VNUMs are zero. There are no negative or
unresolved WLD targets, fall-chance markers, or currents.

| Active inbound source | Direction and destination | Return evidence |
| --- | --- | --- |
| `prison.wld` #7484 | East, south, west, up, and down to #25504 ([rows 144–148](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L144)) | Five records from one source room; no reverse WLD edge |
| `rftjngle.wld` #80444 | North to #25405 ([row 973](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L973)) | No reverse WLD edge |
| `alatorin.wld` #83347 | Down to #25460 ([row 1027](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1027)) | No reverse WLD edge |
| `firep.wld` #88301 | West to #25415 ([row 1096](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1096)) | Fire #25415 west leads to Firep #88321, a different endpoint; not a reverse pair |
| `brass.wld` #139000 | South to #25455 ([row 1313](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1313)) | Reciprocal with Fire #25455 north to #139000 ([rows 398–399](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L398)) |

The two outbound records are Fire #25415 west to Firep #88321 and Fire #25455
north to Brass #139000 ([rows 398–399](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L398)).
Only the Brass boundary reciprocates at the same endpoint. Preserve the five
Prison directions as one source-room group; the row count does not establish
five independent entrances.

## Code and object routes

These routes are outside the WLD edge count and do not prove current access,
safe arrival, or return.

| Route | Source evidence | Classification |
| --- | --- | --- |
| Gate, Nether Gate, and Plane Shift | Fire anchor #25401 in the planar destination table ([spells.c](../../../src/magic/spells.c#L295)); `get_room_in_zone` ([spell_travel.c](../../../src/magic/spell_travel.c#L747)) samples the loaded zone and applies entry, no-teleport, and hometown checks | Dynamic zone destination; exact landing and return are not fixed by the anchor |
| Plane of Fire pool #25404 → Ethereal #12459 | Object values ([prototype](../../../areas/obj/plane_fire_one.obj#L59)); reset at #25422 ([reset](../../../areas/zon/plane_fire_one.zon#L9)); `magic_pool` assignment and handler ([assignment](../../../src/specs/specs.assign.c#L1971), [handler](../../../src/specs/specs.teleport_pools.c#L107)) | Declared code route with 200 damage; non-trusted users are left at one hit point if damage would be lethal. No WLD return edge is declared. |
| Smoke portal pair: Fire #25464 ↔ Smoke #139894 | Fire object #25406 points to #139894 and is reset at #25464 ([object](../../../areas/obj/plane_fire_one.obj#L83), [reset](../../../areas/zon/plane_fire_one.zon#L10)); Smoke object #139820 points back to #25464 and is reset in Smoke ([object](../../../areas/obj/smoke.obj#L201), [resets](../../../areas/zon/smoke.zon#L34)) | Paired object route represented by two active type-25 prototypes; no ordinary WLD boundary |
| Negative Plane fissure → Fire #25433 | Active type-25 object #26659 ([prototype](../../../areas/obj/negplane.obj#L930)) reset at #26742 ([reset](../../../areas/zon/negplane.zon#L169)) | Declared one-way object destination; no Fire WLD return edge |
| Ardgral and Ethereal pools → Fire #25410 | Ardgral #30404 is active and assigned `teleporting_pool` ([reset](../../../areas/zon/ardgral.zon#L18), [assignment](../../../src/specs/specs.assign.c#L1935)); Ethereal #12470 has an active reset and the same handler ([resets](../../../areas/zon/ethereal_main.zon#L18), [assignment](../../../src/specs/specs.assign.c#L1916)) | Active object-based arrivals, not WLD edges; player access and safe return remain unqualified |
| Astral pool prototype → Fire #25410 | Astral #19711 is assigned `magic_pool` ([assignment](../../../src/specs/specs.assign.c#L1895)) | Prototype/code binding only; the exact active-ZON scan found no reset for #19711, so no current placed route is established |
| Mind Travel and illithid teleport veil | Destination arrays include Fire rooms #25458/#25459 and #25458/#25459/#25484 ([psionics.c](../../../src/classes/psionics.c#L616), [specs.ixarkon.c](../../../src/specs/specs.ixarkon.c#L20)) | Compiled destination references; exact access and return are not established here |

## Inactive-WLD approaches and candidate records

The all-95 inactive-WLD sweep parsed every file without parser issues. It
found two inactive source records into Fire #25455:

| Inactive source | Edge | Classification |
| --- | --- | --- |
| `brass-old-1.wld` room #139100 | South to #25455 | The archived +100 VNUM crosswalk maps this gate to active Brass #139000, whose south edge and Fire reciprocal are already active. Active Brass #139100 is Kossuth Avenue and is only a same-number collision. Keep the old edge as predecessor topology; it is not active ingress or a new approach. See the [Brass Old 1 audit](BRASS_OLD_1_AUDIT_REVIEW.md#relationship-to-active-brass). |
| `brass-old-3.wld` room #25201 | South to #25455 | One unique-source approach, preserved on existing candidate row 19. It is not active ingress. |

No inactive room definition overlaps Fire's #25401–#25525 range. Only the
unique-source #25201 edge increases the curated source-only total, from 4,261
to 4,262. The Brass VNUM overlap remains documented but is not counted as a
unique-source approach. Candidate active-inbound counts remain zero; do not
add an active route or candidate row.

## Typed dependencies and external consumers

| Reference | Source evidence | Classification |
| --- | --- | --- |
| Brass gate guardians #25400 and #25401 | Active Brass resets at room #139000 ([resets](../../../areas/zon/brass.zon#L472)); Brass story contacts ([sidecar](../../../areas/story/brass.story.json#L122)); #25400 has `guild_guard` and #25401 has `unblock_on_death` ([assignments](../../../src/specs/specs.assign.c#L1053), [#25401 assignment](../../../src/specs/specs.assign.c#L1254)); the Fire QST block declares `qc_unblock 139000 north~` at [plane_fire_one.qst line 3](../../../areas/qst/plane_fire_one.qst#L3) | Active cross-area mobile and room use in source. The QST index still assigns the 254–255 range to `plane_fire_three`, not the active `plane_fire_one` stem; source reference is clear, loading identity is not. |
| Heavens fixed object requirements #25402 and #25403 | [Heavens QST lines 109–130](../../../areas/qst/heavens.qst#L109) | Active item consumers. Nearby prose mentions wearable fire items and Imix remains, then Demogorgon's helmet before the fixed pair; ask the Heavens owner to reconcile stage intent without changing the item IDs. |
| Imix #25440 | [Surfacemini story sidecar](../../../areas/story/surfacemini.story.json#L235) | Typed planar-essence/story reference, not a local quest receipt. |
| Fire #25464 and Smoke #139894 | [Smoke story sidecar](../../../areas/story/smoke.story.json#L10) | The sidecar describes the older portal pocket and the paired Smoke objects; it distinguishes those from later elemental-plane areas. |
| Fire objects #25402 and #25405 | [CHAOS equipment profiles](../../../src/account/chaos_eq_data.h#L203) and [second profile](../../../src/account/chaos_eq_data.h#L449) | Static equipment profiles for the earring and mask, not live inventory. |
| Imix #25440 callback and helper mobiles #25450, #25430, #25410, and #25415 | `imix_shout` assignment ([specs.assign.c](../../../src/specs/specs.assign.c#L1054)) and [Firep helper list](../../../src/specs/specs.firep.c#L129) | Compiled local callback and helper-list references; preserve these mobile VNUMs pending owner intent. |
| Fire object products #25400 and #25401 | [Sylvandawn shop source](../../../areas/shp/sylvandawn.shp#L120) | Unregistered source-only shop product rows for the two potions; not active shop stock. Preserve on existing Sylvandawn candidate row. |
| Imported reset objects #67210, #55199, #359, and #14121 | [Imix reset block](../../../areas/zon/plane_fire_one.zon#L61) | Static external object loads on Imix; they do not make those objects local Plane of Fire prototypes. |

The bounded exact scan found no outside duplicate mobile or object definition
for the Fire prototypes. Compiled range constants `FIRESTART` and `FIREEND`
cover #25401–#25525 ([map.h](../../../src/world/map.h#L58)). Preserve the
current owners and every listed consumer; the static references do not prove
that an item, reset, shop, or route is currently available to a player.

## Title, followed history, and retention

After color stripping and whitespace normalization, “Plane of Fire” is unique
among 442 current `.zon` titles and among 351 active contributors. Commit
`0e925fa992ecb648b260efa159bd016fa88f9136` (December 18, 2025) removed the
older Brass/Fire files and changed the active manifest. The current
`plane_fire_one` WLD, OBJ, ZON, and QST contents match the former
`plane_fire_three` files; six of eight mobile records match, with the two
guardian records edited. The old `plane_fire_one` source described a distinct
105-room #25001–#25105 contributor, which is not the current active content.
This is a followed content transplant and relabel; do not infer a second live
area or restore deleted files. The exact history is recorded in
[DUPLICATE_AND_REPLACED_ZONES.md](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-323-plane-of-fire-title-and-followed-history).

Keep active `plane_fire_one` **DO NOT MOVE**. Preserve its VNUM ownership,
all active WLD boundaries, both Smoke object endpoints, the damaging Ethereal
pool route, the Negative Plane destination, Heavens item contract, Brass
guardian integration, and source-only candidate evidence. No gameplay,
placement, reset, or migration change is proposed.

## Checkpoint 323: builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| FIRE-01 | What player objective, if any, should the local Fire map express? | Preserve its environmental prose and encounters until an accepted action, outcome, and durable receipt are defined. |
| FIRE-02 | Is room #25406's silver air sphere an intentional Plane of Air feature in this map? | Keep its text and sector while the Air and Fire owners decide whether the cross-element detail is intentional. |
| FIRE-03 | Is #25433's chromatic jet the intended arrival point for Negative Plane fissure #26659? | Preserve both records until the owners confirm endpoint, access, and return intent. |
| FIRE-04 | Are rooms #25418, #25450, and #25518 intended as one-way branches with no local inbound WLD edge? | Keep their exits; do not add return edges until each room's intended role is confirmed. |
| FIRE-05 | Is Imix meant to load in #25450, a room with no local WLD inbound? | Preserve the reset and map; qualify any intended encounter access through separate movement or dynamic-travel review. |
| FIRE-06 | Should Imix continue to carry #25403, #25405, and #25402 in those equipment slots? | Preserve the current local equipment reset while the Heavens owner confirms intended item provenance and availability. |
| FIRE-07 | Are the four imported Imix objects #67210, #55199, #359, and #14121 intentional? | Retain the cross-area reset references and coordinate any future VNUM or loot change with each owning area. |
| FIRE-08 | Are guardian prototypes #25400 and #25401 owned here for use by the City of Brass? | Keep the Fire prototype identities and active Brass resets until both area owners agree on ownership or replacement. |
| FIRE-09 | Should #25401's death clear `qc_unblock 139000 north~` in the Brass QST context? | Preserve the current callback and message while the Brass owner verifies the gate's intended state and player sequence. |
| FIRE-10 | Is the single local QST record only a technical Brass hook, or should Fire have its own quest contract? | Keep the existing shared hook and do not count it as a local Q/QA story contract; add a separate objective only after its owner and receipt are defined. |
| FIRE-11 | Why does the Heavens QST require #25402 and #25403 near text that has moved from wearable Fire items and Imix to Demogorgon's helmet? | Preserve the fixed IDs and ask the Heavens owner to reconcile requirement placement with the adjacent stages. |
| FIRE-12 | How are Heavens players intended to acquire the earring and cloak required by VNUM? | Preserve Imix's reset placement and verify an intended acquisition path before changing the contract or loot. |
| FIRE-13 | Should Gate, Nether Gate, and Plane Shift continue to use #25401 as their Fire anchor? | Keep the anchor and helper checks; document landing, entry, and return separately if the spell behavior changes. |
| FIRE-14 | Are Prison #7484's five directions to #25504 intended as five alternatives from one room? | Preserve all five records and review the group as one entrance source before describing separate routes. |
| FIRE-15 | Should Firep #88301 west to #25415 and Fire #25415 west to Firep #88321 remain different endpoints? | Keep the authored links and ask both owners to confirm whether the mismatch is deliberate. |
| FIRE-16 | Is the reciprocal Fire #25455 / Brass #139000 boundary the intended ordinary connection to Brass? | Preserve both same-endpoint records and review their relation to the separate guardian gate hook. |
| FIRE-17 | Is pool #25404's Ethereal destination #12459 and 200-damage behavior intended? | Keep the destination and damage field; review access, the one-hit-point floor, and return as one route decision. |
| FIRE-18 | Should the Smoke portal at #25464 continue to pair with Smoke #139894? | Preserve both type-25 destinations and resets until the Smoke and Fire owners confirm the older pocket's place in current navigation. |
| FIRE-19 | Are Ardgral #30404 and Ethereal #12470 intended to deliver to #25410, and should Astral #19711 remain only a bound prototype? | Preserve the active pool consumers and distinguish Astral's no-reset prototype until its owner confirms placement. |
| FIRE-20 | Is the current content transplant from old `plane_fire_three` to `plane_fire_one` the intended permanent identity, with the 250-range map retired? | Keep the active stem and current content in place; retain the exact history record and require an explicit owner decision before any future move or restoration. |
| FIRE-21 | Should `areas/qst/AREA` still list `plane_fire_three *254-255` after that QST content was renamed to active `plane_fire_one.qst`, which begins at #25401? | Ask the QST index owner to reconcile the historical stem and range with the active file; preserve current content and do not restore a separate QST absent an owner decision. |

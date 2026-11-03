# Varathorn Keep — Checkpoint 113 audit review

Source snapshot: 72ba3deff3bb30bde880018cdebe7c6db675b697. This repository-wide supplement reconciles the active world graph, inactive WLD approaches, bounded typed dependencies, title ownership, tracked history, and attic disposition with the [existing comprehensive Varathorn Keep story dossier](../zone-stories/VARATHORN_KEEP.md).

The active kastle contributor remains **DO NOT MOVE**. The inactive surf and surface2011 map sources remain **RETAIN** pending map ownership and remaining dependency/tooling review. This checkpoint changes documentation only; it makes no native source, placement, database, or gameplay change.

## Active graph and Surface boundaries

The active manifest contains 351 WLD contributors and 253,262 room VNUM records. kastle contributes 100 rooms and 222 authored exits: 220 local and two outbound. All 222 targets resolve in the active world, there are no local parse issues, and the undirected local graph has one component of 100 rooms.

In the local directed graph, treating every authored exit as traversable, entrance #99400 reaches 92 rooms. Ninety-nine rooms have a directed return path to #99400; #99499 is the only room without one. Rooms #99492–#99498 are not reachable from the entrance through authored local exits, though each has a directed path back to the main graph. These are source graph counts, not proof of loaded state, gate availability, runtime arrival, or safe travel. The single duplicate active room VNUM in the full census is unrelated to Kastle: #51800 occurs in hell_three_minauros and hell_two_dis ([first record](../../../areas/wld/hell_three_minauros.wld#L1), [duplicate record](../../../areas/wld/hell_two_dis.wld#L10375)).

Kastle has two reciprocal active Surface boundaries:

| Kastle edge | Surface return | Register evidence |
| --- | --- | --- |
| #99400 south → #578633 ([Kastle target](../../../areas/wld/kastle.wld#L22)) | #578633 north → #99400 ([Surface target](../../../areas/wld/surface.wld#L1650660)) | [Kastle exit row 1185](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1185), [Surface return row 1510](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1510), [pair row 218](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L218) |
| #99439 up → #575843 ([Kastle target](../../../areas/wld/kastle.wld#L1001)) | #575843 down → #99439 ([Surface target](../../../areas/wld/surface.wld#L1592134)) | [Kastle exit row 1186](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1186), [Surface return row 1501](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1501), [pair row 524](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L524) |

There are two active cross-zone records in each direction, all with reverse-direction records. Door state, switches, fall chance, reset availability, dynamic travel, and return safety remain outside this static census; the [story dossier](../zone-stories/VARATHORN_KEEP.md) records their selected source details.

## Inactive approaches and attic dependencies

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits with no parse issues and no inactive source-room VNUM overlap with Kastle's 100 local rooms. It found two source-only approaches in surf.wld:

| Inactive source record | Target | Qualification |
| --- | --- | --- |
| #265063 north → Kastle #99400 ([exit target](../../../areas/wld/surf.wld#L1365258)) | Active entrance | surf is absent from areas/AREA; this does not create current ingress. |
| #266570 down → Kastle #99439 ([exit target](../../../areas/wld/surf.wld#L1396909)) | Active interior | surf is absent from areas/AREA; this does not create current ingress. |

These findings raise the reviewed Surf subset from 137 to 139 and the curated source-only approach total from 1,526 to 1,528. Preserve both records with surf.wld, separately from the two active Surface pairs. Keep surf and surface2011 **RETAIN**; do not merge, reactivate, delete, or relocate either map based on these edges. See the [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97) and [attic plan](ATTIC_MIGRATION_PLAN.md).

## Typed references and outside dependencies

The local Volordles exchange consumes two exact #98914 bloody talons in kastle.qst ([first input](../../../areas/qst/kastle.qst#L37), [second input](../../../areas/qst/kastle.qst#L38)). Their prototype is defined in tezcat.obj ([object #98914](../../../areas/obj/tezcat.obj#L276)); the local sidecar also names #98914 in its preparation step ([item VNUM](../../../areas/story/kastle.story.json#L140)). Supplied talons remain distinct from proof of personal recovery.

Bounded exact-reference scans found these outside uses of Kastle prototypes:

| Outside source | Typed relationship |
| --- | --- |
| raxthan.shp shop #42963 ([first product](../../../areas/shp/raxthan.shp#L19), [last product](../../../areas/shp/raxthan.shp#L25)) | Lists seven local objects: #99412, #99413, #99416, #99425, #99431, #99436, and #99437. Its active reset source also loads ten local objects, including these seven plus #99410, #99438, and #99435 ([reset rows](../../../areas/zon/raxthan.zon#L100), [last local item row](../../../areas/zon/raxthan.zon#L110)). |
| surfacekeeps.shp shop #120005 ([product](../../../areas/shp/surfacekeeps.shp#L90)) | Lists backpack #99415; surfacekeeps.zon also resets #99415 ([reset](../../../areas/zon/surfacekeeps.zon#L62)). |
| valdrak.shp shop #53156 ([products](../../../areas/shp/valdrak.shp#L13)) | Lists boots #99410 and bluejay feather #99445; valdrak.zon resets both ([boots](../../../areas/zon/valdrak.zon#L113), [feather](../../../areas/zon/valdrak.zon#L114)). |
| alatorin.zon ([first](../../../areas/zon/alatorin.zon#L3922), [last](../../../areas/zon/alatorin.zon#L3960)) | Five active M resets reuse local huge centipede mobile #99460 in rooms #83834, #83841, #83845, #83847, and #83850. Its local definition is in [kastle.mob](../../../areas/mob/kastle.mob#L913). |
| tharnadia.qst ([tome reward](../../../areas/qst/tharnadia.qst#L78), [quill reward](../../../areas/qst/tharnadia.qst#L79)) and tharnadia.story.json ([accepted exchange](../../../areas/story/tharnadia.story.json#L617)) | Zaberetornaz accepts magical text #132664 for mysterious tome #132620 and local raven-feather quill #99412. This is outside circulation of the quill, not a fourth Kastle return. |

The selected Raxthan, Surface Keeps, and Valdrak SHP lists are authored product references, not proof that an item is currently in stock or available to buy. The existing dossier qualifies the selected keeper, room, shop, reset, and reward contexts.

Local reset-role references all resolve within Kastle's local prototypes and range; Kastle has no local SHP source. The bounded scan found no external duplicate mobile/object prototype for Kastle's local VNUMs, no other outside QST item reference or exact sidecar VNUM beyond the rows above, and no target into Kastle among 872 active type-25 portal declarations. The direct numeric code references remain the two effective object assignments for #99447 and #99432 and the phantom destination #99465 already analyzed in the dossier ([dagger binding](../../../src/specs/specs.assign.c#L1503), [claymore binding](../../../src/specs/specs.assign.c#L2585), [phantom target](../../../src/classes/innates.c#L2012)); no broader code or runtime circulation claim is made.

## Title, history, and disposition

After color-code and tilde removal, the exact active .zon title “Varathorn Keep” has one owner among 351 active titles: kastle ([title record](../../../areas/zon/kastle.zon#L2)). Lexical neighbors such as Bloodstone Keep, The Keep of Evil, Krethik Keep, Shadamehr Keep, and The Twin Keeps of Devastated Tharnadia have distinct normalized titles and source identities.

Followed [kastle.wld](../../../areas/wld/kastle.wld#L1), [kastle.zon](../../../areas/zon/kastle.zon#L1), [kastle.mob](../../../areas/mob/kastle.mob#L1), [kastle.obj](../../../areas/obj/kastle.obj#L1), and [kastle.qst](../../../areas/qst/kastle.qst#L1) paths remain under the same stem after the 2009 repository-wide areas/ relocation. The WLD history includes 2011–2013 same-path map edits; MOB/OBJ/QST history includes the 2009 overhaul and later same-path edits; ZON/OBJ history includes the 2015 quest-item-flag pass. The tracked kastle.story.json sidecar ([source](../../../areas/story/kastle.story.json#L1)) was added by 7abb2569d on 2026-10-06. The active manifest and followed paths show no alternate-stem rename, explicit predecessor, or replacement claim. This bounded history does not decide undocumented map intent.

Keep active kastle **DO NOT MOVE**. Keep surf and surface2011 **RETAIN** until map purpose, exact typed dependencies, and editor/tooling closure receive owner review. Preserve the two source-only surf approaches separately from the two reciprocal active Surface pairs.

## References

- [Coverage row](ZONE_COVERAGE_REGISTER.csv#L142)
- [World connectivity report](WORLD_CONNECTIVITY.md)
- [Duplicate and replacement history](DUPLICATE_AND_REPLACED_ZONES.md)
- [Attic candidate dependency register](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97)
- [Source-comprehensive Varathorn Keep dossier](../zone-stories/VARATHORN_KEEP.md)

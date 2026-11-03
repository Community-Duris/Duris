# Killing Fields repository-wide audit supplement

This supplement reconciles the Killing Fields active map and boundaries, inactive-map records, typed cross-area references, compiled VNUM bindings, and title/history evidence with the [source-comprehensive Killing Fields dossier](../zone-stories/KILLING_FIELDS.md). The frozen source snapshot is `72ba3deff3bb30bde880018cdebe7c6db675b697`. No area or gameplay source is changed by this audit.

## Active world and entry boundaries

The active [Killing Fields WLD](../../../areas/wld/killing_fields.wld#L1) has 198 rooms (#33300–#33497) and 737 authored exits. All 737 targets resolve: 730 are local and seven leave the area. The local graph is one weak component; raw traversal from #33300 reaches and returns to all 198 rooms under authored local exits, with no flag-8 edge. There are no negative or unresolved exits, local duplicate room IDs, authored fall chances, or currents. The active-WLD scan parsed 351 files; its single duplicate room VNUM elsewhere does not overlap Killing Fields.

Surface room #585374 south enters Killing Fields #33302 and #33302 north returns to Surface ([Killing Fields edge](../../../areas/wld/killing_fields.wld#L65), [Surface edge](../../../areas/wld/surface.wld#L1792227), [pair rows 224 and 525](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L224)). Bloody Plains contributes five reciprocal boundaries: #33612/#33627/#33642/#33657/#33672 west to Killing Fields #33437/#33452/#33467/#33482/#33497, with east returns from the Fields rooms ([pair rows 48 and 223](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L48)). A sixth Bloody Plains-targeting exit, #33422 east to #33597, has no reverse west edge ([WLD edge](../../../areas/wld/killing_fields.wld#L3831)). This leaves six registered active inbounds, seven active outbounds, and six reverse boundary records. Keep the one-way #33422 boundary explicit; static linkage does not decide its intended access policy.

## Inactive map and reset-source records

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits with no parser errors or room-VNUM overlaps with Killing Fields. It found one non-overlapping source-only `surf.wld` exit: room #279135 south to Killing Fields #33302 ([source edge](../../../areas/wld/surf.wld#L1660579)). It is not active ingress. Curated source-only totals rise from 2,962 to 2,963 overall; the reviewed Surf subset rises from 158 to 159.

Separately, unregistered `surf.zon` contains one reset for local mobile #33308 and G resets for objects #33309–#33312, #33314, and #33315 ([reset block](../../../areas/zon/surf.zon#L6); [mobile](../../../areas/mob/killing_fields.mob#L104); [objects](../../../areas/obj/killing_fields.obj#L115)). `surf` is outside the active area manifests, so these are source-only reset declarations, not active merchant or item stock. Preserve them with the Surf candidate's map and dependency evidence.

## Typed quest, shop, sidecar, and portal references

Typed scans found no active external QST item command, SHP product, story-sidecar `item_vnums`, or ZON reset role targeting Killing Fields rooms, mobiles, or objects. The area has one local shop file. All 231 external story sidecars parsed without errors and none has a typed item-VNUM hit into Killing Fields. Across 442 raw object files, the type-25 scan found 872 declarations and no target into the area. No external mobile or object prototype duplicates any of the local nine mobiles or fifteen objects. The unregistered `surf.zon` reset references are recorded separately below.

Raw numeric searches find #33333 in `smokev.qst` and `tower.qst` `R E` commands and in matching sidecar story strings ([SmokeV](../../../areas/qst/smokev.qst#L415), [Tower](../../../areas/qst/tower.qst#L69), [SmokeV sidecar](../../../areas/story/smokev.story.json#L237), [Tower sidecar](../../../areas/story/tower.story.json#L222)). In each case #33333 is the experience award amount, not an item VNUM or a typed room dependency. These six sidecar token hits do not change the zero external `item_vnums` result.

## Compiled references

The command table binds `shift_prime` to `do_shift_prime` ([binding](../../../src/classes/innates.c#L159)). Its Githyanki destination array contains local room #33366 ([array](../../../src/classes/innates.c#L2008)). Githyanki, Pillithid, and Illithid branches can choose a loaded room from that array while shifting from the Astral zone; the function later moves the character to its selected `real_room` destination ([branch and move](../../../src/classes/innates.c#L2070)). This is an exact compiled travel dependency; it is distinct from the WLD boundary count and does not establish the frequency of arriving in this room.

## Title, history, and disposition

After stripping color codes and normalizing whitespace/case, “Killing Fields” occurs once across all 442 `.zon` files, in [killing_fields.zon](../../../areas/zon/killing_fields.zon#L2), and is unique among active titles. Followed same-stem WLD history reaches the 2009 area-tree move and a later 2013 edit; no distinct predecessor or whole-zone replacement was established. The single inactive Surf approach is not replacement evidence.

`killing_fields` remains an un-commented contributor in both [the world manifest](../../../areas/AREA#L161) and [QST manifest](../../../areas/qst/AREA#L149); keep it **DO NOT MOVE**. Preserve the Surface pair, six Bloody Plains boundary leaves and the one-way #33422 exit, the compiled #33366 destination, the inactive Surf approach, and its source-only reset records. See [coverage row 145](ZONE_COVERAGE_REGISTER.csv#L145), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), [Surf coverage row 448](ZONE_COVERAGE_REGISTER.csv#L448), [connectivity](WORLD_CONNECTIVITY.md), [history](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan](ATTIC_MIGRATION_PLAN.md). Static review does not claim actual access, encounter, shop operation, or arrival.

# Phantasmagoric Caverns repository-wide source audit

This supplement reconciles the active map and boundaries, inactive-WLD approaches, typed prototype dependencies, compiled numeric references, normalized title, and bounded same-stem history with the [source-comprehensive dossier](../zone-stories/VALDRAK.md). It uses the pinned native source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`. No area or gameplay source is changed by this audit.

## Active map and UnderDark boundaries

The active [Valdrak WLD](../../../areas/wld/valdrak.wld#L1) contains 100 rooms (#53100–#53199) and 234 authored exits. All resolve: 232 are local and two cross to the UnderDark. The local map is one weak component; directed traversal from root #53100 reaches and returns from all 100 rooms when authored exits are available. There are no negative or unresolved exits, authored falls, or currents. This source graph does not qualify current door state, hazards, actor conditions, or safe return.

Both active boundaries are reciprocal: Valdrak #53100 south ↔ UnderDark #832131 north, and #53100 west ↔ #831730 east ([UnderDark-to-Valdrak pair row 740](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L740); [Valdrak-to-UnderDark pair row 764](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L764)).

## Inactive-map sweep

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits with no parser issues. It found no source-room VNUM overlap with Valdrak and one non-overlapping source-only approach: [ud_map2.wld room #1125389 down to Valdrak #53100](../../../areas/wld/ud_map2.wld#L66333). It is not active ingress. The curated source-only total rises by one to 3,228 overall; the reviewed `ud_map2` subset rises to 15, and Surf remains at 159. Preserve the approach with `ud_map2`; it does not establish a traversable route or migration clearance.

## Typed references and compiled numeric matches

Valdrak is listed in the active [world manifest](../../../areas/AREA#L227) and [QST index](../../../areas/qst/AREA#L208). An active Raxthan shop, #42963, sells Valdrak objects #53101, #53105, and #53111 ([products](../../../areas/shp/raxthan.shp#L16)); active `raxthan.zon` also G-resets those three local objects ([resets](../../../areas/zon/raxthan.zon#L97)). These are external consumers and stock declarations; they do not prove generated availability or an actual purchase. No external QST item command or external story-sidecar `item_vnums` reference targets Valdrak.

Valdrak's own shop #53156 sells nine foreign object prototypes, and local `valdrak.zon` also G-resets them ([shop products](../../../areas/shp/valdrak.shp#L6); [resets](../../../areas/zon/valdrak.zon#L106)). Seven belong to active `smokev` (#20204, #20221, #20222, #20224, #20227, #20240, and #20243); two belong to active `kastle` (#99410 and #99445). Both source keys are registered in the [world manifest](../../../areas/AREA#L104) and [QST index](../../../areas/qst/AREA#L97); the `kastle` entries are at [world manifest line 360](../../../areas/AREA#L360) and [QST index line 318](../../../areas/qst/AREA#L318). This is a typed local outbound dependency on those object prototypes. The external mobile/object census found no duplicate definitions of Valdrak mobile or object prototypes. Across all 442 object files, none of the 872 external type-25 declarations targets a Valdrak room.

All 231 external story sidecars parsed without errors and produced no typed VNUM references or raw numeric-token hits for Valdrak. The direct compiled-number scan did find two real local dependencies:

- [`do_shift_prime`](../../../src/classes/innates.c#L2009) includes room #53122 in `githyanki_room[]`. The selected code can resolve and move a Githyanki, Pillithid, or Illithid there while in the Astral zone and outside combat, subject to the innate's other runtime conditions ([race/route selection](../../../src/classes/innates.c#L2036); [room movement](../../../src/classes/innates.c#L2070)). This is a compiled travel destination, not proof of unconditional player access.
- [`chaos_eq_data.h`](../../../src/account/chaos_eq_data.h#L84) contains 29 class-profile entries `{27, 53113}`. The record fields are `slot` and `vnum`; the CHAOS new-character kit path reads the object, checks use/slot fit, then attaches admitted items to its generated kit ([object read](../../../src/account/nanny.c#L634), [profile loop](../../../src/account/nanny.c#L709)). Object #53113 is defined locally as the backpack ([definition](../../../areas/obj/valdrak.obj#L198)). This is a compiled object-prototype dependency subject to the kit's profile and runtime gates.

## Title, history, and disposition

The normalized title `Phantasmagoric Caverns` occurs once among all 442 `.zon` titles, in [`valdrak.zon`](../../../areas/zon/valdrak.zon#L2). Followed `valdrak.wld` history reaches the 2009 area-tree move and a 2013 edit; followed `valdrak.zon` history also includes the 2015 quest-item-flag overhaul. The reviewed same-stem history does not establish an alternate-stem rename, distinct predecessor, or whole-zone replacement.

Keep active `valdrak` **DO NOT MOVE**. Preserve the #1125389 source-only approach with `ud_map2`, the Raxthan consumers of local objects, the nine active Smokev/Kastle object imports, the compiled room #53122 route, and the compiled CHAOS-kit dependency on object #53113. The existing dossier remains the source-comprehensive narrative review. This supplement is static source research and does not claim gameplay qualification or migration clearance.

The source-mapping queue is now 189/220 complete, with 31 pending. Priority 190, The Bronze Citadel (`bctdl`), is next.

Validation: Checkpoint 149 documentation validation checked 120 changed/added Markdown files; all 4,795 local link targets and 2,190 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. `git diff --check` passed, and `areas/` is unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

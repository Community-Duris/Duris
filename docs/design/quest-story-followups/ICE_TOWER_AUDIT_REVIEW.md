# Ice Tower audit review: active Surface pair, shared epic stone and inactive Surf approach

This repository-wide checkpoint supplements the [comprehensive Ice Tower dossier](../zone-stories/ICE_TOWER.md). It records the active boundary, one inactive-map approach, typed references to the local prototypes and imported reset objects, and bounded title/history evidence. The dossier remains the detailed source for the two item returns, speech/key/PICK/SEARCH access tiers, room intent and conditional reach.

## Active connectivity and directed reach

The active `icetower.wld` contributes 162 rooms and 359 exits. All 359 targets resolve: 358 stay within Ice Tower and one leaves it. The local undirected projection is one weak component containing all 162 rooms. There are no negative or unresolved targets, numeric-range mismatches, fall-chance rooms or current fields.

Ice Tower room #31800 exits south to Surface room #523117; Surface #523117 exits north back to #31800. This is one active reciprocal pair, with one inbound, one outbound and one reverse record. See [Ice Tower outbound row 437](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L437), [Surface inbound row 1367](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1367), and pair rows [201](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L201) and [520](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L520). Under an all-authored-exits-available assumption, directed traversal from #31800 reaches and returns to all 162 rooms. The dossier separately records conditional speech, key, SEARCH and PICK reach tiers; these are not claims about live state, safe travel or guaranteed access.

The all-95 inactive-WLD sweep found one non-overlapping source-only approach: [`surf.wld` room #206576 north to #31800](../../../areas/wld/surf.wld#L138028). `surf.wld` is outside the active loader list, so this does not add active ingress. The reviewed Surf subset rises from 141 to 142, and the curated source-only approach total rises from 1,531 to 1,532. Preserve this record with its source map; keep `surf` and `surface2011` **RETAIN** pending map ownership and remaining dependency/tool closure.

## Typed references and imported reset objects

The local reset file gives Ashivahr #31824 one each of imported objects #359, #55192 and #67258 at cap 1 and chance 100 ([`icetower.zon`](../../../areas/zon/icetower.zon#L304)). These are imports, not prototypes defined by Ice Tower.

| Object | Ice Tower use and active definition | Bounded outside references |
| --- | --- | --- |
| #359, rune-covered stone | Reset by Ashivahr ([reset](../../../areas/zon/icetower.zon#L304)). Defined in [`heavens.obj`](../../../areas/obj/heavens.obj#L3725); the active object assignment binds it to `epic_stone` ([assignment](../../../src/specs/specs.assign.c#L1604), [handler](../../../src/world/epic.c#L1111)). | The typed ZON scan found 58 active reset-role references to object #359 across other active areas. Preserve the shared object and its existing consumers when reviewing Ice Tower; a reset reference alone does not prove an epic-stone award. |
| #55192, memory of the Ice Tower | Reset by Ashivahr ([reset](../../../areas/zon/icetower.zon#L305)); the prototype is defined in `wh.obj` ([prototype](../../../areas/obj/wh.obj#L2488)). | White Hills QST #55228 accepts it ([Q record](../../../areas/qst/wh.qst#L3221), item command at line 3226); the matching [story request](../../../areas/story/wh.story.json#L1050) records the same item. The accepted exchange awards 1,000,000 copper, one gnomish shopkeepers token #55033 and one strange scroll #55362. No outside active ZON object-reset reference or SHP product entry for #55192 was found. |
| #67258, icicle saber | Reset by Ashivahr ([reset](../../../areas/zon/icetower.zon#L306)); the prototype is defined in `unique.obj` ([prototype](../../../areas/obj/unique.obj#L909)). | No outside active reset-role, QST item, SHP product or typed story-sidecar reference was found. Its custom assignment is commented out at [`specs.assign.c:2075`](../../../src/specs/specs.assign.c#L2075), so the custom saber handler is not active. |

The bounded scans found no outside QST item command, SHP product, story `item_vnums`, active ZON reset-role reference or type-25 target into Ice Tower's local 24 objects, 28 mobiles or 162 rooms. No external duplicate mobile/object prototype targets its local ranges. There is no local Ice Tower shop file. The direct C/C++ numeric scan for Ice Tower's local prototype VNUMs found no matches. The #359 binding above is an imported shared-object dependency; the disabled #67258 assignment is not an enabled local behavior.

These are bounded source scans. They do not close indirect runtime use, editor/map-tool references, persisted player identities or live gameplay outcomes. The detailed dossier keeps imported-object outcomes separate from Ice Tower's two native accepted item returns.

## Title, identity and history

After color-code removal and normalization, “Ice Tower” has one exact owner among 351 non-empty active `.zon` titles: `icetower`. The active title scan found no other lexical candidate containing “Ice Tower.” The current title in [`icetower.zon`](../../../areas/zon/icetower.zon#L2) and active [`areas/AREA`](../../../areas/AREA#L155) membership remain tied to the current source stem.

Followed `icetower.wld`, `.zon`, `.mob`, `.obj` and `.qst` histories remain under that stem after the 2009 repository-wide `areas/` relocation. The tracked area edits include older 2013–2016 changes; the story sidecar was added on 2026-10-06 (`0b68e45fd`). No alternate-stem rename, distinct predecessor or explicit replacement claim appears in these followed paths. This does not establish the absence of an untracked predecessor.

Preserve active `icetower` as its own current identity; it remains **DO NOT MOVE**. Preserve the inactive Surf approach and keep `surf` / `surface2011` **RETAIN**. No gameplay or source placement changed in this checkpoint.

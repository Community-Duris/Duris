# Arachdrathos Wilderness repository-wide audit review

This supplement completes the source-backed story, graph, typed-reference,
title, and bounded-history review for active area `arac_wild`. It accompanies
the [Arachdrathos Wilderness source story dossier](../zone-stories/ARACHDRATHOS_WILDERNESS.md).
The compiled travel targets are static source references; they do not qualify
runtime arrival safety or gameplay access. No area or native gameplay source
was changed.

## Active source and story inventory

At source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`, zone 360 is titled
`Arachdrathos Wilderness` and contains 200 rooms (#36000–#36199), 16 mobiles
(#36000–#36015), and 19 objects (#36000–#36018). Active WLD, MOB, OBJ, and ZON
files are present. The stem is listed in `areas/qst/AREA`, but
`areas/qst/arac_wild.qst` is absent; there are zero native Q/QA contracts, no
SHP file, and no story sidecar. The reset file has 87 mobile, 93 equipment,
three give, and two object reset records, with no door-reset records. The two
reset objects are warnings at the glacier opening and the Bloody Ledge, not
switches.

The authored story centers on Ylani's Half-Drow enclave and its stated plan to
retake part of Arachdrathos, then follows a separate mountain route through a
shadow dragon's lair into the Crescent Chasm. The chasm continues to the
neighboring Arac-Web and Llzazan areas. The source supplies hostile encounters
and a city map, but no local QST outcome or persistent story state.

## Active WLD graph and source-only approach

The active parser resolves all 645/645 authored exits: 640 local and five
outbound. All 200 rooms form one weak component. Directed traversal from
#36000 reaches and can return from all 200 rooms using local exits. All WLD
exit flags and keys are zero, and the ZON file has no door-reset commands.

Every active boundary has a reciprocal WLD record:

| Arachdrathos Wilderness | Neighboring active area | Cross-area exit evidence |
| --- | --- | --- |
| #36000 west to #589915 | Surface, #589915 east returns | [row 472](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L472), [row 1551](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1551) |
| #36046 east to #588318 | Surface, #588318 west returns | [row 474](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L474), [row 1547](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1547) |
| #36004 east to #93115 | Dungeon, #93115 west returns | [row 473](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L473), [row 1124](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1124) |
| #36197 east to #36588 | Arac-Web, #36588 west returns | [row 475](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L475), [row 492](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L492) |
| #36199 north to #36200 | Llzazan, #36200 south returns | [row 476](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L476), [row 477](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L477) |

The reciprocal pair summaries are recorded for
[Arac-Web to Arachdrathos Wilderness](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L16) and
[Arachdrathos Wilderness to Arac-Web](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L19),
[Dungeon](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L20),
[Llzazan](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L21), and
[Surface](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L22). These current active exits
also appear in the [area WLD source](../../../areas/wld/arac_wild.wld#L1).

All 95 WLD files outside the active `areas/AREA` list parsed without errors:
527,791 rooms and 2,080,773 exits, with no room-VNUM overlap against this
active area. One source-only approach points into the forest: `surf.wld` room
#287030 east (D1) to #36000
([inactive source room](../../../areas/wld/surf.wld#L1826258)). It is not active
ingress. Curated source-only totals rise to 3,439 overall, 205 for the reviewed
Surf subset, and 15 for `ud_map2`. Preserve the Surf record with candidate row
[97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

## Typed dependencies and compiled travel endpoints

Repository-wide exact typed scans found these inbound references:

| Source | Reference | Classification |
| --- | --- | --- |
| `wander.zon` rows 26–27 | Two M resets use mobile #36011 in reset room #36600. Mobile #36011 is the Arachdrathos chasm demon. The `wander` room describes a randomized mobile-loading zone for Llzazan Ghetto and Arachdrathos. | Active reset dependency; preserve the mobile prototype and its destination context. See [reset rows](../../../areas/zon/wander.zon#L26), [wander room](../../../areas/wld/wander.wld#L1), and [mobile](../../../areas/mob/arac_wild.mob#L168). |
| `newbie_kit_plan.c` | Object #36016 appears in the Drow and Drider basic item arrays. It is the silk-paper Arachdrathos map. | Direct compiled object reference; preserve the prototype and its city map text. See [Drow list](../../../src/account/newbie_kit_plan.c#L306), [Drider list](../../../src/account/newbie_kit_plan.c#L1397), and [object](../../../areas/obj/arac_wild.obj#L238). |
| `innates.c`, `psionics.c`, `specs.ixarkon.c` | Room #36171 is a possible destination in the Githyanki/Illithid prime-shift list, the mind-travel list, and the illithid teleport-veil list. | Three direct compiled room references; preserve the room as a possible travel endpoint. See [`do_shift_prime`](../../../src/classes/innates.c#L2008), [`spell_mind_travel`](../../../src/classes/psionics.c#L614), [illithid veil destinations](../../../src/specs/specs.ixarkon.c#L18), and [Crescent Chasm room](../../../areas/wld/arac_wild.wld#L4550). |

Exact source scans found no external QST item command, SHP product, typed
story `item_vnums` field, or other external reset-role reference. The only
external ZON match is the two-row `wander.zon` mobile import above. A scan of
21,682 active and unregistered object prototypes found no ITEM_SWITCH targeting
this area; its only parse exception was the `end.obj` #9999999 terminator.
No external mobile or object prototype definition duplicates a local VNUM, and
no external type-25 portal targets an Arachdrathos room. All 231 external story
sidecars parsed without errors or numeric-token hits into local prototypes.
The bounded C/C++ numeric scan found only object #36016 and room #36171
references listed above.

## Title and bounded history

After color-code normalization, `Arachdrathos Wilderness` is unique among 442
scanned current `.zon` titles. Related names—`Arachdrathos - Drow City`
(`arac-web`), `Arachdrathos Guilds` (`aracguil`), and `Llzazan Ghetto of
Arachdrathos`—name distinct neighboring areas, not title duplicates.

Followed `arac_wild.wld` and `arac_wild.zon` histories remain on the same stem
through the 2009 `areas/` tree move and later same-stem edits. A 2013 WLD edit
removes old Surface exit records and rewires the retained boundaries to the
current reciprocal pairs at #36000 and #36046; it is a route update, not
evidence of another zone owner.
The 2015 `arac_wild.zon` change only updates the zone-editor version comment.
The bounded title and WLD/ZON history review found no alternate-stem rename,
distinct predecessor, or whole-zone replacement. See
[duplicate and replacement review](DUPLICATE_AND_REPLACED_ZONES.md).

## Migration disposition

Keep `arac_wild` **DO NOT MOVE** as a current un-commented `areas/AREA`
contributor. Preserve its five reciprocal active boundaries, the external
`wander.zon` reset of mobile #36011, the compiled map item #36016, and the
three travel-table references to room #36171. Keep `surf` **RETAIN IN PLACE**
with existing candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97)
and the separate source-only approach to #36000; add no candidate row for
Arachdrathos Wilderness. Static graph and source analysis do not qualify live
climb/fall outcomes or teleport-arrival safety. See coverage row
[10](ZONE_COVERAGE_REGISTER.csv#L10) and the
[source story dossier](../zone-stories/ARACHDRATHOS_WILDERNESS.md).

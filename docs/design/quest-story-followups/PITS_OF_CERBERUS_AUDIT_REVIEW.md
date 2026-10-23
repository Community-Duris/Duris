# Pits of Cerberus: repository-wide connectivity and dependency audit

**Checkpoint 96; priority 136 of the original 220-zone queue.** This
supplement reconciles the [source-comprehensive story dossier](../zone-stories/PITS_OF_CERBERUS.md)
with the active WLD graph, all inactive WLD sources, typed external
references, numeric C/C++ matches, normalized active title, and followed
WLD/ZON history. Authored exits and source-only map declarations do not
establish player availability, safe travel, or played qualification.

## Active WLD graph

The active manifest has 351 WLD contributors. Pits of Cerberus has 156 rooms
and 323 authored exits; all 323 resolve with no parse issue. The local weak
components contain 51, 36, 35, 20, 13, and 1 room. Room #22108 is the isolated
singleton. Eight fixed local ENTER portals connect four room pairs, and the
outside portals and handler-driven routes described in the story dossier are
separate from this authored-exit graph.

Five active WLD exits enter the area: Surface rooms #560129, #561729, and
#562128 reach rooms #22118 or #22005; MIRA room #82667 reaches #22000; and
Spshold room #22649 reaches #22113. The latter two are one-way in the
authored WLD graph and enter load-network rooms. The three Pits exits leave
from #22005 east to #561729, #22005 south to #562128, and #22118 north to
#560129. Each has a reciprocal Surface exit. These exact boundaries are in
the [cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L360), with the
MIRA and Spshold inbounds at [#L992](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L992)
and [#L374](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L374), and the Surface
inbounds beginning at [#L1463](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1463).

The five fall-chance fields are F20 at #22025, F50 at #22046, F55 at #22048,
F65 at #22050, and F50 at #22150
([first](../../../areas/wld/cerebusp.wld#L584),
[second](../../../areas/wld/cerebusp.wld#L986),
[third](../../../areas/wld/cerebusp.wld#L1027),
[fourth](../../../areas/wld/cerebusp.wld#L1069),
[fifth](../../../areas/wld/cerebusp.wld#L2972)). There are no current fields,
negative targets, unresolved targets, out-of-range source rooms, or exits
from range-mismatched rooms. The only duplicate active room VNUM remains the
unrelated #51800 shared by Hell Two and Hell Three.

The all-95 inactive-WLD scan found three non-overlapping source-only
approaches:

| Source-only map declaration | Target | Evidence |
| --- | --- | --- |
| `surf.wld` #218489 south | #22118 | [exit](../../../areas/wld/surf.wld#L387917) |
| `surf.wld` #219089 north | #22005 | [exit](../../../areas/wld/surf.wld#L400493) |
| `underworld2.wld` #427709 east | #22118 | [exit](../../../areas/wld/underworld2.wld#L83417) |

Neither `surf.wld` nor `underworld2.wld` is in `areas/AREA`, and none of
those source rooms overlaps a Pits room VNUM. These are map-source records,
not active ingress. The scan parsed all 2,080,773 exits without issues. The
reviewed Surf subset is now 125, and the curated source-only reference total
is 1,509.

These are authored graph relationships. The portals, reset state, room
obstructions, dynamic routes, ordinary-versus-staff access, travel success,
and safe return remain separate; no live outcome is qualified here.

## Typed references and external producers

Three active external QST command records refer to local objects: Alatorin
accepts banana #22017; Divhome returns #22020 and accepts PureDark #22032
([Alatorin](../../../areas/qst/alatorin.qst#L3352),
[Divhome return](../../../areas/qst/divhome.qst#L80),
[Divhome input](../../../areas/qst/divhome.qst#L417)). The active
`surfacekeeps.shp` shop #120008 lists banana #22017 as stock
([product](../../../areas/shp/surfacekeeps.shp#L201)); its active ZON also
resets that object ([reset](../../../areas/zon/surfacekeeps.zon#L502)). Two
other active ZON records reset local objects: `vehicles.zon` resets cloak
#22063 ([reset](../../../areas/zon/vehicles.zon#L833)) and
`Voluntown.zon` resets crown #22070
([reset](../../../areas/zon/Voluntown.zon#L153)). No source-only QST command,
shop product, or ZON reset matched.

The Alatorin and Divhome sidecars carry typed item references to #22017 and
#22032 ([Alatorin](../../../areas/story/alatorin.story.json#L1486),
[Divhome](../../../areas/story/divhome.story.json#L1597)). All 231 external
story sidecars parse without errors. The deeper scan found 11 numeric-token
matches across those two files; context review maps their completion keys
and step IDs to the same Alatorin/Divhome transactions, including Divhome's
#22020 reward. They add no further local prototype dependency.

Outbound from the local area, `cerebusp.zon` resets four objects with no local
`.obj` definition: stone #358 ([reset](../../../areas/zon/cerebusp.zon#L190)),
memory #55452 ([reset](../../../areas/zon/cerebusp.zon#L191)), Sealot's heart
#40774 ([reset](../../../areas/zon/cerebusp.zon#L261)), and south relic #76713
([reset](../../../areas/zon/cerebusp.zon#L418)). The source dossier maps
their imported-object behavior. Local QST item commands, SHP products, and
typed sidecar `item_vnums` add no references to nonlocal prototypes.

No external mobile or object prototype definition duplicates a local
prototype. Across 863 external type-25 declarations (704 active and 159
source-only), two active portal objects target local room #22053: Divhome
object #40753 and Ravenloft2 object #59085
([Divhome prototype](../../../areas/obj/divhome.obj#L847),
[Ravenloft2 prototype](../../../areas/obj/ravenloft2.obj#L1281)). Both are
already mapped in the story dossier.

The exact-number scan returned 34 C/C++ match lines across seven files.
Context review identifies the `cerberus_load` binding for mobile #22024,
the `master_set` binding for object #22063, and the `revenant_helm` binding
and crown checks for #22070
([assignments](../../../src/specs/specs.assign.c#L283),
[set binding](../../../src/specs/specs.assign.c#L1339),
[crown binding](../../../src/specs/specs.assign.c#L1538),
[crown constant](../../../src/core/defines.h#L1380),
[crown check](../../../src/specs/specs.clfhaven.c#L86)). The old explicit
PureDark callback for #22032 is commented out
([assignment source](../../../src/specs/specs.assign.c#L1332)); its remaining
packed effects are separately described in the dossier. Selected
Winterhaven object-production paths and the legacy/set-equipment tables were
also classified there. Numeric coincidences in tables or constants are not
treated as additional world routes.

## Title, history, and disposition

The color-stripped title in
[`cerebusp.zon`](../../../areas/zon/cerebusp.zon#L2) normalizes uniquely
among the 351 active contributors. Followed WLD/ZON history includes the
2009 area-tree move, 2012–2014 maintenance, the 2015 quest-item flag update,
the 2016 rockworm species change, and 2026 wandering-area and line-ending
updates. It identifies no distinct predecessor or replacement. The
[duplicate/replacement register](DUPLICATE_AND_REPLACED_ZONES.md) has no
named Pits of Cerberus entry; broader semantic comparison remains open.

The current un-commented [`areas/AREA` row](../../../areas/AREA#L111) remains
**DO NOT MOVE**. The source dossier retains the recipes, alternative treasure
trades, four-badge vault key, equipment and service rules, outside arcs, and
thirteen builder/capability follow-ups. Any separate attic review must
preserve the active reciprocal Surface routes, the MIRA/Spshold inbounds,
the two Surf and one Underworld2 source-only approaches, the external QST and
stock/reset records, and the two portal references. None of these source
records by itself establishes player access or a safe relocation.

This checkpoint used repository sources and generated documentation only.
No tests, build, server, gameplay session, database operation, migration, or
operational script was run.

## Checkpoint 787: reciprocal Divine Home portal

Active Cerebusp object #22010 is reset into room #22053 and its ITEM_TELEPORT
destination is Divine Home room #40787 ([object](../../../areas/obj/cerebusp.obj#L142),
[reset](../../../areas/zon/cerebusp.zon#L179)). This pairs at the source level with
Divine Home object #40753, placed in #40787 and targeting #22053. The declarations
establish reciprocal typed destinations, not runtime availability; see the [Divine Home
typed audit](DIVHOME_AUDIT_REVIEW.md#compiled-bindings-and-item-travel).

# Glacier audit review

This supplement records the source-only review of active contributor glacier. It separates authored WLD exits from door resets, item placement, and the nearby inactive-WLD approach. It does not qualify installed state or live gameplay.

## Source inventory

The active area roster lists Glacier at *904-905 under the title “Towering Glacier of Dayedenvale.” The active files are areas/wld/glacier.wld, areas/mob/glacier.mob, areas/obj/glacier.obj, and areas/zon/glacier.zon. They contain 105 rooms (#90401–#90505), ten mobiles (#90401–#90410), and nine objects (#90401–#90409). The ZON uses reset mode 2 and has 81 non-S commands: 64 M, eight G, five E, and four D.

The QST roster also lists Glacier, but areas/qst/glacier.qst is absent. There is no local SHP record, native Q/QA contract, or story sidecar. See [coverage row 108](ZONE_COVERAGE_REGISTER.csv#L108).

## Narrative and resets

The source moves from floating ice mounds and exposed glacier ledges into a fur-lined cavern, an explicit kill pile, and blood-filled chambers. The reset roster places Vishava and her locket outside, animal packs through the glacier, stone cuffsmen as guards, and Princess Falina in the furnished room #90505. Falina’s object #90402 is labeled as a key and “Falina’s Soul”; its G reset immediately follows King Polar #90405 at #90447. The engine gives a G-reset object to the current mobile ([resets](../../../areas/zon/glacier.zon#L34), [key prototype](../../../areas/obj/glacier.obj#L19), [loader](../../../src/world/db.c#L7574)). No local quest source records a player objective, completed rescue, or other outcome. See the [source dossier](../zone-stories/THE_TOWERING_GLACIER_OF_DAYEDENVALE.md).

## Authored graph and reset-aware reach

The WLD contains 257 exits. Of these, 255 are local, one is an outbound exit from #90402 east to active Surface #523536, and one is the negative target #90411 east to -1. The active loader resolves 256/257 targets; there are no positive unresolved targets. The local graph is one weak component of 105 rooms. From each of the six examined roots (#90401, #90452, #90466, #90491, #90500, and #90505), the raw directed local graph reaches and returns from all 105 rooms.

The active Glacier→Surface exit has no matching reverse exit. Its boundary record is [exit-register row 1114](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1114), and the ordered owner pair is [pair-summary row 157](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L157). Active Surface room #523536 goes south to #523936 and west to #37090; it does not return to Glacier. There are no active WLD exits into Glacier.

The all-95 inactive-WLD sweep found no room-number overlap and one source-only approach: unregistered surf.wld room #208853 has a north exit to Glacier #90401 ([source room](../../../areas/wld/surf.wld#L185821), [exit target](../../../areas/wld/surf.wld#L185828)). surf is absent from the active area list, so this is not active ingress. Preserve it with the existing Surf candidate in the [attic dependency register](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

Glacier has four door reset commands. Treating each reset door as unavailable leaves 99/105 rooms reachable and returnable from #90401, #90452, #90466, and #90491; it leaves 5/105 from #90500 and 1/105 from #90505. These figures are a strict source-graph model, not proof of actual room access. The zone has no current fields or fall-chance values.

The concealed block between #90491 and #90500 is set closed and secret on both sides. The slab between #90504 and #90505 uses key #90402, but its two resets differ: the #90504 east side is closed, locked, and secret (state 6), while the #90505 west side is closed and locked (state 2) ([door resets](../../../areas/zon/glacier.zon#L8), [paired slab exits](../../../areas/zon/glacier.zon#L16)). Loaded door state and behavior were not inspected.

## Typed references and source closure

No outside active ZON reset-role reference to a local Glacier mobile or object, active QST item reference to a local object, SHP product reference, or active ITEM_TELEPORT target into Glacier was found. The Glacier key and its two slab exits are local source relationships. No local QST, SHP, native Q/QA contract, or story sidecar is present. Compiled source scans found no direct numeric reference to Glacier’s room range.

## Title, history, and disposition

After color-code removal and normalization, “Towering Glacier of Dayedenvale” is unique among 442 current ZON titles. Followed history stays on glacier: the WLD has only the 2009-08-12 source-tree move; MOB and OBJ history reaches 2014-10-02; ZON history reaches 2015-10-13. No alternate-stem predecessor, retirement, or explicit replacement is established by the checked-in titles and followed histories.

Keep active glacier **DO NOT MOVE**. Preserve the one-way Surface exit, the inactive Surf approach, the key and asymmetric slab resets, and the Princess Falina source records. Add the Surf approach to its existing candidate history; do not add a Glacier candidate row or migration clearance. The reviewed Surf subset is now 222 and curated source-only totals are 3,462 overall, 222 for Surf, and 18 for ud_map2. Underworld2 retains eight approaches.

This review covers checked-in source only. Current reset state, loaded keys and doors, player access, rescue outcome, safe return, and live traversal were not tested. No test, build, server, gameplay session, database operation, migration, or operational script was run for this documentation audit.

## Checkpoint 810 current typed dependency closure

The local zone contains 77 prototype reset operands: 64 mobile operands and
13 object operands (eight `G` and five `E`). All resolve locally; all ten
local mobiles and nine local objects are reset-used. The four `D` commands
are door-state records, not prototype operands. A current repository scan
found no outside active or source-only ZON reset-role or reset-room reference,
QST giver/item reference, SHP keeper/product reference, typed story-sidecar
reference, or duplicate external MOB/OBJ definition for Glacier prototypes.
Glacier has no local QST, SHP, or story sidecar.

The two nonzero local WLD key fields are on the slab exits between #90504 and
#90505; both use local object #90402 ([exit fields](../../../areas/wld/glacier.wld#L2144),
[key](../../../areas/obj/glacier.obj#L19)). No outside WLD key field targets
a Glacier object. Glacier has no local type-25/type-29 object and no external
type-25/type-29 target into a Glacier room. An exact compiled numeric scan
found no direct room, mobile, or object binding in its VNUM sets. The existing
G reset for #90402 still follows the King Polar placement at #90447
([zone resets](../../../areas/zon/glacier.zon#L35)); this records authored
source order, not a live rescue or key-availability outcome.

No connectivity or attic finding changed: the active WLD remains 256/257
loader-resolved, with no active inbound edge and one one-way Surface exit.
The single `surf.wld` approach remains source-only and stays with the existing
Surf candidate; no Glacier candidate row or migration clearance is added.
Live reset/key state, editor/runtime behavior, player access, and persisted
ownership remain unqualified.

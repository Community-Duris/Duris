# Goblin Camp audit review

This supplement records the source-only review of active contributor goblincamp. It maps the local forest-camp story, authored WLD graph, reset placements, typed references, and one inactive-WLD approach. It does not qualify installed state or live gameplay.

## Source inventory

The active area roster lists goblincamp at *114 under the title “The Goblin Camp.” The active files are areas/wld/goblincamp.wld, areas/mob/goblincamp.mob, areas/obj/goblincamp.obj, and areas/zon/goblincamp.zon. They contain 21 rooms (#11400–#11420), nine mobiles (#11400–#11408), and four objects (#11400–#11403). The ZON uses reset mode 2 and has 41 non-S commands: 34 M, five E, one O, and one P.

The QST roster lists Goblin Camp, but areas/qst/goblincamp.qst is absent. There is no local SHP record, native Q/QA contract, or story sidecar. See [coverage row 109](ZONE_COVERAGE_REGISTER.csv#L109).

## Narrative and resets

The room and mobile text describes an Underdark group from Malch’hor Ganl that arrived to collect timber, then entrenched itself in a forest settlement. The camp has a perimeter wall, weapons, a shaman, families, children, and elders. The room text repeatedly describes deforestation, slaughtered wildlife, firewood gathering, trash, animal remains, and a likely poisoned water supply. No local quest source defines a player objective or outcome. See the [source dossier](../zone-stories/GOBLINCAMP.md).

The reset source loads dump object #11400 in room #11403 and places thigh bone #11401 inside it. Goblin-made sword #11403 is equipped on several goblin warriors, while pinewood totem #11402 is equipped on shaman #11407 at #11414 ([resets](../../../areas/zon/goblincamp.zon#L9), [objects](../../../areas/obj/goblincamp.obj#L1)).

## Authored graph and active boundaries

All 65 WLD exits resolve: 64 are local and one leaves Goblin Camp from #11400 south to active Surface #617373. That Surface room has a reciprocal north exit to #11400. Two active exits enter Goblin Camp: the reciprocal Surface arrival at #11400, and a one-way northeast exit from wh room #55402, “Incarne loading room for Evils,” to center room #11412. The local graph is one weak component of 21 rooms. Raw directed reach and return each cover 21/21 from both active arrival rooms, #11400 and #11412. All local exit flags and keys are zero; the ZON has no D resets, and no falls or currents are authored.

The exact active boundary records are [Goblin Camp exit row 209](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L209), [the War Hammers arrival row 757](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L757), and [the Surface arrival row 1617](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1617). Pair summaries are [Goblin Camp→Surface row 158](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L158), [Surface→Goblin Camp row 506](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L506), and [wh→Goblin Camp row 794](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L794). The wh arrival has no reverse boundary.

## Source-only inactive-WLD approach

The all-95 inactive-WLD sweep found no room-number overlap and one non-overlapping approach: unregistered surf.wld room #264548, “A Dark and Overgrown Forest,” has a north exit to active Goblin Camp #11400 ([source room](../../../areas/wld/surf.wld#L1354448), [exit target](../../../areas/wld/surf.wld#L1354455)). surf is absent from areas/AREA, so the route is not active ingress. Preserve it with the existing Surf candidate in the [attic dependency register](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

## Typed references and source closure

No outside active ZON reset-role reference to a local Goblin Camp mobile or object, active QST item reference to a local object, SHP product reference, or active ITEM_TELEPORT target into rooms #11400–#11420 was found. No direct compiled numeric reference to the local room range or story-sidecar item_vnums entry for the local objects was found. The dump, bone, sword, and totem relationships are local reset records.

## Title, history, and disposition

After color-code removal and normalization, “The Goblin Camp” is unique among 442 current ZON titles. Followed history stays on goblincamp: the WLD has only the 2009-08-12 source-tree move; MOB and OBJ history reaches 2015-09-21; ZON history reaches 2015-10-13. No alternate-stem predecessor, retirement, or explicit replacement is established by the checked-in titles and followed histories.

Keep active goblincamp **DO NOT MOVE**. Preserve its reciprocal Surface route, one-way wh arrival, forest-damage narrative, and the inactive Surf approach. Add the source-only approach to the existing Surf candidate; do not add a Goblin Camp candidate row or migration clearance. The reviewed Surf subset is now 223 and curated source-only totals are 3,463 overall, 223 for Surf, and 18 for ud_map2. Underworld2 retains eight approaches.

This review covers checked-in source only. Current reset state, NPC population, object availability, route access, the forest’s future, and live traversal were not tested. No test, build, server, gameplay session, database operation, migration, or operational script was run for this documentation audit.

## Checkpoint 811 current typed dependency closure

The local Goblin Camp zone has 41 prototype operands: 34 mobile operands and
seven object operands (one `O`, one `P`, and five `E`). All resolve to the
nine local mobiles and four local objects; every local prototype is
reset-used. A current repository scan found no outside active or source-only
ZON prototype/room reset role, QST giver/item reference, SHP keeper/product
reference, typed story-sidecar reference, external MOB/OBJ definition, or WLD
key field targeting a Goblin Camp prototype. The area has no local QST, SHP,
or story sidecar.

The object source has no type-25 or type-29 item, and no outside
type-25/type-29 target enters rooms #11400–#11420. An exact compiled numeric
scan found no direct room, mobile, or object binding in the area's VNUM sets.
The dump #11400, nested thigh bone #11401, sword #11403, and totem #11402
remain local reset relationships ([zone resets](../../../areas/zon/goblincamp.zon#L9),
[object prototypes](../../../areas/obj/goblincamp.obj#L1)).

No WLD or attic totals changed: the active graph remains 65/65 resolved, with
the reciprocal Surface pair and the separate one-way War Hammers arrival.
The single Surf approach stays source-only with the existing Surf candidate;
no Goblin Camp candidate row or migration clearance is added. Live population,
access, and persisted owner behavior remain unqualified.

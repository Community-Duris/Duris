# Twin Towers Forest: boundary and typed-dependency supplement

This supplement reconciles the existing [source-comprehensive story dossier](../zone-stories/TWIN_TOWERS_FOREST.md), [milestone decisions](TWIN_TOWERS_FOREST_MILESTONES.md), active WLD census, and direct external references. Findings are source-backed; installed reachability, player outcomes, shop operation, and dynamic resolution are not qualified.

## Active graph and directed boundary records

Active `twin_towers_forest` is listed at [areas/AREA](../../../areas/AREA#L76), title “Twin Towers Forest” in [its zone header](../../../areas/zon/twin_towers_forest.zon#L2). Its WLD has 100 rooms and 259/259 loader-resolving exits, divided into two local weak components (largest 99 rooms); no negative or positive unresolved targets, fall fields, or currents are recorded. The full graph and unresolved-route limits are in [Checkpoint 407 of the connectivity report](WORLD_CONNECTIVITY.md#checkpoint-407-twin-towers-forest-boundaries-and-directed-access).

Four outward records are all reciprocal: Forest #13500 west ↔ Surface #559646 east, plus Forest #13569/#13570/#13571 ↔ Twin Towers #13259/#13200/#13230. Two more active inbound records point to Forest #13595 from Pine Hollow #16162 and Krimman #16486. Those source rooms are labeled quest-mob load rooms; they do not prove public arrival or an ordinary return. The exact records are in [cross-area exit rows 230–232](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L230) and [rows 282 and 294](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L282).

The all-95 inactive-WLD sweep found no overlapping room identity and no source-only approach into this forest. There is no Forest-specific candidate approach to add. Older source-only aggregate subtotals at Checkpoints 407 and 859 are historical; the current cumulative total is 11,993 overall / 415 Surf.

## Typed cross-area references and code bindings

The exact bounded scan of active QST, ZON, SHP, story sidecars, and C/H sources found these typed external relationships to Forest prototypes:

| External source | Forest prototype and source role | Evidence and status |
| --- | --- | --- |
| Area-listed Nax ZON and SHP | Reset row supplies object #13563 to Nax keeper/mobile #37762; the Nax shop's product list also names #13563 | [reset](../../../areas/zon/nax.zon#L380), [shop stock](../../../areas/shp/nax.shp#L288), [product](../../../areas/shp/nax.shp#L298). Static source ownership is established; actual reset and sale are not qualified. |
| Unregistered Nexus56 ZON and SHP | Seven reset rows reference Forest objects #13554, #13556, #13558, #13560, #13562, #13564, and #13566; the source-only shop list for keeper #83031 contains those flowers, with #13562 repeated | [resets](../../../areas/zon/nexus56.zon#L1313), [shop keeper and stock](../../../areas/shp/nexus56.shp#L201). Nexus56 is not listed in active [areas/AREA](../../../areas/AREA); preserve these source-only dependencies with its existing [RETAIN candidate row 79](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79). They do not establish active resets or stock. |
| Active New Cave City sources served by Forest ZON | Forest reset places New Cave City wizard/mobile #15120 in New Cave room #15141, then adds four G resets of New Cave object #15119 to that mobile | [Forest reset lines](../../../areas/zon/twin_towers_forest.zon#L392), [object rows](../../../areas/zon/twin_towers_forest.zon#L393). The New Cave QST/story and compiled teacher use remain separately documented in the [New Cave City audit](NEW_CAVECITY_AUDIT_REVIEW.md#typed-and-compiled-references). Preserve the reset-source ownership; do not copy the wizard into a second placement. |

Local compiled bindings in `specs.assign.c` connect the nine forest animal mobiles and corresponding corpse objects to their procedures, and bind `gardener_block` to twelve forest rooms ([mobile bindings](../../../src/specs/specs.assign.c#L968), [object bindings](../../../src/specs/specs.assign.c#L1877), [room bindings](../../../src/specs/specs.assign.c#L2526)). `specs.twintowers.c` checks the gardener belt #13521 at the waist for guarded movement ([procedure](../../../src/specs/specs.twintowers.c#L167)); `vnum.obj.h` names forest rotting-corpse object #13520 ([constant](../../../src/world/vnum.obj.h#L105)). These are local behavior bindings, not external map routes.

No external QST item command, story `item_vnums`, active external reset-role target, duplicate active mobile/object prototype, or external type-25 target into Forest rooms was found in the bounded scan. The local area has no SHP file. The source dossier remains the authority for all 84 native Q/QA contracts, service outcomes, reset limits, and its 20 explicit builder questions. Keep the separate, owner-approved any-order flower milestone terms and preserve the unshipped status of implementation in [TWIN_TOWERS_FOREST_MILESTONES.md](TWIN_TOWERS_FOREST_MILESTONES.md).

## Identity and migration disposition

Checkpoint 762's normalized title and bounded WLD/ZON history comparison remains complete: this title has one active owner. “The Twin Towers” (`ttowers`) is a separate title, stem, room range, and story map, joined by three reciprocal boundaries; it is adjacent content, not a replacement. No distinct Forest predecessor or whole-zone replacement was found in the followed same-stem history ([identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-762-twin-towers-forest-and-the-twin-towers-identities)).

Keep active `twin_towers_forest` **DO NOT MOVE**. Add no candidate, activate no route, and grant no migration clearance. Retain the New Cave City reset-source relationship and the Nax/Nexus56 item consumers with their current owners. Dynamic shop/reset behavior, installed player access, editor/tooling, and persisted identity remain outside this static closure. See [coverage row 320](ZONE_COVERAGE_REGISTER.csv#L320).

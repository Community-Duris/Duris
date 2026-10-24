# The End of the World

## At a glance

**end** is a terminal source marker, not a playable area in the active world. The final **areas/AREA** entry explains that this zone “adds top_of_world marker.” Its WLD and MOB files contain only the #9999999 terminator, and the OBJ file does not contain a complete prototype at that number. The ZON file supplies the terminal label “The End of the World” and an empty command list; the QST file contains only its file terminator. The source does not describe an in-world room, mobile, object, quest, route, or encounter. See the [area-list entry](../../../areas/AREA#L414), [WLD marker](../../../areas/wld/end.wld#L1), [MOB marker](../../../areas/mob/end.mob#L1), [OBJ marker](../../../areas/obj/end.obj#L1), [zone label](../../../areas/zon/end.zon#L1), and [empty QST](../../../areas/qst/end.qst).

The meaningful area data stored under this stem is a shared shop catalog. Four end.shp records place the global local-drug-dealer mobile (#101) at active rooms in Llzazan, Nax, Shady, and Tharnadia. Those locations are authored by the four owning areas; end itself has no shop room. The record tail is kept after the other shop files to preserve the existing shop catalog order. See the [shop records](../quest-story-followups/END_OF_THE_WORLD_AUDIT_REVIEW.md#the-shared-dealer-catalog).

## Shared dealer locations

| Owning area | Room | Room name | Matching reset and shop source |
| --- | ---: | --- | --- |
| Llzazan | #36329 | Tavern of the Decapitated Dwarves | [reset](../../../areas/zon/llzazan.zon#L320), [shop](../../../areas/shp/end.shp#L1) |
| Nax | #37718 | Brawling Foot Tavern | [reset](../../../areas/zon/nax.zon#L158), [shop](../../../areas/shp/end.shp#L48) |
| Shady Grove | #97685 | Back Room of the Bleeding Heart | [reset](../../../areas/zon/shady.zon#L1112), [shop](../../../areas/shp/end.shp#L95) |
| Tharnadia | #132767 | Sleezy Brothel | [reset](../../../areas/zon/tharnadia.zon#L1355), [shop](../../../areas/shp/end.shp#L142) |

Each catalog record uses keeper #101 and the same ten products, #829–#838. The active Heavens sources define the dealer and all ten stock objects. Their names describe a range of drug ingredients, rolling paper, and pipes. The ZON placements and SHP locations agree in source; current keeper presence, stock, and sales have not been observed in a running world. See the [dealer prototype](../../../areas/mob/heavens.mob#L1228), [stock prototypes](../../../areas/obj/heavens.obj#L7799), and [catalog and loader ordering](../quest-story-followups/END_OF_THE_WORLD_AUDIT_REVIEW.md#the-shared-dealer-catalog).

## Direct area-owner questions and conservative recommendations

| Question | Source evidence | Recommendation |
| --- | --- | --- |
| Should `end` remain a terminal source marker rather than a playable area? | The final `areas/AREA` entry supplies the top-of-world marker; WLD/MOB are terminators, OBJ lacks a complete #9999999 prototype, ZON has no commands, and QST has no contract. See the [marker sources](#at-a-glance). | Keep `end` last in the active roster and preserve its terminator role. Do not invent playable rooms, aliases, routes, or encounters. |
| Should `end.shp` continue to supply the four shared #101 dealer catalogs at rooms owned by Llzazan, Nax, Shady Grove, and Tharnadia? | The four SHP records match active ZON resets at #36329, #37718, #97685, and #132767; `end` itself has no shop room. See the [shared dealer locations](#shared-dealer-locations). | Preserve each owning area's room/reset and the shared catalog entry as a coordinated set. Do not transfer ownership to `end` or claim its existence is a local shop. |
| Should all four shared dealer records remain aligned with keeper #101 and Heavens products #829–#838, in their current catalog order? | Each record declares the same ten products and Heavens provides the active keeper and object prototypes; current keeper presence, stock, and sales have not been observed. See [dealer and stock prototypes](#shared-dealer-locations) and the [catalog audit](../quest-story-followups/END_OF_THE_WORLD_AUDIT_REVIEW.md#the-shared-dealer-catalog). | Keep all four records and active prototype identities aligned, preserving catalog order. Only change the catalog with its owning areas and the Heavens stock source. |
| What evidence should be required before promising the shared dealer is available to players? | Source reset and SHP locations agree, but live keeper presence, current product stock, and successful sales remain unqualified. See the [shared dealer locations](#shared-dealer-locations). | Qualify the actual owning rooms, keeper spawn, stock, shop dispatch, and sale before making availability claims. Source declarations alone are not a running-world result. |

See the [repository-wide audit supplement](../quest-story-followups/END_OF_THE_WORLD_AUDIT_REVIEW.md) for the terminal-marker, history, and exact-reference review.

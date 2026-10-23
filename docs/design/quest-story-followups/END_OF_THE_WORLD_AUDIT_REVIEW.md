# The End of the World repository-wide source audit

Checkpoint 215 maps active area:end as terminal world-build infrastructure and documents the four shared shop locations stored in end.shp. The [source dossier](../zone-stories/THE_END_OF_THE_WORLD.md) summarizes the role and authored locations. This is a static source review; it does not qualify live reset state or shop operation.

## Source identity and terminal markers

The active area roster lists end last and annotates it as the “top_of_world marker.” The QST roster includes the same entry. The ZON header is #9999999, titled “The End of the World,” with reset mode 2 and no reset commands. The WLD and MOB files end at #9999999 followed by $~; the OBJ file's #9999999 block is incomplete and is not a complete object prototype. The QST file is only $~. No playable room, mobile, object, giver, local quest, sidecar, or native Q/QA contract is defined under this stem ([area roster](../../../areas/AREA#L414), [QST roster](../../../areas/qst/AREA#L348), [zone header](../../../areas/zon/end.zon#L1), [WLD](../../../areas/wld/end.wld#L1), [MOB](../../../areas/mob/end.mob#L1), [OBJ](../../../areas/obj/end.obj#L1), [QST](../../../areas/qst/end.qst)).

This distinction corrects the initial inventory, which counted the WLD, MOB, and OBJ sentinels as one local record apiece. The world loader stops when it reads the terminal $~ in place of a room name, and the shared VNUM index drops a final #9999999 sentinel from the active prototype index. The artifact inventory also explicitly skips #9999999 as an index sentinel ([room parser](../../../src/world/db.c#L1262), [prototype-index rule](../../../src/world/db.c#L1164), [artifact inventory rule](../../../scripts/artifact_source_inventory.py#L25)). The coverage row now records zero local rooms, mobiles, and objects and leaves their VNUM bounds blank.

## The shared dealer catalog

end.shp contains four shop records. Each keeper is global mobile #101, “your local drug dealer,” defined in active Heavens. Every record lists the same ten products, #829–#838, which are defined in active heavens.obj. The shop locations align with one active ZON reset in each owning area:

| Owner area | Shop location | Matching source records |
| --- | ---: | --- |
| Llzazan | #36329 | [SHP](../../../areas/shp/end.shp#L1), [M reset](../../../areas/zon/llzazan.zon#L320), [room](../../../areas/wld/llzazan.wld#L3224) |
| Nax | #37718 | [SHP](../../../areas/shp/end.shp#L48), [M reset](../../../areas/zon/nax.zon#L158), [room](../../../areas/wld/nax.wld#L430) |
| Shady Grove | #97685 | [SHP](../../../areas/shp/end.shp#L95), [M reset](../../../areas/zon/shady.zon#L1112), [room](../../../areas/wld/shady.wld#L4212) |
| Tharnadia | #132767 | [SHP](../../../areas/shp/end.shp#L142), [M reset](../../../areas/zon/tharnadia.zon#L1355), [room](../../../areas/wld/tharnadia.wld#L6508) |

The four room descriptions place these records in a tavern, another tavern, the Bleeding Heart back room, and a Tharnadian brothel. These are cross-area dealer placements, not rooms owned by end. The shop stock VNUMs are all active Heavens object prototypes (#829–#838), and keeper #101 is an active Heavens mobile prototype ([keeper](../../../areas/mob/heavens.mob#L1228), [first stock object](../../../areas/obj/heavens.obj#L7799)).

The standard shop assembler follows areas/AREA order, where end is last. The runtime fallback likewise appends end.shp after the other shop sources; its comment says this file is the append-only home for records whose durable numeric IDs must not renumber the catalog ([shop assembler](../../../areas/src/shp/make_shp.c#L36), [runtime fallback and ordering](../../../src/economy/shop.c#L3304)). The file history records the four replicated local-shopkeeper entries in a September 2026 change. Preserve the tail placement and all four records when editing this shared catalog.

## Connectivity, exact references, and candidate disposition

There are zero playable WLD records under end, so there is no local exit graph, active WLD boundary, valid inbound room target, or fall/current field to assess. Exact active and inactive-WLD searches found no approach to #9999999; the number occurs here as a terminal marker and zone header, not a playable room. Consequently, no end candidate or source-only WLD count changes.

The shared shop records are the source-backed dependencies involving this stem. There is no active source target that treats the #9999999 sentinel as a real room, mobile, or object. No duplicate active prototype, external item-teleport or switch target, or alternate local quest contract was found. Presence, reset availability, and successful sales remain outside static source evidence.

## Title, history, and disposition

After color stripping and normalization, “The End of the World” is unique among the 442 current .zon titles. Followed history stays on the end stem: WLD, ZON, and QST histories include a 2013 same-stem update; MOB and OBJ are present at the 2009 area-tree move; end.shp has a September 2026 addition of the four dealer records. No alternate-stem predecessor, retirement, or whole-zone replacement is established.

Keep active end **DO NOT MOVE**. It is the terminal source marker and the final shop-catalog source, despite having no playable room content. Preserve its placement at the end of the active area roster and the four aligned shop/reset locations. The all-95 inactive-WLD scan found no overlap or source-only approach to an End room because no End room exists. No candidate row or curated source-only count changes. See the [coverage row 89](ZONE_COVERAGE_REGISTER.csv#L89), [source-only disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-215-end-terminal-marker-and-shared-shop-catalog), and [source dossier](../zone-stories/THE_END_OF_THE_WORLD.md).

## Builder follow-up

1. Preserve the end area roster position and its WLD/MOB/OBJ terminal records when maintaining the active compiled source set.
2. Keep the four SHP locations matched to their four #101 ZON placements if the dealer or owning areas change.
3. Confirm intended keeper and stock availability in the running game before treating these source declarations as proof of sale access.

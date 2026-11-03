# The Town of Breale typed-dependency audit

Checkpoint 777, reviewed October 12, 2026. This static source review closes the typed-reference inventory for active Breale and its registered attic candidates. It records declared dependencies; it does not establish reset admission, successful quest loading, shop operation, or live item availability. No native gameplay or area source was changed.

## Active inventory and local ownership

Breale is the active full-area contributor at [areas/AREA line 13](../../../areas/AREA#L13), and its QST source is listed at [areas/qst/AREA line 13](../../../areas/qst/AREA#L13). The source set contains 56 rooms (#2600–#2655), 30 mobiles (#2600–#2629), 86 objects (#2600–#2685), the [Breale QST](../../../areas/qst/breale.qst), [story sidecar](../../../areas/story/breale.story.json), and five shops in [breale.shp](../../../areas/shp/breale.shp). No same-type room, mobile, or object VNUM in those catalogs is also defined by another current source file.

Every G/R I item command in the six native Breale exchanges resolves to a local object. The structured giver_vnum, mob_vnum, and item_vnums fields in the story sidecar also resolve to Breale mobiles and objects. No external active story sidecar points at a Breale giver, mobile, room, or object.

## Outbound active dependencies

Breale ZON G commands at [lines 190–192](../../../areas/zon/breale.zon#L190) reference Heavens objects #369, #368, and #363. Heavens is the active object owner for all three. The ZON G at [line 266](../../../areas/zon/breale.zon#L266) instead references object #72609, which has no definition in any current areas/obj prototype file. Breale shop #2620 also lists #72609 as a product at [breale.shp line 42](../../../areas/shp/breale.shp#L42); neither declaration identifies a current prototype owner.

The five Breale shops list 20 product rows: 19 local objects and unresolved #72609. No shop product imports an object owned by another active contributor. The local reset and shop declarations preserve the #72609 reference as an unresolved source edge rather than proving it can load or be sold.

## Inbound active consumers

| Source | Typed Breale references |
| --- | --- |
| SurfaceKeeps | Eight ZON G rows: object #2679 six times at [lines 180, 341, 449, and 567–569](../../../areas/zon/surfacekeeps.zon#L180), plus objects #2658 and #2657 at lines 500–501. Its SHP also lists #2658 and #2657 as products at [lines 199–200](../../../areas/shp/surfacekeeps.shp#L199). |
| White Harbor | ZON G resets object #2656 at [line 694](../../../areas/zon/wh.zon#L694); its SHP lists #2656 at [line 494](../../../areas/shp/wh.shp#L494). |
| Alatorin | QST G I references object #2657 at [line 51](../../../areas/qst/alatorin.qst#L51) and #2658 at [line 2823](../../../areas/qst/alatorin.qst#L2823). |
| Bastine | QST G I references object #2607 at [line 80](../../../areas/qst/bastine.qst#L80). |
| Surface | QST G I references object #2676 at [line 556](../../../areas/qst/surface.qst#L556). |

These are nine active ZON rows, three active SHP product rows, and four active QST item-command rows. The active QST scan found no outside giver header matching a Breale mobile and no qc room-coordinate command targeting a Breale room. No active or registered-candidate ITEM_TELEPORT destination or ITEM_SWITCH value[1] room field targets a Breale room, and Breale defines no local ITEM_TELEPORT or ITEM_SWITCH object.

## Registered candidate-only consumers

Candidate Nexus56 has five source-only ZON G resets into Breale objects #2660, #2662, #2663, #2658, and #2667 at [nexus56.zon lines 1332, 1336–1338, and 1349](../../../areas/zon/nexus56.zon#L1332). Its source-only shops #83032 and #83034 list those same five products at [nexus56.shp lines 257, 261–263, and 345](../../../areas/shp/nexus56.shp#L257). Candidate surface2011 has one QST G I reference to Breale object #2676 at [line 556](../../../areas/qst/surface2011.qst#L556). Neither candidate is in the current full-area or QST manifest, so these records are not active resets or active shop stock. No other registered candidate ZON, QST, SHP, or structured story-sidecar reference targets Breale prototypes.

## Compiled source references

[src/specs/specs.assign.c lines 332–340](../../../src/specs/specs.assign.c#L332) assigns the breale_townsfolk procedure to nine Breale mobiles: #2613, #2614, and #2619–#2625. [salchemist.c line 1347](../../../src/classes/salchemist.c#L1347) and [line 1430](../../../src/classes/salchemist.c#L1430) create Breale object #2670, the ash item. These are direct compiled references to Breale prototypes; other numeric matches in source were unrelated constants or comments.

## Owner questions and disposition

The source closure is complete, with #72609 retained as an explicit unresolved object edge and new area-owner question 12 in the [Breale story dossier](../zone-stories/BREALE.md#direct-area-owner-questions-and-conservative-recommendations). Confirm its intended object prototype and owner, or identify the intended correction to both the ZON and SHP declarations. Do not infer a replacement from the number alone.

Keep active Breale **DO NOT MOVE**. Preserve candidate Nexus56 under candidate row 78 and surface2011 under row 97 with their existing RETAIN plans; no candidate row or active exit changed. Static QST indexing and source declarations do not establish runtime loading, reset outcomes, shop operation, or player access. See [coverage row 38](ZONE_COVERAGE_REGISTER.csv#L39), [Nexus56 candidate row 78](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79), and [surface2011 candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L98).
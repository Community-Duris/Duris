# Bloodstone Keep typed-dependency audit

Checkpoint 778, reviewed October 12, 2026. This static source review closes the typed-reference inventory for active Bloodstone Keep and its registered attic candidates. It records source declarations and compiled bindings; it does not establish successful loading, reset admission, shop operation, quest execution, player access, or durable item availability. No native gameplay or area source was changed.

## Active source inventory and ownership

Bloodstone Keep, source stem bs, is an active full-area contributor and its QST file is indexed. The source set has 918 rooms (#74000–#74917), 255 mobiles (#74000–#74258), 300 objects (#74000–#74299), 21 local QST giver headers, 34 shops, and 222 listed shop products. No same-type room, mobile, or object VNUM in these catalogs is defined by another current source file.

The active bs.qst contains 264 G/R I item-command rows: 38 refer to local objects; 175 G I rows reference 101 White Harbor objects; 50 R I rows reference 48 White Harbor objects; and one G I 0 entry is the zero-valued exchange already excluded in the [Bloodstone story dossier](../zone-stories/BLOODSTONE_KEEP.md#direct-area-owner-questions-and-conservative-recommendations). All 21 giver headers match local Bloodstone mobiles. There is no outside QST giver header for a Bloodstone mobile.

The structured [story sidecar](../../../areas/story/bs.story.json) has 178 giver_vnum fields, 20 mob_vnum fields, and 189 item_vnums roles. Its 15 foreign giver_vnum roles point to White Harbor mobiles #55093 and #55095. Its 165 foreign item_vnums roles cover the same 101 White Harbor object VNUMs as the QST G I rows. The remaining sidecar giver, mobile, and item references resolve locally. No active or registered-candidate sidecar has a typed room, mobile, or object reference into Bloodstone.

## Outbound active reset references

Bloodstone ZON has 91 external object reset rows with these current prototype owners or unresolved source identities:

| Target source | Reset rows | Distinct object VNUMs | VNUMs |
| --- | ---: | ---: | --- |
| Heavens | 28 | 21 | #41, #45, #73, #77, #90, #92, #191, #310, #312, #358, #363, #364, #368, #369, #377, #385, #386, #390, #396, #399, #500 |
| Verz1 | 54 | 36 | #2800, #2805, #3001–#3004, #3008–#3011, #3024, #3025, #3029, #3033, #3034, #3036, #3038, #3040–#3046, #3050, #3054, #3057, #3061, #3070, #3071, #3075, #3076, #3088, #3097, #3234, #3237 |
| White Harbor | 2 | 2 | #55396, #55445 |
| Cityruin | 1 | 1 | #66219 |
| Minizones | 1 | 1 | #5743 |
| Split Shield | 2 | 2 | #10340, #10382 |

The exact source rows are in [bs.zon](../../../areas/zon/bs.zon); the associated active owner files are the current manifest owners. These declarations do not promise reset admission or player acquisition.

Two P resets at [lines 573–574](../../../areas/zon/bs.zon#L573) use object #4814. The only current object definition is the source-only [alterian_wilderness.obj](../../../areas/obj/alterian_wilderness.obj#L183), candidate row 7 / CSV line 8, which remains RETAIN IN PLACE. There is no active object owner for #4814. Keep the active reset and candidate identity distinct until their owners decide whether the object should be available to the active area.

The G reset at [line 989](../../../areas/zon/bs.zon#L989) uses object #3027, for which no current areas/obj prototype file defines a record. Bloodstone shop #74059 also lists #3027 at [bs.shp line 312](../../../areas/shp/bs.shp#L312). Both references remain unresolved source edges and are recorded as separate direct questions in the story dossier.

## Outbound QST and story-sidecar items

All foreign nonzero item commands in the local QST resolve to White Harbor objects. The exact 101-object set used by 175 G I rows and mirrored by the sidecar's 165 item_vnums roles is: #55029, #55077, #55112–#55131, #55136–#55141, #55145, #55154–#55169, #55198–#55203, #55206–#55209, #55270–#55271, #55276, #55279, #55281–#55284, #55302–#55303, #55305, #55308, #55310, #55312–#55313, #55316–#55317, #55319, #55321, #55323–#55325, #55336, #55343–#55360, and #55550–#55554. The 50 R I rows use 48 White Harbor objects: #55053, #55066, #55136–#55140, #55155–#55169, #55204–#55205, #55210–#55211, #55277, #55280, #55301, #55304, #55306–#55307, #55309, #55311, #55315, #55318, #55320, #55322, #55343–#55351, and #55361. No foreign item VNUM has an unresolved or multi-owner active resolution.

## Outbound shop products

The 34 Bloodstone shops list 222 product rows, including 65 external or unresolved rows:

| Product source | Rows | Distinct VNUMs | Source VNUMs |
| --- | ---: | ---: | --- |
| Verz1 | 44 | 34 | #2800, #2805, #3001–#3004, #3008–#3011, #3024, #3025, #3029, #3033, #3034, #3036, #3038, #3040–#3046, #3050, #3054, #3057, #3061, #3070, #3071, #3075, #3076, #3234, #3237 |
| Heavens | 16 | 16 | #41, #73, #77, #191, #310, #312, #363, #364, #368, #369, #377, #386, #390, #396, #399, #500 |
| White Harbor | 1 | 1 | #55396 |
| Minizones | 1 | 1 | #5743 |
| Split Shield | 2 | 2 | #10340, #10382 |
| Unresolved | 1 | 1 | #3027 |

The remaining 157 product rows name local Bloodstone objects. SHP rows identify listed products; they do not prove that a shop is loaded or operating.

## Inbound active consumers

| Source | Typed references into Bloodstone |
| --- | --- |
| Alatorin | 12 ZON G resets of ten distinct Bloodstone objects (#74016, #74021, #74085, #74132, #74181, #74209, #74216, #74274, #74281, #74282); SHP shops #83137 and #83149 list #74021, #74132, and #74181. |
| SurfaceKeeps | Four ZON G resets of #74209–#74212; SHP shop #120010 lists the same four products. |
| Tharnadia | One ZON G reset and shop #132617 product #74203. |
| White Harbor | One ZON G reset and shop #55098 product #74210; QST line 1281 has G I #74047. |
| Verspin | QST line 348 has G I #74298. |

The active ZON consumers are [alatorin.zon](../../../areas/zon/alatorin.zon#L2106), [surfacekeeps.zon](../../../areas/zon/surfacekeeps.zon#L161), [tharnadia.zon](../../../areas/zon/tharnadia.zon#L641), and [wh.zon](../../../areas/zon/wh.zon#L802). The active SHP product rows are in the corresponding owner files; there are nine rows across five shops. The QST item references are in [verspin.qst](../../../areas/qst/verspin.qst#L348) and [wh.qst](../../../areas/qst/wh.qst#L1281). The scans found no active external QST giver, qc room-coordinate command, or typed story-sidecar reference into a Bloodstone prototype.

## Registered candidate-only dependencies

Candidate Nexus56 has a ZON G reset of Bloodstone object #74256 at [line 1365](../../../areas/zon/nexus56.zon#L1365) and candidate shop #83037 lists #74256 at [nexus56.shp line 453](../../../areas/shp/nexus56.shp#L453). Candidate Tharnadia1 shop #6117 lists Bloodstone object #74203 at [line 626](../../../areas/shp/tharnadia1.shp#L626). Neither source is active. No registered candidate QST or story sidecar points into Bloodstone.

The #4814 object above is an outgoing active reset dependency on candidate Alterian Wilderness. Preserve that object-only source under candidate row 7 / CSV line 8; it does not establish that the candidate is active or should be moved. Candidate Nexus56 remains RETAIN under row 78, and Tharnadia1 remains RETAIN IN PLACE under row 102.

## Portals and compiled references

Bloodstone defines six type-25 portal objects; their target rooms are all local: objects #74043 and #74044 target #74122; #74045 targets #74009; #74070 targets #74602; #74071 targets #74601; and #74188 targets #74869. No active or registered-candidate ITEM_TELEPORT destination or ITEM_SWITCH value[1] room field targets a Bloodstone room.

Compiled references include 48 mobile-procedure assignments to 45 distinct Bloodstone mobiles in [specs.assign.c](../../../src/specs/specs.assign.c#L789), and room callbacks for #74916 and #74271 in [lines 2319 and 2422](../../../src/specs/specs.assign.c#L2319). The assignment at #74009 is overwritten from bs_sinister to bs_menacing and repeated; this is already called out in the story dossier and is not changed here. The [Bloodstone special procedures](../../../src/specs/specs.bloodstone.c#L43) reference 71 local helper mobiles. Other direct typed uses include the chaos support kit's object #74021 at [chaos_eq_data.h line 580](../../../src/account/chaos_eq_data.h#L580), the staff pet command's mobile #74132 at [staff_newbie_aid.c line 104](../../../src/cmd/staff_newbie_aid.c#L104), and the Bloodstone guild-location table's room #74124 at [constant.c lines 1941–1944](../../../src/core/constant.c#L1941).

## Owner questions and disposition

Two direct area-owner questions were added to the [Bloodstone story dossier](../zone-stories/BLOODSTONE_KEEP.md#direct-area-owner-questions-and-conservative-recommendations): whether active resets should continue to rely on source-only object #4814, and which object/owner or correction was intended for #3027. Keep both references unchanged until the area owner resolves them.

Keep active Bloodstone **DO NOT MOVE**. The registered candidates retain their existing plans: Alterian Wilderness **RETAIN IN PLACE**, Nexus56 **RETAIN**, and Tharnadia1 **RETAIN IN PLACE**. No candidate row, active exit, native quest, or gameplay behavior changed. See [coverage row 40](ZONE_COVERAGE_REGISTER.csv#L41), [Alterian Wilderness candidate row 7](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L8), [Nexus56 candidate row 78](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79), and [Tharnadia1 candidate row 102](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L103).
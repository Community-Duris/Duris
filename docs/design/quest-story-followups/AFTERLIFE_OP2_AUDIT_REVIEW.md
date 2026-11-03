# Afterlife GC OP2 audit review

Checkpoint 443 is a static review of the unregistered afterlife_op2 source component, including its three local WLD components, two unlisted Duris3 exits, active Torg room-VNUM collision, editor map, reset references, and type-25 portal targets. It does not inspect live loading, editor databases, persisted world state, or runtime traversal. The [source dossier](../zone-stories/AFTERLIFE_GC_OP2.md) summarizes the authored settings; the [builder review](../zone-stories/AFTERLIFE_GC_OP2_BUILDER_QUESTIONS.md) records open owner decisions.

## Inventory and source membership

| Source | Current evidence |
| --- | --- |
| [WLD](../../../areas/wld/afterlife_op2.wld#L1) | Nine rooms, #29120–#29128, with fourteen direction records. |
| [MOB](../../../areas/mob/afterlife_op2.mob) | Tracked zero-byte file; no local mobile prototypes. |
| [OBJ](../../../areas/obj/afterlife_op2.obj#L1) | Seven type-25 teleport objects, #29120–#29126. |
| [ZON](../../../areas/zon/afterlife_op2.zon#L1) | Header #1, title “Afterlife GC OP,” reset mode 0; nine object resets and twelve mobile resets. |
| [Editor map](../../../areas/map/afterlife_op2.json) | Positions for all nine room VNUMs, zoneId afterlife_op2, lastModified 2025-12-02. |
| QST / SHP | No afterlife_op2.qst or afterlife_op2.shp file; not listed in areas/qst/AREA. |

The component is absent from full areas/AREA, areas/qst/AREA, and areas/RANDOM_AREA. The full area list contains active torg *289-291 ([index](../../../areas/AREA#L140)); it does not list afterlife_op2 or Duris3. There is no earlier dossier or row-specific audit for OP2. The source snapshot contains no exact current QST, shop, or custom C/C++ reference to its nine room VNUMs. Those bounded scans do not cover editor databases, saved state, plugins, or live runtime state. See [coverage row 356](ZONE_COVERAGE_REGISTER.csv#L356) and [candidate row 5](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L5).

## Three-map story and source graph

The WLD describes three separate settings: an icy cavern beneath the tundra, a foggy ring of oak trees and stone path, and a salt circle opening into the Deep Beyond. The cavern prose mentions four stoic frost golems. The third setting describes skeletons, other undead, and foul flying creatures around the salt circle. The source text establishes this imagery, but its reset numbers do not establish which inhabitants would load.

| Source room | Authored title | Direction records |
| --- | --- | --- |
| #29120 | A Massive Cavern of Rock and Ice | D0 → #29121; D4 → #172709 |
| #29121 | A Slippery Walkway Through the Tunnel | D2 → #29120; D3 → #29122 |
| #29122 | A Massive Portal Encrusted in Frost | D1 → #29121 |
| #29123 | A Ring of Trees Enshrouded in Fog | D0 → #29124; D4 → #163943 |
| #29124 | A Stone Path Through the Woods | D2 → #29123; D3 → #29125 |
| #29125 | A Stone Portal Amist the Trees | D1 → #29124 |
| #29126 | A Large Circle of Salt | D0 → #29127 |
| #29127 | Passage Into the Deep | D2 → #29126; D3 → #29128 |
| #29128 | Within the Deep Beyond | D1 → #29127 |

The fourteen records comprise twelve local directions and two outward D4 directions. The local directions form three weak components of three rooms each. Directed traversal from #29120, #29123, or #29126 reaches all three rooms in that component. No WLD exit connects the three components. The editor JSON positions the cavern around y=0 and both other groups at y=400; the forest and salt groups appear side by side on that editor row despite having no WLD link.

The D4 from #29120 targets #172709, “The Ocean,” in Duris3.wld; the D4 from #29123 targets #163943, “Rolling Hills,” in that same unlisted source. See the [Ocean target](../../../areas/wld/Duris3.wld#L1526890) and [Rolling Hills target](../../../areas/wld/Duris3.wld#L1342804). Neither target has a direct return exit to its OP2 source room. Duris3 is not in the current full area list, so these source edges do not establish active ingress, loaded destinations, or safe return.

## Active Torg room ownership

Every OP2 room VNUM #29120–#29128 is also defined by active torg.wld with a different authored title or room purpose. This is a nine-room numeric ownership collision, not evidence that the active Torg map and OP2 form one map.

| VNUM | OP2 source title | Active Torg title |
| --- | --- | --- |
| #29120 | A Massive Cavern of Rock and Ice | The Colloseum Path |
| #29121 | A Slippery Walkway Through the Tunnel | The Colloseum Path |
| #29122 | A Massive Portal Encrusted in Frost | The Colloseum Path |
| #29123 | A Ring of Trees Enshrouded in Fog | LOAD ROOM FOR TRANUG |
| #29124 | A Stone Path Through the Woods | TRANUG LOAD ROOM |
| #29125 | A Stone Portal Amist the Trees | TRANUG LOAD ROOM |
| #29126 | A Large Circle of Salt | TRANUG LOAD ROOM |
| #29127 | Passage Into the Deep | TRANUG LOAD ROOM |
| #29128 | Within the Deep Beyond | TRANUG LOAD ROOM |

The active Torg WLD has fourteen direction records targeting #29120–#29128, including #29007 D0 → #29120 and internal exits among Torg's same-number rooms. No other active WLD source targets this range. These paths resolve to active Torg room records; they are not OP2-specific ingress. The candidate register therefore retains zero candidate inbound active exits and zero distinct unresolved active exit records. Active Torg's own routes and the unregistered OP2 source graph must remain separate in the coverage record.

## Identity and source history

The normalized ZON title “Afterlife GC OP” is shared by exactly two of 442 current ZON titles: afterlife_op and afterlife_op2. Both have zone header #1 but different room ranges and content: OP uses #29110–#29112, and OP2 uses #29120–#29128. Keep their source identities separate unless an owner establishes a relationship.

Followed OP2 WLD, OBJ, and ZON history remains on the afterlife_op2 stem through the 2009 area-tree move and later same-stem edits. The empty MOB has earlier same-stem history. The editor map is a separate 2025 asset. No alternate-stem rename or explicit whole-zone replacement is established. The map's saved editor alias or external editor records remain uninspected.

## Typed resets and cross-candidate dependencies

The OP2 ZON has nine object reset commands:

- Six imported Player Castles object resets: #11053 in #29120, #29123, and #29126; #11103 in #29121, #29124, and #29127. These prototypes are not declared in OP2's OBJ file.
- Three OP2-local type-25 objects: #29122 in #29122, #29123 in #29125, and #29125 in #29128.

The same ZON has twelve mobile reset commands: VNUMs #29101–#29104 in each of #29120, #29123, and #29126. The OP2 MOB is empty. Matching IDs occur in afterlife_gh.mob; the same numeric IDs also occur in separate candidate sources scrubs_hall, solstice_gh, void_gh, and wlust_gh (the particular files differ by ID). No active Torg mobile prototype owns these IDs. The reset records alone do not identify the intended prototype owner or show that the cavern prose's frost golems appear.

In the other direction, [afterlife_gh.zon](../../../areas/zon/afterlife_gh.zon#L14) imports OP2 objects #29120, #29124, and #29126 into Guildhall room #29109. No current ZON placement for OP2 object #29121 was found. These are source-only candidate dependencies, not active resets.

## Portal target values

All seven OP2 objects are type 25 (ITEM_TELEPORT) with value[1]=7 (CMD_ENTER). The generic travel path reads value[0] and passes it to real_room ([type](../../../src/core/defines.h#L140), [command](../../../src/cmd/interp.h#L42), [target handling](../../../src/magic/spell_travel.c#L991)). The stored target is a numeric room VNUM; it does not prove that the room named in the object's description is the resolved destination.

| Object | Description destination | Stored value[0] | Current source collision |
| --- | --- | --- | --- |
| #29120 | Tundra | #29122 | OP2 #29122 and active Torg #29122 |
| #29121 | Goodie Continent | #29120 | OP2 #29120 and active Torg #29120 |
| #29122 | Khomani-Khan | #29109 | Active Torg's Small Barracks in the City and Guildhall's Room of Exercises |
| #29123 | Khomani-Khan | #29109 | Active Torg's Small Barracks in the City and Guildhall's Room of Exercises |
| #29124 | Evermeet | #29125 | OP2 #29125 and active Torg #29125 |
| #29125 | Khomani-Khan | #29109 | Active Torg's Small Barracks in the City and Guildhall's Room of Exercises |
| #29126 | Undead Cont. | #29128 | OP2 #29128 and active Torg #29128 |

The portal text and stored values preserve competing destination evidence. No destination is qualified as playable or safe, and the static scan does not establish which same-number owner is loaded. The two D4 WLD exits are separate authored edges and should not be counted as portal-object routes.

## Disposition

Keep candidate row 5 afterlife_op2 **RETAIN IN PLACE**. Preserve the three disconnected WLD maps, the tracked empty MOB, all seven type-25 objects and stored values, the six Player Castles resets, the twelve mobile reset commands, the three local portal resets, and the editor JSON. Preserve its distinction from same-title afterlife_op and the active Torg room owner. Resolve room ownership, both Duris3 endpoints and their return routes, all portal destinations, reset-mobile ownership, and editor/persisted/runtime closure before any separate move or merge decision. See the [builder questions](../zone-stories/AFTERLIFE_GC_OP2_BUILDER_QUESTIONS.md), [identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-443-afterlife-gc-op2-title-history-and-room-ownership), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-443-afterlife-op2-three-components-and-source-only-boundaries), and [retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-443-afterlife-op2-retains-its-current-source).

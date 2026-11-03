# Guildhalls — repository-wide audit supplement

This supplement reconciles active contributor guildhalls across the checked-in area sources, world data, quest and shop lists, source history, code-level room construction, and typed references. The [source dossier](../zone-stories/GUILDHALLS.md) reviews its environment and builder questions.

## Source inventory and quest boundary

Both areas/AREA and areas/qst/AREA list guildhalls. The active files are areas/wld/guildhalls.wld, areas/mob/guildhalls.mob, areas/obj/guildhalls.obj, and areas/zon/guildhalls.zon. They contain 1,000 room prototypes (#48000–#48999), three mobiles (#48000–#48002), and 49 objects (#48000–#48018 and #48100–#48129). There is no local QST, SHP, story sidecar, or native Q/QA contract. The area is not a random-area template.

The 15 named room prototypes are #48000–#48014; the other 985 have the blank name Unnamed and no long description. Only four WLD rooms have a long description: the Forge, Weaving Hall, Jeweller’s Workshop, and Guild Store. The three mobiles are the guardian golems. The 49 objects comprise 19 hall/service/workshop objects and 30 board prototypes.

The zone header is #480, title Guildhalls, reset mode 0. It has two authored O resets: object #48002 in room #48001 and object #48001 in room #48004. There are no mob or shop resets in the local ZON. The source describes a configurable realm facility, not a local quest result. A command to create or extend a guildhall and the save/load implementation are native gameplay code; no database or flat-file instance data was read.

## Static connectivity and runtime reachability

The static WLD has zero authored exits, so its 1,000 room prototypes form 1,000 singleton weak components. All 1,000 prototypes are in the declared #48000-#48999 range. A scan of the other 445 WLD files (350 other active WLD files and 95 inactive WLD files) found no room-number overlap and no active or inactive exit target into #48000-#48999. [Coverage row 115](ZONE_COVERAGE_REGISTER.csv#L115) retains the source-exit count and component census.

Runtime connectivity is stored separately. Guildhall::initialize loads saved hall and room records before kingdom initialization. GuildhallRoom::init applies guild-room restrictions, recreates saved room exits, and derives the room name from its template when no custom name is stored. EntranceRoom::init connects a hall’s entrance to its saved outside_vnum. Constructing a main hall makes entrance, heartstone, portal, and inn rooms; later construction adds linked rooms. These are code-created connections across the shared WLD room pool, not authored WLD exits ([boot order](../../../src/net/comm.c#L989), [room initializer](../../../src/guild/guildhall_rooms.c#L103), [entrance](../../../src/guild/guildhall_rooms.c#L174), [main-hall creation](../../../src/guild/guildhall_cmds.c#L1201), [additional rooms](../../../src/guild/guildhall_cmds.c#L1268)).

The header classifies #48020–#48999 as the runtime guildhall range. The allocator in next_guildhall_room_vnum loops from #48020 while the value is less than #48999, which yields #48020–#48998; IN_GH_ZONE also accepts #48999. The source therefore leaves a boundary question about the last slot. It also leaves #48015–#48019 outside the allocation loop. This is recorded for owner review and was not changed.

The entrance’s actual outside room, stored layouts, and number of instantiated halls depend on saved records that were not inspected. TownPortalRoom code sends Good halls to one of the Tharnadia, Kimordril, or Woodseer inns; Evil halls select Shady Grove for an Underdark map position and Khildarak otherwise. Window objects point at the saved outside_vnum. By contrast, PortalRoom::init does not load a portal and still contains a TODO for outpost portals. A separate BUILDING_PORTAL object path is present in buildings and travel code, but it does not establish that each saved hall currently has such a route. Static reachability is therefore zero; saved-instance reachability remains unqualified.

## Typed dependency closure

The source-exact scan found one active external reset consumer: areas/zon/brass.zon line 458 O-resets Guildhalls object #48012, the magical tome, into active Brass room #139063, its Study Room ([consumer](../../../areas/zon/brass.zon#L458), [room](../../../areas/wld/brass.wld#L1233), [object](../../../areas/obj/guildhalls.obj#L97)). This is an active inbound object dependency even though it creates no WLD edge.

The existing brass-old-1 attic candidate has an additional inactive importer: brass-old-1.zon line 458 O-resets object #48012 into its historical Study Room #139163. In current brass.wld, #139163 is Magma Lake, while the active Study Room is #139063. The old reset is not active stock. Its source-only dependency is added to the [brass-old-1 candidate record](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17); keep that candidate retained pending its existing full closure work.

Across active and inactive ZON command roles, no other room, mobile, or object VNUM in the guildhalls sets was referenced. The exact closure found no external QST item command, SHP product, or story-sidecar VNUM. A scan of 868 external type-25 object declarations found no target into these 1,000 room VNUMs. The 445 other WLD files have no overlapping room prototype VNUMs, and no external mobile or object definitions duplicate this contributor’s mobile or object VNUMs.

The compiled feature has broader direct dependencies that are not represented as WLD edges:

- src/guild/guildhall.h maps room templates #48000–#48014, objects #48000–#48018, mobile golems #48000–#48002, and board objects based at #48100. src/guild/guildhall.c binds the door, window, heartstone, cargo-board, and golem procedures ([constants](../../../src/guild/guildhall.h#L45), [bindings](../../../src/guild/guildhall.c#L27)).
- src/guild/guildhall_rooms.c constructs typed room services from those templates and objects: entrance door, inn board, heartstone, window, healing fountain, bank, town portal, library tome, and workshop props ([room types](../../../src/guild/guildhall_db.c#L133), [services](../../../src/guild/guildhall_rooms.c#L174)).
- src/cmd/boards.c maps board VNUMs #48101–#48129 to guild board files. The guild-store implementation uses kingdom-craft object #48018, and the workshop types use the four newer room templates and props ([board bindings](../../../src/cmd/boards.c#L58), [craft blank](../../../src/world/vnum.obj.h#L88), [store and craft](../../../src/kingdom/kingdom_craft.c#L615)).
- Saved entrance positions, map and inn anchors, and travel restrictions are read by the guildhall, kingdom, building, and travel code. The source review confirms these code dependencies but does not inspect live persisted records or run the gameplay feature ([town-portal initializer](../../../src/guild/guildhall_rooms.c#L370), [guild portal checks](../../../src/magic/spell_travel.c#L1015)).

## Title, history, and replacement boundary

The normalized title Guildhalls is unique among all 442 current .zon titles. Followed source history identifies castle_rooms as the direct same-stem predecessor: commit 58e017ead on 2010-02-02 explicitly renames that zone to guildhalls. The current source continues to receive feature updates, including a 2026 workshop and guild-store update.

Active player_castles retains the similar title The Adventurers Guildhalls. In the 2010 history, commit cd88c875c says it added a few objects from player_castles into guildhalls. Current source still lists player_castles separately, while older house and guildhall handlers in code are marked deprecated. This establishes related feature history and item-level copying, not a full-zone replacement. Keep the two active contributors distinct pending owner reconciliation.

## Attic disposition

Keep active guildhalls DO NOT MOVE. Its empty-looking room prototypes are the runtime pool for saved guild halls. The code depends on their VNUMs for room allocation, stored exit restoration, template-name substitution, service objects, custom procedure bindings, guild boards, kingdom workshops, and map/outpost travel. The active Brass reset also imports object #48012. Moving the zone or its prototypes out of the loader would strand the shared feature and break an active external reset.

No guildhalls attic candidate is proposed. If an owner ever asks to retire this contributor, the safe plan requires an owned migration of all saved hall and room records, including room VNUMs, template types, names, exits, outside anchors, and services; compatible replacement prototypes and bindings; and explicit updates or retained aliases for the Brass importer and every code VNUM dependency. Do that as a separately reviewed feature migration, not as a file-only attic move. For the already-unregistered brass-old-1 source, keep its object #48012 importer recorded and RETAIN the candidate; this does not add a WLD approach or active stock.

# Celestia repository-wide audit supplement

This checkpoint reviews active `celestia` against the source snapshot recorded in [the coverage register](ZONE_COVERAGE_REGISTER.csv). It adds documentation only. No live server, player state, or database was inspected.

## Inventory and story boundary

The active contributor is `celestia *455-457` at [areas/AREA line 214](../../../areas/AREA#L214); the independent quest index also lists it at [areas/qst/AREA line 196](../../../areas/qst/AREA#L196). The `.zon` title is “Celestial Plane.” Local source contains 230 WLD rooms (#45500–#45729), 88 mobile prototypes, 46 object prototypes, 896 WLD exits, and 36 door-reset commands. There is no same-stem QST or SHP file, no local Q/QA contract, and no story sidecar. The indexed QST membership does not represent a local contract.

The [source-story dossier](../zone-stories/CELESTIA.md) records the refuge-to-wargrounds lore, Life Stream and Evermore scenes, the Titan Palace, Malevolence, the Avernus gate, repeated room descriptions, and the apparently imported Tharnadia, Bahamut, Macom, and Shady Grove vignettes. Its questions require owner decisions; this supplement records them without proposing content edits.

## Static WLD graph and authored access

All 896 Celestia exit targets resolve in the active WLD source set: 895 are local and one is outbound. The local undirected graph has three weak components of 159, 70, and one room. The 70-room group contains the Palace approach and interior (#45639–#45717, with gaps); the singleton is summon/load room #45719. The remaining 159-room group contains the wargrounds and inbound anchor #45718.

| Direction | Source | Destination | Reverse WLD record |
| --- | --- | --- | --- |
| Active inbound | `realm.wld` #14208 D6/northwest ([room](../../../areas/wld/realm.wld#L4633), [exit](../../../areas/wld/realm.wld#L4667), [target](../../../areas/wld/realm.wld#L4671)) | Celestia #45718 | None |
| Active inbound | `wh.wld` #55634 D0/north ([room](../../../areas/wld/wh.wld#L13082), [exit](../../../areas/wld/wh.wld#L13086), [target](../../../areas/wld/wh.wld#L13089)) | Celestia #45718 | None |
| Active outbound | Celestia #45637 D0/north ([room](../../../areas/wld/celestia.wld#L4346), [exit](../../../areas/wld/celestia.wld#L4355)) | Hell One/Avernus #51207 ([room](../../../areas/wld/hell_one_avernus.wld#L5565)) | None |

Raw directed reach, treating every authored local exit as traversable, is 149/230 from #45500 and 149/230 from #45718; #45639 reaches all 70 rooms in the Palace component. Those counts ignore door state. Applying the unconditional Celestia ZON door resets leaves open, non-secret, non-blocked directed reach at 92/230 from #45500, 1/230 from #45718, and 2/230 from #45639. The room-exit checks exclude closed, secret, and blocked exits ([movement checks](../../../src/core/utils.h#L650)); this source-state count does not simulate searching, opening, unlocking, permissions, or dynamic travel.

At #45718 the sole authored local exit is D4/up to Malevolence’s chamber #45688. Both source exits have raw flags 7; their ZON resets set state 6, which closes and locks the doors and marks them secret ([Celestia reset](../../../areas/zon/celestia.zon#L104), [reverse reset](../../../areas/zon/celestia.zon#L108), [reset-state implementation](../../../src/world/db.c#L7868)). Thus the two active WLD arrivals land in a room whose only authored way into the main WLD cluster is closed, locked, and hidden by the reset instructions.

The 36 ZON door commands set 18 doors to state 2 (closed/locked), eight to state 14 (closed/locked/secret/blocked), two to state 6 (closed/locked/secret), and eight to state 1 (closed/unlocked). Many paired doors lead through the Life Stream, gravity-well sections, palace gates, towers, and vaults. The apparently accessible Palace approach at #45639 belongs to the separate 70-room WLD component; open-exit reach from it is only two rooms. No encoded local type-25 destination points into this Palace component.

The all-source inactive-WLD scan covered 95 unregistered files, 527,791 rooms, and 2,080,773 exits with no parse issues. It found no inactive room-ID overlap with Celestia and no source-only inactive-WLD exit targeting a Celestia room. Static edges therefore do not explain the Palace connection or the return from either #45718 arrival.

## Reset-placed portals and code-defined travel

Celestia has three local `ITEM_TELEPORT` (25) objects. Their `value[1]` is command 7 (`CMD_ENTER`), while `value[0]` supplies the destination ([command value](../../../src/cmd/interp.h#L42), [generic item-teleport dispatch](../../../src/cmd/interp.c#L2412), [handler and destination lookup](../../../src/magic/spell_travel.c#L932)).

| Object | Encoded target | Known reset placements |
| ---: | --- | --- |
| #45535 | Astral Main #19721 | Celestia #45500 ([definition](../../../areas/obj/celestia.obj#L526), [reset](../../../areas/zon/celestia.zon#L153)) |
| #45536 | Celestia #45500 | Astral Main #19721 and #19796; Ethereal Main #12401, #12406, and #12431 ([definition](../../../areas/obj/celestia.obj#L534), [Astral resets](../../../areas/zon/astral_main.zon#L9)) |
| #45538 | Celestia #45500 | Celestia #45718 ([definition](../../../areas/obj/celestia.obj#L550), [reset](../../../areas/zon/celestia.zon#L161)) |

The special table binds #45536 to `teleporting_pool` ([assignment](../../../src/specs/specs.assign.c#L1920)); that procedure calls `magic_pool` for `CMD_ENTER`, resolves `value[0]`, and calls `teleport_to` ([pool handler](../../../src/specs/specs.teleport_pools.c#L19), [destination path](../../../src/specs/specs.teleport_pools.c#L107)). The generic dispatcher also has an `ITEM_TELEPORT` path. In source, a reset-loaded #45538 could take an entrant from #45718 to #45500, and the portal at #45500 could lead to Astral Main #19721. Astral Main and Ethereal Main reset #45536 as return portals to #45500. These are code- and reset-defined possibilities; current reset presence, command use, player access, return safety, and live traversal were not checked. No item route in the scanned local OBJ file bridges the 70-room Palace component to the main component.

The #45718 reset also loads object #63 beside #45538. It is absent from `celestia.obj`; active `dalvik.obj`, `heavens.obj`, `hostel.obj`, and `mavulsk.obj` each define #63, with unrelated prototypes ([Dalvik](../../../areas/obj/dalvik.obj#L801), [Heavens](../../../areas/obj/heavens.obj#L892), [Hostel](../../../areas/obj/hostel.obj#L823), [Mavulsk](../../../areas/obj/mavulsk.obj#L1029)). Celestia’s own #45537 is a liquid-container pool and is not reset there ([local object](../../../areas/obj/celestia.obj#L542)). Confirm the intended #63 owner and whether the local pool should be used before changing either reset or prototype.

## Typed inbound and outbound dependencies

Bounded source scans covered active and unregistered QST, SHP, sidecar, ZON, MOB, and OBJ inputs; exact room/mobile/object values; active and inactive WLD sources; and the compiled C/C++ references. Findings:

- External active ZON files reset Celestia objects: Astral Main resets #45536 twice; Ethereal Main resets it three times; Negative Plane resets #45534 at #26863; Vehicles equips #45530 and #45531 on its mobiles. Celestia itself resets #45535, #45538 and other local objects, external object #63, and several cross-area potion objects ([Celestia resets](../../../areas/zon/celestia.zon#L153), [Astral Main](../../../areas/zon/astral_main.zon#L9), [Ethereal](../../../areas/zon/ethereal_main.zon#L9), [Negative Plane](../../../areas/zon/negplane.zon#L198), [Vehicles](../../../areas/zon/vehicles.zon#L831)).
- No external reset-role reference targets a local Celestia room or mobile. No external QST item command or SHP product matches a local Celestia object VNUM. No external mobile/object file duplicates a local Celestia prototype.
- The scan found 867 external type-25 declarations and none targets a Celestia room. It also found 376 external type-29 (`ITEM_SWITCH`) declarations; no numeric value in those prototypes matches a Celestia room, mobile, or object VNUM. The 231 story sidecars parsed without errors and had no Celestia VNUM hit.
- Compiled local bindings assign `Malevolence` to mobile #45565, `Malevolence_vapor` to #45571, and `celestia_pulsar` to #45510 ([bindings](../../../src/specs/specs.assign.c#L1244)). Object #45536 uses `teleporting_pool`; #45528, #45514, and #45525 have their own special procedures ([bindings](../../../src/specs/specs.assign.c#L1557)).
- Compiled code lists #45565 in the high-drop mob table ([randomeq.c](../../../src/item/randomeq.c#L124)); uses potion #45534 as the level-56 random-zone potion ([random-zone generator](../../../src/world/random.zone.c#L962)); includes #45530 and #45531 in the Master equipment set ([set members](../../../src/specs/specs.set.c#L47)); and assigns a Celestia skill-beacon load to room #45718 ([progression table](../../../src/specs/specs.character_progression.c#L45)). `Malevolence` can banish non-trusted players to #45500–#45520 while fighting and has a low-health transformation/death path ([procedure](../../../src/specs/specs.celestia.c#L17)).

These scans cover repository source inputs only. External editor deployments, builder tools, persisted player/account data, and live server state were not inspected. The exact reference inventory supports retention; a safe path change still requires those remaining references to be qualified.

## Duplicate/history review and owner decisions

The normalized title “Celestial Plane” is unique among 351 active `.zon` titles and among all 442 current `.zon` source titles. Followed WLD, MOB, OBJ, and ZON paths retain the `celestia` stem back to the 2009 repository import. Tracked 2014 edits move the Celestia stone and lock the vault/Malevolence door (commit `5d3addfd8`); another 2014 ZON edit moves the ioun and artifact loads onto Malevolence (commit `d6533d357`). The bounded tracked paths establish same-stem maintenance and show no alternate-stem rename, distinct predecessor, or whole-zone replacement. No unregistered `.zon` source shares the normalized title, the inactive-WLD scan found no Celestia room overlap, and external MOB/OBJ scans found no duplicate local prototype definitions.

Ask the builder to resolve the intended Palace access, #45718 portal availability and safety, the cross-setting room descriptions, the competing #63 prototype definitions, and the summon/no-load rooms and Malevolence encounter. Do not silently create a WLD edge, retarget an item, or normalize the prose as part of this audit.

Keep active `celestia` **DO NOT MOVE**. Preserve its three active WLD boundaries, all 230 rooms, keyed/reset door state, portal destinations, reset placements, imported object consumers, and compiled bindings. No attic candidate row is added. Before any separate owner-led retirement proposal, close source/build and editor/tool references, the #63 prototype resolution, reset-dependent route behavior, and persisted-state references; then review a concrete proposal with the area owner. The [coverage row](ZONE_COVERAGE_REGISTER.csv#L49), [source dossier](../zone-stories/CELESTIA.md), [world-connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-200-celestia-static-graph-and-reset-placed-routes), [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-200-celestia-title-and-path-history), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-200-celestia-active-zone-retention) track the evidence.

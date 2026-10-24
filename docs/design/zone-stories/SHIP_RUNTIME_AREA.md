# The Adventurers Shipyards: runtime ship rooms

Checkpoint 360 documents the active source area key ship. It is distinct from [Ship Yards](SHIP_YARDS.md), which maps the player-facing shipy port, vendors, services, and story exchanges. The ship area is a fixed VNUM pool and three special rooms used by the ship and ferry code; it is not a second port or a populated quest zone.

## Source shape and authored prose

Active source files are [ship.wld](../../../areas/wld/ship.wld), [ship.zon](../../../areas/zon/ship.zon), [ship.mob](../../../areas/mob/ship.mob), and [ship.obj](../../../areas/obj/ship.obj). The area header is 600 and covers #60000-#64999. It contains 5,000 unique rooms, 13 mobile prototypes, and four object prototypes. Its ZON is reset mode 2 with no command rows. There is no local QST, shop, or story sidecar, although ship is listed in areas/qst/AREA.

The authored WLD labels only four room roles. #60000 is Davy Jones' Locker; #60001 is “Speeding Across The Ocean”; #60002 is aboard a spectral galleon. The remaining 4,997 records, #60003-#64999, all have the same “Aboard a ship” name, empty room prose, stored flags 176193608, and sector 1. They are distinct numbered slots, not duplicate rooms to merge. No room contains an authored D exit, fall marker, or current marker.

The active ship code defines #60000 as Davy Jones' Locker, #60001 as the transit room, and #60002 as the undead ferry room. Its generic ship interiors begin at #60003. The static graph therefore has 5,000 isolated nodes, while the source comments and implementation describe runtime allocation and exit creation for actual ship layouts. Static WLD isolation is not the live ship interior graph.

## Code-owned dynamic pool

The ship header fixes the room pool at #60000-#64999, reserves the first three, caps a ship layout at 15 rooms, and permits up to 2,000 ship records. [find_free_ship_room](../../../src/ships/ship_base.c#L1583) scans from #60003 and marks a room in use when its room procedure is attached. [set_ship_physical_layout](../../../src/ships/ship_base.c#L1617) claims rooms, wires the saved room-slot graph as exits, and assigns names. [clear_ship_layout](../../../src/ships/ship_base.c#L1546) frees those dynamic exits and makes rooms reusable. The full 4,997-room interior pool can hold at most 333 simultaneous 15-room layouts; actual ship layouts may use fewer slots, and that arithmetic is not a promise of service capacity.

At boot, [initialize_ships](../../../src/ships/ship_base.c#L547) binds special procedures to the control panel and ship prototypes, resolves the two fixed ship-system rooms, and loads persisted ship records. This is code-owned state, not ZON reset stock. Exact arrival, movement, teardown, save/recovery, and safe travel are not established by the template WLD.

## Fixed prototypes and their source roles

| Namespace | Prototype | Source role |
| --- | --- | --- |
| Object | #60000 | “Ship Control Panel Object.” The ship system assigns the panel procedure in source. |
| Object | #60001 | Type-28 ship hull prototype used when the ship system creates a ship object. |
| Object | #60002 | Wooden cargo crate prototype used by ship cargo helpers. |
| Object | #60003 | Spectral galleon prototype used by the separate Charon ferry path. |
| Mobile | #60001-#60007 | Roving hellbeasts and wasteland/undead figures. No local ZON reset or active external typed ZON reset was found for these prototypes. |
| Mobile | #60008 | Charon boatman; named by the ferry code, given its mobile special procedure, and read by ferry initialization. |
| Mobile | #60009-#60013 | Undead behemoth, zombie warrior, wight warlord, dracolich, and bog wyrm. No local ZON reset or active external typed ZON reset was found. |

The ferry source reads spectral galleon #60003 and Charon #60008; the Underworld special uses fixed room #60002. These source bindings are distinct from the dynamic player-ship interior pool. The legacy Black Pearl/zone-ship loader is explicitly dormant in [ship_npc.c](../../../src/ships/ship_npc.c#L2056); its call and old zone-link path are commented out.

## Title, history, and disposition

After removing color codes and the terminator, trimming whitespace, and folding case, “The Adventurers Shipyards” occurs once among 442 current ZON title records and once among 351 active contributors. The followed WLD history remains on ship through 2016-10-26 and reaches the 2009-08-12 area-tree move; ZON history reaches 2016-08-15 and the same 2009 move; MOB history includes a 2026-08-26 maintenance edit and the 2016 Charon move; OBJ history reaches 2016-10-26. No same-type active room, mobile, or object header overlap was found.

The 2016 source history explicitly removed the old player_ships files and moved Davy Jones' Locker and the ship-transit room into ship.wld (commit 857124f14). This is a recorded predecessor cleanup, not a current competing zone. Keep ship DO NOT MOVE: the VNUMs and contiguous generic room pool are native ship-system contracts. Do not merge it with active shipy. No active WLD edge, inactive WLD approach, or candidate-specific migration case was found.

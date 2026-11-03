# Ship runtime-area audit

Checkpoint 360 completes source coverage for physical line 272, area:ship. This is a system-owned room pool and fixed ferry rooms, distinct from the active shipy port and its [Ship Yards story dossier](../zone-stories/SHIP_YARDS.md). The WLD contains 5,000 rooms and no authored exits; native code reserves the first three and dynamically allocates #60003-#64999 as ship interiors.

## Builder questions and conservative recommendations

1. **Is “The Adventurers Shipyards” intended as the title for runtime room templates rather than the player-facing ship port?** Keep the title and the shipy identity separate until the area owner chooses a rename; do not move shipy content into this source.
2. **Is the entire #60000-#64999 VNUM span a permanent ship-system contract?** Preserve the contiguous 5,000-room source pool and matching code constants together; do not reclaim or split IDs based on their currently empty prose.
3. **Should #60000, #60001, and #60002 remain the three reserved system rooms?** Preserve their Davy Jones, transit, and undead-ferry roles and the matching constants until any coordinated system revision is approved.
4. **Are 4,997 identical “Aboard a ship” room records intentional allocation slots?** Keep every distinct room VNUM; the ship loader claims IDs from this sequence, so deduplicating identical names would shrink or break its pool.
5. **Are stored flags 176193608 and sector 1 the intended baseline for every generic room?** Preserve the raw WLD data until the ship owner confirms which room flags are required before and after a ship is loaded.
6. **Should static WLD connectivity remain empty because ship exits are reconstructed from each saved ShipRoom graph?** Keep the zero authored exits and document runtime links separately; do not add static passages as a substitute.
7. **Is MAX_SHIP_ROOM 15 the intended maximum interior size for each ship?** Preserve the current array bound and per-layout encoding together; any larger ship needs coordinated persistence and table changes.
8. **Is the room-pool capacity acceptable relative to MAXSHIPS 2,000?** The current pool holds at most 333 simultaneous 15-room interiors, with lower counts possible for smaller ships. Ask the ship owner to decide whether those limits match intended concurrency before changing either constant.
9. **Is reset mode 2 with no ZON commands intentional?** Keep the empty reset file; the ship objects and saved layouts are created through code and persistence rather than area stock.
10. **Should control panel object #60000 remain procedure-bound rather than reset into a room?** Preserve its prototype VNUM and the ship initializer binding; no reset-based panel is inferred.
11. **Should object #60001 remain the generic type-28 ship object used by new_ship?** Keep the exact object and compile-time VNUM relationship; do not treat it as an ordinary placed boat.
12. **Is cargo-crate object #60002 still the only prototype intended for the ship cargo helpers?** Preserve it for cargo storage code and distinguish it from the spectral galleon object #60003.
13. **Should spectral galleon #60003 continue to pair with room #60002 in the separate Charon ferry path?** Preserve both identities and the typed code references until the ferry owner confirms a replacement.
14. **Is Charon mobile #60008 intended to be loaded dynamically with his special procedure?** Retain its mobile prototype and code assignment; no local ZON spawn is present.
15. **Which of mobile prototypes #60001-#60007 and #60009-#60013 remain supported?** Keep all definitions pending an owner inventory of script, staff, and legacy callers. Their lack of local or active external ZON resets is not authorization to delete them.
16. **Should the commented Black Pearl/zone_ship implementation remain as reference code?** Keep its dormant status and do not uncomment it as part of a map audit; its loader, room links, and special cases were intentionally disabled together.
17. **Is the 2016 retirement of player_ships the complete replacement history?** Preserve the current ship.wld rooms and record the exact moved roles. Do not restore or create a parallel player_ships zone from the historical commit.
18. **Should dynamic room claims continue to mark rooms by attaching ship_room_proc and release them only through clear_ship_layout?** Preserve that in-use/reuse contract; any lifecycle repair should be a separately scoped code change with persistence review.
19. **Are the separate code-owned Davy/transit/ferry arrivals the intended ways into fixed ship-area rooms?** Preserve those route consumers and avoid inventing static WLD edges or a local quest prerequisite.
20. **Is qst/AREA membership intentional despite no ship.qst file or local sidecar?** Keep the index fact separate from actual local quest content; do not add a new journal or conflate this runtime area with the story-bearing shipy port without an owner design.
21. **Should `areas/qst/AREA` retain the historical `player_ships *500-509` row after the 2016 change remapped the Davy's Locker and transit rooms from #50000/#50001 to #60000/#60001 under active `ship`?** Ask the QST manifest owner to reconcile the old row; preserve current ship room identities and do not recreate a parallel `player_ships` area.

These recommendations retain existing source and code contracts. They do not establish a loaded ship, booted ferry, safe journey, or live service. Any native repair needs a separate named scope.

## Typed callers, source boundaries, and migration

Exact active WLD scans found no route into the pool; all 95 inactive WLD files were checked for room-header overlap and destination references, with none found. Active ZON, typed QST, shop-source, and typed story-sidecar scans found no imports of these MOBJ prototypes. The story-sidecar scan checked `mob_vnum`, `giver_vnum`, and `item_vnums`. The compiled ship subsystem directly owns panel #60000, hull #60001, crate #60002, Davy Jones room #60000, transit room #60001, and the generic pool bounds. The ferry code owns galleon #60003 and Charon #60008; Underworld code references ferry room #60002. Keep active ship DO NOT MOVE and add no candidate or source-only route record.

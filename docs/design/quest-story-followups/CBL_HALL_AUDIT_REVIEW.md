# CBL Hall repository-wide audit supplement

This checkpoint audits active `cbl_hall` against the source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`. It adds no native area or code changes and does not inspect live players, persisted state, or a running server.

## Inventory and narrative boundary

The active row is `cbl_hall *297-298` at [areas/AREA line 143](../../../areas/AREA#L143). The independent [quest index entry](../../../areas/qst/AREA#L133) has no same-stem QST file, native Q/QA contract, or story sidecar behind it. The source has 124 rooms, 8 mobiles, 14 objects, 2 shops, and 202 authored exit records. The two shops and their 11 shared products were missing from the coverage row’s initial shop count; this checkpoint corrects that row to #29701–#29703, count 2.

The room text forms a collage of the guild’s self-image, inn, trophies, arena/citadel, memorial, parody shrines, portal signs, underdark outposts, and Monster Island. See the [source-story dossier](../zone-stories/CBL_HALL.md) for room examples and builder questions. No story source was edited.

## Static graph and unregistered-WLD boundary

The 124-room active WLD file has 202 exits: 201 local and one outbound exit whose target resolves to active Heavens. Every target resolves; there are no negative or positive unresolved targets, rooms outside the numeric zone range, range-mismatch source rooms, nonzero door flags/keys, fall chances, or currents. The local undirected graph has 13 weak components (eleven of size 10, one of size 9, one of size 5). Raw directed WLD reach from #29703 covers 10 rooms. This is a source-graph result; it does not qualify traversal through item commands, special handlers, reset timing, or current player permissions.

| Boundary direction | Source room | Exit | Target room | Reverse record |
| --- | --- | --- | --- | --- |
| Active CBL outbound | CBL #29703, [guildhall](../../../areas/wld/cbl_hall.wld#L74) | D4/up | Heavens #420, [Zion’s Hall of Quitting](../../../areas/wld/heavens.wld#L631) | Heavens D5/down back to #29703 |

No other active WLD boundary crosses the CBL range. From #29703, raw local reach is 10/124, so the reciprocal Heaven connection does not establish access to the remaining CBL components.

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without parse issues. No inactive WLD source-room VNUM overlaps an active CBL room. It found one non-overlapping source-only approach: unregistered `ud_map2.wld` room #1124947 at [line 59101](../../../areas/wld/ud_map2.wld#L59101), D5/down at [line 59122](../../../areas/wld/ud_map2.wld#L59122), targets active CBL #29703. `ud_map2` is not listed in the full `areas/AREA` loader, so this is not active ingress. Its candidate row 114 now records 17 reviewed source-only approaches and retains the source in place.

## Encoded portal objects and qualified runtime links

All fourteen CBL object prototypes are type `ITEM_TELEPORT` (25) and encode a CBL room in `value[0]`. Eight objects have ZON placements. The object/reset relationship is:

| Object | Reset room | Encoded target |
| ---: | ---: | ---: |
| #29700 | #29710 | #29708 |
| #29706 | #29711 | #29780 |
| #29704 | #29740 | #29792 |
| #29712 | #29750 | #29708 |
| #29708 | #29781 | #29709 |
| #29703 | #29792 | #29740 |
| #29702 | #29797 | #29709 |
| #29710 | #29824 | #29708 |

These destinations are prototype values, not graph edges counted above. The code has a generic `ITEM_TELEPORT` command path ([handler](../../../src/magic/spell_travel.c#L932), [target lookup](../../../src/magic/spell_travel.c#L991)); `do_enter()` traverses the room’s authored exits ([entry handler](../../../src/cmd/actmove.c#L3378)). The current special-assignment table binds `portal_door` to other object VNUMs (#751, #782, #752, #781), with no direct CBL portal VNUM binding ([#751 assignment](../../../src/specs/specs.assign.c#L1781), [#752 assignment](../../../src/specs/specs.assign.c#L1791)). The source scan therefore records CBL item destinations as encoded possible routes and leaves actual command eligibility, callback path, return, and player safety unqualified.

## Typed inbound and outbound dependencies

Bounded scans covered active and unregistered QST, SHP, sidecar, ZON, MOB, OBJ, and exact numeric C/C++ source references. Results:

- No external QST, SHP, or story-sidecar numeric hit targets a CBL room, mobile, or object VNUM.
- No external ZON reset-role record or duplicate mobile/object prototype targets or defines a CBL local VNUM.
- Across 858 external type-25 declarations, none encodes a CBL room destination. Across 380 external type-29 declarations, none targets a CBL room/mobile/object VNUM. The 231 sidecars parsed without errors or CBL token hits.
- The only exact compiled CBL references found are the arena’s EVIL return/home value #29700 ([arena code](../../../src/combat/arena.c#L52)) and the inn special at #29701 ([special table](../../../src/specs/specs.assign.c#L2446)). The room title of #29700 calls it “The Goodie Bitchin Room,” so confirm this mapping with the arena owner.
- The two CBL shops stock eleven product VNUMs each ([shop #29701](../../../areas/shp/cbl_hall.shp#L1), [shop #29703](../../../areas/shp/cbl_hall.shp#L54)). Current prototypes resolve for #377/#398/#364/#412/#347/#111 in `heavens.obj`, #3050 in `verz1.obj`, #53 in `dalvik.obj`, `heavens.obj`, `hostel.obj`, and `mavulsk.obj`, and #108/#109 in `heavens.obj` and `mavulsk.obj`. Product #6089 has no current OBJ prototype. Its old Tharnadia vial was renumbered; the current prototype is #132589 at [tharnadia.obj line 1471](../../../areas/obj/tharnadia.obj#L1471). Both shop stock and the two merchant inventory reset lists still use #6089 ([resets](../../../areas/zon/cbl_hall.zon#L38), [second merchant](../../../areas/zon/cbl_hall.zon#L74)).
- CBL reset placements also use shared `player_castles.obj` decorations #11013, #11054, #11055, #11062, #11104, and #11105 ([local placements](../../../areas/zon/cbl_hall.zon#L11)).

The missing #6089 prototype and the usefulness of the local portal objects remain owner questions. The exact external typed scans do not cover external editor deployments, scripts/tool databases outside the repository, or persisted player/account state.

## Title, history, and duplicate-content review

The normalized active `.zon` title “The Guildhall of Clan BloodLust” is unique among the 351 active contributors. Followed tracked paths begin with the 2009 source import. The WLD has same-stem 2011 and 2013 edits; MOB history has 2010, 2014, and 2026 same-stem entries; OBJ history begins at import; ZON has 2013 and 2015 maintenance; SHP includes same-stem 2013/2014 edits. The bounded active paths show no alternate-stem rename, distinct predecessor, or whole-zone replacement.

Two unregistered sources, `Monsteri` and `cbl_outpost2`, have zone #1000–#1004, identical title “CBL Guildhall Monster Island,” room IDs #1000–#1004, mobile IDs #1000–#1003, and matching four-mobile reset rows. Neither is in `areas/AREA`; `Monsteri.obj` and `Monsteri.qst` are zero-byte files, while `cbl_outpost2.obj` is absent. Their VNUM ranges do not collide with active WLD room VNUMs, and no active WLD edge targets #1000–#1004.

Active CBL rooms #29820–#29824 reuse Monsteri’s five room descriptions after color-token and whitespace normalization; four of five titles also match. `cbl_outpost2` has a generic first title and formatting variants elsewhere. The four active Monster Island mobiles have corresponding roles, but exact prototype equality has not been established. The sources show strong duplicate-content evidence, while tracked history does not establish which source is canonical or whether an unregistered file replaced the active rooms. `infernal_gh` and `guildhalls` are separate identities, not evidence of a CBL rename.

## Candidate-specific retention plan and disposition

- Keep active `cbl_hall` **DO NOT MOVE**. Preserve its Heavens boundary, all local room/object IDs, arena and inn references, reset and shop dependencies, and encoded portal prototypes. No active candidate row is added.
- Keep unregistered `Monsteri` at candidate row 72 and `cbl_outpost2` at row 24 **RETAIN IN PLACE**. Before any owner-approved archival proposal, identify the canonical Monster Island source and intended relation to active #29820–#29824; close typed code/reset, editor/tool, identity, and persisted-reference checks; retain the historical source content for provenance; then review a concrete path change separately. Current scans do not clear either path for attic.
- Keep `ud_map2` at candidate row 114 **RETAIN IN PLACE**. Preserve its newly recorded room #1124947 D5/down approach to CBL #29703 alongside its other source-only approaches and the separate unresolved active Shafts reference. It is not active ingress.
- Ask the CBL owner to resolve the #6089 stock and portal/runtime questions before a separate native content change. This audit does not alter WLD, MOB, OBJ, ZON, SHP, C++, or persisted data.

Coverage is reconciled in [row 48](ZONE_COVERAGE_REGISTER.csv#L48); candidate rows are [24](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L24), [72](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L72), and [114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114). The active-zone source dossier is [CBL_HALL.md](../zone-stories/CBL_HALL.md).

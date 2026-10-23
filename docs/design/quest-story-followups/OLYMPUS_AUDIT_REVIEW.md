# Olympus: connectivity, portal behavior, and retention review

This bounded static review accompanies the [Olympus source dossier](../zone-stories/OLYMPUS.md). It reconciles the WLD graph, unresolved west exit, old Surface target identities, local portal reset layout, dormant special-procedure assignment, name-only Herders reference, typed dependencies, title ownership, and followed same-stem history. It does not establish installed portal bindings, runtime arrival, or played outcomes.

## Active WLD graph and unresolved west exit

Active `olympus` has ten rooms (#99800–#99809) and 19 authored exits. Eighteen resolve locally; the only unresolved target is room #218064, west from #99809 ([source exit](../../../areas/wld/olympus.wld#L158), [unresolved-exit register row 318](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L318)). The local graph is one weak component of ten rooms. From #99800, all ten rooms are reachable and all ten can return to #99800 along authored local exits. Each of the 18 resolving local exits has an opposite-direction record. All raw exit states and keys are zero; the local ZON has no door (`D`) resets, fall fields, or currents.

There are no registered active cross-area WLD exits into or out of Olympus. The #218064 target is positive-unresolved, not a usable active boundary. Normal loading omits its source maps and `renum_world` prunes the exit. The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 direction records without issues and found no Olympus room overlap or exit into its ten room IDs.

Room #218064 occurs in both inactive candidate maps. In `surf.wld`, it is “The Vast Bay of Tharnadia” ([room](../../../areas/wld/surf.wld#L378980)); in `surface2011.wld`, it is “The Azure Waters of a Stormy Sea” ([room](../../../areas/wld/surface2011.wld#L379345)). Both records have the same four neighboring VNUMs, but neither is in `areas/AREA`, neither provides a direct reverse exit to Olympus, and the titles do not identify which source should own the target. Preserve the single active Olympus reference on both existing candidate rows, `surf` and `surface2011`; it is already included in each row's 145 active references. Do not infer an active route or map replacement from the matching VNUM.

## Portal resets and dormant code path

The active reset table has twelve commands: eight `O` rows reset the four color pairs shown in the [source dossier](../zone-stories/OLYMPUS.md#portal-layout), and four identical `M` rows name ancient sentry #99800 at #99809 with a cap of four. No local QST, SHP, or story sidecar is present, and no local giver or contract is defined.

All eight portal objects are type 25, use command selector 7 (`enter`), and declare unlimited charges (-1). Under that static data, the `check_item_teleport` path uses the declared target ([teleport handler](../../../src/magic/spell_travel.c#L932)). The separate `olympus_portal` procedure ([implementation](../../../src/specs/specs.olympus.c#L18)) would run periodically, choose non-ocean rooms from zone ranges 140000, 150000, 160000, and 170000, move selected objects there, and update their paired return objects. But the four assignments are disabled under `#if 0` in [specs.assign.c](../../../src/specs/specs.assign.c#L1365). Treat the roaming behavior as dormant source code unless an effective installed binding is independently demonstrated.

Other source code recognizes these objects without enabling that procedure: [map.c](../../../src/world/map.c#L588) classifies type-25 objects numbered 99800–99899 as portal map markers, and [tradeskill.c](../../../src/economy/tradeskill.c#L3996) has a special-value guard for the `olympus_portal` procedure pointer. These references do not override the disabled assignment block.

## Name-based story reference

Herders' active QST response to `flaming horde` says the horde lives deep within Mount Olympus while the Herders are still digging ([QST text](../../../areas/qst/herders.qst#L34)). The text has no Olympus room/mobile/object VNUM, no item requirement, and no WLD route. The area defines one ancient sentry and no flaming-horde prototype. Record this as a name-only narrative link; do not infer that the visible sentry is the referenced horde. The [Herders dossier](../zone-stories/THE_BEHEMOTH_HERDERS.md#lore-foreign-ownership-and-orphan-closure) and its [audit review](HERDERS_AUDIT_REVIEW.md#bounded-typed-references) now cross-reference this qualification.

## Exact typed and compiled dependencies

An exact context-aware scan found no external active ZON reset references to Olympus rooms, mobile, or objects; no external type-25 object targets into the ten local rooms; no external duplicate mobile/object prototype; no exact Olympus VNUM in external QST, SHP, or story-sidecar data; and no numeric story-sidecar match. The Herders QST mention above is the sole name-only narrative reference found.

The four object-procedure assignments and the procedure's local VNUM/room references are the direct Olympus-specific code connections. The active `#if 0` guard makes the assignment status explicit in source. The broad map marker range and tradeskill function-pointer check are supporting code paths, not evidence of a runtime portal controller. No external typed dependency requires a candidate row beyond the unresolved #218064 reference already tracked by `surf` and `surface2011`.

## Title, bounded history, and retention

After color stripping and case folding, “Olympus” belongs only to active stem `olympus` among 442 `.zon` titles ([title source](../../../areas/zon/olympus.zon#L2)). Followed WLD, MOB, OBJ, and ZON histories stay on that stem and reach the 2009 move into the current `areas/` tree; later entries are same-stem content or source-format edits. These bounded paths establish no alternate-stem rename, distinct predecessor, or whole-zone replacement. The Herders phrase “Mount Olympus” is narrative context, not replacement evidence. The different #218064 titles in inactive `surf` and `surface2011` likewise do not establish which map is newer or authoritative.

Keep active `olympus` **DO NOT MOVE**. Preserve its ten-room graph, eight local portals, four sentry reset rows, disabled special-procedure assignments, and the unresolved #99809 west edge. Keep both `surf` and `surface2011` **RETAIN IN PLACE** with #218064 recorded on each existing candidate row. Each candidate remains at 145 active exit references; the current source-only totals remain 4,248 overall, 251 for Surf, 19 for ud_map2, and eight for Underworld2. No candidate row or active route is added. Editor, administrative, installed-data, indirect-runtime, and gameplay qualification remain outside this review.

## Checkpoint 316: twenty direct builder questions and recommendations

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| OLY-01 | Is Olympus intended as a travel gateway, an encounter setting, or both? | Keep the current environmental description until a specific player objective, beneficiary, and outcome are defined. |
| OLY-02 | Are the five nearly identical Golden Hall descriptions intentional repetitions? | Preserve the rooms and text until the owner confirms whether each hall should communicate a distinct stage or remain a repeated vista. |
| OLY-03 | Do the four colored archways represent specific realms or destinations beyond their local rooms? | Retain their named colors and declared VNUMs; document any realm-level intent with exact destinations before changing a target. |
| OLY-04 | Is the two-step item route from #99804 through a colored room to #99809 the intended portal circuit? | Keep object and WLD routes distinct; qualify actual use and return before promising a complete circuit. |
| OLY-05 | Should all eight portals remain unlimited-use `enter` items? | Preserve selector 7 and charges -1 unless the owner specifies admission, cooldown, or finite-use behavior. |
| OLY-06 | Is `olympus_portal` meant to be bound to objects #99801, #99803, #99805, and #99807? | Leave the existing `#if 0` block unchanged until the intended live behavior and compatibility are approved. |
| OLY-07 | If the roaming handler is intended, are zone bases 140000, 150000, 160000, and 170000 still the desired destination pools? | Verify every pool's ownership and loaded status before enabling those source-code targets. |
| OLY-08 | Is the handler's 30-second interval and random room selection acceptable for a player-facing portal? | Confirm timing, destination visibility, concurrency, and refusal behavior before making the handler active. |
| OLY-09 | Is excluding only ocean-sector rooms enough to ensure safe destinations if roaming is enabled? | Define no-travel, hazard, instance, combat, and return rules before enabling random arrivals. |
| OLY-10 | Should each roaming color portal update its matching return portal's target as the code specifies? | Preserve the pair mapping only after builders approve and qualify that return behavior; avoid claiming safety from the code alone. |
| OLY-11 | Is the west exit from #99809 to #218064 an intended route? | Keep the unresolved source edge visible; do not treat it as an active connection while the target is pruned. |
| OLY-12 | Which inactive source identity is intended for #218064: “The Vast Bay of Tharnadia” in `surf` or “The Azure Waters of a Stormy Sea” in `surface2011`? | Retain both candidate records and request an explicit destination-owner decision; VNUM equality does not choose a map. |
| OLY-13 | If a Surface target is selected, should it be added to active world data or should the west exit be retired? | Decide the intended route and return first, then make any source-list change as a separately reviewed world change. |
| OLY-14 | Does the selected #218064 destination need a direct return exit to Olympus? | Confirm reciprocal direction and arrival safety before promising an out-and-back route. |
| OLY-15 | Does Herders' “flaming horde” line refer to this Olympus area or a separate, unmapped place? | Keep it as a name-only story lead until the intended room/mobile is identified; do not assign the ancient sentry by inference. |
| OLY-16 | Should a flaming-horde encounter be authored locally? | Treat it as a separate content decision requiring a defined encounter, route, population, and story outcome. |
| OLY-17 | Are the four identical sentry `M` resets with cap four deliberate? | Preserve them while confirming the intended maximum population and reset cadence. |
| OLY-18 | Is the area intentionally listed in `areas/qst/AREA` without a local QST file? | Confirm the manifest entry; keep the area lore-only unless a complete native objective and replay policy are approved. |
| OLY-19 | Should a story sidecar be added to describe portal and destination history? | Keep no sidecar until builders define stable facts, exact local stage transitions, and any useful player outcome. |
| OLY-20 | Should the zero-state WLD links and absence of local door resets remain open to all visitors? | Preserve the authored graph and lack of gates until a specific access restriction and safe escape are justified. |

## Scope and validation

Checkpoint 316 adds documentation, updates the Olympus coverage row, records the existing unresolved exit on its two existing candidate rows, and reconciles the name-only Herders reference. Source scans are read-only. No WLD, MOB, OBJ, ZON, QST, story sidecar, C/C++, database, migration, server, or gameplay state is changed. Static links, register shape and row, question count, whitespace, and restricted-directory status are checked after the documentation edits; no build or source test is run.

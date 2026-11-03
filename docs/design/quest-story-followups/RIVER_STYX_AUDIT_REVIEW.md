# The River Styx: source audit supplement

This supplement records the checked-in source evidence for active zone #907 (`river_styx`) at inventory snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`. The [source dossier](../zone-stories/RIVER_STYX.md) summarizes the environmental story and provides twenty builder questions with recommended resolutions.

## Source inventory and narrative

The full-world roster lists `river_styx` at `*907` on [line 322 of areas/AREA](../../../areas/AREA#L322); `areas/qst/AREA` also lists it at [line 286](../../../areas/qst/AREA#L286). The local WLD contains 100 rooms (#90700–#90799), the MOB contains 19 prototypes (#90700–#90718), the OBJ contains 23 (#90700–#90722), and the ZON uses reset mode 2. There is no local `.qst`, `.shp`, or story sidecar file, and no native Q/QA contract. QST roster membership does not by itself supply a quest.

The [WLD source](../../../areas/wld/river_styx.wld#L1) runs from the lonesome dock and its bone-horn inscription through river bends, caves, the mauve moss/cess-pit branch, a gemstone chamber, the bloodflow rooms, melted-rock and slime caverns, a windy tributary, a whirlpool, and three submerged rooms. A source-text pass found 87 unique normalized title/description pairs across the 100 rooms; eight exact text families recur. The full room/mobile/object review is summarized in the dossier.

The local [reset source](../../../areas/zon/river_styx.zon#L1) has 104 actions: 75 M, 17 O, two P, one G, three E, and six D. The six D resets have 100% chance and set two secret-door pairs (#90716/#90717 and #90717/#90718) plus the locked adamantium door between #90759 and #90760. Both sides of the last door name key VNUM #90702; the reset script places that key at #90799. Local object teleports connect the pool hall #90761 to #90762/#90763/#90764 and back; #90764 has a local exit to study #90780.

## Static connectivity and reset-aware reachability

The active source has 194/194 loader-resolved WLD exits: 193 local and one outbound. The exit to Hell One is #90700 south to #51001 ([cross-area exit row 1115](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1115); [paired-zone row 410](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L410)). Hell One has no direct return edge to #90700. The current census has zero active inbound edges, zero positive unresolved targets, and zero unregistered WLD target references.

The raw local weak components are 96, 2, 1, and 1 rooms. Starting a directed walk at #90700 reaches 96/100 rooms; the same 96 can route back to #90700 over authored local WLD exits. After the six current chance-100 D states are applied and all closed/locked/secret doors are excluded, strict reach is 92/100. The reset groups are:

| Source rooms and directions | Reset state | Observed branch |
| --- | --- | --- |
| #90716 west and #90717 east | Closed/secret | Mauve moss cavern |
| #90717 down and #90718 up | Closed/secret | Festering cess pit |
| #90759 west and #90760 east | Closed/locked; the WLD exit key is #90702 | Bloodflow chamber and fountain hall |

The four off-graph rooms #90762–#90764 and #90780 are accessed by the six local type-25 pool/portal declarations, not ordinary WLD exits. Each declaration uses command value 7 (`enter`) and a local fixed target. These links form a dynamic overlay only when the traveler reaches the pool hall and the reset objects are available; they do not increase the strict post-reset WLD count.

Room #90740 has an authored north exit back to itself, the only unpaired local direction. The dock-to-Hell One exit is the only other unpaired WLD record and is the only cross-zone exit. Room #90796 describes open swamp water to its south but has just one WLD exit, north to #90795.

The [all-source WLD sweep](WORLD_CONNECTIVITY.md#checkpoint-341-river-styx-static-connectivity) covered all other 445 WLD files: 350 other active sources and 95 non-active sources. It found no inbound edge or room-number overlap. No outside active or source-only type-25 object declares a fixed or random-zone destination into this area. No live player route, return, safety, or reset-population claim follows from those static results.

The WLD declares no fall chances and 71 current fields. Source values range from speed 10 to 50 and point north, east, south, or west. Native movement checks can block some upstream attempts and can sweep a character downstream after movement in a water room; those effects are probabilistic and exclude several character states. See [the movement-command current check](../../../src/cmd/actmove.c#L1403), [post-command sweep](../../../src/cmd/interp.c#L1909), and [room-entry sweep](../../../src/world/handler.c#L1632). Treat current fields as movement risks, not guaranteed graph edges or safe routes.

## Typed dependencies and the Charon/ferry distinction

The local reset file uses 18 local mobile VNUMs but does not load Charon #90708. The typed scan of every registered active ZON found 193 external M rows loading River Styx mobiles and 12 external F follow rows referencing those mobile prototypes, for 205 active reset references:

| Active consumer | M rows | F rows | Imported River Styx mobile VNUMs | Evidence |
| --- | ---: | ---: | --- | --- |
| Avernus | 60 | 12 | #90701–#90707, #90709–#90712 | [Avernus resets](../../../areas/zon/avernus.zon#L13) |
| Hell One | 128 | 0 | #90701–#90706, #90711, #90712, #90715 | [Hell One resets](../../../areas/zon/hell_one_avernus.zon#L10) |
| Ravenloft 2 | 5 | 0 | #90702–#90706 | [Ravenloft 2 resets](../../../areas/zon/ravenloft2.zon#L1310) |

The twelve Avernus F commands set follow relationships; they do not add another mob load. Their references use #90705, #90711, and #90712 on lines 13–14, 70–73, and 77–82 of the [Avernus reset file](../../../areas/zon/avernus.zon#L13). Preserve both the M prototype definitions and F relationships when reviewing that importer.

No external ZON reset references a River Styx object, room, or door. A numeric sweep of external QST files found no exact River Styx room/mobile/object VNUM; no SHP product entry uses a local River Styx object. All 232 other `*.story.json` files parsed successfully and contained no exact structured numeric reference to a local room, mobile, or object. The only numeric hits for #90702/#90706 in C/C++ source are inside a commented-out code block in [`specs.zion.c`](../../../src/specs/specs.zion.c#L100), not active direct prototype bindings. No other active same-type WLD, MOB, or OBJ declarations overlap this area's VNUMs.

The horn/boatkeeper prose does not align with the current native ferry definitions. Local #90700 and #90701 are `ITEM_INSTRUMENT` objects ([object types](../../../areas/obj/river_styx.obj#L1); [instrument type](../../../src/core/defines.h#L147)). The local #90708 mobile is not a zone reset. Native code defines the ferry's character as VNUM #60008, its object as #60003, and its onboard room as #60002 ([VNUM definitions](../../../src/world/vnum.mob.h#L16), [object definitions](../../../src/world/vnum.obj.h#L129), [room definitions](../../../src/world/vnum.room.h#L16)). `init_ferries()` places that other object and mobile at #600586 ([ferry bootstrap](../../../src/world/ferryact.c#L214)); the special's fixed itinerary begins at #600586 and ends at #605825 ([galleon route](../../../src/specs/specs.underworld.c#L4777)). The active Surface WLD calls #600586 “The Lifeless Plains of Dead Rising” ([Surface room](../../../areas/wld/surface.wld#L2111683)); this is not a River Styx room. The compiled sources inspected do not connect the River Styx instrument to that ferry. Confirm with builders whether the horn text is an unimplemented legacy hook or describes a different intended travel action.

## Title, duplicate prototypes, and followed history

After MUD color removal, whitespace normalization, and case folding, “The River Styx” occurs once among 442 current ZON titles and once among 351 active area titles. The local room review did find intentional or accidental repeated text blocks, and the MOB contains same-prose variants (#90702/#90711, #90703/#90712, and #90710/#90713). The two horn OBJ prototypes share the same type/value tuple and description, while only #90700 has the inscription. Preserve the distinct VNUMs until an area owner confirms whether these similarities are intentional.

Followed Git history for the `river_styx` WLD, MOB, OBJ, and ZON remains on the current stem through the 2009 move into the tracked `areas/` tree. The available files show later same-stem edits through 2013 and 2015; the bounded title/path review found no alternate-stem predecessor, title alias, or explicit whole-zone replacement. This conclusion is limited to tracked repository history and current source titles.

## Builder follow-up and disposition

The [source dossier's builder table](../zone-stories/RIVER_STYX.md#builder-questions-and-recommended-resolutions) records twenty questions spanning the horn and ferry mismatch, absent inbound/one-way boundaries, the missing swamp exit, self-loop, door/key route, current hazards, pool portals, story props, repeated prototypes, and the QST roster boundary.

Keep active `river_styx` **DO NOT MOVE** and add no attic-candidate row or migration plan. The one-way active WLD departure and 205 active mobile-reset references (193 M loads and 12 F follow rows) make the source a live shared contributor. There is no candidate-specific dependency record to reconcile; preserve the prototype owner and its active importers in place until the area owner resolves any future redesign separately.

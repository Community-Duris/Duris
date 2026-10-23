# Hell One, Avernus: source audit supplement

This supplement records the checked-in source evidence for active zone #510 (hell_one_avernus) at inventory snapshot 72ba3deff3bb30bde880018cdebe7c6db675b697. The [source dossier](../zone-stories/HELL_ONE_AVERNUS.md) contains the environmental summary and builder questions. WLD exit links below are also recorded in the [cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv).

## Source inventory and narrative

The full-area and QST rosters both list hell_one_avernus at range *510-513 ([AREA](../../../areas/AREA#L221); [qst/AREA](../../../areas/qst/AREA#L202)). The contributor has 400 WLD rooms (#51001–#51400), two mobile prototypes (#51002–#51003), six object prototypes (#51001–#51006), and one ZON. There is no local QST file, SHP file, story sidecar, or native Q/QA contract. qst/AREA membership is roster membership only.

All 400 WLD records repeat the same title and environmental paragraph. The prose describes darkness, crags and hillocks, poisonous vegetation, bare rock, concealed stake pits, no roads or buildings, and caves and warrens that serve as devil homes ([first room](../../../areas/wld/hell_one_avernus.wld#L1)). The mobile prototypes are duplicate swarm-devil entries with matching descriptions ([#51002 and #51003](../../../areas/mob/hell_one_avernus.mob#L1)). Object #51005 describes the unholy Death Rider sword; #51006 describes Tiamat's poison tail stinger ([object file](../../../areas/obj/hell_one_avernus.obj#L45)). The source includes visible spelling errors in the swarm description and the #51006 extra-description keyword.

The ZON file has 209 active reset commands: 129 M, 39 F, 40 E, and one G. It loads 128 River Styx mobiles from nine prototype numbers (#90701–#90706, #90711, #90712, #90715), one local swarm mobile #51003 at #51381, and 39 local #51002 followers. Forty E rows equip poison stinger object #51002; the first G row gives object #51003, the staff of Hellspawn, to the preceding mobile near #51001. This distinguishes mobile #51003 from object #51003 by role even though their numeric VNUM is shared. The records are in the [local reset source](../../../areas/zon/hell_one_avernus.zon#L10) and the [River Styx mobile source](../../../areas/mob/river_styx.mob#L16).

No local reset command is a D or R room-state reset. The source does not provide an area-specific player objective, acceptance/completion contract, shop, or separately authored environmental effect. The WLD records have no fall-chance or current values.

## Static connectivity and reachability

The local source contains 1,600 WLD exits: 1,599 remain inside Hell One and one leaves the zone. All authored exit flags are zero. The induced local directed graph is strongly connected across all 400 rooms; 63 directed exit records have no reciprocal exit in the opposite direction, but alternate local paths still connect the rooms. From each active arrival room (#51001, #51032, and #51207), local directed reach covers 400/400 rooms. Every local room can reach the outgoing room #51100.

The active inbound boundary records are:

| From | Exit | To | Register |
| --- | --- | --- | --- |
| River Styx #90700 | south | Hell One #51001 | [row 1115](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1115) |
| Prison #7492 | north, east, south, west, up | Hell One #51032 | [rows 164–168](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L164) |
| Celestia #45637 | north | Hell One #51207 | [row 623](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L623) |

The [Prison source record](../../../areas/wld/prison.wld#L3513) has five directional exits from the same room to #51032. Hell One's one outbound edge is #51100 east to Tiamat #19600 ([exit source](../../../areas/wld/hell_one_avernus.wld#L2693); [register row 639](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L639)). There is no direct reverse edge from Tiamat #19600 to #51100; its south exit instead returns from Tiamat to active Avernus #32031 ([Tiamat source](../../../areas/wld/tiamat.wld#L15)). The paired-area rows are [Celestia to Hell One, row 73](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L73), [Hell One to Tiamat, row 187](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L187), [Prison to Hell One, row 360](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L360), and [River Styx to Hell One, row 410](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L410).

The full scan of the other 445 WLD files (350 other active sources and 95 inactive sources) found exactly seven active inbound edges listed above, no inactive/source-only approaches, no same-number room overlap, and no additional target into #51001–#51400. The active exit census resolves all 1,600 Hell One source exits. No external ZON D or R room-state reset targets a Hell One room. A scan of 872 external type-25 teleport object declarations found no destination in the range. Static source therefore establishes connected rooms and their authored boundary exits; it does not establish live player access, reset-time population, global safe return, or gameplay safety.

## Typed dependencies and shared prototypes

The Hell One reset script imports River Styx mobile prototypes #90701–#90706, #90711, #90712, and #90715. In the other direction, 18 active external ZON reset rows consume Hell One mobile or object prototypes:

| Active source | Typed references to Hell One | Evidence |
| --- | --- | --- |
| Avernus | M #51003 plus nine F #51002 follower rows in room #32208 (10 actions) | [zone rows 203–212](../../../areas/zon/avernus.zon#L203); already listed in the [Avernus audit supplement](AVERNUS_AUDIT_REVIEW.md#L65) |
| Astral Tiamat | O #51003 staff of Hellspawn in room #19953 | [row 70](../../../areas/zon/astral_tiamat.zon#L70) |
| Bahamut | O #51004 glowing potion in rooms #25928 and #25935 | [rows 165 and 170](../../../areas/zon/bahamut.zon#L165) |
| Celestia | O #51004 in room #45727 | [row 167](../../../areas/zon/celestia.zon#L167) |
| Negative Plane | O #51004 in room #26869 | [row 202](../../../areas/zon/negplane.zon#L202) |
| Shabo | P #51005 Death Rider inside container #32870 | [row 271](../../../areas/zon/shabo.zon#L271); [#32870 is a heavy chest](../../../areas/obj/shabo.obj#L731) |
| Tiamat | O #51004 in room #19637 and G #51006 Tiamat stinger | [rows 95 and 139](../../../areas/zon/tiamat.zon#L95) |

The row count is 10 Avernus mobile actions plus eight object actions. The same numeric value may identify different prototype roles: for example, M #51003 is the swarm mobile in Avernus, while O #51003 is the staff of Hellspawn in Astral Tiamat. The active Avernus audit documents the ten mobile-reset rows; this supplement adds the zone's complete inbound and outbound dependency closure.

Code has direct prototype references as well. The object assignment table binds #51005 to holy_weapon and #51006 to tiamat_stinger ([assignment rows](../../../src/specs/specs.assign.c#L2003)); #51005 is named VOBJ_HOLYSWORD_DEATHRIDER ([VNUM definition](../../../src/world/vnum.obj.h#L117)) and is handled in holy_weapon ([procedure switch](../../../src/specs/specs.heavens.c#L5890)). Three call sites load potion #51004 ([random-zone loader](../../../src/world/random.zone.c#L448), [second loader](../../../src/world/random.zone.c#L946), [Heavens procedure](../../../src/specs/specs.heavens.c#L5830)). These bindings make the object prototypes active shared dependencies.

The typed closure found no external exact QST references, SHP product references, or story-sidecar VNUMs into Hell One; the apparent opaque story identifier containing digits was not a VNUM. There are no external same-role WLD/MOB/OBJ declaration collisions in the range, and no other matching direct C/H numeric bindings were found. The role-scoped scans are repository-source evidence only and do not establish installed runtime state.

## Title, similar names, and source history

After color-code removal and normalization, the current ZON title Hell, Avernus is unique among the 442 current zone titles. It is distinct from active zone avernus (zone #320), whose normalized title is The Plane of Avernus, and from inactive source-only plane_avernus (zone #135), which shares the latter title. plane_avernus contains only two room prototypes; it is not evidence that Hell One was replaced. The phrase The Plane of Avernus on the Nine Hells also occurs in Prison room #7492, which is the source room for the five inbound exits, rather than a separate copy of Hell One.

Followed Git history for Hell One's WLD, MOB, OBJ, and ZON files remains on the same stem. It includes the 2009 move into the tracked areas tree and later same-stem edits through 2015. No alternate-stem predecessor, title rename, deletion, or explicit whole-zone replacement appears in the tracked history. This conclusion is bounded by repository history and does not make a claim about untracked archives or external builder records.

## Builder follow-up and disposition

The [dossier's builder table](../zone-stories/HELL_ONE_AVERNUS.md#builder-questions-and-recommended-resolutions) records the repeated room copy, Prison's five same-destination exits, one-way boundaries, imported mobiles and shared artifacts, absent local quest contract, and spelling corrections.

Keep active hell_one_avernus **DO NOT MOVE**. The full area roster, active ingress and egress, local resets, external reset consumers, and code-bound objects all demonstrate live source dependencies. It is not an attic candidate; do not add a candidate row or migration plan.

# BB GH source audit

Canonical key: `component:bb_gh`. Physical coverage row 365; attic candidate
row 14. The five source files are not listed in areas/AREA, and there is no
local QST source, full-area membership, map JSON, or story sidecar. The source
is retained in place while its map, guild identity, imports, and duplicate
source assignments are resolved.

## Source identity and story-bearing content

The source set is areas/wld/bb_gh.wld, areas/mob/bb_gh.mob,
areas/obj/bb_gh.obj, areas/zon/bb_gh.zon, and areas/shp/bb_gh.shp. Its ZON
header is #1 and its raw title is “BB GH”
([ZON](../../../areas/zon/bb_gh.zon#L1)). Header #1 is used by multiple
current ZON files, while normalized title “BB GH” is unique in the 442-file
scan. The abbreviation is not expanded by the source. Room prose uses “clan”
and “the clan’s history,” but does not provide a full organization name.

The entrance room #30300 is a natural crack in the Underdark lined by two rows
of obsidian pedestals. Blood-painted runes, fallen enemies, and faerie fire
give the hall a ritual and martial tone. The central cave, #30301, has a large
obsidian pedestal surrounded by flames, four bridges over a lake of fire, and
a pentagram filled with boiling blood
([rooms](../../../areas/wld/bb_gh.wld#L1)). The surrounding rooms describe a
goblin alchemist’s laboratory and shop, a war room, vaults, pools and
mushrooms, and a resting cavern with battle plans and clan history. The
resets place stone golems, a goblin alchemist, and shadow wraiths. Four
disconnected WLD pockets describe fiery shafts, blackened earth, cold
surfaces, and lightning-charged air. These are authored map and room-text
details; they do not identify which guild “BB” means or establish current
gameplay.

## Static WLD graph and room ownership

| Measure | Source result |
| --- | ---: |
| WLD rooms | 44 (#30300–#30343) |
| Authored directions | 64: 60 targets in the candidate’s numeric room set, four boundary exits |
| Local weak components | 5, of sizes 10, 10, 10, 10, and 4 |
| Largest local component | 10 rooms |
| Rooms reachable from #30300 on authored local exits | 10 / 44 |
| Rooms with fall-chance or current records | 0 / 0 |

The entrance-side component is #30300–#30309. Four further components are
#30310–#30319, #30320–#30329, #30330–#30339, and #30340–#30343. The static
WLD graph therefore does not connect the four pockets back to the entrance.
The four reset portal pairs described below are separate object references;
they must not be counted as authored WLD directions or proof of live travel.

| Source boundary | Target owner found in this checkout | Source interpretation |
| --- | --- | --- |
| #30300 up to #1126542 ([exit](../../../areas/wld/bb_gh.wld#L14)) | Unregistered ud_map2 ([room](../../../areas/wld/ud_map2.wld#L80518)) | ud_map2 #1126542 has a down exit to #30300 at line 80541. This is one reciprocal source-only numeric pair; #30300 is also defined by unregistered vodgh and venom. |
| #30320 up to #111255 ([exit](../../../areas/wld/bb_gh.wld#L346)) | Active tharnrifts; the room number also occurs in unregistered Duris3 | This is the sole exit target resolved by the current full-world WLD set and the one bb_gh source-only approach already included in the eligible recount. |
| #30330 up to #123743 ([exit](../../../areas/wld/bb_gh.wld#L509)) | Unregistered Duris3 ([room](../../../areas/wld/Duris3.wld#L498604)) | Source target exists, but it is not active WLD ingress. |
| #30340 up to #140954 ([exit](../../../areas/wld/bb_gh.wld#L672)) | Unregistered Duris3 ([room](../../../areas/wld/Duris3.wld#L860035)) | Source target exists, but it is not active WLD ingress. |

No active WLD exit targets a VNUM uniquely attributable to bb_gh; the
candidate-specific active inbound count is zero. The ud_map2 reciprocal pair
is not active inbound, and its destination #30300 is numerically ambiguous
among three unregistered room sources. The raw zone titles “VoDsGH” and
“Venom GH” are distinct from bb_gh’s “BB GH.” The active #111255 target does not
make the source room #30320 active. The all-candidate eligible inactive-WLD
approach total remains 11,987 overall / 415 from Surf because this
#30320-to-#111255 direction was already counted at Checkpoint 386. These
source records do not change that total.

## Portals and compiled behavior

Objects #30302–#30309 are eight local type-25 teleport objects. Their fixed
destination value is a local room VNUM, their command value is 7
(CMD_ENTER), and their value[2] is -1. ZON places each object in the map pocket
opposite its target, forming four reciprocal pairs:

| Object placed by ZON | Room | Fixed target | Reciprocal object | Room | Fixed target |
| --- | ---: | ---: | --- | ---: | ---: |
| #30302 | #30307 | #30313 | #30303 | #30313 | #30307 |
| #30304 | #30308 | #30323 | #30307 | #30323 | #30308 |
| #30305 | #30303 | #30333 | #30306 | #30333 | #30303 |
| #30309 | #30306 | #30343 | #30308 | #30343 | #30306 |

The code defines ITEM_TELEPORT as type 25 and CMD_ENTER as 7; command dispatch
calls check_item_teleport, which checks the object type/command and resolves
its fixed room VNUM before teleporting
([type](../../../src/core/defines.h#L140),
[command](../../../src/cmd/interp.h#L42),
[dispatch](../../../src/cmd/interp.c#L2410),
[handler](../../../src/magic/spell_travel.c#L932)). This source/code match
supports the intended pairing, but does not establish that bb_gh is loaded,
that these rooms win numeric ownership, or that a live character can use the
routes.

The source also has a direct numeric room callback:
specs.assign.c binds room #30303 to inn at line 2455
([assignment](../../../src/specs/specs.assign.c#L2455)). The inn procedure
handles player rent requests and checks membership, combat, and other
conditions ([procedure](../../../src/specs/specs.room.c#L243)); this is not
evidence that #30303 is currently loaded as a bb_gh room.

## Resets, shop, and typed references

The ZON contains 21 M, 15 O, and 25 G commands. Six O commands import Player
Castles objects: #11057 in rooms #30300, #30310, #30320, and #30330; #11107 in
#30304; and #11013 in #30305. The local O commands place the pool, mushroom
pouch, and eight portals. G commands include a local mushroom pouch and
external gear products. Current object definitions place #3050 in active
Verz1 and #111, #108, #109, #377, #384, #385, #412, and #491–#494 in active
Heavens. Object #53 also has definitions in Heavens and unregistered Dalvik,
Hostel, and Mavulsk; #108 and #109 also occur in unregistered Mavulsk.
No current object prototype for #6089 was found, though it appears in both
the reset and shop source. Source presence does not prove reset availability.

The standalone shop record begins at key #30301, uses keeper mobile #30301
and room #30302, and lists fourteen sale items: #3050, #111, #109, #108, #377,
#53, #6089, #412, #385, #30301, and #491–#494
([record](../../../areas/shp/bb_gh.shp#L1)). The #30301 product is the local
mushroom pouch; #6089 has no current object prototype. The aggregate
areas/world.shp is absent in this checkout. The shop loader falls back to
areas/shp/*.shp ([loader](../../../src/economy/shop.c#L3267)); unregistered
vodgh.shp contains a second distinct shop record with the same key and
keeper VNUM #30301. Do not merge these records or infer which shop assignment
is available from the source files alone.

The local-number collisions are all among unregistered sources:

| Typed records | Other source definitions overlapping bb_gh |
| --- | --- |
| Rooms #30300–#30343 | vodgh defines #30300–#30313; venom defines #30300–#30309 |
| Mobiles #30300–#30331 | vodgh and venom each define #30300–#30302 |
| Objects #30300–#30309 | vodgh defines #30300 and #30301 |
| Shop key / keeper #30301 | vodgh.shp has its own separate #30301 record |

No active WLD room, mobile, or object definition at these exact local VNUMs
was found. These overlaps do not prove that bb_gh, vodgh, and venom are
alternate drafts or replacements. No local QST contract, external QST item
command, story item-VNUM consumer, or active external reset-role consumer of
bb_gh local VNUMs was found. The bb_gh source itself has no QST file or
registered area membership.

## Title history and disposition

Normalized title “BB GH” is unique among 442 current ZON titles. Header #1
is shared by several current zones and is not a unique identity. Followed
same-stem histories for the five files reach the 2009 areas-tree move; WLD
has a 2013 edit and 2026 ANSI fix, OBJ has a 2016 edit, and ZON/SHP have 2013
and 2026 edits. The reviewed histories do not establish an alternate-stem
predecessor or whole-zone replacement. They also do not resolve the
bb_gh/vodgh/venom ownership questions.

Keep candidate row 14 RETAIN IN PLACE. Resolve the intended guild identity,
numeric room and prototype ownership, four portal destinations, shared SHP
key, missing object #6089, and tool/editor/persisted-state closure before any
manifest, archive, or path change. See
[story dossier](../zone-stories/BB_GH.md),
[builder questions](../zone-stories/BB_GH_BUILDER_QUESTIONS.md),
[candidate row 14](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L14),
[vodgh row 118](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L118),
[venom row 117](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L117), and
[coverage row 365](ZONE_COVERAGE_REGISTER.csv#L365).

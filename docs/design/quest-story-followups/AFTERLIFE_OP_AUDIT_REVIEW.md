# Afterlife GC OP audit review

Checkpoint 442 is a static review of unregistered `afterlife_op`, including
its source graph, empty MOB placeholder, editor map, reset ownership, type-25
portal targets, and same-number room collisions with active Torg and the
separate `void_gh` candidate. It does not inspect live loading, editor
databases, persisted world state, or runtime traversal. The [source dossier](../zone-stories/AFTERLIFE_GC_OP.md)
summarizes the authored passage; the [builder review](../zone-stories/AFTERLIFE_GC_OP_BUILDER_QUESTIONS.md)
records owner decisions.

## Inventory and source membership

| Source | Current evidence |
| --- | --- |
| `areas/wld/afterlife_op.wld` | Three rooms, #29110–#29112. |
| `areas/mob/afterlife_op.mob` | Tracked, zero-byte file; no mobile prototypes. |
| `areas/obj/afterlife_op.obj` | Two type-25 teleport objects, #29110 and #29111. |
| `areas/zon/afterlife_op.zon` | Header #1, title “Afterlife GC OP,” reset mode 0; local and imported reset references below. |
| `areas/map/afterlife_op.json` | Editor positions for all three room VNUMs, `zoneId` `afterlife_op`, `lastModified` 2025-12-02. |
| QST / SHP | No `afterlife_op.qst` or `afterlife_op.shp` file exists. |

The component is not listed in full `areas/AREA` or `areas/qst/AREA`, is not
in `areas/RANDOM_AREA`, and has no existing story sidecar or dossier. The
current full area list contains `torg *289-291` ([index](../../../areas/AREA#L140));
it does not list `afterlife_op` or `Duris3`. The source's ZON header #1 does
not register it. Preserve the map JSON as a separate editor source asset.
See [coverage row 355](ZONE_COVERAGE_REGISTER.csv#L355) and [candidate row 4](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L4).

## Local graph and outward boundary

The WLD contains five direction records: four local and one outward. The
local edges form one weak component, and traversal from #29110 reaches all
three rooms. There is no direct active WLD inbound edge to #29110, #29111, or
#29112. The single outward D4 from #29112 targets #157174, titled “The Ocean”
in the separate `Duris3.wld` source ([target](../../../areas/wld/Duris3.wld#L1200655)).
`Duris3` is not in the current full area list. The Ocean room's four exits
target other Ocean rooms and none points directly back to #29112. The target
record therefore does not prove a loaded route or reciprocal return.

| Source room | Afterlife OP title | Direction records |
| --- | --- | --- |
| #29110 | A Portal Room ([source](../../../areas/wld/afterlife_op.wld#L1)) | D1 → #29111 |
| #29111 | A Large Circular Stone Chamber ([source](../../../areas/wld/afterlife_op.wld#L16)) | D2 → #29112; D3 → #29110 |
| #29112 | Before a Smooth Stone Tunnel ([source](../../../areas/wld/afterlife_op.wld#L33)) | D0 → #29111; D4 → #157174 |

## Room-number and source identity conflicts

All three Afterlife OP room VNUMs are also defined by active `torg.wld` and
the separate unregistered `void_gh.wld`. Their source titles and descriptions
differ; these are numeric ownership collisions, not matching maps or proof of
replacement.

| VNUM | Afterlife OP | Active Torg | Unregistered `void_gh` |
| --- | --- | --- | --- |
| #29110 | A Portal Room ([OP](../../../areas/wld/afterlife_op.wld#L1)) | Compact Soldier's Barracks ([Torg](../../../areas/wld/torg.wld#L4489)) | Down a Dark Chasm ([void](../../../areas/wld/void_gh.wld#L209)) |
| #29111 | A Large Circular Stone Chamber ([OP](../../../areas/wld/afterlife_op.wld#L16)) | Small Barracks In The City ([Torg](../../../areas/wld/torg.wld#L4505)) | A Radiant Passage ([void](../../../areas/wld/void_gh.wld#L222)) |
| #29112 | Before a Smooth Stone Tunnel ([OP](../../../areas/wld/afterlife_op.wld#L33)) | Some Higher Order Barracks ([Torg](../../../areas/wld/torg.wld#L4521)) | A Shimmering Portal of Light ([void](../../../areas/wld/void_gh.wld#L238)) |

The normalized `.zon` title “Afterlife GC OP” occurs twice among 442 current
zone titles: here and on the distinct `afterlife_op2` candidate. OP2 uses
#29120–#29128, a different map range. Followed WLD, OBJ, and ZON history
remains on the `afterlife_op` stem through the 2009 tree move and later
same-stem edits; the editor JSON is a separate 2025 asset. The tracked empty
MOB file has earlier same-stem history. No alternate-stem rename or explicit
whole-zone replacement is established. The shared title and zone header #1
do not join OP and OP2 into one map.

The empty OP MOB file declares none of the four mobiles placed by its ZON.
The referenced #29101–#29104 prototypes are present in
`afterlife_gh.mob` ([#29101](../../../areas/mob/afterlife_gh.mob#L16),
[#29102](../../../areas/mob/afterlife_gh.mob#L33),
[#29103](../../../areas/mob/afterlife_gh.mob#L54),
[#29104](../../../areas/mob/afterlife_gh.mob#L75)); the same numeric mobile
IDs also occur in separate unregistered `scrubs_hall`, `solstice_gh`,
`void_gh`, and `wlust_gh` MOB files. No active Torg mobile owns those IDs.
Keep the reset source and competing prototype owners distinct.

| Reset VNUM | Current same-type MOB source declarations |
| --- | --- |
| #29101 | [`afterlife_gh`](../../../areas/mob/afterlife_gh.mob#L16), [`scrubs_hall`](../../../areas/mob/scrubs_hall.mob#L12), [`solstice_gh`](../../../areas/mob/solstice_gh.mob#L16), [`void_gh`](../../../areas/mob/void_gh.mob#L14), [`wlust_gh`](../../../areas/mob/wlust_gh.mob#L12) |
| #29102 | [`afterlife_gh`](../../../areas/mob/afterlife_gh.mob#L33), [`scrubs_hall`](../../../areas/mob/scrubs_hall.mob#L23), [`solstice_gh`](../../../areas/mob/solstice_gh.mob#L31), [`void_gh`](../../../areas/mob/void_gh.mob#L30) |
| #29103 | [`afterlife_gh`](../../../areas/mob/afterlife_gh.mob#L54), [`scrubs_hall`](../../../areas/mob/scrubs_hall.mob#L34), [`void_gh`](../../../areas/mob/void_gh.mob#L43) |
| #29104 | [`afterlife_gh`](../../../areas/mob/afterlife_gh.mob#L75), [`void_gh`](../../../areas/mob/void_gh.mob#L56) |

## Typed resets and portal targets

The OP ZON resets local object #29110 in room #29110, imported Player Castles
object #11053 in #29112, and mobiles #29104, #29103, #29102, and #29101 in
#29112. The four mobile records are absent from the local MOB file. Their
matching VNUM definitions occur in the unregistered `afterlife_gh.mob` source,
while several other unregistered candidates define some of the same IDs.
Object #11053 is
declared in `player_castles.obj` as a magic mouth ([prototype](../../../areas/obj/player_castles.obj#L145)).
In the other direction, `afterlife_gh.zon` imports object #29111 from
`afterlife_op.obj` into room #29106 ([reset](../../../areas/zon/afterlife_gh.zon#L12)).
These are source-only candidate dependencies, not active resets.

| Object | Source values | Description text | Static target identity |
| --- | --- | --- | --- |
| #29110 | Type 25; `value[0]` #29106; `value[1]` 7 ([record](../../../areas/obj/afterlife_op.obj#L1)) | Khomani-Kan | #29106 is Afterlife Guildhall's Hanging Garden and active Torg's Shop of Gornogo. |
| #29111 | Type 25; `value[0]` #29110; `value[1]` 7 ([record](../../../areas/obj/afterlife_op.obj#L9)) | Goodie Continent | #29110 is OP's Portal Room, active Torg's barracks, and a separate `void_gh` room. |

`ITEM_TELEPORT` is type 25 ([definition](../../../src/core/defines.h#L140));
the generic travel code reads `value[0]` and resolves it with `real_room`
([target handling](../../../src/magic/spell_travel.c#L991)). The two stored
targets therefore identify colliding room numbers in current source, not
named Khomani-Kan or Goodie Continent destinations. Both objects set
`value[1]=7`, which the command table defines as `CMD_ENTER`
([command ID](../../../src/cmd/interp.h#L42)). No direct custom code
binding to room VNUMs #29110–#29112 was found in the bounded literal scan.
No competing current `.obj` prototype with local IDs #29110 or #29111 was
found; their target-room collisions are in the WLD namespace.
No current QST refers to those room VNUMs; OP has no local QST or shop source.
Editor database, saved player/object references, and live object presence
remain uninspected.

## Disposition

Keep existing candidate `afterlife_op` **RETAIN IN PLACE**. Preserve its
three-room WLD, tracked empty MOB file, two teleport objects and their exact
numeric values, map editor JSON, Player Castles object import, and
`afterlife_gh` mobile dependency. Resolve the active Torg and `void_gh` room
collisions, the `afterlife_op`/`afterlife_op2` identity, and portal target
ownership before any move or source merge. The `Duris3` target, mobile
prototype collisions, and editor/persisted/runtime closure also remain open.
See [candidate row 4](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L4),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-442-afterlife-op-source-graph-and-portal-boundaries),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-442-afterlife-op-and-vnum-ownership-conflicts),
and [retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-442-afterlife-op-retains-its-source-identity).

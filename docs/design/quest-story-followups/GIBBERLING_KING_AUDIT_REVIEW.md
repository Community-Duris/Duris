# Lair of the Gibberling King: shared entrances, inactive approach, and typed references

**Checkpoint 117 · 2026-10-09 · active source `gibber`, zone 87.** This repository-wide supplement adds graph, inactive-map, typed-reference, title/history, and retention evidence to the [comprehensive Lair of the Gibberling King story dossier](../zone-stories/LAIR_OF_THE_GIBBERLING_KING.md). It does not change gameplay or move source files.

Sources: [`areas/AREA`](../../../areas/AREA#L49), [`gibber.wld`](../../../areas/wld/gibber.wld#L1), [`gibber.zon`](../../../areas/zon/gibber.zon#L151), [`gibber.obj`](../../../areas/obj/gibber.obj#L610), [`gibber.qst`](../../../areas/qst/gibber.qst#L1), [`gibber.story.json`](../../../areas/story/gibber.story.json#L1), [active cross-area exits](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L180), [cross-area pair summaries](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L153), [coverage row](ZONE_COVERAGE_REGISTER.csv#L104), and [`ud_map2` candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114).

## Active connectivity and directed source reach

The active `gibber.wld` defines 122 rooms and 406 exits. All 406 targets resolve in the active WLD set: 404 exits stay within Gibber and two leave it. The local undirected projection is one weak component containing all 122 rooms. There are no negative or unresolved targets, numeric-range mismatches, or current fields; room #8700 has the one fall-chance field.

Two exits from entrance #8700 connect reciprocally with Underdark: north to #820318 and west to #820717. The corresponding Underdark south and east exits return to #8700. See Gibber outbound rows [180–181](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L180), Underdark inbound rows [1739–1740](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1739), and pair summaries [153](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L153) and [703](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L703).

Realm room #14209 also exits northeast directly to Gibber room #8788. It is a separate one-way active inbound route with no reverse record; see [Realm exit row 256](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L256) and [pair row 392](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L392). The room is a distributor source, so its raw edge does not establish an ordinary mortal access path or current usability. No selected incoming type-25 declaration targets Gibber.

With authored doors and exits available, the raw directed passable-exit scan from #8700 reaches 110 rooms if the flagged boulder direction is excluded and 114 if it is allowed. The existing dossier separately records the meaningful key, SEARCH, PICK, and PUSH access tiers, including the Realm arrival option and eight rooms outside those tested paths. These counts do not qualify live access, hazard outcomes, reset stock, or safe travel, and do not make every internal room a player objective. See the dossier's [entrance, keys, switches, and alternate arrival](../zone-stories/LAIR_OF_THE_GIBBERLING_KING.md#entrance-keys-switches-and-alternate-arrival).

## Inactive-map approach

All 95 WLD files outside the active `areas/AREA` contributor set were scanned: 527,791 rooms and 2,080,773 exits, with no parse issues or source-room VNUM overlap with Gibber. One source-only approach was found: inactive [`ud_map2.wld` room #1125006 down to #8700](../../../areas/wld/ud_map2.wld#L59862). `ud_map2` is not selected by `areas/AREA`, so this is not active ingress.

This raises the curated source-only approach total from 1,530 to 1,531. The reviewed Surf subset remains 141. It is the eighth source-only `ud_map2` approach into active areas; preserve all eight routes and the separate unresolved active Shafts reference while map ownership and dependencies remain open. Keep `ud_map2` **RETAIN**; see its [candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114) and [coverage row](ZONE_COVERAGE_REGISTER.csv#L465). This WLD scan does not qualify code, transport, staff, or runtime arrival paths.

## Typed references and dependency closure

The bounded typed scans found these direct relationships:

| Reference | Source record | Classification |
| --- | --- | --- |
| Active outside QST item consumer | [`airp.qst:113`](../../../areas/qst/airp.qst#L113) asks for object #8735. [`airp.story.json`](../../../areas/story/airp.story.json#L528) types #8735 in `item_vnums`; the local [`gibber.obj` prototype](../../../areas/obj/gibber.obj#L610) is the living breeze boots. | Real active consumer of a local Gibber object. Mob #8735 is Roshen, a separate mobile-namespace prototype; the item input remains object #8735, not mob #8735. This is an outside Cloudseeker ingredient, not another native Gibber Q/QA return. |
| Active reset imports | [`gibber.zon:151`](../../../areas/zon/gibber.zon#L151) loads object #359; [`gibber.zon:417`](../../../areas/zon/gibber.zon#L417) loads object #55190 on Crymson. | #359 is an active Heavens object prototype ([definition](../../../areas/obj/heavens.obj#L3725)). #55190 is the active Winterhaven memory object ([definition](../../../areas/obj/wh.obj#L2460)); Winterhaven's [`wh.qst:3204`](../../../areas/qst/wh.qst#L3204) consumes it and the matching [story-sidecar request](../../../areas/story/wh.story.json#L1048) types the item. Preserve both imported reset dependencies and the separate outside receipt. |
| Numeric reward, not an item | [`surfacekeeps.qst:393`](../../../areas/qst/surfacekeeps.qst#L393) contains `R E 8750`. | The quest loader maps `R E` to an experience reward ([`quest.c:2260`](../../../src/world/quest.c#L2260)); #8750 is not an object dependency. Its story-sidecar `E:8750` repeats the same reward amount. |
| Dormant direct-code number | [`randomeq.c:153`](../../../src/item/randomeq.c#L153) lists #8746 as Crymson in `highdrop_mobs`. | The only matching consumer loop is inside a block comment at [`randomeq.c:649–667`](../../../src/item/randomeq.c#L649). This is a dormant source literal, not an active code binding. |
| Raw-number false positives | [`memorize.c:79`](../../../src/classes/memorize.c#L79) contains decimal `.8710` in a square-root table. `alatorin.story.json` hits #8762 and #8736 occur in `id` strings; the `wh.story.json` #8704 hit is also an identifier suffix. | These are numeric text matches, not typed room, mobile, or object references. |

No external active ZON reset-role consumer, SHP product, or duplicate active MOB/OBJ prototype targeting Gibber was found. The only external typed QST item consumer of a local object is Airp's #8735 request; the other exact QST match is the non-item experience reward above. Local QST item commands and local sidecar `item_vnums` remain within Gibber prototypes. The exact sidecar number scan found nine hits in four files: Airp's four #8735 tokens include the typed item and its workflow/completion strings; SurfaceKeeps has two `E:8750` experience-reward strings; Alatorin has two identifier suffixes; and Winterhaven has one #8704 request-ID suffix. Of 231 external story sidecars, all parsed without errors and only `airp.story.json` has a typed `item_vnums` entry matching a local Gibber object. Among 872 external type-25 declarations, none targets a Gibber room. Gibber has no local `.shp` file. Local resets also use imported Heavens object #359 and Winterhaven memory #55190; other local reset-role references stay within this area's source prototypes. These are bounded direct scans, not exhaustive runtime, editor/tool, indirect-code, or persisted-player closure.

## Title, duplicate, and replacement history

After Duris color-code removal and normalization, “Lair of the Gibberling King” has one exact owner among 351 non-empty active `.zon` titles: `gibber`. The title scan found no separate active lexical candidate containing “Gibber.” The active `.zon` title and `areas/AREA` membership remain tied to the current source key.

Followed `gibber.wld`, `.zon`, `.mob`, `.obj`, and `.qst` histories stay under the same stem after the 2009 repository-wide `areas/` move (`642da5400`), with same-path edits. The story sidecar was added on 2026-10-06 (`2f7e343bb`). No alternate-stem rename, distinct predecessor, or explicit replacement claim appears in these followed paths; this does not establish the absence of an untracked predecessor.

Preserve active `gibber` as its own current identity. It is an uncommented `areas/AREA` contributor and remains **DO NOT MOVE**. Preserve the source-only `ud_map2` approach and keep that candidate **RETAIN** pending map ownership and full dependency/tooling closure.

## Scope and remaining qualification

The [coverage row](ZONE_COVERAGE_REGISTER.csv#L104) links this supplement and the comprehensive story dossier. It records the active graph and boundaries, one inactive `ud_map2` approach, outside typed item use, imported reset objects, unique title/history result, and retention disposition. Gameplay qualification, alternate-route availability, chance-based item stock, actual source recovery, current door state, fall safety, active accounting, editor/tooling references, indirect code paths, and persisted-player references remain outside this static checkpoint. No source, area placement, or gameplay behavior changed.

## Checkpoint 806: current typed-dependency census

The local Gibber reset has 366 typed operands: 246 mobile operands (216 M and
30 F) and 120 object operands (34 O, ten P slots, eight G, and 68 E). All 246
mobile operands resolve locally. Of 120 object operands, 118 resolve locally;
the other two are active imports: Heavens object #359 and Winterhaven memory
#55190. No reset operand is unresolved, and no outside active or source-only
ZON reset role consumes a Gibber prototype. The reset uses 55 of 56 local
mobiles and 38 of 41 local objects; mobile #8750 and objects #8709/#8718/#8736
are not reset-used, while #8709 and #8718 are QST outputs.

The two native QST contracts have five local item fields: G I #8706/#8717/#8734
and R I #8709/#8718. Their item and giver roles match the local story sidecar.
Airp has the only outside typed item consumer: its QST and story sidecar request
Gibber object #8735. Mobile #8735 is Roshen, a separate prototype namespace;
the QST/sidecar item role is the living-breeze object #8735. SurfaceKeeps'
`R E 8750` is an experience amount, not an object. No local SHP or outside
shop product/keeper reference to Gibber was found. Winterhaven's QST and
sidecar separately consume the imported memory #55190 reset in Gibber.

Gibber has two local type-29 ITEM_SWITCH objects. Dung pile #8708 stores
CMD_ENTER (7), target room #8713, and down direction 5; boulder #8716 stores
CMD_PUSH (270), target room #8784, and north direction 0. Both targets are
local rooms, and the switch handler treats value[1] as the room containing the
exit to change. There is no local type-25 item, and no outside type-25 or
type-29 object targets a Gibber room. The global source census remains 872
type-25 and 380 type-29 records. Four local WLD key fields use #8739 on the
#8701/#8702 reciprocal pair and #8715 on the #8727/#8728 pair; no outside WLD
key field targets a Gibber object.

An exact compiled numeric scan found the dormant `randomeq.c` #8746 table
entry for Crymson; its only matching consumer loop is commented out. The
`.8710` token in `memorize.c` is a decimal table value, not VNUM #8710. No
other direct compiled mobile, object, or room VNUM binding was found. Preserve
the existing Airp request, imported reset ownership, local key/switch
semantics, active boundaries, and source-only `ud_map2` approach. Live access,
random stock, runtime item use, editor/tooling, and persisted-player closure
remain unqualified.

See the [local reset operands](../../../areas/zon/gibber.zon#L151),
[Heavens import](../../../areas/obj/heavens.obj#L3725),
[Winterhaven import](../../../areas/obj/wh.obj#L2460),
[local QST items](../../../areas/qst/gibber.qst#L5),
[local sidecar contracts](../../../areas/story/gibber.story.json#L97),
[Airp QST consumer](../../../areas/qst/airp.qst#L113),
[Airp sidecar item](../../../areas/story/airp.story.json#L528),
[shared VNUM object](../../../areas/obj/gibber.obj#L610),
[Roshen mobile](../../../areas/mob/gibber.mob#L549),
[SurfaceKeeps experience reward](../../../areas/qst/surfacekeeps.qst#L393),
[dung-pile switch](../../../areas/obj/gibber.obj#L144),
[boulder switch](../../../areas/obj/gibber.obj#L268),
[item-switch value roles](../../../src/specs/specs.object.c#L302),
[WLD key pairs](../../../areas/wld/gibber.wld#L35),
[dormant direct-code entry](../../../src/item/randomeq.c#L153),
[commented consumer](../../../src/item/randomeq.c#L649), and the
[coverage row](ZONE_COVERAGE_REGISTER.csv#L104).


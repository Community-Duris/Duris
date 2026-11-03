# Afterlife Guildhall audit review

Checkpoint 441 is a static review of the unregistered `afterlife_gh` source
component, including its lore QST, editor map, local graph, exact source
references, and active Torg room-number overlap. It does not inspect live
loading, player access, editor databases, persisted world state, or runtime
behavior. The [source dossier](../zone-stories/THE_AFTERLIFE_GUILDHALL.md)
summarizes its authored story; the [builder review](../zone-stories/AFTERLIFE_GUILDHALL_BUILDER_QUESTIONS.md)
keeps owner decisions open.

## Source inventory and loader membership

The source set has ten WLD rooms (#29100–#29109), five MOB prototypes
(#29100–#29104), three local OBJ prototypes (#29100–#29102), a ZON file headed
by #1 with title “The Afterlife Guildhall,” a one-giver QST (#29101), and one
SHP record (#29100). The `areas/map/afterlife_gh.json` editor asset contains
positions for every WLD room and records a `lastModified` date of 2025-12-01.
The QST file contains `M` responses but no `Q` or `QA` contract.

Neither `afterlife_gh` nor its QST file is listed in the full `areas/AREA` or
`areas/qst/AREA` indexes. The current area list registers active `torg` for
range *289–291. It does not list `afterlife_gh`; the source's zone #1 header and
the QST file do not make it active. There is no story sidecar or random-area
membership. The distinct unregistered `afterlife_op` and `afterlife_op2`
sources are existing attic candidates rows 4 and 5, and their ZON files also
use header #1. Preserve those separate files and their object ownership.
See the [area index](../../../areas/AREA#L140), [QST index](../../../areas/qst/AREA#L130),
[Afterlife ZON](../../../areas/zon/afterlife_gh.zon#L1),
[editor map](../../../areas/map/afterlife_gh.json), and [coverage row 354](ZONE_COVERAGE_REGISTER.csv#L354).

## Authored map and Torg VNUM overlap

The local map has 19 direction records: 18 target another room in the local
WLD and one points up (`D4`) from #29100 to #123813. That target is titled
“Rolling Hills” in `Duris3.wld`; its presence there does not establish an active
route. The local edges form one weak component, and directed traversal from
#29100 reaches all ten rooms. The candidate register records zero active WLD
inbound edges to the unregistered component. The target's loader status and
return path remain unresolved.

All ten Afterlife room VNUMs are also defined in active `torg.wld`. Their
same-numbered titles and map roles differ:

| VNUM | Afterlife Guildhall source title | Active Torg source title |
| --- | --- | --- |
| #29100 | Before a Wooden House ([Afterlife](../../../areas/wld/afterlife_gh.wld#L1)) | A Small Hobgoblin Home ([Torg](../../../areas/wld/torg.wld#L4324)) |
| #29101 | Entrance to the House of AftwerlLife ([Afterlife](../../../areas/wld/afterlife_gh.wld#L38)) | A Small Hobgoblin Home ([Torg](../../../areas/wld/torg.wld#L4340)) |
| #29102 | Top of the Flight of Stairs ([Afterlife](../../../areas/wld/afterlife_gh.wld#L67)) | Some Higher Order Barracks ([Torg](../../../areas/wld/torg.wld#L4356)) |
| #29103 | Entrance to the Basement ([Afterlife](../../../areas/wld/afterlife_gh.wld#L91)) | A Roughly Built Escape Tunnel ([Torg](../../../areas/wld/torg.wld#L4373)) |
| #29104 | The Food Storage Room ([Afterlife](../../../areas/wld/afterlife_gh.wld#L119)) | A Curve In The Escape Tunnel ([Torg](../../../areas/wld/torg.wld#L4390)) |
| #29105 | Behind a Huge Crystal Window ([Afterlife](../../../areas/wld/afterlife_gh.wld#L133)) | A Dead End In The Escape Tunnel ([Torg](../../../areas/wld/torg.wld#L4412)) |
| #29106 | The Hanging Garden ([Afterlife](../../../areas/wld/afterlife_gh.wld#L151)) | The Shop Of Gornogo ([Torg](../../../areas/wld/torg.wld#L4426)) |
| #29107 | The Small Gift Shop ([Afterlife](../../../areas/wld/afterlife_gh.wld#L166)) | Some Higher Order Barracks ([Torg](../../../areas/wld/torg.wld#L4441)) |
| #29108 | The Room of Prayers ([Afterlife](../../../areas/wld/afterlife_gh.wld#L182)) | Small Barracks In The City ([Torg](../../../areas/wld/torg.wld#L4458)) |
| #29109 | The Room of Exercises ([Afterlife](../../../areas/wld/afterlife_gh.wld#L196)) | Small Barracks In The City ([Torg](../../../areas/wld/torg.wld#L4473)) |

The repository-wide Torg audit independently found these ten of the 97
same-number inactive-WLD room records and reports that their titles differ
from Torg's. This is an identity collision, not a matching map or established
replacement. The current Torg source owns the listed VNUMs under `areas/AREA`;
the Afterlife WLD is unregistered. A compiled Players Guild block also
assigns `inn` to `real_room0(29103)` ([code](../../../src/specs/specs.assign.c#L2448)).
That number is the active Torg escape tunnel and the unregistered Afterlife
basement entrance; the code alone does not authorize either source to move.
See [Torg's inactive-WLD audit](TORG_AUDIT_REVIEW.md#inactive-wld-sources-and-the-surf-candidate)
and [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-441-afterlife-guildhall-map-and-torg-overlap).

## Story and identity history

The source places a multi-faith, reincarnation-focused household in a house
between travels. Suse's QST dialogue gives that lore but has no `Q`/`QA`
contract, reward, or stated state transition. The rooms add household scenes,
a universal altar, a suspended garden, food storage, a gift shop, and a padded
training room. The source does not prove working resurrection, god-brawls,
live conversation access, or a route into the active world. The
[dossier](../zone-stories/THE_AFTERLIFE_GUILDHALL.md) preserves the narrative
without supplying those mechanics.

The normalized title “The Afterlife Guildhall” is unique among all 442 current
`.zon` titles. Followed WLD, MOB, OBJ, ZON, QST, and SHP histories remain on the
`afterlife_gh` stem through the 2009 area-tree move and later same-stem edits.
The map JSON is a separate 2025 editor asset. No alternate-stem rename or
explicit whole-zone replacement is established. `afterlife_op` and
`afterlife_op2` have distinct VNUM sets and are source owners for four objects
reset by this guildhall; their shared `Afterlife GC OP` title and header #1 do
not make them copies of the Guildhall. Several other unregistered guildhall
sources also reuse parts of the #29100 room/mobile/object namespace. See the
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-441-afterlife-guildhall-and-active-torg-identity).

## Typed reset, QST, shop, and code references

The local ZON places four objects owned by Player Castles: #11053, #11005,
#11103, and #11013. It also imports #29111 from `afterlife_op.obj` and #29120,
#29124, and #29126 from `afterlife_op2.obj`. Those are source-only candidate
dependencies. Local reset commands place all three local food OBJ prototypes
on merchant #29100; the merchant's single SHP record sells those three plus
eleven imported object IDs. The shop products and merchant equipment share
IDs; the SHP contains fourteen products in total.

| Reset/shop IDs | Current source declarations |
| --- | --- |
| #11005, #11013, #11053, #11103 | `player_castles.obj` ([#11005](../../../areas/obj/player_castles.obj#L41), [#11013](../../../areas/obj/player_castles.obj#L121), [#11053](../../../areas/obj/player_castles.obj#L145), [#11103](../../../areas/obj/player_castles.obj#L265)) |
| #29111 | `afterlife_op.obj` ([object](../../../areas/obj/afterlife_op.obj#L9)); candidate row 4 |
| #29120, #29124, #29126 | `afterlife_op2.obj` ([#29120](../../../areas/obj/afterlife_op2.obj#L1), [#29124](../../../areas/obj/afterlife_op2.obj#L33), [#29126](../../../areas/obj/afterlife_op2.obj#L49)); candidate row 5 |
| #3050 | `verz1.obj` ([object](../../../areas/obj/verz1.obj#L850)) |
| #111, #347, #364, #377, #398, #412 | `heavens.obj` |
| #108, #109 | `heavens.obj` and `mavulsk.obj` |
| #53 | `dalvik.obj`, `heavens.obj`, `hostel.obj`, and `mavulsk.obj` |
| #6089 | No current `.obj` anchor found |

No other current QST source has an exact numeric reference to these local
room, mobile, or object VNUMs. Other shop and prototype collisions occur in
unregistered guildhall sources; the Torg audit lists the room-overlap family.
The exact C/C++ search has the #29103 `inn` binding but no other exact
#29100–#29109 reference. The `areas/map` file is preserved as a current editor
asset; external editor databases, persisted state, and runtime resolution are
not inspected. The bounded exact-reference scan is not migration clearance.

## Disposition

Keep candidate `afterlife_gh` **RETAIN IN PLACE** on existing candidate row 3.
Preserve its local lore QST, source-only editor map, active-Torg room-number
collision, Players Guild `inn` assignment, Player Castles imports, sibling
candidate object imports, duplicate prototype IDs, and currently undeclared
#6089. Do not merge the source with Torg or `afterlife_op`/`afterlife_op2`.
See [candidate row 3](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L3),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-441-afterlife-guildhall-map-and-torg-overlap),
[identity](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-441-afterlife-guildhall-and-active-torg-identity),
and [retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-441-afterlife-guildhall-retains-its-current-candidate).

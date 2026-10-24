# Aeon Order Guildhall audit review

Checkpoint 440 is a static review of the unregistered `aeon_gh` source
component, its authored map and resets, typed object references, and its
relationship to the separate unregistered `genesis` component. It does not
qualify live loading, room access, shop behavior, editor aliases, persisted
world state, or runtime behavior.

## Source inventory and area-list status

`aeon_gh` has ten WLD rooms (#29900–#29909), two MOB prototypes (#29900 and
#29901), an empty OBJ file, a ZON file headed by zone #299 and titled “Aeon
Order Guildhall,” and one SHP record for merchant #29901. The reset file uses
reset mode 0. There is no local QST file or story sidecar. The only matching
entry in `areas/AREA` is the raw `*OPEN 299` annotation; neither `aeon_gh` nor
`genesis` is a full area-list entry or a `qst/AREA` entry. The open-range note
does not establish that either component is loaded. See the [area list](../../../areas/AREA#L144),
[Aeon zone](../../../areas/zon/aeon_gh.zon#L1), [source files](../../../areas/wld/aeon_gh.wld#L1),
and [existing candidate row 2](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L2).

## Authored story and questions left open

The WLD describes a glow-filled cave with a mix of menace and comfort, a
tapestried meeting room, a warm niche that looks almost like a home, corridors
lined with drifting mist, and storage rooms. Room #29906 is titled “A Portal
Room” and describes an indentation prepared for a portal, but its WLD has no
portal exit and the ZON does not reset a portal object there. This is authored
preparation text, not evidence of a working portal or destination.

The mobile file describes a massive mist golem as still and intimidating, and
places a merchant in service of the Aeon Order. The ZON resets four copies of
the golem in #29900 and one merchant in #29909. The merchant's shop has fifteen
products and the refusal text “I do not sell to your people!” The source does
not identify the group that message means, explain the guild's purpose, or
connect its portal-room text to a quest. The [builder review](../zone-stories/AEON_ORDER_GUILDHALL_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions)
preserves those as owner questions.

## Authored WLD graph and current-world boundary

The ten source rooms contain 23 direction records: 22 target other rooms in
this WLD, and one points outward from #29900 through D4 to #153244. The local
edges form one weak component, and local directed traversal from #29900 reaches
all ten rooms. The target room exists in `Duris3.wld` and is titled “The
Ocean.” Because `aeon_gh` is not in the full area list, its source edge is not
an active-world route. The existing candidate register reports zero active WLD
inbound edges to this component; that does not settle the loader status of the
Ocean source record. See [Aeon room #29900](../../../areas/wld/aeon_gh.wld#L1),
[Ocean room #153244](../../../areas/wld/Duris3.wld#L1118125), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-440-aeon-order-guildhall-source-only-map-and-exit),
and [candidate row 2](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L2).

## VNUM collision, title, and history

The normalized `.zon` title “Aeon Order Guildhall” is unique among all 442
current `.zon` titles. `genesis` has a different unique title, “genesis hall.”
The two components nevertheless claim the same zone number #299, all ten room
VNUMs #29900–#29909, and mobile VNUMs #29900 and #29901. Their records are
different: Aeon #29900 is a massive mist golem and #29901 is a merchant;
Genesis uses those same mobile numbers for gargoyles. Their WLD prose, reset
tables, and shopkeeper identities also differ. Both sources are absent from
the full area list, and both existing attic candidates remain separate at
[row 2](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L2) and
[row 46](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L46).

Compiled assignment code adds an unresolved owner reference: the “Players
Guild” block assigns `inn` to `real_room0(29903)` in
[`specs.assign.c`](../../../src/specs/specs.assign.c#L2452). Both candidate
maps define room #29903, with different descriptions. Static source evidence
does not establish which room the handler was intended for.

Followed histories remain on the separate `aeon_gh` and `genesis` stems. They
include the 2009 area-tree move and later same-stem edits; Aeon ZON/SHP and
Genesis MOB/OBJ/ZON have later maintenance commits. The reviewed history shows
no alternate-stem rename or explicit whole-zone replacement. The shared VNUMs
and open range require an owner decision; they do not establish that either
source supersedes the other. See [title and replacement review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-440-aeon-order-guildhall-and-genesis-vnum-collision).

## Typed reset, shop, and code references

The Aeon ZON places three objects owned by `player_castles`: #11057 in room
#29900, #11107 in #29902, and #11013 in #29903. Its four golem resets and
merchant reset are local. The merchant receives fifteen `G` object references;
the single local SHP record for #29901 lists those same fifteen numeric product
IDs. The local OBJ file is empty, so these are imported references, not local
object declarations. See the [resets](../../../areas/zon/aeon_gh.zon#L9),
[shop](../../../areas/shp/aeon_gh.shp#L1), and [empty OBJ file](../../../areas/obj/aeon_gh.obj).

The current object-source scan places #11013, #11057, and #11107 in
`player_castles`; #3050 in `verz1`; and #111, #347, #364, #377, #398, #412,
and #491–#494 in `heavens`. IDs #108 and #109 have definitions in both
`heavens` and `mavulsk`; #53 has definitions in `dalvik`, `heavens`, `hostel`,
and `mavulsk`. No current OBJ file defines #6089, although both the ZON and
SHP reference it. These source collisions and the missing current declaration
leave the intended loaded item identities unresolved. The genesis candidate
also imports Player Castles #11013 and shares much of the Aeon shop's stock.
Representative source anchors are [Player Castles #11013](../../../areas/obj/player_castles.obj#L121),
[Verz1 #3050](../../../areas/obj/verz1.obj#L850), [Heavens #53](../../../areas/obj/heavens.obj#L788),
[Heavens #108](../../../areas/obj/heavens.obj#L1582), [Mavulsk #108](../../../areas/obj/mavulsk.obj#L1798),
[Dalvik #53](../../../areas/obj/dalvik.obj#L681), and [Hostel #53](../../../areas/obj/hostel.obj#L695).

Outside the two unregistered candidate zone sources, the bounded exact-number
scan of current `.zon`, `.qst`, `.shp`, and 232 story-sidecar files found no
references to Aeon's room or mobile VNUMs. Genesis has its own reset uses of
the colliding IDs. The exact C/C++ search did find the #29903 `inn` assignment
described above. This review does not inspect editor databases, saved world
state, runtime indexes, or live stock resolution. See the [typed-reference details](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-440-aeon-order-guildhall-and-genesis-vnum-collision)
and [candidate disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-440-aeon-and-genesis-retain-their-current-sources).

## Disposition

Keep `aeon_gh` **RETAIN IN PLACE** on its existing candidate row. Its shared
room and mobile VNUMs with `genesis`, the compiled #29903 handler, imported
equipment and shop stock, unresolved #6089 definition, and uninspected editor
and persisted identities prevent a migration decision. Keep `genesis` as its
separate **RETAIN IN PLACE** candidate pending its own row review. Do not infer
which map owns the reserved range, merge either WLD, or treat the D4 source exit
as an active route. The twenty source-backed owner questions are in the
[builder review](../zone-stories/AEON_ORDER_GUILDHALL_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).


## Checkpoint 882: role-aware tracked-source closure

CP882 reconciles the exact-number scan with each source format's operand roles. Across
all 442 current ZON files, the only numeric room/mobile-role hits outside `aeon_gh.zon`
are eight commands in `genesis.zon`: three O resets place Player Castles objects in
Genesis rooms, and five M resets use Genesis mobiles #29900, #29901, #29902, or #29903
in Genesis rooms #29901/#29902. Two mobile VNUMs and all of these room numbers collide
with Aeon, but the commands are authored in Genesis and refer to its separate records.
No other ZON source has a role-matched Aeon room/mobile operand.

The only other current SHP record with a matching role operand is Genesis keeper #29902
in Genesis room #29902; the Aeon shop's keeper #29901 and room #29909 occur only in its
own record. No exact target-number line was found in 265 current QST files or 254 current
area JSON files. All 21,682 current OBJ prototypes parsed with the four-value record
layout; none has ITEM_TELEPORT value[0] or ITEM_SWITCH value[1] targeting Aeon rooms
#29900–#29909. Aeon has no local OBJ prototype. The only exact C/C++ number match remains
`world[real_room0(29903)].funct = inn`, whose intended map identity is unresolved.

A full current WLD recount found 18 numeric target matches in `genesis.wld`, all internal
Genesis edges. They do not establish inbound routes into Aeon's distinct map identity.
There is still no active WLD ingress. This census closes bounded tracked-source review
for the checked-in files; it does not resolve the competing #299 range, code handler
owner, editor aliases, loader/deployment status, saved state, or runtime behavior. Keep
Aeon and Genesis separate and **RETAIN IN PLACE**.

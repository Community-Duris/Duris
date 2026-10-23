# forofcon - Skull City and the Fortress of Conclusion source audit

Canonical key: component:forofcon. Physical coverage row 395 / CSV line
396; candidate dependency row 45 / CSV line 45. Reviewed October 11, 2026,
Checkpoint 482, against source snapshot 72ba3deff3bb30bde880018cdebe7c6db675b697.
This is a static source, connectivity, history, and typed-reference review. It
does not establish runtime loading, travel, quest availability, or permission
to activate or move these files.

## Source scope, title, and backups

The canonical source set contains [WLD](../../../areas/wld/forofcon.wld#L1),
[MOB](../../../areas/mob/forofcon.mob#L1),
[OBJ](../../../areas/obj/forofcon.obj#L1),
[ZON](../../../areas/zon/forofcon.zon#L1), and
[QST](../../../areas/qst/forofcon.qst#L1). The ZON header is #1, its exact
title is “Skull City and the Fortress of Conclusion,” top VNUM #140302, and
reset mode 2. That title occurs once among the current `.zon` files. The raw
AREA annotation is blank; forofcon is absent from full
[areas/AREA](../../../areas/AREA) and
[areas/qst/AREA](../../../areas/qst/AREA). No same-stem story sidecar or
editor-map file is present.

Four tracked editor artifacts accompany the canonical set:
[`.capmob`](../../../areas/mob/forofcon.mob.capmob#L1),
[`.capobj`](../../../areas/obj/forofcon.obj.capobj#L1),
[`.capqst`](../../../areas/qst/forofcon.qst.capqst#L1), and
[`.capzon`](../../../areas/zon/forofcon.zon.capzon#L1). The MOB, OBJ, and QST
backups are byte-identical to their canonical files. The ZON backup differs
only in its editor-version comment (`durisEdit v2.99de` versus `v3.00`). Keep
these artifacts as editor/provenance evidence; their presence does not add
another loaded record set.

The QST is story-bearing. It has 14 giver records and eight native Q
contracts. The dialogue ranges across Payvin's lost amulet and ship, Fred's
goblin-hunting job, Maganon and Desatysso's Mealdrun/Devourer investigation,
Sather's expedition, a mountain giant, a paladin and priest, and two Baldzor
demon records. The absence of a same-stem story sidecar does not make the
quest text disposable.

The quest parser stores `G` goals as offered items and `R` goals as rewards;
completion validates/removes the offered goals and grants the reward goals
([parser](../../../src/world/quest.c#L2195),
[completion](../../../src/world/quest.c#L168)). The eight contracts' item goals
are all typed references to the candidate OBJ set:

| Giver | Offered `G` item | Reward `R` item |
| --- | --- | --- |
| Payvin #139910 | #139907 amulet insignia | No `R I` item directive in this contract |
| Fred #139913 | #139911 head | #139910 coins object |
| Sather #139918 | #139927 wooden ring | #139908 shiny gold key |
| Elagor #139926 | #139929 emerald gem | #139902 metal pry bar |
| Elanor #139949 | #139948 libram | #139949 sun amulet |
| Bracos #139986 | #139968 gold ring | #139971 obsidian key |
| Baldzor #140040 | #140049 blade | #140048 crystal key |
| Baldzor #140049 | #140049 blade | #140048 crystal key |

All 14 QST giver VNUMs exist in candidate MOB, and each has an M placement in
the candidate ZON. All eight item-goal VNUMs resolve in candidate OBJ; the
candidate QST has no external item VNUM requirement. The two distinct
Baldzor giver records carry the same item exchange; preserve the duplicate
until the quest owner confirms intent. There is also typed same-number reuse,
including QST giver/mobile #139949 and OBJ sun amulet #139949.

## Candidate map graph and same-number room identities

The WLD defines 403 consecutive rooms #139900-#140302. Every room number is
outside header #1's conventional #100-#199 range, as are all 817 authored
exit records' source rooms. The exits form nine local weak components with
sizes 292, 63, 28, 14, 2, 1, 1, 1, and 1. Directed traversal from #139900
reaches 102 rooms. Of the 817 directions, 816 target another room in this
candidate WLD. The remaining record is #139900 D3 to negative target -1.
There are eight WLD fall-chance records and eight current records; these are
separate from the ZON's eight F reset commands.

All 403 candidate room VNUMs also occur in inactive
[Duris3 WLD](../../../areas/wld/Duris3.wld#L1), but every compared room block
and title differs. The first 53 candidate rooms #139900-#139952 also share
VNUMs with active [Smoke WLD](../../../areas/wld/smoke.wld#L1); their room
blocks and titles differ as well. Current active-table ownership assigns
those 53 numbers to Smoke. The remaining 350 forofcon room VNUMs have no
current active WLD owner. No active-WLD exit targets one of those 350 unique
forofcon room identities. Preserve the overlapping room sets as typed source
identities, not evidence of map replacement or candidate-identity ingress.

A direct parse of the current forofcon WLD finds 109 directions targeting
Smoke-owned room numbers: 104 originate in colliding #139900-#139952 source
IDs and five originate in non-overlapping source IDs. Those five already
curated source-only approaches are:

- #139953 D7 to #139952
- #139963 D3 to #139906
- #139966 D1 to #139917
- #139970 D5 to #139943
- #140020 D7 to #139941

Keep the five on candidate row 45 and do not add them again. Earlier CP364
Smoke notes said 111 total / 106 from colliding IDs; the current raw WLD has
817 D records and directly parses to 109 / 104. The unique-source five and
the curated total do not change. Curated inactive-WLD approach totals remain
11,987 overall / 415 from Surf.

## MOB, OBJ, QST, and ZON dependencies

The conventional MOB has 152 records #139900-#140051; OBJ has 165 records
#139900-#140064. A current all-source same-type scan found no other MOB or
OBJ prototypes with those candidate VNUMs. Candidate WLD, MOB, and OBJ reuse
many numbers across record types, so every dependency must retain its type.
The maintained source comment in [plushit.c](../../../src/item/plushit.c#L39)
identifies MOB #139930, #139975, and #140039 as PLUSFIVE examples. That is a
source-documentation reference, not a VNUM binding in executable code.

The candidate ZON has 766 commands: 386 M, 166 D, 57 O, 24 P, 67 G, 58 E,
and eight F, all under reset mode 2. Its mobile, object, container, and room
VNUM operands resolve within the candidate files; all 14 QST givers have M
placements. There is no same-stem SHP file. The reset source also contains 19
M placements into 13 room VNUMs in the Smoke overlap, 21 D resets into 16
such room VNUMs, and one O placement of candidate object #139928 into room
#139951. These are unregistered forofcon reset records, not active Smoke
mobile/object producers. Candidate and active Smoke ZON both contain a D
reset for shared-number room #139945 direction D2; keep both source records
and their owners distinct.

Current active ZON scans found no reset reference to the candidate MOB or
OBJ sets. Current active QST scans found no candidate item/giver reference,
and current SHP scans found no candidate product/keeper reference. The QST
and ZON closure described above is internal to forofcon. This is static
source closure; it does not establish a runtime route or that the unlisted
files load.

## History and disposition

The tracked MOB, OBJ, and WLD histories show a 2026 line-ending normalization.
The ZON path also shows a 2026 ANSI cleanup; the QST history reaches a 2015
quest-item-flag overhaul and the 2026 line-ending change. No alternate-stem
predecessor or explicit whole-component replacement appears in these bounded
paths. The byte-identical `.cap` backups and editor-version-only `.capzon`
difference are provenance clues, not replacement evidence.

Keep candidate row 45 **RETAIN IN PLACE**. Preserve the 403-room graph, its
Duris3 and Smoke same-number identities, all 817 exits including the negative
target, nine components, fall/current records, five already-counted
Smoke-directed source-only exits, all 766 reset commands, 14 givers, eight
Q contracts, three byte-identical editor backups, and the ZON editor-version
difference. Resolve the header/range fields, manifest and editor ownership,
shared Smoke room/reset coordinates, duplicate Baldzor exchange, dynamic
route, and saved-VNUM closure before any activation, move, rename, archive,
or merge proposal. See the
[24 builder questions](FOROFCON_BUILDER_QUESTIONS.md),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-482-forofcon-skull-city-and-fortress-map),
[identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-482-forofcon-map-identity-and-history),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-482-forofcon-retained-pending-map-and-quest-review),
[candidate row 45](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L45), and
[coverage row 395](ZONE_COVERAGE_REGISTER.csv#L396). No source-area files,
code, tests, builds, runtime, server, database, or containers were changed or
used.

## Checkpoint 894: role-aware Smoke and portal identity closure

The CP482 room and exit measurements remain unchanged: 403 rooms, 817 authored
directions, nine weak components, 102 rooms directed-reachable from #139900,
five unique-source directions to Smoke-numbered rooms already counted as
source-only approaches, and zero active inbound exits to the Forofcon-only
350-room range. No approach, graph, or curated inactive-WLD total is added by
this checkpoint.

### External typed source roles

The scan of all 441 other ZON files found 78 room operands in
`areas/zon/smoke.zon` (68 M, five F, five O) and four D room operands there,
at #139941, #139942 twice, and #139945. Each targets the active Smoke WLD
identity in the 53-room shared-number band, not the distinct Forofcon room
record. No external ZON mobile-prototype or object-prototype operand targets
the candidate MOB or OBJ ranges. The candidate's own 19 M, 21 D, and one O
Smoke-numbered placements remain records in its unregistered `forofcon.zon`;
they are not active Smoke resets. Both owners' D2 reset records at #139945
remain distinct source records. See the [Smoke resets](../../../areas/zon/smoke.zon#L9),
[Forofcon resets](../../../areas/zon/forofcon.zon#L716), and
[overlapping WLD records](../../../areas/wld/smoke.wld#L3443).

Across all 442 OBJ files, the two external type-25 teleport objects with
targets in the numeric candidate room band are active Smoke objects:
#139802 targets #139937 and #139819 targets #139941. Both destinations are
active Smoke rooms in the shared band, rather than the different Forofcon
records. No external type-29 switch room target enters the band. The 264
other QST files, 232 parsed story sidecars, 180 SHP shards' 712 parser-shaped
N records, and same-type MOB/OBJ definition scans found no external candidate
giver/item, story, shop, or prototype role. The eight Luiren legacy O-format
shop rows still prevent treating the current N-only shard scan as proof that
the fallback catalog fully parses. See [Smoke OBJ #139802 target](../../../areas/obj/smoke.obj#L27),
[Smoke OBJ #139819 target](../../../areas/obj/smoke.obj#L199), and [Smoke rooms](../../../areas/wld/smoke.wld#L4333).

### Candidate-owned teleports and switches

Forofcon defines 16 local type-25 teleport objects and four type-29 switches.
Each stored destination exists in candidate WLD, and each object has a
candidate-local ZON object reset. The destination role is separate from the
room in which the object itself is reset.

| Object role | Stored destination rooms |
| --- | --- |
| Type-25: #139978, #139989, #139992, #140013, #140014, #140015, #140019, #140028 | #140189, #140196, #140193, #140209, #140212, #140228, #140259, #140302 |
| Type-25: #140030, #140038, #140040, #140041, #140042, #140045, #140050, #140052 | #139903, #140263, #140263, #140263, #140287, #140286, #140301, #140302 |
| Type-29: #139994, #139995, #140034, #140039 | #140190, #140191, #140273, #140283 |

The #140030 destination #139903 collides with the active Smoke room identity;
Forofcon also defines a different room #139903. If the candidate object is
loaded while Smoke owns the current room table, the numeric target identifies
Smoke. If Forofcon's WLD is loaded and selected as the owner for #139903, the
target would identify its separate room; loader selection is not established.
This is a source-level identity condition, not evidence that the unregistered
object currently exists at runtime. The other 19 destinations are Forofcon
rooms with no active WLD owner. Object type 25 is `ITEM_TELEPORT` and type 29
is `ITEM_SWITCH`; see [object type constants](../../../src/core/defines.h#L140)
and the [switch room field](../../../src/specs/specs.object.c#L302). See also
[candidate object #140030](../../../areas/obj/forofcon.obj#L1907),
[candidate room #139903](../../../areas/wld/forofcon.wld#L69), and its
[Smoke counterpart](../../../areas/wld/smoke.wld#L3443).

### Compiled numeric consumers

The only exact Forofcon prototype-number match found by the C/C++ scan remains
the non-executable PLUSFIVE comment in `plushit.c`. A separate executable
room-number dependency is relevant: `olympus_portal` is assigned to active
Olympus objects #99801/#99803/#99805/#99807. Its #99801 case sets `base` to
#140000, then reads the zone bounds for `real_room0(base)` to choose a random
room. #140000 is defined by Forofcon and inactive Duris3, but not by current
active WLD source. `real_room0` returns index 0 when the room is absent, so
these source references do not establish which zone identity the active code
uses today. See [assignment](../../../src/specs/specs.assign.c#L1367),
[portal code](../../../src/specs/specs.olympus.c#L38), and
[`real_room0`](../../../src/world/db.c#L8480).

`random_encounters` also has executable comparisons using
`real_room(140000)` through `real_room(149999)` at two sites. #140000 is in
Forofcon and Duris3; #149999 occurs only in inactive Duris3 source. Since
`real_room` returns NOWHERE for an absent VNUM, current active-room behavior
and the intended range owner remain unresolved. A #140000-#159999 travel
block in `spell_travel.c` is fully commented out and is excluded from active
dependency findings. See [encounter checks](../../../src/mob/encounters.c#L151)
and [`real_room`](../../../src/world/db.c#L8517).

Keep candidate row 45 **RETAIN IN PLACE**. Preserve its authored WLD, resets,
and local teleports/switches; retain Smoke's active room, reset, and portal
identities separately. Resolve the #140030 destination collision, Olympus
base-room consumer, encounter-range owner, manifests, dynamic/editor paths,
and persisted IDs before any activation or path proposal. The
[24 builder questions](FOROFCON_BUILDER_QUESTIONS.md) now capture these
specific unresolved roles.

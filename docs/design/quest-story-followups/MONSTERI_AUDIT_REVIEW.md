# Monsteri source and story audit

Canonical key: component:Monsteri. Physical coverage row 422 / CSV line 423;
candidate dependency row 71 / CSV line 72. Reviewed October 11, 2026,
Checkpoint 509, against source snapshot
c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static source inventory,
narrative, WLD graph, typed-reference, identity, and bounded-history review.
It does not establish live availability, travel, combat, or reset outcomes.

## Inventory and environmental story

The unregistered source package has five same-stem paths:
[WLD](../../../areas/wld/Monsteri.wld#L1),
[MOB](../../../areas/mob/Monsteri.mob#L1),
[OBJ](../../../areas/obj/Monsteri.obj),
[ZON](../../../areas/zon/Monsteri.zon#L1), and
[QST](../../../areas/qst/Monsteri.qst). OBJ and QST exist but are zero-byte
files. The ZON header is #1000, its exact title is “CBL Guildhall Monster
Island,” its last room VNUM is #1004, and reset mode is 2. WLD defines five
rooms #1000–#1004; MOB defines four mobiles #1000–#1003. No SHP file or
story sidecar exists. Neither [areas/AREA](../../../areas/AREA) nor
[areas/qst/AREA](../../../areas/qst/AREA) lists Monsteri. No same-stem map
JSON is present under areas/map.

The room prose gives Monster Island a grim, overrun-gatehouse setting. Room
#1000 describes a bloodied approach, broken gates, scattered bodies, and
four undead beasts in a narrow bunker-like hall
([gatehouse](../../../areas/wld/Monsteri.wld#L1)). Room #1001 is a torture
chamber with wall shackles, a rack and table, painful implements, and fresh
and dried blood ([chamber](../../../areas/wld/Monsteri.wld#L24)). Room #1002
opens beside a huge island pit with crude creature drawings and stairs cut
into the far wall ([pit edge](../../../areas/wld/Monsteri.wld#L45)). Room
#1003 follows a narrow, unstable stairway spiraling down the wall, with no
bottom in sight ([stairway](../../../areas/wld/Monsteri.wld#L68)). Room #1004
is titled “The Portal Cave” and calls itself a ticket off the island, but its
only WLD direction returns south to #1003
([Portal Cave](../../../areas/wld/Monsteri.wld#L90)). The source does not
explain who captured the people, what lies at the bottom, or how the cave
provides passage off the island.

The four detailed mobile prototypes are a massive undead ghoul golem, a
decaying flesh golem, a putrid ape-like zombie golem, and a furious lich
golem. Their descriptions continue the grotesque, bloody horror tone. The
four source-only ZON M rows place one of each prototype in room #1000; this
matches the prose about four beasts, but is a reset declaration rather than
proof that the group is currently present. There is no native QST contract,
shop, or local object prototype. Treat this as authored environmental story,
not a completed quest or a verified portal service.

## WLD graph, range discrepancy, and current table

Monsteri has eight direction records with no malformed target tuple,
duplicate direction in a room, zero target, or negative target. Each target
stays within the candidate’s own #1000–#1004 set. The graph is one weak
component, and raw directed traversal from #1000 reaches all five rooms. No
F fall chance or current field was found. There is no candidate outbound
cross-area WLD edge.

The ZON header says #1000, but the source rooms #1000–#1004 fall in numeric
band 10 under the repository’s VNUM/100 range convention. All five rooms are
therefore outside the header’s numeric band, and all eight WLD directions
originate in range-mismatched rooms. Preserve this source discrepancy and
ask the owner to settle the intended numbering before any renumbering.

No active WLD room owns #1000–#1004, no active WLD direction targets them,
and the candidate register records zero active inbounds. All eight positive
candidate-local targets are consequently unresolved in the current active
room table. These are local Monsteri directions, not evidence of active
access. There are no cross-zone reverse records, fall fields, or current
fields. This graph result also matches the source-derived range and
connectivity values recorded for the companion cbl_outpost2 map.

## Duplicate content and separate prototype identities

Unregistered cbl_outpost2 has the same exact ZON title, room IDs #1000–#1004,
mobile IDs #1000–#1003, and four M reset placements. Its #1000 room instead
has the generic title “First Room” and an empty description. These shared
identifiers and reset rows do not establish which unregistered source is
canonical.

Active cbl_hall copies all five Monsteri room descriptions at the separate
VNUMs #29820–#29824. After color-token and whitespace normalization, the
room-description bodies match; four titles match, while active #29820 omits
“At the” from Monsteri #1000’s title. Active CBL Hall keeps these room IDs in
its own area. Its four corresponding mobile roles are #29820–#29823, with
separate prototype blocks and fields. The active CBL Hall version also resets
its own portal object #29710 in room #29824 with value[0] #29708
([CBL Hall audit](CBL_HALL_AUDIT_REVIEW.md#encoded-portal-objects-and-qualified-runtime-links)).
Monsteri’s OBJ is empty, so that active item is not part of this candidate’s
source package and must not be attributed to it.

The candidate mobile IDs #1000–#1003 each collide with same-number records
in cbl_outpost2 and active Heavens. Heavens #1000 is an “unkillable bastard”;
Heavens #1001–#1003 are peasants. The separate areas_mini/mini.mob fixture
also defines all four IDs as peasants. These records differ from Monsteri’s
golems. No active ZON M command uses #1000–#1003; the same-number M commands
are in Monsteri and cbl_outpost2. A raw Menden ZON value #1000 is typed as an
object reference, but Monsteri defines no objects. A typed sidecar scan found
no story JSON VNUM fields in #1000–#1004, and a bounded direct-call scan found
no real_room, real_mobile, or real_object call for these values. The empty
QST/OBJ files cannot own a native giver or item contract.

## Tracked history and safe disposition

Followed WLD, MOB, and ZON paths stay on the Monsteri stem from the 2009
areas-tree move, with same-stem MOB/ZON format maintenance in 2012 and
same-stem updates in 2013. The zero-byte OBJ and QST histories behave
differently: Git’s rename-follow heuristic traces the empty blobs through
Players/Declined/... and Players/justice_list. That empty-file path history
does not identify an editor, predecessor, or source lineage for Monsteri.
No followed area path establishes an alternate-stem rename, retirement, or
whole-zone replacement. The same title and copied prose establish duplicate
content, not source precedence. See the existing
[CBL Outpost 2 audit](CBL_OUTPOST2_AUDIT_REVIEW.md#monster-island-copies-and-active-cbl-hall-mapping),
[CBL Hall audit](CBL_HALL_AUDIT_REVIEW.md#title-history-and-duplicate-content-review),
and [Checkpoint 199 identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-199-cbl-hall-and-monster-island-copies).

**Disposition: RETAIN IN PLACE.** Preserve all five tracked paths, including
the two zero-byte files, as a distinct source package. Resolve whether
Monsteri or cbl_outpost2 is the intended source for the reused Monster Island
prose, how both relate to active CBL Hall, the #1000 zone-header/range
mismatch, the CBL Hall portal difference, the active Heavens mobile-VNUM
collisions, and any external editor or persisted references before activation,
renumbering, merge, archive, or path move. No migration clearance is implied.
See the [24 builder questions](MONSTERI_BUILDER_QUESTIONS.md),
[connectivity supplement](WORLD_CONNECTIVITY.md#checkpoint-509-monsteri-local-map-and-header-range-mismatch),
[identity/history supplement](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-509-monsteri-copies-and-empty-file-history),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-509-monsteri-retained-pending-owner-and-vnum-decisions),
[candidate dependency line 72](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L72), and
[coverage line 423](ZONE_COVERAGE_REGISTER.csv#L423).

### Checkpoint 920: Monsteri exact cross-source role closure

The CP509 source inventory and candidate graph remain unchanged: five rooms,
eight local directions, one weak component, and raw directed reach from #1000
to all five rooms. An all-source WLD target scan found eight other-file target
records in #1000–#1004, all within unlisted [cbl_outpost2](../../../areas/wld/cbl_outpost2.wld).
Those directions stay within cbl_outpost2’s own #1000–#1004 rooms. No active
WLD room owns #1000–#1004, no active WLD target points into that band, and
Monsteri still has zero active inbounds and no outbound cross-area edge.

The active ZON scan found no mobile-role reference to #1000–#1004 and no
room-role reference to #1000–#1004, using the maintained
[ZON role mapping](../../../src/world/db.c#L1634). All-source M commands in Monsteri’s
#1000–#1003 mobile range remain four local placements in
[Monsteri ZON](../../../areas/zon/Monsteri.zon#L10) and four in
[cbl_outpost2 ZON](../../../areas/zon/cbl_outpost2.zon#L10); neither source
is active. Monsteri mobiles #1000–#1003 remain distinct from active
[Heavens mobiles #1000–#1004](../../../areas/mob/heavens.mob#L1867).
Monsteri room #1004 also shares a number with active Heavens mobile #1004, a
peddler, but no active reset uses that mobile number. These same-number
records have different prototype roles and source owners.

Two active object-role references fall in the overlapping #1000–#1004 number
band. [Menden ZON G #1000](../../../areas/zon/menden.zon#L141) is an object
reference under the [maintained reset mapping](../../../src/world/db.c#L1649).
The [N-format loader](../../../src/economy/shop.c#L3364) reads the
[Menden shop #88804](../../../areas/shp/menden.shp#L5), which also lists object
#1000 as stock. Both resolve by number to the current active
[Heavens barrel object #1000](../../../areas/obj/heavens.obj#L8546). Monsteri
OBJ remains zero-byte. Across 544 parser-valid active N-format shops, no
keeper uses Monsteri’s #1000–#1003 mobile band and no shop room uses its
#1000–#1004 room band. This does not assign the Heavens object to Monsteri.

The all-source QST scan found no giver key in #1000–#1004 and no `G I` or
`R I` item reference in that range. A structural scan of 232 story JSON files
found no typed room, mobile, giver, or item VNUM property in #1000–#1004; the
maintained [story schema](../../../src/world/zone_story_quest_story.c#L108)
and [item VNUM parser](../../../src/world/zone_story_quest_story.c#L243)
type giver and item VNUMs explicitly.
Some legacy QST and story text contains the number 1000 in non-VNUM fields;
for example, Alatorin’s [E:1000 completion reward](../../../areas/story/alatorin.story.json#L3431)
is an XP value, while its `item_vnums` remain separate item identities. A
separate scan of story completion keys found no `I:1000`–`I:1004` item role. No
active [ITEM_TELEPORT value[0]](../../../src/magic/spell_travel.c#L991)
targets #1000–#1004. A bounded source-code search found no literal
`real_room`, `real_room0`, `real_mobile`, or `real_object` call for these
values and no Monsteri name binding.

No rename, predecessor, retirement, or whole-zone replacement evidence was
added. **Disposition remains RETAIN IN PLACE.** Preserve the five source
paths, empty OBJ/QST files, separate cbl_outpost2 prototypes, and active
Heavens mobile/object identities. Keep the #1000 ZON-header/range discrepancy
and Portal Cave travel contract unresolved until the owners close numbering,
lineage, and external references. See [MON-15 and MON-23](MONSTERI_BUILDER_QUESTIONS.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-920-monsteri-exact-room-band-adds-no-active-ingress),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-920-monsteri-cross-source-roles-remain-distinct),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-920-monsteri-exact-role-closure-remains-under-retention),
[candidate dependency row 71](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L72), and
[coverage row 422](ZONE_COVERAGE_REGISTER.csv#L423).

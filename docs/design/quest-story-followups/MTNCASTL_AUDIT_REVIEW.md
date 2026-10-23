# mtncastl source and identity audit

Canonical key: component:mtncastl. Physical coverage row 425 / CSV line 426;
candidate dependency row 74 / CSV line 75. Initial review October 11, 2026
(Checkpoint 512); exact role supplement October 13, 2026 (Checkpoint 923),
against source snapshot
c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static source,
connectivity, identity, and bounded-history review. It does not establish
live availability, travel, editor ownership, or archive safety.

## Inventory and source content

The unregistered component has four same-stem paths:
[WLD](../../../areas/wld/mtncastl.wld#L1),
[MOB](../../../areas/mob/mtncastl.mob),
[OBJ](../../../areas/obj/mtncastl.obj), and
[ZON](../../../areas/zon/mtncastl.zon#L1). MOB and OBJ are zero bytes. The
WLD contains one room, #1, titled “Unnamed,” with an empty description and no
exits. The ZON header is #1, its exact title is “The Horrible Tracts of the
Unnamed,” its last VNUM is #1, and its reset mode is 2; it has no reset
commands. No same-stem SHP, QST, story JSON, or map JSON is present. Neither
[areas/AREA](../../../areas/AREA) nor
[areas/qst/AREA](../../../areas/qst/AREA) lists mtncastl.

The source gives no authored setting beyond the empty room and the ZON title.
The title does not describe an implemented “mountain castle” environment by
itself, and no source evidence establishes that name as an alias. The map is
not a route or a completed zone: it has no WLD direction records, no reset
placements, and no local mobiles or objects.

## Room identity and connectivity

Room #1 falls outside the numeric #100–#199 band implied by ZON header #1
under the repository’s VNUM/100 convention. Preserve the raw header and room
number as an unresolved mismatch. The component graph contains one isolated
room: zero direction records, one weak component of size one, no fall-chance
or current fields, and no candidate outbound boundary.

Room #1 is explicitly reserved as ROOM_LIMBO_VNUM in
[utils.h](../../../src/core/utils.h#L637). Active [Limbo WLD](../../../areas/wld/limbo.wld#L8)
is the sole active room definition for #1 and has a different, complete room
record. Static source inventory finds twelve WLD sources defining #1. The
entire mtncastl room block is byte-identical to the #1 placeholder in
unregistered astral and ud-inside; it is a content match, not proof that
these stems share one identity.

Active WLD directions that numerically target #1 resolve by the current
active room table to Limbo. The active candidate-inbound count for mtncastl
is zero; those generic numeric references do not establish an arrival to
this unlisted source. There are no local exits to resolve against the active
room table. A map-editor coordinate file and runtime loader contents were
not established for this component.

## Title, duplicate content, and history

The exact ZON title “The Horrible Tracts of the Unnamed” occurs in five
current files: astral, eoo_op2, mtncastl, theurgist, and ud-inside. The
room-block equality with astral and ud-inside is a separate comparison from
the five-file title match. Neither repeated title nor identical placeholder
content establishes precedence, replacement, or shared editor ownership.

Followed history covers all four same-stem paths. WLD history reaches 2011
area-tree revisions; MOB and OBJ history reaches the 2005 initial revisions,
the 2006 directory change, and the 2011 area-tree change; ZON history reaches
the 2009 area-tree move, 2011 revisions, and the 2015 quest-item flag update.
These paths show no explicit rename to a successor or whole-zone replacement.
The available repository history cannot establish external editor aliases,
untracked source copies, persisted references, or deployment intent. See the existing
[Astral audit](ASTRAL_AUDIT_REVIEW.md#title-and-tracked-history) for the
cross-source #1 inventory and the
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-512-mtncastl-title-and-placeholder-collisions).

## Safe disposition

**Disposition: RETAIN IN PLACE.** Preserve the WLD, zero-byte MOB and OBJ,
and ZON as one distinct source package. Resolve the intended owner and
purpose of room #1, the relationship among the five same-title ZON sources,
the VNUM/100 range mismatch, and any external editor or persisted data before
considering activation, renumbering, merge, archive, or path move. Do not
attribute generic room #1 references to mtncastl while active Limbo owns
that VNUM. No migration clearance is implied. See the
[13 builder questions](MTNCASTL_BUILDER_QUESTIONS.md),
[connectivity supplement](WORLD_CONNECTIVITY.md#checkpoint-923-mtncastl-number-1-references-resolve-to-limbo),
[identity/history supplement](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-923-mtncastl-history-and-placeholder-references),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-923-mtncastl-retained-after-room-role-closure),
[candidate dependency line 75](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L75), and
[coverage line 426](ZONE_COVERAGE_REGISTER.csv#L426).

## Checkpoint 923: exact room #1 dependency and role closure

The candidate still defines one room #1 with no exits, while active Limbo is
the only loaded WLD owner of that VNUM. A repository-wide WLD scan found 23
directions whose raw target is #1: nine active Desert directions from room
#49790, which resolve to Limbo under the current table, and fourteen
source-only directions from eight unlisted maps. None is an active inbound
to mtncastl. The candidate has no outbound WLD edge.

Four current ZON commands use room #1 as a room field: one D command in
unlisted Magetower, one D command in unlisted Mavulsk, and two M placements
in unlisted Mavulsk. mtncastl.zon has no commands. Each numeric #1 use remains
a room-role collision; the Mavulsk mobile VNUM #1 is a separate mobile role.
The active Limbo source also defines mobile #1 and object #1, while mtncastl's
MOB and OBJ files are zero bytes.

Typed endpoint checks found no SHP in_room #1 among 712 N-format records or
the eight legacy O-format records in unlisted luiren; no ITEM_TELEPORT
value[0] or ITEM_SWITCH value[1] target #1 among 22,483 current OBJ records
including mini.obj; no typed/location-like room #1 value in 232 story or 21
map JSON sidecars; and no QST #1 giver or item-goal role. The three compiled
real_room(1) call sites and four ROOM_LIMBO_VNUM artifact placements are
generic Limbo paths, not mtncastl bindings
([Limbo reservation](../../../src/core/utils.h#L637),
[teleport lookup](../../../src/magic/spell_travel.c#L745),
[switch room field](../../../src/specs/specs.object.c#L302),
[shop room field](../../../src/economy/shop.c#L3496)).

These findings preserve the existing zero candidate-specific active WLD
inbound count. They add no map route and no typed dependency on the unlisted
candidate. Retain the current four-file package pending owner, editor, and
persisted-room identity evidence; generic #1 records cannot identify
mtncastl while Limbo owns the active value. See the
[connectivity supplement](WORLD_CONNECTIVITY.md#checkpoint-923-mtncastl-number-1-references-resolve-to-limbo),
[identity/history supplement](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-923-mtncastl-history-and-placeholder-references),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-923-mtncastl-retained-after-room-role-closure),
[candidate dependency row 74](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L75), and
[coverage row 425](ZONE_COVERAGE_REGISTER.csv#L426).

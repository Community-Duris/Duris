# Lords of Chaos Guildhall inactive-source audit

Canonical key: component:loc_gh. Physical coverage row 411 / CSV line 412;
candidate dependency row 60 / CSV line 61. Reviewed October 11, 2026,
Checkpoint 498, against source snapshot
c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static source,
connectivity, numeric-identity, reset, shop, story, code-reference, and
bounded-history review. It does not establish live loading, gameplay, actual
shop boot behavior, saved world ownership, or permission to activate, rename,
move, or archive any file.

## Source inventory, naming, and active Newbie identity

The unlisted component has [WLD](../../../areas/wld/loc_gh.wld#L1),
[MOB](../../../areas/mob/loc_gh.mob#L1),
[OBJ](../../../areas/obj/loc_gh.obj#L1),
[ZON](../../../areas/zon/loc_gh.zon#L1), and
[SHP](../../../areas/shp/loc_gh.shp#L1) sources. It has no same-stem QST,
story sidecar, or raw AREA annotation. Neither [areas/AREA](../../../areas/AREA)
nor [areas/qst/AREA](../../../areas/qst/AREA) lists loc_gh. The ZON header is
#293, titled “Lords of Chaos GuildHall,” with top #29333 and reset mode 0.
The source defines 22 rooms (#29300–#29312, #29314, #29320–#29323, and
#29330–#29333), six mobiles (#29300–#29303 and #29330–#29331), six objects
#29300–#29305, and one shop record headed #29301.

The normalized title is unique among scanned ZON titles. Header #293 is also
used by the unlisted BattleRager Hall and Ebon Skull Guildhall. Active Newbie
is listed across *292–293 and defines 14 of loc_gh's room numbers in
[newbie.wld](../../../areas/wld/newbie.wld#L1975). Those active records are
different map identities, not loc_gh entrances:

| VNUM | Unlisted loc_gh | Active Newbie |
| --- | --- | --- |
| #29300 | Entrance to an abandoned cave | Crypt of the Undead |
| #29303 | Stone golem | Bleeding human |
| #29305 | Inn of the Cavern | A Secret Magic Shop |
| #29308 | Bottom of the cavern shaft | Inside a Storage Locker |

Eight loc_gh rooms (#29320–#29323 and #29330–#29333) have no active WLD room
record. Other unlisted WLD sources also overlap loc_gh: cbr_hall defines ten
of its room numbers, eskull_hall thirteen, ravenmis ten, and ethdawn_op three.
Their room titles differ in representative same-number records, such as
#29300 “Entering the Light” in cbr_hall, a dark mausoleum in eskull_hall, and
a foggy descent in ravenmis. Preserve each source identity until its map owner
resolves the overlaps.

Newbie also defines different active mobile prototypes at #29300–#29303 and
objects at #29300–#29305. For example, loc_gh's #29300 is a stone golem while
Newbie's #29300 is a mercenary in training; loc_gh objects #29300 and #29302
are portals while the active Newbie objects are Pain and a healing emblem.
The active QST and story references to these numbers resolve to the Newbie
prototypes. Candidate mobiles #29330–#29331 do not have same-number Newbie
mobile definitions.

## Candidate graph and active route identity

The loc_gh WLD has 39 authored direction records: 36 local edges and three
D4 references to unlisted Duris3 rooms:

| Candidate exit | Source-only target |
| --- | --- |
| [#29300 up](../../../areas/wld/loc_gh.wld#L14) | [#154441, The Ocean](../../../areas/wld/Duris3.wld#L1143262) |
| [#29310 up](../../../areas/wld/loc_gh.wld#L199) | [#163267, The Ocean](../../../areas/wld/Duris3.wld#L1328608) |
| [#29320 up](../../../areas/wld/loc_gh.wld#L269) | [#110241, Mountains](../../../areas/wld/Duris3.wld#L215062) |

None of those Duris3 target records has a direct exit back to #29300–#29333.
Duris3 is not in the full area list. The 36 local edges form four weak
components of sizes 10, 4, 4, and 4. Directed traversal from #29300, #29310,
#29320, and #29330 reaches all rooms in its respective component. There are
no negative targets, source rooms outside the #293 numeric range,
range-mismatch rooms, fall chances, or current fields.

Twenty-four local target records numerically resolve through the current
active room table to Newbie's #29300–#29312 and #29314 records. Twelve local
target records point to loc_gh-only room numbers and remain unresolved in the
active table; the three Duris3 endpoints are also unresolved. Thus the source
has 15 positive unresolved target records. An exact room-set check found 16
active directions in newbie.wld targeting VNUMs also defined by loc_gh; those
resolve to Newbie's active room identities. Three other newbie.wld targets in
the #29300–#29333 number band are Newbie-only rooms #29313, #29315, and #29316,
which loc_gh does not define. No active WLD edge enters a loc_gh map identity,
so candidate inbound remains zero.

The six type-25 portal OBJ records separately declare stored destinations.
The ZON resets objects #29300 and #29301 as a reciprocal pair between rooms
#29314 and #29309, and objects #29302 and #29303 as a reciprocal pair between
#29322 and #29307. Objects #29304 and #29305 point to #29323 and #29333 but
have no loc_gh O reset. These are source-declared object destinations, not WLD
edges; no callback or live portal behavior was tested.

## Typed Newbie references, code bindings, and reset imports

The candidate is story-bearing despite having no QST or story sidecar. Active
[newbie.qst](../../../areas/qst/newbie.qst#L1287) awards OBJ #29302 and
another branch awards OBJ #29300 at
[line 2053](../../../areas/qst/newbie.qst#L2053); giver #29304 appears at
[line 4135](../../../areas/qst/newbie.qst#L4135). The active
[newbie.story.json](../../../areas/story/newbie.story.json#L418) likewise
uses Newbie mobile #29304 and later references items #29302 and #29300.
Those IDs currently resolve to Newbie's half-elf giver, healing emblem, and
Pain dagger, not loc_gh's different map or portal prototypes.

Compiled code also owns part of this numeric range. The
[mobile VNUM constants](../../../src/world/vnum.mob.h#L13) assign #29303 to
the incapacitated Ailvio and #29304 to his wife; the
[spec assignments](../../../src/specs/specs.assign.c#L206) bind those IDs to
the bandage callbacks. Active Newbie defines the bleeding human at #29303
and Ailvio's wife at #29304, while loc_gh defines a stone golem at #29303 and
no #29304 mobile. Another numeric assignment binds the inn callback to room
#29305 at [specs.assign.c:2447](../../../src/specs/specs.assign.c#L2447).
That active Newbie room is A Secret Magic Shop, while the unlisted candidate
room is the Inn of the Cavern. Keep the Newbie code and quest ownership
attached to its current active identities.

The candidate ZON header uses reset mode 0 and has 46 commands: 14 O,
17 M, and 15 G. Ten O commands import Player Castles objects: #11052 four
times, #11102 four times, and #11013 twice. Preserve the exact placements and
the existing [Player Castles dependency note](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L61);
these are source-only imports, not active loc_gh resets. The remaining four O
commands place local portal prototypes. The 17 M commands place the six
candidate mobile types, while all 15 G loads on Baraki match the shop's
15-product list.

## Shop input and product identity

The candidate [SHP record](../../../areas/shp/loc_gh.shp#L1) is keyed #29301,
uses keeper #29301, and configures shop room #29308. These are candidate
Baraki and the cavern shaft, but active Newbie uses #29301 for an amateur
assassin and #29308 for a storage locker. Fourteen of the 15 listed products
have active OBJ prototypes: #3050 belongs to Verz1; #53, #108, #109, #111,
#347, #364, #377, #398, #412, and #491–#494 belong to Heavens. Product #6089
has no checked-in OBJ prototype.

Commit ed94862f0, dated September 6, 2026, added Heavens gathering bags
#491–#494 to both loc_gh.zon's G list and loc_gh.shp's stock list. This proves
a recent edit to these unlisted source files, not that the shop is part of
the active production input.

The standard [shop compiler](../../../areas/src/shp/make_shp.c#L6) reads its
AREA list and excludes loc_gh. This checkout has no areas/world.shp. In
non-mini mode, [open_shop_stream](../../../src/economy/shop.c#L3267) falls
back to globbing areas/shp/*.shp if the generated aggregate cannot be opened,
which can include loc_gh.shp. Shop loading resolves keeper and room numbers
against current world tables. This is a conditional loader-path finding, not
confirmation of a usable Baraki shop; no runtime load or interaction was
tested.

## Story, bounded history, and disposition

The rooms describe the Lords of Chaos' cavern, its conference and
interrogation spaces, inns and tavern, a shaft and outpost, and several
separate subterranean areas. The six mobiles include stone golems, Baraki,
a dread knight, and an enslaved dark elf. That authored material is
narrative-bearing even though no QST or separate story sidecar accompanies
the component.

Followed file history shows the 2009 area-tree move; a 2013 file-mode change
for the WLD, ZON, and SHP; 2016 punctuation edits to four portal descriptions
in the OBJ; and the September 2026 bag-stock addition to ZON and SHP. These
bounded histories show no direct rename, retirement, or whole-area
replacement decision. The 2026 source edit makes editor and export ownership
material to any later proposal.

**Disposition: RETAIN IN PLACE.** Keep all five candidate files at their
current paths and out of both area lists while the owner resolves the active
Newbie room/mobile/object identities, the other overlapping maps, the three
Duris3 boundaries, code and quest consumers, portal placements, Player
Castles imports, shop fallback inputs, missing #6089 prototype, and
editor/export/deployment/persisted-ID state. Preserve Newbie's active records,
quest references, and compiled bindings. Do not infer replacement or approve
activation, merge, renumbering, path movement, or archival from numeric
overlap or list exclusion. See the [25 builder questions](LOC_GH_BUILDER_QUESTIONS.md),
[identity/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-498-lords-of-chaos-guildhall-source-identities-and-vnum-collisions),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-498-lords-of-chaos-guildhall-retained-pending-map-code-and-shop-ownership),
[candidate dependency row 60](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L61), and
[coverage row 411](ZONE_COVERAGE_REGISTER.csv#L412).

## Checkpoint 910: role-aware source closure

The exact-room and typed-role sweep leaves the CP498 candidate graph unchanged.
The active Newbie WLD has 16 direction targets equal to rooms actually defined
by loc_gh. Three additional Newbie targets in the broad #29300–#29333 numeric
band are #29313, #29315, and #29316, which loc_gh does not define. The other
same-number WLD target records in `cbr_hall`, `eskull_hall`, `ravenmis`, and
`ethdawn_op` remain local to rooms defined by those unlisted maps. None is
active ingress into the loc_gh identity.

Exact reset-role hits come from the active `newbie.zon` and unlisted sibling
shards. Newbie resets its own #29300–#29303 mobiles, its own objects #29301,
#29303–#29305, and uses same-number door/room fields in the active Newbie map.
`cbr_hall`, `eskull_hall`, and `ravenmis` also reset their own same-number
mobile/object prototypes. `ethdawn_op` uses #29310–#29312 as its own mobile,
object, and room roles. These reset operands do not add candidate loading or
change loc_gh's zero active-inbound count.

The all-shop N-format scan found four other exact-role records. Active Newbie
shop #29283 lists its Newbie object #29305 and uses Newbie room #29305. Unlisted
`cbr_hall` uses keeper/room #29303 and stocks its own Ogre-hide backpack #29300;
unlisted `eskull_hall` uses keeper #29303 and room #29302; unlisted `ravenmis`
uses keeper #29300, stocks object #29301, and uses room #29309. These remain
separate keeper, product, and room roles from Baraki's candidate SHP fields
(#29301/#29308). With `areas/world.shp` absent, the possible non-mini glob can
include the unlisted N-format shops as well; no runtime loading or collision
behavior was tested.

Quest/story and portal roles also belong to their current source identities.
Active `newbie.qst` and `newbie.story.json` use Newbie objects #29300 and #29302
as items and Newbie mobile #29304 as a giver; raw item numbers #29310–#29312,
#29330–#29331, and #29333 are Newbie OBJ roles, not loc_gh rooms or mobiles.
Active Newbie portal objects #29299 and #29313 store targets #29305 and #29308,
which are active Newbie rooms. Unlisted `ethdawn_op` portal #29310 targets
#29307, an active Newbie room number also used by loc_gh, while its #29311
portal targets its own #29312 room. The two unlisted Ebon Skull portals target
its own #29312 and #29309 rooms. No external type-29 room target was found.
These values do not establish a portal route into the loc_gh map identity.

Compiled references remain Newbie-owned: #29303/#29304 are Ailvio mobile
constants, with callbacks assigned to the active Newbie prototypes, and
`real_room0(29305)` binds the inn handler to Newbie's active room. Candidate
#29303 is a different mobile and candidate #29305 is a different room. Other
raw code hits such as #29317/#29319/#29328/#29329 do not name any loc_gh room,
mobile, or object. No code binds to a loc_gh source identity. Preserve the
CP498 **RETAIN IN PLACE** disposition and all existing map, quest, shop, and
deployment questions.

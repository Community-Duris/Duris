# genesis hall source audit

Canonical key: component:genesis. Physical coverage row 396 / CSV line 397;
candidate dependency row 46 / CSV line 46. Reviewed October 11, 2026,
Checkpoint 483, against source snapshot
72ba3deff3bb30bde880018cdebe7c6db675b697. This is a static source,
connectivity, history, and typed-reference review. It does not establish
runtime loading, travel, shop behavior, editor ownership, persisted world
state, or permission to activate or move these files.

## Source inventory and area-list status

The current component consists of [WLD](../../../areas/wld/genesis.wld#L1),
[MOB](../../../areas/mob/genesis.mob#L1), an empty
[OBJ file](../../../areas/obj/genesis.obj), [ZON](../../../areas/zon/genesis.zon#L1),
and one [SHP record](../../../areas/shp/genesis.shp#L1). The WLD has ten
rooms #29900–#29909; MOB has four prototypes #29900–#29903; OBJ has no
prototypes. ZON header #299 is titled “genesis hall,” ends at #29909, and
uses reset mode 0. SHP has one shop record for keeper #29902 in room #29902,
with nine products. There is no same-stem QST, story sidecar, editor map, or
tracked .cap backup among the five Genesis source files.

The only matching area-list annotation is the raw *OPEN 299 line in
[areas/AREA](../../../areas/AREA#L144). The stem is absent from the full
areas/AREA list and areas/qst/AREA. The annotation itself is blank for
Genesis, and it does not establish that the source files load. Checkpoint 440
found “genesis hall” unique among 442 current ZON titles. The distinct
“Aeon Order Guildhall” title and the shared #299 range are documented in the
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-440-aeon-order-guildhall-and-genesis-vnum-collision).

## Authored map and narrative

Genesis presents a Nexus, the gateway between pillars, a vault, sanctuary,
grand staircase, landing, garden, gallery, and paired crystal-water and
molten-lava caverns. These descriptions support a guild stronghold setting;
the source does not explain the guild's current owner, membership, quests, or
relationship to Aeon. The missing QST and story sidecar are absences in the
current source bundle, not proof that its prose has no story value.

The ten WLD rooms contain 19 direction records. Eighteen point within the
Genesis source map, making one local weak component; directed traversal from
#29900 reaches all ten rooms. The remaining record is D4 from #29901 to
#157747, “The Ocean,” in [Duris3.wld](../../../areas/wld/Duris3.wld#L1212688).
Duris3 is not listed in the full area list, and the Genesis source is also
unlisted, so this is a source-only edge rather than a qualified current route.
The target's current WLD block has no edge back to Genesis. A bounded scan
found no WLD source outside the Genesis and aeon_gh candidate files targeting
#29900–#29909. Those are the only current WLD files defining that range; their
room titles and blocks differ. No negative target, fall record, or current
record occurs in Genesis WLD. All rooms and exit sources fit the conventional
#29900–#29999 range implied by zone #299.

## Typed reset and shop references

The reset file has 17 commands under mode 0: three O imports, five M
placements, and nine G placements. O imports place Player Castles objects
#11105 in room #29900, #11055 in #29901, and #11013 in #29903. M commands
place gargoyles #29901, #29900, and two #29903s in room #29901, plus keeper
#29902 in room #29902. The nine G commands give the keeper the same nine
object IDs listed in the local SHP record. All three imported objects are
declared in [player_castles.obj #11013](../../../areas/obj/player_castles.obj#L121),
[#11055](../../../areas/obj/player_castles.obj#L161), and
[#11105](../../../areas/obj/player_castles.obj#L281); they remain typed object
imports, not Genesis OBJ declarations.

The Genesis OBJ file is empty, so every shop product is external to this
candidate. Current OBJ definitions found by exact VNUM are:

| Product | Current definition owner(s) |
| --- | --- |
| #3050 | verz1 |
| #6089 | No current OBJ declaration |
| #108 | heavens and mavulsk |
| #109 | heavens and mavulsk |
| #111 | heavens |
| #398 | heavens |
| #53 | dalvik, heavens, hostel, and mavulsk |
| #412 | heavens |
| #377 | heavens |

The duplicated #53/#108/#109 numbers and missing #6089 leave the intended
loaded stock identities unresolved. Genesis and Aeon both reference #6089.
The four local mobile prototypes include keeper #29902, but the only current
same-type definition for #29902 is in Genesis; the component is not a full
area-list member. The configured areas/world.shp file
([db.h](../../../src/world/db.h#L22)) is absent. In non-mini mode,
open_shop_stream() falls back to globbing areas/shp/*.shp when the
configured shop file cannot be opened
([shop.c](../../../src/economy/shop.c#L3276)). The shop loader maps keeper
#29902 through real_mobile(); assign_the_shopkeepers() then indexes
mob_index[keeper] without a negative-index guard
([lookup](../../../src/economy/shop.c#L3492),
[assignment](../../../src/economy/shop.c#L3627)). Startup calls that assignment
after the shop boot
([specs.assign.c](../../../src/specs/specs.assign.c#L1261)). If the fallback
includes Genesis while its keeper prototype is not loaded, this is a
conditional source-code risk. It is not evidence that the fallback is used
or that a runtime failure occurred.

## Shared VNUMs, history, and owner decision

Genesis and aeon_gh each define rooms #29900–#29909 and mobiles #29900/#29901
with different descriptions and mobile identities. Genesis uses the shared
mobile IDs for gargoyles; Aeon uses them for a mist golem and merchant.
Genesis additionally defines keeper #29902 and gargoyle #29903. Both ZON
titles are distinct and unique, but neither source stem is a full area-list
member. Compiled Players Guild code assigns an inn handler to
real_room0(29903) in
[specs.assign.c](../../../src/specs/specs.assign.c#L2452), and both maps
define that room number differently. Current static evidence does not settle
which source identity, if either, should own the shared code binding or zone
range. See the [Aeon source audit](AEON_GH_AUDIT_REVIEW.md#vnum-collision-title-and-history).

Followed histories remain on the Genesis stem. WLD and SHP history show the
2009 area-tree move and the 2013 “stuff” commit. MOB, OBJ, and ZON also show
2012 format-field maintenance and the 2016 unnamed-object cleanup. These
bounded histories show no alternate-stem rename or explicit whole-zone
replacement. The common VNUMs are a source conflict, not proof that one map
replaces the other.

Keep candidate row 46 **RETAIN IN PLACE**. Preserve its authored map, source
exit, three Player Castles imports, five mobile placements, empty OBJ file,
nine shop products and G resets, including the duplicated object definitions
and undeclared #6089. Resolve #299 ownership, the compiled #29903 handler,
shop stock/keeper identity, manifest and editor ownership, and persisted-ID
closure before an activation, move, rename, archive, or merge proposal. See
the [24 builder questions](GENESIS_BUILDER_QUESTIONS.md),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-483-genesis-hall-source-map-and-ocean-edge),
[identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-483-genesis-source-identity-and-shop-dependencies),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-483-genesis-retained-pending-range-and-shop-ownership),
[candidate row 46](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L46), and
[coverage row 396](ZONE_COVERAGE_REGISTER.csv#L397). No source-area files,
code, tests, builds, runtime, server, database, or containers were changed
or used.

## Checkpoint 895: role-aware Aeon and external dependency closure

The Genesis source set remains unchanged from snapshot
`72ba3deff3bb30bde880018cdebe7c6db675b697` through current HEAD
`c500ac51b8cd26540943257e7ca4e9807263ef74`; no `areas/` or `src/` differences
occur between those commits. Genesis and `aeon_gh` remain absent from the full
area list. The sole matching [areas/AREA entry](../../../areas/AREA#L144) is
the blank `*OPEN 299` annotation, so neither bundle's identities are
established as active.

The current all-source ZON role scan found every external numeric match to
Genesis rooms or mobiles in unregistered `aeon_gh.zon`: three O room
placements into #29900, #29902, and #29903; four M placements of Aeon mobile
#29900 into Aeon room #29900; and one M placement of Aeon mobile #29901 into
Aeon room #29909. These are Aeon's own same-number room/mobile records, not
active Genesis placements. No other ZON file contains a matching room or
mobile reset role. See the [Aeon O resets](../../../areas/zon/aeon_gh.zon#L9),
[golem resets](../../../areas/zon/aeon_gh.zon#L13), [merchant reset](../../../areas/zon/aeon_gh.zon#L17),
and [Aeon mobile #29900](../../../areas/mob/aeon_gh.mob#L1)/[#29901](../../../areas/mob/aeon_gh.mob#L17). The scan of all 265
other QST files and 232 story sidecars found no exact candidate VNUM role.
There is no Genesis OBJ prototype, and
the all-source OBJ scan found no external type-25 teleport destination or
type-29 switch room field targeting #29900–#29909. Same-type mobile scans
found only Aeon definitions #29900 and #29901, with records different from
Genesis; no outside object definition overlaps the empty Genesis OBJ set.
The bounded WLD scan still finds no room target into Genesis outside
`genesis.wld` and `aeon_gh.wld`.

The only outside SHP keeper/room match in the numeric band is the unregistered
Aeon shop: it uses Aeon mobile #29901 as keeper and Aeon room #29909 as its
shop room. Genesis instead records keeper and room #29902. Aeon's shop also
stocks all nine Genesis shop product numbers—#3050, #6089, #108, #109, #111,
#398, #53, #412, and #377—plus other products. Those shared product VNUMs
resolve to the separately documented outside OBJ definitions or, for #6089,
no current OBJ declaration; they do not establish a Genesis object owner or
show that either shop replaces the other. See the [Aeon SHP record](../../../areas/shp/aeon_gh.shp#L1),
[product list](../../../areas/shp/aeon_gh.shp#L3), [keeper field](../../../areas/shp/aeon_gh.shp#L47),
and [shop-room field](../../../areas/shp/aeon_gh.shp#L49).

An exact C/C++ numeric scan found one compiled room binding in this range:
`specs.assign.c` assigns the inn handler at `real_room0(29903)`. Both Genesis
and Aeon define different room records at #29903, while neither component is
in the full area list. The intended room identity for the compiled handler
therefore remains unresolved. No other exact #29900–#29909 code reference
was found. See the [assignment](../../../src/specs/specs.assign.c#L2452),
[Genesis room](../../../areas/wld/genesis.wld#L63), and
[Aeon room](../../../areas/wld/aeon_gh.wld#L62).

These role matches add no WLD edge or reachability result. Keep candidate
row 46 **RETAIN IN PLACE**; preserve the separate Aeon and Genesis map, mobile,
reset, shop, and shared product roles, the three Player Castles imports, the
Ocean source exit, and the empty Genesis OBJ file. Resolve the #299 range and
#29903 handler owner, shop/loading relationship, manifests, editor/deployment
ownership, and persisted IDs before any activation, rename, move, archive, or
merge proposal. The [24 builder questions](GENESIS_BUILDER_QUESTIONS.md)
remain open, with GE-14 refined for the Aeon shop overlap.

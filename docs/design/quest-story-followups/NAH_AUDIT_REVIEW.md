# NAH source and map audit

Canonical key: component:nah. Physical coverage row 427 / CSV line 428;
candidate dependency row 76 / CSV line 77. Reviewed October 11, 2026,
Checkpoint 514, against source snapshot
c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static narrative,
connectivity, identity, reset, shop, and bounded-history review. It does not
establish live travel, resets, shop operation, or archive safety.

## Source package and authored setting

The unregistered package has five same-stem files:
[WLD](../../../areas/wld/nah.wld#L1),
[MOB](../../../areas/mob/nah.mob#L1),
[OBJ](../../../areas/obj/nah.obj#L1),
[ZON](../../../areas/zon/nah.zon#L1), and
[SHP](../../../areas/shp/nah.shp#L1). There is no QST or story/map JSON
sidecar, and neither [areas/AREA](../../../areas/AREA) nor
[areas/qst/AREA](../../../areas/qst/AREA) lists nah. The ZON header is #301,
its exact title is “Nihilists Anarchists Horde Guildhall,” its last VNUM is
#30112, and reset mode is 0.

The 13-room WLD depicts a separate fantasy guildhall and castle: “An Entrance
to the Castle,” the “Sacred Temple of Illumination,” a shop, rest room,
treasure vault, blood-soaked altar, gardens, grand library, tower of high
wizardry, two cottages, a Farcaster Portal, and a Wizards Tower
([room list](../../../areas/wld/nah.wld#L1)). The four mobile prototypes are
Kallisti, a high priestess, Kaberf the Great, and Eris, a shopkeeper. The OBJ
contains two glowing portal prototypes. The source has no native quest or
story sidecar.

## WLD graph and active VNUM ownership

All 13 room VNUMs #30100–#30112 fall within the numeric #30100–#30199 band
implied by ZON header #301. The candidate graph has 26 direction records:
24 local and two outbound. The local graph has two weak components of sizes
10 and 3. Raw directed reach from #30100 covers its ten-room component;
reach from #30110 covers its three-room component. No fall-chance or current
fields were found.

| Source exit | Target | Current source-table result |
| --- | --- | --- |
| #30100 up | Duris3 #156123, “The Ocean” ([candidate exit](../../../areas/wld/nah.wld#L49), [target](../../../areas/wld/Duris3.wld#L1178584)) | Not an active room; its Duris3 exits continue to other ocean rooms, with no return to Nah. |
| #30110 up | Duris3 #179220, “The Ocean” ([candidate exit](../../../areas/wld/nah.wld#L280), [target](../../../areas/wld/Duris3.wld#L1663621)) | Not an active room; its Duris3 exits continue to other ocean rooms, with no return to Nah. |

The 24 candidate-local exit targets reuse VNUMs currently owned by active
Underworld3. The full [areas/AREA](../../../areas/AREA#L145) includes
Underworld3, whose WLD defines all #30100–#30112 as different underground
lake/river rooms ([#30100](../../../areas/wld/underworld3.wld#L1999),
[#30112](../../../areas/wld/underworld3.wld#L2245)). Thus the current active
room table resolves those numeric targets to Underworld3 identities, not to
the Nah descriptions. This numeric resolution does not load Nah or make its
map accessible. No active WLD area outside Underworld3 has an exit into
#30100–#30112; candidate-specific active inbounds are zero. The active
Underworld3 code binding at room #30104 is an inn assignment
([assignment](../../../src/specs/specs.assign.c#L2451)); the same number is
Nah’s “Treasure Vault,” a separate inactive source record.

The two candidate-only portal objects appear designed to bridge the
disconnected WLD components: object #30100 is reset into room #30103 and
carries destination VNUM #30111; object #30101 is reset into room #30111 and
carries destination VNUM #30103
([portal objects](../../../areas/obj/nah.obj#L1),
[reset rows](../../../areas/zon/nah.zon#L9)). Teleport use calls real_room on
the object’s destination value ([teleport resolver](../../../src/magic/spell_travel.c#L991)).
Both destination numbers currently identify Underworld3 river rooms. The
portal descriptions instead promise the Icy Tundra and the realms of Light.
These source values and labels need owner resolution; they are not verified
routes.

## Duplicate room and mobile identities

Active Underworld3 owns all 13 Nah room numbers with different titles and
descriptions. Unregistered cabal_hall also defines rooms #30100–#30109, and
shadow_hall defines #30100–#30102; the overlapping records carry different
content from Nah and from each other. The Cabal Hall dossier maps this
three-way numeric collision
([room-identity review](CABAL_HALL_AUDIT_REVIEW.md#active-underworld3-ownership-of-room-vnums)).
It establishes competing source identities, not which guildhall source is
canonical or whether one replaced another.

Nah mobiles #30100 and #30101 also reuse mobile VNUMs from unregistered
cabal_hall and shadow_hall, where they identify different golem/shopkeeper
prototypes. The current full-area mobile set has no #30100–#30103
definitions. These are separate prototype namespaces from the room VNUMs
and from Nah’s two portal object VNUMs; keep each typed identity distinct.

## Resets, shop stock, and imported objects

The ZON has reset mode 0 but contains 27 reset commands: seven O, nine M,
and eleven G rows. Two O rows reset Nah’s own portals. Five other O rows
import Player Castles objects #11061, #11013, and #11111 into Nah rooms
([reset block](../../../areas/zon/nah.zon#L9),
[Player Castles dependency review](PLAYER_CASTLES_AUDIT_REVIEW.md#typed-dependencies-and-external-consumers)).
Four repeated M rows place mobile #30101, the high priestess, twice each in
rooms #30100 and #30110; Kallisti and Kaberf also have placements in both
areas. The raw reset mode and duplicate commands establish authored data,
not live reset behavior.

Nah’s single SHOP record is #30103: Eris (#30103) is keeper, room #30102 is
the Guildhall Shop, and the product list contains eleven objects. Those same
eleven VNUMs are the ZON G inventory assigned to Eris:
#377, #3050, #6089, #398, #364, #53, #347, #412, #109, #108, and #111
([SHP](../../../areas/shp/nah.shp#L1),
[ZON inventory](../../../areas/zon/nah.zon#L22)). Ten have at least one
current object definition; #6089 has none. VNUMs #53, #108, and #109 also
have additional definitions in unregistered object files, so their intended
item identities are not established by the Nah references alone.

Normal SHP aggregation reads areas/AREA and omits Nah. The source-level shop
loader has a fallback that globs every areas/shp/*.shp file when
areas/world.shp cannot be opened, which can include nah.shp
([Moregeeth loader-path evidence](MOREGEETH_AUDIT_REVIEW.md#direct-external-typed-references)).
The current full-area mobile table does not define keeper #30103, because
Nah’s MOB is also unlisted. This is conditional loader-path evidence, not
proof that the source shop or keeper operated.

## History and safe disposition

Followed histories for all five Nah paths reach the 2009 area-tree move and
later same-stem updates; MOB, OBJ, and ZON also show the 2012 field-format
update. The reviewed histories name no successor, alternate-stem rename, or
whole-area replacement. The exact title occurs only in nah.zon. Current room
and mobile VNUM collisions, the two Duris3 exits, imports, and portal values
are independent source findings; none establishes replacement or safe
migration.

**Disposition: RETAIN IN PLACE.** Preserve all five files as a distinct
source package. Resolve Underworld3’s current ownership of all 13 room
numbers, the cabal_hall/shadow_hall overlaps, two disconnected WLD
components and their portal destinations, one-way Duris3 exits, reset mode
and repeated placements, Player Castles imports, Eris’s missing current
keeper prototype, stock identities, and editor/persisted references before
any activation, renumbering, merge, move, or archive decision. No migration
clearance is implied. See the
[20 builder questions](NAH_BUILDER_QUESTIONS.md),
[Cabal Hall supplement](CABAL_HALL_AUDIT_REVIEW.md#checkpoint-514-nah-room-and-mobile-overlaps),
[Underworld3 supplement](UNDERWORLD_AUDIT_REVIEW.md#checkpoint-514-nah-room-identities),
[Player Castles supplement](PLAYER_CASTLES_AUDIT_REVIEW.md#checkpoint-514-nah-reset-imports),
[connectivity supplement](WORLD_CONNECTIVITY.md#checkpoint-514-nah-two-components-and-duris3-edges),
[identity/history supplement](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-514-nah-and-competing-room-identities),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-514-nah-retained-pending-vnum-and-loader-closure),
[candidate dependency line 77](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L77), and
[coverage line 428](ZONE_COVERAGE_REGISTER.csv#L428).
# BattleRager Hall Audit Review

Reviewed October 11, 2026, Checkpoint 462. This is a static source, narrative, WLD graph, followed-history,
and typed-reference review of the unregistered `cbr_hall` component. It does not establish player
access, shop operation, reset execution, or permission to move or activate the source.

## Source scope and identity

The current source family contains [WLD](../../../areas/wld/cbr_hall.wld),
[MOB](../../../areas/mob/cbr_hall.mob), [OBJ](../../../areas/obj/cbr_hall.obj),
[ZON](../../../areas/zon/cbr_hall.zon), and [SHP](../../../areas/shp/cbr_hall.shp). The ZON header is
#293, titled “BattleRager Hall,” with last VNUM #29309 and reset mode 0. The WLD defines ten rooms
#29300–#29309, the MOB defines four prototypes #29300–#29303, the OBJ defines one backpack #29300,
and the SHP defines one shop record #29303. There is no same-stem QST or story sidecar, and the
component is absent from the full [area manifest](../../../areas/AREA) and
[quest manifest](../../../areas/qst/AREA).

## Environmental story and clan facilities

The opening room describes the light-aligned home base of Clan BattleRage. The ten-room source has
an evil-war planning room, inn, treasure shop, news board, sparring room named “The Y.M.C.A.,”
donation storage, leader Spij’s meditation room, gnome-ball/halfling-toss room, and sauna/hot tubs
([entrance](../../../areas/wld/cbr_hall.wld#L1),
[shop room](../../../areas/wld/cbr_hall.wld#L66),
[tournament room](../../../areas/wld/cbr_hall.wld#L145),
[sauna](../../../areas/wld/cbr_hall.wld#L158)). The title “BattleRager Hall” and prose name
“Clan BattleRage” use different spellings; the canonical display name remains an owner question.

This is the BattleRage source `cbr_hall`, distinct from active `cbl_hall`, the Guildhall of Clan
BloodLust, which has a different title and VNUM range ([active CBL Hall audit](CBL_HALL_AUDIT_REVIEW.md)).

The four mobile prototypes are three golems #29300–#29302 and Merzec, the shopkeeper, #29303
([mobiles](../../../areas/mob/cbr_hall.mob#L1)). Four M commands place #29300 at the entrance, and
one places #29303 in the shop room. Prototypes #29301/#29302 have no M placement. No native QST or
sidecar defines a clan activity or further narrative contract.

## Active Newbie ownership of the same room, mobile, and object VNUMs

Active `newbie` is listed for zone range *292–293 in [areas/AREA](../../../areas/AREA#L141). It
defines all ten room VNUMs #29300–#29309, with different room identities from BattleRager Hall. For
example, candidate #29300 is the clan entrance while active Newbie #29300 is “Crypt of the Undead”;
candidate #29305 is the sparring room while active Newbie #29305 is “A Secret Magic Shop”
([active entrance](../../../areas/wld/newbie.wld#L1975),
[active #29303](../../../areas/wld/newbie.wld#L2032),
[active #29305](../../../areas/wld/newbie.wld#L2050),
[candidate Y.M.C.A.](../../../areas/wld/cbr_hall.wld#L92)). The candidate's local directions to #29300–
#29309 therefore numerically resolve to Newbie rooms in the active world; they do not load the
candidate map.

Active Newbie also defines mobile VNUMs #29300–#29303 as a training mercenary, amateur assassin,
alchemist, and bleeding human man. These differ from the candidate's three golems and Merzec
([active Newbie #29300](../../../areas/mob/newbie.mob#L1507),
[active Newbie #29303](../../../areas/mob/newbie.mob#L1544),
[candidate mobiles](../../../areas/mob/cbr_hall.mob#L1)). Active Newbie resets its own mobiles,
including #29303 in room #29289 ([Newbie reset](../../../areas/zon/newbie.zon#L391)). Compiled `VMOB_AILVIO_INCAPACITATED` and `VMOB_AILVIO_WIFE` are
#29303/#29304 ([VNUM definitions](../../../src/world/vnum.mob.h#L13)); their special assignments
refer to the active Newbie characters ([assignments](../../../src/specs/specs.assign.c#L206),
[Ailvio handler](../../../src/specs/specs.ailvio.c#L168)). Candidate Merzec #29303 and the
candidate's missing mobile #29304 are separate source records with colliding numbers.

The candidate's only local object, #29300, is an Ogre-hide backpack; active Newbie object #29300 is
the painful dagger named “Pain” ([candidate backpack](../../../areas/obj/cbr_hall.obj#L1),
[active dagger](../../../areas/obj/newbie.obj#L1376)). Its local stock and ZON G reset both use
#29300, so the active object identity matters if those references are ever consumed.

Other inactive WLD maps also use room numbers in the #29300–#29309 range, including `eskull_hall`,
`loc_gh`, and `ravenmis`. The active Newbie map currently owns all ten of the candidate's room
numbers. These additional source copies reinforce that numeric room matches alone do not identify a
canonical hall.

## Static WLD graph and Duris3 boundary

The candidate has ten rooms and 19 authored directions: 18 local and one outbound D4/up exit from
#29300 to #152260. Its local graph is one weak component, and authored directed traversal from
#29300 reaches all ten rooms. All local target numbers resolve in the active world to Newbie rooms
with different identities. The one outbound target is the unregistered Duris3 room “Grassy Plains”
([target](../../../areas/wld/Duris3.wld#L1097461)); that target has no authored return exit to
#29300. Room fields and the ZON header say 293, consistent with the candidate's VNUM band.

The full active Newbie WLD has nine authored directions targeting #29300–#29309, and those are
Newbie's own routes. Candidate-specific active inbound remains zero. The candidate has no negative
destination, fall-chance field, or current field. Static loader metrics are 18 resolved targets and
one positive unresolved active-world target (#152260); the inactive Duris3 match is source-only and
does not add an eligible approach into an active map. Curated totals remain 11,987 overall / 415
from Surf. See the [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-462-battlerager-hall-graph-and-newbie-vnum-overlap).

## Resets, shop fallback, and typed dependencies

The candidate ZON contains two external O imports from Player Castles, objects #11055 in room
#29300 and #11105 in room #29304 ([imports](../../../areas/zon/cbr_hall.zon#L9)). Preserve them as
source-only references. Its four M commands reset candidate mobile #29300 in room #29300, and one
M command places candidate keeper #29303 in room #29303. The G commands equip that keeper with 16
products, including the local #29300 backpack and active objects #491–#494. Those four gathering
bags were added to both ZON and SHP in the September 2026 source update
([reset additions](../../../areas/zon/cbr_hall.zon#L29),
[shop stock](../../../areas/shp/cbr_hall.shp#L1),
change recorded in commit `ed94862f0`). Their presence shows the source was maintained; it does not
establish loader membership or an operating shop.

The SHP record sets keeper #29303 and room #29303. The checked-in `areas/world.shp` is absent, and
the non-mini `open_shop_stream()` path falls back to concatenating `areas/shp/*.shp`
([loader path](../../../src/economy/shop.c#L3267),
[SHOP_FILE](../../../src/world/db.h#L22),
[keeper lookup](../../../src/economy/shop.c#L3493)). If that fallback path consumes `cbr_hall.shp`,
the keeper number resolves to active Newbie's bleeding human man #29303 and the room number resolves
to active Newbie's “Unnamed” #29303 ([active room](../../../areas/wld/newbie.wld#L2032)), rather
than candidate Merzec and the candidate Shop of Many Treasures. The SHP product #29300 also resolves
numerically to Newbie's dagger, not the unregistered backpack. This is a source identity conflict,
not a conclusion about shop operation.

The compiled inn binding separately attaches `inn` to room #29305
([special assignment](../../../src/specs/specs.assign.c#L2447)). In the current active world that
number is Newbie's Secret Magic Shop; candidate #29305 is its “Y.M.C.A.” sparring room. This direct
code reference is not evidence that the candidate hall is loaded.

The remaining listed stock IDs #3050, #204, #53, #399, #412, #364, #384, #203, #108, #109, #111,
and #491–#494 have active object prototypes in Verz1 or Heavens, including #3050 in Verz1
([prototype](../../../areas/obj/verz1.obj#L850)) and the four gathering bags in Heavens
([#491–#494](../../../areas/obj/heavens.obj#L5055)). No direct literal
`real_room`/`real_mobile`/`real_object` C++ call to #29300–#29309 was found beyond the compiled inn
assignment and the typed Ailvio mobile constants above. External editor deployments and persisted
player/account references were not inspected.

## Followed history and replacement review

Followed paths show `cbr_hall` WLD/MOB/OBJ/ZON/SHP at the 2009 areas-tree move and same-stem
maintenance in the 2013 “stuff” commit. The ZON and SHP were later edited in September 2026 to add
active Heavens gathering-bag stock and equipment #491–#494. No alternate-stem rename or explicit
replacement decision was found. The active Newbie VNUM conflicts, compiled Newbie bindings, and
inactive numeric-copy maps identify current relationships and dependencies; they do not establish
that Newbie or another hall replaced BattleRager Hall.

## Safe disposition

Keep `cbr_hall` **RETAIN IN PLACE**. Resolve active Newbie ownership of rooms, mobile and object
VNUMs #29300–#29309; the compiled #29305 inn and Ailvio #29303/#29304 bindings; the #152260 Duris3
boundary; the conditional SHP fallback's keeper, room, and #29300 stock identities; the two Player
Castles imports; the #491–#494 update; title spelling; and persisted/tooling identity before any
separate path or activation proposal. No area, code, reset, shop, manifest, or world-list change is
authorized by this audit. The owner-question list is in the
[BattleRager Hall builder questions](CBR_HALL_BUILDER_QUESTIONS.md).

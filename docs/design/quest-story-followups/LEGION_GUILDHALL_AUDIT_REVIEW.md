# Legion Guildhall inactive-source audit

Canonical key: component:legion. Physical coverage row 409 / CSV line 410;
candidate dependency row 59 / CSV line 59. Reviewed October 11, 2026,
Checkpoint 496, against source snapshot
72ba3deff3bb30bde880018cdebe7c6db675b697. This is a static source,
connectivity, numeric-identity, reset, shop, story, and bounded-history review.
It does not establish live loading, gameplay, actual shop boot behavior, saved
world ownership, or permission to activate, rename, move, or archive any file.

## Source inventory, naming, and competing identities

The unlisted component has [WLD](../../../areas/wld/legion.wld#L1),
[MOB](../../../areas/mob/legion.mob#L1),
[OBJ](../../../areas/obj/legion.obj#L1),
[ZON](../../../areas/zon/legion.zon#L1), and
[SHP](../../../areas/shp/legion.shp#L1) sources. It has no same-stem QST,
story sidecar, or raw AREA annotation. Neither [areas/AREA](../../../areas/AREA)
nor [areas/qst/AREA](../../../areas/qst/AREA) lists Legion. The ZON header is
#295, titled “Legion Guildhall,” with top #29522 and reset mode 0. Its WLD has
16 rooms (#29500–#29512 and #29520–#29522); its MOB has six prototypes
#29500–#29505; its OBJ has four prototypes #29500–#29503; and its SHP has one
record headed #29500.

The normalized title “Legion Guildhall” is unique among the tracked ZON
titles, but header #295 is shared by Legion, unlisted Genocide Hall, and Void
Guildhall. Active Harrow's AREA entry spans the broader *294–296 band, but no
active WLD record defines a room in #29500–#29523. The shared band therefore
does not make Harrow's current rooms Legion rooms.

The nearby unregistered sources reuse parts of Legion's room band:
[aod_gh.wld](../../../areas/wld/aod_gh.wld#L1) defines all 16 Legion room
numbers, [genocide_hall.wld](../../../areas/wld/genocide_hall.wld#L1) defines
seven (#29500–#29504, #29510–#29511), and
[rf_op.wld](../../../areas/wld/rf_op.wld#L1) defines three (#29510–#29512).
Every compared room block has different text. The Legion mobile IDs also
overlap AoD mobiles #29500–#29502 and Genocide Hall mobiles #29500–#29503;
the four Legion object IDs overlap AoD objects #29500–#29503. Their
definitions differ. In particular, Legion object #29500 is a glowing portal,
while AoD object #29500 is an obsidian Alliance of Darkness badge. These
numeric overlaps establish competing source identities, not aliases or
replacement evidence.

The AoD audit records 40 direction records from the AoD/Genocide Hall/Legion/
RFOP sibling sources that numerically target AoD's room range: 26 from Legion,
10 from Genocide Hall, and four from RFOP. Each sibling source defines its
own target room numbers, and none of these maps is active. They are not active
ingress into AoD or evidence that the maps share one room identity. No active
WLD exit enters Legion's #29500–#29523 range. See the
[AoD identity and connectivity review](AOD_GH_AUDIT_REVIEW.md).

## Candidate graph and active numeric matches

The Legion WLD has 29 authored direction records: 26 local edges and three
D4 exits to unlisted [Duris3 WLD](../../../areas/wld/duris3.wld#L1) targets:

| Candidate exit | Source-only target |
| --- | --- |
| #29500 up | #153434 |
| #29510 up | #178730 |
| #29520 up | #128779 |

The target rooms exist in Duris3 source but do not resolve through the active
room table because Duris3 is unlisted. No reverse WLD edge was identified in
the reviewed source. The candidate's local graph has three weak components
with sizes 10, 3, and 3. Directed traversal from #29500, #29510, and #29520
reaches the rooms in each corresponding component. There are no negative
targets, source rooms outside the #295 zone range, range-mismatch source
rooms, fall chances, or current fields.

All 29 target records are unresolved by the current active room table:
Legion's 26 local edges are source-only because Legion is unlisted, and the
three external targets are in unlisted Duris3. There are zero active inbound
edges to Legion. The three D4 exits are source-only outbound references, not
qualified live travel. The four type-25 objects separately author travel
destinations; they are not WLD exits and are excluded from the graph counts.

The portal placements and stored room values form two source-declared
reciprocal object-travel pairs across the three WLD components:

| Object placement | Stored destination | Pair |
| --- | --- | --- |
| #29500 in room #29503 | #29512 | with #29501 |
| #29501 in room #29512 | #29503 | with #29500 |
| #29503 in room #29502 | #29522 | with #29502 |
| #29502 in room #29522 | #29502 | with #29503 |

The pair destinations are encoded in the OBJ values and placements in the
ZON. This is source-declared intent only; runtime callbacks, teleport access,
and return behavior were not qualified. See the [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-496-legion-guildhall-inactive-source-graph-and-object-travel).

## Resets, imported objects, and shop products

The ZON header's reset mode is 0; the source comment in
[structs.h](../../../src/core/structs.h#L670) defines mode 0 as “Don't reset,
and don't update age.” The final header field 1 is not the reset mode. The
file contains 35 command records: 11 O, 13 M, and 11 G. The 13 M commands
place the candidate's Vuthen and golem prototypes in its own rooms. Four O
commands place candidate portal objects #29500–#29503.

Seven O commands import active Player Castles objects instead of local Legion
objects: #11054 in rooms #29500, #29510, and #29520; #11013 in #29506; and
#11104 in #29507, #29511, and #29521. These are the mouth, bonding altar,
and board prototypes owned by active [Player Castles](PLAYER_CASTLES_AUDIT_REVIEW.md#typed-dependencies-and-external-consumers).
Because Legion's ZON is absent from the full AREA list, these candidate
placements are source-only reset references. Preserve the exact imports and
their object ownership; in particular, Player Castles' #11013 “home” handler
is not a WLD exit.

The 11 G commands give Vuthen the same eleven products listed by the one
Legion shop record: #377, #3050, #398, #6089, #364, #53, #412, #347, #108,
#109, and #111. Ten have active OBJ prototypes: #377, #398, #364, #53, #412,
#347, #108, #109, and #111 are in active Heavens, while #3050 is in active
Verz1. #6089 has no checked-in OBJ prototype. Some of #53, #108, and #109
also occur in unlisted source files, but Heavens is their active-area
definition in the current manifest.

## Shop source and conditional loader path

The candidate SHP record is headed #29500, declares keeper #29500 in room
#29504, and lists the same eleven products as Vuthen's G resets. Both keeper
and room depend on unlisted Legion prototypes. The standard
[shop compiler](../../../areas/src/shp/make_shp.c#L6) reads its AREA list and
therefore excludes the unlisted candidate.

This checkout has no areas/world.shp. In non-mini mode,
[open_shop_stream](../../../src/economy/shop.c#L3267) first tries the generated
shop file and, if it cannot open it, globs areas/shp/*.shp. That conditional
fallback can read legion.shp without full AREA membership. This code path
does not establish that the record becomes a usable shop: its declared keeper
and room are candidate-only, and no runtime load was tested. Confirm the
deployed aggregate and fallback inputs with the shop and operations owners
before drawing a boot-time conclusion.

## Narrative, code, history, and disposition

The room prose presents a Legion guildhall and castle, an entry hall and
basement, a mist-cloaked pair of portal rooms, and separate ice and granite
caverns with golems and a demon statue. Vuthen is a dirty Duergar shopkeeper.
The source is narrative-bearing even though it has no local QST or story
sidecar. Compiled code contains one numeric room use in
[specs.assign.c](../../../src/specs/specs.assign.c#L2449): under a “Players
Guild” comment, it assigns the inn function to room number #29502. That line
does not identify which source-defined #29502 room was intended; AoD,
Genocide Hall, and Legion all define different rooms there, and none currently
provides an active room at that number.

Followed histories for the WLD, MOB, OBJ, ZON, and SHP remain on their Legion
paths through the 2009 area-tree move and later same-path edits. The MOB,
OBJ, and ZON histories also include the 2012 reset-format field update; the
WLD has a 2011 typo correction; and WLD, MOB, OBJ, ZON, and SHP have the
2013 generic “stuff” edit. The bounded history contains no direct rename,
retirement, or whole-area replacement decision. File history does not resolve
the map, prototype, reset, story, shop, editor/export, deployment, or saved
identity owners.

**Disposition: RETAIN IN PLACE.** Keep the five candidate files out of the
full area list and at their current paths while the builder resolves the
overlapping guildhall maps and typed IDs, the three Duris3 exits, the
Player Castles imports, the #6089 product, the conditional shop fallback, and
the #29502 code use. Preserve the source-only resets and portal pairs. Do not
use a shared number, an old same-path edit, or absence from the manifests as
permission to activate, merge, renumber, move, or archive the component. See
the [25 builder questions](LEGION_GUILDHALL_BUILDER_QUESTIONS.md),
[identity/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-496-legion-guildhall-source-identities-and-vnum-collisions),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-496-legion-retained-pending-map-reset-shop-and-code-ownership),
[candidate dependency row 59](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L59), and
[coverage row 409](ZONE_COVERAGE_REGISTER.csv#L410).

## Checkpoint 908: typed role refresh

The exact-number refresh across WLD, ZON, QST, story sidecars, SHP, typed OBJ
travel fields, and compiled source leaves the CP496 graph and identity findings
unchanged. Candidate WLD and ZON numbers occur only in Legion and the already
identified unlisted AoD, Genocide Hall, and RFOP sibling sources. Those map and
reset records remain source-local numeric matches; none adds active WLD ingress
into Legion. No QST or story-sidecar value matches the candidate band.

The four Legion type-25 portals retain their two source-declared local pairs
(#29503/#29512 and #29502/#29522). The sibling portal scan found AoD objects
#29501–#29504 storing #29513, #29509, #29519, and #29507; AoD WLD defines each
number in that same source map. RFOP object #29511 stores #29512, which is also
defined in RFOP WLD; RFOP #29510 stores #29505, outside Legion's room set. These
source-only numeric matches do not establish an active Legion route, even where
a different unlisted map also defines the same room number. No type-29 object
room target into #29500–#29523 was found.

The N-format shop roles also stay typed to their source files. Legion's SHP
header/keeper is mobile #29500 and its shop room is #29504; its product list
does not include Legion portal objects #29500–#29503. The AoD shop uses keeper
#29501 and AoD room #29503, and its product #29500 is AoD's badge object, not
Legion's same-number portal. Genocide Hall's shop keeper and room are both
#29503 in that source. These sibling shop files are unlisted. The current
checkout still lacks `areas/world.shp`, so the possible non-mini SHP glob
fallback remains conditional; no shop runtime was exercised.

The only exact compiled-code hit remains
[specs.assign.c](../../../src/specs/specs.assign.c#L2449), which assigns the inn
callback by numeric room #29502 under a Players Guild comment. It does not
identify Legion, AoD, or Genocide Hall as the intended room owner. The role
refresh found no additional direct code reference to the candidate identities.
Keep all source files **RETAIN IN PLACE**; the 25 builder questions, conditional
shop-input uncertainty, candidate graph metrics, and CP496 history conclusion
remain unchanged.

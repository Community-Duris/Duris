# Mara Guildhall source and dependency audit

Canonical key: component:mara_gh. Physical coverage row 418 / CSV line 419;
candidate dependency row 67 / CSV line 68. Reviewed October 11, 2026,
Checkpoint 505, against source snapshot
c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static inventory,
WLD-graph, typed-reference, identity, and bounded-history review. It does
not establish live loading, shop operation, portal travel, or runtime use.

## Inventory and narrative identity

The unlisted component contains `mara_gh.wld`, `.mob`, `.obj`, `.zon`, and
`.shp`; it has no same-stem QST or story sidecar and is absent from both
[areas/AREA](../../../areas/AREA) and
[areas/qst/AREA](../../../areas/qst/AREA). The zone header is #1, its raw
`.zon` title is `Guildhall`, and its highest room VNUM is #29617. The WLD
contains 16 rooms (#29600–#29617, with #29613 and #29614 absent), the MOB
contains 11 prototypes, the OBJ contains six prototypes, and the SHP file
contains one shop record, #29600. There is no native Q/Q-A contract.

The room and mobile prose describes a Marauders guildhall or lair: a broken
down tavern, Grand Hall and fountain, tavern, meeting room, treasure rooms,
a mist-covered chamber, a hidden approach to Tikitzopl's catacombs, and a
cleft leading deeper into a forest. Named or described characters include
Elrindias the seductress/shopkeeper, Rednax-Ela, a priestess, a cloaked man,
and a huge treant. The `.zon` title is generic; `Lair of the Marauders` and
related names in room/object text are descriptive evidence, not an
owner-confirmed canonical alias. The lack of a local QST file does not
establish that this story has no external owner or intended quest contract.

## WLD graph and external boundaries

The 16-room WLD has 29 authored directions. Twenty-six local directions form
13 reciprocal pairs across three weak components of 10, 3, and 3 rooms. From
#29600, authored WLD traversal reaches the 10-room guildhall core and does
not reach the other two groups. This is a WLD-only result; item teleports are
separate typed routes.

Three additional up exits leave the candidate WLD: room #29600 to Duris3
#156766 (`Scorching-Hot Lava`), #29610 to #124408 (`Grassy Plains`), and
#29617 to #163376 (`The Ocean`). All three targets are defined in unlisted
`Duris3.wld`, which is also outside `areas/AREA`; no Duris3 WLD exit returns
to a Mara Guildhall room. The overlapping `mara_op.wld` has another source
record from #29610 to #124408. These four source records are not current
active routes and do not change Duris3's 26 active-source unresolved exits
or its curated approaches into loaded areas.

No active WLD exit targets a Mara Guildhall room. Against the current full
area room table, zero of the 29 candidate targets resolve and all 29 are
positive unresolved targets: the 26 local destinations belong to this
unlisted WLD, and the three boundary destinations belong to unlisted
Duris3. There are no negative targets, fall-chance records, or current
records. Keep all WLD source, loaded-resolution, inbound/outbound, component,
and environmental metrics distinct from the item and code relationships
below.

## Resets, shop, portals, and compiled code

The unlisted [mara_gh.zon](../../../areas/zon/mara_gh.zon#L9) has 10 object
placements, 13 mobile resets, and 16 gear resets. Five object placements
import Player Castles objects: #11056 three times, #11013 once, and #11116
once; each has a tracked definition in `player_castles.obj`. Two local items
and the other local portal objects are placed in the guildhall's own rooms.
The mobile resets place Elrindias, the four hall characters, Rednax-Ela,
three pillars of gore, three treants, and a cloaked man. These are source-only
resets because `mara_gh` is not a current area-list input.

The [shop record](../../../areas/shp/mara_gh.shp#L1) is #29600, keyed to
Elrindias #29600 and room #29606. It offers 16 object VNUMs: local #29600 and
#29601 plus 14 external IDs (#53, #108, #109, #111, #347, #377, #398, #412,
#491–#494, #3050, and #6089). The matching gear resets reference those same
stock IDs. The tracked object scan found no definition for #6089. Other
external references have same-VNUM definitions in Heavens, Verz1, Player
Castles, Dalvik, Hostel, or Mavulsk; #53, #108, and #109 have multiple
tracked definitions under those different source identities. Heavens,
Verz1, and Player Castles are in `areas/AREA`; the Mara Guildhall shop and
its stock resets are not live inputs. Confirm each stock VNUM and owner
before treating the shop list as a resolved inventory.

Three local portal objects have type ITEM_TELEPORT. Static code reads
`value[0]` as the destination room VNUM
([teleport handler](../../../src/magic/spell_travel.c#L991)). The source
records give #29603 a destination of #29615, #29610 a destination of #29608,
and #29611 a destination of #29612. Their prose names Evermeet, Good, and
Khomani-Kan, respectively, while the VNUMs resolve in this source to `The
Bottom of the Cleft`, a mist-covered room, and the Blazing Vortex. Preserve
the literal data and ask the map/item owners to confirm these intended
relationships; do not infer travel to those named lands from the prose.

The separate unlisted `mara_op.obj` also defines #29610/#29611 with the same
numeric destinations, and its #29610 portal targets Mara Guildhall room
#29608, which is not defined in the Mara Outpost WLD. This is a source-only
cross-component item reference; it is not a WLD edge or proof that either
component is currently loaded.

Compiled code directly assigns the `inn` special to room #29605
([assignment](../../../src/specs/specs.assign.c#L297)). This is a static room
binding, not evidence the unlisted room is present in a live world table.
No QST giver or active ZON reset targets a Mara Guildhall prototype. The
unlisted Mara Outpost ZON does reuse VNUMs that collide with Mara Guildhall
room, mobile, and object definitions; keep its resets separate from active
use claims.

## Shared VNUMs, history, and disposition

The separately unlisted `mara_op` source family duplicates Mara Guildhall
WLD room VNUMs #29610–#29612, mobile VNUMs #29610/#29611, and object VNUMs
#29610/#29611. The room names and exits match but their room-flag fields
differ. The mobile prose is shared while stat fields differ, and the portal
object definitions differ in keywords/fields. Both zones contain source-only
resets for the shared VNUMs. These are same-number source definitions under
two stems, not proof that one file replaced the other; admitting both sets
without owner reconciliation risks choosing among conflicting definitions.

Inside `mara_gh.mob`, #29605 and #29610 have identical record bodies under
different VNUMs, as do #29606 and #29611. These text duplicates do not erase
their separate numeric identities or establish which reset/code roles should
remain. Followed Git history reaches the 2009 area-tree import; later tracked
edits include zone/shop additions, but no direct stem rename, retirement, or
whole-area replacement is established.

**Disposition: RETAIN IN PLACE.** Preserve all five source files outside the
area lists. Reconcile the Mara Guildhall and Mara Outpost owners, Duris3 map
owner, shop/item owners, Player Castles imports, special-assignment owner,
and editor/deployment copies before changing VNUMs, input lists, reset data,
portal destinations, or paths. Preserve the 29 WLD directions and keep the
four source-only Mara-to-Duris3 boundary records separate from active routes.
See the [20 builder questions](MARA_GH_BUILDER_QUESTIONS.md),
[Duris3 supplement](DURIS3_AUDIT_REVIEW.md#checkpoint-505-supplement-source-only-mara-guildhall-boundaries),
[Mara Outpost candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L69),
[coverage row 418](ZONE_COVERAGE_REGISTER.csv#L419), and
[candidate dependency row 67](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L68).

## Checkpoint 506 supplement: separate Mara Outpost source review

The dedicated [Mara Outpost audit](MARA_OP_AUDIT_REVIEW.md) confirms that
its three shared room IDs, two shared mobile IDs, and two shared object IDs
remain separate source definitions with differing fields. It also confirms
the source-only #29610 up exit to Duris3 #124408 and the #29610
ITEM_TELEPORT target to this map's room #29608. This adds no active WLD
inbound, live portal, or new Duris3 endpoint beyond the source records already
classified in CP505. Keep both components outside their current area lists
pending joint owner review; see the [Mara Outpost candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L69).

## Checkpoint 916: exact VNUM-role closure refresh

An exact #29600–#29617 scan across tracked WLD, MOB, OBJ, ZON, SHP, QST,
story, and source-code inputs remains bounded to the two unlisted Mara
families and one static code binding. WLD, MOB, and ZON hits belong to
`mara_gh` or the overlapping `mara_op`; OBJ hits belong to those same two
sources. The only SHP hit is Mara Guildhall shop #29600. No QST or story
source contains a candidate-range reference, and no other compiled source
reference was found beyond the existing `real_room0(29605)` inn assignment.

The candidate's 26 local WLD directions and three Duris3 exits are unchanged;
all three Duris3 targets remain in unlisted `Duris3.wld`, and no outside WLD
source or current active input adds an inbound route. Source-only Mara
Outpost resets and room/object/mobile collisions remain attached to their own
file identities. Mara Guildhall's #29600 shop and 16 goods remain source-only;
its three ITEM_TELEPORT values and missing tracked stock object #6089 are
unchanged. This scan adds no active dependency, shared-owner conclusion,
replacement evidence, or graph count. Preserve **RETAIN IN PLACE**, all 20
builder questions, and the owner review boundaries from CP505/506. No runtime
loading, shop use, or portal travel was tested.

# AoD GH source audit

Checkpoint 447 reviews unregistered `component:aod_gh`. This static source
review covers the room story, WLD reachability, competing map identities,
reset/shop dependencies, and bounded tracked history. It does not establish
loader membership or runtime travel. See the [story dossier](../zone-stories/AOD_GH.md)
and [builder questions](../zone-stories/AOD_GH_BUILDER_QUESTIONS.md).

## Inventory and loader status

| Source | Current evidence |
| --- | --- |
| [AoD WLD](../../../areas/wld/aod_gh.wld#L1) | 24 rooms, #29500–#29523, and 41 authored direction records. |
| [AoD MOB](../../../areas/mob/aod_gh.mob#L1) | Three prototypes, #29500–#29502. |
| [AoD OBJ](../../../areas/obj/aod_gh.obj#L1) | Five prototypes, #29500–#29504. |
| [AoD ZON](../../../areas/zon/aod_gh.zon#L1) | Header #296, title “AoD GH,” maximum #29523; 8 O, 13 M, and 16 G commands, no D commands. |
| [AoD SHP](../../../areas/shp/aod_gh.shp#L1) | One shop record, #29501. Valrok #29501 is the keeper in room #29503. |
| QST / map / story sidecar | No same-stem file; no local QST giver or native quest contract. |
| Current manifests | Absent from [areas/AREA](../../../areas/AREA#L1), [areas/qst/AREA](../../../areas/qst/AREA#L1), and the empty `areas/RANDOM_AREA`. |

The shop record is syntactically present in the source `.shp`, but the shop
generator reads the `areas/AREA` list and opens only the listed `.shp` files
([`make_shp.c`](../../../areas/src/shp/make_shp.c#L26), which reads `AREA`, then opens the named shop files at [lines 52–54](../../../areas/src/shp/make_shp.c#L52)). Since `aod_gh` is not
listed, this source record does not establish a currently generated shop.

## Authored setting and story

The first ten rooms make a desecrated mausoleum, with looted saint reliefs,
Troll and Ogre marks, a Chamber of Bones displaying war trophies, and an
inner sanctum used for political arguments and unholy rites. The supply depot
is a rough extension carved into the earth; Valrok, an old one-armed vampire,
waits there. The adjacent burial tomb repurposes noble sarcophagi as loot
storage. An Inn of Nightmares, a descent into the Netherworld, the Heart of
Onda, Clan Bloodlust imagery, and an altar to dark gods extend the afterlife
and faction themes through #29505–#29509.

Rooms #29510–#29513 shift into a guarded dark-jungle approach, a high canopy,
an immense hollow tree, and a waterfall and stream. The final group describes
a blood-soaked arena, the Great Hall of BLOOD BERZERS, a luxurious clan
gathering room, a citadel courtyard, a stable, an armoury, and Clan
Bloodlust's throne room, inner sanctum, and vault. The story is carried by
room prose, faction marks, Valrok, golem guardians, and reset inventory. The
source contains no local quest file or structured story sidecar.

## Room and prototype identities

AoD uses unique current ZON header number #296 and title “AoD GH.” The three
other current ZON files with header #295 are [`genocide_hall.zon`](../../../areas/zon/genocide_hall.zon#L1) (“Genocide Guild Hall”), [`legion.zon`](../../../areas/zon/legion.zon#L1) (“Legion Guildhall”), and [`void_gh.zon`](../../../areas/zon/void_gh.zon#L1) (“Main Guildhall for The Void”). Those shared reset-zone numbers do not make the maps the same zone.

AoD rooms have same-number records in three other unregistered maps:

| Source map | Shared room VNUMs | Same-number records matching AoD |
| --- | --- | ---: |
| [`genocide_hall.wld`](../../../areas/wld/genocide_hall.wld#L1) | #29500–#29504, #29510–#29511 | 0 of 7 |
| [`legion.wld`](../../../areas/wld/legion.wld#L1) | #29500–#29512, #29520–#29522 | 0 of 16 |
| [`rf_op.wld`](../../../areas/wld/rf_op.wld#L1) | #29510–#29512 | 0 of 3 |

The combined set is 11 distinct room numbers; each map record is a separate
identity. Forty direction records from those sibling sources numerically
target the AoD range (10 from `genocide_hall`, 26 from `legion`, and four from
`rf_op`). Each target is also defined as a room in its own source map, and
none of those source files is in `areas/AREA`; these are not active ingress
edges to AoD.

AoD mobiles #29500–#29502 also overlap mobile numbers in [`genocide_hall.mob`](../../../areas/mob/genocide_hall.mob#L1) and [`legion.mob`](../../../areas/mob/legion.mob#L1); their authored definitions differ. AoD objects #29500–#29503 overlap [`legion.obj`](../../../areas/obj/legion.obj#L1) and have different records. In particular, AoD #29500
is an obsidian Alliance of Darkness badge while Legion #29500 is a glowing
portal. Do not resolve either identity by number alone. No AoD room VNUM is
currently defined by an active WLD source.

The current ZON titles and header scan found no other #296 and no title match
for “AoD GH.” Tracked WLD history includes the 2009 directory move, a 2011
one-word correction in the Netherworld description, and a 2013 source
revision. MOB/OBJ/ZON include the 2012 reset-format edit; SHP has no such
edit. Commit `52bbee121`'s broad message mentions a “Temple of Flames” rename,
but its only AoD WLD change is `painfull` to `painful`; it does not show a
zone-title change. The 2026 ZON edit adds the four gathering-bag resets.
These tracked paths show no alternate-stem predecessor or whole-zone
replacement. This says nothing about untracked editor copies or persisted
state.

## World connectivity and reachability

The WLD-only graph has three weak components: ten rooms (#29500–#29509),
four (#29510–#29513), and ten (#29514–#29523). It has 41 exit records, no
negative targets, and two positive exits outside AoD's local VNUM set:

| Source record | Target record | Source identity |
| --- | --- | --- |
| #29500 down | #141442 | [`duris3.wld`](../../../areas/wld/duris3.wld#L870283), “The Ocean”; the map is not in `areas/AREA`. |
| #29520 down | #210208 | [`surf.wld`](../../../areas/wld/surf.wld#L214212) (“The Great Sea of Silvanos”) and [`surface2011.wld`](../../../areas/wld/surface2011.wld#L214369) define this number differently; neither map is in `areas/AREA`. |

From #29500 all ten rooms in the first component are reachable by directed
authored exits; from #29510 all four rooms in the jungle component are
reachable. From #29514 only eight of the last ten rooms are forward-reachable:
#29516 and #29518 cannot be reached through that component's authored
direction records. No active WLD exit targets any AoD room number. A reverse
source scan found no WLD exit back into AoD's range. The 40 sibling-map
matches above remain their own inactive map records.

The four type-25 AoD objects store destination values #29513, #29509, #29519,
and #29507. Only object #29502 is reset by AoD's ZON, in #29513. These values
and portal descriptions identify authored travel intent, but no AoD-specific
object callback or loaded destination was established. Keep object-triggered
travel separate from the WLD graph and leave actual travel/return behavior
unqualified.

## Typed dependency census

AoD's eight O resets include seven Player Castles object references: #11061
twice, #11111 twice, #11013 once, #11062 once, and #11112 once. The referenced
prototypes are in active [`player_castles.obj`](../../../areas/obj/player_castles.obj#L121) (#11013, #11061, #11062, #11111, and #11112); these AoD reset commands are
source-only. The remaining O command places local portal object #29502 in
#29513. The 13 M commands place local Bone Golem Guardian variants and Valrok.
Sixteen G commands place the listed stock on Valrok; the matching shop
inventory also has 16 products.

The stock IDs are #377, #3050, #398, #6089, #364, #53, #412, #347, #108,
#109, #111, #29500, and #491–#494. #6089 has no object definition under
`areas/obj`, so its reset and shop-product references remain unresolved at
source level. #491–#494 are defined in active `heavens.obj` as gathering
containers in active [`heavens.obj`](../../../areas/obj/heavens.obj#L5055), consistent with the 2026 AoD reset addition. #3050 is defined in
[`verz1.obj`](../../../areas/obj/verz1.obj#L850); #53, #108, and #109 have definitions in more than one source
file. AoD #29500 is sold and reset locally, while the same object number has
a different Legion definition. These source matches do not establish which
prototype the unlisted shop would resolve at runtime.

Static C/C++ scanning found one numeric room binding in this range:
[`specs.assign.c`](../../../src/specs/specs.assign.c#L2449) assigns the `inn`
function to `real_room0(29502)` under a Players Guild comment. AoD #29502 is
the Inner Sanctum; Legion #29502 is a different room, and Genocide Hall also
defines #29502. None is currently loaded from `areas/AREA`, so the code line
does not identify an active destination. No other compiled reference in the
range was found. No local QST or structured story sidecar exists.

## Disposition

Keep candidate row 9, `aod_gh`, **RETAIN IN PLACE**. Preserve the seven
Player Castles imports and the local resets, shop source, faction story,
room/prototype collisions, and unlisted outbound map targets together. Before
any later move or loader change, resolve the #295 room and mobile/object
ownership, the duplicate #295 zone headers, the `real_room0(29502)` callback,
the #6089 product, the #210208 source-map identity, and the intended status
of the disconnected/unreachable room group. External editor names, persisted
instances, and runtime resolution remain outside this static census. See
[candidate row 9](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L9),
[coverage row 360](ZONE_COVERAGE_REGISTER.csv#L360), and the
[builder questions](../zone-stories/AOD_GH_BUILDER_QUESTIONS.md).

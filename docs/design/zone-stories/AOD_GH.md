# AoD GH

Canonical key: `component:aod_gh`. Physical coverage row 360. This is a
source-only guild hall: its five native files are absent from the full
`areas/AREA` loader list. The [technical audit](../quest-story-followups/AOD_GH_AUDIT_REVIEW.md)
records the map graph, competing #295 identities, shop/reset references, and
open builder decisions.

## The authored place

AoD GH joins a desecrated mausoleum to the social and military spaces of the
Alliance of Darkness. The entrance's looted reliefs and Troll and Ogre marks
lead into a Chamber of Bones decorated with the remains and trophies of
defeated enemies. The Inner Sanctum turns a sarcophagus into a political
meeting table; nearby storage, a vampire's supply counter, and the Burial
Tomb make the mausoleum feel occupied and repurposed. Valrok, a deformed old
vampire with one arm, serves as the shopkeeper in a dusty depot.

The rooms deepen the afterlife imagery through an Inn of Nightmares, a
Netherworld descent, the Heart of Onda, Clan Bloodlust symbols, and an altar
for dark rites. Bone golems guard the approaches. The source then changes
setting into a dark jungle with a canopy, a hollow tree, and a waterfall. Its
last rooms present an arena, the Great Hall of BLOOD BERZERS, a clan gathering
space, a sword-citadel courtyard, a stable and armoury, and a throne room,
inner sanctum, and vault.

There is no local QST file or structured story sidecar. Story comes from the
room prose, clan and religious imagery, Valrok, and the guard/stock resets.
The source shop lists sixteen products, including #6089, which has no OBJ
definition in the repository. The `.shp` file has one record, but the shop
generator does not include AoD while the stem is absent from `areas/AREA`.

## Distinct map identities

AoD has 24 rooms and 41 authored exits in three weak components of 10, 4, and
10 rooms. The first mausoleum group (#29500–#29509), jungle route
(#29510–#29513), and citadel group (#29514–#29523) do not connect through
ordinary authored WLD exits. The citadel root #29514 reaches eight of its ten
rooms by directed exits; #29516 and #29518 are outside that forward reach.
Two down exits point to unlisted maps: #29500 to Duris3 #141442 and #29520 to
#210208, which has different definitions in Surf and Surface2011. Those
source records do not prove active access or safe return.

Other unlisted guild halls reuse the same room numbers with different text.
Genocide Hall shares seven IDs, Legion shares sixteen, and `rf_op` shares
three; none of their same-number records matches AoD. The three halls
`genocide_hall`, `legion`, and `void_gh` also use ZON header #295, while AoD
uses #296. Keep source ownership explicit around these collisions and the
Players Guild inn callback at numeric room #29502.

## Keep the relationships visible

AoD ZON imports seven Player Castles objects, resets local mobs and a local
portal, and gives Valrok sixteen objects that duplicate the shop stock list.
The portal objects store #29513, #29509, #29519, and #29507 as destination
values; only object #29502 is reset locally. Static code binds an inn
function to `real_room0(29502)`, but the source-only room definitions at that
number compete across AoD, Legion, and Genocide Hall. Preserve those numbers
and identities as separate source evidence.

Keep `aod_gh` **RETAIN IN PLACE** until the owner confirms the room and
prototype identities, zone numbering, shop inventory including #6089, and
the source map for #210208. The [technical audit](../quest-story-followups/AOD_GH_AUDIT_REVIEW.md#disposition),
[builder questions](AOD_GH_BUILDER_QUESTIONS.md),
[candidate row 9](../quest-story-followups/ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L9),
and [coverage row 360](../quest-story-followups/ZONE_COVERAGE_REGISTER.csv#L360)
preserve the current source boundary.

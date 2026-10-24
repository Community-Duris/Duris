# Jade Empire: directed connectivity and typed-closure supplement

This repository-wide supplement reconciles the active Jade WLD graph, all
current unregistered WLD inputs, checked-in typed references, and tracked title
history with the existing [source-comprehensive Jade dossier](../zone-stories/THE_JADE_EMPIRE.md).
No native area or gameplay source was changed or qualified.

## Local directed reachability

The active `jade.wld` has 340 rooms and 740 authored exits: 736 local and four
outbound. Every exit target resolves. The local weak components have sizes
327, 7, and 6; the strongly connected components have sizes 319, 7, and 6,
plus eight singleton components.

Five active inbound records lead to four unique local arrivals: #76616, #76620,
#76760, and #76928. Each arrival lies in the 319-room strongly connected core
and reaches all 319 core rooms through authored local exits. From each arrival,
325 of 340 rooms have an authored path back. The 21 rooms outside forward
reach are #76844, #76862–#76868, #76909–#76912, #76915–#76922, and #76939.
Six of those rooms (#76844, #76909–#76911, #76915, and #76939) have authored
paths back to the core; the other 15 are the two detached 7- and 6-room
components plus singleton rooms #76912 and #76916. These counts describe WLD
topology only. They do not establish that doors, keys, resets, item routes,
travel services, or live player movement make each room available or safe.

## Active cross-area boundaries

The five active inbound records are:

| Source | Arrival | Register |
| --- | --- | --- |
| Storm Port Stronghold #22649 northwest | Jade #76616 | [row 378](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L378) |
| Winterhaven #55632 northwest | Jade #76760 | [row 785](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L785) |
| Surface #642099 south | Jade #76928 | [row 1675](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1675) |
| Surface #642498 east | Jade #76620 | [row 1677](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1677) |
| Surface #642899 north | Jade #76620 | [row 1678](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1678) |

The four Jade departures are #76620 south to Surface #642899, #76620 west to
#642498, #76883 up to #642501, and #76928 north to #642099 ([outbound rows
883–886](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L883)). The first, second, and
fourth have reciprocal active records. #76883 to #642501 has no active WLD
return. Pair summaries record the three Surface pairs and the one-way Storm
Port Stronghold and Winterhaven arrivals ([rows 210, 450, 521, and 797](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L210)).
There is no direct active WLD boundary between Jade, Mitashi, and the Rice
Fields contributor `jademini`.

## Source-only inactive-WLD approaches

The all-95 unregistered-WLD scan parsed 2,080,773 authored exits with no parse
errors and found no room-VNUM overlap with Jade. It found two previously
uncatalogued exits from `surf.wld` into active Jade rooms:

| Source-only record | Direction and target | Evidence |
| --- | --- | --- |
| Surf room #278596 | South (`D2`) to Jade #76928 | [exit target](../../../areas/wld/surf.wld#L1649276) |
| Surf room #279196 | North (`D0`) to Jade #76620 | [exit target](../../../areas/wld/surf.wld#L1661848) |

Both records remain source-only and do not add active ingress. Preserve them
with the existing Surf candidate record [row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).
They raise the reviewed Surf subset from 227 to 229 and the curated total from
3,469 to 3,471; the other curated subsets remain unchanged.

## Typed dependency closure

Jade's own reset file imports eight currently defined external object
prototypes: Heavens #252, #253, #336, and #359; Winterhaven #55025 and #55170;
Tharnadia #132581; and Unique #67212. The #132581 reset was added when the Jade
bank was changed to use the Tharnadia banking object. Jade's reset file also
names object #6109 at [line 467](../../../areas/zon/jade.zon#L467), but no
current object prototype with that VNUM was found in the complete 442-file
`areas/obj` set; confirm its intended disposition rather than treating it as
an available source.
[Heavens definitions](../../../areas/obj/heavens.obj#L2770) ·
[Winterhaven definitions](../../../areas/obj/wh.obj#L269) ·
[Tharnadia banking object](../../../areas/obj/tharnadia.obj#L1373) ·
[Unique definition](../../../areas/obj/unique.obj#L205) ·
[Jade reset records](../../../areas/zon/jade.zon#L385)

Four active outside reset files load Jade object prototypes: Cerebusp loads
#76713 ([reset](../../../areas/zon/cerebusp.zon#L418)); Rogue Rai puts #76711
inside container #75816 ([reset](../../../areas/zon/roguerai.zon#L80)); Storm
Port Stronghold loads #76712 ([reset](../../../areas/zon/spshold.zon#L159)); and
Vehicles loads #76634 ([reset](../../../areas/zon/vehicles.zon#L845)). No
outside active or unregistered ZON reset targets a Jade mobile prototype.

Seven external native QST records reference Jade items: Hunt's #76613, Cosmic's
#76631, Divine Home's #76697, Cloister's #76728, Winterhaven's #76634, and two
Rice Fields contracts that output Jade map #76676 and consume Jade sprite
#76670 ([Hunt](../../../areas/qst/hunt.qst#L87),
[Cosmic](../../../areas/qst/cosmic.qst#L161),
[Divine Home](../../../areas/qst/divhome.qst#L395),
[Cloister](../../../areas/qst/cloister.qst#L5),
[Winterhaven](../../../areas/qst/wh.qst#L1182), and [Rice Fields](../../../areas/qst/jademini.qst#L5)).
Structured outside story records reference the corresponding items in Hunt,
Cosmic, Divine Home, Cloister, and Winterhaven; Cloister also names Jade mobile
#76720 as Brother Mande ([contact](../../../areas/story/cloister.story.json#L145),
[item step](../../../areas/story/cloister.story.json#L184)). The Rice Fields
sidecar documents its Jade sprite transfer ([line 222](../../../areas/story/jademini.story.json#L222)).
In the reverse direction, Jade's story uses Rice Fields contacts and prototypes
#77214, #77202, #77204, #77205, and #77200; see the [Rice Fields audit](RICE_FIELDS_AUDIT_REVIEW.md#typed-item-reset-story-and-compiled-references).

An exact current-source inventory found no external SHP product referencing a
Jade object, no duplicate Jade mobile/object VNUM in another current
`areas/mob` or `areas/obj` file, and no external type-25 destination or random
zone seed into a Jade room among 872 parsed declarations. The typed scans also
found direct compiled references in the CHAOS equipment table, computed
metalsmith table, ship-shop assignment, and ferry-stop table; the dossier
documents those effective uses and service limits ([CHAOS data](../../../src/account/chaos_eq_data.h#L74),
[smith table](../../../src/economy/tradeskill.c#L205),
[ship-shop binding](../../../src/specs/specs.assign.c#L2442),
[ferry stops](../../../src/world/ferryact.c#L101)). These source references do
not prove a reset, transaction, item grant, travel, or quest ran.

## Title and tracked history

The normalized active title “The Jade Empire” has one owner among the 351
current active `.zon` titles ([title](../../../areas/zon/jade.zon#L2)). Followed
WLD, MOB, OBJ, QST, SHP, and ZON paths retain the `jade` stem after the tracked
2009 `areas/` move and subsequent same-stem updates. The Jade story sidecar was
added in 2026 by commit `d1ecffdec`; the reviewed paths contain no alternate-stem
rename, distinct predecessor, or whole-zone replacement.

Mitashi's “Capital City of the Jade Empire” title belongs to its separate
active `mitashi` contributor and room range; “The Rice Fields” belongs to
`jademini` with its own source set. Typed item and story links establish
authored relationships, while the active WLD registers show no direct Jade
boundary to either contributor. None is a documented Jade replacement or
duplicate. See the [Mitashi title](../../../areas/zon/mitashi.zon#L2),
[Rice Fields title](../../../areas/zon/jademini.zon#L2),
[Mitashi history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-124-mitashi-and-the-duris3-map-identity),
and [Rice Fields audit](RICE_FIELDS_AUDIT_REVIEW.md#title-and-bounded-history).

## Builder and migration boundary

The Jade dossier contains seventeen explicit builder questions and
recommended resolutions alongside its 17 named outcomes, supporting
exchanges, source/access map, exclusions, and capability matrix. Preserve the
active `jade` contributor **DO NOT MOVE**. Do not add a Jade attic candidate.
Keep both newly reviewed Surf approaches on the existing Surf row 97. Preserve
the listed external reset, quest, sidecar, and compiled owners if any separate
map proposal is considered. Dynamic access, fresh stock, paid-service
availability, player attribution, and safe travel remain separate from this
static source review.

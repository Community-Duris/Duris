# Asunwa source audit

Canonical key: `component:asunwa`. Physical coverage row 363; attic candidate
row 12. The WLD contributor is commented out of `areas/AREA`, while
`areas/qst/AREA` still lists Asunwa and an Asunwa shop record remains in the
standalone SHP source. These mixed source registrations make the candidate
more than an unused map file; actual aggregate and runtime state are not
available in this checkout.

## Source identity and loader boundaries

The full-area manifest retains the commented annotation
[`*asunwa *353-354`](../../../areas/AREA#L170). The separate QST manifest has
an active `asunwa *353-354` entry
([line 157](../../../areas/qst/AREA#L157)). The QST source defines Balgar's
contract under mobile #35319, but the repository does not contain the
aggregate `areas/world.qst` which `boot_the_quests()` opens. The quest loader
does not glob `areas/qst/*.qst`
([loader](../../../src/world/quest.c#L2086)). Manifest membership therefore
records source intent without proving the contract is in a live quest index.

The standalone shop file defines one shop for Anan, mobile #35320, in room
#35374 and stocks objects #35311, #35304, and #35301
([shop record](../../../areas/shp/asunwa.shp#L1)). The aggregate
`areas/world.shp` is absent from this checkout. If the shop boot uses its
fallback, [`open_shop_stream()`](../../../src/economy/shop.c#L3267) globs
`areas/shp/*.shp`; the Asunwa shop is among those source files regardless of
the commented WLD manifest entry. The fallback behavior does not prove that
the keeper, room, or stock resolve in a live world.

There is no `areas/map/asunwa.json`, `areas/story/asunwa.story.json`, or
separate story document in the source tree. The six listed files are WLD,
MOB, OBJ, ZON, QST, and SHP. No random-area membership is recorded.

## Quest and environmental story

The title normalizes to “Lair of the Asunwa Jaguar.” The entrance at
[`#35300`](../../../areas/wld/asunwa.wld#L1) is a mountain plateau where twin
jaguar statues guard a vine-covered dark cave. The route passes slick caverns,
an old altar with burning coals and herbs, a crashing waterfall, and a hidden
valley where water feeds pools, a river, dense tropical growth, and bamboo
forest. A large flower-filled hut contrasts with the dark secret passages
carved in dark stone and flecked with gold. The river and forest support
nymphs, merfolk, bats, monkeys, gorillas, ocelots, leopards, jaguars, and
pandas.

The WLD places two druids in the valley. Anair has a cared-for garden home at
[#35310](../../../areas/wld/asunwa.wld#L216); Riana's home at #35314 is
neglected. In the QST text, Balgar explains that the jungle is ruled by a
jaguar family and that an earlier people worshipped a jaguar king. The reset
places the jaguar king Asunwa at the clearing's stone structure (#35405),
Queen Sajana in a lavish forest cavern (#35389), and Captain Irom in the
secret-passage throne room (#35396). Four following resets pair Tylara with
the river-nymph king Hytar and jaguar cubs with Sajana.

Balgar's QST has two keyed responses: one explains the jaguar statue's
history; the other asks for the missing ancient scroll and promises a jade
plaque. The source contract matches that text. In the QST format, G item
#35315 is the requested ancient scroll, while R item #35313 and R experience
10,000 are rewards. [`tell_quest()`](../../../src/world/quest.c#L1845) prints
the G list as “I'm looking for” and the R list as “And I will reward”; the
loader maps those commands into the corresponding lists
([G parser](../../../src/world/quest.c#L2202),
[R parser](../../../src/world/quest.c#L2243)). The ZON resets the scroll in
the throne room (#35396) and gives Balgar the jade plaque at #35388
([QST](../../../areas/qst/asunwa.qst#L1),
[scroll reset](../../../areas/zon/asunwa.zon#L43),
[Balgar reset](../../../areas/zon/asunwa.zon#L170),
[plaque reset](../../../areas/zon/asunwa.zon#L172)). The source-control history
also records the plaque being restored to the quest in 2016.

The hidden paths have two item-key relationships: jade jaguar statue #35307
is the WLD key for the locked passage from #35349, and Balgar carries tiny
jade key #35312 for the secret passage at #35399. The ZON places the statue in
the altar cavern (#35334) and the tiny key on Balgar. Keep those object,
reset, and exit identities attached to their source locations.

## Static graph, hazards, and boundaries

| Measure | Source result |
| --- | ---: |
| WLD rooms | 108 (#35300–#35407) |
| Authored directions | 267: 266 local and one outbound |
| Local weak components | 1, containing all 108 rooms |
| Forward reach from entrance #35300 | 106 / 108 |
| Rooms with an authored local return to #35300 | 108 / 108 |
| WLD fall-chance rooms / current rooms | 6 / 14 |
| Flagged/locked/secret exits with ZON door resets | 8 / 8, all reset at 100% |

The two rooms outside forward reach from #35300 are #35406 and #35407, both
named “Rare Mob Load Room.” Their small one-way loop reaches #35311 but has no
local WLD entrance from the main route; ZON resets a snow leopard in #35406.
Every room can still return to the entrance by the authored local direction
graph. This graph omits door state, fall/current effects, resets, and any
scripted behavior; it is not a player-access or safe-return finding.

The six `F` fields put 30% fall chances on ledges #35324, #35329, and #35330
into the pool, and 25% chances on #35348–#35350 into the lower chasm. Fourteen
`C` fields set current speeds and directions along the waterfall, river, pool,
and chasm rooms. The source prose itself warns of slippery rocks and stronger
current at the pool lip. These authored settings need gameplay-owner review
before anyone describes the route as safe.

Room #35300 has one north exit to active Surface #604903 and one south exit
into Asunwa #35301. Surface is the only active WLD owner of #604903, though
source-only `newbiemaps` and `surface2011-temp` also define that number. The
active Surface room has no exit back to an Asunwa VNUM. Separately, unlisted
`surf.wld` room #247317 has a south exit to Asunwa #35300
([Surf record](../../../areas/wld/surf.wld#L992833)). That approach is
source-only and is not active ingress. No current active WLD direction targets
Asunwa's #35300–#35407 identities; candidate active inbound remains zero.

No source exit has a negative or unresolved positive target, no room lies
outside the numeric zone range, and no exit originates from a range-mismatch
room. The full inactive-WLD recount already includes Asunwa's one outbound
direction to active Surface; the overall 11,987 / 415 Surf totals do not
change. See [WORLD_CONNECTIVITY.md](WORLD_CONNECTIVITY.md#checkpoint-450-asunwa-jaguar-lair-boundaries-and-hazards).

## Typed dependency census

The ZON has 150 commands: 107 mobile resets, 23 equipment resets, eight door
resets, five give resets, four follow resets, and three object-in-room resets.
The 25 local mobile and 18 local object prototypes all have same-type
definitions in their own Asunwa files; a repository scan found no duplicate
mobile or object prototype definition at those exact IDs in another source.
All source ZON reset mobile/object IDs resolve within those local sets.

Typed references cross the component boundary:

| Source | Numeric relationship | Qualification |
| --- | --- | --- |
| `areas/shp/ddguild.shp` | Keeper mobile #35300; stock object #35301; room #35303 | The IDs match Asunwa's jaguar-man guard, claw armor, and cavern room. This shop source is separate from Asunwa. |
| `areas/shp/player_guilds.shp` | Keeper mobile #35300; stock object #35301; room #35303 | Same numeric mobile/object/room references in a second shop record; preserve as a separate source relationship. |
| `areas/qst/player_guilds.qst` | Empty QST key #35320 (`#35320` followed directly by `S`) | Shares Anan's mobile number but contains no quest text or contract. |
| `areas/wld/surf.wld` | Room #247317 D2 → #35300 | One source-only approach to the unlisted Asunwa map. |

The exact typed shop fields are visible in the
[ddguild record](../../../areas/shp/ddguild.shp#L1),
[Player Guilds record](../../../areas/shp/player_guilds.shp#L507), and
[empty Player Guilds QST record](../../../areas/qst/player_guilds.qst#L1).
No external ZON reset uses Asunwa's local mobile or object prototypes. No
compiled source callback was found for the local mobile/object VNUMs. Numeric
matches in unrelated object value/cost fields are not treated as typed
references.

The shop/QST registration split remains a source boundary, not a gameplay
finding: shop fallback globbing is visible in source, but `areas/world.shp`
and `areas/world.qst` are absent from this checkout and no game was booted.
Actual shop stock resolution, QST boot membership, mobile loading, and player
availability remain unqualified.

## Title, history, and safe disposition

The normalized ZON title “Lair of the Asunwa Jaguar” and zone header #353 are
each unique in the current source scan. Followed WLD/MOB/OBJ/ZON/QST/SHP
history reaches the 2009 area-tree move and records later room flags, the
2012 reset-field update, 2014 shop cleanup and rare-ocelot move, 2015 quest
item flags, and the 2016 plaque restoration. These same-stem histories show
no explicit alternate-stem predecessor or whole-zone replacement.

Keep candidate row 12 **RETAIN IN PLACE**. Preserve the commented full-area
entry, active QST manifest entry, shop fallback source, 108-room story and
hazards, Surface boundary, Surf approach, two external SHP references, and
the empty same-number Player Guilds QST key. Do not move or archive the WLD,
MOB, OBJ, QST, or SHP files until the owner resolves manifest intent,
external shop ownership, aggregate-file generation, and editor/persisted/live
state. See the [story dossier](../zone-stories/ASUNWA.md),
[builder questions](../zone-stories/ASUNWA_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-450-asunwa-title-history-and-source-identities),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-450-asunwa-retained-with-mixed-manifest-dependencies),
[candidate row 12](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L12), and
[coverage row 363](ZONE_COVERAGE_REGISTER.csv#L363).

# Arachdrathos Wilderness (`arac_wild`) source story dossier

This dossier maps the authored environmental story in the active wilderness,
its half-drow enclave, the mountain glacier, and the Crescent Chasm. The
[Checkpoint 184 audit supplement](../quest-story-followups/ARAC_WILD_AUDIT_REVIEW.md)
records the complete active graph, its city connections, source-only Surf
approach, typed inbound references, title scan, and bounded history.

## Story shape in the authored rooms

### A forest route to a half-drow refuge

The surface approach follows damp evergreen and granite trails. At the fork in
room #36006, the southern route descends into a lush, waterlogged valley and a
granite ravine; the eastern route climbs toward the mountain. A small cave at
the ravine's end carries faint, indistinct voices. Its tunnels lead into a
Half-Drow enclave whose rooms are small, wet, and carefully kept
([fork](../../../areas/wld/arac_wild.wld#L124),
[cave entrance](../../../areas/wld/arac_wild.wld#L372),
[enclave entrance](../../../areas/wld/arac_wild.wld#L456)).

The enclave's shaman keeps divination-like cave paintings, books, and ritual
vessels ([shaman's cave](../../../areas/wld/arac_wild.wld#L569)). Its audience
chamber has three granite thrones, while the queen's bedroom depicts violent
acts against the Drow and displays symbols of Xyzom, Lord of Discord
([audience chamber](../../../areas/wld/arac_wild.wld#L586),
[queen's bedroom](../../../areas/wld/arac_wild.wld#L612)). Mobile descriptions
make the political conflict explicit: Ylani rules as queen, her consort is a
Drow taken as a slave, and the Half-Drow warriors plan to attack Arachdrathos
and reclaim part of the city. Ylani's description also lays out a coercive,
two-phase breeding program; the child in the enclave is described as future
“breeding stock” ([Ylani](../../../areas/mob/arac_wild.mob#L16),
[consort](../../../areas/mob/arac_wild.mob#L32),
[warrior](../../../areas/mob/arac_wild.mob#L76),
[child](../../../areas/mob/arac_wild.mob#L107)). The source establishes their
goal and the abuse within their community, but gives the player no recorded
role in changing either.

### The glacier, the shadow dragon, and a warned descent

The other branch climbs above the tree line onto slick, red-veined black
marble, then crosses a severe glacier and its black-ice cornice. Scratch marks
record earlier attempts to scale the cliffs. An isolated cave at the summit
contains a nest and the sleeping shadow dragon; the reset places a shadow mask,
scale, and gem with it ([mountain trail](../../../areas/wld/arac_wild.wld#L970),
[cornice](../../../areas/wld/arac_wild.wld#L1165),
[lair](../../../areas/wld/arac_wild.wld#L1275),
[dragon and loot resets](../../../areas/zon/arac_wild.zon#L146)).

A separate blasted opening in the glacier descends through steep shafts into
the mountain. A warning object at the opening says the drop is possible but
handholds are scarce and falling is likely
([opening](../../../areas/wld/arac_wild.wld#L1418),
[warning object](../../../areas/obj/arac_wild.obj#L276),
[reset](../../../areas/zon/arac_wild.zon#L9)). Below it, polished tunnels
descend through a fetid pile of bodies to the Bloody Ledge. A second warning
there describes the danger beyond the ledge
([gore pile](../../../areas/wld/arac_wild.wld#L2333),
[ledge](../../../areas/wld/arac_wild.wld#L2359),
[warning object](../../../areas/obj/arac_wild.obj#L268),
[reset](../../../areas/zon/arac_wild.zon#L10)).

### The Crescent Chasm and its links to Drow settlements

The 77-room Crescent Chasm is cut through black, red-veined stone. Its air
surges through a huge arc; the floor and a blood-covered spire below are
littered with bones and marks left by creatures that fell from above. Chasm
demons inhabit the shafts and ledges, while driders—described as Drow
transformed by Lolth—occupy the lower tunnels. At the bottom, Durithalite
elementals stand in a glowing red cavern
([spire](../../../areas/wld/arac_wild.wld#L4434),
[lower shaft and bones](../../../areas/wld/arac_wild.wld#L5224),
[drider](../../../areas/mob/arac_wild.mob#L219),
[Durithalite cavern](../../../areas/wld/arac_wild.wld#L5419),
[elemental](../../../areas/mob/arac_wild.mob#L236)). The source suggests a
history of lethal falls, but does not say who caused them or why.

Two active passages tie the lower chasm to Drow districts: a blasted opening
east from the black-crystal cavern enters Arac-Web, and a Durithalite boundary
connects the southern cavern run to Llzazan Ghetto. A silk-paper map item pictures Arachdrathos
and labels its noble houses, main city, Llzazan Ghetto, Sa'Zarn, farms, and
slave camp ([black-crystal cavern](../../../areas/wld/arac_wild.wld#L5391),
[map item](../../../areas/obj/arac_wild.obj#L238),
[Arac-Web boundary](../quest-story-followups/ARAC_WILD_AUDIT_REVIEW.md#active-wld-graph-and-source-only-approach),
[Llzazan boundary](../quest-story-followups/ARAC_WILD_AUDIT_REVIEW.md#active-wld-graph-and-source-only-approach)).
The map is included in the Drow and Drider basic item arrays, but is not loaded
by this area's reset file ([Drow kit](../../../src/account/newbie_kit_plan.c#L306),
[Drider kit](../../../src/account/newbie_kit_plan.c#L1397)).

The area has no local QST file or contracts, no SHP file, and no story sidecar.
The forest's political conflict, dragon, and chasm are authored encounters;
the available sources do not record a rebellion outcome, dragon quest result,
or persistent chasm story state.

## Builder questions and recommended resolutions

| Question for the area owner | Source evidence | Recommendation |
| --- | --- | --- |
| Is Ylani's plan to reclaim part of Arachdrathos intended to be a player-facing conflict? | Mob text describes an invasion goal and a coercive breeding plan, but the active zone has no local QST contract or recorded outcome. | Decide whether the enclave is meant to remain a hostile encounter or support player choices. If it should resolve, specify an outcome that addresses the source's abuse and the Half-Drow's political goal before adding a quest. |
| Is the silk-paper map meant to guide new Drow and Drider characters toward the city and its districts? | Object #36016 diagrams Arachdrathos and its surrounding districts and appears in both basic kit arrays. | Preserve the map-to-kit connection and verify its labels against current district ownership before changing either source. |
| Are the warning objects meant to communicate specific climb or fall mechanics? | Objects #36017 and #36018 warn about the Bloody Ledge and glacier descent; the source files do not qualify live movement outcomes. | Confirm the intended risk and in-game messages in an authorized gameplay review before describing the route as safe, lethal, or gated. |
| Is room #36171 an intended landing point for remote travel? | Three compiled travel tables include this Crescent Chasm room as a possible destination. | Preserve the endpoint when reviewing ownership or danger. Confirm arrival safety and intended access before changing those lists. |
| Is the shadow dragon intended as a stand-alone boss encounter or part of a larger story? | The reset equips the dragon with three shadow-themed items, but the zone has no local quest result. | Keep the encounter and loot mapping together; document any intended consequence before adding a separate story contract. |
| Should `arac_wild` have any command-target alias beyond its normalized title “Arachdrathos Wilderness”? | The title is unique among the 442 scanned `.zon` files; Arachdrathos – Drow City, Guilds, and Llzazan Ghetto are distinct neighboring titles. The bounded source review does not establish an additional wilderness command alias. | Keep `arac_wild` and “Arachdrathos Wilderness” as the source key and display identity. Add an alias only after the owner names its exact target and intended distinction. |

Keep the zone in the active map. The audit supplement records its five
reciprocal active boundaries and the distinct inactive Surf record; neither is
evidence of a replacement or migration clearance.

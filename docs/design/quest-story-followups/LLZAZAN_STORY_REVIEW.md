# Llzazan (`llzazan`): source story review

**Scope:** source review of 197 WLD rooms, 45 mobile prototypes, 53 local
objects, 273 reset commands, four shop records, and the authored story sidecar.
This maps environmental narrative, static routes, and typed source references;
it does not qualify live combat, shop behavior, special procedures, or safe
traversal.

## Story coverage state

`llzazan` is an active `areas/AREA` contributor (`*362-363`), with zone header
362 and title Llzazan Ghetto of Arachdrathos
([AREA entry](../../../areas/AREA#L174)). The register counts 197 rooms, 45
mobiles, 53 objects, no native Q/QA contracts, and now four shops
([coverage row](ZONE_COVERAGE_REGISTER.csv#L162)). `areas/qst/AREA` also lists
Llzazan, but there is no `areas/qst/llzazan.qst` file; index membership alone
does not supply local quest definitions ([quest index](../../../areas/qst/AREA#L161)).

The sidecar says `coverage: complete` and invites players to meet residents to
“uncover local requests,” but its `contacts`, `stories`, and `exclusions`
arrays are empty ([coverage](../../../areas/story/llzazan.story.json#L5),
[introduction](../../../areas/story/llzazan.story.json#L8),
[contacts](../../../areas/story/llzazan.story.json#L11),
[stories](../../../areas/story/llzazan.story.json#L12),
[exclusions](../../../areas/story/llzazan.story.json#L13)). The room and mobile
text offers several social and historical hooks, but the reviewed files map no
local QST contract or player objective. Special procedures and other runtime
behavior remain outside this source pass.

## Environmental story

The source describes an underdark settlement shaped by a severe class divide:
enslaved svirfneblin laborers support drow agriculture, livestock, timber
processing, and a decaying lower-class ghetto, beneath an older noble district
centered on Lolth worship and military history.

| Setting | Source evidence | Story cues |
| --- | --- | --- |
| Durithalite cavern, lake, and mushroom groves | [cavern entrance](../../../areas/wld/llzazan.wld#L1), [slave-camp gate](../../../areas/wld/llzazan.wld#L70), [mushroom-grove entrance](../../../areas/wld/llzazan.wld#L1520), [turbulent lake water](../../../areas/wld/llzazan.wld#L2716) | Pulsing durithalite provides the cavern’s dim red light. A large cold lake threads through giant mushroom groves and farming caverns. Thoqqua and stone guardians are reset around the upper cavern approach ([resets](../../../areas/zon/llzazan.zon#L95)). The WLD graph also records six current-configured rooms; that source setting does not establish current behavior or route safety. |
| Captive camp and food production | [slave-camp homes](../../../areas/wld/llzazan.wld#L99), [locked taskmaster house](../../../areas/wld/llzazan.wld#L377), [fungus farm and gallows road](../../../areas/wld/llzazan.wld#L746), [Rothe pasture](../../../areas/wld/llzazan.wld#L1999), [slave-clearing-house pen](../../../areas/wld/llzazan.wld#L1240) | Poor svirfneblin dwellings, crowded dormitories, a mess hall, kitchen, and communal latrines describe life under forced labor. Workers tend fungus and Flaming Fungus fields, while reset placements include a slave trader, executioner, shackled captives, taskmasters, and field laborers. Rothe are raised and slaughtered for meat and hide; the mushroom harvest and livestock products feed the ghetto’s commerce. |
| Working ghetto and commercial district | [skin-goods shop](../../../areas/wld/llzazan.wld#L3171), [laboratory](../../../areas/wld/llzazan.wld#L3207), [Barter Way](../../../areas/wld/llzazan.wld#L3249), [reading room](../../../areas/wld/llzazan.wld#L4484) | The lower settlement is described as rubble-strewn and impoverished. Its shops sell low-grade goods at high prices; one leather store is built around svirfneblin skin, and a nearby laboratory contains skinned humanoid bodies. A peaceable reading room and an inn for wealthier drow sit alongside those scenes, so the ghetto is not represented as a single-purpose work camp. |
| House Llzazan’s fall, temple, and armory | [battle-site tavern](../../../areas/wld/llzazan.wld#L3224), [upper noble cavern](../../../areas/wld/llzazan.wld#L3518), [Lyvrach Armory](../../../areas/wld/llzazan.wld#L3718), [Lolth temple](../../../areas/wld/llzazan.wld#L3543), [altar](../../../areas/wld/llzazan.wld#L3564), [blood fountain](../../../areas/wld/llzazan.wld#L3654), [sacrifice chamber](../../../areas/wld/llzazan.wld#L3810) | The source recounts a Drow-Dwarf battle over durithalite and the later defeat of House Llzazan by the combined Houses of Arachdrathos. The upper cavern, which holds the temple and armory, escaped the raid. The armory later came under Arachdrathos control as a satellite training facility. Talice remains the High Priestess of Lolth; the temple includes an altar, blood fountain, and room for preparing sacrifices. |
| Timber trade and personal coercion | [mushroom-timber treatment facility](../../../areas/wld/llzazan.wld#L4102), [Dinin’s office](../../../areas/wld/llzazan.wld#L4185), [personal prison](../../../areas/wld/llzazan.wld#L4629), [Dinin’s profile](../../../areas/mob/llzazan.mob#L29) | Dinin’s mobile description makes him a wealthy timber supplier who exploits local labor. His office links the production district to a concealed prison where the room text says he extracts information from surface visitors to support his trade. This is a coercion story in the environment; the source does not map a player rescue or investigation contract. |

## Residents, resets, and shops

The named mobiles imply local conflicts without defining their resolution.
Dinin owns the treatment facility; Talice serves as High Priestess; Wole has
taken the former High Priestess’s home; Qilue is a bounty hunter who knows the
caverns; Ulviirala is described as a fugitive with many hideouts; and Dzara
weaves cavefisher silk for drow clothing
([Dinin](../../../areas/mob/llzazan.mob#L29),
[Talice and Wole](../../../areas/mob/llzazan.mob#L44),
[Ulviirala and Qilue](../../../areas/mob/llzazan.mob#L74),
[Dzara](../../../areas/mob/llzazan.mob#L166)). Belgos’s gruesome embalming work,
the slave trader, and the executioner reinforce the area’s violence
([Belgos](../../../areas/mob/llzazan.mob#L15),
[slave trader and executioner](../../../areas/mob/llzazan.mob#L196)). These
descriptions are hooks for builder review, not evidence of unrecorded quests.

The zone file has 273 typed reset commands: 123 `M`, 66 `E`, 39 `G`, 21 `O`,
16 `D`, 6 `P`, and 2 `F`. It places the captive labor force and overseers in the
camp and fields, specialist rothe butchers in the processing facility, Dinin
in the timber works, and the named residents in their described rooms
([camp and captives](../../../areas/zon/llzazan.zon#L95),
[butchers and processors](../../../areas/zon/llzazan.zon#L284),
[Dinin and named residents](../../../areas/zon/llzazan.zon#L305)).

The `.shp` file contains four records, not zero: Alak at room 36338, Zzarak at
36335, Yzzir at 36368, and Azliz at 36326
([Alak](../../../areas/shp/llzazan.shp#L1),
[Zzarak](../../../areas/shp/llzazan.shp#L37),
[Yzzir](../../../areas/shp/llzazan.shp#L77),
[Azliz](../../../areas/shp/llzazan.shp#L113)). The trades cover imported
general goods, rothe products, and svirfneblin-skin clothing. The four records
and their shopkeepers are source inventory only; actual shop availability and
transactions were not tested.

## Doors and static connectivity

Eight paired door boundaries are declared in the reset comments: the two
secret cavern passages, the slave-camp gates, the taskmaster’s steel door, the
temple curtain, Dinin’s concealed prison, and two hidden ghetto rooms
([door reset declarations](../../../areas/zon/llzazan.zon#L8)). The most
important key linkage needs builder confirmation:

| Boundary | Source state | Open question |
| --- | --- | --- |
| 36203–36204, slave-camp gates | The WLD exits name key object 36242, while zone-reset comments mark the gates open. A reset gives a steel key 36241 to the gate-post golem ([WLD exit](../../../areas/wld/llzazan.wld#L89), [gate resets](../../../areas/zon/llzazan.zon#L16), [key placement](../../../areas/zon/llzazan.zon#L103)). | The gate key field and the reset key VNUM differ. Confirm whether the gates should be open at reset and whether object 36241 is meant for another lock. |
| 36214–36224, taskmaster quarters | Both WLD exits name key 36242; comments call the paired door closed and locked. The reset gives key 36242 to the taskmaster placed inside room 36224 ([outside exit](../../../areas/wld/llzazan.wld#L401), [inside exit](../../../areas/wld/llzazan.wld#L583), [door reset](../../../areas/zon/llzazan.zon#L24), [taskmaster and key](../../../areas/zon/llzazan.zon#L149)). | The matching key is reset with the mobile behind the locked boundary. Confirm the intended way to obtain or use it. |
| 36201–36243 and 36246–36297 | The reset comments call both passage pairs closed and secret ([first pair](../../../areas/zon/llzazan.zon#L8), [second pair](../../../areas/zon/llzazan.zon#L32)). | Confirm whether either passage is intended to serve as a usable route between the cavern, lake, and fungus fields. |
| 36351–36356, 36374–36396, 36375–36378, 36377–36380 | These are a closed temple curtain, the concealed office-to-prison boundary, and secret doors to a training room and a junk room ([remaining resets](../../../areas/zon/llzazan.zon#L40)). | Static flags and room descriptions do not qualify how players discover or traverse these rooms. |

The census records 503 source exits, all loader-resolved, and all 197 rooms in
one local weak component. It records six current-configured rooms and no
fall-chance rooms ([coverage row](ZONE_COVERAGE_REGISTER.csv#L162)). There are
five outgoing cross-area exits, all with reverse-direction records: one to
`arac_wild`, three to `arac-web`, and one to `underdark`
([an exact exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L477),
[Arac-web pair](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L252),
[Arac-wild pair](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L253),
[Underdark pair](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L254)). The census also
records 42 inbound exits: 36 from six `wander` rooms, one from `surface`, and
the paired routes from `arac-web`, `arac_wild`, and `underdark`
([Wander routes](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L774),
[Surface route](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1685),
[Arac-web](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L18),
[Arac-wild](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L21),
[Underdark](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L715)). The 36 `wander`
edges and the Surface edge have no reverse WLD exit record. These are static
links; their ordinary-player availability, return behavior, and safety remain
unverified.

## Typed dependencies and history boundary

Llzazan’s own reset and shop sources import mobile/object prototypes from
active Heavens sources: mobile 101, objects 90, 102, 203, 204, 256, and
829–838 ([reset references](../../../areas/zon/llzazan.zon#L81),
[additional item resets](../../../areas/zon/llzazan.zon#L193),
[Alak’s stock](../../../areas/zon/llzazan.zon#L217),
[drug dealer and stock](../../../areas/zon/llzazan.zon#L320)). Objects 90 and 102 also appear in the unregistered `mavulsk` object
file, which is outside the full area list; they are not unique numeric leads
without owner context ([Llzazan imports](../../../areas/zon/llzazan.zon#L81),
[Heavens object 90](../../../areas/obj/heavens.obj#L1171),
[Heavens object 102](../../../areas/obj/heavens.obj#L1522),
[Mavulsk inventory](ZONE_COVERAGE_REGISTER.csv#L421)). The Alak shop also
stocks Heavens objects 203, 204, and 256. Preserve these cross-area references
until their source ownership and intended stock are confirmed.

Active `wander` resets reference Llzazan rothe objects 36204 and 36205, and
active `rtun` reset/shop files reference Llzazan music objects 36245–36248
([Wander rothe meat reset](../../../areas/zon/wander.zon#L29),
[Wander rothe product reset](../../../areas/zon/wander.zon#L39),
[Rtun resets](../../../areas/zon/rtun.zon#L107),
[Rtun shop stock](../../../areas/shp/rtun.shp#L3)). The unregistered ud_map0.zon has two M reset records for Llzazan mobile
36225 in rooms #1104813 and #1105038
([first reset](../../../areas/zon/ud_map0.zon#L25),
[second reset](../../../areas/zon/ud_map0.zon#L26)). ud_map0 is outside
areas/AREA; preserve both as source-only typed dependencies while its owner
and tool/editor closure remain unresolved
([component row](ZONE_COVERAGE_REGISTER.csv#L463),
[existing candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L112)). A text search found no
external QST references to Llzazan mobile or object VNUMs.

One reset object remains a notable history clue. Object 36252 is a placeholder
whose name and description say it “used to lead to mazzolin, room 21300,” and
the reset still places it in room 36269 ([object](../../../areas/obj/llzazan.obj#L709),
[reset](../../../areas/zon/llzazan.zon#L73)). The current cross-area exit
census has no Llzazan-to-Mazzolin route. Treat the text as an unresolved old
route reference, not proof that either area replaced the other.

The normalized active-title scan reports no repeated Llzazan zone title, and
the duplicate/replacement register identifies no Llzazan retirement or
replacement. Keep this contributor and its files in place; the current
evidence supports no attic move ([replacement review](DUPLICATE_AND_REPLACED_ZONES.md),
[active coverage row](ZONE_COVERAGE_REGISTER.csv#L162)).

## Directed WLD reachability

The raw directed graph has 197 rooms and 503 resolving exits: 498 local exits
and five outbound exits. All rooms are in one weak component and one directed
strongly connected component. From #36200, all 197 rooms are reachable and all
197 have an authored local return path. These are source-graph results; they
do not simulate reset door states, discovery of secret exits, current effects,
mobile movement, or safe travel.

There are 42 active inbound exits, reaching 40 distinct Llzazan rooms. Wander
contributes 36 exits from six source rooms (#36601–#36606); Surface contributes
one one-way arrival to #36287; the other five arrivals are reciprocal pairs:
three with Arac-Web, one with Arac-Wild, and one with Underdark. The five
Llzazan outbound exits are #36200 south to #36199, #36242 north to #36546,
#36262 south to #805516, #36285 west to #36573, and #36363 west to #36582.
Every inbound target reaches the full local graph, and every local room can
return to an active arrival
([exit rows 476–481 and 489–528](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L476),
[Surface arrival](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1685),
[Underdark arrival](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1722)).
The six source-configured current rooms and all reset-controlled doors remain
unqualified for player safety or present availability.

## Compiled source-code bindings

| Source identity | Compiled binding | Source meaning |
| --- | --- | --- |
| Talice, mobile #36203 | specs.assign.c assigns rentacleric; the handler purchase table includes healing and restorative services, antidote and cure services, and resurrection. | Confirms a compiled service binding. Current purchase availability and quest behavior remain unqualified. See [assignment](../../../src/specs/specs.assign.c#L419) and [handler](../../../src/specs/specs.mobile.c#L1805). |
| Inn room #36334 | specs.assign.c assigns the inn room handler; inn accepts the player rent command subject to its runtime checks. | Confirms a compiled rent-service binding. See [assignment](../../../src/specs/specs.assign.c#L2510) and [handler](../../../src/specs/specs.room.c#L243). |
| Tavern room #36329 | constant.c lists #36329 for Arachdrathos and assigns it to every class in the Arachdrathos guild_locations row. find_starting_location reads that row and writes the selected VNUM to the character home. | Confirms the configured Arachdrathos class starting and home location. See [hometown table](../../../src/core/constant.c#L1719), [class locations](../../../src/core/constant.c#L1897), and [assignment use](../../../src/account/nanny.c#L4234). |

These bindings close the static compiled-reference questions for this pass.
They do not qualify service use, character creation, renting, or live access.

## Builder questions and recommended resolutions

| # | Builder question | Source cue | Recommended resolution |
| --- | --- | --- | --- |
| 1 | Is the sidecar coverage-complete state intended when its contact, story, and exclusion arrays are empty and the introduction invites players to uncover requests? | [sidecar](../../../areas/story/llzazan.story.json#L5) | Confirm environmental-only scope or add authored story entries; make the introduction and coverage claim agree. Do not infer a quest from resident names. |
| 2 | Are the forced-labor, dormitory, trader, and execution scenes intended at their current level of detail? | [camp rooms](../../../areas/wld/llzazan.wld#L99), [reset placements](../../../areas/zon/llzazan.zon#L95) | Keep them classified as environmental narrative and ask the area owner to confirm tone before changing or expanding them. |
| 3 | Are the body-processing, svirfneblin-skin trade, and sacrifice scenes intended to appear together in the public area? | [skin-goods shop](../../../areas/wld/llzazan.wld#L3171), [sacrifice chamber](../../../areas/wld/llzazan.wld#L3810) | Record the owner content decision; do not turn these scenes into a presumed player objective. |
| 4 | Is the Drow-Dwarf battle and House Llzazan defeat described with the intended relationship to the spared upper district? | [battle-site tavern](../../../areas/wld/llzazan.wld#L3224), [upper district](../../../areas/wld/llzazan.wld#L3518) | Preserve the authored history and avoid adding dates or causes unsupported by the source. |
| 5 | Is Dinin’s concealed prison meant to support player interaction, or is it only setting for his coercive timber trade? | [Dinin office](../../../areas/wld/llzazan.wld#L4185), [prison](../../../areas/wld/llzazan.wld#L4629) | Treat it as environmental story until an interaction or contract is specified; do not infer a rescue quest. |
| 6 | Should the slave-camp gates be open at reset while their WLD key field names object #36242 and the guard-post reset gives key #36241? | [gate exits](../../../areas/wld/llzazan.wld#L89), [door reset](../../../areas/zon/llzazan.zon#L16), [key reset](../../../areas/zon/llzazan.zon#L103) | Confirm the intended initial state and key identity together; align comments, key field, and reset item deliberately. |
| 7 | How is key #36242, reset on the taskmaster inside, meant to be obtained when both directions of that house door are locked? | [outside exit](../../../areas/wld/llzazan.wld#L401), [inside exit](../../../areas/wld/llzazan.wld#L583), [taskmaster reset](../../../areas/zon/llzazan.zon#L149) | Confirm intended acquisition or placement before changing the door or key. |
| 8 | Should the closed, secret underwater pair between #36201 and #36243 be discoverable as a route between the cavern and lake? | [door comments](../../../areas/zon/llzazan.zon#L8) | Confirm discovery and return expectations; retain it as a reset-controlled secret until qualified. |
| 9 | Should the second closed, secret underwater pair between #36246 and #36297 connect the Flaming Fungus fields to the underwater passage? | [door comments](../../../areas/zon/llzazan.zon#L32) | Confirm how both sides are found and whether ordinary players can return; keep it out of the open-route count. |
| 10 | Is the closed temple boundary between #36351 and #36356 meant to limit access to the sacrifice-preparation chamber? | [temple door resets](../../../areas/zon/llzazan.zon#L40) | Keep the current closed reset state in the source record and confirm intended access and story framing with the owner. |
| 11 | What discovery and return path is intended for Dinin’s office and prison, the hidden dwelling, training room, and junk room? | [concealed door resets](../../../areas/zon/llzazan.zon#L48), [secret exit](../../../areas/wld/llzazan.wld#L4201) | Confirm each room role and return path; keep the boundaries separate rather than assuming one hidden-room network. |
| 12 | What danger and intended direction do the six current-configured rooms represent along the lake routes? | [current-configured rooms](../../../areas/wld/llzazan.wld#L2716), [coverage row](ZONE_COVERAGE_REGISTER.csv#L162) | Treat current fields as hazards to qualify, not evidence of safe entry or return. |
| 13 | Are the 36 one-way Wander exits from rooms #36601–#36606 intended player routes, mobile paths, or both? | [Wander inbound rows](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L493) | Preserve the active links and verify player access separately from reset or mobile behavior. |
| 14 | Is the one-way Surface arrival from #660003 into #36287 meant to be reachable and usable by ordinary players? | [Surface exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1685) | Preserve the incoming record and verify its player approach and return before describing it as a public route. |
| 15 | Are all five reciprocal active boundaries with Arac-Web, Arac-Wild, and Underworld intended to remain, and is the separate source-only ud_map2 east exit into Llzazan intended? | [Llzazan boundary rows](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L477) · [source-only approach](../../../areas/wld/ud_map2.wld#L36648) · [candidate room](../../../areas/wld/llzazan.wld#L1) · [ud_map2 audit](UD_MAP2_AUDIT_REVIEW.md#authored-wld-graph-and-boundaries) | Preserve the five active pairs under their current area owners and record the candidate approach separately; it is not active ingress. Qualify door state and player access independently. |
| 16 | Are the Heavens mobile #101 and objects #90, #102, #203, #204, #256, and #829–#838 intended imports, given that Mavulsk also has object VNUMs #90 and #102? | [Llzazan resets and shops](../../../areas/zon/llzazan.zon#L81), [Heavens object #90](../../../areas/obj/heavens.obj#L1171), [Mavulsk inventory](ZONE_COVERAGE_REGISTER.csv#L421) | Preserve current Heavens references and ask both owners to resolve the shared object identity; keep the matching number owner-ambiguous. |
| 17 | Are the four shop records and their rothe, general-goods, and skin-clothing stock intentional? | [four shop records](../../../areas/shp/llzazan.shp#L1) | Keep all four records in the inventory; have the shop owners confirm stock and live availability before edits. |
| 18 | Should Wander reset consumers of objects #36204/#36205 and Rtun reset/shop consumers of #36245–#36248 remain attached to Llzazan, and what is intended by the two ud_map0 reset entries for mobile #36225? | [Wander resets](../../../areas/zon/wander.zon#L29), [Rtun references](../../../areas/zon/rtun.zon#L107), [ud_map0 reset at #1104813](../../../areas/zon/ud_map0.zon#L25), [ud_map0 reset at #1105038](../../../areas/zon/ud_map0.zon#L26) | Preserve active typed dependencies under their current owners; keep both ud_map0 reset records on existing source-only candidate row 112 pending its owner’s review. |
| 19 | What current source role, if any, should replace object #36252’s old “used to lead to mazzolin, room 21300” wording when no active Llzazan–Mazzolin WLD pair remains? | [object](../../../areas/obj/llzazan.obj#L709), [Mazzolin source exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L357) | Keep the wording classified as a historical route marker until the builder confirms a current role; update object text and exits together. |
| 20 | Are the compiled rent-a-cleric binding on Talice #36203, inn handler at #36334, and Arachdrathos start/home room #36329 intended to remain? | [compiled bindings above](#compiled-source-code-bindings) | Keep the handlers and home-table references under their current code owner; confirm expected services and start location. |

## Safe resolution summary

Keep Llzazan active and in its present source location. Record the captive
labor, fungus and Rothe production, impoverished ghetto, fallen House Llzazan,
Lolth temple, and Dinin’s timber trade as environmental narrative, with no
local quest objective currently mapped. Preserve the four shop records,
Heavens/Wander/Rtun dependencies, source-only ud_map0 reference, and the
compiled service and Arachdrathos start-location bindings. The 20 questions
capture builder decisions still outstanding; no gameplay change or attic move
is proposed.

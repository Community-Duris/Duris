# Ghore: source story review

**Scope:** static review of Ghore's room and mobile stories, sidecar/QST
boundary, shops, resets, coded routes, local connectivity, and typed cross-area
references. No gameplay, server, or database behavior was qualified.

## Source identity and story coverage

Ghore is an active full-area contributor in the
[area list](../../../areas/AREA#L67), under zone header `*115-119`. Its
current sources define 416 rooms, 107 mobiles, 107 objects, 19 shops, and zero
native Q/QA contracts. The historical [`areas/qst/AREA`](../../../areas/qst/AREA#L61)
index includes `ghore`,
but there is no `areas/qst/ghore.qst`. The schema-two
[`ghore.story.json`](../../../areas/story/ghore.story.json) says `coverage:
complete` while `contacts`, `stories`, and `exclusions` are all empty; its
introduction still promises “local requests.” Treat the current map as a blank
classification boundary, not evidence of authored local quest chains.

Room and mobile descriptions establish these environmental stories:

- A ruined minotaur city and palace have been swallowed by a bog. Mud, rubble,
  collapsed buildings, marsh water, insects, cave-ins, and ruined minotaur
  statues recur through the streets and palace ([opening road](../../../areas/wld/ghore.wld#L1),
  [palace district](../../../areas/wld/ghore.wld#L3194),
  [minotaur statue](../../../areas/wld/ghore.wld#L3490)). Troll settlement
  adapts to swamp and forest through tree homes, bog paths, tree-top routes,
  and dry ground ([tree homes](../../../areas/wld/ghore.wld#L4738),
  [bog](../../../areas/wld/ghore.wld#L6536),
  [tree tops](../../../areas/wld/ghore.wld#L7523),
  [drylands](../../../areas/wld/ghore.wld#L5996)).
- Troll occupation is layered over a former minotaur royal seat. Current
  mobile #11528, Chief Kerzt Bloodeye, claims the Bloodeye line displaced a
  prior chieftain centuries ago and rules the city ([mobile](../../../areas/mob/ghore.mob#L304)).
  Yet room #11685 is “Chief MunChelfs Royal Chamber,” and room #11686 calls
  its pit MunChelf's ([chamber](../../../areas/wld/ghore.wld#L3416),
  [pit](../../../areas/wld/ghore.wld#L3439)); reset commands place Kerzt and
  Bloodeye's guard in that chamber ([resets](../../../areas/zon/ghore.zon#L773)).
  Another room is explicitly Bloodeye's bedroom
  ([room](../../../areas/wld/ghore.wld#L3452)). Builder clarification is
  needed on whether the chamber is intentionally a reused predecessor's room
  or has stale chief names.
- The commercial “Paradise” district mixes tourist framing with depictions of
  captivity, torture, and cannibalism ([entrance](../../../areas/wld/ghore.wld#L2982),
  [district](../../../areas/wld/ghore.wld#L3008)). Rorie's Cat House has
  sexualized/prostitution themes and a separate mating room
  ([Cat House](../../../areas/wld/ghore.wld#L6231),
  [mating room](../../../areas/wld/ghore.wld#L6250)). Ask the author to review
  framing, consent, and age implications before promoting these as player-facing
  story cards.
- Civic and military life includes four troll guilds, guild entrances and
  halls, guards, combat practice, a bank, shops, an inn, stables, taverns, and a
  black market. Distinctive trades include barrel-making and wine/brewing; the
  settlement also has stone weapon and armor craftspeople
  ([barrel-maker](../../../areas/wld/ghore.wld#L2792),
  [wine work](../../../areas/wld/ghore.wld#L2809),
  [stonework](../../../areas/wld/ghore.wld#L8697)).
- Everyday scenes include a sick troll seeking help, children and students,
  rough lessons and discipline, alligators, and a swamp creature. These are
  environmental vignettes unless a builder maps a real interaction and outcome
  ([school](../../../areas/wld/ghore.wld#L8710),
  [students](../../../areas/wld/ghore.wld#L8808),
  [Swamp Thing mobile](../../../areas/mob/ghore.mob#L532)).

### Text and prototype findings for the builder

- Mobile #11531's long description contains the literal `<<<<<<< .mine`
  before Akol's text ([mobile source](../../../areas/mob/ghore.mob#L355)). No
  matching separator or closing conflict marker appears in that description.
  Confirm whether this player-visible artifact is accidental; no source repair
  was made.
- Two Ghore reset commands place mobiles #11625 and #11626 in room #11863,
  but neither mobile VNUM has a definition in the current `areas/mob/*.mob`
  sources ([resets](../../../areas/zon/ghore.zon#L939)). These are missing
  **mobile** prototypes; object VNUM collisions do not satisfy them.
- Rooms #11744, #11936, and #11938 form three singleton components in the
  authored local exit graph. The first is described as a vegetation-sealed
  trap ([room](../../../areas/wld/ghore.wld#L4676)); the latter two describe
  named pet procedures ([Dunkas](../../../areas/wld/ghore.wld#L8669),
  [Sneers](../../../areas/wld/ghore.wld#L8691)). No direct special assignment
  to those room VNUMs was found in the reviewed C/C++ source search. Confirm
  whether editor-side or other indirect owners exist before interpreting them
  as dead rooms.
- Room #11918 has a west exit to #11919 with a nonzero door flag, but no
  matching `.zon` D reset was found for that exit ([WLD](../../../areas/wld/ghore.wld#L8611),
  [destination](../../../areas/wld/ghore.wld#L8629)). The area has 84 door
  resets for 85 flagged exits; review this cave-in connection's intended
  initial state.

## Shops and placements

The source shop file contains 19 records. Each keeper VNUM matches its `.zon`
M reset room; the table records every static placement. Shop records are in
[`ghore.shp`](../../../areas/shp/ghore.shp), and matching resets are in
[`ghore.zon`](../../../areas/zon/ghore.zon).

| Keeper | Reset room | SHP | ZON |
| ---: | ---: | ---: | ---: |
| 11524 | 11527 | [L1](../../../areas/shp/ghore.shp#L1) | [L481](../../../areas/zon/ghore.zon#L481) |
| 11525 | 11652 | [L41](../../../areas/shp/ghore.shp#L41) | [L685](../../../areas/zon/ghore.zon#L685) |
| 11526 | 11904 | [L78](../../../areas/shp/ghore.shp#L78) | [L986](../../../areas/zon/ghore.zon#L986) |
| 11529 | 11654 | [L116](../../../areas/shp/ghore.shp#L116) | [L692](../../../areas/zon/ghore.zon#L692) |
| 11530 | 11651 | [L153](../../../areas/shp/ghore.shp#L153) | [L675](../../../areas/zon/ghore.zon#L675) |
| 11535 | 11606 | [L193](../../../areas/shp/ghore.shp#L193) | [L553](../../../areas/zon/ghore.zon#L553) |
| 11536 | 11646 | [L233](../../../areas/shp/ghore.shp#L233) | [L663](../../../areas/zon/ghore.zon#L663) |
| 11538 | 11618 | [L269](../../../areas/shp/ghore.shp#L269) | [L576](../../../areas/zon/ghore.zon#L576) |
| 11540 | 11655 | [L305](../../../areas/shp/ghore.shp#L305) | [L700](../../../areas/zon/ghore.zon#L700) |
| 11541 | 11657 | [L342](../../../areas/shp/ghore.shp#L342) | [L706](../../../areas/zon/ghore.zon#L706) |
| 11543 | 11667 | [L379](../../../areas/shp/ghore.shp#L379) | [L716](../../../areas/zon/ghore.zon#L716) |
| 11549 | 11627 | [L417](../../../areas/shp/ghore.shp#L417) | [L612](../../../areas/zon/ghore.zon#L612) |
| 11601 | 11832 | [L454](../../../areas/shp/ghore.shp#L454) | [L915](../../../areas/zon/ghore.zon#L915) |
| 11602 | 11609 | [L491](../../../areas/shp/ghore.shp#L491) | [L566](../../../areas/zon/ghore.zon#L566) |
| 11603 | 11634 | [L528](../../../areas/shp/ghore.shp#L528) | [L623](../../../areas/zon/ghore.zon#L623) |
| 11605 | 11625 | [L565](../../../areas/shp/ghore.shp#L565) | [L599](../../../areas/zon/ghore.zon#L599) |
| 11606 | 11639 | [L602](../../../areas/shp/ghore.shp#L602) | [L635](../../../areas/zon/ghore.zon#L635) |
| 11608 | 11621 | [L651](../../../areas/shp/ghore.shp#L651) | [L585](../../../areas/zon/ghore.zon#L585) |
| 11609 | 11668 | [L692](../../../areas/shp/ghore.shp#L692) | [L723](../../../areas/zon/ghore.zon#L723) |

Six shops list active Heavens object prototypes as sale stock: keeper #11524
lists #191, #363, #368, and #369; #11535 lists #613, #634, #655, and #705–707;
#11538 lists #101; #11540 lists #385–386; #11606 lists #105–109, #111, #203–204,
#613, #634, #655, and #704–707; and #11608 lists #53, #700, #707, and #708
with #613, #634, and #655. See the exact stock records ([11524](../../../areas/shp/ghore.shp#L1),
[11535](../../../areas/shp/ghore.shp#L193), [11538](../../../areas/shp/ghore.shp#L269),
[11540](../../../areas/shp/ghore.shp#L305), [11606](../../../areas/shp/ghore.shp#L602),
[11608](../../../areas/shp/ghore.shp#L651)) and the active Heavens prototypes
([area list](../../../areas/AREA#L2), [objects](../../../areas/obj/heavens.obj#L788)).
Ghore's resets place 122 Heavens-object instances across 31 distinct VNUMs:
`53, 90, 92, 101, 105–109, 111, 191, 203–204, 361, 363–364, 368–369, 385–386,
613, 634, 655, 700, 704–708, 821–822`. The largest groups are #821 (77 resets)
and #822 (4). The same numeric prototypes #53, #101, and #105–109 also occur
in unregistered source copies; current full `AREA` ownership for those IDs is
Heavens, not those unlisted copies. Ghore additionally resets active Verz1
object #3097 ([reset](../../../areas/zon/ghore.zon#L346),
[prototype](../../../areas/obj/verz1.obj#L1199)).

## Typed cross-area dependencies

### Active sources that consume or stock Ghore prototypes

- **Active shops:** Khildarak's shop stocks Ghore objects #11606 and #11607
  ([shop](../../../areas/shp/khildarak.shp#L1008)); Surfacekeeps stocks Ghore
  object #11562 ([shop](../../../areas/shp/surfacekeeps.shp#L282)).
- **Active resets:** Alatorin places object #11562 once, #11618 eight times,
  and #11619 ten times ([#11562](../../../areas/zon/alatorin.zon#L1767),
  [#11618](../../../areas/zon/alatorin.zon#L3926),
  [#11619](../../../areas/zon/alatorin.zon#L4009)); Khildarak resets #11606
  and #11607 ([#11606](../../../areas/zon/khildarak.zon#L3164),
  [#11607](../../../areas/zon/khildarak.zon#L3165)); Surfacekeeps resets
  #11562 and equips Ghore #11619 in several placements
  ([#11562](../../../areas/zon/surfacekeeps.zon#L165),
  [#11619](../../../areas/zon/surfacekeeps.zon#L184)).
- **Active QST offers:** Icecrag asks for Ghore objects #11606 and #11607 and
  separately accepts #11519 ([#11606–11607](../../../areas/qst/icecrag.qst#L66),
  [#11519](../../../areas/qst/icecrag.qst#L155)); Shipy
  accepts Ghore #11562 ([QST](../../../areas/qst/shipy.qst#L892)). These are
  foreign contract references, not Ghore-owned Q/QA records.
- **Active mobile reset:** `mobs_underdark.zon` has four M reset commands for
  Ghore mobile #11631 in room #24001 ([resets](../../../areas/zon/mobs_underdark.zon#L20));
  the mobile prototype is in Ghore ([mobile](../../../areas/mob/ghore.mob#L1241)).

### Unregistered Nexus56 references

The current unregistered `nexus56` source component has typed references to
three Ghore object prototypes: ZON object resets #11553, #11589, and #11552
([#11553](../../../areas/zon/nexus56.zon#L1330),
[#11589](../../../areas/zon/nexus56.zon#L1347),
[#11552](../../../areas/zon/nexus56.zon#L1384)); SHP records also list those
same objects ([stock #11553](../../../areas/shp/nexus56.shp#L255),
[#11589](../../../areas/shp/nexus56.shp#L343), [#11552](../../../areas/shp/nexus56.shp#L544)).
All three object definitions belong to Ghore ([#11552–11553](../../../areas/obj/ghore.obj#L383),
[#11589](../../../areas/obj/ghore.obj#L699)). Nexus56 remains unregistered and
is not cleared for movement; its candidate ledger and migration plan record
these references explicitly.

## Static connectivity and code-defined routes

All 987 authored Ghore exits resolve under the active WLD loader rules. The
local weak graph has four components: one of 413 rooms and three singleton
rooms (#11744, #11936, #11938). Ghore has three paired active WLD boundaries:

| Ghore boundary | Paired active room | WLD evidence |
| --- | --- | --- |
| #11529 south to Torture | #95400 north | [Ghore](../../../areas/wld/ghore.wld#L664) · [cross-exit ledger](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L210) |
| #11540 south to Troll Hills | #1909 north | [Ghore](../../../areas/wld/ghore.wld#L898) · [cross-exit ledger](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L211) |
| #11548 down to Shafts | #10043 up | [Ghore](../../../areas/wld/ghore.wld#L1061) · [cross-exit ledger](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L212) |

An inactive-map source adds a separate approach: [ud_map0 #1104885 exits up to
Ghore #11548](../../../areas/wld/ud_map0.wld#L29619). Because ud_map0 is not in
areas/AREA, this is a source-only approach, not active ingress. Ghore #11548’s
existing down exit to Shafts #10043 remains a separate active boundary and does
not make the ud_map0 approach live or establish a return to that candidate.

The foyer-trap room #11684 has an `F 20` field and a down exit to pit #11686
([room](../../../areas/wld/ghore.wld#L3391),
[loader field handling](../../../src/world/db.c#L1395)). This records a source
hazard marker; the room is not a promise of a safe or repeatable path.

Static code adds routes and admission checks outside the WLD graph:

- Ixarkon's active object #96402 is placed at room #96524 and assigned
  `illithid_teleport_veil`; its handler's random destination array includes
  Ghore room #11545 ([reset](../../../areas/zon/ixarkon.zon#L156),
  [assignment](../../../src/specs/specs.assign.c#L1874),
  [handler](../../../src/specs/specs.ixarkon.c#L18)). This is a source-defined
  possible arrival, not a static WLD edge or a tested route.
- Ghore guard #11542 has `ghore_paradise` assigned. In room #11666 it handles
  the up passage and a GIVE payment path that can place the player in #11667
  ([assignment](../../../src/specs/specs.assign.c#L944),
  [handler](../../../src/specs/specs.ghore.c#L31)). This route is code-gated;
  no live entry or payment journey was run.
- The shared `guild_guard` procedure contains class-gate cases for rooms
  #11603 west (Warrior), #11633 west (Shaman), and #11619 south (Mercenary),
  plus blocks east from #11685 and up from #11812
  ([special](../../../src/specs/specs.guards.c#L121),
  [additional cases](../../../src/specs/specs.guards.c#L240)). However, the
  current special-assignment map binds no Ghore mobile to `guild_guard`
  ([assignments](../../../src/specs/specs.assign.c#L944)). Ghore does reset
  Chief Kerzt #11528 and his personal guard #11552 into room #11685
  ([resets](../../../areas/zon/ghore.zon#L773)), but #11552 has no such special
  assignment. No M reset places a mobile in the other case rooms #11603,
  #11619, #11633, or #11812. The `.zon` D resets close the three corresponding
  guild doors ([reset](../../../areas/zon/ghore.zon#L32)), but the code cases do
  not establish active class enforcement. Builder review should decide whether
  the unbound branches are stale or intentionally reserved.

## Direct area-owner questions and conservative recommendations

### Builder questions

| Question | Source evidence | Conservative recommendation |
| --- | --- | --- |
| **GHORE-Q01.** Will Ghore retain its current active identity and title, with no inferred alias, predecessor, or replacement? | The review found no repeated active title and followed WLD history does not identify a predecessor or replacement. See [source identity](#source-identity-and-story-coverage) and [disposition/history](#disposition-and-open-decisions). | Preserve the current `ghore` contributor and title; do not merge, rename, or move the area from bounded title/history evidence alone. |
| **GHORE-Q02.** Does `ghore.story.json` intentionally mark an empty local-story classification as complete while its introduction promises "local requests"? | The sidecar has empty contacts, stories, and exclusions, and the active sources contain no local QST file or native Q/QA contract. See [story coverage](#source-identity-and-story-coverage), [sidecar](../../../areas/story/ghore.story.json), and [QST index](../../../areas/qst/AREA#L61). | Clarify whether the sidecar is a classification boundary or an authored quest inventory; do not describe local requests or progression as implemented until the contradiction is resolved. |
| **GHORE-Q03.** Is Chief Kerzt's placement in the room and pit named for MunChelf an intentional reuse, or should those older chief names be clarified? | Kerzt's mobile claims a Bloodeye succession; the chamber and pit retain MunChelf names, while resets place Kerzt and his guard in that chamber. See [Kerzt](../../../areas/mob/ghore.mob#L304), [chamber](../../../areas/wld/ghore.wld#L3416), [pit](../../../areas/wld/ghore.wld#L3439), and [resets](../../../areas/zon/ghore.zon#L773). | Preserve the present text and placement until the narrative owner confirms the relationship; do not invent succession details or silently rename the old chief. |
| **GHORE-Q04.** Is the literal `<<<<<<< .mine` text in mobile #11531 accidental player-visible conflict residue? | The marker appears in the mobile's long description, with no matching separator or closing marker in that description. See [prototype finding](#text-and-prototype-findings-for-the-builder) and [mobile source](../../../areas/mob/ghore.mob#L355). | Ask the content owner to confirm the intended description; do not publish a speculative replacement or alter native content as part of this review. |
| **GHORE-Q05.** Are the two resets for missing mobile prototypes #11625 and #11626 in room #11863 intended to reference these exact mobile IDs? | Both M resets lack matching Ghore mobile definitions; both numbers instead resolve to local WLD rooms, which do not satisfy a mobile reset operand. See [prototype finding](#text-and-prototype-findings-for-the-builder), [reset records](../../../areas/zon/ghore.zon#L939), [room #11625](../../../areas/wld/ghore.wld#L2248), and [room #11626](../../../areas/wld/ghore.wld#L2263). | Resolve each reset against its owning mobile record; do not treat either room record as the missing mobile or silently remove either reset. |
| **GHORE-Q06.** Are rooms #11744, #11936, and #11938 intentionally isolated in the authored WLD graph, and what editor-side or indirect controller evidence should qualify their access? | The rooms are three singleton components; their text describes a vegetation-sealed trap and two named pet procedures, with no direct special assignment found in the bounded source search. See [prototype finding](#text-and-prototype-findings-for-the-builder), [trap room](../../../areas/wld/ghore.wld#L4676), [Dunkas room](../../../areas/wld/ghore.wld#L8669), and [Sneers room](../../../areas/wld/ghore.wld#L8691). | Retain the source graph finding without claiming reachability or dead content; inspect editor and other indirect owners before any topology repair. |
| **GHORE-Q07.** What is the intended initial state of the flagged west exit from #11918, which has no matching D reset? | Ghore has 84 door resets for 85 flagged exits; #11918 west to #11919 is the identified unreset pair. See [prototype finding](#text-and-prototype-findings-for-the-builder), [source exit](../../../areas/wld/ghore.wld#L8611), and [destination](../../../areas/wld/ghore.wld#L8629). | Confirm the loader/default and intended cave-in state before adding or changing a reset; do not infer that the exit must be open or closed. |
| **GHORE-Q08.** Will the content owner review Paradise's captivity, torture, and cannibalism framing before these rooms are promoted as player-facing story cards? | The Paradise entrance and district describe captivity, torture, and cannibalism. See [environmental story](#source-identity-and-story-coverage), [entrance](../../../areas/wld/ghore.wld#L2982), and [district](../../../areas/wld/ghore.wld#L3008). | Keep the review bounded to existing material; do not amplify violence or market it as a light tourist attraction without an explicit content decision. |
| **GHORE-Q09.** Will Rorie's Cat House and mating-room material receive a content review for consent and age implications before wider presentation? | The source contains sexualized/prostitution framing and a separate mating room. See [Cat House](../../../areas/wld/ghore.wld#L6231) and [mating room](../../../areas/wld/ghore.wld#L6250). | Seek the content owner's decision on framing and audience; do not infer consent or age appropriateness from the room text. |
| **GHORE-Q10.** Should the sick troll, school, student, and rough-discipline vignettes remain environmental scenes until a real interaction and outcome are specified? | These descriptions do not themselves define a quest or durable result. See [everyday scenes](#source-identity-and-story-coverage), [school](../../../areas/wld/ghore.wld#L8710), [students](../../../areas/wld/ghore.wld#L8808), and [Swamp Thing](../../../areas/mob/ghore.mob#L532). | Treat the vignettes as authored scenery until an owner defines an interaction, eligible actors, and a saved outcome; do not grant completion credit for visiting. |
| **GHORE-Q11.** Are the six Ghore shopkeepers' stocks of active Heavens objects intentional, with Heavens remaining the authoritative owner of those imported prototypes? | Six SHP records list Heavens-owned stock; the review records the exact keepers and prototypes. See [shop inventory](#shops-and-placements), [active area list](../../../areas/AREA#L2), and [Heavens objects](../../../areas/obj/heavens.obj#L788). | Preserve the active Heavens IDs and current consumer records; do not assign duplicate unregistered definitions as owners by number alone or promise live stock without separate qualification. |
| **GHORE-Q12.** Are Ghore's 122 reset instances across 31 Heavens object VNUMs, plus active Verz1 object #3097, intentional cross-area placements? | The reset scan records these imports and their source owners. See [reset placements](#shops-and-placements), [Ghore reset](../../../areas/zon/ghore.zon#L346), and [Verz1 prototype](../../../areas/obj/verz1.obj#L1199). | Keep the exact typed imports until their owners decide otherwise; do not collapse them into Ghore-owned stock or remove them based on numeric overlap. |
| **GHORE-Q13.** Should active Alatorin, Khildarak, Surfacekeeps, Icecrag, and Shipy consumers continue to use the exact Ghore object prototypes recorded here? | Active resets, shops, and QST offers reference Ghore objects #11562, #11606, #11607, #11618, and #11619. See [active consumers](#active-sources-that-consume-or-stock-ghore-prototypes). | Preserve each exact cross-area contract and its owning source; do not delete a local object without reviewing its external reset, shop, and QST consumers. |
| **GHORE-Q14.** Will the three unregistered Nexus56 references to Ghore objects #11552, #11553, and #11589 remain separately recorded while Nexus56 is unresolved? | Current Nexus56 ZON and SHP sources reference those Ghore objects. See [Nexus56 references](#unregistered-nexus56-references), [resets](../../../areas/zon/nexus56.zon#L1330), and [stock](../../../areas/shp/nexus56.shp#L255). | Keep the consumer source-only and attached to its candidate review; do not activate or move Nexus56 or its referenced items on this evidence. |
| **GHORE-Q15.** Are the four `mobs_underdark.zon` M commands for Ghore mobile #11631 intentional, separate from Ghore's own placements? | The reviewed source contains four M references from room #24001 to a Ghore mobile prototype. See [active consumer finding](#active-sources-that-consume-or-stock-ghore-prototypes), [external resets](../../../areas/zon/mobs_underdark.zon#L20), and [Ghore mobile](../../../areas/mob/ghore.mob#L1241). | Preserve and qualify the exact reset consumer under its own source owner; do not count it as a local Ghore room or duplicate the prototype. |
| **GHORE-Q16.** Will the three reciprocal WLD boundaries to Torture, Troll Hills, and Shafts retain their current endpoints, with installed crossing and return claims qualified separately? | The three active pairs are listed in the static connectivity table; the table does not establish live player travel. See [connectivity](#static-connectivity-and-code-defined-routes). | Preserve the paired source records and current ownership; do not claim a route is installed, safe, or repeatable from reciprocity alone. |
| **GHORE-Q17.** Is the unregistered `ud_map0` up exit from #1104885 to Ghore #11548 intentionally source-only, distinct from Ghore's active down exit to Shafts? | `ud_map0` is not in `areas/AREA`; its north-world reference does not activate ingress or prove a return. See [source-only approach](#static-connectivity-and-code-defined-routes), [ud_map0 exit](../../../areas/wld/ud_map0.wld#L29619), and [active Ghore boundary](../../../areas/wld/ghore.wld#L1061). | Preserve the map0 tuple under its candidate owner; do not activate it or infer a return through the separate Shafts edge. |
| **GHORE-Q18.** Should Ixarkon's `illithid_teleport_veil` continue to include Ghore #11545 as a possible destination? | The item, assignment, and handler define a possible arrival outside the WLD graph. See [code-defined route](#static-connectivity-and-code-defined-routes), [reset](../../../areas/zon/ixarkon.zon#L156), [assignment](../../../src/specs/specs.assign.c#L1874), and [handler](../../../src/specs/specs.ixarkon.c#L18). | Keep this source-defined possibility distinct from static ingress; do not promise or add an ordinary route without its code and runtime conditions. |
| **GHORE-Q19.** What payment, eligibility, and return outcome should qualify the `ghore_paradise` GIVE path from #11666 to #11667? | Guard #11542 has the special assigned, and its procedure handles the up passage and a GIVE route. See [special assignment](../../../src/specs/specs.assign.c#L944) and [handler](../../../src/specs/specs.ghore.c#L31). | Retain the source-defined conditions until the area owner confirms them; do not describe this as a free or safe entrance or claim a successful journey without qualification. |
| **GHORE-Q20.** Are the `guild_guard` branches for west from #11603/#11633, south from #11619, east from #11685, and up from #11812 intentionally unbound, or are they stale/reserved code? | The code has those cases, but no Ghore mobile is assigned `guild_guard`; only #11685 has the noted guard resets, while the corresponding doors reset closed. See [guild gate findings](#static-connectivity-and-code-defined-routes), [procedure](../../../src/specs/specs.guards.c#L121), [additional cases](../../../src/specs/specs.guards.c#L240), [assignments](../../../src/specs/specs.assign.c#L944), and [door resets](../../../areas/zon/ghore.zon#L32). | Keep the branches described as unbound source behavior; do not claim class enforcement is active or add bindings without an owner decision. |
| **GHORE-Q21.** What trap behavior, escape, and safety outcome are intended for #11684's `F 20` marker and down exit to #11686? | The room has an F field and a down exit; the loader handles the field, but the static source is not a qualification of effects or safe return. See [trap room](../../../areas/wld/ghore.wld#L3391) and [loader handling](../../../src/world/db.c#L1395). | Preserve the source finding and qualify actual behavior separately; do not promise damage, repeatability, a safe route, or a return from the marker alone. |

## Disposition and open decisions

Ghore is an active `areas/AREA` contributor and must not be moved. Its `.zon`
title has no repeated current active title. Followed WLD history contains
content changes in `71d731c5b` and `f156e012e`, plus a 2009 path migration in
`642da5400`; none directly maps Ghore to a retirement or replacement. The
semantic identity questions above remain builder work. Typed static reset,
shop, QST, and selected special references are mapped here; editor/tooling,
persisted-player identity, actual stock/reset outcomes, and dynamic path safety
remain unqualified. The confirmed Nexus56 references are captured in the
[candidate-specific migration plan](ATTIC_MIGRATION_PLAN.md).

Do not infer that the three isolated rooms are reachable, that Paradise has a
safe return, or that the code-defined routes are available to ordinary
characters without the relevant handler, class, and room-state conditions.

## Checkpoint 805: current typed-dependency census

The current Ghore reset has 683 typed mobile/object operands: 365 M, 15 F,
108 O, 61 E, and 134 G command operands. It resolves 378 mobile operands to
103 local mobile prototypes and 180 object operands to 101 local objects. The
remaining object inputs are 122 active Heavens placements across 31 distinct
prototypes plus active Verz1 object #3097. Two mobile reset operands, #11625
and #11626, still have no current mobile prototype; both numbers resolve to local
WLD rooms, which do not satisfy the mobile operand. The #11626 room also appears
in compiled hometown metadata. Four local mobiles and six
local objects are not reset-used. There is no local Ghore QST file; the local
story sidecar has empty contact, story, and exclusion lists.

External reset roles include 48 active placements: `mobs_underdark` resets
mobile #11631 four times; Alatorin resets objects #11562, #11618, and #11619
19 times; Khildarak resets #11606/#11607; Surfacekeeps resets #11562 and
#11619. The unregistered Nexus56 ZON adds source-only object resets #11552,
#11553, and #11589. No outside explicit reset-room operand targets a Ghore
room.

The local 19-record SHP file uses Ghore mobiles as keepers and sells local
objects plus the six-shop Heavens stock already identified above. Active
Khildarak and Surfacekeeps shops also list Ghore objects #11606/#11607 and
#11562. Unregistered Nexus56 SHP lists #11552/#11553/#11589. No external shop
keeper reference targets a Ghore mobile. Active QST item fields point to
#11606/#11607 and #11519 in Icecrag and #11562 in Shipy; Icecrag and Shipy
story sidecars carry the matching typed item values for #11606/#11607 and
#11562. No outside QST giver header targets a Ghore mobile.

Ghore has four internal WLD door-key fields: key #11574 on the reciprocal
#11643/#11648 pair and key #11571 on the #11676/#11677 pair. Both key objects
are local and reset locally; no outside WLD key field targets a Ghore object.
No local type-25 or type-29 object and no external type-25/type-29 target into
a Ghore room was found. The global source census remains 872 type-25 and 380
type-29 objects. These item checks do not add WLD exits.

Exact compiled references include #11545 in githyanki/illithid Plane Shift,
psionic mind-travel, and Ixarkon veil destination tables; #11703 in Ghore
hometown/Chaos-port data; hometown room metadata #11610/#11613/#11626/#11638;
#11503 in Ghore justice data; and room #11666 to #11667 in `ghore_paradise`. `specs.assign.c` binds Ghore mobiles #11542,
#11559, #11640, #11518, and #11601, and room specials #11923 and #11901.
The shared `guild_guard` cases remain unbound to Ghore mobiles as recorded
above. The exact numeric scan found no object-specific compiled VNUM consumer;
other matching numbers resolve in room, mobile, hometown, or justice roles.
Runtime reset admission, shop/QST behavior, editor selection, and persisted
identity remain unqualified. Preserve current imports and external consumer
ownership; do not resolve the two missing mobile prototypes or activate
Nexus56 from these static matches.

See the [local reset census](../../../areas/zon/ghore.zon#L345),
[local prototype files](../../../areas/mob/ghore.mob#L1),
[active external resets](../../../areas/zon/mobs_underdark.zon#L20),
[Nexus56 source reset](../../../areas/zon/nexus56.zon#L1330),
[active shops](../../../areas/shp/khildarak.shp#L1008),
[Nexus56 shop records](../../../areas/shp/nexus56.shp#L255),
[Icecrag item fields](../../../areas/qst/icecrag.qst#L66),
[Shipy item field](../../../areas/qst/shipy.qst#L892),
[local sidecar](../../../areas/story/ghore.story.json#L11),
[Icecrag item mapping](../../../areas/story/icecrag.story.json#L383),
[Shipy item mapping](../../../areas/story/shipy.story.json#L1105),
[door-key WLD records](../../../areas/wld/ghore.wld#L2592),
[key prototypes](../../../areas/obj/ghore.obj#L531),
[Plane Shift array](../../../src/classes/innates.c#L2008),
[mind-travel array](../../../src/classes/psionics.c#L614),
[home-town data](../../../src/core/constant.c#L1730),
[Ghore hometown room table](../../../src/core/constant.c#L1932),
[Chaos port table](../../../src/combat/chaos.c#L33),
[justice row](../../../src/combat/justice.c#L70),
[special assignments](../../../src/specs/specs.assign.c#L772),
[room specials](../../../src/specs/specs.assign.c#L2505), and the
[coverage row](ZONE_COVERAGE_REGISTER.csv#L103).


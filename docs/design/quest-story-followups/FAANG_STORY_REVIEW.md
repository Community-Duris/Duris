# Faang: source story review

**Scope:** static review of Faang's rooms, mobile stories, sidecar/QST
boundary, shop, resets, coded behavior, local connectivity, history, and typed
cross-area references. No gameplay, server, or database behavior was qualified.

## Source identity and story coverage

Faang is an active full-area contributor under zone range 152–158
([area list](../../../areas/AREA#L84)). Its current sources define 538 rooms,
140 mobiles, 109 objects, one shop, and zero native Q/QA contracts. The
historical [areas/qst/AREA index](../../../areas/qst/AREA#L77) includes Faang,
but there is no areas/qst/faang.qst. The schema-two
[faang.story.json](../../../areas/story/faang.story.json) says coverage is
complete while contacts, stories, and exclusions are all empty; its
introduction still promises local requests. Treat this as an empty
classification boundary, not evidence of authored local quest chains.

## Environmental stories

- The surface approach centers on a sealed mountain entrance and the bloody
  name “FAANG” on a skull at the clearing. Inside, the old cavern is broad,
  noisy, slick with ogre and bat guano, and crowded with stalactites and
  limestone formations ([cavern opening](../../../areas/wld/faang.wld#L1),
  [sealed approach](../../../areas/wld/faang.wld#L1158)). A separate bat cave
  describes thousands of bats and treacherous guano, but its room record has no
  exits despite prose describing an upper route and a gap below
  ([bat cavern](../../../areas/wld/faang.wld#L1479)).
- Faang's ogre community is organized around a chief and champions, tribal
  warrior and shaman caves, mercenary spaces, family dwellings, training, an
  alehouse, and an inn ([shaman cave](../../../areas/wld/faang.wld#L1637),
  [warrior cave](../../../areas/wld/faang.wld#L1983),
  [mercenary cave](../../../areas/wld/faang.wld#L3794),
  [warrior school](../../../areas/wld/faang.wld#L10639)). The chief's cave
  describes bloody succession challenges
  ([chief's cave](../../../areas/wld/faang.wld#L2770)); trails lead
  through family homes and caves ([homes](../../../areas/wld/faang.wld#L7348),
  [family dwelling](../../../areas/wld/faang.wld#L7437)), and Gronk's Alehouse
  is a gathering place for merchants and mercenaries
  ([alehouse](../../../areas/wld/faang.wld#L7533)). Room #15264 is a sleeping
  cave described as an inn and is directly assigned the inn procedure
  ([inn](../../../areas/wld/faang.wld#L1424),
  [inn assignment](../../../src/specs/specs.assign.c#L2514)). Room #15302
  describes boulders prepared for defense
  ([ledge](../../../areas/wld/faang.wld#L2161)).
- The underground rivers, hot springs, waterfalls, and Ogre Lake support
  distinct cave wildlife and water-adapted creatures. The river text describes
  a strong downstream current and an unseen drop
  ([river](../../../areas/wld/faang.wld#L1209)); the lower waterfall is called
  deadly and leads toward a deep pool
  ([falls](../../../areas/wld/faang.wld#L10741)). Twenty-four rooms carry fall
  fields and thirteen carry current fields in the source register.
- A vertical chasm links the upper “Old Faang” to a much lower “New Faang”;
  blood-streaked climbing routes and a spiral tunnel descend into the
  Underdark ([chasm](../../../areas/wld/faang.wld#L4623),
  [old-to-new Faang tunnel](../../../areas/wld/faang.wld#L4702),
  [spiral tunnel](../../../areas/wld/faang.wld#L4739)).
- An 18-room illithid settlement sits in a separate local graph component
  entered through a tunnel to Ixarkon. Its environment includes gates made from
  a transport vessel, slave pens, a council chamber, a bazaar, a bank, an inn,
  a shop, and a room titled for scourging implements
  ([entrance](../../../areas/wld/faang.wld#L4315),
  [gates and branches](../../../areas/wld/faang.wld#L4333),
  [slave pens](../../../areas/wld/faang.wld#L4376),
  [council](../../../areas/wld/faang.wld#L4446),
  [bazaar](../../../areas/wld/faang.wld#L4508),
  [inn](../../../areas/wld/faang.wld#L4551),
  [scourging room](../../../areas/wld/faang.wld#L4583)). Review the captivity
  and torture framing before turning these scenes into promoted story cards.
- Faang's sexualized and coercive framing appears in multiple places. The
  Wasteland Whorehouse room describes bouncers watching “slave girls” and
  private rooms for a fee ([room](../../../areas/wld/faang.wld#L8859)); mobile
  #15210 is labeled the chief's concubine and described with exposed skin and
  bite marks ([mobile](../../../areas/mob/faang.mob#L125)); and the alehouse
  prose says Wurtrka can get “a little nasty” in a straw-filled room after
  hours ([room](../../../areas/wld/faang.wld#L7533),
  [mobile](../../../areas/mob/faang.mob#L1086)). That implication is
  ambiguous. Review consent and captivity framing before promoting these
  scenes as quest stories.

## Builder findings

- The bat cavern room #15267 is an isolated singleton in the authored WLD
  graph. Its prose says “up” and describes a gap down, but the room has no
  direction records, no mobile/object placement reset targets it, and no
  direct special assignment was found. The same number is also the turtle
  shell eyepatch object equipped on mobile #15210 in the same room; that object
  reference is not a room placement ([room](../../../areas/wld/faang.wld#L1479),
  [mobile](../../../areas/mob/faang.mob#L125),
  [object](../../../areas/obj/faang.obj#L648),
  [equipment reset](../../../areas/zon/faang.zon#L292)).
- Four editor-marked rooms are also singleton components: #15382
  “PLACEMENT CHECK,” #15408 “PLACEMENT CHECK2,” and #15409–#15410 “UNUSED.”
  No M/O placement reset targets these rooms and no direct procedure
  assignments were found ([#15382](../../../areas/wld/faang.wld#L3788),
  [#15408–#15410](../../../areas/wld/faang.wld#L4282)).
- Room #15554 is titled “Null” and carries placeholder prose, yet its WLD
  record does include an east exit to #15526. That exit has zero door flags,
  while the ZON source contains a D reset for it; the reciprocal west exit at
  #15526 is flagged and has its own reset. This is a builder check on whether
  the zero-flag exit's D reset is intentional, not a missing WLD edge
  ([room and exit](../../../areas/wld/faang.wld#L5687),
  [reciprocal entrance](../../../areas/wld/faang.wld#L5146),
  [door resets](../../../areas/zon/faang.zon#L48)).
- The 54 source exits with nonzero raw state values and 54 ZON D resets do not
  map one-for-one: the hidden east exit from #15299 to #15334 has raw state 4
  but no D reset, while the D reset on #15554 is attached to its zero-state
  east exit
  ([#15299 exit](../../../areas/wld/faang.wld#L2104),
  [ZON reset list](../../../areas/zon/faang.zon#L8),
  [WLD state handling](../../../src/world/db.c#L1567),
  [D reset state handling](../../../src/world/db.c#L7893)). Builder review
  should establish the intended initial door state: the room loader masks WLD
  state to its low two bits, while a D reset adds the secret bit.

## Shop, resets, and typed dependencies

Faang's SHP source contains one keeper record, #15226. Its listed categories
are buy types; it contains no explicit sale-item VNUM list
([shop](../../../areas/shp/faang.shp#L1)). The ZON source places mobile
#15226, an ogre merchant, in room #15366 and follows it with eleven G resets
for Heavens objects
([merchant placement](../../../areas/zon/faang.zon#L310),
[mobile](../../../areas/mob/faang.mob#L320)). Those reset records establish
source inventory only, not live stock behavior.

Faang also imports active objects owned by Heavens: #90 once, #361 three
times, and eleven objects in the merchant's inventory (#101, #612, #633,
#699–#703, #363, #368, and #369). This is fifteen reset instances across
thirteen VNUMs. Object #90 also appears in an unregistered Mavulsk copy, but
Heavens is the current active owner
([Faang #90 reset](../../../areas/zon/faang.zon#L226),
[Faang #361 first reset](../../../areas/zon/faang.zon#L232),
[Faang #361 last reset](../../../areas/zon/faang.zon#L236),
[merchant stock first reset](../../../areas/zon/faang.zon#L311),
[merchant stock last reset](../../../areas/zon/faang.zon#L321),
[Heavens #90](../../../areas/obj/heavens.obj#L1171),
[Heavens #361](../../../areas/obj/heavens.obj#L3757),
[Heavens #363](../../../areas/obj/heavens.obj#L3783),
[Heavens #368](../../../areas/obj/heavens.obj#L3836),
[Heavens #369](../../../areas/obj/heavens.obj#L3850),
[Heavens #612](../../../areas/obj/heavens.obj#L6418),
[Heavens #633](../../../areas/obj/heavens.obj#L6632),
[Heavens #699–#703](../../../areas/obj/heavens.obj#L6992),
[active area list](../../../areas/AREA#L2),
[Mavulsk copy](../../../areas/obj/mavulsk.obj#L1469)).
Faang resets active Verz1 object #3097 once
([reset](../../../areas/zon/faang.zon#L231),
[prototype](../../../areas/obj/verz1.obj#L1199)).

Active foreign sources consume Faang prototypes:

- Heavens has four `P` resets that use Faang wine bottle #15245 as the child
  object inside container #1212
  ([reset group](../../../areas/zon/heavens.zon#L225),
  [last reset](../../../areas/zon/heavens.zon#L228),
  [prototype](../../../areas/obj/faang.obj#L418)).
- Alatorin has three resets of Faang bat #15224
  ([first reset](../../../areas/zon/alatorin.zon#L3691),
  [last reset](../../../areas/zon/alatorin.zon#L3693),
  [prototype](../../../areas/mob/faang.mob#L296)).
- Malch has four resets of Faang spider #15213
  ([first reset](../../../areas/zon/malch.zon#L334),
  [last reset](../../../areas/zon/malch.zon#L337),
  [prototype](../../../areas/mob/faang.mob#L162)).
- Khildarak resets Faang beer bottle #15242 into keeper stock and lists it in
  the matching shop record
  ([reset](../../../areas/zon/khildarak.zon#L2389),
  [shop stock](../../../areas/shp/khildarak.shp#L167),
  [prototype](../../../areas/obj/faang.obj#L394)).
- Icecrag's QST lists Faang brain #15252 as a `G I` item input for Masha's
  cuisine project; the story sidecar repeats it in the excluded contract's
  `give` key ([QST input](../../../areas/qst/icecrag.qst#L152),
  [story input](../../../areas/story/icecrag.story.json#L784),
  [prototype](../../../areas/obj/faang.obj#L482)).

These are typed static references across active sources. They are not proof of
runtime reset success, shop behavior, or quest completion.

## Static connectivity and code-defined hazard

All 1,150 Faang WLD exits resolve. The local undirected weak graph has seven
components: a 515-room main component, an 18-room illithid enclave, and five
single-room components described above. Seven active WLD boundaries are
paired with reverse-direction records:

| Faang boundary | Active peer and return | Evidence |
| --- | --- | --- |
| #15202 south to Underdark #797529 | #797529 north to #15202 | [Faang WLD](../../../areas/wld/faang.wld#L41) · [exit ledger](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L271) · [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1715) |
| #15254 west to Surface #597479 | #597479 east to #15254 | [Faang WLD](../../../areas/wld/faang.wld#L1187) · [exit ledger](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L272) · [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1570) |
| #15255 south to Underdark #818127 | #818127 north to #15255 | [Faang WLD](../../../areas/wld/faang.wld#L1209) · [exit ledger](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L273) · [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1736) |
| #15255 west to Underdark #817726 | #817726 east to #15255 | [Faang WLD](../../../areas/wld/faang.wld#L1209) · [exit ledger](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L274) · [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1735) |
| #15411 down to Surface #597886 | #597886 up to #15411 | [Faang WLD](../../../areas/wld/faang.wld#L4297) · [exit ledger](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L275) · [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1572) |
| #15412 south to Ixarkon #96474 | #96474 north to #15412 | [Faang WLD](../../../areas/wld/faang.wld#L4315) · [exit ledger](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L276) · [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1152) |
| #15557 north to Connectorzones #54232 | #54232 south to #15557 | [Faang WLD](../../../areas/wld/faang.wld#L5750) · [exit ledger](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L277) · [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L729) |

Fall markers occur in 24 rooms, with values from 5 to 100; current markers
occur in 13 rooms (#15255–#15261 and #15401–#15406). Rooms #15393 and #15411
carry 100 fall values; #15799 and #15805–#15807 carry 90, and #15801 carries
80 ([#15393](../../../areas/wld/faang.wld#L3986),
[#15411](../../../areas/wld/faang.wld#L4297),
[#15799](../../../areas/wld/faang.wld#L10741),
[#15805–#15807](../../../areas/wld/faang.wld#L10859)).
The WLD prose also describes slick slopes, a strong underground current, and
deadly falls, so these are material source hazards rather than ordinary route
assumptions.

The boulder pusher is a code-defined hazard separate from the WLD graph.
Mobile #15200 is assigned the Faang boulder procedure and is reset into
#15299–#15302; the code checks targets in #15253 or #15302, then may push a
boulder at a visible, non-evil character below immortal level. The procedure
can damage or kill a target and can rarely stun or knock one out
([assignment](../../../src/specs/specs.assign.c#L1016),
[handler](../../../src/specs/specs.faang.c#L26),
[first pusher reset](../../../areas/zon/faang.zon#L275),
[last pusher reset](../../../areas/zon/faang.zon#L282),
[ledge](../../../areas/wld/faang.wld#L2161)). This is static code evidence;
no live hazard behavior or route safety was tested. The handler also has
branches for source rooms #15354–#15356 aimed at #15302, but no current Faang
reset for mobile #15200 was found in those rooms.

## History and disposition

A fresh normalized scan of all 442 current `.zon` title lines finds one
exact “Faang” owner: `faang`. Both current indexes list `faang *152-158`
([full-area entry](../../../areas/AREA#L84), [QST entry](../../../areas/qst/AREA#L77));
the title is in [faang.zon](../../../areas/zon/faang.zon#L2). The local
“Old Faang” and “New Faang” names do not appear as separate current `.zon`
titles. Their alias, predecessor, and regional identity remain an owner
question, not a proven second area or replacement.

Followed WLD, MOB, OBJ, ZON, and SHP histories remain on the `faang` stem from
the 2009 area-tree move. WLD history includes the 2010 additions for the
planetbound Illithid hometown (13bd0a95c and 621957204); these are edits to
Faang's map, not a tracked alternate-stem rename or whole-zone replacement.
No retirement or direct replacement mapping was found in the followed paths.
The story sidecar was added on 2026-10-02. The 2009 `642da5400` entry is the
area-tree path migration. Keep the active `faang` identity **DO NOT MOVE**;
retain the “Old Faang”/“New Faang” naming question in the [owner review](#direct-area-owner-questions-and-conservative-recommendations).


## Direct area-owner questions and conservative recommendations

These questions turn the source findings into owner decisions. They do not qualify editor/tooling behavior, persisted identity, actual reset outcomes, live shop behavior, dynamic reachability, or safe return.

| Question | Source evidence | Recommendation |
| --- | --- | --- |
| Should Faang's story sidecar remain marked complete while its contacts, stories, and exclusions are empty, despite the introduction promising local requests and the historical QST/AREA listing? | The local QST file is absent, and the sidecar is an empty classification boundary. See the [source identity and story coverage](#source-identity-and-story-coverage), [sidecar](../../../areas/story/faang.story.json), and [QST/AREA index](../../../areas/qst/AREA#L77). | Keep the empty story boundary distinct from an authored quest chain; reconcile the sidecar's coverage statement and promise only with owner intent, and do not invent local addressed requests or topics. |
| Is the bat cavern #15267 intentionally isolated, or should the prose-described upper route and gap have authored exits? | Its WLD record has no directions, no mobile/object reset targets the room, and no direct assignment was found; #15267 also names a separate turtle-shell object equipped on #15210. See the [bat cavern finding](#builder-findings), [room](../../../areas/wld/faang.wld#L1479), [mobile](../../../areas/mob/faang.mob#L125), [object](../../../areas/obj/faang.obj#L648), and [equipment reset](../../../areas/zon/faang.zon#L292). | Preserve the singleton until the owner confirms intended geometry; do not infer an exit from prose or treat the same-numbered object/equipment reference as a room placement. |
| Are #15382 and #15408 placement checks and #15409-#15410 unused rooms intentional isolated editor records, or should any be connected or populated? | All four are singleton rooms without M/O placement resets or direct procedure assignments. See the [builder findings](#builder-findings) and [room records](../../../areas/wld/faang.wld#L3788). | Leave these rooms isolated and unpopulated until the owner identifies intended placements or geometry; do not turn editor labels into inferred content. |
| Is the D reset on room #15554's zero-flag east exit to #15526 intentional, given the placeholder room text and the separately flagged/reset reciprocal west exit? | The east WLD exit has zero door flags but receives a D reset; the reverse west exit is flagged and has its own reset. See the [placeholder exit finding](#builder-findings), [#15554 exit](../../../areas/wld/faang.wld#L5687), [reciprocal exit](../../../areas/wld/faang.wld#L5146), and [door resets](../../../areas/zon/faang.zon#L48). | Keep both authored exit records pending owner review; correct only the specific flag/reset pair the owner identifies, and do not describe this as a missing WLD edge. |
| What initial state is intended where raw WLD exit values and ZON D resets do not pair one-for-one, including #15299 east to #15334 and the #15554 reset? | The loader masks WLD door state to its low two bits, while D resets add the secret bit; the #15299 exit has raw state 4 without D and #15554 has D on a zero-state exit. See the [door-state finding](#builder-findings), [#15299 exit](../../../areas/wld/faang.wld#L2104), [ZON resets](../../../areas/zon/faang.zon#L8), [WLD state handling](../../../src/world/db.c#L1567), and [D reset handling](../../../src/world/db.c#L7893). | Derive intended initial states from actual loader and reset semantics for each exit; do not infer state from aggregate counts or comments, and keep any source repair separately named. |
| Is underdark2011.wld #274261's up exit to Faang #15411 intended to remain a source-only historical approach, or is there an approved active-map decision? | The route exists in an unregistered WLD source, not in Faang's active inbound WLD inventory. See the [Underdark2011 candidate graph](UNDERDARK2011_AUDIT_REVIEW.md#candidate-wld-graph-and-boundaries) and its [source exit](../../../areas/wld/underdark2011.wld#L305491). | Keep it classified as source-only; do not activate, merge, or count it as live Faang ingress without a separate approved map decision. |
| Are the authored falls, currents, and their severe values intended hazards on the routes through Faang, and which routes can be described as safe? | The source marks 24 fall rooms and 13 current rooms; values reach 100, and room prose describes slick slopes, strong current, and deadly falls. See [static connectivity and code-defined hazard](#static-connectivity-and-code-defined-hazard), [fall examples](../../../areas/wld/faang.wld#L3986), [lower falls](../../../areas/wld/faang.wld#L10859), and [current route](../../../areas/wld/faang.wld#L1209). | Preserve authored hazard fields and text while intent is reviewed; do not promise safe traversal or repair thresholds without a separately scoped, evidenced builder decision. |
| Are the boulder pusher's active and dormant target branches intended as written, especially source rooms #15354-#15356 where the pusher has no current reset? | The assigned handler can damage or kill eligible targets below the ledge; current resets place the pusher in #15299-#15302, while the handler also contains #15354-#15356 branches. See [the hazard finding](#static-connectivity-and-code-defined-hazard), [assignment](../../../src/specs/specs.assign.c#L1016), [handler](../../../src/specs/specs.faang.c#L26), and [pusher resets](../../../areas/zon/faang.zon#L275). | Preserve existing code and placements pending owner confirmation; do not add resets to dormant branches or claim live hazard behavior from source alone. |
| How should the illithid captivity/torture scenes and sexualized or coercive references to the whorehouse, #15210, and Wurtrka be framed before any are promoted as story cards? | The review records slave pens, a scourging room, the whorehouse text, the chief's concubine description, and ambiguous after-hours prose. See the [environmental stories](#environmental-stories), [illithid rooms](../../../areas/wld/faang.wld#L4376), [whorehouse](../../../areas/wld/faang.wld#L8859), [mobile #15210](../../../areas/mob/faang.mob#L125), and [alehouse](../../../areas/wld/faang.wld#L7533). | Keep these scenes as source observations until owners define consent, agency, and intended framing; do not promote coercion or ambiguous sexualized prose as a player quest outcome by inference. |
| Is keeper #15226 intended to sell the imported Heavens stock currently reset into its inventory, and should the merchant catalog remain buy-category-only? | Faang has one keeper record with listed buy types and no explicit sale-item list; its ZON places Heavens objects after the merchant reset, but source records do not establish live stock behavior. See [shop, resets, and typed dependencies](#shop-resets-and-typed-dependencies), [shop](../../../areas/shp/faang.shp#L1), and [merchant resets](../../../areas/zon/faang.zon#L310). | Preserve current shop ownership and buy-type data; do not infer sale behavior, current inventory, or live transactions from reset records alone. |
| Should Faang preserve its typed foreign dependencies and consumers: Heavens and Verz1 imports, Heavens' wine-bottle use, Alatorin bats, Malch spiders, Khildarak's beer stock, and Icecrag's cuisine input? | Faang imports active Heavens and Verz1 objects; Heavens uses Faang wine bottle #15245 in four child-object resets; Alatorin resets bat #15224, Malch resets spider #15213, Khildarak resets and stocks beer #15242, and Icecrag QST plus story sidecar list brain #15252 as an input. Each reference resolves by prototype role. See the [typed dependency review](#shop-resets-and-typed-dependencies), [Heavens resets](../../../areas/zon/heavens.zon#L225), [Alatorin resets](../../../areas/zon/alatorin.zon#L3691), [Malch resets](../../../areas/zon/malch.zon#L334), [Khildarak stock](../../../areas/shp/khildarak.shp#L167), and [Icecrag input](../../../areas/qst/icecrag.qst#L152). | Preserve these cross-area references and coordinate any prototype removal or VNUM change with the owning builders; do not equate source placement with a live reset or quest outcome. |
| Is “Faang” the canonical area title, with “Old Faang” and “New Faang” remaining local region names rather than aliases, predecessors, or a replacement zone? | The normalized active title is unique; followed Faang history establishes no retirement or replacement mapping. See [history and disposition](#history-and-disposition) and the [Faang title source](../../../areas/zon/faang.zon#L2). | Retain the active `faang` identity and title; do not create an alias, predecessor, or replacement claim without owner evidence. |

## Checkpoint 796: current typed census refresh

The active Faang ZON contains 442 prototype-reset commands and 442 typed
operands: 356 `M`, two `R`, 51 `E`, 13 `O`, and 20 `G`. All 426 local
operands resolve to Faang mobiles or objects. The remaining 16 operands are
active object imports: 15 from Heavens and one from Verz1. No mobile import
or unresolved reset operand was found. The source records establish reset
references only; they do not establish successful admission or live stock
([first prototype reset](../../../areas/zon/faang.zon#L225),
[last reset](../../../areas/zon/faang.zon#L667)).

Heavens contributes object #90 once and #361 three times, plus 11 objects in
the merchant inventory (#101, #612, #633, #699–#703, #363, #368, and #369).
Verz1 contributes object #3097 once. These source imports resolve to current
same-type definitions. The earlier source-only Mavulsk duplicate of object
#90 remains separate from Heavens' current ownership.

The active inbound reset scan found 12 typed operands: Heavens uses Faang
object #15245 in four `P` child slots; Alatorin resets mobile #15224 three
times; Malch resets mobile #15213 four times; and Khildarak resets object
#15242 once. Khildarak's active SHP also lists object #15242 as stock. Icecrag
QST has `G I #15252`, an item input in Masha's cuisine project, and its story
sidecar repeats #15252 under `give` in the excluded contract mapping. These
are typed source references, not proof of a completed quest or live shop
transaction. No external QST giver header targets a Faang mobile.

The Faang QST file is absent despite the historical QST/AREA entry; its local
story sidecar has empty contacts, stories, and exclusions and contains no
typed VNUM value. Across the other 231 current story sidecars, the only typed
Faang match is Icecrag's #15252 input. No source-only ZON, QST, or SHP
consumer and no external MOB/OBJ duplicate of a Faang mobile or object was
found. Faang's one SHP keeper lists buy categories but no sale-item VNUMs.

Faang has no local type-25 teleport. Its four type-29 switch objects target
local rooms: #15281 to #15411, #15282 to #15393, #15283 to #15748, and #15284
to #15202 ([#15281](../../../areas/obj/faang.obj#L800),
[#15282](../../../areas/obj/faang.obj#L808),
[#15283](../../../areas/obj/faang.obj#L816),
[#15284](../../../areas/obj/faang.obj#L824)). The full current object-source
scan found 872 type-25 and 380 type-29 declarations, with no external
destination into Faang's 538 rooms.
These typed travel destinations do not add WLD exits.

The direct C/C++ scan confirms the existing mobile bindings: #15200 to
`boulder_pusher`, and #15213/#15215 to `poison`; room #15264 is bound to the
inn procedure and `specs.faang.c` contains the boulder-handler room branches.
No direct object-prototype code binding was found. Static references do not
qualify runtime behavior, editor/tooling, or persisted player data.

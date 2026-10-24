# Fort Boyard: source story review

**Scope:** static review of Fort Boyard's QST dialogue, all local room and
mobile narrative, objects, resets, WLD connectivity, file history, and typed
references. No routes, quest behavior, shop behavior, or reset outcomes were
tested live.

## Source identity and dialogue coverage

Fort Boyard is an active full-area contributor in numeric range 38100–38176
([area list](../../../areas/AREA#L183)). Its source defines 77 rooms, 36
mobiles, 45 objects, and three QST giver prototypes. Each giver has only `M`
topic-message records; together they contain seven topics. The local source has
no `Q`/`QA` completion contract, no formal SHP record, no story sidecar, and no
previous mapped source dossier
([coverage row](ZONE_COVERAGE_REGISTER.csv)).

Ible (#38100) answers `hi hello` and `servant fort`; Maiya (#38104) answers
`hi hello` and `fort master help`; Nixie (#38115) answers `hi hello`,
`servant fort`, and `ciuto`
([QST file](../../../areas/qst/fortb.qst#L1)). These are ordinary keyed
responses, not timed `qc_action` room messages: the timed path in the quest
handler looks for that explicit key form
([quest routine](../../../src/world/quest.c#L88)).

## Environmental story

- **A forest approach, river landing, and working harbor.** The route begins
  on dim forest paths; a boulder at #38101 warns visitors to stay away from
  the master and his fort. It reaches a pier and shipping office on the River
  Sema, then follows the rapid, debris-littered river to a calm pool and an
  earthen landing where
  visitors leave boats. Maiya identifies herself as a shipping navigator and
  tells visitors they need a ship to reach the main fort. Shipping crates,
  stores, and a captain's desk make the landing a working supply point. A
  labeled crate contains a captain's note, but the note prototype has only a
  generic ground description, not shipment information
  ([path and boulder](../../../areas/wld/fortb.wld#L21),
  [boulder's warning](../../../areas/obj/fortb.obj#L1),
  [pier and office](../../../areas/wld/fortb.wld#L79),
  [river](../../../areas/wld/fortb.wld#L213),
  [pool and landing](../../../areas/wld/fortb.wld#L339),
  [crate and note resets](../../../areas/zon/fortb.zon#L88),
  [crate and note prototypes](../../../areas/obj/fortb.obj#L40),
  [Maiya's topics](../../../areas/qst/fortb.qst#L18)).
- **An enclosed estate organized around food and service.** The guarded stone
  approach opens into a multi-level fort with red-carpeted halls, visitor
  bedrooms, a dining room, kitchen and pantry, servant quarters, library,
  arcane and earthly-magic laboratories, a guard observatory, royal rooms, and
  a prayer wing. Its sacred garden grows tomatoes, peppers, cucumbers, and
  lettuce for the chef; a white-lit druidic altar stands at its center
  ([fort approach](../../../areas/wld/fortb.wld#L376),
  [green hall](../../../areas/wld/fortb.wld#L513),
  [garden](../../../areas/wld/fortb.wld#L553),
  [altar](../../../areas/wld/fortb.wld#L643),
  [dining and kitchen](../../../areas/wld/fortb.wld#L836),
  [library and laboratories](../../../areas/wld/fortb.wld#L1087),
  [prayer rooms](../../../areas/wld/fortb.wld#L1498)).
- **A guarded household with conspicuous visitors and watchers.** The reset
  roster includes Captain Clome and pier workers, Maiya, gardeners, chef Akal,
  servants and guards, a visiting elven mage and ogre mercenary, librarian
  Hirsch, the transformed shaman Shamla, goblin spell student Alleb, thief
  Fontan, Mistress Seriana, Master Boyard, Queen Zida, Bishop Magib, and royal
  observer Moro. Their descriptions present a place with staff, guests,
  researchers, and royal security rather than an empty ruin. One observatory
  looks toward shipping traffic; the room text reports distant trolls chasing
  a small group of humans
  ([reset roster](../../../areas/zon/fortb.zon#L217),
  [Fontan](../../../areas/mob/fortb.mob#L437),
  [Boyard and Zida](../../../areas/mob/fortb.mob#L468),
  [distant view](../../../areas/wld/fortb.wld#L1389)).
- **A secret cellar and the captive dragon.** The altar hides a cellar beneath
  the garden; the cellar text describes rats and animals feeding on stored
  food. A blocked side opening reaches Ciuto's lair, described as a corpse-
  strewn cavern. Ciuto's mobile description says the master trapped the dragon
  under the fort and keeps it fed. Nixie's dialogue identifies Ciuto as the
  means by which the master killed her
  ([altar and cellar](../../../areas/wld/fortb.wld#L643),
  [lair](../../../areas/wld/fortb.wld#L806),
  [Ciuto](../../../areas/mob/fortb.mob#L207),
  [Nixie's account](../../../areas/qst/fortb.qst#L33)).

The QST text establishes two unresolved deaths. Ible says Boyard killed him
with a diamond-tipped dagger and says he does not know why. Nixie says she saw
Boyard engaged in a “devious act of pleasure,” after which he bound and gagged
her and fed her to Ciuto. Her `ciuto` topic ends with her refusing to discuss
him. The coercion and violence are explicit enough to merit a content-owner
check for tone and Nixie's agency; this review does not add a motive or further
interpret the vague phrase. There is also a source-level placement question:
the matching local diamond-tipped dagger (#38136) is equipped to Fontan in the
reset file, not Boyard
([Ible and Nixie messages](../../../areas/qst/fortb.qst#L1),
[`ciuto` response](../../../areas/qst/fortb.qst#L49),
[dagger placement](../../../areas/zon/fortb.zon#L253),
[dagger prototype](../../../areas/obj/fortb.obj#L387)).

The adjacent Prison of Fort Boyard adds a second framing: Captain Gixmo says
Boyard's army captures dragons for the prison and describes Boyard and Queen
Zida as good people, while offering a dragon-scale cloak quest. That dialogue
is in Prisonb's own QST source; it is narrative context, not a Fort Boyard
completion contract
([Captain Gixmo topics](../../../areas/qst/prisonb.qst#L8)).

## Resets, doors, and static connectivity

The static census records 163 authored exits and 163 loader-resolving targets.
All 77 rooms fall in one local weak component. Three exits leave Fort Boyard
for active areas and each has a reverse-direction record; three active exits
enter Fort Boyard. No room has a fall-chance field or numeric current field.
The River Sema's prose still describes rapids, and none of these counts proves
player reachability, flow, or a safe return path
([coverage row](ZONE_COVERAGE_REGISTER.csv)).

| Fort Boyard boundary | Paired source record |
| --- | --- |
| #38100 east to Surface #562320 | Surface #562320 west to #38100 ([Fortb](../../../areas/wld/fortb.wld#L1), [Surface](../../../areas/wld/surface.wld#L1308220)) |
| #38121 east to Prisonb #43041 | Prisonb #43041 west to #38121 ([Fortb](../../../areas/wld/fortb.wld#L395), [Prisonb](../../../areas/wld/prisonb.wld#L919)) |
| #38149 east to Prisonb #43000 | Prisonb #43000 west to #38149 ([Fortb](../../../areas/wld/fortb.wld#L993), [Prisonb](../../../areas/wld/prisonb.wld#L1)) |

Two more source records point into #38100 from WLD files outside `areas/AREA`:
unregistered `surf.wld` room #205016 north, and unregistered `underworld2.wld`
room #417529 north. The latter is a “Lava of Souls” room; the same file has a
separate map cell titled “FORT BOYARD” at #417229. These are source-only
references and do not add active-world entrances
([`surf.wld`](../../../areas/wld/surf.wld#L105300),
[`underworld2.wld` label](../../../areas/wld/underworld2.wld#L8937),
[`underworld2.wld` exit](../../../areas/wld/underworld2.wld#L10618),
[world-connectivity record](WORLD_CONNECTIVITY.md#L118)).

The resets mark the storage door closed and locked; key #38108 is put inside
the shipping captain's desk. The outside fort gates use key #38113, which a
reset gives to an elite guard posted before the entrance. The altar passage is
reset closed, secret, and blocked, with a secret return door; several bedroom,
pantry, and library doors also reset closed. These are authored initial states,
not proof of how a player reveals or opens each route
([door resets](../../../areas/zon/fortb.zon#L8),
[storage key placement](../../../areas/zon/fortb.zon#L96),
[gate guards and key](../../../areas/zon/fortb.zon#L237),
[keys](../../../areas/obj/fortb.obj#L100),
[altar passage](../../../areas/wld/fortb.wld#L643)).

The thick-brush object (#38143), reset in starting room #38100, says it blocks
travel south. That room's WLD record authors only a north exit and an east exit
to Surface. This placement/text relationship deserves a builder check
([brush reset](../../../areas/zon/fortb.zon#L81),
[brush description](../../../areas/obj/fortb.obj#L471),
[starting room](../../../areas/wld/fortb.wld#L1)).

There is no `fortb.shp` file or formal SHP record. A separate code special is
assigned to room #38107, the shipping office: `crew_shop_proc`, documented in
code as the crew hall for hiring crews and chiefs. The ship data also includes
the Boyard Naval Guard at that room. This code-backed service is distinct from
the SHP-record count of zero
([room assignment](../../../src/specs/specs.assign.c#L2403),
[crew-hall procedure](../../../src/ships/ship_shop.c#L3243),
[Boyard ship data](../../../src/ships/ship_variables.c#L315)).

## Typed dependencies

- Fort Boyard's resets import Prisonb object #43014, a large prison ship, onto
  the pier; object #72 into Boyard's throne room; and object #358 onto Boyard.
  In the active area list, object #72 is Heavens' magical fountain and #358 is
  Heavens' rune-covered stone. Unregistered Hostel and Mavulsk object sources
  also define VNUM #72, so any loader or archival decision should use the
  active-area owner rather than number alone. Boyard also receives Wh object
  #55173, named “memory of Fort Boyard”
  ([prison ship reset](../../../areas/zon/fortb.zon#L94),
  [fountain reset](../../../areas/zon/fortb.zon#L215),
  [Boyard's equipment resets](../../../areas/zon/fortb.zon#L270),
  [Prisonb ship](../../../areas/obj/prisonb.obj#L135),
  [Heavens #72](../../../areas/obj/heavens.obj#L1000),
  [Heavens #358](../../../areas/obj/heavens.obj#L3709),
  [other #72 definitions](../../../areas/obj/hostel.obj#L959),
  [and](../../../areas/obj/mavulsk.obj#L1173),
  [Wh memory](../../../areas/obj/wh.obj#L2226)).
- No imported foreign mobile was found in Fort Boyard's reset file. A scan of
  active `.zon` and `.shp` sources found no external reset or shop consumer of
  its local mobile or object prototypes. C++ does reference local object
  #38140, the silken red sleeves, in two summoner equipment tables
  ([object](../../../areas/obj/fortb.obj#L435),
  [standard kit](../../../src/account/chaos_eq_data.h#L308),
  [enhanceable kit](../../../src/account/chaos_eq_data.h#L552)).
- C++ also binds room #38107 to the crew-hall special and includes that room in
  Boyard naval/crew data. This static connection explains why a crew service
  can be present despite the absent local SHP file. No direct numeric Fortb
  mobile dependency appeared in the scanned QST or code sources.

## Duplicate and replacement history

The normalized active title scan finds no repeated active title for Fort
Boyard. The Prison of Fort Boyard (`prisonb`) is a separately named active
companion zone with two paired WLD boundaries and its own dragon-capture story;
the sources do not identify it as a duplicate or replacement. Unregistered
`underworld2.wld` has a cell titled “FORT BOYARD” and a source-only exit into
the active Fortb forest, but it contains no evidence here of a second active
Fort Boyard prototype set or whole-zone replacement
([Prisonb title](../../../areas/zon/prisonb.zon#L2),
[active area list](../../../areas/AREA#L183),
[legacy map cells](../../../areas/wld/underworld2.wld#L8937)).

Tracked WLD history shows one concrete boundary correction. In 2013,
`71d731c5b` changed #38100's east target from #279361 to #562320. The old
VNUM is a “Lightly Forested Hillside of the Dagger Peaks” room in unregistered
`surf.wld`; the new target is “A Rugged Highway” in active Surface, whose west
exit returns to Fort Boyard. The 2011 Fortb WLD edit is spelling-only, and the
2009 entry is the `areas/` path migration. This supports a destination update,
not retirement or replacement of the Fort Boyard room set
([current boundary](../../../areas/wld/fortb.wld#L1),
[old `surf` endpoint](../../../areas/wld/surf.wld#L1665301),
[active Surface endpoint](../../../areas/wld/surface.wld#L1308220),
[Surface return](../../../areas/wld/surface.wld#L1308239)).

## Direct area-owner questions and conservative recommendations

### Builder questions

| Question | Source evidence | Conservative recommendation |
| --- | --- | --- |
| **FBQ-01.** Will the active contributor retain the player-facing title "Fort Boyard" and remain distinct from the separately named Prison of Fort Boyard? | The active title and companion title have separate current contributors; bounded source history found no predecessor or whole-zone replacement. See [area list](../../../areas/AREA#L183), [Fortb title](../../../areas/zon/fortb.zon#L2), [Prisonb title](../../../areas/zon/prisonb.zon#L2), and [title/history evidence](#duplicate-and-replacement-history). | Retain both current identities and the title; do not alias, merge, or classify either zone as the other's replacement without new owner evidence. |
| **FBQ-02.** Is the `underworld2.wld` north exit from #417529 into #38100 intended to remain source-only, and does its separate "FORT BOYARD" map cell identify this active area? | The exit and map label are in unregistered Underworld2 source; they do not establish active ingress or a second Fortb prototype set. See [map label](../../../areas/wld/underworld2.wld#L8937), [exit](../../../areas/wld/underworld2.wld#L10618), and [identity review](#duplicate-and-replacement-history). | Preserve the bounded source finding and ask map/area owners to confirm intent; do not activate the edge or infer another active zone from the label. |
| **FBQ-03.** Should the unregistered Surf north edge from #205016 into #38100 remain documented as a historical source reference? | The outside Surf WLD contains a north-pointing reference to Fortb but is not registered in `areas/AREA`. See [Surf source edge](../../../areas/wld/surf.wld#L105300) and [active boundary census](#resets-doors-and-static-connectivity). | Keep it marked source-only unless the map and world owners separately qualify registration, route intent, installation, and return. |
| **FBQ-04.** Was the 2013 change from #279361 to Surface #562320 intentional, with the current reciprocal edge retained instead of restoring the old endpoint? | Tracked history records the endpoint change; current Surface #562320 returns west to Fortb, while the old VNUM belongs to an unregistered Surf room. See [history and current pair](#duplicate-and-replacement-history), [old Surf endpoint](../../../areas/wld/surf.wld#L1665301), and [current Surface return](../../../areas/wld/surface.wld#L1308239). | Retain the current target and history; do not restore #279361 without a separately reviewed map decision. |
| **FBQ-05.** Are the three reciprocal Fortb boundaries to Surface and Prisonb intended, and what installed entry and safe-return evidence should qualify player-facing travel claims? | The three local exits each have a paired active WLD record, but the static census does not prove installation or a player's crossing and return. See [boundary table](#resets-doors-and-static-connectivity). | Preserve the source graph and describe only source-record reciprocity until each claimed route is separately qualified in its installed context. |
| **FBQ-06.** Does the River Sema have an intended flow or boat-travel behavior beyond its rapid-water prose, and what route and return should be supported? | The river and boat landing are described in WLD text; no local room has a numeric current or fall-chance field. See [river and landing](../../../areas/wld/fortb.wld#L213) and [static connectivity](#resets-doors-and-static-connectivity). | Keep the environmental description distinct from a movement mechanic; do not promise flow, boat passage, or safe return from prose alone. |
| **FBQ-07.** Is the locked pier-storage door keyed by #38108 in the captain's desk intentional, and how should discovery and use be qualified? | The reset marks the storage door closed and locked and places #38108 in the desk. See [door reset](../../../areas/zon/fortb.zon#L8) and [key placement](../../../areas/zon/fortb.zon#L96). | Preserve the authored reset and key placement until the area owner confirms the intended lock, key custody, and player-facing route. |
| **FBQ-08.** Is key #38113 intended to be carried by the elite guard at the outer fort gate? | Gate resets place the key on the guard posted before the entrance. See [gate resets](../../../areas/zon/fortb.zon#L237) and [key prototypes](../../../areas/obj/fortb.obj#L100). | Retain the current assignment pending an owner decision; do not promise gate access or relocate the key from source inspection alone. |
| **FBQ-09.** Should the garden altar's passage and secret return retain their closed, secret, and blocked reset states? | The altar cellar route and reset states are authored separately from the WLD description. See [altar passage](../../../areas/wld/fortb.wld#L643) and [passage resets](../../../areas/zon/fortb.zon#L8). | Keep the authored states until their intended discovery and traversability are confirmed; do not flatten or advertise the passage based on its description. |
| **FBQ-10.** Is the thick-brush object in #38100 meant to pair its south-blocking prose with a separate item-teleport target? | The object is reset in #38100 and its text says it blocks south, while the room has only north and east WLD exits. The type-25 item stores target #149188 and CMD_PUSH (270); that room occurs only in the unregistered Duris3 WLD. See [brush reset](../../../areas/zon/fortb.zon#L81), [brush values and text](../../../areas/obj/fortb.obj#L471), [room exits](../../../areas/wld/fortb.wld#L1), [Duris3 target](../../../areas/wld/Duris3.wld#L1032949), and [generic item-teleport handler](../../../src/magic/spell_travel.c#L932). | Ask the owner whether the prose and item target are both intended; preserve both declarations pending review. Do not treat the item target as a south WLD exit or claim it resolves to an active room. |
| **FBQ-11.** Is the labeled shipping crate's captain's note meant to contain shipment information, or is its current generic description intentional? | The crate and note are reset together, while the note prototype has only a generic ground description. See [reset](../../../areas/zon/fortb.zon#L88) and [object prototypes](../../../areas/obj/fortb.obj#L40). | Leave the note's content and behavior unchanged until its owner supplies intended information or confirms the generic item. |
| **FBQ-12.** Should the shipping-office `crew_shop_proc` remain the service authority even though Fortb has no local SHP file? | Room #38107 is assigned `crew_shop_proc`; code documents it as a crew hall, and ship data includes the Boyard Naval Guard there. See [room assignment](../../../src/specs/specs.assign.c#L2403), [procedure](../../../src/ships/ship_shop.c#L3243), [ship data](../../../src/ships/ship_variables.c#L315), and [source inventory](#source-identity-and-dialogue-coverage). | Preserve the code-backed room binding and distinguish it from the absent SHP record; do not claim live shop availability until separately qualified. |
| **FBQ-13.** Should Ible, Maiya, and Nixie's seven ordinary keyed topics remain ambient dialogue unless an explicit completion contract is designed? | All three local QST givers have `M` topic messages only; the source has no `Q`/`QA` completion contract or story sidecar. See [dialogue coverage](#source-identity-and-dialogue-coverage) and [local QST](../../../areas/qst/fortb.qst#L1). | Treat the messages as dialogue, not a quest, reward, or completion receipt; define any future progression and durable outcome separately before describing it as implemented. |
| **FBQ-14.** Will the content owner review Nixie's posthumous testimony for tone and agency without adding a motive or detail the source does not establish? | Nixie's `ciuto` response describes coercion and her refusal to discuss Ciuto further. See [Nixie's account](../../../areas/qst/fortb.qst#L33) and [`ciuto` response](../../../areas/qst/fortb.qst#L49). | Keep the review bounded to the existing text and survivor agency; do not invent motive, consent, or additional violence. |
| **FBQ-15.** Is the diamond-tipped dagger's placement on Fontan intentional despite Ible's account that Boyard killed him with such a dagger? | Ible's keyed testimony and the reset placement are separate source records; the matching local dagger is equipped to Fontan. See [Ible's message](../../../areas/qst/fortb.qst#L1), [dagger reset](../../../areas/zon/fortb.zon#L253), and [dagger prototype](../../../areas/obj/fortb.obj#L387). | Ask the narrative and reset owners to resolve the apparent mismatch; do not infer proof of the murder or move the item without an explicit decision. |
| **FBQ-16.** Is Captain Gixmo's favorable description of Boyard in Prisonb intended as an independent viewpoint or as deliberate contrast with Fortb's accounts? | Gixmo's statements belong to Prisonb's QST and are not a Fortb completion contract. See [Gixmo's topics](../../../areas/qst/prisonb.qst#L8) and [Fortb testimony](../../../areas/qst/fortb.qst#L1). | Preserve the neighboring zone's dialogue as separate source; do not use it to erase Fortb testimony or invent a reconciliation. |
| **FBQ-17.** Are the reset imports of Prisonb ship #43014, Heavens objects #72 and #358, and Wh memory #55173 intentional with these exact active owners? | Fortb resets import each object; active ownership differs from unregistered duplicate definitions of #72. See [imports](../../../areas/zon/fortb.zon#L94), [Prisonb ship](../../../areas/obj/prisonb.obj#L135), [Heavens objects](../../../areas/obj/heavens.obj#L1000), [rune stone](../../../areas/obj/heavens.obj#L3709), [duplicate #72 definitions](../../../areas/obj/hostel.obj#L959), [and Mavulsk](../../../areas/obj/mavulsk.obj#L1173), and [Wh memory](../../../areas/obj/wh.obj#L2226). | Preserve exact active source ownership and typed reset references; do not resolve, move, or archive an import by prototype number alone. |
| **FBQ-18.** Should local object #38140 remain stable while both maintained summoner equipment tables reference it? | The silken red sleeves have a local object definition and are referenced in standard and enhanceable summoner equipment data. See [object](../../../areas/obj/fortb.obj#L435), [standard table](../../../src/account/chaos_eq_data.h#L308), and [enhanceable table](../../../src/account/chaos_eq_data.h#L552). | Preserve the prototype and both consumers unless the owning maintainers approve a separately scoped replacement with dependency evidence. |

This review records source text, topology, and static references only. Door
discovery, crew-shop availability, QST interaction, resets, source-only map
status, and safe routes remain unqualified.

## Checkpoint 804: current typed-dependency census

The current local reset scan contains 214 typed mobile/object operands. All 36
local mobiles and 45 local objects are reset-used. Four object operands import
Prisonb #43014, Heavens #72 and #358, and White Harbor #55173; all resolve to
the expected current active prototypes. No unresolved reset operand, outside
active or source-only reset consumer, outside QST item/giver, shop keeper or
product, typed sidecar reference, or same-type duplicate prototype was found.
Fortb has three QST giver headers but no G/R item rows, no local SHP, and no
story sidecar. Its WLD doors use local keys #38108 and #38113; no outside WLD
key field targets a Fortb object.

The local type-29 objects #38100, #38119, and #38123 target local rooms #38101,
#38132, and #38138. Local brush #38143 is type 25 and stores target #149188
with command value 270 (CMD_PUSH); #149188 occurs in unregistered Duris3 WLD.
The generic item-teleport handler selects the stored target with `real_room`,
so this declaration does not establish that the target is loaded or usable.
Prisonb plank #43023 is also type 25, stores Fortb room #38106 with command
value 264 (CMD_JUMP), and is reset in Prisonb room #43022. These item targets
are separate from WLD doors and exits.

Exact compiled references are local object #38140 in the standard and
enhanceable Chaos equipment tables and room #38107 in the Boyard ship data and
`crew_shop_proc` assignment. No other exact compiled mobile, object, or room
reference was found. Preserve the active Fortb graph, source-only Surf and
Underworld2 references, and current owner classifications; the item links do
not change WLD counts or candidate dispositions. Reset admission, editor
selection, runtime item use, and persisted state remain unqualified. See the
[typed source records](../../../areas/obj/fortb.obj#L471),
[Fortb reset](../../../areas/zon/fortb.zon#L81),
[Prisonb plank and reset](../../../areas/obj/prisonb.obj#L244),
[Prisonb placement](../../../areas/zon/prisonb.zon#L190),
[Duris3 target room](../../../areas/wld/Duris3.wld#L1032949),
[CMD_JUMP/CMD_PUSH constants](../../../src/cmd/interp.h#L299),
[item-teleport handler](../../../src/magic/spell_travel.c#L932),
[Chaos tables](../../../src/account/chaos_eq_data.h#L308),
[ship data](../../../src/ships/ship_variables.c#L315), and
[procedure binding](../../../src/specs/specs.assign.c#L2403).


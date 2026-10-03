# Alatorin: district progression draft and audit in progress

Priority 26, source area `alatorin`, canonical zone 831. **This is an interim
source review, not a comprehensive dossier or a deployed journal.** All native
Q/M blocks, all room/mobile/object prototypes, the shop file and every reset
family have been reviewed. Shared-handler qualification and selected foreign
supply paths remain before publishing the sidecar and marking this zone complete.

Player discovery, encounters, journals, achievements and new daily eligibility
require active, ready economic accounting. Several prerequisites deliberately
refuse that mode today. Describing a route does not make its sources or payment
supported. Native item delivery proves an accepted exchange; it does not prove
personal hunting, allegiance, travel or every earlier preparation step.

## Reviewed evidence and remaining boundary

The [native source](../../../areas/qst/alatorin.qst) contains 961 blocks:
482 QA, thirteen Q, 285 MA and 181 M. These yield 495 distinct native
contracts across 82 recipients, including 93 contracts with a coin offering.
There are 296 nonambient addressed dialogue blocks. The conservative inventory
reports 291 because five families contain an apostrophe alongside usable
aliases: Odeth's `ardgral`; Ragmor and Wikzor's `strak`/`ash`/`blackrock`;
Ulster's other named proofs; and Gimabon's `grtak`/`leaders`.
Preserve usable commands in authored contacts and the raw source distinction
in the index. An addressed response still needs a separate accepted learned-topic
adapter before it can be historical progress.

The local files contain 952 rooms, 425 mobile prototypes, 602 object prototypes,
3,596 reset commands and fifteen shops. Resets comprise 1,085 M, 856 E,
511 G, 352 D, 342 O, 270 F, 164 P and sixteen R. All 1,821 reset families
are reviewed, retaining exact parents, destinations/slots, caps, conditional
flags, chance fields, duplicate counts and reserved fields. This is source
coverage, not proof of live availability or accounting admission.

Completed additional reads at this checkpoint:

- [All 952 rooms](../../../areas/wld/alatorin.wld): 83100–84055, including
  every description, field and exit of the existing rooms. Room 83343
  and rooms 84051–84053 are absent; mobile 83343 is a valid separate prototype.
- [All 425 mobiles](../../../areas/mob/alatorin.mob): 83100–83524,
  including every description and numeric field.
- [All 602 objects](../../../areas/obj/alatorin.obj): 83100–83701, including
  every numeric field and description. Mechanisms comprise 22 switches and
  28 teleporters. Item types, wear flags and extra descriptions reveal the
  reward and clue distinctions recorded below.
- [All fifteen shops](../../../areas/shp/alatorin.shp), including stock,
  keepers, rooms, trading types, prices, schedules and restriction fields.
- Both procedures and the completion helper in
  [the Alatorin source](../../../src/specs/specs.alatorin.c); the shared
  attribute-scroll, wondrous-ring, osquip birth/decay and paid-cleric functions;
  the parry handler; the ship-shop dispatcher and relevant mining refusal.
  Their wider dispatch, economic publication and recovery paths still need
  qualification. Existing Winterhaven, Quietus and Jade dossiers provide
  previously reviewed shared-service context.
- Bounded `surfacekeeps` road connections, four selected `surface` entrances,
  the `connectorzones` roc-cave connection, low-numbered blank paper in the
  loaded `limbo` file, and selected global stock declarations. Every boundary
  room listed below was read in full. This does not complete the source map of
  those foreign areas.
- All [3,596 reset commands](../../../areas/zon/alatorin.zon), grouped as
  436 M, 352 D, 341 E, 311 G, 189 O, 123 P, 66 F and three R families.
  The 1,469 non-D families complete the earlier D review. Every raw row has
  three trailing reserved zero fields; the boot parser does not use those
  fields as quest predicates. The loader and relevant probability helper were
  read for the distinctions below. Shared switch dispatch, current command
  IDs, door unlock and accepted key-destruction publication were also reviewed.
- All 336 foreign item prototypes named by local native I offerings/rewards:
  220 in [tradeskills](../../../areas/obj/tradeskills.obj) and 116 in the
  other loaded sources, including paper 5. Every field and description was
  reviewed, preserving exact repeated fields. This completes their prototype
  review, not every foreign supply route or the dossiers for those areas.
  The full [salvage module](../../../src/item/salvage.c), `get_matstart` and
  `do_refine` in [tradeskills](../../../src/economy/tradeskill.c) were also
  reviewed for the material preparation findings below.

The five literal local assignments are miner's helmet 83457, quarterstaff
83605, wondrous ring 83698, ship yard 83786 and doctor 83414. The computed
`VMOB_ALATORIN_STEELGRIP` assignment resolves to mobile 83187 and installs
`smelter`; a literal-only inventory misses it. Foreign osquip mobile/object
120051 use `wh_corpse_to_object` and `wh_corpse_decay`.

## District stories in proposed player order

This is a presentation order, not an enforced campaign. Keep independent
accepted requests visible after encountering their recipient, with services
and optional source routes explained alongside them. Full-family completion
and district pagination remain planned capabilities.

| Order / family | Reviewed progression and exact constraints | Journal treatment |
| --- | --- | --- |
| 1. Inn kitchens and everyday provisions | Four cooks convert particular haunches into three portions plus XP. Stonebrew's clam, apple, garlic and green herb produce its royal dish; BrightAxe uses pike, mandrake root, garlic and orange; the guild soup uses root, garlic, faerie dust and boar meat; Hammerhelm uses garlic, dragon blood, banana and boar meat. | Separate preparations with exact ingredients and outputs. Fish, forage and supplied ingredients are different source routes. Fresh-sounding prose is insufficient for a personal/freshness objective. |
| 2. The royal banquet | Each of four distinct prepared dishes pays the King's attendant for one culinary commendation token. Ten culinary tokens pay Modan for a medal of honor. The attendant also buys the ancient wedding band for 333,000 copper. | Explain a complete banquet route, while each actual dish receipt remains independent. A first dish is not proof that all four courses were served. The band also pays Taark and cannot fund both exchanges with one root. |
| 3. Collecting and the workshop | Dweefniggle has 223 exchanges. Each of 209 salvaged kinds 400000–400208 pays one collecting fragment; nebula material 400209 adds 200,000 copper. Vellum recipe 400210 pays three fragments; ten fragments make one collecting token. Strange object 1250 instead pays a full token and treasure. | A supporting collecting service may share proven equivalent one-item alternatives. Keep different rewards and ten-fragment assembly separate. A second native recipe consumes ten **tinkerer's tokens 83458** for one fragment; its source is still under investigation. |
| 4. Professional commendations and honor | Modan has seven exchanges: ten collecting, dragonslaying, mining, culinary, stealth or siege tokens each pay a medal, with family-specific extra gifts; ten medals pay 500,000 XP and key of honor 83338. Dweefniggle separately buys four medals for gnome currency 55033. | Name every physical token kind precisely. Explain competing uses and optional preparation history. Do not treat a collecting token, a tinker's token, a medal and gnome currency as interchangeable. Explain the honour-key entrance and separate key of kings; further stock/access qualification remains pending. |
| 5. Brewery, militia and guild rivalries | Dweefniggle accepts six brewer badges; Barlow accepts six tinker badges; Grendar accepts six barbed swords; Grem accepts six apprentice robes; Maggeynel accepts six hoods. Sturb accepts six silver axes and has twelve additional proof exchanges. Brudo requests Clund's head; Clund requests Brudo's sword. | Distinct requests and actual six-root counts. Several briefings say five. Native proof possession does not prove kills, enlistment or an exclusive side; journal metadata must not invent those conditions. |
| 6. Miboli's ten arcanums | Blank paper plus two of the **same** named component make each arcanum type. Several sylvan, lithic and sanguine materials are alternatives for the same output. One of each of the ten arcanums pays an enchantment scroll and gnome currency. | Preserve each arcanum identity and the ten-kind assembly. A combined live count across alternative materials must not report a mixed pair as a valid recipe. |
| 7. Attributes and wondrous power | Reciting enchantment scroll 55362 outside combat, through its `scroll` command match, replaces it with a random attribute scroll 55352–55360. Electrum ingot plus two **matching** attribute scrolls make one corresponding ring. All nine rings plus 500,000 copper make wondrous ring 83698. | Separate preparation services, nine attribute outputs and the finale. A fresh route needs at least eighteen random scroll outputs, but eighteen draws do not guarantee two of every kind. Supplied matching scrolls/rings remain valid. The paid finale is currently unavailable with active accounting. |
| 8. Mining and master crafting | Smidrin accepts either item 251 or a vellum recipe for a mining token. Harmon needs six **different large ore kinds** for a mining token and another gift. Gimbrin has 55 commissions: six of one exact tempered material, the matching armor part and 50,000 copper; premium metals also need gnome currency. | Distinguish current mining ore, salvaged tempered material and ready armor. Mining and smelting are deliberately unavailable with active accounting. Cold iron and steel can be terminal alternatives for equal equipment, but mixtures are not accepted recipes. |
| 9. Body enhancements, dragonscale and Xamora | Dweefniggle's five enhancements each need the matching armor, two gnome tokens, drug bag and 50,000 copper. Mundorno has 27 contracts, including 22 paid ones, competing common/chromium/silver scales, wings, shadow scales and a Noroth-head reward. Xamora's six commissions need eight of one exact material, two tokens and 200,000 copper. | Equipment services retain exact recipes and different outputs. Review Mundorno's overlapping wing-breastplate recipes before grouping; one demands a gnome token and the other does not. Xamora's material/output prose also needs a builder decision. |
| 10. Libraries, gardens and private royal requests | Three specific books pay the librarian for **wisp of deadly mist 83267**, despite medal prose. Baron Helgrim separately consumes Jedd's book for a medal and treasure. The mystic gardener requires all four named flowers together. The ancient wedding band has two recipients; Noroth's head has Taark, Queen Bellethra and Mundorno alternatives with different outcomes. | Keep exact outputs and competing allocation explicit. There is no receipt-backed requirement that the player personally visited every flower source or completed every royal errand. Royal keys are different physical kinds with different targets. |
| 11. Guild proofs and opposed outposts | Two rogue districts contain separately owned FrostSharn-heart, five-king-weapon, paired-heart, two-banner and map requests. Khoralator and Helgor each accept the other's head and retire. Ulster's independent proof requests, Ravi's two hearts, Locke's insignia, the prisoner key and the lion cub have their own outcomes. | Equal recipes or gifts at different recipients do not establish a shared story identity. Retiring proof recipients need runtime availability and attempt evidence. Narrated membership, healing and escape are separate from what the exchange actually implements. |
| 12. Deramuth Port and competing supplies | Bimk buys psychomia plants for money and drug bags. Naltem accepts five different barrels together for a stealth token, or buys drug bags separately. Jenk makes elemental potions, elixirs and elemental might; his signet-ring mission retires him, potentially ending those services for that appearance. Lundeen's egg soups compete with Modo, Dezik and the glutton. The clerk buys two Jade shipping-crate kinds at different prices. | Name quantity, contents/disposition, source and competing consumer. Brino's eleven buybacks, crystal sales and glutton food exchanges are commerce/services whose grouping needs explicit semantics, rather than a fabricated campaign. Ship and doctor services remain separate from native Q receipts. |
| 13. The First Mountain and divine gifts | Shard 83626 is declared as ground stock at Moradin's bust, room 84045. Two priests return a replacement shard without another reward; seven named gods each return a replacement shard and their own gifts, retiring after the exchange. Vergadain's coins 83308 and Abbathor's satchel 83617 are actually money-pile prototypes. The priest's sacred-key request has mithril and cash alternatives. Meshadan's token and cash tithe alternatives award a Shanat officer's key. | Track actual independent gift receipts and exact old/new shard lineage. Qualify the two money-pile grants separately from ordinary equipment and explicit C rewards. A same-kind return does not preserve the old UID. Divine narration is not proof of every temple visit or a single finished pilgrimage. Recipient availability and source paths remain under review. |

Other closures need explicit classification in the final map. Odeth's brew
uses faerie dust, dragon blood, strange stone, gnome currency and nebula
material, with **no native coin fee**, despite his quoted price. Jedd trades
four pyrolisk glands and a gnome token for two elixirs. K'tharz accepts a
physical coin object, which differs from wallet currency. Gruneegth's totem,
Taigorus's weapon/heart and Bimk's Eye of the Serpent have separate outcomes.
Borik returns two rejected bandages; his tourniquet exchange grants XP and
retires him, but its entire bandaging sequence is narration and describes
failure. It must not become a successful-healing objective. Several guide,
peacekeeper, flask and potion claims have no matching local terminal contract;
review source execution before declaring them unfinished.

Meshadan's two tithe alternatives actually award **Shanat officer's key 83258**.
The Legion wording describes the exchange; it does not record faction admission.
The key also appears in Meshadan's G stock. Frahzel 83262 and Fizz are separate
people: Frahzel's mobile description calls him Fizz's half-brother. Chungy's
Frahzel-head contract versus Fizz dialogue therefore needs a deliberate content
decision, rather than an alias substitution. Dumathoin 83520 is the eighth
avatar, with no local Q/M blocks; retain seven actual shard-gift requests.

## Access and source findings

Alatorin's interior rooms connect into the separately loaded
[`surfacekeeps` world](../../../areas/wld/surfacekeeps.wld), listed as
1200–1238 in [AREA](../../../areas/AREA). For example, 83100 joins 120667,
and 83103 joins 122103, with real reverse connections. These are not missing
destinations. Native contracts remain owned by their recipient's catalog area;
the future city-family presentation must not award duplicate discoveries or
receipts merely because a path crosses those area boundaries. The client
currently treats that outer range as wilderness and hides ordinary room numbers.
Qualify both canonical discovery and useful city navigation before adding a
district map.

Reviewed interior locks already distinguish the Hammerhelm barracks' key
83198, inner citadel key 83105, private chamber key 83279 and depository vault
key 83630. Queen's reward 83307 and Modan's honor key 83338 are further kinds,
not aliases of those keys. A key offering, live possession, accepted unlock
and actual arrival need separate evidence. Supplying a finale item does not
require replaying the optional route that produced it.

Some useful alternatives are ordinary stock: key 83198 is also carried by its
guard; collecting token 83246 is in a chance-limited container; wedding band
83249 and military letter 83272 are nested supplies; a separate outer-city
carrier has a medal. Alatorin's fragment 83245, honor key 83338, depository key
83630 and enchanted helmet have no selected reset declarations, but native
recipes produce them. **No reset declaration is not the same as no source.**
Tinker's token 83458 needs the remaining producer/custom-source review.

Blank paper 5 exists in active `limbo.obj`, although the playable-area prototype
inventory omits it. Do not describe that as a missing prototype or replace it
with another paper kind. Build inventory/prototype resolution from loaded data,
while keeping administrative areas ineligible for player achievements.

### Declared supplies and reset episodes

All 82 native recipients have local M/F declarations, but a declaration is not
an available appearance. In [the reset loader](../../../src/world/db.c), an
ordinary M/O row passes its initial eligibility check only with chance 100
and room/global conditions, unless `force_item_repop` is nonzero. The force
path then rolls the configured chance. G/E/P and F/R have different cap and
chance rules; [item load checks](../../../src/core/utility.c) also halve the
chance for artifact items. Keep these actual rules when estimating readiness;
do not promise an independent roll of every declaration at each normal reset.

| Supply or recipient | Reviewed declaration | Journal consequence |
| --- | --- | --- |
| Rare court and port appearances | Brek 83341 has M chance 50; the grey elf prisoner 83355 has 85; Locke 83425 and Debmawr 83524 have 30; Baron Helgrim 83494 has 20. Each has only that non-100 local M row. The two High Old Ones priests instead have F declarations at chance 100. | Distinguish forced/initial population, live appearance, retiring native exchange and later repopulation. Neither elapsed time nor discovering the room proves the recipient will return. |
| Gardener's four flowers | Mishanen 83185 is an M chance-30 court visitor. She has four separate G chance-2 declarations: 83282, 83283, 83284 and foreign rose 32627. The gardener consumes all four together. The rose also has O stock in the loaded ethereal source. | These are optional sources with separate draws and shared caps, not a guaranteed four-flower bundle or a local belt prerequisite. Qualify the selected ethereal source before publishing that foreign route. Supplied flowers still meet the exact native request. |
| Three library books | Miboli 83140 and the summoner 83146 have E chance-10 book declarations 83268/83269 in slot 18; Jedd 83144 carries book 83270 at G chance 100. Jedd's book also has a competing Baron consumer. | Explain the exact books and carrier/slot state. A book's low load chance is not quest failure, and a library receipt does not establish personally collecting every book. |
| Nested proofs | Lion cub 83129 is P stock in nest 83128; royal signet 83242 is in box 83241 inside coffer 83139. Wedding band 83249 is in skeleton 83248. Military letter 83272 and clue notebook 83385 are separate P entries in coffer 83139. | Show container preparation and require the offered proof to be directly carried. Do not count the container itself, a same-named outer object or a reused historical receipt as the proof. |
| Prepared tokens and ore | A collecting token 83246 can be P stock at chance 55 in sacred chest 83254, competing with crafting the token from fragments. Chests and sacks also have separate ore/treasure declarations and shared kind caps. | These are alternate supplies, not personal collecting/mining history. A P command searches a live container by kind through `get_obj_num`; freeze the actual parent UID and placement in a future durable reset adapter. An adjacent O/G row alone does not prove which container received the item. |
| Missing reset prototypes | Two M rows at lines 3745–3746 reference mobile 93183 at tavern 83761. G line 3800 references object 57744 on shopkeeper 83359. Neither prototype was found in loaded data or other area prototype files; the renumbering loader disables unresolved arg1 commands. Blank paper 5 is a different case with a real loaded `limbo` prototype. | Record these as targeted stale-reference repair candidates. A builder must select an intended replacement or remove the obsolete declaration; do not guess a VNUM or award a fictional encounter/stock item. The rest of the tavern/shop stock remains separately declared. |

Fresh O/P/G/E issuance is deliberately refused during active accounting until
it has a durable reset-generation identity. This covers stationary switches,
teleporters, keys, clue pages, nested ingredients and NPC equipment as well as
ordinary loot. A previously recovered, admitted object is a separate source
case. The universal adapter must freeze the exact row/generation, random result,
cap decision, object UID and parent/carrier/slot before publication, then qualify
replay, replacement, multiple same-kind parents and restart. Restore source
availability through that adapter before claiming a fresh journal route works;
do not weaken the mandatory accounting gate.

There is no declared local reset or native-output producer for Shanat prison
key 83294 or tinker's token 83458. The global literal search for the token finds
its prototype and Dweefniggle's ten-token input, while same-numbered slime-slug
mobiles are unrelated. Custom-source execution still needs qualification before
classifying the token request as unfinished or choosing a source repair.

### Mechanisms, key routes and the First Mountain

The [object properties](../../../areas/obj/alatorin.obj), exact D resets and
[shared switch handler](../../../src/specs/specs.object.c) expose useful access
relationships without interpreting NPC prose. The fifty local mechanism
prototypes have all been reviewed. Resolve numeric triggers against the current
[command IDs](../../../src/cmd/interp.h): 70 is `hit`, 99 `open`, 65 `grab`, 259
`rub`, 270 `push`, 320 `touch` and 340 `pull`. Shrine 83618–83625 currently
trigger on **`hit`**, whereas `kneel` is command 383. Preserve this source fact
until builders choose whether to change the ritual command or its explanation.

| Route | Exact source-backed action and result | Qualification |
| --- | --- | --- |
| Private royal treasury | Push armoire 83110 in 83285 to expose its east passage to 83297. Push button 83107 in that closet to unblock 83298's west passage to treasury 83296. Queen's key 83307 unlocks 83299's west door to 83298; huge golden key 83443 fits the treasury's three other doors. | Two mechanisms, key unlocks and arrivals are distinct. A ground switch can affect a remote room. |
| Honour and the Hall of Kings | Modan's ten-medal exchange awards honour key 83338. Its target is 83502 north to 83554; the stair route crosses outer rooms 121444/121744 before continuing to the hall. Key of kings 83371 is declared at dais 83575 and fits 83945 north to 84013. | These are two different keys. Their value[1] is 100: a successful unlock attempts key destruction. Await the accepted key outcome before rendering it spent. |
| The sacred stone | Priest 83255 accepts ten dwarven mithril ingots or money for key 83253. It fits trapped container 83254, with a 50 percent break value. Praying at golden altar 83217 instead teleports to 83404; entering tear 83218 returns to 83397. | Chest access and altar travel are separate from the key's exchange and narrated divine attunement. |
| Mystic shadow and Ard'Gral | Enter portals 83120/83121 between 83310 and 83311. Direction-triggered teleporters 83122–83125 supplement the wrapping shadow field. Pull book 83195 at library 83307 to unblock its west passage to 83342, then enter rift 83141 toward 30453. Touch bauble 83138 to reach glass prison 83341, whose down exit returns to 83175. | Resolve exact object/command matches and live effects. This is an access route, not completion of Odeth's separate brew request. |
| Legion and rogue approaches | Officer's key 83258 fits several military locks, while some officer doors reset merely closed despite keyed raw data. Rogue galleries have secret corridors, keyless locked doors and a separate outer entrance. | Apply D state to each exact edge; the existence of a key field alone does not impose a key prerequisite. |

Eight shrine switches have confirmed reciprocal targets and blocked D states:

| Switch / stock room | Passage opened by the current `hit` trigger |
| --- | --- |
| Gorm 83618 / 83477 | 84046 down to 83477 and its reciprocal up edge |
| Vergadain 83619 / 83476 | 84047 down to 83476 and its reciprocal up edge |
| Abbathor 83620 / 83482 | 84050 down to 83482 and its reciprocal up edge |
| Sharindlar 83621 / 83480 | 84049 down to 83480 and its reciprocal up edge |
| Berronar 83622 / 83473 | 83489 down to 83490 and its reciprocal up edge |
| Clangeddin 83623 / 83474 | 83490 down to 83491 and its reciprocal up edge |
| Dumathoin 83624 / 84044 | 83491 down to 84054 and its reciprocal up edge |
| Moradin 83625 / 84045 | 84054 down to 84055 and its reciprocal up edge |

The first four targets are extra avatar load rooms. The other switches open
segments of the descent; Berronar/Clangeddin/Dumathoin/Moradin are declared in
83490/83491/84054/84055 respectively. Opening an exit establishes a world effect,
not an NPC encounter or a guaranteed arrival. Qualify current recipient
episodes separately, particularly after a shard exchange retires its recipient.

**Confirmed malformed mechanism:** rune-covered wall 83368 is a type-29 switch
with `touch`, target room 0 and direction north. The loaded
[`limbo` room 0](../../../areas/wld/limbo.wld) has no exits, so the shared handler
reports the nonexistent-exit failure. The wall is G stock on golem 83454.
Meanwhile, the purported kings gate at 83945 north has D state 10, meaning
locked and blocked; an ordinary player cannot unlock a blocked exit even with
key 83371. A key-only checklist would therefore give misleading guidance for
that route. The intended switch target is a builder decision; confirm where
the wall becomes interactable and validate the complete effect before repairing
it. Existing shrine/outer-road approaches mean this finding does not establish
that the entire mountain or all divine gifts are unreachable.

The general switch handler also mutates the near edge before dereferencing an
unchecked reverse edge. Extend the previously planned target-safe switch repair:
validate both applicable edges before mutation, preserve intentional secret or
one-way semantics, then publish the exact accepted effect. Include this malformed
wall, ground versus carried switches, NPC-held placement, already-open targets
and reset/replay cases. The repair must not invent a successful touch objective.

### Outpost approaches, hidden encounters and return routes

The completed world review adds these source-backed access candidates. They
belong alongside the relevant request as optional route guidance; neither the
native item exchange nor a room description records that the player followed
the route personally.

| Route | Exact source relationship | Journal and qualification consequence |
| --- | --- | --- |
| Helgor's cells and commander | Cell key 83354 fits 83597 east to 83601 and 83600 east/west to 83602/83603. Runed key 83365 fits 83594 north to General Helgor's 83614. The blocked south wall of 83600 leads to torture room 83616 through switches 83359/83360. | The prison-key delivery, actual cell unlock and prisoner escape are separate facts. Declared G parents are reviewed; generation and live availability remain unqualified. The commander door and torture wall use different access mechanisms. |
| Khoralator and the city brig | Large mithril key 83386 fits 83670 north to lord's room 83679. Brig key 83472 fits 83627 south to 83880 and its reverse door; it is G stock on Ulster 83348. | A bounty receipt does not prove jail release or entry to an opposed fort. Signage saying “dwarves only” is not sufficient evidence of a race predicate. |
| Deramuth and Xamora | Etched key 83427 fits 83796 south to manor 83797. Rusty gate key 83431 fits 83821 north to tower base 83826. Elemental key 83430 is declared on Xamora and another carrier; some of its tower doors are locked, others merely closed. | Keep each exact key and live D state distinct. Xamora's prototype and some room descriptions disagree about race/gender; resolve that prose without guessing an admission condition. |
| Sealed encounter rooms | Opening casket 83425 at 83699 unblocks 83814 north to 83699. Opening crate 83478 at 83941 unblocks 83942 down to 83941. Touching handprint 83512 at 83876 unblocks 83923 up to 83874. | The mechanism opens an exit from a load room. It does not itself prove the occupant appeared, reached the player, was defeated or supplied a quest proof. Track the effect and actual encounter separately. |
| Troglodyte throne and captive | Iron key 83560 fits 83868 north to 83869. Captive Borik's 83871 north to 83872 instead has a locked, keyless door. Slime teleporters 83573/83579 connect throne 83870 and channel 83957 using `enter`. | Do not tell the player that the throne key opens Borik's cell, or infer healing/rescue from his tourniquet receipt. The slime route is an alternate approach and return mechanism. |
| Worm-nest ascent | Vines 83577/83578 are ground stock at 83979/83978. Their actual `grab` trigger teleports to ledges 83971/83982; it is not the `climb` command. Adjacent shaft rooms have falling, and other side tunnels connect the nest route back to the river network. | Describe the usable command and destination, then require confirmed travel for a historical ascent. Falling into the nest does not establish a successful return. Supplied quest parts still bypass optional personal travel history. |

Four selected boundaries in [the loaded surface file](../../../areas/wld/surface.wld)
have exact return edges: 83678 up ↔ 518643 down at Khoralator's platform;
83713 up ↔ 522647 down above Helgor's shaft; 83714 north ↔ 519847 south
at the tundra caverns; and 83811 west ↔ 521443 east at Deramuth's waters.
The roc-cave route is also reciprocal: 83913 east ↔ 53801 west in
[`connectorzones`](../../../areas/wld/connectorzones.wld). These are loaded
neighboring rooms, not missing destinations or additional local quest owners.

The selected `surfacekeeps` review confirms return edges at the Steps of
Honour, 83557 down ↔ 121444 up and 83558 up ↔ 121744 down; at the molten
rift, 84023 down ↔ 122218 up; and at the Dhalnadar Span, 83498 east ↔
123446 west. In contrast, 83831 down points to 122947, whose loaded exits
are south and west, with no direct up edge. Its route needs return-path
guidance rather than an assumed reciprocal climb. Alternate entrances are
navigation candidates, not evidence that a given actor traversed them.

### Typed rewards, equipment and authored clue text

**Accounting source gap requiring qualification:** the native Vergadain and
Abbathor contracts declare `R I 83308` and `R I 83617`. Both prototypes are
`ITEM_MONEY` (20), with value[3] = 2,000 and the other three denominations zero.
They also appear as G stock on their respective avatars. Their names do not
make them a dagger or a wearable satchel. Preserve the exact native bindings;
describe them as physical money-pile rewards until a builder chooses otherwise.

The reviewed [quest recovery](../../../src/world/quest.c) routes these I slots
through the ordinary item-creation grant. The SQL
[item repository](../../../src/item/item_transfer_repository.c) can store a
money object's coin payload, but the corresponding
[accounted item transaction](../../../src/persistence/economic_sql_item_transfer_transaction.c)
builds its plan from item custody changes without currency postings. This
does not establish a balanced mint of the pile's value. The normal explicit
C reward path uses a distinct wallet currency transaction. Qualify both
custom money prototypes, their pickup/conversion and native settlement before
claiming the two divine rewards are supported with active accounting. This is
a source finding; no played reward or reconciliation journey has been run.

Add typed reward preflight before offering consumption and recovery publication.
Unsupported money-pile grants must retain the offering or committed obligation;
do not silently turn them into wallet C rewards or clear unpaid slots. A future
adapter must freeze the denomination vector, issuance source, exact recipient,
pile UID and any pickup/conversion, with matching currency and custody evidence.
Test mixed shard/treasure/money rewards, already-paid masks, rejection, donation,
disconnect, replay and restart without duplicate minting or lost entitlement.

**Confirmed equipment-slot mismatches:** Gimbrin's copper legplates 83646 have
wear flags 257 (`TAKE | ARMS`), and his silver sleeves 83650 have 17
(`TAKE | HEAD`). Neighboring leg and sleeve outputs use the expected legs/arms
flags. Verify intended slots, correct only those flags if selected, and exercise
the exact commission, wear operation and journal display. Their different
affects and prices are not evidence that those values need changing. Mundorno's
sabatons 83699/83700/83701 do use feet flags, but retain copied helmet/arm prose;
that is a separate description repair.

Checklist 83247 has fixed X marks, and notebook 83385 contains five authored
bounty pages. They are ordinary clue text, not a saved quest-status interface.
Some `_id_` equipment changes its visible name through
[identification](../../../src/magic/spell_identification.c), while
`_noquest_` is used by the
[random world-quest reward policy](../../../src/world/world_quest_policy.c).
Neither marker overrides the identity of an authored native Q contract. Resolve
progress from VNUM/UID and accepted events, display the actual current item
name and slot, and treat source descriptions or examination as guidance until
a qualified learned-clue adapter exists.

### Material preparation and foreign item identity

The foreign prototype review confirms 210 sequential material kinds
400000–400209, vellum recipe 400210, three treasure/experience rewards
400233–400235 and six large ore kinds used by Alatorin's native deliveries.
The other 116 kinds include ordinary supplies, remote trophies, crafting outputs
and quest rewards. Their source-file owner does not establish where the item
was obtained: an Alatorin exchange can create a Winterhaven-defined output.
Physical adamantite coins 44509 are type-8 treasure, whereas the divine coins
83308 are type-20 money. Use loaded types and exact bindings, not names, when
choosing accounting and journal semantics. A valid foreign prototype without
a reset declaration may still have a native or custom producer.

Selected supply routes have additional source-backed distinctions:

| Preparation | Reviewed source | Journal and qualification consequence |
| --- | --- | --- |
| Gardener's ethereal rose | `eth2.zon` line 133 declares O 32627, cap one, room 32685, chance 100. Rooms 32683–32687 in [the ethereal forest](../../../areas/wld/eth2.wld) were read in full; the source room has ordinary north/east/south edges. Mishanen's local G entry is another supply declaration for the same kind. | Show the selected ground and carrier alternatives with live generation/cap state. These local edges do not establish a complete journey to the plane or a mandatory belt. Mobile 32627 is a separately typed lost girl with a replacement-mob death special; matching VNUMs alone do not make it the rose's producer. |
| Psychomia plant | [Winterhaven](../../../areas/zon/wh.zon) declares four M 55246 rows, cap four/chance 100, in dispersal rooms 55402 and 55614. Both rooms and the treant prototype were read in full. `wh_corpse_to_object` loads one same-VNUM object in the death room and sets value[0]; it is not declared O/G plant stock. | Bind the actual treant death, room and output UID before optional personal recovery. Dispersal-room prose says two of each ingredient, but the reviewed handler creates one. As the [Winterhaven dossier](WINTERHAVEN.md) explains, this plant lacks the animal decay assignment; a duration-valued field alone is not proof of expiry. Do not invent a timed plant objective. |
| Pike and clam | `get_pole`, `do_fish` and the full fishing callback in [tradeskills](../../../src/economy/tradeskill.c) include 318/334 among twelve random kinds. Fishing needs a recognized directly carried pole, water, skill and a successful delayed check; interruption and exhaustion stop it. Its former fish timer is commented out. | Supplied fish satisfy the exact kitchen request without fishing history or a freshness deadline. Catch prose and XP precede the grant, whose boolean result is ignored. Qualify successful publication and accepted catch evidence before personal-fishing milestones; a cast or failed grant is insufficient. |
| Brew and kitchen forage | [Foraging](../../../src/cmd/actoth.c) uses terrain-specific random tables for 822–827, with race/class/luck and poison rules. `do_forage` explicitly refuses active accounting. Its helper places the selected output directly and requires template preflight during a port. | Treat forage as a guarded optional source, alongside supported supplied/shop/reset alternatives. Freeze actual terrain, random kind, poison and output identity, then publish the accepted result. A `_frg_` keyword or an ethereal garden description does not authorize foraging every ingredient there. |

These are bounded supply reads, not full foreign-zone audits. Complete the
remaining remote trophies, source dispatch and guarded preparations before
publishing a comprehensive Alatorin map. The supplied/native terminal policy
remains independent of optional personal routes.

**Confirmed active prerequisites:** both `do_salvage` and `do_refine` explicitly
refuse active accounting. The salvage guard also precedes material downgrading,
although its helper already submits one input and two outputs through a craft
transaction. Its presence does not make that command available in active play.
Keep collecting fragments, arcanum ingredients and armor materials available as
supplied/admitted inputs where the native exchange supports them, while clearly
marking personal salvage/refining routes as awaiting an accounting adapter.
This does not prove that every material or all Alatorin requests are unavailable.

Legacy salvage chooses its material grade from item value and material, can
produce one or two copies, and may add magical essence and a recipe. It varies
output prices using skill, level, luck and random draws; the recipe records a
particular target. Its existing preflight checks the material and eligible recipe
templates. The ordinary reward path grants outputs separately and finally
extracts the source even if an output grant was refused; two essence branches
also send success prose without checking their grant result. Port the entire
outcome, source and applicable tools together, freezing random choices, price,
recipe target and all outputs before publication. Preserve intentional skill-roll
failure as a distinct accepted destructive outcome, rather than treating an
authority/allocation failure as gameplay failure. Reuse the downgrade craft
transaction where it covers the selected outcome, without removing the outer
guard ahead of qualification.

The “67% chance” comment for two materials disagrees with the actual
`!number(0, 2)` condition. The current
[inclusive number helper](../../../src/core/random.c) gives that branch one
of three outcomes. Correct the explanation as a narrow source/comment repair;
changing the probability would be a separate balance decision.

**Confirmed legacy refining lifecycle defect:** the consumption loop calls
`OBJ_VNUM(t_obj)` after `extract_obj(t_obj)`, including computing the ore bonus
after extraction. Its failure messages also retain the selected material pointer
after consuming that material. [Extraction](../../../src/world/handler.c)
calls [free_obj](../../../src/world/db.c), which frees changed descriptions and
releases the object to [the pool](../../../src/core/mm.c); the
[VNUM/display macros](../../../src/core/utils.h) still dereference it. This is
an invalid object-lifetime dependency confirmed in source, not a reproduced
player crash. Active accounting currently blocks the function before it runs.

Plan a focused repair that captures the kind/bonus and stable display data
before retirement, with no subsequent access to released inputs. Exercise both
inventory orders and failure messages using a harness that invalidates released
objects/descriptions; pooled storage alone may hide the problem from a sanitizer.
Refining consumes two equal materials and raises their kind by one on success.
Its catalyst test recognizes old ore kinds 194–233, not large ore 400262 etc.
With exactly one old ore it avoids the 50,000-copper fee; with zero or multiple
ores it charges that fee, yet the loop still consumes counted ores. Preserve and
test that observed policy while a builder decides whether the multiple-ore
behavior is intended. Preflight the output and freeze the selected roots, fee or
catalyst, roll and outcome in a recoverable accounting operation before enabling
it. A rejected grant must not silently lose paid inputs or an earned output.

Qualification must cover zero/one/multiple catalysts, both inventory orders,
highest-grade rejection, missing templates, failed skill rolls, rejected/partial
grants, tool use, donated inputs, replay and restart. A committed preparation
receipt is different from merely holding a supplied material. Selected foreign
source/dispatch review still remains; all 336 prototype reads alone do not
establish their active availability or personal acquisition history.

## Required universal capabilities and repair proposals

| Finding | Required implementation and qualification |
| --- | --- |
| ZSQ-ALA-RECIPES: exact alternatives | Add a versioned recipe predicate with explicit any-of complete recipes and all-of exact per-kind counts. Allocate concrete unused root UIDs; never let the same root satisfy two slots. Tests must reject one of each alternative when two/six/eight identical kinds are required, yet preserve legitimate gifted supplies and any supported terminal alternative. Until then use exact single-kind checks or text guidance without aggregate readiness. |
| ZSQ-ALA-DISTRICTS: bounded large journals | Preserve the 256-row, 32-step, 64-item-check and 512-KiB bounds. Inventory safe equivalences before choosing rows; the 209 ordinary one-item fragment trades are an example, while differing rewards, recipients and retirement are not automatically equivalent. Add family/district expansion and pagination to a shared terminal/client projection; encounter visibility and receipt identity remain authoritative. Increase a bound only if a faithful final map actually requires it. |
| ZSQ-ALA-SERVICES: settlement and staged stock | Keep smelting, mining and paid-cleric refusal. A smelter adapter must admit two exact NPC/player-held ores, frozen price, actor, payment ownership and output together, with recoverable refund/publication and no mixing between helpers. Qualify both pay-first and ore-first sequences. Review detached consumed ore cleanup and unchecked output allocation before any re-enable. Cleric purchase and actual spell/resurrection outcome need separate settlement/effect proof. |
| ZSQ-ALA-MATERIALS: salvage and refining | Keep both active-mode guards, including the guarded downgrade path. Port exact inputs/tools, random grade/count/essence/recipe, dynamic prices and intentional failed outcomes as one recoverable preparation. Fix legacy refining's post-extraction reads and failure display using stable pre-retirement data; test invalidated pooled objects. Qualify the old-ore catalyst/fee policy and rejected grants, and correct the two-material probability comment without an unreviewed balance change. Supplied materials do not prove personal preparation. |
| ZSQ-ALA-SUPPLY: ground, death, fishing and forage | Keep exact source kinds and foreign ownership. Admit ground/reset issuance and death-born plants with source episode/output UID, without inferred timers or personal kills from delivery. Qualify fishing publication before catch prose/XP/history; retain forage refusal until preflight and frozen terrain/kind/poison issuance exist. Test authority rejection, movement/interruption, competing caps, gifts and restart. Dispersal-room quantities are content decisions, not creation receipts. |
| ZSQ-ALA-RANDOM: scroll and power publication | Freeze one random attribute result and consume its exact source scroll only with a recoverable output grant. Gifted outputs need custody, not invented recitation history. Wondrous-ring power changes and helmet `paydirt` use are separate effects; a timer or utterance alone proves no successful power. Test failures, replay, restart, donation and changed equipment. |
| ZSQ-ALA-DECAY: osquip preparation | The osquip death special creates item 120051 in its room, then initializes decay; the periodic consumer replaces expired remains with a generic corpse. Add accepted birth/expiry/replacement lineage and a precise episode clock before optional personal/fresh cooking objectives. Test donation, nested storage, expiry during delivery and reconnect. Retain native exact item acceptance. |
| ZSQ-ALA-CITY: ownership and map visibility | Resolve interior/outer-city membership deliberately. Keep contract owner, current recipient room, visited area and authored district separate. Test movement across both directions, discovered/undiscovered neighboring areas, hidden room-number client behavior and duplicate receipt projection. |
| ZSQ-ALA-ACCESS: actual commands and mechanism effects | Extract typed candidates from loaded object properties, current command IDs, exact D states, stock parents and reciprocal edges. Record accepted reveal/open separately from unlock, key destruction, confirmed travel and encounter. Add eight shrine targets, the remote treasury controls and malformed wall 83368 to target-safe switch qualification. Builders select the ritual command and intended wall target/placement; metadata alone cannot repair either. |
| ZSQ-ALA-TYPED: reward kinds and live item presentation | Preflight native I outputs against loaded prototype types before consuming offerings or publishing recovery. Qualify money-pile issuance, denomination/custody settlement and conversion separately from explicit C rewards; retain unsupported entitlement. Audit actual wear slots, current identified names, copied descriptions and static clue pages without treating cosmetic markers as native quest eligibility or progress. Add focused tests for the two divine money outputs, copper-leg/silver-sleeve slots, supplied items and restart. |
| ZSQ-ALA-RESET: supplies, appearance and parent identity | Implement durable reset issuance before promising fresh keys, controls, equipment or nested supplies under active accounting. Preserve actual M/O forced-versus-normal chance rules, G/E/P/F/R policy, caps, random draws, exact generation and live parent/slot. Preflight loaded references including administrative paper; repair stale 93183/57744 declarations only after a builder selects intent. Test low-chance recipients/books/flowers, multiple same-kind containers, admitted recovered stock and restart without duplicates. |
| ZSQ-ALA-CONTENT: builder decisions | Decide the intended librarian gift, five-versus-six badges, attendant/banquet rewards, Xamora prose and Mundorno's overlapping paid breastplate recipes from actual outputs. Quantify supply paths before selecting a source repair for tinker's tokens or foreign proofs. Existing dialogue could be stale rather than gameplay broken; preserve rewards and balance until that decision. |
| ZSQ-ALA-PILGRIMAGE / ALLOCATION | Specify all-stage pilgrimage/banquet/profession campaigns separately from independent terminals. Confirm exact competing item use, replacement shards, retiring Jenk/proof recipients and reset generations. A final gifted item can complete its request while leaving personal journey history unearned. |

The earring-named handler on the wondrous **ring** is not a naming-based defect:
it changes two distinct maximum attributes every ten minutes while worn and
blocks encrusting broadly. Its intended encrust scope needs a targeted dispatch
test before changing behavior. Likewise, actual `earthen`/`flaming` emissary
deliveries do not implement the world transformations described in dialogue.

## Next audit checkpoint

Complete selected foreign supply and shared-handler qualification;
resolve every gate, timed/random effect and quest-like orphan. Then classify
all 495 contracts into faithful story/request/service/exclusion rows, author
all usable contacts, publish the sidecar and reproducible index, and add
focused native/source regression cases. Keep the existing catalog and completed
25-zone count unchanged until that work is complete.

Native-file review baseline SHA-256:
`5b2ab0b2587ebaf17cc2b010e1d6e00e18e8984ae15e029df0a33fa8fea08913`.
The full [roadmap](../ZONE_STORY_ZONE_PRIORITIES.md) remains active; Newhaven
follows Alatorin. Played active-world journeys are a separate qualification
stage and have not been run at this checkpoint.

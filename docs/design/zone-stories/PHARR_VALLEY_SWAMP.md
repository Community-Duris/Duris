# Pharr Valley Swamp: the shard, paid ornaments and surviving voices

Priority98 of the original220-zone roadmap. This source-comprehensive mapping
covers all local contracts, dialogue, rooms, mobiles, objects, resets, shops and
relevant shared/foreign execution. It does not claim played accounting, source,
wallet, travel or reset qualification. New progress requires active, ready accounting.

The [schema3 journal](../../../areas/story/pods.story.json) has one story and
three paid service cards, five contacts/52 verified aliases and four optional
current-material rows. The existing schema suffices for the current requests.
The [generated audit](../../reference/zone-story-audits/pods.md) is the source index.

## Source closure

| Source | Complete review and implications |
| --- | --- |
| [Quests](../../../areas/qst/pods.qst) | Four exact contracts (one QA/three Q),18 M/six MA dialogue families. All inputs, outputs, responses and remaining-giver behavior reviewed. Five contacts include lore-only insect cloud, Knarlash and Keera.52 aliases are guidance, not52 achievements. |
| [Rooms](../../../areas/wld/pods.wld) | All375 records28500–28874,370 full prose/25 headers/24 complete metadata families,754 exact exits/115 relative-pattern families/five full exit-text families. Sole boundary28500W↔40389E in pharrvly reviewed. Twelve native controls, hidden/closed/blocked paths, ruins, water/fall/current loops and ordinary returns covered. |
| [Mobiles](../../../areas/mob/pods.mob) | All103 full prose and ID-paired numeric records28500–28602. All local actors placed; no imported mobile. Giver/source flags, actual follower equipment, teachers, shopkeepers and stranded-traveler lore reviewed. No literal local procedure assignment or local table-driven epic/smith binding found in the maintained sources. |
| [Objects](../../../areas/obj/pods.obj) | All99 complete records28500–28598:types/flags/values/effects/extras. Unplaced28552/53 are Q products. Imported358 full canonical record reviewed; reset references29555/38555 have no declarations. Exact shard/feather/hide prototypes, keyed mummy/tome, sacks/children,12 type29 controls, blossom/food/potion/lore props covered. No local type25 portal. |
| [Resets](../../../areas/zon/pods.zon) | All1349 commands:M1015/O145/G118/E25/D18/P18/F10.1179 exact/1259 M-ancestry context/297 expanded families read with complete arguments, locations, caps and conditions; actual F receiver additionally traced against reset_zone.1315 chance100/33 chance33/one chance35. Mode2. Missing imports disable at renumbering; valid following hide stock remains declared. |
| [Shops](../../../areas/shp/pods.shp) | All four full records:28566@28707 plants/essence/materials;28575@28790 totems/arrows/quiver;28582@28803 food/essence/potions/tongue;28586@28505 supplies/waterbreathing vial. Stock, markup, hours and restrictions reviewed. None is the chamberlain or crafter; shops retain their own accounting guard. |
| Shared execution | [Quest roots/coin refusal/reward recovery](../../../src/world/quest.c), [coin give](../../../src/cmd/actobj.c), [shop refusal](../../../src/economy/shop.c), [D/F decoding and missing-reference handling](../../../src/world/db.c), [item_switch](../../../src/specs/specs.object.c), [command IDs](../../../src/cmd/interp.h), [OPEN/UNLOCK/PICK](../../../src/cmd/actmove.c), [CARVE](../../../src/classes/new_skills.c), [object placement](../../../src/world/handler.c), [epic node](../../../src/world/epic.c) and [accounting/ready runtime guards](../../../src/world/zone_story_quest_runtime.c) reviewed; unchanged shared custody/continuation paths from preceding complete audits remain applicable. |
| Foreign closure | Five touching recipes:four local plus surfacemini hermitQA33. Full hermit dialogue/mobile/numeric/placement97907/room/reward97903 and boundary40389 reviewed. No foreign reset group. Global active registry object/exit/portal scan finds no portal entering this zone. Ahgra’s crystal-shard lore is not another matching28579 contract. |

## Exact native contracts and useful progression

I means exact item prototype, C means copper. Every giver remains. Requests
are independent; earlier personal kills, access, dialogue or receipts are not
native prerequisites. One consumed copy cannot satisfy multiple hand-ins.

| Giver/contract; actual placement | Exact accepted inputs → result | Journal role |
| --- | --- | --- |
| Chamberlain28533/QA33;28703 | I28579→C75000 | Crystal shard→75platinum; one story achievement/potential daily. |
| Chamberlain28533/Q39;28703 | C50000→I28598 | 50platinum→tongue key; paid service, guarded under active accounting. |
| Hunched podaling28576/Q187;28790 |6×I28554+I28555+C1000→I28552 | Six dark feathers+hide+1platinum→birdfeather headdress; paid service, guarded. |
| Hunched podaling28576/Q199;28790 |3×I28555+C500→I28553 | Three hides+500copper→woven neberihide necklace; paid service, guarded. |

Enter the swamp from Pharr Valley, listen to Knarlash and the chamberlain,
follow the duergar’s shard source and choose whether to return that proof.
This records the chamberlain’s request; it does not install him as emperor.
Seek Keera through the gray vines and learn the city’s past. Find the workshop
for ornament recipes and a later foreign hermit lead. These are useful player
routes, without a fabricated compulsory campaign or per-keyword achievements.

The full key route is a separate purchase→key→mummy→tome puzzle. The shard is
E16/cap1/chance100 on duergar28587 at28842, not inside the mummy. Mummy28575 at
28703 contains skull tome28576. Its values200/15/28598/20 include CLOSED and
LOCKED, but not PICKPROOF. Key28598 has100-percent break on successful UNLOCK.
Chamberlain also carries a key; supplied, borrowed, picked or already-open
paths cannot prove the player purchased one. No personal key receipt is
required for the shard outcome.

Six feathers and three hides mean separate current item roots, taken out of
containers and removed from equipment. Bird28537 G stock has feathercap25;
neberi28523/24/25/26 and one bird28537 have hidecap47. The intent podaling’s
woven sack28593 has P stock of both materials; P searches a globally matching
container. Source/own recovery and current possession are distinct. CARVE
creates generic prototype8, not either craft ingredient. Crafted ornaments
are separate outputs; necklace28553 bitvector2048 provides waterbreathing
when worn. Buying a vial or owning either reward does not record the crafts.

The three services are currently guarded because their inputs include coins.
Native fees are50platinum,1platinum and500copper. The spoken craft prices9/6
platinum disagree; both facts are visible and await builder intent. Item rows
reflect current possession without promising wallet eligibility or availability.
Current paid outputs can feed hermit97901’s foreign two-item request for recipe
97903. This material dependency does not require personal local craft receipts.

## Access, lore and source limits

PULL gray vine28514 at28603E or vines28516 at28605S clears the blocked way to
Keera28607. SEARCH/OPEN may still be needed. Reverse exits28607N/W reset5,
already unblocked. SHAKE thorny vine28524 at28706E operates the shop entrance;
PUSH28525 is the reverse control, whose exit starts unblocked5. Eight other
PULL stone/vine controls connect28563↔28793,28774↔28794,28775↔28777 and28806↔28818.
Their target states13 are CLOSED+SECRET+BLOCKED. item_switch clears forward
BLOCKED and only clears reverse BLOCKED when forward is not SECRET. These
are shared state changes; having traversed a route is not personal operation.

The maze has water, fall probabilities and underwater currents with existing
returns. The journal preserves hazards and describes rather than supplies
waterbreathing. Flower28515 in bamboo room28614 is actual loot outside contracts,
but its reset lacks an explicit height and obj_to_room initially usesz_cord0.
Flight-like prose alone cannot establish a flight prerequisite. Shimmering
tree/container/blossom stock, arrow props, corpse writing and travelers are
clues/treasure/lore rather than native accepted rescue or harvest events.

Fresco28798, Keera’s childhood rescue, ongoing Gartham slave raids, Ahgra and
Mystic messenger rumors outline potential future episodes. No local custom
producer commits a crystal reunion, sun/moon ritual, emperor change, escort or
released cohort. Teachers Choril/Parj/Harmank, four shops and imported epic node
remain separate systems. There is one local potential daily with mode2/cap1
source constraints; the mapping is not a daily replenishment guarantee.

Two missing reset references29555/38555 are documented, not silently substituted.
Each neberi also has valid hide stock. Replacement with28555 could add an extra
hide and change scarcity. Any actual stock, price, clue or access repair must
ship in a separate named fix/news commit with clear original/repaired evidence.

## Builder follow-ups and expanded capability plan

| ID | Source evidence and fair implementation plan |
| --- | --- |
| **ZSQ-PODS-PAYMENT-ADAPTERS** | Chamberlain Q39 is C50000→I28598. Podaling Q187 is6×I28554+I28555+C1000→I28552;Q199 is3×I28555+C500→I28553. Durable offering only accepts item input goals; coin-only and mixed legacy offerings refuse under active accounting. Keep all three services visible and guarded. Implement wallet-plus-distinct-root reservation, atomic consumption/payment, output ordinals, failure compensation, replay/restart and receipt recovery before opening them. Optional material rows do not assert wallet readiness. Four legacy shops have their own unchanged refusal and transaction work. |
| **ZSQ-PODS-PRICE-INTENT** | Spoken headdress price9platinum/necklace6platinum conflicts with actual fees1000/500copper (1platinum/half platinum). Key price50platinum matchesC50000. Establish intended fees with a builder; preserve encoded acceptance today. Any caption or fee repair needs a separate named fix/news commit and original/repaired tests. Do not treat a lower native fee as permission to change the economy. |
| **ZSQ-PODS-MISSING-RESET-PROTOTYPES** | G38555/cap1 atpods.zon390 and G29555/cap1 at1020 have no object declaration anywhere in tracked areas; corresponding mobiles are neberi28524 at28574/28765. Both also have a following valid G28555/cap47. renum_zone_table disables missing arg1. Investigate historical intended items or deletion before replacing with28555:doing so could add a second hide and alter scarcity. Document evidence, chosen stock/cap policy and original-fails/repaired-passes in a separate native fix/news commit. No repair is applied here. |
| **ZSQ-PODS-SHARED-CONTROLS** | Twelve type29 controls:28514/16 PULL gray vines;28524 SHAKE thorny vine at28706E;28525 PUSH reverse28707W;28534–41 PULL stone/vines on six other paired routes. Raw/D13 decodes CLOSED+SECRET+BLOCKED,5 CLOSED+SECRET. item_switch clears forward BLOCKED only for a SECRET target; SEARCH/OPEN remain separate, reciprocal controls may be needed. Shop reverse and Keera reverse start unblocked. Define successful actor/exit/instance/version evidence only if builders want personal operations; preserve shared/already-open routes and alternate returns. |
| **ZSQ-PODS-MATERIAL-CUSTODY** | Exact six feathers and three hides require distinct loose roots; CARVE emits prototype8, never28554/55. Feather G stock belongs to bird28537/cap25; hide G stock to28523/24/25/26 and one28537/cap47. P28554/55 targets a globally matching woven sack28593. F switches actual E/G receiver:elf28600 wears28592,elf28602 wears28597;dwarf28591 boots28583;goblin28592 shield28584;human28593 sleeves28586;goblin28596 belt28588;gartham28534, rather than chamberlain, wears imported-looking blade28580. Qualify root/child UID ownership, removal, bundle identity and supplied-versus-source provenance before adding personal acquisition claims. |
| **ZSQ-PODS-KEY-CONTAINER** | Mummified dwarf28575/O28703 has values200/15/28598/20:CLOSEABLE+deprecated HARDPICK+CLOSED+LOCKED, CLOSED test applies, PICKPROOF16 absent. P28576 is the skull tome, not shard28579. Tongue28598/key type18/value1=100 breaks on successful unlock; giver also carries native key stock. Establish purchase versus custody, pick/loan/already-open alternatives, key break/reset/reconnect and subtree handling before requiring personal purchase. Preserve independent shard receipt and current container flags. |
| **ZSQ-PODS-SOURCE-RENEWAL** | Mode2, cap1 shard E16 on lurking duergar28587/M28842, actual stock caps/chances and active-accounting reset issuance refusal remain. Giver remains after hand-in. No guaranteed daily stock or free renewal follows from map inclusion. Qualify admitted creation/corpse transfer/current versus historical proof, reset generation and pending/replayed reward recovery before source or daily availability promises. |
| **ZSQ-PODS-NARRATIVE-EPISODES** | Keera’s rescued childhood, Gartham raids, Knarlash’s fortress hopes, workshop warnings, stranded travelers and fresco28798 crystal/Mystic/Skexis reunion are prose/dialogue. No local custom producer or native completion represents rescue, escort, restored crystal, ruler change, captive cohort or moon/sun ritual. Builders must specify actual actors, goals, conditions, terminal states, group/reset policy and admitted accounting events before enabling those stories. The chamberlain shard response is accepted evidence for his request only. |
| **ZSQ-PODS-SCENERY-ALTITUDE** | Delicate flower28515/O28614 and shimmering tree/container/blossom stock are real loot/props outside local contracts. The flower’s prose says high bamboo; the reset carries no explicit height, obj_to_room initially puts objects atz_cord0. A prose lead alone cannot require personal flight or harvest. Choose intended height/travel/source and successful event semantics before integrating a flight/plant episode; retain existing movement/water hazards. Arrow props28570/71 and carcass/skeleton writing are clues, not counted rescues. |
| **ZSQ-PODS-FOREIGN-OWNERSHIP** | Humble hermit97901/M97907 in active surfacemini owns QA33:I28552+I28553→I97903 granular-potion recipe. Both paid outputs are exact distinct proofs; supplied matching items fit without local craft receipts. Ahgra/Mystics/castle dialogue is a wider Pharr lead without another28579 input contract. Keep foreign completion/discovery separate and preserve consumptive item competition. Imported epic358 has its own guarded TOUCH/node semantics; Choril28580,Parj28581,Harmank28588 are teachers, not hidden local quest producers. |


## Validation and checkpoint

Focused source/schema assertions and C++ journeys cover one story/three services,
six-feather/three-hide counts, loose versus worn/wrong items, supplied matching
proof without source history, reward possession versus receipts, independent
shard/key outcomes, replay/cold recovery, foreign discovery isolation and raw4
receipts→authored1 story compatibility while retaining all four historical receipts.
Synthetic settled receipts test projection; they do not execute wallet or source
admission. Full production regression, all115 Python/C++ journal journeys,
maintained server build, changed/staged format and preservation are required
before publication. Played wallet/reset/source/access qualification remains pending.

Catalog115 journals/1568 achievements/1438 potential dailies/2192 rows; native2668
definitions/fingerprint/revision2/registry and earlier114 mappings unchanged.
Roadmap98/220 source-comprehensive,122 pending; The Citadel (`citadel`) next.
No native repair or operational activation is applied. Full roadmap goal active.

# Ship Yards: comprehensive source story map

Priority 30, source area `shipy`, canonical zone 431. Source review is complete;
active-world qualification remains pending. Discovery, encounters, journals,
achievements and new daily eligibility require active, ready economic accounting.
This source map does not activate accounting or change native world content.

The [revision-one journal](../../../areas/story/shipy.story.json) covers all 26
native exchanges with nineteen independent outcomes and six support services.
Grimashk's two differently priced crate kinds are alternatives within one
recovery request. Chundel's separate delivery remains independent. Thirty-two
contacts cover all twenty addressed response families, all exchange recipients
and useful source roles. Thirty-four optional checks explain current supplies
and Pol's earlier briefing without imposing new prerequisites. Nineteen outcomes
are potential daily candidates under reset mode two, subject to actual source,
recipient, telemetry and accounting qualification. Raw 26-achievement/20-daily
projection becomes nineteen/nineteen; every definition and receipt stays intact.

## Evidence and review boundary

Read all 124 [native blocks](../../../areas/qst/shipy.qst): four Q, 22 QA,
82 M and sixteen MA. Twenty M/MA are addressed families; 78 are ambient.
Read every one of the 229 [rooms](../../../areas/wld/shipy.wld), 102
[mobiles](../../../areas/mob/shipy.mob), 46 [objects](../../../areas/obj/shipy.obj),
seven [shops](../../../areas/shp/shipy.shp) and 719
[reset commands](../../../areas/zon/shipy.zon), grouped into 265 families.
Resets contain 364 M, 126 G, 77 E, seventy D, 68 O and fourteen F; no P.
All reserved reset fields are zero. Canonical bounds are 43052–43328, while
actual local rooms are 43100–43328. All local reset targets are loaded.

Eight literal assignments are the two money changers Mazi/Tazi and the three
shipwright/three crew rooms in [specs.assign.c](../../../src/specs/specs.assign.c#L203).
Reviewed the entire retired [money-changer procedure](../../../src/economy/currency_exchange_proc.c),
[shipwright dispatch](../../../src/ships/ship_shop.c#L2960),
[hull callback](../../../src/ships/ship_shop.c#L146),
[crew procedure](../../../src/ships/ship_shop.c#L3252) and local catalog rows.
Shared reachable ship transaction helpers were comprehensively reviewed for
[Quietus Quay](QUIETUS_QUAY.md); checked this area's callers and relevant payment,
mutation and save boundaries against that unchanged implementation. Its shared
settlement finding applies here without making every foreign port comprehensive.

Computed-property review finds three ordinary inn rooms, 43249/43272/43312,
no local ACT_TEACHER mobile, no ROOM_ARENA, no positive local exit key, no literal
local object special and no local `_proclib_` script. A monk class or descriptions
about instruction do not supply the teacher property. Optional `areas/world.trg`
is absent in this checkout; deployment scripts require separate review.

Read the full fishing admission/event/grant path, generic teleport dispatch,
movement/locks, bootstrap exit removal, native offering/reward continuation,
reset parent/cap/slot and ordinary shop supply rules. Bounded foreign review
includes all 28 accepted item kinds and their loaded producer/reset occurrences,
the six primary potion suppliers and their rooms/shop entries, map tinker,
totem shaman, ring container/Boneheap, tundra rations vendor, Werrun storage room,
wemic source, four captive creature alternatives and Harrow's pike producer.
Read all 22 remaining foreign support prototypes referenced by local resets,
the Shaledrieth moonwell target, loaded incoming/outgoing boundary rooms and
all dispersal destinations. This does not claim those foreign zones are newly
comprehensive or that live journeys have been played.

## Independent stories and exact native terms

This order explains useful progression, without imposing a mandatory campaign.
QA/MA's A broadcasts response prose; it does not encode all-stage completion,
allegiance or a transformation. Supplied exact final proofs remain valid.
Cash below is copper; declared XP remains subject to native actor/group caps.

| Family | Exact accepted terms | Outcome and interpretation |
| --- | --- | --- |
| [Pol's briefing](../../../areas/qst/shipy.qst#L17) | 300,000 cash → same 300,000 cash + note 43136; stays | Support service, currently guarded. No retained deposit, purchased pole or mandatory final-note predicate. |
| [Pol's lure materials](../../../areas/qst/shipy.qst#L38) | Five shells 43137 + five fire glands 43138 → 300,000 cash + 125,000 XP; retires | Ten exact material objects, within durable offering bound fourteen. Earlier receipt and current note are optional; no Fishfetcher output. |
| [Voshen's kitchen](../../../areas/qst/shipy.qst#L115) | Six pike 318 → 50,000 cash + 40,000 XP; retires | Six distinct fish. No freshness/personal-catch predicate or promised fillet output. |
| [Crendethyl's stein](../../../areas/qst/shipy.qst#L149) | Stein 43111 → 24,000 cash + 166,000 XP; retires | Exact engraved object, supplied proof accepted. Source is another port's bartender. |
| [Clam merchant](../../../areas/qst/shipy.qst#L198) | 5,000 cash → clam 334; retires | Supporting paid supplier; no achievement/daily unit. |
| [Gringash's map](../../../areas/qst/shipy.qst#L233) | Map 49179 → 65,656 cash + 81,000 XP; retires | Independent commission. Bar-opening/Kimordril language has no world-state terminal. |
| [Krintis's research](../../../areas/qst/shipy.qst#L260) | Three venom sacs 43139 + three teeth 43140 → 210,000 cash + 100,000 XP; retires | Six exact reagents; no finished poison output. |
| [Gelden's study](../../../areas/qst/shipy.qst#L304) | Totem 9440 → 150,000 cash + 65,000 XP; retires | Sarmiz obsidian-and-steel proof. Hawk departure is prose, not a new transformed entity. |
| [Chundel's recovery](../../../areas/qst/shipy.qst#L334) | Normal crate 43101 → 200 cash; stays | Independent repeatable port request. Grimashk's receipt cannot substitute. |
| [Elthindeal's ale](../../../areas/qst/shipy.qst#L367) | Cask 43115 → 45,000 cash + 55,000 XP; retires | Cross-port exact cask delivery; no mandatory tavern history. |
| [Trismerk's trophies](../../../areas/qst/shipy.qst#L435) | Five jade katanas 43141 → 135,000 cash + 80,000 XP; retires | Delivery does not prove five personal kills or weakened Jade defenses. |
| [Austugus's research](../../../areas/qst/shipy.qst#L467) | Five hydralisk glands 43142 → 240,000 cash + 110,000 XP; retires | Proposed treatment stays lore; no antidote/cure output. |
| [Pike merchant](../../../areas/qst/shipy.qst#L487) | 5,000 cash → pike 318; retires | Supporting paid supplier, separate from Voshen's six-item terminal. |
| [Martinek's kitchen](../../../areas/qst/shipy.qst#L511) | Six clams 334 → 50,000 cash + 45,000 XP; retires | Exact supply delivery; no personal fishing or restaurant restoration state. |
| [Shaenae's ring](../../../areas/qst/shipy.qst#L559) | Ring 94307 → 200,000 cash + 155,000 XP; retires | Foreign nested proof; moonwell departure is prose rather than a required player journey. |
| [Ayden's reagents](../../../areas/qst/shipy.qst#L621) | Three fire glands 43138 + three venom sacs 43139 → 160,000 cash + 90,000 XP; retires | Separate horde commission; no poison/arsenal output. |
| [Kruth'Urgur's map](../../../areas/qst/shipy.qst#L662) | Map 49179 → 35,000 cash + 35,000 XP; retires | Separate payment/recipient from Gringash; no actual siege or allegiance state. |
| [Vrash's prototypes](../../../areas/qst/shipy.qst#L692) | Three teeth 43140 + three wicked maces 93501 → 245,000 cash + 90,000 XP; retires | Six exact proofs; no finished spiked mace or army upgrade. |
| [Cairme's supplies](../../../areas/qst/shipy.qst#L730) | Four shivs 43143 + rations 13723 → 160,000 cash + 105,000 XP; retires | Four is the implemented count despite a six-shiv sentence. Specific merchant purchase is not enforced. |
| [Grimashk's recovery](../../../areas/qst/shipy.qst#L761) | Normal crate 43101 → 250 cash; old reinforced 43125 → 845 cash; stays | Two exact kind/price alternatives, one recovery story. Underlying receipts remain distinct. |
| [Bronak's chimera](../../../areas/qst/shipy.qst#L789) | Firwood bear 108 + 50,000 cash → totem 43130; stays | Guarded paid equipment service. |
| [Bronak's spirit](../../../areas/qst/shipy.qst#L794) | Quartz 111 + 50,000 cash → totem 43129; stays | Separate guarded material/output identity. |
| [Bronak's elemental](../../../areas/qst/shipy.qst#L799) | Obsidian figurine 109 + 50,000 cash → totem 43128; stays | Different from Gelden's totem 9440. |
| [Frull's cigars](../../../areas/qst/shipy.qst#L822) | Box 38314 → 66,666 cash; retires | No declared XP, voyage, theft or shop-purchase predicate. |
| [Bestile's potions](../../../areas/qst/shipy.qst#L882) | One each 2800/9428/11562/40469/66723/93914 → 666,666 cash + 100,000 XP + mixed potion 43144; retires | Six separate exact kinds. No mandatory six-shop tour, consumption stage or Hulk mobile transformation. |

Pol's two exchanges explain refunded cash followed by another cash reward; they
do not create a player-bound retained-investment entitlement. His shop separately
stocks ordinary fishing pole 66709. The briefing note's text does not add an
accepted ingredient. The three cash-only offerings have `No repeatable item
offering`; all three Bronak recipes have `Unsupported durable offering`. They
stay unavailable with active accounting until compatible paid settlement exists.
Their recipes must never become free outputs through journal classification.

Native item-only deliveries can use the existing durable reward continuation.
The [XP path](../../../src/world/quest.c#L803) freezes delivering actor and
companion amounts with their different tenth-level/full-level caps, as detailed
in the [Verspin review](VERSPIN.md). Preserve admitted identities/amounts across
recovery and confirm builder intent before changing that policy. Group XP does
not grant personal source, learned topic or full-story participation evidence.

## Source stock, slots, caps and competing consumers

| Proof | Actual reviewed source | Qualification consequence |
| --- | --- | --- |
| Normal crate 43101 | Local ground resets across dock/town locations, shared cap 42; additional Alatorin stock | Ground acquisition and delivery are separate evidence. Chundel and Grimashk pay differently. |
| Reinforced crate 43125 | Two O declarations at Cairme cellar 43234, shared cap two; additional Twin Keeps/Alatorin placements | One valid alternative for Grimashk, without changing normal-crate payout. |
| Stein 43111 | E weapon slot sixteen on huge bartender 43120 at 43249, cap one | Despite its glass/ale prose it is an equipped weapon. Carry the exact recovered/supplied object for turn-in. |
| Ale cask 43115 | O at Gringash cellar 43233, cap one | Competes with any ordinary consumption/destruction; no alternate drink identity. |
| Shell 43137 | Fifteen G on turtles 43192 at dispersal 43322, cap fifteen | Skulldrach marsh destinations; five needed by Pol. Captive turtle 47106 at 47182 is another source. |
| Fire gland 43138 | Sixteen G + three E hold-slot eighteen on pyrolisks 43193 at 43323, cap nineteen | Three held glands are not carried stock. Loaded lava-field destinations plus one missing branch; Pol/Ayden compete. Captive 47109 at 47179 also carries glands. |
| Venom sac 43139 | Twenty G on vipers 43194 at 43324, cap twenty | Three Calimshan destinations and one ocean destination. Krintis/Ayden compete. |
| Panther tooth 43140 | Twenty-two E weapon-slot sixteen on panthers 43195 at 43325, cap twenty-two | IceCrag forest/grassland destinations; Krintis/Vrash compete. Captive 47105 at 47122 is another source. |
| Jade katana 43141 | Eight E weapon-slot sixteen on samurai 43196 at 43326, cap eight | One loaded Jade-fields destination. Five simultaneous exact katanas required; other swords cannot substitute. |
| Hydralisk gland 43142 | Seventeen G on hydralisks 43197 at 43327, cap seventeen | DarkPeak hills/valley destinations. Five needed by Austugus; captive 47110 at 47181 also carries glands. |
| Bandit shiv 43143 | Eleven G on bandits 43198 at 43328, cap eleven | Mainland pasture/light forest destinations; four needed by Cairme. |
| Map 49179 | G cap 999 on tinker 49065 at 49265 plus shop stock in [desert](../../../areas/shp/desert.shp) | Different port receipts own Gringash/Kruth'Urgur outcomes; no personal sailing/shop requirement. |
| Totem 9440 | E hold eighteen, cap one, on half-orc shaman 9410 at 9482 in [sarmiz](../../../areas/zon/sarmiz.zon) | Distinct from all Bronak outputs. Source shaman can roam. |
| Ring 94307 | P cap one inside bone pile 94304 following its O at Boneheap 94324 in [herders](../../../areas/zon/herders.zon#L542) | Take ring out of container. Other pile 94610 has a stud; container kind alone does not locate the ring. |
| Wicked mace 93501 | Six E weapon-slot sixteen on wemic warrior 93501 at several village paths in [wemic](../../../areas/zon/wemic.zon#L15), cap six | Three simultaneous maces plus three teeth; same numeric mobile/object VNUM belongs to distinct namespaces. |
| Rations 13723 | G cap one + shop stock on halfbreed 13719 at 13870 in [tundra](../../../areas/shp/tundra.shp); other foreign stock | Local port rations 364 are a different kind. Vendor suggested by lore, not mandatory acquisition history. |
| Cigars 38314 | O cap one at Werrun storage room 38338; rare Alatorin placement | Ordinary closed unkeyed storage door; source, theft and voyage not inferred from delivery. |
| Six potion kinds | Cyan on Bloodstone mystic 74203/74836; black on Severne 9415/9613; steel grey on Chukth 11538/11618; purple on Flakun 40462/40636; sparkling on Tirila 66725/66828; brown on local troll 43179/43310 | All six kinds, one each; additional foreign stock exists. Brown has a local route despite Shadowclave clue. Shop/gift supply is valid without six personal vendor receipts. |
| Bear 108, figurine 109, quartz 111 | G cap 999 + shop stock on local troll 43179 at 43310 | Exact separate Bronak materials; output possession is not proof of the paid recipe. |

All local reset chances are 100. A cap is ordinary reset stock policy, not a
guarantee of simultaneous currently available accepted objects. Review global
caps, historical stock, actor eligibility, durability, appearances, actual live
dispersal and legitimate alternate supplies before enabling personal-source
objectives or describing a reliable daily hunt. F declarations establish actual
followers rather than loose objects; none adds a delivery prerequisite.

Seven shops serve Safi, Pol, Jad, Fatira, Mahii, Ponit and the troll. They stock
weapons, fishing poles, boats, bags, drinks, rations and magic supplies. Monetary
pouches, counters/signs and props are support world content, not new quest
receipts. Stock includes foreign prototypes rather than only local objects.

### Fishing and publication

[get_pole/do_fish](../../../src/economy/tradeskill.c#L562) require an alive player,
fishing skill, an eligible pole in top-level carrying, suitable stance, no
existing fishing event, no disguise and an `IS_WATER_ROOM` destination. Worn or
nested poles are not found by this helper. Local ocean-sector water qualifies;
the normal pole 66709 and local pole 43120 are recognized alongside other kinds.
The event rechecks presence, connection, combat, posture, disguise, casting,
immobility, pole and water. Its random twelve-kind table includes pike 318 and
clam 334. Fish decay is disabled, so native freshness prose is not a time gate.

The event currently publishes the catch, changes fishing affects/XP and may
damage the pole before calling
[grant_tradeskill_item](../../../src/economy/tradeskill.c#L61), whose result is
ignored. Grant admission can fail and destroy the attempted object. This is a
source ordering gap already covered by the shared fishing settlement plan;
it does not prove a live lost catch. Freeze selected fish/source/time/effects,
bind accepted ownership issuance and recover once before publishing catch
success or personal-source credit. Preserve the native draw, skill, exhaustion
and diminishing-XP policy. Supplied-fish kitchen delivery remains independent
of any future personal-catching objective.

## Actual access, actors and services

- Loaded land approaches are Canderthal 43100 south ↔ 545743 highway,
  Fenaline 43180 south ↔ 529227 coastal lowlands and Thur'Gurax 43140 east ↔
  616947 bog-side gravel path. Local gates are closed/unkeyed. Each shared-area
  contact needs an encounter; discovery of one port does not reveal all givers.
- Canderthal ocean edges 43139 north/east, Fenaline 43219 north/west and
  Thur'Gurax 43179 north/west have loaded reciprocal Surface Realm destinations.
  These use sector twelve, `SECT_OCEAN`, with ordinary ocean/ship movement rules.
  A carried canoe, purchasing a hull or prose about sailing does not establish
  an accepted voyage or automatically satisfy every water-movement check.
- Four local outgoing targets are unloaded: 43120 west → 157323, 43225 west →
  154877, 43228 south → 154978 and pyrolisk dispersal 43323 east → 278216.
  [renum_world](../../../src/world/db.c#L1508) removes unresolved exits during
  bootstrap. Three legacy port links and one source branch therefore need
  coordinate/content review. No replacement coordinate is selected here.
- Viper dispersal 43324 west → 507725 is a loaded ocean room while other
  branches lead to Calimshan desert. Confirm intended ecology and actual mob
  movement before treating this as a safe/reliable sand-viper source.
- Barracks entry 43291 east → 43306 is reset locked, with key zero; reverse
  side is merely closed. No local positive key or literal access special adds
  a quest credential. Ordinary pick-lock eligibility and other valid access
  must be qualified. Vrash is upstairs at office 43309; he is not in Banner
  Square or the public magical garden.
- Cairme cellar 43234 west → 43320 is a hidden closed unkeyed passage. The
  staircase down reaches loaded foreign gambling room 23760 with a reciprocal
  return; its torch 23620 is ordinary support stock. The reinforced crates
  are in the cellar, before this foreign passage.
- [Statue 43117](../../../areas/obj/shipy.obj) at 43256 uses worship to reach
  43276; the glen has a south walking return. Altar 43132 at 43317 uses touch
  to reach burial room 43318, with south return. Moonwell 43145 at 43268 uses
  enter to reach loaded Shaledrieth 54245. All three use generic teleport objects
  with unlimited charges, subject to actual teleport admission. No journal
  worship, touch, burial, avatar or moonwell achievement is invented.
- Gelden starts at Canderthal northern pier 43108. Trismerk starts in the
  Wind's Breath dining chamber 43273. Austugus starts on Fenaline road 43265
  and can roam; Frull starts in Banner Square 43292 and can roam. The seafood
  suppliers have multiple roaming appearances. Initial reset rooms provide
  leads, not current live positions or permanent post-turn-in availability.
- Mazi/Tazi's LIST/EXCHANGE service is retired and directs players to Royal Bank.
  Bank counter text does not implement a new conversion milestone. The three
  inn properties provide accommodation services; local monk/instructor prose
  supplies no ACT_TEACHER lesson predicate. Ambient dances, drinking, market
  chatter, spirits and faction taunts are lore without accepted story outcomes.

### Shipwright and crew qualification

The three assigned shipwright rooms are 43118, 43158 and 43198. The actual
crew-room assignments are Canderthal bar 43220, Cairme tavern 43221 and Fenaline
clam house 43222. The first/third are good-side halls, the second evil-side,
according to explicit racewar admission rather than quest allegiance inferred
from roleplay. Hiring also checks ship ownership and catalog frags/skills.

Local catalog includes Sturdy Whalers at 43220 and Strongarms of Ghore at 43221 for
1,000,000 copper, Evermeet Coasters at 43222 for 2,000,000 and ninety frags;
chiefs include Gunner Cadet/Shipwright Tyro at 43220, Deck Cadet at 43222 and
Veteran Boatswain at 43221. Their actual catalog conditions stay authoritative.
These are ship-state selections, not collectible local crew quest objects.

The unchanged shared **ZSQ-SHIP-SERVICE-SETTLEMENT** finding applies: legacy
crew, repairs, reload, summon and other purchases ignore refused `SUB_MONEY`
before changing ship state; cargo/slot sales can remove assets before refused
`ADD_MONEY`. Rename acts before coin debit. Hull completion waits for epic
settlement but ignores the coin debit/refund result. Whole-ship sale refuses
immediately; its old direct mutation tail is unreachable.

First honor denied settlement before any asset/effect mutation; keep unsupported
paid paths refused until coordinated wallet/epic/ship revision and recoverable
save/refund exist. Preserve owner, selected hull/slot/crew, eligibility, frozen
price, port, maintenance and operation identity. Then test denial, changed ship,
departure, concurrent selection, commit/abort/replay/restart and actual summon
arrival. There is no new ship-buying, hiring or voyage achievement in this map.

## Balanced content and capability repair plans

| Finding | Evidence | Plan and qualification |
| --- | --- | --- |
| ZSQ-SHIPY-POL | Advertised Fishfetcher is not an output; deposit is refunded; note requested in prose is absent from final ingredients. | Builder selects corrected offer/note text or a deliberate compatible new recipe. Preserve current ten-material requirement, price/reward and valid supplied terminal; quantify any added pole/entitlement balance. Qualify earlier history separately and do not require unavailable paid briefing for the existing final recipe. |
| ZSQ-SHIPY-CONTENT | Cairme says four and six; Voshen promises an absent fillet; study/research/military success describes hawk, moonwell, poison, antidote, siege, arsenal and Hulk without implemented state. Map geography and copied sails/fishing prose are inconsistent. | Prefer clue/description correction that accurately describes accepted outcomes. If builders intend tangible products or campaign states, author explicit prototypes, costs, selected effects, episode ownership and terminal semantics before credit. Review reward power, frequency and competing demand; never infer rewards or transformations from broadcast text. |
| ZSQ-SHIPY-ACCESS / SOURCE | Four unresolved exits removed at bootstrap; one loaded viper branch is ocean. Proofs use dispersal, equipment slots, caps and a nested foreign ring. Barracks lock has key zero. | Verify intended coordinates/regions, path reciprocity, actual mob routing and legitimate alternatives. Select bounded topology/ecology/lock correction with builders. Qualify active issuance, reset generations, current roaming/retirement, exact sources/counts/kinds, carrying and supplied proofs. Do not guess replacement exits or impose an invented key/garden/ship gate. |
| ZSQ-SHIPY-PAID | Refunded briefing, two seafood purchases and three mixed totem recipes have guarded coin settlement. | Extend existing paid-service adapter for exact fee/refund/material allocation and selected output. Commit wallet/material retirement/output/receipt together; test denial, actor/item changes, replay and recovery. Native supplied-proof stories remain available without these optional purchases. |
| ZSQ-SHIPY-FISH | Catch messages, pole effects and XP precede ownership grant; grant failure is ignored; freshness timer is disabled. | Extend existing fishing continuation with frozen draw/source/time and recoverable accepted issuance/effects. Publish once after accepted ownership; test denied grant, interrupted fishing, pole state, actor departure/restart, supplied fish and no reroll. Define any new freshness or personal-catch objective deliberately. |
| ZSQ-SHIPY-SHIP / CREW | Shared paid ship helpers/crew ignore denied cash, asset sales ignore denied credits, hull completion coordinates epics without coordinated coins. | Apply the existing shared settlement plan here, beginning with denial-before-mutation refusal. Qualify stable owner/revision, exact selection/cost, port/race/frags/skills, timers, durable save/refund and replay. Keep services out of achievements until committed evidence exists. |
| ZSQ-SHIPY-SEMANTICS / XP | Current possession and terminal receipts do not prove personal acquisition, fishing, sailing, allegiance, six vendors or a full quest chain. Frozen XP uses asymmetric actor/companion caps. | Use accepted acquisition/transfer provenance and explicitly authored episodes for deeper objectives. Keep gifts valid for native delivery; aggregate crate alternatives while retaining prices/receipts. Confirm XP intent before deliberate balance changes and preserve admitted continuations. |

## Verification and remaining qualification

The reproducible [review index](../../reference/zone-story-audits/shipy.md)
records every exchange, addressed response, literal assignment and reset family.
Focused fixtures protect all bindings/topics, exact six-fish/five-material/four-
shiv quantities, six potion kinds, optional Pol history/note, distinct maps and
totems, separate Chundel/Grimashk ownership, alternative crate receipts, service
exclusion, source parents/slots and reload persistence. The maintained server
build, format, whitespace, catalog/invariant and local/source-link checks
complement those tests.

Actual active-accounting source/carrying/turn-in, paid, ship, fishing, locked,
ocean, teleport, rare foreign stock and reset/retirement journeys remain pending.
No migrations, accounting activation, DB/server operation or merge occurred.
Next source-comprehensive work is Ultarium, followed by the Surface Realm.

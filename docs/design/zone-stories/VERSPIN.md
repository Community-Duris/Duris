# Verspin: comprehensive source story map

Priority 29, source area `verspin`, canonical zone 281. Source review is complete;
active-world qualification remains pending. Discovery, encounters, journals,
achievements and new daily eligibility require active, ready economic accounting.
This mapping does not activate accounting or change native world data.

The [revision 1 journal](../../../areas/story/verspin.story.json) covers all twelve
native exchanges with six named stories and six supporting services. Eighteen
contacts cover all nine addressed M families and useful source/teacher/service
roles. Eighteen optional checks explain exact current supplies and Ramous's
producer receipt. Six story outcomes are potential daily candidates under reset
mode two, subject to source, recipient, telemetry and accounting qualification.
The raw projection's twelve achievements/seven potential dailies become six/six;
every native definition and receipt remains intact.

## Evidence and review boundary

Reviewed all 52 [native blocks](../../../areas/qst/verspin.qst): eight Q, four QA,
nine addressed M families and 31 ambient M responses. Read every one of the 200
[rooms](../../../areas/wld/verspin.wld), 79 [mobiles](../../../areas/mob/verspin.mob),
57 [objects](../../../areas/obj/verspin.obj), five [shops](../../../areas/shp/verspin.shp)
and 389 [reset commands](../../../areas/zon/verspin.zon), grouped into 206 families.
Resets contain 233 M, 56 G, 52 D, twenty E, fourteen O, nine F and five P. Canonical
bounds are 27471–28299; actual local rooms are 28100–28299. All reserved reset
fields are zero. No local reset target or loaded local exit target is missing.

The two literal local room assignments are [Shinjin's stat shop](../../../src/specs/specs.assign.c#L2332)
and [crew shop](../../../src/specs/specs.assign.c#L2410). Read their entire relevant
procedures in [epic.c](../../../src/world/epic.c#L1377) and
[ship_shop.c](../../../src/ships/ship_shop.c#L3252), all nine permanent-stat spells,
money admission, interpreter routing and pending-operation fences. Bootstrap
also assigns the ordinary teacher to Vespus, Brachik and Shinjin through their
mobile property and the inn service through room 28283's property. No local
literal mobile/object special or `_proclib_` object script is present; optional
`areas/world.trg` is absent in this checkout. Deployment scripts need separate
review.

Read relevant native offering/reward continuation and XP policy, object/mobile
bootstrap, reset parent/cap rules, shop stock, command/room dispatch, movement,
keys, deep water, hidden doors, falling and packed weapon effects. Shinjin's
hidden weapons are combat effects, not enlightenment milestones: 28154 encodes
Full Heal/Full Harm, while 28155 encodes Magma Burst/Incendiary Cloud. Its comment
describes Immolate instead of the actual first effect; metadata repair should
preserve selected combat power unless builders deliberately change it.

Bounded foreign review read the complete jewel thief and its room, sinister
apprentice and tower room, corruption sigil, three ore rewards, and thirteen
foreign reset supplies. Reviewed both foreign incoming room records and the
ordinary outgoing forest connection. These are source/access checks, without
claims that Cheltenham, Bloodstone, the Surface Realm or Winterhaven are newly
comprehensive or that any live journey was played.

## Named progression and exact accepted terms

The narrative order below explains useful routes. Native completion does not
require every dialogue keyword, personal kills, prior producer history or a full
campaign. Supplied final proof is valid. QA's A means broadcast success prose,
not an all-stage quest or a class/race restriction.

| Family | Exact native terms | Meaning and journal treatment |
| --- | --- | --- |
| [Tottan's five totems](../../../areas/qst/verspin.qst#L20) | Five separate totems 28144 → silk pants 28145 and declared 65,000 XP | Named circus-rivalry delivery. Five shaman defeats and restored circus attendance are not separate predicates. Tottan remains. |
| [Ramous's apple service](../../../areas/qst/verspin.qst#L40) | Apple 28112 → steak bone 28113 | Supporting producer, excluded from achievements/dailies. Its receipt explains one lion route without being required. |
| [The golden lion's collar](../../../areas/qst/verspin.qst#L49) | Bone 28113 → collar 28114; lion 28128 retires | Independent named outcome. Supplied bone works without Ramous history; no escort or animal-release world state follows. |
| [Lozin's black gladius](../../../areas/qst/verspin.qst#L78) | Apple 28112 + 10,000 copper → gladius 28128 | Paid equipment service. |
| [Lozin's Verspin hammer](../../../areas/qst/verspin.qst#L83) | Apple 28112 + armor of pins 28116 + 10,000 copper → hammer 28126 | Paid equipment service with two distinct materials. |
| [Lozin's red steel armor](../../../areas/qst/verspin.qst#L89) | Black armor 28107 + 20,000 copper → red armor 28127 | Paid equipment service, separate from buying the same output. |
| [Lozin's hunting knife](../../../areas/qst/verspin.qst#L94) | Two red amulets 28124 + two black 28125 + 5,000 copper → knife 28129 | Paid equipment service; exact quantities/colors stay separate. |
| [Lozin's hunting shield](../../../areas/qst/verspin.qst#L102) | Four red amulets 28124 + 6,000 copper → shield 28130 | Paid equipment service with an ordinary three-red-cap feasibility finding. |
| [Vulm's stolen amethyst](../../../areas/qst/verspin.qst#L120) | Amethyst 28146 → distinct ore pieces 223, 224, 225 + declared 50,000 XP | Named recovery delivery. No separate shop-restoration state or personal thief defeat. |
| [The shrine's five symbols](../../../areas/qst/verspin.qst#L165) | Five marble symbols 28138 → glasses 28139 + declared 50,000 XP; holyman 28147 retires | Named delivery; suggested cleric persuasion has no accepted transfer/dialogue predicate. |
| [Transo's three amulets](../../../areas/qst/verspin.qst#L243) | One green 28123 + one red 28124 + one black 28125 → belt 28141 + declared 70,000 XP | Named bargain. His plan is to sell stolen goods to Ramous, without a direct Ramous return or justice/reunion terminal. |
| [The monk's corruption sigil](../../../areas/qst/verspin.qst#L339) | One foreign sigil 74298 → flower necklace 28153 + declared 85,000 XP | Named Verspin receipt. Does not prove defeating all three apprentices, Maelborg or permanently cleansing Bloodstone. |

The five fees are ten, ten, twenty, five and six platinum, agreeing with the
implemented sign rows. All five mixed cash/material offerings are marked
`Unsupported durable offering` and remain refused with active accounting. Empty
Lozin success text does not remove the native equipment output or authorize a
free grant. Item-only XP outputs use existing reward continuations; native XP
declarations are subject to actual per-recipient caps.

### XP, group credit and policy qualification

The [native reward path](../../../src/world/quest.c#L557) caps the delivering
actor's XP at one tenth of next-level XP. Legacy companion handling restores the
declared amount before applying a full-next-level cap. The existing
[frozen award path](../../../src/world/quest.c#L803) preserves this asymmetry:
recipient zero uses the tenth-level cap, other frozen recipients the full-level
cap. It persists the amounts and recipient identities for recovery. Therefore
the declared 50,000–85,000 awards are not unconditional personal grants, and the
comment's one-notch description is not the complete implemented group policy.

Qualify eligibility, frozen recipient order/levels/amounts, departure, replay and
restart; confirm builder intent before changing either cap. A balance correction
must be explicit and compatible with admitted continuations. Group XP and story
credit are separate policies: neither a companion award nor a supplied proof
establishes personal source recovery, learned dialogue or all-stage completion.

## Sources, caps and competing consumers

| Item | Actual ordinary source | Consequence |
| --- | --- | --- |
| Apple 28112 | P in stall 28111 at 28129 and stall 28110 at 28133; shared cap two | Both are actual containers. Take the apple into top-level carrying for an offering. Food consumption and three recipients compete for existing apples. |
| Bone 28113 / collar 28114 | Ramous / lion Q outputs, with no reset stock | Earlier producer history is optional. Possessing a supplied collar cannot complete the lion receipt. |
| Totem 28144 | G on shaman 28104 at 28104, 28106, 28107, 28111 and 28116; shared cap five | All five exact objects must be delivered together. Shamans can roam; initial placement is a lead, not their live location or personal kill history. |
| Marble symbol 28138 | G on cleric 28103 at 28104, 28111, 28114, 28117 and 28136; shared cap five | The wandering cleric 28149 instead has insignia 28133. Similar titles/items cannot substitute. No local cleric response grants symbols through persuasion. |
| Green amulet 28123 | G on thief 28105 at 28111, loud shopper 28135 at 28132, boatman 28143 at 28169; shared cap three | One distinct Transo ingredient. |
| Red amulet 28124 | G on mime 28101 at 28101, noble 28108 at 28113, boatman 28143 at 28162; shared cap three | Competing Transo/knife/shield uses. The four-red shield recipe exceeds reviewed ordinary cap; qualify alternate/legacy/forced supplies before selecting a repair. |
| Black amulet 28125 | G on commoner 28121 at 28113, snobby shopper 28137 at 28130, shaman 28104 at 28116; cap 999 | Separate kind from green/red, shared by Transo and knife service. |
| Black armor 28107 | G on Brial 28134 at 28160, cap one; ordinary shop stock | Exact input to red-armor service. |
| Armor of pins 28116 | G on huge man 28136 at 28144, cap one; shop stock | Exact second hammer material. |
| Lozin outputs 28126–28130 | All five G/shop stock on Brial at 28160; hammer also E on guards | Shop or third-party acquisition cannot establish Lozin's paid recipe or the hunt story. |
| Amethyst 28146 | G cap one, chance 33, on jewel thief 37191 at 37481 in [mntcastl](../../../areas/zon/mntcastl.zon#L381) | Actual foreign source differs from Vulm's crabmen clue. Rare load and active issuance need qualification; no duplicate ordinary producer was found. |
| Sigil 74298 | Three M apprentices 74249 at 74917, followed by one G cap-one sigil declaration in [Bloodstone](../../../areas/zon/bs.zon#L2008) | Only the third declared apprentice is the G parent. One sigil matches the monk's native contract; do not manufacture three proof requirements. |
| Key 28142 | G on strong guard 28161 at 28214, cap one | Fits the first downward pit gate. |
| Key 28150 | G on elite guard 28171 at 28255, cap one | Fits the second eastern gate. Same generic key name, distinct exact source/lock identity. |

All local G sources above are chance 100 unless the foreign gem says otherwise.
The lion is an F mobile placement following Ramous at 28159, not a carried item.
Givers are sentinel at their declared starting rooms; retiring lion/holyman
availability after acceptance, reset and restart is separate qualification.

Five shops serve the dwarven peddler, Rammzi, Brial, huge man and human peddler.
Meals, canteens, bandages, identify devices, containers, boats and potions are
support supplies. The three inferior ore rewards are distinct foreign objects;
coins 90026/94336 in reset stock are currency representation, without a quest
receipt or proof of personal treasure recovery.

## Actual access, services and unfinished lore

- Ordinary gravel-highway room 531117 enters north to southern city room 28100; the
  city's south exit returns. Its brass door is closed but unkeyed. The loaded
  Winterhaven Incarnation dispersal edge 55614 southwest to 28109 is an
  additional foreign edge, not the normal visitor entrance.
- Circus 28146 east / 28147 west are secret closed, unkeyed backstage access.
  Continue east to 28149 and south to Tottan at 28150. No totem or earlier quest
  unlock is required. Asking `gnome` and `gnomes` shares one response family.
- Western/eastern spectator stairs reach 28198 from 28132 and 28268 from 28113.
  Transo at 28212 is on this level. Do not make the secured pit's two keys a
  bartender prerequisite: 28214 down → 28215 uses 28142, while 28255 east →
  28256 uses 28150. Reverse sides are unkeyed and merely closed. Guard taunts
  have ambient M responses without a separate custom movement barrier.
- The cave has an optional secret southeast link from medicine room 28251 to
  28257. Abandoned home 28260 down reaches a separate hidden shaft route with
  floorless sections 28263–28267. Their downward edges support ordinary
  falling; an atrium/inn room's copied F20 without a down edge is cleared by
  bootstrap and does not establish a falling inn. The spy's stilettos have no
  Bloodhawk purge or rescue terminal.
- Waterways 28161–28173 use deep-water sector movement. Leaf boat 28120 from
  the human peddler is one supported aid, including ordinary carried/equipped
  policy; native flying and other movement exceptions remain valid. Boatmen
  have ambient speech, without a custom ferry/boarding procedure. Qualify actual
  crossing separately from boat purchase, possession or conversation.
- No local room has `ROOM_ARENA`, including the named gladiator pit. Fighting,
  honor and blessings have ambient lore without contest-victory, safe arena
  death or accepted blessing evidence. The normal teachers answer eligible
  `ask ... level` guidance, without a durable lesson milestone. Room 28283's
  inn property provides ordinary accommodation services.

### Stat effects and crew hiring

[Shinjin's stat shop](../../../src/world/epic.c#L1377) lists nine virtual potions
priced by `base_stat^3 * 8` and limited to base stat below 95. The purchase calls
one of nine [permanent-stat spells](../../../src/magic/spell_permanent_stats.c),
which adjusts base/current stat and balances affects. It creates no potion object.
An explicit active-accounting guard already declines this purchase before debit
or effect. Preserve that guard, prices and cap until wallet, expected stat
revision, selected +1 effect, balancing/save, entitlement and publication commit
and recover together. Ordinary teacher guidance remains a separate function.

[Crew hiring](../../../src/ships/ship_shop.c#L3252) requires an owned eligible
ship, room/race catalog, fragments/skills and money. The Verspin hire room is
28197, a private-room prototype, and includes Tekan Madcaps at 5,000,000 copper
and 350 hiring frags. The procedure ignores `SUB_MONEY`'s result, then changes
crew/chief and queues the ship save. Active accounting makes
[`SUB_MONEY`](../../../src/core/utility.c#L3286) return failure. Interpreter
`CMD_HIRE` pending-transaction serialization is not a blanket active-accounting
admission guard, so source ordering exposes an unpaid-mutation risk. This is a
source finding, not a claim of an observed live incident. First reject a failed
payment before crew mutation; retain refusal until coordinated wallet/ship
revision/save settlement is available. Confirm the intended room placement with
builders before changing it. No journal achievement is attached to hiring.

## Balanced content and capability repair plans

| Finding | Current evidence | Plan and qualification |
| --- | --- | --- |
| ZSQ-VERSPIN-SIGN | Lozin's description directs players to sign 28122, but it has no ordinary reset producer. It advertises ten offers; only five native recipes exist. No reviewed local special handles the other offers. | Builder chooses placing/correcting the sign and retiring stale offers or authoring five deliberate compatible contracts/prototypes. Preserve implemented fees/rewards. Test actual reachability, stock, exact recipes and paid refusal; do not invent missing bindings from text. |
| ZSQ-VERSPIN-CAPACITY | Shield needs four reds, while all three ordinary red sources share cap three. | Reproduce fresh ordinary supply under active accounting and check legitimate alternate/legacy/forced sources. Select a bounded supply/cap change or revised recipe with builders; review rarity and competing Transo/knife demand. Never silently substitute colors or remove the fee. |
| ZSQ-VERSPIN-GEM | Vulm's crabmen geography differs from the actual jewel-thief source in Cheltenham. | Choose corrected clue or an intentional compatible source revision, preserving the gem's identity, rarity and reward. Verify rare-load availability, actual travel and supplied-proof acceptance. No invented gem-shop reopening. |
| ZSQ-VERSPIN-LORE | Cleric persuasion, circus attendance, amulet reunion, honor/blessing, spy removal and tower cleansing have no accepted state. Holyman uses copied clown description; hidden weapon comment misnames an effect. | Retain explanatory lore or author explicit outcomes/episodes before semantic credit. Correct misleading descriptions/metadata without changing selected combat power. Keep exact delivered proofs and retiring appearances independent of hoped-for campaign endings. |
| ZSQ-VERSPIN-SOURCE / ACCESS | Five totems/symbols, competing apple/amulet supply, one foreign sigil parent, rare gem, two exact pit keys, secret routes and deep water. | Qualify reset parent/slot/cap/generation, appearance/retirement and current location, source versus gifts, accepted movement and optional earlier receipts. Test supplied proofs, containers/worn items, wrong kind/count/key, already-open alternatives and restart. No access requirement on a valid supplied terminal or spectator route. |
| ZSQ-VERSPIN-PAID / STAT | Five mixed recipes are refused; nine guarded stat purchases have virtual effects rather than potion inventory. | Extend existing paid-service settlement with exact recipe allocation and expected stat revision/effect entitlement. Commit debit/material retirement/output or +1 stat/save before success. Test denial/abort/replay/restart and cap/price preservation; do not award a potion-acquisition objective for a virtual effect. |
| ZSQ-VERSPIN-CREW | Legacy hire ignores denied debit before crew/chief mutation and queued save. | Close the payment-result ordering gap, then qualify coordinated wallet/ship settlement, authorization/frags, concurrent hire/ship changes, denied payment and replay/recovery. Confirm room placement separately; avoid unsourced journal hiring achievements. |
| ZSQ-VERSPIN-XP | Actor and companions have different native caps; continuation freezes recipient identities/amounts. | Confirm intended balance before changing group policy. Qualify recipient ordering/eligibility and durable entitlements on departure/replay/restart. Do not equate group XP with personal acquisition, learned keywords or full-campaign proof. |

## Verification and next qualification

Focused regressions protect all twelve exact native bindings, six story/six
service projection, all addressed topics, eighteen optional checks, repeated
quantities/colors, producer-versus-terminal independence, source caps/parents,
rare foreign source and one-sigil ownership. The native file loader must accept
all journals and show optional preparation without blocking a supplied lion bone.
Native journeys cover no credit on viewing/output possession, exact count/color
checks, independent receipts, service exclusion and restart recovery.

Regenerate the [review index](../../reference/zone-story-audits/verspin.md), global
inventory and catalog together. Preserve all native definitions, revision,
fingerprint, registry and previous maps as parsed data. These static/source
fixtures do not certify live source, combat, paid, travel or stat/crew journeys.
Qualify those with active, ready accounting before enabling fresh daily exposure.
No accounting activation, migration or DB/server operation is part of this map.

The production-catalog regression and native C++20 story/file-loader regression
passed for all 52 maps, including the supplied-bone journey, exact counts/colors,
service exclusion, foreign ownership and all six independent receipt outcomes.
All 29 source indices reproduce. The maintained SQL server build completed
with current objects; formatting, whitespace and all 615 local/source-line
links passed. Native definitions, revision/fingerprint, registry and fifty-one
earlier parsed maps remain unchanged. These are source and fixture results;
active gameplay and service settlement qualification remains pending.

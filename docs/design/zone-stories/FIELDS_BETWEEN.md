# The Fields Between: comprehensive source map

Priority 77 of the original roadmap. **Source-comprehensive; played qualification
remains pending.** The new [journal](../../../areas/story/fields_between.story.json)
uses schema three, revision one: seven story outcomes, 22 contacts, all ten
addressed topic families and 81 aliases. Seventeen optional checks separate 16
present materials/access keys from one earlier receipt. All seven item-only
outcomes, including Timmy's retiring exchange, remain potential daily candidates.
Discovery keeps its own achievement. All new discovery, encounter, journal,
achievement and daily credit requires **active, ready accounting**; frozen reward
recovery remains separate.

**Actual native repair, separate commit:** [05eeca928](https://github.com/Community-Duris/Duris/commit/05eeca928),
`Fix Fields Between shaman quest rift pickup`. The small circular rift was
described as easy to pick up and required by the shaman, but had no TAKE flag and
weight 1,000,000. Exactly two prototype fields now make it takeable and weight
one. The floor source, visibility flags, unlimited ENTER command, destination,
return gateway, exact request and reward are retained. The
[focused regression](../../../tests/async/test_fields_between_portable_rift.py)
fails on the original data and passes on the repair. Normal pickup still needs
visibility and available carrying capacity; a played pickup/offer/settlement
journey remains unqualified. Existing saved instances are not rewritten.

**News-ready:** “The Fields Between's small circular rift can now be picked up
and delivered to Grog's shaman bodyguard for his quest.”

## Complete source closure

The active area is `fields_between`; zone 710 has registry bounds 71000–71153,
reset mode two and native header levels 60–120. These header levels do not assert
enforced admission or difficulty. Physical membership is all 153 rooms
71001–71153, 68 mobiles 71001–71068 and 33 objects 71001–71033.

- [All 17 native blocks](../../../areas/qst/fields_between.qst): ten M and seven
  Q, no QA. The empty section for ordinary mouse 71002 adds no request. All ten
  addressed families/81 aliases are preserved, including misspelled native
  keywords; the guide recommends working `food`, `book`, `son` and `timmy` aliases.
  Timmy alone has D retirement. No coin or XP input/reward occurs locally.
- [All 153 complete rooms](../../../areas/wld/fields_between.wld): 77 exact
  title/prose families, nine complete headers, 428 exact exits/428 numeric
  families and 16 exit-description/keyword families. No extra-description,
  room F/T block or unknown trailing field occurs. Every repeated membership,
  direction, flags, key and destination binding was reviewed.
- [All 68 mobiles](../../../areas/mob/fields_between.mob) and
  [33 complete objects](../../../areas/obj/fields_between.obj): full prose,
  flags, values, affects and extra descriptions. Five heads are distinct loaded
  quest items, rather than ordinary CARVE results. The monkey manuscript is
  actually type 37, worn as a shield; its literary name does not create a BOOK
  controller. Bananas and the seasoned body are type 13, rather than FOOD or
  CORPSE. The professor's narrated injury/chant does not run a new quest action.
- [All 286 resets](../../../areas/zon/fields_between.zon): D22/O6/M230/G14/E11/F3;
  273 exact and 273 parent-aware families. Followers remain attached to the
  preceding Grog M, including both ogre guards and the shaman. No P/R reset
  occurs. Mithril has a 33-percent reset chance and cap one; Timmy has a
  ten-percent chance and cap one. These are source conditions, not guaranteed
  live availability. No configured local shop, inn, teacher, literal special
  assignment, type-29 switch or local epic-teacher entry supplies another quest.
- Four type-25 objects use shared
  [teleport selection](../../../src/magic/spell_travel.c#L930), CMD_ENTER seven
  and unlimited charge -1: farmhouse 71001→71104, door 71002→71014,
  small rift 71030→71153 and gateway 71031→71001. All six ordinary exits from
  71153 loop to itself. The portable floor rift is not produced by killing
  dimensional-rift mobile 71067. The great chamber's described permanent rift
  has no placed matching portal. All 713 active portal prototypes were checked;
  only these four target a local room.
- Bounded foreign closure includes the complete advisor and depressed-troll
  follow-on recipes, their actors/rewards/rooms, all six boundary edges and
  full foreign neighbors. No active foreign reset loads a local actor/material.
  The imported rune-covered stone 359 and memory 55435 were read in full.
  Stone 359 binds [epic_stone](../../../src/specs/specs.assign.c#L1604), whose
  TOUCH checks and queued epic settlement remain a separate system. Winterhaven
  giver 55213 accepts memory 55435 for 55362, 1,000,000 copper and 55033;
  this is a foreign-owned item/coin reward path, without local
  journal credit. No new comprehensive foreign-zone pass is claimed.

## Exact progression stories

| Native receipt | Present materials and actual supply | Outcome and limits |
| --- | --- | --- |
| Grox 71036, Q10 | Orders 71007 from ruffian 71035 at bar 71136 AND prepared boy's body 71008 from cook 71052 at mess hall 71139 | Mask of luck 71020. Neither an ordinary corpse nor a personal earlier Grog receipt is required. |
| Brewer 71037, Q42 | Manuscript 71005 worn by monkey horde 71028 at study 71114 AND banana clump 71016 carried by octopus 71041 at kitchen 71119 | Fez of clarity 71024. Remove a worn manuscript before offering; ape 71062 and quills are not producers. The recipient is a troll despite the accepted caption saying ogre. |
| Grog 71038, Q95 | Elements head 71010 from 71046/71123; nature head 71011 from 71047/71122; creatures head 71012 from 71048/71125; space/time head 71013 from 71049/71126; great head 71014 from 71050/71128 | Crown 71017. All five distinct kinds together; five duplicate copies do not satisfy it. Captive mage/spellcasting narration adds no extra actor action or rescue objective. |
| Shaman 71040, Q125 | Small circular rift 71030, floor O at 71126, following source-specific pickup repair | Band of winds 71033. Offering consumes the portal; ENTER use beforehand is independent. No D, actual escape, teleport or campaign ending is implemented by this receipt. |
| Professor 71056, Q147 | Crunching box 71022 from torturer 71055/71132 AND dark mithril 71021 from weaponsmith 71051/71149, uncommon stock | Tipped boots 71023. Item-only narrative request, without a fee or shop. Timmy needs a separate bar. The laboratory's mithril prose supplies no loose bar. |
| Timmy 71065, Q190 | Dark mithril 71021; recipient may be present at farmhouse 71104 reached by ENTER | Letter 71027; D retires Timmy after acceptance. No actual human restoration or professor delivery is recorded. Daily candidacy does not guarantee a fresh mouse or bar. |
| Mother 71066, Q222 | Letter 71027 from Timmy or a supplied exact copy; recipient loads at 71152 and can wander down to Scorched Valley stable 71265 | Sorrow veil 71028; no D despite promised departure. Optional Timmy history never replaces current letter custody or proves reunion. Canonical ownership stays Fields Between across the boundary. |

Current custody is advisory; accepted exact receipt is completion. All supply/key
and earlier-history checks remain optional. Publicly opened gates and supplied
materials do not gain new forced personal prerequisites. The native selector
requires loose selected roots; worn, nested, spent and handed-away copies remain
missing. No per-keyword achievement, inferred kill, manufactured item, or narrated
rescue is credited.

## Access and neighboring requests

The Surface foothills 629085 and entrance 71001 have a reciprocal north/south
connection. The foliage passage 71045 east/71046 west resets secret/closed D5;
SEARCH and OPEN lead east into the raider woods. The tower entrance 71107
south/71108 north uses raw type two (pickable), D2 locked and thorny key 71003
carried by walrus-giant 71027. The great chamber 71127 south/71128 north uses
raw type three (pickproof), D2 locked and spiral key 71026 carried by vampire
71044. Both keys have value[1] zero and survive ordinary successful unlocking.
Study, laboratory, kitchen, other wildmage rooms and Grog's hut use visible
closed D1. High raw bits and reset state must be decoded separately.

The mother has neither SENTINEL nor STAY_ZONE; the stable's sector and flags
allow the shared wandering route. Her loading room has no player return edge.
Scorched Valley seeker 71256 loads at 71331 and can enter camp corner 71140;
his Tallin-blood exchange for four rings remains Scorched Valley's. Juiblex
Blaxor 87600 and Marvin 87611 load at 87671, whose actual downward destination
is 71106 despite its stale administrative caption saying room 101. Blaxor has
no addressed/Q request here. Marvin's leash receipt remains Juiblex-owned and
retires him. Winterhaven helper 55633 northeast enters great chamber 71128;
it is not a new local portal or quest.

Grog's own head 71009 supports the Scorched Valley advisor's three-head request
with commander head 71227 and former dwarven-lord head 28980 for Devastation
71226. The brewer's fez 71024 supports depressed troll 87518 in Juiblex's
barracks 87588 for true-clarity fez 87594. These are optional continuations,
without another Fields Between achievement. The memory, epic stone, coil,
party, captive wildmages and scenery remain supporting systems/context.

## Gaps, fair repair proposals and qualification

| Finding | Current guidance | Plan before deeper credit or native change |
| --- | --- | --- |
| Two consumers of uncommon alloy | Show separate current mithril rows and warn that either accepted offer consumes the bar | Admit source identity, reset generation, quantity, consumption and output lineage; qualify second-stock availability, donated copies and concurrent offers. Preserve intentional scarcity unless a builder chooses otherwise. |
| Portable portal consumed as a quest material | Distinguish floor rift, living creature, ENTER journey, looping destination and consumed offering | Qualify actual pickup visibility/capacity, durable movement, ENTER/return, offering and recovery. Add admitted selected portal/root/destination facts before travel achievements; retain the isolated pickup repair and its news note. |
| Similar names, different kinds | Name all five source heads and both manuscript/banana inputs | Test strict ALL bundles, duplicate kinds, worn shield, nested roots, wrong source, partial selections and exact supplied items against native transactions. Never infer CARVE or reading controllers from names. |
| Cross-zone recipients and hints | Mother belongs here while physically in Scorched Valley; foreign seekers keep their owning journals | Route encounter hints to an already discovered owning journal, or provide a referral without discovering a remote zone. Qualify physical discovery, ownership, retirement, roaming, unavailable giver and rollback. |
| Narrated transformation, escape and reunion | Credit the actual alloy, portal and letter receipts only | Builder chooses caption clarification or explicit durable campaign stages. Record actor/state transition, prerequisites and rollback before declaring Timmy human, mother reunited, shaman escaped or captives freed. |
| Unbound/stale source descriptions | Great chamber's permanent-rift prose lacks a matching placed portal; portable-rift slain lore differs from floor reset; troll reward caption says ogre; two mages share elements keywords; dispatch caption has stale room number | These remain pending builder decisions, separate from the shipped pickup repair. Choose precise prose/aliases or an intentional controller/source change, then isolated regression, commit and news note. Do not guess a destination or add a new kill gate. |
| Epic/memory and possible portable shared scenery | Imported stone, memory and fixed portals remain independent systems | Qualify TOUCH admission/settlement and foreign rewards; keep unavailable paid guards. Shared level-60 pickup override can affect nominally fixed objects; observe actual presence before promising exits and use separately reviewed source-specific intent for any change. |

Player cards should show the seven exact outcomes, a distinct row for every
head, two separate alloy consumers and Timmy's optional letter link. Put key
access, Timmy's departure, the mother's roaming location and the portal's
consumption beside the next action. Mark historical help separately from
materials ready now. The map is editable source guidance; dynamic availability,
personal recovery, campaign state and renewal need admitted semantic facts.

Focused source/schema/C++ projection tests, the full production regression,
maintained build, formatting and preservation checks are required before
publication. Synthetic receipts exercise journal projection, replay and cold
recovery, without proving played pickup, native offer acceptance, reward
settlement, actor movement/retirement, database persistence or daily renewal.
No accounting activation, DB/server operation, migration, deployment or merge
is part of this checkpoint. The full 220-zone goal remains active.

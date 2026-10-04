# Tundra: comprehensive source map

Priority 76 of the original roadmap. **Source-comprehensive; played qualification
remains pending.** The new [journal](../../../areas/story/tundra.story.json) is
schema three, revision one: six independent story outcomes and one paid armor
service, 16 local contacts, all 12 addressed topic families and 16 aliases.
Fourteen optional checks separate 12 present materials from two earlier receipts.
The six stories are potential daily candidates; the armor service awards neither
achievement nor daily credit. Discovery retains its own achievement. All new
discovery, encounter, journal, achievement and daily credit requires **active,
ready accounting**; frozen reward obligations retain their separate recovery.

**No native Tundra repair ships in this checkpoint.** Source, shop, fishing,
route, loading and presentation findings below remain proposals. Existing native
repair commits and their news wording remain separately identified in the
[execution register](../ZONE_STORY_ROADMAP_EXECUTION.md) and PR.

## Complete source closure

The active `areas/AREA` entry is `tundra *137-139`; zone 137 has registry bounds
13600–13938 and reset mode two. Its header records native levels 25–35; these
do not assert enforced admission or encounter difficulty. Physical membership
is 175 rooms: 13700–13735, 13740–13835, 13859–13870, 13889, 13891–13912,
13926 and 13932–13938. Registry bounds do not manufacture missing rooms.

- [All 20 native blocks](../../../areas/qst/tundra.qst): 13 M and seven Q.
  Twelve M families are addressed with 16 aliases. The wife's `qc_action 80`
  complaint is ambient, without an asked keyword or independent achievement.
  No QA occurs locally. Only the blacksmith has a coin input; only Eleadora's
  head exchange has D retirement. The two shaman requests have empty accepted
  captions but exact inputs, distinct experience rewards and independent receipts.
- [All 175 complete rooms](../../../areas/wld/tundra.wld): 64 exact title/prose
  families, 19 complete headers, 401 exact exit rows/390 numeric families and
  nine full exit-description/keyword families. The only extra-description block
  is the sign at 13835; no unknown trailing field or room F/T block occurs.
  Every repeated membership and direction/state/key/destination binding was
  inspected, including inn, underground river, village, forest, detached roads,
  loading branches and traps.
- [All 26 mobiles](../../../areas/mob/tundra.mob), 13700–13725, and
  [32 objects](../../../areas/obj/tundra.obj), 13700–13730 plus 13755: complete
  prose, flags, values, affects and extra descriptions. Hidden loaded books,
  scales, head and wooden key are not ordinary CARVE products. Snowy adventurer
  boots, grey giant boots, glowing ice leggings and dragon fire leggings are
  separate kinds. Prototype existence alone is not a usable source.
- [All 246 resets](../../../areas/zon/tundra.zon): D36/O8/M180/G16/E6;
  197 exact and 197 parent-aware families. There is no local F/P/R reset.
  Duplicate guest/traveller placements retain their own parent inventory.
  The three imported bandages, 363/368/369, were read in full.
- [The complete shop](../../../areas/shp/tundra.shp): halfbreed 13719 at
  13870 sells iron rations, water, a backpack, two potions and three imported
  bandages. Neither fishmonger 13724 nor bartender 13708 has a configured shop
  or accepted local exchange. The literal room inn assignment is reception
  13714 in [the assignment table](../../../src/specs/specs.assign.c#L2459).
  No local ROOM_INN or ACT_TEACHER flag occurs, and no local epic-teacher entry
  supplies an omitted quest.
- Mirror 13701 has type 29 and values `[270,13751,0,1]`: PUSH, target bedroom,
  north, move the switch item. The loader automatically installs
  [item_switch](../../../src/world/db.c#L3166); absence of a literal object
  assignment does not mean absence of a handler. Selected argument, command,
  target, direction, current BLOCKED state and reverse-side behavior were traced.
- Shared offer selection, exact receipt ownership, multiple material roots,
  reward indexes, coin/experience settlement and D retirement were reviewed.
  SEARCH, OPEN, UNLOCK, key breakage, potion use, CARVE, fishing creation,
  loaded-exit decoding, reset states and ordinary mobile wandering were traced.
  Source closure does not qualify persisted personal acquisition or native play.

Bounded foreign review covers all 148 intersecting Q/QA signatures and all 22
distinct accepted/disappearance caption bodies: newbie 33, Harrow one, ship
yards seven, vehicles 99, Jade two, Alatorin three, Surface mini one and Surface
two. These common fish/material consumers keep their canonical owners; they
are not extra Tundra stories. Bom's complete painting exchange and mobile,
Lake Tantral placement, painting prototype and loaded artist sources were read.
His reward is **pike 318, lobster 319 and hairy crab 330**, without clam 334.
Foreign paid merchants supply clam 334 and pike 318, not lobster; their coin
inputs remain unsupported under active accounting. Their active placements and
full rooms were checked. Cairme's complete rations-plus-four-beer continuation,
mobile, tavern and active placement were also reviewed.

All five foreign required prototypes were read: pike 318, lobster 319, clam 334,
snapjaw shell 43137 and fire gland 43138. There are 68 relevant foreign active
reset groups: ship yards 33, vehicles four, Alatorin 30 and Surface keeps one.
They include turtle/pyrolisk stock, foreign rations and pouch/backpack contents;
none loads a local quest book, snowy boots, head, scales, key or local actor.
Distinct foreign source actors and complete rooms were checked, including
ship-yard dispersal rooms, shackled Crimson Fulgur creatures and Alatorin
bladeback pyrolisks. Wearable shell and potion-type gland must be loose for
the shaman; their ordinary wear/use effects are separate facts.

Boundary closure covers 19 directed edges: seven reciprocal Surface pairs,
the reciprocal Great Underground River pair, one incoming staff-room edge and
two unusable outgoing destinations. Complete foreign room bodies were read.
All 713 active type-25 portal prototypes were scanned; none targets Tundra.
Legacy room 110038 exists only in inactive `Duris3.wld`, not the active registry.
The [generated review index](../../reference/zone-story-audits/tundra.md)
retains locations and every native classification. Bounded dependency review
does not claim a new comprehensive Harrow, ship-yard or vehicle pass.

## Exact accepted exchanges and progression

| Outcome | Exact loose input | Declared reward | Progression and boundary |
| --- | --- | --- | --- |
| [Four books](../../../areas/qst/tundra.qst#L40) | Old 13708 AND green 13709 AND magenta 13710 AND black 13711 | Snowy adventurer boots 13713 | Four different kinds together; no personal guest kill is required, despite the thanks caption |
| [Eleadora's boots](../../../areas/qst/tundra.qst#L109) | Snowy adventurer boots 13713 | Glowing ice leggings 13712 | Optional earlier book receipt; supplied boots skip it. Recipient stays after this exchange |
| [Malinar's head](../../../areas/qst/tundra.qst#L120) | Bloody head 13722 | Tundra sword 13720 | Exact loaded item or supplied copy; Eleadora leaves after acceptance. No actual peace/world-restoration predicate |
| [Fire gland](../../../areas/qst/tundra.qst#L139) | Fire gland 43138 | 85,000 copper AND 65,000 experience | Foreign pyrolisk source or supplied gland; independent of shell request |
| [Turtle shell](../../../areas/qst/tundra.qst#L144) | Snapjaw shell 43137 | 85,000 copper AND 55,000 experience | Foreign turtle source or supplied shell; independent of gland request |
| [Feed the barbarian](../../../areas/qst/tundra.qst#L157) | Clam 334 AND pike 318 AND lobster 319 | Fishbone key 13705 | Three different kinds; Bom can supply two, fishing or supplied stock can complete the set. Key opens the shaman's home |
| [Red scale armor](../../../areas/qst/tundra.qst#L66) | Red dragon scales 13714 AND 500,000 copper (500 platinum) | Spiked red scale armor 13715 | Supporting service, no achievement/daily. Mixed input is unavailable with accounting active |

Six native item-only outcomes are daily-eligible, **including the D-retiring
head exchange**. D changes actor availability; it does not make the definition
daily-ineligible. Actual reset stock, recipient presence, reward obligations,
owned credit and next-day renewal still require gameplay qualification. The
blacksmith's displayed 500 platinum agrees with his native 500,000-copper term;
no local price discrepancy was found.

The hunched man begins at reception 13714. Four exact hidden books begin on
different actors: old G on human traveller 13706 at 13752, green G on guest
13704 at 13750, magenta G on shady traveller 13705 at 13751 and black G on dark
traveller 13707 at 13713. Additional human/shady/strange guests at 13759/13760/
13758 start without those books. Bookrack O13700 at reception is not populated
with them. Neither four generic `book` names nor four copies of one kind replace
the required four roots. Source kill, recovery and donor transfer stay separate.

The book reward can feed Eleadora's boots request. Historical success is optional
for supplied boots and never restores boots spent, worn or handed away. Both
Eleadora outcomes are independent; returning boots first is guidance for keeping
the same recipient available, not a forced DAG edge. Acceptance of the head
retires the actual actor through the [shared completion path](../../../src/world/quest.c#L1117).
Her dialogue's threat, sword origin and peace claim are narrative context until
explicit campaign state and victory events exist.

Malinar begins at 13911 with G13722, his own bloody head already in inventory,
and E13719 ice-shard necklace. The dragon begins at 13753 with G13714 scales and
E13702 fire leggings. Their source scenes and branched movement are explained
without treating an ordinary carved part, personal defeat or rare equipment as
the required item. Supplied heads/scales remain valid native inputs.

The starving barbarian begins at 13808; his food reward is a key, not a fish.
Bom's [Harrow painting exchange](../../../areas/qst/harrow.qst#L114) can supply
the pike and lobster. Hairy crab does not replace the still-missing clam.
The painting's active G stock belongs to artists at 29446; supplied paintings
can skip that source. Bom's canonical receipt remains Harrow-owned. Iron rations
13723 instead support [Cairme's foreign exchange](../../../areas/qst/shipy.qst#L730)
with four black beer 43143; that continuation earns no Tundra quest outcome.
Common fish sold, eaten, cooked or used in another quest remain absent here.

## Access, actual availability and services

Verified Surface pairs are 13703W ↔ 538336E, 13712S ↔ 514280N,
13778E ↔ 538746W, 13796W ↔ 542729E, 13870W ↔ 523898E,
13895E ↔ 543120W and 13936W ↔ 543909E. Underground river 13733S
connects to 30264N. Staff chamber 1221NE enters reception without a matching
ordinary return; it is not a promised mortal entry or quest prerequisite.

Enter Bone Fort Inn through the visible closed northern bone door. From the
lobby, west is reception; north then down reaches the Onyx Stairs. Their
northern passage door has raw five masked to door type one and reset D1:
visible and closed, despite hidden-passage prose. Open it; do not require SEARCH
for this entrance. Bedroom back doors generally have D5 secret/closed state.
The lounge's southern workshop door also starts visible and closed through D1;
its raw five is not a runtime SECRET flag. SEARCH is unnecessary there too.

The working mirror is O13701 in bedroom 13751. Its north exit is a non-door
with D8 OPEN/BLOCKED; the reverse passage wall is a door with D5 SECRET/CLOSED.
Actual `PUSH mirror` selects the object and clears the forward BLOCKED bit;
it clears reverse BLOCKED, without clearing reverse SECRET/CLOSED. It does not
require ownership, a key, a prior book receipt or every decorative bedroom
mirror. The [loader](../../../src/world/db.c#L1511) discards higher raw state
bits; the [D reset](../../../src/world/db.c#L4102) supplies actual secret/blocked
state. The [switch implementation](../../../src/specs/specs.object.c#L309)
does not publish a dedicated quest-switch credit event. Same-episode shared
opening and personal actor credit need separate qualification.

Fishbone key 13705 fits the pickproof west flap 13808→13817, the shaman's
home. The wife 13725 at 13809 carries hidden wooden key 13730 for the different
pickproof north flap 13809→13725, the fishmonger's hut. Both reset locked;
their reverse exits start closed but unlocked. Both keys have break chance 100
on ordinary successful UNLOCK in [the shared command](../../../src/cmd/actmove.c#L3122).
Current key, actual successful unlock, reverse state and later arrival are
different facts. Public opening or supplied key/material can skip personal
feeding/source history. No quest requires harming the wife or rescuing her husband.

[FISH](../../../src/economy/tradeskill.c#L585) requires skill, a recognised pole,
normal standing posture, an eligible water room and a continuing valid actor;
catching is delayed and can fail. Its twelve-kind pool includes 318/319/334.
The six underground-river rooms 13729–13734 have actual underwater-ground
sector 10, whereas Fishing Docks 13819 has land/mountain sector four. The
docks cannot satisfy [IS_WATER_ROOM](../../../src/core/utils.h#L266); do not
advertise guaranteed ice fishing there. The river has sharks and access costs.
Successful catches use the [ownership creation grant](../../../src/economy/tradeskill.c#L61)
with generic crafting origin. Dedicated caught-versus-donated fishing lineage
and first-source objectives need explicit evidence rather than a journal guess.

Dragon 13753–13757, Ixarvonl 13763–13767, Eleadora 13891/13892 and Malinar
13911/13937/13938 use ordinary [mobile wandering](../../../src/mob/mobact.c#L8016)
over branching exits. Some branches reach public rooms; others are exitless traps.
The actor's loaded prototype does not guarantee a reachable giver/drop. Rare
mask O13703, earring O13755 and giant boots O13728 are separate stock; descriptive
1/6, 1/36 and 1/12 odds are not qualified modern runtime guarantees. No source
or journal sends a player into the administrative loading rooms as a normal task.

The general store's icy potion has nonzero fifth value and the separate cloudy
grey potion has its own spell values. Fire gland is also potion-type, while the
shell is wearable. Obtaining, wearing, quaffing, damaging, healing or wielding
the sword does not complete an offering or the larger campaign. The inn service,
shop transactions and ordinary item effects retain their own authority paths.

## Findings and fair repair plans

| Finding | Evidence and present impact | Proposed work and acceptance criteria |
| --- | --- | --- |
| Giver/source availability can vanish into branches | Eleadora/Malinar/dragon/illithid start in branching load rooms with trap outcomes and caps; head D retires Eleadora | Builder confirms intended rarity/absence and same-episode ordering. Add admitted actual actor presence/reset-generation state; qualify both rewards, traps, actor replacement, replay and next-day renewal before altering spawn design |
| Fish request has no local village fish shop or working dock | Fishmonger has selling prose but no shop/Q binding; dock sector four fails water eligibility | Decide scenery, foreign supply or intentional local fishing/vendor. Any shop or sector edit must be a separate native repair with stock, authority, guards, failures and actual played purchase/catch proof; keep supplied routes valid |
| Some signposted routes do not exist | Sign13835 promises south/west, but only north/east exits exist; detached mountain path ends at13869S→-1; 13893E targets inactive legacy110038 | Builder chooses intended modern destinations or revised clues. Audit reachability and reciprocal loaded exits, then make isolated route/clue fixes with before/after player traversal and news wording; do not reactivate a legacy ocean map by guessing |
| Automatic mirror is a shared effect, not a recorded personal switch objective | Type29 auto binding; PUSH clears forward BLOCKED but retains reverse SECRET/CLOSED | Add selected object/actor/command and actual changed exit/reset generation event. Qualify wrong object, repeats, existing public opening, reverse state, reset and custody without inventing a lock/key prerequisite |
| Current ALL ingredients differ from earlier history | Four book kinds and three seafood kinds; Bom supplies only two seafood kinds | Freeze exact input roots, recipe, count and canonical foreign origin, preserving supplied branches. Qualify partial sets, repeated wrong kind, worn/nested/consumed stock, donor transfer and cold retry without restoring spent materials |
| Paid preparation remains guarded | Scales plus500,000copper and foreign clam/pike coin purchases have unsupported durable input | Preserve active-ready accounting. Add atomic wallet/material/output settlement, failures, idempotency and recovery before enabling these services; service receipts remain separate from story achievement/daily credit |
| Narration overstates evidence | Hunched man thanks kills although acceptance checks books; head already loads on Malinar; Eleadora declares peace | Builder chooses whether to clarify captions or define actual personal-victory/world-state rules. First obtain admitted source/combat/campaign events; no new credit from questions, kill claims, CARVE or reward-use prose |
| Empty reward captions and spelling reduce presentation quality | Shaman's two accepted captions are empty; Eleadora has an isolated misspelling; detached signs/load-room odds need editorial decisions | The journal names exact rewards now. Any native caption/typo repair should be separately scoped and tested against exact unchanged terms; do not label presentation improvements as repaired mechanics |

The implementation plan adds actor availability and retirement-aware guidance,
complete ALL material sets, partial foreign outputs, actual fishing eligibility,
automatic shared switches, loader-aware route audits and defined campaign
endpoints. The sidecar expresses reviewed explanations today. Dynamic data can
observe a real item, NPC or exit; it cannot determine a builder's missing route,
fish shop, spawn policy or intended campaign ending.

## Verification boundary and next queue item

Focused source assertions cover the complete counts, exact seven contracts,
different book/seafood kinds, XP amounts, retiring daily candidate, loaded item
parents, automatic mirror, decoded gate/reset states, distinct breaking keys,
actual fishing eligibility, foreign supplies and service restrictions. Synthetic
Python/C++ journeys exercise independent receipts, supplied boots without book
history, four-book/three-seafood readiness, wrong/worn materials, spent stock,
foreign ownership, non-awarding services, exact replay and cold recovery.

These are source and projection checks. They do **not** qualify played fishing,
source recovery, donor lineage, paid order, native offering, reward settlement,
personal victory, switch/unlock/key break, actor retirement, persistence or daily
renewal. No accounting activation, DB/server operation, migration, deployment or
merge was performed. Native area and quest bytes remain unchanged.

Catalog: **96 journals, 1,596 achievement units, 1,449 potential dailies,
2,203 rows**. All 2,668 native definitions, source fingerprint, content revision
and registry remain intact; the paid service replaces one fallback achievement.
Original queue: **76/220 source-comprehensive, 144 pending; The Fields Between
(`fields_between`) next**. The full roadmap goal remains active.

# The Town of Moregeeth: comprehensive source map

Priority 78 of the original roadmap. **Source-comprehensive; played qualification
remains pending.** The [journal](../../../areas/story/goblinht.story.json) moves
from schema two/revision one to schema three/revision two, preserving all seven
original story IDs and exact native contracts. Five cards represent qualifying
story outcomes; two represent paid crafting services without achievement or
daily credit. Twenty-one contacts preserve all nine addressed topic families
and 22 aliases. Fifteen optional checks separate 14 current material/access
checks from one earlier pouch receipt. Four excluded recipes retain their
supporting-service or nonterminal meaning. Discovery has its own achievement.
All new tracking requires **active, ready accounting**; frozen recovery is separate.

**Actual native repair, separate commit:** [7b916b887](https://github.com/Community-Duris/Duris/commit/7b916b887),
`Fix Moregeeth letter quest desk keyhole`. Ungal's letter was reset inside a
closed, locked desk whose key field was negative. Native PICK and KNOCK both
reject that setting before attempting the lock; OPEN rejects the lock and GET
rejects the closed container. Exactly one prototype field now changes the
keyhole setting from -1 to zero, permitting ordinary lockpicking/knock attempts.
The desk remains closed, locked, fixed and trapped. The letter's container
placement, text, Tala's request and reward are retained. No key or guaranteed
success is added. The [focused regression](../../../tests/async/test_moregeeth_letter_desk.py)
fails on original data and passes on repaired data. Source proof does not
qualify actual skill rolls, trap handling, pickup, offer or settlement. Saved
instances are not rewritten.

**News-ready:** “Moregeeth's trapped letter desk now permits normal lockpicking
or knock attempts, restoring access to the letter for Tala's quest.”

## Complete source closure

The active area is `goblinht`, zone 700. Registry bounds are 69294–70352, reset
mode two; physical membership is 353 rooms 70000–70352, 109 mobiles
70000–70108 and 96 objects 70000–70095. Native header levels 25–35 are source
metadata, without a claim of enforced admission or uniform difficulty.

- [All 20 native blocks](../../../areas/qst/goblinht.qst): eight M, six Q,
  one MA and five QA; eleven exact recipes. All nine addressed families and
  22 aliases are covered. Glub's `hello hi` MA is an addressed conversation,
  without a `qc_action` controller. Glub's sword and Gimbatul's crown have D
  retirement. Two paid item/coin inputs and two coin-only package inputs are
  supporting services; two same-kind item returns are nonterminal.
- [All 353 complete rooms](../../../areas/wld/goblinht.wld): 148 complete
  title/prose families, 26 complete headers, 1,350 exact exits/numeric bindings,
  17 complete exit-text/keyword families and no room E/F/T extras. Every
  repeated-family membership, direction, flag, key and destination was reviewed.
  Four planar pockets remain owned by this zone despite their plane names.
- [All 109 mobiles](../../../areas/mob/goblinht.mob): 105 full prose families
  plus all distinct keywords, names, appearance, numeric flags, race/class,
  levels, money and remaining fields. Actor 70108 has no local reset or quest.
  Same-name ghosts, goblins, bats, merchants and guildmasters were distinguished.
- [All 96 objects](../../../areas/obj/goblinht.obj): full prototypes,
  descriptions, values, wear/visibility flags, affects and the desk's trap.
  There are eight unlimited type-25 portals, no type-29 switches. The component
  pouch is type-10 potion; stone heart is a wand; the Moregeeth key is type-31
  lockpick. Names do not create alternate quest-item producers or universal keys.
- [All 549 resets](../../../areas/zon/goblinht.zon): D94/O17/P7/M306/E37/G72/F16,
  442 exact rows and 453 parent-aware families. Parent context matters for
  repeated G/E/F rows: the letter belongs inside the desk, not to Ungal; the
  pouch is inside the table; five skulls belong to five cave bats; bouncers,
  wargs and press-gang members follow their last M leader.
- [All ten shops](../../../areas/shp/goblinht.shp): bartender, Smog,
  performer supplier, odd magic shop, mage supplier, Zagh, rogue supplier,
  shamanic merchant, Ghash and Roge.
  Exact stock, keepers, rooms, price factors, trade types, hours, messages and
  restrictions were reviewed. No shop-only foreign prototype
  expands the imported reset set. All thirteen imported items were read in
  full: wards 54/73/77; potions 188/189/421; bandages 363/368/369; lockpicks 412;
  disguise kit 500; chillum 835; bank counter 3097. These are support stock.
- Local literal assignments are bartender 70029 → `world_quest` and room
  70175 → `inn`. The room lacks a ROOM_INN flag but has an explicit inn binding.
  Twelve ACT_TEACHER mobiles serve ordinary class training; no local epic teacher
  assignment adds another native zone exchange. The bank counter's generic
  wallet/storage hook, hometown/guild constants, justice jail/jailer and chaos
  map nickname remain separate systems. No dedicated local source controller
  supplies a missing rescue, bribe, vault entry or planar campaign.
- The bounded foreign scan covers all 713 active portal prototypes, foreign
  resets and native recipe inputs/rewards involving local kinds. No foreign
  portal targets a local room, no foreign reset loads a local actor/item and
  no foreign recipe consumes or produces a local item. Every required quest
  ingredient is local. Five boundary edges connect three complete foreign rooms:
  reciprocal Trukt cave 1655 ↔ outpost 70198, reciprocal Surface path 626940 ↔
  cave entrance 70327, and one-way staff fast-track 35 → inn 70175.

The [generated audit](../../reference/zone-story-audits/goblinht.md) retains exact
source locations and bindings. Source closure establishes what the native data
and reviewed shared handlers say; it does not assert played availability or
economic/database persistence qualification.

## Progression stories and supporting exchanges

| Recipient and source block | Exact input | Output and behavior | Journal treatment |
| --- | --- | --- | --- |
| Moreg, Q8 | Dura's head70030 | lockpick70028 +10,000 copper; stays | Story; potential daily candidate |
| Glub, QA28 | Pig-Sticker70001 | spectral sword70002; leaves | Story; potential daily candidate, recipient renewal needed |
| Gimbatul, Q61 | pouch70021 | magical key70022; stays | Story and optional preparation for crown |
| Gimbatul, QA70 | heart70013 AND ring70014 AND wings70015 AND pendant70016 | crown70017; leaves | Story; exact ALL set, potential daily candidate |
| Gimbatul, Q84 | sharp dart70093 AND10,000 copper | enchanted dart70094; stays | Paid service; no achievement/daily |
| Gorblag, QA98 | five copper | plastic package70018; stays | Excluded coin-only support |
| Drinking woman, QA105 | rot-gut bottle70026 | same-kind bottle70026 | Excluded nonterminal response |
| Tala, Q119 | Dura's head70030 | same-kind head70030 with refusal | Excluded nonterminal response |
| Tala, Q124 | short letter70065 | zombie flesh70066; stays | Story; potential daily candidate |
| Shamanic merchant, Q144 | five separate skull70075 roots AND500 copper | necklace70074; stays | Paid service; no achievement/daily |
| Gorblog, QA157 | five copper | plastic package70018; stays | Excluded coin-only support |

All seven old story IDs/contracts remain stable. The two paid cards change
category from request to service, reducing the catalog's achievement total by
two without changing native definitions, receipts, daily candidates or row
count. The five qualifying outcomes and discovery remain independently visible.
No keyword grants its own achievement.

### The town's rivalry, killer's sword and recipient availability

Moreg starts in his office70001, reached north from grand entry hall70287. He
wears Pig-Sticker70001. Ask `dura` to learn about his blackmail problem. Dura
starts behind the altar70177, reached through the temple's closed hinged altar
door. She carries the hidden exact head70030; it is not produced by a CARVE
operation on an arbitrary goblin corpse. The offered head gives Moreg's reward.
The “skeleton key” caption describes a type-31 lockpick with bonuses100/100,
not the sticky70019, magical70022 or rusty70057 key. Ordinary exact-key matching
does not let it unlock those doors. It may support normal PICK attempts.

The cemetery caretaker70058 in hovel70200 carries the rusty key. Cemetery
sepulchres are locked; Glub's entrance is the downward exit from70009 to70002.
Raw exit state two loads a pickable door, then D state two locks it. Glub accepts
the sword Moreg wears, grants a spectral sword and disappears. Recovering the
weapon may remove Moreg until reset, so the guide recommends completing his
desired turn-in first while he is present. This is advice, without a new enforced
order, personal-kill requirement or hidden receipt prerequisite. Exact supplied
heads/swords are accepted. Glub's netherworld pursuit is reward narration.

### Pouch → real key → four planar components

The sticky key is carried by the homely prostitute70006 instance in sign
room70333; her other instances do not carry it. It fits the northern private-room
door70029 →70031 in the Sorcerer's Academy. That side is raw state three,
pickproof; the reverse side is raw state two, pickable. Both are reset locked.
Inside, open fixed table70020 holds the pouch70021 by P reset. The table's own
pickup is unnecessary. The pouch is a potion with native spells, not a container;
quaffing it can consume the exact quest material. Generic shop silk/sand/guano
components do not replace it.

Gimbatul70022 starts in classroom70173. Returning the pouch grants magical
key70022, which fits the locked upward door to portal chamber70168. The classroom
side is pickproof, reverse side pickable. Both real keys have break probability
zero, surviving ordinary unlocking; public open doors or supplied components
skip personal key history. The earlier pouch receipt is optional explanation,
without restoring a missing key or enforcing an artificial story chain.

| Pocket and chamber item | Arrival room | Entry | Exact component/source reset | Return at arrival |
| --- | --- | --- | --- | --- |
| Fire, flame70006 |70076 | STARE flame, command139 | pendant70016 worn by efreeti70021 at70073 | STARE flame70009 |
| Earth, oval mirror70012 |70101 | ENTER mirror, command7 | heart70013 held by dao70017 at70100 | PUSH boulder70008, command270 |
| Water, crystal70090 |70124 | TOUCH crystal, command320 | ring70014 worn by marid70014 at70127 | TOUCH crystal70011 |
| Air, circular mirror70010 |70153 | ENTER mirror, command7 | wings70015 worn by jann70011 at70154 | SOUTH, command3 via smoke70007 |

Every portal has charge -1/unlimited. The command numbers were checked against
current [interpreter constants](../../../src/cmd/interp.h), where139 is STARE
and320 is TOUCH. Generic [item travel](../../../src/magic/spell_travel.c#L930)
uses the configured command and exact object; SOUTH is a keywordless directional
trigger. Returning by ordinary exits instead wanders through planar grids.
Room descriptions about pressure, currents, gravity or the protective pentagram
do not prove extra item, rescue or campaign controllers. Existing terrain and
travel checks remain applicable; no personal traversal receipt is invented.

Remove the four different worn/held quest items before offering them together.
Four hearts are not the four-kind set. Accepted crown exchange removes Gimbatul,
also closing his pouch and paid dart exchanges while absent. The player copy
warns of this shared availability without promising daily respawn, reserving
stock, forcing the pouch first or extending the journal into a campaign.

### Tala's evidence, trapped desk and refused head

From the cemetery, search/open the secret slab70015down →70003, then follow
the frieze hallway70203 west to Tala70204. She asks about revenge against Dura.
Giving her Dura's head returns a same-kind head with refusal; that is not the
letter outcome or proof of animation. A catalog receipt for the response must
not become a new story achievement or prove source lineage for a later hand-in.

Ungal70073 teaches in combat room70212. His fixed desk70064, not Ungal's G
inventory, contains the exact letter70065. Reach the rogue guild through either
the secret gaming-room trap door70207down →70209 or abandoned-hovel panel
70300west →70210. Search/open the actual secret door; these are not quest switches.
The repaired desk retains closeable/closed/locked flags13, capacity10, weight100,
and trap516/acid4/one charge/power25. Trap516 includes OPEN and room effect.
DISARM addresses that trap separately from the lock. PICK requires suitable
skill and picks; knock requires its normal availability and roll. Changing the
keyhole to zero restores attempts without bypassing the lock or trap.

Read the letter's extra description: Dura thanks Ungal for help with the
necromancers. Tala accepts the exact loose letter for zombie flesh. Her promise
to make Ungal a spectre is narration without a durable kill/transformation
stage. Supplied exact letters bypass personal opening and reading history.

### Paid crafts, supporting services and nonterminal scenes

The roguish merchant70072 at70211 stocks sharp obsidian dart70093 for the
Gimbatul craft. It requires that exact dart AND10,000 copper; temple bowl70031,
other obsidian objects and enchanted dart70094 are different kinds. The
shamanic merchant70078 at70276 needs five separate skull70075 roots AND500
copper. Five vampire bats70057 at70196 each carry one skull by G reset. The
similar bats70103 on the surface passage carry no skulls, and the cave recess
holds guano70053 instead. Whole corpses and unspecified carving do not replace
the configured items. Current count five requires five copies; wallet readiness
is not presently represented by a carried-item step. Both services keep their
exact accepted-receipt projection but award no local story achievement/daily.

The package sold for five copper by Gorblag70023 and Gorblog70104, the drinking
woman's bottle return and Tala's head refusal retain all four exclusions.
Brogan's inn rent, ten shop definitions, twelve class teachers, bank counter,
justice guards and bartender world quests remain their own services/systems.
Narrated gate bribes, enslaved people, vault protection, ghost hunts, revenge
and magical training do not create additional accepted local outcomes.

## Capability gaps and fair follow-up plans

| Finding | What ships | Additional plan and qualification |
| --- | --- | --- |
| Recipient removal crosses cards | Copy warns Moreg supplies another giver's sword; Gimbatul/Glub disappear | Admitted availability facts with actor instance, owner, retirement/reset lineage and honest next action. Qualify absence, supplied alternatives, concurrent turn-ins, D rollback/recovery and renewal without inventing an order. |
| Pouch reward grants access | Optional pouch receipt, current real key, actual locked/pickproof directions | Qualify durable key reward/pickup/transfer/UNLOCK/OPEN and public passage. Keep keys distinct; current receipt alone never restores an item or proves traversal. |
| Four automatic travel commands | Explicit entries/returns, local ownership and unlimited charges | Capture admitted selected-object command/destination plus actual movement before personal travel credit. Qualify STARE versus TOUCH, directional smoke, grids, terrain, group travel, rollback and supplied-component bypass. |
| Container material with lock/trap | Actual one-field desk repair; current letter and optional picks | Qualify source-container lineage, skill failure/success, trap disarm/open damage, loose custody, give/recovery and reset. Opening or reading alone is not acceptance. Saved instances need separate operational treatment. |
| Typed items differ from prose | Lockpick reward, potion pouch, wand heart and reset head/skulls explained | Derive source guidance from typed prototypes, exact reset parents and native handlers. Qualify consumption/wear/transfer; generic names do not grant CARVE/READ/FOOD/CONTAINER or universal-key capabilities. |
| Multi-root craft plus coins | Services with five-copy readiness and exact contracts, zero achievements | Add explicit fee readiness and admitted all-roots/wallet offer settlement; qualify partial/wrong items, repeated kinds, insufficient coins, concurrent offers, reward replay and failure compensation. Preserve service classification. |
| Nonterminal same-kind returns | Bottle/head exclusions kept | Preserve rejection/nonterminal outcome and root lineage separately from successful quests. Qualify replacement-root semantics and later source claims rather than inferring original-instance continuity. |
| Narrative campaign promises | No additional rescue, revenge, netherworld hunt or vault controller inferred | Builder chooses clarified prose or explicit durable stages with prerequisites, state scope, actor lifecycle, recovery and completion. Guided journals do not implement these campaigns. |
| Minor copied/stale narration | Native Gimbatul pouch caption says Gorbatul; Gorblog package caption says Gorblag; academy/room lore differs | Pending builder copy decisions, not shipped repairs. Correct names separately if chosen; preserve intended guild roles and route difficulty. No evidence warrants labeling every inherited discrepancy a broken quest. |

The desk fix is an actual gameplay-data repair with its own commit, regression
and news sentence. Other findings above are plans, not completed fixes. The
schema/projection fixtures cover exact sources, all eleven classifications,
supplied materials, worn/nested/duplicate kinds, optional history, five-copy
readiness, independent outcomes, service exclusion, replay and cold recovery.
Synthetic receipts do not qualify actual pickup, reading, skill/trap handling,
travel, fees, NPC lifecycle, reward settlement, database persistence or daily
renewal. No accounting activation, DB/server operation, migration, deployment
or merge is part of this source checkpoint.

# The Mountain Settlement of the Harpies: comprehensive story mapping

Priority94 replaces the existing schema2/revision1 generic journal with
schema3/revision2 source-specific guidance. The [sidecar](../../../areas/story/harpyht.story.json)
has two story cards, five contacts, one source-verified custom ASK topic,
three optional rows and one explicit native-contract exclusion. Zone311 spans
registry30938–31260; actual rooms31100–31260. All new discovery, encounter,
journal, achievement and daily credit requires active, ready accounting;
frozen recovery remains separate. Source review is not played qualification.

Player guidance uses named places, exact material names and working commands,
with clear allegiance and availability limits. Prototype numbers, dispatch
internals, reset flags and the technical repair evidence remain in this dossier;
the journal presents the dwarf rescue and queen recognition as readable steps.

## Complete source closure

| Source | Full review and implications |
| --- | --- |
| [Quests](../../../areas/qst/harpyht.qst) | All3 Q responses/terms; no M/MA/QA. Queen consumes shackles with no reward/retirement; dwarf consumes key, gives shackles and retires; khan consumes feather on paper but normal special dispatch handles it first. No qc_action unlock/escort/transform exists in these records. |
| [Rooms](../../../areas/wld/harpyht.wld) | All161 rooms,59 full prose families,10 headers,5 complete metadata families,357 exits,110 relative patterns and6 complete exit texts. Closed/secret routes, keyless doors and authored F20/50/60/80 falling metadata reviewed. Surface620042 north↔31100 south is the only boundary pair. Full foreign room record reviewed; no missing boundary. |
| [Mobiles](../../../areas/mob/harpyht.mob) | All44 full prototypes/prose/numeric tails31100–31143 and all local placements. No imported or unplaced mobiles. Queen31109 at31174, dwarf31118 at31198, khan31124 at31189, gargoyle31108 at31128 and crow31140 at31234/31260. Flags/neutral eligibility are distinct from quest terms. |
| [Objects](../../../areas/obj/harpyht.obj) | All19 complete prototypes31100–31118, flags/values/effects/extras. Shackles31111 are the sole locally unplaced prototype, produced by the dwarf exchange. Hidden key/feather/egg/plans/artifact, two OTHER cage props, fixed cage/ladder teleporters, skeleton container and golem prop distinguished. No item/type/weight/source change. |
| [Resets](../../../areas/zon/harpyht.zon) | All176 commands:M126/D16/O13/G11/E7/F2/P1; all chances100,149 exact/151 parent-aware/84 expanded families. Native key cap1 O31240; feather cap2 G under crow placements; plans cap1 P skeleton; tail cap1 G under one of two gigasaur M31230. Preserve conditionals, shared caps and mode2. |
| [Shops](../../../areas/shp/harpyht.shp) | Both complete records:merchant31100 at31105 sells seven imported prototypes; Merv31103 at31178 has no stock. Full imported193/203/204/336/363/368/369/377/3097 records reviewed. Mining earth, spellbook/quill, fishing/bandages/backpack and counter are not local quest definitions. |
| [Literal special assignments](../../../src/specs/specs.assign.c) | Four active mobile bindings:money_changer/gargoyle_master/harpy_good/harpy_evil. Full [Harpy special file](../../../src/specs/specs.harpy_hometown.c), [retired exchange procedure](../../../src/economy/currency_exchange_proc.c), [interpreter special dispatch](../../../src/cmd/interp.c), generic quest exact-root/retirement publication, handler extraction and portal command matching reviewed. harpy_gate’s object assignment is commented, not active. |
| Foreign closure | Six touching recipes:3 local/3 foreign, all complete texts/terms. Wicks40716 at40717, engineer76049 at76079 and trapper76243 at76255; full mobile records reviewed. Full head76059/ore202 reward prototypes reviewed; egg/plans/tail contracts stay separately owned. No foreign reset group uses local prototypes. All713 active type25 declarations scanned:only local cage31103 and ladder31102 enter local rooms. |

## Exact native exchanges and custom dispatch

| Contract | Actual role and boundary |
| --- | --- |
| Dwarf31118 /Q9: I31104→I31111; D1 | Exact loose key gives shackles and retires that dwarf. Accepted retirement is not a followed escort, personal source recovery or door unlock. Supplied key fits. |
| Queen31109 /Q2: I31111→no reward; D0 | Records native proof recognition when dispatch reaches it. Queen special returns false for neutral actor’s shackles, letting the native handler run; non-neutral GIVE is intercepted. Native response says destiny is chosen, but this Q does not change race/racewar/alignment. Supplied shackles fit without own dwarf receipt. |
| Khan31124 /Q18: I31112→no reward; D0 | Excluded. For neutral actors the custom feather procedure consumes/changes actor and returns before qst_func; non-neutral actors receive already-picked-path handling. No accepted quest receipt or safe daily is produced by normal dispatch. |

The queen and khan both use feather31112 in their custom procedures. They
only inspect the first item word, not the addressed recipient, and do not
verify a PC/Harpy actor. Both force race=Harpy; queen selects good/+1000 and
khan evil/−1000. The generic interpreter visits mobile specials before quest
handlers, so an unrelated addressed GIVE can be intercepted. Direct item
extraction and actor mutation are not coordinated durable quest settlement.
The pending-wallet/storage fences do not replace actual recipient or atomic
actor/item admission. This requires a deliberate shared fix and a distinct
one-time branch outcome, rather than calling the native feather Q a completion.

## Access and current material

ENTER fixed cage31103 at31195 reaches31240; its DOWN exit returns to31195.
Key31104 is hidden floor stock there. Cages31101 at31193/31198 are OTHER13
props; their closed/locked prose is not an operable container. Give the key to
the dwarf at31198. Its value[1]=0 is distinct from consumption during a quest;
no keyed cage unlock or personal recovery is required by the native contract.

Feather31112 is hidden inventory stock on two crow31140 resets, with cap2.
It is not CARVE output and says nothing about who killed or supplied the crow.
The queen’s shackles producer is the dwarf; optional own rescue history never
restores current shackles. Exact gifted proofs remain valid native material.

Fixed ladder31102 at31207 is type25/value command340=PULL; CLIMB556 is
different. It reaches31210, where DOWN returns to31207 and hidden egg31105
is floor stock. Fixed cage command7=ENTER and both unlimited−1 portal values
remain. No ordinary static incoming edge to either destination exists; these
are declared portal routes. Native travel/fall/admission/current presence still
needs played qualification. Changing the ladder to CLIMB would alter a working
command mapping and is not justified by its prose.

Plans31115 are nested in hidden fixed skeleton31109 at31248. Values
1000/5/0/1000 mean closed/pickproof, unlocked and keyless. OPEN/remove papers
before a foreign offer; a skeleton in inventory cannot supply a loose paper
root. Giant battle golem31106 is an unbound OTHER prop. Artifact31108 is hidden
floor stock, with buried prose but no BURIED metadata. No local activation or
archaeology completion is bound.

## Undead path, hometown intent and services

ASK gargoyle undead reaches the corpse-count branch at31128. Its raw matcher
does not accept the advertised unlife word. It counts at least two floor NPC
corpses whose names contain lowercase harpy, with no personal kill/custody
requirement. Below two it reports how many remain; at two the active body has
no transformation or corpse consumption. Those operations, race/racewar changes
and Bard→Spiper code are commented out. Treat this as dormant content requiring
an intent decision; do not promise an undead transformation or award a corpse
delivery achievement. LOOK gargoyle hints at the custom ASK, not a learned fact.

Communal nest31177 has ROOM_INN and native hometown references. Its destiny
sign advertises an exit restriction whose harpy_gate assignment is commented.
Keep the disabled exit policy and verify lore deliberately. The full merchant
shop has seven native imported supplies; Merv’s money_changer now redirects
LIST/EXCHANGE to bank tellers. Counter advertisement/empty shop is not a quest
or evidence of a paid local exchange. Fauna, nests, ancient craftsmanship and
mining earth193 remain separate services, stock and lore.

## Foreign source stories

The [Home of the Divine dossier](HOME_OF_THE_DIVINE.md) owns Wicks’s egg31105
exchange for25000 copper. [Ultarium](ULTARIUM.md) owns plans31115→siege golem
head76059/engineer retirement and tail31117→two copper ores202. Tail is G stock
on gigasaur31136, not a local CARVE action. Local retrieval/current material,
supplied gifts, foreign accepted exchanges and foreign discovery are separate.
No extra local completion is granted for holding an egg, plans, tail or reward.

## Required builder and capability work

| ID | Evidence and fair implementation plan |
| --- | --- |
| **ZSQ-HARPYHT-DISPATCH-RECIPIENT-ACTOR** | Both queen/khan specials precede qst_func, parse only the item and accept the same feather; non-neutral GIVE is intercepted before canonical hand-ins. Neither verifies addressed recipient or Harpy/PC actor. Qualify exact addressed NPC, awake/visibility/actor/race eligibility, item UID, pending operation and intended one-time policy; unrelated-recipient/player gifts must do nothing. Decide queen shackles versus feather design explicitly rather than silently rewriting the native quest or PvP allegiance. |
| **ZSQ-HARPYHT-ATOMIC-LIFE-PATH** | Direct extract_obj plus race/racewar/alignment mutation has no accepted quest receipt or coordinated actor/item publication. Refuse unsafe active-accounting custom conversion until typed settlement is available; commit selected item retirement and exact prior→new actor state together, with actor version, failure isolation, concurrency/replay/reconnect/cold recovery and historical policy. Persist a distinct one-time branch outcome; do not count the shadowed native khan Q as a daily or grant both mutually exclusive paths. Separate named shared fix/news and original-fails/repaired-passes tests required. |
| **ZSQ-HARPYHT-UNDEAD-PATH** | ASK gargoyle undead counts two floor NPC corpses by lowercase harpy substring, while advertised unlife fails the matcher. Consumption/transformation/Bard→Spiper code is commented out; two corpses yields handled silence. Preserve the disabled transformation until intent is established. Choose truthful retired lore or an intentional atomic corpse/actor conversion; qualify exact NPC corpse identities, ownership/root children/decay, allowed supplied corpses versus personal kills, actual recipient, race/class/stat changes and recovery. Do not uncomment legacy mutation as a journal fix. |
| **ZSQ-HARPYHT-SOURCE-RENEWAL** | Hidden cap1 key O31240, cap2 feather G on two crows, dwarf D retirement and reset mode2 govern supply/recipient renewal; active item resets remain refused. Supplied exact key/shackles fit without own source/rescue. Qualify UID/custody/current visibility, retired instance, legitimate reset issuance and repeated source availability before offering two potential dailies. No stock/cap/pickup/retirement edits by convenience. |
| **ZSQ-HARPYHT-ACCESS-PRESENTATION** | ENTER cage31103→31240 with ordinary DOWN return; other cages are OTHER props despite locked prose. Ladder uses PULL340, not CLIMB556; DOWN returns from31210. Sign promises destiny gate but harpy_gate assignment is commented. Preserve commands/portals/falls/disabled exit policy; explain actual access and review mismatched prose deliberately. Dynamic journal availability needs admitted neutral actor state/recipient dispatch, not an assumed universal racewar prerequisite or a fabricated unlock/travel receipt. |
| **ZSQ-HARPYHT-LEARNED-SOURCE-FACTS** | Corpse-count ASK and LOOK hint have no learned completion; current items, source GET/gifts, own kills, reading, OPEN and entering cages are different facts. New optional semantic facts require admitted actor/NPC/UID/room/instance/content/state/output and rejected/replayed/recovered evidence. Existing schema3 material/optional receipt checks suffice now; do not make personal history mandatory from good-deed or kill narration. |
| **ZSQ-HARPYHT-FOREIGN-SOURCE-OWNERSHIP** | Egg→Wicks coins, nested plans→engineer head/retirement and gigasaur tail→two ores belong to Divine Home/Ultarium. Egg/tail are floor/G stock, not CARVE; remove plans from skeleton before offering. Golem prop and hidden artifact lack local custom quest bindings; buried prose is not BURIED metadata. Preserve foreign contract/reward/source ownership and discovery; actual golem activation, archaeology or crafting requires intentional builder design and admitted output. |
| **ZSQ-HARPYHT-SERVICES-LORE** | Merchant’s full native shop and seven imported supplies, Merv’s retired exchange redirect/empty shop/counter sign, hometown/inn flags, fauna/nesting and mining object193 are separate services/lore. Verify actual command/payment/location admission before changing advertised services or awarding deeper objectives. Preserve retirement and native home/inn placement; clarify story versus service without manufacturing bank/mining/merchant achievements. |


Existing schema3 current material/optional accepted history is sufficient for
the two native cards. Custom allegiance requires an admitted actor-state
transition alongside selected item settlement; corpse conversion also needs
identity/custody/children/decay and deliberate race/class policy. Document these
as concrete universal requirements rather than inferred mandatory source kills.

## Validation and limits

Focused native/schema/C++ tests cover all3 classifications, dispatch order,
exact key/shackles, held/wrong/nested-material readiness, supplied shackles
without own rescue, history without current shackles, excluded feather route,
independent receipts, foreign ownership, replay/cold recovery and historical
native receipt compatibility. Full production regression/all111 journal
journeys, server build, changed/staged formatting and preservation are required.

Catalog remains111 journals; totals become1581 achievements/1440 potential
dailies/2192 rows because one formerly generic khan request is excluded.
Native2668 definitions/fingerprint/revision2/registry and the other110 mappings
remain unchanged. Roadmap94/220 comprehensive,126 pending; Behemoth Herders
next. This checkpoint contains no native repair; eight findings remain builder
work. No live custom race conversion/corpse handling/recipient interception,
quest admission/persistence, access/falls or reset-renewal qualification is
claimed. No activation/DB/server/migration/deployment/merge is performed.

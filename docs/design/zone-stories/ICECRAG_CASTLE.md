# IceCrag Castle: comprehensive source map

Priority 71 of the original roadmap. **Source-comprehensive; gameplay qualification
remains pending.** The revision-one [journal](../../../areas/story/icecrag.story.json)
classifies all eleven native offerings as eight stories, two supporting services
and one excluded incomplete recipe. Seven stories are item-only daily candidates;
the sergeant's meaningful winter-clothing request remains guarded by its coin fee.
Twenty-five contacts cover all 55 addressed families and 129 distinct per-contact
topic aliases. Sixteen optional checks describe fifteen material requirements and
one earlier receipt. Discovery has its own achievement. Every new discovery,
encounter, journal update, achievement and daily credit requires **active, ready
accounting**; frozen obligations use their separate recovery path.

No native zone or quest repair ships in this checkpoint. Excluding an impossible
recipe from journal credit is classification, not a repair of that recipe. The
repair proposals below must not be reported as shipped fixes in player news.

## Source closure and authority

The active `areas/AREA` row is `icecrag *970–972`; the canonical zone is 970,
registry range 96982–97289, reset mode two, levels 55–65. Physical membership is
243 room bodies, with gaps and distributor rooms, rather than a contiguous
290-room assumption. Reviewed source consists of:

- [All 67 native blocks](../../../areas/qst/icecrag.qst): 55 addressed M,
  ten Q, one QA and one ambient MA. Twelve addressed actors provide 130 raw
  aliases, reduced to 129 distinct per-contact aliases. Myrke's ambient speech
  is not a thirteenth learned-topic family or an accepted terminal.
- [All 243 rooms](../../../areas/wld/icecrag.wld): 232 complete title/prose
  groups, 30 numeric headers, 44 non-exit metadata families, all 628 exits,
  241 numeric exit families and 200 full description/keyword pairs.
- [All 66 mobiles](../../../areas/mob/icecrag.mob) and
  [138 objects](../../../areas/obj/icecrag.obj), including full descriptions,
  extra text, values, equipment, affects, key break rates and teleport verbs.
- [All 620 resets](../../../areas/zon/icecrag.zon): 374 exact argument families,
  460 parent-aware families; E247/M150/D148/G37/O30/P4/F4. F followers keep their
  actual leader; following E/G rows belong to that follower. There are no R rows,
  imported mobiles or local shop file. No room has ROOM_INN and no local mobile
  has the computed ACT_TEACHER flag.
- [All actual local assignments](../../../src/specs/specs.assign.c#L1105):
  nineteen mobile assignments plus `artifact_hide` on object 97118. The two
  `ice_privates2` assignment lines are commented out; reset F already makes those
  privates follow the sergeant. Numeric guard cases do not create a binding:
  the `guardian` cases for 97126/97242 are not assigned to these local guards.
- All implementations in [the IceCrag procedure section](../../../src/specs/specs.winterhaven.c#L5124),
  [the hiding artifact](../../../src/specs/specs.artifacts.c#L73), and relevant
  shared quest admission/reward, dispatch, visibility/selection, reset/renumber,
  movement, speech-door, teleport, key/container, combat/death and service paths.
  Static/projection checks do not qualify their admitted live transactions.

The bounded active-world scan also closes four imported objects (72, 359, 55172,
67252), twelve foreign recipe kinds (eleven present; 6551 absent), eighteen
selected foreign reset groups, six complete relevant shop records and ten foreign
native producer/consumer rows. The active type-25 scan examines all 713 portals;
only the local pedestal and orb target this registry. Seven cross-registry edges
have five foreign room IDs: four active bodies and absent 97352. Their purposes
include the ordinary Surface approach, Royal Mausoleum boundary, the deranged
man's exit to Lava Tubes, and Winterhaven's incoming gnomish-key distributor into
the ice vault. The generated [audit](../../reference/zone-story-audits/icecrag.md)
is supporting evidence, not a replacement for this closure.

## Native requests and progression

`C` below denotes native coin-value units, not the displayed wallet denomination;
`E` is nominal experience before recipient/group policy. Item IDs are maintainer
evidence; player guidance uses names, places and actions.

| Native line / giver | Exact offering → declared results / departure | Journal treatment and route |
| --- | --- | --- |
| [22 / cleaner 97001](../../../areas/qst/icecrag.qst#L22) | C1000 → I97005; D1 | Paid access service, guarded. Other cleaner placements equip this guard-walk key; the foyer cleaner instead equips page 3 |
| [61 / artist 97002](../../../areas/qst/icecrag.qst#L61) | I11606 + I11607 → I97029 + E50000; D1 | Story: exact hammer AND chisel. Ghore stonecutters separately equip the two tools; Khildarak shop stock is a guarded alternative. Shoes continue to a separate Siege Master story |
| [141 / Masha 97006](../../../areas/qst/icecrag.qst#L141) | I16025 + I15252 + I315 + I303 + I11519 + I6551 + I66058 → I97110 + three separately indexed I97136; D0 | Excluded incomplete recipe. Missing 6551; fox pelt 16025 and health parchment 66058 contradict food lore. Map and three juicy onions are actual declared rewards, not a published cookbook |
| [180 / priest 97008](../../../areas/qst/icecrag.qst#L180) | I97137 + I97138 + I97149 → I97139; D1 | Story: three distinct numbered pages, not three same-name copies. Tubby merchant / quiet female guest / foyer cleaner are their declared source placements |
| [226 / servant 97010](../../../areas/qst/icecrag.qst#L226) | C800 → two separately indexed I97146; D1 | Paid provision service, guarded. Two milk casks still belong to one offering |
| [309 / guest 97014](../../../areas/qst/icecrag.qst#L309) | two I90017 + two I92048 → C75000 + E30000 + I97141; D1 | Story: four physical bottles of two exact kinds. Foggy Woods crate declares two white bottles; Undermountain crate's elven bottle has a world cap of one. Dialogue's timely bonus has no native deadline predicate |
| [365 / sergeant 97020](../../../areas/qst/icecrag.qst#L365) | I97041 + I97047 + I97048 + C25000 → I97143 + I97145; D1 | Guarded story: helping a freezing NPC, rather than a generic sale. Exact hat / jacket / pants; garden attendants and off-duty guards equip them; Alatorin closet is another source. No banquet-start or frostbite-effect terminal |
| [429 / commander 97021](../../../areas/qst/icecrag.qst#L429) | I97006 → I97144; D1 | Story: accepted hidden borrowed-book kind, not identically named I97004. Archivist and deranged man carry the accepted kind; latter can wander to the distant Rock |
| [483 / Viscount 97023](../../../areas/qst/icecrag.qst#L483) | I97115 → I97135; D1 | Story: ordinary kitchen onion, not Masha's juicy I97136. Counter source plus Masha's command interference are separate from the terminal |
| [539 / Siege Master 97029](../../../areas/qst/icecrag.qst#L539) | I97029 → I97140; D1 | Story: exact artist-supplied calf-skin shoes. Earlier artist receipt is optional; supplied loose shoes work without it |
| [589 / Myrke 97039](../../../areas/qst/icecrag.qst#L589) | I97016 + I97017 → C250000 + I55032; D0 | Story: two different named hearts. Captain 97035 carries I97016; Strife 97036 carries I97017. Ordinary CARVE kind 8 is not a substitute; accepted delivery does not prove personal kills |

Current materials are optional evidence, not history. A reward that is worn,
inside a container, consumed, transferred or otherwise unavailable cannot be
reconstructed from its earlier receipt. All exact simultaneous quantities must
remain loose, separately allocated roots. An accepted offering records one
outcome despite duplicate rewards, coins, XP, source actions or a recipient's
departure. Do not add an achievement for each keyword, material, reward or NPC.

## Access, sources and encounter episodes

The front plaque 97111 at room 97026 supplies the Frostmaiden clue. North/south
exits use key `-2`, begin locked, and have asymmetric final keyword tokens.
The ordinary [speech-door implementation](../../../src/cmd/actcomm.c#L186)
checks the last keyword of a locked door after admitted speech. SAY Auril on the
outside clears lock bits on both matching sides; it does not open the doors or
move the player. Another player can unlock them first, so self-unlock, shared
access, OPEN and confirmed arrival need distinct evidence. The internal return
password is not a new prerequisite to enforce in the story journal.

The guard walk uses 97005 and garden doors use 97008; actual D resets can leave
the two sides closed/locked differently. The treasury key 97001, hidden golden
key 97009, hidden notched key 97000, matte-black commander key 97002, red/green
keys 97127/97128 and Frost of Calamity 97011 solve different barriers. Do not
infer access from a key's name or current possession. The captain's key sheath
97107 at 97282 is hidden, closed, locked and pickproof (container flags 29), and
requires tiny silver 97108 equipped by Mahdrel. Its notched key is nested inside.
The thick black key 97010 carried by Cision has a 50% break rate; it opens the
hidden iron chest 97147 whose flags are 15, distinct from the sheath's pickproof
configuration. The shroud and Frost of Calamity lie in the ice vault, along with
an epic rune node, spell pool, memory proof and other treasure. None adds an
invented local relic campaign terminal.

Pedestal 97026 in the Court and orb 97133 in the ice vault are type-25 teleport
objects. Their actual verb is **RUB** (CMD_RUB 259), unlimited charges -1, targets
97026 and 97127 respectively. The generic [teleport execution](../../../src/magic/spell_travel.c#L932)
resolves the selected object and its command, then delegates movement; its
boolean return alone is not proof of arrival. Imported node/pool effects and
Myrke's bodyrush continuation belong to their own service or foreign owner.

[Masha's procedure](../../../src/specs/specs.winterhaven.c#L5257) intercepts GET
when awake and not fighting, using `onion` or `all` argument matching; it attempts
to bash the player and consumes the command. It does not inspect the selected
onion UID, container or full custody operation. Record actual attempted control,
accepted interference and successful item recovery independently. Do not invent
a mandatory kill or publish an alias bypass as a designed quest solution.

[Bodyguards](../../../src/specs/specs.winterhaven.c#L5538) rescue their respective
Viscount, Siege Master or priest during periodic combat. This is an NPC defense
action, not a player rescue. [The Archivist](../../../src/specs/specs.winterhaven.c#L5563)
becomes 97054 while fighting, transfers carried/equipped items, and extracts his
old form. The assistant conversion scans the **global** character list for
97031/97032, creates hunting 97055 wolves, transfers gear and records targets.
There is no zone filter. Qualify actual actor generation, publication, transfer
acknowledgements, extraction, opponent/owner and pursuit scope; stopping the old
fight before allocation can fail is a distinct failure concern, not proof of a
completed transformation or defeat.

[Malice's death procedure](../../../src/specs/specs.winterhaven.c#L5734) creates
97056 vapor and transfers possessions. The first form's death does not provide
the ordinary final corpse path or prove the vapor's defeat, golden-key recovery,
or player credit. Its cleric-targeting behavior is separate from quest admission.
Object 97118's [worn hiding procedure](../../../src/specs/specs.artifacts.c#L73)
uses exact SAY hide, cooldown and water exclusions. It is not assigned to Myrke:
the selected active producer is Bram Burns's 30%-chance Ravenloft follower load.
Its effect does not become a local cloak acquisition or stealth achievement.

Rare and guest distributor rooms are real NPC supply routes, not intended player
entrances. Their prose odds are not the actual random-walk acceptance probability:
review each available direction, no-mob/death/holding flags, speed/state and actual
movement predicate. Myrke starts at 97005, Strife at 97131, guests at 97142, and
the deranged man at 97153. Hunting, stationary behavior, surviving carried stock,
chance and global caps matter. A singleton D1 recipient's retirement must actually
settle before a future reset can publish a new eligible episode.

## Foreign ownership and incomplete plot leads

Artist tools can be recovered from Ghore; its shop offers the rat ingredient.
Khildarak's tools shop has restrictions, and Ashrumite's four shops list parchment;
their buy/sell paths remain guarded under active accounting. Fox pelts have four
Pine Hollow source placements and compete with Darlene's three-pelt service.
Chef salad/clams have prototypes but no selected active producer; 6551 has none.
Food dialogue naming seven cities therefore cannot dynamically establish the
current seven-kind recipe or its intended replacements.

The Viscount's root vial and local dusty ampoule are consumed with King Rodev's
Sarmiz vial in a foreign three-kind recipe. Myrke's bodyrush has a Winterhaven
consumer. The ethereal shroud is one of Air's nine relic ingredients; it does not
complete the theft campaign merely by being found locally. Six Alatorin forge
rows produce local armor kinds but remain foreign paid services. Masha's map and
juicy onions never provide an established local cookbook or Viscount chain.

The frozen garden, banquet delay, construction crew, Cision's trophy displays,
preserved older Icesses, shroud theft, Myrke's cloak/ship lead and personal
revenge supply coherent story leads. Builders can author campaigns from them,
but there is no accepted banquet-start, construction finish, revived victim,
new Icess, escort, cloak return or lore investigation result in this source.

## Balanced repair proposals and news distinction

| Finding / confidence | Current evidence and practical limit | Separate implementation / proof plan |
| --- | --- | --- |
| Masha's incomplete recipe / confirmed | Missing 6551 makes ordinary exact completion impossible; fox pelt and parchment contradict cuisine dialogue. Exclusion prevents journal/daily credit but leaves native data intact | Builder chooses intended seven food kinds or retires/reframes the project. Preserve exact counts and three separate onion rewards unless deliberately redesigned. Add original-fails/repaired-passes prototype/source/mortal turn-in/settlement proof; separately name the native fix and news sentence |
| Onion/book/page ambiguity / confirmed distinction, intent unresolved | 97115 and 97136 differ in name, fullness and quest flags; only the former is accepted. 97004/97006 have identical names; three notes have identical names but numbered contents | Do not merge kinds or invent Masha→Viscount continuation. Builder chooses clearer names/copy or an intentional extra recipe. Verify unchanged intended acceptance, exact-kind negative cases and surviving-instance limits |
| Elven-wine quantity / confirmed fresh-reset mismatch | Q needs two 92048; selected active P has cap one. Existing admitted or supplied stock can differ from a fresh reset | Qualify world counts and actual source episode before selecting cap/source changes. Test two-copy acquisition, no unintended farming/balance change and daily availability. Journal currently states the limitation |
| Barracks downward exit / confirmed dangling destination | 97085 D5 targets absent 97352; renumbering removes it. Prose promises downstairs/dungeons but active lower level is missing | Builder chooses restoration, replacement or obsolete-path removal; test reciprocal route/flags. Never invent the missing dungeon for journal progress |
| Guardian and follow code / dormant, not a demonstrated live defect | 97126/97242 guardian cases have no actual local assignment; ice_privates2 assignments are commented while reset F works | Decide intended guards before binding. Existing guardian direction uses command numbering; test actual south blocking, wake/fight/charm/home semantics and player access. Do not claim unbound code already prevents the live route |
| Transformation scope/failure / observed behavior, intent requires decision | Assistant conversion is global; old fight is stopped before checked allocation and transfers use existing wrappers | Specify local versus global encounter scope and accepted ownership/lifecycle result. Separately fix a reproduced scope or failure defect; preserve balance, original opponent and loot through retry/recovery |
| Rare-room odds, one-way clues, old corpse/lore / unresolved builder intent | Native topology and ambient text need not establish the numeric chance, rescue or promised finale | Select precise text/topology changes only after observed route evidence; keep incomplete plot design distinct from correctness repairs |

Any actual native repair must have a clearly named separate `fix` commit and a
prominent PR **Zone and quest repairs (news)** entry: trigger, old/new behavior,
original-failing and repaired-passing evidence, balance/persistence limits and a
news-ready sentence. Proposed repairs receive no shipped-fix claim. Earlier
actual Opal, Centaur, Kobold and other repair/news sections remain preserved.

## Capability expansion and qualification matrix

| Capability | IceCrag requirement | Acceptance plan |
| --- | --- | --- |
| Invalid content quarantine | Missing required kind; correct complete classification without invented substitutes | Bind the native recipe explicitly to an exclusion; no story/daily unit, materials or valid receipt may imply repair. Publish builder diagnosis and requalification after a deliberate native fix |
| First source / handoff / transformed custody | Hidden books, repeated pages, carried hearts, source NPC→wolf/vapor, foreign mask | Actual item UID/kind, original NPC/room/container generation, reveal and accepted custody transaction; distinguish first personal recovery, supplied handoff, wearing/nesting, extraction and replacement lineage |
| Learned topics and speech gates | 55 M families; magic-door successful speech versus ASK/TELL clue | Record admitted response identity separately from actual lock-bit mutation, OPEN and arrival. Already-unlocked/shared access gives no invented self-unlock; topic aliases deduplicate without per-keyword achievements |
| Actor control / defense / transformation | Masha GET interference, NPC rescue, global assistant hunting, death replacement | Exact actor/participant/owner/reset/combat episode and operation result. Test rejected/busy/interrupted allocation, gear transfer, extraction and replay, negative targets and failure recovery before credit |
| Exact ALL allocation and frozen settlement | Three different pages, 2+2 bottles, two different hearts; repeated milk/onion rewards; nominal XP and coins | Allocate distinct loose UIDs once; freeze each reward index, credited actor/group policy and entitlement. Partial/replayed/cold item+coin+XP settlement and D1 retirement give one outcome; UI is read-only |
| Atomic mixed-fee support | Winter clothing plus C25000, paid key and milk | Keep current guards. Add admitted wallet+root destruction+reward+retirement transaction with rejection/busy/commit/publish/recovery proof; never treat three ready garments as a paid request |
| Source and recipient renewal | World caps, wandering/holding actors, seven departing story givers, two departing service givers, D0 Myrke | Qualify actual source publication/custody and singleton retirement/reset episode. Potential daily selection must not promise access, fresh stock or a recipient; freeze accounting context for publication and recovery |
| Builder-selected campaigns and visual explanation | Artist→shoes is optional producer history; cuisine incomplete; shroud/banquet/dungeons are leads | Show stages, exact counts, current versus historical material, optional producer receipts and unavailable reasons in existing text UI. Add campaign ALL/ANY branches only after native predicates and accepted events exist |

Focused source, schema/file-loader and projection tests cover all eleven bindings,
quarantine, services, three-kind and four-root readiness, same-name negatives,
supplied shoes, spent optional history, native ownership, one credit per outcome,
replay and cold state. Maintained build, formatting and preservation checks are
required before publication. Synthetic receipts do not qualify real speech,
search, GET, Masha interference, key use, combat/transformation, fees, reward/XP
settlement, retirement/reset, persistence or daily renewal. No accounting
activation, database/server operation, migration, deployment or merge is part
of this checkpoint. Priority 72 is Father Tel's Holy Cloister (`cloister`).

# Kobold Settlement: comprehensive source map

Reviewed October 4, 2026. Zone 14, `kobold`; roadmap priority 65.
**The source map is comprehensive; live gameplay qualification remains pending.**
Active, ready accounting is mandatory for discovery, visible encounters,
journals and new achievement/daily credit. Frozen recovery stays separate.

The [schema-three journal](../../../areas/story/kobold.story.json) explains
all four native exchanges: three supporting services and one guarded spectacles
story. Sixteen contacts retain all sixteen addressed aliases and explain the
ambient, source, guardian and shop roles. Seven optional checks comprise five
current-material checks and two earlier service receipts. Exact supplied items
skip personal source, kill and producer history. Three support exchanges leave
the zone-completion denominator; the spectacles request retains one achievement
candidate, with live payment guarded. None of the four is a daily candidate.
Discovery retains its own achievement.

**Separate actual repair:** [02788c573](https://github.com/Community-Duris/Duris/commit/02788c5738a11963647ceb9b4b344463f4f57704)
(`fix: restore Kobold temple guardians to their actual rooms`) changes seven
native source lines and adds the focused executable regression. Jkyl now checks
altar 1481 and sends westward players into pit 1484. The golems check tomb 1482.
The demon checks pit 1484, searches ledge 1483 through `next_in_room`, and pulls
the selected player into the pit. Room numbers previously pointed one room
ahead; the demon also followed the global character list instead of the ledge
list. This prevented intended guardian behavior at normal reset placements.

**News-ready sentence: “Kobold Settlement's temple guardians now defend their
actual altar, tomb and sacrificial pit, restoring the high priest's imp
summoning and the pit demon's ledge attacks.”** The original source fails the
altar regression. The repaired production functions pass altar barriers/pit
arrival, summon cadence/cap/load failure, tomb escape, demon escape and ledge
selection with unrelated global successors. The maintained server builds and
changed/staged formatting passes. Played combat, difficulty and live movement
remain unqualified. This repair is separate from journal/service classification
and the pending content proposals below.

## Reviewed evidence

- All seventeen [native blocks](../../../areas/qst/kobold.qst): thirteen M,
  four Q. Six addressed response families at Szxvu contain sixteen aliases.
  Seven `qc_action` messages belong to the stronad golem, miners and surveyors;
  these are ambient, without addressed acceptance or player material issuance.
- All 147 consecutive [rooms](../../../areas/wld/kobold.wld), 1400–1546:
  116 exact prose groups, twenty-six headers, forty-three non-exit metadata
  groups, ninety-two numeric exit families and 158 text/keyword pairs. Full
  descriptions, properties and every membership were reviewed. No F or C
  property declares a chance fall or current. Vertical/pit danger still exists
  through ordinary movement and the guardian procedures.
- All fifty-eight [mobiles](../../../areas/mob/kobold.mob), 1400–1457, and
  sixty-six [objects](../../../areas/obj/kobold.obj), 1400–1465, including full
  flags, values, equipment effects and extras. The embedded tilde in a mobile
  description and in Szxvu's cryptic response is not a corrupt field delimiter:
  shared `fread_string` ends at a line's last non-whitespace tilde. No native
  parser/data repair ships for those strings.
- All 303 [resets](../../../areas/zon/kobold.zon): M182/E42/G28/O23/D16/P12,
  201 exact argument families and 150 parent-aware families. Raw resets and
  their complete memberships, parents, caps, equipment and container contents
  were reviewed. Header `1546 2 0 15 25 1` retains reset mode two. Checked local
  targets resolve; positive door keys are 1415 and 1419. Source declarations
  and normal cap checks do not qualify played item availability or renewal.
- All three [shops](../../../areas/shp/kobold.shp): maid 1413 at gathering hall
  1415 (egg/ale/tea), baker 1419 at shop room 1420 (bread), Grumbiter 1430 at
  inn 1443 (egg/ale/tea/bandages). Imported object bodies 283, 358, 363, 368,
  369, 3002, 3021, 3043, 3075, 3100 and 55440 were reviewed. Shops have their
  own publication/stock authority; their goods are not new quest endpoints.
- Nine literal local assignments: five mobile procedures, three switches and
  inn at 1444. The actual inn flag on 1443 also installs the shared inn handler.
  Imported stone 358 adds the epic procedure. Szxvu gains `smith` dynamically
  through the tradeskill table, without needing an ACT_TEACHER flag. No local
  mobile has the actual teacher-registration flag. All five Kobold functions,
  the full switch and smith handlers, their ten forge rows, shared command/
  periodic/death dispatch, quest allocation/order, reset admission, inn,
  magic-door speech, boat and weapon-effect paths were reviewed.
- All three ordinary foreign boundary rooms were reviewed: Surface 622130
  north ↔ 1400 south; Underdark 816080 west ↔ 1542 east; Winterhaven 55402
  southwest → 1402 has no ordinary local reverse. Bounded global native
  producers/consumers, resets, shops, assignments and 713 teleport prototypes
  found no foreign fixed teleport into these physical rooms. The imported
  memory's foreign consumer is ambassador 55272, Winterhaven Q3652. No native
  rod-pieces consumer, local producer or smith-row reassembly endpoint appeared
  in the reviewed closure. This is not an assertion about unreviewed operational
  spawning, administrator issuance or player-crafted variants.

The [generated review index](../../reference/zone-story-audits/kobold.md)
retains the exact contracts, sources and classifications. Raw source and
runtime admission take precedence over dialogue, names and graph suggestions.

## Native progression and current limits

All recipients are Szxvu mobile 1420, reset at armory room **1406**. Mobile
1420 is not shop room 1420. All four exchanges have D0; no native completion
retires Szxvu.

| Native block | Exact terms | Integration |
| --- | --- | --- |
| [Q65](../../../areas/qst/kobold.qst#L65) | Eight I1447 + C10000 → I1448 | Supporting smelt. Eight separate roots; mixed fee guarded. Five normal live copies do not establish a played eight-item collection. |
| [Q52](../../../areas/qst/kobold.qst#L52) | Two I1448 + C170000 → I1451 | Supporting shield commission. Two self-produced blocks need sixteen nuggets and two smelt fees; supplied blocks skip that history. |
| [Q85](../../../areas/qst/kobold.qst#L85) | I1431 → I1431 | Optional gem inspection; same kind does not prove preservation of the original item UID. No zone/daily achievement. |
| [Q93](../../../areas/qst/kobold.qst#L93) | C20000 + I1433 + I1431 → I1432 | Guarded spectacles story. Supplied gems/frames work without an inspection receipt or personal source history; this stays one declared achievement candidate, without a daily candidate. |

Fees are copper amounts: 10000/170000/20000 correspond to 10/170/20 platinum.
The durable offering selector accepts item goals, not coin goals. Coin GIVE
and the legacy route are guarded while accounting is active. Journal material
readiness is informational; it does not certify fee acceptance, reserve money,
perform an exchange or authorize new credit. Frozen historical receipts can
still be projected/recovered without enabling unsupported fresh offerings.

Boot prepends completions and each give goal. Consequently Q93 precedes Q85,
and its runtime give order is gems → frames → coins. With only gems, missing
frames can allow selection to continue to inspection. With gems and frames,
the unsupported coin goal can reject the matching commission before reaching
inspection. Qualify this real overlapping-input behavior before changing
branch selection. Never require the inspection, silently consume gems, fall
back to legacy payment or treat a generic failure as a learned quest stage.

The five nugget declarations are G291/G294/G297/G299/G302 after miners 1441
and leaders 1443 at 1495/1498. Each uses live-copy limit five and chance 100.
`read_object` increments the index count; normal G admission checks that count,
while forced repop differs. Offline saved supplies and their reload can change
the live-count situation. The eight-root request exceeds the normal declared
live reset limit, but this alone does not prove the quest impossible. Qualify
ordinary/forced resets, saved accumulation, reload and consumption before a
builder changes caps, sources or the eight-nugget recipe. No cap repair ships.

Szxvu's separate dynamically bound `forge` command is disabled with accounting
active, including its menu. The ten table choices are 70/57/58/59/48/36/38/37/
39/40: copper, silver, iron and mithril armor, using the tradeskill ore constants
and generated base object 1255. These are not local nugget/block commissions
or rod reassembly. The guarded legacy path validates a choice against smith
table position 11 despite ten menu entries and scans the smith's inventory
where its comment promises the player's inventory. The eleventh choice reaches
the -1 sentinel. Parchment learning returns before its old learning loop; its
comment about unlinked smiths is stale because initialization does bind them.
Record these as guarded legacy qualification/repair work. This checkpoint
does not enable, repair or advertise that legacy crafting path.

## Source, clue, access and guardian routes

The outer iron gate 1462 north ↔ 1468 south and Gwark's house door 1433 north
↔ 1434 south use key 1419 from Gwark, reset at pantry 1436. The house door is
pickproof; the perimeter gate is pickable. The shaman at 1427 carries key 1415
for mound door 1425 north ↔ 1456 south. The mound desk contains note 1439
about the temple word. Pantry trapdoor 1436 down ↔ 1455 up is closed without
a declared key lock. Obtaining a key does not prove use, unlocked state or arrival.

Root switches 1421 at 1463 and 1425 at 1469 take TUG341 and clear the blocked
north/south fence passage reciprocally. Their value[3] is one. Bar switch 1427
at 1449 takes PULL340 and clears east to 1470; its return resets closed without
a lock. All three have existing reciprocal targets. Shared nonsecret switch
handling dereferences that return; this local data does not demonstrate a
missing-return crash. World kind nine masks to a door; reset eight supplies
blocking. World kind four at 1495 down ↔ 1499 up masks to no door, without a
local D reset. Do not infer secret state from names or the unused higher bits.

The pond's downward hole 1424→1445, then tunnels to switch 1449, provide the
temple approach: 1470 east→1471 down→1472 east→1473 down→1474. Static warrior
corpse 1463 at 1476 contains frames 1433 and language journal 1437. This is a
reset container, not evidence of a player's generated corpse, death or rescue.
Statue 1433 at 1478 wears gems 1431 in eyes slot 19. `stone_crumble` creates
container 1438 and transfers inventory, equipment and money. Death dispatch
invokes the special rather than ordinary corpse creation, and ignores its
return. Accepted transfer, exact item lineage, failed pile creation, cash,
extraction, interruption and cold recovery need qualification. The functioning
prototype is not proof of a safe durable custom-death publication in every case.

Altar 1481 east has the bone door to tomb 1482: key -2, final word `i>|uub`;
the reverse has key -1. Note 1439, Jkyl's parchment 1440 and language journal
1437 offer the translation puzzle. Shared SAY checks raw supplied speech,
compares the final keyword, and clears matching reciprocal locks/secret state.
It does not automatically open the door or record language learning. Correct
speech, successful unlocking, opening, entering and overcoming Jkyl's separate
east barrier require distinct evidence.

The corrected Jkyl handler deterministically blocks altar north/south/east
commands when dispatched, sends westward players into pit 1484, and reaches
its periodic combat summoning path. It resets its counter to four; global imp
cap five and chance conditions remain intact. Golems at tomb 1482 can block
ordinary westward escape; the demon at pit 1484 can block up or pull an eligible
nonfighting visible player from ledge 1483 and start combat. Shared dispatch
requires an awake, actionable actor for commands and schedules periodic calls
after setup. A normal room visit is not proof of an accepted pull, escape,
survival, guardian defeat or actor state. This repair restores obstacles and
combat behavior; played difficulty remains unqualified.

Tomb sarcophagus 1435 contains tablet half 1459 and gear; demon 1436 carries
the other tablet 1436. Complete rod 1453 and four same-named pieces 1454–1457
have lore and effects but no reviewed local issuance/reassembly endpoint.
Preserve the distinct piece kinds and do not create an invented rod receipt.
Builder intent must decide the source locations, any proof/allocation policy,
whole-rod outcome, and whether this is a retained legacy story or a new quest.

Imported rune-covered stone 358 also resets in the tomb. Its shared epic
procedure sets its physical zone on a periodic call and has actor, peacefulness,
level, group, busy and zone checks before typed touch settlement. The pending
message or hum is not an accepted epic reward. Reset mode two does not request
the special epic mode-zero reset. Epic touch keeps its own authority; this
journal adds no duplicate zone-story terminal.

Imported memory 55440 continues to Winterhaven's Kobold Settlement ambassador
55272, [Q3652](../../../areas/qst/wh.qst#L3652): I55440 → I55362 + C1000000 +
I55033 (enchantment scroll and shopkeeper token). Prior Winterhaven mapping and
foreign ownership remain intact. `_noquest_` excludes random quest generation,
not this explicit native offering. Local custody does not complete the foreign
request or discover Winterhaven.

Inn 1443 has the actual ROOM_INN flag and Grumbiter. A literal assignment also
makes guard post 1444 handle RENT, independently of an inn flag. Shared inn
handling saves the current room as home after its normal eligibility checks;
it does not require a keeper or recheck ROOM_INN. This is a concrete extra rent
location and a likely stale binding, but removing an existing home/rent option
needs an intended policy and played persistence qualification. No inn repair
ships. BOAT boots 1414 on fisherman 1429 at pond 1461 support ordinary water
movement; their name is not a learned spell or fishing objective. Packed weapon
effects use shared proc/selected-action paths; possessing or reading an item
does not prove a successful targeted spell or permanent effect.

## Remaining implementation and balanced repair plan

| Capability or finding | Next work and acceptance |
| --- | --- |
| Atomic fee, material and reward settlement | Coordinate wallet debit, all item roots, reward creation, actor identity, continuation and replay in one accepted operation. Qualify all three commissions and overlapping gem selection. Keep active guards until this works; do not charge on narrative or advance from current custody. |
| Admitted source versus normal live cap | Qualify eight-nugget accumulation across reset/save/reload, quantity allocation, consumption and renewal. Builder decides a cap/source change only after the current legitimate routes are known; distinguish normal limits from forced repops. |
| Original acquisition, player handoff and custom death lineage | Record actual actor/source/item UID, source generation and committed ownership. Qualify pile contents/cash, exact equipment recovery, failures, extraction and restart. Preserve supplied delivery compatibility and avoid inventing personal kills. |
| Learned clues and successful access | Qualify addressed response/examination, literal punctuation and speech language, selected switch, both exit states, actor barrier, command result and arrival. Add typed objectives only after accepted causal evidence; aliases are explanatory, not one achievement each. |
| Forced travel, combat and escape episode | Record priest/demon actor identity, source/destination, selected target, accepted movement, combat start, failure/death/reset and surviving escape. Integrate alternatives without treating visits, warnings, pulls or guardian absence as success. |
| All-stage campaign and restricted foreign ownership | Keep support receipts optional and exact current materials independent. A future parent view must distinguish all-stage progress, supplied shortcuts and one terminal outcome; ambassador and epic outcomes remain owned by their existing systems. |
| Guarded legacy forge and rod integration | Test smith choice bounds/inventory and disabled parchment learning before a separately reviewed repair or accounting adapter. Builder explicitly maps rod sources/reassembly and piece allocation, or labels retained lore. No inferred rod or forge quest ships. |
| Extra guard-post RENT and wording/effect intent | Confirm intended inn policy, current home behavior and recovery before changing binding. Smith response typos/halfling shop wording and mismatched forge ore/name are intent leads; preserve exact recipes until reviewed. Each actual fix needs its own commit, PR before/after/proof/limits and news sentence. |

The executable guardian regression, source/catalog fixture and actual C++
journal journeys cover the repaired functions, quantities, worn versus loose
materials, optional source history, service exclusion, guarded guidance,
read-only projection, historical receipt replay and cold recovery. Injected
historical paid receipts do not qualify live fees. Full catalog/schema/all-map
loader, accounting gates, server build, formatting, native/prior-map preservation
and documentation links are checked at publication. Actual gathering/handoff,
offerings/rewards, guardian combat/travel, passwords, epic/foreign completion,
rent persistence and daily renewal remain unqualified. No DB/account/server
operation, migration, deployment or merge is part of this source checkpoint.

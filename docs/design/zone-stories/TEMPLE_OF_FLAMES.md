# Temple of Flames: comprehensive story mapping

Priority97 adds a schema3/revision1 [journal](../../../areas/story/temple.story.json):
six independent story cards, nine contacts,42 verified topic aliases and eight
optional current-material rows. Zone183 registry18258–18622 contains323 actual
rooms18300–18622. Active, ready accounting is mandatory for every new discovery,
encounter, journal, achievement and daily credit; frozen recovery remains separate.
Complete source review is distinct from played qualification.

Player guidance uses actual named items and commands. Native identities and
execution evidence remain here. Existing schema3 covers the authored hints and
accepted outcomes; the deeper semantic adapters and repair plans below remain
explicit follow-ups. No native mechanics change in this checkpoint.

## Complete source closure

| Source | Complete review and implications |
| --- | --- |
| [Quests](../../../areas/qst/temple.qst) | All6 Q/33 M dialogue families,42 aliases; full input/output/D/response reviewed. Nine contacts include three lore-only actors. No service or exclusion; item-only inputs with coins or empty rewards remain story/daily candidates. |
| [Rooms](../../../areas/wld/temple.wld) | All323 full records18300–18622:184 prose/35 header/17 metadata families,803 exact exits/241 relative patterns/30 complete exit-text families. Sole boundary18300S↔surface587801N reviewed. Complete directed walk closure identifies mirror voids and melting/staff cluster; ordinary teleport eligibility prevents false inaccessible classification. |
| [Mobiles](../../../areas/mob/temple.mob) | All51 full prose and ID-paired numeric records18300–18350. No imported mobile; unplaced18342 is a statue, distinct from actual king18310. Source/giver flags/classes/placements and non-SENTINEL hallucination reviewed. |
| [Objects](../../../areas/obj/temple.obj) | All53 complete records18300–18353 except18338:types/flags/values/effects/extras. Complete imported epic node358 reviewed. Four locally unplaced18301/18302/18316/18337 are Q products. Locket/container children, open+locked chest,100-percent key break, exact four-item proofs, portals/switches/liquids/potions/traps covered. |
| [Resets](../../../areas/zon/temple.zon) | All346 commands:M206/D52/O41/E33/G12/P2;278 exact/288 custody-context/132 expanded families, allchance100, no F/R. Full placement/condition/cap/argument coverage; mode1. Current M ancestry equals actual receiver. P selects globally matching container, not guaranteed specific instance. Accounting issuance refusal remains. |
| [Custom procedure](../../../src/specs/specs.temple.c) | Entire37-line file and literal18302 temple_illyn assignment reviewed. Awake/nonfighting/periodic random fire advice and grumble only, no player quest event. Commented illyns_sword18302 assignment has no active function producer. No room binding/shop/local table-driven epic trainer or smith found. |
| Shared execution | Relevant [SAY](../../../src/cmd/actcomm.c)/[magic doors, OPEN and keys](../../../src/cmd/actmove.c), [GET/DRINK](../../../src/cmd/actobj.c), [reset D decoding](../../../src/world/db.c), [NPC wander](../../../src/mob/mobact.c), [switches](../../../src/specs/specs.object.c), [portal selector/travel](../../../src/magic/spell_travel.c)/[interpreter dispatch](../../../src/cmd/interp.c), [quest roots, reward continuation and replay](../../../src/world/quest.c), [recursive batch custody](../../../src/item/item_movement_transaction.c), [repository admission](../../../src/flatfile/flatfile_item_repository.c) reviewed. Unchanged shared paths from preceding complete audits remain applicable. |
| Foreign closure | All7 touching recipes:6 local plus Hall child77742/Q223. Full child dialogue/mobile/numeric/placement77911/room/reward77743 and adjacent follow-on recipe reviewed; knife ownership stays foreign. No foreign reset group. All713 active type25 portal declarations scanned; only local mirrors enter local rooms. Full surface587801 boundary and imported358 covered. |

## Exact native contracts

I means item prototype, C copper; D1 retires the native giver. Each row is
independent. Line numbers identify native Q contracts, without imposing a campaign.

| Giver/Q; actual placement | Exact accepted inputs → native result | Journal role |
| --- | --- | --- |
| Illyn18302/Q20;18322 | I18300→I18301+I18302;D1 | Soul of Illyn→Illyn’s Armor/Holy Sword and departure. |
| Lleddraick18310/Q87;18363 | I18309→empty;D1 | Well-crafted dagger→illesarus clue and departure. |
| Sage18325/Q233;18440 | 2×I18322+I18324+I18325→C500000;D0 | Two yellow daggers+green token+blue wooden sword together→500 platinum. |
| Angel18331/Q273;18564 | I18336→2×I18337;D1 | Wedding band→two vials of the angel and departure. |
| Weaponsmaster18336/Q294;18572 | I18307→I18316;D1 | Golden locket→steel key/shack clue and departure. |
| Ommsh18337/Q325;18569 | I18339→empty;D0 | Stonecrusher→smash-hole secret; remains. |

All6 native contracts remain intact, with6 achievements/potential dailies.
Coins and empty reward lists do not turn accepted narrative requests into services.
Supplied matching proofs fit without personal kill, access, dialogue or earlier
receipt. Master→key→chest ring→angel and king→word→angel are useful material and
clue paths, without mandatory personal campaign history. Current reward ownership
cannot establish either receipt. One angel receipt does not become two completions.

## Material sources and custody

Aku18320/M18621 carries soul18300/Gcap1 and imported epic358. His soul proof
is a gem-like cage named soul of Illyn, rather than a generic orb. Illyn’s armor
and sword are quest products. Contortionate man18308/M18343 wears well-crafted
dagger18309/E16cap1 and carries note18310 plus two scroll18311. The note supports
the poisoning story, but is not a second input or a separate cure receipt.

Yellow devil18321/M18409 wears two yellow daggers18322/E16,E17/cap2. Green devil
18323/M18414 carries token18324/Gcap1; blue devil18324/M18420 wields wooden sword
18325/E16cap1. Faerie children at18425 do not carry those proofs. Sage’s globe18328
is unrelated stock. The four required roots are atomic; one dagger cannot occupy
both positions. Held/worn/nested pieces need removal into loose inventory for
preparation and native offering. Source identity/own recovery is not inferred.

First maiden18307/M18337 wears locket18307/E3cap1; second maiden M18338 does not
declare another. Locket values4/5/0/4 mean CLOSEABLE+CLOSED, unlocked and not
PICKPROOF; extra flags SECRET/NORENT. Photo18312/Pcap1 enters a globally matching
locket, and is also NORENT. Native acceptance tests only the locket prototype.
The durable batch recursively captures actual child UIDs and snapshot topology;
publication extracts the accepted root and contents. Remove anything to keep before
offering. Cold/replay child issuance and destruction remain played qualification.

Chest18315/O18375 contains wedding band18336/Pcap1. Values500/9/18316/500 mean
OPEN+LOCKED, because CONT_CLOSED4 is absent. GET checks CLOSED. Steel key18316
has value1=100, so a successful UNLOCK breaks it. Master gives key, not ring.
Do not silently close the chest or add a personal master prerequisite. Key and
ring are NORENT. Current state, legitimate stock and save lifecycle differ from
historical recorded outcomes. Four native D hand-ins retire recipients, with no
replacement actor automatically created.

## Access, puzzles and hazards

SAY sepulchre at18362, OPEN door, SOUTH reaches king18363. SAY bdkn at18426,
OPEN doors, EAST enters18427. Reverse last word is bkdn/key0; TRUTH bridge
alphabet xokqgbzswumifdharyjlevptnb repeats b/misses c. Full four inscriptions
and ambient “Fire is the key” were reviewed; intended puzzle repair needs a
manual walkthrough, not an inferred replacement cipher. SAY illesarus at18561,
OPEN wall, EAST reaches angel18564. Successful magic words clear reciprocal
lock/secret but leave CLOSED; shared already-open routes need no personal history.

Old well18305 is TRASH scenery. Hidden basin18326DOWN→18341–43 permits SEARCH/
OPEN/DOWN without actual PUSH switch, despite its prose. Closed/secret foliage
18410DOWN→18425 has no lock and uses ordinary search/open. PUSH table18314 in18371
clears basement panel to18373. PULL latch18319 in18405 clears forward UP drain
to18406; SECRET forward target prevents automatic reverse BLOCKED clearance.
Alternate return routes exist. TOUCH wooden dragon statue18331 in18503 operates
18506E→18507, clears BLOCKED but leaves later secret/closed handling. Reverse
18507W resets open. Commands are guidance, without minted operation credit.

STARE mirror18327 at18431 enters18432; STARE backside18329 at18432 returns18431.
Backside has only that object name; extra LOOK alias mirror is not portal selector
name. Both have unlimited value2=−1 and ordinary travel guards. The sage’s void
has no walk entrance but has a real portal. The directed walk graph, even ignoring
all doors, leaves18377–79/18432–40/18576–83 outside the boundary’s walk closure.

Stonecrusher18339 is G/cap1 on hallucination18339/M18578. Melting18576–78 only
receives18583DOWN from the disconnected staff-labelled18579–83 hub. Pool18578E
is hidden one-way outflow to18569, not an entrance. Initial wander rejects SECRET.
However melting room flags33554432(TWILIGHT),sector0 pass native same-zone TELEPORT
and GROUP TELEPORT destination tests:room numbers18258–18622 resolve to actual
zone183 range and do not fall in LIMITED_TELEPORT_ZONE5700–5999. Destination
selection is independent of walk graph; caster/raid/source eligibility still
applies. This is a real possible native access path, not a guaranteed or played
intended puzzle route. “GODS ONLY” is prose, not evidence of a permission bit.

Ommsh “Drink drop” has no bound local semantic producer. Unholy basin18326 at18427
is finite60/60/liquid28, heals evil/harms good; no hammer or teleport outcome.
Angel vial18337 is level50/SPELL_VITALITY56, not teleport. Moat currents30/40/50/40,
fall shafts and fire traps18349/18350 remain hazards. Epic358 is independently
guarded. No ambient keyword, drinking, combat, service or scenery becomes a
quest receipt merely because narrative mentions it.

## Foreign and narrative ownership

Hall little child77742/Q223 at77911 also consumes knife18309 for letter77743,
feeding the adjacent foreign lost-aberrate exchange. Complete child/room/stock/
reward closure was reviewed. One spent knife cannot complete king and child;
foreign journal/discovery remains foreign. There is no foreign reset group or
portal source of the local accepted proofs. Sole ordinary boundary is
[surface587801](../../../areas/wld/surface.wld) NORTH↔18300 SOUTH.

King’s human-form prose/Illyn’s peace/master’s reunion/angel’s help are accepted
response narratives, without a separately instantiated cure, escort, new actor,
released cohort or permanent restored zone. Attendant/Samael/chef provide lore.
All33 dialogue families/42 aliases are hints. Unplaced mobile18342 is a statue,
not a substitute king or source. Commented sword procedure remains inactive;
generic sword effects remain separate. No shrine/globe/well/trap/unused actor
is made into an invented objective.

## Builder requirements and fair repair plans

| ID | Source evidence and fair implementation plan |
| --- | --- |
| **ZSQ-TEMPLE-SPOKEN-ACCESS** | Sepulchre18362S→18363 and throne18561E→18564 use key−2/last keywords sepulchre/illesarus. SAY clears reciprocal LOCKED/SECRET but leaves CLOSED; OPEN then move. Outer18426E→18427 uses bdkn while reverse says bkdn/key0. Shared open routes require no personal earlier receipt. Define successful actor/exit/instance/state/version events only if builders require personal operation; qualify rejection/reset/recovery/party accounting before adding credit. |
| **ZSQ-TEMPLE-MASTER-CHEST-INTENT** | Master18336 Q294 exchanges locket18307 for steel key18316, retires and points toward the shack. Ring18336 is P stock in chest18315/O18375. Chest values500/9/18316/500 mean OPEN+LOCKED:CONT_CLOSED4 is absent; GET tests CLOSED. Key has100-percent break on successful UNLOCK. Establish intended initial/return/relock/scarcity policy before a closed flag or caption repair. Preserve supplied ring access and separate master/angel receipts. Any actual repair requires a separate named fix/news commit. |
| **ZSQ-TEMPLE-LOCKET-SUBTREE** | Locket18307 has flags5(CLOSEABLE1+CLOSED4), unlocked, SECRET/NORENT; E3/cap1 on first maiden18307/M18337. Photo18312 P/cap1 uses a globally matching locket. Native input checks only locket identity. Batch offering recursively captures actual children and snapshot topology and consumes selected root; photo can be consumed without an independent quest requirement. Qualify legitimate root/child UID issuance, global P target, child removal, extra contents, NORENT save/reconnect and restart/replay refusal. Never silently require photo or promise it is preserved. |
| **ZSQ-TEMPLE-HAMMER-ROUTE** | Hallucination18339/M18578 carries stonecrusher18339/cap1. Melting18576–78 has only incoming18583DOWN from a disconnected staff-labelled hub; hidden pool18578E→18569 is outgoing. Both ordinary TELEPORT and GROUP TELEPORT select eligible same-zone rooms, independent of walk graph; melting room flags33554432/sector0 pass destination filters. Thus no walking route is not proof of unavailability. Hallucination wander initially rejects SECRET. “Drink drop” has no custom local producer; basin18326 is finite unholywater28 and angel potion18337 is vitality56, not teleport. Builder must specify intended puzzle/travel/source-renewal policy, then qualify admitted successful travel/custody without weakening PvP/raid/room guards. |
| **ZSQ-TEMPLE-SHARED-CONTROLS** | PUSH table18314 operates basement18371D→18373. PULL latch18319 operates18405U→18406; SECRET target means item_switch clears forward BLOCKED only, not blocked reciprocal DOWN. TOUCH statue18331 at18503 clears BLOCKED on18506E→18507 but leaves SECRET/CLOSED handling. Mirror18327 STARE→18432 and backside18329 STARE→18431 are real unlimited-value portals. Preserve shared/already-open paths, alternate drain return, exact command selection and hazards; define personal successful-control evidence deliberately. |
| **ZSQ-TEMPLE-CLUE-INTENT** | TRUTH bridge alphabet xokqgbzswumifdharyjlevptnb repeats b/misses c; Illyn periodic “Fire is the key” is ambient, not a SAY trigger. Forward bdkn/reverse bkdn needs an intended puzzle walkthrough. Well18305 is TRASH; basin door permits SEARCH/OPEN/DOWN while room PUSH prose has no bound switch. Determine intended clues before caption/alphabet/password repairs. Add original-fails/repaired-passes command/return matrices and exact-byte scope; no inferred cipher correction, new quest item or disabled procedure activation. |
| **ZSQ-TEMPLE-NARRATIVE-EPISODES** | Four D outcomes retire Illyn/king/angel/master. King response narrates human form; Illyn peace/master reunion/angel aid do not instantiate replacement actors, escort, permanent cure, released cohort or whole-zone liberation. Samael/attendant/chef lore and33 ASK families are hints. Builders must choose actual episode actors/transitions/end states, group/reset/recovery policy and admitted accounting events before achievements for rescuing or curing. |
| **ZSQ-TEMPLE-BUNDLES-REWARD-RECOVERY** | Sage233 requires two distinct yellow-dagger18322 roots plus token18324+sword18325; givesC500000 and stays. Matching loose supplied proofs fit without own kills. Angel273 returns two separate18337 rewards for one accepted ring receipt; duplicate reward ordinals have separate source identities. King87/Ommsh325 have empty reward lists, still real story outcomes. Qualify four-root atomic refusal/consumption, wallet and duplicate-item continuation recovery; reward count/possession must never multiply or identify completions. |
| **ZSQ-TEMPLE-SOURCE-DAILY-RENEWAL** | Mode1/cap1 sources and four retiring recipients govern current availability. Source matching does not guarantee reset issuance, available giver or daily replenishment. Active accounting item-reset refusal remains. Soul18300/key18316/locket18307/ring18336/child photo18312 NORENT and custody require played qualification. Keep finite holy liquids/current stock, hazards and native chances. Admit intended lifecycle/generation identity before daily source promises. |
| **ZSQ-TEMPLE-FOREIGN-OWNERSHIP** | Knife18309 also goes to Hall child77742/Q223 at77911 for letter77743, feeding that zone’s own subsequent chain. Consumed copy cannot complete both; foreign receipts/discovery remain foreign. Imported358 is a separately guarded epic node. Unplaced statue18342 and reward-only18301/18302/18316/18337 are not extra local quests/sources. Ambient temple_illyn and commented sword binding supply no accepted outcomes. Preserve registry/boundary and service/unused intent. |


These ten source-grounded requirements expand the universal plan. Each needs
explicit builder intent and admitted accounting evidence before deeper personal
progress is credited. Proposed native repairs remain separately named fix/news
work, with original-fails/repaired-passes and exact scope; no access/PvP/scarcity
mechanic is weakened as part of journal authoring.

## Validation and checkpoint

Focused source/schema assertions and C++ journeys cover all6 contracts, two actual
dagger roots/four materials, held/nested/wrong proof rejection, optional supplied
ring without master/king history, independent master/angel/knife/hammer outcomes,
empty rewards/coins/two vials, current supply versus recorded/spent proof, replay,
cold recovery, raw6→authored6 historical compatibility and foreign isolation.
Full production catalog/inventory/audit regression, all114 Python/C++ journal
journeys, maintained server build and changed/staged format/preservation checks
are required before publication. These are source/projection checks, not played
source/wallet/child/reset/episode/travel qualification.

Catalog114 journals/1571 achievements/1438 potential dailies/2192 rows; native2668
definitions/fingerprint/revision2/registry and earlier113 mappings unchanged.
Roadmap97/220 source-comprehensive,123 pending; Pharr Valley Swamp next. No native
repair, accounting activation, DB/migration/server/deploy/merge is performed.
Full220-zone goal remains active.

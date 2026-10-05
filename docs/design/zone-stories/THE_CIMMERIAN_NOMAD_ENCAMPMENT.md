# The Cimmerian Nomad Encampment: comprehensive story mapping

Priority 90 closes source review for zone 62, registry 6122–6248, source area
`nomads`, reset mode 2. The [schema 3/revision 1 sidecar](../../../areas/story/nomads.story.json)
classifies both native QA recipes as two linked cards, with seven contacts,
five addressed aliases and six optional checks: five exact materials and one
earlier evidence receipt. The [generated audit](../../reference/zone-story-audits/nomads.md)
records exact bindings. **No native zone or quest repair ships.**

Every new discovery, encounter, journal, achievement and daily credit requires
active, ready accounting. Frozen reward recovery remains separate. Existing
source scarcity, item flags/types, pickup, doors, mobility and PvP are preserved.
Source-comprehensive mapping does not qualify played transactions or renewal.

## Complete source closure

| Source | Complete review and dispatch implications |
| --- | --- |
| [Quests](../../../areas/qst/nomads.qst) | Five blocks: two M, one MA and two QA; three addressed response families/five aliases. Full purpose, work, fate, evidence, collateral, trophies, reward and departure messages read. Both recipes consume all exact listed item roots. A means echoAll, not a separate ALL-goals opcode or room-wide award. No qc_action. |
| [Rooms](../../../areas/wld/nomads.wld) | All 49 physical rooms, 6200–6248; 36 full title/prose families, five headers, 124 exact exits, 31 relative patterns, two complete exit texts and four complete non-exit metadata families (ordinary S plus bucket, bedding and fire descriptions). All destinations resolve. Registry starts below physical rooms; no invented lower-range rooms. |
| [Mobiles](../../../areas/mob/nomads.mob) | All 21 full prototypes, 6200–6220, with 21 complete prose families and every numeric tail. All placed in M commands. Wanderer, horses, toddlers, youths, warriors, elders, recovering adults, blacksmith, two creatures, two strangers, wife and Septimus reviewed. No ACT_TEACHER/epic-teacher binding. |
| [Objects](../../../areas/obj/nomads.obj) | All 23 full prototypes, 6200–6222; complete flags/values/effects/extra descriptions including both journal pages. Shards and heads are exact TREASURE8/QUEST-marked hidden proofs. Heads additionally TRANSIENT/NORENT. Ring/crown are armor; ring is finger-wear. Hidden journal is SPELLBOOK33 with 108-page capacity, not spell108. Rellius is a fixed hidden CONTAINER15, not ITEM_CORPSE. No local KEY18/SWITCH29/type25 portal. |
| [Resets](../../../areas/zon/nomads.zon) | All 102 commands: 55 M, 22 E, 16 D, four O, four G and one P; 90 exact/95 M-parent-aware families and 60 expanded groups. Four cap1 G proofs bind exact holders/rooms. Journal/flask/sword/body floor declarations and body’s P sword retained. All chances are 100; caps and conditional chains still matter. |
| Shared execution | [Quest admission/echo/retirement](../../../src/world/quest.c), [hidden search](../../../src/cmd/actobj.c), [LOOK/READ/spellbook presentation](../../../src/cmd/actinf.c), [spellbook encoding](../../../src/classes/memorize.c), [WAKE](../../../src/cmd/actmove.c), [loader/reset](../../../src/world/db.c), [transient floor decay](../../../src/world/handler.c), [NPC corpse transfer](../../../src/combat/fight.c), [snapshot NORENT custody](../../../src/player/player_snapshot_capture.c) and [mode2 renewal](../../../src/world/events.c) reviewed. |
| Global closure | Both recipes touching local objects are local; no foreign reset group/consumer or imported proof. Four reciprocal Surface boundaries have complete foreign records read. Across 713 active type25 prototypes none targets local rooms. No local shop file/record or literal special assignment. Direct code leads: [Githzerai prime shift](../../../src/classes/innates.c) can choose bonfire6224 from the astral source, and [CHAOS starter profiles](../../../src/account/chaos_eq_data.h) use crown6222 through the [durable starter grant](../../../src/account/nanny.c). Economic limit6144 is a count, not a zone hook. |

## Actual progression and material allocation

| QA line | Exact native terms | Journal meaning |
| --- | --- | --- |
| Septimus6220 /48 | I6217+I6218→I6221; no departure | One wood shard plus one iron shard gives the hallowed Cimmerian ring. Tree creature6215 at6208 carries wood; iron creature6218 at6240 carries iron. Current matching supplied materials fit without personal kills, reading or waking history. |
| Septimus6220 /64 | I6219+I6220+I6221→I6222; D departure | Two different heads plus the exact collateral ring give the crown and narrate the camp’s relief. Shaman6216 at6208 carries head6219; conjurer6217 at6240 carries head6220. Both heads use keyword head, but duplicates of one cannot replace the other. A matching supplied ring fits without personal first-exchange history. |

Active durable admission searches the actor’s current loose carrying list for
all exact roots, rejects duplicate selected pointers and submits one batch to
destruction. Have the complete bundle loose together, then offer a matching
item; do not describe independent instalments or persistent NPC deposits. Worn
or nested ring does not fit that current bundle. Ordinary blue/green camp rings,
wooden swords, generic carved heads and Rellius’s body are different prototypes.
Earlier evidence history is optional; it never supplies a ring consumed in the
final exchange, lost, sold or given away. Actual repeated final completion
requires a fresh ring and both heads; the first receipt is not reusable material.

The first exchange is the only identified native producer of ring6221. No ring
or crown reset is placed on Septimus, despite his narrative handing over a crown.
CHAOS kits can independently issue crown6222 through an admitted starter grant,
so crown possession cannot prove the quest. Keep starter/source provenance and
accepted quest receipts distinct; do not remove that existing alternate source.

The native hand-in checks exact materials, not either stranger’s current death
state, personal killer, Rellius discovery, creature liberation or camp relocation.
Heads are already carried by the living strangers. These are neither CARVE
outputs nor proof of a personal kill by their names alone. Future builder intent
may choose material-only trophies or a real kill prerequisite; imposing the
latter changes gameplay and requires deliberate versioning and separate review.
Room-wide QA narration does not automatically credit every listener; use the
admitted actor/eligible credit context. D extracts Septimus, not the whole camp.
Current quester/completion/room target reselection lacks captured NPC instance/
epoch, so delayed settlement versus replacement still needs qualification.

## Investigation, hidden sources and supporting stories

| Fact | Honest explanation and evidence boundary |
| --- | --- |
| Five keywords | Wanderer purpose/cimmerian, blacksmith work, Septimus fate/future select three responses. MA broadcasts Septimus’s response to the room. Native encountered records an eligible encounter, not learned aliases, an investigation result or a completed exchange. |
| Sleeping blacksmith | Old4/8 loads prone/sleeping; quester refuses while target status is sleeping. Native WAKE can raise an ordinary sleeper to resting, with visibility, actor state and sleep/knockout/trance admission. This makes the work clue available under existing rules, not a mandatory new quest stage. Shaman also starts asleep; that is not a wake-quest endpoint. |
| Rellius’s pages | Hidden journal6215 O at6236; two E records page1/page2 explain newcomers and his planned confrontation. SEARCH reveal, visible LOOK/READ output, current possession and learned-page evidence are different. do_read delegates to LOOK; extra-description resolution can select visible room/equipped/carried/floor sources. No page-specific story fact is recorded. Tie future evidence to selected journal UID/page/content revision and actual admitted output. |
| Spellbook and fixed body | Journal type33’s value2=108 is page capacity; no encoded spell description is authored, so generic spellbook inspection can say unused while page1/page2 contain lore. It is not an automatic fire-shield lesson. Body6216 is hidden, TAKE0/weight165 CONTAINER15 at6240, with P sword6213; no combat corpse, resurrection, corpse recovery or pickup objective. P uses global matching-container lookup, not guaranteed current authored custody. |
| Hidden/transient trophies | Four G proofs all have SECRET; heads also TRANSIENT/NORENT. Native NPC death transfers carrying into its corpse, while no-corpse handling can put contents on the floor. SEARCH may reveal actual hidden contents; GET/gifts, corpse extraction and floor drop differ. Floor placement arms transient decay. Non-death extraction can discard transient inventory. Preserve these rules, not convert proofs into permanent/free objects. |
| Persistence | NORENT alone does not prove an accounting-owned head vanishes on logout. Snapshot capture includes active durable custody despite legacy omit_norent. Qualify actual save/recovery, transient destruction, corpse release/raise and custody conflict paths; do not promise retention or loss from the flag alone. |
| Services and prophecy | Ancient teacher, training swords/mats, infirmary, vanished weapon production, bound creatures, horses and future tribe/Zalkapfaan battle have no additional local accepted lesson/craft/healing/liberation/escort/prophecy endpoint. A potion is loot (spirit armor192/lesser mending234), not a service transaction. Hallowed gear’s short-lived lore is not a new timer/controller; preserve actual flags. |
| Bonfire and access | Fire extra description says logs do not burn away; room6224 is MAGIC_LIGHT with CITY sector1, not elemental-fire terrain. The sixteen doors reset closed/unlocked, raw kind1/key0; OPEN and admitted arrival are separate. Four Surface links exist; local endless-plains prose versus desert/dune boundaries is an intent question. Githzerai prime shift’s random destination is existing travel, not a guaranteed route or new mobility grant. |

All zone item reset issuance is refused while accounting is active, before
read_object; source declarations do not guarantee live shards, heads, book,
body or restock. Both exchanges remain potential native daily candidates.
Mode2 can reset when aged even with players present; caps, existing holders,
spent materials and Septimus’s departure still require actual renewal proof.

## Fair findings and required capability work

| Required ID | Finding, plan and qualification |
| --- | --- |
| **ZSQ-NOMADS-SOURCE-RENEWAL** | Cap1 hidden G proofs and sole collateral producer need qualified source/root/UID/custody, source versus gifts/rewards, actual mode2 renewal and repeated full bundles. Zone item issuance is refused under active accounting. Daily availability must depend on legitimate current materials or qualified producers/renewal. Preserve scarcity; no free stock or quest activation workaround. |
| **ZSQ-NOMADS-HIDDEN-TRANSIENT-PROOF** | Hidden heads exist on living NPCs, use the same alias and carry decay/no-rent flags. Distinguish NPC source/loot/GET/gift/steal/SEARCH from personal kill/CARVE and current possession. Qualify corpse versus floor/no-corpse/non-death extraction, owned transient DROP/destruction, save/recovery, duplicates and root drift. Builder chooses whether accepted items alone or actual admitted death is intended before any kill requirement or flag repair. |
| **ZSQ-NOMADS-COLLATERAL-ALLOCATION** | Evidence gives a wearable ring consumed with two heads; earlier receipt cannot supply it. Qualify loose versus worn/nested roots, exact distinct heads, complete atomic batch, supplied ring without own first stage, concurrent attempts/retry/rollback and fresh-ring repeat. No instalment/deposit or receipt-as-material workaround; preserve both independent outcomes and receipt compatibility. |
| **ZSQ-NOMADS-INVESTIGATION-EVIDENCE** | Native waking, page inspection and aliases are clues, not learned story facts. Universal optional facts need admitted actor/selected NPC instance or journal UID, exact page/content revision, visibility/current status, accepted output and accounting readiness. Qualify ordinary versus magically blocked wake, prior/other-player waking, denied/hidden/wrong-book pages, read alias versus LOOK, replay/cold recovery. Keep optional unless builder explicitly defines a native prerequisite. |
| **ZSQ-NOMADS-RECIPIENT-RETIREMENT** | D removes Septimus; template/room reselection has no captured NPC instance/epoch. Qualify mode2 replacement while occupied, delayed settlement/removal/disconnect/replay and original-target retirement. Preserve room listeners versus eligible actor/credit recipients; narration does not confer extra rescue/camp-movement credit. A confirmed shared repair gets separate named fix/tests/news. |
| **ZSQ-NOMADS-SERVICE-PROP-INTENT** | Journal spellbook presentation versus lore pages, hidden fixed body, immortal gear and services need builder intent. Prefer truthful inspection guidance; no body pickup/type conversion, automatic spell lesson, crafting or creature liberation from names. Any real endpoint must define prerequisites/actor/outcome/source/rewards and qualify accounting; actual data repairs separate named fixes with original-fails/repaired-passes proof and news. |
| **ZSQ-NOMADS-BOUNDARY-CLUE-CONSISTENCY** | Local plains prose differs from actual Surface deserts; prime-shift can independently enter; diary/prophecy/copy spelling merit review. Capture accepted physical arrival and source provenance, not travel intent or neighbouring Surface discovery. Confirm intended geography and smallest truthful wording before edits. Preserve entrances, existing travel and PvP; actual repair separate commit/proof/news. |

Preserve [Fields Between’s owner-confirmed legacy-rift hotfix and required
replacement design](FIELDS_BETWEEN.md). No intentional restriction, hidden flag,
transient lifetime, price, item type, lock, source or mobility is repaired here.

## Validation and limits

Source/schema checks protect both QA bindings/echo semantics, all source families,
exact four holders, hidden/transient flags, ring source/allocation, current ALL
roots, sleeping clue admission, page descriptions/book capacity/body identity,
closed doors, four boundaries/prime-shift, CHAOS crown source, snapshot custody
and active reset refusal. Existing Python/C++ journeys check exact distinct
materials, worn versus loose ring, ordinary rings/body/sword substitutions,
clues or crown without receipts, supplied final bundle without own first stage,
spent collateral versus earlier history, both independent accepted exchanges,
replay and cold recovery without Surface discovery.

Catalog:108 journals/1583 achievement units/1441 potential dailies/2193 story
rows. All2668 native definitions, fingerprint/content revision2/registry and
107 earlier journals remain unchanged. Required checks:full production regression,
maintained build, changed/staged formatting, links/whitespace, original220 queue,
exact old PR archive and repair/news preservation/publication proof.

Synthetic receipts do not qualify played awakening/page reading/SEARCH/GET/
source/gifts/steal/death/looting/CARVE/transient decay/save/plane shift/OPEN/
native exact batches/reward settlement/selected retirement/actual renewal or
database persistence. No accounting activation, DB/server operation, migration,
deployment or merge; frozen recovery remains separate.

# The Ruins of Undermountain: comprehensive story mapping

Priority 91 closes source review for zone920, registry91168–92519, area
`undermountain`, reset mode1. The [schema3/revision1 sidecar](../../../areas/story/undermountain.story.json)
classifies both native hand-ins as two linked cards, three contacts, eight
addressed aliases and three optional checks: intact key, earlier Tamsil receipt,
and current note. The [generated audit](../../reference/zone-story-audits/undermountain.md)
preserves exact contracts. **No native zone or quest repair ships.**

Every new discovery, encounter, journal, achievement and daily credit requires
active, ready accounting. Frozen reward recovery remains separate. Preserve
scarcity, hidden/invisible flags, key breakage, deliberate disabled controllers,
rare-monster distribution, locks, fixed portals and PvP. Source-comprehensive
mapping is not played source, transaction, retirement or renewal qualification.

## Full source and dispatch closure

| Source | Complete review and implications |
| --- | --- |
| [Quests](../../../areas/qst/undermountain.qst) | All five blocks: three M responses/eight aliases, one Q and one QA. Full daughter/prison/bravery/departure messages reviewed. Both inputs are exact items; no qc_action, personal death, reading or unlock prerequisite. QA A means room echo, not listener credit. |
| [Rooms](../../../areas/wld/undermountain.wld) | All441 physical records,92001–92519 with gaps;332 full prose families,16 headers,23 complete non-exit metadata families,1059 exact exits,354 relative patterns and201 full exit-text families. All destinations resolve. Lower registry bound does not invent additional physical rooms. Full pillars/messages, library, coffins, traps, blood, prison, snowy forest, dungeon occupants and concealed controls reviewed. |
| [Mobiles](../../../areas/mob/undermountain.mob) | All95 prototypes92000–92094,94 full prose families and every numeric tail. Nine have no local reset:92003–06 inn family/patrons,92020–22 adventurers,92054 and92092. No imported mobile reset. Unplaced or dormant NPCs are not promised contacts. |
| [Objects](../../../areas/obj/undermountain.obj) | All135 complete prototypes92000–92134, flags/values/effects/traps/descriptions. Eighteen have no local placement:92032,92035,92036,92054,92063,92064,92071,92075,92085,92089–91,92112,92114,92120,92121,92131,92134. Reward-only note/scimitar and foreign Flame placement are distinct from unreachable stock. Full imported283 sword/359 rune node/364 rations/998 wine barrel reviewed. |
| [Resets](../../../areas/zon/undermountain.zon) | All731 commands:256 M/226 D/131 E/55 O/44 P/11 G/8 F;509 exact,548 M-parent-aware and274 expanded families. Chances100 retain caps/conditional chains. Essra carries cap1 grate key; note/scimitar have no reset producer. Mode1 renewal requires an empty zone; retirement and global stock matter. |
| Boundaries/global sources | Reciprocal92501 south/74045 north links Svalich.92518 down→Underworld4557 is one-way;4557 up leads54841, not back. Both complete foreign records reviewed. Across713 active type25 objects, only two local portals target local rooms. All22 touching foreign reset groups and the foreign quest consumer reviewed: Icecrag consumes two wines92048 plus two venisons90017; Brass Fingers can carry mithril picks92046; FirePlane places Flame92121; Caertannad uses20 extra92051 mobiles. These remain their owning zones' outcomes. |
| Custom dispatch | [Assignment source](../../../src/specs/specs.assign.c) has the entire Undermountain NPC block inside `#if 0`. Six literal mobile assignment leads are inactive; empty-VNUM companions are inactive too. [Full dormant routines](../../../src/specs/specs.undermountain.c) include nine-weapon/inn lore, following adventurers, hiring, Essra narration, dagger/corpse/death transformations and Malodine/black-pudding code. No reactivation. Eight active object bindings:92090 undead trident;92080/81/82/86 sunlight-sensitive drow gear;92065 NPC flindbar disarm;92020 shared parry;92121 Flame's combat event. These are equipment mechanics, not hand-in contracts. Generic type29 switch is assigned by the loader. |
| Shared execution | [Quest consumption/echo/retirement](../../../src/world/quest.c), [key identity/unlock/break](../../../src/cmd/actmove.c), [SEARCH visibility](../../../src/cmd/actobj.c), [NOTE/READ](../../../src/cmd/actinf.c), [WRITE](../../../src/cmd/actcomm.c), [door/reset/item loader](../../../src/world/db.c), [generic switches](../../../src/specs/specs.object.c), [renewal](../../../src/world/events.c) and [shared parry](../../../src/combat/defense_resolution.c) reviewed. Avernus staff can summon prototype92076 elsewhere; [staff controller](../../../src/specs/specs.avernus.c) and staff test-command lead do not establish local arrival or another quest. |

## Actual progression and competing key uses

| Native contract | Accepted outcome | Evidence boundary |
| --- | --- | --- |
| Tamsil92082 /QA28: I92133→I92134; D | Intact grate key gives crude note and retires that Tamsil instance; narration says she disappears through the grate | No implemented escort, physical movement to Durnan, reunion, own Essra kill or personal unlock requirement. Room narration alone does not credit listeners. |
| Durnan92002 /Q13: I92134→I92120 | Exact crude note gives Convalescence scimitar; Durnan remains | A supplied matching note fits without the actor's earlier Tamsil receipt. Receipt cannot restore consumed/lost/gifted note; reward possession does not prove this exchange. |

Durnan loads in passage92042. Tamsil loads in prison92519 below spider shrine
92289. Essra92043 loads in92289 carrying hidden/invisible key92133, cap1.
Upper grate92289 down→92519 is raw kind2, key92133; lower92519 up→92289
is raw kind2, key0. Both D2 reset locked. Loader kind2 is pickable, distinct
from D reset state. Native PICK/KNOCK or a previously accessible route may
avoid consuming the offering key, subject to existing admission/skills.
No new access, portable escape, permanent-open state or free second key ships.

Key value[1]=100 is the percent break roll after ordinary successful keyed
UNLOCK, not durability100 or100 uses. UNLOCK clears/mirrors the door lock and
requests key destruction; durable destruction admission/settlement can reject,
so do not promise that every admitted unlock has already destroyed its key.
`has_key` selects matching prototype from HOLD or loose inventory, while quest
admission needs the offered material loose. If a key breaks or is otherwise
spent opening the grate, Tamsil's exact hand-in needs another intact legitimate
copy. Both competing consumers and empty-mode1 renewal need qualification;
do not silently disable key breakage or add stock to make the quest convenient.

Only identified note producer is Tamsil's accepted exchange. Note92134 is
ITEM_NOTE16/TAKE/QUEST, with no authored action-description letter text or E
payload. Native inspection shows blank when no payload; WRITE can edit an
eligible note without changing its VNUM. Native Q checks exact prototype,
not letter content, writer, reading, first source or rescuer. Small blood note
92018 is OTHER13 with separate lockpick lore, and cannot substitute. Builder
may author truthful text or define provenance, but new proof predicates require
explicit versioning and review. Convalescence92120 is SECRET/MAGIC weapon,
TAKE/WIELD, with PROC flag but no literal object binding. The flag alone does
not create a quest/combat controller. SEARCH scans floor or selected open
container/corpse contents; it cannot reveal an ordinary loose inventory item.
Qualify delivered hidden reward usability before promising the player can wield
it. Preserve flags pending builder intent; no unverified reward repair claimed.

## Supporting exploration, controls and alternate sources

| Source story | Actual behavior and follow-up boundary |
| --- | --- |
| Eight aliases | Durnan hello/hi and daughter/tamsil; Tamsil hello/hi/help/prison. Three replies are clue output; encountered eligibility does not record every learned keyword. Essra is a source contact with no local dialogue block. |
| Hidden invisible key | SECRET4096+INVISIBLE8, KEY18, TAKE1, QUEST. SEARCH temporarily clears SECRET but restores it if CAN_SEE_OBJ fails; it never removes invisibility. Corpse/container reveal, source loot, loose ownership, gift and personal first acquisition are distinct. No personal kill inferred. |
| Fixed portals | Painting92122 O at92278→92508; forest92508 has ordinary SOUTH return to92278. Pool92129 O at92450→92281. Both type25, ENTER7,charges−1,TAKE0. Preserved placement/restrictions; a travel attempt differs from accepted arrival. |
| Native sand switch | Mound92130 O at92068, type29 values270/92068/5: PUSH affects blocked DOWN→92043, D9 closed/blocked. Shared switch clears BLOCKED, not OPEN or arrival; SECRET can remain. Journal does not award a fabricated switch/unlock fact. |
| Unplaced ceiling lever | Hidden fixed lever92131 values340/92005/4: PULL targets92005 UP→92007. No identified placement; route resets D8 open/blocked, reciprocal DOWN resets open. Do not bind/place the lever or unblock route as an assumed repair. Builder must choose intended access. |
| Deliberate rarity | Distribution92029 has three routes into no-exit holding92033 and one into playable repop distribution92197. Prose explicitly gives25% playable versus75% held rarity. The holding room is intentional; do not add escape exits or relocate rare bosses. |
| Coffin and lockpick story | Fixed trapped platinum coffin92030 holds P paladin container92031 and blade92124. Thief prop92045 at92326 declares mithril picks92046, blood note92018 and vial92022. Props are CONTAINER15, not ordinary combat corpses. Picks also occur on a foreign Brass Fingers NPC at70%; P uses global container lookup, not assured authored live custody. No new recovery/CARVE/reading quest from lore. |
| Independent epic stone | Imported rune359 binds [epic_stone](../../../src/world/epic.c). Periodic state sets owning zone/payout and removes floor TAKE; TOUCH checks selected instance, magic/current completion, peaceful room, nonstaff/level/location, participants and durable zone-touch submission. Publication/claim/recovery belong to epic, not either material hand-in. Preserve its existing payout/errand/group rules; touching or owning it does not prove Tamsil or Durnan. |
| Foreign continuation | Wine92048 supports Icecrag's full wine+venison bundle; foreign reset supply and consumer retain their owning journal. Flame92121 is placed in FirePlane, despite local weapon lore. Caertannad's related mob and Avernus summoned shadow show that matching prototype encounter/reward ownership is not local discovery or native quest credit. |
| Dormant controllers | Inn offer/nine weapons, gambling, adventurer following, mercenary money/hiring, monster transformations and NPC dialogue in compiled-out code are unavailable. Some dormant routines allocate before command filtering or reference absent92096/926xx prototypes. These are reactivation design blockers, not active runtime failures or authorization to enable code. |
| Geography/prose | Actual entrance is Svalich; Yawning Portal inn and its unplaced family are lore. Secret book/brick/hooks/vents, weapons and room directions need authored-versus-runtime qualification. Prefer minimal truthful clue corrections after builder confirmation; preserve topology and mobility. |

Active accounting refuses zone item reset issuance before read_object. Declared
cap1 source is not guaranteed live key, painting, pool, loot or renewal. Two
native repeatable outcomes remain potential dailies, subject to legitimate
materials and successful accepted transactions. Mode1 empty-zone renewal and
Tamsil's retirement cannot be inferred from a historical receipt.

## Required builder and capability work

| ID | Finding and fair plan before implementation |
| --- | --- |
| **ZSQ-UNDERMOUNTAIN-SOURCE-RENEWAL** | Qualify source/root/UID/custody, cap1 stock, gifts/loot/rewards, active item-reset refusal and actual empty-mode1 renewal. Preserve scarcity. Daily availability needs real current materials or qualified producer/restock; no free stock or accounting bypass. |
| **ZSQ-UNDERMOUNTAIN-GRATE-KEY-ALLOCATION** | One key is competing unlock/offering material, with100% break roll. Qualify exact held/loose/nested selection, break destruction admission/settlement, reciprocal door state, alternate legitimate access, fresh key, concurrency/retry/rollback and repeated exchanges. Builder decides whether current deliberate break/stock design is intended; any change separate fix/proof/news. |
| **ZSQ-UNDERMOUNTAIN-PROOF-VISIBILITY** | Hidden/invisible source and hidden directly delivered reward can be inaccessible to ordinary selectors. Qualify SEARCH/container/corpse, invisibility, source GET/gift, loose reward inspection/equip/save/recovery. Confirm builder intent before minimal flag/source/presentation repair; do not strip protection merely to satisfy a journal. |
| **ZSQ-UNDERMOUNTAIN-NOTE-PROVENANCE** | Exact note has no authored letter and is writable. Builder chooses informational text versus real signed rescue proof. Optional learned/source facts need admitted actor, itemUID, writer/content revision, selected output, source versus gift and explicit prerequisites. Changing accepted proof semantics needs contract/version/historical receipt compatibility; no fabricated read or personal rescue requirement. |
| **ZSQ-UNDERMOUNTAIN-RESCUE-RETIREMENT** | D retires Tamsil, with no escort/reunion controller; target reselection lacks captured NPC instance/epoch. Qualify original target versus reset/removal/delayed settlement/disconnect/replay, eligible actor/group versus listeners and any real rescue transition. Keep narration truthful; shared repair separate named fix/tests/news. |
| **ZSQ-UNDERMOUNTAIN-ACCESS-EVIDENCE** | PICK/KNOCK/UNLOCK/switch/portal/SEARCH and accepted arrival differ. Universal optional facts need selected door/objectUID, actor, room/dir, prior/accepted state and settlement. Qualify blocked/secret/locked modes, pre-open or other-player access, denied attempts/replay/cold recovery. Unplaced lever and deliberately rare holding rooms remain builder intent questions; preserve current controls/topology/PvP. |
| **ZSQ-UNDERMOUNTAIN-DORMANT-STORY-INTENT** | NPC bindings are compiled out; object combat procedures remain active. Builder chooses retirement/truthful lore or a newly designed controller. Before any reactivation, resolve absent IDs, allocation/event guards, per-instance state, paid hiring/pet lifecycle, source stock/rewards/accounting and balance. No blanket enablement or unqualified monster-drop/inn/weapon-collection credit. |

Preserve [Fields Between's owner-confirmed legacy rift hotfix and required
replacement design](FIELDS_BETWEEN.md). Native repairs must be separate named
fix commits with original-fails/repaired-passes evidence and prominent news.
No pickup/weight, charge, key, hidden flag, source, disabled binding or movement
repair ships here. The source-complete journal remains editable builder guidance.

## Validation and limits

Focused source/schema checks and existing Python/C++ projection journeys cover
both bindings, complete local sources, exact key/note, wrong note/key, current
loose material versus held/nested supply, supplied note without own rescue,
earlier receipt without current note, independent outcomes, replay and cold
recovery without Svalich/Underworld discovery. Full production regression,
maintained build, changed/staged formatting and preservation checks are required
before publishing. Earlier108 journals,2668 definitions/fingerprint/content
revision2/registry, original220 queue and prior repair/news remain unchanged.
Catalog109 journals/1583 achievements/1441 potential dailies/2193 rows.

Synthetic receipts qualify projection, not played hidden source/reveal/GET/gift,
key break/destruction, PICK/KNOCK/OPEN, writing/reading, native offer/reward,
retirement, portal/control, source renewal or database persistence. No accounting
activation, DB/server operation, migration, deployment or merge. Full goal stays
active:91/220 source-comprehensive,129 pending; Desolate Under Fire next.

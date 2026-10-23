# The Desert City of Venan'Trut: comprehensive source map

Reviewed October 4, 2026. Zone490, `desert`; roadmap priority81.
**Source review is comprehensive; played gameplay qualification remains pending.**
Active, ready accounting is mandatory for all new discovery, encounter, journal,
achievement and daily credit. Frozen reward recovery is separate.

The [schema3/revision1 journal](../../../areas/story/desert.story.json) adds
all eight exact native exchanges as eight independent cards,22 contacts/all46
addressed aliases and11 optional checks: ten current materials/access conditions
and one earlier Goranon receipt. Mine rescue, rival signets, compass, hidden
contraband and the Queen → Goranon → Eriic story are explained. Winterhaven
follow-ons, epic claims/teaching, ship/crew services and narrative campaigns
retain their own boundaries. **No native zone or quest repair ships.**

## Reviewed evidence and closure

- All16 [native blocks](../../../areas/qst/desert.qst): MA1/M7/Q8;
  eight addressed families/46 aliases. Full response, acceptance, offering,
  output and D flags were read. Every Q takes one item and gives one item;
  none retires its giver or has a native fee/experience reward.
- All677 [complete rooms](../../../areas/wld/desert.wld),49000–49790
  noncontiguously;158 complete title/prose families,35 headers,1710 exact exits,
  408 expanded relative patterns and16 full exit-text families. All four
  non-exit metadata families were read. Only F records49191/492/493 have
  fall chances75/50/25; no room E/T records. Room49029 is marked unused;
  isolated placeholders and Jussinload49790 are not invented quest routes.
- All245 [mobiles](../../../areas/mob/desert.mob),49000–49244, including
  234 complete keyword/name/long/full-description families and every numeric
  record; all194 [objects](../../../areas/obj/desert.obj),49000–49193,
  including every value, flag, wear/type, E/A metadata and weight. No local
  object T record. `_spec1_`/`_spec2_` keywords select combat specialization,
  rather than implying a quest controller or a teacher binding.
- All1256 [reset commands](../../../areas/zon/desert.zon): D124/O42/P2/
  M814/E168/F25/G81;942 exact argument families/964 M-parent-aware families,
  expanded into485 location-compressed groups for human reading. Actual F
  actor changes matter for subsequent gear: M grouping alone cannot assign
  every follower's item to the leader. Caps, probability, conditional chains,
  two gold-chest children and source-versus-recipient placement were reviewed.
  Header49790 1 8 15 20 2/registry49000–49790 retain reset mode1.
- All ten [shop blocks](../../../areas/shp/desert.shp), full keeper/stock/
  room/hours/types/messages. Merchant49142 both carries and stocks potion49072.
  Tinker49065 stocks the map and three imported bandages. Prophet49215 carries
  and sells steel key49028; drunk sailor49110 also wears it. Goranon's empty
  new stock does not disable his independently registered native Q handler.
- Literal assignments are [Narggle49064 → world_quest](../../../src/specs/specs.assign.c#L793),
  [taproom49051 → crew_shop_proc](../../../src/specs/specs.assign.c#L2404),
  and [merchant dock49090 → ship_shop_proc](../../../src/specs/specs.assign.c#L2419).
  Full shared dispatch/services and relevant execution were reviewed.
  [Computed epic teachers](../../../src/classes/epic_skills.c#L169) bind
  Eriic49161 to Improved Endurance and Ruffus49162 to Totemic Mastery.
  No local ACT_TEACHER actor. [Command dispatch](../../../src/cmd/interp.c#L2816)
  retains a separate `qst_func` after a mobile's special; PRACTICE handling
  does not replace Eriic's item exchange.
- [Flag initialization](../../../src/world/db.c#L1368) binds inn to three
  ROOM_INN rooms49000/49034/49051; later literal room assignment gives49051
  the crew handler. No assumption that a name, retained flag or advertised
  upstairs room proves a working rent service. This shared-handler overlap
  needs builder/service review before any native assignment repair.
- The [type29 switch loader](../../../src/world/db.c#L3166) binds boulder49012
  to [item_switch](../../../src/specs/specs.object.c#L307), independently of
  literal assignments. [Generic teleport execution](../../../src/magic/spell_travel.c#L932)
  was read in full, including target/type/command, destination, charges,
  arena and actual movement. All16 local portals are fixed/unlimited:13
  have local O placements;49082/49084/49087 have none. Two unplaced tent
  prototypes point to missing rooms49681/49682; the third points to existing49338.
- The five imported reset items were read completely: rune-covered stone359,
  bandages363/368/369 and memory55285. Sultan49187 carries stone, memory and
  gold-chest key49165. Stone359 actually binds
  [epic_stone](../../../src/specs/specs.assign.c#L1604), whose full periodic,
  target, absorb, group payout, level/peaceful-room/zone checks and committed
  [TOUCH claim](../../../src/world/epic.c#L1111) were traced. Skill-beacon lore
  in its extra description does not change this binding.
- Bounded global closure inspected all713 active portal prototypes:14 lead
  into local rooms, all from this area's own objects; none is a foreign arrival.
  Thirteen boundary edges include four Surface edges/two complete neighbors
  (Amber Sands545019 and bay545021) and nine Jussinload links to complete Limbo1.
  Nineteen foreign reset groups import local ordinary actors/gear into Alatorin
  and Surface Keeps; actual clansdwarf F parents carry the barbed daggers,
  rather than the diplomatic delegate. These imports do not become local Qs.
- Eleven complete recipe bodies touch local objects/foreign locket: eight
  local, two Shipyard map exchanges and Winterhaven's locket exchange.
  Expanded imported-memory/bandage/fabric closure reads Grendilyn's paired
  recipe, the ambassador's memory request and shared foreign bandage uses.
  Complete relevant foreign reward/material prototypes, recipient/source actors,
  their reset groups and placement rooms were read. Reuse
  [Winterhaven's comprehensive dossier](WINTERHAVEN.md) for its broader
  memory/fabric/colored-thread/garment families; no foreign zone is newly
  marked comprehensive by this bounded review.

[Generated source navigation](../../reference/zone-story-audits/desert.md)
retains exact binding/source links. Static declarations and source review do
not prove current actor visibility, source admission, random stock or renewal.

## Eight native stories and real sources

| Card / native Q line | Exact offering → outcome | Source and boundary |
| --- | --- | --- |
| Miner49020 / Q10 | Large glowing potion49072 → studded mining belt49099 | Merchant49142 G901/stock, starts49147; miner M953 at49201. Specific potion, not drinking/any healing item. Mine access and personal purchase are separate. |
| Cloaked figure49061 / Q27 | Dusty signet49171 → copper serpent medallion49042 | Jolly merchant49221 G1302 at49538, with two F guards; giver starts49051. Palace access, kill and ownership are not enforced by exact supplied proof. |
| Traveler49087 / Q43 | Compass49057 → pale tourmaline medallion49058 | Gangleader49116 G1156 at49418, with two F followers; traveler starts49666. Refuse prose does not place the compass inside an unplaced container. |
| Wizard49099 / Q58 | Vernadad's signet49148 → eerie silver medallion49167 | Vernadad49170 E1598 at49677, with four F bodyguards; wizard starts49034. Ring must become loose. Another signet or narrated House collapse is not the exact proof. |
| Shady merchant49155 / Q74 | Contraband49079 → cold bluesteel ring49098 | O542 on floor49272 with Mijium49149; merchant starts49354. Source is floor, not Mijium's inventory. Ship contraband is a different system. |
| Eriic49161 / Q91 | Elvish hunter medallion49173 → desert-island locket55371 | Identified producer is Goranon Q131, without local E/G medallion reset. Eriic starts49413; supplied medallion bypasses earlier receipt/Queen history. Follow-on remains Winterhaven-owned. |
| White-robed figure49220 / Q115 | Writhing wyrm eyeball49027 → glowing runestone49170 | Fire wyrm49024 G955 at49202; recipient starts49000. Output is type11, separate from fixed TOUCH travel stones49174/49175. |
| Goranon49223 / Q131 | Royal garb49063 → elvish hunter medallion49173 | Insect Queen49121 E1437 at49607; Goranon starts49034. His reward enables Eriic's separate recipe. Garb is worn armor, not an automatically loose quest token. |

The eight givers have independent accepted item contracts. Dialogue aliases are
alternative responses, not one achievement each. Every card has exactly one
required terminal completion; its current-item/access and earlier-receipt checks
are optional guidance. Exact loose materials supplied by another player satisfy
native identity checks without proving source recovery, personal combat or travel.
Historical completion remains recorded after its proof is spent, while current
material readiness returns to missing.

## Mine, House, palace and hidden access

| Route | Authored prerequisites and effect | Journal boundary |
| --- | --- | --- |
| Collapsed mine | Boulder49012 O522 on cliff49150; values270/49154/N. PUSH clears EX_BLOCKED on49154 north →49155. D280 starts blocked; reverse south D284 starts open. | Working remote fixed switch: no TAKE edit or entrance-door invention. Successful action/actual aperture must be admitted before future access credit. Another player can leave the route open. |
| Shaft and rivers | Shaft49191–49194 has three fall records; first fixed river49022 at49195 ENTER→49005, Follow SOUTH49005→49007→49009→49196, EAST→49197, SOUTH→49198→49199, EAST→49200 and NORTH→49201. Wyrm lair49202 is UP. Second river49023 at49201 ENTER→49200 is the trench return, not the inbound route. | Follow the actual authored route; ENTER travel and arrival are distinct from holding potion/eye or recorded terminal exchange. Falling risk remains native. |
| House Lo'Ushuur | North49268→49269 is closed/locked pickproof gate, key49106. First of four burly guard49171 resets carries G1044 key; lord wears ring at49677. | Optional key condition never recreates a key or invents personal access. Open/shared route or supplied ring bypasses personal key history. |
| Sultan's palace | Diagonal gates49004↔49106 are pickproof with silver key49107 from elite guard49154 outside. Merchant in pleasure room; Sultan at49054. | Gates, merchant proof, palace memory and epic claim are separate facts; no torch/political overthrow mechanic inferred from prose. |
| Hidden wine cellar | Warehouse49016 secret closed DOWN plank ↔ cellar49272 secret UP; ordinary door kind1, no key required. | SEARCH/OPEN/GET success needs actual target/state/root evidence. Source floor contraband can be supplied; no kill prerequisite. |
| Docks and chest | Steel key49028 from drunk sailor/prophet unlocks pickable beach49015↔49079 gate. Gold chest49163 at49018 is closed/locked/pickproof, key49165 on Sultan, P children49164 and49139. | Ordinary access/treasure is support, not invented delivery or campaign. Key custody and shared open state differ from a historical receipt. |
| Magic tower | Fixed49174 at49023 TOUCH→49396;49175 at49396 TOUCH→49023. Both wear0, charges-1. | Neither needs the white-robed figure's different reward. Preserve existing travel, combat/arena restrictions and unlimited fixed behavior; no mobility change ships. |
| Other travel | Fixed war galley, passenger ship, tents, caravan, warehouse and rope-ladder portals use ENTER; celebration tent49787 has ceremonial combatants/gear. | Described ships are fixed intra-zone portal objects, distinct from player-owned ship services. No ceremony, expedition, rescue or movement terminal was found. |

## Foreign referrals and separate support systems

Goranon → Eriic → Winterhaven ambassador forms an item-progression story,
not an enforced personal multi-stage campaign. Ambassador55242 starts in two
dispersal rooms55005/55400 and may wander. The exact locket gives fabrics55332,
sandy-brine elixir55370 and100,000 copper at Winterhaven Q3527. Grendilyn55133
at55602 requires both Jade Empire fabric55331 and Venan'Trut fabric55332 for
two distinct silk-thread roots55369 plus250,000 copper (Q2460). The elixir
isn't that recipe's input. Winterhaven's dye/tailoring routes stay there.

Separately, Sultan's imported memory55285 G718 is accepted by the same ambassador
(Q3521) for shopkeepers token55033, strange enchantment scroll55362 and1,000,000
copper. Locket and memory do not substitute or require each other. Memory/fabrics
have authored T records; actual custody/effect/admission must be qualified.
Scroll prose alone does not establish a native RECITE spell or new local lesson.

Tinker49065 carries/stocks map49179. Shipyard's Gringash43135 Q233 accepts it
for81,000 experience/65,656 copper and retires; Kruth'Urgur43167 Q662 accepts it
for35,000 experience/35,000 copper and retires. These keep Shipyard ownership.
The map differs from the compass; its `map vernan trut` extra-description typo
needs a minimal builder-reviewed text decision, not a topology or quest change.

Narggle49064's `world_quest` ASK service has its own level11 minimum, assignment,
map, fee, abandon, completion and correct-recipient policies. Random world quests
are excluded from this native catalog. Eriic and Ruffus use computed epic
PRACTICE teaching; a separate mobile special returns false for other commands,
allowing normal Q dispatch. Epic purchases remain guarded with active accounting.

Taproom49051 provides crew LIST/HIRE; dock49090 provides ship LIST/BUY/SELL/
SUMMON/RELOAD/REPAIR. Ownership, dock/maintenance, crew eligibility, wallet,
ship and save state remain separate. Hiring still uses direct wallet/ship
mutations in shared source; this journal does not qualify coordinated settlement.
Room flags and shop stock are evidence, not new quest contracts.

Imported epic stone359 is carried by the Sultan. It has actual target-matching
TOUCH, periodic zone/configuration, group payout and committed zone-touch
transaction, without becoming a ninth item-exchange achievement. Only eligible,
co-located participants enter the frozen claim; busy/unpowered/fighting/staff/
wrong-zone/level rejection and failure to submit differ from accepted committed
reward. Claim publication/recovery should reuse typed accounting receipts,
with explicit policy for journal milestones and no duplicate reward credit.
Its old node lore must be reviewed against its real handler, not used to enable
another controller. Ordinary bandages and their foreign requests remain support.

Slave liberation, House fall, nest destruction, magical-tower expedition,
Morgian errands, guild taxation, fishing payment and celebration success have
prose or ordinary combat/loot but no additional local native quest endpoint.
Builder-designed campaigns need explicit actor/world/stage transitions before
credit. No narration is silently promoted to a terminal receipt.

## Direct area-owner questions and conservative recommendations

| Question | Source evidence | Recommendation |
| --- | --- | --- |
| Which public title and command aliases should identify this area? | The normalized .zon title is “The Desert City of Venan’Trut,” with source stem `desert`; command-target alias and owner disambiguation remain open. | Keep the normalized title and source stem. Add aliases only after a real command binding and owner are confirmed. |
| Should the eight exact item exchanges remain independent rather than form a mandatory Queen-to-Eriic campaign? | Each native Q accepts one exact item and gives one item; the Queen/Goranon/Eriic sequence has separate receipts and no native campaign-wide prerequisite. | Preserve eight independent contracts. Treat the broader narrative as guidance unless an approved campaign defines its own accepted endpoints. |
| Should supplied exact proofs continue to bypass personal source, travel, kill, and earlier-receipt history? | The eight givers accept exact carried items; readiness and optional producer history do not establish personal recovery, purchase, palace access, or a prior Q receipt. | Keep native supplied-proof eligibility. Keep personal acquisition and exploration as distinct optional goals. |
| What is the intended active-accounting policy for the four textile stages that mix items and copper? | Spinner, dyer, weaver, and seamstress together require exact outputs and 60,000 copper, and each is currently guarded as an unsupported durable offering. | Keep the guards until item, fee, output, replay, and recovery can settle atomically. Do not debit or issue from readiness checks. |
| Should the pub-alley contact’s 1,000-copper exchange remain a non-quest service with no item reward? | The native block accepts coins and returns no item; the journal gives it no achievement or daily. | Keep it outside story credit unless the owner defines a bounded reward and accepted result. |
| Which of the six native daily candidates should be promised after source and accounting qualification? | The journal has six potential daily outcomes; zone resets are mode one, many sources and givers have cap one, and stock/renewal has not been played. | Leave all six as candidates until live stock, recipient renewal, settlement, and repeat eligibility are qualified. |
| Should Winterhaven’s locket and memory exchanges count as part of this area’s completion? | Eriic’s locket continues into Winterhaven; Sultan’s memory has a separate Winterhaven consumer, and each foreign contract has its own owner and materials. | Keep the follow-ons owned by Winterhaven. Do not require those foreign receipts for Venan’Trut’s native exchange. |
| Should Narggle’s random world quests, shared teacher advice, or epic purchases create local journal progress? | Narggle uses the generic world-quest service; Eriic/Ruffus have computed epic lessons; no local ACT_TEACHER or local service terminal was found. | Keep those services separate from the eight native receipts. Credit only a distinct accepted service result if the owner deliberately authors one. |
| Should crew hiring or ship services count as local progression while their wallet and ship settlement remains separate? | The taproom crew handler and dock ship handler are literal assignments with their own buy/hire/repair commands and direct wallet/ship mutations. | Keep these outside the journal until coordinated settlement and ownership are qualified. Do not infer a quest from hiring or purchasing. |
| Should room 49051 continue to function as an inn as well as a crew shop? | Room flags initialize three inn rooms, but the taproom’s literal crew handler overrides the shared handler on 49051. | Confirm intended service precedence before changing assignments. Preserve current dispatch until the owner selects which service should operate there. |
| Are the unplaced tent portals with destinations 49681 and 49682 retired templates or intended travel objects? | Two local tent prototypes point to absent rooms and have no local O placement; a third unplaced tent points to existing room 49338. | Keep unplaced templates outside active routes. Choose retirement, corrected target, or placement only with an approved destination and safe return. |
| Are unused room 49029 and the Jussinload room 49790 intentionally reserved or external-only? | Room 49029 is marked unused, isolated placeholders have no inferred path, and 49790 is an identified Jussinload endpoint rather than a local quest route. | Treat these as reserved/external context unless an owner supplies a concrete active route and contract. Do not add exploration credit based on numeric presence. |
| Should the roof’s “large lock” prose become a real locked gate? | The reviewed door/reset data closes interior doors but does not lock the roof gate; no positive local key prerequisite was found. | Let loaded world/reset state govern access. Change lock data only if an owner-defined lock and key source are intended. |
| What safe approach and return are intended for the banner recipient at the flagpole? | The ladder and midair rooms have fall chances 5, 80, and 90 with no-ground sectors; slippers and banner do not grant flight or levitation. | Qualify actor ability, actual fall/survival, offer, and return separately. Do not imply the reward or equipment protects the player. |
| Should retrieving the military assessment from the desk require surviving its acid trap for story credit? | Assessment 82822 is inside a closeable one-charge acid-trapped desk; opening, trap damage, loose-root recovery, reading, and delivery are different events. | Keep the authored trap and exact-item exchange. Add a survival goal only with actor-scoped accepted trap and recovery evidence. |
| Which occasion is intended in the note and Duke’s reply: his son’s wedding or his daughter’s birthday? | The note and the accepted reply name different occasions, a concrete local text mismatch. | Confirm the intended plot, then make a separate narrow copy correction while retaining both exact item identities and outcomes. |
| Should the map’s “vernan trut” spelling be corrected to “Venan’Trut”? | The Tinker’s map extra description contains the variant spelling; the area title and other sources use Venan’Trut. | Confirm the intended proper name and correct only the text. Keep the Shipyard map contracts and ownership unchanged. |
| Which broader narrative threads should stay as lore, and which merit separately authored objectives? | Mine rescue, signets, hidden contraband, palace politics, ship work, slave liberation, tower expeditions, and invasion have prose or ordinary actions but no additional local Q terminal. | Keep the eight native receipts as current completion. Select any future story one at a time with an actual actor/world-state endpoint and clear owning area. |

## Capability work and fair builder follow-ups

| Finding | Evidence and implication | Plan / qualification |
| --- | --- | --- |
| First source versus gifts | Potion stock, floor contraband, equipped rings/garb, mob-held eye/compass and native rewards differ | Admit source/reset UID, exact parent/custody, successful BUY/GET/remove/transfer and first-source fact. Cover scarcity, failed traps/visibility/capacity, gifts, partial retries, ledger recovery and spent proof. |
| Confirmed access/travel | Remote boulder, shared blocked aperture, pickproof gates, secret plank, falling shaft and fixed rivers/stones | Preserve control target, reciprocal exit, actual transition and arrival. Qualify rejected/wrong target, already open route, another player's action, reset generation, falls, teleport failure/arena and recovery. |
| Multi-stage versus supplied proof | Queen → garb → Goranon → medallion → Eriic → locket → foreign fabrics | Keep independent native contracts and optional prior receipt. Builder defines personal all-stage milestones/shortcut policy; aggregate campaign without awarding the same terminal twice. |
| Computed/overlapping services | Eriic Q plus computed epic teaching; flag inn overridden by literal crew handler | Audit type/flag/table/explicit registration and final handler precedence. Qualify teaching/wallet/ship services before unguarding; review taproom RENT intent separately. A name or partial literal scan cannot establish absence. |
| Group claims and imported ownership | Sultan holds epic stone and foreign memory; shared TOUCH transaction freezes beneficiaries | Integrate actual committed group claim identity and versioned physical/canonical affiliation. Reject observation-only or foreign-receipt inflation; retain frozen recovery when new tracking is unavailable. |
| Actor/source renewal | All eight givers D0, reset mode1, many sources cap1, roaming merchant/ambassadors | Static daily candidacy is only a candidate. Qualify live recipient episode, source issuance, current supply, actual empty-zone reset, repeated claims/replay and cold recovery. |
| Orphan templates and missing destinations | Unplaced tents49084/49087 target absent49681/49682;49082 also unplaced but target exists; bag/wagon/refuse descriptions alone do not create quest source | Inactive templates are not proof of a live broken path. Builder decides intended retirement, corrected template or designed placement. Do not add exits, deploy a portal, enable TAKE or change charges to make prose work. Any approved repair gets its own fix commit/news. |
| Copied/misaligned clues | Ambassador describes a Dawndale sigil; map extra spells vernan; epic stone describes node lessons; several venues advertise more than their handlers | Review intended text versus mechanics. Prefer a truthful minimal text correction where intended behavior is confirmed; preserve handler/mobility/access policy pending design. Keep proposals separate from shipped news. |

The Fields Between lesson applies here: an intentionally disabled portable
mechanic is not a journal obstacle to undo. Preserve native TAKE, weight,
charges, combat/access and activation policy. See the owner-confirmed
[rift hotfix replacement follow-up](FIELDS_BETWEEN.md#L170). An inert proof,
fixed-source interaction or retirement needs builder design and qualification
before implementation, with separate fix commit and prominent news.

Validation: exact source/schema fixture; all99 Python/C++ file-loader/projection
journeys; full production catalog/inventory/audit regression; maintained build;
formatting/whitespace, source links, all98 prior journals/all2668 definitions/
fingerprint/content revision2/registry, original220 queue and exact prior PR/news
preservation. Projection tests distinguish dusty/Vernadad rings, worn garb,
ordinary runestone versus eyeball, supplied later proof without earlier history,
old receipts versus spent proof, eight independent outcomes, replay and cold
recovery without fabricated foreign discovery.

Synthetic receipts qualify projections, not played source/access/BUY/PUSH/
SEARCH/OPEN/UNLOCK/GET/falls/travel/combat/learning/group claims/ship/offer/
settlement/actor lifecycle/database persistence/daily renewal. No native repair,
accounting activation, DB/server operation, migration, deployment or merge
occurs in this checkpoint. **81/220 comprehensive,139 pending; Past Ceothia
(`ceopast`) next.** Full roadmap goal remains active.

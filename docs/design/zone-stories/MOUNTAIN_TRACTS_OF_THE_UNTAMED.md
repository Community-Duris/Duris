# Mountain Tracts of the Untamed: comprehensive story mapping

Priority87 closes the source review for zone209, registry20807–21149,
source area `mountaintracks`, reset mode2. The [schema3/revision1 sidecar](../../../areas/story/mountaintracks.story.json)
maps four cards to all four native recipes, with eight contacts, all19 addressed
aliases and six optional checks(five current materials, one earlier Futni receipt).
The [generated audit](../../reference/zone-story-audits/mountaintracks.md) records
binding identity. **No native zone or quest repair ships.** Pickup, scarcity,
mobility, shop/potion admission, passages and PvP mechanics remain unchanged.

All new discovery, encounter, journal, achievement and daily credit requires
active, ready accounting; frozen reward recovery remains separate. Source and
journal projection do not qualify played transactions, access or daily renewal.

## Complete source closure

| Source | Reviewed closure and dispatch implications |
| --- | --- |
| [Native quests](../../../areas/qst/mountaintracks.qst) | All eight blocks:fourM/fourQ, four addressed response families/19 aliases, no qc_action. Putsie20990 and farmer20992 have empty S-only headers, not recipes. All four exact input/reward/departure bindings preserved. |
| [Rooms](../../../areas/wld/mountaintracks.wld) | All245 physical rooms20900–21149 except20914/20915/20916/20917/20920;86 full title/prose families,35 full headers,578 exact exits/199 relative patterns,15 full exit-text families and four complete non-exit metadata families. Waterfalls, currents, caves, labs, crypts, fog, abandoned settlement and northern tracts included. |
| [Mobiles](../../../areas/mob/mountaintracks.mob) | All98 full prototypes20900–20997,89 full prose families and every numeric tail. Quest source holders, shop keeper, supporting wildlife, apprentice and empty/minimal templates distinguished. Fourteen unplaced prototypes remain unplaced. No ACT_TEACHER identified. |
| [Objects](../../../areas/obj/mountaintracks.obj) | All63 full prototypes20900–20962:exact proofs, large stationary statue, miniature wearable proof, hidden/NORENT ingredients, potion, seven travel items, seven switches and ordinary equipment. Types, flags, values, weight/cost and extra descriptions read. |
| [Resets](../../../areas/zon/mountaintracks.zon) | All259 commands:173M/40D/18O/16E/12G;230 exact/230 M-parent-aware families and144 expanded groups. No F/P or imported reset prototype. Actual source rooms/holders, cap1 proofs, unplaced switches/portals and closed-versus-blocked doors reviewed. |
| [Shop](../../../areas/shp/mountaintracks.shp) | One complete record:Riffleraffle20972/room21025, producing20933/20934/20932/20931/20939. Full admission/keeper/open/race fields reviewed. Shared [shop boot/keeper dispatch](../../../src/economy/shop.c) and current stock matter; ordinary shop goods are not quest ingredients or receipts. |
| Shared procedures | [Quest loader/handler](../../../src/world/quest.c) separates addressed ASK/TELL from accepted GIVE. No local literal special assignment, epic-teacher binding or ROOM_INN identified. [Object switch handler](../../../src/specs/specs.object.c) and [automatic type29 registration](../../../src/world/db.c) supply placed PUSH controls. All five placed targets have their expected reciprocal directions. |
| Travel/terrain | Full [item teleport selector](../../../src/magic/spell_travel.c), [interpreter](../../../src/cmd/interp.c), GRAB/CLIMB and [QUAFF](../../../src/cmd/actoth.c) reviewed. ENTER7 and GRAB65 are current commands; CLIMB556 is separate preparation. Room fall/current/door load and unresolved-exit removal checked. |
| Global closure | Across713 active type25 prototypes, all seven fixed incoming destinations are local objects; no foreign fixed incoming travel identified. All36 directed boundary records reviewed:15 valid reciprocal neighbor pairs and six unresolved raw local exits. Full15 foreign boundary room records read across Surface, shafts, connector, lava, bs, labyrinth, underdark, nexus and lavcav. No foreign reset group references local proof/mobile prototypes. |
| Foreign consumer/code | Exactly five recipes touch local objects:four local and [Winterhaven Ohnagra55127](../../../areas/qst/wh.qst#L2199). Full bounded Ohnagra response and input/reward records reviewed; the outcome remains foreign. Exact source-number scan found only ordinary boots20924/bone leggings20937 in [chaos kit data](../../../src/account/chaos_eq_data.h), not custom quest logic. |

The complete review includes supporting surface trails, abandoned camp,
subterranean water/crypt/laboratories, mountain/nexus boundaries and reserve
components. A scenery description or unused prototype does not create a
controller, prerequisite, source producer or additional achievement.

## Player progression and accepted outcomes

| Card / native Q line | Exact input | Accepted outcome and source explanation |
| --- | --- | --- |
| Tall monk /13 | Small carved piece20923 | Enchanted mithril leggings20930; no departure, blank native accepted response. Monk20928 starts21024. Piece is SECRET floor reset cap1 at statue chamber20941. Secret EAST from20933 reaches it. Type11 WORN with TAKE/ear wear bits means a worn copy must be removed before hand-in. Large statue20907 is stationary scenery and remains non-takeable. |
| Half-orc mage /34 | Circular marble piece20954 | Golden serpent bracer20955; no departure, blank accepted response. Mage20981 starts21025. Tronglodish20982 carries hidden piece cap1 at laboratory20952; secret SOUTH from20933 is closed/unlocked. Exact type13 TRASH/quest/magic object is not a key or teleport device. |
| Futni /57 | Green dragon scale20947 + vampire tooth20948 | Glowing green potion20949; no departure. Futni20983 starts20945. Dragon20978 carries scale in cavern20979; bloodthirsty vampire20979 carries tooth in crypt20978. Each is SECRET+NORENT, quest-marked and cap1. Other vampire prototypes or decorative scale prose differ. Two matching scales cannot replace one tooth. |
| Bumble /85 | Fresh glowing green potion20949 | Explorer gloves20950; no departure, blank accepted response. Bumble20984 starts21015. Earlier Futni receipt is an optional explanatory route. A supplied exact potion fits without personal crafting history; an old receipt cannot replace a spent potion. |

The monk’s soul searching does not establish conversion, worship, carving or
chosen-deity outcomes. The mage’s death threat does not require a personal
apprentice kill:the native contract checks the marble item. Likewise Futni’s
recipe does not require personal dragon/vampire kills or grant learned chemistry.
Gifted materials remain valid native inputs. Current possession cannot
distinguish those gifts from first acquisition at the original source.

Ask the monk `statue`, `astansus`, `underdark` or `hi`; the mage `hi`, `half`,
`orc` or `half-orc`; Futni `hi`, `quest`, `elementalist`, `potion`, `drow` or
`futni`; Bumble `hi`, `quest`, `bumble`, `explorer` or `potion`. Nineteen aliases
select four response families, not19 achievements. There are no ambient
qc_action scenes here. Blank accepted responses are a player explanation gap,
not proof of missing rewards or authorization to invent story endings.

## Potion choices and foreign progression

Potion20949 is type10 with native values `[50,280,41,236,0,0,0,0]`:
level50, pass without trace, haste and true seeing. [Spell IDs](../../../src/magic/spells.h)
establish the configured spells; Futni’s prose about foresight does not create a
prediction or learning feature. QUAFF searches carried/held potion, checks native
timer and combat admission, can spill/extract it in combat, dispatches permitted
spells outside NO_MAGIC rooms and extracts it after use. These are existing
mechanics, not newly qualified quest completion events. Do not require drinking
before delivery or promise successful effects in every room.

[Ohnagra’s Winterhaven recipe](../../../areas/qst/wh.qst#L2199) requires
moonstone heart12001, gloves20950 **and another fresh potion20949**, rewarding
55062. Its dialogue points to Futni/Bumble and the Xexos automaton, but remains
Winterhaven’s outcome. Bumble consumes the first potion. Doing both exchanges
therefore needs two separate potion copies; one old Futni receipt or one current
copy cannot fund both. No personal Futni history is imposed, and no Winterhaven
discovery/credit is inferred from merely holding its materials. The existing
Winterhaven journal is preserved.

The journal may show readiness for an item’s competing uses. It neither reserves
that item nor settles its consumption. Builder-designed allocation guidance must
use admitted current custody/quantity and selected use, retaining native recipes.

## Access, controls and preserved restrictions

| Feature | Native facts and limits of journal evidence |
| --- | --- |
| Surface/network boundaries | Surface611042↔20900,607457↔20942,598665↔21042; shafts10035↔20921,10003↔20934; connector53728↔20928,53746↔20973; lava12104↔20970,12131↔20973; bs74680↔20970; labyrinth5000↔20971; underdark823315↔20977; nexus57637↔21015,57536↔21144; lavcav35501↔21148. Fifteen actual reciprocal pairs do not guarantee safe passage through terrain/combat/doors. |
| Fence/earth controls | PUSH fence20952 at20918 targets blocked NORTH20918→20957. PUSH earth20960 at20922 targets UP20922→20973; earth20918 at20973 targets reverse DOWN. Their D8 resets block otherwise open/unlocked directions. Shared handler removes EX_BLOCKED, clearing reverse when target is not secret. No ownership of another actor’s opening is inferred. |
| Tomb/boulder controls | PUSH tombstone20917 at20949 targets DOWN20949→20948; PUSH boulder20935 at21139 targets SOUTH21139→21145. D9 means blocked and closed/unlocked, with closed reciprocal. Handler removes blocking **without clearing EX_CLOSED**; OPEN remains separate. Already unblocked returns Nothing happens. Room message is not committed traversal. |
| Existing fixed travel |20900 ENTER→20989;20902 ENTER→20909;20903 ENTER→20913;20925 ENTER→20953;20936 **GRAB vine** at21121→21144. All are non-takeable with charges−1. Shared selector dispatches before ordinary do_grab. CMD_GRAB65 is valid today; CMD_CLIMB556 prepares a skill effect and is not the vine’s travel command. No command rebinding or portability repair is needed. |
| Unplaced objects | Type25 tower20919→21008 and black hole20943→20979 have no local reset; neither is activated. Type29 mound20941 targets20904 NW and boulder20945 targets20921 EAST, but those target directions do not exist. They remain unplaced. Builder establishes reserve/retirement/old design intent before creating exits or controllers. |
| Water/fall/fog |20913 has C50 1, despite mild-current prose. Waterfall20923–27 has F100;20999/21003/21065/21068–69/21093/21110–11/21131/21144–45 have F10. Actual current, fall, skill, terrain and combat rules govern admission. ROOM_BLOCKS_SIGHT1073741824 is a fog/sight flag, not a flight requirement. |
| Native missing targets |20999 SOUTH uses−1;21126 EAST→21202,21127 EAST→21206/WEST→21204,21128 EAST→21208/WEST→21206 target absent room records across areas/wld. [renum_world](../../../src/world/db.c#L1531) removes unresolved exits at boot. This is not evidence of a live dangling-room crash. Do not create212xx rooms or reinterpret the zone comment as authorization. |
| Isolated/stale clues |21127/21128 have no identified ordinary incoming edge and lose their absent-target exits;21149 has neither ordinary incoming nor exits despite DOWN-for-Lava-Caves prose. Actual Lava Caves boundary is21148 SOUTH.21011 EAST points to itself; three exit keywords are numeric artifacts. These require builder intent and truthful clue review before any mechanical change. |

All seven travel prototypes retain original wear flags/charges/destinations.
Preserve the [Fields Between intentional escape hotfix and required replacement](FIELDS_BETWEEN.md):
quest prose does not authorize pickup, activation, a new passage or PvP mobility.
The old shaman request needs a separately designed replacement, not a portable
working rift. This checkpoint adds no native repair.

## Builder-required follow-ups

| ID | Fair finding and proposed work; qualification before credit or repair |
| --- | --- |
| **ZSQ-MOUNTAINTRACKS-SOURCE-RENEWAL** | Hidden floor miniature and hidden carried marble/scale/tooth have cap1; ingredients are NORENT. Define authoritative source/root/UID/custody/selected actor and gift versus first source. Qualify SEARCH/GET/corpse recovery/worn/nested proof, rejected/duplicate materials, concurrency, rollback, consumed proof, reward settlement, frozen recovery and actual mode2 renewal. Preserve scarcity; static daily eligibility does not prove replenishment. |
| **ZSQ-MOUNTAINTRACKS-POTION-ALLOCATION** | One potion has delivery, consumption and foreign-recipe uses. Explain exact quantity and the two-copy Bumble→Ohnagra route, optional personal history and current possession. Any selection/reservation guidance must use real admitted materials; earlier receipts never restore spent stock. No mandatory craft/QUAFF or foreign credit. |
| **ZSQ-MOUNTAINTRACKS-ACCESS-RESULTS** | Shared PUSH may clear blocked doors that remain closed; GRAB vine is a valid current portal command. Define selected control/target and accepted unblock/open/committed arrival separately, recording the actual actor. Qualify already open, failed/wrong command, wrong selected object, remote audience, other actor and terrain rejection. Preserve pickup/charges/native opcodes. |
| **ZSQ-MOUNTAINTRACKS-STORY-ENDPOINTS** | Three blank accepted responses, faith/apprentice/lost-tower lore and Putsie/farmer S-only headers do not establish extra native endpoints. Builder decides whether truthful response text is enough or explicitly authors a lesson/search endpoint. Any custom learning/death/search prerequisite needs authoritative result events and actual replay/recovery proof, not keyword or timer messages. |
| **ZSQ-MOUNTAINTRACKS-BOUNDARY-INTENT** | Six unresolved raw exits, isolated21127/21128/21149, two unplaced invalid-direction switches and two unplaced travel objects may be reserve remnants. Confirm intended reserve/retirement/connection first. Prefer explanatory cleanup; any new route/controller requires reciprocal, terrain, combat/PvP, visibility and scarcity design. No automatic room creation, portal activation or portable mobility. |
| **ZSQ-MOUNTAINTRACKS-CLUE-CONSISTENCY** | C50 versus mild-current prose, DOWN-for-Lava-Caves in isolated21149 versus real21148 SOUTH, self-loop21011 EAST, numeric exit keywords and spelling/blank responses merit intended clue review. Prefer minimal truthful text/metadata once intent is known. Actual fixes must be separate named commits with original-fails/repaired-passes proof and prominent before/after news; preserve recipe/item mechanics. |

## Validation and limits

Existing source/schema regression protects exact four bindings, source holders,
hidden/NORENT flags, miniature versus stationary statue, valid GRAB route,
PUSH blocked-versus-closed semantics, reserve objects, absent-target closure,
Winterhaven’s separate three-input recipe and19 grouped aliases. Existing Python
and C++ loader/projection journeys cover supplied potion without Futni history,
two scales without tooth, worn miniature, wrong large statue, reward ownership
without receipt, four distinct outcomes, earlier history versus spent potion,
replay and cold recovery without foreign discovery. All earlier journeys remain.

Production catalog/inventory/audit, maintained build, changed/staged formatting,
source/document links and exact prior journal/definition/queue/PR-news preservation
are required checks. Synthetic completion receipts test projection; they do not
qualify real acquisition, gifts, control dispatch, QUAFF, native offers,
consumption/reward settlement, combat/access, daily renewal or DB persistence.
No accounting activation, DB/server operation, migration, deployment or merge.

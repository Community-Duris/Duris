# Temple of the Earth: patrol deliveries, hidden temple and abyss

Priority101 of the original220-zone queue. This is a **source-comprehensive map**;
played accounting/source/access/persistence qualification remains pending. The
[schema3/revision1 journal](../../../areas/story/earth.story.json) adds two independent
story-only outcomes,12 contacts/46 aliases and two optional current-material
rows, without exclusions. New discovery/encounter/journal/achievement/daily credit
requires active, ready accounting; frozen receipt recovery is separate.

## Source closure

| Source | Complete review and implication |
| --- | --- |
| [Quest source](../../../areas/qst/earth.qst) | All2 Q/22 M/MA families,12 dialogue contacts/46 aliases: patrol/bloodrune/keeper/rescue/riddle/encampment/dead-priest dialogue, exact inputs/rewards and both D1 endings. No personal endpoint for the larger narrated missions. |
| [Rooms](../../../areas/wld/earth.wld) | All198 complete physical records43500–43697:140 full prose/58 headers/2 metadata/636 exact exits/169 relative patterns/14 full exit-text families. Registry43329–43697/mode0. Approach, temple, hydra caverns, illusion maze, abandoned temple, bloodrune, abyss camps, fortress and holding/selector rooms reviewed. All boundaries and unusual exits retained. |
| [Mobiles](../../../areas/mob/earth.mob) | All105 full records43500–43604/prose families and ID-paired numeric data. Gromdishar43502 starts43572, captain43509 starts43512 and Lithibar43504 starts43549; these three have SENTINEL. All guardians, high priests, hydras, outpost contacts, Eligoth and holding-room rares reviewed. M/F/E/G receiver relationships are source declarations, not qualification of live stock. |
| [Objects](../../../areas/obj/earth.obj) | All95 full records43500–43594, including inscriptions/traps/types/flags/values, corpse containers, both exact proofs/rewards, all keys, controls, runes, mirrors, generic custom controller and ten button/selector props. Full imported360(heavens) and23805(plane_earth_one) records and bindings/procedures reviewed. Badge43525 is a real prototype without an identified current world producer;43595 has no object prototype. |
| [Resets](../../../areas/zon/earth.zon) | All356 commands: D56/O51/P7/M165/E43/G12/F22;295 exact/304 parent-aware and231 expanded argument/location families. Chances100×348,25×4,20/40/60/80×1 each. All conditions/caps/location/parent and follower groups reviewed, including open-body ring, selector alternatives, givers, keys, abyss camps, boss and separate imported equipment. |
| Custom/shared execution | Full [Earth procedures](../../../src/specs/specs.earth.c), [literal bindings](../../../src/specs/specs.assign.c), [dispatch](../../../src/cmd/interp.c), [switch](../../../src/specs/specs.object.c), [name matching](../../../src/world/handler.c), [creation/renumber/reset](../../../src/world/db.c), [chance](../../../src/core/utility.c), [key/container access](../../../src/cmd/actmove.c), [teleport commands/arrival](../../../src/magic/spell_travel.c), [native quest settlement](../../../src/world/quest.c) and [guarded journal runtime](../../../src/world/zone_story_quest_runtime.c) reviewed with current custody guards. All local numeric/procedure source leads followed; incidental chaos equipment tables do not create quests. |
| Foreign closure | Global active recipe/reset/exit/object/713-portal scan: exactly2 touching recipes, both local;five brass reset groups importing mobile43540; two imported boss objects. Reciprocal Underdark804038E↔43500W and foreign realm14209DOWN→43568 records reviewed. No foreign portal targets a physical Earth room; six local portal declarations do. Physical aopal43341–43 and patrol stock/mobiles/guarded hiring resolve the range-only patrol_shops lead. No local shop file. |

## Exact outcomes and current materials

| Contract | Exact accepted recipe | Actual ending |
| --- | --- | --- |
| Gromdishar43502/Q27 | I43525→I43561;D1 | Badge consumed, vial of blood rewarded, this giver retires. No player escort. |
| Paladin captain43509/Q104 | I43539→I43562;D1 | Silver band consumed, crested shield of Melkivar rewarded, this giver retires. No player recall, rescue or bloodrune closure. |

Neither recipe requires an earlier personal topic, recovery, unlock, rescue, combat
or planar-travel receipt. Exact supplied proof remains valid. Both proofs have
QUESTITEM; the badge is type13, the band type9/finger gear. The journal's optional
preparation rows require loose inventory. A worn band or a reward item does not
establish a current material or accepted delivery.

Gromdishar's hydra-lair account names a lost brother and a badge, but no current
O/P/E/G world reset or maintained custom producer supplies43525. Corpse43522 is
fixed type13 scenery; corpse pile43524 is a container holding a different item,
not the badge. Do not claim a scout or corpse source merely from that dialogue.
Builder-selected legitimate supply and scarcity require a separate repair.

The captain's exact ring is P43539/cap1/chance100 into O43538/cap1 at43628, the
slain paladin's body. The fixed body is type15 with values199/0/0/199: open,
without a native closeable or locked state. Retrieve the band; no OPEN, corpse
kill, trophy or shield input is required. Successful fresh O inserts the newest
matching parent at the global-list head before P selects it. Retained/global
stock still needs admitted-generation qualification, not an assumed placement bug.

## Broader progression stories

The two patrol arcs overlap geographically but remain independent in accepted
evidence. Both describe the abandoned temple and bloodrune. Gromdishar asks to
check a duergar outpost; the captain asks to stop the priests and find survivors.
Lithibar explains corruption, his confinement and the illusion clue. Charinth
claims the key to the bloodrune. Duergar/elven camps, the paladin camp, gnome,
wizard, death knight and shaman provide perspectives on the same invasion.
The map exposes all addressable topics without inventing one achievement per alias.

Access has several separate branches. Key43513 in desk43512@43626 fits Lithibar's
study approach43548N. Guardian43556 carries43529 toward the upper spire. Sun
symbol43534, metallic button43530 and red book43503 use PUSH270; wheel43502 and
dial43509 use PULL340. The charred book43508 contains the dial, and Charinth
carries flesh sliver43542. has_key accepts that exact loose or HOLD vnum without
requiring type18, so the sliver's type13 alone does not establish a broken lock.
The scale43565 on mobile43595 is a different hydra-lair key. These controls,
container traps, shared-open/picked alternatives and native keys are access
evidence, not goals in either delivery.

The illusion chamber has fixed visible materialbutton scenery43590–94 plus
custom controller43584@43550. Unreachable selector room43697 receives independent
type29 material-name selectors43585–89. The full custom scan chooses the last
recognized present selector; PUSH on a recognized material either clears the
actor's north BLOCKED bit or releases43540/43539/43538/43550. It does not create
an actor receipt, enforce a puzzle-specific summon cap or match the selected
visible object before responding. Generic selector values use PULL340, unlike
the custom PUSH branch. Forced boot/nonforced O, list order, already-open state
and generation distribution must be qualified before choosing a puzzle repair.
The missing43595 object reset is separately disabled by renumbering.

The bloodrune is a real type25 travel object:43515 at43611 leads to43652;43583
at43652 returns to43611. Their command is ENTER7. TOUCH320 rune43506@43579
leads43594 and43528@43621 leads43536. Mirrors43582 at43566/43613 return43502.
All use value2=−1 for unlimited charges. These are command routes, not proof of
deciphering a ritual. Full shared code distinguishes matching command/object,
arena separation, destination conversion, actual teleport and item charges.
Qualify successful actor arrival instead of assuming every accepted command moved
the player. Floating abyss-rock prose alone is not a NO_GROUND flight requirement.

Eligoth43576@43682 has two following golems and imported monolith360/mace23805.
His CMD_DEATH attempts to create43580 and place it in his room; the fixed portal
leads43502. Existing read_object authority can refuse creation. Defeat, admitted
creation, successful use, rune closure, rescued cohorts and outpost defense are
different potential endpoints. Epic monolith and zion mace combat/SAY-earth
effects remain separate systems and are not substitute quest receipts.

The literal patrol_shops assignment at43341 belongs to physical aopal room
The Captain's Station, followed by stock43342 and dispatch43343. It is not an
Earth rescue service. The existing accounting-active buy refusal remains, and
the inventory's range-only association is a lead to resolve physical ownership.
Brass uses Earth magma prototype43540 at five foreign magma-lake rooms, cap5;
those deployments do not create Earth discovery or local quest completion.

## Builder and capability follow-ups

| Follow-up | Evidence and intended next step |
| --- | --- |
| ZSQ-EARTH-BADGE-SUPPLY | Q27 consumes43525, whose full prototype exists; global active reset/recipe and maintained-code scans find no world producer. Hydra-lair dialogue and corpse scenery are not a load path. Builder must choose the intended brother/remains location and a scarce legitimate source; qualify generation/UID, admission, reset and supplied proof in a separate fix/news commit before promising availability. Do not add free or unlimited stock. |
| ZSQ-EARTH-CHAMBER-SELECTION | Hidden43697 has five independent O selectors43585–89 with20/40/60/80/100 chances and cap1, while visible43590–94 use materialbutton names. Ordinary nonforced O refuses and can disable chance<100 rows; a forced initial reset can produce several selectors. The custom scan uses the last recognized object in room-list order. Specify intended one-choice-per-generation distribution and renewal, then qualify all choices, retained stock and empty/multiple selector cases before a separate repair. |
| ZSQ-EARTH-CHAMBER-ACTOR | Custom43584 listens to PUSH270 although its generic values say PULL340; room dispatch calls its custom procedure without matching the selected scenery object. Correct choice clears the actor room's north BLOCKED bit, wrong recognized choice creates one of four mobs without a puzzle-specific cap. Confirm exact room/receiver/argument semantics, idempotent shared-open behavior and bounded trap policy; add admitted actor/exit/generation success evidence, not keyword, repeat-spawn or walk-through credit. |
| ZSQ-EARTH-NATIVE-DATA-INTENT | earth.zon248 refers to undefined object43595; mobile43595 exists but is a different namespace. Renumbering disables the missing object reset. Exit43571E has destination−1. Establish the intended missing prop and route, or their intentional removal, before a narrow separate repair/news commit. Preserve maze43589N/S self-loops unless builder intent demonstrates an error; do not guess replacements. |
| ZSQ-EARTH-CUSTODY-AND-KEYS | P43539 belongs to open fixed body43538 at43628; held/worn/nested proof is not current loose preparation. P uses global matching parent, but the successful fresh O places the newest parent first. Charinth G43542/type13 matches a keyed door by exact vnum: has_key accepts loose/HOLD without requiring ITEM_KEY. Sun43534, dial43509, scale43565 and real keys have distinct controls/custody. Qualify exact source/parent UID and successful removal/unlock independently, with supplied/picked/shared-open alternatives. |
| ZSQ-EARTH-RUNES-AND-DEATH | Six type25 routes use actual command IDs TOUCH320 or ENTER7; value2=−1 is unlimited uses, not a permission flag. Bloodrune43515@43611→43652 and43583@43652→43611; death-created43580→43502; two mirrors and two TOUCH runes differ. Existing factory guards can refuse death creation. Qualify actor/source/generation, actual creation and successful arrival separately; no arbitrary rune decipher/closure, boss kill or portal possession credit. |
| ZSQ-EARTH-NARRATIVE-ENDPOINTS | Gromdishar's outpost/escort, captain's survivor/rune missions, Lithibar's rescue, gnome safety and shaman's dead-priest claims have no personal terminal producer. Both native D1 deliveries retire the giver; D text does not call recall or move the player. Builders must specify real endpoint/state/recipient/group/reset policy and optional versus mandatory episodes before enabling these broader stories. |
| ZSQ-EARTH-RENEWAL-ACCOUNTING | Mode0 makes both native outcomes story-only in the current catalog. Any future daily eligibility needs an explicit builder policy; cap1 proof/givers, wandering contacts and D1 retirement constrain repetition. Accounting-active item resets lack durable generation identity and are refused; death read_object and destructive access have separate guards. Qualify admitted stock, settlement, reward and cold recovery under the accounting branch before promising operational daily supply. Map inclusion does not issue items or renew a giver. |
| ZSQ-EARTH-REWARD-WORDING | Captain Q104 says Torm-blessed shield, while reward43562 is named and described as Melkivar; its extra-description alias still says torm. Decide the intended deity and correct only wording or deliberately redesign the reward separately. Preserve accepted identity/history and item power; show the actual reward now. |
| ZSQ-EARTH-FOREIGN-OWNERSHIP | Literal room43341 patrol_shops is physically in aopal, with stock43342 and dispatch43343, not an Earth mission. Range-only inventory assignment is an inspection lead. Future ownership inventory should resolve physical kind/source before claiming local procedures. Preserve the accounting-active hiring refusal. Brass separately spawns magma43540 at five rooms/cap5; Eligoth's imported monolith360/epic and mace23805/zion combat effects remain separately owned. |
| ZSQ-EARTH-HAZARDS-AND-CLUES | Full room flags/exits, underwater areas, traps and keyed/secret/blocked routes are actual evidence; floating-rock and heavy-door prose alone are not flight/strength gates. Lithibar's verse and tapestry are clues, not accepted knowledge. Qualify successful read/control/access/travel and native equipped/breath effects without adding automatic rescue or source credit. |


No native repair ships in this checkpoint. Any selected repair needs its own named
fix commit, focused before/after evidence and prominent PR/news entry. Preserve
native identity, scarcity, hazard/PvP/access policy and historical receipts.
Source-comprehensive mapping does not establish played operational availability.

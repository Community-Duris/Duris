# Temple of the Earth: patrol deliveries, hidden temple and abyss

Priority101 of the original220-zone queue. This is a **source-comprehensive map**;
played accounting/source/access/persistence qualification remains pending. The
[schema3/revision1 journal](../../../areas/story/earth.story.json) adds two independent
story-only outcomes,12 contacts/46 aliases and two optional current-material
rows, without exclusions. The broader boundary, source-only WLD, typed-reference,
title, and path-history reconciliation is in the
[Checkpoint 61 audit supplement](../quest-story-followups/TEMPLE_OF_THE_EARTH_AUDIT_REVIEW.md).
New discovery/encounter/journal/achievement/daily credit requires active, ready
accounting; frozen receipt recovery is separate.

## Source closure

| Source | Complete review and implication |
| --- | --- |
| [Quest source](../../../areas/qst/earth.qst) | All2 Q/22 M/MA families,12 dialogue contacts/46 aliases: patrol/bloodrune/keeper/rescue/riddle/encampment/dead-priest dialogue, exact inputs/rewards and both D1 endings. No personal endpoint for the larger narrated missions. |
| [Rooms](../../../areas/wld/earth.wld) | All198 complete physical records43500–43697:140 full prose/58 headers/2 metadata/636 exact exits/169 relative patterns/14 full exit-text families. Registry43329–43697/mode0. Approach, temple, hydra caverns, illusion maze, abandoned temple, bloodrune, abyss camps, fortress and holding/selector rooms reviewed. All active boundaries and unusual exits retained; Checkpoint 61 also records separate source-only entries from `surf.wld` and `underworld2.wld` into room #43500. |
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

## Direct area-owner questions and conservative recommendations

| Question | Source evidence | Recommendation |
| --- | --- | --- |
| Which public title and command-target aliases should identify this area? | The normalized title “Temple of the Earth” is unique among active area contributors; command-target aliases and owner disambiguation remain open. See the [Checkpoint 61 audit supplement](../quest-story-followups/TEMPLE_OF_THE_EARTH_AUDIT_REVIEW.md) and [zone title](../../../areas/zon/earth.zon#L1). | Keep “Temple of the Earth” and the `earth` stem. Add aliases only after an active command binding and its owner are confirmed. |
| **ZSQ-EARTH-BADGE-SUPPLY.** Where should badge #43525 come from, and how scarce should that legitimate source be? | Q27 consumes #43525 and rewards #43561, but selected active resets, recipes, and maintained-code scans find no world producer; the hydra story and corpse scenery do not create it. See [exact outcomes and current materials](#exact-outcomes-and-current-materials). | Choose the intended brother/remains location and a scarce source before promising availability. Keep generation/UID, admission, reset, and supplied-proof evidence distinct; do not add free or unlimited stock. |
| **ZSQ-EARTH-CHAMBER-SELECTION.** Should each illusion-chamber generation produce exactly one material selector, and which selector should win when several remain? | Hidden #43697 has five independent selector resets (#43585–#43589) with chances from 20% to 100% and cap 1; forced initial reset and room-list scan order can produce/choose among multiple objects. See [broader progression stories](#broader-progression-stories). | Define one-choice-per-generation distribution and renewal, then qualify all choices and empty/multiple retained-stock states before a separate repair. |
| **ZSQ-EARTH-CHAMBER-ACTOR.** What exact command, room, selected object, and receiver argument should govern controller #43584? | The custom controller listens to PUSH although its generic values say PULL; room dispatch does not match the selected scenery object. A recognized choice clears the actor room’s north BLOCKED bit, while a wrong choice creates one of four mobiles without a puzzle-specific cap. See [controller behavior](#broader-progression-stories). | Define receiver/argument semantics, idempotent shared-open behavior, and bounded trap policy. Credit only an admitted actor/exit/generation outcome, not a keyword, repeat spawn, or walk-through. |
| **ZSQ-EARTH-NATIVE-DATA-INTENT.** Are the missing #43595 object reset, negative #43571 east exit, and #43589 north/south self-loops intentional? | ZON line 248 names undefined object #43595 although mobile #43595 exists in a separate namespace; renumbering disables that reset, and #43571 east targets −1. See the [source closure](#source-closure) and [active WLD review](../../../areas/wld/earth.wld). | Confirm the intended prop and route or their removal before any narrow fix. Preserve the maze self-loops unless the owner identifies an error; do not guess replacement VNUMs. |
| **ZSQ-EARTH-CUSTODY-AND-KEYS.** Which exact source and custody events should qualify the two native deliveries and the related doors? | Band #43539 is placed into open body #43538; the reset P command uses global parent matching. Charinth's type-13 #43542 matches a keyed door by exact VNUM; the sun, dial, scale, and ordinary keys have distinct controls. See [exact outcomes](#exact-outcomes-and-current-materials) and [access branches](#broader-progression-stories). | Track exact source/parent UID and successful removal/unlock independently. Keep supplied, picked, or shared-open alternatives distinct from a personal source recovery or accepted delivery. |
| **ZSQ-EARTH-RUNES-AND-DEATH.** Which specific rune, mirror, and death-created portal arrivals should count as story outcomes? | Six type-25 routes use TOUCH or ENTER commands; unlimited charges do not establish permission, and existing factory guards can refuse death creation. The bloodrune directions, two mirrors, and two TOUCH runes have different targets. See [portal and rune analysis](#broader-progression-stories). | Record actor/source/generation, admitted creation, and actual successful arrival separately. Do not award arbitrary rune deciphering, closure, boss-kill, or portal-possession credit. |
| **ZSQ-EARTH-NARRATIVE-ENDPOINTS.** Which patrol, escort, survivor, rescue, safety, or dead-priest stories should receive a personal terminal outcome, and which remain optional? | Gromdishar, the captain, Lithibar, the gnome, and the shaman make broader requests without personal terminal producers; both native D1 deliveries retire their givers but do not recall or move the player. See [broader progression stories](#broader-progression-stories) and [native outcomes](#exact-outcomes-and-current-materials). | Define each real endpoint, state, recipient/group contribution, reset policy, and optional/mandatory status before tracking it. Keep these branches separate from the two existing delivery receipts. |
| **ZSQ-EARTH-RENEWAL-ACCOUNTING.** Should either native outcome become daily, and what actual supply and renewal evidence would that require? | Reset mode 0 makes both native outcomes story-only in the current catalog; proof/giver caps, wandering contacts, D1 retirement, guarded resets, and death-object guards constrain repetition. See the [reset inventory](#source-closure) and [accounting boundary](#accounting-and-validation-boundary). | Leave daily eligibility disabled until the owner defines policy and active-accounting qualification establishes admitted stock, settlement, reward, and cold recovery. Map inclusion alone does not issue materials or renew a giver. |
| **ZSQ-EARTH-REWARD-WORDING.** Should the captain's shield honor Torm, Melkivar, or both? | Q104 calls it Torm-blessed, while reward #43562 is named/described as Melkivar and an extra-description alias still says Torm. See the [native outcome summary](#exact-outcomes-and-current-materials). | Confirm the intended deity and make a separate wording correction, or deliberately redesign the reward. Preserve its accepted identity/history and item power; show the actual reward name now. |
| **ZSQ-EARTH-FOREIGN-OWNERSHIP.** Which of the physical shop-like room, Brass magma sources, and imported boss equipment belong to other area owners? | `patrol_shops` room #43341 and stock/dispatch are physically in `aopal`; Brass spawns Earth magma #43540 at five rooms/cap 5; Eligoth imports monolith #360 and mace #23805. See the [foreign-closure section](#source-closure) and [broader progression](#broader-progression-stories). | Resolve physical kind and source owner before claiming a local procedure or objective. Preserve the current hire refusal and keep foreign stock, epic rewards, and combat effects under their respective owners. |
| **ZSQ-EARTH-HAZARDS-AND-CLUES.** Should the floating rock and heavy-door descriptions remain prose, and what actual read/access/effect outcomes should be tracked? | Room flags, underwater sectors, traps, and keyed/secret/blocked exits are defined in source; prose alone does not impose flight/strength gates, and Lithibar's verse/tapestry are clues without an accepted knowledge receipt. See [rooms and access](../../../areas/wld/earth.wld) and [broader progression](#broader-progression-stories). | Qualify actual reading, control, access, travel, and equipped/breath effects. Do not add automatic rescue or source credit from descriptive text alone. |
| Should the two story-only sidecar outcomes depend on either native delivery, or remain independent? | The schema3/revision1 journal defines two independent story-only outcomes; neither native QST recipe requires earlier topic, recovery, unlock, rescue, combat, or planar-travel history. See the [journal overview](#source-closure) and [exact native outcomes](#exact-outcomes-and-current-materials). | Preserve their independence and the existing unrestricted exact-proof recipes. Add a prerequisite only with an explicit builder contract and a separately qualified accepted endpoint. |


No native repair ships in this checkpoint. Any selected repair needs its own named
fix commit, focused before/after evidence and prominent PR/news entry. Preserve
native identity, scarcity, hazard/PvP/access policy and historical receipts.
Source-comprehensive mapping does not establish played operational availability.

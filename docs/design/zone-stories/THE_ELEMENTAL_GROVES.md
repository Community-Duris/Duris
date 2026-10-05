# The Elemental Groves: three offerings and a separate plaque puzzle

Priority100 of the original220-zone queue. This is a **source-comprehensive map**;
played accounting/source/access/persistence qualification remains pending. The
[schema3/revision1 journal](../../../areas/story/element.story.json) gives three
independent stories/potential dailies,20 contacts/84 aliases and three optional
current-material rows, without exclusions. Native contracts and mechanics are
preserved. All new discovery/encounter/journal/achievement/daily progress requires
active, ready accounting; recovery of frozen receipts is separate.

## Source closure

| Source | Complete review and implication |
| --- | --- |
| [Quest source](../../../areas/qst/element.qst) | All415 lines:3 Q/60 M families,20 dialogue contacts/84 aliases. All exact goals, rewards, giver retirement, sprite/home/sanctum leads, dryad/forrestal conversation and healing narrative reviewed. Each accepted recipe has one input; topics are not individual achievement endpoints. |
| [Rooms](../../../areas/wld/element.wld) | All165 complete records,125 full prose/25 headers/7 metadata/384 exact exits/127 relative patterns/170 full exit-text families. Registry3752–3974/mode1; actual gaps retained. Single boundary3800E↔surface531681W reviewed in full. Elemental fragment routes, reciprocal doors, fall settings, six-key corridor, riddle and valley archway reviewed. |
| [Mobiles](../../../areas/mob/element.mob) | All23 complete records3800–3822 and23 prose families with ID-paired numeric data. Forrestal3819 starts@3851 and lacks SENTINEL; actual placement can wander. Wraith3820@3950 supplies offering3831 in equipment. Local literal source search finds only object3833’s glades_dagger assignment; earth_treant binds foreign23807, not a local guardian. Ordinary XP constants and floating-point tables are unrelated numeric matches. |
| [Objects](../../../areas/obj/element.obj) | All38 complete records3800–3837, including types/flags/values, six plaque inscriptions, every key, both pedestals, distinct cubes, dust, shell, earring and effects. No local type25 portal or type29 switch. Full imported358/750(heavens),55450(wh),78455(caertannad) records and bindings reviewed. |
| [Resets](../../../areas/zon/element.zon) | All144 commands: D34/O14/P8/M63/E20/G5;133 exact/parent-aware and71 full expanded argument/location families. Chance100 for142 total commands/chance60 for2; no reset F, unlike room fall metadata. All parents/caps/conditions and giver/key/wraith/container placements reviewed. P selects a global matching prototype container; the successful preceding O creates the newest matching parent under ordinary list order. Retained stock and future durable parent identity need renewal qualification. |
| Shared execution | [Q loader/goal matching/settled reward](../../../src/world/quest.c), [SAY magic door](../../../src/cmd/actcomm.c), [movement/key/container access](../../../src/cmd/actmove.c), [fall execution](../../../src/world/falling.c), [flags](../../../src/core/defines.h), [reset execution](../../../src/world/db.c), [guarded journal runtime](../../../src/world/zone_story_quest_runtime.c), [bindings](../../../src/specs/specs.assign.c), [dagger/foreign elemental functions](../../../src/specs/specs.elemental_bosses.c), [spring](../../../src/specs/specs.heavens.c) and [epic stone](../../../src/world/epic.c) reviewed with unchanged custody paths. Ordinary key/password actions lack personal story receipts; current preparation is loose inventory, not source provenance. |
| Foreign closure | Global active registry recipe/reset/exit/object/portal scan: exactly3 usable local touching recipes, zero foreign item/reset/recipe groups, one reciprocal surface boundary,713 type25 declarations/no incoming portal. Four imported ordinary/special objects remain separately owned. No local shop file. |

## Exact accepted outcomes

| Forrestal3819 contract | Exact input → output | Consequence |
| --- | --- | --- |
| Q401 | I3808→E20000;D0 | Cloudy key consumed, giver stays; independent story/potential daily. |
| Q408 | I3809→E40000;D0 | Black key consumed, giver stays; independent story/potential daily. |
| Q385 | I3831→I3830+E200000;D1 | Ethereal matter consumed, earring/XP rewarded, this giver retires; independent story/potential daily. |

Experience numbers are native base terms. Existing native caps and frozen solo/
group recipient amounts govern actual settlement. New progress requires active,
ready accounting; legacy reset issuance is guarded. A current reward item is not
proof of an accepted delivery or of the earlier recovery/route used to obtain it.

The actual cloudy key O3808 starts@3854, and black O3809@3823, each cap1/chance100.
They fit3951UP↔3952DOWN and3947UP↔3953DOWN respectively. The same exact key can
open its tower door or be consumed by the forrestal. Both keys have break0.
Supplied proof and shared-open access remain legitimate alternatives; no mandatory
earlier source, puzzle or key-hand-in history is inferred.

The ether wraith M3820@3950 has E3831 in slot18/cap1/chance100. This type12
offering has QUESTITEM and is not a container, despite being called a box in the
reply. The final chamber’s different fixed black cube3832 is type15/state5,
closeable/closed without LOCKED, and contains wand3817/dagger3833. It is not the
quest input. The wraith sits before the final plaque corridor. Getting the cube
does not require completing that treasure puzzle, or either earlier key offering.

## Exploration stories and prerequisites

Rowan west exits3861→3890,3862→3901,3863→3912 and3864→3931 lead to the
earth/water/fire/air fragments. Their sanctum returns3897E→3846,3908W→3836,
3920N→3816,3927N→3889 are native exits. Guardians supply eight distinct keys:

| Fragment | Source → key → actual door pair |
| --- | --- |
| Earth | Earth elemental3811@3900/E3800→3899E↔3898W; ooze3812@3898/E3801→3897N↔3898S. |
| Water | Weird3813@3909/E3802→3910E↔3911W; elemental3814@3910/E3803→3908N↔3910S; G3837 gives a blue waterbreathing shell. |
| Fire | Fire elemental3815@3919/E3804→3921W↔3922E; magma man3816@3921/G3805→3920S↔3921N. |
| Air | Eddy of smoke3817@3925/E3806→3926W↔3928E; kirin3818@3928/G3807→3927S↔3928N; G3820 supplies its horn. |

Each guardian key has cap1/chance100 and break0. Native D2 resets close/lock
the raw3 doors. Their paths retain current movement/visibility/gear/hazard rules.
Air fragment rooms3932–37 and3926 are NO_GROUND sector8, not AIR_PLANE19.
Water includes sector7 deep water and UNDERWATER/sector10 rooms; fire has11.
Worn shell3837 provides AFF_WATERBREATH2048; dust3822 does not grant flight;
reward earring3830 has AFF_PROT_FIRE536870912. Native character/mount status and
falling execution determine access; no universal immunity is asserted.

Baobab3878UP leads to3938–42 with room fall settings2/4/10/6/8, then3943
and the ether tower. Intended pedestal locations3897/3908/3920/3927/3952 share
prototype3818/cap5, each followed by P3810/11/12/13/14; black pedestal3819@3953
has P3815. Both containers are fixed closed state5 with key−1, not locked.
Reset P uses global get_obj_num, but a successful preceding O inserts its fresh
matching parent at the global-list head. The conditional P resolves that new
pedestal in ordinary load order. Retained stock, failed O, cap limits and future
durable parent UID/generation still need renewal qualification. The lookup alone
does not establish a misplaced-plaque bug or authorize a placement repair.

The final corridor3960→3966 uses keys3810/11/12/13/14/15 in that order:
stone/silver/gold/crystal/cloudy/black. These are distinct from guardian and
cloudy/black door keys3800–09. Room3966 displays the assembled riddle and
3966N↔3967S uses key−2 with keyword ending nothing. SAY nothing unlocks the
matching shared reciprocal door; CLOSED remains, so OPEN may still be needed.
Speaking, reading or traversing it records no personal quest event today.
The forrestal’s element verse orders air before fire; it is lore, whereas the
actual gate definitions above control travel. No quest or route mutation ships.

Final room3967 contains epic358 and memory55450 as separate rewards. Its black
container yields combat gear, whose dagger3833 uses glades_dagger, with an undead
wielder hazard and probabilistic on-hit spells. Those spells do not settle a
local quest. Imported spring750@3973 handles exact DRINK spring with spell effects;
its prototype value0 is−1, so effect-level intent needs qualification. Silverleaf
78455@3807/3808 has chance60 O declarations: legacy nonforced O can disable them
while under cap; it is not an offering prerequisite. No scarcity repair is made.

The air route’s downward exits lead into valley3968–74; lady3821@3968 has gown
3834 and no native Q/M contact. Archway3974N returns to local glade path3812.
Protector3822@3800 has two steel daggers. Their lore, ordinary gear and combat
do not add native quest outcomes. Earth3892E loops to itself; builder intent
must be checked before changing that route. No missing local prototype was found.

Cube acceptance says the forrestal may heal the grove and goes to tell the dryad.
Only delivery and giver retirement are implemented. Restore trees, escort sprites,
bring home creatures, obtain kirin permission and reconcile the dryad are possible
future stories requiring deliberate semantic endpoints, not inferred successes.

## Builder decisions and missing capabilities

| Follow-up | Evidence and intended next step |
| --- | --- |
| ZSQ-ELEMENT-SOURCE-PROVENANCE | Floor keys3808/3809 and wraith equipment3831 are different source paths; supplied proof is valid for current hand-ins. Qualify original reset/container/mobile generation, exact UID and first successful native-source custody before adding optional own-recovery episodes. A gift, worn item, transferred trophy or receipt must not manufacture source history. |
| ZSQ-ELEMENT-COMPETING-KEYS | Cloudy3808 and black3809 are both offering inputs and access keys. They have native break setting0; consuming a hand-in can still remove access stock. Qualify supplied keys/shared-open doors and durable consumption/recovery independently. Do not make key offerings prerequisites for the cube or plaque puzzle. |
| ZSQ-ELEMENT-PEDESTAL-PLACEMENT | Five O3818 pedestals alternate with five distinct P children. Shared reset P uses get_obj_num, a global matching-container lookup, rather than the immediately preceding O instance. Confirm intended room ownership and retained stock behavior, then make any generation-aware placement correction in a separate fix/news commit. Do not promise a plaque in each intended sanctum until played qualification. |
| ZSQ-ELEMENT-PUZZLE-EVENTS | Six actual plaque-key gates precede key−2/SAY nothing. The forrestal’s earth/water/air/fire verse is lore; the actual gate order is stone/silver/gold/crystal/cloudy/black. Qualified actor/door generation and successful read/speech/unlock/open transitions are needed for personal episodes. Already-open access and clue possession do not prove solving. |
| ZSQ-ELEMENT-NARRATIVE-ENDPOINTS | Cube acceptance narrates possible healing and a visit to the dryad but retires the giver without changing the grove. Sprites discuss going home; kirin refuses access in dialogue; the valley lady has no Q contract. Builder must choose deliberate restoration/escort/permission endpoints and fair recipient policy before recording those outcomes. |
| ZSQ-ELEMENT-HAZARDS | Air route uses sector8 NO_GROUND, water includes sector7/10 and UNDERWATER flags, fire uses sector11, and baobab rooms have F2/4/10/6/8. Qualify movement/mount/fall/breath/equipped effects and accepted destination evidence. Fairy dust grants no flight; blue shell needs its real worn effect; rewarded earring is protection from fire, not universal immunity. |
| ZSQ-ELEMENT-RENEWAL-AND-XP | Mode1, cap1 proof/giver, wandering and D1 retirement constrain availability. Accounting-active item resets are currently refused without durable generation identity. Qualify guarded issuance, renewal, accepted destruction, frozen solo/group capped XP and reward persistence before promising live daily supply. Listed20k/40k/200k are base terms, not guaranteed payout. |
| ZSQ-ELEMENT-IMPORTED-SYSTEMS | Silverleaf78455 O resets have chance60 and can be disabled by the legacy nonforced O branch; these are ordinary foreign stock, not current offering goals. Confirm intended renewal without changing scarcity. Epic358, memory55450, DRINK spring750 and dagger3833 effects are separate systems; spring prototype level−1 and the dagger’s null-object guard need bounded dispatcher tests before any proposed correction, with separate repair/news evidence. |
| ZSQ-ELEMENT-ROUTE-INTENT | Earth room3892’s east exit loops to itself; the valley archway3974N actually returns to local glade path3812. Confirm whether each route and its wording is intentional, retain native destinations now, and qualify any builder-selected topology/caption repair separately. Ordinary exits described as portals use native movement; confirmed personal travel evidence would need a qualified adapter. |


Any actual mechanic/caption/quest repair requires a separately named fix/news
commit and before/after qualification. None is bundled here. Existing schema3
covers all three accepted deliveries; source/puzzle/restoration episodes need
qualified event adapters and deliberate zone-builder design.

## Validation boundary

Focused source/schema checks and C++ journeys exercise the three independent
receipts, current loose/held/wrong/reward-only supplies, supplied proof without
personal puzzle/source history, replay, cold recovery and all three daily units.
Full117-journal regression, production inventory/audits, maintained build,
changed/staged format and preservation must pass before publication. Played
source/reset/speech/door/XP/accounting/persistence qualification remains pending.

Catalog117 journals/1567 achievements/1438 potential dailies/2191 rows. Native2668
definitions/fingerprint/content revision2/registry and earlier116 maps preserved.
Roadmap100/220 source-comprehensive,120 pending; Temple of the Earth next.

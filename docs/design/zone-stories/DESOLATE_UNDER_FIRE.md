# Desolate Under Fire: comprehensive story mapping

Priority92 closes source review for zone773, registry77263–77465, area
`desolateinv`, reset mode2 and initial CLOSED flag32. The
[schema3/revision1 sidecar](../../../areas/story/desolateinv.story.json) maps
all17 requests to17 cards:16 story outcomes and one guarded purchase service,
16 contacts, no addressed dialogue aliases and27 optional checks (18 current
material rows/nine earlier receipt rows). The
[generated audit](../../reference/zone-story-audits/desolateinv.md) keeps exact
native bindings. New discovery/encounter/journal/achievement/daily credit
requires active, ready accounting; frozen recovery remains separate.
Source-comprehensive mapping does not qualify played transactions or availability.

## Actual repair and news

Separate [fix 27c78dec3](https://github.com/Community-Duris/Duris/commit/27c78dec31370e23452b81e6a4324d11f0633f17)
changes exactly two words in [room descriptions](../../../areas/wld/desolateinv.wld):
Endurance77412 says **southern**, replacing eastern; Courage77414 says
**western**, replacing eastern. These match existing PUSH buttons77348/77337
and blocked exits to77413/77415. No exits, resets, controls, guardians, sources,
item restrictions, payments, rewards or phase behavior change.

Player-facing news: **Desolate Under Fire’s Master trial now gives the correct
south and west directions in the Endurance and Courage chambers.** The extended
[existing regression](../../../tests/async/test_desolate_master_trial_directions.py)
rejects each original caption independently and passes both repaired rooms and
existing normal-Desolate coverage. Exact native bytes are qualified; live
LOOK/PUSH/traversal is not claimed.

## Full source and dispatch closure

| Source | Complete review and implications |
| --- | --- |
| [Quests](../../../areas/qst/desolateinv.qst) | All17 blocks:16 Q/one QA, zero M/MA or addressed aliases. Eight shard rescues, triple-hand bundle, eight-binding bundle, chain, monkey, badge, bones, blade, wheel and coin purchase. Full responses/departure/room echo reviewed; no qc_action or personal kill/phase check. |
| [Rooms](../../../areas/wld/desolateinv.wld) | All165 physical rooms77301–77465;118 full prose families/19 headers/three complete non-exit metadata families/346 exact exits/131 relative patterns/12 full exit-description families. Forest invasion/captives, locked tents/caskets/gates, magician/torch/branch controls, bears/monkey, trial guardians, tunnels, shuttered services, army lore and no-exit shard-check room reviewed. Registry lower bound does not invent rooms. |
| [Mobiles](../../../areas/mob/desolateinv.mob) | All110 prototypes77301–77410,108 full prose families and every numeric tail.34 have no local reset, including request givers77325/77348/77380. Thirteen unique givers have starting placements; hunter owns two outcomes. Imported hunt orc13385 fully reviewed at77311/77328/77333. |
| [Objects](../../../areas/obj/desolateinv.obj) | All96 full records77301–77396, flags/values/effects/traps/descriptions.26 have no local placement, distinguishing reward-only outputs from missing wheel producer. Full imported rune359/bankcounter3097/ring40768 reviewed. Bones77312 and bronze/black-gate keys77311/77384 are NORENT; bindings77345 carry TWOHANDS, not TRANSIENT. Fixed controls/portals remain non-takeable. |
| [Resets](../../../areas/zon/desolateinv.zon) | All332 commands:M159/D64/O44/E30/P16/F9/G9/R1;271 exact/273 parent-aware/182 expanded families. All chances100 retain caps/conditional chains. Nine shard commands include six P, one merchant G and two O in inaccessible stock-check room. Cap9/shared stock is not nine guaranteed fresh offerings. Mode2 renewal is independent of actual materials/recipients. |
| [Shop](../../../areas/shp/desolateinv.shp) | Full single nomadic shop77408 at77432:77326 shard/77356 potion/77388 scroll; types8/9/5 and authored prices/hours/admission/messages. Listed stock is not proof of a paid purchase or renewal. Coin-only quest merchant77380 is a different unplaced giver. |
| Global sources/consumers | All17 active touching recipes are local; no foreign quest consumes these local items. One foreign SurfaceKeeps reset group places hunter120046/sword120029/21567 and potion77366 twice at122629. Potion77366 differs from deep-blue77356. Ring40768 also appears in CHAOS kits. Across713 active type25 objects, seven incoming local-room portals are all local. Foreign possession does not prove a local exchange. |
| Boundary/controller | Static77301 south→217609 is outside current registry world lookup. Full legacy surf217609 says road with NORTH22200; surface2011’s same number instead has NORTH217309. Both exist; do not call this a missing generated room or repair its target without production-source selection evidence. [Normal Desolate dossier](DESOLATE.md), object22291 and O20% source reviewed. [Random-exit callback](../../../src/item/objmisc.c) and loader schedule explain live route/zone-flag mutation, not a player phase receipt. |
| Custom/shared execution | Only literal local special assignment is inn on77442 in [assignments](../../../src/specs/specs.assign.c); inn-prose room77422 is closed. Generic type29 controls are loader-bound in [db](../../../src/world/db.c) to [item_switch](../../../src/specs/specs.object.c). Imported359 epic rune/3097 locker use independent shared handlers. [Locker ENTER/PvP/paid guard](../../../src/item/storage_lockers.c) and [quest exact-root/coin guard](../../../src/world/quest.c) reviewed. Source/reset refusal, visibility/container consumption/portal restrictions, D retirement and mode2 renewal remain qualification boundaries. No unplaced mechanic is enabled. |

Unplaced mobiles:77303–04,77306,77317,77325–26,77331,77335,77343,
77346–49,77358–60,77367,77369,77372–73,77380–81,77384,77388,77390,
77392,77396–99,77401–02,77407,77409.
Unplaced objects:77306,77311,77329,77335,77340,77343,77345,77347,
77351–52,77354–55,77357,77359–60,77362–63,77365,77369–70,77375,
77382,77384,77386–87,77389. These are source facts, not blanket repairs.

## Exact native progression

| Giver / block | Input → accepted result | Explanation and boundary |
| --- | --- | --- |
| Loud Storm Port orc77311 /Q2, campground77358 | I77326 → I77345 + E30000; D | Independent rescue; prisoner wording does not change the native orc. |
| Goblin77312 /Q13,77358 | I77326 → I77345; D | Independent captive, not automatic from orc rescue. |
| Shady Grove orc77313 /Q23,77358 | I77326 → I77345 + E10000; D | Exact separate accepted rescue. |
| Old logger77324 /Q53, cabin77381 | I77326 → I77345; D | Departure is narration/instance retirement, not tracked escort. |
| Monkey hunter77338 /Q93, command post77342 | I77326 → I77345; D | Return monkey first if both outcomes desired; rescue retires selected hunter. |
| Pinehollow logger77370 /Q148,77381 | I77326 → I77345; D | Sixth independent rescue. |
| Monk77385 /Q174, temple77435 | I77326 → I77345; D | Seventh independent rescue. |
| Cleric77386 /Q185,77435 | I77326 → I77345; D | Eighth independent rescue. |
| Beregan77350 /Q116, bar backroom77421 | 8×I77345 → I77389; D | Eight distinct current roots. Supplied bindings fit without eight own rescues. Seven bindings or past receipts cannot substitute. No army-defeat/zone-restoration controller. |
| Jandar77319 /QA34, master chamber77370 | I77373 + I77385 + I77392 → I77384 + I77340 + I77329; D | Three different hands, not heads or duplicate hands. Authored G/E sources do not require own kills/CARVE. QA echo does not credit listeners. |
| Hunter77338 /Q85,77342 | I77315 → I77370 + C15000; stays | Monkey is a container; consuming it can lose its nested chain. |
| Minotaur77327 /Q75, garbage77360 | I77316 → I77311; D | Chain inside monkey; bronze key opens chest77309. Key/loot possession does not prove hand-in. |
| Delegate77365 /Q139,77421 | I77317 → I77387; stays | Badge equipped by scout77339 at77352, slot24. Gift fits; no own kill gate. |
| Dog77383 /Q165,77428 | I77312 → I77351; stays | Hidden garbage bones, no dead-animal/corpse/carve campaign. |
| Halfling77325 /Q67, unplaced | I77331 → C50000; stays | Dark Armageddon blade worn by Alboa at77382, despite drink text. Intended drink/placement unresolved. |
| Driver77348 /Q104, unplaced | I77352 → I77353; D | No exact repaired-wheel producer. Placed Scotson lacks repair recipe; broken wheel/normal Desolate counterparts do not substitute. |
| Travelling merchant77380 /Q158, unplaced | C10000 → I77356; stays | Guarded coin-only service without story/daily credit. Nomadic77408 G/shop stock is a separate declared potion source. |

One thousand native coin units represent one platinum here. D retirement is
distinct from narration and actual world-state victory. All nonterminal material
and history rows are optional. Matching loose possession does not establish
personal first acquisition or source versus gift.

Hidden shard77326 sources:three P in garbage77308 at77360, three P in trapped
weapon pile77310 at77457, one G on nomadic77408 at77432, two O in no-exit
room77465. Keep cap9/conditions; the last room explicitly checks shard load and
is not a player route. Bindings have no reset producer; each rescue consumes
one current shard. Monkey77315 at77394 is closed container value5 with hidden
chain77316 nested inside. Remove chain before offering monkey if retaining
minotaur proof. Bronze key77311 unlocks fixed chest77309 with money, trophy and
bracelet. Black key77384 fits paired raw3/D2 locked gates77343 east/77361 west.

Gromlog hand77373 is G on77323 at77342; Mackama77392 G on77307 at77375;
Alboa77385 E slot18 on77329 at77382. Three different items are required.
Past rescues cannot supply eight roots for Beregan. No personal kill or army
victory prerequisite is added.

## Access, trial and incomplete story intent

Trial:77410 EAST→77411 EAST→77412 SOUTH→77413 WEST→77414 WEST→77415
NORTH→77410. Knighthood77416 is UP from77411. Existing rogues/knight/titan/
behemoths/celestial dragon remain combat; dragon rune359/ring40768/equipment
remain independent loot/epic systems. No quest checks all guardians or awards
trial completion.

Branch77314 PULL77386 west; torch77377 PULL77364 down/soil77376 PUSH77365
up; barricades77371/77391 PUSH77453 down/77454 up. Controls clear BLOCKED;
SECRET/CLOSED/opening/movement are separate. Unplaced rock77354/77369 target
nonexistent77352 west/77441 east. Preserve pending intent. F90 at77376/77/79/83
and F20 at77398/77401 are fall risk, not destinations; loader requires down exit.

Seven fixed type25 portals retain ENTER7/unlimited−1/non-takeability:
77305 at77374→77410;77323 at77410→77374;77393 at77372→77343;
77394 at77343→77372;77334 at77362→77427;77372 at77458→77462;
77374 at77456→77464. Last three destinations have ordinary east returns.
Portal/control success alone is not personal arrival or quest completion.

Inn binding77442 targets tunnel;77422 inn sign says closed. Jandar needs hands
despite heads lore. Cloak77389 long caption says earring; medal77329’s request
caption says tankard. These need exact builder intent before separate repairs.

## Required builder and capability work

| ID | Finding and fair plan before implementation |
| --- | --- |
| **ZSQ-DESOLATEINV-SOURCE-RENEWAL** | Qualify source UID/root/custody, hidden stock, gifts, caps/conditions, mode2 renewal and active item-reset refusal. Repeat lines are not free stock. Keep no-exit shard-check room inaccessible. Sixteen potential dailies need legitimate supply/renewed actors. |
| **ZSQ-DESOLATEINV-CAPTIVE-BINDING-ALLOCATION** | Eight independent rescues consume one shard each; Beregan consumes eight distinct current roots, not eight types or eight own rescue records. Qualify exact count/duplicate rejection, seven-versus-eight, gifts/lost/spent bindings, concurrency/retry/rollback/output. A personal rescue campaign needs separate admitted objectives and actor/group policy. |
| **ZSQ-DESOLATEINV-CONTAINER-CONSUMPTION** | Closed lost-monkey container holds hidden neckchain. Remove chain before consuming monkey if keeping minotaur route. Qualify OPEN/SEARCH, nested destruction/custody/gift/rollback/key chest access. Container/reward does not prove escort/first recovery. No flag/type/pickup change. |
| **ZSQ-DESOLATEINV-INVASION-PHASE** | Normal Desolate random_exit22291 redirects22200 north/77302 south, closes former zone and opens773. Initial CLOSED, source20% and callback100% differ. Legacy surf217609 has two differing sources outside current registry. Verify production source choice/current entrance/status/accepted episode/route generation/return before topology repair. Discovery and hand-in narration do not settle restoration. |
| **ZSQ-DESOLATEINV-RECIPIENT-RETIREMENT** | D retires eight captives/minotaur/driver/Jandar/Beregan. Monkey return before hunter rescue preserves both. Qualify selected NPC instance/epoch, delayed settlement/reset/removal, reconnect/replay, actor/listener credit and restock. Native target reselection repair needs separate proof/fix/news. |
| **ZSQ-DESOLATEINV-UNPLACED-REPAIR-AND-DRINK** | Halfling/driver/quest merchant unplaced. Halfling consumes blade despite drink caption; repaired wheel has no producer; placed Scotson has no repair recipe. Builder chooses exact availability/drink/wheel/fee/reward semantics or retirement. Do not copy normal recipe, substitute IDs, spawn givers or enable unplaced switches. Changes need contract/receipt versioning and separate fixes. |
| **ZSQ-DESOLATEINV-TRIAL-ACCESS** | PUSH ring/secret controls/fixed portals/falls/guardians/epic Rune differ. New optional control/access/trial facts need actor/UID/door/room/direction/prior-accepted state/phase/output and accounting/recovery. No victory from loot or pre-open route. Unplaced rock targets lack exits. Preserve pending intent; only two proven direction words repaired. |
| **ZSQ-DESOLATEINV-SERVICE-AND-CLUE-INTENT** | Coin-only purchase/paid lockers guarded; shop needs live stock/payment/refund proof. Inn assignment targets tunnel77442, closed inn prose77422: builder intent before relocation/reopening. Hands/heads, medal/tankard and cloak/earring are stale captions, not authority to change item kinds. Separate exact prose repairs after intent; transaction extension only deliberately designed. |


Existing schema3 guidance is enough. New source/access/rescue/phase facts need
admitted actor/UID/instance/state/output and accounting/recovery/version proof.
Preserve scarce stock, closed states, unplaced content and PvP restrictions.

## Validation and limits

Existing source/schema and Python/C++ regressions cover all17 bindings,
16 independent outcomes/guarded service, exact sources/flags/controls,
seven-versus-eight bindings, same shard readiness without automatic rescues,
supplied bindings without own rescues, history without material, monkey/chain
nesting, three hands versus wrong/duplicate/held inputs, independent receipts,
replay/cold recovery and no normal Desolate/foreign discovery. Separate
direction regression rejects each original and passes both repaired variants.
Full production suite/all110 journeys/build/format/preservation required.

Catalog110 journals/1582 achievements/1441 potential dailies/2193 rows.
Native2668 definitions/fingerprint/content revision2/registry and earlier109
journals remain intact. Coin purchase classification removes one fallback
achievement; daily count and native completion identities stay unchanged.
Original220-zone queue:92 comprehensive/128 pending; Storm Port Stronghold next.
Exact52154-byte prior PR body archived with SHA-256; previous news retained.
No accounting activation/DB/server/migration/deployment/merge. Projection does
not qualify played offers, child destruction, payment, retirement, phase,
access, hidden source recovery, renewal or database persistence.

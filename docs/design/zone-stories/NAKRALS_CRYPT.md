# Nakral's Crypt: comprehensive source map

Priority84 of the original roadmap. **Source-comprehensive; played qualification
remains pending.** The [journal](../../../areas/story/crypt.story.json) uses
schema3/revision1: five cards bind all five native recipes. Nine contacts retain
all twenty-one dialogue aliases; fourteen optional checks distinguish thirteen
current material checks from an earlier trophy receipt. Discovery has its own
achievement. All new tracking requires **active, ready accounting**, with
frozen recovery separate.

**No native zone or quest repair ships.** Keys, switches, fixed portals, blocked
exits, reset mode and original recipes remain intact. The
[Fields legacy hotfix follow-up](FIELDS_BETWEEN.md#builder-required-follow-up-replace-the-legacy-rift-hotfix-safely)
is retained: old intentional PvP protection is not a mapping defect to remove.
Actual repairs require separate named fix commits and prominent PR/news.

## Complete local and bounded foreign closure

Active `crypt`, zone143, registry14211–14593, physically14300–14593. Native
header `14593 1 0 15 25 1` uses reset mode1. Header levels15–25 do not describe
safe combat: the local mobs include level60 creatures.

- [All11 native blocks](../../../areas/qst/crypt.qst): M6/Q5, with full responses,
  exact quantities, rewards, departure and all21 aliases. No MA/QA, money input,
  type input or unsupported oversized bundle. The hermit's six-item bundle is
  within the supported limit; his completion has no item reward.
- [All294 rooms](../../../areas/wld/crypt.wld):191 complete title/prose families,
  28 headers,653 exact exits,177 relative exit patterns,113 full exit-text
  families and23 complete non-exit metadata families. All repeated memberships,
  terrain, current records, directions, keys, flags and destinations reviewed.
  Forest/river/lake, graveyard/tombs, crypt/banquet/laboratories/cells/morgue,
  earthen shaft and flooded cavern are distinct contexts.
- [All59 mobiles](../../../areas/mob/crypt.mob):56 full prose families and every
  full numeric body. No local ACT_TEACHER or epic-teacher table binding was
  identified. Sources following other mobs must be distinguished from the
  previous M leader: the butterfly, imp, magma devil and wolf are F-loaded
  followers, with subsequent proof attached to the current follower.
- [All201 objects](../../../areas/obj/crypt.obj):14300–14339 and14400–14560,
  with all types, flags, values, E descriptions, affects and numeric tails.
  Five visually identical adamantite chunks are distinct prototypes. The two
  glowing mithril collars are distinct input/reward identities. Four switches
  and one fixed teleport have actual shared dispatch; ordinary decoration,
  papers, corpses and potion/staff effects do not create new item-quest receipts.
- [All521 reset commands](../../../areas/zon/crypt.zon):439 exact families,
  447 M-parent-aware families,288 location-expanded groups; D/O/P/M/E/G/F only.
  Every full argument/cap/chance and repeated location reviewed. M-parent grouping
  is an inventory aid: [the F handler](../../../src/world/db.c#L3952) replaces
  the current `mob`, so subsequent G/E goes to that follower. No personal kill
  or CARVE operation is inferred from body-part names.
- Three imported prototypes have bounded full context: [epic stone358](../../../areas/obj/heavens.obj#L3709)
  carried by Nakral, [magical fountain72](../../../areas/obj/heavens.obj#L1000)
  fixed at14466, and [father-spirit55274](../../../areas/obj/wh.obj#L3416) with
  a chance50 floor reset in the laboratory. The spirit's `_noquest_` alias and
  T record are preserved. Its presence does not prove another zone's quest
  was completed or grant this zone a new rescue outcome.
- The sole local literal assignment is [cabin14362 `inn`](../../../src/specs/specs.assign.c#L2331).
  The shared rent handler checks combat, PvP delay and character state and
  performs terminal persistence. This is a service, not hermit completion.
  No local shop file or literal mob/object procedure was identified; dynamic
  type registration and imported procedures still require review.
- All713 active type25 objects and all active native contracts/foreign resets
  referencing local objects/mobiles were scanned. The five touching local-item
  recipes are all owned here; no foreign reset source for local material was
  identified. The only identified portal destination into the zone is its own
  orb14547→14386. The ordinary boundary14300 WEST→98737 has the reciprocal
  [Undead Outpost](../../../areas/wld/unoutpst.wld) EAST→14300. Its entire room
  body was reviewed. These bounds do not prove absence of dynamic/spell routes.
- Cavern14568 UP→265588 points to a room absent from the active registry. Old
  inactive `surf.wld` contains that room, with DOWN→54500, not a reciprocal
  Crypt route. [World renumbering](../../../src/world/db.c#L1531) removes
  unresolved exits. Do not reactivate an old map or choose a new surface
  destination from numerology. This stale reference is a builder finding.
- Shared native/durable offer selection, source/reset admission, SEARCH and
  READ→LOOK behavior reviewed. [Switch registration](../../../src/world/db.c#L3166),
  [item_switch](../../../src/specs/specs.object.c#L309),
  [magic-door speech](../../../src/cmd/actcomm.c#L186),
  [fixed item teleport](../../../src/magic/spell_travel.c#L933),
  [spell_pool](../../../src/specs/specs.heavens.c#L963) and imported
  [epic_stone](../../../src/world/epic.c#L1111) explain real supporting actions.
  They are not assumed personal story milestones or played qualification.

The [generated audit](../../reference/zone-story-audits/crypt.md) retains every
native binding and reset declaration. It supplements this full source review.

## Progression stories and exact sources

| Card | Native transaction | Actual progression and limits |
| --- | --- | --- |
| Hermit's firewood | [Q35](../../../areas/qst/crypt.qst#L35): four14305 sticks + two14315 branches, no R, D1 | Hermit14302 starts in cabin14362. Twelve stick/seven branch floor resets along trail; logs/timber do not substitute. Completion reveals a key clue and makes him depart; no rewarded key or lit stove. |
| Surok's gloves | [Q100](../../../areas/qst/crypt.qst#L100):14494/14501/14521/14536/14540 →14531, D1 | Surok14432 starts in morgue14516. Exact five different chunks despite same visible name; his four-or-five dialogue is imprecise. |
| Statue's token | [Q128](../../../areas/qst/crypt.qst#L128):14498 wings +14500 tail +14526 horn +14534 ear →14539, D0 | Statue14438 starts at14536 and stays after token. The four actual F-loaded holders supply proof; no personal kill predicate. |
| Statue's bracelet | [Q140](../../../areas/qst/crypt.qst#L140):14539 token →14542 bracelet, D1 | Optional prior token receipt explains a route; supplied matching token works without it. Statue departs; receipt cannot replace a spent token. |
| Statue's collar | [Q154](../../../areas/qst/crypt.qst#L154):14556 original collar →14557 enhanced collar, D1 | Hellhound14449 wears input at14535. Reward has identical name but different prototype/stats. Statue departs, competing with bracelet for the same live recipient episode. |

| Exact proof / source | Reset context |
| --- | --- |
| Adamantite14494 /14458 | Cap1 floor in secret alcove beyond14457 EAST. |
| Adamantite14501 /14475 | Cap1 floor in secret alcove beyond14474 NORTH. |
| Adamantite14521 /14491 | P inside fixed wooden chest14480 (`[150,15,0,250]`), reached beyond secret pickproof ironwood gate14490 SOUTH/key14516. Chest has no portable key; preserve its existing flags and shared magic/container access rules. |
| Adamantite14536 /14550 | Cap1 floor beyond14549 EAST secret door. |
| Adamantite14540 /14539 | Cap1 floor in small room reached from14538 NORTH secret door. |
| Wings14498 /14466 | G after F butterfly14420, following M iron golem14418. |
| Tail14500 /14471 | G after F imp14421, following M hyena14419. |
| Horn14526 /14513 | G after F magma devil14431, following M formless mass14430; ring14527 and warm key14525 also belong to devil. |
| Ear14534 /14527 | G after F wolf14434, following M frost dervish14433; iron collar14533 is separate. |

The rusted key14316 already has a hidden cap1 floor reset at14347. It opens
14386 NORTH→14388, with native5% unlock break chance. The hermit's clue is
helpful history, not an ownership or access gate. Iron key14445 starts carried
by orc cook14402; ancient key14475 is worn by orc captain14410; ironwood key14516
and third note14532 are carried by tall dark creature14427. Wooden key14522 is
carried by the skeleton reset at14496. These sources have their own custody and
availability. Key recovery/usage is not a quest receipt; negative/no-keyhole
doors and locked chests are not automatically broken or authorized for repair.

### Investigation and implemented gates

| Supporting action | Existing controller / declared result | Missing personal journal fact |
| --- | --- | --- |
| PUSH gargoyle14323 in14388 | Opens blocked DOWN14388→14399; reciprocal UP also unblocked for this non-secret exit | Actual selected switch UID/generation, successful before/after and actor, distinct from observing an open passage. |
| PUSH loose stone14333 in14399 | Opens blocked UP to14388 | Same; no success from another player's action or repeated no-op. |
| PULL torch14420 in14411 | Opens blocked EAST→14413; reverse wall is separately authored | Correct target/command, actual shared aperture and personal arrival. |
| PUSH throne14505 in14480 | Opens blocked NORTH→14560, switch-item movement caption | Actual transition; no inferred portable throne or new route. |
| Three notes + SAY |14303 in embers14302 at14303 says `junamez`;14457 carried by shaman14403 says `anol`;14532 on tall dark creature14427 says `unaqzl`. Door14509 SOUTH/14518 NORTH uses key−2 and final keyword `junamezanolunaqzl` | READ→LOOK does not record learned fragments. Real speech handler unlocks the exact magic door and reciprocal locked state; no keyword quest achievement or possession-as-reading. |
| Nakral's journal14468 | P inside island14467 at14431; separate `page17` and `page24` E descriptions | Actual reader, selected passage and delivered revised text; collection/gifts alone insufficient. |
| TOUCH orb14547 at14584 | Fixed wear0 teleport, destination14386, command320, charges−1; shared selector/travel/arena restrictions apply | Committed successful arrival, not command observation. Original pickup/charges are preserved. |
| DRINK imported fountain72 at14466 | Shared spell_pool chooses existing magical buff; type13 despite fountain name | Actual selected effect/policy is separate from item quest and expedition success. |
| Nakral's imported stone358 | Existing targeted epic TOUCH/group/zone transaction and eligibility | Committed authoritative claim/beneficiary/revision, not a mere touch, kill or narration. No duplicate item-story credit. |

Graveyard memorials, robbed/magically sealed coffins, laboratory experiments,
formulae, bodies, cells, the final werebeast and crypt scratchings explain the
larger investigation. No local native terminal proves all experiments ended,
all prisoners were rescued, every page read or every creature defeated. Preserve
authored hazards and distinguish narrative inference from admitted events.

## Required builder work and universal capability additions

| Follow-up | Current evidence | Plan and acceptance before implementation |
| --- | --- | --- |
| **ZSQ-CRYPT-LEARNED-WORDS** | Three fragments, real magic-door SAY handler, multi-page journal; READ has no durable learned milestone | Admit reader/target UID/root/keyword/text revision and delivered response; successful unlock must include actor, aperture, exact before/after and reciprocal effects. Test supplied clues, speech without prior reading, already-open/no-op, wrong words, reset, replay and cold recovery. Builder decides optional investigation versus mandatory personal discovery. |
| **ZSQ-CRYPT-CONTROL-ACCESS** | Four fixed switch actions, key gates, terrain/current records, orb return | Confirm selected control and committed transition/arrival separately. Qualify wrong command/target, another player's clearance, equipment/nesting, destroyed key, invisible sources, blocked/shared doors, river/swim/fly failure and teleport/arena rejection. Preserve fixed mobility and combat policy. |
| **ZSQ-CRYPT-SOURCE-RENEWAL** | Same-name chunks are different kinds; F changes current holder; cap1 sources and departing recipients; reset mode1 | Preserve exact prototypes, reset generations, leader/follower association and actual current holder/root/custody. Qualify original versus supplied proof, duplicate wrong chunks, consumption/reward lineage and stock held elsewhere. Bracelet/collar need separate live statue episodes; no static five-candidate promise of unlimited dailies. |
| **ZSQ-CRYPT-COLLAR-PRESENTATION** | Identical named collars have different prototypes and stats | Present original→enhanced clearly; preserve original UID/prototype distinction. Qualify actual worn recovery, loose removal, consumed UID, reward terms/settlement and replay. Enhanced collar must never become a valid original input through display-name matching. |
| **ZSQ-CRYPT-STALE-SURFACE-EXIT** | UP14568→265588 missing active; old inactive room's DOWN points elsewhere; loader removes exit | Builder determines intended modern surface endpoint or intentionally removes stale authored reference. Prefer minimal clarification/removal if no destination is intended. A new route requires reciprocal/terrain/PvP review and played qualification; never load an obsolete surface map or activate travel from lore. Separate fix/news if implemented. |
| **ZSQ-CRYPT-CLUE-CONSISTENCY** | Surok says four-or-five;14446 says north door despite sole south exit; imported father spirit has foreign quest context | Confirm whether intentional clues/loot or copying. Prefer exact quantity/direction explanation and keep spirit's original alias/T/source semantics. No automatic producer, recipe, combat, portal or reward changes. Any native fix must have before/after evidence and named commit/news. |
| **ZSQ-CRYPT-EPIC-INVESTIGATION** | Nakral journal/lab/boss and imported stone suggest larger story; only five native item endpoints exist | Builder defines actual campaign boundaries and source/reading/control/combat/group-claim evidence. Use authoritative zone-touch beneficiary/result and frozen recovery; no personal achievement from group observation or duplicated terminal receipt. Preserve native eligibility and payments. |

Player presentation shows five clear cards, exact counts, visually identical
chunks labeled by source, original versus enhanced collar, missing/ready loose
materials, optional earlier token history and recorded accepted outcomes.
Unsupported reading, word, key, switch, travel and group-claim milestones stay
guidance until authoritative events can support richer ANSI/GMCP progression.

## Verification and remaining qualification

Focused source/schema and existing Python/C++ loader/projection journeys cover
all five receipts, exact four/two quantities, distinct same-name chunks, original
versus enhanced collar, supplied token without personal earlier receipt, spent
materials versus historical progress, replay and cold recovery. The full
production regression, maintained build, changed/staged formatting, source
links and publication preservation checks must pass.

Catalog:102 journals/1585 achievement units/1441 potential dailies/2195 rows;
five former fallback units now have five authored cards, so global unit counts
are unchanged. All101 prior journals/all2668 native definitions/fingerprint/
revision2/registry and original220 queue remain intact. **84/220 source-comprehensive,
136 pending; The Valoisian Castle (`val`) next.** Full goal remains active.

Synthetic receipts qualify projection, not played first acquisition, gifts,
SEARCH/READ/SAY/control/key/GET/offer/reward/collar identity, follower spawning,
travel, combat, group claims, persistence or real daily renewal. No accounting
activation, DB/server operation, migration, deployment or merge occurs here.

# The Transparent Tower: comprehensive source map

Reviewed October 11, 2026. Zone 162, `trnsptow`; roadmap priority 54.
**Source review is comprehensive; live gameplay qualification remains pending.**
Active, ready economic accounting is required for discovery, encounters, journals
and new achievement/daily credit. Frozen obligation recovery remains separate.

The [schema-three journal](../../../areas/story/trnsptow.story.json) explains
all four native exchanges as four independent outcomes, with fourteen contacts
and eleven optional checks: eight current carried-material/count checks and
three earlier receipts. Only the librarian recipe has native daily shape.
The three companion exchanges return the scepter's item kind plus a common
token, and are excluded from native daily candidacy as item exchanges. They
remain meaningful named story outcomes. No recipe, native identity or daily
eligibility changes.

## Reviewed evidence

- All thirty [native blocks](../../../areas/qst/trnsptow.qst): four Q and
  twenty-six addressed M families, including every alias, narrative, offering,
  reward and recipient-retirement flag. No ambient MA or coin recipes.
- All one hundred [rooms](../../../areas/wld/trnsptow.wld), including full
  prose, headers, metadata, extra descriptions and exits. There are seventy
  exact prose groups, ten numeric-header groups, fourteen non-exit metadata
  groups and eighty-six exit families. The physical interval is 16200–16299;
  the registry interval is 16166–16299. Header `16299 1 0 15 25 1` and reset
  mode one are preserved.
- All forty [mobiles](../../../areas/mob/trnsptow.mob), seventy-five
  [objects](../../../areas/obj/trnsptow.obj), and 208
  [reset commands](../../../areas/zon/trnsptow.zon) in 144 exact families:
  M73, E52, O38, D32, P5, G5, F3. No local shops, R-mounted loads or
  ACT_TEACHER prototypes. Every referenced reset prototype/target resolves
  in the active inventory; that does not prove successful runtime admission.
- All five active literal [special assignments](../../../src/specs/specs.assign.c):
  Aceralde 16205 → `transp_tow_acerlade`; objects 16263/16268 →
  `artifact_stone`; globe 16262 → `trans_tower_shadow_globe`; mace 16242 →
  `zion_light_dark`. The old sword binding is commented out.
- Full [tower procedure](../../../src/specs/specs.trnsptow.c), applicable
  [stone effect](../../../src/specs/specs.artifacts.c#L22),
  [mace procedure](../../../src/specs/specs.zion.c#L408), active
  [configuration](../../../src/core/config.h), globe pet setup, recharm,
  mob scheduling, speech-door handling, item portals, trap dispatch,
  key destruction, container loading, native durable offering and relevant
  [epic-touch execution](../../../src/world/epic.c#L1111) reviewed.
- Active foreign source/consumer scans and relevant full foreign prototypes
  reviewed. No foreign Q consumer of the scepter, token or mist key found.
  ConnectorZones does consume nine other Tower equipment items in its three
  class alternatives. See the repository-wide
  [audit supplement](../quest-story-followups/TRNSPTOW_AUDIT_REVIEW.md#typed-dependency-closure).

## Exact native acceptance and player explanation

| Q line / giver | Required loose-carried roots → reward kinds | Recipient | Journal |
| --- | --- | --- | --- |
| 68 / Gullivier 16203 | scepter 16241 → scepter 16241 + token 16264 | Departs | `gullivier-scepter-token` |
| 104 / Devilish 16206 | scepter 16241 → scepter 16241 + token 16264 | Departs | `devilish-scepter-token` |
| 281 / Lisa 16234 | scepter 16241 → scepter 16241 + token 16264 | Departs | `lisa-scepter-token` |
| 195 / librarian 16207 | scepter 16241 + 3 × token 16264 → mist key 16258 | Stays | `librarian-mist-key` |

Each companion returns the same **kind**, not evidence of the same physical
object UID. Three tokens are three matching roots, not three source-distinct
tokens. The librarian does not enforce personal visits to all three companions,
their order, defeating Aceralde, allegiance, a particular return journey, or
original-source acquisition. Supplied proofs and tokens recovered across
different resets remain valid native acceptance routes. D1 governs companion
availability after an accepted exchange; it does not implement freeing every
soul or returning them to a home zone.

Keep each companion receipt independent. The librarian's three optional history
steps are separate checks: one completion step containing three contracts would
mean OR in the current schema, not ALL. None is a prerequisite. Earlier receipts
do not replenish the scepter or tokens after consumption. The terminal four-root
offering fits the existing fourteen-root durable limit and uses no mixed payment.
Holding or wearing the scepter does not satisfy the native loose-carried search.

Gullivier has seven addressed families, Devilish three, librarian nine and Lisa
seven. Topics explain tower/companions/escape/scepter/books/lords; each alias is
shown in the journal. Asking them awards no keyword achievement. Encounters
control visibility. Source hints and optional live supplies remain read-only.
The final narration claims corporeal restoration, defeating Aceralde, freeing
the Lords/Lady and protecting the world. The implemented receipt proves the
four-root exchange and key reward, not those wider transitions.

## Scepter recovery and fragile keys

1. Illithid 16237 starts in bedroom 16241 with warped key 16271, G admission
   100%, cap one. It opens kitchen 16243 EAST ↔ 16244 WEST, steel door,
   closed/locked after D2 reset. White mist 16238 beyond it carries swirl key
   16272, G admission 100%, cap one.
2. Swirl key opens upper stair 16252 NORTH ↔ chamber 16253 SOUTH, ebonwood
   door, also D2. Aceralde starts in 16253, carrying marble key 16246 and
   foreign ioun 906, with lich/vampire followers and their ordinary equipment.
3. The scepter is **inside** white marble desk 16239, O16255, not in Aceralde's
   inventory. Container flags 29 mean closeable, closed, locked and pickproof;
   marble key 16246 is the key. P16241 puts the scepter into the matching desk.
   Both have cap one/100% declared admission. The shared P loader selects a
   living matching container through `get_obj_num`; current custody/location
   need not remain the original sitting room or a newly loaded desk.
4. Desk metadata `T 516 4 3 100` means OPEN512 + ROOM4, acid type4,
   three charges, trap level100. [Opening dispatch](../../../src/combat/trap.c#L464)
   calls the room-wide acid branch. Disarm/survival and actual recovery are
   separate from possession or a later accepted companion exchange.

All four progression keys—warped, swirl, marble and rewarded mist—are ITEM_KEY
with value[1]100: normal unlocking has a hundred-percent break chance and uses
the existing intentional-destruction path. The already unlocked door/container
and destroyed key are different facts. A current-key step may become missing
after a successful use; it must stay optional and cannot serve as access history.
The shared key search accepts a held key or loose carried key; the journal's
current **carried** aid does not pretend to cover every held-key arrangement.
The broken key hidden in the sofa is trash, not a substitute. Secret odd gem
inside the chair is ordinary loot. Neither supplies another native quest.

## Companions, controls, library and exit

Gullivier starts at 16270. From the central green-door side, O16207 at 16233
uses SOUTH to field 16263. Travel north to 16268, find/open hidden DOWN to
16269 and hidden EAST past blue mist dragon 16229 to 16270. O16257 at
16270 returns on SOUTH to 16228. His reward describes eastward tree escape;
actual ordinary WEST instead returns to the dragon route.

Devilish starts at 16275 below Avernus tree 16274. The red-door side reaches
O16204 at 16235, SOUTH → 16271. `SAY illusion` at iron door 16278 WEST,
key −2, unlocks/reveals matching reciprocal sides; it leaves the door closed.
OPEN then WEST reaches 16297, whose O16206 uses EAST to 16228. The hidden
downward purple-obelisk route at 16287/16288 instead uses SCREAM → 16228.

Lisa starts at 16299. O16260 at lower stair 16242 uses SOUTH to shadow-dragon
cave 16298, despite no ordinary south exit. Find/open hidden EAST to Lisa.
O16257 at 16299 returns SOUTH to 16228. Her small oak desk 16261 is locked,
pickproof, key zero, with no declared P contents. Its floral/drawer prose is
an orphan intent question, not a required token prerequisite or established
blocker for Lisa's accepted recipe.

Library approach: fountain 16236 hidden WEST →16237 DOWN slide16256 →16257
→16258, open south mist →16259, south16260, east16261 librarian. Alternatively
stair O16202 at16229 uses WEST →16256. O16203 at16262 uses READ →16240.
Shelves hold trapped ancient books/magical dust, both trash; no native Q wants
them. Book lore is not a quest reward or learned-topic terminal.

Mist key opens 16241 NORTH ↔16248 SOUTH. `SAY reality` at16248 EAST key−2
unlocks/reveals wall and matching reverse; OPEN is still needed. Closet16249
has guardian16239, ordinary treasures, imported rune359 and unique sword67276.
Its O16209 uses SOUTH to drawbridge16212. Native speech gates also retain
ordinary speech restrictions such as silence/water/throat/wraith conditions.
Typing the right word is not evidence that the door or travel actually succeeded.

| Portal object | Declared placement(s) | Command → target |
| ---: | --- | --- |
| 16201 | 16213 | STARE →16228 |
| 16202 | 16229 | WEST →16256 |
| 16203 | 16262 | READ →16240 |
| 16204 | 16235 | SOUTH →16271 |
| 16205 | 16288 | SCREAM →16228 |
| 16206 | 16297 | EAST →16228 |
| 16207 | 16233 | SOUTH →16263 |
| 16208 | 16263 | CRY →16228 |
| 16209 | 16249 | SOUTH →16212 |
| 16257 | 16270 and16299 | SOUTH →16228 |
| 16260 | 16242 | SOUTH →16298 |

These are unlimited-charge item portals in source. Generic item-teleport
dispatch precedes ordinary handlers; direction portals scan room contents,
named non-direction controls must select their actual object. STARE139 is
distinct from LOOK/EXAMINE; READ63, SCREAM32 and CRY53 are distinct commands.
Unused local portal16200 and wooden sign16274 have no active reset and are
not activated by the journal.

Drawbridge16212 SOUTH ↔ connector54167 NORTH is reciprocal. Foreign realm
14209 SOUTH enters Avernus16271 without a reverse route; its staff dispersal
room does not establish a public alternative entrance. Ordinary16229 SOUTH
already reaches drawbridge16212, and its portal redirects WEST, not SOUTH.
Thus raw source permits central16228 EAST→16229 SOUTH→drawbridge without
the mist-key closet. This is a real policy/prose discrepancy requiring builder
intent and actual runtime qualification, not authority to remove an exit.

## Other custom behavior and imported progression

**Aceralde.** His assigned procedure returns FALSE for CMD_SET_PERIODIC.
The shared loader schedules custom mob pulses only after a TRUE registration.
Its recharm body is reachable at CMD0 but not registered through that standard
path. Ordinary combat/necromancer behavior is separate; this does not prove
that the boss never fights or the scepter quest cannot finish. `recharm_ch`
handles other masters' co-room NPC pets, not unrestricted player mind control.
Any repair needs intended cadence, actor/target policy, iteration/lifecycle
safety and balance qualification before a separate `fix:` commit/news item.

**Globe and mace.** THARKUN_ARTIS=1 selects the active globe periodic branch:
worn globe gives stone property45/60 seconds and can wither eligible nearby
non-group targets. The shared pet setup checks the globe in HOLD/WIELD and
can make newly set-up charm permanent when not restoring pets. The historical
inactive branch's unreachable SAY-invisibility code is not an active deployed
invisibility bug. Globe16262 has an active declared G source on Chronomancer
81454 in `ceofutur`, not a local Tower reset. Mace16242 comes from Chernovog
58835 in `barovia2`; its alignment-sensitive speech/melee/buff procedure is
separate from Tower acceptance. Prototype owner is not reward/source owner.

**Stone bindings.** Note16263 and discarded steel-wire trash16268 have the
stone-effect binding, but their source wear flags are take-only, with no active
declared reset or other discovered producer. The shared prototype loader adds
HOLD to takeable objects, so these source flags alone do not prove that a held
instance could never run the worn-object procedure. The actual wearable steel wire
16270 in the closet has no such literal binding. Note prose promises steel
expiration without implementing that result itself. Confirm intended object,
held/worn behavior, source and expiry policy before moving a binding or spawning it.

**Epic rune.** Imported359 is assigned `epic_stone`. Its shared procedure
initializes zone from loaded location, checks current magic/zone completion,
peace, staff status, level eligibility, participant limits and source-zone
placement, then submits a durable zone-touch transaction. Eligible co-room
group players are included in its payload. Successful result processing makes
the stone powerless and publishes the committed zone touch; recovered claims
avoid duplicating its publication. Story integration should consume an existing
committed result with owner/episode/participant/replay policy, not another
reward path or a typed TOUCH. This rune is neither a fifth native Q nor proof
of all companion rescues or the player's successful escape.

The shared periodic [absorption loop](../../../src/world/epic.c#L863) also
extracts other smaller/equal epic stones and objects whose first affect is
APPLY_LEVEL from the same room or carried list. It then reads the extracted
object's affect and advances using its `next_content`. The shared
[extraction](../../../src/world/handler.c#L3288) unlinks and releases that
object; room/carried unlink clears `next_content`. This is a concrete source
lifetime/traversal defect, separate from durable touch reward settlement.
No live crash or particular player loss was reproduced. Plan a separate shared
epic repair: capture the next live identity before extraction, decide one
destruction reason per object, avoid all post-release access, and qualify
consecutive victims, unrelated objects, room/carried lists, nested contents,
source removal, duplicate eligibility, storage reuse and destruction/recovery
accounting. Do not infer safe item conservation from the touch transaction alone.

## Capability expansion and qualification plan

| Need found here | Proposed accepted evidence and policy | Required failure/recovery proof |
| --- | --- | --- |
| First source recovery versus handoff | Item UID/revision, reset/source actor/container/owner and acquisition/transfer kind; retain supplied-route acceptance | Same-kind replacement, nested/held items, moved desk, foreign or player transfer, competing looters, crash/replay |
| Fragile key access | Actual lock/container UID and accepted state transition tied to key destruction intent/result | Already unlocked, wrong/held key, failed break settlement, disconnect/reset/recovery; missing key never erases successful access |
| Spoken doors and command portals | Bound controller UID, command/word, previous/new exit state, successful arrival and route episode | Silence/wrong object/word, hidden or already unlocked door, still-closed state, movement rejection, reset/replay; no typed-command credit |
| Companion campaign | Builder-defined ALL family, source-distinct tokens only if chosen, recipient retirement/transformation and episode policy | Supplied/repeated-source tokens, any visit order, prior wipe/reset receipts, partial rescue, duplicate rewards, no false OR |
| Escape/world protection | Confirmed player arrival at scoped exterior, actual actor/world transitions, independent of librarian receipt | Ordinary shortcut versus closet route, death/no arrival, foreign room ownership, recovered delivery without a fresh return |
| Epic and artifact effects | Reuse durable epic result; effect/equipment/controller/target lifecycle observations for selected named milestones | Group ownership, source-zone/level/peace failures, recovered claim, buff expiry/removal, pet restore and inactive branches |
| Periodic absorption and destruction | Safe live-instance traversal and single destruction decision, with conserved ownership/tree/recovery evidence independent of touch awards | Adjacent victims, overlapping eligibility, cleared links/released storage, carried/room/container contents, interrupted settlement and replay |
| Availability and custom pulse | Actual source admission/recipient identity/reset generation; qualified custom scheduler and safe target lifecycle | Cap/chance/conditional reset rejection, retired/wandering actor, lag/extraction/movement/storage reuse and competing players |

Use universal adapters with a builder-authored optional per-zone sidecar defining
controller/source ownership and campaign meaning. Source scans reveal the key,
portal, special and recipe candidates dynamically; they cannot choose whether
misdirection is intentional, a soul is actually freed, or a supplied token should
be disallowed. Keep objective evidence, receipt completion and current supply
presentation separate. The existing terminal policy stays active while adapters
are unqualified. Do not activate guarded capabilities through documentation alone.

## Pending native findings and repair plans

**No native Tower repair ships in this checkpoint.** New hints and the journal
are additions; the following are pending proposals, with scope and intent still
to resolve. Any later actual repair requires a separate clear fix commit, PR/news
trigger/before-after/validation and a player-facing sentence for the shipped change.

| Finding | Evidence and balanced interpretation | Planned next step |
| --- | --- | --- |
| Unregistered Aceralde custom pulse | Assigned procedure rejects periodic setup; ordinary combat remains possible | Confirm intended behavior/cadence; actual-procedure original-fails/repaired-passes scheduler/lifecycle/balance tests |
| Shared epic absorption lifetime | The loop reads/advances after `extract_obj`, which clears list links and releases storage; no live crash reproduced | Separate shared epic fix with actual-procedure original-fails/repaired-passes traversal, single-destruction, identity and recovery/accounting tests |
| Closet-only escape claim | Ordinary western-stair SOUTH reaches drawbridge; WEST portal does not intercept it | Qualify live entry/return; choose wording or intended portal/route policy before content changes |
| Illusion-themed direction conflicts | 16230 prose west vs actual east; 16264 prose N/E vs actual N/W; 16277 last east vs actual west; 16259 eastward library extension absent; Gullivier eastward return vs south portal | Builder decides intentional deception versus mistake; chosen repair gets exact source diff, route/portal tests and explicit news |
| Stone effect on unplaced trash | 16263/16268 bound, source take-only/no declared source (loader adds HOLD); wearable16270 unbound | Decide intended note/material/held/worn item and lifecycle; verify source/equip/expiry before binding repair |
| Lisa desk/unused portal/sign | Closed key-zero desk has no declared contents; portal16200/sign16274 unplaced | Confirm intended orphan puzzle/scenery; do not invent a prerequisite, reward or spawn |
| Narrated full rescue/restoration | Librarian receipt proves only exchanged roots/key; no full rescue/world transition recorded | Choose actual native closure and scoped campaign semantics before exposing a terminal |

## Explicit builder questions and recommended resolutions

These questions identify source ambiguities for owner review; their
recommendations preserve current authored behavior until a deliberate change
is approved.

| # | Builder question and direct evidence | Conservative recommendation |
| ---: | --- | --- |
| 1 | Should the three companion exchanges remain independent outcomes that return the scepter's item kind plus one token and retire only the accepted companion? <br> Source: [native QST](../../../areas/qst/trnsptow.qst#L68), [story sidecar](../../../areas/story/trnsptow.story.json#L60) | Preserve each receipt and the current item-kind acceptance separately; do not infer physical-item identity, a visit order, or completion of every rescue. |
| 2 | Is the librarian's exact scepter-plus-three-token receipt intended to remain independently acceptable, without proof of the three companion visits? <br> Source: [librarian QST block](../../../areas/qst/trnsptow.qst#L195), [journal contract](../../../areas/story/trnsptow.story.json#L250) | Keep the four-root recipe and its single key reward exact; do not add visit-order or source-history requirements without a builder decision. |
| 3 | Should the ending continue to claim restoration, Aceralde's defeat, and the Lords' rescue when the native terminal only records the librarian exchange and key? <br> Source: [native QST dialogue](../../../areas/qst/trnsptow.qst), [pending findings](#pending-native-findings-and-repair-plans) | Keep the current claim separate from the receipt; choose a scoped native endpoint before awarding campaign-wide completion. |
| 4 | Are the warped, swirl, marble, and mist keys intentionally breakable at 100% when used on their matching locks? <br> Source: [key objects](../../../areas/obj/trnsptow.obj), [lock resets](../../../areas/zon/trnsptow.zon) | Preserve destruction as the authored access result; any recovery policy should recognize the opened state separately from current key possession. |
| 5 | Should the kitchen, ebonwood stair, and related locks continue to reset closed/locked under the zone's D2 reset, with their matching key owners? <br> Source: [Tower doors](../../../areas/wld/trnsptow.wld), [reset stream](../../../areas/zon/trnsptow.zon) | Keep each lock/key pair distinct and do not add a second copy or route until reset availability and actual access are qualified. |
| 6 | Is the scepter's location inside the pickproof marble desk, plus its room-wide acid trap, the intended Aceralde recovery sequence? <br> Source: [desk and objects](../../../areas/obj/trnsptow.obj#L629), [reset P command](../../../areas/zon/trnsptow.zon) | Keep desk custody, Aceralde's carried keys, trap resolution, and recovered scepter custody as separate facts. |
| 7 | Should ordinary travel from #16228 through #16229 south to drawbridge #16212 remain possible without visiting the mist-key closet? <br> Source: [Tower rooms and exits](../../../areas/wld/trnsptow.wld#L722), [closet portal](../../../areas/obj/trnsptow.obj#L106) | Preserve the authored WLD edge; qualify the live route and adjust story wording or route intent only after owner review. |
| 8 | Are the documented illusion/room directions intentional despite several prose directions conflicting with their actual hidden exits and portals? <br> Source: [route prose and WLD records](../../../areas/wld/trnsptow.wld), [control objects](../../../areas/obj/trnsptow.obj) | Keep the exact control/exit data and mark each discrepancy for a separate wording-versus-route decision. |
| 9 | Should all eleven reset-placed travel objects keep their current exact command/target pairs, including two different objects sharing the SOUTH-to-central return? <br> Source: [portal definitions](../../../areas/obj/trnsptow.obj), [portal resets](../../../areas/zon/trnsptow.zon) | Preserve each object binding, command, target, and placement; keep unused #16200 distinct from active portals. |
| 10 | Is Lisa's key-zero locked desk, the unplaced sign/portal, and the trapped library book-and-dust scenery intentionally optional rather than another quest gate? <br> Source: [Lisa's desk and scenery](../../../areas/obj/trnsptow.obj#L978), [Lisa/library rooms](../../../areas/wld/trnsptow.wld) | Do not invent contents, prerequisites, or rewards; record a new source-backed purpose before adding one. |
| 11 | Is Aceralde intended to reject periodic setup while retaining the current ordinary combat behavior, and is the high-drop list entry dormant? <br> Source: [assigned procedure](../../../src/specs/specs.assign.c#L380), [procedure](../../../src/specs/specs.trnsptow.c), [commented high-drop code](../../../src/item/randomeq.c#L141) | Confirm the desired boss pulse and drop policy independently; do not describe the commented list loop as a live reward. |
| 12 | Should take-only objects #16263 and #16268 keep `artifact_stone` despite lacking an active declared source, while wearable #16270 has no such binding? <br> Source: [bindings](../../../src/specs/specs.assign.c#L1373), [object flags](../../../areas/obj/trnsptow.obj#L1012) | Decide the intended object and wear/hold lifecycle before moving the binding or creating a producer. |
| 13 | Should globe #16262 retain its active shadow-globe effect and its outside Chronomancer reset source? <br> Source: [assignment](../../../src/specs/specs.assign.c#L1722), [Chronomancer reset](../../../areas/zon/ceofutur.zon#L313), [Tower item](../../../areas/obj/trnsptow.obj#L994) | Keep effect behavior and prototype ownership linked; qualify withering and pet interactions before changing equipment policy. |
| 14 | Should imported epic rune #359 remain an optional Tower touch item, separate from native quests and rescue/escape claims? <br> Source: [Tower reset](../../../areas/zon/trnsptow.zon#L146), [epic binding](../../../src/specs/specs.artifacts.c#L22) | Preserve the existing durable touch path and consume only its committed result if story integration is later chosen. |
| 15 | Should the three ConnectorZones Adventurer alternatives continue to consume the nine exact Tower ingredients recorded in its QST and sidecar? <br> Source: [mage alternative](../../../areas/qst/connectorzones.qst#L175), [warrior alternative](../../../areas/qst/connectorzones.qst#L192), [priest alternative](../../../areas/qst/connectorzones.qst#L209), [typed sidecar contracts](../../../areas/story/connectorzones.story.json#L305) | Preserve the three independent five-item bundles and their item VNUMs; no Tower scepter/token/key consumer was found. |
| 16 | Should the active outside reset suppliers keep granting local Tower mace #16242, globe #16262, leg plates #16222, and earring #16248? <br> Source: [Barovia](../../../areas/zon/barovia2.zon#L396), [Future Ceothia](../../../areas/zon/ceofutur.zon#L313), [Shaboath](../../../areas/zon/shabo.zon#L366) | Keep all four supplier records and their local item identities explicit when either area changes. |
| 17 | Should imported #359/#67276/#906 and CHAOS starter-profile references to #16217/#16269/#16244 remain dependencies of Tower prototypes? <br> Source: [epic rune reset](../../../areas/zon/trnsptow.zon#L146), [unique sword reset](../../../areas/zon/trnsptow.zon#L156), [ioun reset](../../../areas/zon/trnsptow.zon#L238), [CHAOS profiles](../../../src/account/chaos_eq_data.h#L91), [other profile](../../../src/account/chaos_eq_data.h#L176), [final profile](../../../src/account/chaos_eq_data.h#L283) | Preserve the imported object owners and update both standard/enhanceable starter tables together if any referenced Tower VNUM changes. |
| 18 | Should Tower retain the reciprocal ConnectorZones crossing and one-way Realm entry while inactive Surf #245010's north approach stays unregistered? <br> Source: [active boundaries](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L253), [source-only Surf exit](../../../areas/wld/surf.wld#L944417) | Preserve the two active boundary relationships and the source-only Surf edge as separate evidence; do not call the Surf edge active ingress or imply a Realm return. |
| 19 | Is the `guild_guard` class gate keyed to home room #16283 intended for Tower, although the reset there is a black abishai and no local `guild_guard` binding was found? <br> Source: [shared branch](../../../src/specs/specs.guards.c#L199), [Tower reset](../../../areas/zon/trnsptow.zon#L287) | Treat it as a conditional source-code collision, not an active local gate, until an actual assigned guard and intended route are established. |
| 20 | Should the six source-only Luiren shop records keep their `in_room` numbers that overlap Tower rooms, given the file is unregistered and uses rejected old `O` format? <br> Source: [Luiren SHP records](../../../areas/shp/luiren.shp#L57), [loader fallback](../../../src/economy/shop.c#L3267), [old-format parser](../../../src/economy/shop.c#L3322) | Keep Luiren separate from active Tower shops; resolve loader/fallback ownership before any shop aggregation or source-list change. |

## Verification and practical limits

Focused regressions cover exact bindings, full native classifications, optional
history/count logic, source/container/key/portal/trap facts and pending procedure
evidence. Actual C++ journal journeys check visibility, exact current supplies,
two versus three tokens, worn scepter, independent outcomes, supplied final
acceptance, no false campaign/escape credit, read-only rendering, replay and cold
recovery. Production generation and other journal/core/build/format checks retain
the catalog's native definitions, fingerprint, revision and daily totals.

Live key destruction, traps/survival, companion retirement/reset, desk custody,
spoken doors, every portal/return, boss combat, epic-touch group settlement and
player/account persistence remain unqualified. No DB migration, account activation,
server operation, deployment or merge is required by this source checkpoint.

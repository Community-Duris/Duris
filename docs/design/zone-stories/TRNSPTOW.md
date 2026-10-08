# The Transparent Tower: comprehensive source map

Reviewed October 4, 2026. Zone 162, `trnsptow`; roadmap priority 54.
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
  The [generated evidence index](../../reference/zone-story-audits/trnsptow.md)
  is navigable evidence, not a substitute for this semantic audit.

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

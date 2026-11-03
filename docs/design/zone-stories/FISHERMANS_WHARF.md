# Fishermans Wharf: comprehensive source map

Reviewed October 4, 2026. Zone 889, `fishermans_wharf`; roadmap priority 63.
**Source review is comprehensive; live gameplay qualification remains pending.**
Active, ready accounting is mandatory for discovery, visible encounters,
journals and new achievement/daily credit. Frozen obligation recovery is separate.

The [schema-three journal](../../../areas/story/fishermans_wharf.story.json)
maps all five accepted native outcomes as two stories and three requests, with
thirteen contacts, forty-four native topic aliases and ten optional checks.
The useful progression is bottle cleanup → bait and guide delivery → line →
adult supplies → cast net, alongside frog jelly → worn snorkel and Baltik's
eight-item bundle → cavern totem. These are independent accepted requests;
supplied exact items require no personal kill, first source or earlier receipt.
No native zone or quest repair ships in this checkpoint.

## Reviewed evidence

- All thirteen [native blocks](../../../areas/qst/fishermans_wharf.qst):
  eight MA response families and five QA outcomes. All forty-four aliases are
  retained. No additional local literal topic or native terminal was found.
- All seventy consecutive [rooms](../../../areas/wld/fishermans_wharf.wld),
  88900–88969: twenty-five exact prose groups, fifteen headers, sixteen
  non-exit metadata groups, eighty-nine numeric exit families and forty-three
  exit text/keyword families. Full prose, properties, exit text and direction/
  target membership were reviewed. No room has an empty description. The sole
  extra non-header property is tree-top `F 50`; no local current is declared.
- All twenty-three [mobile prototypes](../../../areas/mob/fishermans_wharf.mob),
  twenty [object prototypes](../../../areas/obj/fishermans_wharf.obj), and all
  119 [resets](../../../areas/zon/fishermans_wharf.zon) in 106 exact argument
  families: M78/O19/G10/D6/E6. All local references and ordinary exit targets
  resolve. Header 88969 1 0 15 20 1 retains reset mode one; registry bounds are
  88871–88969. All mobiles stay within the zone; the five native recipients
  are sentinel. No local teacher, literal special or inn/arena terminal adds a
  quest. The sole [shop](../../../areas/shp/fishermans_wharf.shp) is Widoc's.
- Bounded active native-consumer, reset-producer and shop scans cover all local
  input/output kinds plus imported crystal skull 31320. No foreign material
  producer or additional local material producer was found. Two foreign
  consumers are Dibbly's snorkel buyback in Newhaven and Qin's five-skull quest
  in Dream. All 713 active teleport-object prototypes were scanned; no foreign
  fixed destination enters these seventy rooms. The ordinary reciprocal
  Surface boundary was reviewed in both directions.
- Shared addressed quest dispatch, exact root allocation, credit ownership,
  reset admission, ordinary boat movement, underwater arrival and drowning,
  worn equipment effects, key lookup/unlock, command-time chance falling and
  fishing selection/reward publication were reviewed. Skull type/flags,
  computed/literal assignments and bounded shared handlers were considered;
  lack of a local special alone is not proof of lack of shared behavior.
  The [generated review index](../../reference/zone-story-audits/fishermans_wharf.md)
  retains exact native contracts and candidate classification.

## Exact requests and progression

| Recipient/block | Exact native offering → reward | Journal treatment |
| --- | --- | --- |
| Baltik 88902, [QA14](../../../areas/qst/fishermans_wharf.qst#L14) | I88913 + four I88910 + three I88911 → I88914; D0 | Eight-item supplies story; petrified fanged snake totem opens a later cavern route. No XP reward is declared. |
| Dimbled 88904, [QA49](../../../areas/qst/fishermans_wharf.qst#L49) | I88900 → I88904 + E7500; D0 | Guide-delivery request; line is a useful adult-request input. No guide-reading or escort terminal. |
| Adult fisherman 88905, [QA68](../../../areas/qst/fishermans_wharf.qst#L68) | I88902 + I88904 → I88906 + E7500; D0 | Combined supplies story, with optional earlier bait/line receipts. Net reward is not a catch endpoint. |
| Old fisherman 88906, [QA94](../../../areas/qst/fishermans_wharf.qst#L94) | Four I88912 → I88902 + E7500; D0 | Bottle-cleanup request; four separate exact items, accepted together. |
| Eager fisherman 88907, [QA114](../../../areas/qst/fishermans_wharf.qst#L114) | Four I88909 → I88905 + E7500; D0 | Frog-jelly request; snorkel must be worn for its breathing effect. |

Baltik starts in shelter 88923 and Dimbled at pine tree 88922. Two adult and
two old fishermen, plus four eager fishermen, start at the pier ends 88906 and
88908. Duplicate actors of one giver kind share one definition; they are not
independent achievements. All remain after acceptance. Native MA requires an
addressed, mutually visible eligible recipient. MA/QA affects response audience;
it does not grant observer credit or an achievement for each keyword.

Baltik describes breakfast, fire-building and comfort, but the actual terminal
is an eight-root bundle. Four separate stick bundles are required even though
each item's name already says "bundle"; three separate pelts and one egg are
also required. Native allocation rejects reusing one object pointer for several
goals. Eight is within the [fourteen-root durable limit](../../../src/world/zone_story_quest_production.h#L15).
Incomplete sets remain with the player; loose current ownership and accepted
durable offering must still be qualified in a played journey.

The adult's optional cleanup and guide receipts explain two useful material
routes. A player holding supplied exact bait/line can complete that request
without either receipt. Historical completion does not refill spent bait,
restore line or reserve a net. Each of the five accepted outcomes has separate
credit. All five native definitions retain static repeatable/potential-daily
classification under the all-I/reset-one heuristic; this does not certify
actual admitted supply, cap availability, recipient presence or daily renewal.

## Declared materials, shops and actor identity

| Exact kind | Actual producer/placement | Qualification boundary |
| --- | --- | --- |
| Guide 88900 | O100/cap1 at pier 88903, reset33; visual-map extra description | Delivery uses item identity; reading or finding a destination is not required. |
| Stick bundle 88910 | Four O100/cap4 placements at 88910/88915/88951/88953, resets34/35/47/48 | Global cap four equals the full recipe quantity. Current recovery and supply matter. |
| Soft pelt 88911 | G100/cap3 after beaver M at 88939/88942/88944, resets93/97/100 | Ordinary preloaded material; no actual death-generated skinning proof. |
| Glass bottle 88912 | Six O100/cap6 placements at 88924/88925/88926/88927/88929/88934, resets36/38/39/40/43/44 | SECRET and FLOAT are actual flags despite the "sunk" narration. Actual perception/recovery needs qualification. |
| Frog jelly 88909 | G100/cap5 after bullfrogs at 88924/88927/88955/88957/88959, resets81/83/113/118/123 | Preloaded material, not an accepted breeding/spawning or fishing action. |
| Eagle egg 88913 | G100/cap1 at reset126 after guarding eagle 88910 M at tree top 88960, reset125 | Flying eagle 88911 has the same displayed name/keywords, but no egg declaration. |
| Bait 88902 / line 88904 | Old-fisherman QA94 / Dimbled QA49 only in reviewed active sources | Earlier receipt is optional, and current exact item is required for the adult's bundle. |
| Snorkel 88905 / totem 88914 | Eager-fisherman QA114 / Baltik QA14 only | Reward possession is distinct from active breathing, lock acceptance or surviving arrival. |
| Cast net 88906 | Adult QA68 and Widoc's declared shop stock | ITEM_CONTAINER15, capacity150, value[3]500, prototype weight−75; no reviewed cast-net fishing procedure. Builder intent and actual weight behavior need qualification. |
| Pole 88903 / paddle boat 88901 | Widoc shop; adults E-poles at hold18/back27; boat O at88927 | Pole is selected by exact vnum when carried loose. Boat uses ordinary ITEM_BOAT22 movement; one-passenger lore is not a ship occupancy contract. |

Many quest materials, including guide, bottles, egg, pelts and jelly, are
ITEM_TREASURE8, not ITEM_BOOK23. The pole's present type is ITEM_SPAWNER39,
with zero values, and hold/back wear flags; actual fishing selection is by the
explicit recognized vnum list, not a fishing-pole type. Its adult reset slots
18 and 27 match hold and back. No active type-specific spawner dispatch for
this pole was found in the bounded source review. Do not silently change types
or equipment slots to match names; compatibility and builder intent come first.

The negative net shell weight is a balanced intent lead, not proof of an
exploit. Shared container code explicitly handles negative weight and has a
separate value[4] reduction policy; this net has value[4]zero. Builder review
and actual capacity/nested-transfer/encumbrance tests must precede any change.
The net is a real carrying-container reward for helping the anglers, so that
mission remains credited; ordinary shop availability does not turn the mission
into an uncredited equipment-conversion service.

## Physical access, effects and hazards

Surface 567269 north ↔ Wharf88900 south is the only ordinary foreign boundary.
Trail88901 northeast reaches88902 and northwest reaches88952. The shack pair
88904 west ↔88909 east and shelter pair88923 south ↔88969 north reset closed
but unlocked. Tree88922 up ↔88960 down leads to the guarding eagle.

Actual sectors are hills six, city six, inside two, forest fifteen, water-swim
eighteen, water-noswim five, field ten, underwater four and underwater-ground
four. Windy lake88939–88943 uses WATER_NOSWIM. [Movement](../../../src/cmd/actmove.c#L966)
recognizes carried/worn ITEM_BOAT, appropriate flight/levitation and other
actual character/mount alternatives. Wind warnings do not by themselves
declare capsize/current events; local current properties are absent. Boat
presence is preparation, not an accepted boarding or occupancy receipt.

Lake88942 down→88961,88941 down→88962 and88939 down→88963 have reciprocal
upward routes. From88962, descend to88964 and then88965; east/west side caverns
88966/88967 branch from88965. The lair is south88968. All88961–88968 have
ROOM_UNDERWATER. The upper four and lair use TWILIGHT;88965–88967 are DARK.
Lair mystic-light prose is not MAGIC_LIGHT. No local room is SAFE. NO_MOB
restricts ordinary mobile movement and does not make reset occupants harmless.

The [snorkel](../../../areas/obj/fishermans_wharf.obj#L84) is ARMOR9 with face/
nose wear flags and AFF_WATERBREATH2048. The [belt](../../../areas/obj/fishermans_wharf.obj#L253)
has the same effect, waist wear flag and aquaelf E-slot13 at88964. Actual
[equipment application](../../../src/magic/affects.c#L1527) applies worn object
bitvectors but ignores a held non-weapon with other wear flags. Carrying or
merely holding the snorkel/belt is not breathing. Actual effects, spells/innates
and valid slots are alternatives; no earlier snorkel receipt is universally
mandatory. The belt source is already underwater, so it cannot promise initial
access. [Arrival](../../../src/world/handler.c#L1445) and the shared
[drowning event](../../../src/mob/specials.c#L113) use the current breathing
state and can progress from held breath to damage/death.

The cavern south exit88965→88968 is closed/locked/pickproof, key88914, D-reset2.
Its reverse north exit88968→88965 is an ordinary closed door, key0, D-reset1.
The reward is ITEM_TOTEM34, but actual [has_key](../../../src/cmd/actmove.c#L2856)
checks held/loose exact vnum without requiring ITEM_KEY. Thus the totem is a
valid key; this is not a broken recipe. Its break value[1] is zero. Unlocking,
opening, reciprocal state and surviving arrival are separate. Existing unlocked
state and shared pass-door rules prevent claiming one mandatory historical
route. Current delivery credit does not prove lair entry or a hydra defeat.

Tree top88960 has F50. Actual [command dispatch](../../../src/cmd/interp.c#L1898)
rolls chance falling before eligible command execution and calls
[falling_start](../../../src/world/falling.c#L113). Flight/levitation, mount,
climb-catching, scheduling admission and the downward path affect the result.
This grounded room property differs from immediate no-ground arrival falling.
Arrival, attempting a command, surviving a fall and safely recovering the egg
are distinct; none is an inferred accepted quest step. Healing room88969 uses
ROOM_HEAL, increasing particular shared recovery calculations; it does not
promise safety or uniformly doubled recovery for every resource.

## Foreign continuity and custody boundaries

Razhl88900 starts in88968, wearing snake-rattle gloves88919 and carrying
[Dream skull31320](../../../areas/obj/dream.obj) via G-reset138. Qin31310's
[Q46](../../../areas/qst/dream.qst#L46) consumes five different skulls31316–31320
for31315; his separate Q27 consumes four same-kind soul shards31311 for armor.
Neither imported skull custody nor a personal hydra kill completes either
Dream outcome. The Wharf journal explains the referral without claiming Qin's
foreign definition or adding a local completion.

Skull31320 is ITEM_TREASURE8 with zero values and NODROP/NORENT/NORESET/SECRET/
FLOAT/HUM and quest/magic flags. Its extra-description owner-death warning and
Qin narration establish lore, not an accepted ownership-curse event. No literal
skull assignment or relevant owner-death callback was found in the reviewed
active assignment/type/flag paths; the review is bounded, not a claim about
every future script. Preserve ordinary handling restrictions and qualify the
actual intended curse/lifetime behavior before designing such objectives.

Ordinary [give](../../../src/cmd/actobj.c#L6137) rejects NODROP. Native quest
special dispatch precedes ordinary commands and
[durable offering selection](../../../src/world/quest.c#L1517) directly selects
current exact loose roots. Its [ownership policy](../../../src/item/item_command_policy.c#L10)
has no NODROP or treasure-type exclusion. Therefore NODROP alone does not prove
Qin's native recipe is impossible. Ordinary handoff and committed native bundle
consumption need separate policy/parity tests, including acquisition, custody,
rent, interruption, replay and frozen recovery. No live Qin completion is claimed.

Dibbly35216's Newhaven Q177 consumes snorkel88905 for C5000, while ambient speech
requests line88904. The prior [Newhaven dossier](NEWHAVEN.md#L63) already records
that mismatch and preserves the actual buyback as an uncredited service with
optional Wharf receipt. That receipt remains owned by Wharf and the supplied
pipe requires no personal jelly route. No Newhaven repair ships here.

## Fishing execution and plan expansion

Actual [get_pole](../../../src/economy/tradeskill.c#L562) scans loose inventory
for nine recognized vnums, including88903. [do_fish](../../../src/economy/tradeskill.c#L585)
requires Fishing skill, standing, a water room, a recognized loose pole, no
existing session and no disguise. The event also checks movement, combat,
connection, posture/effects and continued pole availability. It generates one
of twelve global fish kinds293/294/295/318/319/330/332/333/334/335/355/356.
It does not consume Wharf bait, line, net or jelly, or convert the preloaded
swimming catfish/bass mobiles into an accepted catch.

**Concrete source-confirmed publication-order lead:** the catch event narrates
success, changes diminishing-XP state and grants XP before
[grant_tradeskill_item](../../../src/economy/tradeskill.c#L61); the caller at
[line727](../../../src/economy/tradeskill.c#L727) ignores the boolean result.
The helper submits ownership with generic `crafting` source kind, and on failed
submission extracts the fish and reports that no item was created. Thus existing
narration/XP cannot serve as committed catch evidence. No played failure or
shipped fishing fix is claimed. Qualify actual grant rejection/accepted callback,
actor/session interruption and reward/replay recovery, then repair publication/
reward ordering in an isolated fix if those journeys confirm the intended policy.

The [integration plan](../ZONE_STORY_INTEGRATION_PLAN.md) adds concrete acceptance
requirements for typed committed catch/source events, alternative active-effect
readiness, admitted reset supply/quantity bundles, command-time access/fall
episodes and foreign restricted custody. Existing exact-item/current-count
checks do not distinguish original pickup from another player's handoff. A
future source objective needs actor/item UID/source/session/zone provenance
and a committed accepted event, while supplied delivery recipes stay compatible.

Two bounded shared macro hygiene leads also remain pending:
[IS_UNDERWATER](../../../src/core/utils.h#L259) mixes formal `c` and outer `ch`;
[CHAR_IN_HEAL_ROOM](../../../src/core/utils.h#L689) mixes `CH` and outer `ch`.
Reviewed active callers pass `ch`, so no wrong-target call or Wharf crash is
established. Qualify independent-argument helpers before isolated hardening;
do not advertise these as repaired gameplay failures.

## Direct area-owner questions and conservative recommendations

| Question | Source evidence | Recommendation |
| --- | --- | --- |
| Should the five native requests retain their exact accepted inputs, quantities, recipients, and outputs, with supplied exact items eligible and no personal source or earlier-receipt gate? | The five QA blocks define bottle cleanup, bait, adult supplies, frog jelly, and Baltik's eight-root bundle; the accepted receipts are separate and repeatable. See [exact requests and progression](#exact-requests-and-progression). | Preserve the five exact native contracts and separate credits; keep optional readiness/history distinct from acceptance and do not add inferred kill, fishing, or route prerequisites. |
| Is the “sunk” bottle prose intentional despite bottles #88912 being SECRET and FLOAT, or should the text or flags be reconciled? | Six bottle placements are declared; the prototype is both SECRET and FLOAT despite the “sunk” narration. See [declared materials](#declared-materials-shops-and-actor-identity) and [the bottle prototype](../../../areas/obj/fishermans_wharf.obj#L180). | Keep the loaded flags and qualify actual perception/recovery; correct prose or flags only through an owner-approved, separately reviewed change. |
| Should cast net #88906 remain the intended carrying-container reward with its current capacity and negative shell weight, or is a balance/encumbrance adjustment intended? | The net is an ITEM_CONTAINER with capacity 150, weight -275, and value[4] zero; shared container code handles negative weight. See [net material boundary](#declared-materials-shops-and-actor-identity), [prototype](../../../areas/obj/fishermans_wharf.obj#L96), and [shared container handling](#declared-materials-shops-and-actor-identity). | Preserve its carrying reward and current values until capacity, nested transfer, and encumbrance behavior are qualified; do not treat ordinary shop stock as an uncredited equipment-conversion service. |
| Is pole #88903 intentionally ITEM_SPAWNER39 with zero values and HOLD/BACK wear flags, given that fishing recognizes it by exact VNUM rather than its type? | Widoc stocks the pole; adults reset it in the matching hold/back slots; the bounded review found no active pole-specific spawner dispatch. See [declared materials and shop](#declared-materials-shops-and-actor-identity), [pole prototype](../../../areas/obj/fishermans_wharf.obj#L58), and [fishing selection](#fishing-execution-and-plan-expansion). | Preserve its type, flags, and reset slots until compatibility and builder intent are confirmed; do not infer that the item name or type creates a fishing action. |
| Should the built-in fishing activity remain independent from Wharf quest materials, or should any bait, line, net, jelly, or local fish NPC be an explicit input or output? | `do_fish` uses Fishing skill and one of the recognized loose pole VNUMs, generates a global fish kind, and does not consume these Wharf materials or turn preloaded fish mobiles into catches. See [fishing execution](#fishing-execution-and-plan-expansion). | Keep the five native receipts and the generic fishing skill as separate systems unless an owner defines a new source, transaction, and reward contract. Do not promise a local catch procedure from item prose. |
| Should a catch count as published only after item issuance succeeds, with XP/narration moved behind or reconciled with the committed grant result? | The catch event narrates success and grants XP before calling `grant_tradeskill_item`; it ignores the helper's boolean, and a failed submission can extract the fish. See [the publication-order finding](#fishing-execution-and-plan-expansion), [helper](../../../src/economy/tradeskill.c#L61), and [caller](../../../src/economy/tradeskill.c#L727). | Treat the observed ordering as a source-confirmed repair lead, not proof of a played failure. Do not use narration or XP as committed catch evidence; qualify rejection/recovery and handle any ordering change in a separate focused fix. |
| Which worn breathing options and entry routes should player guidance present for the underwater caverns, given that the belt's source room is already underwater? | The snorkel and belt must be worn in their valid slots; carrying or holding them is not breathing, and arrival/drowning use current effects. See [physical access and hazards](#physical-access-effects-and-hazards), [snorkel](../../../areas/obj/fishermans_wharf.obj#L84), and [belt](../../../areas/obj/fishermans_wharf.obj#L253). | Keep valid worn equipment, spells, and innates as distinct alternatives; do not make an earlier snorkel receipt universally mandatory or claim the submerged belt supplies initial access. |
| Should the cavern route require a prior journal receipt or hydra kill, or only the authored key/totem and current shared door state? | The totem #88914 is ITEM_TOTEM and the generic key lookup accepts its exact VNUM; reverse door/key/reset states differ and travel is a separate outcome. See [physical access](#physical-access-effects-and-hazards), [totem prototype](../../../areas/obj/fishermans_wharf.obj#L204), and [key lookup](../../../src/cmd/actmove.c#L2856). | Preserve the key and door declarations; do not add a personal receipt or hydra gate. Keep delivery, unlock, open, arrival, and safe return as separate evidence. |
| How should the guarding-eagle egg and tree-top risk be presented, given that the flying eagle shares a name but is not the declared egg source? | Egg #88913 is reset from guarding eagle #88910 at the tree top; the separate flying eagle has no egg declaration, and room #88960 has fall chance 50. See [declared materials](#declared-materials-shops-and-actor-identity), [falling boundary](#physical-access-effects-and-hazards), and [resets](../../../areas/zon/fishermans_wharf.zon#L125). | Preserve the typed producer and source; do not assign the egg to the same-named flying eagle or treat reaching, attempting a command, falling, or surviving as an accepted recipe step. |
| Should paddle-boat movement remain distinct from boat occupancy, and what may be claimed from one-passenger room prose or lake-wind warnings? | Movement recognizes carried/worn ITEM_BOAT, but the review found no occupancy contract; lake-wind warnings do not establish capsize/current events. See [physical access](#physical-access-effects-and-hazards), [boat prototype](../../../areas/obj/fishermans_wharf.obj#L42), and [movement handling](../../../src/cmd/actmove.c#L966). | Describe only the verified movement capability; do not infer passenger limits, capsize, or safe transport from prose without an owner-defined and qualified rule. |
| What skull custody/lifetime behavior is intended for Dream's NODROP skull #31320, and should any Wharf-side curse or owner-death objective exist? | Qin's Dream Q46 consumes five different skulls; #31320 has NODROP and other flags, ordinary GIVE rejects it, but durable native offering selects the exact loose root. No reviewed local owner-death callback was found. See [foreign continuity and custody](#foreign-continuity-and-custody-boundaries), [Dream recipe](../../../areas/qst/dream.qst#L46), and [Dream skull prototype](../../../areas/obj/dream.obj#L293). | Preserve Dream's recipe ownership and current handling restrictions; treat curse/lifetime prose as lore until an actual controller and intended custody rule are confirmed. Do not infer that NODROP alone makes the native recipe impossible. |
| Should Newhaven keep ownership of Dibbly's snorkel buyback and its separate pipe hint, while accepting a supplied Wharf snorkel without granting local buyback credit? | Newhaven Q177 consumes snorkel #88905 for coins; its ambient speech requests line #88904. The existing Newhaven dossier records the mismatch. See [foreign continuity](#foreign-continuity-and-custody-boundaries) and [Newhaven note](NEWHAVEN.md#L63). | Keep the buyback and any correction with Newhaven; preserve optional Wharf receipt/supplied-item eligibility and do not add local currency or completion credit. |
| Should non-request scene prose remain atmosphere unless the builder defines an explicit actor, action, and committed result? | The dossier distinguishes the five QA outcomes from item descriptions, guides, boat lore, fish NPCs, skull warnings, and the professor's narrated injury/chant. See [reviewed evidence](#reviewed-evidence), [exact requests](#exact-requests-and-progression), and [foreign custody](#foreign-continuity-and-custody-boundaries). | Keep authored atmosphere and lore separate from receipts; do not create rescue, curse, reading, fishing, or ownership milestones from prose alone. |
| Should the two shared macro hygiene leads be qualified separately, given that reviewed active callers pass `ch` and no wrong-target call is established? | `IS_UNDERWATER` mixes formal `c` with outer `ch`; `CHAR_IN_HEAL_ROOM` mixes `CH` with outer `ch`, but reviewed callers pass `ch`. See [fishing execution and plan expansion](#fishing-execution-and-plan-expansion), [IS_UNDERWATER](../../../src/core/utils.h#L259), and [CHAR_IN_HEAL_ROOM](../../../src/core/utils.h#L689). | Do not advertise a demonstrated Wharf crash or change shared macros as part of this mapping; qualify independent argument paths first, then handle any confirmed cleanup separately. |
| Is “Fishermans Wharf” the canonical area title, with no alternate title or command-target alias requiring owner disambiguation? | A fresh normalized scan of all 442 current `.zon` title lines finds one exact “Fishermans Wharf” owner: `fishermans_wharf`. Dock, harbor, and port titles belong to separate indexed areas; command-target alias ownership remains pending. See the [title and history audit](../quest-story-followups/FISHERMANS_WHARF_AUDIT_REVIEW.md#title-history-and-disposition), [zone header](../../../areas/zon/fishermans_wharf.zon#L2), and [source review index](../../reference/zone-story-audits/fishermans_wharf.md). | Retain the current title and `fishermans_wharf` identity; add no alias or target mapping without owner evidence. |


## Verification and limits

Focused source regression covers all exact recipes/quantities, native aliases,
actor identities/placements, optional receipts, slots/effects, doors, tree
property, imported ownership and unchanged prior Newhaven continuity. Actual
C++ projection journeys exercise discovery/encounter visibility, exact counts,
worn versus loose supplies, read-only preparation, supplied-route independence,
five separate outcomes, foreign owner rejection, replay and cold recovery.
These are projection journeys, not live world generation or durable-offering
qualification. Full catalog, schema/all-map loader, accounting gates, daily
projection, maintained build, formatting, source preservation and document
links are checked before publication; actual command results belong in the PR.

Live admitted generation/caps, first source/handoff, mixed eight-root commit,
fishing grants/XP recovery, valid worn breathing/alternatives, underwater travel,
perception, key/open/arrival, chance fall/survival, skull custody/Qin completion
and recipient renewal remain pending. There is no DB/account/server operation,
migration, deployment or merge. Catalog: 84 maps/1625 achievement units/1459
potential dailies/2207 rows; all 2668 native definitions, fingerprint/revision
two/registry and prior 83 maps stay unchanged. Original priority queue: 63/220
complete, 157 pending; Northern Lakes and Settlements (`nlakes`) next.

# Centaur Villages: comprehensive source map

Reviewed October 4, 2026. Zone 933, `centaur_zone`; roadmap priority 67.
**Source coverage is comprehensive; live gameplay qualification remains pending.**
Active, ready accounting is mandatory for discovery, visible encounters,
journals and new achievement/daily credit. Frozen obligations remain separate.

The [schema-three journal](../../../areas/story/centaur_zone.story.json) explains
all seven native exchanges: four story outcomes and three inspection/briefing
services. Eight contacts preserve all eight addressed aliases. Sixteen optional
checks comprise eight current-material checks and eight earlier receipts. The
normal progression is heart → letter → returned horn → one half, recovered staff
→ second half, then two distinct halves → bracelet and legplates. Supplied exact
items skip personal production, briefing, kill, search and source history.
Discovery stays separate. Three support units leave zone completion; four
potential daily candidates remain, without certifying actual renewal.

## Reviewed evidence

- All eleven [native blocks](../../../areas/qst/centaur_zone.qst): seven Q/four M,
  three same-kind returns, four outcomes, and one D1 retirement. Four addressed
  response families preserve `dragon/heart`, Llewyn's `amulet`, Tamilea's
  `learned/one/banitoor/vorsileez`, and Banitoor's `amulet`. The eight aliases are
  conversation choices, without eight separate accepted achievements.
- All hundred [rooms](../../../areas/wld/centaur_zone.wld), 93300–93399:
  hundred exact prose groups, fourteen headers, nineteen non-exit metadata
  groups, seventy-one numeric exit families and 135 text/keyword pairs. Full
  descriptions, properties and memberships were reviewed. Registry range
  93116–93399 is broader than the physical rooms. Header remains
  `93399 2 0 10 20 1`, reset mode two. Two fall declarations and four real
  currents are distinct from scenic mountain, river, lake and forest prose.
- All twenty-nine [mobiles](../../../areas/mob/centaur_zone.mob), 93300–93328,
  and thirty-one [objects](../../../areas/obj/centaur_zone.obj), 93300–93330:
  full flags, types, values, extras, effects and memberships. The horn's trailing
  affect-mask words are supported by the actual object parser. Both halves use
  one kind; neither reward armor, pendant nor ring replaces a half.
- All 199 [resets](../../../areas/zon/centaur_zone.zon): M70/P71/G23/O21/F7/D6/E1,
  117 exact argument families and 125 parent-aware families. Every raw row and
  family membership/cap/destination/chance/parent was reviewed. Three guardian
  F rows retain the last M elder as leader; successive followers do not lead
  each other. G/E target the actual latest M/F; P resolves a matching container
  by kind with `get_obj_num`, rather than guaranteeing the last declared UID.
  The sixty-nine repeated fruit P rows use cap72; the horn and staff have one
  live-copy caps. References resolve; no positive exit-key requirement appears.
- No local shop file, imported reset prototype or literal local special
  assignment exists. There is actual dynamic behavior: five type-29 switches
  bind via [read_object](../../../src/world/db.c#L3166); ACT_TEACHER binds
  Llewyn and the treant elder via [read_mobile](../../../src/world/db.c#L2764).
  Their full shared [teacher](../../../src/classes/epic_skills.c#L424) provides
  class/level guidance for currently present epic stones; it is not a separate
  lesson outcome. No local smith table binding, inn flag or fixed teleport
  object was found. Clearing93373 and cave93399 have actual healing flags.
- The full ordinary foreign boundary rooms were reviewed: Surface519684 north
  ↔93300 south; Northern Wilderness12269 north→93300, whose southern return
  leads to Surface. Bounded global native producers/consumers, reset/shop/literal
  assignments, all713 teleport prototypes and custom-code references were
  reviewed for local inputs/kinds/rooms. No foreign native quest continuation,
  imported reset or fixed teleport into these rooms appeared. Shared
  `event_conjure_water` uses object93300 anywhere the innate executes, without
  proving a Centaur visit, original quest acquisition or local water objective.
- Relevant actual item-only offering/allocation, same-kind return, accepted
  reward recovery/D1 retirement, switch, hidden-name selection, container GET,
  type/flag binding, reset, falling/current/movement/arrival execution were
  reviewed. A literal-only assignment scan or display-only visibility check
  cannot establish that the source or control is inert.

## All native exchanges

| Giver / Q line | Exact input → output | Classification and source limits |
| --- | --- | --- |
| Llewyn93301 / [Q21](../../../areas/qst/centaur_zone.qst#L21) | I93310 heart → I93311 letter; D0 | Story. Naergot93300 starts at93398 with the heart. A supplied heart is valid; the recipe does not require the recipient's personal dragon kill or actually cure age |
| Llewyn93301 / [Q34](../../../areas/qst/centaur_zone.qst#L34) | I93313 half → I93313 half; D0 | Optional inspection service. Directs toward Tamilea/Learned One. Same-kind returned output does not guarantee the original UID |
| Treant93302 / [Q46](../../../areas/qst/centaur_zone.qst#L46) | I93311 letter → I93311 letter; D0 | Optional briefing service. The horn request is explained; letter kind retained for the later offering. With both inputs present, later Q62 is first in the reversed native completion list |
| Treant93302 / [Q62](../../../areas/qst/centaur_zone.qst#L62) | I93311 letter + I93312 horn → I93313 half; D0 | Story. Horn P129 goes into hunter backpack93318; both exact items are required loose and together. Neither briefing nor personal kill is encoded |
| Banitoor93309 / [Q126](../../../areas/qst/centaur_zone.qst#L126) | I93313 half → I93313 half; D0 | Optional briefing service. Describes Jer'ard and the staff request, without an accepted extra lineage/search prerequisite |
| Banitoor93309 / [Q139](../../../areas/qst/centaur_zone.qst#L139) | I93317 staff → I93313 half; D0 | Story. P119 supplies staff inside static vines container93316 at93370. No first half, earlier inspection or personal forest visit is an input |
| Hateeu93310 / [Q152](../../../areas/qst/centaur_zone.qst#L152) | Two separate I93313 roots → I93314 bracelet + I93330 legplates; D1 | Story with two rewards and receiver retirement. The same physical root cannot fill both slots. One accepted outcome does not imply two achievements, both rewards currently held or a fresh reset episode |

All four outcome offerings and all three services are item-only and use the
existing committed offering/reward path. This source-level compatibility is
not live qualification of ownership admission, partial failure, both final
outputs, publication, receiver retirement, persistence or renewal. Nothing
here introduces a mixed fee, changes exact terms or enables another service.

## Sources, controls and travel

Llewyn's M145/G146 at island93335 independently preload letter93311, cap1.
The elder's M173/G174 at93366 and Banitoor's M223/G225 at93399 independently
preload half93313 under the shared cap2. Native outcomes also create those
kinds. Such supply does not force a player to have personally earned each
producer receipt. A cap bounds reset loading; it does not prove impossibility,
availability, scarcity timing or fresh daily replenishment by itself.

Haornig M125 at arch93317 has backpack G128 and horn P129, cap1. Two gladii,
boots/cap and a wolf follower are independent gear/actors. Haornig is not a
sentinel and lacks stay-zone; his source room is not a permanent location.
Naergot M221 at93398/G222 carries exact heart93310, cap1, and is a sentinel.
His static treasure container O123 has no reviewed native contents/reward
endpoint. The heart is food and the horn a weapon: eating the heart or wearing
the horn affects current custody, rather than preserving offering readiness.

Staff93317 P119 is inside static container93316 O118 at93370. The separate
boulder93309 O117 is scene scenery. Both the container and four switch vines
carry ITEM_NOSHOW, as does rock93323. Normal PC display visibility rejects
those objects, but [get_obj_in_list_vis](../../../src/world/handler.c#L5980)
explicitly permits named NOSHOW objects, including the no-tracks branch used
by [item_switch](../../../src/specs/specs.object.c#L309).
[Container target resolution](../../../src/cmd/actobj.c#L1531) uses the same
named lookup; its preflight checks closed state and combat/corpse conditions,
without inventing a display-visibility gate. Therefore these flags alone are
not evidence of broken controls or an inaccessible staff. Actual GET/source
ownership and command selection still need a played journey. No visibility
flag or hidden-container presentation change ships.

| Control kind / reset | Configured effect | Actual boundary |
| --- | --- | --- |
| Vines93307 at93363 / O114 | PUSH270,93363 north→93365 | D16 blocks north; D20 blocks reciprocal south. World exits are ordinary, not secret; shared switch clears both blocked bits |
| Vines93308 at93365 / O115 | PUSH270,93365 south→93363 | Same reciprocal gate, operated from its other side |
| Vines93305 at93377 / O121 | PUSH270,93377 north→93375 | D28 blocks north; D24 blocks reciprocal south. Shared non-secret branch clears both |
| Vines93306 at93375 / O120 | PUSH270,93375 south→93377 | Same reciprocal gate from its other side |
| Rock93323 at93326 / O53 | PUSH270,93326 east→93399 | D8 blocks east, D12 independently resets return west open. Valid reciprocal ordinary exits; no key, secret or close requirement is declared |

The shared switch checks actor, selected object, configured verb, target and
blocked state before changing the route. A true return can mean "nothing
happens". Selection, pre/post reciprocal bits, admitted crossing, return and
reset interruption need separate evidence. The NOSHOW lookup exception is
distinct from showing an object, discovering its alias or a party learning it.

Mountain incline93305 has F38/down93304; river-side93306 has F13/down93305.
The cave route branches west93305→93397, south→93398; its granite-slab prose
does not set a blocked or locked exit. Lair93398 has no-recall/no-summon; cave
93397 and arch93317 impose real tunnel/single-file conditions. Flight,
levitation, mounts and climb admission, scheduled movement, injury/removal
and surviving return must be qualified, rather than credited from prose.

Actual current fields are C5/east at93337, C6/east at93338, C7/south at93341
and C8/south at93349. [Arrival](../../../src/world/handler.c#L1516) may sweep
the actor; [command dispatch](../../../src/cmd/interp.c#L1909) can move them
instead of executing the requested command; [upstream movement](../../../src/cmd/actmove.c#L1401)
can reject movement based on strength/current/protection. The lake's sector7
and river's sectors6/7 matter without an added underwater flag. Record the
actual final room, survival and interruption, not an assumed intended arrival.

## Actual repair and news

**Shipped native repair: [e456b3403](https://github.com/Community-Duris/Duris/commit/e456b3403).**
The separate `fix:` commit changes exactly six directional clue lines:

- Tamilea's Banitoor clue changes southwest corner → eastern edge, matching
  rock93326/east→cave93399 and its westward return.
- Grotto93310's west exit now says west rather than east.
- Footpaths93313/93319 place their neighboring forest west rather than east.
- Intersection93381's east exit now says east rather than west.
- Dead-end93393 describes its entrance from the east rather than the west.

No exits, flags, placements, terms, rewards, retirement, caps or aliases change.
[Focused regression](../../../tests/async/test_centaur_directions.py) checks
the actual destinations, reciprocal cave/intersection routes and six clues.
All six checks fail on the original data and pass on the corrected source.
This qualifies source consistency; played LOOK/ASK/traversal remains pending.

News: **Centaur Villages' quest and travel clues now point in the correct
directions, including Tamilea's route to Banitoor's cave.** Keep this actual
repair prominent in the PR/news, apart from journal additions and pending work.

## Capability gaps and balanced follow-up

| Need | Evidence and next step |
| --- | --- |
| Full parent progression with supplied entry points | Present four outcomes and three optional briefings as one legible story graph. Keep exact current materials separate from earlier receipts. Do not force personal heart/horn/staff production when supplied exact roots satisfy the actual recipe |
| Original source versus handoff and hidden-name knowledge | Capture committed source UID/generation, container/root, current custodian and acquisition reason. Qualify NOSHOW display versus allowed direct selection; derive source/learned-alias credit only from accepted events, never an invisible flag or a static room hint |
| Identical repeated kinds and same-kind returns | Allocate two distinct half roots, reject one/worn/nested/wrong-kind inputs, preserve optional route independence. Returned inspections mean same kind, not identity continuity. Qualify actual destruction/generated output and spent-versus-current custody |
| Two rewards, recipient retirement and reappearance | Track the accepted offering, each indexed reward entitlement/publication, Hateeu's actual removal and a fresh reset recipient episode. Qualify interrupted/failed/replayed output and cold recovery independently of NPC presence. D1 text or projection credit alone does not certify retirement/renewal |
| Accepted control, current, fall and surviving return | Qualify actor/item/verb selection, pre/post reciprocal state, reset/interruption, movement admission, nested current displacement and final surviving arrival. Existing journal checklists do not produce these richer episodes; extend semantic adapters only after actual execution proof |
| Story claims and wording | Age cure, widow grief/burial, ancestry and "protector" honor are narrative, without separate native effects/title/rescue outcomes. Builder may deliberately retain lore or define accepted effects later. Minor spellings and hidden-container prose deserve a separate wording pass if desired; none is silently counted as a shipped fix |

There is no proven missing local quest input, absent native reference or
inoperative switch in the reviewed closure. Fresh supply, hidden retrieval,
roaming recipient encounters, final two-root/two-output settlement and Hateeu's
reappearance remain **qualification gaps**, rather than a claim that the zone
cannot be completed. No additional native repair ships with the journal.

## Validation and remaining scope

The existing production/source fixture and actual C++ catalog/schema/file-loader
journey cover all seven contracts, support exclusion, optional supplied routes,
same-kind count2 versus one/worn/wrong items, spent materials, read-only readiness,
immutable zone ownership, replay and cold recovery. Native clue regression,
maintained build, changed/staged formatting, whitespace, catalog/source/prior-map/
local-link preservation are checked before publication. These are local/source/
projection proofs, not played first acquisition, GET/offer/reward, controls,
current/falling/death/survival, receiver retirement, persistence or fresh-day renewal.

Catalog:88 authored maps/1615 achievement units/1459 potential dailies/2207 rows.
All2668 native definitions, fingerprint/revision two/registry and previous87
maps remain unchanged. Original comprehensive queue:67/220 complete,153 pending;
Enclave of the Opal Phoenix next. No DB/account/server operation, migration,
deployment or merge is part of this checkpoint.

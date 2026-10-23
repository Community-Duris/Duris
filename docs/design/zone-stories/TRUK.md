# Truktakyagyo: settlement source map and builder review

Reviewed October 11, 2026. Active area stem truk, zone 16, title
“Truktakyagyo.” The local source set has 56 WLD rooms (#1600–#1655), 18
mobiles (#1600–#1617), 10 objects (#1600–#1609), and one ZON file. There is no
local QST file, story sidecar, or SHP file, although [areas/qst/AREA lists
truk](../../../areas/qst/AREA#L5). A compiled assignment binds the tavern
bartender to the global world_quest special; that service is not a local QST
contract or a Truktakyagyo story receipt.

The source describes a goblin settlement built around its gates, a north–south
stone avenue, low-lit homes, a tavern, guard barracks, and the rothe herds that
feed and clothe its residents. It presents a working community under Traoul's
rule, with guards and errand runners, families and children, a weaponsmaster,
drunk patrons, and rothe handlers. Its authored lore does not define a local
player campaign, native achievement, daily, or material hand-in.

## Districts and authored routes

- The reddish side passage runs from the Underdark boundary at room #1600
  north through #1607 to the closed gateway at #1608–#1609. The ZON resets
  close both gateway faces, with no key configured for those doors. From the
  settlement side, the hill at #1610 drops to the long central avenue.
- The avenue runs #1611–#1622 and branches to the west and east residential
  trails, the less-used west road, tavern, barracks, and rothe enclosure.
  Room #1622 connects north to Traoul's Lair. Its exit record stores key VNUM
  #1609 but has exit flags zero and no matching D reset; this does not establish
  a mechanically closed or locked gate.
- The Dark Cave at #1655 connects south to Goblin Outpost #70198. Both this
  crossing and the Underdark and Traoul boundaries have reciprocal active
  WLD records. The complete static graph has 115/115 resolving exits, one
  56-room local weak component, and no fall or current fields. The
  [repository-wide connectivity review](../quest-story-followups/WORLD_CONNECTIVITY.md#checkpoint-403-truktakyagyo-boundaries)
  records those directed boundaries.
- West and east residential roads end among six one-exit hovels. The south
  spur contains the Dark Tavern at #1646. The Rothe Enclosure spans #1647–#1651
  and has two further gates into pens #1652 and #1653; their matching ZON
  resets close the gate faces.

The active exits identify route structure, not present actor access or safety.
The realm, door, key, and reset declarations do not prove live stock, intended
gate behavior, an admitted crossing, or a quest outcome.

## Population, objects, and resets

The 18 mobile prototypes form the settlement's stated population: outer and
gate guards, a sentry, bartender, drunk and patron, goblin women and children,
male/female/young rothe, a rothe tender, an errand runner, city guards, an elite
guard, a patrol, and the weaponsmaster. The weaponsmaster's training language
has no local special assignment or QST endpoint in the reviewed source.

The ten local objects are a spear, great axe, rothe-hide boots, dirty eyepatch,
stone mug, short sword, rothe dung, putrid fungi, vicious scimitar, and bright
red key. The key is equipped by the massive gate guard at #1622 in the ZON
source; its reset is not a guarantee of live custody or proof that the #1622
exit is mechanically gated. Dung and fungi are reset as room objects around
the pens, not defined as native quest inputs.
[Mobile prototypes](../../../areas/mob/truk.mob) ·
[object prototypes](../../../areas/obj/truk.obj) ·
[zone resets](../../../areas/zon/truk.zon#L1)

The zone header is the authored line “1655 2 0 35 45 1.” The ZON contains 99
reset commands: 68 M, 11 O, nine E, eight D, and three G. All local reset
prototypes and rooms resolve. It has no P, F, or local quest-delivery command.
These are declarations and caps, not proof of current reset stock.

## Global world-quest service

The bartender mobile #1603 is reset in the Dark Tavern at #1646 and receives
the compiled world_quest binding in
[specs.assign.c](../../../src/specs/specs.assign.c#L775). Its special uses
ASK for global “quest,” “map,” “abandon,” or “resign” actions; the implementation
is in [specs.world_quest.c](../../../src/specs/specs.world_quest.c#L191).

The code requires level 11 to request a new world quest, applies a configured
per-level fee with difficulty scaling, and submits the charge through the
currency transaction service. It can sell a map for an active global quest
and charges a time-scaled amount to abandon; a FIND_AND_KILL quest asks for
confirmation. A completed quest pays through its recorded giver, and this
bartender only recognizes completion when its VNUM matches that giver. These
terms belong to the shared world-quest system, whose objectives and target
zones are not local Truktakyagyo story stages. No gameplay, payment, quest
creation, abandonment, or reward was exercised in this source audit.

The local QST source is absent despite its QST AREA entry. Keep that inventory
mismatch visible; do not infer a local contract from the global bartender
binding or from goblin dialogue.

## Explicit builder questions and recommended resolutions

These are source-backed decisions. They do not implement local quest content,
repair doors, change global services, or claim live travel outcomes.

| # | Builder question | Recommended resolution from current source |
| --- | --- | --- |
| 1 | Should Truktakyagyo remain an authored settlement without a local story contract? [QST index](../../../areas/qst/AREA#L5) | Do not infer an achievement or daily from atmosphere. Add local objectives only after an owner defines their intended recipients, accepted outcomes, and persistence. |
| 2 | Is the QST AREA entry for truk intentional when areas/qst/truk.qst is absent? [index row](../../../areas/qst/AREA#L5) | Resolve the stale-looking index/file mismatch with the content owner. Do not create an empty file or delete the index entry as an audit-side repair. |
| 3 | Should bartender #1603's global world_quest binding remain distinct from any future local story? [assignment](../../../src/specs/specs.assign.c#L775) · [bartender](../../../areas/mob/truk.mob#L43) | Preserve the global special as a separate service; do not count its dynamic objectives as local zone achievements. |
| 4 | Should global quest requests retain the code-defined level-11 gate and configured, difficulty-scaled fee? [level check](../../../src/specs/specs.world_quest.c#L357) · [fee](../../../src/specs/specs.world_quest.c#L405) | Describe the shared policy as configured global behavior. Do not turn its fee or level threshold into a native Truk quest term. |
| 5 | Should world-quest objectives and target zones remain separate from Truktakyagyo exploration? [global quest implementation](../../../src/world/world_quest.c#L1139) | Keep the global contract and its zone selection separate from any future local narrative or visit objective. |
| 6 | Should the bartender award a completed global quest only when its VNUM matches the recorded giver? [giver check](../../../src/specs/specs.world_quest.c#L379) | Preserve the recorded-giver boundary; do not describe any bartender as interchangeable for reward settlement. |
| 7 | Should abandon/resign remain a paid global action with extra confirmation for FIND_AND_KILL? [abandon flow](../../../src/specs/specs.world_quest.c#L227) | Keep the configured elapsed-time cost and confirmation rule inside the existing world-quest service; do not count a request as a committed abandonment. |
| 8 | Should buying a map remain a separate payment for an already-active world quest? [map flow](../../../src/specs/specs.world_quest.c#L297) | Keep map purchase distinct from quest acceptance and local zone discovery; the code prices it from player level. |
| 9 | Should payment or quest-state credit be reported only after the currency transaction settles? [transaction submit](../../../src/specs/specs.world_quest.c#L425) | Preserve the asynchronous transaction boundary. An ASK response or displayed balance is not a committed payment receipt. |
| 10 | Should the outer settlement gateways reset closed but remain keyless? [gateway rooms](../../../areas/wld/truk.wld#L149) · [north reset](../../../areas/zon/truk.zon#L8) · [south reset](../../../areas/zon/truk.zon#L12) | Keep the two faces' closed reset state explicit. Confirm the intended opening and return behavior before describing a usable route. |
| 11 | Is the Traoul's Lair gate at room #1622 meant to use key #1609 despite exit flags zero? [exit record](../../../areas/wld/truk.wld#L435) | Treat the key field and gate prose as separate source facts. Do not claim a locked door unless an owner confirms and a focused behavior change defines it. |
| 12 | Should the guard-carried key be presented as guaranteed access to Traoul's Lair? [key reset](../../../areas/zon/truk.zon#L84) · [key object](../../../areas/obj/truk.obj#L85) | No. Preserve the reset cap and source actor; qualify current custody and the exit's actual door state independently. |
| 13 | Should the north crossing at #1622 count as entry into Traoul's Lair rather than a Truk completion event? [cross-area register](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L25) | Keep it as an active reciprocal area boundary. The WLD edge does not define a native quest receipt. |
| 14 | Should the side passage’s east exit remain the reciprocal Underworld boundary, and is the source-only north approach from ud_map2 into Truk room #1600 intended? [Truk exit](../../../areas/wld/truk.wld#L1) · [cross-area register](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L24) · [candidate approach](../../../areas/wld/ud_map2.wld#L80799) · [ud_map2 audit](../quest-story-followups/UD_MAP2_AUDIT_REVIEW.md#authored-wld-graph-and-boundaries) | Preserve the active reciprocal boundary and keep the candidate approach with ud_map2; it is source-only, not active ingress. Qualify route behavior separately. |
| 15 | Should room #1655's south exit remain a separate reciprocal route to Goblin Outpost? [Truk room](../../../areas/wld/truk.wld#L1032) · [cross-area register](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L26) | Retain the explicit boundary without inventing an outpost quest, reward, or safety claim. |
| 16 | Should the Rothe enclosure and pen gates reset closed without keys? [gate resets](../../../areas/zon/truk.zon#L16) · [Rothe rooms](../../../areas/wld/truk.wld#L884) | Preserve the declared closed, keyless door resets. Qualify opening and return behavior before adding route objectives. |
| 17 | Should rothe dung and putrid fungi remain scenery/material objects unless an owner defines a use? [objects](../../../areas/obj/truk.obj#L57) · [resets](../../../areas/zon/truk.zon#L41) | Do not claim a harvest, crafting recipe, sale, or local quest input from the floor resets alone. |
| 18 | Should the bartender be described as a global quest service rather than a local tavern shop? [special assignment](../../../src/specs/specs.assign.c#L775) | Keep the absence of a local SHP file and the compiled world-quest role distinct. Do not imply food, drink, or ordinary shop transactions are implemented here. |
| 19 | Should warm-cave and animal prose imply falls, currents, or authored survival hazards? [WLD and hazard census](../quest-story-followups/TRUK_AUDIT_REVIEW.md#active-graph-and-boundaries) | No fall or current fields are authored in this zone. Keep unimplemented hazards and survival rewards out of the source map. |
| 20 | How should the unregistered brass-old-1 reset for object #1604 be treated? [reset reference](../../../areas/zon/brass-old-1.zon#L545) · [Truk object](../../../areas/obj/truk.obj#L39) · [dormant object file](../../../areas/obj/brass-old-1.obj) | Record it as a dormant source-only reference: brass-old-1 is not an active AREA contributor and its object file does not define #1604. Do not call it an active reset supplier. |

## Validation and publication boundary

Static review covers all 56 local rooms, 18 mobiles, 10 objects, 99 reset
commands, the complete active exit register for this zone, the compiled
world_quest assignment and implementation, the QST index/file mismatch, and
bounded cross-zone item, shop, duplicate-prototype, portal, and inactive-reset
references. No gameplay test, build, migration, database action, or source
repair was performed for this documentation checkpoint.

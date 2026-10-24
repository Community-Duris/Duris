# The Charcoal Palace repository-wide audit supplement

This checkpoint reconciles the [source-comprehensive Charcoal Palace dossier](../zone-stories/CHARCOAL_PALACE.md) with active-area graph, inactive-WLD, typed dependency, title, bounded history, and attic-source passes at source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`. It records repository dependencies and builder decisions. It does not change native gameplay or area placement.

## Active connectivity and map admission

The active `firep` WLD has 151 rooms and 412 authored exits, with no parse errors. All 412 resolve: 411 stay local and one leaves the zone. The graph has two weak components, one of 150 rooms and one isolated room. From Plane of Fire room #25415, the active west exit enters #88321. Local room #88301 also exits west to #25415; it is a different boundary endpoint, so these are separate one-way inbound/outbound records and do not form a reciprocal WLD pair ([Plane of Fire](../../../areas/wld/plane_fire_one.wld#L586), [local outbound](../../../areas/wld/firep.wld#L47)).

A plain authored traversal from #88321 reaches 25/151 rooms when the two `EX_BLOCKED` exits are unavailable and 149/151 when those controls are treated as available. The unrestricted graph has authored returns to #88321 from 150/151 rooms; isolated fissure #88448 has no local edge, and kitchen #88399 has no incoming local edge. The dossier maps the conditional switch/key stages and hazards. These source graph counts do not establish successful access, current state, travel safety, or a played route.

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without parse issues. It found no source-room VNUM overlap and no inactive-WLD exit into `firep`. The curated source-only approach total remains 1,533; the Surf subset remains 142. This pass adds no source-only WLD approach or attic route.

## Typed dependencies and scoped negative checks

| Dependency | Source-backed result and boundary |
| --- | --- |
| Active WLD boundaries | Plane of Fire #25415 west enters #88321; local #88301 west exits to #25415. The endpoints differ, and neither edge has a reverse counterpart ([Plane of Fire](../../../areas/wld/plane_fire_one.wld#L586), [Charcoal Palace](../../../areas/wld/firep.wld#L47)). |
| Outside exact item use | Active Airp QST consumes Charcoal Palace cloak #88304 in a nine-item return for Cloudseeker #131650. Its sidecar records the same exact item in the carried-item and accepted-receipt steps; supplied ownership is accepted without claiming a local source history ([QST](../../../areas/qst/airp.qst#L115), [Airp story item](../../../areas/story/airp.story.json#L546), [receipt](../../../areas/story/airp.story.json#L618)). |
| Unregistered Brass reset | Unregistered `brass-old-1.zon` line 812 has a `G` reset record for local Firep scales #88322 at 10 percent. It is outside the active contributor list and is not active stock ([source ZON](../../../areas/zon/brass-old-1.zon#L812), [candidate record](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17)). |
| Imported local reset objects | `firep.zon` imports outside objects #61 and #92121 as vault stock and #359 on Kossuth. The #61 VNUM appears in four current object files (Dalvik, Heavens, Hostel, and Mavulsk), so its source owner is ambiguous; #92121 is in Undermountain and #359 in Heavens ([vault objects](../../../areas/zon/firep.zon#L104), [Kossuth reset](../../../areas/zon/firep.zon#L352), [Dalvik #61](../../../areas/obj/dalvik.obj#L777), [Heavens #61](../../../areas/obj/heavens.obj#L868), [Hostel #61](../../../areas/obj/hostel.obj#L795), [Mavulsk #61](../../../areas/obj/mavulsk.obj#L997), [Undermountain #92121](../../../areas/obj/undermountain.obj#L1258), [Heavens #359](../../../areas/obj/heavens.obj#L3725)). |
| Local QST output outside the zone | The Q33 native recipe awards local gauntlets #88320 and imported broken rays of morning Sunrise #55270. The local story-sidecar receipt repeats #55270. This is a Firep-owned reward reference to Winterhaven's object source, not an external consumer of a Firep object ([QST rewards](../../../areas/qst/firep.qst#L50), [sidecar receipt](../../../areas/story/firep.story.json#L105), [Winterhaven object](../../../areas/obj/wh.obj#L3360)). |
| CHAOS starter kits | Current standard/enhanceable profile arrays reference local equipment #88301 (crown), #88305 (bone ring), #88315 (Kossuth totem), #88317 (liquid-rock ring), and #88320 (gauntlets). The kit loader selects a profile and loads each eligible item with `read_object`; actual profile selection and kit publication were not exercised ([standard profiles](../../../src/account/chaos_eq_data.h#L71), [shaman item](../../../src/account/chaos_eq_data.h#L145), [summoner item](../../../src/account/chaos_eq_data.h#L307), [enhanceable warrior](../../../src/account/chaos_eq_data.h#L324), [enhanceable shaman](../../../src/account/chaos_eq_data.h#L389), [enhanceable summoner](../../../src/account/chaos_eq_data.h#L550), [kit loader](../../../src/account/nanny.c#L634), [profile selection](../../../src/account/nanny.c#L706)). |
| Rehomed artifact and binding | Commit `0673b6654` (2025-12-19) describes moving the Sword of Fire and Flame from an old Brass object file into Charcoal Palace. It adds object #88327 to `firep.obj`, changes Kossuth's reset from #25030 to #88327, and changes the enabled Zion binding to #88327. This is item-level lineage, not evidence that Firep replaces the whole Brass source ([object](../../../areas/obj/firep.obj#L305), [reset](../../../areas/zon/firep.zon#L351), [binding](../../../src/specs/specs.assign.c#L2171)). |
| Dormant numeric match | `randomeq.c` lists Kossuth #88316 in a high-drop table, but the selected consuming loop is commented out in the current source. Treat this as dormant configuration, not active stock behavior ([list entry](../../../src/item/randomeq.c#L135)). |
| Compiled Firep mobile bindings and callbacks | `specs.assign.c` binds Kossuth #88316, Fruaack #88319, eleven guards to `charcoal_guard`, and #88329 to `block_dir`. `specs.firep.c` counts and summons helper #88323 and lists #88300, #88301, #88304, and #88326 as Fruaack helpers ([bindings](../../../src/specs/specs.assign.c#L2229), [last binding](../../../src/specs/specs.assign.c#L2242), [Kossuth helper](../../../src/specs/specs.firep.c#L86), [Fruaack helper list](../../../src/specs/specs.firep.c#L115)). |
| Local item switches | Firep has no type-25 teleport object. Type-29 object #88310 targets local room #88324, and #88318 targets local room #88339 through `value[1]`; no external type-25 or type-29 destination targets a Firep room ([wardstone data](../../../areas/obj/firep.obj#L122), [chain data](../../../areas/obj/firep.obj#L219), [switch value roles](../../../src/specs/specs.object.c#L302), [item type constants](../../../src/core/defines.h#L140)). |
| Shared soul-taking item check | The combat kill path calls `soul_taking_check`, which tests whether the wielded object is Firep stiletto #88314 before applying its soul-taking effect ([object](../../../areas/obj/firep.obj#L165), [check](../../../src/specs/specs.object.c#L1742), [VNUM constant](../../../src/core/utils.h#L186), [combat caller](../../../src/combat/fight.c#L2080)). |
| Bounded negatives | No same-type outside mobile/object definition matching a Firep prototype, no outside SHP product, and no local shop were found. No active reset or QST giver header points into Firep. Airp is the only active outside exact QST/typed-sidecar item use of local objects in this scan; unregistered `brass-old-1` is the only source-only typed reset consumer. No type-25 or type-29 destination targets a local Firep room. Raw `88396`/`88344` matches in Alatorin/White Hills story IDs are strings, not typed `item_vnums` dependencies. |

The existing [source-comprehensive dossier](../zone-stories/CHARCOAL_PALACE.md) remains the authority for the two-material accepted recipe, hidden stock, spell/guard effects, actual switch and key controls, Fireplane conditions, epic/pool services, and competing foreign returns. Its 20 builder follow-ups remain open. This supplement does not claim successful stock recovery, a played Q33 exchange, a starter-kit grant, an epic/pool service outcome, or any foreign return.

## Title, history, and attic-source disposition

After stripping color tokens and normalizing whitespace/case, “The Charcoal Palace” has one exact owner among 351 active `.zon` titles: `firep` ([title source](../../../areas/zon/firep.zon#L2)). Followed Firep WLD/ZON/MOB/OBJ/QST histories remain under the same `firep` stem after the 2009 `areas/` relocation; no alternate-stem rename or explicit whole-zone predecessor/replacement claim appears in those bounded paths. The story sidecar was added on 2026-10-06.

The separate inactive candidate `brass-old-1` has relevant item-level ties: its current ZON contains the source-only #88322 reset above, and commit `0673b6654` records one Sword of Fire and Flame transfer into Firep. Neither fact establishes that a complete Brass source was replaced by the active Charcoal Palace. Keep active `firep` **DO NOT MOVE** and keep `brass-old-1` **RETAIN IN PLACE** pending its own source, map, shop, tooling, and typed-reference closure.

## Builder questions and recommendations

1. Confirm the Plane of Fire arrival at #88321 and the separate return from #88301. Preserve both current records and describe their asymmetric endpoint without implying reciprocal topology.
2. Preserve the 150-room main weak component and isolated #88448 in any future map proposal. Do not convert the kitchen's one-way authored reach or gated route stages into claims of public access or safety without builder intent and live qualification.
3. Confirm the Airp cloak consumer and the local CHAOS starter-kit references. Preserve the exact object identities, active item dispatch, reset stock, and admission conditions if a prototype or source record changes.
4. Preserve `brass-old-1.zon`'s typed reference to #88322 and the item-level #88327 move in its own candidate record. Keep source-only resets distinct from current active supply and do not infer whole-zone replacement from a single artifact transfer.
5. Retain the dossier's 20 Firep-specific builder follow-ups. The repository-wide audit adds no native repair, no zone move, and no candidate clearance.

## Checkpoint 798: current typed census refresh

The active reset file contains 260 typed operands: 172 M, 35 F, 9 O, 10 G,
32 E, and two object operands in P slots. Of these, 257 resolve to local
prototypes. The three external object references are #61 and #92121 in the
vault, and #359 on Kossuth; each has a current object definition and no reset
operand is unresolved. The current source tree defines object #61 in Dalvik,
Heavens, Hostel, and Mavulsk, so this VNUM does not have a unique source owner.
The #92121 prototype belongs to Undermountain, and #359 belongs to Heavens.
[First vault import](../../../areas/zon/firep.zon#L104),
[second vault import](../../../areas/zon/firep.zon#L105),
[Kossuth reset](../../../areas/zon/firep.zon#L352),
[Undermountain object](../../../areas/obj/undermountain.obj#L1258),
[Heavens object](../../../areas/obj/heavens.obj#L3725).

The local QST has one giver and four typed item fields: G inputs #88322 and
#88323, plus R outputs #88320 and Winterhaven object #55270. Its structured
sidecar repeats the exact #55270 receipt; its other typed mobile and object
references resolve within Firep. The only active outside QST and story-sidecar
consumer is Airp's nine-item Cloudseeker exchange, which consumes local cloak
#88304. No active external ZON reset targets a Firep mobile or object. The only
source-only reset consumer found is Brass-old-1's G reset of #88322. There is
no local shop, external SHP product, external QST giver header, other outside
typed sidecar consumer, or same-type duplicate definition for a Firep mobile or
object prototype.
[QST fields](../../../areas/qst/firep.qst#L50),
[local sidecar contacts](../../../areas/story/firep.story.json#L23),
[Firep receipt](../../../areas/story/firep.story.json#L105),
[local sidecar item mapping](../../../areas/story/firep.story.json#L124),
[Airp QST input](../../../areas/qst/airp.qst#L115),
[Airp carried-item mapping](../../../areas/story/airp.story.json#L546),
[Brass source-only reset](../../../areas/zon/brass-old-1.zon#L812).

Firep has no type-25 item. Its two type-29 switches use local destinations:
wardstone #88310 targets room #88324, and chain #88318 targets room #88339.
Across 872 type-25 and 380 type-29 declarations in the current active and
source-only object files, none targets a Firep room. These item destinations
do not change the WLD census or the separate one-way Plane of Fire boundary.
[Wardstone destination](../../../areas/obj/firep.obj#L122),
[chain destination](../../../areas/obj/firep.obj#L219),
[switch value roles](../../../src/specs/specs.object.c#L302).

The compiled CHAOS arrays reference Firep objects #88301, #88305, #88315,
#88317, and #88320. Compiled special bindings cover Firep mobile handlers and
object #88327; local callbacks also read or spawn the listed helper mobiles.
The generic combat soul-taking check recognizes stiletto #88314. Kossuth
#88316's randomeq entry remains in dormant configuration with its consuming
loop disabled. No gameplay, area source, WLD total, attic disposition, or owner
question changed. Runtime, editor/tooling, persisted-player, and indirect or
dynamic closure remain unqualified.
[CHAOS profiles](../../../src/account/chaos_eq_data.h#L71),
[mobile bindings](../../../src/specs/specs.assign.c#L2229),
[object binding](../../../src/specs/specs.assign.c#L2171),
[soul-taking check](../../../src/specs/specs.object.c#L1750),
[dormant Kossuth entry](../../../src/item/randomeq.c#L135).

# ud_map0 audit review

Canonical key: component:ud_map0. Coverage data row 462 / CSV line 463;
candidate dependency row 111 / CSV line 112. Reviewed October 12, 2026
(Checkpoint 549) against checkout c500ac51b8cd26540943257e7ca4e9807263ef74.
This is a static source, graph, typed-reference, identity, and followed-history
review. It does not establish that the map is loaded or that any route or reset
executes at runtime.

## Source inventory and narrative review

The same-stem component consists of [ud_map0.wld](../../../areas/wld/ud_map0.wld#L1),
empty [ud_map0.mob](../../../areas/mob/ud_map0.mob), empty
[ud_map0.obj](../../../areas/obj/ud_map0.obj), and
[ud_map0.zon](../../../areas/zon/ud_map0.zon#L1). The ZON header is #10000,
titled “deep below the surface,” spans through #1109809, and has reset mode 0.
The stem is absent from [areas/AREA](../../../areas/AREA) and
[areas/qst/AREA](../../../areas/qst/AREA); no same-stem QST, SHP, map JSON, or
story sidecar exists.

The WLD defines 2,708 rooms from #1100189 through #1109809. All room
descriptions are empty. The 19 title groups label labyrinth passages, huge
stalactite caverns, fungus-covered caverns, rocky shorelines, streams and
lakes, dwarven mines, and ice tunnels/cave-ins. These room names and terrain
labels provide setting cues, but the checked-in map source contains no
descriptive prose story.

## Authored WLD graph

The WLD has 10,837 authored exits: 7,767 target another ud_map0 room, six
target active WLD sources, and 3,064 positive targets are absent from every
current WLD source. There are no negative targets. The 7,767 local edges form
one weak component containing all 2,708 rooms. In the authored directed graph,
a raw authored-edge traversal starting at either room #1104328 or #1106354
reaches all 2,708 rooms. This ignores doors, handlers, and runtime state and
describes only source topology: ud_map0 is excluded from the active loader, so
the two Shafts targets below do not establish a live entry or a traversable
runtime route.

The six source-only exits into active WLDs are:

| ud_map0 source exit | Current target | Evidence |
| --- | --- | --- |
| #1102287 down to Myconid #2300 | Myconid | [ud_map0](../../../areas/wld/ud_map0.wld#L7246) · [Myconid](../../../areas/wld/myconid.wld#L1) |
| #1104328 down to Shafts #10011 | Shafts | [ud_map0](../../../areas/wld/ud_map0.wld#L22517) · [Shafts](../../../areas/wld/shafts.wld#L190) |
| #1104885 up to Ghore #11548 | Ghore | [ud_map0](../../../areas/wld/ud_map0.wld#L29619) · [Ghore](../../../areas/wld/ghore.wld#L1061) |
| #1105558 north to Underworld #4469 | Underworld | [ud_map0](../../../areas/wld/ud_map0.wld#L37414) · [Underworld](../../../areas/wld/underworld.wld#L2167) |
| #1106354 up to Shafts #10020 | Shafts | [ud_map0](../../../areas/wld/ud_map0.wld#L41698) · [Shafts](../../../areas/wld/shafts.wld#L341) |
| #1106895 down to Shafts #10003 | Shafts | [ud_map0](../../../areas/wld/ud_map0.wld#L43235) · [Shafts](../../../areas/wld/shafts.wld#L56) |

All six are inactive-map approaches, not active outbound routes. Five had
already been recorded in prior candidate reviews; the #1104885 up edge to
Ghore #11548 is the newly reconciled non-overlapping source-only approach.
This adds one tuple to the curated inactive-WLD total: 11,988 overall, of
which 415 originate in Surf. There are no falls or currents, and every source
room and exit is within the ZON numeric range. No ud_map0 room VNUM overlaps
another current WLD source.

Two active Shafts exits target this candidate:

| Active source exit | Candidate room | Evidence |
| --- | --- | --- |
| Shafts #10011 up to #1104328 | #1104328 | [unresolved-exit register](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L18) |
| Shafts #10020 south to #1106354 | #1106354 | [unresolved-exit register](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L20) |

These are unresolved active-source references because ud_map0 is not loaded.
The first target pair also has the exact reverse-direction ud_map0 exit:
#1104328 down to Shafts #10011. The second pair uses different directions in
the two files: ud_map0 #1106354 exits up to #10020, while Shafts #10020 exits
south to #1106354. No direction was changed or inferred from the apparent
room pairing.

## Typed reset consumers and bounded reference closure

The 26 M commands in [ud_map0.zon](../../../areas/zon/ud_map0.zon#L10)
request mobile prototypes in candidate rooms. Their source-only status follows
from the candidate’s exclusion from areas/AREA. Twenty-three commands name
prototypes defined in active-area MOB files; they do not establish current
placements or live stock.

| Mobile prototype and owner | Source-only ud_map0 placements | Evidence |
| --- | --- | --- |
| Underworld #4480 | one at #1103458 | [reset](../../../areas/zon/ud_map0.zon#L15) · [prototype](../../../areas/mob/underworld.mob#L193) |
| Underworld #4510 | #1104554, #1104649, #1105542 | [reset range](../../../areas/zon/ud_map0.zon#L19) · [prototype](../../../areas/mob/underworld.mob#L234) |
| Underworld #4511 | #1103793 twice, #1105316 twice, #1109709 twice | [reset range](../../../areas/zon/ud_map0.zon#L16) · [prototype](../../../areas/mob/underworld.mob#L247) |
| Underworld #4520 | #1102790, #1103449, #1104760 | [reset range](../../../areas/zon/ud_map0.zon#L12) · [prototype](../../../areas/mob/underworld.mob#L260) |
| Wander #36601 | #1105256 | [reset](../../../areas/zon/ud_map0.zon#L29) · [prototype](../../../areas/mob/wander.mob#L15) |
| Wander #36605 | #1102380, #1105256 | [reset](../../../areas/zon/ud_map0.zon#L10) · [prototype](../../../areas/mob/wander.mob#L71) |
| Wander #36626 | #1104623, #1105159, #1105240 | [reset](../../../areas/zon/ud_map0.zon#L20) · [prototype](../../../areas/mob/wander.mob#L370) |
| Llzazan #36225 | #1104813, #1105038 | [reset](../../../areas/zon/ud_map0.zon#L25) · [prototype](../../../areas/mob/llzazan.mob#L367) |
| Arachdrathos #36431 | #1104644 | [reset](../../../areas/zon/ud_map0.zon#L21) · [prototype](../../../areas/mob/arac-web.mob#L465) |
| Moonshae #26225 | #1102588 | [reset](../../../areas/zon/ud_map0.zon#L11) · [mobile](../../../areas/mob/moonshae.mob#L272) |

The M commands for #1518 at #1103993, #1519 at #1102893, and #1520 at
#1104692 ([#1518 reset](../../../areas/zon/ud_map0.zon#L18),
[#1519 reset](../../../areas/zon/ud_map0.zon#L13),
[#1520 reset](../../../areas/zon/ud_map0.zon#L23)) have no matching current MOB prototype.
The same numbers appear in specs.assign.c as conjurer, necromancer, and
sorcerer class-slot entries ([#1518](../../../src/specs/specs.assign.c#L427),
[#1519](../../../src/specs/specs.assign.c#L428),
[#1520](../../../src/specs/specs.assign.c#L423)); those code references do not
provide missing mobile definitions or prove that these resets are usable.

The #26225 dependency is a mobile reset, not an object reset. The M command
names active Moonshae mobile #26225 (“robyn druid”); Moonshae also has a local
OBJ definition numbered #26225, a separate cross-type numeric collision.
See the [Moonshae audit correction](MOONSHAE_ISLAND_AUDIT_REVIEW.md#typed-inbound-and-outbound-dependencies).

A checked-in source scan found no other WLD source targeting ud_map0, no
external ZON room-target command into it, no QST/SHP/map-story integer room
reference, no type-25 OBJ destination to its rooms, and no matching room VNUM
literal in maintained src code. The two active Shafts WLD references and the
26 candidate ZON M commands above are the bounded current source references
identified. Editor projects, generated maps, deployment configuration,
runtime loader state, and persisted room identities were not inspected.

## Title, history, and disposition

After color and whitespace normalization, “deep below the surface” is unique
among the 442 current ZON titles. The sibling titles “the dreadful underdark”
(ud_map1) and “the caverns of the underdark” (ud_map2) are also distinct. The
three map room-number sets are pairwise disjoint, and ud_map0 room IDs have no
overlap with any other current WLD source. Followed same-stem WLD/ZON history
reaches the 2009 area-tree move and later 2013 edits; no explicit predecessor,
successor, or replacement decision was found. Similar underdark naming and
shared source-only reset consumers do not establish a replacement relationship.

Keep ud_map0 **RETAIN IN PLACE** outside both manifests. Preserve its six
source-only outbound exits, two active unresolved Shafts references, 26 M
commands, empty companion files, and sparse source identities. Do not activate,
merge, regenerate, move, or archive it until the map/editor owner confirms the
authoritative project, map/export role, reset identities, route directions,
and runtime and persisted ownership. See the
[20 builder questions](UD_MAP0_BUILDER_QUESTIONS.md). No build, boot,
deployment, editor project, or database state was inspected.

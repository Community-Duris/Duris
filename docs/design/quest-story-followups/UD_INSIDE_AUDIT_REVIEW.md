# ud-inside repository-wide audit supplement

Canonical key: `component:ud-inside`. Coverage data row 461 / CSV line 462;
candidate dependency data row 110 / CSV line 111. Reviewed October 12, 2026
(Checkpoint 548) against checkout `c500ac51b8cd26540943257e7ca4e9807263ef74`.
This is a static source, graph, typed-reference, identity, and followed-history
review. It does not establish runtime loading, editor ownership, or persisted
room identity.

## Source inventory and narrative review

The same-stem files are [ud-inside.wld](../../../areas/wld/ud-inside.wld#L1),
[ud-inside.mob](../../../areas/mob/ud-inside.mob),
[ud-inside.obj](../../../areas/obj/ud-inside.obj), and
[ud-inside.zon](../../../areas/zon/ud-inside.zon#L1). The MOB and OBJ files are
empty. There is no same-stem QST, SHP, map JSON, or story sidecar, and the stem
is absent from both [areas/AREA](../../../areas/AREA) and
[areas/qst/AREA](../../../areas/qst/AREA).

The WLD defines one room, #1, titled “Unnamed.” Its description is empty and
it has no exits. The ZON is also numbered #1, titled “The Horrible Tracts of
the Unnamed,” with last VNUM #1 and reset-mode field 2. It contains no reset
or door commands. Its `durisEdit v3.00` comment is provenance evidence only;
the editor project and meaning of the ZON fields have not been recovered.
There is no room prose, local mobile/object prototype, quest, or story
sidecar from which to establish a narrative or gameplay purpose.

## Authored graph and numeric owner

The `ud-inside` WLD graph consists of one singleton room and zero directed
exits. Its local weak-component count is one, with one room in the largest
component. There is no candidate-authored WLD outbound edge.

Room #1 is a shared source number. Twelve current WLD files define that number,
but active `limbo.wld` is the only one selected by `areas/AREA`; the code also
reserves it as [`ROOM_LIMBO_VNUM`](../../../src/core/utils.h#L637). The complete
`ud-inside` #1 block is byte-identical to the placeholder blocks in
[`astral.wld`](../../../areas/wld/astral.wld#L1) and
[`mtncastl.wld`](../../../areas/wld/mtncastl.wld#L1). Active Limbo has a
different room record. These identical placeholders do not establish a shared
owner, alias, or replacement.

Nine active WLD directions from Desert room #49790 (“jussin load room”) target
numeric room #1: north, east, west, up, down, northwest, southwest, northeast,
and southeast. Their existing cross-area rows resolve the target to active
Limbo, not `ud-inside`; the pair has no reverse direction. Accordingly,
candidate-specific active ingress remains zero, as recorded in the
[cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L630) and
[pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L113). The raw number match is
not an approach into this candidate.

Fourteen further WLD directions to #1 occur in eight unregistered sources.
Under the current active manifest, these source-only declarations also
resolve numerically to Limbo; they are not active routes or candidate
entrances.

| Unregistered WLD source | Directions to #1 |
| --- | ---: |
| [dalvik.wld#L34](../../../areas/wld/dalvik.wld#L34) | 1 |
| [hostel.wld#L37](../../../areas/wld/hostel.wld#L37), [#L931](../../../areas/wld/hostel.wld#L931), [#L1754](../../../areas/wld/hostel.wld#L1754) | 3 |
| [Magetower.wld#L18](../../../areas/wld/Magetower.wld#L18), [#L860](../../../areas/wld/Magetower.wld#L860) | 2 |
| [mavulsk.wld#L44](../../../areas/wld/mavulsk.wld#L44), [#L67](../../../areas/wld/mavulsk.wld#L67) | 2 |
| [Mentiri.wld#L18](../../../areas/wld/Mentiri.wld#L18) | 1 |
| [Nexus_1.wld#L14](../../../areas/wld/Nexus_1.wld#L14) | 1 |
| [theurgist.wld#L22](../../../areas/wld/theurgist.wld#L22), [#L31](../../../areas/wld/theurgist.wld#L31), [#L40](../../../areas/wld/theurgist.wld#L40) | 3 |
| [waterplane.wld#L103](../../../areas/wld/waterplane.wld#L103) | 1 |
| **Total** | **14** |

Two other unregistered zones contain four room-#1 commands:
[`Magetower.zon` line 8](../../../areas/zon/Magetower.zon#L8) has one D
door-state command; [`mavulsk.zon` line 8](../../../areas/zon/mavulsk.zon#L8)
has one D command and [M placements at lines 532](../../../areas/zon/mavulsk.zon#L532)
and [534](../../../areas/zon/mavulsk.zon#L534). Neither zone is in the full
area manifest. `ud-inside.zon` has no such commands. A repository-wide static
scan of 872 type-25 object records found no teleport destination of #1. These
source references preserve the shared Limbo-number history; none assigns #1
to `ud-inside`.

## Title, history, and disposition

“The Horrible Tracts of the Unnamed” is the exact normalized ZON title shared
by five sources: `astral`, `eoo_op2`, `mtncastl`, `theurgist`, and `ud-inside`.
All five are outside the current full-area list. Separately, the #1 WLD
placeholder block also matches `astral` and `mtncastl`; neither the shared
title nor shared bytes identifies the canonical editor source. The wider
room-#1 collision and the Limbo code reservation are summarized in the
[duplicate and replacement review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-548-ud-inside-retains-a-limbo-numbered-placeholder).

Followed history shows the WLD in the May 12, 2011 commit. MOB and OBJ history
reaches the 2005 and 2006 tracked commits and the 2011 commit. ZON history
reaches the August 12, 2009 area-tree move, the 2011 commit, and an October
2015 zone overhaul. These paths contain no explicit `ud-inside` replacement
or canonical-owner decision.

Keep `ud-inside` **RETAIN IN PLACE** outside both manifests. Preserve its
single placeholder and keep numeric #1 bound to the active Limbo identity.
Do not activate, merge, move, or archive it based on the shared title or
placeholder bytes. Resolve the editor project, source owner, and persisted
room identity first. The [20 builder questions](UD_INSIDE_BUILDER_QUESTIONS.md)
record those decisions. No build, boot, deployment, database, or editor state
was inspected.

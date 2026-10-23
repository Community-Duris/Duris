# Alterian Mountains source audit

Checkpoint 445 reviews the unregistered `component:alterian_mountains` source
set. It reconciles the mountain map's story, room identities, source-only
links, reset records, and typed dependencies with active Mining and the
unregistered Alterian Wilderness set. This is a static source review; it does
not establish loader behavior, runtime traversal, or external editor state.
See the [story dossier](../zone-stories/ALTERIAN_MOUNTAINS.md) and
[builder questions](../zone-stories/ALTERIAN_MOUNTAINS_BUILDER_QUESTIONS.md).

## Inventory and current membership

| Source | Current evidence |
| --- | --- |
| [Mountain WLD](../../../areas/wld/alterian_mountains.wld#L1) | 80 room records, #4920–#4999, with 176 directional exit records. |
| [Mountain MOB](../../../areas/mob/alterian_mountains.mob#L1) | 18 prototypes, #4817–#4834. |
| [Mountain OBJ](../../../areas/obj/alterian_mountains.obj#L1) | One prototype, object #4900, named `nothing`. |
| [Mountain ZON](../../../areas/zon/alterian_mountains.zon#L1) | Zone number 49, title “Alterian Region - Mountains,” 22 M resets and one E reset. |
| QST / SHP / map / story sidecar | No same-stem source exists. |
| Current manifests | `alterian_mountains` is absent from [areas/AREA](../../../areas/AREA), [areas/qst/AREA](../../../areas/qst/AREA), and the empty `areas/RANDOM_AREA`. |

The active area `mining` is listed at [areas/AREA line 25](../../../areas/AREA#L25).
Both `mining.zon` and `alterian_mountains.zon` use zone number 49, but their
titles and source records differ. The source-only `alterian_wilderness` zone
is also unlisted and has its own zone number and data files.

## Numeric identity and bounded history

Active Mining defines 69 rooms from #4900 through #4968. This source defines
80 rooms from #4920 through #4999. Their shared interval contains 49 VNUMs;
all 49 Mountain room records differ from the same-numbered active Mining
records after line-ending normalization. The VNUM overlap does not show that
either map replaced or copied the other.

The namespaces also contain a direct object collision. Mountain object #4900
is `nothing`; active [Mining object #4900](../../../areas/obj/mining.obj#L1)
is a wooden crate, reset by [mining.zon](../../../areas/zon/mining.zon#L97).
There is no matching mobile overlap between the two components. The Mountain
mobile range is #4817–#4834.

The bounded tracked-path review reaches the 2009 storage-layout move, a 2011
mobile-file edit, the 2012 reset-format update, and a 2013 source revision.
It shows no alternate-stem predecessor or whole-zone replacement.
The distinct “Alterian Region - Mountains” title appears in its own ZON
header; the shared numeric zone number 49 is with active Mining's different
title, “The Halfling Silver Mine.”

## Authored place and story

The rooms describe a difficult route through the Alterian range and up
Mt. Gandar: winding trails, steep passes, narrow ledges, scrambles, cracks,
and exposed slopes. The prose emphasizes effort and risk without specifying
a complete traversal or safety outcome. A hot, steaming high-altitude lake
suggests an underground heat source. A small natural cave leads to a dark
animal lair with picked-clean humanoid bones. Farther south, the hot cave
#4980 vents steam and descends to #4750, “Vertical Cavern,” in the separate
Alterian Wilderness WLD. The steep grades and hidden passages are source
facts; their runtime access is not verified.

At Keeper's Dale, #4995–#4999 describe a sheer gorge, a hidden stone staircase,
and large metal doors at the bottom. The east exit from #4999 stores target
`-1`, so the text does not prove that the doors connect to another room.
Other rooms show a treant, dire wolves, a Tharnadian half-elf scout, a hill
giant, primitive humans, troglodytes, farm residents, an adventurer, and
mountain wildlife. The source contains environmental and encounter story,
with no local QST, shop, or structured story contract.

## World connectivity

The 80-room source map has one weak component and 176 authored exit records.
Five exits use target `-1`. Two positive exits cross into source-only
Alterian Wilderness: Mountain #4920 west to Wilderness #4919, and Mountain
#4980 down to Wilderness #4750. Wilderness #4919 is itself also a different
active Mining room, so that integer has competing room identities. The
Wilderness file also defines #4750 as “Vertical Cavern.”

Three exits originate in Mountain-only room VNUMs but target numbers that are
also active Mining rooms: #4969 west to #4968, #4981 down to #4924, and #4986
up to #4925. The target numbers also have Mountain source records. These
source-only edges therefore sit across competing map identities; they are not
active ingress into this component. No active WLD exit targets the
Mountain-only range #4969–#4999. Candidate row 7 retains zero active inbound
exits. See the existing [Mining audit supplement](MINING_AUDIT_REVIEW.md#inactive-wld-sweep-and-overlapping-sources).

## Typed dependency census

The Mountain ZON contains 22 M commands and one E command. Sixteen M room
fields target the shared Mining interval #4920–#4968; the other six target
Mountain-only rooms. The Mountain M commands use local mobiles #4822 five
times and #4832 four times. Twelve references to #4813 and one to #4812 point
to mobile prototypes in `alterian_wilderness.mob`, not this MOB file. Its E
command references object #4806 from `alterian_wilderness.obj`.

The dependency also runs the other way: `alterian_wilderness.zon` has 30 M
commands using 13 mobile VNUMs defined by `alterian_mountains.mob`. Both
zones are outside the current full area list, so these reset references do
not prove either reset source is loaded.

Compiled code binds the `wanderer` callback to mobile #4830, which this MOB
file defines ([specs.assign.c line 624](../../../src/specs/specs.assign.c#L624)).
That code binding is a static dependency, not proof that the unregistered
mobile is present in the running index.

Other typed hits require namespace separation. Active `mining.zon` resets
active Mining object #4900, not Mountain's different object #4900. The
`mob_vnum` #4900 in [mining.story.json](../../../areas/story/mining.story.json#L27)
is an active Mining mobile. The Alatorin and Shady story-sidecar tokens that
contain #4931 and #4963 are string IDs, not Mountain room fields. The bounded
scan found no typed QST or SHP use of Mountain's object/mobile VNUMs and no
active ZON reset of mobile VNUMs #4817–#4834.

## Disposition

Keep candidate row 7 `alterian_mountains` **RETAIN IN PLACE**. Preserve the
80 Mountain room records, all 18 mobiles, object #4900, zone-number/title
identity, and both directions of its source-only Wilderness dependencies.
Reconcile the 49 conflicting room VNUMs and object #4900 with active Mining
before any co-load, merge, rename, or move. Retain Wilderness as a separate
candidate and preserve compiled callback #4830. External editor state,
runtime loading, and persisted data remain uninspected. See [candidate row 7](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L7),
[coverage row 358](ZONE_COVERAGE_REGISTER.csv#L358), and the
[duplicate/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-445-alterian-mountains-and-mining-identity).

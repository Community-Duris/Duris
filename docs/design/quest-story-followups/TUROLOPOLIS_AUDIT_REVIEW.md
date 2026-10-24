# The Ruins of Turolopolis: boundary and typed-reference supplement

Status: repository-wide static supplement to the existing
[Turolopolis dossier](../zone-stories/RUINS_OF_TUROLOPOLIS.md). It reconciles
active WLD boundaries, current unregistered WLD inputs, bounded external typed
references, a code-defined arrival candidate, and the active-title/history
boundary. No zone data, story sidecar, or gameplay code was changed or
qualified.

## Active and source-only WLD boundaries

The active area entry is `willem *71-72`; the canonical zone header is `#71`.
The existing dossier records 394 authored and loader-resolving exits, two
local weak components (largest: 153 rooms), no unresolved exits, four rooms
with fall chances, and no current fields. Its two active reciprocal boundaries
are Surface and Turolopolis Zoo:

| Active boundary | Evidence |
| --- | --- |
| Willem room 7102 south ↔ Surface room 548452 north | [Willem exit](../../../areas/wld/willem.wld#L73), [Surface exit](../../../areas/wld/surface.wld#L1017123) |
| Willem room 7123 west ↔ Turolopolis Zoo room 53000 east | [Willem exit](../../../areas/wld/willem.wld#L583), [Zoo exit](../../../areas/wld/turolzoo.wld#L14) |

Willem room 7198 also has six active one-way exits to Surface rooms 578047,
532894, 551641, 550095, 514897, and 524045. No reverse WLD records lead back
to 7198; the existing dossier identifies this empty Lothrell start room and
its actual foreign reset separately: [north](../../../areas/wld/willem.wld#L2308),
[east](../../../areas/wld/willem.wld#L2312),
[south](../../../areas/wld/willem.wld#L2316),
[west](../../../areas/wld/willem.wld#L2320),
[up](../../../areas/wld/willem.wld#L2324), and
[down](../../../areas/wld/willem.wld#L2328).

A stateful parse of all 95 current unregistered WLD files read 2,080,773
authored D0–D9 exits with zero parse issues. One source-only exit targets an
active Willem room:

| Unregistered source record | Direction and active target | Evidence |
| --- | --- | --- |
| `surf.wld` room 236469 | North (`D0`) to Willem room 7102 | [exit target](../../../areas/wld/surf.wld#L765120) |

This is source-only, not active-world ingress. It does not change the active
394-exit census or establish usable access, safe arrival, or a return path.
It brings the curated source-only record count from 369 to 370.

## Direct external typed references

The existing dossier maps the foreign Surface reset for Lothrell, the local
quest contracts, and the selected active boundaries. This bounded exact scan
also found two references in the unregistered `nexus56` source group to local
biography object 7129:

| Current source | Willem prototype and source role | Evidence |
| --- | --- | --- |
| Active Surface reset | Reset group declares local Lothrell mobile 7121 in room 515264 | [reset row](../../../areas/zon/surface.zon#L122), [mobile](../../../areas/mob/willem.mob#L311) |
| Unregistered Nexus56 reset | `G` reset equips local biography book 7129 on mobile 83038 | [reset group](../../../areas/zon/nexus56.zon#L1371), [object reset](../../../areas/zon/nexus56.zon#L1379), [book](../../../areas/obj/willem.obj#L463) |
| Unregistered Nexus56 shop list | Shop keeper 83038's stock includes local biography book 7129 | [shop keeper and stock](../../../areas/shp/nexus56.shp#L493), [stock entry](../../../areas/shp/nexus56.shp#L502), [book](../../../areas/obj/willem.obj#L463) |

The Nexus56 reset and shop records are outside the active area list; they are
source-path dependencies, not active stock or a current active-area consumer.
The exact active MOB, OBJ, QST, SHP, ZON, and story-sidecar scan found no other
direct external Willem prototype reference beyond the Surface mobile reset
above. Numeric tokens in unrelated object value/affect records, damage dice,
and historical code comments were not treated as prototype links.

## Code-defined arrival candidate

The shared `shift_prime` handler's Githzerai destination array includes Willem
room 7117, **A Small Shop**. The Githzerai branch selects one of the fourteen
listed VNUMs and resolves it through `real_room`; all fourteen currently occur
in active WLD source: #41767, #20484, #6224, #66651, #7117, #93665, #16805,
#5759, #2018, #99295, #93919, #16302, #574427, and #557620. Githzerai receive
the innate at level 1.
[Destination array](../../../src/classes/innates.c#L2010),
[Githzerai branch](../../../src/classes/innates.c#L2040),
[room resolution](../../../src/classes/innates.c#L2043),
[innate assignment](../../../src/classes/innates.c#L505), and
[room 7117](../../../areas/wld/willem.wld#L421).

This code-defined destination sits outside the static WLD exit and type-25
portal counts. The shared handler checks for the Astral zone and other innate
conditions; this source scan does not qualify its invocation, selected arrival,
or a player's path through the room. [Zone guard](../../../src/classes/innates.c#L2021).

## Active title and tracked history boundary

The normalized active title “The Ruins of Turolopolis” has one owner: area
`willem`, zone 71. Followed WLD, MOB, OBJ, ZON, and QST histories remain on the
`willem` stem and reach the August 2009 areas-tree move. The same-stem story
sidecar was added on October 4, 2026. No alternate-stem predecessor, distinct
renamed source, or whole-area replacement was identified in the bounded
current-source and tracked-history review.

The nearby Turolopolis Zoo is a separate active contributor with its own title,
map, and local hunter contract; its reciprocal east/west boundary with
Turolopolis joins adjacent content rather than replacing it. The
inactive Surf exit into room 7102 and the shared Githzerai destination in room
7117 are dependencies/arrival evidence, not another owner of the city. The
prior semantic-comparison question is closed for these reviewed candidates;
command-target alias disambiguation remains separately pending in coverage
column 37.

[Active title](../../../areas/zon/willem.zon#L2) ·
[active-area entry](../../../areas/AREA#L40) ·
[duplicate/replacement register](DUPLICATE_AND_REPLACED_ZONES.md)

## Audit boundary

These are static source findings. Active boundary records do not qualify route
access, the Surface source's live reset generation, successful shop stock,
code-driven use of `shift_prime`, arrival safety, or return travel. Editor and
tooling references, persisted-player state, and indirect or dynamic consumers
remain outside this bounded closure.

## Checkpoint 434: builder question review

The existing source-comprehensive dossier and this audit supplement already
cover the six exact item offerings, five statue/badge routes, Surface giver,
shared code arrival, one-way boundaries, and the unregistered Nexus56 typed
references. Twenty direct questions and recommended resolutions now collect
the remaining content/owner decisions in
[Turolopolis builder questions](../zone-stories/RUINS_OF_TUROLOPOLIS_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).
This does not change any quest, map, runtime, or accounting behavior.

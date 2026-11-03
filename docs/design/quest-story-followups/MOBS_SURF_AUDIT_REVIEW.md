# Mobs_Surf repository-wide audit supplement

This source audit follows the active contributor in
[coverage row 185](ZONE_COVERAGE_REGISTER.csv#L185). It records active graph
structure, outgoing old-map identities, cross-zone reset consumers,
normalized title ownership, and bounded same-stem history. No area or gameplay
source changed.

## Active graph and unresolved Surface targets

Active mobs_surf has 24 rooms and 64 authored exits: 40 local exits and 24
positive targets that do not resolve in the active world. The local weak
components contain 11, 5, 5, and 3 rooms. There are no active cross-area
boundaries, fall markers, or current fields.

| Local room pocket | Source-only destination VNUMs | Local WLD evidence |
| --- | --- | --- |
| #31401–#31411, Shady/Goblin/Faang | #285519, #284018, #282526, #282525, #284032, #285517, #284918, #286118, #282824, #283425, #283126 | [first](../../../areas/wld/mobs_surf.wld#L45), [last](../../../areas/wld/mobs_surf.wld#L178) |
| #31412–#31416, Bugbear | #281338, #280738, #279243, #279830, #280729, #281643 | [first](../../../areas/wld/mobs_surf.wld#L203), [last](../../../areas/wld/mobs_surf.wld#L259) |
| #31417–#31421, Faang spiders | #284034, #284335, #284036, #284333 | [first](../../../areas/wld/mobs_surf.wld#L293), [last](../../../areas/wld/mobs_surf.wld#L332) |
| #31422–#31424, Faang map load | #285238, #284338, #285236 | [first](../../../areas/wld/mobs_surf.wld#L349), [last](../../../areas/wld/mobs_surf.wld#L375) |

Every target VNUM occurs in both inactive surf.wld and surface2011.wld. Their
same-numbered rooms have different identities. For example, #285519 is a
grassy rocky-hills room in surf.wld ([source](../../../areas/wld/surf.wld#L1794535))
and an Azure Waters room in surface2011.wld
([source](../../../areas/wld/surface2011.wld#L1795900)). Neither old WLD is
active input, so these declarations remain unresolved and do not establish
ingress, replacement, or safe travel. All 24 exact source exits were already
recorded in the coverage census; this checkpoint adds no route or candidate.

## Typed reset relationships

The local mobs_surf.zon places external Bloody Plains mobile #33500 into room
#31412 with twenty repeated M commands, each carrying a population value of
20 and a 100% chance. The local prototype is
[Bloody Plains mobile #33500](../../../areas/mob/bloody_plains.mob#L1).
The repeated M rows appear at [line 10](../../../areas/zon/mobs_surf.zon#L10)
and [line 29](../../../areas/zon/mobs_surf.zon#L29). The local file defines
nine different mobiles, #31401–#31409.

Active external consumers of those nine local prototypes are:

| Owner | Reset references | Evidence |
| --- | ---: | --- |
| Active surface.zon | 54 M commands across #31401–#31409 | [first](../../../areas/zon/surface.zon#L83), [last](../../../areas/zon/surface.zon#L142) |
| Active multiclass_temple.zon | 2 M commands for #31402 and #31403 in room #52909 | [golem](../../../areas/zon/multiclass_temple.zon#L20), [crystal](../../../areas/zon/multiclass_temple.zon#L21) |
| Inactive surf.zon | 25 source-only M commands across #31401–#31409 | [first](../../../areas/zon/surf.zon#L249), [last](../../../areas/zon/surf.zon#L273) |

These are reset relationships, separate from WLD route declarations. No other
active reset consumer or direct QST, sidecar, SHP, duplicate-prototype, or
compiled numeric reference to the local mobiles was found. No exact room
VNUM from #31401–#31424 appears as an external WLD exit target or direct
object-portal/switch destination in the bounded scan.

The [coverage row](ZONE_COVERAGE_REGISTER.csv#L185) notes that
areas/qst/AREA lists mobs_surf at [line 142](../../../areas/qst/AREA#L142),
although areas/qst/mobs_surf.qst is absent. There is no local story sidecar,
shop file, or object prototype; areas/obj/mobs_surf.obj is empty. The active
area list includes mobs_surf at [line 153](../../../areas/AREA#L153).

## Title, source history, and disposition

After stripping Duris color tokens and the title terminator, then normalizing
case and whitespace, “The Surface World” has one owner among all 442 current
.zon files: active mobs_surf ([title source](../../../areas/zon/mobs_surf.zon#L2)).
The followed WLD, MOB, OBJ, and ZON paths remain same-stem through the
repository area-tree move and later edits. No alternate-stem rename or
whole-zone replacement is established by those histories.

The active surface area consumes Mobs_Surf mobiles through typed reset rows;
this is prototype ownership, not evidence that mobs_surf duplicates or
replaced the surface room map. The 24 unresolved destinations point into two
inactive map generations whose same-numbered records differ. Preserve active
mobs_surf DO NOT MOVE and keep the 24 targets as unresolved source evidence
until the map owner identifies intended destinations.

## Checkpoint 287: direct builder and owner questions

| # | Question | Source evidence | Recommended resolution |
| ---: | --- | --- | --- |
| 1 | Are the four disconnected room groups intended as separate spawn pockets rather than one walkable region? | [Room groups](../zone-stories/MOBS_SURF.md#contributor-inventory), [active graph](MOBS_SURF_AUDIT_REVIEW.md#active-graph-and-unresolved-surface-targets) | Preserve the four local components until the owner confirms authored connectivity; do not infer travel between pockets. |
| 2 | Which old Surface map should the 24 outbound exits use when every target VNUM exists in both inactive surf.wld and surface2011.wld? | [Grouped targets](MOBS_SURF_AUDIT_REVIEW.md#active-graph-and-unresolved-surface-targets), [different #285519 identities](../../../areas/wld/surface2011.wld#L1795900) | Keep the active targets unresolved until the map owner selects the authoritative generation and exact room identity. |
| 3 | Should the source-only surf and surface2011 room records remain separate where the same VNUM has different landscape titles and descriptions? | [surf #285519](../../../areas/wld/surf.wld#L1794535), [surface2011 #285519](../../../areas/wld/surface2011.wld#L1795900) | Preserve both historical identities; do not alias or merge rooms by number alone. |
| 4 | Are the Shady/Goblin/Faang, Bugbear, and two Faang labels intentional spawn/aggro markers, despite the unresolved exits? | [WLD labels](../../../areas/wld/mobs_surf.wld#L1), [reset layout](../../../areas/zon/mobs_surf.zon#L10) | Treat labels as source notes, not player directions or evidence of active aggressive encounters. |
| 5 | Are the nine local ice prototypes intended as a shared population library used by Surface and Multiclass Temple resets, rather than placed by mobs_surf.zon itself? | [Local mobile source](../../../areas/mob/mobs_surf.mob#L1), [active consumer](../../../areas/zon/surface.zon#L83) | Preserve the typed prototype definitions and their existing reset owners; do not add local placements without owner intent. |
| 6 | Should active surface.zon continue to place all nine local mobile VNUMs at its current room groups? | [Surface reset rows](../../../areas/zon/surface.zon#L83) | Keep the 54 declarations distinct by prototype, room, and cap; qualify active reset generation before claiming live stock. |
| 7 | Should multiclass_temple.zon keep its one ice golem and one crystal reset at room #52909? | [Multiclass resets](../../../areas/zon/multiclass_temple.zon#L20) | Preserve both typed records and their destination; do not treat these as local mobs_surf placements. |
| 8 | Should the 25 unregistered surf.zon resets for Mobs_Surf prototypes remain source-only at their old map coordinates? | [Surf resets](../../../areas/zon/surf.zon#L249), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97) | Retain them with the existing Surf source group; they do not create active stock or justify moving its files. |
| 9 | Is Bloody Plains mobile #33500, a mature male leucrotta, intentionally imported into mobs_surf room #31412? | [Mobs_Surf reset](../../../areas/zon/mobs_surf.zon#L10), [mobile definition](../../../areas/mob/bloody_plains.mob#L1) | Preserve the typed cross-zone link until both source owners confirm intent and population ownership. |
| 10 | Are the twenty identical M resets for #33500 deliberate, with their shared population value of 20 and 100% chance? | [First and last repeated reset](../../../areas/zon/mobs_surf.zon#L10), [reset summary](MOBS_SURF_AUDIT_REVIEW.md#typed-reset-relationships) | Do not collapse rows or promise twenty live creatures; qualify the runtime population cap and reset timing first. |
| 11 | Should the three ice-golem prototypes #31402, #31407, and #31408 retain separate VNUMs despite matching keywords and descriptions? | [Prototype records](../../../areas/mob/mobs_surf.mob#L12) | Preserve distinct identities and all active/source-only reset references; names do not merge their spawn records. |
| 12 | Is #31404 intended to remain a mobile prototype called “ice pile,” rather than becoming scenery or an object? | [Ice pile record](../../../areas/mob/mobs_surf.mob#L34), [empty object file](../../../areas/obj/mobs_surf.obj) | Retain its current type and consumers until the builder selects a separate, typed conversion. |
| 13 | Is the qst/AREA entry intentional when no areas/qst/mobs_surf.qst file or native Q/QA contract exists? | [QST/AREA entry](../../../areas/qst/AREA#L142), [active area entry](../../../areas/AREA#L153) | Confirm whether the empty manifest entry is deliberate; do not invent a contract or remove the entry during this audit. |
| 14 | Is the present zero-byte mobs_surf.obj file an intentional empty placeholder? | [Object path](../../../areas/obj/mobs_surf.obj), [local inventory](../zone-stories/MOBS_SURF.md#contributor-inventory) | Preserve the source layout until its owner decides whether the empty file is needed. |
| 15 | Is this contributor meant to remain without shops, local objects, QST contracts, and a story sidecar? | [Local inventory](../zone-stories/MOBS_SURF.md#contributor-inventory) | Keep absence as a documented source finding; require explicit builder intent before adding services or story progress. |
| 16 | Should reset mode 2 remain separate from any promise that the imported leucrotta or local ice populations renew on schedule? | [Zone header](../../../areas/zon/mobs_surf.zon#L3), [population resets](../../../areas/zon/mobs_surf.zon#L10) | Preserve current mode and caps; qualify ready reset generation and actor availability before claiming renewal. |
| 17 | Do local room records #31401–#31424 have no external inbound exits, as the bounded active and inactive WLD scan found? | [Exact WLD exit inventory](MOBS_SURF_AUDIT_REVIEW.md#active-graph-and-unresolved-surface-targets) | Keep ingress unclaimed unless a new typed room reference is found; do not infer arrival from reset consumers. |
| 18 | Should the generic normalized title “The Surface World” remain attached to the separate mobs_surf contributor? | [Unique title](MOBS_SURF_AUDIT_REVIEW.md#title-source-history-and-disposition), [current title](../../../areas/zon/mobs_surf.zon#L2) | Retain the current identity; resolve external editor aliases separately if the owner finds a collision. |
| 19 | Should the mobile numeric flags and special blocks remain unchanged while no local C/C++ procedure binding was found? | [Mobile inventory](../zone-stories/MOBS_SURF.md#contributor-inventory), [bounded compiled scan](MOBS_SURF_AUDIT_REVIEW.md#typed-reset-relationships) | Keep decoded runtime meaning unclaimed until a dedicated mobile-flag review or gameplay qualification establishes it. |
| 20 | Is any zone retirement, merge, or reconnect authorized by the 2009 path history, the active Surface reset consumers, or the inactive map exits? | [Title/history review](MOBS_SURF_AUDIT_REVIEW.md#title-source-history-and-disposition), [reset consumers](MOBS_SURF_AUDIT_REVIEW.md#typed-reset-relationships) | No. Keep active mobs_surf DO NOT MOVE; require a map-owner decision and complete destination/reference review before separate changes. |

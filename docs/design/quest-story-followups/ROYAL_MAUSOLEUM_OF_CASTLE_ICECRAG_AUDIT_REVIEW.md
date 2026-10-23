# The Royal Mausoleum of Castle IceCrag: repository-wide connectivity and dependency audit

**Checkpoint 93; priority 133 of the original 220-zone queue.** This
supplement reconciles the [source-comprehensive dossier](../zone-stories/THE_ROYAL_MAUSOLEUM_OF_CASTLE_ICECRAG.md)
with the active WLD graph, all inactive WLD sources, typed external references,
C/C++ numeric source matches, normalized active title, and followed WLD/ZON
history. Authored exits and map declarations do not establish door admission,
safe traversal, or played qualification.

## Active WLD graph

The current active manifest has 351 WLD contributors. Voluntown has 133 rooms
and 286 authored exits; all 286 resolve with no parse issue. The local graph
has one weak component containing all 133 rooms: 285 internal exits and one
cross-area exit. There is one reciprocal IceCrag boundary: Voluntown room
#142400 south to IceCrag room #97132, and IceCrag room #97132 north back to
Voluntown room #142400 ([Voluntown exit](../../../areas/wld/Voluntown.wld#L12),
[IceCrag exit](../../../areas/wld/icecrag.wld#L3415)). No other active inbound
or outbound WLD boundary, negative target, unresolved target, source room
outside the numeric range, or range-mismatched source exit was found. The
graph scan found one fall-chance room, the cliff at #142449, and no current
fields ([cliff source](../../../areas/wld/Voluntown.wld#L1073)).

The weak component is a structural reading of authored exits. Locked doors,
trap state, item custody, actual entry, and safe return remain separate. The
source dossier maps those local conditions and the separately tracked
capitalized-filename loader repair; this supplement does not qualify them in a
running server.

## Inactive WLD sources

The all-95 inactive-WLD scan parsed 2,080,773 exits without issues. It found
532 `Duris3.wld` exit records targeting Voluntown rooms. The inactive file also
contains 133 source-room records with VNUMs #142400–#142532; exits from those
overlapping VNUMs account for 264 target records. The remaining 268 records
originate in non-overlapping map rooms and cover all 133 Voluntown rooms:

| Inactive map-source records | Destinations | Evidence |
| --- | --- | --- |
| 133 south (`D2`) exits from #142100–#142232 | One per room, #142400–#142532 | [`#142100`](../../../areas/wld/Duris3.wld#L884113) to [`#142232`](../../../areas/wld/Duris3.wld#L886885) |
| 133 north (`D0`) exits from #142700–#142832 | One per room, #142400–#142532 | [`#142700`](../../../areas/wld/Duris3.wld#L896705) to [`#142832`](../../../areas/wld/Duris3.wld#L899477) |
| Two endpoint exits: #142399 east (`D1`) and #142533 west (`D3`) | #142400 and #142532 | [`east`](../../../areas/wld/Duris3.wld#L890388), [`west`](../../../areas/wld/Duris3.wld#L893210) |

`Voluntown` appears at [line 404](../../../areas/AREA#L404) of the current
active `areas/AREA` manifest; `Duris3.wld` is not listed. The 268 map
declarations are source-only and do not add active ingress.
They raise the curated source-only reference total to 1,505; the reviewed Surf
subset remains 122. The other 264 records are excluded because their source
room VNUMs overlap the active Voluntown room range.

## Typed references and external producers

The exact external scans found no QST item command, SHP product, ZON reset-role
reference, or external mobile/object prototype definition for a local
Voluntown VNUM. Across 872 external type-25 declarations (713 active and 159
source-only), none targets a Voluntown room. All 231 external story sidecars
parsed without errors and none contains a structured Voluntown room, mobile,
or object VNUM reference.

The local ZON also consumes two imported objects on Haratius #142425:
`Voluntown.zon` line 152 resets object #359, owned by `heavens.obj`; its
`epic_stone` callback is assigned in `specs.assign.c` line 1604. Line 153
resets object #22070, owned by `cerebusp.obj`; `specs.assign.c` line 1538
binds `revenant_helm`, and `specs.clfhaven.c` line 86 contains its rareload
display check. The Pits of Cerberus audit also records this Voluntown reset.
These are outbound dependencies from Voluntown to external object owners, not
external consumers of local Voluntown prototypes. Preserve each owner and
handler when assessing disposition.


The exact-number C/C++ source scan found 24 item-slot matches in the generated
chaos equipment tables
([first](../../../src/account/chaos_eq_data.h#L73),
[last](../../../src/account/chaos_eq_data.h#L319)). They reference eight local objects: #142416, #142419,
#142422, #142435, #142445, #142447, #142448, and #142449. These are alternate
equipment sources already mapped by the source dossier, not local quest
callbacks. No local room or mobile VNUM matched the source scan.

## Title, history, and disposition

The normalized title in
[`Voluntown.zon`](../../../areas/zon/Voluntown.zon#L2) matches exactly one of
the 351 active contributors. Followed WLD/ZON history records continued
maintenance, including room/teleport and key-placement changes, quest-item
flag updates, and the Sanguine Song move and proc adjustment. The tracked
Voluntown WLD/ZON source is introduced under the same stem by commit
`e71c1369d` on 2012-12-29; no followed history establishes an alternate-stem
predecessor, title rename, or whole-area replacement. The bounded semantic
comparison is now recorded in [Checkpoint 427 of the duplicate/replacement
register](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-427-voluntown-title-history-and-icecrag-relationship):
IceCrag Castle is the closest related active contributor, joined to this
separate Mausoleum by the reciprocal #97132/#142400 boundary, while the
Pits-of-Cerberus-owned #22070 is only an imported object dependency. Neither
relationship shows duplicate ownership or replacement. Command-target alias
and owner disambiguation remains a separate name-review question.

The current un-commented
[`areas/AREA` row](../../../areas/AREA#L404) remains **DO NOT MOVE**. The
inactive map overlap is not replacement evidence. The existing dossier retains
the two exact returns, access and trap distinctions, imported callbacks,
alternate equipment dependencies, scene gaps, and builder questions with
recommended resolutions.

This checkpoint used repository sources and generated documentation only. No
tests, build, server, gameplay session, database operation, migration, or
operational script was run.


### Directed paths under an open-edge abstraction

Following only the 285 resolving internal WLD exits, with every authored edge
treated as traversable, room #142400 reaches 132 of 133 local rooms; #142472
is the only omitted room. All 133 rooms have a directed path back to #142400.
The resulting strongly connected components have sizes 132 and 1. Room
#142472 has no incoming local WLD exit and one outgoing exit, south to
#142471 ([room record](../../../areas/wld/Voluntown.wld#L1488)). No ZON command
mentions #142472. These results describe authored topology only: the traversal
does not apply the closed/locked entry resets, keys, traps, fall chance,
resets, actor state, or runtime scripts. The one reciprocal active boundary
remains IceCrag #97132 ↔ Voluntown #142400; the 268 non-overlapping Duris3 map
approaches remain inactive source declarations, not live ingress.


## Checkpoint 427: directed topology, outbound imports, and reconciliation

The current raw directed scan confirms 132/133 rooms reachable from #142400
over internal WLD exits and all 133 able to return to #142400. #142472 is the
singleton SCC: it has no incoming local WLD edge and one exit to #142471. The
open-edge scan does not account for the closed/locked entry, key custody,
traps, fall state, resets, or runtime behavior. Preserve the separate
reciprocal IceCrag boundary and the inactive Duris3 approach records.

Voluntown's ZON resets imported objects #359 (`heavens.obj`, assigned
`epic_stone`) and #22070 (`cerebusp.obj`, assigned `revenant_helm`). These
outbound object dependencies supplement the inbound scan of 872 external
declarations and the 24 C/C++ equipment-table matches to eight local objects;
the two directions have different owners and meanings. The completed bounded
title/history comparison records IceCrag as a related but separate owner, no
alternate-stem predecessor or replacement, and the Pits crown as a typed
dependency only. No native area or gameplay source changed.

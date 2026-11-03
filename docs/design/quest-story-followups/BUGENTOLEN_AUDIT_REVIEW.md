# Bugentolen repository-wide audit reconciliation

This checkpoint reconciles the existing
[Bugentolen source-story dossier](../zone-stories/BUGENTOLEN.md) with the
coverage register and refreshes its connectivity and dependency evidence. The
reviewed source snapshot is `72ba3deff3bb30bde880018cdebe7c6db675b697`. The
dossier dates from Checkpoint 27; it was present in the repository but had no
mapped-dossier entry in the coverage row. No area or gameplay source is changed.

## Active source identity

`bugentol` is an un-commented contributor in the active
[world manifest](../../../areas/AREA#L343) and also appears in
[areas/qst/AREA](../../../areas/qst/AREA#L303). The active source has 125 rooms
(#95800–#95924), 27 mobile prototypes (#95800–#95826), 18 object prototypes
(#95800–#95817), and 329 ZON commands. There is no same-stem `.qst`, native
Q/QA contract, `.shp`, or story sidecar. QST/AREA membership does not establish
local quest content.

The [existing dossier](../zone-stories/BUGENTOLEN.md) documents the settlement,
its labor and prison scenes, reset placements, and builder questions. This
checkpoint maps that dossier to coverage row
[42](ZONE_COVERAGE_REGISTER.csv#L42); it does not create a duplicate story
dossier.

## Active WLD topology and reset state

All 253 authored exits resolve: 252 are local and one leaves Bugentol. The 125
rooms form one weak component when all authored directions are available. Raw
directed traversal from both active inbound roots reaches and returns from all
125 rooms. The two local door pairs have four directional edges; each endpoint
has a matching ZON state-1 reset that closes it without locking it. Treating
those four reset-closed edges as unavailable leaves raw directed reach and
return of 110/125 from either inbound root. The local weak graph then separates
into components of 110, 14, and one room. This is a static edge-removal result,
not a test of opening doors, reset timing, player access, or live travel.

| Active boundary | Source edge | Reverse WLD edge |
| --- | --- | --- |
| Inbound from White Harbor | [Wh #55402 east](../../../areas/wld/wh.wld#L8982) enters Bugentol #95813 | None |
| Paired Surface boundary | [Surface #589071 west](../../../areas/wld/surface.wld#L1869883) enters Bugentol #95800 | [Bugentol #95800 east](../../../areas/wld/bugentol.wld#L15) returns to Surface #589071 |

There are two active inbound edges and one outbound edge, with one reciprocal
pair. The [exit ledger rows 751](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L751),
[1150](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1150), and
[1550](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1550), and pair-summary rows
[63](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L63),
[474](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L474), and
[784](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L784) retain these records.
The four reset-closed exits are the curtain pair #95826 west ↔ #95827 east
and the door pair #95885 west ↔ #95886 east
([WLD records](../../../areas/wld/bugentol.wld#L612),
[ZON resets](../../../areas/zon/bugentol.zon#L8)). The main gate description
does not correspond to a door-coded/reset exit. No room has a fall marker or
current field.

The all-95 inactive-WLD scan found one non-overlapping source-only approach:
inactive `surf.wld` room #280727 west to Bugentol #95800
([source room](../../../areas/wld/surf.wld#L1693959),
[exit](../../../areas/wld/surf.wld#L1693975),
[target](../../../areas/wld/surf.wld#L1693978)). It is not active ingress and
was already counted in the curated source-only totals. As of this
checkpoint those totals remain 3,444 overall, 209 for Surf, and 16 for
`ud_map2`. Preserve the source-only edge with existing
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); its 145
active inbound references are unchanged. Add no Bugentol candidate row.

## Typed dependencies and bounded history

The refreshed exact scans found no external ZON reset-role reference to
Bugentol's local room, mobile, or object VNUMs; no exact local-ID reference in
external QST, SHP, or story-sidecar sources; no literal room or prototype
number binding in C/C++; and no external MOB/OBJ prototype duplicate. The scan
of 872 external ITEM_TELEPORT declarations and 380 external ITEM_SWITCH
declarations found no room target in Bugentol. The active Wh arrival and inactive
Surf approach above are the identified cross-area WLD dependencies. This is
bounded static evidence; editor/tooling, persisted references, indirect data,
and dynamic routes remain outside the scan.

A whole-directory `areas/zon/*.zon` inventory found reset rows for D, E, F, G,
M, O, P, and R; A, B, C, and Y rows are absent. The external typed scan checked
the loader-typed argument positions for every present reset family.

The normalized active title “Bugentolen” is unique. Followed WLD history shows
the 2011 east-exit edit from Surface #589071 to #259558 and the 2013 restore
to #589071. The old number #259558 has different room descriptions in
inactive `surf.wld` and `surface2011.wld`; inactive Surf room #280727 enters
Bugentol, while same-numbered `surface2011.wld` room #280727 describes stormy
sea and leads west to #280726. These map records and the temporary boundary
target do not establish a whole-zone replacement or predecessor. See the
[bounded title and path-history review](DUPLICATE_AND_REPLACED_ZONES.md#bugentol-and-parallel-surface-map-records).

Keep active `bugentol` **DO NOT MOVE**. Preserve both active arrivals, the
reciprocal Surface exit, the two reset-closed door pairs, and the already
recorded source-only Surf approach. Keep `surf` and `surface2011`
**RETAIN IN PLACE** until map provenance and typed/editor/persisted dependency
closure receive owner review. The existing candidate plan does not authorize a
move. No gameplay, source placement, or attic migration is performed here.

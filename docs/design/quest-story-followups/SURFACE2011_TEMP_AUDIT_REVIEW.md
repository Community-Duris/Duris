# surface2011-temp WLD source audit

Canonical key: component:surface2011-temp. Coverage data row 449 / CSV line 450; candidate dependency data row 98 / CSV line 99. Reviewed October 11, 2026, Checkpoint 536, against checkout c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static WLD, manifest, graph, map-identity, source-only-approach, and followed-history review. It does not establish a live connection or an authoritative editor/build role for a file named “temp.”

## Source inventory and status

The only tracked same-stem source is [surface2011-temp.wld](../../../areas/wld/surface2011-temp.wld#L1). It defines 90,000 rooms #600000–#689999 and 360,000 exits, 90,000 in each cardinal direction. There is no same-stem MOB, OBJ, ZON, QST, SHP, story sidecar, or full-area manifest entry; no zone header or zone title is available for this WLD-only component.

Current active Surface defines 60,004 rooms #600000–#660003, all with different full WLD records and different room titles from the matching surface2011-temp records. The other 29,996 surface2011-temp VNUMs #660004–#689999 have no active Surface room owner. No active WLD source directs into the temp-only identities, so candidate active inbound remains zero.

## Candidate graph and raw numeric matches

All 360,000 authored exits target one of the candidate’s own 90,000 defined rooms. The graph is one weak component, and a simple directed walk from #600000 reaches all 90,000 rooms without modeling room access. There are no external targets or negative exits in the candidate graph.

A raw numeric match scan finds 240,016 candidate exit records whose target VNUMs also occur in current active Surface. That count includes edges from same-numbered source rooms whose active Surface records are different; it is not a count of verified travel routes. A separate, source-only filter identifies 602 directions from 600 source-room VNUMs absent from the active WLD set into active Surface VNUMs. These are already represented in the existing Surface source-only census and global curated register; Checkpoint 536 adds no route total.

The 602 source-only tuples are exact matches for the 602 directions in newbiemaps.wld. They include 300 north edges from #660004–#660303 to #659704–#660003, 300 south edges from #689700–#689999 to #600000–#600299, plus #660004 west to #660003 and #660299 east to #660000. For example, surface2011-temp #660004 north to #659704 is at [line 1260092](../../../areas/wld/surface2011-temp.wld#L1260092), and its west edge to #660003 is at [line 1260104](../../../areas/wld/surface2011-temp.wld#L1260104). All 600 source-room titles differ between the two files. Their shared source VNUMs and direction tuples do not establish that either map is loaded or that its target is intended to be current Surface.

## Map identity and history

The WLD VNUM set overlaps 60,004 active Surface rooms with zero identical full room blocks and 89,847 newbiemaps rooms with one identical block, #666059 “The Ocean.” The complete records and titles otherwise distinguish the source families. The 602 matching source-only exit tuples appear in both inactive files, preserving two file identities while representing one repeated set of source/direction/target combinations.

Followed history reaches 2011 and includes an August 2026 line-ending cleanup. The “temp” stem alone does not record that the file is disposable. No explicit retirement, predecessor, successor, or whole-map replacement decision was found. See the [Surface Realm audit](SURFACE_AUDIT_REVIEW.md#inactive-wld-surface-approaches), [surface2011 audit](SURFACE2011_AUDIT_REVIEW.md), and the [candidate dependency row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L99).

## Disposition

Retain surface2011-temp.wld in place. Ask the map owner to resolve its 60,004 same-number but distinct Surface rooms, its 602 source-only boundary directions shared with newbiemaps, the repeated “temp” provenance, and the authoritative editor/export/build inputs before any separate merge, move, activation, or archive proposal. The twenty builder questions in [SURFACE2011_TEMP_BUILDER_QUESTIONS.md](SURFACE2011_TEMP_BUILDER_QUESTIONS.md) record the decisions. No migration clearance is implied.

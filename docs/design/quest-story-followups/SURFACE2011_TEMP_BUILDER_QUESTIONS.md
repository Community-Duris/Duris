# surface2011-temp builder questions and recommended resolutions

Checkpoint 536, source snapshot c500ac51b8cd26540943257e7ca4e9807263ef74. These questions concern the WLD-only map, its 60,004 room identity collisions with active Surface, 602 source-only directions duplicated in newbiemaps, and the “temp” filename. They do not authorize activation, movement, merging, or archiving.

## Explicit builder questions and recommended resolutions

1. **Who owns surface2011-temp.wld and can approve changes to its paths?** Recommended resolution: identify the map, editor, export, and deployment owners first.
2. **What does “temp” mean in this file name, and is it still an authoritative source?** Recommended resolution: confirm the filename’s provenance and document the current input/export path; do not infer disposability from its name.
3. **Should the 60,004 same-number room identities #600000–#660003 remain distinct from active Surface?** Recommended resolution: have both map owners resolve each overlap before using either file as a replacement or merging records.
4. **What is the relationship between surface2011-temp and newbiemaps?** Recommended resolution: document why their 602 source/direction/target tuples match and preserve both file histories while that is unresolved.
5. **Should the one exact room block shared with newbiemaps, #666059 “The Ocean,” remain duplicated?** Recommended resolution: confirm whether this single identical block is intentional while keeping the rest of the map identities distinct.
6. **Are all 90,000 room records and 360,000 cardinal exits the intended WLD output?** Recommended resolution: ask the mapper to confirm the complete #600000–#689999 map range and topology.
7. **How should the 240,016 raw target-number matches into active Surface be interpreted?** Recommended resolution: distinguish map identity from numeric lookup; only the 602 directions with source VNUMs absent from the active set belong in the existing source-only approach review.
8. **Are the 602 source-only directions from 600 rooms intended to lead into current Surface?** Recommended resolution: verify each source/destination group with the owning mapper; keep the current curated route count unchanged until map identity is decided.
9. **Should duplicate source-only tuples in newbiemaps be preserved in both files?** Recommended resolution: retain both records and their provenance; identical tuples do not show that one WLD supersedes the other.
10. **Why are there no active inbound WLD records to the temp-only VNUMs?** Recommended resolution: confirm whether the out-of-range rooms are meant to have no active-map entrances.
11. **Is there a zone title or numeric zone header that should accompany this WLD?** Recommended resolution: identify the authoritative ZON source if one exists; none is currently tracked under this stem.
12. **Is the source intended to remain WLD-only?** Recommended resolution: ask whether MOB, OBJ, ZON, QST, or deployment sidecars are maintained elsewhere before adding or removing companion files.
13. **Do the 602 directions represent a seam or export artifact shared by two map copies?** Recommended resolution: have the map author explain the repeated boundaries and record the accepted generation process.
14. **Which map owner should resolve Surface VNUMs #600000–#660003?** Recommended resolution: use the active Surface room records and builder decision for destination identity, not the candidate’s raw number matches alone.
15. **Should room #666059’s identical “The Ocean” block be the only shared full room record?** Recommended resolution: confirm the match against both editors and preserve all differing room text and exits.
16. **Are there external editor or generated copies that are newer than the 2026 line-ending cleanup?** Recommended resolution: check the build/export source and retain independent copies until provenance is established.
17. **Does the current Surface map intentionally omit connections to #660004–#660303 and #689700–#689999?** Recommended resolution: verify edge intent with the active Surface owner; no active exit targets these temp-only source identities.
18. **Do the 602 candidate tuples remain part of the existing global inactive-WLD census?** Recommended resolution: preserve their current audit links and deduplication policy; do not add a second count.
19. **What history explains creation and continued maintenance of this WLD?** Recommended resolution: ask its owner to document authorship, version purpose, and 2011/2026 source changes.
20. **What disposition should follow map-owner review?** Recommended resolution: retain the WLD in place until the owners document whether it is a separate map, a generator input, a test export, or a retired snapshot.

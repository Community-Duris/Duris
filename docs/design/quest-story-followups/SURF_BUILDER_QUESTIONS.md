# surf builder questions and recommended resolutions

Checkpoint 534, source snapshot c500ac51b8cd26540943257e7ca4e9807263ef74. These questions concern the unlisted World of Duris source, its 90,000-room map, two overlapping unlisted map variants, 145 ambiguous active inbound references, source-only resets, and direct code bindings. They do not authorize activation, source edits, movement, merging, or archiving.

## Explicit builder questions and recommended resolutions

1. **Who owns surf.wld, surf.mob, the empty surf.obj, and surf.zon?** Recommended resolution: identify map, mobile, deployment, and tool owners before any status or path change.
2. **Is “World of Duris” the approved title, and should any alias be recorded?** Recommended resolution: ask the source owner to confirm the title; its uniqueness among current ZON titles does not establish canonical intent.
3. **Why is surf absent from areas/AREA and areas/qst/AREA?** Recommended resolution: confirm intended loader and QST status with the manifest owner before changing either list.
4. **How does this Surf component relate to the separately titled active Surface Realm?** Recommended resolution: record the canonical identities and ownership of both sources; do not infer that their similar names mean one replaces the other.
5. **Is the 300 by 300, 90,000-room Surf grid the intended complete map?** Recommended resolution: have the mapper confirm the grid, room range, and any generator or editor inputs that produce it.
6. **What is the intended relationship between Surf, surface2011, and underdark2011?** Recommended resolution: preserve all three as distinct maps until their owners reconcile 89,999 and 15,481 same-number Surf room overlaps.
7. **Are the overlapping room and mobile VNUMs intentional parallel identities?** Recommended resolution: establish the authoritative source per range and do not treat matching numbers as proof of shared records.
8. **Which map identity should receive the 145 active inbound exit records?** Recommended resolution: resolve each target against a builder-approved map; 134 match Surf and surface2011, and 11 match all three unlisted variants.
9. **Are the 17 active source contributors’ exits into #200001–#290000 intended to remain?** Recommended resolution: confirm their ownership and destinations, retaining the source records until each target map is identified.
10. **What should happen to Surf’s three outbound targets found only in inactive maps: #35300, #67005, and #67087?** Recommended resolution: confirm asunwa and jungle ownership and intended connections before redirecting any exit.
11. **What are the intended destinations for the thirteen outbound target records without a current WLD definition?** Recommended resolution: check the map archive or generator source for the twelve listed VNUMs and avoid inventing destinations from adjacent numbers.
12. **Should the 75 matching authored reverse directions be retained as written?** Recommended resolution: review each against the correct map identity; a reverse source edge does not prove a current loaded route.
13. **Are all 453 M reset commands and 32 F room-field commands current?** Recommended resolution: have the map owner confirm reset scope and ensure any future manifest decision also resolves mobile and room identities.
14. **Should Surf continue to place Great Realm mobile prototypes?** Recommended resolution: preserve the 209 source-only references to 16 active Great Realm prototypes until the area and reset owners decide whether those encounters belong here.
15. **Should Surf’s Map5 reset references remain?** Recommended resolution: retain the 45 source-only commands using nine Map5 mobile VNUMs in 40 rooms until Map5 and Surf owners confirm the intended characters.
16. **Are the Killing Fields M, G, and E dependencies intended?** Recommended resolution: confirm mobile #33308, objects #33309–#33312/#33314/#33315, and equipped object #5720 with both source owners before changing them.
17. **Why does surf.obj exist as an empty file while resets import external objects?** Recommended resolution: ask whether the empty file is a required editor/export artifact and preserve the six G and one E references unless their intended replacements are identified.
18. **Should code continue to bind #200011, #200012, and #200321?** Recommended resolution: confirm the expected behavior with the code and mobile owners; the active MOB table currently has no Surf prototypes for these calls to resolve.
19. **Which files are authoritative inputs for the 2025 recode and August 2026 ANSI cleanup?** Recommended resolution: document the source editor, generator, export path, and deployment copy so future edits do not discard distinct map variants.
20. **What disposition should apply after ownership and toolchain review?** Recommended resolution: retain all four files in place until owners decide their role; do not activate, move, merge, or archive them based only on list exclusion or overlapping numbers.
21. **Do `surface2011.qst` and `underdark2011.qst` references inside #200000–#200321 identify Surf mobiles or objects?** Recommended resolution: preserve these unlisted QST headers and item operands under their own source stems. Their item fields resolve to unlisted OBJ records; numeric overlap with `surf.mob` does not establish a Surf QST dependency or active loader identity.

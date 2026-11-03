# NEWBIEMAPS builder questions

These source-backed questions are for the map, area, loader, editor, and
persisted-data owners. They record recommended resolutions; they do not imply
live travel or that either map should be loaded.

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| NBM-01 | Is “newbiemap” the intended canonical area title for the plural newbiemaps source stem? | Preserve the file stem and exact ZON title until the area owner confirms aliases. |
| NBM-02 | What project or map-export process produced the 89,847-room WLD? | Identify the editor/project source and record it with this retained package. |
| NBM-03 | Why does the ZON header use #10879352 while its last VNUM and WLD room band end at #689999? | Preserve the raw header and have the area/loader owner confirm the intended zone number and VNUM range. |
| NBM-04 | What do the ZON reset field 1635218021 and 300 300 row mean for this source format? | Confirm them against the generating tool; do not infer or normalize reset behavior from the text alone. |
| NBM-05 | Is the absence from both areas/AREA and areas/qst/AREA intentional? | Keep the file source-only unless the deployment owner approves manifest changes after map identity is resolved. |
| NBM-06 | Are the 153 absent room records inside #600000–#689999 intentional? | Preserve the current sparse source and compare it with the 2011 copy before restoring any records. |
| NBM-07 | Why does surface2011-temp.wld contain those 153 additional rooms and 804 additional exits? | Treat the files as separate versions and ask the map owner which differences are intentional. |
| NBM-08 | Which map is authoritative between newbiemaps, surface2011.wld, and surface2011-temp? | Record provenance and purpose for each path; the copy history alone does not establish replacement. |
| NBM-09 | How should newbiemaps coexist with active Surface on 59,851 same-number room VNUMs whose titles differ? | Preserve active Surface ownership and resolve the map identity overlap before any activation or renumbering. |
| NBM-10 | Are the 239,212 candidate exits that numerically reach active Surface rooms intended map-grid overlap or actual routes? | Classify them by source identity; do not treat the 238,610 directions from overlapping room VNUMs as new ingress. |
| NBM-11 | Are the 602 directions from 600 source-only rooms into Surface intended as approaches to the active map? | Preserve the existing 602 records and confirm destinations, entry intent, and return-path requirements. |
| NBM-12 | Should the paired north/south strips and two east/west edges be kept on both 2011-derived map paths? | Preserve each source identity while the map owner explains their shared tuples and differing records. |
| NBM-13 | Is it intentional that no current active WLD exit targets the candidate-only #660004–#689999 rooms? | Keep it source-only until the intended one-way approaches and ownership are documented; no active WLD ingress targets candidate-only VNUMs. |
| NBM-14 | Are the 24 title strings and single-room NEWBIE1–NEWBIE7, HARROW, GnomeDwarf, and connector markers current design or export annotations? | Retain every label and ask the map author to identify markers that carry editor-only meaning. |
| NBM-15 | Should the area title be treated as narrative content when no MOB, OBJ, QST, SHP, or story sidecar accompanies the WLD? | Keep this as a map-source review; add story interpretation only with owner-supplied context. |
| NBM-16 | Do any object, quest, or script numbers that fall in the candidate range actually refer to its rooms? | Preserve typed namespaces; confirm only structured room fields as dependencies. In particular, shipy.qst R C 666666 is a currency amount. |
| NBM-17 | Are there editor, deployment, database, or persisted player/object references outside this repository? | Locate those systems before a move, renumber, or archive proposal; they were not part of this static review. |
| NBM-18 | Should the candidate remain source-only while the header, room omissions, copy history, and Surface identities remain unresolved? | Retain both files in place and make any later activation, merge, or archive a separate owner decision. |

See the [NEWBIEMAPS source audit](NEWBIEMAPS_AUDIT_REVIEW.md) for the
room, graph, history, and dependency evidence behind these questions.

# Monsteri builder questions

These are source-backed prompts for the area, story, mobile, tooling, and
persisted-data owners. They record unresolved decisions and recommendations;
they do not imply that runtime or gameplay has been tested.

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| MON-01 | Is “CBL Guildhall Monster Island” the canonical title for Monsteri, and are there owner-confirmed aliases? | Keep the exact ZON title; record aliases only after an owner identifies their use. |
| MON-02 | Is the five-path WLD/MOB/OBJ/ZON/QST component intentionally absent from both areas/AREA and areas/qst/AREA? | Confirm intended load and quest-index status before changing either list. |
| MON-03 | What is the relationship among Monsteri, cbl_outpost2, and active cbl_hall’s #29820–#29824 Monster Island rooms? | Preserve the three source identities separately until the owners establish a canonical source or documented history. |
| MON-04 | Should all five Monsteri room descriptions remain identical to the active CBL Hall descriptions under different VNUMs? | Treat matching prose as duplicate-content evidence; do not infer a rename, replacement, or shared room identity. |
| MON-05 | Is the #1000–#1004 room band intentional when the ZON header is #1000 but the repository’s VNUM/100 convention places the rooms in band 10? | Keep the raw values and ask the map owner to settle the numbering before any renumbering. |
| MON-06 | Is Monsteri #1000’s “At the blood-stained gates” title meant to differ from active CBL #29820’s title without “At the”? | Preserve both title records until the owner confirms which wording is canonical. |
| MON-07 | Is the bloodied gatehouse, dead bodies, and four undead-beast description the intended arrival experience? | Keep the source prose and confirm content tone and intended player entry with the story owner. |
| MON-08 | Is the detailed torture-chamber imagery in #1001 appropriate for the intended map and audience? | Ask the content owner to confirm the tone before any separate prose revision. |
| MON-09 | Do the four golems in the #1000 reset group intentionally account for the four beasts described in the gatehouse? | Preserve the four M rows and their room placement until the builder confirms the relationship and reset intent. |
| MON-10 | Is #1002’s huge pit and #1003’s unstable spiral stairway intended as a complete downward exploration route? | Confirm the map’s intended endpoint, hazards, and return path; do not infer a fall from prose. |
| MON-11 | What does the #1004 Portal Cave’s “ticket off the island” text promise when its only WLD exit returns to #1003? | Ask the owner to identify the actual travel contract or clarify whether the line is unfinished environmental text. |
| MON-12 | Is active CBL Hall object #29710, reset at #29824 and targeting #29708, related to the Portal Cave prose copied into Monsteri? | Keep that active CBL item owned by cbl_hall unless its owner documents a shared source relationship. |
| MON-13 | Are the four local mobiles intended to remain together in gatehouse #1000? | Keep the source-only M placements and confirm intended formation and count fields with the builder. |
| MON-14 | Do the Monsteri golems correspond behaviorally to active CBL Hall mobiles #29820–#29823? | Compare complete prototype blocks and intended roles; similar descriptions do not establish shared mobile identity. |
| MON-15 | How should Monsteri mobiles #1000–#1003 be distinguished from active Heavens mobiles with the same VNUMs? | Preserve Heavens mobiles #1000–#1004 as separate prototypes, distinguish Monsteri room #1004 from Heavens mobile #1004 by role, and resolve the four mobile VNUM collisions before loading or renumbering. |
| MON-16 | Do the four same-number areas_mini/mini.mob peasants have any intended identity connection to Monsteri’s four golems? | Keep the mini-mode fixture separate unless its owner documents deliberate shared prototypes. |
| MON-17 | Are the matching four M reset rows in Monsteri and cbl_outpost2 intended to be duplicate placement programs? | Preserve both reset files until the source owners document their intended lineage and canonical placement set. |
| MON-18 | Are the zero-byte Monsteri.obj and Monsteri.qst files intentional records, or should either contain data? | Preserve the empty tracked paths while the owner confirms whether they are placeholders or deliberate empty files. |
| MON-19 | Should the environmental story remain without a native QST or same-stem SHP file? | Do not invent a quest or shop; ask whether the source is intentionally story-only. |
| MON-20 | What source history should be assigned to the zero-byte OBJ and QST files whose rename-follow paths pass through Players placeholder paths? | Record only owner-confirmed area lineage; empty-file similarity is not a rename or replacement decision. |
| MON-21 | Is there an external map-editor project or export for Monsteri even though no same-stem areas/map JSON is present? | Identify the editor and export owners before considering a path or archive proposal. |
| MON-22 | Should the four named mobile roles and the active CBL copy be treated as one encounter, separate implementations, or historical alternatives? | Ask the mobile/story owners to resolve lineage and behavior while keeping all current prototypes distinct. |
| MON-23 | Do any external scripts, tools, deployments, or persisted character references assign #1000–#1004 to Monsteri? | Complete owner/tool/data closure before a separate activation, ID, or path decision. CP920 found Menden ZON and shop references to object #1000, currently owned by Heavens; those object-role references do not identify Monsteri’s empty OBJ file. |
| MON-24 | Should Monsteri remain an attic/source-only candidate until the owner resolves its lineage and range? | Retain all files in place; make any later activation, archive, or move a separate owner decision. |

See the [Monsteri source audit](MONSTERI_AUDIT_REVIEW.md) for the graph,
content comparison, typed-reference findings, and source links behind these
questions.

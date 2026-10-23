# Nexus56 builder questions

These source-backed questions are for the map, area, mobile, item, shop,
reset, loader, tooling, and persisted-data owners. They record decisions and
recommended resolutions; they do not assert live loading or gameplay results.

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| NEXUS56-01 | Is “Nexus” the intended title and canonical identity for `nexus56`? | Keep the exact ZON title; document aliases only after the area owner confirms them. |
| NEXUS56-02 | How should `nexus56` relate to registered `nexus`, whose current title is “the Mountain of Peril Peaks”? | Keep the two source stems and titles separate until their owners establish provenance. |
| NEXUS56-03 | Is this 760-room city package intentionally absent from `areas/AREA` and `areas/qst/AREA`? | Confirm deployment and quest-loader intent before changing either manifest. |
| NEXUS56-04 | What is the intended room ownership for 666 Nexus56 VNUMs already owned by Alatorin or Delwyn with different titles? | Preserve both map records and settle the active room-owner table before loading or renumbering. |
| NEXUS56-05 | Are the 23 routes from Nexus56-only rooms to current Alatorin/Delwyn owners intended approaches? | Preserve each listed source tuple and confirm destination, return path, and owner with map maintainers. |
| NEXUS56-06 | Should the three one-way source directions at #83400, #83639, and #83804 have reverse exits? | Keep the exact source edges until the map editor confirms any repair. |
| NEXUS56-07 | Are the 66 gaps inside #83000–#83825 intentional? | Preserve the numbered map and request the author’s range/export explanation. |
| NEXUS56-08 | Should the 102 mobile VNUM overlaps with active Alatorin remain distinct prototypes? | Keep both prototypes and document intended ownership before any shared-number load. |
| NEXUS56-09 | How should the 168 object VNUM overlaps with active Alatorin and `alatorin_objects.obj` be resolved? | Preserve separate object identities; confirm which prototypes are intended for each map and reset. |
| NEXUS56-10 | Are reset mode 2 and all 1,181 D/M/F/R/O/P/G/E commands intended together? | Retain the authored mode and rows while the zone owner verifies their reset semantics. |
| NEXUS56-11 | Are follower and mount groups in the 76 F and 4 R rows intended to accompany their M placements? | Preserve the exact mobile, leader, and room references until the reset author reviews them. |
| NEXUS56-12 | What object sources should the five unresolved ZON IDs #2569, #17233, #17234, #17241, and #17243 identify? | Keep each typed reference; locate the intended object prototypes or ask the author to resolve the rows. |
| NEXUS56-13 | Are the 60 out-of-package ZON object imports intentionally sourced from their current active prototypes? | Preserve their source references and confirm ownership with the corresponding area maintainers. |
| NEXUS56-14 | Should the 19 SHP records, including #83032, be retained with their current 95 product rows? | Preserve every record. Confirm the 18 keeper VNUMs that lack active mobile prototypes, the #83184 Alatorin mobile collision, the 18 Alatorin-owned room-number overlaps, and the #83076 candidate-only shop location before any shop-loader or manifest change. |
| NEXUS56-15 | What is the intended product identity for unresolved #2569 in shop #83032 and the ZON G reset? | Preserve both references while the item owner resolves the missing object VNUM. |
| NEXUS56-16 | Should Nexus56 SHP participate in the conditional individual-file shop fallback? | Ask the loader and area owners to confirm the intended file set; source code alone does not establish operation. |
| NEXUS56-17 | Are the three portal-described objects #83161, #83219, and #83220 meant to teleport, given their source value rows contain no nonzero room destination? | Keep descriptions and values together until the object author confirms intended behavior. |
| NEXUS56-18 | Are active QST, ZON, SHP, and chaos-kit references in the same VNUM ranges meant to use Alatorin’s current prototypes? | Preserve the active owner mappings and document each typed consumer before any number reuse. |
| NEXUS56-19 | Do the followed 2009–2026 same-path histories capture the full map and prototype provenance? | Record the tracked Git evidence and request owner/editor evidence for untracked history. |
| NEXUS56-20 | Which editor project, persisted map, and deployment package owns these five files? | Identify editor and persisted references before any separate activation, merge, move, or archive decision. |
| NEXUS56-21 | Is the candidate chair #83148 intended as an ITEM_SWITCH for Nexus56 room #83638 when that VNUM currently belongs to Alatorin's differently titled mountain room and active Alatorin OBJ #83148 is a mithril sign? | Keep the chair and its value fields with the Nexus56 source package; confirm both room and object identity before loading, renumbering, or moving it. |

| NEXUS56-22 | Should source-only shop #83037 and the `nexus56.zon` G reset at line 1369 continue to import Marigot instrument #5319? | Preserve both exact references under RETAIN IN PLACE, keep Marigot as the prototype owner, and confirm shop-reader and reset intent before any separate activation or path decision. |

See the [Nexus56 source audit](NEXUS56_AUDIT_REVIEW.md) for the graph,
owner tables, typed references, shop inventory, and history evidence behind
these questions.

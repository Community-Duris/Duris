# The Obsidian Citadel: repository-wide source reconciliation

Checkpoint 111 reconciles the existing source-comprehensive Obsidian Citadel dossier against the current active and inactive world files, exact external reset consumers, title ownership, and tracked source history. The source checkout is `c500ac51b`; no native gameplay or area files are changed by this review.

## Active boundary and authored reachability

The active WLD census parsed 351 contributors and 253,262 active room VNUMs without parse issues. Obsidian's 179 rooms contain 382 exits: 379 local and three outbound; all 382 targets resolve. Its local exit graph is one weak component of 179 rooms, has no active room-VNUM overlap with another area, and the raw directed graph from #75600 reaches all 179 rooms when every authored exit is treated as passable. This open-exit result does not supersede the dossier's gate-qualified counts: 147/179 without gate items, 178 with the selected gate identities, and 179 after the fixed CACKLE shelf/orb edges. Neither calculation proves a played route, current stock, room safety, or safe return.

| Direction | Authored edge | Reverse edge | Source |
| --- | --- | --- | --- |
| Active in and out | Surface #579804 north to Obcita #75600 | Obcita #75600 south to Surface #579804 | [Surface source](../../../areas/wld/surface.wld#L1675232); [Obcita source](../../../areas/wld/obcita.wld#L10); [exit rows](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L873), [pair row](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L332) |
| Active in and out | Underdark #736889 west to Obcita #75778 | Obcita #75778 east to Underdark #736889 | [Underdark source](../../../areas/wld/underdark.wld#L52638); [Obcita source](../../../areas/wld/obcita.wld#L3088); [exit row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L874), [pair row](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L333) |
| Active in and out | Underdark #737288 north to Obcita #75778 | Obcita #75778 south to Underdark #737288 | [Underdark source](../../../areas/wld/underdark.wld#L53490); [Obcita source](../../../areas/wld/obcita.wld#L3092); [exit row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L875), [pair row](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L333) |

Together these are three active inbound exits, three outbound exits, and three reverse-paired records. The existing cross-area exit and pair registers already contain all six active rows: [inbound rows](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1516), [Underdark inbound rows](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1696), and the matching [pair summaries](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L568).

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits with zero parse errors and zero source-room VNUM overlap against Obcita. It found one non-overlapping source-only approach: inactive `surf.wld` #262357 north to Obcita #75600 ([source exit](../../../areas/wld/surf.wld#L1308501)). This does not create current ingress or alter the active 3-pair boundary. It raises the reviewed Surf subset from 135 to 136 and the curated source-only approach total from 1,523 to 1,524. Keep the raw source identity in the `surf` candidate record; preserve `surf` and `surface2011` as **RETAIN** pending map ownership and full dependency closure.

## Typed dependencies and code bindings

| Direction and type | Evidence | Classification |
| --- | --- | --- |
| Obcita reset imports | `obcita.zon` G rows name object #359 and #55188 ([lines 532-533](../../../areas/zon/obcita.zon#L532)); definitions are in active [Heavens](../../../areas/obj/heavens.obj#L3725) and [Winterhaven](../../../areas/obj/wh.obj#L2432) object sources. | Active reset imports, not local prototypes. Keep their source owners and object identities distinct. |
| Outside reset loads Obcita objects | Active `ravenloft.zon` has four P rows for #75638 at room #58339 and four for #75669 at room #58338 ([first group](../../../areas/zon/ravenloft.zon#L472), [second group](../../../areas/zon/ravenloft.zon#L478)). | Inbound reset references to local acid/cold scrolls, as already classified in the dossier. Source stock declarations do not prove an actual Ravenloft shelf instance or player acquisition. |
| Outside QST, SHP, or story-sidecar values | Exact typed scans found no external QST item command, no external SHP product, and no structured external sidecar VNUM. All 231 nonlocal story sidecars parsed; an exact numeric-token scan also found zero local room/mobile/object hits. | No match in the bounded source scan. This does not close editor, generated, indirect runtime, or persisted-player references. |
| Outside prototype or portal target | No external mobile/object definition duplicates a local prototype. None of 870 external type-25 object declarations targets a local Obcita room. | No exact outside prototype collision or dynamic type-25 arrival was found. The two local CACKLE portals remain the distinct local arrivals described in the dossier. |
| Local shop | No `obcita.shp` source is present. | No local shop inventory to treat as a current dependency. |

Direct C/C++ source-number matches are limited to bindings and guards already covered by the existing dossier: Rolart #75615 in [`epic_skills.c`](../../../src/classes/epic_skills.c#L208), shrine room #75610 in [`actoth.c`](../../../src/cmd/actoth.c#L7822), smith #75628's legacy choice table (including repeated menu value 67) in [`tradeskill.c`](../../../src/economy/tradeskill.c#L216), the commented Rolart assignment and active death-knight/Satar assignments in [`specs.assign.c`](../../../src/specs/specs.assign.c#L892), and the Satar summon's #75648 creation in [`specs.obsidian_citadel.c`](../../../src/specs/specs.obsidian_citadel.c#L33). Their actor eligibility, active-accounting refusal, stock lineage, and correctness questions remain those recorded in the [complete story dossier](../zone-stories/THE_OBSIDIAN_CITADEL.md).

## Title and followed history

The normalized active `.zon` title “The Obsidian Citadel” has one owner among all 351 current active titles ([title line](../../../areas/zon/obcita.zon#L2)). Lexical near-matches are the separate active titles “The Bronze Citadel” (`bctdl`) and “The Citadel” (`citadel`), each in a different `.zon` source ([Bronze](../../../areas/zon/bctdl.zon#L2), [Citadel](../../../areas/zon/citadel.zon#L2)); none is an exact title or source-stem match.

`git log --follow` for `obcita.wld`, `.zon`, `.mob`, `.obj`, and `.qst` follows the same source paths through the 2009 repository-wide `areas/` relocation and subsequent same-stem edits. The WLD history includes 2010-2013 map edits; the ZON history includes 2013 and the 2015 quest-item-flag update. The story sidecar was added on 2026-10-06. The followed paths show no alternate-stem rename, explicit predecessor, or replacement claim. The inactive `surf.wld` approach is a separate unregistered source record, not evidence of replacement.

## Builder question and disposition

**OBCITA-Q15 — Map ownership:** does the inactive `surf.wld` north edge into #75600 represent an intended legacy surface approach, or only retained map data? **Recommended handling:** preserve the edge as source-only, keep `surf` and `surface2011` **RETAIN**, and do not promise, activate, or repair this route without an explicit map-owner decision. The existing dossier's access follow-up now records the distinction; its twenty story/payment/access/summon/service follow-ups remain intact.

Obcita remains an active full-area contributor and **DO NOT MOVE**. The exact active map pairs remain in service; this review changes no gameplay, source areas, or world files. Indirect/tooling references, database or persisted-player state, current arrival conditions, actual traversal, hazards, and safe return remain outside this static source audit.

See the updated [Obcita coverage row](ZONE_COVERAGE_REGISTER.csv#L221), [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), [world-connectivity report](WORLD_CONNECTIVITY.md), [duplicate/replacement register](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan](ATTIC_MIGRATION_PLAN.md).

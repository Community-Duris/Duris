# Vargan: graph, dependency, history, and retention reconciliation

Checkpoint 104 · Priority 144 · active area [`vargan`](../../../areas/AREA#L9) · zone 21; title at [`vargan.zon`](../../../areas/zon/vargan.zon#L2).

This supplement adds repository-wide graph, inactive-map, typed-dependency,
source-history, and retention evidence to the [comprehensive Vargan
dossier](../zone-stories/VARGAN.md) and [source audit
index](../../reference/zone-story-audits/vargan.md). The dossier remains the
detailed account of the hidden pieces, armor exchange, environmental paths,
and builder questions. Static source records do not establish played
availability, ordinary access, or safe return.

## Active graph and reciprocal boundaries

The active WLD manifest contains 351 files. `vargan.wld` has 94 rooms and
239 exits: 236 local and three outbound. All 239 targets resolve, and the
local undirected projection is one weak component of 94 rooms. Each inbound
boundary has a reciprocal return:

| Inbound source | `vargan` return | Cross-area register |
| --- | --- | --- |
| [`surface.wld`](../../../areas/wld/surface.wld#L2202368) #604903 south → #2100 | [`vargan.wld`](../../../areas/wld/vargan.wld#L13) #2100 north → #604903 | [rows 35 and 1586](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L35) |
| [`surface.wld`](../../../areas/wld/surface.wld#L2210739) #605302 east → #2100 | [`vargan.wld`](../../../areas/wld/vargan.wld#L22) #2100 west → #605302 | [rows 36 and 1588](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L36) |
| [`v2.wld`](../../../areas/wld/v2.wld#L15) #7800 up → #2182 | [`vargan.wld`](../../../areas/wld/vargan.wld#L1887) #2182 down → #7800 | [rows 37 and 176](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L37) |

These six WLD records describe three reciprocal static boundaries. They do not
prove an ordinary player's access, reset or door state, environmental outcome,
or a safe return. The local river, crawl, wall, and no-ground qualifications
remain in the [story dossier](../zone-stories/VARGAN.md); broader outside-zone
reachability is still pending.

## Inactive WLD sweep and source-only approach

All 95 WLD files outside the active manifest parsed 527,791 rooms and
2,080,773 exits without parse issues or source-room VNUM overlap with active
rooms. The scan found one non-overlapping source-only approach into Vargan:

| Inactive source | Active target | Evidence |
| --- | --- | --- |
| [`surf.wld`](../../../areas/wld/surf.wld#L1391657) #266321 east → #2100 | [`vargan.wld`](../../../areas/wld/vargan.wld#L1) #2100 | Inactive source room #266321, exit at line 1391665, target tuple at line 1391668 |

This record is not active ingress and does not add an edge to the active graph.
It raises the reviewed `surf.wld` subset from 130 to 131 and the curated
source-only approach total from 1,517 to 1,518. Preserve it as source-only.
The `surf`/`surface2011` candidate remains **RETAIN**: active exits still
reference old Surface VNUMs, and map ownership, generation, and dependency
closure are incomplete. See the [`surf` retention plan](ATTIC_MIGRATION_PLAN.md#L27)
and [candidate dependency register](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv).

## Typed inbound and outbound dependencies

The local QST item commands and ZON reset roles stay within Vargan's local
prototypes/range. Vargan has no local SHP source. Across active and unregistered
sources, exact typed scans found no outside QST item command, shop product,
ZON reset-role target, or typed `item_vnums` sidecar reference into the local
prototypes. All 231 outside story sidecars parsed without error and yielded no
exact structured numeric target hits. No external mobile or object prototype
duplicates a Vargan VNUM. The external type-25 scan covered 872 declarations
and found no target into Vargan rooms.

Broad number matching surfaced non-dependencies that require type context.
Alatorin story-sidecar IDs contain digit sequences matching Vargan VNUMs, but
not typed item fields. Shady Grove's [`R E 2100`](../../../areas/qst/shady.qst#L41)
is an experience reward; its sidecar describes 2,100 experience for the golden
skeleton key, not a reference to Vargan's forge object #2100 ([story
record](../../../areas/story/shady.story.json#L174)). The local dossier's
context review of compiled numeric matches likewise identifies namespace
constants, comments, tables, and an unused legacy generator declaration, with
no selected Vargan quest binding or literal special assignment.

## Title, tracked history, and disposition

The normalized active `.zon` title “Vargan” has one owner among 351 current
non-empty active titles. Followed `vargan.wld` history starts at the 2009-08-12
repository-wide `areas/` move (`642da5400`) and includes same-path edits in
2011 (`81c009b4b`), 2013 (`71d731c5b`), the 2026-04-06 source refactor
(`0bae49b7c`), and the 2026-05-01 line-ending change (`428ebfddf`). Followed
`vargan.zon` history also records the 2012 zone-field update (`bb0d59425`),
2015 quest-item flags (`f18992b8f`), and 2016 minotaur-race correction
(`e6ad0de10`). The current story commit is `cab826923` (2026-10-06). No
alternate-stem rename, distinct predecessor, or explicit replacement was
found.

The active `v2` area is titled “Vargan II” and connects reciprocally to Vargan
at #2182/#7800. Its source refers to Vargan's children and disciples, but its
distinct active stem, zone, and room range do not establish that either area
replaces the other. Preserve both active identities.

Vargan remains **DO NOT MOVE** as an active area contributor. Preserve its
three reciprocal boundaries. Keep the `surf.wld` approach distinct from active
ingress and retain the Surface candidate until its owners and remaining
dependencies are resolved. No zone files were moved and no native gameplay or
area-source change ships with this supplement.

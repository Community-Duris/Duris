# Underworld2 source audit

Canonical key: `component:underworld2`. Coverage data row 466 / CSV line 467;
candidate dependency row 115 / CSV line 116. Reviewed October 12, 2026
(Checkpoint 553) against checkout `72ba3deff3bb30bde880018cdebe7c6db675b697`.
This is a static source, graph, typed-reference, identity, and followed-history
review. It does not establish live access, route safety, or runtime state.

## Source inventory and authored story

The only current same-stem source files are
[`underworld2.wld`](../../../areas/wld/underworld2.wld#L1) and
[`underworld2.zon`](../../../areas/zon/underworld2.zon#L1). The stem is absent
from [areas/AREA](../../../areas/AREA) and
[areas/qst/AREA](../../../areas/qst/AREA). No same-stem MOB, OBJ, QST, SHP,
map JSON, or story sidecar exists. The ZON header is #415402, titled
“Underdark of Duris,” with declared top VNUM #599999 and reset mode 0. It has
no reset commands. The highest authored room is #440657, leaving the declared
top range substantially above the source room set; do not infer that this is
an error or reserved content without the map owner.

The WLD defines 8,256 unique rooms from #415402 through #440657. Every room
long description is empty. Sixteen normalized room-title groups provide
environmental labels rather than prose: “Dried Lava of Lost Souls” (4,955),
“Dried Lava Mountains” (2,908), “Scorching Hot Lava of Demons” (275), and
“Bridge made of Bone and Flesh” (106), plus singleton labels “HOLY CLOISTER,”
“FORT BOYARD,” “KHULLOR,” “GITHZERAI STRONGHOLD,” “NAKRALS,” “sevenoaks,”
“PORTAL,” “PITS OF CEREBRUS,” “TEZCAT,” “DRAGONNIA,” “FORGOTTEN MANSION,”
and “TEMPLE OF EARTH.” The source evokes a dried-lava, lost-souls, demon, and
bone-and-flesh environment, but contains no authored room-description story,
characters, quest, or object narrative.

## Authored WLD graph and active-map boundaries

The WLD contains 32,603 exit records: 32,592 target another underworld2 room
and eleven target active WLD rooms. There are no negative targets, unresolved
positive target VNUMs, exit flags or key fields, fall-chance fields, or current
fields. All rooms and exit sources lie within the declared zone range.

The local graph is one weak component of all 8,256 rooms. Raw directed
traversal from #415402 reaches 8,245 rooms; reverse traversal reaches all
8,256. There are twelve strongly connected components: one of 8,245 rooms and
eleven singleton rooms. These eleven labeled rooms are not forward-reachable
from #415402, although each can reach back to it through authored directions:

| Singleton room | Source title |
| --- | --- |
| #417216 | HOLY CLOISTER |
| #417229 | FORT BOYARD |
| #421085 | KHULLOR |
| #421440 | GITHZERAI STRONGHOLD |
| #421744 | NAKRALS |
| #425644 | sevenoaks |
| #427710 | PITS OF CEREBRUS |
| #429494 | TEZCAT |
| #431339 | DRAGONNIA |
| #436466 | FORGOTTEN MANSION |
| #438228 | TEMPLE OF EARTH |

All eleven exits from underworld2 to active maps are source-only declarations:

| Source exit | Current active target | Evidence |
| --- | --- | --- |
| #417516 north (`D0`) | Cloister #67100 | [WLD exit](../../../areas/wld/underworld2.wld#L10349) |
| #417529 north (`D0`) | Fort Boyard #38100 | [WLD exit](../../../areas/wld/underworld2.wld#L10622) |
| #421086 west (`D3`) | Lab #82350 | [WLD exit](../../../areas/wld/underworld2.wld#L33424) |
| #421439 east (`D1`) | Githzerai Stronghold #44400 | [WLD exit](../../../areas/wld/underworld2.wld#L36668) |
| #422044 north (`D0`) | Nakral’s Crypt #14300 | [WLD exit](../../../areas/wld/underworld2.wld#L41114) |
| #425645 west (`D3`) | Sevenoaks #38600 | [WLD exit](../../../areas/wld/underworld2.wld#L68222) |
| #427709 east (`D1`) | Pits of Cerberus #22118 | [WLD exit](../../../areas/wld/underworld2.wld#L83417) |
| #429194 south (`D2`) | Tezcat #98900 | [WLD exit](../../../areas/wld/underworld2.wld#L94503) |
| #431340 west (`D3`) | Dragonnia #6803 | [WLD exit](../../../areas/wld/underworld2.wld#L111420) |
| #436465 east (`D1`) | Forgotten Mansion #3500 | [WLD exit](../../../areas/wld/underworld2.wld#L148802) |
| #437928 south (`D2`) | Temple of the Earth #43500 | [WLD exit](../../../areas/wld/underworld2.wld#L157534) |

Each destination VNUM resolves to a unique active WLD owner in the current
source set. None has a matching active reverse exit to the candidate source.
Because underworld2 is outside `areas/AREA`, these declarations do not create
active ingress or prove that any source-only route is usable. The #417529 Fort
Boyard exit was already present in the destination connectivity evidence; this
review corrects the candidate-specific tally to eleven without adding a
duplicate to the curated global total of 11,993 inactive-WLD approaches / 415
from Surf. See [WORLD_CONNECTIVITY.md](WORLD_CONNECTIVITY.md#checkpoint-553-underworld2-source-graph-and-boundaries).

## Inbound edges and typed dependency closure

The bounded scan of current active and unregistered WLD sources found no exit
into any of underworld2’s 8,256 room VNUMs. No other current WLD file shares
one of its room IDs. Thus candidate-specific active and inactive WLD inbound
counts are both zero; raw numeric coincidences, dynamic code, editor projects,
or deployed data are not established by this result.

The same-stem ZON contains no reset commands. Bounded typed-reference scans
found no external ZON room-target reset, QST room-VNUM reference, SHP numeric
reference, structured story JSON VNUM, type-25 OBJ portal target, or direct
candidate-specific room-VNUM integer binding in maintained C/C++ sources. The
candidate has no local MOB, OBJ, QST, or story-sidecar source to close. These
are repository-bounded checks; generic range-based code behavior, dynamic
routes, map-editor inputs, deployment, and persisted room identity remain
unqualified.

## Title, history, and disposition

After case and whitespace normalization, “Underdark of Duris” occurs once
across the current ZON title set. The underworld2 room VNUM set is disjoint from
all other current WLD sources. Followed WLD and ZON history reaches commit
`642da5400` (August 12, 2009), the area-tree relocation. The checked manifest
histories contain no underworld2 entry. No tracked predecessor, successor,
retirement, or replacement decision was found. The title, source-only routes,
and list exclusion do not establish supersession.

Keep underworld2 **RETAIN IN PLACE** outside both manifests. Preserve all
8,256 room identities and the eleven source-only active-map exits. Do not
activate, merge, regenerate, move, or archive it until the map owner confirms
the source/editor authority, sparse one-way topology, intended boundary
routes, title/range metadata, and any external runtime or persisted
dependencies. See the [30 builder questions](UNDERWORLD2_BUILDER_QUESTIONS.md).
No build, boot, live server, editor project, deployment, or database state was
inspected.

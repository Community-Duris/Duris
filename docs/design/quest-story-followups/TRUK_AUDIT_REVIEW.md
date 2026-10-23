# Truktakyagyo: repository-wide audit supplement

This supplement records the active graph, typed dependency closure, title and
history review, and retention boundary for the [Truktakyagyo source dossier](../zone-stories/TRUK.md).
The review is documentation-only. No area, gameplay, database, or runtime
change is made.

## Active graph and boundaries

The active Truk WLD has 56 rooms and 115/115 resolving authored exits. The
local graph is one weak component of all 56 rooms, with 112 local exits and
three outbound cross-area records. There are three inbound records and three
reverse-paired boundaries:

| Truk boundary | Reciprocal source evidence |
| --- | --- |
| #1600 east → Underdark #846820 | [Truk room](../../../areas/wld/truk.wld#L1) · [cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L24) · [Underdark return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1780) |
| #1622 north → Traoul's Lair #7000 | [Truk room](../../../areas/wld/truk.wld#L435) · [cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L25) · [Traoul return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L130) |
| #1655 south → Goblin Outpost #70198 | [Truk room](../../../areas/wld/truk.wld#L1032) · [cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L26) · [outpost return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L845) |

The zone has no authored fall-chance or current fields. There is no room
overlap. One non-overlapping source-only inactive-WLD approach is present:
ud_map2.wld #1126563 north to Truk #1600 ([source](../../../areas/wld/ud_map2.wld#L80799);
[ud_map2 audit](UD_MAP2_AUDIT_REVIEW.md#authored-wld-graph-and-boundaries)).
It is not active ingress while ud_map2 remains outside areas/AREA. These counts
are static directions and do not qualify door state, key use, current stock,
route admission, or safety.

## Typed references, special binding, and bounded negatives

| Finding | Source-backed boundary |
| --- | --- |
| Global quest service | Local mobile #1603 is reset in the tavern and receives the compiled world_quest binding. The special dispatches the shared world-quest service; it is not a local QST entry or zone-authored quest receipt. [bartender reset](../../../areas/zon/truk.zon#L104) · [assignment](../../../src/specs/specs.assign.c#L775) · [implementation](../../../src/specs/specs.world_quest.c#L191) |
| QST index/file mismatch | areas/qst/AREA lists truk at zone 16, but areas/qst/truk.qst is absent. This is an inventory discrepancy, not proof of local QST content. [QST AREA entry](../../../areas/qst/AREA#L5) |
| Dormant reset reference | Unregistered brass-old-1.zon has a G reset for object #1604 at [line 545](../../../areas/zon/brass-old-1.zon#L545), although its local object file does not define #1604. The only current definition of object #1604 is Truk's stone mug. brass-old-1 is not an active areas/AREA contributor; this is a source-only dependency candidate, not an active supplier. [Truk prototype](../../../areas/obj/truk.obj#L39) · [dormant object file](../../../areas/obj/brass-old-1.obj) |
| Bounded reference scan | No outside exact QST item consumer, active SHP stock reference, duplicate mobile/object prototype, or active type-25 teleport target into #1600–#1655 was found. The local ZON resets use local prototypes. Other indirect/editor/player-data dependencies were not included in this closure. |

The dormant reset reference is not included in the active connectivity counts
and does not change the curated source-only WLD totals. The local source set
has no SHP or story sidecar.

## Title, history, and retention

After color-code removal and case/whitespace normalization, “Truktakyagyo” has
one exact owner among the 351 active ZON titles: area truk, zone 16
([title source](../../../areas/zon/truk.zon#L2)). Followed WLD, MOB, OBJ, and
ZON histories stay on the same stem through the 2009 area-directory move and
same-stem changes from 2012 through 2015. No distinct predecessor, alternate
stem, or replacement claim was found in those paths. There is no local story
sidecar history to follow. This is a bounded title/path finding; it is not
proof that every thematic similarity in the world was compared.

Keep active truk **DO NOT MOVE**. Preserve the world_quest binding, the QST
index/file discrepancy, the three reciprocal active boundaries, and the
dormant brass-old-1 reset reference as separate source records.

## Checkpoint 403 builder-question coverage

The [Truktakyagyo dossier](../zone-stories/TRUK.md#explicit-builder-questions-and-recommended-resolutions)
contains exactly twenty direct-source builder questions and conservative
recommendations. Static review does not qualify the global quest service,
payment outcomes, gateway behavior, or actual cross-zone travel.

## Checkpoint 858: Current boundary and source-only approach carried forward

The reviewed `areas/` and `src/` content remains identical to pinned source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`; this checkpoint carries the Checkpoint 403 source findings and builder questions forward without claiming a second independent parse. Truktakyagyo remains 56 rooms with 115/115 resolving exits (112 local, three outbound) in one weak component. Its three active reciprocal boundaries remain #1600 / Underdark #846820, #1622 / Traoul #7000, and #1655 / Goblin Outpost #70198. The inactive ud_map2.wld approach #1126563 north to #1600 remains source-only on existing candidate row 113 and is already included in current cumulative totals of 11,993 overall / 415 Surf. The Checkpoint 403 global total of 9,480 / 298 is historical.

The typed closure remains unchanged: bartender #1603 retains the compiled `world_quest` binding; `qst/AREA` still lists `truk` without a local QST file; and unregistered `brass-old-1.zon` retains a G reset for locally defined OBJ #1604, though its own object file lacks that prototype. The dormant reset remains source-only and is not an active supplier. No outside exact QST item consumer, active SHP stock reference, duplicate mobile/object prototype, or type-25 teleport target into Truk was established. The twenty builder questions from Checkpoint 403 remain source review, not gameplay qualification.

Keep active `truk` **DO NOT MOVE**. Preserve the three reciprocal active boundaries, global service binding, QST index/file discrepancy, and dormant Brass reset as separate source identities. Retain the ud_map2 approach on existing candidate row 113; it does not establish active ingress. Add no Truk candidate, activate no route, and grant no migration clearance. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L316), [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-858-truktakyagyo-boundaries-and-ud_map2-approach-remain-unchanged), and [ud_map2 candidate row 113](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114).

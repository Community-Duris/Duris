# Nexus_1 source audit

Canonical key: `component:Nexus_1`. Physical coverage row 430 / CSV line 431;
candidate dependency row 79 / CSV line 80. Reviewed October 11, 2026,
Checkpoint 517, against source snapshot
`c500ac51b8cd26540943257e7ca4e9807263ef74`. This is a static narrative,
connectivity, identity, reset, dependency, and bounded-history review. It does
not establish live area loading, reset outcomes, editor ownership, or safe
migration.

## Package, membership, and authored setting

The source package consists of [Nexus_1.wld](../../../areas/wld/Nexus_1.wld#L1),
[Nexus_1.mob](../../../areas/mob/Nexus_1.mob),
[Nexus_1.obj](../../../areas/obj/Nexus_1.obj), and
[Nexus_1.zon](../../../areas/zon/Nexus_1.zon#L1). The MOB and OBJ files are
zero bytes. No same-stem SHP, QST, story sidecar, or map-coordinate file was
found. The normalized ZON title `Nexus_1` is unique in the current title scan.
The source is not listed in [areas/AREA](../../../areas/AREA) or
[areas/qst/AREA](../../../areas/qst/AREA), and `areas/RANDOM_AREA` is empty.
The registered `nexus` stem is a separate area with zone annotation
`*575-580`; that manifest row does not register this package.

The ZON header is zone #1, title `Nexus_1`, last VNUM #40, reset mode 2. The
WLD contains 40 rooms numbered #1–40. Its descriptions place the reader inside
the Black Onyx Fortress: an archway, halls and corridors, a tower base and
shafts, the Altar Room of Ralimeist (#38), and a runic alcove (#40). Six room
titles are `Unnamed`; #16 has a blank title. These landmarks are authored
setting evidence, but the empty MOB/OBJ files and missing QST/SHP/sidecar leave
no authored actor, item, quest, or shop context in this package to establish
the map's intended story or use.

## Candidate graph and active VNUM owners

The WLD has 92 directional exits. Every exit target is one of the 40 candidate
rooms; treating the records as an undirected graph yields one weak component
of all 40 rooms, and all 92 directions have an opposite-direction tuple. No
fall-chance (`F`) or current (`C`) directives were found. These are source
graph properties, not proof of runtime traversal.

Current active room definitions own 35 of the candidate's 40 numeric IDs:
room #1 belongs to Limbo, and #3–5 plus #10–40 belong to Heavens. None of
those 35 candidate room titles or complete room blocks exactly matches its
current active owner. The five candidate-only IDs, with no active WLD owner,
are #2 and #6–9. Room #1 is also the `ROOM_LIMBO_VNUM` constant in
[`utils.h`](../../../src/core/utils.h#L637). The 35 overlaps therefore cannot
be treated as evidence that this WLD is loaded under those IDs.

The five candidate-only rooms have ten outgoing directions. Six are
source-only approaches to existing active numeric owners; the four remaining
directions stay within the candidate-only subset.

| Candidate-only source direction | Current target owner | WLD line |
| --- | --- | ---: |
| #2 south to #1 | Limbo | [14](../../../areas/wld/Nexus_1.wld#L14) |
| #2 northwest to #4 | Heavens | [18](../../../areas/wld/Nexus_1.wld#L18) |
| #2 southwest to #3 | Heavens | [22](../../../areas/wld/Nexus_1.wld#L22) |
| #6 west to #5 | Heavens | [86](../../../areas/wld/Nexus_1.wld#L86) |
| #9 west to #3 | Heavens | [117](../../../areas/wld/Nexus_1.wld#L117) |
| #9 southeast to #10 | Heavens | [121](../../../areas/wld/Nexus_1.wld#L121) |

No new increment is taken for these six in the world-wide inactive-source
approach tally pending route-level duplicate reconciliation. Their target
numbers currently resolve to Limbo/Heavens owners, while the source records
belong to this separate, unlisted WLD.

Active WLD files contain 63 raw target records into #1–40: 53 from Heavens,
nine from Desert, and one from Thetis. All 63 target numbers are currently
owned by Limbo or Heavens; none targets #2 or #6–9. Candidate-specific active
WLD inbounds remain zero after disambiguating numeric owners. This is an
owner-aware count; a raw numeric match alone is not a route into Nexus_1.

## Same-number map identities

The candidate shares 26 room IDs with
[`areas_mini/mini.wld`](../../../areas_mini/mini.wld). There are no matching
titles; the only shared direction tuples are #19 east to #20 and #20 west to
#19. The shared IDs and two tuples do not make the room records identical.

Twelve other `areas/wld` files also define at least one Nexus_1 room number:
the active `heavens` and `limbo` maps plus ten other WLD sources—`astral`,
`dalvik`, `hostel`, `Magetower`, `mavulsk`, `Mentiri`, `mtncastl`,
`theurgist`, `ud-inside`, and `waterplane`. Their overlapping candidate-ID
records have no exact title or complete-block match. Seven of those
unregistered sources (`dalvik`, `hostel`, `Magetower`, `mavulsk`, `Mentiri`,
`theurgist`, and `waterplane`) have 78 exit records targeting the candidate's
otherwise unowned IDs #2 and #6–9; each such target also exists in its own
source WLD. Those are local numeric loops in separate files, not evidence of
Nexus_1 inbound routes. The remaining three named sources overlap #1 only.

The collision is a low-VNUM identity conflict, including the globally
reserved Limbo number, rather than evidence of a successor or exact copy. No
replacement decision follows from numeric overlap, title similarity, or the
two mini-map tuples.

## Reset and typed dependency evidence

[`Nexus_1.zon`](../../../areas/zon/Nexus_1.zon#L1) has exactly eight reset
commands, all `D` door-state commands; it has no M/F/R/O/P/G/E commands. The
eight `D` rows form four local pairs:

| Local door pair | State values | Source lines | Note |
| --- | --- | --- | --- |
| #3 direction 6 ↔ #5 direction 9 | 5 / 5 | [8](../../../areas/zon/Nexus_1.zon#L8), [12](../../../areas/zon/Nexus_1.zon#L12) | Both comments say closed/secret. |
| #17 direction 6 ↔ #40 direction 9 | 5 / 1 | [16](../../../areas/zon/Nexus_1.zon#L16), [20](../../../areas/zon/Nexus_1.zon#L20) | The comments say northwest closed/secret and southeast closed; the states are asymmetric. |
| #20 direction 1 ↔ #39 direction 3 | 5 / 5 | [24](../../../areas/zon/Nexus_1.zon#L24), [28](../../../areas/zon/Nexus_1.zon#L28) | Both comments say closed/secret. |
| #32 direction 0 ↔ #35 direction 2 | 5 / 5 | [32](../../../areas/zon/Nexus_1.zon#L32), [36](../../../areas/zon/Nexus_1.zon#L36) | Both comments say closed/secret. |

All door room/direction pairs match local WLD exits. The #17/#40 difference
needs builder confirmation; it is not silently normalized here. Reset mode
and commands describe source instructions, not proof that this ZON is
loaded or that the doors take these states at runtime.

Current active ZON sources have 35 M/O room-placement references and 27 D
door-state references into #1–40. They point only at the already-owned Limbo
and Heavens IDs, not the candidate-only #2 and #6–9. Active item teleport
definitions have no destination value into #2/#6–9. No direct `Nexus_1`
source reference was found in the scanned source tree. These typed matches
are numeric owner collisions, not proof of import or runtime dependency.

## Bounded history and review disposition

Git history shows the WLD path move in 2009; the ZON has that move plus a
2015 zone-overhaul edit; MOB/OBJ histories reach the initial 2005 revision,
the 2006 directory change, and the 2009 move. The bounded same-path review
found no later replacement, successor, or retirement decision. Absence of a
tracked decision does not identify the author, editor project, or persisted
reference owner.

**Disposition: RETAIN IN PLACE.** Preserve the four files and their original
low-VNUM identity. Do not activate, renumber, merge, move, or archive this
map based on the current static evidence. First resolve the reserved #1
collision, the five unowned candidate IDs, active-owner reset/import context,
the six source-only approaches, mini-map relationship, asymmetric door state,
builder/editor ownership, and persisted references. No migration clearance
is implied. See the [20 builder questions](NEXUS_1_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md),
[identity/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md),
[candidate dependency row 79](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L80),
and [coverage row 430](ZONE_COVERAGE_REGISTER.csv#L431).

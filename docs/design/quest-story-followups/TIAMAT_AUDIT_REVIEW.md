# Tiamat: connectivity, history, and dependency reconciliation

Checkpoint 110 reconciles the existing source-comprehensive Tiamat dossier with the full active-world graph, inactive-WLD scan, followed source history, and typed references. The dossier remains the detailed account of eight trades, key progression, boss and slave leads, services, and action-level builder questions.

## Active graph and routes

The full area list contains `tiamat` at [areas/AREA line 99](../../../areas/AREA#L99); its [zone header](../../../areas/zon/tiamat.zon#L1) is 196. The WLD contains 39 rooms and 116/116 resolving exits: 109 local and seven outbound. Its local weak components contain 25, 10, 1, 1, 1, and 1 rooms. Starting from #19600 and following authored local exits with ordinary gates treated as traversable reaches 23/39 rooms; the other sixteen, including the peace alcove and load/wander rooms, are outside this static route model. It does not simulate NPC movement, reset stock, the external pool, disabled procedures, or safe return.

Two active WLD exits enter Tiamat #19600: Avernus #32031 north and Hell One of Avernus #51100 east. The Avernus edge reciprocates with Tiamat #19600 south; the Hell One edge has no reverse. Tiamat has seven outbound exits: the Avernus return and six one-way exits to Astral Tiamat #19953 from #19620, #19624–#19627, and #19638. The latter rooms are vault or NPC-load routes in the current source; their directions do not establish a safe player return. Exact records are in the [Tiamat-to-Avernus exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L312), [six Tiamat-to-Astral exits](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L313), [Avernus inbound exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L438), and [Hell One inbound exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L639), with pair summaries for [Astral Tiamat](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L32), [Avernus](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L34), and [Hell One](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L187).

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without parse issues or source-room VNUM overlap. It found no inactive-WLD approach into Tiamat and no active-room VNUM overlap with another active area. Tiamat has no source-only WLD approach to add to the curated attic-edge total.

Astral Tiamat object #19907 is type 25, names Tiamat #19600 as its target, and is reset at #19902 and #19949. No `magic_pool` or other object callback assignment for #19907 was found; its target field and type do not establish a working transport. Treat it as an unbound type-25 prototype until the builder confirms its intended behavior ([pool object](../../../areas/obj/astral_tiamat.obj#L108), [first reset placement](../../../areas/zon/astral_tiamat.zon#L57), [second reset placement](../../../areas/zon/astral_tiamat.zon#L62), [callback assignments](../../../src/specs/specs.assign.c#L1894)).

## Typed inbound and outbound references

The local reset file names nine external object prototypes: #66, #360, #25106, #80831, #26661, #51004, #51006, #32019, and #911; it also imports mobile #19700. Eight of those objects and the mobile have active definitions. Object #25106 is defined only in unregistered `brass-old-3.obj` as a level-52 potion. The active Tiamat ZON's O command for it targets room #19632, but `renum_zone_table` changes an unresolved object reset command to `!`; this is not current stock. Preserve the reference and candidate until a builder decides whether to remove or replace it; do not reactivate the old source to satisfy the stale command ([Tiamat reset](../../../areas/zon/tiamat.zon#L92), [archived object](../../../areas/obj/brass-old-3.obj#L86), [loader resolution](../../../src/world/db.c#L1612)).

One active outside ZON also references a Tiamat item: `astral_tiamat.zon` O-loads local object #19607, the green-wyrm barb, into room #19953 at 25%. This matches the outside vault lineage in the dossier, but its source/placement must remain distinct from a local kill or accepted crafting history ([outside reset](../../../areas/zon/astral_tiamat.zon#L75), [local item](../../../areas/obj/tiamat.obj#L57)).

Bounded source scans found no external exact QST item command, SHP product, story-sidecar typed VNUM, or duplicate MOB/OBJ prototype definition for Tiamat-owned local prototypes. All 231 external story sidecars parsed without an exact numeric-token hit. Of 872 external type-25 declarations, one targets Tiamat: the Astral Tiamat pool above. The local Tiamat ZON has no SHP file.

The direct code scan's exact matches include #19617 binding `tiamat_human_to_rareloads`, which creates one of local Tiamat objects #19637/#19638 or Astral Tiamat objects #19911/#19916. Objects #19911 (Volo's boots) and #19916 (Isha's cloak) are prototypes owned by `astral_tiamat`; they are callback outputs, not Tiamat-owned local drops. Object #19916 also binds `artifact_hide`, and #19911/#19916 appear in Chaos equipment kits; Astral Tiamat mobile #19901 also appears in the Demogorgon helper list. Winterhaven consumes #19916 in its Volo QST/story exchange, while Heavens consumes Astral Tiamat helmet #19996. Object #19996 has no active reset or C++ creation reference found. Other matches include #19600 binding `block_dir` and #19638 binding the shield proc and appearing in standard and enhanceable psionicist kits ([mobile bindings](../../../src/specs/specs.assign.c#L280), [blocker binding](../../../src/specs/specs.assign.c#L1037), [shield binding](../../../src/specs/specs.assign.c#L2169), [standard kit row](../../../src/account/chaos_eq_data.h#L90), [enhanceable kit row](../../../src/account/chaos_eq_data.h#L342), [rare-load callback](../../../src/specs/specs.winterhaven.c#L4626), [Winterhaven consumer](../../../areas/qst/wh.qst#L281), [Heavens consumer](../../../areas/qst/heavens.qst#L148)). The `VROOM_TIAMAT_HOME` constant also uses numeric #19617 in a distinct room namespace ([room constant](../../../src/world/vnum.room.h#L14)). A dormant `TiamatThrone` implementation tries to move west to absent room #19623, and its assignment remains commented out; this does not add a live exit ([dormant assignment](../../../src/specs/specs.assign.c#L2288), [procedure](../../../src/specs/specs.room.c#L523), [missing destination](../../../src/specs/specs.room.c#L538)).

## Title, history, and disposition

After color-code removal, “Tiamat” has one exact owner among 351 current non-empty active ZON titles. `astral_tiamat` (“Astral Plane, Tiamat”) and `azhural` (“The Lair of Tiamat”) are separate active identities, not alternate names for this source. Followed `tiamat.wld` and `tiamat.zon` histories include the 2009 area-path move, 2009 route/alcove changes, later same-stem loot and teleport edits, and the 2015 quest-item-flag update. The Tiamat story sidecar was added on 2026-10-06. The followed paths show no explicit predecessor, alternate-stem rename, or replacement claim; no such relationship is inferred from the similar names.

Tiamat remains **DO NOT MOVE** as an active area. Keep `brass-old-3` **RETAIN** pending its typed-reference and owner review. The currently disabled #25106 reset is a source dependency, not active potion placement. No native repair, source move, or gameplay qualification is part of this checkpoint.

## Source references

- [Tiamat WLD](../../../areas/wld/tiamat.wld#L1), [Avernus inbound room](../../../areas/wld/avernus.wld#L421), and [Hell One of Avernus inbound room](../../../areas/wld/hell_one_avernus.wld#L2676).
- [Active boundary register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L312), [active pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L32), [coverage row](ZONE_COVERAGE_REGISTER.csv), and [existing source-comprehensive story dossier](../zone-stories/TIAMAT.md).
- [Connectivity report](WORLD_CONNECTIVITY.md), [history register](DUPLICATE_AND_REPLACED_ZONES.md), [candidate dependency register](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv), and [attic migration plan](ATTIC_MIGRATION_PLAN.md).

No native gameplay, test, build, database, migration, or operational input was changed or run for this documentation checkpoint.

## Checkpoint 389: builder-question coverage reconciliation

The source-comprehensive Tiamat dossier contains twenty numbered findings.
Checkpoint 389 pairs each with an explicit builder question and a recommended
resolution in
[ZSQ-TIAMAT-01 through ZSQ-TIAMAT-20](../zone-stories/TIAMAT.md#L54).
The rows cover the eight native exchanges, addressed dialogue and aliases,
key and gate progression, exact fragment identity, boss and slave drops,
services, timed items, missing or disabled source references, reset
availability, and active cross-zone boundaries. Each finding is paired with
a conservative next step.

I reconciled that table with the current eight-story sidecar, the selected
QST source, and this audit's graph, history, and typed-dependency evidence.
The coverage cell for physical row 302 now records the existing question set
as complete. This confirms authored source coverage; it does not establish
runtime access, repeatable stock, safe return, or unverified player outcomes.
Tiamat remains DO NOT MOVE. Keep `brass-old-3` separately RETAIN pending the
unresolved #25106 reset's owner decision; no new candidate or migration is
added. See [coverage row 302](ZONE_COVERAGE_REGISTER.csv#L302),
[connectivity](WORLD_CONNECTIVITY.md), [history](DUPLICATE_AND_REPLACED_ZONES.md),
and [attic disposition](ATTIC_MIGRATION_PLAN.md).

## Checkpoint 844: Astral Main import and typed roles refreshed

The current active Tiamat reset still imports Astral Main MOB #19700 at room
#19617, and the Tiamat sidecar still names that same outside mobile as a
contact. Its prototype remains in `astral_main.mob`; the maintained assignment
also gives it the `tiamat` procedure. Preserve the contact, reset placement,
prototype owner, and code binding as distinct source roles.
([reset](../../../areas/zon/tiamat.zon#L135),
[prototype](../../../areas/mob/astral_main.mob#L1),
[sidecar contact](../../../areas/story/tiamat.story.json#L53),
[procedure assignment](../../../src/specs/specs.assign.c#L1031),
[Astral Main cross-area audit](ASTRAL_MAIN_AUDIT_REVIEW.md))

The reciprocal typed review remains unchanged: Astral Tiamat resets Tiamat OBJ
#19607 in vault room #19953 at 25%, while Astral Tiamat OBJ #19907 names
Tiamat room #19600 as its type-25 target and has two Astral Tiamat reset
placements. No maintained object callback assignment for #19907 was found;
keep the target classified as declared data rather than a working portal. The
older Tiamat reset of object #25106 still names an object present only in
unregistered `brass-old-3.obj`; the loader disables that unresolved object
command, so it is not active potion stock. Retain `brass-old-3` on candidate
row 19 until its owner resolves this reference.
([Astral reset of Tiamat OBJ #19607](../../../areas/zon/astral_tiamat.zon#L75),
[Tiamat OBJ](../../../areas/obj/tiamat.obj#L57),
[pool prototype](../../../areas/obj/astral_tiamat.obj#L108),
[pool placements](../../../areas/zon/astral_tiamat.zon#L57),
[stale reset](../../../areas/zon/tiamat.zon#L92),
[retired-owner prototype](../../../areas/obj/brass-old-3.obj#L86),
[loader resolution](../../../src/world/db.c#L1612),
[brass-old-3 candidate row 19](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L19))

No new active WLD edge, external quest consumer, shop product, or source-only
approach is added. Keep Tiamat **DO NOT MOVE**, preserve the two existing
inbounds and seven outbounds, and keep Astral Tiamat and candidate
`brass-old-3` as distinct owners.

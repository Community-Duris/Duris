# The Twin Towers: repository-wide connectivity and dependency audit

**Checkpoint 78; priority 118 of the original 220-zone queue.** This supplement
reconciles the [source-comprehensive Twin Towers dossier](../zone-stories/TWIN_TOWERS.md)
with the active WLD graph, inactive WLD inputs, typed references, compiled
numeric matches, normalized title, and bounded source history. Authored edges
and dependencies do not prove ordinary reachability, service availability,
safety, or played qualification.

## Active WLD graph

The active manifest contains 351 WLD contributors. Twin Towers has 99 rooms and
211 exits: 207 internal exits and four cross-area exits. All 211 targets
resolve, and all local rooms form one weak component. Each boundary exit has a
reciprocal active return, for four pairs across the Twin Towers Forest and
Underworld 3.

| Twin Towers edge | Reciprocal edge | Cross-area register rows |
| --- | --- | --- |
| #13200 south to Forest #13570 ([exit](../../../areas/wld/ttowers.wld#L16)) | Forest #13570 north to #13200 ([exit](../../../areas/wld/twin_towers_forest.wld#L1816)) | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L224), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L232) |
| #13230 north to Forest #13571 ([exit](../../../areas/wld/ttowers.wld#L682)) | Forest #13571 south to #13230 ([exit](../../../areas/wld/twin_towers_forest.wld#L1856)) | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L225), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L233) |
| #13259 east to Forest #13569 ([exit](../../../areas/wld/ttowers.wld#L1386)) | Forest #13569 west to #13259 ([exit](../../../areas/wld/twin_towers_forest.wld#L1801)) | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L226), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L231) |
| #13297 west to Underworld 3 #30167 ([exit](../../../areas/wld/ttowers.wld#L2106)) | Underworld 3 #30167 east to #13297 ([exit](../../../areas/wld/underworld3.wld#L3441)) | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L227), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L419) |

There are no unresolved active Twin Towers exits. All 95 inactive WLD files
also parse without issues; their 2,080,773 exits contain no target into a Twin
Towers room and no source-room VNUM overlaps this area. Checkpoint 78 added no source-only inbound approach; its cumulative snapshot was 680 overall / 110 Surf. Those counts are historical; the current source-only total is 11,993 overall / 415 Surf.

## Typed references and compiled numeric matches

The external QST scan found four item commands that accept Twin Towers hearts.
These remain owned by the foreign recipients already mapped in the dossier.

| External source | Local item and role |
| --- | --- |
| [`alatorin.qst`](../../../areas/qst/alatorin.qst#L5532), `G I 13221`; [`alatorin.qst`](../../../areas/qst/alatorin.qst#L5533), `G I 13223` | Romgar's Q5526 accepts Mixt's and Lyena's hearts together. |
| [`newhaven.qst`](../../../areas/qst/newhaven.qst#L341), `G I 13221` | Vulgaris's Q331 accepts Mixt's heart with the Newhaven collar and fee. |
| [`wh.qst`](../../../areas/qst/wh.qst#L920), `G I 13223` | Enfil's Q916 accepts Lyena's heart with two other foreign items. |

The 231 other story sidecars parsed without errors and contain four exact
structured item references: Alatorin's sidecar names both hearts, Newhaven's
names Mixt's heart, and Werthorm's names Lyena's heart
([Alatorin](../../../areas/story/alatorin.story.json#L1710),
[Newhaven](../../../areas/story/newhaven.story.json#L478),
[Werthorm](../../../areas/story/wh.story.json#L902)). No external SHP product,
ZON reset-role reference, or MOB/OBJ prototype definition targets a local
Twin Towers prototype.

The portal scan found no foreign type-25 object destination into a Twin Towers
room. It covered 713 active external declarations and another 159 declarations
from source-only object files, 872 total. A broad C/C++ numeric scan found two
contextual references: [`epic_skills.c`](../../../src/classes/epic_skills.c#L186)
binds mobile #13219 to the computed Jin Touch teacher table, as the dossier
describes; [`actoth.c`](../../../src/cmd/actoth.c#L7818) checks room #13272 as a
conditional Paladin/Avenger ascension location. These are code bindings, not
additional WLD edges or foreign prototype definitions.

## Title, history, and disposition

The normalized title from [`ttowers.zon`](../../../areas/zon/ttowers.zon#L2) is
unique among the 351 active contributors. The global active-WLD duplicate-room
scan found only #51800 in Hell Two and Hell Three; Twin Towers has no duplicate
room VNUM. Bounded WLD and ZON `git log --follow` histories stay on the
`ttowers` paths back to the 2009-08-12 area-tree move. The latest tracked WLD
change is 2013-11-13 and the latest tracked ZON change is 2015-10-13. No
separate predecessor or replacement appears in those histories or the curated
duplicate/replacement register.

Twin Towers remains a current, un-commented [`areas/AREA` contributor](../../../areas/AREA#L74)
and is **DO NOT MOVE**. The existing dossier covers source-level service,
access, reset, foreign-receipt, and computed-teacher details. Dynamic travel,
door and reset state, safety, actual service/accounting outcomes, editor/tooling
closure, persisted references, and played qualification remain separate.

This checkpoint used repository sources and generated documentation only. No
tests, build, server, gameplay session, database operation, migration, or
operational script was run.

## Checkpoint 404: explicit builder questions

Physical coverage row 317 now has exactly 20 direct-source builder questions
with conservative recommendations in the [Twin Towers dossier](../zone-stories/TWIN_TOWERS.md#explicit-builder-questions-and-recommended-resolutions).
They preserve the seven native Q endpoints and distinguish the priest's
equivalent heart alternatives, Talfyn's chance-limited three-heart exchange,
foreign heart consumers, shared access controls, the rubble reset lifecycle,
the foyer fall, captured duergar, trapped chest, and computed Jin Touch
service. Static source review does not qualify these dynamic outcomes in play.
No area or gameplay source changed.

## Checkpoint 859: Current map boundaries and typed closure carried forward

The reviewed `areas/` and `src/` content remains identical to pinned source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`; this checkpoint carries the Checkpoint 78 source audit and Checkpoint 404 builder recommendations forward without claiming a second independent parse. Twin Towers remains 99 rooms with 211/211 resolving exits, 207 local and four outbound, in one weak component. The four active boundaries remain reciprocal: three with Twin Towers Forest (#13200/#13570, #13230/#13571, #13259/#13569) and one with Underworld 3 (#13297/#30167). The all-95 inactive scan found no source-room overlap or source-only approach; current cumulative totals remain 11,993 overall / 415 Surf. The earlier Checkpoint 78 and 404 totals (680/110 and 9,480/298) are historical.

The recorded typed closure remains unchanged: active Alatorin, Newhaven, and Winterhaven QSTs consume the authored hearts under their own contracts; four exact item-VNUM references remain across three external sidecars. No external shop product, reset role, duplicate mobile/object prototype, or type-25 target into Twin Towers was established. Compiled references remain computed Jin Touch teacher selection for mobile #13219 and a Paladin/Avenger ascension-room check at #13272. These links remain separate from the four reciprocal WLD boundaries. The twenty builder questions from Checkpoint 404 remain source review, not gameplay qualification.

Keep active `ttowers` **DO NOT MOVE**. Preserve the four reciprocal boundaries, foreign heart consumers, computed teacher binding, and ascension-room check with their current owners. The inactive-WLD review adds no approach; add no candidate, activate no route, or grant migration clearance. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L317), [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-859-twin-towers-boundaries-and-typed-dependencies-remain-unchanged), and [Twin Towers Forest distinction](TWIN_TOWERS_FOREST_AUDIT_REVIEW.md).

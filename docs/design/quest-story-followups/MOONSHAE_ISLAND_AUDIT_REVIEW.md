# Moonshae Island: graph, dependency, history, and retention reconciliation

Checkpoint 101 · Priority 141 · active area [`moonshae`](../../../areas/AREA#L132) · zone 262; title at [`moonshae.zon`](../../../areas/zon/moonshae.zon#L2).

This supplement adds repository-wide graph, inactive-map, typed-dependency,
source-history, and candidate-retention evidence to the
[comprehensive Moonshae dossier](../zone-stories/MOONSHAE_ISLAND.md) and
[source audit index](../../reference/zone-story-audits/moonshae.md). The dossier
remains the detailed story, access, reset, and builder-follow-up account; this
file records the broader cross-area closure. Source references below are static
repository declarations. They do not establish played availability or live
reachability.

## Active graph and cross-area routes

The active WLD has 127 Moonshae rooms and 266 exits. Two exits do not resolve in
the normal full-area set, leaving 264 loader-resolving exits. The local authored
graph is one weak component of all 127 rooms, with no fall-chance or current
markers. Five active inbound boundaries and five outbound exits are reciprocal:

| Inbound source | Moonshae return | Register |
| --- | --- | --- |
| `surface.wld` #557218 south → #26208 | #26208 north → #557218 | [cross-area register rows 404 and 1454](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L404) |
| `surface.wld` #557619 west → #26208 | #26208 east → #557619 | [rows 405 and 1457](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L405) |
| `llyrath.wld` #94900 north → #26257 | #26257 south → #94900 | [rows 406 and 1141](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L406) |
| `underworld3.wld` #30000 up → #26272 | #26272 down → #30000 | [rows 407 and 417](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L407) |
| `myrloch_vale.wld` #26401 east → #26311 | #26311 west → #26401 | [rows 408 and 409](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L408) |

The remaining two exits are from Port of Caer Corwell #26200: north to #148979
and south to #149179. Both target rooms occur only in inactive [`Duris3.wld`](../../../areas/wld/Duris3.wld), so
the normal loader prunes them. They are recorded as positive unresolved exits
in the [unresolved-exit register](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L149).
This qualifies the source graph and port repair question; it does not establish
that the port is otherwise unreachable. Preserve both intended-destination
questions for builder review before any separate route repair.

## Inactive WLD sweep and source-only approach

All 95 current WLD files outside the active manifest were parsed: 527,791 rooms
and 2,080,773 exits, with no parse issues and no source-room VNUM overlap with
active rooms. The sweep found one non-overlapping source-only approach into
Moonshae: inactive [`surf.wld`](../../../areas/wld/surf.wld) room #219584 exits west to active room #26208.
This is not active ingress. It raises the reviewed Surf subset from 129 to 130
and the curated source-only approach total from 1,513 to 1,514. The Surf attic
row and the global total now include it; see the
[Surf retention plan](ATTIC_MIGRATION_PLAN.md#L27).

## Typed inbound and outbound dependencies

The full local QST inputs and outputs refer to locally defined prototypes. No
local story-sidecar `item_vnums` field points outside Moonshae. The reviewed
external consumers are:

| Type and source | Target | Effective classification |
| --- | --- | --- |
| Active [`blackpearl.qst`](../../../areas/qst/blackpearl.qst#L384) and [`blackpearl.story.json`](../../../areas/story/blackpearl.story.json#L1069) | Object #26201 | One active external QST receipt and one typed sidecar reference. The sidecar's other numeric tokens are completion-key strings/step identifiers, not additional typed item dependencies. |
| [`fooquest.qst`](../../../areas/qst/fooquest.qst#L384) and [`raxquest.qst`](../../../areas/qst/raxquest.qst#L384) | Object #26201 | Both are source-only QST copies whose stems are absent from `areas/qst/AREA`; neither is an active contract. |
| Active [`cityruin.zon`](../../../areas/zon/cityruin.zon#L216), O reset | Object #26233 | Chance 20/cap 1 source row. Current ordinary O handling requires chance 100 unless forced; otherwise the row is disabled. Treat this as a legacy reference, not an ordinary reliable source. |
| Active [`myrloch_vale.obj`](../../../areas/obj/myrloch_vale.obj) #26407 at reset room #26440, type 25 | Room #26309 | One outside waterfall arrival lead. [`myrloch_vale.zon`](../../../areas/zon/myrloch_vale.zon#L89) resets it at #26440. It is selected travel context, not a required Moonshae story step. |
| Unregistered [`nexus56.shp`](../../../areas/shp/nexus56.shp#L346) and [`nexus56.zon`](../../../areas/zon/nexus56.zon#L1350) | Object #26238 | Source-only shop/reset references. The candidate plan records conditional shop-loader fallback exposure; neither declaration proves an operating shop or active reset. |

No outside MOB or OBJ prototype duplicates a Moonshae mobile/object VNUM. All
231 outside story sidecars parsed without error; exact typed `item_vnums` review
found only the Black Pearl #26201 reference above. The external type-25 scan
covered 872 declarations and found only the Myrloch Vale waterfall targeting a
Moonshae room. Its source and stock room are recorded in the existing dossier
and the row above.

Outbound source review found one unresolved local reset prototype:
[`moonshae.zon`](../../../areas/zon/moonshae.zon#L113) has an O reset for object
#11100 at port #26200, but no matching object prototype exists in the reviewed active/source catalog. Full `renum_zone_table`
disables that reset. Do not count it as loaded stock. The local QST, local shops,
and typed sidecar item references otherwise do not add an outside prototype
dependency. [ud_map0.zon](../../../areas/zon/ud_map0.zon#L11) has a source-only M
reset to active Moonshae mobile #26225 (“robyn druid”;
[mobile](../../../areas/mob/moonshae.mob#L272)). Moonshae also defines local
object #26225 ([OBJ](../../../areas/obj/moonshae.obj#L372)), a separate
cross-type number collision. Because ud_map0 is unregistered, the M command does
not establish a current mobile placement.

Effective direct-code matches remain bounded: [`get_pole`](../../../src/economy/tradeskill.c#L571)
recognizes pole #26200, and [`specs.assign.c`](../../../src/specs/specs.assign.c#L937)
actively assigns `sister_knight` to #26218–#26222.
The `cymric_hugh` sword assignment is commented; ticket-taker references are
inactive/unbound as detailed in the dossier. Preserve the distinction between
active callbacks, dormant text/table references, generic item effects, and
source-only reset/shop rows.

## Title, tracked history, and disposition

The normalized active title “Moonshae Island” has one owner among 351 current
non-empty active `.zon` titles. Followed `moonshae.wld` path history begins with
the repository-wide `areas/` path move on 2009-08-12 and includes later edits on
2011-10-22, 2012-02-25, and 2013-11-13. The reviewed tracked path history and
likely alternate stems identify no distinct predecessor, rename to another
area stem, or replacement. This bounded title/path result does not decide
broader semantic similarities.

Moonshae remains **DO NOT MOVE** as an active `areas/AREA` contributor. Keep the
five reciprocal current boundaries, two pruned port destinations, source-only
Surf approach, active Black Pearl and Cityruin consumers, selected waterfall
lead, and source-only Nexus56 references visible in any future candidate review.
The active area itself is not an attic candidate. Preserve the existing two
independent native returns and thirteen builder follow-ups in the dossier; this
supplement adds no native gameplay or area-source change.

## Checkpoint 289: all-442 title and bounded history

Color-code, trailing-tilde, case, and whitespace normalization finds one active
owner for “Moonshae Island” among all 442 current `.zon` files:
[`moonshae.zon`](../../../areas/zon/moonshae.zon#L2). The earlier 351-title
result is superseded by this full repository scan. Followed same-stem WLD,
ZON, MOB, OBJ, QST, and SHP path histories begin with the 2009 area-tree move
and continue through later edits on those stems. The bounded history scan
establishes no alternate-stem rename, distinct predecessor, or whole-zone
replacement. Broader semantic resemblance remains an owner question and is not
replacement evidence. Preserve the active contributor as **DO NOT MOVE**.

The bounded direct typed-reference closure recorded above is complete for the
reviewed active and source-only declarations: active Black Pearl QST/sidecar
references to #26201, Cityruin's chance-20 O row for #26233, the Myrloch Vale
type-25 lead into #26309, source-only `fooquest`/`raxquest` and Nexus56
references, the missing local #11100 reset, the source-only ud_map0 M reset to active
Moonshae mobile #26225 (also the number of a local object), and effective
get_pole and sister_knight source bindings. No
outside duplicate MOB/OBJ prototype was found. Editor/tooling custody,
persisted references, indirect or dynamic consumers, and played outcomes remain
outside this source scan; none is inferred from a source declaration.

## Checkpoint 289: direct builder and owner questions

These questions close the source-backed builder-question review. Each preserves
the established graph and provenance until an owner resolves intent; none
asserts live access, availability, or successful gameplay.

1. **Question:** Should Port of Caer Corwell #26200 north still lead to #148979,
   which occurs only in inactive `Duris3.wld`?
   **Recommendation:** Keep the positive target documented as pruned and
   unresolved. Confirm the destination with the area owner before any separate
   route repair; do not present it as current access.
2. **Question:** Should port room #26200 south still lead to #149179 in inactive
   `Duris3.wld`?
   **Recommendation:** Preserve it as a second, separate unresolved destination
   and obtain an owner decision before changing the graph.
3. **Question:** Is Sea room #26285 north intentionally a self-loop despite its
   moonwell look text?
   **Recommendation:** Confirm the intended sea graph with the builder and
   preserve the source mismatch as a question. Do not infer a missing route
   from room prose alone; see the [dossier's room and route notes](../zone-stories/MOONSHAE_ISLAND.md#L24).
4. **Question:** Are the five current reciprocal boundaries still the intended
   Moonshae approaches?
   **Recommendation:** Keep the Surface pair at #26208, Llyrath #26257,
   Underworld3 #26272, and Myrloch Vale #26311 distinct in route records. Any
   closure or relocation needs an owner-reviewed graph change.
5. **Question:** Should the inactive Surf room #219584 west exit to #26208 count
   as current ingress?
   **Recommendation:** Keep it source-only and already counted on Surf candidate
   row 97. Do not add a second route or inflate the curated approach total.
6. **Question:** Is Myrloch Vale object #26407, type 25, targeting Moonshae
   room #26309 an intended waterfall arrival?
   **Recommendation:** Retain it as a selected external travel lead, with its
   source reset at Myrloch Vale #26440. Confirm the player's arrival and return
   path separately before making a travel or quest claim.
7. **Question:** Should active Black Pearl QST and sidecar references continue
   to use Moonshae object #26201?
   **Recommendation:** Keep the active QST and typed sidecar dependency visible
   as one live source owner. Preserve the existing independent contract and
   qualify acceptance, custody, and persistence through separate gameplay
   evidence.
8. **Question:** Do source-only `fooquest.qst` and `raxquest.qst` copies for
   object #26201 represent retired or intended contracts?
   **Recommendation:** Leave both classified as inactive source copies because
   their stems are absent from `areas/qst/AREA`. Ask their content owner before
   adopting or deleting either copy.
9. **Question:** Is active Cityruin's O reset for #26233 intended to supply the
   Moonshae item despite chance 20 and cap 1?
   **Recommendation:** Treat the row as a legacy source reference. Verify any
   forced-reset path with its owner; ordinary O handling does not establish a
   reliable item source.
10. **Question:** Should Moonshae port reset #26200 keep its O command for
    missing object #11100?
    **Recommendation:** Keep the reset classified as disabled by
    `renum_zone_table`; ask the area owner whether to remove it or define the
    intended object. Do not describe a passenger vessel as loaded stock.
11. **Question:** Do source-only Nexus56 shop/reset rows for local object #26238
    reflect an intended supported shop?
    **Recommendation:** Preserve them as unregistered source references and
    retain the candidate plan's conditional shop-loader note. Confirm shop
    ownership and active registration before claiming an operating shop.
12. **Question:** Does ud_map0.zon intentionally M-reset active Moonshae
    mobile #26225 into candidate room #1102588, given that Moonshae also has
    object #26225?
    **Recommendation:** Record the command as a source-only mobile consumer of
    Moonshae #26225 and separately note the same-number object; do not classify
    the M reset as an object consumer or a current placement.
13. **Question:** Is pole #26200's effective `get_pole` recognition intended
    to remain part of Moonshae's fishing content?
    **Recommendation:** Record the compiled direct-code binding, while leaving
    skill, water, timing, interruption, XP, and committed catch outcomes to
    gameplay qualification; see the [fishing dossier note](../zone-stories/MOONSHAE_ISLAND.md#L34).
14. **Question:** Are the active `sister_knight` assignments to #26218–#26222
    still the intended cast?
    **Recommendation:** Preserve the exact active assignment range from
    `specs.assign.c`; the source establishes bindings, not that each mobile is
    loaded or reachable.
15. **Question:** Should the commented `cymric_hugh` sword assignment remain
    dormant?
    **Recommendation:** Keep it excluded from active dependency claims unless a
    builder explicitly restores and verifies the assignment.
16. **Question:** Should legacy `ticket_taker` references become a supported
    passenger service?
    **Recommendation:** Keep ticket-taker classified as unbound and the
    `#if 0` callbacks inactive. The dossier records that #11100 is missing and
    #11300 is an active brittle key; an owner must choose retirement or a
    separately scoped supported route before ticket/voyage claims.
17. **Question:** Should the two native Moonshae QST returns from Brigit #26221
    and Tristan #26208 remain independent?
    **Recommendation:** Preserve both contracts, their existing return items,
    and the dossier's separate outcomes. Do not combine receipts or assume a
    shared item proves both contracts.
18. **Question:** Does a competing alias or title owner justify merging or
    renaming Moonshae?
    **Recommendation:** The normalized “Moonshae Island” title has one owner
    across all 442 current `.zon` files. Preserve that identity result and
    route broader command-alias questions to the owning builder.
19. **Question:** Does the followed history show a predecessor or replacement
    that changes Moonshae's ownership?
    **Recommendation:** No alternate-stem predecessor, rename, or whole-zone
    replacement appears in the bounded same-stem histories. Keep broader
    semantic similarity as a separate, unresolved owner judgment.
20. **Question:** Should the active Moonshae contributor be considered for
    attic migration?
    **Recommendation:** Keep **DO NOT MOVE** while the five active reciprocal
    boundaries and typed consumers remain documented. Reopen only through a
    separate owner-reviewed disposition that resolves routes, stock ownership,
    and dependent content.

The all-442 title/history review, bounded direct typed closure, and twenty
source-backed builder questions are complete for Checkpoint 289. Static source
review still does not qualify actual access, availability, safe return, or
played quest and fishing outcomes.

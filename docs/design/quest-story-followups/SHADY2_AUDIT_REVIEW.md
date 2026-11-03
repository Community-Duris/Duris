# shady2 mobile-source audit

Canonical key: `component:shady2`. Physical coverage row 443 / CSV line
444; candidate dependency data row 92 / CSV line 93. Reviewed October 11,
2026, Checkpoint 530, against checkout
`c500ac51b8cd26540943257e7ca4e9807263ef74`. This is a static source,
manifest, typed-reference, prototype-identity, and history review. It does
not establish that the unlisted file is consumed by an editor, export, or
runtime path.

## Inventory and manifest status

The candidate consists of one file,
[shady2.mob](../../../areas/mob/shady2.mob#L1), with 90 mobile prototypes
#97500–#97589. It has no same-stem WLD, OBJ, ZON, QST, SHP, or story
sidecar. `shady2` is absent from [areas/AREA](../../../areas/AREA) and
[areas/qst/AREA](../../../areas/qst/AREA); the current
[RANDOM_AREA](../../../areas/RANDOM_AREA) is empty. With no WLD source,
there are no candidate rooms, authored exits, or WLD connectivity totals.

By contrast, the listed [shady](../../../areas/AREA#L351) component is also
in [qst/AREA](../../../areas/qst/AREA#L310). Its
[shady.mob](../../../areas/mob/shady.mob#L1) defines all 90 same-type
VNUMs #97500–#97589, plus 16 further mobiles #97590–#97605. The active
[Shady Grove sidecar](../../../areas/story/shady.story.json#L1) declares
`source_area` as `shady`; it does not establish a `shady2` identity.

## Same-VNUM prototype comparison

The two files share all 90 mobile VNUMs, but none of the 90 full records is
identical. A comparison of the four tilde-terminated text fields and the
following stat/mechanical lines found:

| Record content | Exact matches out of 90 | Difference |
| --- | ---: | --- |
| Keyword/alias list | 85 | #97540, #97545, #97562, #97580, and #97582 differ. |
| Short description | 89 | #97580 differs. |
| Long description | 89 | #97580 differs. |
| Detailed description | 89 | #97580 differs. |
| Stat/mechanical block | 0 | All 90 blocks differ. |

Thus the common names and much of the prose do not establish that the two
files are interchangeable snapshots. In particular, #97580 changes all
four text fields, and every shared mobile has a distinct mechanical
definition. The candidate's
[#97540](../../../areas/mob/shady2.mob#L590),
[#97545](../../../areas/mob/shady2.mob#L664),
[#97562](../../../areas/mob/shady2.mob#L913),
[#97580](../../../areas/mob/shady2.mob#L1168),
[#97582](../../../areas/mob/shady2.mob#L1194), and
[#97589](../../../areas/mob/shady2.mob#L1296) records illustrate the
affected aliases and broad mechanical divergence; the same VNUMs occur at
the corresponding records in active `shady.mob`.

## Current typed consumers

Mobile-typed consumers found in listed source use the IDs represented by
active `shady.mob` under the current `areas/AREA` manifest. Object-typed
quest and story item references resolve separately to active `shady.obj`:

| Source type | Static count and source set | Interpretation |
| --- | --- | --- |
| ZON mobile resets | 342 `M` rows: 336 in active `shady.zon`, four in `caertannad.zon`, and two in `alatorin.zon` | They name active Shady prototypes by VNUM; they do not identify the unlisted `shady2.mob` file as their source. |
| Shopkeepers | 12 records in active `shady.shp`, with keeper IDs #97502, #97515, #97521, #97522, #97534, #97535, #97539, #97558, #97575, #97576, #97581, and #97587 | Current manifest identity is the corresponding active Shady mobile. |
| QST mobile headers | Four active `shady.qst` headers use #97510, #97537, #97545, and #97548 | These are keyed to the active Shady mobile family; no `shady2.qst` is present. |
| QST item operands | Four active `shady.qst` item operands use #97514, #97570, #97582, and #97572 | Each resolves by item type to an active Shady OBJ prototype, regardless of the same-number mobile VNUMs. |
| Story item contracts | `shady.story.json` carries those four Shady OBJ item IDs; active Alatorin QST/story also use item #97549 | These resolve to active Shady objects. The numbers overlap the candidate MOB range; they are not references to `shady2` mobiles. |
| C/C++ references | 13 source lines across assignments, summon/read-mobile code, justice, mail, and Winterhaven | These mobile-typed references resolve numerically to active Shady prototypes under the current manifest. |

Representative source evidence includes the active ZON files
([shady](../../../areas/zon/shady.zon),
[Alatorin](../../../areas/zon/alatorin.zon),
[Caertannad](../../../areas/zon/caertannad.zon)), the active
[Shady shop records](../../../areas/shp/shady.shp#L1), and the four
[Shady QST headers](../../../areas/qst/shady.qst#L1). The four QST item
operands are at [lines 18](../../../areas/qst/shady.qst#L18),
[20](../../../areas/qst/shady.qst#L20), [42](../../../areas/qst/shady.qst#L42),
and [67](../../../areas/qst/shady.qst#L67); their active object definitions
are #97514 at [line 131](../../../areas/obj/shady.obj#L131), #97570 at
[line 689](../../../areas/obj/shady.obj#L689), #97572 at
[line 711](../../../areas/obj/shady.obj#L711), and #97582 at
[line 811](../../../areas/obj/shady.obj#L811). Active Alatorin uses item
#97549 in its [QST](../../../areas/qst/alatorin.qst#L5347) and
[story contract](../../../areas/story/alatorin.story.json#L2735); the active
Shady object definition is at [line 477](../../../areas/obj/shady.obj#L477).
Code sites include
[mobile special assignments](../../../src/specs/specs.assign.c#L346), the
[illus assignment](../../../src/specs/specs.assign.c#L429), and the two
[Ixie assignments](../../../src/specs/specs.assign.c#L730),
[including the second binding](../../../src/specs/specs.assign.c#L758),
[summoning](../../../src/item/wonder_actions.c#L220),
[highway mobile creation](../../../src/specs/specs.highway.c#L337),
[justice data](../../../src/combat/justice.c#L79),
[mail](../../../src/cmd/mail.c#L135), and
[Winterhaven helpers](../../../src/specs/specs.winterhaven.c#L162).

The matching number #97521 also appears as an OBJ stock/reset reference in
unlisted `nexus56`; that is a different prototype type, not an external MOB
reference to `shady2` ([shop](../../../areas/shp/nexus56.shp),
[zone reset](../../../areas/zon/nexus56.zon#L1324)). Likewise, quest/story
item numbers #97514, #97549, #97570, #97572, and #97582 resolve to active
Shady OBJ identities. All lie in the candidate's #97500–#97589 MOB range;
#97549 also names the active Shady and candidate Shady2 baby-orc MOB records
([active](../../../areas/mob/shady.mob#L725),
[candidate](../../../areas/mob/shady2.mob#L725)). The Alatorin item contract
is the orcish-forged sword OBJ #97549
([object](../../../areas/obj/shady.obj#L477)), not either same-number MOB.
Numeric scans alone must keep these prototype types distinct.

## History, open questions, and disposition

Followed-path history reaches the 2009 area-tree move and does not record a
later edit to `shady2.mob` or an owner decision naming it as a predecessor,
replacement, or retired export. The prior
[Shady Grove source supplement](SHADY_GROVE_AUDIT_REVIEW.md#tracked-history-and-duplicate-source-boundary)
records the same history boundary and the 90-ID collision. That collision
does not establish which file an editor, custom export, deployment step,
or saved VNUM reference may expect.

Retain `areas/mob/shady2.mob` IN PLACE and separately from active
`areas/mob/shady.mob`. Preserve its 90 records and all variant fields while
the source owner confirms their purpose, namespace assignment, and external
tooling/persisted-reference closure. Do not infer that it is disposable,
merge it into `shady.mob`, or rename it from manifest exclusion and numeric
overlap alone. No activation, archive, move, or migration clearance is
implied. See the [21 builder questions](SHADY2_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-530-shady2-mob-only-source),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-530-shady2-shared-mobile-vnums-are-distinct-records),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-530-shady2-preserve-the-mob-variant),
[candidate dependency row 92](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L93),
and [coverage row 443](ZONE_COVERAGE_REGISTER.csv#L444).

## Checkpoint 779: Caertannad typed dependency

Active Caer Tannad has four ZON rows for mobile #97550. The active manifest owner is Shady; the same-number Shady2 prototype remains a separate candidate identity. See [Caertannad typed audit](CAERTANNAD_AUDIT_REVIEW.md#outbound-active-reset-references).

## Checkpoint 934: quest and story item IDs remain object typed

The active Shady QST and story sidecar use item VNUMs #97514, #97570,
#97572, and #97582, each defined as an active Shady OBJ. Alatorin also uses
#97549 as an item in its active QST and Brino story contract; the active
object is the orcish-forged sword. These five item VNUMs lie inside
`shady2.mob`'s numeric range. #97549 additionally identifies a baby-orc MOB
in both active `shady.mob` and candidate `shady2.mob`. The item-typed
contracts resolve to the OBJ record and do not identify either same-number
mobile. The candidate still has no QST/story sidecar, WLD source, or active
map ingress; its disposition remains **RETAIN IN PLACE**. See the
[Shady Grove typed-reference supplement](SHADY_GROVE_AUDIT_REVIEW.md#checkpoint-934-97549-is-an-object-and-mobile-vnum),
[builder questions](SHADY2_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-934-shady2-item-references-add-no-map-ingress),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-934-shady2-item-and-mobile-numbers-remain-type-distinct),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-934-shady2-retains-mobile-source-and-typed-item-boundaries),
candidate dependency [row 92](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L93),
and coverage [row 443](ZONE_COVERAGE_REGISTER.csv#L444).

# myranth1 quest and shop source audit

Canonical key: component:myranth1. Physical coverage row 426 / CSV line 427;
candidate dependency row 75 / CSV line 76. Reviewed October 11, 2026,
Checkpoint 513, against source snapshot
c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static source,
narrative, typed-reference, loader-path, and bounded-history review. It does
not establish live quest or shop availability, editor ownership, or archive
safety.

## Package and quest story

The source-only package has two tracked files:
[QST](../../../areas/qst/myranth1.qst#L1) and
[SHP](../../../areas/shp/myranth1.shp#L1). There is no same-stem WLD, MOB,
OBJ, or ZON, nor a story or map JSON sidecar. Neither
[areas/AREA](../../../areas/AREA) nor
[areas/qst/AREA](../../../areas/qst/AREA) lists myranth1. Therefore the
candidate has no local room graph or mobile/object prototype package.

The QST contains two giver headers, #70132 and #70133, and two native
Q/QA contracts. Header #70132 is Moorney, who asks for a cell key and
describes escaping an infernal magical trap. Its completion removes 10,000
coins and item #70157, gives item #70108, while its D text says “the
conjurer” dashes out rather than clearly naming Moorney
([QST lines](../../../areas/qst/myranth1.qst#L1)). Header #70133 is the
leper, who asks the player to take Moorney’s key to him; its item transition
removes #70108 and gives #70107
([leper contract](../../../areas/qst/myranth1.qst#L21)). The prose calls the
output Moorney’s key, but the source cannot verify the item roles because
these VNUMs have no object prototypes. This is a source story chain, not
evidence that either giver or key exists in the current world.

There is a clear narrative echo in active Knight mobile Zyla #70919: its
description says that apprentice Moorney was imprisoned for Zyla’s demon
summoning ([Zyla](../../../areas/mob/knight.mob#L278)). That correspondence
makes a Knight/Moorney story relationship plausible, but it is prose rather
than a QST ID, reset, or loader link. The candidate giver prototypes #70132
and #70133 are absent from the current mobile sources, and no current
Knight QST establishes this escape chain as active.

## Typed item references and missing prototypes

All three unique QST item VNUMs—#70157, #70108, and #70107—lack current
object prototypes in the full and mini object sources. Candidate QST uses
#70107 and #70108 with item commands, while active Moregeeth mobiles at
those exact VNUMs are a huge warg and warg cub
([warg](../../../areas/mob/goblinht.mob#L1615),
[cub](../../../areas/mob/goblinht.mob#L1632)). Treat this as an object/mobile
type collision in the source data, not as a live item resolution.

The SHP has five records with 19 unique product references, and none of the
19 VNUMs has a current object prototype:

| Shop | Keeper mobile | In-room VNUM | Product object VNUMs |
| --- | ---: | ---: | --- |
| #70101 | #70101 | #70101 | #70182, #70172, #70184 ([source](../../../areas/shp/myranth1.shp#L1)) |
| #70102 | #70102 | #70107 | #70174, #70175, #70180 ([source](../../../areas/shp/myranth1.shp#L43)) |
| #70103 | #70103 | #70108 | #70168, #70169, #70170, #70171, #70173 ([source](../../../areas/shp/myranth1.shp#L81)) |
| #70104 | #70104 | #70111 | #70176, #70177, #70178, #70179, #70183 ([source](../../../areas/shp/myranth1.shp#L126)) |
| #70105 | #70105 | #70110 | #70187, #70188, #70190 ([source](../../../areas/shp/myranth1.shp#L165)) |

The shop keeper and room values match active Moregeeth mobile and room
prototypes; the exact typed fields and active mobile links are recorded in the
[Moregeeth supplement](MOREGEETH_AUDIT_REVIEW.md#direct-external-typed-references).
These are SHOP references into Moregeeth, not WLD routes. The active
goblinht SHP has its own shop records #70029–#70038; myranth1’s five records
are separate records keyed to Moregeeth mobiles #70101–#70105.

## Manifest and conditional loader paths

Normal SHP aggregation reads areas/AREA and chooses each listed stem’s
per-area shop file. It omits myranth1. The source-level runtime shop opener
first tries areas/world.shp, then—if that file cannot be opened—globs every
areas/shp/*.shp file, including myranth1
([Moregeeth loader-path evidence](MOREGEETH_AUDIT_REVIEW.md#direct-external-typed-references)).
This establishes a conditional source-code path only; it does not establish
that a live boot used it or that these five shops operated.

The normal QST compiler likewise reads the root AREA list and compiles only
listed quest files. Runtime quest loading opens areas/world.qst and explicitly
does not glob areas/qst/*.qst
([QST compiler](../../../areas/src/qst/make_qst.c#L6),
[QST runtime loader](../../../src/world/quest.c#L2086)). Because myranth1 is
absent from both lists, its QST is excluded from the normal aggregate path;
the source does not establish a loaded Moorney quest.

## Provenance and safe disposition

Followed histories for the QST and SHP paths reach the 2009 area-tree move
and a same-stem 2013 update. They show no explicit rename to a successor or
whole-area replacement. The QST’s Moorney text aligns with active Zyla’s
backstory, while the SHP’s keeper and room fields point into active
Moregeeth. These separate evidence types do not establish that myranth1
replaced either active area or that the two candidate files have one complete
zone owner.

**Disposition: RETAIN IN PLACE.** Preserve both files as a distinct
source-only component. Resolve the intended QST owner and location, missing
giver and item prototypes, the #70107/#70108 type collision, all 19 missing
shop products, the Moregeeth shop bindings, and the conditional shop-loader
path before any separate activation, move, merge, or archive decision. No
migration clearance is implied. See the
[16 builder questions](MYRANTH1_BUILDER_QUESTIONS.md),
[Knight story supplement](KNIGHT_AUDIT_REVIEW.md#checkpoint-513-moorney-quest-story-link),
[Moregeeth supplement](MOREGEETH_AUDIT_REVIEW.md#checkpoint-513-myranth1-qst-item-typing-and-moregeeth-shop-records),
[connectivity supplement](WORLD_CONNECTIVITY.md#checkpoint-513-myranth1-has-no-wld-graph),
[identity/history supplement](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-513-myranth1-provenance-and-story-link),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-513-myranth1-retained-pending-qst-and-shop-closure),
[candidate dependency line 76](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L76), and
[coverage line 427](ZONE_COVERAGE_REGISTER.csv#L427).
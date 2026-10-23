# surface2011 source-component audit

Canonical key: component:surface2011. Coverage data row 448 / CSV line 449; candidate dependency data row 97 / CSV line 98. Reviewed October 11, 2026, Checkpoint 535, against checkout c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static manifest, source-map, QST narrative, typed-reference, identity, and followed-history review. It does not establish the candidate’s current load state or the effect of its historical manifest entry.

## Source inventory, title, and manifest history

The five tracked files are [surface2011.wld](../../../areas/wld/surface2011.wld#L1), [surface2011.mob](../../../areas/mob/surface2011.mob#L1), [surface2011.obj](../../../areas/obj/surface2011.obj#L1), [surface2011.zon](../../../areas/zon/surface2011.zon#L1), and [surface2011.qst](../../../areas/qst/surface2011.qst#L1). The WLD contains 90,000 rooms #200000–#289999. The MOB has 213 prototypes #200000–#200212, the OBJ has 76 prototypes #200000–#200075, and the QST has 37 giver headers, all matching local mobile prototypes. The QST contains 90 M prompt blocks, 23 QA response blocks, and 76 R/G item references. The ZON header is #2000, its title is “Norabi Map,” its last room is #289999, and reset mode is 2; it has a 300 by 300 grid declaration but no M/G/O/E/P/D/R/H/F reset commands. There is no SHP source or separate story sidecar.

After color removal, whitespace normalization, and case folding, “Norabi Map” occurs in two of the 442 current ZON sources: surface2011 and underdark2011. Surface2011 is absent from current [areas/AREA](../../../areas/AREA), [areas/qst/AREA](../../../areas/qst/AREA), and the empty current [RANDOM_AREA](../../../areas/RANDOM_AREA). No active QST header overlaps its 37 giver IDs.

The historical full-area list included surface2011 under header #2000 in May 2011. The November 2013 areas/AREA diff removed that row while resolving a merge conflict; its generic commit message gives no reason. The current manifest excludes the component, but the tracked change does not state whether the removal was intended as retirement, replacement, or cleanup. A December 2011 ZON commit titled “surface2011 is no longer silent zone file” changes a header flag; the current file still has no reset commands. Keep the historical entry as provenance, not as proof of current availability or a replacement decision.

## Map topology and competing identities

The candidate map has exactly 360,000 authored exits: 90,000 each in D0, D1, D2, and D3. Every target is one of its 90,000 defined rooms. There are no external or negative targets, no out-of-range source rooms, and no range-mismatch source exits. The graph is one weak component of 90,000 rooms; a simple directed walk from #200000 reaches all 90,000 when room access conditions are ignored. This does not establish gameplay access. The map has no authored outbound WLD edge that returns any of the 145 currently active inbound targets.

Of the 90,000 room numbers, 89,999 also occur in surf.wld and 15,482 in underdark2011.wld. Full-record comparisons found no identical room blocks. The 213 candidate mobile numbers also occur in surf.mob; 68 occur in underdark2011.mob. No full mobile records match. Eight OBJ VNUMs #200000–#200007 also occur in underdark2011.obj, but the full object records differ; surf.obj is empty. No current active MOB or OBJ source defines a surface2011 VNUM.

Surface2011 has 145 active inbound WLD references from the same 17 current contributors recorded on its dependency row. All 145 targets are ambiguous by number: 134 also identify rooms in surf, and 11 occur in all three candidate maps. No source-only ingress can be assigned uniquely to surface2011 from VNUM alone. Candidate identity and reciprocal direction remain unresolved; no global active or curated route total changes.

## QST narrative and typed item dependencies

The 37 QST giver headers all resolve to local surface2011 mobile prototypes. The QST carries a story about Labyrinthmaster Sa’Dah, four named traitors, their heads/hearts and lockets, a book of tomorrows, and a powerful mystic whose imprisonment is tied to the lockets. The candidate’s Sa’Dah QST header #200000 and active Surface’s #500000 block share 28 nonempty body lines verbatim out of 40 stored non-header rows; both describe the four-traitor and locket campaign, while requirement/reward records and some text differ. Candidate Sa’Dah mobile #200000 also shares most descriptive text with active Surface #500000, but the complete records differ. This is strong story-content continuity, not an explicit area migration or map replacement. See the [active Surface audit](SURFACE_AUDIT_REVIEW.md) and [Surface Realm dossier](../zone-stories/SURFACE_REALM.md).

The 76 QST R/G item rows reference 56 local objects and 20 objects from active sources. The external rows point to 14 distinct object VNUMs in Heavens (#293, #294, #295, #319, #330, #333–#335, #355, #356, and #729), Breale (#2676), and Ship Yards (#43140 and #43142); all resolve to active object prototypes. The candidate’s 76 local objects have no current active numeric counterpart. The ZON contributes no placement resets, so QST item records do not establish live availability.

Code also contains direct mobile assignments for #200011 and #200012 at [specs.assign.c lines 436 and 439](../../../src/specs/specs.assign.c#L436); both IDs occur in surface2011.mob and surf.mob. [real_mobile0()](../../../src/world/db.c#L8553) returns zero for an absent prototype. This is a direct source/code binding, not evidence that either candidate mobile is loaded or has a live special procedure.

## History and disposition

Tracked WLD, MOB, OBJ, ZON, and QST history shows extensive 2011 content work, and later 2013/2026 source maintenance. The 2011 full-area listing and 2013 removal are the explicit manifest history found; neither commit states a whole-zone replacement. The current active Surface map uses a different room range and has no same-number WLD, MOB, or OBJ identities, although its active QST and Sa’Dah text preserve part of this campaign.

Retain all five files in place. Ask the map, quest, and deployment owners to explain the 2013 manifest removal, confirm the “Norabi Map” title collision, reconcile the 145 ambiguous inbound references, and identify the authoritative editor/export/build inputs before any separate activation, movement, merge, or archive proposal. The twenty-one questions in [SURFACE2011_BUILDER_QUESTIONS.md](SURFACE2011_BUILDER_QUESTIONS.md) record those decisions. No migration clearance is implied.

## Checkpoint 777: Breale QST dependency

The candidate QST has G I #2676 at line 556, referencing active Breale object #2676. Preserve it under the existing RETAIN IN PLACE plan; it does not establish active quest availability. See the [Breale typed-dependency audit](BREALE_AUDIT_REVIEW.md#registered-candidate-only-consumers).

## Checkpoint 936: Surf-range QST values retain their source-owned types

Surf’s mobile range #200000–#200321 also contains 37 `surface2011.qst` giver headers and 56 item operands. The giver values are source-local QST keys matching `surface2011.mob`; the item operands are OBJ-typed and resolve to local prototypes in `surface2011.obj`. `underdark2011.qst` contributes one source-local giver header and four OBJ item operands in the same numeric range, owned by its own unlisted MOB and OBJ sources. Neither QST file is listed in `areas/qst/AREA`. The numeric overlap creates no Surf QST dependency and does not change the existing RETAIN IN PLACE disposition. See the [Surf QST boundary review](SURF_AUDIT_REVIEW.md#checkpoint-936-old-surface-qst-values-do-not-identify-surf-mobiles).

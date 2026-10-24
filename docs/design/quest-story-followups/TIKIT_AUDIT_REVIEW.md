# Lost City of Tikitzopl (Priority 220) repository-wide audit review

This supplement adds current WLD reconciliation, inactive-map approaches, typed cross-area references, and bounded title/history review to the [comprehensive Lost City of Tikitzopl source dossier](../zone-stories/LOST_CITY_OF_TIKITZOPL.md). The dossier retains the detailed story families, builder questions, and recommended resolutions. These static findings do not qualify installed access, item publication, or player outcomes.

## Active map and directed reach

Lost City of Tikitzopl (tikit) has 340 rooms (#43700–#44039) and 855/855 resolving exits: 853 local and two outbound. Its active local graph is one weak component of 340 rooms. With all authored local exits treated as available, raw traversal from root #43700 reaches and returns from all 340 rooms; the scan found no flag-8 internal exits. Both active outbound boundaries reciprocate:

- City room #43901 west to Surface #561015 east.
- City room #44039 down to Lost Temple of Tikitzopl #44101 up.

See the [root room](../../../areas/wld/tikit.wld#L1), active exit-register [outbound rows 617–618](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L617), [Temple return row 619](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L619), and [Surface return row 1467](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1467), plus pair-summary [rows 609 and 659–661](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L609). A Temple object also declares a type-25 destination to a separate City room; that is an object route, not an authored WLD edge.

## Inactive WLD sweep

All 95 inactive WLD files parsed: 527,791 rooms and 2,080,773 exits, with no parse errors or source-room VNUM overlap against the City. One non-overlapping source-only approach enters through surf.wld: room #248527 east (D1) to #43901 ([source room](../../../areas/wld/surf.wld#L1018227), [target](../../../areas/wld/surf.wld#L1018238)). This is an inactive map declaration, not active ingress. Curated source-only totals rise from 3,432 to 3,433 overall and from 199 to 200 for Surf; ud_map2 remains 15. Preserve it with existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); no candidate row is added.

## Typed cross-area references

The City’s own QST item commands, ZON reset roles, SHP products, and story-sidecar item_vnums remain within its local prototype sets. The repository-wide exact scan found these external references into City prototypes:

- Active Lost Temple QST inputs reference 15 City objects across six equipment recipes: #43720, #43706, #43719, #43723, #43705, #43707, #43710, #43714, #43715, #43717, #43718, #43739, #43703, #43752, and #43753 ([QST entries](../../../areas/qst/tikitt.qst#L233), [ring recipe](../../../areas/qst/tikitt.qst#L262), [sapphire recipe](../../../areas/qst/tikitt.qst#L274)). The active Lost Temple story sidecar has corresponding typed item_vnums entries ([sidecar](../../../areas/story/tikitt.story.json)). These cross-area crafting contracts remain owned by the Temple; their item references do not imply that the City owns those contracts.
- Active Surface Keeps reset line 122 has G object #43708 under mobile #120050 in room #120992 ([reset source](../../../areas/zon/surfacekeeps.zon#L121), [City object](../../../areas/obj/tikit.obj#L139)). The source comment says the object is not defined in Surface Keeps’ object file; preserve the external prototype reference.
- Lost Temple object #44190 declares type 25 with destination #43704 ([portal definition and target](../../../areas/obj/tikitt.obj#L1461)); its active reset places it in room #44333 ([reset](../../../areas/zon/tikitt.zon#L200); [City destination room](../../../areas/wld/tikit.wld#L88)). This one-way Temple return is separate from the reciprocal Temple WLD boundary.

The bounded exact scan found no other external QST item command, SHP product, ZON reset-role reference, or external story-sidecar typed VNUM use of City prototypes. The broad sidecar token scan’s only other hits were digits embedded in an Alatorin exchange ID (#43847) and a White Harbor request ID (#43844), not typed VNUM fields. All 231 external story sidecars parsed without errors. No external mobile/object prototype duplicates or direct C/C++ numeric VNUM bindings were found. The 872 external type-25 declarations include the one Temple destination above and no other target into a City room.

## Title and bounded history

The normalized .zon title “Lost City of Tikitzopl” is unique among the 442 scanned current .zon files ([title source](../../../areas/zon/tikit.zon#L2)). Lost Temple of Tikitzopl (tikitt) has a distinct normalized title and separate zone/VNUM ranges. Followed tikit.wld history stays on the same stem through the 2009 areas-tree move, 2010 edits, and 2011/2013 changes; tikit.zon history reaches the same move, same-stem 2010/2013 edits, the 2014 typo correction, and the 2015 quest-item-flag pass. This bounded WLD/ZON review found no alternate-stem rename, distinct predecessor, or whole-zone replacement. The reciprocal Temple boundary, Temple crafting dependencies, type-25 return, and source-only Surf approach are route/dependency evidence, not replacement evidence.

## Migration disposition

Keep active tikit **DO NOT MOVE**. Preserve its reciprocal Surface and Temple WLD boundaries, the source-only Surf approach, the Temple’s exact recipe inputs and type-25 return, and Surface Keeps’ active reset reference to object #43708. Keep surf and surface2011 **RETAIN IN PLACE** with candidate row 97. See [Lost City coverage row 303](ZONE_COVERAGE_REGISTER.csv#L303), [Lost Temple row 304](ZONE_COVERAGE_REGISTER.csv#L304), [Surface Keeps row 287](ZONE_COVERAGE_REGISTER.csv#L287), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). No migration clearance is implied.

## Checkpoint 390: builder-question coverage reconciliation

The existing Lost City dossier had fifteen broader storylines and forty-five
technical findings with follow-up plans, but those entries were not phrased
as direct builder questions. This checkpoint adds sixteen explicit,
source-backed questions with conservative recommendations in the
[dossier table](../zone-stories/LOST_CITY_OF_TIKITZOPL.md#explicit-builder-questions-and-recommended-resolutions):
one addresses contact-target ownership and aliases; the other fifteen map to
the broader progression stories already documented there. The existing
ZSQ-TIKIT-01 through ZSQ-TIKIT-45 rows retain detailed source and integration
boundaries.

These are owner decisions and proposed story boundaries, not claims that new
rescue, welfare, civic, service, or safe-travel mechanics already exist. The
coverage cell for physical row 303 now records the question-and-resolution
review as complete. Keep active `tikit` DO NOT MOVE. Preserve its current
Surface and Temple pairs, and keep the #248527 Surf approach source-only on
existing Surf candidate row 97; its detailed plan now records the source room, exit, and active target. The route was already included in CP180 curated totals, so no aggregate count changes. No new map candidate or migration is added.
See [coverage row 303](ZONE_COVERAGE_REGISTER.csv#L303),
[connectivity](WORLD_CONNECTIVITY.md), [identity/history](DUPLICATE_AND_REPLACED_ZONES.md),
and [attic disposition](ATTIC_MIGRATION_PLAN.md).

## Companion Lost Temple review

Checkpoint 391 closes the Temple side of the shared boundary and recipes.
See the [Lost Temple audit](TIKITT_AUDIT_REVIEW.md) for its separate directed
reach, one-way Winterhaven arrival, Heavens reset prototype, owner-attributed
City inputs, type-25 route and retention decision. Those Temple records do
not change the City candidate disposition.

## Checkpoint 845: Current cross-area typed closure refreshed

The typed closure remains the one recorded in Checkpoints 180 and 390. Six
active Lost Temple QST equipment recipes consume fifteen exact City objects;
the Temple's story sidecar mirrors those item VNUMs. These are Temple-owned
crafting contracts, not City requests or proof that the items are recovered
from City reset sources. ([Temple QST recipes](../../../areas/qst/tikitt.qst#L233),
[additional recipes](../../../areas/qst/tikitt.qst#L262),
[Temple sidecar](../../../areas/story/tikitt.story.json))

The other active typed edges remain separate: Surface Keeps resets local City
OBJ #43708 beneath its keeper, and Lost Temple OBJ #44190 declares a type-25
target into City room #43704 while its reset places the item at Temple #44333.
The target is item travel, not an authored WLD edge. The bounded scans still
find no other outside exact QST, SHP, ZON, story-sidecar VNUM, duplicate
mobile/object prototype, or type-25 destination into City. Two broad sidecar
number matches (#43847 and #43844) remain opaque Alatorin/White Harbor IDs,
not typed City VNUMs. No direct C/C++ numeric VNUM binding was found.
([Surface Keeps reset](../../../areas/zon/surfacekeeps.zon#L121),
[City object](../../../areas/obj/tikit.obj#L139),
[Temple type-25 object](../../../areas/obj/tikitt.obj#L1461),
[Temple reset](../../../areas/zon/tikitt.zon#L200))

The reciprocal Surface and Lost Temple WLD pairs are unchanged. Inactive Surf
#248527 east to City #43901 remains source-only on Surf candidate row 97; its
approach was already included in cumulative counts. Keep active `tikit`
**DO NOT MOVE**, with `surf` and `surface2011` **RETAIN IN PLACE** as separate
source identities. No new route, candidate, or migration clearance is added.

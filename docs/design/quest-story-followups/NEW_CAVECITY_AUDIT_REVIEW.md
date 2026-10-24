# New Cave city (Priority 219) repository-wide audit review

This supplement adds current WLD reconciliation, inactive-map approaches, typed cross-area references, and bounded title/history review to the [comprehensive New Cave city source dossier](../zone-stories/NEW_CAVE_CITY.md). These static findings do not qualify installed access, item publication, or player outcomes.

## Active map and directed reach

New Cave city (new_cavecity) has 73 rooms (#15100–#15186) and 155/155 resolving exits: 152 local and three outbound. Its active local graph is one weak component of 73 rooms. With all authored local exits treated as available, raw traversal from root #15100 reaches and returns from all 73 rooms; the scan found no flag-8 internal exits. The three outbound exits each have a reciprocal active return:

- Room #15102 south to Underdark #749656 north.
- Room #15161 east to Surface #584184 west.
- Room #15161 south to Surface #584583 north.

See the [root room](../../../areas/wld/new_cavecity.wld#L1), active exit-register outbound [rows 268–270](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L268), inbound [rows 1531, 1533, and 1707](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1531), and pair-summary [rows 314–315](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L314) and return [rows 557 and 724](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L557). The static graph does not establish door state, reset state, scripts, transport behavior, or safe return in a running game.

## Inactive WLD sweep

All 95 inactive WLD files parsed: 527,791 rooms and 2,080,773 exits, with no parse errors or source-room VNUM overlap against New Cave city. One non-overlapping source-only approach enters through surf.wld: room #277907 north (D0) to #15102 ([source room](../../../areas/wld/surf.wld#L1634807), [target](../../../areas/wld/surf.wld#L1634814)). This is an inactive map declaration, not active ingress. Curated source-only totals rise from 3,431 to 3,432 overall and from 198 to 199 for Surf; ud_map2 remains 15. Preserve it with existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); no candidate row is added.

## Typed and compiled references

New Cave city’s local Q16 recipe consumes Heavens mystic tablet #402, Bctdl Bel’s heart #32490, and Negative Plane Orb of Unmaking #26614, then returns Heavens Intelligence scroll #411 without disappearance ([QST lines 20–24](../../../areas/qst/new_cavecity.qst#L20)). Its structured sidecar’s three item_vnums entries mirror those exact inputs ([tablet](../../../areas/story/new_cavecity.story.json#L219), [heart](../../../areas/story/new_cavecity.story.json#L230), [orb](../../../areas/story/new_cavecity.story.json#L241)). The reviewed source owners are [Heavens tablet #402](../../../areas/obj/heavens.obj#L4199), [Bctdl heart #32490](../../../areas/obj/bctdl.obj#L800), [Negative Plane orb #26614](../../../areas/obj/negplane.obj#L198), and [Heavens scroll #411](../../../areas/obj/heavens.obj#L4315).

The local reset file also imports Heavens object #359, White Harbor object #55182, and Unique object #67272 ([reset rows 170–172](../../../areas/zon/new_cavecity.zon#L170); [#359](../../../areas/obj/heavens.obj#L3725), [#55182](../../../areas/obj/wh.obj#L2352), [#67272](../../../areas/obj/unique.obj#L1118)). These are source references to externally owned prototypes and remain distinct from local definitions.

One active external reset source owns the wizard’s placement: Twin Towers Forest resets mobile #15120 into New Cave city room #15141, then resets local object #15119 in four G rows ([Twin Towers reset rows 392–396](../../../areas/zon/twin_towers_forest.zon#L392)). Preserve this source-zone ownership and the associated recurring stock.

Literal compiled references to New Cave city VNUMs include the Epic Intelligence teacher binding for #15120 ([epic_skills.c line 212](../../../src/classes/epic_skills.c#L212)), the phantom-room list entry #15100 ([innates.c line 2012](../../../src/classes/innates.c#L2012)), Chaos equipment entries for object #15119 ([chaos_eq_data.h lines 578–579](../../../src/account/chaos_eq_data.h#L578)), random-equipment data and special assignment uses of mobile #15113 ([randomeq.c line 150](../../../src/item/randomeq.c#L150), [specs.assign.c line 1023](../../../src/specs/specs.assign.c#L1023)), mobile #15125 ([specs.assign.c line 1024](../../../src/specs/specs.assign.c#L1024)), and object #15116 ([specs.assign.c line 1889](../../../src/specs/specs.assign.c#L1889)). Retain these exact bindings during any separately authorized path proposal.

The bounded exact-reference scan found no other external QST item command, SHP product, active or unregistered ZON reset-role reference, or external story-sidecar typed VNUM use of New Cave city prototypes; all 231 external story sidecars parsed without errors. No external mobile/object prototype duplicates were found. The scan of 872 external type-25 declarations found no destination into a New Cave city room. The zone has no local SHP file.

## Title and bounded history

The normalized .zon title “New Cave city” is unique among 442 scanned current .zon files ([title source](../../../areas/zon/new_cavecity.zon#L2)). Followed new_cavecity.wld history remains on the same stem through the 2009 areas-tree move, 2009 entrance/dimension-target edits, and a 2013 same-stem change. Followed new_cavecity.zon history reaches the same 2009 move, same-stem 2011 and 2013 edits, and the 2015 quest-item-flag pass. This bounded WLD/ZON review found no alternate-stem rename, distinct predecessor, or whole-zone replacement. The source-only Surf approach is route evidence, not replacement evidence.

## Migration disposition

Keep active new_cavecity **DO NOT MOVE**. Preserve its three reciprocal Surface/Underdark boundaries, the source-only Surf approach, the local recipe’s Heavens/Bctdl/Negative Plane inputs and Heavens reward, its local resets importing Heavens/White Harbor/Unique objects, Twin Towers Forest’s wizard reset ownership, and the compiled bindings above. Keep surf and surface2011 **RETAIN IN PLACE** with candidate row 97. See [New Cave city coverage row 201](ZONE_COVERAGE_REGISTER.csv#L201), [Twin Towers Forest row 320](ZONE_COVERAGE_REGISTER.csv#L320), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). No migration clearance is implied.


## Checkpoint 303: all-442 title and followed source history

The normalized title “New Cave city” is unique among all 442 current .zon files ([title source](../../../areas/zon/new_cavecity.zon#L2)). This confirms the CP179 exact-title result against the current file set.

Followed current paths remain on the new_cavecity stem. The WLD history reaches the 2009 areas-tree move and same-stem entrance/dimension-target edits in 2009, with later edits through 2013. The MOB history includes reviewed same-stem edits through August 2026; OBJ history through 2014; ZON history through 2015; and QST history remains on the same path from the 2009 move. The schema-three story sidecar was added October 7, 2026. The current source inventory has no SHP file. No alternate-stem predecessor, tracked rename, or explicit whole-zone replacement claim was found in these bounded paths. Preserve the existing source-only Surf approach with candidate row 97 and the three active reciprocal boundaries.

## Checkpoint 303: twenty direct builder questions and recommendations

The questions below turn established dossier findings into direct owner decisions. Recommendations preserve each source owner and current exact recipe until the owner selects a documented change.

1. **Should wizard #15120’s Q16 remain one atomic offer of tablet #402, Bel’s heart #32490, and orb #26614?** Recommendation: preserve one three-material completion and award only after the complete exact recipe is accepted.
2. **Where should mystic tablet #402 come from if it is intended as a player-supplied material?** Recommendation: identify a real authorized producer, cost, rarity, controller, and renewal before claiming availability.
3. **Does the tablet clue’s request for another artifact describe the intended recipe?** Recommendation: have the owner choose copy-versus-recipe intent and keep any data change in a separately named fix/news change.
4. **Should exact supplied materials remain valid without personal boss kills or first-recovery history?** Recommendation: preserve the present item-based proof and record personal recovery or encounters only as separately authored optional stories.
5. **Should scroll #411 grant a useful intelligence benefit, given its TRASH type and zero value fields?** Recommendation: confirm its actual intended use and useful saved effect before changing its item type or adding training credit.
6. **Should Wizard #15120 continue to spawn through Twin Towers Forest’s reset?** Recommendation: keep that cross-zone source owner and do not add a duplicate local spawn to make the giver appear in local reset counts.
7. **Are Twin Towers Forest’s four G rows for potion #15119 the intended recurring stock?** Recommendation: preserve its reset parent, caps, and controller and qualify admitted issuance before promising an unlimited supply.
8. **Should the epic-intelligence lesson remain separate from the Q16 material exchange?** Recommendation: preserve the existing lesson authority and qualify original coin/epic settlement, refusal, saved learning, and replay before adding any training milestone.
9. **Is undeadfish #15122’s ACT_TEACHER flag deliberate?** Recommendation: confirm the flag’s historical service intent and effective fallback before adding a fish teacher or changing the mobile.
10. **Should #411 remain a direct exchange reward without an automatic READ or RECITE service?** Recommendation: keep the accepted scroll receipt distinct from any later useful effect or transformation.
11. **Do the sidecar’s three optional materials represent readiness only, with no separate reward for each?** Recommendation: preserve the one accepted native exchange and ensure partial, surplus, equipped, and wrong items produce no prefix completion.
12. **Should the nine shared-material stat exchanges and alternate Zariel/Sodolum contracts remain independent choices?** Recommendation: preserve their exact recipients, rewards, and foreign ownership; one consumed material set cannot complete every competing recipe.
13. **Are the three reciprocal Underdark/Surface boundaries intended as the active ordinary entrance and return set?** Recommendation: preserve all three authored pairs and qualify each route and surviving return before changing topology.
14. **Should the inactive Surf approach #277907 remain source-only on candidate row 97?** Recommendation: retain the existing candidate record and avoid describing it as active ingress.
15. **Should the guardian’s eastward prose change to match its actual northward locked route?** Recommendation: confirm copy-versus-topology intent and preserve exact adjacency and lock state until that choice is made.
16. **Do the legacy M0 tails at #15115 and #15143 represent intended timed behavior?** Recommendation: confirm their historical meaning before a separate loader or reset change; do not infer a timer from skipped commands.
17. **Should the mine bag and key reset retain the current global-parent selection and raw weight?** Recommendation: confirm intended parent, cap, and weight before any separately scoped balance or reset repair.
18. **Should Akabar and Ordack’s curse, library, and victim history remain environmental lore or support a restoration outcome?** Recommendation: define recipient consent, controlling authority, useful saved benefit, and episode persistence before adding a quest.
19. **Should Boatkeeper #15124 provide a real skiff service despite having no local SHP record?** Recommendation: choose an authorized price and service controller and qualify purchase, custody, travel, and return before describing a live sale.
20. **How should skiff weight, lake sectors, NO_MOB corridors, and Akulab’s travel restrictions shape a safe route?** Recommendation: qualify actor admission, boat/load state, combat, arrival, and return together while preserving current travel and PvP protections.

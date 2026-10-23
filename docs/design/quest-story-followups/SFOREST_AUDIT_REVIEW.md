# Arbre's Forest repository-wide source audit

This supplement completes the static story, connectivity, title/history and
typed-dependency review for active area `sforest`, coverage row 264. Source
records do not certify installed access, safety, or a player-completed quest.
The narrative map is in the [Arbre's Forest dossier](../zone-stories/SFOREST.md).

## Checkpoint 352: Arbre's Forest reconciliation and builder decisions

### Active story and source inventory

The active [`areas/AREA`](../../../areas/AREA#L350) contributor `sforest` is
headed `#974`, titled “Arbre's Forest,” and contains 24 WLD rooms, seven
mobiles, six objects and 23 ZON reset rows (17 M, five E and one O). Reset
mode is 2. [`areas/qst/AREA`](../../../areas/qst/AREA#L309) lists
`sforest *974`, but no local QST, SHP or
story JSON file exists. The WLD and mobile prose tell an environmental story
of a wet forest, the druid Arbre, hermit Istbanlor, a lost merchant and Valnar
the renegade rogue. There is no native quest or completion record for these
leads.

The reset placements give these leads concrete identities: Arbre and his
tree amulet/leaf mask are placed in the grove; Istbanlor, the wooden hut and
moldy eyepatch are placed in the bog; the lost merchant and leather coat are
placed on the path; Valnar and the black dagger are placed beneath the bridge.
The grove's cottage is described as leaves and sticks, while the wooden-hut
object is reset at the separate bog hut. Room prose names two contribution
boxes on the bridge, but no local box object or reset exists.

### Active connectivity

The active WLD parses to 24 rooms and 48/48 loader-resolved exits. All 48
directions are local or form the two reciprocal Surface boundaries; there
are no unresolved targets, flagged exits, door resets, falls or currents.
All 24 rooms form one weak component. Raw directed traversal from #97400
reaches all 24 rooms, and all 24 have a return path to #97400.

| Active pair | Forest side | Surface side | Classification |
| --- | --- | --- | --- |
| Northern forest path | [#97400 north → #559312](../../../areas/wld/sforest.wld#L15) | [#559312 south → #97400](../../../areas/wld/surface.wld#L1245127) | Reciprocal pair |
| Southern village approach | [#97423 south → #560911](../../../areas/wld/sforest.wld#L460) | [#560911 north → #97423](../../../areas/wld/surface.wld#L1278662) | Reciprocal pair |

The complete sweep of all 95 WLD files outside `areas/AREA` parsed 527,791
rooms and 2,080,773 exits with no parse errors. It found no source-room VNUM
overlap with SForest and two distinct inactive approaches:

| Existing candidate | Inactive source-only edge | Source location | Result |
| --- | --- | --- | --- |
| Surf, candidate row 97 | `surf.wld` #208170 south → #97400 | [room and exit](../../../areas/wld/surf.wld#L171506) | Not active ingress |
| Surf, candidate row 97 | `surf.wld` #209066 east → #97423 | [room and exit](../../../areas/wld/surf.wld#L190290) | Not active ingress |

Preserve both records on existing Surf candidate row 97. Its reviewed
source-only subset rises from 265 to 267; the curated total rises from 4,502
to 4,504. Surf's 145 active references are unchanged. Do not add a new
candidate row or treat these inactive routes as live access.

### Title, duplicate identity and followed history

After color-code removal, trimming, whitespace normalization and case-folding,
“Arbre's Forest” occurs once among 442 current `.zon` titles and once among
351 active contributors. Whole-source same-type scans found no room, mobile
or object VNUM overlap with any active or source-only area. The two similar
inactive Surf approaches have distinct source rooms and do not overlap the
active forest.

Followed WLD and OBJ histories remain on the `sforest` stem through same-stem
edits to 2013-11-13; MOB history continues to 2015-10-13 and ZON history to
2015-10-13. The paths reach the 2009 area-tree move. There is no local QST
history or story sidecar, and no alternate-stem predecessor or whole-zone
replacement was identified in the bounded followed histories. Keep active
`sforest` DO NOT MOVE; Forest title resemblance alone is not replacement
evidence.

### Typed dependencies and compiled hooks

All active ZON consumers and imports, active QST item commands, SHP products,
story JSON numeric references and all 872 type-25 object destinations were scanned against
local rooms #97400–#97423, mobiles #97400–#97406 and objects #97400–#97405.
No external typed ZON/QST/SHP/story-JSON reference or foreign type-25 target
into SForest was found. The local ZON has no external-prototype reset. The
bounded compiled source scan found no literal SForest prototype binding.
The two active Surface WLD pairs and two inactive Surf routes are the only
identified cross-area map dependencies in this review.

### Builder questions and recommended decisions

| # | Source question | Recommended decision before any repair or new story tracking |
| ---: | --- | --- |
| 1 | Are both Surface connections intended as ordinary, reciprocal public approaches to the forest? | Preserve both authored pairs; verify actual access and a useful safe return separately. |
| 2 | Are the two inactive `surf.wld` approaches deliberate alternate map routes or historical remnants? | Keep them source-only on Surf row 97 until that map's owner resolves identity and activation. |
| 3 | Is the warm, damp forest meant to remain an environmental vignette without a QST chain? | Keep it descriptive unless an owner supplies an intended giver, accepted action and outcome. |
| 4 | Should Arbre have a dialogue or interaction beyond the current druid/grove prose? | Do not invent a quest from his prototype description; record the intended exchange first. |
| 5 | Are the bridge's “Contribution Box” signs meant to refer to usable objects? | Confirm whether boxes should exist on both banks; current local objects and resets contain none. |
| 6 | Is Arbre's leaf-and-stick cottage intentionally prose-only while the wooden-hut object belongs to the bog? | Confirm two distinct shelters or specify the intended object placement before moving it. |
| 7 | Is Istbanlor's boat intended as scenery or as a travel/fishing lead? | Keep the hut and boat prose separate from transport until a controller and return path are specified. |
| 8 | Is the lost merchant meant to have a recoverable story or remain an ambient worried traveler? | Confirm intended dialogue, aid and accepted result before adding quest tracking. |
| 9 | Is Valnar only hiding from authorities, or should his rogue story have an actionable resolution? | Preserve his current under-bridge placement and dagger; define any escape or surrender state first. |
| 10 | Are the tree-shaped amulet and leaf mask intentional equipment on Arbre? | Confirm whether they are possessions, clues or rewards before changing their native reset rows. |
| 11 | Does the merchant's dark leather coat intentionally load on the lost merchant? | Preserve the current carrier and distinguish item custody from a rescue outcome. |
| 12 | Does the moldy eyepatch intentionally load on Istbanlor rather than the merchant? | Confirm its flavor/ownership role before any item reassignment. |
| 13 | Does Valnar intentionally carry the black dagger beneath the bridge? | Preserve its source placement; do not infer an arrest or combat objective from the title “renegade.” |
| 14 | Are thorny-vine mobiles intended to physically block travel? | Their descriptions say they block the path, but WLD exits are unflagged; confirm any controller before changing exits. |
| 15 | Is room #97408's flooded dip meant to impose a mechanical movement hazard? | It has ordinary WLD exits and no local fall/current/door reset; retain prose-only treatment until a hazard is specified. |
| 16 | Are the pollen, fungi, insects and wildlife intended as atmosphere only? | Do not add effects, quests or traps from descriptive ecology alone. |
| 17 | Is the clear stream under the bridge meant to support fishing beyond room prose? | Confirm a native fishing action and allowed location before adding functionality. |
| 18 | Do the two inactive routes target the intended forest ends? | Keep the exact sources #208170→#97400 and #209066→#97423 separate from the active Surface pairs. |
| 19 | Is ZON reset mode 2 and the present 17 mobile/five equipment/one object reset set intentional? | Preserve reset values until the builder confirms intended respawn behavior and placements. |
| 20 | Should prose typos and inconsistent shelter descriptions receive a separate copy pass? | Gather owner-approved before/after text; do not silently alter legacy descriptions during mapping. |

### Disposition

Keep active `sforest` DO NOT MOVE. Preserve both reciprocal Surface
boundaries. Add the two inactive Surf edges only to existing candidate row
97; Surf rises to 267 reviewed source-only records and curated totals to
4,504. No source prototype collision, typed foreign dependency, whole-zone
replacement or migration clearance is supported by this checkpoint.

See [coverage row 264](ZONE_COVERAGE_REGISTER.csv#L264), the
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-352-arbres-forest-reset-aware-connectivity-and-inactive-map-identity),
[title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-352-arbres-forest-title-vnum-identity-and-followed-history),
and [retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-352-arbres-forest-in-place-retention-and-candidate-approaches).

# The Bandit Canyons (Priority 215) repository-wide audit review

This review supplements the [Bandit Canyons source dossier](../zone-stories/BANDIT_CANYONS.md) with the active and inactive WLD sweep, typed cross-area references, compiled numeric review, title/history review, and migration disposition. These static records do not establish installed item publication, access, or player qualification.

## Active map, boundary, and directed reachability

The Bandit Canyons (`bandit`) has 76 rooms (#98500–#98575) and 163/163 loader-resolving exits: 162 local and one outbound. The local graph has one weak component. Raw directed traversal from root #98500 reaches 75/76 rooms and has return paths from all 76; #98575 is not reached from the root. The sole active boundary is reciprocal: Bandit #98500 south to Surface #536467 and Surface #536467 north back to #98500. See [Bandit root](../../../areas/wld/bandit.wld#L1), [Bandit outbound exit](../../../areas/wld/bandit.wld#L9), [unreached room #98575](../../../areas/wld/bandit.wld#L1461), and [Surface return](../../../areas/wld/surface.wld#L765542), exit-register [rows 1177](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1177) and [1404](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1404), and pair-summary [rows 36](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L36) and [466](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L466).

## Inactive WLD sweep

All 95 inactive WLD files were parsed: 527,791 rooms and 2,080,773 exits, with no parse errors or room-VNUM overlap with the active Bandit Canyons. One source-only exit enters the area: inactive `surf.wld` #234839 west to #98500 at [line 730899](../../../areas/wld/surf.wld#L730899). It is not active ingress. Curated source-only totals rise to 3,426 overall and 193 for Surf; `ud_map2` remains 15. Preserve this approach with existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); no candidate row is added.

## Typed item, story, and compiled references

The local Olat QST and story contract accept heart #32490, orb #26614, and tablet #402, then return Strength scroll #403: see [bandit.qst lines 17–20](../../../areas/qst/bandit.qst#L17) and [bandit story lines 407–429](../../../areas/story/bandit.story.json#L407). Their prototypes are [BCTDL heart #32490](../../../areas/obj/bctdl.obj#L800), [Negative Plane orb #26614](../../../areas/obj/negplane.obj#L198), and [Heavens tablet #402](../../../areas/obj/heavens.obj#L4199) and [scroll #403](../../../areas/obj/heavens.obj#L4219). The source dossier’s complete 888-file world/reset and 235-file active QST closure found no #402 tablet reset or active Q reward producer. This bounded negative result does not qualify computed, generated, or installed sources; the accepted QST recipe still requires exactly its three declared inputs.

Alatorin has an active QST `G I 98505` reference and a structured story `item_vnums` entry for the local serrated stone dagger #98505: see [alatorin.qst line 5332](../../../areas/qst/alatorin.qst#L5332), [alatorin story line 2699](../../../areas/story/alatorin.story.json#L2699), and [Bandit object #98505](../../../areas/obj/bandit.obj#L73). All 231 non-local story sidecars parsed without errors; the only external typed match is Alatorin’s #98505 contract. No external SHP product, ZON reset-role target, duplicate mobile/object prototype, or target among 872 external type-25 declarations points into the Bandit Canyons. The area has no local SHP file.

The compiled scan found one direct object/skill table entry: [epic_skills.c line 209](../../../src/classes/epic_skills.c#L209) maps local object #98534 to `SKILL_EPIC_STRENGTH` for Olat. The prototype is the local [spiked crown stone](../../../areas/obj/bandit.obj#L471). The static mapping does not establish current installed use or a saved player effect. No other exact compiled local-VNUM match was found.

## Title and bounded history

The normalized title “The Bandit Canyons” is unique among 442 scanned `.zon` files; `banditca` is the separate Bandit Camp key. See the [title source](../../../areas/zon/bandit.zon#L2). Followed `bandit.wld` history reaches the 2009 areas-tree move, an opaque 2013 same-stem edit, and a 2016 Olat door-keyword change. `bandit.zon` history reaches the 2009 move, 2013 edit, and 2015 quest-item-flag pass. These bounded paths establish no alternate-stem rename, distinct predecessor, or whole-zone replacement.

## Migration disposition

Keep active `bandit` **DO NOT MOVE**. Preserve the reciprocal active Surface pair, the one source-only Surf #234839 west approach, and the exact incoming Alatorin dagger reference. Keep Surf RETAIN IN PLACE with candidate row 97. See [Bandit coverage row 23](ZONE_COVERAGE_REGISTER.csv#L23), [Surface coverage row 286](ZONE_COVERAGE_REGISTER.csv#L286), and the [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

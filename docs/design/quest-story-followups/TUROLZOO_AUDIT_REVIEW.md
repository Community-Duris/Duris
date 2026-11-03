# Turolopolis Zoo (Priority 211) repository-wide audit review

This review supplements the [Turolopolis Zoo source dossier](../zone-stories/TUROLZOO.md) with the active and inactive WLD sweep, typed cross-area references, compiled numeric matches, bounded title/history review, and migration disposition. These are static source findings and do not qualify installed access or player outcomes.

## Active map and Willem boundary

Turolopolis Zoo contains 62 rooms, #53000–#53061, and 141/141 loader-resolving exits: 140 local and one outbound. The map is one weak component. Raw directed traversal from #53000 reaches and returns from all 62 rooms when authored exits are considered open. The main route to gorilla cage #53021 uses the closed/secret/blocked #53019 down exit (raw flags 13); with that exit unavailable, root reach is 60/62 while all 62 rooms still have a path back. See [turolzoo.wld line 370](../../../areas/wld/turolzoo.wld#L370).

The only active boundary is reciprocal with the Ruins of Turolopolis (`willem`): Willem #7123 west reaches Zoo #53000, and Zoo #53000 east returns to Willem #7123. There is one active inbound and one outbound, both reciprocating. See [Willem source](../../../areas/wld/willem.wld#L580), [Zoo source](../../../areas/wld/turolzoo.wld#L11), exit-register [row 132](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L132) and [row 656](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L656), and pair-summary [rows 682](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L682) and [828](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L828).

## Inactive WLD sweep

All 95 inactive WLD files were parsed: 527,791 rooms and 2,080,773 exits, with no parse errors. The sweep finds no room overlap and no inactive exit into the Zoo. Curated source-only totals remain 3,405 overall, 173 for Surf, and 15 for `ud_map2`. No candidate row is added or changed for the Zoo.

## Typed references and numeric false positives

No external QST item command, SHP product, story `item_vnums` reference, ZON reset-role target, duplicate external mobile/object prototype, or external type-25 portal target was found. None of the 872 external type-25 declarations targets a Zoo room. The Zoo has no local SHP file, and its own ZON resets do not import an external prototype.

The raw QST/story match to #53000 is Tower currency, not a Zoo object dependency: [tower.qst line 27](../../../areas/qst/tower.qst#L27) uses `R C 53000`, and the matching Tower story completion keys at [line 163](../../../areas/story/tower.story.json#L163) and [line 207](../../../areas/story/tower.story.json#L207) encode `receive=C:53000`. A raw compiled-code match to #53001 is also not a prototype reference: [specs.verzanan.c line 48](../../../src/specs/specs.verzanan.c#L48) embeds the digits in Piergeiron's movement-script string, whose parser passes individual direction digits to `exitnumb_to_cmd` at [line 316](../../../src/specs/specs.verzanan.c#L316). No direct compiled Zoo prototype binding was identified.

## Title and bounded history

The normalized title “Turolopolis Zoo” is unique among 442 scanned `.zon` files; the nearby Ruins of Turolopolis (`willem`) is a separate area and title. Followed `turolzoo.wld` history reaches the 2009 areas-tree move and a 2013 same-stem change; `turolzoo.zon` history reaches the same move, a 2013 same-stem change, and the 2015 quest-item flag pass. These bounded histories establish no alternate-stem rename, distinct predecessor, or whole-zone replacement. See the [title source](../../../areas/zon/turolzoo.zon#L2).

## Migration disposition

Keep active `turolzoo` **DO NOT MOVE**. Preserve its reciprocal Willem boundary and current closed-cage behavior pending the source dossier's builder-intent review. The inactive-WLD sweep adds no approach and no candidate row. Tower's #53000 amount is currency, and Piergeiron's #53001 movement string is direction data; neither establishes a Zoo prototype dependency. See [Zoo coverage row 319](ZONE_COVERAGE_REGISTER.csv#L319), [Willem coverage row 347](ZONE_COVERAGE_REGISTER.csv#L347), and the [Turolopolis Zoo source dossier](../zone-stories/TUROLZOO.md).

## Checkpoint 406: explicit builder questions

Physical coverage row 319 now has exactly 20 direct-source builder questions
with conservative recommendations in the [Turolopolis Zoo dossier](../zone-stories/TUROLZOO.md#explicit-builder-questions-and-recommended-resolutions).
They preserve the single native tooth exchange and distinguish its accepted
receipt from source provenance, controller state, departure, and the nine
broader story proposals. The questions cover route-copy discrepancies,
animal-care and habitat outcomes, the collapsed tunnel, reset availability,
and bounded external-reference results. No area or gameplay source changed.

## Checkpoint 761: Zoo and Ruins of Turolopolis identities

A fresh normalized scan of all 442 current `.zon` title records finds exactly
one “Turolopolis Zoo” owner: `turolzoo`, zone 530. The only other current
title containing the shared place name is “The Ruins of Turolopolis,” active
`willem`, zone 71. The two entries and titles are separately keyed in the area
rosters and source files ([Zoo roster](../../../areas/AREA#L226),
[Zoo title](../../../areas/zon/turolzoo.zon#L2),
[Ruins roster](../../../areas/AREA#L40),
[Ruins title](../../../areas/zon/willem.zon#L2)). Their active room ranges
are also disjoint: Zoo #53000–#53061 and Willem #7100–#7253.

The story sources describe neighboring content, not competing map ownership.
The Zoo sidecar identifies its own source area and describes entry from the
ruined city park ([Zoo story](../../../areas/story/turolzoo.story.json#L4));
the Willem sidecar names the Zoo as one of the city's open approaches
([Ruins story](../../../areas/story/willem.story.json#L8)). The active WLD
boundary is reciprocal between Willem #7123 and Zoo #53000
([boundary review](TUROLZOO_AUDIT_REVIEW.md#active-map-and-willem-boundary)).
The Zoo retains its separate local hunter contract, map, and bounded
same-stem WLD/ZON histories. The reviewed histories identify no alternate
stem, distinct predecessor, or whole-zone replacement. The complementary
[Ruins title/history review](TUROLOPOLIS_AUDIT_REVIEW.md#active-title-and-tracked-history-boundary)
likewise classifies the Zoo as adjacent content and closes the semantic
comparison for those reviewed candidates.

Keep `turolzoo` **DO NOT MOVE** and preserve the reciprocal route and current
cage-door behavior. This review adds no attic candidate row and does not
change the existing 20 explicit Zoo builder questions; command-target alias
ownership and installed route qualification remain separate.
## Checkpoint 861: current source boundary and typed closure carried forward

CP861 reconciles the existing source-backed Zoo review against the unchanged pinned area/code snapshot. The active map remains 62 rooms (#53000–#53061) and 141/141 loader-resolving exits (140 local, one outbound) in one weak component. With authored exits considered open, traversal from #53000 reaches and returns from all 62 rooms; with the closed/secret/blocked #53019 down exit unavailable, raw reach is 60/62 and return remains 62/62. Its only active cross-area pair is reciprocal with Willem #7123 west ↔ Zoo #53000 east. These graph calculations do not establish switch/search/open behavior, encounter eligibility, safe travel, or installed player access.

The all-95 inactive-WLD sweep found no room overlap and no inactive exit into the Zoo. The seven Surf approaches documented for Tundra are unrelated; the Zoo adds no source-only approach or attic candidate. The older 3,405 overall / 173 Surf / 15 `ud_map2` source-only subtotal in the original supplement, and 9,480 / 298 in Checkpoint 406, are historical. Current cumulative source-only totals are 11,993 overall / 415 Surf.

The bounded typed-reference scan is complete for direct static sources: no external QST item command, SHP product, story `item_vnums`, ZON reset-role target, duplicate external mobile/object prototype, or external type-25 target into the Zoo was found. The Zoo has no local SHP and its own ZON does not import an external prototype. Tower's raw #53000 references are currency (`R C` / `receive=C`), and compiled #53001 is a direction-digit string in Piergeiron's movement script; neither is a Zoo prototype binding. The reviewed same-stem history and Checkpoint 761 title comparison still identify no alternate owner or whole-zone replacement; Willem remains adjacent content with its own title, stem, and map.

Keep active `turolzoo` **DO NOT MOVE**. Add no attic candidate and grant no migration or activation clearance. Installed traversal, door procedure, encounter/access qualification, persisted state, and indirect/dynamic consumers remain outside this static closure. See the [source dossier](../zone-stories/TUROLZOO.md), [Checkpoint 761 title review](#checkpoint-761-zoo-and-ruins-of-turolopolis-identities), and [coverage row 319](ZONE_COVERAGE_REGISTER.csv#L319).

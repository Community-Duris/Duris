# The Scorched Valley: connectivity, typed dependencies, history, and builder decisions

**Checkpoint:** 347, coverage register row 259. **Disposition:** DO NOT MOVE. No gameplay, zone, or migration change is proposed.

This supplement reconciles the [comprehensive Scorched Valley source story map](../zone-stories/THE_SCORCHED_VALLEY.md) and [generated source review index](../../reference/zone-story-audits/scorchvalley.md) with active connectivity, reset-aware reach, inactive map approaches, typed item ownership, compiled hooks, title/history, and builder decisions. It uses checked-in source only. This checkpoint did not run builds, tests, live travel, door interactions, item recovery, encounter, or effect journeys.

## Source story and authored scope

The active `scorchvalley` contributor is listed in `areas/AREA` and `areas/qst/AREA`. Its active files define 132 rooms, 60 mobiles, 55 objects, six QST giver VNUMs, nine exact native contracts, and 211 reset commands across 118 source families: 114 M, 31 F, 22 E, 15 G, 14 D, nine O, and six P. The native source review index maps nine Q and eleven addressed mobile dialogue families, plus three literal special assignments. There is no local `.shp` file; the zero shop count in [coverage row 259](ZONE_COVERAGE_REGISTER.csv#L259) is correct.

All nine native offerings are item-only and all nine recipients remain after acceptance (`disappear=0`). The existing story map follows the independent ring, three-head, sack, essence, banner/hide, charm, godly-magic, blood/rings, and wildmage outcomes; temple keys and rifts; foreign encounters; the unimplemented Rod of Seven Parts ending; and Yeenoghu's disconnected procedure. Zone reset mode is one, and all 211 listed commands have 100% chances. Those source declarations do not guarantee admission, stock, NPC arrival, safe access, or a particular reset schedule.

The local three-head contract imports two exact foreign objects: Torg's former dwarven lord head #28980 and Grog's head #71009 from Fields Between, together with local commander head #71227, for output #71226. The [native terms](../../reference/zone-story-audits/scorchvalley.md#native-exchanges) and the typed owners ([Torg object](../../../areas/obj/torg.obj#L1232), [Fields Between object](../../../areas/obj/fields_between.obj#L117), [local head](../../../areas/obj/scorchvalley.obj#L394)) should remain coordinated if any prototype changes.

## Active connectivity and reset-aware reach

The active WLD has 132 rooms and 354 exit records: 350 local and four outbound. All 354 resolve in the active loader. The local directed map has four weak components: one of 129 rooms and three isolated one-room helper destinations, #71326, #71330, and #71331.

| Active boundary | Reverse record | Classification | Evidence |
| --- | --- | --- | --- |
| Surface #576166 west → Scorched Valley #71201 east | Reciprocal | Only ordinary reciprocal boundary | [pair row 419](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L419); [Surface inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1503); [Scorch outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L851) |
| Winterhaven #55634 south → #71321 | None | One-way load/helper inbound | [exit row 799](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L799) |
| Fields Between #71152 down → #71265 | None | One-way load/helper inbound | [exit row 850](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L850) |
| Juiblex #87674 down → #71221 | None | One-way load/helper inbound | [exit row 1090](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1090) |
| #71326 down → Mount #9122 | None | One-way outbound helper room | [exit row 852](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L852) |
| #71330 down → Torg #29071 | None | One-way outbound helper room | [exit row 853](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L853) |
| #71331 down → Fields Between #71140 | None | One-way outbound helper room | [exit row 854](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L854) |

The three inbound helper paths and three outbound destinations match the source descriptions for mobile loading and dispersal. They do not add reciprocal routes for ordinary player travel or transfer the owning journal. The isolated outbound rooms are not connected to the 129-room local component by authored local exits.

| Arrival root | Raw reach / return | After conservatively excluding 100%-chance reset-closed directions: reach / return |
| --- | ---: | ---: |
| Surface #576166 → #71201 | 88 / 88 | 3 / 3 |
| Juiblex #87674 → #71221 | 88 / 88 | 66 / 66 |
| Fields Between #71152 → #71265 | 88 / 88 | 66 / 66 |
| Winterhaven #55634 → #71321 | 37 / 37 | 37 / 37 |

All 14 door resets are at 100% and form seven reciprocal pairs: four state-2 closed/locked pairs, two state-1 closed pairs, and one state-5 closed/secret pair. The conservative graph removes each reset-controlled direction, even where a key or another authored bypass could later make it traversable. The exact pairs are:

| Pair | Reset state | Source |
| --- | --- | --- |
| #71203 north ↔ #71204 south, shield | Closed/locked, state 2 | [ZON line 8](../../../areas/zon/scorchvalley.zon#L8) |
| #71240 down ↔ #71241 up, barricade basement | Closed, state 1 | [ZON line 16](../../../areas/zon/scorchvalley.zon#L16) |
| #71263 south ↔ #71268 north, village house | Closed, state 1 | [ZON line 24](../../../areas/zon/scorchvalley.zon#L24) |
| #71265 down ↔ #71273 up, stable/caves | Closed/secret, state 5 | [ZON line 32](../../../areas/zon/scorchvalley.zon#L32) |
| #71267 west ↔ #71270 east, temple pit | Closed/locked, state 2 | [ZON line 40](../../../areas/zon/scorchvalley.zon#L40) |
| #71279 north ↔ #71280 south, inner sanctum | Closed/locked, state 2 | [ZON line 48](../../../areas/zon/scorchvalley.zon#L48) |
| #71281 north ↔ #71282 south, vault | Closed/locked, state 2 | [ZON line 56](../../../areas/zon/scorchvalley.zon#L56) |

No room declares a fall chance or current field. These are source-graph reach and return counts, not runtime access, key use, room discovery, NPC loading, combat, or return-safety results. The [full source map](../zone-stories/THE_SCORCHED_VALLEY.md#temple-access-and-sourcerecipient-conflict) documents the relevant keys, locked container, lava-floor sources, and one-way helper-room purposes.

## Inactive WLD approach

The all-95 inactive-WLD scan found one non-overlapping, source-only approach into the active area:

| Inactive source exit | Active target | Source |
| --- | ---: | --- |
| `surf.wld` #280633 west | #71201 | [source record](../../../areas/wld/surf.wld#L1692001) |

No other inactive WLD has an edge into a Scorched Valley room, and no inactive room shares a Scorched Valley room VNUM. Preserve this source-only edge on the existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); it is not loaded ingress. Surf's reviewed source-only subset rises from 262 to 263, and the curated global total rises from 4,498 to 4,499 (Surf 263, `ud_map1` two, `ud_map2` 20, Underworld2 eight). Surf's 145 active references are unchanged.

## Typed dependencies and compiled hooks

The local ZON resets have no external room, mobile, or object imports. The exact QST, shop, and story-sidecar scans found no external consumer of a Scorched Valley object, and there is no local shop file. No external mobile or room reset targets a local Scorched Valley prototype or room. No external type-25 object declaration targets a Scorched Valley room (870 declarations inspected). Same-type room scans across all active and source-only WLDs found no local room-number collision; all-source mobile and object scans found no duplicate definitions. These source scans do not cover persisted or dynamic runtime data.

Compiled typed references preserve these local identities:

| Local prototype | Reference | Evidence and interpretation |
| --- | --- | --- |
| Mob #71223, flaming demon | `block_up` special assignment | [assignment](../../../src/specs/specs.assign.c#L184); it guards the collector's overlook exit. |
| Mob #71259, Yeenoghu | `yeenoghu` special assignment and random-equipment owner slot 30 | [assignment](../../../src/specs/specs.assign.c#L185), [special implementation](../../../src/specs/specs.scorchvalley.c#L71), [equipment table](../../../src/item/randomeq.c#L145). The handler rejects setup and nonzero commands, so the authored boss actions have no normal dispatch path. |
| Object #71231, wisp artifact | `artifact_invisible` object procedure | [assignment](../../../src/specs/specs.assign.c#L1406); the source dossier records its say-triggered effect and cooldown, not a story-completion terminal. |
| Object #71254, Yeenoghu's spiked helm | Eight CHAOS equipment profiles | [standard profile example](../../../src/account/chaos_eq_data.h#L132) and [additional profile](../../../src/account/chaos_eq_data.h#L243); it is an equipment grant, not a quest receipt. |

No other exact Scorched Valley prototype lookup was found in C/C++ source. Preserve the item/mobile/room types where their numeric VNUMs coincide; the compiled references are typed by their actual source fields.

## Title, duplicates, and followed history

After color-code removal, trimming, whitespace normalization, and case-folding, “The Scorched Valley” occurs once among all 442 current `.zon` titles and once among 351 active contributors. No same-type room, mobile, or object VNUM collision with another active or source-only area was found. The shared `(kolut)` note in the `areas/AREA` entries for Torg, Fields Between, and Scorched Valley is a grouping annotation; it is not by itself a rename or predecessor claim.

Followed WLD, MOB, OBJ, ZON, and QST histories stay on the `scorchvalley` paths from the 2009 area-tree move through later same-stem edits. The area-owned special source is `src/specs/specs.scorchvalley.c`. The story sidecar was added on 2026-10-03. No alternate-stem predecessor or explicit whole-zone replacement was identified. Keep active `scorchvalley` DO NOT MOVE.

## Builder questions and recommended resolutions

1. **Is #71201 east to Surface #576166 the intended ordinary player entrance?** The raw local graph reaches and returns from 88 rooms, while the conservative closed-door graph reaches and returns from three. **Recommendation:** preserve the reciprocal boundary and qualify access through the shield before presenting the raw count as an ordinary route.
2. **Are the Winterhaven, Fields Between, and Juiblex inbounds intended only for loading helper mobiles?** They terminate at #71321, #71265, and #71221 without direct reverse records. **Recommendation:** retain their load-room classification and keep them separate from public-arrival and contract-ownership claims.
3. **Are #71326, #71330, and #71331 intended as one-way load/dispersal exits to Mount, Torg, and Fields Between?** They are isolated single-room components. **Recommendation:** preserve them for their documented callers until each destination owner confirms a public route or replacement.
4. **Should the three one-room components be described as player-accessible content?** Their only authored exits leave the area. **Recommendation:** keep those rooms classified as helper destinations unless a separate player path is authored and qualified.
5. **Is the shield's state-2 reset and sharp shard #71201 source on the mostly formed golem the intended entry gate?** **Recommendation:** preserve the exact key and paired reset; qualify ordinary key acquisition and both-side behavior before revising directions.
6. **Are the closed barricade/basement and destroyed-village/home pairs intentionally state 1?** **Recommendation:** confirm which prose and routes imply opening each door, then align both paired resets if their shared access behavior should differ.
7. **Is the state-5 stable-to-cave pair intended to remain secret and closed while Fields Between's helper lands in #71265?** **Recommendation:** separate the load-room purpose from player discovery and verify the hidden door from both sides before changing either reset.
8. **Do the temple-pit and inner-sanctum locks use their exact distinct key sources?** The dossier maps burning-hot key #71206 and abyssal key #71208 to different dragon carriers. **Recommendation:** preserve those typed keys and qualify acquisition, pickproof flags, and each return edge separately.
9. **Is the vault key distinct from the advisor's final chest key?** The vault key #71236 and chest key #71237 have different holders and locks. **Recommendation:** keep the two identities separate and validate the final container's nested inventory and opening path.
10. **Should the demonic advisor remain the shared key holder and recipient for blood, sack, heads, and essence?** Killing him for a key can remove the same QST recipient. **Recommendation:** preserve native terms and availability; make any peaceful key path or retiring-recipient behavior an explicit builder decision.
11. **How should foreign encounter hints route when Scorched Valley owns the contract but the encountered NPC is physically in Mount, Torg, or Fields Between?** **Recommendation:** keep the physical-area discovery guard, point to an already discovered owning journal or give honest referral text, and never create remote discovery or move the completion.
12. **Should Yeenoghu's assigned procedure be restored to periodic or combat dispatch?** It returns false for setup and rejects nonzero commands. **Recommendation:** choose one intended cadence in a separate gameplay change and qualify setup, null/dead actors, healing, attacks, and accounting before claiming the boss behavior works.
13. **Is the whirlwind loop safe to enable around `insectbite` callbacks?** The current loop does not revalidate every participant after damage callbacks. **Recommendation:** resolve actor/target continuation and room traversal safety before enabling or changing the boss special.
14. **Should Tallin's advisor say either commander's head will work when the accepted recipe requires local head #71227 and the other listed head is a different proof?** **Recommendation:** correct the clue or explicitly author and qualify a distinct alternative; do not silently substitute the godly-magic request.
15. **Should the council advisor's departure text stay even though its accepted contract has `disappear=0`?** **Recommendation:** align text with a recipient who remains, or explicitly approve a retiring recipient and repeatability policy before changing the binding.
16. **Are the captive, bodyguard return, society rank, and curse-resolution passages intended as lore only?** Their checked items do not create those world states. **Recommendation:** retain the descriptive classification unless builders specify supported entities/effects, accepted events, failure recovery, and durable outcomes.
17. **Is object #71215 intended only as the Rod of Seven Parts artifact, without an active assembly or battle finale?** No active producer, assembly, or terminal was found. **Recommendation:** preserve it as lore/unused equipment until a complete, source-backed integration and all-stage policy is selected.
18. **Do cap-one bosses, followers, and holding-room dispersal provide adequate proof stock and encounter availability?** Reset chances alone do not guarantee access or a schedule. **Recommendation:** qualify actual reset, movement, replacement, and recovery before describing these routes as reliably available.
19. **Should journal completion record entering either dimensional rift or receiving the wisp's invisible effect?** The two rifts teleport, while the wisp effect is temporary and cooldown-limited. **Recommendation:** keep item possession, rift observation, successful arrival/survival, and effect delivery as separate facts; add credit only for a chosen durable event.
20. **How should the 46 advisor aliases fit the 32-topic contact limit, and should souvenir-sale dialogue be updated despite no local shop file?** **Recommendation:** retain the curated useful aliases and identify the abandoned cart as the current charm source; treat the repeated `(kolut)` note as metadata, not a new alias or zone identity.

## Safe disposition

Keep active `scorchvalley` in place. Preserve the Surface pair, three inbound and three outbound helper routes, seven door-reset pairs, Torg/Fields Between head owners, and typed compiled references. Preserve inactive `surf.wld` #280633 west to #71201 on existing Surf candidate row 97 as source-only map evidence. No gameplay adjustment, new attic candidate, or migration clearance follows from this source audit.

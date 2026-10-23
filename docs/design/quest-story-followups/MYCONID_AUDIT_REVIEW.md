# Myconid Mushroom Forest repository-wide audit

This supplement reconciles the active map, inactive-WLD approaches, typed source dependencies, compiled room destinations, normalized title, and bounded same-stem history with the [source-comprehensive Myconid dossier](../zone-stories/MYCONID.md). It uses the pinned native snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`. No area or gameplay source is changed by this audit.

## Active map and reciprocal UnderDark boundaries

The active [Myconid WLD](../../../areas/wld/myconid.wld#L1) contains 93 rooms (#2300–#2392) and 224 authored exits. All exits resolve: 222 remain within Myconid and two leave the area. The local map is one weak component. Raw directed traversal from #2300 reaches and returns from 92/93 rooms when flag-8 exits are unavailable, or 93/93 when those exits are assumed available. That static result does not establish admission, switch operation, or a player's route.

Two active boundaries reciprocate with UnderDark. Myconid [#2300 goes up](../../../areas/wld/myconid.wld#L12) to #836064 and UnderDark returns down; Myconid [#2392 goes down](../../../areas/wld/myconid.wld#L1806) to #811016 and UnderDark returns up. The two pairs are recorded in [cross-area pair rows 311 and 723](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L311). The internal #2374 east ↔ #2392 west links carry flag 8. There are no negative or unresolved targets, falls, or currents.

## Inactive source approaches

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without parser issues or room-VNUM overlap with Myconid. It found two non-overlapping source-only approaches to entrance #2300:

- [ud_map0.wld #1102287](../../../areas/wld/ud_map0.wld#L7266), down to #2300.
- [ud_map2.wld #1124242](../../../areas/wld/ud_map2.wld#L48876), down to #2300.

These inactive source records are not active ingress. Preserve them with `ud_map0` and `ud_map2`. The curated source-only total rises from 3,225 to 3,227 overall; the reviewed `ud_map0` subset rises from one to two and `ud_map2` from 13 to 14. Surf remains at 159 reviewed records.

## Typed dependencies and compiled destinations

Myconid is listed in the active [world manifest](../../../areas/AREA#L11) and [QST index](../../../areas/qst/AREA#L11). Its local [QST](../../../areas/qst/myconid.qst#L22) imports exact object #88502, defined in [udmini.obj](../../../areas/obj/udmini.obj#L34); the Myconid journal records that same `item_vnums` value. The external item has two registered source declarations: `udmini.zon` produces #88502 from gills #88501, and `newbie.zon` resets #88502 from mobile #29225 ([udmini reset](../../../areas/zon/udmini.zon#L10); [newbie reset](../../../areas/zon/newbie.zon#L362)). These are source declarations, not evidence of generated stock or a played recovery.

No external QST item command, SHP product, story-sidecar `item_vnums` entry, or typed ZON reset target was found for a Myconid mobile or object prototype. Fourteen raw `M` reset rows in unregistered `mushroom_caverns.zon` contain numbers that overlap Myconid room VNUMs; those numbers do not identify Myconid mobiles or objects and are cross-namespace collisions, not typed dependencies. The 231 external story sidecars parsed without errors and had no raw numeric-token matches. The external mobile/object census found no duplicate Myconid prototype definitions. Across the complete 442-object-file census, none of 872 type-25 declarations targets a Myconid room.

Two C/C++ room destinations are genuine compiled references. [`githyanki_room[]`](../../../src/classes/innates.c#L2008) includes #2381; the related Githyanki/Pillithid/Illithid shift code resolves and moves to its selected room ([route selection](../../../src/classes/innates.c#L2036); [movement](../../../src/classes/innates.c#L2070)). [`illithid_teleport_veil`](../../../src/specs/specs.ixarkon.c#L18) can select #2368 and moves the actor to the resolved room ([destination selection](../../../src/specs/specs.ixarkon.c#L38); [movement](../../../src/specs/specs.ixarkon.c#L42)). These code routes make Myconid rooms direct compiled dependencies; static source review does not qualify their runtime predicates or actual player access. Other broad numeric matches in stat/economic constants and an unused random-item macro are not room destinations.

## Title, history, and disposition

The normalized title `Myconid Mushroom Forest` is unique across 442 `.zon` files and among active titles; the active definition is [`myconid.zon`](../../../areas/zon/myconid.zon#L2). Same-stem WLD history reaches the 2009 area-tree move and a 2013 edit. It shows no separate predecessor, replacement stem, or whole-zone replacement.

Keep active `myconid` **DO NOT MOVE**. Retain both source-only approaches with `ud_map0` and `ud_map2`. The imported #88502 story input and the compiled #2381/#2368 travel destinations remain dependencies to preserve. This report records source evidence; it does not claim gameplay qualification or migration clearance.

The source-mapping queue is now 187/220 complete, with 33 pending. Priority 188, Vargan II (`v2`), is next.

Validation: Checkpoint 147 documentation validation checked 116 changed/added Markdown files; all 4,723 local link targets and 2,145 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. `git diff --check` passed, and `areas/` is unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.


## Checkpoint 299 direct builder questions and recommendations

The previous [repository-wide review](MYCONID_AUDIT_REVIEW.md) and [source dossier](../zone-stories/MYCONID.md) already cover the map, two source-only approaches, exact imported spore item and producers, compiled room destinations, and native acceptance boundaries. These questions make the builder choices explicit without changing those source records.

1. **Should Myconid Mushroom Forest remain a separate area from Mushroom Forest and Mushroom Caverns?** Recommendation: retain the three titles and stems as separate identities unless an owner documents a concrete predecessor or shared ownership.
2. **Are both reciprocal UnderDark entrances intended as maintained ways into and out of Myconid?** Recommendation: preserve the #2300/#836064 and #2392/#811016 pairs and qualify actual access and safe return before changing either.
3. **Should the two inactive ud_map0 and ud_map2 approaches remain source-only records?** Recommendation: preserve both candidate records while those stems remain outside the active manifest; do not describe them as active entrances.
4. **Is the blocked #2374 east / #2392 west passage intended to open through the two local mushroom roots?** Recommendation: keep the exact switch roots, target rooms, and directions in the source record; test each selected root and edge before changing its binding.
5. **Should #2392’s copied “back west” prose be corrected to match its ordinary down exit?** Recommendation: confirm the intended landmark wording and update that description only after builder review.
6. **Should the two blocked exits reset closed after D8 and D12?** Recommendation: preserve the current reset declarations unless the owner selects a different shared-open duration and restoration rule.
7. **Does the PULL switch open one shared passage for all occupants, with no personal completion outcome?** Recommendation: keep switch operation, another actor’s open state, crossing, and reset restoration as separate events.
8. **Is exact external object #88502 the intended offering to the alchemist?** Recommendation: retain that typed item requirement and keep it distinct from local glowing spore #2301, mushroom food, gills, roots, and the foreign jar.
9. **Are the udmini and newbie producer declarations both intended sources for #88502?** Recommendation: retain each source’s own cap, parent, conditional construction, and renewal policy; source declarations alone do not promise stock.
10. **Should the udmini gills remain closed and NOSHOW while still permitting keyword lookup and the OPEN-triggered trap?** Recommendation: qualify visibility, opening, trap recovery, and exact GET before changing flags or trap behavior.
11. **Is giant mushroom root #88500 meant to provide travel despite having no bounded literal reset producer?** Recommendation: confirm its generation and entry source before promising route availability or adding a new producer.
12. **Should the foreign stem and cap rooms #88500/#88501 remain a reciprocal route without an ordinary outside return?** Recommendation: test intended magic, actor, and travel access; add no exit based on static topology alone.
13. **Are the alchemist’s two addressed topic families and four aliases intended to make one exchange?** Recommendation: preserve one accepted outcome and track successful addressed responses separately from keyword matches or QA echoes.
14. **Who should receive the 50-platinum reward when eligible party members receive completion?** Recommendation: confirm the native actor-wallet settlement and party history policy before adding per-member currency credit.
15. **Should the king’s green-key and locked potion-cache sequence stay optional and separate from the alchemist request?** Recommendation: keep the cache and its two potion pickups as independent source outcomes; exact supplied spores require no personal king kill.
16. **Does the alchemist’s brewing prose describe flavor, or should a real recipe and potion result exist?** Recommendation: preserve the current single spore exchange until a distinct crafting input, effect, recipient, and outcome are specified.
17. **Should scarlet food continue producing the current eaten penalty and sickness echo rather than an actual poison affliction?** Recommendation: confirm the intended food behavior before touching the legacy TODO path; retain the other foods’ distinct values.
18. **Are the pool’s coded-looking letters, bright ring, and crystal ooze intended as environmental clues or separate objectives?** Recommendation: keep them as separate scenery, item, water, and encounter facts unless the owner defines a distinct mechanic and accepted result.
19. **Should the registry start value #2296 remain even though local rooms begin at #2300?** Recommendation: preserve both fields as authored until the registry owner confirms a schema or range correction.
20. **Should the potential daily remain disabled until exact-source renewal, actor availability, wallet settlement, and active READY acceptance are qualified?** Recommendation: keep daily classification separate from authored repeatability and from a stored receipt or UTC-day change.

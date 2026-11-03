# Sarmiz'Duul: connectivity, typed dependencies, history, and builder decisions

**Checkpoint:** 345, coverage register row 257.  
**Disposition:** DO NOT MOVE. No gameplay, zone, or migration change is proposed.

This supplement reconciles the [source-comprehensive Sarmiz'Duul dossier](../zone-stories/SARMIZ_DUUL.md) and [source review index](../../../docs/reference/zone-story-audits/sarmiz.md) with whole-map reach, reset-aware arrivals, inactive WLD approaches, exact typed dependencies, code hooks, title/history, and explicit builder decisions. It uses checked-in source only. Door opening, scripts, shop operation, account creation, ship service, travel safety, and live availability were not tested.

## Source story and authored scope

The active `sarmiz` contributor is listed in `areas/AREA` and `areas/qst/AREA`. Its existing dossier maps 583 rooms, 57 mobiles, 61 objects, ten QST giver VNUMs, eight native Q/QA contracts, four shops, and 535 reset commands across D/E/F/G/M/O/P families. The [source review index](../../../docs/reference/zone-story-audits/sarmiz.md) records the eight exact exchanges, thirteen raw addressed dialogue families, literal special assignments, and reset-family counts. The longer dossier describes the local courtship, relic, royal, and conspiracy episodes, the foreign Ice Crag ingredients, and Erzul's custom moonstone arc.

Sarmiz has no native Q/QA binding for Erzul's custom moonstone story. The custom assembly and ship-crew path has unresolved transaction and accounting boundaries documented in the existing dossier; this review does not create a completion stage or enable the blocked core source.

## Active connectivity and reset-aware reach

The active WLD contains 583 rooms and 1,389 exit records: 1,386 local and three outbound. All 1,389 resolve in the active loader, and the local directed graph has one weak component of 583 rooms. No room declares a fall chance or current field. The exact active exit census is in [coverage row 257](ZONE_COVERAGE_REGISTER.csv#L257).

| Active boundary | Return record | Census evidence |
| --- | --- | --- |
| Surface #621875 south to Sarmiz #9400 | Sarmiz #9400 north to Surface #621875 | [pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L414); [inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1628); [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L187) |
| Underworld #4404 north to Sarmiz #9853 | Sarmiz #9853 south to Underworld #4404 | [pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L415); [inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L59); [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L188) |
| Surface #622675 north to Sarmiz #9975 | Sarmiz #9975 south to Surface #622675 | [pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L414); [inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1633); [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L189) |
| Storm Port #22541 down to Sarmiz #9597 | No direct reverse record | [pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L455); [exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L369) |
| Winterhaven #55402 west to Sarmiz #9401 | No direct reverse record | [pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L809); [exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L752) |

Every listed root reaches all 583 rooms and has an authored return walk in the raw directed local WLD graph. Applying the 100% ZON door resets conservatively—excluding direction edges reset to states 1 (closed), 2 (closed/locked), or 5 (closed/secret)—reduces mutual reach and return to 249/583 from #9400, 6/583 from #9853, 18/583 from #9975, and 249/583 from each one-way inbound room #9597 and #9401. These are static post-reset graph counts; they do not account for unlocking, picking, discovering hidden exits, scripted transport, reset timing, or movement.

Sarmiz has 50 100%-chance door-reset commands: 43 state-1, four state-2, and three state-5 records. They cover 25 reciprocal exit pairs. Twenty-four pairs use matching states; the #9733 down / #9899 up pair is mixed, with state 5 on the hideout side and state 1 on the reverse side. Relevant reset declarations include [the temple hallway](../../../areas/zon/sarmiz.zon#L104), [hideout and maze door](../../../areas/zon/sarmiz.zon#L120), [the secret pair](../../../areas/zon/sarmiz.zon#L152), and [the throne-room pair](../../../areas/zon/sarmiz.zon#L200). The actual key, unlock, discovery, and player return behavior remain unqualified.

## Inactive Surface approaches

The all-95 inactive-WLD sweep found no room-number overlap with Sarmiz and three non-overlapping, source-only exits from `surf.wld`:

| Inactive source exit | Sarmiz target | Source |
| --- | ---: | --- |
| Surface #277517 south | #9400 | [source room](../../../areas/wld/surf.wld#L1626625) |
| Surface #278416 east | #9975 | [source room](../../../areas/wld/surf.wld#L1645492) |
| Surface #278717 north | #9975 | [source room](../../../areas/wld/surf.wld#L1651801) |

These records are not loaded approaches. Preserve them on existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). The reviewed Surf source-only subset rises from 259 to 262 and curated source-only totals rise from 4,495 to 4,498 (Surf 262, `ud_map1` two, `ud_map2` 20, Underworld2 eight). Surf's 145 active unresolved references are unchanged.

## Typed prototype dependencies

### Local resets and shops importing objects

The local ZON resets and shop product list depend on object prototypes owned outside Sarmiz. Preserve the typed owner when reviewing local placements or moving source files.

| Sarmiz use | Object owner / status | Evidence |
| --- | --- | --- |
| Reset object #72 in room #9965 | Active Heavens; source-only `hostel` and `mavulsk` files also define #72 | [reset](../../../areas/zon/sarmiz.zon#L211); [active object](../../../areas/obj/heavens.obj#L1000) |
| Reset object #3097 in room #9651 | Active Verzanan (`verz1`) | [reset](../../../areas/zon/sarmiz.zon#L209); [object](../../../areas/obj/verz1.obj#L1199) |
| Reset and Learett stock #363, #368, and #369 | Active Heavens | [resets](../../../areas/zon/sarmiz.zon#L377); [shop products](../../../areas/shp/sarmiz.shp#L1); [#363](../../../areas/obj/heavens.obj#L3783), [#368](../../../areas/obj/heavens.obj#L3836), [#369](../../../areas/obj/heavens.obj#L3850) |
| Sior stock reset #1232 | Active Heavens | [reset](../../../areas/zon/sarmiz.zon#L552); [object](../../../areas/obj/heavens.obj#L9665) |

The local ZON also has 100%-chance object resets for #3095, #3096, #6076, and #6018 at Sior's room, but no `areas/obj/*.obj` file defines those object VNUMs. The local mobile #6018 is a different type and does not resolve the object reset. No external mobile reset or external room reset targets a Sarmiz-owned prototype.

### External consumers of Sarmiz objects

Exact typed scans found the following active consumers. Structured sidecar scans parsed all 231 other story files without errors and found three matching `item_vnums` entries, all duplicating the corresponding native item dependency.

| Sarmiz object | External use | Evidence |
| --- | --- | --- |
| #9436, doubling sword of Chaos | Quietus QST gives the sword to the angry orcish lieutenant; `quietus.story.json` records it as an optional carried item. | [Quietus QST](../../../areas/qst/quietus.qst#L151); [story sidecar](../../../areas/story/quietus.story.json#L240); [Sarmiz object](../../../areas/obj/sarmiz.obj#L612) |
| #9440, obsidian-and-steel totem | Gelden's Ship Yards exchange consumes it; `shipy.story.json` lists it as the optional supply. | [Ship Yards QST](../../../areas/qst/shipy.qst#L309); [story sidecar](../../../areas/story/shipy.story.json#L560); [Sarmiz object](../../../areas/obj/sarmiz.obj#L694) |
| #9428, pitch black potion | Ship Yards' Bestile exchange consumes it; Alatorin stocks it and has an object reset for it. `shipy.story.json` lists the same kind. | [Ship Yards QST](../../../areas/qst/shipy.qst#L891); [Alatorin shop](../../../areas/shp/alatorin.shp#L444); [Alatorin reset](../../../areas/zon/alatorin.zon#L3807); [story sidecar](../../../areas/story/shipy.story.json#L1094); [Sarmiz object](../../../areas/obj/sarmiz.obj#L493) |

No same-type room, mobile, or object VNUM collision with another active or source-only WLD/MOB/OBJ file was found. No outside type-25 object declaration targets a Sarmiz room (872 external declarations were inspected). The only exact external QST/SHP/ZON prototype references found are the item uses above; the bounded scan does not cover persisted or dynamic runtime data.

### Compiled hooks and item tables

Reviewed numeric code references as typed source fields or direct prototype lookups, excluding numeric substrings and unrelated constants:

- [Otten #9412](../../../areas/mob/sarmiz.mob#L173) is a smith registered in [`smith_array`](../../../src/economy/tradeskill.c#L209); the smith function binds configured mobile VNUMs at [initialization](../../../src/economy/tradeskill.c#L958).
- [Anders #9445](../../../areas/mob/sarmiz.mob#L628) has `money_changer`, and [Erzul #9453](../../../areas/mob/sarmiz.mob#L739) has `erzul_proc` ([assignments](../../../src/specs/specs.assign.c#L805)). Explicit zero special assignments for #9420, #9432, and #9454 are present at [the corresponding lines](../../../src/specs/specs.assign.c#L466); the #9454 craft-table mention is commented out ([historical entry](../../../src/classes/epic_skills.c#L197)).
- [Room #9704](../../../areas/wld/sarmiz.wld#L6137) binds the Scallywags crew shop, [#9738](../../../areas/wld/sarmiz.wld#L6721) binds the inn, and [#9967](../../../areas/wld/sarmiz.wld#L11115) binds the ship shop ([assignments](../../../src/specs/specs.assign.c#L2394), [inn and ship shop](../../../src/specs/specs.assign.c#L2435)). The port table also identifies Sarmiz with #9967 and Surface destination #622675 ([port record](../../../src/ships/ship_variables.c#L43)); #9704 is in the compiled evil-crew shop list ([table](../../../src/ships/ship_shop.c#L3239)).
- The ferry stop table names Sarmiz at passenger dock #9979 ([stop](../../../src/world/ferryact.c#L120)), and the `sarmiz` chaos-travel alias resolves to gate room #9401 ([alias](../../../src/combat/chaos.c#L31)).
- CHAOS starter equipment profiles contain typed item VNUMs #9438 (belt of skulls), #9447 (staff of power), and #9460 (ring of the legendary hunter) in [the generated kit table](../../../src/account/chaos_eq_data.h#L99), [staff entries](../../../src/account/chaos_eq_data.h#L159), and [ring entries](../../../src/account/chaos_eq_data.h#L267). The account setup path instantiates profile `vnum` values ([profile selection and item append](../../../src/account/nanny.c#L620)). Erzul's custom reward branch also looks up #9460 directly ([ring output](../../../src/ships/ship_shop.c#L3768)).

These source links identify bindings and item lookups. They do not prove an enabled shop, live NPC, travel arrival, equipment issuance, or successful custom transaction.

## Title, duplicates, and followed history

After color-code removal, trimming, whitespace normalization, and case-folding, “Sarmiz'Duul” occurs once among all 442 current `.zon` titles and once among the 351 active contributors. Same-type room, mobile, and object scans across all source files found no duplicate VNUM declarations. Followed WLD, MOB, OBJ, ZON, QST, and SHP history stays on the `sarmiz` paths; the tracked area sources have same-stem history from the 2009 area-tree move, later 2014–2026 edits, and no alternate-stem rename, explicit predecessor, or whole-zone replacement. The story sidecar was added separately in commit `422a3ca61` on 2026-10-03. The CHAOS alias `sarmiz` and ferry stop name are command/travel bindings to specific rooms, not duplicate zone titles. Keep active `sarmiz` DO NOT MOVE.

## Builder questions and recommended resolutions

1. **Should the three inactive Surface approaches remain historical map records?** They point to active #9400 and #9975 only from source-only rooms. **Recommendation:** preserve them on Surf candidate row 97 until the Surface owner decides their map disposition; do not count them as active arrival paths.
2. **Is the open Surface gate at #9400 intended to expose the whole city at reset?** All 583 rooms are raw-reachable from this arrival, but the state-1 door filter leaves 249. **Recommendation:** confirm the guard/gate and return intent before changing either side of the Surface pair.
3. **Is the Underworld arrival at #9853 meant to have only six rooms in its conservative post-reset component?** Raw reach covers all 583. **Recommendation:** document the intended route through closed/hidden state and preserve the reciprocal Underworld boundary until an owner qualifies access.
4. **Is the harbor arrival at #9975 meant to expose 18 rooms at reset?** Raw reach is all 583 while strict reach is 18. **Recommendation:** verify the intended port/dungeon connection and return before altering its reciprocal Surface exit.
5. **Are Storm Port #22541 and destination #9597 an administrative-only path or public one-way entrance?** No reverse WLD record is present. **Recommendation:** preserve the record and have both map owners name its intended caller and return route.
6. **Is Winterhaven's #55402 load-room exit to Sarmiz #9401 intentional for ordinary travel?** It has no direct reverse record and starts in a room titled “Incarne loading room for Evils.” **Recommendation:** keep its load-room classification separate from public ingress until its owner confirms intent.
7. **Are the 21 state-1 closed pairs across the city roads, barracks, temple approaches, and underground passages intentional?** They are all reset at 100%. **Recommendation:** preserve the pairs while the builder checks intended access, key, and hidden-door behavior in source and live configuration.
8. **Should the temple hallway pair #9657/#9678 remain state-2 closed/locked?** The dossier maps the silver key #9458 to one deacon source. **Recommendation:** keep both reset records until the owner confirms key placement, lock flags, break value, and return access.
9. **Is the hideout passage's asymmetric reset deliberate?** #9733 down is state 5, but reciprocal #9899 up is state 1. **Recommendation:** confirm whether secret visibility should differ by direction; align both reset states if the asymmetry has no intended behavior.
10. **Should the #9802/#9848 passage remain state-5 closed/secret on both sides?** **Recommendation:** record its discovery and return method before changing either state or describing it as an available route.
11. **Is the throne room's “Secret Room” pair #9962/#9965 intentionally locked at state 2?** Both resets are 100%, and the dossier notes raw door flags do not prove that it is discoverable. **Recommendation:** establish the intended key/discovery path before changing flags, reset state, or room claims.
12. **What should happen to Sior's four object resets with no object definitions (#3095, #3096, #6076, #6018)?** **Recommendation:** identify intended stock or remove stale reset rows only after builder review; do not treat local mobile #6018 as the missing object.
13. **Should local imported reset objects #72, #1232, and #3097 remain owned by Heavens/Verzanan?** #72 also has source-only Hostel and Mavulsk declarations. **Recommendation:** preserve active owner files and IDs until all importing areas agree on any replacement.
14. **Should Learett's shop #9413 keep Heavens products #363/#368/#369 beside the Sarmiz-made gear?** **Recommendation:** preserve their external ownership and confirm expected shop stock with Heavens before moving or replacing either side.
15. **Are the shared potion uses of Sarmiz object #9428 intentional?** It is consumed by Ship Yards' Bestile, stocked/reset by Alatorin, and stocked by Severne in Sarmiz. **Recommendation:** preserve the exact item identity and coordinate any type, effect, or label change across all three areas.
16. **Should Quietus continue accepting Sarmiz sword #9436 as its delivery proof?** Quietus QST consumes the same locally sourced sword carried by Rodev. **Recommendation:** treat its VNUM as a shared contract dependency and coordinate replacement with the Quietus owner.
17. **Should Ship Yards continue accepting Sarmiz totem #9440 as Gelden's proof?** **Recommendation:** preserve the exact item and cap/source intent unless both Sarmiz and Ship Yards owners approve a change.
18. **Should Chaos equipment continue using Sarmiz objects #9438, #9447, and #9460?** These are typed item lookups in compiled starter profiles; #9460 is also Erzul's custom ring. **Recommendation:** preserve each item VNUM and coordinate balancing or replacement with the account/equipment and ship-system owners.
19. **Should Erzul's custom moonstone arc gain a native journal binding?** The source sidecar explains contacts and blockers but does not bind a native Q/QA. **Recommendation:** keep journal completion absent until builders choose observable, durable acquisition, assembly, ring, or crew stages; do not award credit for keywords or proximity.
20. **What should Erzul's transaction contract and Sarmiz's prose/effects claim?** The accounting guard leaves pirate-core issuance unavailable; assembly has incomplete failure ordering and a second-core TODO, the handler does not validate the addressed recipient, and the sparkling “sleeping potion” is a generic type-13 object while the separate flask has spell values. **Recommendation:** retain the guard; define recipient, material/payment reservation, accepted output, and replay policy before enabling the custom arc, and align story text only after builders decide the intended potion/effect and any actual campaign endings.

## Safe disposition

Keep active Sarmiz in place. Preserve its three reciprocal ordinary boundaries, two separate inbound-only records, imported Heavens/Verzanan objects, the active external consumers of #9428/#9436/#9440, and the compiled service, ferry, alias, smith, and CHAOS item references. Preserve the three inactive Surf approaches on the existing RETAIN IN PLACE row. The missing Sior objects and custom moonstone transaction remain builder decisions; no migration clearance follows from this source audit.

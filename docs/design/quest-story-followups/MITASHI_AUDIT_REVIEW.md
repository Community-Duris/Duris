# Mitashi repository-wide audit supplement

This checkpoint supplements the [source-comprehensive Mitashi dossier](../zone-stories/MITASHI.md) with active and inactive map topology, typed cross-area dependencies, normalized title, and bounded file history. The source snapshot is `72ba3deff3bb30bde880018cdebe7c6db675b697`. This is a documentation audit; it changes no area or gameplay source.

## Active graph and boundaries

The active `mitashi` WLD has 229 rooms and 572 authored exits. All 572 resolve: 570 stay local and two leave for active Surface rooms. Its 229 rooms form one weak component. From #138200, a directed traversal of authored local exits reaches all 229 and has an authored return path from all 229 when the recorded exits are treated as available. There are no fall-chance or current fields and no `EX_BLOCKED` local edge in the parsed WLD; the dossier separately tracks doors, OPEN, water, equipment and actor-state access. These source graph counts do not establish a safe or currently available journey.

| Active boundary | Evidence and relationship |
| --- | --- |
| Mitashi #138200 north → Surface #647688 | [Mitashi exit](../../../areas/wld/mitashi.wld#L14). |
| Surface #647288 south → Mitashi #138200 | [Surface exit](../../../areas/wld/surface.wld#L3092445). It arrives at the northern gate, but it does not reverse the first boundary because its Surface endpoint is #647288 rather than #647688. |
| Mitashi #138409 south → Surface #648088 | [Mitashi exit](../../../areas/wld/mitashi.wld#L4929). |
| Surface #648088 north → Mitashi #138409 | [Surface exit](../../../areas/wld/surface.wld#L3109229). This is one reciprocal pair. |

Keep the two northern records as separately authored, non-reciprocal edges. Preserve the southern bay pair as authored. The dossier remains the source for city routes, gates, equipment and water restrictions.

## Inactive Duris3 map records

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits with no parse issues. It found 229 inactive source-room VNUM overlaps, all in `Duris3.wld`, matching the active Mitashi range #138200–#138428. Inactive sources contain 916 target records into Mitashi: 462 originate outside the active room-ID range, and 454 originate in the same-numbered `Duris3.wld` rooms. The latter remain separate map-identity evidence and are not added to the curated approach count.

The 462 non-overlapping source records form two map bands and four endpoint records:

| Source-only records in inactive `Duris3.wld` | Target mapping | Evidence |
| --- | --- | --- |
| #137900–#138128, south (`D2`), 229 exits | One per source room to Mitashi #138200–#138428 in order | [first](../../../areas/wld/Duris3.wld#L795913), [last](../../../areas/wld/Duris3.wld#L800701). |
| #138500–#138728, north (`D0`), 229 exits | One per source room to Mitashi #138200–#138428 in order | [first](../../../areas/wld/Duris3.wld#L808505), [last](../../../areas/wld/Duris3.wld#L813293). |
| #138199 east (`D1`) → #138200; #138699 east (`D1`) → #138400 | Two endpoint records | [first](../../../areas/wld/Duris3.wld#L802188), [second](../../../areas/wld/Duris3.wld#L812688). |
| #138100 west (`D3`) → #138399; #138429 west (`D3`) → #138428 | Two endpoint records | [first](../../../areas/wld/Duris3.wld#L800117), [second](../../../areas/wld/Duris3.wld#L807026). |

`Duris3` is absent from the active [`areas/AREA`](../../../areas/AREA) list. These 462 records are source-only map approaches, not current ingress. The VNUM collisions also do not identify a single shared set of rooms: active #138200 is “Outside the Gates of Mitashi,” while inactive `Duris3.wld` #138200 is “Mountains”; active #138428 is the Abandoned Warehouse, while the inactive same-numbered record is titled “The Ocean” ([active #138200](../../../areas/wld/mitashi.wld#L1), [Duris3 #138200](../../../areas/wld/Duris3.wld#L802201), [active #138428](../../../areas/wld/mitashi.wld#L5285), [Duris3 #138428](../../../areas/wld/Duris3.wld#L806989)). Preserve the 454 same-numbered-source records as a separate map ownership question rather than reading them as active routes.

Adding the 462 curated records raises the source-only approach total from 1,535 to 1,997. The reviewed Surf subset remains 144. Keep the `Duris3` candidate **RETAIN IN PLACE**; the record set does not establish map ownership or clearance to move it.

## Typed dependencies and scoped negative checks

| Dependency | Source-backed result and boundary |
| --- | --- |
| Local six-sword receipt | Mitashi Q48 consumes one each of local objects #138267–#138272 for #138279 ([QST](../../../areas/qst/mitashi.qst#L48), [story contract](../../../areas/story/mitashi.story.json#L134)). The comprehensive dossier maps the native receipt and competing uses. |
| Air Q98 consumer | Active Air Q98 consumes local North Wind #138268 as one of its exact materials for Cloudseeker #131650 ([QST](../../../areas/qst/airp.qst#L118), [typed story record](../../../areas/story/airp.story.json#L495)). This is a separate Air receipt. |
| Savannah base services and upgrades | Active Savannah QST has six distinct base recipes consuming the six Mitashi swords, then six matching Retribution-plus-instrument upgrade recipes ([first sword recipe](../../../areas/qst/savannah.qst#L17), [last base recipe](../../../areas/qst/savannah.qst#L67), [upgrade recipes](../../../areas/qst/savannah.qst#L89)). Its active story sidecar records the six-sword contract and upgrades ([first](../../../areas/story/savannah.story.json#L182), [six-sword contract](../../../areas/story/savannah.story.json#L422), [upgrade contract](../../../areas/story/savannah.story.json#L398)). These are competing physical roots and recipient-zone receipts, not extra Mitashi prerequisites. |
| CHAOS configured object references | The CHAOS standard-cleric profile includes local object #138254 in slot 17, and the support-consumables table lists local object #138280 twice ([profile](../../../src/account/chaos_eq_data.h#L117), [support list](../../../src/account/chaos_eq_data.h#L580)). These are configured alternate issuance contexts, not Mitashi reset or shop supply. |
| Bounded negative scan results | No external duplicate mobile/object prototype definition, outside active ZON reset-role use, outside SHP product, or type-25 portal target into Mitashi was found. The outside exact QST and structured story-sidecar item matches are the Air and Savannah consumers listed above; no other outside sidecar VNUM hit was found. |

The existing dossier remains the detailed authority for Mitashi's six distinct source parents, hidden/searchable stock, equipment transitions, shops and computed inn, route restrictions, native receipt semantics, and 22 builder follow-ups. The typed scans do not establish live stock, player acquisition, successful trade, or delivery outcome.

## Title, history, and disposition

After removing color tokens and normalizing whitespace/case, “Mitashi - Capital City of the Jade Empire” has one exact owner among 351 current active `.zon` titles: `mitashi` ([title](../../../areas/zon/mitashi.zon#L2), [active area membership](../../../areas/AREA#L393)). The lexical title scan found no second active candidate.

The current WLD, ZON, MOB, OBJ and QST are added to the active area in commit `49df21a02` (2012-04-14); follow-up history stays on those same paths through the recorded later edits. The tracked `mitashi.shp` path is a component-level copy detected from `stormport.shp` in that same 2012 addition (`C086` similarity); its current four shops use Mitashi-local room, actor and object IDs ([current shop source](../../../areas/shp/mitashi.shp#L1)). That copied shop-file ancestry is not evidence that the Storm Port zone as a whole was Mitashi’s predecessor or replacement. The zone sidecar was added on 2026-10-06 in commit `dc0672461`.

The followed area paths show no alternate-stem rename, explicit Mitashi predecessor, or whole-zone replacement claim. The unregistered `Duris3.wld` map bands and same-numbered rooms are inactive map evidence, not a replacement history. Keep active `mitashi` **DO NOT MOVE** and `Duris3` **RETAIN IN PLACE** pending its broader map and dependency closure.

## Builder recommendations

1. Preserve the active northern and southern boundaries as authored, including the non-reciprocal northern endpoints and the southern reciprocal pair. Do not infer live travel, safety or service availability from WLD edges.
2. Preserve the two 229-record Duris3 bands and four endpoint approaches as source-only records. Keep the 454 same-numbered-source target records separate for map identity review; do not treat any of them as active ingress or proof of area replacement.
3. Preserve the exact identities of the six swords and Retribution across Mitashi, Savannah and Air. Keep recipient-zone receipts and competing item custody distinct.
4. Retain active `mitashi` as **DO NOT MOVE** and `Duris3` as **RETAIN IN PLACE**. This checkpoint introduces no gameplay fix or area placement change.


## Checkpoint 285: all-source title and map identity

After stripping Duris color tokens and the title terminator, then normalizing
case and whitespace, “Mitashi - Capital City of the Jade Empire” has one owner
among all 442 current .zon files: active mitashi. The prior bounded history
review remains unchanged: current Mitashi WLD/ZON/MOB/OBJ/QST files entered
the active area under their own stems in 2012; mitashi.shp shares a component
copy-detection link to stormport.shp, which does not establish zone
predecessorship or replacement. The tracked area histories establish no
alternate-stem rename or whole-zone replacement.

The 462 non-overlapping Duris3 approaches and 454 same-numbered-source
references were already documented and counted according to their distinct
roles. This checkpoint adds no route or candidate row. Keep active mitashi
DO NOT MOVE and Duris3 RETAIN IN PLACE. Current curated totals remain 3,777
overall, 249 Surf, 18 ud_map2, and eight Underworld2. See [coverage row 183](ZONE_COVERAGE_REGISTER.csv#L183), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-285-mitashi-boundaries-and-duris3-records), and [bounded title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-285-mitashi-title-and-map-identity).

## Checkpoint 285: direct builder and owner questions

| # | Question | Source evidence | Recommended resolution |
| ---: | --- | --- | --- |
| 1 | Should Kunji's sole native receipt continue requiring one each of the six distinct swords #138267–#138272 for Retribution #138279? | [Kunji Q48](../../../areas/qst/mitashi.qst#L48), [native receipt](../../../areas/story/mitashi.story.json#L134) | Preserve one exact multiset as one receipt; six copies of one sword or six separate receipts do not satisfy it. |
| 2 | Should supplied exact swords remain valid without personal clan kills, first recovery, prior dialogue, Garden access, or an Empress encounter? | [Native preparation rules](../zone-stories/MITASHI.md#prepare-the-six-distinct-swords), [six-item QST](../../../areas/qst/mitashi.qst#L48) | Keep those histories optional; add a personal source requirement only as a separately designed, explicit outcome. |
| 3 | Should the six sword identities remain distinct from ordinary clan blades with similar names? | [Six prototypes](../../../areas/obj/mitashi.obj#L1093), [source table](../zone-stories/MITASHI.md#prepare-the-six-distinct-swords) | Preserve prototype identity and exact kind; name, clan, or weapon category alone cannot substitute. |
| 4 | Should every sword need to be a loose owned root at Kunji's exchange, with worn/HOLD equipment removed and nested copies excluded? | [Exchange boundaries](../zone-stories/MITASHI.md#prepare-the-six-distinct-swords), [native terms](../../../areas/qst/mitashi.qst#L48) | Keep actual loose-root admission explicit and report removal, curse, capacity, ownership, and busy-state refusals separately. |
| 5 | Should the six clan lords remain the declared parents of their respective swords, without requiring a specific kill or loot source for the native exchange? | [Reset parents and placements](../../../areas/zon/mitashi.zon), [source review](../zone-stories/MITASHI.md#prepare-the-six-distinct-swords) | Preserve the current parent mapping; track optional source custody only when a durable source-and-actor contract is selected. |
| 6 | Should the Savannah base and upgrade recipes remain optional uses that compete for the same physical swords, rather than prerequisites to Kunji? | [Savannah branches](../zone-stories/MITASHI.md#choose-later-uses-before-spending-a-sword), [Savannah QST](../../../areas/qst/savannah.qst#L17) | Present the separate recipient-zone choices and exact costs; do not collapse their receipts into Mitashi or imply all branches can reuse one root. |
| 7 | Should Air Q98 remain an independent consumer of North Wind #138268, with no effect on Kunji's local receipt eligibility? | [Air Q98](../../../areas/qst/airp.qst#L118), [cross-zone source review](MITASHI_AUDIT_REVIEW.md#typed-dependencies-and-scoped-negative-checks) | Keep the Air return in Air's receipt history and distinguish each sword root's actual custody and use. |
| 8 | Should Lotus Blossom #138270 and Torrential Rain #138272 remain hidden carried sources whose SEARCH reveal, visibility, recovery, and later gift are separate events? | [Hidden source review](../zone-stories/MITASHI.md#prepare-the-six-distinct-swords), [search implementation](../../../src/cmd/actobj.c#L9583) | Preserve chance and SECRET behavior; do not credit a failed reveal, another player's discovery, or a visible container as personal recovery. |
| 9 | Should preparation status distinguish prototype presence from actual six-item offerability when hidden initiating items or ownership checks can reject admission? | [Readiness and admission limits](../zone-stories/MITASHI.md#prepare-the-six-distinct-swords), [journal snapshot](../../../src/world/zone_story_quest_runtime.c#L167), [native selection](../../../src/world/quest.c#L1517) | Expose supported, blocked, and unknown capabilities separately; keep the native acceptance predicate authoritative. |
| 10 | Should mode-two contract metadata remain separate from a promise that cap-one sources or Kunji renew for every daily cycle? | [Zone header and resets](../../../areas/zon/mitashi.zon#L3), [renewal follow-up](../zone-stories/MITASHI.md#builder-follow-ups-and-capability-expansion) | Keep daily use unqualified until ready reset generation, recipient renewal, reward, replay, and persistence are established. |
| 11 | Should the “sword/swords,” “torment,” and “empress” aliases remain dialogue responses without awarding learned-topic progress on an attempted keyword alone? | [Mitashi QST responses](../../../areas/qst/mitashi.qst#L2), [dialogue interpretation](../zone-stories/MITASHI.md#prepare-the-six-distinct-swords) | Preserve the aliases and no-op/refusal states; define an addressed response event before adding any topic receipt. |
| 12 | Should Kunji continue to depart after accepting the six swords and awarding Retribution, with revenge and the Empress's fate left as lore? | [Kunji exchange](../../../areas/qst/mitashi.qst#L48), [story limits](../zone-stories/MITASHI.md#prepare-the-six-distinct-swords) | Preserve the native departure and single receipt; require a separate authored campaign endpoint for any later revenge or Empress outcome. |
| 13 | Should consumed inputs, reward issuance, giver departure, and the accepted receipt remain one recoverable transaction across replay or restart? | [Native contract](../../../areas/qst/mitashi.qst#L48), [transaction follow-ups](../zone-stories/MITASHI.md#builder-follow-ups-and-capability-expansion) | Keep reward and retirement atomic and idempotent; recovery must not duplicate experience or fabricate missing sword custody. |
| 14 | Should Retribution's packed cure-critical and spirit-anguish behaviors remain separate from quest completion and from generic possession credit? | [Reward prototype](../../../areas/obj/mitashi.obj#L1336), [effect limits](../zone-stories/MITASHI.md#choose-later-uses-before-spending-a-sword) | Preserve the source effect data pending chosen combat qualification; record a real eligible resolved effect, not a weapon message or handled proc. |
| 15 | Should the two northern Surface records remain directed and non-reciprocal while the southern bay pair stays reciprocal? | [Active boundary table](MITASHI_AUDIT_REVIEW.md#active-graph-and-boundaries), [WLD records](../../../areas/wld/mitashi.wld#L14) | Preserve the four authored endpoints; review any route repair as a separate map-owner decision and qualify actual arrival and return. |
| 16 | Should bay travel continue to honor WATER_NOSWIM and actor-specific boat, flight, levitation, mount, and altitude rules despite dock flags? | [City routes and water](../zone-stories/MITASHI.md#city-access-and-service-boundaries), [room declarations](../../../areas/wld/mitashi.wld) | Keep dockability separate from a ship purchase or service; record admitted movement and safe return under actual actor conditions. |
| 17 | Should the four current Mitashi shops remain distinct local room/mobile/item bindings even though active accounting blocks trade mutations? | [Shop bindings](../../../areas/shp/mitashi.shp#L1), [shop boundary](../zone-stories/MITASHI.md#city-access-and-service-boundaries) | Preserve shop identities and prices until durable trade publication is supported; listing goods or meeting a shopkeeper is not a purchase receipt. |
| 18 | Should Seven Gardens remain an effective computed inn through ROOM_INN even though there is no literal local assignment row? | [Inn room](../../../areas/wld/mitashi.wld#L5250), [effective binding](../../../src/world/db.c#L1368), [rent behavior](../zone-stories/MITASHI.md#city-access-and-service-boundaries) | Discover effective room bindings as well as explicit assignments; credit only a successfully saved terminal rent outcome. |
| 19 | Should Le Ming's closure notice, Askume's vial keywords, and Can Hy's stable remain current contexts until service intent and valid product types are selected? | [Service boundary review](../zone-stories/MITASHI.md#city-access-and-service-boundaries), [local shops](../../../areas/shp/mitashi.shp) | Preserve present behavior while owners choose any repair; do not advertise an unavailable poison, mount, or ship service as implemented. |
| 20 | Do the 462 non-overlapping Duris3 approaches and 454 same-numbered-source references belong to one authoritative map, alternate layouts, or separate historical data? | [Duris3 map records](MITASHI_AUDIT_REVIEW.md#inactive-duris3-map-records), [Duris3 candidate evidence](../../../areas/wld/Duris3.wld#L802201) | Keep Duris3 RETAIN IN PLACE and the 462 routes source-only until the map owner resolves identity; do not read VNUM overlap as replacement proof. |

# Myrabolus: repository-wide route, dependency, and history supplement

This bounded static supplement accompanies the [comprehensive Myrabolus source dossier](../zone-stories/MYRABOLUS.md) and its [generated source index](../../reference/zone-story-audits/mira.md). It records active and source-only boundaries, exact typed references, compiled bindings, title ownership, and bounded same-stem history. These source findings do not establish current runtime access, stock, payment, or player outcomes.

## Active boundaries and source-only approaches

The active `mira` map has 188 rooms from #82500 through #82687 and 393/393 resolving exits: 387 local and six across area boundaries. Its weak components have sizes 182, 2, 1, 1, 1, and 1. Four cross-area pairs are reciprocal: three with Surface and one with Darkfall. The active one-way inbound exits are Shantil's Stronghold #22649 south to #82540 and Winterhaven #55614 northwest to #82502. The one-way outbound exits from #82667 lead east to Desolate #22209 and west to Cerebus Park #22000. All six outbound and six inbound records resolve in the active WLD set; an authored exit does not prove the corresponding service, arrival, or return is usable.

| Active pair or direction | Source records | Register evidence |
| --- | --- | --- |
| Surface, three reciprocal pairs | #82500 south to #513674; #82500 west to #513273; #82670 north to #512874 | [Mira outbound pairs](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L280), [Surface returns](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L544) |
| Darkfall, reciprocal pair | #82679 south to #95312 | [Inbound/outbound pair](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L110) |
| One-way active inbound | Shantil's Stronghold #22649 south to #82540; Winterhaven #55614 northwest to #82502 | [Shantil's Stronghold](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L451), [Winterhaven](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L803) |
| One-way active outbound | #82667 east to Desolate #22209; #82667 west to Cerebus Park #22000 | [Desolate](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L279), [Cerebus Park](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L277) |

### Source-only inactive-WLD approaches

All 95 WLD files outside active `areas/AREA` parsed without errors, covering 527,791 rooms and 2,080,773 exits. Their room VNUMs do not overlap Myrabolus. Two non-overlapping `surf.wld` exits target active rooms:

| Source-only Surf exit | Myrabolus target | Source |
| --- | ---: | --- |
| #235782 south | #82670 | [surf.wld](../../../areas/wld/surf.wld#L750698) |
| #236382 north | #82500 | [surf.wld](../../../areas/wld/surf.wld#L763290) |

These records are source-only, not active ingress. Preserve both on existing Surf candidate row 97. They increase the curated source-only count to 3,777 overall and 249 for the reviewed Surf subset; ud_map2 remains 18 and Underworld2 remains eight. They do not add an active boundary pair or candidate row.

## Exact typed references and prototype closure

The bounded global scans distinguish room, mobile, and object namespaces and classify QST, sidecar, reset, shop, and portal fields by role. Active external native contracts consume four Myrabolus object kinds:

| Active source | Typed reference | Local prototype and role |
| --- | --- | --- |
| Cosmic QST | `G I #82561` at [line 150](../../../areas/qst/cosmic.qst#L150) | Su-pelt #82561, accepted for the declared experience result; see [prototype](../../../areas/obj/mira.obj#L844). |
| Desolate QST | `G I #82543` at [line 118](../../../areas/qst/desolate.qst#L118) | Letter #82543, delivered for the Desolate totem and coins; see [prototype](../../../areas/obj/mira.obj#L622). |
| Winterhaven QST | `G I #82553`, #82554, and #82555 at [lines 1183–1185](../../../areas/qst/wh.qst#L1183) | Combat brace, priest's shayla, and axe of the Maelstrom; three of thirteen distinct inputs to the Winterhaven upgrade. See [local prototypes](../../../areas/obj/mira.obj#L753). |

Structured sidecars repeat these references in typed fields: Cosmic's `item_vnums` contains #82561 ([sidecar](../../../areas/story/cosmic.story.json#L757)); Desolate's item #82543 and mobile contact/giver #82569 are separately typed ([contract](../../../areas/story/desolate.story.json#L652), [contact](../../../areas/story/desolate.story.json#L211)); Rogue Rai has a contact on mobile #82569 ([sidecar](../../../areas/story/roguerai.story.json#L193)); Winterhaven carries items #82553–#82555 in its thirteen-part request ([sidecar](../../../areas/story/wh.story.json#L903)). A `mob_vnum` or `giver_vnum` is not an item reference even where the integer also appears in the room range.

Active external resets and shops also import local objects:

| Source | Prototype references | Context |
| --- | --- | --- |
| `desolate.zon` | #82502, #82513, #82514, and #82501 at [lines 504–507](../../../areas/zon/desolate.zon#L504) | Resets the Myrabolan backpack, lockpicks, spellbook, and quill. |
| `desolate.shp` | #82502, #82513, #82514, and #82501 at [lines 55–58](../../../areas/shp/desolate.shp#L55) | Product stock in shop #22292; this is distinct from reset declarations and payment admission. |
| `vehicles.zon` | #82545, #82559, and #82553–#82555 at [lines 826–844](../../../areas/zon/vehicles.zon#L826) | Possible master equipment and Maelstrom equipment sources; reset chance/cap and actual admission still apply. |

The local `mira.shp` contains two shop records, #82515 and #82523 ([first](../../../areas/shp/mira.shp#L1), [second](../../../areas/shp/mira.shp#L38)). The coverage inventory had incorrectly recorded zero; Checkpoint 281 corrects it to two. The bounded scan found no external QST, SHP, story, or ZON typed reference from an unregistered source into these local prototypes.

Across all other object files, the type-25 scan found one target into Myrabolus: active Hunt portal #13363 targets room #82627 ([portal](../../../areas/obj/hunt.obj#L936)). The local portal #82539 targets the Hunt, so these are portal routes rather than WLD exits; their arrival, admission, and return behavior remain separate from the Surf count. No external mobile or object definition duplicates a local Myrabolus prototype. The same scan includes active and source-only object files; the existing dossier separately records the active-only portal inventory.

Myrabolus's active ZON also has an unresolved local object reset: `O 0 #2267` in room #82626 at [line 196](../../../areas/zon/mira.zon#L196), but #2267 is absent from `mira.obj`. The source dossier records that the loader disables the unresolved object argument. This is a builder-owned restoration/removal decision, not evidence for an arbitrary replacement item.

## Compiled references and services

Seven direct local assignments bind Myrabolus prototypes: mobile #82521 to `money_changer`, mobile #82511 to `world_quest`, objects #82545 and #82500 to `master_set` and `generic_parry_proc`, and rooms #82574, #82641, and #82669 to the inn, crew-shop, and ship-shop functions ([assignments](../../../src/specs/specs.assign.c#L290)). The shared assignment table also binds imported storage locker #3097, spell pool #72, and huntsman ward #54 to their functions; their actual admission and payment behavior stays with those shared services ([storage assignment](../../../src/specs/specs.assign.c#L1308), [spell pool](../../../src/specs/specs.assign.c#L1620), [ward](../../../src/specs/specs.assign.c#L1803)).

Other typed compiled matches include Andryn mobile #82518 in the smith table ([tradeskill](../../../src/economy/tradeskill.c#L203)), Myrabolan backpack #82502 in Chaos equipment rows ([table](../../../src/account/chaos_eq_data.h#L204)), master-set equipment #82545 and #82559 ([legacy set list](../../../src/specs/specs.set.c#L47), [adapter](../../../src/specs/specs.set_equipment.c#L419)), and ship/ferry service IDs #82641, #82669, and #82686 ([ship stops](../../../src/ships/ship_variables.c#L42), [ferry stop](../../../src/world/ferryact.c#L104)). The Chaos target list gives room #82500 aliases `myra myrabolus` ([target names](../../../src/combat/chaos.c#L22)). These are typed bindings, equipment, or service references; a raw numeric match alone is not an additional prototype dependency.

## Title and bounded history

The normalized title “Myrabolus” is unique among the 442 scanned `.zon` files ([current title](../../../areas/zon/mira.zon#L2)). Followed `mira.wld` and `mira.zon` histories remain on their current stems from the 2009 area-tree move through same-stem edits in 2011–2017 and the 2026 ferry update. No tracked alternate-stem rename, distinct predecessor, or whole-area replacement was established in those bounded paths. The active contributor remains listed in [areas/AREA](../../../areas/AREA#L297); this bounded history cannot prove an untracked predecessor never existed.

## Migration disposition

Keep active `mira` **DO NOT MOVE**. Preserve the four reciprocal active boundary pairs, the active one-way service and dispersal edges, both Surf source-only approaches on candidate row 97, the two local shops, all typed foreign references, the Hunt portal return, and the named #2267 reset gap. No new candidate row is added. This source review does not qualify the one-way travel, dynamic services, accounting, actual reset stock, or player quest outcomes.

## Checkpoint 281: direct builder and owner questions

| # | Question | Source evidence | Recommended resolution |
| ---: | --- | --- | --- |
| 1 | Should Roland's local Maximas-head exchange remain an accepted zero-reward support receipt while Hunt Roland's rewarded exchange stays separately owned? | [Local Q2](../../../areas/qst/mira.qst#L2), [Hunt source review](../zone-stories/MYRABOLUS.md#native-progression-and-classifications) | Preserve the authored no-reward result until the owner chooses a real reward or deprecation; do not copy the Hunt bow/arrow reward into Myrabolus. |
| 2 | Are Roland and Ethan intended to wander from their current load rooms, or should either receive a new fixed placement? | [Load-room and wandering review](../zone-stories/MYRABOLUS.md#real-bindings-services-and-access-properties), [local resets](../../../areas/zon/mira.zon) | Qualify actual spawn and encounter behavior before moving either mobile; empty houses and prose do not establish a placement defect. |
| 3 | Should every native exchange continue accepting the exact supplied input without requiring a personal kill, first-source, or earlier-receipt history? | [Sixteen native contracts](../../../areas/qst/mira.qst), [source and custody boundaries](../zone-stories/MYRABOLUS.md#native-progression-and-classifications) | Preserve exact native eligibility; record optional provenance separately from accepted receipt and reward custody. |
| 4 | Should Xavier's parchment, Krazzi proof, and delivery-copy study remain three independent contracts with their current exact prototypes? | [Xavier contracts](../../../areas/qst/mira.qst#L7), [Q16/Q20](../../../areas/qst/mira.qst#L16) | Keep the three recipes independent; require the exact current input and don't infer the observatory, Hunt, or campaign outcome from a hand-in. |
| 5 | Should original study #76068 and delivery copy #76069 remain distinct even though they share name and aliases? | [Ultarium exchange and local input](../zone-stories/MYRABOLUS.md#foreign-deliveries-exact-identities-and-allocation), [Xavier Q20](../../../areas/qst/mira.qst#L20) | Preserve typed VNUM identity; supplied #76069 remains eligible without personal production history. |
| 6 | Should the four treasure recipients continue requiring separate #13364 copies for four independent rewards? | [Rico, Random, Decker, Alexis](../../../areas/qst/mira.qst#L26), [Hunt treasure stock](../zone-stories/MYRABOLUS.md#foreign-deliveries-exact-identities-and-allocation) | Keep each receipt and reward separate; source caps and another recipient's exchange do not promise fresh stock. |
| 7 | Should Decker's `D1` contract removal remain separate from receiving the war helm and from any Hunt-campaign result? | [Decker Q60](../../../areas/qst/mira.qst#L60), [recipient boundary](../zone-stories/MYRABOLUS.md#native-progression-and-classifications) | Record exact recipient removal and item delivery independently; don't infer a relocation or campaign victory. |
| 8 | Should Markam's parchment return grant same-kind parchment plus a new keystone half, with actor removal and UID continuity kept separate? | [Markam Q84](../../../areas/qst/mira.qst#L84), [parchment reset](../../../areas/zon/mira.zon#L187) | Preserve the native staged recipe; treat an earlier Markam receipt as optional history and the returned-input stage as outside native daily candidacy. |
| 9 | Should the two keystone halves remain different required kinds, and should assembled key #82520 retain its authored transient/NORENT and break behavior? | [Andryn Q45](../../../areas/qst/mira.qst#L45), [key and treasury route](../zone-stories/MYRABOLUS.md#warning-halves-palace-and-portal) | Preserve exact kind, custody, drop, and break behavior until a balanced change is specified; count actual unlock and passage separately. |
| 10 | How should active reset O196's missing treasury prototype #2267 be resolved: restore that exact object, remove the obsolete row, or select a justified replacement? | [Unresolved reset](../../../areas/zon/mira.zon#L196), [finding and acceptance evidence](../zone-stories/MYRABOLUS.md#findings-and-planned-work) | Keep the current unresolved finding visible; don't substitute an arbitrary object or claim its disabled reset creates stock. |
| 11 | Is the treasury's forward key #82520 and reverse key0 policy intentional, or should both faces use a synchronized access rule? | [Treasury exits and key-state review](../zone-stories/MYRABOLUS.md#warning-halves-palace-and-portal), [current world source](../../../areas/wld/mira.wld) | Confirm actual unlock, key-destruction ordering, reverse synchronization, reset, and return before changing either face. |
| 12 | Should Xavier's key, local portal #82539, and Hunt return portal #13363 allow arrival that bypasses earlier city stages? | [Xavier Q7](../../../areas/qst/mira.qst#L7), [local portal](../../../areas/obj/mira.obj), [Hunt portal](../../../areas/obj/hunt.obj#L936) | Preserve supplied-item and portal routes unless the owner selects a full campaign gate; record actual destination and arrival separately. |
| 13 | Should both local Myrabolus shop records #82515 and #82523 remain part of the area inventory with their current individual stock and guards? | [First shop](../../../areas/shp/mira.shp#L1), [second shop](../../../areas/shp/mira.shp#L38) | Keep both authored records; qualify each purchase/sale and accounting gate instead of treating either shop as absent. |
| 14 | Should fixed raft #82546 remain distinct from takeable boat #431 and from Shantil's guarded shop stock for the naval officer's exchange? | [Officer Q110](../../../areas/qst/mira.qst#L110), [source distinctions](../zone-stories/MYRABOLUS.md#foreign-deliveries-exact-identities-and-allocation) | Preserve exact item types and current purchase guards; a fixed raft or shop reset declaration does not satisfy the boat input. |
| 15 | Should lost-monkey proof #13365 remain separate from living monkey NPC #82542 and from a personal rescue history? | [Hunter Q102](../../../areas/qst/mira.qst#L102), [item/NPC distinction](../zone-stories/MYRABOLUS.md#foreign-deliveries-exact-identities-and-allocation) | Keep exact item acceptance; create a rescue objective only with a live actor, beneficiary, destination, and saved outcome. |
| 16 | Are the unplaced living monkey and wounded guard, plus unreset head, falchion, and alternate quiver prototypes, intentional context or unfinished content? | [Unplaced-content finding](../zone-stories/MYRABOLUS.md#findings-and-planned-work), [local reset sources](../../../areas/zon/mira.zon) | Keep their status as builder questions; don't add placement, healing, or rewards without an authored source and outcome. |
| 17 | Should paid scale armor crafting remain blocked while accounting is active until the scale, 50,000 copper, and output settle atomically? | [Andryn Q53](../../../areas/qst/mira.qst#L53), [smith binding](../../../src/economy/tradeskill.c#L203) | Preserve current refusal until debit, item consumption, output custody, replay, and save are qualified together. |
| 18 | Should ship-crew hiring mutate crew or ship state only after a confirmed debit and durable save? | [Crew-room binding](../../../src/specs/specs.assign.c#L2401), [service finding](../zone-stories/MYRABOLUS.md#findings-and-planned-work) | Keep the existing service gap explicit; any repair needs separate shared-service evidence and must refuse state change after a rejected/busy debit. |
| 19 | Should the master equipment set count only its seven-entry bound list, or also the #82559 longbow in the separate eight-entry adapter? | [Direct binding](../../../src/specs/specs.assign.c#L1344), [set lists](../../../src/specs/specs.set_equipment.c#L419), [owner review](../zone-stories/MYRABOLUS.md#real-bindings-services-and-access-properties) | Preserve current membership until the builder resolves intended count and binding; qualify equip, unequip, duplicates, transfer, death, and restart before changing it. |
| 20 | Should the two non-overlapping Surf approaches remain source-only on candidate row 97 rather than imply active ingress into Myrabolus? | [Surf exits](MYRABOLUS_AUDIT_REVIEW.md#source-only-inactive-wld-approaches), [Surf candidate](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97) | Retain both exact records with Surf; keep them distinct from the six active cross-area exits and do not add a candidate row. |


## Checkpoint 782: Ultarium study delivery dependencies

Myrabolus QST line 23 consumes Cosmic delivery-copy object #76069. Its story
sidecar also identifies Cosmic giver #76029 and distinguishes original study
#76068 from delivery copy #76069 ([contract metadata](../../../areas/story/mira.story.json#L316)).
Preserve these mobile/object identities and the optional producer history;
see the [Cosmic typed audit](COSMIC_AUDIT_REVIEW.md#active-and-candidate-inbound-dependencies).

## Checkpoint 786: Mira rewards use Desolate objects

Active mira.qst lines 112–113 return Desolate objects #22279 and #22280.
Mira story completion keys at lines 676 and 709 repeat those two object VNUMs.
Preserve Desolate as their current prototype owner and the active Myrabolus
consumer; the authored result does not establish availability. See the
[Desolate typed audit](DESOLATE_AUDIT_REVIEW.md#qst-story-sidecar-and-compiled-references).

## Checkpoint 787: Divine Home QST reward

Active `mira.qst` row 55 returns Divine Home object #40738
([source](../../../areas/qst/mira.qst#L55)). Preserve Divine Home as the current
prototype owner; the QST declaration does not establish supply or successful delivery.
See the [Divine Home typed audit](DIVHOME_AUDIT_REVIEW.md#active-external-consumers).

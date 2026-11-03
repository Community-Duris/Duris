# The Surface Realm of Duris audit review

Static source review for coverage checkpoint 373. The existing [Surface Realm story map](../zone-stories/SURFACE_REALM.md) remains the content dossier; this supplement records repository-wide identity, connectivity, dependency, and builder-question closure. Source evidence does not establish played availability, transaction success, safe travel, or builder policy decisions.

## Checkpoint 373: active source scope

The active contributor is surface, titled “The Surface Realm of Duris.” Its loaded source contains 160,004 rooms (#500000–#660003), 234 mobiles, 78 objects, 37 QST givers, and 31 native exchanges. There is no local shop source. The existing dossier groups all 141 QST blocks, every local mobile/object/reset, all 160,004 rooms, and the eight literal code assignments; no narrative map rewrite was needed. See [coverage row 286](ZONE_COVERAGE_REGISTER.csv#L286), [native-source index](../../reference/zone-story-audits/surface.md), and [story map](../zone-stories/SURFACE_REALM.md).

The active WLD census remains 639,866 authored directions, 639,864 loader-resolved directions, 423 inbound boundary directions, 370 outbound boundary directions, and 359 reciprocal records. Two positive unresolved targets remain: Surface #522255 east and #522656 north both target unloaded #33807. This checkpoint changes no active WLD edge.

## Inactive WLD Surface approaches

A targeted scan of every inactive WLD source found 479,232 authored directions whose numeric targets are loaded Surface rooms. Of those, 1,208 originate in source-room VNUMs with no active WLD owner. The other 478,024 begin at VNUMs already owned by active rooms; they remain map-identity evidence and are not added as separate ingress.

| Inactive source | Non-overlapping directions into active Surface | Source evidence |
| --- | ---: | --- |
| asunwa.wld | 1: #35300 north → #604903 | [source destination](../../../areas/wld/asunwa.wld#L13) |
| dalvik.wld | 1: #298 south → #635660 | [source destination](../../../areas/wld/dalvik.wld#L6990) |
| jungle.wld | 2: #67005 north → #616072; #67087 south → #616872 | [first](../../../areas/wld/jungle.wld#L80), [second](../../../areas/wld/jungle.wld#L1917) |
| newbiemaps.wld | 602 across 600 source rooms | [sample north edge](../../../areas/wld/newbiemaps.wld#L1256111), [west edge](../../../areas/wld/newbiemaps.wld#L1256123), [east edge](../../../areas/wld/newbiemaps.wld#L1262310) |
| surface2011-temp.wld | 602 across 600 source rooms | [sample north edge](../../../areas/wld/surface2011-temp.wld#L1260092), [west edge](../../../areas/wld/surface2011-temp.wld#L1260104), [east edge](../../../areas/wld/surface2011-temp.wld#L1266291) |

The two large maps contain the same 602 (source room, direction, target room) tuples, but their source room titles differ. The shared family consists of 300 north directions from #660004–#660303 to #659704–#660003, 300 south directions from #689700–#689999 to #600000–#600299, one west direction from #660004 to #660003, and one east direction from #660299 to #660000. Their source rooms are absent from the active loader. Keep each file and title identity separate; matching numbers and exit tuples do not establish that either inactive map is loaded or represents current travel.

The five candidate sources add 1,208 source records to the curated inactive-approach total, raising it from 4,851 to 6,059. The existing surf.wld subset remains 285. The duplicate 602-tuple set is counted once per source file so the audit preserves both map identities; it is not a claim of 1,208 distinct geographic entrances. Retain these findings on existing candidate rows 12, 29, 55, 78, and 99. No candidate or active-area path move is authorized.

## Title, same-type identity, and tracked history

After color-code removal, whitespace normalization, and case folding, “The Surface Realm of Duris” appears once among all 442 current ZON titles and once among the 351 active contributors. The active canonical key is surface. No title alias or route owner is inferred from repeated room numbers.

The active Surface WLD has same-number room identities only in newbiemaps.wld (59,851 VNUMs) and surface2011-temp.wld (60,004 VNUMs), both within #600000–#660003 and with different room titles. No other same-type WLD overlap with Surface was found. The separate surface2011.wld contains 90,000 rooms #200000–#289999, with no room-number overlap against the active Surface map; preserve its existing 145 active numeric references on candidate row 98 without treating them as replacement evidence. No same-type mobile or object VNUM overlap with Surface was found.

Followed WLD, MOB, OBJ, ZON, and QST histories remain on the surface source stem and reach the 2009-08-12 areas-tree move. The reviewed path history contains no alternate-stem predecessor or explicit whole-zone replacement decision. The story sidecar was added on 2026-10-03. The inactive newbiemaps and surface2011-temp histories stay attached to their own WLD source identities. Retain all existing candidate maps; no migration clearance follows from this bounded comparison. See [duplicate/replacement register](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-373-surface-titles-identity-and-history) and [attic plan](ATTIC_MIGRATION_PLAN.md#checkpoint-373-surface-route-retention).

## Typed dependency closure

The active external QST scan found 30 G item commands referencing Surface objects, all in five active QST sources: Alatorin (5), Divhome (1), Minizones (20 repeated crystal awards), Surface Keeps (2), and Surface Mini (2). The exact blocks are anchored in [Alatorin](../../../areas/qst/alatorin.qst#L5342), [Divhome](../../../areas/qst/divhome.qst#L281), [Minizones](../../../areas/qst/minizones.qst#L149), [Surface Keeps](../../../areas/qst/surfacekeeps.qst#L120), and [Surface Mini](../../../areas/qst/surfacemini.qst#L227).

Fourteen matching structured item_vnums entries occur in those same five active story sidecars. Their exact groups are Alatorin (#500009, #500021, #500026, #500004, #500006), Divhome (#500026), Minizones (four #500025 entries), Surface Keeps (#500121, #500104), and Surface Mini (#500003, #500046). Five additional typed sidecar fields reference Surface mobiles: Mushroom Caverns lists Draknah #500231 as a contact; Surface Mini lists the Prophet of Fire #500116 and Prophet of Ice #500117 as contacts and repeats those IDs as contract giver_vnum values. See [Mushroom Caverns contact](../../../areas/story/mushroom_caverns.story.json#L74), [Surface Mini contacts](../../../areas/story/surfacemini.story.json#L333), and [Surface Mini giver fields](../../../areas/story/surfacemini.story.json#L1222). These references preserve the defining Surface prototype; they do not prove delivery or active stock.

Twenty-eight active external reset records reference Surface prototypes. Alatorin has 20: objects #500001 once, #500012 four times, #500013 four times, #500023 once, and #500026 seven times; mobiles #500057 once and #500097 twice. The #500026 records include three P placements and four G loads. See [Alatorin lines 1564–1653](../../../areas/zon/alatorin.zon#L1564), [P placements](../../../areas/zon/alatorin.zon#L2633), [G placements](../../../areas/zon/alatorin.zon#L3022), [mobile and object group](../../../areas/zon/alatorin.zon#L4489), and [fairy-potion group](../../../areas/zon/alatorin.zon#L4503). Vehicles has eight G loads of #500026; see [lines 499–501](../../../areas/zon/vehicles.zon#L499), [746–748](../../../areas/zon/vehicles.zon#L746), and [804–807](../../../areas/zon/vehicles.zon#L804). No cross-file QST block is attached to a local Surface mobile. No inactive QST R/G/E item command, ZON reset prototype command, or structured item_vnums, mob_vnum, or giver_vnum sidecar field uses an exact Surface prototype VNUM.

Six active external type-25 objects fixed-target Surface rooms: airshipgrave #77521 → #522469; connectorzones #53666 → #559633; newhaven #35242 → #552844 and #35243 → #552052; tharnadian_ruin #5539 → #568439; and tradeskills #400220 → #554045. The source prototypes are in [Airship Grave](../../../areas/obj/airshipgrave.obj#L290), [Connector Zones](../../../areas/obj/connectorzones.obj#L856), [Newhaven](../../../areas/obj/newhaven.obj#L746), [Tharnadian Ruin](../../../areas/obj/tharnadian_ruin.obj#L539), and [Tradeskills](../../../areas/obj/tradeskills.obj#L1813). The unlisted tradeskills.old.obj also contains an inactive #400220 copy to #554045; it is source-only and is not counted as a loaded travel object. The active SHP scan found no external product reference to a Surface object, and Surface has no local SHP file.

The existing story map separately records local resets of imported prototypes, recipe inputs, building travel, direct code assignments, and shared handlers. Preserve each dependency under its owning area or code path. Static references alone do not establish live stock, safe arrival, or successful transaction.

## Builder questions and recommended resolutions

The 20 questions below close the register’s question-coverage field. They record source-backed decisions for a builder; they do not claim that policy answers have been supplied.

| # | Builder question | Recommended resolution |
| ---: | --- | --- |
| 1 | Should the dialogue correct the four wardens’ head-versus-heart wording, the air locket’s fire aliases, the Fire prophet’s copied Ice introduction, Agnut’s bracer number, and the sparse fishing/fang clues? | Align prose and aliases with the exact QST commands and quantities; keep optional riddles and do not change gates as part of copy repair. |
| 2 | Should the four heart branches enforce allegiance or episode order? | Keep the four #500015 hand-ins independent unless a durable faction or episode policy is deliberately designed; require separate consumed heart instances if a complete campaign is later tracked. |
| 3 | Should Sa’Dah’s four-trophy exchange mean that the four wardens were personally killed or that a world campaign was won? | Preserve the four current trophy items as the native gate. Do not infer personal kills or a final world state from the exchange. |
| 4 | Does the mystic require four distinct locket kinds plus the book at one time, or historic receipts from five completed stages? | Preserve the exact five-kind current-item test (#500028–#500032) and do not substitute receipts for supplied materials. |
| 5 | Should Krekish and Gr’Tak create a permanent opposing allegiance? | Keep their two independent head turn-ins and avoid a permanent faction lock that is absent from source. |
| 6 | Does Crodog’s membership praise enroll the player in a persistent group? | Treat it as dialogue only unless a real membership state and accepted change event are designed. |
| 7 | Should Cheshynn’s five-fang necklace service remain available after the grey-paw hand-in retires her? | Surface the fang service before the paw turn-in if that ordering is intended; keep both contracts independent and do not merge their items. |
| 8 | Does Tren’s white-potion exchange prove that the player personally completed the hunter’s revenge? | Keep supplied potions valid for the native exchange and keep the producer story receipt separate from the potion consumer. |
| 9 | How should five claw items be made available when five of fourteen nightcrawler appearances lack claws? | Confirm intended repeatability and stock before promising completion; count five actual #500020 items, not kills or worn equipment. |
| 10 | Should Jonius’s totem hand-in require a personal raid or witchdoctor kill? | Preserve the exact #500024 item requirement; add personal combat credit only after an explicit builder decision and valid source event. |
| 11 | Should Snardag’s three-crystal story and Flegnus’s four-crystal wand recipe share one completion state? | Keep their different quantities and consumers distinct; qualify enough separate crystals before guidance promises both outcomes. |
| 12 | Are the wyvern egg “study” and “rebirth” statements intended to produce persistent outcomes? | Decide whether to retire stale continuation text or design explicit study/transformation events; item delivery alone does not prove either outcome. |
| 13 | Should treant flamewood/spiritwood decay, and what source history should award first-acquisition credit? | Resolve timer/decay behavior with the builder and record accepted death-created issuance and collection before granting source credit. |
| 14 | Is the highwayman guardian’s helper a story objective or an ambient combat effect? | Keep it ambient unless a quest is intended; any objective must bind the actual helper identity, accepted spawn, defeat, and population/reset policy. |
| 15 | Should a racewar shadow-rift trip count on attempted use or confirmed arrival, especially when a pet master selects the destination? | Record the selected destination and confirmed per-traveler arrival; do not credit shared destination mutation or travel text. |
| 16 | Should Baltazo’s eleven-devil invasion return, or should old invasion lore be retired? | Make that policy decision before restoring the commented death branch; qualify spawn, attempt, defeat, reset, party, and accounting behavior together. |
| 17 | Is the old five-orb descent campaign still intended when do_descend immediately reports that descent is unavailable? | Retire the obsolete continuation or rebuild the whole accepted custody, denial, save, and replay path before presenting it as playable. |
| 18 | How should Surface’s two exits to unloaded #33807 be repaired? | Restore the intended room or choose a verified Mril destination and reciprocal route; preserve valid adjacent paths and test bounded reachability after a builder decision. |
| 19 | Should the mystic’s locked tomb, transparent gateway, building teleports, harbor tent, and ship services be presented as one campaign? | Keep the distinct keys and destination owners separate; record each actual arrival, and keep ship wallet/ownership/refund state transactional. |
| 20 | Should fishing, lessons, and epic purchases become Surface story outcomes? | Keep them as support services unless intended as goals; require accepted durable catch grants, committed skill/class changes, and settled fees before recording any new outcome. |

The [source map](../zone-stories/SURFACE_REALM.md#access-custom-mechanics-and-balanced-repair-plan) retains the underlying room, QST, reset, prototype, handler, and code anchors for these recommendations.

## Checkpoint 777: Breale QST item

Active Surface QST line 556 has G I #2676, a Breale object. Preserve the current Breale owner and Surface's authored item command; see the [Breale typed-dependency audit](BREALE_AUDIT_REVIEW.md#inbound-active-consumers).

## Checkpoint 821: active Randomness mobile reset consumer

Active `surface.zon` has four M reset rows for Randomness mobile #100323, the xorn ([prototype](../../../areas/mob/random.mob#L2547)), at lines 1666, 1667, 1669, and 1670. Preserve the typed mobile dependency on active Randomness; these rows add no WLD direction and do not change Surface’s exit graph. See the [Randomness typed audit](RANDOMNESS_AUDIT_REVIEW.md#checkpoint-821-current-typed-dependency-closure), [`surface.zon`](../../../areas/zon/surface.zon#L1666), and [Randomness coverage row](ZONE_COVERAGE_REGISTER.csv#L247).


## Checkpoint 828: current source and completion-signature reconciliation

The current `areas/` and `src/` trees still match pinned source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`. Surface remains 160,004 rooms (#500000–#660003), 234 mobiles, 78 objects, 37 QST givers, and 31 native Q/QA contracts. The active boundary counts, title/history findings, and 20 builder questions therefore remain as recorded above; this refresh adds no native source change or gameplay claim.

The exact external QST scan still finds 30 active `G I` commands for Surface objects in Alatorin (5), Divhome (1), Minizones (20), Surface Keeps (2), and Surface Mini (2), with no matching external `R I` command. The five active story sidecars retain 14 matching `item_vnums` entries and five mobile/giver VNUM fields; all 232 sidecars parse without errors. A role-aware scan of the `I:` operands inside external `completion_key` strings finds 14 distinct completion signatures containing 15 distinct Surface object VNUMs across those same five sidecars. These signatures repeat QST completion keys, so their item tokens are dependencies; `C:` currency and `E:` experience values are not VNUMs. Surface Mini additionally records the Fire and Ice prophet completion signatures that consume heads #500048 and #500047 and return rings #500003/#500046 plus potions #500049/#500050; see [Fire signature](../../../areas/story/surfacemini.story.json#L1223), [Ice signature](../../../areas/story/surfacemini.story.json#L1308), and the corresponding [Surface QST records](../../../areas/qst/surface.qst#L840).

The external typed reset scan still finds 28 active rows: 20 in Alatorin and eight in Vehicles. It finds no additional external F-command mobile reference. The six active external type-25 destinations, no external active SHP product, no local Surface SHP, and the eight existing direct code bindings remain unchanged from the pinned source review. The completion signatures add no WLD edge and do not change the 639,866 authored / 639,864 resolved direction census, 423 inbound and 370 outbound boundary directions, 359 reciprocal records, or the two unresolved #33807 exits from #522255 east and #522656 north.

Keep the five inactive approach identities on candidate rows 12, 29, 55, 78, and 99, including the distinct `newbiemaps` and `surface2011-temp` title/map identities. The source refresh adds no candidate and no activation, replacement, or path-move clearance. Surface remains an active contributor and **DO NOT MOVE**.
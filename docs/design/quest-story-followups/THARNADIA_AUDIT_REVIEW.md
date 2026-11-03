# Tharnadia audit review — Checkpoint 384

Checkpoint 384 reconciles the existing [Tharnadia source-story dossier](../zone-stories/THARNADIA.md) with active and inactive connectivity, typed prototype dependencies, current title identity, followed history, and twenty-one builder decisions. The source snapshot is `72ba3deff3bb30bde880018cdebe7c6db675b697`. This is a documentation review: it changes no gameplay source, database state, map placement, or migration behavior, and it performs no live player journey.

The active `tharnadia` contributor contains 584 rooms, 176 mobile prototypes, 206 object prototypes, 19 native QST givers/contracts, and 19 shops. Its zone file has 1,480 reset commands grouped into 661 reset families. The dossier reads every local room, mobile, object, shop, quest block, reset, and the complete story sidecar. It maps eight independent story outcomes, eight support rows covering ten contracts, and one excluded typed-food request. These records do not define one required campaign order.

## Story boundary and local decisions

The city’s authored material includes the town and gates, Caer Tannad and the docks, guild and trade services, a wizard-tower story, Xero’s hunting story, Arkelyn’s promised lesson, Leodra’s quill story, Rhed’s animal-rescue text, Chiln’s medicine finale, and a collection of independent crafting, map, equipment, and service exchanges. The dossier records each exact item input, accepted result, giver departure, reset source, and optional preparation route. Ambient claims such as the map seller, holy ascent, recovered quill, or rescued animal do not become extra quest outcomes unless the builder identifies an accepted durable result.

Keep ownership of accepted delivery in Tharnadia even when a required item comes from another area. In particular, Zaberetornaz accepts Tezcat object #98928; other branches use Limbo paper #5 and Heavens weapon prototypes #1108, #1110–#1113. Some outputs also use foreign-owned prototypes, including Kastle object #99412 and Heavens object #500. Supplied proof does not imply that the player personally found, killed, or crafted its source.

## Active connectivity and route state

The [active WLD](../../../areas/wld/tharnadia.wld) declares 1,268 exit records. The loader resolves 1,267 positive directions: 1,262 internal directions and five outward directions. The remaining record is [room #132641 north to `-1`](../../../areas/wld/tharnadia.wld#L3576); the loader drops it, as recorded in [unresolved-exit row 329](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L329). There are six active inbound records, five outward records, and three reciprocal boundary pairs. There is no positive unresolved target.

| Active boundary | Source → destination | Return evidence |
| --- | --- | --- |
| Inbound | Braddistock #135001 south → Tharnadia #132650 | Tharnadia #132650 north → #135001; reciprocal. |
| Inbound | Heavens #24 north → Tharnadia #132573 | Administrative approach with no ordinary Tharnadia reverse. |
| Inbound | Surface #570023 south → Tharnadia #133066 | No outbound Surface edge starts at #133066. |
| Inbound | Surface #570422 east → Tharnadia #133053 | Tharnadia #133053 west → #570422; reciprocal. |
| Inbound | Surface #570424 west → Tharnadia #132867 | Tharnadia #132867 east → #570424; reciprocal. |
| Inbound | Surface #570823 north → Tharnadia #133056 | Tharnadia #133064 south → #570823 instead. |
| Outbound | Tharnadia #132650 north → Braddistock #135001 | Reciprocal. |
| Outbound | Tharnadia #132867 east → Surface #570424 | Reciprocal. |
| Outbound | Tharnadia #133053 west → Surface #570422 | Reciprocal. |
| Outbound | Tharnadia #133064 south → Surface #570823 | Its inbound counterpart reaches #133056. |
| Outbound | Tharnadia #133070 north → Surface #570023 | Its inbound counterpart reaches #133066. |

The six active inbound targets reach 550/584 rooms by ordinary directed WLD exits. From 551/584 rooms, ordinary directed exits lead to one of the five outbound source rooms. The undirected local projection has components of 551, 20, 10, 1, 1, and 1 rooms. The 20-room component begins at #132501. The ten-room component is #132632–#132641 and includes the dropped negative exit; #132615, #132744, and #132869 are isolated in the strict WLD graph. This graph ignores doors, resets, secrets, item travel, scripts, falls, and actor state, so it does not establish a safe or complete player journey.

Tharnadia has 110 authored door directions, all matched by D resets; the zone also has eight D resets on non-door switch/training exits. Four paired door records have differing authored key/flag/reset fields: #132503/#132504, #132511/#132528, #132619/#132809, and #132766/#132767. Preserve their stateful interpretation from the dossier: the western gate’s reset closes it without locking it, and the brick switch at #132526 is pushed to clear the blocked state before searching for the secret route at #132619/#132809. The other asymmetries still need their intended initial and discovery state confirmed. There are 31 fall-chance rooms and no current fields.

Item-teleport prototypes provide additional authored destinations outside the strict WLD graph. Local object #132625 at #133010 targets #132501 ([OBJ target](../../../areas/obj/tharnadia.obj#L1991), [reset](../../../areas/zon/tharnadia.zon#L617)); its return portal #132626 targets park room #132919 ([OBJ target](../../../areas/obj/tharnadia.obj#L2007), [reset](../../../areas/zon/tharnadia.zon#L481)). Portals #132665 and #132666 link wizard rooms #133018 and #133019 ([first target](../../../areas/obj/tharnadia.obj#L2738), [second target](../../../areas/obj/tharnadia.obj#L2750), [resets](../../../areas/zon/tharnadia.zon#L618)). Active Newbie Zone object #22803 (“the life stream”) at #22806 targets city room #132575 ([OBJ target](../../../areas/obj/newbie2.obj#L68), [reset](../../../areas/zon/newbie2.zon#L14)). These destinations are not counted as ordinary WLD edges or as a runtime guarantee that an object is available or usable.

The [cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L4) records the Heavens arrival; [rows 1302–1306](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1302) record the five Tharnadia outbounds; [row 1308](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1308) records the Braddistock return; and [rows 1486–1489](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1486) record the four Surface arrivals.

## Inactive Duris3 map identity and approaches

The all-95 inactive-WLD sweep found 2,336 directions in `Duris3.wld` whose destinations are Tharnadia room VNUMs #132500–#133083. Of these, 604 begin in 602 source rooms with no active WLD owner; they are source-only map approaches, not live entrances:

| Duris3 source-only pattern | Directions | Targets |
| --- | ---: | --- |
| #132200–#132499 south | 300 | #132500–#132799 |
| #133084–#133383 north | 300 | #132784–#133083 |
| #132499 east → #132500; #133299 east → #133000 | 2 | Two endpoints. |
| #132400 west → #132699; #133084 west → #133083 | 2 | Two endpoints. |

The other 1,732 target-matching directions begin at VNUMs that overlap active Tharnadia rooms. All 584 overlapping numeric room identities have different source titles in Duris3 and Tharnadia; preserve these as separate source maps and exclude them from the 604 unique-source count. Do not add these routes to Tharnadia’s six active inbound directions. Keep the source evidence with the existing [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36); its 26 active/unresolved outbound target records and 14 source-area references do not change.

The four source-only pattern samples are visible at [Duris3 #132200](../../../areas/wld/Duris3.wld#L676201), [#132499](../../../areas/wld/Duris3.wld#L682480), [#133084](../../../areas/wld/Duris3.wld#L694765), and [#133383](../../../areas/wld/Duris3.wld#L701044). The same-numbered Duris3 #132500 is at [line 682501](../../../areas/wld/Duris3.wld#L682501), while active Tharnadia #132500 is the adventurer-supply shop at [line 1](../../../areas/wld/tharnadia.wld#L1). The numeric match does not establish room identity, active reachability, or replacement.

## Typed ownership and maintained bindings

The local QST and sidecar reference foreign-owned objects as contract inputs or outputs. This preserves object ownership and makes the exact typed relationship explicit.

| Direction | Source evidence | Typed relationship |
| --- | --- | --- |
| Foreign item → Tharnadia contract | [QST line 90](../../../areas/qst/tharnadia.qst#L90); [story sidecar line 648](../../../areas/story/tharnadia.story.json#L648) | Zaberetornaz accepts Tezcat OBJ #98928; the local delivery remains a Tharnadia outcome. |
| Foreign item → Tharnadia service | [QST line 176](../../../areas/qst/tharnadia.qst#L176) | Ithilin uses OBJ #5 from active Limbo. Exact object identity matters; paper is not a blank-text substitute. |
| Foreign items → Tharnadia services | [QST lines 204–236](../../../areas/qst/tharnadia.qst#L204) | Zech's distinct service branches consume Heavens OBJ #1108, #1110, #1111, #1112, and #1113. |
| Tharnadia contract → foreign reward | [QST line 79](../../../areas/qst/tharnadia.qst#L79) and [line 87](../../../areas/qst/tharnadia.qst#L87) | Local reward definitions refer to Kastle OBJ #99412 and Heavens OBJ #500. Keep those prototype owners and the city’s reward/receipt ownership distinct. |

Tharnadia’s resets import objects from active Heavens, Limbo, Player Castles, Arena, BS, Tradeskills, Torrhan, Verz1, and White Harbor, plus Heavens MOB #101 and Tradeskills MOBs #400000/#400001. The local reset file also tries to load missing MOB #132677 at #132618 (line 1052); OBJ #132677 exists in its separate namespace, so it is not a valid substitute. Local shops stock active foreign prototypes from Heavens, Player Castles, Torrhan, BS, Tradeskills, White Harbor, and Verz1. Preserve these owners when reviewing reset parents or shop inventory; exact placements remain in [the local ZON](../../../areas/zon/tharnadia.zon) and [SHP](../../../areas/shp/tharnadia.shp).

Active outside sources consume or place Tharnadia-owned objects as follows:

| Active consumer | Source evidence | Tharnadia prototype and use |
| --- | --- | --- |
| `brass` | [SHP](../../../areas/shp/brass.shp#L582), [ZON](../../../areas/zon/brass.zon#L819) | OBJ #132509 is shop stock and an external G reset. The inactive `brass-old-1` source also records old stock/reset references, but is not active. |
| `jade` | [ZON line 394](../../../areas/zon/jade.zon#L394) | OBJ #132581 reset in room #76800. |
| `lylr` | [ZON line 82](../../../areas/zon/lylr.zon#L82) | Counter OBJ #132581 reset in room #82228; retained from Checkpoint 161. |
| `unoutpst` | [ZON line 54](../../../areas/zon/unoutpst.zon#L54) | OBJ #132581 reset in room #98820. |
| `ravenloft2` | [ZON lines 826–828](../../../areas/zon/ravenloft2.zon#L826) | Two P resets for OBJ #132622 and one for OBJ #132632 into container #59019. |

The bounded exact-reference scan found no outside QST item command or story-sidecar `item_vnums` consumer for a Tharnadia object, no external same-type mobile/object prototype definition, and no other outside SHP product or type-25 target into this WLD. Maintained code binds home and arena locations in [constant.c](../../../src/core/constant.c#L1702), race/class start rooms in [the Tharn table](../../../src/core/constant.c#L1878), and Tharnadia justice rooms and guard in [justice.c](../../../src/combat/justice.c#L43). Other direct locations include the inn in [guildhall.h](../../../src/guild/guildhall.h#L36), ferry stop #133052 in [ferryact.c](../../../src/world/ferryact.c#L129), crew shop #133075 in [ship_shop.c](../../../src/ships/ship_shop.c#L3238), and tradeskill reset room #133071 in [tradeskill.c](../../../src/economy/tradeskill.c#L1658). The city/home association appears in [Chaos](../../../src/combat/chaos.c#L21) and [CTF](../../../src/combat/ctf.c#L32); Winterhaven's helper list includes local MOBs #132520/#132521 in [specs.winterhaven.c](../../../src/specs/specs.winterhaven.c#L150). Tharnadia-specific procedures are assigned in [specs.assign.c](../../../src/specs/specs.assign.c#L687). Direct object/code references #132508 and #132581 also appear in [Chaos equipment data](../../../src/account/chaos_eq_data.h#L581) and [locker handling](../../../src/cmd/actoth.c#L2442). These are maintained numeric bindings, not new story receipts.

## Title, identity, and followed history

After color-code and terminator removal, whitespace normalization, and case folding, the title is “Tharnadia - City of Humans.” It is unique among 442 current ZON title records and belongs to one of the 351 active contributors. Active same-type room, mobile, and object scans find no numeric VNUM collision; Checkpoint 709 separately records duplicated Caer Tannad room text. The inactive Duris3 room-number overlaps are the distinct identities described above. `tharnadian_ruin` and `tharnrifts` remain separate active source contributors, not predecessor names for the city.

Followed WLD, MOB, OBJ, ZON, and QST paths remain on the `tharnadia` stem. Commit `3a88fc6ce` (“Renumbered tharn,” 2010-08-06) changes the same-stem sources from room #6001 / zone header 60 to room #132500 / header 1325 and adds the area to `areas/AREA`. This establishes a same-path renumber, not a replacement by a separately named area. Commit `79e3dd0e4` changed the exits at the gates to reduce camping in rooms off the map; `102254c47` removed the IC teleporter; `f0ff4a726` added exits from Tharnadia to the map. These changes document route evolution, not present intent for each one-way edge. The current story sidecar was added in 2026 on the same active stem. No followed path establishes a separate whole-area predecessor or replacement.

A later content-level comparison with standalone caertannad found 95 exact normalized WLD title-and-description pairings covering 68 Caertannad room records. The same overlap is present in the pre-renumber Tharnadia WLD; the 2010 change is a same-stem renumber, not evidence that either area replaced the other. The overlapping room-text relationship and unresolved owner choice are recorded in [Checkpoint 709](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-709-caertannad-tharnadia-wld-content-overlap).

Keep active `tharnadia` **DO NOT MOVE**. No candidate or migration clearance is supported by the title or history review.

## Builder questions and recommended resolutions

Use these as decisions for future content work. The recommendation preserves current source ownership and receipts until the builder confirms an intended change.

1. **City identity:** Does “City of Humans” describe a human-founded city with a mixed present-day population, or should the title/prose be more specific? Keep the current title and population as authored until clarified.
2. **Independent story order:** Should the eight local outcomes remain independent, as their native QST and sidecar rules currently allow? Keep them independent; do not introduce an all-stage prerequisite without an authored campaign contract.
3. **Administrative arrival and holy ascent:** Is Heavens #24’s unpaired arrival to fountain #132573 administrative access, while the knight’s ascent advice remains ambient? Preserve the unpaired admin edge; keep holy ascent outside the journal until a player-facing route and accepted access result are named.
4. **Hunting story:** Do Xero’s deer and trap descriptions promise a recoverable hunt, or set town context? Keep the bread-for-boots exchange as the sole accepted outcome unless a source-backed hunt result is selected.
5. **Arkelyn’s lesson:** Does returning the three distinct toys teach a skill, or is the actual cash/XP reward the whole exchange? Preserve the exact three-object input and current reward; do not infer performance history.
6. **Leodra’s quill:** Should the missing-quill rumor be implemented, or is the actual blank-book/autograph exchange intentional? Keep the accepted book exchange independent from the rumor.
7. **Animal rescue:** Should Rhed’s under-bed clue match the reset sack that supplies OBJ #132700? Choose truthful text or a deliberate hidden source; preserve the exact pup-object proof until then.
8. **Medicine finale:** Should the drider/healing prose imply production or personal recovery history? Keep Nebble’s preparation optional and Chiln’s accepted vial/pendant finale unchanged unless those events get explicit receipts.
9. **Food offering:** Is the homeless child’s typed ITEM_FOOD request meant to remain excluded support or become a durable story? Preserve its current exclusion and refusal behavior until selection, settlement, and continuation semantics are designed.
10. **Coin-only wand:** Should Menae’s 1,000-copper service become a durable objective under active accounting? Keep it a service/refusal until the exact debit and wand settlement are qualified.
11. **Missing mobile reset:** Which encounter was intended by ZON line 1052’s MOB #132677 reset in #132618? Resolve the missing mobile against its namespace and parent; never substitute OBJ #132677.
12. **Text-chest access:** Is the chest at #133050 intended to start locked with key zero, or should the reachable shelf at #133018 be the only text source? Keep both source records visible while testing reset state and access.
13. **Counter and container sources:** Should accepted paper/book/plant supplies remain under global first-matching-container reset behavior? Preserve existing supply identities, and confirm actual parent instance and cap behavior before crediting any item-source history.
14. **Heavens weapon #1111:** What is the intended repeatable source for this one Zech input? Preserve supplied valid copies and current source owner; do not add shop stock or a producer silently.
15. **Map seller:** Is Kabanon meant to sell a map? The crier’s description has no ordinary shop record attached to Kabanon. Confirm the intended stock and price before changing the shop list.
16. **Door-state differences:** Is key #2999 meant to unlock #132503/#132504 despite their closed-but-unlocked reset? Should #132511/#132528 and #132766/#132767 keep their asymmetric secret/door flags? Preserve the paired reset states until the intended initial and discovery behavior is confirmed.
17. **Switch and secret route:** Is pushing brick #132526, then searching the blocked/secret #132619/#132809 pair, the intended route? Keep push behavior and the current switch-target binding; qualify search and passage as a full journey before broader progress credit.
18. **Surface road joins:** Should Surface arrivals #133056/#133066 join the same roads as the outward endpoints #133064/#133070? Preserve both one-way pairs until the builder chooses the intended joins.
19. **Disconnected rooms, falls, and negative exit:** Are the 20-room tower component, ten-room #132632–#132641 pocket, three isolated rooms, and 31 fall-chance rooms intentional? Is #132641 north to `-1` an intended boundary, an obsolete placeholder, or an incomplete destination? Preserve the exact records and do not infer a safe route from raw reachability.
20. **Map and migration ownership:** Are Duris3’s 604 non-overlapping source-only approaches historical map traces? Preserve them on existing candidate row 36, exclude the 1,732 overlapping-source records from ingress, and keep Tharnadia DO NOT MOVE pending a separate owner decision.

21. **Twin Keeps room-text overlap:** Why do 68 standalone Caertannad room records have matching normalized titles and descriptions in the Tharnadia WLD, and should both copies remain? Preserve the separate active ranges and current room/reset ownership until both owners decide whether the repeated rooms are intentional parallel content or a stale copy. See [Checkpoint 709](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-709-caertannad-tharnadia-wld-content-overlap).

## Coverage references

- [Coverage row 297](ZONE_COVERAGE_REGISTER.csv#L297)
- [Connectivity checkpoint 384](WORLD_CONNECTIVITY.md#checkpoint-384-tharnadia-connectivity-and-duris3-approaches)
- [Title/history checkpoint 384](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-384-tharnadia-title-identity-and-history)
- [Retention checkpoint 384](ATTIC_MIGRATION_PLAN.md#checkpoint-384-tharnadia-retention-and-duris3-candidate-update)
- [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36)
- [Tharnadia source dossier](../zone-stories/THARNADIA.md)

## Bloodstone Checkpoint 778: reset and shop consumer

Active Tharnadia ZON and shop #132617 reference Bloodstone object #74203. Preserve its current ownership; see the [Bloodstone typed-dependency audit](BS_AUDIT_REVIEW.md#inbound-active-consumers).

## Checkpoint 839: Bloodstone input rechecked against current sources

The active Tharnadia reset and shop still share an incoming Bloodstone object
edge. Tharnadia MOB #132617 Kieta is reset at #132500; the following `G` row
places Bloodstone OBJ #74203 on the mobile. Local SHP #132617 also lists OBJ
#74203 as stock. Its prototype remains in Bloodstone's `bs.obj`. The same
numeric VNUM #74203 also names a separate Bloodstone MOB, `the graveside
mystic`; keep the mobile and wooden-walking-stick object in separate typed
roles. ([Tharnadia reset](../../../areas/zon/tharnadia.zon#L636),
[Tharnadia shop](../../../areas/shp/tharnadia.shp#L620),
[Bloodstone object](../../../areas/obj/bs.obj#L2630),
[Bloodstone mobile](../../../areas/mob/bs.mob#L2902),
[Bloodstone owner audit](BS_AUDIT_REVIEW.md#inbound-active-consumers))

This confirms and narrows the existing Checkpoint 778 note; it adds no new
prototype owner or WLD edge. Preserve the earlier active Lylr reset of local
counter OBJ #132581, the Arena reset of OBJ #67900, and the full CP384 typed
closure. Their placements and owners remain as recorded in this audit and the
[coverage register](ZONE_COVERAGE_REGISTER.csv#L297). No move, stock guarantee,
quest claim, or runtime-availability conclusion follows from these source
records.

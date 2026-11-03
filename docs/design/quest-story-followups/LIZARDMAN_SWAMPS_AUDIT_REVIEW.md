# Lizardman Swamps of Clavikord repository-wide audit supplement

This supplement reconciles the active Lizard WLD graph and its cross-area boundaries, the all-file inactive-map scan, typed outside references, compiled prototype bindings, and title/history evidence with the [source-comprehensive Lizardman Swamps dossier](../zone-stories/LIZARDMAN_SWAMPS_OF_CLAVIKORD.md). The frozen source snapshot is `72ba3deff3bb30bde880018cdebe7c6db675b697`. No area or gameplay source is changed by this audit.

## Active graph and WLD boundaries

The active [Lizard WLD](../../../areas/wld/lizard.wld#L1) contains 120 rooms (#6000–#6121) and 328 authored exits; all 328 targets resolve against active rooms. There are 326 local exits and two outbound cross-area exits. The local graph has weak components of 101 and 19 rooms. With local exits treated as available, raw directed traversal from #6000 reaches 100/120 rooms, and 101/120 can return to #6000. No local exit has the blocked flag; there are no negative or unresolved targets, local room-ID duplicates, fall chances, or current fields.

The authored outbound boundaries are reciprocal with Surface and the Underdark: Lizard #6098 south → Surface #634225 and Lizard #6120 north → Underdark #834369 ([Surface pair rows 249 and 532](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L249), [Underdark pair rows 250 and 714](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L250); [Lizard exits](../../../areas/wld/lizard.wld#L2755)). Two further active maps point in without a reverse Lizard WLD exit: Heavens #15 up → #6076 and Winterhaven #55402 southeast → #6087 ([Heavens row 175](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L175), [Winterhaven row 800](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L800)). The active WLD scan parsed 351 files; a single duplicate active room ID elsewhere does not overlap Lizard.

The raw WLD components omit the local type-25 pool controls: object #6010 in room #6102 targets #6103, and object #6011 in #6103 returns to #6102 ([pool definitions](../../../areas/obj/lizard.obj#L177), [return pool](../../../areas/obj/lizard.obj#L191)). Their configured travel is source evidence, not proof of actual admission, arrival, or safe return; the dossier follows their dispatch and route conditions.

## Inactive-map sweep

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without parser errors. It found no source-room VNUM overlap with Lizard and no inactive exit targeting rooms #6000–#6121. No source-only approach is added. Curated source-only totals remain 2,798 overall and 156 for the reviewed Surf subset. This result does not qualify the active Surface/Underdark entries, Heavens/WH inbounds, local pools, or administrative travel.

## Typed quest, reset, shop, and sidecar references

No outside QST item command targets a Lizard object. The broad exact-number scan does find `R C 6000` in active Solonar and `G C 6000` in active Verspin; those are currency amounts, not object-VNUM commands ([Solonar](../../../areas/qst/solonar.qst#L264), [Verspin](../../../areas/qst/verspin.qst#L105)). No local ZON reset imports an external prototype.

Active Randomness reset data puts local Sslith head #6008 on merchant #100015, and its active shop record lists #6008 as a product ([reset](../../../areas/zon/random.zon#L15), [shop product](../../../areas/shp/random.shp#L38)). These are configured sources; they do not establish available stock or a completed purchase. A separate unregistered `sylvandawn.zon` reset targets local object #6002, the wormrotted warning sign ([source-only reset](../../../areas/zon/sylvandawn.zon#L517)). Unregistered `tharnadia1.shp` and `tharold.shp` each list local objects #6007–#6009 in shop #6000 ([tharnadia1](../../../areas/shp/tharnadia1.shp#L1), [tharold](../../../areas/shp/tharold.shp#L1)); neither file is an active area contributor. Lizard's local shop record #6014 has no products ([Lizard shop source](../../../areas/shp/lizard.shp#L1)).

All 231 external story sidecars parsed without errors. No external typed `item_vnums` field points to a Lizard object. Nine broader digit-token matches in Solonar, Verspin, and Winterhaven resolve to `C:6000` currency text or embedded request identifiers containing 6012/6119, not typed Lizard VNUMs. The outside type-25 scan found 870 declarations and no target into a Lizard room. No external mobile or object prototype duplicates any of Lizard's 24 mobiles or 14 objects.

## Compiled references

Type-aware review of exact numeric matches in C/C++ source confirms three direct bindings. `tradeskill.c` includes mobile #6002 in `smith_array`; `initialize_tradeskills()` installs the `smith` handler in that mobile's primary slot when empty ([table row](../../../src/economy/tradeskill.c#L211), [binding](../../../src/economy/tradeskill.c#L958)); startup invokes the initializer from [comm.c](../../../src/net/comm.c#L960). In Lizard, mobile #6002 is the gigantic tako demon, while object #6002 is the distinct warning sign. This is a compiled mobile binding, not an object binding or evidence of a usable shop; the handler refuses FORGE while economic accounting is active ([guard](../../../src/economy/tradeskill.c#L793)). `epic_skills.c` maps mobile #6013, Bemon, to Expert Parry ([teacher row](../../../src/classes/epic_skills.c#L207)). `arena.c` assigns room #6075 as the Goodie arena hometown and later uses that location for return placement ([location table](../../../src/combat/arena.c#L51), [return use](../../../src/combat/arena.c#L417)). No direct compiled object-VNUM binding to a Lizard item was found. Other raw 6xxx matches are constants, currency, comments, hashes, or references interpreted in another prototype namespace.

## Title, history, and disposition

After stripping color codes and normalizing whitespace/case, “The Lizardman Swamps of Clavikord” occurs once across all 442 `.zon` files, in [lizard.zon](../../../areas/zon/lizard.zon#L2); it is also unique among the 351 active titles. Followed same-stem WLD history reaches the 2009 area-tree move and later 2011/2013 edits; no alternate-stem rename, distinct predecessor, or whole-zone replacement was established. Unregistered ZON/SHP references above do not establish replacement or active service.

`lizard` remains an un-commented contributor in both [the world manifest](../../../areas/AREA#L30) and [QST manifest](../../../areas/qst/AREA#L28); keep it **DO NOT MOVE**. Preserve `sylvandawn`, `tharnadia1`, and `tharold` as **RETAIN** candidates while their source-only typed references and broader tooling/editor closure remain unresolved. See [coverage row 160](ZONE_COVERAGE_REGISTER.csv#L160), [connectivity checkpoint](WORLD_CONNECTIVITY.md), [history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md), and boundary-pair rows linked above. Static review does not claim played head recovery, accepted hand-in, working shop purchase, successful teleport, guardian passage, or safe return.

## Checkpoint 262: builder questions and recommended resolutions

These questions translate the dossier's implementation boundaries into
decisions for the area and service owners. They preserve exact item identity,
source ownership, and ordinary movement until an owner chooses a separately
qualified change.

| # | Direct builder question | Source-backed recommended resolution |
| --- | --- | --- |
| 1 | Is Bemon's Q14 redemption intended to remain an independent exact Sslith-head #6008 return for bracelet #6009 followed by departure? | Preserve the exact offering and reward; do not require Vornin's receipt, a prior conversation, or a personal Sslith kill. |
| 2 | Is Vornin's Q40 revenge intended to remain an independent exact Bemon-head #6005 return for bracelet #6007 followed by departure? | Preserve its separate item and reward identity; do not make Bemon's earlier receipt a hidden prerequisite. |
| 3 | Should Bemon's carried #6005 be treated as a competing source and recipient episode because his own Q14 departure can remove it? | Keep the native reset/carry source and D behavior; accept another exact supplied or later sourced head, and require a separate owner decision before imposing a branch. |
| 4 | Is Randomness merchant #100015 intended to remain an alternate configured source for exact Sslith head #6008? | Preserve that source under Randomness; separate its M/G/shop data from personal king-defeat history and from an actually settled purchase. |
| 5 | Should ordinary CARVE's generic Limbo body part #8 remain distinct from accepted heads #6005 and #6008? | Keep both exact head prototypes as proof; do not treat generic carving or an unrelated body part as either receipt. |
| 6 | Should the requests continue to accept only the exact loose, directly carried head rather than held, equipped, or nested versions? | Preserve the authored custody boundary; qualify exact root, owner, and settlement separately without inventing personal source or provenance requirements. |
| 7 | Is the reward-and-D sequence intended to credit the receipt before Bemon's attempted carried-object extraction and removal? | Keep reward and departure as separate reported outcomes; qualify atomic offering, item ownership, pending extraction, rejection, replay, and cold recovery before claiming every consequence settled. |
| 8 | Should the two resettable contracts remain potential daily candidates when chance, caps, wandering, and source competition can make a giver or head unavailable? | Preserve the current disabled-by-default daily policy; do not advertise reliable renewal from a day change alone. |
| 9 | Are Bemon and Vornin meant to remain mobile from dispersal room #6099 with normal wander and STAY_ZONE rather than a fixed counter location? | Preserve native spawn and movement; resolve actual NPC generation and location before promising a meeting point. |
| 10 | Should Bemon's Expert Parry teacher binding remain separate from his QST handler and the head return? | Preserve the primary teacher and quest fall-through roles; do not credit training or promise purchase while active accounting refuses settlement. |
| 11 | Is Vornin's local shop #6014 intentionally empty while its quest handler remains active? | Keep the shop and quest roles separate; do not remove the binding or invent local products without the shop owner’s intent. |
| 12 | Should Randomness and local trade descriptions imply that head #6008 can be bought while active accounting refuses BUY/SELL? | Preserve source and shop declarations but describe purchase as unqualified until guarded trade and durable settlement work. |
| 13 | Are Surface #634225 and Underdark #834369 intended as the reciprocal public boundaries, with Heavens #15 and Winterhaven #55402 remaining distinct inbound declarations? | Preserve all four owners and disclose the two one-way inbounds as unqualified; a WLD edge does not prove public selector access or safe return. |
| 14 | Is the willow route's secret closed DOWN exit with key value zero intended to rely on SEARCH/reveal, OPEN, and separate passage? | Preserve its authored door state and qualify each successful step; do not add a key, kill, receipt, or learned-word gate. |
| 15 | Should pools #6010 and #6011 continue to use their local type-25 ENTER controls without head-return prerequisites? | Preserve object-owned travel and the physical route; record command admission and actual arrival separately. |
| 16 | Is Mukrok's high-level warning meant to remain atmospheric when the selected source paths show regeneration but no effective admission gate? | Preserve ordinary guardian combat and affected_by data; correct obsolete prose or design a fair explicit gate only after owner intent is established. |
| 17 | Is wight bracelet trap #6003's GET/PUT mask and unsupported damage code 12 an intentional spent/dud trap, obsolete data, or a missing effect? | Keep the current trap data until the owner chooses; do not silently create damage or change charge, level, mask, or pickup behavior. |
| 18 | Should the shrine, witchdoctor, tako/bodyguard, and growing-shadows stories remain optional lore without additional local QST receipts? | Keep the two accepted heads as the complete local contract set; add no rescue, cure, loyalty, guardian, or personal-kill achievement from dialogue alone. |
| 19 | Does Bemon's promise of future help intentionally have no later help exchange or temple-reinstatement endpoint? | Preserve the current text and D behavior until the builder adds an owned follow-up or approves a separate editorial correction. |
| 20 | Should the exact 6008 producer, 6005 competing source, recipient caps, and mode-1 reset remain separate availability episodes? | Preserve the source map and disclose uncertainty; qualify actual resets, wander, source movement, and renewal before projecting reliable supply. |

## Checkpoint 262 disposition

Keep active `lizard` **DO NOT MOVE**. Preserve its two independent exact-head
returns, current Bemon/Vornin source episodes, the reciprocal Surface and
Underdark pairs, the Heavens/Winterhaven one-way inbounds, and local pool and
willow controls. Keep the `sylvandawn`, `tharnadia1`, and `tharold` source-only
references on existing candidate rows 101, 103, and 104 **RETAIN IN PLACE**.
No inactive-WLD approach, new candidate, or native area/gameplay change is
indicated. Live access, mobile availability, purchase, trap, travel, and saved
receipt outcomes remain unqualified.

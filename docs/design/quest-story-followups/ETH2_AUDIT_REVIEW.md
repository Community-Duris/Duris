# The Fortress of Dreams repository-wide source audit

Checkpoint 216 maps active eth2 across its WLD, MOB, OBJ, and ZON sources; active and inactive WLD connections; reset-aware reach; typed item and key references; compiled VNUM use; and followed source history. Persisted reset values, installed player inventories, live item presence, and gameplay remain outside this static source audit. The [source dossier](../zone-stories/THE_FORTRESS_OF_DREAMS.md) summarizes the setting.

## Source identity and authored content

The active area roster lists eth2 with note *326-327. Its ZON header is #326, “The Fortress of Dreams,” last room #32710, reset mode 0. The active WLD has 111 rooms, MOB has 45 prototypes, and OBJ has 42. Its 158 non-S reset commands comprise 61 M, 12 O, 14 G, 27 E, 14 F, and 30 D commands. QST/AREA includes eth2. The local QST file, SHP file, story sidecar, and native Q/QA contract are absent ([area roster](../../../areas/AREA#L158), [QST index](../../../areas/qst/AREA#L147), [zone header and reset list](../../../areas/zon/eth2.zon#L1), [coverage row 91](ZONE_COVERAGE_REGISTER.csv#L91)).

The room text moves from the Deep and Border Ethereal through a Vortex and yellow fog to an obsidian fortress. It calls the fortress home of Aramus, Lord of the Ethereal Plane, and describes his throne as a place of nightmares and fantasies. A separated forest branch contains two sacred trees, an ancient tree, and a lost girl. The treasure-room description says its riches and weapons are illusory even though the zone resets equipment and a shared Heavens monolith there ([Vortex](../../../areas/wld/eth2.wld#L1669), [obsidian approach](../../../areas/wld/eth2.wld#L1731), [fortress](../../../areas/wld/eth2.wld#L1814), [forest](../../../areas/wld/eth2.wld#L2319), [throne room](../../../areas/wld/eth2.wld#L2836), [treasure room](../../../areas/wld/eth2.wld#L2860), [Heavens monolith reset](../../../areas/zon/eth2.zon#L140)).

## WLD connectivity, doors, and arrival

The active WLD census has 383/383 resolving exits, all local. Its weak components contain 57, 28, and 26 rooms; there are no fall fields or current fields. The sole active inbound WLD edge is Realm #14209 north to #32707, with no reverse exit and no outbound WLD edge. The exact active exit is [cross-area exit-register row 252](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L252), sourced from [Realm WLD](../../../areas/wld/realm.wld#L4696).

From arrival room #32707, all-exit directed WLD traversal reaches 12/111 rooms, and 26 rooms have a path back to #32707. Applying all 30 local D reset positions as unavailable leaves only #32707 reachable from the arrival and only that room able to return. The treasure room's south exit to the throne room uses key #32620 and resets closed and locked. These are static source graph calculations; the zone uses reset mode 0, and live door and reset state remain unqualified.

The WLD graph excludes six declared ITEM_TELEPORT targets. Local objects #32601, #32639, #32612, #32622, and #32640 are reset at #32656, #32660, #32682, #32707, and #32632; they target #32657, #32632, #32683, #32600, and Ethereal Main room #12494 respectively. Active Ethereal Main object #12410 is reset at #12511 and targets #32632 ([local resets](../../../areas/zon/eth2.zon#L129), [local teleport definitions](../../../areas/obj/eth2.obj#L19), [external target and reset](../../../areas/obj/ethereal_main.obj#L156), [external placement](../../../areas/zon/ethereal_main.zon#L17)). The source records these as item-mediated routes; reset declarations alone establish no live access.

## Reset keys and unresolved trapdoor fields

The 30 local D commands set 16 exits closed and 14 closed and locked. Six keyed pairs have local source prototypes and matching reset placements:

| Door pair | WLD key VNUM | Matching source placement |
| --- | ---: | --- |
| #32660 / #32661, brass entrance | #32604 | Ashahel carries the small brass key |
| #32669 / #32670 | #32606 | Yves carries a small glowing pendant |
| #32670 / #32671 | #32610 | A ravager assassin carries a bone key |
| #32675 / #32676 | #32638 | A death slaad carries a mold-encrusted key |
| #32692 / #32693, ancient tree | #32613 | The lost girl carries a wooden key |
| #32706 / #32707, throne and treasure rooms | #32620 | Aramus carries the rainbow key |

The cathedral-to-bell-tower trapdoor pair is inconsistent. The upward exit from #32703 to #32710 names key #32461, while the downward exit from #32710 to #32703 names #20541; both directions reset closed and locked. #32461 is an active BCTDL object called “solid mirror of Nessus.” The reviewed BCTDL sources contain no reset, shop, QST, or sidecar placement for that object. BCTDL's reset command targets mobile #32433 in room #32461; no command there loads object #32461. No OBJ prototype #20541 was found in the current object sources; the same number is a Pyramid WLD room. Source placement for either key remains unverified ([trapdoor exits](../../../areas/wld/eth2.wld#L2790), [door resets](../../../areas/zon/eth2.zon#L104), [BCTDL mirror](../../../areas/obj/bctdl.obj#L414), [same-numbered BCTDL room reset](../../../areas/zon/bctdl.zon#L253), [Pyramid room](../../../areas/wld/pyramid.wld#L3088)).

## Typed dependencies and compiled behavior

Exact role-aware source scans found these active relationships:

- Alatorin QST #32627 consumes the ethereal rose in an exchange; its story sidecar repeats the item contract and declares one rose carried by Mishanen. Alatorin ZON has a 2% G reset for object #32627 on Mishanen. The local eth2 ZON also resets the rose in the forest ([QST](../../../areas/qst/alatorin.qst#L3653), [story contract](../../../areas/story/alatorin.story.json#L1607), [Mishanen reset](../../../areas/zon/alatorin.zon#L2582), [local rose reset](../../../areas/zon/eth2.zon#L133)).
- Local ZON imports active Heavens object #360, the rune-covered monolith, into #32707 ([reset](../../../areas/zon/eth2.zon#L140), [prototype](../../../areas/obj/heavens.obj#L3741)).
- Active Chaos equipment tables name local objects #32624 (boots of the dreamer), #32628 (slaadi necklace), and #32630 (gloves of the adept). The random-equipment table names Aramus mobile #32637 ([equipment tables](../../../src/account/chaos_eq_data.h#L141), [Aramus entry](../../../src/item/randomeq.c#L140)).
- Compiled assignments bind local forest animals, the lost girl, demon princess, Aramus, and tree, Godsfury, and crown objects to handlers. Those handlers can create nightmare demons, replace the girl with the demon princess on death, awaken the tree mobiles, give Aramus helpers, and apply Godsfury and crown effects ([assignments](../../../src/specs/specs.assign.c#L2607), [forest and Aramus handlers](../../../src/specs/specs.eth2.c#L27), [tree and item handlers](../../../src/specs/specs.eth2.c#L221)).

Beyond the Alatorin rose use, scans found no active external QST item use or ZON reset-role reference to eth2 prototypes. They found no external SHP product, duplicate active MOB/OBJ prototype, or external WLD exit that uses an eth2 VNUM as its key. No other active type-25 target into eth2 was found beyond Ethereal Main object #12410. The occurrence of 32620 in specs.lohrr.c compares player IDs and is excluded from the eth2 VNUM references. The local QST/AREA entry has no matching local QST source.

## Source-only approaches and title/history

The all-95 inactive-WLD sweep found no room-number overlap or target exit into eth2. No candidate row or curated source-only WLD count changes; totals remain 3,460 overall, 221 for Surf, and 17 for ud_map2, with eight Underworld2 approaches.

After color stripping and normalization, “The Fortress of Dreams” is unique among 442 current .zon titles. Followed history stays on the eth2 stem: WLD through 2014-08-28, MOB through the August 2026 rebuild-warning cleanup, OBJ through 2015-11-10, and ZON through 2016-02-24. No QST, SHP, or story sidecar history exists because those local files are absent. No alternate-stem predecessor, retirement, or whole-zone replacement is established. Ethereal Main has a distinct title and is linked by portal targets.

Keep active eth2 **DO NOT MOVE**. Preserve its one-way Realm arrival, the portal target network, local lock and key VNUMs, Alatorin rose exchange, Heavens monolith, Chaos equipment references, and compiled prototype bindings. See the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-216-the-fortress-of-dreams-title-and-source-history), [coverage row 91](ZONE_COVERAGE_REGISTER.csv#L91), and [source dossier](../zone-stories/THE_FORTRESS_OF_DREAMS.md).

## Builder follow-up

1. Reconcile the #32703/#32710 trapdoor key mismatch and establish whether #32461 or another item is intended; #20541 has no current object prototype, and the active BCTDL mirror has no reset placement in the reviewed sources.
2. Confirm whether mode-0 resets and the six type-25 objects produce the intended arrival, connection, and return routes, especially from Realm #14209 into the locked treasure-room door.
3. Confirm whether local object prototypes #32625 and #32626 are intentionally unplaced.
4. Confirm whether the QST/AREA listing should remain while eth2 has no local QST.
5. Preserve the Alatorin rose contract, Heavens #360 reset, Chaos equipment references, and special-handler VNUMs when changing the source set.

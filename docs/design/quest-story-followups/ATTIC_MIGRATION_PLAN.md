# Candidate-specific attic migration plan

**Plan only. No source files were moved, copied, deleted, or renamed. No
candidate has been cleared for archival.**

## Current disposition

The normal full-area generator consumes the un-commented entries in
[`areas/AREA`](../../../areas/AREA). The source census contains 351 listed
contributors and 121 additional current source-component stems. The 121 are
outside that world list, but exclusion from the list proves only that their
files are not concatenated by the normal generator. It does not prove that
their quest prototypes, resets, shops, VNUMs, editor workflows, special
bindings, or cross-zone links have no owner.

The exact one-row-per-current-component review ledger is
[`ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv`](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv).
It names every component stem, the exact files present, quest-list membership,
possible inbound active exit references, and the remaining disposition. Four
additional history-only rows in the zone register have no current files to
move: `plane_fire_two`, `plane_fire_three`, `player_ships`, and `siege`.

### Candidate families with known blockers

| Candidate family | Current evidence | Safe-plan disposition |
| --- | --- | --- |
| cabal | Its 13 rooms and three mobiles reuse active Torg VNUMs for different identities. The source has two outbound WLD records, six Player Castles object reset imports, and one SHP record exposed to the non-mini `*.shp` fallback; keeper #29001 and room #29003 resolve to active Torg identities, while stock #29000, #29001, and #6089 has no current active OBJ prototype. The similarly titled `cabal_hall` has different VNUMs/content and no proven replacement relationship. See the [Cabal Guildhall audit](CABAL_GUILDHALL_AUDIT_REVIEW.md). | Retain in place. Resolve Torg ownership, outbound target identity, shop loader/keeper/room/stock, Cabal and `cabal_hall` relationship, editor/tooling, and persisted identity before a separate path proposal. |
| cabal_hall | Its ten room VNUMs #30100–#30109 are actively owned by Underworld3 and reused by inactive `nah` and `shadow_hall` maps. The unregistered shopkeeper #30101 has no active MOB prototype, while the shop room #30103 resolves to Underworld3; a compiled inn binding also names #30104. The source has a one-way exit to Duris3 #153434, external reset imports, a missing #6089 product, and explicit anti-LGBTQ violence and sexual/drug content. The 2026 ZON/SHP updates add active gathering bags #491–#494. See the [Cabal Hall audit](CABAL_HALL_AUDIT_REVIEW.md). | Retain in place. Resolve room and code ownership, source-map relationships, boundary, shop configuration and stock, content review, editor/tooling, and persisted identity before a separate path proposal. |
| casino | All 15 Casino room VNUMs #77801–#77815 are currently active Hall of the Ancients room identities with different content. Casino's 38 authored exits all target those same numbers; Hall has 23 authored exits into the range, which remain Hall routes and do not provide Casino ingress. Two resets import active Heavens object #45, and two door comments both say closed while raw D values differ (2 and 0). No Mob, SHP, QST, or C++ game handler was identified in the bounded scan. See the [Casino audit](CASINO_AUDIT_REVIEW.md). | Retain in place. Resolve Hall/Casino room ownership and persisted identity, both object #45 imports, door reset intent, and any separate gambling contract, then close editor/tooling before a separate path or activation proposal. |
| cbr_hall | All ten candidate rooms #29300–#29309 are active Newbie room identities with different content; candidate mobiles #29300–#29303 and object #29300 also collide with active Newbie prototypes. Compiled code binds an inn to active Newbie #29305 and assigns Ailvio handlers to #29303/#29304. The candidate's Duris3 boundary #152260 is one-way, and its SHP keeper, room, and #29300 stock share numeric IDs with Newbie if the non-mini fallback reads its unlisted SHP. Two Player Castles object imports and the 2026 bag stock updates remain source dependencies. See the [BattleRager Hall audit](CBR_HALL_AUDIT_REVIEW.md). | Retain in place. Resolve Newbie room/mobile/object ownership, compiled inn and Ailvio bindings, the Duris3 exit, conditional shop-loader identities, imported objects, reset/stock intent, editor/tooling, and persisted identity before a separate path or activation proposal. |
| bsghop | Its four rooms and mobiles #29440–#29443, plus object #29440, collide with different active Harrow records. darkop.wld repeats nearly the same room text and darkop.obj contains an alternate portal record. BSGHOP has a negative up exit, a zone-1/294xx mismatch, four reset placements at reused VNUMs, violent child-death prose, and a player-specific bulletin board. See the [BSGHOP audit](BSGHOP_AUDIT_REVIEW.md). | Retain in place. Resolve the VNUM owner, canonical map and portal, reset and loader intent, content review, editor/tooling, and persisted identity before a separate path proposal. |
| `surf`, `surface2011` | Each inactive WLD family contains 145 VNUM targets referenced by active exits from 17 active WLD areas. The same raw exits match both old files. The reviewed source-only `surf.wld` subset includes five Ugta, Nax, and Sea records; two exits into active Darkfall rooms; 53 into active Mistywood rooms; two into active Kimordril rooms; one from room 280727 west into Bugentol room 95800; two into Troll Caves rooms 96900 and 96903; one east exit into IceCrag room 97001; one south exit from room 229643 into Father Tel's Holy Cloister room 67100; one north exit from room 236469 into Turolopolis room 7102; three into Du'Maathe Castle rooms 37409, 37416, and 37425; seven into Tundra rooms 13703, 13712, 13778, 13796, 13870, 13895, and 13936; one into The Fields Between room 71001 (see the [Fields Between audit supplement](FIELDS_BETWEEN_AUDIT_REVIEW.md)), one west exit from room 274726 into The Town of Moregeeth room 70198 (see the [Moregeeth audit supplement](MOREGEETH_AUDIT_REVIEW.md)), plus one north exit from room 267058 into Ceothia room 80800 (see the [Ceothia audit supplement](CEOTHIA_AUDIT_REVIEW.md)), and one east exit from room 202208 into Desert City room 49003 (see the [Desert City audit supplement](DESERT_CITY_AUDIT_REVIEW.md)), plus one down exit from room 229308 into Basin Wastes room 34000 (see the [Basin Wastes audit supplement](BASIN_WASTES_AUDIT_REVIEW.md)); room 276166 also has a north exit into Nakral's Crypt room 14300 (see the [Crypt audit supplement](NAKRALS_CRYPT_AUDIT_REVIEW.md)); room 205053 also has a north exit into Val room 38575 (see the [Valoisian Castle audit supplement](VALOISIAN_CASTLE_AUDIT_REVIEW.md)); room 214566 also has a north exit into Harrow room 29400 (see the [Harrow audit supplement](HARROW_AUDIT_REVIEW.md)); rooms 277280 west, 277864 south, and 280870 north also lead to Mountain Tracts rooms 20951, 21110, and 20901 (see the [Mountain Tracts audit supplement](MOUNTAIN_TRACTS_AUDIT_REVIEW.md)); room 285640 adds one west exit into active Storm Port Stronghold room 22634 (see the [Storm Port Stronghold audit supplement](STORM_PORT_STRONGHOLD_AUDIT_REVIEW.md)); this unregistered record remains source-only, not active ingress; unregistered room 249084 also exits north to active Harpy Settlement room 31100 (see the [Harpy audit supplement](HARPYHT_AUDIT_REVIEW.md)), still source-only; rooms 285152 east and 285453 north both target active Behemoth Herders room 94300 (see the [Herders audit supplement](HERDERS_AUDIT_REVIEW.md)), also source-only; room 263869 adds a south exit into Temple of Flames room 18300 (see the [Temple audit supplement](TEMPLE_AUDIT_REVIEW.md)), also source-only; unregistered `surf.wld` room #215024 exits west into active Elemental Groves room #3800 (see the [Elemental Groves audit supplement](ELEMENTAL_GROVES_AUDIT_REVIEW.md)), source-only and not active ingress; room #229658 exits south into active Temple of the Earth room #43500 (see the [Temple of the Earth audit supplement](TEMPLE_OF_THE_EARTH_AUDIT_REVIEW.md)), also source-only; room #242869 exits north into active Githzerai Stronghold room #44400 (see the [Githzerai Stronghold audit supplement](GITHZERAI_STRONGHOLD_AUDIT_REVIEW.md)), also source-only. Unregistered room #246122 east exits into active Tribal Oasis room #78000 (see the [Tribal Oasis audit supplement](TRIBAL_OASIS_AUDIT_REVIEW.md)), source-only and not active ingress. Checkpoint 73 adds 70 source-only `surf.wld` exits into The Great Realm of Duris from 70 non-overlapping source rooms; Checkpoint 74 adds one west exit from `surf.wld` room #279354 into The Battlefield room #66400. Checkpoint 75 adds one east exit from `surf.wld` room #275277 into The Forgotten Mansion room #3500, plus one east exit from `underworld2.wld` room #436465 into the same active room. Checkpoint 76 adds two source-only `surf.wld` exits into Woodseer: room #226868 south to #16891 and room #228065 east to #16915, with no source-room overlap (see the [Woodseer audit supplement](WOODSEER_AUDIT_REVIEW.md)). Checkpoint 77 adds one source-only west exit from `surf.wld` room #264438 into Village of Refugees room #98600, with no source-room overlap (see the [Village of Refugees audit supplement](VILLAGE_OF_REFUGEES_AUDIT_REVIEW.md)). Checkpoint 78 found no source-only WLD approach into The Twin Towers. Checkpoint 79 adds four non-overlapping source-only `surf.wld` exits into The Deep Ravine of Passage: room #209185 north to #94878, room #250895 south and #251196 west to #94721, and room #252396 north to #94700 (see the [Deep Ravine audit supplement](DEEP_RAVINE_AUDIT_REVIEW.md)). Checkpoint 80 adds one source-only `surf.wld` exit from room #235767 north to Neverwind Valley room #20400 (see [Neverwind Valley audit supplement](NEVERWIND_VALLEY_AUDIT_REVIEW.md)). The reviewed Surf subset remains 115; Checkpoint 81 adds 400 source-only Duris3 exits into Grumbar's Domain, raising the global curated source-only total to 1085. With the separately recorded underworld2, underdark2011, and ud_map2 approaches, the source-only total through Checkpoint 80 was 685 (see the [Great Realm audit supplement](CONNECTORZONES_AUDIT_REVIEW.md), [Battlefield audit supplement](BATTLEFIELD_AUDIT_REVIEW.md), and [Mansion audit supplement](MANSION_AUDIT_REVIEW.md)). All listed exits remain source-only, not active-world ingress; the 115 individually reviewed `surf.wld` records include the Troll Caves through Mountain Tracts subset and the additional Storm Port Stronghold, Harpy Settlement, Behemoth Herders, Temple of Flames, Elemental Groves, Temple of the Earth, Githzerai Stronghold, Tribal Oasis, Great Realm, Battlefield, Forgotten Mansion, Woodseer, Village of Refugees, Deep Ravine of Passage, and Neverwind Valley boundaries. The unregistered `surf.zon` also has six `M` resets for Tundra mobile 13714 and two for mobile 13718; neither source group is active. Checkpoint 73 also found 209 `M` records for Great Realm mobile prototypes #53639, #53644, and #53675–#53688; these source-only resets are not active (see [Great Realm audit supplement](CONNECTORZONES_AUDIT_REVIEW.md)). The same-numbered `surface2011.wld` room 280727 points west to 280726. The same-numbered `surface2011.wld` bodies for Darkfall's two endpoints stay within that old map, while its Kimordril-numbered endpoints lead to #216661 and #216663. A full raw scan found 416 `surf.wld` exit targets matching active room VNUMs, including these reviewed records, pending map-source classification. Desolate Under Fire room #77301 has an unresolved south target #217609 that also matches rooms in `surf.wld`, `surface2011.wld`, and `underdark2011.wld`; these same-numbered rooms have different identities and exits. Keep that match unresolved; see the [Desolate Under Fire audit supplement](DESOLATE_UNDER_FIRE_AUDIT_REVIEW.md). Checkpoint 83 adds three non-overlapping source-only `surf.wld` exits into Caves of Mt. Skelenak (#213977 west to #4130, #214276 north to #4001, and #215488 west to #4131); the reviewed Surf subset is now 118 and the curated source-only total is 1088 (see [Caves audit supplement](CAVES_OF_MT_SKELENAK_AUDIT_REVIEW.md)). Checkpoint 84 adds two non-overlapping source-only `surf.wld` exits into Southern Coastal Highway (#238784 south to #41837 and #239989 west to #41732); the reviewed Surf subset is now 120 and curated source-only total 1090 (see [Highway audit supplement](SOUTHERN_COASTAL_HIGHWAY_AUDIT_REVIEW.md)). Checkpoint 87 adds one non-overlapping source-only `surf.wld` exit from room #216305 north to Strathor room #30800; the reviewed Surf subset is now 121 and the curated source-only total is 1092 (see [Strathor Valley audit supplement](STRATHOR_VALLEY_AUDIT_REVIEW.md)). Checkpoint 90 adds one non-overlapping source-only `surf.wld` exit from #248234 north to Forest City of Aravne #21500. Checkpoint 91 found no inactive-WLD approach into The Great Shaboath. Checkpoint 92 adds 144 non-overlapping Duris3 exits into The Altar of the Firesworn, and Checkpoint 93 adds 268 into the Royal Mausoleum of Castle IceCrag. Checkpoint 95 adds one source-only Surf approach from #263479 north to Prison #7497. Checkpoint 96 adds two source-only Surf approaches to Pits of Cerberus (#218489 south to #22118 and #219089 north to #22005). The reviewed Surf subset is 125 and the curated source-only total is 1509; all remain source-only, not active ingress. See the [Forest City of Aravne](FOREST_CITY_OF_ARAVNE_AUDIT_REVIEW.md), [Altar of the Firesworn](ALTAR_OF_THE_FIRESWORN_AUDIT_REVIEW.md), [Royal Mausoleum](ROYAL_MAUSOLEUM_OF_CASTLE_ICECRAG_AUDIT_REVIEW.md), [Prisons of Carthapia](PRISONS_OF_CARTHAPIA_AUDIT_REVIEW.md), and [Pits of Cerberus](PITS_OF_CERBERUS_AUDIT_REVIEW.md) audit supplements. Checkpoint 98 adds three non-overlapping source-only `surf.wld` approaches into Shady Grove (#282224 east to #97755, #282226 west to #97648, and #282525 north to #97717); the reviewed Surf subset is now 128 and the curated source-only total is 1,512 (see [Shady Grove audit supplement](SHADY_GROVE_AUDIT_REVIEW.md)). Checkpoint 99 adds one non-overlapping source-only `surf.wld` exit from room #271560 north into active Apocalypse Castle #34500 (see [Apocalypse Castle audit supplement](APOCALYPSE_CASTLE_AUDIT_REVIEW.md)); the reviewed Surf subset is now 129 and the curated source-only total is 1,513. Checkpoint 101 adds one non-overlapping source-only `surf.wld` exit from room #219584 west to active Moonshae room #26208; the reviewed Surf subset is now 130 and the curated source-only total is 1,514 (see [Moonshae audit supplement](MOONSHAE_ISLAND_AUDIT_REVIEW.md)). It remains source-only, not active ingress. Checkpoint 104 adds one non-overlapping source-only `surf.wld` exit from #266321 east into active Vargan room #2100; the reviewed Surf subset is 131 and curated source-only total is 1,518 (see [Vargan audit supplement](VARGAN_AUDIT_REVIEW.md)); this is not active ingress. Checkpoint 105 adds inactive `surf.wld` #281856 east into Maze entrance #94177; it remains source-only, raises the reviewed Surf subset to 132 and curated total to 1,519, and is not active ingress (see [Maze audit supplement](MAZE_OF_UNDEAD_ARMY_AUDIT_REVIEW.md)). Checkpoint 106 adds inactive `surf.wld` #220524 north into Bandit Camp #14800; it remains source-only, raises the reviewed Surf subset to 133 and curated total to 1,520, and is not active ingress (see [Bandit Camp audit supplement](BANDIT_CAMP_AUDIT_REVIEW.md)). Checkpoint 108 adds inactive `surf.wld` #220537 west to Cloud Giant entrance #99500; the reviewed Surf subset is 134 and the curated source-only total is 1,521. This is source-only, not active ingress; preserve the record and keep `surf` / `surface2011` **RETAIN** pending map ownership and dependency closure (see the [Cloud Giant Kingdom audit supplement](CLOUD_GIANT_KINGDOM_AUDIT_REVIEW.md)). Checkpoint 109 adds the non-overlapping source-only `surf.wld` exit #256676 west to Mountain entrance #9100; the Surf subset is 135 and the curated source-only total is 1,523 after adding this and one `oldmap8.wld` edge. This is not active ingress; see the [Mountain audit supplement](MOUNTAIN_OF_THE_BANISHED_AUDIT_REVIEW.md). Checkpoint 114 adds source-only `surf.wld` room #267236 down to active Goblin City room #23600; no source-room overlap (see [Goblin City audit supplement](MALCH_AUDIT_REVIEW.md)). The Surf subset is 140 and curated source-only total is 1,529. | Retain. Preserve the #220537 west approach into Cloud Giant #99500, #219584 west approach into Moonshae #26208, and #266321 east approach into Vargan #2100 as distinct source-only records. Resolve their intended map roles before any separate source move; neither is active ingress. Builders must decide which former Surface topology, if any, is relevant to each current exit and whether the source-only Ugta, Nax, Sea, Darkfall, Mistywood, Kimordril, Bugentol, Troll Caves, IceCrag, Cloister, Turolopolis, Du'Maathe Castle, Tundra, The Fields Between, The Town of Moregeeth, Ceothia, Desert City, Basin Wastes, Nakral's Crypt, The Valoisian Castle, Harrow, Mountain Tracts, Apocalypse Castle records, the new source-only Moonshae #26208 approach, and the Mountain #9100 approaches from `surf.wld` and `oldmap8.wld` still have intended meaning before retiring references or moving either source; classify the source-only Storm Port Stronghold, Harpy Settlement, Behemoth Herders, Temple of Flames, Elemental Groves, and Temple of the Earth and Githzerai Stronghold boundaries separately in any proposal.  Checkpoint 112 adds inactive surf.wld #253287 west to Chasm #15000; preserve this as a source-only record, separate from the active Surface pair. Surf subset 137; curated source-only total 1,526. See [Chasm audit supplement](MIST_CHASM_AUDIT_REVIEW.md). Preserve inactive surf.wld #265063 north to active Varathorn Keep room #99400 and #266570 down to #99439 as source-only approaches, not ingress; see [Kastle audit supplement](KASTLE_AUDIT_REVIEW.md). Preserve the #267236 down approach to Goblin City as source-only, not active ingress; see [Goblin City audit supplement](MALCH_AUDIT_REVIEW.md). Checkpoint 177 adds four non-overlapping source-only Surf approaches into Barrow of the Quiosho: #275979/#275980 east to #20600, #276595 west to #20611, and #277493 east to #20615; preserve them as source-only, not active ingress. See [Barrow audit supplement](ELFTOMB_AUDIT_REVIEW.md). Checkpoint 178 adds inactive surf.wld #270471 east to active Keep of Evil #44801; preserve it as source-only, not ingress. The reviewed Surf subset is 198 and curated total is 3,431. See [Keep audit supplement](EVKEEP_AUDIT_REVIEW.md). Checkpoint 179 adds inactive surf.wld #277907 north to active New Cave city #15102; preserve it as source-only, not ingress. The reviewed Surf subset is 199 and curated total is 3,432. See [New Cave city audit supplement](NEW_CAVECITY_AUDIT_REVIEW.md). Checkpoint 180 adds inactive surf.wld #248527 east to active Lost City of Tikitzopl #43901; preserve it as source-only, not ingress. The reviewed Surf subset is 200 and curated total is 3,433. See [Lost City audit supplement](TIKIT_AUDIT_REVIEW.md). |
| `torr` | Inactive torr.wld room #79092 exits south to active Chasm room #15052; this is a source-only approach and not current ingress (see [Chasm audit supplement](MIST_CHASM_AUDIT_REVIEW.md)). | Retain. Preserve its original file, VNUM, and direction; resolve map ownership and remaining dependency/tooling closure before any separate path decision. |
| myranth1 | QST has two missing giver mobiles (#70132/#70133), three item VNUMs without object prototypes, and a Moorney story echo in active Knight Zyla #70919. Its five SHP records bind active Moregeeth mobiles/rooms but all 19 distinct stock objects lack current object prototypes; #70107/#70108 collide by type with active warg mobiles. Normal manifests omit the files; the QST loader has no candidate glob, while the shop loader retains a conditional all-SHP fallback. See MYRANTH1_AUDIT_REVIEW.md. | Retain. Resolve the owner/story link, all giver/item prototypes, intended Moregeeth shop assignments, fallback and quest aggregate behavior, editor/tooling, and persisted identity before any separate move or archive decision. |
| `player_guilds` | Its unregistered SHP record #35554 names active Lava Caves room #35552 as its location; keeper mobile #35554 and produced-object #35551 have no matching prototypes in the current source set. The shop loader's fallback reads all `areas/shp/*.shp` files when `areas/world.shp` is missing; see the [Underground Lava Caves audit supplement](UNDERGROUND_LAVA_CAVES_AUDIT_REVIEW.md). The same candidate's SHP stock also has four source-only Tundra iron-ration entries (#13723). Checkpoint 97 finds nine unregistered SHP product rows in shops #34202, #34301, #34402, #35000, #35050, #35101, #35554, and #35952 for active Kimordril objects #95547, #95540, and #95536; these are possible fallback parser inputs, not confirmed operating stock (see [Kimordril audit supplement](KIMORDRIL_AUDIT_REVIEW.md)). | Retain. Resolve the LavCav room-location intent, missing keeper and stock prototypes, fallback exposure, and complete QST/shop/tooling closure before any separate move decision. |
| `tharnadia1`, `tharold` | Each unregistered SHP file has records labeled #6025 and #6066 whose room locations are Nomads rooms #6242 and #6243; the keeper mobile VNUMs do not resolve. Both files are excluded from `areas/AREA`, but the shop loader's fallback reads all `areas/shp/*.shp` files if `areas/world.shp` is missing. See the [Cimmerian Nomad audit supplement](CIMMERIAN_NOMAD_ENCAMPMENT_AUDIT_REVIEW.md). | Retain. Resolve the Nomads room-location intent, missing keeper prototypes, fallback exposure, and remaining source-owner and tooling questions before any separate move decision. |
| `Duris3` | 26 active exit targets from 14 WLD areas match this inactive WLD source. Separately, 88,256 source-only exit targets numerically match active room VNUMs, and 22,064 source-room VNUMs overlap active rooms; the individually reviewed 276 Fortress records are only a subset. The [Braddistock Mansion supplement](BRADDISTOCK_MANSION_AUDIT_REVIEW.md) classifies another 192 source-only exits in four bands targeting rooms 135001–135048 and 48 overlapping source-room VNUMs. Checkpoint 81 adds 400 non-overlapping `Duris3.wld` exits into Grumbar's Domain, plus 199 same-numbered but differently titled Ocean room records; these are source-only map references, not active ingress. See the [Grumbar's Domain audit supplement](GRUMBARS_DOMAIN_AUDIT_REVIEW.md). Checkpoint 101 confirms Moonshae port #26200 north/south exits to #148979/#149179 match only inactive Duris3 records and are pruned by the full loader; the intended current port destinations need builder resolution before any separate map or route move (see [Moonshae audit supplement](MOONSHAE_ISLAND_AUDIT_REVIEW.md)). Checkpoint 124 adds 462 non-overlapping inactive `Duris3.wld` exits into all 229 Mitashi rooms; 454 additional target records begin in source-room VNUMs overlapping #138200–#138428 and remain separate map-identity evidence. The curated source-only total is 1,997 and Surf remains 144; see the [Mitashi audit supplement](MITASHI_AUDIT_REVIEW.md). | Retain. Resolve every current destination against the active source owner and current map before any move; classify map-grid VNUM overlap separately from authored routes. For Moonshae, determine whether port #26200 north/south to #148979/#149179 are intended current routes before moving Duris3; the active exits are pruned by the full loader. See the [Moonshae audit supplement](MOONSHAE_ISLAND_AUDIT_REVIEW.md). |
| `newbiemaps`, `surface2011-temp` | Raw unregistered-WLD scans found 239,212 and 240,016 exit targets, respectively, that numerically match active room VNUMs. Their source-room VNUMs also overlap active rooms by 59,851 and 60,004, respectively. | Retain. Establish each map's provenance, active-room identity overlap, and intended relation to current geography before classifying any match as a route or considering a path move. |
| `ud`, `underdark2011` | 18 and 11 potential active exit targets respectively; some VNUMs are shared with current sparse map sources. Separately, `underdark2011.wld` room 272717 has a source-only down exit to active Troll Caves room 96900, room 224716 has an east exit to IceCrag room 97001, room 249233 has an east exit to Behemoth Herders room 94300, and room 249534 has a north exit to Herders room 94300, and room #255552 exits north to Githzerai Stronghold room #44400; all five records remain source-only. | Retain. Trace the `mobs_underdark`, `patrols`, `connectorzones`, and other exact inbound edges to their intended active map/controller; preserve all five reviewed source-only boundary records to Troll Caves, IceCrag, Behemoth Herders, and Githzerai Stronghold in any separate path proposal. This source also contains a same-numbered cave room #217609 targeted by the unresolved south exit from active Desolate Under Fire room #77301. `surf.wld` and `surface2011.wld` contain different room identities at the same VNUM; no current route owner is established. See the [Desolate Under Fire audit supplement](DESOLATE_UNDER_FIRE_AUDIT_REVIEW.md). |
| `underworld2` | The unregistered WLD source contains a north exit from room 417516 to active Father Tel's Holy Cloister room 67100 and a north exit from room 422044 to active Nakral's Crypt room 14300; room 437928 south exits into active Temple of the Earth room 43500 (see the [Temple of the Earth audit supplement](TEMPLE_OF_THE_EARTH_AUDIT_REVIEW.md)); room #421439 exits east into active Githzerai Stronghold room #44400 (see the [Githzerai Stronghold audit supplement](GITHZERAI_STRONGHOLD_AUDIT_REVIEW.md)); room #436465 east exits into active Forgotten Mansion room #3500 (see the [Mansion audit supplement](MANSION_AUDIT_REVIEW.md)). All six are source-only, not active ingress, including room #427709 east to Pits of Cerberus room #22118 (see the [Pits audit supplement](PITS_OF_CERBERUS_AUDIT_REVIEW.md)). | Retain. Preserve all six source-only references while map ownership, intended connections, typed dependencies, and tooling closure remain unresolved. |
| `ud_map0`, `ud_map1`, `ud_map2` | Four active Shafts exits target VNUMs found in these inactive WLD sources. A full source-only WLD scan found six ud_map0 exits into active maps (three to Shafts, one to Myconid #2300, one to Underworld #4469, and one to Ghore #11548), as well as two ud_map2 exits into active Ixarkon room 96492, one east exit from room 1124844 into active Worms room 6900, and one up exit from room 1124854 into active Khildarak room 17000; all four are source-only, not active-world ingress. Worms lies on a directed 50-exit ud_map2 source path from Shafts room 10012's unresolved target #1121446 (see [Worms audit supplement](WORMS_AUDIT_REVIEW.md)); the Khildarak approach is documented in the [Khildarak audit supplement](KHILDARAK_STRONGHOLD_AUDIT_REVIEW.md). Checkpoint 103 adds three non-overlapping source-only `ud_map2.wld` exits into active Lair of the Purple Worm: #1123784 west to #42525, #1123975 east to #42542, and #1124682 north to #42500. Checkpoint 117 adds ud_map2.wld #1125006 down to Gibber entrance #8700. All eight ud_map2 approaches are source-only, not active ingress; the active Shafts #10012 south exit to #1121446 remains a separate unresolved reference. See the [Lair audit supplement](LAIR_OF_THE_PURPLE_WORM_AUDIT_REVIEW.md) and [Gibber audit supplement](GIBBERLING_KING_AUDIT_REVIEW.md). | Retain. Preserve the active Shafts targets, all six ud_map0 source-only exits, its 26 source-only M commands, and the ud_map2 source-only approaches recorded elsewhere while confirming destination ownership, map-generator roles, and remaining typed/tooling/persisted dependencies. Checkpoint 549 adds the Ghore #11548 approach and raises curated inactive-WLD totals to 11,988 overall / 415 from Surf. Checkpoint 550 records seven source-only ud_map1 approaches into active maps and four source-only Underworld #4520 M resets; the new Valinhav #90800 edge is not active ingress. Curated totals rise to 11,989 overall / 415 from Surf. See [ud_map1 audit](UD_MAP1_AUDIT_REVIEW.md). No source-path move is cleared.  Checkpoint 551 records the four newly reconciled source-only approaches from ud_map2.wld: #1123442 east to Llzazan #36200, #1123855 east to Valinhav #90800, #1124947 north to Clawed Caverns #80700, and #1126563 north to Truk #1600. The reviewed ud_map2 subset is now 27; curated total is 11,993 overall / 415 from Surf. These are not active ingress; retain them with the map pending owner and tooling review. See [ud_map2 audit](UD_MAP2_AUDIT_REVIEW.md). |
| `oldmap8` | The unregistered WLD has two source-only exits into active Du'Maathe Castle rooms 37409 and 37416 and one east exit from #184183 into active Mountain entrance #9100; see the [Du'Maathe Castle audit supplement](DU_MAATHE_CASTLE_AUDIT_REVIEW.md) and [Mountain audit supplement](MOUNTAIN_OF_THE_BANISHED_AUDIT_REVIEW.md). | Retain. Establish the map's provenance and intended relationship to current castle and Mountain routes before any path move; preserve all three records as source-only, not active ingress. |
| `brass-old-2` | The 2025 relabel preserves the former `plane_fire_one *250-251` WLD/MOB/OBJ/ZON as this unlisted source; current active `plane_fire_one` is *254-255. Seven active exits from Potions and Realm target Old 2 room #25087. Its ZON has 66 Old 3 imports, and Old 3 returns 30 M resets to Old 2 mobile #25000; Old 3 WLD room #25341 also points to Old 2 #25001. Two external object VNUMs, two missing mobile prototypes, and code references remain. | RETAIN IN PLACE. Reconcile the stale QST index range and resolve the seven active room references, both directions of the source-only WLD/ZON relationship with Old 3, source-code references, missing mobile resets, editor/tooling, and persisted identity before any path decision. See the [Brass Old 2 audit](BRASS_OLD_2_AUDIT_REVIEW.md) and [builder questions](BRASS_OLD_2_BUILDER_QUESTIONS.md). |
| mavulsk, dalvik | Thetis #38095 north → #104 is unresolved in the active loader; #104 appears in both inactive sources (see the [Thetis audit supplement](THETIS_AUDIT_REVIEW.md)). | Retain. Establish the intended current destination and which legacy source, if either, owns it. |
| `brass-old-1` | The 2012 New Brass source was disconnected and its 2025 `areas/AREA` name changed to `brass`; the retained old files map to active Brass by −100 across all 144 mobiles, 150 objects, seven QST givers, 18 SHP keys, and 356/357 rooms. Its old gate boundary maps to the existing active Brass/Fire pair. The 66 source-only ZON resets, old QST #55271, and 18 fallback-visible shops remain dependencies. | RETAIN. Resolve the non-mini SHP fallback, 17 missing shopkeepers, owner/editor/persisted identity, and source-only dependencies before any path move or deletion. See the [Brass Old 1 audit](BRASS_OLD_1_AUDIT_REVIEW.md#full-mode-shop-loader-path-and-risk) and [builder questions](BRASS_OLD_1_BUILDER_QUESTIONS.md). |
| `brass-old-3` | Five exact renames preserve the former `plane_fire_two *252-253` WLD/MOB/OBJ/ZON/QST. Old 3 has a one-way source-only WLD edge into Old 2 and 30 M resets to Old 2; Old 2 has 66 reverse typed resets. Its map also points to current Fire #25455. Active Bahamut, Celestia, and Tiamat reset sources name its unresolved objects #25106/#25103; the old QST index row remains under `plane_fire_two`. | RETAIN IN PLACE. Resolve the orphaned QST index, one-way WLD links, reciprocal source-only resets, active reset owners, the Yodono contract’s receive/reward mismatch, code references, editor/tooling, and persisted identity before any future path decision. See the [Old 3 audit](BRASS_OLD_3_AUDIT_REVIEW.md), [builder questions](BRASS_OLD_3_BUILDER_QUESTIONS.md), and [candidate row 19](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L19). |
| `nexus56` | Its unregistered ZON resets and SHP sale-stock lists reference Ghore object VNUMs #11552, #11553, and #11589, plus Willem biography book #7129; no active WLD exit targets its rooms. Exact Ghore source lines are recorded in [Ghore's review](GHORE_STORY_REVIEW.md), and the Willem references are in the [Turolopolis audit supplement](TUROLOPOLIS_AUDIT_REVIEW.md#direct-external-typed-references). Checkpoint 51 also finds Undermountain rum barrel #92050 in its source-only ZON reset and SHP stock; keeper #83039 is in the source-only `nexus56.mob` file. The shop-loader fallback may parse the SHP when `areas/world.shp` is unavailable, but this does not establish an operating shop (see the [Undermountain audit supplement](UNDERMOUNTAIN_AUDIT_REVIEW.md)). Checkpoint 59 also found source-only Citadel book objects #13038/#13039 in `nexus56` SHP stock and ZON resets; see the [Citadel audit supplement](CITADEL_AUDIT_REVIEW.md). Checkpoint 71 also found Pharr Valley apple #40208 in unregistered `nexus56.shp` shop #83032 and `nexus56.zon` G reset (see the [Pharr Valley audit supplement](PHARR_VALLEY_AUDIT_REVIEW.md)). Checkpoint 77 adds the source-only `nexus56.shp` shop #83039 product and `nexus56.zon` G reset for Village of Refugees waterskin #98618; the loader fallback may read the shop when `areas/world.shp` is unavailable, and this checkout has no such file (see the [Village of Refugees audit supplement](VILLAGE_OF_REFUGEES_AUDIT_REVIEW.md)). Checkpoint 97 finds unregistered shop #83032 products #95510/#95508 and matching G resets for active Kimordril objects; qualify this conditional shop-reader exposure before any path move (see [Kimordril audit supplement](KIMORDRIL_AUDIT_REVIEW.md)). Checkpoint 98 finds source-only shop #83032 product #97521 and its matching G reset in `nexus56.zon` for active Shady object #97521; preserve both records pending owner and fallback qualification (see [Shady Grove audit supplement](SHADY_GROVE_AUDIT_REVIEW.md)). Checkpoint 139 also finds source-only `nexus56.zon` G reset and `nexus56.shp` shop #83038 product for Domain book #36822; preserve both pending owner and shop-reader fallback qualification (see [Domain audit supplement](DLSC_AUDIT_REVIEW.md)). | Retain. Complete the remaining exact reset/shop, code, editor/tooling, and identity closure for the whole source group before any separate owner decision about a path move. |
| `shady2` | Unregistered `shady2.mob` defines 90 mobile VNUMs #97500–#97589 also defined in active `shady.mob`; the active file adds #97590–#97605. Five shared alias lists differ, and no shared record block is line-identical. Active Shady and Alatorin QST/story item IDs #97514, #97549, #97570, #97572, and #97582 resolve to active Shady OBJ records, despite the numeric overlap with candidate mobiles; #97549 also names active Shady and candidate baby-orc mobiles. Its separate 2009 tracked addition and these cross-type matches do not establish a predecessor or replacement. See the [Shady Grove audit supplement](SHADY_GROVE_AUDIT_REVIEW.md) and [shady2 audit](SHADY2_AUDIT_REVIEW.md). | Retain. Preserve separately; confirm source-owner intent, typed item contracts, loader/editor/tooling consumers, and persisted-reference closure before any merge or path decision. |
| `IC3` | Four current uppercase source components remain after its 2026 removal from `areas/AREA`. | Retain. Confirm all numeric and tooling references and complete its environmental/story review before a separate owner decision. |
| `asunwa`, `jungle` | Both remain in the historical areas/qst/AREA list but are absent from the production areas/AREA input. Asunwa has QST/shop data; Jungle has no areas/qst/jungle.qst, and its full AREA entry was commented in April 2026. | Retain. Treat Jungle’s QST/AREA row as stale-looking metadata pending its owner; preserve its source-only Surface and Duris3 routes, mode-1 resets, and movement fields while story, editor, deployment, and persisted identity are reconciled. |
| Other unregistered component stems | Listed with exact file sets in the candidate ledger. They include current QST/shop/story or shared-data sources as well as full WLD/MOB/OBJ/ZON groups. | Retain pending typed closure. No component is approved based only on being absent from `areas/AREA`. |

The WLD-only pass found 201 unique active raw exit records whose targets match
at least one current unregistered WLD source. Because some old maps contain the
same VNUMs, those become 358 file-specific matches across eleven candidate
files. The exact links are recorded in
[`WORLD_UNRESOLVED_EXIT_REGISTER.csv`](WORLD_UNRESOLVED_EXIT_REGISTER.csv).
This is an inbound-reference screen only; it does not cover all dependency
types.

## Closure required for each candidate stem

Complete these checks for one exact source stem at a time and record the
evidence in the candidate ledger before requesting any archive move:

1. **Identify the full source group.** Record exact case-sensitive WLD, MOB,
   OBJ, ZON, QST, SHP, and story files. Include renames, alternate generators,
   and adjacent replacement stems from Git history.
2. **Resolve inbound references.** Search active WLD exits, ZON resets and
   prototypes, native Q/QA offered/reward terms, shop stocks, map/connector
   inputs, literal and computed special assignments, and compiled C/C++ VNUM
   references. Record every match and distinguish an active reference from a
   stale comment or an unlisted source copy.
3. **Resolve outbound dependencies.** Trace every item/mobile/room produced or
   consumed by a native Q contract, reset, shop, portal, ship/vehicle feature,
   script, or other unregistered source. Keep exact VNUM identity and current
   owner; equal names do not authorize substitution.
4. **Check tooling and generation.** Review root and area Makefiles, generators,
   scripts, editor configuration, lookup/map inputs, case-sensitive behavior,
   and path-based imports. `areas/qst/AREA` is not the normal QST loader list,
   but may still be read by other tools.
5. **Preserve runtime and persisted identity.** Do not inspect private player
   data or query a database as part of this audit. If a proposed move changes
   runtime VNUM meaning or blocks normal recovery, stop and request the needed
   owner evidence; never silently reassign a room or item ID.
6. **Get an explicit owner decision.** Only after the prior rows close, propose
   a separate path-only Git move preserving all files and their internal
   layout. Keep active `areas/AREA` membership unchanged unless a separately
   authorized zone change says otherwise.

## If a later move is approved

Keep each source group intact under an owner-selected attic path, preserving
letter case, VNUMs, comments, source bytes, and the Git rename history. Put the
move in its own commit with a stem-by-stem explanation and completed inbound
and outbound evidence. Confirm no production generator or editor still reads
the old path. The already retired attic tree stays deleted unless the owner
explicitly chooses a destination and structure for a new archive.

## Explicit no-move cases

- All 351 un-commented `areas/AREA` contributors: **not archive candidates**.
- `plane_fire_two`, `plane_fire_three`, and `player_ships`: no current source
  files; preserve the history-only crosswalk instead of recreating files.
- `siege`: source files were deleted during retirement and zone number 4010 is
  reserved; do not restore or reuse it.
- The deleted historical attic tree: do not restore it or inspect its private
  contents for this plan.

No source move, test, build, gameplay journey, database operation, migration,
or production action is part of this documentation checkpoint.

### Checkpoint 103 source-only total
Three non-overlapping `ud_map2.wld` exits into active Lair of the Purple Worm rooms #42525, #42542, and #42500 raise the current curated source-only approach total from 1,514 to 1,517. The records remain inactive-source approaches, not active ingress; the `ud_map2` candidate remains **RETAIN** pending owner, map-generator, and dependency review. See the [Lair of the Purple Worm audit supplement](LAIR_OF_THE_PURPLE_WORM_AUDIT_REVIEW.md).


### Checkpoint 104 source-only total
One non-overlapping inactive `surf.wld` exit from #266321 east into active Vargan room #2100 raises the reviewed Surf subset from 130 to 131 and the curated source-only approach total from 1,517 to 1,518. It remains source-only, not active ingress; the `surf`/`surface2011` candidate remains **RETAIN**. See the [Vargan audit supplement](VARGAN_AUDIT_REVIEW.md).

### Checkpoint 105 source-only total

One non-overlapping inactive `surf.wld` exit from #281856 east into active Maze entrance #94177 raises the reviewed Surf subset from 131 to 132 and the curated source-only approach total from 1,518 to 1,519. It remains source-only, not active ingress. The inactive `brass-old-1.zon` G reset to Maze object #94009 is a separate typed source reference and does not change the WLD total. Keep both candidate groups **RETAIN** pending map ownership and full dependency closure. See the [Maze audit supplement](MAZE_OF_UNDEAD_ARMY_AUDIT_REVIEW.md).

### Checkpoint 106 source-only total

One non-overlapping inactive `surf.wld` exit from #220524 north into active Bandit Camp room #14800 raises the reviewed Surf subset from 132 to 133 and the curated source-only approach total from 1,519 to 1,520. It remains source-only, not active ingress. Preserve the source record in the `surf` candidate review; the source is **RETAIN** and no movement is cleared. See the [Bandit Camp audit supplement](BANDIT_CAMP_AUDIT_REVIEW.md).


### Checkpoint 108 source-only total

One non-overlapping inactive `surf.wld` exit from #220537 west into active Cloud Giant entrance #99500 raises the reviewed Surf subset from 133 to 134 and the curated source-only approach total from 1,520 to 1,521. It is a source-only map record, not active ingress. Preserve the route and keep `surf` / `surface2011` **RETAIN** pending map ownership and full dependency closure; Cloud Giant remains **DO NOT MOVE**. See the [Cloud Giant Kingdom audit supplement](CLOUD_GIANT_KINGDOM_AUDIT_REVIEW.md).

### Checkpoint 109 source-only total

The all-95 inactive-WLD scan adds two non-overlapping source-only approaches into Mountain entrance #9100: `surf.wld` room #256676 west and `oldmap8.wld` room #184183 east. The reviewed Surf subset rises from 134 to 135 and the curated source-only total from 1,521 to 1,523. Both are inactive-map records, not active ingress. Preserve each with its source map; retain `oldmap8`, `surf`, and `surface2011` until map ownership and dependency closure are complete. Mountain remains **DO NOT MOVE**. See the [Mountain audit supplement](MOUNTAIN_OF_THE_BANISHED_AUDIT_REVIEW.md).


### Checkpoint 110 typed dependency

The inactive-WLD scan found no source-only approach into Tiamat, so the curated WLD-edge total does not change. Separately, active `tiamat.zon` names archived object #25106 from inactive `brass-old-3.obj`; the loader disables that unresolved reset command, so it does not create current stock. Keep `brass-old-3` **RETAIN** until builder intent and typed-reference closure are resolved. Do not reactivate the archived source to satisfy the stale reset. See the [Tiamat audit supplement](TIAMAT_AUDIT_REVIEW.md).

### Checkpoint 111 source-only approach

The all-95 inactive-WLD scan found one non-overlapping source-only `surf.wld` exit: #262357 north to active Obsidian Citadel entrance #75600. It does not create current ingress. The reviewed Surf subset rises from 135 to 136 and the curated source-only approach total from 1,523 to 1,524. Preserve this edge with its source map; keep `surf` and `surface2011` **RETAIN** until map ownership and full dependency closure are complete. Obcita remains **DO NOT MOVE**. See the [Obcita audit supplement](OBCITA_AUDIT_REVIEW.md) and the [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 112 source-only approaches

The all-95 inactive-WLD sweep found surf.wld #253287 west to active Chasm #15000 and torr.wld #79092 south to active Chasm #15052. Neither source room overlaps Chasm's 100 active room VNUMs; neither source is selected by areas/AREA. Preserve both original records as source-only, not active ingress. This raises the Surf subset to 137 and the curated source-only approach total to 1,526. Keep surf / surface2011 and torr **RETAIN**; keep active mist_chasm **DO NOT MOVE**. Map ownership, safe access, and remaining candidate dependency/tooling questions are unresolved. See the [Chasm audit supplement](MIST_CHASM_AUDIT_REVIEW.md), [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [Torr candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L107).

### Checkpoint 113 source-only approaches

The all-95 inactive-WLD sweep found two non-overlapping surf.wld approaches into active Varathorn Keep: #265063 north to #99400 and #266570 down to #99439. Neither source room overlaps Kastle's 100 active room VNUMs, and surf is absent from areas/AREA; both remain source-only, not active ingress. This raises the reviewed Surf subset to 139 and the curated source-only approach total to 1,528. Keep surf and surface2011 **RETAIN**; keep active kastle **DO NOT MOVE**. Preserve both original records until map ownership and remaining dependency/tooling questions receive review. See the [Kastle audit supplement](KASTLE_AUDIT_REVIEW.md) and [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 114 source-only approach

The all-95 inactive-WLD sweep found one non-overlapping `surf.wld` exit: room #267236 down to active Goblin City room #23600. `surf` is absent from `areas/AREA`, so this remains source-only and is not active ingress. The reviewed Surf subset rises to 140 and the curated source-only approach total to 1,529. Preserve the original edge with `surf.wld`; keep `surf` and `surface2011` **RETAIN**, and keep active `malch` **DO NOT MOVE**. See the [Goblin City audit supplement](MALCH_AUDIT_REVIEW.md) and [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 115: Miaeril has no inactive-WLD approach

The all-95 inactive-WLD sweep found no source-room overlap with Miaeril and no inactive edge into its 65 active rooms. The curated source-only approach total remains 1,529 and the reviewed Surf subset remains 140. This adds no attic candidate and clears no existing one. Keep active `mril` **DO NOT MOVE**; retain `surf` and `surface2011` pending map ownership and dependency/tool closure. See the [Miaeril audit supplement](MIAERIL_AUDIT_REVIEW.md) and [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 116: inactive Surf approach and source-only brass reset

The all-95 inactive-WLD scan found one non-overlapping `surf.wld` exit from #237595 west into active Temple room #82493. It is source-only, not active ingress; the reviewed Surf subset rises to 141 and the curated source-only total to 1,530. Preserve the edge with its source-map identity; keep `surf` and `surface2011` **RETAIN** and active `suntmpl` **DO NOT MOVE**. Separately, unregistered `brass-old-1.zon` has a source-only G reset for active Temple object #82416 at line 520. This is not active stock, but it adds a typed dependency to the existing `brass-old-1` candidate record. Retain that candidate pending full source/dependency/tool closure; no map move is cleared. See the [Temple audit supplement](TEMPLE_OF_THE_SUN_AUDIT_REVIEW.md), [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [brass-old-1 candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17).

### Checkpoint 117: inactive `ud_map2` approach

The all-95 inactive-WLD sweep found one non-overlapping source-only `ud_map2.wld` exit: room #1125006 down to active Gibber entrance #8700. It is not active ingress and raises the curated source-only total from 1,530 to 1,531; the reviewed Surf subset remains 141. This is the eighth `ud_map2` approach into active areas. Preserve all eight source-only approaches and the separate active Shafts target while map ownership and dependency/tooling closure remain incomplete. Keep `ud_map2` **RETAIN** and active `gibber` **DO NOT MOVE**. See the [Gibber audit supplement](GIBBERLING_KING_AUDIT_REVIEW.md), [`ud_map2` candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114), and [`ud_map2` coverage row](ZONE_COVERAGE_REGISTER.csv#L465).

### Checkpoint 118: inactive Surf approach into Ice Tower

The all-95 inactive-WLD sweep found one non-overlapping source-only `surf.wld` exit: room #206576 north to active Ice Tower entrance #31800. It is not active ingress. The reviewed Surf subset rises from 141 to 142, and the curated source-only total rises from 1,531 to 1,532. Preserve the record with its source map; keep `surf` and `surface2011` **RETAIN** pending map ownership and dependency/tool closure. Active `icetower` remains **DO NOT MOVE**. See the [Ice Tower audit supplement](ICE_TOWER_AUDIT_REVIEW.md), [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [Ice Tower coverage row](ZONE_COVERAGE_REGISTER.csv#L132).

### Checkpoint 119: Durian Underdark has no inactive-WLD approach

The all-95 inactive-WLD sweep found no room overlap and no exit into the current active Underdark. The reviewed Surf subset remains 142 and the curated source-only approach total remains 1,532. This adds no attic candidate and clears no existing one. Preserve active `underdark` as **DO NOT MOVE**. Keep `underdark2011` **RETAIN IN PLACE**: its 11 active WLD inbound references and the conflicting #217609 room identity remain unresolved. See the [Underdark audit supplement](DURIAN_UNDERDARK_AUDIT_REVIEW.md), [coverage row](ZONE_COVERAGE_REGISTER.csv#L324), and [candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L115).

### Checkpoint 120: Bahamut has no inactive-WLD approach

The all-95 inactive-WLD sweep found no room overlap or exit into active Bahamut. This adds no source-only approach and no attic candidate; the curated total remains 1,532 and the reviewed Surf subset remains 142. Preserve active `bahamut` as **DO NOT MOVE**. No path was moved and no existing candidate was cleared. See the [Bahamut audit supplement](BAHAMUT_AUDIT_REVIEW.md) and [coverage row](ZONE_COVERAGE_REGISTER.csv#L22).

### Checkpoint 121: inactive `ud_map2` approach into Clan Stoutdorf

The all-95 inactive-WLD sweep found one non-overlapping source-only `ud_map2.wld` exit: room #1121983 west to active Stoutdorf entrance #34200. It is not active ingress. The curated source-only approach total rises from 1,532 to 1,533; the reviewed Surf subset stays 142. This is the ninth recorded `ud_map2` approach into active areas, separate from its unresolved active Shafts target. Preserve every approach and keep `ud_map2` **RETAIN IN PLACE** while map ownership and dependency/tooling closure remain open. Active `drst` remains **DO NOT MOVE**. See the [DRST audit supplement](DRST_AUDIT_REVIEW.md), [`ud_map2` candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114), and [Stoutdorf coverage row](ZONE_COVERAGE_REGISTER.csv#L82).

### Checkpoint 122: Charcoal Palace has no inactive-WLD approach

The all-95 inactive-WLD scan found no source-room overlap or approach into active `firep`; the curated source-only total remains 1,533 and the reviewed Surf subset remains 142. This adds no source-only WLD edge. Separately, unregistered `brass-old-1.zon` line 812 contains a `G` reset of Firep object #88322; it is not active stock and adds typed dependency to the retained Brass candidate. The 2025 Sword of Fire and Flame #88327 move is item-level history, not whole-zone replacement. Checkpoint 138 adds source-only brass-old-1.zon line 924 G reset of active Church object #87859; it is not a live reset. Preserve this dependency with the retained Brass source; see CHURCH_AUDIT_REVIEW.md. Preserve both records and keep `brass-old-1` **RETAIN** pending full closure. Active `firep` remains **DO NOT MOVE**. See the [Firep audit supplement](FIREP_AUDIT_REVIEW.md), [Brass candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17), and [coverage row](ZONE_COVERAGE_REGISTER.csv#L96).

### Checkpoint 123: two inactive Surf approaches into Menden

The all-95 inactive-WLD scan found two non-overlapping `surf.wld` exits into active Menden pier #88846: room #232071 east and room #232372 north. They are source-only, not active ingress. The reviewed Surf subset rises from 142 to 144 and the curated source-only total rises from 1,533 to 1,535. Preserve both edges with the Surf map; keep `surf` and `surface2011` **RETAIN** pending map ownership and remaining dependency/tool closure. Active `menden` remains **DO NOT MOVE**. See the [Menden audit supplement](MENDEN_AUDIT_REVIEW.md), [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [Menden coverage row](ZONE_COVERAGE_REGISTER.csv#L174).

### Checkpoint 126: two inactive Surf approaches into Troll Hills

The all-95 inactive-WLD sweep found two non-overlapping, source-only `surf.wld` exits: #278929 south to active Troll Hills #1852 and #279529 north to #1871. Neither is active ingress. At Checkpoint 126, the reviewed Surf subset rose from 144 to 146 and the curated source-only total from 1,997 to 1,999; those counts are historical, and both approaches are already included in the current 11,993 overall / 415 Surf total. Preserve both map records and keep `surf` / `surface2011` **RETAIN** pending map ownership and remaining dependency/tool closure. Active `troll_hills` remains **DO NOT MOVE**. See the [Troll Hills audit supplement](TROLL_HILLS_AUDIT_REVIEW.md), [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [coverage row](ZONE_COVERAGE_REGISTER.csv#L315).

### Checkpoint 127: five inactive Surf approaches into Twisted Wood

The all-95 inactive-WLD sweep found five non-overlapping source-only `surf.wld` exits: #226853 east to #16303, #227153 east to #16302, #227453 east to #16301, #227753 east to #16300, and #228054 north to #16300. None is active ingress. The reviewed Surf subset rises from 146 to 151 and the curated source-only total from 1,999 to 2,004. Preserve these map records and keep `surf` / `surface2011` **RETAIN** pending map ownership and remaining dependency/tool closure. Active `twstwd` remains **DO NOT MOVE**. See the [Twisted Wood audit supplement](DARK_AND_TWISTED_WOOD_AUDIT_REVIEW.md), [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [coverage row](ZONE_COVERAGE_REGISTER.csv#L321).

### Checkpoint 128: Underworld approaches and mobile reset references

The all-95 inactive-WLD sweep adds three non-overlapping source-only approaches into Underworld: `ud_map0.wld` #1105558 north to #4469 and `ud_map2.wld` #1120813 north to #4469 and #1127503 up to #4608. They are not active ingress. The curated source-only total rises from 2,004 to 2,007; the reviewed Surf subset stays at 151. The Underworld typed scan also finds 244 active `.zon` M commands across six files and 289 inactive-candidate M commands across `ud_map0` (13), `ud_map1` (4), `ud_map2` (267), and `surf` (5), all referencing Underworld mobile prototypes. These source reset references do not establish current placement. Retain `ud_map0`, `ud_map1`, `ud_map2`, `surf`, and `surface2011`; active `underworld` remains **DO NOT MOVE**. Preserve the WLD approaches and reset dependencies pending map ownership, generator-role, and remaining tool/dependency closure. See the [Underworld audit supplement](UNDERWORLD_AUDIT_REVIEW.md), candidate rows [Surf](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97) and [Underworld maps](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L112), and [coverage row](ZONE_COVERAGE_REGISTER.csv#L326).

### Checkpoint 129: one inactive Surf approach into Zalkapfaan

The all-95 inactive-WLD sweep found one non-overlapping source-only `surf.wld` exit: #221513 west to active Zalkapfaan entrance #2730. It is not active ingress. The reviewed Surf subset rises from 151 to 152 and the curated source-only total from 2,007 to 2,008. Preserve the edge with its source map and keep `surf` / `surface2011` **RETAIN** pending map ownership and remaining dependency/tool closure. Active `headless` remains **DO NOT MOVE**. See the [Zalkapfaan audit supplement](ZALKAPFAAN_AUDIT_REVIEW.md), [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [coverage row](ZONE_COVERAGE_REGISTER.csv#L120).

### Checkpoint 130: Duris3 approaches and overlapping source rooms at the Spires

The inactive `Duris3.wld` has 1,332 exits targeting active Eternal rooms #135400–#135732. Of those, 602 source-only edges start outside the active room VNUM range: 300 south edges from source rooms #135100–#135399, 300 north edges from #135733–#136032, and two endpoints. Another 730 target records start in the 333 source-room VNUMs overlapping active Eternal rooms. Only the 602 non-overlapping exits raise the curated source-only total, from 2,008 to 2,610; the reviewed Surf subset remains 152. None is active ingress. Preserve both the source-only bands and overlapping map-identity evidence, and keep `Duris3` **RETAIN IN PLACE** while owners resolve map identity. Active `eternal` remains **DO NOT MOVE**. See the [Spires audit supplement](SPIRES_OF_ELDER_EVIL_AUDIT_REVIEW.md), [Duris3 candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), and [coverage row](ZONE_COVERAGE_REGISTER.csv#L90).

### Checkpoint 131: no inactive-WLD approach into Shaughin

The all-95 inactive-WLD scan found no source-room overlap or exit target into active `shaughin` rooms #6500–#6548. No source-only approach is added; curated totals remain 2,610 overall and 152 for the reviewed Surf subset. Preserve the active reciprocal Underdark/Shaughin and Shaughin/Shaughin2 pairs separately from the lower area's type-25 portal and trophy-source dependencies. This scan does not add migration clearance. See the [Shaughin audit supplement](SHAUGHIN_AUDIT_REVIEW.md), [coverage row](ZONE_COVERAGE_REGISTER.csv#L270), and [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 132: one inactive Surf approach into Trakkia

The all-95 inactive-WLD sweep found one non-overlapping source-only approach: `surf.wld` #246406 west to active Trakkia #57000. It is not active ingress. At Checkpoint 132, this raised the reviewed Surf subset from 152 to 153 and curated source-only total from 2,610 to 2,611; those are historical counts, and this approach is already included in the current 11,993 overall / 415 Surf total. Preserve it with the source map; keep `surf` / `surface2011` **RETAIN** pending map ownership and remaining dependency/tool closure. Active `trakkia` remains **DO NOT MOVE**. See the [Trakkia audit supplement](TRAKKIA_AUDIT_REVIEW.md), [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [coverage row](ZONE_COVERAGE_REGISTER.csv#L310).

### Checkpoint 133: no inactive-WLD approach into Kelek

The all-95 inactive-WLD scan found no source-room overlap or exit target into active Kelek rooms #87950–#88165. No new approach is added; curated source-only totals remain 2,611 overall and 153 for the reviewed Surf subset. Preserve the reciprocal Underdark and Krethik links and the existing imported epic-node/Deliverer special dependencies independently of the corrected Church story ownership. No migration clearance follows from the scan. See the [Kelek audit supplement](KELEK_AUDIT_REVIEW.md), [coverage row](ZONE_COVERAGE_REGISTER.csv#L143), and [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 134: four inactive approaches into Krethik

The all-95 inactive-WLD scan found two non-overlapping `oldmap8.wld` approaches (#182553 east→#20000, #182654 north→#20085) and two non-overlapping `surf.wld` approaches (#274522 east→#20000, #274526 west→#20112). None is active ingress. The curated source-only total rises from 2,611 to 2,615, and the reviewed Surf subset from 153 to 155. Preserve all four records and keep `oldmap8`, `surf`, and `surface2011` **RETAIN** pending map ownership/dependency review. Active `krethik` remains **DO NOT MOVE**. See the [Krethik audit supplement](KRETHIK_AUDIT_REVIEW.md), candidate rows [oldmap8](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L81) and [Surf](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [coverage row](ZONE_COVERAGE_REGISTER.csv#L149).

### Checkpoint 135 — Thetis map-source evidence

The all-95 inactive-WLD sweep found Surf #251543 north → Thetis #38000. It is source-only, non-overlapping, and not active ingress. Retain surf/surface2011 and preserve #38095 north → #104 as unresolved; see the [Thetis audit supplement](THETIS_AUDIT_REVIEW.md).

### Checkpoint 136 — Azhural and the former tiamats_lair source

Duris3 contributes 180 non-overlapping source-only exits into Azhural; another 89 Duris3 room VNUMs overlap the active map. The old tiamats_lair WLD has 83 overlapping room VNUMs and 200 exits into current Azhural room IDs. A 2012 commit explicitly replaces the older Lair version with Azhural, but qst/AREA still selects tiamats_lair.qst. The two QST files are byte-identical today, so preserve the legacy QST path and all source identities while ownership/tooling closure remains open. See the [Azhural audit supplement](AZHURAL_AUDIT_REVIEW.md); the curated total rises to 2,796 and Surf remains 156.

### Checkpoint 137: no inactive-WLD approach into Quintaragon Castle

The all-95 inactive-WLD scan found no room-VNUM overlap or exit target into active Castle rooms #2401–#2532. No source-only approach is added; curated totals remain 2,796 overall and 156 for the reviewed Surf subset. Preserve the reciprocal Minizones boundary and the two typed imported reset objects with their owning zones. The scan provides no migration clearance. See the [Castle audit supplement](CASTLE_AUDIT_REVIEW.md), [coverage row 46](ZONE_COVERAGE_REGISTER.csv#L46), and existing [boundary pair rows](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L68).


### Checkpoint 138: no inactive-WLD approach into the Church

The all-95 inactive-WLD sweep found no room overlap or exit target into active Church rooms #87820–#87852. No source-only WLD approach is added; curated totals remain 2,796 overall and 156 for the reviewed Surf subset. Separately, unregistered brass-old-1.zon line 924 G-resets Church object #87859. It is a typed source-only dependency, not active stock. Keep brass-old-1 RETAIN and Church DO NOT MOVE; no migration clearance follows. See the [Church audit supplement](CHURCH_AUDIT_REVIEW.md), [Brass candidate row 17](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17), and [coverage row 56](ZONE_COVERAGE_REGISTER.csv#L56).

### Checkpoint 139: two source-only map approaches into Domain and Nexus56 book references

The all-95 inactive-WLD scan finds no Domain room overlap and adds `ud_map1.wld` #1117360 east and `ud_map2.wld` #1127382 down to entrance #36800. Both are source-only, not active ingress; curated totals are 2,798 overall and 156 for the reviewed Surf subset. The typed scan also finds unregistered `nexus56.zon` G reset and `nexus56.shp` shop #83038 product references to active Domain book #36822. With `areas/world.shp` absent, shop fallback parsing remains conditional and does not establish operation. Preserve both map approaches and keep `nexus56` RETAIN. See the [Domain audit supplement](DLSC_AUDIT_REVIEW.md), [candidate rows](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79), [coverage rows 430, 464, and 465](ZONE_COVERAGE_REGISTER.csv#L430), and [connectivity checkpoint](WORLD_CONNECTIVITY.md).

### Checkpoint 140: no inactive-WLD approach into Drifting Realm

The all-95 inactive-WLD scan found no source-room overlap or exit target into Dream rooms #31300–#31326. No source-only approach is added; curated totals remain 2,798 overall and 156 for the reviewed Surf subset. Dream stays **DO NOT MOVE**. Preserve its reciprocal Ethereal boundary, active Arcium bed #85725 entry, four outside reset sources for local skulls #31317–#31320, and Alatorin dreamfoil import as separate live dependencies. The inactive-map scan provides no migration clearance. See the [Drifting Realm audit supplement](DRIFTING_REALM_AUDIT_REVIEW.md), [coverage row 80](ZONE_COVERAGE_REGISTER.csv#L80), and [connectivity checkpoint](WORLD_CONNECTIVITY.md).

### Checkpoint 141 — no inactive-WLD approach into Lizardman Swamps; typed legacy references

The all-95 inactive-WLD sweep found no room overlap or exit target into Lizard rooms #6000–#6121. No source-only approach is added; curated totals remain 2,798 overall and 156 for the reviewed Surf subset. Separately, active `random.zon` and `random.shp` reference local head #6008; unregistered `sylvandawn.zon` resets local sign #6002, and unregistered `tharnadia1.shp`/`tharold.shp` list local objects #6007–#6009. Preserve all three source files as **RETAIN** candidates, and keep active `lizard` **DO NOT MOVE**. The WLD scan provides no migration clearance. See the [Lizardman Swamps audit supplement](LIZARDMAN_SWAMPS_AUDIT_REVIEW.md), [sylvandawn](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L101), [tharnadia1](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L103), and [tharold](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L104) candidate rows, [coverage row 160](ZONE_COVERAGE_REGISTER.csv#L160), and [connectivity checkpoint](WORLD_CONNECTIVITY.md).

### Checkpoint 142 — two inactive Surf approaches into Tower and cross-zone dependencies

The all-95 inactive-WLD sweep found two non-overlapping `surf.wld` exits: #227640 north → active Tower #9334 and #227641 north → #9379. Neither is active ingress. The reviewed Surf subset rises from 156 to 158 and the curated source-only total from 2,798 to 2,800. Preserve both records and keep `surf`/`surface2011` **RETAIN**; active `tower` remains **DO NOT MOVE**. Separately, Tower resets Heavens #359 and WH #55441, depends on the Labyrinth material prototypes, provides ear stud #9375 to an active WH QST, and has an active Raxthan reset/shop reference to #9332. No migration clearance follows. See the [Tower audit supplement](TOWER_OF_HIGH_SORCERY_AUDIT_REVIEW.md), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), [coverage row 308](ZONE_COVERAGE_REGISTER.csv#L308), and [connectivity checkpoint](WORLD_CONNECTIVITY.md).

### Checkpoint 143: Duris3 records into Vecna

The all-95 inactive-WLD sweep found 320 Duris3.wld records targeting Vecna rooms #130000–#130079. Of these, 162 start in non-overlapping source-room VNUMs: 80 south records from #129700–#129779, one west record from #130080, one east record from #130299, and 80 north records from #130300–#130379. Another 158 matches begin in 80 Duris3 rooms with the same VNUMs as Vecna but different titles. All are source-only, not active ingress. Preserve both the non-overlapping approaches and the distinct overlapping map identities; keep Duris3 **RETAIN IN PLACE** and active vecna **DO NOT MOVE**. Curated source-only totals rise from 2,800 to 2,962 overall; Surf remains 158. See the [Vecna audit supplement](VECNAS_TOMB_AUDIT_REVIEW.md), [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), and [coverage rows 336 and 387](ZONE_COVERAGE_REGISTER.csv#L336).

### Checkpoint 144: Surf records into Killing Fields

The all-95 inactive-WLD sweep found one non-overlapping source-only exit in surf.wld: room #279135 south to Killing Fields #33302. It is a source record, not active ingress. In unregistered surf.zon, an M reset places Killing Fields mobile #33308 at source room #221734, followed by six G records for objects #33309–#33312, #33314, and #33315. These records do not establish active merchant or item stock. Preserve both the approach and reset references while map ownership and dependency/tool closure remain open; keep surf and surface2011 RETAIN. The reviewed Surf subset rises to 159 and the curated source-only total to 2,963. Active killing_fields remains DO NOT MOVE. See the [Killing Fields audit supplement](KILLING_FIELDS_AUDIT_REVIEW.md), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [coverage rows 145 and 448](ZONE_COVERAGE_REGISTER.csv#L145).

### Checkpoint 145: Duris3 and Vodgh records into Magma

The all-95 inactive-WLD sweep found 517 source records targeting Magma: 516 in Duris3.wld and one in vodgh.wld. Duris3 contributes 260 exits from source-room VNUMs outside Magma's range plus 256 east/west records from 129 same-numbered rooms titled The Ocean. Vodgh #30310 has one up exit to #142034. These are unregistered source records, not active ingress. Add only the 261 non-overlapping-source exits to curated totals, which now stand at 3,224 overall; Surf remains 159. Preserve the records while ownership and dependency/tool closure remain open; keep Duris3 and vodgh RETAIN IN PLACE and active magma DO NOT MOVE. See the [Magma audit supplement](MAGMA_AUDIT_REVIEW.md), [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), [Vodgh candidate row 118](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L118), and [coverage row 168](ZONE_COVERAGE_REGISTER.csv#L168).

### Checkpoint 146: ud_map2 record into Mazzolin

The all-95 inactive-WLD sweep found one non-overlapping source-only ud_map2.wld exit: room #1125718 south to Mazzolin #21300. It is not active ingress. Preserve it with ud_map2 and keep that candidate RETAIN IN PLACE; active mazzolin remains DO NOT MOVE. The ud_map2 reviewed subset is now 13, curated source-only totals are 3,225 overall, and Surf remains 159. See the [Mazzolin audit supplement](MAZZOLIN_AUDIT_REVIEW.md), [ud_map2 candidate row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114), and [coverage row 173](ZONE_COVERAGE_REGISTER.csv#L173).

### Checkpoint 147: two UnderDark map records into Myconid

The all-95 inactive-WLD sweep found non-overlapping source-only approaches to Myconid #2300: `ud_map0.wld` #1102287 down and `ud_map2.wld` #1124242 down. Neither is active ingress. Preserve both with their source maps; keep `ud_map0` and `ud_map2` RETAIN IN PLACE and active `myconid` DO NOT MOVE. The reviewed subsets are now two and 14, the curated total is 3,227 overall, and Surf remains 159. See the [Myconid audit supplement](MYCONID_AUDIT_REVIEW.md), [candidate rows 112 and 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L112), and [coverage row 197](ZONE_COVERAGE_REGISTER.csv#L197).

### Checkpoint 148: no inactive-WLD approach into Vargan II

The all-95 inactive-WLD sweep found no source-room overlap or exit target into active Vargan II rooms #7800–#7918. No source-only approach is added; curated totals remain 3,227 overall and 159 for the reviewed Surf subset. Active `v2` stays DO NOT MOVE. The absence of an inactive map edge does not clear the active map or its broader dependencies for migration. See the [Vargan II audit supplement](V2_AUDIT_REVIEW.md), [coverage row 330](ZONE_COVERAGE_REGISTER.csv#L330), and reciprocal [boundary pair rows](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L762).

### Checkpoint 149: ud_map2 approach into Phantasmagoric Caverns

The all-95 inactive-WLD sweep found one non-overlapping, source-only `ud_map2.wld` exit: #1125389 down to active Valdrak #53100. It is not active ingress. Preserve it with `ud_map2` and keep that candidate RETAIN IN PLACE; active `valdrak` remains DO NOT MOVE. The reviewed `ud_map2` subset is now 15, curated source-only totals are 3,228 overall, and Surf remains 159. See the [Valdrak audit supplement](VALDRAK_AUDIT_REVIEW.md), [ud_map2 candidate row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114), and [coverage row 332](ZONE_COVERAGE_REGISTER.csv#L332).

### Checkpoint 150: no inactive-WLD approach into The Bronze Citadel

The all-95 inactive-WLD sweep found no source-room overlap or exit target into active Bctdl rooms #32420–#32501. No source-only approach is added; curated totals remain 3,228 overall, 15 for the reviewed `ud_map2` subset, and 159 for Surf. Active `bctdl` stays DO NOT MOVE. The absence of an inactive map edge does not clear active WLD inbounds, the Avernus type-25 gate target, shared quest items, reset imports, or compiled prototype bindings for migration. See the [Bctdl audit supplement](BCTDL_AUDIT_REVIEW.md), [coverage row 30](ZONE_COVERAGE_REGISTER.csv#L30), and inbound [pair rows 385 and 782](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L385).

### Checkpoint 151: Duris3 approaches into Brimeforge

The all-95 inactive-WLD sweep found 100 non-overlapping source-only Duris3.wld approaches into active Brimeforge #131001–#131049: 49 south, one east endpoint, one west endpoint, and 49 north. Another 96 east/west matches begin in 49 same-numbered Duris3 source rooms (#131001–#131049); keep those map identities separate and exclude them from the approach total. None of these records is active ingress or replacement evidence. Curated totals rise to 3,328 overall; Surf remains 159 and the reviewed `ud_map2` subset remains 15. Preserve the source records and keep Duris3 RETAIN IN PLACE; active `brimeforge` remains DO NOT MOVE. See the [Brimeforge audit supplement](BRIMEFORGE_AUDIT_REVIEW.md), [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), and [coverage row 40](ZONE_COVERAGE_REGISTER.csv#L40).

### Checkpoint 152: no inactive-WLD approach into The Bugger Caves

The all-95 inactive-WLD sweep found no source-room VNUM overlap or exit into active Bugger rooms #6400–#6427. No source-only approach is added; curated totals remain 3,328 overall, 15 for the reviewed ud_map2 subset, and 159 for Surf. Active bugger stays DO NOT MOVE. The absence of an inactive-map edge does not clear its two active reciprocal WLD boundaries, compiled room #6405 reference, or broader dependencies for migration. See the [Bugger audit supplement](BUGGER_AUDIT_REVIEW.md), [coverage row 43](ZONE_COVERAGE_REGISTER.csv#L43), and Surf [candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 153: no inactive-WLD approach into Dirk’nspire Stronghold

The all-95 inactive-WLD sweep found no source-room VNUM overlap or exit into active Dirk’nspire rooms #96800–#96885. No source-only approach is added; curated totals remain 3,328 overall, 15 for reviewed ud_map2, and 159 for Surf. Active dirkn stays DO NOT MOVE. The absence of an inactive-map edge does not clear the reciprocal Valinhav boundary, Alatorin’s external contracts/reset, the Alatorin sidecar guidance, compiled room #96803 references, or broader dependencies for migration. See the [Dirk’nspire supplement](DIRKN_AUDIT_REVIEW.md), [coverage row 75](ZONE_COVERAGE_REGISTER.csv#L75), and Surf [candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 154: source-only Surf approaches into Storm Port

The all-95 inactive-WLD sweep found no source-room overlap and four non-overlapping surf.wld exits into active Storm Port: #274843 south to #22479, #275142 east to #22401, #275144 west to #22432, and #275443 north to #22401. These entries are source-only, not active ingress or replacement evidence. Curated totals rise to 3,332 overall and Surf rises to 163 reviewed approaches; ud_map2 stays at 15. Keep surf and surface2011 RETAIN IN PLACE, and keep active stormport DO NOT MOVE. See the [Storm Port audit supplement](STORMPORT_AUDIT_REVIEW.md), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [coverage rows 284 and 286](ZONE_COVERAGE_REGISTER.csv#L284).

### Checkpoint 155: Arachdrathos typed candidate dependencies

The all-95 inactive-WLD sweep adds no source-only approach or room overlap for active arac-web. Typed scans find seven active Alatorin P resets to Arachdrathos objects #36415, #36416, #36418, and #36419, four unregistered nexus56 G resets to Arachdrathos objects #36427–#36430, and one unregistered ud_map0 M reset to Arachdrathos mobile #36431. The unregistered resets do not establish active stock, but they are exact candidate dependencies. Preserve those references and keep nexus56 and ud_map0 RETAIN IN PLACE. Active arac-web remains DO NOT MOVE. Existing curated source-only totals stay 3,332 overall, Surf 163, and ud_map2 15. See the [Arachdrathos audit supplement](ARAC_WEB_AUDIT_REVIEW.md), [nexus56 candidate row 79](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79), [ud_map0 candidate row 112](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L112), and [Arachdrathos coverage row 9](ZONE_COVERAGE_REGISTER.csv#L9).

### Checkpoint 156: Arcium source-only approach and reset dependencies

The all-95 inactive-WLD sweep adds one non-overlapping source-only Surf exit, room #283370 D0 to Arcium #85700. It does not establish active ingress. Curated source-only totals rise to 3,333 overall and the reviewed Surf subset to 164; ud_map2 remains 15. Active Alatorin resets Arcium mobile #85708, while unregistered brass-old-1.zon G-resets local Arcium heart #85716 under Hutzparoon; that reset is source-only. Preserve the route and typed references, keep surf and brass-old-1 RETAIN IN PLACE, and keep active arcium DO NOT MOVE. See the [Arcium audit supplement](ARCIUM_AUDIT_REVIEW.md), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), [brass-old-1 candidate row 17](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17), and [Arcium coverage row 12](ZONE_COVERAGE_REGISTER.csv#L12).

### Checkpoint 157: Future Ceothia dependencies and no inactive map approach

The all-95 inactive-WLD sweep finds no source-room overlap or source-only exit into active ceofutur; source-only totals remain 3,333 overall, 164 Surf, and 15 ud_map2. Preserve the active Realm inbound, Past/Future type-25 portal targets, active Alatorin and Present Ceothia QST consumers, structured-story contacts and item references, local imports from Heavens/Winterhaven/TransTower, and compiled CHAOS/random-equipment references. Keep ceofutur DO NOT MOVE. No candidate WLD row is created because no inactive map approach or source-only typed dependency was found. See the [Future Ceothia audit supplement](FUTURE_CEOTHIA_AUDIT_REVIEW.md), [coverage row 51](ZONE_COVERAGE_REGISTER.csv#L51), and source areas [Alatorin](ZONE_COVERAGE_REGISTER.csv#L6), [Present Ceothia](ZONE_COVERAGE_REGISTER.csv#L53), and [Surface Mini](ZONE_COVERAGE_REGISTER.csv#L448).

### Checkpoint 158: Treasure Caves links and Mountain candidate dependency

The all-95 inactive-WLD sweep finds no room overlap or source-only WLD approach into active dungeon; source-only totals remain 3,333 overall, 164 Surf, and 15 ud_map2. Two active reciprocal boundaries join Dungeon to Arachdrathos Wilderness and Underdark. Typed closure finds active White Harbor and Surface Mini references to Dungeon hide #93011, four active Tharnadian Ruin resets to mobile #93015, and compiled room/object references. It also finds unregistered Mountain.qst line 56 G I #93001, an exact source-only dependency on Dungeon object #93001. Keep Mountain RETAIN IN PLACE and preserve that reference; the file is not in areas/AREA or qst/AREA and is not active QST input. Keep active dungeon DO NOT MOVE. See the [Treasure Caves audit supplement](TREASURE_CAVES_AUDIT_REVIEW.md), [coverage row 83](ZONE_COVERAGE_REGISTER.csv#L83), and [Mountain candidate row 74](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L74).

### Checkpoint 159: Jindon source-only approach and Nexus56 dependency

The all-95 inactive-WLD sweep adds one non-overlapping source-only `surf.wld` exit, #244020 north to active Jindon #82063. It is not active ingress. Curated source-only totals rise to 3,334 overall and 165 for the reviewed Surf subset; ud_map2 remains 15. Active Otower has a G reset for Jindon object #82018. Unregistered Nexus56 has both a G reset and SHP #83032 product for object #82011; these remain source-only references with conditional shop-reader fallback, not evidence of operation. Keep `surf` and `nexus56` RETAIN IN PLACE and preserve their existing candidate rows and dependencies. Keep active `jin` DO NOT MOVE. See the [Jindon audit supplement](JINDON_DEATHWOOD_AUDIT_REVIEW.md), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), [Nexus56 candidate row 79](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79), and [Jindon coverage row 139](ZONE_COVERAGE_REGISTER.csv#L139).

### Checkpoint 160: Lava Springs source-only approach and active reset dependencies

The all-95 inactive-WLD sweep adds one non-overlapping source-only `surf.wld` exit, #259346 north to active Lava #99700. It is not active ingress. Curated source-only totals rise to 3,335 overall and 166 for the reviewed Surf subset; ud_map2 remains 15. Active Lava resets Dream object #31318 to Voinak and Verz1 counter object #3097 to room #99725. Preserve these typed cross-area dependencies and the source-only Surf approach. Keep `surf` RETAIN IN PLACE and preserve its existing candidate row; keep active `lava` DO NOT MOVE. See the [Lava Springs audit supplement](LAVA_SPRINGS_AUDIT_REVIEW.md), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [Lava coverage row 155](ZONE_COVERAGE_REGISTER.csv#L155).

### Checkpoint 161: Lylr-Meop source-only approaches and cross-area item dependencies

The all-95 inactive-WLD sweep adds two non-overlapping source-only `surf.wld` exits into Lylr: #222832 north to #82240 and #224629 south to #82200. They are not active ingress. Curated source-only totals rise to 3,337 overall and 168 for the reviewed Surf subset; ud_map2 remains 15. Active Lylr resets a Tharnadia counter and Heavens bandages, and its active SHP sells the three Heavens items. Preserve these typed cross-area references and both Surf approaches. Keep `surf` RETAIN IN PLACE and preserve its existing candidate row; keep active `lylr` DO NOT MOVE. See the [Lylr-Meop audit supplement](LYLR_MEOP_AUDIT_REVIEW.md), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [Lylr coverage row 167](ZONE_COVERAGE_REGISTER.csv#L167).

### Checkpoint 162: Mining approaches, source overlaps, and Alterian references

The all-95 inactive-WLD sweep finds 160 target matches into active Mining and 69 room-VNUM overlaps in unregistered Alterian sources. Preserve nine non-overlapping source-only exits: three from `alterian_mountains.wld`, five from `alterian_wilderness.wld`, and one from `surf.wld`; the other 151 matches come from overlapping source rooms and are excluded from the curated approach count. Preserve the 16 and 17 unregistered Alterian ZON M-reset destination references into Mining room VNUMs and the distinct unregistered `alterian_mountains.obj` prototype #4900. These candidate records do not establish active ingress or active resets. Curated totals are 3,346 overall and 169 for Surf; ud_map2 remains 15. Keep `alterian_mountains`, `alterian_wilderness`, and `surf` RETAIN IN PLACE; keep active `mining` DO NOT MOVE. See the [Mining audit supplement](MINING_AUDIT_REVIEW.md), candidate rows [7](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L7), [8](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L8), and [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [Mining coverage row 175](ZONE_COVERAGE_REGISTER.csv#L175).

### Checkpoint 163: Mir dependencies and no inactive-map approach

The all-95 inactive-WLD sweep finds no source-room overlap or exit into active Mir; source-only totals remain 3,346 overall, 169 for Surf, and 15 for ud_map2. Preserve the two reciprocal active Highway boundaries and the active Bastine and Surface Mini QST/story references to Mir objects, Highway resets to Mir objects #41915 and #41917, Mir's reset to Heavens object #359, and compiled procedure/equipment references. These are active-area dependencies, not attic candidate rows. Keep active mir DO NOT MOVE; no candidate row is added. See the [Mir audit supplement](MIR_AUDIT_REVIEW.md), [Mir coverage row 178](ZONE_COVERAGE_REGISTER.csv#L178), and [Highway pair rows 194 and 276](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L194).

### Checkpoint 164: Shadow Forest source-only approaches and namespace collision

The all-95 inactive-WLD sweep adds two non-overlapping alterian_wilderness.wld exits into active Mist #6368 (#4801 south and #4918 south) and one aopal.wld exit (#43301 west). They are source-only, not active ingress, and raise curated totals to 3,349 overall; Surf remains 169 and ud_map2 remains 15. Preserve the exits with existing candidate rows [8](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L8) and [10](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L10). A raw numeric scan hit at tharnadia1.shp line 561 is shop #6113 in_room #6358, which matches Mist room #6358; it is not a product VNUM, and no Mist object #6358 exists. Keep the existing tharnadia1 candidate row 103 for its other dependencies and preserve this false-positive classification. Keep active mist DO NOT MOVE; no candidate row is added. See the [Shadow Forest audit supplement](SHADOW_FOREST_AUDIT_REVIEW.md), [coverage row 180](ZONE_COVERAGE_REGISTER.csv#L180), and [tharnadia1 coverage row 454](ZONE_COVERAGE_REGISTER.csv#L454).

### Checkpoint 165: Mosswood source-only Surf approaches

The all-95 inactive-WLD sweep finds no room overlap and adds three non-overlapping source-only surf.wld exits into active Mosswood #23400: #229223 south, #229522 east, and #229524 west. They are not active ingress. Curated totals rise to 3,352 overall and 172 for Surf; ud_map2 remains 15. Preserve them with existing Surf candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). Keep surf RETAIN IN PLACE and active moss DO NOT MOVE; no candidate row is added. See the [Mosswood audit supplement](MOSSWOOD_AUDIT_REVIEW.md), [Mosswood coverage row 190](ZONE_COVERAGE_REGISTER.csv#L190), and [Surf coverage row 448](ZONE_COVERAGE_REGISTER.csv#L448).

### Checkpoint 166: Roper Den approaches and Duris3 source overlaps

The all-95 inactive-WLD sweep finds 100 Duris3.wld exits targeting Roper Den room VNUMs #130400–#130424. Fifty-two start from non-overlapping source rooms and are curated: 25 south exits from #130100–#130124, #130399 east to #130400, #130425 west to #130424, and 25 north exits from #130700–#130724. The other 48 matches start in 25 same-numbered Duris3 source rooms and are excluded from the approach count. These records are separate map identities, not active ingress or replacement evidence. Curated totals rise to 3,404 overall, Surf remains 172, and ud_map2 remains 15. Keep Duris3 RETAIN IN PLACE with candidate row [36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36); keep active nexus_roper DO NOT MOVE. See the [Roper Den audit supplement](NEXUS_ROPER_AUDIT_REVIEW.md), [coverage row 210](ZONE_COVERAGE_REGISTER.csv#L210), and [Duris3 coverage row 387](ZONE_COVERAGE_REGISTER.csv#L387).

### Checkpoint 167: Nyneth2 boundary, dependencies, and no source-only approaches

The all-95 inactive-WLD sweep finds no room overlap or exit into active Nyneth2, so curated totals remain 3,404 overall, 172 for Surf, and 15 for ud_map2. Preserve the active one-way White Harbor #55632 north edge to Nyneth2 #23098 and the active prior-stage portal #22903 target to #23000. Typed reset dependencies connect Nyneth2 to Nyneth mobile #22904/object #22966 and Heavens object #360; two Alatorin reset rows reference Nyneth2 object #23045. Compiled source references Nyneth2 objects #23056 and #23064. Keep active `nyneth2` DO NOT MOVE; no candidate row is added. See the [Nyneth2 audit supplement](NYNETH2_AUDIT_REVIEW.md), [coverage row 218](ZONE_COVERAGE_REGISTER.csv#L218), and active boundary [exit row 780](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L780).

### Checkpoint 168: Nyneth3 technical inbounds and typed dependencies

The all-95 inactive-WLD sweep finds no room overlap or exit into active Nyneth3, so curated totals remain 3,404 overall, 172 for Surf, and 15 for ud_map2. Preserve the two one-way Realm and White Harbor exits to Treasure Room #38811 and the active Nyneth portal #22904 target to #38700. Typed references include Alatorin QST/story/ZON use of Nyneth3 object #38779, two Barovia G reset rows for object #38715, and Nyneth3 stock resets from Heavens, White Harbor, Negative Plane, and prior-stage Nyneth. Compiled bindings reference the Ny’Neth mobile, local Fury, Chaos-kit items, and four item procedures. Keep active `nyneth3` DO NOT MOVE; no candidate row is added. See the [Nyneth3 audit supplement](NYNETH3_AUDIT_REVIEW.md), [coverage row 219](ZONE_COVERAGE_REGISTER.csv#L219), active WLD exit-register [row 249](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L249) and [row 792](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L792).

### Checkpoint 169: Orrak boundary, source-only approach, and reset dependencies

The all-95 inactive-WLD sweep finds no room overlap and one non-overlapping source-only `surf.wld` exit, #260629 north to active Orrak #9000. Preserve it in existing Surf candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); curated totals rise to 3,405 overall and 173 for Surf, with ud_map2 at 15. The active Orrak #9000 south / Surface #610629 north boundary reciprocates. Preserve Orrak's active resets of Heavens object #358 and White Harbor object #55446, plus active Charing and source-only Sylvandawn G resets to Orrak object #9017. Keep active `orrak` DO NOT MOVE, and keep `surf` and `sylvandawn` RETAIN IN PLACE with rows 97 and 101. No candidate row is added. See the [Orrak audit supplement](ORRAK_AUDIT_REVIEW.md), [Orrak coverage row 225](ZONE_COVERAGE_REGISTER.csv#L225), and [Sylvandawn candidate row 101](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L101).

### Checkpoint 170: Sea Kingdom boundaries, dependencies, and no inactive approach

The all-95 inactive-WLD sweep finds no room overlap or exit into active Sea Kingdom; curated source-only totals remain 3,405 overall, 173 for Surf, and 15 for ud_map2. Preserve the reciprocal active Sea Kingdom #31500 up / Plane Water #23281 down boundary and the one-way White Harbor #55634 southeast edge to Sea Kingdom #31668. Preserve Alatorin's item references to Sea Kingdom objects #31538/#31544, Heavens' QST reference to #31549, and Sea Kingdom's reset of Heavens object #359. Keep active `seakngdm` DO NOT MOVE; no candidate row is added. See the [Sea Kingdom audit supplement](SEAKNGDM_AUDIT_REVIEW.md), [Sea Kingdom coverage row 262](ZONE_COVERAGE_REGISTER.csv#L262), [Plane Water row 234](ZONE_COVERAGE_REGISTER.csv#L234), and [White Harbor row 345](ZONE_COVERAGE_REGISTER.csv#L345).

### Checkpoint 171: Turolopolis Zoo boundary and no inactive approach

The all-95 inactive-WLD sweep finds no room overlap or exit into active Turolopolis Zoo; curated source-only totals remain 3,405 overall, 173 for Surf, and 15 for ud_map2. Preserve the sole active reciprocal boundary with the Ruins of Turolopolis: Willem #7123 west to Zoo #53000 and Zoo #53000 east to Willem #7123. No candidate row is added. Keep `turolzoo` DO NOT MOVE, and retain existing `surf`/`surface2011` candidates for their other mapped dependencies. See the [Turolopolis Zoo audit supplement](TUROLZOO_AUDIT_REVIEW.md), [Zoo coverage row 319](ZONE_COVERAGE_REGISTER.csv#L319), and [Willem row 347](ZONE_COVERAGE_REGISTER.csv#L347).

### Checkpoint 172: Valley of Crushk boundary, typed dependencies, and Surf approach

The all-95 inactive-WLD sweep finds no overlap with active Valley of Crushk and one non-overlapping source-only Surf exit, #232517 north to Valley #2060; curated totals rise to 3,406 overall and 174 for Surf, while ud_map2 remains 15. Preserve the active Mini Zones one-way inbound, reciprocal Pharr Valley boundary, and one-way Valley-to-Surface exit. Preserve the Ship Yards shiv dependency, outside reset/shop references to Valley objects, Heavens imports, and unresolved local mobile #207 reset as separately bounded evidence. Keep `valley_crushk` DO NOT MOVE and `surf` RETAIN IN PLACE with existing candidate row 97; no candidate row is added. See the [Valley audit supplement](VALLEY_CRUSHK_AUDIT_REVIEW.md), [Valley coverage row 334](ZONE_COVERAGE_REGISTER.csv#L334), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 173: Forgotten Forest title, boundaries, dependencies, and Surf approaches

The all-95 inactive-WLD sweep finds no room overlap with active `forgotten_forest` and 15 non-overlapping source-only `surf.wld` exits into Forest rooms. Preserve all 15 with the existing Surf row; curated source-only totals rise to 3,421 overall and 189 for Surf, while ud_map2 remains 15. Preserve the reciprocal active Surface boundary. Keep `forgotten_forest` DO NOT MOVE and `surf` RETAIN IN PLACE with existing candidate row 97; add no candidate row. Keep the compiled Chaos druid equipment references to local object #82707 visible in future dependency review. See the [Forgotten Forest audit supplement](FORGOTTEN_FOREST_AUDIT_REVIEW.md), [Forest coverage row 101](ZONE_COVERAGE_REGISTER.csv#L101), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 174: Rice Fields boundaries, typed dependencies, and inactive approaches

The all-95 inactive-WLD sweep finds no overlap with active `jademini` and four non-overlapping source-only approaches: one `IC3.wld` exit north to #77262 and three `surf.wld` exits to #77258, #77220, and #77218. Preserve them with existing IC3 row 53 and Surf row 97; curated totals rise to 3,425 overall and 192 for Surf, while ud_map2 remains 15. Preserve the three reciprocal active Surface pairs and record the discarded negative #77262 south target separately. Keep `jademini` DO NOT MOVE and `IC3`/`surf` RETAIN IN PLACE. Preserve exact cross-area QST/story items, Thetis mobile reset sources, and compiled archer/map references in any separate proposal. No candidate row is added. See the [Rice Fields audit supplement](RICE_FIELDS_AUDIT_REVIEW.md), [coverage row 138](ZONE_COVERAGE_REGISTER.csv#L138), and existing [IC3](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L53) and [Surf](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97) rows.

### Checkpoint 175: Bandit Canyons boundary, dependencies, and Surf approach

The all-95 inactive-WLD sweep finds no room overlap with active `bandit` and one non-overlapping source-only `surf.wld` exit, #234839 west to #98500. Preserve it with Surf candidate row 97; curated totals rise to 3,426 overall and 193 for Surf, while ud_map2 remains 15. Preserve the reciprocal active Surface pair and exact incoming Alatorin reference to dagger #98505. Keep `bandit` DO NOT MOVE and `surf` RETAIN IN PLACE; no candidate row is added. See the [Bandit audit supplement](BANDIT_CANYONS_AUDIT_REVIEW.md), [coverage row 23](ZONE_COVERAGE_REGISTER.csv#L23), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).
### Checkpoint 181 — Ako Village source-only approach

Unregistered surf.wld room #224164 exits east into active Ako Village #3751. Preserve that declaration as source-only, not active ingress; the curated source-only total is 3,434 overall, with the reviewed Surf subset at 201 and ud_map2 at 15. Keep active ako DO NOT MOVE and preserve its one-way WLD departure to Surface #544136 and three reset imports of Heavens object #283. Keep surf RETAIN IN PLACE in candidate row 97; do not add an Ako candidate row. The source-only approach does not establish current access, return, or migration safety. See the [Ako audit supplement](AKO_AUDIT_REVIEW.md) and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 182 — An Ancient Bridge dependencies and source-only approaches

Keep active `ancientb` DO NOT MOVE. Preserve its two one-way Surface departures, Alatorin's active reset of pony #85617, and its two active reset imports of Heavens patrolman #445. The all-95 inactive-WLD sweep adds three non-overlapping approaches: aopal.wld #43343 north to #85606 and surf.wld #234341 east to #85600 plus #234343 west to #85623. They are source-only, not active ingress. Curated totals rise to 3,437 overall and 203 for Surf; ud_map2 remains 15. Keep `aopal` and `surf` RETAIN IN PLACE with existing candidate rows [10](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L10) and [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); add no candidate row for Ancient Bridge. These static records do not establish live access, safe travel, return, or migration clearance. See the [Ancient Bridge audit supplement](ANCIENTB_AUDIT_REVIEW.md).

### Checkpoint 183 — Ant Forest switch and Surf dependencies

Keep active `ants` DO NOT MOVE. Preserve its reciprocal Surface boundary and two local `ITEM_SWITCH` interactions: corpse #10901 uses `CMD_CRY` to clear the blocked #10904 down exit, and handprint #10902 uses `CMD_TOUCH` to clear the blocked #10915 north exit. Preserve their ZON placements and reset door states; the generic handler leaves secret/access flags separate and no live traversal was qualified. The all-95 inactive-WLD sweep adds one source-only approach, `surf.wld` #211552 east to #10900. It is not active ingress. Curated totals rise to 3,438 overall and 204 for Surf; `ud_map2` remains 15. Keep `surf` RETAIN IN PLACE with existing candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); add no Ant Forest candidate row. See the [Ant Forest audit supplement](ANTS_AUDIT_REVIEW.md).

### Checkpoint 184 — Arachdrathos Wilderness routes and typed dependencies

Keep active `arac_wild` DO NOT MOVE. Preserve its five reciprocal active boundaries, the two `wander.zon` resets that reference chasm-demon mobile #36011, the Drow/Drider basic-kit map object #36016, and the three compiled travel lists that include Crescent Chasm room #36171. The all-95 inactive-WLD sweep adds one source-only approach, `surf.wld` #287030 east to #36000. It is not active ingress. Curated totals rise to 3,439 overall and 205 for Surf; `ud_map2` remains 15. Keep `surf` RETAIN IN PLACE with existing candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); add no candidate row for Arachdrathos Wilderness. Static source findings do not qualify climb/fall behavior or teleport-arrival safety. See the [Arachdrathos Wilderness audit supplement](ARAC_WILD_AUDIT_REVIEW.md).

### Checkpoint 185 — Arachdrathos Guilds routes and dependencies

Keep active `aracguil` DO NOT MOVE. Preserve its seven reciprocal Arac-Web boundaries, ten reset switch/blocked-exit pairings, 44 door resets, Heavens mana stone #359 reset import, and local Jewel #36760 reset on the lost-shrine shade. Preserve the Scribe Mastery teacher #36720 and the compiled callbacks for #36744/#36760; review the missing object prototype #36761 and unplaced lever #36740 before relying on those source paths. The all-95 inactive-WLD sweep finds no overlap or approach into `aracguil`. Curated totals remain 3,439 overall, 205 for Surf, and 15 for `ud_map2`; add no candidate row. Keep `surf` RETAIN IN PLACE with existing row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). See the [Arachdrathos Guilds audit supplement](ARACGUIL_AUDIT_REVIEW.md).

### Checkpoint 186 — Ard'gral and Yggdrasil dependency disposition

Keep active `ardgral` DO NOT MOVE. Preserve its twelve reset fixed rifts and their active destination owners, six gateways using the random surface-map callback, and the compiled illusionist portal range #30401–#30464. Preserve Alatorin's active reset of type-25 rift #83141 to Ard'gral room #30453. The 21 apparent Yggdrasil exits into Ard'gral and its #30400-to-#30401 edge are ambiguous because dormant `ygg.wld` reuses Ard'gral room IDs #30401–#30412; they are not active ingress or resolved migration dependencies. Keep existing Yggdrasil candidate row [122](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L122) RETAIN IN PLACE until its room, mobile, object, reset, portal, and SHP identities are reconciled. Add no Ard'gral candidate row. See the [Ard'gral audit supplement](ARDGRAL_AUDIT_REVIEW.md).

### Checkpoint 187 — Arena disposition

Keep active `arena` DO NOT MOVE. It remains a full-world contributor, not an attic candidate; no candidate row is added. Preserve the active Tharnadia reset of object #67900 in room #132574, the compiled object callback, all three race-war staging pairs, the #67906–#67986 battle-map range, and the current return settings until their owners review them. Verify the Goodie/Evil comment destinations against their room owners and titles, and resolve the Undead -1 hometown, non-Evil portal-exit selection, and absent portal reset in the Undead pair before treating team returns as safe. See the [Arena audit supplement](ARENA_AUDIT_REVIEW.md) and [coverage rows 14 and 297](ZONE_COVERAGE_REGISTER.csv#L14).

### Checkpoint 188 — Astral Plane, Side Areas disposition

Keep active `astral_areas` DO NOT MOVE. It remains a full-world contributor
with four disconnected WLD components and active boundaries to Astral Main;
add no attic candidate row. Preserve its reset-loaded dependencies on
Astral Main, Astral Tiamat, and Heavens, both compiled pool objects, and the
likely but unconfirmed “old gith fort” relationship with the separate active
`githyanki_fortress` map. Keep `Duris3` RETAIN IN PLACE under existing
candidate row [36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36) until active
pool target #163372 is resolved. Its pool callback reference is typed travel
dependency evidence, not active WLD ingress. See the [Astral Side Areas audit
supplement](ASTRAL_AREAS_AUDIT_REVIEW.md), [coverage row
16](ZONE_COVERAGE_REGISTER.csv#L16), and the [connectivity
record](WORLD_CONNECTIVITY.md#checkpoint-188-astral-plane-side-areas).

### Checkpoint 189 — Astral Main disposition

Keep active `astral_main` DO NOT MOVE; it is a full-world contributor, not
an attic candidate. Preserve its five reciprocal WLD boundaries, 21 active
inbound WLD edges, external reset imports and consumers, four active inbound
type-25 portals, Celestia return portals, plane-spell bindings, and pool
objects. Resolve the three invalid mobile #19860 resets, invalid object
#19997 reset, missing room #19890 callback, and Duris3-only pool destination
before relying on those source paths. Preserve the reciprocal pool route
between Astral Tiamat rooms #19945 and #19946. Keep existing `nexus56` candidate row
[79](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79) RETAIN IN PLACE for its
source-only reset and shop references to Astral Main object #19770; keep
`alatorin_objects` row [6](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L6)
RETAIN IN PLACE for the duplicate #83301 type-25 declaration. Neither source
gets migration clearance from this review. See the [Astral Main audit
supplement](ASTRAL_MAIN_AUDIT_REVIEW.md).

### Checkpoint 190 — Astral Plane, Tiamat disposition

Keep active `astral_tiamat` **DO NOT MOVE**. It is a full-world contributor
with a reciprocal boundary to Astral Main, two one-way exits to Main, eight
one-way inbound routes to its Vault of Avernus, imported reset stock, external
quest consumers, and a reciprocal compiled pool pair. Preserve those typed
links while the owner resolves the unbound type-25 #19907 behavior, #19996
quest-item source, missing typed mobile reset, guard binding, and door/key
questions. Add no candidate row for this active zone. The [Astral Tiamat audit
supplement](ASTRAL_TIAMAT_AUDIT_REVIEW.md) records the exact source boundaries
and [coverage row 18](ZONE_COVERAGE_REGISTER.csv#L18).

### Checkpoint 191 — Asylum disposition

Keep active `asylum` **DO NOT MOVE**. Preserve its three active inbound WLD
edges, reciprocal Surface boundary, 56 reset-defined doors, local key/chest
relationship, and the active `random` shop/reset consumers of Asylum object
#9225. Add no attic candidate row for Asylum. Keep `surf` **RETAIN IN PLACE**
under candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); its
inactive WLD room #212125 adds one source-only approach to Asylum #9288 and
does not create active ingress or migration clearance. See the [Asylum audit
supplement](ASYLUM_AUDIT_REVIEW.md), [coverage row
19](ZONE_COVERAGE_REGISTER.csv#L19), and [connectivity
record](WORLD_CONNECTIVITY.md#checkpoint-191-asylum).

### Checkpoint 192 — Avernus disposition

Keep active `avernus` **DO NOT MOVE**. It is a full-world contributor with a
reciprocal Tiamat WLD boundary, Bctdl type-25 return pair, six Ravenloft2
arrivals, a conditional compiled Hell-gate route, external reset imports,
QST/story consumers of local objects, and callback dependencies on active
Undermountain and Heavens prototypes. Its title also appears on a separate
inactive `plane_avernus` component, but the inactive source has only two rooms,
no WLD overlap or approach into active Avernus, and no bounded replacement
history. Do not add an Avernus candidate row.

Keep existing candidate row [83](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L83)
for `plane_avernus` **RETAIN IN PLACE**. The repeated title is not migration
clearance; retain its current typed-source and tool/editor closure requirement.
See the [Avernus audit supplement](AVERNUS_AUDIT_REVIEW.md), [coverage row
20](ZONE_COVERAGE_REGISTER.csv#L20), [connectivity
record](WORLD_CONNECTIVITY.md#checkpoint-192-avernus), and [history
record](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-192-avernus-title-and-path-history).

### Checkpoint 193 — Black Tunnels disposition

Keep active `black_tunnels` **DO NOT MOVE**. Preserve its full-world source
identity, reciprocal Pragtog and Khildarak WLD boundaries, and Khildarak's
reset-loaded type-25 arrival to #10721. Door reset state, key/door prose,
secret-route discoverability, vertical passages, and hazard expectations need
builder resolution before treating those passages as reliable live routes or
making safe-return claims. Add no attic
candidate row. See the [Black Tunnels audit supplement](BLACK_TUNNELS_AUDIT_REVIEW.md),
[coverage row 31](ZONE_COVERAGE_REGISTER.csv#L31), [connectivity
record](WORLD_CONNECTIVITY.md#checkpoint-193-black-tunnels), and [history
record](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-193-black-tunnels-title-and-path-history).

### Checkpoint 194 — Bloody Plains disposition

Keep active `bloody_plains` **DO NOT MOVE**. Preserve its full-world identity,
seven active inbound boundaries, six outbound WLD edges, one-way Killing
Fields arrival, reset-keyed shack, crater fall markers, and active
`mobs_surf.zon` reset imports of local mobile #33500. Add no attic candidate
row for Bloody Plains.

Keep `surf` **RETAIN IN PLACE** under existing candidate row
[97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). Checkpoint 194 adds inactive
`surf.wld` room #280345 north to Bloody Plains #33681 as one source-only
approach, plus fourteen reset imports in unregistered `surf.zon`. The approach
is not active ingress and does not clear a move. Active `mobs_surf.zon` also
imports #33500 into room #31412, a separate active dependency. Preserve these
map and reset findings with the existing candidate record; actual placement,
access, and reset behavior are not established. See the [Bloody Plains audit
supplement](BLOODY_PLAINS_AUDIT_REVIEW.md), [coverage row
33](ZONE_COVERAGE_REGISTER.csv#L33), and [connectivity
record](WORLD_CONNECTIVITY.md#checkpoint-194-bloody-plains).

### Checkpoint 195 — BogenTok disposition

Keep active `bogentok` **DO NOT MOVE**. Preserve its two reciprocal
Underdark boundaries, 72 reset-closed doors, three local shops, generic
bartender and inn callbacks, and Alatorin's active reset imports of local
mobile and object #18718. Add no BogenTok candidate row.

Keep `ud_map2` **RETAIN IN PLACE** under its existing candidate row 114.
Checkpoint 195 adds its non-overlapping source-only WLD approach from
#1128575 west to BogenTok #18700. This is not active ingress and does not
clear a move; it raises the reviewed `ud_map2` subset to 16 and the curated
source-only total to 3,442, while Surf remains 207. See the [BogenTok audit
supplement](BOGENTOK_AUDIT_REVIEW.md), [coverage row
34](ZONE_COVERAGE_REGISTER.csv#L34), and [connectivity
record](WORLD_CONNECTIVITY.md#checkpoint-195-bogentok).

### Checkpoint 196 — Bordello source-only approaches

Keep `surf` and `surface2011` **RETAIN IN PLACE**. Inactive `surf.wld` rooms
#225923 south and #226224 north both target active Bordello room #93600.
Preserve both as source-only approaches, not active ingress, on the existing
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). They
raise the curated source-only total to 3,444 overall and the reviewed Surf
subset to 209; `ud_map2` remains 16. Keep active `bordello` **DO NOT MOVE**
and add no Bordello candidate row. See the
[Bordello audit supplement](BORDELLO_AUDIT_REVIEW.md),
[coverage row 35](ZONE_COVERAGE_REGISTER.csv#L35), and
[connectivity record](WORLD_CONNECTIVITY.md#checkpoint-196-vellas-bordello).

### Checkpoint 197 — Bugentolen source-only approach

The single inactive `surf.wld` approach from room #280727 west into Bugentol
#95800 was already recorded at Checkpoint 27; the current all-95 WLD scan
confirms it and finds no additional approach. Keep it source-only, not active
ingress, on the existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).
Curated totals remain 3,444 overall and 209 for Surf, with `ud_map2` at 16.
Keep `surf` and `surface2011` **RETAIN IN PLACE** and active `bugentol`
**DO NOT MOVE**. This reconciliation adds no candidate row or migration
clearance. See the [Bugentolen audit reconciliation](BUGENTOLEN_AUDIT_REVIEW.md),
[coverage row 42](ZONE_COVERAGE_REGISTER.csv#L42), and
[connectivity record](WORLD_CONNECTIVITY.md#checkpoint-197-bugentolen-reconciliation).

### Checkpoint 198 — Buildings shared template range

Keep active `buildings` **DO NOT MOVE** and do not add it to the attic
candidate register. C++ reserves rooms #97800–#97899 for generated outpost
interiors, loads the tower and guard prototypes, reads the rubble object, and
creates optional portals from object #97801. Normal non-mini startup calls
`init_outposts()`; its loader attempts to instantiate three persisted records
through the room-range allocator. Without the WLD templates, that generator
cannot supply an interior room. The current load wrapper returns a `Building*`
even when `is_loaded()` is false, so the exact startup consequence is not
proven from source alone; the loss of the intended interior is. The static WLD
scan found no authored inbound/outbound edge or source-only approach, but that
does not clear this compiled and persisted dependency. See the [Buildings
audit supplement](BUILDINGS_AUDIT_REVIEW.md), [coverage row
44](ZONE_COVERAGE_REGISTER.csv#L44), and [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-198-buildings-static-shells-and-runtime-outposts).

Any future retirement plan must reconcile the startup loader, outpost command,
room and prototype VNUM consumers, optional guildhall portals, and persisted
outpost records with the feature owner before an attic decision. No database
state was inspected, and no migration or source change is authorized by this
static plan.


### Checkpoint 199 — CBL Hall and Monster Island candidates

Keep active `cbl_hall` **DO NOT MOVE**. Preserve the Heavens boundary, the
#29700 arena return and #29701 inn binding, all local room and portal-object
VNUMs, reset placements, and both shops. The shop lists and merchant inventories
still name #6089, which has no current object prototype; the historical
Tharnadia vial is now #132589. Resolve product intent with the owner before a
separate native change.

Keep unregistered `Monsteri` row
[72](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L72) and `cbl_outpost2` row
[24](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L24) **RETAIN IN PLACE**. Both
share their room/mobile ranges, title, and reset pattern; their rooms overlap
active CBL #29820–#29824. The source history does not name a canonical version
or prove replacement. `cbl_outpost2` mobile VNUMs #1000–#1003 also collide with
active Heavens prototypes of different identities, and the source's raw zone
number 1000 does not align with its #1000–#1004 VNUM band under the register's
VNUM/100 convention. Resolve source ownership, the VNUM overlap, numbering,
and exact tool/editor/persisted references before preparing any path move;
current evidence clears neither candidate for attic.

Keep `ud_map2` row
[114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114) **RETAIN IN PLACE** and
preserve its non-overlapping source-only exit #1124947 D5/down to active CBL
#29703. This brings its reviewed approach subset to 17 and the curated total to
3,445 overall (209 Surf, 17 `ud_map2`). This is not active ingress. See the
[CBL Hall audit supplement](CBL_HALL_AUDIT_REVIEW.md) and
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-199-cbl-hall-static-graph-and-source-only-approach).

### Checkpoint 200: Celestia active-zone retention

Keep active `celestia` **DO NOT MOVE**. Its two active WLD inbounds both land at
#45718; the Palace forms a separate 70-room WLD component; and source-defined
portal objects supply conditional travel paths that are not represented by the
static graph. The area also owns 88 mobile and 46 object prototypes, external
reset consumers, compiled encounter/set/random-zone bindings, and a #45718
skill-beacon room mapping. The title/history scan found no unregistered source
that establishes a Celestia predecessor, replacement, or duplicate zone.

No candidate row or migration clearance is added. Before any separate
owner-led retirement proposal, resolve the intended Palace and #45718 access,
confirm the reset object #63 owner, and finish typed code, editor/tool, and
persisted-state closure. Keep the active source and its IDs in place while those
questions remain open. See the [Celestia audit supplement](CELESTIA_AUDIT_REVIEW.md),
[coverage row 49](ZONE_COVERAGE_REGISTER.csv#L49), and [connectivity
checkpoint](WORLD_CONNECTIVITY.md#checkpoint-200-celestia-static-graph-and-reset-placed-routes).

### Checkpoint 201: Charing and source-only dependencies

Keep active `charing` **DO NOT MOVE**. Preserve its local room, mobile, and
object IDs; the paired Surface connection; reset-placed doors and shops; and
the generic world-quest binding. Room #45037 is pet-shop staging used by the
procedure on #45036, while #45151 remains unexplained staging. Charing’s
partial Quartermaster history does not establish whole-zone replacement.

Keep `surf` candidate row
[97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97) **RETAIN IN PLACE** and
preserve its source-only approaches #209060 south to Charing #45185 and #209660
north to #45205. They mirror Charing’s unresolved active exits but are not
active ingress. Reviewed source-only totals are 3,447 overall, 211 for Surf,
and 17 for `ud_map2`.

Keep `surface2011` candidate row
[98](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L98) **RETAIN IN PLACE**. Its
same-numbered rooms do not contain either reverse Charing approach; the two
unresolved active Charing exits remain within the candidate’s 145 active
unresolved-VNUM references. Preserve map identity without inferring a route or
precedence.

Keep `nexus56` candidate row
[79](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79) **RETAIN IN PLACE**. Its
source-only ZON reset and SHP product both reference Charing object #45037.
The conditional shop-reader fallback does not establish an operating shop.
No Charing attic candidate row or source move is proposed. See the [Charing
audit supplement](CHARING_AUDIT_REVIEW.md), [coverage row
55](ZONE_COVERAGE_REGISTER.csv#L55), and [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-201-charing-reset-aware-static-graph).

### Checkpoint 202: Cityruin and Surf retention

Keep active `cityruin` **DO NOT MOVE**. Preserve its paired Tharnadian Ruin
connection, its active QST/story item relationship with that zone, the imported
Moonshae and Heavens objects, and its compiled justice, hometown, money-changer,
guild-room, and inn bindings. Reset-aware static reach records a closed but
unlocked entrance gate and an asymmetric armory reset; those counts do not
qualify actual route access or safety.

Keep Surf candidate row
[97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97) **RETAIN IN PLACE** and
preserve its source-only `surf.wld` room #262344 west exit to Cityruin #66200.
That route was already recorded and counted in the reviewed source-only Surf
subset; this checkpoint adds no curated approach. Keep the source group and
its existing map references in place.

Keep `nexus56` candidate row
[79](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79) **RETAIN IN PLACE** and
preserve its source-only ZON reset and SHP product references to Cityruin
object #66211. The source-only shop record and conditional shop-reader fallback
do not establish an operating shop.

No Cityruin attic candidate row or migration clearance is added. Before any
separate owner-led retirement proposal, resolve the #66355 inn binding, the
quest remains' descriptions and identities, and the intended door and story
relationship. See the [Cityruin audit supplement](CITYRUIN_AUDIT_REVIEW.md),
[coverage row 58](ZONE_COVERAGE_REGISTER.csv#L58), and [connectivity
checkpoint](WORLD_CONNECTIVITY.md#checkpoint-202-cityruin-reset-aware-access).

### Checkpoint 203: cyrictunnel and Surf retention

Keep active cyrictunnel DO NOT MOVE. Preserve its five reciprocal active
boundaries, current room and prototype VNUMs, and source-defined switch/key
interactions. No cyrictunnel attic candidate row or migration clearance is
added.

Keep Surf candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97)
RETAIN IN PLACE. Preserve the five inactive surf.wld approaches from rooms
#265588, #267393, #270997, #271893, and #273391 to cyrictunnel. They are
source-only, not active ingress. The reviewed source-only Surf subset is now
216 and the curated total is 3,452; ud_map2 remains 17. Resolve Surf’s wider
map ownership and dependency questions before any separate move proposal.
See the [cyrictunnel audit supplement](CYRICTUNNEL_AUDIT_REVIEW.md), [coverage
row 68](ZONE_COVERAGE_REGISTER.csv#L68), and [connectivity
checkpoint](WORLD_CONNECTIVITY.md#checkpoint-203-cyrictunnel-reset-aware-access).

### Checkpoint 204: Dark Forest and Surf retention

Keep active dark_forest DO NOT MOVE. Preserve its three active Surface
arrivals, two reciprocal exits, current room and prototype VNUMs, and
compiled archer binding. No dark_forest attic candidate row or migration
clearance is added.

Keep Surf candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97)
RETAIN IN PLACE. Preserve inactive surf.wld room #208177 north to
dark_forest #75171 as source-only, not active ingress. The reviewed
source-only Surf subset is now 217 and the curated total is 3,453; ud_map2
remains 17. Resolve Surf’s wider map ownership and dependency questions
before any separate move proposal. See the [dark_forest audit
supplement](DARK_FOREST_AUDIT_REVIEW.md), [coverage row
69](ZONE_COVERAGE_REGISTER.csv#L69), and [connectivity
checkpoint](WORLD_CONNECTIVITY.md#checkpoint-204-dark-forest-reset-aware-access).

### Checkpoint 205: Yuan Ti2 and source-only retention

Keep active `yuan_ti2` **DO NOT MOVE**. Preserve its Surface and neighboring
`yuan_ti` reciprocal boundaries, room and prototype VNUMs, reset-placed
chest/key interaction, local shop, and imported Heavens goods. Its title and
followed same-stem history establish no whole-zone replacement.

Keep Surf candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97)
**RETAIN IN PLACE** and preserve `surf.wld` room #281432 west to active room
#80600 as source-only, not active ingress. Keep oldmap8 candidate row
[81](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L81) **RETAIN IN PLACE** and
preserve `oldmap8.wld` room #187735 west to #80600 on the same basis. The
curated source-only totals are now 3,455 overall, 218 for Surf, and 17 for
`ud_map2`. No `yuan_ti2` attic candidate row or migration clearance is added.
See the [Yuan Ti2 audit supplement](YUAN_TI2_AUDIT_REVIEW.md), [coverage
row 352](ZONE_COVERAGE_REGISTER.csv#L352), and [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-205-yuan-ti2-reset-aware-access).

### Checkpoint 206: Outpost and Surf retention

Keep active outpost **DO NOT MOVE**. Preserve its reciprocal Surface boundary,
room and prototype VNUMs, local QST dialogue and Lord Perrin key placement,
and the source-defined key uses in the Outpost Treasury and Tharnadian Ruin
Cathedral. No outpost attic candidate row or migration clearance is added.

Keep Surf candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97)
**RETAIN IN PLACE** and preserve inactive `surf.wld` room #206585 east to
active outpost room #82124 as source-only, not active ingress. The reviewed
Surf subset is now 219 and the curated total is 3,456; `ud_map2` remains 17.
Resolve Surf's wider map ownership and dependency questions before any
separate move proposal. See the [Outpost audit supplement](OUTPOST_AUDIT_REVIEW.md),
[coverage row 227](ZONE_COVERAGE_REGISTER.csv#L227), and [connectivity
checkpoint](WORLD_CONNECTIVITY.md#checkpoint-206-outpost-reset-aware-access).

### Checkpoint 207: Lab and source-only retention

Keep active `lab` **DO NOT MOVE**. Preserve its active ConnectorZones and
Kvarkpass boundaries, local room and prototype VNUMs, enterable portal
objects, five reset-closed door pairs, and the key placements for the swamp,
treasure room, and keep chamber. No `lab` attic candidate row or migration
clearance is added.

Keep Surf candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97)
**RETAIN IN PLACE** and preserve inactive `surf.wld` #269091 west to lab room
#82350 as source-only, not active ingress. Keep Underworld2 candidate row
[116](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L116) **RETAIN IN PLACE** and
preserve `underworld2.wld` #421086 west to #82350 on the same basis. The
reviewed Surf subset is now 220 and the curated total is 3,458; `ud_map2`
remains 17 and Underworld2 has seven recorded source-only approaches. Resolve
map ownership and remaining dependency/tooling questions before any separate
move proposal. See the [Lab audit supplement](LAB_AUDIT_REVIEW.md), [coverage
row 153](ZONE_COVERAGE_REGISTER.csv#L153), and [connectivity
checkpoint](WORLD_CONNECTIVITY.md#checkpoint-207-lab-reset-aware-access).

### Checkpoint 208: kuotoa and source-only retention

Keep active kuotoa DO NOT MOVE. Preserve its reciprocal Underworld boundary, room and prototype VNUMs, and 14 reset-closed door pairs. The all-95 inactive-WLD scan found no non-overlapping source-only approach to rooms #88200–#88236; no kuotoa candidate row or migration clearance is added.

The curated source-only totals remain 3,458 overall, 220 for Surf, and 17 for ud_map2; Underworld2 retains seven recorded approaches. Preserve the already reviewed Surf and Underworld2 references to lab from checkpoint 207. See the [Kuo-Toa Temple audit supplement](KUOTOA_AUDIT_REVIEW.md), [coverage row 151](ZONE_COVERAGE_REGISTER.csv#L151), and [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-208-kuotoa-reset-aware-access).

### Checkpoint 209: udmini and source-only retention

Keep active udmini DO NOT MOVE. Preserve its two active inbound WLD edges, one outbound edge, separate two-room mushroom branch, fall fields, and #88502 outside item consumers. The all-95 inactive-WLD scan found no room overlap or source-only approach to #88500–#88520; no udmini candidate row or migration clearance is added.

Curated source-only totals remain 3,458 overall, 220 for Surf, and 17 for ud_map2; Underworld2 retains seven recorded approaches. No previously reviewed candidate references change. See the [UDMini audit supplement](UDMINI_AUDIT_REVIEW.md), [coverage row 322](ZONE_COVERAGE_REGISTER.csv#L322), and [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-209-udmini-reset-aware-access).

### Checkpoint 210: morg and source-only retention

Keep active `morg` DO NOT MOVE. Preserve rooms #88600–#88629, the 19 reset-used mobiles, the reciprocal UnderDark chasm boundary, the reset-closed entrance and private doors, and the secret cellar route. The all-95 inactive-WLD scan found no room overlap or source-only approach to this zone; the separate UnderDark room titled “Morg's Palace” is on the active chasm approach. No `morg` candidate row or migration clearance is added.

Curated source-only totals remain 3,458 overall, 220 for Surf, and 17 for ud_map2; Underworld2 retains seven recorded approaches. No previously reviewed candidate references change. See the [Morg's Place audit supplement](MORG_AUDIT_REVIEW.md), [coverage row 188](ZONE_COVERAGE_REGISTER.csv#L188), and [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-210-morg-reset-aware-access).

### Checkpoint 211: foggy woods and source-only retention

Keep active foggy_woods **DO NOT MOVE**. Preserve its six reciprocal active WLD boundary pairs, reset-closed and blocked cave route, local rune and key objects, and exact outside item consumers. The all-95 inactive-WLD scan found no source-room overlap or source-only approach into #90000–#90220.

Keep Duris3 candidate row [36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36) **RETAIN IN PLACE**. The active foggy_woods outbound from #90046 west to Duris3 #143836 is already represented there and remains pruned from the normal active world list. Do not add a candidate row or migration clearance for foggy_woods. The unregistered Nexus56 ZON/SHP object references are source-only and do not change curated WLD counts.

Curated source-only totals remain 3,458 overall, 220 for Surf, and 17 for ud_map2; Underworld2 retains seven recorded approaches. See the [Foggy Woods audit supplement](FOGGY_WOODS_AUDIT_REVIEW.md), [coverage row 100](ZONE_COVERAGE_REGISTER.csv#L100), and [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-211-foggy-woods-reset-aware-access).

### Checkpoint 212: dracopit and source-only retention

Keep active `dracopit` **DO NOT MOVE**. Preserve rooms #15900–#15968, the Realm one-way approach, the reciprocal Mountain Castle pair, both teleport targets, the reset-key dependency #37115, and the local item VNUMs used by Heavens and compiled code. The all-95 inactive-WLD scan found no room overlap or source-only approach into #15900–#15968.

No candidate row or migration clearance is added. The Caertannad-to-Dracopit item portal is an active dynamic target already classified through active zone sources; the active key and rune-stone references likewise do not add source-only WLD counts. Curated source-only totals remain 3,458 overall, 220 for Surf, and 17 for ud_map2; Underworld2 retains seven recorded approaches. See the [Dracopit audit supplement](DRACOPIT_AUDIT_REVIEW.md), [coverage row 78](ZONE_COVERAGE_REGISTER.csv#L78), and [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-212-dracopit-reset-aware-access).

### Checkpoint 213: dragonnia and source-only retention

Keep active `dragonnia` **DO NOT MOVE**. Preserve rooms #6800–#6869, its four active WLD arrivals, two reciprocal Surface boundaries, Realm and Winterhaven inbound links, local teleport target, cross-area heart exchanges, Heavens resets, and compiled prototype VNUMs. The active zone has a reset-mode-0 path whose persisted state is not inspected; the reset-aware static graph is documented without a live-access conclusion.

The all-95 inactive-WLD scan found two non-overlapping source-only approaches into Dragonnia #6803: Surf #252526 north and Underworld2 #431340 west. Keep both in their existing candidate records; neither file is active ingress. Surf's reviewed approach subset increases to 221 and the curated source-only total to 3,459 after the Surf edge; adding the Underworld2 edge makes the overall curated total 3,460 and its approach count eight. ud_map2 remains 17. An unregistered Sylvandawn SHP lists Dragonnia emerald #6834 as a product; this source-only shop reference does not change WLD counts. No Dragonnia candidate row or migration clearance is added. See the [Dragonnia audit supplement](DRAGONNIA_AUDIT_REVIEW.md), [coverage row 79](ZONE_COVERAGE_REGISTER.csv#L79), and [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-213-dragonnia-reset-aware-access).

### Checkpoint 214: Dreggan Wood and source-only retention

Keep active `dreggan_wood` **DO NOT MOVE**. Preserve its 188 rooms, Ethereal and Shadamehr reciprocal WLD pairs, directed reach structure, timed Cronus Flower swap, four Dreggan lily placements, Alatorin quest contract and giver stock, and Shadamehr’s #41003 key use. Keep the unresolved Shadamehr object reset #41002 visible for owner review.

The all-95 inactive-WLD scan found no room overlap or source-only approach into #41001–#41189. No `dreggan_wood` candidate row or migration clearance is added. Candidate and curated source-only counts remain 3,460 overall, 221 for Surf, and 17 for ud_map2; Underworld2 retains eight approaches. See the [Dreggan audit supplement](DREGGAN_WOOD_AUDIT_REVIEW.md), [coverage row 81](ZONE_COVERAGE_REGISTER.csv#L81), and [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-214-dreggan-wood-reset-aware-access).

### Checkpoint 215: end terminal marker and shared shop catalog

Keep active `end` **DO NOT MOVE** at the tail of `areas/AREA`. Its WLD/MOB/OBJ sources provide the terminal #9999999 marker, while its ZON supplies the terminal zone label; there is no playable End room or local exit route. Keep the marker in its compiler position.

Preserve the four `end.shp` records and their active #101 placements in Llzazan #36329, Nax #37718, Shady #97685, and Tharnadia #132767. The all-95 inactive-WLD scan found no overlap or approach to an End room because no such room exists. No End candidate row, migration clearance, or curated source-only WLD count is added.

Candidate and curated source-only totals remain 3,460 overall, 221 for Surf, and 17 for ud_map2; Underworld2 retains eight approaches. See the [End audit supplement](END_OF_THE_WORLD_AUDIT_REVIEW.md), [coverage row 89](ZONE_COVERAGE_REGISTER.csv#L89), and [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-215-end-terminal-marker-and-connectivity).

### Checkpoint 216: eth2 and source-only retention

Keep active eth2 **DO NOT MOVE**. Preserve its one-way Realm arrival to #32707, three local WLD components and six type-25 targets, paired local key/door references, and source-defined rose, Heavens monolith, Chaos-kit, and compiled-handler dependencies. Resolve the mismatched #32703/#32710 trapdoor keys before relying on that route.

The all-95 inactive-WLD sweep found no room overlap or target exit into eth2. No eth2 candidate row, migration clearance, or curated source-only WLD count changes. Totals remain 3,460 overall, 221 for Surf, and 17 for ud_map2; Underworld2 retains eight approaches. See the [Fortress of Dreams audit supplement](ETH2_AUDIT_REVIEW.md), [coverage row 91](ZONE_COVERAGE_REGISTER.csv#L91), and [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-216-eth2-reset-aware-access).


### Checkpoint 217: Ethereal Main and source-only retention

Keep active ethereal_main DO NOT MOVE. Preserve its Dream and Dreggan reciprocal WLD pairs, five one-way Prison arrivals, ten active item and pool approaches, local pool and portal targets, Menzellon helper references, Heavens QST items, and imported Celestia and Heavens objects. Keep the unresolved object reset #18202 visible for owner review. The seven extra rift callbacks in the disabled assignment block and their destination values are source records without local reset placements.

The all-95 inactive-WLD sweep found no room overlap or approach into #12401-#12550. No ethereal_main candidate row, migration clearance, or curated source-only WLD count is added. Totals remain 3,460 overall, 221 for Surf, and 17 for ud_map2; Underworld2 retains eight approaches. See the [Ethereal Main audit supplement](ETHEREAL_MAIN_AUDIT_REVIEW.md), [coverage row 92](ZONE_COVERAGE_REGISTER.csv#L92), and [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-217-ethereal-main-reset-and-transport).

### Checkpoint 218: Flind and source-only retention

Keep active flind DO NOT MOVE. Preserve rooms #3400–#3488, the reciprocal underdark boundary, six local teleport-object placements, the golden-passage key and door records, and the local shop. The source-only ud_map2.wld room #1124533 down exit to Flind #3475 is non-overlapping and is not active ingress. Add it to the existing ud_map2 candidate history; do not add a Flind candidate row or migration clearance.

There are now 18 reviewed ud_map2 approaches. Curated source-only totals are 3,461 overall, 221 for Surf, and 18 for ud_map2; Underworld2 retains eight approaches. Keep ud_map2 RETAIN IN PLACE while ownership, map-generator, typed/tool/editor, and persisted-reference closure remain incomplete. See the [Flind audit supplement](FLIND_AUDIT_REVIEW.md), [coverage row 99](ZONE_COVERAGE_REGISTER.csv#L99), and [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-218-flind-reset-and-teleport-reach).

### Checkpoint 219: Glacier and source-only retention

Keep active glacier DO NOT MOVE. Preserve its 105-room source, one-way Surface boundary, negative #90411 target, closed/secret passage, keyed slab with asymmetric resets, and local Princess Falina and King Polar records. The non-overlapping inactive surf.wld exit #208853 north to Glacier #90401 is not active ingress; add it to the existing Surf candidate and do not add a Glacier candidate row or migration clearance.

There are now 222 reviewed Surf approaches. Curated source-only totals are 3,462 overall, 222 for Surf, and 18 for ud_map2; Underworld2 retains eight approaches. Keep Surf RETAIN IN PLACE while ownership, map-generator, typed/tool/editor, and persisted-reference closure remain incomplete. See the [Glacier audit supplement](GLACIER_AUDIT_REVIEW.md), [coverage row 108](ZONE_COVERAGE_REGISTER.csv#L108), and [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-219-glacier-reset-aware-connectivity).

### Checkpoint 220: Goblin Camp and source-only retention

Keep active goblincamp DO NOT MOVE. Preserve its reciprocal Surface route, one-way wh arrival, and local reset relationships for the dump, bone, weapons, and totem. The non-overlapping inactive surf.wld exit #264548 north to Goblin Camp #11400 is not active ingress; add it to the existing Surf candidate and do not add a Goblin Camp candidate row or migration clearance.

There are now 223 reviewed Surf approaches. Curated source-only totals are 3,463 overall, 223 for Surf, and 18 for ud_map2; Underworld2 retains eight approaches. Keep Surf RETAIN IN PLACE while ownership, map-generator, typed/tool/editor, and persisted-reference closure remain incomplete. See the [Goblin Camp audit supplement](GOBLINCAMP_AUDIT_REVIEW.md), [coverage row 109](ZONE_COVERAGE_REGISTER.csv#L109), and [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-220-goblin-camp-active-arrivals).

### Checkpoint 221: Grave and source-only retention

Keep active `grave` **DO NOT MOVE**. Preserve its 204-room source, two reciprocal Surface routes, key-gated crypts, local Ogrebane callback, conditional Phantom arrival, and four non-overlapping inactive-WLD approaches. The two `surf.wld` exits (#275541 west to #8427 and #276137 east to #8400) belong on the existing Surf candidate; the two `underdark2011.wld` exits (#250432 south and #250731 east to #8400) belong on that existing candidate. They are source-only, not active ingress. Do not add a Grave candidate row or migration clearance.

There are now 225 reviewed Surf approaches. Curated source-only totals are 3,467 overall, 225 for Surf, and 18 for ud_map2; Underworld2 retains eight approaches. Keep Surf and `underdark2011` RETAIN IN PLACE while ownership and typed/tool/editor closure remain incomplete. See the [Grave audit supplement](GRAVE_AUDIT_REVIEW.md), [coverage row 113](ZONE_COVERAGE_REGISTER.csv#L113), and [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-221-grave-reset-aware-connectivity).

### Checkpoint 222: Graves and source-only retention

Keep active graves **DO NOT MOVE**. Preserve its 25 room templates and objects used by the random-zone generator, its three assigned entrance procedures, and the 15 active Alatorin/Verspin potion resets. No active or inactive WLD approach reaches Graves, so no candidate approach or Graves candidate row is added. The generated-room path remains conditional on an in-tree staff command; the automatic boot-generation body is excluded by the current macro definition. Do not treat source templates as migration clearance.

Candidate histories and curated totals remain unchanged: 3,467 overall, 225 for Surf, and 18 for ud_map2; Underworld2 retains eight approaches. See the [Graves audit supplement](GRAVES_AUDIT_REVIEW.md), [coverage row 114](ZONE_COVERAGE_REGISTER.csv#L114), and [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-222-graves-template-connectivity).

### Checkpoint 223: Guildhalls retention and Brass imports

Keep active guildhalls DO NOT MOVE. Its 1,000 prototypes are the backing room pool and template catalog for persistent guildhall records. The code allocates and reconnects these room VNUMs and binds doors, heartstones, windows, boards, workshops, and kingdom-store objects. The active Brass zone also resets Guildhalls object #48012 into its Study Room, so the dependency remains even outside the dynamic halls.

No guildhalls attic candidate is proposed. Any future owner-approved retirement would require a separately reviewed migration of saved guildhall and room records, templates, exits, exterior anchors, code VNUM bindings, and the active Brass reset. The existing brass-old-1 candidate remains RETAIN: its inactive ZON also names #48012 in its old Study Room, but that is source-only and not active stock. No WLD approach or curated source-only total is added. See the [Guildhalls audit supplement](GUILDHALLS_AUDIT_REVIEW.md#attic-disposition), [brass-old-1 candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17), and [Guildhalls coverage row](ZONE_COVERAGE_REGISTER.csv#L115).
### Checkpoint 224: Hell, Phlegethos retention and access ambiguity

Keep active hell_four_phlegethos **DO NOT MOVE**. It is a listed full-area contributor with 400 room prototypes, and no source-only WLD candidate or attic candidate row applies. The static graph has no external WLD ingress; the named Bronze Citadel portal prototype has no checked-in reset loader or resolved destination. These are reasons to clarify intended access, not evidence of retirement or migration safety. Do not create a candidate-specific attic plan until an owner establishes a genuine source-only candidate and its dependency closure. See the [audit supplement](HELL_FOUR_PHLEGETHOS_AUDIT_REVIEW.md#attic-disposition) and [coverage row 122](ZONE_COVERAGE_REGISTER.csv#L122).


### Checkpoint 225: Hell One retention and shared dependencies

Keep active hell_one_avernus **DO NOT MOVE**. Its seven active WLD arrivals, one outbound edge, 209 local reset commands, imported River Styx mobiles, 18 external active reset rows, and direct object-procedure bindings establish current source dependencies. The uniform prose and unresolved live return/safety questions are builder follow-ups, not attic eligibility.

No Hell One candidate row or migration plan is warranted. Retain its VNUMs and review each active reset importer and callback before considering any future disposition. See the [Hell One audit supplement](HELL_ONE_AVERNUS_AUDIT_REVIEW.md) and [coverage row 123](ZONE_COVERAGE_REGISTER.csv#L123).


### Checkpoint 226: Hell Three retention with overlapping room VNUM

Keep active hell_three_minauros **DO NOT MOVE**. Its current-area roster membership and connected source room pool establish an active contributor. The shared room #51800 with Hell Two makes live ownership and access ambiguous; it is a builder resolution, not permission to archive either zone.

No candidate row or migration plan is warranted. Preserve both zone contributors until the owner confirms room identity, assigns any needed unique VNUM, and verifies every related exit and return path. See the [Minauros audit supplement](HELL_THREE_MINAUROS_AUDIT_REVIEW.md) and [coverage row 124](ZONE_COVERAGE_REGISTER.csv#L124).


### Checkpoint 227: Hell Two retention and external rod dependency

Keep active `hell_two_dis` **DO NOT MOVE**. It is a listed full-area contributor with 400 room prototypes. Its boundary shares room #51800 with Minauros, so owner, entry, and safe return remain unresolved. The sole rod object #51401 has an active reset consumer in `4horse.zon` and an assigned `dispator` code procedure; the external consumer alone rules out treating it as an unreferenced attic source.

No Hell Two candidate row or migration plan is warranted. Preserve the object VNUM, active reset importer, and procedure binding while the room collision is reviewed separately. See the [Hell Two audit supplement](HELL_TWO_DIS_AUDIT_REVIEW.md) and [coverage row 125](ZONE_COVERAGE_REGISTER.csv#L125).


### Checkpoint 228: Herders retention and source-only approaches

Keep active `herders` **DO NOT MOVE**. The contributor remains in the full world roster with local WLD/MOB/OBJ/ZON/QST/SHP/story sources, twelve native contracts, active code-table consumers, and foreign reset/QST dependencies. Four excluded Surf and Underdark 2011 WLD exits point into active room #94300; they remain source-only and do not qualify Herders or those old WLD files for movement.

No candidate row is created for active Herders. Existing Surf and Underdark 2011 candidate rows continue to record their source-only approaches into #94300. Preserve the local prototypes and their active consumers. Any future source-only map retirement must review its exact boundary records separately from Herders' active area disposition. See the [Herders audit supplement](HERDERS_AUDIT_REVIEW.md) and [coverage row 126](ZONE_COVERAGE_REGISTER.csv#L126).


### Checkpoint 229: Highmoor retention and cross-source references

Keep active `highmoor` **DO NOT MOVE**. It is listed in `areas/AREA`, has an active reciprocal Surface boundary, and is a possible compiled destination of the active Ixarkon veil. The Type-25 portal census found no other portal destination into its room range. No Highmoor candidate row or attic move plan is warranted.

Preserve Surf’s inactive #209922 north exit into Highmoor #19100 in the existing Surf candidate record; it is source-only and does not establish active ingress. Preserve Brass-old-1’s source-only reset of Highmoor object #19127 in that candidate record as well. The compiled Anatomy service for Dr. Fester #19141 and Ixarkon’s code route to #19275 remain active cross-area dependencies. Curated source-only approach totals rise to 3,468 overall and 226 for Surf; ud_map2 remains 18 and Underworld2 retains eight approaches. See the [Highmoor audit supplement](HIGHMOOR_AUDIT_REVIEW.md) and [coverage row 127](ZONE_COVERAGE_REGISTER.csv#L127).

### Checkpoint 230: Highway retention and Mir references

Keep active `highway` **DO NOT MOVE**. Its current `areas/AREA` membership, local WLD/MOB/OBJ/ZON/QST/story sources, three local contracts, five reciprocal active boundary pairs, and typed consumers remain in scope. The Highway reset file loads Mir objects #41915 and #41917, and compiled procedures bind those objects and use additional Mir web/fire objects and mobile helpers. Preserve those active cross-area references and the corresponding Mir prototypes ([Highway audit](SOUTHERN_COASTAL_HIGHWAY_AUDIT_REVIEW.md#checkpoint-230-builder-review-and-current-cross-area-dependencies), [Mir audit](MIR_AUDIT_REVIEW.md)).

No Highway candidate row or attic plan is warranted. Keep the two inactive Surf exits into Highway rooms #41837 and #41732 in Surf's existing candidate record as source-only approaches. They do not change Highway's active disposition or the curated totals. See [coverage row 128](ZONE_COVERAGE_REGISTER.csv#L128).


### Checkpoint 231: Hunt retention and cross-source dependencies

Keep active `hunt` **DO NOT MOVE**. Its active WLD boundaries, local reset inventory, eighteen local outcomes, and active typed consumers remain part of the world roster. No source-only WLD approach or room-number overlap was found, and no Hunt attic candidate row is warranted.

Preserve the active imports of Soulsword #67278, epic rune #359, memory #55449, fountain #72 and Jade head #76613. Preserve incoming Desolate Invaders and Vehicles resets, Myrabolus and Cosmic quest ownership, and the type-25 portal pair between Hunt #13341 and Myrabolus #82627. These references do not make either owner safe to move. Keep Surf's existing candidate records and curated totals unchanged. See the [Hunt audit supplement](HUNT_AUDIT_REVIEW.md#typed-dependencies-and-dynamic-travel) and [coverage row 129](ZONE_COVERAGE_REGISTER.csv#L129).


### Checkpoint 232: Hwood retention and Surf reference

Keep active `hwood` **DO NOT MOVE**. Preserve its active Surface pair, the native room #10857 inn binding, and active Heavens prototypes #363, #368 and #369 used by the elven shop. No Hwood attic candidate row is warranted.

The inactive `surf.wld` room #209648 north exit into Hwood #10800 is now recorded on the existing Surf candidate row 97. It is source-only, not active ingress. The reviewed Surf subset rises to 227 and curated source-only totals rise to 3,469 overall; `ud_map2` remains 18 and Underworld2 remains eight. See the [Surf candidate disposition](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), [Hwood audit supplement](HWOOD_AUDIT_REVIEW.md#source-only-inactive-wld-approach), and [coverage row 130](ZONE_COVERAGE_REGISTER.csv#L130).

### Checkpoint 233: IceCrag retention and typed dependencies

Keep active `icecrag` **DO NOT MOVE**. Its source set includes active WLD/MOB/OBJ/ZON/QST/story files, eleven native contracts, two reciprocal active boundaries, and distinct one-way arrivals/departures. No IceCrag candidate row is warranted. Preserve the existing source-only `surf.wld` and `underdark2011.wld` approaches to #97001 on [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97) and [Underdark 2011 candidate row 115](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L115); they do not provide active ingress.

Retain IceCrag prototypes with their checked-in typed references: Alatorin forge results and closet stock; Barovia2 chest-contained coin resets; the Ravenloft follower load; Air and Sarmiz quest inputs; chaos equipment tables; the inactive Brass-old-1 reset; and the local type-25 portal objects. The Voluntown boundary leads to a separate active contributor with its own ownership and dossier. Curated source-only totals remain 3,469 overall, 227 for Surf, 18 for `ud_map2`, and eight for Underworld2. See the [IceCrag audit supplement](ICECRAG_AUDIT_REVIEW.md), the existing Surf and Underdark 2011 candidate records, and [coverage row 131](ZONE_COVERAGE_REGISTER.csv#L131).

### Checkpoint 237: Ixarkon retention and source-only approaches

Keep active `ixarkon` **DO NOT MOVE**. It remains an active contributor with local stories, custom veil travel and service bindings, active Faang and other zone boundaries, and two Ixxillikor reset imports of Ixarkon mobile #96433. No Ixarkon attic candidate row is warranted. Preserve the two source-only `ud_map2.wld` approaches to room #96492 (#1120305 west and #1120404 north) on the existing `ud_map2` candidate record; neither is active ingress. The title/history review found no named predecessor or replacement, and the “Pillithid” wording does not identify a tracked area source. See the [Ixarkon supplement](IXARKON_AUDIT_REVIEW.md), [coverage row 135](ZONE_COVERAGE_REGISTER.csv#L135), and [existing candidate row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114).

### Checkpoint 238: Ixxillikor retention and typed imports

Keep active `ixxillikor` **DO NOT MOVE**. Its 199-room, 456-exit contributor has four reciprocal active boundaries, local story contracts and services, and no source-only WLD approach or room-number overlap. Preserve its imported Verz1 object #3097 and two Ixxillikor resets of active Ixarkon mobile #96433 with their current owners. No Ixxillikor attic candidate row is warranted. The audit's 195-room SCC and three arrival rooms do not qualify live access or safe return. See the [Ixxillikor audit supplement](IXXILLIKOR_AUDIT_REVIEW.md), [coverage row 136](ZONE_COVERAGE_REGISTER.csv#L136), and [typed reset references](IXXILLIKOR_AUDIT_REVIEW.md#typed-dependency-closure).

### Checkpoint 235: Illithid Arena retention and compiled handler

Keep active `illithid_arena` **DO NOT MOVE**. The current contributor has 18 rooms, 40 local WLD exits, one local fungus mobile, two distinct sword prototypes, and the `squid_arena` room handler bound across the room range. No source-only WLD approach, typed outside reset/QST/SHP/story consumer, external type-25 target, or attic candidate row was found. Preserve the closed/secret pit exits and code-bound WILL behavior during any future builder review.

The #11216/#11217 combatant rooms remain one-way WLD returns with swords and no local mob resets. Their builder question does not create migration eligibility. Curated source-only totals remain 3,469 overall, 227 for Surf, 18 for `ud_map2`, and eight for Underworld2. See the [Illithid Arena audit supplement](ILLITHID_ARENA_AUDIT_REVIEW.md), [coverage row 133](ZONE_COVERAGE_REGISTER.csv#L133), and [coverage row 134](ZONE_COVERAGE_REGISTER.csv#L134) for the next contributor.


### Checkpoint 236: Interspace retention and cross-area references

Keep active `interspa` **DO NOT MOVE**. It remains a listed full-area contributor with a connected 100-room local graph, one one-way Astral Main boundary, two reset records importing Limbo mobile #2, and an external Ravenloft 2 type-25 destination declaration into #99999. The item source does not establish a usable entrance. Preserve those source identities and the boundary while builders resolve the flag and reset questions.

No Interspace attic candidate row is warranted. No active or source-only WLD approach or room overlap was found, so curated source-only totals remain 3,469 overall, 227 for Surf, 18 for `ud_map2`, and eight for Underworld2. See the [Interspace audit supplement](INTERSPACE_AUDIT_REVIEW.md), the [Ravenloft 2 coverage row](ZONE_COVERAGE_REGISTER.csv#L249), and [coverage row 134](ZONE_COVERAGE_REGISTER.csv#L134).

### Checkpoint 239: Jade retention and typed dependencies

Keep active `jade` **DO NOT MOVE** and add no Jade attic candidate. Preserve the four active outside reset consumers and seven external native QST references to Jade objects, the eight currently defined external-object imports in Jade resets, and the checked-in sidecar and compiled service references. Jade's reset also names absent object #6109; confirm its intended disposition before any reset edit. Preserve the two unregistered Surf approaches (#278596 south to Jade #76928 and #279196 north to #76620) on Surf's existing candidate row 97. They remain source-only and do not add active ingress. Curated totals rise to 3,471 overall and 229 for Surf; `ud_map2` remains 18 and Underworld2 eight. See the [Jade audit supplement](JADE_AUDIT_REVIEW.md#typed-dependency-closure) and [coverage row 137](ZONE_COVERAGE_REGISTER.csv#L137).

### Checkpoint 242: Jotunheim retention and bounded source closure

Keep active `jotun` **DO NOT MOVE** and add no Jotunheim attic candidate. Preserve the active Astral object route and return with their current owners, the active Shabo resets of #96077/#96078, and source-only `brass-old-1` G references to #96071/#96076 on Brass's existing candidate record. Retain the separate Storm Giants, Cloud Giant Kingdom, and unregistered Valhalla identities. The bounded current-source scan found no duplicate Jotun mobile/object prototype owner, outside SHP product, or additional typed/code consumer; no area or gameplay source changes are indicated. Static player/account records were not queried. The existing negative WLD target and unqualified live reachability keep the active-area review open; they do not create a migration candidate. Curated source-only totals remain 3,471 overall, 229 for Surf, 18 for `ud_map2`, and eight for Underworld2. See the [Jotunheim audit supplement](JOTUN_AUDIT_REVIEW.md#checkpoint-242-current-catalog-tool-and-semantic-revalidation), [coverage row 140](ZONE_COVERAGE_REGISTER.csv#L140), and [Brass candidate dependencies](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79).

### Checkpoint 243: Juiblex retention and typed dependencies

Keep active `juiblex` **DO NOT MOVE** and add no Juiblex attic candidate. Preserve the Underdark reciprocal WLD pair, the separate Faerie administrative entry, the four foreign NPC load-room exits, and Ravenloft 2's active type-25 portal #59082 to #87528 with its current owner. Preserve Winterhaven's active QST and sidecar use of Juiblex crystal #87510 and Juiblex's five active imported reset objects from Heavens, Unique, and Winterhaven. Keep the inactive `brass-old-1` reset of mandolin #87568 on Brass's existing candidate row 17; it is not active stock. There is no inactive-WLD approach, active outside ZON consumer, outside SHP product, or duplicate local mobile/object definition. Curated source-only totals remain 3,471 overall, 229 for Surf, 18 for `ud_map2`, and eight for Underworld2. The phase transfer, live access, and return questions do not create migration eligibility. See the [Juiblex audit supplement](JUIBLEX_AUDIT_REVIEW.md), [coverage row 141](ZONE_COVERAGE_REGISTER.csv#L141), and [Brass candidate row 17](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17).

### Checkpoint 244: Kastle retention after builder review

Keep active `kastle` **DO NOT MOVE** and add no Kastle attic candidate. Preserve both reciprocal active Surface pairs and both inactive Surf approaches (#265063 north to #99400 and #266570 down to #99439) under their current map owners. Preserve the existing external reset/shop use of Kastle prototypes by Raxthan, Surface Keeps, Valdrak, and Alatorin, and the Tharnadia quill exchange with its own QST and story owner. No new area or gameplay source change is indicated by the twenty builder questions. The current mode-2 resets, incomplete weapon dispatch, captives, and conditional hazards do not establish a migration plan. Curated source-only totals remain 3,471 overall, 229 for Surf, 18 for `ud_map2`, and eight for Underworld2. See the [Kastle audit supplement](KASTLE_AUDIT_REVIEW.md), [coverage row 142](ZONE_COVERAGE_REGISTER.csv#L142), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 245: Kelek retention after builder review

Keep active `kelek` **DO NOT MOVE** and add no Kelek attic candidate. Preserve reciprocal Underdark/Krethik boundaries, isolated room #87951, imported epic node #359 and Deliverer #87950 bindings, and the Church quest-owner correction. The direct builder questions preserve those source identities and distinguish optional routes, supplied materials, physical encounters, and accepted receipts; they reveal no new migration dependency or approach. The all-95 inactive-WLD scan remains negative. No native area or gameplay source changed. Curated source-only totals remain 3,471 overall, 229 for Surf, 18 for `ud_map2`, and eight for Underworld2. See the [Kelek audit supplement](KELEK_AUDIT_REVIEW.md), [coverage row 143](ZONE_COVERAGE_REGISTER.csv#L143), and [existing Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 246: Khildarak retention after builder review

Keep active `khildarak` **DO NOT MOVE** and add no Khildarak attic candidate. Preserve the one source-only `ud_map2.wld` approach (#1124854 up to #17000) with the existing `ud_map2` RETAIN candidate row 114; it is not active ingress. Keep the separate Heavens one-way arrival, three reciprocal active boundaries, foreign reset/shop uses, and local generated/service references with their current owners. The new builder questions do not resolve the open stock, control, missing-prototype, or live qualification choices and create no migration clearance. Curated source-only totals remain 3,471 overall, 229 for Surf, 18 for `ud_map2`, and eight for Underworld2. See the [Khildarak audit supplement](KHILDARAK_STRONGHOLD_AUDIT_REVIEW.md), [coverage row 144](ZONE_COVERAGE_REGISTER.csv#L144), and [`ud_map2` candidate row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114).

### Checkpoint 247: Killing Fields retention after builder review

Keep active `killing_fields` **DO NOT MOVE** and add no Killing Fields attic candidate. Preserve the source-only `surf.wld` approach #279135 south to #33302 and the `surf.zon` mobile #33308/object #33309–#33312, #33314, and #33315 reset references on the existing Surf RETAIN candidate row 97. These declarations are not active ingress, merchant placement, or player stock. Keep the Surface pair, six Bloody Plains leaves including the outward-only #33422→#33597 edge, and compiled destination #33366 under their current owners. The builder questions do not establish a new map owner or a migration path. Curated source-only totals remain 3,471 overall, 229 for Surf, 18 for `ud_map2`, and eight for Underworld2. See the [Killing Fields audit supplement](KILLING_FIELDS_AUDIT_REVIEW.md), [coverage row 145](ZONE_COVERAGE_REGISTER.csv#L145), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 248: Kimordril retention after builder review

Keep active `kimordril` **DO NOT MOVE** and add no Kimordril candidate. Preserve its three reciprocal Surface pairs and the two already-counted source-only `surf.wld` approaches (#216660 east to #95502 and #216664 west to #95655) with Surf candidate row 97. Keep source-only `nexus56` reset/shop references on row 79 and `player_guilds` shop references on row 84; conditional shop-reader fallback does not establish active stock. Preserve active Alatorin and Player Castles dependencies under their current owners. The #214592→#543256 tracked endpoint change and parallel map records do not establish a predecessor or replacement. No move clearance follows. Curated source-only totals remain 3,471 overall, 229 for Surf, 18 for `ud_map2`, and eight for Underworld2. See the [Kimordril audit supplement](KIMORDRIL_AUDIT_REVIEW.md), [coverage row 146](ZONE_COVERAGE_REGISTER.csv#L146), and candidate rows [Surf 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), [nexus56 79](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79), and [player_guilds 84](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L84).


### Checkpoint 249: Hall of Knighthood retention and source-only approach

Keep active `knight` **DO NOT MOVE** and add no Knight candidate. Preserve the reciprocal active Surface boundary and the one source-only `surf.wld` approach (#259810 north to #70900) on Surf candidate row 97. Retain the imported Heavens and White Haven reset objects, Airp and Bastine quest inputs, and compiled Pathfinder, Chaos-kit, and Kithron high-drop references with their existing owners. The 2013 endpoint change and differing inactive old-map VNUMs do not establish a Knight predecessor or replacement. Builder questions do not establish a migration plan. Curated source-only totals rise to 3,472 overall and 230 for Surf; `ud_map2` remains 18 and Underworld2 eight. See the [Hall of Knighthood audit supplement](KNIGHT_AUDIT_REVIEW.md), [coverage row 147](ZONE_COVERAGE_REGISTER.csv#L147), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 250: Kobold retention and source-only dependencies

Keep active `kobold` **DO NOT MOVE** and add no Kobold candidate. Preserve the two reciprocal Surface/Underdark boundaries and Winterhaven's separate one-way arrival and memory completion under their current owners. Retain the inactive `surf.wld` #207232 north approach to #1401 on Surf's existing RETAIN candidate row 97; it is source-only, not active ingress. Keep source-only Sylvandawn's reset and shop references to Kobold bread #1403 on existing RETAIN row 101.

Keep the eleven imported object prototypes, Charing's active #1403 reset/shop listing, Alatorin's ten active #1460 resets, and Surface Keeps' active #1460 reset with their current owners. These references do not grant migration clearance to any contributor. No new candidate is warranted. Curated source-only totals are 3,473 overall, 231 for Surf, 18 for `ud_map2`, and eight for Underworld2. See the [Kobold audit supplement](KOBOLD_AUDIT_REVIEW.md), [coverage row 148](ZONE_COVERAGE_REGISTER.csv#L148), Surf candidate [row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and Sylvandawn candidate [row 101](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L101).

### Checkpoint 252: Krimman DO NOT MOVE retention

Keep active `krimman` **DO NOT MOVE** and add no Krimman candidate. The all-95 inactive-WLD scan found no approach or room overlap. Preserve the single reciprocal Twisted Wood pair and the three active outbound declarations from Eckraldu's load room with their existing source owners; these mobile exits do not create a source-only map candidate or ordinary player route. Keep Quietus's two QST/story item dependencies and the conditional compiled #16408 room reference tied to their current owners. Curated source-only totals remain 3,473 overall, 231 for Surf, 18 for `ud_map2`, and eight for Underworld2. See the [Krimman audit supplement](KRIMMAN_AUDIT_REVIEW.md), [coverage row 150](ZONE_COVERAGE_REGISTER.csv#L150), and the active [Twisted Wood pair](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L685).

### Checkpoint 258: Lava Tubes retention and source-only approaches

Keep active lava_tubes DO NOT MOVE and add no Lava Tubes attic candidate. Preserve the six active arrivals, five reciprocal outbound pairs, and the loader-unresolved #12138 east exit while destination ownership is unresolved. Keep the four inactive surf.wld approaches (#280593, #282376, #282989, and #283874) on Surf's existing RETAIN IN PLACE row 97; they are source-only, not active ingress. Preserve Xexos's Heavens object imports and the White Haven and compiled moonstone consumers under their existing owners. Keep lavcav distinct. Curated source-only totals rise to 3,477 overall and 235 for Surf; ud_map2 remains 18 and Underworld2 eight. See the [Lava Tubes audit supplement](LAVA_TUBES_AUDIT_REVIEW.md), [coverage row 156](ZONE_COVERAGE_REGISTER.csv#L156), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 259: LavCav retention and existing player_guilds dependency

Keep active lavcav DO NOT MOVE and add no LavCav attic candidate. Its only active WLD boundary remains the reciprocal Mountain Tracks pair, and the all-95 inactive-WLD scan found no source-only approach or room overlap. Preserve Ravenloft2's reset of LavCav oil lamp #35589 and retain unregistered player_guilds.shp's room #35552 location reference on existing candidate row 84 RETAIN IN PLACE; the keeper #35554 and product #35551 lack matching current prototypes, and the shop loader fallback may parse the unregistered file when areas/world.shp is absent. This is not confirmed active stock. Curated source-only totals remain 3,477 overall, 235 for Surf, 18 for ud_map2, and eight for Underworld2. See the [Lava Caves audit supplement](UNDERGROUND_LAVA_CAVES_AUDIT_REVIEW.md#bounded-typed-and-code-references), [coverage row 157](ZONE_COVERAGE_REGISTER.csv#L157), and [player_guilds candidate row 84](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L84).

### Checkpoint 260: Arcaneum retention and dynamic portals

Keep active `library` **DO NOT MOVE** and add no Arcaneum attic candidate. The reciprocal Underdark boundary, local type-25 portals, reset object sources, and external typed consumers remain with their current owners. The all-95 inactive-WLD scan found no room overlap or source-only target into the Arcaneum; it adds no candidate row or source-only count. The portal objects connect two local WLD components through dynamic object travel and do not indicate an attic dependency. See the [Arcaneum audit supplement](ARCANEUM_AUDIT_REVIEW.md#checkpoint-260-directed-wld-reachability), [coverage row 158](ZONE_COVERAGE_REGISTER.csv#L158), and [source-only register](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv).

### Checkpoint 261: Limbo retention and shared low-VNUM consumers

Keep active `limbo` **DO NOT MOVE** and add no Limbo attic candidate. Its room #1 is a system recovery destination, the active Desert load room has nine one-way exits into it, and active Heavens code/reset plus three active shops and external resets consume Limbo-owned low-VNUM prototypes. The compiled dracolich assignment also spans Heavens-owned mobiles #7-#10 and #1201. Preserve each reference under its current area or code owner; the thirteen inactive-WLD references and unregistered low-VNUM consumers remain source-only. No candidate row or curated source-only count changes. See the [Limbo source and dependency review](LIMBO_STORY_REVIEW.md#checkpoint-261-builder-questions-and-recommended-resolutions) and [coverage row 159](ZONE_COVERAGE_REGISTER.csv#L159).

### Checkpoint 262: Clavikord retention and existing source-only references

Keep active `lizard` **DO NOT MOVE**. Preserve its reciprocal Surface and Underdark pairs, Heavens and Winterhaven one-way inbounds, Randomness head #6008 consumers, and the three existing source-only typed references. Keep `sylvandawn`, `tharnadia1`, and `tharold` on RETAIN IN PLACE rows 101, 103, and 104 while their source-owner and tool/editor closure remains open. The all-95 inactive-WLD scan found no approach or room overlap; no candidate row or curated source-only count changes. See the [Lizardman Swamps audit supplement](LIZARDMAN_SWAMPS_AUDIT_REVIEW.md#checkpoint-262-builder-questions-and-recommended-resolutions), [coverage row 160](ZONE_COVERAGE_REGISTER.csv#L160), and existing candidate rows [101](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L101), [103](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L103), and [104](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L104).

### Checkpoint 263: Llyrath retention and the isolated pillar destination

Keep active `llyrath` **DO NOT MOVE** and add no Llyrath attic candidate. Preserve the reciprocal Moonshae pair, the local type-25 pillar destination, current-marked river fields, and reset door records under their current owners. The raw WLD entrance cannot reach #94992, but its down exit returns to the local graph and object #94906 declares dynamic travel there; neither gives migration clearance or qualifies safe access. The all-95 inactive-WLD scan found no overlap or source-only approach, so no candidate row or curated count changes. See the [Llyrath source review](LLYRATH_STORY_REVIEW.md#checkpoint-263-directed-wld-reachability), [coverage row 161](ZONE_COVERAGE_REGISTER.csv#L161), and [connectivity section](WORLD_CONNECTIVITY.md#checkpoint-263-llyrath-directed-wld-reachability).

### Checkpoint 264: Llzazan retention and existing ud_map0 reference

Keep Llzazan active and DO NOT MOVE. It has a strongly connected 197-room local
WLD graph and five reciprocal active boundaries, plus one-way arrivals from
Wander and Surface. Retain its Heavens imports, active Wander and Rtun typed
consumers, four shops, and compiled service/start-location bindings under their
current owners. The all-95 inactive-WLD sweep found no Llzazan approach or room
overlap, so add no attic candidate or source-only WLD count. Preserve the
two unregistered ud_map0.zon reset records for Llzazan mobile #36225 on existing
[ud_map0 candidate row 112](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L112);
it is separate from active WLD ingress. See the
[Llzazan audit review](LLZAZAN_STORY_REVIEW.md) and
[coverage row 162](ZONE_COVERAGE_REGISTER.csv#L162).

### Checkpoint 265: lockers retention and storage code

Keep the active lockers contributor DO NOT MOVE. The authored room VNUM band
is hard-coded in the storage module, reused dynamically for private rooms, and
tied to compiled bank-counter/guildhall entry hooks and SQL-backed locker
identities/items. The code also contains a boot-time room-index assignment
whose mapping requires code-owner confirmation. Preserve all source rooms and
persistent storage ownership while that relation remains unresolved.

The all-95 inactive-WLD scan found no locker-room overlap or approach; add no
attic candidate or source-only WLD count. No database operation or migration
was run. See the [locker audit review](LOCKERS_STORY_REVIEW.md),
[coverage row 163](ZONE_COVERAGE_REGISTER.csv#L163), and
[existing-storage dependencies](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv).

### Checkpoint 266: Longhollow retention and overlapping inactive source

Keep active long DO NOT MOVE. Preserve its two reciprocal Surface pairs, local
shop #34438, the active Airship Graveyard QST producer for object #34464, and
the compiled mobile bindings for #34446 and #34429. Preserve one source-only
surf.wld approach from #222252 south to #34401 on Surf's existing retained
candidate row 97; it is not active ingress and adds no Longhollow candidate.
The reviewed Surf subset rises to 236 and curated source-only totals to 3,478
overall, including 236 for Surf.

Keep unregistered temple2 on its existing retained-candidate row 102. Its
distinct WLD, MOB, and OBJ files reuse part of Longhollow's numeric band with
different content, so a blind move, merge, activation, or VNUM reassignment
could bind the wrong source owner. Preserve the source-only player_guilds.shp
shop #34402 product #34405 on candidate row 84 while its identity and
shop-loader exposure remain unresolved; it matches both Longhollow object
#34405 and temple2 object #34405. See the
[Longhollow audit review](LONGHOLLOW_AUDIT_REVIEW.md),
[coverage row 164](ZONE_COVERAGE_REGISTER.csv#L164),
[candidate row 84](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L84), and
[candidate row 102](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L102).

### Checkpoint 267: Ironstar retention and Duris3 room identity

Keep active lornecro DO NOT MOVE. Preserve the reciprocal Surface connection,
the main 98-room directed component, isolated staging rooms #138991 and
#138992, current and fall fields, and effective door-reset distinctions.
Inactive Duris3.wld reuses the 100 Ironstar room VNUMs for differently named
Ocean records; keep those source identities separate on existing candidate
row 36. No source-only non-overlapping WLD approach or new candidate is added.

Preserve the external tradeskill object resets #400280, #400274, and #400277,
Heavens rune stone #359 and its shared epic-stone binding, plus the Soulcatcher
equipment-table references. Keep the tracked .caplor files unchanged. See the
[Ironstar audit review](LORNECRO_AUDIT_REVIEW.md),
[coverage row 165](ZONE_COVERAGE_REGISTER.csv#L165), and
[Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

### Checkpoint 268: The Tower of Darkness retention and Duris3 approaches

Keep active lortower DO NOT MOVE. Preserve its active room, mobile, object,
QST, shop, and reset ownership; retain local shop #134161 with products
#134128–#134130. The existing active boundary remains one-way in current WLD
data: #134000 east to Troll Hills #1883, with no active inbound exit into
Tower. Effective entry, transport, reset admission, doors, and player safety
remain unqualified.

Keep inactive Duris3.wld separate on existing candidate row 36. Its 142
same-numbered room records have different titles from Tower; its 286 exits
from non-overlapping source rooms into Tower are inactive and do not authorize
co-loading, moving, or VNUM reassignment. Exclude 282 matching exits from
same-numbered Duris3 source rooms from that non-overlap count. No new attic
candidate is indicated. The curated source-only count rises by 286 to 3,764.
See the [Tower audit review](LORTOWER_AUDIT_REVIEW.md#title-history-and-attic-disposition),
[coverage row 166](ZONE_COVERAGE_REGISTER.csv#L166),
[Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), and
[source-only approach example](../../../areas/wld/Duris3.wld#L707713).

### Checkpoint 269: Lylr-Meop retention and existing Surf approaches

Keep active lylr DO NOT MOVE. Preserve its two one-way Surface exits, the
separate 40-room and 24-room components, three local shop records, the
Tharnadia counter reset, the imported Heavens bandages, and the compiled
money-changer, world-quest, inn, and locker bindings. Retain unselected
surf.wld approaches #222832 north to #82240 and #224629 south to #82200 on
existing Surf candidate row 97. They are not active ingress and were already
included in the source-only totals. No new candidate, area move, VNUM change,
or gameplay change is indicated. See the
[Lylr audit supplement](LYLR_MEOP_AUDIT_REVIEW.md#migration-disposition),
[coverage row 167](ZONE_COVERAGE_REGISTER.csv#L167), and
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


### Checkpoint 270: Magma retention and inactive approaches

Keep active magma DO NOT MOVE. Preserve its 129 rooms, nineteen mobiles, five
objects, authored QA contract, mode-0 reset behavior, and absent local shop.
Keep the authored controls, sectors, and one-way boundaries intact until their
intended access, return, and hazard policies are confirmed.

Keep inactive Duris3 and Vodgh source references on existing candidate row 36.
Its 260 non-overlapping Duris3 approaches and one Vodgh approach are source-only;
exclude the 256 target matches that start in same-numbered Duris3 Ocean rooms.
These findings were already counted at CP145, so this checkpoint adds no
candidate and does not change the curated source-only total of 3,764. See the
[Magma audit review](MAGMA_AUDIT_REVIEW.md#title-history-and-disposition),
[coverage row 168](ZONE_COVERAGE_REGISTER.csv#L168), and
[Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).


### Checkpoint 271: Malch retention and existing Surf approach

Keep active malch DO NOT MOVE. Preserve all four reciprocal area boundaries,
its three separate exact QST contracts and shops, its typed external reset
imports, and the outside reset consumers of local objects #23616 and #23620.
The current map includes hidden paired switches, conditional access, and fall
markers; its source graph does not qualify safe live travel or service use.

Keep the one inactive surf.wld approach from #267236 to Malch #23600 on existing
Surf candidate row 97. It is source-only and already included in the curated
totals; this checkpoint adds no candidate or count. Keep surf and
surface2011 RETAIN while map ownership and dependency/tool closure remain open.
See the [Malch audit review](MALCH_AUDIT_REVIEW.md#title-history-and-disposition),
[coverage row 169](ZONE_COVERAGE_REGISTER.csv#L169), and
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


### Checkpoint 272: The Forgotten Mansion retention and existing approaches

Keep active mansion DO NOT MOVE. Preserve its three independent native QST
returns, the reciprocal Surface connection, the one-way White House arrival,
all local door/key and fall declarations, and the imported Winterhaven memory,
vapor, and epic-stone behavior.

Keep the Surf #275277 and Underworld2 #436465 approaches to #3500 on their
existing source candidates. They are inactive and already counted. This
checkpoint adds no candidate, area move, or source-only count. Keep both source
families RETAIN while ownership and broader tooling closure remain open. See the
[Mansion audit review](MANSION_AUDIT_REVIEW.md#title-history-and-disposition),
[coverage row 170](ZONE_COVERAGE_REGISTER.csv#L170),
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and
[Underworld2 candidate register](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv).


### Checkpoint 273: Fort Marigot retention and source dependencies

Keep active marigot DO NOT MOVE. Preserve its two reciprocal Surface routes,
fourteen paired door reset declarations, seven shops, Heavens stock, Verz1
counter prop, Kerbien conversation, and ambient Talish line.

Preserve active spshold's typed reset of Marigot object #5308. Keep current
nexus56 and solstice_gh source references to #5319/#5322 with their unregistered
owners; the Surf and Underworld2 approaches remain source-only on existing
candidates. These dependencies were already counted, so this checkpoint adds no
candidate or source-only count. See the
[Marigot source review](MARIGOT_STORY_REVIEW.md#reset-and-cross-area-dependencies),
[coverage row 171](ZONE_COVERAGE_REGISTER.csv#L171),
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and
[Underworld2 candidate register](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv).


### Checkpoint 274: Maze of Undead Army retention and source-only references

Keep active maze_are DO NOT MOVE. Preserve its seven exact native contracts,
reciprocal Surface boundary, four carrier staging rooms, hidden doors, keys,
containers, and reset behavior. Active accounting currently blocks fresh reset
issuance; no fresh stock or played availability is claimed.

Keep the inactive Surf approach #281856 and brass-old-1 reset of Maze object
#94009 on their existing source records. They remain unselected and already
counted. Do not add a candidate, move a map, or change the cumulative source-only
totals for this checkpoint. See the
[Maze audit review](MAZE_OF_UNDEAD_ARMY_AUDIT_REVIEW.md#disposition-and-builder-questions),
[coverage row 172](ZONE_COVERAGE_REGISTER.csv#L172),
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and
[brass-old-1 candidate row 17](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17).


### Checkpoint 275: Mazzolin retention and ud_map2 approach

Keep active mazzolin DO NOT MOVE. Preserve its five-piece accepted return, two
reward objects, reset import of Heavens #358, reciprocal UnderDark routes,
protected room and door flags, and mode-1 renewal policy.

Keep the inactive ud_map2 approach #1125718 south to #21300 on the existing
candidate record. It is not active ingress and was already counted. No new
candidate, area move, or source-only total is added. See the
[Mazzolin audit review](MAZZOLIN_AUDIT_REVIEW.md#title-history-and-disposition),
[coverage row 173](ZONE_COVERAGE_REGISTER.csv#L173), and
[ud_map2 candidate register](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv).


### Checkpoint 276: Menden retention and shop inventory

Keep active menden DO NOT MOVE. Preserve Kitan's exact relic exchange, all four
reciprocal area pairs, the four source-driven services at the house, pools,
altar, and dock, external relic and shop prototypes, and typed Alatorin food
consumers. The coverage inventory now records its three local shops at #88804,
#88807, and #88810.

Keep the two inactive surf.wld approaches to #88846 on existing Surf candidate
row 97. They remain source-only and were already counted. No new approach,
candidate, area move, or source-only total is added. See the
[Menden audit review](MENDEN_AUDIT_REVIEW.md#typed-dependencies-and-scoped-negative-checks),
[coverage row 174](ZONE_COVERAGE_REGISTER.csv#L174), and
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


### Checkpoint 277: Halfling Silver Mine retention and Alterian sources

Keep active mining DO NOT MOVE. Preserve the reciprocal Surface boundary,
native exact whip exchange, all nine non-overlapping source-only approaches,
Alterian's two source-only reset groups, and the separate object #4900
collision. Keep Alterian Mountains, Alterian Wilderness, and Surf RETAIN on
their existing candidate records.

Keep the 69 same-numbered Alterian room identities separate and exclude the
other 151 target matches from the curated approach count. The nine curated
records were already counted; no new candidate, map move, or source-only total
is added. See the [Mining audit review](MINING_AUDIT_REVIEW.md#migration-disposition),
[coverage row 175](ZONE_COVERAGE_REGISTER.csv#L175), and
[Alterian candidate records](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv).

### Checkpoint 278: Mini Zones retention and Surf candidate sources

Keep active `minizones` DO NOT MOVE. Preserve its five shop records, active
cross-area boundaries, nine unresolved exit records, and typed prototype
references. Retain the eleven non-overlapping `surf.wld` approaches on the
existing Surf candidate row 97; these source records are not active ingress.
No candidate row or map move is added. The curated source-only total is 3,775
overall, with 247 Surf records, 18 ud_map2, and eight Underworld2. See the
[Mini Zones audit review](MINIZONES_AUDIT_REVIEW.md#migration-disposition),
[coverage row 176](ZONE_COVERAGE_REGISTER.csv#L176), and
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 279: Deep Ravine retention and existing Surf approaches

Keep active `minopass` DO NOT MOVE. Preserve its six reciprocal active
boundaries, the four already-counted non-overlapping `surf.wld` approaches,
external reset/QST dependencies, and the five source-only `brass-old-1.zon`
reset placements as separate evidence. These findings add no candidate row,
route, or curated source-only count. Totals remain 3,775 overall, with 247 Surf,
18 ud_map2, and eight Underworld2. See the [Deep Ravine audit review](DEEP_RAVINE_AUDIT_REVIEW.md#title-history-and-disposition),
[coverage row 177](ZONE_COVERAGE_REGISTER.csv#L177), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


### Checkpoint 280: Forest of Mir retention and Highway dependencies

Keep active `mir` DO NOT MOVE. Preserve its two reciprocal Highway boundaries,
Bastine and Surface Mini QST/story references, Highway reset imports, Mir's
Heavens object reset, and compiled prototype bindings. The inactive-WLD sweep
adds no approach into Mir, so no candidate row or source-only count changes;
totals remain 3,775 overall, 247 Surf, 18 ud_map2, and eight Underworld2. See
the [Mir audit review](MIR_AUDIT_REVIEW.md#migration-disposition), [coverage row 178](ZONE_COVERAGE_REGISTER.csv#L178), and [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-280-forest-of-mir-reciprocal-boundaries).


### Checkpoint 281: Myrabolus retention and Surf candidate sources

Keep active `mira` DO NOT MOVE. Preserve its four reciprocal active boundary
pairs, the active one-way service/dispersal edges, and all typed external
consumers and service references. Retain Surf exits #235782 south to #82670
and #236382 north to #82500 on existing candidate row 97; they are source-only,
not active ingress. No new candidate row is added. Curated totals rise to
3,777 overall and 249 Surf, with 18 ud_map2 and eight Underworld2. The two local
shops and unresolved O196/#2267 reference remain visible in the [Myrabolus audit review](MYRABOLUS_AUDIT_REVIEW.md#migration-disposition), [coverage row 179](ZONE_COVERAGE_REGISTER.csv#L179), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


### Checkpoint 282: Shadow Forest retention and counted approaches

Keep active `mist` DO NOT MOVE. Preserve its reciprocal Surface paths,
one-way Minizones/NewFor inbounds, and the three already-counted inactive
approaches on Alterian Wilderness and Aopal candidate rows 8 and 10. This
checkpoint adds no route or candidate; curated totals remain 3,777 overall,
249 Surf, 18 ud_map2, and eight Underworld2. See the [Shadow Forest audit review](SHADOW_FOREST_AUDIT_REVIEW.md#migration-disposition), [coverage row 180](ZONE_COVERAGE_REGISTER.csv#L180), and [candidate rows 8 and 10](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L8).


### Checkpoint 283: Chasm retention and existing Surf/Torr approaches

Keep active `mist_chasm` DO NOT MOVE. Preserve its reciprocal Surface pair and
the two already-counted inactive approaches: Surf room #253287 west to #15000
and Torr room #79092 south to #15052. Keep them with existing candidate rows 97
and 107; no route or candidate row is added. Curated totals remain 3,777
overall, 249 Surf, 18 ud_map2, and eight Underworld2. See the [Chasm audit review](MIST_CHASM_AUDIT_REVIEW.md#title-history-and-disposition), [coverage row 181](ZONE_COVERAGE_REGISTER.csv#L181), and [candidate rows 97 and 107](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


### Checkpoint 284: Mistywood retention and counted Surf approaches

Keep active mistywood DO NOT MOVE. Preserve the reciprocal Underworld3 pair,
the Pineholl/Krimman staging inbounds, and the 53 previously counted source-only
Surf approaches on existing candidate row 97. Keep the 38 unresolved active
old-map targets distinct from both active and source-only routes. This
checkpoint adds no route, candidate row, or migration clearance. Curated totals
remain 3,777 overall, 249 Surf, 18 ud_map2, and eight Underworld2. See the
[Mistywood disposition](MISTYWOOD_AUDIT_REVIEW.md#checkpoint-284-title-history-and-bounded-typed-closure), [coverage row 182](ZONE_COVERAGE_REGISTER.csv#L182), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


### Checkpoint 285: Mitashi and Duris3 retention

Keep active mitashi DO NOT MOVE and Duris3 RETAIN IN PLACE. Preserve the 462
non-overlapping Duris3 approaches as already-counted source-only records and
the 454 same-numbered-source target records as a separate map-identity
question. Keep the northern directed Surface edges apart from the reciprocal
southern bay pair. This checkpoint adds no route, candidate row, or migration
clearance. Curated totals remain 3,777 overall, 249 Surf, 18 ud_map2, and eight
Underworld2. See the [Mitashi disposition](MITASHI_AUDIT_REVIEW.md#checkpoint-285-all-source-title-and-map-identity), [coverage row 183](ZONE_COVERAGE_REGISTER.csv#L183), and [Duris3 map candidate](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv).


### Checkpoint 286: Du'Maathe Castle retention and counted approaches

Keep active mntcastl DO NOT MOVE. Retain all five source-only approaches on the
existing Surf and oldmap8 candidate entries; preserve the two reciprocal
active pairs separately. No route, candidate row, or migration clearance is
added. Curated totals remain 3,777 overall, 249 Surf, 18 ud_map2, and eight
Underworld2. See the [Castle disposition](DU_MAATHE_CASTLE_AUDIT_REVIEW.md#checkpoint-286-all-source-title-and-bounded-typed-closure), [coverage row 184](ZONE_COVERAGE_REGISTER.csv#L184), and [existing candidate evidence](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv).


### Checkpoint 287: Mobs Surf retention and existing references

Keep active `mobs_surf` DO NOT MOVE. Preserve its 24 unresolved old-map exits
as unresolved source references, and retain the 25 inactive `surf.zon` reset
commands for its mobile prototypes on existing Surf candidate row 97. Active
cross-zone reset consumers and the local import of Bloody Plains mobile
#33500 remain typed dependencies; they do not add WLD routes or migration
clearance. No candidate row or curated route count changes. Totals remain
3,777 overall, 249 Surf, 18 ud_map2, and eight Underworld2. See the [Mobs_Surf disposition](MOBS_SURF_AUDIT_REVIEW.md#title-source-history-and-disposition), [coverage row 185](ZONE_COVERAGE_REGISTER.csv#L185), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


### Checkpoint 288: Mobs Underdark retention and existing references

Keep active `mobs_underdark` DO NOT MOVE. Preserve its 18 unresolved target
matches on the existing `ud` candidate record, separately from the two
resolved one-way active exits and its typed reset, sidecar, and compiled-item
consumers. The current source review adds no route, candidate row, or migration
clearance. Curated totals remain 3,777 overall, 249 Surf, 18 ud_map2, and eight
Underworld2. See the [Mobs_Underdark disposition](MOBS_UNDERDARK_AUDIT_REVIEW.md#title-source-history-and-disposition), [coverage row 186](ZONE_COVERAGE_REGISTER.csv#L186), and [existing `ud` candidate row 110](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L110).


### Checkpoint 289: Moonshae retention and existing references

Keep active moonshae DO NOT MOVE. Preserve its five reciprocal boundaries,
two pruned inactive-Duris3 port destinations, already-counted source-only Surf
approach, Black Pearl and Cityruin references, Myrloch Vale waterfall lead,
and source-only Nexus56 declarations. The bounded direct typed review and
all-442 title/history scan add no attic candidate, route, or migration
clearance. Curated totals remain 3,777 overall, 249 Surf, 18 ud_map2, and eight
Underworld2. See the [Moonshae disposition](MOONSHAE_ISLAND_AUDIT_REVIEW.md#title-tracked-history-and-disposition), [coverage row 187](ZONE_COVERAGE_REGISTER.csv#L187), and [existing Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


### Checkpoint 290: Morg retention and existing boundary

Keep active morg DO NOT MOVE. Preserve its 30-room source, reciprocal
UnderDark chasm boundary, reset-closed entrance and nested secret route, empty
OBJ catalog, and unresolved Morg's Palace naming question. The bounded typed
scan found no outside direct consumer or source-only WLD approach. This review
adds no attic candidate, route, or migration clearance. Curated totals remain
3,777 overall, 249 Surf, 18 ud_map2, and eight Underworld2. See the [Morg disposition](MORG_AUDIT_REVIEW.md#title-followed-history-and-disposition), [coverage row 188](ZONE_COVERAGE_REGISTER.csv#L188), and [dossier](../zone-stories/MORG.md).


### Checkpoint 291: Moria retention and existing typed dependencies

Keep active moria DO NOT MOVE. Preserve both reciprocal Surface pairs, active
Divine Home's QST/story reference to horseshoes #99061, and compiled
do_shift_prime's destination #99295. Candidate row 73 records the separate
moriatst.shp source and its keeper, stock, and room references into Moria.
Checkpoint 510 further qualifies loader exposure: normal AREA aggregation
omits moriatst, while the runtime fallback can include it when
areas/world.shp is unavailable. See the [Moria disposition](MORIA_AUDIT_REVIEW.md#checkpoint-291-all-442-title-and-bounded-history), [moriatst plan](#checkpoint-510-moriatst-retained-pending-shop-loader-closure), [coverage row 189](ZONE_COVERAGE_REGISTER.csv#L189), and [existing candidate row 73](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L73). The source review adds no Moria candidate row, route, or migration clearance. Curated totals remain 3,777 overall, 249 Surf, 18 ud_map2, and eight Underworld2.


### Checkpoint 292: Mosswood retention and existing references

Keep active moss DO NOT MOVE. Preserve its reciprocal Surface boundary, the
three source-only Surf approaches on existing candidate row 97, and the
bounded typed-reference result. The review adds no Mosswood candidate row,
route, or migration clearance; curated totals remain 3,777 overall, 249 Surf,
18 ud_map2, and eight Underworld2. Keep surf RETAIN IN PLACE. See the
[Mosswood audit review](MOSSWOOD_AUDIT_REVIEW.md#migration-disposition),
[coverage row 190](ZONE_COVERAGE_REGISTER.csv#L190), and [existing Surf
candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 293: Mountain retention and existing candidate references

Keep active mount DO NOT MOVE. Preserve the reciprocal Surface entrance,
one-way Juiblex and Scorch Valley load-room paths, and the two already counted
source-only oldmap8 and Surf approaches on their existing candidate records.
Keep oldmap8, surf, and surface2011 RETAIN until provenance and map ownership
are resolved. This checkpoint adds no Mountain candidate, route, or migration
clearance; curated totals remain 3,777 overall, 249 Surf, 18 ud_map2, and
eight Underworld2. See the [Mountain audit review](MOUNTAIN_OF_THE_BANISHED_AUDIT_REVIEW.md#title-followed-history-and-disposition),
[coverage row 191](ZONE_COVERAGE_REGISTER.csv#L191), and the [attic
candidate register](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv).

### Checkpoint 294: Mountain Tracts retention and existing approaches

Keep active mountaintracks DO NOT MOVE. Preserve its fifteen reciprocal active
boundaries and three already counted Surf approaches on existing Surf
candidate row 97. Keep the source-only Surf records RETAIN IN PLACE; this
checkpoint adds no Mountain candidate, route, or migration clearance.
Curated totals remain 3,777 overall, 249 Surf, 18 ud_map2, and eight
Underworld2. See the [Mountain Tracts audit
review](MOUNTAIN_TRACTS_AUDIT_REVIEW.md#scope-and-disposition),
[coverage row 192](ZONE_COVERAGE_REGISTER.csv#L192), and [existing Surf
candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 295: Miaeril retention and imported Dream material

Keep active mril DO NOT MOVE. Preserve its two existing outbound edges to
Surface #522257 and their asymmetric return status. Keep the imported Dream
object #31317 and the separate Qin recipe in the bounded dependency review;
no inactive-WLD approach, new route, or candidate row is added. Keep surf and
surface2011 RETAIN pending their independent map ownership and dependency
closure. Curated totals remain 3,777 overall, 249 Surf, 18 ud_map2, and eight
Underworld2. See the [Miaeril audit review](MIAERIL_AUDIT_REVIEW.md#typed-references-and-dependency-closure),
[coverage row 193](ZONE_COVERAGE_REGISTER.csv#L193), and the [attic
candidate register](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv).

### Checkpoint 296: Lost Temple retention and static dependencies

Keep active multiclass_temple DO NOT MOVE. Preserve its five reciprocal Surface boundary pairs, three separate local WLD components, touch teleport from #52904 to #52918, and imported mobs_surf mobiles #31402/#31403 as source dependencies. The QST index entry without a local quest remains an owner question. No route, archive candidate, or migration clearance is added. Curated source-only totals remain 3,777 overall, 249 Surf, 18 ud_map2, and eight Underworld2. See the [Temple typed-reference review](MULTICLASS_TEMPLE_AUDIT_REVIEW.md#typed-reference-and-code-closure), [coverage row 194](ZONE_COVERAGE_REGISTER.csv#L194), and the [candidate register](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv).

### Checkpoint 297: Mushroom Forest retention and UD Map 2 approach

Keep active mush1 DO NOT MOVE. Preserve its reciprocal UnderDark entrance #10100/#856059 and add the non-overlapping source-only ud_map2 approach #1124761 north to #10100 to existing candidate row 114. The unregistered approach is not active ingress. The reviewed ud_map2 subset is now 19; curated source-only totals are 3,778 overall, 249 Surf, 19 ud_map2, and eight Underworld2. Keep ud_map2 RETAIN IN PLACE while source-map ownership and tooling closure remain open. See the [Mushroom Forest audit supplement](MUSHROOM_FOREST_AUDIT_REVIEW.md#typed-reference-closure-and-source-only-approach), [coverage row 195](ZONE_COVERAGE_REGISTER.csv#L195), and [candidate row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114).


### Checkpoint 298: Mushroom Caverns retention and shared quest dependencies

Keep active mushroom_caverns DO NOT MOVE. Preserve the five inbound and five outbound active cross-zone records and their five reverse records; the all-95 inactive-WLD sweep found no room overlap or source-only approach. No candidate row or route is added, and curated source-only totals remain 3,778 overall, 249 Surf, 19 ud_map2, and eight Underworld2. Keep the shared Mushroom giver contracts and local-file inventory distinct: all sixteen owned QST blocks are in mobs_underdark.qst, while Mushroom Caverns has no same-stem QST. See the [typed-reference closure](MUSHROOM_CAVERNS_AUDIT_REVIEW.md#shared-quest-ownership-and-typed-reference-closure), [coverage row 196](ZONE_COVERAGE_REGISTER.csv#L196), and the [candidate register](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv).


### Checkpoint 299: Myconid retention and builder follow-ups

Keep active myconid DO NOT MOVE and preserve the two existing inactive source-only approaches with ud_map0 and ud_map2. Retain the exact imported #88502 spore source declarations and compiled destinations #2381/#2368 as reviewed dependencies. CP299 adds twenty direct builder questions and no migration clearance, active route, or candidate row. Curated source-only totals remain 3,778 overall, 249 Surf, 19 ud_map2, and eight Underworld2. See the [typed dependency review](MYCONID_AUDIT_REVIEW.md#typed-dependencies-and-compiled-destinations), [builder questions](MYCONID_AUDIT_REVIEW.md#checkpoint-299-direct-builder-questions-and-recommendations), and [coverage row 197](ZONE_COVERAGE_REGISTER.csv#L197).


### Checkpoint 300: Myrloch Vale retention and builder review

Keep active myrloch_vale DO NOT MOVE. Preserve the reciprocal Moonshae boundary, Caves Q155 shield #26438 dependency, imported reset objects #358/#55455, and direct local bindings. The all-95 inactive-WLD sweep found no approach; no candidate row or migration clearance is added. Curated source-only totals remain 3,778 overall, 249 Surf, 19 ud_map2, and eight Underworld2. See the [typed dependency review](MYRLOCH_VALE_AUDIT_REVIEW.md#typed-dependencies-and-effective-numeric-references), [builder questions](MYRLOCH_VALE_AUDIT_REVIEW.md#checkpoint-300-twenty-direct-builder-questions-and-recommendations), and [coverage row 198](ZONE_COVERAGE_REGISTER.csv#L198).


### Checkpoint 301: Nax retention and compiled dependencies

Keep active nax DO NOT MOVE. Preserve active reciprocal Surface #37800/#625238, source-only Surf #256920 west to #37800 on the existing Surf record, Alatorin’s reset of local object #37742, and the typed imported reset owners. CP301 also records direct compiled routes and services while keeping dynamic behavior unqualified. No route, candidate row, or migration clearance is added; curated source-only totals remain 3,778 overall, 249 Surf, 19 ud_map2, and eight Underworld2. See the [typed and compiled closure](NAX_AUDIT_REVIEW.md#bounded-typed-references-and-compiled-bindings), [coverage row 199](ZONE_COVERAGE_REGISTER.csv#L199), and the [existing Surf retention record](ATTIC_MIGRATION_PLAN.md#L27).


### Checkpoint 302: Negative Material Plane retention and builder review

Keep active negplane DO NOT MOVE. Preserve its two one-way active WLD inbounds, the Ravenloft2 portal to #26600, external orb #26614 consumers, and reviewed external reset sources for local objects. The all-95 inactive-WLD sweep found no approach; no route, candidate, or migration clearance is added. Curated source-only totals remain 3,778 overall, 249 Surf, 19 ud_map2, and eight Underworld2. See the [typed dependency closure](NEGATIVE_MATERIAL_PLANE_AUDIT_REVIEW.md#typed-references-and-external-producers), [twenty builder questions](NEGATIVE_MATERIAL_PLANE_AUDIT_REVIEW.md#checkpoint-302-twenty-direct-builder-questions-and-recommendations), and [coverage row 200](ZONE_COVERAGE_REGISTER.csv#L200).


### Checkpoint 303: New Cave city retention and builder review

Keep active new_cavecity DO NOT MOVE. Preserve its three reciprocal Surface/Underdark boundaries, source-only Surf approach #277907 on existing candidate row 97, local Heavens/Bctdl/Negative Plane recipe dependencies, imported Heavens/White Harbor/Unique objects, Twin Towers Forest’s wizard reset ownership, and compiled bindings. No route or candidate is added. Curated source-only totals remain 3,778 overall, 249 Surf, 19 ud_map2, and eight Underworld2. See the [typed closure](NEW_CAVECITY_AUDIT_REVIEW.md#typed-and-compiled-references), [twenty builder questions](NEW_CAVECITY_AUDIT_REVIEW.md#checkpoint-303-twenty-direct-builder-questions-and-recommendations), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


### Checkpoint 304: Ailvio retention and source-only references

Keep active newbie DO NOT MOVE. It remains a current un-commented areas/AREA contributor. Preserve the active 117-room authored graph, Alatorin’s reset ownership of Ailvio object #29318, cross-area QST/shop/reset prototypes, compiled bindings, and inactive guildhall overlap evidence. Three inactive WLD files point from #29200 north to #29201; inactive EOO, Ravenmis, and old-guildhall type-25/reset references remain source-only. No attic candidate, route, source-area, or migration change is made. Row 202’s local SHP inventory is corrected to eight shop records. See [Ailvio retention disposition](NEWBIE_AUDIT_REVIEW.md#retention-disposition) and [coverage row 202](ZONE_COVERAGE_REGISTER.csv#L202).


### Checkpoint 305: Plains of Life retention and transport

Keep active newbie2 DO NOT MOVE. Preserve its seven-room scripted tutorial, the seven Heavens-owned sack contents, the QST AREA manifest entry without a local QST file, the sidecar, and the two Ailvio transport declarations. There are no active or inactive WLD approaches and no inactive-map room overlaps. The Ailvio type-25 target and stream script are source-backed transport evidence, not migration candidates. No candidate or route is added. See [Plains retention disposition](NEWBIE2_AUDIT_REVIEW.md#retention-disposition) and [coverage row 203](ZONE_COVERAGE_REGISTER.csv#L203).



### Checkpoint 306: Forest of Serenity retention and fixed mirrors

Keep active newfor DO NOT MOVE. Preserve the authored 111-room forest, the main 95-room round-trip component, the separate Gnoll Wood source component, both reset mirrors and their reciprocal fixed targets, the reset-closed hut doors, and the one-way WLD exit to Mist #6366. The area imports no foreign prototypes and has no external typed reset, QST, SHP, sidecar, duplicate-prototype, or type-25 inbound owner. The QST AREA manifest entry has no local QST file. These static declarations do not establish live mirror access, hut-door usability, or safe return. No attic candidate, route, or migration change is made. See [newfor retention disposition](NEWFOR_AUDIT_REVIEW.md#retention-disposition), [dependency review](NEWFOR_AUDIT_REVIEW.md#typed-and-compiled-dependencies), and [coverage row 204](ZONE_COVERAGE_REGISTER.csv#L204).



### Checkpoint 307: Newhaven retention and source-only Surf approach

Keep active newhaven DO NOT MOVE. Preserve its existing source-comprehensive dossier, local QST and revision-two sidecar, four shop blocks, reset imports from Heavens and Malch, local teleport declarations, reciprocal Surface highway boundary, and seven one-way exits from isolated room #35272. Correct coverage row 205 from zero to four SHP records. Preserve the non-overlapping inactive surf.wld approach #221739 north to #35200 on Surf candidate row 97; it is not active ingress. Curated source-only totals are now 3,779 overall, 250 for Surf, 19 for ud_map2, and eight for Underworld2. No active route or migration clearance is added. See [Newhaven retention review](NEWHAVEN_AUDIT_REVIEW.md#retention-disposition), [typed dependencies](NEWHAVEN_AUDIT_REVIEW.md#typed-dependencies-and-corrected-shop-inventory), and [coverage row 205](ZONE_COVERAGE_REGISTER.csv#L205).

### Checkpoint 308: New Hope retention and corrected shop inventory

Keep active newhope DO NOT MOVE and retain its existing comprehensive source dossier, four independent mithral services, guarded-vault material, and bounded typed-dependency findings. Correct coverage row 206 from zero to ten SHP records at #89091, #89096, #89097, #89098, #89115, #89116, #89117, #89118, #89120, and #89131. The all-442 title/history review and entrance-to-all-room reach/return confirmation add no route or migration clearance. The inactive-WLD sweep found no New Hope approach; curated source-only totals remain 3,779 overall, 250 Surf, 19 ud_map2, and eight Underworld2. See [New Hope audit supplement](NEWHOPE_AUDIT_REVIEW.md#checkpoint-308-twenty-direct-builder-questions-and-recommendations), [source dossier](../zone-stories/THE_VILLAGE_OF_NEW_HOPE.md), and [coverage row 206](ZONE_COVERAGE_REGISTER.csv#L206).

### Checkpoint 309: Peril Peaks retention and corrected shop inventory

Keep active `nexus` DO NOT MOVE. Preserve its two reciprocal Mountain Tracks boundaries, all five WLD components, source-defined travel controls and hazards, ten separate item-only outcomes, exact item identities, and source caps. Correct coverage row 207 from zero to two SHP records at #57537 and #57560. The all-95 inactive-WLD sweep found no overlap or approach; no route, candidate, or migration clearance is added. Curated source-only totals remain 3,779 overall, 250 Surf, 19 ud_map2, and eight Underworld2. See [Peril Peaks audit supplement](NEXUS_AUDIT_REVIEW.md#retention-disposition), [comprehensive source dossier](../zone-stories/THE_MOUNTAIN_OF_PERIL_PEAKS.md), and [coverage row 207](ZONE_COVERAGE_REGISTER.csv#L207).


### Checkpoint 310: Reliquary Nexus of Aggression retention and Duris3 source-only references

Keep active nexus_ic DO NOT MOVE. Preserve its four reciprocal Surface crossings, 76/24-room WLD components, and eight local mushroom destinations. Keep Duris3 RETAIN IN PLACE; add the 176 non-overlapping source-only exits into Nexus IC to its existing candidate row 36 while leaving the 26 active-exit-target count unchanged. Exclude 224 target matches that originate at VNUMs owned by active nexus_ic, nexus_uc, or nexus_roper. The source-only records do not establish installed access and do not authorize a world-list or route change. Curated totals rise to 3,955 overall; Surf remains 250, ud_map2 19, and Underworld2 eight. See [Nexus IC retention review](NEXUS_IC_AUDIT_REVIEW.md#retention-disposition), [source dossier](../zone-stories/NEXUS_IC.md), [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), and [coverage row 208](ZONE_COVERAGE_REGISTER.csv#L208).


### Checkpoint 311: Reliquary Nexus of the Corsairs retention and Duris3 source-only references

Keep active nexus_island DO NOT MOVE. Preserve its reciprocal Surface boundary, four door resets, both negative D5 targets for builder review, and 39 fall markers. Keep Duris3 RETAIN IN PLACE; add 121 non-overlapping inactive exits into Nexus Island to existing candidate row 36 and leave the 26 active-exit-target count unchanged. Exclude 279 target matches from source VNUMs owned by active contributors, including 198 same-numbered Duris3 room records, 80 Nexus IC VNUMs, and #130400 in Nexus Roper. The source-only records do not establish installed access or authorize a world-list/route change. Curated totals rise to 4,076 overall; Surf remains 250, ud_map2 19, and Underworld2 eight. See [Nexus Island retention review](NEXUS_ISLAND_AUDIT_REVIEW.md#retention-disposition), [source dossier](../zone-stories/NEXUS_ISLAND.md), [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), and [coverage row 209](ZONE_COVERAGE_REGISTER.csv#L209).

### Checkpoint 312: Nexus UC retention and external entry dependencies

Keep active `nexus_uc` **DO NOT MOVE**. Preserve its Surface boundary, all seven paired item-triggered transitions, the five active placements of shared rift object #468, the level limits, and key-gated tower structure while builders resolve the story questions. Keep `Duris3.wld` **RETAIN IN PLACE**; 171 of its 340 matches into #130200–#130284 begin at source rooms outside active ownership. Record those three approach groups on existing candidate row 36 without changing its 26 active references. The other 169 matches begin at active-owned VNUMs. No route change, candidate clearance, or migration is implied. See [Nexus UC audit supplement](NEXUS_UC_AUDIT_REVIEW.md#retention-disposition) and [coverage row 210](ZONE_COVERAGE_REGISTER.csv#L211).
### Checkpoint 313: Nhavan Island in-place retention

Keep active `nhavan_island_one` **DO NOT MOVE**. Its #20301 east exit to inactive Duris3 room #148782 is already one of the candidate's 26 active WLD references; the full loader prunes it, and no active or inactive WLD ingress into the island was found. Preserve the record on Duris3 candidate row 36 until the intended loaded sea destination or retirement is approved. No source-only approach is added, no candidate count changes, and the curated source-only total remains 4,247. See [Nhavan coverage row 211](ZONE_COVERAGE_REGISTER.csv#L212), [audit retention plan](NHAVAN_ISLAND_ONE_AUDIT_REVIEW.md#safe-retention-plan), and [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

### Checkpoint 314: Northern Wilderness in-place retention

Keep active `northern_wilderness` **DO NOT MOVE**. Preserve its local WLD and the small-bat prototype #12210, which ten active Troll Caves `M` reset rows use at #96981. Retain the one-way #12269 north exit to Centaur #93300 while its missing return is a builder decision. The all-95 inactive-WLD sweep found no source-only approach, and no candidate row is added. Duris3 stays at 26 active references; curated source-only totals remain 4,247 overall, 250 Surf, 19 ud_map2, and eight Underworld2. See [Northern Wilderness retention plan](NORTHERN_WILDERNESS_AUDIT_REVIEW.md#safe-retention-plan), [coverage row 215](ZONE_COVERAGE_REGISTER.csv#L216), and [Troll Caves reset source](../../../areas/zon/troll_caves.zon#L277).

### Checkpoint 315: Nyneth in-place retention

Keep active `nyneth` **DO NOT MOVE**. Preserve its reciprocal Surface entrance, the technical White Harbor inbound to #22994 pending controller review, first-stage portals and Nyneth2/Nyneth3 destinations, return portal #22966, paired door/reset rows, imported Heavens object #360, and shared prototypes used by Alatorin, Earth Plane, Ravenloft2, Stormhold, Nyneth2, and Nyneth3. Preserve the #22995 five-mob population and #22996 staging description while builders resolve the 25%-text versus 100%-reset-field mismatch and the exit-direction prose. Keep inactive `surf.wld` #284533 north to #22900 on existing candidate row 97; this source-only edge is not active ingress. No new candidate row or active route change is proposed. Curated source-only totals rise to 4,248 overall and 251 Surf, with ud_map2 at 19 and Underworld2 at eight. See the [Nyneth retention review](NYNETH_AUDIT_REVIEW.md#title-bounded-history-and-retention), [coverage row 216](ZONE_COVERAGE_REGISTER.csv#L217), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 316: Olympus in-place retention

Keep active `olympus` **DO NOT MOVE**. Preserve its ten-room WLD graph, eight portal prototypes and reset placements, four sentry reset rows, and `#99809` west exit to unresolved #218064. The target appears in both excluded `surf` and `surface2011` maps under different titles; retain it on both existing candidate rows pending an owner decision. Each candidate remains at 145 active references; no candidate count or curated source-only total changes. The special portal procedure's four assignments remain under `#if 0`, and the Herders QST name mention supplies no route or typed target. See the [Olympus retention review](OLYMPUS_AUDIT_REVIEW.md#title-bounded-history-and-retention), [coverage row 221](ZONE_COVERAGE_REGISTER.csv#L222), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [Surface 2011 candidate row 98](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L98).


### Checkpoint 317: Otower in-place retention

Keep active otower **DO NOT MOVE**. Preserve its two reciprocal active boundaries, the White Harbor quest object #55447, Heavens #359, Jindon #82018, local keyed progression, Flog's epic-teacher VNUM, and CHAOS references to objects #11306, #11307, and #11319. Legacy Silver Lady references to object #11300 are unbound or disabled; keep the key's VNUM until that code is reconciled.

The inactive Surf approach at #225677 north to #11300 is not active ingress. Preserve it on existing Surf row 97 and keep Surf **RETAIN IN PLACE**. Surf remains at 145 active references; its reviewed source-only subset is now 252. Curated totals are 4,249 overall, 252 Surf, 19 ud_map2, and eight Underworld2. Add no candidate row and change no active route. See [Otower audit](OTOWER_AUDIT_REVIEW.md#active-graph-door-resets-and-source-only-approach), [coverage row 225](ZONE_COVERAGE_REGISTER.csv#L226), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 318: Patrols in-place retention

Keep active Patrols **DO NOT MOVE**. Preserve its seven load-room VNUMs, 21 mobiles, seven equipment objects, and 28 active positive-unresolved exits. The inactive Surf, Surface2011, and UnderDark2011 targets have conflicting identities and are not active routes. Preserve all 28 records on the unresolved-exit register and retain these map targets on existing candidate rows 97, 98, and 115; their active reference counts remain 145, 145, and 11.

Keep the unregistered Fists of Fury Guildhall on existing candidate row 42 with **RETAIN IN PLACE**. Preserve its two unique-source-room exits to Patrols as identity-ambiguous source-only references; the other twelve come from overlapping room IDs. Keep all seven room and four mobile conflicts attached to that candidate. The compiled room #30504 inn binding and #30511 assignment remain unresolved ownership references. Add no candidate row and infer no route. See [Patrols audit](PATROLS_AUDIT_REVIEW.md#external-identity-collisions-and-typed-dependencies), [coverage row 228](ZONE_COVERAGE_REGISTER.csv#L228), and candidate rows [42](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L42), [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), [98](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L98), and [115](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L115).

### Checkpoint 319: Pharr Valley in-place retention

Keep active `pharrvly` **DO NOT MOVE**. Preserve all 189 rooms, 408 resolving exits, both reciprocal cross-area pairs, the two independent native quest receipts, local material/custody contracts, and the compiled ship/pet bindings. No inactive WLD ingress or room-ID overlap was found, and this checkpoint adds no route or candidate row.

Keep the unregistered Nexus56 apple product and G reset attached to existing candidate row 79 with its RETAIN disposition. They reference local apple #40208 but remain source-only; the shop-reader fallback is conditional, and the reset is outside the production zone list. Resolve owner and loader-path intent before any separate migration decision. See [Pharr Valley audit](PHARR_VALLEY_AUDIT_REVIEW.md#checkpoint-319-builder-questions-and-recommended-resolutions), [coverage row 229](ZONE_COVERAGE_REGISTER.csv#L229), and [Nexus56 candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79).

### Checkpoint 320: Pine Hollow in-place retention

Keep active `pineholl` **DO NOT MOVE**. Preserve its 166-room source, all 437 resolved exits, both reciprocal cross-area pairs, the Rftjngle one-way inbound, the isolated five-room NPC staging component, seven native quest contracts, and exact foreign QST consumers. The 95-source inactive-WLD scan found no overlap or ingress; add no route or candidate row.

Keep Luiren on existing candidate row 62 with **RETAIN IN PLACE**. Its 44 QST giver headers and 16 SHP product, eight keeper, and two shop-room values overlap Pine Hollow VNUMs, but Luiren has no current WLD/MOB sources and its old `O` shop records are rejected by the current loader. Preserve the potential fallback compatibility hazard and identity ambiguity; do not treat it as an active shop or replacement. Resolve source ownership and loader input before any separate move decision. See [Pine Hollow review](PINE_HOLLOW_AUDIT_REVIEW.md#luiren-candidate-identity-and-old-shop-data), [coverage row 230](ZONE_COVERAGE_REGISTER.csv#L230), and [candidate row 62](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L62).

### Checkpoint 321: Plane of Air and Water Plane retention

Keep active `plane_air_one` **DO NOT MOVE**. Preserve its 125-room WLD, nine active inbound records, two reciprocal outbound pairs, three directed one-way rooms, smoke-pocket portal, Ethereal color pool, and all Air/Heavens item identities. No gameplay, reset, or active-route change is proposed.

Keep unregistered `waterplane` on existing candidate row 120 with **RETAIN IN PLACE**. Preserve five unique-source exits into Plane of Air and five into Heavens as source-only references. Keep the five Air-targeting exits that start at active-owned source VNUMs attached as identity overlaps; the full WLD definitions overlap active Limbo/Heavens at #1, #3, #4, #5, #10, and #12. Do not promote these records to active ingress or add a candidate row. Candidate active-inbound count remains zero. Curated source-only totals rise by ten to 4,261 overall; Surf remains 252, ud_map2 19, and Underworld2 eight. See [Plane of Air review](PLANE_OF_AIR_AUDIT_REVIEW.md#inactive-water-plane-approaches-and-identity-overlaps), [Water Plane candidate row 120](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L120), [Water Plane component row 471](ZONE_COVERAGE_REGISTER.csv#L471), and [Air coverage row 231](ZONE_COVERAGE_REGISTER.csv#L231).

### Checkpoint 322: Plane of Earth in-place retention

Keep active `plane_earth_one` **DO NOT MOVE**. Preserve its 129-room WLD, eight active inbound records, reciprocal Earthp boundary, two one-way technical outbounds, Negative Plane fissure target, Ethereal pool target and damage behavior, and the Heavens four-object contract. The all-95 inactive-WLD sweep found no room overlap or approach; no candidate row or source-only count changes. Keep the Plane of Earth mobile/object VNUMs as their current shared owners while Earth, Earthp, Tharnadian Ruin, and Heavens retain their exact imports. The technical room bounds, helper-list room VNUMs, access, reset availability, and story intent remain builder questions. See the [Plane of Earth review](PLANE_OF_EARTH_AUDIT_REVIEW.md#title-followed-history-and-retention), [coverage row 232](ZONE_COVERAGE_REGISTER.csv#L232), and [connectivity](WORLD_CONNECTIVITY.md#checkpoint-322-plane-of-earth-graph-and-external-routes).
### Checkpoint 323: Plane of Fire and retained Brass candidates

Keep active `plane_fire_one` **DO NOT MOVE**. Preserve its 125-room map, nine inbound and two outbound active WLD records, reciprocal Brass boundary, one-way Firep boundary endpoints, Fire/Smoke type-25 pair, damaging Ethereal pool, Negative Plane destination, Heavens object requirements, and Brass guardian integration. The all-95 inactive-WLD sweep adds one unique-source approach from `brass-old-3` room #25201 to Fire #25455. Preserve `brass-old-1` room #139100's approach to #25455 as identity-ambiguous because that source VNUM is reused with different room text by active Brass. Neither is active ingress. Add no route or candidate row; candidate active-inbound counts remain zero. Keep Sylvandawn source-only shop products #25400/#25401 with its existing **RETAIN IN PLACE** candidate row; they are not active stock. Curated source-only totals rise by one to 4,262 (Surf 252, ud_map2 19, Underworld2 eight). See the [Plane of Fire review](PLANE_OF_FIRE_AUDIT_REVIEW.md#inactive-wld-approaches-and-candidate-records), [Fire coverage entry (CSV line 233)](ZONE_COVERAGE_REGISTER.csv#L233), and [connectivity](WORLD_CONNECTIVITY.md#checkpoint-323-plane-of-fire-graph-and-external-routes).

### Checkpoint 324: Plane of Water in-place retention

Keep active `plane_water_one` **DO NOT MOVE**. Preserve its 125-room WLD, eight active inbound records, one reciprocal Sea Kingdom pair, one weak and strongly connected component, Heavens' fixed item VNUMs, Olhydra callback and ship-quest consumer, the active pools at #23324, Negative Plane lake to #23233, and the local damaging Ethereal pool. The all-95 inactive-WLD sweep found no room overlap or approach into #23201–#23325. Make no active route or candidate change. Keep unregistered `waterplane` on existing candidate row 120 with **RETAIN IN PLACE** for its prior unique-source Air/Heavens approaches and identity overlaps; it has no current Water-targeting edge. Curated source-only totals remain 4,262 overall (Surf 252, ud_map2 19, Underworld2 eight). See the [Plane of Water review](PLANE_OF_WATER_AUDIT_REVIEW.md#inactive-wld-approaches-and-attic-candidates), [Water coverage entry (CSV line 234)](ZONE_COVERAGE_REGISTER.csv#L234), and [connectivity](WORLD_CONNECTIVITY.md#checkpoint-324-plane-of-water-graph-and-external-routes).

### Checkpoint 325: Player Castles and existing candidate importers

Keep active `player_castles` **DO NOT MOVE**. Its source set owns three WLD
rooms, ten mobile prototypes, 59 object prototypes, shared guildhall object
callbacks, and the #11010/#11011 construction templates. Preserve 20 active
external reset/shop references, including CBL Hall's 16 object resets,
Heavens' Player Council board, Surface's Dalvik reset, and Tharnadia's
lock/key stock. Preserve its local shop's twelve listed product IDs while
the owner resolves the absent checked-in object prototype #6089.

The all-95 inactive-WLD scan adds no approach or room overlap, and there is
no active WLD boundary. Exact typed scans add source-only Player Castles
imports to 44 existing candidate rows: 41 unregistered ZON components with
174 object-reset records, `surf.zon` with one mobile reset, and two SHP
components with four board/lock/key product records. Keep every candidate
RETAIN and keep active-inbound counts and curated source-only WLD totals at
4,262 overall (Surf 252, ud_map2 19, Underworld2 eight). No candidate is
added or cleared. See the [Player Castles audit review](PLAYER_CASTLES_AUDIT_REVIEW.md#typed-dependencies-and-external-consumers),
[candidate dependency register](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv),
[coverage row 235](ZONE_COVERAGE_REGISTER.csv#L235), and
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-325-player-castles-graph-and-inactive-wld-sweep).

### Checkpoint 326: Port Skythic and existing Duris3 approaches

Keep active `PortSkythic` **DO NOT MOVE**. Preserve its 110-room WLD, 20
mobile prototypes, 15 object prototypes, reset-controlled door states, and
current source ownership. Its 218 authored exits form three separate strong
components: the city, a six-room sunken ship, and a standalone rift. No active
WLD boundary enters or leaves the area.

Keep `Duris3.wld` on existing candidate row 36 with **RETAIN IN PLACE**. Add
224 distinct approaches from 222 source rooms into PortSkythic to that
candidate's preservation plan; classify its 216 same-numbered `The Ocean`
source-room edges separately as identity overlaps. Do not add them as active
routes or count them as unique-source approaches. The candidate's 26 active
inbound references stay unchanged. Curated source-only totals increase by
224 to 4,486 overall (Surf 252, ud_map2 19, Underworld2 eight). No candidate
is created or cleared. See the [Port Skythic audit review](PORTSKYTHIC_AUDIT_REVIEW.md#inactive-wld-approaches-and-overlapping-room-identities),
[coverage row 237](ZONE_COVERAGE_REGISTER.csv#L237), and
[Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

### Checkpoint 327: Potions and the retained Brass candidate

Keep active `potions` **DO NOT MOVE**. Preserve both room clusters, the two
guard resets, seven one-way exits to active Ceothia room #81094, and six
unresolved exits to #25087. Keep inactive `brass-old-2` on existing
candidate row 18 with **RETAIN IN PLACE**: its seven active source references
remain six from Potions and one from Realm. Potions has no active WLD inbound,
and the all-95 inactive-WLD scan found no approach or room overlap. No
candidate or curated source-only WLD count changes; the total remains 4,486
(Surf 252, ud_map2 19, Underworld2 eight). See the [Potions audit
review](POTIONS_AUDIT_REVIEW.md#title-followed-history-and-retention),
[coverage row 238](ZONE_COVERAGE_REGISTER.csv#L238), [candidate row
18](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L18), and [connectivity
checkpoint](WORLD_CONNECTIVITY.md#checkpoint-327-potions-graph-and-inactive-wld-sweep).

### Checkpoint 328: Pragtog and two existing source-only candidates

Keep active pragtog DO NOT MOVE. Its two WLD boundaries are reciprocal and
its reset ITEM_TELEPORT objects provide additional command-triggered routes;
retain their source placements while access and return remain unqualified.

Preserve two source-only approaches to Pragtog #10649 on existing inactive
candidate rows: ud_map1 room #1115029 up, and ud_map2 room #1126541 up.
Neither is active ingress, and neither changes the candidates' active
inbound counts of one Shafts reference each. ud_map1 now records two reviewed
unique-source approaches, including the earlier DLSC edge; ud_map2 records
20. Keep both candidate rows RETAIN IN PLACE. Curated source-only totals
rise by two to 4,488 overall (Surf 252, ud_map1 two, ud_map2 20, Underworld2
eight). No candidate is created or cleared. See the
[Pragtog audit review](PRAGTOG_AUDIT_REVIEW.md#inactive-wld-approaches-and-candidate-plans),
[coverage row 239](ZONE_COVERAGE_REGISTER.csv#L239),
[ud_map1 candidate row 113](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L113),
and [ud_map2 candidate row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114).

### Checkpoint 329: Carthapia retention and the existing Surf record

Keep active prison DO NOT MOVE. Any separate move review would need to
retain the reciprocal Surface pair, inspect the seven one-way outbound
planes, and account for source-only Nexus56 item references, external reset
producers, and the existing inactive surf.wld approach. No attic candidate
or curated source-only count changes in this checkpoint.

Surf candidate row 97 already records the historical surf.wld room
#263479 north exit to #7497 as source-only. Keep that record distinct from
active ingress and leave Surf RETAIN IN PLACE with its 145 active
references. Curated source-only totals remain 4,488 overall (Surf 252,
ud_map1 two, ud_map2 20, Underworld2 eight). See [Prison coverage line
240](ZONE_COVERAGE_REGISTER.csv#L240), the [Prison audit
review](PRISONS_OF_CARTHAPIA_AUDIT_REVIEW.md#checkpoint-329-reset-aware-access-and-builder-decisions),
and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 330: Prisonb in-place retention and external producer

Keep active prisonb DO NOT MOVE and add no attic candidate. Preserve its two
reciprocal Fortb boundaries: #38149 to Prisonb #43000 and #38121 to #43041.
Fortb's active reset at #38106 stocks Prisonb object #43014, whose ENTER
target is #43020; the Prisonb plank at #43022 returns to Fortb #38106. The
producer is an active area dependency and must be included in any separately
authorized path migration plan.

Retain local object imports #359 and #67230 in their documented reset roles.
The all-95 inactive-WLD scan found no overlap or approach into Prisonb, and
the title/history review found no predecessor or replacement. These findings
do not clear a move: preserve the two boundaries, external ship reset,
prototype ownership, and source/editor consumers before evaluating any
future candidate. No candidate row or active reference count changes here.
See [coverage line 241](ZONE_COVERAGE_REGISTER.csv#L241), the [Prisonb audit
review](PRISON_OF_FORT_BOYARD_AUDIT_REVIEW.md#checkpoint-330-reset-aware-arrival-reach-and-builder-decisions),
and [Fort Boyard dossier](../zone-stories/PRISON_OF_FORT_BOYARD.md).

### Checkpoint 331: Purple Worm Lair retention and ud map2 evidence

Keep active pworm DO NOT MOVE and add no attic candidate. Preserve its four
reciprocal Underworld/Underdark boundaries and the active Alatorin reset-role
dependency: nine Alatorin M rows load local mobile #42501 in rooms #83972,
#83974, and #83975. These rows are source dependencies; they do not prove
that each reset executed.

The three ud_map2.wld approaches to pworm (#1123784 west to #42525,
#1123975 east to #42542, and #1124682 north to #42500) remain source-only on
existing candidate row 114. That candidate retains 20 reviewed approaches
and one active Shafts reference; neither number changes here. Preserve the
map source, its active Shafts relationship, and all recorded approaches while
owner, map-generator, and tool/editor closure remain open. The current Lair
title/history comparison found no predecessor or replacement. No candidate
row or area membership changes in this checkpoint. See [coverage line
242](ZONE_COVERAGE_REGISTER.csv#L242), the [Purple Worm audit
review](LAIR_OF_THE_PURPLE_WORM_AUDIT_REVIEW.md#checkpoint-331-reset-aware-arrival-reach-and-builder-decisions),
and [ud_map2 candidate row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114).

### Checkpoint 332: Neverwind Valley retention and Surf approach


### Checkpoint 333: Quietus Quay retention and typed consumers


### Checkpoint 334: Rabble in-place retention and Heavens boundary

Keep active rabble DO NOT MOVE and add no attic candidate. Preserve the reciprocal Heavens #1234 / Rabble #65100 boundary, the local Frothe shop, and its typed reset/shop records. Keep Frothe's active Heavens imports (#377, #347, #399, and #53) under their source owners; the #6094 and #6098 G rows have no current OBJ definitions and need owner identification before any separate source change. Three source-only object files also reuse VNUM #53, so preserve the active Heavens resolution and keep those inactive sources distinct. The all-95 inactive-WLD scan found no Rabble overlap or approach. Curated source-only totals remain 4,488 (Surf 252, `ud_map1` two, `ud_map2` 20, Underworld2 eight). See [coverage row 245](ZONE_COVERAGE_REGISTER.csv#L245), the [Rabble audit](RABBLE_AUDIT_REVIEW.md#checkpoint-334-builder-questions-and-recommended-resolutions), and [setting dossier](../zone-stories/RABBLE.md).

### Checkpoint 335: Ogre Raiders retention and Surf approach

Keep active raiders DO NOT MOVE. Preserve the reciprocal Surface #593506 / Raiders #8900 boundary and the active SurfaceKeeps P placement of local sword #8900 inside sack #120028. Preserve the non-overlapping source-only surf.wld #277145 north exit to Raiders #8900 on existing Surf candidate row 97; it is map evidence, not active ingress. Surf remains at 145 active references, while its reviewed source-only subset rises to 253 and the curated source-only total to 4,489. This finding adds no candidate and clears no move for surf or surface2011. See the [Raiders audit](RAIDERS_AUDIT_REVIEW.md#checkpoint-335-builder-questions-and-recommended-resolutions), [coverage row 246](ZONE_COVERAGE_REGISTER.csv#L246), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 336: Castle Ravenloft active retention

Keep active ravenloft DO NOT MOVE and add no attic candidate. Preserve its six inbound routes and three reciprocal outbounds; the local story sidecar and QST identities; the five external QST item-command workflows and four exact external sidecar object references; 36 cross-zone reset groups; the Catacombs type-25 destination to local room #58456; and the current shared code assignments. These references retain their owning areas and do not establish guaranteed live stock or migration eligibility. The all-95 inactive-WLD scan found no room overlap or source-only approach, so the curated source-only total remains 4,489 overall (Surf 253, ud_map1 two, ud_map2 20, Underworld2 eight). See the [Ravenloft audit](RAVENLOFT_AUDIT_REVIEW.md#checkpoint-336-castle-ravenloft-reset-aware-arrivals) and [coverage row 248](ZONE_COVERAGE_REGISTER.csv#L248).

### Checkpoint 337: Ravenloft 2 in-place retention

Keep active `ravenloft2` DO NOT MOVE and add no attic candidate. Preserve both reciprocal Castle boundaries, the seven one-way outbounds, its local story sidecar and QST identities, the imported reset prototypes, the Castle reset of local handprint #59326, and the three type-25 declarations (#59081 to Astral Main #19721, #59082 to Juiblex #87528, and reward #59310 to Interspace #99999). Keep all prototype ownership with its defining area; these declarations do not qualify stock, activation, or safe travel. The inactive-WLD scan found no room overlap or approach, so the curated source-only total remains 4,489 overall (Surf 253, ud_map1 two, ud_map2 20, Underworld2 eight). See the [Ravenloft 2 audit](RAVENLOFT2_AUDIT_REVIEW.md#typed-references-and-ownership) and [coverage row 249](ZONE_COVERAGE_REGISTER.csv#L249).

### Checkpoint 338: Raxthan active retention and Surf source-only approach

Keep active `raxthan` DO NOT MOVE and add no Raxthan attic candidate. Preserve its three reciprocal WLD boundaries, the Alatorin-owned Bloodbeast-heart receipt, shared CHAOS ring and guarded `shift_prime` references, the single dealer shop, and current room/reset ownership. Preserve inactive `surf.wld` #266966 north to Raxthan #42980 on existing Surf candidate row 97. It is a source-only approach, not active ingress, and was already included in the curated source-only total; no Surf or Raxthan move is cleared. Current totals remain 4,489 overall, including 253 reviewed Surf source-only references. See the [Yerdonia audit](YERDONIA_AUDIT_REVIEW.md#checkpoint-338-raxthan-reset-aware-access-and-builder-decisions), [coverage row 250](ZONE_COVERAGE_REGISTER.csv#L250), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 339: Faerie Realm in-place retention and dependencies

Keep active realm DO NOT MOVE and add no Realm attic candidate. Preserve the
reciprocal Surface #531694/#14002 boundary, the 22 resolved one-way dispersal
exits, and the two unresolved exits that point only to Duris3 #164929 and
brass-old-2 #25087. Preserve source-only Surf #244463 south to Realm #14002 on
existing Surf candidate row 97; it is not active ingress and raises curated
totals to 4,490 overall, including 254 Surf references.

Keep the typed prototype owners in place: active Alatorin/Astral Main/Brass/
Caertannad/Nax and elemental-plane reset consumers, Alatorin's separate
#14126 QST receipt, three active shop products, and the shared CHAOS use of
#14045. Preserve inactive brass-old-1, nexus56, sylvandawn, and player_guilds
references on their existing candidate rows. No source-only reference grants
permission to move its owner. See the [Faerie Realm audit](FAERIE_REALM_AUDIT_REVIEW.md),
[coverage row 251](ZONE_COVERAGE_REGISTER.csv#L251), [Surf row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97),
[Duris3 row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), and
[brass-old-2 row 18](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L18).

### Checkpoint 340: Rift Valley Jungle in-place retention and dependencies

Keep active rftjngle DO NOT MOVE and add no Jungle attic candidate. Preserve
the four reciprocal active arrivals, five one-way resolved departures, and
the unresolved #80460 south target #228371. Keep both definitions of that
number isolated on Surf and Surface2011; only inactive surf.wld supplies the
north return. Preserve the two source-only Surf approaches (#226279 south to
#80469 and #228371 north to #80460) on existing Surf candidate row 97. These
are not active ingress. Curated source-only totals rise to 4,492 overall,
including 256 Surf references.

Preserve all eight local shop records and the exact story/QST ownership. Keep
active shipy and surfacekeeps product references to Jungle object #80186,
the shared CHAOS entries, and the two local world_quest mobile assignments.
Preserve source-only nexus56.shp products #80074 and #80002 on existing
Nexus56 row 79. No source-only reference authorizes moving its owner. See the
[Rift Valley Jungle audit](RFTJNGLE_AUDIT_REVIEW.md),
[coverage row 252](ZONE_COVERAGE_REGISTER.csv#L252),
[Surf row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97),
[Surface2011 row 98](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L98), and
[Nexus56 row 79](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79).

### Checkpoint 341: River Styx in-place retention and shared mobiles

Keep active `river_styx` DO NOT MOVE and add no attic-candidate row or
migration plan. Preserve its one-way River Styx #90700 south exit to Hell One
#51001 and all local fountain-pool portal pairs. Its mobile prototypes are
used by 205 active reset references: 193 M loads plus 12 F follow rows; the
M counts are 60 in Avernus, 128 in Hell One, and five in Ravenloft 2. Keep the
defining mobile prototypes with River Styx and preserve each importer's
ownership until an area owner resolves the dependencies.

The local Charon #90708 and dock-horn prose do not match the compiled ferry
prototypes (#60008 mobile and #60003 object) initialized on the Surface route.
Record that as a builder follow-up; it does not authorize a zone move. The
all-445-other-WLD scan found no inbound or source-only approach and no room
overlap, so curated source-only totals stay 4,492 overall, including 256 Surf
references. See the [River Styx audit](RIVER_STYX_AUDIT_REVIEW.md),
[coverage row 253](ZONE_COVERAGE_REGISTER.csv#L253), and
[source dossier](../zone-stories/RIVER_STYX.md).

### Checkpoint 342: Rogue Plains in-place retention and Surf approach

Keep active roguerai DO NOT MOVE and add no Rogue Plains candidate row or
migration plan. Preserve its two reciprocal active Surface routes and the
unresolved #75895 north exit into Duris3 under existing candidate row 36.
The all-95-inactive-WLD sweep adds one non-overlapping source-only approach
from surf.wld #201299 north to Rogue Plains #75801. Preserve that edge with
the existing Surf RETAIN IN PLACE candidate row 97; it is not loaded ingress
or move clearance.

The active Surf-reference count stays 145. Its curated reviewed source-only
subset rises from 256 to 257, and the global curated source-only total rises
from 4,492 to 4,493 (ud_map1 two, ud_map2 20, Underworld2 eight). Keep the
Duris3 and Surf owners in place until they make their respective map decisions.
See the [Rogue Plains audit](ROGUE_PLAINS_AUDIT_REVIEW.md),
[coverage row 254](ZONE_COVERAGE_REGISTER.csv#L254),
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and
[Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

### Checkpoint 343: Tunnels of Payang in-place retention and Surf approaches

Keep active rtun DO NOT MOVE. Its active cross-area links, imported Llzazan/Heavens/Verz1 objects, 17 active Underdark mobile resets to #90305, and C/C++ service/justice hooks are typed dependencies. No current source evidence supports moving its rooms, mobs, or objects. Preserve inactive surf.wld #263253 west to #90380 and #270444 down to #90376 on existing Surf candidate row 97; neither is loaded ingress. This adds two reviewed source-only records: Surf rises to 259 and the curated global total to 4,495; Surf active references remain 145. No migration is cleared. See [RTUN audit](RTUN_AUDIT_REVIEW.md), [coverage row 255](ZONE_COVERAGE_REGISTER.csv#L255), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 344: Village of Refugees in-place retention

Keep active ruins DO NOT MOVE. Both active Surface pairs, the Newhaven collar QST/sidecar, Newbie torch product/reset, Alatorin horse reset, and the source-only Nexus56 waterskin references are typed dependencies. The existing nexus56 SHP has only conditional fallback-reader exposure because this checkout lacks areas/world.shp; this does not establish a live shop. Preserve inactive surf.wld #264438 west to #98600 on existing Surf candidate row 97. It is not loaded ingress and was already counted; curated source-only totals remain 4,495 overall and Surf active references remain 145. No new candidate row or migration clearance is supported. See [Village of Refugees audit](VILLAGE_OF_REFUGEES_AUDIT_REVIEW.md#checkpoint-344-reset-aware-reach-refreshed-source-closure-and-builder-decisions) and [Nexus56 candidate row 79](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79).

### Checkpoint 345: Sarmiz'Duul in-place retention and Surf approaches

Keep active `sarmiz` DO NOT MOVE. Preserve its three reciprocal Surface/Underworld
routes, the two separate one-way inbounds, Heavens/Verzanan reset and shop
imports, the Quietus and Ship Yards QST consumers, Alatorin's potion shop/reset,
and compiled ship, ferry, service, smith, and CHAOS item references. Keep the
four unresolved Sior object reset VNUMs for owner review; do not confuse object
#6018 with the local mobile of the same number.

Preserve inactive surf.wld #277517 south to #9400, #278416 east to #9975, and
#278717 north to #9975 on existing Surf candidate row 97. These three routes
are source-only, not loaded ingress. Surf's active-reference count remains 145;
its reviewed source-only subset rises to 262 and the curated global total to
4,498 (ud_map1 two, ud_map2 20, Underworld2 eight). No migration clearance is
supported. See the [Sarmiz audit](SARMIZ_AUDIT_REVIEW.md), [coverage row
257](ZONE_COVERAGE_REGISTER.csv#L257), and [Surf candidate row
97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 346: Savannah in-place retention

Keep active `savannah` DO NOT MOVE. Preserve the reciprocal Surface boundary,
typed Mitashi inputs and epic recipes, the Plane of Air consumer of cloak
#138515, and compiled CHAOS profile item references. Record the 167-room
`Duris3.wld` VNUM overlap and its 668 identity-ambiguous target edges on the
existing Duris3 candidate row 36; they are not uniquely attributable Savannah
approaches. Savannah adds no source-only ingress and does not change Surf's
145 active references or the 4,498 curated source-only total. No move is
cleared. See the [Savannah audit](SAVANNAH_AUDIT_REVIEW.md), [coverage row
258](ZONE_COVERAGE_REGISTER.csv#L258), and [Duris3 candidate row
36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

### Checkpoint 347: Scorched Valley retention and Surf approach

Keep active `scorchvalley` DO NOT MOVE. Preserve its ordinary reciprocal
Surface pair, separate one-way load-helper boundaries, foreign Torg/Fields
Between head dependencies, and typed Yeenoghu, blocker, artifact, and CHAOS
equipment hooks. Preserve inactive `surf.wld` #280633 west to active room
#71201 on existing Surf candidate row 97; it is source-only, not loaded
ingress. Surf active references stay 145; its reviewed source-only subset
rises to 263 and curated global total to 4,499 (`ud_map1` two, `ud_map2` 20,
Underworld2 eight). No move is cleared. See the [Scorched Valley audit](SCORCHVALLEY_AUDIT_REVIEW.md),
[coverage row 259](ZONE_COVERAGE_REGISTER.csv#L259), and [Surf candidate row
97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 348: Sea in-place retention and candidate identities

Keep active `sea` DO NOT MOVE. Preserve its unresolved #67568 east/south/west
exits as unresolved active-source records while builders decide whether a
modern Surface route is intended. The three reciprocal `surf.wld` returns to
Sea were already included in the checked-in source-only baseline; do not
increase Surf's 263 reviewed records or the 4,499 global total. Preserve the
same-numbered but differently titled `surface2011.wld` Storm Sea records on
their own existing candidate row 98; they do not return to Sea and are not
additional approaches. Keep Surf and Surface2011 in place until candidate
identity, access, and dependency decisions are separately resolved. See the
[Sea audit](SEA_AUDIT_REVIEW.md), [coverage row 260](ZONE_COVERAGE_REGISTER.csv#L260),
[Surf row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and
[Surface2011 row 98](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L98).

### Checkpoint 349: Sea Caves in-place retention and candidate approach

Keep active seacaves DO NOT MOVE. Preserve its reciprocal active Surface
boundary, imported Heavens equipment and WH mobile reset, the compiled altar
and dagger callbacks, and the data-bound mural switch. The all-inactive-WLD
sweep adds one non-overlapping source-only surf.wld approach, #212716 west to
#2200; retain it on existing
Surf candidate row 97 and do not count it as loaded ingress. Surf remains
at 145 active references, its reviewed source-only subset rises to 264, and
the curated total rises to 4,500. No move is cleared. See the [Sea Caves
audit](SEACAVES_AUDIT_REVIEW.md), [coverage row 261](ZONE_COVERAGE_REGISTER.csv#L261),
and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 350: Sea Kingdom in-place retention

Keep active seakngdm DO NOT MOVE. Preserve its reciprocal Plane Water route,
the one-way White Harbor arrival, Alatorin's QST/story item consumers,
Heavens' necklace consumer, Sea Kingdom's Heavens #359 reset import, and the
compiled artifact, equipment, and vault bindings. The inactive-WLD sweep
found no Sea Kingdom approach or room-number overlap, so add no candidate
record. Surf remains at 264 reviewed source-only records and the curated
global total remains 4,500; these counts include the Sea Caves Surf approach
recorded in checkpoint 349. No migration clearance is supported. See the
[Sea Kingdom audit](SEAKNGDM_AUDIT_REVIEW.md), [coverage row
262](ZONE_COVERAGE_REGISTER.csv#L262), and [source dossier](../zone-stories/SEAKNGDM.md).

### Checkpoint 351: Sevenoaks in-place retention and candidate approaches

Keep active `seveno` DO NOT MOVE. Preserve its reciprocal Surface boundary,
White Harbor's one-way arrival into vault room #38632, imported Heavens object
#359 and White Harbor object #55443, Alatorin's active reset/shop use of local
object #38639, and compiled Sevenoaks item/CHAOS equipment references. The
inactive-WLD sweep adds source-only `surf.wld` #209270 north and
`underworld2.wld` #425645 west to #38600. Retain those references on existing
Surf row 97 and Underworld2 row 116; neither is active ingress. Surf now has
265 reviewed source-only records, Underworld2 nine, and the curated global
total is 4,502. No move or migration clearance is supported. See the
[Sevenoaks audit](SEVENO_AUDIT_REVIEW.md),
[coverage row 263](ZONE_COVERAGE_REGISTER.csv#L263), and
[source dossier](../zone-stories/SEVENOAKS.md).

### Checkpoint 352: Arbre's Forest in-place retention and candidate approaches

Keep active `sforest` DO NOT MOVE. Preserve both reciprocal Surface pairs:
#559312 south ↔ #97400 north and #560911 north ↔ #97423 south. The inactive
WLD scan adds two source-only Surf approaches, #208170 south to #97400 and
#209066 east to #97423. Retain both on existing Surf row 97; they are not
active ingress. Surf's reviewed source-only subset rises from 265 to 267,
and curated totals from 4,502 to 4,504, with 145 active references
unchanged. No move or migration clearance is supported. See the
[Arbre's Forest audit](SFOREST_AUDIT_REVIEW.md),
[coverage row 264](ZONE_COVERAGE_REGISTER.csv#L264), and
[source dossier](../zone-stories/SFOREST.md).

### Checkpoint 353: The Great Shaboath in-place retention and dependencies

Keep active shabo DO NOT MOVE. Preserve the one-way ConnectorZones and Winterhaven inbounds, the reciprocal Underdark boundary, all 64 exact-match D reset groups, Alatorin’s reset of local object #32857, and Ravenloft’s reset of #32856. The inactive-WLD sweep found no Shaboath room overlap or source-only approach, so add no candidate record. Surf remains at 267 reviewed source-only records, Underworld2 at nine, and the curated global total at 4,504. No migration clearance is supported. See the [Great Shaboath audit](GREAT_SHABOATH_AUDIT_REVIEW.md), [coverage row 265](ZONE_COVERAGE_REGISTER.csv#L265), and [source dossier](../zone-stories/THE_GREAT_SHABOATH.md).

### Checkpoint 354: Shadamehr Keep in-place retention and external item dependencies

Keep active shadamehr_keep DO NOT MOVE. Preserve the reciprocal Dreggan Woods route; local metal, ornate, and vault key dependencies; Dreggan Woods key #41003; Heavens object #359; Unique object #67219; the Alatorin QST consumption of local cloak #41204; and Barovia's reset of Bloodlust #41203 onto Doru. The Shadamehr O reset for object #41002 remains unresolved because no active object prototype with that VNUM was found. The inactive-WLD scan found no approach or room-number overlap, so add no candidate record. Surf remains at 267 source-only records, Underworld2 at nine, and the curated global total at 4,504. No migration clearance is supported. See the [Shadamehr Keep audit](SHADAMEHR_KEEP_AUDIT_REVIEW.md), [coverage row 266](ZONE_COVERAGE_REGISTER.csv#L266), and [source dossier](../zone-stories/SHADAMEHR_KEEP.md).

### Checkpoint 355: Shadowclave in-place retention and cross-zone dependencies

Keep active shadowcl DO NOT MOVE. Preserve its three reciprocal Surface WLD pairs, the conditional Githzerai arrival in room #93919, the local Shadow Circle badge and potion dependencies used by Malch/Newhaven/Shipy QST, Alatorin/Shipy item resets and stores, the generic fishing-pole code reference, and the local Iliyad potion shop. The inactive-WLD scan adds two source-only Surf approaches to #93900 and #93952; retain both on existing Surf candidate row 97. Surf now has 269 reviewed source-only records and the curated total is 4,506; active Surf references remain 145. No move or migration clearance is supported. See the [Shadowclave audit](SHADOWCL_AUDIT_REVIEW.md), [coverage row 267](ZONE_COVERAGE_REGISTER.csv#L267), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [source dossier](../zone-stories/SHADOWCLAVE.md).

### Checkpoint 356: Shady Grove builder-review reconciliation

Shady Grove remains an active contributor and is not a migration candidate. Keep the three inactive `surf.wld` approach records in the Surf source-only plan, and keep `shady2.mob` separate pending its provenance and owner. Checkpoint 356 closes the coverage review of those recommendations only; it adds no migration authorization or candidate row. See [Shady Grove audit supplement](SHADY_GROVE_AUDIT_REVIEW.md#checkpoint-356-builder-question-coverage-reconciliation).

### Checkpoint 357: Underworld Shafts and ud_map0/1/2 source-only approaches

Keep Shafts in place as an active areas/AREA contributor. The four exits from Shafts into ud_map0/1/2 and ten inactive-map exits back into Shafts are recorded without merging the source families. Checkpoint 357 adds three ud_map0 approaches, four ud_map1 approaches, and three ud_map2 approaches to existing candidate rows 112-114; these are not active ingress. Three correspond to direct reverse edges for active exits; the ud_map2 room 1121446 target has no direct reverse to Shafts room 10012. Preserve each exact WLD file and VNUM until the candidate owner confirms purpose and loading intent. The source-only total is now 4,516 (Surf 269; Underworld2 9 unchanged; ud_map2 reviewed subset 23). No new candidate row or migration authorization is created. See [Shafts audit supplement](SHAFTS_AUDIT_REVIEW.md).

### Checkpoint 358: Shaughin builder-question review

Shaughin remains DO NOT MOVE, with shaughin2 retained independently and its imported trophies, reset parents, portals, and reciprocal boundaries preserved. Checkpoint 358 verifies 20 explicit builder questions and recommendations; it adds no migration candidate or change to the selected route assumptions. Answers and any later native repair require separate builder direction. See [Shaughin audit supplement](SHAUGHIN_AUDIT_REVIEW.md).

### Checkpoint 359: Shaughin2 and the inactive Surf approach

Keep active shaughin2 DO NOT MOVE. Preserve its four reciprocal active boundaries, reset-controlled local secret passage, upper Shaughin trophy and portal consumers, and worker references. Also preserve Alatorin's two active resets of Shaughin2 campfire object #6603 and the Tharnadia1 shop-source listing of charred-skeleton object #6609 under shop #6111; the latter alone does not prove a reachable shop. The inactive Surf source has one recorded approach, #230408 east to #6683. Keep it on existing Surf candidate row 97 as source-only evidence; it is not active ingress. The curated source-only total is now 4,517 (Surf 270; ud_map2 reviewed subset 23; Underworld2 9). No candidate row or migration authorization is added. See the [Shaughin2 audit](SHAUGHIN2_AUDIT_REVIEW.md), [coverage row 271](ZONE_COVERAGE_REGISTER.csv#L271), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 360: Ship runtime pool retention

Keep active `ship` DO NOT MOVE. Preserve the contiguous #60000-#64999 room source with the native ship-system constants: fixed Davy Jones, transit, and ferry rooms; the dynamically allocated #60003-#64999 interior pool; and the associated ship/ferry prototypes. `shipy` remains a distinct active port area. Active and inactive WLD scans found no ship-range ingress or room overlap, so no candidate row or source-only route record is added. This static finding does not qualify runtime exits or travel. See the [ship runtime-area audit](SHIP_AREA_AUDIT_REVIEW.md), [coverage row 272](ZONE_COVERAGE_REGISTER.csv#L272), and [runtime-room dossier](../zone-stories/SHIP_RUNTIME_AREA.md).

### Checkpoint 361: Ship Yards in-place retention and inactive approaches

Keep active `shipy` DO NOT MOVE. Preserve its ten active entry directions, 32 active departures, Malch and Surface reciprocal boundaries, four unresolved map links, exact typed QST/story/reset/shop callers, and the distinct `ship` runtime pool. The inactive Aopal map shares 28 room VNUMs with shipy; keep its four non-overlapping approaches on existing Aopal candidate row 10. Keep eight non-overlapping Surf approaches on existing Surf row 97. The curated source-only total rises by twelve to 4,529, with Surf at 278 reviewed records. These are source references, not active ingress or migration clearance. See the [Ship Yards audit](SHIP_YARDS_AUDIT_REVIEW.md), [Aopal row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L10), [Surf row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [coverage row 273](ZONE_COVERAGE_REGISTER.csv#L273).

### Checkpoint 362: Undead Fog in-place retention

Keep active `shortb` DO NOT MOVE. Preserve both reciprocal Surface exits, the isolated Web room and its local type-25 portal connection, all source reset parentage, the local vampire QST action text, and the unused object VNUM #67806. The all-95 inactive scan adds three non-overlapping Surf approaches; keep them on existing row 97 as source-only. Surf rises to 281 reviewed records and the curated source-only total to 4,532. These edges are not active ingress and do not clear a map move. See the [Undead Fog audit](SHORTB_AUDIT_REVIEW.md), [coverage row 274](ZONE_COVERAGE_REGISTER.csv#L274), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).
### Checkpoint 363: Skulldrach in-place retention and typed consumers

Keep active skulldrach DO NOT MOVE. Preserve its reciprocal Citadel and Surface boundaries, three negative-target exit records, and separate 27-room WLD component. Keep the golem at #12796 and shield #12802 together because active Bastine QST and story data depend on the exact object VNUM. Preserve the non-overlapping source-only Surf edge #273887 north to #12859 on existing candidate row 97. It is not active ingress. Surf rises to 282 reviewed source-only records and the curated total to 4,533. See the [Skulldrach audit](SKULLDRACH_AUDIT_REVIEW.md), [coverage row 276](ZONE_COVERAGE_REGISTER.csv#L276), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 364: Smoke in-place retention and typed dependencies

Keep active `smoke` DO NOT MOVE. Preserve the 153-room authored WLD, the separate portal routes, both keyed vault directions and their #139818 dependency, and the current local quest recipes. Keep the two foreign fixed portal destinations from Plane of Air and Plane of Fire together with their documented Smoke returns. Retain the three Smoke object VNUMs referenced by maintained Chaos starter-kit profiles. Preserve the 310 Duris3 and five forofcon non-overlapping inactive WLD directions on existing candidate rows 36 and 45; retain their colliding source-room records and forofcon reset-role overlaps with their own inactive identities. The 315 source-only directions are not active ingress; the curated total is 4,848 and Surf remains at 282. This review does not clear any map move or qualify live gameplay. See the [Smoke audit](SMOKE_AUDIT_REVIEW.md), [coverage row 277](ZONE_COVERAGE_REGISTER.csv#L277), [Duris3 candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), and [forofcon candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L45).

### Checkpoint 365: Smokeveil in-place retention and external dependencies

Keep active `smokev` DO NOT MOVE. Preserve its two reciprocal Surface boundary pairs, twelve reset-matched state-1 doors, three state-4 directions without resets, and the corrected local shop record #20234. Preserve the single inactive source-only Surf direction #263026 west to #20201 on existing candidate row 97. Keep exact foreign object references grouped by type and source: Alatorin's item command and story record for Tentabeast heart #20253; active Raxthan, Tezcat, and Valdrak G resets for local Smokeveil object prototypes; the Raxthan and Valdrak shop listings; and maintained Chaos, smith, and epic-teacher code users. The unassigned `seas_coral_golem` handler is not active travel. The Surf source edge is not active ingress; the curated total is 4,849 and Surf remains DO NOT MOVE with 283 reviewed source-only records. See the [Smokeveil audit](SMOKEV_AUDIT_REVIEW.md), [coverage row 278](ZONE_COVERAGE_REGISTER.csv#L278), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 366: Snow Ogres in-place retention and typed dependencies

Keep active `snogres` DO NOT MOVE. Its active boundary remains the reciprocal Surface pair plus the one-way Faerie Realm inbound direction. The inactive-WLD sweep found no room overlap or approach, so there is no new candidate ingress and the curated source-only total remains 4,849 (Surf 283).

Preserve the typed active dependencies: Surface's reset of Leppts mobile #87742; Newbie's object #87700 and White Harbor's four shaman-totem consumers; and the two local reset references to active Heavens objects #360 and #1230. Source-only `brass-old-1.zon:815` G-resets Snow Ogres object #87725; this is not active stock. The reference is retained on existing [brass-old-1 candidate row 17](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17) while full candidate closure remains pending. See [the audit supplement](SNOGRES_AUDIT_REVIEW.md) and [coverage row 279](ZONE_COVERAGE_REGISTER.csv#L279).

### Checkpoint 367: Solonar in-place retention and cross-area routes

Keep active `solonar` DO NOT MOVE. There are no active or inactive WLD approaches and no same-type WLD/MOB/OBJ collision. The active Elvish statue object #35804 at Homestead room #35810 fixed-targets Solonar room #30648; local object #30604 routes back to Homestead. Preserve this item travel pair separately from WLD connectivity. Alatorin ZON resets the Solonar mobile #30611 at #83958; the chain has no following filament G command and does not add another local filament source.

The corrected local shop inventory is seven records, #30600–#30606. No external QST item command, typed story-sidecar item, or SHP product reference to a Solonar object was found. The all-95 inactive-WLD census adds no candidate approach, so the curated total remains 4,849 (Surf 283). See [coverage row 280](ZONE_COVERAGE_REGISTER.csv#L280), the [Solonar audit](SOLONAR_AUDIT_REVIEW.md), and the [source dossier](../zone-stories/VAST_HIDDEN_GROVE.md).

### Checkpoint 368: Split Shield in-place retention and typed map references

Keep active `split_shield` DO NOT MOVE. Preserve its three reciprocal Surface pairs, reciprocal shortc boundary, and White Harbor's one-way inbound direction. Two inactive Surf exits enter #10300 and #10334; retain both source-specific approaches on existing candidate row 97, with source-room VNUM collisions against differently titled `surface2011` and `underdark2011` records. Surf rises to 285 reviewed approaches and the curated source-only total to 4,851.

The local zone resets active Heavens objects #215/#376/#386/#398; active Black Shroud resets and shop #74195 reference local objects #10382 and #10340. Preserve object/mobile type identity for #10340. No local QST, sidecar, or SHP source exists and no foreign QST, sidecar item, or fixed type-25 destination was found. See [coverage row 281](ZONE_COVERAGE_REGISTER.csv#L281), the [Split Shield audit](SPLIT_SHIELD_AUDIT_REVIEW.md), and the [new source map](../zone-stories/VILLAGE_OF_SPLIT_SHIELD.md).

### Checkpoint 373: Surface route retention

Keep active surface DO NOT MOVE. Preserve 1,208 inactive source-only approaches to its loaded rooms on existing candidate rows: asunwa #35300 north, dalvik #298 south, jungle #67005 north and #67087 south, plus 602 each from newbiemaps and surface2011-temp. The two large candidate files share the same route tuples but have distinct room titles and remain separate source identities. Their overlapping source VNUMs do not establish active ingress. Keep surface2011’s existing 145 active numeric references and its distinct #200000–#289999 room identity; no replacement or path move is supported by this review. Curated source-only totals rise to 6,059 overall; the surf.wld subset remains 285. See the [Surface audit](SURFACE_AUDIT_REVIEW.md), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-373-surface-connectivity-and-inactive-approaches), and candidate rows 12, 29, 55, 78, 98, and 99.
## Checkpoint 374: SurfaceKeeps in-place retention and typed dependencies

Keep active surfacekeeps DO NOT MOVE. Preserve its 79 inbound/74 outbound active boundary records, 69 reciprocal records, six unresolved Duris3 targets, and the distinct Alatorin type-25 lava-tube route. Retain 1,451 Duris3, five oldmap8, and one ygg inactive source-only WLD approaches on their existing candidate rows; preserve Duris3’s separate 2,645-room same-VNUM map identity, with no exact room-title matches. Keep the unregistered surfacekeeps_objects.obj companion, its 54 repeated object VNUMs (39 text-identical bodies, 15 differing), and stub-loading history on candidate row 100.

The bounded typed review records local QST, sidecar, ZON, shop, procedure, and Chaos-kit imports, plus Alatorin’s QST/sidecar/reset consumers and one inactive brass-old-1 reset. The active SHP inventory is ten shops, corrected from zero in coverage; preserve its external stock identities. Curated source-only WLD rises to 7,516 overall and Surf stays 285. No candidate gets migration clearance. See the [SurfaceKeeps audit](SURFACEKEEPS_AUDIT_REVIEW.md), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-374-surfacekeeps-connectivity-and-inactive-approaches), [title/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-374-surfacekeeps-title-same-type-identity-and-followed-history), and [coverage row 287](ZONE_COVERAGE_REGISTER.csv#L287).


## Checkpoint 375: Surface Minizones in-place retention and typed dependencies

Keep active `surfacemini` DO NOT MOVE. Preserve all 119 rooms, its eleven Surface exits, eight Surface inbounds, four one-way outward WLD boundaries, and the second unmatched inbound to crater #98014. Keep the four active Surface type-25 destinations to #97906, #97952, #98016, and #98017 separate from ordinary WLD travel. The inactive-WLD sweep adds no source-only approach, so the curated total remains 7,516 (Surf 285); no candidate row is added.

Preserve type identity across the 25 mobile/object VNUMs #97900–#97924. The bounded cross-area review records Alatorin and Mntcastl QST references, Mntcastl sidecar references, Alatorin and SurfaceKeeps resets, SurfaceKeeps shop stock, and maintained wand/collar code bindings. The local zero-shop inventory is correct. The inactive `surface2011.obj` portal declarations remain source-only evidence. See the [Surface Minizones audit](SURFACEMINI_AUDIT_REVIEW.md#typed-dependencies), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-375-surface-minizones-connectivity-and-inactive-approaches), [title/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-375-surface-minizones-title-same-type-identity-and-history), and [coverage row 288](ZONE_COVERAGE_REGISTER.csv#L288). No gameplay or migration decision is implied.

## Checkpoint 376: Swamp in-place retention and typed dependencies

Keep active swamp_one **DO NOT MOVE**. Preserve the reciprocal Surface and Swamp Two boundaries, the hidden weeds pocket and its asymmetric reset concealment, and the local key prototypes with their Swamp Two lock references. Swamp Two remains the source owner of its 26 mobile resets for Swamp One prototypes #26003–#26006/#26011 and four object resets for #26011–#26014. Keep Caves of Mt. Skelenak’s optional sphere #26013 exchange and Nightbringer’s maintained #26014 assignment with their existing source owners.

Retain inactive surf.wld #267604 west to #26001 as a source-only direction on existing candidate row 97. The equal VNUM in inactive surface2011.wld has different room text/topology and no edge into Swamp One. Surf now has 286 reviewed source-only directions; the curated total is 7,517. The 145 active Surf numeric references do not change. No active route, guaranteed key availability, gameplay repair, or migration clearance is implied. See the [Swamp One audit](SWAMP_ONE_AUDIT_REVIEW.md), [connectivity checkpoint 376](WORLD_CONNECTIVITY.md#checkpoint-376-swamp-connectivity-and-inactive-approach), and [title/history checkpoint 376](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-376-swamp-title-same-type-identity-and-followed-history).


## Checkpoint 377: Swamp Two in-place retention and typed dependencies

Keep active swamp_two DO NOT MOVE. Preserve its 35-room graph and reciprocal Swamp One boundary, all maze directions and three keyed door pairs, and the #26134 cavern routes pending the builder’s resolution of its sealed-room prose. Preserve Swamp Two’s reset ownership for imported Swamp One mobiles #26003–#26006/#26011 and objects #26011–#26014. Preserve the king’s typed G references to Unique object #67238, Heavens object #359, and White Harbor object #55185. These source objects remain owned by their existing active contributors; #55185 is the OBJ prototype, while White Harbor also defines a distinct MOB with that number.

No inactive-WLD source-only approach, same-type collision, local native QST, story sidecar, or shop source was found. No attic candidate or source-only direction is added; curated source-only WLD remains 7,517 (Surf 286). Preserve the missing-file and “nothing” stub findings as source observations, not migration permission. This review does not qualify live access, guarantee keys or item supply, resolve the cavern contradiction, or clear an archive action. See the [Swamp Two audit](SWAMP_TWO_AUDIT_REVIEW.md), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-377-swamp-two-connectivity-and-cavern-routes), [title/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-377-swamp-two-title-same-type-identity-and-history), and [coverage row 290](ZONE_COVERAGE_REGISTER.csv#L290).
## Checkpoint 378: Tchan source-only approaches and Alterian retention

Keep active tchan DO NOT MOVE and inactive alterian_wilderness RETAIN IN PLACE. The current candidate row 8 still has zero active inbound exits. A bounded all-95 inactive-WLD scan found 20 Alterian directions targeting Tchan rooms: four from non-overlapping source room IDs and sixteen from the nine room VNUMs #4801-#4809 that already collide with active Tchan. The four distinct source-only approaches are #4810 south to #4809, #4815 south to #4807, #4821 west to #4803, and #4822 west to #4804. Preserve the sixteen other records with their different-title Alterian room identities; do not count them as distinct live routes.

The inactive Alterian component also defines mobile #4800 and object #4800 and has two M resets for its mobile plus one G reset for its object. Active Tchan separately defines its own guard mobile #4800 and key object #4800, assigns the active mobile to newbie_guard_east, and has no local reset for the key. Preserve the typed source ownership and existing path layout. No map or prototype move is cleared. The curated source-only WLD total rises from 7,517 to 7,521; Surf remains 286. See the [Tchan audit](TCHAN_AUDIT_REVIEW.md), [candidate row 8](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L8), [coverage row 291](ZONE_COVERAGE_REGISTER.csv#L291), and [Alterian coverage row 359](ZONE_COVERAGE_REGISTER.csv#L359).

## Checkpoint 381: Temple of Laduguer retention and Sylvandawn dependencies

Keep active temple_laduguer **DO NOT MOVE** as a current full-area contributor. Add no Temple candidate row. Its single active WLD boundary with Underdark is reciprocal; this source review establishes no reason to transfer the map or its locally owned prototypes.

Keep unregistered sylvandawn **RETAIN IN PLACE** on existing [candidate row 101](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L101). Preserve its seventeen source-only WLD approaches to Temple rooms #8100–#8202 there. Its other 206 raw directions to the same numeric rooms originate in Sylvandawn room IDs whose titles differ from the active Temple identities; do not count them as active ingress.

The Sylvandawn source also contains eight inactive SHP product references to Temple objects #8103, #8105, #8106, #8108, #8111, #8112, #8116, and #8118, plus 115 source-only M/O/D room arguments in the Temple numeric band. Candidate row 101 already carries unrelated typed consumers of Lizardman Swamps, Orrak, Dragonnia, Kobold Settlement, Plane of Fire, and the Faerie Realm; retain all with the candidate. Do not update its active-inbound or unresolved-active counts, which remain zero. Keep the compiled guard references tied to their source identities and do not reactivate Sylvandawn by resolving number overlap.

The curated source-only WLD total rises from 7,521 to 7,538, with Surf unchanged at 286. No row or source path is added, moved, or cleared for migration. See the [Temple audit](TEMPLE_LADUGUER_AUDIT_REVIEW.md), [connectivity checkpoint 381](WORLD_CONNECTIVITY.md#checkpoint-381-temple-of-laduguer-graph-and-sylvandawn-approaches), [identity/history checkpoint 381](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-381-temple-of-laduguer-title-identity-and-history), and [coverage row 294](ZONE_COVERAGE_REGISTER.csv#L294).

## Checkpoint 382: Tentro retention and Surf dependencies

Keep active tentro2 **DO NOT MOVE** as a current full-area contributor. Add no Tentro candidate row. The source audit establishes no reason to transfer its rooms or locally owned prototypes; its resets continue to depend on ConnectorZones wildlife and the shared Verz1 bank-counter object, whose ownership must remain distinct.

Keep unregistered surf.wld **RETAIN IN PLACE** on existing [candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). Preserve its four source-only approaches to Tentro rooms #69093, #69092, #69080, and #69222 at source rooms #212805, #213704, #214304, and #214744. They are not active ingress, and Surf's active reference counts remain unchanged. The reviewed Surf source-only subset rises from 286 to 290; the curated total rises from 7,538 to 7,542.

No source is cleared or moved for attic migration. The one-way Winterhaven sewer route, item-travel pool entrance, disconnected Salol component, local quest-list mismatch, and shared bank procedure remain builder questions rather than migration findings. See the [Tentro audit](TENTRO2_AUDIT_REVIEW.md), [connectivity checkpoint 382](WORLD_CONNECTIVITY.md#checkpoint-382-tentro-connectivity-and-surf-approaches), [identity checkpoint 382](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-382-tentro-title-vnum-identity-and-followed-history), and [coverage row 295](ZONE_COVERAGE_REGISTER.csv#L295).

## Checkpoint 383: Tezcat retention and existing map candidates

Keep active `tezcat` **DO NOT MOVE** as a current full-area contributor. Add
no Tezcat candidate row. Its reciprocal Surface entrance/return, one-way
Winterhaven arrival, local prototypes, and active external quest consumers
remain attached to their current source owners.

Keep inactive `surf.wld` **RETAIN IN PLACE** on existing
[candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). Preserve its
source-only #265973 east exit to Tezcat #98900. The reviewed Surf subset rises
from 290 to 291; Surf's 145 active numeric references are unchanged.

Keep inactive `underworld2.wld` **RETAIN IN PLACE** on existing
[candidate row 116](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L116). Preserve
its source-only #429194 south exit to Tezcat #98900. Underworld2's reviewed
approach count rises from nine to ten; its active-inbound count remains zero.
Neither inactive record creates active ingress or clears either map for a
move.

The two reviewed approaches raise the curated source-only WLD total from
7,542 to 7,544. No source path, candidate, or native map is moved, copied,
reactivated, or cleared for migration. See the [Tezcat audit](TEZCAT_AUDIT_REVIEW.md),
[connectivity checkpoint 383](WORLD_CONNECTIVITY.md#checkpoint-383-tezcat-connectivity-and-inactive-approaches),
[identity checkpoint 383](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-383-tezcat-title-identity-and-history),
and [coverage row 296](ZONE_COVERAGE_REGISTER.csv#L296).

## Checkpoint 384: Tharnadia retention and Duris3 candidate update

Keep active `tharnadia` **DO NOT MOVE** as a full-area contributor. No
Tharnadia candidate row is added. Preserve its local story, prototype and
code ownership, six active inbound records, five outbound records, item
teleports, and established outside reset/shop/contract consumers at their
current source owners.

Keep inactive `Duris3.wld` **RETAIN IN PLACE** on existing
[candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36). Add the 604
non-overlapping source-only approaches into Tharnadia to its source-map
evidence there. Retain the 1,732 matches beginning in the 584 differently
titled shared room VNUMs as identity evidence, not distinct approaches. The
candidate's 26 active/unresolved outbound target records and its 14 referenced
active area keys are unchanged.

This finding raises the curated source-only WLD total from 7,544 to 8,148;
Surf remains at 291 reviewed source-only directions. No map, prototype,
source path, or candidate is moved, copied, or reactivated, and no area is
cleared for migration. The review does not qualify runtime travel or safety.
See the [Tharnadia audit](THARNADIA_AUDIT_REVIEW.md), [connectivity
checkpoint 384](WORLD_CONNECTIVITY.md#checkpoint-384-tharnadia-connectivity-and-duris3-approaches),
[title/history checkpoint 384](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-384-tharnadia-title-identity-and-history),
and [coverage row 297](ZONE_COVERAGE_REGISTER.csv#L297).

### Checkpoint 385: Tharnadian Ruin in-place retention

Keep active tharnadian_ruin DO NOT MOVE as a full-area contributor. The active
WLD has three reciprocal neighboring boundaries, a dynamic Surface portal
route, and a local type-25 return to Surface; preserve their room and
prototype identities at current owners. Preserve local key #5504's four
Heavens container resets and Caertannad story contract use, Caertannad giver
references to #5506/#5528, and the four local reset placements each importing
Dungeon mobile #93015 and Plane of Earth mobile #23803.

No inactive-WLD overlap or approach was found in the current 95-source sweep.
There is no Tharnadian Ruin attic candidate, no candidate row change, and no
new source-only approach. Retain global source-only totals at 8,148 (Surf 291).
The directed reach and door/reset review records builder questions; it does not
clear a route for player safety or clear the area for migration. See the [audit supplement](THARNADIAN_RUIN_AUDIT_REVIEW.md#checkpoint-385-directed-reachability-reset-state-and-builder-questions),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-385-tharnadian-ruin-directed-reach-and-route-state),
[title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-385-tharnadian-ruin-title-identity-and-history),
and [coverage row 298](ZONE_COVERAGE_REGISTER.csv#L298).

## Checkpoint 386: Tharnadia Rifts and Duris3 retention

Keep active tharnrifts DO NOT MOVE as a current full-area contributor. Preserve
its active boundary directions, imported Caertannad and Minopass reset
ownership, the #78503 type-25 route to Caertannad #78848, and its current
room identity. No Tharnrifts candidate row is added.

Keep inactive Duris3 RETAIN IN PLACE on existing
[candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36). Add the 1,323
source-only Duris3 approaches into Tharnrifts to its evidence, plus the
bb_gh #30320 up->#111255 and circlegh #45417 up->#110072 source-only
directions. Preserve 21,809 Duris3 target matches beginning in shared,
differently titled Tharnrifts room VNUMs as map identity evidence and exclude
them from approach counts. Cabal and rf_gh target matches begin at active-owned
source rooms and are excluded. The candidate's 26 active unresolved target
records and 14 active source-area keys remain unchanged.

The curated source-only WLD total rises from 8,148 to 9,473; Surf remains 291.
No source, map, prototype, or candidate is moved or reactivated, and no area
is cleared for migration. The source review does not qualify runtime travel
or safety. See the [Tharnrifts audit](THARNRIFTS_AUDIT_REVIEW.md),
[connectivity checkpoint 386](WORLD_CONNECTIVITY.md#checkpoint-386-tharnadia-rifts-directed-reach-and-inactive-approaches),
[identity checkpoint 386](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-386-tharnadia-rifts-title-identity-and-history),
and [coverage row 299](ZONE_COVERAGE_REGISTER.csv#L299).

## Checkpoint 387: Thetis and Surf source-map retention

Keep active thetis DO NOT MOVE. Preserve its 100-room internal graph, two
outbound WLD leaves, and unresolved #104 target until their route owners
confirm intent. No Thetis attic candidate is added.

Keep inactive Surf RETAIN IN PLACE on existing
[candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). Add the
source-only #251543 north exit to Thetis beach #38000 to its evidence. It is
not active ingress and does not change the candidate's 145 active unresolved
references or 17 active source-area keys. Curated source-only counts rise
from 9,473 to 9,474 overall and 291 to 292 in the reviewed Surf subset.
No map is moved, reactivated, or cleared for migration. See the
[Thetis audit](THETIS_AUDIT_REVIEW.md#checkpoint-387-active-boundary-refresh-surf-approach-and-builder-questions),
[connectivity checkpoint 387](WORLD_CONNECTIVITY.md#checkpoint-387-thetis-boundary-and-surf-approach-refresh),
[identity checkpoint 387](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-387-thetis-title-map-identity-and-followed-history),
and [coverage row 300](ZONE_COVERAGE_REGISTER.csv#L300).

## Checkpoint 388: Thri and Surf source-map retention

Keep active `thri` DO NOT MOVE; no Thri attic candidate is added. Preserve its
reciprocal Surface WLD boundary and its exact room identity. Keep inactive
Surf RETAIN IN PLACE on existing
[candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), including
#250600 west to Thri #67600 as source-only evidence. The same source VNUM in
Surface2011 has a different title and no Thri exit; retain that map separately
on its existing candidate row. The approach is not loaded ingress and does
not alter Surf's 145 active unresolved references or 17 source-area keys.
Curated source-only WLD totals rise from 9,474 to 9,475 overall and 292 to 293
for Surf. No map is moved, loaded, or cleared for migration. See the
[Thri review](THRI_STORY_REVIEW.md#checkpoint-388-graph-identity-and-typed-dependency-refresh),
[connectivity checkpoint 388](WORLD_CONNECTIVITY.md#checkpoint-388-thri-connectivity-and-inactive-approach),
[identity checkpoint 388](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-388-thri-title-source-identity-and-followed-history),
and [coverage row 301](ZONE_COVERAGE_REGISTER.csv#L301).

### Checkpoint 391: Lost Temple retention and typed dependencies

Keep active tikitt DO NOT MOVE. Preserve its reciprocal Lost City WLD boundary,
the one-way Winterhaven arrival at High Altar #44333, all fifteen City-owned
recipe objects, the Heavens-owned reset object #360, and portal object #44190's
destination #43704. The all-95 inactive-WLD scan found no Temple room overlap
or source-only approach. No candidate row or migration is added. Curated
source-only WLD totals remain 9,475 overall and 293 for Surf. See the
[Temple audit](TIKITT_AUDIT_REVIEW.md), [coverage row 304](ZONE_COVERAGE_REGISTER.csv#L304),
and [City audit](TIKIT_AUDIT_REVIEW.md).

## Checkpoint 392: Torg in-place retention and Surf approach

Keep active `torg` DO NOT MOVE. Preserve its reciprocal Surface boundary,
one-way active arrivals, imported objects, foreign QST/story consumers, and
the legacy `rf_gh` shop identity until their owners and live behavior are
resolved. Retain the inactive room, prototype, reset, and item-portal
collisions as source identity evidence.

The inactive `surf.wld` source-only edge #277211 east to Torg #28900 belongs on
the existing Surf candidate row 97, with exact source lines recorded in that
row. It is not active ingress and creates no Torg candidate or migration.
Curated source-only WLD totals rise to 9,476 overall and 294 for Surf. See the
[Torg audit](TORG_AUDIT_REVIEW.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-392-torg-boundary-and-inactive-approach),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-392-torg-title-map-identity-and-followed-history),
and [coverage row 305](ZONE_COVERAGE_REGISTER.csv#L305).

## Checkpoint 393: Torrhan in-place retention and Surf approaches

Keep active torrhan DO NOT MOVE. Preserve its four reciprocal Surface edges,
the separate administrative Winterhaven entry, and the active typed
dependencies documented in the [Torrhan audit](TORRHAN_AUDIT_REVIEW.md).

Three inactive surf.wld approaches to Torrhan rooms #66689, #66712, and #66692
belong on existing Surf candidate row 97. They are not active ingress and add
no Torrhan candidate or migration. Curated source-only WLD totals rise to
9,479 overall and 297 for Surf. See the
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-393-torrhan-boundaries-and-directed-reach),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-393-torrhan-title-map-identity-and-followed-history),
and [coverage row 306](ZONE_COVERAGE_REGISTER.csv#L306).

## Checkpoint 394: Troll Slave Compound in-place retention

Keep active `torture` DO NOT MOVE. Preserve its 31-room identity, reciprocal
Ghore WLD pair, local reset and hazard declarations, and the distinction
between the open outside boundary and the reset-closed interior gate. The
inactive-WLD sweep found no room overlap or source-only approach, and the
bounded typed scan found no outside reset, quest, shop, story, portal, or
maintained-code reference to move with it. Add no Torture candidate or
migration. Curated source-only totals remain 9,479 overall and 297 for Surf.
See the [Torture audit](TORTURE_AUDIT_REVIEW.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-394-troll-slave-compound-topology-and-ghore-boundary),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-394-troll-slave-compound-title-map-identity-and-followed-history),
and [coverage row 307](ZONE_COVERAGE_REGISTER.csv#L307).

## Checkpoint 396: Tradeskills in-place retention

Keep active Tradeskills DO NOT MOVE. Preserve rooms #400000/#400999 and the
conditional #400220 corpse-portal landing; the all-95 inactive-WLD sweep found
no room overlap or source-only WLD approach. Eight active outside reset files,
registered QSTs, five external shop records, four story sidecars, and direct
compiled bindings use the catalog's item or mobile identities. Keep
`tradeskills.old.obj` separate from the active 293-object file; it is not a
migration source. Add no attic candidate or zone migration. Curated source-only
totals remain 9,479 overall and 297 for Surf. See the
[Tradeskills audit](TRADESKILLS_AUDIT_REVIEW.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-396-tradeskills-utility-hub-and-conditional-portal),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-396-tradeskills-title-catalog-identity-and-followed-history),
and [coverage row 309](ZONE_COVERAGE_REGISTER.csv#L309).

## Checkpoint 397: Trakkia in-place retention

Keep active Trakkia DO NOT MOVE. Retain the two reciprocal Surface pairs,
four currently blocked exit records, conditional switch/key/portal routes,
local reset imports, and the inactive Surf #246406 approach as separately
owned map evidence. The source-only approach is not active ingress. This
builder review adds no attic candidate, migration, or source placement change.
See the [Trakkia audit](TRAKKIA_AUDIT_REVIEW.md),
[access review](WORLD_CONNECTIVITY.md#checkpoint-397-trakkia-access-scenarios-retained),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-397-trakkia-title-and-bounded-history-retained),
and [coverage row 310](ZONE_COVERAGE_REGISTER.csv#L310).

## Checkpoint 398: Traoul in-place retention

Keep active Traoul DO NOT MOVE. Preserve the reciprocal Truk #1622 boundary,
Truk-owned entrance key #1609, local reset-closed gates and secret tunnels,
and the exact relationship between configured keys and placements. Keep
unplaced prototype #7001 as an unresolved source question; do not move the
area or add an attic candidate. No separate inactive WLD approach or room
overlap was found. See the [audit](TRAOUL_AUDIT_REVIEW.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-398-traouls-lair-doors-and-boundary),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-398-traoul-title-and-bounded-history),
and [coverage row 311](ZONE_COVERAGE_REGISTER.csv#L311).

## Checkpoint 399: Tribal Forest retained in place

Keep active Tribal Forest **DO NOT MOVE**. Preserve its three reciprocal
Surface approaches, Astral boundary, active Heavens object #72 fountain import,
local object #42235 procedure, and external CHAOS starter reference to object
#42226. The isolated Force Cage room and one-way WLD junction remain source
questions for the builder; they do not establish an attic candidate or a move
plan. No separate inactive Tribal Forest WLD or replacement area was found.
See the [audit](TRIBAL_FOREST_AUDIT_REVIEW.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-399-tribal-forest-boundaries-and-directed-access),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-399-tribal-forest-title-and-bounded-history),
and [coverage row 312](ZONE_COVERAGE_REGISTER.csv#L312).


## Checkpoint 400: Transparent Tower retained in place

Keep active `trnsptow` **DO NOT MOVE**. Preserve its reciprocal ConnectorZones
drawbridge, one-way Realm arrival, and the newly reconciled source-only Surf
edge as separate map evidence. Keep ConnectorZones' nine Tower equipment
ingredients, four active outside reset suppliers, imported reset objects,
CHAOS kit references, and Luiren's unregistered old-format SHP records
attached to their source owners. No separate predecessor or replacement was
established. See the [audit](TRNSPTOW_AUDIT_REVIEW.md),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-400-transparent-tower-boundaries-and-directed-access),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-400-transparent-tower-title-and-bounded-history),
and [coverage row 313](ZONE_COVERAGE_REGISTER.csv#L313).

## Checkpoint 401: Troll Caves retained in place

Keep active `troll_caves` **DO NOT MOVE**. Retain its two reciprocal
active boundaries, three previously cataloged source-only WLD approaches,
local QST contracts, and externally owned reset prototypes as separate
evidence. No predecessor or replacement was established. The dossier now has
20 source-backed builder questions; no area or gameplay source changed. See
the [audit supplement](TROLL_CAVES_AUDIT_REVIEW.md),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-401-troll-caves-boundaries-and-directed-access),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#troll-caves),
and [coverage row 314](ZONE_COVERAGE_REGISTER.csv#L314).

## Checkpoint 403: Truktakyagyo retained in place

Keep active truk **DO NOT MOVE**. Preserve its three reciprocal active WLD
boundaries, the global world_quest binding on bartender #1603, the QST
AREA/file mismatch, and the dormant brass-old-1 reset reference as separate
evidence. No predecessor or replacement was established; no area or gameplay
source changed. See the [Truktakyagyo audit](TRUK_AUDIT_REVIEW.md),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-403-truktakyagyo-boundaries),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-403-truktakyagyo-title-and-bounded-history),
and [coverage row 316](ZONE_COVERAGE_REGISTER.csv#L316).

## Checkpoint 404: Twin Towers retained in place

Keep active `ttowers` **DO NOT MOVE**. Preserve its seven native Q identities,
four reciprocal active WLD boundaries, three externally owned heart consumers,
computed Jin Touch teacher, and source-defined access/reset behavior with their
existing owners. The source dossier now has 20 explicit builder questions; no
area or gameplay source changed. See the [Twin Towers audit](TWIN_TOWERS_AUDIT_REVIEW.md),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-404-twin-towers-boundaries-and-directed-access),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-404-twin-towers-title-and-bounded-history),
and [coverage row 317](ZONE_COVERAGE_REGISTER.csv#L317).

## Checkpoint 405: Tundra retained in place

Keep active Tundra **DO NOT MOVE**. Preserve its seven unregistered Surf WLD
approaches as source-only, the unusable and inactive-target exits, its external
typed item owners, and distinct local service and quest outcomes. Existing
dynamic access and dependency limits remain. The dossier now has 20 explicit
builder questions; no area or gameplay source changed. See the [Tundra audit](TUNDRA_AUDIT_REVIEW.md),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-405-tundra-boundaries-and-directed-access),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-405-tundra-title-and-bounded-history),
and [coverage row 318](ZONE_COVERAGE_REGISTER.csv#L318).

## Checkpoint 406: Turolopolis Zoo retained in place

Keep active `turolzoo` **DO NOT MOVE**. Preserve its reciprocal Willem
boundary, closed cage route, and the absence of any inactive-WLD approach or
candidate attic row. The Zoo dossier now has 20 explicit builder questions; no
area or gameplay source changed. See the [Zoo audit](TUROLZOO_AUDIT_REVIEW.md),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-406-turolopolis-zoo-boundaries-and-directed-access),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-406-turolopolis-zoo-title-and-bounded-history),
and [coverage row 319](ZONE_COVERAGE_REGISTER.csv#L319).

## Checkpoint 407: Twin Towers Forest retained in place

Keep active `twin_towers_forest` **DO NOT MOVE**. Preserve its active Surface
and Twin Towers boundaries, the additional one-way load-room entrances, and
its reset ownership of the New Cave city wizard and four local object #15119
rows. No inactive-WLD approach or candidate row was found. The dossier now has
20 explicit builder questions; the gardener challenge and any-order flower
achievement terms remain as the owner approved them. No area or gameplay source
changed. See the [forest dossier](../zone-stories/TWIN_TOWERS_FOREST.md),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-407-twin-towers-forest-boundaries-and-directed-access),
[title review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-407-twin-towers-forest-title-scan),
and [coverage row 320](ZONE_COVERAGE_REGISTER.csv#L320).

## Checkpoint 408: A Dark and Twisted Wood retained in place

Keep active `twstwd` **DO NOT MOVE**. Preserve the four reciprocal active
boundaries, two Pine Hollow load-room inputs, five inactive Surf approaches,
and the faerie's two Pine Hollow item owners as separate evidence. The
conditional SHIFT PRIME route is not universal ingress. No candidate row is
added. The dossier now has 20 explicit builder questions; no area or gameplay
source changed. See the [Twisted Wood audit](DARK_AND_TWISTED_WOOD_AUDIT_REVIEW.md),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-408-a-dark-and-twisted-wood-boundaries-and-directed-access),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-408-a-dark-and-twisted-wood-title-and-bounded-history),
and [coverage row 321](ZONE_COVERAGE_REGISTER.csv#L321).

## Checkpoint 409: udmini retained in place

Keep active `udmini` **DO NOT MOVE**. Preserve both authored WLD components,
the cave-only active arrivals, the unplaced self-targeting mushroom object,
the two fall fields, and the crack's fall-risk Kvarkpass target. Keep Myconid
and Newbie's spores exchanges and Newbie's reset source under their own
outside owners. No inactive-WLD approach or migration candidate was found;
the static evidence does not establish mushroom access, live stock, or safe
fall/return behavior. See the [UDMINI audit](UDMINI_AUDIT_REVIEW.md),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-409-udmini-boundaries-and-directed-access),
[title/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-409-udmini-title-and-bounded-history),
and [coverage row 322](ZONE_COVERAGE_REGISTER.csv#L322).

## Checkpoint 410: Ugta retained in place

Keep active Ugta **DO NOT MOVE**. Preserve its reciprocal Surface boundary,
the four isolated source rooms, gate and prison-door reset states, and external
Heavens/Verz1 object owners. Keep the `surf.wld` exit to Ugta as source-only
evidence in the existing Surf retention note; it adds no active ingress or
Ugta migration candidate. The source review does not prove lawful entry, safe
river crossing, or a prisoner-release route. See the [Ugta review](UGTA_STORY_REVIEW.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-410-ugta-boundaries-and-directed-access),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-410-ugta-title-and-bounded-history),
and [coverage row 323](ZONE_COVERAGE_REGISTER.csv#L323).

## Checkpoint 411: Durian Underdark and 2011 candidate retained

Keep active `underdark` **DO NOT MOVE**. Preserve its sparse physical map,
101 reciprocal active boundary pairs, external seal consumer, dynamic portal,
and selected worm/relic/source dependencies. Keep `underdark2011` **RETAIN**:
11 active WLD records from five owners target its unregistered VNUMs, and
#217609 has an unresolved identity across `surf`, `surface2011`, and
`underdark2011`. The absence of approaches into current `underdark` does not
resolve that candidate's dependency, ownership, or tooling closure. See the
[candidate review](DURIAN_UNDERDARK_AUDIT_REVIEW.md#inactive-wld-sweep-and-the-underdark-2011-candidate),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-411-durian-underdark-loader-qualified-graph),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-411-durian-underdark-title-and-bounded-history),
and [coverage row 324](ZONE_COVERAGE_REGISTER.csv#L324).

## Checkpoint 412: Undermountain retained in place

Keep active `undermountain` **DO NOT MOVE**. Preserve its exact native key and
note sources, locked grate and alternate access evidence, hidden object
flags, intentionally rare monster distribution, and foreign recipe/reset
owners. No inactive-WLD approach or candidate row was found for this active
area. Keep controlled source or gameplay changes separate from this review.
See the [Undermountain audit](UNDERMOUNTAIN_AUDIT_REVIEW.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-412-undermountain-boundaries-and-directed-access),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-412-undermountain-title-and-bounded-history),
and [coverage row 325](ZONE_COVERAGE_REGISTER.csv#L325).

## Checkpoint 413: Underworld retained in place

Keep active `underworld` **DO NOT MOVE** and keep active `underworld3` as a
separate contributor despite their shared normalized title. Preserve the three
source-only `ud_map0`/`ud_map2` approaches, all active and candidate reset
references, and the four active inbound-only boundary records. Keep `ud_map0`,
`ud_map1`, `ud_map2`, `surf`, and `surface2011` **RETAIN** pending map ownership
and dependency/tool closure. No source move or merge is cleared. See the
[Underworld audit](UNDERWORLD_AUDIT_REVIEW.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-413-underworld-boundaries-and-directed-access),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-413-underworld-title-and-bounded-history),
and [coverage row 326](ZONE_COVERAGE_REGISTER.csv#L326).

## Checkpoint 414: Underworld3 retained in place

Keep active `underworld3` **DO NOT MOVE**, independently of active `underworld`
despite their shared normalized title. Preserve the authored boulder-door
resets, paired portal definitions and placements, and the active inbound-only
`mobs_underdark` boundary to #30149. The one-way edge is active source evidence,
not an unregistered-WLD approach. Do not merge or relocate either contributor
on this review. See the [Underworld3 questions](../zone-stories/UNDERWORLD3.md#explicit-builder-questions-and-recommended-resolutions),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-414-underworld3-boundaries-and-directed-access),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-414-underworld3-title-and-bounded-history),
and [coverage row 327](ZONE_COVERAGE_REGISTER.csv#L327).

## Checkpoint 415: Unique Vault retained in place

Keep active `unique` **DO NOT MOVE** as the owner of its object prototypes.
Preserve the three documented external reset consumers: Astral Main's
#67206, New Cave city's #67272, and Swamp Two's #67238. Do not duplicate those
objects into the consumer areas or mistake their `G` references for local
Unique placement. No move or archive is cleared. See the [Unique builder
questions](../zone-stories/UNIQUE.md#explicit-builder-questions-and-recommended-resolutions),
[Astral Main consumer](../../../areas/zon/astral_main.zon#L45),
[New Cave city consumer](../../../areas/zon/new_cavecity.zon#L172),
[Swamp Two consumer](../../../areas/zon/swamp_two.zon#L59), and
[coverage row 328](ZONE_COVERAGE_REGISTER.csv#L328).

## Checkpoint 416: Undead Outpost retained in place

Keep active `unoutpst` **DO NOT MOVE**. Preserve its paired Surface and Crypt
boundaries, the closed gate/well/forest-path resets, and the dropped negative
north exit as source evidence. Keep the external object reset #132581
unassigned until its prototype owner is identified; do not create a local
copy. This review clears no source move or archive. See the [Undead Outpost
builder questions](../zone-stories/UNDEAD_OUTPOST.md#explicit-builder-questions-and-recommended-resolutions),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-416-undead-outpost-boundaries-and-directed-access),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-416-undead-outpost-title-and-bounded-history),
and [coverage row 329](ZONE_COVERAGE_REGISTER.csv#L329).

## Checkpoint 417: Vargan II retained in place

Keep active `v2` **DO NOT MOVE**. The repository-wide scan found no inactive
WLD approach into rooms #7800–#7918 and no external typed prototype dependency
requiring a new attic record. Preserve the reciprocal connection to the
separate active `vargan` area; neither the builder questions nor the shared
name clear a move, merge, or archive. See the [Vargan II builder
questions](../zone-stories/V2_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[retention/history audit](V2_AUDIT_REVIEW.md#title-history-and-disposition),
[boundary](../../../areas/wld/v2.wld#L15), and
[coverage row 330](ZONE_COVERAGE_REGISTER.csv#L330).

## Checkpoint 418: Valoisian Castle retained in place

Keep active `val` **DO NOT MOVE**. Preserve the reciprocal active Surface
boundary and keep the separate `surf.wld` exit to #38575 classified as
source-only because that WLD is not in the active index. Keep local dagger
#38421 with its active owner while retaining the Verspin reset consumer; do
not duplicate or reassign it. No move or archive is cleared. See the
[Valoisian Castle builder questions](../zone-stories/VALOISIAN_CASTLE_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[typed-reference and retention audit](VALOISIAN_CASTLE_AUDIT_REVIEW.md#bounded-typed-and-code-references),
[active boundary](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L547), and
[coverage row 331](ZONE_COVERAGE_REGISTER.csv#L331).

## Checkpoint 419: Phantasmagoric Caverns retained in place

Keep active `valdrak` **DO NOT MOVE**. Preserve its two reciprocal UnderDark
connections and keep the unregistered `ud_map2.wld` approach with that
source rather than treating it as active ingress. Retain Raxthan's use of
Valdrak objects, the Smokev/Kastle shop imports, the compiled room #53122
route, and the CHAOS-kit object reference under their established owners.
No move, merge, or archive is cleared. See the [Phantasmagoric Caverns
builder questions](../zone-stories/VALDRAK_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[dependency and retention audit](VALDRAK_AUDIT_REVIEW.md#typed-references-and-compiled-numeric-matches),
[source-only approach](../../../areas/wld/ud_map2.wld#L66333), and
[coverage row 332](ZONE_COVERAGE_REGISTER.csv#L332).

## Checkpoint 420: Valinhaven retained in place

Keep active `valinhav` **DO NOT MOVE**. Preserve its connected 159-room map,
both reciprocal active boundaries, and the authored local doors and reset
states under their current owner. The QST directory membership does not create
a local contract, and the source inventory establishes no shop or story
sidecar. No move, merge, or archive is cleared. See the [Valinhaven builder
questions](../zone-stories/VALINHAV_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[river boundary](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1116),
[cave boundary](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1117), and
[coverage row 333](ZONE_COVERAGE_REGISTER.csv#L333).

## Checkpoint 421: Valley of Crushk retained with its foreign source

Keep active `valley_crushk` **DO NOT MOVE**. Preserve the 85-room map, the
Mini Zones and Pharr Valley inbounds, the reciprocal Pharr Valley route, and
the one-way Surface target as distinct authored edges. Keep Ship Yards object
#43143 and its bandit/reset source with Ship Yards while retaining the Valley's
exact QST consumer. The bounded inactive-WLD review does not make an inactive
approach live ingress. No move, merge, or archive is cleared. See the [Valley
of Crushk builder questions](../zone-stories/VALLEY_CRUSHK_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[route review](VALLEY_CRUSHK_AUDIT_REVIEW.md#active-map-incoming-leaves-and-exits),
[typed-source review](VALLEY_CRUSHK_AUDIT_REVIEW.md#typed-item-reset-and-shop-dependencies),
[Ship Yards shiv](../../../areas/obj/shipy.obj#L607), and
[coverage row 334](ZONE_COVERAGE_REGISTER.csv#L334).

## Checkpoint 422: Vargan retained with source-only Surface approach

Keep active `vargan` **DO NOT MOVE**. Preserve its three reciprocal Surface and
Vargan II boundaries and retain the inactive `surf.wld` approach as source-only,
not live ingress. Keep the `surf`/`surface2011` candidate retained until its
owners, map-generation path, and remaining dependencies are resolved. Preserve
Norkon's exact local recipe and the distinct item owner/contract under their
current sources. No move, merge, or archive is cleared. See the [Vargan builder
questions](../zone-stories/VARGAN_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[boundary and retention review](VARGAN_AUDIT_REVIEW.md#inactive-wld-sweep-and-source-only-approach),
[source-only approach](../../../areas/wld/surf.wld#L1391665), and
[coverage row 335](ZONE_COVERAGE_REGISTER.csv#L335).

## Checkpoint 423: Vecna's Tomb retained with its mapped dependencies

Keep active `vecna` **DO NOT MOVE**. Preserve its Surface reciprocal pair, the
Realm inbound leaf, the registered Heavens and Negative Material Plane reset
imports, and the inactive `Duris3.wld` room matches as distinct source records.
Do not promote the inactive map records to live ingress or treat Vargan's
neighboring map ownership as a replacement. No move, merge, or archive is
cleared. See the [Vecna builder questions](../zone-stories/VECNAS_TOMB_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[Checkpoint 143 dependency and retention review](VECNAS_TOMB_AUDIT_REVIEW.md#typed-quest-reset-shop-and-sidecar-references),
[inactive-map review](VECNAS_TOMB_AUDIT_REVIEW.md#inactive-map-matches), and
[coverage row 336](ZONE_COVERAGE_REGISTER.csv#L336).

## Checkpoint 424: Vehicles retained with transport and story dependencies

Keep active `vehicles` **DO NOT MOVE**. Preserve its WLD, MOB, OBJ, ZON, QST,
and SHP sources together, including the seven ferry definitions whose route
paths and ticket handling are implemented outside the area files. Keep the
two commented external mobile resets typed as external until their owners are
confirmed, retain all four local shop records, and leave the Kjelgar crystal
VNUM mismatch unresolved. The local story and link reviews do not establish
operational route availability, a complete external dependency closure, or
current shop/item stock. No move, merge, or archive is cleared. See the
[Vehicles builder questions](../zone-stories/VEHICLES_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[source story review](VEHICLES_STORY_REVIEW.md),
[ferry definitions](../../../src/world/ferryact.c#L140), and
[coverage row 337](ZONE_COVERAGE_REGISTER.csv#L337).

## Checkpoint 425: Verspin retained with its story and world dependencies

Keep active `verspin` **DO NOT MOVE**. Preserve its 200 rooms, native story and
service contracts, five SHP records, and local item/mobile/reset sources as a
single active contributor. Retain the foreign Mntcastl gem source, Bloodstone
sigil source, Surface highway entrance, and one-way WH dispersal as typed
dependencies; preserve the literal Shinjin and crew-hall room assignments
pending their separate service-qualification reviews. The source dossier and
sidecar do not prove route safety, current shop stock, paid settlement, or
successful crew/stat operations. No move, merge, or archive is cleared. See the
[Verspin builder questions](../zone-stories/VERSPIN_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[source story map](../zone-stories/VERSPIN.md),
[foreign gem reset](../../../areas/zon/mntcastl.zon#L381),
[foreign sigil reset](../../../areas/zon/bs.zon#L2008), and
[coverage row 338](ZONE_COVERAGE_REGISTER.csv#L338).

## Checkpoint 426: Verzanan preserved as an active dependency source

Keep active Verz1 DO NOT MOVE. It has only two reciprocal placeholder WLD
rooms and no local reset placements, but other sources consume 60 locally
owned object prototypes: 202 full-area ZON object resets, 109 full-area SHP
product references, and two QST item grants. Another 105 ZON and 108 SHP
references are in stems outside the current full-area list and must remain in
the owner review. Preserve all VNUM owners, the distinct Verspin identity,
and the active #3097 storage/balance hook. No attic candidate-specific move
plan is warranted for a current full-area contributor, and no move, merge, or
archive is cleared. See the
[Verzanan source review](../zone-stories/VERZANAN_STORY_REVIEW.md#typed-external-dependency-closure),
[builder questions](../zone-stories/VERZANAN_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[#3097 local hook](../../../src/specs/specs.assign.c#L1307),
[Lava consumer](../../../areas/zon/lava.zon#L105), and
[coverage row 339](ZONE_COVERAGE_REGISTER.csv#L339).


## Checkpoint 427: Voluntown retained with imported object dependencies

Keep active Voluntown **DO NOT MOVE**; its current `areas/AREA` entry is at
line 404. Preserve the reciprocal IceCrag boundary and the locally authored
room, mobile, object, reset, and quest sources as a distinct Mausoleum owner.
Its Haratius reset group imports Heavens object #359 (`epic_stone`) and Pits
of Cerberus object #22070 (`revenant_helm`); preserve both prototypes and
handlers under their existing owners. Also retain the 24 C/C++ equipment-table
references to eight local objects as inbound consumers, not as ownership
changes. The non-overlapping Duris3 map exits remain source-only approaches
under the existing Duris3 candidate review; same-numbered Duris3 room records
remain separate overlapping source data. No Voluntown candidate-specific
migration plan, move, merge, or archive is warranted for this active
contributor. See the [Voluntown audit](ROYAL_MAUSOLEUM_OF_CASTLE_ICECRAG_AUDIT_REVIEW.md#typed-references-and-external-producers),
[active manifest row](../../../areas/AREA#L404),
[Heavens import](../../../areas/zon/Voluntown.zon#L152),
[Pits import](../../../areas/zon/Voluntown.zon#L153), and
[coverage row 340](ZONE_COVERAGE_REGISTER.csv#L340).


## Checkpoint 428: Crystalspyre retained with separate map approaches

Keep active Crystalspyre Mountains (`vrzawild`) **DO NOT MOVE**. Preserve the
reciprocal Surface boundary and the local room/mobile/object/reset group,
including Zikata's imported Heavens object #358 and its `epic_stone` callback.
The inactive `surf.wld` approach from #210777 to #37090 remains source-only
under the existing Surf candidate review; do not present it as live ingress
or a proven return route. This adds one reviewed Surf record: curated
source-only totals rise to 1,506 overall and 123 for Surf. The planned
Verzanan relationship in the local slab and Bloodguard text does not transfer
ownership or establish a move/merge decision. No Crystalspyre candidate-
specific migration plan is warranted for this current full-area contributor.
See the [Crystalspyre audit](CRYSTALSPYRE_MOUNTAINS_AUDIT_REVIEW.md#active-and-inactive-approaches),
[active manifest row](../../../areas/AREA#L179),
[Heavens import](../../../areas/zon/vrzawild.zon#L120),
[source-only Surf exit](../../../areas/wld/surf.wld#L226153), and
[coverage row 341](ZONE_COVERAGE_REGISTER.csv#L341).

## Checkpoint 429: Wander retained with Llzazan and Arachdrathos dependencies

Keep active Wander **DO NOT MOVE**. Preserve its 36 one-way WLD exits into
Llzazan, the two reset imports of Arachdrathos mobile #36011, and the Llzazan
rothe-item references #36204/#36205. Six unregistered ud_map0.zon resets
consume active Wander mobiles; preserve those source-only references under
existing candidate row 112. Keep ud_map0 **RETAIN IN PLACE** pending map-owner
and tooling closure. The inactive-WLD sweep found no approaches to Wander and
the curated source-only totals stay 1,506 overall and 123 for Surf. No
Wander-specific migration candidate or clearance is warranted. See the
[Wander audit](WANDER_AUDIT_REVIEW.md#typed-dependencies-and-external-consumers),
[ud_map0 candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L112),
[active manifest entry](../../../areas/AREA#L176),
[Llzazan object import](../../../areas/zon/wander.zon#L29), and
[coverage row 342](ZONE_COVERAGE_REGISTER.csv#L342).

## Checkpoint 430: Wellvolen retained with typed quest and map dependencies

Keep active Wemic **DO NOT MOVE**. Preserve its Surface reciprocal boundary,
WH mob-dispersal reference, local prototype/reset group, and Shipyard’s exact
use of wicked-mace object #93501. Keep inactive surf.wld **RETAIN IN PLACE**
under existing candidate row 97 and preserve room #229296 east to Wemic
#93501 as a source-only approach, not active ingress. Curated source-only WLD
totals rise from 1,506 to 1,507 overall and from 123 to 124 for Surf. No
Wemic-specific candidate row or migration clearance is warranted. See the
[Wellvolen audit](WEMIC_AUDIT_REVIEW.md#safe-disposition),
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97),
[inactive Surf exit](../../../areas/wld/surf.wld#L614548), and
[coverage row 343](ZONE_COVERAGE_REGISTER.csv#L343).

## Checkpoint 432: Winterhaven retained with typed ownership references

Keep active `wh` **DO NOT MOVE**. Its source-comprehensive story set, custom
handlers, five active map arrivals, 60 outbound exits, and foreign reset/QST/SHP
references remain attached to current owners. Preserve the 29 distinct
nonlocal-object references in `wh.zon`, including ambiguous object definitions
#53/#100 and unresolved #6067, without assuming runtime loader choice. Keep the
unregistered `brass-old-1` and `brass-old-3` references under existing candidate
rows 17 and 19: `brass-old-1.zon` G-resets Winterhaven objects #55550-#55554 and
#55150, while the old Brass QST/SHP records refer to #55271/#55150; `brass-old-3.qst`
also has an exact item command for #55271. These source-only records do not
become active resets, contracts, or stock. No new candidate, source move, or
migration clearance is supported. See the [Winterhaven audit](WINTERHAVEN_AUDIT_REVIEW.md#typed-dependencies-and-exact-source-closure),
[candidate row 17](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17),
[candidate row 19](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L19), and
[coverage row 345](ZONE_COVERAGE_REGISTER.csv#L345).

## Checkpoint 433: Wildland Trails retained with unresolved legacy targets

Keep active `wildland_trails` **DO NOT MOVE**. Preserve the two local WLD exits
to #215745 and #217245 as unresolved: each VNUM occurs in both inactive Surf
and Surface2011, with different room identities, and the normal build omits
both sources. Keep Surf's two source-only reverse approaches on existing
candidate row 97 and Surface2011's matching room identity on row 98. No new
candidate, source move, alias decision, or migration clearance is supported.
The 99-room area has no local QST, SHP, or story sidecar and no exact active
external typed reset, QST, shop, sidecar, or same-type prototype references in
the bounded scan. The local key, closed/secret routes, and custom combat
procedures remain source evidence; access and recovery are not qualified. See
the [Wildland Trails audit](WILDLAND_TRAILS_AUDIT_REVIEW.md#safe-disposition),
[candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97),
[candidate row 98](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L98), and
[coverage row 346](ZONE_COVERAGE_REGISTER.csv#L346).

## Checkpoint 434: Turolopolis retained with source-only approach

Keep active `willem` **DO NOT MOVE**. Preserve Surf's source-only north
approach from room #236469 to Willem room #7102 under existing candidate row
97; it is not active ingress and does not support moving or merging either
area. Retain the current Surface Lothrell reset, shared `shift_prime` room
#7117 dependency, and unregistered Nexus56 book reset/shop references under
their source owners. No new candidate, source move, or migration clearance is
supported. Static source findings do not qualify arrival, stock, custody,
safe access, or return. See the [Turolopolis audit](TUROLOPOLIS_AUDIT_REVIEW.md#active-title-and-tracked-history-boundary),
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and
[coverage row 347](ZONE_COVERAGE_REGISTER.csv#L347).

## Checkpoint 435: Shadowfall retained with mapped portal dependencies

Keep active `wolfen` **DO NOT MOVE**. Its one reciprocal Surface WLD boundary,
two reset cave rifts, and campsite-to-wagon portal remain owned by the active
area. The directed WLD graph alone has separate tunnel and main-map
components; the three current type-25 object destinations are static route
evidence, not permission to move or merge area sources. No inactive inbound
WLD reference or new migration candidate was found. Keep the current Surface
boundary and active area in place. Live access, shop stock, disease behavior,
and safe return remain unqualified. See the [Shadowfall audit](WOLFEN_AUDIT_REVIEW.md#active-wld-boundary-and-local-graph),
[coverage row 348](ZONE_COVERAGE_REGISTER.csv#L348), and
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-435-shadowfall-directed-graph-surface-pair-and-portal-routes).

## Checkpoint 436: Woodseer remains active

Keep active woodseer **DO NOT MOVE** as a current, un-commented areas/AREA
contributor. Its two inactive Surf approaches remain source-only, and the
unregistered Duris3 target at #154581 is not an active connection. Preserve the
active source owner while those map roles and the open builder decisions remain
separate. See the [Woodseer audit](WOODSEER_AUDIT_REVIEW.md#active-wld-graph),
[coverage row 349](ZONE_COVERAGE_REGISTER.csv#L349), and [builder questions](../zone-stories/WOODSEER_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).

## Checkpoint 437: Worms remains active

Keep active worms **DO NOT MOVE** as a current areas/AREA contributor. Preserve
its distinct ownership from the Lair of the Purple Worm; the ud_map2 path remains
source-only and does not clear or change a candidate disposition. See the
[Worms audit](WORMS_AUDIT_REVIEW.md#title-and-tracked-history),
[coverage row 350](ZONE_COVERAGE_REGISTER.csv#L350), and
[builder questions](../zone-stories/THE_CAVERNS_OF_THE_WORMS_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).

## Checkpoint 438: The Temple to Skrentherlog remains active

Keep active yuan_ti **DO NOT MOVE** as a current areas/AREA contributor. Its
Winterhaven inbound and reciprocal Village boundary remain separate authored
connections; neither changes ownership of the neighboring yuan_ti2 area. See
the [Temple audit](TEMPLE_TO_SKRENTHERLOG_AUDIT_REVIEW.md#active-wld-graph),
[coverage row 351](ZONE_COVERAGE_REGISTER.csv#L351), and
[builder questions](../zone-stories/THE_TEMPLE_TO_SKRENTHERLOG_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).

## Checkpoint 439: Village of the Damned remains active

Keep active yuan_ti2 **DO NOT MOVE**. Preserve the reciprocal active Surface
and Temple boundaries, and keep the Surf row 97 and oldmap8 row 81 approaches
with their own source maps as **RETAIN IN PLACE**, not active ingress. The
neighboring yuan_ti area remains a separate owner. See the
[Yuan Ti2 audit](YUAN_TI2_AUDIT_REVIEW.md#source-only-inactive-wld-approaches),
[coverage row 352](ZONE_COVERAGE_REGISTER.csv#L352), and
[builder questions](../zone-stories/YUAN_TI2_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).

## Checkpoint 440: Aeon and Genesis retain their current sources

Keep the existing `aeon_gh` candidate row 2 **RETAIN IN PLACE** and preserve
`genesis` as its separate existing candidate row 46. Their unregistered maps
share zone #299, rooms #29900–#29909, and mobiles #29900/#29901; compiled code
also attaches the Players Guild `inn` handler to shared room #29903. Their
authored identities and reset contents differ, so neither is established as a
replacement for the other. Aeon's three Player Castles imports, fifteen
merchant equipment/shop product IDs (including duplicate definitions and
currently undeclared #6089), and editor/persisted state remain owner questions.
Do not move, merge, or clear either candidate until range and code ownership
and the remaining reference closure are resolved. See the [Aeon audit](AEON_GH_AUDIT_REVIEW.md#disposition),
[VNUM review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-440-aeon-order-guildhall-and-genesis-vnum-collision),
[Aeon candidate row 2](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L2) and
[Genesis candidate row 46](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L46),
and [Aeon coverage row 353](ZONE_COVERAGE_REGISTER.csv#L353).

## Checkpoint 441: Afterlife Guildhall retains its current candidate

Keep existing candidate `afterlife_gh` row 3 **RETAIN IN PLACE**. Preserve its
map editor JSON and local lore QST along with the ten room VNUMs that collide
with active Torg and the Players Guild `inn` binding at #29103. Its source
resets import four Player Castles objects and four objects owned by the two
separate Afterlife OP candidates; the shop references fourteen products,
including duplicate IDs and unresolved #6089. These overlaps and source-only
dependencies do not clear the candidate to move or merge. Resolve room/code
ownership and remaining editor, persisted-state, and runtime closure before
any separate owner decision. See the [Afterlife audit](AFTERLIFE_GH_AUDIT_REVIEW.md#disposition),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-441-afterlife-guildhall-and-active-torg-identity),
[candidate row 3](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L3), and
[coverage row 354](ZONE_COVERAGE_REGISTER.csv#L354).

## Checkpoint 442: Afterlife OP retains its source identity

Keep existing candidate `afterlife_op` row 4 **RETAIN IN PLACE**. Preserve
the three-room WLD, its tracked empty MOB file, both type-25 object portals
and their encoded targets, and `areas/map/afterlife_op.json`. Its ZON depends
on Player Castles object #11053 and mobile prototypes #29101–#29104 whose
matching definitions occur in the separate `afterlife_gh` source; the
Guildhall ZON also imports OP object #29111. The three room IDs collide with
active Torg and unregistered `void_gh`, while the stored portal targets #29106
and #29110 have competing room owners. Resolve room and item target ownership,
cross-candidate prototype ownership, the inactive Duris3 boundary, and
editor/persisted/runtime closure before any separate owner decision. See the
[Afterlife OP audit](AFTERLIFE_OP_AUDIT_REVIEW.md#disposition),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-442-afterlife-op-and-vnum-ownership-conflicts),
[candidate row 4](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L4), and
[coverage row 355](ZONE_COVERAGE_REGISTER.csv#L355).


## Checkpoint 443: Afterlife OP2 retains its current source

Keep existing candidate afterlife_op2 row 5 **RETAIN IN PLACE**. Preserve its
three disconnected maps, nine room VNUMs colliding with active Torg, editor
map JSON, tracked empty MOB, seven type-25 objects and their stored targets,
six Player Castles object resets, twelve mobile reset commands, and three
local portal resets. Its two D4 exits target unlisted Duris3 rooms with no
direct return; the Guildhall ZON also imports three OP2 objects, and OP2
mobile IDs have competing candidate prototype definitions. Resolve room
ownership, both source boundaries, every portal target, mobile ownership,
and editor/persisted/runtime closure before any separate move or merge
decision. See the [OP2 audit](AFTERLIFE_OP2_AUDIT_REVIEW.md#disposition),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-443-afterlife-gc-op2-title-history-and-room-ownership),
[builder questions](../zone-stories/AFTERLIFE_GC_OP2_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[candidate row 5](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L5), and
[coverage row 356](ZONE_COVERAGE_REGISTER.csv#L356).

## Checkpoint 444: Alatorin object bank retains its source

Keep candidate alatorin_objects row 6 **RETAIN IN PLACE**. Preserve all 408
object records and their variants, the separate active alatorin.obj copies,
the 168 distinct Nexus56 prototype collisions, and the 2010 loader-stub
history tied to existing player-held Alatorin objects. Preserve active
Alatorin's reset, quest, shop, story, and compiled-code references. The
active #83434 portal copy targets #83188 while this source-only copy targets
#83831. Resolve saved-object identity, object-version ownership, typed item
uses, and editor/persisted state before any separate removal, merge, or path
move decision. See the [object-bank audit](ALATORIN_OBJECTS_AUDIT_REVIEW.md#connectivity-identity-and-disposition),
[builder questions](../zone-stories/ALATORIN_OBJECTS_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-444-alatorin-object-bank-history-and-overlap),
[candidate row 6](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L6), and
[coverage row 357](ZONE_COVERAGE_REGISTER.csv#L357).

## Checkpoint 445: Alterian Mountains retains distinct source records

Keep candidate `alterian_mountains` row 7 **RETAIN IN PLACE**. Preserve its
80-room map, 18 mobile prototypes, object #4900, and 23 reset commands.
Reconcile the 49 distinct room records shared numerically with active Mining
and the Mountain `nothing` object #4900 versus Mining's reset wooden crate
before any co-load, merge, or path move. Preserve the source-only links to
Alterian Wilderness, Mountain's resets that use Wilderness prototypes, the
30 Wilderness resets that use Mountain mobile prototypes, and the compiled
#4830 `wanderer` callback. Keep both maps source-only until loader ownership
and external tool/editor state are settled. See the [Mountain audit](ALTERIAN_MOUNTAINS_AUDIT_REVIEW.md#disposition),
[builder questions](../zone-stories/ALTERIAN_MOUNTAINS_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[duplicate/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-445-alterian-mountains-and-mining-identity),
[candidate row 7](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L7), and
[coverage row 358](ZONE_COVERAGE_REGISTER.csv#L358).

## Checkpoint 446: Alterian Wilderness remains a separate source

Keep candidate `alterian_wilderness` row 8 **RETAIN IN PLACE**. Preserve its
130-room map, 17 mobiles, 15 objects, three-component WLD graph, and source
story. Reconcile its 29 different room records shared numerically with active
Tchan and Mining, plus same-type mobile/object #4800 collisions with Tchan.
Preserve five Mining, two Shadow Forest, and four unique-origin Tchan
source-only approaches and keep the 16 overlapping-origin Tchan matches
distinct. The reciprocal Mountain and Surf source links and cross-sibling
reset references must stay attached to their source identities.

The dependency census also records active `bs.zon` imports of object #4814,
the compiled `poison` callback for mobile #4812, `magic_pool` callbacks for
objects #4801/#4802, and the active Tchan #4800 guard callback. Keep all
source-only data in place until loader ownership, the conflicting room and
prototype identities, and external editor/persisted state are resolved. See
the [Wilderness audit](ALTERIAN_WILDERNESS_AUDIT_REVIEW.md#disposition),
[builder questions](../zone-stories/ALTERIAN_WILDERNESS_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-446-alterian-wilderness-map-and-prototype-identities),
[candidate row 8](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L8), and
[coverage row 359](ZONE_COVERAGE_REGISTER.csv#L359).

## Checkpoint 447: AoD GH remains a separate source

Keep candidate `aod_gh` row 9 **RETAIN IN PLACE**. Preserve the 24-room,
three-component map, the one `.shp` record, the 13 mobile resets and 16 G
resets, and all seven Player Castles imports. Resolve the same-number
Genocide Hall, Legion, and `rf_op` room/prototype identities; the three #295
zone-header competitors; the #29502 Players Guild callback; and the
source-only exits to Duris3 #141442 and the competing Surf/Surface2011 #210208
records. Confirm the intended reset/shop stock, including missing OBJ #6089,
before any loader edit or path move.

Keep this source **RETAIN IN PLACE** until the owner resolves map and prototype
ownership, the intended loader file set, external editor aliases, persisted
instances, and runtime travel/return. No active WLD inbound edge or migration
clearance is established. See the [AoD audit](AOD_GH_AUDIT_REVIEW.md#disposition),
[builder questions](../zone-stories/AOD_GH_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-447-aod-gh-map-and-zone-identities),
[candidate row 9](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L9), and
[coverage row 360](ZONE_COVERAGE_REGISTER.csv#L360).

## Checkpoint 448: Aopal retained with source-only approaches

Keep candidate `aopal` row 10 **RETAIN IN PLACE**. Preserve the 44-room
Tharnadian Causeway, its 28 different room records shared numerically with
active Ship Yards, six already documented source-only approaches, and the
ambiguous #230412 identity in Surf and Surface2011. Keep active inbound count
at zero: the 30 WLD target-number matches into the #43301–#43344 range resolve
to active Ship Yards room identities, and none targets an Aopal-only room.

Preserve the 37 mobile resets, including the unresolved #6116 reference and
two source-only resets of active Heavens mobile #445; the 12 matching door
resets; Samara and the dress reset; and the compiled patrol handler attached
to numeric room #43341. These references do not show that Aopal is loaded or
that its service is currently available. Resolve map ownership, reset
imports, the patrol binding, and external editor/persisted/runtime state
before any separate owner decision about moving these files.

The full inactive-WLD recount in Checkpoint 448 records 11,987 eligible
source-only direction records (415 Surf) under an explicit target-map
identity rule. This replaces the conflicting historical cumulative snapshots
for current reporting; it does not create migration clearance. See the
[Aopal audit](AOPAL_AUDIT_REVIEW.md#safe-disposition),
[builder questions](../zone-stories/AOPAL_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[connectivity recount](WORLD_CONNECTIVITY.md#checkpoint-448-aopal-source-graph-and-current-inactive-wld-recount),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-448-aopal-tharnadian-causeway-title-history-and-room-identities),
[candidate row 10](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L10), and
[coverage row 361](ZONE_COVERAGE_REGISTER.csv#L361).

## Checkpoint 449: Astral retained in place

Keep candidate `astral` row 11 **RETAIN IN PLACE**. Preserve its blank room
#1, empty MOB/OBJ files, reset-free ZON, and tracked `areas/map/astral.json`
until the map owner identifies the placeholder's purpose and editor
ownership. Room #1 is the reserved Limbo VNUM and active Limbo is its sole
active WLD owner; fourteen source-only and nine active numeric approaches to
#1 do not establish an Astral route. Do not infer deletion or replacement from
the shared ZON title or identical blank records in `mtncastl` and `ud-inside`.
Resolve loader intent, editor project, persisted instances, and live runtime
state before any separate owner decision about path or archive changes. See
the [Astral audit](ASTRAL_AUDIT_REVIEW.md#safe-disposition),
[builder questions](../zone-stories/ASTRAL_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-449-astral-placeholder-and-room-1-identities),
[candidate row 11](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L11), and
[coverage row 362](ZONE_COVERAGE_REGISTER.csv#L362).

## Checkpoint 450: Asunwa retained with mixed manifest dependencies

Keep candidate `asunwa` row 12 **RETAIN IN PLACE**. Preserve the commented
full-area entry, active QST manifest source, standalone shop record and
loader glob fallback, Balgar's source contract, the one-way Surface exit,
source-only Surf approach, hazard and door-reset pairings, and the separate
`ddguild`/`player_guilds` SHP references to mobile #35300, object #35301, and
room #35303. Keep the empty Player Guilds QST key #35320 distinct as well.
The shop/QST aggregates are absent from this checkout and no runtime was
booted, so the source evidence does not qualify live loading or object stock.
Resolve the manifest intent, shop/QST ownership, map/editor state, and
persisted/live identity before any separate decision to move or archive these
files. See the [Asunwa audit](ASUNWA_AUDIT_REVIEW.md#title-history-and-safe-disposition),
[builder questions](../zone-stories/ASUNWA_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-450-asunwa-title-history-and-source-identities),
[candidate row 12](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L12), and
[coverage row 363](ZONE_COVERAGE_REGISTER.csv#L363).

## Checkpoint 451: Bastard retained with room and shop collisions

Keep candidate `bastard` row 13 **RETAIN IN PLACE**. Preserve the five
#30200–#30204 WLD records that overlap different active Underworld3 rooms
and different unregistered Sunhall rooms; the three-way mobile/object/shop
number collisions; the up exit to source-only `ud_map2` #1120721; and the
Player Castles object imports #11060, #11110, and #11013. Keep the same-key
#30201 shop records for Ilixctthu and Sunhall's Ahash separate. Resolve the
missing current object prototypes #6084, #6089, and #6094 and the area/shop
ownership before a move or aggregate change. Neither the source-only `ud_map2`
edge nor the active Underworld3 numeric owner establishes live Bastard access.
See the [Bastard audit](BASTARD_AUDIT_REVIEW.md#title-history-and-safe-disposition),
[builder questions](../zone-stories/BASTARD_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-451-bastard-die-hall-room-ownership-and-history),
[candidate row 13](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L13),
[Sunhall candidate row 96](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L96),
[ud_map2 candidate row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114), and
[coverage row 364](ZONE_COVERAGE_REGISTER.csv#L364).

## Checkpoint 452: BB GH retained with portal and shop collisions

Keep candidate bb_gh row 14 RETAIN IN PLACE. Preserve its 44 source rooms,
five disconnected WLD components, four type-25 portal pairs, and four
boundary directions, including the reciprocal but identity-ambiguous
#30300/ud_map2 #1126542 source pair and the already-counted inactive approach
to active tharnrifts #111255. Resolve room, mobile, and object overlaps with
unregistered vodgh and venom; keep their separate #30301 shop records apart.
Retain the six Player Castles object imports and the imported Heavens/Verz1
gear set. Resolve missing object #6089 and the numeric #30303 inn callback
with their owners. The review does not authorize a manifest edit, archive,
or source-path move. See the
[BB GH audit](BB_GH_AUDIT_REVIEW.md#title-history-and-disposition),
[builder questions](../zone-stories/BB_GH_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[candidate row 14](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L14),
[vodgh row 118](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L118),
[venom row 117](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L117), and
[coverage row 365](ZONE_COVERAGE_REGISTER.csv#L365).

## Checkpoint 453: BGTK retained pending shop-loader review

Keep BGTK candidate row 16 RETAIN IN PLACE. Although it is not in areas/AREA,
the current non-mini loader falls back to areas/shp/*.shp because
areas/world.shp is absent, so bgtk.shp enters full-mode shop initialization.
Its three records reference five active Barrow object prototypes and six
missing products. Keeper mobile prototypes #20615 and #20647 are absent;
the active #20607 keeper is reset in a different room from its configured
shop location. Static code inspection finds that the missing real_mobile
lookups can reach an unguarded negative mob_index access. Resolve the source
owner and loader/keeper issue before a full-mode start or source-path change;
no runtime outcome is claimed. See the
[BGTK audit](BGTK_AUDIT_REVIEW.md#default-full-mode-loader-path-and-risk),
[builder questions](BGTK_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[Barrow audit](ELFTOMB_AUDIT_REVIEW.md#checkpoint-453-bgtk-shop-loader-reconciliation),
[candidate row 16](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L16), and
[coverage row 366](ZONE_COVERAGE_REGISTER.csv#L367).


## Checkpoint 454: Brass Old 1 retained with source and loader dependencies

Keep candidate `brass-old-1` row 17 RETAIN IN PLACE. The 2012 and 2025 Git/AREA history and the −100 VNUM crosswalk establish it as the archived predecessor to active Brass. The old #139100 south boundary maps to the active #139000/Fire #25455 reciprocal pair and is not another candidate approach. Preserve the 66 source-only reset commands, the QST reference to Winterhaven #55271, and the old SHP products as references rather than active stock. The current non-mini fallback reads all 18 old shop records; 17 keepers lack active mobile prototypes, creating a static unguarded negative-index risk. Resolve loader ownership, persisted VNUM identity, and editor/tooling before any separate path decision. The 21 owner questions and full reference list are in the [Brass Old 1 audit](BRASS_OLD_1_AUDIT_REVIEW.md) and [builder questions](BRASS_OLD_1_BUILDER_QUESTIONS.md).

## Checkpoint 455: Brass Old 2 retained with route, reset, and code dependencies

Keep candidate `brass-old-2` row 18 RETAIN IN PLACE. Checkpoint 455 identifies it as the byte-preserved former `plane_fire_one *250-251` WLD/MOB/OBJ/ZON source; the current active `plane_fire_one *254-255` is a distinct content family. Preserve all seven active WLD references to Old 2 treasure room #25087 from Potions and Realm. Its 333 ZON resets include 66 source-only references to `brass-old-3`, two current active object VNUMs, and two M resets with no current mobile prototype. Current code also names Old 2 mobiles #25000, object #25080, and rooms #25001/#25086. Reconcile the QST index's old range and resolve the source-code, route, owner, editor, and persisted-data questions before any path decision. See the [Brass Old 2 audit](BRASS_OLD_2_AUDIT_REVIEW.md), [builder questions](BRASS_OLD_2_BUILDER_QUESTIONS.md), and [candidate row 18](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L18).

### Checkpoint 456: Brass Old 3 retained pending source and quest closure

Keep candidate row 19 brass-old-3 RETAIN IN PLACE. Git history confirms
that its five source files are the byte-preserved former plane_fire_two
*252-253 family, while the current QST index still names the old stem. The map
has one outbound source-only edge to active Fire and one one-way source-only
edge into Old 2; its ZON also has 30 Old 2 resets, while Old 2 has 66 Old 3
reset imports. Active Bahamut, Celestia, and Tiamat source files still name
Old 3 objects that are missing from the active catalog. The Yodono QST has a
receive/reward mismatch against its dialogue, and code retains exact mobile,
object, and room references.

Before any future path proposal, ask the area and quest owners to settle the
old QST index and canonical Yodono contract, then obtain decisions from the
active reset owners, map owners, and code owner. Close editor/tooling and
persisted-VNUM identity as well. This audit authorizes no source move,
manifest repair, reset edit, or gameplay change. See the
[Old 3 audit](BRASS_OLD_3_AUDIT_REVIEW.md),
[builder questions](BRASS_OLD_3_BUILDER_QUESTIONS.md),
[candidate row 19](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L19), and
[coverage row 370](ZONE_COVERAGE_REGISTER.csv#L370).

### Checkpoint 457: BSGHOP retained pending identity and content review

Keep candidate row 20 BSGHOP RETAIN IN PLACE. Its four rooms and four mobiles #29440–#29443 and
object #29440 reuse active Harrow VNUMs for different room, mobile, and object records. An
unregistered darkop map repeats the BSGHOP room text, and its portal object has different numeric
fields; the shared text does not prove replacement. BSGHOP's nine authored WLD directions include
one negative target, and its zone header/room fields say 1 while the VNUMs belong numerically to
zone 294.

The BSGHOP reset commands place its overlapping object and mobile identifiers at room #29440.
The active Tundra item contract for #29440 and C/C++ item-slot references to #29443 currently
resolve to Harrow's painting and yo-yo. The child-death horror story and the player-specific board
also require owner review; this audit records neither names nor contact details. Confirm VNUM
ownership, canonical map and portal, reset and loader intent, content handling, editor/tooling, and
persisted identity before a separate path proposal. See the
[BSGHOP audit](BSGHOP_AUDIT_REVIEW.md),
[builder questions](BSGHOP_BUILDER_QUESTIONS.md),
[candidate row 20](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L20), and
[coverage row 371](ZONE_COVERAGE_REGISTER.csv#L371).

### Checkpoint 458: Cabal Guildhall retained pending shop and identity review

Keep candidate row 21 `cabal` RETAIN IN PLACE. Its 13 rooms #29000–#29012 and three mobiles
#29000–#29002 overlap active Torg room and mobile identities with different meanings. Its two
outbound exits start from room VNUMs owned by active Torg; one targets active Tharn Rifts #111079
and one targets unregistered Duris3 #156242. Neither is active ingress into Cabal.

The source contains a Cabal citadel, portal nexus, meditation room, and separate three-room outpost,
with no native QST or story sidecar. Preserve its six Player Castles object reset imports as
source-only typed references. Because the checked-in `areas/world.shp` is absent, the non-mini loader fallback
can read `cabal.shp`; the keeper #29001 and shop room #29003 resolve numerically to active Torg
identities, and stock #29000/#29001/#6089 lacks current active OBJ prototypes. The similarly titled
`cabal_hall` and active `cbl_hall` remain distinct identities absent owner confirmation. Resolve the
shop, map, reset, title, editor/tooling, and persisted-VNUM questions before any separate path
proposal. See the [Cabal Guildhall audit](CABAL_GUILDHALL_AUDIT_REVIEW.md),
[24 builder questions](CABAL_GUILDHALL_BUILDER_QUESTIONS.md),
[candidate row 21](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L21), and
[coverage row 372](ZONE_COVERAGE_REGISTER.csv#L372).

### Checkpoint 459: Cabal Hall retained pending identity and content review

Keep candidate row 22 `cabal_hall` RETAIN IN PLACE. Its ten rooms #30100–#30109 are active Underworld3
room identities with different descriptions. Inactive `nah` and `shadow_hall` maps also reuse VNUMs in
this range. The compiled inn binding at #30104 and the Cabal Hall shop-room value #30103 share
numbers with Underworld3 rooms; neither establishes which unregistered source identity was intended.

Cabal Hall has 19 authored WLD directions: 18 local and one up exit from #30100 to inactive Duris3
#153434. All ten rooms are reachable from #30100. The target has no return exit into Cabal Hall, and
this source-only edge adds no active ingress or eligible approach. The ZON preserves three external
Player Castles object imports and 13 equipment references. The SHP has one record whose #30101 keeper
has no active mobile prototype and whose #30103 room number resolves to active Underworld3 if the
loader path is used. Its stock includes active gathering bags #491–#494 added in the September 2026
source update; #6089 has no active OBJ prototype.

The story includes drug-dealing, sexualized and pornographic material, a homophobic slur, and explicit
anti-LGBTQ threats of violence. Ask the owner to review that content alongside the similar `cabal`,
`nah`, and `shadow_hall` source identities. No area or code source changed. See the
[Cabal Hall audit](CABAL_HALL_AUDIT_REVIEW.md), [30 builder questions](CABAL_HALL_BUILDER_QUESTIONS.md),
[candidate row 22](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L22), and
[coverage row 373](ZONE_COVERAGE_REGISTER.csv#L373).

### Checkpoint 460: casino retained pending Hall VNUM and reset review

Keep candidate row 23 `casino` RETAIN IN PLACE. Active Hall of the Ancients owns all 15 Casino room
numbers #77801–#77815 with different room identities and zone field 777. Casino has 38 authored
directions that numerically resolve to Hall's rooms, while Hall has 23 of its own authored exits
into the same number range. Neither set establishes active Casino access.

Preserve the source-only #45 imports in Casino rooms #77803 and #77804; active Heavens object #45 is
a slot machine. The two Casino door reset comments both say closed, but their raw D state values are
2 and 0. Confirm the intended room mapping, reset and door contract, gambling behavior, editor data,
and persisted identities before considering any manifest or path proposal. See the
[Casino audit](CASINO_AUDIT_REVIEW.md),
[24 builder questions](CASINO_BUILDER_QUESTIONS.md),
[Hall story map](../zone-stories/THE_HALL_OF_THE_ANCIENTS.md),
[candidate row 23](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L23), and
[coverage row 374](ZONE_COVERAGE_REGISTER.csv#L374).

### Checkpoint 461: CBL Outpost 2 retained pending source and VNUM review

Keep candidate row 24 `cbl_outpost2` RETAIN IN PLACE beside unregistered `Monsteri` (row 72) and
active `cbl_hall`. The unregistered maps share title, room/mobile IDs #1000–#1004/#1000–#1003, and
four matching M resets. Active CBL Hall remaps the five room descriptions to #29820–#29824, so the
content relationship does not prove which source is canonical or whether one replaced another.

Active Heavens defines mobile VNUMs #1000–#1003 for a differently named character and three
peasants, while `cbl_outpost2` uses those IDs for four golems. The candidate also has five rooms
outside the current VNUM/100 band and eight source exits from those range-mismatched rooms. Confirm
source lineage, Heavens mobile ownership, legacy zone numbering, the four reset placements, Portal
Cave intent, editor/tooling, and persisted references before any separate path or activation proposal.
See the [CBL Outpost 2 audit](CBL_OUTPOST2_AUDIT_REVIEW.md),
[24 builder questions](CBL_OUTPOST2_BUILDER_QUESTIONS.md),
[candidate row 24](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L24), and
[coverage row 375](ZONE_COVERAGE_REGISTER.csv#L375).

### Checkpoint 462: CBR Hall retained pending Newbie identity and shop review

Keep candidate row 25 `cbr_hall` RETAIN IN PLACE. Active Newbie owns all ten room VNUMs #29300–#29309
with different room identities. It also owns mobile #29300–#29303 and object #29300 with different
prototypes. Compiled code assigns the inn at #29305 and Ailvio handlers at #29303/#29304; these
current numbers resolve to Newbie's secret magic shop and Ailvio characters, not the candidate's
Y.M.C.A. and Merzec.

Preserve the one-way source exit from #29300 to inactive Duris3 #152260, the two Player Castles
object imports, and the 2026 Heavens bag additions. The non-mini shop fallback can include
`cbr_hall.shp`; if consumed, its keeper, room, and product #29300 resolve to Newbie identities. This
is static path and VNUM evidence, not a shop-operation finding. Resolve all room, mobile, object,
code, shop, tooling, and persisted-identity questions before any separate path or activation
proposal. See the [BattleRager Hall audit](CBR_HALL_AUDIT_REVIEW.md),
[28 builder questions](CBR_HALL_BUILDER_QUESTIONS.md),
[candidate row 25](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L25), and
[coverage row 376](ZONE_COVERAGE_REGISTER.csv#L376).

### Checkpoint 463: Yggdrasil Outpost retained pending shared-content and portal review

Keep candidate row 26 `circle_op` RETAIN IN PLACE. Its three room blocks,
four mobile prototypes, and two object prototypes are exact content copies of
the corresponding records in separate unregistered `circlegh`; same-numbered
blocks do not decide which source owns the map. Both WLD paths are present at
the 2009 area-tree move, while followed history shows no explicit replacement.

Preserve the five authored exits, including #45410 D4/up to inactive Duris3
#154462, and the raw zone value 1 for the #454 VNUM band until its numbering
convention is confirmed. The object #45410 destination #45407 exists in
`circlegh.wld`, outside `circle_op.wld`; object #45411 targets #45412 while
its text names “Goodie Cont.” Preserve the Player Castles magic-mouth object
#11052 reset into #45410 and the local root portal #45410 reset into #45412.
Resolve the shared room/prototype records, outpost/guildhall relationship,
portal text and targets, reset intent, dynamic routes, editor/tooling, and
persisted identities before any separate move or activation proposal. See the
[`circle_op` audit](CIRCLE_OP_AUDIT_REVIEW.md),
[27 builder questions](CIRCLE_OP_BUILDER_QUESTIONS.md),
[candidate row 26](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L26), and
[coverage row 377](ZONE_COVERAGE_REGISTER.csv#L377).

### Checkpoint 464: Circle Guildhall retained pending shop and boundary review

Keep candidate row 27 `circlegh` RETAIN IN PLACE. It has 16 rooms and three
separate local components. Its #45410–#45412 rooms, #45410–#45413 mobiles, and
#45410/#45411 objects exactly match the smaller unregistered `circle_op`
source. Neither set of files is in the full area manifest, and the shared
content does not select a canonical copy.

Preserve the negative #45400 D4 target, the one-way source exit from #45410 to
inactive Duris3 #154462, and the #45417 D4 exit to active Tharnrifts #110072,
which is already included in the Checkpoint 386 source-only recount. Retain
the seven Player Castles object imports and four local portal resets. Resolve
the #45411 “Goodie Cont” / #45412 target and #45416 “Ice Crag” / #45415 target
descriptions with the builder. The SHP record configures keeper and room
#45400, but the non-roaming squirrel is reset in #45408; the non-mini SHP
fallback can include this source while its keeper and room are absent from the
full loader. Reconcile the zone/range values, #45418 max-VNUM issue, shop
inclusion, active Heavens/Verz1 stock identities, editor/tooling, dynamic
routes, and persisted records before a separate path or activation proposal.
See the [`circlegh` audit](CIRCLEGH_AUDIT_REVIEW.md),
[30 builder questions](CIRCLEGH_BUILDER_QUESTIONS.md),
[candidate row 27](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L27), and
[coverage row 378](ZONE_COVERAGE_REGISTER.csv#L378).

### Checkpoint 465: `cript` empty SHP retained pending owner decision

Keep candidate row 28 `cript` RETAIN IN PLACE. Its only tracked source is a
zero-byte `areas/shp/cript.shp`, with no shop records, VNUMs, WLD/MOB/OBJ/ZON
files, QST, or title. Git history reaches the 2005 initial revision, passes
through Players paths, and enters the area tree in 2009, but does not identify
an area or replacement.

The non-mini shop fallback can encounter the empty file through its wildcard
when the aggregate `areas/world.shp` cannot be opened; the zero-byte source
contributes no record. Preserve it until its source, editor, and deployment
owners confirm whether it is a placeholder or accidental truncation and
whether external data depends on the path. See the [`cript` audit](CRIPT_AUDIT_REVIEW.md),
[10 builder questions](CRIPT_BUILDER_QUESTIONS.md),
[candidate row 28](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L28), and
[coverage row 379](ZONE_COVERAGE_REGISTER.csv#L379).

### Checkpoint 466: Dalvik retained pending owner review

Keep candidate row 29 `dalvik` **RETAIN IN PLACE**. Its low room range overlaps
50 active WLD room VNUMs; all 63 object numbers overlap current Limbo or Heavens
objects with different full records, and mobile #1–#3 differ from active
Limbo. Preserve each source identity. The active Thetis #38095 north → #104
exit remains unresolved between Dalvik and Mavulsk, while Dalvik #298 south →
Surface #635660 is a previously counted source-only approach.

The ZON also imports active Heavens object #72 and active Tharnadia #132581.
Its 420 QA contracts reference active Tradeskills objects #400000–#400209, but
Dalvik is absent from both area manifests and its QST has no established
aggregate loader path. No SHP file is present. Keep the files until owners
resolve map and prototype collisions, the Thetis destination, reset imports,
quest ownership and packaging, Surface-port association, editor/deployment
references, and persisted identity. No move or activation is proposed. See the
[Dalvik audit](DALVIK_AUDIT_REVIEW.md),
[28 builder questions](DALVIK_BUILDER_QUESTIONS.md),
[candidate row 29](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L29), and
[coverage row 380](ZONE_COVERAGE_REGISTER.csv#L380).

### Checkpoint 467: Dark retained pending owner review

Keep candidate row 30 `dark` **RETAIN IN PLACE**. Its ten rooms, three mobiles,
and two objects reuse active Harrow VNUMs with different complete records;
preserve Harrow as the current active identity. The 36 same-number local exit
targets are not Dark ingress. Preserve the separate up exit to inactive
Duris3 #156726 as unresolved.

The single Dark shop record #29401 references keeper #29401 and room #29408,
which resolve to different active Harrow identities if the non-mini SHP
fallback reads the candidate file. Its key also occurs in `infernal_gh.shp`;
product #29401 maps to Harrow’s big bag, #6089 has no active OBJ definition,
and the other products map to Heavens or Verz1. Preserve all eight external
ZON object imports and the source-only Player Castles/Harrow ownership. Resolve
the wildcard shop contract, both #29401 records, product and reset intent,
editorial text, map owners, tools, and persisted references before any separate
path or activation proposal. See the [Dark audit](DARK_AUDIT_REVIEW.md),
[30 builder questions](DARK_BUILDER_QUESTIONS.md),
[candidate row 30](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L30), and
[coverage row 381](ZONE_COVERAGE_REGISTER.csv#L381).

### Checkpoint 468: The Dark Guildhall retained pending identity decisions

Keep candidate row 31, `dark_gh`, **RETAIN IN PLACE**. It is not listed in
`areas/AREA` or `areas/qst/AREA`, and its 22-room source map has no QST or story
sidecar. Do not infer that the similar `dark` title is an alias or replacement.

Preserve the 21 active Newbie room-number overlaps and every distinct Dark room
block; also preserve the three conflicting mobile and seven conflicting object
prototype IDs. The 50 active Newbie WLD target records remain Newbie routes to
Newbie records. Keep Dark's four one-way source boundaries unresolved: three
target numbers are absent from the active table, while #123619 resolves to
Surface Keeps and also has an inactive Duris3 definition. The #29230 source
room is itself shared with active Newbie, so this does not establish a unique
Dark approach.

Retain all 21 M, 18 O, and 15 G source resets, including the eleven Player
Castles object imports (#11051, #11101, and #11013), seven local portal
placements, and the product with no tracked object definition (#6089). Resolve
the 39 typed Newbie ZON references and the overlapping Newbie QST records as
Newbie-owned consumers. The non-mini shop fallback can glob `dark_gh.shp` when
`areas/world.shp` is absent; its #29201 record collides with active
`newbie.shp` #29201, and the candidate keeper/room IDs map to different active
Newbie identities. Preserve this conditional loader risk, all stock identities,
and the shared shop response text until the shop, map, content, loader, tools,
and persisted-data owners review them. See the [Dark Guildhall audit](DARK_GH_AUDIT_REVIEW.md),
[32 builder questions](DARK_GH_BUILDER_QUESTIONS.md),
[candidate row 31](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L31), and
[coverage row 382](ZONE_COVERAGE_REGISTER.csv#L382).

### Checkpoint 469: Dark Outpost retained pending owner review

Keep candidate row 32, `darkop`, **RETAIN IN PLACE**. It is unlisted in
`areas/AREA` and `areas/qst/AREA` and has no QST, SHP, or story sidecar.
Preserve all 19 room, 18 mobile, and ten object IDs that overlap active Harrow
with different blocks. The 44 active Harrow directions to those room numbers
belong to Harrow; they are not Dark Outpost ingress. Retain the four one-way
outward directions and their competing Duris3/Oldmap8 target identities until
each map owner confirms them. Keep the WLD zone-field value 1 discrepancy
against the #294xx VNUMs visible for builder review.

Preserve all 20 M and ten O source resets, including five Player Castles
`11059` imports and five local placements. Resolve the five `dark.zon`
resets that share Dark Outpost portal object VNUMs #29410 and #29412–#29415;
current active resolution is to Harrow objects, while both source components
are unregistered. Preserve the separate #29440 portal prose shared with BSGHOP
and the active Harrow painting owner. Keep active Harrow QST, Tundra, and
compiled consumers on Harrow identities. Resolve map, reset, content, editor,
deployment, and persisted-data ownership before any separate path or activation
proposal. See the [Dark Outpost audit](DARKOP_AUDIT_REVIEW.md),
[32 builder questions](DARKOP_BUILDER_QUESTIONS.md),
[candidate row 32](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L32), and
[coverage row 383](ZONE_COVERAGE_REGISTER.csv#L383).

### Checkpoint 470: `ddguild` retained pending shop and loader review

Keep `ddguild` RETAIN IN PLACE. Its one shop record points to Asunwa keeper
#35300, object #35301, and room #35303, while Asunwa is not in full `areas/AREA`
and its source resets do not place that keeper in #35303. A second unregistered
`player_guilds.shp` record uses the same keeper-room pair with a different
catalog. Since `areas/world.shp` is absent, the non-mini loader fallback can
read both source files; the absent full-area keeper lookup and unguarded shop
assignment create a static boot-safety risk if that path is used. Preserve both
files and all literal fields until source owners resolve loader, map, stock,
content, tooling, and persisted-state questions. See the [audit](DDGUILD_AUDIT_REVIEW.md),
[18 builder questions](DDGUILD_BUILDER_QUESTIONS.md), [candidate row 33](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L33),
and [coverage row 384](ZONE_COVERAGE_REGISTER.csv#L384).

### Checkpoint 471: empty `drag` files retained pending owner confirmation

Keep zero-byte `drag.qst` and `drag.shp` RETAIN IN PLACE. The tracked path
history includes `Players/Declined/...` and `Players/justice_list` but supplies
no title, VNUM, quest, shop, or map content. The non-mini SHP fallback may open
the empty file, which contributes no record. Confirm whether either path is an
intentional placeholder and check editor, package, archive, and persisted
filename dependencies before any delete or move proposal. See the
[audit](DRAG_AUDIT_REVIEW.md), [11 builder questions](DRAG_BUILDER_QUESTIONS.md),
[candidate row 34](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L34), and
[coverage row 385](ZONE_COVERAGE_REGISTER.csv#L385).

### Checkpoint 472: `dranquest` retained pending questline and placement review

Keep `dranquest` RETAIN IN PLACE. The candidate has two reciprocal WLD rooms,
five mobiles, seventeen objects, one QST record, and no ZON resets. Its Balor
QST asks for Joukarth Forgath's head, while active Heavens QST text has
Rozkadrath helping Joukarth pursue Blood War dominance. The source relation and
chronology are unresolved. Preserve all candidate identities and the reset
empty ZON until the map, quest, content, code, tooling, and data owners resolve
placement, target handling, legacy room-number checks, and any future map/quest
move. See the [audit](DRANQUEST_AUDIT_REVIEW.md), [18 builder questions](DRANQUEST_BUILDER_QUESTIONS.md),
[candidate row 35](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L35), and
[coverage row 386](ZONE_COVERAGE_REGISTER.csv#L386).

### Checkpoint 473: Duris3 retained pending owner and map-identity review

Keep Duris3 **RETAIN IN PLACE**. The only tracked component file is the
unregistered 90,000-room WLD. Preserve its separate room identities, all 26
current active-source edges to Duris3-only VNUMs, and the prior checkpoint
classifications of map-number matches. The broad raw collision totals do not
establish an active route or whole-map replacement. Close the map owner,
authoring/editor, full-world-list, typed code, player-location, and persisted
identity questions before any activation, renaming, move, or archive decision.
See the [audit](DURIS3_AUDIT_REVIEW.md), [20 builder questions](DURIS3_BUILDER_QUESTIONS.md),
[candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), and
[coverage row 386](ZONE_COVERAGE_REGISTER.csv#L387).

### Checkpoint 474: EOO retained pending import, shop, and map-identity review

Keep EOO (Valhalla) **RETAIN IN PLACE**. Preserve its ten room identities,
five mobiles, two objects, seven external O resets, fourteen SHP products,
single #29204 shop record, and one-way Duris3 boundary. Keep the prior Ailvio
source-only approach count as already reviewed. Resolve all #29200–#29209
map collisions with dark_gh, genocide_gh, and Newbie; the #29211–#29213
Newbie/eoo_op object conflicts; Player Castles imports; the conditional shop
fallback and its active Newbie keeper/room/prototype identities; Gunnar’s
unimplemented story text; and editor, deployment, and persisted-data closure
before any activation, move, or archive decision. See the [audit](EOO_AUDIT_REVIEW.md),
[24 builder questions](EOO_BUILDER_QUESTIONS.md), [candidate row 37](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L37),
and [coverage row 387](ZONE_COVERAGE_REGISTER.csv#L387).

### Checkpoint 475: eoo_op retained pending map, portal, and owner review

Keep candidate row 38, eoo_op, **RETAIN IN PLACE**. Preserve its three WLD
rooms, four mobiles, four objects, five directions, and the one-way #123225
Duris3 boundary. Keep the seven active Newbie directions to #29210–#29212
classified as Newbie-owned routes; the candidate’s three room numbers are
already active Newbie identities and establish no Outpost ingress.

Preserve the distinct Newbie and dark_gh room/prototype records, all four
type-25 eoo_op portals and their stored targets, the eoo.zon imports of
#29211–#29213, eoo_op.zon’s Player Castles #11051 import, and all current
reset placements. Resolve whether portal targets #29206/#29222/#29212/#29225
mean the active Newbie rooms or the competing EOO/EOO_OP2 map identities.
Close the Surt/Muspelheim-to-EOO story relationship, authoring/editor,
manifest, code, deployment, and persisted-room dependencies before any
activation, move, rename, archive, or merge decision. See the
[technical audit](EOO_OP_AUDIT_REVIEW.md), [24 builder questions](EOO_OP_BUILDER_QUESTIONS.md),
[candidate row 38](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L38), and
[coverage row 388](ZONE_COVERAGE_REGISTER.csv#L389).

### Checkpoint 475: eoo_op retained pending map, portal, and owner review

Keep candidate row 38, eoo_op, **RETAIN IN PLACE**. Preserve its three WLD
rooms, four mobiles, four objects, five directions, and the one-way #123225
Duris3 boundary. Keep the seven active Newbie directions to #29210–#29212
classified as Newbie-owned routes; the candidate’s three room numbers are
already active Newbie identities and establish no Outpost ingress.

Preserve the distinct Newbie and dark_gh room/prototype records, all four
type-25 eoo_op portals and their stored targets, the eoo.zon imports of
#29211–#29213, eoo_op.zon’s Player Castles #11051 import, and all current
reset placements. Resolve whether portal targets #29206/#29222/#29212/#29225
mean the active Newbie rooms or the competing EOO/EOO_OP2 map identities.
Close the Surt/Muspelheim-to-EOO story relationship, authoring/editor,
manifest, code, deployment, and persisted-room dependencies before any
activation, move, rename, archive, or merge decision. See the
[technical audit](EOO_OP_AUDIT_REVIEW.md), [24 builder questions](EOO_OP_BUILDER_QUESTIONS.md),
[candidate row 38](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L38), and
[coverage row 388](ZONE_COVERAGE_REGISTER.csv#L389).

### Checkpoint 476: eoo_op2 retained pending portals, map, and prototype owners

Keep candidate row 39, eoo_op2, **RETAIN IN PLACE**. Preserve six rooms, four
mobiles, two portal objects, nine directions, and the two separate
three-room WLD chains. Keep #29223 up to inactive Duris3 #151855 source-only
and retain both repeated title/prose sets until the builder explains them.

Preserve all six active Newbie room identities, dark_gh’s separate #29220–#29222
rooms, and Newbie’s same-number mobile/object prototypes. Retain the two
source-level EOO portal pairs, the dark_gh portal declarations, Newbie QST and
story references, Ailvio’s compiled reads of #29220/#29221, and the Player
Castles #11051 import. The 16 active Newbie WLD matches are Newbie routes, not
eoo_op2 ingress. Resolve exact room owners, reciprocal portal destinations,
the one-way Duris3 boundary, reset placement, editor/code/deployment paths, and
persisted room/object IDs before any activation, move, rename, archive, or
merge decision. See the [technical audit](EOO_OP2_AUDIT_REVIEW.md),
[24 builder questions](EOO_OP2_BUILDER_QUESTIONS.md), [candidate row 39](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L39),
and [coverage row 389](ZONE_COVERAGE_REGISTER.csv#L390).

### Checkpoint 477: eskull_hall retained pending map, code, and shop review

Keep candidate row 40, eskull_hall, **RETAIN IN PLACE**. Preserve its thirteen
room identities in two WLD components, four mobiles, two portal objects, 24
directions, both one-way Duris3 exits, the four Player Castles imports, and
the exact 15-item reset/shop list including unresolved #6089 and the 2026
gathering bags.

Resolve all active Newbie room/mobile/object overlaps, the #29305 inn callback,
Ailvio mobile IDs #29303/#29304, the portal destinations, and the conditional
non-mini SHP fallback with keeper #29303 and room #29302. Confirm the missing
#6089 prototype, story and editor ownership, deployment, and persisted VNUM
closure before any activation, rename, move, archive, or merge proposal. See
the [audit](ESKULL_HALL_AUDIT_REVIEW.md),
[24 builder questions](ESKULL_HALL_BUILDER_QUESTIONS.md),
[candidate dependency row 40](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L40),
and [coverage row 390](ZONE_COVERAGE_REGISTER.csv#L391).

### Checkpoint 478: ethdawn_op retained pending portal and owner review

Keep candidate row 41, ethdawn_op, **RETAIN IN PLACE**. Preserve the three
Mistywood rooms as one WLD component, three mobile prototypes, two type-25
portals, four directions, and the one-way Duris3 #156745 boundary. Record the
header #1 / room #29310–#29312 numeric-range mismatch and all four exits from
those out-of-range room IDs for owner review. Retain the
Player Castles #11054/#11104 imports and the source-only Ravenmis #29311
object reset.

Resolve the three distinct Newbie/loc_gh/eskull_hall room identities, Newbie’s
same-number bag/arrow/quiver item consumers, both portal targets and charges,
the #29311 Goodie portal placement, story and editor ownership, deployment,
and persisted-VNUM closure before any activation, rename, move, archive, or
merge proposal. See the [audit](ETHDAWN_OP_AUDIT_REVIEW.md),
[24 builder questions](ETHDAWN_OP_BUILDER_QUESTIONS.md),
[candidate dependency row 41](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L41),
and [coverage row 391](ZONE_COVERAGE_REGISTER.csv#L392).

### Checkpoint 479: fists_gh retained pending identity and shop review

Keep candidate row 42, fists_gh, **RETAIN IN PLACE**. Preserve its ten-room
WLD and single local component, all 21 directions, the one-way Duris3
#157556 boundary, seven room and four mobile collisions with active Patrols,
the five M and fourteen G resets, empty OBJ file, and one SHP record with
fourteen products. Keep CP318's #30507 east to #30506 and #30509 up to #30503
source-only approaches in the existing curated totals.

Preserve Player Castles object #11013 in candidate room #30504, the missing
#6089 stock prototype, the 2026 gathering bags, and the conditional shop
fallback fields keeper #30503 / room #30502. Resolve their active Patrols
identities and compiled inn binding #30504, plus the Duris3 boundary, story
and editor ownership, deployment, and persisted-VNUM closure before any
activation, move, rename, archive, or merge proposal. See the
[technical audit](FISTS_GH_AUDIT_REVIEW.md),
[24 builder questions](FISTS_GH_BUILDER_QUESTIONS.md),
[candidate row 42](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L42), and
[coverage row 392](ZONE_COVERAGE_REGISTER.csv#L393).

### Checkpoint 480: fooquest retained pending quest and code review

Keep candidate row 43, fooquest, **RETAIN IN PLACE**. Preserve its
16-room captain-ship component and isolated #65050 quest room, 30 local
directions, #636 / #65000-range field mismatch, 37 mobiles, 46 objects,
13 QST givers, and 31 Q/QA directives. Keep the extra WLD-directory
#63600-#63611 pirate set separate from the conventional #65000-#65011
MOB records until the owner identifies its source role.

Preserve the 37 M, 72 F, 330 G, and two E commands, including 320
consecutive #65041 tooth G resets after mobile #65035. Keep the external
Moonshae object #26201 QST requirement, the 2016-removed unnamed object
#65004, and compiled references for #65012-#65014, the 13 quest-giver
specials, object #65050, and transform items #21/#22. Resolve QST
registration, the isolated-room route, story and editor ownership,
deployment, and persisted-VNUM closure before any activation, move, rename,
archive, or merge proposal. See the
[technical audit](FOOQUEST_AUDIT_REVIEW.md),
[24 builder questions](FOOQUEST_BUILDER_QUESTIONS.md),
[candidate row 43](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L43), and
[coverage row 393](ZONE_COVERAGE_REGISTER.csv#L394).

### Checkpoint 481: forestb retained pending quest and shop review

Keep candidate row 44, forestb, **RETAIN IN PLACE**. Preserve its 45 MOB and
62 OBJ prototypes, three QST giver records, and four Q contracts; no WLD or
ZON file is present. Keep the bluebird's first QST G/R item directives and
narrative difference for owner review. Preserve the one SHP record, its four
candidate OBJ products, keeper mobile #38235, and room field #38259, which
also identifies a candidate OBJ crude comb.

Resolve whether the non-mini `areas/shp/*.shp` fallback should ingest this
unlisted file when `areas/world.shp` is absent. If it does, the unmatched
keeper can resolve to -1 before an unguarded `mob_index[keeper]` use; this is
a conditional static code risk, not a runtime observation. Identify the
intended quest-giver and shopkeeper placements, manifest/editor/deployment
owners, external typed references, and persisted-ID policy before any
activation, move, rename, archive, or merge proposal. See the
[technical audit](FORESTB_AUDIT_REVIEW.md),
[24 builder questions](FORESTB_BUILDER_QUESTIONS.md),
[candidate row 44](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L44), and
[coverage row 394](ZONE_COVERAGE_REGISTER.csv#L395).

### Checkpoint 482: forofcon retained pending map and quest review

Keep candidate row 45, forofcon, **RETAIN IN PLACE**. Preserve its 403-room
WLD, 817 directions, nine weak components, the #139900 D3 negative target,
eight fall and eight current records, and all 766 ZON commands. Keep the
#139900-#140302 map distinct from both the 403-room inactive Duris3 overlap
and the first 53 active Smoke room identities #139900-#139952. All such
same-number room blocks differ.

Preserve the five already-counted source-only exits to Smoke, the 19 M/21 D/
one O reset placements in Smoke-numbered rooms, and the duplicate #139945 D2
reset coordinate as source-specific data. Keep 152 MOBs, 165 OBJs, all 14
QST givers and eight contracts, including the two matching Baldzor exchanges.
The candidate QST/ZON item references are internal; active ZON/QST/SHP scans
found no exact mobile/object consumer. Preserve the three byte-identical
editor backups and the `.capzon` version-comment difference. Resolve header
#1 range fields, manifest/editor ownership, source-only route identity,
reset/map collisions, dynamic access, and persisted-VNUM closure before any
activation, move, rename, archive, or merge proposal. See the
[technical audit](FOROFCON_AUDIT_REVIEW.md),
[24 builder questions](FOROFCON_BUILDER_QUESTIONS.md),
[candidate row 45](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L45), and
[coverage row 395](ZONE_COVERAGE_REGISTER.csv#L396).

### Checkpoint 483: genesis retained pending range and shop ownership

Keep existing candidate row 46, Genesis, **RETAIN IN PLACE**. Preserve its
ten-room/#29900–#29909 WLD and one source-only D4 to Duris3 #157747, plus its
four mobiles and reset placements. The range and mobiles #29900/#29901
conflict numerically with separate unregistered aeon_gh records, and compiled
code also binds an inn handler to shared room #29903. Keep all three
Player Castles imports (#11105, #11055, #11013), the empty OBJ file, and the
single shop for keeper/room #29902. Preserve all nine shop products and
matching G resets: #3050, #6089, #108, #109, #111, #398, #53, #412, and #377.
The repeated current definitions for #53/#108/#109 and missing #6089 leave
stock identity unresolved; the non-mini shop fallback makes loading the SHP
without its unregistered keeper a conditional source-code risk. Resolve
zone-range and handler ownership, shop stock, manifest/editor state, and
persisted identities before any activation, move, rename, archive, or merge.
See the [Genesis audit](GENESIS_AUDIT_REVIEW.md#disposition),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-483-genesis-source-identity-and-shop-dependencies),
[24 builder questions](GENESIS_BUILDER_QUESTIONS.md),
[candidate row 46](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L46), and
[coverage row 396](ZONE_COVERAGE_REGISTER.csv#L397).

### Checkpoint 484: Genocide Guildhall retained pending map and shop ownership

Keep existing candidate row 47, genocide_gh, **RETAIN IN PLACE**. Preserve
its ten-room/#29200–#29209 title-only map, one-component reach from #29200,
source-only D0 to active Newbie #29201 already counted at CP304, and one-way
D4 to Duris3 #157677. Preserve its four MOBs, empty OBJ, and 21 ZON commands:
three Player Castles imports (#11059, #11109, #11013), five M placements,
and thirteen G stock resets. Its one SHP record has keeper #29202 and room
#29204; both numbers belong to different active Newbie identities, while the
candidate merchant and room are unlisted. If the non-mini shop fallback
includes the candidate SHP, its keeper can resolve to Newbie mobile #29202.
Keep all thirteen products, including duplicated #53/#108/#109, missing
#6089, and Heavens gathering bags #491–#494. Preserve the separate
genocide_hall source at #29500–#29511; similar names and the parallel bag
update do not establish replacement. Resolve Newbie/dark_gh/eoo map and
mobile identities, the compiled #29202 inn handler, shop room and keeper
mapping, source ownership, and editor/persisted state before any activation,
move, rename, archive, or merge proposal. See the
[Genocide Guildhall audit](GENOCIDE_GH_AUDIT_REVIEW.md#same-number-identities-history-and-disposition),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-484-genocide-guildhall-identities-and-history),
[24 builder questions](GENOCIDE_GH_BUILDER_QUESTIONS.md),
[candidate row 47](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L47), and
[coverage row 397](ZONE_COVERAGE_REGISTER.csv#L398).

### Checkpoint 485: Genocide Guild Hall retained pending range and shop ownership

Keep existing candidate row 48, genocide_hall, **RETAIN IN PLACE**. Preserve
its seven-room map, two disconnected components, unnamed room #29504, and
source-only D4 edges to ud_map2 #1124854 and Duris3 #144550. Preserve four
MOB prototypes, empty OBJ, and 25 ZON commands: four Player Castles imports,
nine M placements, and twelve G stock resets. Its one SHP record uses keeper
and room #29503; no matching active keeper is present in the full area
identity set, and shop fallback assignment remains a conditional static risk.
Keep all twelve products, including duplicate-owner #53, missing #6089, and
Heavens gathering bags #491–#494.

Resolve Harrow’s active #294–#296 manifest range, the same-number aod_gh,
legion, and rf_op map/mobile/object identities, and the compiled #29502 inn
binding. Confirm both disconnected map sections and external destinations,
shop room/keeper mapping, source ownership, editor state, and persisted IDs
before any activation, move, rename, archive, or merge proposal. The 2026
parallel bag update with genocide_gh does not establish replacement. See the
[Genocide Guild Hall audit](GENOCIDE_HALL_AUDIT_REVIEW.md#same-number-identities-history-and-disposition),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-485-genocide-guild-hall-identities-and-history),
[24 builder questions](GENOCIDE_HALL_BUILDER_QUESTIONS.md),
[candidate row 48](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L48), and
[coverage row 398](ZONE_COVERAGE_REGISTER.csv#L399).

### Checkpoint 486: Gimluk’s Cottage retained pending range and shop ownership

Keep candidate row 49, gianthou, **RETAIN IN PLACE**. Preserve its six source
files, 271-room connected local map, negative #42811 D0 target, source-only
#42870 D3 edge to unresolved #274141, and all 14 QST giver records. Preserve
the full reset set, including 6,634 P object-in-object placements and 38
mobile-follow commands, all 122 local objects, the unused #42618 mobile, and
four SHP records with 28 locally defined products.

Resolve the Pworm #425–#426 manifest band and open #427–#428 ranges, world
and QST list ownership, both conflicting unlisted definitions of #274141,
quest/story continuity, the nested-object reset set, all shop keeper/room
pairs, and the conditional wildcard SHP fallback before activation or travel
claims. Confirm editor/deployment copies, exact persisted IDs, and source
owners before any move, rename, archive, or merge proposal. No same-stem
replacement or rename evidence clears these dependencies. See the
[Gimluk’s Cottage audit](GENIANTHOU_AUDIT_REVIEW.md#identity-history-and-disposition),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-486-gimluks-cottage-identities-and-history),
[24 builder questions](GENIANTHOU_BUILDER_QUESTIONS.md),
[candidate row 49](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L49), and
[coverage row 399](ZONE_COVERAGE_REGISTER.csv#L400).

### Checkpoint 487: Guild Rooms retained pending typed shop ownership

Keep candidate row 50, guild_rooms, **RETAIN IN PLACE**. Preserve its one
SHP record headed #8850, five product references #8555–#8559, keeper #8552,
and shop room #8560. Do not treat the same-number active Grave rooms as
mobile or object prototypes. Preserve the separate compiled guild-transfer
reference to active Grave room #8556.

Resolve the missing current #8552 MOB and #8555–#8559 OBJ definitions, the
shop’s active Grave room #8560, the relation to the separate guildhalls
#480–#489 area range, and the non-mini wildcard shop-file fallback before
loading or moving this SHP. Confirm editor/deployment copies and persisted
shop/mobile/object IDs before any activation, rename, archive, or merge
proposal. See the [Guild Rooms audit](GUILD_ROOMS_AUDIT_REVIEW.md#typed-shop-references-and-active-grave-identities),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-487-guild-rooms-identities-and-history),
[24 builder questions](GUILD_ROOMS_BUILDER_QUESTIONS.md),
[candidate row 50](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L50), and
[coverage row 400](ZONE_COVERAGE_REGISTER.csv#L401).

### Checkpoint 488: Halfcut Hills retained as a separate inactive source

Keep candidate row 51, halfcut_hills, **RETAIN IN PLACE** and do not load its
definitions alongside active halfcut. Preserve all five files and their
history: the candidate repeats all 235 active room vnums, all 45 mobile
vnums, and all 22 object vnums, with divergent WLD/MOB/OBJ/QST records. Keep
its 184-command reset set, two dialogue-only QST records, unused MOBs
#27031/#27042, and unused OBJ prototypes #27006/#27007 distinct from the
active area.

Resolve the source relationship and shared numeric identities, candidate
#27001 D0 to unlisted #256031, active Surface ingress to active halfcut
#27001, story/quest sidecar ownership, code binding #27009, and editor,
deployment, and persisted-ID state before any activation, merge, or path
decision. The active dossier calls the candidate inactive but does not
provide archive authorization. See the
[Halfcut Hills audit](HALFCUT_HILLS_AUDIT_REVIEW.md#source-history-and-disposition),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-488-halfcut-hills-candidate-and-active-source-identities),
[24 builder questions](HALFCUT_HILLS_BUILDER_QUESTIONS.md),
[candidate row 51](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L51), and
[coverage row 401](ZONE_COVERAGE_REGISTER.csv#L402).

### Checkpoint 489: Hostel retained pending story and low-VNUM ownership

Keep candidate row 52, hostel, **RETAIN IN PLACE**. Preserve its five unlisted
source files and do not load their #1–#102 rooms, #1–#66 mobiles, or #1–#76
objects alongside the current Limbo/Heavens identities. Retain its 242-exit,
five-component WLD graph and its 230 reset commands plus 36 door-state
commands as a distinct source. The active Savannah six-instrument lead is
unresolved; the candidate QST file contains four different story threads and
does not implement that lead.

Resolve the 45 room, 65 mobile, 76 object, and one QST giver-number overlaps;
the active Desert and Thetis exits to Limbo #1 and Heavens #38; the intended
Hostel spelling and map boundaries; candidate door/follower/reset semantics;
Savannah’s missing continuation; and editor, deployment, code, and
persisted-ID ownership before any activation, renumber, merge, or path
proposal. The active numeric matches do not establish Hostel ingress. See
the [Hostel audit](HOSTEL_AUDIT_REVIEW.md#history-and-disposition),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-489-hostel-source-identities-and-history),
[24 builder questions](HOSTEL_BUILDER_QUESTIONS.md),
[candidate row 52](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L52), and
[coverage row 402](ZONE_COVERAGE_REGISTER.csv#L403).

### Checkpoint 490: IC3 retained pending route and identity ownership

Keep candidate row 53, `IC3`, **RETAIN IN PLACE**. Preserve its four unlisted
WLD, MOB, OBJ, and ZON files and their high VNUM identities. The candidate WLD
has one source-only exit to active Rice Fields #77262 and no current active
inbound route; the former active entrance was removed from #77262 in April
2026. Do not treat the open `areas/AREA` range as proof that the identities
are available for reuse.

Resolve the source owner and post-de-listing editor/deployment copies; persisted
character and world-state references from IC3's time in the active list; the
Heavens #360 ZON import; mode-0 reset semantics; all five keyed doors and ten
door resets; the unused #420018 key object; and the portal destination and
placement discrepancy. The map's Poseidon-curse and Dreadnaught narrative has
no same-stem QST or story sidecar. Preserve the source-only Rice Fields edge
without calling it current access. See the [IC3 audit](IC3_AUDIT_REVIEW.md#narrative-history-and-disposition),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-490-ic3-source-identity-and-history),
[24 builder questions](IC3_BUILDER_QUESTIONS.md),
[candidate row 53](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L53), and
[coverage row 403](ZONE_COVERAGE_REGISTER.csv#L404).

### Checkpoint 491: Infernal Guildhall retained pending map, shop, and prototype ownership

Keep candidate row 54, `infernal_gh`, **RETAIN IN PLACE**. Preserve the five
unlisted WLD, MOB, OBJ, ZON, and SHP files. Harrow actively owns every one of
Infernal's 16 room, three mobile, and four object VNUMs with different
definitions; keep Harrow's live room graph, resets, special assignment, and
item consumers on their current owners.

Resolve the three source-only one-way exits to `Duris3.wld`; the candidate's
three WLD components and four placed type-25 links; the seven Player Castles
object imports; the unprototyped #6089 shop/reset item; the `dark.shp` duplicate
#29401 record and conditional all-file shop fallback; Harrow keeper #29401 and
room #29407 mappings; the September 2026 ZON stock edit; and map, editor,
deployment, persisted-ID, and story ownership. The two Surface numeric exits
to #29400 are Harrow's existing reciprocal boundaries, not Infernal access.
See the [Infernal audit](INFERNAL_GH_AUDIT_REVIEW.md#narrative-history-and-disposition),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-491-infernal-guildhall-source-identities),
[30 builder questions](INFERNAL_GH_BUILDER_QUESTIONS.md),
[candidate row 54](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L54), and
[coverage row 404](ZONE_COVERAGE_REGISTER.csv#L405).

### Checkpoint 492: Jungle retained pending source, route, and movement ownership

Keep candidate row 55, Jungle, **RETAIN IN PLACE**. Preserve its four WLD,
MOB, OBJ, and ZON sources and #67000–#67092 identities. The full area-list
entry was commented in April 2026, while qst/AREA still lists Jungle even
though qst/jungle.qst is absent. The standard quest compiler reads the full
AREA file, so the separate QST/AREA row is not proof of a loaded quest.

No active WLD exit enters Jungle. Preserve four inbound source-only references
from surf.wld and underdark2011.wld, three one-way outbound references to
unlisted Duris3, and the two Jungle-to-Surface routes already counted at CP373.
Keep the mode-1 reset commands, eleven southbound current fields, and F 5
bridge field at #67051 with its down exit to #67092 until the area and reset
owners confirm their intent. The bounded tracked scan found no same-type
VNUM collision or external typed ZON, QST, SHP, sidecar, or C/C++ consumer.

Resolve the stale-looking QST/AREA entry, map and quest ownership, all source-only
routes, current/fall movement, editor and deployment copies, and persisted IDs
before a separate archive or path proposal. The source remains story-bearing
but has no QST, SHP, or story sidecar. See the [Jungle audit](JUNGLE_AUDIT_REVIEW.md),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-492-jungle-source-identities-and-manifest-history),
[27 builder questions](JUNGLE_BUILDER_QUESTIONS.md),
[candidate row 55](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L55), and
[coverage row 405](ZONE_COVERAGE_REGISTER.csv#L406).

### Checkpoint 493: Kilfray retained pending QST lineage and contract ownership

Keep candidate row 56, Kilfray, **RETAIN IN PLACE**. Preserve the single
areas/qst/kilfray.qst file outside both production area lists. The normal QST
compiler reads areas/AREA, and the tracked standard input does not include
Kilfray. The five giver VNUMs are active Mistywood mobiles; five item
references target active Mistywood objects. Three contracts parallel active
Mistywood, with Riliatar and Jarnes experience rewards present only in
Mistywood.

There is no Kilfray map, WLD route, mobile, object, zone, shop, or story
sidecar. The file's story text uses the Mistywood giver identities, but no
tracked commit establishes it as an earlier version or replacement. Keep it
separate while its author, quest owner, editor/export path, reward intent, and
persisted completion identity are reconciled. Do not add it to production,
delete it, or merge its dialogue from this audit alone. See the
[Kilfray audit](KILFRAY_AUDIT_REVIEW.md),
[Mistywood audit](MISTYWOOD_AUDIT_REVIEW.md#typed-dependency-and-qst-lineage),
[20 builder questions](KILFRAY_BUILDER_QUESTIONS.md),
[candidate row 56](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L56), and
[coverage row 406](ZONE_COVERAGE_REGISTER.csv#L407).

### Checkpoint 905: Kilfray retains parallel QST and Mistywood map identities

Keep candidate row 56, `kilfray`, **RETAIN IN PLACE** outside both production
lists. Preserve the five active Mistywood giver/item identities and the three
parallel Q contracts with their existing reward differences. Active Mistywood
rooms #95000–#95001 are on the Kilfray River; three unlisted Surf approach
records from Lake Kilfray / The Kilfray River already belong to Mistywood's
CP284 source-only count. They are not active routes and do not establish that
the QST is bound to those rooms. Reconcile the setting of the swamp rescue and
tower scenes, source author, editor/export path, XP rewards, and persisted
quest identity before any production-list change or path proposal. See the
[Kilfray role audit](KILFRAY_AUDIT_REVIEW.md#checkpoint-905-cross-source-identity-environment-and-typed-role-refresh),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-905-kilfrays-namesake-routes-remain-mistywood-owned),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-905-kilfray-map-names-do-not-establish-qst-replacement),
[20 builder questions](KILFRAY_BUILDER_QUESTIONS.md), and [coverage CSV line 407](ZONE_COVERAGE_REGISTER.csv#L407).

### Checkpoint 494: Kzkrkeep retained pending quest, shop, and item ownership

Keep candidate row 57, Kzkrkeep, **RETAIN IN PLACE**. Its QST and SHP sources
are absent from both area lists, and the normal compilers use `areas/AREA`.
Quest runtime does not glob orphan QST files. Shop runtime has a separate
fallback that globs `areas/shp/*.shp` when `areas/world.shp` is unavailable;
that fallback can include `kzkrkeep.shp`, so list exclusion alone does not
settle its shop-input status.

The QST says troll leader amulet, but its commands remove active Worms OBJ
#6900 (grey wormskin) and award #6912 (purple wormskin). Both shop keepers
#6917/#6924 lack active MOB prototypes; their locations #6915/#6919 resolve to
Worms rooms. Twenty-two of 25 positive product references resolve to active
objects; #6958/#6968/#6969 are Worms room VNUMs without OBJ prototypes.
Resolve the item/story conflict, keeper ownership, shop placement and stock,
generated/deployed `world.shp` behavior, and editor/persisted copies before
changing either path or list. Do not infer replacement from VNUM overlap.

See the [Kzkrkeep audit](KZKRKEEP_AUDIT_REVIEW.md),
[20 builder questions](KZKRKEEP_BUILDER_QUESTIONS.md),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md),
[Worms typed-reference findings](WORMS_AUDIT_REVIEW.md#external-typed-and-code-references),
[candidate row 57](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L57), and
[coverage row 407](ZONE_COVERAGE_REGISTER.csv#L408).

### Checkpoint 906: Kzkrkeep retains unlisted QST and shop sources

Keep candidate row 57, `kzkrkeep`, **RETAIN IN PLACE** outside both area
lists. Preserve the current Worms typed targets: QST item commands #6900 and
#6912 bind active wormskin objects; SHP locations #6915/#6919 bind active
Worms rooms; 22 of 25 shop products bind active Worms/Heavens objects; and
#6958/#6968/#6969 remain room VNUMs with no object prototypes. The additional
Ixarkon code role for #6900 is a room destination, separate from its QST item
role. Thirteen external `N`-format shop rows elsewhere reuse seven active
Heavens stock objects, without owning Kzkrkeep's missing keepers or rooms.
Resolve quest text, mobile identities, shop input/fallback, invalid stock,
editor/export paths, and persisted completion state before any list, path, or
deletion proposal. See the [Kzkrkeep role audit](KZKRKEEP_AUDIT_REVIEW.md#checkpoint-906-cross-source-typed-role-refresh),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-906-kzkrkeep-roles-remain-in-active-worms),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-906-kzkrkeep-numeric-overlaps-remain-worms-identities),
[20 builder questions](KZKRKEEP_BUILDER_QUESTIONS.md), and [coverage CSV line 408](ZONE_COVERAGE_REGISTER.csv#L408).

### Checkpoint 495: Labyrint retained as an unlisted parallel QST copy

Keep candidate row 58, `labyrint.qst`, **RETAIN IN PLACE**. The active
`labyrinth` area remains in both area lists and defines the same giver #5028,
the same ten item-command IDs, and the same 50,000-experience command. The
active block adds a nine-proof response and fuller reward dialogue. Standard
QST generation uses `areas/AREA`, so it compiles the active `labyrinth.qst`
and excludes `labyrint.qst`.

The candidate has no WLD or separate map; its item references resolve to
active Labyrinth objects. Its followed history shows no direct rename or
replacement decision. Keep the source out of both lists until the author
confirms lineage, ownership, and persisted quest identity. The existing
Labyrinth audit already records a source-backed builder question and
recommendation on this exact copy. See the
[Labyrinth audit](LABYRINTH_OF_NO_RETURN_AUDIT_REVIEW.md),
[source dossier](../zone-stories/LABYRINTH_OF_NO_RETURN.md),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md),
[candidate row 58](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L58), and
[coverage row 408](ZONE_COVERAGE_REGISTER.csv#L409).

### Checkpoint 907: Labyrint keeps its unlisted parallel QST source

Keep candidate row 58, `labyrint.qst`, **RETAIN IN PLACE** outside both
production area lists. Preserve the single unlisted QST file and its authored
copy of active Labyrinth's #5028 Knight branch, ten item commands, and
50,000-experience amount. The active `labyrinth.story.json` records the
generated branch; adding the parallel file would not create a new map and
could duplicate the active contract if separately loaded. Resolve author and
quest provenance plus persisted completion identity before any activation,
move, or archive proposal. See the [Checkpoint 907 typed audit](LABYRINTH_OF_NO_RETURN_AUDIT_REVIEW.md#checkpoint-907-labyrint-qst-branch-retains-active-labyrinth-item-roles),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-907-labyrint-remains-qst-only-beside-the-active-labyrinth-graph),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-907-labyrints-parallel-qst-remains-a-separate-source-identity),
[embedded builder questions](LABYRINTH_OF_NO_RETURN_AUDIT_REVIEW.md#builder-questions-and-recommended-resolutions), and [coverage CSV line 409](ZONE_COVERAGE_REGISTER.csv#L410).

### Checkpoint 496: Legion retained pending map, reset, shop, and code ownership

Keep candidate row 59, Legion, **RETAIN IN PLACE**. Its WLD, MOB, OBJ, ZON,
and SHP sources are absent from both area lists; do not activate or move them
from that fact alone. Preserve the 16-room, three-component map; its three
one-way source references to unlisted Duris3; the competing AoD, Genocide
Hall, and RFOP room identities; and the six Legion mobiles and four portals
that reuse same-type VNUMs in those sibling sources. Active Harrow's *294–296
AREA band contains no current room in #29500–#29523.

Keep all 35 ZON commands and the mode-0 header together for owner review,
including seven source-only Player Castles imports, four local portal
placements, thirteen candidate mobile placements, and eleven G loads. Preserve
the two source-declared reciprocal portal-object pairs separately from WLD
connectivity. The one SHP record has keeper #29500 and room #29504 and lists
the same eleven products as Vuthen's G resets; #6089 lacks a checked-in OBJ.
The standard shop compiler follows AREA, while this checkout's non-mini
runtime fallback can glob SHP files if areas/world.shp is absent. Compiled
code also assigns an inn callback by numeric room #29502, which is shared by
different unlisted map identities.

Resolve map and typed-VNUM ownership, the title/header collisions, each
Duris3 boundary, the Player Castles placements, the shop aggregate/fallback
inputs, missing #6089 prototype, #29502 code use, and editor, deployment,
story, and persisted-ID state before a separate list or path proposal. See
the [Legion audit](LEGION_GUILDHALL_AUDIT_REVIEW.md),
[25 builder questions](LEGION_GUILDHALL_BUILDER_QUESTIONS.md),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-496-legion-guildhall-source-identities-and-vnum-collisions),
[candidate dependency row 59](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L59),
and [coverage row 409](ZONE_COVERAGE_REGISTER.csv#L410).

### Checkpoint 908: Legion retains separate map, portal, and shop identities

The typed refresh keeps Legion **RETAIN IN PLACE** with no list, path, or
archive proposal. Preserve AoD and RFOP portal destinations with their own
unlisted WLD source identities, the separate Legion portal pairs, AoD's badge
product #29500, and each shop's source-local keeper and room fields. The shop
fallback remains conditional because `areas/world.shp` is absent and no runtime
was inspected. No exact QST/story consumer or additional code lookup changes
the separate #29502 inn-owner question. Retain the CP496 map, reset, Player
Castles, missing #6089, history, and persisted-ID decisions for their owners.
See the [CP908 typed audit](LEGION_GUILDHALL_AUDIT_REVIEW.md#checkpoint-908-typed-role-refresh),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-908-legion-sibling-travel-records-remain-source-local),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-908-legion-same-number-portal-and-shop-roles-remain-source-local),
[25 builder questions](LEGION_GUILDHALL_BUILDER_QUESTIONS.md), and
[coverage row 409](ZONE_COVERAGE_REGISTER.csv#L410).

### Checkpoint 497: Light Guildhall retained pending Underworld3 and shop ownership

Keep candidate row 60, light_gh, **RETAIN IN PLACE**. Preserve its five source
files outside both area lists while active Underworld3 retains the same 16
room, four mobile, and four object VNUMs with different content. Keep
Underworld3's #30000 Moonshae boundary and its same-number reset consumers
with their current active identities; neither the numeric match nor the
candidate's source graph gives Light Guildhall active ingress.

Retain the 28 candidate WLD exits: 26 local targets that resolve numerically
to active Underworld3 rooms and two one-way source references to unlisted
Duris3. Preserve the reciprocal #30000/#30001 portal placements and values,
the #30002 portal reset, and unplaced #30003 prototype pending map-owner
review. Keep all 38 ZON commands together, including seven source-only Player
Castles imports, three local portal placements, 13 mobile placements, and
15 item loads.

The single SHP record uses keeper #30001 and room #30004, which are active
Underworld3's lizardman and shaft VNUMs. Fourteen of its 15 products have
active object prototypes; #6089 has none. Standard shop generation uses AREA,
but the non-mini fallback can glob the candidate SHP when areas/world.shp is
missing. Reconcile the current generated/deployed shop input, VNUM ownership,
2026 bag-stock edit, editor/export copies, and persisted identity before any
separate loader or path proposal. See the
[Light Guildhall audit](LIGHT_GH_AUDIT_REVIEW.md),
[25 builder questions](LIGHT_GH_BUILDER_QUESTIONS.md),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-497-light-guildhall-room-and-prototype-collisions),
[candidate dependency row 60](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L60),
and [coverage row 410](ZONE_COVERAGE_REGISTER.csv#L411).

### Checkpoint 909: Light Guildhall keeps Underworld3 active roles separate

Keep candidate row 60, `light_gh`, **RETAIN IN PLACE** outside both area lists.
Preserve active Underworld3 room/mobile/object identities and reset consumers,
including its two M room placements at #30004/#30005, 33 mobile reset uses of
#30000–#30003, and four equipment reset uses of #30000–#30003. These refer to
Underworld3 source identities rather than activating Light Guildhall. Keep the
Moonshae boundary attached to Underworld3, the candidate's two Duris3 exits,
its three source portal placements and unplaced #30003 object, seven Player
Castles imports, recent #491–#494 edit, and unresolved #6089 product. External
QST/story reward amounts, shop fields, and raw compiled constants add no new
candidate consumer. The shop fallback remains conditional because
`areas/world.shp` is absent. Resolve editor/deployment inputs and persisted
identity with the owners before any separate path/list/archive proposal. See
the [CP909 typed audit](LIGHT_GH_AUDIT_REVIEW.md#checkpoint-909-typed-role-refresh),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-909-underworld3-reset-roles-remain-active-map-uses),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-909-active-underworld3-reset-roles-remain-separate-from-light-guildhall),
[25 builder questions](LIGHT_GH_BUILDER_QUESTIONS.md), and
[coverage row 410](ZONE_COVERAGE_REGISTER.csv#L411).

### Checkpoint 498: Lords of Chaos Guildhall retained pending map, code, and shop ownership

Keep candidate row 60, loc_gh, **RETAIN IN PLACE**. Preserve its five source
files outside both area lists while active Newbie retains its 14 different
room definitions at shared VNUMs #29300–#29312 and #29314, active mobiles
#29300–#29303, and active objects #29300–#29305. Keep the Newbie QST/story
references and compiled mobile/room bindings attached to Newbie's current
identities. Resolve the additional cbr_hall, eskull_hall, ravenmis, and
ethdawn_op map overlaps before proposing any separate list or path.

Retain the 39 candidate WLD exits: 36 local directions and three one-way
source references to unlisted Duris3. Preserve the four local components and
the two reciprocal object-portal pairs as separate authored travel data.
Keep all 46 ZON commands together, including ten source-only Player Castles
imports, four local portal placements, 17 mobile placements, and 15 G loads.
Do not use Newbie's 19 active same-number WLD references as evidence of LOC
ingress.

The single SHP record keys keeper #29301 and room #29308, which resolve in the
active table to Newbie's amateur assassin and storage locker. Fourteen of its
15 products have active object prototypes; #6089 has none. Standard shop
generation uses AREA, while the non-mini fallback can glob loc_gh.shp when
areas/world.shp is missing. Preserve the 2026 Heavens bag stock/reset edit.
Reconcile the shop aggregate and runtime inputs, room/prototype ownership,
Player Castles placements, code and QST consumers, editor/export copies, and
persisted identity before any loader, rename, or path proposal. See the
[Lords of Chaos audit](LOC_GH_AUDIT_REVIEW.md),
[25 builder questions](LOC_GH_BUILDER_QUESTIONS.md),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-498-lords-of-chaos-guildhall-source-identities-and-vnum-collisions),
[candidate dependency row 60](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L61),
and [coverage row 411](ZONE_COVERAGE_REGISTER.csv#L412).

### Checkpoint 910: loc_gh retains separate reset, quest, shop, and portal identities

Keep candidate row 60, `loc_gh`, **RETAIN IN PLACE**. Preserve active Newbie's
16 exact same-room WLD targets, its room/mobile/object table and reset roles,
QST/story items, #29303/#29304 mobile callbacks, and #29305 inn binding. Three
other active Newbie exits in the broad band target Newbie-only #29313/#29315/
#29316 rooms and must not be counted as loc_gh overlaps. Keep same-number local
WLD records in CBR Hall, Ebon Skull, Ravenmis, and Ethdawn on those unlisted
source identities.

Preserve the distinct candidate and sibling reset, shop, and portal records:
Newbie's #29305 stock/portal and #29308 portal destination are active Newbie
roles; Ebon Skull portals target its own rooms; Ethdawn's #29310 to #29307 is a
source-only numeric destination. Keep CBR Hall, Ebon Skull, Ravenmis, and
loc_gh N-format SHP roles separate, and confirm fallback aggregate inputs
before any runtime proposal. Retain all 46 candidate ZON commands, Player
Castles imports, portal pairs and unplaced objects, the three Duris3 exits,
2026 bag additions, and missing #6089 until map, shop, code, story, editor,
deployment, and persisted-ID owners resolve them. No list, path, or archive
clearance follows from these same-number roles. See the
[CP910 typed audit](LOC_GH_AUDIT_REVIEW.md#checkpoint-910-role-aware-source-closure),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-910-loc_gh-same-number-destinations-remain-source-owned),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-910-loc_gh-reset-shop-and-portal-roles-remain-separate),
[25 builder questions](LOC_GH_BUILDER_QUESTIONS.md), and
[coverage row 411](ZONE_COVERAGE_REGISTER.csv#L412).

### Checkpoint 499: Luiren retained pending QST and shop identity

Keep candidate row 61, luiren, **RETAIN IN PLACE**. Preserve luiren.qst and
luiren.shp outside both area lists. The 44 M dialogue giver headers have
numeric matches to active Pine Hollow mobiles, but the candidate has no WLD,
MOB, OBJ, or ZON source, and standard QST generation excludes it through
qst/AREA. Do not infer a map, live giver set, route, or Pine Hollow
replacement from those VNUMs.

Preserve all eight old O-format shop records as identity-ambiguous source
data. They contain 16 Pine Hollow object-product matches, eight Pine Hollow
keeper matches, and room fields matching #16008 and #16058. The standard
shop compiler excludes the file through AREA, but non-mini fallback can glob
it if areas/world.shp is missing; the runtime loader rejects O format. Confirm
the deployment aggregate, runtime mode, supported shop input, source lineage,
and persisted quest/shop state before any format conversion, list entry, path
change, or archive proposal. See the
[Luiren audit](LUIREN_AUDIT_REVIEW.md),
[20 builder questions](LUIREN_BUILDER_QUESTIONS.md),
[Pine Hollow identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-499-luiren-giver-and-legacy-shop-vnum-overlaps),
[candidate dependency row 61](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L62),
and [coverage row 412](ZONE_COVERAGE_REGISTER.csv#L413).

### Checkpoint 500: Magetower retained pending low-VNUM, map, code, and editor ownership

Keep candidate row 62, Magetower, **RETAIN IN PLACE**. Preserve its WLD, MOB,
OBJ, and ZON sources outside both area lists and keep the tracked
areas/map/Magetower.json coordinate file with them. Its 71 rooms use VNUMs
#1–#71 under header #1000, with 43 same-number active rooms in Limbo and
Heavens. Preserve their active room, mobile, and object tables, current code
bindings, active resets, and route ownership.

Keep the 152 candidate WLD directions in one local 71-room component and
retain the four D door-state commands and five M resets together. The
candidate's two local edges to #1 are source-only numeric matches to Limbo's
active room, not evidence of an active Magetower route. Keep the 23 mobile
and 20 object definitions separate from active Limbo/Heavens prototypes and
the separate areas_mini fixture.

Reconcile the #1000 header reuse, low VNUM range, 70 active numeric WLD
targets, compiled uses of #1–#44, the 2025 coordinate file, and all editor,
export, deployment, and persisted-world copies before any loader, renumbering,
path, merge, or archive proposal. See the
[Magetower audit](MAGETOWER_AUDIT_REVIEW.md),
[25 builder questions](MAGETOWER_BUILDER_QUESTIONS.md),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-500-magetower-low-vnum-source-identities-and-active-limbo-and-heavens-ownership),
[candidate dependency row 62](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L63),
and [coverage row 413](ZONE_COVERAGE_REGISTER.csv#L414).

### Checkpoint 501: map1 retained pending empty-file ownership

Keep candidate row 63, `map1`, **RETAIN IN PLACE pending owner decision**.
Preserve its tracked zero-byte `areas/mob/map1.mob` outside both area lists.
It contains no mobile prototype or VNUM, and no same-stem WLD, OBJ, ZON,
QST, SHP, story, or map file is present. Its rename-follow history reaches
older zero-byte `Players/` paths, but cannot establish a shared purpose or
game-area identity. Keep the distinct `ud_map1` family untouched and separate.
Do not synthesize content, move, or remove the file until its data, editor,
export, and deployment owners identify its role. See the
[map1 audit](MAP1_AUDIT_REVIEW.md),
[10 builder questions](MAP1_BUILDER_QUESTIONS.md),
[candidate dependency row 63](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L64),
and [coverage row 414](ZONE_COVERAGE_REGISTER.csv#L415).

### Checkpoint 502: map22 retained for random-lab code and Duris3 entry

Keep candidate row 64, `map22`, **RETAIN IN PLACE**. Preserve its complete
10,000-room WLD grid and the code that resets #900000–#909999 and starts the
type-1 generated maze at #905050. Preserve Duris3's distinct #145443 Ocean
room as the code-selected entrance. Both sources remain outside the full
area list, and `areas/RANDOM_AREA` is empty. The code checks room availability
before changing the map, but no runtime behavior or active route was tested.
Reconcile the random-zone, Duris3, object #68, editor, and deployment owners
before changing input lists, path, VNUMs, or lifecycle. See the
[map22 audit](MAP22_AUDIT_REVIEW.md),
[Duris3 audit](DURIS3_AUDIT_REVIEW.md),
[20 builder questions](MAP22_BUILDER_QUESTIONS.md),
[candidate dependency row 64](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L65),
and [coverage row 415](ZONE_COVERAGE_REGISTER.csv#L416).

### Checkpoint 503: map5 retained with source-only Surf resets and code bindings

Keep candidate row 65, `map5`, **RETAIN IN PLACE**. Preserve its 40 mobile
prototypes outside both area lists, the 45 `surf.zon` M resets for nine
map5 VNUMs in the 40-room source-only Surf map, and the direct patrol and
Tharnadian special assignments. Keep identical prose under different
VNUMs separate: active Killing Fields #33300/#33308 retain their QST/shop
roles; no matching role is established for map5 #150002/#150003. The `surf`
reset rows are not active placements. Reconcile the mobile-code, Surf,
Tharnadian, Minizones, Killing Fields, and deployment owners before changing
the file, reset inputs, VNUMs, or area lists. See the
[map5 audit](MAP5_AUDIT_REVIEW.md),
[20 builder questions](MAP5_BUILDER_QUESTIONS.md),
[Surf dependency row 96](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97),
[candidate dependency row 65](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L66),
and [coverage row 416](ZONE_COVERAGE_REGISTER.csv#L417).

### Checkpoint 504: map8 retained as an empty path pending owner identification

Keep candidate row 66, `map8`, **RETAIN IN PLACE pending owner decision**.
Preserve the tracked zero-byte `areas/mob/map8.mob` outside both area lists.
Its rename-follow history passes through empty `Players/` paths and is also
reported for map1, so it cannot establish a shared identity or safe cleanup.
Keep map1 and the separately named `oldmap8.wld` distinct. Do not create
content, move, or remove the path until source, data, editor, export, and
deployment owners identify its role. See the
[map8 audit](MAP8_AUDIT_REVIEW.md),
[10 builder questions](MAP8_BUILDER_QUESTIONS.md),
[candidate dependency row 66](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L67),
and [coverage row 417](ZONE_COVERAGE_REGISTER.csv#L418).

### Checkpoint 505: Mara Guildhall retained with shared VNUM and reset owners unresolved

Keep candidate row 67, `mara_gh`, **RETAIN IN PLACE** outside both area
lists. Preserve its 16-room, three-component WLD; 11 mobiles; six objects;
one shop; source-only resets; three Duris3 boundaries; room #29605 `inn`
assignment; and item-teleport values. Coordinate Mara Outpost's same-number
room/MOB/OBJ definitions, Player Castles object imports, shop stock (including
unresolved #6089), and editor/deployment copies before any path, list, reset,
or VNUM change. This does not clear activation or migration. See the
[Mara Guildhall audit](MARA_GH_AUDIT_REVIEW.md),
[20 builder questions](MARA_GH_BUILDER_QUESTIONS.md),
[Mara Outpost candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L69),
[candidate dependency row 67](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L68),
and [coverage row 418](ZONE_COVERAGE_REGISTER.csv#L419).

### Checkpoint 506: Mara Outpost retained with unresolved cross-component portal

Keep candidate row 68, `mara_op`, **RETAIN IN PLACE** outside both area
lists. Preserve its three-room WLD, two mobile prototypes, two portal objects,
four source-only resets, and single Duris3 boundary. Its #29610 portal
targets Mara Guildhall room #29608; the two components also share room, MOB,
and OBJ VNUMs with different fields. Reconcile the Mara owners, the portal
destination, Duris3, and editor/deployment copies before changing lists,
resets, VNUMs, or paths. No activation, merge, or move is cleared. See the
[Mara Outpost audit](MARA_OP_AUDIT_REVIEW.md),
[20 builder questions](MARA_OP_BUILDER_QUESTIONS.md),
[Mara Guildhall audit](MARA_GH_AUDIT_REVIEW.md),
[candidate dependency row 68](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L69),
and [coverage row 419](ZONE_COVERAGE_REGISTER.csv#L420).

### Checkpoint 507: Mavulsk retained pending VNUM and map-owner decisions

Keep candidate row 69, mavulsk, RETAIN IN PLACE outside both area lists.
Preserve its five files, 233-room map, 98 mobiles, 110 objects, 484 reset
commands, 13 QST giver records, 18 Q clauses, and six source-only
ITEM_TELEPORT definitions. Every same-type VNUM in its WLD/MOB/OBJ bands is
also defined in another source, with active same-number owners for 48 rooms,
97 mobiles, and 110 objects. Resolve the Thetis #38095 → #104 owner question,
Mavulsk #188 → Surf #228202 destination, all local teleport targets, the
QST index, reset/loading intent, and editor/deployment copies before any
activation, renumbering, archive, or path decision. No migration clearance
is implied. See the
[Mavulsk audit](MAVULSK_AUDIT_REVIEW.md),
[25 builder questions](MAVULSK_BUILDER_QUESTIONS.md),
[candidate dependency row 69](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L70),
and [coverage row 420](ZONE_COVERAGE_REGISTER.csv#L421).

### Checkpoint 508: Mentiri retained pending owner and tool closure

Keep candidate row 70, Mentiri, **RETAIN IN PLACE** outside both area lists.
Preserve the four WLD/MOB/OBJ/ZON files and the 72-position map JSON. The
setting names a captured archon, gate guardian, commander, and infernal
prison, but 39 rooms and all 20 objects remain generic placeholders, and no
same-stem QST or SHP is present. Resolve whether that content is intentional,
which map/editor file is canonical, and the active Limbo/Heavens and
areas_mini identity overlaps before activation, renumbering, archive, merge,
or path change. No migration clearance is implied. See the
[Mentiri audit](MENTIRI_AUDIT_REVIEW.md),
[22 builder questions](MENTIRI_BUILDER_QUESTIONS.md),
[candidate dependency row 70](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L71),
and [coverage row 421](ZONE_COVERAGE_REGISTER.csv#L422).


### Checkpoint 509: Monsteri retained pending owner and VNUM decisions

Keep Monsteri at candidate dependency data row 71 / CSV line 72, **RETAIN IN
PLACE**. Preserve its five tracked paths, including the empty OBJ and QST
files. Resolve the same-title cbl_outpost2 copy, the five reused descriptions
in active cbl_hall, the #1000 header versus band-10 range mismatch, the
separate Heavens/areas_mini mobile identities, and the Portal Cave’s missing
candidate-owned travel data. Identify any external editor, deployment, and
persisted copies before a separate activation, renumbering, archive, merge, or
path proposal. The empty-file rename-follow history is not migration clearance.
See the [Monsteri audit](MONSTERI_AUDIT_REVIEW.md),
[24 builder questions](MONSTERI_BUILDER_QUESTIONS.md),
[candidate dependency line 72](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L72), and
[coverage line 423](ZONE_COVERAGE_REGISTER.csv#L423).


### Checkpoint 510: moriatst retained pending shop-loader closure

Keep candidate row 72, `moriatst`, **RETAIN IN PLACE**. Preserve its single
SHP file and the three records that duplicate active Moria keeper, stock, and
room references. Normal `AREA` aggregation omits this stem, but the runtime
fallback glob includes `areas/shp/moriatst.shp` whenever `areas/world.shp`
cannot be opened. In that path the duplicate keeper/room records make the
reset-side shop selector non-unique. Resolve launch and deployment paths,
generated shop-stream ownership, and the original builder intent before any
archive or move decision. No migration clearance is implied. See the
[moriatst audit](MORIATST_AUDIT_REVIEW.md),
[12 builder questions](MORIATST_BUILDER_QUESTIONS.md),
[candidate dependency line 73](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L73),
and [coverage line 424](ZONE_COVERAGE_REGISTER.csv#L424).


### Checkpoint 511: Mountain QST retained for reference closure

Keep candidate row 73, uppercase `Mountain.qst`, **RETAIN IN PLACE**. The
five quester headers have no current matching mobile prototypes, seven
unique #210xx item references have no current object definitions, and the
remaining output `G I #93001` points into active Dungeon to black leather
armor while the prose calls its result a key. Normal QST aggregation omits
the file, and the runtime quest loader does not glob unlisted QST files.
Resolve its original map/story owner, all missing identities, Dungeon's item
role, and editor/deployment/persisted data before any archive or path change.
No migration clearance is implied. See the
[Mountain QST audit](MOUNTAIN_QST_AUDIT_REVIEW.md),
[12 builder questions](MOUNTAIN_QST_BUILDER_QUESTIONS.md),
[candidate dependency line 74](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L74),
and [coverage line 425](ZONE_COVERAGE_REGISTER.csv#L425).

### Checkpoint 512: mtncastl retained pending room #1 owner

Keep candidate row 75, mtncastl, **RETAIN IN PLACE**. Preserve its WLD room
#1, zero-byte MOB and OBJ files, and ZON title. Its room block matches astral
and ud-inside, while its exact title appears in five current ZON files; the
active room owner of #1 is Limbo. Resolve the candidate’s intended identity,
the header/range mismatch, the shared placeholder and title relationships,
and any external editor or persisted references before activation,
renumbering, merge, archive, or path move. No migration clearance is
implied. See the [mtncastl audit](MTNCASTL_AUDIT_REVIEW.md),
[13 builder questions](MTNCASTL_BUILDER_QUESTIONS.md),
[candidate dependency line 75](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L75),
and [coverage row 426](ZONE_COVERAGE_REGISTER.csv#L426).
### Checkpoint 513: myranth1 retained pending QST and shop closure

Keep candidate row 76, myranth1, **RETAIN IN PLACE**. Preserve its QST and
SHP files. The two QST giver prototypes and 22 unique item prototypes
referenced by the quest and shops are absent from current source; #70107 and
#70108 also collide by type with active Moregeeth mobiles. The five shops
point to active Moregeeth keepers and rooms and could enter the shop stream
through the source-code fallback if world.shp cannot be opened; this is not
live-loader proof. Resolve the Moorney/Zyla story owner, item/giver
definitions, intended shop bindings, manifest and loader paths, editor, and
persisted references before any separate move or archive decision. No
migration clearance is implied. See the [myranth1 audit](MYRANTH1_AUDIT_REVIEW.md),
[16 builder questions](MYRANTH1_BUILDER_QUESTIONS.md),
[candidate dependency line 76](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L76), and
[coverage row 427](ZONE_COVERAGE_REGISTER.csv#L427).
### Checkpoint 514: Nah retained pending VNUM and loader closure

Keep candidate row 77, nah, **RETAIN IN PLACE**. Preserve its five files and
do not merge its 13-room guildhall with active Underworld3 or unregistered
cabal_hall/shadow_hall. The candidate graph has two components and two
one-way up exits to Duris3. Twenty-four local target VNUMs currently resolve
to Underworld3 rooms with different content. Preserve the paired portal
value fields, seven O/nine M/eleven G reset commands, Player Castles object
imports, Eris shop, and overlapping mobile identities. The shop fallback is
conditional source-code behavior; the candidate is absent from normal
manifests. Resolve the room and mobile identities, reset mode, imports,
portal targets, shop stock, editor, and persisted references before any
activation, renumbering, move, or archive decision. No migration clearance is
implied. See the [NAH audit](NAH_AUDIT_REVIEW.md),
[20 builder questions](NAH_BUILDER_QUESTIONS.md),
[candidate dependency line 77](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L77), and
[coverage row 428](ZONE_COVERAGE_REGISTER.csv#L428).

### Checkpoint 515: Newbiemaps retain both map identities

Keep newbiemaps.wld and newbiemaps.zon in place. The source is absent from
the full and QST manifests, has an anomalous ZON header/reset field, and
overlaps active Surface on 59,851 room numbers with different titles. The
602 source-only approaches to Surface are already recorded and have the same
tuples in the 2011 temp copy. Git records the temp file as a 2011 copy from
newbiemaps; it contains 153 additional rooms and 804 additional directions,
but no source history declares either map canonical or retired. Preserve the
two source paths and their 602 records separately; do not infer an active route
or permission to move from numeric matches. Resolve map provenance, omitted
records, VNUM ownership, loader intent, editor and persisted references before
any separate activation, merge, move, or archive decision. No migration
clearance is implied. See the [NEWBIEMAPS audit](NEWBIEMAPS_AUDIT_REVIEW.md),
[18 builder questions](NEWBIEMAPS_BUILDER_QUESTIONS.md),
[identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-515-newbiemaps-source-identity-and-copy-history),
[candidate dependency line 78](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L78), and
[coverage row 429](ZONE_COVERAGE_REGISTER.csv#L429).


### Checkpoint 516: Nexus56 retained pending owner and reference closure

Keep the five-file Nexus56 source package **RETAIN IN PLACE**. Preserve its
city map, 23 distinct source-only approaches, 102 Alatorin mobile overlaps,
168 Alatorin object overlaps, all 1,181 ZON commands, and 19 legacy shops.
The candidate is absent from both normal manifests. Five external object
reset references lack current object prototypes (#2569, #17233, #17234,
#17241, and #17243); preserve those rows pending item-owner review.
The 60 other external reset object IDs have current active prototypes, and
54 external shop-product rows include one unresolved #2569. The conditional
SHP fallback does not establish live loading. Resolve room/mobile/object
ownership, route and reverse-exit intent, imports, keeper and stock identity,
manifest/editor state, and persisted references before any activation,
renumbering, merge, move, or archive decision. No migration clearance is
implied. See the [Nexus56 audit](NEXUS56_AUDIT_REVIEW.md),
[20 builder questions](NEXUS56_BUILDER_QUESTIONS.md),
[candidate dependency row 78](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79),
and [coverage row 429](ZONE_COVERAGE_REGISTER.csv#L430).

Checkpoint 772 verifies that source-only nexus56.zon line 1385 G resets
active Ashrumite object #66025. Preserve this import with the existing RETAIN
IN PLACE plan; the candidate audit now names the exact reference.

Checkpoint 773 verifies that source-only nexus56.zon line 1370 G resets
active Bastine object #7627 and nexus56.shp shop #83037 line 458 lists the
same product. Preserve both references with the existing RETAIN IN PLACE
plan; the candidate audit now names the exact reset and product.

Checkpoint 775 verifies that source-only nexus56.zon line 1378 G resets
active zone-13 Braddistock object #1335 and nexus56.shp shop #83038 line 501
lists the same product. Preserve both with the existing RETAIN IN PLACE plan;
the candidate audit now names the exact reset and product.

### Checkpoint 517: Nexus_1 retained pending low-VNUM ownership

Keep the four-file Nexus_1 package **RETAIN IN PLACE**. Its 40-room Black Onyx
Fortress map has no same-stem MOB/OBJ content, QST, SHP, story sidecar, or
current manifest entry. Preserve its 92-direction graph, all eight local ZON
door-state commands, the #17/#40 state asymmetry, five candidate-only room
IDs, and six source-only approaches into current Limbo/Heavens numeric owners.
Active references to #1–40 are owner aliases: 63 WLD target rows and 62 ZON
room/door reset rows target currently owned IDs, with none targeting #2 or
#6–9. Resolve the reserved #1 collision, map/editor provenance, mini-map
relationship, source-only route intent, door flags, imports, and persisted
references before any activation, renumbering, merge, move, or archive
decision. No migration clearance is implied. See the
[Nexus_1 audit](NEXUS_1_AUDIT_REVIEW.md),
[20 builder questions](NEXUS_1_BUILDER_QUESTIONS.md),
[identity/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md),
[candidate dependency row 79](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L80),
and [coverage row 430](ZONE_COVERAGE_REGISTER.csv#L431).

### Checkpoint 518: oldmap8 retained beside competing Duris3 grid

Keep `areas/wld/oldmap8.wld` **RETAIN IN PLACE**. It is a WLD-only 100×100
regional map with 39,918 directions, 88 within-file directions lacking an
opposite tuple, 410 external targets, and no active WLD inbounds. Thirteen
outbound directions reach active room owners and are already included in the
global source-only total. Preserve the 10,000 same-VNUM rooms in unregistered
Duris3, the 19,759 shared direction tuples, the 20,159/20,241 file-specific
tuples, and 102 target numbers without a room in another current WLD. Resolve
map ownership, editor/grid provenance, one-way route intent, destination
records, and persisted references before any activation, merge, renumber,
move, or archive decision. No migration clearance is implied. See the
[oldmap8 audit](OLDMAP8_AUDIT_REVIEW.md),
[20 builder questions](OLDMAP8_BUILDER_QUESTIONS.md),
[identity/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md),
[candidate dependency row 80](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L81),
and [coverage row 431](ZONE_COVERAGE_REGISTER.csv#L432).

### Checkpoint 519: Petrified Castle retain pending owner and reference closure

Keep the unlisted six-file Petrified Castle package **RETAIN IN PLACE**. It
has story-bearing castle and kingdom prose, 187 rooms, a local reset/population
set, one shop, and one QST contract. The source also contains a disconnected
room, six non-reciprocal plain directions, one exit to -1, and an apparent
QST item mismatch: the story asks for a creature's head while the QST records
keys as an R item and the Medusa head as a G item. No active typed inbound
references were found, but runtime aggregate selection, editor/import state,
deployed data, and persisted identity remain unverified. Resolve the zone
title versus story identity, quest exchange, shop and quest import paths,
route intent, and owner provenance before any activation, source repair,
renumbering, merge, move, or archive decision. No migration clearance is
implied. See the [Petrified Castle audit](PETRIFIED_CASTLE_AUDIT_REVIEW.md),
[20 builder questions](PETRIFIED_CASTLE_BUILDER_QUESTIONS.md),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-519-petrified-castle-story-package-with-no-same-type-collision),
[candidate dependency row 81](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L82),
and [coverage row 432](ZONE_COVERAGE_REGISTER.csv#L433).

### Checkpoint 520: plane_avernus retain pending map identity and provenance

Keep plane_avernus.wld **RETAIN IN PLACE**. Its two reciprocal rooms have no
population, shop, or quest records, and its reset-only ZON has no commands.
The zone title repeats active Avernus, but the WLD uses a different room
range and has no active inbound route. Two other unlisted WLDs also define
the candidate’s #135–#136 room numbers; their content and routes differ.
Resolve the Avernus title relationship, competing low-VNUM map identities,
empty reset/population state, editor/import history, and persisted references
before activation, merge, renumbering, move, or archive. No migration
clearance is implied. See the
[plane_avernus audit](PLANE_AVERNUS_AUDIT_REVIEW.md),
[20 builder questions](PLANE_AVERNUS_BUILDER_QUESTIONS.md),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-520-plane_avernus-title-collision-and-three-source-room-identities),
[candidate dependency row 82](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L83),
and [coverage row 433](ZONE_COVERAGE_REGISTER.csv#L434).

### Checkpoint 521: player_guilds retain shop records pending owner closure

Keep areas/qst/player_guilds.qst and areas/shp/player_guilds.shp **RETAIN IN
PLACE**. The QST is a no-contract #35320/S header; the SHP file has 13 shops
and 97 product rows. Nine keepers have no current MOB prototype, 21 stock
rows use 13 VNUMs with no current OBJ prototype, and six shop locations have
no current WLD definition. Other rows point to active Kimordril and Tundra
objects, active Longhollow/Temple2 #34405 identities, and active Lava Caves
room #35552. The same Asunwa keeper/location also appears in unlisted
ddguild.shp with different stock and flags. This checkout lacks
areas/world.shp, and the source loader has a conditional glob fallback; this
does not prove any shop is operating. Resolve the source owner, missing
prototypes, duplicate Asunwa record, cross-area stock/locations, and
deployment/editor state before any separate path decision. No migration
clearance is implied. See the
[Player Guilds audit](PLAYER_GUILDS_AUDIT_REVIEW.md),
[20 builder questions](PLAYER_GUILDS_BUILDER_QUESTIONS.md),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-521-player-guilds-shared-shop-sources-not-zone-replacement),
[candidate dependency row 83](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L84),
and [coverage row 434](ZONE_COVERAGE_REGISTER.csv#L435).

### Checkpoint 522: Ravenmis retain source identities pending owner decisions

Keep `areas/wld/ravenmis.wld`, `areas/mob/ravenmis.mob`,
`areas/obj/ravenmis.obj`, `areas/zon/ravenmis.zon`, and
`areas/shp/ravenmis.shp` **RETAIN IN PLACE**. The ten-room source-only graph
has one positive outlet to unlisted Duris3 room #125119 and no active inbound
route. Its same-number rooms, mobiles, and objects have distinct identities
from active Newbie and other unlisted guildhall sources. Preserve the three
Player Castles reset imports, unresolved #29311/#6098 object rows, active
Heavens/Verz1 shop stock, and the one shop’s #29300/#29309 keeper/location
mapping until the owner resolves source membership, collisions, outlet,
fallback-loader, editor, and persisted-state questions. No migration or
activation clearance is implied. See the
[Ravenmis audit](RAVENMIS_AUDIT_REVIEW.md),
[20 builder questions](RAVENMIS_BUILDER_QUESTIONS.md),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-522-ravenmis-distinct-guildhall-source),
[candidate dependency row 84](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L85),
and [coverage row 435](ZONE_COVERAGE_REGISTER.csv#L436).

### Checkpoint 523: Raxquest retain source files pending route and code closure

Keep areas/wld/raxquest.wld, areas/mob/raxquest.mob,
areas/obj/raxquest.obj, areas/zon/raxquest.zon, and areas/qst/raxquest.qst
RETAIN IN PLACE. Preserve the isolated #70500 room, the 83-room graph, and
its one-way source outlet to unlisted Duris3 #134956 while the map owner
resolves the route. Preserve all 63 mobiles, 98 objects, 391 reset commands,
and the 13-giver/31-contract QST. Resolve the exact normalized QST copies in
FooQuest and Black Pearl and the 2012 C091 OBJ copy from raxquest.obj into
blackpearl.obj. These are file-level lineage signals, not whole-zone
replacement evidence. Also resolve the external typed #26201 key reference
and the compiled nine-object/thirteen-mobile callback set plus ship_shop_proc
binding to #70501. The no-glob quest loader and absent areas/world.qst aggregate mean
per-stem files do not establish loaded contracts. Check external editor,
deployment, and saved-state references before any separate activation, move,
rename, merge, or archive proposal. No migration clearance is implied. See
the [Raxquest audit](RAXQUEST_AUDIT_REVIEW.md),
[21 builder questions](RAXQUEST_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-523-raxquest-source-only-wreck-and-outlet),
[QST identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-523-raxquest-qst-mirror-and-standalone-pirate-wreck-identity),
[Black Pearl copy-lineage checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-706-black-pearl-copy-lineage-and-separate-zone-history),
[candidate dependency row 85](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L86),
and [coverage row 436](ZONE_COVERAGE_REGISTER.csv#L437).

### Checkpoint 524: realms_master retain shop file pending loader and keeper closure

Keep areas/shp/realms_master.shp RETAIN IN PLACE. Preserve the two shop
records, eight stock OBJ VNUMs, keeper fields #11108/#11109, and location
fields #11121/#11124 while the shop and world-data owners resolve their
missing same-type prototypes. The full-mode loader can glob this file when
areas/world.shp is absent, then later assigns each keeper through real_mobile;
the current areas source has no such MOBs, and assign_the_shopkeepers has no
local missing-index guard. Keep the Player Castles OBJ boards #11108/#11109
typed separately from the SHP keeper IDs and preserve the five source ZON
board resets. Verify the stale #69/#68 diagnostic messages, disabled ship-room
binding block, external editor/deployment inputs, and saved references before
any separate activation, move, or archive proposal. No migration clearance is
implied. See the [realms_master audit](REALMS_MASTER_AUDIT_REVIEW.md),
[20 builder questions](REALMS_MASTER_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-524-realms_master-shop-records-without-a-room-map),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-524-realms_master-shop-records-and-typed-vnum-collisions),
[candidate dependency row 86](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L87),
and [coverage row 437](ZONE_COVERAGE_REGISTER.csv#L438).

### Checkpoint 525: returns retain mobile source pending tower and placement owners

Keep areas/mob/returns.mob RETAIN IN PLACE. Preserve both records, silent
golem #93400 and diminutive cleric #93401, while the mobile owner identifies
the tower, intended room placements, and spawn/reset path. This checkout has
no same-stem WLD/ZON, no exact external reset or code reference, and no same-
type duplicate; areas/AREA does not list returns, and areas/world.mob is
absent. Check editor/export sources, generated aggregate inputs, mini sources,
and saved VNUMs before any separate activation, move, or archive proposal.
No migration clearance is implied. See the [returns audit](RETURNS_AUDIT_REVIEW.md),
[20 builder questions](RETURNS_BUILDER_QUESTIONS.md),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-525-returns-two-mobile-prototypes-without-a-map),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-525-returns-isolated-mobile-source-identity),
[candidate dependency row 87](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L88),
and [coverage row 438](ZONE_COVERAGE_REGISTER.csv#L439).

### Checkpoint 526: rf_gh retain five source files pending map and shop owners

Keep areas/wld/rf_gh.wld, areas/mob/rf_gh.mob, areas/obj/rf_gh.obj,
areas/zon/rf_gh.zon, and areas/shp/rf_gh.shp RETAIN IN PLACE. Preserve the
13-room, two-component map and its one-way exits to #111082 and #154153, all
seven mobiles, five objects, 34 reset commands, and the one shop record while
owners resolve the full same-type collision set with listed Torg. Preserve
the five Player Castles object placements and 15 imported carried-object
resets; resolve #53 between active Heavens and unlisted Dalvik sources.

The full-mode shop fallback can include rf_gh.shp when areas/world.shp is
absent. Its keeper #28904, room #28909, and product #28902 numerically map to
Torg's duergar miner, corridor, and axe if the active tables are used. This
is a conditional source-code path, not an observed boot. Confirm shop inputs,
ID ownership, target routes, editor/export history, and saved references
before any separate activation, renumbering, move, merge, or archive
proposal. No migration clearance is implied. See the
[rf_gh audit](RF_GH_AUDIT_REVIEW.md),
[20 builder questions](RF_GH_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md),
[candidate dependency row 88](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L89),
and [coverage row 439](ZONE_COVERAGE_REGISTER.csv#L440).

### Checkpoint 527: rf_op retain four source files pending map and portal owners

Keep areas/wld/rf_op.wld, areas/mob/rf_op.mob, areas/obj/rf_op.obj, and
areas/zon/rf_op.zon RETAIN IN PLACE. Preserve the three-room map, five
authored exits, both portal objects and destinations, and all five ZON
commands while owners resolve the copied rf_gh cathedral fragment and
different unlisted same-number WLD identities in aod_gh, genocide_hall,
and legion.

Confirm whether OBJ #29510 should target either unlisted #29505 definition,
whether OBJ #29511 should target #29512, and why the second portal is not
placed. Resolve the one-way outlet to Duris3 #154153 and the Father Benali
story clue with their owners. No migration or activation clearance is
implied. See the [rf_op audit](RF_OP_AUDIT_REVIEW.md),
[20 builder questions](RF_OP_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-527-rf_op-fragment-and-unlisted-ocean-outlet),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-527-rf_op-reuses-a-cathedral-fragment-without-replacement-proof),
[candidate dependency row 89](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L90),
and [coverage row 440](ZONE_COVERAGE_REGISTER.csv#L441).
### Checkpoint 528: scrubs_hall retain map and shop source

Keep areas/wld/scrubs_hall.wld, areas/mob/scrubs_hall.mob, the tracked
empty areas/obj/scrubs_hall.obj, areas/zon/scrubs_hall.zon, and
areas/shp/scrubs_hall.shp RETAIN IN PLACE. Preserve the ten-room map, 19
authored exits, four mobile prototypes, all 17 reset commands, and the one
shop record. The September 2026 gathering-bag edit confirms recent source
maintenance but does not establish full-area membership.

Resolve the different active Torg WLD identities and the other unlisted
#29100–#29109 maps; confirm the intended target and return for Duris3
#156119. The full-mode shop fallback may include the SHP record, but its
keeper #29103 has no active same-type MOB and its in_room #29106 maps to
Torg. The room-special assignment to #29103 also resolves to Torg's escape
tunnel. Preserve the twelve imported object rows and resolve #6089 and #53
with the shop and item owners. No migration or activation clearance is
implied. See the [scrubs_hall audit](SCRUBS_HALL_AUDIT_REVIEW.md),
[20 builder questions](SCRUBS_HALL_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-528-scrubs-hall-map-and-shop-fallback),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-528-scrubs_hall-room-mobile-overlaps-and-recent-shop-updates),
[candidate dependency row 90](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L91),
and [coverage row 441](ZONE_COVERAGE_REGISTER.csv#L442).
### Checkpoint 529: shadow_hall retain guildhall source

Keep areas/wld/shadow_hall.wld, areas/mob/shadow_hall.mob, the tracked
empty areas/obj/shadow_hall.obj, and areas/zon/shadow_hall.zon RETAIN IN
PLACE. Preserve its three rooms, five exits, one mobile prototype, six ZON
commands, and two typed Player Castles object imports.

Resolve room #30100–#30102 against active Underworld3 and unlisted
cabal_hall/nah sources, and confirm the one-way D4 outlet to unlisted Duris3
#151052. The two O resets currently identify active Player Castles objects
#11110 and #11060; preserve them as typed external dependencies. No
migration or activation clearance is implied. See the
[shadow_hall audit](SHADOW_HALL_AUDIT_REVIEW.md),
[20 builder questions](SHADOW_HALL_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-529-shadow_hall-underworld3-number-collision),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-529-shadow_hall-and-underworld3-room-identity),
[candidate dependency row 91](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L92),
and [coverage row 442](ZONE_COVERAGE_REGISTER.csv#L443).

### Checkpoint 530: shady2 preserve the MOB variant

Keep `areas/mob/shady2.mob` RETAIN IN PLACE and separate from active
`areas/mob/shady.mob`. Preserve all 90 overlapping mobile records and their
different stat/mechanical blocks; five keyword lists and the #97580
descriptions also vary. The listed ZON, SHP, QST, and C/C++ consumers
resolve to the active Shady prototypes, but private editor, export,
deployment, and persisted-state consumers remain unknown. Resolve source
ownership and the purpose of this separate file before any merge, activation,
move, rename, or archive proposal. No migration clearance is implied. See
the [shady2 audit](SHADY2_AUDIT_REVIEW.md),
[20 builder questions](SHADY2_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-530-shady2-mob-only-source),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-530-shady2-shared-mobile-vnums-are-distinct-records),
[candidate dependency row 92](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L93),
and [coverage row 443](ZONE_COVERAGE_REGISTER.csv#L444).

### Checkpoint 531: sky_gh retain map, shop, and imports

Keep areas/wld/sky_gh.wld, areas/mob/sky_gh.mob, the tracked empty
areas/obj/sky_gh.obj, areas/zon/sky_gh.zon, and areas/shp/sky_gh.shp RETAIN
IN PLACE. Preserve its five-room graph, nine WLD exits, 18 ZON commands,
two Player Castles object resets, 14 imported `G` rows, and one shop record.
Resolve the different Harrow room/mobile identities and unlisted Dark and
Infernal duplicates; confirm the one-way outlet to Duris3 #153760; determine
the intended #29403 inn and shopkeeper identities and #29404 shop room; and
resolve missing OBJ #6089 plus alternate #53 stock identities. Keep the
September 2026 #491–#494 edits and their history while the owner confirms
tool/export inputs and full-mode fallback behavior. No activation, move, or
migration clearance is implied. See the [sky_gh audit](SKY_GH_AUDIT_REVIEW.md),
[20 builder questions](SKY_GH_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-531-sky_gh-graph-and-harrow-number-resolution),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-531-sky_gh-and-harrow-competing-identities),
[candidate dependency row 93](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L94),
and [coverage row 444](ZONE_COVERAGE_REGISTER.csv#L445).

### Checkpoint 532: solstice_gh retain map, shop, and imports

Keep areas/wld/solstice_gh.wld, areas/mob/solstice_gh.mob,
areas/obj/solstice_gh.obj, areas/zon/solstice_gh.zon, and
areas/shp/solstice_gh.shp RETAIN IN PLACE. Preserve the ten-room seasonal
map, 19 authored exits, 31 reset commands, three Player Castles O imports,
22 matching G/shop products, and one SHP record. Resolve the current Torg
room identities and unlisted map/mobile/object overlaps; confirm the one-way
Duris3 #154578 outlet, `inn` binding on #29103, and the unlisted
Afterlife #29101 QST boundary. Identify missing object #1514 and wrong-type
#8336, then confirm #108/#109 Mavulsk alternatives and the #491–#494 bag
edit history. Because areas/world.shp is absent, full-mode fallback can
include the candidate shop; keeper #29100 has no active MOB prototype and
`assign_the_shopkeepers()` indexes the resolved keeper without a local
negative-index guard. This is a conditional static hazard, not a confirmed
shop boot. Complete owner, tooling, and deployment review before any separate
move, activation, merge, or archive proposal. See the
[solstice_gh audit](SOLSTICE_GH_AUDIT_REVIEW.md),
[20 builder questions](SOLSTICE_GH_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-532-solstice_gh-seasonal-map-and-torg-number-resolution),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-532-solstice_gh-map-and-prototype-identities),
[candidate dependency row 94](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L95),
and [coverage row 445](ZONE_COVERAGE_REGISTER.csv#L446).

### Checkpoint 533: sunhall retain map, shops, and imports

Keep areas/wld/sunhall.wld, areas/mob/sunhall.mob, areas/obj/sunhall.obj,
areas/zon/sunhall.zon, and areas/shp/sunhall.shp RETAIN IN PLACE. Preserve
the ten-room pyramid map, 19 authored exits, 25 reset commands, three Player
Castles object imports, 17 matching G/shop products, and one same-key SHP
record. Resolve its active Underworld3 room numbers, separate Bastard map,
and one-way Duris3 #158571 outlet. Preserve the distinct unlisted mobiles
and objects, missing #6089, #53/#108/#109 alternate sources, and September
2026 Heavens bag additions. `sunhall.shp` and `bastard.shp` both use key and
keeper #30201; neither keeper prototype is active, and their room numbers
resolve to different active Underworld3 tunnels. With areas/world.shp absent,
full-mode fallback can read both; `assign_the_shopkeepers()` has no local
negative-index guard after `real_mobile()` fails. Treat this as conditional
static source exposure, not evidence of either shop operating. Complete area,
shop, deployment, and tooling-owner review before any separate move, merge,
activation, or archive proposal. See the [sunhall audit](SUNHALL_AUDIT_REVIEW.md),
[20 builder questions](SUNHALL_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-533-sunhall-pyramid-map-and-underworld3-number-resolution),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-533-sunhall-underworld3-and-bastard-source-identities),
[candidate dependency row 95](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L96),
and [coverage row 446](ZONE_COVERAGE_REGISTER.csv#L447).

### Checkpoint 534: surf retain map, mobiles, reset imports, and variants

Keep areas/wld/surf.wld, areas/mob/surf.mob, the tracked empty
areas/obj/surf.obj, and areas/zon/surf.zon RETAIN IN PLACE. Preserve the
90,000-room source map, 322 mobile prototypes, 492 reset commands, all 145
ambiguous active inbound references, and the distinct surface2011 and
underdark2011 maps. Resolve the three inactive-map outbound targets and
thirteen target records without a current WLD definition. Preserve the Great
Realm, Killing Fields, and Map5 source-only resets, six external G-object
references, one E-object reference, and code bindings #200011/#200012/#200321.
The full-source recount identifies 415 active-room matches among 431 outbound
Surf records; this corrects an older one-record raw-match count but does not
change the separate global curated total of 11,987 overall / 415 from Surf.
Complete source-owner, editor/export, and deployment review before any separate
activation, move, merge, or archive proposal. See the [Surf audit](SURF_AUDIT_REVIEW.md),
[20 builder questions](SURF_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-534-surf-full-source-map-and-ambiguous-variant-identities),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-534-surf-surface2011-and-underdark2011-map-identities),
[candidate dependency row 96](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97),
and [coverage row 447](ZONE_COVERAGE_REGISTER.csv#L448).

### Checkpoint 535: surface2011 retain map and QST

Keep areas/wld/surface2011.wld, areas/mob/surface2011.mob,
areas/obj/surface2011.obj, areas/zon/surface2011.zon, and
areas/qst/surface2011.qst RETAIN IN PLACE. Preserve the 90,000-room,
360,000-exit self-contained map; its 213 MOBs, 76 OBJs, 37 QST givers,
23 QA responses, 145 ambiguous active inbound references, and 20 external
QST object references. Retain the history that areas/AREA listed the source
in 2011 and removed it in a 2013 merge-conflict resolution without a stated
reason. The Sa'Dah story text continues in active Surface QST #500000, but
the requirements and rewards differ and the map has distinct identities.
Resolve the shared Norabi Map title, map owners, editor/export provenance,
and Heavens/Breale/Ship Yards item imports before any separate activation,
move, merge, or archive proposal. See the
[surface2011 audit](SURFACE2011_AUDIT_REVIEW.md),
[20 builder questions](SURFACE2011_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-535-surface2011-full-grid-and-historically-listed-map),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-535-surface2011-and-surf-map-and-story-identities),
[candidate dependency row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L98),
and [coverage row 448](ZONE_COVERAGE_REGISTER.csv#L449).

### Checkpoint 536: surface2011-temp retain WLD and boundary evidence

Keep areas/wld/surface2011-temp.wld RETAIN IN PLACE. Preserve its 90,000
room records and 360,000 internal exits, including 60,004 VNUMs that identify
different room records from active Surface. The 602 source-only directions
from 600 rooms absent in the active WLD set are already represented in the
global source-only census and exactly match newbiemaps source/direction/target
tuples. Do not count them as new routes or use 240,016 raw target-number
matches as verified travel. Resolve the shared one-record match #666059,
“temp” provenance, map owners, and editor/export/build role before any
separate movement, merge, activation, or archive proposal. See the
[surface2011-temp audit](SURFACE2011_TEMP_AUDIT_REVIEW.md),
[20 builder questions](SURFACE2011_TEMP_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-536-surface2011-temp-full-map-and-duplicate-boundary-tuples),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-536-surface2011-temp-and-surface-room-identities),
[candidate dependency row 98](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L99),
and [coverage row 449](ZONE_COVERAGE_REGISTER.csv#L450).

### Checkpoint 537: surfacekeeps_objects retain object stub

Keep areas/obj/surfacekeeps_objects.obj RETAIN IN PLACE. It defines 54 object
prototypes #120000–#120053 also defined in active surfacekeeps.obj; 39 bodies
are line-identical and 15 differ. Preserve the candidate ore deposit #120048
against active “rringlor no-show proc 1” and candidate pink-tinted glasses
#120016 against active blue-tinted glasses and their different listed prices.
The file’s January 2010 history says it was added for stub loading; confirm
current loader, editor/export, and record synchronization with its owner
before any move, merge, or archive proposal. Active Alatorin QST/story/ZON
references, current SurfaceKeeps resets, code bindings, and Chaos kit entries
resolve through active surfacekeeps.obj under the manifest; keep unregistered
brass-old-1’s source-only #120047 reset separate. See the
[surfacekeeps_objects audit](SURFACEKEEPS_OBJECTS_AUDIT_REVIEW.md),
[20 builder questions](SURFACEKEEPS_OBJECTS_BUILDER_QUESTIONS.md),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-537-surfacekeeps_objects-duplicate-prototypes-and-stub-history),
[candidate dependency row 99](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L100),
and [coverage row 450](ZONE_COVERAGE_REGISTER.csv#L451).

### Checkpoint 538: Sylvandawn retain map, reset, shop, and code evidence

Keep the five unlisted Sylvandawn source files RETAIN IN PLACE. Its 327-room
WLD has 103 differently titled room-number collisions with active Temple of
Laduguer, 17 already-reviewed source-only approaches into Temple, and two
unregistered Duris3 targets. Preserve the 305 mobile resets, 142 object-to-
mobile resets, five object-in-room resets, 58 door commands, 26 shop records,
and prior imports documented on candidate row 100. Maintained C++ retains
mobile and room-handler bindings, hometown VNUMs, and guard checks in the
colliding room range. The shop loader also has a static glob fallback that
includes every SHP file when `areas/world.shp` is absent; confirm its intended
use with the shop/build owner. Resolve source ownership, route identity,
unresolved typed VNUMs, editor/export use, and loader behavior before any
activation, move, merge, or archive proposal. See the
[Sylvandawn audit](SYLVANDAWN_AUDIT_REVIEW.md),
[20 builder questions](SYLVANDAWN_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-538-sylvandawn-room-identities-source-only-approaches-and-duris3-exits),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-538-sylvandawn-and-temple-of-laduguer-share-vnums-not-rooms),
[candidate dependency row 100](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L101),
and [coverage row 451](ZONE_COVERAGE_REGISTER.csv#L452).

### Checkpoint 540: tharnadia1 retain old shop references and loader evidence

Keep the 18 shop records in `areas/shp/tharnadia1.shp` RETAIN IN PLACE. Sixteen
have related active Tharnadia shops in a remapped VNUM band, but 73 of the
candidate's 108 distinct product references have no current OBJ definition.
Preserve its active Lizard stock #6007–#6009, Player Castles stock #11010/
#11011, Nomads room references #6242/#6243, and the distinct `tharold` and
`twinkeep` shop-source comparisons. Correctly type the CP164 #6358 hit as
shop #6113's line 561 `in_room` value matching Mist room #6358; it is not an
object product. Normal shop generation selects from `areas/AREA`, while the
non-mini server fallback can glob all `.shp` files when `areas/world.shp` is
unavailable. Confirm intended aggregate, explicit shop bindings, editor
ownership, and persisted state before any activation, co-loading, move, merge,
or archive proposal. See the [tharnadia1 audit](THARNADIA1_AUDIT_REVIEW.md),
[20 builder questions](THARNADIA1_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-540-tharnadia1-is-a-shop-source-with-no-wld-graph),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-540-tharnadia1-records-relate-to-remapped-shops-not-an-archive-decision),
[candidate dependency row 102](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L103),
and [coverage row 453](ZONE_COVERAGE_REGISTER.csv#L454).

### Checkpoint 541: tharold retain separate shop inventory and bindings

Keep all twelve `areas/shp/tharold.shp` records RETAIN IN PLACE. They list 59
distinct product VNUMs, including active Lizard stock #6007–#6009; 55 product
numbers have no current OBJ definition. Nine keeper VNUMs have no active MOB
match, and room locations #6130/#6262/#6267 have no active WLD match. Preserve
the nine currently resolving Lizard/Nomads locations and the separate
Nomads-room shop references #6242/#6243. The full records also occur under
`tharnadia1.shp` with matching keeper/room pairs and are excluded from normal
`areas/AREA` aggregation. The non-mini fallback can glob both files when the
combined shop file is unavailable; duplicated non-roaming matches require a
valid explicit binding to resolve. Confirm the intended aggregate, editor,
persisted shop state, and ownership before any activation, co-loading, merge,
move, or archive proposal. See the [tharold audit](THAROLD_AUDIT_REVIEW.md),
[20 builder questions](THAROLD_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-541-tharold-is-a-shop-only-source-with-no-wld-graph),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-541-tharold-and-tharnadia1-keep-separate-shop-identities),
[candidate dependency row 103](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L104),
and [coverage row 454](ZONE_COVERAGE_REGISTER.csv#L455).

### Checkpoint 539: temple2 retain separate map, prototypes, and quest source

Keep all five unlisted temple2 WLD/MOB/OBJ/ZON/QST files RETAIN IN PLACE.
The source has 89 rooms, 30 mobiles, 31 objects, and two QST contracts; its
zone header and all three prototype families numerically overlap active
Longhollow while the full records and room titles differ. The candidate WLD's
196 exits are source-local; 229 active targets into its shared room band
resolve to Longhollow, leaving zero temple2-specific active ingress. Preserve
the priest-and-Jared rescue-and-letter QST as source evidence without claiming
it is loaded or playable. Keep the unregistered player_guilds #34405 product
ambiguity with existing candidate row 84, not as active temple2 stock. Resolve
direction-code meaning, QST command roles, loader/editor use, and code-owner
questions before any activation, co-loading, merge, move, or archive decision.
See the [temple2 audit](TEMPLE2_AUDIT_REVIEW.md),
[20 builder questions](TEMPLE2_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-539-temple2-graph-stays-distinct-from-active-longhollow),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-539-temple2-and-longhollow-share-vnums-not-source-records),
[candidate dependency row 101](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L102),
and [coverage row 452](ZONE_COVERAGE_REGISTER.csv#L453).

### Checkpoint 542: theurgist class notes and map asset stay in place

Keep `theurgist.wld`, `.mob`, `.obj`, `.zon`, and
`areas/map/theurgist.json` RETAIN IN PLACE. The WLD has four local rooms and
six resolving exits; the three ZON O resets place the `Pets`, `Innates`, and
`Spells` documentation objects, and no M reset places its fourteen candidate
mobiles. Current class code uses a separately owned active Heavens mobile and
class data; low numeric matches do not identify the candidate prototypes. Ask
the source, editor/map, and loader owners about the 2010 non-live notes, the
2025 map asset, and active same-number records before any activation, move,
merge, or archive proposal. See the [theurgist audit](THEURGIST_AUDIT_REVIEW.md),
[20 builder questions](THEURGIST_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-542-theurgist-is-a-four-room-local-map-with-no-candidate-ingress),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-542-theurgist-notes-keep-a-separate-low-vnum-identity),
[candidate dependency row 104](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L105),
and [coverage row 455](ZONE_COVERAGE_REGISTER.csv#L456).

### Checkpoint 543: retain Tiamats Lair while the quest manifest selects its file

Keep all five legacy `tiamats_lair` files RETAIN IN PLACE. The 2012 commit
explicitly replaced the old world-list entry with Azhural, but the current
`qst/AREA` still selects `tiamats_lair.qst`; that 97-line QST is byte-identical
to `azhural.qst` today. Preserve its two contracts and the old source path
until the QST manifest, generated aggregate, deployment, editor, and persisted
quest owners reconcile the split. Do not re-add the old WLD/ZON family to
`areas/AREA` or infer that its 200 local exits are live routes. See the
[Tiamats Lair audit](TIAMATS_LAIR_AUDIT_REVIEW.md),
[20 builder questions](TIAMATS_LAIR_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-543-tiamats-lair-legacy-graph-is-not-active-ingress),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-543-azhural-replacement-and-mixed-legacy-record-identity),
[candidate dependency row 105](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L106),
and [coverage row 456](ZONE_COVERAGE_REGISTER.csv#L457).

### Checkpoint 544: retain Fehlnan pending Torrhan identity and loader review

Keep the six `torr` source files RETAIN IN PLACE. Its WLD graph has 300 rooms,
634 exits, and one source-only outbound edge to active Chasm #15052; the
inactive candidate has no active WLD ingress. The old QST path is absent from
`qst/AREA`, and the normal SHP builder omits the six shop records through
`areas/AREA`. The shop runtime has a source-glob fallback when
`areas/world.shp` is absent, so check the deployed aggregate and loader mode
before treating those shops as inactive. The Fehlnan/Torrhan story relation,
map/editor ownership, and generated/deployed artifacts remain owner questions.
Do not move, archive, merge, or reactivate this source from text similarity
alone. See the [Torr audit](TORR_AUDIT_REVIEW.md),
[20 builder questions](TORR_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-544-torr-is-a-300-room-source-map-with-one-outbound-chasm-edge),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-544-fehlnan-and-torrhan-share-story-evidence-without-a-tracked-replacement),
[candidate dependency row 106](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L107),
and [coverage row 457](ZONE_COVERAGE_REGISTER.csv#L458).

## Checkpoint 545: Tradeskills old retained as an inactive catalog source

Keep `areas/obj/tradeskills.old.obj` **RETAIN IN PLACE** outside the selected
`tradeskills` source stem. It is OBJ-only, has no WLD graph to migrate, and
reuses all 234 VNUMs in the active 293-object file; 16 records differ, including
old #400232's elven bag versus the active turkey innards identity. Preserve the
active file as the current catalog and never combine the duplicate VNUM sets.
No build, generated aggregate, deployment, or editor state was inspected, so
this source-only review makes no archival or path-move decision. See the
[old-catalog audit](TRADESKILLS_OLD_AUDIT_REVIEW.md),
[20 builder questions](TRADESKILLS_OLD_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-545-tradeskills-old-has-no-wld-graph-or-room-ingress),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-545-tradeskills-old-is-a-separate-catalog-copy-with-active-vnum-collisions),
[candidate dependency row 107](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L108),
and [coverage row 458](ZONE_COVERAGE_REGISTER.csv#L459).

## Checkpoint 546: Twinkeep retained pending shop-loader owner review

Keep `areas/shp/twinkeep.shp` **RETAIN IN PLACE**. The normal shop generator
excludes it through `areas/AREA`, but the non-mini runtime loader can glob all
`areas/shp/*.shp` when `areas/world.shp` cannot be opened. The checkout has no
combined `world.shp`; no boot or deployed aggregate was inspected. If that
fallback loads Twinkeep and `tharnadia1`, both #6500 records are roaming for
keeper #6500 and both #6519 records are non-roaming for keeper #6519 at room
#6656. Preserve their different records and the active #6500 keeper/#6656 room
references. Resolve aggregate selection, explicit bindings, editor and
persisted shop owners before activation, merge, move, or archive. See the
[Twinkeep audit](TWINKEEP_AUDIT_REVIEW.md),
[20 builder questions](TWINKEEP_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-546-twinkeep-has-no-wld-graph-or-room-edges),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-546-twinkeep-and-tharnadia1-share-shop-keys-but-differ),
[candidate dependency row 108](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L109),
and [coverage row 459](ZONE_COVERAGE_REGISTER.csv#L460).

## Checkpoint 547: ud retained as a separate map candidate

Keep `areas/wld/ud.wld` and `areas/zon/ud.zon` **RETAIN IN PLACE** outside
the active manifest. The map has 17,462 rooms, 50,291 authored exits, 11 weak
components, and 273 positive exit targets absent from every current WLD file.
The existing 18 active approaches from `mobs_underdark` land on unique ud
rooms and remain unresolved; ud itself has no active or other current WLD
outbound target. Preserve the inactive `surf.zon` M reset of mobile #100116
into ud room #349161 as a separate typed dependency. Do not merge ud with a
similarly named underworld/underdark source or propose a route, move, or
archive until map generation, the `300 300` trailer, ingress intent, and
editor/runtime/persisted ownership are resolved. No WLD source-only approaches
into active areas were added, so eligible totals remain 11,987 overall / 415
from Surf. See the [ud audit](UD_AUDIT_REVIEW.md),
[20 builder questions](UD_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-547-ud-map-graph-and-unregistered-approaches),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-547-ud-has-a-unique-title-and-distinct-wld-identities),
[candidate dependency row 109](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L110),
and [coverage row 460](ZONE_COVERAGE_REGISTER.csv#L461).

## Checkpoint 548: ud-inside retained as a Limbo-numbered placeholder

Keep `areas/wld/ud-inside.wld` and `areas/zon/ud-inside.zon` **RETAIN IN
PLACE** outside both active manifests. The WLD has one blank room #1 and no
exits. Active Limbo owns the number #1, which the code also reserves; nine
active Desert directions to #1 already resolve to Limbo. Fourteen other WLD
references and four ZON room commands come from unregistered source files.
The shared title and byte-identical astral/mtncastl placeholders do not show
that this stub is an alias or replacement. Resolve the editor project, owner,
and persisted identity before any activation, merge, move, or archive. See the
[ud-inside audit](UD_INSIDE_AUDIT_REVIEW.md),
[20 builder questions](UD_INSIDE_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-548-ud-inside-is-a-one-room-limbo-numbered-placeholder),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-548-ud-inside-retains-a-limbo-numbered-placeholder),
[candidate dependency row 110](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L111),
and [coverage row 461](ZONE_COVERAGE_REGISTER.csv#L462).

## Checkpoint 549: ud_map0 retained with source-only boundaries and resets

Keep areas/wld/ud_map0.wld, its empty same-stem MOB/OBJ files, and
areas/zon/ud_map0.zon **RETAIN IN PLACE** outside areas/AREA and
areas/qst/AREA. The map has 2,708 rooms, one weak component, 10,837 authored
exits, and 3,064 positive targets absent from every current WLD source. Preserve
the two active unresolved Shafts references, all six source-only exits to active
maps (including the newly reconciled Ghore #11548 approach), and all 26 M reset
commands. Twenty-three resets name active-area mobile definitions; three
prototype IDs remain unresolved. The #26225 reset is a source-only use of the
active Moonshae mobile and separately shares a number with a Moonshae object.
The new Ghore tuple raises curated inactive-WLD totals to 11,988 overall / 415
from Surf. Do not activate, merge, regenerate, move, or archive before map/editor
ownership, reset identity, route orientation, deployment, runtime, and persisted
room identity are resolved. See the
[ud_map0 audit](UD_MAP0_AUDIT_REVIEW.md),
[20 builder questions](UD_MAP0_BUILDER_QUESTIONS.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-549-ud_map0-source-graph-and-boundaries),
[identity](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-549-ud_map0-is-distinct-from-ud_map1-and-ud_map2),
[candidate dependency row 111](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L112),
and [coverage row 462](ZONE_COVERAGE_REGISTER.csv#L463).

## Checkpoint 550: ud_map1 retained with source-only routes and resets

Keep `areas/wld/ud_map1.wld`, its empty same-stem MOB/OBJ files, and
`areas/zon/ud_map1.zon` **RETAIN IN PLACE** outside both active manifests. The
2,425-room WLD has 9,705 authored exits: 8,611 local, seven source-only exits
to active WLDs, and 1,087 positive targets absent from current WLD sources. One
weak local component includes all rooms, and raw directed source traversal from
#1116916 reaches all 2,425; this does not establish runtime access, safety, or
return. One active Shafts exit to #1116916 remains unresolved. Preserve the
four source-only Underworld #4520 M resets and the seven source-only approaches,
including the newly reconciled north exit to Valinhav #90800. Eligible curated
inactive-WLD totals rise to 11,989 overall / 415 from Surf. Do not activate,
merge, regenerate, move, or archive before map/editor ownership, route intent,
reset ownership, deployment, runtime, and persisted room identity are resolved.
See the [ud_map1 audit](UD_MAP1_AUDIT_REVIEW.md), [20 builder
questions](UD_MAP1_BUILDER_QUESTIONS.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-550-ud_map1-source-graph-and-approaches),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-550-ud_map1-title-and-room-identity),
[candidate dependency row 112](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L113), and
[coverage row 463](ZONE_COVERAGE_REGISTER.csv#L464).

## Checkpoint 551: ud_map2 retained with source-only routes and resets

Keep areas/wld/ud_map2.wld, its empty same-stem MOB/OBJ files, and areas/zon/ud_map2.zon RETAIN IN PLACE outside both active manifests. The 6,155-room WLD has 24,594 authored exits: 22,839 local, 27 to active WLDs, one to ambiguous unregistered #30300, 1,722 positive targets absent from current WLD sources, and five negative targets. Nine weak components include a 6,146-room largest component. One active Shafts exit and five inactive-WLD exits target this source. Preserve all 27 source-only exits to active WLDs, including four newly reconciled approaches to Llzazan, Valinhav, Clawed Caverns, and Truk. The ZON contains 268 M commands: 267 request active Underworld mobile prototypes, and one requests undefined mobile #44230. Curated source-only totals rise to 11,993 overall / 415 from Surf; the reviewed ud_map2 subset is 27. Do not activate, merge, regenerate, move, or archive before map/editor ownership, route intent, reset ownership, deployment, runtime, and persisted room identity are resolved. See the [audit](UD_MAP2_AUDIT_REVIEW.md), [25 builder questions](UD_MAP2_BUILDER_QUESTIONS.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-551-ud_map2-source-graph-and-boundaries), [identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-551-ud_map2-title-and-room-identity), [candidate dependency row 113](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114), and [coverage row 464](ZONE_COVERAGE_REGISTER.csv#L465).


## Checkpoint 552: underdark2011 retained with ambiguous map identities

Keep all five underdark2011 source files RETAIN IN PLACE outside areas/AREA and areas/qst/AREA. The 15,482-room WLD has 61,931 exits: 50,697 local, ten to active WLD destinations, and 11,224 to unregistered WLD identities. Preserve the ten already-counted source-only approaches to IceCrag, Divhome, Herders, Grave, Githzerai Stronghold, Troll Caves, Faang, and Kvarkpass Connector; the two links to unlisted Jungle; and all eleven active-source exits whose target ownership remains ambiguous among Surf, Surface2011, and UnderDark2011. Keep the 68 duplicated mobile IDs, eight Surface2011-duplicated object IDs, candidate QST, empty reset file, and #200000–#289999 room identities. Do not activate, merge, regenerate, move, or archive before map-editor and prototype ownership, route identity and safe returns, quest packaging, generic code-range behavior, deployment, runtime, and persisted identity are resolved. Curated global source-only totals remain 11,993 overall / 415 from Surf; these ten exits were already counted. See the [audit supplement](UNDERDARK2011_AUDIT_REVIEW.md), [30 builder questions](UNDERDARK2011_BUILDER_QUESTIONS.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-552-underdark2011-source-graph-and-boundaries), [identity](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-552-underdark2011-map-and-prototype-identity), [candidate row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L115), and [coverage row 465](ZONE_COVERAGE_REGISTER.csv#L466).

## Checkpoint 553: underworld2 retained with unique rooms and source-only edges

Keep `areas/wld/underworld2.wld` and `areas/zon/underworld2.zon`
**RETAIN IN PLACE** outside `areas/AREA` and `areas/qst/AREA`. Its 8,256
unique rooms have 32,603 authored exits: 32,592 local and eleven source-only
directions to active WLD owners. No active or unregistered WLD source points
into its room IDs. Preserve all eleven one-way approaches, including the
already-recorded #417529 north exit to Fort Boyard #38100; the curated total
stays 11,993 overall / 415 from Surf. Keep the eleven singleton strongly
connected components, empty room descriptions, reset-free ZON, and unique room VNUMs
with this map. Do not activate, merge, regenerate, move, or archive before
map/editor ownership, intended routes and returns, range/title metadata,
generic code behavior, deployment, runtime, and persisted identity are
resolved. See the [audit](UNDERWORLD2_AUDIT_REVIEW.md), [30 builder
questions](UNDERWORLD2_BUILDER_QUESTIONS.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-553-underworld2-source-graph-and-boundaries),
[identity](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-553-underworld2-retains-a-distinct-map-identity),
[candidate dependency row 115](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L116),
and [coverage row 466](ZONE_COVERAGE_REGISTER.csv#L467).


## Checkpoint 554: Venom GH retained with shared VNUM and typed dependencies

Keep `areas/wld/venom.wld`, `areas/mob/venom.mob`, the empty
`areas/obj/venom.obj`, and `areas/zon/venom.zon` **RETAIN IN PLACE** outside
both manifests. The ten-room WLD has 16 exits: 15 local and one up from #30300
to unregistered ud_map2 #1124260. Preserve ud_map2 #1126542 down to shared
#30300, which numerically pairs with the BB GH outlet but cannot distinguish
BB GH, Venom, or VoDs GH. Keep the two imported Player Castles O resets, four
Venom M commands, the distinct 10-room/3-mobile identities shared by VNUM with
bb_gh and vodgh, the source-only Sunhall portal target, two unregistered shop
records, and the compiled #30303 inn binding. No active WLD ingress or active
outbound destination exists, and this adds no active-area approach to the
11,993 overall / 415 from Surf total. Do not activate, merge, regenerate,
move, or archive before map/guild ownership, source-editor authority, reset
and prototype identity, boundary/portal/shop targets, deployment, runtime,
and persisted identity are resolved. See the [audit](VENOM_AUDIT_REVIEW.md),
[30 builder questions](VENOM_BUILDER_QUESTIONS.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-554-venom-gh-source-graph-and-boundaries),
[identity](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-554-venom-gh-and-two-guildhalls-share-numbers-not-identity),
[candidate dependency row 116](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L117),
and [coverage row 467](ZONE_COVERAGE_REGISTER.csv#L468).

## Checkpoint 555: VoDsGH retained with shared identities and typed references

Keep `areas/wld/vodgh.wld`, `areas/mob/vodgh.mob`, `areas/obj/vodgh.obj`,
`areas/zon/vodgh.zon`, and `areas/shp/vodgh.shp` **RETAIN IN PLACE** outside
`areas/AREA` and `areas/qst/AREA`. Preserve the 14 rooms and both disconnected
graph components, the two source-only up exits, and the ud_map2 #1126542 down
target to shared #30300. All 14 room VNUMs overlap BB GH; the first ten also
overlap Venom, and no full room blocks match. Preserve the five Player Castles
object imports, nine M resets, two type-25 portal objects, #30301 shop at
#30306, the unresolved #6089 item, and shared #30303 inn binding. The Magma
#142034 exit was counted in Checkpoint 145; the current 11,993 overall / 415
from Surf source-only tally does not change. Do not activate, merge,
regenerate, move, or archive before map ownership, route/return intent,
prototype and shop identity, editor authority, deployment, runtime, and
persisted identity are resolved. See the [audit](VODGH_AUDIT_REVIEW.md),
[30 builder questions](VODGH_BUILDER_QUESTIONS.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-555-vodgh-source-graph-and-boundaries),
[identity](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-555-vodgh-shares-room-and-prototype-numbers-with-two-guildhalls),
[candidate dependency row 117](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L118),
and [coverage row 468](ZONE_COVERAGE_REGISTER.csv#L469).

## Checkpoint 556: void_gh retained with source-only boundaries and shared identities

Keep `areas/wld/void_gh.wld`, `areas/mob/void_gh.mob`,
`areas/obj/void_gh.obj`, `areas/zon/void_gh.zon`, and
`areas/shp/void_gh.shp` **RETAIN IN PLACE** outside `areas/AREA` and
`areas/qst/AREA`. Preserve its 19 rooms, four local WLD components, three
source-only Duris3 exits, all same-number conflicts with active Torg and other
guildhall sources, nine Player Castles O imports, seven local object resets,
seventeen M commands, fifteen G/shop stock references, and raw #29103/#29116
inn/invasion code bindings. The missing shop stock prototype #6089, shared
stock object #53, recent Heavens imports, four local type-25 destinations,
conditional SHP fallback behavior, absent active keeper prototype #29101, and
editor/runtime/persisted identity all remain owner questions. The three
boundaries do not reach the active world and do not change the curated 11,993
overall / 415 from Surf totals.

Do not activate, merge, renumber, regenerate, move, or archive before guild
and map ownership, the shared #291 namespace, boundary destinations and
returns, portal behavior, reset/shop prototypes, raw #29103/#29116 code bindings,
SHP build inputs, source editor/export, deployment, runtime, and persisted
identity are reconciled. See the [Void Guildhall audit](VOID_GH_AUDIT_REVIEW.md),
[36 builder questions](VOID_GH_BUILDER_QUESTIONS.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-556-void_gh-source-graph-and-boundary-identities),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-556-void-guildhall-title-history-and-shared-identities),
[candidate dependency row 118](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L119), and
[coverage row 469](ZONE_COVERAGE_REGISTER.csv#L470).

### Checkpoint 557: waterplane retained with identity conflicts

Keep `areas/wld/waterplane.wld`, `areas/mob/waterplane.mob`,
`areas/obj/waterplane.obj`, and `areas/zon/waterplane.zon` **RETAIN IN PLACE**
outside `areas/AREA` and `areas/qst/AREA`, on existing candidate row 120. The
12-room WLD has a ten-room strongly connected core and isolated rooms #12 and
#72; the latter receives all three candidate M resets. Six room VNUMs belong
to active Limbo/Heavens with different source records, all three mobile
VNUMs overlap active Limbo and other unregistered sources, and other legacy
WLDs also reuse the candidate's low room numbers. Preserve the ten previously
counted unique-source Air/Heavens exits and the five Air exit records whose
source IDs overlap active rooms. Do not count the 87 other unregistered WLD
directions to #2, #6–#9, and #72 as Waterplane ingress; every target is
defined locally in its own source file.

No active WLD exit targets a Waterplane-specific room, no active type-25
object destination falls in #1–#72, and no local OBJ, QST, or SHP prototype
contract exists. The active Heavens reset for shared mobile #2 resolves to
the active Limbo prototype, not the candidate Waterplane record. Do not
activate, merge, renumber, regenerate, move, or archive these files until
map/editor ownership, mobile/reset ownership, route/return intent, deployment,
runtime, and persisted identity are resolved. The active `plane_water_one`
area remains a separate contributor. Checkpoint 557 adds no route to the
11,993 overall / 415 from Surf source-only totals. See the [Waterplane
audit](WATERPLANE_AUDIT_REVIEW.md), [24 builder
questions](WATERPLANE_BUILDER_QUESTIONS.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-557-waterplane-candidate-graph-and-active-identities),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-557-water-plane-low-number-identities-and-followed-history),
[coverage row 470](ZONE_COVERAGE_REGISTER.csv#L471), and
[candidate dependency row 119](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L120).

### Checkpoint 558: wlust_gh retained with room, mobile, and shop conflicts

Keep `areas/wld/wlust_gh.wld`, `areas/mob/wlust_gh.mob`,
`areas/obj/wlust_gh.obj`, `areas/zon/wlust_gh.zon`, and
`areas/shp/wlust_gh.shp` **RETAIN IN PLACE** outside `areas/AREA` and
`areas/qst/AREA`, on existing candidate row 120. Preserve the ten authored
rooms and their one-way Duris3 #154553 boundary, both mobile records, three
Player Castles object imports, five M resets, fifteen G/shop stock VNUMs, and
the current shop definition.

Resolve the ten room VNUMs shared with active Torg and four other unregistered
guildhall sources; mobile #29100/#29101 ownership; the Duris3 endpoint and
return; the shopkeeper and room identity; the shared shopkeeper key with
`void_gh`; the conditional missing-`world.shp` fallback; the imported Player
Castles objects; Heavens bag additions; product #53 owner and missing #6089;
and compiled `inn` binding at #29103. No activation, merge, renumbering,
regeneration, move, or archive is supported until source/editor ownership,
deployment, runtime, and persisted identity are resolved. The Duris3 route
does not change the 11,993 overall / 415 from Surf source-only totals.

See the [Wlust audit](WLUST_GH_AUDIT_REVIEW.md), [28 builder
questions](WLUST_GH_BUILDER_QUESTIONS.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-558-wlust_gh-candidate-graph-and-active-identities),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-558-wanderlust-guild-hall-title-room-ids-and-source-history),
[coverage row 471](ZONE_COVERAGE_REGISTER.csv#L472), and
[candidate dependency row 120](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L121).

### Checkpoint 559: ygg retained with room, prototype, and shop conflicts

Keep areas/wld/ygg.wld, areas/mob/ygg.mob, areas/obj/ygg.obj,
areas/zon/ygg.zon, and areas/shp/ygg.shp **RETAIN IN PLACE** outside
areas/AREA and areas/qst/AREA, on existing candidate row 121. Preserve the
thirteen rooms and both local graph components; six mobile prototypes; two
portal objects; seven O, nine M, and eleven G reset commands; five
Player Castles object imports; both SHP records; and the existing story
prose.

Resolve the exact title/header collision with circlegh and the shared
#30401–#30412 room, mobile, and object identities with active Ard’gral.
Resolve the #9 header versus #30400–#30412 conventional-range mismatch with
the area owner; preserve the source values until then.
Confirm the #30400 to SurfaceKeeps #123522 boundary, the #30410 to
unregistered Duris3 #157535 boundary, and both return paths; confirm portal
destinations, reset ownership, imported Player Castles objects, shopkeepers,
stock #53/#6089, and the conditional missing-areas/world.shp fallback. Keep
the active Ard’gral compiled #30401–#30464 spell range and object callbacks
separate. Do not activate, merge, renumber, regenerate, move, or archive
before source/editor identity, deployment, runtime, and persisted identity
are resolved.

The #123522 exit was recorded in Checkpoint 374. The full inactive-WLD table
already includes two ygg source-only records under its numeric counting rule;
the ambiguous #30401 target is not confirmed active access. The Duris3 exit
does not target an active WLD owner. Checkpoint 559 adds no route to the
11,993 overall / 415 from Surf totals. See the
[Yggdrasil audit](YGG_AUDIT_REVIEW.md), [28 builder
questions](YGG_BUILDER_QUESTIONS.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-559-yggdrasil-graph-and-active-identities),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-559-yggdrasil-title-history-and-ardgral-collisions),
[coverage row 472](ZONE_COVERAGE_REGISTER.csv#L473), and
[candidate dependency row 121](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L122).

### Checkpoint 560: Brass and Duris3 source identities

Keep active Brass **DO NOT MOVE**. Its static typed closure records 34
outbound reset references across 14 active owners, 12 foreign shop products,
and the Plane of Fire QST coordinate reference to Brass room #139000. These
are current source dependencies, not candidate migration evidence; the Brass
QST index omission and shop/runtime qualification remain unresolved. See the
[Brass audit](BRASS_AUDIT_REVIEW.md), [active coverage row
37](ZONE_COVERAGE_REGISTER.csv#L38), and the [Heavens](HEAVENS_AUDIT_REVIEW.md)
and [Winterhaven](WINTERHAVEN_AUDIT_REVIEW.md) consumer updates.

Keep candidate row 17, brass-old-1, **RETAIN IN PLACE** with its existing
archive-crosswalk and source-only imports. Active Brass shares 257 room, 47
mobile, and 70 object VNUMs with the archived +100 predecessor; these
different source identities do not authorize renumbering or removal. Keep
Duris3 candidate row 36 **RETAIN IN PLACE** as well: all 357 Brass room IDs
occur in the Duris3 map, but none of the paired titles or complete blocks
matches. The overlap is not an active route or replacement finding. See the
[Brass Old 1 audit](BRASS_OLD_1_AUDIT_REVIEW.md), [Duris3 audit](DURIS3_AUDIT_REVIEW.md),
[candidate row 17](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17), and
[candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

## Checkpoint 777: Breale typed dependencies

Keep active Breale **DO NOT MOVE**. Its static typed closure records three active Heavens object imports, nine active inbound ZON rows from SurfaceKeeps and White Harbor, three active SHP products, and four active QST item commands. The unresolved object #72609 appears in Breale's ZON and SHP sources and remains an owner question; do not infer a substitute. Candidate Nexus56 row 78 remains **RETAIN** with five source-only imports of Breale objects across ZON and SHP; surface2011 row 97 remains **RETAIN IN PLACE** with its QST reference to Breale object #2676. These are not active routes, resets, quests, or shop stock. See the [Breale audit](BREALE_AUDIT_REVIEW.md), [candidate row 78](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79), and [candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L98). No candidate row or world-exit total changed.

## Checkpoint 778: Bloodstone typed dependencies

Keep active Bloodstone Keep **DO NOT MOVE**. Its typed closure records external ZON reset dependencies on Heavens, Verz1, White Harbor, Cityruin, Minizones, and Split Shield; a source-only object #4814 reference into candidate Alterian Wilderness; and unresolved object #3027 in both reset and shop declarations. Bloodstone's QST and structured sidecar also use many White Harbor prototypes. Candidate Alterian Wilderness row 7 remains **RETAIN IN PLACE**. CP446 completes its bounded static graph, history, reset, QST/SHP, and code-reference review; CP778 adds two active `bs.zon` P resets for source-only object #4814. Preserve its distinct source-only routes and #4814 identity without treating the candidate prototype as active. Resolve loader/manifest ownership, runtime availability, editor/deployment state, persisted identity, and content-owner intent before any move or archive decision. Nexus56 row 78 remains **RETAIN**, and Tharnadia1 row 102 remains **RETAIN IN PLACE**. See the [Bloodstone audit](BS_AUDIT_REVIEW.md), [Alterian Wilderness audit](ALTERIAN_WILDERNESS_AUDIT_REVIEW.md#bloodstone-checkpoint-778-active-object-consumer), [Alterian Wilderness candidate row 7](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L8), [Nexus56 candidate row 78](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79), and [Tharnadia1 candidate row 102](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L103). No candidate count, active exit, or gameplay source changed.

## Checkpoint 779: Caer Tannad typed references

Keep active Caer Tannad **DO NOT MOVE**. Its static closure records 78 outbound reset rows across 13 active owners, 74 active inbound ZON rows from four owners, 36 Alatorin QST item commands into local objects, two Caer Tannad sidecar giver references to Tharnadian Ruin, and the active #78510 portal destination to Dracopit. Object #78519 is a local type-29 ITEM_SWITCH with command ID 340 and affected room #78781; it is not a room #340 or Dalvik dependency. See the [Caertannad audit](CAERTANNAD_AUDIT_REVIEW.md), [coverage row 44](ZONE_COVERAGE_REGISTER.csv#L45), and [world-connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-779-caertannad-item_teleport-and-item_switch-fields).

Keep candidate row 92, `shady2`, **RETAIN IN PLACE** and separate from active Shady: Caertannad's four mobile #97550 reset rows resolve to the active manifest owner. No Caertannad dependency on candidate Dalvik row 28 was found. No candidate count, active WLD exit total, or gameplay source changed.

## Checkpoint 780: Centaur Villages typed references

Keep active Centaur Villages **DO NOT MOVE**. Its 193 typed reset rows, five QST givers, 17 QST item commands, and 38 structured sidecar roles all resolve to local prototypes. Five type-29 ITEM_SWITCH objects point to local rooms through value[1]; value[0] command ID 270 is not an edge to candidate Dalvik room #270. No candidate dependency was found and no candidate row or plan changes. See the [Centaur audit](CENTAUR_VILLAGES_AUDIT_REVIEW.md), [coverage row 49](ZONE_COVERAGE_REGISTER.csv#L50), and [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-780-centaur-switch-fields-remain-local).


## Checkpoint 781: Clawed Caverns typed references

Keep active Clawed Caverns **DO NOT MOVE**. Its only external typed reset
dependency is 15 ZON G commands for Heavens object #827 under guardian
#80739; preserve the current Heavens ownership. The local #80747 type-29
ITEM_SWITCH affects local room #80773, and no inbound active/candidate typed
reference or candidate plan dependency was found. Preserve the already
documented `ud_map2` source-only WLD approach with candidate row 113; it is
not active ingress. No candidate row, source-only route disposition, or WLD
exit total changes. See the [Clawed Caverns audit](CLWCVRN_AUDIT_REVIEW.md),
[coverage row 61](ZONE_COVERAGE_REGISTER.csv#L62), and
[ud_map2 candidate row 113](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114).


## Checkpoint 782: Ultarium typed dependencies

Keep active Ultarium **DO NOT MOVE**. Its 11 ZON object-reset imports resolve
to ten Heavens prototypes, and its eight external QST item references resolve
to active Harpyht, Divine Home, Myrabolus, Hunt, Jade, and White Harbor
prototypes. Vehicles and Alatorin actively reset six Cosmic objects; White
Harbor and Myrabolus QSTs and their structured sidecars consume further local
prototypes. Candidate Brass Old 1 row 16 (CSV line 17) remains **RETAIN IN
PLACE** with its two source-only imports of Cosmic mobile #76073 and object
#76064. No candidate row was added or disposition changed; existing candidate
row 16 now records the Cosmic references. WLD exit totals are unchanged. The
coverage shop census is corrected from zero to two local shops. See the
[Cosmic audit](COSMIC_AUDIT_REVIEW.md),
[coverage row 63](ZONE_COVERAGE_REGISTER.csv#L64), and [candidate row 16](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17).


## Checkpoint 783: Court of the Muse typed references

Keep active Court **DO NOT MOVE**. Its only external reset rows are G imports
of Heavens object #359 and White Harbor object #55448. Its one local shop
sells local object #6706; five type-25 teleports have local destinations, and
no active/candidate inbound typed dependency was found. The static QST file
has nine authored contracts but is absent from `qst/AREA`, so do not infer
that they are loaded. No candidate dependency, candidate row, or WLD exit
total changes. The coverage shop census is corrected to one. See the [Court
audit](COURT_AUDIT_REVIEW.md), [coverage row 64](ZONE_COVERAGE_REGISTER.csv#L65),
and [QST index](../../../areas/qst/AREA).

## Checkpoint 784: Crakkaro typed dependencies

Keep Crakkaros Liar **DO NOT MOVE**. Preserve SurfaceKeeps active G reset
of Crakkaro object #87074 and source-only brass-old-1 candidate row 16 G reset
of object #87011. Candidate brass-old-1 remains **RETAIN IN PLACE**; no new candidate row or disposition change; existing candidate row 16 now records this dependency. WLD exit totals are unchanged. These declarations do not prove live
stock, reset admission, or runtime access and do not grant migration clearance.
See the [Crakkaro typed audit](CRAKKARO_AUDIT_REVIEW.md),
[coverage row 65](ZONE_COVERAGE_REGISTER.csv#L66),
[SurfaceKeeps audit](SURFACEKEEPS_AUDIT_REVIEW.md#checkpoint-784-crakkaro-blue-potion-reset),
and [candidate row 16](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17).

## Checkpoint 785: Delwyn typed dependencies

Keep active Delwyn **DO NOT MOVE**. Preserve the reciprocal Surface gate
and the previously counted source-only Nexus56 WLD approach to #83006.
The candidate’s six D reset-room fields and seven M placement-room fields at
#83000–#83006 are source-local Nexus56 room IDs that overlap Delwyn VNUMs;
they do not establish active Delwyn resets or another approach. Keep the
existing Nexus56 candidate **RETAIN** plan and current disposition. No new
candidate row is added or disposition changes; existing candidate row 78 now
records these reset-room fields. WLD exit totals remain unchanged. See the [Delwyn typed audit](DELWYN_AUDIT_REVIEW.md),
[coverage row 70](ZONE_COVERAGE_REGISTER.csv#L71),
[Nexus56 candidate row 78](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79),
and [Nexus56 audit](NEXUS56_AUDIT_REVIEW.md#checkpoint-785-delwyn-room-reset-fields-in-nexus56).

## Checkpoint 786: Desolate typed dependencies

Keep active Desolate **DO NOT MOVE**. Preserve the active import of Vehicles
object #22237, Mira's two QST/sidecar results using Desolate objects #22279
and #22280, its own Myrabolus letter/Balancer references, and its reset/shop
imports from Heavens, Mira, Verz1, and Divine Home. Preserve the three
unresolved local stock/reset IDs #6070/#6109/#6110 and all three local shop
records. No candidate dependency or disposition changes, and WLD exit totals
remain unchanged. No replacement object, shop deletion, or move is justified
by this static review. See the [Desolate typed audit](DESOLATE_AUDIT_REVIEW.md),
[coverage row 72](ZONE_COVERAGE_REGISTER.csv#L73), and
[Divine Home coverage row 75](ZONE_COVERAGE_REGISTER.csv#L76).

## Checkpoint 787: Divine Home typed dependency closure

Keep active Home of the Divine **DO NOT MOVE**. Its 221 local prototype-reset fields
resolve locally; the 32 native Q/QA contracts name nine active external item VNUMs and
the existing missing reward #31341. The shop census is one record, not zero. Preserve
nine active external reset rows, six external QST item fields, five sidecar fields, four
compiled equipment-item references and two compiled mobile bindings. Cerebusp's #22010
and Divine Home's #40753 declare reciprocal type-25 destinations between #22053 and
#40787. No candidate dependency or disposition changed, and no WLD totals changed. The
existing twenty owner questions remain open and unchanged; see the [Divine Home typed
audit](DIVHOME_AUDIT_REVIEW.md).

## Checkpoint 788: Domain typed closure refresh

Keep active Domain of Lost Souls **DO NOT MOVE**. Its 438 typed local ZON prototype
fields include nine active external imports; its two QST contracts and current
structured story sidecar retain the known Winterhaven #55037/#55038 exchange. No active
outside typed consumer of Domain prototypes or rooms was found. Preserve unregistered
Nexus56's book #36822 reset and shop product on existing candidate row 79 **RETAIN**. No
candidate plan, disposition, or WLD total changes; all twenty Domain owner questions
remain unchanged. See the [Domain audit
supplement](DLSC_AUDIT_REVIEW.md#checkpoint-788-post-sidecar-typed-closure-refresh).

## Checkpoint 789: Drifting Realm typed dependency refresh

Keep active Drifting Realm **DO NOT MOVE**. Its one local shop record (#31310) sells the
local tarot pack #31314, correcting the former zero-shop census. Its four active external
ZON reset sources, Alatorin ingredient import, Arcium portal destinations, current story
sidecar, and CHAOS prototype table references remain typed dependencies, not evidence of
live supply, purchase, travel, or a completed receipt. No candidate row, disposition, or
WLD total changed; the existing twenty owner questions remain unchanged. See the
[audit](DRIFTING_REALM_AUDIT_REVIEW.md#checkpoint-789-shop-and-sidecar-closure-refresh).

## Checkpoint 790: Clan Stoutdorf static typed closure

Keep active Clan Stoutdorf **DO NOT MOVE** and retain the `ud_map2.wld` source-only
approach. The refreshed inventory resolves 301 of 303 local ZON reset commands locally
and records imports #358/#55451; its 303 commands contain 303 prototype VNUM
fields. All 17 local QST item fields and 19 story-sidecar VNUM
values resolve locally. Alatorin's two axe-item commands, sidecar item values, and gauntlet
reset remain typed external consumers. The enabled DwarfSlayer binding is local; the
#34200 board assignment is commented. No duplicate prototype, active shop, local
portal/switch, or external type-25/type-29 room target was found. Runtime stock and
persisted player references were not inspected. No candidate, WLD, area-source, gameplay,
or owner-question change. See the [Stoutdorf
audit](DRST_AUDIT_REVIEW.md#checkpoint-790-current-static-typed-closure).


## Checkpoint 791: Treasure Caves static typed closure

Keep active Treasure Caves **DO NOT MOVE**. Its 184 local ZON prototype-reset commands (188 VNUM fields),
one QST item field, and 39 sidecar VNUM values resolve locally. Preserve active White
Harbor's #93011 item request and sidecar, Tharnadian Ruin's four resets of mobile #93015,
and the local `ITEM_SWITCH` #93023/#93002 fields. Surface Mini's #93011 `completion_key`
strings remain separately classified. Keep unregistered `Mountain.qst` on existing
candidate row 74 outside active QST inputs. No external duplicate prototype, active SHP
product, local shop, or external type-25/type-29 room target was found. Static source review
does not qualify runtime availability or persisted player state. No candidate, WLD,
area-source, gameplay, or owner-question change. See the [Treasure Caves
audit](TREASURE_CAVES_AUDIT_REVIEW.md#checkpoint-791-current-typed-closure-refresh).

## Checkpoint 792: Temple of the Earth static typed census

Keep active Temple of the Earth DO NOT MOVE. Its 278 ZON reset commands carry
285 prototype VNUM operands: 282 local refs, imports of Heavens #360 and
Plane of Earth #23805, and unresolved O reset #43595. Preserve five active
Brass resets of Earth mobile #43540 and five separate source-only
brass-old-1 rows on existing candidate row 17. The four local QST item fields
and 18 story-sidecar typed values resolve locally. Six type-25 destinations
and eleven type-29 room values stay within the Earth WLD. Compiled Chaos,
high-drop, and local procedure bindings are documented in the audit. No
candidate, area-source, WLD, gameplay, or owner-question change was made.
Static source review does not establish live reset, travel, editor, or player
state. See the [Earth typed audit](TEMPLE_OF_THE_EARTH_AUDIT_REVIEW.md).

## Checkpoint 793: Grumbar's Domain static typed closure

Keep active Grumbar's Domain DO NOT MOVE. Its 529 typed ZON prototype operands
resolve to 417 local refs and 112 imports from Heavens, Nyneth, Ravenloft,
Tradeskills, and Plane of Earth; no reset target is unresolved. Preserve the
follower-mobile imports, external object reset groups, and Plane of Earth
mobile rows. The local QST has ten shard-item refs to Tradeskills #400104;
the sidecar has one typed #400104 item value. Active tradeskills.obj owns the
current same-type source, while unregistered tradeskills.old.obj overlaps
five imported VNUMs and remains a separate source-only component. The local
type-25 crevice and type-29 wall switch target Earthp rooms. No active
external typed consumer, shop product, duplicate prototype, or inbound
type-25/type-29 room target was found. No candidate, area-source, WLD,
gameplay, or owner-question change was made. Runtime, editor, and persisted
state remain unqualified. See the
[Grumbar's Domain typed audit](GRUMBARS_DOMAIN_AUDIT_REVIEW.md).

## Checkpoint 794: Elemental Groves static typed closure

Keep active Elemental Groves DO NOT MOVE. Its 110 prototype-reset commands
contain 118 typed VNUM operands: 113 local and five O imports from
Caertannad #78455 (two rows), Heavens #358/#750, and White Harbor #55450.
The local QST's four item fields and all 29 story-sidecar typed values resolve
locally. Preserve the existing Caertannad consumer note and these current
cross-area object owners. No outside QST, ZON, SHP, story-sidecar, duplicate
prototype, or type-25/type-29 room target was found. The raw SHP #3902 values
remain room-number collisions, not local object products. Local code binds
#3833 to glades_dagger. No candidate, area-source, WLD, gameplay, or
owner-question change was made. Runtime, editor, and persisted state remain
unqualified. See the
[Elemental Groves typed audit](ELEMENTAL_GROVES_AUDIT_REVIEW.md).

## Checkpoint 795: Abandoned Elven Homestead static typed closure

Keep active Elvish DO NOT MOVE. Its 118 prototype-reset commands contain 134
typed VNUM operands, all resolving to local prototypes. The 14 QST item
fields and 20 story-sidecar typed values also resolve locally. Preserve the
Solonar reciprocal item-travel pair: Elvish #35804 to room #30648 and Solonar
#30604 to Elvish room #35810. The other two Elvish type-25 destinations stay
within room #35800. One external type-25 declaration targets Elvish, and no
type-29 target does. CHAOS starter tables reference local object #35808. No
external reset, QST, story, shop, or duplicate prototype consumer was found.
No candidate, area-source, WLD, gameplay, or owner-question change was made.
Runtime availability, transport success, editor state, and persisted identity
remain unqualified. See the [Elvish typed audit](ELVISH_AUDIT_REVIEW.md).

## Checkpoint 796: Faang static typed closure

Keep active Faang DO NOT MOVE. Its 442 typed ZON operands resolve to 426 local
references and 16 active Heavens/Verz1 object imports. Preserve the active
inbound reset references from Heavens, Alatorin, Malch, and Khildarak; the
Khildarak shop product; and Icecrag's QST and sidecar cuisine-item input.
Faang's four type-29 switches target local rooms, with no external type-25 or
type-29 destination into the area. No source-only typed consumer, duplicate
mobile/object prototype, candidate disposition, WLD total, gameplay, or owner
question changed. Runtime, editor/tooling, and persisted identity remain
unqualified. See the
[Faang typed audit](FAANG_STORY_REVIEW.md#checkpoint-796-current-typed-census-refresh).

## Checkpoint 797: Fields Between static typed closure

Keep active Fields Between DO NOT MOVE. Its 264 typed reset operands include
two object imports from Heavens and White Harbor; all QST item fields and local
story-sidecar references resolve locally. Preserve Juiblex's optional brewer
recipe using Fields Between manuscript #71005 and banana clump #71016 to
produce fez #71024, plus Scorched Valley's Grog-head input #71009. Four local
type-25 destinations stay within Fields Between; no external type-25/type-29
destination reaches its rooms. Chaos tables reference local objects #71025
and #71028. This static census changes no candidate disposition, WLD total,
area source, gameplay, or owner question; runtime, editor, and persisted-player
state remain unqualified. See the
[Fields Between typed audit](FIELDS_BETWEEN_AUDIT_REVIEW.md#checkpoint-797-current-typed-census-refresh).

## Checkpoint 798: Charcoal Palace static typed closure

Keep active Firep DO NOT MOVE. Its 260 typed reset operands include 257 local
references and outside objects #61, #92121, and #359; preserve the #61 source
ambiguity across Dalvik, Heavens, Hostel, and Mavulsk, with the other objects
owned by Undermountain and Heavens. Preserve Airp's active cloak #88304 input,
Brass-old-1's source-only reset of #88322, and Firep's QST/story-sidecar reward
reference to Winterhaven object #55270. Two local type-29 items target Firep
rooms and no external type-25/type-29 destination reaches it. The source scan
also records CHAOS equipment, compiled mobile callbacks, #88327's object
binding, and the stiletto combat check. This static refresh changes no
candidate disposition, WLD total, area source, gameplay, or owner question;
runtime, editor, and persisted-player state remain unqualified. See the
[Charcoal Palace typed audit](FIREP_AUDIT_REVIEW.md#checkpoint-798-current-typed-census-refresh).

## Checkpoint 799: Altar of the Firesworn static typed closure

Keep active Firesworn DO NOT MOVE. Its 95 typed reset operands and all seven
QST item fields resolve to local prototypes. Preserve Heavens' active P reset
of local book #135129 in bookcase #40. The local story sidecar's typed roles
resolve locally; no outside QST item/giver, SHP product, or story-sidecar
reference targets the altar. Two local type-25 portals target rooms #135157
and #135171 across the two local WLD components; no local type-29 or external
type-25/type-29 destination reaches the area. No source-only reset consumer,
same-type duplicate, or direct C/C++ VNUM reference was found. This static
review changes no candidate disposition, WLD total, area source, gameplay, or
owner question; runtime and persisted-player state remain unqualified. See the
[Firesworn typed audit](ALTAR_OF_THE_FIRESWORN_AUDIT_REVIEW.md#checkpoint-799-current-typed-census-refresh).

## Checkpoint 800: Fisherman's Wharf static typed closure

Keep active Fisherman's Wharf DO NOT MOVE. Its 113 reset operands include one
Dream skull #31320 import; the other 112 are local. Preserve Winterhaven's
active reset and shop product for Wharf fishing pole #88903, plus Newhaven's
active pipe consumer and sidecar references to jelly #88909 and eager
fisherman #88907. The local shop declares cast net #88906, pole #88903, and
paddle boat #88901. CHAOS arrays and fishing code also reference local Wharf
objects. No source-only reset/QST consumer, external duplicate prototype, or
type-25/type-29 destination into Wharf was found. This static review changes no
candidate disposition, WLD total, area source, gameplay, or owner question;
runtime, shop availability, and persisted-player state remain unqualified. See
the [Wharf typed audit](FISHERMANS_WHARF_AUDIT_REVIEW.md#checkpoint-800-current-typed-census-refresh).

## Checkpoint 801: Flind current typed closure

Keep active Flind DO NOT MOVE. Its 113 reset mobile/object operands and all
explicit reset room operands resolve locally. Preserve the six local portal
placements and the compiled #3404 room targets, along with the reciprocal
Underdark boundary and the non-overlapping source-only ud_map2 approach. No
active or source-only outside reset, QST, SHP, typed sidecar, duplicate
prototype, or type-25/type-29 target into Flind was found. This static review
does not change its WLD total, candidate disposition, or owner questions;
runtime and persisted-player state remain unqualified. See the
[Flind typed audit](FLIND_AUDIT_REVIEW.md#checkpoint-801-current-typed-dependency-census).

## Checkpoint 802: Foggy Woods current typed closure

Keep active Foggy Woods DO NOT MOVE. Preserve its active Icecrag wine inputs,
Alatorin armor-service output, and Verspin gold reset, plus the source-only
Nexus56 resets and shop products for Foggy-owned objects. Keep the local
#90004 monocle binding and room range, #90010 switch binding and exit target,
three room-warning callbacks, and existing Duris3 candidate classification.
No source-only reference becomes active through this static review. No WLD
total or candidate disposition changed; live callbacks and persisted-player
state remain unqualified. See the
[Foggy typed audit](FOGGY_WOODS_AUDIT_REVIEW.md#checkpoint-802-typed-census-and-compiled-bindings).

## Checkpoint 803: Forgotten Forest current typed closure

Keep active `forgotten_forest` DO NOT MOVE. Preserve the four local food
contracts and their sidecar item mappings, plus Chaos and enhanceable druid
equipment references to local object #82707. Preserve the reciprocal Surface
boundary and 15 non-overlapping source-only Surf approaches with existing
candidate row 97. No outside ZON, QST, SHP, typed story-sidecar, WLD key,
duplicate-prototype, or type-25/type-29 destination reference into Forest was
found. No WLD total or candidate disposition changed; supply, settlement,
installed equipment, and player access remain unqualified. See the
[Forest typed audit](FORGOTTEN_FOREST_AUDIT_REVIEW.md#checkpoint-803-current-typed-census-refresh).

## Checkpoint 804: Fortb typed item routes and map retention

Keep Fortb's active WLD graph and its paired Surface/Prisonb boundaries as
reviewed. Its local brush #38143 declares a CMD_PUSH item-teleport target to
Duris3 #149188, a room in the unregistered candidate map. Prisonb plank #43023
separately declares CMD_JUMP to Fortb #38106. Preserve both item declarations
for owner review, while keeping them outside WLD exit counts. The Fortb brush
target does not change Duris3's 26 active-source WLD exits or candidate-only
room counts. Keep Duris3 candidate row 36 **RETAIN IN PLACE** pending owner
review; do not activate, archive, rename, or merge it based on this typed item
target. See the [Fort Boyard typed review](FORT_BOYARD_STORY_REVIEW.md#checkpoint-804-current-typed-dependency-census),
[Duris3 typed-reference note](DURIS3_AUDIT_REVIEW.md#checkpoint-804-fortb-item-target),
[Fortb item-link connectivity note](WORLD_CONNECTIVITY.md#checkpoint-804-fortb-item-teleports-stay-outside-wld-totals),
[candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), and
[Duris3 coverage row](ZONE_COVERAGE_REGISTER.csv#L387).

## Checkpoint 805: Ghore typed consumers and current ownership

Keep Ghore's current active source ownership and the source-only Nexus56
references distinct. Preserve the 122 Heavens reset placements, Verz1 object
#3097, active Ghore consumers in Alatorin, Khildarak, Surfacekeeps,
Icecrag, Shipy, and `mobs_underdark`, and Nexus56's three source-only reset
and shop references. Keep the two unresolved mobile operands #11625/#11626
visible for owner resolution. Ghore has no type-25/type-29 portal endpoints;
its two reciprocal WLD key pairs use local key objects. Code-defined room
references and travel candidates do not change its WLD graph or source-only
route totals. No source, registration, or migration disposition changes.

Runtime, editor, persisted identity, and consumer behavior remain open. See
the [Ghore typed audit](GHORE_STORY_REVIEW.md#checkpoint-805-current-typed-dependency-census),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-805-ghore-typed-links-stay-separate-from-wld-exits),
and [coverage row](ZONE_COVERAGE_REGISTER.csv#L103).

## Checkpoint 806: Gibber typed consumers and owner boundaries

Keep active Gibber **DO NOT MOVE**. Preserve Heavens object #359 and Winterhaven
memory #55190 as exact local reset imports; keep the Airp request for local
object #8735 with its typed sidecar mapping. The same-number Roshen mobile is a
separate prototype role. Retain the two local type-29 switch declarations and
the two keyed WLD pairs without converting them into extra routes. The local
switch targets remain in Gibber; no external type-25/type-29 target into its
rooms was found. The source-only ud_map2 approach remains separate
from its active WLD boundaries. No area source or registration disposition
changes; tooling, runtime, and persisted-reference closure remain open.

See the [Gibber typed audit](GIBBERLING_KING_AUDIT_REVIEW.md#checkpoint-806-current-typed-dependency-census),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-806-gibber-switch-and-key-roles-stay-within-the-graph),
and [coverage row](ZONE_COVERAGE_REGISTER.csv#L104).

## Checkpoint 807: Gith_ht typed consumers and owner boundaries

Keep active Gith_ht **DO NOT MOVE**. Preserve the local reset's active Heavens
items #363/#368/#369 and Verz1 counter #3097, plus the unresolved object reset
IDs #19442/#19443. Their same-number local room and mobile identities do not
provide the missing OBJ prototypes. Retain the separate current-source,
unregistered Nexus56 reset and shop references to local objects #19429,
#19434, and #19439 as source-only dependencies; they do not establish active
world use. No outside active reset/shop/QST/story-sidecar consumer or WLD key
reference to Gith_ht objects was found. No area source, registration, or
disposition changes; editor/runtime and persisted-reference closure remain
pending.

See the [Gith_ht typed audit](GITH_HT_STORY_REVIEW.md#checkpoint-807-current-typed-dependency-census),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-807-gith_ht-typed-roles-do-not-add-wld-routes),
and [coverage row](ZONE_COVERAGE_REGISTER.csv#L105).

## Checkpoint 808: Fortress prototype owners remain distinct

Keep active Githyanki Fortress **DO NOT MOVE**. Preserve its Heavens reset
imports #64/#359, the five reciprocal WLD key pairs using local objects, and
the three local objects named by Chaos equipment data. Same-number #64
definitions in unregistered Hostel and Mavulsk object files do not replace
active Heavens ownership. Keep the 276 `Duris3.wld` references in the
source-only component review; they do not create active ingress. No source,
registration, or candidate disposition changes. Runtime/editor/persisted
behavior remains unqualified.

See the [Fortress typed audit](GITHYANKI_FORTRESS_STORY_REVIEW.md#checkpoint-808-current-typed-dependency-census),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-808-fortress-item-roles-stay-separate-from-wld-ingress),
and [coverage row](ZONE_COVERAGE_REGISTER.csv#L106).

## Checkpoint 809: Githzerai prototype ownership stays with its contributors

Keep active Githzerai **DO NOT MOVE**. Preserve its Heavens #360 reset import,
the active Alatorin recipes and Brass stock using Githzerai items, and the
Barovia2/Ravenloft2 reset references. Keep the matching Brass-old-1 reset and
stock source-only with that unregistered contributor. Active Ravenloft2's
object #59079 remains an item-portal target into Githzerai, separate from WLD
edges; the three inactive-WLD approaches remain source-only. No candidate or
area disposition changes. Runtime, access, and persisted owner behavior remain
unqualified.

See the [Githzerai typed audit](GITHZERAI_STRONGHOLD_AUDIT_REVIEW.md#checkpoint-809-current-typed-role-refresh),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-809-githzerai-item-travel-stays-outside-the-wld-boundary-count),
and [coverage row](ZONE_COVERAGE_REGISTER.csv#L107).

## Checkpoint 810: Glacier and Surf ownership remain separate

Keep active Glacier **DO NOT MOVE**. Preserve key #90402 with the two slab
exit fields and the existing authored King Polar/Falina reset relationship.
Keep the one `surf.wld` approach to Glacier #90401 source-only with the
existing Surf candidate; no Glacier candidate row or migration clearance is
added. Active connectivity and curated source-only totals do not change.
Runtime, access, and persisted owner behavior remain unqualified.

See the [Glacier typed audit](GLACIER_AUDIT_REVIEW.md#checkpoint-810-current-typed-dependency-closure),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-810-glacier-key-fields-do-not-change-its-one-way-boundary),
[Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and
[coverage row](ZONE_COVERAGE_REGISTER.csv#L108).

## Checkpoint 811: Goblin Camp remains with its active boundaries

Keep active Goblin Camp **DO NOT MOVE**. Preserve its local dump, bone, sword,
and totem reset roles; retain the reciprocal Surface pair and separate
one-way War Hammers arrival. Keep the `surf.wld` approach with the existing
Surf candidate as source-only; no Goblin Camp candidate row or migration
clearance is added. No WLD or curated source-only totals change. Runtime,
access, and persisted owner behavior remain unqualified.

See the [Goblin Camp typed audit](GOBLINCAMP_AUDIT_REVIEW.md#checkpoint-811-current-typed-dependency-closure),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-811-goblin-camp-item-roles-do-not-add-wld-directions),
[Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and
[coverage row](ZONE_COVERAGE_REGISTER.csv#L109).

## Checkpoint 812: Goblin Cave retains its local contract and route ownership

Keep active Goblin Cave **DO NOT MOVE**. Preserve the local dump and nested
bone resets, its 20 QST item fields, and the sidecar mappings with their local
giver/item roles. Keep Ixarkon object #96402 and its dynamic destination list
with Ixarkon; room #19022 is not a WLD edge or candidate relationship. No
outside typed consumers, inactive-WLD approach, source move, or candidate
disposition change was found. Editor/tooling and persisted/runtime closure
remain pending.

See the [Goblin Cave typed audit](GAGGA_JOBO_CAVE_SYSTEM_AUDIT_REVIEW.md#checkpoint-812-current-typed-dependency-closure),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-812-goblin-cave-item-fields-remain-separate-from-wld-edges),
and [coverage row](ZONE_COVERAGE_REGISTER.csv#L110).

## Checkpoint 813: Moregeeth stays separate from source-only Myranth1

Keep active Moregeeth **DO NOT MOVE**. Preserve its local QST contracts,
sidecar roles, and imported Heavens desk-picks object #412. Keep the five
`myranth1.shp` shop records and its QST as a separately inventoried
source-only component; they reference Moregeeth keeper mobiles and rooms,
while the shop products and QST item values do not resolve as object
prototypes. The conditional shop-loader fallback prevents treating those
records as inert. Retain the existing source-only `surf.wld` approach and
candidate relationship; no Moregeeth candidate or migration clearance is
added. Runtime/editor, persisted-player, and dynamic closure remain pending.

See the [Moregeeth typed audit](MOREGEETH_AUDIT_REVIEW.md#checkpoint-813-current-typed-role-closure),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-813-moregeeth-item-routes-and-key-fields-stay-within-the-graph),
[Myranth1 audit](MYRANTH1_AUDIT_REVIEW.md#typed-item-references-and-missing-prototypes),
[Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and
[coverage row](ZONE_COVERAGE_REGISTER.csv#L111).

## Checkpoint 814: Golden Hall retains its local and inbound item roles

Keep active Golden Hall **DO NOT MOVE**. Preserve its two shop records, the
Heavens products used locally, and the accepted outcome/service objects that
do not have a ZON placement. Retain all active foreign reset consumers,
including Caertannad's six #40444 object resets, as inbound ownership
dependencies. Preserve Ship Yards' QST, sidecar, and Alatorin's shop-product
reference to Golden Hall purple potion #40469. These static typed references
do not add WLD exits or establish stock, access, or live availability. No
candidate or migration clearance is added; editor/runtime, persisted-player,
loading, and indirect/dynamic closure remain pending.

See the [Golden Hall typed audit](GOLDEN_HALL_OF_THE_CROWN_AUDIT_REVIEW.md#checkpoint-814-current-typed-dependency-closure),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-814-golden-hall-item-targets-stay-separate-from-wld-exits),
[Caertannad outbound reset audit](CAERTANNAD_AUDIT_REVIEW.md#outbound-active-reset-references),
and [coverage row](ZONE_COVERAGE_REGISTER.csv#L112).

## Checkpoint 815: Halfcut and Halfcut Hills remain separate sources

Keep active `halfcut` **DO NOT MOVE** and candidate `halfcut_hills`
**RETAIN IN PLACE**. Do not load both sets of shared room, mobile, object, or
QST IDs. Keep the active #27009 crossbow and #27035 epic-teacher bindings,
active sidecar, shop, reset rules, and active Surface return with the listed
source. The candidate
reset closure is local to its unregistered files; its #256031 exit and the
active Surface inbound number #27001 do not establish candidate arrival.
Preserve the unresolved `R I 25000` item reward as a documented source defect
until its owner selects a repair. No migration or activation clearance is
added; editor/deployment and persisted-ID ownership remain open.

See the [Halfcut typed audit](HALFCUT_HILLS_AUDIT_REVIEW.md#checkpoint-815-active-halfcut-typed-closure-and-candidate-boundary),
[candidate source audit](HALFCUT_HILLS_AUDIT_REVIEW.md#source-inventory-and-numeric-ownership),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-815-halfcut-portals-stay-outside-the-shared-candidate-graph),
[candidate row 51](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L51), and
[active coverage row](ZONE_COVERAGE_REGISTER.csv#L116).


## Checkpoint 816: Hall and Casino retain separate source identities

Keep active Hall of the Ancients **DO NOT MOVE** and candidate row 23 `casino`
**RETAIN IN PLACE**. Hall's three imported reset objects, Temple dagger reward,
shop #77724, story sidecar, typed code bindings, and local WLD routes belong to
the active Hall source. Preserve the 12 Casino `D`/`O` room-number operands as
source-only fields whose VNUMs currently identify different Hall rooms. Do not
merge or load the two room sets together, reinterpret the Casino operands as
active Hall dependencies, or change the active exit graph from those numeric
overlaps.

Keep the candidate's two Heavens object #45 imports, door-state questions,
persisted VNUM ownership, and any separate gambling contract open for owner
resolution. Hall's compiled exit-state hooks mean static WLD resolution alone
does not establish access or safe return. No candidate or migration clearance
is added; editor/runtime, persisted-player, indirect/dynamic, and installed
availability remain unqualified.

See the [Hall typed audit](HALL_OF_THE_ANCIENTS_AUDIT_REVIEW.md#checkpoint-816-current-typed-dependency-closure),
[Casino typed cross-check](CASINO_AUDIT_REVIEW.md#checkpoint-816-hall-typed-closure-confirms-casino-room-operand-identity),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-816-hall-item-dependencies-remain-separate-from-the-wld-graph),
[Casino candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L23), and
[Hall coverage row](ZONE_COVERAGE_REGISTER.csv#L117).

### Checkpoint 817: Phlegethos remains an active retained area

Keep `hell_four_phlegethos` **DO NOT MOVE** and preserve all 400 active room
prototypes. Its source graph has no external WLD approach, and the Bronze
Citadel portal is an unreset template with unresolved destination value 99999.
The four rooms outside #52200's directional reach and 86 non-reciprocal exits
are owner questions about authored topology and access; they are not migration
evidence. No source-only candidate or candidate-specific attic row applies, so
none is added. Keep live entry, dynamic travel, return, and safety open until
those paths are independently established.

See the [Phlegethos reachability audit](HELL_FOUR_PHLEGETHOS_AUDIT_REVIEW.md#checkpoint-817-static-reachability-reconciled),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-817-phlegethos-reachability-remains-analysis-seed-bounded),
and [coverage row 121](ZONE_COVERAGE_REGISTER.csv#L122).

### Checkpoint 818: Northern Lakes remains an active area

Keep active `nlakes` **DO NOT MOVE** and preserve its 219 room prototypes and
five reciprocal WLD boundaries. Keep Alatorin's reset of local horse #75229,
local reset/shop stock from active Heavens, and the shared Winterhaven visage
#55287 ownership and quest handoff on their current typed sources. The source
has no unregistered WLD approach, no `nlakes` candidate row, and no separate
source-only map to plan. Do not create a candidate or migration path from
editor/runtime uncertainty; qualify reset/shop/quest availability, current and
boat behavior, and persisted state separately. No migration clearance is
added.

See the [Nlakes typed audit](NLAKES_AUDIT_REVIEW.md#checkpoint-818-typed-dependencies-and-source-ownership),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-818-northern-lakes-item-roles-do-not-add-wld-exits),
[title/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-745-northern-lakes-title-neighboring-settlements-and-tracked-history),
and [coverage row 213](ZONE_COVERAGE_REGISTER.csv#L214).

### Checkpoint 819: Opal and Aopal retain separate identities

Keep active `opalphoenix` **DO NOT MOVE** and candidate row 9 `aopal`
**RETAIN IN PLACE**. The active Enclave of the Opal Phoenix keeps its local
reset and shop prototypes, mobile #70806's skill-teacher binding, and object
#70813/#70814's Chaos-kit roles. Aopal remains the distinct source-only
Tharnadian Causeway, with its documented Shadow Forest, Ancient Bridge, and
Ship Yards approaches; none targets an Opal prototype. Do not merge the stems,
load both maps into one identity, or infer an Opal predecessor from the similar
name. No migration or activation clearance is added; owner/tooling and runtime
qualification remain open.

See the [Opal typed audit](OPAL_PHOENIX_AUDIT_REVIEW.md#checkpoint-819-current-typed-dependency-closure),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-819-opal-typed-roles-do-not-add-wld-edges),
[title/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-747-enclave-of-the-opal-phoenix-title-and-aopal-separation),
[Aopal candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L10), and
[coverage row 222](ZONE_COVERAGE_REGISTER.csv#L223).

### Checkpoint 820: Orrak typed closure preserves candidate ownership

Keep active orrak **DO NOT MOVE**. The only foreign reset consumers of its prototypes are Charing's active G reset and the two source-only Sylvandawn G resets of object #9017; preserve those latter rows with Sylvandawn candidate data row 100 / CSV line 101 **RETAIN IN PLACE**. Preserve Orrak's imported Heavens #358 and White Harbor #55446 objects. Orrak receives no candidate row. Its reciprocal Surface route is unchanged, and the non-overlapping Surf #260629 approach stays on Surf candidate data row 96 / CSV line 97 **RETAIN IN PLACE**. These source links do not identify a replacement map or authorize a move. See the [Orrak typed audit](ORRAK_AUDIT_REVIEW.md#checkpoint-820-current-typed-dependency-closure), [Sylvandawn candidate data row 100](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L101), and [Surf candidate data row 96](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 821: Randomness and Duris3 remain separate retained identities

Keep active `random` **DO NOT MOVE** and add no candidate row. Correct its local SHP inventory to the two source-defined shop headers. Preserve four active Surface resets of Randomness mobile #100323 and keep Surf’s 80 additional source-only reset rows on candidate data row 96 / CSV line 97 **RETAIN IN PLACE**; neither changes Surf’s active WLD-ingress metric. Preserve Duris3 candidate data row 35 / CSV line 36 **RETAIN IN PLACE**: its complete #100000–#109999 room-number overlap has different titles, and its 602 source-only directions into those numbers do not add an active boundary. Keep map22 candidate data row 64 / CSV line 65 separate at #900000–#909999. Randomness’s active object #53 import resolves to Heavens; retain the same-number source-only object files in Dalvik, Hostel, and Mavulsk under each existing **RETAIN IN PLACE** plan. No numeric overlap or source-only reset authorizes activation, renumbering, merging, or a move.

See the [Randomness typed audit](RANDOMNESS_AUDIT_REVIEW.md#checkpoint-821-current-typed-dependency-closure), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-821-randomness-typed-references-do-not-add-active-wld-edges), [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), [Duris3 candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), and [Randomness coverage row](ZONE_COVERAGE_REGISTER.csv#L247).

### Checkpoint 822: Raxthan stays active; its Surf approach stays retained

Keep active `raxthan` **DO NOT MOVE** and add no candidate row. Preserve the three reciprocal active Surface/Underdark boundaries, the Alatorin-owned #42948 receipt, shared CHAOS #42903 and guarded `shift_prime` #42950 references, and the dealer shop on Raxthan’s current sources. Keep inactive Surf #266966 north to Raxthan #42980 on Surf candidate data row 96 / CSV line 97 **RETAIN IN PLACE**; the current 95-file inactive WLD sweep finds no other Raxthan approach or room overlap. The count correction does not change curated source-only totals or authorize activation, renumbering, or a move. See the [Raxthan source audit](YERDONIA_AUDIT_REVIEW.md#checkpoint-822-current-typed-and-wld-source-reconciliation), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-822-raxthan-static-boundaries-and-inactive-source-count), [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [Raxthan coverage row](ZONE_COVERAGE_REGISTER.csv#L250).

### Checkpoint 823: The Orcish Slave Camp stays active

Keep active `shortc` **DO NOT MOVE**. It has no candidate key in the attic dependency register, and the current full inactive-WLD scan finds no camp approach or source-room collision. Preserve its reciprocal Split Shield boundary. The two inactive Surf exits to Split Shield #10300 and #10334 remain on the separate Surf candidate row 97 **RETAIN IN PLACE**; neither target is a camp room. Do not create a `shortc` migration plan, activate either Surf direction, or infer replacement identity from those unrelated routes. The camp's actual access, return, and safety still need runtime qualification.

See the [camp source audit](ORCISH_SLAVE_CAMP_AUDIT_REVIEW.md#checkpoint-823-current-wld-and-typed-source-reconciliation), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-823-orcish-slave-camp-source-totals-reconcile), [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [coverage row 274](ZONE_COVERAGE_REGISTER.csv#L275).

### Checkpoint 824: Stronghold remains active; Surf approach remains retained

Keep active `spshold` **DO NOT MOVE** and add no candidate row. Preserve its three reciprocal Surface pairs and six one-way exits from load room #22649. The only all-95 source-only Stronghold approach remains Surf #285640 west to #22634; retain it on Surf candidate row 97 **RETAIN IN PLACE**. Do not activate that edge or use the unrelated Storm Port and Jade story connections as replacement evidence. The current typed-source refresh adds no migration or activation clearance; service settlement, actual access, and safe return remain separate qualification questions.

See the [Stronghold typed-source refresh](STORM_PORT_STRONGHOLD_AUDIT_REVIEW.md#checkpoint-824-current-typed-source-reconciliation), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-824-stronghold-dependencies-leave-its-wld-graph-unchanged), [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [coverage row 281](ZONE_COVERAGE_REGISTER.csv#L282).

### Checkpoint 825: Strathor stays active; Surf remains source-only

Keep active `stormht` **DO NOT MOVE** and add no candidate row. Preserve its two reciprocal Surface pairs and all six authored one-way exits from load room #22649; the current graph has no source-room collision or unresolved exit. Retain Surf #216305 north to Strathor #30800 on candidate data row 96 / CSV line 97 **RETAIN IN PLACE**. The two active Surface returns and one inactive Surf target are the only exact current WLD references into Strathor; do not activate Surf or infer a replacement map. No migration clearance follows from this static review.

See the [current Strathor audit](STRATHOR_VALLEY_AUDIT_REVIEW.md#checkpoint-825-current-wld-and-typed-source-reconciliation), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-825-strathors-current-wld-closure-reconciles), [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [coverage row 282](ZONE_COVERAGE_REGISTER.csv#L283).

### Checkpoint 826: Storm Port stays active; Surf remains source-only

Keep active `stormport` **DO NOT MOVE** and add no candidate row. Preserve its four reciprocal Surface exits, one-way Sarmiz outbound, Stronghold and Winterhaven inbounds, and unresolved east target to Duris3 #144664. Keep the four non-overlapping inactive `surf.wld` approaches on existing candidate data row 96 / CSV line 97 **RETAIN IN PLACE**. These source-only records are not loaded ingress or replacement evidence. Preserve the distinct Duris3 map and its existing [candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36); do not merge it with active Storm Port. The source closure adds no candidate, changes no curated route count, and grants no activation, ownership, or path-move clearance. See the [Storm Port audit](STORMPORT_AUDIT_REVIEW.md#checkpoint-826-current-source-reconciliation), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-826-storm-ports-current-wld-closure-reconciles), [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [coverage row 283](ZONE_COVERAGE_REGISTER.csv#L284).

### Checkpoint 827: Temple of the Sun remains active; Surf and Brass Old 1 stay distinct

Keep active `suntmpl` **DO NOT MOVE** and add no candidate row. Preserve the reciprocal Surface boundary and the non-overlapping inactive Surf #237595 west approach on existing candidate data row 96 / CSV line 97 **RETAIN IN PLACE**; it is not active ingress. Keep `brass-old-1` **RETAIN IN PLACE** on candidate row 17 with its source-only `G` reset of Temple object #82416. Preserve the active Winterhaven QST/story consumer of ring #82405, Temple resets importing Heavens/Unique objects, and Frolikk's code binding. These are distinct dependencies; the unregistered Brass reset does not create active stock or a new WLD route. No candidate row or curated route count changes, and no activation, renumbering, ownership, or path-move clearance follows. See the [Temple audit](TEMPLE_OF_THE_SUN_AUDIT_REVIEW.md#checkpoint-827-current-source-reconciliation), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-827-sun-temple-graph-remains-reciprocal-source-only-edges-stay-separate), [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), [Brass Old 1 candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17), and [coverage row 284](ZONE_COVERAGE_REGISTER.csv#L285).


### Checkpoint 828: Surface stays active and source-only approaches stay retained

Keep active `surface` **DO NOT MOVE**. Preserve its 423 inbound and 370 outbound active WLD boundary records, the 359 reciprocal outbound records, and the two unresolved authored exits to #33807. Keep the 1,208 non-overlapping inactive approaches on existing candidate rows 12, 29, 55, 78, and 99; the other 478,024 numeric target matches begin at source VNUMs already owned by active rooms and are not added as approaches. Keep `newbiemaps` and `surface2011-temp` as separate identities despite their shared 602-direction tuple family.

Preserve the active external QST, ZON, and story dependencies recorded in the [Surface audit](SURFACE_AUDIT_REVIEW.md#checkpoint-828-current-source-and-completion-signature-reconciliation), including the Surface Mini completion signatures that use Surface-owned prophet heads, rings, and potion objects. Those signatures add no WLD route and do not establish runtime availability. No candidate row, curated approach total, activation, replacement, or path-move clearance changes at CP828. See the [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-828-surface-graph-and-inactive-map-identities-reconcile) and [coverage row 285](ZONE_COVERAGE_REGISTER.csv#L286).

### Checkpoint 829: SurfaceKeeps remains active with its three retained approach identities

Keep active `surfacekeeps` **DO NOT MOVE**. Preserve its six unresolved Duris3 target exits and the existing one-way/reciprocal Alatorin, Tharn Rifts, and White Harbor boundaries. Retain the 1,457 non-overlapping inactive approaches on Duris3 candidate row 36, oldmap8 row 81, and ygg row 122; do not add the 9,131 active-owned-source VNUM matches as separate approaches. Keep the all-2,645-room Duris3 numeric overlap as distinct inactive map evidence, with no predecessor or replacement conclusion.

Retain the separate `surfacekeeps_objects.obj` stub source on candidate row 100 and preserve the source-only Brass Old 1 reset on its existing candidate row. The current typed refresh corrects the local reset inventory to 79 distinct external object VNUMs across 135 object reset records; it adds no route or migration clearance. Keep the ten local shops, active and candidate typed consumers, and the owner-specific dependencies recorded in the [SurfaceKeeps audit](SURFACEKEEPS_AUDIT_REVIEW.md#checkpoint-829-current-source-dependency-and-graph-reconciliation). No candidate row, route total, activation, source identity, or path disposition changes. See the [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-829-surfacekeeps-static-graph-and-source-only-routes-reconcile) and [coverage row 286](ZONE_COVERAGE_REGISTER.csv#L287).

### Checkpoint 830: SurfMini stays active with no candidate route

Keep active `surfacemini` **DO NOT MOVE** and add no attic candidate row. Preserve its eight inbound and eleven outbound active WLD records, the seven reciprocated departures, and the four separate active Surface type-25 arrivals. Keep `surface2011.obj`’s matching portal declarations source-only, and retain distinct map identities for `surfacemini`, `minizones`, `udmini`, and `surface`. The current typed refresh reconciles QST outputs and completion signatures without adding a WLD route, candidate dependency, or migration clearance. See the [SurfMini audit](SURFACEMINI_AUDIT_REVIEW.md#checkpoint-830-current-source-item-roles-and-completion-signatures), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-830-surfmini-wld-boundaries-and-item-routes-remain-separate), and [coverage row 287](ZONE_COVERAGE_REGISTER.csv#L288).

### Checkpoint 831: Swamp One remains in place after typed-source refresh

Keep active `swamp_one` **DO NOT MOVE**. Preserve the reciprocal Surface and Swamp Two boundaries, Swamp Two's 26 mobile and four object reset roles, the three paired Swamp Two key routes, the Caves-owned optional sphere exchange, and the maintained #26014 object binding. Retain inactive `surf.wld` #267604 west to #26001 only as source-only evidence on existing Surf candidate row 97. This source review adds no attic candidate and grants no gameplay or migration change. See the [Swamp One typed-source refresh](SWAMP_ONE_AUDIT_REVIEW.md#checkpoint-831-current-typed-source-roles), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-831-swamp-one-typed-routes-do-not-change-its-wld-graph), and [coverage row 288](ZONE_COVERAGE_REGISTER.csv#L289).

### Checkpoint 832: Swamp Two stays active with shared item routes

Keep active `swamp_two` **DO NOT MOVE**. Preserve its reciprocal Swamp One boundary, 35 room identities, maze routes, three paired keyed doors, and all 26 mobile and seven object reset roles. Keep White Harbor's #55185 request with its OBJ owner and Swamp Two reset, Caves' #26013 exchange with its existing owners, and the Heavens #359 prototype/handler with each recorded reset role, including Smoke. The White Harbor shopkeeper #55185 is a separate MOB identity and is not a product reference to OBJ #55185. No attic candidate or migration change is added. See the [Swamp Two typed-source refresh](SWAMP_TWO_AUDIT_REVIEW.md#checkpoint-832-current-item-routes-and-prototype-roles), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-832-swamp-two-item-routes-remain-separate-from-wld-reachability), and [coverage row 289](ZONE_COVERAGE_REGISTER.csv#L290).

### Checkpoint 833: Tchan remains in place; Alterian routes stay source-only

Keep active `tchan` **DO NOT MOVE**. Preserve its ten room identities, reciprocal Surface boundary, closed monastery door, local guard and key roles, and Storm Port's mobile/request ownership. Retain the four distinct inactive Alterian approaches on existing candidate row 8 and keep the sixteen same-number matches from overlapping Alterian source-room IDs excluded from the approach count. This review adds no candidate approach or migration clearance. See the [Tchan typed-source refresh](TCHAN_AUDIT_REVIEW.md#checkpoint-833-current-typed-source-roles), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-833-tchan-remains-connected-only-by-its-reviewed-surface-boundary), and [candidate row 8](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L8).

### Checkpoint 834: Teka2 stays in place with typed routes preserved

Keep active `teka2` **DO NOT MOVE**. Preserve its 98 room identities, reciprocal North Lakes boundary, typed item-teleport routes, and current fall/current fields. Retain Heavens #358 and White Harbor #55454 with their object owners, Alatorin's two resets of Teka2 blood #75529, and the separate DLSC experience contract. This source refresh finds no inactive approach and adds no attic candidate or migration clearance. See the [Teka2 typed-source refresh](TEKA2_AUDIT_REVIEW.md#checkpoint-834-typed-source-closure-rechecked), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-834-teka2-conditional-item-routes-stay-outside-the-raw-wld-graph), and [coverage row 291](ZONE_COVERAGE_REGISTER.csv#L292).

### Checkpoint 835: Temple stays active with the Surf approach source-only

Keep active Temple **DO NOT MOVE**. Preserve the reciprocal Surface boundary, the separate Phantom `shift_prime` arrival candidate, and the Hall-owned Temple-dagger-to-letter-to-hair item chain. Retain inactive `surf.wld` #263869 south to #18300 only as source-only evidence under the existing Surf classification; do not add a Temple candidate or activate ingress. No migration or gameplay change is cleared by this review. See the [Temple source refresh](TEMPLE_AUDIT_REVIEW.md#checkpoint-835-hall-item-chain-and-conditional-arrival-remain-separate) and [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-835-temple-source-only-surf-exit-and-wld-graph-stay-distinct).

### Checkpoint 836: Keep Temple active and Sylvandawn RETAIN IN PLACE

Keep active Temple **DO NOT MOVE** and preserve its reciprocal Underdark boundary. Keep Sylvandawn candidate row 101 **RETAIN IN PLACE** with its 17 source-only approaches, 206 differently titled same-number transitions, 115 Temple-range room arguments, and eight Temple object product references. These are candidate source identities, not active Temple ingress, reset state, or shop stock. Add no new candidate or migration clearance. See the [Temple source refresh](TEMPLE_LADUGUER_AUDIT_REVIEW.md#checkpoint-836-sylvandawn-dependencies-remain-source-only), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-836-temple-active-connectivity-stays-separate-from-sylvandawn-sources), and [candidate row 101](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L101).

### Checkpoint 837: Tentro2 remains active with Surf approaches source-only

Keep active `tentro2` **DO NOT MOVE**. Preserve its 294 room identities, five inbound records, four outward records, two WLD components, pool portal item route, six ConnectorZones mobile resets, and Verz1 bank object #3097 with its shared storage binding. Retain four inactive Surf approaches only as source-only evidence on candidate row 97. Add no Tentro candidate or migration clearance. See the [Tentro2 typed-source refresh](TENTRO2_AUDIT_REVIEW.md#checkpoint-837-current-reset-bank-and-pool-routes-rechecked), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-837-tentro-item-travel-and-external-resets-remain-separate-from-wld-edges), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 838: Tezcat remains active with source-only approaches retained

Keep active `tezcat` **DO NOT MOVE**. Preserve the paired Surface boundary and
the separate one-way Winterhaven arrival. Keep inactive Surf #265973 and
Underworld2 #429194 only on their existing candidate rows 97 and 116; neither
is active ingress or a Tezcat migration candidate. Keep Kastle's talon quest,
Tharnadia's dagger delivery, the four foreign reset prototypes, and maintained
code bindings with their current owners. The refresh adds no candidate, route
activation, replacement finding, or migration clearance. See the [Tezcat
typed-source refresh](TEZCAT_AUDIT_REVIEW.md#checkpoint-838-current-typed-closure-refresh),
the [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-838-tezcat-typed-routes-remain-outside-the-wld-graph),
and [candidate rows 97 and 116](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 839: Tharnadia remains active; Duris3 stays retained

Keep active `tharnadia` **DO NOT MOVE**. Retain the 604 non-overlapping
Duris3-to-Tharnadia approaches on existing candidate row 36 and exclude the
1,732 same-number directions whose Duris3 source rooms overlap active
Tharnadia room VNUMs. Preserve the unresolved 68-room Caertannad text overlap
as a subarea-copy question, not a whole-zone replacement. The active incoming
Bloodstone OBJ #74203 reset and Kieta shop product remain attached to their
current owners; earlier Lylr #132581 and Arena #67900 inputs remain recorded.
This refresh adds no candidate, activation, move, or migration clearance. See
the [Tharnadia audit](THARNADIA_AUDIT_REVIEW.md#checkpoint-839-bloodstone-input-rechecked-against-current-sources),
the [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-839-tharnadia-item-dependencies-leave-its-wld-graph-unchanged),
and [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

### Checkpoint 840: Tharnadian Ruin remains active with no candidate approach

Keep active `tharnadian_ruin` **DO NOT MOVE**. Its three reciprocal WLD
boundaries remain with Caertannad, Cityruin, and Braddistock Mansion; the
inactive-WLD review found no approach or room overlap, so add no candidate row.
Keep the Braddistock #1375 proof, Caertannad giver references, Dungeon/Plane
mobile imports, Heavens key placements, and Surface item travel attached to
their existing owners. Dynamic item routes do not add ordinary WLD reachability
or migration clearance. See the [Tharnadian Ruin typed-source refresh](THARNADIAN_RUIN_AUDIT_REVIEW.md#checkpoint-840-current-typed-source-closure-refreshed)
and [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-840-tharnadian-ruin-typed-routes-remain-outside-its-wld-graph).

### Checkpoint 841: Rifts stays active; Duris3 remains retained in place

Keep active `tharnrifts` **DO NOT MOVE**. Retain inactive Duris3 **RETAIN IN
PLACE** on candidate row 36, with 1,323 distinct-source approaches into Rifts;
exclude the 21,809 directions that begin in VNUMs already owned by active
contributors. Keep the two active exits into Duris3-only destinations unresolved
and separate from inactive source-only ingress. Preserve the two additional
`bb_gh` and `circlegh` approaches within the existing 1,325 source-only total.
Keep all 62 typed reset imports with Caertannad and Minopass prototype owners;
item travel through #78503 and the silverleaf story reference remain distinct.
No candidate, map activation, or migration clearance is added. See the
[Tharnadia Rifts audit](THARNRIFTS_AUDIT_REVIEW.md#checkpoint-841-current-typed-ownership-and-boundary-refreshed),
the [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-841-tharnadia-rifts-wld-and-reset-edges-stay-distinct),
and [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

### Checkpoint 842: Thetis remains active; preserve Surf and #104 evidence

Keep active `thetis` **DO NOT MOVE**. Retain inactive Surf #251543 north to
#38000 only as source-only evidence on candidate row 97; do not activate Surf
or infer an ordinary inbound route. Preserve #38095 north → #104 as unresolved
between inactive dalvik and mavulsk source records until its intended owner is
established. Keep Thetis QSTs, the Jade map exchange, optional Stronghold story
references, and local shop with their current owners. No candidate, map move,
route activation, or migration clearance is added. See the [Thetis audit](THETIS_AUDIT_REVIEW.md#checkpoint-842-current-typed-closure-and-source-only-ingress-refreshed),
the [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-842-thetis-item-exchanges-remain-outside-its-wld-graph),
and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 843: Thri remains active with its Surf approach source-only

Keep active `thri` **DO NOT MOVE**. Preserve the reciprocal Surface WLD pair and
retain inactive Surf #250600 west to #67600 only on existing candidate row 97.
Keep `surface2011` as a separate source identity; it has no matching approach
exit. Do not create a Thri candidate. Preserve the Heavens reset/shop owners,
local type-25 declarations, and code-bound room references without interpreting
them as an active incoming path or hometown eligibility. No route activation,
area move, or migration clearance is added. See the [Thri source refresh](THRI_STORY_REVIEW.md#checkpoint-843-current-typed-stock-and-room-references-refreshed),
the [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-843-thri-item-routes-and-code-bound-rooms-stay-distinct),
and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 844: Tiamat stays active; brass-old-3 remains retained

Keep active `tiamat` **DO NOT MOVE** and preserve its existing Avernus,
Hell One, and Astral Tiamat WLD directions. Retain `brass-old-3` **RETAIN IN
PLACE** on candidate row 19: Tiamat's unresolved #25106 object reset is disabled
by loader resolution and does not authorize reactivating the candidate source.
Keep Astral Main MOB #19700's prototype, reset, sidecar contact, and code roles
together with their current owner. Astral Tiamat's #19907 target remains an
unbound declaration, not evidence of a working portal. No candidate, activation,
or migration clearance is added. See the [Tiamat audit](TIAMAT_AUDIT_REVIEW.md#checkpoint-844-astral-main-import-and-typed-roles-refreshed),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-844-tiamat-typed-dependencies-leave-its-wld-graph-unchanged),
and [candidate row 19](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L19).

### Checkpoint 845: Lost City stays active with Surf retained

Keep active `tikit` **DO NOT MOVE**. Preserve its reciprocal Surface and Lost
Temple WLD pairs, and retain inactive Surf #248527 east to #43901 only on
existing candidate row 97. Keep `surf` and `surface2011` **RETAIN IN PLACE** as
separate source identities. Maintain the Temple's exact City-object recipe
inputs, its separate #44190 type-25 item route, and Surface Keeps' #43708 reset
with their current owners. No City candidate, route activation, or migration
clearance is added. See the [TikiT audit](TIKIT_AUDIT_REVIEW.md#checkpoint-845-current-cross-area-typed-closure-refreshed),
the [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-845-city-item-contracts-remain-outside-wld-connectivity),
and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 846: Lost Temple stays active with no candidate approach

Keep active `tikitt` **DO NOT MOVE**. Preserve the reciprocal Lost City boundary,
one-way Winterhaven arrival, fifteen City-owned crafting inputs, Heavens OBJ
#360 reset, and separate #44190 type-25 route into City room #43704. The
all-95 inactive-WLD scan found no Temple approach or room overlap; add no
candidate row. Keep Lost City and Lost Temple as separately owned areas, with
no migration clearance. See the [Temple audit](TIKITT_AUDIT_REVIEW.md#checkpoint-846-current-item-inputs-and-ownership-rechecked),
the [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-846-temple-item-travel-remains-separate-from-wld-reach),
and [coverage row 303](ZONE_COVERAGE_REGISTER.csv#L304).

### Checkpoint 847: Torg remains active; source-only and fallback records stay separate

Keep active `torg` **DO NOT MOVE**. Retain inactive Surf #277211 east to #28900
only on existing Surf candidate row 97; the 162 other inactive target matches
begin at VNUMs owned by active Torg rooms and are not source-only approaches.
Keep `brass-old-1` reset references to #28983/#28984 with their existing
candidate ownership, and do not load inactive afterlife portals as active
routes. Preserve the conditional `rf_gh.shp` loader collision with the separate
RF_GH component's RETAIN IN PLACE disposition; startup mode and live binding
remain unqualified. No Torg candidate, source activation, or migration
clearance is added. See the [Torg audit](TORG_AUDIT_REVIEW.md#checkpoint-847-current-typed-owners-and-shop-loader-boundary-refreshed),
the [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-847-torg-typed-dependencies-remain-separate-from-wld-reach),
and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 848: Torrhan remains active with Surf approaches retained

Keep active `torrhan` **DO NOT MOVE**. Preserve its four reciprocal Surface
pairs and one-way Winterhaven administrative arrival. Retain inactive Surf
#212237, #212538, and #212837 approaches only on existing candidate row 97; do
not add a Torrhan candidate. Keep Shipy, Alatorin, Tharnadia, Heavens,
Winterhaven, and inactive shop/reset identities with their current owners.
Preserve the inactive `torr` map and QST story material as separate source
identity evidence, not as a predecessor or replacement. The loader paths and
stock remain unqualified; no map move or migration clearance follows. See the
[Torrhan audit](TORRHAN_AUDIT_REVIEW.md#checkpoint-848-current-typed-ownership-and-shop-records-refreshed),
the [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-848-torrhan-typed-references-remain-outside-its-wld-graph),
and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 849: Torture remains active with no candidate approach

Keep active `torture` **DO NOT MOVE**. Preserve its reciprocal Ghore boundary,
closed internal gate, reset-closed locked hatch/grate, and four fall fields as
authored source state. The all-95 inactive-WLD sweep found no room overlap or
source-only approach, so add no Torture candidate. The stale `qst/AREA` entry
and absent local QST do not authorize new story content or a migration. No
route activation, move, or migration clearance is added. See the [Torture
audit](TORTURE_AUDIT_REVIEW.md#checkpoint-849-current-source-boundary-and-negative-typed-closure-refreshed),
the [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-849-torture-graph-and-ghore-boundary-remain-unchanged),
and [coverage row 306](ZONE_COVERAGE_REGISTER.csv#L307).

### Checkpoint 850: Tower stays active with Surf retained

Keep active `tower` **DO NOT MOVE**. Preserve its reciprocal Surface boundary, Labyrinth-owned quest inputs, Heavens and Winterhaven reset imports, the Winterhaven consumer of OBJ #9375, and Raxthan's reset/shop references to OBJ #9332 with their current owners. Keep inactive Surf #227640 and #227641 source-only on existing candidate row 97; both approaches are already counted in the current 11,993 overall / 415 Surf total. Keep `surf` and `surface2011` **RETAIN IN PLACE**. No Tower candidate, activation, relocation, or migration clearance is added. See the [Tower audit](TOWER_OF_HIGH_SORCERY_AUDIT_REVIEW.md#checkpoint-850-current-graph-and-typed-closure-carried-forward), the [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-850-tower-graph-and-inactive-surf-approaches-remain-unchanged), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 851: Tradeskills stays active with the old catalog separate

Keep active `tradeskills` **DO NOT MOVE**. Preserve its two-room utility hub, conditional #400220 portal path, 293-object numeric catalog, active reset/QST/shop/sidecar consumers, and maintained-code bindings with their current owners. Keep unlisted `tradeskills.old.obj` separate on existing candidate row 108 as **RETAIN IN PLACE**; do not merge or load it with `tradeskills.obj`, whose 234 overlapping IDs include 16 changed records. Add no candidate for the active zone. Tradeskills has no source-only WLD approach and does not change the current 11,993 overall / 415 Surf total; the 9,479 / 297 figure in Checkpoint 396 is historical. No activation, relocation, or migration clearance is added. See the [Tradeskills audit](TRADESKILLS_AUDIT_REVIEW.md#checkpoint-851-current-source-boundary-and-catalog-dependencies-carried-forward), the [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-851-tradeskills-graph-and-dynamic-portal-remain-distinct), and [old-catalog candidate row 108](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L108).

### Checkpoint 852: Trakkia stays active with Surf retained

Keep active `trakkia` **DO NOT MOVE**. Preserve its two reciprocal Surface pairs, four source-flagged blocked exits, four fall fields, conditional switch/key/portal scenarios, Q24 reward ownership, local reset imports, active Alatorin reset, source-only Brass reset, local shop, and #57068 compiled pet-shop binding. Retain inactive Surf #246406 only on existing candidate row 97; it is already counted in the current 11,993 overall / 415 Surf total. Keep `surf` and `surface2011` **RETAIN IN PLACE**. The 2,611 / 153 count in Checkpoint 132 is historical. No Trakkia candidate, activation, relocation, or migration clearance is added. See the [Trakkia audit](TRAKKIA_AUDIT_REVIEW.md#checkpoint-852-current-graph-source-only-approach-and-typed-closure-carried-forward), the [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-852-trakkia-graph-and-surf-approach-remain-unchanged), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 853: Traoul stays active with its Truk key owner

Keep active `traoul` **DO NOT MOVE**. Preserve the reciprocal Truk #1622 boundary and the Truk-owned #1609 entrance key, all fourteen local reset door states, hidden-tunnel topology, and the unplaced #7001 key as unresolved source evidence. The all-95 inactive scan found no separate Traoul approach or room overlap, so add no candidate; current cumulative source-only totals remain 11,993 overall / 415 Surf. The QST/AREA entry without a local QST file does not establish a quest or migration source. No route activation or migration clearance is added. See the [Traoul audit](TRAOUL_AUDIT_REVIEW.md#checkpoint-853-current-boundary-and-typed-closure-carried-forward), the [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-853-traouls-graph-and-truk-boundary-remain-unchanged), and [coverage row 310](ZONE_COVERAGE_REGISTER.csv#L311).

### Checkpoint 854: Tribal Forest stays active with current owners

Keep active `tribal` **DO NOT MOVE**. Preserve its three reciprocal Surface pairs, six Astral Main exits, isolated Force Cage room, Heavens OBJ #72 import and `spell_pool` behavior, local shop/object procedure, and CHAOS starter reference to OBJ #42226. The all-95 inactive review found no separate Tribal Forest approach or room overlap, so add no candidate; current source-only totals remain 11,993 overall / 415 Surf. Keep Tribal Oasis as a distinct area identity. No activation, relocation, or migration clearance is added. See the [Tribal Forest audit](TRIBAL_FOREST_AUDIT_REVIEW.md#checkpoint-854-current-boundaries-and-typed-dependencies-carried-forward), the [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-854-tribal-forest-boundaries-and-routes-remain-unchanged), and the [Tribal Oasis audit](TRIBAL_OASIS_AUDIT_REVIEW.md).

### Checkpoint 855: Transparent Tower stays active with Surf retained

Keep active `trnsptow` **DO NOT MOVE**. Preserve the reciprocal ConnectorZones drawbridge, one-way Realm arrival, and inactive Surf #245010 source-only approach on existing candidate row 97; it is already counted in the current 11,993 overall / 415 Surf total. Keep ConnectorZones' nine Tower-equipment ingredients, active outside reset suppliers, imported reset objects, CHAOS kit references, and `luiren.shp` old-format collision records with their current source owners. Keep `surf` / `surface2011` **RETAIN IN PLACE**. No Tower candidate, activation, relocation, or migration clearance is added. The 9,480 / 298 total recorded at Checkpoint 400 is historical. See the [Transparent Tower audit](TRNSPTOW_AUDIT_REVIEW.md#checkpoint-855-current-graph-surf-approach-and-typed-closure-carried-forward), the [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-855-transparent-tower-boundaries-and-surf-approach-remain-unchanged), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 856: Troll Caves stays active with source maps retained

Keep active `troll_caves` **DO NOT MOVE**. Preserve its Surface and Malch reciprocal boundaries, three already-counted source-only approaches from Surf and underdark2011, Surface Keeps gem reset/shop ownership, the Graves sign's unplaced fixed-teleport declaration, and the separate Ixarkon code route. Keep the three inactive records on their current map identities: the two Surf approaches on existing candidate row 97 and the underdark2011 approach on existing candidate row 114. They are already included in the current cumulative source-only total of 11,993 overall / 415 Surf. The all-95 scan found no room overlap or new approach, so add no Troll Caves candidate. Keep the sidecar's old Alatorin label documented without changing source data. No route activation or migration clearance is added. See the [Troll Caves audit](TROLL_CAVES_AUDIT_REVIEW.md#checkpoint-856-current-boundaries-and-typed-dependencies-carried-forward), the [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-856-troll-caves-boundaries-and-source-only-approaches-remain-unchanged), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), plus [underdark2011 candidate row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L115).

### Checkpoint 857: Troll Hills stays active with Surf retained

Keep active `troll_hills` **DO NOT MOVE**. Preserve its three reciprocal boundaries, the separate Lortower inbound, two inactive Surf approaches on existing candidate row 97, five Heavens #283 imports, the Airp portal declaration, and the unresolved `bridge_troll` binding as distinct source evidence. The Surf approaches are already included in current cumulative totals of 11,993 overall / 415 Surf; the 1,999 / 146 figure at Checkpoint 126 is historical. Keep `surf` / `surface2011` **RETAIN IN PLACE**. The incomplete bridge binding, Airp route, and room-header questions remain builder issues and do not authorize activation or migration. No Troll Hills candidate or migration clearance is added. See the [Troll Hills audit](TROLL_HILLS_AUDIT_REVIEW.md#checkpoint-857-current-boundaries-and-source-only-approaches-carried-forward), the [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-857-troll-hills-boundaries-and-surf-approaches-remain-unchanged), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 858: Truktakyagyo stays active with ud_map2 retained

Keep active `truk` **DO NOT MOVE**. Preserve its three reciprocal active boundaries, bartender #1603's `world_quest` binding, the QST index/file discrepancy, and dormant `brass-old-1.zon` reference to OBJ #1604 as separate records. Keep ud_map2.wld #1126563 north to #1600 on existing candidate row 113 as source-only evidence; it is already counted in the current 11,993 overall / 415 Surf total and is not active ingress. Keep `ud_map2` **RETAIN IN PLACE**. The 9,480 / 298 figure at Checkpoint 403 is historical. No Truk candidate, activation, relocation, or migration clearance is added. See the [Truktakyagyo audit](TRUK_AUDIT_REVIEW.md#checkpoint-858-current-boundary-and-source-only-approach-carried-forward), the [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-858-truktakyagyo-boundaries-and-ud_map2-approach-remain-unchanged), and [ud_map2 candidate row 113](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114).

### Checkpoint 859: Twin Towers stays active with current owners

Keep active `ttowers` **DO NOT MOVE**. Preserve its four reciprocal boundaries, three foreign heart-QST alternatives and matching sidecar references, computed Jin Touch binding for #13219, and ascension-room check at #13272. The all-95 inactive review found no source-only approach or room overlap, so add no candidate; current cumulative totals remain 11,993 overall / 415 Surf. Keep Twin Towers Forest as a separate active area identity. The earlier 680/110 and 9,480/298 cumulative figures at Checkpoints 78 and 404 are historical. No route activation, relocation, or migration clearance is added. See the [Twin Towers audit](TWIN_TOWERS_AUDIT_REVIEW.md#checkpoint-859-current-map-boundaries-and-typed-closure-carried-forward), the [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-859-twin-towers-boundaries-and-typed-dependencies-remain-unchanged), and the [Twin Towers Forest audit](TWIN_TOWERS_FOREST_AUDIT_REVIEW.md).
## Checkpoint 860: Tundra stays active with existing candidates retained

Keep active `tundra` **DO NOT MOVE**. Carry forward its eight reciprocal WLD boundaries, one-way Heavens edge, unusable/inactive-target exits, seven unregistered Surf approaches, and bounded foreign item consumers. Preserve the Surf approaches on candidate row 97 and Player Guilds stocks on row 84 for their separate owner decisions. Those approaches are already included in the current 11,993 overall / 415 Surf source-only aggregate; the 9,480 / 298 total at Checkpoint 405 is historical. Add no Tundra candidate and grant no migration or activation clearance. See the [Tundra audit](TUNDRA_AUDIT_REVIEW.md#checkpoint-860-current-boundaries-and-typed-closure-carried-forward), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-860-tundra-boundaries-and-surf-approaches-remain-unchanged), and [coverage row 318](ZONE_COVERAGE_REGISTER.csv#L318).
## Checkpoint 861: Turolopolis Zoo stays active without a candidate

Keep active `turolzoo` **DO NOT MOVE**. Preserve the reciprocal Willem boundary and the authored closed-cage route behavior pending builder-intent review. The all-95 inactive-WLD sweep found no Zoo room overlap or inbound source-only approach; add no candidate row and grant no migration clearance. The Zoo's direct typed-reference scan found no external static prototype dependency; Tower #53000 is currency and Piergeiron's #53001 match is direction data. Older global subtotals of 3,405 / 173 / 15 and 9,480 / 298 are historical; current cumulative source-only totals are 11,993 overall / 415 Surf. Installed access and dynamic consumers remain outside this static review. See the [Turolopolis Zoo audit](TUROLZOO_AUDIT_REVIEW.md#checkpoint-861-current-source-boundary-and-typed-closure-carried-forward), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-861-turolopolis-zoo-graph-and-boundary-remain-unchanged), and [coverage row 319](ZONE_COVERAGE_REGISTER.csv#L319).
## Checkpoint 862: Twin Towers Forest stays active with owners retained

Keep active `twin_towers_forest` **DO NOT MOVE** and add no candidate. Preserve its four reciprocal boundaries, two load-room inbounds, local code bindings, active Nax #13563 reset/shop consumer, and the New Cave City wizard/object reset-source ownership. Retain the source-only Nexus56 resets and product list for Forest flower objects on existing Nexus56 candidate row 79; they do not establish operating stock. The all-95 inactive-WLD sweep found no Forest overlap or inbound approach. Current cumulative source-only totals are 11,993 overall / 415 Surf; the CP407/CP859 subtotals are historical. Do not duplicate the wizard placement or ship the separately unshipped milestone proposal. See the [Forest audit](TWIN_TOWERS_FOREST_AUDIT_REVIEW.md#typed-cross-area-references-and-code-bindings), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-862-twin-towers-forest-boundaries-and-typed-routes-carried-forward), and [Nexus56 candidate row 79](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79).
## Checkpoint 863: Twisted Wood stays active with Surf retained

Keep active `twstwd` **DO NOT MOVE**. Preserve its four reciprocal boundaries, two Pine Hollow NPC-load inbounds, conditional SHIFT PRIME reference, and five source-only `surf.wld` approaches into #16300–#16303. Keep those approaches on existing Surf candidate row 97; they are already included in current cumulative totals of 11,993 overall / 415 Surf. CP127's 2,004 / 151 subtotal and CP408's 9,480 / 298 count are historical. Preserve `surface2011` as its separate candidate source identity; it does not own these five records. Do not activate, merge, move, or grant migration clearance. See the [Twisted Wood audit](DARK_AND_TWISTED_WOOD_AUDIT_REVIEW.md#checkpoint-863-current-surf-approaches-and-dependencies-carried-forward), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-863-twisted-wood-boundaries-and-surf-approaches-remain-unchanged), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).
## Checkpoint 864: UDMini remains active without an attic candidate

Keep active `udmini` **DO NOT MOVE**. Preserve its two disconnected WLD components, two cave arrivals, single one-way outbound, F 100 fall rooms, and reset-placed #88503 teleport relationship to Kvarkpass. The two-room mushroom branch has no active WLD ingress and #88500 is not reset. Keep both external spore consumers and Newbie's independent reset source with their existing owners; do not infer generated stock. The all-95 inactive-WLD sweep found no overlap or source-only approach, so add no candidate. CP209's 3,458 / 220 / 17 aggregate is historical; current cumulative source-only totals are 11,993 overall / 415 Surf. See the [UDMini audit](UDMINI_AUDIT_REVIEW.md#checkpoint-864-current-arrivals-fall-state-and-spore-dependencies-carried-forward), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-864-udmini-arrivals-and-fall-state-remain-unchanged), and [coverage row 322](ZONE_COVERAGE_REGISTER.csv#L322).
## Checkpoint 865: Ugta stays active with Surf retained

Keep active `ugta` **DO NOT MOVE** and add no candidate. Preserve the reciprocal Surface pair, four singleton source rooms, closed/locked gate states, and imported Heavens/Verz1 object owners. Keep `surf.wld` room #209731 north to Ugta #39335 on existing Surf candidate row 97 as a source-only approach; it does not add active ingress. Preserve the source-only UGTA HOMETOWN/Stone Road chain on the Surf plan and do not classify it as another active Ugta map. Current cumulative source-only totals remain 11,993 overall / 415 Surf. See the [Ugta review](UGTA_STORY_REVIEW.md#checkpoint-865-current-source-only-ugta-approach-carried-forward), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-865-ugta-boundary-and-source-only-surf-path-remain-unchanged), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).
## Checkpoint 866: Durian Underdark remains active; 2011 map stays retained

Keep active `underdark` **DO NOT MOVE**. Preserve its 101 reciprocal active boundary pairs, sparse-map pruning result, outside White Hills seal consumer, Alatorin mobile reset reuse, dynamic portal, and selected worm/relic/system dependencies. Keep `underdark2011` **RETAIN IN PLACE**: 11 active WLD records from five owners still target its unregistered VNUMs, and #217609 remains ambiguous across `surf`, `surface2011`, and `underdark2011`. No approach or overlap into active `underdark` clears that separate candidate. CP119's 1,532 / 142 source-only subtotal is historical; current cumulative totals are 11,993 overall / 415 Surf. See the [Durian Underdark audit](DURIAN_UNDERDARK_AUDIT_REVIEW.md#checkpoint-866-sparse-map-and-candidate-boundaries-carried-forward), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-866-durian-underdark-graph-and-2011-candidate-remain-distinct), [underdark2011 candidate row 115](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L115), and [coverage row 324](ZONE_COVERAGE_REGISTER.csv#L324).
## Checkpoint 867: Undermountain stays active with foreign owners preserved

Keep active `undermountain` **DO NOT MOVE**. Preserve its Brass Fingers reciprocal boundary, one-way Underworld record and alternate Kvarkpass route, native note/key sources, closed grate, hidden object flags, and intentionally rare monster distribution. Retain the external IceCrag wine consumer, Caertannad #92051 placements, Brass #92046 stock/reset, FirePlane #92121 reset, source-only `brass-old-1`/Nexus56 references, and Avernus code binding with their current owners. The all-95 inactive-WLD scan found no overlap or approach. CP51's 586 / 26 source-only subtotal is historical; current cumulative totals are 11,993 overall / 415 Surf. No candidate, movement, or migration clearance is added. See the [Undermountain audit](UNDERMOUNTAIN_AUDIT_REVIEW.md#checkpoint-867-current-map-and-typed-owners-carried-forward), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-867-undermountain-boundaries-and-sparse-routes-remain-unchanged), and [coverage row 325](ZONE_COVERAGE_REGISTER.csv#L325).
## Checkpoint 868: Underworld stays active with source maps retained

Keep active `underworld` **DO NOT MOVE** and retain active `underworld3` as a distinct source despite their shared title. Preserve the 26 reciprocal boundary pairs, four additional inbound-only records, the unresolved #4645 target, and current-field/door/key/pool distinctions. Keep the three `ud_map0`/`ud_map2` approaches outside active ingress on candidate rows 112 and 114; the all-95 sweep found no room overlap. Preserve active and source-only reset references to Underworld mobile prototypes under each current owner. CP128's 2,007 / 151 subtotal is historical; current cumulative source-only totals are 11,993 overall / 415 Surf. No movement or migration clearance is added. See the [Underworld audit](UNDERWORLD_AUDIT_REVIEW.md#checkpoint-868-current-source-only-approaches-and-dependencies-carried-forward), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-868-underworld-boundaries-and-inactive-approaches-remain-unchanged), and candidate rows [ud_map0 112](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L112) and [ud_map2 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114).
## Checkpoint 869: Underworld3 stays active with candidate maps separate

Keep active `underworld3` **DO NOT MOVE**, independently of active `underworld`, despite their shared title. Preserve its eight reciprocal active boundaries and the one-way `mobs_underdark` inbound to #30149. Add no Underworld3 candidate. Keep the numeric room/prototype overlaps and candidate-local reset, shop, and portal records on their existing RETAIN plans: Bastard row 13, Cabal Hall row 22, Light Guildhall row 60, Nah row 77, Shadow Hall row 92, and Sunhall row 96. Nah's #30111/#30103 portal targets remain candidate-map identities, not active Underworld3 arrivals. Current cumulative source-only totals remain 11,993 overall / 415 Surf. No migration or route clearance is added. See the [Underworld3 audit](UNDERWORLD3_AUDIT_REVIEW.md#typed-dependencies-and-numeric-false-positives), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-869-underworld3-map-identity-and-active-routes-carried-forward), and [coverage row 327](ZONE_COVERAGE_REGISTER.csv#L327).
## Checkpoint 870: Unique remains active with candidate imports preserved

Keep active unique DO NOT MOVE and add no Unique attic candidate. Preserve the 42 active external ZON imports as dependencies on Unique-owned prototypes; do not infer live stock or transfer prototype ownership. Keep inactive source records on their current candidate plans: Brass Old 1 row 16, Brass Old 2 row 17, and Nexus56 row 78. The Juiblex source is active by manifest stem; its *876 index versus #875 ZON header discrepancy remains with Juiblex coverage. See the [Unique typed audit](UNIQUE_AUDIT_REVIEW.md#candidate-only-reset-references), [Brass Old 1 candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17), [Brass Old 2 candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L18), [Nexus56 candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79), and [coverage row 328](ZONE_COVERAGE_REGISTER.csv#L328). No migration clearance is added.
## Checkpoint 871: Undead Outpost remains with typed and code dependencies

Keep active unoutpst DO NOT MOVE and add no attic candidate. Preserve the paired Surface and Crypt WLD boundaries, the #98737 negative unresolved exit, the RACE_PHANTOM prime-shift selection of #98731, and the local ZON import of active Tharnadia counter #132581 into #98820. Keep Tarn #98718's money_changer callback distinct from the bank-counter/locker object path; its exchange text explicitly redirects coin exchange to the Royal Bank. Do not treat the unreachable #98735 literal after do_descend's disabled return as an active route. See the [Undead Outpost typed audit](UNOUTPOST_AUDIT_REVIEW.md), [Tharnadia owner audit](THARNADIA_AUDIT_REVIEW.md), [Human Outpost identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-766-undead-outpost-and-human-outpost-identities), and [coverage row 329](ZONE_COVERAGE_REGISTER.csv#L329). No source move or gameplay clearance is added.
## Checkpoint 872: Val remains active with Surf approach retained

Keep active val DO NOT MOVE and add no Val attic candidate. Preserve the reciprocal Surface boundary and the single unregistered surf.wld north approach from #205053 to Val #38575 on existing Surf candidate row 96 / physical line 97; it is not active ingress and creates no room overlap. Keep Verspin's active G reset of Val dagger #38421 attached to Verspin. The 13 chaos-kit values for Val objects remain static references, not proof of item creation or issuance. See the [Val audit](VALOISIAN_CASTLE_AUDIT_REVIEW.md#checkpoint-872-current-tracked-source-refresh), [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [coverage row 331](ZONE_COVERAGE_REGISTER.csv#L331). No migration clearance is added.

## Checkpoint 873: Valinhaven retained with typed and code dependencies

Keep active valinhav **DO NOT MOVE** and add no Valinhaven candidate. Preserve the two reciprocal active WLD boundaries, the possible code-selected prime-shift arrival at #90803, and the source-only ud_map1 #1113221 and ud_map2 #1123855 approaches into #90800 on their existing candidate plans (data rows 112 and 113, physical lines 113 and 114). Do not treat the inactive-map edges as active ingress or the code route as a WLD edge. Current source-only totals remain 11,993 overall / 415 Surf. Dynamic reachability, editor/tool closure, deployment/runtime, persisted room identity, and owner intent remain unqualified; no move, merge, or archive is cleared. See the [Valinhaven audit](VALINHAV_AUDIT_REVIEW.md), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-873-valinhaven-source-only-and-code-selected-approaches), [ud_map1 candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L113), [ud_map2 candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114), and [coverage row 333](ZONE_COVERAGE_REGISTER.csv#L333).

## Checkpoint 874: Valley of Crushk remains with its typed owners

Keep active valley_crushk **DO NOT MOVE** and add no candidate. Preserve the two active WLD inbounds, reciprocal Pharr boundary, one-way Surface departure, and the source-only Surf #232517 north exit to Valley #2060 on existing Surf candidate data row 96 / physical line 97. Keep Valley’s QST/story item #43143 attached to Ship Yards; keep Heavens OBJ #399/#606 as foreign reset and shop-stock imports; keep Alatorin, Charing, and Surface Keeps reset and shop references to Valley objects with those current areas. Leave local ZON M #207 unresolved because no exact mobile prototype was found in the scanned MOB sources. The source-only route is already counted and remains inactive ingress; totals stay 11,993 overall / 415 Surf. Runtime/deployment, editor/tool, and owner qualification remain open. See the [Valley typed refresh](VALLEY_CRUSHK_AUDIT_REVIEW.md#checkpoint-874-current-typed-source-refresh), [Surf route review](VALLEY_CRUSHK_AUDIT_REVIEW.md#inactive-wld-sweep), [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-421-valley-of-crushk-reciprocal-and-one-way-routes), and [coverage row 334](ZONE_COVERAGE_REGISTER.csv#L334).

## Checkpoint 875: Vehicles remains in place with its typed and transport owners

Keep active vehicles **DO NOT MOVE** and add no candidate. Preserve the 8-component WLD map separately from the seven code-defined ferries and transport table. Keep the 81 defined external ZON reset operands under their current source owners; keep the QST food inputs under Alatorin and Heavens, the WH/Tradeskills rewards and crafting items under their current owners, and local M #6124/#6123 plus QST item #400941 unresolved. Keep the four SHP records despite the coverage count discrepancy. Retain the inactive Alatorin Objects, Nexus56, Tradeskills Old, Duris3, Newbiemaps, and Surface2011 Temp VNUM collisions on their existing candidate rows; they do not replace active owners or create static WLD ingress. See the [Vehicles audit](VEHICLES_AUDIT_REVIEW.md), [builder questions](../zone-stories/VEHICLES_BUILDER_QUESTIONS.md), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-875-vehicles-static-graph-stays-separate-from-coded-travel), [coverage row 337](ZONE_COVERAGE_REGISTER.csv#L337), and the candidate rows linked in the audit. No move, merge, route, or gameplay change is cleared.

## Checkpoint 876: Verspin stays active with Surf retained in place

Keep active `verspin` **DO NOT MOVE** and add no candidate. Preserve the reciprocal Surface highway boundary and the one-way Winterhaven arrival, plus the source-only `surf.wld` #229065 north exit to Verspin #28100 on the existing Surf candidate data row 96 / CSV line 97. It is not active ingress and is already included in the 11,993 overall / 415 Surf source-only aggregate. Keep Verspin's active ZON imports attached to Foggy Woods, Verz1, Heavens, Herders, Val, Graves, and Nizari; retain Mntcastl's reset of Verspin object #28146 and Heavens/Bloodstone item ownership. Keep all five SHP records pending reconciliation with the zero shop count in coverage. The [Checkpoint 771 title/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-771-verspin-destination-references) remains unchanged; no predecessor or replacement was found. Live source behavior, editor/tooling, deployment/runtime, persisted identity, and owner qualification remain open. See the [Verspin audit](VERSPIN_AUDIT_REVIEW.md), [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-876-verspins-inactive-surf-approach-remains-source-only), [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [coverage line 338](ZONE_COVERAGE_REGISTER.csv#L338). No migration clearance is added.

## Checkpoint 877: Fateful Falls remains in its source fragment

Keep candidate `beggin` **RETAIN IN PLACE** under candidate data row 14 / CSV line 15. CP877 closes the bounded current-source typed scan: the local three reset commands resolve to the same-stem prototypes, with no active or unregistered external ZON, QST, SHP, story-sidecar, type-25, type-29, or compiled VNUM consumer/target. No other tracked WLD/MOB/OBJ source duplicates its room or prototype VNUMs; no other WLD map overlaps or connects to the two-room reciprocal road fragment. Keep the Fateful Falls title and source identity separate from active maps; do not infer access to the village, a Zornade contact, field harvesting, or a combat objective from descriptive text. The six existing builder questions remain the owner-decision list. Editor/tool, generated/deployed, persisted-identity, and owner-intent closure remain open. See the [Fateful Falls audit](FATEFUL_FALLS_AUDIT_REVIEW.md), [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-877-fateful-falls-remains-a-retained-two-room-source-fragment), [identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-877-fateful-falls-keeps-its-beggin-source-identity), [candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L15), and [coverage row 366](ZONE_COVERAGE_REGISTER.csv#L366). No migration clearance is added.


## Checkpoint 881: Alterian Wilderness source closure and retention

Keep `alterian_wilderness` **RETAIN IN PLACE** on candidate row 8. CP881 records three reciprocal inbound exits from Alterian Mountains and Surf, sixteen typed external ZON operands into Wilderness prototypes, and 37 same-number active Mining/Tchan reset operands that resolve to those active owners. Exact external QST, SHP, story-sidecar, OBJ-value, ITEM_TELEPORT, and ITEM_SWITCH scans found no typed edge into Wilderness prototypes or rooms. Preserve the separate Mining/Tchan map and #4800 mobile/object identities, all eleven source-only Wilderness approaches to active areas, reciprocal Mountain/Surf routes, sibling reset imports, code callbacks, and active Bloodstone's unresolved #4814 consumer. Keep the file set out of the full-area manifest until loader ownership, runtime/deployment, editor state, persisted identity, and content-owner intent are resolved; no move or archive is cleared. See the [Wilderness audit](ALTERIAN_WILDERNESS_AUDIT_REVIEW.md#checkpoint-881-external-source-identities-reconciled), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-880-alterian-wilderness-source-only-inbound-routes), [builder questions](../zone-stories/ALTERIAN_WILDERNESS_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions), [candidate row 8](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L8), and [coverage row 359](ZONE_COVERAGE_REGISTER.csv#L359).

## Checkpoint 882: Aeon remains separate from Genesis

Keep `aeon_gh` on candidate row 2 **RETAIN IN PLACE**, separate from `genesis` on row 46.
CP882 closes the bounded tracked-source WLD, reset, QST, SHP, JSON, OBJ travel-target, and
compiled-number census. Genesis is the only other source with exact numeric hits: its
eighteen WLD targets are internal map edges; its eight ZON commands refer to Genesis
rooms and mobile records sharing Aeon's VNUMs; and its shop uses keeper/room #29902 under
that source identity. No active ingress or separately attributable external Aeon consumer
was found. The only exact code match remains the #29903 `inn` assignment. Preserve Aeon's
Player Castles imports, fifteen external shop/equipment IDs, the duplicated #53/#108/#109
object owners, and unresolved #6089. This static source closure does not resolve #299
range ownership, the handler target, editor aliases, deployment/loader status, persisted
identities, or live runtime behavior; it grants no movement, merge, or archive clearance.
See the [Aeon audit](AEON_GH_AUDIT_REVIEW.md#checkpoint-882-role-aware-tracked-source-closure),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-882-aeon-and-genesis-numeric-wld-matches-classified),
and [candidate row 2](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L2).

### Checkpoint 884: Duris3 retains typed and code references

Keep Duris3 **RETAIN IN PLACE** on existing candidate row 36. The current
source census finds no distinct external ZON room reset into Duris3: Forofcon
and Brass Old 1 room operands resolve to their own source WLDs. Preserve the
active Fort Boyard #38143 type-25 target #149188, its Fortb #38100 reset,
the conditional random-lab code bindings at Duris3 #145443/#154938, and the
compiled #140854 `ship_shop_proc` shipwright binding while map/code owners reconcile
identity. QST and story-sidecar numeric matches are not room routes. None of
these references changes WLD counts or authorizes manifest activation, map
merging, renumbering, or path movement. Keep typed dependency **PENDING** until
editor/generator/plugin/deployment and persisted-room/player closure is
verified; then seek a separate owner decision. See the [Duris3 audit](DURIS3_AUDIT_REVIEW.md#checkpoint-884-typed-room-roles-and-compiled-consumers), [builder questions](DURIS3_BUILDER_QUESTIONS.md), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-884-duris3-typed-and-code-room-references), [candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), and [coverage row 387](ZONE_COVERAGE_REGISTER.csv#L387).

### Checkpoint 886: EOO retains role-separated identities and shop input

Keep EOO **RETAIN IN PLACE**. Checkpoint 886 confirms no new active WLD approach and separates numeric reset, portal, quest/story, code, and shop references by their actual source roles. Preserve the dark_gh, genocide_gh, active Newbie, eoo_op, eoo_op2, and EOO identities independently where VNUMs overlap. Retain the EOO shop record, but describe fallback loading as conditional: this checkout has no `areas/world.shp`, and the shard glob also encounters eight legacy `O`-format `luiren.shp` records rejected by the current `N`-only parser. Do not infer successful loading or permission to move from the glob path. Keep owner, editor, deployment, story, and persisted-state closure open. See the [technical audit](EOO_AUDIT_REVIEW.md#checkpoint-886-role-aware-reference-closure-and-shop-stream-qualification), [24 builder questions](EOO_BUILDER_QUESTIONS.md), [candidate row 37](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L37), and [coverage row 387](ZONE_COVERAGE_REGISTER.csv#L388).

### Checkpoint 887: EOO-OP retains role-separated map and portal identities

Keep candidate row 38, `eoo_op`, **RETAIN IN PLACE**. Preserve its three-room WLD, four mobiles, four type-25 objects and stored targets, the one-way #123225 Duris3 boundary, the Player Castles #11051 import, and EOO's three O resets of #29211-#29213. Checkpoint 887 classifies seven source-only dark_gh room resets, four active Newbie room resets, eight Newbie mobile reset rows, and two dark_gh portal targets by source role. None changes the active-Newbie route ownership or creates a candidate inbound route. Keep dark_gh, Newbie, EOO, EOO-OP, and EOO-OP2 identities separate until the map/story/object owners resolve the intended destinations, tool/editor, deployment, and persisted IDs. See the [technical audit](EOO_OP_AUDIT_REVIEW.md#checkpoint-887-typed-role-boundary-closure), [24 builder questions](EOO_OP_BUILDER_QUESTIONS.md), [candidate row 38](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L38), and [coverage row 388](ZONE_COVERAGE_REGISTER.csv#L389).

### Checkpoint 888: EOO-OP2 retains role-separated map and portal identities

Keep candidate row 39, `eoo_op2`, **RETAIN IN PLACE**. Preserve six rooms in the two repeated Hel chains, four mobiles, two pool portals and their #29202/#29206 targets, the one-way #29223 Duris3 boundary, all eight M and three O reset rows, and Player Castles object #11051. Checkpoint 888 confirms the dark_gh and Newbie reset/portal matches belong to those separate source identities; Newbie QST/story numeric hits are item roles, not room routes. Retain the two eoo_op portal pairs, dark_gh pair, and Newbie salt-box/vial QST, story, and Ailvio consumers independently. No active approach is added. Resolve map/story, VNUM, editor/tool, deployment, and persisted-data ownership before activation, movement, rename, archive, or merge. See the [technical audit](EOO_OP2_AUDIT_REVIEW.md#checkpoint-888-eoo-op2-typed-role-boundary-closure), [24 builder questions](EOO_OP2_BUILDER_QUESTIONS.md), [candidate row 39](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L39), and [coverage row 389](ZONE_COVERAGE_REGISTER.csv#L390).

### Checkpoint 889: eskull_hall retains separate map and shop identities

Keep candidate row 40, `eskull_hall`, **RETAIN IN PLACE**. Preserve the two Duris3 boundaries; the 10-room and 3-room WLD components; both type-25 portals; all four Player Castles imports; the fifteen G/SHP product matches and unresolved #6089; and each cbr_hall, ethdawn_op, loc_gh, Newbie, ravenmis, and eskull_hall identity. Checkpoint 889 classifies the 65 room, 27 mobile, and six object ZON operands by source role, plus six type-25 target records and four external shops. Keep the Ailvio mobile and #29305 inn bindings attached to active Newbie identities until code/map owners resolve them. The global fallback remains conditional: this checkout lacks `areas/world.shp` and its combined shard set includes eight legacy `O` records rejected by the current `N`-only parser. No active approach is added. Resolve catalog, map/story, editor, deployment, and persisted-ID ownership before activation, movement, rename, archive, or merge. See the [technical audit](ESKULL_HALL_AUDIT_REVIEW.md#checkpoint-889-role-aware-source-and-shop-field-closure), [24 builder questions](ESKULL_HALL_BUILDER_QUESTIONS.md), [candidate row 40](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L40), and [coverage row 390](ZONE_COVERAGE_REGISTER.csv#L391).

### Checkpoint 890: ethdawn_op retains typed portal and map identities

Keep candidate row 41, `ethdawn_op`, **RETAIN IN PLACE**. Preserve its three-room WLD, four directions, one-way #156745 Duris3 edge, header #1/range mismatch, Player Castles imports, and both type-25 portals. Checkpoint 890 classifies sixteen external room-role ZON fields, Ravenmis's #29311 object reset, Ebon Skull's #29312 portal target, and Newbie QST/story item fields by their source roles. These matches do not create an active WLD route or settle the target maps. Retain the active Newbie bag/arrows/quiver, loc_gh, ESKULL, Ravenmis, and ethdawn_op identities independently until the map, item, editor, deployment, story, and persisted-state owners decide. No WLD count changes. See the [technical audit](ETHDAWN_OP_AUDIT_REVIEW.md#checkpoint-890-role-aware-portal-and-reset-closure), [24 builder questions](ETHDAWN_OP_BUILDER_QUESTIONS.md), [candidate row 41](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L41), and [coverage row 391](ZONE_COVERAGE_REGISTER.csv#L392).

### Checkpoint 891: fists_gh retains Patrols and shop identities

Keep candidate row 42, `fists_gh`, **RETAIN IN PLACE**. Preserve its ten rooms, 21 directions, one-way Duris3 #157556 exit, the two previously counted source-only approaches, empty OBJ source with the 2016 deletion history, Player Castles #11013 import, five M resets, fourteen G/SHP product references including missing #6089, and compiled #30504 inn binding. Checkpoint 891 classifies all 70 room-role and 24 mobile-prototype external reset operands as active Patrols identities; no portal, QST/story, or outside shop-room consumer was found in that VNUM band. The shop fallback remains conditional: this checkout has no `areas/world.shp`, and its shard aggregate contains eight legacy `O`-format records rejected by the current parser. Keep the candidate and Patrols identities separate until map/shop/code/tooling/deployment and persisted-state owners decide. No WLD count changes. See the [technical audit](FISTS_GH_AUDIT_REVIEW.md#checkpoint-891-patrols-role-matches-and-shop-stream-qualification), [24 builder questions](FISTS_GH_BUILDER_QUESTIONS.md), [candidate row 42](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L42), and [coverage row 392](ZONE_COVERAGE_REGISTER.csv#L393).


### Checkpoint 892: fooquest retains quest and map identities

Keep candidate row 43, `fooquest`, **RETAIN IN PLACE**. The role-aware continuation found no external ZON reset operand, typed QST/story consumer, type-25 portal or type-29 switch destination, parser-valid shop keeper/room/product field, or same-type current WLD/MOB/OBJ definition for its candidate ranges. This does not close FooQuest's internal question set: retain the exact QST reuse finding, external Moonshae key #26201, separate #63600 pirate records, disconnected room #65050, 320 tooth resets, compiled special/giver bindings, and #21/#22 transforms. Keep loader manifests, editor/tooling, deployment, story, code, and persisted-VNUM ownership open. The conditional shop fallback remains unqualified because the current parser rejects eight legacy `O`-format Luiren records. No graph count changes. See the [technical audit](FOOQUEST_AUDIT_REVIEW.md#checkpoint-892-role-aware-external-closure-and-room-target-scan), [24 builder questions](FOOQUEST_BUILDER_QUESTIONS.md), [candidate row 43](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L43), and [coverage row 393](ZONE_COVERAGE_REGISTER.csv#L394).


### Checkpoint 893: forestb retains unplaced quest and shop identities

Keep candidate row 44, `forestb`, **RETAIN IN PLACE**. Checkpoint 893 found no external typed ZON/QST/story consumer, same-type MOB/OBJ definition, portal/switch room target to the SHP room #38259, or other parser-valid shop role using its mobile/object/room IDs. The only matching shop roles are in Forestb's own SHP record. The source bundle still has no WLD/ZON, its QST and SHP records remain unlisted, and its keeper/room relationship is unresolved. Preserve the bluebird QST mismatch, candidate item goals, #38235 keeper and #38259 room, local type-25 portals #38234→#38297 and #38253→#38295 with no current WLD destinations, four stock products, and loader guard question. The non-mini fallback is not established as loadable because its shard set includes eight `O`-format records rejected by the current parser; qualify any negative keeper-index risk as conditional on a complete catalog reaching assignment. No migration clearance or WLD count change. See the [technical audit](FORESTB_AUDIT_REVIEW.md#checkpoint-893-role-aware-cross-source-closure), [24 builder questions](FORESTB_BUILDER_QUESTIONS.md), [candidate row 44](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L44), and [coverage row 394](ZONE_COVERAGE_REGISTER.csv#L395).


### Checkpoint 894: forofcon retains Smoke and compiled room identities

Keep candidate row 45, `forofcon`, **RETAIN IN PLACE**. Preserve the CP482 graph and five already-counted Smoke-directed source-only approaches without changing connectivity totals. CP894 separates Forofcon's unregistered 19 M/21 D/one O Smoke-numbered resets from active Smoke's 78 M/F/O room operands, four D room operands, and two type-25 portal targets, all of which resolve to active Smoke identities in the overlapping band. Preserve all 16 Forofcon type-25 destinations and four type-29 switch destinations; resolve the special #140030→#139903 numeric collision with Smoke before map/object activation. Keep the executable Olympus #140000 zone-base reference and encounter range `real_room(140000)` through `real_room(149999)` under code and map-owner review. The scans found no other external typed ZON prototype, QST/story/shop, type-29, or same-type MOB/OBJ roles for the candidate ranges. This closure does not settle manifests, editor or dynamic paths, code intent, or persisted IDs and adds no WLD route. See the [technical audit](FOROFCON_AUDIT_REVIEW.md#checkpoint-894-role-aware-smoke-and-portal-identity-closure), [24 builder questions](FOROFCON_BUILDER_QUESTIONS.md), [candidate row 45](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L45), and [coverage row 395](ZONE_COVERAGE_REGISTER.csv#L396).


### Checkpoint 895: Genesis retains separate Aeon and shop identities

Keep candidate row 46, Genesis, **RETAIN IN PLACE**. Preserve its ten-room graph and source-only D4 to Duris3 #157747 without adding a current route. The only external ZON role matches are unregistered Aeon resets to Aeon's own same-number mobiles and rooms; no active outside ZON, QST, story-sidecar, WLD, portal, or switch target was found. Preserve the distinct Genesis and Aeon room/mobile records, the compiled `real_room0(29903)` inn binding pending an owner decision, Genesis keeper/room #29902, and Aeon keeper/room #29901/#29909. Both unregistered shops reference the same nine product IDs, whose object owners remain external or unresolved (#6089). These source overlaps do not establish that either area replaces the other or that either shop loads. Keep manifests, editor/deployment tooling, dynamic path, shop loading, and persisted-ID review open. See the [Genesis audit](GENESIS_AUDIT_REVIEW.md#checkpoint-895-role-aware-aeon-and-external-dependency-closure), [24 builder questions](GENESIS_BUILDER_QUESTIONS.md), [candidate row 46](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L46), and [coverage row 396](ZONE_COVERAGE_REGISTER.csv#L397).


### Checkpoint 896: genocide_gh retains Newbie and source-owned boundaries

Keep candidate row 47, `genocide_gh`, **RETAIN IN PLACE**. Preserve the CP484 graph and its one previously counted source-only approach without changing curated connectivity totals. Keep active Newbie room/mobile, QST/story, and executable identities separate from genocide_gh; keep unregistered Dark Guildhall and EOO map, reset, portal, and shop values attached to their own source files. The five outside type-25 destinations and the compiled #29201/#29202 room references need owner resolution before any registration, activation, or path proposal. Treat the #29201 Ailvio lookup as Newbie OBJ, and keep the conditional fallback shop collisions qualified until a valid loaded catalog and keeper mapping are established. Do not infer replacement history or move clearance from numeric overlap. See the [role-aware audit](GENOCIDE_GH_AUDIT_REVIEW.md#checkpoint-896-role-aware-newbie-dark-guildhall-and-eoo-closure), [24 builder questions](GENOCIDE_GH_BUILDER_QUESTIONS.md), and [coverage row 397](ZONE_COVERAGE_REGISTER.csv#L398).


### Checkpoint 897: genocide_hall retains source-owned map and shop boundaries

Keep candidate row 48, `genocide_hall`, **RETAIN IN PLACE**. Preserve its CP485 seven-room/two-component graph and two unresolved source-only D4 boundaries without adding an inbound route. Keep the 28 WLD destination records, 25 AoD and 25 Legion typed ZON matches, four RF Outpost room operands, two Legion type-25 portal values, and AoD/Legion SHP roles attached to those unregistered source identities. The external records do not establish active travel or replacement. Resolve Harrow's broad #294-#296 manifest range, same-number map/mobile/object owners, #29502 inn binding, #29503 shop room/keeper, fallback loader behavior, and persisted IDs before any activation or path proposal. See the [role-aware audit](GENOCIDE_HALL_AUDIT_REVIEW.md#checkpoint-897-role-aware-closure-across-the-overlapping-map-band), [24 builder questions](GENOCIDE_HALL_BUILDER_QUESTIONS.md), and [coverage row 398](ZONE_COVERAGE_REGISTER.csv#L399).


### Checkpoint 898: gianthou retains source-only boundaries and typed identities

Keep candidate row 49, `gianthou`, **RETAIN IN PLACE**. Preserve the CP486 271-room/one-component graph, its 20 fall-chance rooms, and one source-only #42870 D3 boundary to #274141 without adding ingress. The role-aware continuation found no external WLD/MOB/OBJ identity, ZON role, WLD target, OBJ portal/switch target, QST/story-sidecar value, parser-shaped N-format shop field, or exact executable VNUM reference. Keep the `surf`/`surface2011` target choice, Pworm/open manifest range, story and quest ownership, full reset set, four shops, fallback keeper behavior, editor/deployment copies, and persisted IDs under owner review. The negative scans do not authorize registration or a path change. See the [role-aware audit](GENIANTHOU_AUDIT_REVIEW.md#checkpoint-898-complete-typed-cross-source-closure), [24 builder questions](GENIANTHOU_BUILDER_QUESTIONS.md), and [coverage row 399](ZONE_COVERAGE_REGISTER.csv#L400).


### Checkpoint 899: guild_rooms retains source-owned shop identities

Keep candidate row 50, `guild_rooms`, **RETAIN IN PLACE** as one unregistered SHP record with no map or reset component. Preserve the missing keeper #8552 MOB and missing stock prototypes #8555-#8559 as unresolved. Keep active Grave room #8560, its two local WLD target edges, its two Grave reset placements, and the compiled #8556 transfer callsites attached to Grave identities. No external N-format shop, QST/story source, OBJ portal/switch target, or other typed consumer adds a Guild Rooms route. Resolve the shop owner, loading source, keeper, products, Grave location, editor/deployment copies, and persisted IDs before activation or any path decision. See the [role-aware audit](GUILD_ROOMS_AUDIT_REVIEW.md#checkpoint-899-typed-shop-and-active-grave-boundary-closure), [24 builder questions](GUILD_ROOMS_BUILDER_QUESTIONS.md), and [coverage row 400](ZONE_COVERAGE_REGISTER.csv#L401).

### Checkpoint 900: Halfcut Hills retains both source identities pending owner review

Keep candidate row 51, halfcut_hills, **RETAIN IN PLACE** and do not load it
with active halfcut. Candidate #27001 north to #256031 and unregistered
surf.wld #256031 south to #27001 form a numeric reciprocal pair across
separate unregistered source files. surface2011.wld supplies a conflicting
#256031 room whose south exit targets #256331. The surf direction is already
included in the Checkpoint 448 11,987 / 415 source-only recount because its
target is uniquely active halfcut #27001 and its origin is absent from that
map. It is not candidate-specific active ingress and does not change the
active inbound count of one from Surface #629274. Resolve target-map ownership,
the old candidate source relationship, map loading, reset/story/code roles,
editor/deployment copies, and persisted IDs before any activation, merge, or
path decision. See the [role-aware audit](HALFCUT_HILLS_AUDIT_REVIEW.md#checkpoint-900-source-only-reciprocal-edge-and-typed-dependency-closure),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-900-halfcut-hills-reconciles-source-only-reciprocal-route),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-900-halfcut-hills-source-only-reciprocal-edge-and-history-boundary),
and [24 builder questions](HALFCUT_HILLS_BUILDER_QUESTIONS.md).

### Checkpoint 901: Hostel retains separate low-VNUM identities

Keep candidate row 52, `hostel`, **RETAIN IN PLACE**. Preserve its five
unlisted files, 102-room/five-component graph, ten active numeric matches to
Limbo/Heavens, and candidate-specific active inbound count of zero. The only
non-source-local outside WLD record into the shared numeric band is unregistered Surf #201670
D5 to active Heavens #36, already included in the Checkpoint 448 source-only
recount; it is not Hostel ingress. Keep all 25 candidate key fields associated
with their separate Hostel OBJ identities and current active Limbo/Heavens
same-number owners. Maintain the distinct candidate and active reset, QST,
shop, portal, and code roles, especially mobile/object/QST #18/#41/#46/#72.
The scanned external typed values do not establish that Hostel replaces
Limbo, Heavens, Mavulsk, or any other parallel source identity. Resolve story
ownership for Savannah's six-instrument lead, disconnected-room/door intent,
all low-VNUM owners, editor and deployment copies, dynamic paths, and
persisted IDs before activation, renumbering, archiving, or any path proposal.
See the [Hostel typed audit](HOSTEL_AUDIT_REVIEW.md#checkpoint-901-typed-cross-source-closure),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-901-hostel-role-scans-leave-the-graph-unchanged),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-901-hostel-parallel-low-vnum-identities),
[24 builder questions](HOSTEL_BUILDER_QUESTIONS.md), and
[coverage row 402](ZONE_COVERAGE_REGISTER.csv#L403).

### Checkpoint 902: IC3 retains its source-only route and typed boundary

Keep candidate row 53, `IC3`, **RETAIN IN PLACE**. Preserve the four unlisted
high-VNUM files, zero current active ingress, and single candidate outbound
from #420000 to active Rice Fields #77262; its former active return was removed
and its source-only direction is already in the Checkpoint 174 curated total.
Preserve candidate ZON's Heavens OBJ #360 import, both type-25 portals to local
room #420022, ten door resets, and the #420018 unused-key question. The
Checkpoint 902 outside role census found no incoming WLD, typed ZON, QST,
portal/switch, SHP, or direct-code dependency; range-wide same-type scans found
no outside room/mobile/object definitions. Caertannad's 420000 is a currency
reward, while the `nexus.mob` numeric tuple remains explicitly unclassified.
Do not treat those findings as closure for the route, reset-mode-0 behavior,
story ownership, editor/deployment copies, dynamic lookups, or state persisted
while IC3 was active. Resolve I-01 through I-24 with the affected owners before
activation, renumbering, archive, or path change. See the
[IC3 role audit](IC3_AUDIT_REVIEW.md#checkpoint-902-typed-cross-source-role-refresh),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-902-ic3-has-no-new-external-ingress),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-902-ic3-remains-a-distinct-high-vnum-source),
[24 builder questions](IC3_BUILDER_QUESTIONS.md), and
[coverage row 403](ZONE_COVERAGE_REGISTER.csv#L404).

### Checkpoint 903: Infernal retains typed shop and map boundaries

Keep candidate row 54, `infernal_gh`, **RETAIN IN PLACE**. Preserve the 16
candidate rooms, three mobiles, four portal objects, three source-only
Duris3 exits, and separate active Harrow ownership of every matching room,
mobile, and object VNUM. The outside WLD scan's active Surface edges and
source-only Surf edge to #29400 resolve to Harrow. Preserve Harrow's
QST/story-sidecar item roles #29400/#29402, active SHP keeper/room/product
values, and room #29403 inn binding. Keep active reset roles with their source
areas, the candidate's seven Player Castles imports, Heavens #491–#494 stock
references, missing #6089, and the two unlisted #29401 shop records. The
conditional shop fallback is a source path, not proof of runtime loading.
Resolve source/story/map owners, alias and shared-generation history, reset
exports, shop input, editor/deployment copies, dynamic paths, and persisted
identities before activation, merge, archive, or path proposal. See the
[Infernal role audit](INFERNAL_GH_AUDIT_REVIEW.md#checkpoint-903-typed-cross-source-role-closure),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-903-infernal-numeric-routes-remain-harrow-owned),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-903-infernal-keeps-harrow-and-sibling-identities-separate),
[30 builder questions](INFERNAL_GH_BUILDER_QUESTIONS.md), and
[coverage row 404](ZONE_COVERAGE_REGISTER.csv#L405).

### Checkpoint 904: Jungle retains existing route and movement boundaries

Keep candidate row 55, `jungle`, **RETAIN IN PLACE**. Preserve the four
unlisted source files and #67000–#67092 identities, four source-only inbound
WLD records from `surf`/`underdark2011`, three one-way Duris3 targets, and two
Surface departures already counted at CP373. The role-aware recheck found no
external reset, quest, sidecar, shop, portal/switch, or direct compiled lookup
into Jungle. Keep the old `qst/AREA` row distinct from production quest input:
no `qst/jungle.qst` exists, and the compiler follows the commented full AREA
list. Preserve reset mode 1, eleven southbound current fields, and bridge F 5
pending owner review; they do not establish live access or safe return. Resolve
map/story and manifest owners, editor/deployment copies, dynamic paths, and
persisted IDs before any activation, archive, or path proposal. See the
[Jungle typed audit](JUNGLE_AUDIT_REVIEW.md#checkpoint-904-external-typed-role-refresh),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-904-jungle-has-no-new-external-typed-route),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-904-jungle-retains-its-unlisted-identity),
[27 builder questions](JUNGLE_BUILDER_QUESTIONS.md), and
[coverage row 405](ZONE_COVERAGE_REGISTER.csv#L406).

### Checkpoint 911: Magetower retains its separate map and prototype sources

Keep candidate row 62, `Magetower`, **RETAIN IN PLACE**. Preserve the four
compiler files and `areas/map/Magetower.json` together and unlisted. Keep the
71-room/152-direction graph, #1000 range mismatch, four D and five M resets,
20 type-13 objects, and all editor coordinates unchanged. Active Limbo and
Heavens retain their room, mobile, object, quest, shop, and compiled-code
identities at matching low VNUMs. Unlisted Dalvik, Hostel, Mavulsk, Mentiri,
Nexus_1, Theurgist, Waterplane, Astral, Mtncastl, and ud-inside retain their
separate same-number source definitions.

CP911 found 70 active WLD targets to active Limbo/Heavens rooms and no active
Magetower ingress; 868 of 869 unlisted WLD targets are local to their own map,
with the remaining Surf target resolving to Heavens #36. The external typed
reset, QST/story, N-format SHP, and code fields stay attached to those active
or source-local records. No external type-25/type-29 target selects a candidate
room, and the candidate has no portal or switch object. Keep all 25 builder
questions and the editor, export, deployment, and persisted-state decisions
open. Do not activate, merge, renumber, move, or archive these files based on
the numeric overlaps. See the
[Magetower role-aware audit](MAGETOWER_AUDIT_REVIEW.md#checkpoint-911-role-aware-low-vnum-source-closure),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-911-magetower-numeric-destinations-remain-source-owned),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-911-magetower-retains-separate-low-vnum-map-identities),
[25 builder questions](MAGETOWER_BUILDER_QUESTIONS.md), and
[coverage row 413](ZONE_COVERAGE_REGISTER.csv#L414).

### Checkpoint 912: map1 stays as a zero-byte path pending owner review

Keep candidate row 63, `map1`, **RETAIN IN PLACE pending owner decision**.
Preserve the only tracked source, zero-byte `areas/mob/map1.mob`, outside both
area lists. With no VNUMs or other source files, there is no map, prototype,
reset, quest, shop, or story identity to activate or reconcile. The bounded
path scan found no reference to the exact MOB source path; `MAP1_FILE` and
`MAP1A_FILE` point to separate information files. Keep `ud_map1` distinct and
retain the current questions about placeholder purpose, editor/export, and
external copies. No removal, rename, or reclassification is authorized by
this static review. See the
[map1 audit](MAP1_AUDIT_REVIEW.md#checkpoint-912-exact-path-reference-refresh),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-912-map1-has-no-graph-or-room-identity),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-912-map1-remains-an-empty-path-artifact),
[10 builder questions](MAP1_BUILDER_QUESTIONS.md), and
[coverage row 414](ZONE_COVERAGE_REGISTER.csv#L415).

### Checkpoint 913: map22 keeps its grid and conditional generator roles

Keep candidate row 64, `map22`, **RETAIN IN PLACE**. Preserve the 10,000-room
#900000–#909999 WLD, the type-1 `reset_lab`/`create_lab` bindings, separate
Duris3 room #145443, and the unresolved object #68 source identities. No
external WLD, typed room-role ZON, parser-valid N-format shop-room, QST,
story-room, type-25/type-29 destination, or direct literal room-code reference
selects a candidate room. Four raw #900000 values in ordinary object fields do
not constitute routes. The created down entrance is conditional generator
behavior and was not exercised. Keep the 20 builder questions and reconcile
the map, random-zone, Duris3, object, editor, and deployment owners before any
input or path change. See the [map22 audit](MAP22_AUDIT_REVIEW.md#checkpoint-913-role-aware-random-lab-dependency-closure),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-913-map22-external-room-references-leave-the-grid-unchanged),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-913-map22-reference-closure-does-not-establish-a-replacement),
[20 builder questions](MAP22_BUILDER_QUESTIONS.md), and
[coverage row 415](ZONE_COVERAGE_REGISTER.csv#L416).

### Checkpoint 914: map5 mobile dependencies remain source-only or static

Keep candidate row 65, `map5`, **RETAIN IN PLACE**. Preserve all 40 MOB
prototypes, the 45 `M` placement rows and nine `F` follow rows in unlisted
`surf.zon`, and the static patrol/Tharnadian bindings. Keep the four absent
MOB IDs #150120, #150121, #150127, and #150128 in the code-owner review.
The same 40 numbers are each a separate room VNUM in unlisted `Duris3.wld`;
this cross-type match does not identify map5 with that WLD or alter its
nonexistent room graph. QST/story #150000 occurrences are reward currency or
experience fields, and no parser-valid N-format shop keeper points to a map5
mobile. Keep the 20 builder questions and reconcile map5, Surf, Tharnadian,
content, code, and deployment owners before any source-list or path change.
See the [map5 audit](MAP5_AUDIT_REVIEW.md#checkpoint-914-map5-cross-namespace-role-closure),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-914-map5-reset-roles-do-not-create-a-room-graph),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-914-map5-mobile-and-duris3-room-numbers-remain-distinct-identities),
[20 builder questions](MAP5_BUILDER_QUESTIONS.md), and
[coverage row 416](ZONE_COVERAGE_REGISTER.csv#L417).

### Checkpoint 915: keep the empty map8 path in place

Keep candidate row 66, `map8`, **RETAIN IN PLACE pending owner decision**.
Preserve the zero-byte `areas/mob/map8.mob` outside both area lists. Bounded
case-insensitive token and exact-path scans across `areas/`, `src/`, `scripts/`,
and `tests/` found no in-checkout reference; the source has no VNUMs for typed
references or connectivity. Do not merge it with `map1.mob` or `oldmap8.wld`
from filename or empty-history similarity. Reconcile source, editor, export,
backup, and deployment owners before any separate path decision. See the
[map8 path-reference audit](MAP8_AUDIT_REVIEW.md#checkpoint-915-exact-path-reference-refresh),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-915-map8-has-no-path-or-room-identity),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-915-map8-path-scan-does-not-resolve-empty-file-identity),
[10 builder questions](MAP8_BUILDER_QUESTIONS.md), and
[coverage row 417](ZONE_COVERAGE_REGISTER.csv#L418).

### Checkpoint 916: Mara Guildhall typed references stay in place

Keep candidate row 67, `mara_gh`, **RETAIN IN PLACE** outside both area lists.
The exact #29600–#29617 role scan found only the `mara_gh` and `mara_op`
source families across WLD, MOB, OBJ, ZON, and SHP; QST/story scans found no
candidate reference, and compiled source adds only the established room
#29605 inn binding. Keep the three Duris3 exits, duplicate Mara Outpost
records, 13 mobile and 10 object placements, 16 gear resets, Player Castles
imports, shop goods, and three ITEM_TELEPORT values under their current source
roles. No graph or active-ingress metric changes. Preserve the 20 questions
and reconcile map, shop/item, code, Player Castles, and deployment owners
before any activation, merge, or path decision. See the
[Mara Guildhall audit](MARA_GH_AUDIT_REVIEW.md#checkpoint-916-exact-vnum-role-closure-refresh),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-916-mara-guildhall-source-role-scan-adds-no-active-boundary),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-916-mara-guildhall-exact-vnum-closure-remains-source-local),
[20 builder questions](MARA_GH_BUILDER_QUESTIONS.md), and
[coverage row 418](ZONE_COVERAGE_REGISTER.csv#L419).

### Checkpoint 917: Mara Outpost remains separate under its source plan

Keep candidate row 68, `mara_op`, **RETAIN IN PLACE**. Preserve the four
source files, three-room/five-direction WLD, four source-only resets, the
Duris3 #124408 boundary, and both ITEM_TELEPORT values. Exact #29610–#29612
scans find no third WLD/MOB/OBJ/ZON owner, no QST/story reference, and no
compiled Mara Outpost binding; Mara Guildhall remains the sole matching shop
source and the source of portal target room #29608. Keep the overlapping
Mara Guildhall definitions separate and retain all 20 owner questions. No
activation, merge, or path change is cleared. See the
[Mara Outpost audit](MARA_OP_AUDIT_REVIEW.md#checkpoint-917-mara-outpost-exact-vnum-role-closure),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-917-mara-outpost-room-scan-adds-no-active-route),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-917-mara-outpost-exact-vnum-scan-confirms-two-source-identities),
[20 builder questions](MARA_OP_BUILDER_QUESTIONS.md), and
[coverage row 419](ZONE_COVERAGE_REGISTER.csv#L420).

### Checkpoint 918: Mavulsk typed cross-source references remain under retention

Keep candidate row 69, `mavulsk`, **RETAIN IN PLACE** outside both area
lists. The active Heavens M #2 reset resolves to the distinct active Limbo
mobile #2; unlisted Mavulsk #2 and Mentiri #2 are separate prototypes.
Active low-object reset, QST, SHP, and story values do not identify Mavulsk
prototypes. Seventeen active ITEM_TELEPORT fields point to #2, #9,
#50, #100, #150, or #200, where the current active WLD table has no room and
multiple unlisted WLD identities exist. Preserve the 233-room source, local
reset/QST/teleport roles, Thetis #104 and Surf #228202 questions, and all
same-type identities. No activation, renumbering, merge, archive, or path
change is cleared. See the
[Mavulsk role-aware audit](MAVULSK_AUDIT_REVIEW.md#checkpoint-918-mavulsk-typed-cross-source-role-closure),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-918-mavulsk-object-destinations-do-not-change-wld-metrics),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-918-mavulsk-role-matches-do-not-establish-identity-or-replacement),
[25 builder questions](MAVULSK_BUILDER_QUESTIONS.md), and
[coverage row 420](ZONE_COVERAGE_REGISTER.csv#L421).

### Checkpoint 919: Mentiri cross-source roles remain under retention

Keep candidate row 70, `Mentiri`, **RETAIN IN PLACE** outside both area
lists. Preserve its four WLD/MOB/OBJ/ZON files and 72-position map JSON.
Active Heavens M #2 resolves to current Limbo #2; active ZON object resets
#5/#17, active QST/story/shop values, and code lookups match current
Limbo/Heavens identities, not Mentiri. The six external ITEM_TELEPORT values
to #2/#9/#50 have no active WLD owner, while a separate Surf source-only
target to #36 resolves to active Heavens. Preserve the areas_mini fixture as
a separate configuration. No activation, VNUM change, merge, archive, or
path move is cleared. See the
[Mentiri exact role audit](MENTIRI_AUDIT_REVIEW.md#checkpoint-919-mentiri-exact-cross-source-role-closure),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-919-mentiri-source-only-targets-do-not-add-active-ingress),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-919-mentiri-cross-source-roles-do-not-establish-lineage),
[22 builder questions](MENTIRI_BUILDER_QUESTIONS.md), and
[coverage row 421](ZONE_COVERAGE_REGISTER.csv#L422).

### Checkpoint 920: Monsteri exact role closure remains under retention

Keep candidate row 71, `Monsteri`, **RETAIN IN PLACE** outside both area
lists. Preserve all five tracked paths, including zero-byte OBJ and QST.
There is no active WLD owner or ingress for #1000–#1004, and no active mobile
reset uses #1000–#1004. Keep Heavens mobiles #1000–#1004 distinct from
Monsteri’s #1000–#1003 prototypes and room #1004. Menden ZON G and shop
stock #1000 resolve as object-role references to active Heavens OBJ #1000;
they do not assign an item or route to Monsteri. Preserve the source-only
Monsteri/cbl_outpost2 identities, the header/range discrepancy, and the
unresolved Portal Cave intent. No activation, renumbering, merge, archive, or
path move is cleared. See the
[Monsteri exact role audit](MONSTERI_AUDIT_REVIEW.md#checkpoint-920-monsteri-exact-cross-source-role-closure),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-920-monsteri-exact-room-band-adds-no-active-ingress),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-920-monsteri-cross-source-roles-remain-distinct),
[24 builder questions](MONSTERI_BUILDER_QUESTIONS.md), and
[coverage row 422](ZONE_COVERAGE_REGISTER.csv#L423).

### Checkpoint 921: moriatst role closure keeps the candidate in place

Keep candidate row 72, `moriatst`, **RETAIN IN PLACE**. Preserve its single
SHP file and three shop records outside the normal `areas/AREA` aggregation.
Each keeper, room, and stock field resolves to active Moria; Moria ZON resets
place the same keepers in the same rooms, and Moria-specific code binds those
mobile VNUMs to merchant procedures. The candidate #99037 sell factor remains
1.00 while active Moria uses 1.10. The conditional wildcard fallback can
include the candidate alongside active `moria.shp` and duplicate keeper/room
bindings; no current boot or deployment is inferred. Keep the corrected sell
factor and selector ambiguity documented, and resolve all load paths and
owner intent before any archive or path decision. No migration clearance is
implied. See the
[moriatst exact role audit](MORIATST_AUDIT_REVIEW.md#checkpoint-921-moria-role-bindings-and-shop-factor-correction),
[Moria supplement](MORIA_AUDIT_REVIEW.md#checkpoint-921-moriatst-typed-shop-roles-and-effective-factors),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-921-moriatst-shop-rooms-are-not-wld-edges),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-921-moriatst-roles-confirm-duplicate-source-not-lineage),
[12 builder questions](MORIATST_BUILDER_QUESTIONS.md), and
[coverage row 423](ZONE_COVERAGE_REGISTER.csv#L424).

## Checkpoint 922: retain Mountain QST with typed identities unresolved

Keep coverage row 424 / candidate dependency row 73 at RETAIN IN PLACE.
CP922 found that the candidate's five mobile-header VNUMs and seven missing
item VNUMs share numbers with active mountaintracks rooms. Room identities do
not resolve those mobile or item roles, so this overlap supplies no archive
or migration clearance. Preserve the Mountain QST source and the separate
Dungeon object #93001 output reference while the story owner, prototypes, and
deployment or persisted-data references remain unqualified. See the
[Mountain QST role audit](MOUNTAIN_QST_AUDIT_REVIEW.md#checkpoint-922-typed-collision-with-active-mountaintracks-rooms).

## Checkpoint 923: mtncastl retained after room-role closure

Keep candidate row 74, mtncastl, **RETAIN IN PLACE**. Preserve its four
same-stem paths, including the zero-byte MOB and OBJ files. CP923 confirms
that active Limbo owns numeric room #1; nine active Desert directions resolve
there, while fourteen other WLD directions and four ZON room-field commands
come only from unlisted sources. No active WLD inbound, typed shop/portal/
switch/story location, or compiled room binding identifies mtncastl. This
source result still does not qualify external editor, deployment, or
persisted-room ownership, so no archive, activation, rename, or move is
cleared. See the
[mtncastl exact role audit](MTNCASTL_AUDIT_REVIEW.md#checkpoint-923-exact-room-1-dependency-and-role-closure),
[13 builder questions](MTNCASTL_BUILDER_QUESTIONS.md),
[candidate dependency line 75](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L75), and
[coverage line 426](ZONE_COVERAGE_REGISTER.csv#L426).

## Checkpoint 924: Nexus56 retained after room-role closure

Keep candidate row 78, Nexus56, **RETAIN IN PLACE**. Preserve all five
source files. Current role scans found no external typed room reference to
its 94 candidate-only rooms. Its own SHP has 18 locations whose numbers
resolve to active Alatorin rooms and one candidate-only location, #83076;
18 keeper VNUMs are not in active mobile sources, and keeper #83184 resolves
to a different active Alatorin mobile. Candidate chair/switch #83148 targets
room #83638, where current active Alatorin owns a differently titled room.
The conditional SHP fallback and these VNUM collisions do not establish
live loading or operation. Resolve intended keepers, locations, switch
endpoint, loader, editor, deployment, and persisted identity before any
separate activation, move, merge, or archive decision. No migration clearance
is implied. See the
[Nexus56 typed room-role audit](NEXUS56_AUDIT_REVIEW.md#checkpoint-924-exact-room-role-endpoints-and-shop-fallback-bindings),
[21 builder questions](NEXUS56_BUILDER_QUESTIONS.md),
[candidate dependency row 78](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79), and
[coverage row 429](ZONE_COVERAGE_REGISTER.csv#L430).

## Checkpoint 925: Player Guilds retained after reset-role census

Keep both `player_guilds.qst` and `player_guilds.shp` **RETAIN IN PLACE**.
The follow-up found 40 current ZON mobile reset references to four keeper
VNUMs, yet none places the matching keeper in its Player Guilds shop room;
five shop room fields have reset placements by different mobiles. Three
missing stock prototypes also have 36 current ZON object reset references.
Active code and map sources use #5541 as a room destination; a separate
mobile-callback reference is disabled under `#if 0`. These source-only
overlaps add owner and load-resolution questions; they do not establish
active shop service, catalog ownership, or safe attic eligibility. Resolve
the intended keeper and stock identities, source precedence, fallback
loader, deployment, editor, and external state before any separate shop
activation, move, merge, or archive decision. No migration clearance is
implied. See the
[Player Guilds typed reset audit](PLAYER_GUILDS_AUDIT_REVIEW.md#checkpoint-925-typed-reset-and-numeric-role-closure),
[21 builder questions](PLAYER_GUILDS_BUILDER_QUESTIONS.md),
[connectivity checkpoint 925](WORLD_CONNECTIVITY.md#checkpoint-925-player-guilds-reset-roles-do-not-add-room-edges), and
[candidate dependency row 83](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L84).

## Checkpoint 926: Ravenmis retained after Newbie role closure

Keep all five Ravenmis source files **RETAIN IN PLACE**. The follow-up
confirmed that active Newbie owns the same ten room VNUMs; nine active WLD
exits and six ITEM_TELEPORT destinations resolve to Newbie rooms, not to the
Ravenmis source map. Current Newbie QST/story sources use #29300/#29302 as
items and #29304 as a mobile; unlisted guildhall shops and resets reuse other
Ravenmis-range mobile, object, and room IDs. These findings expand the owner
and aggregate-precedence questions but establish no alias, operational route,
or archive safety. Resolve the #125119 outlet, teleporter owners, quest/story
identity, sibling shop rows, fallback/loading provenance, and external state
before any separate activation, renumber, move, merge, or archive decision.
No migration clearance is implied. See the
[Ravenmis typed role audit](RAVENMIS_AUDIT_REVIEW.md#checkpoint-926-shared-room-numbers-resolve-to-newbie-identities),
[22 builder questions](RAVENMIS_BUILDER_QUESTIONS.md),
[connectivity checkpoint 926](WORLD_CONNECTIVITY.md#checkpoint-926-ravenmis-shared-room-numbers-remain-newbie-rooms), and
[candidate dependency row 84](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L85).

## Checkpoint 927: Raxquest retained after cross-type role closure

Keep all five Raxquest source files **RETAIN IN PLACE**. The 84 room, 63
mobile, and 98 object records contain 62 numbers used in all three types.
Current `areas/` sources outside the component contain no standalone 705xx
references; code retains nine object callbacks, thirteen mobile callbacks,
the #70501 room procedure, and a dedicated Raxquest shout/summon path that
uses mobile #70536. The type-specific bindings and overlapping numbers do
not establish active loading, resolve the index-0 fallback, add an SHP
catalog, or qualify the absent QST aggregate. Resolve those code, loader,
deployment, editor, and persisted-identity questions before any separate
activation, migration, rename, merge, or archive proposal. No migration
clearance is implied. See the
[Raxquest role audit](RAXQUEST_AUDIT_REVIEW.md#checkpoint-927-cross-type-ids-keep-separate-source-and-code-roles),
[22 builder questions](RAXQUEST_BUILDER_QUESTIONS.md), and
[connectivity checkpoint 927](WORLD_CONNECTIVITY.md#checkpoint-927-raxquests-overlapping-vnum-types-add-no-wld-edges).

## Checkpoint 928: realms_master retained after typed board-number review

Keep `areas/shp/realms_master.shp` **RETAIN IN PLACE**. Its keeper/shop keys
#11108/#11109 and room values #11121/#11124 also match current Player Castles
OBJ boards, but the matching values do not supply the absent keeper MOBs or
shop-room WLD definitions. Five source ZON O rows load the board objects;
the old matching room callbacks are disabled, and a separate mobile-helper
reference to #11108 does not qualify a keeper. Resolve loader fallback and
missing-keeper handling, actual shop/room owners, and editor/deployment
provenance before any activation, move, merge, or archive proposal. The
existing conditional full-mode fallback and unguarded missing-keeper path
remain unresolved; no migration clearance is implied. See the
[Realms Master audit](REALMS_MASTER_AUDIT_REVIEW.md#checkpoint-928-shop-keeper-room-and-board-numbers-remain-typed),
[21 builder questions](REALMS_MASTER_BUILDER_QUESTIONS.md), and
[connectivity checkpoint 928](WORLD_CONNECTIVITY.md#checkpoint-928-realms_master-shop-rooms-and-board-objects-add-no-wld-edges).

## Checkpoint 929: returns mobiles remain in place after exact-role rescan

Keep both records in `areas/mob/returns.mob` **RETAIN IN PLACE**. A fresh
exact-value scan found #93400 and #93401 only in that MOB file, with no
checked-in WLD, OBJ, ZON, QST, SHP, sidecar, mini-area, or C/C++ consumer.
This confirms there is no current source-defined placement or direct
cross-type dependency, but does not qualify generated `world.mob`, dynamic
spawning, external scripts, editor/export inputs, or saved state. Resolve
those sources and the tower/mobile owners before any activation, rename,
move, merge, or archive proposal. No migration clearance is implied. See
the [returns role audit](RETURNS_AUDIT_REVIEW.md#checkpoint-929-both-mobile-vnums-remain-isolated-in-current-sources)
and [20 builder questions](RETURNS_BUILDER_QUESTIONS.md).

## Checkpoint 930: rf_gh retained with portal pair and role collisions

Keep all five rf_gh files **RETAIN IN PLACE**. The source ZON places two
ITEM_TELEPORT objects that point reciprocally between WLD rooms #28905 and
#28912, crossing the candidate's two WLD-only groups. Both object IDs and
both destination room IDs have different same-number Torg definitions, so
the pair cannot establish active travel or load ownership. Preserve the
typed object values and reset rows; resolve both areas' room/object owners,
reset and aggregate inputs, and the existing SHP fallback risk before any
activation, move, merge, or archive proposal. The WLD graph metrics remain
unchanged, and no migration clearance is implied. See the
[rf_gh portal audit](RF_GH_AUDIT_REVIEW.md#checkpoint-930-reciprocal-portal-objects-cross-the-two-wld-groups),
[21 builder questions](RF_GH_BUILDER_QUESTIONS.md), and
[connectivity checkpoint 930](WORLD_CONNECTIVITY.md#checkpoint-930-rf_gh-teleport-destinations-stay-outside-wld-exit-counts).

## Checkpoint 931: rf_op retained with type-qualified portal references

Keep all four `rf_op` files **RETAIN IN PLACE**. Preserve separate WLD,
MOB, and OBJ definitions sharing #29510/#29511, and the existing ZON rows
that place OBJ #29510 and four mobiles. OBJ #29510 targets #29505; OBJ
#29511 targets room #29512 but has no ZON placement. Unlisted Legion OBJ
#29500 also targets its own unlisted room #29512; do not count that as active
inbound travel to `rf_op`. Resolve the portal destinations, source manifests,
table inputs, and reset ownership before activation or path changes. No
migration clearance is implied. See the
[rf_op typed-role audit](RF_OP_AUDIT_REVIEW.md#checkpoint-931-same-number-records-keep-separate-typed-roles)
and [21 builder questions](RF_OP_BUILDER_QUESTIONS.md).

## Checkpoint 932: scrubs_hall retained pending shared #6089 ownership

Keep all five Scrubs files **RETAIN IN PLACE**, including the empty OBJ
file. Preserve the current #6089 shop-stock and ZON G rows while owners
identify whether a canonical OBJ record is supplied outside the checked-in
full and mini sources. The number appears in dozens of other shop and reset
sources, including full-manifest members, while active Lizard WLD #6089 is a
different type. Resolve shared object, aggregate, and deployment ownership
before any source-specific stock or path proposal. The `world.shp` fallback
and missing active #29103 keeper remain separate loader questions. No
migration clearance is implied. See the
[scrubs_hall #6089 audit](SCRUBS_HALL_AUDIT_REVIEW.md#checkpoint-932-product-6089-is-a-shared-unresolved-source-reference)
and [21 builder questions](SCRUBS_HALL_BUILDER_QUESTIONS.md).

## Checkpoint 933: shadow_hall retained with typed Player Castles imports

Keep all four `shadow_hall` source files **RETAIN IN PLACE**. Preserve the
two ZON O rows that import active Player Castles objects #11110 and #11060;
the latter is bound to the `magic_mouth` object special. Preserve the
separate code-side #11110 mobile-helper references pending owner review,
since no same-number MOB #11110 prototype is checked in and code assignment
does not establish runtime reachability. The same objects are reset by
unlisted `bastard` and `sky_gh` sources; confirm each source owner and the
helper-list intent before any activation or path proposal. No migration
clearance is implied. See the
[shadow_hall import audit](SHADOW_HALL_AUDIT_REVIEW.md#checkpoint-933-player-castles-imports-have-separate-code-consumers)
and [21 builder questions](SHADOW_HALL_BUILDER_QUESTIONS.md).

## Checkpoint 934: shady2 retains mobile source and typed item boundaries

Keep candidate row 92, `shady2`, **RETAIN IN PLACE** and separate from
active Shady. Preserve all 90 candidate mobile records and the active
Shady/Alatorin item contracts for #97514, #97549, #97570, #97572, and
#97582 as active OBJ references. #97549 also has separate active Shady and
candidate baby-orc MOB records. Resolve source owner, editor/export and
deployment inputs, and persisted-reference closure before any merge, move,
rename, activation, or archive proposal. No WLD or exit finding changed. See
the [shady2 typed-reference audit](SHADY2_AUDIT_REVIEW.md#checkpoint-934-quest-and-story-item-ids-remain-object-typed) and [21 builder questions](SHADY2_BUILDER_QUESTIONS.md).

## Checkpoint 935: sky_gh retains field-specific Harrow identities

Keep candidate row 93, `sky_gh`, **RETAIN IN PLACE** with all five source
files, its empty OBJ file, nine WLD exits, two Player Castles imports, and
shop data. Harrow's story `mob_vnum: 29404` is an active mobile contact;
its same-number exchange reward is an OBJ. Candidate #29404 remains the
Hall's Armory room and a source SHP location, whose current numeric room
lookup resolves to Harrow if parsed. Preserve those separate typed roles
while owners resolve the existing map, reset, shop, fallback, and history
questions. No activation, move, archive, or migration clearance is implied.
See the [sky_gh typed-role audit](SKY_GH_AUDIT_REVIEW.md#checkpoint-935-harrow-contact-and-item-roles-stay-separate) and [21 builder questions](SKY_GH_BUILDER_QUESTIONS.md).

## Checkpoint 936: Surf retains separate QST and prototype identities

Keep Surf's four source files **RETAIN IN PLACE** and separate from
`surface2011` and `underdark2011`. Their unlisted QST files contain 37 plus
one mobile-key header and 56 plus four item operands overlapping Surf's
#200000–#200321 mobile range. Neither QST file is in `areas/qst/AREA`;
their item operands resolve to source OBJ records in those unlisted source
families, and Surf's OBJ file is empty. Preserve the typed source boundaries
while resolving loader and owner provenance. No WLD graph, active-ingress,
or migration disposition changed. Keep both QST files attached to their
source-owning components; see the [Surf typed-boundary audit](SURF_AUDIT_REVIEW.md#checkpoint-936-old-surface-qst-values-do-not-identify-surf-mobiles), [Surface2011 questions 21](SURFACE2011_BUILDER_QUESTIONS.md), [UnderDark2011 question 31](UNDERDARK2011_BUILDER_QUESTIONS.md), and [Surf questions 21](SURF_BUILDER_QUESTIONS.md).

## Checkpoint 943: Marigot shop and reset imports stay with candidates

Keep `nexus56` and `solstice_gh` **RETAIN IN PLACE**. Preserve Nexus56's source-only `G` reset and shop product for Marigot instrument #5319, and Solstice GH's source-only `G` resets and shop products for Marigot objects #5319/#5322. Marigot remains the prototype owner. These unregistered candidate records do not establish active resets or live shop stock; resolve owner, loader, editor, and deployment intent before any separate activation, merge, move, or archive decision. No WLD graph or active-ingress count changed. See the [Marigot typed/code review](MARIGOT_STORY_REVIEW.md#checkpoint-943-bounded-typed-and-compiled-code-closure), [Nexus56 audit](NEXUS56_AUDIT_REVIEW.md#checkpoint-943-source-only-marigot-shop-and-reset-references), [Solstice GH audit](SOLSTICE_GH_AUDIT_REVIEW.md#checkpoint-943-source-only-marigot-shop-and-reset-references), and candidate dependency [rows 79 and 95](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79).

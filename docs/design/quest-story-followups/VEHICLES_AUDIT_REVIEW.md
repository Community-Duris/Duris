# Vehicles typed and code-route audit review

Checkpoint 875 closes the bounded tracked-source dependency review for active vehicles against source snapshot 72ba3deff3bb30bde880018cdebe7c6db675b697. The detailed [Vehicles story review](VEHICLES_STORY_REVIEW.md) remains the source for the 255 local QA contracts and their exact terms; the [20 builder questions](../zone-stories/VEHICLES_BUILDER_QUESTIONS.md) retain the open owner and runtime questions. [Checkpoint 770](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-770-vehicles-title-and-scope) remains the title and followed-history decision.

## Static WLD map and code-defined routes

The active WLD contains 216 rooms and 464/464 loader-resolving exits, in eight weak components. It has no active cross-area WLD boundary, no inactive-WLD target into its room set, and no source-room VNUM overlap. Five rooms carry fall fields and none carries a current field. These are static WLD facts; moving ferries and code-defined transport stops are separate dependencies.

The ferry definitions in ferryact.c name these external stop VNUMs and current active WLD owners:

| Code-defined service | Named stops and active WLD owners |
| --- | --- |
| Wave Dancer | #635261 Surface; #76654 Jade Empire (listed three times); #1712 Quietus; #82686 Mira |
| Seaspray | #22444 Stormport; #550724 Surface |
| Stalval | #83787 Alatorin; #564319 Surface |
| Crimson Fulgur | #83788 Alatorin; #9979 Sarmiz’Duul |
| Ironhold Grudge | #83789 Alatorin; #49089 Desert |
| Old Rickety Ferry | #133052 Tharnadia; #557216 and #550723 Surface |
| Stromvok | #22445 Stormport; #66688 Torrhan; #30929 Stormht |

The stop arrays are at [ferryact.c lines 98–138](../../../src/world/ferryact.c#L98); ferry initialization is called from [comm.c line 974](../../../src/net/comm.c#L974). The constructor resolves stop VNUMs and asks dijkstra for each leg’s ship path ([ferry.c lines 87–93](../../../src/world/ferryact.c#L87), [path construction](../../../src/world/ferry.c#L126)). The tables identify authored destinations, not successful live paths, boarding, route availability, fare behavior, or safe return. The repeated Jade Empire stop and the Seaspray auxiliary room range remain the explicit builder questions; do not normalize these definitions from the static WLD census.

The separate transport service maps 16 local Vehicles mobile prototypes to active Surface loading rooms in [transport.c lines 119–143](../../../src/world/transport.c#L119): #47027→#619771, #47015→#576577, #47016→#606262, #47017→#635756, #47018→#588326, #47021→#543701, #47025→#582084, #47022→#514072, #47020→#580080, #47019→#545407, #47023→#622276, #47024→#563522, #47028→#519043, #47030→#543662, #47026→#522247, and #47029→#631413. Its configured route table is also in transport.c at lines 50–116. These code references are not authored Vehicles WLD edges.

Some numeric room identities also occur in inactive WLD sources. Ferry stop #133052 is owned by active Tharnadia while inactive Duris3 contains the same number; #635261, #619771, #620606, #606262, #635756, #622276, and #631413 are active Surface identities with source copies in newbiemaps and/or surface2011-temp. The compiled VNUM resolves through the current active WLD set; preserve the inactive copies on their current candidate plans as separate source identities.

The older wagon route literals #47000/#47001 and rooms #157026/#152728/#150777/#154478 appear in mount.c, but init_wagons returns unconditionally at line 647 before applying the hooks or navigation table. They are not counted as current wagon routes ([disabled function](../../../src/classes/mount.c#L641)).

## QST inputs, rewards, and external object owners

The local Vehicles QST consumes exact object VNUMs outside its local OBJ set:

| Contract item family | External prototypes | Active owner |
| --- | --- | --- |
| Meat inputs accepted by the three cooks | #83223, #83220, #83116, #83117, #83201 | Alatorin OBJ; representative input records begin at [vehicles.qst lines 33–57](../../../areas/qst/vehicles.qst#L33) |
| Fish-pair inputs across the three cooks | #293, #294, #295, #318, #319, #330, #332, #333, #334, #335, #355, #356 | Heavens OBJ; the repeated pair matrix begins at [vehicles.qst line 65](../../../areas/qst/vehicles.qst#L65) |

These are written item requirements, not proof that the items are in stock or that a player can safely reach a cook. The five meat VNUMs also occur in inactive alatorin_objects.obj and nexus56.obj: alatorin_objects repeats the Alatorin meat definitions, while Nexus56 uses different prototype identities at the same numbers. Only Alatorin is the active owner. Keep those candidate collisions attached to their existing [Alatorin Objects row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L6) and [Nexus56 row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79); they do not make either candidate a live food source.

Local contract outputs and crafting inputs also cross source ownership. Captain/stranger rewards use WH objects #55362/#55033 and Tradeskills objects #400233/#400235. Kjelgar’s recipes consume Tradeskills objects #400069/#400179; the five crystal-input commands instead name undefined #400941, while active Tradeskills defines #400094 as a similar moonstone crystal. Do not substitute or alias the VNUMs without the builder’s resolution; see the [contract and recipe review](VEHICLES_STORY_REVIEW.md#finding-2-a-three-ship-rivalry-and-a-second-flag-exchange) and [Kjelgar review](VEHICLES_STORY_REVIEW.md#finding-3-kjelgars-three-crafting-offers). Tradeskills.old.obj repeats #400069, #400179, and #400233, but the active owner remains tradeskills.obj. Keep the duplicate under its existing [Tradeskills Old candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L108).

## Vehicles ZON imports and unresolved mobile VNUMs

A token-aware scan of vehicles.zon found 81 reset operands to 46 defined external prototypes across 17 active source areas:

| Active prototype owner | Imported prototype VNUMs and reset line groups |
| --- | --- |
| Alatorin | OBJ #83116/#83117/#83201/#83220/#83223; lines 477–497, 646–658, 742–765 |
| Wh | MOB #55246/#55550–#55554; lines 549/644/753 and 812–816 |
| Heavens | OBJ #830–#836 and #838; lines 535–602, 702–704, 818–820 |
| Ship Yards | OBJ #43137/#43138/#43140/#43142; lines 641–643, 750–761 |
| Surface | OBJ #500026; lines 499/501, 746/748, 804–807 |
| Gold Hall | OBJ #40444; lines 518, 545–546, 663, 776 |
| Mira | OBJ #82545/#82553/#82554/#82555/#82559; lines 826–827, 842–844 |
| Hunt | OBJ #13320/#13321; lines 829–830, 835–837 |
| RogueRai | OBJ #75856/#75857; lines 828/841 |
| SPS Hold | OBJ #22621/#22631; lines 825/840 |
| Celestia | OBJ #45530/#45531; lines 831–832 |
| Cosmic | OBJ #76050/#76066/#76243; lines 846–848 |
| CerebusP | OBJ #22063; line 833 |
| Desolate | OBJ #22237; line 834 |
| Jade | OBJ #76634; line 845 |
| 4Horse | OBJ #34541; line 839 |
| Tradeskills | OBJ #400235; line 851 |

The source commands identify reset ownership only. They do not establish that the reset executes, that its command is admitted, or that a reset item is available. The exact imports from Cosmic and Desolate remain attached to their existing owner reviews in [coverage notes](ZONE_COVERAGE_REGISTER.csv#L337).

Two additional groups of local M commands have no exact mobile prototype in any tracked MOB input: #6124 at vehicles.zon lines 407–409 and #6123 at lines 416–417. Although the source comments note that these mobiles are absent from vehicles.mob, the complete tracked-source scan found no external owner either. Preserve both as unresolved reset references; do not call them verified external prototypes.

## SHP inventory and inbound-consumer scans

vehicles.shp contains four shop records: #47061, #47070, #47097, and #47116. Their 16 product slots include eight Heavens imports (#830–#836 and #838) and eight local Vehicles objects. The current coverage inventory reports zero shop records; that count discrepancy remains open under builder question 17. Keep the source records intact and do not infer that all four services are loaded or available.

No external active ZON reset targets a Vehicles MOB or OBJ prototype. No external QST item command, SHP product record, or structured story item_vnums field targets a Vehicles OBJ prototype. No duplicate external MOB/OBJ prototype definition was found. A census of all 713 active type-25 teleport objects found no destination in the Vehicles room set.

A tracked C/C++ numeric scan found local Vehicles OBJ #47081/#47082 in static chaos-equipment arrays and #47092 in two more static gear slots ([array entries](../../../src/account/chaos_eq_data.h#L141), [cloak entries](../../../src/account/chaos_eq_data.h#L252)). These are table values only; they do not prove selection, creation, or issuance. ferry.h also binds local ticket #47003 and automat #47006 to the compiled ferry system. No other direct source occurrence established a local custom handler.

## Identity and disposition

Checkpoint 770 records one exact current owner for the lowercase vehicles title and no distinct followed same-stem predecessor or replacement. The same contributor contains the ships, transport objects, tickets, mounts, and shipboard scenes; thematic subgroups do not establish separate zone owners.

Keep active vehicles **DO NOT MOVE** and add no Vehicles candidate. Preserve the code-defined ferries and transport mappings as separate from the static WLD graph; preserve all external reset, QST, SHP, and candidate-identity dependencies under their current owners. The #6124/#6123 resets, #400941 recipe mismatch, four-shop inventory discrepancy, dynamic route/path behavior, live access, deployed availability, and owner intent remain open. No source move, route change, or gameplay change is included in this review.

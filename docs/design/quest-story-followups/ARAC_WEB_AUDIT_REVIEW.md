# Arachdrathos repository-wide source audit

This current-source supplement reconciles the [Arachdrathos source-comprehensive dossier](../zone-stories/ARAC-WEB.md) with active boundaries, directed reachability, inactive-WLD approaches, typed references, compiled numeric contexts, title, and bounded same-stem history. It uses the pinned source snapshot 72ba3deff3bb30bde880018cdebe7c6db675b697. No gameplay or area source is changed by this audit.

## Active map and boundary routes

The active arac-web WLD has 190 rooms and 457 authored exits: 446 local and 11 outbound cross-area edges. All 457 targets resolve. The local map is one weak component, has no raw flag-8 exit, and directed traversal from #36400 reaches 189 rooms; all 190 rooms have a directed path back to the root. The only room outside root-forward WLD traversal is #36568, which the existing dossier identifies as Body Traders' adjacent real-room-plus-one stock room behind shop room #36567. That exception is not evidence of a broken public entrance or of every possible computed route.

All 11 outbound WLD edges have reciprocal active records:

| External area | Arachdrathos route | Boundary evidence |
| --- | --- | --- |
| aracguil | Seven north exits from #36427, #36431, #36444, #36447, #36453, #36482, and #36506 | [outbound and reverse pair rows 17 and 23](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L17) |
| llzazan | #36546 south, #36573 east, and #36582 east | [outbound and reverse pair rows 18 and 252](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L18) |
| arac_wild | #36588 west to #36197 | [outbound and reverse pair rows 16 and 19](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L16) |

The 13 active inbounds consist of those 11 reciprocal returns plus two one-way Heavens exits into inn room #36564: #15 down and #27 south ([Heavens pair row 170](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L170), [exact exit records](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L3)). The Heavens #15 exit text says it leads to Underdark even though its authored target is Arachdrathos; the displayed direction text needs an owner review. Arachdrathos' exact outbound source records are in [cross-area register rows 482–492](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L482). These static WLD routes do not qualify current door state, access policy, arrival, return safety, or service completion.

The full active WLD duplicate scan still finds only the unrelated room #51800 collision between Hell Two and Hell Three ([Hell Three](../../../areas/wld/hell_three_minauros.wld#L1), [Hell Two](../../../areas/wld/hell_two_dis.wld#L10375)); no arac-web room VNUM is duplicated in active WLD sources.

## Inactive-WLD scan

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits without parser issues. It found no inactive source-room VNUM overlap with arac-web and no inactive-WLD target into rooms #36400–#36589. No source-only WLD approach is added, and the curated source-only total remains 3,332 overall, including 163 reviewed Surf approaches and 15 reviewed ud_map2 approaches. These counts do not establish dynamic or code-defined routes.

## Typed references and compiled numeric contexts

External QST item commands, SHP product rows, and STORY fields contain no exact typed reference to arac-web rooms, mobiles, or objects. The active external ZON scan found seven Alatorin P resets for arac-web objects: #36415, four #36416 instances, #36418, and #36419, nested in the bag at room #83327 ([Alatorin reset rows](../../../areas/zon/alatorin.zon#L2864)). This is active cross-area prototype use and must remain in dependency closure.

Two unregistered source groups also name local Arachdrathos prototypes:

- nexus56.zon G-resets local objects #36427–#36430 at lines 1356–1359 ([source](../../../areas/zon/nexus56.zon#L1356)). These are source-only reset declarations, not active placements; retain nexus56.
- ud_map0.zon M-resets local mobile #36431 in ud_map0 room #1104644 ([source](../../../areas/zon/ud_map0.zon#L21)). This is separate from ud_map0's two active unresolved Shafts WLD references; retain ud_map0 and preserve the mobile dependency.

Arachdrathos' own ZON file imports nine object prototypes outside its 44 local objects: four O rows for #90, #361, #3097, and #92, plus five G rows for #500, #363, #368, #369, and #54 ([O reset rows](../../../areas/zon/arac-web.zon#L34), [G reset rows](../../../areas/zon/arac-web.zon#L92), [later G rows](../../../areas/zon/arac-web.zon#L218)). Each has an object definition elsewhere in the current source set. Three local SHP shops also list five external products: #363, #368, and #369 at shop #36419, #54 at shop #36439, and #500 at shop #36440 ([shop products](../../../areas/shp/arac-web.shp#L124), [crossbow shop](../../../areas/shp/arac-web.shp#L218), [disguise shop](../../../areas/shp/arac-web.shp#L256)). The local story sidecar has no out-of-zone item_vnums. All other 231 story sidecars parse without errors and contain no exact numeric token matching Arachdrathos' local prototypes.

The C/C++ numeric scan found these typed uses:

- Object #36419 is the default account-reward object; its consumer validates it with real_object, so its same-numbered mobile is a separate namespace ([default](../../../src/account/account_reward.h#L4), [object lookup](../../../src/account/account_reward.c#L1990)).
- Room #36544 is a CHAOS port destination with aliases arach, arachdrathos, and drow ([port table](../../../src/combat/chaos.c#L42), [room lookup](../../../src/combat/chaos.c#L64), [move](../../../src/combat/chaos.c#L81)).
- The Drow justice row uses rooms #36572, #36563, and #36544 as guard locations and mobile #36436 as the guard prototype ([field meanings and typed row](../../../src/combat/justice.c#L29)).
- Mobile #36420 receives world_quest in two assignments; #36413 receives money_changer. Rooms #36564 and #36567 receive the inn and pet-shop callbacks. Mobiles #36440, #36441, and #36442 have explicit zero assignments in the default spec tables ([world quest](../../../src/specs/specs.assign.c#L729), [money changer](../../../src/specs/specs.assign.c#L813), [room callbacks](../../../src/specs/specs.assign.c#L2316), [mobile defaults](../../../src/specs/specs.assign.c#L425)).
- Two additional scan hits are comments rather than active bindings: old evil-room #36539 in nanny.c and old postmaster #36439 in mail.c ([nanny comment](../../../src/account/nanny.c#L1207), [mail comment](../../../src/cmd/mail.c#L133)).

Across all other mobile and object files, no duplicate definition of an Arachdrathos local prototype was found. None of the 872 external type-25 declarations targets an Arachdrathos room. These source hits are dependency evidence; they do not establish active accounting, purchases, learned skills, justice outcomes, travel, or saved player results.

## Title, history, and disposition

The normalized title “Arachdrathos - Drow City” is unique among 442 scanned .zon files. Lexical neighbors are Arachdrathos Wilderness (arac_wild), Arachdrathos Guilds (aracguil), and Llzazan Ghetto of Arachdrathos (llzazan); their titles and source ownership are distinct. The title source is [arac-web.zon](../../../areas/zon/arac-web.zon#L2).

Followed arac-web.wld history reaches the 2009 directory relocation and same-stem 2010/2013 edits. Followed arac-web.zon history has the same relocation, same-stem 2011/2013 changes, the 2015 quest-item-flag update, and a 2017 Beligron Earth Reaver spec change. These bounded paths show no alternate-stem rename, explicit predecessor, or whole-zone replacement. Keep active arac-web DO NOT MOVE. Preserve the active Alatorin reset use and the nexus56/ud_map0 source-only reset references; keep nexus56 and ud_map0 RETAIN IN PLACE pending full typed, tool, editor, and owner closure.

The source-mapping queue is now 195/220 complete, with 25 pending; Priority 196, Arcium (arcium), is next.

Validation: Checkpoint 155 documentation validation checked 132 changed/added Markdown files; all 5,057 local link targets and 2,392 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

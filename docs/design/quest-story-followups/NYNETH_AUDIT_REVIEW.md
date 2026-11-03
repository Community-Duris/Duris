# Ny'Neth's Stronghold: route, dependency, and history supplement

This bounded static review accompanies the [first-stage source dossier](../zone-stories/NYNETH.md). It reconciles active and source-only WLD approaches, conditional portal routes, local reset/story claims, typed cross-area dependencies, compiled prototype bindings, title ownership, and followed same-stem history. It does not establish installed controllers, runtime arrival, or played outcomes.

## Active WLD graph and approaches

Active `nyneth` has 78 rooms (#22900–#22996) and 175/175 loader-resolving exits: 174 local and one outward boundary, #22900 south to Surface #581417 ([source room](../../../areas/wld/nyneth.wld#L1), [Surface room](../../../areas/wld/surface.wld#L1709097), [exit-register row 379](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L379)). Surface #581417 has the reciprocal north exit into #22900 ([exit-register row 1521](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1521)).

The raw WLD graph has two weak components of 67 and 11 rooms. From #22930, authored local exits reach 65 rooms in the larger component and 66 can return to it. Rooms #22995 and #22996 are not reachable from that root; #22996 has no path back. The separate 11-room entrance component is reachable and returnable from #22900. No local WLD edge joins the entrance to #22930; the reset portal object #22902 conditionally joins those components. These counts assume authored exits are available and do not prove door admission, portal activation, arrival, or a safe route. The WLD records declare no fall chance or current.

There are two active inbound WLD records. The reciprocal Surface exit enters the ordinary stronghold entrance. White Harbor room #55633, titled “First teleport room for gnomish key,” has a down exit to first vault #22994 and no reciprocal Nyneth exit ([source room](../../../areas/wld/wh.wld#L13037), [exit-register row 793](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L793)). Treat this multi-exit technical room as a controller lead, not verified ordinary ingress; its installed trigger, intended users, and safe return are unresolved.

All 95 WLD files outside active `areas/AREA` parsed without issues: 527,791 rooms and 2,080,773 direction records. There is no inactive room-VNUM overlap with Nyneth. One inactive source-only exit enters the area: `surf.wld` room #284533 north to #22900 ([source room and exit](../../../areas/wld/surf.wld#L1773829)). It is not active ingress. The existing `surf` candidate row already tracks this source; preserve the edge there and raise the curated source-only count by one, from 4,247 to 4,248 overall and from 250 to 251 for Surf. No new candidate row is needed.

Name-based WLD text needs room-ID context. Surface room #581017 is titled “Nyneth” and its south exit leads to Surface #581417, the rocky foothills room whose north exit enters #22900 ([Surface room](../../../areas/wld/surface.wld#L1700701), [boundary room](../../../areas/wld/surface.wld#L1709097)). This is a Surface-local approach to the stronghold, not a second area owner. Realm room #14208 labels its southwest exit “Southwest to Nyneth,” but its target is #38811 in Nyneth3, not first-stage #22900 ([Realm exit](../../../areas/wld/realm.wld#L4672), [Nyneth3 inbound review](NYNETH3_AUDIT_REVIEW.md#active-wld-boundaries-and-directed-reach)). Keep the direction prose and numeric targets distinct when describing campaign access.

## Conditional portal routes and reset gates

The active type-25 object routes that extend the raw graph are:

| Object | Reset placement | Declared target | Source role |
| --- | --- | --- | --- |
| #22902 | #22908 | #22930 | First portal into the main complex ([prototype](../../../areas/obj/nyneth.obj#L17), [reset](../../../areas/zon/nyneth.zon#L97)) |
| #22903 | #22909 | #23000 | Portal to Nyneth2 ([prototype](../../../areas/obj/nyneth.obj#L25), [reset](../../../areas/zon/nyneth.zon#L98)) |
| #22904 | #22910 | #38700 | Portal to Nyneth3 ([prototype](../../../areas/obj/nyneth.obj#L33), [reset](../../../areas/zon/nyneth.zon#L99)) |
| #22966 | #22994 | #22908 | Dark return portal to the entrance ([prototype](../../../areas/obj/nyneth.obj#L403), [reset](../../../areas/zon/nyneth.zon#L106)) |

Each uses the `enter` command selector (7) and has unlimited charges (-1). The generic `check_item_teleport` path in [spell_travel.c](../../../src/magic/spell_travel.c#L932) checks the item type, command, destination, and travel admission before calling the teleport path; positive charges are decremented separately. These declarations explain a conditional route, not a qualified player journey. The two onward portals and the return object #22966 also connect distinct campaign stages without making the first stage's story depend on a completed Nyneth2 or Nyneth3 exchange.

The local ZON applies 22 `D` rows to eleven paired doorways. Examples include the Ny'Neth key #22900 at the entry pair, wyrm key #22901 between the altar and portal passage, straw key #22930 around the Hall of Titans, lightning key #22931 at the Hall of Titans/dead boundary, and vault key #22965 before #22994. Reset rows carry closed, locked, or secret state. Keep each paired WLD exit and reset row together until builders verify the intended key, flag, and return behavior; static source does not qualify pick/bash controls or an actual traversal.

## Story text and reset questions

The area has 32 local mobile prototypes and 47 objects but no local QST giver or contract, no SHP file, and no `nyneth.story.json`. `areas/qst/AREA` lists `nyneth` at line 109, although `areas/qst/nyneth.qst` is absent. Thus the local source defines an environmental and encounter story, not a native tracked contract.

The audience room #22993 describes a construct holding Ny'Neth's mind; the first vault #22994 is presented as a trophy room after that encounter. The reset stock includes local vault guardian #22904 and the imported Heavens monolith object #360 ([first-vault reset rows](../../../areas/zon/nyneth.zon#L106)). The monolith is governed by the separate Heavens epic award path. Neither the boss's defeat nor the presence of the monolith establishes a saved liberation or reward receipt.

The rareload notes expose a concrete source mismatch. Room #22995 says “25% chance of loading each mob” ([room](../../../areas/wld/nyneth.wld#L1591)), while five local `M` rows place Vlessus, Tssiss, Drkehn, Nian, and Agaroch there with percent fields set to 100 ([reset rows](../../../areas/zon/nyneth.zon#L376), [#22996](../../../areas/zon/nyneth.zon#L385)). Room #22996 says rareloads that will not enter the grid ([room](../../../areas/wld/nyneth.wld#L1614)), but no local reset command targets #22996. In #22995, the prose says north leads to the lava grid; the actual north exit leads to #22993, east leads to lava-grid room #22975, and south and west lead to #22996. The source alone does not explain whether these statements are stale text, intended off-grid staging, or a runtime-managed rareload system.

## Exact typed dependencies and compiled bindings

An exact context-aware scan against the 78 room IDs, 32 mobile IDs, and 47 object IDs found no external QST item command, SHP product, external type-25 declaration targeting a local room, duplicate external mobile/object prototype, or numeric-ID match in external story sidecars. It did find 27 active external ZON references to local prototypes:

| Active source | Exact reset references into Nyneth | Count | Reset evidence |
| --- | --- | ---: | --- |
| Alatorin | `G` resets use object #22944 twice and #22938 once. | 3 | [#22944 rows](../../../areas/zon/alatorin.zon#L2061), [#22938 row](../../../areas/zon/alatorin.zon#L3863) |
| Earth Plane | `G` resets use object #22936. | 12 | [first and last rows](../../../areas/zon/earthp.zon#L93), [#L360](../../../areas/zon/earthp.zon#L360) |
| Ravenloft2 | `P` resets use object #22940. | 3 | [reset rows](../../../areas/zon/ravenloft2.zon#L1852) |
| Stormhold | `P` resets use #22938 three times and #22948 once; `G` resets use #22938 twice. | 6 | [#22938 and #22948 container rows](../../../areas/zon/stormht.zon#L182), [#22938 mob-carried rows](../../../areas/zon/stormht.zon#L411) |
| Nyneth2 | `O` reset uses object #22966 and `M` reset uses mobile #22904 in the second vault. | 2 | [object reset](../../../areas/zon/nyneth2.zon#L144), [mobile reset](../../../areas/zon/nyneth2.zon#L396) |
| Nyneth3 | `M` reset uses mobile #22904 in the later vault. | 1 | [mobile reset](../../../areas/zon/nyneth3.zon#L241) |

Nyneth2's use of #22966 consumes the first-stage dark portal targeting #22908. The shared mobile #22904 is the Nyneth vault guardian; preserve it separately from the same-numbered object #22904, the Nyneth3 portal. Nyneth3 also resets that guardian mobile as later-stage stock.

Two direct compiled bindings refer to local prototypes: mobile #22956 is assigned `construct` in [specs.assign.c](../../../src/specs/specs.assign.c#L1249), and object #22962 is assigned `hammer_titans` ([assignment](../../../src/specs/specs.assign.c#L1588)). Their procedure details are in [specs.nyneth.c](../../../src/specs/specs.nyneth.c#L22). These are combat/item behavior bindings, not quest-completion consumers. No source-only map change may drop shared prototypes #22936, #22938, #22940, #22944, #22948, #22962, #22966, or mobile #22904 without replacing or explicitly retiring the dependent content.

## Title, bounded history, and retention

After color stripping, the title “Ny'Neth” belongs only to active stem `nyneth` among 442 `.zon` titles ([title source](../../../areas/zon/nyneth.zon#L2)). Followed WLD, MOB, OBJ, and ZON histories remain on the same stem and reach the 2009 move into the current `areas/` tree; later commits are same-stem maintenance and content edits, including room/title corrections, key and reset fixes, and the 2015 quest-item flag pass. These bounded histories establish no alternate-stem rename, distinct predecessor, or whole-zone replacement. `nyneth2` (“Ny'Neth's Stronghold”) and `nyneth3` (“Ny'Neth's Stronghold Continued”) are separate active sequel stages. Portals and reused prototypes are campaign/dependency evidence, not replacement proof.

Keep active `nyneth` **DO NOT MOVE**. Preserve the reciprocal Surface entrance, the White Harbor technical inbound pending controller review, all four local portal objects, paired door/reset rows, the imported Heavens monolith, the #22995/#22996 rareload records, and the cross-area mobile/object consumers above. Preserve inactive `surf.wld` room #284533 north to #22900 on the existing Surf candidate row; it remains source-only. No candidate row is added and no active route or gameplay change is proposed. Editor, administrative, installed-data, persisted-player, indirect-runtime, and played-traversal closure remain outside this static review.

## Checkpoint 315: twenty direct builder questions and recommendations

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| NY-01 | Is the intended story outcome to free, restore, or otherwise resolve Ny'Neth's mind after the construct encounter? | Keep the text as lore until an admitted action, proof, beneficiary, and saved result are named. |
| NY-02 | Is `nyneth` intentionally listed in `areas/qst/AREA` without a local QST file or story sidecar? | Confirm the manifest entry's intent; add a native contract only after its objective, eligibility, reward, and replay policy are specified. |
| NY-03 | Is Surface #581417 the ordinary player entrance to the stronghold, and is the reciprocal return sufficient? | Preserve the reciprocal boundary and qualify the actual arrival and safe return before promising access. |
| NY-04 | What controller admits travelers from White Harbor #55633 down to vault #22994? | Treat it as a technical lead until its trigger, intended users, prerequisite, and safe exit are confirmed. |
| NY-05 | Is inactive `surf.wld` room #284533 north to #22900 a historical approach builders still intend to preserve? | Preserve the source-only record on the existing Surf candidate row; do not count it as active ingress. |
| NY-06 | Is portal object #22902 at #22908 the intended only authored route from the 11-room entrance component to the main complex? | Keep the WLD components and portal distinct; document the installed admission and return path before changing either. |
| NY-07 | Should the unlimited portals #22903 and #22904 lead to Nyneth2 and Nyneth3 for any traveler who reaches them? | Preserve their declared targets; specify any campaign gate or prerequisite before adding one. |
| NY-08 | Is dark portal #22966 in the first vault the intended return to #22908, including its reuse in the Nyneth2 second vault? | Preserve the shared prototype and destination; qualify the actual arrival and a safe path back through the campaign. |
| NY-09 | Which source and action are intended to provide Ny'Neth key #22900 for the entry doors? | Retain both keyed exit records until the key source and intended access policy are verified. |
| NY-10 | Is wyrm key #22901 meant to gate both directions between the altar and portal passage? | Preserve the paired reset rows; confirm the key source and whether either side should lock travelers in. |
| NY-11 | Are straw key #22930 and lightning key #22931 meant to control the Hall of Titans and Cavern of the Living Dead boundaries as authored? | Keep the current door pairs until builders confirm the key order, secret state, and return behavior. |
| NY-12 | Is vault key #22965 the intended prerequisite for entering the first trophy room? | Preserve the audience-room/vault pair and confirm where the key is obtained and whether exit is always available. |
| NY-13 | Are all eleven paired `D` reset groups intentionally closed, locked, or secret on reset? | Review each pair against its opposite WLD direction, exact key, and desired bash/pick behavior before editing. |
| NY-14 | Does the five-mob #22995 staging population really have a 25% chance, or should the source text describe its `M` reset rows whose percent fields are 100? | Resolve the reset/runtime rate and text together; do not change rarity based on prose alone. |
| NY-15 | What is the intended use of #22996, which is described as an off-grid rareload room but has no local reset target? | Keep it as source-defined staging until a producer, route, and intended population are identified. |
| NY-16 | Should #22995's north/east/south/west prose be corrected to match its WLD targets (#22993, #22975, and #22996)? | Confirm whether the exits or text are stale, then update the chosen representation as one reviewed content change. |
| NY-17 | Is the `construct` procedure on mobile #22956 intended to be the encounter that resolves the mind story? | Preserve its combat behavior; add a story result only after a builder names the accepted proof and beneficiary. |
| NY-18 | Is Hammer of Titans #22962's lightning proc part of the intended Hall of Titans story, and what effect should count as useful completion? | Keep the item procedure separate from encounter credit; define a specific action/result before tracking it. |
| NY-19 | Is imported Heavens monolith #360 meant to be available as first-vault trophy stock, with its current epic claim rules? | Preserve the exact imported object and its Heavens award flow; qualify placement and committed recipients independently from boss defeat. |
| NY-20 | Should the Surface locale titled “Nyneth” and Realm's “Southwest to Nyneth” label be documented as different stage references? | Preserve the room targets: Surface #581017 leads via #581417 to first-stage #22900, while Realm #14208 points to Nyneth3 #38811. Keep their names and destination stages explicit. |

## Scope and validation

Checkpoint 315 adds documentation, updates the Nyneth coverage row, and records the source-only Surf approach on its existing candidate row. The source scans are read-only. No WLD, MOB, OBJ, ZON, QST, story sidecar, C/C++, database, migration, server, or gameplay state is changed. Static documentation links, CSV shape and row, twenty-question count, whitespace, and restricted-directory status are checked after the register edits; no build or source test is run.

# `dranquest` quest-source audit

Canonical key: `component:dranquest`. Physical coverage row 385; candidate dependency row 35. Reviewed October 11, 2026, Checkpoint 472. This is a static source, graph, history, and typed-reference review; it does not establish live quest availability or authorize activation, migration, or content changes.

## Source scope and map

The tracked files are [WLD](../../../areas/wld/dranquest.wld), [MOB](../../../areas/mob/dranquest.mob), [OBJ](../../../areas/obj/dranquest.obj), [ZON](../../../areas/zon/dranquest.zon), and [QST](../../../areas/qst/dranquest.qst). There is no `dranquest` entry in the full [areas/AREA](../../../areas/AREA) or [areas/qst/AREA](../../../areas/qst/AREA), and no story sidecar. The raw ZON title is `Quest`, zone header #1, last VNUM #12625; the short generic title does not identify a map owner or lineage.

The WLD contains only two rooms: #12601, “Room 1 of Dranquest Zone,” has D5 to #12625; #12625, “Bookend room for Dranquest Zone,” has D4 back to #12601 ([room #12601](../../../areas/wld/dranquest.wld#L1), [room #12625](../../../areas/wld/dranquest.wld#L10)). This is one reciprocal, source-local component of two rooms and two directions; both rooms are reachable from #12601 in the candidate file. There are no external directions, negative targets, fall chances, currents, or range-mismatch rooms/exits. Neither room has an active full-area WLD definition, and the active inbound exit count is zero. This source graph does not establish player access.

The ZON has the header and comments but no reset commands ([ZON](../../../areas/zon/dranquest.zon#L1)). Its five mobile prototypes (#12601–#12605) and seventeen object prototypes (#12601–#12617) therefore have no placement reset in this candidate ZON. The QST has one key #12601 and conversation text; #12601 is also the candidate Balor mobile ([QST](../../../areas/qst/dranquest.qst#L1), [mobile](../../../areas/mob/dranquest.mob#L1)). The request to bring Joukarth Forgath’s head contains no numeric object VNUM, and this source set does not show how that request is placed or fulfilled.

## Story lineage with active Heavens QST

The candidate MOBs name a Balor (#12601), Joukarth Forgath (#12602), a tarrasque (#12603), a glabrezu (#12604), and Rozkadrath (#12605). The objects include seven numbered seals (#12601–#12607), the glorious bag of hoarding, Joukarth’s eyelid and crown, the boots of Netheril mages, a Balor whip, Tennalani, Drogtha’s handbook, a demon’s tear, a glabrezu face, and a live goblin on a chain. The candidate QST text gives the Balor a mission to bring Joukarth’s head and end the Blood War.

The full-manifest [Heavens QST](../../../areas/qst/heavens.qst#L1) also names Rozkadrath and Joukarth, but presents Rozkadrath assisting Joukarth in a Blood War plan to collect elemental components. This creates an obvious chronology/canon question with the candidate text that asks the Balor to kill Joukarth. The text overlap is not a VNUM reference and does not prove `dranquest` is a prior, alternate, or replacement version of Heavens. Keep both source identities separate until the owners reconcile the story and any intended order.

## Typed references and numeric-only code matches

Coverage finds no same-type active room, mobile, object, or QST-giver definition for the candidate VNUMs. A scoped scan of tracked ZON, QST, SHP, story, and code sources found no external active ZON reset, shop product, or numeric QST/story reference to #12601–#12617 or the candidate rooms. Candidate inbound active WLD exits are also zero. The Heavens relationship above is name/text only.

One code-only number match needs type qualification: [`albert()`](../../../src/specs/specs.mobile.c#L1306) checks `world[ch->in_room].number` for room #12613 or #12614 ([checks](../../../src/specs/specs.mobile.c#L1326)). `dranquest.wld` contains rooms #12601 and #12625 only, and no current WLD record defines room #12613 or #12614. Those two numbers are candidate OBJ prototypes, not candidate rooms; the function is not a typed consumer of those objects. A repository search found only the `albert` declaration and definition, with no current special assignment. Treat these literals as an unresolved legacy-code clue, not proof of ownership or an active dependency.

## Title, history, and disposition

The exact raw ZON title `Quest` appears once among the 442 tracked titles, but is too generic to resolve identity. Followed histories of the five same-stem files reach the 2009 area-tree move and later same-stem updates through 2015; no alternate-stem predecessor or explicit whole-area replacement was found. The history does not resolve the relationship with the active Heavens story or the two-room placeholder map.

Keep candidate row 35 RETAIN IN PLACE. Preserve the source map, all five MOBs, seventeen OBJs, QST key/text, empty ZON reset section, and Heavens story comparison until owners resolve canon, placement, quest completion, source/tooling, and persisted identity. See the [18 builder questions](DRANQUEST_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions), [candidate row 35](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L35), and [coverage row 386](ZONE_COVERAGE_REGISTER.csv#L386). No area/source files were changed, and no test, build, runtime, server, database, or container work was performed.

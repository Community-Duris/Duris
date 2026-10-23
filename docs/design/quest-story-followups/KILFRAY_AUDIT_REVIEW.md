# Kilfray source-only QST audit

Canonical key: component:kilfray. Physical coverage row 406 / CSV line 407; candidate dependency row 56 / CSV line 56. Reviewed October 13, 2026, Checkpoint 905, against source snapshot 72ba3deff3bb30bde880018cdebe7c6db675b697; CP493 baseline retained.

This is a static QST source, manifest, contract-lineage, and typed-reference review. It does not establish live quest loading, gameplay, reward delivery, database progress ownership, editor copies, or permission to add, move, or delete a quest file.

## Source inventory and production-list status

The current Kilfray component contains only [areas/qst/kilfray.qst](../../../areas/qst/kilfray.qst#L1). Its five quest-giver headers are #95019–#95023. It has three Q contract blocks and no WLD, MOB, OBJ, ZON, SHP, or story sidecar file. Neither [areas/AREA](../../../areas/AREA) nor [areas/qst/AREA](../../../areas/qst/AREA) lists Kilfray.

The production [make_qst.c](../../../areas/src/qst/make_qst.c#L6) reads the top-level AREA file and only opens qst/<area>.qst for its un-commented entries. The runtime [quest.c](../../../src/world/quest.c#L79) reads the generated areas/world.qst aggregate and does not glob the QST directory. The generated aggregate is absent in this checkout. Since Kilfray is in neither input list, this source file is not included by the tracked standard quest-generation path. Mistywood is listed in both area lists, so its separate [active QST source](../../../areas/qst/mistywood.qst#L1) is the production-list counterpart.

The QST prose describes a hunter pursuing a claw-wearing bear, a druid opposing a destructive giant, Daumis’s request to rescue Jarnes with a mushroom, Jarnes’s rescue and scimitar reward, and a Korred’s tale about an immortal person in a tower. There is no map or separate area definition under the Kilfray stem. The relationship of the filename and these scenes to active Mistywood remains an owner question.

## Giver and contract crosswalk

All five Kilfray giver VNUMs match active Mistywood mobile prototypes. The three Kilfray contracts use five exact item references, all of which match active Mistywood object prototypes #95000–#95004. Daumis #95021 and Korred #95023 have dialogue but no Q contract.

| Giver | Kilfray offer and reward | Active Mistywood QST comparison |
| --- | --- | --- |
| Riliatar #95019 | Give claw #95000; receive 15,000 coins | Same offered item and coins, plus 25,000 experience in Mistywood |
| Ascuren #95020 | Give club #95001; receive potion #95004 | Same offer and item reward; dialogue differs |
| Jarnes #95022 | Give mushroom #95003; receive scimitar #95002; authored D continuation | Same offer, item, and D continuation, plus 10,000 experience in Mistywood |

The two reward differences are source facts, not proof that Kilfray is an obsolete version or that its rewards should be copied into Mistywood. The prose, color formatting, and record order also differ. The full source-backed comparison is already recorded in the [Mistywood audit](MISTYWOOD_AUDIT_REVIEW.md#typed-dependency-and-qst-lineage) and [Mistywood story dossier](../zone-stories/MISTYWOOD.md#three-independent-native-contracts).

## History, identity, and disposition

Tracked QST history shows the 2009 area-tree move and a 2013 generic “stuff” edit. The followed Mistywood QST history has the same path-level history, but no tracked rename, direct source move, or explicit Kilfray-to-Mistywood replacement decision was found. The lack of Kilfray from both current area lists does not establish why it was retained or whether it is a draft, a sibling, or an earlier quest text.

The active Mistywood QST already defines all five givers and three parallel contracts. The Kilfray QST does not supply a separate map or prototype set; its giver and item references resolve numerically to the active Mistywood prototypes. Keep areas/qst/kilfray.qst **RETAIN IN PLACE** outside both production lists while the source author, quest owner, editor/export owner, and persisted progress owner resolve its lineage and the two experience-reward differences. Do not combine or discard the parallel file based on similarity alone. See [identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-493-kilfray-qst-lineage-and-giver-ownership), [retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-493-kilfray-retained-pending-qst-lineage-and-contract-ownership), [candidate row 56](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L56), and [coverage row 406](ZONE_COVERAGE_REGISTER.csv#L407).

## Checkpoint 905: cross-source identity, environment, and typed-role refresh

The tracked-source scan rechecked exact occurrences of Kilfray's five giver
VNUMs (#95019–#95023) and five item VNUMs (#95000–#95004). Mistywood owns
these numbers in separate namespaces: its WLD defines rooms #95000–#95202;
its MOB source defines the five giver prototypes #95019–#95023 and also
mobiles #95000–#95004; its OBJ source defines the five referenced items
#95000–#95004. Typed QST item commands and sidecar item fields use the objects,
while WLD destinations use the rooms. Mistywood's ZON and story sources also
retain their active typed reset and giver roles. The source-only Kilfray QST
remains a parallel text and contract file; no third current QST source
consumes those values as the same giver/item roles.

The scan found three `surf.wld` room targets already counted in Mistywood's
Checkpoint 284 source-only approach set: [#229842 D2](../../../areas/wld/surf.wld#L626018)
and [#229843 D2](../../../areas/wld/surf.wld#L626039) enter active Mistywood
room [#95000](../../../areas/wld/mistywood.wld#L1), and [#231936 D0](../../../areas/wld/surf.wld#L669948)
enters active room [#95001](../../../areas/wld/mistywood.wld#L36). The Surf
rooms are titled “Lake Kilfray” and “The Kilfray River”; active Mistywood rooms
#95000–#95001 are titled “On The Kilfray River.” These records support a
namesake landscape relationship to the active Mistywood map, but the Surf file
is unlisted and its exits are not active ingress. They do not create a WLD map
or route for the QST-only Kilfray component.

Other exact-number hits were role-checked rather than counted as item
consumers. `surface.qst` and source-only `surface2011.qst` use #95000 in `R C`
coin and `R E` experience reward fields ([Surface](../../../areas/qst/surface.qst#L725),
[Surface2011](../../../areas/qst/surface2011.qst#L725)); the matching values in
[`surface.story.json`](../../../areas/story/surface.story.json#L953) are typed
currency/experience amounts. Two unrelated type-9 object records contain
#95000 inside trailing numeric payload tuples ([Krethik #20026](../../../areas/obj/krethik.obj#L416),
[Newhope #89145](../../../areas/obj/newhope.obj#L2627)), not as prototype
headers or established typed VNUM fields. No non-Mistywood ZON reset role or
other same-type WLD/MOB/OBJ definition, external SHP keeper/room/product,
type-25 destination, type-29 room target, or direct C/C++ lookup was found.
The exact-number occurrences add no Kilfray dependency or route count.

The active Mistywood map strengthens the environmental/name crosswalk, while
the Kilfray quest text still gives no room VNUM or named destination for
Jarnes's swamp rescue or the Korred's tower. K-04 and K-15 now ask the story
owner to reconcile these map names with the QST setting. Keep the file outside
both production lists and **RETAIN IN PLACE** until its author, quest owner,
editor/export path, reward intent, and persisted completion identity are
resolved. This is additional map-name evidence, not a tracked rename or
whole-zone replacement decision. See [20 builder questions](KILFRAY_BUILDER_QUESTIONS.md),
[Mistywood's source-only approaches](MISTYWOOD_AUDIT_REVIEW.md#connectivity-reconciliation),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-905-kilfrays-namesake-routes-remain-mistywood-owned),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-905-kilfray-map-names-do-not-establish-qst-replacement),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-905-kilfray-retains-parallel-qst-and-mistywood-map-identities),
[candidate dependency row 56](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L56), and
[coverage CSV line 407](ZONE_COVERAGE_REGISTER.csv#L407).

## Review limits

No tests, builds, server, database, migration, or container work was run. Source-file comparison does not establish quest loading, reward delivery, item custody, completion persistence, or intended authorial precedence.

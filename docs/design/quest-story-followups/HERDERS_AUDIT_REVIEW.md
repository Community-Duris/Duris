# The Behemoth Herders: cross-area audit

Checkpoint 55 supplements the comprehensive [Herders source dossier](../zone-stories/THE_BEHEMOTH_HERDERS.md). This review records the current cross-area boundary, source-only WLD references, exact foreign prototype dependencies, normalized title result, and tracked WLD/ZON history. It is a static source audit; it does not establish installed reachability, gameplay behavior, or player history.

## Active and source-only connectivity

The active area has 842 of 842 resolving exits in one 347-room local weak component. Its three registered cross-zone exits each have a reciprocal record: Herders #94300 south ↔ Surface #594610 north ([outbound row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1132), [return row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1563)); Herders #94300 west ↔ Surface #594209 east ([outbound row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1133), [return row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1562)); and Herders #94646 west ↔ Underdark #746503 east ([outbound row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1134), [return row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1704)). The six rooms with fall fields and 284 rooms with currents remain native hazard data, not proof of safe travel.

All 95 unregistered WLD files parsed without errors: 527,791 rooms and 2,080,773 exits, with no source-room VNUM overlap against Herders. Four source-only exits target active room #94300:

| Source record | Direction and target | Source |
| --- | --- | --- |
| Surf #285152, “Piles of Rubble and Dismembered Bodies” | east → #94300 | [room](../../../areas/wld/surf.wld#L1786832), [exit](../../../areas/wld/surf.wld#L1786840) |
| Surf #285453, same title | north → #94300 | [room](../../../areas/wld/surf.wld#L1793149), [exit](../../../areas/wld/surf.wld#L1793153) |
| Underdark 2011 #249233, “Grassy Plains” | east → #94300 | [room](../../../areas/wld/underdark2011.wld#L182890), [exit](../../../areas/wld/underdark2011.wld#L182898) |
| Underdark 2011 #249534, “Thick Forest” | north → #94300 | [room](../../../areas/wld/underdark2011.wld#L184045), [exit](../../../areas/wld/underdark2011.wld#L184049) |

These records are excluded with their source files from areas/AREA. They are source-only references, not active ingress or additional registered boundary pairs. The four records increase the curated source-only WLD total from 588 to 592 and the reviewed surf.wld subset from 28 to 30.

## Bounded typed references

The only external active QST exact match is the Shipyards ring exchange: Shaenae’s QST accepts object #94307 ([contract](../../../areas/qst/shipy.qst#L559)). The 231-file story-sidecar scan parsed without errors and found one exact numeric match: the Shipyards story sidecar's structured #94307 fields ([story records](../../../areas/story/shipy.story.json#L773)). No external SHP reference matched a local Herders prototype.

The complete review of 32 foreign reset groups across Verspin, Ravenloft, Barovia, Caer Tannad, Alatorin, and Surface Keeps found references to two local object prototypes. The reset roles include P/G placements of #94336 and O placements of #94381 ([Alatorin](../../../areas/zon/alatorin.zon#L1736), [Barovia](../../../areas/zon/barovia2.zon#L99), [Caer Tannad](../../../areas/zon/caertannad.zon#L385), [Ravenloft](../../../areas/zon/ravenloft.zon#L1075), [Surface Keeps](../../../areas/zon/surfacekeeps.zon#L169), [Verspin](../../../areas/zon/verspin.zon#L262)). Their local prototypes are a small concealed money pouch [#94336](../../../areas/obj/herders.obj#L539) and blood chunks [#94381](../../../areas/obj/herders.obj#L1276). These foreign reset references do not supply alternate Herders quest proofs. The existing dossier remains the detailed account of each group and its parent/source ownership.

The exact C/C++ numeric scan also found direct CHAOS equipment-profile references. The code can select #94396, cracked bone arm bracers, in both standard and enhanceable Blighter profiles ([standard](../../../src/account/chaos_eq_data.h#L266), [enhanceable](../../../src/account/chaos_eq_data.h#L510)); the enhanceable Mercenary profile includes #94324, the blood-smeared quiver ([profile](../../../src/account/chaos_eq_data.h#L437)). The account-entry path selects a profile and appends its listed items to the CHAOS kit ([profile selection and kit loop](../../../src/account/nanny.c#L706)). This is an equipment grant path, not a local area procedure. The remaining direct local code reference is #94364, Archazel’s table-driven epic teacher ([mobile](../../../areas/mob/herders.mob#L922), [teacher table](../../../src/classes/epic_skills.c#L190)), already mapped in the dossier. No additional foreign C/C++ literal matched a local Herders room, mobile, or object.

A separate name-only narrative reference appears in the Herders QST response to `flaming horde`: it says the horde lives deep within Mount Olympus ([QST text](../../../areas/qst/herders.qst#L34)). It supplies no exact prototype VNUM or route and does not add a typed dependency. See the [Olympus audit review](OLYMPUS_AUDIT_REVIEW.md#name-based-story-reference) for the local source and route findings.

## Title and tracked history

Color-stripping, whitespace normalization, and case-folding across all 442 current `.zon` titles returns one exact “The Behemoth Herders” title, owned by active contributor `herders` ([title](../../../areas/zon/herders.zon#L2)). The tracked Herders WLD history reaches the 2013 and 2011 edits and the 2009 area-tree relocation; the ZON history reaches the 2016 procedure edit, 2015 quest-item flag update, and the same relocation. This review found no distinct predecessor or replacement record. The title scan does not resolve every semantic alias or command-target ownership question.

## Scope and disposition

Herders remains an active areas/AREA contributor. Its three reciprocal active boundary pairs remain the registered WLD connections. The four inbound records above belong to excluded sources and do not change the active exit census. Shipyards retains ownership of its ring contract; foreign resets retain their actual object and reset roles; the CHAOS references describe conditional account-entry kit profiles. No migration or gameplay decision follows from these findings.

This documentation-only review used source files and static registers. It did not run tests, a build, server/runtime/gameplay, database work, a migration, or an operational script. The source-comprehensive audit queue is now **95/220 complete, 125 pending**; priority 96, Jotunheim (jotun), is next.

## Checkpoint 779: Caertannad typed dependency

Active Caer Tannad has four object reset rows for Herders object #94381. See [Caertannad typed audit](CAERTANNAD_AUDIT_REVIEW.md#outbound-active-reset-references).

# The Great Realm of Duris: repository-wide connectivity and dependency audit

**Checkpoint 73; priority 113 of the original 220-zone queue.** This supplement
revalidates the comprehensive [Great Realm dossier](../zone-stories/THE_GREAT_REALM_OF_DURIS.md)
against the active world manifest, inactive WLD sources, typed references,
compiled numeric bindings, title inventory, and bounded tracked history. It
records source declarations and code references; it does not certify live
availability, travel safety, or gameplay outcomes.

## Active graph and inactive WLD approaches

All 351 active WLD inputs parsed without issues. `connectorzones` contains 646
rooms and 1,406 authored exits: 1,301 are internal, while 105 leave its area
boundary. The loader resolves 1,372 targets; 71 resolved exits lead to other
active areas and 34 positive targets remain unresolved. Those 34 target VNUMs
occur in inactive `surf.wld`, `surface2011.wld`, or `underdark2011.wld` and stay
in the [unresolved-exit register](WORLD_UNRESOLVED_EXIT_REGISTER.csv). The
active cross-area ledger separately records 66 inbound records, 71 resolved
outbound records, and 64 outbound edges with a reverse record. The local weak
exit graph has 44 components; the largest contains 77 rooms.

The scan of all 95 inactive WLD files parsed 527,791 rooms and 2,080,773 exits
without issues. No inactive source room reuses a Great Realm room VNUM. It
found 70 source-only `surf.wld` exits from 70 distinct source rooms into 67
Great Realm target rooms. These are map-source declarations, not active-world
entrances. The source line for each exit is listed below; D0 through D9 preserve
the authored direction code. The [coverage row](ZONE_COVERAGE_REGISTER.csv#L63) records these 70 inbounds separately from the 34 unresolved active outbounds.

| Inactive source room | Direction | Active Great Realm target | Exit declaration |
| --- | --- | --- | --- |
| #218196 | D1 | #53661 | [`surf.wld`#L381760](../../../areas/wld/surf.wld#L381760) |
| #218200 | D3 | #53672 | [`surf.wld`#L381852](../../../areas/wld/surf.wld#L381852) |
| #222684 | D3 | #53818 | [`surf.wld`#L475892](../../../areas/wld/surf.wld#L475892) |
| #223348 | D2 | #53805 | [`surf.wld`#L489828](../../../areas/wld/surf.wld#L489828) |
| #224246 | D1 | #53806 | [`surf.wld`#L508658](../../../areas/wld/surf.wld#L508658) |
| #224649 | D2 | #53634 | [`surf.wld`#L517093](../../../areas/wld/surf.wld#L517093) |
| #225381 | D0 | #53807 | [`surf.wld`#L532413](../../../areas/wld/surf.wld#L532413) |
| #226152 | D3 | #53639 | [`surf.wld`#L548604](../../../areas/wld/surf.wld#L548604) |
| #232765 | D2 | #53719 | [`surf.wld`#L687361](../../../areas/wld/surf.wld#L687361) |
| #233182 | D1 | #53601 | [`surf.wld`#L696110](../../../areas/wld/surf.wld#L696110) |
| #233188 | D3 | #53608 | [`surf.wld`#L696240](../../../areas/wld/surf.wld#L696240) |
| #242046 | D1 | #53933 | [`surf.wld`#L882226](../../../areas/wld/surf.wld#L882226) |
| #242950 | D3 | #53918 | [`surf.wld`#L901206](../../../areas/wld/surf.wld#L901206) |
| #243613 | D1 | #54111 | [`surf.wld`#L915117](../../../areas/wld/surf.wld#L915117) |
| #244754 | D2 | #53712 | [`surf.wld`#L939054](../../../areas/wld/surf.wld#L939054) |
| #246331 | D5 | #53891 | [`surf.wld`#L972155](../../../areas/wld/surf.wld#L972155) |
| #246549 | D1 | #53707 | [`surf.wld`#L976717](../../../areas/wld/surf.wld#L976717) |
| #248130 | D5 | #54008 | [`surf.wld`#L1009902](../../../areas/wld/surf.wld#L1009902) |
| #248138 | D5 | #53917 | [`surf.wld`#L1010074](../../../areas/wld/surf.wld#L1010074) |
| #248423 | D5 | #53909 | [`surf.wld`#L1016059](../../../areas/wld/surf.wld#L1016059) |
| #248426 | D5 | #53893 | [`surf.wld`#L1016126](../../../areas/wld/surf.wld#L1016126) |
| #249571 | D1 | #54014 | [`surf.wld`#L1040147](../../../areas/wld/surf.wld#L1040147) |
| #249577 | D3 | #53791 | [`surf.wld`#L1040281](../../../areas/wld/surf.wld#L1040281) |
| #249630 | D5 | #53892 | [`surf.wld`#L1041398](../../../areas/wld/surf.wld#L1041398) |
| #250530 | D5 | #53901 | [`surf.wld`#L1060302](../../../areas/wld/surf.wld#L1060302) |
| #253034 | D3 | #53830 | [`surf.wld`#L1112830](../../../areas/wld/surf.wld#L1112830) |
| #253631 | D1 | #53827 | [`surf.wld`#L1125351](../../../areas/wld/surf.wld#L1125351) |
| #261258 | D1 | #53781 | [`surf.wld`#L1285426](../../../areas/wld/surf.wld#L1285426) |
| #261263 | D3 | #53782 | [`surf.wld`#L1285539](../../../areas/wld/surf.wld#L1285539) |
| #265729 | D2 | #54007 | [`surf.wld`#L1379253](../../../areas/wld/surf.wld#L1379253) |
| #265752 | D2 | #53832 | [`surf.wld`#L1379736](../../../areas/wld/surf.wld#L1379736) |
| #265827 | D3 | #53780 | [`surf.wld`#L1381315](../../../areas/wld/surf.wld#L1381315) |
| #266074 | D2 | #53865 | [`surf.wld`#L1386490](../../../areas/wld/surf.wld#L1386490) |
| #266329 | D0 | #54002 | [`surf.wld`#L1391829](../../../areas/wld/surf.wld#L1391829) |
| #266352 | D0 | #53831 | [`surf.wld`#L1392312](../../../areas/wld/surf.wld#L1392312) |
| #266383 | D2 | #53868 | [`surf.wld`#L1392971](../../../areas/wld/surf.wld#L1392971) |
| #266416 | D2 | #53777 | [`surf.wld`#L1393664](../../../areas/wld/surf.wld#L1393664) |
| #266684 | D3 | #53868 | [`surf.wld`#L1399296](../../../areas/wld/surf.wld#L1399296) |
| #266715 | D1 | #53777 | [`surf.wld`#L1399939](../../../areas/wld/surf.wld#L1399939) |
| #267021 | D1 | #53765 | [`surf.wld`#L1406365](../../../areas/wld/surf.wld#L1406365) |
| #268172 | D1 | #53833 | [`surf.wld`#L1430516](../../../areas/wld/surf.wld#L1430516) |
| #269329 | D2 | #53679 | [`surf.wld`#L1454813](../../../areas/wld/surf.wld#L1454813) |
| #269983 | D0 | #53852 | [`surf.wld`#L1468539](../../../areas/wld/surf.wld#L1468539) |
| #270003 | D2 | #53753 | [`surf.wld`#L1468967](../../../areas/wld/surf.wld#L1468967) |
| #270228 | D1 | #53683 | [`surf.wld`#L1473684](../../../areas/wld/surf.wld#L1473684) |
| #270532 | D3 | #53674 | [`surf.wld`#L1480076](../../../areas/wld/surf.wld#L1480076) |
| #270730 | D2 | #53826 | [`surf.wld`#L1484230](../../../areas/wld/surf.wld#L1484230) |
| #270831 | D0 | #53674 | [`surf.wld`#L1486339](../../../areas/wld/surf.wld#L1486339) |
| #271156 | D2 | #53757 | [`surf.wld`#L1493168](../../../areas/wld/surf.wld#L1493168) |
| #271204 | D3 | #53748 | [`surf.wld`#L1494180](../../../areas/wld/surf.wld#L1494180) |
| #272064 | D3 | #53616 | [`surf.wld`#L1512228](../../../areas/wld/surf.wld#L1512228) |
| #272228 | D1 | #53819 | [`surf.wld`#L1515664](../../../areas/wld/surf.wld#L1515664) |
| #272707 | D2 | #53660 | [`surf.wld`#L1525727](../../../areas/wld/surf.wld#L1525727) |
| #273000 | D1 | #53641 | [`surf.wld`#L1531868](../../../areas/wld/surf.wld#L1531868) |
| #273259 | D0 | #53609 | [`surf.wld`#L1537291](../../../areas/wld/surf.wld#L1537291) |
| #274204 | D0 | #53659 | [`surf.wld`#L1557120](../../../areas/wld/surf.wld#L1557120) |
| #274301 | D3 | #53631 | [`surf.wld`#L1559169](../../../areas/wld/surf.wld#L1559169) |
| #274894 | D1 | #53625 | [`surf.wld`#L1571610](../../../areas/wld/surf.wld#L1571610) |
| #275636 | D1 | #53754 | [`surf.wld`#L1587164](../../../areas/wld/surf.wld#L1587164) |
| #275638 | D3 | #53755 | [`surf.wld`#L1587206](../../../areas/wld/surf.wld#L1587206) |
| #277827 | D2 | #53695 | [`surf.wld`#L1633139](../../../areas/wld/surf.wld#L1633139) |
| #278730 | D3 | #53706 | [`surf.wld`#L1652090](../../../areas/wld/surf.wld#L1652090) |
| #278783 | D2 | #53742 | [`surf.wld`#L1653199](../../../areas/wld/surf.wld#L1653199) |
| #278994 | D3 | #53764 | [`surf.wld`#L1657630](../../../areas/wld/surf.wld#L1657630) |
| #280190 | D1 | #53758 | [`surf.wld`#L1682702](../../../areas/wld/surf.wld#L1682702) |
| #280282 | D0 | #53746 | [`surf.wld`#L1684630](../../../areas/wld/surf.wld#L1684630) |
| #280765 | D2 | #53799 | [`surf.wld`#L1694769](../../../areas/wld/surf.wld#L1694769) |
| #282265 | D0 | #53792 | [`surf.wld`#L1726237](../../../areas/wld/surf.wld#L1726237) |
| #282408 | D1 | #53992 | [`surf.wld`#L1729244](../../../areas/wld/surf.wld#L1729244) |
| #282412 | D3 | #53993 | [`surf.wld`#L1729336](../../../areas/wld/surf.wld#L1729336) |

The active-room duplicate scan still finds the repository's separate #51800
collision in Hell Two and Hell Three; Great Realm has no room-VNUM collision.

## External typed references and compiled bindings

Eight external QST item commands reference Great Realm objects. All are in
`surfacemini.qst` and support its foreign artifact-crafting closures already
mapped in the dossier.

| Source line | External item command |
| --- | --- |
| [#L183](../../../areas/qst/surfacemini.qst#L183) | `R I #53661` |
| [#L184](../../../areas/qst/surfacemini.qst#L184) | `G I #53650` |
| [#L191](../../../areas/qst/surfacemini.qst#L191) | `R I #53662` |
| [#L192](../../../areas/qst/surfacemini.qst#L192) | `G I #53651` |
| [#L199](../../../areas/qst/surfacemini.qst#L199) | `R I #53663` |
| [#L200](../../../areas/qst/surfacemini.qst#L200) | `G I #53652` |
| [#L214](../../../areas/qst/surfacemini.qst#L214) | `R I #53664` |
| [#L215](../../../areas/qst/surfacemini.qst#L215) | `G I #53646` |

The 231 other story sidecars parsed without issues. Ten exact structured VNUM
matches all occur in [`surfacemini.story.json`](../../../areas/story/surfacemini.story.json): two mobile contacts (#53700,
#53704), four story-giver links to those mobiles, and four recipe item references
(#53650, #53651, #53652, #53646). No external SHP product or mobile/object
prototype definition targets this area.

The unregistered `surf.zon` also contains 209 `M` reset records referencing
16 local Great Realm mobile prototypes. These declarations are source-only
because `surf` is outside `areas/AREA`; they do not establish active resets.
The counts below are reset command records, grouped by referenced mobile.

| Local mobile prototype | `surf.zon` reset records | First matching source line |
| --- | ---: | --- |
| [#53639](../../../areas/mob/connectorzones.mob#L533) | 4 | [`surf.zon`#L483](../../../areas/zon/surf.zon#L483) |
| [#53644](../../../areas/mob/connectorzones.mob#L603) | 10 | [`surf.zon`#L487](../../../areas/zon/surf.zon#L487) |
| [#53675](../../../areas/mob/connectorzones.mob#L1085) | 12 | [`surf.zon`#L408](../../../areas/zon/surf.zon#L408) |
| [#53676](../../../areas/mob/connectorzones.mob#L1102) | 12 | [`surf.zon`#L420](../../../areas/zon/surf.zon#L420) |
| [#53677](../../../areas/mob/connectorzones.mob#L1119) | 29 | [`surf.zon`#L432](../../../areas/zon/surf.zon#L432) |
| [#53678](../../../areas/mob/connectorzones.mob#L1136) | 2 | [`surf.zon`#L467](../../../areas/zon/surf.zon#L467) |
| [#53679](../../../areas/mob/connectorzones.mob#L1152) | 38 | [`surf.zon`#L274](../../../areas/zon/surf.zon#L274) |
| [#53680](../../../areas/mob/connectorzones.mob#L1167) | 20 | [`surf.zon`#L327](../../../areas/zon/surf.zon#L327) |
| [#53681](../../../areas/mob/connectorzones.mob#L1182) | 15 | [`surf.zon`#L364](../../../areas/zon/surf.zon#L364) |
| [#53682](../../../areas/mob/connectorzones.mob#L1196) | 6 | [`surf.zon`#L347](../../../areas/zon/surf.zon#L347) |
| [#53683](../../../areas/mob/connectorzones.mob#L1211) | 6 | [`surf.zon`#L321](../../../areas/zon/surf.zon#L321) |
| [#53684](../../../areas/mob/connectorzones.mob#L1227) | 11 | [`surf.zon`#L353](../../../areas/zon/surf.zon#L353) |
| [#53685](../../../areas/mob/connectorzones.mob#L1243) | 14 | [`surf.zon`#L312](../../../areas/zon/surf.zon#L312) |
| [#53686](../../../areas/mob/connectorzones.mob#L1258) | 13 | [`surf.zon`#L379](../../../areas/zon/surf.zon#L379) |
| [#53687](../../../areas/mob/connectorzones.mob#L1272) | 13 | [`surf.zon`#L392](../../../areas/zon/surf.zon#L392) |
| [#53688](../../../areas/mob/connectorzones.mob#L1286) | 4 | [`surf.zon`#L405](../../../areas/zon/surf.zon#L405) |

The existing dossier maps 139 foreign reset groups from 15 active sources and
the direct C/C++ bindings. Those code matches include local object references
in [`chaos_eq_data.h#L221`](../../../src/account/chaos_eq_data.h#L221), Chauseis's
compiled Wisdom entry in [`epic_skills.c#L214`](../../../src/classes/epic_skills.c#L214),
ship rows using #54240 in [`ship_variables.c#L302`](../../../src/ships/ship_variables.c#L302),
and Great Realm dispatches in [`specs.assign.c#L734`](../../../src/specs/specs.assign.c#L734).
No external SHP product was found. Of 865 external type-25 declarations
scanned, [`shipy.obj`#L636](../../../areas/obj/shipy.obj#L636) has the one
external target into this area: object #43145 points to room #54245. The shipyard return and local portal
network are already mapped in the dossier.

## Title, tracked history, and disposition

The normalized active-zone-title scan found one exact title `The Great Realm of Duris` among 351 contributors. Its color-coded title is in
[`connectorzones.zon`](../../../areas/zon/connectorzones.zon#L2), and the
registered source key appears in [`areas/AREA`](../../../areas/AREA#L230).
Bounded WLD/ZON history reaches the 2009 area-tree move and shows later source
edits through 2016. The [duplicate/replacement register](DUPLICATE_AND_REPLACED_ZONES.md)
identifies no distinct predecessor or replacement. Great Realm remains **DO NOT MOVE** as a current
full-world contributor.

This checkpoint adds no zone, quest, or runtime change. It used repository
sources and static registers only; no tests, build, server/runtime, gameplay,
database, migration, or operational script was run. Gameplay, editor/tooling,
persisted-reference, and dynamic-route qualification remain separate.

## Checkpoint 779: Caertannad typed dependency

Active Caer Tannad resets 12 mobiles (#53610, #53639) and one object (#53607) from Connectorzones. See [Caertannad typed audit](CAERTANNAD_AUDIT_REVIEW.md#outbound-active-reset-references).

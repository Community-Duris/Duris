# `torr` legacy-source audit

Canonical key: `component:torr`. Coverage data row 457 / CSV line 458;
candidate dependency data row 106 / CSV line 107. Reviewed October 12, 2026,
Checkpoint 544, against checkout
`c500ac51b8cd26540943257e7ca4e9807263ef74`. This is a static source,
identity, graph, and typed-dependency review; it does not qualify generated
aggregates, deployed configuration, or gameplay.

## Source selection and the Fehlnan/Torrhan identity question

The current [full world list](../../../areas/AREA#L251) contains `torrhan`
and no `torr`; the current [quest list](../../../areas/qst/AREA#L225) also
contains `torrhan` and no `torr`. The candidate ZON is headed #790,
“City of Fehlnan,” through #79299, reset mode 1
([header](../../../areas/zon/torr.zon#L2)). The separately selected source is
“City of Torrhan” ([active ZON](../../../areas/zon/torrhan.zon#L2)). Each
normalized title is unique in the current ZON title set.

The maps have separate room, mobile, and object VNUM sets. No same-type WLD,
MOB, or OBJ VNUM in `torr` occurs in any other source file. The source texts
still share strong story evidence: old room #79000 and active room #66600 are
both market squares describing the city's decline from triple its former
crowds; the old ZON and QST name King Torrhan, a hypnotized king, a secret
beach entrance, dragon scales, Princess Rhithel, and Zrilxa
([old room](../../../areas/wld/torr.wld#L1),
[old quest](../../../areas/qst/torr.qst#L1),
[active room](../../../areas/wld/torrhan.wld#L1),
[active quest](../../../areas/qst/torrhan.qst#L28)). This suggests a shared
or revised story, but it does not by itself prove that `torr` was replaced by
`torrhan` or that the two maps should be merged.

Followed source history reaches the 2009 directory move. Later commits modify
the old seaman and modernize MOB/OBJ source fields; the 2013 “stuff” commit
does not change the WLD, ZON, QST, or SHP contents. The available history
contains no explicit `torr`→`torrhan` replacement decision. The earlier
[Torrhan dossier question](../zone-stories/CITY_OF_TORRHAN.md#explicit-builder-questions-and-recommended-resolutions)
already recommends keeping their source identities separate until an owner
resolves the relationship.

| Type | `torr` source inventory |
| --- | --- |
| WLD | 300 rooms, #79000–#79299 |
| MOB | 158 records, #79000–#79157 |
| OBJ | 115 records, #79000–#79114 |
| QST | 10 giver headers, #79046–#79108; 22 Q/QA contracts |
| ZON | Header #790, 517 reset commands |
| SHP | Six records: #79000, #79001, #79008, #79123–#79125; 14 product entries |

There is no `areas/map/torr.json` or `areas/story/torr.story.json` in this
checkout.

## Legacy WLD graph

The 300-room source has 634 authored exits: 633 local and one to active Chasm
room #15052 from #79092 south (D2)
([source exit](../../../areas/wld/torr.wld#L1909)). All 634 targets resolve
against the candidate or current full-area WLD sources; there are no negative
or unresolved destinations. The candidate graph has one weak component of
300 rooms. A directed walk from #79000 reaches 293; #79264–#79270 are not in
that directed reach set. Two tunnel links use the loader-supported D8 and D7
directions between #79157 and #79158
([D8](../../../areas/wld/torr.wld#L3225),
[D7](../../../areas/wld/torr.wld#L3244)); the loader defines ten exit
directions ([NUM_EXITS](../../../src/core/defines.h#L440)). Three rooms have
fall chances: #79151 (10), #79152 (5), and #79153 (3)
([directives](../../../areas/wld/torr.wld#L3113),
[directives](../../../areas/wld/torr.wld#L3132),
[directives](../../../areas/wld/torr.wld#L3151)); no current directives were
found.

No selected active WLD exit targets a `torr` room VNUM. Candidate-specific
active ingress is zero, with one candidate-to-active outbound source edge.
The #79092 approach was already recorded in the Mist Chasm source-only
approach review, so this checkpoint does not add it again to the curated
approach total. The source edge is not evidence that this inactive map is
currently reachable.

## Reset, quest, object, and shop references

The ZON contains 271 M, 18 F, 100 D, 13 O, 66 E, 48 G, and one P reset. Every
typed mobile, object, room, door, and object-container reference resolves to a
record in the candidate files. All ten QST givers have local MOB records and
are placed by local M resets. The 22 Q/QA contracts' R I and G I references
name local OBJ records.

The candidate OBJ #79046 is a type-25 portal to local room #79180; its ZON O
reset places it in #79205
([portal placement](../../../areas/zon/torr.zon#L412)). No type-25 object in
another area targets a `torr` room. The selected active WLD scan found no
exit into the candidate; other source ZON, QST, and SHP files have no typed
references into its room/mobile/object sets.

All six SHP records have local keepers and rooms, and each keeper/room pair
matches a local ZON M reset. Their 14 listed products all resolve to local
objects. Source selection has a caveat: the normal SHP builder reads
`areas/AREA` and therefore omits `torr`
([manifest](../../../areas/AREA#L251),
[world build](../../../Makefile#L137),
[builder](../../../areas/src/shp/make_shp.c#L26)); the runtime shop loader
first opens `areas/world.shp` ([path](../../../src/world/db.h#L22)), but if
that aggregate is absent its fallback concatenates every `areas/shp/*.shp`,
without filtering through `areas/AREA`
([loader](../../../src/economy/shop.c#L3278)). The generated aggregate and
deployed boot path were not inspected, so the six unlisted shops cannot be
called either loaded or inactive from this source review alone.

The QST builder selects files from `qst/AREA`
([manifest](../../../areas/qst/AREA#L1),
[world build](../../../Makefile#L137),
[builder](../../../areas/src/qst/make_qst.c#L54)); the server reads the
generated `areas/world.qst` ([runtime input](../../../src/world/quest.c#L79)).
Thus the normal QST build input excludes `torr.qst`, but the generated file
and deployment were not checked. A direct source-code VNUM scan found no
typed implementation reference to the candidate IDs.

## Disposition

Keep all six `torr` source files **RETAIN IN PLACE**. Preserve the one-way
source approach to Chasm, the internal fall/portal/reset data, and the QST and
SHP files until their manifest and loader owners identify the intended source
and deployment paths. Keep `torr` and `torrhan` as separate identities while
the overlapping narrative remains an explicit builder question. Do not
archive, rename, move, or re-add the candidate based only on title similarity
or shared plot material. See the [twenty builder questions](TORR_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-544-torr-is-a-300-room-source-map-with-one-outbound-chasm-edge),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-544-fehlnan-and-torrhan-share-story-evidence-without-a-tracked-replacement),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-544-retain-fehlnan-pending-torrhan-identity-and-loader-review),
[candidate dependency row 106](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L107),
and [coverage row 457](ZONE_COVERAGE_REGISTER.csv#L458).

# Mountain.qst source and quest-story audit

Canonical key: component:Mountain. Coverage data row 424 / CSV line 425;
candidate dependency row 73 / CSV line 74. Initial review October 11, 2026
(Checkpoint 511); typed-role supplement October 13, 2026 (Checkpoint 922),
against source snapshot
c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static QST, typed
reference, source-list, identity, and bounded-history review. It does not
establish runtime loading or successful quest exchanges.

## Inventory and environmental story

The component consists of one unregistered file,
[`areas/qst/Mountain.qst`](../../../areas/qst/Mountain.qst#L1). It defines
five QST giver headers (#21012, #21014, #21025, #21028, #21029) and four Q
completion clauses. It has no same-stem WLD, MOB, OBJ, ZON, SHP, or story
sidecar. The file is absent from both the full
[`areas/AREA`](../../../areas/AREA#L52) manifest and
[`areas/qst/AREA`](../../../areas/qst/AREA#L46), which list lowercase `mount`
instead. The candidate has no ZON title or room text.

The source preserves two connected but distinct quest threads. Header #21012
seeks an otherworldly metal said to be held by a great white dragon; its
response points toward adamantite and mithril chips, Givnab the Conjurer, and
Sorlag the dwarven smith, who is meant to forge a weapon to slay evil
([opening thread](../../../areas/qst/Mountain.qst#L1)). Header #21014 is an
enchantment exchange. Gulik's #21025 text describes a once-peaceful mountain
settlement divided by fighting and an order whose halfling thief stole a
serpentine dagger and black leather armor; the completion prose promises
armoury access and a protected suit
([order quest](../../../areas/qst/Mountain.qst#L30)). Header #21028 gives
Sorlag's completion text about the ethereal demon and the leaders' keys,
followed by the finished weapon, while #21029 is a knight's invitation to
ask Gulik about joining
([smith and invitation](../../../areas/qst/Mountain.qst#L58)).

These authored passages have no current same-stem mobile or item prototypes
for their named givers and most exchanged objects. Treat them as an
incomplete source story, not as an active quest or a verified route through a
mountain map.

## Typed QST references and unresolved item identities

The QST loader interprets header numbers as quester mobile VNUMs; `G I` and
`R I` clauses are item goals. The current source has no mobile prototypes for
quester IDs #21012, #21014, #21025, #21028, or #21029. Its nine item-goal
commands refer to eight unique object VNUMs:

| QST header | Source item commands | Current source resolution |
| --- | --- | --- |
| #21012 | `R E 10000`; `G I 21000` | The 10,000 EXP-typed removal is literal source data; object #21000 has no current definition. |
| #21014 | `R I 21009`; `G I 21002` | Objects #21009 and #21002 have no current definitions. |
| #21025 | `R I 21027`; `G I 93001` | Object #21027 has no current definition. #93001 resolves to active Dungeon's black leather armor. |
| #21028 | `R I 21026`; `G I 21009`; `G I 21008`; `G I 21025` | Objects #21026, #21009, #21008, and #21025 have no current definitions. |

The seven unique #210xx item VNUMs (#21000, #21002, #21008, #21009, #21025,
#21026, and #21027) were not found in current `areas/obj` or the mini object
source. The one existing output reference, object #93001, is defined in
[Dungeon's OBJ](../../../areas/obj/dungeon.obj#L19) as black leather armor.
Gulik's completion text calls its reward a key to the armoury, while the
source clause gives #93001 armor; the candidate's required input #21027 is
unresolved. Preserve both records and ask the story/item owners to identify
the intended key, accepted armor, and reward identity.

The active full-area contributor `mount` has a separate lowercase
[`mount.qst`](../../../areas/qst/mount.qst#L1), with different quester VNUMs
(#9107, #9121, #9123, and #9136) and a distinct Lokpan/prison story. Shared
mountain wording does not establish that uppercase `Mountain.qst` is an old
version or alternate source for active `mount`.

## Load selection and connectivity

This QST-only component has no WLD rooms or exits, so graph and numeric-room
range metrics are not applicable. The candidate dependency register records
zero active WLD inbound edges.

Normal QST aggregation reads the full root `areas/AREA` manifest and opens
`qst/<listed-stem>.qst`; that manifest names `mount`, not `Mountain`
([QST compiler](../../../areas/src/qst/make_qst.c#L16)). Runtime quest boot
opens only `areas/world.qst`; `open_quest_stream` explicitly has no
`areas/qst/*.qst` fallback
([quest loader](../../../src/world/quest.c#L2086)). The current checkout has
no generated `areas/world.qst`. Thus the checked source/build configuration
does not select this candidate. This is not a claim about a separate
deployment's manually supplied aggregate or persisted quest data.

## History, identity, and safe disposition

Followed `Mountain.qst` history shows the 2009 area-tree move and a 2013
same-path update. It establishes no alternate-stem rename, source-file
replacement, or owner relationship to active `mount.qst`. The current
uppercase and lowercase QST files contain different headers and story text;
their coexistence and similar names do not prove that they are one lineage.
A raw #21025 mention in a free-text `mountaintracks.story.json` description
does not establish a typed inbound QST mobile or object dependency.

**Disposition: RETAIN IN PLACE.** Preserve the one source file and its exact
cross-component output reference to active Dungeon object #93001. Resolve the
seven missing object identities, all five absent giver prototypes, the
key-versus-armor wording, any intended relationship to active `mount`, and
editor/deployment/persisted-data ownership before a separate archive or path
decision. No migration clearance or active quest behavior is implied. See the
[Dungeon dependency supplement](TREASURE_CAVES_AUDIT_REVIEW.md#checkpoint-511-supplement-mountain-qst-reward-identity),
[active Mountain review](MOUNTAIN_OF_THE_BANISHED_AUDIT_REVIEW.md#checkpoint-511-separate-unregistered-mountain-qst),
[12 builder questions](MOUNTAIN_QST_BUILDER_QUESTIONS.md),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-511-mountain-qst-has-no-wld-graph),
[duplicate/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-511-mountain-qst-and-active-mount),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-511-mountain-qst-retained-for-reference-closure),
[candidate dependency row 73](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L74), and
[coverage row 424](ZONE_COVERAGE_REGISTER.csv#L425).

## Checkpoint 922: typed collision with active Mountaintracks rooms

A new typed-source pass confirms that the Mountain QST's five headers are
mobile VNUMs and its G I/R I clauses are item VNUMs; R E 10000 is an
experience goal. The quest loader sends header numbers to real_mobile and
marks I goals as items
([header binding](../../../src/world/quest.c#L2137),
[item goals](../../../src/world/quest.c#L2210),
[experience goal](../../../src/world/quest.c#L2261)). Those roles do not
resolve through same-number WLD rooms.

All five candidate giver numbers (#21012, #21014, #21025, #21028, #21029)
and all seven unresolved item numbers (#21000, #21002, #21008, #21009,
#21025, #21026, #21027) also name room records in the active
mountaintracks map, listed in the full [area manifest](../../../areas/AREA#L108).
For example, its WLD defines rooms #21000, #21002, #21008, #21009, #21012,
#21014, and #21025–#21029
([early rooms](../../../areas/wld/mountaintracks.wld#L2349),
[later rooms](../../../areas/wld/mountaintracks.wld#L2937)).
No current MOB definitions exist for the five QST header numbers, and no
current OBJ definitions exist for the seven item numbers. The WLD records
are room identities, not mobile or object prototypes.

Active mountaintracks resets reinforce the role distinction: they place
different mobiles #20942 in room #21012, #20944 in #21014, #20946, #20972,
and #20981 in #21025, and #20948 in #21028
([resets](../../../areas/zon/mountaintracks.zon#L288)). The active shop
record uses keeper #20972 and room #21025
([shop room](../../../areas/shp/mountaintracks.shp#L23)); the shop loader
reads that field as an in-room value
([loader](../../../src/economy/shop.c#L3496)). The story sidecar likewise
identifies the #20972 shopkeeper and #20981 mage at room #21025
([contacts](../../../areas/story/mountaintracks.story.json#L37)).
These records do not supply a #21025 quest mobile or object.

This distinction is especially important for #21025: Mountain.qst uses it
both as a quester header (mobile role) and as an item goal (object role),
while active mountaintracks uses it as a room. The values overlap, but none
of those records substitutes for the other typed identity. No other current
QST header or goal binds the candidate's #210xx values; raw numeric text and
non-header values in unrelated files were not counted as prototypes.

The separate output G I #93001 remains an object reference to Dungeon OBJ
#93001, the black leather armor. The same number also names Dungeon mobile
#93001 and its QST header, WLD room #93001, and a compiled room-array entry;
those are mobile and room roles, not additional item matches
([Dungeon object](../../../areas/obj/dungeon.obj#L19),
[mobile](../../../areas/mob/dungeon.mob#L13),
[QST header](../../../areas/qst/dungeon.qst#L1),
[room](../../../areas/wld/dungeon.wld#L31),
[compiled room array](../../../src/classes/innates.c#L2008)).
The key-versus-armor question therefore remains an item-identity question.
The candidate still has no WLD source or active WLD inbound edge; these
numeric overlaps do not add a route, activate the QST, or establish that it
belongs to mountaintracks or lowercase mount.

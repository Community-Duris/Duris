# The Human Outpost repository-wide source audit

Checkpoint 206 maps active outpost against its authored story, active WLD
boundary, reset-aware graph, inactive-WLD approach, typed dependencies, title,
and followed source history. This is a static source audit. It does not inspect
a live server, player state, database, editor, persisted world state, or actual
gameplay. The [source dossier](../zone-stories/OUTPOST.md) summarizes the
environmental story and builder questions.

## Source identity and authored content

The full area list identifies outpost with range note *821. The .zon header is
#821, reset mode 2, title The Human Outpost. The room range #82100-#82139 has
40 records; the mobile file has 22 prototypes #82100-#82121, the object file
has 21 prototypes #82100-#82120, the local QST is attached to mobile #82120,
and the SHP file has one shop, #82121 in room #82106. The source is listed in
both areas/AREA and areas/qst/AREA. There is no story sidecar and no native
Q/QA completion contract ([area list](../../../areas/AREA#L293), [zone
header](../../../areas/zon/outpost.zon#L1), [QST index](../../../areas/qst/AREA#L261),
[local QST](../../../areas/qst/outpost.qst#L1), [shop](../../../areas/shp/outpost.shp#L1),
[coverage row 227](ZONE_COVERAGE_REGISTER.csv#L227)).

The source portrays an underground human military base in Tharnadia. It has
guard walks, barracks, an armory and shop, planning rooms with maps of Duris
and evil-race cities, a treasury for raid loot, a training area, and a dark
prison. Reset-placed sick guards lie among bloody bandages; one goblin is
described as shackled and tortured. The text leaves the sickness and the
prisoner's capture unexplained. Lord Perrin commands the outpost and discusses
raids with officers. See the [room descriptions](../../../areas/wld/outpost.wld#L1),
[mobile descriptions](../../../areas/mob/outpost.mob#L1), and
[resets](../../../areas/zon/outpost.zon#L120).

Lord Perrin's local QST record asks about “key ethereal cathedral.” Its answer
says he has the key, refuses to give it, and says that a skeleton could
devastate the world if it were not destroyed ([dialogue](../../../areas/qst/outpost.qst#L1)).
The reset file puts ethereal key #82110 on Perrin in room #82123, together
with a black box containing an azure holy symbol and a scroll signed by Lord
Kaynor ([resets](../../../areas/zon/outpost.zon#L184), [key
object](../../../areas/obj/outpost.obj#L150), [box, symbol, and
scroll](../../../areas/obj/outpost.obj#L252)).

These are directly connected to the active Tharnadian Ruin source. Its QST
dialogue says the ethereal key to the sealed Cathedral is held by Lord Perrin
at the Winterhaven Outpost and describes the skeletal knight inside
([Cathedral quest dialogue](../../../areas/qst/tharnadian_ruin.qst#L49)). The
Cathedral's outer WLD door explicitly requires an ethereal key and both door
fields name #82110; its room is titled Before a Gothic Cathedral. The paired
room is titled Inside the Cathedral. Both door sides are reset locked
([entrance key field](../../../areas/wld/tharnadian_ruin.wld#L971),
[inside reverse key field](../../../areas/wld/tharnadian_ruin.wld#L1016),
[room #5540 reset](../../../areas/zon/tharnadian_ruin.zon#L40),
[room #5541 reset](../../../areas/zon/tharnadian_ruin.zon#L44)). The skeletal
knight's mobile text says it is free to ravage the world after the Cathedral's
seal is released ([mobile #5503](../../../areas/mob/tharnadian_ruin.mob#L49)).

The same local key #82110 opens the Outpost treasury pair #82106/#82125,
whose room text describes the treasury as storage for raid loot
([armory-side key field](../../../areas/wld/outpost.wld#L150), [treasury-side
key field](../../../areas/wld/outpost.wld#L554)). The authored WLD therefore names
two door pairs for this key. The quest narrative also introduces a second
possible Cathedral key: the scroll says the nested holy symbol #82118 is the
key to entering the Cathedral, but no scanned WLD door-key field uses #82118.
The active Cathedral door fields use #82110. Whether the key and symbol open
separate seals, or one source is stale, remains unresolved
([scroll](../../../areas/obj/outpost.obj#L276), [Cathedral key
field](../../../areas/wld/tharnadian_ruin.wld#L971)).

The quest sources also differ on Perrin's decision. Outpost dialogue says he
will not give the key under any condition. Tharnadian Ruin's expeditionary
leader says Perrin might grant it if the player proves worthy, while another
answer says he will give access to no one ([Outpost dialogue](../../../areas/qst/outpost.qst#L3),
[Tharnadian Ruin dialogue](../../../areas/qst/tharnadian_ruin.qst#L49),
[later answer](../../../areas/qst/tharnadian_ruin.qst#L147)).

The armorer sells four local armor pieces and imported Heavens supplies #363,
#368, and #369. The imported definitions name them gauze bandage, gauze
tourniquet, and large medicated bandage; the reset file gives them to the
shopkeeper. The shop's refusal line says “I do not sell to your people!” but
does not identify a buyer predicate ([shop](../../../areas/shp/outpost.shp#L1),
[resets](../../../areas/zon/outpost.zon#L141), [gauze
bandage](../../../areas/obj/heavens.obj#L3783), [gauze
tourniquet](../../../areas/obj/heavens.obj#L3836), [medicated
bandage](../../../areas/obj/heavens.obj#L3850)).

## Active WLD graph and reset-state reach

The local WLD has 81/81 resolving exits: 80 local and one outbound. Its local
graph is one weak component, and directed traversal with authored local exits
available reaches all 40 rooms from active arrival #82124. The active Surface
boundary is reciprocal:

| Neighbor | Active inbound edge | Paired outbound edge |
| --- | --- | --- |
| Surface | Surface #527555 east (D1) to #82124 ([Surface edge](../../../areas/wld/surface.wld#L578526), [target](../../../areas/wld/surface.wld#L578529)) | outpost #82124 west (D3) to #527555 ([local edge](../../../areas/wld/outpost.wld#L530), [target](../../../areas/wld/outpost.wld#L533)) |

Room #82124 also leads down to #82100, the path through the outpost. No other
active external WLD inbounds or outbounds target the local room range. The
cross-boundary source and reverse counts are each one; there are no negative,
positive-unresolved, or unregistered WLD target references.

The .zon contains 24 unconditional directional door resets covering 12 local
pairs. One pair is reset closed and locked: #82106 north to #82125 and
#82125 south to #82106, with key #82110. Eleven pairs are reset closed without
locking: ten doors between the barracks, storage, map, and meeting rooms, and
the lower staircase between #82135 and #82136. The resets leave 56/80 local
directed exits open. The strict reset-open graph has weak components sized
25, 4, and eleven single-room components. Directed reach from active arrival
#82124 is 25/40. The four-room component is #82136-#82139; isolated rooms
include the sleeping rooms, storage rooms, map room, Perrin's meeting room,
and treasury. These counts model unconditional reset states only; they do not
qualify door commands, key access, dynamic movement, or safe return. See the
[local door resets](../../../areas/zon/outpost.zon#L8) and
[all room exits](../../../areas/wld/outpost.wld#L1).

The 40 room records have no authored fall-chance or current fields. Those
static counts and the room descriptions do not establish movement safety.

## Source-only inactive-WLD approach

The scan of all 95 currently inactive WLD files found one source-only approach
to active room #82124. It is not active ingress:

| Inactive source | Exit | Evidence |
| --- | --- | --- |
| surf.wld #206585 | East (D1) to #82124 | [Room](../../../areas/wld/surf.wld#L138217), [exit](../../../areas/wld/surf.wld#L138225), [target](../../../areas/wld/surf.wld#L138228) |

No other scanned inactive WLD file points into the local range. Preserve this
approach on Surf candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).
It adds one to the curated source-only total, now 3,456 overall and 219 for
Surf; ud_map2 remains 17. The approach does not change the active graph.

## Typed dependency reconciliation

The local reset file references all 22 mobile and all 21 object prototypes.
The local shop and reset agree on imported Heavens products #363, #368, and
#369. Current-source scans found:

| Consumer scan | Result |
| --- | --- |
| External ZON reset roles | No outside reset targets a local room, mobile, or object among 441 other current ZON files. |
| External QST numeric references | No exact numeric token to a local room, mobile, or object among 264 other current QST files. The Tharnadian Ruin QST has the named Perrin/Cathedral link described above. |
| External SHP products | No shop product targets a local object among 179 other current shop files. |
| External story-sidecar VNUMs | No exact local room, mobile, or object VNUM among 232 current story sidecars. |
| External MOB/OBJ definitions | No duplicate definition of a local mobile or object VNUM among 445 other MOB and 441 other OBJ files. |
| External type-25/type-29 room targets | Among 872 ITEM_TELEPORT and 380 ITEM_SWITCH declarations, no target points into a local room. |
| C/C++ exact local VNUM references | No exact room, mobile, or object VNUM was found under src/. The generic player-built outpost code uses #97800/#97801; active area Buildings is zone #978, a separate source identity ([code constants](../../../src/world/buildings.h#L31), [zone](../../../areas/zon/buildings.zon#L1)). |

The WLD door-key field scan across all 446 current WLD files found only four
uses of local object #82110: both sides of the local Treasury pair and both
sides of the active Tharnadian Ruin Cathedral pair. No WLD door-key field uses
another local outpost object VNUM. A broad raw-number scan also encounters
82137 in the metadata header for unrelated worn object #139977 in
forofcon.obj; its object value vector is zeroed, and it is not a WLD target,
reset role, or portal/switch target. These are bounded source scans; they do
not cover editor aliases, server memory, databases, persisted references, or
runtime behavior.

## Title, followed history, and disposition

After color stripping and normalization, The Human Outpost is unique among
the 442 current .zon titles. Other current titles that include Outpost are
distinct. The similarly named active Buildings zone #978 and the generic
player-built outpost subsystem have separate source identities. Followed
WLD, MOB, OBJ, ZON, QST, and SHP histories retain the outpost stem through the
2009 area-tree move commit 642da5400 and later same-stem edits. No alternate-
stem predecessor, retirement, or whole-zone replacement is established.

Keep active outpost **DO NOT MOVE**. Preserve the Surface boundary, local room
and prototype VNUMs, Perrin dialogue, the two local/external key dependencies,
and shop/reset relationships. Keep Surf candidate row 97 **RETAIN IN PLACE**
with its source-only approach. No outpost attic candidate row or migration
clearance is added.

## Builder follow-up

1. Confirm whether Cathedral access is intended to use ethereal key #82110,
   holy symbol #82118, or both, and whether the symbol has a source-defined
   interaction beyond its description.
2. Reconcile Perrin's refusal with the Tharnadian Ruin quest statements about
   granting the key to a worthy character and giving access to no one.
3. Confirm whether reusing #82110 for the Outpost Treasury is intentional and
   how Perrin's key is meant to be reached in the reset state.
4. Explain the sick guards and whether their condition is part of another
   story.
5. Identify the condition that triggers the shop refusal and confirm the
   intended imported medical stock.

The audit records authored files and bounded source references. It does not
qualify live dialogue, shop access, door commands, traversal, safety, or
return.

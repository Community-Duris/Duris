# Alatorin object bank

Canonical key: component:alatorin_objects. Physical coverage row 357. This is
an object-only source set, not a mapped zone. Its 408 prototypes contain
Alatorin lore, equipment, keys, monuments, and interactive portals. The
[technical audit](../quest-story-followups/ALATORIN_OBJECTS_AUDIT_REVIEW.md)
traces the current Alatorin copies, historical loader stub, quest and reset
consumers, and VNUM conflicts. The [builder review](ALATORIN_OBJECTS_BUILDER_QUESTIONS.md)
records the owner decisions.

## The source's authored story

The prototypes evoke the dwarven city and its history. Object #83100 is a
statue of Taark Shanat, named as the ruler and hero of the shield dwarves of
Alatorin ([source](../../../areas/obj/alatorin_objects.obj#L1)). The
Wyrmskull Throne, #83102, carries four blue-dragon skulls and a story crediting
Taark Shanat with killing the dragons ([source](../../../areas/obj/alatorin_objects.obj#L31)).
Other landmarks include a statue of Breuldin (#83203), a golden altar (#83217),
a mithril lever protruding from a ruined statue (#83280), an ancient wedding
band, Shanat keys and armor, and the tome The Story of Jedd Burningbraids
(#83270; [source](../../../areas/obj/alatorin_objects.obj#L2579)). The prose
connects craftsmanship, religion, military identity, trade, and the city's
older ruins. These are item descriptions; the file has no WLD rooms to place
them in.

The object bank also contains a portal network that joins Alatorin's
Elemental Fusion hall and four elemental dens to a shadow globe, Glass Prison,
the Temple of Dumathoin, and other named destinations. Three objects point
outside Alatorin's numeric range: a shadow rift to Ard'gral, a wormhole to
Astral Main, and a mist portal to Ethereal Main. The objects' commands and
stored targets are source data; they do not establish when a player can find
or use them or whether a safe return is available.

## Type-25 targets

The 24 type-25 objects use value[0] as a room VNUM and value[1] as the
triggering command. Sixteen use CMD_ENTER; the remaining eight use sit,
west, east, south, north, touch, rub, and pray. Target titles below come
from the current WLD records. Several Alatorin targets also have different
same-number rooms in the unlisted Nexus56 source.

| Object | Description / trigger | Stored target | Current WLD identity |
| --- | --- | --- | --- |
| #83102 | Wyrmskull Throne / sit | #83882 | Hallway Behind the Wyrmskull Throne, Alatorin |
| #83120 | Dark shadow portal / enter | #83311 | Within a Globe of Shadow, Alatorin |
| #83121 | Dark shadow portal / enter | #83310 | Shadowy Summoning Platform, Alatorin |
| #83122 | Directional teleporter / west | #83311 | Within a Globe of Shadow, Alatorin |
| #83123 | Directional teleporter / east | #83311 | Within a Globe of Shadow, Alatorin |
| #83124 | Directional teleporter / south | #83311 | Within a Globe of Shadow, Alatorin |
| #83125 | Directional teleporter / north | #83311 | Within a Globe of Shadow, Alatorin |
| #83130 | Portal / enter | #83337 | Den of Elemental Fire, Alatorin |
| #83131 | Portal / enter | #83308 | Bend in the Hallway of Elemental Fusion, Alatorin |
| #83132 | Portal / enter | #83338 | Den of Elemental Earth, Alatorin |
| #83133 | Portal / enter | #83309 | Hallway of Elemental Fusion, Alatorin |
| #83134 | Portal / enter | #83339 | Den of Elemental Water, Alatorin |
| #83135 | Portal / enter | #83306 | Bend in the Hallway of Elemental Fusion, Alatorin |
| #83136 | Portal / enter | #83305 | Hallway of Elemental Fusion, Alatorin |
| #83137 | Portal / enter | #83340 | Den of Elemental Air, Alatorin |
| #83138 | Floating bauble / touch | #83341 | Glass Prison, Alatorin |
| #83141 | Rift spewing shadow / enter | #30453 | Ard'gral, Plane of Dreams |
| #83203 | Statue of Breuldin / rub | #83408 | Ancient Staircase, Alatorin |
| #83217 | Golden altar / pray | #83404 | Rock-Strewn Embankment, Alatorin |
| #83218 | Tear in space / enter | #83397 | Temple of Dumathoin, Alatorin |
| #83301 | Wormhole / enter | #19721 | Great Worm Hole, Astral Main |
| #83432 | Swirling portal of mist / enter | #12511 | Pocket of Antimatter, Ethereal Main |
| #83433 | Shimmering portal / enter | #83823 | Top of the Tower of Xamora, Alatorin |
| #83434 | Shimmering portal / enter | #83831 | Ancient Lava Tube, Alatorin; active copy instead stores #83188 |

The shared travel handler reads value[0] and resolves it as a room VNUM. The
active Alatorin prototype for #83434 is reset in room #83823 but points to
#83188, Atop the Mystic's Tower. This source-only copy points to #83831, an
Ancient Lava Tube. Both targets are in active Alatorin; the difference is
between competing object records, not evidence that either route is currently
selected. Object #83301 is the same record in both files and is reset in
Alatorin room #83487 toward Astral Main's Great Worm Hole. See the
[reset lines](../../../areas/zon/alatorin.zon#L1622) and
[active portal reset](../../../areas/zon/alatorin.zon#L1751).

## Identity and safe disposition

The current full area list loads active Alatorin and does not list
alatorin_objects. The separate file has no WLD, MOB, ZON, QST, SHP, map, or
story sidecar. It has no zone title or local world graph.

The [audit](../quest-story-followups/ALATORIN_OBJECTS_AUDIT_REVIEW.md#historical-loader-status-and-object-identity)
shows that all 408 object VNUMs also exist in active alatorin.obj: 204 current
records match after line-ending normalization and 204 differ. A separate
168-ID overlap with nexus56.obj has different prototypes throughout. Keep
these object identities distinct from Alatorin's active source and from the
unlisted Nexus56 candidate.

Preserve this file in place while saved-player object identity, the competing
prototype versions, and all reset, quest, shop, story, and code references
remain unresolved. Its 2010 history explicitly describes a temporary loader
stub added because existing players' Alatorin objects were causing crashes;
current absence from areas/AREA does not prove that persisted references are
gone.

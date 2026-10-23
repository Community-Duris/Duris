# The Minizones of the Underdark repository-wide source audit

Checkpoint 209 maps active udmini against its authored room, mobile, object, and reset text; active and inactive WLD boundaries; chance-fall behavior; typed outside consumers; title ownership; and followed source history. This is a static source audit. It does not inspect a live server, player state, database, editor, persisted world state, or actual gameplay. The [source dossier](../zone-stories/UDMINI.md) summarizes the setting and builder questions.

## Source identity and authored content

The active area list identifies udmini with range note *885-886. Its zone header is #885, titled The Minizones of the Underdark, with last room #88520 and reset mode 2. The WLD contains 21 rooms #88500–#88520, the MOB has four prototypes #88500–#88503, and the OBJ has four prototypes #88500–#88503. The zone contains 20 M, two O, and one P reset commands. All four mobile prototypes and three of four object prototypes are referenced by these resets. Object #88500 has no reset reference. The source appears in both areas/AREA and areas/qst/AREA, but has no current udmini.qst, SHP file, or story sidecar ([area list](../../../areas/AREA#L312), [zone header](../../../areas/zon/udmini.zon#L1), [QST index](../../../areas/qst/AREA#L278), [coverage row 322](ZONE_COVERAGE_REGISTER.csv#L322)).

The WLD contains two separately described locations: a glowing hollow giant mushroom (#88500–#88501) and a corpse-covered maggot cave (#88502–#88520). The latter is home to reset-placed growing fire larvae and fire beetles. The two locations are separate WLD components, not rooms in one traversable local graph.

## Active WLD graph, boundaries, and fall state

The local WLD has 39/39 resolving exits: 38 local and one outbound. The raw local graph has weak components sized 19 and 2. Both active inbound targets, #88502 and #88515, are in the 19-room cave component; their combined all-exits directed reach is 19/21 rooms. The two-room mushroom branch (#88500–#88501) has no active WLD inbound edge. The local ZON has no door reset commands, so no reset-closed doors reduce this source graph. No local current fields are authored.

The active boundaries are:

| Neighbor | Inbound edge | Local outbound edge | Pairing |
| --- | --- | --- | --- |
| ConnectorZones | #54239 down (D5) to #88502 ([source](../../../areas/wld/connectorzones.wld#L12269)) | #88502 up (D4) to #54194 ([source](../../../areas/wld/udmini.wld#L31)) | No direct reverse edge |
| Kvarkpass Connector | #54811 up (D4) to #88515 ([source](../../../areas/wld/kvarkpass_connector.wld#L4507)) | No WLD edge back from #88515; reset object #88503 enters to #54811 ([object](../../../areas/obj/udmini.obj#L42), [reset](../../../areas/zon/udmini.zon#L11)) | One-way WLD inbound; item return source |

These yield two inbound, one outbound, and zero reciprocal WLD boundary pairs. The all-95 inactive-WLD scan found no source-room VNUM overlap with #88500–#88520 and no inactive exit targeting a local room. The exact external WLD scan found only the two active inbounds above; no source-only approach or attic candidate is added.

Two local rooms declare F 100: #88502 and #88520 ([room records](../../../areas/wld/udmini.wld#L31), [lower cave room](../../../areas/wld/udmini.wld#L335)). The WLD loader stores F as chance_fall; room-entry handling checks the configured percentage and calls falling_start. For a normal grounded actor without an existing fall, flight, levitation, or a successful climb catch, the open-down path from #88502 continues to #88520 and then #88503. #88520 also has its own 100% check on entry. Fall routing follows open downward exits and applies fall effects; the WLD-only 19/21 reach result does not model this movement or safety ([loader](../../../src/world/db.c#L1395), [entry handler](../../../src/world/handler.c#L1581), [fall start and exclusions](../../../src/world/falling.c#L113), [downward route](../../../src/world/falling.c#L203)).

The reset-placed large crack #88503 in room #88515 is an ITEM_TELEPORT whose value[1] is CMD_ENTER (7) and value[0] is Kvarkpass room #54811. That target itself declares F 100 and has an open down exit to #54832; #54832 declares F 100 and exits down to #54812. Arrival at the target is therefore a fall-state question for ordinary actors, not by itself proof of a safe return ([teleport handler](../../../src/magic/spell_travel.c#L932), [command constant](../../../src/cmd/interp.h#L42), [target room](../../../areas/wld/kvarkpass_connector.wld#L4507), [lower shaft](../../../areas/wld/kvarkpass_connector.wld#L4931)).

## Local items and outside story consumers

Local resets place gills container #88501 in room #88501 and nest spores #88502 inside it. The large crack #88503 is placed in room #88515. The fourth local object, mushroom #88500, is an ITEM_TELEPORT with destination #88500 and CMD_ENTER, but no local or external ZON reset references it. Consequently, its declaration does not currently connect the isolated mushroom branch to the active cave arrivals. The gills prototype also contains raw T fields 512, 0, 10, and 60. The object loader maps these values to trap fields; the source does not identify the intended effect. See the [object records](../../../areas/obj/udmini.obj#L1), [zone placements](../../../areas/zon/udmini.zon#L9), [teleport value handler](../../../src/magic/spell_travel.c#L991), and [object trap parser](../../../src/world/db.c#L4749).

The spores prototype #88502 has two active outside QST consumers:

| Consumer | Exact source use | Source or reward |
| --- | --- | --- |
| Myconid alchemist #2320 | QST accepts exact object #88502 ([QST](../../../areas/qst/myconid.qst#L22)); sidecar names the same item ([story](../../../areas/story/myconid.story.json#L180)) | Rewards 50,000 currency units. |
| Maerg #29218 in Newbie | QST accepts exact object #88502 and returns #29291 ([QST](../../../areas/qst/newbie.qst#L241), [reset on drow mother](../../../areas/zon/newbie.zon#L361)); sidecar maps the same exchange ([story](../../../areas/story/newbie.story.json#L511)) | Optional preparation for Foet’s separate jar request; the supplied item can also fit. |

The Newbie reset is a second active source declaration for #88502, independent of the local gills-and-spores reset. These sources and exchanges do not prove current stock or a player’s recovery path. The [Myconid audit supplement](MYCONID_AUDIT_REVIEW.md) and [Ailvio dossier](../zone-stories/AILVIO.md) already record their own campaign-level dependencies.

Repository-wide typed scans found no other outside reset-role, room/mobile QST, shop-product, duplicate MOB/OBJ, WLD door-key, ITEM_TELEPORT/ITEM_SWITCH destination, or C/C++ exact VNUM reference. The two exact outside QST references are the object input #88502; the two matching story sidecars contain no other local VNUM. These are bounded source scans, not claims about runtime inventory, editor aliases, persisted data, or players’ access.

## Title, followed history, and disposition

After color stripping and normalization, The Minizones of the Underdark is unique among the 442 current ZON titles. Followed WLD, MOB, OBJ, and ZON histories retain the udmini stem through the 2009 area-tree move commit 642da5400 and later same-stem edits. No alternate-stem predecessor, retirement, or whole-zone replacement is established.

Keep active udmini DO NOT MOVE. Preserve the two active incoming boundaries, room and prototype VNUMs, nested spores source, fall fields, large-crack teleport target, and external #88502 quest/reset consumers. No inactive-WLD candidate row or migration clearance is added. See the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-209-udmini-title-and-history), [coverage row 322](ZONE_COVERAGE_REGISTER.csv#L322), and [source dossier](../zone-stories/UDMINI.md).

## Builder follow-up

1. Decide how the giant mushroom component is meant to be entered and whether the unplaced self-targeting mushroom object should be reset somewhere.
2. Confirm whether the two local F 100 fields are intended to force entrants into the maggot caves and what safe, climb, fly, and return behavior is expected.
3. Confirm what the gills object trap data is meant to do.
4. Preserve both active outside uses of spores #88502 and Newbie’s second reset source without inferring available stock or journey order.
5. Clarify the QST/AREA entry when there is no local QST, shop, or sidecar.

## Checkpoint 409: udmini builder-question coverage

Physical coverage row 322 now has twenty explicit, direct-source builder
questions with recommended resolutions in the [UDMINI dossier](../zone-stories/UDMINI.md#explicit-builder-questions-and-recommended-resolutions).
They preserve the separate mushroom and corpse-cave components, record both
outside spore consumers and Newbie's independent reset source, and qualify the
F 100 fall fields and crack teleport without promising access, safe arrival,
or current stock. No area or gameplay source changed. See the [connectivity
review](WORLD_CONNECTIVITY.md#checkpoint-409-udmini-boundaries-and-directed-access),
[title/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-409-udmini-title-and-bounded-history),
and [coverage row 322](ZONE_COVERAGE_REGISTER.csv#L322).
## Checkpoint 864: current arrivals, fall state, and spore dependencies carried forward

CP864 carries forward the Checkpoint 209 source audit against the unchanged pinned area/code snapshot. Active `udmini` remains 21 rooms and 39/39 resolving WLD exits: 38 local and one outbound, with weak components sized 19 and 2. ConnectorZones #54239 down to #88502 and Kvarkpass Connector #54811 up to #88515 are the two active inbounds; #88502 up to ConnectorZones #54194 is the only local outbound. Neither inbound has a direct reciprocal WLD edge. Both arrivals are in the corpse-cave component; the two-room mushroom branch #88500–#88501 still has no active WLD ingress.

The F 100 entries at #88502 and #88520 and the reset-placed #88503 teleport to Kvarkpass #54811 remain separate arrival/fall-safety concerns. The authored source graph is not evidence of safe arrival, ordinary return, or live player access. The all-95 inactive-WLD scan found no overlap or source-only approach to any `udmini` room, and no candidate attic row is added. Checkpoint 209's 3,458 overall / 220 Surf / 17 `ud_map2` aggregate is historical; current cumulative source-only totals are 11,993 overall / 415 Surf.

The typed closure remains: local spore object #88502 has two active outside QST consumers (Myconid alchemist #2320 and Maerg #29218 in Newbie); Newbie also has an independent reset source. The local QST/AREA index entry still has no matching local QST file, and the two-room mushroom teleport #88500 remains unreferenced by any local or external reset. The source dossier's trap-field ambiguity on gills object #88501 remains unresolved. Keep active `udmini` **DO NOT MOVE**; no candidate, route activation, or migration clearance is added. See the [source dossier](../zone-stories/UDMINI.md), [Checkpoint 209 attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-209-udmini-and-source-only-retention), and [coverage row 322](ZONE_COVERAGE_REGISTER.csv#L322).

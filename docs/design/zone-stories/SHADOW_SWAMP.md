# Shadow Swamp: comprehensive source story map

**Source-comprehensive, revision one — October 10, 2026. Gameplay qualification remains open.**

This map reconciles the active `swamp_one` WLD, MOB, OBJ, and ZON sources with
the two active area lists, the adjoining `swamp_two` source, bounded typed
consumers, and followed source history. It records authored text and references;
it does not claim that a reset item is obtainable, that a door works as its
comments imply, or that the Death Bog is safe to traverse.

## Evidence and review boundary

The active [area list](../../../areas/AREA#L130) names `swamp_one` at zone 260.
The separate [quest index](../../../areas/qst/AREA#L122) also lists the stem,
but there is no `areas/qst/swamp_one.qst` file, local QST giver, local shop, or
`swamp_one.story.json`. That index is not the production world list. The
registered source files are the complete 36-room
[WLD](../../../areas/wld/swamp_one.wld), 12-prototype
[MOB](../../../areas/mob/swamp_one.mob), 16-prototype
[OBJ](../../../areas/obj/swamp_one.obj), and [ZON](../../../areas/zon/swamp_one.zon).

The local reset table has two door resets, three ground-object resets, twenty
mobile resets, and six equipment resets. Korbin is reset in the clearing and
Phyla in her lair; the bugs, dragonflies, snakes, fish, and centaur populate the
trail and bog. The corpse and dirty key are reset together in the weeds pocket.
The zone does not reset Mog, the swamp trolls, guards, or troll king locally;
`swamp_two.zon` owns those placements while importing their mobile prototypes.

## Place and story beats

| Rooms | Source text and authored movement | Reading for builders |
| --- | --- | --- |
| #26001 | The entry is named “Entrance to Shadow Swamp.” Strange sounds, disappearing images, and dread establish the threshold. East returns to Surface #608724; south enters the trail. | Preserve the reciprocal Surface boundary and the entry’s foreboding tone. [WLD](../../../areas/wld/swamp_one.wld#L1) |
| #26002–#26008 | A rarely used trail branches through the swamp. Unknown creature sounds appear; #26004–#26008 turn through forks before the route reconnects. | This is a navigable path, not an authored quest step. Encounter resets are separate from the room descriptions. [WLD](../../../areas/wld/swamp_one.wld#L17) |
| #26009–#26010 and #26020 | A feeling of goodness draws south from #26009 to a small clearing. A large tree blocks further southern travel; the room says someone lives there. Korbin is reset here. | The text identifies Korbin’s clearing but does not explain the “goodness” or connect Korbin to another plot. Keep those as open builder intent. [WLD](../../../areas/wld/swamp_one.wld#L143), [resets](../../../areas/zon/swamp_one.zon#L34) |
| #26011–#26018 | A dreadful smell strengthens near the trail. Blood marks the route, footprints fade, and weeds obscure a small opening at #26017. East reaches hidden room #26036; west returns to #26017. | This is the clearest authored clue-to-hidden-room sequence. The dirty key and decomposing corpse are reset in the pocket. [WLD](../../../areas/wld/swamp_one.wld#L177), [pocket](../../../areas/wld/swamp_one.wld#L676), [resets](../../../areas/zon/swamp_one.zon#L19) |
| #26019 | “The End of the Trail” warns that the ground south is soft and unstable. The sign reset here says “Quicksand Beware! Turn back now this means you!” The south exit is named “Quicksand” and enters Swamp Two #26100; west returns along the trail. | Preserve the reciprocal area boundary and warning together. Static WLD data does not establish the crossing’s actual movement or safety behavior. [WLD](../../../areas/wld/swamp_one.wld#L331), [sign and boundary](../../../areas/zon/swamp_one.zon#L15) |
| #26021–#26033 | The trail ends beside Death Bog. The prose escalates from waist-deep stagnant water to extremely deep water, murk, moving shapes, and directions described as impassable or blocked by growth. | Treat these as environmental warnings, not proof of a movement rule. No fall-chance or current marker is present in this WLD. [WLD](../../../areas/wld/swamp_one.wld#L360) |
| #26034–#26035 | A dry patch opens into Phyla’s lair, described with bubbling pots, strange vials, creatures, and exotic plants. Phyla is reset at #26035 with a witchcraft hat, shadow cape, and dingy key. | The source places a witch in a distinct dwelling; it does not author a local quest contract. The key is consumed by a later Swamp Two door reference, but its obtainability is not established here. [WLD](../../../areas/wld/swamp_one.wld#L646), [resets](../../../areas/zon/swamp_one.zon#L42) |

## Prototype and reset roles

The mobile file declares Korbin (#26001), Phyla (#26002), Mog (#26003), swamp
troll (#26004), swamp troll guard (#26005), swamp troll king (#26006), bug
(#26007), snake (#26008), dragonfly (#26009), fish (#26010), elite troll guard
(#26011), and centaur (#26012). The active local ZON places Korbin, Phyla, bugs,
dragonflies, snakes, fish, and a centaur. Mog and the troll family are imported
as **mobile** prototypes by [Swamp Two resets](../../../areas/zon/swamp_two.zon#L34).
These IDs must remain type-qualified: #26001–#26012 also occur in the local
object namespace as different prototypes.

The object file declares the druid staff (#26001), swamp root (#26002), witchcraft
hat (#26003), shadow cape (#26004), dingy key (#26005), decomposing corpse
(#26006), dirty key (#26007), sign (#26008), protection cloak (#26009), belt of
the ancients (#26010), bent key (#26011), platemail of life (#26012), glowing
crystal sphere (#26013), Nightbringer mace (#26014), troll tooth (#26015), and
black leather saddle (#26016). The local reset equips the staff and root on
Korbin, the hat/cape/dingy key on Phyla, and the saddle on the centaur; it places
the sign at #26019 and the corpse plus dirty key at #26036. Swamp Two imports
**object** prototypes #26011–#26014. A bounded active reset/shop/quest scan found
no placement for #26009, #26010, or #26015; that finding does not prove global
non-use. [Object source](../../../areas/obj/swamp_one.obj), [local reset source](../../../areas/zon/swamp_one.zon#L15), [Swamp Two object resets](../../../areas/zon/swamp_two.zon#L54).

## Hidden passage and key progression

The weed opening is a two-way authored WLD connection. The first field on its
WLD exit is interpreted by the room loader as an initial door state: state 6 is
masked to 2 and loads an `EX_ISDOOR` with the legacy `EX_PICKABLE` flag; state 2
on the return side does the same. `EX_PICKABLE` is marked ignored for backwards
compatibility. The two ZON resets then set the east side to state 5 (closed plus
secret) and the west side to state 1 (closed, not secret, and unlocked). The
resets do not set the locked state. See the [exit records](../../../areas/wld/swamp_one.wld#L289),
[reset comments and states](../../../areas/zon/swamp_one.zon#L8),
[loader](../../../src/world/db.c#L1567), [reset handler](../../../src/world/db.c#L7868),
and [door flags](../../../src/core/defines.h#L442). This is a source-state
description only; it does not qualify secret discovery, opening, or return.

The items form an authored cross-area key sequence:

1. Phyla wears local object #26005, the dingy key. Swamp Two declares it as the
   key for the locked Quicksand-to-Beneath-the-Sand boundary at #26100/#26101.
2. The corpse and local object #26007, the dirty key, are placed in #26036.
   Swamp Two declares #26007 as the key for the locked tunnel door between
   #26101 and #26102.
3. Swamp Two places local object #26011, the bent key, with a swamp troll guard
   in #26132; it is the key on the locked #26132/#26133 king-chamber boundary.

The Swamp Two WLD and reset source preserve those separate rooms, doors, and
prototype owners ([door map](../../../areas/wld/swamp_two.wld#L1), [reset table](../../../areas/zon/swamp_two.zon#L8)).
They do not prove that any key or guard is guaranteed to spawn or that a player
can obtain the key. The swamp troll king’s room also receives local object
#26012, #26013, and #26014 through Swamp Two’s object resets; they are not local
Swamp One stock declarations.

Swamp Two room #26101 describes Mog’s sanctuary as safe “now that Mog is gone,”
while the Swamp Two reset table places Mog at #26100. The room prose is static
source text; no dynamic update tied to Mog’s presence was found in this bounded
review. Preserve the distinction between narrative wording and a live encounter
state.

## Typed external references

| Source | Exact reference | What the source supports |
| --- | --- | --- |
| [Swamp Two ZON](../../../areas/zon/swamp_two.zon#L34) | 26 `M` resets import Swamp One mobiles #26003–#26006 and #26011. Four `G` resets import local objects #26011–#26014. | Cross-area reset ownership only; no local duplicate reset or guaranteed availability is implied. |
| [Caves of Mt. Skelenak QST](../../../areas/qst/caves_skelenak.qst#L203) and [sidecar](../../../areas/story/caves_skelenak.story.json#L423) | Object #26013 appears in a `G I` item command and in a typed `item_vnums` record. The sidecar completion key says give #26013, receive #4031, disappear. | This is an outside optional sphere exchange, not a Swamp One native quest. The reward and receipt belong to Caves. |
| [Maintained assignment](../../../src/specs/specs.assign.c#L2014) and [Nightbringer handler](../../../src/specs/specs.underworld.c#L1363) | Object #26014 is bound to `nightbringer`; source comments show a 1/30 sleep branch and a 1/40 darkness branch. | A code binding and implemented source branches are present. They do not prove installed runtime behavior or quest ownership. |

The bounded scans found no same-type external WLD/MOB/OBJ VNUM collision, no
external QST giver collision with the local mobile IDs, no other typed sidecar
consumer, no external shop product row for these objects, no local SHP file, and
no type-25 portal targeting a local room. No other C++ exact-integer match for
these room/mobile/object IDs was found. These are bounded source findings, not
proof about untracked sources or runtime-created items.

## Identity, history, and builder handoff

The normalized `.zon` title is “Swamp,” unique among 442 current ZON titles and
used by one of 351 active area contributors. `swamp_one` and the separately
registered `swamp_two` (“The Lair of the Swamp Troll King”) have distinct room
ranges, active WLDs, and story roles. Followed tracked history stays on the
`swamp_one` stem: WLD and OBJ edits through 2013, MOB and ZON edits through 2015,
with tracked history reaching the 2009 area-tree move. No alternate-stem rename,
explicit predecessor, or whole-zone replacement was established in those
histories. No local QST, SHP, or story-sidecar history exists because those
files are absent. The boundary review and twenty owner questions are in the
[Swamp One audit supplement](../quest-story-followups/SWAMP_ONE_AUDIT_REVIEW.md).

**Disposition: DO NOT MOVE.** Preserve active area membership, both reciprocal
boundaries, the hidden room and its reset-state asymmetry, the typed Swamp Two
reset ownership, the Caves sphere consumer, and the separate inactive Surf map
identity. This dossier does not clear gameplay repairs or any attic migration.

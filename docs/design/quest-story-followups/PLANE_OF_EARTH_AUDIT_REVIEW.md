# Plane of Earth: connectivity, dependencies, history, and builder decisions

Reviewed October 10, 2026 for active `plane_earth_one`, zone 238. The source
dossier is [PLANE_OF_EARTH.md](../zone-stories/PLANE_OF_EARTH.md).

## Active graph and raw directed reachability

The 129-room WLD has 763 exits and all resolve in the current loader: 760 are
internal and three leave the area. The undirected view is one 129-room weak
component. Directed analysis finds a 121-room strongly connected core and
eight singleton components. Four ordinary map rooms (#23818, #23845, #23850,
and #23918) have no local inbound exit and each points into the core. The
four active inbound targets are all in that core. The remaining singleton
rooms are the technical tail #23926–#23929; #23926 has no local WLD inbound,
and #23927 is terminal. The graph does not establish whether dynamic travel,
mobile movement, or reset state makes any such room available to a player.

All authored exit door flags and key VNUMs are zero. The source has no
negative or unresolved WLD targets, fall-chance markers, or currents.

| Active inbound source | Direction and destination | Return evidence |
| --- | --- | --- |
| `prison.wld` #7494 | North, east, south, up, and down to #23905; five records from one source room ([register rows 169–173](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L169)) | No reverse WLD exit |
| `rftjngle.wld` #80444, “Elemental Ranger Load Room” | West to #23805 ([row 976](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L976)) | No reverse WLD exit |
| `alatorin.wld` #83344, Earth conduit | Down to #23860 ([row 1024](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1024)) | No reverse WLD exit |
| `earthp.wld` #131200, “A Newly Cut Section of Earth” | Up to #23908 ([row 1299](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1299)) | Reciprocal with Plane of Earth #23908 down |

The one reciprocal boundary pair is Plane of Earth #23908 down ↔ Earthp
#131200 up ([Plane of Earth row 386](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L386)).
The two other outbound records both point from technical rooms into Grumbar's
Domain: #23926 up and #23928 south to #131364, with no reverse WLD record
([rows 387–388](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L387)). Room #131364 is
Grumbar's lair. Treat the first boundary as a map pair and the two technical
links as separate one-way records; none proves safe return.

These are eleven active boundary records in total. The five Prison
directions share one source and one destination, so do not describe them as
five independent entrances without builder confirmation.

## Code and object routes

The static WLD register omits routes implemented through spells and objects:

| Route | Source evidence | Classification |
| --- | --- | --- |
| Gate, Nether Gate, Plane Shift | Earth anchor #23801 in the planar destination table and calls to `get_room_in_zone` ([spells.c](../../../src/magic/spells.c#L295), [helper](../../../src/magic/spell_travel.c#L747)) | The helper samples the loaded zone range and applies entry, no-teleport, and hometown checks. Exact live landing and return remain unqualified. |
| Negative Plane fissure → Earth #23833 | Active type-25 object #26660 ([prototype](../../../areas/obj/negplane.obj#L942)) reset in Negative Plane #26693 ([reset](../../../areas/zon/negplane.zon#L166)) | Declared object destination; no reverse WLD record. It is not an active WLD inbound. |
| Earth color pool #23804 → Ethereal #12492 | Local object values and reset ([object](../../../areas/obj/plane_earth_one.obj#L44), [reset](../../../areas/zon/plane_earth_one.zon#L9)); `magic_pool` binding and handler ([assignment](../../../src/specs/specs.assign.c#L1992), [handler](../../../src/specs/specs.teleport_pools.c#L107)); target [Ethereal room](../../../areas/wld/ethereal_main.wld#L3823) | Code teleports the user to the declared room. Its 200-damage effect leaves non-trusted users at one hit point when the damage would be lethal. No paired WLD return is declared. |
| Mind Travel and illithid veil | Room destination arrays in [spell_mind_travel](../../../src/classes/psionics.c#L612) and [illithid_teleport_veil](../../../src/specs/specs.ixarkon.c#L18) | Both lists include Earth rooms #23805 and #23812. These are code references, not WLD exits. |

## Inactive-WLD approaches and active prototype consumers

All 95 non-active files in `areas/wld/` were scanned with the loader's two
direction-description fields and exit records. None targets #23801–#23929;
no inactive approach or source-room overlap was found. The two other `.wld`
files outside that directory, `areas_mini/mini.wld` and
`areas/mob/astral_main.wld`, also contain no reference into this range. No
attic candidate row or source-only count changes.

The bounded typed scan found these active cross-area links and exact prototype
uses:

| Reference | Source evidence | Classification |
| --- | --- | --- |
| Heavens final QST requirements #23809, #23808, #23807, #23806 | [Heavens QST lines 165–168](../../../areas/qst/heavens.qst#L165) | Four Plane of Earth object requirements, paired by local resets across Entemoch and Ogremoch. Objects #23808 and #23809 carry `_noquest_` ([object aliases](../../../areas/obj/plane_earth_one.obj#L97), [#23809 alias](../../../areas/obj/plane_earth_one.obj#L111)); current code uses this tag to exclude generic random world-quest reward sources ([policy](../../../src/world/world_quest_policy.c#L151), [reward selection](../../../src/world/world_quest_policy.c#L276)). It does not negate these fixed QST VNUM requirements. The quest and its receipt remain owned by Heavens; live item availability is unqualified. |
| Ogremoch mobile #23806 | [Surfacemini sidecar line 228](../../../areas/story/surfacemini.story.json#L228) | Typed as an Earth planar essence source; no topic or local quest outcome. |
| Plane of Earth object #23805 | [Temple of the Earth reset](../../../areas/zon/earth.zon#L503) | One active G reset import into the local #43682 encounter. The object prototype remains owned by Plane of Earth. |
| Plane of Earth mobiles #23802 and #23801 | [Grumbar's Domain resets](../../../areas/zon/earthp.zon#L510) and [#23801 reset](../../../areas/zon/earthp.zon#L574) | Seven #23802 reset rows at #131384 and one #23801 row at #131398. Preserve the shared mobile IDs; reset availability is not live-spawn evidence. |
| Plane of Earth mobile #23803 | [Tharnadian Ruin resets](../../../areas/zon/tharnadian_ruin.zon#L299) | Four active M reset rows at rooms #5667, #5670, and #5674. The mobile remains owned by Plane of Earth. |
| Object #23811 in CHAOS equipment data | [first profile](../../../src/account/chaos_eq_data.h#L168) and [second profile](../../../src/account/chaos_eq_data.h#L417) | Static slot-21 equipment references in two profiles, not live inventory records. |
| Ogremoch and Entemoch callbacks | [assignments](../../../src/specs/specs.assign.c#L1218) and [handler](../../../src/specs/specs.elemental_bosses.c#L27) | Mobiles #23806 and #23808 receive `ogremoch_shout`; #23807 receives `earth_treant`. The shout helper list includes #23927 and #23862, both room VNUMs with no current mobile prototype in `areas/mob`. The helper filters against mobile VNUMs ([comparison](../../../src/combat/justice.c#L650)), so these entries do not name current helpers. Preserve the callbacks pending owner review. |
| Object specials | [assignments](../../../src/specs/specs.assign.c#L1377), [Plane of Earth block](../../../src/specs/specs.assign.c#L1992), and [armor spell](../../../src/magic/spell_physical_armor.c#L9) | #23808 uses `artifact_stone`, #23806 `earthquake_gauntlet`, #23807 `blind_boots`, #23809 `artifact_hide`, #23804 `magic_pool`, and #23805 `zion_mace_of_earth`. The mace VNUM also boosts Living Stone absorption in `spell_armor`. These are compiled behaviors, not quest receipts. |

The current mobile catalog defines #23801–#23808 only in Plane of Earth; the
current object catalog defines #23801–#23811 only in Plane of Earth. The
bounded exact scan found no external SHP
product, external duplicate mobile/object definition, additional QST or
sidecar numeric reference, or additional type-25 destination into the Earth
range. Static code references above are recorded separately from live
resets, saved player state, and editor/tooling references, which remain
unqualified.

## Title, followed history, and retention

After color-code removal and whitespace normalization, **Plane of Earth** is
unique among all 442 current zone titles and among the 351 active area
contributors. Followed WLD, MOB, OBJ, and ZON histories remain on the
`plane_earth_one` stem back through the 2009 move into the current area tree,
with later same-stem edits. An August 2009 commit added `_noquest_` to objects
#23808 and #23809 and added several trap exits from #23926 into #23927; its
message says the exits reduce Entemoch's chance to reach the map. This remains
same-stem history, not predecessor or replacement evidence. No alternate-stem
predecessor, tracked rename, or whole-zone replacement is established.

Keep active `plane_earth_one` **DO NOT MOVE**. Preserve its VNUM ownership,
the four active WLD inbound groups, the Earthp reciprocal pair, the two
technical Earthp outbounds, the Neg Plane type-25 destination, the Ethereal
pool route and damage field, the Heavens object requirements, and every
cross-area mobile/object reset reference. No route, reset, story, or migration
change is proposed.

## Checkpoint 322: builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| EARTH-01 | What player objective, if any, should this area represent? | Preserve its environmental prose and encounters until an accepted action, outcome, and durable receipt are defined. |
| EARTH-02 | Should Plane of Earth gain a local QST or story sidecar, or should Heavens remain the owner of the four-item contract? | Keep the existing Heavens QST as the only source-defined contract until a separate local objective is specified. |
| EARTH-03 | Do “solid and crushed” reagents mean the four boots and gauntlets required by Heavens, and are Ogremoch's and Entemoch's reset placements intended as their sources? | Preserve all four object VNUMs and owner identities, keep the stone and dusty pairs with their current residents, and clarify text or acquisition only through a reviewed story and gameplay decision. |
| EARTH-04 | Should `_noquest_` on #23808 and #23809 prevent their use in Heavens' fixed QST requirements? | Treat the tag according to its current random reward-source filter; preserve the fixed QST VNUM requirements unless the Heavens owner changes that contract. |
| EARTH-05 | Are Prison #7494's five directions to #23905 intentional? | Retain all five records and review them as one source-room group before describing them as separate entrances. |
| EARTH-06 | Should Rftjngle #80444 west to #23805 have a return route? | Preserve the one-way WLD record until its player-facing role is confirmed. |
| EARTH-07 | Is Alatorin's conduit down to #23860 intentionally one-way? | Keep the current boundary and leave return travel unpromised pending owner review. |
| EARTH-08 | Is the reciprocal Plane Earth #23908 / Earthp #131200 boundary the intended ordinary connection to Grumbar's Domain? | Preserve both records and confirm their campaign sequence and access separately. |
| EARTH-09 | Are the technical exits from #23926 and #23928 to Grumbar's lair #131364 player routes or staging links? | Preserve the one-way records and confirm their use before changing either endpoint. |
| EARTH-10 | Should rooms #23926–#23929 remain in the Plane of Earth zone after the ordinary map end? | Keep them loaded pending a source-owner decision about their technical and reset roles. |
| EARTH-11 | Is the #23925 `IS_EARTH` end versus #23929 `EARTHP_VNUM_END` difference intentional? | Preserve both constants until their intended range semantics are confirmed; no call site was found in this review. |
| EARTH-12 | Is Ogremoch's reset room #23850 intended to have no local WLD inbound? | Keep the reset and map; qualify any intended encounter path through mobile movement or dynamic travel separately. |
| EARTH-13 | Are the other no-inbound map rooms #23818, #23845, and #23918 deliberate directed branches? | Preserve their authored exits and request route intent before creating return edges. |
| EARTH-14 | Is Entemoch meant to load in technical room #23926, whose title says “Ogremoch load room,” and are the trap branches to #23927 still intended? | Preserve the reset and trap exits while asking the source owner whether the room title or reset is stale and what purpose the branches serve. |
| EARTH-15 | Is the core monolith at #23865 descriptive scenery or meant to support an interaction? | Keep the current prose as environmental description until a specific interaction and receipt are defined. |
| EARTH-16 | Are the Lightning fragment at #23806 and Fire-sector Jet Stream #23833 intentional cross-element details? | Preserve the current descriptions and sectors pending a terrain/story decision. |
| EARTH-17 | Is Earth pool #23804's route to Ethereal #12492 and its 200-damage field intended? | Keep the target and damage behavior; review access, lethal floor, and return as one separate route decision. |
| EARTH-18 | Should Negative Plane fissure #26660 continue to arrive at the Fire-sector Jet Stream #23833? | Preserve the type-25 destination and its active reset until both owners confirm the endpoint and return plan. |
| EARTH-19 | Are #23927 and #23862 intentional entries in Ogremoch's helper list? | Retain the compiled callback while confirming whether these room VNUMs should be replaced with mobile prototype IDs. |
| EARTH-20 | Are the imported prototypes used by Earth, Earthp, Tharnadian Ruin, and Heavens intended to remain shared? | Preserve Plane of Earth's ownership and all exact reset/QST references; coordinate any future ID change across those consumers. |

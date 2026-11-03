# Rift Valley Jungle audit supplement

Checkpoint 340 reconciles active Rift Valley Jungle against its existing
comprehensive [story dossier](../zone-stories/RFTJNGLE.md) and generated
[source review index](../../reference/zone-story-audits/rftjngle.md). This pass
adds reset-aware WLD reach, active and source-only boundaries, inventory and
typed consumers, current map identity, tracked history, and builder questions.
It is a static repository review; live movement, encounter availability, safety,
and installed accounting remain unqualified. No native world data changed.

## Active WLD graph and reset-aware reach

The active contributor is listed in areas/AREA and areas/qst/AREA. Its WLD has
470 rooms and 1,229 source exits. The loader resolves 1,228: 1,219 local and
nine cross-area exits. The remaining positive target is #228371 from room
#80460 south. The local weak graph has five components: one of 466 rooms and
four single-room components.

All four active arrivals have reciprocal WLD records:

| Boundary | Active records |
| --- | --- |
| Woodseer #16950 east ↔ Jungle #80000 west | Reciprocal |
| Woodseer #16951 east ↔ Jungle #80001 west | Reciprocal |
| Woodseer #16952 east ↔ Jungle #80002 west | Reciprocal |
| Surface #568516 west ↔ Jungle #80469 east | Reciprocal |

The corresponding [Jungle WLD](../../../areas/wld/rftjngle.wld#L1) records
include room #80000 at line 1, #80001 at line 31, #80002 at line 65, pass
#80398 at line 10662, ranger room #80444 at line 11647, unresolved-exit room
#80460 at line 11970, and Surface trail #80469 at line 12132. See the
[Woodseer boundary](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L298),
[Jungle departures](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L969), and
[Surface return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1481).

Five resolved departures do not have an active reverse WLD record:

| Jungle source | Direction and destination | Source context |
| --- | --- | --- |
| #80398 | down to Pine Hollow #16008 | Mountain pass; destination has no reverse exit to #80398 |
| #80444 | north to Plane of Fire #25405 | Elemental ranger load room |
| #80444 | east to Plane of Water #23250 | Elemental ranger load room |
| #80444 | south to Plane of Air #24501 | Elemental ranger load room |
| #80444 | west to Plane of Earth #23805 | Elemental ranger load room |

From each of the four active arrivals, ordinary directed WLD traversal reaches
419 of 470 rooms. A strict baseline that applies all 42 D resets and treats
every closed, locked, or secret door as unavailable reaches 385 of 470. All 42
commands are unconditional, chance 100, and target unique room/direction pairs:
26 reset closed and unlocked, eight closed and locked, and eight closed and
secret. None adds the blocked bit. Reset mode two can fire regardless of zone
occupancy; the strict count is a door-state snapshot, not a claim that closed
unlocked doors cannot be opened or that secret doors cannot be found. The
shared [door reset handler](../../../src/world/db.c#L7857) and
[reset-mode check](../../../src/world/events.c#L429) define those limits.

The story dossier documents four local, paired item-teleporter routes which
the WLD-only walk excludes:

| Route | Jungle source, command, and destination | Paired return |
| --- | --- | --- |
| Fire | #80061, up, to #80162 | #80162, down, to #80061 |
| Water | #80072, up, to #80164 | #80164, down, to #80072 |
| Air | #80060, enter, to #80161 | #80161, enter, to #80060 |
| Earth | #80071, up, to #80163 | #80163, down, to #80071 |

If all four source-declared pairs are modeled as usable, the WLD-plus-portal
overlay reaches 423 rooms raw and 389 with the reset-door filter. This remains
a source graph estimate: it does not establish that a particular portal has
loaded, that a player can use its command, or that elemental-plane movement is
safe. The area also declares 44 fall rooms and 27 rooms with currents; those
are hazards requiring actual route and movement qualification, not blanket
safety ratings.
See the [zone resets](../../../areas/zon/rftjngle.zon#L8),
[door reset handler](../../../src/world/db.c#L7857), and
[reset-mode check](../../../src/world/events.c#L429).

## Current inactive-WLD sweep and Surface candidate

The current source tree contains 446 WLD files: 351 active stems and 95
non-active stems. The inactive-source scan found no room-ID overlap with
Rift Valley Jungle and two non-overlapping source-only approaches, both in
surf.wld:

| Inactive source | Direction and active target | Interpretation |
| --- | --- | --- |
| #226279, D2 (south), line 551267 ([Surf source](../../../areas/wld/surf.wld#L551267)) | #80469 | Alternate source-only approach to the current Surface trail room |
| #228371, D0 (north), line 595155 ([Surf source](../../../areas/wld/surf.wld#L595155)) | #80460 | Source-only reverse record for the active unresolved #80460 south exit |

Neither record is active ingress. Preserve both on the existing
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); Surf's
active-reference count remains 145. Its reviewed source-only WLD subset rises
from 254 to 256, and curated source-only totals rise from 4,490 to 4,492:
Surf 256, ud_map1 two, ud_map2 20, and Underworld2 eight.

The unresolved target #228371 has different inactive source identities.
surf.wld [calls it “Endless Plains of Grassy Pastures”](../../../areas/wld/surf.wld#L595151)
and gives it a north exit to active #80460. surface2011.wld
[calls the same number “The Azure Waters of a Storm Sea”](../../../areas/wld/surface2011.wld#L595792)
and has no north exit to #80460. Likewise, surf.wld #226279 is a
“Grassy Clearing” with a south exit to #80469, while surface2011.wld
[uses a different #226279 title and topology](../../../areas/wld/surface2011.wld#L551860)
and does not have that target. Preserve the source
families separately. The current build omits both maps, so neither candidate
supplies a live entrance or return route. See unresolved-exit register row 263
and the distinct [Surf row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97)
and [Surface2011 row 98](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L98).

## Typed and compiled dependencies

The [eight local SHP records](../../../areas/shp/rftjngle.shp#L1) have
shopkeeper VNUMs 80111 through 80201; the
coverage row's former zero-shop inventory was stale. The exact active external
consumer scan found no ZON prototype reset, QST item exchange, structured story
sidecar item reference, or item-type-25/type-29 target into a Jungle prototype
or room. Two active shops stock local object #80186, a clear potion:

| Active consumer | Reference |
| --- | --- |
| [shipy.shp shop #43179](../../../areas/shp/shipy.shp#L259) | Product #80186 at line 265 |
| [surfacekeeps.shp shop #120010](../../../areas/shp/surfacekeeps.shp#L274) | Product #80186 at line 276 |
| [Shared CHAOS support-consumable data](../../../src/account/chaos_eq_data.h#L577) | Three #80186 entries at lines 577–578 |

The two literal special assignments are local mobile #80123 and #80194 to
world_quest ([assignments](../../../src/specs/specs.assign.c#L762)). These are
local procedural hosts, not foreign prototype imports. No other exact compiled
Rift VNUM use was identified; unrelated integer 80000 constants are not room
references.

One inactive candidate has source-only shop consumers:
[nexus56.shp shop #83031](../../../areas/shp/nexus56.shp#L201) lists Jungle
object #80074, a delicate purple flower, at line 205;
[shop #83037](../../../areas/shp/nexus56.shp#L451) lists #80002, an ebonwood
mandolin, at line 456. Preserve both products on
[Nexus56 candidate row 79](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79).
They are not active shop stock. No other typed inactive ZON, SHP, or QST
consumer of Jungle mobile/object prototypes was found in this bounded source
scan.

## Title, identity, and tracked history

After color-token removal, whitespace normalization, and case folding, “Rift
Valley Jungle” occurs once among all 442 current ZON titles and once among the
351 active areas. The 470 room, 220 mobile, and 220 object VNUM definitions have
no same-type overlap with other active files. The normalized full description
comparison found no exact room-description match elsewhere; repeated titles
within the Jungle describe repeated forest/canopy templates and do not establish
replacement identity.

Follow-history for rftjngle.wld, mob, obj, zon, qst, and shp stays on the current
stem and reaches the 2009 area-tree move commit 642da5400. No alternate-stem
rename or distinct predecessor was found. The story sidecar was added by
348f3ee49 on 2026-10-04. The separately tracked five-description Moonhollow
route repair is already in the native history; this checkpoint makes no native
changes. Keep active rftjngle DO NOT MOVE.

## Builder questions and recommended resolutions

1. **What should active room #80460 south reach?** The target #228371 exists
   only in inactive Surf and Surface2011 sources with different room identities;
   only Surf has a north record back to #80460. Confirm the intended map owner
   and destination, then explicitly restore or retire the edge. Do not select a
   target by matching VNUM alone.
2. **Should Surf #226279 south to #80469 ever be an active approach?** Preserve
   it as source-only while the historical Surf map remains unregistered; the
   current Surface boundary is already reciprocal.
3. **Is the mountain-pass down exit from #80398 to Pine Hollow #16008 intended
   to be one-way?** Confirm the return and fall-risk policy before promising a
   safe round trip or adding a reverse record.
4. **Are the four #80444 exits to elemental planes player routes or ranger
   wander routes?** Keep mobile roaming separate from player access and define
   an intentional return policy before presenting these one-way exits as travel.
5. **Which of the 42 reset doors are progression gates?** Review both sides,
   key policy, state and clue together; the strict 385-room count does not prove
   permanent inaccessibility.
6. **Do all eight closed-secret doors have discoverable clues at both ends?**
   Preserve their paired reset states while confirming whether each hidden route
   is meant to be found, bypassed, or retired.
7. **Should locked and unlocked reset states align with the written access
   descriptions in the central cave and chieftain's quarters?** Confirm the key
   holders and reverse exits before changing only one side.
8. **Are the canopy climbs and paired tree doors intended to be the ordinary
   route to canopy sources?** Keep the actual climb/door graph and reset policy
   in player guidance; titles and prose alone do not grant access.
9. **Which of the 44 fall rooms expose required quest paths or source caches?**
   Qualify fall destinations, recovery and return before describing a safe
   collection route.
10. **Are the 27 current rooms meant to push actors, objects, or players along
    the same river paths?** Confirm current direction and movement effects in
    the live rules before advertising either bank as a reliable route.
11. **Should each of the four item-teleporter pairs remain command-specific and
    local to the Jungle's elemental cages?** Preserve the paired source targets;
    qualify object availability, ENTER/UP/DOWN command, plane movement and
    return before adding travel credit.
12. **Should the engineer's plan remain in Moonhollow guard quarters even though
    his clue says “somewhere in these caves”?** Keep the real source documented
    and decide whether to clarify the clue or relocate the source; do not create
    a duplicate parchment as a shortcut.
13. **How should the 15-feather cloak request fit the 14-root durable
    exchange limit?** Keep the current recipe guarded until a complete atomic
    batch is supported; do not award partial daily/story credit.
14. **When should the seven mixed coin-and-material crafts become eligible for
    credit?** Preserve the current recipes as services until wallet, item trees,
    reward and recipient departure can settle atomically.
15. **Should the couatl egg outcome distinguish personal source recovery from
    supplied matching eggs?** Keep the exact three-egg-plus-wrist-guard recipe;
    any personal recovery milestone needs source/instance evidence rather than
    possession alone.
16. **Should the chief and scout dialogue be revised to match their accepted
    tokens?** The chief accepts the amulet, the scout accepts the golden rod,
    and the bloody head is refused. Keep those exact proofs until the intended
    story or kill requirement is chosen.
17. **Is the woodcarver's reachable staff exchange the intended alternative to
    the crystal dragon?** Preserve the single grouped outcome and do not imply
    that an unspawned dragon appears or transforms.
18. **Should the four elemental amulet exchanges remain independent outcomes?**
    Keep each proof/reward pair distinct unless builders define a shared
    campaign episode, completion rule and safe return.
19. **Should Mistwalker's reunion narrative remain uncredited beyond the sword
    exchange and his departure?** Do not claim that a ghost is created, escorted,
    restored or made to understand him without a corresponding endpoint.
20. **Should the two bartender world-quest hosts receive any local story
    milestones?** Bind any future credit to accepted assignment, target/count,
    payout and abandonment events; keyword dialogue and teacher aliases are not
    completed procedural assignments.

The comprehensive story dossier remains the source for the 65 QST blocks, 28
native contracts, 12 story outcomes, 12 supply services, three excluded
referrals/refusals, item sources, actor roles and capability findings. No native
map, quest, reset, or code change is authorized by this audit.

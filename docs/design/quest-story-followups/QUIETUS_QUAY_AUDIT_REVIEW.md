# Quietus Quay: connectivity, typed dependencies, history, and builder decisions

**Checkpoint 333; coverage row 244.** This supplement extends the
[source-comprehensive Quietus dossier](../zone-stories/QUIETUS_QUAY.md) and
reconciles its current WLD graph, ZON door resets, inactive-WLD scan, typed
references, compiled numeric matches, title, and followed history. These
source records do not establish live access, service settlement, safe travel,
or played qualification.

## Active WLD graph and reset-aware arrival reach

The active contributor owns 95 rooms and 216 authored exits. The loader
resolves 214: 211 local edges and three exits to other active WLDs. The local
graph is one 95-room weak component. The remaining two exits originate in
unused room #1794 and target `-1`; the loader rejects them. Room #1794 has no
local reset. The WLD declares no fall markers or currents.

| Active cross-area boundary | Reciprocal record | Cross-area register |
| --- | --- | --- |
| Underdark #850045 up to Quietus #1700 | Quietus #1700 down to #850045 | [inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1783), [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L27) |
| Surface #636555 north to Quietus #1723 | Quietus #1723 south to #636555 | [inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1667), [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L28) |
| Surface #635755 south to Quietus #1737 | Quietus #1737 north to #635755 | [inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1664), [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L29) |
| Heavens #31 north to Quietus #1736 | Incoming-only; no reverse local edge | [inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L11) |

The four distinct active WLD arrival rooms have this source-only directed
reach. The ZON `D` commands below are applied by source room and direction;
closed, locked, and secret exits are excluded from the strict traversal.

| Arrival source | Quietus root | Raw reach | Strict post-reset reach |
| --- | ---: | ---: | ---: |
| Underdark #850045 | #1700 | 91/95 | 5/95 |
| Surface #636555 | #1723 | 91/95 | 60/95 |
| Heavens #31 | #1736 | 91/95 | 60/95 |
| Surface #635755 | #1737 | 91/95 | 60/95 |

Quietus has 38 local door resets: 35 set closed/unlocked state 1, one sets
closed/locked state 2, and two set closed/secret state 5. All 38 match
door-capable WLD exits. The loader sets door/pick capability while the zone
reset applies closed, locked, secret, and blocked bits; see
[`setup_dir`](../../../src/world/db.c#L1565) and the
[`D` reset handler](../../../src/world/db.c#L7857). The strict graph does not
model opening or picking doors, finding secrets, reset timing, or player
movement. It is not a safety or gameplay result.

All four roots' raw walks miss #1780, #1781, #1784, and #1794. The dossier
documents #1780/#1784 as special mobile load rooms, #1781 as a no-exit trap,
and #1794 as unused. Reset-loaded actors and object triggers are separate
from WLD player reach.

An all-95 inactive-WLD pass parsed 2,080,773 exits with no parse errors, no
room-number overlap with Quietus, and no exit into #1700–#1794. No source-only
approach or attic candidate is added.

## Typed dependencies and compiled references

### Local resets and shops importing other prototypes

The current Quietus ZON uses these externally owned objects; preserve their
prototypes and owner files if the reset or area paths are reviewed.

| Consumer source | Quietus-owned prototypes used | Evidence |
| --- | --- | --- |
| Alatorin ZON | 12 placements of #1703, #1705, #1712, #1722, #1723, and #1728 | [reset groups](../../../areas/zon/alatorin.zon#L1517), [additional groups](../../../areas/zon/alatorin.zon#L1567) |
| Heavens ZON | Three placements of #1721 | [reset rows](../../../areas/zon/heavens.zon#L206) |
| Surface Keeps ZON | One #1715 | [reset](../../../areas/zon/surfacekeeps.zon#L160) |
| Winterhaven ZON | One #1715 | [reset](../../../areas/zon/wh.zon#L801) |
| Surface Keeps SHP | Product #1715 | [shop product](../../../areas/shp/surfacekeeps.shp#L277) |
| Winterhaven SHP | Product #1715 | [shop product](../../../areas/shp/wh.shp#L144) |

No foreign ZON resets a Quietus mobile or room. Quietus also imports
externally owned items: `quietus.zon` places Verz1 object #3097 in room
#1741, and its shop/reset sources use Heavens objects #500, #412, #377,
#390, #391, and #364. These are typed prototype dependencies, not duplicate
Quietus definitions. The six Heavens products are listed by local shop
#1712; see [Quietus SHP](../../../areas/shp/quietus.shp#L37) and
[Quietus reset rows](../../../areas/zon/quietus.zon#L168).

### QST and story item ownership

Quietus QST uses four externally owned prototypes: Ceothia longsword #80808
as an alternate credential (`quietus.qst` lines 50–51), Sarmiz sword #9436
as Rodev proof (line 151), and Krimman objects #16450 and #16429 for the
cloak request and captain's briefing (lines 189 and 255). The story sidecar
also records #80808 and #9436; its Krimman item references #16450 and #16429
appear at [lines 338 and 419](../../../areas/story/quietus.story.json#L338).
The existing [Krimman audit](KRIMMAN_AUDIT_REVIEW.md#typed-dependencies-and-bounded-negative-checks)
records those same consumer-specific links.

The bounded scan found no external QST giver or item command that consumes a
Quietus mobile or item. No other story sidecar has an exact structured room,
mobile, or item VNUM reference to Quietus. No external object file duplicates
a Quietus room, mobile, or item VNUM. None of the scanned external type-25
teleport destinations targets a Quietus room.

### Other compiled and service routes

The compiled reference scan found these direct, typed Quietus dependencies:

| Consumer | Source reference | Meaning and limit |
| --- | --- | --- |
| Function assignment | [`specs.assign.c`](../../../src/specs/specs.assign.c#L2225) | Mobile #1709 receives `world_quest`; room #1719 receives `ship_shop_proc`; room #1736 receives the inn function at line 2312; room #1734 receives `crew_shop_proc` at line 2393. |
| Ship and crew tables | [`ship_variables.c`](../../../src/ships/ship_variables.c#L249) | Quietus port room #1734 is referenced in ship service and hiring rows, including the crew entry at line 425. |
| Chaos portal | [`chaos.c`](../../../src/combat/chaos.c#L32) | The alias `qq` resolves directly to Quietus room #1711 through the checked `chaos_port` path. This is code movement, not an authored WLD edge. |
| Ferry stop | [`ferryact.c`](../../../src/world/ferryact.c#L102) | The Wave Dancer lists Quietus room #1712 as a named stop. Arrival depends on the ferry path. |
| Bard starting kit | [`chaos_eq_data.h`](../../../src/account/chaos_eq_data.h#L450) | The enhanceable Bard kit references Quietus instruments #1734, #1736, #1737, and #1738; the common kit builder consumes these VNUMs. |
| Tharn arena table | [`constant.c`](../../../src/core/constant.c#L1702) | Tharn's `hometown_arena` range spans #1701–#1725. The active utility path requires `ROOM_ARENA` ([check](../../../src/core/utility.c#L5791)); none of those Quietus WLD headers sets that bit. Treat the numeric range as a dormant source collision, not a confirmed travel route, and ask its owner before changing either mapping. |

The remaining direct numeric matches are Unicode ranges, combat/ship tuning
values, a commented-out arena path, or unrelated constants. They do not add
typed room, mobile, or object dependencies.

## Title, duplicate identity, and followed history

After removing Duris color codes and normalizing whitespace and case, “The
Docks of Quietus Quay” occurs once among 442 current zone titles and once
among the 351 active contributors. Quietus has no duplicate room, mobile, or
object VNUM within its files or across the other loaded and source-only
prototype files. The only global active-WLD duplicate room VNUM remains
#51800 in Hell Two and Hell Three; it is unrelated to Quietus.

Followed `quietus.wld` history records the 2009 area-tree move and a 2010
description edit. `quietus.zon` history includes the 2009 tree move and rare
load change, 2010 crew-area descriptions, and 2015 quest-item flags. Native
QST history remains on the Quietus stem through the 2009 tree move and 2011
dagger update. The revision-2 story sidecar was added in 2026 to map the
current missions and services. The bounded paths and curated history show no
alternate-stem predecessor or explicit replacement. Keep current Quietus
**DO NOT MOVE**.

## Checkpoint 333 builder questions and recommended resolutions

These are source-backed questions for the zone owner. They preserve current
native contracts and service boundaries until an intended change is selected.

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| QQ01 | Should the narrated Darvanu-to-officers-to-finale route become a required progression chain? | Keep the four terminal missions independent; current QST does not require earlier receipts, learned topics, faction state, or a personal kill. |
| QQ02 | Are the five credential briefings supporting services, rather than five more missions? | Keep the sidecar service rows separate from the four mission accomplishments; do not infer that every briefing was completed. |
| QQ03 | Should both badge #1701 and Ceothian longsword #80808 remain accepted credentials despite badge-only prose? | Preserve both native alternatives and keep guidance explicit about the longsword path. |
| QQ04 | What should happen when a badge contains captain's key #1742 or other contents at turn-in? | Keep the current empty-badge warning until exact-tree tests establish a chosen rule; then decide whether to reject, preserve the original tree, or transfer contents safely. |
| QQ05 | Should proof items require personal recovery or a recorded kill? | Keep supplied items valid and separate possession/readiness from personal acquisition and encounter history. |
| QQ06 | Is the drow contract meant to require the note and Aresliean's head together? | Preserve the two-item AND and do not promote the note or head alone to completion; keep the supplied-item route. |
| QQ07 | Is Aresliean's shared, rare load at #1784 and the head's reset equipment chance intended? | Qualify normal load, wandering, trap occupancy, death, and reset behavior before changing placement or promising availability. |
| QQ08 | Should the Rodev sword lead imply that every king episode carries proof? | Keep the source's 50% equipment chance and accept a supplied sword; do not claim a guaranteed personal kill or guaranteed live inventory. |
| QQ09 | Should Eckraldu's staff be spendable at either Quietus or Krimman? | Preserve the shared #16450 prototype and its competing consumers; document which mission consumes it without duplicating the item or assigning both outcomes. |
| QQ10 | Is consuming Krimman's seal for a briefing with no returned item intentional? | Keep the current receipt/reward contract until the builder chooses a different payoff; do not manufacture a returned seal. |
| QQ11 | Should the bloodstone finale require prior credentials, lieutenant work, or the seal/dagger briefing? | Keep the final Q independent; its current input is Quietus bloodstone #1730 and the captain departure/rewards are its terminal outcome. |
| QQ12 | Should rank, coup success, retaliation, or a new pirate allegiance persist after the finale? | Keep those as authored narrative only until the builder defines an actual state, subject, transition, failure path, and replay rule. |
| QQ13 | Are the captain, Aresliean, and bat load locations and their trap routes intentional? | Preserve the rare loads and current cap/room structure until observed episodes establish a defect and the owner selects an availability policy. |
| QQ14 | Is reset-closed access from #1700 meant to reach only 5 rooms before opening the tunnel doors, compared with 60 from the other three entries? | Preserve all 38 current door resets and qualify the intended starting route, key use, secret discovery, and return before describing access as blocked or safe. |
| QQ15 | Are the Golden Lyre and bat-shrine hidden exits adequately hinted? | Preserve their source flags and clue prose; verify discovery from both sides before changing secret state or adding a completion milestone. |
| QQ16 | Are chests #1708 and #1726 meant to be open while their flags still include locked/pickproof bits? | Reproduce ordinary get/open behavior and choose the intended closed/key rule before changing flags or telling players a key is required. |
| QQ17 | Is the green granite fountain's alignment-dependent water effect intended as its full story outcome? | Keep liquid #28's current unpoisoned data and shared alignment behavior; add no cure or investigation reward without a selected rule. |
| QQ18 | Should the ferry and `chaos_port qq` arrivals count as part of ordinary Quietus progression? | Keep these code routes distinct from WLD entrance metrics and mission completions; retain the Wave Dancer #1712 stop and #1711 destination while their owners confirm intended access. |
| QQ19 | Should bartender world-quest fees and refunds be presented as usable under active accounting? | Keep the random world-quest namespace separate from local QSTs; settle wallet admission and recoverable refunds before advertising paid actions or awarding local story credit. |
| QQ20 | Should shipwright, crew, shop, and inn actions advance the coup story or remain port services? | Keep them outside mission credit; resolve their payment/ship settlement and inn save behavior through their owning service paths before using them as durable story events. |

This is a documentation-only checkpoint. No native source behavior, area
membership, candidate row, or active route was changed. No build, test, server,
database operation, migration, or gameplay session was run.

# Faerie Realm audit supplement

Checkpoint 339 reconciles active Faerie Realm against its existing
source-comprehensive [story dossier](../zone-stories/FAERIE_REALM.md) and
[source index](../../reference/zone-story-audits/realm.md): WLD arrival and
reset-door reach, inactive-source approaches, typed consumers, title and tracked
history, in-place retention, and direct builder recommendations. This is a
static repository review. It does not certify live travel, player safety,
installed configuration, or persisted player history, and it changes no native
world data.

## Active WLD graph and reset-door reach

The active contributor is listed in [areas/AREA line 78](../../../areas/AREA#L78)
and [areas/qst/AREA line 71](../../../areas/qst/AREA#L71). Its WLD has 211 rooms
and 468 source exits. The loader resolves 458: 435 local exits and 23 resolved
cross-area exits. Eight negative targets are dropped, and two positive targets
are unresolved. The raw local weak graph has components of 206, 3, 1, and 1
rooms.

The only active WLD arrival is reciprocal with Surface:

| Boundary | Direction and target | Reverse |
| --- | --- | --- |
| Surface #531694 ↔ Realm #14002 | Surface east; Realm west | Both records exist |

See [Surface's inbound record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1393),
[Realm's return record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L242), and the
paired [Surface-to-Realm](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L578) and
[Realm-to-Surface](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L400) summaries.

From that arrival at #14002, raw directed local WLD traversal reaches 196 of
211 rooms. Only 6 of 211 are reachable in a strict baseline after applying
Realm's 26 D resets and treating every closed, locked, secret, or blocked exit
as unavailable. The strict walk is a door-state snapshot: most closed,
unlocked doors can be opened, secret doors may be found, and the count does not
simulate player actions or installed procedures.

All 26 local D commands have no conditional predecessor and a 100% chance; no
room/direction is reset twice. Their states are sixteen state-1 closed,
unlocked doors, eight state-2 closed and locked doors, and two state-5 closed
and secret doors. The two state-5 records are the south exit from #14006 and
the north exit from #14008
([resets](../../../areas/zon/realm.zon#L8)). The native handler sets closed
and locked bits from the low two state bits and applies the secret and blocked
bits from the upper flags
([db.c](../../../src/world/db.c#L7857)); reset mode 2 runs regardless of
occupancy ([events.c](../../../src/world/events.c#L429)).

The only two positive unresolved edges leave different parts of the Realm:

| Realm source | Target | Current source-only owner | Disposition |
| --- | --- | --- | --- |
| #14090 east | #164929 | unregistered Duris3 WLD | The loaded build prunes this edge; preserve the source reference on Duris3 candidate row 36 |
| #14210 southeast | #25087 | unregistered brass-old-2 WLD | The loaded build prunes this edge; preserve the source reference on brass-old-2 candidate row 18 |

The target records are [Duris3 #164929](../../../areas/wld/Duris3.wld#L1363510)
and [brass-old-2 #25087](../../../areas/wld/brass-old-2.wld#L2078). These are not
active routes, and the coverage register's two unresolved-target records remain
unchanged. The eight negative exits remain separately listed in unresolved
exit-register rows [29–38](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L29); the loader
drops their negative VNUMs.

Of the 23 resolved outbound WLD records, Realm #14002 west returns to Surface.
The other 22 resolved edges leave dispersal rooms with no reciprocal WLD edge:

| Dispersal room | Resolved one-way destinations |
| --- | --- |
| #14208 | North to Astral Tiamat #19953; south to Vecna #130021; west to Juiblex #87628; up to Bahamut #25918; down to Bctdl #32469; northwest to Celestia #45718; southwest to Nyneth3 #38811; northeast to Hall #77867; southeast to Ravenloft #58369 |
| #14209 | North to Ethereal 2 #32707; south to TransTower #16271; up to Draconic Pit #15913; down to Earth #43568; northeast to Gibber #8788 |
| #14210 | North/east to Githzerai #44753/#44476; south to Four Horsemen #34804; up to Negative Plane #26852; down to Barovia 2 #58984; northwest to Dragonnia #6839; southwest to Snogres #87777; northeast to Future Ceothia #81677 |

The exact owner and reverse-edge records are in the
[cross-area exit register rows 243–264](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L243).
The three rooms are builder-named “NOREPAIR SMITHY DISPERSMENT” locations, not
ordinary WLD destinations reachable from the Realm's active arrival. Both
smiths are reset at #14209; their wandering and actual player encounters remain
unqualified ([resets](../../../areas/zon/realm.zon#L540)).

## Current inactive-WLD sweep and Surf candidate

The current checkout contains 446 WLD files under areas/wld: 351 active stems
and 95 non-active stems. The all-source inactive scan parsed the 95 non-active
files—527,791 rooms and 2,080,773 exits—with no parser issues. No source room
VNUM overlaps Realm. One non-overlapping source-only approach was found:
inactive surf.wld room #244463 south to Realm #14002
([source room and exit](../../../areas/wld/surf.wld#L932939)).

This source-only edge is not active ingress. Preserve it on existing
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). It was
absent from the prior curated list, so Surf's reviewed source-only subset rises
from 253 to 254 and the overall curated source-only total rises from 4,489 to
4,490: Surf 254, ud_map1 two, ud_map2 20, and Underworld2 eight.

## Typed and compiled dependencies

The exact active-zone reset scan finds ten reset records importing Realm-owned
object prototypes. The planes' component objects are intentional sources for
the two five-part forge services:

| Active consumer | Imported Realm object |
| --- | --- |
| Alatorin ZON lines 2875 and 4649 | #14097 earth staff; #14126 Fix scroll |
| Astral Main ZON line 43 | #14124 astral forge component |
| Brass ZON line 821 | #14030 waybread |
| Caertannad ZON line 415 | #14004 pile of bones |
| Nax ZON line 377 | #14029 herbs |
| Plane of Air / Earth / Fire / Water ZON | #14122 wind / #14123 hammer / #14121 heat / #14125 water basin |

The five component reset sources are documented individually in the
[story dossier](../zone-stories/FAERIE_REALM.md#five-distinct-planar-parts-and-alternate-fix-sources).
Alatorin's active QST accepts Realm Fix scroll #14126
([QST](../../../areas/qst/alatorin.qst#L7446)); its structured story
contract retains that separate Alatorin receipt
([sidecar](../../../areas/story/alatorin.story.json#L3955)). The same item
also appears in an Alatorin reset on Moradin, which is a distinct source and
does not replace the Realm's five-part recipe.

Three active shops list Realm-owned products: Alatorin sells #14097
([SHP](../../../areas/shp/alatorin.shp#L182)), Brass lists #14030
([SHP](../../../areas/shp/brass.shp#L584)), and Nax lists #14029
([SHP](../../../areas/shp/nax.shp#L295)). The local Fulbar shop is one actual
record with keeper/mobile #14004
([SHP](../../../areas/shp/realm.shp#L1)); the coverage row's former blank/zero
shop inventory was stale.

The shared CHAOS equipment data uses Realm belt #14045 in three item sets
([references](../../../src/account/chaos_eq_data.h#L134)). The exact compiled
VNUM scan found no other external literal for a Realm room, mobile, or object.
The four Realm mobile assignments remain Finn #14015, tree spirit #14026,
faerie #14029, and cricket #14048
([assignments](../../../src/specs/specs.assign.c#L980)). A separate
bridge_troll assignment to #14202 is not a loaded Realm mobile; its comment
belongs to Wilderness Near Verzanan, as the source dossier records.

Across all current OBJ sources, 872 item-type-25 teleport declarations and 380
item-type-29 switch declarations were reviewed for Realm room targets. The
only match is Realm orb #14048, whose teleport target is local approach room
#14002 ([prototype](../../../areas/obj/realm.obj#L648)). No external portal or
switch targets a Realm room. The two Realm reset placements of the orb are at
library room #14067 and heart room #14112; both share global object limit two
([resets](../../../areas/zon/realm.zon#L127) and
[second placement](../../../areas/zon/realm.zon#L153)).

The source-only typed consumers remain visible on their existing candidates:

- brass-old-1's inactive ZON and SHP use Realm waybread #14030 (candidate row 17).
- nexus56's inactive ZON and SHP use Realm herbs #14029 (candidate row 79).
- sylvandawn's inactive ZON references Realm sword #14001 (candidate row 101).
- player_guilds' inactive SHP lists Realm identify scroll #14112 (candidate row 84).

These are retained source-only dependencies, not active reset or shop stock.
The exact scan of other active QST sources found no additional Realm item
exchange; Forgotten Forest's numeric 14000 is an XP reward field, not a room
reference.

The local reset at
[realm.zon line 174](../../../areas/zon/realm.zon#L174) requests absent object
#141120 at room #14203. Object #14120 exists, but the extra digit is not
authorization to substitute it. The active loader disables the missing object
reset. Preserve this discrepancy until the intended equipment and placement
are confirmed.

## Title, identity, and tracked history

After color-token removal, whitespace normalization, and case folding,
“Faerie Realm” occurs once among all 442 current ZON titles and once among
the 351 active areas. Its 211 room, 74 mobile, and 123 object VNUMs have no
duplicate current WLD, MOB, or OBJ definitions. Fifteen normalized generic
room-title matches occur elsewhere, but no complete room description matches;
these labels do not establish duplicate identity or replacement.

Git follow-history for realm.wld, realm.mob, realm.obj, realm.zon, realm.qst,
and realm.shp stays on the current realm stem and reaches the 2009 area-tree
move commit 642da5400; no alternate-stem rename or distinct predecessor was
found. The story sidecar was added at 7319e78eb on 2026-10-03. The other
current titles with “realm” or “realms” in their wording have separate active
keys and title sources:

| Current title | Active contributor and roster range |
| --- | --- |
| [The Realm of Barovia](../../../areas/zon/barovia.zon#L2) | `barovia` (*910–911) |
| [The Realm of Barovia Continued](../../../areas/zon/barovia2.zon#L2) | `barovia2` (*588–589) |
| [The Great Realm of Duris](../../../areas/zon/connectorzones.zon#L2) | `connectorzones` (*536–543) |
| [Drifting Realm](../../../areas/zon/dream.zon#L2) | `dream` (*313) |
| [The Surface Realm of Duris](../../../areas/zon/surface.zon#L2) | `surface` (*5000–6600) |
| [The Dark Stone Tower of the Northern Realms](../../../areas/zon/teka2.zon#L2) | `teka2` (*755) |
| [Thetis's Realm](../../../areas/zon/thetis.zon#L2) | `thetis` (*380) |
| [The Lost Realms of Shadowfall](../../../areas/zon/wolfen.zon#L2) | `wolfen` (*180–182) |

`barovia2` is explicitly titled as a continuation and remains a separate
listed contributor from `barovia`; neither title identifies a Faerie Realm
replacement. The other shared “realm” wording identifies different current
areas and titles. Keep the active `realm` contributor DO NOT MOVE.

## Builder questions and recommended resolutions

1. **Should room #14000 remain the Seelie Court's “beginning room” when the only active arrival targets #14002 and raw directed reach from #14002 does not include #14000?** Choose an intended entry or correct the room's role; do not present the disconnected description as an active starting route.
2. **Are the paired #14006 south and #14008 north state-5 exits intended as secret doors from the forest approach?** Keep both reset states paired and make the discovery clue explicit if this is the intended way through.
3. **Which of the 26 reset doors are meant to gate progression, and which are ordinary closed doors?** Review their keys, search clues, reset state, and reverse side together; the six-room strict baseline is not a claim of permanent inaccessibility.
4. **Should the cottage's closed/reset thresholds keep their current paired access and key policy?** Confirm the #14061–#14066, cellar, and kitchen doors together before changing only one side.
5. **Is the golden gate at #14081 intentionally unfinished, with key -2 and no destination?** Select a real riddle, destination, open/locked state, and return policy, or retire the passage promise; do not guess a VNUM.
6. **Should the cellar, no-magic roots, and floorless shaft remain the personal route to the heart?** Qualify the white-key use, falling behavior, tree-spirit presence, and orb return before describing this as a safe route.
7. **Is the #14110-to-#14111 mud-pit passage meant to be one-way?** If return is intended, choose its reciprocal direction and door policy; otherwise state the one-way risk clearly.
8. **Should the garden's “peace” gate and the tomb's key/“blah” reverse door retain different opening rules?** Choose the accepted word/key and reciprocal policy on both sides rather than silently normalizing one exit.
9. **Are the eight negative-target exits intentional barriers, text-only directions, or unfinished routes?** Preserve them as non-routes until each target and reciprocal behavior is identified.
10. **Should cottage east from #14090 enter the Duris3-only room #164929?** Restore it only with an owner-approved loaded destination; otherwise retire the edge explicitly and keep candidate row 36's source evidence.
11. **Should #14210 southeast enter brass-old-2's room #25087?** Treat it as unresolved until the destination owner confirms restoration or retirement; retain candidate row 18.
12. **Are all nine #14208 smithy-dispersal exits intended as player routes to their listed areas?** Decide destination admission, return, and danger policies individually; they currently have no reciprocal Realm-side WLD edges.
13. **Are the five #14209 one-way destinations intentional for the two resetting smiths?** Keep mobile roaming and player access separate, and validate any intended encounter without inventing a return edge.
14. **Are the eight resolved #14210 one-way destinations intended to remain in one dispersal room?** Confirm why the Githzerai destinations use two exits and whether each destination remains safe and supported.
15. **Should the Nyneth3 treasure-room and Astral Tiamat vault exits remain one-way from the no-repair smithy network?** Keep their current source identities until owners decide the expected arrival and return path.
16. **Should both smiths continue to reset at #14209 and wander from that isolated local component?** Qualify source room, reset placement, movement, and actual service encounter before making the forge depend on finding them.
17. **Should the tree spirit's dormant helper-combat behavior be enabled?** Its current callback rejects the modern combat event; choose a supported dispatch, per-appearance state, cadence, and cap before re-enabling, and do not add a helper kill as a quest gate.
18. **What equipment does reset command #141120 at #14203 intend, and should the stale bridge_troll binding be removed or corrected elsewhere?** Confirm each owner first; do not substitute #14120 or treat #14202 as a Realm mobile.
19. **Should the five independent forge components, 5,000,000-coin recipes, and two smith recipients remain the exact repair service?** Preserve atomic settlement and the existing Alatorin-owned #14126 receipt as a separate source until its contract is intentionally migrated.
20. **Should all currently active cross-area reset, shop, QST, and CHAOS consumers of Realm prototypes remain supported?** Preserve their owning areas and current VNUMs until each consumer has an explicit replacement or retirement decision.

The existing dossier remains the source for the seven local exchanges, story
sidecar outcomes, NPC procedures, keys, environmental warnings, and narrative
questions. No native map, quest, reset, or code change is authorized by this
audit.

## Checkpoint 779: Caertannad typed dependency

Active Caer Tannad has one object reset row for Faerie Realm object #14004. See [Caertannad typed audit](CAERTANNAD_AUDIT_REVIEW.md#outbound-active-reset-references).

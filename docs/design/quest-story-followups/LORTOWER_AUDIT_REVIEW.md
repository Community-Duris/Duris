# The Tower of Darkness (lortower): world and owner review

**Scope:** checkpoint 268 adds directed connectivity, access boundaries, typed
dependencies, title/history, attic disposition, and builder-owner reconciliation
to the existing [source-comprehensive Tower dossier](../zone-stories/TOWER_OF_DARKNESS.md)
and [native source index](../../reference/zone-story-audits/lortower.md). This is
a source review; active player entry, reset admission, travel, survival, renewal,
and story outcomes remain unqualified.

## Existing story and environmental coverage

The active contributor is lortower, zone header 1340, The Tower of Darkness.
Its WLD contains 142 rooms #134000–#134141; its source files define 170 mobiles,
146 objects, one shop, 32 QST blocks, and 580 reset commands. The native QST
contains twelve active contracts: seven whose giver VNUMs belong to Tower and
five physically placed in Tower but credited to Braddistock. The revision-1
story sidecar maps six Tower stories, 27 contacts, and twelve optional
preparation checks. The existing dossier reviews all six stories, all seven
Tower-owned exchanges, the five Braddistock-owned physical requests, room and
item identities, exact recipes, access keys, passwords, teleport objects,
hazards, and limits on credit. It remains the detailed source of record for
exact story behavior.

The local shop is also present: shop record #134161 is kept by Fenrral and
offers objects #134128–#134130. Coverage row 166 previously showed no shop;
this review corrects that inventory mismatch.

## Directed WLD graph and entry boundary

The 142 rooms have 500/500 exits resolving to active rooms: 499 are local and
one leaves Tower from #134000 east to Troll Hills #1883
([cross-area row 1307](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1307)). The local
undirected graph has four weak components of 91, 32, 12, and 7 rooms. Its
directed strongly connected components have sizes 75, 32, 12, 7, 5, 2, 2, and
seven single rooms. Raw exit traversal from #134000 reaches 75 rooms in the
largest 91-room weak component. This raw graph treats closed or locked exits
as available and excludes directional ITEM_TELEPORT objects.

There is no active WLD exit into the Tower room range, and the source dossier's
bounded scan found no foreign fixed ITEM_TELEPORT destination into these rooms.
The Troll Hills exit from #134000 reaches #1883, whose west exit reaches #1882;
no active exit returns from Troll Hills into Tower. Dynamic, random, event, and
staff transport are not qualified. The visible route text, active area
membership, and internal teleport maze do not establish an ordinary player
entry path or journal discovery.

The existing dossier maps directional teleport objects and magic-door
interactions that connect story routes outside the raw WLD graph
([travel and door analysis](../zone-stories/TOWER_OF_DARKNESS.md#actual-dialogue-magic-doors-and-travel)).
Those authored mechanisms remain distinct from room exits. Loaded lock state,
password acceptance, opening, portal use, arrival, falling, survival, and
return are not proven by static graph traversal.

See [WORLD_CONNECTIVITY.md](WORLD_CONNECTIVITY.md#checkpoint-268-the-tower-of-darkness-raw-exits-and-inactive-duris3-approaches),
[coverage row 166](ZONE_COVERAGE_REGISTER.csv#L166), and the exact
[cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1307).

## Typed dependencies and shop inventory

All local typed reset targets resolve to lortower rooms, mobiles, and objects.
The 580 reset commands are 176 M, 116 E, 114 D, 74 O, 50 F, 32 G, and 18 P
commands. The bounded active-source scan found no foreign typed ZON reset, QST
giver/item command, or SHP product reference to a Tower room, mobile, or object.

The active foreign Braddistock story sidecar is a real typed dependency. It
maps twelve Tower mobiles as contacts and includes the five physically local,
Braddistock-owned QST contracts for Azlion, Jenifer, Father Darrin, Danthas,
and Isabia ([Braddistock story](../../../areas/story/brad.story.json#L175)).
The local lortower sidecar also maps these five physical requests. Keep their
native completion owner and Braddistock receipt identity intact; this
cross-zone presentation relationship does not itself make Braddistock
discoverable from Tower or move those receipts into Tower.

The local shop is shop/keeper #134161, Fenrral, with product objects #134128,
#134129, and #134130 ([shop record](../../../areas/shp/lortower.shp#L1),
[keeper](../../../areas/mob/lortower.mob#L2465),
[products](../../../areas/obj/lortower.obj#L2218)). This is ordinary food and
drink stock, not a quest or rental service.

Two compiled generic object searches use 134000 as an inclusive VNUM ceiling:
the random container reward search and the enhancement index. Tower object
#134000, the standing stone, falls on that numeric boundary
([prototype flags](../../../areas/obj/lortower.obj#L6),
[reward search](../../../src/cmd/actmove.c#L2611),
[enhancement ceiling](../../../src/item/enhance.c#L1328)). Its source flags
include ITEM_NOSHOW ([flag definition](../../../src/core/defines.h#L202)); both
searches filter that flag ([reward filter](../../../src/cmd/actmove.c#L2622),
[enhancement filter](../../../src/item/enhance.c#L1985)), so the boundary does
not establish an eligible Tower reward today. Preserve that exclusion if the
prototype or shared search rules change. No literal Tower special assignment,
teacher, or trigger establishes another local story endpoint.

## Title, history, and attic disposition

After stripping color codes and normalizing case and whitespace, The Tower of
Darkness is unique among 351 active .zon contributors. Followed source history
begins with the Tower's 2010 addition, stays on the lortower source stem, and
contains no title rename, separate predecessor, or whole-area replacement.
The 2014 shop cleanup and the October 2026 direction/password repair do not
establish replacement history. No tracked lortower .caplor sidecar was found.

Inactive Duris3.wld has a differently named source record for every Tower room
VNUM #134000–#134141. None of its 142 normalized room titles matches the active
Tower title at that VNUM; examples include active #134000 An Old Path versus
Duris3 #134000 Mountains, and active #134141 Random Mob Placement Chamber
versus Duris3 #134141 The Ocean. These are separate numeric owners, not
duplicate rooms or evidence that Duris3 replaced Tower.

The all-95 inactive-WLD scan found 286 Duris3 exits from non-overlapping source
rooms into the active Tower range: 142 south exits from #133700–#133841, two
endpoints (#133999 east to #134000 and #134142 west to #134141), and 142 north
exits from #134300–#134441. Another 282 Duris3 exit records into the same
target range start in same-numbered Duris3 rooms and are excluded from the
non-overlapping approach count. None are active ingress. Keep active lortower
DO NOT MOVE and preserve Duris3 separately on existing candidate row 36; do
not co-load or reassign those room identities. The curated source-only total
rises by 286 to 3,764.

See [DUPLICATE_AND_REPLACED_ZONES.md](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-268-the-tower-of-darkness-title-history-and-duris3-identity),
[ATTIC_MIGRATION_PLAN.md](ATTIC_MIGRATION_PLAN.md#checkpoint-268-the-tower-of-darkness-retention-and-duris3-approaches),
[Duris3 approach example](../../../areas/wld/Duris3.wld#L707713),
[Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), and
[coverage row 166](ZONE_COVERAGE_REGISTER.csv#L166).

## Builder and owner questions

These direct questions record decisions to resolve before changing content,
credit, access, or shared item selection. The recommendations do not authorize
a native gameplay change.

| # | Question | Source evidence | Recommended resolution |
| ---: | --- | --- | --- |
| 1 | Which supported player route enters Tower and records Tower discovery? | [No active inbound route](../zone-stories/TOWER_OF_DARKNESS.md#reviewed-evidence), [entry room](../../../areas/wld/lortower.wld#L1) | Identify the existing dynamic/event route or approve a fully mapped entrance and return path before assigning ordinary entry credit. |
| 2 | Is the east exit from #134000 to Troll Hills #1883 meant to be one-way from Tower? | [Tower exit](../../../areas/wld/lortower.wld#L16), [cross-area row 1307](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1307), [Troll Hills return direction](../../../areas/wld/troll_hills.wld#L2501) | Confirm intended return routing and travel boundary; do not add a reciprocal exit from prose alone. |
| 3 | Are the four raw WLD components meant to connect through the authored teleporter objects and magic words? | [component and portal analysis](../zone-stories/TOWER_OF_DARKNESS.md#actual-dialogue-magic-doors-and-travel) | Keep ordinary exits, passwords, and ITEM_TELEPORT transitions distinct; approve any topology change only after choosing the intended route. |
| 4 | Is the Bloodstone traveller's non-sentinel movement through the road and disconnected seven-room component the intended letter source? | [traveller and letter](../zone-stories/TOWER_OF_DARKNESS.md#sources-quantities-and-competing-keys), [traveller prototype](../../../areas/mob/lortower.mob#L995) | Qualify actual birth, roaming, perception, and recovery before promising a reliable messenger supply. |
| 5 | Are the random-placement chamber and Danthas's loading/holding destinations intended, including dead-end room #134120? | [Danthas placement review](../zone-stories/TOWER_OF_DARKNESS.md#star-stone-prisoners-and-ownership-boundary), [Danthas prototype](../../../areas/mob/lortower.mob#L2557) | Confirm the actual distribution and allowed roaming rules; do not move or promise an encounter before that decision. |
| 6 | Should reset mode zero remain the physical Tower policy for the quest recipients? | [reset header](../../../areas/zon/lortower.zon#L3), [repeatability review](../zone-stories/TOWER_OF_DARKNESS.md#exact-exchanges-and-progression-stories) | Decide actor episodes and renewal independently of credit metadata before describing any exchange as daily or renewable. |
| 7 | Should the two elite stone giants share the note and cash alternatives while a D1 exchange retires only its selected instance? | [giant exchange](../../../areas/qst/lortower.qst#L45), [reset and actor analysis](../zone-stories/TOWER_OF_DARKNESS.md#exact-exchanges-and-progression-stories) | Preserve the current per-instance receipt and clarify intended population and replacement rules before changing caps or retirement. |
| 8 | Should the 100-platinum branch stay unavailable while durable currency settlement is unsupported? | [cash offering](../../../areas/qst/lortower.qst#L65), [guarded fee analysis](../zone-stories/TOWER_OF_DARKNESS.md#blockers-balanced-repair-proposals-and-acceptance) | Keep the branch guarded until item/currency allocation, rollback, replay, and recovery are qualified. |
| 9 | Is Katalia's accepted key exchange meant to be the full native endpoint even though her response describes release? | [Katalia contract](../../../areas/qst/lortower.qst#L1), [story limit](../zone-stories/TOWER_OF_DARKNESS.md#exact-exchanges-and-progression-stories) | Credit the accepted exchange only; add an escort, release, or world-state event only after defining it. |
| 10 | Should Amelia's key-for-locket exchange have a separate homecoming or reunion endpoint? | [Amelia contract](../../../areas/qst/lortower.qst#L135), [story limit](../zone-stories/TOWER_OF_DARKNESS.md#star-stone-prisoners-and-ownership-boundary) | Keep the current receipt separate from her narrated departure until an accepted return event is selected. |
| 11 | Should Azlion's accepted staff exchange create a real rift, or remain a completed Braddistock request with rift narration? | [Azlion contract](../../../areas/qst/lortower.qst#L173), [physical-owner review](../zone-stories/TOWER_OF_DARKNESS.md#star-stone-prisoners-and-ownership-boundary) | Preserve its Braddistock owner and current reward; do not claim a portal or release route without a concrete transition. |
| 12 | Does Father Darrin's Star Key delivery end at the accepted exchange, or should it record Sargon's defeat and return as later events? | [Darrin contract](../../../areas/qst/lortower.qst#L229), [Star Stone route](../zone-stories/TOWER_OF_DARKNESS.md#star-stone-prisoners-and-ownership-boundary) | Keep Star Key delivery, passage, survival, Sargon defeat, and return as separate facts until the campaign endpoint is defined. |
| 13 | How should the five Braddistock-owned local requests appear to a player who discovers only Tower? | [Braddistock owner mapping](../../../areas/story/brad.story.json#L175), [runtime owner boundary](../zone-stories/TOWER_OF_DARKNESS.md#star-stone-prisoners-and-ownership-boundary) | Keep immutable receipts in Braddistock and define an explicit cross-zone referral/discovery policy before adding credit. |
| 14 | Should matching supplied items remain sufficient, without requiring this character's personal kill or original recovery? | [accepted recipe behavior](../zone-stories/TOWER_OF_DARKNESS.md#exact-exchanges-and-progression-stories), [native contract examples](../../../areas/qst/lortower.qst#L217) | Preserve current exact-item acceptance; add source lineage only as a separate approved requirement with replay and handoff rules. |
| 15 | Should accepted passwords, lock clearing, opening, portal use, and surviving arrival remain separate uncredited events? | [magic-door and travel rules](../zone-stories/TOWER_OF_DARKNESS.md#actual-dialogue-magic-doors-and-travel) | Keep the stages separate; add event credit only for a selected successful transition and actor/target context. |
| 16 | How should the Cardinal's one bone key be allocated between Isabia's exchange and the locked chest child? | [bone-key source and competing use](../zone-stories/TOWER_OF_DARKNESS.md#sources-quantities-and-competing-keys), [Isabia contract](../../../areas/qst/lortower.qst#L298) | Preserve one physical item identity and qualify both reset/source paths before describing both uses as guaranteed. |
| 17 | Is Fenrral's shop #134161 intended to offer only the three current local food/drink prototypes? | [shop record](../../../areas/shp/lortower.shp#L1), [keeper](../../../areas/mob/lortower.mob#L2465) | Keep the one-shop coverage count and three product identities; confirm stock/keeper availability before any service claim. |
| 18 | Are the copied ale labels and the hammer's literal Testing messages intentional? | [ale prototypes](../../../areas/obj/lortower.obj#L2230), [hammer repair review](../zone-stories/TOWER_OF_DARKNESS.md#blockers-balanced-repair-proposals-and-acceptance) | Select the intended maker/name and weapon messaging before a text-only repair; preserve object behavior. |
| 19 | Should standing-stone object #134000 remain excluded from generic object pools whose inclusive search ceiling is 134000? | [standing-stone flags](../../../areas/obj/lortower.obj#L6), [random reward filter](../../../src/cmd/actmove.c#L2622), [enhancement index filter](../../../src/item/enhance.c#L1985) | Preserve ITEM_NOSHOW while those broad searches include the boundary; recheck both shared pools if its flags change. |
| 20 | Should the 142 Duris3 same-numbered rooms and 286 inactive approach exits remain separate from active Tower ownership? | [active room](../../../areas/wld/lortower.wld#L1), [Duris3 room](../../../areas/wld/Duris3.wld#L714001), [candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36) | Keep both source owners separate and Duris3 inactive until an owner-approved identity map resolves the room and loader boundary. |


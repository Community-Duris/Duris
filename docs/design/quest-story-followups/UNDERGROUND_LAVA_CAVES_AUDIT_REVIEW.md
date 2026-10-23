# The Underground Lava Caves audit supplement

**Checkpoint 49 — active and source-only connectivity, exact typed and code
references, title, and tracked-path history.** The existing
[comprehensive source map](../zone-stories/THE_UNDERGROUND_LAVA_CAVES.md)
remains the authority for local story, quest, room, mobile, object, reset, key,
and access details. This supplement records bounded cross-area evidence
without changing native sources.

## Active WLD boundary

All 451 authored Lava Caves exits resolve across its 142 rooms, which form one
weak component in the static authored-exit graph. The only active cross-area
connection is one reciprocal Mountain Tracts pair: Mountain room #21148 exits
south (`D2`) to Lava Caves room #35501, and #35501 returns north (`D0`) to
#21148. The exact records are in the
[cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L356) and
[return row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L469). Directional pairing and
static connectivity do not establish door state, traversal safety, or actual
player reachability.

## Unregistered WLD review

A stateful parse of all 95 current unregistered WLD files read 2,080,773
authored exits with zero parse issues. None of the 527,791 unregistered source
rooms reuses a Lava Caves room VNUM, and no unregistered exit target matches a
Lava Caves room. No source-only ingress candidate was found. The curated
source-only follow-up total therefore remains 586; the individually reviewed
`surf.wld` subset remains 26.

## Bounded typed and code references

Exact external QST scans found no room, mobile, or object VNUM references to
Lava Caves prototypes. A structured scan of the other story sidecars found no
exact numeric value or decimal-only string equal to a local room, mobile, or
object VNUM. The generic text scan's apparent `83291` match in
`alatorin.story.json` is only part of a generated exchange ID, not a VNUM.

The one external active reset reference is in Ravenloft2. Its reset group puts
Lava Caves oil lamp object #35589 inside Ravenloft2 slab object #59045 in room
#59269 ([reset lines](../../../areas/zon/ravenloft2.zon#L998)). The object
prototype is local to Lava Caves
([object source](../../../areas/obj/lavcav.obj#L418)); the foreign reset is
already active source data, but this static finding does not qualify the
container's runtime availability.

An unregistered source file, `areas/shp/player_guilds.shp`, has one typed
reference into Lava Caves: its shop record stores room #35552 as its location
([shop record](../../../areas/shp/player_guilds.shp#L567),
[room field](../../../areas/shp/player_guilds.shp#L618)). The record's `#35554`
header is an SHP record label; its keeper field #35554 is a mobile lookup and
has no matching mobile prototype. Its produced-object list includes #35551,
but no object prototype #35551 exists in the current source set; that number
matches Lava Caves room #35551 only, so it is not an object edge into this
area. The only Lava Caves prototype reference in the shop record is therefore
the typed room location #35552.

The normal shop loader first opens `areas/world.shp`; if that file is absent,
its fallback concatenates every `areas/shp/*.shp` file, including this
unregistered source ([fallback path](../../../src/economy/shop.c#L3276)). The
loader resolves the keeper as a mobile and reads the room field separately
([SHP fields](../../../src/economy/shop.c#L3489)). This establishes potential
fallback parser exposure, not an operating shop: the keeper does not resolve,
and no runtime was run. Keep `player_guilds` in place pending its complete
typed, source-owner, fallback, and tooling closure.

The bounded exact C/C++ literal scan found only #35523, the Dark Knight value
in the `highdrop_mobs` table ([table entry](../../../src/item/randomeq.c#L119)).
The selector loop is commented out in this source snapshot
([commented loop](../../../src/item/randomeq.c#L649)); the literal does not
establish a live horn-drop mechanic. The comprehensive dossier separately
records no local special binding and no active type-25 destination into Lava
Caves rooms.

## Active title and tracked history

Color-stripping and whitespace normalization across all 351 active `.zon`
titles yields one “The Underground Lava Caves” title, owned by `lavcav`
([manifest entry](../../../areas/AREA#L171),
[zone title](../../../areas/zon/lavcav.zon#L2)). The tracked WLD and ZON paths
reach the August 2009 area-tree relocation. The 2013 commit changes file
mode only; the 2015 ZON change updates only the editor-version comment. No
distinct predecessor or replacement appears in the reviewed path history.
Broader semantic alias and map-identity review remains pending.

## Scope and disposition

Lava Caves remains an active `areas/AREA` contributor. Its one reciprocal
Mountain Tracts pair stays in the active ledger, and the all-95 WLD scan found
no overlapping source-room VNUMs or target matches. The Ravenloft2 reset and
`player_guilds.shp` location are bounded exact references; dynamic access,
reset state, safe traversal, shop operation, and persisted-player outcomes
remain unverified. The candidate remains in place; no migration or gameplay
decision follows from this review.

This documentation-only review used source and static registers. It did not
run tests, a build, server/runtime/gameplay, database work, a migration, or an
operational script. At checkpoint 49, the source-comprehensive audit queue
stood at **89/220 complete, 131 pending**; priority 90, the Cimmerian Nomad
Encampment (`nomads`), was next.

## Checkpoint 259: directed WLD reachability

The active WLD contains 142 rooms and 451 authored exits. All exits resolve: 450 remain within the contributor, and #35501 north to Mountain Tracks #21148 is the sole outbound edge and its reciprocal arrival. The local undirected graph is one weak component.

In the raw directed local-WLD graph, 140 of 142 rooms are reachable from entrance #35501. There is a directed return path to #35501 from 141 rooms. Room #35635, the adventurer's spawn room, and room #35634, reached from its east side, are outside the entrance's forward-reachable set. Room #35634 has no authored exit and no local return path to #35501. The strongly connected components have sizes 140, 1, and 1. The source dossier separately records the cap-one adventurer's wandering and the trap-like room pair; mobile movement does not create an ordinary player route.

The active reciprocal pair is recorded at [cross-area rows 356 and 469](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L356) and [pair summary row 247](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L247). All 95 unregistered WLD files have no Lava Caves target or room overlap; no source-only ingress candidate is added. This raw graph does not model closed, locked, secret, pickable, blocked, or switch-controlled routes, nor does it qualify current doors, mobile location, or safe travel.

## Checkpoint 259: title and related-area distinction

The normalized active title remains uniquely owned by lavcav among 351 active zone headers. Current review now also compares the separate active lava_tubes contributor, zone 120 titled “Lava Tubes,” which has its own stem, room range, and source dossier. The similar volcanic setting and wording do not show a shared map, predecessor, or replacement. The followed lavcav WLD/ZON history remains on the same stem through the 2009 areas-tree relocation, 2013 file-mode change, and 2015 ZON-comment update. See the [title/history register](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-259-underground-lava-caves-title-history-and-lava-tubes-distinction) and [Lava Tubes supplement](LAVA_TUBES_AUDIT_REVIEW.md#title-and-bounded-source-history).

## Builder questions and recommended resolutions

These questions ask builders to resolve the open story, balance, access, and source-ownership decisions identified by the existing comprehensive dossier. Recommendations preserve current source behavior and item ownership until the responsible owner confirms an intended change.

| # | Direct builder question | Source-backed recommended resolution |
| --- | --- | --- |
| 1 | Is lavcav intended to remain a separate active contributor from zone 120, Lava Tubes, despite the related volcanic theme? | Keep the distinct active stems, titles, room ranges, and source histories; do not merge or rename either contributor from thematic resemblance. |
| 2 | Should the gnome prisoner's QST line that he faints produce a posture or encounter-state change? | Preserve the current dialogue and native posture/reset values until builders choose a truthful caption or a separately specified behavior. |
| 3 | Is the adventurer's coin-only offer intended to be the sole source of the platinum horns when no reset or ordinary loot producer exists? | Keep the horns scarce and the existing accounting refusal intact; identify an admitted, owner-approved producer or retain the current limitation rather than adding free stock. |
| 4 | Which price should the adventurer quote: 1,500 platinum in dialogue or the configured 100,000 base units, which currency formatting presents as 100 platinum? | Confirm the intended balance with the quest and economy owners; do not alter dialogue or price until one value is authoritative. |
| 5 | Is the lieutenant's exact hand-in—two matching horns #35515 for chain #35525 and 50,000 experience—the intended family-symbol resolution? | Preserve the exact native exchange and its item consumption while the owner confirms the reward text and amount. |
| 6 | Should the lieutenant's repeatable return count as a reliable daily opportunity while its only coded horn producer is the refused coin purchase? | Keep repeatability, daily candidacy, and actual producer availability separate; do not advertise reliable renewal without a qualified source. |
| 7 | Is the adventurer's disconnected spawn in #35635, with east leading to exitless #35634, an intentional trap design? | Preserve the two-room topology and mark both rooms outside entrance reach; do not add ordinary player exits by assumption. |
| 8 | Should the cap-one adventurer wander between the isolated room and lake-side routes, potentially hiding or trapping the seller? | Retain the authored STAY_ZONE and wandering settings; describe availability as uncertain and qualify any location guarantee separately. |
| 9 | Does the prisoner's Lake of Fire search clue intentionally point to no local reset or ordinary drop for the horns? | Preserve the clue without inventing a physical producer; builders should name the exact source and owner if the text is meant to be actionable. |
| 10 | Is the Dark Prison entrance meant to remain pickable as well as key-gated, with a cap-one iron key from the gatekeeper? | Keep the current key and picking rules as optional access; do not make killing the gatekeeper a personal quest prerequisite. |
| 11 | Is the Training Arena gate intentionally reset open even though its exit is keyed and pickproof, and what purpose should the rare worn onyx key serve? | Preserve open resets and the exact key prototype/chance until builders confirm the unusual type and values; make no speculative item repair. |
| 12 | Does pushing the fixed cracked wall at #35537 intentionally clear only the blocked flags, leaving the closed door for ordinary reveal/open handling? | Keep the switch and door operations separate; document any additional route action only after confirming the intended command sequence. |
| 13 | Are the closed secret routes to the instructor's barracks and keep discovery-only, or should the Dark Knight encounter change access? | Keep current secrets and reset states; do not create a Dark Knight kill gate without a native source rule. |
| 14 | Should the Lake of Fire rooms be described as survivable only with protection when compiled fire-sector effects can strip protection, damage, or kill? | Preserve the fire-plane flags and dispatch; avoid promising protection or safe passage without live qualification. |
| 15 | Are the instructor, manufacturer, library, prison, and mine services in room/NPC prose meant as lore only, since there is no local shop or matching service binding? | Keep unsupported service claims descriptive until builders name an implemented owner and exact transaction; do not invent training, crafting, mining, or rescue endpoints. |
| 16 | Is Dark Knight #35523 meant to drop horns because it appears in the high-drop table, even though the selector loop is commented out? | Treat the table literal as inactive; do not enable a drop or establish a horn source without a separate reviewed design. |
| 17 | Is the unregistered player_guilds SHP location #35552 intentional, despite its missing keeper mobile #35554 and product object #35551 and the loader's fallback scan? | Keep player_guilds RETAIN IN PLACE on candidate row 84; resolve source owner, exact prototypes, and fallback exposure before any move or operating-stock claim. |
| 18 | Is Ravenloft2's reset placing Lava Caves lamp #35589 inside slab #59045 at room #59269 an intentional cross-area fixture? | Preserve the typed reset and current prototype owner until both zone owners decide otherwise; do not treat it as a Lava Caves quest reward. |
| 19 | Is the single reciprocal Mountain Tracks boundary at #35501 intended as the only ordinary WLD entrance and exit for this contributor? | Keep the active pair and the negative all-95 source-only scan; record any proposed route with its owner before changing the map. |
| 20 | Should the source-native misspelling “tresure” remain one of the exact QST keywords for the gnome prisoner? | Preserve the current alias set and compatibility until builders confirm a deliberate spelling change and any migration for existing command use. |

## Checkpoint 259 disposition

Keep active lavcav **DO NOT MOVE**. Preserve its reciprocal Mountain Tracks pair, the isolated adventurer topology, and the Ravenloft2 reset under their current source owners. Keep the unregistered player_guilds SHP room reference on existing candidate row 84 **RETAIN IN PLACE** while its missing keeper/product prototypes and fallback exposure remain unresolved. The inactive-WLD sweep adds no source-only approach or room overlap, so no new candidate is warranted. No native gameplay or area data changed.

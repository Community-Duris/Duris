# Traoul's Lair repository-wide audit supplement

This supplement reconciles the active route graph, cross-area key source,
typed dependencies, exact title, bounded history, and retention status for the
[Traoul source dossier](../zone-stories/TRAOULS_LAIR.md). The source snapshot is
`72ba3deff3bb30bde880018cdebe7c6db675b697`. No world or gameplay source changed.

## Active graph and Truk boundary

The active Traoul WLD contains 44 rooms and 91/91 resolving source exits: 90
local and one outbound. Every room target resolves. When all authored exits are
treated as passable, the local graph is one weak component and one strongly
connected component; from #7000, 44 rooms are reachable and 44 have a path
back. This raw calculation ignores reset-closed/locked doors, hidden exits,
the placement and ownership of keys, actor admission, and survival.

The active boundary is reciprocal with Truk: #7000 south returns to Truk
#1622, and Truk #1622 north enters #7000. The [pair register](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L667)
records the outbound edge and [reverse record](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L676)
records the inbound edge. There is no other outside WLD source targeting
Traoul's room range, active or source-only, in the current map scan. The source
row's recorded one weak component / 44-room largest component is retained.

## Reset-state doors and conditional access

The local ZON has 14 door-reset commands. They restore the first gate
#7000→#7001 closed/locked with external key #1609; the courtyard gate
#7001→#7024 closed/locked with local key #7009; the storage/tunnel secret door;
the #7001-locked tunnel trapdoor and training approach; two closed/secret
trapdoors from the lower side passage to #7040; and the closed/locked door
between #7042 and #7043 with no configured key field. Resets cover the reverse
directions where authored. These state declarations do not qualify current
live access.

Key #1609 is defined in `truk.obj`, carried by gate guard #1601 at Truk #1622,
and placed by Truk's reset. It is the cross-area source for Traoul's first
gate. The `#7009` key for the second gate is carried by a guard reset inside
the foyer beyond that gate. The alternate tunnel reaches the foyer by way of
#7039/#7040, but the two tunnel approaches name key #7001. The #7001 prototype
exists in `traoul.obj` yet has no placement in any active local or outside
reset, shop, QST item command, or direct code assignment found in the bounded
scan. A skill-based unlock may change the live route; the source audit cannot
qualify it.

The local object key #7004 has a separate typed use: the warmaster carries it,
and the #7003 wall brace that contains #7008 names it. Dark stone key #7006 is
also on the warmaster but matches no current WLD door key field or container
key field. The gate key #7009 also matches the key field of the #7005 desk.
Keep these object-level uses distinct from room-door access.

## Typed dependencies and negative checks

| Type | Source-backed result |
| --- | --- |
| Cross-area WLD | Active Truk #1622 ↔ Traoul #7000 is the only boundary pair. No external active or inactive WLD room overlaps #7000–#7043, and no other outside exit targets a Traoul room. |
| Entrance key | Traoul's first internal gate uses object #1609 from Truk; `truk.zon` places it on mobile #1601 at #1622. This is an active typed world dependency. |
| Outside resets | No external active or source-only `M/O/P/G/E` reset command places Traoul mobiles #7000–#7014 or objects #7000–#7014. |
| QST | `qst/AREA` contains `traoul` but `areas/qst/traoul.qst` is absent, so there are zero local giver prototypes/contracts. No outside QST item input or reward references these objects. The Surface Keeps `R E 7000` / `E:7000` matches are XP, not an object reference. |
| Shops | No local `traoul.shp` and no outside shop seller or product reference to a Traoul prototype. |
| Story sidecars | No local `traoul.story.json`; a typed-integer scan found no external room/mobile/object target in #7000–#7043. A loose `7035` text match is inside an opaque story-card ID, not a typed VNUM. |
| Portals and compiled bindings | No outside type-25 object targets a Traoul room. No fixed compiled room, mobile, or object assignment uses #7000–#7043; loose `7000` code matches belong to generic Underdark zone-number checks. |
| Prototype collisions | Repository-wide current WLD/MOB/OBJ header scans find no outside duplicate room or prototype VNUM in Traoul's local ranges. |

No QST, shop, external reset, sidecar, or compiled consumer creates a source
for moldy key #7001. The object prototypes #7002 and #7011 also have no typed
reset placement. The existence of those prototypes does not prove inventory.
The local reset has 38 mobile placements, 14 door resets, two room-level
object placements, two nested object placements, three carried-item
placements, and nine equipment placements.

## Title, history, and disposition

After removing color tokens and normalizing whitespace and case, “Traoul's
Lair” has one exact owner among 442 current `.zon` titles: `traoul`. The nearby
title “Truktakyagyo” is the connected Truk area. No other current WLD room,
mobile, or object prototype collides with Traoul's local VNUMs. Followed
WLD/MOB/OBJ/ZON history stays on the same stems through the 2009 area-tree
move and later 2012–2015 maintenance. The bounded history shows no alternate
stem, distinct predecessor, rename, or whole-zone replacement. The manifest
only QST listing remains unresolved source metadata, not replacement
evidence.

Keep active Traoul **DO NOT MOVE**. Preserve the reciprocal Truk entrance and
its key #1609, the local doors and reset states, and the current placement
identities. Keep missing/unplaced keys and item prototypes visible for owner
review rather than creating an attic candidate or moving the area.

## Builder review

Checkpoint 398 records 20 explicit source-backed builder questions with
conservative recommendations in the [source dossier](../zone-stories/TRAOULS_LAIR.md#explicit-builder-questions-and-recommended-resolutions).
The recommendations distinguish configured reset and key facts from static
map possibilities, missing key sources, unqualified skill paths, and any
future story or service design. No gameplay result is claimed.

## Checkpoint 853: Current boundary and typed closure carried forward

The reviewed `areas/` and `src/` content remains identical to pinned source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`; this checkpoint carries the Checkpoint 398 source review forward without claiming a second independent parse. Traoul remains 44 rooms with 91/91 resolving exits (90 local, one outbound) in one raw strongly connected component. The sole active boundary is reciprocal with Truk at #1622 / #7000. The all-95 inactive scan still found no room overlap or other source-only approach; current cumulative source-only totals remain 11,993 overall / 415 Surf.

Preserve Truk's active placement of entrance key #1609 on #1601 at #1622. The local reset continues to declare 14 closed, locked, or secret door states; key #7001 remains unplaced despite the tunnel door references, while #7009 is reset beyond the inner gate. The QST manifest entry still has no local QST file, and the prior negative closure remains unchanged for outside reset, QST item, shop, sidecar, type-25, compiled binding, and prototype-duplicate references. These source facts do not qualify live key availability or passage. Checkpoint 398's twenty builder questions remain source review, not gameplay qualification.

Keep active `traoul` **DO NOT MOVE**. Preserve the reciprocal Truk boundary, #1609 key ownership, local reset states, and hidden tunnel topology. The all-95 scan adds no candidate approach; do not add an attic candidate, activate a route, or grant migration clearance. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L311), [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-853-traouls-graph-and-truk-boundary-remain-unchanged), and [attic plan](ATTIC_MIGRATION_PLAN.md#checkpoint-853-traoul-stays-active-with-its-truk-key-owner).

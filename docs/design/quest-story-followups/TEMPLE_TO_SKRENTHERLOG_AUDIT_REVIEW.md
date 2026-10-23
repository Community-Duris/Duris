# The Temple to Skrentherlog: repository-wide connectivity and dependency audit

**Checkpoint 82; priority 122 of the original 220-zone queue.** This supplement
reconciles the [source-comprehensive dossier](../zone-stories/THE_TEMPLE_TO_SKRENTHERLOG.md)
with the active WLD graph, inactive WLD inputs, exact typed references,
compiled numeric matches, normalized title, and bounded source history.
Authored edges and references do not establish ordinary reachability, safe
traversal, source availability, or played qualification.

## Active WLD graph

The active manifest has 351 WLD contributors. The Temple to Skrentherlog has
56 rooms and 104 exits; all 104 targets resolve. Its local exit graph has a
55-room weak component plus isolated room #80599. There are two active inbounds
and one outbound. The Yuan-Ti Village boundary is reciprocal; the Winterhaven
edge is inbound-only.

| Boundary | Reciprocal edge | Cross-area register row |
| --- | --- | --- |
| Winterhaven #55632 up to #80545 ([exit](../../../areas/wld/wh.wld#L13012)) | Incoming-only; no reverse local edge | [inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L783) |
| #80554 south to Village of the Damned #80671 ([exit](../../../areas/wld/yuan_ti.wld#L1355)) | Yuan-Ti Village #80671 north to #80554 ([return](../../../areas/wld/yuan_ti2.wld#L1606)) | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L978), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L980) |

There are no unresolved active exits. The reciprocal village pair is a declared
static boundary; this scan does not qualify its hidden door state, ordinary
availability, or safe travel.

## Inactive WLD sources

The all-95 inactive WLD scan parsed 2,080,773 exits without issues. It found
no exit targeting a Temple room and no inactive source-room VNUM overlap with
the active room VNUMs (#80500–#80554 and #80599). The curated source-only
total remains 1085, including the 115 previously reviewed Surf approaches.
These findings cover inactive WLD files only; computed routes, editor data,
and runtime movement remain separate.

## Typed references and compiled numeric matches

Exact scans across active and unregistered QST, SHP, and story files found no
external QST consumer, shop product, or structured sidecar reference to a
local room, mobile, or object. All 231 other parsed story sidecars had zero
exact hits and no parse issues. No external mobile or object prototype
definition matched. The area has no local SHP file.

The external reset-role scan found two active O rows in
[`astral_tiamat.zon`](../../../areas/zon/astral_tiamat.zon#L76) ([second](../../../areas/zon/astral_tiamat.zon#L77))
that place local coin-pile object #80571 in room #19953. The
source-comprehensive dossier already records these two resets; they add no
local quest consumer.

The external type-25 scan found no destination in a Temple room among 872
declarations from other object files: 713 active and 159 source-only. The
three active direct object-function bindings for #80556, #80579, and #80569
are already documented in the dossier
([#80556](../../../src/specs/specs.assign.c#L1730),
[#80579](../../../src/specs/specs.assign.c#L1731),
[#80569](../../../src/specs/specs.assign.c#L1732),
[#80569 behavior](../../../src/cmd/actoff.c#L2614)). Direct numeric matches
also include the local room and object constants
([#80545](../../../src/world/vnum.room.h#L18),
[#80546](../../../src/world/vnum.room.h#L19),
[#80563](../../../src/world/vnum.obj.h#L131)). Other hits are comments,
including the old stone binding and a commented-out room relocation block in
[`specs.zalrix.c`](../../../src/specs/specs.zalrix.c#L123); they are not active
route or quest hooks.

## Title, history, and disposition

The color-stripped title from [`yuan_ti.zon`](../../../areas/zon/yuan_ti.zon#L2)
normalizes to “The Temple to Skrentherlog” and is unique among the 351 active
contributors. The only global active-WLD duplicate is room #51800 in Hell Two
and Hell Three; none is in this area. The inactive-WLD scan found no room
number overlap with this zone.

The followed WLD history records a 2025 UTF-8 recoding, a 2014 typo correction,
and the older path move; the bounded ZON history's 2015 update changes an
editor-version comment. No distinct predecessor or replacement appears in the
curated [replacement register](DUPLICATE_AND_REPLACED_ZONES.md). The active,
un-commented [`areas/AREA` row](../../../areas/AREA#L351) remains **DO NOT MOVE**.

The existing dossier covers the foreign coin resets, active boundary records,
three object bindings, keys, resets, custom hammer action, epic touch, and
foreign memory ownership. The repository-wide scans add no inactive WLD route
or new exact typed consumer. Tooling/editor closure, indirect/runtime
references, persisted identity, door/reset state, dynamic access, and played
qualification remain separate.

This checkpoint used repository sources and generated documentation only. No
tests, build, server, gameplay session, database operation, migration, or
operational script was run.

## Checkpoint 438: builder question review

Twenty source-backed questions and recommended resolutions now collect the
open blood-exchange, captive, hammer, route, reset, and foreign-ownership
decisions in the [Temple builder review](../zone-stories/THE_TEMPLE_TO_SKRENTHERLOG_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).
The existing graph, typed-reference, title/history, and DO NOT MOVE findings
remain unchanged; no area source or runtime behavior changed.

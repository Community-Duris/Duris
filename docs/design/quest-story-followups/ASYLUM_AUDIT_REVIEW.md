# Asylum repository-wide audit review

This supplement completes the bounded source review for active area `asylum`.
It accompanies [The Temple of Lizizania source story
dossier](../zone-stories/ASYLUM.md). The audit describes current source
ownership and authored routes; it does not qualify live player access, reset
timing, door discovery, or safe travel.

## Active source inventory

At source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`, zone 92 is titled
`The Temple of Lizizania`. It is listed in the full world area list and the
QST area list. The active WLD contains 89 rooms (#9200–#9288), the MOB file
contains 15 prototypes (#9200–#9214), and the OBJ file contains 49 prototypes
(#9200–#9248). The active ZON header uses reset mode 1 and ends at #9288
([world membership](../../../areas/AREA#L53),
[QST membership](../../../areas/qst/AREA#L47),
[zone title and reset header](../../../areas/zon/asylum.zon#L2),
[WLD source](../../../areas/wld/asylum.wld#L1)).

`areas/qst/asylum.qst` and `areas/shp/asylum.shp` are absent; there are no
local Q/QA contracts and no story sidecar. The source reset table contains 225
commands: 56 door, 46 mobile, 98 equipment, eight give, 16 room-object, and
one nested-object reset. The local reset references resolve to Asylum's own
mobile and object catalogs, and all 56 door resets match a defined WLD exit.

## Active WLD graph and door state

The active map has 202 authored exits: 201 local and one outbound. All 202
resolve to active rooms and all 89 rooms form one weak component. Raw directed
local traversal reaches and returns from all 89 rooms when every authored
edge is treated as available. The result is 89/89 from each active inbound
anchor: #9288, #9287, and #9202. This is a source-graph result, not a live
accessibility finding.

All 56 WLD door exits have corresponding ZON resets. The reset states are 48
closed, six closed/secret, and two locked/secret; the latter pair is the
reciprocal niche passage between #9200 and #9201. Those exits carry key VNUM
#9213, the emerald key reset on Head Priestess #9209. The WLD source encodes
that pair as pickproof and keyed, and the ZON comments call it
closed/locked/secret ([WLD door](../../../areas/wld/asylum.wld#L11),
[door resets and comments](../../../areas/zon/asylum.zon#L8),
[Head Priestess reset](../../../areas/zon/asylum.zon#L251),
[key give](../../../areas/zon/asylum.zon#L256)). The
other reset doors are unkeyed. All-exit traversal does not account for closed,
secret, locked, or pickproof state; actual reveal and crossing remain
unqualified. WLD door flags are initialized in [`db.c`](../../../src/world/db.c#L1568);
ZON D resets are applied in [`db.c`](../../../src/world/db.c#L7857), and the
source audit folds their states in [`zcheck.c`](../../../src/world/zcheck.c#L575).

The active boundaries are:

| Direction | Authored edge | Evidence |
| --- | --- | --- |
| Reciprocal | Asylum #9288 south to Surface #551667; Surface north returns | [exit rows 185 and 1448](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L185), [pair row 33](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L33) |
| One-way inbound | Pineholl #16164 south to Asylum #9287 | [exit row 285](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L285), [pair row 342](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L342) |
| One-way inbound | White Harbor #55402 down to Asylum #9202 | [exit row 754](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L754), [pair row 779](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L779) |

The Pineholl source room is titled “Quest Mob Load 4,” and the White Harbor
source is an “Incarne loading room for Evils.” Their active WLD records are
real graph edges, but those room labels do not establish player-facing access.
No Asylum exit leads back to those two source areas. No active boundary is
unresolved or has a negative target.

## Inactive WLD approach

The all-95 inactive-WLD scan found no source room VNUM overlap with Asylum and
one unique source-only approach: inactive `surf.wld` room #212125 exits north
to Asylum #9288. The source file is not in `areas/AREA`, so this edge is not
active ingress ([source-only exit](../../../areas/wld/surf.wld#L254453),
[target field](../../../areas/wld/surf.wld#L254456),
[target room](../../../areas/wld/asylum.wld#L1804)). Preserve it with the
existing Surf candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).
It raises the reviewed Surf subset from 205 to 206 and the curated source-only
approach total from 3,439 to 3,440 overall; `ud_map2` remains at 15. The raw
all-source scan is broader and is not included in that curated count.

## Typed references and dependency closure

The exact typed scan found two active consumers of Asylum object #9225, “a cup
of coffee”:

| Active source | Exact reference | Classification |
| --- | --- | --- |
| `random.zon` | Line 17 gives object #9225 to the preceding mobile #100015, loaded in room #105302 | Active typed reset consumer ([give row](../../../areas/zon/random.zon#L17); [mobile reset](../../../areas/zon/random.zon#L15)) |
| `random.shp` | The shop record for mobile/shop #100015 lists object #9225 as stock | Active typed shop consumer ([stock row](../../../areas/shp/random.shp#L41)) |

No other active or inactive QST item command, SHP product, ZON reset-role
reference, or structured story-sidecar item reference matched. The QST area
list includes `asylum`, but its `.qst` source is absent and there are zero
native contracts. The local reset commands reference only locally owned
mobile and object prototypes. No external duplicate mobile/object definition
or type-25 destination into Asylum was found; 872 external type-25 declarations
were checked. The broad C/C++ number scan's only apparent local-range hit is
`0.9261%` in `specs.heavens.c`, line 5226, which is a floating-point percentage
and not a VNUM. No compiled room, mobile, or object callback binding into
Asylum was found.

This is source ownership evidence. It does not establish that the merchant
stock is currently offered, that either reset has run, or that a player can
reach the merchant or obtain the cup.

## Title and bounded history

The color-stripped current `.zon` title `The Temple of Lizizania` is unique in
the 442-title scan ([current title](../../../areas/zon/asylum.zon#L2)). Followed
WLD, MOB, OBJ, and ZON histories trace the same stem through the 2009 move into
the current `areas/` tree and later same-stem typo, text, item-flag, and
refactor edits. Bounded history shows no alternate-stem rename, distinct
predecessor, or whole-zone replacement. The prose at #9273 says that one
connecting passage was added later; that is local architectural history, not
replacement evidence.

## Builder review and migration disposition

No native task outcome can be inferred from the High Priestess's offering
dialogue, the worshipers' gifts, the initiate's uncertainty, the blue
priestess's search, the patients' confinement, or the treatment scene. Builders
should decide whether any of these become an accepted offering service,
missing-ward task, patient-care story, escape, or lasting religious outcome.
Define actual prerequisites, recipient, alternatives, agency, persistence, and
safe destination before attaching a reward. Preserve the current key/door/chest
relationship until its owner decides otherwise.

Keep active `asylum` **DO NOT MOVE** as a current full-world contributor; add
no attic candidate row. Keep `surf` **RETAIN IN PLACE** under existing
candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97) while its active
Surface-target exits, dormant WLD approaches, reset imports, and ownership
remain unresolved. The Checkpoint 191 route adds another source-only record
and grants no move clearance.


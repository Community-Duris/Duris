# Home of the Enlightened (`temple2`) audit review

Canonical key: `component:temple2`. Coverage data row 452 / CSV line 453;
candidate dependency data row 101 / CSV line 102. Reviewed October 12, 2026,
Checkpoint 539, against checkout `c500ac51b8cd26540943257e7ca4e9807263ef74`.
This is a static source, identity, story, graph, and dependency review. It
does not establish runtime quest behavior, active map ownership for
`temple2`, or player reachability.

## Source identity and Longhollow collision

The unregistered source has [WLD](../../../areas/wld/temple2.wld#L1),
[MOB](../../../areas/mob/temple2.mob#L1),
[OBJ](../../../areas/obj/temple2.obj#L1),
[ZON](../../../areas/zon/temple2.zon#L1), and
[QST](../../../areas/qst/temple2.qst#L1) files. The zone header is #344,
title `Home of the Enlightened`, reset mode 1, and last VNUM #34488. Current
`areas/AREA` and `areas/qst/AREA` do not list `temple2`; there is no local
SHP file, story sidecar, or current random-area membership. The normalized
candidate title has no exact current active `.zon` title match.

Active `long` is also zone #344. It has 100 rooms (#34400–#34499), 77 mobiles,
and 74 objects. Every candidate numeric band is shared with `long`:

| Type | Candidate records | Same-number active Longhollow records | Exact full-record matches |
| --- | ---: | ---: | ---: |
| WLD rooms | 89 (#34400–#34488) | 89 | 0 |
| MOB prototypes | 30 (#34400–#34429) | 30 | 0 |
| OBJ prototypes | 31 (#34400–#34430) | 31 | 0 |

All 89 paired room titles differ. For one concrete object conflict,
Longhollow #34405 is its Selune token while `temple2` #34405 is a black hood.
Do not co-load, merge, renumber, or treat a numeric match as proof of
replacement. The two `temple2.qst` header VNUMs, #34426 and #34427, do not
overlap Longhollow's QST header set; they are still source-only because
`temple2` is not in the quest manifest. The existing
[Longhollow audit](LONGHOLLOW_AUDIT_REVIEW.md#typed-dependencies-and-source-identity)
keeps the two area identities separate.

## Story source

The WLD describes a forest trail, hidden garden, vegetable patch, small
bridges, caves, and tunnels. The MOB file includes cultists, captives, a
frightened priest, and a chained boy. The QST records give that place a
source-authored rescue-and-letter thread: #34426 is the scared priest who
says his son Jared was captured by devil worshipers and asks that a letter
be delivered; #34427 contains the boy's response. The text has no companion
story sidecar and should remain distinct from Longhollow's active fifteen
native Q/QA contracts and story sidecar.

The QST has literal item/resource commands `R I 34427` at line 21,
`R E 100000` at line 36, and `G I 34427` at line 37. Candidate object #34427
is named `letter`; candidate mobile #34427 is the boy. Keep those typed roles
separate and confirm the QST command meanings against the feature owner
before interpreting the encoded value as an implemented reward or
requirement. The source records are evidence of intended story, not evidence
that this quest is loaded or playable.

## WLD topology and active owner disambiguation

`temple2.wld` contains 89 rooms and 196 authored exit records. All 196
targets are within the candidate's room VNUM set; its numeric graph has one
weak component, and a directed walk from #34400 reaches all 89 rooms when
all authored direction codes are included. Fourteen exit records use
direction values 6–9. Confirm their intended interpretation with the map and
loader owners before relying on a movement or reachability conclusion. No
fall-chance or current directives were found. Door state, route safety, and
return paths remain unqualified.

All 89 room VNUMs also have active Longhollow room owners. A raw scan finds
229 active WLD target records into the candidate's numeric room set: 227
from Longhollow's own graph and two Surface approaches to #34401. Those
targets resolve by the current area list to Longhollow's active room owner;
they are not `temple2` ingress. The two Surface approaches are already
documented in the [Longhollow connectivity review](LONGHOLLOW_AUDIT_REVIEW.md)
and world register. After owner disambiguation, candidate-specific active
ingress remains zero. The candidate file itself has no external WLD target,
so it adds no cross-zone route or unresolved-target count.

## Resets, typed references, and code

`temple2.zon` has 65 `M`, 58 `O`, 41 `P`, 13 `E`, one `G`, and 60 `D`
commands. Every mobile/object prototype reference is in the candidate's own
source set, and every `M`, `O`, and `D` room argument is in its WLD set. These
file-local references do not make the candidate active: their VNUMs also
resolve to different active Longhollow prototypes under current membership.

The unregistered [player_guilds SHP](../../../areas/shp/player_guilds.shp#L121)
has a source-only shop #34402 product #34405. That number means Longhollow's
active token under the current list and `temple2`'s black hood in this
candidate. Preserve the ambiguity on existing candidate dependency row 101;
do not count it as active `temple2` stock. This source-only dependency is
already traced in the [Longhollow audit](LONGHOLLOW_AUDIT_REVIEW.md#typed-dependencies-and-source-identity)
and player-guilds candidate row 84.

The bounded scan found no active foreign ZON reset, QST item/header, or SHP
keeper/product reference into the candidate MOB/OBJ VNUMs. The candidate has
no SHP source. Maintained code reads mobile #34429 for the mount skill at
[new_skills.c line 2194](../../../src/classes/new_skills.c#L2194); that is
the active Longhollow mobile under current membership, while `temple2`
defines #34429 as a slave. `epic_skills.c` references Longhollow mobile
#34446, outside the candidate's mobile band. An old `real_object0(34404)`
line in `specs.assign.c` is inside a commented block, not an active binding.
These code and shop references require separate owner interpretation; they
do not activate `temple2`.

## History, disposition, and builder questions

Followed candidate source paths reach the 2009 `areas/` path move and
subsequent 2011–2013 edits; those histories do not establish an intentional
retirement, whole-zone predecessor, or replacement decision. The different
Longhollow title and the 89/30/31 distinct same-number record sets are
positive evidence to preserve two identities, not evidence to combine them.

Keep `temple2` RETAIN IN PLACE while an owner confirms its map, QST, prototype,
code, and editor/export roles. The twenty source-backed questions and
conservative recommendations are in
[TEMPLE2_BUILDER_QUESTIONS.md](TEMPLE2_BUILDER_QUESTIONS.md). No activation,
co-loading, merge, move, or archive clearance is implied.

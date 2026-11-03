# Ako Village repository-wide audit review

This supplement completes the source-backed narrative, graph, typed-reference,
title, and bounded history review for active area `ako`. It accompanies the
[Ako Village source story dossier](../zone-stories/AKO_VILLAGE.md), which maps
the environmental story and records builder questions. All findings are
static; they do not qualify installed access, safe combat, live shop behavior,
or player outcomes. No native gameplay data changed.

## Active source and story inventory

At source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`, zone 37 is titled
`Ako Village` and contains 52 rooms (#3700–#3751), 49 mobiles (#3700–#3748),
and six objects (#3700–#3705). Its active files are WLD, MOB, OBJ, and ZON.
There is no `ako.qst`, `ako.shp`, or `ako.story.json`; the native Q/QST contract
count is zero. The area reset file has 124 M rows, 20 E rows, and two G rows.
The prose about the witchcraft trial, elders, and poverty is environmental
story, not a recorded local quest completion.

The local C special-procedure scan found five typed mobile assignments:

- `ako_hypersquirrel` on mobile #3715
- `ako_songbird` on #3701
- `ako_vulture` on #3716
- `ako_wildmare` on #3721
- `ako_cow` on #3720

These procedures provide periodic animal flavor and the songbird's pet-command
response. The numeric scan found no additional typed compiled room, mobile, or
object binding for Ako; its other raw numeric matches are table values or
comments rather than area-prototype references ([procedure source](../../../src/specs/specs.ako.c#L14),
[assignments](../../../src/specs/specs.assign.c#L214)).

## Active WLD graph and inactive approaches

The active WLD parser resolves all 107/107 authored exits. There are 106 local
edges, one outbound cross-area edge, one weak local component of 52 rooms, no
flag-8 internal exits, and no unresolved target. Raw directed traversal from
#3700 reaches all 52 rooms and has a return path from all 52 when the authored
local exits are treated as available. This is source-graph evidence, not a
player-safety or runtime-access finding.

The only active boundary is Ako #3751 west to Surface #544136; no active WLD
exit points into an Ako room, so this departure has no reciprocal active
arrival. See the [active edge register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L47)
and [pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L6).

All 95 inactive WLD files parsed in this audit sweep: 527,791 rooms and
2,080,773 exits, with no parse errors or room-VNUM overlap against the active
world. One non-overlapping source-only approach points into Ako:
`surf.wld` room #224164 east (D1) to #3751
([source room](../../../areas/wld/surf.wld#L506928)). `surf.wld` is outside the
active `areas/AREA` list, so this is not current ingress. It is preserved with
the existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).
The curated source-only approach totals therefore rise from 3,433 to 3,434
overall, 200 to 201 for Surf, and remain 15 for `ud_map2`.

## Typed cross-area references

Ako's own reset file has three E rows that equip each reset village guard with
object #283. This is an intentional, typed outgoing dependency: #283 is defined
in `heavens.obj` as a steel long sword
([Ako reset rows](../../../areas/zon/ako.zon#L91),
[Heavens prototype](../../../areas/obj/heavens.obj#L3036)). Preserve the
dependency and record it on Heavens coverage row [121](ZONE_COVERAGE_REGISTER.csv#L121).
The other local reset roles target Ako's own mobile/object/room sets.

Repository-wide exact scans found:

- No external QST item command, SHP product, typed story `item_vnums` entry, or
  active/unregistered ZON reset role referencing an Ako room/mobile/object.
- No external mobile or object prototype duplicates for the local Ako VNUMs.
- No external type-25 portal declaration targeting an Ako room among 872
  scanned declarations.
- 231 outside story sidecars parsed without errors. The only broad numeric
  token hits were `3720` inside an Alatorin exchange ID and `3707` inside a
  White Harbor request ID; neither is a typed VNUM field.
- No local QST, SHP, or story-sidecar item reference outside Ako, because those
  three area inputs are absent.

The local reset-to-object#283 dependency is not evidence of an external quest
producer or a story outcome. The separate Grandma-of-Ako invitation prototype
#22808 is described in the [Plains of Life dossier](../zone-stories/PLAINS_OF_LIFE.md#L59);
it is not a typed Ako-area input or active Ako reset/quest reference in the
scanned sources.

## Title and bounded history

After color-code normalization, the `.zon` title `Ako Village` is unique among
442 scanned current `.zon` titles; the title scanner found no lexical
candidates. Followed `ako.wld` history stays on the same stem through the 2009
areas-tree move and 2011/2013 changes. Followed `ako.zon` history stays on the
same stem through the 2009 move, 2013 edits, and 2015 quest-item-flag pass. The
bounded title and WLD/ZON history review found no alternate-stem rename,
distinct predecessor, or whole-zone replacement. The source-only Surf approach
is route evidence, not replacement evidence. Title source: [ako.zon](../../../areas/zon/ako.zon#L2).

## Migration disposition

Keep `ako` **DO NOT MOVE** because it is a current un-commented `areas/AREA`
contributor. Preserve its one-way active Surface departure, the separate
source-only Surf approach, and its three reset imports of Heavens object #283.
Keep `surf` **RETAIN IN PLACE** with existing candidate row 97. Do not add a
candidate row for Ako. Neither the source-only approach nor the complete raw
local graph proves ordinary access, safety, return travel, or migration
clearance. See [Ako coverage row 5](ZONE_COVERAGE_REGISTER.csv#L5),
[Heavens coverage row 121](ZONE_COVERAGE_REGISTER.csv#L121),
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and the
[source story dossier](../zone-stories/AKO_VILLAGE.md).

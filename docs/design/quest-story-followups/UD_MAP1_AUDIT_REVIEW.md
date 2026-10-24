# ud_map1 audit review

Canonical key: component:ud_map1. Coverage data row 463 / CSV line 464;
candidate dependency row 112 / CSV line 113. Reviewed October 12, 2026
(Checkpoint 550) against checkout c500ac51b8cd26540943257e7ca4e9807263ef74.
This is a static source, graph, typed-reference, identity, and followed-history
review. It does not establish live access, route safety, or reset outcomes.

## Source inventory and environmental story

The same-stem files are [ud_map1.wld](../../../areas/wld/ud_map1.wld#L1),
empty [ud_map1.mob](../../../areas/mob/ud_map1.mob), empty
[ud_map1.obj](../../../areas/obj/ud_map1.obj), and
[ud_map1.zon](../../../areas/zon/ud_map1.zon#L1). The ZON header is #11100,
titled “the dreadful UnderDark,” has top VNUM #1119716, and reset mode 0.
The stem is absent from [areas/AREA](../../../areas/AREA) and
[areas/qst/AREA](../../../areas/qst/AREA). No same-stem QST, SHP, map JSON,
or story sidecar exists.

The WLD defines 2,425 unique rooms from #1110554 through #1119716. All room
descriptions are empty. Nine title groups label purple phosphorescent fungus
in a huge cavern, stalactite-lined shore, lakes, rapids, a murky stream, a
wide river, and the edge/base of a waterfall. Those titles establish an
environmental setting, but the checked-in map has no prose story or authored
room-description narrative.

## Authored WLD graph

The WLD contains 9,705 exit records: 8,611 target another ud_map1 room, seven
target active WLD rooms, and 1,087 positive targets are absent from every
current WLD source. The missing targets contain 654 distinct VNUMs. There are
no negative targets. Four missing exit records target three numbers beyond
the ZON top VNUM #1119716: #1119717 twice, #1119815 once, and #1119816 once.
All source rooms and exit sources are inside the ZON range. For example,
ud_map1 #1110554 north targets missing room #1110454
([source](../../../areas/wld/ud_map1.wld#L8)); this is a source gap, not a
verified route to an omitted or future room.

The local exits form one weak component of all 2,425 rooms. Raw directed
traversal over authored WLD edges from the active Shafts target room #1116916
reaches all 2,425 rooms. This counts source directions without proving that
doors, handlers, deployment, or runtime state permit travel. There are no
fall-chance or current fields, and no ud_map1 room VNUM overlaps any other
current WLD source.

The seven source-only exits from this unregistered map into active WLDs are:

| ud_map1 source exit | Current target | Evidence |
| --- | --- | --- |
| #1111610 down to Shafts #10025 | Shafts | [ud_map1](../../../areas/wld/ud_map1.wld#L6070) · [Shafts](../../../areas/wld/shafts.wld#L423) |
| #1112933 down to Shafts #10035 | Shafts | [ud_map1](../../../areas/wld/ud_map1.wld#L15629) · [Shafts](../../../areas/wld/shafts.wld#L578) |
| #1113221 north to Valinhav #90800 | Valinhav | [ud_map1](../../../areas/wld/ud_map1.wld#L16872) · [Valinhav](../../../areas/wld/valinhav.wld#L1) |
| #1115029 up to Pragtog #10649 | Pragtog | [ud_map1](../../../areas/wld/ud_map1.wld#L25482) · [Pragtog](../../../areas/wld/pragtog.wld#L879) |
| #1116916 up to Shafts #10004 | Shafts | [ud_map1](../../../areas/wld/ud_map1.wld#L35944) · [Shafts](../../../areas/wld/shafts.wld#L75) |
| #1117360 east to Domain #36800 | Domain | [ud_map1](../../../areas/wld/ud_map1.wld#L39560) · [Domain](../../../areas/wld/dlsc.wld#L1) |
| #1117763 up to Shafts #10000 | Shafts | [ud_map1](../../../areas/wld/ud_map1.wld#L42080) · [Shafts](../../../areas/wld/shafts.wld#L1) |

The six Shafts, Domain, and Pragtog approaches were already recorded in prior
checkpoints. The #1113221 north exit to Valinhav #90800 is the newly
reconciled non-overlapping source-only approach. It raises curated inactive-WLD
totals to 11,989 overall / 415 from Surf. None is active ingress while
ud_map1 remains unregistered.

One active Shafts exit targets this candidate:

| Active source exit | Candidate room | Evidence |
| --- | --- | --- |
| Shafts #10004 down to #1116916 | #1116916 | [unresolved-exit register](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L17) |

The candidate has the exact reverse-direction exit, #1116916 up to Shafts
#10004. The active Shafts target remains unresolved because the current loader
does not include ud_map1. A static reciprocal pair does not activate either
side.

## Typed reset consumer and bounded reference closure

The four M commands in [ud_map1.zon](../../../areas/zon/ud_map1.zon#L10)
request active Underworld mobile #4520 (a roper) at candidate rooms #1111555,
#1113136, #1115329, and #1118722. Their source-only status follows from the
candidate’s exclusion from areas/AREA. They do not establish current stock or
live placements. The prototype definition is in
[underworld.mob](../../../areas/mob/underworld.mob#L260).

A checked-in source scan found no other WLD source targeting ud_map1; the
single external WLD target is the active Shafts record above. No external ZON
room reset, QST/SHP/map-story room reference, external OBJ room destination,
or additional numeric room reference in maintained src code was found.
Editor projects, map-generator inputs, deployment configuration, and
persisted room identities were not inspected.

The one src numeric-token match is #1111222 inside
[Piergeiron’s open-path string](../../../src/specs/specs.verzanan.c#L48).
The routine indexes that string one character at a time, and each digit is
converted to a movement command through [exitnumb_to_cmd](../../../src/specs/specs.verzanan.c#L316)
and [the direction mapper](../../../src/core/utility.c#L717). In this context,
the seven digits in 1111222 are successive direction steps, not a room-VNUM
operand. The special is assigned to mobile #3211 in
[specs.assign.c](../../../src/specs/specs.assign.c#L548). This classifies the
numeric hit as an encoded path string rather than a direct map1 room
dependency; no live travel behavior was tested.

## Title, history, and disposition

After color and whitespace normalization, “the dreadful underdark” is unique
among 442 current ZON titles. The ud_map0 and ud_map2 titles are also distinct,
and the three maps’ room-number sets are pairwise disjoint. Followed
ud_map1 WLD/ZON history reaches the August 2009 area-tree move and a 2013
same-stem update. The empty MOB/OBJ files have older tracked history reaching
2005/2006 and the 2009 move. No explicit predecessor, successor, or
replacement decision was found. Similar underdark titles and shared
source-only Underworld resets do not establish replacement.

Keep ud_map1 **RETAIN IN PLACE** outside both manifests. Preserve the single
active unresolved Shafts reference, all seven source-only approaches to
active maps, 1,087 missing-target exits, and four source-only Underworld
mobile reset records. Do not activate, merge, regenerate, move, or archive
until the map/editor owner confirms the authoritative project, sparse-edge
intent, Valinhav/other boundaries, reset ownership, and runtime and persisted
identity. See the [20 builder questions](UD_MAP1_BUILDER_QUESTIONS.md). No
build, boot, deployment, editor project, or database state was inspected.

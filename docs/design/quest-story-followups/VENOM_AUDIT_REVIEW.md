# Venom GH source audit

Canonical key: `component:venom`. Coverage data row 467 / CSV line 468;
candidate dependency row 116 / CSV line 117. Reviewed October 12, 2026
(Checkpoint 554) against checkout `72ba3deff3bb30bde880018cdebe7c6db675b697`.
This is a static source, graph, typed-reference, identity, and followed-history
review. It does not establish live access, runtime shop state, or route safety.

## Source inventory and authored story

The same-stem files are [venom.wld](../../../areas/wld/venom.wld#L1),
[venom.mob](../../../areas/mob/venom.mob#L1),
[venom.obj](../../../areas/obj/venom.obj), and
[venom.zon](../../../areas/zon/venom.zon#L1). The OBJ file is empty. There is
no same-stem QST, SHP, story sidecar, or map JSON. `venom` is absent from
[areas/AREA](../../../areas/AREA) and
[areas/qst/AREA](../../../areas/qst/AREA). The ZON header is #303, titled
“Venom GH,” with top VNUM #30309 and reset mode 0.

The WLD defines ten rooms #30300–#30309, each with a non-empty long
description. The text depicts a Venom clan cave with a fanged entrance, a
web-covered cavern, a glittering underground cove, a merchant’s hovel, an
Elder’s Gathering Hall, a sleeping cavern, a haunted “Room of Shifting
Images,” a glowing treasury, a descent, and battlements/training grounds.
Descriptions mention snake carvings, skulls, bones, blood, negative energy,
goblin currency, and clan training. There is no authored QST contract or
separate story brief in the same-stem source package.

The three MOB records #30300–#30302 are bronze golems. The ZON contains two
object resets: Player Castles object #11054 at room #30300 and #11104 at room
#30301 ([ZON lines 9–10](../../../areas/zon/venom.zon#L9)). The prototypes are
owned by [player_castles.obj #11054](../../../areas/obj/player_castles.obj#L153)
and [#11104](../../../areas/obj/player_castles.obj#L273), not by the empty
Venom OBJ source. Four M commands reset Venom’s numeric mobile IDs into room
#30300, including two identical source commands for #30300. These records are
source data only while Venom remains outside the area manifest.

## Authored WLD graph and map boundaries

The WLD contains 16 exit records: 15 target another room in the Venom source
and one exits up from #30300 to unregistered ud_map2 room #1124260
([Venom exit](../../../areas/wld/venom.wld#L13),
[ud_map2 target](../../../areas/wld/ud_map2.wld#L49196)). There are no negative
targets, exit flags/key fields, fall-chance fields, or current fields. The
outbound ud_map2 direction has no reverse exit from its exact target room.

The local graph is one weak component of ten rooms. Directed traversal from
#30300 reaches four rooms; reverse traversal reaches all ten. Its strongly
connected component sizes are 4, 3, 2, and 1. This is raw authored topology;
it does not establish that the unregistered source loads or that any route is
usable.

No active WLD exit targets rooms #30300–#30309. One unregistered WLD exit from
ud_map2 #1126542 down (`D5`) targets #30300
([ud_map2 source exit](../../../areas/wld/ud_map2.wld#L80538)). The same target
number is also defined by `bb_gh.wld` and `vodgh.wld`, so the source data does
not identify Venom as the intended destination. The ud_map2 #1126542 down
direction pairs numerically with `bb_gh.wld` #30300 up to #1126542; the
duplicated #30300 identities make the ud_map2 return target ambiguous. This
is not active ingress. Venom separately declares a one-way exit from #30300
up to ud_map2 #1124260, which has no reverse from that exact room.

## Room and prototype identity

Every Venom room VNUM #30300–#30309 also occurs in both `bb_gh.wld` and
`vodgh.wld`. Full room-block comparisons found no identical room records
between these three sources. Venom mobile IDs #30300–#30302 also occur in both
`bb_gh.mob` and `vodgh.mob`, with no identical full mobile records. The
normalized title “Venom GH” is unique in the current ZON sources, while WLD
header #303 and the room/mobile VNUMs are shared. These overlaps are competing
source identities, not proof that one guildhall replaced another.

The compiled [inn binding](../../../src/specs/specs.assign.c#L2455) names room
#30303. That VNUM is shared by three different unregistered room records; the
binding does not establish which map identity was intended or that it is
effective in the current runtime.

## Typed references and bounded dependency closure

| Source or role | Finding | Classification |
| --- | --- | --- |
| Active WLD inbound | No active WLD direction targets #30300–#30309. | Candidate-specific active WLD ingress is zero. |
| ud_map2 WLD boundary | #1126542 down targets shared #30300; Venom #30300 up targets #1124260. | One unregistered inbound number match is ambiguous among three maps; one Venom outbound edge is source-only. |
| Venom ZON | Two O commands import Player Castles objects #11054/#11104; four M commands use mobile IDs #30300–#30302. | Preserve the cross-area object imports and the source reset commands. The mobile VNUMs collide with bb_gh and vodgh records. |
| External OBJ portals | `sunhall.obj` object #30202 is type 25 and targets #30302 ([source](../../../areas/obj/sunhall.obj#L29)); bb_gh/vodgh also have portal objects whose target numbers occur in their own overlapping room sets. | The Sunhall source is not in the current manifests; its #30302 target does not resolve map identity. The bb_gh/vodgh local portal pairs are not evidence of Venom access. |
| SHP | Venom has no SHP source. Unregistered `bb_gh.shp` and `vodgh.shp` both use keeper key #30301 with distinct room locations #30302 and #30306. | Numeric overlap with Venom’s mobile and room IDs is not proof of a Venom shop or active shop state. |
| QST/story | No same-stem QST or sidecar, and no external QST room-VNUM reference was found. | No repository-defined Venom quest contract was identified. |
| Maintained code | The exact room-number binding at #30303 is in specs.assign.c; no other direct candidate VNUM binding was found. | Owner and intended room identity remain unresolved; no runtime behavior was tested. |

These are repository-bounded findings. Generic range-based code behavior,
map-editor inputs, generator/export source, deployment, live shop parsing, and
persisted room identity were not verified.

## Title, history, and disposition

The normalized title “Venom GH” occurs once in current `.zon` sources. Followed
WLD/ZON history reaches the 2009 area-tree relocation and a 2013 same-stem
“stuff” update; MOB history also shows a 2012 reset-format update. The checked
manifests contain no Venom entry. No explicit predecessor, successor,
retirement, or whole-zone replacement was found. The same #303 header and
room/mobile VNUMs across `bb_gh`, Venom, and `vodgh` do not establish map
ownership or replacement.

Keep Venom **RETAIN IN PLACE** outside both manifests. Preserve all four
same-stem files, the Player Castles O resets, the local M commands, the
source-only ud_map2 edge, the ambiguous ud_map2 inbound target, the three-map
room/mobile identities, and the #30303 code binding. Do not activate, merge,
regenerate, move, or archive until the map/guild owner confirms the map
identity, reset/prototype ownership, portal and shop targets, and source/editor
authority. See the [30 builder questions](VENOM_BUILDER_QUESTIONS.md). No
build, boot, editor project, deployment, or database state was inspected.

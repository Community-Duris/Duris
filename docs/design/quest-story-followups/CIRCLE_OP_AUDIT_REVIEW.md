# Yggdrasil Outpost Audit Review

Reviewed October 11, 2026, Checkpoint 463. This is a static source, story, WLD graph, followed-history, and typed-reference review of the unregistered `circle_op` component. It does not establish player access, reset execution, area ownership, or permission to move or activate these files.

## Source scope and title

The current source family contains [WLD](../../../areas/wld/circle_op.wld), [MOB](../../../areas/mob/circle_op.mob), [OBJ](../../../areas/obj/circle_op.obj), and [ZON](../../../areas/zon/circle_op.zon). The ZON header is #1, titled “Yggdrasil Outpost,” with last VNUM #45412 and reset mode 0. The WLD defines three rooms #45410–#45412, the MOB defines four prototypes #45410–#45413, and the OBJ defines two prototypes #45410–#45411. There is no same-stem SHP or QST, and no story sidecar. The component is absent from the full [area manifest](../../../areas/AREA) and [quest manifest](../../../areas/qst/AREA); the raw area-note field in the coverage row is blank.

## Root, tree, and tunnel story

The three rooms describe an entrance into a large root, an elevated room in the tree, and a tunnel through a root. The entrance prose emphasizes rough, textured roots and a soft glow from fine filaments; the upper room describes the tree rising above the forest and nearby plains; the tunnel continues through the root interior ([entrance](../../../areas/wld/circle_op.wld#L1), [tree room](../../../areas/wld/circle_op.wld#L20), [tunnel](../../../areas/wld/circle_op.wld#L39)).

Four mobile prototypes accompany the rooms: a sunbeam golem, moonbeam golem, weeping willow, and cypress. The ZON has one M command for each, all placed in room #45410. The candidate has no QST, SHP, or separate story contract; its prose and reset pattern are the available authored story evidence.

## Exact shared records with the separate `circlegh` source

Unregistered `circlegh` is titled “Yggdrasil Guild Hall” and covers a larger room set, #45400–#45417. Direct block comparisons show that its WLD records for #45410–#45412 are byte-for-byte identical to the three `circle_op` room records. Its MOB records #45410–#45413 and OBJ records #45410–#45411 are also exact copies of the corresponding `circle_op` prototypes ([`circlegh` rooms](../../../areas/wld/circlegh.wld#L169), [mobile copies](../../../areas/mob/circlegh.mob#L68), [object copies](../../../areas/obj/circlegh.obj#L1)).

This establishes shared source content, not which map is canonical or whether one is a safe replacement for the other. The ZON titles differ (“Yggdrasil Outpost” and “Yggdrasil Guild Hall”), and both components are outside the full loader manifest. `circlegh` also defines room #45407, which `circle_op` does not.

Other unregistered sources use Yggdrasil guildhall names in separate numeric bands: `rf_gh` covers #28900–#28912 and `ygg` covers #30400–#30412 ([coverage rows 440](ZONE_COVERAGE_REGISTER.csv#L440) and [473](ZONE_COVERAGE_REGISTER.csv#L473)). Their titles and shared subject do not establish a rename or common room identity. The existing [Yggdrasil/`ygg` review](ARDGRAL_AUDIT_REVIEW.md) documents a separate Ard’gral VNUM conflict for that source.

## Static WLD graph and zone-field discrepancy

The candidate has five authored directions:

| Source room | Direction | Target | Relation |
| --- | --- | --- | --- |
| #45410 | D0/north | #45411 | Local |
| #45410 | D4/up | #154462 | Outbound to Duris3 |
| #45411 | D2/south | #45410 | Local |
| #45411 | D5/down | #45412 | Local |
| #45412 | D4/up | #45411 | Local |

The four local directions form one weak component, and directed traversal from #45410 reaches all three candidate rooms. The D4/up exit from #45410 targets #154462, titled “The Ocean” in the unregistered [Duris3 WLD](../../../areas/wld/Duris3.wld#L1143703); no return exit to #45410 is authored there.

The ZON header and all three WLD room fields use raw zone number 1, while the room VNUMs are in the #454 numeric VNUM/100 band. Thus all three rooms and all five exits from them are flagged by the register’s numeric-range checks. The component is not in the full loader list; no active WLD room defines #45410–#45412 and no active WLD edge targets those IDs. All five positive exit targets are unresolved in the active world, including the four targets repeated by unregistered `circlegh`. Candidate-specific active inbound remains zero. The inactive Duris3 edge does not add an approach to an active area. Static metrics are five source directions, zero active-loader-resolved targets, one outbound cross-zone target, five positive unresolved targets, one local component of three rooms, and zero fall/current rooms. Curated inactive-WLD totals remain 11,987 overall / 415 from Surf. See the [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-463-yggdrasil-outpost-graph-range-and-duris3-boundary).

## Resets, portals, and typed references

The candidate ZON places external object #11052 in room #45410 and local object #45410 in room #45412. Object #11052 is defined in [Player Castles](../../../areas/obj/player_castles.obj#L137) as a magic mouth; this reference occurs in the unregistered `circle_op` source and is not evidence of an active reset. The four M commands place mobiles #45410–#45413 in room #45410. There is no reset for local object #45411.

The two local objects are teleport objects. Object #45410, “root portal,” has destination #45407, a room defined in `circlegh.wld` but absent from `circle_op.wld` ([object](../../../areas/obj/circle_op.obj#L1), [room #45407](../../../areas/wld/circlegh.wld#L129)). The ZON places that object at room #45412. Object #45411 targets room #45412, while its long description says it transports to “Goodie Cont”; `circle_op.zon` does not place it ([object text and destination](../../../areas/obj/circle_op.obj#L18)). The separate `circlegh.zon` places object #45411 in room #45407 ([reset](../../../areas/zon/circlegh.zon#L11)). Preserve the authored destination, prose, and placements until their owner confirms their intended relationship.

Static searches found no other-area ZON reference to rooms #45410–#45412, no active WLD ingress, and no exact C++ source reference to these numbers. Dynamic travel, external tooling, editor state, and persisted data were not established by these source checks.

## Followed history, identity, and retention

Followed `circle_op` WLD history remains on the same-stem path from the 2009 area-tree move through the 2013 maintenance commit. The separate `circlegh` WLD path is also present at the 2009 move and remains same-stem through the 2013 maintenance history. No tracked path rename or explicit replacement decision was found. Their exact shared records and concurrent histories support preserving two source identities until an owner provides a map of their relationship.

Keep `circle_op` RETAIN IN PLACE. Before any separate path or activation proposal, resolve the exact copies with `circlegh`, the raw zone/range mismatch, #154462 boundary intent, object #45410’s #45407 destination, object #45411’s “Goodie Cont” description, the Player Castles magic-mouth import, reset intent, dynamic routes, editor/tooling, and persisted room/mobile/object ownership.

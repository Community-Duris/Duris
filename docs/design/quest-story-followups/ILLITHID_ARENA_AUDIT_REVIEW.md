# Arena of the Elder Brain audit: directed graph and room dependencies

This static supplement accompanies the [environmental story dossier](../zone-stories/ILLITHID_ARENA.md). It reconciles the local WLD graph, current active and inactive WLD approaches, typed references, compiled room behavior, title, and tracked source history. No native area, code, quest, or story-sidecar source was changed.

## Static connectivity and hidden arrivals

The active area contributes 18 rooms (#11200–#11217) and 40 authored exits. All 40 target another local room; there is no active inbound or outbound cross-area WLD record and no negative or unresolved target. The local undirected projection has one weak component. The directed graph has SCCs sized 16, 1, and 1. From #11200, 16 rooms are forward-reachable and all 18 have authored local returns to #11200. #11216 and #11217 each have only a down exit back to the center and no authored WLD entrance; both are titled combatant rooms.

Thirty-eight exit records form 19 reciprocal pairs. The two unpaired records are the one-way down exits from the combatant rooms to #11200. The hidden pit route is reciprocal at the source-record level: #11208 down to #11212 and #11212 up to #11208. Both exits are reset closed and secret by [`illithid_arena.zon`](../../../areas/zon/illithid_arena.zon#L8). The raw graph counts include that route without modeling its reset state, discovery, or opening.

I scanned the 351 active WLD files and all 95 non-active WLD files. None contains an exit targeting #11200–#11217, and none defines an overlapping room VNUM. The full scan encountered two malformed exit blocks in `northern_wilderness.wld`; their visible target rows are #12270 and #6803, neither an Arena room. Local WLD parsing had no errors. These results establish the checked-in WLD boundary, not dynamic arrival, live movement, or safe travel. See the [local WLD source](../../../areas/wld/illithid_arena.wld) and [coverage row 133](ZONE_COVERAGE_REGISTER.csv#L133).

## Typed dependencies and special behavior

The current source set contains WLD/MOB/OBJ/ZON files, one local mobile (#11200), two local objects (#11200–#11201), and no QST, SHP, or story sidecar. Role-based scans found no active or non-active foreign ZON reset into the local room/mobile/object sets, and no external QST item, SHP product, or typed story `item_vnums` reference. The all-object type-25 destination scan found no portal target in the Arena. There is no local type-25 object.

The compiled room assignment is a direct active dependency: [`specs.assign.c`](../../../src/specs/specs.assign.c#L2282) binds the `squid_arena` room procedure to every room #11200–#11217. The procedure [`specs.room.c`](../../../src/specs/specs.room.c#L1407) handles WILL only, telling the player the Elder Brain prevents them from enforcing their will. It does not create a QST receipt, route, combatant, or victory.

Two additional numeric room cases target #11208 in [`specs.guards.c`](../../../src/specs/specs.guards.c#L259): `guild_guard` checks a southward move, while [`guardian`](../../../src/specs/specs.guards.c#L432) selects west as a block direction. The active room has north and east exits plus its reset-hidden down exit; it has no authored south or west exit. The local mobile file contains only the quiet fungus, so no local guard reset is bound to these cases. Treat the numeric matches as guard-code context, not extra WLD edges or proof of a current in-area guardian.

The broad numeric scan also encountered code point data in `unicode.c` and an object value row in `cosmic.obj`; neither parsed as an Arena room target or typed room/mobile/object reference. Active VNUM assignment scans show no external mobile/object prototype collision with the Arena sets.

## Title, semantic comparison, and tracked history

After Duris color-code removal, “Arena of the Elder Brain” has one exact owner in the current active title inventory: `illithid_arena`, zone 112. Tracked WLD/MOB/OBJ/ZON histories remain on the `illithid_arena` stem after the 2009 `areas/` relocation; the history records later same-stem maintenance and no alternate-stem predecessor, title rename, or explicit whole-area replacement.

Nearby concepts are separate contributors. The active `arena` area is titled “CTF Tournament.” Ixarkon and Ixxillikor are separately named illithid cities whose own source text mentions an Elder Brain. The bounded scans found no shared room VNUM, WLD boundary, typed reset, QST/SHP/story item reference, or type-25 destination linking those areas to this Arena's room and prototype sets. Shared vocabulary does not identify the vat's brain as one of their actors. See the [title/history register](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-235-elder-brain-arena-title-and-history) and the [related Ixxillikor](../zone-stories/IXXILLIKOR.md) and [Ixarkon](../zone-stories/IXARKON.md) dossiers.

## Disposition

Keep active `illithid_arena` **DO NOT MOVE**. It remains a current contributor with a one-component local WLD graph and an active compiled WILL handler. There is no source-only ingress, attic candidate row, local quest, or direct foreign typed consumer to justify movement. The combatant-room reachability and guard-code cases remain builder questions; they do not change current ownership.

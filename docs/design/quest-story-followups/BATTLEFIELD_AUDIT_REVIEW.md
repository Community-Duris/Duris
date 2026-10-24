# The Battlefield: repository-wide connectivity and dependency audit

**Checkpoint 74; priority 114 of the original 220-zone queue.** This supplement
revalidates the comprehensive [Battlefield dossier](../zone-stories/THE_BATTLEFIELD.md)
against the active world manifest, all inactive WLD sources, typed references,
compiled numeric bindings, title inventory, and bounded tracked history. It
records source declarations and code references; it does not certify live
availability, travel safety, or gameplay outcomes.

## Active graph and inactive Surf approach

All 351 active WLD inputs parsed without issues. The Battlefield has 163 rooms
and 320 authored exits; all 320 targets resolve. Its local weak-exit graph has
four components of 134, 16, 8, and 5 rooms. The single active outbound is
Battlefield room #66400 south to Surface #624087, paired with the Surface north
return ([Battlefield](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L831),
[Surface](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1634)).

All 95 inactive WLD files parsed 527,791 rooms and 2,080,773 exits without
issues. No inactive source room reuses a Battlefield room VNUM. One
source-only `surf.wld` exit, from room #279354 west (`D3`) to Battlefield room
#66400, is recorded at [`surf.wld`#L1665170](../../../areas/wld/surf.wld#L1665170).
It is not an active entrance. This record raises the curated source-only
inbound total from 674 to 675 and the reviewed `surf.wld` subset from 105 to
106. The global active-room scan continues to find only the separate #51800
collision in Hell Two and Hell Three.

## External typed references and compiled bindings

The bounded scan found no external QST item command, SHP product, ZON
reset-role target, exact structured story-sidecar VNUM, mobile/object prototype
definition, or external type-25 destination into the Battlefield. All 231 other
story sidecars parsed without issues and had zero exact structured matches;
864 external type-25 declarations were scanned.

Four direct compiled numeric matches are already classified in the
comprehensive dossier. Two CHAOS equipment rows reference local object #66454
([`chaos_eq_data.h`#L192](../../../src/account/chaos_eq_data.h#L192) and
[`#L438`](../../../src/account/chaos_eq_data.h#L438)); the tradeskill table
contains #66410 ([`tradeskill.c`#L212](../../../src/economy/tradeskill.c#L212));
and object #66419 has the `righteous_blade` special assignment
([`specs.assign.c`#L1504](../../../src/specs/specs.assign.c#L1504)). These are
local code bindings, not foreign prototype declarations. No new native
journal dependency was identified.

## Title, tracked history, and disposition

The normalized active-zone-title scan found one exact title, `The Battlefield`,
among 351 contributors. Its color-coded form is in
[`battlefi.zon`](../../../areas/zon/battlefi.zon#L2), and the registered source
key appears in [`areas/AREA`](../../../areas/AREA#L250). Bounded WLD/ZON history
reaches the 2009 area-tree move, with later WLD edits through 2014 and ZON edits
through 2015. The [duplicate/replacement register](DUPLICATE_AND_REPLACED_ZONES.md)
identifies no distinct predecessor or replacement. The Battlefield remains
**DO NOT MOVE** as a current full-world contributor.

This checkpoint adds no zone, quest, or runtime change. It used repository
sources and static registers only; no tests, build, server/runtime, gameplay,
database, migration, or operational script was run. Gameplay, editor/tooling,
persisted-reference, and dynamic-route qualification remain separate.

# Caves of Mt. Skelenak: repository-wide connectivity and dependency audit

**Checkpoint 83; priority 123 of the original 220-zone queue.** This supplement
reconciles the [source-comprehensive dossier](../zone-stories/CAVES_OF_MT_SKELENAK.md)
with the active WLD graph, inactive WLD sources, exact typed references,
compiled numeric matches, normalized title, and bounded source history.
Authored edges and code destinations do not establish ordinary availability,
safe travel, or played qualification.

## Active WLD graph

The active manifest has 351 WLD contributors. Caves of Mt. Skelenak has 152
rooms and 324 exits; 323 target active rooms. Its internal-exit graph has
three weak components of 124, 22, and 6 rooms. There are five reciprocal
boundary pairs, one additional active outbound, and one positive unresolved
outbound.

| Caves edge | Return or disposition | Cross-area register rows |
| --- | --- | --- |
| #4001 south to Surface #607420 | Surface #607420 north to #4001 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L49), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1592) |
| #4093 down to Underdark #709664 | Underdark #709664 up to #4093 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L50), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1689) |
| #4125 west to Surface #557269 | Surface #557269 east to #4125 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L51), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1455) |
| #4131 north to Surface #556876 | Surface #556876 south to #4131 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L53), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1453) |
| #4152 up to Tentro2 #69088 | Tentro2 #69088 down to #4152 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L54), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L843) |
| #4130 east to Surface #556876 | No reverse edge to #4130; Surface returns to #4131 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L52) |
| #4111 east to #153645 | Positive unresolved active target; see the [unresolved register](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L6) | — |

The unresolved target VNUM #153645 exists in inactive `Duris3.wld`
([room](../../../areas/wld/Duris3.wld#L1126546)), which is absent from
[`areas/AREA`](../../../areas/AREA). It is not an active destination. The
existing unresolved-exit record says the normal world build omits that source
and prunes the edge. The cross-area table therefore has six active outbounds,
five active inbounds, and five reciprocated pairs; #4111 remains outside the
resolving graph.

## Inactive WLD sources

The all-95 inactive WLD scan parsed 2,080,773 exits without issues. It found
three non-overlapping `surf.wld` source rooms that target active Caves rooms:

| Source-only record | Direction and active target | Evidence |
| --- | --- | --- |
| #213977, Molten Rim of the Lava Lake | West to #4130 | [exit](../../../areas/wld/surf.wld#L293317) |
| #214276, The Footsteps of Clangeddin Silverbeard | North to #4001 | [exit](../../../areas/wld/surf.wld#L299576) |
| #215488, Molten Rim of the Lava Lake | West to #4131 | [exit](../../../areas/wld/surf.wld#L324964) |

No inactive WLD source-room VNUM overlaps one of the active Caves room VNUMs.
These three Surf records raise the curated source-only total from 1085 to
1088 and the reviewed Surf subset from 115 to 118. They remain source-only map
records, not active ingress.

## Typed references and compiled numeric matches

Exact scans of active and unregistered QST files found no external consumer;
active and unregistered SHP scans found no external shop product; ZON
reset-role scans found no import. The 231 other parsed story sidecars had zero
exact numeric hits and no parse issues. No external mobile or object prototype
definition matched. The area has no local SHP file.

The external type-25 scan found no destination in a Caves room among 871
declarations from other object files: 712 active and 159 source-only. A
context-reviewed C/C++ numeric scan found these exact local-type matches:

| Local target | Static reference | What the source establishes |
| --- | --- | --- |
| Mobile #4007, orc warrior | [`new_skills.c`](../../../src/classes/new_skills.c#L2542) | The orc-summoning event calls `read_mobile(4007)`. Its nearby failure log still says mobile #4010; that text is not the load target. |
| Mobile #4010, small hobgoblin | [`sparser.c`](../../../src/net/sparser.c#L763) | The follow-message path suppresses bystander messages for this mobile VNUM. |
| Room #4109 | [`psionics.c`](../../../src/classes/psionics.c#L614) | `spell_mind_travel` includes the room twice in its random destination table. |
| Rooms #4108 and #4109 | [`specs.ixarkon.c`](../../../src/specs/specs.ixarkon.c#L18), bound at [`specs.assign.c`](../../../src/specs/specs.assign.c#L1874) | The object #96402 illithid teleport veil includes both rooms as destinations. This code table does not establish when the object is available. |
| Room #4128 | [`specs.guards.c`](../../../src/specs/specs.guards.c#L234) | The shared `guild_guard` room-number switch blocks north when that procedure is active in this room. No Caves mobile is assigned this special by the scanned literal bindings. |
| Object #4031, trollhide belt | [`chaos_eq_data.h`](../../../src/account/chaos_eq_data.h#L269) and [enhanceable profile](../../../src/account/chaos_eq_data.h#L513) | The item appears in standard and enhanceable Blighter equipment profiles; those declarations do not prove kit selection or issuance. |

The raw numeric pass also finds room-number collisions in mobile bindings for
other VNUMs, plus generic buffer and port constants. These are not counted as
Caves prototype dependencies. The existing dossier already covers its local
special handlers and literal-binding limits.

## Title, history, and disposition

The color-stripped title from [`caves_skelenak.zon`](../../../areas/zon/caves_skelenak.zon#L2)
normalizes to “Caves of Mt. Skelenak” and is unique among the 351 active
contributors. The only global active-WLD duplicate is room #51800 in Hell Two
and Hell Three; none is in this area. The inactive-WLD scan found no room
number overlap with the active Caves room set.

The followed WLD history includes broad 2026 line-ending/refactor changes and
the 2011 caves-enhancement edit; the bounded ZON history records the 2015
quest-item-flag pass and older cave changes. No distinct predecessor or
replacement appears in the curated [replacement register](DUPLICATE_AND_REPLACED_ZONES.md).
The active, un-commented [`areas/AREA` row](../../../areas/AREA#L21) remains
**DO NOT MOVE**.

The existing dossier covers the unresolved Duris3 destination, active
boundary records, hazards, reset sources, custom behaviors, and imported
dependencies. This repository-wide scan adds three inactive Surf approaches
and the listed direct code references. Tooling/editor closure, indirect and
persisted references, dynamic route availability, doors and resets, safe
travel, and played qualification remain separate.

This checkpoint used repository sources and generated documentation only. No
tests, build, server, gameplay session, database operation, migration, or
operational script was run.

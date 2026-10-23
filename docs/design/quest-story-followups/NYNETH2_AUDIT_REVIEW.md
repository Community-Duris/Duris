# Ny’Neth’s Stronghold: repository-wide route, dependency, and history supplement

This bounded static supplement accompanies the [comprehensive Ny’Neth’s
Stronghold source dossier](../zone-stories/NYNETH2.md). It reconciles active
and source-only WLD routes, typed QST/story/reset references, compiled
prototype references, title ownership, and bounded same-stem history. Static
source review does not establish runtime access or gameplay outcomes.

## Active WLD boundary and directed reach

Active `nyneth2` has 101 rooms (#23000–#23100) and 210/210 resolving exits,
all local; it has no authored outbound WLD edge. The local graph has seven
weak components of 78, 6, 5, 4, 3, 3, and 2 rooms. Raw directed traversal from
root #23000 reaches 76 rooms, and 77 rooms have a path back. These figures
assume every authored exit is available and do not settle keyed access,
movement controls, or actual arrival. The [WLD source](../../../areas/wld/nyneth2.wld#L1)
has no parse issues or internal flag-8 exits.

The sole active WLD inbound is White Harbor #55632 north to Nyneth2 #23098,
the second vault ([source room](../../../areas/wld/wh.wld#L12992),
[target room](../../../areas/wld/nyneth2.wld#L1991),
[exit-register row 780](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L780),
[area-pair summary row 807](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L807)). It has
no reciprocal Nyneth2 exit. The source is a multi-exit room titled “First
teleport room for gnomish key”; treat this as a technical controller lead,
not a verified ordinary approach. Its live activation, intended users, and
safe return are unqualified.

All 95 WLD files outside the active `areas/AREA` list parsed without issues
(527,791 rooms and 2,080,773 exits). None overlaps Nyneth2 room VNUMs or exits
into them. The curated source-only totals therefore remain 3,404 overall,
172 for Surf, and 15 for ud_map2. No candidate row is added.

## Exact typed and compiled dependencies

The active prior-stage `nyneth` reset places object #22903 in room #22909
([reset](../../../areas/zon/nyneth.zon#L98)). That object is a type-25 portal
whose declared target is Nyneth2 room #23000
([prototype and target](../../../areas/obj/nyneth.obj#L25)). Among 854 scanned
type-25 declarations outside Nyneth2’s object prototypes, this is the one
target into the area.

Nyneth2 resets two prior-stage dark portals, object #22966, one Heavens
monolith, object #360, and prior-stage vault guardian mobile #22904 into
second vault #23098 ([resets](../../../areas/zon/nyneth2.zon#L144),
[portal prototype](../../../areas/obj/nyneth.obj#L403),
[monolith prototype](../../../areas/obj/heavens.obj#L3741),
[guardian prototype](../../../areas/mob/nyneth.mob#L66)). These are typed
cross-area reset references. Active Alatorin also has two G reset rows for
Nyneth2 object #23045, the potion of slime, on the aboleth slime mage
([reset rows](../../../areas/zon/alatorin.zon#L4314),
[local object](../../../areas/obj/nyneth2.obj#L425)).

The bounded scans found no external QST item command, SHP product, or
structured story item reference into Nyneth2 prototypes. All 231 other story
sidecars parsed without errors and had no numeric-token matches. No external
duplicate mobile or object prototype was found. The only direct compiled
prototype references are object #23056 Lifereaver’s special assignment
([assignment](../../../src/specs/specs.assign.c#L1702)) and object #23064 in
the standard and enhanceable berserker Chaos kits
([standard kit](../../../src/account/chaos_eq_data.h#L243),
[enhanceable kit](../../../src/account/chaos_eq_data.h#L487)). The raw #23000
match in `new_events.c` is an example memory-address comment, not a room
dependency. Nyneth2 has no local SHP file.

## Title and bounded history

The normalized title “Ny'Neth's Stronghold” belongs only to active stem
`nyneth2` among 442 scanned `.zon` files
([title source](../../../areas/zon/nyneth2.zon#L2)). `nyneth3` has the distinct
title “Ny'Neth's Stronghold Continued” and is the next roadmap priority.
Followed `nyneth2.wld` history reaches the 2009 areas-tree move and an opaque
2013 same-stem edit; `nyneth2.zon` history also includes the 2015 quest-item
flag pass. These bounded paths establish no alternate-stem rename, distinct
predecessor, or whole-zone replacement. The White Harbor edge and Nyneth
portal/reset references are route and dependency evidence, not replacement
proof. See the [duplicate and replacement review](DUPLICATE_AND_REPLACED_ZONES.md).

## Migration disposition

Keep active `nyneth2` DO NOT MOVE. Preserve its one-way White Harbor inbound,
the active prior-stage portal target, both directions of cross-area reset
dependencies, and the compiled Lifereaver and Chaos-kit references. Keep
`nyneth`, `wh`, `alatorin`, and `heavens` as separate active contributors.
There are no inactive-WLD approaches and no new candidate row. Editor,
administrative, installed-data, persisted-player, indirect-runtime, and actual
traversal closure remain outside this bounded scan; no gameplay, migration,
or source change is proposed.

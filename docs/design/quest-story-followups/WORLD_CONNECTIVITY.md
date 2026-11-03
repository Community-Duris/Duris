# World connectivity and reachability audit

**Checkpoint 2 — static source graph complete at the frozen source snapshot.
Door state, dynamic travel, safe traversal, and player reachability still need
manual review. No zone or native room data changed.**

## Scope and loader model

This census uses native files at source snapshot
`72ba3deff3bb30bde880018cdebe7c6db675b697`. The normal generator reads the
uncommented entries of [`areas/AREA`](../../../areas/AREA), concatenates each
listed `.wld` file in that order, and writes the generated world input
([`make_wld.c`](../../../areas/src/wld/make_wld.c#L24),
[`make_wld.c`](../../../areas/src/wld/make_wld.c#L49)). The server reads the
generated [`areas/world.wld`](../../../src/world/db.h#L23). During boot, it
assigns rooms to numeric zone ranges and binds direction records; `renum_world`
removes an exit if its target VNUM cannot resolve
([`db.c`](../../../src/world/db.c#L1321),
[`db.c`](../../../src/world/db.c#L1383),
[`db.c`](../../../src/world/db.c#L1545),
[`db.c`](../../../src/world/db.c#L1587)).

Duris has ten ordinary direction slots: north/east/south/west/up/down plus
four diagonals ([`defines.h`](../../../src/core/defines.h#L435)). The loader
also reads fall chance and current fields from room records
([`db.c`](../../../src/world/db.c#L1395)). Area files outside `areas/AREA`
are not concatenated by this normal generator. A target room found only in
one of those files is still absent from `areas/world.wld` and is removed by
the runtime's target-resolution pass.

This is an authored-exit graph, not a verdict that characters can safely or
ordinarily reach every room. It does not execute secret-door searches, resets,
switches, timed transitions, spells, portals, vehicle movement, custom
procedures, staff paths, hazards, race/level rules, or PvP restrictions. The
raw door/key values in a `.wld` exit do not represent its final reset state.
The component counts below also treat a direction pair as an undirected link;
they say nothing about travel permission or return travel.

## Reconciled graph counts

| Measure | Count | Interpretation |
| --- | ---: | --- |
| Full `areas/AREA` contributors | 351 | Includes shared/administrative sources, `limbo`, and the `end` marker. |
| Loaded room records | 253,261 | Excludes the `end` file's `$` sentinel, which the loader stops at. |
| Unique loaded room VNUMs | 253,260 | One VNUM occurs in two active WLD files: 51800. |
| Authored exit records | 906,176 | Every parsed `D0`–`D9` record from the 351 active WLD inputs. |
| Positive targets present in active WLD sources | 899,032 | Addressable by a VNUM, with the duplicate-51800 ambiguity noted below. |
| Positive targets absent from active WLD sources | 7,099 | The loader cannot resolve these through the current full-area list. |
| Negative targets | 45 | `setup_dir` rejects them before normal room renumbering. |
| Raw exits pruned by target validation | 7,144 | 7,099 positive missing targets plus 45 negative values. |
| Cross-zone exit records | 1,785 | Classified by the numeric zone ranges loaded from `.zon` headers. |
| Directed source-zone → target-zone pairs | 836 | Exact pair totals and examples are in the pair summary. |
| Cross-zone exits with a reverse-direction record | 1,384 | Structural reverse edge exists; this does not prove it opens or is passable. |
| Missing positive targets also present in an unregistered WLD | 201 | These values occur only in current source files excluded from `areas/AREA`; see the exact target register. |

The simple graph contains 6,702 weak components when active room records are
collapsed by numeric VNUM. Its largest component has 223,671 VNUMs; the next
has 10,000; 29,589 VNUMs lie outside the largest component. These counts
include generated maps and isolated room templates, ignore direction and door
state, and collapse the duplicated VNUM. They are useful for finding source
clusters, not for claiming a zone is unreachable.

The 351 active rows in
[`ZONE_COVERAGE_REGISTER.csv`](ZONE_COVERAGE_REGISTER.csv) now carry room,
exit, range-owner, unresolved-target, fall/current, and local-component
counts. The full source-level edge evidence is available in:

- [`WORLD_CROSS_AREA_EXIT_REGISTER.csv`](WORLD_CROSS_AREA_EXIT_REGISTER.csv) —
  all 1,785 cross-zone exit records, including source line, room, direction,
  destination, raw flags, key VNUM, reverse-edge status, and numeric zone owners.
- [`WORLD_CROSS_AREA_PAIR_SUMMARY.csv`](WORLD_CROSS_AREA_PAIR_SUMMARY.csv) —
  all 836 directed zone pairs and their edge counts.
- [`WORLD_UNRESOLVED_EXIT_REGISTER.csv`](WORLD_UNRESOLVED_EXIT_REGISTER.csv) —
  all 7,144 raw targets that the loader does not keep, with exact source lines
  and matching unregistered WLD files when found.

## Source-topology findings that need an owner decision

### Duplicate room VNUM 51800

[`hell_two_dis.wld`](../../../areas/wld/hell_two_dis.wld#L10375) and
[`hell_three_minauros.wld`](../../../areas/wld/hell_three_minauros.wld#L1)
both declare room `#51800`, with different room descriptions and exits. The
Dis zone's header ends at 51800
([`hell_two_dis.zon`](../../../areas/zon/hell_two_dis.zon#L3)); Minauros ends
at 52199 ([`hell_three_minauros.zon`](../../../areas/zon/hell_three_minauros.zon#L3)).
Under the current numeric ranges, both records fall inside Dis's 51401–51800
range. The WLD generator concatenates both sources; `real_room0` performs a
binary search over the numeric world table, which does not define an intended
winner for duplicate VNUMs ([`db.c`](../../../src/world/db.c#L8480)). Ten
authored exits point to VNUM 51800, and the duplicate Minauros record has four
outgoing exits to Minauros VNUMs. The duplicate is therefore a concrete
identity/range conflict, not proof that either plane should be removed.

**Builder question:** Which room owns 51800, and should Dis's upper bound,
Minauros's first room, or a wider range be changed? Before any separate fix,
trace every reset, quest, item, procedure, exit, and persisted-VNUM exposure
affected by the chosen unique mapping. Keep both zones in place until that
decision is made.

### Positive missing targets in unregistered WLD files

Of the 7,099 positive targets absent from the active room table, 201 match
room VNUMs in eleven current WLD stems outside `areas/AREA`. Because several
legacy files reuse the same numeric rooms, those 201 exits produce 358
file-to-target matches. They originate in 33 active WLD source areas. The
most frequent candidate files are `surf` and `surface2011` (145 matches each),
`Duris3` (26), `ud` (18), `underdark2011` (11), and `brass-old-2` (7). The
remaining exact list is in
[`ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv`](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv).

These are unresolved raw references today because the target files are not in
the normal build list. Do not treat the old room's presence as proof of a
working route, or the missing target as an automatic repair request. The
unresolved-exit register preserves the source descriptions and destinations
for builder review.

### Source-only exits into active rooms found during zone follow-up

The active-world census above does not include exits authored in unregistered
WLD files. Before the Mistywood supplement, 302 source-only exits into active
rooms had been individually reviewed. Mistywood adds 53 records, Darkfall adds
two, Kimordril adds two, Bugentol adds one, and Ixarkon adds two. Dirk’nspire
adds none, Troll Caves adds three, IceCrag adds two, Father Tel's Holy Cloister
adds two, Turolopolis adds one, Du'Maathe Castle adds five, Tundra adds seven,
The Fields Between and The Town of Moregeeth each add one, Ceothia adds one,
Braddistock Mansion adds 192, The Desert City of Venan'Trut adds one, and
The Basin Wastes adds one, Nakral's Crypt adds two, The Valoisian Castle adds
one, Harrow adds one, and Mountain Tracts adds three, bringing this listed
curated evidence to 586 before the Githzerai and Worms checkpoints below:
five `surf.wld` records from the Nax, Ugta, and Sea reviews, five records into
Kvarkpass Connector from `surf.wld` and `underdark2011.wld`, two into Fort
Boyard from `surf.wld` and `underworld2.wld`, one into Cityruin from
`surf.wld`, thirteen into Limbo from seven unregistered WLD files, 276 into
Githyanki Fortress from unregistered `Duris3.wld`, and 53 into Mistywood plus
two into Darkfall, two into Kimordril, and one into Bugentol from unregistered
`surf.wld`, plus two into Ixarkon from unregistered `ud_map2.wld`, two from
`surf.wld` plus one from `underdark2011.wld` into Troll Caves, and one each
from those two source files into IceCrag room 97001, plus one each from
`surf.wld` and `underworld2.wld` into Father Tel's Holy Cloister room 67100,
one from `surf.wld` room 236469 north into Turolopolis room 7102, three from
`surf.wld` plus two from `oldmap8.wld` into Du'Maathe Castle rooms 37409,
37416, and 37425, seven from `surf.wld` into Tundra rooms 13703, 13712,
13778, 13796, 13870, 13895, and 13936, one into The Fields Between room 71001,
one into The Town of Moregeeth room 70198, one from `surf.wld` room 267058
north into Ceothia room 80800, and 192 `Duris3.wld` records in four bands
whose numeric targets cover all 48 Braddistock Mansion rooms. The Mansion
review also found 48 `Duris3.wld` source-room VNUMs overlapping the active
Mansion range; both the exits and source-room records remain source-only. One
additional `surf.wld` exit targets Desert room 49003 and remains source-only.
The Basin Wastes review adds one down exit from `surf.wld` room 229308 into
room 34000, with no source-room VNUM overlap. Nakral's Crypt adds one north
exit each from `surf.wld` room 276166 and `underworld2.wld` room 422044 into
room 14300; neither source file declares a room in the active Crypt range.
The Valoisian Castle adds one north exit from `surf.wld` room 205053 into room
38575, with no source-room VNUM overlap against Val's active room set.
Harrow adds one north exit from `surf.wld` room 214566 into room 29400; that
source room does not overlap Harrow's active room VNUMs.
Mountain Tracts adds three exits from `surf.wld` rooms 277280 west, 277864
south, and 280870 north into active rooms 20951, 21110, and 20901,
respectively. None of the 95 unregistered WLD files declares a source room in
Mountain Tracts' active room VNUM set.
The Orcish Slave Camp scan found no unregistered target match or source-room
VNUM overlap; its only active boundary is the reciprocal Split Shield pair.

| Unregistered source record | Direction and active target | Evidence |
| --- | --- | --- |
| Room 209731 | North (`D0`) to active Ugta room 39335 | [`surf.wld`](../../../areas/wld/surf.wld#L204203), [Ugta review](UGTA_STORY_REVIEW.md) |
| Room 256920 | West (`D3`) to active Nax room 37800 | [`surf.wld`](../../../areas/wld/surf.wld#L1194408), [Nax review](NAX_STORY_REVIEW.md) |
| Room 219818 east (`D1`), 219820 west (`D3`), and 220119 north (`D0`) | All three lead to active Sea room 67568 | [`219818`](../../../areas/wld/surf.wld#L415766), [`219820`](../../../areas/wld/surf.wld#L415808), [`220119`](../../../areas/wld/surf.wld#L422075), [Sea review](SEA_STORY_REVIEW.md) |
| Room 275132 | South (`D5`) to active Kvarkpass Connector room 54804 | [`surf.wld`](../../../areas/wld/surf.wld#L1576619), [Kvarkpass review](KVARKPASS_CONNECTOR_STORY_REVIEW.md) |
| Room 276995 | Down (`D2`) to active Kvarkpass Connector room 54690 | [`surf.wld`](../../../areas/wld/surf.wld#L1615682), [Kvarkpass review](KVARKPASS_CONNECTOR_STORY_REVIEW.md) |
| Room 277897 | West (`D3`) to active Kvarkpass Connector room 54803 | [`surf.wld`](../../../areas/wld/surf.wld#L1634616), [Kvarkpass review](KVARKPASS_CONNECTOR_STORY_REVIEW.md) |
| Room 278490 | East (`D1`) to active Kvarkpass Connector room 54600 | [`surf.wld`](../../../areas/wld/surf.wld#L1647053), [Kvarkpass review](KVARKPASS_CONNECTOR_STORY_REVIEW.md) |
| Room 278150 | Down (`D5`) to active Kvarkpass Connector room 54804 | [`underdark2011.wld`](../../../areas/wld/underdark2011.wld#L320912), [Kvarkpass review](KVARKPASS_CONNECTOR_STORY_REVIEW.md) |
| Room 205016 | North (`D0`) to active Fort Boyard room 38100 | [`surf.wld`](../../../areas/wld/surf.wld#L105307), [Fort Boyard review](FORT_BOYARD_STORY_REVIEW.md) |
| Room 417529 | North (`D0`) to active Fort Boyard room 38100 | [`underworld2.wld`](../../../areas/wld/underworld2.wld#L10625), [Fort Boyard review](FORT_BOYARD_STORY_REVIEW.md) |
| Room 262344 | West (`D3`) to active Cityruin room 66200 | [`surf.wld`](../../../areas/wld/surf.wld#L1308240), [Cityruin review](CITYRUIN_STORY_REVIEW.md) |
| Rooms 265588, 267393, 270997, 271893, and 273391 | Down/west/north exits to active cyrictunnel rooms 54500, 54529 (two), 54554, and 54549 | [Exact records](CYRICTUNNEL_AUDIT_REVIEW.md#source-only-inactive-surf-approaches); source-only, not active ingress. |
| surf.wld room 208177 | North to active Dark Forest room 75171 | [Source record](../../../areas/wld/surf.wld#L171645), [Dark Forest audit](DARK_FOREST_AUDIT_REVIEW.md#source-only-inactive-surf-approach); source-only, not active ingress. |
| oldmap8.wld room 187735 | West (`D3`) to active yuan_ti2 room 80600 | [Source room and exit](../../../areas/wld/oldmap8.wld#L162128), [Yuan Ti2 audit](YUAN_TI2_AUDIT_REVIEW.md#source-only-inactive-wld-approaches); source-only, not active ingress. |
| surf.wld room 281432 | West (`D3`) to active yuan_ti2 room 80600 | [Source room and exit](../../../areas/wld/surf.wld#L1708752), [Yuan Ti2 audit](YUAN_TI2_AUDIT_REVIEW.md#source-only-inactive-wld-approaches); source-only, not active ingress. |
| `dalvik.wld` room 2 | North (`D0`) to active Limbo room 1 | [`dalvik.wld`](../../../areas/wld/dalvik.wld#L37), [Limbo review](LIMBO_STORY_REVIEW.md) |
| `hostel.wld` rooms 2 (`D2`), 49 (`D0`), and 86 (`D1`) | All three lead to active Limbo room 1 | [`#2`](../../../areas/wld/hostel.wld#L40), [`#49`](../../../areas/wld/hostel.wld#L934), [`#86`](../../../areas/wld/hostel.wld#L1757), [Limbo review](LIMBO_STORY_REVIEW.md) |
| `Magetower.wld` rooms 2 (`D0`) and 64 (`D2`) | Both lead to active Limbo room 1 | [`#2`](../../../areas/wld/Magetower.wld#L21), [`#64`](../../../areas/wld/Magetower.wld#L863), [Limbo review](LIMBO_STORY_REVIEW.md) |
| mavulsk.wld rooms #2 (D2) and #3 (D0) | Numeric target #1 currently resolves to Limbo, but Mavulsk itself defines #1 as “Before the Stone Mansion”; these are not confirmed Limbo approaches. | [room #2](../../../areas/wld/mavulsk.wld#L47), [room #3](../../../areas/wld/mavulsk.wld#L70), [Mavulsk identity audit](MAVULSK_AUDIT_REVIEW.md#wld-graph-and-unresolved-map-boundaries) |
| `Mentiri.wld` room 2 | Up (`D4`) to active Limbo room 1 | [`Mentiri.wld`](../../../areas/wld/Mentiri.wld#L21), [Limbo review](LIMBO_STORY_REVIEW.md) |
| `Nexus_1.wld` room 2 | South (`D2`) to active Limbo room 1 | [`Nexus_1.wld`](../../../areas/wld/Nexus_1.wld#L17), [Limbo review](LIMBO_STORY_REVIEW.md) |
| `theurgist.wld` rooms 2 (`D0`), 3 (`D2`), and 4 (`D3`) | All three lead to active Limbo room 1 | [`#2`](../../../areas/wld/theurgist.wld#L25), [`#3`](../../../areas/wld/theurgist.wld#L34), [`#4`](../../../areas/wld/theurgist.wld#L43), [Limbo review](LIMBO_STORY_REVIEW.md) |
| Rooms 132300–132368 | 69 north (`D0`) exits, one from each room to active Fortress rooms 132000–132068 | [`Duris3.wld` first target](../../../areas/wld/Duris3.wld#L678308), [last target](../../../areas/wld/Duris3.wld#L679736), [Fortress review](GITHYANKI_FORTRESS_STORY_REVIEW.md) |
| Rooms 131999–132067 | 69 east (`D1`) exits, one from each room to active Fortress rooms 132000–132068 | [`Duris3.wld` first target](../../../areas/wld/Duris3.wld#L671991), [last target](../../../areas/wld/Duris3.wld#L673419), [Fortress review](GITHYANKI_FORTRESS_STORY_REVIEW.md) |
| Rooms 131700–131768 | 69 south (`D2`) exits, one from each room to active Fortress rooms 132000–132068 | [`Duris3.wld` first target](../../../areas/wld/Duris3.wld#L665716), [last target](../../../areas/wld/Duris3.wld#L667144), [Fortress review](GITHYANKI_FORTRESS_STORY_REVIEW.md) |
| Rooms 132001–132069 | 69 west (`D3`) exits, one from each room to active Fortress rooms 132000–132068 | [`Duris3.wld` first target](../../../areas/wld/Duris3.wld#L672041), [last target](../../../areas/wld/Duris3.wld#L673469), [Fortress review](GITHYANKI_FORTRESS_STORY_REVIEW.md) |
| Rooms 228050–233443 | 53 exits to 27 active Mistywood rooms | [Mistywood's complete grouped record list](MISTYWOOD_AUDIT_REVIEW.md#connectivity-reconciliation) |
| `surf.wld` room 248948 east (`D1`) and room 249568 west (`D3`) | Active Darkfall rooms 95300 and 95348, respectively | [248948 exit](../../../areas/wld/surf.wld#L1027063), [249568 exit](../../../areas/wld/surf.wld#L1040095), [Darkfall supplement](DARKFALL_AUDIT_REVIEW.md) |
| `surf.wld` room 216660 east (`D1`) and room 216664 west (`D3`) | Active Kimordril rooms 95502 and 95655, respectively | [216660 exit](../../../areas/wld/surf.wld#L349555), [216664 exit](../../../areas/wld/surf.wld#L349647), [Kimordril supplement](KIMORDRIL_AUDIT_REVIEW.md) |
| `surf.wld` room 280727 west (`D3`) | Active Bugentol room 95800 | [room 280727](../../../areas/wld/surf.wld#L1693959), [Bugentolen review](../zone-stories/BUGENTOLEN.md) |
| `ud_map2.wld` room 1120305 west (`D3`) and room 1120404 north (`D0`) | Active Ixarkon room 96492 | [first target](../../../areas/wld/ud_map2.wld#L26), [second target](../../../areas/wld/ud_map2.wld#L140), [Ixarkon review](IXARKON_AUDIT_REVIEW.md) |
| `ud_map2.wld` room 1124844 east (`D1`) | Active The Caverns of the Worms room 6900; source-only, not active ingress | [source room](../../../areas/wld/ud_map2.wld#L57606), [exit](../../../areas/wld/ud_map2.wld#L57614), [target](../../../areas/wld/ud_map2.wld#L57617), [Worms audit supplement](WORMS_AUDIT_REVIEW.md) |
| `surf.wld` room 275641 | South (`D2`) to active Troll Caves room 96903 | [exit target](../../../areas/wld/surf.wld#L1587268), [Troll Caves review](TROLL_CAVES_AUDIT_REVIEW.md) |
| `surf.wld` room 276840 | Down (`D5`) to active Troll Caves room 96900 | [exit target](../../../areas/wld/surf.wld#L1612439), [Troll Caves review](TROLL_CAVES_AUDIT_REVIEW.md) |
| `underdark2011.wld` room 272717 | Down (`D5`) to active Troll Caves room 96900 | [exit target](../../../areas/wld/underdark2011.wld#L298938), [Troll Caves review](TROLL_CAVES_AUDIT_REVIEW.md) |
| `surf.wld` room 215567 | East (`D1`) to active IceCrag room 97001 | [exit target](../../../areas/wld/surf.wld#L326610), [IceCrag review](ICECRAG_AUDIT_REVIEW.md) |
| `underdark2011.wld` room 224716 | East (`D1`) to active IceCrag room 97001 | [exit target](../../../areas/wld/underdark2011.wld#L109754), [IceCrag review](ICECRAG_AUDIT_REVIEW.md) |
| `surf.wld` room 229643 | South (`D2`) to active Cloister room 67100 | [exit target](../../../areas/wld/surf.wld#L621846), [Cloister review](CLOISTER_AUDIT_REVIEW.md) |
| `underworld2.wld` room 417516 | North (`D0`) to active Cloister room 67100 | [exit target](../../../areas/wld/underworld2.wld#L10352), [Cloister review](CLOISTER_AUDIT_REVIEW.md) |
| `surf.wld` room 236469 | North (`D0`) to active Turolopolis room 7102 | [exit target](../../../areas/wld/surf.wld#L765120), [Turolopolis review](TUROLOPOLIS_AUDIT_REVIEW.md) |
| `oldmap8.wld` room 188135 | West (`D3`) to active Du'Maathe Castle room 37409 | [exit target](../../../areas/wld/oldmap8.wld#L170540), [Du'Maathe review](DU_MAATHE_CASTLE_AUDIT_REVIEW.md) |
| `oldmap8.wld` room 188234 | North (`D0`) to active Du'Maathe Castle room 37416 | [exit target](../../../areas/wld/oldmap8.wld#L172603), [Du'Maathe review](DU_MAATHE_CASTLE_AUDIT_REVIEW.md) |
| `surf.wld` room 224560 | South (`D2`) to active Du'Maathe Castle room 37425 | [exit target](../../../areas/wld/surf.wld#L515235), [Du'Maathe review](DU_MAATHE_CASTLE_AUDIT_REVIEW.md) |
| `surf.wld` room 224861 | West (`D3`) to active Du'Maathe Castle room 37409 | [exit target](../../../areas/wld/surf.wld#L521540), [Du'Maathe review](DU_MAATHE_CASTLE_AUDIT_REVIEW.md) |
| `surf.wld` room 225160 | North (`D0`) to active Du'Maathe Castle room 37416 | [exit target](../../../areas/wld/surf.wld#L527787), [Du'Maathe review](DU_MAATHE_CASTLE_AUDIT_REVIEW.md) |
| `surf.wld` room 213929 | East (`D1`) to active Tundra room 13796 | [exit target](../../../areas/wld/surf.wld#L292308), [Tundra review](TUNDRA_AUDIT_REVIEW.md) |
| `surf.wld` room 217352 | North (`D0`) to active Tundra room 13712 | [exit target](../../../areas/wld/surf.wld#L364059), [Tundra review](TUNDRA_AUDIT_REVIEW.md) |
| `surf.wld` room 219160 | East (`D1`) to active Tundra room 13870 | [exit target](../../../areas/wld/surf.wld#L401991), [Tundra review](TUNDRA_AUDIT_REVIEW.md) |
| `surf.wld` room 219780 | East (`D1`) to active Tundra room 13703 | [exit target](../../../areas/wld/surf.wld#L414987), [Tundra review](TUNDRA_AUDIT_REVIEW.md) |
| `surf.wld` room 219787 | West (`D3`) to active Tundra room 13778 | [exit target](../../../areas/wld/surf.wld#L415134), [Tundra review](TUNDRA_AUDIT_REVIEW.md) |
| `surf.wld` room 224279 | East (`D1`) to active Tundra room 13936 | [exit target](../../../areas/wld/surf.wld#L509354), [Tundra review](TUNDRA_AUDIT_REVIEW.md) |
| `surf.wld` room 224287 | West (`D3`) to active Tundra room 13895 | [exit target](../../../areas/wld/surf.wld#L509522), [Tundra review](TUNDRA_AUDIT_REVIEW.md) |
| `surf.wld` room 267058 | North (`D0`) to active Ceothia room 80800 | [exit target](../../../areas/wld/surf.wld#L1407141), [Ceothia review](CEOTHIA_AUDIT_REVIEW.md) |
| `surf.wld` room 267652 | South (`D2`) to active The Fields Between room 71001 | [exit target](../../../areas/wld/surf.wld#L1419619), [Fields Between review](FIELDS_BETWEEN_AUDIT_REVIEW.md) |
| `surf.wld` room 274726 | West (`D3`) to active The Town of Moregeeth room 70198 | [exit declaration](../../../areas/wld/surf.wld#L1568090), [Moregeeth review](MOREGEETH_AUDIT_REVIEW.md) |
| Four `Duris3.wld` source-room bands: 134701–134748 (`D2`), 135000–135047 (`D1`), 135002–135049 (`D3`), and 135301–135348 (`D0`) | 192 numeric exits to active Braddistock Mansion rooms 135001–135048, 48 in each band | [four-band evidence and source-room overlap](BRADDISTOCK_MANSION_AUDIT_REVIEW.md#active-and-source-only-wld-boundaries) |
| `surf.wld` room 202208 east (`D1`) | Active The Desert City of Venan'Trut room 49003 | [exit](../../../areas/wld/surf.wld#L46344), [Desert City review](DESERT_CITY_AUDIT_REVIEW.md) |
| `surf.wld` room 229308 down (`D5`) | Active The Basin Wastes room 34000 | [source room](../../../areas/wld/surf.wld#L614796), [exit](../../../areas/wld/surf.wld#L614816), [Basin Wastes review](BASIN_WASTES_AUDIT_REVIEW.md) |
| `surf.wld` room 276166 north (`D0`) and `underworld2.wld` room 422044 north (`D0`) | Both lead to active Nakral's Crypt room 14300 | [Surf exit](../../../areas/wld/surf.wld#L1598270), [Underworld2 exit](../../../areas/wld/underworld2.wld#L41114), [Crypt review](NAKRALS_CRYPT_AUDIT_REVIEW.md) |
| `surf.wld` room 205053 north (`D0`) | Active The Valoisian Castle room 38575 | [exit](../../../areas/wld/surf.wld#L106081), [Valoisian Castle review](VALOISIAN_CASTLE_AUDIT_REVIEW.md) |
| `surf.wld` room 214566 north (`D0`) | Active Harrow room 29400 | [source room](../../../areas/wld/surf.wld#L305638), [exit](../../../areas/wld/surf.wld#L305642), [target](../../../areas/wld/surf.wld#L305645), [Harrow review](HARROW_AUDIT_REVIEW.md) |
| `surf.wld` room 277280 west (`D3`) | Active Mountain Tracts room 20951 | [source room](../../../areas/wld/surf.wld#L1621652), [exit](../../../areas/wld/surf.wld#L1621668), [target](../../../areas/wld/surf.wld#L1621671), [Mountain Tracts review](MOUNTAIN_TRACTS_AUDIT_REVIEW.md) |
| `surf.wld` room 277864 south (`D2`) | Active Mountain Tracts room 21110 | [source room](../../../areas/wld/surf.wld#L1633904), [exit](../../../areas/wld/surf.wld#L1633916), [target](../../../areas/wld/surf.wld#L1633919), [Mountain Tracts review](MOUNTAIN_TRACTS_AUDIT_REVIEW.md) |
| `surf.wld` room 280870 north (`D0`) | Active Mountain Tracts room 20901 | [source room](../../../areas/wld/surf.wld#L1696962), [exit](../../../areas/wld/surf.wld#L1696966), [target](../../../areas/wld/surf.wld#L1696969), [Mountain Tracts review](MOUNTAIN_TRACTS_AUDIT_REVIEW.md) |
| `surf.wld` room 215024 west (`D3`) | Active Elemental Groves room 3800; source-only, not active ingress | [source room](../../../areas/wld/surf.wld#L315232), [exit](../../../areas/wld/surf.wld#L315248), [target](../../../areas/wld/surf.wld#L315251), [Elemental Groves supplement](ELEMENTAL_GROVES_AUDIT_REVIEW.md) |
| `surf.wld` room 229658 south (`D2`) | Active Temple of the Earth room 43500; source-only, not active ingress | [source room](../../../areas/wld/surf.wld#L622146), [exit](../../../areas/wld/surf.wld#L622158), [target](../../../areas/wld/surf.wld#L622161), [Temple of the Earth supplement](TEMPLE_OF_THE_EARTH_AUDIT_REVIEW.md) |
| `underworld2.wld` room 437928 south (`D2`) | Active Temple of the Earth room 43500; source-only, not active ingress | [source room](../../../areas/wld/underworld2.wld#L157522), [exit](../../../areas/wld/underworld2.wld#L157534), [target](../../../areas/wld/underworld2.wld#L157537), [Temple of the Earth supplement](TEMPLE_OF_THE_EARTH_AUDIT_REVIEW.md) |
| `surf.wld` room 242869 north (`D0`) | Active Githzerai Stronghold room 44400; source-only, not active ingress | [source room](../../../areas/wld/surf.wld#L899489), [exit](../../../areas/wld/surf.wld#L899493), [target](../../../areas/wld/surf.wld#L899496), [Githzerai supplement](GITHZERAI_STRONGHOLD_AUDIT_REVIEW.md) |
| `underdark2011.wld` room 255552 north (`D0`) | Active Githzerai Stronghold room 44400; source-only, not active ingress | [source room](../../../areas/wld/underdark2011.wld#L216028), [exit](../../../areas/wld/underdark2011.wld#L216032), [target](../../../areas/wld/underdark2011.wld#L216035), [Githzerai supplement](GITHZERAI_STRONGHOLD_AUDIT_REVIEW.md) |
| `underworld2.wld` room 421439 east (`D1`) | Active Githzerai Stronghold room 44400; source-only, not active ingress | [source room](../../../areas/wld/underworld2.wld#L36660), [exit](../../../areas/wld/underworld2.wld#L36668), [target](../../../areas/wld/underworld2.wld#L36671), [Githzerai supplement](GITHZERAI_STRONGHOLD_AUDIT_REVIEW.md) |

| `surf.wld` room 245010 north (`D0`) | Active Transparent Tower room 16212; source-only, not active ingress | [source room](../../../areas/wld/surf.wld#L944410), [exit target](../../../areas/wld/surf.wld#L944417), [Transparent Tower supplement](TRNSPTOW_AUDIT_REVIEW.md#active-graph-and-world-boundaries) |

The shared Githzerai `shift_prime` handler also lists Turolopolis room 7117 as
one of its code-defined destinations; this is outside the WLD-exit ledger and
does not add to the curated source-only count. See the
[Turolopolis audit supplement](TUROLOPOLIS_AUDIT_REVIEW.md).

The four `Duris3.wld` bands account for 276 records and each band maps in
order to Fortress targets `#132000`–`#132068`. Of those source rooms, 272 are
titled “The Ocean” and four are titled “Mountains”: `#132026` on `D1` and `D3`,
and `#131725`/`#131726` on `D2`
([`#132026` east](../../../areas/wld/Duris3.wld#L672558),
[`#132026` west](../../../areas/wld/Duris3.wld#L672566),
[`#131725` south](../../../areas/wld/Duris3.wld#L666241),
[`#131726` south](../../../areas/wld/Duris3.wld#L666262)). `Duris3` is not in
the active `areas/AREA` list, so these are legacy source-grid references, not
active-world ingress. Their presence does not change the one paired active
Astral boundary or establish player access. See the
[Fortress coverage row](ZONE_COVERAGE_REGISTER.csv#L106) and
[source-only component row](ZONE_COVERAGE_REGISTER.csv#L387).

These are candidate-to-active source-only references, not active-world ingress
records or proof of a runtime route. They do not change the 906,176 active
source exit count or the 201 active exits targeting unregistered WLD rooms.
The manually reviewed 579 are a curated subset, not a complete count of
source-only exits into active room VNUMs. A separate parse of all 95
unregistered WLD files found 572,824 authored D0–D9 exit records whose target
numbers match a currently loaded room VNUM. The largest raw match groups are
`surface2011-temp.wld` (240,016 records; 60,004 source-room VNUMs overlap the
active world), `newbiemaps.wld` (239,212; 59,851 overlaps), and `Duris3.wld`
(88,256; 22,064 overlaps). `surf.wld` has 415 raw target matches and no source
room VNUM overlap with active areas. These are numeric matches, not confirmed
travel routes: several large unregistered map sources have heavy source-room
VNUM overlap with active areas, so source ownership and replacement history
must be resolved before their matches can be treated as paths. The full raw scan remains open for that
classification. The previously identified `surf` records are also recorded in the
[`surf` retention plan](ATTIC_MIGRATION_PLAN.md#L27); the new
`underworld2.wld` record remains source-only and needs an owner decision.
Broader source-only-to-active route discovery and classification remain open.

### Known sparse and dynamic cases

- The active `underdark` map has 6,813 missing-cell targets. The prior
  comprehensive source dossier records its sparse-grid behavior: `renum_world`
  prunes those cells instead of synthesizing rooms. Keep that finding distinct
  from newly discovered route gaps.
- `mobs_underdark` has 94 positive missing targets; 18 target VNUMs appear in
  the unregistered `ud` WLD source. Treat it as a source-generation/replacement
  question, not a complete travel graph.
- `connectorzones` has 34 positive missing targets that appear in unregistered
  legacy WLD files. Its local filename describes connector content, so each
  intended destination needs a current-owner decision.
- `potions` has six authored directions and `realm` one authored direction to
  room 25087, which appears only in `brass-old-2`. The exact entries remain in
  the unresolved register; the 2025 Brass history is in
  [`DUPLICATE_AND_REPLACED_ZONES.md`](DUPLICATE_AND_REPLACED_ZONES.md).
- `ship`, `vehicles`, `guildhalls`, `buildings`, and `lockers` include template
  or utility rooms without ordinary geographic exits. Vehicle/ship placement
  and movement are separate systems. A zero cross-area exit count is not a
  reachability conclusion for these inputs.
- `ceopast` and `ceofutur` are separately named phases with the same zone title;
  any time-transition controller is outside the ordinary exit graph. The Past
  Ceothia supplement confirms no ordinary cross-zone WLD declaration and no
  source-only WLD target or source-room overlap for its active rooms; its
  Present/Future portals remain separate typed arrivals
  ([Past Ceothia audit supplement](PAST_CEOTHIA_AUDIT_REVIEW.md)).

## Remaining connectivity work

The exact WLD exit census is complete for the frozen full-area source list.
Remaining manual review must trace current `.zon` door resets, secret/revealed
edges, switches and object effects, coded portals/teleports, time/instance
transitions, ships and vehicles, fall/current hazards, and actual return paths.
For each geographic source area, that review must record intended entry points
and meaningful player routes without assuming all rooms are public, safe,
repeatable, or reachable from a universal starting room. The 220 existing
comprehensive source dossiers and twenty-four new follow-up reviews at the
original Checkpoint 2 endpoint provided bounded story evidence for 244 active
keys, but did not establish dynamic reachability. Subsequent per-zone work now
links every active contributor to a local source dossier or audit supplement
in the current coverage register (Checkpoint 939). This is a file-reference
measure and does not claim equal story depth. All active contributors still
need the manual route and behavior review above. At the checkpoint 22 endpoint,
Llyrath added no source-only WLD inbound records and the curated count was 302.
The checkpoint 23 Mistywood
supplement added 53; the checkpoint 24 Darkfall supplement added two.
Checkpoint 25's Torture review found no source-only inbound WLD record; its
paired Ghore boundary was already in the active exit ledger. Checkpoint 26's
Kimordril supplement adds two, bringing the curated count to 359.
Checkpoint 27 adds one source-only `surf.wld` record into Bugentol, bringing
the curated count to 360.
Checkpoint 28 scanned all 95 unregistered WLD files for Jotunheim and found no
targets in rooms 96000–96295. Checkpoint 29 adds two ud_map2 records into
Ixarkon room 96492, bringing the curated count to 362.
Checkpoint 30 scanned all 95 unregistered WLD files for Dirk’nspire and found
no targets in rooms 96800–96885; the curated count remains 362.
Checkpoint 31 adds two `surf.wld` records and one `underdark2011.wld` record
into Troll Caves rooms 96900 and 96903, bringing the curated count to 365.
Checkpoint 32 adds one record apiece from `surf.wld` and `underdark2011.wld`
into IceCrag room 97001, bringing the curated count to 367.
Checkpoint 33 adds one record apiece from `surf.wld` and `underworld2.wld`
into Cloister room 67100, bringing the curated count to 369.
Checkpoint 34 adds one `surf.wld` north exit into Turolopolis room 7102,
bringing the curated count to 370.
Checkpoint 35 adds three `surf.wld` and two `oldmap8.wld` exits into Du'Maathe
Castle rooms 37409, 37416, and 37425, bringing the curated count to 375.
Checkpoint 36 adds seven `surf.wld` exits into Tundra rooms 13703, 13712,
13778, 13796, 13870, 13895, and 13936, bringing the curated count to 382.
Checkpoint 37 adds one `surf.wld` south exit into The Fields Between room
71001, bringing the curated count to 383.
Checkpoint 38 adds one `surf.wld` west exit into The Town of Moregeeth room
70198, bringing the curated count to 384.
Checkpoint 39 adds one `surf.wld` north exit into Ceothia room 80800, bringing
the curated count to 385. Checkpoint 40 adds 192 `Duris3.wld` records targeting
the Braddistock Mansion range, bringing the curated count to 577; 48
source-room VNUMs also overlap the active range and remain unclassified map
records. Checkpoint 41 adds one `surf.wld` east exit into Desert City room
49003, bringing the curated count to 578; the legacy record remains
source-only. Checkpoint 42 confirms no unregistered-WLD target or source-room
VNUM overlap with Past Ceothia's exact room set; the curated count remains 578.
Checkpoint 43 adds one unregistered `surf.wld` down exit from room 229308 into
Basin Wastes room 34000, with no source-room VNUM overlap; the curated count
rises to 579. Checkpoint 44 adds one unregistered `surf.wld` north exit from
room 276166 and one `underworld2.wld` north exit from room 422044 into active
Crypt room 14300. Neither source has a room VNUM in Crypt's active range; both
records remain source-only, raising the curated count to 581.
Checkpoint 45 adds one north exit from unregistered `surf.wld` room 205053 into
active Val room 38575, with no source-room VNUM overlap. It remains source-only
and raises the curated count to 582.
Checkpoint 46 adds one north exit from unregistered `surf.wld` room 214566 into
active Harrow room 29400, with no source-room VNUM overlap. It remains
source-only and raises the curated count to 583. The 110 other numeric target
matches originate in five unregistered maps with Harrow-range source-room
collisions; map ownership remains unresolved.
Checkpoint 47 adds three source-only `surf.wld` exits into Mountain Tracts
rooms 20951, 21110, and 20901. No unregistered source room overlaps Mountain's
active room VNUMs; the curated count rises to 586.
Checkpoint 48 found no source-only WLD target or source-room overlap with The
Orcish Slave Camp; the curated count remains 586.
Checkpoint 49 found no source-only WLD target or source-room overlap with The
Underground Lava Caves; the curated count remains 586.
Checkpoint 50 found no source-only WLD target or source-room overlap with The
Cimmerian Nomad Encampment; the curated count remains 586.

### Ghore code-defined route candidates

Ghore's source review found two routes outside the static WLD graph. Ixarkon's
active veil object #96402 is reset in room #96524 and assigned
`illithid_teleport_veil`; that handler's random destination pool includes Ghore
room #11545. Ghore guard #11542 uses `ghore_paradise` to gate the up passage
from #11666 and place a qualifying visitor in #11667. These are static source
routes, not live-tested travel, and do not change the WLD cross-zone census.
The Ghore review also records unbound `guild_guard` code cases for rooms #11603,
#11619, #11633, #11685, and #11812, plus the paired WLD boundaries to Torture,
Troll Hills, and Shafts. The cases are not counted as active class restrictions
because no Ghore mobile is bound to that procedure. Chief Kerzt #11528 and guard
#11552 load into #11685, but #11552 has no `guild_guard` assignment; no M resets
place a guard in the other case rooms.
See [GHORE_STORY_REVIEW.md](GHORE_STORY_REVIEW.md) for exact source links and
conditions.

### Nakral's Crypt code-defined `shift_prime` destination

The Phantom racial innate registers `shift_prime`; its handler requires the
character to be outside combat, in the Astral zone, and past the innate
cooldown check. The Phantom destination array includes active Crypt room
#14373, and the handler chooses a valid room from that array before moving the
character. This is a code-defined dynamic route candidate outside the WLD
ledger and curated source-only total. The declaration does not establish
ordinary access, a successful live arrival, or a safe return. See the
[Crypt audit supplement](NAKRALS_CRYPT_AUDIT_REVIEW.md) and
[`do_shift_prime`](../../../src/classes/innates.c#L2005),
[destination list](../../../src/classes/innates.c#L2012),
[Phantom innate registration](../../../src/classes/innates.c#L730).

### Valoisian Castle static chaos-kit references

An exact C/C++ VNUM scan found 13 static equipment-slot values referencing
local Val objects #38444, #38450, and #38455 in 11 `chaos_kit_item` arrays.
This is equipment table data, outside the WLD route census, and does not
establish kit selection or issuance. The bounded foreign typed scan also
records one Verspin reset group for dagger #38421. See the
[Valoisian Castle audit supplement](VALOISIAN_CASTLE_AUDIT_REVIEW.md).

### Harrow static references

The bounded exact scan found one active Tundra story-sidecar contract for
Harrow's Bom painting exchange, 83 static chaos-kit values for seven local
objects, a fishing-pole code reference to object #29439, and the explicit inn
assignment for room #29403. One unregistered `brass-old-1.zon` G reset names
local flute #29479; it remains source-only. See the
[Harrow audit supplement](HARROW_AUDIT_REVIEW.md).

### Mountain Tracts static references

The exact external-source scan found one active Winterhaven QST exchange that
consumes local gloves #20950 and potion #20949, with no other active external
QST, reset, shop, or story-sidecar VNUM references. Three static chaos-kit
values name ordinary boots #20924 and bone leggings #20937. The source map's
active type-25 scan found no foreign fixed destination into Mountain Tracts.
See the [Mountain Tracts audit supplement](MOUNTAIN_TRACTS_AUDIT_REVIEW.md).

### The Orcish Slave Camp static references

The camp has one reciprocal Split Shield WLD pair, already present in the
active ledger. Exact active and unregistered QST, reset, shop, and story-sidecar
scans found no external VNUM references; the bounded C/C++ scan found no
direct literal reference. The source dossier's all-active type-25 scan also
found no foreign fixed destination into a camp room. See the
[Orcish Slave Camp audit supplement](ORCISH_SLAVE_CAMP_AUDIT_REVIEW.md).

### The Underground Lava Caves static references

Lava Caves' 451 resolving exits include one reciprocal Mountain Tracts pair,
already present in the active cross-area ledger. The all-95 unregistered WLD
scan found no source-room overlap or target match. Exact typed scans found the
Ravenloft2 reset that places local oil lamp #35589 inside slab #59045 in room
#59269. They also found unregistered `player_guilds.shp` storing Lava Caves
room #35552 as its shop location. The shop loader's fallback reads every
`areas/shp/*.shp` file when `areas/world.shp` is absent; the record's keeper
#35554 and produced-object #35551 have no matching mobile/object prototypes.
External QST and structured story-sidecar scans found no exact numeric VNUM
reference. See the
[Underground Lava Caves audit supplement](UNDERGROUND_LAVA_CAVES_AUDIT_REVIEW.md).

### The Cimmerian Nomad Encampment static references

Nomads' 124 resolving exits include four reciprocal Surface pairs already
present in the active cross-area ledger. The all-95 unregistered WLD scan found
no source-room overlap or target match. No external QST, active/unregistered
reset, or structured story-sidecar VNUM reference was found. Two unregistered
SHP files, `tharnadia1.shp` and `tharold.shp`, each declare shops in local rooms
#6242 and #6243. Their keeper VNUMs #6025 and #6066 do not resolve to mobile
prototypes. These source-only files can enter the shop-loader fallback when
`areas/world.shp` is absent; the records are not qualified as operating shops.
The code scan also found the existing Githzerai prime-shift candidate room
#6224 and 24 CHAOS starter-table entries for crown #6222. See the
[Cimmerian Nomad audit supplement](CIMMERIAN_NOMAD_ENCAMPMENT_AUDIT_REVIEW.md).

### The Ruins of Undermountain boundaries and references

Undermountain has 1,059 resolving exits across 441 rooms in one weak
component. Room #92501's south exit pairs with the north return from `bs` room
#74045. Room #92518's down exit reaches Underworld #4557 without a direct
reciprocal record; Underworld #4557's up exit reaches paired Kvarkpass
Connector room #54841. The exact active exit rows remain in the cross-area
ledger. The all-95 unregistered WLD scan found no Undermountain room overlap or
target match.

Exact external references include IceCrag's two-wine quest input, twenty
Caertannad resets for Undermountain spider #92051, active Brass reset/shop
references to mithril picks #92046, and FirePlane's reset for Flame of the
North #92121. Unregistered `brass-old-1` and `nexus56` reset/shop data also
reference local prototypes and may enter the shop loader's fallback if
`areas/world.shp` is absent. Avernus code reads mobile #92076. Static records
do not establish current stock, working shops, travel safety, or actual player
arrival. See the
[Undermountain audit supplement](UNDERMOUNTAIN_AUDIT_REVIEW.md).

### Desolate Under Fire's unresolved Surface boundary

Desolate Under Fire has 346 authored exits, with 345 resolved in the current
world lookup. Its one positive-unresolved exit is room #77301 south to
#217609. That number occurs in three unregistered WLD sources: `surf.wld` has a
road whose north exit goes to Desolate room #22200; `surface2011.wld` has a
grassland room whose north exit goes to #217309; and `underdark2011.wld` has a
mossy cave whose north exit goes to #217309. The sources are excluded from
`areas/AREA`, and the normal build prunes this exit. Keep its map owner
unresolved; matching VNUMs do not establish a live route. The all-95 scan found
no source-room overlap or WLD target into Desolate Under Fire rooms.

The exact external typed scan found two SurfaceKeeps `G` resets for local potion
#77366 at room #122629. No external QST, SHP, story-sidecar, or current code
prototype reference was found. The comprehensive dossier records its local
type-25 portal placements and a historical Githzerai shift destination that
was removed in 2010. See the
[Desolate Under Fire audit supplement](DESOLATE_UNDER_FIRE_AUDIT_REVIEW.md).

### Storm Port Stronghold's reciprocal approaches and load hub

Storm Port Stronghold has 130/130 resolving exits across nine local weak
components; the largest has 50 rooms. Three WLD exits pair with Surface:
#22600 north↔#636281 south, #22600 west↔#636680 east, and #22634 east↔#636682
west. Six more exits leave load room #22649 for Desolate, Cerebusp, Mira,
Divine Home, Storm Port, and Jade Mini. None has a static reverse record, and
no active WLD exit enters #22649. The complete rows are recorded in the
[cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L370), with the
six load-room links beginning at [row 373](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L373)
and ending at [row 378](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L378).

One source-only record adds context without changing the active graph:
unregistered `surf.wld` room #285640 exits west to Stronghold room #22634. A
stateful parse of all 95 unregistered WLDs found 2,080,773 exits and 527,791
source rooms with no parse issues or room-VNUM overlap; this was the only
source-only exit target into a Stronghold room. `surf.wld` is excluded from
`areas/AREA`, so this record is not active ingress. See the
[Storm Port Stronghold audit supplement](STORM_PORT_STRONGHOLD_AUDIT_REVIEW.md).

The bounded typed scan also traces two `vehicles.zon` gear resets, foreign
Divine Home, Jade Mini, Mira, and Winterhaven QST references, and local crew,
ship, and master-set code bindings. No SHP prototype match was found. The
comprehensive dossier maps the corresponding story-sidecar references,
foreign recipe ownership, crew payment failure, and set-list discrepancy.

### The Mountain Settlement of the Harpies' Surface boundary

The Harpy Settlement has 357/357 resolving exits in one 161-room weak
component; twelve rooms have fall fields and none has a current field. Its only
active boundary is the reciprocal pair #31100 south↔Surface #620042 north
([outbound row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L434),
[return row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1624)).

All 95 unregistered WLD files parsed without errors or source-room overlap.
One source-only exit, `surf.wld` room #249084 north, targets active Harpy room
#31100. The source is omitted from `areas/AREA`, so this is not active ingress.
It is the 28th reviewed `surf.wld` boundary record and raises the curated
source-only total to 588. Exact scans found foreign QST references to local
egg, plans, and tail objects from Divine Home and Cosmic; no foreign ZON reset
or SHP reference matched. See the
[Harpy audit supplement](HARPYHT_AUDIT_REVIEW.md).

### The Behemoth Herders and four source-only approaches

Herders has 842/842 resolving exits in one 347-room component. Its three
registered cross-area exits each have a reciprocal record: #94300 south to
Surface #594610, #94300 west to Surface #594209, and #94646 west to Underdark
#746503 ([#94300 south](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1132),
[#94300 west](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1133),
[#94646 west](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1134),
[Surface returns](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1562),
[second Surface return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1563),
[Underdark return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1704)). The zone has six
fall fields and 284 current fields; static connectivity does not qualify route
safety.

The all-95 unregistered-WLD scan parsed 2,080,773 exits with no parse issues or
source-room overlap against Herders. Four source-only exits target boundary
room #94300: Surf #285152 east and #285453 north, plus Underdark 2011 #249233
east and #249534 north. All four sources are excluded from areas/AREA, so these
records do not add active ingress. They raise the curated source-only count
from 588 to 592 and the reviewed surf.wld subset from 28 to 30. Exact room and
exit anchors are in the [Herders audit supplement](HERDERS_AUDIT_REVIEW.md).

The bounded typed scan found only Shipyards' QST ring #94307 and matching
sidecar records, six-area foreign reset references to local money pouch #94336
and blood chunks #94381, and direct CHAOS kit tables for #94324/#94396. No
foreign SHP match or additional foreign code literal matched a local Herders
prototype. The same scan confirms Archazel #94364's existing table-driven
epic-teacher binding; see the supplement and [comprehensive dossier](../zone-stories/THE_BEHEMOTH_HERDERS.md).

### Jotunheim's negative WLD target and typed routes

Jotunheim has 746 authored exits, 745 resolving exits, and one local 296-room
component. It has no active WLD boundary; room #96004's down exit targets
negative VNUM -1. The refreshed all-95 WLD scan found 2,080,773 exits and
527,791 rooms without parse issues or source-room overlap, and no target in
Jotunheim's #96000–#96295 rooms. The curated source-only count remains 592.
Its Astral object portal is a separate authored route, not a WLD edge.

The typed scan found Fearfrost and Mundorno as the two foreign QST consumers,
the active Shabo reset group for #96077/#96078, and two unregistered
brass-old-1 G rows for #96071/#96076. It found no external SHP or mobile
prototype match. The direct code scan also found #96012, the Mistweave mace,
in standard and enhanceable Druid CHAOS starter profiles. Exact recipes,
reset rows, portal fields, code references, title scan, and history are in the
[Jotunheim audit supplement](JOTUN_AUDIT_REVIEW.md) and
[comprehensive dossier](../zone-stories/JOTUNHEIM.md).

### Temple of Flames' Surface pair and source-only approach

Temple has 803/803 resolving exits in one 323-room component, with nine fall
fields and four current fields. Its only active cross-area WLD boundary is the
reciprocal pair #18300 south↔Surface #587801 north ([Temple row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L306),
[Surface return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1543)).

The all-95 unregistered WLD scan found 2,080,773 exits and 527,791 rooms with
no parse issues or source-room overlap. One source-only exit enters Temple
#18300: `surf.wld` room #263869 exits south. Since `surf` is outside
`areas/AREA`, it is not active ingress. The route raises the curated
source-only total to 593 and the reviewed `surf.wld` subset to 31. See the
[Temple audit supplement](TEMPLE_AUDIT_REVIEW.md) for exact room/exit anchors.

The only external typed prototype consumer found was Hall Q223’s child recipe,
which accepts Temple dagger #18309; its Hall sidecar carries the matching
structured material record. No external SHP, reset-role, mobile/object
prototype, or portal-target match was found. The code scan separately found
room #18305 in the Phantom `shift_prime` destination array. That conditional
teleport route is not a WLD connection and does not qualify ordinary access or
safe return. The active title is unique; tracked path history documents the
2011 rename from “Ruins of Desecration” on the same `temple` source path, not a
distinct replaced area.

### Pharr Valley Swamp's reciprocal Pharr Valley boundary

Pods has 754/754 resolving exits in one 375-room component, with twelve fall
fields and twelve current fields. Its sole active cross-area boundary is the
reciprocal pair #28500 west↔Pharr Valley #40389 east ([Pods row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L412),
[Pharr Valley return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L552)). The `pods`
and `pharrvly` keys are distinct areas.

The all-95 unregistered WLD scan found 2,080,773 exits and 527,791 rooms with
no parse issues or source-room overlap, and no target in Pods rooms
#28500–#28874. The curated source-only total remains 593 and the reviewed
`surf.wld` subset remains 31. No source-only ingress was found; the active
edge is the reciprocal pair listed above.

The only external QST consumer is Surfacemini's hermit QA33 recipe for Pharr
ornaments #28552/#28553; its sidecar records the same foreign-owned exchange.
Mntcastl's active story sidecar also points optionally to that same recipe as
an earlier route into its granular-potion story. Surface Keeps' numeric
#28750 hit is `C` currency, not a prototype VNUM. No external SHP, reset-role,
MOB/OBJ prototype, portal-target, or direct code reference into Pods matched.
The normalized active title is unique. Tracked history records same-path key
and chest updates, not a renamed or replaced area. See the
[Pods audit supplement](PODS_AUDIT_REVIEW.md) for exact source references.

### The Citadel and its reciprocal Skulldrach boundary

The Citadel has 475/475 resolving exits across 200 rooms in one local weak
component. Its sole active cross-area boundary is reciprocal: Skulldrach
#12796 north enters Citadel #13000, and Citadel #13000 south returns to #12796
([Skulldrach row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L221),
[Citadel return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L223)).

The all-95 unregistered WLD scan found 2,080,773 exits and 527,791 rooms with
no parse issues or source-room overlap, and no target in Citadel rooms
#13000–#13199. The curated source-only total remains 593 and the reviewed
`surf.wld` subset remains 31. No source-only ingress was found. See the
[Citadel audit supplement](CITADEL_AUDIT_REVIEW.md) for typed references, title,
and path history.


### The Elemental Groves and its separate source-only approach

The Elemental Groves has 384/384 resolving exits across 165 rooms in one local
weak component. Its sole active boundary is reciprocal: Element #3800 east to
Surface #531681, with the west return ([Element row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L48),
[Surface row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1391)).

Separately, unregistered `surf.wld` room #215024 west (`D3`) targets Element
room #3800. The all-95 scan found no source-room overlap and no other
unregistered target into Element. This adds one source-only approach, bringing
the curated total to 594 and the reviewed `surf.wld` subset to 32. See the
[Elemental Groves audit supplement](ELEMENTAL_GROVES_AUDIT_REVIEW.md) for exact
source lines, typed-reference classification, title, and history.

### Temple of the Earth: active boundaries and source-only approaches

Temple of the Earth has 635/636 resolving exits across 198 rooms and two local
weak components; the largest contains 197 rooms. The active Earth #43500 west
exit and Underdark #804038 east exit form a reciprocal pair ([Earth row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L616),
[Underdark row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1720)). Separately, Realm
#14209 down enters Earth #43568 without a reverse WLD exit
([Realm row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L255)). Earth #43571 east retains
a negative target #-1 ([unresolved row](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L215)).

The all-95 scan found two distinct source-only south exits into Earth #43500:
`surf.wld` room #229658 and `underworld2.wld` room #437928. No source room
VNUM overlaps Earth, and no other unregistered exit targets the zone. This
raises the curated source-only total to 596 and the reviewed `surf.wld` subset
to 33. See the [Temple of the Earth audit supplement](TEMPLE_OF_THE_EARTH_AUDIT_REVIEW.md)
for exact lines, typed references, title, and history.

### Githzerai Stronghold and three source-only approaches

Githzerai Stronghold has 919/919 resolving exits across 381 rooms and five
local weak components; the largest contains 194 rooms. Four active exits enter
the area: two from Realm room #14210, one from Winterhaven room #55634, and one
from Surface room #538611. The only active outbound boundary is Githzerai room
#44400 south to Surface #538611, paired with Surface north
([Realm north](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L257), [Realm east](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L258),
[Winterhaven](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L802), [Githzerai south](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L620),
[Surface north](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1410)).

The all-95 scan found three additional source-only approaches, all into room
#44400: north from Surf #242869, north from Underdark 2011 #255552, and east
from Underworld 2 #421439. No unregistered source room overlaps Githzerai.
These raise the curated total from 596 to 599 and the reviewed Surf subset from
33 to 34. The [Githzerai audit supplement](GITHZERAI_STRONGHOLD_AUDIT_REVIEW.md)
records exact lines, typed references, title, and path history.

### The Caverns of the Worms and an unregistered `ud_map2` approach

Worms has 220/220 resolving exits across 100 rooms and one local weak
component. Its two active boundary pairs are Underdark #811708 north↔Worms
#6900 south and Connectorzones #54211 north↔Worms #6984 south
([Worms to Underdark](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L128),
[Worms to Connectorzones](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L129),
[Connectorzones return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L727),
[Underdark return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1727)).

The all-95 scan found one unregistered `ud_map2.wld` exit, room #1124844 east
to Worms room #6900. That source room lies in the same directed 50-exit source
path as #1121446, the target of active Shafts room #10012's still-unresolved
south exit. Because `ud_map2.wld` is absent from the active manifest, the
source path neither resolves that active exit nor adds active Worms ingress.
No unregistered source room overlaps Worms. The curated source-only total rises
from 599 to 600. See the [Worms audit supplement](WORMS_AUDIT_REVIEW.md) for
exact WLD lines, typed overlaps in unregistered KzKrKeep sources, the
conditional shop-loader path, Ixarkon's code route, title, and history.

### Castle Ravenloft: reciprocal boundaries and no source-only approach

Castle Ravenloft has 269 rooms and 689/689 resolving exits. Its local weak-exit
graph has five components (223, 43, 1, 1, and 1 rooms). Six active exits enter:
Barovia #91167 down→Ravenloft #58568, Barovia Continued #58958 south→#58567,
Catacombs #59057 up→#58538, Catacombs #59352 north→#58542, Catacombs #59398
east→#58305, and The Realm #14208 southeast→#58369
([Barovia](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1120),
[Barovia Continued](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L815),
[Catacombs arrivals](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L816),
[Catacombs arrivals](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L817),
[Catacombs arrival](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L823),
[The Realm](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L251)). Three Ravenloft exits
return to the Catacombs or Barovia Continued, closing three reciprocal pairs
([Ravenloft exits](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L811),
[Ravenloft exits](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L812),
[Ravenloft exits](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L813)).

The all-95 unregistered WLD scan found no source-room VNUM overlap and no exit
targeting a Ravenloft room. This checkpoint adds no source-only ingress, so the
curated source-only total remains 600. The [Ravenloft audit supplement](RAVENLOFT_AUDIT_REVIEW.md)
records the typed external references, title check, and tracked source history;
the [coverage row](ZONE_COVERAGE_REGISTER.csv#L248) records the completed exit
census. These findings describe authored source topology, not traversal safety,
door state, or live reset behavior.

### Faang boulder hazard and linked regions

Faang's 1,150 authored exits all resolve. Its local graph has seven weak
components: a 515-room main component, an 18-room illithid enclave, and five
singletons. Seven active WLD boundaries have reverse-direction records,
including connections to Underdark, Surface, Ixarkon, and Connectorzones; the
Faang dossier links every pair to the exact cross-exit ledger rows.

Mobile #15200 is assigned a special procedure that can throw a boulder at a
visible non-evil character below immortal level in target rooms #15253 or
#15302. The source code allows damage, death, and rare stun or knockout
outcomes. Twenty-four rooms have fall fields, thirteen have current fields,
and the steep river/cavern descriptions reinforce those hazards. These are
static source findings; door state, whether a character can safely traverse
or return, and actual behavior remain untested. See
[FAANG_STORY_REVIEW.md](FAANG_STORY_REVIEW.md).

### Nizari Surface route and phrase-open doors

Nizari has 409 resolving exits and one 192-room local weak component. Its one
active WLD boundary is the north exit from Nizari room #40002 to Surface room
#575327, paired with Surface's south return to #40002
([outbound ledger row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L550),
[return ledger row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1497)).
Nizari has no fall or current fields, though its many palace, jail, alley, and
thieves' doors still require source-level state review before route claims.

Nearby name references do not add a WLD edge. Active Basin Wastes room #34008
describes paths from Nizari to the Basin Wastes, but its south exit leads to
local Basin room #34009 ([description](../../../areas/wld/basin_wa.wld#L194),
[exit](../../../areas/wld/basin_wa.wld#L208)). Two unregistered surface-map
records titled “Nizari” (#227488 and #627488) have empty descriptions and four
cardinal links to neighboring VNUMs in their respective map files; those
records do not target active Nizari rooms ([surface2011](../../../areas/wld/surface2011.wld#L577249),
[surface2011-temp](../../../areas/wld/surface2011-temp.wld#L577249)). Treat
these as geographic/map-label evidence, not additional active entry or return
paths. The active Nizari boundary remains the paired Surface link above.

Two route pairs use key value -2. The Sphinx Gate keyword ends in “hi”; the Forty
Thieves vault keyword ends in “jasmine.” The say handler checks the last word
of the exit keyword and unlocks the paired exit when it matches
([Sphinx exit](../../../areas/wld/nizari.wld#L179),
[vault exit](../../../areas/wld/nizari.wld#L1243),
[handler](../../../src/cmd/actcomm.c#L186)).
This explains the static puzzle wiring; it does not qualify live access,
door-reset state, or safe return. See
[NIZARI_STORY_REVIEW.md](NIZARI_STORY_REVIEW.md) for key placements,
typed item dependencies, and builder questions.

### Kvarkpass Connector and its legacy Surface endpoint

Kvarkpass Connector has 566 resolving exits and three local weak components:
220 rooms, a 24-room nursery/cave segment, and isolated room #54833. Ten
active WLD boundaries lead to other active numeric zones; nine have a reverse
edge. Four are paired with Surface, three with Underworld, one with Lab, and
one with Connectorzones. The remaining boundary is the up exit from #54811 to
UDmini #88515, with no reverse-direction record. Thirteen rooms carry fall
fields; none has a numeric current field. The exact outgoing records are in
[the cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L737), and
the five source-only inbound records appear in the table above.

The 2013 WLD history changed room #54804's upward target from #278150 to active
Surface room #629815. The corresponding `underdark2011.wld` exit from room
#278150 to #54804 remains only in an unregistered source file; do not count it
as an active-world entrance. The area review also documents its reset-closed
and secret doors, two shops, and typed prototype references
([Kvarkpass review](KVARKPASS_CONNECTOR_STORY_REVIEW.md)).

### Fort Boyard and the Prison of Fort Boyard

Fort Boyard has 163 resolving exits in one local weak component of 77 rooms.
Three active WLD boundaries leave it, and each has a reverse-direction record:
one to Surface and two to Prisonb. The same three paired records enter it from
those active areas. Its source-only inbound references are separate: room
#205016 in unregistered `surf.wld` and room #417529 in unregistered
`underworld2.wld` both lead north to active room #38100. The latter file also
has a cell titled “FORT BOYARD”; neither source-only record changes the active
WLD census
([Fort Boyard review](FORT_BOYARD_STORY_REVIEW.md),
[`surf.wld`](../../../areas/wld/surf.wld#L105307),
[`underworld2.wld`](../../../areas/wld/underworld2.wld#L10625)).

The tracked 2013 Fortb WLD edit changed room #38100's east target from legacy
room #279361 to active Surface room #562320. The area review also records the
closed/locked storage and gate doors, the secret cellar, a code-assigned crew
hall in the shipping office despite zero SHP records, and imported prototypes
from Prisonb, Heavens, and Wh. The `prisonb` area is a distinct active companion
with two paired WLD connections; no whole-zone replacement is established by
the sources.

### Cityruin and Tharnadian Ruin

Cityruin has 327 resolving exits in one 155-room local weak component. Its
east exit from #66200 pairs with the west return from active Tharnadian Ruin
#5588. A separate west exit from room #262344 in unregistered `surf.wld` points
to Cityruin #66200; it remains a source-only reference, not an active-world
entrance. Applying 48 unconditional door resets leaves 278/326 local exits
already open, non-secret, and non-blocked; strict open-exit reach from the
arrival at #66200 is one room, while raw authored reach is all 155. The gate
reset is closed but unlocked, so this static metric does not establish
inaccessibility. The repository-wide [Cityruin audit supplement](CITYRUIN_AUDIT_REVIEW.md)
records reset-aware access and the active Tharnadian Ruin QST/story references
to Cityruin remains and key; the [detailed source-story review](CITYRUIN_STORY_REVIEW.md)
retains room and quest evidence.

### The Realm of Barovia Continued: reciprocal gates and one-way arrivals

Barovia Continued has 196 rooms and 494/494 resolving exits, with five local
weak components of 163, 30, 1, 1, and 1 rooms. Two exits leave: #58800 south
to Barovia #91162 and #58958 south to Castle Ravenloft #58567. Both are
reciprocal pairs. Five active exits enter: Barovia #91162 north to #58800,
Castle Ravenloft #58567 north to #58958, Catacombs #59356 down to #58958,
Catacombs #59398 north to #58811, and The Realm #14210 down to #58984
([Barovia](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1119),
[Castle Ravenloft](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L813),
[Catacombs arrivals](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L819),
[Catacombs arrival](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L822),
[The Realm](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L261)). The three Catacombs/Realm
arrivals have no reverse WLD exit. The all-95 unregistered WLD scan found no
source-room overlap or target into Barovia Continued, so the curated source-only
total remains 600. See the [Barovia Continued audit supplement](BAROVIA2_AUDIT_REVIEW.md)
and [coverage row](ZONE_COVERAGE_REGISTER.csv#L26). This graph excludes portals,
reset routes, and other dynamic travel; actual reachability and return safety
remain pending.

### Village of Werrun: Surface pair, Wh arrival, and source-only roads

Werrun has 84 rooms and 197/197 resolving exits. Its local weak graph has two
components of 43 and 41 rooms. The only outbound exit is #38300 south to Surface
#527537, which pairs with Surface #527537 north to #38300. Wh #55614 west to
#38316 is the second active inbound and has no reverse WLD exit
([Werrun outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L546),
[Surface return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1377),
[Wh arrival](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L766)).

The all-95 unregistered WLD scan found no source-room VNUM overlap and two
source-only north exits into Werrun room #38300: `oldmap8.wld` room #181462 and
`surf.wld` room #215822
([oldmap8 target](../../../areas/wld/oldmap8.wld#L30698),
[surf target](../../../areas/wld/surf.wld#L331957)). These remain source-only
approaches, separate from the active Surface pair. The curated source-only
total rises from 600 to 602. See the [Werrun audit supplement](WERRUN_AUDIT_REVIEW.md)
and [coverage row](ZONE_COVERAGE_REGISTER.csv#L344). WLD components exclude
mirrors, resets, and other dynamic routes; actual access, return safety, and
the unregistered Nexus56 shop source remain to be qualified.

### The Village of New Hope: paired Surface crossings and no source-only ingress

New Hope has 228 rooms and 550/550 resolving exits in one weak component.
Walkway #89217 east and south lead to Surface #618434 and #618833; both Surface
exits return to #89217, giving two reciprocal active pairs
([New Hope east exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1104),
[New Hope south exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1105),
[Surface west return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1620),
[Surface north return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1621)). The two pairs
are the area's only active cross-area boundaries.

The all-95 unregistered WLD scan found no source-room VNUM overlap and no
source-only exit into New Hope. Its source-only ingress count is zero, so the
curated total remains 602. See the
[New Hope audit supplement](NEWHOPE_AUDIT_REVIEW.md) and
[coverage row](ZONE_COVERAGE_REGISTER.csv#L206). WLD connectivity excludes
scripted death, hidden and locked doors, imported travel, and other dynamic
routes; actual access, return safety, and service availability remain pending.

### Drustl's Yerdonia Enslaved: three reciprocal boundaries and an inactive approach

Raxthan has 100 rooms and 227/227 resolving exits in one weak component.
Three outbound exits connect it to Surface twice and the Underdark once; each
has a reverse-direction record. The all-95 unregistered WLD scan found no
source-room overlap and one inactive `surf.wld` north exit from room #266966
into Raxthan #42980. That source-only reference raises the curated ingress
total from 602 to 603; it is not an active entrance
([Yerdonia audit supplement](YERDONIA_AUDIT_REVIEW.md),
[coverage row](ZONE_COVERAGE_REGISTER.csv#L250)).

The bounded typed scan found one foreign QST item command: Alatorin's Ravi
contract consumes local Bloodbeast heart #42948, with the receipt remaining
Alatorin-owned. No external reset room target, matching mobile/object
prototype, SHP product, or type-25 destination was found. Shared CHAOS kit
reference #42903 and the Astral/race-guarded random `shift_prime` candidate
room #42950 are separate from physical WLD boundaries and local quest receipts.
The title is unique and tracked source history names no distinct replacement;
Raxthan stays **DO NOT MOVE**. Live access, door state, safe return, combat,
and service outcomes remain pending.

The Werrun, New Hope, and Yerdonia checkpoints used repository-source and
generated documentation inputs only.

### The Arcaneum of L'srillizzin: reciprocal Underdark boundary and portal-linked components

The Arcaneum has 150 rooms and 410/410 resolving exits, with local weak
components of 113 and 37 rooms. Its single outbound edge, #402000 north to
Underdark #813342, pairs with the Underdark south exit back to #402000
([Arcaneum](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1315),
[Underdark](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1731)).

The 37-room component is the Plane of Dreams and Nightmares. Two local type-25
objects target across the WLD component boundary: #402037 at #402072 targets
#402073, and #402038 at #402099 targets #402072. These are declared dynamic
portal routes, not WLD exits; successful use and safe return remain separate.
The all-95 unregistered WLD scan found no room overlap or source-only ingress,
so the curated source-only inbound total remains 603. See the
[Arcaneum audit supplement](ARCANEUM_AUDIT_REVIEW.md) and
[coverage row](ZONE_COVERAGE_REGISTER.csv#L158).

Exact external QST item and SHP scans, sidecar VNUM matching, ZON reset-role/
room targets, prototype reuse, and external type-25 targets found no Arcaneum
matches. The source/title history identifies its 2014 zone creation but no
distinct predecessor or replacement; its active manifest disposition stays
**DO NOT MOVE**. Live access, door/reset state, successful portal arrivals,
return safety, and recipient/service availability remain pending.

### Mistywood: active Underworld3 pair and inactive Surface-map references

Mistywood has 203 rooms and 542/580 loader-resolved exits in one weak
component. Its active inbounds are Pineholl #16164 to #95048, Krimman #16486
to #95100, and Underworld3 #30162 to #95045; its only valid active outbound,
#95045 to Underworld3 #30162, is reciprocal
([Pineholl](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L286),
[Krimman](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L293),
[Underworld3 inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L418),
[Mistywood outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1142)). The other 38
authored outbound records target absent old-map rooms and remain unresolved.

The all-95 inactive WLD scan found no source-room overlap and 53
`surf.wld` exits into 27 active Mistywood rooms. They are source-only records,
not active entrances; the curated source-only total remains 603 because these
records were already counted. The [Mistywood audit supplement](MISTYWOOD_AUDIT_REVIEW.md#checkpoint-70-source-revalidation)
contains the grouped records and revalidated dependency findings. The
[coverage row](ZONE_COVERAGE_REGISTER.csv#L182) now distinguishes 53
source-only inbounds from 38 unresolved active outbounds. Static WLD edges do
not establish ordinary player reachability, staging access, or dynamic routes.

### Pharr Valley: reciprocal boundaries and conditional Nexus56 references

Pharr Valley has 189 rooms and 408/408 resolving exits in one weak component.
Its two outbound edges pair with Valley Crushk #2006 and Pharr Valley Swamp
(`pods`) #28500
([Valley Crushk inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L33),
[Pharr Valley return west](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L551),
[Pods inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L412),
[Pharr Valley return east](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L552)).

The all-95 inactive WLD scan found no source-room overlap or exit target into
Pharr Valley, so there is no source-only WLD ingress and the curated total
remains 603. Typed review found two unregistered Nexus56 references to apple
#40208: a shop product and a `G` reset. The shop file is excluded from normal
`AREA` aggregation, but the runtime loader's all-file fallback can read it when
`areas/world.shp` cannot be opened. See the
[Pharr Valley audit supplement](PHARR_VALLEY_AUDIT_REVIEW.md) for the exact
records, fallback source, compiled ship/pet bindings, title, and history.
Pharr Valley remains **DO NOT MOVE**; source references do not establish a
current shop, reset, or player route.

### Tribal Oasis: reciprocal Surface boundary and one inactive map approach

Tribal Oasis has 304 rooms and 685/685 resolving exits in one weak component.
Its one outbound edge to Surface #611692 is reciprocal
([Oasis outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L894),
[Surface return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1602)).

The all-95 inactive WLD scan found no source-room overlap and one `surf.wld`
east exit from room #246122 to Oasis #78000
([source declaration](../../../areas/wld/surf.wld#L967758)). It is source-only,
not active ingress, and raises the curated source-only inbound total from 603
to 604. No external QST item command, SHP product, ZON reset-role target,
story sidecar VNUM, matching mobile/object prototype, or type-25 destination
was found. CHAOS equipment and Bargor's legacy training-table bindings remain
as classified in the [Tribal Oasis dossier](../zone-stories/TRIBAL_OASIS.md).
The normalized active title is unique; bounded WLD/ZON history names no
distinct predecessor or replacement. Oasis remains **DO NOT MOVE**. See the
[repository-wide audit supplement](TRIBAL_OASIS_AUDIT_REVIEW.md) and
[coverage row](ZONE_COVERAGE_REGISTER.csv#L220). Live access, hidden-door
states, training availability, ordinary return safety, and gameplay outcomes
remain separate.

### The Great Realm of Duris: active graph and 70 inactive Surf approaches

Great Realm has 646 rooms and 1,372/1,406 loader-resolved exits. The authored
cross-area boundary contains 105 records: 71 resolve to other active areas and
34 positive targets remain unresolved in legacy WLD sources. Its local weak
exit graph has 44 components (largest 77 rooms); the active ledger records 66
inbounds, 71 resolved outbounds, and 64 reverse records.

The all-95 inactive WLD scan found no source-room VNUM overlap and 70
`surf.wld` exits from 70 source rooms into 67 Great Realm rooms. They are
source-only, not active entrances, and raise the curated source-only inbound
total from 604 to 674 and the reviewed `surf.wld` subset from 35 to 105. Exact
source lines are tabulated in the
[Great Realm audit supplement](CONNECTORZONES_AUDIT_REVIEW.md). The 34
unresolved active outbounds remain separate from these 70 inactive-source
inbounds.

Typed scans found eight foreign QST item commands, ten exact structured
sidecar matches, and one external type-25 target (`shipy.obj` #43145 to room
#54245). The unregistered `surf.zon` contains 209 `M` reset records for 16
local Great Realm mobiles; they are not active resets. The existing dossier
maps the active foreign resets and direct compiled bindings. The active title
is unique and bounded source history names no distinct replacement; Great
Realm remains **DO NOT MOVE**. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L63).

### The Battlefield: reciprocal Surface boundary and one inactive map approach

The Battlefield has 163 rooms and 320/320 resolving exits in four weak
components (largest 134 rooms). Its sole outbound, #66400 south to Surface
#624087, pairs with the Surface north return
([Battlefield](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L831),
[Surface](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1634)).

The all-95 inactive WLD scan found no source-room overlap and one `surf.wld`
west exit from room #279354 into Battlefield #66400
([source declaration](../../../areas/wld/surf.wld#L1665170)). This is
source-only, not active ingress; it raises the curated source-only inbound
total from 674 to 675 and the reviewed `surf.wld` subset from 105 to 106. The
[Battlefield audit supplement](BATTLEFIELD_AUDIT_REVIEW.md) records the exact
source line and direct code bindings.

External QST, SHP, ZON reset-role, exact sidecar, prototype-definition, and
external type-25 scans found no Battlefield target. Four direct compiled
numeric matches are classified in the existing dossier. The normalized title
is unique and bounded history names no distinct replacement; Battlefield
remains **DO NOT MOVE**. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L29).

### The Forgotten Mansion: two active inbounds and two inactive approaches

The Forgotten Mansion has 176 rooms and 390/390 resolving exits in four weak
components (largest 158 rooms). Room #3500 west to Surface #588620 is
reciprocal; White House room #55634 northwest enters #3675 without a recorded
return ([Mansion](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L46),
[White House](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L803),
[Surface](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1548)).

The all-95 inactive WLD scan found no Mansion source-room overlap. One
source-only east exit from `surf.wld` room #275277 and one from
`underworld2.wld` room #436465 target Mansion #3500
([Surf](../../../areas/wld/surf.wld#L1579649),
[Underworld2](../../../areas/wld/underworld2.wld#L148802)). Neither is active
ingress. Together they raise the curated source-only inbound total from 675 to
677 and the reviewed Surf subset from 106 to 107.

The repository-wide typed scan found no active external reset-role, external
prototype, SHP, external QST-item, exact story-sidecar, or type-25 destination
target. Two `G` object-role rows in unregistered `brass-old-1.zon` refer to
Mansion objects #3532 and #3524; they are not active resets. CHAOS kit rows
reference four Mansion objects, a direct callback names room #3600 but has no
active assignment, and Tyrlos's high-drop bonus loop remains commented. The
normalized title is unique; bounded WLD/ZON history names no replacement. The
Mansion remains **DO NOT MOVE**. See the [Mansion audit supplement](MANSION_AUDIT_REVIEW.md)
and [coverage row](ZONE_COVERAGE_REGISTER.csv#L170).

### Woodseer: six reciprocal boundaries and two inactive Surf approaches

Woodseer has 453 rooms and 1,040/1,041 resolving exits in two weak components
(largest 452 rooms). Its active boundary has eight inbounds and seven
outbounds: six reciprocal pairs, two one-way arrivals from Heavens and
Krimman, and an unresolved #16500 up exit to #154581 in unregistered
`Duris3.wld`. The exact active boundaries are recorded in the
[cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L296) and unresolved
exit register ([row 42](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L42)).

The all-95 inactive WLD scan found no Woodseer source-room overlap and two
source-only exits from `surf.wld` room #226868 south and room #228065 east,
targeting Woodseer #16891 and #16915. They raise the curated source-only total
from 677 to 679 and the reviewed Surf subset from 107 to 109. They are not
active ingress; see the [Woodseer audit supplement](WOODSEER_AUDIT_REVIEW.md).

Typed scans classify the active Surface Mini salmon QST/sidecar references and
Alatorin's mailman/dagger reset roles, plus the inactive Solstice Guild Hall
shop/reset references. No external prototype or type-25 target matched.
Compiled CHAOS, justice, mail, hometown/placement, auction, inn/pet, and special
procedure bindings are classified in the supplement and source dossier. The
normalized title is unique, bounded source history identifies no replacement,
and Woodseer remains **DO NOT MOVE**. See the
[coverage row](ZONE_COVERAGE_REGISTER.csv#L349).

### Village of Refugees: two reciprocal Surface pairs and one inactive approach

Village of Refugees has 67 rooms and 142/142 resolving exits in one weak
component. Its two active outbounds from room #98600 to Surface #627239 and
#627640 both have reciprocal returns. The exact pairs are in the
[cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1178).

The all-95 inactive WLD scan found no source-room overlap and one source-only
west exit from `surf.wld` room #264438 into room #98600. This raises the curated
source-only total from 679 to 680 and the reviewed Surf subset from 109 to 110;
the record is not active ingress. See the
[Village of Refugees audit supplement](VILLAGE_OF_REFUGEES_AUDIT_REVIEW.md).

Typed scans classify the Newhaven collar QST/sidecar reference, active Newbie
torch shop/reset and Alatorin horse reset, and unregistered Nexus56 waterskin
shop/reset references. Nexus56's SHP may be read by the shop-loader fallback
when `areas/world.shp` cannot be opened; the current checkout has no generated
`areas/world.shp`, but no operating shop is established. No external prototype,
type-25 target, or direct compiled numeric reference matched. The normalized
title is unique, bounded history identifies no replacement, and Village of
Refugees remains **DO NOT MOVE**. See the
[coverage row](ZONE_COVERAGE_REGISTER.csv#L256).

### The Twin Towers: four reciprocal boundaries, no inactive approach

The Twin Towers has 99 rooms and 211/211 resolving exits in one weak
component. Four active outbounds have reciprocal returns: three connect its
towers and temple with the Twin Towers Forest, and one connects its dug tunnel
to Underworld 3. The exact eight records are in the
[cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L224).

The all-95 inactive WLD scan found no source-room overlap and no source-only
exit into a Twin Towers room. At Checkpoint 78, the cumulative snapshot remained 680 overall / 110 Surf. Those counts are historical; current totals are 11,993 overall / 415 Surf. See the
[Twin Towers audit supplement](TWIN_TOWERS_AUDIT_REVIEW.md).

Typed closure maps four external QST item commands and four exact structured
heart references in the Alatorin, Newhaven, and Werthorm sidecars. No external
shop product, reset-role reference, mobile/object definition, or type-25 portal
target matched. Of 872 external type-25 declarations, 713 are in active object
files and 159 in source-only files. Two direct C/C++ matches bind Oldmonk
#13219 to Jin Touch and check room #13272 for Paladin/Avenger ascension. The
normalized title is unique, bounded file history identifies no replacement,
and Twin Towers remains **DO NOT MOVE**. See the
[coverage row](ZONE_COVERAGE_REGISTER.csv#L317) and the audit supplement.

### The Deep Ravine: six reciprocal boundaries and four inactive Surf approaches

The Deep Ravine has 178 rooms and 404/404 resolving exits in five weak
components (81, 27, 27, 23, and 20 rooms). Six outbounds have reciprocal
returns: four with Surface and two with the Underdark. Their exact records are
in the [cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1135).

The all-95 inactive WLD scan found no source-room overlap and four source-only
exits from `surf.wld`: #209185 north to #94878, #250895 south and #251196 west
to #94721, and #252396 north to #94700. These do not provide active ingress.
They raise the curated source-only total from 680 to 684 and the reviewed Surf
subset from 110 to 114. See the
[Deep Ravine audit supplement](DEEP_RAVINE_AUDIT_REVIEW.md).

Typed closure maps 37 active foreign reset commands importing local mobiles
and objects, plus five additional source-only Brass reset rows. Alatorin's
Q7425 and sidecar return alternate symbol #94726; no foreign shop product,
prototype definition, or type-25 target matched. The external type-25 scan
covered 705 active declarations and 159 source-only declarations. Six direct
C/C++ references are chaos equipment entries for #94726. The normalized title
is unique, bounded history includes the current 2026 source edits, and the area
remains **DO NOT MOVE**. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L177)
and audit supplement.

### Neverwind Valley: three reciprocal boundaries and one inactive Surf approach

Neverwind Valley has 199 rooms and 555/555 resolving exits in one weak
component. Three outward boundaries have reciprocal active returns with
Surface; Connector Zones room #53725 also enters #20466 without a reverse
local edge. See the [cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L336)
and [Neverwind Valley audit supplement](NEVERWIND_VALLEY_AUDIT_REVIEW.md).

The all-95 inactive WLD scan found no source-room overlap and one
source-only `surf.wld` exit: room #235767 north to Neverwind Valley #20400. It is
not active ingress. The curated source-only total is now 685 and the reviewed
Surf subset is 115; see the [Surf component row](ZONE_COVERAGE_REGISTER.csv#L448)
and audit supplement.

Typed scans found no foreign reset, QST consumer, shop product, prototype
definition, sidecar reference, or type-25 destination. One direct code match
is the computed Githzerai arrival at room #20484. The normalized title is
unique, bounded history identifies no replacement, and the area remains
**DO NOT MOVE**. See the audit supplement for source lines and scope limits.

### Grumbar's Domain: active and source-only boundaries

Grumbar's Domain has 199 rooms and 794/794 resolving exits in one weak
component. One outbound to the Elemental Plane of Earth has a reciprocal
return; two additional active plane exits enter room #131364 without local
reverse edges. See the [cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1299)
and [Grumbar's Domain audit supplement](GRUMBARS_DOMAIN_AUDIT_REVIEW.md).

The all-95 inactive WLD scan found 400 non-overlapping `Duris3.wld` map exits
to all 199 active rooms, in two 199-edge bands and two endpoint records. The
same file also contains 199 same-numbered generic “The Ocean” room records
with different identities and topology. These declarations are source-only,
not active ingress. They raise the curated source-only total from 685 to 1085;
the reviewed Surf subset remains 115. See the
[Duris3 component row](ZONE_COVERAGE_REGISTER.csv#L387) and audit supplement.

Typed and compiled scans found no external exact prototype or portal target;
the sole direct C/C++ match is the already-documented purple-worm binding.
The normalized title is unique, and bounded history identifies no replacement.
Grumbar's Domain remains **DO NOT MOVE**.

### The Temple to Skrentherlog: one reciprocal village pair and one one-way inbound

The Temple to Skrentherlog has 56 rooms and 104/104 resolving exits in a
55-room component plus isolated room #80599. Two active inbounds and one
outbound include a reciprocal boundary with the Village of the Damned; the
other inbound is Winterhaven #55632 up to #80545. See the
[cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L783) and
[Temple audit supplement](TEMPLE_TO_SKRENTHERLOG_AUDIT_REVIEW.md).

The all-95 inactive WLD scan found no exit into an active Temple room and no
source-room VNUM overlap. The curated source-only total remains 1085 and the
reviewed Surf subset remains 115. Typed scans classify two active Astral-Tiamat
coin resets for local object #80571, with no external QST consumer, shop
product, sidecar reference, prototype definition, or type-25 target. The three
direct object-function bindings are already in the source dossier. Title and
bounded history identify no replacement; the area remains **DO NOT MOVE**.
See the audit supplement for exact references and limits.

### Caves of Mt. Skelenak: five reciprocal boundaries and source-only Surf approaches

Caves of Mt. Skelenak has 152 rooms and 323/324 exits resolving to active rooms
in three weak components of 124, 22, and 6 rooms. Five active inbounds and six
resolved outbounds form five reciprocal pairs plus one additional one-way exit:
#4130 east reaches Surface #556876, whose return edge is to #4131. The #4111 east
exit to #153645 remains unresolved in the active world; that VNUM appears only
in inactive `Duris3.wld`. See the [cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L49),
[unresolved register](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L6), and [Caves audit
supplement](CAVES_OF_MT_SKELENAK_AUDIT_REVIEW.md).

The all-95 inactive-WLD scan found three non-overlapping source-only `surf.wld`
exits into Caves rooms #4130, #4001, and #4131. They are map records, not active
ingress. The curated source-only total is now 1088 and the reviewed Surf subset
is 118. The aggregate inactive-WLD reference field records these three Surf
approaches plus the separate unresolved Duris3 target match. See the [Caves
coverage row](ZONE_COVERAGE_REGISTER.csv#L47), [Surf component row](ZONE_COVERAGE_REGISTER.csv#L448),
and audit supplement. Typed scans found no external exact QST, shop, sidecar,
prototype, or type-25 destination; direct code matches and bounded title/history
findings are recorded in the supplement. The area remains **DO NOT MOVE**.

### Southern Coastal Highway: five reciprocal boundaries and a dropped negative exit

Southern Coastal Highway has 539 rooms and 1,410/1,411 exits resolving to active
rooms in components of 533 and 6. Five active outbound and five inbound
boundaries make five reciprocal pairs with Mir, Bastine, and Surface. Exit
#41488 down to #-1 is a negative sentinel that the unresolved register says is
dropped before room renumbering; it is not a positive unresolved destination.
See the [cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L559),
[negative-exit record](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L210), and [Highway
audit supplement](SOUTHERN_COASTAL_HIGHWAY_AUDIT_REVIEW.md).

The all-95 inactive-WLD scan found two non-overlapping source-only `surf.wld`
exits into rooms #41837 and #41732. They are not active ingress. The curated
source-only total is now 1090 and the reviewed Surf subset is 120. See the [Surf
component row](ZONE_COVERAGE_REGISTER.csv#L448) and audit supplement.

Typed scans classify ten active Bastine QST item commands, seven Bastine
sidecar item references, and an active Alatorin reset for Highway horse #41362.
No external shop product, prototype definition, or type-25 destination matched.
The direct Githzerai shift destination and other code references are classified
in the supplement. The title is unique, bounded history identifies no
replacement, and the area remains **DO NOT MOVE**.

### Labyrinth of No Return: one reciprocal Mountain Tracks boundary

Labyrinth of No Return has 296 rooms and 602/602 resolving exits in one weak
component. Its sole active boundary is room #5000 up to Mountain Tracks #20971,
with a reciprocal down return. See the
[cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L87) and
[Labyrinth audit supplement](LABYRINTH_OF_NO_RETURN_AUDIT_REVIEW.md).

The all-95 inactive-WLD scan found no source-room overlap and no source-only
approach into Labyrinth. The curated source-only total remains 1090 and the
reviewed Surf subset remains 120. Typed scans classify Tower's three-item
Smedgewack return, the dormant duplicate Knight QST sequence, Caertannad's four
web resets, Troll Caves' guano reset, and the static compiled references. No
external typed shop product, prototype definition, or type-25 destination
matched. The title is unique, bounded history identifies no replacement, and
the area remains **DO NOT MOVE**.

### Khildarak Stronghold: three reciprocal boundaries and a one-way inbound

Khildarak Stronghold has 751 rooms and 1,582/1,582 resolving exits across three
internal weak components (749/1/1); the two singleton rooms are #17303 and
#17592. Three cross-area exits are reciprocal with Black Tunnels #10721,
Mushroom Caverns #24188, and Mushroom Caverns #24108. A fourth active inbound
edge comes from Heavens #32 south to Khildarak #17087 and has no local reverse.
There are no negative or positive unresolved exits, 51 fall-chance rooms, and
no current fields. See the [cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L302)
and [Khildarak audit supplement](KHILDARAK_STRONGHOLD_AUDIT_REVIEW.md).

The all-95 inactive-WLD scan found no source-room overlap. One source-only
`ud_map2.wld` exit from #1124854 up to Khildarak #17000 is not active ingress;
the curated source-only total is now 1091 and the reviewed Surf subset remains
120. Typed scans classify five active Alatorin mobile reset records, source-only
Nexus56 and player-guild shop product matches, QST reward-number collisions,
and the static compiled destinations and bindings. No external QST item
consumer, mobile/object prototype, active type-25 destination, or structured
sidecar item reference matched. The normalized title is unique, bounded history
identifies no replacement, and Khildarak remains **DO NOT MOVE**. See the
[coverage row](ZONE_COVERAGE_REGISTER.csv#L144), [ud_map2 row](ZONE_COVERAGE_REGISTER.csv#L465),
and [audit supplement](KHILDARAK_STRONGHOLD_AUDIT_REVIEW.md).

### Strathor Valley of the Storm Giants: two reciprocal Surface boundaries

Strathor Valley of the Storm Giants has 136 rooms and 297/297 resolving exits
in one weak component. Its two cross-area exits are reciprocal with Surface:
#30800 south to #600071 and #30929 north to #599271. There are no negative or
positive unresolved exits, fall-chance fields, or current fields. See the
[cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L432) and
[Strathor audit supplement](STRATHOR_VALLEY_AUDIT_REVIEW.md).

The all-95 inactive-WLD scan found no source-room overlap and one source-only
`surf.wld` approach from room #216305 north to Strathor #30800. It is not active
ingress; the curated source-only total is now 1092 and the reviewed Surf subset
is now 121. Typed scans classify one active Alatorin reset of local object
#30820 and the compiled quest-handler and ferry-stop references. No external
QST item consumer, shop product, mobile/object prototype, type-25 destination,
or structured sidecar item reference matched. The normalized title is unique,
bounded history identifies no replacement, and Strathor remains **DO NOT
MOVE**. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L283) and [audit
supplement](STRATHOR_VALLEY_AUDIT_REVIEW.md).

### The Gagga'Jobo Cave System: one reciprocal Underdark boundary

The Gagga'Jobo Cave System has 23 rooms and 45/45 resolving exits in one weak
component. Its sole active boundary is room #19000 west to Underdark #842047,
with a reciprocal east return. There are no negative or positive unresolved
exits, fall-chance fields, or current fields. See the
[cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L309) and
[Gagga'Jobo audit supplement](GAGGA_JOBO_CAVE_SYSTEM_AUDIT_REVIEW.md).

The all-95 inactive-WLD scan found no source-room overlap and no approach into
the cave. The curated source-only total remains 1092 and the reviewed Surf
subset remains 121. Typed scans found no external QST item, shop product,
mobile/object prototype, type-25 destination, or structured sidecar item
reference. The compiled scan classifies the dynamic Ixarkon veil destination
at room #19022; it is not an ordinary WLD boundary or guaranteed route. The
normalized title is unique, bounded history identifies no replacement, and
the area remains **DO NOT MOVE**. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L110)
and [audit supplement](GAGGA_JOBO_CAVE_SYSTEM_AUDIT_REVIEW.md).

### The Tharnadian Ruin: three reciprocal boundaries and no inactive approach

The Tharnadian Ruin has 179 rooms and 371/371 resolving exits in weak
components of 168, 10, and 1 room; the singleton is #5592. Its three active
boundaries pair with Caertannad, Cityruin, and Braddistock Mansion. There are
no negative or positive unresolved exits, one fall-chance field at #5651, and
no current fields. See the [cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L90)
and [audit supplement](THARNADIAN_RUIN_AUDIT_REVIEW.md).

The all-95 inactive-WLD scan found no source-room overlap and no approach into
the Ruin. The curated source-only total remains 1092 and the reviewed Surf
subset remains 121. Typed scans classify two Caertannad sidecar giver
references and four active Heavens resets for local key #5504; there is no
external QST item command, valid SHP product, prototype definition, type-25
destination among 871 declarations (712 active, 159 source-only), or sidecar
`item_vnums` hit. The compiled scan records the
dynamic Tharnadian portal destination #5569/#5583 and active mobile/object
bindings; old Verzanan candidates are compiled out. The normalized title is
unique, bounded history identifies no replacement, and the area remains
**DO NOT MOVE**. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L298) and
[audit supplement](THARNADIAN_RUIN_AUDIT_REVIEW.md).

### The Forest City of Aravne: one reciprocal Surface pair and one Surf approach

The Forest City of Aravne has 438 rooms and 923/923 resolving exits in one
weak component. Its Surface boundary #21500 south to #620880 has a reciprocal
north return; Winterhaven #55632 adds a separate inbound to #21937. There are
no negative or positive unresolved exits, one fall-chance field, and no
current fields. See the [cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L359)
and [audit supplement](FOREST_CITY_OF_ARAVNE_AUDIT_REVIEW.md).

The all-95 inactive-WLD scan found no source-room overlap and one inactive
`surf.wld` approach, room #248234 north to #21500. It is not active ingress;
the curated source-only total is now 1093 and the reviewed Surf subset is 122.
Typed scans classify the Airship Grave sand QST, Nax shop product #21585, and
three foreign object resets. No external mobile/object prototype or type-25
target into Aravne matched; Airship Grave has the only structured sidecar
references. The compiled scan distinguishes the active death callback and
dynamic epic teacher from static CHAOS profiles and a commented high-drop
consumer. The normalized title is unique, bounded history identifies no
replacement, and the area remains **DO NOT MOVE**. See the
[coverage row](ZONE_COVERAGE_REGISTER.csv#L298) and
[audit supplement](FOREST_CITY_OF_ARAVNE_AUDIT_REVIEW.md).

### The Great Shaboath: three active inbounds and an isolated room

The Great Shaboath has 116 rooms and 243/243 resolving exits in components of
115 rooms and one isolated room, Shaboath Control Room #32800. ConnectorZones
#54012 north to #32803 and Winterhaven #55633 southeast to #32916 are
inbound-only. Underdark #710327 north to #32919 pairs with Shaboath #32919
south to #710327. There are no negative or positive unresolved exits, one
fall-chance field, and no current fields. See the [cross-area register]
(WORLD_CROSS_AREA_EXIT_REGISTER.csv#L439) and [audit supplement]
(GREAT_SHABOATH_AUDIT_REVIEW.md).

The all-95 inactive-WLD scan found no source-room overlap or approach; the
curated source-only total remains 1,093 and the reviewed Surf subset remains
122. Typed scans found two active foreign resets for local objects #32856 and
#32857, and no external QST item command, SHP product, prototype, type-25
destination, or structured sidecar reference. The compiled scan classifies
active literal and computed bindings, local procedure references, CHAOS
profiles, and the holy-weapon constant. The normalized title is unique, bounded
history identifies no replacement, and the area remains **DO NOT MOVE**. See
the [coverage row](ZONE_COVERAGE_REGISTER.csv#L60) and [audit supplement]
(GREAT_SHABOATH_AUDIT_REVIEW.md).

### The Altar of the Firesworn: a separate dream component and Duris3 map links

The Altar has 71 rooms and 259/259 resolving exits in components of 44 and 27,
with no active WLD inbounds or outbounds, negative or unresolved exits,
fall-chance fields, or currents. The 27-room component is #135144–#135170; the
44-room component contains #135101–#135143 and #135171. The local portal from
Maelshem at #135131 to dream room #135157 crosses the WLD components; the
projection portal from #135154 targets #135171 in the larger component. These
are object routes declared inside Firesworn, not external static entrances.

The all-95 inactive-WLD scan found 284 target records in `Duris3.wld`. It
contains 71 same-numbered source rooms (#135101–#135171), whose 140 target
records are excluded from the source-only count. The remaining 144 exits come
from two 71-record map bands (#134801–#134871 south and #135401–#135471 north)
plus two endpoint exits (#135100 east and #135172 west). Together they target
all 71 Firesworn rooms. `Duris3.wld` is not in the active `areas/AREA` list, so
these are map-source declarations, not active ingress. They bring the curated
source-only total to 1237; the reviewed Surf subset remains 122. See the
[inactive-source evidence](ALTAR_OF_THE_FIRESWORN_AUDIT_REVIEW.md#inactive-wld-sources).

Typed closure found one Heavens bookcase reset for local object #135129 and no
external QST item command, SHP product, mobile/object prototype, type-25
destination, or structured sidecar reference. The namespace-aware compiled
scan found no exact local C/C++ VNUM match. The normalized title is unique,
bounded history identifies no replacement, and the area remains **DO NOT
MOVE**. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L97) and
[audit supplement](ALTAR_OF_THE_FIRESWORN_AUDIT_REVIEW.md).

### The Royal Mausoleum of Castle IceCrag: reciprocal IceCrag boundary and Duris3 map links

The Royal Mausoleum has 133 rooms and 286/286 resolving exits in one weak
component: 285 internal exits and one cross-area exit. Its only active WLD
boundary is the reciprocal IceCrag pair: room #142400 south to IceCrag #97132,
and IceCrag #97132 north to #142400. There are no negative or unresolved
targets, range mismatches, or currents; the cliff at #142449 is the one
fall-chance room. These are authored WLD relationships; successful entry,
door/trap state, route safety, and return remain separate.

The all-95 inactive-WLD scan found 532 target records in `Duris3.wld`. Its 133
same-numbered source rooms (#142400–#142532) account for 264 records and are
excluded from the curated source-only set. The remaining 268 map exits cover
all 133 active rooms: 133 south exits from #142100–#142232, 133 north exits
from #142700–#142832, and two endpoint exits from #142399 east and #142533
west. `Duris3.wld` is not in the active `areas/AREA` list, so these are
source-only declarations, not active ingress. They bring the curated total to
1505; the reviewed Surf subset remains 122. See the
[inactive-source evidence](ROYAL_MAUSOLEUM_OF_CASTLE_ICECRAG_AUDIT_REVIEW.md#inactive-wld-sources).

Typed closure found no external QST item command, SHP product, ZON reset-role
consumer, external prototype, or type-25 destination among 872 external
declarations (713 active, 159 source-only). All 231 external story sidecars
parsed without errors and have no structured local VNUM. The direct C/C++
source scan found 24 chaos-equipment entries referencing eight local equipment
objects, with no direct local room or mobile VNUM. The normalized title is
unique, followed WLD/ZON history identifies no distinct replacement, and the
area remains **DO NOT MOVE**. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L340)
and [audit supplement](ROYAL_MAUSOLEUM_OF_CASTLE_ICECRAG_AUDIT_REVIEW.md).

### Negative Material Plane: five components, two one-way inbounds, and external dependencies

Negative Material Plane has 270 rooms and 999/999 resolving exits, all local
to the area, in weak components of 168, 27, 25, 25, and 25 rooms. The four
smaller components are the Dust, Salt, Ash, and Vacuum planes; their dynamic
LICK and other travel mechanisms are separate from WLD connectivity. Realm
#14210 up to #26852 and Winterhaven #55633 south to #26860 are the only active
WLD inbounds, both one-way. There are no WLD outbounds, unresolved targets,
negative targets, range mismatches, or unregistered-WLD references. Two rooms
have F100 fall-chance fields; there are no currents.

The all-95 inactive-WLD scan found no source-room overlap or approach, so the
curated source-only total remains 1,505 and the reviewed Surf subset remains
122. Typed closure found nine external QST consumers of orb #26614, one
Alatorin reward of boots #26631, and five active foreign reset records for
local objects #26661, #26665, and #26666. One of 856 external type-25
declarations targets room #26600: active Ravenloft2 portal #59087 in room
#59169. No external SHP product or prototype matched. All 231 external story
sidecars parsed; deeper token scanning found 40 numeric-token occurrences
across ten sidecars, mostly mirrored completion and step identifiers for
#26614, #26631, and #26642 dependencies.
The C/C++ scan classified 30 exact-number match lines across nine files.
The title is unique, followed history identifies no distinct predecessor or
replacement, and the current `areas/AREA` row remains **DO NOT MOVE**. See the
[audit supplement](NEGATIVE_MATERIAL_PLANE_AUDIT_REVIEW.md),
[coverage row](ZONE_COVERAGE_REGISTER.csv#L200), and
[cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L260).

### The Prisons of Carthapia: one reciprocal Surface pair, 35 one-way exits, and a source-only map approach

The Prisons of Carthapia has 199 rooms and 455/455 resolving exits in weak
components of 166, 17, and 16 rooms. Surface #636262 north to Prison #7497
has a reciprocal south return and is the only active inbound. Thirty-five
additional outbound exits leave seven planar rooms in groups of five for
active destinations #23255, #25504, #12544, #19728, #24439, #51032, and
#23905; none has an authored WLD return to Prison. The two falls are F60 at
#7382 and F80 at #7383; no room has a current field. Fixed portals and the
custom Nexus handler create movement beyond this static graph.

The all-95 inactive-WLD scan found one non-overlapping `surf.wld` approach:
room #263479 north to #7497. It is a source-only map declaration, not active
ingress. The curated source-only total is now 1,506 and the reviewed Surf
subset is 123. Typed scans found no external QST item command, 13 active ZON
reset-role references for Prison prototypes, two source-only Nexus56 ZON
references, and one source-only Nexus56 SHP product. None of 862 external
type-25 declarations targets Prison; no external prototype definition or
structured sidecar VNUM matched. Three deeper sidecar token matches occur
inside opaque identifiers. The C/C++ scan returned 12 match lines across
eight files; contextual review identifies Warden, Axe, Nexus, summon-helper,
and training-table references, with other matches classified as comments,
hashes, or numeric constants.

The normalized title is unique, followed WLD/ZON history names no distinct
predecessor or replacement, and the active `areas/AREA` row remains **DO NOT
MOVE**. The [audit supplement](PRISONS_OF_CARTHAPIA_AUDIT_REVIEW.md),
[coverage row](ZONE_COVERAGE_REGISTER.csv#L240),
[active cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1665), and
[source-only Surf evidence](../../../areas/wld/surf.wld#L1332043) record the
details. The existing dossier retains the story and its 12 builder/capability
follow-ups; doors, resets, dynamic travel, safety, and played outcomes remain
separate.

### Pits of Cerberus: six authored components, reciprocal Surface routes, and source-only map approaches

Pits of Cerberus has 156 rooms and 323/323 resolving exits in weak components
of 51, 36, 35, 20, 13, and 1 room. Five active WLD exits enter: three from
Surface, each paired with a reciprocal outbound exit, plus one-way inbounds
from MIRA and Spshold load-network rooms. Five rooms have fall-chance fields
and none has a current field. Eight fixed local ENTER portals are outside
this static WLD graph.

The all-95 inactive-WLD scan found three non-overlapping approaches:
`surf.wld` #218489 south to #22118 and #219089 north to #22005, and
`underworld2.wld` #427709 east to #22118. They are source-only declarations,
not active ingress. The reviewed Surf subset is now 125 and the curated
source-only total is 1,509. Typed scans found three active external QST
commands, one SHP product, three active ZON reset-role references, and two
active type-25 portal objects targeting local room #22053. All 231 external
story sidecars parse; two have typed item-VNUM matches, and 34 C/C++ match
lines across seven files were context-classified. See the [Pits audit
supplement](PITS_OF_CERBERUS_AUDIT_REVIEW.md), [coverage
row](ZONE_COVERAGE_REGISTER.csv#L54), and [cross-area
register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L360).

Local `cerebusp.zon` also resets four imported objects (#358, #55452, #40774,
and #76713). Local QST item commands, SHP products, and typed sidecar
`item_vnums` add no outbound prototype references.

The title is unique, followed history identifies no distinct predecessor or
replacement, and the active `areas/AREA` row remains **DO NOT MOVE**. Doors,
resets, live travel, safety, and played qualification remain separate.

### Kimordril: one authored component and two already-counted Surf approaches

Kimordril has 159 rooms and 351/351 resolving active exits in one weak
component. Three Kimordril exits lead to Surface rooms, each with a reciprocal
Surface return. The all-95 inactive-WLD scan reconfirmed two non-overlapping
`surf.wld` approaches to rooms #95502 and #95655; these were already recorded
in Checkpoint 26 and do not change the reviewed Surf subset of 125 or curated
source-only total of 1,509. The parallel same-numbered rooms in
`surface2011.wld` and `underdark2011.wld` do not lead to active Kimordril.

Typed scans found two active Alatorin QST item commands, one active
Player Castles shop with two products, twelve active foreign reset-role rows
across three zones, and no external type-25 portal target. Local resets and
stock also depend on imported objects. Source-only Nexus56 resets/stock and
Player Guilds stock reference Kimordril objects and need to remain visible in
the candidate plans because the shop loader has a conditional all-file
fallback. See the [Kimordril audit supplement](KIMORDRIL_AUDIT_REVIEW.md),
[coverage row](ZONE_COVERAGE_REGISTER.csv#L146), and the [Nexus56](ATTIC_MIGRATION_PLAN.md#L40)
and [Player Guilds](ATTIC_MIGRATION_PLAN.md#L29) candidate plan rows.

Kimordril remains **DO NOT MOVE**. Static exits and fallback source records do
not establish live player access, an operating shop, or safe return.

### Shady Grove: paired Surface routes, Heavens arrivals, and inactive approaches

Shady Grove has 258 rooms and 561/561 resolving active exits. Its undirected
authored graph contains a 257-room weak component plus isolated pet-shop stock
room #97758. Three exits to Surface are reciprocated; two additional active
Heavens exits arrive at inn #97663 without a reciprocal Shady WLD exit. The
coverage census records three fall-chance markers and no currents. Door state,
fall behavior, directed travel, and safe return remain unqualified.

The all-95 inactive-WLD scan found three non-overlapping `surf.wld` approaches:
#282224 east to #97755, #282226 west to #97648, and #282525 north to #97717.
They were not previously included in the reviewed Surf subset, so Checkpoint
98 raises it from 125 to 128 and the curated source-only total from 1,509 to
1,512. These declarations are not active ingress. Same-numbered
`surface2011.wld` records have different identities and exits; the unresolved
`mobs_surf.wld` #31404 down exit to #282525 remains map-ambiguous. Active
`end.shp` also names Shady room #97685 as a shop location. The supplement
records the route, typed dependency, history, and `shady2.mob` VNUM overlap.
See the [Shady Grove audit supplement](SHADY_GROVE_AUDIT_REVIEW.md),
[coverage row](ZONE_COVERAGE_REGISTER.csv#L268), and the [Surf](ATTIC_MIGRATION_PLAN.md#L27)
and [`shady2`](ATTIC_MIGRATION_PLAN.md#L41) candidate rows.

Shady Grove remains **DO NOT MOVE**. Source-only Surf edges, conditional
Nexus56 shop-reader exposure, and active typed consumers remain separate from
proof of live access or safe travel.

### Apocalypse Castle: Surface pair, Realm arrival, and inactive Surf approach

Apocalypse Castle has 312 rooms and 777/777 resolving active exits. Its
undirected local graph has weak components of 254, 57, and one room; the
coverage census records two fall-chance markers and no currents. Surface
#515793 enters castle #34500 and the castle exit returns to #515793. A
separate Realm exit enters vault #34804 with no reciprocal castle WLD exit;
the source dossier identifies this as a Staff Realm lead. These authored
edges do not establish ordinary player access, resolved door state, fall
safety, or safe return.

The all-95 inactive-WLD scan found one non-overlapping source-only `surf.wld`
approach: room #271560 north to #34500. It was not previously included in the
reviewed Surf subset, so Checkpoint 99 raises it from 128 to 129 and the
curated source-only total from 1,512 to 1,513. This is not active ingress. The
supplement also records outside `wh` QST and sidecar references,
13 active foreign reset-role rows, the two imported local reset prototypes,
and the unique active title and followed history. See the [Apocalypse Castle
audit supplement](APOCALYPSE_CASTLE_AUDIT_REVIEW.md), [coverage
row](ZONE_COVERAGE_REGISTER.csv#L2), [history
register](DUPLICATE_AND_REPLACED_ZONES.md), and the [Surf candidate
plan](ATTIC_MIGRATION_PLAN.md#L27).

Apocalypse Castle remains **DO NOT MOVE**. Preserve the reciprocal Surface
pair, one-way Realm boundary, source-only Surf approach, and selected outside
quest/reset consumers until their intended roles and live availability are
qualified.

### Ixxillikor: four reciprocal boundaries and no inactive approach

Ixxillikor has 199 rooms and 456/456 resolving exits: 452 local and four
outbound in one weak component. The census records one fall-chance marker and
no currents. Its four active boundaries are all reciprocal: #4200 north↔
Underdark #841353 south, #4200 west↔Underdark #841752 east, #4380 down↔Bugger
#6426 up, and #4398 up↔Underworld #4441 down. Exact local and reverse rows are
in the [cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L55), and the
source links appear in the [Ixxillikor audit supplement](IXXILLIKOR_AUDIT_REVIEW.md).

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without
parse issues or source-room overlap; it found no approach into Ixxillikor.
The authored reciprocal edges do not establish ordinary-player reachability,
door state, hazard outcome, or safe return. Ixxillikor remains **DO NOT MOVE**
as an active area contributor.

### Moonshae Island: five reciprocal boundaries and inactive port targets

Moonshae has 127 rooms and 266 authored exits; 264 resolve in the normal
full-area set. The authored local graph is one weak component of 127 rooms,
with no fall-chance or current markers. Five active inbounds are paired with
five reciprocal Moonshae exits: Surface #557218/#557619 to #26208, Llyrath
#94900 to #26257, Underworld3 #30000 to #26272, and Myrloch Vale #26401 to
#26311. The exact pairs appear in the [cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L404).

Port #26200 has two positive unresolved exits, north to #148979 and south to
#149179. Both VNUMs match only inactive `Duris3.wld` records, and the full
loader prunes them. The all-95 inactive-WLD scan found one additional
non-overlapping source-only approach: `surf.wld` #219584 west to #26208. It is
not active ingress. This raises the reviewed Surf subset from 129 to 130 and
the curated source-only total from 1,513 to 1,514. Static edges do not establish
ordinary-player access, port availability, dynamic travel, door state, or safe
return. See the [Moonshae audit supplement](MOONSHAE_ISLAND_AUDIT_REVIEW.md).

### Prison of Fort Boyard: two reciprocal Fort Boyard boundaries

Prisonb has 52 rooms and 108/108 resolving exits: 106 local exits and two
outbound exits to Fort Boyard. Its local undirected projection has three weak
components of 23, 22, and 7 rooms, with no fall-chance or current markers. Its
two active inbound boundaries are reciprocal: Fortb #38121 east ↔ Prisonb
#43041 west, and Fortb #38149 east ↔ Prisonb #43000 west. The exact pairs are
in [cross-area register rows 544/583 and 545/582](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L544).

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without
parse issues or source-room VNUM overlap; it found no approach into Prisonb.
Fortb's active reset at #38106 stocks Prisonb ship #43014, whose ENTER target
is #43020, but that dynamic path does not alter the static WLD census. Static
edges, weak components, and reset declarations do not establish ordinary
player reachability, live door or ship state, or safe return. See the
[Prison of Fort Boyard audit supplement](PRISON_OF_FORT_BOYARD_AUDIT_REVIEW.md).

### Lair of the Purple Worm: reciprocal Underdark and Underworld boundaries

The active `pworm` WLD has 98 rooms and 216/216 resolving exits: 212 local and
four outbound. Its local undirected projection is one 98-room weak component.
Four active inbound edges pair with all four outbound returns: Underworld
#4618 west ↔ `pworm` #42588 east; Underdark #814263 east ↔ #42583 west;
Underdark #832712 west ↔ #42500 east; and Underdark #833111 north ↔ #42500
south. Exact source and return records are in [cross-area register rows
81/578, 1733/577, 1759/575, and 1761/576](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L81).

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without
parse issues or active-room VNUM overlap. It found three non-overlapping
source-only `ud_map2.wld` exits into `pworm` rooms #42525, #42542, and #42500.
They raise the curated source-only approach total from 1,514 to 1,517; they do
not alter active reachability or the active-exit register. The same audit
found nine active Alatorin reset rows for `pworm` mobile #42501. Static exits
and reset rows do not establish ordinary-player reachability, door state,
spawn availability, or safe return. See the [Lair of the Purple Worm audit
supplement](LAIR_OF_THE_PURPLE_WORM_AUDIT_REVIEW.md).

### Vargan: three reciprocal boundaries and one inactive Surface approach

The active `vargan` WLD has 94 rooms and 239/239 resolving exits: 236 local
and three outbound, in one weak component of 94 rooms. Its three active inbound
edges are reciprocal: Surface #604903 south ↔ Vargan #2100 north; Surface
#605302 east ↔ #2100 west; and Vargan II #7800 up ↔ Vargan #2182 down. Exact
source and return records are in [cross-area register rows 35/1586, 36/1588,
and 37/176](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L35).

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without
parse issues or active-room VNUM overlap. It found one non-overlapping
source-only approach: inactive `surf.wld` #266321 east to active Vargan #2100.
This raises the reviewed Surf subset from 130 to 131 and the curated
source-only total from 1,517 to 1,518; the edge does not alter active
reachability or the active-exit register. Typed scans found no outside exact
QST item, shop, reset-role, sidecar item, prototype duplicate, or type-25 target
into Vargan. Static edges do not establish ordinary-player access, reset or
door state, environmental outcome, or safe return. See the [Vargan audit
supplement](VARGAN_AUDIT_REVIEW.md).

### The Maze of Undead Army: one reciprocal Surface boundary and two inactive-source records

The active `maze_are` WLD has 177 rooms and 395/395 resolving exits: 394 local
and one outbound, in one weak component. Surface #571011 east ↔ Maze #94177
west is the only active boundary pair (see [cross-area pair rows
265/539](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L265) and [exit rows
1131/1490](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1131)). A directed scan from
entrance #94177 reaches 173 rooms; carrier staging rooms #94117, #94118,
#94119, and #94121 are outside that directed set. This is authored-graph
reachability only; it does not simulate door state, roaming, travel mechanics,
or safe return.

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without
parse issues or source-room VNUM overlap. It found one non-overlapping,
source-only approach: `surf.wld` #281856 east to Maze entrance #94177. The
reviewed Surf subset is now 132 and the curated source-only total is 1,519;
this record does not create active ingress. Typed scans found no active
external QST item, SHP product, or ZON reset-role target, no external sidecar
target, duplicate mobile/object definition, or target among 872 external
type-25 declarations. One separate inactive `brass-old-1.zon` G reset names
Maze object #94009; it is source-only, not active stock. See the [Maze audit
supplement](MAZE_OF_UNDEAD_ARMY_AUDIT_REVIEW.md). Static records do not establish
live ordinary-player access or safe return.

### Bandit Camp: reciprocal Surface pair, WH dispersal edge, and inactive Surf approach

The active `banditca` WLD has 127 rooms and 311/311 resolving exits: 310 local and one outbound, in one weak component of 127 rooms. Directed authored local exits from #14800 reach all 127 rooms. The only outbound is Camp #14800 south to Surface #546515, paired with Surface #546515 north to Camp #14800 ([exit rows 266/1435](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L266), [pair rows 37/467](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L37)).

The other active inbound is `wh.wld` #55614 down to Camp #14854 ([exit row 768](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L768), [pair row 781](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L781)). Its room is labeled “Zone dispersal room for Incarnates”; the WH edge is one-way and does not establish ordinary player access or a return to WH. Camp's local authored graph reaches entrance #14800 from #14854, but static reachability does not establish actual travel, door/reset state, or safe return. No direct active WLD edge connects Camp to the separately active `bandit` / Bandit Canyons source.

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without parse issues or Bandit Camp source-room overlap. It found one non-overlapping source-only approach: inactive `surf.wld` #220524 north to active Camp #14800. This raises the reviewed Surf subset from 132 to 133 and the curated source-only total from 1,519 to 1,520; it is not active ingress.

Typed scans found active `tharnadian_ruin.zon` G object #14821, active WH reset/shop matches for ale object #14800, and Camp's imported object #67262. No outside QST item, exact structured sidecar VNUM, external prototype duplicate, incoming type-25 target, or direct C/C++ numeric binding was found. The broad sidecar token scan's #14910 hit is an identifier string, not a VNUM. See the [Bandit Camp audit supplement](BANDIT_CAMP_AUDIT_REVIEW.md) for exact references, current title/history, and disposition.

Static boundaries do not establish ordinary-player access, door or reset state, Incarnate availability, map ownership, or safe return. Bandit Camp remains **DO NOT MOVE**; preserve the reciprocal Surface pair, WH dispersal source edge, and inactive Surf approach as different kinds of evidence.

The Werrun, New Hope, Yerdonia, Arcaneum, Mistywood, Pharr Valley, Tribal Oasis,
Great Realm, Battlefield, Forgotten Mansion, Woodseer, Village of Refugees,
Twin Towers, Deep Ravine, Neverwind Valley, Grumbar's Domain, The Temple to
Skrentherlog, Caves of Mt. Skelenak, Southern Coastal Highway, Labyrinth of No
Return, Khildarak Stronghold, Strathor Valley, Gagga'Jobo Cave System,
Tharnadian Ruin, The Forest City of Aravne, The Great Shaboath, Lair of the
Purple Worm, The Altar of the Firesworn, The Royal Mausoleum of Castle IceCrag,
Negative Material Plane, The Prisons of Carthapia, Pits of Cerberus, Kimordril,
Shady Grove, Apocalypse Castle, Ixxillikor, Moonshae Island, Prison of Fort
Boyard, Vargan, The Maze of Undead Army, Bandit Camp, Myrloch Vale, Cloud Giant Kingdom, The Mountain of the Banished, Tiamat, The Obsidian Citadel, The Chasm of the Misty Vale, and Varathorn Keep reviews used repository-source and generated documentation inputs only.
No server build, test, gameplay session, database query, migration, or operational
script was run.

### Varathorn Keep: two reciprocal Surface pairs and two inactive approaches

The active kastle WLD has 100 rooms and 222/222 resolving exits: 220 local and two outbound, in one weak component. Treating local authored exits as traversable, #99400 reaches 92 rooms and 99 rooms can return to it; #99499 is the sole room without an authored return path. The seven distribution rooms #99492–#99498 have no directed entry from #99400. The two active Surface pairs are #99400 south ↔ #578633 north and #99439 up ↔ #575843 down. See Kastle outbound rows [1185](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1185) and [1186](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1186), Surface return rows [1510](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1510) and [1501](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1501), and pair summaries [218](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L218) and [524](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L524).

The all-95 inactive-WLD sweep found two non-overlapping source-only surf.wld approaches: #265063 north to Kastle #99400 and #266570 down to #99439. Both source rooms are absent from areas/AREA and neither creates current ingress. The reviewed Surf subset rises from 137 to 139, and the curated source-only approach total rises from 1,526 to 1,528. Keep surf and surface2011 **RETAIN** pending map ownership and remaining dependency/tooling closure.

Bounded typed closure records the two #98914 talon inputs, Tharnadia's outside quill reward, outside Raxthan/Surface Keeps/Valdrak stock and reset rows, and five Alatorin resets of Kastle centipede #99460. There is no local SHP source, duplicate outside mobile/object prototype, or inbound target among 872 active type-25 declarations. The exact title is unique and followed paths show no explicit predecessor or replacement. Keep kastle **DO NOT MOVE**. See the [Checkpoint 113 audit supplement](KASTLE_AUDIT_REVIEW.md) for source anchors and limits.

### Myrloch Vale: reciprocal Moonshae boundary and no inactive approach

The active `myrloch_vale` WLD has 178 rooms and 373/373 resolving exits: 372 local and one outbound. Its local weak graph has components of 165 and 13 rooms. Directed authored exits from #26401 reach only #26401–#26413 (13/178); this scan does not simulate the dossier’s TOUCH/ENTER travel, doors, reset stock, or runtime access.

Myrloch #26401 east ↔ Moonshae #26311 west is its only active boundary; Moonshae is also the single active inbound source (see [cross-area exit rows 408–409](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L408) and [pair rows 290 and 312](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L290)). The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits without parse issues or source-room overlap and found zero approaches into Myrloch. The reviewed Surf subset stays 133 and curated source-only total stays 1,520.

Typed closure records the active Caves of Mt. Skelenak Q155 and exact sidecar use of Myrloch shield #26438, reset imports #358/#55455, and local `unblock_on_death`, `item_switch`, and `inn` bindings. It found no other external QST item, SHP product, reset-role target, sidecar VNUM, duplicate prototype, or target among 865 external type-25 declarations. See the [Myrloch Vale audit supplement](MYRLOCH_VALE_AUDIT_REVIEW.md). Static graph structure does not establish ordinary-player reachability, live state, or safe return.


### Cloud Giant Kingdom: reciprocal Surface boundary and source-only Surf approach

The active `cldgt` WLD has 181 rooms and 391/391 resolving exits: 390 local and one outbound, in one weak component. Directed authored exits from #99500 reach all 181 rooms when ordinary gates are considered available; the dossier records that removing both boulder directions leaves 90 rooms inaccessible. This static graph does not establish current key availability, door state, flight, or safe return.

Cloud Giant #99500 south ↔ Surface #553314 north is the sole active boundary pair (see [exit rows 1187 and 1450](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1187) and [pair rows 81 and 481](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L81)).

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without parse issues or source-room overlap. It found one non-overlapping source-only approach: `surf.wld` #220537 west to Cloud entrance #99500. This is not active ingress; it raises the reviewed Surf subset from 133 to 134 and the curated source-only total from 1,520 to 1,521. Preserve it separately from the active Surface pair and keep `surf` / `surface2011` **RETAIN**.

Typed closure found no outside exact QST item, SHP product, ZON reset-role reference, sidecar VNUM, duplicate prototype, or external type-25 target into Cloud Giant. Its Q68 uses external object inputs #32490, #26614, and #402 and returns #405; direct code references include Nonstros #99548’s `SKILL_EMPOWER_SONG` entry and Chaos kit object #99538. See the [Cloud Giant Kingdom audit supplement](CLOUD_GIANT_KINGDOM_AUDIT_REVIEW.md). Static/source-only records do not establish player access or a working legacy tablet/reward route.

### The Mountain of the Banished: Surface, load-room, and inactive-map boundaries

The active `mount` WLD has 67 rooms and 135/135 resolving exits: 134 local and one outbound, across weak components of 46, 15, 5, and 1 room. Directed authored exits from #9100 plus all six fixed ENTER objects, modeled available, reach 64 rooms; administrative rooms #9157, #9165, and #9166 remain outside that static graph. This does not establish runtime access, reset stock, safe traversal, or return.

Mountain #9100 down ↔ Surface #517517 up is the reciprocal active pair. Juiblex #87673 down to Mountain #9106 and Scorch Valley #71326 down to #9122 are separate one-way active inbound edges to NPC load rooms. Those three edges yield three inbound active exits, one outbound exit, and one reciprocal reverse record. See [exit rows 184, 852, 1089, and 1345](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L184) and [pair rows 214, 296, 418, and 552](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L214).

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without parse issues or source-room VNUM overlap. It found two source-only approaches into Mountain entrance #9100: `oldmap8.wld` #184183 east and `surf.wld` #256676 west. Neither is active ingress. The reviewed Surf subset rises from 134 to 135, and the curated source-only total rises from 1,521 to 1,523. Keep both approaches with their source identities pending map ownership; retain `oldmap8`, `surf`, and `surface2011`.

The four external objects imported by Mountain's resets and two direct shaman-kit uses of local #9114 are documented in the [Mountain audit supplement](MOUNTAIN_OF_THE_BANISHED_AUDIT_REVIEW.md). Bounded scans found no other external exact QST item, SHP product, ZON reset-role reference, sidecar VNUM, duplicate prototype, literal callback, teacher binding, or external type-25 target. Source citations and limits are in the supplement; static source matches do not establish player use.

### Tiamat: Avernus and Astral Tiamat boundaries

The active `tiamat` WLD has 39 rooms and 116/116 resolving exits: 109 local and seven outbound. Its internal weak components have sizes 25, 10, 1, 1, 1, and 1. Following authored local exits from #19600 with ordinary gates assumed passable reaches 23/39 rooms. This source graph does not simulate NPC circulation, reset availability, the external pool, the dormant throne procedure, or safe return.

Two active WLD edges enter Tiamat #19600: Avernus #32031 north and Hell One of Avernus #51100 east. The Avernus entrance has its reverse Tiamat #19600 south; the Hell One edge is one-way. Tiamat has one outbound Avernus return and six one-way exits into Astral Tiamat #19953 from #19620, #19624–#19627, and #19638. The exact rows are [Tiamat exits 312–318](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L312), [Avernus inbound row 438](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L438), and [Hell One inbound row 639](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L639); the paired-area summary is [row 32](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L32), [row 34](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L34), and [row 187](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L187).

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without parse issues or source-room VNUM overlap. It found no approach into Tiamat and no overlap with Tiamat's active room VNUMs. A separate type-25 scan found one dynamic arrival: Astral Tiamat pool #19907 targets #19600 and is reset at #19902 and #19949. This arrival is not a WLD edge. See the [Tiamat audit supplement](TIAMAT_AUDIT_REVIEW.md).

Typed closure records nine external object prototypes and imported mobile #19700 in local resets, including unresolved object #25106 from inactive `brass-old-3.obj`; the loader disables that reset command because the object is absent from the active catalog. An active Astral Tiamat reset loads local barb #19607 at #19953. Exact QST/SHP/story-sidecar and prototype-duplicate scans found no other outside reference. Preserve the documented item, code, and dormant-room distinctions in the [Tiamat audit supplement](TIAMAT_AUDIT_REVIEW.md); Tiamat remains **DO NOT MOVE** and `brass-old-3` remains **RETAIN** pending owner review.

### The Obsidian Citadel: three reciprocal active pairs and one inactive Surf approach

The active `obcita` WLD has 179 rooms and 382/382 resolving exits: 379 local and three outbound, in one weak component. With every authored local exit treated as passable, directed travel from #75600 reaches all 179 rooms; the existing dossier separately records 147/179 without gate items, 178 with its selected gate identities, and 179 with the fixed CACKLE bookshelf/orb routes. These static counts do not establish runtime arrival, available stock, hazards, or a safe return.

Obcita #75600 south ↔ Surface #579804 north is one active reciprocal pair. Obcita #75778 east ↔ Underdark #736889 west and #75778 south ↔ Underdark #737288 north are two more reciprocal pairs. There are three active inbound exits, three outbound exits, and three reverse records. See [Obcita outbound rows 873-875](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L873), [Surface inbound row 1516](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1516), [Underdark inbound rows 1696-1697](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1696), and [pair summaries](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L332).

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits with zero parse issues or source-room VNUM overlap. It found one non-overlapping source-only approach: inactive `surf.wld` #262357 north to Obcita #75600. This is not active ingress. The reviewed Surf subset rises from 135 to 136, and the curated source-only total rises from 1,523 to 1,524. Keep `surf` and `surface2011` **RETAIN** pending map ownership and dependency closure; see the [Obcita audit supplement](OBCITA_AUDIT_REVIEW.md) and [candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

Bounded typed scans found reset imports #359 and #55188 from active Heavens/Winterhaven sources, plus active Ravenloft P resets for local scrolls #75638/#75669. No outside QST item, SHP product, exact story-sidecar VNUM, duplicate mobile/object prototype, or external type-25 target into Obcita was found. Direct code bindings and followed title/history are documented in the [Obcita audit supplement](OBCITA_AUDIT_REVIEW.md). Static source declarations do not prove live stock or player travel.

### The Chasm of the Misty Vale: reciprocal Surface boundary and two inactive approaches

The active mist_chasm WLD has 100 rooms and 209/209 resolving exits: 208 local and one outbound, in one weak component. With all authored local exits treated as passable, directed traversal from entrance #15000 reaches all 100 rooms, and every room has an authored directed return path to #15000. The active Chasm #15000 east ↔ Surface #638825 west boundary is reciprocal. See [Chasm exit row 267](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L267), [Surface return row 1670](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1670), and [pair rows 282 and 546](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L282).

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits with no parse issues and zero VNUM overlap with Chasm rooms #15000–#15099. It found two source-only approaches: surf.wld #253287 west to Chasm #15000 and torr.wld #79092 south to Chasm #15052. Both source stems are absent from areas/AREA; neither is active ingress. The reviewed Surf subset rises from 136 to 137, and the curated source-only total rises from 1,524 to 1,526. Preserve the records with their source identities; keep surf / surface2011 and torr **RETAIN** pending ownership and dependency/tooling closure. See the [Chasm audit supplement](MIST_CHASM_AUDIT_REVIEW.md).


### Goblin City: four reciprocal boundaries and one inactive Surf approach

The active `malch` graph contains 161 rooms and 360/360 resolving exits: 356 local and four outbound, in one weak component. With authored blocked directions excluded, local directed traversal from each of #23600, #23721, and #23741 reaches 159 rooms and 159 have a directed return; allowing the paired blocked controls raises both counts to 161. These source counts do not establish runtime access, reset state, or safe return.

Malch has four reciprocal active pairs: #23600 up ↔ Surface #632944 down, #23721 east ↔ Underdark #849235 west, #23741 east ↔ Troll Caves #96976 west, and #23760 up ↔ Shipyards #43320 down. The exact edges are [Malch rows 382–385](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L382), [reverse rows 593, 1157, 1659, and 1782](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L593), and [pair rows 259–262](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L259).

The all-95 inactive-WLD scan found one non-overlapping source-only approach: `surf.wld` #267236 down to Malch #23600. This is not active ingress; the reviewed Surf subset is now 140 and the curated source-only total is 1,529. Keep `surf` and `surface2011` **RETAIN**. The [Checkpoint 114 Goblin City audit supplement](MALCH_AUDIT_REVIEW.md) records typed dependencies, the corrected three-shop inventory, and title/history evidence.

### Miaeril Village: two outbound records, one reciprocal Surface pair, no inactive approach

The active `mril` graph has 65 rooms and 190/190 resolving exits: 188 local and two outbound, in one weak component. Authored directed traversal from #33700 or #33764 reaches 63/65 rooms when the concealed stump route is unavailable; #33742 and #33743 are omitted. Every room has an authored return path to the tested roots. Treating all authored exits as available raises reachability to 65/65. This is source-only traversal; it does not model SEARCH/open success, current/fall movement, actor state, reset timing, or route safety. The source has two fall-chance rooms and six current fields.

Both outbound exits target Surface #522257: #33717 south and #33764 east. Only #33764 has a reverse edge, Surface #522257 west, so this is one reciprocal pair plus a separate one-way outbound record. See [Miaeril exits 453–454](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L453), [Surface inbound row 1365](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1365), [Miaeril-to-Surface pair row 306](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L306), and [Surface-to-Miaeril pair row 554](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L554).

The all-95 inactive-WLD sweep found no source-room overlap with Miaeril and no exit into any of its 65 rooms. This adds no source-only approach: the reviewed Surf subset stays at 140 and the curated total at 1,529. Keep `surf` and `surface2011` **RETAIN** pending map ownership and remaining dependency/tool closure. See the [Miaeril audit supplement](MIAERIL_AUDIT_REVIEW.md).

### The Temple of the Sun: reciprocal Surface pair and one inactive Surf approach

The active `suntmpl` graph has 94 rooms and 217/217 resolving exits: 216 local and one outbound, in one weak component. With every authored exit treated as available, directed traversal from entrance #82493 reaches all 94 rooms and each has an authored return. The comprehensive dossier records gated layers of 51 without SEARCH/keys, 54 with SEARCH, 88 with the vine key, 89 with cellar key, 90 with all three keys, and 94 with skilled PICK access. These source assumptions do not prove live access, spent-key persistence, or safe travel. The Temple has no fall or current fields.

Temple #82493 west ↔ Surface #541275 east is one reciprocal active pair, with one inbound, one outbound, and one reverse record. See [Temple outbound row 988](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L988), [Surface inbound row 1415](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1415), [Temple-to-Surface pair row 457](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L457), and [Surface-to-Temple pair row 600](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L600).

The all-95 inactive-WLD sweep found one non-overlapping source-only approach: `surf.wld` #237595 west to #82493. It is not active ingress. The reviewed Surf subset rises from 140 to 141 and the curated source-only total from 1,529 to 1,530. Keep `surf` and `surface2011` **RETAIN** pending ownership and dependency/tool closure. See the [Temple audit supplement](TEMPLE_OF_THE_SUN_AUDIT_REVIEW.md).

### Lair of the Gibberling King: two reciprocal Underdark pairs, a Realm arrival, and one inactive approach

The active `gibber` graph has 122 rooms and 406/406 resolving exits: 404 local and two outbound, in one weak component. It has three active inbound records, two reverse records, one fall-chance room, and no current fields. From entrance #8700, the raw directed passable-exit graph reaches 110 rooms when the flagged boulder direction is excluded and 114 when it is allowed; the story dossier records stricter SEARCH/key/PICK/PUSH tiers and the alternate arrival. See the [Gibber audit supplement](GIBBERLING_KING_AUDIT_REVIEW.md).

Gibber #8700 connects reciprocally with Underdark #820318 south and #820717 east; its local north and west exits return to those endpoints. A separate Realm #14209 northeast exit enters #8788 with no reverse edge. Exact active records are [Gibber outbound rows 180–181](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L180), [Underdark inbound rows 1739–1740](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1739), [Realm inbound row 256](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L256), and pair summaries [153](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L153), [392](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L392), and [703](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L703).

The all-95 inactive-WLD sweep found one non-overlapping source-only approach: `ud_map2.wld` #1125006 down to Gibber entrance #8700. This is not active ingress. It raises the curated source-only total from 1,530 to 1,531; the reviewed Surf subset stays 141. `ud_map2` now has eight recorded source-only approaches into active areas, separate from its active Shafts target. Keep `ud_map2` **RETAIN** and active `gibber` **DO NOT MOVE**.

Typed closure records Airp's QST/story-sidecar use of object #8735, imported Heavens object #359 and Winterhaven memory #55190 reset objects, and Winterhaven's #55190 return. The SurfaceKeeps #8750 hit is an experience reward, not an item; the `randomeq.c` Crymson number is in a dormant list whose only matching loop is commented out. No outside reset-role target, SHP product, duplicate prototype, type-25 destination, or local shop was found. The normalized title is unique and followed paths show no alternate stem or explicit predecessor/replacement. See the [Checkpoint 117 audit supplement](GIBBERLING_KING_AUDIT_REVIEW.md) for the typed evidence and bounds.

### Ice Tower: reciprocal Surface pair and one inactive Surf approach

The active `icetower` graph has 162 rooms and 359/359 resolving exits: 358 local and one outbound, in one weak component. Its entrance #31800 and Surface #523117 have one reciprocal pair, with one inbound, one outbound and one reverse record. All-authored directed traversal from #31800 reaches and returns to all 162 rooms; the story dossier records speech/key/SEARCH/PICK-gated tiers. There are no fall or current fields. These source counts do not establish live access or safety.

The all-95 inactive-WLD scan found one non-overlapping source-only approach: `surf.wld` #206576 north to #31800. It is not active ingress. The reviewed Surf subset rises from 141 to 142 and the curated source-only total from 1,531 to 1,532. Keep `surf` and `surface2011` **RETAIN** and active `icetower` **DO NOT MOVE**. See the [Ice Tower audit supplement](ICE_TOWER_AUDIT_REVIEW.md), [cross-area exit rows](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L437), [pair rows](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L201), and [attic plan](ATTIC_MIGRATION_PLAN.md).

### Checkpoint 119 — Durian Underdark

The active Underdark map has 13,587 rooms and 53,661 authored exits. Loader qualification resolves 46,848: 46,747 local and 101 outbound; 6,813 absent sparse-cell targets are pruned. Its 101 inbound and 101 outbound cross-area records form reciprocal pairs. The local graph has six weak components (13,192/274/102/14/4/1 rooms) and no fall/current room metadata. The dossier's terrain-aware route counts apply the mountain map-admission rule but do not establish safe or played reachability.

The all-95 inactive-WLD sweep found no room overlap or exit into the current active Underdark, so no new source-only approach was added. The curated total remains 1,532 and the Surf subset remains 142. This does not resolve the 11 active exits into the separate unregistered `underdark2011` candidate or its conflicting #217609 room identity. See the [Underdark audit supplement](DURIAN_UNDERDARK_AUDIT_REVIEW.md), [coverage row](ZONE_COVERAGE_REGISTER.csv#L324), [candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L115), and [cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv).

### Checkpoint 120 — Bahamut's Palace

The active Bahamut map has 236 rooms and 655 authored exits; all 655 resolve (654 local, one outbound to Plane of Air #24460). Three active entrances arrive from Plane of Air #24460, Realm staging #14208, and White Hills staging #55632. Only the Plane of Air route has a reciprocal local exit. The local graph is one weak component with no fall/current fields. The raw directed traversal from #25700 reaches 223 rooms and all 236 have authored returns if gate, secrecy, flight, and live admission are ignored; the dossier records the actual conditional key circuit.

The all-95 inactive-WLD sweep found no room overlap or approach into Bahamut. The curated total remains 1,532 and the Surf subset remains 142. See the [Bahamut audit supplement](BAHAMUT_AUDIT_REVIEW.md), [coverage row](ZONE_COVERAGE_REGISTER.csv#L22), and [cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv).

### Checkpoint 121 — Clan Stoutdorf Settlement

The active `drst` map has 199 rooms and 432/432 resolving exits: 430 local and two outbound. Its 199 rooms form one weak component; authored directed traversal from #34200 reaches and returns from all 199 when edges are assumed available. The two active inbound and two outbound records form two reciprocal pairs: Underdark #840288 ↔ Stoutdorf #34200, and Cloister #67138 ↔ Stoutdorf #34351. These counts do not establish current door state or safe travel.

The all-95 inactive-WLD sweep found one non-overlapping source-only approach: `ud_map2.wld` #1121983 west to Stoutdorf #34200. It is not active ingress. The curated source-only approach count is now 1,533; the Surf subset remains 142. Preserve the approach and the active `drst` map. See [DRST audit supplement](DRST_AUDIT_REVIEW.md) and [coverage row](ZONE_COVERAGE_REGISTER.csv#L82).

### Checkpoint 122 — The Charcoal Palace

The active `firep` map has 151 rooms and 412/412 resolving exits: 411 local and one outbound to Plane of Fire #25415 from #88301 west. The one inbound edge also comes from #25415 west, entering #88321. The endpoints differ, so the two directions are not reciprocal. The weak graph has components of 150 and 1 rooms. Directed authored traversal from #88321 reaches 25/151 with both `EX_BLOCKED` controls unavailable and 149/151 when controls are assumed available; the unrestricted graph has authored returns from 150/151. The dossier records the gated progression and the unreachable kitchen/fissure rooms. There are no fall fields and two current fields; this source analysis does not qualify live access or safety.

The all-95 inactive-WLD scan found no room overlap or approach into `firep`. The curated source-only approach total remains 1,533 and the Surf subset remains 142. See the [Firep audit supplement](FIREP_AUDIT_REVIEW.md) and [coverage row](ZONE_COVERAGE_REGISTER.csv#L96).

### Checkpoint 123 — Menden-on-the-Deep

The active `menden` graph has 71 rooms and 165/165 resolving exits: 161 local and four outbound. Four active Surface/Ocean inbound edges and four Menden outbound edges form four reciprocal pairs: #549925↔#88863, #550326↔#88870, #550324↔#88846, and #550725↔#88846. The local raw WLD graph has components of 70 and 1 rooms; directed traversal from #88863 reaches and returns from 70 rooms. The remaining #88860 pool landing is linked by the dossier's bound special-object route, not a local WLD exit. There are no fall/current fields.

The all-95 inactive-WLD scan found two non-overlapping source-only approaches from `surf.wld` into #88846: #232071 east and #232372 north. They are not active ingress. The curated source-only total rises from 1,533 to 1,535; the reviewed Surf subset rises from 142 to 144. Preserve both Surf records and active `menden`. See the [Menden audit supplement](MENDEN_AUDIT_REVIEW.md) and [coverage row](ZONE_COVERAGE_REGISTER.csv#L174).

### Priority 164 — Mitashi: active boundaries and inactive Duris3 map approaches

The active `mitashi` WLD has 229 rooms and 572/572 resolving exits (570 local, two outbound). Its weak graph contains all 229 rooms, and authored directed traversal from #138200 reaches and returns from all 229 when exits are treated as available. The two outbound routes are #138200 north to Surface #647688 and #138409 south to #648088. Active inbound routes are Surface #647288 south to #138200 and #648088 north to #138409. Only the #138409↔#648088 endpoints form a reciprocal pair; the northern records use different Surface rooms. There are no fall/current fields. See the [Mitashi audit supplement](MITASHI_AUDIT_REVIEW.md) and [coverage row](ZONE_COVERAGE_REGISTER.csv#L183).

The all-95 inactive-WLD sweep found 916 `Duris3.wld` target records into Mitashi. It includes 462 records from non-overlapping source rooms—two 229-exit north/south map bands plus four endpoints—and 454 records from source-room VNUMs overlapping #138200–#138428. Only the 462 non-overlapping records enter the curated source-only total; the overlaps remain separate map-identity evidence. `Duris3` is not an active loader input, so none is current ingress. The curated total rises from 1,535 to 1,997, and the reviewed Surf subset stays 144. Preserve active `mitashi` and retain `Duris3` pending broader map/dependency closure.

### Priority 165 — The Dark Stone Tower: reciprocal North Lakes access and a split local graph

The active `teka2` WLD has 98 rooms and 198/198 resolving exits: 197 local and one outbound. Its weak graph has components of 85 and 13 rooms. From cathedral #75546, raw directed local-WLD traversal reaches 11 rooms, while reverse traversal reaches 85/98. The one outbound edge is #75546 south to North Lakes #75264; North Lakes #75264 north returns to #75546, forming one reciprocal pair. Teka2 has one fall field at #75581 and three current fields at #75596–#75598. The dossier separately maps key and typed object-teleport access; the raw graph does not model those actions.

The all-95 inactive-WLD scan found no room overlap and no target into Teka2. The curated source-only total remains 1,997 and the reviewed Surf subset remains 144. Typed closure records local imports #358/#55454, Alatorin's two reset uses of local #75529, DLSC's `E:75500` experience amount and North Lakes route prose. See the [Teka2 audit supplement](TEKA2_AUDIT_REVIEW.md), [coverage row](ZONE_COVERAGE_REGISTER.csv#L292), and active cross-area pair [register rows 326 and 641](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L326).

### Priority 166 — The Troll Hills

The active `troll_hills` WLD has 148 rooms and 460/460 resolving exits: 457 local and three outbound, in one weak component. With all authored local exits available, directed traversal from #1801 reaches and returns from all 148 rooms. Four active inbound, three outbound, and three reverse records form three reciprocal pairs—with Surface twice and Ghore once—plus a separate Lortower #134000 east → #1883 inbound. Two fall fields and no current fields are present. These counts do not model the dossier's secret-door, water, actor, stock, or hazard conditions.

The all-95 inactive-WLD scan found two non-overlapping source-only `surf.wld` approaches: #278929 south → #1852 and #279529 north → #1871. They are not active ingress. At Checkpoint 126, the reviewed Surf subset rose from 144 to 146 and the curated source-only total from 1,997 to 1,999; those counts are historical, and both approaches are already included in the current 11,993 overall / 415 Surf total. Keep `surf` / `surface2011` **RETAIN** and active `troll_hills` **DO NOT MOVE**. See the [Troll Hills audit supplement](TROLL_HILLS_AUDIT_REVIEW.md) and [coverage row 315](ZONE_COVERAGE_REGISTER.csv#L315).

### Priority 167 — A Dark and Twisted Wood

The active `twstwd` WLD has 100 rooms and 335/335 resolving exits: 331 local and four outbound, in one weak component. Raw authored traversal from #16300 reaches 97 rooms and has a directed return from all 100; staging rooms #16397–#16399 have no incoming local exit. The four outbounds form reciprocal Surface, Pine Hollow and Krindor pairs. Two additional Pine Hollow inbound links to #16308/#16344 are NPC-load connections, not reverse pairs. There are no fall or current fields; route-specific SEARCH/OPEN, camp access, conditional SHIFT PRIME arrival and live placement remain separate.

The all-95 inactive-WLD scan found five non-overlapping source-only `surf.wld` approaches to #16303, #16302, #16301 and #16300 (twice). They are not active ingress. The reviewed Surf subset rises from 146 to 151 and the curated source-only total from 1,999 to 2,004. Keep `surf` / `surface2011` **RETAIN** and active `twstwd` **DO NOT MOVE**. See the [Twisted Wood audit supplement](DARK_AND_TWISTED_WOOD_AUDIT_REVIEW.md) and [coverage row 321](ZONE_COVERAGE_REGISTER.csv#L321).

### Priority 168 — The Underworld

The active `underworld` WLD has 232 rooms and 511 authored exits: 484 local, 26 valid outbound, and one unresolved north exit from #4645 to #4646. The loader resolves 510/511 exits. Weak components have 154, 70, 6, 1, and 1 rooms; directed traversal from #4401 reaches and returns from all 154 rooms in its component. There are 30 inbound boundary records, 26 outbound and 26 reverse records forming reciprocal pairs, plus four inbound-only records. The source has 13 current fields and no fall fields. The dossier separately maps conditional doors, keys, pools, and inbound-target scenarios; raw WLD traversal does not establish live availability.

The all-95 inactive-WLD scan found three non-overlapping source-only approaches: `ud_map0.wld` #1105558 north to #4469 and `ud_map2.wld` #1120813 north to #4469 and #1127503 up to #4608. They are not active ingress. The curated source-only approach total rises from 2,004 to 2,007; the reviewed Surf subset remains 151. Keep `ud_map0`, `ud_map1`, `ud_map2`, `surf`, and `surface2011` **RETAIN** and active `underworld` **DO NOT MOVE**. See the [Underworld audit supplement](UNDERWORLD_AUDIT_REVIEW.md), [coverage row 326](ZONE_COVERAGE_REGISTER.csv#L326), and [unresolved-exit row 7](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L7).

### Priority 169 — Zalkapfaan, City of the Headless Horde

The active `headless` WLD has 60 rooms and 132/132 resolving exits: 130 local and two outbound. Its weak graph is one 60-room component. Raw directed traversal from #2730 reaches 54 rooms and has authored returns from 59/60; the six staging rooms #2784–#2789 lie outside root traversal, and #2789 has no authored return to #2730. The two outbound Surface exits have two reciprocal Surface returns. There is one current field and no fall field. The dossier separately maps the six staging rooms, conditional gates, keys, passwords, and roaming actors.

The all-95 inactive-WLD scan found one non-overlapping source-only approach: `surf.wld` #221513 west to #2730. It is not active ingress. The reviewed Surf subset rises from 151 to 152 and the curated source-only total from 2,007 to 2,008. Keep `surf` / `surface2011` **RETAIN** and active `headless` **DO NOT MOVE**. See the [Zalkapfaan audit supplement](ZALKAPFAAN_AUDIT_REVIEW.md), [coverage row 120](ZONE_COVERAGE_REGISTER.csv#L120), and reciprocal pair [summary row 169](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L169).

### Priority 170 — The Spires of the Elder Eternal Evil

The active `eternal` WLD has 333 rooms and 708/708 resolving exits, all local, with no active cross-area inbound or outbound records. Its weak graph has 17 components: 275 and 43 rooms plus 15 single-room components. From #135400, raw directed traversal reaches 264 and has authored returns from 274 with the three blocked edges unavailable; assuming them available reaches 272 and returns from 275. The area has no fall or current fields. These static figures do not model switch use, keys, password gates, item stock, or live travel.

The all-95 inactive-WLD scan found 1,332 `Duris3.wld` target records into Eternal room VNUMs #135400–#135732. Of these, 602 begin in non-overlapping source rooms, and 730 begin in numerically overlapping source rooms. Only the 602 non-overlapping records enter the curated source-only total; neither group is active ingress. The curated total rises from 2,008 to 2,610 and the reviewed Surf subset remains 152. Keep `Duris3` **RETAIN IN PLACE** and active `eternal` **DO NOT MOVE**. See the [Spires audit supplement](SPIRES_OF_ELDER_EVIL_AUDIT_REVIEW.md), [coverage row 90](ZONE_COVERAGE_REGISTER.csv#L90), and [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

### Priority 171 — The Shaughin Settlement

The active `shaughin` WLD has 49 rooms and 135/135 resolving exits: 131 local and four cross-area. Weak components contain 45 and 4 rooms; raw directed traversal from #6500 reaches 45 and returns from 43. Four inbound, outbound, and reverse records form reciprocal pairs with Underdark #804249 and `shaughin2` #6699/#6686/#6695. There are two falls and three currents in the upper WLD. The all-95 inactive-WLD scan finds no overlap or target into #6500–#6548, so the curated source-only total remains 2,610 and the reviewed Surf subset 152. The selected 149-room combined scenario depends on the dossier's stated access, object-teleport, water, and survival assumptions. See the [Shaughin audit supplement](SHAUGHIN_AUDIT_REVIEW.md), [coverage row 270](ZONE_COVERAGE_REGISTER.csv#L270), and [existing boundary pairs](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L433).

### Priority 172 — The Trakkia Mountains

The active `trakkia` WLD has 166 rooms and 387/387 resolving exits (385 local, 2 outbound) in five weak components of 109/28/16/12/1. From #57000, raw reach/return is 72/106 with the four blocked-flag exits unavailable and 104/106 when all four are assumed available. The two outbound Surface edges and two inbound Surface returns form two reciprocal pairs at #629225 and #629624. There are four fall fields and no currents. The all-95 inactive-WLD scan adds one non-overlapping, source-only Surf approach: #246406 west to #57000; it is not active ingress. At this checkpoint, curated totals reached 2,611 overall and 153 for Surf; these are historical counts. The approach is already included in the current 11,993 overall / 415 Surf totals. See the [Trakkia audit supplement](TRAKKIA_AUDIT_REVIEW.md), [coverage row 310](ZONE_COVERAGE_REGISTER.csv#L310), and existing [Surface pair rows](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L613).

### Priority 173 — The Stone Tomb of Kelek

The active `kelek` WLD has 216 rooms and 430/430 resolving exits (428 local, 2 outbound) in components of 215 and 1. #87951 is the isolated singleton; raw directed traversal from #87950 reaches and returns from 215/216. Two active inbound/outbound/reverse records form reciprocal pairs with Underdark #857219 and Krethik #20115. Kelek has no fall/current fields. The all-95 inactive-WLD scan finds no overlap or target into Kelek, so curated source-only totals stay 2,611 overall and 153 for Surf. Secret routes, the crystalline key, Deliverer, and conditional Prime Shift are separate from ordinary WLD traversal. See the [Kelek audit supplement](KELEK_AUDIT_REVIEW.md), [coverage row 143](ZONE_COVERAGE_REGISTER.csv#L143), and [existing boundary pair rows](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L219).

### Priority 174 — Krethik Keep

The active `krethik` WLD has 148 rooms and 307/307 resolving exits (304 local, 3 outbound) in weak components of 146/1/1. Root #20000 reaches/returns 142/146 with the two blocked exits unavailable, or 145/146 if both are assumed open. Three fall fields and no currents are present. Three outbound/inbound/reverse pairs link Surface twice and Kelek once; an additional Winterhaven inbound #55633 southwest→#20147 has no reverse. The all-95 inactive-WLD sweep adds four non-overlapping source-only approaches: two oldmap8 edges (#182553→#20000 and #182654→#20085) and two Surf edges (#274522→#20000, #274526→#20112). These are not active ingress. Curated totals are now 2,615 overall and 155 for Surf. See the [Krethik audit supplement](KRETHIK_AUDIT_REVIEW.md), [coverage row 149](ZONE_COVERAGE_REGISTER.csv#L149), and existing [boundary summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L230).

### Priority 175 — Thetis's Realm

The active thetis WLD has 100 rooms and 218 authored exits: 217 resolve (215 local and two outbound), while #38095 north → #104 remains unresolved. Its one weak component has 100 rooms; raw directed reach/return from #38000 is 100/100, with no fall or current fields. The Surface #618491 and Heavens #38 boundaries have no active reverse or inbound records. The all-95 inactive-WLD sweep adds Surf #251543 north → #38000 as a non-overlapping source-only approach, raising curated totals to 2,616 overall and 156 for Surf. The #104 candidate rooms in dalvik and mavulsk remain outside areas/AREA. See the [Thetis audit supplement](THETIS_AUDIT_REVIEW.md), [coverage row 300](ZONE_COVERAGE_REGISTER.csv#L300), and existing [boundary summary rows](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L654).

### Priority 176 — The Lair of Tiamat

The active azhural WLD has 89 rooms and 216/216 resolving local exits in weak components of 80, 8, and 1 rooms. Raw reach/return from #135201 is 80/89; there are no active cross-area exits, falls, or currents. The all-95 inactive-WLD sweep finds 180 non-overlapping Duris3 approaches, plus 172 file-specific source-room VNUM overlaps (89 in Duris3 and 83 in tiamats_lair). The 200 tiamats_lair exit records target current room VNUMs from overlapping source rooms and are not new approaches. Preserve the local item portals and isolated #135283 as separately documented non-WLD mechanics. The explicit 2012 source swap confirms Azhural as the new version; see the [Azhural audit supplement](AZHURAL_AUDIT_REVIEW.md) and [coverage row 21](ZONE_COVERAGE_REGISTER.csv#L21).

### Priority 177 — Quintaragon Castle

The active Castle WLD has 130 rooms and 392/392 resolving exits: 391 local and one cross-area. Its local graph is one 130-room weak component; raw directed traversal from #2401 reaches 123 rooms and returns from all 130. The only boundary is the reciprocal Minizones entrance #2401 south ↔ #5760 north. One authored F25 field at #2463 is suppressed because the room has no down exit; there are no currents. The all-95 inactive-WLD sweep finds no source-room overlap or target into Castle, so curated totals remain 2,796 overall and 156 for Surf. See the [Castle audit supplement](CASTLE_AUDIT_REVIEW.md), [coverage row 46](ZONE_COVERAGE_REGISTER.csv#L46), and boundary [pair rows 68 and 270](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L68).

### Priority 178 — The Church of the Eternal Dusk

The active Church WLD has 33 rooms and 69/69 resolving exits (68 local and one cross-area), one weak component, and raw reach/return from #87820 of 33/33. Its only boundary is reciprocal with Surface: Church #87847 east ↔ Surface #527483 west. There are no falls or currents. The all-95 inactive-WLD scan finds no source-room overlap or target into Church, so curated totals remain 2,796 overall and 156 for Surf. A separate all-file typed scan finds inactive `brass-old-1.zon` G-reset #87859 from Church; it is not active stock and is tracked in the retained Brass candidate. See the [Church audit supplement](CHURCH_AUDIT_REVIEW.md), [coverage row 56](ZONE_COVERAGE_REGISTER.csv#L56), and boundary [pair rows 78 and 480](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L78).

### Priority 179 — Domain of Lost Souls

The active Domain WLD has 100 rooms and 218/218 resolving exits (217 local, one outbound), one weak component, and raw root #36800 reach/return of 91/100. The Underdark boundary is reciprocal; WH #55632 has a one-way inbound exit to #36879. There are no negative/unresolved targets, falls, or currents. The all-95 inactive-WLD scan finds no room overlap and two source-only approaches to #36800 from `ud_map1` and `ud_map2`, raising curated totals to 2,798 overall while Surf remains 156. An all-file typed scan finds source-only `nexus56` reset/shop references to book #36822, plus effective CHAOS kit, epic-teacher, and object-special references documented in the supplement. Keep `dlsc` DO NOT MOVE and `nexus56` RETAIN. See the [Domain audit supplement](DLSC_AUDIT_REVIEW.md), [coverage row 77](ZONE_COVERAGE_REGISTER.csv#L77), and boundary [pair rows 118, 697, and 789](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L118).

### Priority 180 — Drifting Realm

The active Dream WLD has 27 rooms and 46/46 resolving exits (45 local and one outbound) in weak components of 24 and 3 rooms; raw root #31300 reach/return is 3/27. Its only WLD boundary is reciprocal with Ethereal #12442. The all-95 inactive-WLD scan finds no room overlap or source-only approach, so curated totals remain 2,798 overall and 156 for Surf. A non-WLD scan finds active Arcium type-25 bed #85725 targeting #31300; Dream's lights/cliff add dynamic travel across its raw WLD components. Active Mriel, Lava, Arcium, and Fishermans Wharf resets stock Dream skulls #31317–#31320; Dream imports Alatorin flower #83283. Compiled CHAOS tables reference five other Dream objects, while no proof-construction reference for the shard/skull recipes was found. Keep `dream` DO NOT MOVE. See the [Drifting Realm audit supplement](DRIFTING_REALM_AUDIT_REVIEW.md), [coverage row 80](ZONE_COVERAGE_REGISTER.csv#L80), and Ethereal boundary [pair rows 121 and 133](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L121).

### Priority 181 — The Lizardman Swamps of Clavikord

The active 120-room Lizard WLD has 328/328 resolving exits (326 local, two outbound) in weak components of 101 and 19 rooms; raw root #6000 reaches/returns 100/101 rooms. Surface and Underdark boundaries reciprocate; Heavens #15 and WH #55402 add two one-way active inbounds. The all-95 inactive-WLD scan finds no overlap or approach, so curated totals remain 2,798 overall and 156 for Surf. The local reciprocal pools connect rooms #6102 and #6103 outside raw WLD traversal. No falls, currents, negative/unresolved targets, or local duplicate IDs. See [coverage row 160](ZONE_COVERAGE_REGISTER.csv#L160), [boundary rows](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L175), and the [Lizardman Swamps audit supplement](LIZARDMAN_SWAMPS_AUDIT_REVIEW.md).

### Priority 182 — Tower of High Sorcery

The active 100-room Tower WLD has 219/219 resolving exits (218 local, one outbound) in one weak component. Its Surface boundary pair reciprocates. Two internal flag-8 exits link #9353 and #9372. Raw traversal from #9300 reaches/returns 72/75 rooms with those edges excluded, or 92/99 when available; even with every authored local exit available, #9392–#9399 are outside root-forward traversal and #9399 has no authored return. No negative/unresolved targets, falls, currents, or local duplicates. The all-95 inactive-WLD scan adds two non-overlapping source-only Surf approaches, to #9334 and #9379; curated totals rise to 2,800 overall and 158 for Surf. See [coverage row 308](ZONE_COVERAGE_REGISTER.csv#L308), active boundary [pair rows 612 and 665](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L612), and the [Tower audit supplement](TOWER_OF_HIGH_SORCERY_AUDIT_REVIEW.md).

### Priority 183 — Vecna's Tomb

The active 80-room Vecna WLD has 187/187 resolving exits (186 local, one outbound) in weak components of 78, 1, and 1. Raw traversal from either active inbound room #130000 or #130021 reaches/returns 61/77; no local edge has flag 8. Surface #597408↔#130000 reciprocates, while Realm #14208 has one-way inbound to #130021. The map has one authored F100 fall in room #130037; no currents, negative/unresolved targets, or local duplicate IDs. The all-95 inactive-WLD scan adds 162 non-overlapping source-only Duris3.wld records into Vecna; 158 additional matches come from 80 same-numbered but differently titled Duris3 rooms. Curated totals rise to 2,962 overall; Surf remains 158. These records are not active ingress. See [coverage row 336](ZONE_COVERAGE_REGISTER.csv#L336), boundary [pair rows 402, 625, and 771](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L402), the [Duris3 candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), and the [Vecna audit supplement](VECNAS_TOMB_AUDIT_REVIEW.md).

### Priority 184 — Killing Fields

The active 198-room Killing Fields WLD has 737/737 resolving exits (730 local, seven outbound) in one weak component; raw traversal from #33300 reaches and returns to all 198 rooms. Surface #585374↔#33302 and five Bloody Plains boundaries reciprocate. The sixth Bloody Plains exit, #33422 east to #33597, has no reverse edge. There are no negative/unresolved targets, local duplicate IDs, falls, or currents. The all-95 inactive-WLD scan adds one non-overlapping source-only Surf exit, #279135 south to #33302; curated totals rise to 2,963 overall and 159 for Surf. The unregistered surf.zon also resets mobile #33308 and objects #33309–#33312, #33314, and #33315 as source-only records. See [coverage row 145](ZONE_COVERAGE_REGISTER.csv#L145), boundary [pair rows 48, 223, 224, and 525](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L48), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and the [Killing Fields audit supplement](KILLING_FIELDS_AUDIT_REVIEW.md).

### Priority 185 — The Para-Elemental Plane of Magma

Magma has 129 rooms and 612/612 resolving exits, all local; its weak components are 128 and 1, with no active WLD boundaries. Raw grid/control traversal confirms the one-way control leaves, isolated info room, and no-exit holding room. The all-95 inactive-WLD scan finds 517 incoming source records: 260 non-overlapping Duris3 records and one Vodgh record, plus 256 exits from same-numbered Duris3 rooms titled The Ocean. Only the 261 non-overlapping-source records enter the curated source-only total, now 3,224; Surf remains 159. The authored QST file is present while the legacy qst/AREA index omits magma; that index is not the production list. See [coverage row 168](ZONE_COVERAGE_REGISTER.csv#L168), [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), [Vodgh candidate row 118](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L118), [Magma audit supplement](MAGMA_AUDIT_REVIEW.md), and [Magma dossier](../zone-stories/MAGMA.md).

### Priority 186 — Mazzolin

Mazzolin has 136 rooms and 288/288 resolving exits (286 local, two outbound) in one weak component. Its two reciprocal active UnderDark boundaries are Mazzolin #21300 south to #803053 and west to #802652; raw root traversal reaches all rooms and returns through authored directed edges to 93/136. The all-95 inactive-WLD sweep finds one non-overlapping source-only `ud_map2.wld` approach, #1125718 south to #21300; curated totals rise to 3,225 overall, with 13 reviewed `ud_map2` approaches and Surf still 159. See [coverage row 173](ZONE_COVERAGE_REGISTER.csv#L173), boundary [pair rows 266 and 717](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L266), [ud_map2 candidate row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114), [Mazzolin audit supplement](MAZZOLIN_AUDIT_REVIEW.md), and [Mazzolin dossier](../zone-stories/MAZZOLIN.md).

### Priority 187 / Checkpoint 147 — Myconid Mushroom Forest

The active Myconid WLD has 93 rooms and 224/224 resolving exits (222 local and two outbound) in one weak component. Two UnderDark boundaries reciprocate: #2300 up ↔ #836064 down and #2392 down ↔ #811016 up (pair rows 311 and 723). Two inactive source-only, non-overlapping approaches reach #2300: `ud_map0.wld` #1102287 down and `ud_map2.wld` #1124242 down. These are not active ingress. The curated source-only total rises to 3,227; `ud_map0` and `ud_map2` reviewed subsets are two and 14, and Surf remains 159. See the [Myconid audit supplement](MYCONID_AUDIT_REVIEW.md), [coverage row 197](ZONE_COVERAGE_REGISTER.csv#L197), and candidate rows [112](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L112) and [114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114).

### Priority 188 / Checkpoint 148 — Vargan II

The active Vargan II WLD has 119 rooms and 249/249 resolving exits (248 local and one outbound), all in one weak component. Its sole boundary is reciprocal: #7800 up ↔ Vargan #2182 down (pair rows 762 and 770). The all-95 inactive-WLD scan finds no VNUM overlap or target into Vargan II, so no source-only approach is added. Curated totals remain 3,227 overall; the reviewed Surf subset remains 159. Raw state values on the internal legacy doors are documented separately from fresh-load state in the [Vargan II audit supplement](V2_AUDIT_REVIEW.md) and [existing dossier](../zone-stories/V2.md).

### Priority 189 / Checkpoint 149 — Phantasmagoric Caverns

The active Valdrak WLD has 100 rooms and 234/234 resolving exits (232 local, two outbound) in one weak component; root #53100 reaches and returns from all 100 rooms through authored directions. Both UnderDark boundaries at #53100 reciprocate (#53100 south ↔ #832131 north and west ↔ #831730 east; pair rows 740 and 764). The all-95 inactive-WLD sweep adds one non-overlapping, source-only approach, `ud_map2.wld` #1125389 down to #53100. It is not active ingress. Curated totals rise to 3,228 overall; the reviewed `ud_map2` subset rises to 15 and Surf remains 159. See [coverage row 332](ZONE_COVERAGE_REGISTER.csv#L332), boundary [pair rows 740 and 764](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L740), and the [Valdrak audit supplement](VALDRAK_AUDIT_REVIEW.md).

### Priority 190 / Checkpoint 150 — The Bronze Citadel

The active Bctdl WLD has 77 rooms and 160/160 resolving exits, all local, in one weak component. Realm #14208 down to #32469 and Winterhaven #55633 east to #32480 are one-way inbound edges (pair rows 385 and 782); Bctdl has no active outbound WLD exit. From root #32420, authored traversal reaches 71/77 with the raw flag-8 edge #32471 D4 to #32481 excluded and 76/77 with it available; all 77 return to root in either pass. The all-95 inactive-WLD scan finds no room overlap or approach into Bctdl. Curated totals remain 3,228 overall, with 15 reviewed `ud_map2` approaches and 159 for Surf. The separately declared Avernus type-25 gate target and other typed dependencies are documented in the [Bctdl audit supplement](BCTDL_AUDIT_REVIEW.md); a non-WLD target is not counted as a WLD boundary.

### Priority 191 / Checkpoint 151 — Brimeforge

The active Brimeforge map has 49 rooms and 98/98 resolving exits (96 local, two outbound) in one weak component; raw authored traversal from #131001 reaches and returns from all rooms. Both cross-area connections reciprocate: #131036 west ↔ Connector Zones #53802 east, and #131037 east ↔ Mini Zones #5879 west (pair rows 56/87 and 57/269). The all-95 inactive-WLD sweep finds 196 Duris3 records targeting Brimeforge: 100 non-overlapping source-only approaches, plus 96 east/west matches from 49 same-numbered Duris3 map rooms. Only the 100 non-overlapping records enter the curated total, now 3,328 overall; `ud_map2` remains 15 and Surf remains 159. No Surf approach was found. These records are not active ingress. See [coverage row 40](ZONE_COVERAGE_REGISTER.csv#L40), [Duris3 coverage row 387](ZONE_COVERAGE_REGISTER.csv#L387), the [Brimeforge audit supplement](BRIMEFORGE_AUDIT_REVIEW.md), and [pair rows 56, 57, 87, and 269](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L56).

### Priority 192 / Checkpoint 152 — The Bugger Caves

The active Bugger map has 28 rooms and 56/56 resolving exits (54 local, two outbound) in one weak component; authored traversal from #6400 reaches and returns from all rooms. Both cross-area connections reciprocate: #6400 north ↔ UnderDark #807099 south, and #6426 up ↔ Ixxillikor #4380 down (pair rows 65/692 and 64/207). The all-95 inactive-WLD sweep finds no room-VNUM overlap or target into Bugger, so no source-only approach is added. Curated totals remain 3,328 overall, 15 for ud_map2, and 159 for Surf. The compiled room reference and other typed-source findings are documented in the [Bugger audit supplement](BUGGER_AUDIT_REVIEW.md); no inactive map edge is counted as active ingress.

### Priority 193 / Checkpoint 153 — Dirk’nspire Stronghold

The active Dirk’nspire map has 86 rooms and 177/177 resolving exits (176 local, one outbound) in one weak component. Authored traversal from #96800 reaches and returns from all 86 rooms. Its sole Valinhav boundary is reciprocal (pair rows 116 and 765). The all-95 inactive-WLD sweep finds no source-room overlap or target into Dirk’nspire, so no source-only approach is added. Curated totals remain 3,328 overall, 15 for ud_map2, and 159 for Surf. The two Alatorin QST consumers of object #96835, one Alatorin rubble reset of #96820, Alatorin sidecar mirrors, three compiled #96803 room references, and unique title/history boundary are recorded in the [Dirk’nspire supplement](DIRKN_AUDIT_REVIEW.md).

### Priority 194 / Checkpoint 154 — Storm Port

The active Storm Port map has 147 rooms and 320 authored exits (314 local, six outbound); 319 resolve, with one unresolved Duris3 target from #22541. The local graph has weak components of 139, 7, and 1; authored traversal from #22400 reaches 137/147 and returns 139/147. Four Surface boundaries reciprocate. Storm Port Stronghold and Winterhaven each have a one-way inbound record; Sarmiz has one-way outbound. The all-95 inactive-WLD sweep finds no room overlap and four non-overlapping source-only Surf approaches to #22479, #22401, and #22432. Curated totals rise to 3,332 overall and 163 for Surf; ud_map2 remains 15. See the [Storm Port audit supplement](STORMPORT_AUDIT_REVIEW.md), [coverage row 284](ZONE_COVERAGE_REGISTER.csv#L284), [boundary pair rows 452, 455, 456, 599, and 815](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L452), and [unresolved target row 54](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L54).

### Priority 195 / Checkpoint 155 — Arachdrathos

The active arac-web map has 190 rooms and 457/457 resolving exits (446 local, 11 outbound) in one weak component. Root #36400 reaches 189/190 and all 190 have a directed path back; #36568 is the one room outside root-forward WLD traversal and is the documented adjacent shop stock room. The 11 outbound edges pair reciprocally with aracguil (seven), llzazan (three), and arac_wild (one). Two extra one-way Heavens exits target inn room #36564, including one whose text incorrectly says Underdark. The all-95 inactive-WLD sweep finds no source-room overlap or approach; curated source-only totals remain 3,332 overall, 163 Surf, and 15 ud_map2. See the [Arachdrathos audit supplement](ARAC_WEB_AUDIT_REVIEW.md), [coverage row 9](ZONE_COVERAGE_REGISTER.csv#L9), [pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L16), and [Heavens inbound rows](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L3).

### Priority 196 / Checkpoint 156 — Arcium

The active Arcium map has 115 rooms and 268/268 resolving exits (266 local and two outbound) in one weak component. Authored directed traversal from #85700 reaches and returns from every room. The two outbound exits to Surface have reciprocal returns. The all-95 inactive-WLD sweep finds no Arcium room overlap and one non-overlapping source-only Surf exit: #283370 D0 to #85700. It is not active ingress. Curated source-only totals are now 3,333 overall, 164 for Surf, and 15 for ud_map2. Dream transport targets remain distinct from WLD edges. See the [Arcium audit supplement](ARCIUM_AUDIT_REVIEW.md), [coverage row 12](ZONE_COVERAGE_REGISTER.csv#L12), pair rows [Arcium-to-Surface](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L24) and [Surface-to-Arcium](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L463), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Priority 197 / Checkpoint 157 — Future Ceothia

The active Future Ceothia map has 278 rooms and 1,016/1,016 resolving exits, all local, in weak components of 276 and 2. From #81400, raw directed traversal reaches 271 rooms and returns from 275; #81671–#81677 are outside root-forward reach and #81675–#81677 have no authored return. Its only active WLD boundary is Realm #14210 northeast to #81677, with no outbound Ceothia WLD exit. Past and local type-25 portals provide separate time-device targets to #81400 and #81676; Future portal #81413 targets Present room #80980. The all-95 inactive-WLD sweep finds no room overlap or target into Future Ceothia, so source-only totals stay 3,333 overall, 164 Surf, and 15 ud_map2. These portal routes are not WLD edges or proof of player traversal. See the [Future Ceothia audit supplement](FUTURE_CEOTHIA_AUDIT_REVIEW.md), [coverage row 51](ZONE_COVERAGE_REGISTER.csv#L51), Realm [pair row 387](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L387), and inbound [exit row 264](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L264).

### Priority 198 / Checkpoint 158 — Treasure Caves

The active Dungeon map has 116 rooms and 236/236 resolving exits (234 local, two outbound) in one weak component. Authored traversal from #93000 reaches and returns from all 116 rooms. Its Arachdrathos Wilderness and Underdark boundaries both reciprocate. The all-95 inactive-WLD sweep finds no room overlap or exit into Treasure Caves, so source-only totals remain 3,333 overall, 164 Surf, and 15 ud_map2. See the [Treasure Caves audit supplement](TREASURE_CAVES_AUDIT_REVIEW.md), [coverage row 83](ZONE_COVERAGE_REGISTER.csv#L83), and pair summary rows [20](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L20), [126](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L126), [127](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L127), and [699](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L699).

### Priority 199 / Checkpoint 159 — Jindon the Deathwood Forest

The active Jindon map has 90 rooms and 193/193 resolving exits (192 local, one outbound), in weak components of 65 and 25. Raw directed reach and return from #82000 each cover 65/90; the other component is #82065–#82089. Its sole active WLD boundary reciprocates with Surface #619653. The all-95 inactive-WLD sweep adds one non-overlapping source-only Surf approach, #244020 north to #82063; it is not active ingress. Curated source-only totals are 3,334 overall, 165 Surf, and 15 ud_map2. See the [Jindon audit supplement](JINDON_DEATHWOOD_AUDIT_REVIEW.md), [coverage row 139](ZONE_COVERAGE_REGISTER.csv#L139), boundary [pair rows 212 and 523](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L212), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Priority 200 / Checkpoint 160 — Lava Springs

The active Lava Springs map has 65 rooms and 147/147 resolving exits (146 local, one outbound) in one weak component. Authored traversal from #99700 reaches and returns from all 65 rooms. Its only active WLD boundary reciprocates with Surface #565027. The all-95 inactive-WLD sweep adds one non-overlapping source-only Surf approach, #259346 north to #99700; it is not active ingress. Curated source-only totals are 3,335 overall, 166 Surf, and 15 ud_map2. See the [Lava Springs audit supplement](LAVA_SPRINGS_AUDIT_REVIEW.md), [coverage row 155](ZONE_COVERAGE_REGISTER.csv#L155), boundary [pair rows 243 and 531](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L243), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Priority 201 / Checkpoint 161 — Lylr-Meop

The active Lylr-Meop map has 64 rooms and 134/134 resolving exits (132 local, two outbound), in weak components of 40 and 24. Raw directed reach and return from #82200 each cover 40/64; the other component is #82240–#82263. Both active exits lead one way to Surface: #82200 north to #540876 and #82240 south to #539281. The all-95 inactive-WLD sweep adds two non-overlapping source-only Surf approaches, #222832 north to #82240 and #224629 south to #82200; they are not active ingress. Curated source-only totals are 3,337 overall, 168 Surf, and 15 ud_map2. See the [Lylr-Meop audit supplement](LYLR_MEOP_AUDIT_REVIEW.md), [coverage row 167](ZONE_COVERAGE_REGISTER.csv#L167), outbound [pair row 258](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L258), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Priority 202 / Checkpoint 162 — The Halfling Silver Mine

The active Mining map has 69 rooms and 153/153 resolving exits (152 local, one outbound) in one weak component. Raw directed traversal from #4900 reaches 64 rooms and has 68 rooms with a return path. Its single active WLD boundary reciprocates with Surface. The all-95 inactive-WLD sweep finds 160 target matches into Mining, including 69 source-room VNUM overlaps in unregistered Alterian maps. Nine non-overlapping source-only exits are curated and the other 151 overlap records are excluded: three from Alterian Mountains, five from Alterian Wilderness, and one from Surf. Curated totals rise to 3,346 overall, 169 Surf, and 15 ud_map2. See the [Mining audit supplement](MINING_AUDIT_REVIEW.md), [coverage row 175](ZONE_COVERAGE_REGISTER.csv#L175), reciprocal [pair rows 268 and 541](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L268), and candidate rows [7](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L7), [8](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L8), and [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Priority 203 / Checkpoint 163 — Forest of Mir

The active Mir map has 281 rooms and 558/558 resolving exits (556 local, two outbound), in weak components of 271 and 10. Raw directed reach from #41900 covers 237 rooms; 271 rooms have return paths. Its two active boundaries both reciprocate with Highway: Mir #42137 south to #41316 and #42152 east to #41585. The all-95 inactive-WLD sweep finds no room overlap or exit target into Mir, so curated source-only totals remain 3,346 overall, 169 Surf, and 15 ud_map2. See the [Mir audit supplement](MIR_AUDIT_REVIEW.md), [coverage row 178](ZONE_COVERAGE_REGISTER.csv#L178), reciprocal [pair rows 194 and 276](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L194), and the [Highway coverage row 128](ZONE_COVERAGE_REGISTER.csv#L128).

### Priority 204 / Checkpoint 164 — The Shadow Forest

The active Mist map has 73 rooms and 199/199 resolving exits (197 local, two outbound) in one weak component. Raw directed reach from #6300 covers 67 rooms, and 72 have return paths. Four active inbounds come from Minizones #5804 north to #6368, NewFor #53300 south to #6366, and Surface #628063 west/#628462 north to #6300. The two outbound Mist edges at #6300 reciprocate with those Surface rooms. The all-95 inactive-WLD sweep adds three non-overlapping source-only exits to #6368: two from alterian_wilderness.wld and one from aopal.wld. Curated totals rise to 3,349 overall; Surf remains 169 and ud_map2 remains 15. See the [Shadow Forest audit supplement](SHADOW_FOREST_AUDIT_REVIEW.md), [coverage row 180](ZONE_COVERAGE_REGISTER.csv#L180), and boundary pair rows [271](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L271), [281](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L281), [316](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L316), and [545](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L545).

### Priority 205 / Checkpoint 165 — Mosswood

The active Mosswood map has 161 rooms and 399/399 resolving exits (398 local, one outbound), in one weak component. Raw directed reach from #23400 covers 157 rooms; all 161 have return paths. Its sole active WLD boundary reciprocates with Surface: Moss #23400 north to #611257 and Surface #611257 south to #23400. The all-95 inactive-WLD sweep adds three non-overlapping source-only Surf exits: #229223 south, #229522 east, and #229524 west to #23400. Curated totals rise to 3,352 overall, 172 for Surf, and 15 for ud_map2. See the [Mosswood audit supplement](MOSSWOOD_AUDIT_REVIEW.md), [coverage row 190](ZONE_COVERAGE_REGISTER.csv#L190), and reciprocal [pair rows 295 and 551](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L295).

### Priority 206 / Checkpoint 166 — The Reliquary Nexus of the Roper Den

The active Roper Den map has 25 rooms and 51/51 resolving exits (50 local, one outbound) in one weak component. Authored reach and return from #130400 cover all 25 rooms. Its only active boundary reciprocates with Underdark: Roper Den #130400 south to #811645 and Underdark #811645 north to #130400. The all-95 inactive-WLD sweep finds 100 target records in Duris3.wld: 52 start in non-overlapping source rooms and are curated as source-only approaches; 48 start in 25 same-numbered Duris3 rooms and are excluded as overlapping map identities. Curated totals rise to 3,404 overall; Surf remains 172 and ud_map2 remains 15. See the [Roper Den audit supplement](NEXUS_ROPER_AUDIT_REVIEW.md), [coverage row 210](ZONE_COVERAGE_REGISTER.csv#L210), reciprocal [pair rows 322 and 725](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L322), and [Duris3 coverage row 387](ZONE_COVERAGE_REGISTER.csv#L387).

### Priority 207 / Checkpoint 167 — Ny’Neth’s Stronghold

The active Nyneth2 map has 101 rooms and 210/210 resolving exits (210 local, none outbound), in seven weak components sized 78, 6, 5, 4, 3, 3, and 2. Raw directed reach from #23000 covers 76 rooms; 77 have return paths. Its sole active WLD boundary is an unpaired White Harbor #55632 north exit to Nyneth2 #23098. The all-95 inactive-WLD sweep finds no room overlap or target exit, so curated source-only totals remain 3,404 overall, 172 for Surf, and 15 for ud_map2. See the [Ny’Neth’s Stronghold audit supplement](NYNETH2_AUDIT_REVIEW.md), [coverage row 218](ZONE_COVERAGE_REGISTER.csv#L218), and active boundary [exit row 780](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L780) / [summary row 807](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L807).

### Priority 208 / Checkpoint 168 — Ny’Neth’s Stronghold Continued

The active Nyneth3 map has 118 rooms and 232/232 resolving exits (232 local, none outbound), in weak components of 98, 11, and 9. Raw directed reach from #38700 covers 11 rooms; only two have return paths. Two active one-way boundaries enter Treasure Room #38811: Realm #14208 southwest and White Harbor #55633 up. The all-95 inactive-WLD sweep finds no overlap or target exit, so curated source-only totals remain 3,404 overall, 172 for Surf, and 15 for ud_map2. See the [Ny’Neth’s Stronghold Continued audit supplement](NYNETH3_AUDIT_REVIEW.md), [coverage row 219](ZONE_COVERAGE_REGISTER.csv#L219), exit-register [row 249](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L249) and [row 792](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L792).

### Priority 209 / Checkpoint 169 — Orrak

Orrak has 39 rooms and 77/77 resolving exits (76 local and one outbound) in one weak component; root #9000 reaches all 39 rooms and all 39 have a return path. The active Orrak #9000 south / Surface #610629 north boundary is reciprocal. The inactive-WLD sweep adds one non-overlapping source-only Surf exit, #260629 north to Orrak #9000; it is not active ingress. Curated totals rise to 3,405 overall and 173 for Surf, while ud_map2 remains 15. See the [Orrak audit supplement](ORRAK_AUDIT_REVIEW.md), [coverage row 225](ZONE_COVERAGE_REGISTER.csv#L225), active exit-register [rows 183 and 1598](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L183), and pair-summary [rows 336 and 570](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L336).

### Priority 210 / Checkpoint 170 — Sea Kingdom

The active Sea Kingdom map has 225 rooms and 558/558 resolving exits (557 local, one outbound), in weak components of 117, 65, 22, 16, and 5. Raw directed traversal from #31500 reaches 113 rooms; 23 rooms have a return path. Sea Kingdom #31500 up ↔ Plane Water #23281 down is reciprocal. White Harbor #55634 southeast has a separate one-way inbound to Sea Kingdom #31668 with no reverse WLD edge. The all-95 inactive-WLD sweep finds no room overlap or target into Sea Kingdom, so source-only totals remain 3,405 overall, 173 for Surf, and 15 for ud_map2. See the [Sea Kingdom audit supplement](SEAKNGDM_AUDIT_REVIEW.md), [coverage row 262](ZONE_COVERAGE_REGISTER.csv#L262), active exit-register [rows 380](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L380), [436](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L436), and [806](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L806), and pair-summary [rows 353](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L353), [422](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L422), and [811](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L811).

### Priority 211 / Checkpoint 171 — Turolopolis Zoo

The active Zoo map has 62 rooms and 141/141 resolving exits (140 local, one outbound) in one weak component. With the closed/secret/blocked #53019 down exit unavailable, raw root #53000 reach is 60/62 and return is 62/62; with all authored local exits available, both are 62/62. The sole active boundary reciprocates with the Ruins of Turolopolis: Willem #7123 west to Zoo #53000 and Zoo #53000 east to Willem #7123. The all-95 inactive-WLD sweep finds no room overlap or exit target into the Zoo, so curated source-only totals remain 3,405 overall, 173 for Surf, and 15 for ud_map2. See the [Turolopolis Zoo audit supplement](TUROLZOO_AUDIT_REVIEW.md), [coverage row 319](ZONE_COVERAGE_REGISTER.csv#L319), active exit-register [rows 132](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L132) and [656](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L656), and pair-summary [rows 682](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L682) and [828](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L828).

### Priority 212 / Checkpoint 172 — Valley of Crushk

The active Valley map has 85 rooms and 182/182 resolving exits (180 local, two outbound) in one weak component; raw directed traversal from #2001 reaches and returns from all rooms. Mini Zones #5806 south enters Valley #2001 without a reverse WLD edge. Pharr Valley #40300 west to #2006 is reciprocal with Valley #2006 east to #40300. Valley #2060 south exits to Surface #548133, whose selected active room has east, south, and west exits but no north return. The all-95 inactive-WLD sweep found no room overlap and one source-only Surf approach, #232517 north to #2060. Curated source-only totals are 3,406 overall, 174 for Surf, and 15 for ud_map2. See the [Valley audit supplement](VALLEY_CRUSHK_AUDIT_REVIEW.md), [coverage row 334](ZONE_COVERAGE_REGISTER.csv#L334), active exit-register [rows 33, 34, 99, and 551](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L33), pair-summary [rows 273, 341, 767, and 768](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L273), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Priority 213 / Checkpoint 173 — The Forgotten Forest

The active Forest map has 92 rooms and 283/283 resolving exits (282 local, one outbound) in one weak component. Raw directed traversal from #82701 reaches 90 rooms and has return paths from 91. The only active boundary is reciprocal with Surface: Surface #516090 east to Forest #82701, and Forest #82701 west to Surface #516090. The all-95 inactive-WLD sweep found no room overlap and 15 non-overlapping source-only `surf.wld` exits into Forest rooms; these are not active ingress. Curated totals are 3,421 overall, 189 for Surf, and 15 for ud_map2. See the [Forest audit supplement](FORGOTTEN_FOREST_AUDIT_REVIEW.md), [coverage row 101](ZONE_COVERAGE_REGISTER.csv#L101), exit-register [rows 995 and 1340](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L995), pair-summary [rows 147 and 503](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L147), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Priority 214 / Checkpoint 174 — The Rice Fields

The active `jademini` map has 63 rooms and 153 authored exits: 149 local, three reciprocal Surface departures, and one negative target exit from #77262 south to #-1 that the loader discards. The raw directed graph has weak components of 58 and 5 rooms; root #77200 reaches 23 rooms and has return paths from 58. The all-95 inactive-WLD sweep found no overlap and four non-overlapping source-only approaches: IC3 #420000 north to #77262; Surf #254130 down to #77258, #277695 north to #77220, and #278293 north to #77218. These are not active ingress. Curated totals are 3,425 overall, 192 for Surf, and 15 for ud_map2. See the [Rice Fields audit supplement](RICE_FIELDS_AUDIT_REVIEW.md), [coverage row 138](ZONE_COVERAGE_REGISTER.csv#L138), active exit-register [rows 887–889](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L887), [1451](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1451), [1672](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1672), and [1674](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1674), pair-summary [rows 211](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L211) and [522](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L522), plus candidate rows [53](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L53) and [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Priority 215 / Checkpoint 175 — The Bandit Canyons

The active Bandit Canyons map has 76 rooms and 163/163 resolving exits (162 local, one outbound) in one weak component. Raw root #98500 reaches 75 rooms and has return paths from all 76. Its sole active boundary reciprocates with Surface #536467: Bandit #98500 south to Surface and Surface north back to #98500. The all-95 inactive-WLD sweep found no room overlap and one source-only `surf.wld` exit, #234839 west to #98500; this is not active ingress. Curated totals are 3,426 overall, 193 for Surf, and 15 for ud_map2. See the [Bandit audit supplement](BANDIT_CANYONS_AUDIT_REVIEW.md), [coverage row 23](ZONE_COVERAGE_REGISTER.csv#L23), active exit-register [rows 1177](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1177) and [1404](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1404), pair-summary [rows 36](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L36) and [466](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L466), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Priority 217 / Checkpoint 177 — Barrow of the Quiosho

The active Barrow map has 207 rooms and 667/667 resolving exits (664 local, three outbound), in weak components of 206 and 1. Raw traversal from #20600 reaches and returns from 206/207 rooms; holding room #20804 is isolated from the ordinary local exit graph. Three active boundaries reciprocate with Surface at #20600/#609100, #20611/#612309, and #20615/#613104. The all-95 inactive-WLD sweep found four non-overlapping source-only `surf.wld` approaches: #275979 and #275980 east to #20600, #276595 west to #20611, and #277493 east to #20615. They are not active ingress. Curated totals rise to 3,430 overall, 197 for Surf, and 15 for ud_map2. See the [Barrow audit supplement](ELFTOMB_AUDIT_REVIEW.md), [coverage row 87](ZONE_COVERAGE_REGISTER.csv#L87), active exit-register [rows 339–341](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L339), Surface return rows [1595](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1595), [1604](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1604), and [1606](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1606), pair-summary [rows 131](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L131) and [496](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L496), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Priority 218 / Checkpoint 178 — Keep of Evil

The active Keep map has 147 rooms and 401/401 resolving exits (400 local, one outbound), in weak components of 91 and 56. Raw root #44801 reach/return is 91/147. Surface #583837 east and Keep #44801 west reciprocate; White Harbor #55634 up is a separate one-way inbound to #44888. The all-95 inactive-WLD sweep found one non-overlapping source-only Surf approach, #270471 east to #44801; it is not active ingress. Curated totals rise to 3,431 overall, 198 for Surf, and 15 for ud_map2. See the [Keep of Evil audit supplement](EVKEEP_AUDIT_REVIEW.md), [coverage row 93](ZONE_COVERAGE_REGISTER.csv#L93), active exit-register [row 621](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L621), Surface return [row 1527](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1527), White Harbor inbound [row 801](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L801), pair-summary [rows 135](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L135), [498](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L498), and [791](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L791), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Priority 219 / Checkpoint 179 — New Cave city

The active map has 73 rooms and 155/155 resolving exits (152 local, three outbound) in one weak component of 73. Raw traversal from #15100 reaches and returns from all 73 rooms. Its three active boundaries reciprocate: New Cave city #15102 south/Underdark #749656 north; #15161 east/Surface #584184 west; and #15161 south/Surface #584583 north. The all-95 inactive-WLD sweep adds one non-overlapping source-only Surf approach, #277907 north to #15102; it is not active ingress. Curated totals rise to 3,432 overall, 199 for Surf, and 15 for ud_map2. See the [New Cave city audit supplement](NEW_CAVECITY_AUDIT_REVIEW.md), [coverage row 201](ZONE_COVERAGE_REGISTER.csv#L201), active exit-register [rows 268–270](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L268), inbound [rows 1531, 1533, and 1707](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1531), pair-summary [rows 314–315](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L314), return [rows 557 and 724](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L557), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Priority 220 / Checkpoint 180 — Lost City of Tikitzopl

The active map has 340 rooms and 855/855 resolving exits (853 local, two outbound) in one weak component of 340. Raw traversal from #43700 reaches and returns from all 340 rooms. Both active boundaries reciprocate: City #43901 west/Surface #561015 east, and City #44039 down/Lost Temple #44101 up. The Temple’s type-25 return to City room #43704 remains a separate object route. The all-95 inactive-WLD sweep adds one non-overlapping source-only Surf approach, #248527 east to #43901; it is not active ingress. Curated totals rise to 3,433 overall, 200 for Surf, and 15 for ud_map2. See the [Lost City audit supplement](TIKIT_AUDIT_REVIEW.md), [coverage row 303](ZONE_COVERAGE_REGISTER.csv#L303), active exit-register [rows 617–619](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L617), [Surface inbound row 1467](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1467), pair-summary [rows 609 and 659–661](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L609), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Coverage checkpoint 181 — Ako Village

The active Ako map has 52 rooms and 107/107 resolving exits (106 local, one outbound) in one weak component. Raw traversal from #3700 reaches and returns from all 52 rooms over authored local exits. The only active WLD boundary is Ako #3751 west to Surface #544136; there is no active WLD arrival into Ako. The inactive-WLD sweep found one separate, non-overlapping source-only Surf approach, #224164 east to #3751, which is not active ingress. Curated source-only totals rise to 3,434 overall and 201 for Surf, while `ud_map2` remains 15. See the [Ako audit supplement](AKO_AUDIT_REVIEW.md), [coverage row 5](ZONE_COVERAGE_REGISTER.csv#L5), active exit-register [row 47](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L47), pair-summary [row 6](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L6), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 182 — An Ancient Bridge

The active Ancient Bridge map has 37 rooms and 74/74 resolving exits (72 local and two outbound) in one weak component. Raw traversal from #85600 reaches and returns from all 37 rooms over authored local exits. Both outbound edges lead one way to Surface: #85600 west to #572443 and #85623 east to #572446; there is no active inbound or reverse WLD edge. The all-95 inactive-WLD sweep adds three non-overlapping, source-only approaches: aopal.wld #43343 north to #85606, surf.wld #234341 east to #85600, and surf.wld #234343 west to #85623. These are not active ingress. Curated source-only totals rise to 3,437 overall and 203 for Surf; ud_map2 remains 15. Preserve the aopal and Surf records with candidate rows [10](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L10) and [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). See the [Ancient Bridge audit supplement](ANCIENTB_AUDIT_REVIEW.md), coverage row [7](ZONE_COVERAGE_REGISTER.csv#L7), active exit-register [rows 1082–1083](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1082), and pair-summary [row 14](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L14).

### Checkpoint 183 — Ant Forest

The active Ant Forest map has 41 rooms and 122/122 resolving exits (121 local and one outbound) in one weak component. From #10900, raw directed traversal over all authored local edges reaches 39/41 rooms and has a return path from 40/41; #10939/#10940 are not root-forward reachable, and #10940 has no return. Excluding the two internal `EX_BLOCKED` exits reduces root reach to 15/41. Local `ITEM_SWITCH` objects connect `CMD_CRY` to #10904 D5 and `CMD_TOUCH` to #10915 D0, clearing the corresponding blocked bits; actual room/search/reset behavior is not live-qualified. The sole active boundary is reciprocal with Surface at #10900/#526267. The all-95 inactive-WLD sweep adds one non-overlapping source-only `surf.wld` approach, #211552 east to #10900. It is not active ingress. Curated source-only totals rise to 3,438 overall and 204 for Surf; `ud_map2` remains 15. Preserve the source-only record with existing Surf candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). See the [Ant Forest audit supplement](ANTS_AUDIT_REVIEW.md), coverage row [8](ZONE_COVERAGE_REGISTER.csv#L8), active exit-register rows [206](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L206) and [1374](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1374), and reciprocal pair-summary [rows 15 and 461](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L15).

### Checkpoint 184 — Arachdrathos Wilderness

The active Arachdrathos Wilderness map has 200 rooms and 645/645 resolving exits (640 local and five outbound) in one weak component. Raw directed traversal from #36000 reaches and returns from all 200 rooms. Five active boundaries are reciprocal: Surface at #36000/#589915 and #36046/#588318, Dungeon at #36004/#93115, Arac-Web at #36197/#36588, and Llzazan at #36199/#36200. All WLD exit flags and keys are zero, with no ZON door resets. The all-95 inactive-WLD sweep adds one non-overlapping source-only `surf.wld` approach, #287030 east to #36000; it is not active ingress. Curated source-only totals rise to 3,439 overall, 205 for Surf, and 15 for `ud_map2`. Preserve the record with existing Surf candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). See the [Arachdrathos Wilderness audit supplement](ARAC_WILD_AUDIT_REVIEW.md), coverage row [10](ZONE_COVERAGE_REGISTER.csv#L10), active exit-register [rows 472–476](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L472), and pair-summary rows [16](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L16) and [19–22](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L19).

### Checkpoint 185 — Arachdrathos Guilds

The active Guilds map has 78 rooms and 150/150 resolving exits (143 local, seven outbound) in weak components of 25, 12, 11, 10, 10, and 10. From #36700, all-authored-edge traversal reaches and returns from 25/78 rooms; excluding its ten `EX_BLOCKED` local exits yields reach of 19/78 and return of 25/78. Seven active boundaries reciprocate with Arac-Web, two at Zarbonesti and one at each of the other five houses. The local ZON file places ten ITEM_SWITCH objects against the ten blocked passages and has 44 door resets; see the [switch table and reset evidence](ARACGUIL_AUDIT_REVIEW.md#the-ten-reset-switches). The all-95 inactive-WLD sweep finds no overlap or source-only approach into `aracguil`. Curated source-only totals remain 3,439 overall, 205 for Surf, and 15 for `ud_map2`. Add no candidate row. See the [Arachdrathos Guilds audit supplement](ARACGUIL_AUDIT_REVIEW.md), coverage row [11](ZONE_COVERAGE_REGISTER.csv#L11), active exit-register rows [482–488](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L482) and [529–535](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L529), and pair-summary rows [17](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L17) and [23](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L23).

### Checkpoint 186 — Ard'gral

The active 64-room plane has 256/256 resolving exits, all local, in one weak component. Raw directed traversal from #30401 reaches and returns from all 64 rooms. The all-95 inactive-WLD sweep finds twelve `ygg.wld` room-ID overlaps (#30401–#30412), 21 authored Yggdrasil exits into those colliding IDs, and one source-only room #30400 exit to #30401. Because `ygg` also defines #30401 as its own room, that apparent ingress is owner-ambiguous and is not counted as active access or a curated candidate. Source-only totals remain 3,439 overall, 205 for Surf, and 15 for `ud_map2`; add no candidate row for Ard'gral.

The separate typed network has twelve fixed reset rifts to the Ethereal Plane, the four elemental planes, Killing Fields, Moonshae, Vella's Bordello, Bloodstone Keep, the Caves of Mt. Skelenak, the Deep Ravine of Passage, and the Astral Plane. Six reset gateways use the random surface-map callback. The illusionist spell creates a one-way portal to a random room in Ard'gral, subject to its source checks. Active Alatorin resets type-25 object #83141 in room #83342 to target Ard'gral #30453. Keep active `ardgral` DO NOT MOVE and `ygg` RETAIN IN PLACE under its existing candidate row; see the [Ard'gral audit supplement](ARDGRAL_AUDIT_REVIEW.md), coverage row [13](ZONE_COVERAGE_REGISTER.csv#L13), and [Yggdrasil candidate row 122](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L122).

### Checkpoint 187 — Arena

The active arena WLD has 87 rooms and 214/214 resolving exits, all local, in weak components of 81, 2, 2, and 2. The battle-map component (#67906–#67986) has 81/81 raw directed reach and return from #67906; the three two-room staging pairs are disconnected in the WLD graph. There are no active cross-area exits, and the all-95 inactive-WLD sweep found no overlap, target, or non-overlapping source-only approach. Curated source-only totals remain 3,439 overall, 205 for Surf, and 15 for `ud_map2`.

Runtime entry uses compiled `arenaobj_proc` on object #67900 rather than a WLD edge. Its three race-war teams are placed in #67904/#67905, #67902/#67903, and #67900/#67901; match code transfers contestants into #67906–#67986. Active `tharnadia.zon` resets #67900 in room #132574, the only external ZON reset-role dependency found. The code defaults closed and to Deathmatch, despite the `CTF Tournament` title. Return settings need review for all three teams: the Goodie/Evil comments point to #6075/#29700, whose current titles are the Swamp of the Clavikord and Goodie Bitchin Room, while Undead is -1; the portal exit branch routes every non-Evil race to #6075. See the [Arena audit supplement](ARENA_AUDIT_REVIEW.md) and [coverage rows 14 and 297](ZONE_COVERAGE_REGISTER.csv#L14). Keep `arena` DO NOT MOVE; it is not an attic candidate.

### Checkpoint 188 — Astral Plane, Side Areas

The 61-room active map has 141/141 resolving WLD exits: 137 local and four
cross-area. Its weak components are #19826–#19827 (2 rooms), #19828–#19866
(39), #19867–#19884 (18), and #19885–#19886 (2). Raw directed local
traversal from each component's first room reaches all rooms in that
component; only #19885 fails to reach a return path from every room in its
component.

The rift pair #19826/#19827 reciprocates with Astral Main #19755, and the
sphere cluster #19867–#19884 has a reciprocal boundary with Astral Main
#19815. Two other exits are one-way: side-area fortress #19828 descends to
Astral Main #19798, and staging room #19885 descends to Astral Main #19806.
There is no active WLD inbound to either the old-fortress component
#19828–#19866 or the staging/failure pair #19885/#19886. Astral Main #19798
has a separate reciprocal connection to the newer `githyanki_fortress`
#132000; it does not connect back to the side-area fortress. The all-95
inactive-WLD sweep found no source-room overlap, target, or source-only
approach into this area. See active exit-register [rows 319 and 321](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L319)
and [rows 323–327](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L323), plus pair-summary
[rows 27–28](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L27) and [row 31](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L31).

The local reset doors add traversal constraints that the raw graph omits:
four pairs reset closed and locked, four pairs reset closed and unlocked, and
two of the latter have WLD key fields even though their ZON resets do not set
the locked state. The keyed-passage table and exact door resets are in the
[Astral Side Areas audit supplement](ASTRAL_AREAS_AUDIT_REVIEW.md#door-resets-and-keys).
This is source evidence only; it does not establish player access through
each guarded or keyed route.

The two reset-loaded Astral Main pools are compiled `magic_pool` callbacks.
Object #19860 in room #19842 targets active Ethereal room #12479 and applies
its 400-point damage value to entrants below the trusted threshold. Object
#19861 in room #19856 targets #163372, which exists only in unregistered
`Duris3.wld`; in the full-world configuration the room lookup fails, so the
callback reports a broken pool and cannot teleport there. Ethereal Main
object #12460 separately targets Astral Main #19702. Keep Duris3 RETAIN IN
PLACE under existing candidate row [36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36)
until the #163372 destination is resolved; this pool reference is not active
WLD ingress. The [Astral Side Areas audit supplement](ASTRAL_AREAS_AUDIT_REVIEW.md)
records the typed reset owners, compiled callbacks, and limits of this static
reachability review.

### Checkpoint 189 — Astral Main

The active map has 125 rooms and 748/748 resolving exits: 743 internal and
five outbound. All rooms form one weak component, and authored directed
traversal from #19701 reaches and returns from 125/125 rooms when all
authored edges are assumed available. Every WLD door flag and key VNUM is
zero, and the zone has no door-reset commands. The five outbound boundaries
are reciprocal: #19755/#19826 and #19815/#19867 with Astral Side Areas,
#19794/#19400 with Gith Hometown, #19798/#132000 with Githyanki Fortress,
and #19800/#19928 with Astral Tiamat.

Twenty-one active WLD exits enter Astral Main: six from Tribal Oasis, five
from Prison, four from Astral Side Areas, three from Astral Tiamat, and one
each from Gith Hometown, Githyanki Fortress, and Interspace. The one-way
inbound groups are Side Areas #19828 and #19885, Tiamat #19948 and #19955,
Interspace #99999, Prison #7488, and Tribal Oasis #42297. The other five
inbound edges are the reciprocal boundaries listed above. Exact active
records appear in [exit-register rows 154–158](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L154),
[311](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L311), and
[319–329](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L319), with pair summaries in
[rows 27–32](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L27),
[154–155](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L154),
[202](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L202),
[358](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L358), and
[668](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L668).

The all-95 inactive-WLD sweep found no Main room-ID overlap, source-only
target, or source-only approach. Typed portal objects add four active
inbound destinations at #19721/#19733; the Jotun object points to Yggdrasil.
Compiled plane-gate spells and innate shifts use room #19701 as an Astral
zone anchor. These routes are separate from the authored WLD graph, and this
source review does not qualify live player access, reset effects, or portal
arrival safety. Astral Tiamat room #19945 holds pool #19710 to #19946, and
room #19946 holds pool #19715 back to #19945. Another reset pool targets an
unregistered-Duris3-only room; see the
[Astral Main audit supplement](ASTRAL_MAIN_AUDIT_REVIEW.md).

### Checkpoint 190 — Astral Plane, Tiamat

The active map has 31 rooms and 65 authored exits: 61 local, three positive
cross-area, and one negative target dropped by the loader. Sixty-four exits
resolve to active rooms. Weak component sizes are 18, 6, 3, 2, and 2; authored
local traversal from #19928 reaches and returns from 18/18 mountain-and-cave
rooms. The three-room load branch reaches all three from #19947 but has no
return from Astral Main into that branch. These figures ignore door reset
states and compiled object callbacks.

Three positive exits leave the area: #19928 south reciprocates with Astral Main
#19800; #19948 and #19955 descend one-way to Astral Main #19785. The #19946
north exit targets #-1 and is dropped. Nine active WLD exits enter the area:
the reciprocal Main edge plus one-way entrances from Realm #14208, six Tiamat
rooms (#19620, #19624–#19627, and #19638), and Winterhaven #55634. The all-95
inactive-WLD sweep found no room overlap or source-only approach. See active
[exit rows 328–330](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L328), inbound
[Realm row 243](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L243),
[Tiamat rows 313–318](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L313),
[Winterhaven row 800](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L800), and negative
[unresolved row 46](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L46).

Twelve reset commands alter six paired doors, with key fields split between
local keys, Astral Main objects #19872/#19876/#19878, and the #19953 `-2`
sentinel. Astral Main objects #19710/#19715 form reciprocal `magic_pool`
callbacks between rooms #19945 and #19946. Local object #19907 is type 25,
reset in #19902 and #19949, and names Tiamat #19600; no callback assignment
for it was found, so it is not counted as a working route. The detailed
[door, key, callback, and typed-dependency review](ASTRAL_TIAMAT_AUDIT_REVIEW.md)
records builder questions and the limits of static reachability.

### Checkpoint 191 — Asylum

The active Asylum map has 89 rooms and 202/202 resolving authored exits: 201
local and one outbound. All rooms are in one weak component. Raw directed
local traversal reaches and returns from 89/89 rooms from each of its three
active inbound anchors, #9288, #9287, and #9202. The one outbound boundary is
#9288 south to Surface #551667, with a reciprocal Surface north exit.

Three active WLD exits enter Asylum: the reciprocal Surface edge, Pineholl
#16164 south to #9287, and White Harbor #55402 down to #9202. The latter two
have no active WLD return to their source zones. Pineholl's source room is
titled “Quest Mob Load 4” and White Harbor's is an Incarne loading room; their
records do not establish ordinary player access. The all-95 inactive-WLD scan
found no room-ID overlap and one source-only approach, inactive `surf.wld`
#212125 north to #9288. It is not active ingress. The reviewed Surf subset
rises to 206 and the curated source-only approach total to 3,440 overall;
`ud_map2` remains at 15.

All 56 WLD door exits have matching ZON resets: 48 reset closed, six
closed/secret, and two locked/secret with key #9213 on the reciprocal passage
between #9200 and #9201. Raw authored traversal assumes those doors can be
opened or discovered; actual reset timing, key custody, player access, and safe
return remain unqualified. Exact edges are in active [exit-register rows
185](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L185),
[285](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L285),
[754](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L754), and
[1448](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1448), with pair summaries in
[rows 33](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L33),
[342](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L342), and
[779](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L779). Preserve source-only Surf
room #212125 with existing candidate row
[97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). See the
[Asylum audit supplement](ASYLUM_AUDIT_REVIEW.md) for typed dependencies,
source line evidence, and the reachability limits.

### Checkpoint 192 — Avernus

The active Avernus WLD has 392 rooms and 1,437/1,437 resolving exits: 1,436
local and one outbound. Its weak local components have 375, 16, and one room.
From #32031, raw directed local traversal reaches 369/375 rooms in the main
component; all 375 have a directed local path back. The other six unreached
rooms (#32001–#32004, #32006, and #32007) are titled/noted as random mob loads
and direct visitors to contact a god. The separate 16-room component is
#32008–#32023; #32000 is isolated.

Avernus #32031 north to Tiamat #19600 is its sole active cross-area WLD exit,
and Tiamat #19600 south reciprocates. See active exit-register [row
438](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L438), Tiamat reverse [row
312](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L312), and pair summaries [rows
34](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L34) and
[658](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L658). The all-95 inactive-WLD scan
found no overlapping source room and no inactive exit into Avernus.

Seven River-of-Blood current records run north at source speed 50. No Avernus
WLD exit has nonzero door/key fields; there are no ZON `D` resets and no fall
chance records. The zone's 275 source reset commands and reset-mode-0 behavior
are described separately in the [Avernus audit supplement](AVERNUS_AUDIT_REVIEW.md);
boot population does not establish periodic repop or current live contents.

Typed routes include Avernus type-25 object #32013 to Bctdl #32420 and Bctdl
object #32436 back to Avernus #32057, plus six Ravenloft2 items targeting
Avernus rooms. The compiled gate spell can conditionally choose an Avernus room
in its Hell range. These are separate from authored WLD edges. Their reset
placement, dynamic resolution, player access, arrival safety, and return
behavior are source questions, not established live journeys; see the
[Avernus typed-travel table](AVERNUS_AUDIT_REVIEW.md#typed-travel-and-world-connectivity).

### Checkpoint 193 — Black Tunnels

The active Black Tunnels map has 82 rooms and 200/200 resolving authored exits:
198 local and two outbound. All rooms form one weak component. Raw directed
local traversal from either inbound anchor, #10700 or #10721, reaches all
82 rooms, and all 82 have return paths to each anchor when authored exits are
treated as available. A conservative as-reset static traversal excluding
closed, secret, or blocked exits reaches 59/82 from either anchor; it does not
simulate search, opening, picking, passdoor, live reset state, or player access.

The reciprocal WLD boundaries are Black Tunnels #10700 south ↔ Pragtog #10693
north and #10721 east ↔ Khildarak #17377 west. See active exit-register
[rows 202–204](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L202) and Khildarak reverse
[row 303](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L303), plus pair summaries
[rows 46–47](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L46). The all-95 inactive-WLD
scan found no overlapping room IDs or inbound approaches. The area has no
fall-chance or current records.

Ten directed door exits form five reset pairs: two closed/secret, two
closed/locked/secret, four closed, and two closed/locked with key VNUM 0. Every
pair has a matching ZON `D` reset. Four raw WLD state-4 edges are reduced by
the loader's `& 3` mask and do not add closed routes. Zone mode 2 can reset
while occupied, so the source can reapply door states; current timing and live
state remain unqualified. The Khildarak type-25 object #17097 also declares
an `open`-command arrival to #10721 and is reset in Khildarak #17377. Preserve
that route alongside the WLD pair; execution, access, reset placement, arrival
safety, and return were not live-tested. See the [Black Tunnels audit
supplement](BLACK_TUNNELS_AUDIT_REVIEW.md) for exact reset rows and typed
source scans.

### Checkpoint 194 — Bloody Plains

The active Bloody Plains WLD has 199 rooms and 704/704 resolving authored
exits: 698 local and six outbound. All rooms form one weak component. Raw
directed local traversal from each inbound anchor reaches and returns from all
199 rooms when authored exits are available. A static traversal excluding the
two reset-closed shack door edges reaches and returns from 198/199 from every
anchor; this does not model opening, key custody, reset timing, player access,
or live state.

Seven active exits enter Bloody Plains. Killing Fields #33422 east reaches
#33597 without a direct return; its other five east entries reach #33612,
#33627, #33642, #33657, and #33672, each paired with a west exit back to
Killing Fields. Surface #586575 north reaches #33681 and is paired with the
south exit back to Surface. Those five Killing Fields links and the Surface
link are the six outbound edges. The exact one-way Fields edge and local
alternatives should stay visible in navigation decisions.

The two shack exits are reset closed; the #33501 west exit is also locked to
key #33513, reset in crater-bottom room #33695. Twelve chance-fall records
range from 78% to 100%, each with a downward exit; there are no current
records. The 100% crater sequences continue through additional fall-marked
rooms. Loader and movement source identify the chance processing and its
character-state checks, but fall outcome, recovery, and safe return were not
runtime-tested. Reset mode 0 schedules no periodic zone reset event.

The all-95 inactive-WLD scan found no room-VNUM overlap and one source-only
`surf.wld` approach, #280345 north to #33681. It is not active ingress. The
reviewed Surf subset is now 207 and curated source-only totals are 3,441
overall; `ud_map2` remains 15. Active `mobs_surf.zon` imports local mobile
#33500 into #31412 with twenty M declarations. Unregistered `surf.zon` has
fourteen additional source-only reset declarations of the same mobile.
Scans found no other exact external QST, SHP, sidecar item, duplicate mobile or
object prototype, code callback binding, external ITEM_TELEPORT destination, or
external ITEM_SWITCH value[1] room field into Bloody Plains. See the [audit supplement](BLOODY_PLAINS_AUDIT_REVIEW.md)
for source lines and the typed-scan boundary.

### Checkpoint 195 — BogenTok

The active BogenTok WLD has 205 rooms and 440/440 resolving authored exits:
438 local and two outbound. All rooms form one weak component. From each of
the two active Underdark inbound anchors, raw directed traversal reaches and
returns from all 205 rooms when authored exits are available. The two entries
are Underdark #812256 east to #18714 and #812663 west to #18700; each has a
direct BogenTok return. Excluding the 72 door exits that ZON resets closed
leaves 152/205 rooms statically reachable from either inbound anchor. This
estimate does not simulate opening doors, player access, or reset timing.

Every door exit carries only the door flag, has key VNUM 0, and has a matching
ZON `D` reset with state 1. There are no fall or current records. The zone
header uses reset mode 2, which the event source permits to reset while
occupied. This identifies source scheduling behavior, not a live reset time
or current door state.

The all-95 inactive-WLD sweep finds no BogenTok room overlap and one
source-only `ud_map2.wld` approach: #1128575 west to #18700. It is not active
ingress. The reviewed Surf subset remains 207 and curated source-only totals
rise to 3,442 overall; the reviewed `ud_map2` subset rises from 15 to 16.
Preserve the approach on existing `ud_map2` candidate row 114; its one active
Shafts inbound reference is unchanged. The audit supplement also records
Alatorin's active object and mobile reset imports, the three local shops,
the local callback bindings, and the static traversal limits.
See the [BogenTok audit supplement](BOGENTOK_AUDIT_REVIEW.md) for exact
source references and dependency scope.

### Checkpoint 196 — Vella's Bordello

The active Bordello map has 69 rooms and 147/147 resolving exits (146 local,
one outbound) in one weak component. Each of its three active inbound anchors
statically reaches and returns from all 69 rooms over authored exits. Heavens
#29 south enters #93641, White Harbor #55614 up enters #93610, and Surface
#563634 north enters #93600; the Bordello #93600 south exit returns to Surface
#563634. Only the Surface pair reciprocates. See active exit-register rows
[9](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L9),
[767](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L767),
[1127](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1127), and
[1473](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1473), plus pair-summary rows
[51](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L51),
[171](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L171),
[471](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L471), and
[783](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L783).

The Main Hallway ↔ Treasure Room door pair has raw flags 5 and key 0, while
both ZON state-0 resets clear locked and closed bits. This leaves a
reset-open door pair despite the room's hidden-room description; actual reset
state and how the room is found remain unqualified. Room #93616 is the sole
fall marker, with raw chance 2 and a down exit to fountain room #93610.
There are no currents.

The all-95 inactive-WLD scan found two non-overlapping source-only Surf
approaches to #93600: #225923 south and #226224 north. They are not active
ingress. Curated source-only totals rise to 3,444 overall and 209 for Surf;
`ud_map2` remains 16. Preserve both on existing Surf candidate row
[97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); its 145 active inbound
references are unchanged. Githzerai prime shift to #93665 and the Chaos
portal alias `vella` to #93610 are separate code routes, not WLD edges. The
audit also records four Bordello object #45 resets, whose only active-manifest
prototype is Heavens' slot machine. See the [Bordello audit supplement](BORDELLO_AUDIT_REVIEW.md),
[coverage row 35](ZONE_COVERAGE_REGISTER.csv#L35), and
[title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-196-vellas-bordello-title-and-path-history).

### Checkpoint 197 — Bugentolen reconciliation

The current Bugentolen WLD recheck confirms 125 rooms and 253/253 resolving
exits (252 local, one outbound). Raw directed traversal from either active
arrival reaches and returns from all 125 rooms. With the four edges in the two
state-1 reset-closed door pairs unavailable, reach and return from each active
arrival is 110/125; the remaining weak components have 14 and one room. The
active exits remain Wh #55402 east to #95813 without a reverse, and #95800
east to Surface #589071 with a reciprocal return. No fall or current fields
are present.

The single inactive-WLD Surf approach, #280727 west to #95800, was already
recorded at Checkpoint 27 and remains source-only, not active ingress. Curated
totals remain 3,444 overall, 209 for Surf, and 16 for `ud_map2`; Surf's 145
active inbound references are unchanged. Preserve the source-only approach on
existing candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).
See the [Bugentolen audit reconciliation](BUGENTOLEN_AUDIT_REVIEW.md),
[coverage row 42](ZONE_COVERAGE_REGISTER.csv#L42), and the active exit ledger
[rows 751 and 1150](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L751).

### Checkpoint 198 — Buildings static shells and runtime outposts

The active `buildings` WLD has 100 rooms (#97800–#97899) and zero authored
exits, so its static graph consists of 100 singleton components. No active WLD
edge enters or leaves the range, and the all-source WLD scan found no
unregistered source-only approach. A static traversal from any room therefore
reaches and returns from only that room.

The current C++ path supplies separate runtime edges. In normal non-mini
startup, `init_outposts()` reads three persisted outpost records and attempts
to instantiate each through the `buildings` room allocator. The generator
adds a down exit from an allocated room to the outpost's exterior location;
the outpost mobile's enter handler transfers authorized players inside.
The fixed code anchors are Surface #588184, #628856, and #531100. Optional
portals add a runtime link to a guildhall hearthstone. These generated routes
are not static WLD edges, and live persisted state or player traversal was not
checked. See the [Buildings audit supplement](BUILDINGS_AUDIT_REVIEW.md) and
[coverage row 44](ZONE_COVERAGE_REGISTER.csv#L44).


### Checkpoint 199 — CBL Hall static graph and source-only approach

Active `cbl_hall` has 124 rooms and 202/202 resolving authored exits: 201 local
and one outbound boundary. Its only active boundary pair connects CBL #29703
D4/up to Heavens #420 and returns from Heavens #420 D5/down to CBL #29703
([CBL room](../../../areas/wld/cbl_hall.wld#L74), [Heavens room](../../../areas/wld/heavens.wld#L631)).
The CBL local graph has 13 weak components (eleven size 10, one size 9, one size
5). Raw directed reach from #29703 is 10/124. No door flags or keys, unresolved
targets, range mismatches, falls, or currents were found. These counts include
WLD exits only.

The all-95 inactive-WLD scan found one non-overlapping source-only approach:
`ud_map2.wld` #1124947 D5/down to active CBL #29703
([source room](../../../areas/wld/ud_map2.wld#L59101), [exit](../../../areas/wld/ud_map2.wld#L59122)).
It is not active ingress. The reviewed `ud_map2` subset rises to 17 and curated
source-only totals rise to 3,445 overall, 209 for Surf, and 17 for `ud_map2`.
Preserve the route on candidate row
[114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114); keep `ud_map2` RETAIN IN
PLACE.

Fourteen local OBJ prototypes are ITEM_TELEPORT objects with local room
`value[0]` destinations. Eight are reset into rooms. Their encoded values are
not counted as established reachability because command eligibility, CBL
callback wiring, traversal, return, and safety need runtime or owner
qualification. Keep active `cbl_hall` **DO NOT MOVE**. See the [CBL Hall audit
supplement](CBL_HALL_AUDIT_REVIEW.md), [coverage row
48](ZONE_COVERAGE_REGISTER.csv#L48), and [source-story dossier](../zone-stories/CBL_HALL.md).

### Checkpoint 200: Celestia static graph and reset-placed routes

Active `celestia` has 230 rooms and 896/896 resolving authored WLD exits: 895
local and one outbound. The local undirected graph has three weak components
(159, 70, and one room). Raw directed local WLD reach, with every local exit
treated as traversable, is 149/230 from #45500, 149/230 from inbound anchor
#45718, and 70/230 from Palace approach #45639. After the unconditional door
resets, exits that remain open, non-secret, and non-blocked yield 92/230,
1/230, and 2/230 from those same starts. These source counts do not execute
search, keys, player permissions, item commands, or live resets.

| Direction | WLD edge | Destination | Reverse WLD record |
| --- | --- | --- | --- |
| Inbound | `realm` #14208 D6/northwest | Celestia #45718 | None |
| Inbound | `wh` #55634 D0/north | Celestia #45718 | None |
| Outbound | Celestia #45637 D0/north | Avernus #51207 | None |

Both active arrivals land in #45718, whose only authored local exit goes up to
#45688. Celestia’s reset commands apply state 6 to both sides: closed, locked,
and secret. The entry therefore has one room of source-state open WLD reach.
Reset #45538 in that room encodes `enter` to #45500; #45535 at #45500 encodes
`enter` to Astral Main #19721. Astral Main and Ethereal Main reset #45536 as a
return object to #45500. Generic `ITEM_TELEPORT` dispatch and the #45536
`teleporting_pool` callback provide code-defined command paths, but the object
resets, player use, return, and safety were not runtime-qualified. Keep these
object routes outside the WLD edge counts. See the [Celestia audit
supplement](CELESTIA_AUDIT_REVIEW.md) and [source dossier](../zone-stories/CELESTIA.md).

The 70-room component holds the Titan Palace rooms and has no authored WLD edge
or local type-25 destination into the 159-room main component. Its approach at
#45639 reaches only two rooms through exits left open by the ZON door resets.
The singleton #45719 is a summon/load room. These source structures need a
builder’s intended-entry explanation; this report does not invent a route.

The complete inactive-WLD scan found no Celestia room-ID overlap and no
source-only exit into this zone across 95 unregistered WLD files (527,791 rooms
and 2,080,773 exits; zero parse issues). No inbound edge from an inactive file
was discovered. The world-wide totals in the initial static census do not
change.

### Checkpoint 201: Charing reset-aware static graph

Active Charing has 210 room records and 467 direction slots: 464 local exits
and three boundary records. All 464 local targets resolve; one boundary exit
resolves to active Surface #513692 and two targets (#209060 and #209660) are
absent from the active loader. The local graph has weak components of 208, 1,
and 1 rooms. With every local exit treated as available, directed reach from
the active Surface arrival #45183 covers 208/210 rooms.

| Direction | WLD edge | Active status |
| --- | --- | --- |
| Inbound | Surface #513692 east to Charing #45183 | Active; paired by Charing #45183 west to #513692. |
| Inbound | Surface #513693 east to Charing #45183 | Active one-way record; Charing has no second west exit. |
| Outbound | Charing #45183 west to Surface #513692 | Active reverse record for one of the two Surface arrivals. |
| Unresolved outbound | Charing #45185 north to #209060 | Target absent from the active loader. |
| Unresolved outbound | Charing #45205 south to #209660 | Target absent from the active loader. |

The all-source inactive-WLD scan adds two non-overlapping `surf.wld`
approaches: #209060 south to Charing #45185 and #209660 north to #45205.
They mirror the unresolved Charing exits and are source-only, not active
ingress. `surface2011.wld` has the same numbered rooms but no matching reverse
approaches. Preserve the two Surf records on candidate row
[97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); the reviewed Surf subset is
211 and curated source-only totals are 3,447 overall, 211 for Surf, and 17 for
`ud_map2`.

Charing has 58 unconditional door resets: 51 closed, six closed and locked,
and one closed and secret. The source-state open/non-secret/non-blocked subset
contains 406 of 464 local directed exits. Simple directed reach in that subset
is 40/210 from #45183 and 77/210 from both center #45000 and hometown #45006;
the reset-open graph has 29 weak components, with largest sizes 77 and 40.
These are static reset calculations and do not model keys, search, picking,
race rules, live reset timing, callbacks, or safety. See the [Charing audit
supplement](CHARING_AUDIT_REVIEW.md), [coverage row
55](ZONE_COVERAGE_REGISTER.csv#L55), and [attic plan](ATTIC_MIGRATION_PLAN.md#checkpoint-201-charing-and-source-only-dependencies).

### Checkpoint 202: Cityruin reset-aware access

Active Cityruin has 155 rooms and 327/327 loader-resolving exits: 326 local
and one outbound to Tharnadian Ruin. Its local WLD graph is one weak component;
raw directed reach from the paired arrival room #66200 is 155/155.

| Direction | WLD edge | Active status |
| --- | --- | --- |
| Inbound | Tharnadian Ruin #5588 west to Cityruin #66200 | Active, paired by Cityruin #66200 east to #5588. |
| Outbound | Cityruin #66200 east to Tharnadian Ruin #5588 | Active reverse record. |
| Source-only inbound | `surf.wld` #262344 west to Cityruin #66200 | Already recorded in the global source-only ledger; not active ingress. |

The inactive-WLD scan of all 95 unregistered files confirms one
non-overlapping approach, the existing `surf.wld` #262344 record. It is
preserved on Surf candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97)
and does not change the curated source-only totals.

Cityruin has 48 unconditional door resets: 27 closed, 14 closed/secret, and
seven closed/locked. These leave 278/326 local exits already open,
non-secret, and non-blocked. Strict directed reach in that reset-open graph is
1/155 from paired arrival #66200, and 91/155 from #66201 or #66202; the graph
has 19 weak components, with largest sizes 91, 24, 14, 5, 4, and 4. The gate
resets closed but unlocked, and the armoury exit resets asymmetrically: the
Cityruin entrance is locked for key #66200 while its reverse is closed but
unlocked. Closed-unlocked exits may be opened, and secret routes may be found;
these counts do not model player commands, key/search/pick behavior, reset
timing, dynamic handlers, or safe return. See the [Cityruin audit
supplement](CITYRUIN_AUDIT_REVIEW.md) and [coverage row
58](ZONE_COVERAGE_REGISTER.csv#L58).

### Checkpoint 203: cyrictunnel reset-aware access

Active cyrictunnel has 82 rooms and 195/195 resolving exits: 190 local and
five outbound. Its local graph is one weak component; all-exits-available
directed reach from each active arrival covers all 82 rooms. The five active
crossings are paired with Surface, three Underdark locations, and Connector
Zones.

| Direction | Active WLD edge | Paired edge |
| --- | --- | --- |
| Surface | cyrictunnel #54500 up to Surface #581796 | Surface #581796 down to #54500 |
| Underdark | cyrictunnel #54529 down to Underdark #757626 | Underdark #757626 up to #54529 |
| Underdark | cyrictunnel #54549 up to Underdark #743256 | Underdark #743256 down to #54549 |
| Underdark | cyrictunnel #54554 up to Underdark #746483 | Underdark #746483 down to #54554 |
| Connector Zones | cyrictunnel #54561 up to Connector Zones #53632 | Connector Zones #53632 down to #54561 |

An all-95-file scan found five non-overlapping source-only approaches in
inactive surf.wld: room #265588 down to #54500; rooms #267393 and #270997 west
to #54529; room #271893 west to #54554; and room #273391 north to #54549.
They remain source-only, not active ingress. Preserve them on Surf candidate
row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). The reviewed source-only
totals rise by five to 3,452 overall and 216 for Surf; ud_map2 remains 17.
The exact exit register and [cyrictunnel audit
supplement](CYRICTUNNEL_AUDIT_REVIEW.md#source-only-inactive-surf-approaches)
provide source locations.

Twelve unconditional door resets leave 178 of the 190 local exits open,
non-secret, and non-blocked. The strict reset-open graph has seven weak
components sized 40, 19, 8, 7, 6, 1, and 1. Directed reach is 7/82 from
Surface arrival #54500; 40/82 from each of the three Underdark arrivals; and
1/82 from Connector Zones arrival #54561. The two pushable rubble switches
clear the cave-in blockers; modeling only those switch effects yields 47/82
from #54500 while closed, locked, and secret exits remain unavailable. The
statue is a push switch for the blocked floor exit, while the necromancer and
threshold sentry carry keys matching their respective doors. These are
source-backed mechanics, not live-game qualification.

The audit found 14 lava lake/flow rooms assigned FIREPLANE sector value 11,
while LAVA is defined separately as 39. The movement handler applies
firesector() on entry to either sector. Confirm the intended assignment. See
the [cyrictunnel audit supplement](CYRICTUNNEL_AUDIT_REVIEW.md), [coverage
row 68](ZONE_COVERAGE_REGISTER.csv#L68), [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-203-cyrictunnel-title-and-history),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-203-cyrictunnel-and-surf-retention).

### Checkpoint 204: Dark Forest reset-aware access

Active dark_forest has 171 rooms and 354/354 resolving exits: 352 local and
two outbound. Its local graph is one weak component, and directed reach from
the active arrival room #75171 covers all 171 rooms when all exits are
available. Surface supplies three active inbounds to #75171; two local exits
from that same room are reciprocal Surface pairs, while the inbound from
Surface #605089 has no direct return edge.

An all-95-file scan found one non-overlapping source-only Surf approach:
inactive surf.wld #208177 north to #75171. It is not active ingress. Preserve
it on Surf candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).
Curated source-only totals rise by one to 3,453 overall and 217 for Surf;
ud_map2 remains 17. See the [dark_forest audit
supplement](DARK_FOREST_AUDIT_REVIEW.md#source-only-inactive-surf-approach)
for exact source lines.

Twelve unconditional door resets close six pairs without locking them. The
strict reset-open graph contains 340/352 local exits and four weak components
of 147, 16, 5, and 3 rooms. Directed reach from the active arrival #75171 is
147/171 before opening any of those doors. The river rooms use WATER_NOSWIM
sector 7, and the precarious bridge rooms are marked SINGLE_FILE but have no
authored fall-chance or current records. These static readings do not
establish inaccessibility, actual crossing hazards, or safe return.

The compiled archer binding for mobile #75013 uses local firing and target
rooms; it is documented with the typed dependency scan, not modeled in the
WLD reach count. See the [dark_forest audit supplement](DARK_FOREST_AUDIT_REVIEW.md),
[coverage row 69](ZONE_COVERAGE_REGISTER.csv#L69), [title/history
checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-204-dark-forest-title-and-history),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-204-dark-forest-and-surf-retention).

### Checkpoint 205: Yuan Ti2 reset-aware access

Active `yuan_ti2` has 79 rooms and 167/167 resolving exits: 165 local and two
outbound. Its local graph is one weak component. Active arrivals are Surface
#600250 to room #80600 and neighboring `yuan_ti` #80554 to room #80671; both
boundaries are reciprocal. With all authored local exits available, directed
reach from either arrival covers 77/79 rooms. The two excluded rooms are
#80677 and #80678, which have no inbound WLD records in active or inactive
sources; #80677 nonetheless receives reset mobile #80627 and exits down to
#80665.

Four door resets close two local pairs without locking them. They leave
161/165 local exits open, non-secret, and non-blocked. The strict reset-open
graph has weak components of 77, 1, and 1 rooms; local directed reach is 75/79
from #80600 and 1/79 from #80671. The latter arrival retains its open paired
north exit back to `yuan_ti` #80554. The fence route is closed and secret on
the village side; the shop-to-quarters door is closed on both sides. These
source counts do not qualify door commands, dynamic behavior, safe travel, or
actual return.

The all-95 inactive-WLD scan adds two non-overlapping source-only approaches
to #80600: `oldmap8.wld` #187735 west and `surf.wld` #281432 west. Neither is
active ingress. Preserve them on oldmap8 candidate row [81](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L81)
and Surf row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). Curated
source-only totals rise by two to 3,455 overall and 218 for Surf; `ud_map2`
remains 17. Exact room, exit, and target lines are in the [Yuan Ti2 audit
supplement](YUAN_TI2_AUDIT_REVIEW.md#source-only-inactive-wld-approaches).

The swamp segment uses sector 26 and contains no authored fall-chance or
current records. The local gold key #80612 matches the local chest, not an
exit-key field. The [Yuan Ti2 audit supplement](YUAN_TI2_AUDIT_REVIEW.md),
[coverage row 352](ZONE_COVERAGE_REGISTER.csv#L352), [title/history
checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-205-yuan-ti2-title-and-history),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-205-yuan-ti2-and-source-only-retention)
record typed dependencies and the builder questions.

### Checkpoint 206: Outpost reset-aware access

Active outpost has 40 rooms and 81/81 resolving exits: 80 local and one
outbound. Its local graph is one weak component. The active arrival is room
#82124 from Surface #527555; the eastbound Surface edge and westbound outpost
edge are reciprocal. Directed traversal with all authored local exits
available reaches all 40 rooms from #82124.

The scan of all 95 current inactive WLD files found one source-only approach:
inactive `surf.wld` #206585 east to active #82124. It is not active ingress.
Preserve it on Surf candidate row
[97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). Curated source-only totals
rise by one to 3,456 overall and 219 for Surf; `ud_map2` remains 17. The
[Outpost audit supplement](OUTPOST_AUDIT_REVIEW.md#source-only-inactive-wld-approach)
provides exact source lines.

Twenty-four unconditional door resets close 12 local pairs: eleven pairs are
closed and the Treasury pair #82106 north / #82125 south is closed and locked
with key #82110. The strict reset-open graph leaves 56/80 local directed exits
open and has weak components sized 25, 4, and eleven singletons. Directed
reach is 25/40 from #82124. The four-room lower-dungeon component is #82136-
#82139; the keyholder's meeting room #82123 and Treasury #82125 are isolated
in this reset-open graph. The keyholder's room itself is behind a closed
meeting-room door.

The exact key field #82110 also appears on both reset-locked sides of the
active Tharnadian Ruin Cathedral door pair #5540/#5541. The local QST dialogue,
Tharnadian Ruin QST, and scroll in Perrin's box connect those sources. The
scroll separately calls the nested holy symbol #82118 a Cathedral key, but no
scanned WLD door-key field uses that object. These counts and item fields do
not qualify quest interaction, opening commands, actual travel, or safe
return. See the [Outpost audit supplement](OUTPOST_AUDIT_REVIEW.md),
[coverage row 227](ZONE_COVERAGE_REGISTER.csv#L227), [title/history
checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-206-outpost-title-and-history),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-206-outpost-and-surf-retention).

### Checkpoint 207: Lab reset-aware access

Active `lab` has 51 rooms and 100/100 resolving exits: 99 local and one
outbound. Its WLD-only local graph has weak components sized 40, 5, 4, and 2;
all-exits-available directed reach from arrival #82350 is 20/51. The active
inbounds are ConnectorZones #53855 north and Kvarkpass Connector #54690 north
to #82350. Lab #82350 exits south to #54690, making the Kvarkpass boundary
reciprocal; ConnectorZones is one-way inbound.

Ten unconditional directional door resets close five pairs. The strict
reset-open WLD graph leaves 89/99 local directed exits open and has weak
components sized 27, 6, 5, 4, 3, 2, 2, 1, and 1. Directed reach from #82350 is
2/51 because the first gate pair #82300/#82301 is reset closed. Seven local
`ITEM_TELEPORT` objects encode `CMD_ENTER` destinations and link three of the
WLD-only side components back to the main area; those object actions are kept
separate from the WLD exit counts. No fall-chance or current fields are
authored.

The current WLD inventory has 446 files: 351 active area files and 95
unregistered files. The all-95 inactive scan adds two non-overlapping
source-only approaches to #82350: `surf.wld` #269091 west and
`underworld2.wld` #421086 west. Neither is active ingress. Preserve the Surf
edge on candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97) and
the Underworld2 edge on row
[116](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L116). Curated source-only
totals rise by two to 3,458 overall and 220 for Surf; `ud_map2` remains 17,
and Underworld2 now has seven recorded approaches. The [Lab audit
supplement](LAB_AUDIT_REVIEW.md#source-only-inactive-wld-approaches) gives
exact room and exit lines. This 446-file inventory corrects the prior
checkpoint's inactive-file denominator from 96 to 95.

### Checkpoint 208: kuotoa reset-aware access

Active kuotoa has 37 rooms and 75/75 resolving WLD exits: 74 local and one outbound. Its local raw graph has one weak component, and all-exits-available directed reach from #88200 is 37/37. Underworld #4454 east reaches #88200; local #88200 west returns to #4454, so one inbound, one outbound, and one reciprocal pair form the active boundary.

Twenty-eight unconditional door resets close 14 local pairs. With those exits excluded, 46/74 local directed exits remain open; the strict graph has weak components sized 14, 6, 4, 2, and eleven singletons. Directed reach from #88200 is 14/37: #88200–#88210, #88212, #88229, and #88230. The closed pair #88212 east / #88224 west is the double-door route into the temple entrance. No local fall-chance or current fields are authored.

Scanning all 95 unregistered WLDs found no source-room overlap or exit target into #88200–#88236. Across the remaining current WLD source, the only exact external target is Underworld #4454 east to #88200, which is the reciprocal boundary above. The current inventory remains 446 WLD files: 351 active and 95 unregistered. No inactive candidate row or curated source-only count changes. See the [Kuo-Toa Temple audit supplement](KUOTOA_AUDIT_REVIEW.md), [coverage row 151](ZONE_COVERAGE_REGISTER.csv#L151), the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-208-kuo-toa-temple-title-and-history), and the [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-208-kuotoa-and-source-only-retention).

### Checkpoint 209: udmini reset-aware access

Active udmini has 21 rooms and 39/39 resolving WLD exits: 38 local and one outbound. Its local raw graph has weak components sized 19 and 2. The active arrival targets, #88502 and #88515, reach all 19 rooms in the corpse-and-larvae cave component; the two-room mushroom branch (#88500–#88501) is outside the active WLD arrival graph. There are no local door resets or current fields.

The active inbounds are ConnectorZones #54239 down to #88502 and Kvarkpass Connector #54811 up to #88515. The local outbound is #88502 up to ConnectorZones #54194, producing two inbound, one outbound, and no direct reciprocal WLD pair. The reset-placed crack item in #88515 enters #54811; that room and its lower chamber #54832 have F 100 fields and open downward exits. The local F 100 rooms are #88502 and #88520. Entry handling can schedule falling down open exits, so ordinary movement and return safety are not modeled by the source graph.

The all-95 inactive-WLD scan found no source-room VNUM overlap or source-only edge to #88500–#88520. Current WLD inventory remains 446 files: 351 active and 95 unregistered. No attic candidate row or curated source-only count changes. See the [UDMini audit supplement](UDMINI_AUDIT_REVIEW.md), [coverage row 322](ZONE_COVERAGE_REGISTER.csv#L322), the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-209-udmini-title-and-history), and the [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-209-udmini-and-source-only-retention).

### Checkpoint 210: morg reset-aware access

Active `morg` has 30 rooms and 69/69 resolving WLD exits: 68 local and one outbound. Its raw weak graph is one component of 30 rooms. UnderDark chasm #844836 exits south to #88600, and #88600 exits north back to #844836. This is the only active inbound/outbound boundary pair. UnderDark room #845236, titled “Morg's Palace,” also reaches #844836; the room title and shared chasm do not establish whether it is an alias or a separate landmark.

The inbound edge arrives in #88600, a vestibule outside the main tavern. The east exit into #88604 and its west return both reset closed. In total, 14 D commands close ten ordinary door exits and mark four exits closed and secret. Reset state 1 sets closed and clears locked; state 5 also sets the secret flag. The back-room drop and cellar passage to the wererat nest use the secret state. No fall or current fields are authored. Actual access, door operation, and secret discovery remain pending; the raw WLD graph does not model them.

The WLD text also describes a possible hatch above #88614 and a northward inn hallway from #88623, but neither corresponding exit is present in those rooms. All-95 inactive-WLD scans found no source-room VNUM overlap or source-only edge into #88600–#88629. Current inventory remains 446 WLD files: 351 active and 95 unregistered. No candidate row or curated source-only count changes. See the [Morg's Place audit supplement](MORG_AUDIT_REVIEW.md), [coverage row 188](ZONE_COVERAGE_REGISTER.csv#L188), the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-210-morgs-place-title-and-source-history), and the [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-210-morg-and-source-only-retention).

### Checkpoint 211: foggy woods reset-aware access

Active foggy_woods has 213 rooms and 607 source exits: 592 local, six outbound to active areas, one positive unresolved target, and eight negative targets. Of these, 598 resolve against the active loader inventory. Its raw local graph has one weak component of 213 rooms. The six active outbound boundaries are reciprocal: Surface #579251/#579650 to #90000, #578042 to #90162, and #575652/#576053 to #90185; Underworld3 #30281 connects to #90212. The exact pairs remain in the [cross-area summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L145) and [exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L423).

Room #90046 west targets #143836 in unregistered Duris3.wld, which is omitted by the standard active list; keep the existing Duris3 candidate row [36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36) and unresolved-exit row [266](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L266). The eight negative -1 targets remain cataloged in the unresolved-exit register. The all-95 inactive-WLD scan found no room overlap or source-only approach to #90000–#90220. The WLD inventory remains 446 files: 351 active and 95 unregistered.

Twenty D commands reset ten local door pairs: nine state-1, four state-2, five state-5, one state-6, and one state-14 commands. States 1/5 close exits, state 5 also marks secret; state 2 locks, state 6 adds locked and secret, and state 14 adds blocked. With those exits treated as unavailable, strict directed reach is 193/213 from each of the six active arrival targets; the same 20 rooms are unreachable in that static graph. The local key #90024 is reset on the shaman in #90205; switch #90010 in #90210 clears the blocked bit on the #90207 south exit but leaves its closed, locked, and secret bits. Actual opening, discovery, interaction, and movement remain pending. No fall or current fields are authored. See the [Foggy Woods audit supplement](FOGGY_WOODS_AUDIT_REVIEW.md), [coverage row 100](ZONE_COVERAGE_REGISTER.csv#L100), [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-211-fog-enshrouded-wood-title-and-source-history), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-211-foggy-woods-and-source-only-retention).

### Checkpoint 212: dracopit reset-aware access

Active dracopit has 69 rooms and 393/393 loader-resolved source exits: 392 local and one outbound to Mountain Castle. It has two active WLD inbounds, one outbound, and one reciprocal pair. Realm #14209 enters #15913 one way; the pair with Mountain Castle connects #15968 and #37574. See cross-area summary rows [119](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L119) and [388](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L388), and exit-register rows [254](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L254), [278](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L278), and [539](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L539).

The raw local WLD graph has components of 64 rooms (#15900–#15963) and five rooms (#15964–#15968). From Realm arrival #15913, all-exits and strict reset-state WLD reach each cover 64/69 rooms. From Mountain Castle arrival #15968, all-exits reach covers five rooms; applying the two local state-2 D commands leaves only #15968 reachable in the strict graph. Both door locks use key #37115, and current condition is unknown because reset mode is 0 and its special handler depends on persisted state.

WLD-only reach excludes ITEM_TELEPORT behavior. The local portal #15909 is reset in #15964 and targets pocket #15940. Separately, active Caertannad resets portal #78510 in #78442, targeting local room #15907. This creates an active dynamic arrival into the main Pit component. No local exit is unresolved or negative, and the all-95 inactive-WLD scan found no overlapping source room or source-only approach to #15900–#15968. The world inventory remains 446 WLD files: 351 active and 95 unregistered. See the [Dracopit audit supplement](DRACOPIT_AUDIT_REVIEW.md), [coverage row 78](ZONE_COVERAGE_REGISTER.csv#L78), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-212-dracopit-and-source-only-retention).

### Checkpoint 213: dragonnia reset-aware access

Active dragonnia has 67 rooms and 141 loader-resolved source exits: 139 local and two outbound to Surface. There are four active WLD inbounds to three unique local targets: two Surface exits to #6803, Realm #14210 to #6839, and Winterhaven #55633 to #6801. Only the Surface links have reverse exits. The exact active pair records are in cross-area summary rows [120](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L120), [389](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L389), and [494](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L494); exit-register entries are [126](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L126), [127](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L127), [262](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L262), [789](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L789), [1673](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1673), and [1676](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1676).

The raw local WLD graph has components of 66 rooms and one isolated private room #6858. Each of the three unique active arrival targets reaches 64/67 rooms when all authored exits are available. Applying the 20 local D resets and treating closed, locked, or secret exits as unavailable leaves 1/67 from Winterhaven arrival #6801 and 52/67 from Realm #6839 and Surface #6803. The Dark Track rooms #6867–#6868 are outside these directed reach sets even with doors open. The zone header uses reset mode 0; ordinary scheduling skips it, while the special event path can call `reset_zone` based on persisted state. Current door and mobile state is unknown. See the [Dragonnia audit supplement](DRAGONNIA_AUDIT_REVIEW.md), [coverage row 79](ZONE_COVERAGE_REGISTER.csv#L79), and [shaft/reset dossier](../zone-stories/DRAGONNIA.md).

WLD-only reach does not include the local ITEM_TELEPORT object #6860, which targets #6836 and is reset on a temple statue with a 10% load chance. The all-95 inactive-WLD scan found two additional non-overlapping source-only exits to active #6803: Surf #252526 north and Underworld2 #431340 west. These candidate WLD files are not active ingress. The source inventory remains 446 WLD files: 351 active and 95 unregistered. Preserve both approach records with their existing candidates; see the [source-only retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-213-dragonnia-and-source-only-retention).

### Checkpoint 214: Dreggan Wood reset-aware access

Active dreggan_wood has 188 rooms and 729/729 loader-resolved source exits: 727 local and two outbound to Ethereal Plane and Shadamehr Keep. There are two active inbounds and two reciprocal pairs. Ethereal room #12480 west enters #41001, with a matching east return; Shadamehr Keep #41200 south enters #41186, with a matching north return. See cross-area summary rows [122](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L122), [123](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L123), [134](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L134), and [426](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L426), and exit-register rows [220](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L220), [556](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L556), [557](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L557), and [558](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L558).

The local graph has one weak component of 188 rooms. Directed all-exit reach from Ethereal arrival #41001 covers 188/188; from Shadamehr arrival #41186 it covers 89/188. All exit door flags are 0, and the local zone has no D reset commands. No falls or currents are authored. Dreggan’s zone header uses reset mode 0, whose special handler can call `reset_zone` based on persisted state; current reset state is unknown. See the [entrance and maze](../zone-stories/DREGGAN_WOODS.md), [source exit at #41186](../../../areas/wld/dreggan_wood.wld#L5031), and [reset-mode handling](../../../src/world/db.c#L6846).

WLD reach excludes item and quest availability. Dreggan resets four Alatorin lilies #83284; Alatorin consumes the lily with three other flowers for #83285, and also has a 2% reset chance to place a lily on Mishanen. The hourly weather event swaps local Cronus Flowers #41004 and #41005 at #41059. The source-only all-95 WLD scan found no source-room overlap or approach to this zone. The world inventory remains 446 WLD files: 351 active and 95 unregistered. See the [Dreggan audit supplement](DREGGAN_WOOD_AUDIT_REVIEW.md), [coverage row 81](ZONE_COVERAGE_REGISTER.csv#L81), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-214-dreggan-wood-and-source-only-retention).

### Checkpoint 215: end terminal marker and connectivity

The active `end` contributor has zero playable WLD records. Its only WLD record marker is #9999999 followed by $~; the source roster describes `end` as the top-of-world marker, and the room loader treats the $~ name as the end of the file. The local exit, inbound-boundary, graph, fall, and current counts are therefore all zero. See the [area roster](../../../areas/AREA#L414), [terminal WLD file](../../../areas/wld/end.wld#L1), [world loading](../../../src/world/db.c#L1262), and [coverage row 89](ZONE_COVERAGE_REGISTER.csv#L89).

No active WLD target or any of the 95 inactive WLD sources approach #9999999. That number is a terminal marker and zone header, not a travel destination. The four `end.shp` dealer locations are in active rooms owned by Llzazan, Nax, Shady, and Tharnadia; their ZON resets are separate from End connectivity. See the [End source audit](END_OF_THE_WORLD_AUDIT_REVIEW.md#connectivity-exact-references-and-candidate-disposition).

### Checkpoint 216: eth2 reset-aware access

Active eth2 has 111 rooms and 383/383 resolving exits, all local, in weak components of 57, 28, and 26 rooms. Its only active WLD inbound is the unpaired Realm #14209 north exit to #32707; eth2 has no outbound WLD boundary. The exact active source record is [exit-register row 252](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L252). The all-95 inactive-WLD sweep found no room overlap or source-only target into eth2.

From the Realm arrival at #32707, raw directed WLD reach covers 12/111 rooms, and 26 rooms have a directed path back to #32707. The 30 local D commands set 16 exits closed and 14 closed and locked. Treating every reset position as unavailable leaves only #32707 reachable from the arrival and only that room able to return. The zone header uses reset mode 0; persisted and live door state was not inspected.

WLD traversal omits five local ITEM_TELEPORT objects: #32601 targets #32657, #32639 targets #32632, #32612 targets #32683, #32622 targets #32600, and #32640 targets Ethereal Main #12494. Active Ethereal Main object #12410 also targets eth2 #32632. These declared targets link the three local WLD components and Ethereal Main at source level. Reset availability and actual use remain unqualified. See the [Fortress of Dreams audit supplement](ETH2_AUDIT_REVIEW.md), [coverage row 91](ZONE_COVERAGE_REGISTER.csv#L91), and [portal resets](../../../areas/zon/eth2.zon#L129).


### Checkpoint 217: Ethereal Main reset and transport

Active ethereal_main has 150 rooms and 900/900 resolving source exits: 898 local and two outbound. There are seven active WLD inbounds from Dream, Dreggan Woods, and Prison. The Dream and Dreggan links are reciprocal; five Prison directions enter #12544 without a matching return. All three unique arrival rooms reach and return from all 150 rooms in the directed source graph. All exit door flags are zero, and there are no local D resets, falls, or currents. See [exit-register rows 149-153](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L149), [219](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L219), [220](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L220), [435](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L435), and [556](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L556).

WLD reach omits ten active object routes into Ethereal rooms and local departures through ITEM_TELEPORT and pool callbacks. The incoming routes include Alatorin, Ardgral, Astral Side Areas, eth2, Negative Plane, Ravenloft2, and the four elemental planes. Locally reset objects lead to Astral Main, the elemental planes, Celestia, and eth2. Plane travel code also uses #12401 as an Ethereal anchor. Random pool movement, reset state, command access, safe return, and live traversal remain unqualified; the exact reset and target records are in the [Ethereal Main audit supplement](ETHEREAL_MAIN_AUDIT_REVIEW.md).

The all-95 inactive-WLD sweep found no room-number overlap or source-only exit into rooms #12401-#12550. The zone header uses reset mode 1, and the static graph does not establish current object or reset state. See the [Ethereal Plane dossier](../zone-stories/THE_ETHEREAL_PLANE.md) and [coverage row 92](ZONE_COVERAGE_REGISTER.csv#L92).

### Checkpoint 218: Flind reset and teleport reach

Active flind has 89 rooms and 183/183 active-loader-resolving WLD exits: 182 local and one outbound from #3475 south to underdark #829678. The reverse underdark exit runs north to #3475. See [exit-register rows 45 and 1756](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L45) and [pair summary row 144](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L144). No exit is negative or unresolved. The local WLD graph has four weak components sized 53, 18, 15, and 3. From the active underdark arrival #3475, WLD-only reach and return each cover 3/89 rooms.

Flind’s ZON has 18 door reset commands: 13 set closed, four set closed and secret, and one set closed and locked. Treating those source-reset doors as unavailable and including the six local reset-loaded ITEM_TELEPORT links gives 8/89 reachable and returnable rooms from both #3475 and the compiled target #3404. Treating all authored local exits as traversable and including all six portal links gives 89/89 reach and return from both roots. Eleven WLD rooms have fall-chance values and no current fields are authored. Zone reset mode is 2; loaded door/object state and actual use were not inspected.

The six local item routes form pairs between #3408/#3409, #3431/#3441, and #3435/#3462. The exact object targets, command values, and reset lines are in the [Flind audit supplement](FLIND_AUDIT_REVIEW.md#reset-loaded-item_teleport-routes). The all-95 inactive-WLD sweep found no room-number overlap and one source-only approach: ud_map2 #1124533 down to #3475. It is not active ingress and remains with the existing ud_map2 candidate. The WLD inventory remains 446 files: 351 active and 95 unregistered. See [the source-only room](../../../areas/wld/ud_map2.wld#L53018), [Flind coverage row 99](ZONE_COVERAGE_REGISTER.csv#L99), and the [Flind dossier](../zone-stories/FLIND.md).

### Checkpoint 219: Glacier reset-aware connectivity

Active glacier has 257 authored WLD exits: 255 local, one outbound from #90402 east to Surface #523536, and one negative target from #90411 east to -1. The active loader resolves 256/257 targets. The local directed graph is one weak component containing all 105 rooms; raw reach and return from #90401, #90452, #90466, #90491, #90500, and #90505 each cover 105/105. There is no active WLD inbound to Glacier. The outbound Surface boundary has no reverse exit; see [exit-register row 1114](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1114) and [pair summary row 157](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L157).

With the four ZON-reset door exits treated as unavailable, reach and return cover 99/105 from #90401, #90452, #90466, and #90491; 5/105 from #90500; and 1/105 from #90505. The concealed block pair at #90491/#90500 is closed and secret. The key-#90402 slab between #90504/#90505 has state 6 (closed, locked, secret) on one side and state 2 (closed, locked) on the other. No fall or current fields are authored, and loaded door state was not inspected.

The all-95 inactive-WLD scan found no room overlap and one source-only approach: surf.wld #208853 north to #90401. It is not active ingress; see the [source room and exit](../../../areas/wld/surf.wld#L185821) and [Glacier audit supplement](GLACIER_AUDIT_REVIEW.md).

### Checkpoint 220: Goblin Camp active arrivals

All 65 active Goblin Camp WLD exits resolve: 64 are local and one leaves #11400 south to Surface #617373. Surface returns north to #11400. A second active inbound exit runs northeast from wh #55402 to #11412 and has no reverse boundary. The active records are [rows 209](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L209), [757](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L757), and [1617](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1617); the owner-pair summaries are [rows 158](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L158), [506](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L506), and [794](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L794).

The local WLD graph is one weak component containing all 21 rooms. Directed reach and return from both active arrival rooms, #11400 and #11412, cover 21/21. All authored exit flags and keys are zero. There are no local D reset commands, fall values, or current fields; reset mode is 2. The all-95 inactive-WLD sweep found no room-number overlap and one source-only approach: surf.wld #264548 north to #11400. It is not active ingress; see the [Goblin Camp audit supplement](GOBLINCAMP_AUDIT_REVIEW.md).

### Checkpoint 221: Grave reset-aware connectivity

All 480 active Grave WLD exits resolve: 478 are local and two leave the area. Exit #8400 south reaches Surface #593389, and #8427 east reaches Surface #592593; both Surface rooms have reciprocal entries. The active records are [Grave exits 178–179](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L178), [Surface reverse exits 1558–1559](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1558), and owner-pair summaries [164](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L164) and [509](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L509).

The local graph is one weak component of 204 rooms. Raw directed reach and return from both active arrival rooms, #8400 and #8427, cover 204/204. The ZON has 18 D resets: six state 1, eleven state 2, one state 5. Its key-gated pairs use iron key #8401, marble key #8405, and wooden key #8413; the concealed #8487/#8490 passage has no key. Treating reset-door exits and their opposite sides as unavailable gives 3/204 reachable and returnable rooms from #8400 and 149/204 from #8427. No fall or current fields are authored. A registered `shift_prime` code path separately includes Grave rooms #8486 and #8430 for Phantoms on the Astral Plane; see the [Grave audit supplement](GRAVE_AUDIT_REVIEW.md).

The all-95 inactive-WLD sweep found no room-number overlap and four source-only approaches: two from `surf.wld` (#275541 west to #8427; #276137 east to #8400) and two from `underdark2011.wld` (#250432 south and #250731 east to #8400). None is active ingress; see the [source records](GRAVE_AUDIT_REVIEW.md#source-only-inactive-wld-approaches).

### Checkpoint 222: Graves template connectivity

The active graves roster entry describes a random template zone. Its 25 WLD prototypes (#19500–#19524) have zero static exits, giving 25 singleton components; there are no active cross-area exits, inbound active WLD edges, or authored fall/current fields. The all-95 inactive-WLD scan found no overlap or exit targeting these room VNUMs.

The generator in random.zone.c can clone selected prototypes into dynamic rooms beginning at #100000, create randomized internal exits, and attach the first copy to a map room through a secret, closed down trapdoor. A successful dig through the assigned theme entrance object invokes search for the hidden exit. The boot-generation body is excluded by the current RANDOM_ZONES macro definition; the trusted randobj zone command is still an in-tree caller. No generated instance was created or inspected, so live reach and return remain unqualified. The selector also calls number(0, 10) against a ten-column array, allowing an out-of-range index 10. See the [Graves audit supplement](GRAVES_AUDIT_REVIEW.md) and [coverage row 114](ZONE_COVERAGE_REGISTER.csv#L114).

### Checkpoint 223: Guildhalls dynamic room-pool connectivity

The static guildhalls WLD contains 1,000 room prototypes and no authored exits, producing 1,000 singleton weak components. A correctly parsed scan of all 445 other WLD files found no overlapping room VNUM and no active or inactive exit target into #48000-#48999. Static cross-area exit counts and source-only ingress are zero.

This is a reusable room pool, not evidence of 1,000 authored hall rooms. Current code loads saved guildhall and room records, restores their stored exits, and joins each entrance to its stored outside map room. The main-hall constructor creates entrance, heartstone, portal, and inn rooms; later constructors add adjacent rooms. Town portals and building portals use separate code paths. No saved instance data was inspected, so actual halls, routes, and directed reachability remain unqualified.

The allocator scans #48020 through #48998, while the IN_GH_ZONE range includes #48999. The six unnamed prototypes #48015-#48019 and #48999 are not allocated by that loop. PortalRoom::init still contains a TODO for outpost portals; its room title and static template reset do not prove that a saved hall has an outpost route. See the [Guildhalls audit supplement](GUILDHALLS_AUDIT_REVIEW.md#static-connectivity-and-runtime-reachability).
### Checkpoint 224: Hell, Phlegethos directed grid and ingress

The 400 active room prototypes #52200-#52599 contain 1,600 local D0-D3 exits. All targets resolve inside the zone, all exit-info flags are zero, and the graph is one weak component. Directional traversal from #52200 reaches and returns through a 396-room strongly connected component; #52311, #52312, #52313, and #52599 are outside that directed reach. Eighty-six exit records have no matching reverse-direction edge. No other active or inactive WLD file targets this range, and no local exit leaves it.

The other 445 WLD files comprise 350 other active sources and 95 inactive sources. The full scan found no exact room-VNUM overlap or inbound target. This establishes no static WLD entrance. The named BCTDL object #32456 portal template is not reset-loaded in its source zone and has destination value 99999; it does not prove a runtime route. See the [Phlegethos audit supplement](HELL_FOUR_PHLEGETHOS_AUDIT_REVIEW.md#static-connectivity-and-reachability) and [source dossier](../zone-stories/HELL_FOUR_PHLEGETHOS.md#connectivity-and-retention-boundary).

Directional graph counts use #52200 as an analysis seed, not as a verified player arrival. Dynamic spells, staff travel, and live destination state were not exercised; entry, return, and safety remain unverified.


### Checkpoint 225: Hell One, Avernus static graph and boundaries

Hell One's 400 active room prototypes contain 1,600 WLD exits: 1,599 local exits and one outbound edge. All flags are zero. The induced local directed graph is one strongly connected component of 400 rooms. Sixty-three directed exits lack a matching reverse-direction record, but all rooms remain mutually reachable through alternate local paths. Each inbound room (#51001, #51032, and #51207) reaches all 400 rooms; all rooms can reach outbound room #51100.

Seven active WLD edges enter Hell One: River Styx #90700 south to #51001, five directional exits from Prison #7492 to #51032, and Celestia #45637 north to #51207. Hell One #51100 exits east to Tiamat #19600, with no reciprocal edge back to #51100. The full records are in [exit-register rows 164–168, 623, 639, and 1115](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L164); paired edges are in [rows 73, 187, 360, and 410](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L73).

The other 445 WLD files include 350 other active areas and 95 inactive sources. Their scan found no additional inbound exits, same-number room overlap, or inactive source-only approach. All Hell One source exits resolve. No external ZON D/R room reset targets a room in the range, and none of the 872 scanned external type-25 teleport object destinations enters it. The local reset script still has 209 mobile, follower, equipment, and give commands; live reset state and player access were not inspected. The [Hell One audit supplement](HELL_ONE_AVERNUS_AUDIT_REVIEW.md) records source links and the full edge list. Static topology is complete; live access, cross-zone return, dynamic resolution, and player safety remain unverified.


### Checkpoint 226: Hell Three, Minauros owner collision and room graph

The Minauros WLD source contains 400 rooms and 1,600 D0–D3 exits, all with flags zero and targets in the source room set. The weak graph is one component of 400. Directed analysis has one 396-room strongly connected component and four single-room components (#51911, #51912, #51913, #52199). From source seed #51800, 396 rooms are reachable and all 400 can return to #51800. Eighty-five directed exit records lack opposite-direction records. This source graph does not prove a player route because #51800 is duplicated in Hell Two.

The cross-area register classifies four Hell Two-to-Minauros and seven Minauros-to-Hell-Two exits, all touching #51800; see [exit rows 640–650](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L640) and [pair-summary rows 188–189](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L188). Four purported inbound edges are authored in the Minauros WLD file as exits from its own duplicated #51800 to Minauros neighbors. Seven purported outbound edges point from Minauros rooms back to #51800. Three additional active exits from Hell Two rooms #51420, #51780, and #51799 target #51800, but are owner-local in Hell Two and do not appear as Minauros inbounds in the cross-area register ([sources](../../../areas/wld/hell_two_dis.wld#L504), [second](../../../areas/wld/hell_two_dis.wld#L9872), [third](../../../areas/wld/hell_two_dis.wld#L10362)).

A scan of all other 445 active and inactive WLD files found no overlap except Hell Two's duplicate #51800 and no source-only approach. No active source WLD exit targets Minauros's unique room range #51801–#52199. The current owner classification places both #51800 room records at Hell Two's zone boundary; the VNUM-only runtime lookup was not exercised. Keep the source census, but treat the effective Minauros entry, exit, and safe return as unresolved until the room has one owner and its route is verified. See the [Minauros audit supplement](HELL_THREE_MINAUROS_AUDIT_REVIEW.md).


### Checkpoint 227: Hell Two, Dis graph and shared-room boundary

The Hell Two WLD source has 400 rooms (#51401–#51800) and 1,600 D0–D3 exits; all targets are in that source set. The weak graph is one component of 400. Its directed graph contains a 398-room strongly connected component and singleton rooms #51512 and #51513. From analysis seed #51401, 398 rooms are reachable and all 400 can return; 78 exits lack a matching reverse-direction record. This is a source graph, not proof of player arrival or safety.

Hell Two rooms #51420 north, #51780 south, and #51799 east each point to #51800 ([exit sources](../../../areas/wld/hell_two_dis.wld#L504), [second](../../../areas/wld/hell_two_dis.wld#L9872), [third](../../../areas/wld/hell_two_dis.wld#L10362)). All seven source edges classified as inbound come from Minauros and target that same number. The four cross-area outbound rows are authored from Minauros-file room #51800 to four unique Minauros rooms, then attributed to Hell Two by numeric ownership. The [cross-area register rows 640–650](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L640) and [pair-summary rows 188–189](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L188) show all eleven collision-flagged records.

Across the other 445 WLD files (350 other active and 95 inactive), there is no target to a unique Hell Two room #51401–#51799, no inactive approach, and no additional room overlap. The only duplicate active WLD room VNUM is #51800, authored separately in Hell Two and Minauros. The source graph and register counts do not establish which copy the merged runtime lookup uses. Keep actual entry, return, and safety unresolved until the owner decision and route verification. See the [Hell Two audit supplement](HELL_TWO_DIS_AUDIT_REVIEW.md).


### Checkpoint 229: Highmoor directed graph and hidden arrivals

Highmoor’s active WLD has 258 rooms and 731 authored exits: 727 positive local targets, one positive target to Surface, and three negative targets. All 728 positive targets resolve; the negative exits from #19327 north, south, and west are dropped by the loader ([unresolved rows 43–45](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L43)).

The local weak graph has components 256/1/1, with singleton rooms #19356 and #19357. From active Surface arrival #19100, raw directed reach covers 252 rooms and authored returns cover 255. Rooms #19352–#19357 are not reached from that seed; #19354, #19356, and #19357 cannot return by authored exits. The first four identify themselves as load or boot-result rooms, so these counts must not be read as an ordinary player map. Door reset state is also outside the raw graph model.

Highmoor #19100 south to active Surface #526069 is reciprocal with Surface #526069 north to #19100 ([Highmoor source](../../../areas/wld/highmoor.wld#L21), [Surface source](../../../areas/wld/surface.wld#L547316), [cross-area rows](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L310), [pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L192)). The all-95 inactive-WLD scan adds one non-overlapping source-only Surf edge: #209922 north to #19100. It is not active ingress; retain it in Surf’s existing candidate row. No other inactive Highmoor approach or room overlap was found.

Highmoor has 48 `C` current records and two 40% `F` records in waterfall rooms #19271 and #19272 ([first fall](../../../areas/wld/highmoor.wld#L4851), [second](../../../areas/wld/highmoor.wld#L4876)). Static exits also omit compiled travel: active Ixarkon’s `illithid_teleport_veil` destination list includes Highmoor attic #19275 ([assignment](../../../src/specs/specs.assign.c#L1874), [destination table](../../../src/specs/specs.ixarkon.c#L20)); its ceiling-tile connection to Dr. Fester’s chambers is reset closed/secret ([reset](../../../areas/zon/highmoor.zon#L132)). Runtime route availability, current movement, falls, and a safe return remain unverified. See the [Highmoor audit supplement](HIGHMOOR_AUDIT_REVIEW.md).

### Checkpoint 230: Highway boundaries, Surf approaches, and computed arrivals

The existing source audit records 539 Highway rooms, 1,411 authored exits, 1,410 active-room resolutions, and weak components of 533 and 6 rooms. Five outbound and five inbound records form reciprocal pairs with Mir, Bastine, and Surface; see the [active graph and pair table](SOUTHERN_COASTAL_HIGHWAY_AUDIT_REVIEW.md#active-wld-graph). Room #41488's down exit targets the negative #-1 sentinel and is dropped before renumbering ([source](../../../areas/wld/highway.wld#L4122), [unresolved row](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L210)). Its climb-down description does not identify a destination.

The all-95 inactive-WLD scan found two non-overlapping Surf approaches into active Highway rooms: #238784 south to #41837 and #239989 west to #41732 ([first exit](../../../areas/wld/surf.wld#L813740), [second exit](../../../areas/wld/surf.wld#L839041)). Keep them with Surf as source-only records; they do not establish active ingress or safe travel. No new WLD edge or source-only approach was found in this checkpoint.

Compiled `do_shift_prime` can select Highway room #41767 as a Githzerai arrival from Astral ([destination table](../../../src/classes/innates.c#L2010), [room](../../../areas/wld/highway.wld#L10107)). That computed arrival is separate from the WLD graph. Runtime availability, actual return path, door state, and travel safety remain unverified. See the [Highway audit supplement](SOUTHERN_COASTAL_HIGHWAY_AUDIT_REVIEW.md#checkpoint-230-builder-review-and-current-cross-area-dependencies).


### Checkpoint 231: Hunt directed graph and boundaries

The active Hunt WLD contains 149 rooms and 320 authored exits: 318 local and two outbound to Surface, all loader-resolved. Its local weak graph has one component. The directed graph has strong components sized 119, 15, 13, 1 and 1. From analysis seed #13307, 134 rooms are reachable and 134 have authored local WLD returns. The fifteen outside forward reach are #13350, #13429–#13433, #13435–#13436 and #13439–#13445; the fifteen without a WLD return are #13348, #13417–#13428, #13434 and #13437. These raw graph sets omit type-25 portals and the rescue ladder, and do not prove player access or safety.

Hunt has two active outbound and two inbound Surface records, with one reciprocal pair. Hunt #13307 south to Surface #650096 pairs with Surface #650096 north to #13307. Hunt #13307 west to #649693 differs from Surface #649695 east to #13307. See [cross-area exit rows 228–229 and 1681–1682](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L228) and [pair summaries 196 and 517](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L196). The all-95 inactive-WLD scan found no source-only approach or same-number room overlap.

Static travel also includes tent #13302 to #13350, rift #13339 to #13429, and the local #13363 ↔ Myrabolus #82539 portal pair between rooms #13341 and #82627. The secret rescue path begins above throne #13431 and exits one-way through #13445 to forest #13355. Reset state and runtime route availability, door state, arrival, return and travel safety remain unqualified. See the [Hunt audit supplement](HUNT_AUDIT_REVIEW.md#static-connectivity-and-dynamic-routes).


### Checkpoint 232: Hwood directed graph and Surf approach

The Haunted Woods has 58 active rooms and 157 WLD exits: 156 local and one outbound to Surface; all targets resolve. The local weak graph has one component, with a 57-room strongly connected component and singleton #10835. From Surface arrival #10800, 57 rooms are directed-reachable and all 58 have authored returns to #10800. #10835 is titled “Mob Load Room” and has exits into the forest, while the resets place roaming wildlife there. This is source topology, not proof of player access or live mobile movement.

The active boundary is reciprocal: Hwood #10800 south to Surface #517668 and Surface #517668 north to #10800 ([cross-area rows 205 and 1347](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L205), [pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L197), [Hwood exit](../../../areas/wld/hwood.wld#L19)). The all-95 inactive-WLD scan adds Surf room #209648 north to #10800 as one source-only approach ([Surf source](../../../areas/wld/surf.wld#L202471)). Keep it in Surf's candidate row 97; it is not active ingress. No Hwood room-VNUM overlap or other source-only approach was found. See the [Hwood audit supplement](HWOOD_AUDIT_REVIEW.md#static-connectivity-and-reachability).

### Checkpoint 233: IceCrag directed graph and boundaries

The active IceCrag WLD contains 243 local rooms and 628 authored exits: 624 local, three cross-area outbound, and one unresolved target. The unresolved down exit from #97085 targets absent room #97352, so 627 of 628 exits resolve. The local weak graph has one component; its 23 strongly connected components have sizes 219, 3, and twenty-one singletons. From analysis seed #97001, 220 rooms are forward-reachable and 239 have authored local WLD returns. Rooms outside forward reach are #97005, #97131, and #97133–#97153; rooms #97121, #97133, #97149, and #97153 have no local WLD return to the seed. Portals, reset/door state, and safe travel are outside this raw graph.

The two reciprocal active pairs connect IceCrag #97001 with Surface #511914 and IceCrag #97132 with Voluntown #142400. Winterhaven #55634 has a one-way northeast exit to IceCrag #97121, and IceCrag #97153 has a one-way south exit to Lava Tubes #12049. Two source-only records from inactive `surf.wld` #215567 and `underdark2011.wld` #224716 point east to #97001; keep them with their existing candidate records rather than treating them as active ingress. See the [IceCrag audit supplement](ICECRAG_AUDIT_REVIEW.md#local-directed-graph).

### Checkpoint 235: Illithid Arena directed graph and boundary

The active Illithid Arena WLD contains 18 rooms and 40 authored exits, all local and resolving. Its weak graph is one component; its SCCs have sizes 16, 1, and 1. From center #11200, 16 rooms are forward-reachable and all 18 have local authored returns. The two unreachable-from-center rooms #11216 and #11217 have one-way down exits back to #11200. The southwest pit route #11208/#11212 is reciprocal in source but reset closed and secret; the raw graph does not model discovery or door state.

The active-area sweep found no inbound or outbound WLD boundary to this contributor. The other 95 WLD files also have no inbound target or same-number room overlap. Two malformed exit blocks elsewhere in `northern_wilderness.wld` target #12270 and #6803, outside this room range. No source-only approach or candidate dependency was added. The compiled `squid_arena` WILL handler and the dormant guard-code cases are separate from this static exit graph; see the [Illithid Arena audit supplement](ILLITHID_ARENA_AUDIT_REVIEW.md#static-connectivity-and-hidden-arrivals).


### Checkpoint 236: Interspace grid and Astral boundary

Interspace contains 100 active rooms and 601 authored exits: 600 local and one outbound. Every target resolves, the local weak graph is one component, and all 100 rooms are in one directed strongly connected component. From #99900, all 100 rooms are forward-reachable and all 100 have a local authored return.

Room #99999 exits southeast (`D9`) to Astral Main #19801. This is Interspace's only WLD boundary and has no reverse WLD edge; see [exit-register row 1189](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1189) and [pair-summary row 202](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L202). The scan of all 351 active and 95 non-active WLD files found no inbound target or room-VNUM overlap. This rules out a checked-in WLD approach, while typed item travel remains a separate source layer.

The exit record's raw state is `4`, which `setup_dir` masks to zero before setting door flags. Interspace's ZON has no door reset for it. This is loader interpretation, not a gameplay observation. The external type-25 destination from Ravenloft 2 object #59310 to #99999 does not establish an active entrance or safe return; see the [Interspace audit supplement](INTERSPACE_AUDIT_REVIEW.md#typed-dependencies-and-tracked-history).

### Checkpoint 237: Ixarkon directed reachability and boundaries

Ixarkon contains 201 rooms and 468 authored exits, all resolving: 464 local and four outbound. Its local weak components and strongly connected components have sizes 199/1/1. The four unique active inbound arrival rooms (#96471, #96474, #96588, and #96600) each reach all 199 rooms in the large local component, and every room in that component has an authored return to each arrival. Five inbound records reach those four rooms, with two distinct records into #96471. Four outbound records each have a reverse pair. See the [Ixarkon audit supplement](IXARKON_AUDIT_REVIEW.md#local-directed-reachability), [inbound rows 69, 276, 390, 730, and 1755](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L69), [outbound rows 1151–1154](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1151), and [pair summaries 203–206](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L203).

Singleton rooms #96524 (the assigned veil chamber) and #96550 (the pet-shop storage backend) are outside local WLD reachability. Their code routes and service references are separate. Two unregistered `ud_map2.wld` exits target #96492, from #1120305 west and #1120404 north; they remain source-only, not active ingress. Preserve both on `ud_map2`'s existing candidate record. Curated totals are unchanged at 3,469 overall, 227 Surf, 18 `ud_map2`, and eight Underworld2 records. None of these authored edges establishes live player access or safe travel.

### Checkpoint 238: Ixxillikor directed reachability and boundaries

Ixxillikor has 199 rooms and 456 authored exits: 452 local and four outbound. All targets resolve, and the local weak graph has one 199-room component. Its strongly connected component sizes are 195/1/1/1/1. The three unique active inbound arrival rooms (#4200, #4380, and #4398) each reach the 195-room core; all 199 rooms can return to each arrival. Rooms #4332–#4335 are outside forward reach from those arrivals, although they have authored returns into the core. The four active cross-area edges each have an authored reciprocal exit; see the [Ixxillikor audit supplement](IXXILLIKOR_AUDIT_REVIEW.md#local-directed-reachability) and its four linked boundary pairs.

Those four single-room SCCs contain the elder brain and follower load room plus the adjacent NO_MOB rooms described in the dossier. This is source topology; room flags, resets, mob wandering, player access, door state, and safe travel remain separate. No source-only WLD approach or room-number overlap was found, so source-only totals do not change.

### Checkpoint 239: Jade Empire directed reachability and boundaries

Jade contains 340 rooms and 740 authored exits: 736 local and four outbound, all resolving. Its local weak components have sizes 327/7/6; SCCs have sizes 319/7/6 plus eight singletons. Five active inbound records reach four unique arrivals (#76616, #76620, #76760, and #76928). Each arrival reaches the 319-room core, and 325 of 340 rooms have authored paths back to each arrival. Twenty-one rooms lie outside forward reach; fifteen also lack a return path to the core. See the [Jade directed-graph supplement](JADE_AUDIT_REVIEW.md#local-directed-reachability).

The five active inbound records are Storm Port Stronghold #22649 northwest to #76616, Winterhaven #55632 northwest to #76760, Surface #642099 south to #76928, and Surface #642498 east plus #642899 north to #76620 ([inbound rows 378, 785, 1675, 1677, and 1678](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L378)). The four outbound records are #76620 south/west to Surface #642899/#642498, #76883 up to #642501, and #76928 north to #642099 ([rows 883–886](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L883)); three have active reverse records. Two unregistered `surf.wld` exits also point to Jade: #278596 south to #76928 and #279196 north to #76620. They remain source-only, not active ingress, and stay on Surf's existing candidate row 97. The all-95 scan parsed 2,080,773 exits without errors and found no Jade room overlap. Curated totals are now 3,471 overall, 229 for Surf, 18 for `ud_map2`, and eight for Underworld2. Static graph paths do not establish live access or safe return.

### Checkpoint 243: Juiblex directed reachability and boundaries

Juiblex has 183 rooms and 447 authored WLD exits: 442 local and five outbound, all resolving. Its local weak components have sizes 169, 5, 2, 2, and five single rooms. The sixteen directed SCCs have sizes 157, 5, 4, four pairs, and nine singletons. Treating all authored local exits as available, the Underdark arrival #87500 reaches five rooms. The Faerie Realm arrival #87628 reaches 157; 168 rooms have authored paths back to #87628. Realm #14208 is an administrative dispersal room, so its edge is not proof of public access. Door flags, resets, script actions, falls, and player safety are not modeled.

Active inbound WLD exits enter at #87500 from Underdark #772524 and at #87628 from Realm #14208. The Underdark pair is reciprocal. Juiblex has five outbound records: #87500 to Underdark #772524 and staging rooms #87671–#87674 to Fields Between #71106, Torg #29073, Mountain #9106, and Scorched Valley #71221. The four staging exits are not automatic fall chutes; eligible NPC wandering and player escort or return remain separate questions. See [inbound exits](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L245), [outbound exits](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1086), and [pair summaries](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L395).

The all-95 inactive-WLD scan records no source-room overlap or target in #87500–#87682. The separate Ravenloft 2 type-25 portal to #87528 is an object destination, not a WLD edge; see the [Juiblex audit supplement](JUIBLEX_AUDIT_REVIEW.md#typed-dependency-closure). Curated source-only totals remain 3,471 overall, 229 for Surf, 18 for `ud_map2`, and eight for Underworld2. Static reachability does not qualify a live entrance, a safe route, or return.


### Checkpoint 249: Hall of Knighthood directed reachability and boundaries

The Hall of Knighthood has 41 rooms and 80/80 resolving authored exits: 79 local and one outbound. Its local weak components are 40/1. Raw directed traversal from #70900 reaches 36 rooms; 39 rooms have authored paths back to #70900. #70936–#70939 and bookend #70999 are outside forward reach, while #70937 and #70999 have no exits. These counts do not apply 38 reset door records or qualify dynamic movement. The area has no fall-chance or current fields.

The one active outbound edge is #70900 south to Surface #639011, paired with Surface #639011 north back to Knight ([cross-area rows 848 and 1671](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L848)). The all-95 inactive-WLD scan found one source-only Surf approach, #259810 north to #70900, and no source-room VNUM overlap. Preserve it on Surf candidate row 97; it is not active ingress. Curated source-only totals now stand at 3,472 overall, 230 Surf, 18 `ud_map2`, and eight Underworld2. See the [Hall of Knighthood audit supplement](KNIGHT_AUDIT_REVIEW.md#active-graph-and-source-only-approach). Static reachability does not establish door state, player access, encounter availability, or safe return.

### Checkpoint 250: Kobold Settlement directed reachability and boundaries

Kobold has 147 rooms and 337 resolving authored exits: 335 local and two outbound. Treating all local exits as traversable, the local graph is one directed strongly connected component: each active arrival (#1400, #1402, and #1542) reaches all 147 rooms, and all rooms have authored paths back to each arrival. This raw graph ignores reset state and dynamic movement.

The reset file has sixteen unconditional `D` records: thirteen close doors (seven locked and six closed/unlocked), while three set exits blocked at #1449, #1463, and #1469. A strict static filter that removes every reset-closed or blocked local edge leaves 319 local edge records; it reaches six rooms from #1400 and 121 from each of #1402 and #1542. This is only a conservative edge-filter snapshot: keys, switches, speech handlers, guardian actions, and code travel are not simulated. Kobold has no fall-chance or current fields.

The active inbound edges are Surface #622130 north to #1400, Winterhaven #55402 southwest to #1402, and Underdark #816080 west to #1542. Kobold's two outbound edges are #1400 south to Surface #622130 and #1542 east to Underdark #816080; those two boundaries are reciprocal, while Winterhaven's arrival has no ordinary reverse WLD exit ([cross-area rows 22–23, 756, 1629, and 1734](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L22)).

The all-95 inactive-WLD scan found one additional source-only approach, `surf.wld` #207232 north to Kobold #1401, with no source-room overlap. Keep it on Surf's existing RETAIN candidate row 97; it is not active ingress. The reviewed Surf subset rises to 231 and curated source-only totals to 3,473 overall, 231 for Surf, 18 for `ud_map2`, and eight for Underworld2. See the [Kobold audit supplement](KOBOLD_AUDIT_REVIEW.md#directed-map-reset-states-and-boundaries). Static paths do not establish live admission, successful door traversal, dynamic routes, safe passage, or return.

### Checkpoint 252: Krimman directed reachability and boundaries

The 90-room `krimman` WLD has 211 resolving exits: 207 local and four outbound. Its local weak components contain 89 and one room; directed SCCs contain an 86-room core and four singletons. From the active arrival #16400, raw WLD traversal reaches 86 rooms and 88 have authored returns to #16400. Rooms #16486–#16489 are outside forward reach; #16486 is Eckraldu's load room. These counts do not simulate reset doors, keys, hidden passages, traps, mobile wandering, or player safety. The WLD has no fall-chance or current fields.

Twisted Wood #16378 east to #16400 and the reciprocal Krimman #16400 west to #16378 form the sole active inbound/outbound pair ([cross-area rows 291–292](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L291)). Three outward records leave the unreachable Eckraldu load room #16486 for Mistywood #95100, Twin Towers Forest #13595, and Woodseer #16660 ([rows 293–295](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L293)); none has an active reverse edge. The source dossier describes ordinary mobile wandering but does not establish Eckraldu's live location or a player route through that load room.

The all-95 inactive-WLD scan found no source-room overlap and no approach into Krimman. Curated source-only counts remain 3,473 overall, 231 for Surf, 18 for `ud_map2`, and eight for Underworld2; no candidate row is added for Krimman. See the [Krimman audit supplement](KRIMMAN_AUDIT_REVIEW.md#directed-graph-and-active-boundaries). Static paths do not establish current door state, a live encounter, access to the mobile load room, or safe return.

### Checkpoint 258: Lava Tubes directed reach and Surf source records

The active Lava Tubes graph has 152 rooms and 317 authored exits: 311 local, five resolved outbound, and one loader-unresolved target. Its weak graph is one component. Each of the six active arrivals (#12037, #12097, #12161, #12049, #12104, and #12131) reaches all 152 rooms in raw directed WLD, and every room has a local WLD return to each arrival. IceCrag's #97153 south exit is a one-way active arrival; the five outbound pairs are with Bloody Plains, ConnectorZones, and MountainTracks. Reset doors and scripted behavior are outside this raw graph.

Ten reset-closed directions across five local pairs leave 148 of 152 rooms in the combined forward reach and return sets from the six arrivals; the weak components then have sizes 140, 8, 1, 1, 1, 1. This is a filter over ZON reset rows, not current door-state qualification. The active east exit from #12138 to #282989 is unresolved and pruned by the normal loader; its VNUM appears only in inactive Surf and surface2011 sources.

The all-95 inactive-WLD sweep adds four non-overlapping source-only approaches from Surf: #280593 west to #12118, #282376 south to #12097, #282989 west to #12138, and #283874 north to #12037. They are not active ingress. Preserve them on Surf candidate row 97. Curated source-only totals are now 3,477 overall and 235 for Surf, with ud_map2 at 18 and Underworld2 at eight. See the [Lava Tubes audit supplement](LAVA_TUBES_AUDIT_REVIEW.md#active-wld-boundaries-and-directed-reach).

### Checkpoint 259: LavCav directed reachability and the isolated adventurer rooms

Lava Caves has 142 rooms and 451 resolving WLD exits: 450 local and one outbound. Its local undirected graph is one component, but the raw directed graph has a 140-room strongly connected component plus singletons #35635 and #35634. From entrance #35501, 140/142 rooms are forward-reachable and 141/142 have authored local returns. The adventurer spawn room #35635 and its east neighbor #35634 are outside entrance forward reach; #35634 has no authored exit and no return path to the entrance. The mobile's wandering behavior does not supply an ordinary player edge.

The sole active outside boundary is reciprocal: Lava Caves #35501 north to Mountain Tracks #21148 and Mountain Tracks #21148 south to #35501 ([exit rows 356 and 469](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L356), [pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L247)). Reset-closed/locked/secret doors, the blocked cracked-wall exit, key use, and fire-sector effects are not modeled as successful traversal by this raw graph.

The all-95 inactive-WLD scan found no room-VNUM overlap and no source-only target into Lava Caves. The typed player_guilds.shp reference to #35552 is a shop-room field and a potential loader-fallback input, not WLD ingress or confirmed operating stock. Curated source-only totals remain 3,477 overall, 235 for Surf, 18 for ud_map2, and eight for Underworld2. See the [Underground Lava Caves audit supplement](UNDERGROUND_LAVA_CAVES_AUDIT_REVIEW.md#checkpoint-259-directed-wld-reachability).

### Checkpoint 260: Arcaneum directed reachability and dream portals

The Arcaneum has 150 rooms and 410 resolving exits: 409 local and one outbound. Its local weak components contain 113 and 37 rooms. From entrance #402000, raw directed traversal reaches 110 rooms; 112 rooms have local authored paths back to #402000. The strongly connected components contain 110, 28, 2, and ten singleton rooms. The dream-plane range #402073–#402109 and collector staging rooms #402147–#402149 are outside entrance forward reach. The dream plane has no local WLD return to #402000; #402147 and #402148 can return via #402147 down to #402002, while #402149 has no exits. These results do not simulate reset state, mobile wandering, or safe player access.

The sole outbound WLD edge is #402000 north to Underdark #813342, paired with Underdark #813342 south to #402000 ([cross-area rows 1315 and 1731](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1315)). Type-25 objects #402037 and #402038 declare local travel between the main and dream-plane components; portal arrival is not part of the WLD graph. The all-95 inactive-WLD scan found no room overlap or source-only target into the Arcaneum. No source-only candidate or curated inbound count changes. See the [Arcaneum audit supplement](ARCANEUM_AUDIT_REVIEW.md#checkpoint-260-directed-wld-reachability).

### Checkpoint 263: Llyrath directed WLD reachability

Llyrath has 93 rooms and 234 resolving exits: 233 local and one outbound to Moonshae #26257, paired with Moonshae's return edge. Its local weak graph contains all 93 rooms. From arrival #94900, raw directed traversal reaches 92/93 rooms, while all 93 have authored local paths back to #94900. The SCC sizes are 92 and one singleton, #94992. That room is outside ordinary WLD forward reach from #94900 but has a return path via its down exit to #94912. Object #94906, reset in cave #94991, separately declares type-25 travel to #94992; this dynamic destination is not a WLD exit.

The four current-marked rooms remain outside any conclusion about swimming, survival, and safe return. The cave's one-way prose still differs from its authored south return exit, and the clearing/shack resets do not prove live door state. The all-95 inactive-WLD scan found no room overlap or source-only target into Llyrath. No candidate or curated source-only count changes. See the [Llyrath source review](LLYRATH_STORY_REVIEW.md#checkpoint-263-directed-wld-reachability).

### Checkpoint 264: Llzazan directed reachability and compiled code destinations

Llzazan has 197 rooms and 503 resolving exits: 498 local and five outbound.
All 197 rooms form one weak component and one directed strongly connected
component. Raw WLD traversal from #36200 reaches all 197 rooms and returns to
#36200 from all 197. The graph includes reset-controlled and secret exits as
authored; it does not establish current door states, current effects, mobile
movement, player access, or safe travel. Six rooms have current settings and
none has a fall-chance field.

The active map has 42 inbound WLD exits to 40 distinct Llzazan rooms. Wander
contributes 36 one-way exits from six rooms (#36601–#36606), Surface one
one-way exit to #36287, and the other five arrivals are reciprocal: three
Arac-Web pairs, one Arac-Wild pair, and one Underdark pair. Llzazan’s five
outbound edges are #36200 south to #36199, #36242 north to #36546, #36262 south
to #805516, #36285 west to #36573, and #36363 west to #36582
([outbound rows 477–481](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L477),
[reciprocal Arac-Web rows](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L489),
[Wander rows](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L493),
[Surface row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1685),
[Underdark row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1722)).

The all-95 inactive-WLD sweep found no Llzazan room overlap or source-only
approach; coverage records zero inactive-WLD target references. Curated
source-only totals remain 3,477 overall, 235 for Surf, 18 for ud_map2, and
eight for Underworld2. Preserve the two source-only ud_map0 reset records for
Llzazan mobile #36225 on existing candidate row 112; these typed references
sit outside the active WLD graph. See the [Llzazan review](LLZAZAN_STORY_REVIEW.md),
[coverage row 162](ZONE_COVERAGE_REGISTER.csv#L162), and [candidate row 112](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L112).

### Checkpoint 265: Adventurer's Vaults static rooms and dynamic locker paths

The active lockers map has 99 authored rooms, #65201–#65299, and zero WLD
exits. Its raw graph therefore has 99 singleton weak components; there are no
active inbound or outbound WLD boundaries. The all-95 inactive-WLD sweep finds
no overlap or approach, and the coverage row records no current or fall fields.

Compiled storage code uses #65201 as a base/staging location and allocates
#65202–#65299 as up to 98 temporary locker rooms. It populates each room and
creates a personalized exit back to the requesting player's entry room. Access
is invoked by bank-counter object hooks and a guildhall room hook, so the
active storage path exists in code and does not appear as authored WLD edges.
The boot loader's numeric special-case compares its loaded room index with
65201–65300, while the allocator resolves locker VNUMs through real_room0; ask
the code owner to reconcile those ranges before changing either side.

No source-only WLD record or candidate is added. The zone remains active
system support and DO NOT MOVE. See the [locker source review](LOCKERS_STORY_REVIEW.md),
[coverage row 163](ZONE_COVERAGE_REGISTER.csv#L163), and the
[history crosswalk](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-265-adventurers-vaults-title-and-tracked-path-history).

### Checkpoint 266: Defense of Longhollow directed WLD graph

The active long map has 100 rooms and 251/251 resolving exits: 249 local and
two outbound from forest entrance #34401. Its internal graph is one weak
component and one directed SCC; raw traversal from #34401 reaches and returns
from all 100 rooms. The outbound edges go north to Surface #520903 and west to
#521302. Surface returns south and east to #34401, making two reciprocal pairs
([outbound rows 458–459](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L458),
[inbound rows 1361 and 1363](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1361)).

Fourteen D resets set their listed doors open and unlocked. No room has a
current or fall-chance field; all active WLD targets resolve. Raw source reach
does not qualify player arrival, loaded door state, transport, scripted
movement, or safety. See the [Longhollow audit review](LONGHOLLOW_AUDIT_REVIEW.md),
[coverage row 164](ZONE_COVERAGE_REGISTER.csv#L164), and exact boundary rows.

The all-95 inactive-WLD sweep also found a source-only surf.wld approach:
#222252 south to #34401 ([source exit](../../../areas/wld/surf.wld#L466844)).
It is not active ingress and stays on Surf's existing retained candidate row
97 ([candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97)); no new
candidate is added. The reviewed Surf subset rises to 236 and
curated source-only totals rise to 3,478 overall, including 236 for Surf.

### Checkpoint 267: Lornecro directed reachability and Surface pair

The active lornecro map has 100 rooms and 207/207 resolving exits: 206 local
and one outbound from Ironstar entrance #138978 north to Surface #512296. The
Surface room exits south back to #138978, a single reciprocal pair
([Ironstar exit, row 1312](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1312),
[Surface return, row 1324](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1324)). The raw
local graph has one weak component and directed SCC sizes 98, 1, and 1.
Traversal from Wherston Road #138900 or arrival #138978 reaches 98 rooms; 99
rooms can route back to #138900. Rooms #138991 and #138992 are outside the
main SCC: #138991 leads toward the graph and #138992, while #138992 has no exit.

The WLD has 52 door-marked exits, ten current-marked rooms, and three
fall-chance rooms; 50 D resets set effective door states. An all-95 inactive
WLD scan found no non-overlapping approach into lornecro. Inactive Duris3.wld
does overlap all 100 room VNUMs, but its same-numbered rooms are separately
named Ocean source records, not loaded Ironstar rooms. Preserve this identity
boundary on existing candidate row 36. Player access, reset state, secret
discovery, hazards, and safe travel remain unqualified. See the
[Ironstar audit review](LORNECRO_AUDIT_REVIEW.md),
[coverage row 165](ZONE_COVERAGE_REGISTER.csv#L165), and
[Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

### Checkpoint 268: The Tower of Darkness raw exits and inactive Duris3 approaches

The active lortower WLD has 142 rooms and 500/500 resolving exits: 499 local
and one outbound from #134000 east to Troll Hills #1883. No active WLD exit
enters the Tower range. The local raw exit graph has four weak components of
91, 32, 12, and 7 rooms; directed SCC sizes are 75, 32, 12, 7, 5, 2, 2, and
seven singletons. Raw traversal from #134000 reaches 75 rooms in the largest
91-room component. It does not include password state or directional
ITEM_TELEPORT transitions, which the [source dossier](../zone-stories/TOWER_OF_DARKNESS.md#actual-dialogue-magic-doors-and-travel)
maps separately. The existing scan found no foreign fixed ITEM_TELEPORT
destination into Tower. The Troll Hills exit does not return into Tower:
#1883 west leads to #1882. Dynamic travel and player entry remain unqualified.

The all-95 inactive-WLD sweep found 286 non-overlapping Duris3 source exits
into Tower: 142 south from #133700–#133841, #133999 east to #134000, #134142
west to #134141, and 142 north from #134300–#134441. Another 282 target
matches begin in same-numbered inactive Duris3 source rooms and are excluded
from that approach count. These are not active ingress and remain on existing
[Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).
Curated source-only totals rise to 3,764. See the
[Tower audit review](LORTOWER_AUDIT_REVIEW.md#directed-wld-graph-and-entry-boundary),
[coverage row 166](ZONE_COVERAGE_REGISTER.csv#L166), and
[active cross-area row 1307](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1307).

### Checkpoint 269: Lylr-Meop's split graph and Surface approaches

The active Lylr map has 64 rooms and 134/134 resolving exits: 132 local and
two outbound to Surface. Its weak components contain 40 and 24 rooms; raw
traversal from #82200 reaches and can return from 40. The other component is
#82240–#82263. Neither Surface target has an active reverse WLD exit into
Lylr ([pair summary row 258](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L258)).

The all-95 inactive-WLD sweep found the two existing Surf approaches:
#222832 north to #82240 and #224629 south to #82200. They are source-only,
not active ingress, and remain on Surf candidate row 97. These two records
are already included in the current curated source-only totals; this
checkpoint adds no approach or candidate. See the
[Lylr audit supplement](LYLR_MEOP_AUDIT_REVIEW.md#active-wld-boundaries-and-directed-reach),
[coverage row 167](ZONE_COVERAGE_REGISTER.csv#L167), and
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


### Checkpoint 270: Magma's active graph and inactive approaches

The active Magma map has 129 rooms and 612/612 resolving exits. Its weak
components contain 128 and one room; the 125-room grid is strongly connected,
while info room #142000 is isolated. Control rooms #142127 and #142128 can
reach 127 rooms and have no authored return route; holding room #142126 has no
exits. The map has no active WLD inbound or outbound boundary. These results
describe authored exits and do not qualify a dynamic arrival or safe return.

The all-95 inactive-WLD sweep found 261 non-overlapping-source records into
Magma: 260 from Duris3 and one from Vodgh. Another 256 Duris3 matches start in
same-numbered Ocean rooms and remain excluded from that approach count. The
records are source-only and remain on existing Duris3 candidate row 36; there
is no Surf approach and no new candidate. The curated total remains 3,764,
including 236 Surf, 18 ud_map2, and eight Underworld2. See the
[Magma audit review](MAGMA_AUDIT_REVIEW.md#active-world-and-entry-boundaries),
[coverage row 168](ZONE_COVERAGE_REGISTER.csv#L168), and
[Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).


### Checkpoint 271: Malch's reciprocal crossings and hidden interior

The active Malch WLD has 161 rooms and 360/360 resolving exits, all in one weak
component. Its four cross-area pairs remain reciprocal. From each of the three
city entries, the authored passable graph reaches and can return from 159 local
rooms; rooms #23759 and #23760 become source-reachable when the paired secret
controls are allowed. Nineteen rooms carry fall-chance markers. These counts
describe source edges and do not qualify reset state, search, safe traversal,
or a live character's route.

The inactive-WLD sweep found the existing Surf source-only approach from
#267236 down to Malch #23600. It remains on Surf candidate row 97 and was
already included in the curated source-only total. No new edge or candidate is
added; current totals remain 3,764 overall, 236 Surf, 18 ud_map2, and eight
Underworld2. See the [Malch audit review](MALCH_AUDIT_REVIEW.md#active-graph-and-boundaries),
[coverage row 169](ZONE_COVERAGE_REGISTER.csv#L169), and
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


### Checkpoint 272: The Forgotten Mansion graph and one-way White House entry

The active Mansion WLD has 176 rooms and 390/390 resolving exits in four weak
components of 158, 16, 1, and 1 rooms. Its west exit from #3500 to Surface
#588620 is reciprocal. The Mansion also receives Surface #588620's return and
White House #55634 northwest to #3675; the White House edge has no authored
reverse WLD route. The other local exits and their door states remain part of
the source graph, not proof of safe live traversal.

The inactive-WLD scan retains two approaches to #3500: Surf #275277 east and
Underworld2 #436465 east. Both remain source-only on their existing candidate
rows and were already included in the curated total. This checkpoint adds no
approach or candidate; current totals remain 3,764 overall, 236 Surf, 18
ud_map2, and eight Underworld2. See the
[Mansion audit review](MANSION_AUDIT_REVIEW.md#active-graph-and-inactive-approaches),
[coverage row 170](ZONE_COVERAGE_REGISTER.csv#L170),
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and
[Underworld2 candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv).


### Checkpoint 273: Fort Marigot's reciprocal Surface routes

Fort Marigot has 118 rooms and 265/265 loader-resolved exits in one weak local
component. Its two outbound Surface edges from #5315 and #5417 have reciprocal
Surface returns. Two local rooms carry fall-chance values. These are authored
source connections and do not establish live door state, safe traversal, or
player access.

The previously recorded source-only approaches remain on existing Surf and
Underworld2 candidates: Surf #275277 east and Underworld2 #436465 east both
target Marigot #5300. They are unselected source declarations already included
in the cumulative total of 3,764, with 236 Surf, 18 ud_map2, and eight
Underworld2. This checkpoint adds no approach or candidate. See the
[Marigot source review](MARIGOT_STORY_REVIEW.md#static-connectivity-and-replacement-boundary),
[coverage row 171](ZONE_COVERAGE_REGISTER.csv#L171),
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and
[Underworld2 candidate register](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv).


### Checkpoint 274: Maze of Undead Army entrance and carrier staging rooms

The active Maze map has 177 rooms and 395/395 resolving exits in one weak
component. Its sole active boundary is reciprocal with Surface at Maze entrance
#94177 and Surface #571011. A directed scan from the entrance reaches 173
rooms; four carrier staging rooms (#94117, #94118, #94119, and #94121) remain
outside that traversal. Door state, mobile roaming, dynamic travel, and safety
remain separate from authored graph reach.

The inactive WLD scan found Surf #281856 east to #94177. It remains source-only
on existing Surf candidate row 97. The inactive brass-old-1 reset of object
#94009 is separately retained on that candidate's source dependency record.
Both were already counted. No new approach or candidate is added; curated
totals remain 3,764 overall, 236 Surf, 18 ud_map2, and eight Underworld2. See
the [Maze audit review](MAZE_OF_UNDEAD_ARMY_AUDIT_REVIEW.md#active-graph-and-boundary),
[coverage row 172](ZONE_COVERAGE_REGISTER.csv#L172), and
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


### Checkpoint 275: Mazzolin's reciprocal UnderDark boundaries

The active Mazzolin WLD has 136 rooms and 288/288 resolving exits in one weak
component. Cave #21300 connects reciprocally with UnderDark rooms #803053 and
#802652. From #21300, authored directed traversal reaches all 136 rooms and
returns to the root from 93. Secret doors, spell effects, one-way local routes,
and player safety remain separate from this raw graph.

The inactive-WLD sweep found the existing ud_map2 source-only approach from
#1125718 south to #21300. It remains on the existing ud_map2 candidate record
and is already included in current totals: 3,764 overall, 236 Surf, 18 ud_map2,
and eight Underworld2. This checkpoint adds no route or candidate. See the
[Mazzolin audit review](MAZZOLIN_AUDIT_REVIEW.md#active-map-and-reciprocal-underdark-boundaries),
[coverage row 173](ZONE_COVERAGE_REGISTER.csv#L173), and
[ud_map2 candidate register](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv).


### Checkpoint 276: Menden-on-the-Deep's reciprocal routes and special landing

The active Menden map has 71 rooms and 165/165 resolving exits. Four active
boundary pairs reciprocate across three Surface endpoints and an Ocean room.
The local WLD graph has components of 70 and one room; ordinary authored
traversal from #88863 reaches and returns from 70, while isolated room #88860
has no ordinary exit. The selected pool handlers provide separate routes into
and out of #88860. No fall or current fields are present; live access and safety
remain unqualified.

The all-95 inactive-WLD scan found two existing Surf approaches into pier
#88846: #232071 east and #232372 north. They remain source-only on Surf candidate
row 97 and are already counted. This checkpoint adds no route or candidate;
curated totals remain 3,764 overall, 236 Surf, 18 ud_map2, and eight Underworld2.
See the [Menden audit review](MENDEN_AUDIT_REVIEW.md#active-connectivity-and-map-admission),
[coverage row 174](ZONE_COVERAGE_REGISTER.csv#L174), and
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


### Checkpoint 277: Halfling Silver Mine graph and Alterian overlaps

The active Mining map has 69 rooms and 153/153 resolving exits in one weak
component. Its only active boundary reciprocates with Surface at #4900 and
#566915. Directed traversal from #4900 reaches 64 rooms and can return from
68. The nine curated non-overlapping-source approaches remain source-only:
seven from Alterian maps and two from Surf. An additional 151 raw target matches
begin in 69 same-numbered Alterian source rooms and remain excluded as overlap
identities.

The nine approaches were already counted; this checkpoint adds no route or
candidate. Current curated totals remain 3,764 overall, 236 Surf, 18 ud_map2,
and eight Underworld2. See the
[Mining audit review](MINING_AUDIT_REVIEW.md#active-wld-boundaries-and-directed-reach),
[coverage row 175](ZONE_COVERAGE_REGISTER.csv#L175), and
[Alterian candidate records](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv).

### Checkpoint 278: Mini Zones and source-only Surf approaches

The active Mini Zones map has 695 source exit records; 686 resolve in the
loaded world and nine target rooms present only in unregistered WLD sources.
Its 15 weak components have a largest component of 69 rooms; 15 rooms have
fall chances and none has currents. Fourteen active exits enter and fourteen
leave Mini Zones, with twelve reverse-direction records. Heavens and `wh`
have one-way inbounds; Mist and Valley of Crushk have one-way outbounds.

The all-95 inactive-WLD sweep finds eleven non-overlapping Surf exits into
Mini Zones rooms #5700, #5734, #5735, #5756, #5770, #5785, #5795, #5819,
#5830, #5847, and #5868. These remain source-only, not active ingress. The
curated total rises to 3,775 overall and 247 for the reviewed Surf subset;
ud_map2 remains 18 and Underworld2 eight. See the [Mini Zones audit review](MINIZONES_AUDIT_REVIEW.md#active-and-source-only-connectivity),
[coverage row 176](ZONE_COVERAGE_REGISTER.csv#L176), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97),
and boundary pair summaries [176](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L176),
[272](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L272), [273](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L273), and
[802](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L802).

### Checkpoint 279: Deep Ravine reciprocal routes

The active Deep Ravine map has 178 rooms and 404/404 resolving exits in five
weak components (81, 27, 27, 23, and 20 rooms). All six active outbounds have
reciprocal returns: four Surface pairs and two Underdark pairs. The map has no
active unresolved targets, five fall rooms, and no currents.

The four non-overlapping Surf approaches to rooms #94878, #94721 (two exits),
and #94700 were already curated at Checkpoint 79. This checkpoint adds no
route or candidate; source-only totals remain 3,775 overall, 247 Surf, 18
ud_map2, and eight Underworld2. See the [Deep Ravine audit review](DEEP_RAVINE_AUDIT_REVIEW.md#active-wld-graph),
[coverage row 177](ZONE_COVERAGE_REGISTER.csv#L177), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


### Checkpoint 280: Forest of Mir reciprocal boundaries

Active Mir has 281 rooms and 558/558 resolving exits: 556 local and two
cross-area exits. Its local weak components have 271 and 10 rooms. Raw directed
reach from #41900 includes 237 rooms; 271 rooms have a path back. The two
Highway boundaries, #42137 south↔#41316 north and #42152 east↔#41585 west, are
reciprocal. The 34-room forest component and ten-room island/lair component
remain outside ordinary root-forward reach; the latter also has no authored
return to #41900. The inactive-WLD sweep found no room overlap or target into
Mir, so this checkpoint adds no source-only route. See the [Mir audit review](MIR_AUDIT_REVIEW.md#active-wld-boundaries-and-directed-reach), [coverage row 178](ZONE_COVERAGE_REGISTER.csv#L178), and boundary pair summaries [194](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L194) and [276](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L276).


### Checkpoint 281: Myrabolus boundaries and two Surf approaches

The active Myrabolus map has 188 rooms and 393/393 resolving exits (387 local,
six across area boundaries), in weak components of 182, 2, 1, 1, 1, and 1
rooms. Four active boundary pairs are reciprocal: three with Surface and one
with Darkfall. Shantil's Stronghold and Winterhaven each have an unpaired
inbound; Myrabolus room #82667 has two unpaired outbound routes to Desolate and
Cerebus Park. See boundary pair summary rows [110](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L110), [277–280](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L277), [451](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L451), [544](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L544), and [803](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L803).

The all-95 inactive-WLD sweep adds two non-overlapping source-only Surf exits:
#235782 south to #82670 and #236382 north to #82500. They are not active
ingress. Preserve both on Surf candidate row 97; curated totals rise to 3,777
overall and 249 Surf, while ud_map2 remains 18 and Underworld2 eight. See the
[Myrabolus audit review](MYRABOLUS_AUDIT_REVIEW.md#source-only-inactive-wld-approaches), [coverage row 179](ZONE_COVERAGE_REGISTER.csv#L179), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


### Checkpoint 282: Shadow Forest route and guardian review

The active `mist` graph remains 73 rooms and 199/199 resolving exits, with
197 local exits and two outbound. Four active records enter from Minizones,
NewFor, and Surface; the two Surface paths are reciprocal. The three
non-overlapping inactive-WLD approaches from Alterian Wilderness and Aopal
were already counted at Checkpoint 164. This checkpoint adds builder questions
only; no route is added. Current curated totals remain 3,777 overall, 249 Surf,
18 ud_map2, and eight Underworld2. See the [Shadow Forest audit review](SHADOW_FOREST_AUDIT_REVIEW.md#active-wld-boundaries-and-directed-reach), [coverage row 180](ZONE_COVERAGE_REGISTER.csv#L180), and [existing source-only routes](SHADOW_FOREST_AUDIT_REVIEW.md#inactive-wld-sweep).


### Checkpoint 283: Chasm routes and counted inactive approaches

The active Chasm remains 100 rooms with 209/209 resolving exits: 208 local
and one reciprocal Surface boundary pair. Authored directed exits reach and
return from all 100 rooms when door and hazard admission are assumed. The two
inactive approaches from `surf.wld` to #15000 and `torr.wld` to #15052 were
already counted at Checkpoint 112. This checkpoint adds builder questions only;
no route is added. Curated totals remain 3,777 overall, 249 Surf, 18 ud_map2,
and eight Underworld2. See the [Chasm audit review](MIST_CHASM_AUDIT_REVIEW.md#active-graph-and-boundary), [coverage row 181](ZONE_COVERAGE_REGISTER.csv#L181), and [counted inactive approaches](MIST_CHASM_AUDIT_REVIEW.md#inactive-approaches-and-attic-dependencies).


### Checkpoint 284: Mistywood source reconciliation

Mistywood remains a 203-room contributor with 580 authored exits, 542 loader
resolutions, three active inbound records, one active outbound record, and one
reciprocal Underworld3 pair. Its 38 positive unresolved exits still target
absent old-map rooms. The all-inactive-WLD sweep's 53 non-overlapping Surf
approaches into 27 Mistywood rooms were already counted on the existing Surf
candidate row 97; this checkpoint adds no route or candidate. Treat those
records as source-only, separately from the three active inbounds and 38
unresolved active exits. Curated totals remain 3,777 overall, 249 Surf, 18
ud_map2, and eight Underworld2. See the [Mistywood audit review](MISTYWOOD_AUDIT_REVIEW.md#connectivity-reconciliation), [coverage row 182](ZONE_COVERAGE_REGISTER.csv#L182), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


### Checkpoint 285: Mitashi boundaries and Duris3 records

Mitashi remains a 229-room graph with 572/572 resolving exits. Its two
inbounds and two outbounds share one reciprocal southern Surface pair; the
northern Surface records have different endpoints and remain non-reciprocal.
Authored local edges reach and return from all 229 rooms when recorded exits
are treated as available. The all-95 inactive-WLD sweep's 916 Duris3 records
into Mitashi divide into 462 exits from non-overlapping source rooms and 454
from source-room VNUMs overlapping active Mitashi. The 462 non-overlapping
approaches were already counted; preserve the overlaps separately as map
identity evidence. This checkpoint adds no route or candidate row. Curated
totals remain 3,777 overall, 249 Surf, 18 ud_map2, and eight Underworld2. See
the [Mitashi audit review](MITASHI_AUDIT_REVIEW.md#inactive-duris3-map-records), [coverage row 183](ZONE_COVERAGE_REGISTER.csv#L183), and [Duris3 map candidate](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv).


### Checkpoint 286: Du'Maathe Castle active pairs and counted approaches

Du'Maathe Castle remains a 506-room contributor with 1,224/1,224 resolving
exits, one local weak component, and no unresolved targets, fall fields, or
currents. Both active boundary pairs remain reciprocal: Castle #37416 west
with Surface #563532 east, and Castle #37574 south with Dracopit #15968 north.
The five inactive-WLD approaches from Surf and oldmap8 to rooms #37409, #37416,
and #37425 were already counted on existing candidate rows. They remain
source-only, not active ingress; no route or candidate row is added. Curated
totals remain 3,777 overall, 249 Surf, 18 ud_map2, and eight Underworld2. See
the [Castle audit review](DU_MAATHE_CASTLE_AUDIT_REVIEW.md#active-and-source-only-wld-boundaries), [coverage row 184](ZONE_COVERAGE_REGISTER.csv#L184), and [existing Surf/oldmap8 candidates](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv).


### Checkpoint 287: Mobs Surf graph and legacy targets

Active Mobs_Surf has 24 rooms and 64 authored exits: 40 local and 24
unresolved positive targets. Its local graph has four components of 11, 5, 5,
and 3 rooms. Every unresolved VNUM appears in both inactive `surf.wld` and
`surface2011.wld`, where same-numbered room identities can differ. These are
outbound legacy map references, not active boundaries or source-only ingress;
no route or candidate row is added. Curated totals remain 3,777 overall, 249
Surf, 18 ud_map2, and eight Underworld2. See the [Mobs_Surf audit review](MOBS_SURF_AUDIT_REVIEW.md#active-graph-and-unresolved-surface-targets), [coverage row 185](ZONE_COVERAGE_REGISTER.csv#L185), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


### Checkpoint 288: Mobs Underdark static connectivity

Active `mobs_underdark` has 15 rooms and 98 exit records. Four resolve in the
active world: the reciprocal local pair #24013/#24014 and outbound exits from
#24007 to Underworld3 #30149 and #24012 to Ixarkon #96471. The other 94
positive targets are unresolved; 18 also occur in inactive `ud` WLD sources.
There are no active inbound exits or reverse records for the two outbound
boundaries. The 18 `ud` matches were already retained on the existing `ud`
candidate row; no route or candidate is added. Curated totals remain 3,777
overall, 249 Surf, 18 ud_map2, and eight Underworld2. See the [Mobs_Underdark audit review](MOBS_UNDERDARK_AUDIT_REVIEW.md#active-graph-and-unresolved-targets), [coverage row 186](ZONE_COVERAGE_REGISTER.csv#L186), and [existing `ud` candidate row 110](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L110).


### Checkpoint 289: Moonshae routes and source-only approaches

Moonshae retains 127 rooms, 266 authored exits, 264 loader-resolving exits,
one local weak component, and five reciprocal active cross-area boundaries.
The two positive port exits from #26200 to #148979 and #149179 occur only in
inactive Duris3.wld and remain unresolved. The inactive Surf approach #219584
west to Moonshae #26208 is already counted on Surf candidate row 97; it adds no
route or curated-count change. Keep the Myrloch Vale type-25 waterfall into
#26309 and the #26285 sea self-loop as separate source and owner questions.
This checkpoint adds no route. See the [Moonshae audit review](MOONSHAE_ISLAND_AUDIT_REVIEW.md#active-graph-and-cross-area-routes), [coverage row 187](ZONE_COVERAGE_REGISTER.csv#L187), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


### Checkpoint 290: Morg's Place boundary and reset-closed routes

Active morg has 30 rooms and 69 resolving exits: 68 local plus the reciprocal
UnderDark chasm boundary at #88600/#844836. Its raw weak graph is one component,
but the entry pair, ten ordinary doors, and four secret exits reset closed.
The chasm arrival is outside the tavern's closed east door. No inactive-WLD
overlap or source-only approach was found in the repository-wide scan. Preserve
the authored graph and separate static connectivity from actual door opening,
secret discovery, and movement. No route is added. See the [Morg audit review](MORG_AUDIT_REVIEW.md#active-wld-graph-and-source-text-gaps), [coverage row 188](ZONE_COVERAGE_REGISTER.csv#L188), and the [cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv).


### Checkpoint 291: Moria boundaries and compiled room travel

Moria has 335 rooms and 1,041 authored exits, all resolving, in one 335-room
weak component. Its two reciprocal Surface pairs are #99028/#528071 and
#99335/#528069. The all-95 inactive-WLD sweep found no source-room overlap or
source-only approach into Moria. Compiled do_shift_prime separately names room
#99295 as one Githzerai destination; it does not add a WLD exit or source-only
route count. Actual room access, maze state, and played travel remain
unqualified. See the [Moria audit review](MORIA_AUDIT_REVIEW.md#active-graph-and-source-only-wld-sweep), [coverage row 189](ZONE_COVERAGE_REGISTER.csv#L189), and the [cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv).


### Checkpoint 292: Mosswood boundary and source-only approaches

Active moss has 161 rooms and 399/399 resolving exits, including one
reciprocal Surface boundary at #23400/#611257. Ordinary directed traversal
from #23400 reaches 157 rooms; four room records remain outside that root
reach. The inactive-WLD sweep found no source-room overlap and retained three
source-only Surf approaches to #23400 on existing candidate row 97. Preserve
the active pair and the source-only classification; no route or candidate is
added. Actual actor access and travel remain unqualified. See the [Mosswood
audit review](MOSSWOOD_AUDIT_REVIEW.md#active-wld-boundaries-and-directed-reach),
[coverage row 190](ZONE_COVERAGE_REGISTER.csv#L190), and [Surf candidate row
97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 293: Mountain of the Banished boundaries and staging

Active mount has 67 rooms and 135/135 resolving exits: 134 local and one
outbound. Three active inbound exits include the reciprocal Surface entrance
#9100/#517517 and one-way Juiblex and Scorch Valley load-room paths to #9106
and #9122. The authored graph has four weak components; with ordinary gates
and all six fixed ENTER objects assumed available, raw traversal from #9100
reaches 64/67 rooms, leaving administrative #9157, #9165, and #9166 outside.
The inactive-WLD sweep retains two non-overlapping source-only approaches to
#9100 on existing oldmap8 and Surf candidates. Preserve these distinctions;
no route is added and actual access remains unqualified. See the [Mountain
audit review](MOUNTAIN_OF_THE_BANISHED_AUDIT_REVIEW.md#active-graph-and-route-types),
[coverage row 191](ZONE_COVERAGE_REGISTER.csv#L191), and the [cross-area exit
register](WORLD_CROSS_AREA_EXIT_REGISTER.csv).

### Checkpoint 294: Mountain Tracts connectivity and unresolved exits

Mountain Tracts has 245 rooms and 578 authored exits, with 572 resolving
during the reviewed load and 15 reciprocal active cross-area pairs. Six local
positive exits target absent rooms; a separate south exit at #20999 targets
−1. All 95 unregistered WLD files were parsed without room overlap; the only
three unregistered target matches are the existing Surf approaches to #20951,
#21110, and #20901, already retained on candidate row 97. Preserve these
sets separately. No route or candidate row is added; actual access and safe
return remain unqualified. See the [Mountain Tracts audit
review](MOUNTAIN_TRACTS_AUDIT_REVIEW.md#unregistered-wld-target-matches),
[coverage row 192](ZONE_COVERAGE_REGISTER.csv#L192), and [Surf candidate row
97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 295: Miaeril asymmetric Surface exits and stump route

Miaeril has 65 rooms and 190/190 resolving exits in one local weak component,
with two outbound exits to Surface #522257. The west-trail edge #33764 east
has the sole reciprocal return; village #33717 south is one-way. Authored
directed travel from either root reaches 63/65 rooms without the concealed
stump route and all rooms when its doors are assumed open; fall and current
conditions remain outside that static model. The all-95 inactive-WLD sweep
found no room overlap or approach, so source-only totals do not change. No
route is added. See the [Miaeril audit review](MIAERIL_AUDIT_REVIEW.md#active-connectivity-and-directed-source-reach),
[coverage row 193](ZONE_COVERAGE_REGISTER.csv#L193), and the [cross-area exit
register](WORLD_CROSS_AREA_EXIT_REGISTER.csv).

### Checkpoint 296: Lost Temple of the North active boundaries

The 20 active rooms have 47/47 resolving exits in three local weak components of 13, 4, and 3 rooms. Five active Surface boundary pairs are reciprocal: Temple #52901/#511126, #52914/#513537, #52917/#513539, #52918/#518740, and #52920/#517940. The shaft rooms #52902 and #52903 each have fall chance 100. A touch-configured type-25 object at #52904 points to #52918, linking the main cavern to the otherwise separate southern tunnel through item travel. The keyed #52913/#52904 gate is reset closed and locked, with asymmetric WLD key fields. These source declarations do not qualify access, traversal, fall safety, or return. No route or source-only candidate is added. See the [Temple graph and boundary review](MULTICLASS_TEMPLE_AUDIT_REVIEW.md#active-graph-and-surface-boundaries), [source dossier](../zone-stories/MULTICLASS_TEMPLE.md), [coverage row 194](ZONE_COVERAGE_REGISTER.csv#L194), and [cross-area exit rows 651–655](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L651).

### Checkpoint 297: Mushroom Forest graph and source-only entrance

The active mush1 map has 128 rooms and 1,001 authored exit declarations. The loader resolves 1,000; 999 edges are local, one is an active exit from #10100 east to UnderDark #856059, and #10178 northeast has a negative target -1 that is dropped. The weak graph is one component. Raw directed travel from #10100 reaches 125/128 rooms, with #10224, #10225, and #10227 outside forward reach; all 128 have a directed path back. The active UnderDark pair is reciprocal. The all-95 inactive-WLD sweep found no room overlap and one non-overlapping source-only ud_map2 approach, #1124761 north to #10100. It is not active ingress. Curated totals rise to 3,778 overall, 249 Surf, 19 ud_map2, and eight Underworld2. See the [Mushroom Forest graph review](MUSHROOM_FOREST_AUDIT_REVIEW.md#current-map-and-active-boundaries), [source-only room](../../../areas/wld/ud_map2.wld#L56451), [coverage row 195](ZONE_COVERAGE_REGISTER.csv#L195), [ud_map2 candidate row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114), and active [cross-area pair rows 308 and 721](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L308). Actual room admission, NPC dispersal, door use, and safe return remain unqualified.


### Checkpoint 298: Mushroom Caverns graph and inactive-WLD sweep

Mushroom Caverns has 132 rooms and 313/313 loader-resolving exits, two local weak components with a largest component of 118 rooms, five inbound and five outbound cross-zone exits with five reverse records, three fall rooms, and no currents. The all-95 inactive-WLD sweep found no room overlap and no exit into #24101–#24237, so no source-only approach or route candidate is added. Static graph data does not qualify actual access, portal choice, falling safety, or return. See the [Mushroom Caverns review](MUSHROOM_CAVERNS_AUDIT_REVIEW.md#static-map-and-inactive-wld-approach-scan), [source dossier](../zone-stories/MUSHROOM_CAVERNS.md), and [coverage row 196](ZONE_COVERAGE_REGISTER.csv#L196).


### Checkpoint 299: Myconid builder follow-ups on the existing graph

The existing CP147 graph review remains the source record: 93 rooms, 224/224 resolving exits, one local weak component, two reciprocal UnderDark pairs, and two inactive-WLD approaches to #2300. CP299 adds twenty builder questions about route intent, the two item switches, blocked-edge reset, access and return; it changes no exit declaration or candidate record. See the [builder questions](MYCONID_AUDIT_REVIEW.md#checkpoint-299-direct-builder-questions-and-recommendations), [coverage row 197](ZONE_COVERAGE_REGISTER.csv#L197), and [CP147 map review](MYCONID_AUDIT_REVIEW.md#active-map-and-reciprocal-underdark-boundaries).


### Checkpoint 300: Myrloch Vale route-intent follow-ups

The CP107 graph census remains 178 rooms and 373/373 resolving exits, two weak components of 165 and 13 rooms, and one reciprocal Moonshae boundary. The all-95 inactive-WLD sweep found no approach. CP300 adds twenty builder questions about access to the larger component, dynamic teleport routes, and the unstocked return stairway; it changes no exit or route record. See the [builder questions](MYRLOCH_VALE_AUDIT_REVIEW.md#checkpoint-300-twenty-direct-builder-questions-and-recommendations), [CP107 graph review](MYRLOCH_VALE_AUDIT_REVIEW.md#active-source-and-static-connectivity), and [coverage row 198](ZONE_COVERAGE_REGISTER.csv#L198).


### Checkpoint 301: Nax compiled routes and static graph

Nax retains 526/526 resolving authored exits, local weak components of 171 and 74 rooms, one active reciprocal Surface pair, eight current rooms, and one fall-chance room. The existing all-95 sweep found no room overlap and retains source-only Surf room #256920 west to Nax #37800. CP301 records the compiled chaos-port destination #37712 and builder questions about access to the detached component; it adds no active route or candidate. See the [Nax audit supplement](NAX_AUDIT_REVIEW.md#bounded-typed-references-and-compiled-bindings), [existing graph review](NAX_STORY_REVIEW.md#doors-and-static-connectivity), and [coverage row 199](ZONE_COVERAGE_REGISTER.csv#L199).


### Checkpoint 302: Negative Material Plane builder follow-ups

The existing graph remains 270 rooms and 999/999 resolving exits in five local weak components, with two one-way active inbound WLD edges, no outbound or reverse edge, two F100 rooms, and no currents. The inactive-WLD sweep found no overlap or source-only approach. CP302 adds twenty direct builder questions about separated planes, the one-way entries, dynamic travel, the vault, and falling hazards; no route is added. See the [builder questions](NEGATIVE_MATERIAL_PLANE_AUDIT_REVIEW.md#checkpoint-302-twenty-direct-builder-questions-and-recommendations), [CP94 graph](NEGATIVE_MATERIAL_PLANE_AUDIT_REVIEW.md#active-wld-graph), and [coverage row 200](ZONE_COVERAGE_REGISTER.csv#L200).


### Checkpoint 303: New Cave city route and builder follow-ups

New Cave city remains 73 rooms with 155/155 resolving exits, one local weak component, and three reciprocal active Surface/Underdark boundaries. Preserve the existing source-only Surf approach #277907 north to #15102 on candidate row 97. CP303 adds twenty builder questions about these routes, the disconnected map copies, mine access, and lake travel; no route or candidate changes. See the [builder questions](NEW_CAVECITY_AUDIT_REVIEW.md#checkpoint-303-twenty-direct-builder-questions-and-recommendations), [existing graph review](NEW_CAVECITY_AUDIT_REVIEW.md#active-map-and-directed-reach), and [coverage row 201](ZONE_COVERAGE_REGISTER.csv#L201).


### Checkpoint 304: Ailvio source graph and inactive approaches

Ailvio (newbie) has 117 rooms and 234/234 resolving local exits in five weak components (largest 109). Authored traversal from #29201 reaches and can return from 100 rooms; nine additional rooms in its weak component can return to the entry without a directed path from it. Doors, resets, installed access, and safe return remain unqualified. The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits without errors. It found three nonoverlapping source-only approaches from Dark Guildhall, Valhalla, and Genocide Guildhall #29200 north to #29201. The other 162 of 165 target matches begin in source rooms with reused VNUMs. No active edge, route, or candidate is added. See [Ailvio coverage row 202](ZONE_COVERAGE_REGISTER.csv#L202), [audit supplement](NEWBIE_AUDIT_REVIEW.md#active-graph-and-inactive-source-only-approaches), and its [twenty builder questions](NEWBIE_AUDIT_REVIEW.md#checkpoint-304-twenty-direct-builder-questions-and-recommendations).


### Checkpoint 305: Plains of Life scripted connection

Plains of Life (newbie2) has seven rooms and 12/12 resolving local exits; authored traversal from #22800 reaches and can return from every room. The all-95 inactive-WLD sweep found no room overlap or approach. Travel lies outside the WLD graph: stream object #22803 routes to Ailvio #29201 after the tutorial tag is absent, while active Ailvio archway #29236 has a type-25 destination of #22800 and a custom hometown procedure. Arrival, door/reset state, and effective return remain unqualified. See [coverage row 203](ZONE_COVERAGE_REGISTER.csv#L203), [Plains audit supplement](NEWBIE2_AUDIT_REVIEW.md#active-graph-and-source-only-wld-sweep), and [twenty builder questions](NEWBIE2_AUDIT_REVIEW.md#checkpoint-305-twenty-direct-builder-questions-and-recommendations).



### Checkpoint 306: Forest of Serenity graph and mirror boundary

Forest of Serenity (newfor) has 111 rooms and 299/299 loader-resolving exits: 298 local and one outbound to active Mist room #6366. There is no active inbound WLD edge or reverse exit. From #53300, authored exits reach and return from all 95 rooms in the main weak component. A detached 16-room Gnoll Wood component contains #53338, #53339, and #53397–#53410. Its reset mirrors form a reciprocal fixed type-25 route: object #53302 at #53377 targets #53338; object #53312 at #53338 targets #53377. Four zone reset rows close both directions of the two hut doorways, so live access remains unqualified.

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits without errors. It found no room-number overlap, target edge, or source-only approach into newfor. No route or candidate is added. See [coverage row 204](ZONE_COVERAGE_REGISTER.csv#L204), [audit supplement](NEWFOR_AUDIT_REVIEW.md#active-graph-and-inactive-wld-sweep), and [twenty builder questions](NEWFOR_AUDIT_REVIEW.md#checkpoint-306-twenty-direct-builder-questions-and-recommendations).



### Checkpoint 307: Newhaven WLD graph and source-only approach

Newhaven has 100 rooms and 217/217 resolving exits: 209 local and eight outbound. Its local weak components contain 99 and one room; #35272 is the singleton Goodie Continent distribution room. From #35200, authored local exits reach and return from 49 rooms. Fifty more rooms can return to #35200 but have no authored path from it. The active Surface/Newhaven boundary at #553247 north and #35200 south is the only reciprocal WLD pair. Seven further exits from #35272 lead one-way to loaded Surface rooms.

The all-95 inactive-WLD sweep found no room overlap and one non-overlapping source-only Surf approach: #221739 north to active Newhaven #35200. It is not active ingress. Preserve it on Surf candidate row 97. The reviewed Surf subset rises to 250 and the curated source-only total to 3,779; ud_map2 remains 19 and Underworld2 eight. No active route is added. See [Newhaven coverage row 205](ZONE_COVERAGE_REGISTER.csv#L205), [audit supplement](NEWHAVEN_AUDIT_REVIEW.md#directed-wld-graph-and-source-only-approach), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 308: New Hope reach/return and source-only confirmation

From entrance #89000, all 228 New Hope rooms are reachable and returnable by authored WLD exits. The map retains 550/550 resolving exits, 548 local and two reciprocal Surface boundaries. The all-95 inactive-WLD sweep recorded in the existing New Hope audit found no overlap or source-only approach; curated source-only totals remain 3,779 overall, 250 Surf, 19 ud_map2, and eight Underworld2. No route or candidate row changes. See [New Hope coverage row 206](ZONE_COVERAGE_REGISTER.csv#L206), [audit supplement](NEWHOPE_AUDIT_REVIEW.md#checkpoint-308-all-442-title-followed-history-and-graph-confirmation), and [New Hope dossier](../zone-stories/THE_VILLAGE_OF_NEW_HOPE.md).

### Checkpoint 309: Peril Peaks directed graph and inactive-WLD sweep

Peril Peaks has 196 rooms and 380 authored exits: 379 resolve and the single negative #57553 D8 target does not. The local weak components contain 188, 4, 2, 1, and 1 rooms. The reciprocal Mountain Tracks boundaries are #57536 down ↔ #21144 up and #57637 west ↔ #21015 east. Within Peril Peaks edges, #57536 reaches 66 rooms and 95 can return to it; #57637 reaches 37 and 52 can return. All 95 inactive-WLD files parsed without issues and had no room overlap or source-only approach into the area. No active route or candidate is added; curated source-only totals remain 3,779 overall, 250 Surf, 19 ud_map2, and eight Underworld2. See [Nexus coverage row 207](ZONE_COVERAGE_REGISTER.csv#L207), [audit supplement](NEXUS_AUDIT_REVIEW.md#active-boundaries-and-directed-wld-graph), and [the reciprocal boundary rows](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L809).


### Checkpoint 310: Reliquary Nexus of Aggression graph and inactive-WLD sweep

Nexus IC has 100 rooms and 258/258 resolving WLD exits: 254 local and four reciprocal Surface boundaries (#130100/#511534, #130112/#511135, #130124/#511536, and #130136/#511935). Its two local weak components contain 76 and 24 rooms. The all-95 inactive-WLD sweep traversed 527,791 rooms and 2,080,773 exits; six extra empty terminator lines in unrelated inactive waterplane rooms #5–#10 were skipped. It found 400 targets into #130100–#130199, all from Duris3: 224 source VNUMs are owned by active areas and excluded, while 176 are source-only (100 south from #129800–#129899, one east from #130099, and 75 north from #130425–#130499). The Duris3 #130200 west-to-#130199 record starts at VNUM #130200, which is owned by active nexus_uc, so it is excluded from source-only ingress. Preserve the 176 on Duris3 candidate row 36; no active route is added. Curated source-only totals rise to 3,955 overall; Surf remains 250, ud_map2 19, and Underworld2 eight. See [Nexus IC coverage row 208](ZONE_COVERAGE_REGISTER.csv#L208), [audit supplement](NEXUS_IC_AUDIT_REVIEW.md#active-graph-and-reciprocal-surface-boundaries), and [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).


### Checkpoint 311: Reliquary Nexus of the Corsairs graph and inactive-WLD sweep

Nexus Island has 100 rooms and 277 authored exits: 275 resolve, with two dropped D5 targets of -1 at #130342 and #130348. Its 274 resolving local exits form one 100-room weak component; the one outbound crossing is paired with Surface’s inbound crossing at #519153/#130301. From inbound room #130301, all 100 rooms are reachable and all 100 can return in raw WLD traversal, ignoring doors and fall outcomes. Thirty-nine rooms have fall-chance markers from 30 to 100; there are no currents. The all-95 inactive-WLD sweep traversed 527,791 rooms and 2,080,773 exits, with six extra empty terminators skipped in unrelated waterplane rooms #5–#10. It found 121 non-overlapping Duris3 source-only exits (100 north from #130600–#130699, one east from #130599, and 20 south from #130080–#130099); 279 other matches begin at active-owned VNUMs. No active route or candidate count changes. Curated source-only totals rise to 4,076 overall; Surf remains 250, ud_map2 19, and Underworld2 eight. See [Nexus Island coverage row 209](ZONE_COVERAGE_REGISTER.csv#L209), [audit supplement](NEXUS_ISLAND_AUDIT_REVIEW.md#active-wld-graph-and-surface-boundary), and the [reciprocal Surface rows](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1295).

### Checkpoint 312: Nexus Upper City graph, object routes, and source-only approaches

Active `nexus_uc` (#130200–#130284) has 85 rooms and 177/177 resolving WLD exits: 176 local and one reciprocal Surface boundary through #577404. The WLD-only graph has eight weak components (39/10/10/10/4/4/4/4); raw reach and return from #130200 each cover four rooms. Fourteen reset `ITEM_TELEPORT` destinations form seven paired object routes and conditionally connect all 85 rooms. A second, code-driven entrance comes from the shared object #468, reset at Heavens #23 and active rooms in Tharnadia, Kimordril, Khildarak, and Shady; its special proc sends levels 10–30 to #130200 outside the WLD exit register. Live reset state, door availability, and safety remain unqualified.

The all-95 inactive-WLD scan traversed 527,791 room records and 2,080,773 D0–D9 direction records with no structural parse issue. It found 340 references to #130200–#130284, all in `Duris3.wld`: 171 source-only (85 south from #129900–#129984, one west from #130285, and 85 north from #130500–#130584) plus 169 active-owned starts (168 same-numbered `nexus_uc` records and one `nexus_ic` source #130199). No diagonal record targets this range. The six standalone tilde lines in unrelated waterplane rooms #5–#10 remain a scanner caveat and do not affect these targets. Add the 171 non-overlapping records to existing candidate row 36; its 26 active-reference count stays unchanged. See [Nexus UC audit supplement](NEXUS_UC_AUDIT_REVIEW.md#inactive-wld-sweep-and-duris3-source-only-references), [Surface boundary](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1294), and [coverage row 210](ZONE_COVERAGE_REGISTER.csv#L211).
### Checkpoint 313: Nhavan Island reach/return and inactive-WLD sweep

Nhavan Island has 23 rooms and 53 authored exits: 52 resolve within the area, while #20301 east to #148782 is the only unresolved target. On resolved local WLD edges, all 23 rooms are reachable from and can return to #20300. Three reciprocal gate pairs reset closed. There is no active external WLD ingress or other loaded-zone exit. The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 direction records without issues and found no target into #20300–#20322 or inactive room identity in that range. The #148782 sea target exists only in Duris3 and is pruned by the full loader. Preserve it on existing Duris3 candidate row 36; its 26 active-reference count and the curated source-only total of 4,247 remain unchanged. See [Nhavan coverage row 211](ZONE_COVERAGE_REGISTER.csv#L212), [audit supplement](NHAVAN_ISLAND_ONE_AUDIT_REVIEW.md#directed-wld-graph-and-active-routes), and [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

### Checkpoint 314: Northern Wilderness directed graph and inactive-WLD sweep

Northern Wilderness has 149 rooms and 348/348 resolving exits: 347 local and one one-way boundary from #12269 north to Centaur #93300. Its weak components have 112, 26, and 11 rooms. From #12200, 101 rooms are reachable and 112 can return; an 11-room chasm pocket has a single exit south into the main network and no authored local entry from it. The 11-room quarry and 26-room river/farm valley are each internally reachable and returnable from #12256 and #12350. There are two F10 fall markers and no currents. The all-95 inactive-WLD sweep found no room overlap or approach into Northern Wilderness. Centaur #93300 has a reciprocal boundary to Surface #519684, which returns to Surface rather than #12269. Preserve this one-way route and the chasm's text-only references pending builder review. See [coverage row 215](ZONE_COVERAGE_REGISTER.csv#L216), [audit supplement](NORTHERN_WILDERNESS_AUDIT_REVIEW.md#directed-wld-graph-and-active-routes), and [active cross-area record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L218).

### Checkpoint 315: Nyneth directed graph and inactive approach

Active `nyneth` has 78 rooms and 175/175 resolving exits: 174 local, one south from #22900 to Surface #581417. The Surface north exit reciprocates; White Harbor #55633 has a second inbound down to #22994 with no local return and is a technical teleport-room lead. The WLD graph has weak components of 67 and 11 rooms. From #22930, 65 rooms are reachable and 66 can return; the entrance component is internally reachable and returnable from #22900. The reset type-25 portal #22902 joins these components conditionally, and #22903/#22904 lead to separate sequel zones; none is a WLD edge. Across all 95 inactive WLD files, 527,791 rooms and 2,080,773 exits parsed without issues. There is no room-VNUM overlap and one source-only exit: `surf.wld` #284533 north to #22900. It is not active ingress. Preserve it on the existing Surf candidate row; curated source-only totals become 4,248 overall and 251 Surf. See [coverage row 216](ZONE_COVERAGE_REGISTER.csv#L217), [Nyneth route and portal review](NYNETH_AUDIT_REVIEW.md#active-wld-graph-and-approaches), [active Surface pair](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L379), [technical White Harbor inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L793), and [source-only Surf approach](../../../areas/wld/surf.wld#L1773829).

### Checkpoint 316: Olympus directed graph and inactive target

Active `olympus` has 10 rooms and 19 authored exits: 18 resolve locally, while #99809 west to #218064 is the sole positive-unresolved target. On local edges, all 10 rooms are reachable and returnable from #99800 in one weak component. The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 directions without issues and found no overlap or exit into Olympus. No active cross-area boundary is registered. The unresolved VNUM exists in inactive `surf` as “The Vast Bay of Tharnadia” and `surface2011` as “The Azure Waters of a Stormy Sea”; neither source is active, and no direct return to Olympus is declared. Preserve the one target record on both existing candidates. See [coverage row 221](ZONE_COVERAGE_REGISTER.csv#L222), [audit review](OLYMPUS_AUDIT_REVIEW.md#active-wld-graph-and-unresolved-west-exit), and [unresolved-exit row 318](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L318).


### Checkpoint 317: Otower graph and inactive approach

Active otower has 51 rooms and 104 authored exit records; all resolve at load. There are 102 local edges and two outbound boundaries. The raw local graph is one directed component: from #11300 all 51 rooms are reachable and all can return. No target is negative or positive-unresolved, and there are no fall-chance or current fields. This static graph does not prove that reset-closed or locked doors are passable without keys.

The active reciprocal boundaries are #11300 south to Surface #616249 and #11348 north to Underworld3 #30172, with reverse records at both destinations. See the [Otower-to-Surface row 207](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L207), [Otower-to-Underworld3 row 208](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L208), [Underworld3 reverse row 420](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L420), and [Surface reverse row 1611](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1611).

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 directions without parse issues, found no room overlap, and found one source-only approach: surf.wld #225677 north to #11300 (“A Grassy Clearing”). It is not active ingress. Preserve it on Surf row 97. The reviewed Surf subset rises from 251 to 252 and the curated total rises from 4,248 to 4,249 overall; ud_map2 remains 19 and Underworld2 eight. See [source room](../../../areas/wld/surf.wld#L538621), [coverage row 225](ZONE_COVERAGE_REGISTER.csv#L226), and [Otower audit](OTOWER_AUDIT_REVIEW.md#active-graph-door-resets-and-source-only-approach).

### Checkpoint 318: Patrols active graph and inactive approaches

Active Patrols has seven rooms and 28 authored exits. None resolves locally or in the active loader; the seven-room local graph is seven singleton components, with no active inbound or outbound boundary. All 28 exits have positive targets recorded in the unresolved-exit register at lines 151–178. Each target occurs in inactive surf and surface2011 sources, and three also occur in underdark2011. Their differing room titles leave map identity unresolved.

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits without parse issues. It found seven room-VNUM overlaps, all in fists_gh.wld, and 14 raw exits from that source into Patrols VNUMs. Twelve originate in the overlapping room IDs. The two exits from unique source rooms are #30507 east to #30506 and #30509 up to #30503; preserve them on existing candidate row 42 as source-only identity-ambiguous records, not active ingress. The curated source-only total is 4,251: Surf 252, ud_map2 19, Underworld2 eight. See [Patrols audit](PATROLS_AUDIT_REVIEW.md#active-graph-unresolved-targets-and-source-only-approaches), [candidate register](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L42), and [coverage row 228](ZONE_COVERAGE_REGISTER.csv#L228).

### Checkpoint 319: Pharr Valley graph and inactive-map review

The current WLD, area manifests, and inactive WLD inputs match the source snapshot used for Checkpoint 71. The active graph remains 189 rooms and 408 authored exits: 406 internal and two outbound, all loader-resolved, in one weak component. Reciprocal boundary pairs remain Pharr Valley #40300 west to Valley Crushk #2006 and Pharr Valley #40389 east to Pods #28500; the corresponding return exits remain present. Two rooms have fall-chance values and there are no current fields.

The prior all-95 inactive-WLD scan remains applicable: it found no room-VNUM overlap and no inactive source exit targeting Pharr Valley. This checkpoint adds no source-only ingress. The current curated source-only total remains 4,251 overall (Surf 252, ud_map2 19, Underworld2 eight). This graph does not prove ordinary access, hazard safety, ship travel, or dynamic behavior. See [Pharr Valley audit](PHARR_VALLEY_AUDIT_REVIEW.md#active-connectivity-and-inactive-wld-scan), [cross-area records](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L551), and [coverage row 229](ZONE_COVERAGE_REGISTER.csv#L229).

### Checkpoint 320: Pine Hollow graph and reachability

The WLD has 166 rooms and 437/437 resolved authored exits: 429 internal and eight outbound. Three active inbound records target #16008, #16097, and #16065; eight outbound records leave Pine Hollow, of which two have reciprocal exits. The 161-room main local component is raw-directed reachable and returnable from each active inbound anchor.

The five-room quest-mob staging group #16161–#16165 has no local edge to the main component and no active WLD inbound. Its six one-way outbound records originate at load rooms #16162–#16164. The roaming source mobiles load at #16161 and can enter terminal room #16165, which has no outgoing exit and cannot return to the staging root. Record this as a source-supported mobile-availability risk, not a claim that current deployed mobiles are stranded. Door resets, the conditional `guild_guard` room keys, hazards, and player access remain outside raw-graph proof. The previous all-95 inactive-WLD sweep found no room overlap or exit into Pine Hollow; no source-only ingress is added. Curated source-only total remains 4,251 overall, including Surf 252, ud_map2 19, and Underworld2 eight. See [Pine Hollow review](PINE_HOLLOW_AUDIT_REVIEW.md#active-graph-and-raw-directed-reachability) and [coverage row 230](ZONE_COVERAGE_REGISTER.csv#L230).

### Checkpoint 321: Plane of Air graph and source-only approaches

Plane of Air has 125 rooms and 749/749 loader-resolved exits: 747 local and two outbound. The weak graph has one 125-room component, but its directed strongly connected components are 122, 1, 1, and 1. Rooms #24418, #24450, and #24518 have no incoming local WLD edge; each can follow exits to the 122-room core. The Yan-C-Bin reset at #24450 is one of these one-way rooms. All nine active inbound records target the core. Five directions from Prison #7490 target #24439; the other inbound sources are Rftjngle #80444 south to #24501, Alatorin #83346 down to #24460, Bahamut #25700 west to #24460, and Airp #131600 down to #24422. The two outbound records reciprocate to Bahamut and Airp. All authored door flags and key VNUMs are zero; there are no negative/unresolved targets, fall markers, or currents.

The all-95 inactive-WLD sweep found ten south exits from `waterplane.wld` into Air. Five start in rooms whose VNUMs are already owned by active Limbo/Heavens and remain identity-overlap records. Five start at unique source VNUMs (#2, #6–#9) and are source-only approaches, not active ingress. The same source contributes five further unique-source exits into Heavens (#6–#9). Preserve all ten unique-source records on existing Water Plane candidate row 120; its active-inbound count stays zero. Heavens' coverage count now includes these five alongside its existing `surf` and `surface2011` references. Curated source-only totals rise by ten to 4,261 overall; Surf remains 252, ud_map2 19, and Underworld2 eight.

The inactive source-only records do not capture dynamic routes: Gate, Nether Gate, and Plane Shift select destinations through the Air zone helper, the Air grey pool targets Ethereal #12451, and the paired type-25 portals connect Air #24405 with Smoke #139832. These code/object routes are outside the WLD exit count and do not prove ordinary access, safe arrival, or return. See the [Plane of Air review](PLANE_OF_AIR_AUDIT_REVIEW.md#active-graph-and-raw-directed-reachability), [Water Plane candidate row 120](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L120), and [coverage row 231](ZONE_COVERAGE_REGISTER.csv#L231).

### Checkpoint 322: Plane of Earth graph and external routes

Plane of Earth has 129 rooms and 763/763 loader-resolved exits: 760 local and three outbound. Its weak graph has one component, while the directed graph has a 121-room strongly connected core and eight singleton components. Four ordinary map rooms (#23818, #23845, #23850, #23918) have no local inbound and point toward the core; all four active inbound targets lie in that core. The technical tail #23926–#23929 is present in the loaded zone beyond `IS_EARTH`'s #23925 bound; #23926 has no local WLD inbound and #23927 is terminal. No negative/unresolved exits, fall markers, or currents occur; all exit door flags and key VNUMs are zero.

The eight active inbound records are five directions from Prison #7494 to #23905, Rftjngle #80444 west to #23805, Alatorin #83344 down to #23860, and reciprocal Earthp #131200 up to #23908. The one reciprocal pair is #23908 down ↔ #131200 up. The other two outbounds are technical links #23926 up and #23928 south to Earthp #131364; neither has a reverse record. Preserve the five Prison directions as one source-room group. See [cross-area rows 169–173](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L169), [386–388](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L386), [976](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L976), [1024](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1024), and [1299](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1299).

The all-95 inactive-WLD scan found no approach or room-ID overlap into #23801–#23929. There are no source-only counts or candidate changes. Code and object paths remain separate: Negative Plane type-25 fissure #26660 targets #23833; Plane of Earth pool #23804 uses `magic_pool` to send to Ethereal #12492 with a 200-damage field and one-hit-point floor; Gate, Nether Gate, and Plane Shift use #23801 as a sampled zone anchor. These routes do not establish ordinary WLD ingress, safe arrival, or return. See the [Plane of Earth review](PLANE_OF_EARTH_AUDIT_REVIEW.md#active-graph-and-raw-directed-reachability) and [coverage row 232](ZONE_COVERAGE_REGISTER.csv#L232).

### Checkpoint 323: Plane of Fire graph and external routes

Plane of Fire has 125 rooms and 750/750 loader-resolved exits: 748 local and two outbound. Its weak graph is one 125-room component. Raw directed traversal from the union of active WLD arrival targets reaches 122 rooms; #25418, Imix's reset room #25450, and #25518 have no local inbound exit. All authored door flags and key VNUMs are zero; there are no negative/unresolved targets, fall markers, or currents.

The nine active inbound records are five directions from Prison #7484 to #25504, Rftjngle #80444 north to #25405, Alatorin #83347 down to #25460, Firep #88301 west to #25415, and Brass #139000 south to #25455. The five Prison directions share one source room. The two outbound records are Fire #25415 west to Firep #88321 and #25455 north to Brass #139000. Only the Brass edge reciprocates at the same endpoint; Firep's inbound and outbound use different Firep rooms. See [cross-area rows 144–148](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L144), [398–399](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L398), [973](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L973), [1027](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1027), [1096](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1096), and [1313](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1313).

The all-95 inactive-WLD scan found two south exits to Fire #25455. `brass-old-1` room #139100 reuses an active Brass VNUM with different room text, so keep that edge identity-ambiguous on candidate row 17. `brass-old-3` room #25201 is one unique-source approach on candidate row 19. Neither is active ingress; only the unique-source edge raises the curated total by one to 4,262. Code and object paths are separate: a sampled planar spell anchor, the Fire pool to Ethereal #12459 with its 200-damage/one-hit-point-floor behavior, paired Fire/Smoke portals, a Negative Plane fissure to #25433, and active Ardgral/Ethereal pool arrivals to #25410. Astral #19711 has a code binding but no current reset. These paths do not establish ordinary WLD access, safe arrival, or return. See the [Plane of Fire review](PLANE_OF_FIRE_AUDIT_REVIEW.md#code-and-object-routes), [candidate row 17](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17), [candidate row 19](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L19), and [Fire coverage entry (CSV line 233)](ZONE_COVERAGE_REGISTER.csv#L233).

### Checkpoint 324: Plane of Water graph and external routes

Plane of Water has 125 rooms and 750/750 loader-resolved exits: 749 local and one outbound. Its weak and strongly connected graphs each contain all 125 rooms, and the union of active WLD arrival targets reaches all 125. The eight inbound records are five directions from Prison #7479 to #23255, Rftjngle #80444 east to #23250, Alatorin #83345 down to #23260, and Sea Kingdom #31500 up to #23281. The only outbound is #23281 down to Sea Kingdom #31500, a reciprocal same-endpoint pair. See [cross-area rows 139–143](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L139), [380](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L380), [436](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L436), [974](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L974), and [1025](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1025).

The all-95 inactive-WLD sweep found no source-room overlap or exit into #23201–#23325. No source-only count or candidate row changes; the existing `waterplane` candidate remains on row 120 for its previously recorded Air/Heavens references. Separate code/object paths include Gate, Nether Gate, and Plane Shift's sampled Water anchor; the local 200-damage pool to Ethereal #12492; Negative Plane lake #26658 to #23233; Ardgral #30407 and Ethereal #12476 pool arrivals at #23324; and Orb of the Sea's conditional random-room relocation. These do not establish safe arrival or return. See the [Plane of Water review](PLANE_OF_WATER_AUDIT_REVIEW.md#code-and-object-routes), [waterplane candidate row 120](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L120), and [Water coverage entry (CSV line 234)](ZONE_COVERAGE_REGISTER.csv#L234).

### Checkpoint 325: Player Castles graph and inactive-WLD sweep

Active `player_castles` has three rooms and three authored exits. Two exits
resolve internally between #11000 and #11001; #11000 D2/south targets #-1.
The raw local graph has two weak components (two rooms and one room): the
foyer and testing room connect reciprocally, while shop room #11005 is
isolated. There are no active WLD inbounds or outbounds. The all-95
inactive-WLD scan found no source-room overlap or target into #11000–#11005.
Add no cross-area exit or source-only approach, and keep candidate
active-inbound counts and curated source-only WLD totals unchanged at 4,262
overall (Surf 252, ud_map2 19, Underworld2 eight). Code-based house/guild
door behavior is documented separately from this WLD graph. See the
[Player Castles review](PLAYER_CASTLES_AUDIT_REVIEW.md#active-graph-and-directed-reachability),
[coverage row 235](ZONE_COVERAGE_REGISTER.csv#L235), and
[source dossier](../zone-stories/PLAYER_CASTLES.md).

### Checkpoint 326: Port Skythic active graph and inactive-WLD sweep

Active `PortSkythic` has 110 rooms and 218/218 loader-resolved exits, all
internal to its own WLD. There are no active cross-area edges, negative
targets, positive unresolved targets, fall-chance rooms, or current fields.
The raw undirected graph has three components of 103, six, and one room; each
component is strongly connected when all its authored exits are traversable.
The 103-room component contains the city and tower, piers, and gatehouse; the
six-room component is the sunken ship (#138101–#138106); #138107, the
space-time rift, has no exit. No active WLD edge connects the ship or rift to
the city. This source graph does not establish live access, door state, or
safe return.

The six local ZON D commands reset the paired Raging Boar/storage-room and
Dancing Sun/lounge/brothel passages; the file comments describe the first two
as closed and the latter four as closed/secret. Seven WLD exit records have a
nonzero flag field, including flag-4 #138045 south to #138044, which has no
matching D reset. Preserve the exact source state until the area owner
confirms the intended behavior.

The all-95 inactive-WLD scan found 440 `Duris3.wld` destination matches into
#138000–#138109: 224 distinct approaches from 222 non-overlapping source
rooms, plus 216 edges originating in the same-numbered `The Ocean` source
rooms. The latter are identity overlaps and are excluded from the curated
source-only approach count. Keep the 224 on existing Duris3 candidate row 36;
they are not active ingress. Curated source-only totals rise to 4,486 overall
(Surf 252, ud_map2 19, Underworld2 eight). See the
[Port Skythic review](PORTSKYTHIC_AUDIT_REVIEW.md#inactive-wld-approaches-and-overlapping-room-identities),
[coverage row 237](ZONE_COVERAGE_REGISTER.csv#L237), and
[Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

### Checkpoint 327: Potions graph and inactive-WLD sweep

Active `potions` owns 15 rooms and 86 authored exits. The current loader
resolves 80 records: 73 local edges and seven one-way edges to Ceothia room
#81094. Six Brass-branch edges from #30705 and #30706 target #25087 in
inactive `brass-old-2.wld`. The seven-room Brass cluster and eight-room
Ceothia cluster are weak components; every room in each cluster is directed
reachable from its named start, while all 15 rooms are singleton strongly
connected components. There is no active WLD inbound to Potions or reverse
edge from Ceothia. The all-95 inactive-WLD scan found no room overlap or
inactive exit into Potions. Add no source-only approach; curated source-only
counts remain 4,486 overall (Surf 252, ud_map2 19, Underworld2 eight). These
source records do not establish live access or safe return. See the [Potions
audit review](POTIONS_AUDIT_REVIEW.md#active-graph-and-authored-access),
[coverage row 238](ZONE_COVERAGE_REGISTER.csv#L238), and [source
dossier](../zone-stories/POTIONS.md).

### Checkpoint 328: Pragtog graph and inactive-WLD sweep

Active pragtog has 94 rooms and 198 authored exits; all 198 resolve in the
active loader. Its two inbound and two outbound cross-area exits form
reciprocal pairs: #10649 up to Underdark #822668 and #10693 north to Black
Tunnels #10700. The local WLD graph has weak components of 79 and 15 rooms
and strong components of 56, 23, and 15. Raw WLD reach from either arrival
room reaches the same 56-room set. The 23-room #10650-#10672 set has a
one-way WLD exit from #10666 to #10673; the 15-room set is WLD-isolated.

Three reset ITEM_TELEPORT objects provide authored item-triggered routes
across these WLD regions: #10614 in #10618 to #10619 on hug, #10615 back to
#10618 on kill, and #10645 from #10648 to #10650 on enter. The fifth local
type-25 object #10659 moves within the 56-room set; #10647 has no local reset.
The ZON resets 20 doors, including closed, locked, and secret states.
Conservative initial-open traversal reaches three rooms from #10649 and two
from #10693; this ignores opening doors, keys, secret discovery, and item
triggers. These source measures do not establish player accessibility or
safe return.

The all-95 inactive-WLD scan found no room overlap and exactly two distinct
source-only exits into #10649: ud_map1 room #1115029 up and ud_map2 room
#1126541 up. Preserve them on existing candidate rows 113 and 114. They do
not add active ingress or change the candidates' one active reference from
Shafts. Curated source-only totals rise by two to 4,488 overall (Surf 252,
ud_map1 two, ud_map2 20, Underworld2 eight). No fall markers, currents,
negative exits, unresolved targets, or room-range mismatch exist. See the
[Pragtog audit review](PRAGTOG_AUDIT_REVIEW.md#active-graph-and-authored-access),
[coverage row 239](ZONE_COVERAGE_REGISTER.csv#L239), and [source dossier](../zone-stories/PRAGTOG.md).


## Checkpoint 329: Prisons of Carthapia reset-aware reach and boundaries

The active Prison graph has 199 rooms and 455/455 resolved exits. Its local
weak components contain 166, 17, and 16 rooms. The reciprocal Surface pair
is #636262 north to Prison #7497 and #7497 south back to #636262; 35 other
active exits leave Prison one way for seven zones. Two rooms declare falls
(F60 and F80); no currents, negative exits, unresolved targets, or source
range mismatches were found.

Raw directed local WLD reach from the Surface arrival #7497 is 166/199.
Applying the current 76 door-reset declarations and treating closed,
locked, secret, and blocked exits as unavailable reduces strict initial-open
reach to five rooms: #7497, #7496, #7300, #7301, and #7302. The first main
gate, #7300 north to #7303, resets closed and locked and names key #7300;
the Minotaur carries that key in #7300. This static traversal does not model
key use, secret discovery, switches, fixed portal objects, the Nexus
callback, or live world state.

The inactive surf.wld approach #263479 north to #7497 remains source-only
and is already documented on Surf candidate row 97. It does not add active
ingress or change Surf's 145 active references. Keep source reachability
separate from played access, safety, and return. See the [Prison audit
review](PRISONS_OF_CARTHAPIA_AUDIT_REVIEW.md#checkpoint-329-reset-aware-access-and-builder-decisions),
[coverage line 240](ZONE_COVERAGE_REGISTER.csv#L240), and [source
dossier](../zone-stories/PRISONS_OF_CARTHAPIA.md).

### Checkpoint 330: Prison of Fort Boyard door-state reach

Prisonb has 52 rooms, 108/108 resolved exits, and weak components of 23, 22,
and 7 rooms. Its two reciprocal Fortb arrivals are #38149 east to #43000 and
#38121 east to #43041. Raw directed local reach from those entries is 22/52
and 7/52; strict initial-open reach after applying 44 current D resets is
7/52 and 4/52. The dynamic ship landing at #43020 has raw reach 23/52 and
strict initial-open reach 10/52. The reset states comprise 20 closed, 20
closed/locked, two open, and two blocked exits. No D reset sets a secret
flag.

The strict graph treats closed, locked, and blocked exits as unavailable.
It does not model key use, the two ITEM_SWITCH levers, the ship's ENTER and
JUMP routes, reset timing, or the live state of shared doors. The entrance
from #43004 east to #43005 is closed/locked and the entrance key #43000 is
reset on the guard in #43004. The ship is an external Fortb reset at #38106;
its separate return plank is reset at #43022. The full 95-source inactive
WLD scan found no source-room overlap or approach into Prisonb. These source
counts do not establish played accessibility or safe travel. See the
[Prisonb audit review](PRISON_OF_FORT_BOYARD_AUDIT_REVIEW.md#checkpoint-330-reset-aware-arrival-reach-and-builder-decisions),
[coverage line 241](ZONE_COVERAGE_REGISTER.csv#L241), and [dossier](../zone-stories/PRISON_OF_FORT_BOYARD.md).

### Checkpoint 331: Purple Worm arrival reach and reset doors

The Lair has 98 rooms, 216/216 resolved exits, and one 98-room weak
component. Four reciprocal active boundaries enter three distinct rooms:
Underworld #4618 to #42588; Underdark #814263 to #42583; and Underdark
#832712 and #833111 to #42500. Raw directed local reach from each entry is
95/98. Strict initial-open reach after applying the six ZON D resets is
92/98 from #42588 and #42583, but only 1/98 from #42500.

All six D resets set closed/unlocked doors. The #42500 north boulder exit is
closed at reset, so a traversal starting in that entry room reaches no
further local room until the door is opened. From the other two roots,
closed directions at #42511 and #42564 are excluded. All three raw directed
traversals miss loader rooms #42594–#42596 despite the single weak component;
this is a graph result and does not establish runtime placement or mobile
availability. The three ud_map2 approaches stay source-only on its existing
candidate row 114 and do not change its one active Shafts reference. Preserve
the four reciprocal active boundaries and Alatorin's nine reset rows for
local mobile #42501. See the [Purple Worm audit review](LAIR_OF_THE_PURPLE_WORM_AUDIT_REVIEW.md#checkpoint-331-reset-aware-arrival-reach-and-builder-decisions),
[coverage line 242](ZONE_COVERAGE_REGISTER.csv#L242), and [dossier](../zone-stories/LAIR_OF_THE_PURPLE_WORM.md).

### Checkpoint 332: Neverwind Valley reset-aware arrival reach

Neverwind Valley has 199 rooms and 555/555 resolved exits in one weak component. Raw directed reach is 192/199 from each distinct entry root: active WLD arrivals #20400 and #20466, plus the computed Githzerai `shift_prime` arrival #20484. The seven rooms outside those raw walks are #20592–#20598, including the staged elf and Goar reset rooms.

All 14 local D resets match door-capable WLD exits. Eight set closed/unlocked state 1, one sets closed/locked state 2, and five set closed/secret state 5. After applying those reset states and excluding closed, locked, and secret exits, directed reach is 91/199 from each root. This static traversal does not simulate SAY DUNAN, OPEN, keys, secret discovery, reset timing, mobile wandering, or the racial spell's actual arrival. The source declares six fall rooms (two F25 stairs and four F10 treetops) and no currents. These counts do not prove live accessibility or safe movement.

Preserve the three reciprocal Surface boundaries and the incoming-only Connector Zones edge to #20466. Surf candidate row 97 already records the source-only `surf.wld` #235767 north exit to #20400; it stays source-only and does not change active ingress or candidate counts. See the [Neverwind audit](NEVERWIND_VALLEY_AUDIT_REVIEW.md#checkpoint-332-reset-aware-access-and-builder-decisions), [coverage row 243](ZONE_COVERAGE_REGISTER.csv#L243), and [source dossier](../zone-stories/NEVERWIND_VALLEY.md).

### Checkpoint 333: Quietus Quay reset-aware connectivity

Quietus Quay owns 95 rooms and 216 exits: 214 resolve (211 local and three outbound) while two exits from unused #1794 target -1. The local graph is one weak component. Four active entries reach the same raw directed set, 91/95 rooms. After the 38 D resets apply, strict reach is 5/95 from Underdark root #1700 and 60/95 from each of Surface #1723, Heavens #1736, and Surface #1737. Resets set 35 closed/unlocked exits, one closed/locked, and two closed/secret.

Three outbounds have reciprocal UnderDark/Surface returns; Heavens #31 north to #1736 is incoming-only. The all-95 inactive-WLD scan found no Quietus room overlap or approach. Ferry stop #1712 and the `chaos_port qq` arrival at #1711 are separate code routes, not WLD edges. These static records do not prove live access, mobile availability, or safe return. See the [Quietus audit](QUIETUS_QUAY_AUDIT_REVIEW.md#checkpoint-333-builder-questions-and-recommended-resolutions), [coverage row 244](ZONE_COVERAGE_REGISTER.csv#L244), and [source dossier](../zone-stories/QUIETUS_QUAY.md).

### Checkpoint 334: Rabble reciprocal boundary and six-room reach

Rabble has six rooms and 11/11 resolving exits: ten local exits and one outward edge from #65100 up to Heavens #1234. The six-room local graph is one weak component, and directed raw reach from the only active arrival, Heavens #1234 down to #65100, is 6/6. No local D reset, fall marker, or current changes the strict traversal, so strict post-reset reach is also 6/6. The boundary is reciprocal. The all-95 inactive-WLD scan found no Rabble room overlap or source-only approach; no candidate or curated source-only count changes. These static records do not qualify live actor presence or shop behavior. See the [Rabble audit](RABBLE_AUDIT_REVIEW.md#checkpoint-334-builder-questions-and-recommended-resolutions), [coverage row 245](ZONE_COVERAGE_REGISTER.csv#L245), and [setting dossier](../zone-stories/RABBLE.md).

### Checkpoint 335: Ogre Raiders reset-aware entry and fence

Ogre Raiders has 21 rooms and 59/59 resolving exits: 58 local and one outbound to Surface. The local weak graph is one component of 21 rooms. The reciprocal active boundary connects Surface #593506 and Raiders #8900. Raw directed reach from the active Surface arrival is 21/21; strict post-reset reach is 20/21 after two D resets close the fence between #8910 and corral #8920. Both fence doors are source-marked closed and unlocked. No other D reset, fall marker, or current affects the static walk.

The all-95 inactive-WLD scan found one non-overlapping source-only Surf approach: #277145 north to active Raiders #8900. It is not active ingress. Preserve it on [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); the 145 active references remain unchanged. The reviewed Surf subset rises from 252 to 253 and the curated source-only total from 4,488 to 4,489. See the [Raiders audit](RAIDERS_AUDIT_REVIEW.md#checkpoint-335-builder-questions-and-recommended-resolutions), [coverage row 246](ZONE_COVERAGE_REGISTER.csv#L246), and [setting dossier](../zone-stories/RAIDERS.md).

### Checkpoint 336: Castle Ravenloft reset-aware arrivals

Castle Ravenloft has 269 rooms and 689/689 resolving exits (686 local, three outbound) in five weak components of 223, 43, 1, 1, and 1 rooms. Its six active inbound roots each reach 140/269 rooms through raw directed WLD exits. After applying 110 current D reset states (32 open, 46 closed/unlocked, 14 closed/locked, four closed/secret, 14 open/blocked), strict reach is 14/269 from #58568, 1/269 from #58567, 14/269 from #58538, 3/269 from #58542, 14/269 from #58305, and 5/269 from #58369. Three outbound edges are reciprocal. Four fall rooms are present and no currents are declared. The strict walk is a static baseline that does not execute puzzle controls, type-25 travel, or falling movement.

The all-95 inactive-WLD scan found no room overlap or source-only approach into Ravenloft. The curated source-only total stays 4,489 (Surf 253, ud_map1 two, ud_map2 20, Underworld2 eight). See the [Ravenloft audit](RAVENLOFT_AUDIT_REVIEW.md#checkpoint-336-castle-ravenloft-reset-aware-arrivals), [coverage row 248](ZONE_COVERAGE_REGISTER.csv#L248), and [source dossier](../zone-stories/CASTLE_RAVENLOFT.md).

### Checkpoint 337: Ravenloft 2 reset-aware access

The Ravenloft Catacombs has 400 rooms and 961/961 resolving exits (952 local, nine outbound) in six weak components sized 392, 4, 1, 1, 1, and 1. Its two Castle Ravenloft arrival roots reach 83 and 383 rooms raw. After applying 194 D resets across 192 distinct room/direction pairs, strict post-reset reach is 47/400 from #59057 and 1/400 from #59352. Reset states include 103 closed/locked, 73 open/blocked, and two duplicate state-5 door directions (#59145 west and #59149 east). The area declares 80 fall-marked rooms and no currents; static reach does not qualify puzzle interaction, travel, or safe movement.

The two reciprocal Castle edges are #58538 ↔ #59057 and #58542 ↔ #59352. Seven other outbounds lead to Barovia, Barovia2, or Castle Ravenloft without reciprocal WLD edges. All 97 non-active WLD files present under `areas/wld/` were scanned; none overlaps a Catacombs room or approaches it. No candidate or curated source-only count changes. See the [Ravenloft 2 audit](RAVENLOFT2_AUDIT_REVIEW.md#checkpoint-337-ravenloft-2-reset-aware-access-and-builder-decisions), [coverage row 249](ZONE_COVERAGE_REGISTER.csv#L249), and [source dossier](../zone-stories/THE_RAVENLOFT_CATACOMBS.md).

### Checkpoint 338: Raxthan reset-aware access

Raxthan has 100 rooms and 227/227 resolving exits (224 local, three outbound) in one weak component. All three inbound WLD edges have reciprocal returns. Both Surface arrivals at #42980 reach 100/100 rooms raw and 90/100 in the strict post-reset graph; the Underdark arrival at #42999 reaches 100/100 raw and 94/100 strict. Four D resets set three closed/unlocked exits and one open/blocked exit; there are no duplicate room/direction resets. Seven fall chances and no currents are declared, so neither graph reach nor the F fields qualify safe movement.

The all-97 inactive-WLD scan found no Raxthan room overlap and one source-only `surf.wld` exit from #266966 north to #42980. This edge was already counted at Checkpoint 68; preserve it on [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97) as alternate-map evidence, not active ingress. Surf's 145 active references, its reviewed source-only subset of 253, and the current curated source-only total of 4,489 remain unchanged. See the [Yerdonia audit](YERDONIA_AUDIT_REVIEW.md#checkpoint-338-raxthan-reset-aware-access-and-builder-decisions), [coverage row 250](ZONE_COVERAGE_REGISTER.csv#L250), and [story dossier](../zone-stories/DRUSTLS_YERDONIA_ENSLAVED.md).

### Checkpoint 339: Faerie Realm reset-aware access

The Realm has 211 rooms and 468 source exits: 435 local and 23 resolved
cross-area exits, plus eight dropped negative targets and two positive
unresolved targets. The only active arrival is Surface #531694 east to #14002,
with Realm #14002 west returning. Raw local reach is 196/211 from #14002;
after applying the 26 chance-100 D resets and treating every closed, locked,
secret, or blocked door as unavailable, strict baseline reach is 6/211. The raw
weak components have sizes 206, 3, 1, and 1. This static baseline does not
simulate opening or searching for doors.

Twenty-two resolved one-way exits leave dispersal rooms #14208–#14210.
Realm #14090 east to #164929 in unregistered Duris3 and #14210 southeast to
#25087 in unregistered brass-old-2 remain the two positive unresolved edges.
The current 446-file WLD inventory has 351 active and 95 non-active stems. Its
non-active sweep found no Realm room overlap and one source-only Surf edge:
#244463 south to #14002. Preserve that route on Surf candidate row 97; it is
not active ingress. Curated source-only totals rise to 4,490 overall and 254
for Surf. See the [Realm audit](FAERIE_REALM_AUDIT_REVIEW.md),
[coverage row 251](ZONE_COVERAGE_REGISTER.csv#L251), and
[source dossier](../zone-stories/FAERIE_REALM.md).

### Checkpoint 340: Rift Valley Jungle reset-aware connectivity

Rift Valley Jungle has 470 rooms and 1,229 source exits: 1,219 local, nine
resolved outbound, and one positive unresolved target. The loader resolves
1,228 exits. Five local weak components contain 466 rooms and four singletons.
All four active arrivals are reciprocal: Woodseer #16950/#16951/#16952 east
to #80000/#80001/#80002, and Surface #568516 west to #80469. See the
[Woodseer boundary](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L298),
[Jungle departures](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L969), and
[Surface return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1481).

The other five resolved departures have no active reverse record: #80398 down
to Pine Hollow #16008, and #80444 north/east/south/west to Fire #25405, Water
#23250, Air #24501, and Earth #23805. The latter four exits start at the
elemental ranger's load room; a wander path for that mobile does not establish
ordinary or safe player travel.

Each of the four active roots reaches 419/470 rooms by ordinary directed WLD
edges. After the 42 chance-100, unconditional D resets are applied, a strict
walk treating closed, locked, and secret doors as unavailable reaches 385/470.
The resets set 26 closed/unlocked, eight closed/locked, and eight
closed/secret doors. This door snapshot does not model opening, searching,
object teleports, or live movement. Four declared paired ITEM_TELEPORT routes
connect local elemental-cage rooms; including all eight source-declared portal
edges yields a 423/470 raw and 389/470 strict static overlay. Neither count
qualifies availability or survival. The area also declares 44 fall rooms and
27 rooms with currents; route safety remains unqualified.

Room #80460 south to #228371 is the single positive unresolved exit. The
target exists only in unregistered Surf and Surface2011 sources. Inactive
surf.wld #228371 north returns to #80460, but its map is not loaded;
surface2011.wld uses the same number for a different room and has no such
return. Preserve the ambiguity on
[unresolved exit row 263](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L263).

The current 446-file inventory has 351 active and 95 non-active WLD stems.
The all-current inactive-WLD sweep found no room overlap and two source-only
Surf approaches: #226279 south to #80469 and #228371 north to #80460. Neither
is active ingress. Preserve them on existing Surf candidate row 97; its
reviewed source-only subset is now 256 and curated source-only totals are 4,492
overall (Surf 256, ud_map1 two, ud_map2 20, Underworld2 eight). See the
[Rift Valley Jungle audit](RFTJNGLE_AUDIT_REVIEW.md),
[coverage row 252](ZONE_COVERAGE_REGISTER.csv#L252), and
[source dossier](../zone-stories/RFTJNGLE.md).

### Checkpoint 341: River Styx static connectivity

River Styx has 100 rooms and 194/194 resolving WLD exits: 193 local and one
outbound. Its weak components are sized 96, 2, 1, and 1. There are no active
inbound edges. From dock #90700, the ordinary directed graph reaches 96/100
rooms; all 96 can route back to the dock. After applying the six chance-100 D
reset states and treating closed, locked, and secret doors as unavailable,
strict reach is 92/100.

The single cross-area edge is dock #90700 south to Hell One #51001, with no
direct reverse. The local graph also has one unpaired north self-exit from
#90740 back to itself. Room #90796 says swamp waterways lie south but has only
one WLD exit, north to #90795. The exact boundary remains in
[cross-area exit row 1115](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1115) and
[pair row 410](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L410).

The type-25 objects connect the fountain hall #90761 to rooms #90762–#90764
and return, with #90764 also linked to #90780. These dynamic routes are outside
the WLD-only counts. Seventy-one rooms declare currents and no rooms declare
fall chances; currents can affect actual movement in water, so the static
walk does not qualify a safe route.

The all-445-other-WLD sweep (350 other active sources and 95 non-active files)
found no room overlap or approach into River Styx. No external active or
source-only fixed or random-zone type-25 declaration targets one of its rooms.
The curated source-only total remains 4,492 overall (Surf 256, ud_map1 two,
ud_map2 20, Underworld2 eight). See the [River Styx audit](RIVER_STYX_AUDIT_REVIEW.md),
[coverage row 253](ZONE_COVERAGE_REGISTER.csv#L253), and
[source dossier](../zone-stories/RIVER_STYX.md).

### Checkpoint 342: Rogue Plains reset-aware connectivity

Rogue Plains has 158 rooms and 382 WLD exit records: 379 local, two resolved
cross-zone, and one unresolved. Both resolved exits begin at #75801 and are
reciprocated by Surface: east to #541822 / west back, and south to #542221 /
north back. The third boundary is #75895 north to #170047, which exists only
in inactive Duris3.wld and is pruned by the active loader.

The directed local graph has weak components sized 136, 21, and one. From
arrival room #75801 it reaches 123/158 rooms, all of which have an authored
return walk. Twelve chance-100 D resets apply closed or closed/secret states
to six door pairs; strict WLD reach and return after those states is 96/158.
The source has no fall chances or current fields.

Windtalker loads at #75951, which has no incoming active, inactive, or local
WLD edge; its north and east exits lead to #75930 and #75952. Seven rooms
#75952–#75957 and #75959 are titled “Unnamed” and have no outbound exits;
#75955 is isolated. These records need builder decisions and are not safe
route claims.

The full 95-file inactive-WLD sweep found no Rogue Plains room-number
overlap. It found one non-overlapping source-only Surf approach: #201299
north to #75801. Preserve it on existing Surf candidate row 97; it is not
loaded ingress. Curated source-only totals rise by one to 4,493 overall
(Surf 257, ud_map1 two, ud_map2 20, Underworld2 eight). See the
[Rogue Plains audit](ROGUE_PLAINS_AUDIT_REVIEW.md),
[coverage row 254](ZONE_COVERAGE_REGISTER.csv#L254),
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and
[Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

### Checkpoint 343: Tunnels of Payang reset-aware reachability

Tunnels of Payang has 87 rooms and 172 exits: 170 local and two outbound Surface exits, all loader-resolved, in one 87-room weak component. Active arrivals are Heavens #30 north to #90346, Surface #574593 down to #90376, and Surface #576997 west to #90380. The latter two have reciprocal RTUN exits; the Heavens edge has no direct reverse record. See [RTUN audit](RTUN_AUDIT_REVIEW.md#active-connectivity-and-reset-aware-reach), [coverage row 255](ZONE_COVERAGE_REGISTER.csv#L255), and cross-area registers #10, #1112, #1496, #1504.

From each arrival, 83/87 rooms are mutually reachable in the raw directed local graph. Six 100% reset-controlled door pairs change that result: #90346 and #90376 each have 53 rooms mutually reachable with their arrival, while #90380 reaches only itself while the tombstone door to #90341 remains closed. This treats state 1 as closed and state 5 as closed/secret; live unlocks and traversal were not tested. Rooms #90381–#90384 remain unreachable even in the raw local graph. Rooms #90326 and #90327 each have a 30% fall chance; no room has a current field.

The complete 95-file inactive-WLD sweep found no room-number overlap and two non-overlapping source-only Surf approaches: #263253 west to #90380 and #270444 down to #90376. Preserve these on existing Surf candidate row 97; they are not active ingress. The reviewed Surf source-only subset is now 259, and curated source-only total is 4,495 (Surf 259, ud_map1 two, ud_map2 20, Underworld2 eight); Surf active references remain 145. See [RTUN audit](RTUN_AUDIT_REVIEW.md#inactive-map-approaches).

### Checkpoint 344: Village of Refugees reset-aware connectivity

Village of Refugees has 67 rooms and 142 exits: 140 local and two outbound Surface exits, all resolved in one weak component. Both active arrivals enter room #98600 and have reciprocal returns, north to #627239 and east to #627640 ([cross-area records](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1178)). From #98600, all 67 rooms are mutually reachable in the raw directed local graph.

Six 100% door pairs affect reset-state reach: #98607/#98610 closed/blocked (state 9), #98609/#98629 closed/secret (state 5), #98614/#98615 open/secret (state 4), #98624/#98628 closed (state 1), #98626/#98627 closed/locked (state 2), and #98665/#98666 closed (state 1). Excluding states 1, 2, 5, and 9 leaves 45/67 rooms mutually reachable with #98600. Rooms #98610, #98627–#98646, and #98666 are outside that conservative route set. Eight rooms have 10% fall chance: #98600–#98602, #98613, and #98629–#98632; there are no current fields. See [Village of Refugees audit](VILLAGE_OF_REFUGEES_AUDIT_REVIEW.md#checkpoint-344-reset-aware-reach-refreshed-source-closure-and-builder-decisions).

The full inactive-WLD sweep adds no new edge in this checkpoint: the already-counted non-overlapping surf.wld #264438 west to #98600 remains source-only, not active ingress. Curated source-only totals stay 4,495 overall (Surf 259, ud_map1 two, ud_map2 20, Underworld2 eight); Surf active references stay 145.

### Checkpoint 345: Sarmiz'Duul reset-aware connectivity

Sarmiz'Duul has 583 rooms and 1,389 exits: 1,386 local and three outbound,
all loader-resolved in one weak component. Three ordinary reciprocal routes
connect Surface #621875 to #9400, Underworld #4404 to #9853, and Surface
#622675 to #9975. Separate one-way inbounds are Storm Port #22541 down to
#9597 and Winterhaven load room #55402 west to #9401; see the [boundary
register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L187), [pair rows 414–415,
455, and 809](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L414), and [coverage row
257](ZONE_COVERAGE_REGISTER.csv#L257).

From each of the five arrival rooms, all 583 rooms are reachable and have a
return walk in the raw directed local graph. Sarmiz has 50 100%-chance door
reset records: 43 state-1, four state-2, and three state-5. Excluding closed,
closed/locked, and closed/secret exit directions leaves mutual reach and
return of 249/583 from #9400, 6/583 from #9853, 18/583 from #9975, and
249/583 from each one-way inbound room. The 50 records cover 25 reciprocal
pairs; the #9733/#9899 pair has asymmetric state 5/state 1. No room declares
a fall chance or current field. These are conservative source graph counts,
not live door or travel results. See the [Sarmiz audit](SARMIZ_AUDIT_REVIEW.md#active-connectivity-and-reset-aware-reach).

The all-95 inactive-WLD sweep found no room-number overlap and three
non-overlapping, source-only Surf approaches: #277517 south to #9400, #278416
east to #9975, and #278717 north to #9975. Preserve them on existing Surf
candidate row 97; they are not active ingress. The reviewed Surf subset rises
to 262 and curated source-only totals rise to 4,498 (Surf 262, ud_map1 two,
ud_map2 20, Underworld2 eight); Surf's 145 active references remain
unchanged. See the [Sarmiz inactive-map review](SARMIZ_AUDIT_REVIEW.md#inactive-surface-approaches).

### Checkpoint 346: Savannah reset-aware connectivity and source identity

Savannah has 167 rooms and 450 exits: 449 local and one reciprocal outbound
boundary, Surface #562072 east to #138584. All exits resolve in one weak
component. From #138584 the raw graph reaches and returns from all 167 rooms;
excluding directions with 100%-chance closed/secret door resets leaves 165
reachable and 164 with return. Six state-5 resets form three closed, key-zero
secret pairs (#138580/#138585, #138619/#138665, #138623/#138629). Four rooms
declare falls and none declares currents. These are static counts, not live
travel results; see the [Savannah audit](SAVANNAH_AUDIT_REVIEW.md#active-connectivity-and-reset-aware-access)
and [coverage row 258](ZONE_COVERAGE_REGISTER.csv#L258).

The inactive-WLD sweep found all 167 Savannah room VNUMs duplicated in
`Duris3.wld`, with different map identities. Its 668 edges into those target
numbers also resolve to VNUMs defined in that inactive file, so they are not
uniquely attributable approaches into Savannah. No other inactive WLD has an
edge into the Savannah range. Curated source-only totals stay at 4,498 (Surf
262); Surf's 145 active references remain unchanged. Preserve the numeric
identity finding on existing Duris3 candidate row 36, not as Savannah or Surf
ingress. See the [source identity review](SAVANNAH_AUDIT_REVIEW.md#inactive-wld-room-number-overlap)
and [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

### Checkpoint 347: Scorched Valley reset-aware connectivity and Surf approach

Scorched Valley has 132 rooms and 354 resolved exits: 350 local and four
outbound. The local map has four weak components: one of 129 rooms and three
single-room load destinations (#71326, #71330, #71331). Its only ordinary
reciprocal boundary is Surface #576166 ↔ #71201. The other three inbounds from
Winterhaven, Fields Between, and Juiblex are one-way helper loads; the three
outbounds lead to Mount, Torg, and Fields Between helper locations.

Raw reach/return from #71201 is 88/88; the inbound load roots #71221 and #71265
each reach/return 88/88, and #71321 reaches/returns 37/37. Excluding all
directions covered by the 14 100%-chance door resets leaves 3/3 from #71201,
66/66 from #71221 and #71265, and 37/37 from #71321. The door resets form
seven reciprocal pairs: four state-2 locked, two state-1 closed, and one
state-5 secret pair. No fall or current fields are declared. These are static
source graph counts; see the [Scorched Valley audit](SCORCHVALLEY_AUDIT_REVIEW.md#active-connectivity-and-reset-aware-reach)
and [coverage row 259](ZONE_COVERAGE_REGISTER.csv#L259).

The inactive-WLD sweep adds one non-overlapping source-only `surf.wld` exit:
#280633 west to #71201. Preserve it on existing Surf candidate row 97; it is
not active ingress. Surf's reviewed source-only subset rises to 263 and
curated global totals rise to 4,499 (Surf 263, `ud_map1` two, `ud_map2` 20,
Underworld2 eight). Surf's 145 active references remain unchanged. See the
[inactive-map review](SCORCHVALLEY_AUDIT_REVIEW.md#inactive-wld-approach) and
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 348: Sea reset-aware connectivity and inactive-map identity

Active Sea has 270 rooms and 719 exit records: 716 resolve within Sea and
three leave room #67568 for #219820 east, #220119 south, and #219818 west.
None resolves to an active WLD room, and no other active area has an inbound
exit to Sea. Each target VNUM appears in both unregistered `surf.wld` and
`surface2011.wld`, whose same-numbered Storm Sea records have different titles
and exits from Surf's foothills/waterfall records.

The all-95 inactive-WLD sweep parsed without errors, found no Sea source-room
overlap, and found three edges into Sea, all in `surf.wld`: #219818 east,
#219820 west, and #220119 north to #67568. They are source-only returns and
were already included in the existing Surf subset; retain Surf at 263 and the
global curated total at 4,499. The equal-numbered `surface2011.wld` records
do not return to Sea and remain identity-ambiguous with Surf; they do not add
approaches. See the [Sea audit](SEA_AUDIT_REVIEW.md#active-connectivity-and-reset-aware-reach)
and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97) plus
[Surface2011 candidate row 98](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L98).

The local weak graph has components 269 and 1 (isolated stable #67540).
Directed reach from #67301, #67305, and #67568 is 243 rooms from each using
all authored local exits. Suppressing all 38 reset-controlled door directions
reduces those reaches to 1, 210, and 1 respectively. Each flagged direction
has a matching 100%-chance reset: 27 state-1 closed, nine state-5
closed/secret, and two state-6 closed/locked/secret. These are static
source-graph counts; the roots are comparison points, not active arrivals,
and actual keys, specials, travel, and safe return were not tested. The WLD
also records currents in 58 rooms and one fall field at #67501.

### Checkpoint 349: Sea Caves reset-aware connectivity and inactive-map identity

The active scan parsed all 351 manifest WLD contributors and found 95
seacaves rooms, 193/193 resolved exits, 192 local exits, and one reciprocal
Surface boundary: Surface #558717 north to #2200, paired with seacaves #2200
south to #558717. The local weak graph is one component of 95 rooms. With all
local edges available, #2200 reaches all 95 rooms and all 95 can reach back.
Suppressing the 25 nonzero WLD door directions leaves 51 reachable from and
51 able to return to #2200. These are static graph comparisons; reset,
door, travel, and safety behavior were not played.

The 26 D resets all have 100% chance and match each nonzero WLD door
direction by room and direction. Their states are 20 state 1, four state 5,
one state 8 “open/blocked,” and one state 0 “open”; the state-0 reset is on
an unflagged direction. There are no fall or current fields in the source.
The narrow cliff-ledge warning remains prose, not a fall mechanic.
Temple room #2262's east exit is targeted by mural object #2208, an
ITEM_SWITCH configured for PUSH. Its generic procedure clears the blocked
state from both directions and sends a message that the mural moves aside,
but it does not remove the switch object. The filtered reach count treats
this like every other reset-controlled direction; actual use and passage were
not tested. See the [Sea Caves audit](SEACAVES_AUDIT_REVIEW.md#typed-dependencies-and-compiled-bindings).

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits with no
parse errors or source-room overlap. It found one source-only Surf approach:
unregistered surf.wld #212716 west to active seacaves #2200. Preserve it on
existing candidate row 97 as source-only, not active ingress. Surf's 145
active references remain unchanged; its reviewed source-only subset rises to
264 and the curated global total to 4,500. See the [Sea Caves audit](SEACAVES_AUDIT_REVIEW.md#inactive-wld-identity-and-source-only-approach)
and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 350: Sea Kingdom reset-aware connectivity and inactive-map identity

The active scan parsed all 351 manifest WLD contributors. Sea Kingdom has 225
rooms and 558/558 resolved exits: 557 local and one outbound. Its five local
weak components contain 117, 65, 22, 16, and 5 rooms. The reciprocal Plane
Water boundary is #31500 up to #23281 and back down to #31500. White Harbor
#55634 southeast enters #31668 without a reverse Sea Kingdom edge; its
“gnomish key” source title does not establish player access.

All 20 local D resets have 100% chance and match 20 of the 24 local exits with
nonzero exit-info. Their authored state values are 1×5, 2×4, 4×3, 6×2, 8×5,
and 12×1. Four remaining directions form two pairs: #31595 down/#31596 up
and #31607 up/#31608 down. Each has raw value 4 (EX_LOCKED alone), no
EX_ISDOOR bit, and no matching D reset. Generic key/lock and bash paths
require EX_ISDOOR, so these entries need builder intent review. See [exit flag
definitions](../../../src/core/defines.h#L442), [lock handling](../../../src/cmd/actnew.c#L3145),
and [bash handling](../../../src/classes/innates.c#L2970).

| Comparison root | Raw directed reach / return | Suppressing every nonzero exit-info direction |
| --- | ---: | ---: |
| #31500 | 113 / 23 | 1 / 1 |
| #31501 | 63 / 43 | 3 / 3 |
| #31503 | 113 / 23 | 99 / 22 |
| #31504 | 113 / 23 | 99 / 22 |
| #31505 | 113 / 23 | 99 / 22 |
| #31668, active White Harbor arrival | 63 / 43 | 1 / 3 |

Filtered traversal is a conservative static comparison, not actual locked,
secret, switch, portal, or travel behavior. Four fall markers occur on the
living mast rooms #31714–#31717: 40 at #31714/#31716 and 45 at
#31715/#31717. The zone has no current flags. See the
[Sea Kingdom audit](SEAKNGDM_AUDIT_REVIEW.md#checkpoint-350-static-reconciliation-and-builder-decisions)
and [coverage row 262](ZONE_COVERAGE_REGISTER.csv#L262).

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits with no
parse errors, Sea Kingdom room-number overlap, or inactive approach. Surf
remains at 264 reviewed source-only records, and the curated total remains
4,500 overall.

### Checkpoint 351: Sevenoaks reset-aware connectivity and inactive-map identity

The active `seveno` graph contains 77 rooms and 222/222 loader-resolved WLD
exits: 221 local exits and one outbound edge. Two active WLD edges enter the
area, one with a reciprocal return. The local graph has seven weak components
of 31/14/9/8/5/5/5 rooms and no unresolved positive or negative target.

| Boundary | Active WLD edge | Reverse edge |
| --- | --- | --- |
| Surface | #516252 north → #38600 | #38600 south → #516252 |
| White Harbor | #55632 northeast → #38632 | None; arrival is in the vault |

Thirty-four nonzero exit-info directions match 34 D resets, all at 100%
chance. Raw D states are 1×6, 2×9, 6×18 and 14×1. From Surface root
#38600 the raw local graph has 31 reachable and 31 returnable rooms; from
White Harbor arrival #38632 it also has 31/31. Conservatively suppressing all
nonzero-flag directions gives 3/3 and 1/1 respectively. This comparison is
not a runtime door, portal, access or safety result. Five fall-chance rooms
and two current-marked rooms are present in the source census. The paired
exit and reset table, including the EX_CLOSED-only #38669/#38670 pair, is in
the [Sevenoaks audit](SEVENO_AUDIT_REVIEW.md#checkpoint-351-sevenoaks-static-reconciliation-and-builder-decisions).

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits with
no parse errors or Sevenoaks room-number overlap. It found two non-overlapping
source-only approaches into active room #38600: `surf.wld` #209270 north and
`underworld2.wld` #425645 west. These are not active ingress. Preserve them on
Surf candidate row 97 and Underworld2 row 116; no new candidate row is added.
Surf's reviewed source-only subset rises to 265, Underworld2 to nine, and the
curated total to 4,502 overall. See [coverage row 263](ZONE_COVERAGE_REGISTER.csv#L263).

### Checkpoint 352: Arbre's Forest reset-aware connectivity and inactive-map identity

The active `sforest` WLD has 24 rooms and 48/48 loader-resolved exits. Two
exits leave the forest for Surface and both have reverse Surface exits; the
other 46 edges remain local. All rooms form one 24-room directed component,
with 24 reachable and 24 returnable rooms from #97400. There are no flagged
exits, D resets, unresolved targets, fall values or current values.

| Boundary | Forest exit | Surface reverse |
| --- | --- | --- |
| Northern path | #97400 north → #559312 | #559312 south → #97400 |
| Southern village approach | #97423 south → #560911 | #560911 north → #97423 |

All 95 inactive WLD files parsed without errors (527,791 rooms and 2,080,773
exits); none overlaps SForest room VNUMs. Two distinct inactive `surf.wld`
exits enter the forest: #208170 south to #97400 and #209066 east to #97423.
They are not active ingress. Preserve both on existing Surf candidate row 97;
its reviewed source-only subset increases from 265 to 267 and curated totals
from 4,502 to 4,504. Surf's 145 active references remain unchanged. See the
[Arbre's Forest audit](SFOREST_AUDIT_REVIEW.md#checkpoint-352-arbres-forest-reconciliation-and-builder-decisions)
and [coverage row 264](ZONE_COVERAGE_REGISTER.csv#L264).

### Checkpoint 353: The Great Shaboath reset-aware connectivity and inactive-map identity

The active Shaboath WLD contains 116 rooms and 243/243 loader-resolved exits. Its weak components contain 115 rooms plus isolated Control Room #32800; the 115-room inhabited portion is one strongly connected directed component. There are no negative or positive-unresolved targets, room-range mismatches, or current fields. One room carries a fall-chance field.

| Active boundary | Counterpart | Classification |
| --- | --- | --- |
| ConnectorZones #54012 north → Shaboath #32803 | No Shaboath reverse edge | Inbound only ([register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L704)) |
| Winterhaven #55633 southeast → Shaboath #32916 | No Shaboath reverse edge | Inbound only ([register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L797)) |
| Underdark #710327 north → Shaboath #32919; Shaboath #32919 south → #710327 | Both directions are authored | Reciprocal ([Underdark inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1690), [Shaboath outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L439)) |

All 64 nonzero-flag exit directions match D resets at 100% chance. Reset-state values are 1×54, 2×7, 6×2, and 13×1. Raw authored reachability and returnability are 115/115 from each boundary room (#32803, #32916, #32919). Suppressing every flagged direction as a conservative static comparison leaves 6/6, 1/1, and 6/6 respectively. This does not establish live reset state, key/access availability, safety, or actual travel.

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits without errors. It found neither an inactive source-room VNUM overlap nor an inactive approach into Shaboath. The current curated source-only total remains 4,504, including 267 reviewed Surf records and 9 Underworld2 records. Shaboath adds no candidate entry. See the [Great Shaboath audit](GREAT_SHABOATH_AUDIT_REVIEW.md#checkpoint-353-refreshed-world-history-and-typed-dependency-reconciliation) and [coverage row 265](ZONE_COVERAGE_REGISTER.csv#L265).

### Checkpoint 354: Shadamehr Keep reset-aware connectivity and inactive-map identity

Shadamehr Keep has 25 rooms and 57/57 resolved exits in one strongly connected directed component. Its single external pair is reciprocal with Dreggan Woods: #41186 north -> #41200 and #41200 south -> #41186. There are no negative/unresolved targets, range mismatches, fall fields, or current fields.

Six flagged directions form three paired doors: #41207/#41218 uses key #41201, #41219/#41222 uses key #41202, and #41222/#41223 uses imported key #41003. All six flagged directions match local D resets at 100% chance, state 2. From entrance #41200, raw reachability and returnability are 25/25; suppressing all flagged directions leaves 19/19. This static comparison does not establish live door state, key supply, safe route, or travel.

All 95 inactive WLD sources were searched for exact room headers in #41200-#41224 and exit destination records into that range; no overlap or source-only approach was found. The curated source-only total remains 4,504 (Surf 267, Underworld2 9). No candidate register row is added. See the [Shadamehr Keep audit](SHADAMEHR_KEEP_AUDIT_REVIEW.md#active-graph-and-door-resets) and [coverage row 266](ZONE_COVERAGE_REGISTER.csv#L266).

### Checkpoint 355: Shadowclave connectivity, scripted arrival, and inactive-map identity

The active Shadowclave WLD has 53 rooms and 108 authored exits; 107 resolve and one negative target remains at #93918 east -> -1. All rooms form one strongly connected 53-room component when authored exits are traversable. Three distinct reciprocal pairs connect to Surface: north and east from #93900 to #545307/#545708, and up from #93952 to #544905. There are no range mismatches, fall fields, or current fields.

Twenty flagged directions form ten paired closed passages and match 20 state-1 D resets, all at 100% chance. From #93900, raw reach/return is 53/53 and suppressing every flagged edge leaves 5/5; from #93952 it leaves 20/20. This comparison is static. Separately, compiled do_shift_prime can randomly place an eligible Githzerai at pond room #93919 when shifting back from the Astral; this is a conditional code route, not a WLD edge.

The all-95 inactive-WLD target scan found no room-number overlap and two source-only Surf approaches: #215655 west to #93900 and #212935 down to #93952. These are not active ingress; retain both on existing Surf candidate row 97. Surf's reviewed source-only subset rises from 267 to 269 and the curated total from 4,504 to 4,506; its 145 active references and Underworld2's nine reviewed entries are unchanged. See the [Shadowclave audit](SHADOWCL_AUDIT_REVIEW.md#inactive-map-approaches) and [coverage row 267](ZONE_COVERAGE_REGISTER.csv#L267).

### Checkpoint 356: Shady Grove builder-review reconciliation

The static connectivity findings remain those recorded at checkpoint 98; no exit, reachability, or candidate-ingress finding changed. This pass verifies the three source-backed builder questions and recommendations for active Surface approaches, the separate `shady2.mob` source identity, and Heavens/pet-shop route qualification. Their recommendations preserve current graph evidence without promoting inactive approaches or implying a played safe return. See [Shady Grove audit supplement](SHADY_GROVE_AUDIT_REVIEW.md#checkpoint-356-builder-question-coverage-reconciliation).

### Checkpoint 357: Underworld Shafts connectivity and inactive-map boundaries

The 43-room zone has 84 authored exits: 74 local targets, six active cross-zone exits, and four positive unresolved exits. There are six reciprocal active boundary pairs (Underworld 4580/10000, MountainTracks 20934/10003 and 20921/10035, Underdark 844848/10021, Underworld 4585/10033, and Ghore 11548/10043). The six local directed components are strongly connected internally, sized 13, 9, 8, 5, 4, and 4. Active entry roots cover and return through three of them, 22 rooms total; the other 21 rooms sit in three components without active cross-zone ingress. All exit flags and keys are zero.

Four active exits target unregistered ud_map rooms; three have direct reverse edges only in inactive sources, while the 10012-to-1121446 edge has no direct reverse in the current source scan. Ten additional exits from inactive ud_map0/1/2 rooms point into active Shafts. No inactive room-header VNUM overlaps the zone. Preserve the inactive directions as source-only and do not infer live ingress.

Static hazards include 25 F fall markers and eight C current markers on waterfall rooms 10013-10020. This census does not qualify live safety or dynamic return. See [Shafts audit supplement](SHAFTS_AUDIT_REVIEW.md), the [active cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L190), and the [unresolved-exit register](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L17).

### Checkpoint 358: Shaughin builder-question review

Checkpoint 131 already recorded the 49-room graph, 45-room and 4-room components, four reciprocal boundary records, and the limits of the selected 149-room travel scenario. Checkpoint 358 adds no graph changes; it verifies 20 source-backed builder questions about paired receipts, hazardous/portal routes, lower-map ownership, and incidental services. Those questions do not make assumed travel playable or safe. See [Shaughin audit supplement](SHAUGHIN_AUDIT_REVIEW.md).

### Checkpoint 359: Shaughin2 Hunting Grounds connectivity

The Shaughin2 WLD has 100 rooms and 212 exits: 208 local destinations and four reciprocal active boundaries. Its raw local directed graph is one strongly connected component; each active entry room (#6686, #6695, #6696, #6699) reaches and returns from all 100 rooms. This excludes zone door resets, current/water behavior, teleport admission, and live safety. There are no unresolved or negative WLD destinations, out-of-range rooms, or range-mismatch exits.

The four pairs are upper Shaughin #6543/#6699, #6546/#6686, and #6547/#6695, plus Underdark #790149/#6696. The #6695 waterfall room has the zone's sole current marker (100 north); there are no fall markers. Two 100%-chance D resets at #6696 west and #6697 east apply state 5 to the local secret passage. Do not infer its reset state from raw-WLD reachability.

The all-95 inactive-WLD scan found no room-header overlap and one source-only Surf approach, #230408 east to #6683. Preserve it on existing Surf candidate row 97; it is not active ingress. See the [Shaughin2 audit](SHAUGHIN2_AUDIT_REVIEW.md), the [cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L118), and the [coverage row](ZONE_COVERAGE_REGISTER.csv#L271).

### Checkpoint 360: Ship runtime pool and fixed ferry rooms

The active `ship.wld` has 5,000 room records (#60000-#64999) and zero authored exit directions. Its raw local graph is 5,000 singleton components, largest size one; there are no authored fall or current markers. The first three rooms are code-reserved as Davy Jones' Locker (#60000), ship transit (#60001), and undead ferry room (#60002). The remaining 4,997 slots are dynamically claimed for ship interiors. Native code builds and removes exits from saved ship-room layouts, so the isolated static WLD census does not describe a live interior or qualify travel.

Active WLD scans and an all-95 inactive-WLD sweep found no room overlap or target into the ship range. No active boundary, inactive approach, or source-only candidate is added. The curated source-only total remains 4,517. See the [ship runtime-area audit](SHIP_AREA_AUDIT_REVIEW.md), [coverage row 272](ZONE_COVERAGE_REGISTER.csv#L272), and [runtime-room dossier](../zone-stories/SHIP_RUNTIME_AREA.md).

### Checkpoint 361: Ship Yards static connectivity and inactive-map identity

The active `shipy.wld` has 229 rooms and 606 exits: 570 local, 32 resolved cross-area exits, and four positive targets in unregistered WLD sources. Ten active exits enter through seven rooms (nine Surface directions and one Malch direction); 32 leave, with ten reciprocal boundary records. The local weak graph has twelve components sized 86, 64, 59, 10, 3, and seven singletons. Seventy flagged directions match 70 100%-chance D resets (68 state 1, one state 2, one state 5). Static reach/return and door-state limits are documented in the [Ship Yards audit](SHIP_YARDS_AUDIT_REVIEW.md).

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits without errors. Inactive `aopal.wld` shares 28 room headers with `shipy`; four more Aopal exits from non-overlapping rooms (#43329, #43330, #43334, #43342) target Ship Yards. Eight non-overlapping Surf exits also target the port. Preserve those twelve source-only directions on existing candidate rows 10 and 97; they are not active ingress. Surf's reviewed subset becomes 278 and the curated source-only total 4,529. See the [Aopal candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L10), [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [coverage row 273](ZONE_COVERAGE_REGISTER.csv#L273).

### Checkpoint 362: The Undead Fog connectivity and inactive approaches

The shortb WLD has 13 rooms and 87 authored directions: 85 local and two reciprocal Surface boundary directions. Every exit flag and key is zero; there are no door resets, fall/current markers, or unresolved targets. Its raw local weak graph has a twelve-room component plus isolated Web room #67809. Each Surface entry (#67801, #67810) reaches and can be returned to from all twelve fog rooms. Two type-25 objects connect the isolated Web and fog in source data; they are not WLD edges or live-travel qualification.

The all-95 inactive-WLD sweep found no source-room overlap and three non-overlapping Surf approaches: #282142 east to #67801, #282143 down to #67800, and #282144 west to #67810. Preserve them on existing Surf candidate row 97 as source-only, not active ingress. Surf's reviewed subset rises to 281 and the curated total to 4,532. See the [Undead Fog audit](SHORTB_AUDIT_REVIEW.md), [coverage row 274](ZONE_COVERAGE_REGISTER.csv#L274), and [Surf row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).
### Checkpoint 363: Skulldrach connectivity and inactive approaches

The active Skulldrach WLD has 252 rooms and 691 directions. All flags and keys are zero; 686 directions stay local, two form reciprocal active boundaries, and three have negative destinations. The reciprocal pairs are #12796 north to Citadel #13000 and #12859 south to Surface #632637. The local weak graph has components of 225 and 27 rooms; room #12700 reaches the full 225-room component by authored directed exits. There are no positive unresolved targets, door resets, fall markers, or current markers.

All 95 inactive WLD sources parsed cleanly with no source-room VNUM overlap. One non-overlapping source-only Surf direction, #273887 north to #12859, reaches Skulldrach. Preserve it on existing candidate row 97; it is not active ingress. The reviewed Surf subset rises from 281 to 282 and the curated total from 4,532 to 4,533. See the [Skulldrach audit](SKULLDRACH_AUDIT_REVIEW.md), [coverage row 276](ZONE_COVERAGE_REGISTER.csv#L276), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 364: Para-Elemental Plane of Smoke connectivity and inactive approaches

The active Smoke WLD has 153 rooms and 649 authored directions; every target is local. Exit flags are 645 zero, two state 3 and two state 1. The two state-3 directions use key #139818. Four weak components have sizes 147, 4, 1, and 1. Four flagged directions match the local vault and forge D resets. There are no active cross-area directions, negative targets, positive unresolved targets, fall markers, or current markers. Fixed type-25 portal destinations remain separate from WLD adjacency; see the [Smoke audit](SMOKE_AUDIT_REVIEW.md).

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 directions without errors. Duris3 overlaps all 153 Smoke room VNUMs and has 612 target-matching directions: 302 from its colliding source-room IDs and 310 from source VNUMs with no active WLD owner. Forofcon overlaps Smoke #139900–#139952 and has 109 matching directions: 104 from colliding source IDs and five from non-overlapping sources with no active WLD owner. CP482 re-parsed all 817 current forofcon WLD exits and corrected the earlier 111/106 count; the five unique-source approaches are unchanged. Preserve the 315 distinct source-only directions on [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36) and [forofcon candidate row 45](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L45). They are inactive, not active ingress. The curated source-only total rises from 4,533 to 4,848; Surf remains at 282. See the [Smoke audit](SMOKE_AUDIT_REVIEW.md#inactive-wld-approaches-and-vnum-collisions) and [coverage row 277](ZONE_COVERAGE_REGISTER.csv#L277).

### Checkpoint 365: Shairak and Smokeveil Forest connectivity and inactive approach

The active Smokeveil WLD has 100 rooms and 226 directions: 224 local, with two reciprocal Surface pairs. Smokeveil #20201 east connects to Surface #614187 west; Smokeveil #20297 south connects to #614586 north. All 226 targets resolve. The local weak graph has one 100-room component, and authored directed traversal from #20200 reaches all 100 rooms. All keys are zero. Twelve state-1 directions match 100%-chance D resets; three state-4 directions have no reset. There are no negative or unresolved targets, fall markers, or current markers. See the [Smokeveil audit](SMOKEV_AUDIT_REVIEW.md).

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 directions without errors, with no Smokeveil room overlap and one non-overlapping source-only Surf direction: #263026 west to Smokeveil #20201. Preserve it on existing Surf candidate row 97; it is not active ingress. Surf rises from 282 to 283 reviewed source-only records and the curated total rises from 4,848 to 4,849. See the [Surf row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), [coverage row 278](ZONE_COVERAGE_REGISTER.csv#L278), and [Smokeveil audit](SMOKEV_AUDIT_REVIEW.md#inactive-wld-approach).

### Checkpoint 366: Valley of the Snow Ogres connectivity and inactive approach

The active [Snow Ogres WLD](../../../areas/wld/snogres.wld) contains 100 rooms and 281 authored directions: 280 local and one up from #87700 to Surface #620605. All 281 targets resolve in the active WLD set. The local graph has one weak component, and directed traversal from #87700 reaches all 100 rooms. Surface #620605 returns down to #87700; active Faerie Realm room #14210 has a separate northwest edge into smithy #87777. There are two active inbound directions, one outbound, and one reciprocal pair. The target WLD parses cleanly. The broad active sweep found an unrelated malformed block in northern_wilderness.wld near line 1232, visibly targeting #12270.

Three flag-4 directions from #87708 have no matching D reset. Four other flagged directions—#87717 west, #87718 east, #87745 south, and #87746 north—match exact 100-percent local D resets. Keep those reset-state observations separate from the switch effect and live door qualification in [the audit supplement](SNOGRES_AUDIT_REVIEW.md).

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 directions without errors, with no Snow Ogres room-VNUM overlap and no direction into #87700–#87799. No source-only approach is added, and no active-ingress conclusion follows from an inactive map. See [coverage row 279](ZONE_COVERAGE_REGISTER.csv#L279) and the [existing story map](../zone-stories/VALLEY_OF_THE_SNOW_OGRES.md).

### Checkpoint 367: Vast Hidden Grove connectivity and scenery travel

The active [Solonar WLD](../../../areas/wld/solonar.wld) has 100 rooms and 182 authored directions. All directions resolve locally; there are no active WLD boundary directions. The weak graph has seven components of 33, 26, 20, 10, 5, 3, and 3 rooms. The target WLD parses cleanly. The broad active scan found one unrelated malformed block in northern_wilderness.wld near line 1232, visibly targeting #12270.

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 directions with no parse errors, room-ID overlaps, or approaches into Solonar. Item travel remains a separate layer: active Elvish object #35804 at Homestead #35810 targets Solonar #30648, and local statue #30604 routes back to #35810. The story dossier records twelve other local scenery travel properties. Keep all such destinations separate from the WLD boundary census; see [coverage row 280](ZONE_COVERAGE_REGISTER.csv#L280) and the [Checkpoint 367 audit](SOLONAR_AUDIT_REVIEW.md).

### Checkpoint 368: Village of Split Shield connectivity and inactive approach

The active [Split Shield WLD](../../../areas/wld/split_shield.wld) has 99 rooms and 213/213 resolved authored directions in one weak component. Five active edges enter, four leave, and four outbound pairs have active reverse directions: three Surface pairs and the shortc pair. White Harbor #55402 up to #10307 is an additional one-way active inbound. The all-95 inactive-WLD census parsed 527,791 rooms and 2,080,773 exits without errors and found no Split Shield room-ID overlap. Two Surf directions target the village: #206049 south to #10300 and #206649 north to #10334. Retain them on candidate row 97 with their inactive source-file identity; the IDs also occur in surface2011 and underdark2011 under different room titles. They are not active ingress. The curated source-only total rises to 4,851 and Surf to 285. See the [Checkpoint 368 audit](SPLIT_SHIELD_AUDIT_REVIEW.md).

### Checkpoint 373: Surface connectivity and inactive approaches

The active Surface graph remains 639,866 authored directions, 639,864 loader-resolved, 423 inbound, 370 outbound, and 359 reciprocal records; two Surface exits still target unloaded #33807. A targeted scan of every inactive WLD source found 479,232 numeric directions into loaded Surface rooms. Of these, 1,208 originate at source VNUMs with no active WLD owner: asunwa #35300 north to #604903; dalvik #298 south to #635660; jungle #67005 north to #616072 and #67087 south to #616872; and 602 each in newbiemaps and surface2011-temp. The two large files share all 602 (source, direction, target) tuples while retaining different source titles. Their routes comprise 300 north directions from #660004–#660303 to #659704–#660003, 300 south directions from #689700–#689999 to #600000–#600299, plus #660004 west to #660003 and #660299 east to #660000. Preserve both file identities. The remaining 478,024 matching target records start at source-room VNUMs already owned by active rooms and remain map-identity evidence, not additional source-only ingress. These 1,208 source records raise the curated total from 4,851 to 6,059; the existing surf.wld subset stays 285. Keep the records on existing candidate rows 12, 29, 55, 78, and 99. See the [Surface audit](SURFACE_AUDIT_REVIEW.md#inactive-wld-surface-approaches) and [coverage row 286](ZONE_COVERAGE_REGISTER.csv#L286).
### Checkpoint 374: SurfaceKeeps connectivity and inactive approaches

The active SurfaceKeeps graph has 2,645 rooms and 9,452 authored directions; 9,446 resolve (9,372 local, 74 outbound), while six active exits still target unloaded Duris3 rooms. Seventy-nine active directions enter from Alatorin (76), Tharn Rifts (two), and White Harbor (one); 74 leave for Alatorin, with 69 reciprocal records. All 9,372 internal WLD exits have zero door flags and key VNUMs. Raw directed traversal from 76 distinct inbound rooms reaches 2,360 rooms; 285 are not reached. Reverse traversal from 74 outbound source rooms includes 2,644 rooms; #120000 is the only room without a static path to an outbound edge. Nine weak components remain (1,603, 673, 143, 87, 64, 33, 30, 11, 1). This graph does not qualify terrain, type-25 travel, runtime movement, or safe return. See the [SurfaceKeeps audit](SURFACEKEEPS_AUDIT_REVIEW.md#active-graph-and-boundaries) and [coverage row 287](ZONE_COVERAGE_REGISTER.csv#L287).

The inactive-WLD sweep found 10,588 directions into loaded SurfaceKeeps rooms. Of those, 1,457 start at VNUMs with no active owner: 1,451 from Duris3, five from oldmap8, and one from ygg. The other 9,131 start at active-owned VNUMs and remain identity evidence. Duris3 also declares all 2,645 SurfaceKeeps room VNUMs with different room titles; those overlapping source records are not counted as source-only approaches. The 1,457 records raise the curated source-only total from 6,059 to 7,516; the reviewed Surf subset remains 285. Keep them on existing candidate rows 36, 81, and 122. They are not active ingress. See the [audit supplement](SURFACEKEEPS_AUDIT_REVIEW.md#inactive-wld-approaches-and-room-identity).


### Checkpoint 375: Surface Minizones connectivity and inactive approaches

The active [Surface Minizones WLD](../../../areas/wld/surfacemini.wld) has 119 rooms and 261/261 resolved directions: 250 local and eleven outward to Surface. Eight directions enter from Surface and eleven leave; seven outbound records have reverse-direction edges. All active targets resolve, with no negative, unresolved, range-mismatch, or unregistered-WLD target. Ten weak components have sizes 55, 44, 6, 6, 2, 2, 1, 1, 1, and 1. Raw directed traversal from the seven distinct inbound targets reaches 112 rooms; reverse traversal from outbound source rooms reaches 118, with terminal room #98018 unable to reach an outward boundary. These are static WLD directions, not runtime arrival or safe-return evidence. See the [Surface Minizones audit](SURFACEMINI_AUDIT_REVIEW.md#active-graph-and-boundaries) and [coverage row 288](ZONE_COVERAGE_REGISTER.csv#L288).

Four outward routes lack ordinary reverse directions: #97906 south to #564065, #97952 south to #578863, #98016 south to #522869, and #98017 south to #626598. Crater #98014 has two inbound Surface routes; #519153 returns while #518753 does not. Active Surface objects #500002, #500035, #500051, and #500052 also target #97906, #97952, #98016, and #98017. These item routes remain distinct from the WLD graph. The inactive-WLD scan found no source-only direction into SurfMini and no same-VNUM room overlap; no candidate approach is added. Curated source-only totals remain 7,516 overall and 285 for Surf. See the [audit](SURFACEMINI_AUDIT_REVIEW.md#active-graph-and-boundaries).

### Checkpoint 376: Swamp connectivity and inactive approach

The active [Swamp One WLD](../../../areas/wld/swamp_one.wld#L1) has 36 rooms and 84/84 resolving authored directions: 82 local and two outward. Active Surface room #608724 enters #26001 and receives the reciprocal return; active Swamp Two room #26100 enters #26019 and receives its reciprocal return. The area has two active inbound and two outbound records, all in reciprocal pairs. Its weak graph is one component of 36 rooms. Static directed traversal from the two inbound targets reaches all 36 rooms, and every room has a directed path to an outward edge. This calculation does not model secret discovery, closed exits, Death Bog movement, or safety. There are no negative/unresolved targets, numeric range mismatches, fall markers, or current markers in this WLD.

The hidden weeds route connects #26017 east to #26036 west. The WLD loader converts its state field into a door; the local ZON resets close both directions, marking the east direction secret while leaving the west return non-secret and unlocked. This is source-state evidence only.

The inactive-WLD census found one source-only approach: [`surf.wld` room #267604 west to #26001](../../../areas/wld/surf.wld#L1418596). The numeric #267604 record in [`surface2011.wld`](../../../areas/wld/surface2011.wld#L1419685) is a differently titled Storm Sea room with no exit to this zone. Keep the Surf source identity; this is not active ingress. Surf’s reviewed source-only subset rises from 285 to 286, and the curated source-only total rises from 7,516 to 7,517. The existing 145 Surf active numeric references are unchanged. See the [Swamp One audit](SWAMP_ONE_AUDIT_REVIEW.md#static-connectivity-and-source-only-approach), [coverage row 289](ZONE_COVERAGE_REGISTER.csv#L289), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


### Checkpoint 377: Swamp Two connectivity and cavern routes

The active [Swamp Two WLD](../../../areas/wld/swamp_two.wld#L1) has 35 rooms and 85/85 resolving directions: 84 internal and one outward from #26100 north to Swamp One #26019. Swamp One #26019 south returns to #26100. The graph is one weak component of 35 rooms; raw directed traversal from #26100 reaches all 35, and each room has a directed path to the outward boundary. These are static directions with locks ignored; key access, quicksand movement, room safety, and live reachability remain unqualified.

Room #26134 says it is completely sealed, but the WLD gives it inbound directions from #26106 north and #26119 east plus a northwest (`DIR_NORTHWEST`) direction to #26102. That outgoing direction has WLD state 4, which the loader masks to 0, and has no matching D reset; it is not authored as a door. Ask the builder whether the room text, directions, or a separately implemented collapse effect should govern the route. The inactive-WLD scan found no source-only approach or same-VNUM room overlap. No candidate row is added and the curated source-only total remains 7,517 (Surf 286). See [direction 6](../../../src/core/defines.h#L435), the [Swamp Two audit](SWAMP_TWO_AUDIT_REVIEW.md#static-connectivity-and-cavern-routes), and [coverage row 290](ZONE_COVERAGE_REGISTER.csv#L290).
### Checkpoint 378: Tchan connectivity and Alterian Wilderness approaches

The active Tchan WLD has 10 rooms and 19 authored directions: 18 internal and one outbound from #4800 west to Surface #513140. Surface #513140 has the reverse east direction. All 19 resolve in the active source set. Tchan has one weak component of 10 rooms; directed traversal from #4800 reaches all rooms, and every room has a path to the outward boundary when doors and special procedures are ignored. The paired #4808/#4809 door is reset closed; the tranquility guard is placed at #4808 and has an eastward special. Static adjacency does not qualify passage, safe travel, or live reachability. There are no negative or unresolved targets, fall markers, or current markers.

All 95 inactive WLD files parsed without issues. Twenty authored exits in inactive alterian_wilderness.wld target Tchan VNUMs. Sixteen originate from the nine overlapping source room IDs #4801-#4809, whose Alterian titles and route identities differ from the active Tchan rooms. Preserve those matches as same-number source records, not current ingress.

Four exits originate in non-overlapping Alterian rooms: #4810 south to Tchan #4809, #4815 south to #4807, #4821 west to #4803, and #4822 west to #4804. These are distinct source-only approaches; preserve them on [Alterian candidate row 8](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L8). The curated source-only total rises from 7,517 to 7,521, while Surf remains 286. See the [Tchan audit](TCHAN_AUDIT_REVIEW.md), [coverage row 291](ZONE_COVERAGE_REGISTER.csv#L291), and [Alterian coverage row 359](ZONE_COVERAGE_REGISTER.csv#L359).

## Checkpoint 381: Temple of Laduguer graph and Sylvandawn approaches

The active Temple WLD has 103 rooms and 215/215 resolving authored directions: 214 internal and one outward from #8100 west to active Underdark #827455. Underdark #827455 east returns to #8100. One weak component contains all 103 rooms; ignoring doors and special handling, raw directed and reverse traversal from #8100 each cover 103 rooms. These static results do not qualify access past reset doors, class checks, hazards, spell travel, or safe return. The local census has zero negative or unresolved targets, zero range mismatches, two fall-chance markers, and zero current markers.

All 95 unregistered WLD sources were scanned. Only sylvandawn.wld contains directions into Temple VNUMs. It has 223 raw target directions: seventeen begin at source room IDs with no active owner, and 206 begin at IDs that overlap Temple rooms but have different room titles. Preserve those seventeen source-only approaches on existing [Sylvandawn candidate row 101](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L101):

| Inactive source | Direction | Temple target |
| --- | --- | --- |
| #8007 City Center | south | #8163 |
| #8007 City Center | west | #8110 |
| #8026 An Intersection of Pathways | east | #8172 |
| #8026 An Intersection of Pathways | south | #8171 |
| #8030 A Split in the Pathways | east | #8180 |
| #8086 A split in the Paths | south | #8128 |
| #8091 A Split in the Paths | east | #8102 |
| #8091 A Split in the Paths | south | #8119 |
| #8094 The Sylvan Trail | east | #8101 |
| #8098 The Sylvan Trail | east | #8100 |
| #8203 Practice Room | north | #8202 |
| #8206 An Overgrown Path | west | #8195 |
| #8210 The Trail of Tears | north | #8185 |
| #8211 The City Dump | east | #8121 |
| #8311 A Burnt Down and Abandoned Tower | north | #8180 |
| #8317 The Commons of the Sylvandawn Grovetree Theatre | south | #8188 |
| #8327 Alkatia's Fletcher Shop | south | #8172 |

Sylvandawn is not in the current full-world area list, so none of these are active Temple inbounds. The 206 same-number transitions are likewise source records, not current routes. The curated source-only WLD total increases from 7,521 to 7,538; Surf remains 286. See the [Temple audit](TEMPLE_LADUGUER_AUDIT_REVIEW.md#inactive-sylvandawn-approaches-and-room-identity), [coverage row 294](ZONE_COVERAGE_REGISTER.csv#L294), and [Sylvandawn candidate row 101](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L101).

## Checkpoint 382: Tentro connectivity and Surf approaches

The active [Tentro WLD](../../../areas/wld/tentro2.wld#L1) has 294 rooms and 682/682 resolving authored directions: 678 internal and four outward. Its raw weak graph has components of 227 and 67 rooms. Five active directions enter four distinct Tentro rooms: caves_skelenak #4152 up to #69088; Surface #543736 north to #69222; Surface #557287 east and #557688 north to #69080; and Winterhaven #55559 up to #69249. Four outward exits are paired with active returns: #69080 south/west to Surface #557688/#557287, #69088 down to caves_skelenak #4152, and #69222 south to Surface #543736. #69249 has no ordinary WLD exit back to Winterhaven.

Raw directed traversal from active inbound targets reaches 250/294 rooms; reverse traversal from outward source rooms covers 291/294. These are WLD-only counts with doors treated as open and item travel, scripts, and hazards omitted. Type-25 object #69001 is reset at prairie room #69038 and targets underwater room #69041 on CMD_ENTER; it provides a separate authored item-travel route into the cave, not an authored WLD direction. All local WLD targets resolve, with no negative targets, fall markers, or current markers.

Four source-only directions in unregistered [surf.wld](../../../areas/wld/surf.wld) enter active Tentro rooms. Preserve them on existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97):

| Inactive Surf source | Direction | Active Tentro target |
| --- | --- | --- |
| #212805, A Steep and Rocky Hiking Trail | east | #69093 |
| #213704, A Grassy Clearing in the Mountains | south | #69092 |
| #214304, Rolling Grassy Foothills | north | #69080 |
| #214744, The Great Sylvan Woods of Mielikki | north | #69222 |

These are source-only approaches, not active ingress. The separate Surface room #557288 titled Tentro has no direct exit to this area; neighboring #557287 and #557688 provide the active routes. The curated source-only WLD total rises from 7,538 to 7,542, and Surf from 286 to 290. See the [Tentro audit](TENTRO2_AUDIT_REVIEW.md#active-connectivity-and-routes), [coverage row 295](ZONE_COVERAGE_REGISTER.csv#L295), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

## Checkpoint 383: Tezcat connectivity and inactive approaches

The active [Tezcat WLD](../../../areas/wld/tezcat.wld) contains 100 rooms
and 239 authored directions: 238 internal and one outbound. All local targets
resolve, and the local weak graph is one 100-room component. Treating authored
directions as traversable, entry from either active inbound target reaches all
100 rooms; all 100 also have a directed route to the outbound source. These
raw traversals do not simulate door state, secret discovery, scripts, item
travel, or player safety.

| Boundary | Source → target | Reading |
| --- | --- | --- |
| Active inbound | Surface #575445 east → Tezcat #98900 | Paired with Tezcat #98900 west → #575445. |
| Active inbound | Winterhaven #55632 west → Tezcat #98974 | One-way WLD arrival; no ordinary Tezcat return to Winterhaven. |
| Active outbound | Tezcat #98900 west → Surface #575445 | Reciprocal Surface pair. |

The all-95 inactive-WLD sweep found two non-overlapping source-only approaches
to #98900: [surf.wld #265973 east](../../../areas/wld/surf.wld#L1384365)
and [underworld2.wld #429194 south](../../../areas/wld/underworld2.wld#L94491).
Neither source room has an active WLD owner, so neither record creates live
ingress. Preserve the Surf record on existing candidate row 97 and the
Underworld2 record on existing candidate row 116. Their active-reference
counts do not change.

All 14 reset door directions match authored doors. The entrance pairs reset
closed/unlocked; the office and bedroom pairs are closed/locked with stone
key #98913 and bone key #98912 respectively. The altar trapdoor pair uses
bloody key #98915 and reset state 6 (closed, locked, secret). The #98943/#98944
secret chamber has asymmetric authored secret flags that its paired reset
normalizes; the #98957/#98962 wall is secret in both directions. Key reset
placements and WLD clues are detailed in the
[Tezcat audit supplement](TEZCAT_AUDIT_REVIEW.md#active-connectivity-and-keyed-routes).

The [cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1182)
records Tezcat's active outbound, [row 782](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L782)
records the Winterhaven arrival, and [row 1498](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1498)
records the Surface return. The inactive approaches are source-map evidence,
not current world routes. See the [Tezcat story dossier](../zone-stories/TEZCAT.md)
and [audit supplement](TEZCAT_AUDIT_REVIEW.md).

## Checkpoint 384: Tharnadia connectivity and Duris3 approaches

The active [Tharnadia WLD](../../../areas/wld/tharnadia.wld) has 584 rooms
and 1,268 authored exit records. The loader resolves 1,267 positive
directions: 1,262 local and five outward; the remaining source record is
#132641 north to `-1` and is dropped. Six active directions enter and five
leave. Braddistock #135001 south to #132650 is reciprocal. Surface #570422
east to #133053 and #570424 west to #132867 are also reciprocal. The other
three arrivals are Surface #570023 south to #133066, Surface #570823 north to
#133056, and administrative Heavens #24 north to #132573. Their local outward
edges leave from #133070, #133064, and no matching city exit, respectively.
The full boundary table and source links are in the
[Tharnadia audit](THARNADIA_AUDIT_REVIEW.md#active-connectivity-and-route-state).

The strict undirected WLD graph has components of 551, 20, 10, 1, 1, and 1
rooms. Raw directed WLD traversal from all six active arrival targets reaches
550/584 rooms; 551/584 rooms can reach an outward source. These counts omit
doors, resets, secrets, item teleport, scripts, falls, and actor state. Four
local item-teleport destinations provide authored access outside that graph:
objects #132625/#132626 connect the #132501 pocket with park #132919,
#132665/#132666 link wizard rooms #133018/#133019, and active Newbie Zone
object #22803 targets #132575. There are 31 fall-chance rooms and no current
fields. The negative-target record, teleport destinations, and door/reset
differences do not establish safe access or a successful player journey.

The all-95 inactive-WLD scan found 2,336 Duris3 directions targeting the
Tharnadia room-number range. The 604 from non-overlapping source rooms are
source-only approaches: 300 south from #132200–#132499 to #132500–#132799,
300 north from #133084–#133383 to #132784–#133083, and four endpoints
(#132499 east to #132500; #133299 east to #133000; #132400 west to #132699;
#133084 west to #133083). The other 1,732 directions start at the 584 shared
room VNUMs. Those Duris3 rooms have different titles from active Tharnadia;
keep both map identities and exclude those matches from the unique-source
approach count. Preserve all findings on existing [Duris3 candidate row
36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36); its 26 active target
references do not change. The 604 approaches raise the curated source-only
WLD total from 7,544 to 8,148. The reviewed Surf subset remains 291. They are
inactive source-map evidence, not current ingress. See [coverage row
297](ZONE_COVERAGE_REGISTER.csv#L297), the [Tharnadia audit](THARNADIA_AUDIT_REVIEW.md),
and [Duris3 room sample](../../../areas/wld/Duris3.wld#L676201).

### Checkpoint 385: Tharnadian Ruin directed reach and route state

Tharnadian Ruin has 179 rooms and 371/371 resolving authored exits in weak
components of 168, 10, and 1. Its three active WLD arrivals target #5501 from
Caertannad, #5588 from Cityruin, and #5616 from Braddistock Mansion. Three
outbound directions return from those same rooms to the neighboring areas;
all three boundaries are reciprocal; see the [cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L90) and return rows [895](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L895), [830](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L830), and [21](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L21).

Raw directed traversal from the three arrival targets reaches 165/179 rooms.
It does not reach #5592, #5602–#5611, or #5661–#5663. Reverse traversal from
the three outbound sources finds that 167/179 rooms can reach an active
boundary; #5592, #5602–#5611, and #5662 cannot. These are WLD-only counts and
do not model doors or resets. Each of the 26 authored door directions has a
matching D reset, with no reset on a non-door. Two reciprocal pairs have
different source values: #5511 down ↔ #5528 up has flags/reset values 5/1
on the two sides; #5671 east ↔ #5672 west has matching flags/reset value 2
but keys #5527/#5528. The [Tharnadian Ruin audit](THARNADIAN_RUIN_AUDIT_REVIEW.md#checkpoint-385-directed-reachability-reset-state-and-builder-questions)
records both for builder review without labeling either a defect.

The active Surface type-25 object #500055 is reset at #568439 (“The Impact
Crater”). Its compiled handler sends a user of “portal” or “shadowrift” to
local #5569 or #5583 according to race-war side ([Surface reset](../../../areas/zon/surface.zon#L32), [object binding](../../../src/specs/specs.assign.c#L1277), [handler](../../../src/specs/specs.zion.c#L37)). Local type-25 object #5539 at
#5659 points back to Surface #568439 ([local object](../../../areas/obj/tharnadian_ruin.obj#L539)). These routes are distinct from the WLD
edge register, and source declarations do not prove the object is present,
usable, or safe. The all-95 inactive-WLD sweep found no Ruin room overlap and
no inactive exit into a Ruin room. Curated source-only totals remain 8,148
overall and 291 in the reviewed Surf subset. Keep active Tharnadian Ruin DO
NOT MOVE; no attic candidate or approach row changes.

## Checkpoint 386: Tharnadia Rifts directed reach and inactive approaches

The active Tharnadia Rifts WLD has 5,783 rooms and 22,252 authored directions:
22,225 local targets, 25 active neighboring-zone exits, and two positive
targets found only in inactive Duris3. Seven weak components contain
1,963/1,846/1,124/845/3/1/1 rooms. Seventy-four active arrival edges lead to
74 rooms (73 Caertannad, one Heavens); 25 edges leave (23 reciprocal to
Caertannad, two one-way to SurfaceKeeps #120000). Directed reach from the
arrival targets covers 5,349/5,783 rooms; 5,781/5,783 can reach an active
outbound. #110000 and #112545 cannot reach one. All exit flags are zero; no
doors, D resets, fall fields, or current fields were found. The
[Tharnrifts audit](THARNRIFTS_AUDIT_REVIEW.md#active-wld-graph) records the
exact unresolved targets, routes, and limits.

The all-95 inactive-WLD scan parsed 2,080,773 directions without issues. It
found 1,323 source-only approaches from Duris3 and one each from bb_gh and
circlegh. Another 21,809 Duris3 directions begin at shared source VNUMs whose
room titles all differ from the active map; they remain separate map identity
evidence. Cabal and rf_gh each add one target match from an active-owned source
room and are excluded from the approach count. The increment is 1,325
source-only directions, raising the curated global total 8,148->9,473; Surf
remains 291. Keep all inactive sources on their existing identities and
candidate rows. Preserve the two active unresolved edges to Duris3 VNUMs on
[unresolved rows 319–320](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L319) and
the existing [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

## Checkpoint 387: Thetis boundary and Surf approach refresh

Thetis has 100 rooms and 218 authored exit records: 215 local targets, two
active outbound edges, and one unresolved #38095 north exit to #104. There is
no active WLD inbound edge. The one weak component has directed raw reach and
return of 100/100 from beach root #38000; this local graph does not make that
room an active arrival. The outbounds remain #38000 south to Surface #618491
and #38075 south to Heavens #38. There are no fall or current fields. See the
[Thetis audit](THETIS_AUDIT_REVIEW.md#checkpoint-387-active-boundary-refresh-surf-approach-and-builder-questions).

The all-95 inactive-WLD recheck parses 2,080,773 exits without issues and
finds no Thetis room-VNUM overlap. It finds one source-only Surf edge,
#251543 north to #38000. Keep it on existing Surf candidate row 97; it is not
active ingress. The curated source-only WLD total rises from 9,473 to 9,474,
and Surf rises from 291 to 292. The Surf candidate's existing 145 active
unresolved references and 17 source-area keys are unchanged.

## Checkpoint 388: Thri connectivity and inactive approach

The active Thri WLD has 155 rooms and 339 authored exit directions: 338 local
and one outbound, with no negative or unresolved target. All rooms form one
weak component. The active Surface boundary is reciprocal: #603653 west to
Thri #67600 and #67600 east to Surface #603653. Raw directed traversal from
#67600 reaches 145/155 rooms, while all 155 can reach #67600; ten rooms are
outside raw WLD reach from the entrance. This excludes item type-25 and runtime
routes and does not establish safe player access. See the
[Thri review](THRI_STORY_REVIEW.md#checkpoint-388-graph-identity-and-typed-dependency-refresh)
and [existing active boundary rows](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L608).

The all-95 inactive-WLD sweep finds no Thri room overlap and one source-only
Surf approach, #250600 west to #67600. It is not active ingress. Curated
source-only WLD totals rise from 9,474 to 9,475 overall and from 292 to 293
for Surf; Surf's 145 active unresolved-reference records and 17 source-area
keys are unchanged on candidate row 97.

## Checkpoint 391: Lost Temple connectivity and inbound routes

The active Lost Temple WLD has 234 rooms and 616 resolving authored exits:
615 local and one outbound, #44101 up to Lost City #44039. The local graph is
one weak component. Ordinary directed traversal from #44101 reaches 234/234
rooms, while 233/234 can return to #44101 because High Altar #44333 has no
ordinary outward WLD exit. There are no negative or unresolved targets,
out-of-range source rooms, falls or currents. These are source-graph counts;
they do not model reset locks, secrets, script state or safe play.

Two active exits enter the Temple: the reciprocal Lost City boundary
#44039 down to #44101, and the one-way Winterhaven arrival #55632 east to
#44333. Only the City boundary has a reverse WLD edge. The sole active
Temple outbound is #44101 up to #44039. See exit-register
[rows 618–619](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L618) and
[Winterhaven row 781](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L781), plus pair
summary [rows 660–661 and 820](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L660).

The altar sink also has a distinct item route: reset array #44190 at #44333
is type 25 and targets Lost City Snake Pit #43704. That declaration is not a
WLD return edge, and source data does not establish item availability or
safe onward travel. The [Temple review](TIKITT_AUDIT_REVIEW.md#active-map-and-directed-reach)
records this route, all 95 inactive-WLD results, and the keyed/speech-door
boundary. The inactive sweep finds no Temple room overlap or approach; the
curated source-only total remains 9,475 overall and 293 for Surf. Keep active
tikitt DO NOT MOVE; no candidate or source-only route is added.

## Checkpoint 392: Torg boundary and inactive approach

The active Torg WLD has 282 rooms and 624 resolving exits: 623 local and one
outbound. Two weak components contain 265 and 17 rooms. Four active WLD edges
enter Torg: Surface #611053 east to #28900, Scorch Valley #71330 down to
#29071, Winterhaven #55633 northwest to #29093, and Juiblex #87672 down to
#29073. Only the Surface edge has a reverse; Torg #28900 west returns to
Surface #611053. Raw directed reach is 253/282 from #28900 and #29093, and
17/282 from #29071 and #29073. A total of 264/282 rooms can reach the active
outbound; #29063 and #29065–#29081 cannot. Direction 6 is Northwest in
`src/core/defines.h`; doors, item routes, resets, and travel safety are outside
these WLD counts. See the [Torg audit](TORG_AUDIT_REVIEW.md#active-wld-graph-and-boundary).

The all-95 inactive-WLD sweep found 97 same-numbered room records with titles
different from Torg and 163 target matches. 162 begin at VNUMs also owned by
active Torg rooms. One non-overlapping source-only edge remains: inactive
`surf.wld` #277211 east to Torg #28900
([source room](../../../areas/wld/surf.wld#L1620203),
[exit](../../../areas/wld/surf.wld#L1620211),
[target](../../../areas/wld/surf.wld#L1620214)). Add it to the existing Surf
candidate row 97; it is not active ingress. Curated source-only WLD totals rise
from 9,475 to 9,476 overall and 293 to 294 for Surf. Keep active Torg DO NOT
MOVE. No Torg candidate is added.

## Checkpoint 393: Torrhan boundaries and directed reach

The active Torrhan WLD has 289 rooms and 623 resolving exit records: 619 local
and four outbound, in one weak component. Four active Surface arrivals
reciprocate all four Torrhan outbound edges. Winterhaven #55614 D8 also points
to market #66600 from an administrative Incarnate dispersal room, not a normal
arrival. Each of the five WLD arrival targets reaches 280/289 rooms by authored
directional exits. Nine rooms (#66864–#66870, #66900, and #66901) are outside
that reach set; 286/289 rooms can reach an outbound by WLD alone. #66804 and
#66805 have a separate local type-25 portal route; it does not change WLD
reach or prove safe travel.

The all-95 inactive-WLD sweep found no Torrhan room overlap and three
source-only approaches from inactive surf.wld: #212237 D2 to #66689, #212538
D3 to #66712, and #212837 D0 to #66692. Record them on existing Surf candidate
row 97, not as active ingress. Curated source-only WLD totals rise from 9,476
to 9,479 overall and from 294 to 297 for Surf. Keep active Torrhan DO NOT MOVE.
See the [Torrhan audit](TORRHAN_AUDIT_REVIEW.md#active-wld-graph-and-boundaries).

## Checkpoint 394: Troll Slave Compound topology and Ghore boundary

The active Torture WLD has 31 rooms and 61 resolving exit records: 60 internal
and one outbound to Ghore. Its internal exits form one weak component; in the
raw directed WLD graph, all 31 rooms are reachable from #95400 and all 31 can
reach #95400 and its Ghore outbound. The reciprocal boundary records are
Ghore #11529 south to Torture #95400 and Torture #95400 north to Ghore #11529.
Both have exit flags 0 and key 0, so neither boundary record is a door. The
separate interior gate from #95400 to #95401 resets closed from both sides.

Four authored fall fields and two reset-closed, locked internal pairs remain
distinct from the raw exit graph. Graph reach does not prove traversal through
those states, fall safety, live gate behavior, or return after a hazard. All 95
inactive WLD sources were checked: none overlaps Torture's room numbers or
targets a Torture room. No source-only inbound edge is added; curated totals
remain 9,479 overall and 297 for Surf. Keep active Torture DO NOT MOVE. See the
[Torture audit](TORTURE_AUDIT_REVIEW.md#active-wld-graph-and-ghore-boundary)
and [coverage row 307](ZONE_COVERAGE_REGISTER.csv#L307).

## Checkpoint 396: Tradeskills utility hub and conditional portal

The active Tradeskills WLD contains only #400000 and #400999, with two
resolving local exits forming one directed component. Neither room has an
authored outside WLD edge. The all-95 inactive-WLD sweep found no inbound
approach or room-number overlap, and no declared type-25 portal targets either
room. Curated source-only totals remain 9,479 overall and 297 for Surf.

Death handling can create portal #400220, set its destination to a victim's
room, and place it in #400000. This conditional code path is not part of the
static WLD graph and does not qualify entry, return, or safety. See the
[Tradeskills audit](TRADESKILLS_AUDIT_REVIEW.md#active-graph-and-dynamic-portal),
[source dossier](../zone-stories/TRADESKILLS.md), and
[coverage row 309](ZONE_COVERAGE_REGISTER.csv#L309).

## Checkpoint 397: Trakkia access scenarios retained

The existing Trakkia review remains 166 rooms and 387/387 resolving exits,
with two reciprocal Surface pairs, five weak components, and four
source-flagged blocked exits. Conditional switches, keys, seven portals,
staging rooms, actor admission, and hazard survival remain separate from the
raw root traversal; no route or count changed in this builder-question review.
See the [Trakkia audit](TRAKKIA_AUDIT_REVIEW.md#active-graph-and-surface-boundaries),
[builder questions](../zone-stories/TRAKKIA_MOUNTAINS.md#explicit-builder-questions-and-recommended-resolutions),
and [coverage row 310](ZONE_COVERAGE_REGISTER.csv#L310).

## Checkpoint 398: Traoul's Lair doors and boundary

The active Traoul WLD has 44 rooms and 91/91 resolving exits: 90 local and
one outbound to Truk #1622. With every authored edge treated as passable, all
44 rooms are reachable from #7000 and all can return; this is one raw strongly
connected component. Truk #1622 provides the reciprocal inbound edge. The
14 local door resets restore closed, locked, or secret states, including the
first gate keyed to Truk object #1609, the inner gate keyed to #7009, and
tunnel doors keyed to #7001. The latter key has no reset placement in the
source scan. No other external active or source-only WLD approach was found.
See the [Traoul audit](TRAOUL_AUDIT_REVIEW.md#active-graph-and-truk-boundary),
[route dossier](../zone-stories/TRAOULS_LAIR.md#routes-doors-and-keys), and
[coverage row 311](ZONE_COVERAGE_REGISTER.csv#L311).

## Checkpoint 399: Tribal Forest boundaries and directed access

The active Tribal WLD has 175 rooms and 462/462 resolving exits: 453 local
and nine outbound. Its local graph has three weak components sized 173, 1,
and 1. From #42281, 129 rooms are reachable through authored WLD exits alone,
and 172 have a directed WLD path back; doors, switches, travel objects,
actor eligibility, and hazards are outside that raw count. Singleton #42297
has six exits to Astral Main; singleton #42374, “Inside a Force Cage,” has no
WLD exit or static WLD ingress. The reset-placed black rift at #42297 is a
separate type-25 route to #42295.

Surface has three reciprocal approaches at #42281: north to #630869, south to
#631669, and west to #631268, with matching return exits. The Astral boundary
is WLD-outbound only in the active source; no other active or source-only WLD
approach into Tribal was found. No outside active type-25 object points into a
Tribal room. See the [Tribal audit](TRIBAL_FOREST_AUDIT_REVIEW.md#active-graph-and-world-boundaries),
[dossier](../zone-stories/TRIBAL_FOREST.md#access-controls-travel-and-hazards),
and [coverage row 312](ZONE_COVERAGE_REGISTER.csv#L312).


## Checkpoint 400: Transparent Tower boundaries and directed access

The active Tower WLD has 100 rooms and 248/248 resolving exits: 247 local
and one outbound. Its local graph has four weak components sized 63, 27, 8,
and 2. From #16228, 63 rooms are reachable by authored direction alone and
30 can return; the Avernus component at #16271 has 27/27 forward and return;
from drawbridge #16212, 28 are reachable and 43 can return. These counts
ignore door, speech, hidden-exit, reset, hazard, actor, and item-travel state.

Active boundaries are the reciprocal Tower #16212 south ↔ ConnectorZones
#54167 north crossing and the one-way Realm #14209 south arrival at Tower
#16271. The all-95 inactive-WLD scan adds one non-overlapping source-only
`surf.wld` edge: #245010 north to #16212. It is not active ingress. At Checkpoint 400, the curated source-only totals were 9,480 overall and 298 for Surf. These are historical counts; the #245010 approach is already included in the current 11,993 overall / 415 Surf total. No outside active type-25 object targets a Tower room. See the
[Transparent Tower audit](TRNSPTOW_AUDIT_REVIEW.md#active-graph-and-world-boundaries),
[source dossier](../zone-stories/TRNSPTOW.md#companions-controls-library-and-exit),
and [coverage row 313](ZONE_COVERAGE_REGISTER.csv#L313).

## Checkpoint 401: Troll Caves boundaries and directed access

The active Troll Caves WLD has 82 rooms and 178/178 resolving exits in one
local weak component, with no authored current fields. Its two active
cross-area boundaries are reciprocal: Surface room #622564 down ↔ Troll Caves
#96900 up, and Malch #23741 east ↔ Troll Caves #96976 west. Raw direction-only
counts do not account for doors, switches, hidden exits, item travel, actor
eligibility, or falls.

The three previously cataloged unregistered source records from `surf.wld`
and `underdark2011.wld` still target active rooms #96900 and #96903. They remain
source-only and do not add active ingress; they were already included in the
then-current curated totals of 9,480 overall and 298 for Surf. Those counts are
historical; current cumulative totals remain 11,993 overall / 415 Surf. No new
cross-area WLD record was found in CP401. See the [Troll Caves audit](TROLL_CAVES_AUDIT_REVIEW.md#active-and-source-only-wld-boundaries),
[route dossier](../zone-stories/TROLL_CAVES.md#material-access-and-exploration-routes),
and [coverage row 314](ZONE_COVERAGE_REGISTER.csv#L314).

## Checkpoint 403: Truktakyagyo boundaries

The active Truktakyagyo WLD has 56 rooms and 115/115 resolving exits: 112 local
and three outbound. All 56 rooms are in one local weak component. Three
reciprocal boundaries connect room #1600 east ↔ Underdark #846820 west, #1622
north ↔ Traoul's Lair #7000 south, and #1655 south ↔ Goblin Outpost #70198
north. There are no fall or current fields. These raw exits do not qualify
gateway state, key use, or live travel.

The inactive-WLD sweep found no overlapping room or additional source-only
approach into Truktakyagyo. At Checkpoint 403, curated source-only totals were
9,480 overall and 298 for Surf; those counts are historical. The ud_map2
approach is already included in the current 11,993 overall / 415 Surf totals. See the [Truktakyagyo audit](TRUK_AUDIT_REVIEW.md#active-graph-and-boundaries),
[source dossier](../zone-stories/TRUK.md#districts-and-authored-routes),
and [coverage row 316](ZONE_COVERAGE_REGISTER.csv#L316).

## Checkpoint 404: Twin Towers boundaries and directed access

The active Twin Towers WLD has 99 rooms and 211/211 resolving exits: 207 local
and four outbound. All rooms are in one local weak component. Four reciprocal
boundaries connect rooms #13200, #13230, and #13259 with Twin Towers Forest,
and room #13297 with Underworld 3. One authored F30 fall is at room #13239;
there are no current fields. These records do not qualify door state, dynamic
access, safe travel, or a successful return.

The inactive-WLD sweep found no overlapping room or additional source-only
approach into Twin Towers. At Checkpoint 404, the cumulative source-only total was 9,480 overall / 298 Surf. Those counts are historical; current totals are 11,993 overall / 415 Surf. See the [Twin Towers audit](TWIN_TOWERS_AUDIT_REVIEW.md#active-wld-graph),
[source dossier](../zone-stories/TWIN_TOWERS.md#access-clues-and-larger-episodes),
and [coverage row 317](ZONE_COVERAGE_REGISTER.csv#L317).

## Checkpoint 406: Turolopolis Zoo boundaries and directed access

The active Zoo WLD has 62 rooms and 141/141 resolving exits: 140 local and one
outbound. The local graph is one weak component. The reciprocal active boundary
is Willem #7123 west ↔ Zoo #53000 east. From #53000, authored open exits reach
and return from all 62 rooms; with the closed/secret/blocked down exit at
#53019 unavailable, forward reach is 60 rooms while all 62 retain a return
path. These counts do not qualify switch, SEARCH/OPEN, lawful encounter, safe
return, or actual player access.

The inactive-WLD sweep found no room overlap or additional source-only approach
into the Zoo. There are no fall or current fields. The curated source-only WLD
totals remain 9,480 overall and 298 for Surf. See the [Turolopolis Zoo audit](TUROLZOO_AUDIT_REVIEW.md#active-map-and-willem-boundary),
[source dossier](../zone-stories/TUROLZOO.md#access-controls-and-sharedcustom-caller-boundaries),
and [coverage row 319](ZONE_COVERAGE_REGISTER.csv#L319).

## Checkpoint 407: Twin Towers Forest boundaries and directed access

The active Twin Towers Forest WLD has 100 rooms and 259/259 resolving exits.
Its local weak graph has two components, the largest containing 99 rooms. Four
outbound active boundaries comprise Surface #559646 east ↔ Forest #13500 west
and three reciprocal routes with Twin Towers rooms #13200, #13230, and #13259.
Six active inbound records include the four returns plus two additional
one-way exits into #13595 from Pine Hollow #16162 and Krimman #16486. The latter
are load-room records; they do not establish public entry. No inactive-WLD
overlap or source-only approach was found, and no fall or current fields are
authored.

These directed records do not qualify hidden garden access, door state,
ordinary arrival, safe return, or completion of a connected tower campaign.
The curated source-only WLD totals remain 9,480 overall and 298 for Surf. See
the [forest dossier](../zone-stories/TWIN_TOWERS_FOREST.md#lore-and-boundaries),
the [active exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L230), and
[coverage row 320](ZONE_COVERAGE_REGISTER.csv#L320).

## Checkpoint 405: Tundra boundaries and directed access

The active Tundra WLD has 175 rooms and 401 authored exits; 399 resolve through
the active registry. Eight reciprocal boundary pairs connect Tundra to Surface
and the Great Underground River. Heavens has one incoming staff-room edge to
reception without a corresponding ordinary return. Two outgoing records are
unusable: room #13869 targets -1, and room #13893 targets legacy room #110038
in inactive Duris3. Four local weak components remain, with the largest at 162
rooms. No fall or current fields are authored.

Seven additional Surf exits appear only in unregistered surf.wld source. They
target active Tundra rooms but add no active ingress. These records do not
qualify door/reset state, live reachability, fishing access, safe traversal,
actor availability, or return. The curated source-only WLD totals stay at
9,480 overall and 298 for Surf. See the [Tundra audit](TUNDRA_AUDIT_REVIEW.md#active-and-source-only-wld-boundaries),
[source dossier](../zone-stories/TUNDRA.md#access-actual-availability-and-services),
and [coverage row 318](ZONE_COVERAGE_REGISTER.csv#L318).

## Checkpoint 408: A Dark and Twisted Wood boundaries and directed access

The active Twisted Wood WLD has 100 rooms and 335/335 resolving exits: 331
local and four outbound. Its local weak graph is one component. From #16300,
authored open exits reach 97 rooms and every room has a directed return path
to the root. Four outbound edges form reciprocal pairs with Surface #568510
and #568109, Pine Hollow #16097, and Krindor #16400. Two additional active
incoming-only Pine Hollow edges lead from #16162 east to #16308 and #16163 east
to #16344; these load-room records are not ordinary entrances.

Five approaches from unregistered surf.wld target rooms #16300–#16303. There
is no inactive-WLD room overlap, and these source-only exits do not add active
ingress. No fall or current fields are authored. The authored graph does not
qualify concealed search/open state, lawful arrival, actor placement, safe
return, or conditional SHIFT PRIME use. The curated source-only WLD totals
remain 9,480 overall and 298 for Surf. See the [Twisted Wood audit](DARK_AND_TWISTED_WOOD_AUDIT_REVIEW.md#active-graph-and-boundary-records),
[source dossier](../zone-stories/DARK_AND_TWISTED_WOOD.md#access-concealed-camp-and-optional-treasure),
and [coverage row 321](ZONE_COVERAGE_REGISTER.csv#L321).

## Checkpoint 409: udmini boundaries and directed access

Active udmini has 21 rooms and 39/39 resolving exits: 38 local and one
outbound. Its weak components contain 19 and two rooms. The active inbound
records are ConnectorZones #54239 down to #88502 and Kvarkpass Connector
#54811 up to #88515; the one local outbound boundary is #88502 up to
ConnectorZones #54194. These yield two inbounds, one outbound, and no direct
reciprocal boundary pair. Both arrivals reach the corpse-and-cave component;
the two mushroom rooms have no active WLD ingress.

Rooms #88502 and #88520 each declare F 100. For an ordinary grounded actor
without a fall-avoidance or catch outcome, the open downward route can carry a
fall from #88502 through #88520 to #88503. The reset-placed crack in #88515
teleports to Kvarkpass #54811, which itself has F 100 and a downward exit.
These source records do not prove safe arrival or return. No inactive-WLD room
overlap, approach, or target to udmini was found. See the [UDMINI audit](UDMINI_AUDIT_REVIEW.md#active-wld-graph-boundaries-and-fall-state),
[source dossier](../zone-stories/UDMINI.md#entrances-falling-and-the-return-crack),
and [coverage row 322](ZONE_COVERAGE_REGISTER.csv#L322).

## Checkpoint 410: Ugta boundaries and directed access

Active Ugta has 242 rooms and 481/481 resolving exits. Its local weak graph
has five components: a 238-room component and four singletons (#39300–#39303).
The only active cross-area boundary is reciprocal with Surface: Ugta #39335
south to Surface #543298 and the Surface return north to #39335. An additional
`surf.wld` source-only exit, #209731 north to #39335, is not active ingress.

The gate approach #39332 and massive gate #39331 are reset closed on both
sides. The caribou-bone cell doors are reset closed and locked, with key field
0. The static census records no current or fall fields, even though the source
describes cold river crossings. These records do not establish live door
state, route availability, or crossing safety. See the [Ugta source review](UGTA_STORY_REVIEW.md#doors-and-static-connectivity),
[builder questions](UGTA_STORY_REVIEW.md#explicit-builder-questions-and-recommended-resolutions),
and [coverage row 323](ZONE_COVERAGE_REGISTER.csv#L323).

## Checkpoint 411: Durian Underdark loader-qualified graph

The active area has 13,587 rooms and 53,661 authored exits. Loader
qualification resolves 46,848 exits (46,747 local and 101 outbound); 6,813
targets are absent sparse cells and are pruned. The 101 active inbound and
101 outbound boundary records form reciprocal pairs. The local graph has six
weak components sized 13,192, 274, 102, 14, 4, and 1 rooms. No fall or current
fields are authored, and mountain terrain admission excludes non-trusted map
entrants, so static graph reach does not establish safe or ordinary-player
travel.

The inactive-WLD sweep found no overlap or approach into current `underdark`.
That result does not clear the separate `underdark2011` candidate, which has
11 active exits from five owners into its unregistered VNUMs and remains
retained pending its own closure. See the [audit graph review](DURIAN_UNDERDARK_AUDIT_REVIEW.md#active-graph-and-traversal-limits),
[candidate review](DURIAN_UNDERDARK_AUDIT_REVIEW.md#inactive-wld-sweep-and-the-underdark-2011-candidate),
[builder questions](../zone-stories/DURIAN_UNDERDARK.md#explicit-builder-questions-and-recommended-resolutions),
and [coverage row 324](ZONE_COVERAGE_REGISTER.csv#L324).

## Checkpoint 412: Undermountain boundaries and directed access

Active Undermountain has 441 rooms and 1,059/1,059 resolving exits in one
local weak component. Its west boundary #92501 south to `bs` #74045 has a
reciprocal north return. Room #92518 down reaches Underworld #4557 without a
direct reverse edge; Underworld #4557's up route reaches Kvarkpass Connector
#54841 instead. The all-95 inactive-WLD review found no source-room overlap or
exit into Undermountain. Static connectivity does not establish door state,
ordinary access, safe travel, or a live return. See the [audit boundary
review](UNDERMOUNTAIN_AUDIT_REVIEW.md#active-wld-boundaries), [builder
questions](../zone-stories/THE_RUINS_OF_UNDERMOUNTAIN.md#explicit-builder-questions-and-recommended-resolutions),
and [coverage row 325](ZONE_COVERAGE_REGISTER.csv#L325).

## Checkpoint 413: Underworld boundaries and directed access

Active Underworld has 232 rooms and 511 authored exits; the loader resolves
510 (484 local and 26 outbound) and drops the #4645 north target #4646. The
local graph has components of 154, 70, 6, 1, and 1 rooms. Thirty active inbound
records and 26 outbound records include 26 reciprocal pairs; four inbound-only
records remain separate. Thirteen current fields and no fall fields are
present.

The all-95 inactive-WLD review found no source-room overlap and three
source-only approaches from `ud_map0` and `ud_map2`. They are not active
ingress and remain retained with their source paths. These counts do not
establish live stock, door state, ordinary access, or safe travel. See the
[audit graph](UNDERWORLD_AUDIT_REVIEW.md#active-graph-and-boundary-records),
[inactive approach review](UNDERWORLD_AUDIT_REVIEW.md#inactive-wld-approaches-and-attic-evidence),
[builder questions](../zone-stories/UNDERWORLD.md#explicit-builder-questions-and-recommended-resolutions),
and [coverage row 326](ZONE_COVERAGE_REGISTER.csv#L326).

## Checkpoint 414: Underworld3 boundaries and directed access

Active Underworld3 has 283 rooms and 682/682 resolving exits. Its local weak
graph has two components, the largest containing 238 rooms. Nine active
inbound records, eight outbound records, and eight reverse records include
one active inbound-only edge: `mobs_underdark` #24007 north to Underworld3
#30149. Outbound source records lead to Moonshae #26272 from #30000
([exit](../../../areas/wld/underworld3.wld#L13)), Mistywood #95045 from #30162
([exit](../../../areas/wld/underworld3.wld#L3353)), Twin Towers #13297 from
#30167 ([exit](../../../areas/wld/underworld3.wld#L3441)), Otower #11348 from
#30172 ([exit](../../../areas/wld/underworld3.wld#L3546)), Tundra #13733 from
#30264 ([exit](../../../areas/wld/underworld3.wld#L5477)), and Foggy Woods
#90212 east from #30281 ([exit](../../../areas/wld/underworld3.wld#L5821)).
Two separate Underworld3 boundaries reach Underdark: #30281 north to #722840
([exit](../../../areas/wld/underworld3.wld#L5817)) and #30282 down to #709628
([exit](../../../areas/wld/underworld3.wld#L5843)). No fall or current fields
are authored.

`mobs_underdark` is itself listed in `areas/AREA`; its #24007 edge is active
one-way ingress, not an unregistered/source-only approach. Counts do not
establish ordinary access, safe travel or return, reset state, portal
availability, or dynamic behavior. See the [Underworld3 builder questions](../zone-stories/UNDERWORLD3.md#explicit-builder-questions-and-recommended-resolutions),
[active inbound edge](../../../areas/wld/mobs_underdark.wld#L174),
[cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L389), and
[coverage row 327](ZONE_COVERAGE_REGISTER.csv#L327).

## Checkpoint 415: Unique Vault boundaries and directed access

Active `unique` has two rooms and two of two resolving exits in one weak
component. The only local route is the reciprocal vertical pair from #67200
down to #67300 and back up. The static cross-area register records no active
inbound or outbound boundary, and neither room has a current or fall field.
This small display-oriented map does not establish that its object prototypes
are reset into the rooms or otherwise available to players. See the
[Unique builder questions](../zone-stories/UNIQUE.md#explicit-builder-questions-and-recommended-resolutions),
[#67200 down exit](../../../areas/wld/unique.wld#L6),
[#67300 return exit](../../../areas/wld/unique.wld#L17),
and [coverage row 328](ZONE_COVERAGE_REGISTER.csv#L328).

## Checkpoint 416: Undead Outpost boundaries and directed access

Active `unoutpst` has 121 rooms and 250 authored exits; the loader resolves
249 and drops the negative north target #98737 to -1. Its local weak graph has
one component. Two outbound records to Surface and Crypt have two matching
inbound and reverse records. The WLD contains one `F 10` field at #98815 and
no current fields. The reciprocal boundaries and connected graph do not
establish safe climbing, open gates, or accessible underground routes. See
the [Undead Outpost builder questions](../zone-stories/UNDEAD_OUTPOST.md#explicit-builder-questions-and-recommended-resolutions),
[negative exit source](../../../areas/wld/unoutpst.wld#L783),
[unresolved register](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L317),
[fall field](../../../areas/wld/unoutpst.wld#L2399),
[cross-area exits](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1180), and
[coverage row 329](ZONE_COVERAGE_REGISTER.csv#L329).

## Checkpoint 417: Vargan II routes and builder-question scope

Active Vargan II has 119 rooms and 249 resolving authored exits in one local
weak component. Its only cross-area edge is the reciprocal passage from #7800
to Vargan #2182. The source dossier separately records legacy door flags,
reset state, the underwater branch, and the upper no-ground rooms; static
connectivity does not establish access, safety, or return. See the [Vargan II
builder questions](../zone-stories/V2_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[active passage](../../../areas/wld/v2.wld#L15),
[cross-area edge](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L176),
[source audit](V2_AUDIT_REVIEW.md), and
[coverage row 330](ZONE_COVERAGE_REGISTER.csv#L330).

## Checkpoint 418: Valoisian Castle connectivity and source-only ingress

Active `val` has 174 rooms and 374 resolving authored exits in three local
weak components, the largest containing 157 rooms. Its active boundary is a
reciprocal Surface pair: #38575 south to #521073 and back. A separate north
exit from unregistered `surf.wld` room #205053 targets #38575; because that
source WLD is outside the active index, it is not live ingress. There are no
active unresolved targets, falls, or currents. Static routes do not establish
actor admission, key state, or safe return. See the [Valoisian Castle builder
questions](../zone-stories/VALOISIAN_CASTLE_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[active boundary](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L547),
[source-only exit](../../../areas/wld/surf.wld#L106081),
[Val target](../../../areas/wld/val.wld#L3902),
[Checkpoint 45 supplement](VALOISIAN_CASTLE_AUDIT_REVIEW.md#source-only-wld-reference-into-val),
and [coverage row 331](ZONE_COVERAGE_REGISTER.csv#L331).

## Checkpoint 419: Phantasmagoric Caverns routes and directed access

Active `valdrak` has 100 rooms and 234 resolving exits: 232 local plus two
reciprocal UnderDark boundary pairs, all in one weak component. The separate
`ud_map2.wld` exit from #1125389 down to #53100 is a source-only reference
outside the active world list, not another active entrance. No active exits
remain unresolved and the local map has no fall or current fields. These
records do not establish spell admission, single-file passage, safe access,
or return. See the [Phantasmagoric Caverns builder
questions](../zone-stories/VALDRAK_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[local boundary](../../../areas/wld/valdrak.wld#L19),
[UnderDark boundary audit](VALDRAK_AUDIT_REVIEW.md#active-map-and-underdark-boundaries),
[source-only exit](../../../areas/wld/ud_map2.wld#L66333), and
[coverage row 332](ZONE_COVERAGE_REGISTER.csv#L332).

## Checkpoint 420: Valinhaven routes and cave access

Active `valinhav` has 159 rooms and 320/320 resolving exits: 316 local exits,
one outbound UnderDark pair, and one outbound Dirk'nspire pair, with each
boundary reciprocal. All rooms are in one weak component; there are no
negative or positive unresolved targets, fall-chance fields, or current
fields. The river at #90800 and the cave at #90810 are distinct active routes.
Static connectivity does not establish actor admission, safe travel, or return
under every state. See the [Valinhaven builder questions](../zone-stories/VALINHAV_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[UnderDark exit](../../../areas/wld/valinhav.wld#L18),
[Dirk'nspire exit](../../../areas/wld/valinhav.wld#L207),
[cross-area records](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1116), and
[coverage row 333](ZONE_COVERAGE_REGISTER.csv#L333).

## Checkpoint 421: Valley of Crushk reciprocal and one-way routes

Active `valley_crushk` has 85 rooms and 182/182 resolving exits: 180 local
and two outbound edges. The #2006 edge is reciprocal with Pharr Valley; the
#2060 edge reaches Surface without a reverse exit. Two active inbounds enter
the Valley: Mini Zones #5806 to #2001 has no local return, while Pharr Valley
#40300 to #2006 is reciprocal. The local graph is one weak component; raw
directed traversal from #2001 reaches and returns from all 85 rooms. There are
no unresolved or negative targets, falls, or currents. These static paths do
not establish installed admission or safe return. See the [Valley of Crushk
builder questions](../zone-stories/VALLEY_CRUSHK_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[Checkpoint 172 route review](VALLEY_CRUSHK_AUDIT_REVIEW.md#active-map-incoming-leaves-and-exits),
[local exits](../../../areas/wld/valley_crushk.wld#L115),
[Surface exit](../../../areas/wld/valley_crushk.wld#L1075),
[cross-area records](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L33), and
[coverage row 334](ZONE_COVERAGE_REGISTER.csv#L334).

## Checkpoint 422: Vargan reciprocal boundaries and directed access

Active `vargan` has 94 rooms and 239/239 resolving exits: 236 local and three
outbound, each reciprocal with its inbound Surface or Vargan II edge. All 94
rooms form one weak component. There are no negative or positive unresolved
targets, fall-chance fields, or current fields. A separate non-overlapping
east exit from inactive `surf.wld` room #266321 targets #2100; it is source-only
and is not a fourth active route. Static reciprocity does not establish actor
admission or safe return, and wider outside-zone reachability remains open.
See the [Vargan builder questions](../zone-stories/VARGAN_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[Checkpoint 104 boundary and source-only review](VARGAN_AUDIT_REVIEW.md#active-graph-and-reciprocal-boundaries),
[local boundary](../../../areas/wld/vargan.wld#L13),
[Vargan II boundary](../../../areas/wld/vargan.wld#L1887),
[inactive approach](../../../areas/wld/surf.wld#L1391665), and
[coverage row 335](ZONE_COVERAGE_REGISTER.csv#L335).

## Checkpoint 423: Vecna's Tomb routes and directed access

Active `vecna` has 80 rooms and 187/187 resolving exits: 186 local and one
outbound Surface edge. The local graph has weak-component sizes 78, 1, and 1;
traversal from either active entry reaches 61 rooms and returns to 77 under
the authored directed exits. Surface #597408 to #130000 is reciprocated by
Vecna; Realm #14208 to #130021 is an incoming leaf without a Vecna return.
There are no negative or unresolved exits and no currents, but pit #130037
has an F100 fall declaration. The 320 `Duris3.wld` matches are inactive
source-only records. Static topology does not establish door state, actor
admission, portal arrival, or a safe return. See the [Vecna builder questions](../zone-stories/VECNAS_TOMB_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[Checkpoint 143 boundary review](VECNAS_TOMB_AUDIT_REVIEW.md#active-world-and-entry-boundaries),
[pit source](../../../areas/wld/vecna.wld#L938),
[inactive-map review](VECNAS_TOMB_AUDIT_REVIEW.md#inactive-map-matches), and
[coverage row 336](ZONE_COVERAGE_REGISTER.csv#L336).

## Checkpoint 424: Vehicles static layout and configured ferry travel

Physical coverage row 337 records 464/464 resolving WLD exits, no active
cross-area or unresolved exits, eight weak components (largest 169 rooms),
five fall declarations, and no current fields. Those static findings remain
separate from the seven ferry definitions in `src/world/ferryact.c`, whose
named external stops are turned into ship paths by `src/world/ferry.c`. The
configured stops do not by themselves establish that every path resolves,
that a ticket admits a particular actor, or that a safe return is available.
Keep the static graph intact and review ferry paths, route direction, ticket
admission, and hazard guidance separately. See the [Vehicles builder
questions](../zone-stories/VEHICLES_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[ferry definitions](../../../src/world/ferryact.c#L140),
[path construction](../../../src/world/ferry.c#L126), and
[coverage row 337](ZONE_COVERAGE_REGISTER.csv#L337).

## Checkpoint 425: Verspin local graph, entrances, and hazards

Physical coverage row 338 records 435/435 resolving WLD exits, two inbound
cross-zone records, one outbound record with one reciprocal pair, one weak
component spanning all 200 local rooms, 35 rooms with fall chance, and no
current fields. The source-backed approaches include the reciprocal Surface
highway connection at #28100 and a distinct one-way WH dispersal edge to
#28109. Keep those ingress roles separate; static resolution does not establish
ordinary admission, actual passage, pit access, or a safe return. The secret
circus route, keyed gladiator gates, deep waterways, and shaft hazards remain
separate local route conditions. See the [Verspin builder questions](../zone-stories/VERSPIN_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[local entrance](../../../areas/wld/verspin.wld#L1),
[Surface highway endpoint](../../../areas/wld/surface.wld#L653272),
[WH dispersal endpoint](../../../areas/wld/wh.wld#L12606), and
[coverage row 338](ZONE_COVERAGE_REGISTER.csv#L338).

## Checkpoint 426: Verzanan static room graph and absent ingress

Physical coverage row 339 has two resolving exits out of two records, one weak
component containing both rooms, and no cross-zone WLD ingress, egress, or
reverse pair. Room #2800 goes down to #3257 and #3257 returns up to #2800.
The external WLD source scan found no exit targeting either local room. The
zone file contains only its header and no placements. Keep this two-room
placeholder graph separate from the hundreds of external object reset and
shop references, which do not create room connectivity. Static source
resolution does not establish an accessible city route. See the
[Verzanan source review](../zone-stories/VERZANAN_STORY_REVIEW.md#source-inventory-and-world-shape),
[room source](../../../areas/wld/verz1.wld#L1),
[zone header](../../../areas/zon/verz1.zon#L1), and
[coverage row 339](ZONE_COVERAGE_REGISTER.csv#L339).


## Checkpoint 427: Voluntown directed reach and return paths

Physical coverage row 340 records 286/286 resolving exits: 285 internal and
one reciprocal boundary with IceCrag. In the local directed graph, following
all resolving internal WLD exits without applying door or actor state reaches
132 of 133 rooms from #142400. Room #142472 is the sole omission; it has no
incoming local exit and one south exit to #142471. All 133 rooms can reach
#142400, and the strongly connected component sizes are 132 and 1. This graph
reading does not apply the closed/locked entrance reset, item custody, traps,
fall state, resets, or runtime scripts, so it does not qualify installed
access, route safety, or player return. The reciprocal IceCrag pair remains
#142400 south ↔ #97132 north. The 268 non-overlapping Duris3 map approaches
remain source-only and cover all 133 rooms; the 264 additional records from
133 same-numbered Duris3 rooms overlap active Voluntown room VNUMs. Preserve
those map records under their existing inactive-source review. See the
[Voluntown audit](ROYAL_MAUSOLEUM_OF_CASTLE_ICECRAG_AUDIT_REVIEW.md#directed-paths-under-an-open-edge-abstraction),
[unnamed room](../../../areas/wld/Voluntown.wld#L1488),
[entry reset](../../../areas/zon/Voluntown.zon#L8), and
[coverage row 340](ZONE_COVERAGE_REGISTER.csv#L340).


## Checkpoint 428: Crystalspyre directed paths, falls, and approaches

Physical coverage row 341 records 281/281 resolving exits: 280 internal and
one outbound reciprocal Surface boundary. Raw directed traversal from the
Surface arrival at #37090 reaches and returns from all 100 local rooms in the
open-edge graph. The three explicit F33 fall fields are at #37065–#37067; four
D1 resets close the warming-hut door pairs at reset. Those states and runtime
conditions remain outside the graph count, so it is not a safe-travel or
installed-access result.

The only active approach is the Surface pair #523536 ↔ #37090. The all-file
inactive-WLD scan found one additional source-only approach, `surf.wld`
#210777 north to #37090, and no inactive room-number overlap. Keep that record
under the existing Surf candidate review, not as active ingress. Curated
source-only totals rise to 1,506 overall and 123 for Surf. See the
[Crystalspyre audit](CRYSTALSPYRE_MOUNTAINS_AUDIT_REVIEW.md#active-and-inactive-approaches),
[fall fields](../../../areas/wld/vrzawild.wld#L1761),
[closed hut reset](../../../areas/zon/vrzawild.zon#L8),
[inactive map approach](../../../areas/wld/surf.wld#L226153), and
[coverage row 341](ZONE_COVERAGE_REGISTER.csv#L341).

## Checkpoint 429: Wander staging graph and Llzazan exits

Physical coverage row 342 records 54/54 resolving WLD exits: 18 internal and
36 outbound to Llzazan. The six exits from each selector room #36601–#36606
are one-way and have no reciprocal record; #36602 repeats Llzazan target
#36267. No active or inactive WLD source targets a Wander room. In the local
directed graph #36600 reaches only itself and the six selector rooms; the
probability chain rooms #36607 and #36609–#36613 can reach #36600, while
#36608 is an exitless sink. The rooms form one weak component.

The root text identifies Wander as a random mobile-loading section for
Llzazan. Shared NPC wandering code checks exit validity, destination mobile
admission, and ACT_STAY_ZONE before movement. The local mobile act flags and
unflagged exits support the mobile-distribution interpretation; static paths
do not qualify reset odds, player entry, actual movement, or a safe return.
There are no falls, currents, or door resets. See the
[Wander audit](WANDER_AUDIT_REVIEW.md#active-wld-graph-directed-reach-and-approaches),
[staging room](../../../areas/wld/wander.wld#L1),
[probability chain](../../../areas/wld/wander.wld#L211),
[NPC movement](../../../src/mob/mobact.c#L8018), and
[coverage row 342](ZONE_COVERAGE_REGISTER.csv#L342).

## Checkpoint 430: Wellvolen directed graph and approaches

Physical coverage row 343 records all 93 Wemic exits as resolving: 92 local
and one outbound to Surface. All 47 rooms are in one weak component. Under
open-edge local traversal, both #93501 and #93514 reach every room, and every
room can return to either start inside the local graph. No negative or
unresolved targets, range-mismatch rooms, falls, currents, or local D reset
commands were found.

The active boundary has two inbound records and one outbound record, forming
one Surface reciprocal pair plus a one-way WH edge. Surface #572118 east
enters Wemic #93501, which exits west back to Surface #572118. WH #55614 south
enters Wemic #93514 with no reverse WH edge. The WH room is authored as a mob
dispersal location for the Bipplewizzlille potion quest; its edge should not be
presented as ordinary player ingress without qualification. See the
[Surface record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1492),
[Wemic record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1126),
[WH record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L765), and
[WH room prose](../../../areas/wld/wh.wld#L12606).

The 95-file inactive-WLD sweep found one source-only approach:
inactive surf.wld #229296 east to Wemic #93501, with no room-number overlap.
It is not active ingress and remains under the existing
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).
Curated source-only totals rise to 1,507 overall and 124 for Surf. See the
[Wellvolen audit](WEMIC_AUDIT_REVIEW.md#active-wld-graph-directed-reach-and-approaches),
[inactive Surf room](../../../areas/wld/surf.wld#L614548), and
[coverage row 343](ZONE_COVERAGE_REGISTER.csv#L343).

## Checkpoint 432: Winterhaven directed graph and approaches

Physical coverage row 345 has 1,558 resolving exits: 1,498 local and 60
outbound to 51 area stems. Five active exits enter Winterhaven from Heavens,
Nlakes, and Surface. Four inbound edges reciprocate a Winterhaven outbound;
Heavens #41 east to Winterhaven #55160 has no reverse edge. The five arrivals
are [Heavens row 15](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L15), [Nlakes row
870](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L870), and [Surface rows
1395-1397](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1395). There are seven weak
components with sizes 585, 5, 4, 2, 1, 1, and 1.

On open directed local WLD edges, the five arrival rooms reach 553/599 rooms.
From 587/599 rooms a local directed path reaches some outbound edge; 12 rooms
cannot reach one. The 60 outbound edges span 51 area stems. These measurements
do not establish ordinary player access, controller behavior, or safe return;
doors, keys, special handlers, teleports, and state remain unqualified. The
inactive-WLD review found no Winterhaven approach or overlapping room identity.
See the [Winterhaven bounded audit](WINTERHAVEN_AUDIT_REVIEW.md#active-wld-graph-directed-reach-and-approaches),
[coverage row 345](ZONE_COVERAGE_REGISTER.csv#L345), and
[source dossier](../zone-stories/WINTERHAVEN.md#access-availability-and-custom-dependencies).

## Checkpoint 433: Wildland Trails directed graph and legacy map candidates

Physical coverage row 346 has 99 rooms and 198 exits. The 196 local exits form
one weak component. Two active-source exits are positive unresolved records:
room #14601 south to #217245 and #14677 north to #215745, preserved in
[unresolved-exit rows 40 and 41](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L40). Both
targets exist in inactive Surf and Surface2011 maps. Their identities differ:
Surf describes a forested Quistery approach, while Surface2011 uses the same
VNUMs for sea rooms. The build omits those sources and prunes the exits.

Inactive `surf.wld` has the reverse source-only edges: #217245 north to
Wildland #14601 and #215745 south to #14677. They remain on [Surf candidate row
97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); Surface2011's alternate room
identity remains on [candidate row 98](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L98).
Neither candidate is a current world route. On open local edges, the area is
one weak component of 99 rooms; either source-only arrival reaches 70 rooms,
and every room can return to both arrival nodes. Doors, hidden exits, current
route ownership, player access, and safe return remain unqualified. See the
[Wildland Trails audit](WILDLAND_TRAILS_AUDIT_REVIEW.md#active-and-source-only-wld-graph),
[coverage row 346](ZONE_COVERAGE_REGISTER.csv#L346), and
[environmental story review](../zone-stories/WILDLAND_TRAILS_STORY_REVIEW.md).

## Checkpoint 435: Shadowfall directed graph, Surface pair, and portal routes

Physical coverage row 348 has 258 rooms and 632 resolving WLD exits. One
reciprocal boundary connects Surface room #620706 south with Shadowfall room
#18000 north. The local WLD edges form weak components of 255 and 3 rooms; the
smaller component is the collapsed tunnel #18000–#18002, separate from the
main map at #18003. From the Surface arrival, local WLD-only directed reach
covers the three tunnel rooms. The reset rifts at #18002 and #18003 create a
source-declared type-25 route between the components. The merchant-wagon
portal at campsite room #18031 targets interior #18256, whose WLD exit returns
to the campsite; #18257 is the wagon storage room. Adding those three reset
portal destinations to local WLD edges yields source-graph reach to 258/258
rooms from #18000. This does not qualify live object presence, player access,
safe travel, or return. [Wolfen boundary exit](../../../areas/wld/wolfen.wld#L10),
[Surface boundary exit](../../../areas/wld/surface.wld#L2534290),
[portal resets](../../../areas/zon/wolfen.zon#L25),
[portal prototypes](../../../areas/obj/wolfen.obj#L575),
[wagon portal](../../../areas/obj/wolfen.obj#L646), and
[coverage row 348](ZONE_COVERAGE_REGISTER.csv#L348). The two tunnel slab
switches and the closed cottage doors are recorded in the
[Shadowfall audit](WOLFEN_AUDIT_REVIEW.md#active-wld-boundary-and-local-graph).

## Checkpoint 440: Aeon Order Guildhall source-only map and exit

The unregistered `aeon_gh` WLD has ten rooms and 23 direction records: 22
local exits and one outward D4 from #29900 to #153244. Its local exits form one
weak component, and traversal from #29900 reaches all ten rooms. The target
exists in `Duris3.wld` as “The Ocean”; the Aeon source's presence does not make
that exit an active route because the component is absent from the full area
list. The existing candidate row reports no active WLD inbound edge to Aeon.
Target-map loader status and return are unqualified.

The room VNUMs overlap the separate unregistered `genesis` candidate, which
also defines #29900–#29909. The #29903 code assignment and candidate ownership
questions are recorded in the [Aeon audit](AEON_GH_AUDIT_REVIEW.md#authored-wld-graph-and-current-world-boundary)
and [identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-440-aeon-order-guildhall-and-genesis-vnum-collision).
Keep the source exit distinct from active-world connectivity and retain the
existing candidate row [Aeon 2](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L2).

## Checkpoint 441: Afterlife Guildhall map and Torg overlap

The unregistered `afterlife_gh` WLD has ten rooms and 19 direction records:
18 local targets and one upward exit from #29100 to #123813. The local graph
is one weak component, and traversal from #29100 reaches all ten local rooms.
The target exists in `Duris3.wld` as “Rolling Hills”; source presence alone
does not establish a loaded route or return. The current candidate records no
active WLD inbound edge to this component. Target loader membership, return,
and dynamic/code routes remain unqualified.

Each Afterlife room VNUM #29100–#29109 is also assigned to active `torg.wld`
with different room text and titles. That collision is a numeric source
conflict, not evidence that the authored maps connect or that either is a
replacement. Keep the Afterlife source graph separate from active reachability
and retain its existing [candidate row 3](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L3)
**RETAIN IN PLACE** pending owner resolution. See the [Afterlife audit](AFTERLIFE_GH_AUDIT_REVIEW.md#authored-map-and-torg-vnum-overlap),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-441-afterlife-guildhall-and-active-torg-identity),
and [coverage row 354](ZONE_COVERAGE_REGISTER.csv#L354).

## Checkpoint 442: Afterlife OP source graph and portal boundaries

The unregistered `afterlife_op` WLD has three rooms and five direction
records: four local edges and one D4 from #29112 to #157174. Its local edges
form one weak component, and traversal from #29110 reaches all three source
rooms. The target is “The Ocean” in `Duris3.wld`, which is absent from the full
`areas/AREA` list; the target record has no direct exit back to #29112. No
active WLD direction enters #29110–#29112, and the authored edge does not
establish active access or safe return.

The two local type-25 object portals store #29106 and #29110 as their target
VNUMs, although their descriptions name Khomani-Kan and the Goodie Continent.
Those values identify VNUMs assigned to Afterlife Guildhall, OP, and active
Torg rooms with different content. Record them as typed teleport references,
not as active WLD edges. Keep the existing [candidate row 4](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L4)
**RETAIN IN PLACE** while map ownership and teleport targets remain unresolved.
See the [Afterlife OP audit](AFTERLIFE_OP_AUDIT_REVIEW.md#local-graph-and-outward-boundary),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-442-afterlife-op-and-vnum-ownership-conflicts),
and [coverage row 355](ZONE_COVERAGE_REGISTER.csv#L355).


## Checkpoint 443: Afterlife OP2 three components and source-only boundaries

The unregistered afterlife_op2 WLD has nine rooms and fourteen direction
records: twelve local directions and two outward D4 exits. Its local graph
has three weak components of three rooms each; directed traversal from
#29120, #29123, or #29126 reaches all three rooms in its component. The D4s
target Duris3.wld #172709 (“The Ocean”) and #163943 (“Rolling Hills”).
Duris3 is absent from the full areas/AREA list, and neither target has a
direct return to its OP2 source. These edges do not establish active access
or safe return.

All nine room VNUMs also have different active Torg definitions. Fourteen
active Torg direction records target these same numeric IDs, but those paths
enter Torg-owned room records and are not OP2 ingress. No other active WLD
source targets this range; candidate row 5 remains at zero active candidate
inbound exits. Keep the authored source graph separate from active Torg
connectivity. See the [OP2 audit](AFTERLIFE_OP2_AUDIT_REVIEW.md#three-map-story-and-source-graph),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-443-afterlife-gc-op2-title-history-and-room-ownership),
[candidate row 5](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L5), and
[coverage row 356](ZONE_COVERAGE_REGISTER.csv#L356).

## Checkpoint 444: Alatorin object bank and typed travel targets

The unregistered alatorin_objects component defines no WLD rooms or
direction records, so it contributes no WLD graph or active inbound exit.
Its 24 type-25 objects encode command-triggered room targets, not WLD edges.
Twenty-three target values match active alatorin.obj; object #83434 differs:
the bank stores #83831 (Ancient Lava Tube), while the active prototype stores
#83188 (Atop the Mystic's Tower). Both target records are in active Alatorin.
The #83301 wormhole targets Astral Main #19721 and has no direct WLD return
to its reset room #83487.

Other type-25 targets lead to active Alatorin, Ard'gral, and Ethereal Main
rooms. Several Alatorin target VNUMs also have different definitions in the
unlisted Nexus56 source. Preserve these as typed object references with
competing source identities; do not count them as WLD connectivity or verified
runtime routes. See the [object-bank audit](ALATORIN_OBJECTS_AUDIT_REVIEW.md#story-content-and-room-targets),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-444-alatorin-object-bank-history-and-overlap),
[candidate row 6](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L6), and
[coverage row 357](ZONE_COVERAGE_REGISTER.csv#L357).

## Checkpoint 445: Alterian Mountains source graph and boundary identities

The source-only Mountain map has 80 rooms and 176 exit records. Five exits
store target `-1`; two positive exits lead to source-only Alterian Wilderness
records (#4920 west to #4919 and #4980 down to #4750). Its local graph has one
weak component. A directed sweep from #4920 reaches 75 rooms; all 80 have a
path back to #4920, while #4995–#4999 are not forward-reachable through the
Mountain file's local exits.

Three edges begin in Mountain-only source rooms and target shared Mining
numbers: #4969 west to #4968, #4981 down to #4924, and #4986 up to #4925.
Those targets also have separate Mountain WLD records, so the same integers
can name Mountain rooms or active Mining rooms. All 49 shared room records
differ. Active WLD has no inbound exit to Mountain-only VNUMs #4969–#4999;
the existing Mining audit records the three Mountain source-only approaches
as inactive-source edges. The candidate's active inbound count remains zero.

Wilderness #4919 is a third room identity at the same number as active
Mining #4919 and Mountain's target. Do not represent the shared integers as
proof of a live route, a merged map, or a safe return. See the [Mountain audit](ALTERIAN_MOUNTAINS_AUDIT_REVIEW.md#world-connectivity),
[duplicate/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-445-alterian-mountains-and-mining-identity),
[Mountain candidate row 7](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L7), and
[coverage row 358](ZONE_COVERAGE_REGISTER.csv#L358).

## Checkpoint 446: Alterian Wilderness source graph and boundary identities

The source-only Wilderness WLD has 130 rooms, 366 exit records, no negative
targets, and three weak components sized 119 (#4801–#4919), nine
(#4750–#4758), and two (#4759–#4760). Five positive exits leave its local
room VNUM set: #4750 up to source-only Mountain #4980, #4801 and #4918 south
to active Shadow Forest #6368, #4915 east to source-only Surf #229817, and
#4919 east to source-only Mountain #4920. The Mountain and Surf source edges
are reciprocal; #4919/#4920 also have different active Mining records.

The outward source-only approaches to active areas were already cataloged:
five distinct-origin exits to Mining, two to Shadow Forest, and four distinct
origins to active Tchan. Sixteen additional Tchan target matches originate
inside the overlapping Wilderness/Tchan room-number range #4801–#4809. All
remain source-side records. No active WLD exit targets a Wilderness-only
room identity, so candidate row 8 retains an active inbound count of zero.
The cave-to-lair portal pair is represented by object VNUMs and a compiled
`magic_pool` callback, not by WLD exits; runtime travel depends on loading the
objects and their destination rooms. This checkpoint adds no new curated
source-only exit into an active area. See the [Wilderness audit](ALTERIAN_WILDERNESS_AUDIT_REVIEW.md#world-connectivity-and-map-identity),
[candidate row 8](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L8), and
[coverage row 359](ZONE_COVERAGE_REGISTER.csv#L359).

## Checkpoint 447: AoD GH source graph and boundary identities

The source-only AoD map has 24 rooms and 41 direction records in three weak
components sized 10 (#29500–#29509), 4 (#29510–#29513), and 10
(#29514–#29523). It has no negative targets. Two down exits leave the local
VNUM set: #29500 to Duris3 #141442 and #29520 to #210208. The latter number
has distinct room definitions in source-only `surf.wld` and
`surface2011.wld`; Duris3 and both Surface maps are absent from the full
`areas/AREA` list. From #29514 only eight of ten rooms in its component are
forward-reachable by authored exits; #29516 and #29518 are not.

There is no active WLD exit into any AoD room ID and no source-map reverse
exit into its range. `genocide_hall`, `legion`, and `rf_op` contain 40
direction records whose target numbers overlap the AoD range, but these are
source-only maps with their own same-number room records. Their overlaps with
AoD are seven, sixteen, and three different room records respectively. Keep
the candidate inbound count at zero and do not count these numeric matches as
live routes. AoD's four type-25 objects store room values separately from
this WLD graph; their callbacks, loader identity, and safe return remain
unqualified. This checkpoint adds no curated source-only exit into an active
area. See the [AoD audit](AOD_GH_AUDIT_REVIEW.md#world-connectivity-and-reachability),
[candidate row 9](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L9), and
[coverage row 360](ZONE_COVERAGE_REGISTER.csv#L360).

## Checkpoint 448: Aopal source graph and current inactive-WLD recount

The source-only Aopal WLD has 44 rooms and 94 authored directions. Its weak
components have 43 rooms and one room (#43343). From #43301, local directed
traversal reaches 42 rooms and misses #43342 and #43343; #43342 reaches 43
rooms, and isolated #43343 has one north exit to Ancient Bridge #85606.
There are no negative targets. Three directions leave Aopal's local VNUM
set: #43301 west to active Shadow Forest #6368, #43323 east to the
source-only and identity-conflicted #230412, and #43343 north to active
Ancient Bridge #85606.

Four more directions from non-overlapping Aopal source rooms match active
Ship Yards targets: #43329 down to #43319, #43330 down to #43321, #43334 up
to #43324, and #43342 north to #43324. These targets also have Aopal room
records. Aopal and Ship Yards share 28 room VNUMs (#43301–#43328), and all
paired records differ. Of 62 Aopal target matches into Ship Yards' numeric
range, 58 start at overlapping room VNUMs; only the four directions above
start outside that shared range. Active WLD contains 30 numeric matches into
#43301–#43344, all resolving to active Ship Yards rooms: 29 are Ship Yards'
own directions and one is Malch's exit to #43320. No active exit targets an
Aopal-only room #43329–#43344; candidate row 10 retains active inbound zero.
See the [Aopal audit](AOPAL_AUDIT_REVIEW.md#world-connectivity-and-reachability),
[Ship Yards audit](SHIP_YARDS_AUDIT_REVIEW.md#inactive-map-identity-and-approaches),
[Shadow Forest audit](SHADOW_FOREST_AUDIT_REVIEW.md#inactive-wld-sweep),
[Ancient Bridge audit](ANCIENTB_AUDIT_REVIEW.md#active-wld-graph-and-inactive-approaches),
and [candidate row 10](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L10).

### Full inactive-WLD source-only approach recount

The older cumulative summaries are inconsistent: prior checkpoints state
4,529 overall / 278 Surf, 9,480 / 298, and 1,507 / 124. This checkpoint
recounts the current source snapshot
(`72ba3deff3bb30bde880018cdebe7c6db675b697`; the tracked area tree matches
the current checkout) with one explicit rule: count each direction from a
WLD stem absent from `areas/AREA` when its target VNUM is
defined by exactly one active WLD, and its origin VNUM is absent from that
same target WLD. This keeps map identity local to the destination area; for
example, Aopal #43301 west to Shadow Forest counts even though #43301 also
belongs to active Ship Yards. It counts 11,987 source-only direction records
from 41 of the 95 unlisted WLD sources, including 415 from `surf.wld`. No
eligible record had multiple active target owners. The 54 other unlisted
sources add none under this rule.

| Unlisted WLD source | Records | Unlisted WLD source | Records |
| --- | ---: | --- | ---: |
| `alterian_mountains` | 3 | `alterian_wilderness` | 11 |
| `aopal` | 6 | `asunwa` | 1 |
| `bb_gh` | 1 | `brass-old-1` | 7 |
| `brass-old-3` | 1 | `cabal` | 1 |
| `circlegh` | 1 | `dalvik` | 37 |
| `dark_gh` | 2 | `darkop` | 1 |
| `duris3` | 10,068 | `eoo` | 1 |
| `fists_gh` | 2 | `forofcon` | 5 |
| `genocide_gh` | 1 | `hostel` | 21 |
| `ic3` | 1 | `jungle` | 2 |
| `magetower` | 11 | `mavulsk` | 26 |
| `mentiri` | 10 | `newbiemaps` | 602 |
| `nexus56` | 23 | `nexus_1` | 6 |
| `oldmap8` | 13 | `rf_gh` | 1 |
| `surf` | 415 | `surface2011-temp` | 602 |
| `sylvandawn` | 18 | `theurgist` | 5 |
| `torr` | 1 | `ud_map0` | 6 |
| `ud_map1` | 7 | `ud_map2` | 27 |
| `underdark2011` | 10 | `underworld2` | 11 |
| `vodgh` | 1 | `waterplane` | 17 |
| `ygg` | 2 |  |  |

These are source records, not active ingress, unique destinations, player
reachability, or safe returns. The values 4,529, 9,480, and 1,507 remain
historical checkpoint snapshots and should not be combined as increments or
carried forward as current totals. The 11,987 / 415 recount supersedes them
under the rule above; it does not authorize any candidate move. See the
[Aopal audit](AOPAL_AUDIT_REVIEW.md#safe-disposition),
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97),
[coverage row 361](ZONE_COVERAGE_REGISTER.csv#L361), and
[attic plan](ATTIC_MIGRATION_PLAN.md#checkpoint-448-aopal-retained-with-source-only-approaches).

### Checkpoint 449: Astral placeholder has no candidate-specific route

The unlisted `astral.wld` contains one room, #1, and zero directions, so its
static graph is one weak component of one room with no local boundary,
fall-chance, or current records. Active `limbo.wld` is the sole active owner
of numeric #1; the source code explicitly defines #1 as `ROOM_LIMBO_VNUM`.
Nine directions from active Desert room #49790 target #1 and therefore resolve
to Limbo. Fourteen directions from eight other unlisted maps also target that
number, but those numeric records do not establish Astral as their destination.
The candidate-specific active inbound count remains zero.

This checkpoint adds no direction to the current inactive-WLD approach
recount: Astral has none. The 11,987 overall / 415 Surf totals remain unchanged
under the target-map rule above. These static records do not prove live
reachability or return safety. See the
[Astral audit](ASTRAL_AUDIT_REVIEW.md#static-graph-and-typed-dependencies),
[candidate row 11](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L11), and
[coverage row 362](ZONE_COVERAGE_REGISTER.csv#L362).

### Checkpoint 450: Asunwa Jaguar Lair boundaries and hazards

Unregistered `asunwa` contains 108 rooms and 267 authored directions: 266
local and one outbound from #35300 north to active Surface #604903. The local
graph is one weak component; 106 rooms are forward-reachable from the
entrance, while rare-load rooms #35406/#35407 are not. All 108 rooms have an
authored local return path to #35300. Six rooms have fall chances and fourteen
have current settings. Eight flagged/locked/secret exits each match a 100%
ZON door reset. These static paths do not qualify door state, hazards, live
access, or safe return.

No active WLD direction targets Asunwa's #35300–#35407 rooms. The separate
source-only `surf.wld` record #247317 south to #35300 remains one inactive
approach, not active ingress. The already-counted one-way outbound to Surface
does not change the current 11,987 eligible source-only directions overall /
415 from Surf. See the [Asunwa audit](ASUNWA_AUDIT_REVIEW.md#static-graph-hazards-and-boundaries),
[candidate row 12](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L12), and
[coverage row 363](ZONE_COVERAGE_REGISTER.csv#L363).

### Checkpoint 451: Bastard Die Hall room ownership and source boundaries

Unregistered `bastard` has five rooms and nine authored directions: eight
within the five-room VNUM set and one up from #30200 to #1120721 in unregistered
`ud_map2`. Its local graph is one weak component; all five rooms are
forward-reachable from #30200 and have authored local returns. There are no
fall or current settings. The `ud_map2` target has no authored exit back to
#30200, so this source-only boundary has no WLD return.

The active `underworld3.wld` defines #30200–#30204 as different dwarven
tunnels. Its numeric ownership means same-number active exits resolve to
Underworld3, not into the `bastard` candidate. Unregistered `sunhall.wld` also
defines the same five numbers with different rooms. Candidate-specific active
inbound remains zero despite those number overlaps. The single outbound
targets source-only `ud_map2`, so this checkpoint adds no direction to the
eligible active-destination recount; the 11,987 overall / 415 Surf totals
remain unchanged. See the [Bastard audit](BASTARD_AUDIT_REVIEW.md#static-graph-and-numeric-room-ownership),
[candidate row 13](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L13),
[Sunhall candidate row 96](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L96),
[ud_map2 candidate row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114), and
[coverage row 364](ZONE_COVERAGE_REGISTER.csv#L364).

### Checkpoint 452: BB GH static graph and source boundaries

Unregistered bb_gh has 44 rooms and 64 authored directions, of which 60 stay
within its numeric room set. Those local directions form five weak
components (10, 10, 10, 10, and 4 rooms); the entrance #30300 reaches only
10/44 by authored local exits. Four reset type-25 portal pairs link the
entrance-side rooms to the four side components in source data, but they are
not WLD edges and do not establish live travel. No fall-chance or current
record was found.

The four WLD boundaries are #30300 up to ud_map2 #1126542, #30320 up to
active tharnrifts #111255, #30330 up to unregistered Duris3 #123743, and
#30340 up to unregistered Duris3 #140954. ud_map2 #1126542 has the numeric
return to #30300; because #30300 is also defined by unregistered vodgh and
venom, this remains an ambiguous source-only pair. Candidate-specific active
inbound is zero. Only the tharnrifts target resolves in the active full-world
WLD set, and the #30320-to-#111255 approach was already included in the
eligible inactive-WLD recount at Checkpoint 386. Total remains 11,987
directions overall / 415 from Surf. See the
[BB GH audit](BB_GH_AUDIT_REVIEW.md#static-wld-graph-and-room-ownership),
[ud_map2 candidate row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114),
[Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36),
[coverage row 365](ZONE_COVERAGE_REGISTER.csv#L365), and
[Tharnadia Rifts audit](THARNRIFTS_AUDIT_REVIEW.md#inactive-wld-sources-and-map-identity).

### Checkpoint 453: BGTK has no WLD component

The BGTK candidate contains only bgtk.shp, so it adds no authored WLD room,
direction, or inactive-map approach. The eligible inactive-WLD recount stays
11,987 overall / 415 from Surf. Its separate shop-loader coupling is
documented in the [BGTK audit](BGTK_AUDIT_REVIEW.md#default-full-mode-loader-path-and-risk)
and does not change the connectivity totals.

### Checkpoint 454: Brass Old 1 predecessor graph and boundary

The inactive `brass-old-1` WLD has 357 rooms and 821/821 resolving exits:
820 local and one outbound from old room #139100 south to active Fire #25455.
The local graph is one weak component. Authored directed traversal from
#139100 reaches 354/357; #139265, #139266, and #139347 are not reached. There
are no fall-chance or current fields. The historical Brass AREA span
#139000–#139399 puts 57 old rooms and 131 old exits outside that span; the
−100 VNUM crosswalk maps those records into active Brass's current range.

The old boundary maps to active Brass #139000 south to #25455 and the
existing reciprocal Fire boundary. Although 598 active WLD exits target
numeric IDs also present in the archived file, all 598 target current active
Brass room records; none targets the 100 old-only numeric IDs. Thus active
candidate ingress remains zero, and the predecessor edge is not another
inactive-WLD approach. The curated total remains 11,987 overall / 415 from
Surf. This resolves the earlier same-number uncertainty recorded in the
[Plane of Fire review](PLANE_OF_FIRE_AUDIT_REVIEW.md#inactive-wld-approaches-and-candidate-records).
See the [Brass Old 1 audit](BRASS_OLD_1_AUDIT_REVIEW.md#static-wld-graph-and-boundary),
[candidate row 17](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17), and
[coverage row 368](ZONE_COVERAGE_REGISTER.csv#L368).

### Checkpoint 455: Brass Old 2's unresolved active destinations

The unlisted `brass-old-2` WLD contains 105 rooms and 243 exits; all targets
stay inside the source. Its local graph is one weak component, and authored
directed traversal from #25001 reaches all 105 rooms. It has no outbound
boundary, negative target, fall-chance field, or current field. These are
source-graph facts; doors, resets, live access, and return behavior were not
qualified.

Seven active records target Old 2 room #25087: six directions from Potions
#30705/#30706 and Realm #14210 D9 southeast. Because the target appears only
in the unlisted file, the current unresolved-exit register classifies all
seven as positive unresolved records. They are not loaded Old 2 entrances,
and Old 2 has no reciprocal outbound WLD edge. Keep all seven references on
candidate row 18 and preserve the current active route sources pending their
owners' decisions. Curated inactive-WLD totals remain 11,987 overall / 415
from Surf. See the [Brass Old 2 audit](BRASS_OLD_2_AUDIT_REVIEW.md#wld-graph-and-current-inbound-references),
[Potions review](POTIONS_AUDIT_REVIEW.md#active-graph-and-authored-access),
[Realm review](FAERIE_REALM_AUDIT_REVIEW.md#active-wld-graph-and-reset-door-reach),
[candidate row 18](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L18), and
[coverage row 369](ZONE_COVERAGE_REGISTER.csv#L369).

### Checkpoint 456: Brass Old 3 connectivity and the Old 2 link

The unregistered former plane_fire_two source has 170 rooms and 362 authored
directions: 360 local and two cross-zone. Its local map is one weak component,
and authored directed traversal from #25201 reaches all 170 rooms. No target is
negative, no room is outside the declared range, and no fall-chance or current
field was found.

The two outbound records are Old 3 #25201 D2 south to current Fire #25455 and
Old 3 #25341 D0 north to unregistered Old 2 #25001. The first is the
source-only approach already counted in the Plane of Fire review; Fire #25455
has no return to #25201. The second is an inactive-to-inactive link from the
royal palace gates into Old 2’s “Entry Way,” with no reciprocal Old 2 edge.
Neither is active ingress into Old 3, and no active WLD record targets Old 3.
Candidate active inbound remains zero. The new Old 3→Old 2 edge therefore adds
no active ingress or eligible inactive-WLD approach and the curated total stays
11,987 overall / 415 Surf.

The WLD edge sits alongside a two-way source-only reset dependency: Old 3 has
30 M resets for Old 2 mobile #25000, while Old 2 has 66 M/E/G resets for Old 3
prototypes. These are not travel edges. Keep the map and reset links distinct
when deciding future ownership. See the [Old 3 audit](BRASS_OLD_3_AUDIT_REVIEW.md#wld-graph-and-source-only-routes),
[Old 2 audit](BRASS_OLD_2_AUDIT_REVIEW.md#wld-graph-and-current-inbound-references),
[candidate row 19](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L19), and
[coverage row 370](ZONE_COVERAGE_REGISTER.csv#L370).

### Checkpoint 457: BSGHOP local graph and Harrow VNUM overlap

The unregistered BSGHOP WLD has four rooms and nine authored directions: eight local exits and one
negative target from #29440 D4 up to −1. Its local graph is one weak component, and directed
traversal from #29440 reaches all four rooms. Every room's WLD zone field is 1 although the room
VNUMs are in zone 294; its ZON header also names zone 1 and ends at #29443.

There are no cross-area exits, positive unresolved targets, or BSGHOP-specific active inbound
exits. All source targets are local to the four-room set except the negative target. Harrow's
current active rooms use the same VNUMs for different rooms; the eight source-only numeric target
matches to Harrow therefore remain identity-ambiguous and do not establish access to BSGHOP. The
source-only reset placements also target reused room #29440, but are not travel edges. No new
inactive-WLD approach is added, so the curated totals remain 11,987 overall / 415 from Surf. See the
[BSGHOP audit](BSGHOP_AUDIT_REVIEW.md#static-wld-graph-and-zone-number-discrepancy),
[Harrow audit](HARROW_AUDIT_REVIEW.md#unregistered-wld-target-matches),
[candidate row 20](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L20), and
[coverage row 371](ZONE_COVERAGE_REGISTER.csv#L371).

### Checkpoint 458: Cabal Guildhall graph and Torg-owned source VNUMs

The unregistered Cabal WLD has 13 rooms and 24 authored directions: 22 local and two outbound
cross-zone exits. Its local graph has two weak components (#29000–#29009, ten rooms; and
#29010–#29012, three rooms), with ten rooms reachable by directed traversal from #29000. All 13
room zone fields say 1 while their VNUMs belong numerically to zone 290.

Room #29001 exits up to active Tharn Rifts #111079, and room #29012 exits up to unregistered
Duris3 #156242. There is no reciprocal exit into Cabal from either target. The first source room
VNUM is already owned by active Torg, so the Tharn Rifts match remains excluded from source-only
approaches; the second edge is inactive-to-inactive. Neither creates active ingress into Cabal, and
the eligible inactive-WLD totals remain 11,987 overall / 415 Surf. Cabal's type-25 object targets
may connect its two WLD components through item commands, but they are not authored WLD edges. See
the [Cabal Guildhall audit](CABAL_GUILDHALL_AUDIT_REVIEW.md#static-wld-graph-and-outbound-records),
[Tharn Rifts audit](THARNRIFTS_AUDIT_REVIEW.md#inactive-wld-sources-and-map-identity),
[candidate row 21](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L21), and
[coverage row 372](ZONE_COVERAGE_REGISTER.csv#L372).

### Checkpoint 459: Cabal Hall graph and Underworld3 VNUM overlap

The unregistered Cabal Hall WLD has ten rooms and 19 authored directions: 18 local and one outbound
exit from #30100 up to Duris3 #153434 (“Grassy Plains”). The local graph is one weak component, and
directed traversal from #30100 reaches all ten candidate rooms. All room zone fields say 301.

All ten Cabal Hall room VNUMs #30100–#30109 resolve to different active Underworld3 room identities
in the full world. Inactive `nah` and `shadow_hall` WLDs also use overlapping room numbers. These
numeric matches remain distinct source maps; candidate-specific active inbound exits remain zero.
The outbound target exists in unregistered Duris3 and has no return exit to Cabal Hall. The edge adds
no active ingress or eligible inactive-WLD approach; totals remain 11,987 overall / 415 Surf. See the
[Cabal Hall audit](CABAL_HALL_AUDIT_REVIEW.md#static-wld-graph-and-boundary-exit),
[Underworld audit](UNDERWORLD_AUDIT_REVIEW.md#underworld3-room-overlap-and-the-compiled-inn-binding),
[candidate row 22](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L22), and
[coverage row 373](ZONE_COVERAGE_REGISTER.csv#L373).

### Checkpoint 460: Casino graph and Hall VNUM overlap

The unregistered Casino WLD has 15 rooms and 38 authored directions, all targeting its local
#77801–#77815 set. Its local graph is one weak component, and directed traversal from #77801 reaches
all 15 rooms. Every room's zone field is 778; no outbound cross-zone, negative, unresolved, fall, or
current record was found.

Active Hall of the Ancients currently owns every one of those room VNUMs with different identities
and zone field 777. All 38 source exit targets numerically resolve to Hall rooms, but this does not
load the Casino source. Hall has 23 authored exit records targeting the same number range, all for
Hall's map; candidate-specific active inbound remains zero. Hall object #77714 also stores #77813 as
its destination, which is Hall's active waterfall room. This source-only number overlap adds no
eligible inactive-WLD approach; curated totals remain 11,987 overall / 415 from Surf. See the
[Casino audit](CASINO_AUDIT_REVIEW.md#active-hall-ownership-of-all-casino-room-vnums),
[Hall story map](../zone-stories/THE_HALL_OF_THE_ANCIENTS.md),
[candidate row 23](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L23), and
[coverage row 374](ZONE_COVERAGE_REGISTER.csv#L374).

### Checkpoint 461: CBL Outpost 2 graph, VNUM, and mobile overlap

The unregistered `cbl_outpost2` WLD has five rooms and eight authored directions. All eight target
its local #1000–#1004 set; the graph is one weak component, with directed reach from #1000 covering
all five rooms. No outbound cross-zone, negative, fall, or current record was found. The WLD and
ZON use raw zone number 1000, while the rooms fall outside numeric VNUM/100 band 10; all five rooms
and all eight originating exits are flagged by the range checks.

No active WLD room owns #1000–#1004, and no active WLD exit targets them. Therefore the eight
positive candidate-local targets are unresolved in the active loader, candidate-specific active
inbound remains zero, and no new source-only approach is added. The inactive `Monsteri` copy uses
the same room targets. Separately, active Heavens defines mobile #1000–#1003 for different
prototypes; both unregistered ZON files reset the golem identities into #1000. This typed mobile
collision is not a travel edge. Curated inactive-WLD totals remain 11,987 overall / 415 Surf. See the
[CBL Outpost 2 audit](CBL_OUTPOST2_AUDIT_REVIEW.md#static-wld-graph-and-range-discrepancy),
[active CBL Hall audit](CBL_HALL_AUDIT_REVIEW.md#title-history-and-duplicate-content-review),
[candidate row 24](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L24), and
[coverage row 375](ZONE_COVERAGE_REGISTER.csv#L375).

### Checkpoint 462: BattleRager Hall graph and Newbie VNUM overlap

The unregistered `cbr_hall` WLD has ten rooms and 19 authored directions: 18 local and one D4/up
exit from #29300 to Duris3 #152260 (“Grassy Plains”). The local graph is one weak component, and
directed traversal from #29300 reaches all ten rooms. The local room band and zone fields are 293.
The Duris3 target is in an unregistered WLD and has no exit back to #29300.

Every local target number #29300–#29309 resolves in the full active world to a different Newbie
room identity. Active Newbie has nine authored exits into that numeric range; they are Newbie routes,
not candidate-specific ingress. Candidate-specific active inbound remains zero. Eighteen directions
are loader-resolved to active Newbie rooms; the one Duris3 target is positive unresolved. The
inactive-to-inactive edge adds no eligible approach into an active area; curated totals stay 11,987
overall / 415 Surf. See the
[BattleRager Hall audit](CBR_HALL_AUDIT_REVIEW.md#static-wld-graph-and-duris3-boundary),
[active Newbie source](../../../areas/wld/newbie.wld#L1975),
[candidate row 25](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L25), and
[coverage row 376](ZONE_COVERAGE_REGISTER.csv#L376).

### Checkpoint 463: Yggdrasil Outpost graph, range, and Duris3 boundary

The unregistered `circle_op` WLD has three rooms and five authored directions:
four local directions and one D4/up exit from #45410 to Duris3 #154462,
“The Ocean.” Its local graph is one weak component, and directed traversal
from #45410 reaches all three rooms. Duris3 has no authored return to #45410.

The ZON header and all three room fields use raw zone number 1 although their
VNUMs fall in the #454 band under the current VNUM/100 check. No full-loader WLD
defines #45410–#45412, and no active WLD exit targets those IDs. All five
positive targets therefore remain unresolved in the active world, including
the four local targets duplicated in unregistered `circlegh`; candidate-
specific active inbound remains zero. The portal object's #45407 destination
is a typed object reference into the `circlegh` source, not a WLD exit.
Metrics are five source directions, zero loader-resolved, one outbound
cross-zone, five positive unresolved, three rooms and five source exits with a
zone-range mismatch, one local component of three, and one unregistered WLD
target reference to Duris3. This does not add active ingress; curated
source-only totals remain 11,987 overall / 415 from Surf. See the
[`circle_op` audit](CIRCLE_OP_AUDIT_REVIEW.md#static-wld-graph-and-zone-field-discrepancy),
[Duris3 target](../../../areas/wld/Duris3.wld#L1143703),
[candidate row 26](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L26), and
[coverage row 377](ZONE_COVERAGE_REGISTER.csv#L377).

### Checkpoint 464: Circle Guildhall graph, portals, and Tharnadia boundary

The unregistered `circlegh` WLD has 16 rooms and 29 authored directions across
three weak components of 10, 3, and 3 rooms. Directed reach from #45400, #45410,
and #45415 covers each respective component. Twenty-six directions target
candidate-local room IDs; one D4/up exit from #45410 targets inactive Duris3
#154462; one D4/up exit from #45417 targets active Tharnrifts #110072; and
#45400 D4/up has the negative destination -1.

All 16 room fields use raw zone number 9 although their VNUMs fall in the #454
band under the current VNUM/100 check; all 29 directions originate in those
range-mismatched rooms. No full-loader WLD owns or targets candidate room IDs,
so the 26 local exits are positive unresolved targets and candidate-specific
active inbound remains zero. One positive target resolves to active Tharnrifts,
and #154462 remains positive unresolved in Duris3. The #45417 to #110072
source-only approach was already counted at Checkpoint 386; preserve that
global total. Metrics are 29 directions, one loader-resolved, two positive
cross-zone out, 27 positive unresolved, one negative, three local components
(largest ten), and one unregistered WLD target in Duris3. No fall/current
rooms were found. Curated totals remain 11,987 overall / 415 from Surf. See the
[`circlegh` audit](CIRCLEGH_AUDIT_REVIEW.md#static-wld-graph-and-boundaries),
the existing [Checkpoint 386 recount](WORLD_CONNECTIVITY.md#checkpoint-386-tharnadia-rifts-directed-reach-and-inactive-approaches),
[active Tharnrifts target](../../../areas/wld/tharnrifts.wld#L19),
[candidate row 27](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L27), and
[coverage row 378](ZONE_COVERAGE_REGISTER.csv#L378).

### Checkpoint 465: `cript` has no WLD graph

The unregistered `cript` candidate consists only of a zero-byte
`areas/shp/cript.shp`; it defines no room VNUMs, WLD records, or authored
directions. Its source-exit, inbound, range, component, fall, and current
metrics are therefore zero, with no candidate travel graph or source-only
approach to add. The non-mini shop fallback globs `areas/shp/*.shp`, but this
empty file contributes no shop record or travel edge. It does not affect the
curated inactive-WLD totals (11,987 overall / 415 from Surf). See the
[`cript` audit](CRIPT_AUDIT_REVIEW.md#static-loader-path-and-dependency-scope),
[candidate row 28](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L28), and
[coverage row 379](ZONE_COVERAGE_REGISTER.csv#L379).

### Checkpoint 466: Dalvik Dockyard source graph

The unregistered `dalvik.wld` has 476 rooms and 1,274 authored directions:
1,273 target locally defined candidate rooms, and room #298 south targets
active Surface #635660. There are no negative or positive unresolved targets,
range mismatches, or candidate-source targets in other unregistered WLD files.
The local graph has 34 weak components, largest 434; plain directed traversal
from #1 reaches 411 rooms, and 427 have a raw directed return path to #1. Seven
rooms have fall fields and six have current fields (#250–#255 overlap both;
#266 has a fall field). These source traversals do not establish loaded or safe
travel.

One active Thetis direction, #38095 north → #104, numerically enters the
candidate range. Room #104 also exists in inactive `mavulsk.wld` with a
different title, so Thetis remains unresolved and this is not confirmed Dalvik
ingress. The #298 south → #635660 source-only approach was already included in
Checkpoint 373; do not count it again. Curated inactive-WLD totals remain
11,987 overall / 415 from Surf. See the [Dalvik audit](DALVIK_AUDIT_REVIEW.md),
[Thetis unresolved-exit row 209](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L209),
[Surface audit](SURFACE_AUDIT_REVIEW.md#inactive-wld-surface-approaches),
[candidate row 29](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L29), and
[coverage row 380](ZONE_COVERAGE_REGISTER.csv#L380).

### Checkpoint 467: Guildhall of the Dark

The unregistered `dark.wld` has ten rooms and 37 authored directions: 36 local
targets in #29400–#29409, plus one up exit from #29400 to inactive Duris3
#156726 (“Grassy Plains”). All ten rooms form one weak component, and plain
directed traversal from #29400 reaches all ten. The source has no negative
targets, range mismatch, fall field, or current field. The external Duris3
target has no reverse exit to #29400 and remains unresolved in the active
loader.

Every local target VNUM also belongs to active Harrow’s room range, but all ten
Dark room blocks differ from Harrow’s. The 36 records are local to Dark’s own
source graph and already appear as numeric identity matches in the [Harrow
review](HARROW_AUDIT_REVIEW.md#unregistered-wld-target-matches); do not count
them as active Dark ingress or new non-overlapping source-only approaches.
Harrow’s active Surface connections remain attached to Harrow room #29400.
Curated inactive-WLD totals remain 11,987 overall / 415 from Surf. See the
[Dark audit](DARK_AUDIT_REVIEW.md), [candidate row 30](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L30),
and [coverage row 381](ZONE_COVERAGE_REGISTER.csv#L381).

### Checkpoint 468: The Dark Guildhall

The unregistered `dark_gh.wld` contains 22 rooms and 38 authored directions.
Thirty-four directions target rooms in the candidate file, forming five weak
components of sizes 10, 3, 3, 3, and 3. Plain directed traversal from #29200
reaches ten rooms. The remaining twelve rooms are in four separate three-room
pieces. The candidate has four outward directions, no negative target, range
mismatch, fall field, or current field.

Twenty-one candidate room numbers also belong to active `newbie.wld`, and the
complete same-number room blocks differ. Thirty-three candidate-local targets
currently resolve numerically to Newbie; the one target to the candidate-unique
#29200 does not resolve in the active room table. The other current active-table
resolution is #29230 up to active Surface Keeps #123619. The other external
target numbers #153032, #212674, and #163940 do not resolve in the active area
set. #153032 and #163940 occur in inactive Duris3; #212674 occurs in inactive
`surf` and `surface2011`, whose same-number rooms have different descriptions.
Target #123619 also occurs in inactive Duris3. None of the four target rooms has
a reverse exit to its Dark source room.

Fifty active Newbie directions target numbers reused by Dark, but those numbers
name Newbie's active room blocks; none targets Dark-unique #29200. They are not
active Dark ingress. The #29230 source identity is also shared with active
Newbie, so its one-way numeric edge to Surface Keeps does not add an eligible
non-overlapping inactive-WLD approach. Curated inactive-WLD totals remain
11,987 overall / 415 from Surf. See the [Dark Guildhall audit](DARK_GH_AUDIT_REVIEW.md),
[32 builder questions](DARK_GH_BUILDER_QUESTIONS.md), [candidate row 31](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L31),
and [coverage row 382](ZONE_COVERAGE_REGISTER.csv#L382).

### Checkpoint 469: Dark Outpost

The unregistered `darkop.wld` contains 19 rooms and 36 authored directions.
Thirty-two directions are local to the candidate; four point outward. The local
graph has five weak components of sizes 4, 4, 4, 4, and 3. Plain directed
traversal from #29410 reaches only three rooms. The source has no negative
target, range mismatch, fall field, or current field. Every WLD room zone field
and the ZON header say 1 although the room VNUMs are in zone 294.

All 19 Dark Outpost room VNUMs also belong to active Harrow, and all 19
same-number room blocks differ. The 32 local directions resolve numerically to
Harrow under the current active WLD set. Harrow itself has 44 direction records
targeting these reused room numbers; those remain Harrow routes to active Harrow
blocks, not Dark Outpost ingress.

The four outward exits have no reverse in their tracked target rooms. #29410
up → #123112 resolves to active Surface Keeps “The Walls of Alatorin,” with an
inactive Duris3 “The Ocean” definition at the same VNUM. #29420 up → #163278
and #29430 up → #177358 point to inactive Duris3 ocean rooms. #29445 up →
#182631 has inactive Duris3 ocean and Oldmap8 drylands definitions. The active
table resolves 33/36 exits (the 32 Harrow-local targets plus #123112); the
three other targets remain unresolved there. Because every Dark Outpost source
room number is already owned by Harrow, no distinct inactive-WLD approach is
added. See the [Dark Outpost audit](DARKOP_AUDIT_REVIEW.md), [32 builder
questions](DARKOP_BUILDER_QUESTIONS.md), [candidate row 32](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L32),
and [coverage row 383](ZONE_COVERAGE_REGISTER.csv#L383).

### Checkpoint 470: `ddguild` shop-source connectivity

`ddguild` is a one-record SHP-only component: it defines no rooms, directions,
or WLD approach, so it adds no graph nodes or eligible inactive-WLD exits.
Its keeper #35300, product #35301, and configured room #35303 numerically
resolve only to unregistered Asunwa source definitions. The full `areas/AREA`
manifest comments Asunwa out; the shop fields do not establish an active
approach or shop location. Curated WLD totals remain 11,987 overall / 415 from
Surf. See the [audit](DDGUILD_AUDIT_REVIEW.md), [18 builder
questions](DDGUILD_BUILDER_QUESTIONS.md), [candidate row 33](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L33),
and [coverage row 384](ZONE_COVERAGE_REGISTER.csv#L384).

### Checkpoint 471: Empty `drag` QST/SHP connectivity

Both current `drag` files are zero bytes and there is no WLD, room VNUM, or
exit record from which to form a graph. The source register records zero active
inbound WLD exits. The SHP fallback can open the empty file, but receives no
shop record or approach. Curated WLD totals remain 11,987 overall / 415 from
Surf. See the [audit](DRAG_AUDIT_REVIEW.md), [11 builder
questions](DRAG_BUILDER_QUESTIONS.md), [candidate row 34](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L34),
and [coverage row 385](ZONE_COVERAGE_REGISTER.csv#L385).

### Checkpoint 472: `dranquest` two-room map

The unregistered WLD is a reciprocal pair: #12601 down to #12625 and #12625
up to #12601. Both rooms are in one weak component and forward-reachable from
#12601; neither has an active full-area WLD definition, and there is no active
inbound approach or external boundary. The remaining five MOBs and seventeen
OBJs have no ZON reset commands in this candidate. Curated totals remain
11,987 overall / 415 from Surf. See the [audit](DRANQUEST_AUDIT_REVIEW.md),
[18 builder questions](DRANQUEST_BUILDER_QUESTIONS.md), [candidate row 35](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L35),
and [coverage row 386](ZONE_COVERAGE_REGISTER.csv#L386).

### Checkpoint 473: Duris3 WLD graph and map identity

The unregistered `Duris3.wld` contains 90,000 rooms and 360,000 directions;
all 360,000 targets are within that WLD’s own room-VNUM set. Its candidate
graph is one weak component, with all 90,000 rooms forward-reachable from
#100000. It has no authored external target. The 26 active WLD records from
14 areas into 24 Duris3-only target VNUMs remain unresolved in the full-area
table; their combined candidate-graph reach is not current in-game access.

Duris3 shares 22,064 room VNUMs with 27 active WLD files, but no paired
normalized title or complete room block matches. Another 88,256 Duris3 target
numbers occur in active WLD tables; those are raw numeric matches within the
Duris3 graph, not proof that its local edges enter the active map identities.
No new source-only approach count is added from that broad set. Curated totals
remain 11,987 overall / 415 from Surf. See the [Duris3 audit](DURIS3_AUDIT_REVIEW.md),
[20 builder questions](DURIS3_BUILDER_QUESTIONS.md), [candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36),
and [coverage row 386](ZONE_COVERAGE_REGISTER.csv#L387).

### Checkpoint 474: EOO Valhalla connectivity and typed boundaries

EOO has ten rooms and 19 WLD directions: 18 targets stay in #29200–#29209;
the one external edge is #29200 up to inactive Duris3 #179748, which has no
return to EOO. The ten-room source graph is one weak component and is
forward-reachable from #29200. There is no active inbound edge to EOO-only
#29200. Twenty-two active Newbie exits target the other nine shared numbers,
but those are Newbie-to-Newbie edges, not EOO ingress. The previously reviewed
inactive EOO #29200 north exit to Ailvio #29201 remains among the three
source-only approaches already counted; no new approach is added.

Every EOO room number also occurs in both dark_gh and genocide_gh; nine also
occur in active Newbie. Curated totals remain 11,987 overall / 415 from Surf.
The candidate also has one SHP record under #29204; the conditional non-mini
shop fallback can glob eoo.shp despite EOO’s missing AREA entry. See the
[EOO audit](EOO_AUDIT_REVIEW.md), [24 builder questions](EOO_BUILDER_QUESTIONS.md),
[candidate row 37](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L37), and
[coverage row 387](ZONE_COVERAGE_REGISTER.csv#L387).

### Checkpoint 475: eoo_op Muspelheim map and portal targets

The unregistered eoo_op WLD has three rooms and five directions: four reciprocal
local directions form one weak component, and #29210 up points to inactive
Duris3 #123225 (“The Ocean”) without a return. All three candidate room VNUMs
are also defined by active Newbie and inactive dark_gh with different complete
room blocks. The current active table resolves the four candidate-local
numeric targets to Newbie identities. Seven active Newbie directions target
the shared #29210–#29212 numbers; they remain Newbie-to-Newbie edges, so
candidate inbound stays zero and no approach is added to curated totals.

The four type-25 portals store #29206, #29222, #29212, and #29225 while their
descriptions name EOO/Valhalla, the Undead Continent, Khomani-Kan, and Goodie
Continent. Each requires CMD_ENTER (7) and stores value[2] = -1; current code
only decrements positive charges. Active Newbie owns all four target numbers,
while inactive EOO and eoo_op2 records provide competing map identities for
three of them. eoo.zon also imports #29211–#29213 from this candidate, so
preserve those object dependencies without treating the active same-number
Newbie objects as the portals. Curated totals remain 11,987 overall / 415
from Surf. See the [audit](EOO_OP_AUDIT_REVIEW.md), [24 builder
questions](EOO_OP_BUILDER_QUESTIONS.md), [candidate row 38](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L38),
and [coverage row 388](ZONE_COVERAGE_REGISTER.csv#L389).

### Checkpoint 475: eoo_op Muspelheim map and portal targets

The unregistered eoo_op WLD has three rooms and five directions: four reciprocal
local directions form one weak component, and #29210 up points to inactive
Duris3 #123225 (“The Ocean”) without a return. All three candidate room VNUMs
are also defined by active Newbie and inactive dark_gh with different complete
room blocks. The current active table resolves the four candidate-local
numeric targets to Newbie identities. Seven active Newbie directions target
the shared #29210–#29212 numbers; they remain Newbie-to-Newbie edges, so
candidate inbound stays zero and no approach is added to curated totals.

The four type-25 portals store #29206, #29222, #29212, and #29225 while their
descriptions name EOO/Valhalla, the Undead Continent, Khomani-Kan, and Goodie
Continent. Each requires CMD_ENTER (7) and stores value[2] = -1; current code
only decrements positive charges. Active Newbie owns all four target numbers,
while inactive EOO and eoo_op2 records provide competing map identities for
three of them. eoo.zon also imports #29211–#29213 from this candidate, so
preserve those object dependencies without treating the active same-number
Newbie objects as the portals. Curated totals remain 11,987 overall / 415
from Surf. See the [audit](EOO_OP_AUDIT_REVIEW.md), [24 builder
questions](EOO_OP_BUILDER_QUESTIONS.md), [candidate row 38](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L38),
and [coverage row 388](ZONE_COVERAGE_REGISTER.csv#L389).

### Checkpoint 476: eoo_op2 Niflheim map and reciprocal portal pairs

The unregistered eoo_op2 WLD has six rooms and nine directions. Eight directions
form two separate reciprocal chains of three rooms; #29223 also points up to
inactive Duris3 #151855 (“Grassy Plains”), with no target return. Directed WLD
traversal from #29220 reaches only three of six rooms. The other three-room
chain is not part of its WLD component.

All six room VNUMs also belong to active Newbie with different room blocks;
#29220–#29222 additionally occur in inactive dark_gh with distinct records.
Sixteen active Newbie exits target the same #29220–#29225 numbers, but those
are Newbie-to-Newbie links, not eoo_op2 ingress. Candidate-identity active
inbound remains zero. Curated totals stay 11,987 overall / 415 from Surf.

The two EOO portal imports from eoo_op target eoo_op2 Inner Sanctum rooms
#29222/#29225; eoo_op2’s pools at those rooms store the reciprocal EOO targets
#29202/#29206. These source-only type-25 links cross the two WLD components
through a third unregistered component. Current active room ownership maps all
four targets to Newbie records instead, so preserve both authored pairs without
asserting current access. See the [audit](EOO_OP2_AUDIT_REVIEW.md), [24 builder
questions](EOO_OP2_BUILDER_QUESTIONS.md), [candidate row 39](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L39),
and [coverage row 389](ZONE_COVERAGE_REGISTER.csv#L390).

### Checkpoint 477: eskull_hall Ebon Skull Guildhall map and active identities

The unregistered eskull_hall WLD has thirteen rooms and 24 directions: 22
local, plus #29300 up to inactive Duris3 #156221 and #29310 up to inactive
Duris3 #162341. Neither Ocean target has a return exit. The candidate-only map
has two weak components, sizes ten and three; each component is directed-reachable
from its own first room, #29300 or #29310. The current active table resolves
the 22 local target VNUMs to Newbie records, not to these candidate room
identities.

A full active-WLD scan found thirteen Newbie directions with raw targets in
#29300–#29312. Every source and target belongs to Newbie’s loaded map, so
candidate-identity inbound remains zero. Keep the active Newbie routes separate
from the unregistered Ebon Skull map, and add no source-only approach to the
curated totals. Those totals remain 11,987 overall / 415 from Surf.

No fall or current records occur in the candidate WLD. Preserve both separate
local components and the two one-way Duris3 boundary records pending the map
owners’ decision. See the [eskull_hall audit](ESKULL_HALL_AUDIT_REVIEW.md),
[24 builder questions](ESKULL_HALL_BUILDER_QUESTIONS.md),
[candidate dependency row 40](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L40),
and [coverage row 390](ZONE_COVERAGE_REGISTER.csv#L391).

### Checkpoint 478: ethdawn_op Mistywood outpost map and portal targets

The unregistered ethdawn_op WLD has three rooms and four directions: three
local directions form one weak component, and all three rooms are
directed-reachable from #29310. All three room VNUMs lie outside the numeric range for header #1, so all four directions originate in range-mismatch rooms. The remaining direction goes up from #29310
to inactive Duris3 #156745, “Rolling Hills,” with no return. All three
candidate room VNUMs also belong to active Newbie and inactive loc_gh and
eskull_hall records with different complete room blocks.

Six active Newbie directions target #29310–#29312. Those are Newbie-to-Newbie
routes through its Grotto, Small Home, and Life Crossroads rooms, not
Mistywood ingress. Candidate-identity active inbound remains zero, and no
new approach is added to the curated totals. They remain 11,987 overall /
415 from Surf.

The two type-25 candidate portals are separate from the WLD graph: #29310
stores #29307 and #29311 stores #29312, both with CMD_ENTER and value[2] = -1.
Active Newbie owns both target numbers; #29307 is also an Ebon Skull room
number. Ravenmis has a source-only O reset of object #29311 into #29307.
Preserve these typed source references without counting them as current WLD
routes. See the [ethdawn_op audit](ETHDAWN_OP_AUDIT_REVIEW.md),
[24 builder questions](ETHDAWN_OP_BUILDER_QUESTIONS.md),
[candidate dependency row 41](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L41),
and [coverage row 391](ZONE_COVERAGE_REGISTER.csv#L392).

### Checkpoint 479: fists_gh Guildhall map and Patrols collisions

The unregistered fists_gh WLD has ten rooms and 21 directions: 20 local
directions form one weak component, and all ten rooms are directed-reachable
from #30500. The remaining direction goes up to inactive Duris3 #157556
("Grassy Plains") without a return. No fall or current records occur.

Seven candidate room VNUMs #30500-#30506 also belong to active Patrols, but
the complete room blocks differ. Current active tables resolve 14 candidate
target VNUMs to Patrols identities; the six edges among candidate-only
#30507-#30509 and the Duris3 boundary are unresolved. Candidate-identity
active inbound remains zero.

The all-source comparison found 14 candidate directions to Patrols VNUMs.
Twelve originate in the overlapping source rooms. The two source-only
approaches are #30507 east to #30506 and #30509 up to #30503. They were
already included in the CP318 curated source-only total; add none. The
#30503 down and #30509 up directions are reciprocal; CP318 documentation
previously mislabeled the #30509 direction as down. Curated totals remain
11,987 overall / 415 from Surf. See the [Fists audit](FISTS_GH_AUDIT_REVIEW.md),
[24 builder questions](FISTS_GH_BUILDER_QUESTIONS.md),
[candidate row 42](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L42), and
[coverage row 392](ZONE_COVERAGE_REGISTER.csv#L393).

### Checkpoint 480: fooquest ship map and quest room

The unregistered fooquest WLD has 17 rooms and 30 directions. The sixteen
captain-ship rooms form one weak component and are all directed-reachable
from #65000. The separate #65050 Newbie Quest Mob Room has no exits, so it
forms its own component. All 30 directions stay within the source WLD; no
fall or current records occur.

All 17 room VNUMs lie outside the conventional numeric range for header
#636, and all 30 directions originate in those range-mismatch rooms. No other
current WLD source defines these same room VNUMs, none of the candidate
targets resolves to an active WLD record, and active-WLD inbound remains
zero. The candidate therefore has no recorded external WLD boundary or
active approach. Preserve the disconnected quest room and exact source
fields for owner review. Curated totals remain 11,987 overall / 415 from
Surf. See the [FooQuest audit](FOOQUEST_AUDIT_REVIEW.md),
[24 builder questions](FOOQUEST_BUILDER_QUESTIONS.md),
[candidate row 43](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L43), and
[coverage row 393](ZONE_COVERAGE_REGISTER.csv#L394).

### Checkpoint 481: forestb source bundle and shop references

forestb has no candidate WLD or ZON file, so it contributes no room graph,
exit, component, fall/current, or zone-range metric. The candidate-inbound
active-WLD count remains zero and the curated inactive-WLD totals remain
11,987 overall / 415 from Surf. Do not treat the #38259 SHP room field as a
WLD room: the same number is a candidate crude-comb OBJ and no current WLD
source defines it. The shop's #38235 keeper is only present in the unlisted
candidate MOB file. If the non-mini fallback is used, its all-file SHP glob
can include forestb; that conditional source path and the unguarded negative
keeper index are documented in the [audit](FORESTB_AUDIT_REVIEW.md). See the
[24 builder questions](FORESTB_BUILDER_QUESTIONS.md),
[candidate row 44](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L44), and
[coverage row 394](ZONE_COVERAGE_REGISTER.csv#L395).

### Checkpoint 482: forofcon Skull City and Fortress map

The forofcon WLD has 403 rooms and 817 directions. Nine weak components
have sizes 292, 63, 28, 14, 2, 1, 1, 1, and 1; directed traversal from
#139900 reaches 102 rooms. All positive targets are candidate-local; the
single other exit is #139900 D3 to -1. Eight rooms carry fall-chance data and
eight carry current data. All 403 rooms and all 817 exits are outside the
conventional #100-#199 range implied by ZON header #1.

All 403 room VNUMs overlap inactive Duris3 with different WLD blocks. The
first 53 also overlap active Smoke #139900-#139952 with different blocks;
the remaining 350 candidate room identities have no active WLD owner, and
active-WLD inbound to those unique forofcon rooms is zero. Current active
ownership maps the shared 53 numbers to Smoke. A direct parse found 109
forofcon directions to those Smoke-owned numbers: 104 from colliding source
IDs and five from unique sources. The five source-only approaches were
already counted at CP364; preserve them without adding again. Curated totals
remain 11,987 overall / 415 from Surf.

The 766-command candidate ZON includes 19 M, 21 D, and one O placement into
Smoke-numbered rooms; all stay with the unregistered forofcon reset identity.
It also shares the #139945 D2 reset coordinate with active Smoke. Keep both
reset owners distinct. See the [audit](FOROFCON_AUDIT_REVIEW.md),
[24 builder questions](FOROFCON_BUILDER_QUESTIONS.md),
[candidate row 45](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L45), and
[coverage row 395](ZONE_COVERAGE_REGISTER.csv#L396).

### Checkpoint 483: genesis hall source map and Ocean edge

The unregistered Genesis WLD has ten rooms and 19 direction records: 18
local edges and one D4 from #29901 to #157747 in Duris3.wld. Its local edges
form one weak component, and traversal from #29900 reaches all ten rooms.
The target room is “The Ocean,” but Duris3 and Genesis are absent from the
full area list, so this source edge is not a qualified current route. No
WLD source outside the Genesis and aeon_gh candidate files targets
#29900–#29909; their room blocks are different, and both source components
are unregistered.
Negative targets, fall records, and currents are absent. Keep the source
graph and the shared-number Aeon map as separate identities. See the
[Genesis audit](GENESIS_AUDIT_REVIEW.md#authored-map-and-narrative),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-483-genesis-source-identity-and-shop-dependencies),
[candidate row 46](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L46), and
[coverage row 396](ZONE_COVERAGE_REGISTER.csv#L397).

### Checkpoint 484: Genocide Guildhall and Newbie boundaries

The unregistered genocide_gh WLD has ten rooms and 19 directions: 18 local
edges and one D4 from #29200 to Duris3 #157677. The local graph is one weak
component, with all ten rooms directed-reachable from #29200. Seventeen
directions target #29201–#29209, which are active Newbie room numbers; their
current targets are Newbie rooms, not genocide_gh rooms. The other local edge
targets #29200, and the Duris3 target is absent from the full area list, so
these two directions are positive-unresolved. There are no negative targets,
fall records, or currents.

genocide_gh D0 from #29200 to active Newbie #29201 is one of the three
already-counted source-only approaches in Checkpoint 304, alongside dark_gh
and eoo. Preserve it without adding a new approach or changing the curated
11,987 overall / 415 from Surf totals. No active WLD direction targets the
Newbie-unshared #29200; active Newbie directions to #29201–#29209 remain
Newbie-to-Newbie. The one-way D4 to Duris3 has no return edge. Keep all four
same-number source maps separate from the active Newbie map. See the
[Genocide Guildhall audit](GENOCIDE_GH_AUDIT_REVIEW.md#authored-map-and-source-only-boundaries),
[Newbie connectivity review](NEWBIE_AUDIT_REVIEW.md#active-graph-and-inactive-source-only-approaches),
[candidate row 47](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L47), and
[coverage row 397](ZONE_COVERAGE_REGISTER.csv#L398).

### Checkpoint 485: Genocide Guild Hall map and boundaries

The unregistered genocide_hall WLD has seven rooms, #29500–#29504 and
#29510–#29511, with twelve directions: ten local edges and two D4 edges to
other maps. The local graph has two weak components of four and three rooms;
the largest has four rooms. Directed traversal reaches four rooms from
#29500 and three from #29504. All twelve targets are unresolved in the
current full loader list. The local destinations have no current room
records in the listed Harrow WLD, whose area/QST manifest entry spans zone
numbers #294–#296 but has no genocide_hall source entry.

The D4 from #29500 targets #1124854 in unlisted ud_map2.wld, “The Endless
Rock Filled Cavern Ithilienienzuth”; that target room has no direct return to
#29500. The D4 from #29510 targets #144550, “The Ocean,” in unlisted
Duris3.wld; that room has no direct return to #29510. There are no negative
targets, fall records, or currents. No currently listed active WLD direction
targets #29500–#29511. Same-number WLD sources aod_gh, legion, and rf_op are
also unregistered and do not provide active destinations.

The two disconnected local components and both source-only boundary edges
need map-owner review. See the
[Genocide Guild Hall audit](GENOCIDE_HALL_AUDIT_REVIEW.md#authored-map-and-source-only-boundaries),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-485-genocide-guild-hall-identities-and-history),
[candidate row 48](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L48), and
[coverage row 398](ZONE_COVERAGE_REGISTER.csv#L399).

### Checkpoint 486: Gimluk’s Cottage map and boundaries

The unregistered gianthou WLD contains 271 rooms and 698 directions: 696
local and two nonlocal targets. Its local graph is one weak component of 271
rooms; directed traversal from #42600 reaches all 271. The source room range
matches the ZON top #42870, but its zone header #426 overlaps the listed
Pworm #425–#426 manifest band and its #427–#428 rooms lie in bands marked
open. The area and QST lists contain no gianthou entry.

All 696 local targets point into the unlisted candidate, so no target
resolves through the current active WLD set. The remaining two directions
are #42811 D0 to negative #-1 and #42870 D3 to positive #274141. In total,
zero of 698 targets resolve; 697 positive targets are unresolved and one is
negative. There is one cross-zone outbound direction, no cross-zone inbound
or reverse direction, and no active WLD inbound to the candidate.

The #274141 record occurs in two unlisted files with different identities:
surf calls it “Vast Plains of Grassy Meadows,” and surface2011 calls it “The
Azure Waters of a Stormy Sea.” Neither file is listed in areas/AREA, while
active surface.wld covers a different range. Neither target record directly
returns to #42870. Keep this as a source-only, unresolved boundary; it is not
an active Surf route. The candidate has 20 WLD fall-chance rooms and zero
current rooms. Its 38 ZON F commands are follower resets
([ZON](../../../areas/zon/gianthou.zon#L6813),
[loader](../../../src/world/db.c#L7747)).

The Pworm/open-band ownership and both #274141 target identities require
map-list owner decisions before activation or travel claims. See the
[Gimluk’s Cottage audit](GENIANTHOU_AUDIT_REVIEW.md#authored-map-and-source-only-boundaries),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-486-gimluks-cottage-identities-and-history),
[candidate row 49](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L49), and
[coverage row 399](ZONE_COVERAGE_REGISTER.csv#L400).

### Checkpoint 487: Guild Rooms shop references

guild_rooms contains no WLD file, so it has no candidate map graph or source
exits. Its single unregistered SHP record uses keeper #8552, products
#8555–#8559, and shop room #8560. Active grave.wld defines all seven referenced values as
rooms in its #84–#86 area band; #8560 is “A Simple Stone Tomb.” The matching
MOB prototype #8552 and OBJ prototypes #8555–#8559 are absent from current
source files. Keep these as cross-type identity collisions rather than
resolved mobile or product definitions.

The shop-room reference #8560 names an active Grave room, and compiled
guild-transfer code separately calls real_room(8556), also an active Grave
room. Neither is a WLD exit into a guild_rooms map. No source WLD exits or
candidate map inbound exits exist for this SHP-only source. The full-area
lists contain grave and a separate guildhalls band #480–#489, but no
guild_rooms entry. See the [Guild Rooms audit](GUILD_ROOMS_AUDIT_REVIEW.md#typed-shop-references-and-active-grave-identities),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-487-guild-rooms-identities-and-history),
[candidate row 50](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L50), and
[coverage row 400](ZONE_COVERAGE_REGISTER.csv#L401).

### Checkpoint 488: Halfcut Hills inactive-source graph

The unlisted halfcut_hills WLD has 235 rooms and 550 directions: 549 local
and one outbound from #27001 D0 to #256031. Its local graph is one weak
component of 235 rooms; directed traversal from #27001 reaches 226. The
active loader resolves the 549 local target vnums through active halfcut's
same-number rooms. The #256031 target is unresolved in the active source set
and has different definitions in unlisted surf and surface2011 sources.
Unregistered surf.wld room #256031 has a south exit back to #27001, opposite
the candidate's north exit; the surface2011 definition instead exits south
to #256331. The source-only reciprocal and its limits are reconciled at
[Checkpoint 900](#checkpoint-900-halfcut-hills-reconciles-source-only-reciprocal-route).
The candidate has no negative exits, WLD fall chances, or current fields.

Active Surface #629274 has one southbound cross-area exit to #27001. That
target resolves to listed active halfcut, which also provides its reciprocal
north exit to #629274; it is a numeric inbound match to the inactive
halfcut_hills source, not ingress into that source identity. Candidate #27001
exits north to unlisted #256031; the candidate source has no return record of
its own. The unregistered surf definition adds a source-only numeric return,
not an active map edge. Keep active and inactive route pairs separate. See the
[Halfcut Hills audit](HALFCUT_HILLS_AUDIT_REVIEW.md#candidate-wld-graph-and-target-identities),
[active map review](../zone-stories/THE_HALFCUT_HILLS.md#actual-access-and-hazards),
[candidate row 51](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L51), and
[coverage row 401](ZONE_COVERAGE_REGISTER.csv#L402).

### Checkpoint 489: Drakenwood Hostel inactive-source graph

The unlisted hostel WLD has 102 rooms and 242 exits. All 242 target numbers
are within candidate #1–#102, with no negative or external destinations. Its
weak graph has five components of sizes 78, 17, 4, 2, and 1; traversal from
#1 reaches 78 rooms and leaves 24 outside that directed traversal. There are
no candidate WLD fall chances or current fields.

Ninety-one candidate exit targets have same-number rooms in active Limbo or
Heavens; 151 target references have no matching active room. Since hostel is
absent from the active WLD list, these are numeric resolutions through the
current active table or unresolved numbers, not a loaded Hostel graph.

Ten active cross-area exits point to VNUMs also used by Hostel: Desert #49790
has nine directions to #1, and Thetis #38075 south points to #38. The live
destinations are Limbo #1 and Heavens #38. They do not enter Hostel, and the
candidate has no outbound cross-area edge or reciprocal pair. Preserve these
active links with their current owners. See the
[Hostel audit](HOSTEL_AUDIT_REVIEW.md#candidate-graph-and-active-numeric-matches),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-489-hostel-source-identities-and-history),
[candidate row 52](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L52), and
[coverage row 402](ZONE_COVERAGE_REGISTER.csv#L403).

### Checkpoint 490: IC3 inactive-source graph

The unlisted IC3 WLD has 54 rooms and 112 exit records. One hundred eleven
edges target rooms in the candidate's #420000–#420053 range; the remaining
edge is #420000 north to active Rice Fields #77262. The candidate-local graph
is one weak component, and directed traversal from #420000 reaches all 54
rooms. There are no negative targets, range-mismatch rooms, fall chances, or
current fields. In the current active room table, only the one outbound target
resolves; the local candidate edges are source-only while IC3 is unlisted.

No active cross-area WLD exit enters the IC3 range. The former Rice Fields
south exit from #77262 to #420000 was changed to `-1` in the April 7, 2026
entrance-removal commit. The active room now has no return to IC3, and no
reciprocal active pair remains. The retained candidate exit is not active
ingress. See the [IC3 audit](IC3_AUDIT_REVIEW.md#candidate-graph-and-active-numeric-matches),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-490-ic3-source-identity-and-history),
[candidate row 53](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L53), and
[coverage row 403](ZONE_COVERAGE_REGISTER.csv#L404).

### Checkpoint 491: Infernal Guildhall inactive-source graph

The unlisted Infernal Guildhall WLD has 16 rooms and 29 exits: 26 local
edges and three D4 up exits to unlisted `Duris3.wld` rooms #154534, #124233,
and #187352. The source-only target records do not return to the candidate.
The local WLD graph has three components of sizes 10, 3, and 3; directed
traversal from #29400 reaches ten rooms. Four type-25 portal objects, placed by
ZON, author links across the three WLD components but are outside these WLD
edge counts.

All 26 local target VNUMs currently resolve to different active Harrow rooms;
the three Duris3 targets do not resolve through the active room table. Two
active Surface exits also target numeric #29400, but they are the reciprocal
Surface–Harrow routes and lead to active Harrow's room, not Infernal. No active
Infernal ingress or reciprocal candidate-to-Duris3 edge was found. See the
[Infernal audit](INFERNAL_GH_AUDIT_REVIEW.md#candidate-graph-and-active-numeric-matches),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-491-infernal-guildhall-source-identities),
[candidate row 54](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L54), and
[coverage row 404](ZONE_COVERAGE_REGISTER.csv#L405).

### Checkpoint 492: Jungle inactive-source graph

The unlisted Jungle WLD has 92 rooms and 288 exit records: 282 local edges,
two positive exits to active Surface rooms #616072 and #616872, three
one-way source references to unlisted Duris3 rooms #158558, #158560, and
#158659, and one authored -1 at #67074 west. The local graph is one weak
component; directed traversal from #67001 reaches all 92 rooms. In the active
WLD table, only the two Surface targets resolve. No active WLD exit enters
Jungle.

A separate source-only inbound scan found four references to Jungle: surf.wld
#252412 south and underdark2011.wld #253961 south to #67005; surf.wld #253012
north and underdark2011.wld #255161 north to #67087. Neither source stem is
in the full area list, so these are not active routes. Preserve the two
Jungle-to-Surface exits already recorded at CP373 and do not count them again
in Surface totals.

Eleven rooms have C 5 2 southbound-current fields. Bridge room #67051 has
F 5 and a down exit to #67092. Reset mode is 1. These source fields and raw
reachability do not establish live movement, access, or safe return. See the
[Jungle audit](JUNGLE_AUDIT_REVIEW.md#candidate-wld-graph-active-targets-and-source-only-approaches),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-492-jungle-source-identities-and-manifest-history),
[candidate row 55](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L55), and
[coverage row 405](ZONE_COVERAGE_REGISTER.csv#L406).

### Checkpoint 904: Jungle has no new external typed route

The CP492 connectivity totals remain 92 rooms / 288 exits: 282 local targets,
two existing source-only departures to active Surface, three one-way exits to
unlisted Duris3, and one authored `-1`. No active WLD exit enters Jungle. Its
four inbound WLD records remain the two `surf` and two `underdark2011` source-
only approaches, with no direct return from Jungle. The two Surface exits stay
within the earlier CP373 count; no route count changes.

Checkpoint 904 found no outside ZON reset role, teleport/switch target, QST,
story-sidecar role, SHP field, or direct compiled numeric lookup that creates
another Jungle route. Preserve the eleven southbound current fields and the
bridge's F 5 marker as authored movement fields, not WLD edges or access proof.
See the [Jungle typed audit](JUNGLE_AUDIT_REVIEW.md#checkpoint-904-external-typed-role-refresh),
[27 builder questions](JUNGLE_BUILDER_QUESTIONS.md), and
[candidate row 55](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L55).

### Checkpoint 493: Kilfray QST-only component

Kilfray has no WLD source, room VNUM range, or authored exit records, so no
candidate map graph or connectivity count applies. The source-only QST defines
five giver headers and three contracts using active Mistywood giver and object
VNUMs. It is absent from both area lists and is not included in the tracked
standard quest-generation input. QST references do not establish a route to a
separate Kilfray area. See the [Kilfray audit](KILFRAY_AUDIT_REVIEW.md),
[Mistywood audit](MISTYWOOD_AUDIT_REVIEW.md#typed-dependency-and-qst-lineage),
[candidate row 56](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L56), and
[coverage row 406](ZONE_COVERAGE_REGISTER.csv#L407).

### Checkpoint 905: Kilfray's namesake routes remain Mistywood-owned

Kilfray remains a QST-only component with no WLD rooms or exits. Active
Mistywood rooms #95000–#95001 are on the Kilfray River, and three source-only
`surf.wld` approaches from rooms named Lake Kilfray / The Kilfray River target
those active Mistywood rooms. They are already included in Mistywood's 53
Checkpoint 284 Surf approaches. Because `surf.wld` is unlisted, none is active
ingress, and none creates a Kilfray map or quest route. Preserve the separate
map and quest identities while the story owner determines whether the QST's
swamp and tower scenes belong to this Mistywood setting. See the [Kilfray role
audit](KILFRAY_AUDIT_REVIEW.md#checkpoint-905-cross-source-identity-environment-and-typed-role-refresh),
[Mistywood approach table](MISTYWOOD_AUDIT_REVIEW.md#connectivity-reconciliation),
[20 builder questions](KILFRAY_BUILDER_QUESTIONS.md), and [candidate row 56](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L56).

### Checkpoint 494: Kzkrkeep QST/SHP-only component

Kzkrkeep has no WLD file, rooms, or authored exits, so there is no candidate
map graph or exit count. Its two SHP records place shops in active Worms rooms
#6915 and #6919; those room fields do not create a Kzkrkeep area or WLD route.
No inbound active WLD exit can be attributed to this component. See the
[Kzkrkeep audit](KZKRKEEP_AUDIT_REVIEW.md),
[Worms audit](WORMS_AUDIT_REVIEW.md#external-typed-and-code-references),
[candidate row 57](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L57), and
[coverage row 407](ZONE_COVERAGE_REGISTER.csv#L408).

### Checkpoint 906: Kzkrkeep roles remain in active Worms

Kzkrkeep still has no WLD rooms or authored exits. Its two shop `in_room`
fields #6915/#6919 resolve to active Worms rooms, as do the three product
values #6958/#6968/#6969 that lack object prototypes. These fields describe
Worms shop locations and product-type mismatches; they do not form a
Kzkrkeep map or candidate route. The new #6900 Ixarkon code hit is a separate
compiled room destination that resolves to active Worms room #6900. No
candidate inbound or connectivity count changes. See the [Kzkrkeep role
audit](KZKRKEEP_AUDIT_REVIEW.md#checkpoint-906-cross-source-typed-role-refresh),
[20 builder questions](KZKRKEEP_BUILDER_QUESTIONS.md), and [candidate row 57](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L57).

### Checkpoint 495: Labyrint QST-only source

The unlisted `labyrint.qst` has no WLD rooms or exits. Its giver #5028 resolves
to active Labyrinth's Sir Uthor, but that typed identity does not create a
separate Labyrint graph or candidate route. Keep its candidate WLD metrics
blank and its inbound-exit count at zero. See the
[Labyrinth audit](LABYRINTH_OF_NO_RETURN_AUDIT_REVIEW.md#typed-references-and-compiled-numeric-matches),
[active dossier](../zone-stories/LABYRINTH_OF_NO_RETURN.md),
[candidate row 58](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L58), and
[coverage row 408](ZONE_COVERAGE_REGISTER.csv#L409).

### Checkpoint 907: Labyrint remains QST-only beside the active Labyrinth graph

The source-only `labyrint.qst` still has no WLD rooms or exits. Its #5028
giver resolves to active Labyrinth's Sir Uthor, while all ten item commands
and the 50,000-experience amount repeat the active Knight branch. Those shared
typed identities do not create a Labyrint map, active inbound, or route.
Labyrinth's active Mountain Tracks pair and its prior 1,090 source-only global
approach census are unchanged; no route count is added for the candidate. See
the [Checkpoint 907 typed refresh](LABYRINTH_OF_NO_RETURN_AUDIT_REVIEW.md#checkpoint-907-labyrint-qst-branch-retains-active-labyrinth-item-roles),
[active Labyrinth audit](LABYRINTH_OF_NO_RETURN_AUDIT_REVIEW.md#active-wld-graph),
[candidate row 58](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L58), and
[coverage row 409](ZONE_COVERAGE_REGISTER.csv#L410).

### Checkpoint 496: Legion Guildhall inactive-source graph and object travel

The unlisted Legion WLD has 16 rooms and 29 direction records: 26 local exits
and three D4 exits to unlisted Duris3 rooms #153434, #178730, and #128779.
Its local graph has three weak components of sizes 10, 3, and 3; directed
traversal from roots #29500, #29510, and #29520 reaches each component. There
are no negative targets, range-mismatch rooms, fall chances, or currents.

None of the 29 target records resolves through the current active room table:
the 26 local edges are source-only while Legion is unlisted, and the other
three targets belong to unlisted Duris3. No active WLD edge enters #29500–#29523.
The four type-25 portal objects separately encode two reciprocal room pairs
(#29503 with #29512, and #29502 with #29522); they are not WLD exits and are
not included in these counts. See the
[Legion audit](LEGION_GUILDHALL_AUDIT_REVIEW.md#candidate-graph-and-active-numeric-matches),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-496-legion-guildhall-source-identities-and-vnum-collisions),
[candidate row 59](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L59), and
[coverage row 409](ZONE_COVERAGE_REGISTER.csv#L410).

### Checkpoint 908: Legion sibling travel records remain source-local

The exact-number role scan adds no active route or inbound WLD edge to Legion.
AoD type-25 destinations #29513, #29509, #29519, and #29507 all have matching
AoD WLD definitions; RFOP's #29512 destination has an RFOP WLD definition, and
its other target #29505 is outside Legion's room set. Same-number Legion rooms
remain separate unlisted identities. Legion's own four portals stay outside its
WLD edge counts. No external type-29 room target, QST/story consumer, or
additional compiled code reference was found; the #29502 inn assignment remains
identity-ambiguous. The CP496 16-room/29-direction graph, three components,
three Duris3 exits, and zero active inbound count do not change. See the
[CP908 role refresh](LEGION_GUILDHALL_AUDIT_REVIEW.md#checkpoint-908-typed-role-refresh),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-908-legion-same-number-portal-and-shop-roles-remain-source-local),
[candidate row 59](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L59), and
[coverage row 409](ZONE_COVERAGE_REGISTER.csv#L410).

### Checkpoint 497: Light Guildhall inactive map and active Underworld3 boundary

The unlisted Light Guildhall WLD has 16 rooms and 28 direction records: 26
local edges and two D4 exits to unlisted Duris3 Ocean rooms #157434 and
#179053. Its local graph has three weak components of sizes 10, 3, and 3;
directed traversal from #30000, #30010, and #30020 reaches every room in its
component. There are no negative targets, range-mismatch rooms, fall chances,
or currents.

The 26 local target VNUMs resolve to active Underworld3 records, which are
different room identities from Light Guildhall's source records. The two
Duris3 endpoints are unresolved in the active room table and have no direct
return to Light Guildhall. No active WLD exit enters the candidate identity.
Moonshae #26272 and Underworld3 #30000 retain their reciprocal active route;
the numeric #30000 match is not Light Guildhall ingress. Four type-25 objects
separately declare a reciprocal pair between #30007 and #30012 and an
unpaired reset from #30022 to #30005; object #30003 has a possible return
value but no Light Guildhall ZON placement. Keep these objects outside WLD
counts. See the
[Light Guildhall audit](LIGHT_GH_AUDIT_REVIEW.md#candidate-graph-active-route-and-portal-objects),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-497-light-guildhall-room-and-prototype-collisions),
[candidate row 60](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L60), and
[coverage row 410](ZONE_COVERAGE_REGISTER.csv#L411).

### Checkpoint 909: Underworld3 reset roles remain active-map uses

The CP497 Light Guildhall graph is unchanged: 16 rooms, 28 direction records,
three weak components, two one-way Duris3 targets, and zero active inbound
exits. The existing Moonshae #26272 to #30000 boundary remains the reciprocal
route into active Underworld3, not Light Guildhall. Underworld3's two ZON M-room
operands #30004 and #30005 resolve to its own active room records; they do not
add a route into the unlisted candidate map. Its 33 same-number mobile resets
and four equipment resets likewise remain Underworld3-owned prototype roles.
The role refresh found no external type-25/type-29 destination into the
candidate band. See the [CP909 role audit](LIGHT_GH_AUDIT_REVIEW.md#checkpoint-909-typed-role-refresh),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-909-active-underworld3-reset-roles-remain-separate-from-light-guildhall),
[candidate row 60](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L60), and
[coverage row 410](ZONE_COVERAGE_REGISTER.csv#L411).

### Checkpoint 498: Lords of Chaos Guildhall source graph and active Newbie boundary

The unlisted Lords of Chaos GuildHall WLD has 22 rooms and 39 direction
records: 36 local edges and three D4 exits to unlisted Duris3 rooms #154441,
#163267, and #110241. Its local graph has four weak components of sizes 10,
4, 4, and 4; directed traversal from #29300, #29310, #29320, and #29330
reaches every room in its component. There are no negative targets,
range-mismatch rooms, fall chances, or currents.

Twenty-four local target records resolve numerically to active Newbie rooms
#29300–#29312 and #29314, whose map identities differ from the candidate.
Twelve local target records to loc_gh-only rooms and the three Duris3 exits
remain unresolved in the active room table. Newbie has 16 active direction
targets equal to exact loc_gh room VNUMs; all resolve to Newbie's active map
identity, so candidate-identity inbound remains zero. Three other Newbie targets
in the numeric band (#29313, #29315, and #29316) are not loc_gh rooms. None of
the three Duris3 target records has a direct return to the loc_gh band. Six type-25
portal objects separately encode two reciprocal pairs and two unplaced
vortex destinations; do not add them to WLD exit totals. See the
[Lords of Chaos audit](LOC_GH_AUDIT_REVIEW.md#candidate-graph-and-active-route-identity),
[candidate row 60](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L61), and
[coverage row 411](ZONE_COVERAGE_REGISTER.csv#L412).

### Checkpoint 910: loc_gh same-number destinations remain source-owned

The exact-room recount corrects CP498's Newbie total: 16 active Newbie WLD
directions target VNUMs defined by loc_gh; the three other Newbie targets in
the broad numeric band are Newbie-only rooms #29313, #29315, and #29316. Those
16 active edges resolve to Newbie and do not add candidate-identity inbound.
Same-number WLD targets from `cbr_hall`, `eskull_hall`, `ravenmis`, and
`ethdawn_op` resolve to rooms in those unlisted source maps.

Active Newbie type-25 objects target its own #29305/#29308 rooms. Unlisted
`ethdawn_op` and `eskull_hall` portal values stay with their respective source
maps; no type-29 target or WLD edge changes loc_gh connectivity. Keep all portal
objects outside the 39 authored WLD direction count. The active Newbie boundary,
zero candidate inbound, 36 local candidate edges, and three Duris3 exits remain
unchanged. See the [CP910 role audit](LOC_GH_AUDIT_REVIEW.md#checkpoint-910-role-aware-source-closure),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-910-loc_gh-reset-shop-and-portal-roles-remain-separate),
[candidate row 60](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L61), and
[coverage row 411](ZONE_COVERAGE_REGISTER.csv#L412).

### Checkpoint 499: Luiren QST and old shop sources without a map

Luiren has no WLD source, room range, or authored exits, so there is no
candidate graph or exit count. Its QST contains 44 source-only M dialogue
headers whose giver VNUMs overlap active Pine Hollow mobiles. Those typed
numeric overlaps do not create WLD access or a separate Luiren map. The
candidate inbound active-exit count remains zero; keep candidate WLD metrics
blank rather than treating QST or SHP fields as exits. See the
[Luiren audit](LUIREN_AUDIT_REVIEW.md#source-inventory-and-quest-input),
[Pine Hollow review](PINE_HOLLOW_AUDIT_REVIEW.md#luiren-candidate-identity-and-old-shop-data),
[candidate row 61](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L62), and
[coverage row 412](ZONE_COVERAGE_REGISTER.csv#L413).

### Checkpoint 500: Magetower source graph and active Limbo and Heavens VNUM collisions

The unlisted Magetower WLD has 71 rooms and 152 direction records. All 152
target VNUMs are local to its own #1–#71 map; the directed graph is one weak
component of 71 rooms, and traversal from #1 reaches all rooms. There are no
negative targets, external candidate exits, fall chances, or currents. All
71 source rooms are outside the numeric band for ZON header #1000, so all 152
exit sources are range-mismatch rooms under the register's numeric-range
check.

The active room table resolves 93 candidate target records by number to
current Limbo/Heavens rooms; the other 59 target numbers are not currently
loaded. Forty-three candidate room VNUMs have active definitions: #1 in
Limbo, and #3–#5, #10–#46, #66, and #69 in Heavens. Separately, 70 active WLD
directions target numbers in this same band: 60 from Heavens, nine Desert
directions to Limbo #1, and one Thetis direction to Heavens #38. None enters
the unlisted Magetower identity. Candidate-identity inbound remains zero.

Magetower #2 north and #64 south target candidate-local #1 in the source
graph; the active room table assigns #1 to Limbo. Keep those source-only
numeric references distinct from active Limbo ingress. See the
[Magetower audit](MAGETOWER_AUDIT_REVIEW.md#candidate-graph-and-active-table-resolution),
[Limbo review](LIMBO_STORY_REVIEW.md#L63),
[candidate row 62](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L63), and
[coverage row 413](ZONE_COVERAGE_REGISTER.csv#L414).

### Checkpoint 501: map1 empty MOB placeholder with no room graph

The unlisted `map1` component consists only of a tracked zero-byte
`areas/mob/map1.mob`. It defines no mobile prototype and has no WLD, room
record, authored exit, or candidate room VNUM. Leave room-connectivity metrics
blank; they are not zero-valued graph results. The candidate inbound-exit
count is zero because there is no room identity to target. The separately
named `ud_map1` WLD and its routes remain a different source component.
See the [map1 audit](MAP1_AUDIT_REVIEW.md#references-and-area-list-status),
[candidate row 63](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L64), and
[coverage row 414](ZONE_COVERAGE_REGISTER.csv#L415).

### Checkpoint 502: map22 grid template and conditional random-lab connection to Duris3

The unlisted `map22.wld` contains 10,000 rooms (#900000–#909999) and 39,600
local D0–D3 records. The rooms form one reciprocal 100-by-100 grid; traversal
from #900000 reaches all rooms. No other WLD defines or targets this range,
so all 39,600 candidate targets are unresolved in the current active room
table. There are no negative, external, fall, or current records, no active
WLD ingress, and no unregistered-WLD approach.

Static code separately selects #900000–#909999 and #905050 for the type-1
random labyrinth and uses Duris3 room #145443 (“The Ocean”) as its entrance.
The generator can add a down exit from that room to #905050, but checks that
the necessary rooms exist before operating. This code-created, conditional
path is not a WLD edge or current active route; keep the WLD connectivity
counts separate. See the [map22 audit](MAP22_AUDIT_REVIEW.md),
[Duris3 audit](DURIS3_AUDIT_REVIEW.md),
[candidate row 64](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L65), and
[coverage row 415](ZONE_COVERAGE_REGISTER.csv#L416).

### Checkpoint 503: map5 mobile source with no room graph

The unlisted `map5.mob` defines 40 mobile prototypes and no WLD rooms or
exits, so its room-connectivity metrics remain blank. Forty-five source-only
`M` commands in the separate unlisted `surf.zon` place nine of these mobiles
in 40 rooms defined by `surf.wld`; they are not active resets or WLD
ingress. Compiled code also binds patrol and Tharnadian specials to exact
map5 mobile VNUMs. Keep those typed/reset dependencies separate from world
edge counts. See the [map5 audit](MAP5_AUDIT_REVIEW.md),
[Surf candidate row 96](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97),
[candidate row 65](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L66), and
[coverage row 416](ZONE_COVERAGE_REGISTER.csv#L417).

### Checkpoint 504: map8 empty MOB path with no room graph

The unlisted `map8.mob` is a tracked zero-byte file with no prototype, WLD
room, or authored exit. Keep room-connectivity metrics blank; the candidate
inbound-exit count is zero because no room identity exists to target. Its
rename-follow history through empty `Players/` paths is not a WLD route or
area identity, and the distinct `oldmap8.wld` remains separate. See the
[map8 audit](MAP8_AUDIT_REVIEW.md#references-and-area-list-status),
[candidate row 66](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L67), and
[coverage row 417](ZONE_COVERAGE_REGISTER.csv#L418).

### Checkpoint 505: Mara Guildhall graph and source-only Duris3 exits

The unlisted `mara_gh.wld` contains 16 rooms and 29 directions. Twenty-six
local directions form 13 reciprocal pairs across weak components of 10, 3,
and 3 rooms; root #29600 reaches only the 10-room Guildhall core. Three up
exits target unlisted Duris3 rooms #156766, #124408, and #163376. No target
resolves in the current active room table, no Duris3 WLD return to a Mara
room was found, and no active WLD inbound targets this component. ITEM_TELEPORT
objects and the room #29605 `inn` special are separate non-WLD dependencies.
See the [Mara Guildhall audit](MARA_GH_AUDIT_REVIEW.md#wld-graph-and-external-boundaries),
[Duris3 supplement](DURIS3_AUDIT_REVIEW.md#checkpoint-505-supplement-source-only-mara-guildhall-boundaries),
[candidate row 67](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L68), and
[coverage row 418](ZONE_COVERAGE_REGISTER.csv#L419).

### Checkpoint 506: Mara Outpost graph and shared Mara Guildhall identities

The unlisted `mara_op.wld` has three rooms and five directions: four local
directions form two reciprocal pairs across one three-room component, and
#29610 up targets unlisted Duris3 room #124408. None of the five targets
resolves in the current active room table; no active WLD inbound or Duris3
return was found. Keep the ITEM_TELEPORT destination #29608 separate from
WLD edges; Mara Guildhall is its only tracked room definition. See the
[Mara Outpost audit](MARA_OP_AUDIT_REVIEW.md#wld-graph-and-duris3-boundary),
[Mara Guildhall audit](MARA_GH_AUDIT_REVIEW.md#shared-vnums-history-and-disposition),
[candidate row 68](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L69), and
[coverage row 419](ZONE_COVERAGE_REGISTER.csv#L420).

### Checkpoint 507: Mavulsk local identities and the unresolved Surf target

The unlisted Mavulsk WLD has 233 rooms and 518 directions. Its local graph
has four weak components of 228, 3, 1, and 1 rooms; directed traversal from
#1 reaches 200 rooms. Six source-only ITEM_TELEPORT objects add local
value[0] destinations that the WLD graph omits. Two mining-shaft rooms have
F=50 fields, and no current fields were found.

The current active room table has same-number destinations for 94 Mavulsk
exit records and no destination for the other 424. These numeric hits are
not confirmed routes: Mavulsk defines all rooms #1–#233 itself, and every
active same-number owner differs from its Mavulsk room. For example, Mavulsk
#2 and #3 both target its own “Before the Stone Mansion,” room #1, although
the current active owner of #1 is Limbo. Mavulsk’s 26 numeric records in the
prior inactive-approach recount remain counted under that recount’s stated
rule; they do not establish a Mavulsk-to-Limbo journey or active ingress.

Mavulsk #188 east → #228202 is the only target outside its local room set.
Unlisted surf.wld and surface2011.wld both define #228202 with different
titles and the same four neighboring target numbers; neither has a WLD
return to Mavulsk #188. The active Thetis #38095 north → #104 remains
unresolved between Mavulsk’s “Quiet Library” and Dalvik’s Great Temple.
Keep both references as ownership questions. No active WLD edge or curated
inactive-approach total changes in this checkpoint. See the
[Mavulsk audit](MAVULSK_AUDIT_REVIEW.md#wld-graph-and-unresolved-map-boundaries),
[Thetis audit](THETIS_AUDIT_REVIEW.md#active-graph-and-boundary-exits),
[Dalvik audit](DALVIK_AUDIT_REVIEW.md#static-wld-graph),
[candidate row 69](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L70), and
[coverage row 420](ZONE_COVERAGE_REGISTER.csv#L421).

### Checkpoint 508: Mentiri low VNUMs remain distinct from active owners

Mentiri’s source WLD has 72 rooms and 160 direction records. All targets stay
within its own #1–#72 band; the graph has one weak component and raw traversal
from #1 reaches all 72 rooms. There are no external WLD exits, fall fields, or
current fields. Current room-table lookup numerically matches 106 candidate
exit targets to active Limbo/Heavens definitions and leaves 54 unmatched;
those lookups do not identify Mentiri rooms because the candidate is unlisted
and every overlapping block differs.

Seventy active WLD directions from Heavens, Desert, and Thetis numerically
target #1–#72. Their current destinations belong to active Limbo or Heavens,
so no direction is attributable as Mentiri ingress. The candidate register
records zero active inbounds. Preserve Mentiri’s complete local graph and
keep these low-number routes separate. See the
[Mentiri audit](MENTIRI_AUDIT_REVIEW.md#wld-graph-and-current-numeric-lookup),
[candidate row 70](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L71), and
[coverage row 421](ZONE_COVERAGE_REGISTER.csv#L422).


### Checkpoint 509: Monsteri local map and header-range mismatch

The unregistered Monsteri WLD has five rooms and eight direction records.
All eight are local within #1000–#1004; the graph has one weak component and
raw traversal from #1000 reaches all five rooms. No fall or current fields
were found. The ZON header is #1000 while the VNUM/100 convention places the
room IDs in band 10, so all five rooms and all eight exit origins are marked
range-mismatched. Preserve the raw local graph without inferring a correction.

The current active room table has no #1000–#1004 definitions and no active
WLD direction targets that band. Therefore all eight positive local targets
remain unresolved in the active table; candidate active inbound, outbound,
and reverse-boundary counts remain zero. Active CBL Hall instead owns rooms
#29820–#29824 with matching Monsteri descriptions, but those are separate
room VNUMs and no WLD edge connects the maps. See the
[Monsteri audit](MONSTERI_AUDIT_REVIEW.md#wld-graph-range-discrepancy-and-current-table),
[candidate dependency line 72](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L72), and
[coverage line 423](ZONE_COVERAGE_REGISTER.csv#L423).


### Checkpoint 510: moriatst has no WLD graph

The unregistered `moriatst` component contains only three SHP records, so it
has no rooms, local exit graph, VNUM range, or cross-area WLD edges. The
candidate register records zero active WLD inbounds. Its three `in_room`
values (#99306, #99312, and #99318) are shop references into active Moria
rooms; they are not WLD connections. Preserve that distinction in the graph
totals. The shop-loader fallback can conditionally include this file, as
described in the [moriatst audit](MORIATST_AUDIT_REVIEW.md), but it does not
add a WLD route. See [candidate row 73](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L73)
and [coverage row 424](ZONE_COVERAGE_REGISTER.csv#L424).


### Checkpoint 511: Mountain QST has no WLD graph

The uppercase `Mountain` component consists only of QST data. It defines no
rooms or WLD exits, so local reachability and room-range metrics do not
apply; the candidate dependency register records zero active WLD inbounds.
Its output reference to Dungeon object #93001 is a typed item dependency,
not a travel edge. See the [Mountain QST audit](MOUNTAIN_QST_AUDIT_REVIEW.md),
[candidate row 74](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L74), and
[coverage row 425](ZONE_COVERAGE_REGISTER.csv#L425).

### Checkpoint 512: mtncastl is an isolated room #1 placeholder

The unregistered mtncastl WLD contains one room, #1, and no exits. Its graph
is one isolated weak component; there are no fall or current fields, external
targets, or candidate outbound boundaries. ZON header #1 does not match the
room’s VNUM/100 band. The active room owner for #1 is Limbo, so generic active
directions to that number are not candidate inbounds. The placeholder block
also matches astral and ud-inside byte for byte, which does not prove shared
identity. See the [mtncastl audit](MTNCASTL_AUDIT_REVIEW.md), [candidate row
75](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L75), and [coverage row
426](ZONE_COVERAGE_REGISTER.csv#L426).
### Checkpoint 513: myranth1 has no WLD graph

The unregistered myranth1 component has only QST and SHP records, so it
defines no rooms, WLD exits, numeric room band, or local map graph. Its five
SHP in_room values reference active Moregeeth rooms; those are typed shop
placements, not travel edges. Candidate active WLD inbounds are zero. See the
[myranth1 audit](MYRANTH1_AUDIT_REVIEW.md), [candidate row 76](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L76), and
[coverage row 427](ZONE_COVERAGE_REGISTER.csv#L427).
### Checkpoint 514: Nah two components and Duris3 edges

The unregistered Nah WLD has 13 rooms and 26 directions: 24 local and two up
exits to unregistered Duris3 Ocean rooms #156123 and #179220. Its local graph
has weak components of 10 and 3 rooms; current active WLD inbounds to Nah are
zero. The 24 local targets reuse VNUMs that resolve in the active room table
to Underworld3, whose room descriptions differ from Nah. The two external
targets are not active and have no return into Nah. Nah’s two portal objects
carry values #30111 and #30103, but both VNUMs are also active Underworld3
rooms; this is a separate teleport-data question, not a WLD edge. See the
[NAH audit](NAH_AUDIT_REVIEW.md), [candidate row 77](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L77), and
[coverage row 428](ZONE_COVERAGE_REGISTER.csv#L428).

### Checkpoint 515: Newbiemaps source-map graph

The unregistered newbiemaps.wld has 89,847 rooms (#600000–#689999) and
359,196 north/east/south/west directions. Every target exists in the candidate
WLD and every direction has its opposite; the weak graph is one component.
The file omits 153 room records and 192 directional slots from existing rooms.
Its 359,196 exit tuples are all present in surface2011-temp.wld, which has
153 more rooms and 804 more exits.

Current active Surface owns 59,851 of the candidate’s room numbers, all with
different title strings. A static candidate scan finds 239,212 exits whose
targets currently resolve numerically to Surface. Of these, 238,610 start in
source-room numbers already owned by active Surface; 602 start in 600
candidate-only rooms and point to Surface. Those 602 records are the paired
300 north and 300 south strip directions plus two east/west edges already
documented in the [Surface audit](SURFACE_AUDIT_REVIEW.md#inactive-wld-surface-approaches);
the same tuples occur in surface2011-temp and must not be double-counted as
distinct routes. Active WLD sources have zero exits into candidate-only
#660004–#689999 rooms. The 239,396 raw active-target matches into the
candidate’s full numeric set all target VNUMs currently owned by Surface,
not candidate-specific rooms. Preserve the source records and map identities;
these numbers do not establish that newbiemaps is loaded or traversable. See
the [NEWBIEMAPS audit](NEWBIEMAPS_AUDIT_REVIEW.md), [candidate row 78](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L78),
and [coverage row 429](ZONE_COVERAGE_REGISTER.csv#L429).


### Checkpoint 516: Nexus56 one-city graph and 23 source-only approaches

The source-only Nexus56 WLD contains 760 rooms (#83000–#83825) and 1,663
directions. Every target is in the candidate file, and the weak graph contains
all 760 rooms. Three directions lack a reverse tuple: #83400 south to #83397,
#83639 north to #83636, and #83804 up to #83802. The graph has no fall-chance
or current directives.

Current active owners overlap 666 room numbers (659 Alatorin and 7 Delwyn),
all with different titles. The candidate has 1,454 target records whose
numbers currently map to these active owners. Within the source-only rooms,
23 directions are distinct approach records: 22 to Alatorin and one to
Delwyn. These 23 are already included in the global 11,987 source-only route
count (415 from Surface). Active WLD sources contain 1,516 raw target rows in
the candidate’s numeric set (1,446 Alatorin, 51 Surface Keeps, 14 Delwyn,
3 Surface, 2 WH); all resolve to current active owners, and none points to
the 94 Nexus56-only room numbers. Candidate-specific active WLD
inbounds remain zero. See the [Nexus56 audit](NEXUS56_AUDIT_REVIEW.md),
[candidate dependency row 78](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79),
and [coverage row 429](ZONE_COVERAGE_REGISTER.csv#L430).

### Checkpoint 517: Nexus_1 low-VNUM fortress graph and owner aliases

The source-only Nexus_1 WLD contains 40 rooms (#1–40) and 92 directional
exits. Every exit target is local, all 40 rooms form one weak component, and
every direction has its reverse tuple. No fall or current directives were
found. Current room owners overlap 35 IDs: #1 is Limbo, #3–5 and #10–40 are
Heavens; the five candidate-only IDs are #2 and #6–9. Active WLD sources
contain 63 raw target rows into #1–40, but all 63 resolve to the current Limbo
or Heavens owners and none targets #2 or #6–9, so candidate-specific active
inbounds remain zero after owner disambiguation. Six source-only exits from
#2, #6, and #9 approach those current owners; no additional global tally is
taken pending route-level duplicate reconciliation. See the
[Nexus_1 audit](NEXUS_1_AUDIT_REVIEW.md), [candidate dependency row 79](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L80),
and [coverage row 430](ZONE_COVERAGE_REGISTER.csv#L431).

### Checkpoint 518: oldmap8 100×100 regional grid and thirteen counted approaches

The source-only oldmap8 WLD defines every room #180000–#189999 and contains
39,918 directions in one 10,000-room weak component. Its 410 external target
records comprise 400 grid-edge crossings and ten additional cross-map exits.
Active WLD sources have zero targets into this room range, and no active WLD
owns an oldmap8 room number. Thirteen of the external destinations belong to
current active WLD owners: Werrun, Krethik, Mountain, Yuan Ti2, Mntcastl,
Bastine, and SurfaceKeeps. The global source-only tally already records all
13 oldmap8 directions; no count is added. The remaining 397 records target
Duris3 source-only rooms or numbers without a WLD definition. See the
[oldmap8 audit](OLDMAP8_AUDIT_REVIEW.md), [candidate dependency row 80](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L81),
and [coverage row 431](ZONE_COVERAGE_REGISTER.csv#L432).

### Checkpoint 519: Petrified Castle isolated keep and one negative exit

The unlisted Petrified Castle WLD defines 187 rooms and 395 directions. Its
main graph contains 186 rooms; Guards lounge #93723 is an isolated singleton.
There are six plain within-file directions without reverse tuples, one
negative south target (#93724 to -1), and no positive external target. Active
WLD sources have zero targets into the candidate range. This adds no active
route or source-only active-owner approach to the existing world totals. Keep
the map source-only pending confirmation of the negative target, disconnected
room, one-way interiors, editor ownership, and intended loader path. See the
[Petrified Castle audit](PETRIFIED_CASTLE_AUDIT_REVIEW.md),
[candidate dependency row 81](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L82),
and [coverage row 432](ZONE_COVERAGE_REGISTER.csv#L433).

### Checkpoint 520: plane_avernus two-room map with no active ingress

The unlisted plane_avernus WLD contains rooms #135 and #136 and two
reciprocal directions connecting them. There are no external exits and no
active WLD source targets either room. Two other unlisted WLD files, Dalvik
and Mavulsk, also define #135 and #136 with different room content and
topology. The repeated “The Plane of Avernus” ZON title does not create a
route to active Avernus, whose rooms occupy a separate VNUM range. This adds
no route to the active connectivity totals. See the
[plane_avernus audit](PLANE_AVERNUS_AUDIT_REVIEW.md),
[candidate dependency row 82](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L83),
and [coverage row 433](ZONE_COVERAGE_REGISTER.csv#L434).

### Checkpoint 521: Player Guilds catalog with no room graph

The unlisted player_guilds component has no WLD file, so it contributes no
room graph, directed exit, or candidate ingress count. Its 13 SHP records do
contain separate in_room values, including active-owner rooms such as
Drst #34202 and Lava Caves #35552. These are shop location references, not
WLD directions or proof that the catalog is loaded. The active connectivity
totals remain unchanged. See the
[Player Guilds audit](PLAYER_GUILDS_AUDIT_REVIEW.md),
[candidate dependency row 83](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L84),
and [coverage row 434](ZONE_COVERAGE_REGISTER.csv#L435).

### Checkpoint 522: Ravenmis guildhall map and unregistered outlet

The unlisted Ravenmis WLD has ten rooms and 19 authored exits: 18 target
numbers #29300–#29309 and form one weak component of ten rooms; the remaining
exit goes up from #29300 to #125119. That target exists as “Grassy Plains” in
unlisted Duris3.wld, with no return to Ravenmis. Although the 18 local target
numbers resolve in the active table, they resolve to Newbie’s distinct
same-number rooms. No active exit targets the Ravenmis source identity. Its
positive #125119 target remains unresolved through the current AREA
membership, so active-world connectivity totals do not change. See the
[Ravenmis audit](RAVENMIS_AUDIT_REVIEW.md),
[candidate dependency row 84](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L85),
and [coverage row 435](ZONE_COVERAGE_REGISTER.csv#L436).

### Checkpoint 523: Raxquest source-only wreck and outlet

The unlisted Raxquest WLD contains 84 rooms and 175 authored exits. Its 174
internal directions form one 83-room weak component; “Newbie Quest Mob Room”
#70500 is an isolated singleton. The only external direction goes south from
#70501 to unlisted Duris3 room #134956 (“Mountains”), with no return to
Raxquest. None of the 84 room VNUMs is in the current active WLD table, no
active WLD exit targets them, and none of the 175 source exits resolves through
the current full-area WLD table. All 175 positive targets therefore remain
unresolved for the active table, including the 174 source-local destinations.
There are no negative exits, range mismatches, fall/current records, or active
inbound approaches. The source-only graph has two weak components, largest 83.
This adds no active route to the world totals. Keep the unregistered outlet
under owner review; it is not a reciprocal or safety-qualified path. See the
[Raxquest audit](RAXQUEST_AUDIT_REVIEW.md),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-523-raxquest-qst-mirror-and-standalone-pirate-wreck-identity),
[candidate dependency row 85](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L86),
and [coverage row 436](ZONE_COVERAGE_REGISTER.csv#L437).

### Checkpoint 524: realms_master shop records without a room map

The realms_master component has no WLD source, so it adds no room graph or
authored exit records. Its two SHP records name in_room #11121 and #11124, but
neither VNUM has a current WLD definition. No candidate-room inbound exit
count is added. A legacy Realms Master ship-room binding block exists in
specs.assign.c but is disabled under #if 0. The shop loader's full-mode
fallback can glob this SHP file when areas/world.shp is absent; that possible
input path is not a world route or evidence the catalog boots. See the
[realms_master audit](REALMS_MASTER_AUDIT_REVIEW.md),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-524-realms_master-shop-records-and-typed-vnum-collisions),
[candidate dependency row 86](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L87),
and [coverage row 437](ZONE_COVERAGE_REGISTER.csv#L438).

### Checkpoint 525: returns two mobile prototypes without a map

The unlisted returns component contains two MOB records, #93400 and #93401,
but no WLD rooms or ZON placement/reset source. The silent golem text mentions
a tower entry and the diminutive cleric looks out a window; neither description
identifies a current source room. Exact source scans found no outside WLD
target, ZON reset, or active WLD ingress using either candidate VNUM. With no
candidate rooms, the component adds no graph or exit totals. This does not
resolve dynamic or deployment-specific placement. See the
[returns audit](RETURNS_AUDIT_REVIEW.md),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-525-returns-isolated-mobile-source-identity),
[candidate dependency row 87](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L88),
and [coverage row 438](ZONE_COVERAGE_REGISTER.csv#L439).

### Checkpoint 526: rf_gh split map and Torg VNUM collisions

The unlisted rf_gh WLD has 13 rooms and 24 authored exits. Twenty-two
same-number internal targets form weak components of ten rooms (#28900–#28909)
and three rooms (#28910–#28912). The two external exits are D4 from #28900 to
#111082 and D4 from #28910 to #154153. #111082 exists in listed Tharnrifts
as “The DragonMaw Mountains” and also in unlisted Duris3 as “The Ocean”;
#154153 exists only in unlisted Duris3 as “The Ocean.” Neither target source
has a return exit to the rf_gh numbers. Numeric lookup resolves the 22
internal targets to different rooms in listed Torg and resolves #111082 to
Tharnrifts; #154153 remains unresolved through the current area list.

The listed surface.wld exit from #611053 to #28900 is paired with Torg's
return from #28900 to #611053. That is the current surface/Torg connection:
#28900 is Torg's “Thin Trail,” not the absent rf_gh Organ Pit. Unlisted
surf.wld also has a numeric edge from #277211 to #28900. Neither reference
identifies an active rf_gh room or changes its candidate inbound count of
zero. The component adds no active route totals. See the
[rf_gh audit](RF_GH_AUDIT_REVIEW.md),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md),
[candidate dependency row 88](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L89),
and [coverage row 439](ZONE_COVERAGE_REGISTER.csv#L440).

### Checkpoint 527: rf_op fragment and unlisted ocean outlet

rf_op has three rooms and five authored WLD exits. Four same-number
intra-map targets connect all three rooms as one weak component; the fifth
is a one-way D4 exit from #29510 to #154153. No target resolves to a room
identity in the current areas/AREA list: the four local VNUMs are absent
from listed WLD sources, while #154153 is “The Ocean” only in unlisted
Duris3.wld. There are no active inbound exits to an rf_op room. The same
room numbers in aod_gh, genocide_hall, and legion are different unlisted
map identities.

The two ITEM_TELEPORT value[0] destinations (#29505 and #29512) are object
routes and are not included in WLD exit totals. #29505 has two different
unlisted WLD definitions, and #29512 is likewise absent from the current
manifest; rf_op's ZON does not place the second portal. The reused rf_gh
cathedral fragment does not establish a world route or component
replacement. See the [rf_op audit](RF_OP_AUDIT_REVIEW.md),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md),
[candidate dependency row 89](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L90),
and [coverage row 440](ZONE_COVERAGE_REGISTER.csv#L441).
### Checkpoint 528: scrubs_hall map and shop fallback

The unlisted scrubs_hall WLD has ten rooms and 19 exits: 18 same-number
local targets form one weak component; the remaining D4 outlet from #29100
targets #156119. Current numeric lookup resolves those 18 local edges to
different, listed Torg rooms. #156119 is “The Ocean” only in unlisted
Duris3.wld and has no exit back to #29100–#29109. No current active exit
enters a scrubs_hall identity. Thirteen active Torg WLD edges also target
these numbers, but they lead among Torg's listed rooms. Torg's 32 ZON door
and mobile-placement fields referencing the same numbers likewise belong
to its active map.

The SHP file is outside areas/AREA, while the full-mode shop fallback can
glob every `areas/shp/*.shp` file when areas/world.shp is absent. The
shopkeeper #29103 has no active MOB definition, and the shop room #29106
resolves to Torg's Shop of Gornogo. These shop/code bindings are conditional
source dependencies, not WLD connectivity or evidence of a boot. See the
[scrubs_hall audit](SCRUBS_HALL_AUDIT_REVIEW.md),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-528-scrubs_hall-room-mobile-overlaps-and-recent-shop-updates),
[candidate dependency row 90](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L91),
and [coverage row 441](ZONE_COVERAGE_REGISTER.csv#L442).
### Checkpoint 529: shadow_hall and Underworld3 number collision

The unlisted shadow_hall WLD has three rooms and five exits. Four local
number targets form one weak component; current numeric lookup resolves
them to different listed Underworld3 rooms. The remaining D4 outlet from
#30102 targets #151052, defined as “Thick Forest” only in unlisted
Duris3.wld, with no reverse exit. Eight active Underworld3 WLD edges and
one Underworld3 ZON room placement also use #30100–#30102; these resolve to
Underworld3 identities, not to the Shadow Law Guildhall. Candidate-identity
inbound count remains zero. The two ZON object imports are Player Castles
objects and do not add room routes. See the
[shadow_hall audit](SHADOW_HALL_AUDIT_REVIEW.md),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-529-shadow_hall-and-underworld3-room-identity),
[candidate dependency row 91](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L92),
and [coverage row 442](ZONE_COVERAGE_REGISTER.csv#L443).

### Checkpoint 530: shady2 MOB-only source

The unlisted `shady2.mob` contains 90 mobile prototypes and no WLD, so it
adds no rooms or authored exits to the connectivity census. Its VNUMs
#97500–#97589 all also occur in active `shady.mob`. The 342 listed ZON
mobile resets using these numbers resolve through the active Shady mobile
definitions; they are typed mobile references, not WLD edges or evidence of
`shady2` map ingress. Candidate inbound active exit count remains zero.
See the [shady2 audit](SHADY2_AUDIT_REVIEW.md),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-530-shady2-shared-mobile-vnums-are-distinct-records),
[candidate dependency row 92](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L93),
and [coverage row 443](ZONE_COVERAGE_REGISTER.csv#L444).

### Checkpoint 531: sky_gh graph and Harrow number resolution

The unlisted sky_gh WLD has five rooms and nine exits. Eight local targets
form one weak component of all five rooms; current numeric lookup resolves
those eight targets to different active Harrow rooms. D4 up from #29400 to
#153760 resolves only in unlisted Duris3 and has no reverse to sky_gh. No
active exit enters a sky_gh identity. Fifteen active exits also target
#29400–#29404, but 13 are Harrow-local and two Surface entrances resolve to
Harrow #29400. See the [sky_gh audit](SKY_GH_AUDIT_REVIEW.md),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-531-sky_gh-and-harrow-competing-identities),
[candidate dependency row 93](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L94),
and [coverage row 444](ZONE_COVERAGE_REGISTER.csv#L445).

### Checkpoint 532: solstice_gh seasonal map and Torg number resolution

The unlisted solstice_gh WLD has ten rooms and 19 exits. Eighteen local
targets form one weak component; current numeric lookup resolves them to
different active Torg rooms. The remaining D4 up exit from #29100 targets
#154578, “The Ocean” only in unlisted Duris3, with no return to the Solstice
range. No active exit enters a Solstice identity. Thirteen active Torg WLD
exits also target #29100–#29109, and its 32 ZON door/placement references
(9 D, 23 M) belong to Torg identities. The candidate adds no active-world
route totals. See the [solstice_gh audit](SOLSTICE_GH_AUDIT_REVIEW.md),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-532-solstice_gh-map-and-prototype-identities),
[candidate dependency row 94](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L95),
and [coverage row 445](ZONE_COVERAGE_REGISTER.csv#L446).

### Checkpoint 533: sunhall pyramid map and Underworld3 number resolution

The unlisted Sunhall WLD has ten rooms and 19 exits. Eighteen local targets
form one weak component, with all ten rooms reachable from #30200 and an
authored return to it; the remaining D4 up exit targets #158571. Current
numeric lookup resolves the 18 internal targets to different active
Underworld3 mining-tunnel rooms. The outward target is “Mountains” only in
unlisted Duris3 and has no return to #30200–#30209. Twenty-two active
Underworld3 WLD exits and four active Underworld3 ZON placements reuse the
range but remain Underworld3 identities. Candidate active inbound remains
zero; no active route totals change. See the [sunhall audit](SUNHALL_AUDIT_REVIEW.md),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-533-sunhall-underworld3-and-bastard-source-identities),
[candidate dependency row 95](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L96),
and [coverage row 446](ZONE_COVERAGE_REGISTER.csv#L447).

### Checkpoint 534: Surf full-source map and ambiguous variant identities

The unlisted Surf source has 90,000 rooms and 359,659 authored exits: 359,228
target its own room set and 431 point outside it. A full-source recount finds
415 outbound records targeting current active room VNUMs and 16 without an
active target definition. Three target only inactive asunwa/jungle rooms; the
other thirteen records target twelve VNUMs with no current WLD definition.
This corrects the older raw match note of 416 by one and adds no curated route.
The separate existing 11,987 / 415 global total is unchanged.

There are 145 active inbound exit records from 17 areas. Their VNUMs also
occur in surface2011 and underdark2011: 134 match Surf and surface2011, and
11 match all three candidates. Seventy-five have an authored reverse in Surf,
which does not establish a loaded reciprocal route. See the [Surf audit](SURF_AUDIT_REVIEW.md),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-534-surf-surface2011-and-underdark2011-map-identities),
[candidate dependency row 96](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97),
and [coverage row 447](ZONE_COVERAGE_REGISTER.csv#L448).

### Checkpoint 535: surface2011 full grid and historically listed map

The unlisted Norabi Map has 90,000 rooms and 360,000 authored exits, all
pointing within its own room set. It forms one weak component and has no
external WLD connector. Its 145 active inbound target records also occur in
Surf; 11 of those also occur in underdark2011. None can be assigned uniquely
to this map by VNUM. The prior active manifest listed surface2011 in 2011;
a 2013 merge-conflict change removed it without stating why. See the
[surface2011 audit](SURFACE2011_AUDIT_REVIEW.md),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-535-surface2011-and-surf-map-and-story-identities),
[candidate dependency row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L98),
and [coverage row 448](ZONE_COVERAGE_REGISTER.csv#L449).

### Checkpoint 536: surface2011-temp full map and duplicate boundary tuples

The WLD-only source has 90,000 rooms and 360,000 internal exits in one
weak component. Of its 240,016 raw target matches to active Surface VNUMs,
60,004 source room IDs also overlap active Surface but identify different
room records. A separate source-only filter yields 602 directions from 600
source rooms absent from the active map. Their exact source/direction/target
tuples also appear in newbiemaps.wld. These are existing source-only
references; no verified route or global count is added. See the
[surface2011-temp audit](SURFACE2011_TEMP_AUDIT_REVIEW.md),
[Surface approach review](SURFACE_AUDIT_REVIEW.md#inactive-wld-surface-approaches),
[candidate dependency row 98](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L99),
and [coverage row 449](ZONE_COVERAGE_REGISTER.csv#L450).

### Checkpoint 537: surfacekeeps_objects OBJ-only stub identity

The unlisted surfacekeeps_objects.obj has no WLD or room graph and adds no
active route. It duplicates 54 active SurfaceKeeps object VNUMs: 39 bodies
are line-identical and 15 differ. History says the file was added for stub
loading in 2010 and cleaned up in 2016. Preserve this object-source evidence
without changing active SurfaceKeeps ownership. See the
[surfacekeeps_objects audit](SURFACEKEEPS_OBJECTS_AUDIT_REVIEW.md),
[active SurfaceKeeps audit](SURFACEKEEPS_AUDIT_REVIEW.md#typed-references-and-source-ownership),
[candidate dependency row 99](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L100),
and [coverage row 450](ZONE_COVERAGE_REGISTER.csv#L451).

### Checkpoint 538: Sylvandawn room identities, source-only approaches, and Duris3 exits

The unlisted Sylvandawn WLD has 327 rooms and 693 directions. Its numeric
local graph has three weak components (325, 1, and 1); a directed walk from
#8001 reaches 320 rooms. Two exits point to unregistered Duris3 rooms:
#8087 west to #163980 and #8325 east to #164082. The other 691 target records
are within Sylvandawn's own WLD VNUM set.

Sylvandawn and active Temple of Laduguer share room numbers #8100–#8202, but
all 103 full room records and titles differ. The existing Checkpoint 381
review has 223 raw Sylvandawn directions into that numeric band: 17 begin in
Sylvandawn-only room IDs and are source-only approaches to Temple; 206 begin
in IDs also owned by active Temple and remain ambiguous same-number
transitions. None is added again to the global inactive-WLD count. Active
WLD targets in the overlap resolve to Temple's current room identities, so
Sylvandawn-specific active ingress remains zero. No fall or current
directives were found. See the [Sylvandawn audit](SYLVANDAWN_AUDIT_REVIEW.md),
[Temple approach review](TEMPLE_LADUGUER_AUDIT_REVIEW.md#inactive-sylvandawn-approaches-and-room-identity),
[candidate dependency row 100](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L101),
and [coverage row 451](ZONE_COVERAGE_REGISTER.csv#L452).

### Checkpoint 540: tharnadia1 is a shop source with no WLD graph

The unlisted `tharnadia1` component is one SHP file with 18 shop records and
no WLD rooms or authored exits. Keep its shop `in_room` fields as typed
location dependencies; they are not map edges. This includes source-only
shops #6025/#6066 naming active Nomads rooms #6242/#6243 and shop #6113
naming active Mist room #6358. Candidate-specific WLD ingress and graph
counts remain zero/not applicable. See the
[tharnadia1 audit](THARNADIA1_AUDIT_REVIEW.md),
[Cimmerian Nomad review](CIMMERIAN_NOMAD_ENCAMPMENT_AUDIT_REVIEW.md),
[Shadow Forest review](SHADOW_FOREST_AUDIT_REVIEW.md),
[candidate dependency row 102](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L103),
and [coverage row 453](ZONE_COVERAGE_REGISTER.csv#L454).

### Checkpoint 541: tharold is a shop-only source with no WLD graph

The unlisted `tharold` component is one SHP file with twelve shop records and
no WLD rooms or exits. Treat its `in_room` fields as typed shop-location
references, not map edges. Its twelve keeper/room pairs are also present in
`tharnadia1.shp`; those pairs do not create routes or candidate WLD ingress.
Preserve the zero candidate-specific active-WLD inbound count. See the
[tharold audit](THAROLD_AUDIT_REVIEW.md),
[tharnadia1 audit](THARNADIA1_AUDIT_REVIEW.md),
[candidate dependency row 103](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L104),
and [coverage row 454](ZONE_COVERAGE_REGISTER.csv#L455).

### Checkpoint 539: temple2 graph stays distinct from active Longhollow

The unlisted temple2 WLD has 89 rooms and 196 authored exit records. All
targets stay inside its source room set; the numeric graph has one weak
component, and a directed walk from #34400 reaches all 89 rooms when every
authored direction code is included. Fourteen exits use direction values
6–9; confirm their meaning with the map and loader owners before using this
walk as a movement conclusion. No fall or current directives were found.

All 89 candidate room numbers are also active Longhollow room numbers, but
the paired titles and full records differ. A raw scan finds 229 active WLD
target records into the shared numeric room set: 227 from Longhollow and two
Surface approaches to #34401. Under current area membership those targets
resolve to Longhollow, so temple2-specific active ingress remains zero. Keep
these collisions separate from live routes and from the inactive-WLD total.
See the [temple2 audit](TEMPLE2_AUDIT_REVIEW.md),
[Longhollow audit](LONGHOLLOW_AUDIT_REVIEW.md),
[candidate dependency row 101](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L102),
and [coverage row 452](ZONE_COVERAGE_REGISTER.csv#L453).

### Checkpoint 542: theurgist is a four-room local map with no candidate ingress

The unlisted `theurgist` WLD has four rooms and six authored exits, all local
and resolving. Its graph has one weak component of four rooms, with raw
directed reach from #1 to every room. The matching `areas/map/theurgist.json`
places the same four room IDs in a compact layout. Current active Limbo owns
room #1 and active Heavens owns #3/#4 with different records; #2 has no active
room owner. Ten active exits target shared numbers #1/#4, but those destinations
resolve to the active Limbo/Heavens identities, leaving zero candidate-specific
ingress. See the [theurgist audit](THEURGIST_AUDIT_REVIEW.md),
[20 builder questions](THEURGIST_BUILDER_QUESTIONS.md),
[candidate dependency row 104](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L105),
and [coverage row 455](ZONE_COVERAGE_REGISTER.csv#L456).

### Checkpoint 543: Tiamats Lair legacy graph is not active ingress

The former `tiamats_lair` WLD has 83 rooms and 200/200 locally resolving
exits. Its weak components contain 74, 8, and 1 rooms; directed reach from
#135201 covers 74. All 83 room numbers also exist in active Azhural, so the
200 legacy exits start in same-numbered former rooms and target that old
internal map. They do not establish a second active arrival; candidate-specific
active ingress remains zero. See the [Tiamats Lair audit](TIAMATS_LAIR_AUDIT_REVIEW.md),
[20 builder questions](TIAMATS_LAIR_BUILDER_QUESTIONS.md),
[candidate dependency row 105](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L106),
and [coverage row 456](ZONE_COVERAGE_REGISTER.csv#L457).

### Checkpoint 544: torr is a 300-room source map with one outbound Chasm edge

The unlisted `torr` WLD has 300 rooms and 634 authored exits: 633 local and
one outbound from #79092 south to active Mist Chasm #15052. All 634 resolve
against the candidate or current full-area WLD sources. The local graph has
one weak component; a directed walk from #79000 reaches 293 rooms, leaving
#79264–#79270 outside that walk. Three rooms carry fall chances and no current
directives were found. The two tunnel links between #79157 and #79158 use
loader-supported D8/D7 directions. No active WLD exit targets a `torr` room
VNUM, so candidate-specific active ingress remains zero. The outbound Chasm
edge was already included in the prior Chasm source-only review and does not
change its curated total. See the [Torr audit](TORR_AUDIT_REVIEW.md),
[20 builder questions](TORR_BUILDER_QUESTIONS.md),
[candidate dependency row 106](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L107),
and [coverage row 457](ZONE_COVERAGE_REGISTER.csv#L458).

### Checkpoint 545: Tradeskills old has no WLD graph or room ingress

`tradeskills.old.obj` is OBJ-only, so room-graph metrics and candidate-room
reachability do not apply. Its #400220 type-25 record names room #554045, but
that object-to-room declaration is not a WLD exit and adds no candidate room
or active WLD ingress. The old and active #400220 records share the target but
have different numeric flags; current corpse-portal code consumes the active
VNUM. Keep that typed object reference distinct from authored world
connectivity. See the
[old-catalog audit](TRADESKILLS_OLD_AUDIT_REVIEW.md),
[20 builder questions](TRADESKILLS_OLD_BUILDER_QUESTIONS.md),
[Surface target review](SURFACE_AUDIT_REVIEW.md#L43),
[candidate dependency row 107](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L108),
and [coverage row 458](ZONE_COVERAGE_REGISTER.csv#L459).

### Checkpoint 546: Twinkeep has no WLD graph or room edges

`twinkeep.shp` has no candidate WLD component, so a room graph and candidate
exit reachability do not apply. Its #6500 record is a roaming shop with
`in_room` 0; its #6519 record names active Shaughin2 room #6656 in the typed
shop-location field. That location is not a WLD edge. The product #6537 is
also not a room route: the VNUM exists as a Shaughin WLD room but has no
current OBJ definition. Candidate-specific active WLD ingress remains zero.
See the [Twinkeep audit](TWINKEEP_AUDIT_REVIEW.md),
[20 builder questions](TWINKEEP_BUILDER_QUESTIONS.md),
[candidate dependency row 108](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L109),
and [coverage row 459](ZONE_COVERAGE_REGISTER.csv#L460).

### Checkpoint 547: ud map graph and unregistered approaches

The unregistered `ud.wld` defines 17,462 rooms and 50,291 exits. Of these,
50,018 target another ud room; 273 positive targets are absent from every
current WLD source. The authored graph has 11 weak components, with largest
sizes 7,143, 5,978, and 3,863. All 18 active exits from `mobs_underdark`
target unique ud room VNUMs and remain unresolved in the active manifest.
Separately, inactive `surf.zon` line 162 resets mobile #100116 into ud room
#349161; this is a typed source-only placement, not an active WLD edge. The ud
WLD has no target to another active or current WLD source, so no active
outbound or return route is established. See the [ud audit](UD_AUDIT_REVIEW.md),
[20 builder questions](UD_BUILDER_QUESTIONS.md),
[candidate dependency row 109](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L110),
and [coverage row 460](ZONE_COVERAGE_REGISTER.csv#L461).

### Checkpoint 548: ud-inside is a one-room Limbo-numbered placeholder

The unregistered `ud-inside.wld` defines only room #1, with an empty
description and zero exits. Its local graph is one singleton; there is no
candidate-authored outbound edge. Twelve WLD files define #1, but active Limbo
is the only loaded owner and `ROOM_LIMBO_VNUM` is 1. Nine active Desert
directions from #49790 target that number and are already recorded as
Desert-to-Limbo declarations, with no reverse direction. Candidate-specific
active ingress remains zero. Fourteen more #1 directions and four zone room
commands come from unregistered sources; they are preserved as source-only
numeric references, not `ud-inside` routes. See the
[ud-inside audit](UD_INSIDE_AUDIT_REVIEW.md),
[20 builder questions](UD_INSIDE_BUILDER_QUESTIONS.md),
[cross-area rows](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L630),
[candidate dependency row 110](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L111),
and [coverage row 461](ZONE_COVERAGE_REGISTER.csv#L462).

### Checkpoint 549: ud_map0 source graph and boundaries

The unregistered ud_map0.wld defines 2,708 rooms and 10,837 authored exits:
7,767 local, six to active WLD sources, and 3,064 positive targets absent
from every current WLD source. The local graph is one weak component; its
authored directed graph reaches all rooms from either #1104328 or #1106354.
Two active Shafts exits target those rooms, but both remain unresolved because
ud_map0 is absent from areas/AREA. The six exits from ud_map0 to active maps
are source-only approaches: three to Shafts (#10011, #10020, #10003), one to
Myconid #2300, one to Underworld #4469, and one to Ghore #11548. The Ghore
approach is the newly reconciled tuple; it raises curated inactive-WLD totals
to 11,988 overall / 415 from Surf. Source topology does not establish live
access, a safe route, or a return. See the
[ud_map0 audit](UD_MAP0_AUDIT_REVIEW.md),
[20 builder questions](UD_MAP0_BUILDER_QUESTIONS.md),
[candidate dependency row 111](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L112),
and [coverage row 462](ZONE_COVERAGE_REGISTER.csv#L463).

### Checkpoint 550: ud_map1 source graph and approaches

The unregistered `ud_map1.wld` defines 2,425 unique rooms and 9,705 authored
exits: 8,611 local, seven to active WLD sources, and 1,087 positive targets
absent from every current WLD source (654 distinct missing targets). There are
no negative targets. The local graph has one weak component; raw directed
source traversal from Shafts target #1116916 reaches all 2,425 rooms. This is
static authored topology only and does not establish live travel, safety, or
return. There are no fall-chance or current fields.

The seven source-only exits to active maps are #1111610 down to Shafts #10025,
#1112933 down to Shafts #10035, #1113221 north to Valinhav #90800, #1115029 up
to Pragtog #10649, #1116916 up to Shafts #10004, #1117360 east to Domain
#36800, and #1117763 up to Shafts #10000. The Valinhav edge is a new
source-only approach, not active ingress. One active Shafts exit, #10004 down
to candidate #1116916, remains unresolved because `ud_map1` is not in
`areas/AREA`; the candidate has the reciprocal authored up edge.

The ZON records four source-only M resets to active Underworld mobile #4520 at
candidate rooms #1111555, #1113136, #1115329, and #1118722. The normalized
ZON title is unique, the three ud_map room sets are disjoint, and no ud_map1
room VNUM overlaps another current WLD source. Retain the map in place pending
owner, editor, deployment, reset, and persisted-identity review. See the
[ud_map1 audit](UD_MAP1_AUDIT_REVIEW.md), [20 builder
questions](UD_MAP1_BUILDER_QUESTIONS.md), [coverage row
463](ZONE_COVERAGE_REGISTER.csv#L464), and [candidate dependency row
112](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L113).

### Checkpoint 551: ud_map2 source graph and boundaries

The unregistered ud_map2.wld defines 6,155 unique rooms and 24,594 authored exits: 22,839 local, 27 to active WLDs, one to unregistered VNUM #30300, 1,722 positive targets absent from every current WLD source, and five negative #-1 targets. Eighty unresolved target records extend beyond ZON top #1129479. The local graph has nine weak components; its largest contains 6,146 rooms. Raw directed source traversal from Shafts target #1121446 reaches 6,142 rooms; reverse traversal reaches 6,146. No fall or current fields are present. These are static authored edges and do not establish live access, safety, or return.

There is one active inbound Shafts exit, #10012 south to #1121446, and no candidate-side return exit. Five more inbound records originate in unregistered bastard, bb_gh, genocide_hall, venom, and vodgh WLDs; the three #30300 sources remain numerically ambiguous. The 27 source-only approaches to active areas include newly reconciled exits to Llzazan, Valinhav, Clawed Caverns, and Truk. All remain non-active ingress while ud_map2 is omitted from areas/AREA; the reviewed subset rises from 23 to 27 and curated global total from 11,989 to 11,993 (Surf remains 415). Preserve each route with the source map. See the [ud_map2 audit](UD_MAP2_AUDIT_REVIEW.md), [builder questions](UD_MAP2_BUILDER_QUESTIONS.md), [coverage row 464](ZONE_COVERAGE_REGISTER.csv#L465), and [candidate dependency row 113](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114).


### Checkpoint 552: underdark2011 source graph and boundaries

The unregistered underdark2011.wld defines 15,482 rooms and 61,931 exits: 50,697 target locally, ten leave for active WLDs, and 11,224 leave for unregistered map sources. Of those 11,224 records, 11,222 have 6,826 target VNUMs present in both Surf and Surface2011; two point to unregistered Jungle #67005 and #67087. Every positive target exists somewhere in the current WLD corpus, but duplicated inactive maps do not establish target ownership. The graph has 26 weak components, largest 4,154 rooms, with no fall or current fields.

All ten exits to active areas are source-only and have no direct reverse exit in the active destination: #224716 east to IceCrag #97001; #232202 down to Divhome #40780; #249233 east and #249534 north to Herders #94300; #250432 south and #250731 east to Grave #8400; #255552 north to Githzerai Stronghold #44400; #272717 down to Troll Caves #96900; #274261 up to Faang #15411; and #278150 down to Kvarkpass Connector #54804. These ten records are already included in the curated 11,993 source-only total; do not add them a second time. The Surf subset remains 415. Exact source rows and target-owner reviews are in the [UnderDark2011 supplement](UNDERDARK2011_AUDIT_REVIEW.md#candidate-wld-graph-and-boundaries).

Eleven active-source exits also target candidate room numbers: one Ashrumite, five Connectorzones, one Crypt, one Desolate Under Fire, and three Patrols records. Their active exit-register rows remain positive-unresolved because the same target numbers also occur in Surf and Surface2011 under different room identities. Preserve all eleven as ambiguous candidates; none proves live ingress. Desolate Under Fire #77301 south to #217609 remains especially ambiguous. The candidate has no reset commands. See the [map identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-552-underdark2011-map-and-prototype-identity) and [coverage row 465](ZONE_COVERAGE_REGISTER.csv#L466).

### Checkpoint 553: underworld2 source graph and boundaries

The unregistered underworld2.wld defines 8,256 unique rooms and 32,603 exits:
32,592 target another underworld2 room and eleven target active WLD owners.
There are no negative or absent-corpus target VNUMs, no room-number overlap
with another current WLD source, and no active or unregistered-WLD inbound
exit. The local graph is one weak component. From #415402, raw directed
forward traversal reaches 8,245 rooms and reverse traversal reaches all 8,256;
twelve SCCs comprise one component of 8,245 and eleven singleton rooms. This
is source topology only; it does not establish live travel, safety, or return.

The eleven source-only, one-way exits to active WLDs are #417516 north to
Cloister #67100; #417529 north to Fort Boyard #38100; #421086 west to Lab
#82350; #421439 east to Githzerai Stronghold #44400; #422044 north to Nakral’s
Crypt #14300; #425645 west to Sevenoaks #38600; #427709 east to Pits of
Cerberus #22118; #429194 south to Tezcat #98900; #431340 west to Dragonnia
#6803; #436465 east to Forgotten Mansion #3500; and #437928 south to Temple
of the Earth #43500. The Fort Boyard record was already present in destination
evidence; this candidate audit corrects the source-specific count to eleven
and adds no route to the curated 11,993 overall / 415 from Surf total. No
candidate room is loaded while underworld2 remains outside areas/AREA. See the
[underworld2 audit](UNDERWORLD2_AUDIT_REVIEW.md), [30 builder questions](UNDERWORLD2_BUILDER_QUESTIONS.md),
[coverage row 466](ZONE_COVERAGE_REGISTER.csv#L467), and
[candidate dependency row 115](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L116).

### Checkpoint 554: Venom GH source graph and boundaries

The unregistered `venom.wld` defines ten rooms #30300–#30309 and 16 authored
exits: 15 local and one up from #30300 to unregistered ud_map2 #1124260. The
local graph is one weak component; raw directed traversal from #30300 reaches
four rooms, while reverse traversal reaches all ten. Its four SCCs have sizes
4, 3, 2, and 1. There are no negative targets, fall/current fields, exit
flags/keys, or active WLD inbounds.

All ten Venom room VNUMs also occur in `bb_gh.wld` and `vodgh.wld` with
different room records. One unregistered ud_map2 exit, #1126542 down (`D5`),
targets the shared #30300. That direction pairs numerically with `bb_gh.wld`
#30300 up to ud_map2 #1126542; the ud_map2 target cannot distinguish the three
map owners. It is not active ingress. Venom’s separate #30300 up exit targets
ud_map2 #1124260 and has no reverse from that exact room. This source-only
link adds no active-area approach to the curated global total of 11,993 / 415
from Surf. See the [Venom audit](VENOM_AUDIT_REVIEW.md), [30 builder
questions](VENOM_BUILDER_QUESTIONS.md), [coverage row 467](ZONE_COVERAGE_REGISTER.csv#L468),
and [candidate dependency row 116](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L117).

### Checkpoint 555: vodgh source graph and boundaries

The unregistered `vodgh.wld` defines 14 rooms #30300–#30313 and 26 exits:
24 local, one up from #30300 to unregistered `ud_map2` #1123336, and one up
from #30310 to active Magma #142034. The local graph has two weak components
of ten and four rooms. From #30300, raw directed traversal reaches all ten
rooms in its component; reverse traversal also reaches ten. SCC sizes are 10
and 4, matching the two weak components. There are no negative targets, exit
flags/keys, fall fields, or current fields.

No active WLD exit targets #30300–#30313. One unregistered `ud_map2` exit,
#1126542 down (`D5`), targets the shared #30300; it cannot distinguish
`bb_gh`, `venom`, or `vodgh`. `vodgh` #30300 points up to the different
`ud_map2` room #1123336. The #30310 up exit to active Magma #142034 was
recorded in Checkpoint 145 and adds no route to the current 11,993 overall /
415 from Surf source-only tally. No candidate edge establishes active ingress
or safe travel. See the [VoDsGH audit](VODGH_AUDIT_REVIEW.md), [30 builder
questions](VODGH_BUILDER_QUESTIONS.md), [coverage row 468](ZONE_COVERAGE_REGISTER.csv#L469),
and [candidate dependency row 117](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L118).

### Checkpoint 556: void_gh source graph and boundary identities

The unregistered `void_gh.wld` defines 19 rooms #29100–#29118 and 33 exits:
30 target local room VNUMs and three leave the map. The local graph has four
weak components and four SCCs sized 10, 3, 3, and 3. From #29100, forward and
reverse traversal each reach all ten rooms in its component. All 30 local
targets numerically resolve to same-number rooms in active `torg.wld`, but
those are Torg's distinct room records; this is not an active Void route.

The three source-only boundaries are D4 up from #29100 to unregistered
Duris3 #154553 (“Grassy Plains”), and D4 up from #29113 and #29116 to
unregistered Duris3 #163943 (“Rolling Hills”). Neither exact Duris3 target
room has a checked-in direction back to #29100–#29118. No active WLD exit
targets a Void-specific room identity. Active Torg has 21 exits whose target
VNUMs fall in the Void range; they resolve to Torg's own same-number rooms.
Other unregistered guildhall sources likewise target their own overlapping
room IDs. These are numeric collisions, not active Void ingress. The three
Duris3 outlets add no route to the curated 11,993 overall / 415 from Surf
source-only totals. Code also binds `inn` to raw #29103 and Torg's dynamic
invasion logic to raw #29116; both resolve to active Torg room identities,
not Void. Preserve the exits as source declarations until both map owners
confirm destination and return intent. See the [Void Guildhall audit](VOID_GH_AUDIT_REVIEW.md),
[36 builder questions](VOID_GH_BUILDER_QUESTIONS.md), [coverage row 469](ZONE_COVERAGE_REGISTER.csv#L470),
and [candidate dependency row 118](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L119).

### Checkpoint 557: waterplane candidate graph and active identities

The unregistered `waterplane.wld` defines twelve rooms (#1–#10, #12, and #72)
and 30 directions: 20 target Waterplane room numbers and ten D2 exits into
active Air. Its numeric local graph has weak and strongly connected component
sizes 10, 1, and 1; #1 reaches all ten rooms #1–#10 forward and in reverse,
while #12 and #72 are isolated. There are no negative targets, door flags,
key VNUMs, fall fields, or current fields. The three M resets all place
Waterplane mobile IDs #1–#3 at isolated room #72.

Twenty-one target VNUMs occur in current active WLD sources; nine target the
candidate-only values #2 and #6–#9. Active Limbo owns Waterplane room #1, and
active Heavens owns #3, #4, #5, #10, and #12, each with a different full WLD
record. No active WLD exit targets Waterplane-specific rooms #2, #6–#9, or
#72, so candidate-specific active ingress remains zero. The five Air exits
from source IDs #1, #3, #4, #5, and #10 remain identity overlaps. Five more
unique-source exits from #6–#9 target active Heavens #4, #5, and #10 even
though those numbers are also local Waterplane IDs. Along with the five
unique-source Air exits from #2 and #6–#9, these ten source-only records were
counted in Checkpoint 321; no route count changes here.

The broader low-ID scan found 87 directions from seven other unregistered
WLD sources into #2, #6–#9, and #72. Every target is also present in the same
source WLD, so these are source-local loops, not Waterplane approaches. The
full review also covers the WLD/MOB collisions, the active Heavens reset that
names shared mobile #2, title and history, and the empty OBJ/no-QST/no-SHP
dependency boundary. Keep all four candidate files on existing row 120.
Curated source-only totals remain 11,993 overall / 415 from Surf. See the
[Waterplane audit](WATERPLANE_AUDIT_REVIEW.md),
[24 builder questions](WATERPLANE_BUILDER_QUESTIONS.md),
[coverage row 470](ZONE_COVERAGE_REGISTER.csv#L471), and
[candidate dependency row 119](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L120).

### Checkpoint 558: wlust_gh candidate graph and active identities

The unregistered `wlust_gh.wld` defines ten rooms #29100–#29109 and 19
directions: 18 local-number edges and D4 from #29100 to Duris3 #154553
(“Grassy Plains”). The candidate-local graph is one weak component and one
SCC of ten; #29100 reaches all ten rooms in both directions. All 18 local
target numbers occur in active `torg.wld`, but those resolve to Torg's
different room identities. The full manifest gives Torg the *289–291 band,
and 13 active Torg exits target #29100–#29109; they remain Torg-to-Torg
routes, not Wlust ingress. Candidate-specific active inbound remains zero.

Duris3 #154553 has no checked-in return to #29100–#29109. The same target is
declared by `void_gh` #29100 D4, but both declarations point only to inactive
Duris3 source data. No active route is added to the curated 11,993 overall /
415 from Surf totals. The review records zero negative targets, out-of-range
rooms, range-mismatch exits, falls, and currents; one positive unresolved
target and one reference to the unregistered WLD stem `Duris3`.

See the [Wlust audit](WLUST_GH_AUDIT_REVIEW.md), [28 builder
questions](WLUST_GH_BUILDER_QUESTIONS.md),
[coverage row 471](ZONE_COVERAGE_REGISTER.csv#L472), and
[candidate dependency row 120](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L121).

### Checkpoint 559: Yggdrasil graph and active identities

The unregistered ygg.wld defines thirteen rooms #30400–#30412 and 24
directions: 22 target local room numbers, one D4 from #30400 to active
SurfaceKeeps #123522, and one D4 from #30410 to unregistered Duris3 #157535.
Its numeric graph has two weak and strongly connected components sized ten
and three. Forward traversal reaches ten rooms from #30400 and three from
#30410. All thirteen room VNUMs are outside the conventional #900–#999 range
for header #9, and all 24 directions originate in those range-mismatch rooms.
No negative targets, fall fields, or current fields were found. Preserve the
source mismatch until the area owner confirms the intended header and VNUMs.

All 22 local target numbers are also active Ard’gral rooms #30401–#30412,
with different room records. Forty-eight active Ard’gral directions target
those numbers and remain Ard’gral-to-Ard’gral edges. No active WLD exit
resolves to a Yggdrasil-specific identity, so candidate active inbound is
zero. The ygg #30400 D0 to #30401 record is both a numeric local target and a
match to active Ard’gral #30401; it remains owner-ambiguous.

Twenty-three of the 24 authored target records resolve numerically to current
active WLD owners: the 22 local-number records resolve to Ard’gral and the
#123522 record resolves to SurfaceKeeps. The remaining #157535 target belongs
only to unregistered Duris3.

The #30400 D4 to SurfaceKeeps #123522 route was recorded in Checkpoint 374;
the exact SurfaceKeeps room has no return to Yggdrasil. The #30410 D4 to
Duris3 #157535 target also has no checked-in return to #30401–#30412, and
Duris3 is unregistered. The full inactive-WLD source-only table reports two
ygg records: #30400 to #123522 and the owner-ambiguous #30400 to #30401.
These are already included in 11,993 overall / 415 from Surf. The
Duris3-bound exit is not in that active-target recount. None of these records
qualifies as a new confirmed player route. See the
[Yggdrasil audit](YGG_AUDIT_REVIEW.md),
[28 builder questions](YGG_BUILDER_QUESTIONS.md),
[coverage row 472](ZONE_COVERAGE_REGISTER.csv#L473), and
[candidate dependency row 121](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L122).

### Checkpoint 562: retired player_ships room graph and remap

The parent of retirement commit `857124f14c648b26031a2a40c6335e846bb94b9d`
contains two `player_ships.wld` rooms: #50000, Davy Jones Locker, and #50001,
“Speeding Across The Ocean.” Each has zero authored exits, so the historical
map consists of two singleton weak components with no WLD path between them.
Current full and inactive WLD sources contain no destination to either old room
VNUM. The old MOB and OBJ files were empty, and the old ZON had no resets.

The 2016 change removed that contributor and reused active `ship.wld` rooms
#60000 and #60001 for the Davy Jones and transit functions. Ship constants
changed from the old 50000/50001 identities and 50001–50999 dynamic pool to the
current #60000–#64999 ship-room system. Those native/code-owned arrivals are
separate from authored WLD edges; this static review does not establish a live
ship or route. No active or source-only WLD count changes. The stale
`areas/qst/AREA` row `player_ships *500-509` is not a WLD edge or proof of a
current QST file. See the [Ship area audit](SHIP_AREA_AUDIT_REVIEW.md), its
[retirement and index questions](SHIP_AREA_AUDIT_REVIEW.md#builder-questions-and-conservative-recommendations),
the [replacement record](DUPLICATE_AND_REPLACED_ZONES.md#player-ships), and
[coverage row 475](ZONE_COVERAGE_REGISTER.csv#L476).

### Checkpoint 563: Siege historical graph and retirement

The pre-retirement `siege.wld` contained one room, #401000 “The Siege Room,”
with no authored exits. It was a one-node, zero-edge graph. A snapshot scan
found no other WLD target to #401000; current full and inactive WLD sources
also contain no such target. The retired `siege.zon` had no reset commands.

Ten quartermaster M resets from Tharnadia, Woodseer, Ashrumite, Charing,
Kimordril, Faang, Ghore, Goblinht, Khildarak, and Shady targeted separate
mobile IDs in their town rooms; they were not WLD paths and retirement removed
them. Follow-up commit `bf2ef4ab3` removed the dormant warmaster bindings and
Siege runtime source. Current exact scans find no remaining #401000, #401001,
or #401010–#401090 references under `areas/` or `src/`. Zone 4010 remains
reserved as `*RETIRED 4010 (siege; do not reuse)`; no graph, active inbound, or
source-only count is added. See the [Siege audit](SIEGE_AUDIT_REVIEW.md),
[replacement review](DUPLICATE_AND_REPLACED_ZONES.md#siege), and
[coverage row 476](ZONE_COVERAGE_REGISTER.csv#L477).

### Checkpoint 565: Tempest Court graph and Air boundary

`airp` has 200 rooms and 1,074/1,074 loader-resolved source exits. Its local
weak graph has eight components; the largest contains 121 rooms. Its sole
active inbound and outbound edges form the reciprocal boundary
`plane_air_one.wld` #24422 up ↔ `airp.wld` #131600 down. The static source
census has no negative or unresolved targets, numeric range mismatches,
unregistered WLD targets, fall-chance rooms, or currents. The reciprocal pair
is preserved in cross-area summary rows 4 and 348. The Tempest Court's portals,
air-plane access limits, resets, and source availability have separate
qualification boundaries; resolved WLD edges alone do not establish live
reachability or safe return. Keep this active area DO NOT MOVE. See the
[Tempest Court audit](TEMPEST_COURT_AUDIT_REVIEW.md), [story dossier](../zone-stories/TEMPEST_COURT.md),
[Plane of Air audit](PLANE_OF_AIR_AUDIT_REVIEW.md), and [coverage row 2](ZONE_COVERAGE_REGISTER.csv#L3).

### Checkpoint 566: Dawndale graph and Surface connections

`airshipgrave` has 150 rooms and 311/311 loader-resolved source exits. Its
local weak graph is one component containing all 150 rooms. Three inbound and
three outbound active WLD records form reciprocal pairs with Surface:
Dawndale #77543 north ↔ Surface #616080 south; #77557 east ↔ #617684 west;
and #77557 south ↔ #618083 north. The static source census has no negative or
unresolved targets, numeric-range mismatches, or unregistered WLD targets.
The separate object portal #77521 declares Surface #522469; it is not a fourth
WLD boundary or proof of safe return. See the [Dawndale audit](AIRSHIPGRAVE_AUDIT_REVIEW.md),
[story dossier](../zone-stories/THE_MOUNTAIN_VALLEY_OF_DAWNDALE.md),
[Surface audit](SURFACE_AUDIT_REVIEW.md), and [coverage row 3](ZONE_COVERAGE_REGISTER.csv#L4).

### Checkpoint 568: Alatorin's loaded graph and city cross-boundaries

Alatorin has 952 rooms and 2,128/2,128 loader-resolved source exits. Its local
weak graph has 47 components, the largest with 288 rooms. There are 81 inbound
and 85 outbound cross-area WLD directions, with 74 reverse-direction records.
Outbound targets are Surfacekeeps (76), Surface (4), Connector Zones (1), and
the Air, Earth, Fire, and Water Planes (one each). Inbound sources are
Surfacekeeps (74), Surface (4), Connector Zones (1), and White Harbor (2).
Surfacekeeps is a separately loaded but intentional city connection; these
records are not unresolved same-file targets.

A raw directed traversal over all active WLD files keys each room by source
stem and VNUM and assigns cross-area targets by the resolved-owner register.
It follows resolved local and cross-area exits only, treating every authored
direction as passable and omitting unresolved/negative records. From Alatorin
#83100 it reaches 213,687 current active room records; 899/952 Alatorin rooms
are reachable, 896/952 have a raw path back to the root, and 890 lie in the
same root strongly connected component. This is a static map result, not proof
of passage through locked doors, sector restrictions, combat, resets,
type-25 objects, or dynamic routes. See the [Alatorin audit](ALATORIN_AUDIT_REVIEW.md),
[cross-area summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L7), and [coverage row 5](ZONE_COVERAGE_REGISTER.csv#L6).

### Checkpoint 779: Caertannad ITEM_TELEPORT and ITEM_SWITCH fields

The active Caer Tannad WLD census remains 958/958 loader-resolved authored
exits, with 25 inbound and 74 outbound cross-area WLD directions (24 reverse
direction records). These do not count item-based travel or switch fields. The
zone defines ten type-25 ITEM_TELEPORT objects: nine target local rooms, and
#78510 targets active Dracopit room #15907. Its type-29 object #78519 is an
ITEM_SWITCH; value[0] is command ID 340 and value[1] is local affected room
#78781. It does not reference Dalvik room #340 and is not a portal. A full
active/candidate object scan found no external ITEM_TELEPORT destination or
ITEM_SWITCH value[1] room field into Caer Tannad. No WLD or curated source-only
exit totals change. See [Caertannad typed audit](CAERTANNAD_AUDIT_REVIEW.md),
[coverage row 44](ZONE_COVERAGE_REGISTER.csv#L45), and the
[Caertannad story dossier](../zone-stories/TWIN_KEEPS_OF_DEVASTATED_THARNADIA.md).

### Checkpoint 780: Centaur switch fields remain local

Centaur's five type-29 ITEM_SWITCH objects reference local affected rooms
through value[1]: #93305 → #93377, #93306 → #93375, #93307 → #93363,
#93308 → #93365, and #93323 → #93326. Their repeated value[0] of 270 is the
trigger command ID, not a room target or an edge to candidate Dalvik room
#270. A current active/candidate scan found no external ITEM_SWITCH room
field or ITEM_TELEPORT destination into Centaur. These switch declarations are
separate from the authored WLD graph; no active or source-only WLD exit total
changes. See the [Centaur typed audit](CENTAUR_VILLAGES_AUDIT_REVIEW.md),
[coverage row 49](ZONE_COVERAGE_REGISTER.csv#L50), and [existing switch-route
owner question](../zone-stories/CENTAUR_VILLAGES.md#direct-area-owner-questions-and-conservative-recommendations).

### Checkpoint 781: Clawed Caverns typed fields and source-only approach

Clawed Caverns' object #80747 is a type-29 ITEM_SWITCH whose value[1] room
target #80773 is local; value[0] is push command ID 270. The zone defines no
local ITEM_TELEPORT target, and no active/candidate ITEM_TELEPORT destination
or ITEM_SWITCH value[1] field into Clawed Caverns was found. The fifteen ZON G
commands for Heavens object #827 are typed object imports, not WLD edges. Keep
the prior `ud_map2.wld` #1124947 north approach to #80700 identified as
source-only, not active ingress. No WLD or curated source-only exit total
changes. See the [Clawed Caverns typed audit](CLWCVRN_AUDIT_REVIEW.md),
[coverage row 61](ZONE_COVERAGE_REGISTER.csv#L62), and the
[ud_map2 audit](UD_MAP2_AUDIT_REVIEW.md).


### Checkpoint 782: Ultarium item travel remains local

Ultarium has four type-25 ITEM_TELEPORT objects targeting local rooms
#76228, #76002, #76218, and #76217. Its two type-29 ITEM_SWITCH objects use
command ID 270 and target local rooms #76134 and #76185 through value[1]. No
active/candidate teleport destination or switch room field into Ultarium was
found. These fields and the Heavens reset imports are separate from the WLD
graph; no active or curated source-only exit total changes. See the [Cosmic
typed audit](COSMIC_AUDIT_REVIEW.md), [coverage row 63](ZONE_COVERAGE_REGISTER.csv#L64),
and the [Ultarium dossier](../zone-stories/ULTARIUM.md).


### Checkpoint 783: Court item travel stays within the zone

Court's five type-25 ITEM_TELEPORT objects target local rooms #6796, #6720,
#6770, #6706, and #6713. It has no local type-29 ITEM_SWITCH and no active or
candidate teleport/switch room field into Court. These object destinations
are separate from the authored WLD graph; no WLD or curated source-only exit
total changes. See the [Court typed audit](COURT_AUDIT_REVIEW.md),
[coverage row 64](ZONE_COVERAGE_REGISTER.csv#L65), and the
[Court dossier](../zone-stories/COURT_OF_THE_MUSE.md).

### Checkpoint 784: Crakkaro item switch stays local

Crakkaro object #87062 is a type-29 ITEM_SWITCH with command ID 346,
value[1] room #87418, direction 5 (down), and move flag 1. The command ID
is not a room VNUM. This is a local object-to-room field, not a WLD exit.
Crakkaro has no local type-25 ITEM_TELEPORT destination, and no active or
candidate type-25 destination or type-29 value[1] field targets Crakkaro.
No active or curated source-only WLD exit total changes. See the
[Crakkaro typed audit](CRAKKARO_AUDIT_REVIEW.md),
[coverage row 65](ZONE_COVERAGE_REGISTER.csv#L66), and the
[Crakkaro dossier](../zone-stories/CRAKKAROS_LIAR.md).

### Checkpoint 785: Delwyn item travel has no external arrival

Delwyn has no local type-25 ITEM_TELEPORT or type-29 ITEM_SWITCH. The scan
of 872 external type-25 and 380 external type-29 declarations found no
destination or value[1] room field into Delwyn. The reciprocal Surface gate
and the source-only Nexus56 approach from #83007 west to #83006 remain as
previously counted; the candidate’s reset-room VNUM overlap is not a WLD
edge. No active or curated source-only exit total changes. See the
[Delwyn typed audit](DELWYN_AUDIT_REVIEW.md),
[coverage row 70](ZONE_COVERAGE_REGISTER.csv#L71),
[Nexus56 audit](NEXUS56_AUDIT_REVIEW.md#checkpoint-785-delwyn-room-reset-fields-in-nexus56),
and [Delwyn dossier](../zone-stories/DUKE_DELWYN.md).

### Checkpoint 786: Desolate item travel stays local

Desolate's six type-25 ITEM_TELEPORT objects target local rooms #22309, #22273,
#22326, #22364, #22365, and #22366. Its eleven type-29 ITEM_SWITCH objects
use value[1] for local room targets. Scans of 866 external type-25 and 369
external type-29 declarations found no destination or switch-room field into
Desolate. No WLD exit or curated source-only route total changes. See the
[Desolate typed audit](DESOLATE_AUDIT_REVIEW.md#item-travel-and-disposition),
[coverage row 72](ZONE_COVERAGE_REGISTER.csv#L73), and
[Desolate dossier](../zone-stories/DESOLATE.md).

### Checkpoint 787: Divine Home portal pair remains outside WLD totals

Divine Home object #40753 is reset in room #40787 and declares Cerebusp room #22053 as
its ITEM_TELEPORT destination. Active Cerebusp object #22010 is reset in #22053 and
declares #40787 as its destination. This source-level typed pair adds no authored WLD
exit, inbound/outbound WLD edge, or curated candidate route count; runtime use remains
unqualified. See the [Divine Home typed
audit](DIVHOME_AUDIT_REVIEW.md#compiled-bindings-and-item-travel), [coverage row
75](ZONE_COVERAGE_REGISTER.csv#L76), and [Cerebusp
audit](PITS_OF_CERBERUS_AUDIT_REVIEW.md#checkpoint-787-reciprocal-divine-home-portal).

### Checkpoint 788: Domain item travel adds no WLD edges

Domain's two local type-29 ITEM_SWITCH targets and two local type-25 ITEM_TELEPORT
destinations resolve to Domain rooms. The scan of 870 external type-25 and 378 external
type-29 declarations found no target into Domain; no WLD boundary or source-only
approach count changes. See the [Domain typed
audit](DLSC_AUDIT_REVIEW.md#checkpoint-788-post-sidecar-typed-closure-refresh) and
[coverage row 76](ZONE_COVERAGE_REGISTER.csv#L77).

### Checkpoint 789: Dream item routes stay outside WLD totals

Dream's type-25 objects target local room #31302 and Arcium room #85805; active Arcium
bed #85725 targets Dream room #31300. The external type-29 scan found no switch
destination into Dream. These typed travel declarations do not change the authored WLD
boundary or source-only approach totals. See the [Dream
audit](DRIFTING_REALM_AUDIT_REVIEW.md#checkpoint-789-shop-and-sidecar-closure-refresh)
and [coverage row 79](ZONE_COVERAGE_REGISTER.csv#L80).

### Checkpoint 790: Stoutdorf item travel adds no WLD edge

No local type-25 teleport or type-29 switch object is declared for Stoutdorf, and the
current external object-source scan found no type-25 destination or type-29 switch room
inside its 199 WLD rooms. The active WLD boundary counts and the source-only `ud_map2.wld`
approach remain unchanged. See the [Stoutdorf typed
audit](DRST_AUDIT_REVIEW.md#checkpoint-790-current-static-typed-closure) and
[coverage row 81](ZONE_COVERAGE_REGISTER.csv#L82).


### Checkpoint 791: Treasure Caves local switch stays within its WLD

Local Dungeon object #93023 is an `ITEM_SWITCH` whose room field is #93002; the reset
loads it in #93002. No local type-25 teleport or external type-25/type-29 room field
points into the 116 Dungeon rooms. The switch does not add an authored WLD edge, so the
two Dungeon boundary pairs and source-only approach totals remain unchanged. See
[audit](TREASURE_CAVES_AUDIT_REVIEW.md#checkpoint-791-current-typed-closure-refresh)
and [coverage row 82](ZONE_COVERAGE_REGISTER.csv#L83).

### Checkpoint 792: Earth item travel stays within the local map

Earth's six local type-25 destinations and eleven type-29 switch room values
all resolve to its 198 local rooms. No external type-25 or type-29 room target
points into Earth. These typed item routes add no authored WLD exits or
source-only approach, and do not change the existing WLD totals. See the
[Earth typed audit](TEMPLE_OF_THE_EARTH_AUDIT_REVIEW.md) and
[coverage row 83](ZONE_COVERAGE_REGISTER.csv#L84).

### Checkpoint 793: Earthp item routes add no WLD edge

Earthp's type-25 crevice object #131223 targets local room #131396; its
type-29 fractured-wall switch #131210 targets local room #131362. All 871
external type-25 and 379 external type-29 declarations resolve outside
Earthp's 199-room WLD. Neither typed item destination changes the authored
WLD exits or curated source-only approach totals. See the
[Grumbar's Domain audit](GRUMBARS_DOMAIN_AUDIT_REVIEW.md) and
[coverage row 84](ZONE_COVERAGE_REGISTER.csv#L85).

### Checkpoint 794: Elemental Groves item travel adds no WLD edge

Element has no local type-25 or type-29 item destination. No destination
among 872 external type-25 and 380 external type-29 declarations targets an
Element room. The typed travel scan does not change its 384 authored exits,
reciprocal Surface pair, or source-only surf.wld approach. See the
[Elemental Groves audit](ELEMENTAL_GROVES_AUDIT_REVIEW.md) and
[coverage row 85](ZONE_COVERAGE_REGISTER.csv#L86).

### Checkpoint 795: Elvish statue travel stays outside WLD totals

Elvish type-25 objects #35801 and #35803 target local room #35800; object
#35804 targets Solonar room #30648. Solonar type-25 object #30604 targets
Elvish room #35810, forming the documented source-level item route in both
directions. One external type-25 target reaches Elvish, and no external
type-29 target reaches it. These typed destinations add no WLD exit or
candidate approach. See the [Elvish typed audit](ELVISH_AUDIT_REVIEW.md) and
[coverage row 87](ZONE_COVERAGE_REGISTER.csv#L88).

### Checkpoint 796: Faang switches stay within the local map

Faang type-29 objects #15281, #15282, #15283, and #15284 target local rooms
#15411, #15393, #15748, and #15202, respectively; the area has no type-25
teleport. The current object-source scan found no external type-25 or type-29
destination into Faang. These item destinations add no authored WLD exit and
do not change Faang's 1,150-exit census or its paired active boundaries. See
the [Faang typed audit](FAANG_STORY_REVIEW.md#checkpoint-796-current-typed-census-refresh)
and [coverage row 93](ZONE_COVERAGE_REGISTER.csv#L94).

### Checkpoint 797: Fields Between item destinations stay local

Fields Between type-25 objects #71001, #71002, #71030, and #71031 target
local rooms #71104, #71014, #71153, and #71001; the area has no type-29
object. The scan of 872 type-25 and 380 type-29 declarations found no
external destination into Fields Between. These item destinations add no WLD
exit and leave the 428-exit census and source-only Surf approach unchanged.
See the [Fields Between typed audit](FIELDS_BETWEEN_AUDIT_REVIEW.md#checkpoint-797-current-typed-census-refresh)
and [coverage row 94](ZONE_COVERAGE_REGISTER.csv#L95).

### Checkpoint 798: Firep item switches stay within the map

Firep type-29 switches #88310 and #88318 target local rooms #88324 and
#88339 through value[1]; the area has no type-25 item. Across 872 type-25 and
380 type-29 declarations in current active and source-only object files, no
external destination targets a Firep room. These item destinations add no WLD
edge and leave the 412-exit census and separate one-way Plane of Fire boundary
unchanged. See the [Charcoal Palace typed audit](FIREP_AUDIT_REVIEW.md#checkpoint-798-current-typed-census-refresh)
and [coverage row 95](ZONE_COVERAGE_REGISTER.csv#L96).

### Checkpoint 799: Firesworn item portals stay within the map

Firesworn type-25 objects #135101 and #135131 target local rooms #135157 and
#135171, respectively; there is no local type-29 item. These portals connect
the two local WLD components without adding authored exits. No external
type-25 or type-29 destination targets a Firesworn room, so the 259-exit WLD
census remains unchanged. See the
[Firesworn typed audit](ALTAR_OF_THE_FIRESWORN_AUDIT_REVIEW.md#checkpoint-799-current-typed-census-refresh)
and [coverage row 96](ZONE_COVERAGE_REGISTER.csv#L97).

### Checkpoint 800: Wharf item travel stays outside WLD totals

Fisherman's Wharf has no local type-25 or type-29 item destination. The scan
of all 872 type-25 and 380 type-29 declarations found no external destination
into a Wharf room. These inventory, shop, and quest references add no authored
WLD exit or source-only approach. See the
[Wharf typed audit](FISHERMANS_WHARF_AUDIT_REVIEW.md#checkpoint-800-current-typed-census-refresh)
and [coverage row 97](ZONE_COVERAGE_REGISTER.csv#L98).

### Checkpoint 801: Flind portal targets add no WLD exits

Flind's six local type-25 objects target three reciprocal in-zone room pairs;
it has no local type-29 object. The full 872 type-25 and 380 type-29 object
scan found no external destination into a Flind room. These destinations do
not change Flind's authored WLD exit totals or its separately recorded
source-only ud_map2 approach. See the
[Flind typed audit](FLIND_AUDIT_REVIEW.md#checkpoint-801-current-typed-dependency-census)
and [coverage row 98](ZONE_COVERAGE_REGISTER.csv#L99).

### Checkpoint 802: Foggy switch stays on the authored graph

Foggy object #90010 is a type-29 switch placed in #90210 and configured for
the existing #90207 south exit; its handler clears the blocked bit on that
exit. The custom #90004 monocle code can relocate the object among local rooms
#90124–#90142 after reset. Neither item creates an authored WLD exit or changes
the 607-source/598-resolving exit census. The scan of all 872 type-25 and 380
type-29 declarations found no external destination into Foggy rooms. See the
[Foggy typed audit](FOGGY_WOODS_AUDIT_REVIEW.md#checkpoint-802-typed-census-and-compiled-bindings)
and [coverage row 99](ZONE_COVERAGE_REGISTER.csv#L100).

### Checkpoint 803: no item travel targets enter Forgotten Forest

The Forest declares no local type-25 or type-29 destination, and the scan of
all 872 type-25 and 380 type-29 objects found no external target into one of
its 92 rooms. This leaves the reciprocal Surface boundary, 283 authored WLD
exits, and 15 already-counted source-only Surf approaches unchanged. See the
[Forest typed audit](FORGOTTEN_FOREST_AUDIT_REVIEW.md#checkpoint-803-current-typed-census-refresh)
and [coverage row 100](ZONE_COVERAGE_REGISTER.csv#L101).

### Checkpoint 804: Fortb item teleports stay outside WLD totals

Fortb object #38143 stores a type-25 item-teleport target of Duris3 #149188
with CMD_PUSH (270). That room appears in the unregistered Duris3 WLD, so this
source declaration is not evidence of a currently loaded destination. Active
Prisonb plank #43023 stores Fortb #38106 with CMD_JUMP (264) and is reset in
Prisonb room #43022. Both are typed item links, not authored WLD directions.
They leave Fortb's 163/163 WLD records, its 77-room component, and its three
paired active boundaries unchanged. They also do not alter curated source-only
WLD approach totals. See the [Fort Boyard typed review](FORT_BOYARD_STORY_REVIEW.md#checkpoint-804-current-typed-dependency-census),
[brush values](../../../areas/obj/fortb.obj#L476),
[Prisonb plank](../../../areas/obj/prisonb.obj#L249),
[Duris3 room](../../../areas/wld/Duris3.wld#L1032949), and
[Prisonb reset placement](../../../areas/zon/prisonb.zon#L190).

### Checkpoint 805: Ghore typed links stay separate from WLD exits

CP805 finds no local type-25/type-29 item or external type-25/type-29 target
into Ghore rooms. Four internal door-key fields use Ghore objects #11574 and
#11571 on two reciprocal WLD pairs; no outside WLD key field points to a
Ghore object. Compiled arrival candidates such as room #11545 and conditional
special routes remain separate from authored WLD edges. The authored census
stays 987/987 resolved with three paired active boundaries; the source-only
ud_map0 approach and active Shafts boundary remain unchanged. No WLD or
curated source-only approach total changes. See the
[Ghore typed census](GHORE_STORY_REVIEW.md#checkpoint-805-current-typed-dependency-census),
[door-key records](../../../areas/wld/ghore.wld#L2592),
[Plane Shift target](../../../src/classes/innates.c#L2008), and
[coverage row](ZONE_COVERAGE_REGISTER.csv#L103).

### Checkpoint 806: Gibber switch and key roles stay within the graph

Gibber's type-29 switch objects #8708 and #8716 reference local exit rooms
#8713 and #8784. The two reciprocal WLD key pairs likewise use local objects
#8739 and #8715. No local type-25 item or outside type-25/type-29 target into
a Gibber room was found. These switches and keys do not add WLD directions:
the authored census stays 406/406 resolved, with two reciprocal Underdark
pairs and the separate one-way Realm inbound edge. Preserve the source-only
ud_map2 approach separately. See the
[Gibber typed audit](GIBBERLING_KING_AUDIT_REVIEW.md#checkpoint-806-current-typed-dependency-census),
[switch objects](../../../areas/obj/gibber.obj#L144),
[keyed exits](../../../areas/wld/gibber.wld#L35), and
[coverage row](ZONE_COVERAGE_REGISTER.csv#L104).

### Checkpoint 807: Gith_ht typed roles do not add WLD routes

Gith_ht has no local type-25/type-29 item target, incoming type-25/type-29
room target, or outside WLD key field targeting a local object. Reset object
IDs #19442 and #19443 remain unresolved as objects even though each also
names a local room and mobile prototype. The compiled Chaos destination
#19428 is a code-defined room route, not a WLD exit or item portal. The static
census remains 185/185 loader-resolved exits and one reciprocal Astral
boundary; no WLD totals or boundary pairs changed. See the
[Gith_ht typed audit](GITH_HT_STORY_REVIEW.md#checkpoint-807-current-typed-dependency-census),
[Chaos destination](../../../src/combat/chaos.c#L39), and
[coverage row](ZONE_COVERAGE_REGISTER.csv#L105).

### Checkpoint 808: Fortress item roles stay separate from WLD ingress

Githyanki Fortress has no local type-25/type-29 item or outside item target
into its rooms. Its ten WLD key fields use five local key objects; no external
WLD key field targets a Fortress object. The 276 exits authored in unregistered
`Duris3.wld` remain source-only and do not alter the active graph. The active
Fortress remains at 159/159 loader-resolved exits with one reciprocal Astral
boundary. See the
[Fortress typed audit](GITHYANKI_FORTRESS_STORY_REVIEW.md#checkpoint-808-current-typed-dependency-census),
[Duris3 source-only component row](ZONE_COVERAGE_REGISTER.csv#L387), and
[Fortress coverage row](ZONE_COVERAGE_REGISTER.csv#L106).

### Checkpoint 809: Githzerai item travel stays outside the WLD boundary count

Githzerai's 22 local type-25 routes and ten type-29 switches do not add WLD
directions. Active Ravenloft2 object #59079 separately targets room #44487;
that item route is not a WLD edge. Forty local door-key fields use local
objects, and no external WLD key field points to a Githzerai object. The
active graph remains 919/919 resolved exits with four inbound and one
reciprocal outbound boundary; the three unregistered source-only approaches
remain separate. See the
[Githzerai typed audit](GITHZERAI_STRONGHOLD_AUDIT_REVIEW.md#checkpoint-809-current-typed-role-refresh),
[portal object](../../../areas/obj/ravenloft2.obj#L1203), and
[coverage row](ZONE_COVERAGE_REGISTER.csv#L107).

### Checkpoint 810: Glacier key fields do not change its one-way boundary

Glacier has two local WLD key fields between rooms #90504 and #90505, both
using local object #90402. No outside WLD key field targets a Glacier object;
there is no local type-25/type-29 item or outside type-25/type-29 room target.
The authored WLD census remains 256/257 resolved, with no active inbound edge
and the one-way exit from #90402 to Surface #523536. The `surf.wld` exit to
#90401 remains source-only, not active ingress. See the
[Glacier typed audit](GLACIER_AUDIT_REVIEW.md#checkpoint-810-current-typed-dependency-closure),
[paired exits](../../../areas/wld/glacier.wld#L2144),
[source-only approach](../../../areas/wld/surf.wld#L185821), and
[coverage row](ZONE_COVERAGE_REGISTER.csv#L108).

### Checkpoint 811: Goblin Camp item roles do not add WLD directions

Goblin Camp has no local type-25/type-29 item, incoming type-25/type-29 room
target, or nonzero WLD key field. Its 65/65 resolved active WLD exits remain
unchanged: Surface has the reciprocal pair, and War Hammers has a separate
one-way arrival. The `surf.wld` approach remains source-only, not active
ingress. See the
[Goblin Camp typed audit](GOBLINCAMP_AUDIT_REVIEW.md#checkpoint-811-current-typed-dependency-closure),
[boundary records](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L158),
[source-only approach](../../../areas/wld/surf.wld#L1354448), and
[coverage row](ZONE_COVERAGE_REGISTER.csv#L109).

### Checkpoint 812: Goblin Cave item fields remain separate from WLD edges

Goblin Cave has no local type-25/type-29 object or external type-25/type-29
target into its rooms; its local WLD key fields are zero. The Ixarkon veil's
code-defined destination #19022 is a separate dynamic route, not an authored
WLD edge. The active graph remains 45/45 resolved with the reciprocal
Underdark pair and no inbound item or WLD route; the inactive-WLD scan still
has no approach. See the
[Goblin Cave typed audit](GAGGA_JOBO_CAVE_SYSTEM_AUDIT_REVIEW.md#checkpoint-812-current-typed-dependency-closure),
[Ixarkon destination list](../../../src/specs/specs.ixarkon.c#L19), and
[coverage row](ZONE_COVERAGE_REGISTER.csv#L110).

### Checkpoint 813: Moregeeth item routes and key fields stay within the graph

Moregeeth's eight type-25 routes target local rooms; there are no local
type-29 routes or external type-25/type-29 room targets. Its 12 local WLD key
fields use local objects #70057 and #70019, with no outside WLD key field
pointing into the town. The active graph remains 1,350/1,350 resolved exits,
with three inbound and two outbound boundary records. The `surf.wld` approach
remains source-only. Moregeeth's code-defined room/mobile bindings and the
separate source-only `myranth1.shp` consumers are typed dependencies, not WLD
directions. See the
[Moregeeth typed audit](MOREGEETH_AUDIT_REVIEW.md#checkpoint-813-current-typed-role-closure),
[Myranth1 audit](MYRANTH1_AUDIT_REVIEW.md#typed-item-references-and-missing-prototypes),
and [coverage row](ZONE_COVERAGE_REGISTER.csv#L111).

### Checkpoint 814: Golden Hall item targets stay separate from WLD exits

Golden Hall's 42 local WLD key fields use local objects, and no outside WLD
key field targets a Golden Hall object. Its two type-25 portals target local
rooms #40699/#40698; four type-29 switches name local rooms. No external
type-25 destination or type-29 room field enters the zone. The 76 inbound
active ZON prototype reset operands, Ship Yards QST/sidecar references, and
the Alatorin shop product are typed item/mobile roles, not additional WLD
directions. The authored graph remains 674/674 resolved exits with three
inbound, two outbound, and two reciprocal boundary records. See the
[Golden Hall typed audit](GOLDEN_HALL_OF_THE_CROWN_AUDIT_REVIEW.md#checkpoint-814-current-typed-dependency-closure),
[Caertannad outbound reset audit](CAERTANNAD_AUDIT_REVIEW.md#outbound-active-reset-references),
and [coverage row](ZONE_COVERAGE_REGISTER.csv#L112).

### Checkpoint 815: Halfcut portals stay outside the shared candidate graph

Active Halfcut remains at 1,388/1,388 resolved WLD exits with one inbound
Surface edge and its reciprocal outbound. Its two WLD key fields use local
object #27033. Four active type-25 portals and two type-29 switches target
active Halfcut rooms; they add no WLD directions, and no external type-25 or
type-29 target enters the active source. The unregistered `halfcut_hills`
copy still shares numeric rooms with active Halfcut: active Surface exit
#629274 returns to active #27001, not the candidate. The candidate's own
#27001 edge to #256031 remains an unresolved, source-only approach. Keep those
graphs and owners separate. See the
[Halfcut typed audit](HALFCUT_HILLS_AUDIT_REVIEW.md#checkpoint-815-active-halfcut-typed-closure-and-candidate-boundary),
[candidate graph review](HALFCUT_HILLS_AUDIT_REVIEW.md#candidate-wld-graph-and-target-identities),
and [coverage row](ZONE_COVERAGE_REGISTER.csv#L116).


### Checkpoint 816: Hall item dependencies remain separate from the WLD graph

Hall remains at 635 source exits / 633 loader-resolved exits, with two inbound,
one outbound, and one reciprocal boundary record; two targets remain negative
and none is positively unresolved. The six WLD key fields use local objects,
and local type-25/type-29 fields stay within Hall. The imported Heavens/Unique
reset objects, Temple dagger used by Hall's QST and sidecar, shop inventory, and
compiled Hall bindings are typed dependencies, not extra WLD directions.

The source-only Casino has two `D` room operands and ten `O` placement-room
operands that share numbers with Hall rooms but describe different room
identities. They do not add active edges or establish Casino access. Hall code
also changes exit flags on local rooms #77888, #77893, and #77944, so static
resolution does not settle phase access or safety. See the [Hall typed
audit](HALL_OF_THE_ANCIENTS_AUDIT_REVIEW.md#checkpoint-816-current-typed-dependency-closure),
[Casino identity audit](CASINO_AUDIT_REVIEW.md#checkpoint-816-hall-typed-closure-confirms-casino-room-operand-identity),
and [coverage row 116](ZONE_COVERAGE_REGISTER.csv#L117).

### Checkpoint 817: Phlegethos reachability remains analysis-seed-bounded

Phlegethos's 400 rooms contain 1,600 internal exits in one weak component and
five directed components. From analysis seed #52200, the 396-room strongly
connected component is reachable and returnable; #52311, #52312, #52313, and
#52599 are outside that seed's directed reach. Eighty-six exit records lack a
reverse record. A scan of all 446 WLD files found no room-number overlap,
inbound target, or outbound target, so no static WLD entrance or egress is
identified.

Bronze Citadel object #32456 is an unreset portal template with value #99999.
The same number on the BCTDL ZON line is a mobile reset's room operand, not an
object load. It does not establish an arrival. Keep dynamic travel, live player
entry, return, and safety unqualified. The active WLD record count remains
1,600/1,600 resolved with no cross-zone boundary. See the [Phlegethos typed and
reachability audit](HELL_FOUR_PHLEGETHOS_AUDIT_REVIEW.md#checkpoint-817-static-reachability-reconciled),
[CP224 graph record](WORLD_CONNECTIVITY.md#checkpoint-224-hell-phlegethos-directed-grid-and-ingress),
and [coverage row 121](ZONE_COVERAGE_REGISTER.csv#L122).

### Checkpoint 818: Northern Lakes item roles do not add WLD exits

Northern Lakes remains at 551/551 resolved WLD exits, five inbound and five
outbound boundary records, all five reciprocal. The boundaries pair Teka and
Winterhaven once each and Surface three times. The Alatorin reset of mobile
#75229, Heavens objects used by local resets and the two shops, and Artek's
Winterhaven visage #55287 are typed prototype/story roles; they add no WLD
edges. No local or external WLD key, local fixed teleport, or external type-25
destination enters the 219-room range. Fall and current fields remain three
and seventeen, respectively. Live access, currents, boat behavior, door state,
and safety still need runtime qualification. See the [Nlakes typed
audit](NLAKES_AUDIT_REVIEW.md#checkpoint-818-typed-dependencies-and-source-ownership),
[existing exit pairs](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L325), and [coverage row
213](ZONE_COVERAGE_REGISTER.csv#L214).

### Checkpoint 819: Opal typed roles do not add WLD edges

Opal Phoenix remains at 199/199 resolved WLD exits, with one inbound, one
outbound, and one reciprocal Surface boundary. Its shop, local reset/QST/story
items, teacher mobile #70806, and Chaos-kit objects #70813/#70814 are typed
roles, not additional WLD routes. No WLD key or type-25 destination enters the
76-room range. The source-only `aopal` candidate does not provide Opal ingress;
its retained approaches are to other named maps. See the [Opal typed
audit](OPAL_PHOENIX_AUDIT_REVIEW.md#checkpoint-819-current-typed-dependency-closure),
[Surface pair](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L334), and [coverage row
222](ZONE_COVERAGE_REGISTER.csv#L223).

### Checkpoint 820: Orrak typed roles do not add WLD edges

Orrak remains at 77/77 resolving WLD exits, in one weak component, with root #9000 reaching and returning from all 39 rooms and a reciprocal active Surface boundary. The three local WLD key sides use Orrak objects #9002 and #9010; no outside WLD key field uses an Orrak object. Reset imports #358/#55446 and external resets of mask #9017 are prototype dependencies, not exits. No type-25 fixed or random destination and no type-29 target-room field names an Orrak room. Keep the static graph from CP169; actual key use, other dynamic routes, player access, return, and safety remain unqualified. See the [Orrak typed audit](ORRAK_AUDIT_REVIEW.md#checkpoint-820-current-typed-dependency-closure), active exit-register [rows 183 and 1598](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L183), and pair-summary [rows 336 and 570](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L336).

### Checkpoint 821: Randomness typed references do not add active WLD edges

Randomness remains at 39,600/39,600 locally resolved exits in one weak component containing all 10,000 rooms, with zero active cross-area inbound, outbound, or reciprocal directions. The all-project VNUM-target scan covered 448 `.wld` files (446 under `areas/wld` and two elsewhere); it found no active external direction into Randomness #100000–#109999. The two nonstandard `northern_wilderness.wld` records have targets #12270 and #6803, outside the range. The only non-overlapping source-only numeric matches are 602 directions in the separate, unregistered Duris3 map; its same-number rooms have different titles. Do not count those source records as active routes or merge the Duris3 and Randomness identities. Typed ZON stock and mobile-reset references add no WLD directions. See the [Randomness typed audit](RANDOMNESS_AUDIT_REVIEW.md#checkpoint-821-current-typed-dependency-closure), [Randomness coverage row 246](ZONE_COVERAGE_REGISTER.csv#L247), and [Duris3 candidate row 35](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

### Checkpoint 822: Raxthan static boundaries and inactive-source count

Raxthan remains a 100-room, 227/227-resolved-exit component. Its three active outbound boundaries still have reciprocal Surface/Underdark records: #42980 east to #595852, #42980 south to #596251, and #42999 south to #749737. A current count finds 446 `areas/wld` files (351 active, 95 inactive); all 95 inactive files parsed without errors and still yield no Raxthan room overlap plus one source-only Surf direction, #266966 north to #42980. The previous 97-file wording was stale; its room and exit totals and curated source-only counts remain unchanged. Keep the Surf direction out of active ingress and preserve the existing candidate plan. Static graph reach does not qualify door use, fall safety, access, or return. See the [Raxthan source audit](YERDONIA_AUDIT_REVIEW.md#checkpoint-822-current-typed-and-wld-source-reconciliation), [Surf source exit](../../../areas/wld/surf.wld#L1405206), [cross-area rows 579–581](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L579), and [coverage row 249](ZONE_COVERAGE_REGISTER.csv#L250).

### Checkpoint 823: Orcish Slave Camp source totals reconcile

The Orcish Slave Camp remains a 14-room, 27/27-resolved-exit component with one reciprocal active Split Shield boundary: camp #53200 west to Split Shield #10327, and #10327 east back to #53200. The current 95-file unregistered WLD sweep reproduces 527,791 rooms and 2,080,773 exits with no parse issue, camp-room overlap, or target into #53200–#53213. A scan of all 448 repository `.wld` files found no other external camp-range reference; the two WLD files outside `areas/wld` contain none. The complete set of active and unregistered area-tree WLD sources therefore adds no camp ingress beyond the active reciprocal pair.

Keep actual trapdoor/key operation, player access, return, and safety open for runtime qualification. The inactive Surf exits to Split Shield #10300 and #10334 stay on the separate Surf candidate plan; they do not target camp rooms. See the [camp source audit](ORCISH_SLAVE_CAMP_AUDIT_REVIEW.md#checkpoint-823-current-wld-and-typed-source-reconciliation), [Split Shield direction row 198](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L198) and [camp return row 659](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L659), and [coverage row 274](ZONE_COVERAGE_REGISTER.csv#L275).

### Checkpoint 824: Stronghold dependencies leave its WLD graph unchanged

Storm Port Stronghold remains at 130/130 resolving exits across nine local weak components (largest 50 rooms), with three reciprocal Surface pairs. Its load room #22649 has six authored outbound records and no active inbound record. The 95-file unregistered-WLD parse again finds one non-overlapping source-only Surf exit, #285640 west to #22634, with no room collision. A repository-wide exact range scan finds only three active Surface returns to Stronghold—#636281 south and #636680 east to #22600, and #636682 west to #22634 ([rows 1666, 1668, and 1669](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1666))—plus that one Surf source-only exit. No other `.wld` file contains a Stronghold-range match. A current WLD exit-tuple scan found no foreign key field using a Stronghold object; the only matches are two local uses of #22602. Preserve the three Surface pairs and the one-way load-room edges as authored; the Surf direction is not active ingress. Actual access, service state, return, and safety remain unqualified. See the [typed-source refresh](STORM_PORT_STRONGHOLD_AUDIT_REVIEW.md#checkpoint-824-current-typed-source-reconciliation), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [coverage row 281](ZONE_COVERAGE_REGISTER.csv#L282).

### Checkpoint 825: Strathor's current WLD closure reconciles

Strathor remains at 136 rooms and 297/297 resolving exits in one weak component. Its two active reciprocal Surface pairs remain #30800 south ↔ #600071 north and #30929 north ↔ #599271 south. The all-95 unregistered-WLD scan again finds no source-room overlap and one source-only Surf exit, #216305 north to #30800. An exact room-range scan across all 448 repository `.wld` files finds only the two active Surface returns and that Surf record; no other external WLD reference targets Strathor. Preserve the Surf direction on its existing candidate row 97, not as active ingress. Doors, ferry operation, live access, return, and safety remain unqualified. See the [current source audit](STRATHOR_VALLEY_AUDIT_REVIEW.md#checkpoint-825-current-wld-and-typed-source-reconciliation), [first outbound row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L432), [second outbound row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L433), [first return row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1576), and [second return row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1575), [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [coverage row 282](ZONE_COVERAGE_REGISTER.csv#L283).

### Checkpoint 826: Storm Port's current WLD closure reconciles

Storm Port remains at 147 rooms, 320 authored exits, and 319 loader-resolved targets. Its local weak components remain 139/7/1; directed reach from #22400 is 137 rooms, and 139 rooms can return to #22400. Six authored outward records comprise four reciprocal Surface exits, one one-way outbound to Sarmiz #9597, and the unresolved east exit from #22541 to Duris3 #144664. Six active inbound records remain the four reciprocal Surface returns plus Stronghold #22649 down to #22423 and Winterhaven #55402 north to #22424. See the [Storm Port source audit](STORMPORT_AUDIT_REVIEW.md#checkpoint-826-current-source-reconciliation), [outbound register rows 365–369](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L365), [active inbound rows 377, 750, and 1539–1545](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L377), [unresolved row 54](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L54), and [pair summary rows 452, 455, 456, 599, and 815](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L452).

The current all-95 inactive-WLD scan again finds no room overlap and four source-only exits from `surf.wld`: #274843 south to #22479, #275142 east and #275443 north to #22401, and #275144 west to #22432. The all-448 repository-WLD target scan finds no other incoming reference. Preserve all four as source-only on existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); do not count them as active ingress or replacement evidence. Keep active Storm Port **DO NOT MOVE** and retain the separate unresolved Duris3 identity on [candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36). Static connectivity does not qualify actual access, service availability, safe arrival, return, or gameplay.

### Checkpoint 827: Sun Temple graph remains reciprocal; source-only edges stay separate

The active Temple of the Sun graph remains 94 rooms and 217/217 resolved exits: 216 local and one outbound. It remains one weak component, with directed reach to and from #82493 covering all 94 rooms when all authored local exits are treated as available. The single active cross-area pair is Temple #82493 west to Surface #541275 and the reciprocal Surface return east; see [outbound row 988](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L988), [inbound row 1415](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1415), and [pair rows 457 and 600](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L457).

The current inactive-WLD scan finds no source-room overlap and only Surf #237595 west to Temple #82493. A repository-wide exact range scan confirms that the only external WLD targets are this source-only Surf edge and the active Surface return. Preserve Surf on existing [candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); it is not loaded ingress. WLD key fields remain local to objects #82420, #82423, and #82424, and there is no external type-25 or type-29 room target. The `brass-old-1` reset of object #82416 is a source-only typed dependency, not a WLD edge; preserve it on [candidate row 17](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17). SEARCH, keys, locked doors, PICK, actor success, and return safety remain separate from this static graph. See the [current Temple audit](TEMPLE_OF_THE_SUN_AUDIT_REVIEW.md#checkpoint-827-current-source-reconciliation) and [coverage row 284](ZONE_COVERAGE_REGISTER.csv#L285).


### Checkpoint 828: Surface graph and inactive map identities reconcile

The current source remains at 160,004 rooms and 639,866 authored directions, of which 639,864 resolve. The maintained boundary registers still show 423 inbound directions, 370 outbound directions, and 359 reciprocal outbound records. Two positive unresolved targets remain: Surface #522255 east and #522656 north both target unloaded #33807 ([first](../../../areas/wld/surface.wld#L467220), [second](../../../areas/wld/surface.wld#L475642)). The typed completion-signature refresh adds no WLD edge.

The source-unchanged check preserves Checkpoint 373’s full inactive-WLD result: 1,208 directions from non-overlapping source-room VNUMs enter active Surface, while 478,024 further numeric target matches begin at VNUMs already owned by active rooms. Retain only the 1,208 non-overlapping source records as candidate approaches, on existing rows 12, 29, 55, 78, and 99. Keep `newbiemaps.wld` and `surface2011-temp.wld` separate despite their shared 602-direction tuple family and overlapping room numbers; their source titles differ, and neither inactive source is active ingress. No new approach, replacement identity, or move evidence was found. See the [Surface audit](SURFACE_AUDIT_REVIEW.md#checkpoint-828-current-source-and-completion-signature-reconciliation) and [Surface coverage row 285](ZONE_COVERAGE_REGISTER.csv#L286).

### Checkpoint 829: SurfaceKeeps static graph and source-only routes reconcile

The current active graph remains 2,645 rooms with 9,452 authored and 9,446 loader-resolved directions: 9,372 internal and 74 outbound. Its 79 active inbound records land in 76 distinct rooms; 69 of the 74 outbound records have a reverse-direction record. Nine weak components remain sized 1,603, 673, 143, 87, 64, 33, 30, 11, and 1. With authored exits treated as available, traversal from active inbound anchors reaches 2,360 rooms; reverse traversal from all outbound sources includes 2,644, leaving #120000 without a static path to an outbound edge. Six positive unresolved exits all target VNUMs found in inactive Duris3 source. The #120057 type-25 travel object is a separate transport and adds no WLD edge.

The inactive-source count remains 1,457 non-overlapping directions into SurfaceKeeps: 1,451 from Duris3, five from oldmap8, and one from ygg. Another 9,131 numeric target matches begin at source-room VNUMs already owned by active WLD contributors and remain excluded from the candidate approach count. Preserve the complete 2,645-room Duris3 VNUM overlap as separate inactive map identity evidence; all paired room titles differ. No WLD source changed from the pinned snapshot and no route was activated. See the [SurfaceKeeps audit](SURFACEKEEPS_AUDIT_REVIEW.md#checkpoint-829-current-source-dependency-and-graph-reconciliation), [unresolved rows 321–326](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L321), and [coverage row 286](ZONE_COVERAGE_REGISTER.csv#L287).

### Checkpoint 830: SurfMini WLD boundaries and item routes remain separate

The active `surfacemini` graph remains 119 rooms and 261/261 resolved directions: 250 local and 11 outbound. Eight active WLD directions enter, seven outbound records have reverse-direction records, and ten local weak components remain. The seven distinct inbound anchors reach 112 rooms under raw authored-edge traversal; reverse traversal from outward sources reaches 118, with room #98018 isolated from ordinary WLD travel. There are no unresolved exits, unregistered-WLD targets, source-only WLD approaches, or room-VNUM overlaps.

Four active Surface type-25 objects point to SurfMini rooms #97906, #97952, #98016, and #98017; the matching `surface2011.obj` declarations remain inactive source evidence. These item routes are separate from WLD edges. No new path or candidate is inferred. See the [SurfMini audit](SURFACEMINI_AUDIT_REVIEW.md#checkpoint-830-current-source-item-roles-and-completion-signatures), [coverage row 287](ZONE_COVERAGE_REGISTER.csv#L288), and the [existing boundary census](SURFACEMINI_AUDIT_REVIEW.md#active-graph-and-boundaries).

### Checkpoint 831: Swamp One typed routes do not change its WLD graph

The current `areas/` and `src/` sources still match the pinned snapshot, so the recorded Swamp One WLD census remains 36 rooms with 84/84 authored directions resolving: 82 local and two outbound, with two inbound records and one weak component of 36 rooms. The inactive `surf.wld` #267604 west approach to #26001 remains source-only. Checkpoint 831 reconciles reset, key, and quest-item ownership; it does not add a WLD edge or establish runtime key availability. Keep the existing graph and source-only classifications in [Checkpoint 376](#checkpoint-376-swamp-connectivity-and-inactive-approach). See the [typed-source refresh](SWAMP_ONE_AUDIT_REVIEW.md#checkpoint-831-current-typed-source-roles) and [coverage row 288](ZONE_COVERAGE_REGISTER.csv#L289).

### Checkpoint 832: Swamp Two item routes remain separate from WLD reachability

Swamp Two's recorded graph remains 35 rooms with 85/85 authored directions resolving, 84 local and one outbound, a single inbound reciprocal boundary with Swamp One, and one weak component. The three keyed WLD pairs still use #26005, #26007, and #26011; the CP377 #26134 cavern prose/route discrepancy remains unresolved. White Harbor's #55185 request, Caves' #26013 exchange, and the shared Heavens #359 reset are typed item/story relationships, not additional WLD exits. They do not prove item acquisition, door traversal, or a route through the cavern. Keep the existing graph and no-candidate classification. See the [Swamp Two typed-source refresh](SWAMP_TWO_AUDIT_REVIEW.md#checkpoint-832-current-item-routes-and-prototype-roles) and [coverage row 289](ZONE_COVERAGE_REGISTER.csv#L290).

### Checkpoint 833: Tchan remains connected only by its reviewed Surface boundary

The active Tchan graph remains ten rooms with 19/19 authored exits resolving: 18 local directions and one outward to Surface #513140, whose east exit returns to #4800. The four distinct Alterian approaches remain inactive source-only evidence on candidate row 8; sixteen other same-number target matches begin at source rooms that already overlap active Tchan VNUMs and do not add approaches. The closed monastery door, guard, and Storm Port mobile reset do not add WLD edges. Keep the CP378 graph and candidate classification unchanged. See the [Tchan typed-source refresh](TCHAN_AUDIT_REVIEW.md#checkpoint-833-current-typed-source-roles) and [coverage row 290](ZONE_COVERAGE_REGISTER.csv#L291).

### Checkpoint 834: Teka2 conditional item routes stay outside the raw WLD graph

Teka2's recorded graph remains 98 rooms with 198/198 authored exits resolving: 197 local and one to North Lakes, with weak components of 85 and 13 rooms. The reciprocal #75546/#75264 boundary remains the only active cross-area WLD pair; the inactive-WLD sweep found no Teka2 approach. The dossier's four typed item-teleport routes remain conditional routes separate from WLD edges. Checkpoint 834's Alatorin item stock, DLSC experience value, and North Lakes orientation text do not add a map path. Keep the existing graph and no-candidate classification. See the [Teka2 typed-source refresh](TEKA2_AUDIT_REVIEW.md#checkpoint-834-typed-source-closure-rechecked) and [coverage row 291](ZONE_COVERAGE_REGISTER.csv#L292).

### Checkpoint 835: Temple source-only Surf exit and WLD graph stay distinct

Temple remains 323 rooms with 803/803 authored exits resolving in one weak component, with one reciprocal active boundary to Surface. Inactive `surf.wld` room #263869 still has one south exit into Temple #18300; it is source-only and does not change active ingress. The Phantom `shift_prime` entry for room #18305 is code-defined arrival, not a WLD edge. Hall's dagger and letter exchanges are typed item routes, not map connections. Keep the CP57 graph and existing Surf classification unchanged. See the [Temple source refresh](TEMPLE_AUDIT_REVIEW.md#checkpoint-835-hall-item-chain-and-conditional-arrival-remain-separate) and [coverage row 292](ZONE_COVERAGE_REGISTER.csv#L293).

### Checkpoint 836: Temple active connectivity stays separate from Sylvandawn sources

Temple remains 103 rooms with 215/215 authored directions resolving: 214 local and one reciprocal Underdark boundary, in one weak component. The 17 Sylvandawn approach directions remain source-only on candidate row 101; its 206 same-number source-room matches have different titles and add no ingress. The 115 Sylvandawn reset room arguments and eight SHP products for Temple objects remain unregistered source data. No active WLD route or boundary changed. See the [Temple source refresh](TEMPLE_LADUGUER_AUDIT_REVIEW.md#checkpoint-836-sylvandawn-dependencies-remain-source-only) and [Sylvandawn candidate row 101](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L101).

### Checkpoint 837: Tentro item travel and external resets remain separate from WLD edges

Tentro2 remains 294 rooms with 682/682 authored directions resolving, 678 local and four outward, five inbound records, and weak components of 227 and 67 rooms. Its pool type-25 object #69001 still targets #69041 on entry; this item route remains separate from its WLD graph. Six Tentro resets import ConnectorZones mobiles and one imports Verz1 object #3097, with no change to room connectivity. The four inactive Surf approaches remain source-only on candidate row 97. Keep the CP382 graph and candidate classification unchanged. See the [Tentro2 typed-source refresh](TENTRO2_AUDIT_REVIEW.md#checkpoint-837-current-reset-bank-and-pool-routes-rechecked) and [coverage row 294](ZONE_COVERAGE_REGISTER.csv#L295).

### Checkpoint 838: Tezcat typed routes remain outside the WLD graph

Tezcat remains 100 rooms with 239 authored directions (238 internal, one
outbound), two active inbound records, and one weak component. The raw authored
traversal still reaches all 100 rooms and returns from all 100 to the outbound
source. Kastle's two talons, Tharnadia's dagger delivery, four imported object
resets, and three maintained code-data references add no WLD directions.
Surface #575445 ↔ Tezcat #98900 remains the paired public boundary; Winterhaven
#55632 → #98974 remains a one-way arrival without a local WLD return. The Surf
#265973 and Underworld2 #429194 approaches remain source-only on candidate rows
97 and 116. Keep the CP383 graph and existing candidate classifications
unchanged. See the [Tezcat typed-source refresh](TEZCAT_AUDIT_REVIEW.md#checkpoint-838-current-typed-closure-refresh)
and [coverage row 295](ZONE_COVERAGE_REGISTER.csv#L296).

### Checkpoint 839: Tharnadia item dependencies leave its WLD graph unchanged

Tharnadia remains at 1,268 authored directions: 1,267 loader-resolved (1,262
internal and five outward) and one dropped negative target. Six active inbound
records, five outbound records, three reciprocal pairs, and local components of
551, 20, 10, 1, 1, and 1 rooms remain as documented. The incoming Bloodstone OBJ
#74203 reset and shop product, earlier Lylr/Arena object resets, city item
portals, and external quest items do not add WLD edges. Keep the six active
arrivals, five departures, and Duris3 source-only evidence separate. The 604
non-overlapping Duris3 approaches remain on candidate row 36; its 1,732
same-number directions from overlapping source rooms remain excluded. See the
[Tharnadia current-source refresh](THARNADIA_AUDIT_REVIEW.md#checkpoint-839-bloodstone-input-rechecked-against-current-sources)
and [coverage row 296](ZONE_COVERAGE_REGISTER.csv#L297).

### Checkpoint 840: Tharnadian Ruin typed routes remain outside its WLD graph

The Tharnadian Ruin remains 179 rooms with 371/371 authored exits resolving in
weak components of 168, 10, and 1 rooms. Its three reciprocal active
boundaries still connect Caertannad, Cityruin, and Braddistock Mansion, and the
all-95 inactive-WLD sweep found no approach or room overlap. Braddistock's
#1375 QST proof, Caertannad's giver roles for local #5506/#5528, the Dungeon
and Plane of Earth mobile resets, Heavens' resets of local OBJ #5504, and the
Surface item routes add no WLD direction. Keep the CP385 graph unchanged and
treat the item routes separately from its three arrivals and departures. See
the [typed-source refresh](THARNADIAN_RUIN_AUDIT_REVIEW.md#checkpoint-840-current-typed-source-closure-refreshed)
and [coverage row 297](ZONE_COVERAGE_REGISTER.csv#L298).

### Checkpoint 841: Tharnadia Rifts WLD and reset edges stay distinct

Tharnadia Rifts remains 5,783 rooms with 22,252 authored directions: 22,250
resolve in the active loader, while two target inactive-only Duris3 rooms.
There are 74 active arrivals (73 Caertannad, one Heavens) and 25 active
departures (23 reciprocal Caertannad links and two one-way SurfaceKeeps links).
Raw reach, seven WLD components, and the two pruned Duris3 targets remain at
Checkpoint 386 counts. The 62 reset commands import Caertannad/Minopass
prototypes; the silverleaf and hidden-crevice item references remain separate
from WLD topology. Preserve the 1,325 source-only inactive approaches, with
1,323 on existing Duris3 candidate row 36 and two additional distinct
`bb_gh`/`circlegh` sources. Exclude the 21,809 overlapping-source Duris3
directions. Curated source-only totals remain 11,993 overall and 415 Surf. See
the [current typed-source refresh](THARNRIFTS_AUDIT_REVIEW.md#checkpoint-841-current-typed-ownership-and-boundary-refreshed)
and [coverage row 298](ZONE_COVERAGE_REGISTER.csv#L299).

### Checkpoint 842: Thetis item exchanges remain outside its WLD graph

Thetis remains 100 rooms with 218 authored exits: 215 local, two outbound, and
one unresolved #38095 north to #104; no active inbound boundary exists. Its
single weak component still gives raw WLD reach and return of 100/100 from
beach room #38000. The three independent requests, Jade Mini Zones reset roles,
optional Stronghold references, and local Salty shop add no WLD edge. Preserve
the two outbound Surface/Heavens edges and the unresolved #104 target. Inactive
Surf #251543 north to #38000 remains source-only on candidate row 97; the
current cumulative source-only totals remain 11,993 overall and 415 Surf. See
the [typed-source refresh](THETIS_AUDIT_REVIEW.md#checkpoint-842-current-typed-closure-and-source-only-ingress-refreshed)
and [coverage row 299](ZONE_COVERAGE_REGISTER.csv#L300).

### Checkpoint 843: Thri item routes and code-bound rooms stay distinct

Thri remains 155 rooms with 339/339 authored exits resolving in one weak
component. Its single active cross-area boundary is the reciprocal Surface
pair; raw directed traversal from #67600 reaches 145 rooms, while all 155 can
reach that source room. Four local type-25 targets, two reset placements, the
inn assignment, hometown/guild room data, and Heavens shop/reset stock do not
add WLD directions or an active arrival. Inactive Surf #250600 west to #67600
remains source-only on candidate row 97. The cumulative source-only totals are
11,993 overall and 415 Surf. Keep the CP388 graph and classifications unchanged.
See the [Thri source refresh](THRI_STORY_REVIEW.md#checkpoint-843-current-typed-stock-and-room-references-refreshed)
and [coverage row 300](ZONE_COVERAGE_REGISTER.csv#L301).

### Checkpoint 844: Tiamat typed dependencies leave its WLD graph unchanged

Tiamat remains 39 rooms with 116/116 authored exits resolving: 109 local and
seven outbound, across six weak components. The two active inbounds remain the
reciprocal Avernus arrival and one-way Hell One arrival; six one-way exits still
lead to Astral Tiamat. Astral Main's #19700 mobile reset/contact, the Astral
Tiamat OBJ #19607 import, unbound #19907 pool declaration, and disabled
#25106 reset add no WLD direction. The all-95 inactive scan found no approach
or room overlap; the cumulative source-only totals remain 11,993 overall and 415 Surf. Keep the CP110 graph and current unresolved reset classification.
See the [Tiamat typed-source refresh](TIAMAT_AUDIT_REVIEW.md#checkpoint-844-astral-main-import-and-typed-roles-refreshed),
[coverage row 301](ZONE_COVERAGE_REGISTER.csv#L302), and [candidate row 19](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L19).

### Checkpoint 845: City item contracts remain outside WLD connectivity

Lost City of Tikitzopl remains 340 rooms with 855/855 exits resolving: 853
local and two outbound, all in one weak component. Its Surface and Lost Temple
boundaries remain reciprocal, with 340/340 raw local reach and return from
#43700. Lost Temple's six crafting recipes, Surface Keeps' reset of City OBJ
#43708, and the Temple's type-25 return item do not add WLD edges. Inactive
Surf #248527 east to #43901 remains source-only on candidate row 97; this route
was already included in the cumulative totals of 11,993 overall and 415 Surf.
See the [typed-source refresh](TIKIT_AUDIT_REVIEW.md#checkpoint-845-current-cross-area-typed-closure-refreshed)
and [coverage row 302](ZONE_COVERAGE_REGISTER.csv#L303).

### Checkpoint 846: Temple item travel remains separate from WLD reach

Lost Temple of Tikitzopl remains 234 rooms with 616/616 authored exits
resolving (615 local, one outbound) in one weak component. Its two inbound
records remain the reciprocal City arrival and one-way Winterhaven arrival;
its only outbound is the City return. Raw reach from #44101 remains 234/234,
with the High Altar #44333 as the WLD sink. Six City-item crafting recipes,
Heavens OBJ #360, and Temple OBJ #44190's type-25 target to City #43704 do not
add WLD directions. The all-95 inactive scan still finds no Temple approach or
room overlap. See the [typed-source refresh](TIKITT_AUDIT_REVIEW.md#checkpoint-846-current-item-inputs-and-ownership-rechecked)
and [coverage row 303](ZONE_COVERAGE_REGISTER.csv#L304).

### Checkpoint 847: Torg typed dependencies remain separate from WLD reach

Torg remains 282 rooms and 624/624 resolving exits (623 local, one outbound)
in weak components of 265 and 17 rooms. Four active arrivals remain: the
reciprocal Surface pair plus one-way arrivals from Scorch Valley, Winterhaven,
and Juiblex. Three item-teleport objects remain separate from ordinary exits;
162 inactive targets from Torg-overlapping source VNUMs are excluded from
source-only approaches. Inactive Surf #277211 east to #28900 remains the one
distinct source-only approach on candidate row 97. The current cumulative
totals remain 11,993 overall and 415 Surf; Torg's edge was already counted.
The `rf_gh.shp` fallback collision and inactive resets/portals do not add WLD
ingress. See the [Torg current-source refresh](TORG_AUDIT_REVIEW.md#checkpoint-847-current-typed-owners-and-shop-loader-boundary-refreshed)
and [coverage row 304](ZONE_COVERAGE_REGISTER.csv#L305).

### Checkpoint 848: Torrhan typed references remain outside its WLD graph

Torrhan remains 289 rooms with 623/623 exits resolving (619 local, four
outbound) in one weak component. Its five active arrivals remain four
reciprocal Surface edges and the separate one-way Winterhaven administrative
arrival; 280/289 rooms are reachable from each arrival and 286/289 can reach an
outbound by WLD alone. The local #66646 item route, six active foreign reset
roles, shop products, and imported objects do not add WLD edges. The three
inactive Surf approaches to #66689/#66712/#66692 remain source-only on
candidate row 97. The current cumulative source-only total remains 11,993
overall and 415 Surf; these three edges are already counted. See the
[Torrhan typed-source refresh](TORRHAN_AUDIT_REVIEW.md#checkpoint-848-current-typed-ownership-and-shop-records-refreshed)
and [coverage row 305](ZONE_COVERAGE_REGISTER.csv#L306).

### Checkpoint 849: Torture graph and Ghore boundary remain unchanged

Torture remains 31 rooms with 61/61 authored exits resolving: 60 local and one
outbound, all in one weak component. The reciprocal Ghore boundary remains a
no-door pair. The separate interior gate, hatch, locked grate, and four fall
fields do not alter the raw WLD graph; all 31 rooms remain reachable from and
able to return to #95400 by authored exits. The all-95 inactive review still
finds no overlap or approach. No WLD edge or candidate count changes; current
cumulative source-only totals remain 11,993 overall and 415 Surf. See the
[Torture typed-source refresh](TORTURE_AUDIT_REVIEW.md#checkpoint-849-current-source-boundary-and-negative-typed-closure-refreshed)
and [coverage row 306](ZONE_COVERAGE_REGISTER.csv#L307).

### Checkpoint 850: Tower graph and inactive Surf approaches remain unchanged

Tower remains 100 rooms with 219/219 authored exits resolving: 218 local and one outbound, in one weak component. Its only active cross-area boundary is the reciprocal pair Tower #9334 south / Surface #586837 north. Raw directed traversal from #9300 reaches/returns 72/75 rooms when the two flag-8 switch edges are excluded and 92/99 when those edges are available; this is source-graph evidence, not qualified passage. Inactive Surf #227640 north to #9334 and #227641 north to #9379 remain source-only on candidate row 97 and were already counted in the current cumulative 11,993 overall / 415 Surf total. No active route or candidate count changes. See the [Tower source refresh](TOWER_OF_HIGH_SORCERY_AUDIT_REVIEW.md#checkpoint-850-current-graph-and-typed-closure-carried-forward), [coverage row 307](ZONE_COVERAGE_REGISTER.csv#L308), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 851: Tradeskills graph and dynamic portal remain distinct

Tradeskills remains a two-room WLD (#400000 and #400999) with 2/2 authored exits resolving as reciprocal local directions in one component. There is no active cross-area WLD edge, no inactive-WLD approach or room overlap, and no declared type-25 target into either room. Conditional death handling can create OBJ #400220 with the victim's room as destination and place it at #400000; that code-created portal remains outside the authored WLD graph and does not qualify entry, return, or safety. Tradeskills adds no approach to the current cumulative 11,993 overall / 415 Surf source-only total. The older 9,479 / 297 count in Checkpoint 396 is historical. See the [Tradeskills source refresh](TRADESKILLS_AUDIT_REVIEW.md#checkpoint-851-current-source-boundary-and-catalog-dependencies-carried-forward), [coverage row 308](ZONE_COVERAGE_REGISTER.csv#L309), and [old-catalog candidate row 108](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L108).

### Checkpoint 852: Trakkia graph and Surf approach remain unchanged

Trakkia remains 166 rooms with 387/387 authored exits resolving (385 local, two outbound) in five weak components of 109/28/16/12/1. The two outbound Surface edges and two inbound returns remain reciprocal pairs at #629225 and #629624. Four source-flagged blocked exits and four fall fields remain authored route/hazard conditions; raw reach scenarios do not qualify switches, keys, entry, or survival. Inactive Surf #246406 west to #57000 remains source-only on existing candidate row 97 and is already included in the current cumulative 11,993 overall / 415 Surf total. The Checkpoint 132 count of 2,611 overall / 153 Surf is historical. No active route or candidate count changes. See the [Trakkia source refresh](TRAKKIA_AUDIT_REVIEW.md#checkpoint-852-current-graph-source-only-approach-and-typed-closure-carried-forward), [coverage row 309](ZONE_COVERAGE_REGISTER.csv#L310), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 853: Traoul's graph and Truk boundary remain unchanged

Traoul remains 44 rooms with 91/91 resolving exits: 90 local and one outbound to Truk #1622, with the reciprocal Truk northbound edge to #7000. All-edge raw traversal reaches and returns to all 44 rooms in one strong component. The 14 reset door commands retain the authored closed, locked, and secret states, including the entrance's Truk-owned key #1609 and inner/tunnel references to #7009/#7001. The latter key still has no reset placement in the bounded source scan. No other active or inactive WLD approach or room overlap was found; no route or candidate count changes, and current source-only totals remain 11,993 overall / 415 Surf. See the [Traoul source refresh](TRAOUL_AUDIT_REVIEW.md#checkpoint-853-current-boundary-and-typed-closure-carried-forward), [coverage row 310](ZONE_COVERAGE_REGISTER.csv#L311), and [Truk pair rows](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L667).

### Checkpoint 854: Tribal Forest boundaries and routes remain unchanged

Tribal Forest remains 175 rooms with 462/462 resolving exits (453 local, nine outbound) in three weak components of 173/1/1. From #42281, authored WLD reach is 129 rooms and 172 can return. Three reciprocal Surface pairs remain at #42281; six outbound Astral Main exits from #42297 have no reverse WLD edge, and isolated #42374 still has no authored exit or static ingress. The reset-placed black rift's type-25 path to #42295 remains separate from WLD. No additional active or inactive approach or room overlap was found, so no route or candidate count changes; current cumulative source-only totals remain 11,993 overall / 415 Surf. See the [Tribal Forest source refresh](TRIBAL_FOREST_AUDIT_REVIEW.md#checkpoint-854-current-boundaries-and-typed-dependencies-carried-forward), [coverage row 311](ZONE_COVERAGE_REGISTER.csv#L312), and [Tribal Oasis audit](TRIBAL_OASIS_AUDIT_REVIEW.md).

### Checkpoint 855: Transparent Tower boundaries and Surf approach remain unchanged

Transparent Tower remains 100 rooms with 248/248 resolving exits (247 local, one outbound) in four weak components of 63/27/8/2. Its active boundaries remain reciprocal Tower #16212 / ConnectorZones #54167 and one-way Realm #14209 to Tower #16271. Inactive Surf #245010 north to #16212 remains source-only on existing candidate row 97 and is already included in the current 11,993 overall / 415 Surf total; the Checkpoint 400 count of 9,480 / 298 is historical. The unregistered Luiren old-format SHP room matches and active type-25 scan do not add WLD ingress. No active route or candidate count changes. See the [Transparent Tower source refresh](TRNSPTOW_AUDIT_REVIEW.md#checkpoint-855-current-graph-surf-approach-and-typed-closure-carried-forward), [coverage row 312](ZONE_COVERAGE_REGISTER.csv#L313), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 856: Troll Caves boundaries and source-only approaches remain unchanged

Troll Caves remains 82 rooms with 178/178 resolving authored exits in one local weak component and no current fields. Its active cross-area boundaries remain the reciprocal Surface pair #622564 down ↔ #96900 up and Malch #23741 east ↔ #96976 west. The three previously cataloged unregistered records from Surf #275641/#276840 and underdark2011 #272717 still target #96903/#96900; they remain source-only, do not add active ingress, and are already included in current cumulative totals of 11,993 overall / 415 Surf. The CP401 total of 9,480 / 298 is historical. Keep fixed-teleport and code-defined travel separate from WLD. No active route or candidate count changes. See the [Troll Caves source refresh](TROLL_CAVES_AUDIT_REVIEW.md#checkpoint-856-current-boundaries-and-typed-dependencies-carried-forward), [coverage row 313](ZONE_COVERAGE_REGISTER.csv#L314), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [underdark2011 candidate row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L115).

### Checkpoint 857: Troll Hills boundaries and Surf approaches remain unchanged

Troll Hills remains 148 rooms with 460/460 resolving exits (457 local, three outbound) in one weak component; all-room directed reach/return holds only when every authored local exit is treated as available. Three reciprocal pairs remain: Surface at #1801/#625367 and #1820/#626167, and Ghore at #1909/#11540. Lortower #134000 east to #1883 remains a separate one-way inbound. Two inactive Surf approaches (#278929 south to #1852 and #279529 north to #1871) remain source-only on existing candidate row 97 and are already included in current cumulative totals of 11,993 overall / 415 Surf. The Checkpoint 126 total of 1,999 / 146 is historical. The Airp #1823 object declaration and missing `bridge_troll` binding do not establish active WLD routes. No active route or candidate count changes. See the [Troll Hills source refresh](TROLL_HILLS_AUDIT_REVIEW.md#checkpoint-857-current-boundaries-and-source-only-approaches-carried-forward), [coverage row 314](ZONE_COVERAGE_REGISTER.csv#L315), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 858: Truktakyagyo boundaries and ud_map2 approach remain unchanged

Truktakyagyo remains 56 rooms with 115/115 resolving exits (112 local, three outbound) in one weak component. Three active reciprocal pairs remain #1600 east ↔ Underdark #846820 west, #1622 north ↔ Traoul #7000 south, and #1655 south ↔ Goblin Outpost #70198 north. Inactive ud_map2.wld #1126563 north to #1600 remains source-only on existing candidate row 113, is already counted in the current 11,993 overall / 415 Surf total, and adds no active ingress. The Checkpoint 403 total of 9,480 / 298 is historical. No active route or candidate count changes. See the [Truktakyagyo source refresh](TRUK_AUDIT_REVIEW.md#checkpoint-858-current-boundary-and-source-only-approach-carried-forward), [coverage row 315](ZONE_COVERAGE_REGISTER.csv#L316), and [ud_map2 candidate row 113](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114).

### Checkpoint 859: Twin Towers boundaries and typed dependencies remain unchanged

Twin Towers remains 99 rooms with 211/211 resolving exits (207 local, four outbound) in one weak component. Four reciprocal boundaries remain: #13200/#13570, #13230/#13571, and #13259/#13569 with Twin Towers Forest, plus #13297/#30167 with Underworld 3. One authored fall at #13239 and no current fields remain recorded. The all-95 inactive scan found no source-room overlap or source-only approach; current cumulative totals remain 11,993 overall / 415 Surf. The Checkpoint 78 and 404 totals of 680/110 and 9,480/298 are historical. Heart QST/sidecar consumers and compiled #13219/#13272 references do not add WLD routes. See the [Twin Towers source refresh](TWIN_TOWERS_AUDIT_REVIEW.md#checkpoint-859-current-map-boundaries-and-typed-closure-carried-forward), [coverage row 316](ZONE_COVERAGE_REGISTER.csv#L317), and the [separate Twin Towers Forest entry](TWIN_TOWERS_FOREST_AUDIT_REVIEW.md).
## Checkpoint 860: Tundra boundaries and Surf approaches remain unchanged

CP860 carries forward the active Tundra census of 175 rooms and 401 authored exits (399 loader-resolved), its eight reciprocal boundary pairs, the one-way Heavens reception edge, the unusable `-1` exit, and the inactive Duris3 destination. The seven `surf.wld` records still target Tundra rooms only from unregistered source and do not add active ingress. Preserve them on existing Surf candidate row 97. They are already included in the current cumulative source-only total of 11,993 overall / 415 Surf; Checkpoint 405's 9,480 / 298 is historical. Door/reset state, live reachability, safe traversal, actor availability, and return remain unqualified. See the [Tundra audit](TUNDRA_AUDIT_REVIEW.md#checkpoint-860-current-boundaries-and-typed-closure-carried-forward), [source dossier](../zone-stories/TUNDRA.md#access-actual-availability-and-services), and [coverage row 318](ZONE_COVERAGE_REGISTER.csv#L318).
## Checkpoint 861: Turolopolis Zoo graph and boundary remain unchanged

CP861 carries forward the 62-room, 141/141 resolving-exit Zoo graph (140 local, one outbound) and one weak component. Willem #7123 west and Zoo #53000 east form the sole reciprocal active boundary. With authored exits open, raw reach and return from #53000 are 62/62; treating the closed/secret/blocked #53019 down exit as unavailable gives reach 60/62 and return 62/62. The all-95 inactive-WLD sweep found no room overlap or approach into the Zoo. The older 9,480 overall / 298 Surf subtotal at Checkpoint 406 is historical; current source-only totals are 11,993 overall / 415 Surf. Static traversal does not qualify door procedures, encounters, safe return, or installed access. See the [Turolopolis Zoo audit](TUROLZOO_AUDIT_REVIEW.md#checkpoint-861-current-source-boundary-and-typed-closure-carried-forward), [source dossier](../zone-stories/TUROLZOO.md#access-controls-and-sharedcustom-caller-boundaries), and [coverage row 319](ZONE_COVERAGE_REGISTER.csv#L319).
## Checkpoint 862: Twin Towers Forest boundaries and typed routes carried forward

CP862 carries the Forest's 100-room, 259/259 resolving-exit graph, two weak components (largest 99 rooms), four reciprocal outbound boundaries, and two additional load-room inbounds to #13595 forward. The all-95 inactive-WLD sweep found no overlap or approach into the Forest. Nax's #13563 reset/shop reference and source-only Nexus56 flower resets/shop list are item dependencies, not WLD routes; the Forest's New Cave City wizard reset also does not add a route. Older CP407/CP859 global source-only subtotals are historical; current totals are 11,993 overall / 415 Surf. Ordinary arrival, safe return, reset/shop operation, and public access from quest-load rooms remain unqualified. See the [Forest audit](TWIN_TOWERS_FOREST_AUDIT_REVIEW.md#active-graph-and-directed-boundary-records), [source dossier](../zone-stories/TWIN_TOWERS_FOREST.md), and [coverage row 320](ZONE_COVERAGE_REGISTER.csv#L320).
## Checkpoint 863: Twisted Wood boundaries and Surf approaches remain unchanged

CP863 carries forward 100 rooms and 335/335 loader-resolving exits (331 local, four outbound) in one weak component. Raw authored traversal from #16300 reaches 97 rooms and every room has a return path; #16397–#16399 have no authored incoming local exit. Four outbound boundaries reciprocate with Surface, Pine Hollow, and Krindor; two more Pine Hollow inbounds reach #16308/#16344 from NPC-load rooms. Five unregistered `surf.wld` exits still target #16300–#16303 and do not establish active ingress. They are already included in current cumulative totals 11,993 overall / 415 Surf; CP127's 2,004 / 151 and CP408's 9,480 / 298 totals are historical. Concealed SEARCH/OPEN state, actor placement, safe return, and conditional SHIFT PRIME arrival remain unqualified. See the [Twisted Wood audit](DARK_AND_TWISTED_WOOD_AUDIT_REVIEW.md#checkpoint-863-current-surf-approaches-and-dependencies-carried-forward), [source dossier](../zone-stories/DARK_AND_TWISTED_WOOD.md#access-concealed-camp-and-optional-treasure), and [coverage row 321](ZONE_COVERAGE_REGISTER.csv#L321).
## Checkpoint 864: UDMini arrivals and fall state remain unchanged

CP864 carries forward 21 rooms and 39/39 resolving exits (38 local, one outbound), with local weak components of 19 and 2. ConnectorZones #54239 down to #88502 and Kvarkpass Connector #54811 up to #88515 are the two inbounds; #88502 up to ConnectorZones #54194 is the only outbound, and none is reciprocal. Both arrivals reach the 19-room cave component; mushroom rooms #88500–#88501 remain outside the active arrival graph. F 100 at #88502/#88520 and the reset-placed #88503 teleport to the F 100 Kvarkpass shaft still prevent a static safety or return conclusion. The all-95 inactive scan found no overlap or approach. CP209's 3,458 overall / 220 Surf / 17 `ud_map2` aggregate is historical; current totals are 11,993 overall / 415 Surf. See the [UDMini audit](UDMINI_AUDIT_REVIEW.md#checkpoint-864-current-arrivals-fall-state-and-spore-dependencies-carried-forward), [source dossier](../zone-stories/UDMINI.md#entrances-falling-and-the-return-crack), and [coverage row 322](ZONE_COVERAGE_REGISTER.csv#L322).
## Checkpoint 865: Ugta boundary and source-only Surf path remain unchanged

CP865 carries forward Ugta's 242-room, 481/481 resolving-exit map, five weak components (238 rooms plus singleton #39300–#39303), and sole reciprocal Surface pair #39335 ↔ #543298. Gate and prison-door resets remain closed, with the cell doors also locked and key fields 0; no current or fall fields are authored. The `surf.wld` route from #209731 north to #39335 remains source-only and outside active ingress counts. It is preserved on Surf candidate row 97 and is already reflected in current cumulative source-only totals 11,993 overall / 415 Surf. These static records do not establish lawful entry, gate operation, safe river crossing, or prisoner release. See the [Ugta review](UGTA_STORY_REVIEW.md#checkpoint-865-current-source-only-ugta-approach-carried-forward), [source review](UGTA_STORY_REVIEW.md#explicit-builder-questions-and-recommended-resolutions), and [coverage row 323](ZONE_COVERAGE_REGISTER.csv#L323).
## Checkpoint 866: Durian Underdark graph and 2011 candidate remain distinct

CP866 carries forward 13,587 active rooms and 53,661 authored exits: 46,848 loader-resolved (46,747 local, 101 outbound) and 6,813 absent sparse-map targets pruned. The 101 active inbound/outbound records still form reciprocal pairs; six local weak components remain, largest 13,192. The all-95 inactive-WLD sweep found no overlap or approach into current `underdark`. Preserve the separate `underdark2011` candidate's 11 active unresolved-owner exits from five sources and the #217609 identity ambiguity across Surf, Surface2011, and Underdark2011. CP119's 1,532 overall / 142 Surf subtotal is historical; current cumulative source-only totals remain 11,993 overall / 415 Surf. Terrain entry conditions and dynamic travel do not follow from the sparse static graph. See the [Durian Underdark audit](DURIAN_UNDERDARK_AUDIT_REVIEW.md#checkpoint-866-sparse-map-and-candidate-boundaries-carried-forward), [underdark2011 candidate row 115](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L115), and [coverage row 324](ZONE_COVERAGE_REGISTER.csv#L324).
## Checkpoint 867: Undermountain boundaries and sparse routes remain unchanged

CP867 carries forward 441 rooms and 1,059/1,059 resolving exits in one weak component. The Brass Fingers boundary #92501 south ↔ #74045 north is reciprocal; #92518 down reaches Underworld #4557 without a direct return, and #4557 exits up to Kvarkpass Connector #54841. The all-95 inactive-WLD scan found no room overlap or exit target into Undermountain. CP51's 586 overall / 26 Surf source-only subtotal is historical; current cumulative totals remain 11,993 overall / 415 Surf. Static map edges do not qualify door state, ordinary access, safe travel, or live return. See the [Undermountain audit](UNDERMOUNTAIN_AUDIT_REVIEW.md#checkpoint-867-current-map-and-typed-owners-carried-forward), [source dossier](../zone-stories/THE_RUINS_OF_UNDERMOUNTAIN.md), and [coverage row 325](ZONE_COVERAGE_REGISTER.csv#L325).
## Checkpoint 868: Underworld boundaries and inactive approaches remain unchanged

CP868 carries forward 232 rooms and 511 authored exits (510 resolved: 484 local, 26 outbound; one unresolved #4645→#4646), five weak components, 30 active inbounds, and 26 outbound records forming 26 reciprocal pairs. Thirteen current fields and no fall fields remain authored. Three source-only approaches still enter #4469/#4608 from `ud_map0`/`ud_map2`; the all-95 scan found no room overlap. Keep them outside active ingress and preserve their sources on candidate rows 112 and 114. CP128's 2,007 overall / 151 Surf subtotal is historical; current cumulative totals remain 11,993 overall / 415 Surf. See the [Underworld audit](UNDERWORLD_AUDIT_REVIEW.md#checkpoint-868-current-source-only-approaches-and-dependencies-carried-forward), [source dossier](../zone-stories/UNDERWORLD.md), and [coverage row 326](ZONE_COVERAGE_REGISTER.csv#L326).
## Checkpoint 869: Underworld3 map identity and active routes carried forward

CP869 carries forward 283 rooms and 682/682 resolving exits in two weak components, with nine active inbounds, eight outbound records, and eight reciprocal pairs. `mobs_underdark` #24007 north to #30149 remains one active inbound-only edge. The all-95 inactive-WLD scan reports no outside target reference into the current map. Unregistered Cabal Hall, Bastard, Light Guildhall, Nah, Shadow Hall, and Sunhall numeric overlaps remain separate candidate identities, not active routes. The shared “The Underworld” title does not merge active `underworld` and `underworld3`. Current cumulative source-only totals remain 11,993 overall / 415 Surf. See the [Underworld3 audit](UNDERWORLD3_AUDIT_REVIEW.md#active-map-boundaries-and-title-identity), [source dossier](../zone-stories/UNDERWORLD3.md), and [coverage row 327](ZONE_COVERAGE_REGISTER.csv#L327).
## Checkpoint 870: Unique object consumers retain one prototype owner

CP870 reconciles the full tracked-source object consumer scan for active unique: 42 external ZON reset commands across 38 active area contributors import 42 distinct Unique OBJ prototypes. Four further commands occur only in candidate sources Brass Old 1, Brass Old 2, and Nexus56, and remain attached to their existing plans. No external QST/SHP prototype reference or type-25 destination to #67200/#67300 was found. The two-room WLD remains only the reciprocal #67200/#67300 pair from CP415; no external WLD edge is added. See the [Unique typed audit](UNIQUE_AUDIT_REVIEW.md#active-external-zon-imports), [title/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-765-unique-vault-title-and-object-ownership), and [coverage row 328](ZONE_COVERAGE_REGISTER.csv#L328). Current cumulative source-only totals remain 11,993 overall / 415 Surf.
## Checkpoint 871: Undead Outpost code route and bank-object dependency

CP871 keeps the 121-room WLD graph at 250 authored / 249 resolving exits, with the Surface #605489 and Crypt #14300 boundaries reciprocal and the #98737 north target -1 unresolved. The all-95 inactive-WLD scan adds no target or overlap. The compiled shift_prime innate has a separate possible arrival: when used in Astral by RACE_PHANTOM, its destination list includes Main Passage #98731. This is a code-selected route, not a WLD edge or general access promise. The #98735 do_descend literal is behind an unconditional disabled-command return and is not counted as current access. The ZON also imports active Tharnadia sign counter OBJ #132581 into #98820; the bank commands recognize that object, while Tarn's money_changer callback says coin exchange has moved to the Royal Bank. See the [Undead Outpost typed audit](UNOUTPOST_AUDIT_REVIEW.md#active-graph-and-code-bound-arrival), [builder questions](../zone-stories/UNDEAD_OUTPOST.md#explicit-builder-questions-and-recommended-resolutions), [Tharnadia owner audit](THARNADIA_AUDIT_REVIEW.md), and [coverage row 329](ZONE_COVERAGE_REGISTER.csv#L329). Current cumulative source-only totals remain 11,993 overall / 415 Surf.
## Checkpoint 872: Val graph and source-only Surf approach remain unchanged

CP872 reconfirms Val's 174 rooms and 374/374 resolved exits in three weak components, with the sole active reciprocal Surface boundary at #38575/#521073. The all-95 inactive-WLD scan still finds no room overlap; unregistered surf.wld room #205053 north to #38575 remains source-only on existing Surf candidate row 96. No WLD count or active ingress changes. Current typed source closure still has only Verspin's active G reset of Val dagger #38421, while the 13 chaos-kit exact-number matches remain static table data for three Val objects and the type-25 scan finds no arrival into Val. See the [Val audit](VALOISIAN_CASTLE_AUDIT_REVIEW.md#checkpoint-872-current-tracked-source-refresh), [source dossier](../zone-stories/THE_VALOISIAN_CASTLE.md), [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [coverage row 331](ZONE_COVERAGE_REGISTER.csv#L331). Current cumulative source-only totals remain 11,993 overall / 415 Surf.

## Checkpoint 873: Valinhaven source-only and code-selected approaches

Valinhaven remains a 159-room active map with 320/320 loader-resolving exits in one weak component. Its two active outbound WLD records are reciprocal: #90800 east to Underdark #798725 and #90810 north to Dirk’nspire #96800. The all-95 inactive-WLD scan finds no room overlap and retains two source-only approaches into #90800: ud_map1 room #1113221 north and ud_map2 room #1123855 east. These remain on their existing candidate plans and are not active ingress. The compiled do_shift_prime route can also select #90803 for Githyanki/Pillithid or Illithid while the character is in Astral; this is a conditional code route, separate from WLD connectivity. Static routes do not establish safety, live access, or return. See the [Valinhaven audit](VALINHAV_AUDIT_REVIEW.md), [source-only map candidates](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L113), [second candidate](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114), and [coverage row 333](ZONE_COVERAGE_REGISTER.csv#L333). Current curated source-only totals remain 11,993 overall / 415 Surf.

## Checkpoint 874: Valley of Crushk typed dependencies reconfirmed

The CP421 graph census remains unchanged: 85 rooms, 182/182 resolving exits in one weak component, two active inbounds, and two outbound records (the reciprocal Pharr route and one-way Surface departure). The all-95 inactive-WLD sweep still has one non-overlapping source-only Surf exit from #232517 north to #2060; it remains on Surf candidate row 97 and is not active ingress. A current typed-source refresh reconfirms the Ship Yards quest item, Heavens imports, Alatorin/Charing/Surface Keeps object consumers, and unresolved local reset M #207. No new route or source-only count is added; current curated totals remain 11,993 overall / 415 Surf. See the [Valley audit](VALLEY_CRUSHK_AUDIT_REVIEW.md#checkpoint-874-current-typed-source-refresh), [route review](VALLEY_CRUSHK_AUDIT_REVIEW.md#active-map-incoming-leaves-and-exits), [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [coverage row 334](ZONE_COVERAGE_REGISTER.csv#L334).

## Checkpoint 875: Vehicles static graph stays separate from coded travel

Vehicles retains 216 rooms and 464/464 resolving WLD exits in eight weak components, with five fall fields, no current fields, no active WLD boundary, and no inactive-WLD target into the local room set. Seven compiled ferry definitions name active external WLD stops; the separate transport table maps 16 local vehicle mobiles to Surface loading rooms. These code tables are not WLD edges, and ferry path construction, admission, access, availability, and return remain unqualified. The current typed refresh records 81 defined external ZON reset operands, plus unresolved M #6124/#6123; QST and SHP imports are owner-linked in the [Vehicles audit](VEHICLES_AUDIT_REVIEW.md). Preserve static connectivity and code routes as distinct evidence. See the [Vehicles builder questions](../zone-stories/VEHICLES_BUILDER_QUESTIONS.md), [coverage row 337](ZONE_COVERAGE_REGISTER.csv#L337), and [route tables](../../../src/world/ferryact.c#L98).

## Checkpoint 876: Verspin's inactive Surf approach remains source-only

CP876 carries forward the CP425 200-room, 435/435-resolving-exit Verspin graph, with two active inbounds and one outbound record forming one reciprocal Surface pair. The all-95 inactive-WLD sweep found no room overlap and one source-only Surf exit: #229065 north to active Verspin #28100 ([surf.wld line 609705](../../../areas/wld/surf.wld#L609705)). Keep it on the existing Surf candidate data row 96 / CSV line 97; this tuple is already included in current curated source-only totals of 11,993 overall / 415 Surf and adds no active ingress or aggregate count. The typed refresh records 21 outbound ZON reset operands across seven active owners, the Mntcastl reset of local object #28146, Verspin's Heavens and Bloodstone item dependencies, and code-bound crew/stat shop rooms. No other external QST item/giver, SHP product, or story-sidecar typed reference targets Verspin; no duplicate prototype, type-25 destination, or ITEM_SWITCH destination into the map was found. Keep source owners and code bindings separate from static WLD reachability. See the [Verspin audit](VERSPIN_AUDIT_REVIEW.md), [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [coverage line 338](ZONE_COVERAGE_REGISTER.csv#L338).

## Checkpoint 877: Fateful Falls remains a retained two-room source fragment

CP877 records the unregistered `beggin` WLD separately from active connectivity. Its #3300 east / #3301 west pair forms one two-room local component and has no outbound exit. Exact target scans found no active WLD entrance; all other inactive WLD sources were checked for source-room overlap or approach, with none found. A generic parser flagged two unrelated legacy exit-field layouts in active `northern_wilderness.wld`; their raw targets are #12270 and #6803, not Fateful Falls. Keep `beggin` outside active route counts and preserve its RETAIN IN PLACE candidate status. See the [Fateful Falls audit](FATEFUL_FALLS_AUDIT_REVIEW.md#candidate-graph-and-map-ownership), [source dossier](../zone-stories/FATEFUL_FALLS.md), [candidate row 14](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L15), and [coverage row 366](ZONE_COVERAGE_REGISTER.csv#L366).

## Checkpoint 880: Alterian Wilderness source-only inbound routes

An all-95 inactive-WLD scan found three other-file exits into `alterian_wilderness.wld`: Alterian Mountains #4920 west to #4919 and #4980 down to #4750, plus Surf #229817 west to #4915. All three reciprocate the existing Wilderness outbound records. No other inactive WLD file overlaps or approaches the map, and no active WLD edge targets a Wilderness-only room identity. Same-number room targets in active Mining and Tchan resolve to those active maps. Preserve the three source-only inbound pairs on existing Mountain and Surf candidate plans; they do not establish live traversal. See the [Wilderness audit](ALTERIAN_WILDERNESS_AUDIT_REVIEW.md#checkpoint-881-external-source-identities-reconciled), [candidate row 8](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L8), and [coverage row 359](ZONE_COVERAGE_REGISTER.csv#L359).


## Checkpoint 882: Aeon and Genesis numeric WLD matches classified

The full-source WLD recount found 18 exits in `genesis.wld` whose numeric targets fall
inside #29900–#29909. All are internal edges in Genesis's own ten-room map. Since Genesis
and Aeon define different rooms at the same VNUMs, these matches remain Genesis-owned
source edges and are not evidence of WLD ingress into Aeon. No active or other tracked WLD
source targets Aeon; its active inbound count remains zero. The existing Aeon D4 source
exit from #29900 to Duris3 #153244 remains a source-only outbound record, with loader and
return behavior unresolved. No aggregate route or source-only count changes. See the
[Aeon audit](AEON_GH_AUDIT_REVIEW.md#checkpoint-882-role-aware-tracked-source-closure),
[candidate row 2](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L2), and
[coverage row 353](ZONE_COVERAGE_REGISTER.csv#L353).

## Checkpoint 884: Duris3 typed and code-room references

The new role-aware pass found no active ZON reset room operand into Duris3
identity. The 735 candidate-only ZON room operands belong to `forofcon` and
`brass-old-1`, and every one resolves to its own same-stem WLD. Candidate-range
QST values are mobile, item, coin, or experience operands; story-sidecar values
have no room/route paths; and none of the 720 parsed shop-room fields identifies
a Duris3-only room.

Twenty typed OBJ room operands remain source evidence: nineteen are local to
`forofcon.wld` (type-25 destinations and type-29 switch-room operands), while
active Fort Boyard object #38143 stores type-25 target #149188 and is reset in
Fortb room #38100. The target is defined only by Duris3 among current WLD files;
Duris3 is not loaded by the current full-area manifest. The compiled room
references at #140854, #145443, and #154938 are a shipwright-service callback and
conditional random-lab entrances. Encounter range checks also cover a band that
would include Duris3 if loaded. None adds a static WLD direction or proves a
current live route. Preserve the existing graph counts and keep these typed/code
references under owner review; candidate row 36 remains **RETAIN IN PLACE**.
See the [Duris3 audit](DURIS3_AUDIT_REVIEW.md#checkpoint-884-typed-room-roles-and-compiled-consumers), [map22 review](MAP22_AUDIT_REVIEW.md), and [coverage row 387](ZONE_COVERAGE_REGISTER.csv#L387).

## Checkpoint 885: Barovia directed reachability and cross-area closure

Barovia’s source WLD contains 168 rooms and 442/442 resolving D0–D9 directions in two weak components (167 rooms and isolated #91020). Raw directed traversal reaches 158 rooms from active arrivals #91000, #91010, #91121 and #91162; it reaches 163 from glen rooms #91163–#91165. The ordinary arrival roots cannot follow the WLD graph into #91163–#91167; item teleport #91064 at #91012 separately enters #91163. All counts ignore dynamic door opening, mobile movement, scripted access and safety.

Six active WLD edges enter Barovia: Bloodstone #74057→#91000; Barovia Continued #58800→#91162; and the Ravenloft Catacombs (`ravenloft2`) #59355→#91121, #59359→#91165, #59359→#91164, and #59398→#91010. Three active edges leave: #91000→Bloodstone #74057, #91162→Barovia Continued #58800, and #91167→Castle Ravenloft #58568. The first two have reciprocal records; the Castle boundary has no direct reverse WLD target to #91167. A full scan of all 446 repository WLD files found no extra active or unregistered WLD ingress into Barovia.

The two Barovia gates in the Glen reset locked: #91164 D0→#91166 uses key #91037, and #91165 D0→#91167 uses Castle key #58826 (`barovia.zon:184,188,192,196`). At the Castle landing, room #58568 has a D0 gate to #58567, reset closed/locked by #58826 (`ravenloft.zon:440,444`); D1 also leads to #58300. Castle’s #58567 D0 edge returns to Barovia Continued #58958. The shared key #58826 also has a 100-percent break chance and serves the #91165 gate. These authored edges and reset states establish a keyed onward route but do not qualify a safe end-to-end return or live door operation.

Barovia’s WLD results and typed source closure are documented in the [Barovia dossier](../zone-stories/THE_REALM_OF_BAROVIA.md#checkpoint-885-directed-access-and-role-aware-cross-area-closure) and [coverage row 25](ZONE_COVERAGE_REGISTER.csv#L25). The area remains active and **DO NOT MOVE**.

### Checkpoint 886: EOO role-aware connectivity and shop-source closure

The EOO WLD totals and candidate inbound count remain unchanged: ten rooms, 19 directions, one one-way Duris3 edge, zero active inbound exits to EOO-only #29200, and no newly counted approach. The 22 active WLD matches to #29201-#29209 remain Newbie-owned. A role-aware reset scan found 25 room operands in `dark_gh`, `genocide_gh`, and `newbie` that land on the shared numeric band; source ownership keeps them separate from EOO. Same-number OBJ portal targets in `dark_gh`, `eoo_op`, and `eoo_op2` likewise target shared Newbie room IDs rather than proving EOO ingress.

Three other parser-valid `N`-format SHP records share the band: the `dark_gh` and `genocide_gh` shops use room #29204, while the active Newbie shop uses room #29201; their keepers resolve within those same source identities. EOO's record itself names #29204 as keeper and shop room and lists fourteen products. The non-mini fallback glob includes `eoo.shp` when `areas/world.shp` is missing, but the shard set also contains eight legacy `O`-format records in `luiren.shp`, which the current `N`-only parser rejects. Source inclusion is not evidence of a successfully loaded fallback catalog. Curated WLD totals remain 11,987 overall / 415 from Surf. See the [EOO audit](EOO_AUDIT_REVIEW.md#checkpoint-886-role-aware-reference-closure-and-shop-stream-qualification), [24 builder questions](EOO_BUILDER_QUESTIONS.md), [candidate row 37](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L37), and [coverage row 387](ZONE_COVERAGE_REGISTER.csv#L388).

### Checkpoint 887: EOO-OP typed boundaries

The EOO-OP WLD graph is unchanged: three rooms, five directions, one one-way exit to inactive Duris3 #123225, and zero candidate-identity inbound exits. The seven active Newbie directions to #29210-#29212 remain Newbie-to-Newbie. Role-aware ZON scans found seven dark_gh room placements and four Newbie room placements in the shared band; eight Newbie mobile reset rows use #29210-#29213, while EOO contributes three explicit O object imports of EOO-OP portal IDs #29211-#29213. The EOO imports preserve source intent but do not change the active Newbie prototype owner. Two source-only dark_gh type-25 objects target #29211/#29212; they are not an EOO-OP return edge. No QST/story-sidecar, parser-valid SHP, or direct code consumer was found for #29210-#29213. Curated totals remain 11,987 overall / 415 from Surf. See the [EOO-OP audit](EOO_OP_AUDIT_REVIEW.md#checkpoint-887-typed-role-boundary-closure), [24 builder questions](EOO_OP_BUILDER_QUESTIONS.md), [candidate row 38](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L38), and [coverage row 388](ZONE_COVERAGE_REGISTER.csv#L389).

### Checkpoint 888: EOO-OP2 typed boundaries

EOO-OP2 connectivity remains unchanged: six rooms in two three-room components, nine WLD directions, one-way #29223 up to inactive Duris3 #151855, and zero candidate-identity inbound exits. The 16 active Newbie directions to #29220-#29225 remain Newbie-owned. External ZON room fields include eight dark_gh placements and six Newbie room operands; five Newbie M rows also reset Newbie mobiles #29220-#29223, while two Newbie O rows place Newbie items #29220/#29221. `newbie.qst` and `newbie.story.json` matches are typed item references, including the salt-box/vial identities at #29220/#29221, not room routes. Two eoo_op and two dark_gh type-25 portals target shared EOO-OP2 numbers; their owners and targets remain distinct. No external type-29, parser-valid N-format SHP, or other direct code room reference was found. Curated totals remain 11,987 overall / 415 from Surf. See the [EOO-OP2 audit](EOO_OP2_AUDIT_REVIEW.md#checkpoint-888-eoo-op2-typed-role-boundary-closure), [24 builder questions](EOO_OP2_BUILDER_QUESTIONS.md), [candidate row 39](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L39), and [coverage row 389](ZONE_COVERAGE_REGISTER.csv#L390).

### Checkpoint 889: eskull_hall typed-role connectivity and shop dependencies

The 13-room / 24-direction Ebon Skull graph remains two components of 10 and 3 rooms, with its two one-way Duris3 exits, thirteen active Newbie numeric targets, and zero candidate-identity inbound edges. The 65 external ZON room operands, 27 mobile-prototype operands, and six object-prototype operands fall in cbr_hall, ethdawn_op, loc_gh, Newbie, or ravenmis source identities. Six source-owned type-25 objects point into the shared room band; none establishes an Ebon Skull route. Four external parser-valid SHP records use matching keeper, room, or product IDs, while the candidate shop itself has keeper #29303, room #29302, and fifteen products. The global non-mini fallback includes all shards, but the eight legacy `O` records in `luiren.shp` stop the current `N`-only parser through its fatal error path; no successful catalog load is established. Curated totals remain 11,987 overall / 415 from Surf. See the [Ebon Skull audit](ESKULL_HALL_AUDIT_REVIEW.md#checkpoint-889-role-aware-source-and-shop-field-closure), [24 builder questions](ESKULL_HALL_BUILDER_QUESTIONS.md), [candidate row 40](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L40), and [coverage row 390](ZONE_COVERAGE_REGISTER.csv#L391).

### Checkpoint 890: ethdawn_op role-aware boundaries

The outpost's three-room/four-direction WLD graph, one-way Duris3 #156745 exit, six active Newbie target matches, and zero candidate-identity inbound count remain unchanged. Sixteen external ZON room-role operands come from eskull_hall (6), loc_gh (7), and Newbie (3); none adds an exit. There are no external mobile-role reset refs. Ravenmis has one object-role reset of #29311 into #29307, while the active Newbie prototype at #29311 is arrows and ethdawn_op's is a portal. Ebon Skull portal #29300 also stores #29312, shared by ethdawn_op, ESKULL, and active Newbie. Newbie QST/story hits for #29310-#29312 are item fields, not room destinations. No type-29, parser-valid N-format SHP, or direct C/C++ numeric consumer was found. Curated totals remain 11,987 overall / 415 from Surf. See the [ethdawn_op audit](ETHDAWN_OP_AUDIT_REVIEW.md#checkpoint-890-role-aware-portal-and-reset-closure), [24 builder questions](ETHDAWN_OP_BUILDER_QUESTIONS.md), [candidate row 41](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L41), and [coverage row 391](ZONE_COVERAGE_REGISTER.csv#L392).

### Checkpoint 891: fists_gh role-aware resets and shop input

Fists of Fury connectivity remains ten rooms and 21 directions, one local weak component, a one-way up exit to inactive Duris3 #157556, and zero candidate-identity active inbound. Its two source-only approaches remain in the earlier curated totals. All 70 external ZON room-role and 24 mobile-prototype operands matching #30500-#30509 are in active Patrols' own resets, so they do not indicate Fists resets. No external type-25/type-29 target, QST/story-sidecar role, or SHP keeper/room reference to the range was found. The candidate's own keeper/room fields are #30503/#30502, which resolve numerically to Patrols under the conditional fallback. That fallback also contains eight legacy `O`-format Luiren shop records rejected by the current `N`-only parser. Curated totals remain 11,987 overall / 415 from Surf. See the [Fists audit](FISTS_GH_AUDIT_REVIEW.md#checkpoint-891-patrols-role-matches-and-shop-stream-qualification), [24 builder questions](FISTS_GH_BUILDER_QUESTIONS.md), [candidate row 42](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L42), and [coverage row 392](ZONE_COVERAGE_REGISTER.csv#L393).


### Checkpoint 892: fooquest role-aware boundaries

FooQuest's graph remains 17 rooms and 30 internal directions: sixteen ship rooms in one weak component and isolated #65050, with zero active inbound WLD exits. The 441 outside ZON files have no candidate-range room, mobile-prototype, or object-prototype operand. The 264 outside QST files and 232 parsed story sidecars have no typed giver/item consumer; none of 872 type-25 portal targets or 380 type-29 switch room targets enters a FooQuest room. FooQuest defines no local portal/switch object and has no SHP shard. No WLD approach or component count changes. Its 712 external parser-shaped `N` shop records also have no candidate keeper, room, or product field; the global shard fallback remains conditional because the eight legacy Luiren `O` records fail the current parser. Curated inactive-WLD totals remain 11,987 overall / 415 from Surf. See the [FooQuest audit](FOOQUEST_AUDIT_REVIEW.md#checkpoint-892-role-aware-external-closure-and-room-target-scan), [candidate row 43](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L43), and [coverage row 393](ZONE_COVERAGE_REGISTER.csv#L394).


### Checkpoint 893: forestb has no static map ingress

Forestb still has no WLD or ZON source, so it has no candidate graph metrics, authored resets, or active inbound room edge. The SHP room #38259 has no current WLD owner; no WLD record/reference, outside type-25 destination, or type-29 switch room field targets it. Forestb's own type-25 objects #38234 and #38253 point outward to #38297 and #38295, neither of which has a current WLD definition; those object portals do not add WLD edges or establish a live destination. All 442 ZON files, 264 external QST files, and 232 sidecars have no typed Forestb consumer. Of the 712 parser-shaped `N` shops, only Forestb's own row matches keeper #38235, room #38259, and its four products. The conditional all-shard fallback still contains eight Luiren `O` records rejected by the current parser, so shop loading and later keeper assignment remain unqualified. Curated inactive-WLD totals remain 11,987 overall / 415 from Surf. See the [Forestb audit](FORESTB_AUDIT_REVIEW.md#checkpoint-893-role-aware-cross-source-closure), [candidate row 44](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L44), and [coverage row 394](ZONE_COVERAGE_REGISTER.csv#L395).


### Checkpoint 894: role-aware Smoke and portal identity closure

Forofcon's CP482 WLD totals and the five previously counted source-only approaches remain unchanged. The 441 outside ZON files contain 78 M/F/O room operands and four D room operands only in active `smoke.zon`; those target active Smoke identities in the shared #139900-#139952 band. Two external type-25 objects, Smoke #139802 and #139819, target Smoke rooms #139937 and #139941. Forofcon's own 16 type-25 objects and four type-29 switches target candidate WLD rooms and have candidate-local object resets. One destination, #140030→#139903, numerically resolves to active Smoke while the candidate map is absent; it would identify the separate Forofcon record only if that WLD is loaded and selected as the owner. Loader selection is not established. The other 19 targets are Forofcon-only rooms. The scan found no external mobile/object reset prototype, QST/story/shop role, same-type MOB/OBJ definition, or external type-29 room target in the candidate ranges.

There is also executable code tied to the numeric room band: active Olympus `olympus_portal` uses #140000 as a zone lookup base, and `random_encounters` compares room indexes bounded by `real_room(140000)` and `real_room(149999)`. #140000 belongs numerically to Forofcon and inactive Duris3, while #149999 is only in inactive Duris3 source; absent-room fallback behavior leaves the intended identities unresolved. These source references do not add WLD edges or prove a current route. No graph or curated total changes; retain Forofcon pending identity, manifest, editor/dynamic-path, and persisted-ID review. See the [Forofcon audit](FOROFCON_AUDIT_REVIEW.md#checkpoint-894-role-aware-smoke-and-portal-identity-closure), [candidate row 45](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L45), and [coverage row 395](ZONE_COVERAGE_REGISTER.csv#L396).


### Checkpoint 895: Genesis role matches remain Aeon-owned

Genesis connectivity remains ten rooms, 19 direction records, one weak component, and directed reach to all ten rooms from #29900. Its only outbound WLD edge remains #29901 D4 to inactive, unlisted Duris3 #157747; no WLD source outside Genesis and aeon_gh targets the shared #29900–#29909 range, and neither component is active in the full area list. The role scan does not change active inbound, source-only approach, or curated connectivity totals.

The external ZON matches are all in unregistered `aeon_gh.zon`: four M resets of Aeon mobile #29900 to Aeon room #29900, one M reset of Aeon mobile #29901 to Aeon room #29909, and three O placements in Aeon rooms #29900, #29902, and #29903. No other ZON, QST, or story-sidecar role matched Genesis's room/mobile VNUMs, and no external type-25 or type-29 OBJ destination targets a Genesis room. Aeon's shop uses keeper #29901 in room #29909 and shares all nine product VNUMs with the Genesis shop; these remain Aeon shop roles and do not add map edges. The only direct compiled room binding remains the inn assignment at `real_room0(29903)`, whose Genesis/Aeon room identity is unresolved. Retain candidate row 46 pending range, handler, shop, and manifest ownership review. See the [Genesis audit](GENESIS_AUDIT_REVIEW.md#checkpoint-895-role-aware-aeon-and-external-dependency-closure), [candidate row 46](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L46), and [coverage row 396](ZONE_COVERAGE_REGISTER.csv#L397).


### Checkpoint 896: genocide_gh role identities remain source-owned

No connectivity totals change. The source map remains ten rooms (#29200-#29209), 19 direction records, 18 internal edges, one weak component, and all ten rooms directed-reachable from #29200. Its D4 to inactive Duris3 #157677 remains source-only. The seventeen directions that numerically target #29201-#29209 resolve to active Newbie room identities under the current manifest. The source-only D0 approach from #29200 to Newbie #29201 was counted previously; no active WLD ingress to a genocide_gh-only identity was found.

CP896 adds typed source context, not routes. External ZON matches occur only in active `newbie.zon` and unregistered `dark_gh.zon`/`eoo.zon`; their reset prototypes and room operands belong to those source identities. Five outside type-25 objects in `dark_gh`, `eoo_op`, and `eoo_op2` store targets #29202, #29206, or #29209. These are object destinations, not WLD edges, and their destination identity is conditional while duplicate maps are unresolved. Newbie QST/story values are giver, mobile, or item roles; the compiled #29201/#29202 room consumers and #29201 map object lookup remain separate code roles. See the [Genocide Guildhall audit](GENOCIDE_GH_AUDIT_REVIEW.md#checkpoint-896-role-aware-newbie-dark-guildhall-and-eoo-closure) and [24 builder questions](GENOCIDE_GH_BUILDER_QUESTIONS.md). Candidate row 47 remains **RETAIN IN PLACE**.


### Checkpoint 897: genocide_hall retains source-owned graph boundaries

The CP485 graph remains seven rooms, twelve directions, ten local edges, two disconnected components of four and three rooms, and two source-only D4 exits to inactive `ud_map2` #1124854 and Duris3 #144550. There is still no active WLD ingress into #29500-#29511 and no return edge from either target. CP897 found 28 additional authored WLD destination records to these numeric IDs: thirteen from unregistered `aod_gh`, twelve from unregistered `legion`, and three from unregistered `rf_op`. All three sources define their own same-number rooms, so none adds an active route or changes the candidate graph/counts.

Typed resets match only those same unregistered source maps: 25 role fields each in AoD and Legion and four RF Outpost room placements. Two Legion type-25 objects target #29503 and #29502; no external type-29 switch target matches. These are source-owned reset and object records, not WLD connectivity. AoD's shop uses its own keeper #29501 and room #29503; the external shop number is not a route. Keep candidate row 48 **RETAIN IN PLACE** until the map range, duplicate identities, handler, manifests, and shop references are resolved. See the [Genocide Hall audit](GENOCIDE_HALL_AUDIT_REVIEW.md#checkpoint-897-role-aware-closure-across-the-overlapping-map-band) and [24 builder questions](GENOCIDE_HALL_BUILDER_QUESTIONS.md).


### Checkpoint 898: gianthou retains its source-only map boundary

The CP486 map totals remain 271 rooms and 698 directions: 696 local directions, one negative target, and one positive outbound D3 from #42870 to #274141. The active WLD loader still resolves none of these targets; all rooms remain in one source component, with 20 fall-chance rooms and no currents. A full outside-source scan found no WLD edge into a `gianthou` room, no external portal/switch destination targeting the map, and no external ZON room operand to add another approach. No local type-25/type-29 object target adds a separate object route. These scans add no route or graph count. The #274141 target remains ambiguous between inactive `surf` and `surface2011`, neither with a direct return edge. Candidate row 49 remains **RETAIN IN PLACE**. See the [Gimluk's Cottage audit](GENIANTHOU_AUDIT_REVIEW.md#checkpoint-898-complete-typed-cross-source-closure) and [24 builder questions](GENIANTHOU_BUILDER_QUESTIONS.md).


### Checkpoint 899: guild_rooms remains SHP-only at an active Grave room

The candidate still has no WLD file, so it has no authored graph or candidate inbound/outbound edge count. Its SHP room field #8560 resolves to active Grave room “A Simple Stone Tomb.” Two Grave WLD directions target #8560 within Grave ([D3](../../../areas/wld/grave.wld#L3336), [D0](../../../areas/wld/grave.wld#L3374)); they remain part of the Grave map, not routes into a Guild Rooms component. The only outside ZON room operands matching the SHP location are Grave M resets for mobiles #8424 and #8401 in that same Grave room. The compiled #8556 transfer callsites also resolve to an active Grave room. No portal/switch destination or external shop field creates a new Guild Rooms route or a loaded shop. Keep candidate row 50 **RETAIN IN PLACE**. See the [Guild Rooms audit](GUILD_ROOMS_AUDIT_REVIEW.md#checkpoint-899-typed-shop-and-active-grave-boundary-closure) and [24 builder questions](GUILD_ROOMS_BUILDER_QUESTIONS.md).

### Checkpoint 900: Halfcut Hills reconciles source-only reciprocal route

The candidate's north exit from halfcut_hills #27001 to #256031 has a
numerically reciprocal source-only record: unregistered surf.wld #256031
exits south to #27001 ([destination line 1175762](../../../areas/wld/surf.wld#L1175762)).
A competing unregistered surface2011.wld #256031 definition exits south to
#256331, so endpoint identity remains unresolved. Neither source is listed in
areas/AREA; the reciprocal record does not create an active return or
candidate-specific ingress.

The surf direction already satisfies the Checkpoint 448 source-only recount
rule because #27001 is uniquely owned by active halfcut and #256031 is absent
from that active map. It is included in the existing 11,987 eligible records,
including 415 from surf.wld; do not increment those totals. The source-only
target resolves to active halfcut #27001 by numeric ownership. Active Surface
#629274 south to #27001 remains the only active cross-area inbound, while
candidate halfcut_hills remains unlisted. Candidate totals stay 550 exits,
549 local, one unresolved against the active map set, one weak component, and
226/235 directed-reachable from #27001. See the
[Halfcut audit](HALFCUT_HILLS_AUDIT_REVIEW.md#checkpoint-900-source-only-reciprocal-edge-and-typed-dependency-closure),
[24 builder questions](HALFCUT_HILLS_BUILDER_QUESTIONS.md), and
[candidate row 51](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L51).

### Checkpoint 901: Hostel role scans leave the graph unchanged

The Hostel graph remains 102 rooms / 242 directions, five weak components,
and zero candidate outbound edges. The ten active cross-area numeric targets
still resolve to Limbo #1 and Heavens #38, not Hostel. The only non-source-local
outside WLD record found with a target in this number band is unregistered `surf.wld`
#201670 D5 → #36; #36 is active Heavens. That source-only edge was already
included in the Checkpoint 448 recount (11,987 total / 415 Surf), so it adds
neither a Hostel ingress nor a new recount item. No outside WLD edge targets a
Hostel-only room.

The 25 candidate WLD key fields resolve to candidate OBJ records that also
have active Limbo/Heavens owners. External key-field matches are source-local
in Mavulsk or resolve to those current active owners; they do not describe
travel. The candidate defines no type-25 teleport or type-29 switch objects,
and no external type-25 destination or type-29 room target points into its
rooms. These role checks add no WLD route or connectivity count. See the
[Hostel typed audit](HOSTEL_AUDIT_REVIEW.md#checkpoint-901-typed-cross-source-closure),
[24 builder questions](HOSTEL_BUILDER_QUESTIONS.md), and
[candidate row 52](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L52).

### Checkpoint 902: IC3 has no new external ingress

The IC3 graph remains 54 rooms and 112 directions: 111 local targets plus the
source-only #420000 D0 north exit to active Rice Fields #77262. No outside WLD
target resolves into the #420000–#420053 candidate room range, and the current
active Rice Fields room's south destination remains `-1`; candidate-specific
active ingress and reverse pairs remain zero. The IC3 outbound direction is
already part of the Checkpoint 174 curated recount (3,425 overall, 192 Surf,
15 `ud_map2`), so there is no connectivity increment.

The all-source role scan found no external ZON, type-25 teleport destination,
type-29 switch room target, QST, SHP, or direct code route into IC3. The two
candidate teleports point to local room #420022 and do not add an external
edge. Keep the source-only Rice Fields boundary and the active #77262 removal
as separate route facts. See the [IC3 role review](IC3_AUDIT_REVIEW.md#checkpoint-902-typed-cross-source-role-refresh),
[24 builder questions](IC3_BUILDER_QUESTIONS.md), and
[candidate row 53](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L53).

### Checkpoint 903: Infernal numeric routes remain Harrow-owned

The Infernal graph remains 16 rooms and 29 directions: 26 local exits and
three one-way D4 exits to unlisted Duris3, with components of 10, 3, and 3
rooms. An outside-source recount found 102 WLD directions targeting the
candidate number band: 44 Harrow-local, 36 `dark`-local, 11 `darkop`-local,
and eight `sky_gh`-local. Two active Surface exits and one unregistered Surf
exit also target #29400; the active exits are the Surface–Harrow pairs, while
the Surf record is source-only. Those three non-source-local records resolve
to active Harrow #29400, not to the separate Infernal room. The other WLD
directions target rooms within their own map sources. No active Infernal
ingress or return to the three Duris3 targets exists.

Role-aware reset, QST, sidecar, SHP, portal/switch, and code references do not
add an Infernal route. In particular, Harrow's #29400/#29402 quest and story
items, #29403 inn binding, and keeper/room/product fields remain active Harrow
roles. The four candidate portals connect candidate components through stored
local destinations and are not WLD edges. No graph or candidate-ingress metric
changes. See the [Infernal typed audit](INFERNAL_GH_AUDIT_REVIEW.md#checkpoint-903-typed-cross-source-role-closure),
[30 builder questions](INFERNAL_GH_BUILDER_QUESTIONS.md), and
[candidate row 54](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L54).

### Checkpoint 911: Magetower numeric destinations remain source-owned

The CP500 graph remains 71 rooms and 152 direction records, one weak component,
and no active ingress to the unlisted Magetower identity. A role-aware active
WLD recount finds 70 directions targeting the #1–#71 numeric band: 60 are
Heavens-to-Heavens targets, nine Desert exits target Limbo #1, and one Thetis
direction targets Heavens #38. These are active Limbo/Heavens room identities,
not Magetower routes.

An all-WLD scan finds 869 unlisted direction records targeting the same band.
Dalvik (262), Hostel (176), Mavulsk (153), Mentiri (159), Nexus_1 (92),
Theurgist (6), and Waterplane (20) account for 868 source-local targets. The
remaining unlisted source is Surf #201670 to active Heavens #36. None adds
active Magetower ingress or changes the graph or curated connectivity totals.
The typed reset, quest, shop, and object-destination matches in Checkpoint 911
remain separate source roles and do not add WLD edges. See the
[Magetower role-aware audit](MAGETOWER_AUDIT_REVIEW.md#checkpoint-911-role-aware-low-vnum-source-closure).

### Checkpoint 912: map1 has no graph or room identity

`component:map1` still contains only the zero-byte MOB path and has no WLD,
room VNUMs, or authored exits. Graph metrics remain not applicable; active
candidate ingress remains zero because no candidate room identity exists.
The `MAP1_FILE` and `MAP1A_FILE` constants name `lib/information/` files and
do not add a world route. Keep the separate `ud_map1` graph and routes attached
to their own source identity; the map1 path adds no connectivity or curated
total.

### Checkpoint 913: map22 external room references leave the grid unchanged

The CP502 graph remains 10,000 rooms and 39,600 reciprocal local directions in
one 100-by-100 component, with zero active WLD ingress. An all-WLD scan found
no external room definition or direction target in #900000–#909999. The typed
room-role ZON scan, parser-valid N-format shop-room scan, QST giver/item scan,
story `room_vnum` scan, external OBJ type-25/type-29 destination scan, and
direct literal `real_room`/`real_room0` scan also found no candidate-room
reference. The type-1 link from Duris3 #145443 to #905050 remains conditional
generator behavior, not a WLD edge or current active route; it was not run.

A raw object-data search found four #900000 values: Avernus #32020 (type 10),
Dragonnia #6829 (type 20), Tradeskills #400283 (type 8), and Voluntown #142429
(type 8). These are ordinary OBJ fields, not room destinations. The separate
code-selected object #68 is active Heavens' Relic of the Spider Queen; unlisted
Hostel and Mavulsk also define #68. Hostel's three G resets and Mavulsk's one
G reset select their own source-local records. No QST or story item reference
or N-format shop stock entry for #68 was found. The competing object definitions
do not add room connectivity. See the [map22 role-aware audit](MAP22_AUDIT_REVIEW.md#checkpoint-913-role-aware-random-lab-dependency-closure).

### Checkpoint 914: map5 reset roles do not create a room graph

`map5.mob` still has no WLD rooms or authored exits, so graph metrics remain
not applicable and active candidate WLD ingress remains zero. The exact 40
map5 mobile VNUMs also occur as room definitions in unlisted `Duris3.wld`, one
room per number; this is a separate WLD namespace and map identity. Typed
ZON review found 45 source-only `M` placements and nine source-only `F`
followers for map5 mobiles in unlisted `surf.zon`. Their 40 placement rooms
belong to the separate `surf.wld` source, and `surf` is not a current area
contributor. No active reset or route metric changes. See the
[map5 role-aware audit](MAP5_AUDIT_REVIEW.md#checkpoint-914-map5-cross-namespace-role-closure).

### Checkpoint 915: map8 has no path or room identity to connect

The zero-byte `areas/mob/map8.mob` still defines no rooms, VNUMs, or exits.
The case-insensitive `map8`/`map_8`/`map-8` and exact-path scans across
`areas/`, `src/`, `scripts/`, and `tests/` found no in-checkout references.
Connectivity metrics remain not applicable and candidate inbound active WLD
exits remain zero because there is no room identity. The scan adds no route
or graph change. See the [map8 path-reference audit](MAP8_AUDIT_REVIEW.md#checkpoint-915-exact-path-reference-refresh).

### Checkpoint 916: Mara Guildhall source-role scan adds no active boundary

The exact #29600–#29617 WLD scan remains confined to unlisted `mara_gh.wld`
and `mara_op.wld`; all Mara Guildhall graph metrics remain 16 rooms, 29
directions, and components of 10/3/3, with three source-only exits to
unlisted Duris3 and zero active WLD ingress. No additional WLD source or
current active area-list input names a candidate room. Source-only item
teleports, reset commands, shop bindings, and the #29605 inn assignment remain
typed dependencies outside WLD edge totals. See the
[Mara Guildhall role-aware audit](MARA_GH_AUDIT_REVIEW.md#checkpoint-916-exact-vnum-role-closure-refresh).

### Checkpoint 917: Mara Outpost room scan adds no active route

The exact #29610–#29612 WLD scan remains within unlisted `mara_op.wld` and
`mara_gh.wld`. Mara Outpost retains three rooms, five directions, four local
directions in one component, and one up exit from #29610 to unlisted Duris3
#124408. The matching Mara Guildhall WLD exit is a second authored source
record; no active WLD source or Duris3 return targets Mara Outpost. The
#29610 ITEM_TELEPORT value to Mara Guildhall #29608 remains outside WLD edge
and active-inbound metrics. See the
[Mara Outpost role-aware audit](MARA_OP_AUDIT_REVIEW.md#checkpoint-917-mara-outpost-exact-vnum-role-closure).

### Checkpoint 918: Mavulsk object destinations do not change WLD metrics

Mavulsk remains at 233 rooms and 518 authored WLD directions, with the
existing single active WLD inbound record from Thetis #38095 north → #104.
The exact ZON room-role scan found 83 active command arguments in the
#1–#233 band, all paired with same-stem active WLD rooms. Seventeen active
ITEM_TELEPORT `value[0]` fields instead target #2, #9, #50, #100, #150, or
#200, none of which has an active WLD definition. Each number occurs in
Mavulsk and at least one other unlisted WLD, so the object fields do not
identify their intended destination owner. They are typed object values, not
WLD edges or active ingress. The Thetis #104 ambiguity and Mavulsk #188 east
→ Surf #228202 remain unchanged. See the
[Mavulsk role-aware audit](MAVULSK_AUDIT_REVIEW.md#checkpoint-918-mavulsk-typed-cross-source-role-closure).

### Checkpoint 919: Mentiri source-only targets do not add active ingress

The 72-room, 160-direction Mentiri graph remains fully local, with one weak
component and raw directed reach of 72/72 from #1. The all-source sweep found
939 other-WLD target records in the candidate #1–#72 band: 70 active records
all resolve to active Limbo/Heavens rooms; 869 unlisted records include 868
targets local to their own source and the already-counted Surf #201670 D5 →
#36 source-only edge. That Surf target resolves to active Heavens #36. The
candidate-specific active inbound count remains zero. Six active
ITEM_TELEPORT values to #2, #9, or #50 have no active WLD destination and
remain object-field references, not WLD edges. See the
[Mentiri exact role audit](MENTIRI_AUDIT_REVIEW.md#checkpoint-919-mentiri-exact-cross-source-role-closure).

### Checkpoint 920: Monsteri exact room band adds no active ingress

The all-source scan found eight other-file WLD targets in Monsteri’s
#1000–#1004 room band; all eight are local to unlisted cbl_outpost2. No active
WLD target points to the band, no active WLD definition owns it, and the
candidate inbound count remains zero. Active ZON room-role references and
active ITEM_TELEPORT `value[0]` fields also add no room target. Menden’s
active ZON and shop use of #1000 is typed as an object reference to current
Heavens OBJ #1000, not as a Monsteri room edge. See the
[Monsteri exact role audit](MONSTERI_AUDIT_REVIEW.md#checkpoint-920-monsteri-exact-cross-source-role-closure).

### Checkpoint 921: moriatst shop rooms are not WLD edges

The three `moriatst` `in_room` values #99306, #99312, and #99318 resolve by
room role to active Moria WLD rooms. Active Moria ZON resets place the
matching keeper mobiles there. These shop-placement fields do not create a
candidate WLD graph or active WLD inbound; the candidate remains SHP-only
with zero active WLD inbounds. See the
[moriatst exact role audit](MORIATST_AUDIT_REVIEW.md#checkpoint-921-moria-role-bindings-and-shop-factor-correction).

### Checkpoint 922: Mountain QST values overlap map rooms by number

The Mountain QST remains QST-only: it contributes no WLD rooms and still has
zero active WLD inbounds. CP922 found that all five quester numbers and all
seven unresolved item numbers also identify rooms in the active mountaintracks
WLD. Those are room VNUMs in mountaintracks, while the QST headers are mobile
VNUMs and its I goals are object VNUMs. mountaintracks ZON commands place
different mobile numbers in several of those rooms; the matching room values
do not connect the QST to the map or resolve its typed references. Existing
candidate graph and inbound-edge counts do not change. See the
[Mountain QST role audit](MOUNTAIN_QST_AUDIT_REVIEW.md#checkpoint-922-typed-collision-with-active-mountaintracks-rooms).

### Checkpoint 923: mtncastl number #1 references resolve to Limbo

The mtncastl WLD remains a singleton with no exits. Across current WLD source,
23 directions target numeric #1: nine active Desert directions from #49790
resolve to active Limbo, and fourteen directions from eight unlisted maps
remain source-only. Four ZON room-field commands also use #1, all in
unlisted Magetower/Mavulsk sources; none belongs to mtncastl.zon. The
candidate-specific active inbound count remains zero.

The source-role scan found no #1 room destination in SHP in_room fields,
ITEM_TELEPORT or ITEM_SWITCH destinations, structured story/map sidecars, or
compiled room binding. The direct real_room(1) calls and
ROOM_LIMBO_VNUM references are generic Limbo paths. These numeric records do
not add an active route to mtncastl or change the active graph. See the
[mtncastl exact role audit](MTNCASTL_AUDIT_REVIEW.md#checkpoint-923-exact-room-1-dependency-and-role-closure).

## Checkpoint 924: Nexus56 has no typed inbound to candidate-only rooms

Nexus56 defines 760 WLD rooms. Current Alatorin/Delwyn sources own 666 of
those numbers; the remaining 94 have no active WLD owner. The current-source
room-role scan found no ZON room field, SHP `in_room`, ITEM_TELEPORT
destination, ITEM_SWITCH room, story/map JSON number, or fixed code room
literal aimed at those 94 rooms. The 1,169 active ZON room fields, 11 active
shop placements, 21 active teleport destinations, and 11 active switch room
fields in the shared-number range resolve to existing active room IDs. They
do not add an active WLD inbound or identify the source-only Nexus56 rooms.

The active Alatorin SHP record #83343 uses #83343 as its shop ID and keeper;
its `in_room` field is #83681. This is a shop/mobile-role match to
Nexus56's candidate-only room number #83343, not a room placement or graph
edge. See the [Nexus56 role audit](NEXUS56_AUDIT_REVIEW.md#checkpoint-924-exact-room-role-endpoints-and-shop-fallback-bindings).

The candidate's own SHP records place 18 shops at same-number Alatorin rooms
and one at candidate-only #83076; its candidate chair/switch #83148 points
to Nexus56 room #83638, whose current active owner is Alatorin. These are
typed room placements, not WLD edges. No graph, active-inbound, or global
source-only approach total changes. See the
[Nexus56 typed room-role audit](NEXUS56_AUDIT_REVIEW.md#checkpoint-924-exact-room-role-endpoints-and-shop-fallback-bindings).

## Checkpoint 925: Player Guilds reset roles do not add room edges

`player_guilds` has no WLD graph. The typed ZON follow-up found 40 mobile
reset rows using four of its shop keeper VNUMs, while no such row places a
matching keeper in its Player Guilds `in_room`. Five shop room numbers do
have ZON mobile placements, but they target other mobile VNUMs; the rows are
population commands, not WLD directions. Three missing stock-object VNUMs
also occur in object reset fields. These mobile, object, and room references
do not create WLD edges, so the candidate-specific active WLD inbound count
remains zero and no connectivity total changes. See the
[Player Guilds typed reset audit](PLAYER_GUILDS_AUDIT_REVIEW.md#checkpoint-925-typed-reset-and-numeric-role-closure).

## Checkpoint 926: Ravenmis shared room numbers remain Newbie rooms

All ten Ravenmis room VNUMs are also owned by active Newbie. Nine active
WLD exits target those numbers, all from `newbie.wld`, and resolve to Newbie
rooms in the current room table. Six ITEM_TELEPORT objects also point into
that number range; two are Newbie objects and four are unlisted guildhall or
portal sources. Their destinations resolve to Newbie rooms and do not add
WLD exit edges or active inbound routes to the Ravenmis source identity.
The active-WLD inbound count to Ravenmis remains zero. See the
[Ravenmis typed role audit](RAVENMIS_AUDIT_REVIEW.md#checkpoint-926-shared-room-numbers-resolve-to-newbie-identities).

## Checkpoint 927: Raxquest's overlapping VNUM types add no WLD edges

Raxquest's 84 WLD rooms, 63 mobiles, and 98 objects reuse 62 numbers across
all three types. The room procedure at #70501 is type-qualified as a room;
the same number also names a mobile and object. Current scans found no
standalone 705xx reference outside the five Raxquest files under `areas/`,
and no Raxquest ITEM_TELEPORT or ITEM_SWITCH prototype. The dedicated
Raxquest code's mobile references and `real_*0` bindings do not add WLD
directions or an active inbound route. The existing 84-room / 175-exit graph,
one unregistered Duris3 outlet, and zero active WLD inbounds remain unchanged.
See the [Raxquest role audit](RAXQUEST_AUDIT_REVIEW.md#checkpoint-927-cross-type-ids-keep-separate-source-and-code-roles).

## Checkpoint 928: realms_master shop rooms and board objects add no WLD edges

The two `realms_master.shp` room placements are #11121 and #11124, neither
of which has a current WLD definition. Both numbers also identify
Player Castles OBJ bulletin boards; the object identity does not resolve a
room or create an edge. Five source ZON O rows for OBJ #11108/#11109 likewise
remain object resets, not exits or shop placements. The only code references
to #11108/#11121 as rooms sit inside the disabled `#if 0` Realms Master
block. No active WLD target, room, or inbound route was found for these
numbers. The candidate remains SHP-only with no connectivity totals. See the
[Realms Master role audit](REALMS_MASTER_AUDIT_REVIEW.md#checkpoint-928-shop-keeper-room-and-board-numbers-remain-typed).

## Checkpoint 929: returns remains a two-mobile source without a map

The exact #93400/#93401 scan found no current WLD room, target, or exit
outside `returns.mob`, and no external typed source reference. `returns` has
no WLD file or candidate rooms, so no graph or connectivity count changes;
its candidate-specific active WLD inbound count remains zero. The absence
of map edges does not establish that external or dynamic spawn inputs are
absent. See the
[returns role audit](RETURNS_AUDIT_REVIEW.md#checkpoint-929-both-mobile-vnums-remain-isolated-in-current-sources).

## Checkpoint 930: rf_gh teleport destinations stay outside WLD exit counts

The source ZON places ITEM_TELEPORT #28903 in WLD room #28905 with
`value[0]` #28912, and #28904 in #28912 with `value[0]` #28905. These form
a reciprocal object-destination pair between the two WLD-only groups. They
do not add WLD exit records: the WLD graph remains at 24 authored directions
and two weak components, with its existing external outlets and zero active
inbound to the absent rf_gh identity. If the data is loaded, the portal
lookups would still depend on the colliding Torg room/object identities; no
usable or active route is inferred. See the
[rf_gh portal audit](RF_GH_AUDIT_REVIEW.md#checkpoint-930-reciprocal-portal-objects-cross-the-two-wld-groups).

## Checkpoint 931: rf_op typed destinations do not add WLD edges

The #29510–#29512 number scan found WLD, MOB, and OBJ roles sharing IDs
inside `rf_op`, and separate same-number WLD rooms in unlisted
`aod_gh`, `genocide_hall`, and `legion`. Legion OBJ #29500 also stores
#29512 as a type-25 destination, alongside Legion's own WLD room #29512.
That object field is not a WLD direction or an active inbound route to
`rf_op`. Its five authored WLD directions, zero loader-resolved targets,
one weak component, one #154153 outlet, and zero active inbound exits remain
unchanged. See the
[rf_op typed-role audit](RF_OP_AUDIT_REVIEW.md#checkpoint-931-same-number-records-keep-separate-typed-roles).

## Checkpoint 932: scrubs_hall's shared item reference adds no WLD edge

OBJ #6089 has no checked-in full or mini OBJ prototype, but the exact value
appears in SHP stock and ZON G-reset fields across many source files,
including listed cbl_hall, Player Castles, and Ashrumite sources. Lizard's
active WLD room #6089 is a separate type. These shop and inventory fields
do not add WLD exits or change scrubs_hall's 19-exit graph, one weak
component, Duris3 outlet, or zero active inbound exits. See the
[scrubs_hall #6089 audit](SCRUBS_HALL_AUDIT_REVIEW.md#checkpoint-932-product-6089-is-a-shared-unresolved-source-reference).

## Checkpoint 933: Shadow Hall object imports add no WLD routes

Shadow Hall's two Player Castles O resets, and three same-object resets in
unlisted `bastard` and `sky_gh`, remain object placements rather than WLD
edges. The #11060 magic-mouth code binding and #11110 mobile-helper arrays
also do not add a route to the Shadow Law room identities. The five-exit,
one-component graph, Duris3 outlet #151052, and zero active inbound remain
unchanged. See the
[shadow_hall import audit](SHADOW_HALL_AUDIT_REVIEW.md#checkpoint-933-player-castles-imports-have-separate-code-consumers).

### Checkpoint 934: shady2 item references add no map ingress

The Shady QST/story item IDs #97514, #97570, #97572, and #97582 resolve
to active Shady OBJ prototypes. Alatorin's QST and Brino story contract use
item #97549 for the active orcish-forged sword OBJ; the same number also
names active Shady and unlisted `shady2` baby-orc MOB records. These item
references overlap the candidate mobile range but are not mobile references
or WLD edges. `shady2` remains WLD-less, with no authored exits or active
map ingress. Its graph contribution and zero active inbound exit count are
unchanged. See the [typed-reference audit](SHADY2_AUDIT_REVIEW.md#checkpoint-934-quest-and-story-item-ids-remain-object-typed) and [Shady Grove supplement](SHADY_GROVE_AUDIT_REVIEW.md#checkpoint-934-97549-is-an-object-and-mobile-vnum).

### Checkpoint 935: Harrow story roles add no sky_gh ingress

Harrow story `mob_vnum: 29404` resolves to its active Lomya MOB. Harrow
QST/story values #29400 and #29402 are item inputs, and #29404 is an OBJ
reward; the same number is also an active Harrow WLD room. `sky_gh` uses
#29404 as its Armory room and unlisted SHP location, which current loader
lookup resolves to Harrow's room if the shop is read. These typed identity
roles add no WLD edge or sky_gh-identity inbound exit. The five-room,
nine-exit graph, Duris3 #153760 outlet, and zero active candidate-identity
inbounds remain unchanged. See the [sky_gh typed-role audit](SKY_GH_AUDIT_REVIEW.md#checkpoint-935-harrow-contact-and-item-roles-stay-separate).

### Checkpoint 936: source-only QST overlaps add no Surf routes

Unlisted `surface2011.qst` contains 37 headers and 56 item operands in the
Surf mobile range #200000–#200321; unlisted `underdark2011.qst` contains
one header and four item operands in the range. Neither QST file is in
`areas/qst/AREA`; their item fields resolve to their own source-only OBJ
records, not to `surf.mob`. These typed records add no WLD edge or
candidate-specific active ingress. The full graph remains 359,659 authored
exits, 145 ambiguous active inbound matches, and 431 external source exits.
See the [Surf type-boundary audit](SURF_AUDIT_REVIEW.md#checkpoint-936-old-surface-qst-values-do-not-identify-surf-mobiles), the [Surface2011 reciprocal review](SURFACE2011_AUDIT_REVIEW.md#checkpoint-936-surf-range-qst-values-retain-their-source-owned-types), and the [UnderDark2011 reciprocal review](UNDERDARK2011_AUDIT_REVIEW.md#checkpoint-936-underdark2011-qst-values-retain-their-source-owned-types).

### Checkpoint 943: Marigot code destinations sit outside the WLD graph

The repository source scan finds compiled destinations into active Marigot identities: Chaos Port names room #5302, `HOME_MARIGOT` maps to #5302, the Fort Marigot justice table names local rooms/mobile, and `guild_locations` contains Fort Marigot guild rooms. These are fixed code/table references rather than authored WLD exits. They add no room-edge records and do not change the active WLD graph or the already-counted source-only Surf and Underworld2 approaches. Disabled `#if 0` assignments and the unbound `ticket_taker` table remain dormant code mentions, not active routes. The compiled source does not establish live invocation, player eligibility, access, or safe return. See the [Marigot source review](MARIGOT_STORY_REVIEW.md#checkpoint-943-bounded-typed-and-compiled-code-closure) and [coverage row 171](ZONE_COVERAGE_REGISTER.csv#L171).

### Checkpoint 944: Orogs code bindings remain outside the WLD edge count

The compiled `HOME_OROGS` mapping targets room #45906, and the Orog guild table contains #45905/#45906. Fixed `rentacleric` and `money_changer` assignments bind to mobiles #45818 and #45833; these service bindings are not authored WLD exits. No external type-25/type-29 OBJ destination targets an Orog room. The four reciprocal Underdark WLD pairs remain the route evidence; no authored edge or graph count changed. Class specialization markers, service invocation, door/reset state, and live access or safe return remain outside this static graph result. See the [Orogs source review](OROGS_STORY_REVIEW.md#checkpoint-944-typed-source-and-fixed-code-closure) and [coverage row 224](ZONE_COVERAGE_REGISTER.csv#L224).

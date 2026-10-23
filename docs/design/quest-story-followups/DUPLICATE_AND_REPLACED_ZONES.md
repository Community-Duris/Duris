# Duplicate and replaced zone history

**Checkpoint 24 update to the checkpoint 2 title/history review.** Current
title groups, one loaded room-ID collision, manifest-backed retirements, the
Mistywood/Kilfray QST lineage question, and Darkfall's Surface source history
are recorded here. This is history and source analysis only; no zone was
merged, renamed, or moved.

## Current active-name overlaps

Titles below were compared after removing Duris color tokens and folding case.
The same display title is not sufficient evidence that a zone is obsolete or
that its quests should be merged.

| Normalized title | Current source keys | Current reading |
| --- | --- | --- |
| Braddistock Mansion | `braddistock`, `brad` | Separate active source keys with the same title. Keep zone identity and native contract ownership separate until builders clarify player-facing disambiguation; the [Mansion audit supplement](BRADDISTOCK_MANSION_AUDIT_REVIEW.md) records their distinct current routes and tracked path history. The shared title is not replacement evidence. |
| Ceothia | `ceothia`, `ceopast`, `ceofutur` | Three active phases carrying one title. Preserve their separate histories and contracts; the [Ceothia audit supplement](CEOTHIA_AUDIT_REVIEW.md) records bounded cross-phase references, and the [Past Ceothia supplement](PAST_CEOTHIA_AUDIT_REVIEW.md) records its no-edge WLD scan and tracked key history. The repeated title is not replacement evidence. |
| The Underworld | `underworld`, `underworld3` | Two active sources with the same title. Treat the `3` key as a possible continuation label, not proof of duplicated or superseded content. |

The full per-key crosswalk is in the
[`ZONE_COVERAGE_REGISTER.csv`](ZONE_COVERAGE_REGISTER.csv). Preserve each
source area's key in logs, paths, and any future display disambiguation. The
builders should decide whether these names are intentionally identical,
phase-specific, or need distinct presentation labels.

## Confirmed active source collision: VNUM 51800

Two active WLD files each declare `#51800`: the last Dis-side room in
[`hell_two_dis.wld`](../../../areas/wld/hell_two_dis.wld#L10375) and the first
Minauros-side room in
[`hell_three_minauros.wld`](../../../areas/wld/hell_three_minauros.wld#L1).
The corresponding `.zon` tops are 51800 and 52199. The sorted loader ranges
therefore assign both records to the Dis range. `make_wld` concatenates both
files in `areas/AREA` order, and boot assigns room ownership from numeric zone
tops; the `real_room0` search has no policy that chooses one of two equal
VNUMs ([`make_wld.c`](../../../areas/src/wld/make_wld.c#L49),
[`db.c`](../../../src/world/db.c#L1321),
[`db.c`](../../../src/world/db.c#L8480)). Ten exit records point at 51800.
The exact inbound and outgoing links are enumerated in
[`WORLD_CONNECTIVITY.md`](WORLD_CONNECTIVITY.md).

**Decision needed before a repair:** name the intended room/VNUM and numeric
range owner. A safe fix would require a complete reference inventory for both
source files plus reset, quest, object, special-procedure, and persisted VNUM
references. Do not resolve this by deleting either zone or moving it to an
archive.

## Source-backed retirement and replacement history

### Githyanki Fortress

Commit `9004f09cb` (2010-07-31) added the `githyanki_fortress` WLD, MOB, OBJ,
QST, and ZON files and described them as “the replacement for old gith
fort.” The message also says the new source had not yet been added to
`areas/AREA`. Commit `cc7d194ae` (2010-08-04) changed the commented `*lortower
*1320` entry to active `githyanki_fortress *1320`; `93e77d517` (2010-09-08)
is titled “Readding gith fort” and adds the key to the list again.

The pre-addition tree already included the active `gith_ht` Hometown, whose
rooms describe streets within a Githyanki fortress. A second plausible
predecessor is the fortress group in active `astral_areas`: its 39 rooms
`#19828`–`#19866` describe another Githyanki fortress, and its one-way exit
from `#19828` leads to Astral Main `#19798`. Astral Main also has a separate
route from `#19798` to the newer fortress at `#132000`; that adjacency does
not establish that the newer map replaced the side-area fortress. The tracked
history does not name either source as the “old gith fort,” and no retirement
of the side-area group is recorded. The current `areas/AREA` continues to list
`gith_ht` at zone `194`, `githyanki_fortress` at zone `1320`, and
`astral_areas` at zone `198`, with separate source ranges. Record the author's
replacement claim and the side-area fortress as a likely but unconfirmed
candidate; leave `gith_ht` distinct and do not archive or move any of the three
active areas
([Fortress review](GITHYANKI_FORTRESS_STORY_REVIEW.md),
[Hometown review](GITH_HT_STORY_REVIEW.md),
[Astral Side Areas review](ASTRAL_AREAS_AUDIT_REVIEW.md),
[Hometown entry](../../../areas/AREA#L97),
[Fortress entry](../../../areas/AREA#L380),
[Astral Side Areas title](../../../areas/zon/astral_areas.zon#L2),
[Fortress boundary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L155),
[Hometown boundary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L154)).

### Brass versions

Commit `0e925fa992ecb648b260efa159bd016fa88f9136` (2025-12-18, “removed old
versions of Brass and renamed files to avoid confusion”) separates active
`brass` from three legacy families. It renamed `New_Brass_2012` files to
`brass-old-1`, added `brass-old-2`, and renamed `plane_fire_two` files to
`brass-old-3`; the old `plane_fire_three` world files were deleted and its QST
was renamed to `plane_fire_one`. The current main `areas/AREA` contains active
`brass` and `plane_fire_one`, not the old families. Current `brass-old-*`
components are retained in the source-only portion of the register.

The two old-list names `plane_fire_two` and `plane_fire_three` remain in
`areas/qst/AREA` without current files. Active `potions` and `realm` exits
reference room 25087, which is present only in `brass-old-2`; the normal loader
does not load that file. Keep that route ambiguity explicit while reviewing
the retired QST index and any future archive plan.

### Player ships

Commit `857124f14c648b26031a2a40c6335e846bb94b9d` (2016-08-15) removed the
`player_ships` zone. Its two WLD rooms were Davy Jones Locker #50000 and
“Speeding Across The Ocean” #50001; the commit reassigned those room functions
to existing active `ship.wld` rooms #60000 and #60001 and moved the ship-system
room constants and pool to the 60000 range. The old MOB and OBJ files were
empty and its ZON had no reset commands. There are no current `player_ships`
component files, but `areas/qst/AREA` still contains `player_ships *500-509`
with the note “For creating ships on the fly.” The QST index owner decision is
recorded as question 21 in the [Ship area audit](SHIP_AREA_AUDIT_REVIEW.md).
Preserve that manifest-only evidence and do not recreate the retired zone when
reviewing vehicle or ship behavior.

### IC3, Asunwa, and Jungle

Commit `ef7e3f9b17f492a946425657f1bce8e91e241506` (2026-04-06) removed `IC3`
from the active full-area list. Its uppercase `IC3.wld`, `.mob`, `.obj`, and
`.zon` source components remain in the checkout outside the list. Commit
`c0cf55b85e00b7e01411848a03d6f7d47b32b1f0` (2026-04-16) comments `asunwa`
and `jungle` out of `areas/AREA`; their source files remain, and both names
remain in the separate `areas/qst/AREA`. Asunwa still has QST and shop files;
Jungle has no current QST file. These facts establish disabled list membership,
not that the files are safe to archive or that another zone replaced them.

### Siege

Commit `7ddf5432f53fdc9b9c578d7503f4611529b544ef` (2026-08-31) retired
`siege`, deleted its WLD/MOB/OBJ/ZON source files, removed ten regional
quartermaster resets, and reserved zone number 4010 in `areas/AREA` with
`*RETIRED 4010 (siege; do not reuse)`. The parent WLD held only the empty
one-room “The Siege Room” #401000; its ten quartermasters were prototypes in
other towns, not a replacement map. No current source or successor zone owns
the `siege` stem. A later 2026 commit removed the dormant runtime and
persistence implementation. See the [Siege historical audit](SIEGE_AUDIT_REVIEW.md)
for the exact old reset callers, Charing's partial Quartermaster move, and the
reserved-ID recommendation. Keep 4010 reserved; do not restore Siege to the
active source list.

### Previously retired attic tree

Commit `3cb3646e0b2fd1234db0d1148f9a267ed71a1613` (2026-08-27) removed the
retired attic tree and stale runtime archives. Its commit message records that
the tree was not referenced by server/build/tests and that Git history retained
it. The current checkout has no `attic/` directory. This audit does not inspect
private/archive contents or restore deleted material; any new archive plan must
start only from current source files and preserve their history.

## History-only register entries

The coverage register includes four explicit rows without current source
files: `plane_fire_two`, `plane_fire_three`, `player_ships`, and `siege`. The
first three reconcile stale `areas/qst/AREA` entries against the current
replacement history; Siege records the reserved retired number. This keeps
them visible without counting them as current zones or current source stems.

### Darkfall Forest and Surface map lineage

Darkfall is the active `area:darkfall` contributor at zone 953. Its normalized
title `Darkfall Forest` is unique among the current active area titles. The
path history reviewed for its WLD, QST, and ZON files does not identify an
explicit duplicate or replacement area.

Commit `71d731c5b` (2013-11-13, “stuff”) changes Darkfall's west boundary
target from #214947 to the current Surface room #515272 and its east boundary
from #215551 to #515679. The same commit revises Darkfall resets and removes
three old ambient QST blocks for mobiles #95305–#95307 while retaining
Grellinar's native contract. This records a source/topology update, not a
replacement claim.

Current unregistered `surf.wld` rooms #248948 and #249568 point east/west into
active Darkfall rooms #95300 and #95348. The same-numbered records in
unregistered `surface2011.wld` point to #248949 and #249567 instead. The
parallel bodies and current-list exclusion do not establish either map family
as a Darkfall predecessor or complete replacement. The exact two raw links and
the paired active Surface exits are reconciled in the
[Darkfall supplement](DARKFALL_AUDIT_REVIEW.md).

**Builder question:** Do the two old `surf.wld` exits preserve intentional
Surface-to-Darkfall leads, or are they stale map-grid links from an alternate
Surface draft? **Recommendation:** retain both unregistered map families and
Darkfall in place until the map owner identifies their provenance. Keep the
active Surface boundaries authoritative for current static connectivity; do
not promote source-only links to active routes or use this evidence to move a
zone.

### Mistywood and the source-only Kilfray QST

Mistywood is active in `areas/AREA`; the current unregistered `kilfray`
component contains only `areas/qst/kilfray.qst` and is absent from both
`areas/AREA` and `areas/qst/AREA`. Its five giver VNUMs and three native Q
contracts parallel active Mistywood, with dialogue/format differences and two
reward discrepancies: Kilfray omits Mistywood Riliatar's 25,000 experience
term and Jarnes's 10,000 experience term. The normal QST generator reads the
top-level area list, so the Kilfray QST is not part of the generated production
catalog.

Git path history shows the 2009 area-tree move and 2013 generic import for both
files; it does not identify Kilfray as a formal Mistywood predecessor,
replacement, or duplicate. The 2011 Mistywood WLD edit is spelling-only.
Preserve Kilfray as a source-only, retained component until its author/tooling
lineage is resolved; similarity alone is not a basis to add it to the active list or
move/delete it. Exact QST comparisons, source-only Surface links, and builder
questions are in the [Mistywood audit supplement](MISTYWOOD_AUDIT_REVIEW.md).

### Kimordril and parallel map endpoints

Kimordril's normalized active title is unique in the current active list.
Tracked commit `71d731c5b` (2013-11-13) changes its room #95502 west target
from #214592 to active Surface room #543256. The commit records a one-line WLD
data change and a file-mode change; it does not claim that Kimordril replaces
another zone.

The old endpoint VNUM #214592 has different room text in the unregistered
`surf.wld`, `surface2011.wld`, and `underdark2011.wld` map sources. The same
three unregistered files also declare rooms #216660 and #216664, but only
`surf.wld` points those exits into active Kimordril; the other two maps point
to #216661 and #216663. This evidence describes parallel map records, not a
replacement lineage. Exact exits, room bodies, and the three reciprocal
current Surface boundaries appear in the
[Kimordril supplement](KIMORDRIL_AUDIT_REVIEW.md).

**Builder question:** Which, if any, parallel map family preserves the
historical Kimordril boundary context? **Recommendation:** retain all
unregistered map sources and active Kimordril in place until their provenance
is established; do not infer replacement from numeric overlap.

### Bugentol and parallel Surface map records

Bugentol's normalized active title, Bugentolen, is unique. Tracked commit
28c8b2ab5 (2011-05-12) changes room 95800's east target from active Surface
room 589071 to 259558; commit 71d731c5b (2013-11-13) restores the active
Surface target 589071. The old 259558 number has divergent descriptions in
unregistered surf.wld and surface2011.wld. Separately, surf.wld room 280727
points west into active Bugentol room 95800, while the same-numbered
surface2011.wld room is stormy sea and points west to 280726. These parallel
map bodies and the boundary edit do not establish Bugentol as a replacement or
predecessor. See the [Bugentolen review](../zone-stories/BUGENTOLEN.md), the
[Checkpoint 197 audit reconciliation](BUGENTOLEN_AUDIT_REVIEW.md), and its
linked exact source records.

### Jotunheim and its non-WLD Astral route

The normalized active title Jotunheim has one owner: area Jotun, zone 960.
The tracked local file history records the areas-tree move and generic source
updates, with no manifest entry or reviewed commit identifying a distinct
predecessor or replacement. Jotunheim's one dropped WLD edge targets negative
VNUM -1; its Astral connection is authored through portal objects and does not
establish a second zone or a replacement. The [Jotunheim audit supplement](JOTUN_AUDIT_REVIEW.md)
records the title and history boundary, the exact negative exit, and the portal
context from the existing comprehensive dossier.

### Ixarkon and the Faang connection

Ixarkon's normalized active title has one owner, area Ixarkon at zone 964.
Commit 0643ece38 (2010-07-10) says it connected the new Faang portion for
Pillithid to an exit in Ixarkon and adds one north exit from room 96474 to
Faang room 15412. This is a specific connection-history note, not a claim that
Faang replaced Ixarkon or that the two areas are duplicates. The reviewed
manifest and local path history name no distinct Ixarkon predecessor. The
[Ixarkon audit supplement](IXARKON_AUDIT_REVIEW.md) records the title check,
the two source-only ud_map2 inbounds, and the limits on the replacement
conclusion. A case-insensitive tracked-path search found no zone or area source
named Pillithid. Related Ixxillikor and Illithid Arena sources have distinct
titles and prototype ranges; the reviewed history provides no evidence that
either is an Ixarkon predecessor.

### Dirk’nspire Stronghold

The normalized active title “Dirk’nspire Stronghold” has one owner, area
`dirkn` at zone 968. The tracked WLD/ZON paths reach a 2009 area-directory
move; later retained changes are WLD spelling corrections, a 2013 file-mode
change, and a 2015 zone editor-header update. They name no distinct predecessor
or replacement. This bounded path-history result does not settle broader
semantic similarities. The [Dirk’nspire audit supplement](DIRKN_AUDIT_REVIEW.md)
records the title check, its reciprocal Valinhav boundary, and the external
typed references without making a reachability claim.

### Troll Caves

The normalized active title “Troll Caves” has one owner, area `troll_caves` at
zone 969. CP401 reviewed tracked WLD, MOB, OBJ, ZON, and QST histories on the
same area stem; the story sidecar first appears in the October 4, 2026
documentation/source update. No distinct predecessor or replacement is named
in these paths. The current “The Troll Hills,” “Troll Slave Compound,” and
“The Lair of the Swamp Troll King” titles have separate area owners and do not
establish a predecessor/replacement link to Troll Caves. This is a bounded
title/history finding, not a universal semantic-equivalence claim. The
[Troll Caves audit supplement](TROLL_CAVES_AUDIT_REVIEW.md) records the source
history, three source-only WLD inbounds, and external supply/teleport
references without treating them as active-world ingress.

### IceCrag Castle

The normalized active title “Ice Crag Castle” has one owner, area `icecrag`,
zones 970–972. Its tracked WLD/ZON history remains on the same stem and names
no distinct predecessor or replacement. The title is distinct from `Ice Tower`
and the other Castle-titled contributors. `Voluntown`, titled “The Royal
Mausoleum of Castle IceCrag,” is the related neighboring contributor with its
own rooms and source dossier; its reciprocal boundary with IceCrag records a
connection, not replacement or merged ownership. The [IceCrag audit
supplement](ICECRAG_AUDIT_REVIEW.md) records the semantic comparison, two
source-only WLD inbounds, typed external references, and bounded history result.

### Father Tel's Holy Cloister

The normalized active title “Father Tel's Holy Cloister” has one owner, area
`cloister` at zone 671. The reviewed tracked WLD/ZON history and active
manifest name no distinct predecessor or replacement. Checkpoint 711's
bounded comparison with the source-only “HOLY CLOISTER” node in Underworld2
and Father Tel references in other quest text finds no competing active area
or whole-zone predecessor. See [Checkpoint 711](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-711-cloister-title-and-source-only-name-comparison).
The [Cloister audit supplement](CLOISTER_AUDIT_REVIEW.md)
records two source-only WLD inbounds, the active reciprocal Surface and
Stoutdorf boundaries, the typed external references, and the bounded
history result.

### The Ruins of Turolopolis

The normalized active title “The Ruins of Turolopolis” has one owner, area
`willem` at zone 71. The reviewed tracked WLD/ZON history records the 2009
area-path relocation and continued edits, with no distinct predecessor or
replacement named. This title/path finding does not resolve broader semantic
similarities. The [Turolopolis audit supplement](TUROLOPOLIS_AUDIT_REVIEW.md)
records one source-only WLD inbound, the Surface and Zoo boundaries, a separate
Githzerai `shift_prime` destination, the bounded typed references, and the
history result.

### Du'Maathe Castle

The normalized active title “Du'Maathe Castle” has one owner, area `mntcastl`
at zone 371. The reviewed tracked WLD/ZON history shows continued edits and no
distinct predecessor or replacement named. This title/path finding does not
resolve broader semantic similarities. The
[Du'Maathe Castle audit supplement](DU_MAATHE_CASTLE_AUDIT_REVIEW.md) records
five source-only WLD inbounds, the active Surface and Pit of Dragons boundary
pairs, direct typed references, and the bounded history result.

### Tundra

The normalized active title “Tundra” has one owner, area `tundra` at zone 137.
The reviewed tracked WLD/ZON history names no distinct predecessor or
replacement. This title/path finding does not resolve broader semantic
similarities. The
[Tundra audit supplement](TUNDRA_AUDIT_REVIEW.md) records seven source-only
WLD inbounds, direct external prototype references, and the bounded history
result.

### The Fields Between

The normalized active title “The Fields Between” has one owner, area
`fields_between` at zone 710. The reviewed tracked WLD/ZON history names no
distinct predecessor or replacement. This title/path finding does not resolve
broader semantic similarities. The
[Fields Between audit supplement](FIELDS_BETWEEN_AUDIT_REVIEW.md) records one
source-only WLD inbound, direct external prototype references, and the bounded
history result.

### The Town of Moregeeth

The normalized active title “The Town of Moregeeth” has one owner, area
`goblinht` at zone 700. The reviewed tracked WLD/ZON history names no distinct
predecessor or replacement. This title/path finding does not resolve broader
semantic similarities. The
[Moregeeth audit supplement](MOREGEETH_AUDIT_REVIEW.md) records one source-only
WLD inbound, the active boundaries, source-only shop references to local
prototypes, and the bounded history result.

### Shady Grove and the source-only `shady2.mob`

The normalized active title `Shady Grove` has one owner, area `shady` at zone
975. The tracked `shady.wld` history begins with its 2009 path addition and
contains later edits through 2013; no rename to a different zone stem appears
in the followed path history. The separate unregistered `shady2.mob` first
appears in commit `642da5400` (2009-08-12) and has no later tracked edits. It
defines 90 mobile VNUMs #97500–#97589 also present in active `shady.mob`, whose
additional definitions are #97590–#97605. The shared records are not
line-identical, and five alias lists differ. This is a source collision, not
proof that `shady2` is a canonical predecessor or replacement. Preserve the
two files separately until their source intent and owner are known. The
[Shady Grove audit supplement](SHADY_GROVE_AUDIT_REVIEW.md) also records three
source-only `surf.wld` approaches and distinguishes the different
same-numbered `surface2011.wld` map records.

### Apocalypse Castle and the `4horse` history boundary

The active `.ZON` title `Apocalypse Castle` has one owner, area `4horse` at
zone 345, among 351 current non-empty active titles. The followed
`4horse.wld` history begins with the 2009 path addition and contains later
world edits through 2013; no rename to a different zone stem or explicit
replacement claim appears in that history. The reviewed manifest and tracked
history establish no distinct predecessor or superseded source. Keep the
active source identity intact. The
[Apocalypse Castle audit supplement](APOCALYPSE_CASTLE_AUDIT_REVIEW.md)
records its two active inbound boundaries, one reciprocal Surface pair, one
source-only `surf.wld` approach, and typed outside consumers.

### Ixxillikor

The active `.ZON` title “Ixxillikor” has one normalized owner among the 351
current non-empty titles. The followed WLD path starts with the 2009
repository-wide `areas/` path move and includes a 2011 edit. The reviewed
manifest and tracked path history identify no different-stem predecessor or
explicit replacement. Preserve the active source identity; broader untracked
and external history is not established. See the [Ixxillikor audit
supplement](IXXILLIKOR_AUDIT_REVIEW.md).

## Classification boundary

- A same-name/title pair is a naming overlap until source evidence proves a
  shared campaign, replacement, or duplicate record.
- A `continued`, `past`, `future`, `old`, or numeric-suffix stem needs source
  and manifest evidence before receiving a replacement label.
- Equal item recipes, display names, or story themes do not transfer giver
  ownership or establish one zone as a copy of another.
- Retired files outside `areas/AREA` remain dependency-review candidates until
  active exits, reset sources, QST terms, shops, specials, editor inputs, and
  user-controlled persisted references are reconciled.

The candidate-by-candidate retention plan is
[`ATTIC_MIGRATION_PLAN.md`](ATTIC_MIGRATION_PLAN.md). The exact active WLD
edges are in [`WORLD_CONNECTIVITY.md`](WORLD_CONNECTIVITY.md).

### Moonshae Island

The normalized active `.zon` title “Moonshae Island” has one owner among 351
current non-empty active titles; it is owned by `moonshae` at zone 262. The
followed `moonshae.wld` path begins with the 2009-08-12 repository-wide `areas/`
path move and includes same-path edits in 2011, 2012, and 2013. Reviewed likely
alternate stems and tracked path history show no distinct predecessor,
different-stem rename, or replacement. This title/path finding does not decide
broader semantic similarities. See the
[Moonshae audit supplement](MOONSHAE_ISLAND_AUDIT_REVIEW.md) for the source
references, active graph, and typed consumers.

### Prison of Fort Boyard

The normalized active `.zon` title “Prison of Fort Boyard” has one owner among
351 current non-empty active titles. Followed `prisonb.wld` history includes
the 2009 repository-wide `areas/` move and same-path edits in 2013 and 2015;
followed `prisonb.zon` history includes the move and a 2013 edit. The reviewed
manifest and tracked paths show no distinct predecessor, alternate-stem rename,
or explicit replacement. See the [Prison of Fort Boyard audit
supplement](PRISON_OF_FORT_BOYARD_AUDIT_REVIEW.md).

The active `prison` area is separately titled “The Prisons of Carthapia” and
uses VNUMs 7300–7498, while `prisonb` uses 43000–43051. Both remain active.
Their shared prison theme does not establish duplicate ownership or a
replacement relationship. Preserve both source identities; Prisonb remains
**DO NOT MOVE**.

### Lair of the Purple Worm

The normalized active `.zon` title “Lair of the Purple Worm” has one owner
among 351 current non-empty active titles. Followed `pworm.wld` history starts
with the 2009 repository-wide `areas/` move and includes a 2013 edit; followed
`pworm.zon` history also includes the 2015 quest-item-flag change. The current
story-specific 2026 commit adds the seven-hide family progression. The reviewed
manifest and tracked paths show no alternate-stem rename, distinct predecessor,
or explicit replacement. See the [Lair of the Purple Worm audit
supplement](LAIR_OF_THE_PURPLE_WORM_AUDIT_REVIEW.md).

Active `worms`, titled “The Caverns of the Worms,” is a separate zone with a
different source identity and VNUM range. The shared worm theme does not prove
that either active area replaces or duplicates the other. Preserve both areas;
`pworm` remains **DO NOT MOVE**.

### Vargan

The normalized active `.zon` title “Vargan” has one owner among 351 current
non-empty active titles. Followed `vargan.wld` history begins with the
2009-08-12 repository-wide `areas/` move and includes same-path edits in 2011,
2013, and 2026. Followed `vargan.zon` history includes the 2012 zone-field
update, 2015 quest-item-flag change, and 2016 minotaur-race correction. The
2026 Vargan journal commit records the current armor story. The reviewed active
manifest and tracked paths establish no alternate-stem rename, distinct
predecessor, or explicit replacement. See the [Vargan audit
supplement](VARGAN_AUDIT_REVIEW.md).

The active `v2` area has the separate title “Vargan II” and connects
reciprocally to Vargan at #2182/#7800. Its lore refers to Vargan's children and
disciples, but the distinct active stem and reciprocal boundary do not show a
replacement. Preserve both active source identities; Vargan remains **DO NOT
MOVE**.

### The Maze of Undead Army

The normalized active `.zon` title “The Maze of Undead Army” has one owner
among 351 current nonempty active titles. A scan of current `.zon` titles found
no other title containing “maze” or “undead army.” Tracked history follows
`maze_are.wld` through the 2009 repository path relocation, 2011 map edits, and
2013 same-path edits; `maze_are.zon` has the 2009 relocation and 2015
quest-item-flag update. The 2026 story commit adds `maze_are.story.json`. The
tracked paths and current source inventory show no alternate-stem rename,
distinct predecessor, or explicit replacement. The room-prose phrase “Maze of
Death” is not separate source-history evidence. See the [Maze audit
supplement](MAZE_OF_UNDEAD_ARMY_AUDIT_REVIEW.md).

The active Surface boundary reciprocates with Maze entrance #94177. An
additional `surf.wld` edge and an old `brass-old-1.zon` reset reference are
source-only records, not evidence that either source replaces Maze or is
currently loaded. Preserve the distinct active `maze_are` identity; Maze
remains **DO NOT MOVE**.

### Bandit Camp and The Bandit Canyons

The normalized active `.zon` title “Bandit Camp” has one owner among 351 current non-empty active titles: `banditca`, zone 148, physical rooms #14800–#14926. The separate active `bandit` contributor is titled “The Bandit Canyons,” zone 985, physical rooms #98500–#98575. The active manifest lists both sources; their zone identities, room ranges, and WLD networks are distinct, and the current active graph has no direct WLD edge between them.

Followed Git history shows independent same-stem paths and later edits for `banditca` and `bandit`. It contains no explicit replacement claim or distinct predecessor for Bandit Camp. The 2009 repository-wide area move adds both current WLD paths in their new locations but does not establish whether an earlier path or untracked predecessor existed. Similar titles and bandit subject matter do not show that Bandit Canyons replaced Bandit Camp. Preserve both active source identities. Bandit Camp remains **DO NOT MOVE**; see the [Bandit Camp audit supplement](BANDIT_CAMP_AUDIT_REVIEW.md) for the exact title scan and followed history.

### Myrloch Vale

The normalized active `.zon` title “Myrloch Vale” has one owner among 351 current non-empty active titles. The only lexical near-matches found were `mist_chasm` (“The Chasm of the Misty Vale”) and `glacier` (“Towering Glacier of Dayedenvale”); they do not establish shared ownership. Followed `myrloch_vale.wld` and `.zon` paths include the 2009 repository-wide `areas/` move, same-path edits in 2013 and 2016, and later area updates. The 2016 change integrates `unblock_on_death`; the 2026-10-06 story commit adds the journal/world-action sidecar. The current active manifest and followed paths show no explicit predecessor, alternate-stem rename, or replacement claim. Moonshae is a separate active source with a reciprocal boundary to Myrloch. Preserve both identities; Myrloch Vale remains **DO NOT MOVE**. See the [Myrloch Vale audit supplement](MYRLOCH_VALE_AUDIT_REVIEW.md) for source references and the bounded title/history result.


### Cloud Giant Kingdom

The normalized active `.zon` title “Cloud Giant Kingdom” has one owner among 351 current non-empty active titles. Other titles containing “kingdom” include `torg` (“The Kingdom of Torg”), `arcium` (“Arcium, the Plagued Kingdom”), and `seakngdm` (“Sea Kingdom”); none is an exact title or source identity match. Followed `cldgt.wld` and `cldgt.zon` histories include the 2009 repository-wide `areas/` move and same-path edits in 2013; WLD history also has a 2009 spelling update and ZON history a 2015 quest-item-flag update. The 2026-10-06 story commit adds the journal/legacy-quest sidecar. The active manifest and followed paths show no explicit predecessor, alternate-stem rename, or replacement claim. Surface is a separate active contributor connected by the reciprocal boundary; inactive `surf.wld` also has one source-only approach to Cloud entrance #99500. Preserve those source identities separately. Cloud Giant Kingdom remains **DO NOT MOVE**; see the [Cloud Giant Kingdom audit supplement](CLOUD_GIANT_KINGDOM_AUDIT_REVIEW.md).

### The Mountain of the Banished

After color-code removal, the active `.zon` title “The Mountain of the Banished” has one owner among 351 non-empty active titles. Similar titles, including Mountain Tracts, belong to different active contributors. `git log --follow` for `mount.wld` and `mount.zon` shows the 2009 area-path move and same-stem edits; the ZON path includes quest-item-flag and 2026 equipment/ambassador updates. `mount.story.json` was added on 2026-10-06. The followed paths contain no explicit predecessor, alternate-stem rename, or replacement claim. The separate inactive approaches from `oldmap8.wld` and `surf.wld` into entrance #9100 do not establish either map as a predecessor. Preserve the active `mount` identity and reciprocal Surface pair; Mountain remains **DO NOT MOVE**. See the [Mountain audit supplement](MOUNTAIN_OF_THE_BANISHED_AUDIT_REVIEW.md) for bounded title and history evidence.

### Tiamat

The normalized active `.zon` title “Tiamat” has one exact owner among 351 current non-empty active titles. `astral_tiamat` (“Astral Plane, Tiamat”) and `azhural` (“The Lair of Tiamat”) are separate active sources with distinct titles, not alternate stems or evidence of succession. Followed `tiamat.wld` and `tiamat.zon` histories show the 2009 area-path move, 2009 route/alcove edits, later same-stem loot and teleport changes, and the 2015 quest-item-flag update. The Tiamat story sidecar was added on 2026-10-06. These paths show no explicit predecessor, alternate-stem rename, or replacement claim. Preserve the active `tiamat` identity; Tiamat remains **DO NOT MOVE**. See the [Tiamat audit supplement](TIAMAT_AUDIT_REVIEW.md).

### The Obsidian Citadel

The normalized active `.zon` title “The Obsidian Citadel” has one owner among 351 current active titles. Similar lexical titles belong to separate active stems: `bctdl` (“The Bronze Citadel”) and `citadel` (“The Citadel”). They are not exact title or source-stem matches.

Followed `obcita.wld` and `.zon` history includes the 2009 repository-wide `areas/` path relocation and later same-stem edits; WLD edits continue in 2010-2013 and the ZON path includes the 2015 quest-item-flag update. The tracked `.mob`, `.obj`, and `.qst` paths also remain under `obcita`; `obcita.story.json` was added on 2026-10-06. No alternate-stem rename, explicit predecessor, or replacement claim appears in these followed paths. The source-only `surf.wld` edge to Obcita does not establish replacement or active access. Preserve the active `obcita` identity; it remains **DO NOT MOVE**. See the [Obcita audit supplement](OBCITA_AUDIT_REVIEW.md).

### The Chasm of the Misty Vale

After color-code removal, “The Chasm of the Misty Vale” has one exact owner among the 351 current active .zon titles: mist_chasm. Nearby lexical names mistywood, myrloch_vale, and glacier have separate normalized titles and active source identities.

Followed mist_chasm.wld, .zon, .mob, .obj, and .qst histories show the 2009 repository-wide areas/ relocation and later same-stem edits; the story sidecar was added on 2026-10-06. The current manifest and followed paths show no explicit predecessor, alternate-stem rename, or replacement claim. The inactive surf.wld and torr.wld approaches into Chasm are source-only connectivity records, not evidence that either map replaces Chasm or is currently loaded. Preserve active mist_chasm; it remains **DO NOT MOVE**. See the [Checkpoint 112 audit supplement](MIST_CHASM_AUDIT_REVIEW.md) for bounded path-history detail.

### Varathorn Keep

After color-code and tilde removal, “Varathorn Keep” has one exact owner among the 351 active .zon titles: kastle. Nearby lexical names include Bloodstone Keep, The Keep of Evil, Krethik Keep, Shadamehr Keep, and The Twin Keeps of Devastated Tharnadia; they have distinct normalized titles and active source identities.

Followed kastle.wld, .zon, .mob, .obj, and .qst paths remain under the same stem after the 2009 repository-wide areas/ relocation, with later same-path edits. The kastle.story.json sidecar was added on 2026-10-06. The active manifest and followed paths show no alternate-stem rename, explicit predecessor, or replacement claim. The two inactive surf.wld approaches do not establish replacement or current access. Preserve active kastle; it remains **DO NOT MOVE**. See the [Checkpoint 113 audit supplement](KASTLE_AUDIT_REVIEW.md) for the bounded title, dependency, and history evidence.

### Malch'Hor Ganl the Goblin City

After color-code and tilde removal, the active title “Malch'Hor Ganl the Goblin City” is unique among 351 active `.zon` titles and belongs to `malch`. “The Goblin Camp” belongs to `goblincamp`; their normalized titles and source identities differ. Followed `malch.wld`, `.zon`, `.mob`, `.obj`, `.qst`, and `.shp` histories remain under the same stem after the 2009 repository-wide `areas/` relocation. The story sidecar was added on 2026-10-06. No alternate-stem rename, explicit predecessor, or replacement claim appears in the current manifest or followed histories. The inactive Surf approach is a source-only boundary and does not establish a replacement or current route. Preserve active `malch`; it remains **DO NOT MOVE**. See the [Checkpoint 114 audit supplement](MALCH_AUDIT_REVIEW.md) for title, history, and dependency details.

### Miaeril Village

After Duris color-code removal and normalization, “Miaeril Village” has one exact owner among 351 current non-empty active `.zon` titles: `mril` ([title](../../../areas/zon/mril.zon#L2)). The title scan found no other active lexical candidate containing “Miaeril.” Followed `mril.wld`, `.zon`, `.mob`, `.obj`, `.qst`, and `.shp` paths remain under the same stem after the 2009 repository-wide `areas/` relocation, with same-stem edits through 2011, 2013, 2014 shop cleanup, 2015 quest-item correction, and 2025–2026 area updates. The story sidecar was added on 2026-10-06. No alternate-stem rename, distinct predecessor, or explicit replacement claim appears in these bounded paths. This history does not establish the absence of an untracked predecessor. Preserve active `mril` as its own identity; it remains **DO NOT MOVE**. See the [Checkpoint 115 Miaeril audit supplement](MIAERIL_AUDIT_REVIEW.md).

### The Temple of the Sun

After Duris color-code removal and title normalization, “The Temple of the Sun” has one exact owner among 351 current non-empty active `.zon` titles: `suntmpl`. Other Temple titles, including Temple of the Earth, Temple of Flames, Temple of Laduguer, and Temple to Skrentherlog, belong to distinct active sources and have different normalized titles. Followed `suntmpl.wld`, `.zon`, `.mob`, `.obj`, and `.qst` histories remain under the same stem after the 2009 repository-wide `areas/` relocation; the story sidecar was added 2026-10-06. No alternate-stem rename, distinct predecessor, or explicit replacement claim appears in the followed paths. This does not prove the absence of an untracked predecessor. Preserve active `suntmpl`; it remains **DO NOT MOVE**. See the [Checkpoint 116 Temple audit supplement](TEMPLE_OF_THE_SUN_AUDIT_REVIEW.md).

### Lair of the Gibberling King

After Duris color-code removal and title normalization, “Lair of the Gibberling King” has one exact owner among 351 current non-empty active `.zon` titles: `gibber`. The title scan found no other active lexical candidate containing “Gibber.” Followed `gibber.wld`, `.zon`, `.mob`, `.obj`, and `.qst` paths remain under the same stem after the 2009 repository-wide `areas/` relocation; the story sidecar was added on 2026-10-06. No alternate-stem rename, distinct predecessor, or explicit replacement claim appears in those bounded histories. The eighth source-only `ud_map2` approach does not establish a current route or replacement. Preserve active `gibber`; it remains **DO NOT MOVE**. See the [Checkpoint 117 Gibber audit supplement](GIBBERLING_KING_AUDIT_REVIEW.md).

### Ice Tower

After Duris color-code removal and normalization, “Ice Tower” has one exact owner among 351 non-empty active `.zon` titles: `icetower`. The normalized-title scan found no other active lexical candidate containing “Ice Tower.” Followed `icetower.wld`, `.zon`, `.mob`, `.obj` and `.qst` histories remain under the same stem after the 2009 repository-wide `areas/` relocation; the story sidecar was added on 2026-10-06 (`0b68e45fd`). No alternate-stem rename, distinct predecessor or explicit replacement claim appears in these followed paths. This does not establish the absence of an untracked predecessor. Preserve active `icetower`; it remains **DO NOT MOVE**. See the [Ice Tower audit supplement](ICE_TOWER_AUDIT_REVIEW.md).

### Durian Underdark

After color-token removal and title normalization, “The Twisting Tunnels of the Durian Underdark” has one exact owner among 351 active non-empty `.zon` titles: `underdark`. “The Underdark” (`mobs_underdark`) and “The Minizones of the Underdark” (`udmini`) are distinct titles. Followed `underdark.wld`, `.zon`, `.mob`, `.obj`, and `.qst` histories remain on this stem after the 2009 `areas/` relocation; its story sidecar was added in commit `26fd4a945` on 2026-10-06. No alternate-stem rename, distinct predecessor, or explicit replacement claim appears in these paths. This bounded history does not prove the absence of an untracked predecessor. Preserve active `underdark` (**DO NOT MOVE**) and keep `underdark2011` **RETAIN**. See the [Underdark audit supplement](DURIAN_UNDERDARK_AUDIT_REVIEW.md).

### Bahamut's Palace

After color-token removal and title normalization, “Bahamut's Palace” has one exact owner among 351 active non-empty `.zon` titles: `bahamut`. The lexical title scan found no other active candidate containing “Bahamut.” Followed `bahamut.wld`, `.zon`, `.mob`, `.obj`, and `.qst` histories remain on this stem after the 2009 `areas/` relocation; the story sidecar was added in commit `268c4eb78` on 2026-10-06. No alternate-stem rename, distinct predecessor, or explicit replacement claim appears in these paths. This bounded review does not prove the absence of an untracked predecessor. Preserve active `bahamut` (**DO NOT MOVE**). See the [Bahamut audit supplement](BAHAMUT_AUDIT_REVIEW.md).

### Checkpoint 121 — Clan Stoutdorf Settlement

After stripping color tokens and normalizing whitespace/case, “Clan Stoutdorf Settlement” has one exact owner among the 351 current active `.zon` titles: `drst`. Followed `drst.wld`, `.zon`, `.mob`, `.obj`, and `.qst` paths remain under the same stem after the 2009 `areas/` relocation. The tracked paths show later same-stem edits, including the 2015 quest-item flag update and 2026 ambassador/reset changes; `drst.story.json` was added on 2026-10-06. No alternate-stem rename, explicit predecessor, or replacement claim appears in these bounded histories. Preserve active `drst` as **DO NOT MOVE**. See the [DRST audit supplement](DRST_AUDIT_REVIEW.md) and [coverage row](ZONE_COVERAGE_REGISTER.csv#L82).

### Checkpoint 122 — The Charcoal Palace

After stripping color tokens and normalizing whitespace/case, “The Charcoal Palace” has one exact owner among 351 current active `.zon` titles: `firep`. Followed Firep WLD/ZON/MOB/OBJ/QST histories remain under the same stem after the 2009 `areas/` relocation; the story sidecar was added on 2026-10-06. Commit `0673b6654` explicitly describes moving the Sword of Fire and Flame from old Brass into Firep: it adds object #88327 and updates its Kossuth reset and enabled Zion binding. This is a specific item transfer, not evidence that a whole Brass area was replaced by Firep. Separately, unregistered `brass-old-1.zon` line 812 resets local Firep scales #88322; it is not active stock. Keep active `firep` **DO NOT MOVE** and preserve `brass-old-1` **RETAIN** pending its own closure. See the [Firep audit supplement](FIREP_AUDIT_REVIEW.md), [Brass candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17), and [coverage row](ZONE_COVERAGE_REGISTER.csv#L96).

### Checkpoint 123 — Menden-on-the-Deep

After stripping color tokens and normalizing whitespace/case, “Menden-on-the-Deep” has one exact owner among 351 current active `.zon` titles: `menden`. Followed `menden.wld`, `.zon`, `.mob`, `.obj`, and `.qst` paths remain under the same stem after the 2009 `areas/` relocation; the story sidecar was added on 2026-10-06. No alternate-stem rename, explicit predecessor, or replacement claim appears in these bounded histories. The two inactive `surf.wld` approaches into #88846 are source-only map connections, not evidence of replacement or active access. Preserve active `menden` as **DO NOT MOVE** and keep `surf`/`surface2011` **RETAIN**. See the [Menden audit supplement](MENDEN_AUDIT_REVIEW.md) and [coverage row](ZONE_COVERAGE_REGISTER.csv#L174).

### Checkpoint 124 — Mitashi and the Duris3 map identity

After color-token removal and title normalization, “Mitashi - Capital City of the Jade Empire” has one exact owner among 351 active `.zon` titles: `mitashi`. Its WLD, ZON, MOB, OBJ and QST paths enter the active area in the 2012 `49df21a02` addition and remain under those source stems in the followed histories. The `mitashi.shp` path has a component-level copy-detection link to `stormport.shp` in that same commit (`C086`); its four current shop records use local Mitashi IDs. This file ancestry does not establish Storm Port as a predecessor or whole-zone replacement. The Mitashi story sidecar was added in 2026 (`dc0672461`).

The inactive `Duris3.wld` has 229 same-numbered source rooms across Mitashi's current #138200–#138428 range. Sample room titles and authored map topology differ from the active Mitashi identities. These overlaps and the 462 non-overlapping source-only approaches are not active ingress or replacement evidence. Keep `mitashi` **DO NOT MOVE** and `Duris3` **RETAIN IN PLACE**. See the [Mitashi audit supplement](MITASHI_AUDIT_REVIEW.md), [active area row](../../../areas/AREA#L393), and [title source](../../../areas/zon/mitashi.zon#L2).

### Checkpoint 125 — The Dark Stone Tower of the Northern Realms

After removing color tokens and normalizing whitespace/case, “The Dark Stone Tower of the Northern Realms” has one exact owner among 351 active `.zon` titles: `teka2`. Followed WLD, ZON, MOB, OBJ and QST histories remain under the `teka2` stem in the available tracked history from the 2009 area-source commit, followed by same-stem changes through 2013, 2015 and 2026. No alternate-stem rename, explicit predecessor or whole-zone replacement claim appears in those paths. The `teka2.story.json` sidecar was added in 2026 on the same stem. Preserve active `teka2` **DO NOT MOVE**. See the [Teka2 audit supplement](TEKA2_AUDIT_REVIEW.md), [area membership](../../../areas/AREA#L271), and [title source](../../../areas/zon/teka2.zon#L2).

### Checkpoint 126 — The Troll Hills

After stripping color tokens and normalizing whitespace/case, “The Troll Hills” has one exact owner among 351 active `.zon` titles: `troll_hills`. Followed WLD, ZON, MOB, OBJ and QST histories remain on the same stem from the 2009 area relocation through 2013 same-stem edits; the story sidecar was added on 2026-10-06 in commit `3c13be6d6`. No alternate-stem rename, explicit predecessor or whole-zone replacement claim appears in these bounded paths. The two inactive Surf approaches are source-only map evidence, not replacement evidence. Keep active `troll_hills` **DO NOT MOVE** and `surf` / `surface2011` **RETAIN**. See the [Troll Hills audit supplement](TROLL_HILLS_AUDIT_REVIEW.md), [area membership](../../../areas/AREA#L7), and [title source](../../../areas/zon/troll_hills.zon#L2).

### Checkpoint 127 — A Dark and Twisted Wood

After stripping color tokens and normalizing whitespace/case, “A Dark and Twisted Wood” has one exact owner among 351 active `.zon` titles: `twstwd`. Followed WLD, ZON, MOB, OBJ and QST histories stay on that stem from the 2009 area relocation through later same-stem edits, including the 2015 quest-item flag update. The story sidecar was added on 2026-10-06 in commit `14be6f392`. No alternate-stem rename, explicit predecessor, or whole-zone replacement claim appears in these bounded paths. The five inactive Surf approaches are source-only map evidence, not replacement evidence. Keep active `twstwd` **DO NOT MOVE** and `surf` / `surface2011` **RETAIN**. See the [Twisted Wood audit supplement](DARK_AND_TWISTED_WOOD_AUDIT_REVIEW.md), [coverage row](ZONE_COVERAGE_REGISTER.csv#L321), and [title source](../../../areas/zon/twstwd.zon#L2).

### Checkpoint 128 — The Underworld title overlap and bounded history

After stripping color tokens and normalizing whitespace/case, “The Underworld” has two exact owners among 351 active `.zon` titles: `underworld` and `underworld3`. Their active source ranges are distinct (#4401–#4645 and #30000–#30282). The duplicate-title register treats the numeric suffix as possible continuation wording, not proof of a copied, merged, or superseded zone. Bounded same-stem WLD/ZON history follows Underworld to its 2009 area relocation and later same-stem edits through 2016; its story sidecar was added in commit `5624fd43f` on 2026-10-06. No explicit predecessor or whole-zone replacement is established. Keep active `underworld` **DO NOT MOVE** and retain `underworld3` as a separate active contributor. See the [Underworld audit supplement](UNDERWORLD_AUDIT_REVIEW.md), [coverage row](ZONE_COVERAGE_REGISTER.csv#L326), [active area rows 23 and 145](../../../areas/AREA#L23), and [title sources](../../../areas/zon/underworld.zon#L2) / [Underworld3](../../../areas/zon/underworld3.zon#L2).

### Checkpoint 129 — Zalkapfaan title and bounded history

After stripping color tokens and normalizing whitespace/case, “Zalkapfaan, City of the Headless Horde” has one exact owner among 351 active `.zon` titles: `headless`. Followed WLD, ZON, MOB, OBJ and QST histories remain on the `headless` stem from the 2009 area relocation through same-stem edits; the WLD and OBJ have later 2026 edits. The story sidecar was added on 2026-10-06 in commit `500462537`. No alternate-stem rename, explicit predecessor, or whole-zone replacement claim appears in these bounded paths. The one inactive Surf approach is source-only map evidence, not replacement evidence. Keep active `headless` **DO NOT MOVE** and `surf` / `surface2011` **RETAIN**. See the [Zalkapfaan audit supplement](ZALKAPFAAN_AUDIT_REVIEW.md), [coverage row](ZONE_COVERAGE_REGISTER.csv#L120), and [title source](../../../areas/zon/headless.zon#L2).

### Checkpoint 130 — The Spires and its Duris3 map identity

After stripping color tokens and normalizing whitespace/case, “The Spires of the Elder Eternal Evil” has one exact owner among 351 active `.zon` titles: `eternal`. Commit `85e5be06c` added the WLD, MOB, OBJ, ZON, and QST on 2010-12-15 and describes Eternal as a Yuan-Ti2 follow-up without a direct game connection. Later changes remain on the same stem; the story sidecar was added on 2026-10-06 in commit `8d1611158`. This supports a design follow-up, not an explicit predecessor or replacement.

The inactive `Duris3.wld` has 1,332 records targeting active Eternal room VNUMs. The 602 source-only records come from non-overlapping room bands; 730 additional records begin in the same VNUMs as active Eternal rooms. A sample overlap confirms distinct room identity: Duris3 #135400 is titled “The Ocean,” while Eternal #135400 is a Spires courtyard ([Duris3](../../../areas/wld/Duris3.wld#L743401), [Eternal](../../../areas/wld/eternal.wld#L1)). Keep active `eternal` **DO NOT MOVE** and retain `Duris3` in place. See the [Spires audit supplement](SPIRES_OF_ELDER_EVIL_AUDIT_REVIEW.md), [coverage row](ZONE_COVERAGE_REGISTER.csv#L90), [Duris3 candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), and [title source](../../../areas/zon/eternal.zon#L2).

### Checkpoint 131 — Shaughin title and bounded history

After stripping color tokens and normalizing whitespace/case, “The Shaughin Settlement” has one exact owner among 351 active `.zon` titles: `shaughin`. Followed WLD and QST histories in the current path show the 2009 area-tree relocation; MOB/OBJ also have 2012 and 2013 same-stem changes and the 2015 quest-item flag pass, and ZON has a same-stem 2015 pass. The sidecar was added on 2026-10-06 in commit `f1d7d3f1c`. No alternate-stem rename, explicit predecessor, or whole-zone replacement appears in these bounded paths. Keep active `shaughin` **DO NOT MOVE** and `shaughin2` independent. See the [Shaughin audit supplement](SHAUGHIN_AUDIT_REVIEW.md), [coverage row](ZONE_COVERAGE_REGISTER.csv#L270), and [title source](../../../areas/zon/shaughin.zon#L2).

### Checkpoint 132 — Trakkia title, history, and Surf approach

After stripping color tokens and normalizing whitespace/case, “The Trakkia Mountains” has one exact owner among 351 active `.zon` titles: `trakkia`. Followed same-stem source history includes the 2010 sneakability and Tharn/switch changes, same-stem maintenance through 2014, 2016 chest changes, and 2026 ambassador/equipment edits. The story sidecar was added on 2026-10-06 in commit `287c9070e`. No alternate-stem rename, explicit predecessor, or whole-zone replacement appears in these bounded paths. The inactive Surf edge #246406 west to #57000 is map evidence, not active ingress or replacement evidence. Keep `trakkia` **DO NOT MOVE** and retain `surf` / `surface2011`. See the [Trakkia audit supplement](TRAKKIA_AUDIT_REVIEW.md), [coverage row](ZONE_COVERAGE_REGISTER.csv#L310), and [title source](../../../areas/zon/trakkia.zon#L2).

### Checkpoint 133 — Kelek title, history, and current quest ownership

After stripping color tokens and normalizing whitespace/case, “The Stone Tomb of Kelek” has one exact owner among 351 active `.zon` titles: `kelek`. Followed WLD/MOB/OBJ/ZON/QST histories remain under the same stem, with historical 2009 topology/teleport fixes, a switch/key pass, and later maintenance. The sidecar was added on 2026-10-06 in commit `bc62e2569` and corrected in `0c6f70007`; the current schema2 Kelek entry owns only its smith return while the Church contracts now belong to Church. This owner correction is not an area replacement. No alternate-stem rename, explicit predecessor, or whole-zone replacement appears in bounded history. Keep Kelek **DO NOT MOVE**. See the [Kelek audit supplement](KELEK_AUDIT_REVIEW.md), [coverage row](ZONE_COVERAGE_REGISTER.csv#L143), and [title source](../../../areas/zon/kelek.zon#L2).

### Checkpoint 134 — Krethik title, history, and source-only approaches

After stripping color tokens and normalizing whitespace/case, “Krethik Keep” has one exact owner among 351 active `.zon` titles: `krethik`. Followed WLD/MOB/OBJ/ZON/QST paths stay on that stem, with historical same-stem maintenance through 2016 and a 2026 MOB cleanup; the story sidecar was added on 2026-10-06 in commit `5c543747b`. No alternate-stem rename, explicit predecessor, or whole-zone replacement appears in these paths. Two inactive oldmap8 and two inactive Surf approaches are source-only map evidence, not active ingress or replacement evidence. Keep `krethik` **DO NOT MOVE** and retain the four source records. See the [Krethik audit supplement](KRETHIK_AUDIT_REVIEW.md), [coverage row](ZONE_COVERAGE_REGISTER.csv#L149), and [title source](../../../areas/zon/krethik.zon#L2).

### Checkpoint 135 — Thetis title, history, and source-only approach

After stripping color tokens and normalizing whitespace/case, “Thetis's Realm” has one exact owner among 351 active .zon titles: thetis. Bounded WLD history stays on the thetis stem and supplies no alternate-stem rename, explicit predecessor, or whole-zone replacement evidence. The all-95 inactive-WLD sweep adds one non-overlapping Surf approach to #38000; the separate #38095 north → #104 target remains unresolved against inactive dalvik/mavulsk candidates. See the [Thetis audit supplement](THETIS_AUDIT_REVIEW.md).

### Checkpoint 136 — Azhural version swap and retained QST path

The active title “The Lair of Tiamat” has one owner among 351 active .zon contributors: azhural. The unregistered tiamats_lair source carries the same title and 83 overlapping WLD room VNUMs, plus 20 mobile and 38 object VNUM overlaps. Commit 50b400005d0bfc19adb51cda0d74414e940a7b51 (2012-08-07) explicitly describes Azhural as the new version and says the older version was commented out of AREA. The remaining QST split is material: qst/AREA still selects tiamats_lair.qst, not azhural.qst, although the 97-line files are byte-identical today. Keep azhural active, retain the tiamats_lair QST/source family, and do not claim that an archive is clear. See the [Azhural audit supplement](AZHURAL_AUDIT_REVIEW.md).

### Checkpoint 137 — Quintaragon Castle title and bounded history

After color removal and whitespace/case normalization, “Quintaragon Castle” has one exact owner among 351 active `.zon` titles: `castle`. Followed Castle WLD, MOB, OBJ, ZON, and QST histories show the 2009 area-tree relocation and later same-stem maintenance, including the 2015 quest-item-flag pass. No alternate-stem rename, explicit predecessor, or whole-zone replacement appears in those paths. Its title is distinct from Apocalypse Castle, Bastine Castle, and Icecrag Castle. See the [Castle audit supplement](CASTLE_AUDIT_REVIEW.md) and [title source](../../../areas/zon/castle.zon#L2).

### Checkpoint 138 — Church title, bounded history, and legacy item reference

After color removal and whitespace/case normalization, “The Church of the Eternal Dusk” has one exact owner among 351 active `.zon` titles, and the all-file title scan finds no exact inactive title duplicate. Followed Church WLD/MOB/OBJ/ZON/QST paths show the 2009 area-tree relocation and later same-stem maintenance, with no alternate-stem rename, explicit predecessor, or whole-zone replacement. Separately, unregistered `brass-old-1.zon` line 924 G-loads active Church object #87859; this is source-only and not a live placement. See the [Church audit supplement](CHURCH_AUDIT_REVIEW.md) and the retained [Brass candidate row 17](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17).

### Checkpoint 139 — Domain title, bounded history, and source-only dependencies

After removing color tokens and normalizing whitespace/case, “Domain of Lost Souls” occurs once across current `.zon` sources, in `dlsc.zon`. Followed same-stem source history shows the 2009 area-tree move and later maintenance; no alternate-stem rename, distinct predecessor, or whole-zone replacement was established. The all-file typed scan finds source-only unregistered `nexus56.zon` reset and `nexus56.shp` shop references to Domain book #36822; preserve both while `nexus56` remains RETAIN. See the [Domain audit supplement](DLSC_AUDIT_REVIEW.md), [coverage row 77](ZONE_COVERAGE_REGISTER.csv#L77), and [Nexus56 candidate row 79](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79).

### Checkpoint 140 — Drifting Realm title, bounded history, and cross-zone references

After stripping color tokens and normalizing whitespace/case, “Drifting Realm” occurs once across current `.zon` sources, in `dream.zon`. Followed same-stem history reaches the 2009 area-tree move and later Dream edits; no alternate-stem rename, distinct predecessor, or whole-zone replacement was established. Active outside ZON resets stock local Dream skulls #31317–#31320 in Mriel, Lava, Arcium, and Fishermans Wharf. Arcium's active bed #85725 targets Dream #31300, while an outside Fishermans Wharf story description mentions #31320 as prose rather than a typed field. See the [Drifting Realm audit supplement](DRIFTING_REALM_AUDIT_REVIEW.md), [coverage row 80](ZONE_COVERAGE_REGISTER.csv#L80), and Ethereal boundary pair [rows 121 and 133](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L121).

### Checkpoint 141 — Lizardman Swamps title, bounded history, and external references

Normalized “The Lizardman Swamps of Clavikord” occurs only in `lizard.zon` across 442 zone files and is unique in the active title set. Followed `lizard.wld` history reaches the 2009 area-tree move and 2011/2013 updates without an established alternate-stem rename or whole-zone replacement. The typed sweep records active Randomness reset/shop references to Lizard head #6008, plus source-only `sylvandawn.zon` reset #6002 and `tharnadia1.shp`/`tharold.shp` product records for #6007–#6009. These do not establish an active service or a zone replacement. See the [Lizardman Swamps audit supplement](LIZARDMAN_SWAMPS_AUDIT_REVIEW.md), [coverage row 160](ZONE_COVERAGE_REGISTER.csv#L160), and [sylvandawn](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L101), [tharnadia1](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L103), and [tharold](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L104) candidate rows.

### Checkpoint 142 — Tower of High Sorcery title, history, and cross-zone references

Normalized “Tower of High Sorcery” occurs only in `tower.zon` across 442 current zone-title files and is unique among active titles. Followed `tower.wld` history reaches the 2009 area-tree move, 2011/2013 edits, and later 2026 refactoring/line-ending commits; no alternate-stem rename, distinct predecessor, or whole-zone replacement was established. The two `surf.wld` exits into Tower are non-overlapping source-only edges, not active ingress. Typed closure records Labyrinth/Heavens/Winterhaven local imports, Winterhaven's #9375 consumption, Raxthan's #9332 reset/shop rows, and compiled item/room/mobile bindings. See the [Tower audit supplement](TOWER_OF_HIGH_SORCERY_AUDIT_REVIEW.md), [coverage row 308](ZONE_COVERAGE_REGISTER.csv#L308), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 143 — Vecna title, history, and Duris3 map identity

The normalized title “Vecna's Tomb” occurs once across all 442 .zon files and is unique among active titles. Followed same-stem WLD history reaches the 2009 area-tree move and later 2011–2013 edits; it establishes no distinct predecessor or whole-zone replacement. Duris3.wld has 80 rooms with VNUMs #130000–#130079 whose titles all differ from the active Vecna rooms. Its map-room identity overlaps do not establish a replacement or an active route. See the [Vecna audit supplement](VECNAS_TOMB_AUDIT_REVIEW.md), [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), and [coverage rows 336 and 387](ZONE_COVERAGE_REGISTER.csv#L336).

### Checkpoint 144 — Killing Fields title, history, and map records

The normalized title “Killing Fields” occurs once across all 442 .zon files and is unique among active titles. Followed same-stem WLD history reaches the 2009 area-tree move and a 2013 edit; it establishes no distinct predecessor or whole-zone replacement. The single non-overlapping Surf exit into #33302 is source-only, and the seven surf.zon reset records (one mobile, six objects) are unregistered source evidence rather than replacement evidence. See the [Killing Fields audit supplement](KILLING_FIELDS_AUDIT_REVIEW.md), [coverage row 145](ZONE_COVERAGE_REGISTER.csv#L145), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [Surf coverage row 448](ZONE_COVERAGE_REGISTER.csv#L448).

### Checkpoint 145 — The Para-Elemental Plane of Magma title, history, and map records

The normalized title occurs once across all 442 .zon files and is unique among active titles. Followed magma.wld history reaches the 2012 template and 2013 minor update; no distinct predecessor or whole-zone replacement was established. Duris3.wld has 129 same-numbered source rooms titled The Ocean; 256 of its Magma-targeting exits originate in those overlapping map records. A further 260 non-overlapping Duris3 exits and one Vodgh exit are source-only, not active ingress. Keep magma DO NOT MOVE and Duris3/vodgh RETAIN IN PLACE. See the [Magma audit supplement](MAGMA_AUDIT_REVIEW.md), [Magma coverage row 168](ZONE_COVERAGE_REGISTER.csv#L168), [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), and [Vodgh candidate row 118](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L118).

### Checkpoint 146 — Mazzolin title, history, and map records

The normalized title occurs once across all 442 .zon files and is unique among active titles. Followed mazzolin.wld history reaches the 2009 area-tree move and 2013 edit; no distinct predecessor or whole-zone replacement was established. The single source-only ud_map2.wld approach into #21300 does not establish a replacement or active ingress. Keep mazzolin DO NOT MOVE and ud_map2 RETAIN IN PLACE. See the [Mazzolin audit supplement](MAZZOLIN_AUDIT_REVIEW.md), [coverage row 173](ZONE_COVERAGE_REGISTER.csv#L173), and [ud_map2 candidate row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114).

### Checkpoint 147 — Myconid Mushroom Forest title and history

The normalized title `Myconid Mushroom Forest` occurs once among 442 `.zon` files and active titles. Followed `myconid.wld` history reaches the 2009 area-tree move and a 2013 edit; no distinct predecessor, replacement stem, or whole-zone replacement was established. The two `ud_map0`/`ud_map2` approaches are source-only records, not evidence of map replacement or active ingress. Keep active `myconid` DO NOT MOVE and retain both source maps. See the [Myconid audit supplement](MYCONID_AUDIT_REVIEW.md) and [coverage row 197](ZONE_COVERAGE_REGISTER.csv#L197).

### Checkpoint 148 — Vargan II title and history

The normalized title `Vargan II` occurs once among 442 `.zon` files and active titles. Followed `v2.wld`/`v2.zon` history reaches the 2009 area-tree move, 2012 field-format edit and 2013 zone edits; no alternate-stem rename, distinct predecessor or whole-zone replacement was established. Active `vargan` is a separate zone connected by a reciprocal boundary, not a replacement. See the [Vargan II audit supplement](V2_AUDIT_REVIEW.md), [coverage row 330](ZONE_COVERAGE_REGISTER.csv#L330), and [Vargan dossier](../zone-stories/V2.md).

### Checkpoint 149 — Phantasmagoric Caverns title and history

The normalized title `Phantasmagoric Caverns` occurs once among 442 `.zon` files and active titles. Followed `valdrak.wld` history reaches the 2009 area-tree move and a 2013 edit; followed `valdrak.zon` history also includes the 2015 quest-item-flag overhaul. These same-stem paths show no alternate-stem rename, distinct predecessor, or whole-zone replacement. The `ud_map2.wld` approach to #53100 is source-only map evidence, not active ingress or replacement evidence. Keep active `valdrak` DO NOT MOVE and retain the approach with `ud_map2`. See the [Valdrak audit supplement](VALDRAK_AUDIT_REVIEW.md), [coverage row 332](ZONE_COVERAGE_REGISTER.csv#L332), and [title source](../../../areas/zon/valdrak.zon#L2).

### Checkpoint 150 — The Bronze Citadel title and history

The normalized title `The Bronze Citadel` occurs once among all 442 `.zon` files and active titles. Followed `bctdl.wld` history reaches the 2009 area-tree move. Followed `bctdl.zon` history includes the 2009 move, 2011 object balancing, 2013 reset/compile edits, and the 2015 quest-item-flag pass. The shared `(mabort)` manifest note on active `avernus` and `bctdl` does not establish a source rename or replacement; no current `mabort` source stem exists. Avernus remains a separate active area, with its own gate object targeting Bctdl. See the [Bctdl audit supplement](BCTDL_AUDIT_REVIEW.md), [coverage row 30](ZONE_COVERAGE_REGISTER.csv#L30), and [title source](../../../areas/zon/bctdl.zon#L2).

### Checkpoint 151 — Brimeforge title and history

The normalized title “The BrimStone Forge” occurs once among the 442 scanned `.zon` files and active titles; the lexical candidate scan found none. Followed `brimeforge.wld` history begins with the 2009-09-05 “added brimforge xp zone for your review” commit and continues under the same stem through 2010, 2011, 2013, and 2025. The followed `brimeforge.zon` path begins with that same addition and includes later same-stem edits through 2015. No alternate-stem rename, distinct predecessor, or whole-zone replacement was established. The inactive Duris3 approaches and its same-numbered rooms are separate source-map records, not replacement or active-ingress evidence. Keep active `brimeforge` DO NOT MOVE and Duris3 RETAIN IN PLACE. See the [Brimeforge audit supplement](BRIMEFORGE_AUDIT_REVIEW.md) and [title source](../../../areas/zon/brimeforge.zon#L2).

### Checkpoint 152 — The Bugger Caves title and history

The normalized title “The Bugger Caves” occurs once among the 442 scanned .zon files and active titles; the lexical candidate scan found no other owner. Followed bugger.wld history records the 2009-08-12 areas/ path relocation; followed bugger.zon history records that move and the 2015 quest-item-flag update. No alternate-stem rename, distinct predecessor, or whole-zone replacement was established in these paths. The two active reciprocal WLD boundaries are current map connections, not replacement evidence. Keep active bugger DO NOT MOVE. See the [Bugger audit supplement](BUGGER_AUDIT_REVIEW.md) and [title source](../../../areas/zon/bugger.zon#L2).

### Checkpoint 153 — Dirk’nspire title and history refresh

The normalized title “Dirk’nspire Stronghold” is unique among 442 scanned .zon files; the corrected lexical scan found no other owner. The followed WLD/ZON paths continue to show only same-stem editing after the 2009 directory move and no distinct predecessor or replacement. The reciprocal Valinhav boundary and Alatorin’s typed consumer links are connectivity/dependency findings, not replacement evidence. Keep active dirkn DO NOT MOVE. See the [Dirk’nspire audit supplement](DIRKN_AUDIT_REVIEW.md), [coverage row 75](ZONE_COVERAGE_REGISTER.csv#L75), and [title source](../../../areas/zon/dirkn.zon#L2).

### Checkpoint 154 — Storm Port title and history

The normalized title “Storm Port” is unique among 442 scanned .zon files. Lexical neighbors spshold (Storm Port Stronghold), stormht (Strathor Valley of the Storm Giants), and portskythic (The Ruins of Port Skythic) are separate owners. Followed stormport.wld history reaches the 2009 directory move plus same-stem 2011/2013 edits; stormport.zon history adds same-stem 2010/2013 work, the 2015 quest-item-flag change, and the 2016 granite material fix. These bounded paths establish no alternate-stem rename, explicit predecessor, or whole-zone replacement. Active and source-only WLD edges are current map records, not replacement proof. Keep active stormport DO NOT MOVE. See the [Storm Port audit supplement](STORMPORT_AUDIT_REVIEW.md), [coverage row 284](ZONE_COVERAGE_REGISTER.csv#L284), and [title source](../../../areas/zon/stormport.zon#L2).

### Checkpoint 155 — Arachdrathos title and history

The normalized title “Arachdrathos - Drow City” occurs once among 442 scanned .zon files. Arachdrathos Wilderness (arac_wild), Arachdrathos Guilds (aracguil), and Llzazan Ghetto of Arachdrathos (llzazan) are separate title owners. Followed arac-web.wld history reaches the 2009 directory relocation and same-stem 2010/2013 edits; arac-web.zon history adds same-stem 2011/2013 work, the 2015 quest-item-flag pass, and a 2017 Beligron Earth Reaver spec change. No alternate-stem rename, explicit predecessor, or whole-zone replacement appears in those bounded paths. The active reciprocal boundaries and candidate ZON object/mobile references do not imply a replacement map. Keep arac-web DO NOT MOVE. See the [Arachdrathos audit supplement](ARAC_WEB_AUDIT_REVIEW.md), [coverage row 9](ZONE_COVERAGE_REGISTER.csv#L9), and [title source](../../../areas/zon/arac-web.zon#L2).

### Checkpoint 156 — Arcium title and history

The normalized title “Arcium, the Plagued Kingdom” occurs once among 442 scanned .zon files. Cloud Giant Kingdom, Sea Kingdom, and The Kingdom of Torg are lexical neighbors with separate titles and owners. Followed arcium.wld and arcium.zon histories reach the 2009 areas/ relocation and later same-stem changes, including the 2015 quest-item-flag update; no alternate-stem rename, distinct predecessor, or whole-zone replacement is established. The reciprocal Surface pair, source-only Surf approach, and typed reset dependencies are route or dependency evidence, not replacement evidence. Keep arcium DO NOT MOVE. See the [Arcium audit supplement](ARCIUM_AUDIT_REVIEW.md), [coverage row 12](ZONE_COVERAGE_REGISTER.csv#L12), and [title source](../../../areas/zon/arcium.zon#L2).

### Checkpoint 157 — Future Ceothia title and history

The normalized title Ceothia is shared by the three active phase keys ceothia, ceopast, and ceofutur. Their distinct Present, Past, and Future areas are separate period maps with separate contracts; the repeated title does not establish a duplicate map or replacement. Bounded same-stem ceofutur.wld and ceofutur.zon history reaches the 2009 areas/ tree move and later edits, including a 2013 WLD update and the 2015 quest-item flag pass. The reviewed history shows no alternate-stem rename, distinct predecessor, or whole-zone replacement. Keep ceofutur DO NOT MOVE and preserve all three phase keys. See the [Future Ceothia audit supplement](FUTURE_CEOTHIA_AUDIT_REVIEW.md), [coverage row 51](ZONE_COVERAGE_REGISTER.csv#L51), and the Present, Past, and Future [title sources](../../../areas/zon/ceothia.zon#L2).

### Checkpoint 158 — Treasure Caves title and history

The normalized title Treasure Caves is unique among 442 scanned .zon files and belongs to active stem dungeon. Bounded followed dungeon.wld and dungeon.zon history reaches the 2009 areas/ tree move and later same-stem edits, including 2011/2013 WLD changes and the 2015 quest-item flag pass. No alternate-stem rename, distinct predecessor, or whole-zone replacement was established. Keep dungeon DO NOT MOVE. See the [Treasure Caves audit supplement](TREASURE_CAVES_AUDIT_REVIEW.md), [coverage row 83](ZONE_COVERAGE_REGISTER.csv#L83), and [title source](../../../areas/zon/dungeon.zon#L2).

### Checkpoint 159 — Jindon title and history

The normalized title “Jindon the Deathwood Forest” belongs only to active stem `jin` among 442 scanned `.zon` files. Bounded `jin.wld` and `jin.zon` history shows same-stem edits and moves, including the 2009/2011 WLD moves and the 2015 ZON quest-item flag pass. No alternate-stem rename, distinct predecessor, or whole-zone replacement was established. The reciprocal Surface boundary and the source-only Surf approach are route records, not replacement evidence. Keep `jin` DO NOT MOVE. See the [Jindon audit supplement](JINDON_DEATHWOOD_AUDIT_REVIEW.md), [coverage row 139](ZONE_COVERAGE_REGISTER.csv#L139), and [title source](../../../areas/zon/jin.zon#L2).

### Checkpoint 160 — Lava Springs title and history

The normalized title “Lava Springs” belongs only to active stem `lava` among 442 scanned `.zon` files. Followed `lava.wld` and `lava.zon` histories reach the 2009 areas-tree move and later same-stem edits, including 2013, 2025, and 2026 changes. No alternate-stem rename, distinct predecessor, or whole-zone replacement was established. The reciprocal Surface boundary and source-only Surf approach are route records, not replacement evidence. Keep `lava` DO NOT MOVE. See the [Lava Springs audit supplement](LAVA_SPRINGS_AUDIT_REVIEW.md), [coverage row 155](ZONE_COVERAGE_REGISTER.csv#L155), and [title source](../../../areas/zon/lava.zon#L2).

### Checkpoint 161 — Lylr-Meop title and history

The normalized title “Lylr-Meop” belongs only to active stem `lylr` among 442 scanned `.zon` files. Followed `lylr.wld` and `lylr.zon` histories reach the 2009 areas-tree move and later same-stem edits, including the 2013 WLD update, 2014 bank change, 2015 quest-item flag pass, and 2016 bottle fix. No alternate-stem rename, distinct predecessor, or whole-zone replacement was established. The two one-way active Surface exits and two source-only Surf approaches are route records, not replacement evidence. Keep `lylr` DO NOT MOVE. See the [Lylr-Meop audit supplement](LYLR_MEOP_AUDIT_REVIEW.md), [coverage row 167](ZONE_COVERAGE_REGISTER.csv#L167), and [title source](../../../areas/zon/lylr.zon#L2).

### Checkpoint 162 — The Halfling Silver Mine title and history

The normalized title “The Halfling Silver Mine” belongs only to active stem `mining` among 442 scanned `.zon` files. Followed `mining.wld` and `mining.zon` paths begin with the same-stem August 2015 import and continue through later edits, including the 2025 cleanup. No alternate-stem rename, distinct predecessor, or whole-zone replacement was established within those bounded paths. The reciprocal Surface boundary, overlapping Alterian room VNUMs, nine source-only exits, and object #4900 collision are connectivity/dependency evidence, not replacement proof. Keep `mining` DO NOT MOVE and retain the existing Alterian/Surf candidates. See the [Mining audit supplement](MINING_AUDIT_REVIEW.md), [coverage row 175](ZONE_COVERAGE_REGISTER.csv#L175), and [title source](../../../areas/zon/mining.zon#L2).

### Checkpoint 163 — Forest of Mir title and history

The normalized title “Forest of Mir” belongs only to active stem mir among 442 scanned .zon files. Followed mir.wld and mir.zon histories include the 2009 areas/ move and later same-stem edits, including 2013 changes and 2016 spelling/bug fixes. No alternate-stem rename, distinct predecessor, or whole-zone replacement was established. The reciprocal Highway boundaries and active cross-area item/reset references are connectivity and dependency evidence, not replacement proof. Keep mir DO NOT MOVE. See the [Mir audit supplement](MIR_AUDIT_REVIEW.md), [coverage row 178](ZONE_COVERAGE_REGISTER.csv#L178), and [title source](../../../areas/zon/mir.zon#L2).

### Checkpoint 164 — The Shadow Forest title and history

The normalized title “The Shadow Forest” belongs only to active stem mist among 442 scanned .zon files. Followed mist.wld history reaches the 2009 areas/ move and same-stem 2011 and 2013 edits; mist.zon history includes the 2009 move and 2015 quest-item flag update. No alternate-stem rename, distinct predecessor, or whole-zone replacement was established in those bounded paths. The active Minizones/NewFor/Surface boundaries, three source-only approaches, and typed room/object namespace collision are connectivity and source-reference evidence, not replacement proof. Keep mist DO NOT MOVE. See the [Shadow Forest audit supplement](SHADOW_FOREST_AUDIT_REVIEW.md), [coverage row 180](ZONE_COVERAGE_REGISTER.csv#L180), and [title source](../../../areas/zon/mist.zon#L2).

### Checkpoint 165 — Mosswood title and history

The normalized title “Mosswood” belongs only to active stem moss among 442 scanned .zon files. Followed moss.wld history reaches the 2009 areas/ move and same-stem 2011, 2013, 2016, and 2026 edits; moss.zon history reaches the same move and same-stem 2011, 2013, 2015 quest-item flag, and 2016 typo changes. No alternate-stem rename, distinct predecessor, or whole-zone replacement was established in those bounded paths. The reciprocal Surface boundary and three source-only Surf approaches are route records, not replacement evidence. Keep moss DO NOT MOVE. See the [Mosswood audit supplement](MOSSWOOD_AUDIT_REVIEW.md), [coverage row 190](ZONE_COVERAGE_REGISTER.csv#L190), and [title source](../../../areas/zon/moss.zon#L2).

### Checkpoint 166 — The Reliquary Nexus of the Roper Den title and history

The normalized title “The Reliquary Nexus of the Roper Den” belongs only to active stem nexus_roper among 442 scanned .zon files. Followed nexus_roper.wld history reaches the 2009 areas/ move; nexus_roper.zon history adds a 2010 title fix and 2015 quest-item flag pass after that move. No alternate-stem rename, distinct predecessor, or whole-zone replacement was established within those bounded paths. The reciprocal Underdark boundary and the 52 non-overlapping plus 48 overlapping Duris3 records are route and map-identity evidence, not replacement proof. Keep nexus_roper DO NOT MOVE and preserve Duris3 as a separate source identity. See the [Roper Den audit supplement](NEXUS_ROPER_AUDIT_REVIEW.md), [coverage row 210](ZONE_COVERAGE_REGISTER.csv#L210), and [title source](../../../areas/zon/nexus_roper.zon#L2).

### Checkpoint 167 — Ny’Neth’s Stronghold title and history

The normalized title “Ny'Neth's Stronghold” belongs only to active stem `nyneth2` among 442 scanned `.zon` files; `nyneth3` has the distinct title “Ny'Neth's Stronghold Continued.” Followed `nyneth2.wld` history reaches the 2009 areas-tree move and a 2013 same-stem edit; `nyneth2.zon` history also includes a 2015 quest-item flag update. No alternate-stem rename, distinct predecessor, or whole-zone replacement was established in these bounded paths. The one-way White Harbor edge and the active portal/reset dependencies are route and dependency evidence, not replacement proof. Keep `nyneth2` DO NOT MOVE. See the [Ny’Neth’s Stronghold audit supplement](NYNETH2_AUDIT_REVIEW.md), [coverage row 218](ZONE_COVERAGE_REGISTER.csv#L218), and [title source](../../../areas/zon/nyneth2.zon#L2).

### Checkpoint 168 — Ny’Neth’s Stronghold Continued title and history

The normalized title “Ny'Neth's Stronghold Continued” belongs only to active stem `nyneth3` among 442 scanned `.zon` files. The separate `nyneth2` title omits “Continued” and belongs to an earlier campaign stage. Followed `nyneth3.wld` history reaches the 2009 areas-tree move and an opaque 2013 same-stem edit; `nyneth3.zon` history adds same-stem area tweaks, a 2015 quest-item flag pass, and a 2017 gear-value adjustment. These bounded paths establish no alternate-stem rename or whole-zone replacement. The two technical WLD inbounds and portal/reset references are route and dependency evidence, not replacement proof. Keep `nyneth3` DO NOT MOVE. See the [Ny’Neth’s Stronghold Continued audit supplement](NYNETH3_AUDIT_REVIEW.md), [coverage row 219](ZONE_COVERAGE_REGISTER.csv#L219), and [title source](../../../areas/zon/nyneth3.zon#L2).

### Checkpoint 169 — Orrak title and history

The normalized title “Orrak” belongs only to active stem `orrak` among 442 scanned `.zon` files. Followed `orrak.wld` history reaches the 2009 areas-tree move, an opaque 2013 `stuff` change, and a 2016 Vellel dimming fix; `orrak.zon` history includes the 2009 move, 2013 `stuff` work, the 2015 quest-item flag pass, an April 18, 2026 content change adding wandering ambassadors, and a May 1, 2026 line-ending-only change. No alternate-stem rename, distinct predecessor, or whole-zone replacement was established in these bounded paths. The reciprocal Surface boundary, source-only Surf approach, and typed reset dependencies are route and dependency evidence, not replacement proof. Keep `orrak` DO NOT MOVE. See the [Orrak audit supplement](ORRAK_AUDIT_REVIEW.md), [coverage row 225](ZONE_COVERAGE_REGISTER.csv#L225), and [title source](../../../areas/zon/orrak.zon#L2).

### Checkpoint 170 — Sea Kingdom title and history

The normalized title “Sea Kingdom” belongs only to active stem `seakngdm` among 442 scanned `.zon` files. Followed `seakngdm.wld` history reaches the 2009 areas-tree move and same-stem 2011/2013 edits; `seakngdm.zon` history reaches the same move, a 2013 same-stem edit, and the 2015 quest-item flag pass. No alternate-stem rename, distinct predecessor, or whole-zone replacement was established in these bounded paths. The reciprocal Plane Water boundary, one-way White Harbor inbound, and typed item/reset dependencies are route and dependency evidence, not replacement proof. Keep `seakngdm` DO NOT MOVE. See the [Sea Kingdom audit supplement](SEAKNGDM_AUDIT_REVIEW.md), [coverage row 262](ZONE_COVERAGE_REGISTER.csv#L262), and [title source](../../../areas/zon/seakngdm.zon#L2).

### Checkpoint 171 — Turolopolis Zoo title and history

The normalized title “Turolopolis Zoo” belongs only to active stem `turolzoo` among 442 scanned `.zon` files; the Ruins of Turolopolis (`willem`) is a separate title and map. Followed `turolzoo.wld` history reaches the 2009 areas-tree move and a 2013 same-stem change; `turolzoo.zon` history reaches the same move, a 2013 same-stem change, and the 2015 quest-item flag pass. No alternate-stem rename, distinct predecessor, or whole-zone replacement was established in these bounded paths. Its reciprocal Willem boundary and blocked cage route are connectivity evidence, not replacement proof. Keep `turolzoo` DO NOT MOVE. See the [Turolopolis Zoo audit supplement](TUROLZOO_AUDIT_REVIEW.md), [coverage row 319](ZONE_COVERAGE_REGISTER.csv#L319), and [title source](../../../areas/zon/turolzoo.zon#L2).

### Checkpoint 172 — Valley of Crushk title and history

The normalized title “Valley of Crushk” belongs only to active stem `valley_crushk` among 442 scanned `.zon` files. Followed `valley_crushk.wld` history reaches the 2009 areas-tree move, a 2011 same-stem edit, and an opaque 2013 `stuff` change; `valley_crushk.zon` history includes the same move, a 2013 change, and the 2015 quest-item flag pass. No alternate-stem rename, distinct predecessor, or whole-zone replacement was established in these bounded paths. Its active Mini Zones, Pharr Valley, and Surface routes plus the source-only Surf approach are connectivity evidence, not replacement proof. Keep `valley_crushk` DO NOT MOVE. See the [Valley audit supplement](VALLEY_CRUSHK_AUDIT_REVIEW.md), [coverage row 334](ZONE_COVERAGE_REGISTER.csv#L334), and [title source](../../../areas/zon/valley_crushk.zon#L2).

### Checkpoint 173 — The Forgotten Forest title and history

The normalized title “The Forgotten Forest” belongs only to active stem `forgotten_forest` among 442 scanned `.zon` files. Followed `forgotten_forest.wld` history reaches the 2009 areas-tree move and a 2011 same-stem change; `forgotten_forest.zon` history reaches the same move and 2015 quest-item-flag pass. No alternate-stem rename, distinct predecessor, or whole-zone replacement was established in these bounded paths. Its reciprocal active Surface boundary and source-only Surf approaches are route evidence, not replacement proof. Keep `forgotten_forest` DO NOT MOVE. See the [Forgotten Forest audit supplement](FORGOTTEN_FOREST_AUDIT_REVIEW.md), [coverage row 101](ZONE_COVERAGE_REGISTER.csv#L101), and [title source](../../../areas/zon/forgotten_forest.zon#L2).

### Checkpoint 174 — The Rice Fields title and history

The normalized title “The Rice Fields” belongs only to active stem `jademini` among 442 scanned `.zon` files. Followed `jademini.wld` history reaches the 2009 areas-tree move, same-stem 2011/2013 changes, the November 2025 world connection, an April 2026 change described as removing the IC3 entrance, and a May 2026 line-ending cleanup. `jademini.zon` history reaches the same 2009 move, the 2015 quest-item-flag pass, and 2025 connection work. No alternate-stem rename, distinct predecessor, or whole-zone replacement was established. The three reciprocal Surface pairs and inactive IC3/Surf approaches are route records, not replacement proof. Keep `jademini` DO NOT MOVE. See the [Rice Fields audit supplement](RICE_FIELDS_AUDIT_REVIEW.md), [coverage row 138](ZONE_COVERAGE_REGISTER.csv#L138), and [title source](../../../areas/zon/jademini.zon#L2).

### Checkpoint 175 — The Bandit Canyons title and history

The normalized title “The Bandit Canyons” belongs only to active stem `bandit` among 442 scanned `.zon` files; Bandit Camp (`banditca`) has a distinct title and source identity. Followed `bandit.wld` history reaches the 2009 areas-tree move, an opaque 2013 same-stem change, and a 2016 Olat door-keyword change; `bandit.zon` history reaches the same 2009 move, a 2013 edit, and the 2015 quest-item-flag pass. No alternate-stem rename, distinct predecessor, or whole-zone replacement was established. Its reciprocal Surface boundary and source-only Surf approach are route evidence, not replacement proof. Keep `bandit` DO NOT MOVE. See the [Bandit audit supplement](BANDIT_CANYONS_AUDIT_REVIEW.md), [coverage row 23](ZONE_COVERAGE_REGISTER.csv#L23), and [title source](../../../areas/zon/bandit.zon#L2).

### Barrow of the Quiosho title and path history

The active `elftomb` contributor at zone 206 has the normalized title `Barrow of the Quiosho`, unique among the 442 scanned current `.zon` titles. Followed `elftomb.wld` history stays on the same stem through the 2009 areas-tree move, 2011 edit, and 2013 edit; `elftomb.zon` history includes the 2015 quest-item-flag pass. The bounded WLD/ZON paths show no alternate-stem rename, distinct predecessor, or whole-zone replacement. Four non-overlapping inactive `surf.wld` records lead into current Barrow rooms, but remain source-only map declarations and do not establish a Barrow replacement. See the [Barrow of the Quiosho audit supplement](ELFTOMB_AUDIT_REVIEW.md).

### Keep of Evil title and path history

The active `evkeep` contributor at zone 448 has the normalized title `The Keep of Evil`, unique among the 442 scanned current `.zon` titles. Followed `evkeep.wld` history remains on the same stem through the 2009 areas-tree move, 2010 arena-room change, 2011 edits, and 2013 edit; `evkeep.zon` history includes the 2015 quest-item-flag pass. This bounded WLD/ZON review found no alternate-stem rename, distinct predecessor, or whole-zone replacement. One inactive `surf.wld` room points east to current Keep room #44801; that source-only link does not establish replacement. See the [Keep of Evil audit supplement](EVKEEP_AUDIT_REVIEW.md).

### New Cave city title and path history

The normalized title “New Cave city” belongs only to active stem new_cavecity among the 442 scanned current .zon titles. Followed new_cavecity.wld history reaches the 2009 areas-tree move, same-stem 2009 route edits, and a 2013 edit; new_cavecity.zon history reaches the same move, same-stem 2011/2013 changes, and the 2015 quest-item-flag pass. The bounded WLD/ZON paths show no alternate-stem rename, distinct predecessor, or whole-zone replacement. The active reciprocal Surface/Underdark boundaries and one source-only Surf approach are route evidence, not replacement proof. See the [New Cave city audit supplement](NEW_CAVECITY_AUDIT_REVIEW.md).

### Lost City of Tikitzopl title and path history

The normalized title “Lost City of Tikitzopl” belongs only to active stem tikit among the 442 scanned current .zon titles. Lost Temple of Tikitzopl (tikitt) has a distinct title and separate zone/VNUM ranges; its reciprocal boundary, item recipes, and portal do not establish a replacement. Followed tikit.wld/tikit.zon histories remain on the same stem through the 2009 areas-tree move and later 2010–2015 edits. The bounded WLD/ZON paths show no alternate-stem rename, distinct predecessor, or whole-zone replacement. The source-only Surf approach is route evidence, not replacement proof. See the [Lost City audit supplement](TIKIT_AUDIT_REVIEW.md).

### Checkpoint 181 — Ako Village title and path history

The normalized title `Ako Village` belongs only to active stem `ako` among 442 scanned current `.zon` titles; the title scan found no lexical candidates. Followed `ako.wld` history stays on the same stem through the 2009 areas-tree move, the 2011 typo/content edits, and the 2013 change. Followed `ako.zon` history stays on the same stem through the 2009 move, the 2013 changes, and the 2015 quest-item-flag pass. These bounded title and WLD/ZON paths show no alternate-stem rename, distinct predecessor, or whole-zone replacement. The one-way active Surface edge and separate source-only Surf approach are route evidence, not replacement evidence. See the [Ako Village audit supplement](AKO_AUDIT_REVIEW.md), [coverage row 5](ZONE_COVERAGE_REGISTER.csv#L5), and [title source](../../../areas/zon/ako.zon#L2).

### Checkpoint 182 — An Ancient Bridge title and path history

The normalized title `An Ancient Bridge` belongs only to active stem `ancientb` among 442 scanned current `.zon` titles; the title scan found no lexical candidates. Followed `ancientb.wld` and `ancientb.zon` histories stay on the same stem through the 2009 areas-tree move and later same-stem changes. The 2014 commit `dc4aa9310` says the zone was made “no gate/port/etc.” to stop cheese and changes room-flag values; it does not establish a different map owner or replacement. These bounded title and WLD/ZON paths show no alternate-stem rename, distinct predecessor, or whole-zone replacement. The active one-way Surface exits and inactive approaches are route records, not replacement evidence. See the [Ancient Bridge audit supplement](ANCIENTB_AUDIT_REVIEW.md), coverage row [7](ZONE_COVERAGE_REGISTER.csv#L7), and current [title source](../../../areas/zon/ancientb.zon#L2).

### Checkpoint 183 — Ant Forest title and path history

The normalized title `Ant Forest` belongs only to active stem `ants` among 442 scanned current `.zon` titles, with no lexical candidates. Followed `ants.wld`/`ants.zon` histories stay on the same stem through the 2009 areas-tree move and 2013 content edit. The 2015 quest-item flag pass changes only the zone-editor comment in `ants.zon`; it does not establish a replacement map. These bounded title and same-stem paths show no alternate-stem rename, distinct predecessor, or whole-zone replacement. The reciprocal Surface pair and separate source-only Surf approach are route evidence, not replacement evidence. See the [Ant Forest audit supplement](ANTS_AUDIT_REVIEW.md), coverage row [8](ZONE_COVERAGE_REGISTER.csv#L8), and current [title source](../../../areas/zon/ants.zon#L2).

### Checkpoint 184 — Arachdrathos Wilderness title and path history

The normalized title `Arachdrathos Wilderness` belongs only to active stem `arac_wild` among 442 scanned current `.zon` titles. Nearby names—`Arachdrathos - Drow City` (`arac-web`), `Arachdrathos Guilds` (`aracguil`), and `Llzazan Ghetto of Arachdrathos`—refer to distinct neighboring areas. Followed `arac_wild.wld` and `arac_wild.zon` histories stay on the same stem through the 2009 areas-tree move and later same-stem edits. A 2013 WLD change removes older Surface exit records and rewires the retained boundaries to current Surface rooms #589915 and #588318; a 2015 ZON change updates only the zone-editor comment. These bounded title and history paths show no alternate-stem rename, distinct predecessor, or whole-zone replacement. See the [Arachdrathos Wilderness audit supplement](ARAC_WILD_AUDIT_REVIEW.md), coverage row [10](ZONE_COVERAGE_REGISTER.csv#L10), and current [title source](../../../areas/zon/arac_wild.zon#L2).

### Checkpoint 185 — Arachdrathos Guilds title and path history

After color-code normalization, `Arachdrathos Guilds` is unique among 442 scanned current `.zon` titles. `Arachdrathos - Drow City` (`arac-web`), `Arachdrathos Wilderness` (`arac_wild`), and `Llzazan Ghetto of Arachdrathos` are distinct neighboring contributors. Followed WLD/MOB/OBJ/ZON history stays on the `aracguil` stem through the 2009 areas-tree move and same-stem changes. The September 2009 history toggles Drow guild resets; the 2013 commit revises MOB/OBJ data while leaving WLD/ZON content unchanged; the 2015 pass updates quest-item flags; and the 2025 ZON header sets reset mode 0. These edits do not establish a rename or predecessor. No alternate-stem rename, distinct predecessor, or whole-zone replacement was established. See the [Arachdrathos Guilds audit supplement](ARACGUIL_AUDIT_REVIEW.md).

### Checkpoint 186 — Ard'gral title and path history

After color-code normalization, `Ard'gral, plane of dreams` is unique among 442 scanned current `.zon` titles, with no other lexical candidate. Followed WLD/MOB/OBJ/ZON history remains on the `ardgral` stem through the 2009 areas-tree move, 2011 fixes, a 2013 same-stem change, and the 2015 quest-item flag pass. In 2016, commit `d92a697fe` added the Deep Ravine rift as object #30413 and moved the previous near-invisible Astral rift to #30414; `c884c1c2f` corrected descriptions afterward. These bounded paths establish no alternate-stem rename, distinct predecessor, or whole-zone replacement. Dormant Yggdrasil has a distinct title and source component; its overlapping VNUMs are a collision, not replacement evidence. Keep `ardgral` DO NOT MOVE. See the [Ard'gral audit supplement](ARDGRAL_AUDIT_REVIEW.md), [coverage row 13](ZONE_COVERAGE_REGISTER.csv#L13), and current [title source](../../../areas/zon/ardgral.zon#L2).

### Checkpoint 187 — Arena title and path history

After color-code normalization, `CTF Tournament` is unique among 442 current `.zon` titles. Followed `arena.wld`, `arena.obj`, and `arena.zon` histories remain on the same stem through the 2009 areas-tree move, the 2012 object schema/read-write update, the 2013 same-stem object-data adjustment, and the 2015 quest-item flag pass. No alternate-stem rename, distinct predecessor, duplicate title, or whole-zone replacement was established. The title/default-mode mismatch and Undead return question are runtime/design issues, not evidence of a replacement family. Keep active `arena` DO NOT MOVE; see the [Arena audit supplement](ARENA_AUDIT_REVIEW.md), [coverage row 14](ZONE_COVERAGE_REGISTER.csv#L14), and current [title source](../../../areas/zon/arena.zon#L2).

### Checkpoint 188 — Astral Plane, Side Areas title and path history

The normalized title `Astral Plane, Side Areas` is unique among 442 current
`.zon` titles. Followed `astral_areas.wld` history includes a 2010 same-stem
door-flag edit and a 2013 same-stem edit; its ZON history records removal of
Heavens object #359 from room #19861 in 2011, described as removing an “epic
stone.” These bounded paths establish no whole-zone retirement. The distinct
39-room Githyanki fortress may be the “old gith fort” later named in the
`githyanki_fortress` addition, but the commit does not identify it and both
areas remain active. Keep `astral_areas` DO NOT MOVE; see the [Astral Side
Areas audit supplement](ASTRAL_AREAS_AUDIT_REVIEW.md), [coverage row
16](ZONE_COVERAGE_REGISTER.csv#L16), and current [title
source](../../../areas/zon/astral_areas.zon#L2).

### Checkpoint 189 — Astral Main title and path history

The normalized title `The Astral Plane` is unique among 442 current `.zon`
titles. Followed `astral_main.wld`, `.mob`, `.obj`, and `.zon` histories stay
on the same stem through the 2009 area-tree move and later content, balance,
and reset edits. Bounded history shows no alternate-stem rename, distinct
predecessor, or whole-zone replacement. The 39-room Githyanki fortress in
Astral Side Areas is a possible but unconfirmed referent for a 2010 “old gith
fort” note; the source does not name Astral Side Areas, and both contributors
remain active. Keep `astral_main` DO NOT MOVE. See the [Astral Main audit
supplement](ASTRAL_MAIN_AUDIT_REVIEW.md), [coverage row
17](ZONE_COVERAGE_REGISTER.csv#L17), and current [title
source](../../../areas/zon/astral_main.zon#L2).

### Checkpoint 190 — Astral Plane, Tiamat title and path history

The normalized title `Astral Plane, Tiamat` is unique in the current
442-title `.zon` scan. Followed WLD, MOB, OBJ, and ZON paths show the 2009 move
into the current area tree and later same-stem content, balance, typo, key, and
item-flag edits. ZON history includes a Tiamat key correction in 2010 and the
addition of guardian mobs to vaults in 2009. No alternate-stem rename, distinct
predecessor, or whole-zone replacement was found. `tiamat` (“Tiamat”) and
`azhural` (“The Lair of Tiamat”) are distinct active areas; the separate
`tiamats_lair` history associated with Azhural does not identify
`astral_tiamat` as its predecessor. Keep `astral_tiamat` active and do not add
an attic candidate row. See the [Astral Tiamat audit supplement](ASTRAL_TIAMAT_AUDIT_REVIEW.md),
[coverage row 18](ZONE_COVERAGE_REGISTER.csv#L18), and [current title
source](../../../areas/zon/astral_tiamat.zon#L2).

### Checkpoint 191 — Asylum title and path history

The color-stripped title `The Temple of Lizizania` is unique in the current
442-title `.zon` scan. Followed WLD, MOB, OBJ, and ZON histories retain the
same `asylum` stem from the 2009 move into the current `areas/` tree through
later same-stem typo, text, item-flag, and refactor edits. Bounded history
shows no alternate-stem rename, distinct predecessor, or whole-zone
replacement. Room #9273 says a connecting passage was added later; this is
local architectural history, not evidence of a zone replacement. Keep active
`asylum` DO NOT MOVE. See the [Asylum audit supplement](ASYLUM_AUDIT_REVIEW.md),
[coverage row 19](ZONE_COVERAGE_REGISTER.csv#L19), and current [title
source](../../../areas/zon/asylum.zon#L2).

### Checkpoint 192 — Avernus title and path history

The normalized active title “The Plane of Avernus” is unique among 351 active
contributors. The inactive, unregistered `plane_avernus` source uses the same
normalized title, but its zone #135 has only rooms #135 and #136 and has no
mobile or object records. An all-inactive-WLD scan found no VNUM overlap with
active Avernus and no inactive exit approach into it. This is a repeated title
on a separate small component, not replacement evidence.

Followed `avernus` WLD/MOB/OBJ/ZON histories retain the same stem through the
2009 move into `areas/`, subsequent 2010/2013 edits, and the 2015 quest-item
flag pass; the shop history includes the 2014 stock/reset-correctness cleanup.
Followed `plane_avernus` history reaches its 2009 path move and 2011 tracking
addition. The bounded paths show no alternate-stem rename, distinct
predecessor, or whole-zone replacement. Keep active `avernus` DO NOT MOVE and
keep candidate `plane_avernus` RETAIN IN PLACE. See the [Avernus audit
supplement](AVERNUS_AUDIT_REVIEW.md), [coverage row
20](ZONE_COVERAGE_REGISTER.csv#L20), [candidate row
83](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L83), and active [title
source](../../../areas/zon/avernus.zon#L2).

### Checkpoint 193 — Black Tunnels title and path history

The normalized `Black Tunnels` title is unique in the current `.zon` title
scan. Followed WLD, MOB, OBJ, and ZON histories retain the `black_tunnels`
stem through the 2009 move into `areas/`; bounded later history includes the
2013 area edit, the 2015 quest-item flag pass, and same-stem field/text edits.
No alternate-stem rename, distinct predecessor, or whole-zone replacement was
found. Keep active `black_tunnels` DO NOT MOVE and add no attic candidate row.
See the [Black Tunnels audit supplement](BLACK_TUNNELS_AUDIT_REVIEW.md),
[coverage row 31](ZONE_COVERAGE_REGISTER.csv#L31), and current [title
source](../../../areas/zon/black_tunnels.zon#L2).

### Checkpoint 194 — Bloody Plains title and path history

The normalized title “The Bloody Plains” is unique in the current 442-title
`.zon` scan. Followed WLD, MOB, OBJ, and ZON histories retain the
`bloody_plains` stem. The WLD path includes the 2009 move into `areas/`
(`642da5400`) and a 2013 content edit (`71d731c5b`); MOB and ZON also include
the 2015 quest-item flag pass (`f18992b8f`), while MOB and OBJ include the
2012 field-format edit (`bb0d59425`). These are same-stem changes. The bounded
paths show no alternate-stem rename, distinct predecessor, or whole-zone
replacement. Keep active `bloody_plains` DO NOT MOVE and add no attic candidate
row. See the [Bloody Plains audit supplement](BLOODY_PLAINS_AUDIT_REVIEW.md),
[coverage row 33](ZONE_COVERAGE_REGISTER.csv#L33), and current [title
source](../../../areas/zon/bloody_plains.zon#L2).

### Checkpoint 195 — BogenTok title and path history

The normalized title `BogenTok` is unique in the current 442-title `.zon`
scan. Followed WLD, MOB, OBJ, ZON, and SHP histories retain the `bogentok`
stem. The paths include the 2009 move into `areas/` (`642da5400`), the 2011
WLD spelling edit (`fac285bc7`), the 2013 WLD content edit (`71d731c5b`),
and the 2014 shop cleanup (`1c59fb690`). MOB and OBJ include the 2012
field-format edit (`bb0d59425`); MOB and ZON include the 2015 quest-item flag
pass (`f18992b8f`). These are same-stem edits. The bounded paths show no
alternate-stem rename, distinct predecessor, or whole-zone replacement. Keep active
`bogentok` DO NOT MOVE and add no attic candidate row. See the [BogenTok audit
supplement](BOGENTOK_AUDIT_REVIEW.md), [coverage row 34](ZONE_COVERAGE_REGISTER.csv#L34),
and current [title source](../../../areas/zon/bogentok.zon#L2).

### Checkpoint 196 — Vella's Bordello title and path history

The normalized scan of all 442 current `.zon` titles finds “Vella's
Bordello” only on active `bordello`
([title](../../../areas/zon/bordello.zon#L2)). Followed WLD, MOB, OBJ, ZON,
and SHP histories retain the same stem. The bounded paths include the 2009
move into `areas/`, a 2011 WLD typo edit, 2012 MOB/OBJ format updates, 2013
area edits, a 2014 shop cleanup, a 2015 quest-item flag pass, and 2016
spelling fixes. These show same-stem maintenance; no alternate-stem rename,
distinct predecessor, or whole-zone replacement was found. Keep active
`bordello` **DO NOT MOVE** and add no attic-candidate row. See the
[Bordello audit supplement](BORDELLO_AUDIT_REVIEW.md),
[coverage row 35](ZONE_COVERAGE_REGISTER.csv#L35), and the
[current title source](../../../areas/zon/bordello.zon#L2).

### Buildings and generated outpost interiors

The normalized active `.zon` title “Buildings” has one owner among the 351
active contributors. Followed WLD history begins at the 2009 repository source
import; MOB, OBJ, and ZON history retains the `buildings` stem through outpost
prototype additions and same-path maintenance. The reviewed paths show no
alternate-stem rename, distinct predecessor, or whole-zone replacement. The
Human Outpost, Undead Outpost, and Newbie Outpost have separate titles and
source identities; they are not duplicate copies of this shared template
area. See the [Buildings audit supplement](BUILDINGS_AUDIT_REVIEW.md) and
[active title](../../../areas/zon/buildings.zon#L2).

The code's KK exterior anchor is active Surface room #628856. Unregistered
`newbiemaps.wld` and `surface2011-temp.wld` have different same-numbered room
records, but neither is loaded from `areas/AREA`. These parallel map records
do not establish a Buildings predecessor or replacement. Keep active
`buildings` **DO NOT MOVE**.


### Checkpoint 199 — CBL Hall and Monster Island copies

The normalized active title “The Guildhall of Clan BloodLust” is unique among
351 active contributors. Followed CBL Hall WLD/MOB/OBJ/ZON/SHP paths begin at
the 2009 import and show same-stem maintenance; they establish no alternate-
stem rename, distinct predecessor, or whole-zone replacement.

Unregistered `Monsteri` and `cbl_outpost2` use the same “CBL Guildhall Monster
Island” `.zon` title, room VNUMs #1000–#1004, mobile VNUMs #1000–#1003, and
matching four-mobile reset placements. The five Monsteri room descriptions
match active `cbl_hall` #29820–#29824 after color-token and whitespace
normalization; four titles are exact. `cbl_outpost2` has a generic first title
and formatting variants. The active four-mobile Monster Island lineup has
corresponding roles, but exact mobile-prototype equality has not been
established. Neither unregistered source is listed in `areas/AREA`; no active
WLD edge targets #1000–#1004. Tracked history does not prove which source is
canonical, or that either source replaced the other or active CBL rooms.

Keep active `cbl_hall` **DO NOT MOVE**. Keep `cbl_outpost2` candidate row
[24](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L24) and `Monsteri` candidate row
[72](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L72) **RETAIN IN PLACE** until the
owner resolves source/story lineage and typed, editor/tool, and persisted
references. See the [CBL Hall audit supplement](CBL_HALL_AUDIT_REVIEW.md),
[coverage row 48](ZONE_COVERAGE_REGISTER.csv#L48), and [source-story
dossier](../zone-stories/CBL_HALL.md).

### Checkpoint 200: Celestia title and path history

The `.zon` title “Celestial Plane” normalizes uniquely among the 351 active
contributors and all 442 current `.zon` sources. Followed Celestia WLD, MOB,
OBJ, and ZON histories retain the `celestia` stem back to the 2009 repository
import. Two tracked 2014 changes move the Celestia stone and lock the vault to
Malevolence, then move the ioun and artifact resets to Malevolence (commits
`5d3addfd8` and `d6533d357`). These are same-stem edits; the bounded histories
show no alternate-stem rename, distinct predecessor, or whole-zone replacement.
No unregistered `.zon` title repeats “Celestial Plane”; the inactive-WLD and
external MOB/OBJ definition scans found no Celestia room overlap or duplicate
local prototype definition.

Keep active `celestia` **DO NOT MOVE**. The external object #63 VNUM collision
concerns prototype and reset ownership; it does not change Celestia’s source
identity. See the [Celestia audit supplement](CELESTIA_AUDIT_REVIEW.md),
[coverage row 49](ZONE_COVERAGE_REGISTER.csv#L49), and [source-story
dossier](../zone-stories/CELESTIA.md).

### Checkpoint 201: Charing title and path history

The normalized `.zon` title “Charing” is unique among all 442 current zone
sources. Followed WLD, MOB, OBJ, ZON, and SHP histories retain the `charing`
stem back to its 2009 move into `areas/`; no alternate-stem predecessor or
whole-zone replacement is established.

The history contains a partial content move. Commit `06d31630e` moved the
Charing Quartermaster prototype from #45095 to `siege` mobile #401030 and
changed Charing’s center reset to use it. Commit `7ddf5432f` retired `siege`,
removed its files, and deleted that Charing reset. An exact current source scan
finds no `#401030` reference under `areas/` or `src/`. This records the
Quartermaster’s lineage and retirement, not a replacement of Charing by
`siege`; keep zone number #4010 reserved as retired. The owner should confirm
whether the NPC’s removal was intentional. See the [Charing audit
supplement](CHARING_AUDIT_REVIEW.md), [coverage row
55](ZONE_COVERAGE_REGISTER.csv#L55), and the [historical `siege` row
477](ZONE_COVERAGE_REGISTER.csv#L477).

### Checkpoint 202: Cityruin title and path history

The normalized `.zon` title “The Ruins of Tharnadia's Old Quarter” is unique
among all 442 current zone sources. The `areas/AREA` note “kharne” does not
match a current title or a compiled alias found in the bounded source search.
Tharnadian Ruin is the paired neighboring zone; active Tharnadia is a separate
source. The current files and descriptions establish no replacement relation
among these zones.

Followed WLD, MOB, OBJ, ZON, and QST histories retain the `cityruin` stem from
the 2009 move into `areas/`. Later edits change record serialization, mobile
and object fields, and quest-item flags. The bounded paths show no alternate
stem, retirement, or whole-zone replacement. The shared item dependencies in
Tharnadian Ruin QST/story sources establish a quest relationship, not a
replacement. Keep active `cityruin` **DO NOT MOVE**. See the [Cityruin audit
supplement](CITYRUIN_AUDIT_REVIEW.md), [coverage row
58](ZONE_COVERAGE_REGISTER.csv#L58), and [source dossier](../zone-stories/CITYRUIN.md).

### Checkpoint 203: cyrictunnel title and history

The normalized title “The Tunnels of Cyric the Destroyer” is unique among
442 current zone sources. Black Tunnels, Tunnels of Payang, and The Twisting
Tunnels of the Durian Underdark are separate titles. Surface’s Mountains of
Cyric descriptions and the active boundary do not establish a predecessor or
replacement relationship.

Followed WLD, MOB, OBJ, and ZON histories retain the cyrictunnel stem through the
2009 area-tree path move. Later same-stem edits affect mobile fields, spelling,
serialization, quest-item flags, and an ANSI correction in 2026. No
alternate-stem predecessor, retirement, or whole-zone replacement is
established. Keep active cyrictunnel DO NOT MOVE. The historical QST/AREA
listing without a local QST file remains an owner question; it does not
establish a retired or replaced quest. See the [cyrictunnel audit
supplement](CYRICTUNNEL_AUDIT_REVIEW.md), [coverage row
68](ZONE_COVERAGE_REGISTER.csv#L68), and [source dossier](../zone-stories/CYRIC_TUNNELS.md).

### Checkpoint 204: Dark Forest title and history

The normalized .zon title Dark Forest is unique among 442 current zone
sources. Darkfall Forest is a separate active zone (#953), while similarly
worded Surface room names describe local terrain. The compiled comment
darkfall_forest labels an archer binding that resolves by exact VNUM to active
dark_forest mobile #75013. That naming mismatch is an owner question; it does
not establish a replacement or duplicate zone.

Followed WLD, MOB, OBJ, and ZON histories retain the dark_forest stem through the
2009 area-tree path move. Later edits remain on the same stem; no alternate-stem
predecessor, retirement, or whole-zone replacement is established. The
historical QST/AREA listing without a local QST remains unresolved separately.
Keep active dark_forest DO NOT MOVE. See the [dark_forest audit
supplement](DARK_FOREST_AUDIT_REVIEW.md), [coverage row
69](ZONE_COVERAGE_REGISTER.csv#L69), and [source dossier](../zone-stories/DARK_FOREST.md).

### Checkpoint 205: Yuan Ti2 title and history

After color stripping and normalization, “Village of the Damned” is unique
among the 442 current `.zon` titles. Followed WLD, MOB, OBJ, ZON, and SHP
histories retain the `yuan_ti2` stem through the 2009 area-tree path move
(`642da5400`) and later same-stem edits. No alternate-stem predecessor,
retirement, or whole-zone replacement is established.

Active `yuan_ti` is a separately titled neighboring contributor. Its room
#80554 directly connects to `yuan_ti2` room #80671; the neighboring boundary
establishes a route between the areas, not a replacement relationship. The
QST area index lists `yuan_ti2`, but no local `.qst` file is present. Keep
active `yuan_ti2` **DO NOT MOVE**. See the [Yuan Ti2 audit
supplement](YUAN_TI2_AUDIT_REVIEW.md), [coverage row
352](ZONE_COVERAGE_REGISTER.csv#L352), and [source dossier](../zone-stories/YUAN_TI2.md).

### Checkpoint 206: Outpost title and history

After color stripping and normalization, The Human Outpost is unique among
the 442 current `.zon` titles. Other titles that include “Outpost” are
distinct. The active Buildings zone #978 and the generic player-built outpost
subsystem have separate source identities; neither establishes an alias or
replacement for outpost #821.

Followed WLD, MOB, OBJ, ZON, QST, and SHP histories retain the outpost stem
through the 2009 area-tree move commit `642da5400` and later same-stem edits.
No alternate-stem predecessor, retirement, or whole-zone replacement is
established. Keep active outpost **DO NOT MOVE**. See the [Outpost audit
supplement](OUTPOST_AUDIT_REVIEW.md), [coverage row
227](ZONE_COVERAGE_REGISTER.csv#L227), and [source dossier](../zone-stories/OUTPOST.md).

### Checkpoint 207: Lab title and history

After color stripping and normalization, The Swamp Laboratory of Khul'Lor is
unique among 442 current `.zon` titles. `lab` (#823) and the active
`labyrinth` (#50, Labyrinth of No Return) are separate source identities.
Followed WLD, MOB, OBJ, and ZON histories retain the `lab` stem through the
2009 area-tree move commit `642da5400` and later same-stem edits, including a
2010 vault-room correction. No alternate-stem predecessor, retirement, or
whole-zone replacement is established.

Active Desert source also names the Abandoned Mines of Khul'Lor and describes
the mage's tower, citadel, and lingering spirit. That authored text raises an
identity and chronology question against the living Khul'Lor reset in `lab`;
it does not establish a source-file replacement. Keep active `lab` **DO NOT
MOVE**. See the [Lab audit supplement](LAB_AUDIT_REVIEW.md), [coverage row
153](ZONE_COVERAGE_REGISTER.csv#L153), and [source
dossier](../zone-stories/LAB.md).

### Checkpoint 208: Kuo-Toa Temple title and history

After color stripping and normalization, The Temple of Blibdoolpoolp is unique among the 442 current ZON titles. The altar in local room #88227 spells the deity “Blipdoolpoolpoo.” Gold Hall #40589 uses that spelling in its altar title and “Blipoodoopoo” in the room description; Underworld #4457 depicts a Kuo-Toan god without naming it. The nearby Underworld boundary establishes adjacency, while these textual parallels do not establish a replacement or one canonical divine spelling.

Followed WLD, MOB, OBJ, and ZON histories retain the kuotoa stem through the 2009 area-tree path move (642da5400) and later same-stem edits. A 2016 mobile edit sets the Kuo-Toa race. No alternate-stem predecessor, retirement, or whole-zone replacement is established. Keep active kuotoa DO NOT MOVE. See the [Kuo-Toa Temple audit supplement](KUOTOA_AUDIT_REVIEW.md), [coverage row 151](ZONE_COVERAGE_REGISTER.csv#L151), and [source dossier](../zone-stories/KUOTOA.md).

### Checkpoint 209: UDMini title and history

After color stripping and normalization, The Minizones of the Underdark is unique among the 442 current ZON titles. Followed WLD, MOB, OBJ, and ZON histories retain the udmini stem through the 2009 area-tree path move (642da5400) and later same-stem edits. No alternate-stem predecessor, retirement, or whole-zone replacement is established. Keep active udmini DO NOT MOVE. See the [UDMini audit supplement](UDMINI_AUDIT_REVIEW.md), [coverage row 322](ZONE_COVERAGE_REGISTER.csv#L322), and [source dossier](../zone-stories/UDMINI.md).

### Checkpoint 210: Morg's Place title and source history

After color stripping and normalization, “Morg's Place” is unique among the 442 current ZON titles. Followed WLD, MOB, OBJ, and ZON histories remain on the `morg` stem through the 2009 area-tree move and later same-stem edits through 2016. No alternate-stem predecessor, retirement, or whole-zone replacement is established.

UnderDark WLD room #845236 is titled “Morg's Palace” and connects to the same chasm #844836 that borders Morg's Place. It has no description. The shared approach makes the naming relationship worth confirming, but does not identify the room as the tavern or prove a replacement. Keep active `morg` DO NOT MOVE. See the [Morg's Place audit supplement](MORG_AUDIT_REVIEW.md), [coverage row 188](ZONE_COVERAGE_REGISTER.csv#L188), and [source dossier](../zone-stories/MORG.md).

### Checkpoint 211: Fog Enshrouded Wood title and source history

After color stripping and normalization, “The Fog Enshrouded Wood” is unique among the 442 current .zon titles. WLD room #90046 is titled “The Fog Enshrouded Forest” and its description continues toward an unregistered Duris3 target; it is an internal room label, not evidence of a second contributor. Followed WLD, MOB, OBJ, and ZON histories stay on the foggy_woods stem from the 2009 area-tree move through later same-stem edits, with latest history in 2013 (WLD), 2015 (MOB), and 2017 (OBJ and ZON). No alternate-stem predecessor, retirement, or whole-zone replacement is established.

The unregistered nexus56 reset and shop references to Foggy Woods object VNUMs do not establish a replacement; nexus56 is absent from the active area index. Keep active foggy_woods **DO NOT MOVE**. See the [Foggy Woods audit supplement](FOGGY_WOODS_AUDIT_REVIEW.md), [coverage row 100](ZONE_COVERAGE_REGISTER.csv#L100), and [source dossier](../zone-stories/FOGGY_WOODS.md).

### Checkpoint 212: Pit of Dragons title and source history

After stripping color codes and normalizing, “Pit of Dragons” is unique among the 442 current `.zon` titles. Followed WLD, MOB, OBJ, and ZON histories remain on the `dracopit` stem through the 2009 area-tree move; latest file history is 2013 (WLD), 2013 (MOB), 2013 (OBJ), and 2015 (ZON). No alternate-stem predecessor, retirement, or whole-zone replacement is established.

The active Caertannad portal targets Dracopit room #15907, and Mountain Castle supplies the key used by its hallway locks. Those active links are dependencies, not evidence that either zone replaces Dracopit. Keep active `dracopit` **DO NOT MOVE**. See the [Dracopit audit supplement](DRACOPIT_AUDIT_REVIEW.md), [coverage row 78](ZONE_COVERAGE_REGISTER.csv#L78), and [source dossier](../zone-stories/DRACOPIT.md).

### Checkpoint 213: Dragonnia title and source history

After stripping color codes and normalizing, “Dragonnia” is unique among the 442 current `.zon` titles. WLD, OBJ, and ZON histories stay on the `dragonnia` stem; the 2016 same-stem commit renumbered several keys from the 6700s to the 6800s. The latest MOB history is an August 2026 same-stem cleanup of stat values to address rebuild warnings. No alternate-stem predecessor, retirement, or whole-zone replacement is established.

The outside heart exchanges, Heavens object resets, compiled local prototype references, active Realm and Winterhaven WLD arrivals, and source-only Surf and Underworld2 approaches are dependencies on Dragonnia, not evidence of a replacement. Keep active `dragonnia` **DO NOT MOVE**. See the [Dragonnia audit supplement](DRAGONNIA_AUDIT_REVIEW.md), [coverage row 79](ZONE_COVERAGE_REGISTER.csv#L79), and [source dossier](../zone-stories/DRAGONNIA.md).

### Checkpoint 214: Dreggan Woods title and source history

After color stripping and normalization, “Dreggan Woods” is unique among 442 current `.zon` titles. Followed WLD, MOB, OBJ, and ZON histories remain on the `dreggan_wood` stem; latest file history is 2013 (WLD), August 2026 (MOB rebuild-warning cleanup), 2010 (OBJ), and 2015 (ZON). No alternate-stem predecessor, retirement, or whole-zone replacement is established.

The active Ethereal Plane and Shadamehr Keep boundaries, Shadamehr vault-key dependency, Alatorin lily exchange, and source-only Brass route note do not indicate a replacement. Keep active `dreggan_wood` **DO NOT MOVE**. See the [Dreggan audit supplement](DREGGAN_WOOD_AUDIT_REVIEW.md), [coverage row 81](ZONE_COVERAGE_REGISTER.csv#L81), and [source dossier](../zone-stories/DREGGAN_WOODS.md).

### Checkpoint 215: The End of the World title and source history

After color stripping and normalization, “The End of the World” is unique among the 442 current `.zon` titles. The WLD, ZON, and QST histories include a same-stem 2013 update; the MOB and OBJ terminal files are present at the 2009 area-tree move. The four replicated dealer shops were added to `end.shp` in September 2026. No alternate-stem predecessor, retirement, or whole-zone replacement is established.

The active `end` entry is a build marker rather than a playable zone. Its title and the four shared dealer locations do not identify a replaced area. Keep active `end` **DO NOT MOVE** at the end of the source roster so the world marker and final shop-catalog records remain in place. See the [End audit supplement](END_OF_THE_WORLD_AUDIT_REVIEW.md), [coverage row 89](ZONE_COVERAGE_REGISTER.csv#L89), and [source dossier](../zone-stories/THE_END_OF_THE_WORLD.md).

### Checkpoint 216: The Fortress of Dreams title and source history

After color stripping and normalization, “The Fortress of Dreams” is unique among 442 current .zon titles. Followed history stays on the eth2 stem: WLD through 2014-08-28, MOB through the August 2026 rebuild-warning cleanup, OBJ through 2015-11-10, and ZON through 2016-02-24. No local QST, SHP, or story sidecar history exists because these files are absent. No alternate-stem predecessor, retirement, or whole-zone replacement is established.

The linked Ethereal Main area has a distinct title and a type-25 portal that targets eth2. This source relationship identifies neighboring travel routes; the reviewed history does not establish a replacement. Keep active eth2 **DO NOT MOVE**. See the [Fortress of Dreams audit supplement](ETH2_AUDIT_REVIEW.md), [coverage row 91](ZONE_COVERAGE_REGISTER.csv#L91), and [source dossier](../zone-stories/THE_FORTRESS_OF_DREAMS.md).


### Checkpoint 217: The Ethereal Plane title and source history

After color-code removal and normalization, The Ethereal Plane is unique among all 442 current ZON titles. Followed same-stem histories remain on ethereal_main: WLD and OBJ through 2013-11-13; MOB and ZON through 2016-10-31. The 2009 source-tree move appears in the histories, and the later MOB cleanup removes the mount flag from nightmares. No alternate-stem predecessor or explicit replacement is established by the checked-in title and followed histories.

The active Dream and Dreggan boundaries, inbound Prison exits, fixed and random pool destinations, and the cross-area QST and reset references document dependencies on this plane. They do not identify a replacement. Keep active ethereal_main DO NOT MOVE. See the [Ethereal Main audit supplement](ETHEREAL_MAIN_AUDIT_REVIEW.md), [coverage row 92](ZONE_COVERAGE_REGISTER.csv#L92), and [source dossier](../zone-stories/THE_ETHEREAL_PLANE.md).

### Checkpoint 218: The Minor Plane of Planitude title and history

After color-code removal, “The Minor Plane of Planitude” is unique among all 442 current ZON titles. Followed same-stem history reaches 2013-11-13 for WLD, 2014-08-17 for MOB, 2017-02-08 for OBJ and ZON, and 2014-08-18 for SHP; the 2009 source-tree move appears in the histories. No alternate-stem predecessor, retirement, or explicit replacement is established.

The reciprocal underdark WLD boundary, the local portal pairs, source-only ud_map2 approach, key-gated golden passageway, local shop, and compiled room #3404 destinations describe source dependencies on Flind. They do not establish replacement. Keep active flind DO NOT MOVE. See the [Flind audit supplement](FLIND_AUDIT_REVIEW.md), [coverage row 99](ZONE_COVERAGE_REGISTER.csv#L99), and [source dossier](../zone-stories/FLIND.md).

### Checkpoint 219: The Towering Glacier of Dayedenvale title and source history

After color-code removal and normalization, “Towering Glacier of Dayedenvale” is unique among 442 current ZON titles. Followed history remains on the glacier stem: the WLD history contains the 2009-08-12 source-tree move, MOB and OBJ history reaches 2014-10-02, and ZON history reaches 2015-10-13. No alternate-stem predecessor, retirement, or explicit replacement is established. Keep active glacier **DO NOT MOVE**. Preserve its one-way Surface boundary, Falina/King Polar key and slab records, and the source-only Surf approach without treating any as migration clearance. See the [Glacier audit supplement](GLACIER_AUDIT_REVIEW.md), [coverage row 108](ZONE_COVERAGE_REGISTER.csv#L108), and [source dossier](../zone-stories/THE_TOWERING_GLACIER_OF_DAYEDENVALE.md).

### Checkpoint 220: The Goblin Camp title and source history

After color-code removal and normalization, “The Goblin Camp” is unique among 442 current ZON titles. Followed history remains on goblincamp: the WLD history contains the 2009-08-12 source-tree move, MOB and OBJ history reaches 2015-09-21, and ZON history reaches 2015-10-13. No alternate-stem predecessor, retirement, or explicit replacement is established. Keep active goblincamp **DO NOT MOVE**. Preserve its Surface route, one-way wh arrival, forest-settlement source, and source-only Surf approach without treating them as migration clearance. See the [Goblin Camp audit supplement](GOBLINCAMP_AUDIT_REVIEW.md), [coverage row 109](ZONE_COVERAGE_REGISTER.csv#L109), and [source dossier](../zone-stories/GOBLINCAMP.md).

### Checkpoint 221: The Old Graveyard title and source history

After color-code removal and normalization, “The Old Graveyard” is unique among all 442 current ZON titles. Followed history remains on `grave`: WLD reaches 2013-11-13, MOB and ZON reach 2015-10-13, and OBJ reaches 2016-03-05; the 2009 source-tree move appears in each history. No alternate-stem predecessor, retirement, or explicit replacement is established by the checked-in titles and followed histories.

The two reciprocal Surface boundaries, keyed crypts, Ogrebane and its local special procedure, conditional Phantom destination, and four source-only inactive-WLD approaches document dependencies on Grave. Keep active `grave` **DO NOT MOVE**. See the [Grave audit supplement](GRAVE_AUDIT_REVIEW.md), [coverage row 113](ZONE_COVERAGE_REGISTER.csv#L113), and [source dossier](../zone-stories/THE_OLD_GRAVEYARD.md).

### Checkpoint 222: The Crypts of Rays title and source history

After color-code removal and normalization, “The Crypts of Rays” is unique among 442 current ZON titles. Followed file history remains on the graves stem: WLD through 2013-11-13, MOB and ZON through 2015-10-13, and OBJ through 2013-11-13; the older 2012 field update and 2009 source-tree move also appear. No alternate-stem predecessor, retirement, or explicit replacement is established. The QST, SHP, and story-sidecar files are absent. Keep active graves **DO NOT MOVE** because the random-zone generator, assigned object procedures, and Alatorin/Verspin resets consume its room and object prototypes. See the [Graves audit supplement](GRAVES_AUDIT_REVIEW.md), [coverage row 114](ZONE_COVERAGE_REGISTER.csv#L114), and [source dossier](../zone-stories/THE_CRYPTS_OF_RAYS.md).

### Checkpoint 223: Guildhalls title and source history

The normalized title Guildhalls is unique among 442 current .zon titles. Followed history records castle_rooms as the direct same-stem predecessor; commit 58e017ead explicitly renames it to guildhalls on 2010-02-02. Commit cd88c875c records a few object prototypes copied from player_castles, whose active title remains The Adventurers Guildhalls. Older house and guildhall handlers are marked deprecated in current source, but player_castles remains separately listed and no whole-zone replacement is documented. Keep the active contributors distinct. See the [Guildhalls audit supplement](GUILDHALLS_AUDIT_REVIEW.md#title-history-and-replacement-boundary).
### Checkpoint 224: Hell, Phlegethos title and source history

The normalized title Hell, Phlegethos is unique among 442 current ZON titles. Hell's Avernus, Dis, and Minauros are distinct sibling contributors, not evidenced replacements for this zone. Followed history begins with the 2009 tracked areas-tree move, followed by 2012 and 2013 source edits and a 2015 mobile/reset-field update. No alternate-stem predecessor, title rename, source deletion, or whole-zone replacement appears in tracked history. The conclusion is bounded by available repository history. See the [Phlegethos audit supplement](HELL_FOUR_PHLEGETHOS_AUDIT_REVIEW.md#title-and-replacement-history) and [source dossier](../zone-stories/HELL_FOUR_PHLEGETHOS.md).


### Checkpoint 225: Hell One title and source history

The normalized current ZON title Hell, Avernus is unique among 442 zone titles. Active avernus (#320) and inactive source-only plane_avernus (#135) share the different normalized title The Plane of Avernus; the two-room source-only area is not a replacement for the 400-room Hell One contributor. Prison room #7492 repeats Hell One's WLD caption because its five active exits lead to #51032; the matching caption is not a duplicate zone.

Followed history for Hell One's current WLD, MOB, OBJ, and ZON files stays on the hell_one_avernus stem, from the 2009 tracked source-tree move through later edits ending by 2015. No tracked alternate-stem predecessor, rename, or whole-zone replacement was identified. This is repository-history evidence only. Keep active Hell One **DO NOT MOVE**; see the [audit supplement](HELL_ONE_AVERNUS_AUDIT_REVIEW.md).


### Checkpoint 226: Hell Three title, history, and room VNUM collision

The normalized ZON title Hell, Minauros is unique among 442 current titles and differs from Hell, Dis. The active Hell Two and Minauros contributors have separate source stems and different repeated WLD titles: the second and third planes respectively. Both WLD files contain a room #51800 with different prose and exits. The same active room VNUM has two source definitions; this is a collision, not evidence that either whole zone replaced the other.

Followed WLD, MOB, OBJ, and ZON histories stay on their current stems. Both conflicting #51800 WLD records are present in the 2009 tracked source-tree move; same-stem maintenance continues through 2013 and 2015. No explicit title rename, alternate-stem predecessor, or whole-zone replacement was found. The tracked history does not identify which record should own #51800. Keep the duplicate/replacement review pending for builder resolution; see the [audit supplement](HELL_THREE_MINAUROS_AUDIT_REVIEW.md).


### Checkpoint 227: Hell Two title, history, and shared room VNUM

The normalized ZON title Hell, Dis occurs once among 442 current titles. `Mentiri, the Prison of Dis` is a separate contributor; the four named Hell planes have distinct titles. Hell Two and Minauros are separate active source stems, but both author WLD room #51800 with different descriptions and exits. The collision does not show that either full area replaces the other.

Followed Hell Two WLD and ZON histories remain on `hell_two_dis` from the tracked 2009 area-tree move through 2013. MOB and OBJ history on that stem includes 2012, 2013, and 2015 maintenance. No tracked alternate-stem predecessor, rename, title replacement, or whole-zone replacement was found. Repository history does not decide which #51800 record should own the VNUM; keep duplicate/replacement review pending until the builder resolves it. See the [Hell Two audit supplement](HELL_TWO_DIS_AUDIT_REVIEW.md).


### Checkpoint 228: Herders title and tracked history

The normalized active title The Behemoth Herders is unique among the 442 current ZON titles. The tracked Herders WLD and ZON histories remain on the `herders` stem from the 2009 area-tree relocation through later same-stem edits. The audit identified no alternate-stem predecessor, distinct replacement area, or title rename for this contributor. The separately authored Surf and Underdark 2011 exits into room #94300 are excluded source-only records; they do not establish that either old map replaces Herders. See the [Herders audit supplement](HERDERS_AUDIT_REVIEW.md#title-and-tracked-history).


### Checkpoint 229: Highmoor title and tracked history

The normalized title The High Moor Forest occurs once among all 442 current `.zon` titles. Other forest-named contributors, including Dark Forest, Forest of Mir, and The Forgotten Forest, have distinct titles and no source evidence that they replace Highmoor. Followed WLD, MOB, OBJ, and ZON histories stay on the `highmoor` stem from the tracked 2009 area-tree move through later maintenance; the WLD history includes a 2013 edit, while the other source types include later 2015 maintenance. No alternate-stem predecessor, title rename, or explicit whole-area replacement was found. Broader semantic comparison remains open; see the [Highmoor audit supplement](HIGHMOOR_AUDIT_REVIEW.md#title-history-and-disposition).

### Checkpoint 230: Southern Coastal Highway title and tracked history

The normalized title “Southern Coastal Highway” is unique among the 442 current `.zon` titles. No Highway room overlaps the global active-WLD duplicate #51800 or any inactive source room. Followed WLD and ZON histories remain on the `highway` stem through the tracked 2009 path move and later same-stem maintenance; the audit found no alternate-stem predecessor, title rename, or whole-area replacement. Keep the existing history classification; the new builder questions do not establish a replacement. See the [Highway audit supplement](SOUTHERN_COASTAL_HIGHWAY_AUDIT_REVIEW.md#title-history-and-disposition).


### Checkpoint 231: Hunt title and tracked history

The normalized title “The Caverns of Armageddon” is unique among the 442 current `.zon` titles. Followed Hunt WLD/MOB/OBJ/QST/ZON histories remain on the same stem through the tracked 2009 area-tree move and later edits; no alternate-stem predecessor, title rename or explicit replacement was found. Commit `ddbb2549e10de17552514ff94aa9bd4bc4d3a95d` (2026-04-06) redirects Surface #649695 east from #649696 into Hunt #13307 under a message describing a Hunt forest exit to the Jade Empire. Current Hunt #13307 west instead targets #649693, so the commit records route history but does not resolve present builder intent. See the [Hunt audit supplement](HUNT_AUDIT_REVIEW.md#title-history-and-disposition).


### Checkpoint 232: Hwood title and tracked history

The normalized title “The Haunted Woods” occurs once among the 442 current `.zon` files. Similar woods and forest titles belong to separate contributors; no checked-in evidence identifies them as Hwood replacements. Followed `hwood` WLD/MOB/OBJ/ZON histories remain on the same stem from the tracked 2009 `areas/` path move through later maintenance. No alternate-stem predecessor, title rename or explicit whole-area replacement was found. The contributor has no local QST file/history. See the [Hwood audit supplement](HWOOD_AUDIT_REVIEW.md#title-history-and-disposition).

### Checkpoint 233: IceCrag title history and related contributor

The normalized active title “Ice Crag Castle” is unique among the 442 current `.zon` titles. Followed `icecrag` WLD/ZON history remains on the same stem and shows continued maintenance without a named predecessor, title rename, or explicit whole-area replacement. The distinct `Ice Tower` title and the other Castle-named contributors have separate source owners. `Voluntown` is the closest related contributor: its title is “The Royal Mausoleum of Castle IceCrag,” its room range is #142400–#142532, and its own dossier and quest sources describe the connected Mausoleum. The reciprocal boundary between IceCrag #97132 and Voluntown #142400 records an authored relationship. It does not establish a duplicate, predecessor, or replacement. See the [IceCrag audit supplement](ICECRAG_AUDIT_REVIEW.md#active-title-and-tracked-history-boundary), [Royal Mausoleum dossier](../zone-stories/THE_ROYAL_MAUSOLEUM_OF_CASTLE_ICECRAG.md), and [Mausoleum audit](ROYAL_MAUSOLEUM_OF_CASTLE_ICECRAG_AUDIT_REVIEW.md).

### Checkpoint 235: Elder Brain Arena title and history

After color-code normalization, “Arena of the Elder Brain” has one owner in the current active title inventory: `illithid_arena`, zone 112. Followed WLD/MOB/OBJ/ZON histories remain on the same stem after the 2009 `areas/` relocation; no alternate-stem predecessor, title rename, or explicit whole-area replacement was found. The active `arena` area is “CTF Tournament.” Ixarkon and Ixxillikor are separately named city contributors with their own Elder Brain source text. The checked-in files establish thematic relationships but no duplicate room ownership or replacement claim. See the [Illithid Arena audit supplement](ILLITHID_ARENA_AUDIT_REVIEW.md#title-semantic-comparison-and-tracked-history) and the related [Ixxillikor](../zone-stories/IXXILLIKOR.md) and [Ixarkon](../zone-stories/IXARKON.md) dossiers.


### Checkpoint 236: Interspace title and tracked history

After color-code normalization, “Interspace” is unique among the 442 current ZON titles. Followed WLD, MOB, OBJ, and ZON histories remain on the `interspa` stem through the tracked 2009 `areas/` move and later same-stem edits; no alternate-stem predecessor, title rename, or explicit whole-area replacement was identified. The separately titled Ethereal Plane and Astral Main contributors own distinct room ranges; Interspace’s one-way Astral edge does not indicate duplicate ownership or replacement. See the [Interspace audit supplement](INTERSPACE_AUDIT_REVIEW.md#typed-dependencies-and-tracked-history).

### Checkpoint 237: Ixarkon title and semantic boundary

The normalized active title Ixarkon has one owner. Its 2010 Pillithid wording appears in a commit that adds the Ixarkon-to-Faang exit from #96474 to #15412; a case-insensitive tracked-path review found no area source named Pillithid. This supports the bounded Faang connection history, not a named predecessor or replacement claim. Ixxillikor and Illithid Arena are separately titled, separately ranged contributors. See the [Ixarkon audit supplement](IXARKON_AUDIT_REVIEW.md#title-and-replacement-history-boundary).

### Checkpoint 238: Ixxillikor title and related contributor boundary

The color-stripped title “Ixxillikor” has one owner among the 351 current non-empty active titles. Followed `ixxillikor` history names no distinct predecessor, replacement, or superseded source. Its shared Elder Brain theme and the two active Ixxillikor reset imports of Ixarkon mobile #96433 document authored relationships, not duplicate area ownership. Ixarkon, Ixxillikor, and Illithid Arena retain separate titles and prototype ranges. See the [Ixxillikor audit supplement](IXXILLIKOR_AUDIT_REVIEW.md#title-and-tracked-history) and its related [Ixarkon dossier](../zone-stories/IXARKON.md).

### Checkpoint 239: Jade Empire title, history, and related contributors

The normalized active title “The Jade Empire” has one owner among the 351 current active titles. Followed WLD, MOB, OBJ, QST, SHP, and ZON histories remain on the `jade` stem after the tracked 2009 area-tree move and later same-stem edits. The Jade story sidecar was added in 2026; no alternate-stem predecessor, distinct replacement, or title rename was found. Mitashi (“Capital City of the Jade Empire”) and `jademini` (“The Rice Fields”) have separate active identities, room ranges, and authored item/story links. Those relationships, and the lack of a direct active WLD boundary to either contributor, do not indicate duplicate ownership or replacement. See the [Jade audit supplement](JADE_AUDIT_REVIEW.md#title-and-tracked-history), [Mitashi review](#checkpoint-124-mitashi-and-the-duris3-map-identity), and [Rice Fields audit supplement](RICE_FIELDS_AUDIT_REVIEW.md#title-and-bounded-history).

### Checkpoint 241: Jindon title, history, and semantic comparison

The normalized title “Jindon the Deathwood Forest” has one owner among the 442 current `.zon` files. Followed WLD, MOB, OBJ, QST, SHP, and ZON paths stay under the `jin` stem through their tracked moves and edits; no alternate-stem rename, distinct predecessor, or whole-area replacement was found. Dark Forest, Dark and Twisted Wood, Forest of Mir, Haunted Woods, and Mosswood have separate titles and source identities. Their shared forest theme does not supersede Jindon's carnival/exhibit, Sirax arms-return, ticketmaster, and Deathwood routes; the WLD scan also found no Jindon room-VNUM duplicate. Surface's reciprocal edge and inactive Surf's source-only approach are connections, not replacement evidence. See the [Jindon audit supplement](JINDON_DEATHWOOD_AUDIT_REVIEW.md#title-and-bounded-history) and [coverage row 139](ZONE_COVERAGE_REGISTER.csv#L139).

### Checkpoint 242: Jotunheim title history and related source identities

The normalized active title “Jotunheim” remains unique. Followed Jotun WLD, MOB, OBJ, QST, ZON, and story-sidecar histories show the tracked area-tree move and same-stem maintenance; no distinct predecessor, alternate-stem rename, or replacement was found. `stormht` has a separate title and source identity, although its text says the Storm Giants left Jotunheim. `cldgt` is the separately titled Cloud Giant Kingdom. The unregistered `eoo` component is titled Valhalla and has no Jotunheim WLD approach or room overlap; Jotun's Valhalla artifact wording is thematic lore. These relationships do not identify duplicate ownership or a merge. See the [Jotunheim audit supplement](JOTUN_AUDIT_REVIEW.md#checkpoint-242-current-catalog-tool-and-semantic-revalidation), [Jotun title](../../../areas/zon/jotun.zon#L2), [Storm Giants source](../../../areas/wld/stormht.wld#L2294), [Storm Giants title](../../../areas/zon/stormht.zon#L2), [Cloud Giant title](../../../areas/zon/cldgt.zon#L2), [Valhalla source](../../../areas/wld/eoo.wld#L2), and [Jotun artifact](../../../areas/obj/jotun.obj#L1056).

### Checkpoint 243: Juiblex title, same-source spelling history, and related areas

The normalized active title “The 222nd Layer of the Abyss” is unique among 351 current non-empty active titles. Followed WLD, MOB, OBJ, QST, SHP, and ZON history identifies one contributor. The June 2026 correction from `jubilex` to `juiblex`, followed by August's normal source-file extensions, is same-source spelling and path cleanup; the tracked 2009 area-tree move is also path history. No alternate-stem predecessor, title owner, whole-area replacement, or merge was found. The story sidecar was added in 2026 as a separate authored mapping.

Astral Tiamat's separately placed Juiblex is described as an astral form. Ravenloft 2 owns the type-25 portal to Juiblex's fortress approach. The Underdark, Faerie Realm, Fields Between, Torg, Mountain, and Scorched Valley retain separate source identities and authored relationships; the four Hell layers also have distinct owners. These links, shared demon themes, and the spelling correction do not establish duplicate zone ownership or replacement. See the [Juiblex audit supplement](JUIBLEX_AUDIT_REVIEW.md#title-and-tracked-history), [active title](../../../areas/zon/juiblex.zon#L2), [Astral Tiamat form](../../../areas/mob/astral_tiamat.mob#L119), and [Ravenloft 2 portal](../../../areas/obj/ravenloft2.obj#L1239).


### Checkpoint 249: The Hall of Knighthood title, endpoint history, and related identities

After color stripping and case/whitespace normalization, “The Hall of Knighthood” has one owner among current active `.zon` titles. Followed WLD, MOB, OBJ, and ZON history remains on the `knight` stem through the tracked 2009 `areas/` move and later same-stem edits. Commit `71d731c5b` (2013-11-13) changes the entrance from two exits to old-map rooms #237364 and #237063 to one south exit to active Surface #639011; it does not rename or replace the Knight contributor. The old VNUMs remain in unregistered `surf.wld` and `surface2011.wld`; #237063 has different room identity and prose in those files. These are separate old-map sources and do not establish a predecessor or identify the provenance of the former boundary.

The explicit White Haven ambassador description and Bastine’s separate knighthood references document outside relationships, not duplicate ownership. The one source-only `surf.wld` approach #259810 north to #70900 remains on Surf candidate row 97 and is not replacement evidence. Keep `knight` **DO NOT MOVE**. See the [Hall of Knighthood audit supplement](KNIGHT_AUDIT_REVIEW.md#title-same-stem-history-and-map-identity) and [source dossier](../zone-stories/HALL_OF_KNIGHTHOOD.md).

### Checkpoint 250: Kobold Settlement title and source history

After color stripping and case/whitespace normalization, “Kobold Settlement” has one owner among the 351 active `.zon` contributors. Its followed WLD, MOB, OBJ, QST, SHP, and ZON sources retain the `kobold` stem through the tracked 2009 move into `areas/` and later same-stem maintenance; the story sidecar was added in October 2026. No alternate-stem predecessor, title duplicate, whole-area replacement, or room-VNUM overlap was found in the inactive WLD catalog.

Winterhaven's ambassador, one-way approach, and Q3652 memory exchange are relationships owned by Winterhaven and Kobold's current objects; they do not establish a replacement. Likewise, imported Heavens/Verz1/White Haven objects and Charing, Alatorin, Surface Keeps, and Sylvandawn consumers are typed dependencies, not duplicate area ownership. Keep active `kobold` **DO NOT MOVE**. See the [Kobold audit supplement](KOBOLD_AUDIT_REVIEW.md#title-source-history-and-related-identities) and [coverage row 148](ZONE_COVERAGE_REGISTER.csv#L148).

### Checkpoint 252: Lord Krimeneha's Mansion title and history

After removing color codes and normalizing whitespace/case, “Lord Krimeneha's Mansion” has one exact title owner among 351 active `.zon` contributors: `krimman`. Followed WLD and ZON histories remain under that stem through the tracked 2009 source-tree move and later maintenance; the 2015 quest-item flag update is same-source maintenance. No alternate-stem predecessor, repeated active title, or whole-zone replacement was found in the bounded path history.

The reciprocal Twisted Wood boundary, Quietus-owned staff/seal requests, and Eckraldu's exits into Mistywood, Twin Towers Forest, and Woodseer are authored relationships, not duplicate ownership. Keep active `krimman` **DO NOT MOVE**. See the [Krimman audit supplement](KRIMMAN_AUDIT_REVIEW.md#title-history-and-related-identities), [zone title](../../../areas/zon/krimman.zon#L2), and [coverage row 150](ZONE_COVERAGE_REGISTER.csv#L150).

### Checkpoint 258: Lava Tubes title history and contributor distinction

After color stripping and normalization, “Lava Tubes” has no duplicate title among the 351 active .zon contributors. Followed lava_tubes WLD and ZON histories remain on the same stem through the 2009 areas-tree move and later maintenance, including the 2010 automaton-crew quest addition and same-stem quest-item updates. No alternate-stem predecessor, title rename, or whole-zone replacement was found in the bounded history.

The nearby active lavcav contributor has the separate title “The Underground Lava Caves,” zone 355, and its own room range and source dossier. Their underground and volcanic language does not establish shared ownership. Keep lava_tubes and lavcav distinct. See the [Lava Tubes audit supplement](LAVA_TUBES_AUDIT_REVIEW.md#title-and-bounded-source-history), [Lava Tubes title](../../../areas/zon/lava_tubes.zon#L1), and [separate Underground Lava Caves dossier](../zone-stories/THE_UNDERGROUND_LAVA_CAVES.md).

### Checkpoint 259: Underground Lava Caves title history and Lava Tubes distinction

Color-stripping and whitespace normalization leave “The Underground Lava Caves” with one owner among 351 active .zon contributors: lavcav, zone 355. Its followed WLD/ZON history remains on that stem through the 2009 areas-tree relocation; the reviewed 2013 change is file mode and the 2015 ZON change is editor-comment metadata. No alternate-stem predecessor, title rename, or whole-zone replacement was found.

The separately active lava_tubes source is zone 120, titled “Lava Tubes,” with rooms #12000–#12166 and its own dossier. The Lava Caves source uses #35501–#35642 and has separate QST, resets, and story mapping. Similar lava and cavern themes do not establish a merge or replacement. Keep both contributors distinct. See the [Lava Caves audit supplement](UNDERGROUND_LAVA_CAVES_AUDIT_REVIEW.md#checkpoint-259-title-and-related-area-distinction), [Lava Caves title](../../../areas/zon/lavcav.zon#L1), and [Lava Tubes title](../../../areas/zon/lava_tubes.zon#L1).

### Checkpoint 260: Arcaneum title and tracked history

After color stripping and case/whitespace normalization, “The Arcaneum of L'srillizzin” has one owner among 351 active zone titles: `library`, zone 4020. The active WLD duplicate scan found no Arcaneum collision; its only global duplicate is #51800 in the two Hell areas. The tracked WLD/MOB/OBJ/ZON/QST paths remain on the `library` stem from the 2014 creation through the later renumbering and trapdoor edits. The 2026 story sidecar is an authored mapping, not a new zone or replacement. This bounded review identifies no alternate-stem predecessor, title rename, or whole-area replacement. Keep the active contributor distinct and **DO NOT MOVE**. See the [Arcaneum audit supplement](ARCANEUM_AUDIT_REVIEW.md#title-tracked-history-and-disposition), [active title](../../../areas/zon/library.zon#L2), and [coverage row 158](ZONE_COVERAGE_REGISTER.csv#L158).

### Checkpoint 263: Llyrath title and tracked-path history

After color stripping and title normalization, “Llyrath Forest” has one active owner among the 351 current contributors. Followed `llyrath.wld` history stays on the same stem through the 2009 areas-tree move and later same-source edits; 2011 spelling updates, mobile/object field-layout updates, the editor-version comment, and a file-mode change do not identify a rename or replacement. The bounded paths show no distinct predecessor or whole-area replacement. Keep active `llyrath` separate and **DO NOT MOVE**. The Moonshae pair and local pillar destination are connections, not duplicate ownership. See the [Llyrath source review](LLYRATH_STORY_REVIEW.md#tracked-history-and-owner-questions), [title](../../../areas/zon/llyrath.zon#L1), and [coverage row 161](ZONE_COVERAGE_REGISTER.csv#L161).

### Checkpoint 264: Llzazan title and history crosswalk

The normalized active title Llzazan Ghetto of Arachdrathos is unique. Arachdrathos
Wilderness and the other Arachdrathos district files retain their separate
active titles and area stems; the reviewed title/history material identifies no
Llzazan predecessor or replacement. The text on object #36252 says it once led
to Mazzolin room #21300, while the current active WLD register has no paired
Llzazan–Mazzolin boundary. Keep that text as a historical route clue; it does
not identify a superseded area. Keep active Llzazan in place. See the
[Llzazan source review](LLZAZAN_STORY_REVIEW.md#typed-dependencies-and-history-boundary)
and [coverage row 162](ZONE_COVERAGE_REGISTER.csv#L162).

### Checkpoint 265: Adventurer's Vaults title and tracked-path history

The normalized active title Adventurer's Vaults is unique. Tracked path history for lockers records a move involving the areas/ and
mud/areas roots. It establishes no separate predecessor, retired source family,
or replacement area. Keep the active system-support contributor in place.
See the [locker source review](LOCKERS_STORY_REVIEW.md),
[coverage row 163](ZONE_COVERAGE_REGISTER.csv#L163), and the
[attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-265-lockers-retention-and-storage-code).

### Checkpoint 266: Defense of Longhollow title and source history

After color stripping and normalizing case and whitespace, Defense of
Longhollow has one owner among the 351 active .zon contributors. Followed
long.zon, long.wld, long.mob, long.obj, and long.qst history remains on the same
source stem, with no separate predecessor, title rename, or whole-area
replacement established. The unregistered temple2 source has a different title
and a separate tracked source family, despite sharing zone header 344 and
overlapping 89 room, 30 mobile, and 31 object VNUMs with long. The different
prototypes and matching VNUMs make this an identity collision to preserve, not
evidence of replacement. Keep active long DO NOT MOVE and keep candidate
temple2 distinct. See the [Longhollow audit review](LONGHOLLOW_AUDIT_REVIEW.md),
[coverage row 164](ZONE_COVERAGE_REGISTER.csv#L164), and
[temple2 candidate row 102](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L102).

### Checkpoint 267: Ironstar title and tracked history

The normalized title The Ancient Halls of Ironstar is unique among 351 active
.zon contributors. Followed lornecro source history remains on the same stem;
the 2012 map-marker commit describes the Surface entry and exit, and the
October 2026 source fix corrects two exit-direction descriptions. No title
rename, separate predecessor, or whole-area replacement appears in the reviewed
history. Tracked lornecro .caplor sidecars remain same-stem non-loader files.

Inactive Duris3.wld shares room VNUMs #138900–#138999 but names these rooms The
Ocean and contains no matching Ironstar descriptions. This full-map overlap is
a distinct numeric owner, not evidence that Duris3 replaced Ironstar. Keep
active lornecro and Duris3 candidate row 36 separate. See the
[Ironstar audit review](LORNECRO_AUDIT_REVIEW.md),
[active title](../../../areas/zon/lornecro.zon#L2),
[Duris3 source row](../../../areas/wld/Duris3.wld#L816901), and
[coverage row 165](ZONE_COVERAGE_REGISTER.csv#L165).

### Checkpoint 268: The Tower of Darkness title, history, and Duris3 identity

The normalized title The Tower of Darkness is unique among 351 active .zon
contributors. Followed lortower history begins with its 2010 addition and stays
on the same source stem. The 2014 shop cleanup and October 2026 direction and
password repair establish no title rename, separate predecessor, or whole-area
replacement. No tracked lortower .caplor sidecar was found.

Inactive Duris3.wld has differently named records for all 142 active Tower
room VNUMs #134000–#134141. None of the normalized same-VNUM room titles match;
for example, active #134000 is An Old Path while Duris3 #134000 is Mountains,
and active #134141 is Random Mob Placement Chamber while Duris3 #134141 is The
Ocean. Its 286 non-overlapping source-only approach exits are inactive routing,
not loaded Tower ingress. Keep active lortower and the Duris3 map as separate
source owners on existing candidate row 36; do not interpret shared numbers or
approach records as duplicate rooms or replacement history. See the
[Tower audit review](LORTOWER_AUDIT_REVIEW.md#title-history-and-attic-disposition),
[active title](../../../areas/zon/lortower.zon#L2),
[Duris3 room #134000](../../../areas/wld/Duris3.wld#L714001),
[coverage row 166](ZONE_COVERAGE_REGISTER.csv#L166), and
[candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

### Checkpoint 269: Lylr-Meop title and bounded source history

The normalized title Lylr-Meop is unique among active .zon contributors and
among the 442 scanned .zon files. Followed lylr.wld and lylr.zon history
reaches the 2009 areas-tree move and later same-stem edits, including the 2013
WLD update, 2014 bank change, 2015 quest-item flag pass, and 2016 bottle fix.
The reviewed path history establishes no alternate-stem rename, separate
predecessor, or whole-zone replacement. The unselected Surf WLD approaches
remain on existing candidate row 97 and do not prove a superseded Lylr owner.
Keep active lylr in place. See the
[Lylr audit supplement](LYLR_MEOP_AUDIT_REVIEW.md#title-and-bounded-history),
[title source](../../../areas/zon/lylr.zon#L2),
[coverage row 167](ZONE_COVERAGE_REGISTER.csv#L167), and
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


### Checkpoint 270: Magma title and Duris3 identity

The normalized title The Para-Elemental Plane of Magma is unique among active
contributors and across the 442 scanned .zon files. Followed magma.wld and
magma.zon history reaches a 2012 template and 2013 minor update; the reviewed
path establishes no separate predecessor or whole-area replacement. Inactive
Duris3.wld reuses all 129 Magma room numbers for The Ocean records. Those
same-numbered source rooms are separate identities, and the 260 non-overlapping
Duris3 approaches plus the Vodgh approach remain inactive source references.
Keep active magma and Duris3 separate on candidate row 36. See the
[Magma audit review](MAGMA_AUDIT_REVIEW.md#title-history-and-disposition),
[active title](../../../areas/zon/magma.zon#L2),
[coverage row 168](ZONE_COVERAGE_REGISTER.csv#L168), and
[Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).


### Checkpoint 271: Malch title and separate Goblin Camp ownership

The normalized title Malch'Hor Ganl the Goblin City has one owner among active
contributors. The nearby title The Goblin Camp belongs to goblincamp and has no
reviewed predecessor or replacement relationship with Malch. Followed Malch
source histories remain on the same stem after the 2009 areas-tree relocation;
the reviewed edits change resets, shops, flags, and outpost gates without
establishing a rename or whole-area replacement. Keep the two active owners
separate. See the [Malch audit review](MALCH_AUDIT_REVIEW.md#title-history-and-disposition),
[active title](../../../areas/zon/malch.zon#L2),
[coverage row 169](ZONE_COVERAGE_REGISTER.csv#L169), and
[separate Goblin Camp title](../../../areas/zon/goblincamp.zon#L2).


### Checkpoint 272: The Forgotten Mansion title and bounded history

The normalized title The Forgotten Mansion matches one of the 442 scanned .zon
files and is unique among active contributors. Followed WLD and ZON history
reaches the 2009 areas-tree move, with later map edits through 2014 and reset
edits through 2015. The reviewed paths establish no separate predecessor or
whole-area replacement. Keep active mansion in place. See the
[Mansion audit review](MANSION_AUDIT_REVIEW.md#title-history-and-disposition),
[active title](../../../areas/zon/mansion.zon#L2),
[coverage row 170](ZONE_COVERAGE_REGISTER.csv#L170), and
[active manifest entry](../../../areas/AREA#L18).


### Checkpoint 273: Fort Marigot title and followed history

The normalized title Fort Marigot is the sole exact match among the 442 scanned
.zon titles and is unique in the active list. Followed marigot WLD, ZON, MOB,
OBJ, QST, and SHP histories remain on the same stem from the 2009 areas-tree
move through the reviewed 2011–2015 edits and current map updates. The reviewed
paths show no distinct predecessor, alternate-stem rename, or replacement.
Repeated room prose and separate centaur or forest areas do not establish a
replacement relationship. Keep active marigot in place. See the
[Marigot source review](MARIGOT_STORY_REVIEW.md#static-connectivity-and-replacement-boundary),
[active title](../../../areas/zon/marigot.zon#L2),
[coverage row 171](ZONE_COVERAGE_REGISTER.csv#L171), and
[active manifest entry](../../../areas/AREA#L27).


### Checkpoint 274: Maze of Undead Army title and source identity

The normalized active title The Maze of Undead Army is unique among the 351
active contributors; all current .zon titles were scanned, with no other title
containing Maze or Undead Army. Followed maze_are.wld and maze_are.zon history
remains under the same stem from the 2009 area relocation through the reviewed
2011/2013 map edits and 2015 quest-item update. The title Maze of Death appears
as room prose and does not establish another owner. Keep active maze_are and its
inactive references distinct. See the
[Maze audit review](MAZE_OF_UNDEAD_ARMY_AUDIT_REVIEW.md#title-and-tracked-source-history),
[active title](../../../areas/zon/maze_are.zon#L2),
[coverage row 172](ZONE_COVERAGE_REGISTER.csv#L172), and
[active manifest entry](../../../areas/AREA#L334).


### Checkpoint 275: Mazzolin title and bounded source history

The normalized title Mazzolin is unique among active contributors and across
the 442 scanned .zon files. Followed mazzolin.wld history reaches the 2009
areas-tree move and a 2013 edit; the reviewed path establishes no separate
predecessor or whole-area replacement. Its one inactive ud_map2 approach is a
source reference, not replacement evidence. Keep active mazzolin in place. See
the [Mazzolin audit review](MAZZOLIN_AUDIT_REVIEW.md#title-history-and-disposition),
[active title](../../../areas/zon/mazzolin.zon#L2),
[coverage row 173](ZONE_COVERAGE_REGISTER.csv#L173), and
[active manifest entry](../../../areas/AREA#L109).


### Checkpoint 276: Menden-on-the-Deep title and bounded history

The normalized title Menden-on-the-Deep is unique among active contributors and
across the 442 scanned .zon files. Followed menden WLD, ZON, MOB, OBJ, and QST
histories remain on the same stem after the 2009 areas-tree move; the story
sidecar was added in 2026. The reviewed paths establish no alternate-stem
rename, separate predecessor, or replacement. Keep active menden in place. See
the [Menden audit review](MENDEN_AUDIT_REVIEW.md#title-history-and-disposition),
[active title](../../../areas/zon/menden.zon#L2),
[coverage row 174](ZONE_COVERAGE_REGISTER.csv#L174), and
[active manifest entry](../../../areas/AREA#L314).


### Checkpoint 277: Halfling Silver Mine title and Alterian room identities

The normalized title The Halfling Silver Mine is unique among the 442 scanned
.zon files and the active title set. Followed mining.wld and mining.zon history
begins with the same-stem 2015 import and continues through later same-stem
edits, including the 2025 cleanup. No alternate-stem rename, separate
predecessor, or whole-area replacement was established. The 69 same-numbered
Alterian source rooms remain separate identities; their overlap does not claim
replacement. Keep active mining in place. See the
[Mining audit review](MINING_AUDIT_REVIEW.md#title-and-bounded-history),
[active title](../../../areas/zon/mining.zon#L2),
[coverage row 175](ZONE_COVERAGE_REGISTER.csv#L175), and
[active manifest entry](../../../areas/AREA#L25).

### Checkpoint 278: Mini Zones exact title and bounded history

The normalized title `Mini Zones` belongs only to active stem `minizones`
among 442 scanned `.zon` files. The lexical titles for the Surface and
Underdark Minizones are distinct. Followed `minizones.wld` and `minizones.zon`
histories reach the 2009 area-tree move and continue through same-stem edits;
no alternate-stem rename, distinct predecessor, or replacement was found in
those bounded paths. Eleven inactive Surf approaches, unresolved WLD exits,
and cross-area typed references are dependency evidence, not replacement proof.
Keep active `minizones` DO NOT MOVE. See [title and bounded history](MINIZONES_AUDIT_REVIEW.md#title-and-bounded-history),
[active title](../../../areas/zon/minizones.zon#L2), [coverage row 176](ZONE_COVERAGE_REGISTER.csv#L176),
and [active manifest entry](../../../areas/AREA#L29).

### Checkpoint 279: Deep Ravine title and current source history

The normalized title The Deep Ravine of Passage is unique among all 442 scanned
`.zon` files. Followed `minopass.wld` and `minopass.zon` histories remain on
the same stem back through the 2009 area-tree move; no alternate-stem rename,
separate predecessor, or replacement was established in those bounded paths.
The current WLD has #94772 south to #94773 and #94773 north back to #94772;
this reciprocal pair is not a self-exit. The retained gate/reset changes are
source state, not replacement evidence. Keep `minopass` DO NOT MOVE. See the
[title and bounded history](DEEP_RAVINE_AUDIT_REVIEW.md#title-history-and-disposition),
[active title](../../../areas/zon/minopass.zon#L2), [coverage row 177](ZONE_COVERAGE_REGISTER.csv#L177),
and [active manifest](../../../areas/AREA#L337).


### Checkpoint 280: Forest of Mir title and bounded history

The normalized title “Forest of Mir” is unique among all 442 scanned `.zon`
files. Followed `mir.wld` and `mir.zon` history remains on the same stem from
the 2009 `areas/` move through later edits, including the reviewed 2013 and
2016 changes. No alternate-stem rename, distinct predecessor, or whole-zone
replacement was established in those bounded paths. Keep active `mir` DO NOT
MOVE; see [title and bounded history](MIR_AUDIT_REVIEW.md#title-and-bounded-history), [active title](../../../areas/zon/mir.zon#L2), and [coverage row 178](ZONE_COVERAGE_REGISTER.csv#L178).


### Checkpoint 281: Myrabolus title and bounded history

The normalized title “Myrabolus” is unique among the 442 scanned `.zon` files.
Followed `mira.wld` and `mira.zon` histories remain on those stems from the
2009 area-tree move through same-stem edits in 2011–2017 and the 2026 ferry
update. No tracked alternate-stem rename, distinct predecessor, or whole-area
replacement was established in those bounded paths. Keep active `mira` DO NOT
MOVE; see [title and bounded history](MYRABOLUS_AUDIT_REVIEW.md#title-and-bounded-history), [active title](../../../areas/zon/mira.zon#L2), and [coverage row 179](ZONE_COVERAGE_REGISTER.csv#L179).


### Checkpoint 282: Shadow Forest title and bounded history

The normalized title “The Shadow Forest” remains unique among the 442 scanned
`.zon` files. Followed `mist.wld` and `mist.zon` histories remain on their
current stems; the bounded review established no alternate-stem rename,
distinct predecessor, or whole-zone replacement. Checkpoint 282 adds builder
questions and changes no title/history finding. Keep active `mist` DO NOT MOVE;
see [title and bounded history](SHADOW_FOREST_AUDIT_REVIEW.md#title-and-bounded-history), [active title](../../../areas/zon/mist.zon#L2), and [coverage row 180](ZONE_COVERAGE_REGISTER.csv#L180).


### Checkpoint 283: Chasm title and bounded history

A normalized exact-title scan of all 442 `.zon` files finds one owner for “The
Chasm of the Misty Vale”: active `mist_chasm`. The followed WLD, ZON, MOB, OBJ,
and QST paths remain same-stem from the 2009 area-tree move through later
edits; the tracked story sidecar was added in 2026. No explicit alternate-stem
rename, predecessor, or whole-zone replacement was found in those bounded
paths. Checkpoint 283 adds builder questions and no new history claim. Keep
active `mist_chasm` DO NOT MOVE; see [title/history review](MIST_CHASM_AUDIT_REVIEW.md#title-history-and-disposition), [current title](../../../areas/zon/mist_chasm.zon#L2), and [coverage row 181](ZONE_COVERAGE_REGISTER.csv#L181).


### Checkpoint 284: Mistywood title and bounded history

Color-code and whitespace normalization of all 442 current .zon titles yields
one owner for “Mistywood”: active mistywood. Followed Mistywood WLD, ZON, and
QST histories remain on their current stems from the 2009 areas-tree move
through later same-path edits. The parallel source-only kilfray.qst shares
giver and contract identities but is absent from the production lists; neither
the bounded paths nor list membership establish an alternate-stem rename,
predecessor, precedence, or whole-zone replacement. Keep active mistywood DO
NOT MOVE. See [title and lineage review](MISTYWOOD_AUDIT_REVIEW.md#checkpoint-284-title-history-and-bounded-typed-closure), [current title](../../../areas/zon/mistywood.zon#L2), [coverage row 182](ZONE_COVERAGE_REGISTER.csv#L182), and [production membership](../../../areas/AREA#L339).


### Checkpoint 285: Mitashi title and map identity

Color-token removal and case/whitespace normalization finds one owner for
“Mitashi - Capital City of the Jade Empire” among all 442 current .zon files:
active mitashi. The followed active WLD, ZON, MOB, OBJ, and QST paths entered
the current area under those stems in 2012 and remain same-stem; the 2026 story
sidecar has separate addition history. The component-level mitashi.shp copy
link to stormport.shp establishes shop-file ancestry only, not a predecessor
or whole-zone replacement. Duris3's same-numbered records are inactive map
identity evidence, not proof that Mitashi replaced that map. Keep mitashi DO
NOT MOVE and Duris3 RETAIN IN PLACE. See the [Mitashi title and history review](MITASHI_AUDIT_REVIEW.md#checkpoint-285-all-source-title-and-map-identity), [title source](../../../areas/zon/mitashi.zon#L2), and [coverage row 183](ZONE_COVERAGE_REGISTER.csv#L183).


### Checkpoint 286: Du'Maathe Castle title and bounded history

Color-code, case, and whitespace normalization finds one owner for “Du'Maathe
Castle” among all 442 current .zon files: active mntcastl. The followed
active WLD/ZON history retains its current stems and names no distinct
predecessor or whole-zone replacement. The five Surf/oldmap8 approaches are
inactive map records and do not establish lineage. Broader semantic source
comparison remains open. Keep active mntcastl DO NOT MOVE; see the [Castle audit review](DU_MAATHE_CASTLE_AUDIT_REVIEW.md#checkpoint-286-all-source-title-and-bounded-typed-closure), [title source](../../../areas/zon/mntcastl.zon#L2), and [coverage row 184](ZONE_COVERAGE_REGISTER.csv#L184).


### Checkpoint 287: Mobs Surf title and bounded history

Color-code, case, and whitespace normalization finds one owner for “The
Surface World” among all 442 current `.zon` files: active `mobs_surf`. The
followed WLD, MOB, OBJ, and ZON histories remain on their current stems from
the area-tree move through later edits; no alternate-stem rename, predecessor,
or whole-zone replacement was established in those bounded paths. The old
Surface map records are identity conflicts, not replacement evidence. Keep
active `mobs_surf` DO NOT MOVE; the map-owner decision for its 24 unresolved
destinations remains open. See the [Mobs_Surf title and history review](MOBS_SURF_AUDIT_REVIEW.md#title-source-history-and-disposition), [title source](../../../areas/zon/mobs_surf.zon#L2), and [coverage row 185](ZONE_COVERAGE_REGISTER.csv#L185).


### Checkpoint 288: Mobs Underdark title and bounded history

Color-code, case, and whitespace normalization finds one owner for “The
Underdark” among all 442 current `.zon` files: active `mobs_underdark`. “The
Twisting Tunnels of the Durian Underdark” and “The Minizones of the Underdark”
remain distinct titles. Followed WLD, ZON, MOB, OBJ, and QST paths retain their
same stems through the 2009 area-tree move and later edits; no alternate-stem
rename, distinct predecessor, or whole-zone replacement was found in those
bounded paths. Keep `mobs_underdark` DO NOT MOVE. See the [Mobs_Underdark title and history review](MOBS_UNDERDARK_AUDIT_REVIEW.md#title-source-history-and-disposition), [title source](../../../areas/zon/mobs_underdark.zon#L2), and [coverage row 186](ZONE_COVERAGE_REGISTER.csv#L186).


### Checkpoint 289: Moonshae all-title and source-history result

Normalized .zon title scanning across all 442 current files finds exactly one
owner for “Moonshae Island”: active moonshae. Followed WLD, ZON, MOB, OBJ,
QST, and SHP histories stay on the same stem from the 2009 area-tree move
through later edits. The bounded history shows no alternate-stem rename,
distinct predecessor, or whole-zone replacement. Broader semantic similarity
and command aliases remain separate owner questions. Preserve moonshae DO
NOT MOVE. See the [Moonshae title and history review](MOONSHAE_ISLAND_AUDIT_REVIEW.md#checkpoint-289-all-442-title-and-bounded-history), [title source](../../../areas/zon/moonshae.zon#L2), and [coverage row 187](ZONE_COVERAGE_REGISTER.csv#L187).


### Checkpoint 290: Morg's Place all-title and source-history result

Normalized title scanning across all 442 current .zon files confirms one owner
for “Morg's Place”: active morg. Followed WLD/MOB histories stay on that stem
from the 2009 area-tree move through 2015, and OBJ/ZON histories stay on that
stem through 2016. No alternate-stem predecessor, retirement, or whole-zone
replacement is established. UnderDark room #845236, “Morg's Palace,” remains a
related-name question on the same chasm approach and does not establish
replacement. Preserve morg DO NOT MOVE. See the [Morg title and history review](MORG_AUDIT_REVIEW.md#checkpoint-290-all-442-title-and-bounded-history), [title source](../../../areas/zon/morg.zon#L2), and [coverage row 188](ZONE_COVERAGE_REGISTER.csv#L188).


### Checkpoint 291: Moria all-title and tracked-history result

Normalized title scanning across all 442 current .zon files finds one owner for
“Neverwinter Woods”: active moria. Followed WLD and MOB histories stay on the
moria stem from the 2009 area-tree move through later edits, including the 2026
WLD maze fix and reversion. OBJ/ZON histories remain on the same stem through
2015/2016, with QST through 2013 and SHP through 2014. The bounded histories
show no alternate-stem rename, predecessor, retirement, or whole-zone
replacement. Preserve the unique active title and keep related forest wording
as a separate content question. See the [Moria title and history review](MORIA_AUDIT_REVIEW.md#checkpoint-291-all-442-title-and-bounded-history), [title source](../../../areas/zon/moria.zon#L2), and [coverage row 189](ZONE_COVERAGE_REGISTER.csv#L189).


### Checkpoint 292: Mosswood title and bounded history

Normalized title scanning across all 442 current .zon files confirms one
owner for “Mosswood”: active moss. Followed moss.wld history reaches the 2009
areas/ move and same-stem edits through 2026; moss.zon remains on the same
stem through the 2016 typo fixes and earlier same-stem edits. The bounded
histories establish no alternate-stem rename, distinct predecessor, or
whole-zone replacement. Keep active moss DO NOT MOVE. See the [Mosswood audit
review](MOSSWOOD_AUDIT_REVIEW.md#title-and-bounded-history), [title source](../../../areas/zon/moss.zon#L2), and [coverage row 190](ZONE_COVERAGE_REGISTER.csv#L190).

### Checkpoint 293: Mountain of the Banished all-442 title result

A normalized scan of all 442 current .zon files finds one exact owner for
“The Mountain of the Banished”: active mount. Mountain Tracts of the Untamed
and The Mountain of Peril Peaks remain distinct titles. Followed mount.wld
and mount.zon history stays on the same stem through the 2009 move and later
same-stem edits; the mount.story.json journal and spoken-route sidecar was
added in 2026. The bounded paths establish no alternate-stem rename,
predecessor, or whole-zone replacement. Preserve active mount DO NOT MOVE and
keep broad aliases as separate owner questions. See the [Mountain audit
review](MOUNTAIN_OF_THE_BANISHED_AUDIT_REVIEW.md#checkpoint-293-all-442-title-and-direct-builder-questions),
[title source](../../../areas/zon/mount.zon#L2), and [coverage row
191](ZONE_COVERAGE_REGISTER.csv#L191).

### Checkpoint 294: Mountain Tracts all-442 title and tracked history

A normalized scan of all 442 current .zon files finds one exact owner for
“Mountain Tracts of the Untamed”: active mountaintracks. The similarly named
Mountain of the Banished and Mountain of Peril Peaks have distinct exact
titles. Followed same-stem WLD/MOB/OBJ/ZON/QST/SHP paths reach the 2009
areas-tree move and later file-specific edits; the reviewed WLD endpoint
addition remains on the same stem, with no alternate-stem rename, tracked
predecessor, or whole-zone replacement identified. Preserve semantic alias
and map identity as separate owner questions. See the [Mountain Tracts title
and history review](MOUNTAIN_TRACTS_AUDIT_REVIEW.md#active-title-and-tracked-history),
[title source](../../../areas/zon/mountaintracks.zon#L2), and [coverage row
192](ZONE_COVERAGE_REGISTER.csv#L192).

### Checkpoint 295: Miaeril all-442 title and followed history

A normalized scan of all 442 current .zon files finds one exact owner for
“Miaeril Village”: active mril. Followed mril.wld, .mob, .obj, .zon, .qst, and
.shp histories remain on the same stem after the 2009 areas move and through
later reviewed edits; the sidecar was added in 2026. No alternate-stem rename
or explicit predecessor/replacement claim was found in those bounded paths.
The all-442 exact title result does not resolve command aliases or rule out an
untracked predecessor. Keep Miaeril DO NOT MOVE. See the [Miaeril title and
history review](MIAERIL_AUDIT_REVIEW.md#title-duplicate-and-replacement-history),
[title source](../../../areas/zon/mril.zon#L2), and [coverage row
193](ZONE_COVERAGE_REGISTER.csv#L193).

### Checkpoint 296: Lost Temple title and followed history

A normalized scan of all 442 current .zon files finds one exact owner for “The Lost Temple of the North”: active multiclass_temple. The similar tikitt title “Lost Temple of Tikitzopl” is a distinct owner. Followed same-stem WLD/MOB/OBJ/ZON/QST/SHP paths reach the 2009 area-tree move and reviewed later edits; no alternate-stem predecessor, tracked rename, or explicit replacement claim appears in those bounded paths. The QST area index lists multiclass_temple despite the absent local .qst file. Preserve these distinctions and keep the active area DO NOT MOVE. See the [Temple title and history review](MULTICLASS_TEMPLE_AUDIT_REVIEW.md#active-title-and-tracked-history), [title source](../../../areas/zon/multiclass_temple.zon#L2), [QST index](../../../areas/qst/AREA#L206), and [coverage row 194](ZONE_COVERAGE_REGISTER.csv#L194).

### Checkpoint 297: Mushroom Forest title and bounded history

A normalized scan of all 442 current .zon files finds one exact owner for “The Mushroom Forest”: active mush1. Myconid Mushroom Forest and Mushroom Caverns are separate active stems and titles. Followed mush1 WLD/MOB/OBJ/ZON history reaches the 2009 areas-tree move and reviewed edits through 2015; no alternate-stem predecessor, tracked rename, or explicit replacement claim was found in those bounded paths. Preserve the active identity as DO NOT MOVE. See the [Mushroom Forest title and history review](MUSHROOM_FOREST_AUDIT_REVIEW.md#all-442-title-and-followed-history), [title source](../../../areas/zon/mush1.zon#L2), the [separate Myconid dossier](../zone-stories/MYCONID.md), the [separate Mushroom Caverns dossier](../zone-stories/MUSHROOM_CAVERNS.md), and [coverage row 195](ZONE_COVERAGE_REGISTER.csv#L195).


### Checkpoint 298: Mushroom Caverns title and bounded history

A normalized scan of all 442 current .zon titles finds one exact owner for “The Mushroom Caverns”: active mushroom_caverns. mush1 and myconid have distinct titles and remain separate active stems. Followed WLD/MOB/OBJ/ZON paths stay on the same stem from the 2009 areas-tree move through reviewed edits ending in 2015; the story sidecar was added in 2026. No alternate-stem predecessor, tracked rename, or explicit replacement claim was found in those bounded paths. The QST index lists Mushroom Caverns without a local QST; three giver-owned contracts are declared in shared mobs_underdark.qst. Keep the active contributor DO NOT MOVE. See the [title and history review](MUSHROOM_CAVERNS_AUDIT_REVIEW.md#all-442-title-and-followed-history), [title source](../../../areas/zon/mushroom_caverns.zon#L2), [QST/AREA entry](../../../areas/qst/AREA#L116), and [coverage row 196](ZONE_COVERAGE_REGISTER.csv#L196).


### Checkpoint 299: Myconid title-history builder follow-ups

The all-442 normalized title scan and bounded same-stem history were completed in CP147. CP299 records twenty direct builder questions against the existing identity, routes, native spore exchange, optional cache, food, water, and scenery findings; it adds no replacement claim and changes no source. Keep active myconid DO NOT MOVE. See the [builder questions](MYCONID_AUDIT_REVIEW.md#checkpoint-299-direct-builder-questions-and-recommendations), [title and history section](MYCONID_AUDIT_REVIEW.md#title-history-and-disposition), and [coverage row 197](ZONE_COVERAGE_REGISTER.csv#L197).


### Checkpoint 300: Myrloch Vale all-442 title and history

A normalized scan of all 442 current .zon titles finds one exact owner for “Myrloch Vale”: active myrloch_vale. Mist Chasm and Glacier have distinct titles. Followed WLD/MOB/OBJ/ZON/QST/SHP histories remain on the same stem from the 2009 areas-tree move through reviewed edits; the story sidecar was added October 6, 2026. No alternate-stem predecessor, tracked rename, or explicit replacement claim was found in those bounded paths. Moonshae is a separate active contributor joined by the reciprocal boundary. Keep Myrloch Vale DO NOT MOVE. See the [all-442 title and history review](MYRLOCH_VALE_AUDIT_REVIEW.md#checkpoint-300-all-442-title-and-full-followed-history), [title source](../../../areas/zon/myrloch_vale.zon#L2), and [coverage row 198](ZONE_COVERAGE_REGISTER.csv#L198).


### Checkpoint 301: Nax all-442 title and bounded history

A normalized scan of all 442 current .zon titles finds one exact owner for “Nax”: active nax. Followed WLD/MOB/OBJ/ZON/SHP paths remain on the same stem from the 2009 areas-tree move through reviewed later edits; no same-stem QST exists and the story sidecar was added in 2026. No alternate-stem predecessor, tracked rename, or explicit whole-zone replacement claim was found in those bounded paths. Preserve “minotaur hometown -scip” as an annotation pending owner clarification, not replacement evidence. Keep Nax DO NOT MOVE. See the [all-442 title/history review](NAX_AUDIT_REVIEW.md#all-442-title-and-followed-history), [title source](../../../areas/zon/nax.zon#L2), and [coverage row 199](ZONE_COVERAGE_REGISTER.csv#L199).


### Checkpoint 302: Negative Material Plane all-442 title and history

A normalized scan of all 442 current .zon titles finds one exact owner for “Negative Material Plane”: active negplane. Followed WLD/MOB/OBJ/ZON/QST histories remain on the same stem from the 2009 areas-tree move through reviewed maintenance; the story sidecar was added in 2026 and no local SHP file exists. No alternate-stem predecessor, tracked title rename, or explicit whole-zone replacement was found. A combined 2012 commit message names a separate Royal Crypt addition to IceCrag. Keep negplane DO NOT MOVE. See the [all-442 title and history review](NEGATIVE_MATERIAL_PLANE_AUDIT_REVIEW.md#checkpoint-302-all-442-title-and-followed-history), [title source](../../../areas/zon/negplane.zon#L2), and [coverage row 200](ZONE_COVERAGE_REGISTER.csv#L200).


### Checkpoint 303: New Cave city full followed history

The all-442 normalized title scan finds one exact owner for “New Cave city”: active new_cavecity. Followed current WLD/MOB/OBJ/ZON/QST paths remain on the same stem through reviewed edits; the sidecar was added October 7, 2026, and there is no local SHP file. No alternate-stem predecessor, tracked rename, or explicit whole-zone replacement was found in these bounded paths. Keep new_cavecity DO NOT MOVE. See the [all-442 history extension](NEW_CAVECITY_AUDIT_REVIEW.md#checkpoint-303-all-442-title-and-followed-source-history), [title source](../../../areas/zon/new_cavecity.zon#L2), and [coverage row 201](ZONE_COVERAGE_REGISTER.csv#L201).


### Checkpoint 304: Ailvio title, followed history, and inactive identity overlaps

The normalized title “Ailvio, Duris Newbie Outpost” is unique among all 442 current .zon titles; nearby newbiemaps is titled “newbiemap.” Followed WLD/MOB/OBJ/ZON/QST/SHP histories remain on the newbie stem after the 2009 areas-tree move. WLD/MOB history reaches 2014, OBJ June 2026, ZON 2015, QST 2014, and SHP 2014; the story sidecar was added and updated October 2, 2026. No alternate-stem predecessor or explicit whole-zone replacement was found. Thirty-five mobile and 27 object same-number definitions occur across inactive source stems only. The inactive WLD approaches and type-25 references are source-only; they do not establish active ingress or replacement identity. See [Ailvio title/history and typed review](NEWBIE_AUDIT_REVIEW.md#checkpoint-304-all-442-title-and-followed-source-history) and [coverage row 202](ZONE_COVERAGE_REGISTER.csv#L202).


### Checkpoint 305: Plains of Life title and followed history

After color stripping, “The Plains of Life” is unique among 442 current .zon titles. Followed WLD/MOB/OBJ/ZON paths remain on the newbie2 stem; the October 2016 same-stem change restored Stream of Life and connected it to Ailvio. There is no local QST or SHP path; the story sidecar was added and updated October 2, 2026. No alternate-stem predecessor or explicit whole-zone replacement was found. See [Plains title/history and dependency review](NEWBIE2_AUDIT_REVIEW.md#checkpoint-305-all-442-title-and-followed-source-history) and [coverage row 203](ZONE_COVERAGE_REGISTER.csv#L203).



### Checkpoint 306: Forest of Serenity title and followed history

After color stripping, “The Forest of Serenity” is unique among 442 current .zon titles. Followed WLD history reaches the 2009 areas-tree move; MOB, OBJ, and ZON histories include same-stem maintenance through 2015. No current QST, SHP, or story-sidecar path, alternate-stem predecessor, tracked title change, or explicit whole-zone replacement was found. See the [newfor title/history review](NEWFOR_AUDIT_REVIEW.md#checkpoint-306-all-442-title-and-followed-source-history), [title source](../../../areas/zon/newfor.zon), and [coverage row 204](ZONE_COVERAGE_REGISTER.csv#L204).



### Checkpoint 307: Newhaven title and followed history

After color stripping, “The City of Newhaven” is unique among 442 current .zon titles. Followed WLD/MOB/OBJ/ZON/QST/SHP histories remain on the newhaven stem; the WLD and OBJ paths include 2026 text maintenance, and the revision-two sidecar was added and updated in October 2026. The in-room sign dates an earlier same-stem rework to August–October 2014. No alternate-stem predecessor or explicit whole-zone replacement was found. See [Newhaven title and history](NEWHAVEN_AUDIT_REVIEW.md#checkpoint-307-all-442-title-and-followed-history), [title source](../../../areas/zon/newhaven.zon), and [coverage row 205](ZONE_COVERAGE_REGISTER.csv#L205).

### Checkpoint 308: New Hope all-442 title and followed history

After removing color codes, “The Village of New Hope” is unique among all 442 current `.zon` titles. The followed WLD/MOB/OBJ/ZON/QST/SHP paths remain on the `newhope` stem through their tracked histories, including the 2009 area-tree move, 2014 shop-file cleanup, later object and reset edits, and 2026 maintenance/refactoring. The story sidecar was added on 2026-10-05 (`fdafff753`). This bounded review found no alternate-stem predecessor or explicit whole-zone replacement. See [New Hope title/history supplement](NEWHOPE_AUDIT_REVIEW.md#checkpoint-308-all-442-title-followed-history-and-graph-confirmation), [title source](../../../areas/zon/newhope.zon), and [coverage row 206](ZONE_COVERAGE_REGISTER.csv#L206).

### Checkpoint 309: Peril Peaks all-442 title and history

After color stripping, “The Mountain of Peril Peaks” has one exact owner among all 442 current `.zon` files: active `nexus`. Followed WLD/MOB/OBJ/ZON/QST/SHP histories remain on the same stem, through the 2009 area-tree move and later edits through May 2026; QST history reaches November 2013. The story sidecar was added on 2026-10-04. No alternate-stem predecessor or whole-zone replacement is identified. Prior all-442 reviews already distinguish Peril Peaks from the Mountain of the Banished and Mountain Tracts. See [Peril Peaks title and history](NEXUS_AUDIT_REVIEW.md#checkpoint-309-all-442-title-and-followed-history), [title source](../../../areas/zon/nexus.zon#L2), and [coverage row 207](ZONE_COVERAGE_REGISTER.csv#L207).


### Checkpoint 310: Reliquary Nexus of Aggression title and followed history

The color-stripped title “The Reliquary Nexus of Aggression” belongs uniquely to active nexus_ic among all 442 current .zon files. Followed WLD/MOB/OBJ/ZON histories stay on that stem: WLD begins and ends at the 2009-08-12 area-tree move; MOB and ZON continue through the 2015-10-13 QITEM flag overhaul; OBJ’s latest reviewed edit is 2013-11-13. No alternate-stem predecessor, tracked rename, or whole-zone replacement was identified. Nexus Island’s distinct title “The Reliquary Nexus of the Corsairs” is the next register row, not a rename or replacement. See [Nexus IC title and history](NEXUS_IC_AUDIT_REVIEW.md#all-442-title-and-followed-history), [title source](../../../areas/zon/nexus_ic.zon#L2), and [coverage row 208](ZONE_COVERAGE_REGISTER.csv#L208).


### Checkpoint 311: Reliquary Nexus of the Corsairs title and followed history

The color-stripped title “The Reliquary Nexus of the Corsairs” belongs uniquely to active nexus_island among all 442 current .zon files. Followed WLD/MOB/OBJ/ZON histories stay on the same stem: WLD begins and ends with the 2009-08-12 area-tree move; MOB and ZON continue through the 2015-10-13 QITEM flag overhaul; OBJ’s latest reviewed edit is 2013-11-13. No alternate-stem predecessor, tracked rename, or explicit whole-zone replacement was identified. The next row, nexus_uc, has the distinct title “The Reliquary Nexus of The Dead.” See [Nexus Island title and history](NEXUS_ISLAND_AUDIT_REVIEW.md#all-442-title-and-followed-history), [title source](../../../areas/zon/nexus_island.zon#L2), and [coverage row 209](ZONE_COVERAGE_REGISTER.csv#L209).

### Checkpoint 312: Nexus Upper City title and followed history

The normalized title “The Reliquary Nexus of The Dead” is unique among all 442 current `.zon` titles. The Aggression, Corsairs, Roper Den, and The Dead Nexus titles belong to separate active stems. `nexus_uc.wld` history contains only the 2009-08-12 area-tree move (commit `642da5400`). MOB, OBJ, and ZON share the 2016-09-22 level-and-portal update (commit `1866f24c0`); MOB and ZON also have the 2015 quest-item flag pass, while OBJ has a 2016 cleanup. No alternate-stem predecessor, tracked rename, or explicit whole-zone replacement appears in the followed history. See [Nexus UC audit supplement](NEXUS_UC_AUDIT_REVIEW.md#all-442-title-and-followed-history) and [coverage row 210](ZONE_COVERAGE_REGISTER.csv#L211).
### Checkpoint 313: Nhavan Island title and followed history

After color stripping, `Nhavan Island` is unique among 442 current `.zon` files. The other island titles are Moonshae Island and the two CBL Guildhall Monster Island copies; checkpoint 199 already identifies the latter as overlapping copies of active CBL Hall rooms, without a link to Nhavan. Followed WLD, MOB, OBJ, ZON, and SHP history stays on `nhavan_island_one`, including the 2009 area-tree move and later maintenance; no alternate-stem predecessor or explicit whole-zone replacement was found. The target #148782 is an inactive Duris3 `The Ocean` room and does not identify a replacement. Active Menden fisherman #88805 has a compiled ambient line about bringing turtle legs back from Nhavan, but the callback neither checks the item nor supplies travel. Keep that cross-zone mention as narrative context. See [coverage row 211](ZONE_COVERAGE_REGISTER.csv#L212) and the [Nhavan history and dependency review](NHAVAN_ISLAND_ONE_AUDIT_REVIEW.md#title-history-and-replacement-finding).

### Checkpoint 314: Northern Wilderness title and followed history

After color stripping, `Northern Wilderness` is unique among all 442 current `.zon` titles. Nearby northern and wilderness titles are distinct. Followed WLD, MOB, OBJ, and ZON histories remain on `northern_wilderness`; the reviewed history includes the 2009 area-tree move, 2011–2015 source-format and mobile/reset updates, and the 2013 content pass. No alternate-stem predecessor, tracked rename, or whole-zone replacement was found. The Northern Wilderness tree's Moralon clue, the Caves of Mt. Skelenak story, and the Centaur boundary are independent source links, not replacement evidence. See [coverage row 215](ZONE_COVERAGE_REGISTER.csv#L216) and the [Northern Wilderness history review](NORTHERN_WILDERNESS_AUDIT_REVIEW.md#title-history-and-replacement-finding).

### Checkpoint 315: Nyneth title and followed history

After color stripping, `Ny'Neth` is unique among all 442 current `.zon` titles. Followed WLD, MOB, OBJ, and ZON histories stay on active stem `nyneth`, reaching the 2009 move into the current `areas/` tree and later same-stem content, key, reset, and quest-item-flag edits. No alternate-stem rename, distinct predecessor, or whole-zone replacement was found in these bounded paths. Active `nyneth2` (“Ny'Neth's Stronghold”) and `nyneth3` (“Ny'Neth's Stronghold Continued”) are distinct later campaign stages. Their portals and reset reuse of Nyneth prototypes are dependency evidence, not duplicate or replacement proof. Keep `nyneth` DO NOT MOVE. See [coverage row 216](ZONE_COVERAGE_REGISTER.csv#L217), the [first-stage dossier](../zone-stories/NYNETH.md), and the [Nyneth history review](NYNETH_AUDIT_REVIEW.md#title-bounded-history-and-retention).

### Checkpoint 316: Olympus title and followed history

After color stripping and case folding, `Olympus` belongs only to active stem `olympus` among 442 current `.zon` titles. Followed WLD, MOB, OBJ, and ZON history reaches the 2009 area-tree move and later same-stem edits; no alternate-stem rename, distinct predecessor, or whole-zone replacement was established. Herders' “Mount Olympus” is a name-only QST reference, not replacement evidence. VNUM #218064 appears with different titles in inactive `surf` and `surface2011`; the shared number and unresolved Olympus exit do not establish which map is authoritative or newer. Keep Olympus DO NOT MOVE and preserve the identity decision on both existing candidate rows. See [coverage row 221](ZONE_COVERAGE_REGISTER.csv#L222), the [Olympus dossier](../zone-stories/OLYMPUS.md), and [audit review](OLYMPUS_AUDIT_REVIEW.md#title-bounded-history-and-retention).


### Checkpoint 317: Otower title and followed history

After color stripping and case folding, “The Outcasts Tower” is unique among all 442 current ZON titles. Followed WLD, MOB, OBJ, and ZON histories remain on the otower stem back to the 2009 move into the current areas tree; later changes are same-stem content or line-ending edits. No alternate-stem rename, distinct predecessor, duplicate active mobile/object prototype, or whole-zone replacement was found. White Harbor's #55447 memory and ambassador delivery are a cross-zone story dependency, not replacement evidence. Keep active otower **DO NOT MOVE**; see the [coverage row](ZONE_COVERAGE_REGISTER.csv#L226), [dossier](../zone-stories/OTOWER.md), and [audit review](OTOWER_AUDIT_REVIEW.md#title-history-and-retention).

### Checkpoint 318: Patrols title and followed history

The normalized title “The Patrol Guard Headquarters” is unique among the 442 current ZON titles. Followed WLD, MOB, OBJ, and ZON history stays on the patrols stem back to the 2009 move into the current areas tree; later records are same-stem edits. No alternate-stem rename, distinct predecessor, or whole-zone replacement was established. The unregistered fists_gh component has different room and mobile identities at overlapping VNUMs and is not listed in either current area manifest; that establishes a conflict requiring ownership review, not a replacement. Keep active Patrols DO NOT MOVE. See [coverage row 228](ZONE_COVERAGE_REGISTER.csv#L228), [Patrols dossier](../zone-stories/PATROLS.md), and [audit review](PATROLS_AUDIT_REVIEW.md#title-history-and-retention).

### Checkpoint 319: Pharr Valley title and bounded history

The source files reviewed for Checkpoint 71 remain unchanged in the current tree. The normalized active title scan still finds one “Pharr Valley” among 351 active contributors; the distinct active `pods` title is “Pharr Valley Swamp.” Followed WLD/ZON history reaches the 2009 area-tree move and 2010 pirate-chest load-room edits, with no distinct predecessor, alternate-stem rename, or replacement identified. Keep `pharrvly` **DO NOT MOVE**. See [coverage row 229](ZONE_COVERAGE_REGISTER.csv#L229), [dossier](../zone-stories/PHARR_VALLEY.md), and [audit review](PHARR_VALLEY_AUDIT_REVIEW.md#title-history-and-disposition).

### Checkpoint 320: Pine Hollow normalized title and history

After color stripping and case/space normalization, “Pine Hollow” is unique among 442 current `.zon` titles. The active `pods` area is separately titled “Pharr Valley Swamp.” Followed WLD/QST/MOB/OBJ/ZON histories reach the 2009 areas-tree move and later same-stem edits; no alternate-stem rename, distinct predecessor, or whole-zone replacement was established. The Luiren QST/SHP numeric overlaps lack corresponding WLD/MOB sources and establish identity ambiguity, not a replacement. Keep `pineholl` **DO NOT MOVE**. See [coverage row 230](ZONE_COVERAGE_REGISTER.csv#L230), [dossier](../zone-stories/PINE_HOLLOW.md), and [audit review](PINE_HOLLOW_AUDIT_REVIEW.md#title-history-and-retention).

### Checkpoint 321: Plane of Air normalized title and followed history

After color stripping and case/space normalization, “Plane of Air” is unique among all 442 current `.zon` titles. The distinct active `airp` area is titled “The Heart of the Storm” and has a reciprocal WLD boundary with this area. Followed WLD, MOB, OBJ, and ZON histories remain on `plane_air_one` through the 2009 move into the current `areas/` tree and later same-stem changes, including the Fire Plane sector correction to Jet Stream #24433 and 2012–2013 planar content. No alternate-stem predecessor, tracked rename, duplicate active prototype, or whole-zone replacement was established. The older Smoke portal is a separate object route; Water Plane's overlapping room VNUMs are source identity conflicts, not replacement evidence. Keep active `plane_air_one` **DO NOT MOVE**. See [coverage row 231](ZONE_COVERAGE_REGISTER.csv#L231), the [Plane of Air dossier](../zone-stories/PLANE_OF_AIR.md), and the [audit review](PLANE_OF_AIR_AUDIT_REVIEW.md#title-followed-history-and-retention).

### Checkpoint 322: Plane of Earth normalized title and followed history

After color stripping and case/space normalization, “Plane of Earth” is unique among all 442 current zone titles and the 351 active contributors. Followed WLD, MOB, OBJ, and ZON histories remain on `plane_earth_one` through the 2009 move into the current `areas/` tree and later same-stem edits. An August 2009 edit added `_noquest_` to objects #23808 and #23809 and several trap exits from #23926 into #23927; its message says the exits reduce Entemoch's chance to reach the map. This is same-stem content history, not predecessor evidence. No alternate-stem predecessor, tracked rename, or whole-zone replacement is established by that history. Temple of the Earth (`earth`) and Grumbar's Domain (`earthp`) have distinct active area titles and identities; their imports of Plane of Earth prototypes are shared references, not replacement evidence. Keep active `plane_earth_one` **DO NOT MOVE**. See [coverage row 232](ZONE_COVERAGE_REGISTER.csv#L232), the [Plane of Earth dossier](../zone-stories/PLANE_OF_EARTH.md), and the [audit review](PLANE_OF_EARTH_AUDIT_REVIEW.md#title-followed-history-and-retention).

### Checkpoint 323: Plane of Fire title and followed history

After color stripping and case/space normalization, “Plane of Fire” is unique among all 442 current `.zon` titles and the 351 active contributors. Commit `0e925fa992ecb648b260efa159bd016fa88f9136` (December 18, 2025; “FIX: removed old versions of Brass and renamed files to avoid confusion”) changed the active area family. Before the commit, `plane_fire_one` was the distinct 250-range #25001–#25105 map, while `plane_fire_three` owned #25401–#25525. The current `plane_fire_one` WLD, OBJ, ZON, and QST contents match the old `plane_fire_three` files; six of the eight mobile records match, with the two guardian records edited. The old 250-range content is no longer the active `plane_fire_one` contributor. This is a content transplant and stem relabel, not evidence of two current Fire areas. Keep active `plane_fire_one` **DO NOT MOVE** and do not restore the deleted `plane_fire_three` sources. See [Fire coverage entry (CSV line 233)](ZONE_COVERAGE_REGISTER.csv#L233), the [Plane of Fire dossier](../zone-stories/PLANE_OF_FIRE.md), and the [audit review](PLANE_OF_FIRE_AUDIT_REVIEW.md#title-followed-history-and-retention).

### Checkpoint 324: Plane of Water title and followed history

After color stripping and case/space normalization, “Plane of Water” is unique among all 442 current `.zon` titles and the 351 active contributors. Followed WLD, MOB, OBJ, and ZON paths remain on `plane_water_one` from the 2009 move into the current `areas/` tree through later same-stem edits. MOB, WLD, and ZON history includes 2012–2015 edits. OBJ history includes the 2015 Orb of the Sea proc/flag update and a September 2026 gear-stat adjustment. No alternate-stem rename, distinct predecessor, or whole-zone replacement is established. The separate unregistered `waterplane` source is already retained on candidate row 120 and has no inactive WLD exit into this active map; it is not established as a replacement. Sea Kingdom is a distinct active area whose reciprocal boundary is route evidence. Keep `plane_water_one` **DO NOT MOVE**. See [Water coverage entry (CSV line 234)](ZONE_COVERAGE_REGISTER.csv#L234), the [Plane of Water dossier](../zone-stories/PLANE_OF_WATER.md), and the [audit review](PLANE_OF_WATER_AUDIT_REVIEW.md#title-followed-history-and-retention).

### Checkpoint 325: Player Castles title and followed history

After color-code removal and whitespace normalization, “The Adventurers
Guildhalls” occurs once among 442 current `.zon` titles and is unique among
active contributors. Followed WLD, MOB, OBJ, ZON, and SHP history remains on
the `player_castles` stem from the 2009 area-tree move through later
same-stem edits (WLD and OBJ through 2013, SHP through 2014, ZON through
2015, and MOB through 2026). No alternate-stem rename or whole-zone
replacement is established. Active `guildhalls` has a separate
`castle_rooms` history and a few copied Player Castles objects; neither fact
establishes that `player_castles` was renamed or replaced. Keep the active
contributors distinct. See the [Player Castles audit review](PLAYER_CASTLES_AUDIT_REVIEW.md#title-followed-history-and-retention),
[coverage row 235](ZONE_COVERAGE_REGISTER.csv#L235), and the
[Guildhalls history note](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-223-guildhalls-title-and-source-history).

### Checkpoint 326: Port Skythic title and followed history

After game color-code removal and whitespace normalization, “The Ruins of
Port Skythic” occurs once among the 442 current `.zon` titles and once among
the 351 active contributors. Followed WLD, MOB, OBJ, and ZON histories begin
with the 2010-09-01 “Addition of Port Skythic and Tower of Darkness” commit;
WLD was last edited in 2010, MOB and ZON in 2015, and OBJ in 2016. The
followed file paths show no rename, and no alternate-stem predecessor or
whole-zone replacement is established. No local QST or SHP source exists.

The inactive Duris3 overlap does not establish a Port Skythic predecessor:
all 110 same-numbered records are titled “The Ocean,” while the active rooms
use Port Skythic city and harbor titles. Their 216 outgoing matches remain
source-identity overlaps. Keep active `PortSkythic` **DO NOT MOVE**. See the
[Port Skythic audit review](PORTSKYTHIC_AUDIT_REVIEW.md#title-followed-history-and-retention),
[coverage row 237](ZONE_COVERAGE_REGISTER.csv#L237), and
[source dossier](../zone-stories/PORTSKYTHIC.md).

### Checkpoint 327: Potions title and followed history

After game color-code removal and whitespace normalization, “The Potion
Treasure Vault” occurs once among 442 current `.zon` titles and once among
the 351 active contributors. Followed WLD, MOB, and ZON paths remain on the
`potions` stem; WLD edits reach 2013 and MOB/ZON edits reach 2015. The tracked
OBJ file is empty in the current tree and checked area-path snapshots from
2009 through 2015. No alternate-stem predecessor, path rename, or whole-zone
replacement is established. `brass-old-2` supplies a single unresolved
Efreeti vault destination, not replacement evidence. Keep active `potions`
**DO NOT MOVE**. See [coverage row 238](ZONE_COVERAGE_REGISTER.csv#L238), the
[Potions dossier](../zone-stories/POTIONS.md), and [audit
review](POTIONS_AUDIT_REVIEW.md#title-followed-history-and-retention).

### Checkpoint 328: Pragtog title and followed history

After color-code removal and whitespace normalization, "Pragtog's Domain"
occurs once in 442 current zone titles and once among the 351 active
contributors. Followed WLD, MOB, OBJ, and ZON histories remain on the
pragtog stem through the 2009 tree move, 2012-2013 area maintenance, and
2015 quest-item flag update. No alternate-stem rename, predecessor, or
whole-zone replacement is established.

The active Scorched Valley is a separate contributor with a distinct
normalized title, no exact normalized room title or description match, and
no direct WLD boundary to Pragtog. Shared scorched-room subject matter does
not establish replacement. Keep active pragtog DO NOT MOVE. See the
[Pragtog audit review](PRAGTOG_AUDIT_REVIEW.md#title-followed-history-and-retention),
[coverage row 239](ZONE_COVERAGE_REGISTER.csv#L239), and
[source dossier](../zone-stories/PRAGTOG.md).


### Checkpoint 329: Prisons of Carthapia identity and followed history

After color-code stripping and whitespace normalization, “The Prisons of
Carthapia” occurs once among 442 current zone titles and once among the 351
active contributors. Followed WLD and ZON history remains on the prison
stem: the 2009 area-tree move, later maintenance and quest-item flags, and
the 2026 center-sector fix do not identify a distinct predecessor or
replacement.

“Prison of Fort Boyard” (prisonb, VNUMs #43000–#43051) is a separate active
zone. The current titles differ, and normalized room-title and complete room
description scans found no exact overlap between the two zones. This bounded
comparison does not decide broader thematic ownership. Keep active prison
DO NOT MOVE. See [coverage line 240](ZONE_COVERAGE_REGISTER.csv#L240),
the [Prison audit review](PRISONS_OF_CARTHAPIA_AUDIT_REVIEW.md#title-and-neighboring-zone-comparison),
and [source dossier](../zone-stories/PRISONS_OF_CARTHAPIA.md).

### Checkpoint 330: Prison of Fort Boyard identity and separation

After color-code stripping and whitespace normalization, “Prison of Fort
Boyard” occurs once among 442 current zone titles and once among the 351
active contributors. Followed prisonb.wld history records the 2009 areas-tree
move, the 2013 edit, and the 2015 quest-item flag change; prisonb.zon history
records the 2009 move and the 2013 edit. The followed histories show no
alternate-stem rename, distinct predecessor, or explicit replacement.
The 2026 journal and combined-scale progression commit describes the
current scope and does not claim to replace another zone.

Active Carthapia prison is a separately named, disjoint VNUM contributor.
The Checkpoint 329 bounded comparison found no exact normalized room-title
or full-description overlap between the two areas. Shared prison subject
matter does not establish duplication or replacement. Keep Prisonb DO NOT
MOVE. See [coverage line 241](ZONE_COVERAGE_REGISTER.csv#L241), the [Prisonb
audit review](PRISON_OF_FORT_BOYARD_AUDIT_REVIEW.md#checkpoint-330-reset-aware-arrival-reach-and-builder-decisions),
and [source dossier](../zone-stories/PRISON_OF_FORT_BOYARD.md).

### Checkpoint 331: Lair of the Purple Worm identity and comparison

After color-code stripping and whitespace normalization, “Lair of the Purple
Worm” occurs once among 442 current zone titles and once among the 351 active
contributors. Followed pworm.wld history records the 2009 areas-tree move and
the 2013 edit; pworm.zon history includes those records and the 2015 quest-
item flag change. The 2026 story commit documents the current recipe and does
not claim to replace another area. No alternate-stem rename, predecessor, or
explicit replacement was found.

The active Caverns of the Worms is separately named and has 100 WLD rooms;
the Purple Worm Lair has 98. A case-folded, color-stripped, whitespace-
normalized comparison found zero exact room-title matches and zero exact
full-room-description matches between the two WLD files. Shared worm subject
matter does not establish duplication. Preserve both active contributors.
See [coverage line 242](ZONE_COVERAGE_REGISTER.csv#L242), the [Purple Worm
audit review](LAIR_OF_THE_PURPLE_WORM_AUDIT_REVIEW.md#checkpoint-331-reset-aware-arrival-reach-and-builder-decisions),
and [source dossier](../zone-stories/LAIR_OF_THE_PURPLE_WORM.md).

### Checkpoint 332: Neverwind Valley identity and followed history


### Checkpoint 333: Quietus Quay identity and followed history


### Checkpoint 334: Rabble title, identity, and followed history

After color-code removal and title normalization, “Rabble” occurs once among 442 current zone titles and once among the 351 active contributors. The six Rabble room VNUMs, two mobile VNUMs, and seven object VNUMs have no duplicate active prototype definitions. The WLD, MOB, OBJ, ZON, and SHP histories remain on the rabble stem through the 2009 area-tree move and their recorded 2013-2015 content/shop updates. The bounded path and title review found no alternate-stem predecessor or explicit whole-zone replacement. Keep the current contributor in place. See the [Rabble audit](RABBLE_AUDIT_REVIEW.md#checkpoint-334-builder-questions-and-recommended-resolutions), [coverage row 245](ZONE_COVERAGE_REGISTER.csv#L245), and [setting dossier](../zone-stories/RABBLE.md).

### Checkpoint 335: Ogre Raiders title and followed history

After color removal, whitespace normalization, and case folding, the title "Ogre Raiders" occurs once among 442 current zone titles and once among 351 active contributors. The six local mobile and three local object VNUMs have no duplicate current definitions in other active prototype files. Followed WLD/MOB/OBJ/ZON history stays on the raiders source stem through the 2009 tree move and later updates; the MOB history includes the 2015 quest-item-flag update commit. The bounded review found no alternate-stem predecessor or explicit whole-zone replacement. Keep active raiders DO NOT MOVE. See the [Raiders audit](RAIDERS_AUDIT_REVIEW.md#checkpoint-335-builder-questions-and-recommended-resolutions), [coverage row 246](ZONE_COVERAGE_REGISTER.csv#L246), and [setting dossier](../zone-stories/RAIDERS.md).

### Checkpoint 336: Castle Ravenloft title and followed history

After color removal, whitespace normalization, and case folding, "Castle Ravenloft" occurs once among 351 active area titles. The duplicate/replacement review found no separate Ravenloft or Strahd predecessor or replacement entry. The WLD, MOB, OBJ, ZON, and QST paths remain on the current ravenloft stem through the 2009 area-tree move; the story sidecar was added at its current path in a later commit. Preserve the distinct active identities for Castle Ravenloft, Ravenloft 2, Barovia, Barovia Continued, and Ravenloft's Catacombs. See the [Ravenloft audit](RAVENLOFT_AUDIT_REVIEW.md#title-duplicate-review-and-tracked-history) and [coverage row 248](ZONE_COVERAGE_REGISTER.csv#L248).

### Checkpoint 337: Ravenloft 2 title and bounded history

After color-token removal, whitespace normalization, and case folding, "The Ravenloft Catacombs" occurs once among all 442 current `.zon` titles and once among the 351 active area titles. It remains distinct from Castle Ravenloft, Barovia, and Barovia2. The available followed WLD, MOB, OBJ, ZON, and QST history stays on the current `ravenloft2` stem; no alternate-stem predecessor or explicit whole-zone replacement appears in the reviewed title and history records. Available repository history for these paths begins with the April 2026 import/refactor history, so the conclusion is bounded to tracked source. See the [Ravenloft 2 audit](RAVENLOFT2_AUDIT_REVIEW.md#title-duplicate-and-followed-history) and [coverage row 249](ZONE_COVERAGE_REGISTER.csv#L249).

### Checkpoint 338: Drustl's Yerdonia Enslaved title and tracked history

After color-token removal, whitespace normalization, and case folding, "Drustl's Yerdonia Enslaved" is unique among 442 current `.zon` titles and 351 active area titles. The Raxthan WLD has no duplicate active room VNUM; the only global active-WLD duplicate identified by its bounded scan is #51800 in Hell Three and Hell Two. Followed Raxthan source paths retain their current stem through the 2009 area-tree move and later tracked edits; the sidecar remains attached to this area. No distinct predecessor or replacement source was identified. Keep active `raxthan` DO NOT MOVE. See the [Yerdonia audit](YERDONIA_AUDIT_REVIEW.md#title-tracked-history-and-disposition), the [Checkpoint 338 review](YERDONIA_AUDIT_REVIEW.md#checkpoint-338-raxthan-reset-aware-access-and-builder-decisions), and [coverage row 250](ZONE_COVERAGE_REGISTER.csv#L250).

### Checkpoint 339: Faerie Realm title and tracked history

After color-token removal, whitespace normalization, and case folding, “Faerie
Realm” occurs once among 442 current .zon titles and once among the 351
active area titles. Its current room, mobile, and object VNUMs have no duplicate
definitions; fifteen normalized generic room-title matches have no exact full
description matches. Followed WLD, MOB, OBJ, ZON, QST, and SHP histories stay
on the current realm stem from the 2009 area-tree move; no alternate-stem
rename or distinct predecessor was found. The story sidecar was added in
October 2026. Keep active realm DO NOT MOVE. See the
[Faerie Realm audit](FAERIE_REALM_AUDIT_REVIEW.md) and
[coverage row 251](ZONE_COVERAGE_REGISTER.csv#L251).

### Checkpoint 340: Rift Valley Jungle title and tracked history

After color-token removal, whitespace normalization, and case folding, “Rift
Valley Jungle” occurs once among 442 current .zon titles and once among 351
active area titles. Its 470 rooms, 220 mobiles, and 220 objects have no
same-type VNUM overlap with other active definitions. The normalized full-room
description scan found no duplicate description elsewhere; repeated jungle
and canopy titles are generic templates, not replacement evidence. Followed
WLD, MOB, OBJ, ZON, QST, and SHP histories reach the 2009 area-tree move and
stay on the rftjngle stem. No alternate-stem predecessor or explicit whole-zone
replacement was found. Keep active rftjngle DO NOT MOVE. The two inactive Surf
and Surface2011 definitions of #228371 have different titles and topology;
that target ambiguity does not identify a replacement. See the [Rift Valley
Jungle audit](RFTJNGLE_AUDIT_REVIEW.md) and
[coverage row 252](ZONE_COVERAGE_REGISTER.csv#L252).

### Checkpoint 341: River Styx title, duplicate prototypes, and tracked history

After color-code removal, whitespace normalization, and case folding, “The
River Styx” occurs once among 442 current .zon titles and once among 351 active
area titles. There is no duplicate active WLD, MOB, or OBJ declaration in its
VNUM ranges. Inside the area, eight exact room title/description families are
reused; mobile pairs #90702/#90711, #90703/#90712, and #90710/#90713 share
descriptive text while retaining separate prototype fields. The two dock horns
also share type/value data and most prose, but only #90700 has the inscription.
These are internal similarities, not evidence of a whole-zone replacement.

Followed River Styx WLD, MOB, OBJ, and ZON histories remain on the current stem
through the 2009 move into the tracked areas tree and later same-stem edits
through 2013 and 2015. The bounded title/path review found no alternate-stem
predecessor, title alias, or explicit whole-zone replacement. Keep active
`river_styx` DO NOT MOVE. See the [River Styx audit](RIVER_STYX_AUDIT_REVIEW.md)
and [coverage row 253](ZONE_COVERAGE_REGISTER.csv#L253).

### Checkpoint 342: Rogue Plains title and followed history

After color removal, whitespace normalization, and case folding, Rogue Plains
appears once among all 442 current ZON titles and once among the 351 active
area titles. The similar “The Bloody Plains” and “The Plains of Life” titles
are distinct, with no same-type WLD, MOB, or OBJ overlap with roguerai.
The source history for WLD, MOB, OBJ, and ZON stays on the roguerai paths
through the 2009 area-tree move and later same-stem maintenance through 2013
(WLD/MOB/ZON) and 2016 (OBJ). No alternate-stem predecessor or replacement
was found. The similarly named areas/obj/rogue.pl is a Perl object-format
utility, not a zone object file. Keep active roguerai DO NOT MOVE. See the
[Rogue Plains audit](ROGUE_PLAINS_AUDIT_REVIEW.md) and
[coverage row 254](ZONE_COVERAGE_REGISTER.csv#L254).

### Checkpoint 343: Tunnels of Payang title and followed history

After color-code removal, trimming, whitespace normalization, and case-folding, “Tunnels of Payang” occurs once among 442 current ZON titles and once among 351 active contributors. Same-type WLD, MOB, and OBJ scans found no duplicate prototype numbers. Followed history keeps the tracked source on rtun paths from the 2009 area-tree move through later same-stem OBJ, MOB/SHP, and ZON maintenance; the story sidecar was added in 2026. No alternate-stem predecessor or replacement was identified. Object #53 also appears in three source-only component files outside areas/AREA; record those separately, not as active duplicate owners. Keep active rtun DO NOT MOVE. See [RTUN audit](RTUN_AUDIT_REVIEW.md#title-duplicate-and-replacement-history) and [coverage row 255](ZONE_COVERAGE_REGISTER.csv#L255).

### Checkpoint 344: Village of Refugees title and followed history

“Village of Refugees” is unique among 442 current ZON titles and 351 active contributors after color removal, whitespace normalization, and case-folding. Same-type room/mobile/object scans found no duplicate prototype IDs; the global duplicate room VNUM remains unrelated #51800 in Hell Two/Hell Three. Followed WLD, MOB, OBJ, ZON, QST, SHP, and story-sidecar histories stay on the ruins stem from the 2009 area-tree move; no alternate-stem predecessor or replacement was found. The inactive nexus56 consumers and surf.wld approach are source-only dependencies, not evidence of a predecessor. Keep active ruins DO NOT MOVE. See [Village of Refugees audit](VILLAGE_OF_REFUGEES_AUDIT_REVIEW.md#checkpoint-344-reset-aware-reach-refreshed-source-closure-and-builder-decisions) and [coverage row 256](ZONE_COVERAGE_REGISTER.csv#L256).

### Checkpoint 345: Sarmiz'Duul title, duplicates, and followed history

After color-code removal, trimming, whitespace normalization, and case-folding,
“Sarmiz'Duul” occurs once among 442 current `.zon` titles and once among 351
active contributors. Same-type room, mobile, and object scans across active
and source-only files found no duplicate VNUM declarations. Followed WLD, MOB,
OBJ, ZON, QST, and SHP history stays on the `sarmiz` paths from the 2009
area-tree move through later same-stem edits; the story sidecar was added in
2026. No alternate-stem predecessor, explicit whole-zone replacement, or
rename was identified. The `sarmiz` CHAOS alias and ferry stop point to typed
rooms and are not duplicate zone titles. Keep active `sarmiz` DO NOT MOVE. See
the [Sarmiz audit](SARMIZ_AUDIT_REVIEW.md#title-duplicates-and-followed-history)
and [coverage row 257](ZONE_COVERAGE_REGISTER.csv#L257).

### Checkpoint 346: Savannah title, room-number overlap, and followed history

After color removal, trimming, whitespace normalization, and case-folding,
“The Savannah of Broken Trusts” occurs once among 442 current `.zon` titles
and once among 351 active contributors. No same-type mobile or object
duplicates were found. All 167 Savannah room VNUMs also appear in inactive
`Duris3.wld`, whose #138500 “Mountains” and #138584 “The Ocean” records differ
from Savannah's authored map; its 668 matching target edges are identity
ambiguous, not a unique approach or replacement. Followed WLD, MOB, OBJ, ZON,
and QST histories remain on the Savannah stem; the story sidecar was added in
2026. No alternate-stem predecessor or explicit whole-zone replacement was
found. Keep active `savannah` DO NOT MOVE. See the [Savannah audit](SAVANNAH_AUDIT_REVIEW.md#title-duplicates-and-followed-history)
and [coverage row 258](ZONE_COVERAGE_REGISTER.csv#L258).

### Checkpoint 347: Scorched Valley title, duplicates, and followed history

After color removal, trimming, whitespace normalization, and case-folding,
“The Scorched Valley” occurs once among 442 current `.zon` titles and once
among 351 active contributors. No same-type room, mobile, or object VNUM
collision with another active or source-only area was found. The shared
`(kolut)` annotation in the `areas/AREA` entries for Torg, Fields Between, and
Scorched Valley is not evidence of a rename or predecessor. Followed WLD, MOB,
OBJ, ZON, and QST history stays on the `scorchvalley` paths from the 2009
area-tree move; its story sidecar was added in 2026. No alternate-stem
predecessor or explicit whole-zone replacement was identified. Keep active
`scorchvalley` DO NOT MOVE. See the [Scorched Valley audit](SCORCHVALLEY_AUDIT_REVIEW.md#title-duplicates-and-followed-history)
and [coverage row 259](ZONE_COVERAGE_REGISTER.csv#L259).

### Checkpoint 348: Sea title, VNUM identity, and followed history

After color removal, trimming, whitespace normalization, and case-folding,
“Graendiae” occurs once among all 442 current `.zon` titles and once among
351 active contributors. Same-type scans found no duplicate Sea room, mobile,
or object VNUM in another active contributor, and no inactive WLD source-room
overlap with Sea.

Inactive `surf.wld` and `surface2011.wld` both define #219818, #219820, and
#220119, but the identities differ: Surf has two “Rolling Foothills” records
and a “Beautiful Cascading Waterfall” with returns to Sea #67568;
Surface2011 titles all three “The Azure Waters of a Storm Sea” and none
returns to Sea. Preserve the candidate-specific source identities. The
three Surf returns were already included in the prior curated baseline and
are not new source-only approaches.

Sea and `seakngdm` / “Sea Kingdom” are distinct active stems and titles.
Followed WLD/MOB/OBJ/ZON/SHP histories stay on the Sea stem from the 2009
area-tree move through same-stem edits; the latest checked content commits
are WLD 2013, MOB 2014, OBJ 2016, ZON 2015, and SHP 2014. The sidecar was
added in 2026, and no current Sea QST file, alternate-stem predecessor, or
explicit whole-zone replacement was found. Keep active `sea` DO NOT MOVE.
See the [Sea audit](SEA_AUDIT_REVIEW.md#title-duplicates-and-followed-history)
and [coverage row 260](ZONE_COVERAGE_REGISTER.csv#L260).

### Checkpoint 349: The Seacaves title, VNUM identity, and followed history

After color-code removal, trimming, whitespace normalization, and
case-folding, “The Seacaves” occurs once among all 442 current .zon titles
and once among 351 active contributors. Same-type active scans found no
room, mobile, or object VNUM collision. The all-95 inactive-WLD scan found
no source-room VNUM overlap with Sea Caves.

The followed WLD, MOB, OBJ, and ZON histories remain on the seacaves stem
through the repository's area-tree move and subsequent same-stem edits. No
alternate-stem predecessor or explicit whole-zone replacement was
identified. Sea (Graendiae) and Sea Kingdom (seakngdm) have different titles
and remain distinct areas. Keep active seacaves DO NOT MOVE. See the
[Sea Caves audit](SEACAVES_AUDIT_REVIEW.md#title-duplicate-identity-and-followed-history)
and [coverage row 261](ZONE_COVERAGE_REGISTER.csv#L261).

### Checkpoint 350: Sea Kingdom title duplicates and followed history

After color-code removal, trimming, whitespace normalization, and
case-folding, “Sea Kingdom” occurs once among all 442 current .zon titles and
once among 351 active contributors. Whole-source same-type scans found no
room, mobile, or object VNUM overlap with any other active or source-only
area. The inactive-WLD sweep also found no room-number overlap. “Sea”
(Graendiae) and “The Seacaves” remain separate titles and sources.

Followed WLD, MOB, OBJ, ZON, and QST histories stay on the seakngdm stem from
the 2009 areas-tree move. The latest same-stem content commits found were
2013 for WLD and QST, 2015 for ZON, 2025 for MOB, and 2026 for OBJ; the story
sidecar was added in 2026. No alternate-stem predecessor, rename, or explicit
whole-zone replacement was found. Keep active seakngdm DO NOT MOVE. See the
[Sea Kingdom audit](SEAKNGDM_AUDIT_REVIEW.md#title-and-bounded-history) and
[coverage row 262](ZONE_COVERAGE_REGISTER.csv#L262).

### Checkpoint 351: Sevenoaks title, VNUM identity and followed history

After color-code removal, trimming, whitespace normalization and case-folding,
“Sevenoaks” occurs once among all 442 current `.zon` titles and once among
351 active contributors. Whole-source same-type scans found no room, mobile
or object VNUM overlap with any other active or source-only area. The complete
inactive-WLD sweep also found no room-number overlap.

Followed WLD and MOB history remains on the `seveno` stem through same-stem
edits to 2015-10-13; OBJ and ZON remain on that stem through the 2026-08-01
line-ending normalization, after the 2026-04-18 Sevenoaks ambassador/reward
update. No local QST file or story sidecar exists. No alternate-stem
predecessor or explicit whole-zone replacement appeared in the followed
history. Keep active `seveno` DO NOT MOVE. See the
[Sevenoaks audit](SEVENO_AUDIT_REVIEW.md#title-vnum-identity-and-followed-history),
[coverage row 263](ZONE_COVERAGE_REGISTER.csv#L263), and
[source dossier](../zone-stories/SEVENOAKS.md).

### Checkpoint 352: Arbre's Forest title, VNUM identity and followed history

After color-code removal, trimming, whitespace normalization and case-folding,
“Arbre's Forest” occurs once among all 442 current `.zon` titles and once
among 351 active contributors. Whole-source same-type scans found no room,
mobile or object VNUM overlap with any other active or source-only area.

Followed WLD and OBJ history stays on the `sforest` stem through the 2013
same-stem edits; MOB and ZON histories continue through 2015, back to the
2009 area-tree move. The title has no local QST source or story sidecar. No
alternate-stem predecessor or explicit whole-zone replacement was identified
in the bounded paths. Keep active `sforest` DO NOT MOVE. See the
[Arbre's Forest audit](SFOREST_AUDIT_REVIEW.md#title-duplicate-identity-and-followed-history),
[coverage row 264](ZONE_COVERAGE_REGISTER.csv#L264), and
[source dossier](../zone-stories/SFOREST.md).

### Checkpoint 353: The Great Shaboath title, VNUM identity and followed history

After color-code removal, terminator trimming, whitespace normalization, and case folding, “The Great Shaboath” occurs once among all 442 current .zon titles and once among the 351 active contributors. Same-type source review found no active room, mobile, or object VNUM overlap; the inactive-WLD sweep found no room-number overlap.

Followed histories stay on the shabo stem: WLD through 2013-11-13; MOB through 2026-08-26; OBJ through 2026-09-14; ZON through 2016-06-14; QST history reaches the 2009-08-12 area-tree move; and the story sidecar was added 2026-10-06. No alternate-stem rename, distinct predecessor, or explicit whole-zone replacement was found in these bounded records. Keep active shabo DO NOT MOVE. See the [Great Shaboath audit](GREAT_SHABOATH_AUDIT_REVIEW.md#checkpoint-353-refreshed-world-history-and-typed-dependency-reconciliation), [coverage row 265](ZONE_COVERAGE_REGISTER.csv#L265), and [source dossier](../zone-stories/THE_GREAT_SHABOATH.md).

### Checkpoint 354: Shadamehr Keep title, VNUM identity, and followed history

After color stripping, terminator trimming, whitespace normalization, and case folding, Shadamehr Keep is unique among all 442 current .zon titles and the 351 active contributors. Same-type active VNUM checks found no room, mobile, or object ownership overlap, and the inactive-WLD scan found no room VNUM overlap.

Followed WLD and OBJ history remains on shadamehr_keep through 2013-11-13; MOB history continues through 2026-08-26; ZON history reaches 2015-10-13 and same-stem changes back to the 2009 area-tree move. No local QST source or story sidecar exists. No alternate-stem predecessor, distinct prior zone, or explicit whole-zone replacement was found in the bounded history. Keep active shadamehr_keep DO NOT MOVE. See the [Shadamehr Keep audit](SHADAMEHR_KEEP_AUDIT_REVIEW.md#title-history-and-disposition), [coverage row 266](ZONE_COVERAGE_REGISTER.csv#L266), and [source dossier](../zone-stories/SHADAMEHR_KEEP.md).

### Checkpoint 355: Shadowclave title, VNUM identity, and followed history

After color stripping, terminator trimming, whitespace normalization, and case folding, Shadowclave is unique among all 442 current .zon titles and among the 351 active contributors. Same-type active VNUM checks found no room, mobile, or object overlap; inactive-WLD searches found no room-number overlap.

Followed history remains on shadowcl: WLD through 2013-11-13; MOB through 2014-08-17; OBJ through 2016-03-05; ZON through 2015-10-13; SHP through 2014-08-17. There is no local QST source or story sidecar. No alternate-stem predecessor, distinct prior zone, or explicit whole-zone replacement was found in the bounded paths. Keep active shadowcl DO NOT MOVE. See the [Shadowclave audit](SHADOWCL_AUDIT_REVIEW.md#title-history-and-disposition), [coverage row 267](ZONE_COVERAGE_REGISTER.csv#L267), and [source dossier](../zone-stories/SHADOWCLAVE.md).

### Checkpoint 356: Shady Grove builder-review reconciliation

The normalized-title and history findings remain those recorded at checkpoint 98. The `shady2.mob` overlap remains an unresolved source-identity question, not proof of a replacement. Its existing recommendation is to preserve the file separately until ownership, purpose, and tooling consumers are known. See [Shady Grove audit supplement](SHADY_GROVE_AUDIT_REVIEW.md#checkpoint-356-builder-question-coverage-reconciliation).

### Checkpoint 357: Underworld Shafts title and identity

The normalized title The Underworld Shafts is unique among 442 current ZON title records and 351 active contributors. Only the shafts WLD/ZON and empty MOB/OBJ files use this source stem; no alternate-stem or replacement zone was found. Followed WLD history reaches 2013-11-13 and the 2009-08-12 area-tree move; ZON history reaches 2015-10-13, 2013-11-13, and the 2009 move. Keep the active zone and its six reciprocal WLD boundaries in place. The unregistered ud_map0/1/2 files are separate map candidates, not Shafts replacements. See [Shafts audit supplement](SHAFTS_AUDIT_REVIEW.md).

### Checkpoint 358: Shaughin builder-question review

Checkpoint 131's normalized title and bounded-history review remains unchanged: active title owner shaughin, no alternate-stem rename or whole-zone replacement evidence, and shaughin2 remains a distinct active contributor. Checkpoint 358 closes only the explicit builder-question review field. See [Shaughin audit supplement](SHAUGHIN_AUDIT_REVIEW.md).

### Checkpoint 359: Shaughin2 Hunting Grounds title and tracked history

After color-code removal, terminator trimming, whitespace normalization, and case folding, “The Shaughin Hunting Grounds” is unique among 442 current ZON title records and the 351 active contributors. The followed WLD and ZON histories stay on shaughin2 through 2016-01-13 and reach the 2009-08-12 area-tree move; MOB history reaches 2015-10-13 and OBJ history reaches 2015-07-13. The area/qst index lists shaughin2, but no local shaughin2.qst file or story sidecar is present. No alternate-stem rename, distinct predecessor, or whole-zone replacement is established by these bounded histories. The upper shaughin settlement is a separate active contributor. Keep shaughin2 DO NOT MOVE. See the [Shaughin2 audit](SHAUGHIN2_AUDIT_REVIEW.md#candidate-and-evidence-boundary) and [dossier](../zone-stories/SHAUGHIN2_HUNTING_GROUNDS.md).

### Checkpoint 360: Ship title, fixed VNUM identity, and followed history

After color-code removal, terminator trimming, whitespace normalization, and case folding, “The Adventurers Shipyards” is unique among 442 current ZON titles and 351 active contributors. Same-type scans found no active room, mobile, or object VNUM overlap. Followed WLD, MOB, OBJ, and ZON histories remain on the `ship` stem; the reviewed history reaches the 2009 area-tree move and records the 2016 retirement of `player_ships` sources with Davy Jones' Locker and ship transit moved into `ship.wld`. This is historical cleanup, not a competing current area. Active `shipy` is the separate port. Keep active `ship` DO NOT MOVE. See the [ship runtime-area audit](SHIP_AREA_AUDIT_REVIEW.md#title-history-and-disposition), [coverage row 272](ZONE_COVERAGE_REGISTER.csv#L272), and [runtime-room dossier](../zone-stories/SHIP_RUNTIME_AREA.md).

### Checkpoint 361: Ship Yards title, history, and Aopal identity

After stripping color codes and the terminator, trimming whitespace, and folding case, “The Ship Yards” is unique among 442 current ZON titles and 351 active contributors. No same-type active mobile or object overlap was found. The only same-type room-header overlap is inactive `aopal.wld`, which shares #43301-#43328 with shipy but has a separate Tharnadian Causeway identity; 28 duplicate VNUMs do not establish a rename or replacement. Followed shipy file paths remain distinct through the 2009 area-tree move, with WLD reaching 2015-06-01, MOB 2014-08-27, OBJ 2015-10-13, ZON 2015-05-27, QST 2014-08-27, SHP 2015-05-27, and its story sidecar added in 2026. Keep `shipy` DO NOT MOVE and preserve Aopal's own source identity. See the [Ship Yards audit](SHIP_YARDS_AUDIT_REVIEW.md#title-same-type-identity-and-tracked-history), [coverage row 273](ZONE_COVERAGE_REGISTER.csv#L273), and existing [Aopal candidate row 10](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L10).

### Checkpoint 362: The Undead Fog title, VNUM identity, and history

After color-code removal, terminator trimming, whitespace normalization, and case folding, “The Undead Fog” is unique among all 442 current ZON titles and 351 active contributors. Same-type scans found no room, mobile, or object VNUM overlap in active or source-only files. Followed WLD, MOB, OBJ, ZON, and QST histories remain on `shortb` through the 2013-11-13 maintenance history and reach the 2009-08-12 area-tree move; MOB/OBJ history also includes the 2015 quest-item flag update. No alternate-stem predecessor or explicit whole-zone replacement is supported by these bounded paths. Keep `shortb` DO NOT MOVE. See the [Undead Fog audit](SHORTB_AUDIT_REVIEW.md), [coverage row 274](ZONE_COVERAGE_REGISTER.csv#L274), and [story dossier](../zone-stories/THE_UNDEAD_FOG.md).
### Checkpoint 363: Skulldrach title, VNUM identity, and history

After color-code removal, terminator trimming, whitespace normalization, and case folding, Skulldrach is unique among 442 current ZON titles and 351 active contributors. Same-type WLD, MOB, and OBJ scans found no overlap with active or inactive source files. Surface map cells mention Skulldrach Lake, but those titles do not share local room VNUMs or establish replacement ownership. Followed source paths stay on the skulldrach stem through the 2009 area-tree move; WLD changed the #12859 destination and direction in 2011, then restored its reciprocal Surface route in 2013. No alternate-stem predecessor or whole-zone replacement was found. Keep the active contributor DO NOT MOVE. See the [Skulldrach audit](SKULLDRACH_AUDIT_REVIEW.md#title-same-type-identity-and-tracked-history), [coverage row 276](ZONE_COVERAGE_REGISTER.csv#L276), and [source story map](../zone-stories/SKULLDRACH.md).

### Checkpoint 364: Para-Elemental Plane of Smoke title, identity, and history

After color-code removal, terminator trimming, whitespace normalization, and case folding, “The Para-Elemental Plane of Smoke” is unique among 442 current ZON titles and 351 active contributors. Same-type scans found no MOB or OBJ overlap. Inactive Duris3 reuses all 153 Smoke room VNUMs with different room names; inactive forofcon reuses #139900–#139952 with different titles. These are WLD identity collisions, not proof of a shared map or replacement. Followed WLD, MOB, OBJ, and ZON paths remain on `smoke` from the 2012-12-29 zone addition; local QST history starts in the 2015 item-flag update and the story sidecar was added in 2026. No alternate-stem predecessor or whole-zone replacement appears in the bounded history or current manifest. Keep active `smoke` DO NOT MOVE. See the [Smoke audit](SMOKE_AUDIT_REVIEW.md#title-same-type-identity-and-tracked-history) and [coverage row 277](ZONE_COVERAGE_REGISTER.csv#L277).

### Checkpoint 365: Smokeveil title, same-type identity, and history

After color-code removal, terminator trimming, whitespace normalization, and case folding, “Shairak and Smokeveil Forest” is unique among 442 current ZON titles and 351 active contributors. Same-type WLD, MOB, and OBJ scans found no VNUM overlap with any other active or source-only file. The nearby Para-Elemental Plane of Smoke is a distinct active identity. Followed WLD, MOB, OBJ, ZON, QST, and SHP histories remain on `smokev` back through the 2009-08-12 areas-tree move; the journal sidecar was added in 2026. No alternate-stem predecessor or explicit whole-zone replacement appears in the bounded paths. Keep active `smokev` DO NOT MOVE. See the [Smokeveil audit](SMOKEV_AUDIT_REVIEW.md#title-same-type-identity-and-tracked-history), [coverage row 278](ZONE_COVERAGE_REGISTER.csv#L278), and [story map](../zone-stories/SHAIRAK_AND_SMOKEVEIL_FOREST.md).

### Checkpoint 366: Snow Ogres title, same-type identity, and followed history

After color removal and normalized whitespace/case, “Valley of the Snow Ogres” is unique among all 442 current ZON titles and the 351 active contributors. Active and source-only WLD, MOB, and OBJ scans found no same-type VNUM overlap with `snogres`. Followed WLD, MOB, OBJ, ZON, and QST paths reach the 2009-08-12 areas-tree move; their latest tracked source edits are WLD 2013, MOB 2014, OBJ 2026-09-14, ZON 2015, and QST 2013. The story sidecar was added 2026-10-03. No alternate-stem rename, distinct predecessor, or whole-zone replacement appears in those paths or the current manifest. The source-only `brass-old-1.zon` G reference to object #87725 is a typed legacy reset reference, not same-type overlap or replacement evidence. Keep active `snogres` DO NOT MOVE; see the [Checkpoint 366 audit](SNOGRES_AUDIT_REVIEW.md).

### Checkpoint 367: Solonar title, same-type identity, and followed history

After color removal and normalized whitespace/case, “Vast Hidden Grove” is unique among all 442 current ZON titles and the 351 active contributors. Active and source-only WLD, MOB, and OBJ scans found no same-type overlap with `solonar`. Followed WLD, MOB, OBJ, ZON, QST, and SHP paths reach the 2009-08-12 areas-tree move; the latest tracked edits are WLD/MOB 2014, OBJ/ZON 2015, QST 2013, and SHP 2014. The story-sidecar history is from 2026-10-02. No alternate-stem rename, distinct predecessor, or whole-zone replacement appears in those paths or the current contributor list. The missing candidate inn room #30511 is a derived-range/code-binding issue, not room identity evidence. Keep active `solonar` DO NOT MOVE; see the [Checkpoint 367 audit](SOLONAR_AUDIT_REVIEW.md).

### Checkpoint 368: Split Shield title, same-type identity, and followed history

After color removal and normalized whitespace/case, “Village of Split Shield” is unique among all 442 current ZON titles and the 351 active contributors. Active and source-only WLD, MOB, and OBJ scans found no same-type overlap with `split_shield`. Followed WLD, MOB, OBJ, and ZON paths reach the 2009-08-12 areas-tree move; latest tracked edits are WLD 2013, MOB 2026-08-26, OBJ 2016, and ZON 2015. No alternate-stem rename, distinct predecessor, or whole-zone replacement appears in those paths or the current contributor list. `surf.wld`'s two source-only approaches use room IDs that other inactive maps assign to differently titled rooms; preserve the Surf file identity rather than treating the numbers as interchangeable. Keep active `split_shield` DO NOT MOVE; see the [Checkpoint 368 audit](SPLIT_SHIELD_AUDIT_REVIEW.md).

### Checkpoint 373: Surface titles, identity, and history

After color-code removal, whitespace normalization, and case folding, “The Surface Realm of Duris” has one exact match among 442 current ZON titles and 351 active contributors. The active Surface map shares room VNUMs only with newbiemaps (59,851) and surface2011-temp (60,004), within #600000–#660003; their room titles differ. The two candidate maps also share the same 602 source-only Surface approach tuples from non-overlapping source rooms, but that does not identify a loaded route or prove either map replaces current Surface. surface2011.wld has 90,000 rooms #200000–#289999 and no same-numbered room with current Surface; preserve its existing 145 active numeric references separately. No same-type mobile or object overlap was found.

Followed current Surface WLD, MOB, OBJ, ZON, and QST histories stay on the surface stem through the 2009-08-12 areas-tree move. The reviewed path histories do not identify an alternate-stem predecessor or an explicit whole-zone replacement decision; the story sidecar was added on 2026-10-03. Keep active surface DO NOT MOVE and retain the existing map candidates. See the [Surface audit](SURFACE_AUDIT_REVIEW.md#title-same-type-identity-and-tracked-history), [coverage row 286](ZONE_COVERAGE_REGISTER.csv#L286), and [source map](../zone-stories/SURFACE_REALM.md).
### Checkpoint 374: SurfaceKeeps title, same-type identity, and followed history

After color stripping and normalized whitespace/case folding, “The Depths of Duris” is unique among all 351 non-empty active ZON titles and 442 current ZON files. No other active WLD contributor declares a SurfaceKeeps room VNUM. Inactive Duris3.wld declares the same 2,645 room VNUMs, but none of the paired room titles match. This is a complete numeric overlap, not proof of a whole-zone predecessor or replacement; preserve both map identities and request owner lineage clarification. The unregistered surfacekeeps_objects.obj source repeats 54 active object VNUMs (39 bodies compare line-for-line and 15 differ); its followed history says it was added for stub loading in 2010. Keep it on candidate row 100. No mobile VNUM overlap or additional outside object overlap was found.

Followed WLD, MOB, OBJ, ZON, QST, and SHP history remains under the current surfacekeeps stem and extends to the 2009 area history; the story sidecar was added on 2026-10-04. No alternate-stem rename or explicit whole-zone replacement appears in those paths or the current title inventory. Broader command-target alias/owner disambiguation remains pending. See the [SurfaceKeeps audit](SURFACEKEEPS_AUDIT_REVIEW.md#title-same-type-identity-and-followed-history), [coverage row 287](ZONE_COVERAGE_REGISTER.csv#L287), and [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).


### Checkpoint 375: Surface Minizones title, same-type identity, and history

After color-code and terminator removal, whitespace normalization, and case folding, “The Minizones of the Surface” appears once among 442 current ZON titles and belongs to one of 351 active contributors. Active and source-only WLD, MOB, and OBJ scans found no same-type VNUM overlap. The 25 local mobile VNUMs #97900–#97924 are also object VNUMs inside this contributor; these are separate prototype types, not a collision or replacement. The similarly named active `minizones` contributor has its own “Mini Zones” title and identity.

Followed tracked paths remain on the `surfacemini` stem: WLD through 2009-10-25, MOB through 2015-04-22, OBJ through 2016-08-15, ZON through 2015-10-13, and QST through 2026-06-09; the story sidecar was added 2026-10-04. There is no local SHP path. No alternate-stem rename, distinct predecessor, or whole-zone replacement appears in the current manifest or bounded histories. Untracked history cannot be excluded. Keep active `surfacemini` DO NOT MOVE; command-target alias/owner disambiguation remains pending. See the [Surface Minizones audit](SURFACEMINI_AUDIT_REVIEW.md#title-same-type-identity-and-tracked-history) and [coverage row 288](ZONE_COVERAGE_REGISTER.csv#L288).

## Checkpoint 376: Swamp title, same-type identity, and followed history

The normalized active .zon title “Swamp” is unique among 442 current ZON titles and is used by one of 351 active contributors. Same-type active and source-only WLD/MOB/OBJ scans found no collision with another contributor. The #26001–#26012 mobile/object number pairs inside this zone are different prototype types, not same-type duplicates.

Followed tracked history stays on the swamp_one stem: WLD and OBJ through 2013-11-13, MOB and ZON through 2015-10-13, with source history reaching the 2009-08-12 area-tree move. No alternate-stem rename, explicit predecessor, or whole-zone replacement was established in those tracked histories. There are no local QST, SHP, or story-sidecar histories because those files are absent. The numeric #267604 room in inactive surface2011.wld has a Storm Sea title and separate exits; preserve it as a different source-map identity from Surf’s Shadow Swamp source room. Broader command-target alias/owner disambiguation remains pending. See the [Swamp One audit](SWAMP_ONE_AUDIT_REVIEW.md#title-same-type-identity-and-followed-history).


### Checkpoint 377: Swamp Two title, same-type identity, and history

After color-code and terminator removal, whitespace normalization, and case folding, “The Lair of the Swamp Troll King” is unique among 442 current .zon titles and belongs to one of 351 active contributors. Active and source-only WLD, MOB, and OBJ scans found no same-type collision with swamp_two. Its local #26100 mobile and object stubs are different types and neither is reset by its ZON.

Followed WLD and OBJ histories remain on the swamp_two stem through 2013-11-13; MOB and ZON histories remain on that stem through 2015-10-13, and the tracked path reaches the 2009-08-12 area-tree move. The 2010-10-03 ZON edit fdad70594 moved Heavens object #359 from a ground reset into the troll king’s G resets, matching the current placement. No alternate-stem rename, explicit predecessor, or whole-zone replacement was established in those paths. There are no local QST, SHP, or story-sidecar histories because those files are absent. Broader command-target alias ownership remains pending. See the [Swamp Two audit](SWAMP_TWO_AUDIT_REVIEW.md#title-same-type-identity-and-followed-history) and [coverage row 290](ZONE_COVERAGE_REGISTER.csv#L290).
### Checkpoint 378: Tchan and Alterian identity and history

After color-token removal and case folding, Tchan's title normalizes to “The Monestary of Tranquility,” unique among 442 current ZON titles and 351 active contributors. The active Tchan mobile and object #4800 are separate types. An inactive candidate, alterian_wilderness, independently defines a mobile #4800 and object #4800; its inactive ZON resets refer to those candidate-owned prototypes. Preserve the two source identities; the inactive definitions are not active duplicates in the full-area loader.

Inactive alterian_wilderness.wld also reuses Tchan room VNUMs #4801-#4809, but the paired titles differ: Gravelly Dirt Road, End of a Gravely Dirt Road, Front Yard, Next to a Small Well, and Corn Field versus Tchan's icy-trail and monastery titles. These collisions describe distinct map records and do not establish that either source replaced the other.

Followed Tchan history records the character's move to an icy mountain top, addition of tchan to the active area list, the Surface boundary correction, and the door/key/guard changes. The same-day reset-disable commit was reverted. Current followed WLD/MOB/OBJ/ZON histories show no alternate-stem rename, named predecessor, or whole-zone replacement. See the [Tchan audit](TCHAN_AUDIT_REVIEW.md#title-same-type-identity-and-followed-history), [candidate row 8](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L8), and [coverage row 291](ZONE_COVERAGE_REGISTER.csv#L291).

### Checkpoint 381: Temple of Laduguer title, identity, and history

Color-token removal and case normalization make “The Temple of Laduguer” unique among 351 current active .zon titles. Temple of Flames, Temple of the Earth, Temple of the Sun, and Temple to Skrentherlog have different normalized titles and remain separate active contributors.

Followed Temple WLD/ZON history records the new area in commit f74b1c035 on 2016-04-14 and the zone header change from 1 to 81 in commit 2c97f027b on 2016-06-02. The current WLD/MOB/OBJ/ZON identity remains on temple_laduguer. No alternate-stem rename, named predecessor, or whole-zone replacement is established by the followed paths.

Inactive sylvandawn.wld reuses all 103 Temple room VNUMs, but each overlapping source room title differs. It also has seventeen approaches from non-overlapping room IDs and 206 raw transitions from overlapping IDs. These are distinct unregistered Sylvandawn source records, not evidence of a Temple predecessor or replacement. Its typed references and conservative retention decision stay on existing candidate row 101. See the [Temple audit](TEMPLE_LADUGUER_AUDIT_REVIEW.md#title-same-type-identity-and-followed-history), [coverage row 294](ZONE_COVERAGE_REGISTER.csv#L294), and [candidate row 101](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L101).

## Checkpoint 382: Tentro title, VNUM identity, and followed history

After terminator trimming, whitespace normalization, and case folding, "The Mountain Village of Tentro" is unique among all 442 current ZON title records and among the 351 active contributors. Same-type scans find no active WLD, MOB, or OBJ VNUM overlap with tentro2; the scan of all 95 inactive WLD files also finds no exact Tentro room VNUM. Surface room #557288 carries the nearby label "Tentro" but has no direct WLD edge into tentro2, so it is contextual map identity rather than an alternate owner or replacement.

Followed WLD and ZON histories remain on tentro2, with no identified alternate-stem predecessor or explicit whole-area replacement. The history does record a route change: commit 0ac17d14c on 2010-08-05 removed the south exit from Tentro room #69249 to Winterhaven #55559 while Winterhaven's up exit into #69249 remained. The commit title is "Fixed up some missing exits on surface map"; it establishes the source edit but not whether the resulting one-way sewer route was intended.

The 2009 commit af691ce1e, "completed a missing Tentro quest," only adds a ZON equipment reset for surgeon's mask #69016 on deranged surgeon #69075. No local QST file appears in the followed quest history. See the [Tentro audit](TENTRO2_AUDIT_REVIEW.md#typed-dependencies-title-and-history), [coverage row 295](ZONE_COVERAGE_REGISTER.csv#L295), and [Tentro source dossier](../zone-stories/TENTRO.md).

## Checkpoint 383: Tezcat title, identity, and history

After removing color codes and the terminator, normalizing whitespace, and
folding case, the title is “Ruined Temple of Tezcatlipoca.” It is unique among
442 current ZON title records and belongs to one of the 351 active
contributors. Same-type active WLD, MOB, and OBJ scans found no overlap with
Tezcat. The scan of all 95 inactive WLD files also found no exact Tezcat room
VNUM. Mobile/object pairs #98914 and #98928 inside the zone are different
prototype types, not duplicate same-type records.

Followed WLD and ZON paths remain under the `tezcat` stem. No alternate-stem
rename, explicit predecessor, or whole-area replacement is established in
those paths. The WLD history records a temporary entrance reroute: commit
5794d8067 (2011-05-15) changed #98900 west from Surface #575445 to #249233
and added a down route to #249534; commit 71d731c5b (2013-11-13) removed the
down route and restored the Surface west exit. The history does not say why
the detour was added or reverted. ZON history also includes the 2015
quest-item-flag pass and 2014 worshipper spelling correction. There is no
local QST or story-sidecar history because those files are absent. See the
[Tezcat audit](TEZCAT_AUDIT_REVIEW.md#title-identity-and-followed-history),
[coverage row 296](ZONE_COVERAGE_REGISTER.csv#L296), and
[source dossier](../zone-stories/TEZCAT.md).

## Checkpoint 384: Tharnadia title, identity, and history

After color-code and terminator removal, whitespace normalization, and case
folding, the title is “Tharnadia - City of Humans.” It is unique among 442
current ZON titles and belongs to one of the 351 active contributors. Active
same-type room, mobile, and object scans find no VNUM overlap. Duris3 reuses
all 584 active Tharnadia room numbers, but its titles differ; its 604
non-overlapping approaches and 1,732 overlapping-source matches remain
distinct map-source evidence. `tharnadian_ruin` and `tharnrifts` remain separate
active identities.

Followed WLD, MOB, OBJ, ZON, and QST histories stay on the `tharnadia` stem.
Commit `3a88fc6ce` (“Renumbered tharn,” 2010-08-06) changes the same-stem
sources from room #6001 / zone header 60 to #132500 / header 1325 and adds
the area to `areas/AREA`. Commit `79e3dd0e4` changes the gate exits to reduce
camping in off-map rooms; `102254c47` removes the IC teleporter; and
`f0ff4a726` adds map exits. These commits establish historical route changes,
not a separately named predecessor or whole-area replacement. No migration
clearance follows from the title, VNUM overlap, or history. Keep active
`tharnadia` **DO NOT MOVE** and see the [audit supplement](THARNADIA_AUDIT_REVIEW.md#title-identity-and-followed-history),
[coverage row 297](ZONE_COVERAGE_REGISTER.csv#L297), and
[Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

## Checkpoint 385: Tharnadian Ruin title, identity, and history

After color-code removal, whitespace normalization, and case folding, the
[tharnadian_ruin ZON title](../../../areas/zon/tharnadian_ruin.zon#L2) is “The Tharnadian Ruin,” unique among all 442
current .zon titles (351 active contributors). Current active room, mobile,
and object scans find no same-type VNUM overlap. All 95 inactive WLD sources
were rechecked; none overlaps a Ruin room VNUM or targets a Ruin room.

Followed WLD and ZON history stays on tharnadian_ruin. The 2011 commits
8e4a056ff (“Updated some entrances/exits to tharn ruins for quest re-entry”)
and 677a9f877 (“Tharn Rifts Quest material in prep for re-adding it…”) record
same-stem route and quest-source work; 642da5400 is the 2009 repository path move.
These records establish no distinct predecessor, alternate-stem rename, or
whole-area replacement. Keep the current active contributor DO NOT MOVE;
semantic intent questions are listed in the [audit supplement](THARNADIAN_RUIN_AUDIT_REVIEW.md#checkpoint-385-directed-reachability-reset-state-and-builder-questions)
and [coverage row 298](ZONE_COVERAGE_REGISTER.csv#L298).

## Checkpoint 386: Tharnadia Rifts title, identity, and history

After color and terminator removal, whitespace normalization, and case folding,
“Tharnadia Rifts” is unique among all 442 current ZON titles. Same-type active
WLD comparisons find no duplicate room VNUM. Duris3 declares all 5,783 of the
same room numbers, but every paired normalized title differs. Preserve the
two map-source identities; the overlap is not active duplicate-room evidence
or proof that either map replaced the other.

Followed WLD and ZON history remains on the tharnrifts stem, showing the 2009
area-tree move and the ZON quest-item flag pass but no alternate-stem rename,
named predecessor, or whole-area replacement. Keep active tharnrifts DO NOT
MOVE and Duris3 RETAIN IN PLACE on existing candidate row 36. See the
[Tharnrifts audit](THARNRIFTS_AUDIT_REVIEW.md#title-history-and-disposition)
and [coverage row 299](ZONE_COVERAGE_REGISTER.csv#L299).

## Checkpoint 387: Thetis title, map identity, and followed history

After color-token and terminator removal, whitespace normalization, and case
folding, “Thetis's Realm” is unique among all 442 current ZON titles. Active
same-type WLD, MOB, and OBJ comparisons find no overlap with Thetis; the
all-95 inactive-WLD scan finds no room-VNUM overlap. The one Surf approach
from #251543 north to #38000 is a source-only boundary record, not an alternate
room owner or replacement map.

Followed thetis WLD and ZON histories stay on the same stem and establish no
alternate-stem rename, separately named predecessor, or whole-area
replacement. Keep active thetis DO NOT MOVE and preserve the Surf source map
on existing candidate row 97. See the
[Thetis audit](THETIS_AUDIT_REVIEW.md#checkpoint-387-active-boundary-refresh-surf-approach-and-builder-questions)
and [coverage row 300](ZONE_COVERAGE_REGISTER.csv#L300).

## Checkpoint 388: Thri title, source identity, and followed history

After color removal, terminator trimming, whitespace normalization, and case
folding, “Taliccicopiid” is unique among all 442 current ZON titles. Active
same-type WLD/MOB/OBJ overlap is zero; the all-95 inactive scan finds no Thri
room overlap; inactive same-type MOB/OBJ overlap is also zero. Followed
`thri` WLD, MOB, OBJ, ZON, and SHP histories remain on the same stem through
the tracked 2009 areas-tree move and later same-stem edits. No alternate-stem
predecessor, explicit rename, or whole-zone replacement is evidenced by
those bounded paths.

Inactive Surf #250600 and Surface2011 #250600 have different normalized room
titles and topology; only Surf #250600 exits west to Thri #67600. Preserve
Surf row 97 and Surface2011 row 98 separately. Keep active Thri DO NOT MOVE.
See the [Thri story review](THRI_STORY_REVIEW.md#checkpoint-388-graph-identity-and-typed-dependency-refresh)
and [coverage row 301](ZONE_COVERAGE_REGISTER.csv#L301).

## Checkpoint 391: Lost Temple title, map identity, and followed history

After color-token and terminator removal, whitespace normalization, and case
folding, “Lost Temple of Tikitzopl” is unique among all 442 current ZON titles.
Lost City of Tikitzopl (tikit) and The Lost Temple of the North
(multiclass_temple) are similarly named but distinct active owners. The
reciprocal City edge, imported City recipe items and one-way item portal do
not establish duplicate ownership or replacement.

Same-type WLD/MOB/OBJ comparisons find no duplicate Temple VNUM; the all-95
inactive-WLD scan finds no room overlap. Followed tikitt WLD, MOB, OBJ, ZON and
QST paths stay on the same stem through the 2009 areas-tree move and later
edits. The story sidecar is first tracked in commit dafd48924 (2026-10-02,
“Map Tikitzopl temple journals and recipe integration boundaries”). These
bounded paths show no alternate-stem predecessor, explicit rename or
whole-zone replacement. Keep active tikitt DO NOT MOVE; no candidate row is
added. See the [Temple review](TIKITT_AUDIT_REVIEW.md#title-map-identity-and-followed-history)
and [coverage row 304](ZONE_COVERAGE_REGISTER.csv#L304).

## Checkpoint 392: Torg title, map identity, and followed history

After color-token removal, terminator removal, whitespace normalization, and
case folding, “The Kingdom of Torg” is unique among all 442 current ZON titles
and 351 active contributors. No active external same-type WLD, MOB, or OBJ
prototype duplicates a Torg VNUM. Inactive map records preserve separate
numeric identities: 97 inactive WLD rooms across nine source stems have
different normalized titles; inactive `cabal` MOBs #29000–#29002 and `rf_gh`
MOBs #28900–#28906 differ from Torg's same-numbered mobiles; and inactive
`rf_gh` OBJs #28900–#28904 differ from Torg's objects. These are not current
active owners or replacement evidence.

Followed Torg WLD, MOB, OBJ, ZON, QST, and SHP history remains on same-stem
`torg` paths from the 2009 area-tree move through later edits. The story
sidecar is tracked from commit `b61918be4` on 2026-10-02. The shared `(kolut)`
annotation also appears on Fields Between and Scorched Valley. No tracked
`kolut.*` source stem, explicit Torg rename, distinct predecessor, or
whole-zone replacement was found; the parenthetical alone does not establish
one. Keep active Torg DO NOT MOVE. See the [Torg audit](TORG_AUDIT_REVIEW.md#title-followed-history-and-retention),
the earlier [shared annotation review](#checkpoint-347-scorched-valley-title-duplicates-and-followed-history),
and [coverage row 305](ZONE_COVERAGE_REGISTER.csv#L305).

## Checkpoint 393: Torrhan title, map identity, and followed history

After stripping color codes, terminators, and spacing, “City of Torrhan” is
unique among all 442 current ZON titles and has one active owner. No active or
inactive external WLD, MOB, or OBJ source defines a same-type duplicate of a
Torrhan prototype. All 95 inactive WLD sources were checked; none has a
Torrhan room-number overlap.

Inactive source stem torr is a separate City of Fehlnan map around room
#79000. Its similar filename prefix does not establish a Torrhan alias,
predecessor, or replacement. Followed Torrhan WLD, MOB, OBJ, ZON, QST, and SHP
histories stay on same-stem torrhan paths from the 2009 area relocation
through later edits. The story sidecar was added in commit 4085a7810 on
2026-10-03. The active reset references to objects #41 and #72 resolve to the
currently active Heavens definitions; same-number definitions in Dalvik,
Hostel, and Mavulsk are inactive source identities. Keep active Torrhan DO NOT
MOVE. See the [Torrhan audit](TORRHAN_AUDIT_REVIEW.md#same-type-definitions-history-and-maintained-code)
and [coverage row 306](ZONE_COVERAGE_REGISTER.csv#L306).

## Checkpoint 394: Troll Slave Compound title, map identity, and followed history

After color-code and spacing normalization, “Troll Slave Compound” appears
once among all 442 current ZON titles and has one active owner. No active or
inactive external same-type WLD room, MOB, or OBJ prototype duplicates a
Torture identity, and no inactive WLD room overlaps #95400–#95430.

The followed WLD, MOB, OBJ, and ZON histories first appear at the 2009-08-12
tracked area-tree import under the current `torture` stems; later recorded
changes continue on those paths. The available history shows no alternate-
stem rename, distinct predecessor, or whole-zone replacement, and it does not
establish what existed before that import. The area files' `torture` source
stem and normalized ZON title “Troll Slave Compound” identify the one current
active contributor. Keep active `torture` DO NOT MOVE. See the
[Torture audit](TORTURE_AUDIT_REVIEW.md#title-map-identity-and-followed-history)
and [coverage row 307](ZONE_COVERAGE_REGISTER.csv#L307).

## Checkpoint 396: Tradeskills title, catalog identity, and followed history

Normalized title “Tradeskills” occurs once among all 442 current ZON titles
and has one active owner. No external active or inactive WLD room or MOB
prototype duplicates a Tradeskills identity, and no other OBJ source overlaps
the current object set. The unlisted `tradeskills.old.obj` is the distinct
exception: it overlaps 234 object VNUMs, with 218 identical whole records and
16 changed records; the active OBJ has 59 further definitions. Keep that old
catalog out of the active loader and preserve its separate identity.

Followed WLD, MOB, OBJ, and ZON history remains on the `tradeskills` stems from
the 2009 area-tree import through later edits. The 2015 WLD change documents
#400999 as a VNUM inspection ceiling. No alternate-stem rename, distinct
predecessor, or whole-zone replacement is established. Keep the active area
DO NOT MOVE. See the [Tradeskills audit](TRADESKILLS_AUDIT_REVIEW.md#title-history-duplicates-and-disposition),
[source dossier](../zone-stories/TRADESKILLS.md), and
[coverage row 309](ZONE_COVERAGE_REGISTER.csv#L309).

## Checkpoint 397: Trakkia title and bounded history retained

The existing normalized title review still finds one active owner,
`trakkia`, among 351 active `.zon` titles. Followed same-stem history contains
access, chest, equipment, and ambassador edits; it shows no alternate-stem
rename, distinct predecessor, or whole-zone replacement. This builder review
adds no identity or history finding. Keep active Trakkia DO NOT MOVE and see
the [audit](TRAKKIA_AUDIT_REVIEW.md#title-history-and-disposition),
[builder questions](../zone-stories/TRAKKIA_MOUNTAINS.md#explicit-builder-questions-and-recommended-resolutions),
and [coverage row 310](ZONE_COVERAGE_REGISTER.csv#L310).

## Checkpoint 398: Traoul title and bounded history

Normalized “Traoul's Lair” has one owner among 442 current `.zon` titles.
Current WLD/MOB/OBJ header scans found no exact external room or prototype
collision in its local VNUM ranges. Followed source history stays on the same
stems through the 2009 tree move and later 2012–2015 maintenance; no alternate
stem, predecessor, rename, or whole-zone replacement is established. Truk's
distinct title is the connected neighbor. Keep active Traoul DO NOT MOVE; see
the [audit](TRAOUL_AUDIT_REVIEW.md#title-history-and-disposition),
[dossier](../zone-stories/TRAOULS_LAIR.md#registry-dependencies-and-qualification),
and [coverage row 311](ZONE_COVERAGE_REGISTER.csv#L311).

## Checkpoint 399: Tribal Forest title and bounded history

Normalized “Tribal Forest” has one owner, `tribal`, among 442 current `.zon`
titles. Active WLD/MOB/OBJ header scans found no exact external room or
prototype VNUM collision in the matching local sets. Followed WLD, MOB, OBJ, ZON, QST, and SHP histories remain on their current
stems through the 2009 area-tree move and later same-stem edits. The story
sidecar history begins with the 2026 `tribal.story.json` addition. No
alternate-stem predecessor, rename, or whole-zone replacement is established.
Tribal Oasis
is separately titled and retained under its own `oasis` area identity. Keep
active Tribal Forest **DO NOT MOVE**; see the
[audit](TRIBAL_FOREST_AUDIT_REVIEW.md#title-history-and-disposition),
[dossier](../zone-stories/TRIBAL_FOREST.md), and
[coverage row 312](ZONE_COVERAGE_REGISTER.csv#L312).


## Checkpoint 400: Transparent Tower title and bounded history

Normalized “The Transparent Tower” has one owner, `trnsptow`, among 442
current `.zon` titles. Followed WLD, MOB, OBJ, ZON, and QST histories remain
on their same stems through the 2009 area-tree move and later maintenance; the
story sidecar was added in 2026. No alternate-stem rename, distinct
predecessor, or whole-zone replacement is established. Preserve the active
area **DO NOT MOVE**; the source-only Surf approach and Luiren SHP room-number
overlap do not establish replacement. See the
[Transparent Tower audit](TRNSPTOW_AUDIT_REVIEW.md#title-history-and-disposition),
[source dossier](../zone-stories/TRNSPTOW.md), and
[coverage row 313](ZONE_COVERAGE_REGISTER.csv#L313).

## Checkpoint 403: Truktakyagyo title and bounded history

After color-code removal and case/whitespace normalization, “Truktakyagyo” has
one exact owner among the 351 active ZON titles: area truk, zone 16. Followed
WLD, MOB, OBJ, and ZON histories stay on the same stem through the 2009
area-tree move and edits through 2015. No distinct predecessor, alternate
stem, or replacement claim is established in those paths. The absent local
story sidecar has no history to follow. Keep active truk DO NOT MOVE. See the
[Truktakyagyo audit](TRUK_AUDIT_REVIEW.md#title-history-and-retention),
[title source](../../../areas/zon/truk.zon#L2),
and [coverage row 316](ZONE_COVERAGE_REGISTER.csv#L316).

## Checkpoint 404: Twin Towers title and bounded history

The normalized active title “The Twin Towers” has one owner among 351 current
ZON contributors: `ttowers`. Its followed WLD, MOB, OBJ, ZON, and QST paths stay
on the same stems through the 2009 area-tree move and later maintenance. The
global active-WLD duplicate-room finding is #51800 in Hell Two and Hell Three;
none is in Twin Towers. No distinct predecessor, alternate-stem rename, or
whole-zone replacement is established. Keep this active contributor DO NOT
MOVE; see the [Twin Towers audit](TWIN_TOWERS_AUDIT_REVIEW.md#title-history-and-disposition),
[source dossier](../zone-stories/TWIN_TOWERS.md), and
[coverage row 317](ZONE_COVERAGE_REGISTER.csv#L317).

## Checkpoint 405: Tundra title and bounded history

The normalized active title “Tundra” has one owner among current ZON titles:
area `tundra`, zone 137. Followed WLD and ZON paths remain on their current
stems and show no distinct predecessor or replacement. This is bounded file
history; the broader semantic comparison remains pending. Keep active Tundra
DO NOT MOVE. See the [Tundra audit](TUNDRA_AUDIT_REVIEW.md#active-title-and-tracked-history-boundary),
[title source](../../../areas/zon/tundra.zon#L2),
and [coverage row 318](ZONE_COVERAGE_REGISTER.csv#L318).

## Checkpoint 406: Turolopolis Zoo title and bounded history

The normalized active title “Turolopolis Zoo” is unique among the 442 scanned
ZON titles. Ruins of Turolopolis is a separate title and area (`willem`).
Followed WLD and ZON histories stay on the `turolzoo` stem through the 2009
area-tree move and later same-stem edits. No distinct predecessor, rename, or
whole-zone replacement is established in this bounded review. Keep active
`turolzoo` DO NOT MOVE. See the [Zoo audit](TUROLZOO_AUDIT_REVIEW.md#title-and-bounded-history),
[title source](../../../areas/zon/turolzoo.zon#L2), and
[coverage row 319](ZONE_COVERAGE_REGISTER.csv#L319).

## Checkpoint 407: Twin Towers Forest title scan

The normalized active ZON title “Twin Towers Forest” has no duplicate in the
current active title list. The broader semantic/history comparison remains
pending. Keep this active contributor DO NOT MOVE. Its direct source and
builder-question history is in the [forest dossier](../zone-stories/TWIN_TOWERS_FOREST.md)
and [milestone review](TWIN_TOWERS_FOREST_MILESTONES.md); see
[coverage row 320](ZONE_COVERAGE_REGISTER.csv#L320).

## Checkpoint 408: A Dark and Twisted Wood title and bounded history

The normalized active title “A Dark and Twisted Wood” has one exact owner
among 351 current ZON titles: area `twstwd`. Followed WLD, ZON, MOB, OBJ, and
QST histories remain on their current stem from the 2009 area relocation
through later same-stem edits, including the 2015 quest-item flag update. The
story sidecar was added in 2026. No alternate-stem rename, predecessor, or
whole-zone replacement is established in these bounded paths. Keep active
`twstwd` DO NOT MOVE. See the [audit supplement](DARK_AND_TWISTED_WOOD_AUDIT_REVIEW.md#title-history-and-disposition),
[source dossier](../zone-stories/DARK_AND_TWISTED_WOOD.md), and
[coverage row 321](ZONE_COVERAGE_REGISTER.csv#L321).

## Checkpoint 409: udmini title and bounded history

The normalized active title “The Minizones of the Underdark” is unique among
442 current ZON titles. Followed WLD, MOB, OBJ, and ZON paths remain on the
`udmini` stem through the 2009 area-tree move and later same-stem edits. No
alternate-stem predecessor, retirement, or whole-zone replacement is
established in these bounded paths. Keep active `udmini` **DO NOT MOVE**. See
the [UDMINI audit](UDMINI_AUDIT_REVIEW.md#title-followed-history-and-disposition),
[source dossier](../zone-stories/UDMINI.md), and
[coverage row 322](ZONE_COVERAGE_REGISTER.csv#L322).

## Checkpoint 410: Ugta title and bounded history

The normalized active title “Ugta” has no repeated owner in the current title
scan. Followed WLD history records the 2009 `areas/` path migration and a
2013 content change; the current duplicate/replacement register has no Ugta
replacement entry. This bounded file history does not prove that no semantic
predecessor exists. Keep active Ugta **DO NOT MOVE**. See the [Ugta source
review](UGTA_STORY_REVIEW.md#history-and-disposition), [builder questions](UGTA_STORY_REVIEW.md#explicit-builder-questions-and-recommended-resolutions),
and [coverage row 323](ZONE_COVERAGE_REGISTER.csv#L323).

## Checkpoint 411: Durian Underdark title and bounded history

The normalized active title “The Twisting Tunnels of the Durian Underdark”
has one exact owner among 351 active non-empty ZON titles: `underdark`.
Lexical matches “The Underdark” (`mobs_underdark`) and “The Minizones of the
Underdark” (`udmini`) remain distinct titles and source identities. Followed
WLD, ZON, MOB, OBJ, and QST histories stay on the same stem after the 2009
`areas/` relocation; the sidecar was added in 2026. No alternate-stem
predecessor or explicit replacement appears in these bounded paths. Keep
active `underdark` **DO NOT MOVE** and separate `underdark2011` as **RETAIN**.
See the [audit review](DURIAN_UNDERDARK_AUDIT_REVIEW.md#title-history-and-disposition),
[builder questions](../zone-stories/DURIAN_UNDERDARK.md#explicit-builder-questions-and-recommended-resolutions),
and [coverage row 324](ZONE_COVERAGE_REGISTER.csv#L324).

## Checkpoint 412: Undermountain title and bounded history

The normalized title “The Ruins of Undermountain” has one exact owner among
351 active ZON titles: `undermountain`. Followed WLD and ZON paths reach the
August 2009 area-tree relocation and later same-stem edits; no distinct
predecessor or replacement is identified in these bounded paths. Broader
semantic alias and map-identity review remains separate. Keep active
`undermountain` **DO NOT MOVE**. See the [audit review](UNDERMOUNTAIN_AUDIT_REVIEW.md#active-title-and-tracked-history),
[builder questions](../zone-stories/THE_RUINS_OF_UNDERMOUNTAIN.md#explicit-builder-questions-and-recommended-resolutions),
and [coverage row 325](ZONE_COVERAGE_REGISTER.csv#L325).

## Checkpoint 413: Underworld title and bounded history

The normalized active title “The Underworld” is shared by `underworld` and
`underworld3`, separate active contributors with ranges #4401–#4645 and
#30000–#30282. Preserve this as a naming overlap; it establishes no merge,
predecessor, or replacement. Bounded same-stem WLD/ZON history reaches the
2009 area relocation and later edits; the story sidecar was added in 2026.
Keep both active sources distinct and **DO NOT MOVE**. See the [audit review](UNDERWORLD_AUDIT_REVIEW.md#title-history-and-disposition),
[builder questions](../zone-stories/UNDERWORLD.md#explicit-builder-questions-and-recommended-resolutions),
and [coverage row 326](ZONE_COVERAGE_REGISTER.csv#L326).

## Checkpoint 414: Underworld3 title and bounded history

The normalized title “The Underworld” is shared by separate active
contributors `underworld` (#4401–#4645) and `underworld3` (#30000–#30282).
Preserve this as a title overlap; it establishes no merge, predecessor, or
replacement. Keep `underworld3` active and **DO NOT MOVE**. See the
[Underworld3 source review](../zone-stories/UNDERWORLD3.md),
[Underworld title review](#checkpoint-413-underworld-title-and-bounded-history),
and [coverage row 327](ZONE_COVERAGE_REGISTER.csv#L327).

## Checkpoint 415: Unique Vault title and bounded history

The normalized active title “The Unique's Vault” has no repeated title in the
current active `.zon` list. Broader semantic alias and history review remains
separate; this title scan does not establish a predecessor or replacement.
Keep active `unique` **DO NOT MOVE** as the object owner. See the
[source-backed builder review](../zone-stories/UNIQUE.md),
[active ZON title](../../../areas/zon/unique.zon#L2), and
[coverage row 328](ZONE_COVERAGE_REGISTER.csv#L328).

## Checkpoint 416: Undead Outpost title and bounded history

The normalized active title “The Undead Outpost” has no repeated title in the
current active `.zon` list. The similar “The Human Outpost” title belongs to
the distinct active `outpost` contributor; lexical overlap does not establish
an alias, predecessor, or replacement. Broader history review remains
separate. Keep active `unoutpst` **DO NOT MOVE**. See the
[Undead Outpost source review](../zone-stories/UNDEAD_OUTPOST.md),
[Undead Outpost title](../../../areas/zon/unoutpst.zon#L2),
[Human Outpost title](../../../areas/zon/outpost.zon#L2), and
[coverage row 329](ZONE_COVERAGE_REGISTER.csv#L329).

## Checkpoint 417: Vargan II title and bounded history

The normalized active title `Vargan II` is unique among current zone titles.
The distinct active `vargan` area shares a proper name in the Vurlok story and
has a reciprocal boundary, but those facts establish no alias, predecessor,
or replacement. The prior history scan remains bounded to the reviewed WLD
and ZON revisions. Keep `v2` **DO NOT MOVE**. See the [Vargan II builder
questions](../zone-stories/V2_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[title/history audit](V2_AUDIT_REVIEW.md#title-history-and-disposition),
[active title](../../../areas/zon/v2.zon#L1), and
[coverage row 330](ZONE_COVERAGE_REGISTER.csv#L330).

## Checkpoint 418: Valoisian Castle title and bounded history

The normalized active title “The Valoisian Castle” is unique in the current
active list. The tracked WLD history shows no active-boundary endpoint change
or replacement; the reviewed ZON history includes the 2016 steel-key reset
addition but establishes no predecessor. Broader semantic/history review
remains separate. Keep `val` **DO NOT MOVE**. See the [Valoisian Castle
builder questions](../zone-stories/VALOISIAN_CASTLE_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[title and history audit](VALOISIAN_CASTLE_AUDIT_REVIEW.md#active-title-and-tracked-history),
[active title](../../../areas/zon/val.zon#L2), and
[coverage row 331](ZONE_COVERAGE_REGISTER.csv#L331).

## Checkpoint 419: Phantasmagoric Caverns title and bounded history

The normalized title “Phantasmagoric Caverns” is unique among current active
titles and all 442 `.zon` files. Tracked `valdrak.wld` history reaches the
2009 source move and 2013 edit; `valdrak.zon` history includes the 2015
quest-item flag overhaul. No alternate-stem rename, distinct predecessor, or
whole-zone replacement was established. Keep active `valdrak` **DO NOT MOVE**.
See the [Phantasmagoric Caverns builder
questions](../zone-stories/VALDRAK_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[title/history audit](VALDRAK_AUDIT_REVIEW.md#title-history-and-disposition),
[active title](../../../areas/zon/valdrak.zon#L2), and
[coverage row 332](ZONE_COVERAGE_REGISTER.csv#L332).

## Checkpoint 420: Valinhaven title and bounded history

The normalized title “Valinhaven” occurs once among the 442 current `.zon`
titles, in active `valinhav`. The active row's bounded name/alias and semantic
history review remains pending; title uniqueness alone establishes no
predecessor or replacement. Keep `valinhav` **DO NOT MOVE**. See the
[Valinhaven builder questions](../zone-stories/VALINHAV_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[active title](../../../areas/zon/valinhav.zon#L2), and
[coverage row 333](ZONE_COVERAGE_REGISTER.csv#L333).

## Checkpoint 421: Valley of Crushk title and bounded history

The normalized title “Valley of Crushk” is unique among all 442 scanned
`.zon` files. Tracked same-stem `.wld` and `.zon` history reaches the areas
tree move and later source edits; it establishes no alternate-stem rename,
distinct predecessor, or whole-zone replacement. Keep `valley_crushk`
**DO NOT MOVE**. See the [Valley of Crushk builder
questions](../zone-stories/VALLEY_CRUSHK_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[Checkpoint 172 title/history review](VALLEY_CRUSHK_AUDIT_REVIEW.md#title-and-bounded-history),
[active title](../../../areas/zon/valley_crushk.zon#L2), and
[coverage row 334](ZONE_COVERAGE_REGISTER.csv#L334).

## Checkpoint 422: Vargan title, history, and identity

The normalized active title “Vargan” has one owner among 351 non-empty active
titles. Followed same-stem `.wld` and `.zon` history records source moves and
later edits but no alternate-stem rename, distinct predecessor, or explicit
replacement. Vargan II has a separate active stem, title, zone, and room range;
shared story language does not establish that either replaces the other. Keep
both identities and keep `vargan` **DO NOT MOVE**. See the [Vargan builder
questions](../zone-stories/VARGAN_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[Checkpoint 104 title/history review](VARGAN_AUDIT_REVIEW.md#title-tracked-history-and-disposition),
[active title](../../../areas/zon/vargan.zon#L2), and
[coverage row 335](ZONE_COVERAGE_REGISTER.csv#L335).

## Checkpoint 423: Vecna's Tomb title, history, and disposition

After color-code and whitespace normalization, “Vecna's Tomb” occurs once
among all 442 `.zon` files and is unique among active titles. Same-stem WLD
history reaches the 2009 area-tree move and later edits; the inactive
`Duris3.wld` room-number matches are differently titled records, not
replacement evidence. No distinct predecessor or whole-zone replacement was
established. Keep `vecna` **DO NOT MOVE**. See the [Vecna builder
questions](../zone-stories/VECNAS_TOMB_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[Checkpoint 143 title/history review](VECNAS_TOMB_AUDIT_REVIEW.md#title-history-and-disposition),
[active title](../../../areas/zon/vecna.zon#L2), and
[coverage row 336](ZONE_COVERAGE_REGISTER.csv#L336).

## Checkpoint 424: Vehicles title, history, and disposition

The coverage register's normalized-title scan for active `vehicles` reports
no repeated active `.zon` title. Its plural title describes a collection of
ferries, mounts, and shipboard settings, while the source review identifies
separate transport and faction story groups. No distinct predecessor, whole-
zone replacement, or reason to split the current contributor follows from
that thematic breadth. Keep `vehicles` **DO NOT MOVE** while builder questions
about its route boundaries and constituent stories remain open. See the
[Vehicles builder questions](../zone-stories/VEHICLES_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[source title](../../../areas/zon/vehicles.zon#L2),
[source story review](VEHICLES_STORY_REVIEW.md#environmental-story-shape), and
[coverage row 337](ZONE_COVERAGE_REGISTER.csv#L337).

## Checkpoint 425: Verspin title and disposition

The coverage register's normalized active-title scan reports no repeated
`.zon` title for `Verspin`; its title/history field still leaves broader
semantic and source-history review pending. The current dossier maps a distinct
city with circus, services, waterways, and a gladiator district. No title match
or thematic resemblance alone establishes a predecessor or replacement.
Keep active `verspin` **DO NOT MOVE**. See the [Verspin builder
questions](../zone-stories/VERSPIN_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[source title](../../../areas/zon/verspin.zon#L2),
[source story map](../zone-stories/VERSPIN.md#evidence-and-review-boundary), and
[coverage row 338](ZONE_COVERAGE_REGISTER.csv#L338).

## Checkpoint 426: Verzanan title and bounded source history

The normalized active title Verzanan is unique. The available WLD history
shows the same two-room bookend contents by commit 71d731c5b in November 2013;
it does not preserve the earlier city build or an explicit retirement or
replacement decision. Source title resemblance does not establish that
Verspin replaces Verzanan: both remain separate full-area entries with
different source keys and maps ([Verzanan](../../../areas/zon/verz1.zon#L2),
[Verspin](../../../areas/zon/verspin.zon#L2)). The old human-hometown comment identifies
itself as a relic kept for posterity, while an old gates-of-Verzanan message
in specs.room.c sends the character to room #3600, not either Verz1 room.
Complete the bounded title/history review, retain the unresolved semantic
identity boundary, and do not merge or rename either contributor. See the
[Verzanan source review](../zone-stories/VERZANAN_STORY_REVIEW.md#title-history-and-replacement-boundary),
[current title](../../../areas/zon/verz1.zon#L2),
[hometown comment](../../../src/core/constant.c#L1748),
[room destination](../../../src/specs/specs.room.c#L638), and
[coverage row 339](ZONE_COVERAGE_REGISTER.csv#L339).


## Checkpoint 427: Voluntown title, history, and IceCrag relationship

After color-code removal and normalization, “The Royal Mausoleum of Castle
IceCrag” has one owner among the 351 active contributors. Followed Voluntown
WLD/ZON history begins with the same-stem source addition in commit
`e71c1369d` (2012-12-29) and continues with maintenance, key/access updates,
quest-item flags, and the Sanguine Song move and proc adjustment. The reviewed
history identifies no alternate-stem predecessor, title rename, or whole-area
replacement. The [prior IceCrag comparison](#checkpoint-233-icecrag-title-history-and-related-contributor)
already names Voluntown as the connected Mausoleum owner: IceCrag #97132 and
Voluntown #142400 are a reciprocal boundary between contributors with
separate room ranges, QST, resets, and source dossiers. The #22070 crown reset
from `cerebusp.obj` is a typed object import from the Pits of Cerberus, not
evidence of duplicated area ownership. Keep both active owners distinct and
DO NOT MOVE Voluntown. This bounded title/history review is complete;
command-target alias and owner disambiguation remains in its separate
coverage field. See the [Voluntown audit](ROYAL_MAUSOLEUM_OF_CASTLE_ICECRAG_AUDIT_REVIEW.md#title-history-and-disposition),
[title](../../../areas/zon/Voluntown.zon#L2),
[active manifest row](../../../areas/AREA#L404), and
[coverage row 340](ZONE_COVERAGE_REGISTER.csv#L340).


## Checkpoint 428: Crystalspyre and the planned Verzanan road

The normalized active title “Crystalspyre Mountains” has one owner among the
351 active contributors. Followed WLD/MOB/OBJ/ZON history stays on the
`vrzawild` stem through the tracked 2009 area-tree relocation and later
maintenance; no alternate-stem predecessor, title rename, or whole-area
replacement is established. Crystalspyre's own slab says the range is planned
as a road to Verzanan, and its Bloodguard names Verzanan's frontier. Current
`verz1` remains a separate active owner with two placeholder rooms and no
active Crystalspyre boundary. The text establishes a planned narrative link;
it does not establish a current passage, ownership merge, or replacement.
Keep both contributors separate and keep `vrzawild` DO NOT MOVE. Command-target
alias/owner disambiguation remains a separate name-review question. See the
[Crystalspyre audit](CRYSTALSPYRE_MOUNTAINS_AUDIT_REVIEW.md#title-history-and-relationship-to-verzanan),
[slab text](../../../areas/obj/vrzawild.obj#L33),
[Verzanan review](../zone-stories/VERZANAN_STORY_REVIEW.md), and
[coverage row 341](ZONE_COVERAGE_REGISTER.csv#L341).

## Checkpoint 429: Wander title, history, and Llzazan relationship

The normalized active title “The Wandering Fields” is unique. Followed
WLD/MOB/OBJ/ZON history remains on the wander stem from the tracked 2009
areas-tree relocation through same-stem updates. The root-room description
explicitly identifies the zone as a mobile-loading section for Llzazan; its
36 one-way room exits enter Llzazan, and two reset commands import mobile
#36011 from Arachdrathos Wilderness. Those dependencies tie separately owned
contributors together without showing a predecessor, replacement, or
ownership merge. Keep wander, llzazan, and arac_wild distinct and keep Wander
DO NOT MOVE. Alias and command-target disambiguation remain separate. See the
[Wander audit](WANDER_AUDIT_REVIEW.md#title-history-and-owner-boundary),
[staging room](../../../areas/wld/wander.wld#L1),
[imported mobile reset](../../../areas/zon/wander.zon#L26), and
[coverage row 342](ZONE_COVERAGE_REGISTER.csv#L342).

## Checkpoint 430: Wellvolen title, history, and owner boundary

After color-code removal, the active ZON title “Wellvolen” is unique among
the 351 active area titles. The followed WLD, MOB, OBJ, and ZON histories stay
on the wemic stem from the tracked 2009 areas-tree relocation through
same-stem edits. No alternate-stem predecessor, title rename, or whole-zone
replacement was identified. Command-target alias and owner disambiguation
remain separate from this title/history result.

Surface’s reciprocal boundary, WH’s one-way Incarnate dispersal edge, the
Shipyard contract’s use of local mace object #93501, and the WH QST rumor
about Wellvolen link independent content. The matching number #93501 also
names Wemic warrior mobile #93501 in a different prototype type; this
cross-type identity is not a duplicate or replacement claim. Keep Wemic,
Surface, WH, Shipyards, and inactive Surf as separate owners. See the
[Wellvolen audit](WEMIC_AUDIT_REVIEW.md#title-history-and-owner-boundary),
[active Wemic source](../../../areas/zon/wemic.zon#L1),
[typed Shipyard recipe](../zone-stories/SHIP_YARDS.md#source-stock-slots-caps-and-competing-consumers),
and [coverage row 343](ZONE_COVERAGE_REGISTER.csv#L343).

## Checkpoint 432: Winterhaven title, history, and owner boundary

After ANSI-color removal, the active ZON title “The City of Winterhaven” is
unique among current active titles. Followed WLD, MOB, OBJ, ZON, QST, and SHP
histories reach the August 2009 areas-tree move and remain on the `wh` stem;
the story sidecar has two same-stem revisions on October 2, 2026. This bounded
review identified no alternate-stem predecessor or whole-area replacement.
Winterhaven's many reciprocal and one-way routes, imported objects, foreign
reset/QST/shop records, and story relationships are dependencies, not proof of
duplicate ownership. Keep active `wh` DO NOT MOVE. Alias disambiguation remains
separate in coverage column 37. See the [Winterhaven audit](WINTERHAVEN_AUDIT_REVIEW.md#title-followed-history-and-owner-boundary),
[coverage row 345](ZONE_COVERAGE_REGISTER.csv#L345), and
[source dossier](../zone-stories/WINTERHAVEN.md).

## Checkpoint 433: Wildland Trails title, history, and owner boundary

After color-code removal, “The Wildland Trails” is unique among current active
ZON titles. Followed WLD, MOB, OBJ, and ZON histories reach the August 2009
areas-tree move and remain on `wildland_trails`; no alternate-stem predecessor
or whole-area replacement was identified. The “WD bypass road” phrase is a
special-assignment comment, not evidence of a renamed zone. Local room, mobile,
and object VNUMs overlap by type; mobile corpse #14600 is not the carved-key
object #14600. Two trail exits target #215745/#217245, which are also defined
by inactive Surf and Surface2011 sources with different room descriptions.
This candidate map ambiguity is not evidence of a duplicate active area. Keep
active `wildland_trails` DO NOT MOVE and preserve Surf/Surface2011 separately.
See the [Wildland Trails audit](WILDLAND_TRAILS_AUDIT_REVIEW.md#title-followed-history-and-owner-boundary),
[candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97),
[candidate row 98](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L98), and
[coverage row 346](ZONE_COVERAGE_REGISTER.csv#L346).

## Checkpoint 434: Turolopolis title, history, and owner boundary

The normalized active title “The Ruins of Turolopolis” belongs to active area
`willem`, zone 71. Followed WLD, MOB, OBJ, ZON, and QST histories remain on the
`willem` stem and reach the August 2009 areas-tree move; the same-stem story
sidecar was added on October 4, 2026. No alternate-stem predecessor, distinct
renamed source, or whole-area replacement was identified. `turolzoo` has its
own title, map, and local hunter contract;
its reciprocal connection is companion content. The inactive Surf approach
and shared-code room 7117 are arrival/dependency evidence rather than another
owner. Keep active `willem` DO NOT MOVE and preserve the already-curated Surf
source-only approach under candidate row 97. Alias disambiguation remains
separate in coverage column 37. See the [Turolopolis audit](TUROLOPOLIS_AUDIT_REVIEW.md#active-title-and-tracked-history-boundary),
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97),
[coverage row 347](ZONE_COVERAGE_REGISTER.csv#L347), and
[source dossier](../zone-stories/RUINS_OF_TUROLOPOLIS.md).

## Checkpoint 435: Shadowfall title, history, and owner boundary

After color-code and case normalization, “The Lost Realms of Shadowfall” is
unique among active ZON titles. Followed WLD, MOB, OBJ, ZON, QST, and SHP
histories stay on the `wolfen` stem and reach the August 2009 areas-tree move;
no alternate-stem predecessor, renamed source, or whole-area replacement was
identified. The local Wolfen Empire history does not name another active
owner. The similarly named Jade Empire and Mitashi titles are distinct active
contributors; no other current source text was found to claim Shadowfall or
the Llevron Dor history. Keep active `wolfen` DO NOT MOVE. Alias disambiguation
remains separate in coverage column 37. See the [Shadowfall audit](WOLFEN_AUDIT_REVIEW.md#active-title-and-tracked-history-boundary),
[coverage row 348](ZONE_COVERAGE_REGISTER.csv#L348), and
[source review](../zone-stories/WOLFEN_STORY_REVIEW.md).

### Checkpoint 438 — The Temple to Skrentherlog owner boundary

The normalized active title “The Temple to Skrentherlog” belongs to area
yuan_ti and is unique among the 351 active contributors. The bounded WLD/ZON
histories show same-stem maintenance through the 2009 area-tree move and no
distinct predecessor or replacement in the curated register. Neighboring
Yuan-Ti Village (yuan_ti2) has a separate title, source set, and reciprocal
boundary. Keep active yuan_ti DO NOT MOVE. See the
[Temple audit](TEMPLE_TO_SKRENTHERLOG_AUDIT_REVIEW.md#title-history-and-disposition),
[coverage row 351](ZONE_COVERAGE_REGISTER.csv#L351), and
[source dossier](../zone-stories/THE_TEMPLE_TO_SKRENTHERLOG.md).

### Checkpoint 440: Aeon Order Guildhall and Genesis VNUM collision

“Aeon Order Guildhall” (`aeon_gh`) and “genesis hall” (`genesis`) are distinct,
normalized titles; each appears once among 442 current `.zon` titles. Their
source maps nevertheless claim the same zone #299, rooms #29900–#29909, and
mobile prototypes #29900/#29901. The records are not copies: Aeon #29900 is a
mist golem and #29901 a merchant, while Genesis uses gargoyles at those two
VNUMs. Their room prose, shopkeepers, and reset lineups also differ. Neither
stem is listed in `areas/AREA` or `areas/qst/AREA`; `areas/AREA` contains only
the raw `*OPEN 299` annotation. Both are already separate **RETAIN IN PLACE**
attic candidates, Aeon row 2 and Genesis row 46.

The same room-number conflict reaches compiled code: the Players Guild block
assigns the `inn` handler to `real_room0(29903)` in
[`specs.assign.c`](../../../src/specs/specs.assign.c#L2452). Both candidate WLD
files define #29903 differently. The bounded histories retain separate stems
through same-stem edits and the 2009 area-tree move; they establish no
alternate-stem rename or explicit replacement. The source evidence leaves
range and code ownership unresolved, so neither candidate's coverage row is
reclassified as an active area and neither candidate is cleared to move. Keep
the two candidates separate pending an owner decision. See the [Aeon audit](AEON_GH_AUDIT_REVIEW.md#vnum-collision-title-and-history),
[Aeon candidate row 2](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L2),
[Genesis candidate row 46](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L46),
and [coverage row 353](ZONE_COVERAGE_REGISTER.csv#L353).

### Checkpoint 441: Afterlife Guildhall and active Torg identity

The normalized `.zon` title “The Afterlife Guildhall” is unique among 442
current titles. Its WLD, MOB, OBJ, ZON, QST, and SHP histories remain on the
`afterlife_gh` stem through the 2009 tree move and later same-stem edits; the
map JSON is a separate 2025 editor asset. No alternate-stem rename or explicit
whole-zone replacement is established. The existing source-only Guildhall
candidate remains distinct from the two `Afterlife GC OP` candidates, which
have different room ranges and own four objects imported by the Guildhall.

All ten Guildhall room VNUMs #29100–#29109 also have active definitions in
`torg.wld`, with different titles and content. The Guildhall source does not
replace Torg by matching identity or history; its unregistered rooms collide
with active Torg's VNUM ownership. The Players Guild code block binds `inn` to
`real_room0(29103)`, which is Torg's escape tunnel under the current full area
list and the Guildhall's basement entrance in its source. Keep the sources
separate and the existing `afterlife_gh` candidate **RETAIN IN PLACE** until
the VNUM owner and code target are confirmed. See the [Afterlife audit](AFTERLIFE_GH_AUDIT_REVIEW.md#authored-map-and-torg-vnum-overlap),
[candidate row 3](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L3),
[coverage row 354](ZONE_COVERAGE_REGISTER.csv#L354), and the separate
[`afterlife_op`](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L4) and
[`afterlife_op2`](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L5) candidates.

### Checkpoint 442: Afterlife OP and VNUM ownership conflicts

The normalized `.zon` title “Afterlife GC OP” occurs twice among 442 current
zone titles, on `afterlife_op` and the distinct `afterlife_op2` candidate.
OP uses #29110–#29112; OP2 uses #29120–#29128. Their shared title and zone
header #1 do not establish a merge, rename, or replacement. Followed WLD, OBJ,
and ZON histories stay on the `afterlife_op` stem through the 2009 tree move
and later same-stem changes; its editor map is a separate 2025 asset.

All three OP room VNUMs also have different definitions in active `torg.wld`
and unregistered `void_gh.wld`. The repeated IDs are map-owner conflicts, not
matching authored maps. OP's empty MOB file declares no prototypes, while its
ZON uses #29101–#29104, IDs found in `afterlife_gh`, `scrubs_hall`,
`solstice_gh`, `void_gh`, and `wlust_gh` MOB files. Keep those source owners
separate; no alternate-stem identity or whole-zone replacement is established.

The two local type-25 portal objects add a destination conflict: #29110
stores #29106 despite naming Khomani-Kan; #29111 stores #29110 despite naming
the Goodie Continent. The generic item source interprets `value[0]` as a room
VNUM, and each stored number has competing source ownership. Preserve these
values as unresolved target decisions rather than treating the prose as a
verified route. Keep existing `afterlife_op` candidate row 4 **RETAIN IN PLACE**.
See the [Afterlife OP audit](AFTERLIFE_OP_AUDIT_REVIEW.md#room-number-and-source-identity-conflicts),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-442-afterlife-op-source-graph-and-portal-boundaries),
[candidate row 4](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L4), and separate
[`afterlife_op2`](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L5) and
[`void_gh`](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L119) candidates.


### Checkpoint 443: Afterlife GC OP2 title, history, and room ownership

The normalized ZON title “Afterlife GC OP” is shared by exactly two of the
442 current zone titles: afterlife_op and afterlife_op2. Their zone header
numbers match at #1, but OP uses rooms #29110–#29112 and OP2 uses #29120–#29128.
Followed OP2 WLD, OBJ, and ZON histories remain on the afterlife_op2 stem
through the 2009 area-tree move and later same-stem edits; the tracked MOB is
empty, and the editor map is a separate 2025 asset. No alternate-stem rename
or explicit whole-zone replacement is established.

All nine OP2 room IDs also have different definitions in active torg.wld.
Active Torg's fourteen direction records targeting #29120–#29128 belong to
those active Torg rooms; they do not establish OP2 ingress. OP2's two D4
source exits target unlisted Duris3 rooms #172709 and #163943. Keep the two
same-title candidates and active Torg source separate, and keep candidate
row 5 **RETAIN IN PLACE** pending room ownership, portal targets, and external
editor/persisted-state closure. See the [OP2 audit](AFTERLIFE_OP2_AUDIT_REVIEW.md#identity-and-source-history),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-443-afterlife-op2-three-components-and-source-only-boundaries),
[candidate row 5](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L5), and
[coverage row 356](ZONE_COVERAGE_REGISTER.csv#L356).

### Checkpoint 444: Alatorin object-bank history and overlap

The object-only alatorin_objects file is not a separate mapped zone. Commit
60f9f14 (2010-01-26) added it and a temporary areas/AREA stub to load old
Alatorin objects while the zone was reviewed; the commit message says missing
player-held objects were causing crashes. Commit 30db49c (2010-06-08) removed
the stub and restored active Alatorin. The OBJ file remained and received
same-path edits through 2026. It has no current AREA entry or zone title.

All 408 of its object VNUMs are also defined in active alatorin.obj. Half the
candidate records match after line-ending normalization, and half differ.
Another 168 IDs overlap different Nexus56 object prototypes; Nexus56 is not in
the current full area list. These are object-version and numeric-ownership
conflicts, not evidence of a replacement zone. Preserve candidate row 6
**RETAIN IN PLACE** while player-object identity and typed references remain
unresolved. See the [object-bank audit](ALATORIN_OBJECTS_AUDIT_REVIEW.md#historical-loader-status-and-object-identity),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-444-alatorin-object-bank-and-typed-travel-targets),
[Alatorin candidate row 6](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L6),
[Nexus56 candidate row 79](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79), and
[coverage row 357](ZONE_COVERAGE_REGISTER.csv#L357).

### Checkpoint 445: Alterian Mountains and Mining identity

Unregistered `alterian_mountains` uses zone number 49, the same number as
active Mining, but its title is “Alterian Region - Mountains” and its source
records differ. The maps share 49 room VNUMs (#4920–#4968); all 49 Mountain
room records differ from the same-numbered Mining records. Mountain object
#4900 is a `nothing` prototype, while active Mining's #4900 is a reset wooden
crate. These same-number conflicts are not evidence of a copied or replaced
zone.

The bounded Mountain file histories show the older directory move, local
mobile/reset-format edits, and a 2013 source revision. They show no
alternate-stem predecessor or whole-zone replacement. Keep candidate row 7
**RETAIN IN PLACE**, distinct from both active Mining and the source-only
Alterian Wilderness. See the [Mountain audit](ALTERIAN_MOUNTAINS_AUDIT_REVIEW.md#numeric-identity-and-bounded-history),
[Mountain dossier](../zone-stories/ALTERIAN_MOUNTAINS.md),
[candidate row 7](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L7),
[coverage row 358](ZONE_COVERAGE_REGISTER.csv#L358), and the
[Mining supplement](MINING_AUDIT_REVIEW.md#inactive-wld-sweep-and-overlapping-sources).

### Checkpoint 446: Alterian Wilderness map and prototype identities

Unregistered `alterian_wilderness` uses unique current ZON header number 47
and title “Alterian Region - Wilderness.” Its 130-room WLD overlaps active
Tchan at nine room numbers (#4801–#4809) and active Mining at 20
(#4900–#4919); all 29 paired room records differ. Mobile and object #4800
also have separate Wilderness and Tchan definitions. These collisions do not
show a predecessor, shared live map, or whole-zone replacement.

The bounded tracked-path history shows a 2009 storage move, 2012
MOB/OBJ-format edits, 2013 source revisions, and a 2015 ZON quest-item-flag
update. It shows no alternate-stem predecessor or whole-zone replacement.
Keep candidate row 8 **RETAIN IN PLACE**, distinct from active Tchan/Mining
and from source-only Alterian Mountains and Surf. Preserve its reciprocal
source links and cross-file resets without inferring runtime activation. See
the [Wilderness audit](ALTERIAN_WILDERNESS_AUDIT_REVIEW.md#room-and-prototype-identities),
[story dossier](../zone-stories/ALTERIAN_WILDERNESS.md),
[candidate row 8](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L8), and
[coverage row 359](ZONE_COVERAGE_REGISTER.csv#L359).

### Checkpoint 447: AoD GH map and zone identities

Unregistered `aod_gh` uses zone header #296 and title “AoD GH.” The current
ZON scan finds no other #296 or normalized title match. Three other
unregistered sources use header #295 under different titles:
`genocide_hall` (“Genocide Guild Hall”), `legion` (“Legion Guildhall”), and
`void_gh` (“Main Guildhall for The Void”). These are competing zone-number
identities, not evidence that any one file replaced another.

AoD rooms overlap `genocide_hall` at #29500–#29504 and #29510–#29511,
`legion` at #29500–#29512 and #29520–#29522, and `rf_op` at #29510–#29512.
All 26 paired room records differ. Forty direction records from these three
maps point at numbers in the AoD range, while those numbers are defined as
rooms in their own source maps. None of the maps is in the current full area
list. AoD mobiles #29500–#29502 overlap same-number mobile prototypes in
Genocide Hall and Legion, with distinct definitions. AoD objects #29500–#29503
also overlap Legion, including a different #29500 badge/portal identity.

Compiled source binds an `inn` function to numeric room #29502 under a Players
Guild comment. AoD names #29502 “The Inner Sanctum”; Legion has a different
room there, and Genocide Hall also defines #29502. The source does not
identify which unlisted room definition the callback was intended to serve.
Tracked AoD WLD history shows the 2009 directory move, a one-word spelling
correction in 2011, and a 2013 source revision. The 2011 commit message
mentions a “Temple of Flames” rename, but its AoD WLD diff only changes
`painfull` to `painful`; it does not establish a zone-title change.
MOB/OBJ/ZON also show the 2012 reset-format change, and the 2026 ZON edit
adds gathering-bag resets. The tracked same-stem histories show no
alternate-stem predecessor or whole-zone replacement. Keep candidate row 9
**RETAIN IN PLACE** as a separate source identity. See the [AoD audit](AOD_GH_AUDIT_REVIEW.md#room-and-prototype-identities),
[story dossier](../zone-stories/AOD_GH.md),
[candidate row 9](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L9), and
[coverage row 360](ZONE_COVERAGE_REGISTER.csv#L360).

### Checkpoint 448: Aopal Tharnadian Causeway title, history, and room identities

Unregistered `aopal` uses current ZON header #433 and title “The Tharnadian
Causeway.” The current ZON scan found no other #433 header or normalized
title match. The `*aopal` notes in `areas/AREA` and `areas/qst/AREA` describe
a road connected to Tharnadia but do not establish an active route there.
The followed WLD, MOB, OBJ, and ZON paths reach the 2009 directory move; MOB
and OBJ show a 2012 format update and a 2013 same-stem revision, while the
tracked 2013 WLD and ZON diffs are empty. ZON history includes a 2015
quest-item-flag edit. No alternate-stem predecessor or whole-zone
replacement appears in these tracked paths.

Active `shipy` and Aopal both define rooms #43301–#43328, and all 28 paired
WLD records differ. The maps retain separate Tharnadian Causeway and Ship
Yards identities even where numeric exits from unique Aopal rooms target
Ship Yards VNUMs. Aopal MOB #43301–#43315 and OBJ #43301–#43305 have no other
current same-type prototype definitions in the scanned source set. The
room-number collision and source-only edges do not establish replacement or
co-loading intent. Keep candidate row 10 **RETAIN IN PLACE**. See the
[Aopal audit](AOPAL_AUDIT_REVIEW.md#title-numeric-identity-and-tracked-history),
[story dossier](../zone-stories/AOPAL.md),
[candidate row 10](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L10), and
[coverage row 361](ZONE_COVERAGE_REGISTER.csv#L361).

### Checkpoint 449: Astral placeholder and room #1 identities

Unregistered `astral` uses ZON header #1 and title “The Horrible Tracts of
the Unnamed.” Header #1 appears in 30 current ZON files; the exact normalized
title appears in five: `astral`, `eoo_op2`, `mtncastl`, `theurgist`, and
`ud-inside`. Active Astral Main, Astral Areas, and Astral Tiamat are separate
listed stems using headers 197–199.

Twelve WLD sources define room #1, but only `limbo.wld` is active. Astral's
blank #1 WLD block is byte-identical to the placeholder records in
`mtncastl.wld` and `ud-inside.wld`; identical content does not establish a
shared owner or replacement. The source code reserves #1 as Limbo. Followed
same-stem WLD/MOB/OBJ/ZON history shows no explicit whole-zone replacement in
the tracked paths. Keep candidate row 11 **RETAIN IN PLACE** while the owner
resolves source purpose, map/editor ownership, and persisted/runtime state.
See the [Astral audit](ASTRAL_AUDIT_REVIEW.md#title-and-tracked-history),
[story dossier](../zone-stories/ASTRAL.md),
[candidate row 11](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L11), and
[coverage row 362](ZONE_COVERAGE_REGISTER.csv#L362).

### Checkpoint 450: Asunwa title, history, and source identities

Unregistered `asunwa` uses ZON header #353 and normalized title “Lair of the
Asunwa Jaguar”; each is unique in the current source scan. Its #35300–#35407
room identities do not overlap an active WLD owner. Followed WLD/MOB/OBJ/ZON/
QST/SHP histories reach the 2009 area-tree move and record later format,
cleanup, item-flag, room-flag, and quest-reward edits, including the 2016 jade
plaque restoration. These paths show no explicit alternate-stem predecessor
or whole-zone replacement.

Keep the two external SHP records that use mobile #35300, object #35301, and
room #35303 distinct from Asunwa's own shop. Also preserve the empty
`player_guilds.qst` record #35320 separately from the local Anan mobile/shop
identity. Neither numeric reuse nor same-stem history establishes migration
ownership. Keep candidate row 12 **RETAIN IN PLACE** pending owner resolution
of the mixed manifests and typed references. See the
[Asunwa audit](ASUNWA_AUDIT_REVIEW.md#title-history-and-safe-disposition),
[story dossier](../zone-stories/ASUNWA.md),
[candidate row 12](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L12), and
[coverage row 363](ZONE_COVERAGE_REGISTER.csv#L363).

### Checkpoint 451: Bastard Die Hall room ownership and history

Unregistered `bastard` has ZON header #302 and normalized title “Die hall.”
The title is unique in the 442-file scan; header #302 also occurs in
unregistered `sunhall`. Active `underworld3` has header #300 with an AREA
range spanning #30000–#30399 and independently defines all five
#30200–#30204 room numbers as mining tunnels. Every pair of room definitions
differs. The `bastard` and `sunhall` candidates also share those five room
numbers, three mobile VNUMs, object #30200, and SHP keeper key #30201, with
different authored content. None of these overlaps establishes which
unregistered area was intended for a path or replacement relationship.

Followed `bastard` WLD/MOB/OBJ/ZON/SHP histories reach the 2009 area-tree move;
MOB/OBJ/ZON record the 2012 field update, and all five same-stem files appear
in the 2013 “stuff” commit. No explicit alternate-stem predecessor or
whole-zone replacement appears in those histories. Keep candidate row 13
**RETAIN IN PLACE** and preserve the active Underworld3 room owner and
unregistered Sunhall definitions as separate identities. See the
[Bastard audit](BASTARD_AUDIT_REVIEW.md#title-history-and-safe-disposition),
[story dossier](../zone-stories/BASTARD.md),
[candidate row 13](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L13), and
[coverage row 364](ZONE_COVERAGE_REGISTER.csv#L364).

### Checkpoint 452: BB GH title, history, and source identity

Unregistered bb_gh has normalized ZON title “BB GH,” unique among the 442
current ZON files; header #1 is shared by 30 files. Its rooms #30300–#30343
overlap unregistered vodgh (#30300–#30313) and venom (#30300–#30309). The
same candidates overlap bb_gh mobiles #30300–#30302; vodgh also overlaps
objects #30300/#30301 and has its own distinct SHP record keyed to #30301.
No active WLD, mobile, or object definition at the bb_gh local IDs was found.
These matching numbers and guild-hall titles do not establish that the three
sources are alternate drafts or replacements.
Their raw ZON titles, “BB GH,” “VoDsGH,” and “Venom GH,” are distinct.

Followed bb_gh file histories reach the 2009 areas-tree move. WLD has
same-stem 2013 and 2026 edits, OBJ a 2016 same-stem edit, and ZON/SHP 2013
and 2026 same-stem edits. The reviewed histories contain no alternate-stem
predecessor or whole-zone replacement. Preserve the three unregistered
source identities and resolve the numeric owners before any path move. See
the [BB GH audit](BB_GH_AUDIT_REVIEW.md#title-history-and-disposition),
[story dossier](../zone-stories/BB_GH.md),
[vodgh candidate row 118](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L118),
[venom candidate row 117](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L117),
[candidate row 14](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L14), and
[coverage row 365](ZONE_COVERAGE_REGISTER.csv#L365).

### Checkpoint 453: BGTK shop-source history

Candidate bgtk is an SHP-only source with no ZON title or AREA annotation.
Its three shop records reference active Barrow rooms and some active Barrow
objects, but two keeper mobile VNUMs and six stock object VNUMs have no
current matching prototypes. Followed bgtk.shp history reaches the 2009
areas-tree move and a 2013 same-stem edit; no alternate-stem predecessor or
whole-area replacement is established. The typed links and fallback-loader
behavior are dependency findings, not evidence that BGTK replaces Barrow or
another zone. See the
[BGTK audit](BGTK_AUDIT_REVIEW.md#history-and-disposition),
[Barrow audit](ELFTOMB_AUDIT_REVIEW.md#checkpoint-453-bgtk-shop-loader-reconciliation),
[candidate row 16](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L16), and
[coverage row 366](ZONE_COVERAGE_REGISTER.csv#L367).

### Checkpoint 454: Brass Old 1 is the archived +100 Brass predecessor

The normalized “Plane of Fire, Brass” title appears in four current ZON files:
`brass`, `brass-old-1`, `brass-old-2`, and `brass-old-3`. Title equality does
not collapse these candidate rows. For `brass-old-1`, history is explicit:
the 2012 `e71c1369` commit adds New Brass and disconnects old Brass; the 2025
`0e925fa9` commit changes the `areas/AREA` contributor from `New_Brass_2012`
to `brass` and retains that source family under the `brass-old-1` paths.

Subtracting 100 from its local VNUMs maps all 144 old mobiles, all 150 old
objects, all seven QST giver keys, and all 18 shop keys to current Brass
records. The WLD maps 356 of 357 rooms and 633 of 821 exit records; the
active map also has one extra room. Five QST blocks match after translation;
the two differences are the old 3,000,000-copper Herl amount and legacy
`R I 55271` command. This is evidence that `brass-old-1` is the archived
predecessor of active Brass, while `brass-old-2` and `brass-old-3` remain
separate archived sources. The active title and story dossier remain with
`brass`; keep candidate row 17 RETAIN IN PLACE until source-only typed
references, full-mode shop fallback, persisted identity, and editor/tool
ownership are resolved. See the
[Brass Old 1 audit](BRASS_OLD_1_AUDIT_REVIEW.md#relationship-to-active-brass),
[builder questions](BRASS_OLD_1_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[candidate row 17](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17), and
[coverage row 368](ZONE_COVERAGE_REGISTER.csv#L368).

### Checkpoint 455: Brass Old 2 is the former 250-range Plane of Fire One source

At the parent of the 2025 relabel commit `0e925fa9`, the active-list entry
`plane_fire_one *250-251` pointed at the 250-range source. The commit changed
the current `areas/AREA` entry to `plane_fire_one *254-255` and added the
former WLD, MOB, OBJ, and ZON files as `brass-old-2`; all four added files are
byte-identical to their parent-revision `plane_fire_one` blobs. This confirms
that Old 2 is a preserved former 250-range source, distinct from the current
254-range `plane_fire_one`, Old 1's `New_Brass_2012` predecessor, and Old 3's
former `plane_fire_two` source. The current Fire review's history for the
254-range source is at [Checkpoint 323](#checkpoint-323-plane-of-fire-title-and-followed-history).

The QST index still says `plane_fire_one *250-251`, but there was no
`areas/qst/plane_fire_one.qst` at the commit's parent. The current file is a
100%-identical rename from `plane_fire_three.qst` and starts at #25401. The
index row therefore does not establish a local Old 2 QST file or a QST
content relationship. Keep the source identities and unresolved QST index
ownership separate. Candidate row 18 remains RETAIN IN PLACE because Potions
and Realm still author seven active exits to its room #25087, its ZON imports
66 Old 3 prototypes, and current code names Old 2 VNUMs. See the
[Brass Old 2 audit](BRASS_OLD_2_AUDIT_REVIEW.md#followed-history-and-the-qst-index),
[builder questions](BRASS_OLD_2_BUILDER_QUESTIONS.md),
[candidate row 18](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L18), and
[coverage row 369](ZONE_COVERAGE_REGISTER.csv#L369).

### Checkpoint 456: Brass Old 3 is the preserved former Plane of Fire Two source

At the parent of the 2025 relabel commit 0e925fa9, areas/AREA listed
plane_fire_one *250-251, plane_fire_two *252-253, and
plane_fire_three *254-255. The commit retains all five former
plane_fire_two files as brass-old-3; Git reports 100% renames and the
parent/current blob IDs match for WLD, MOB, OBJ, ZON, and QST. Current active
plane_fire_one *254-255 is the former third range. Old 2 is the separate
former first range; Old 1 is the separate +100 New Brass predecessor of active
brass. Matching ZON titles do not merge these identities.

Unlike Old 2, the old plane_fire_two.qst existed at the parent and was moved
with the other four files. The current areas/qst/AREA still lists
plane_fire_two *252-253, while the current file is brass-old-3.qst; record
this as an orphaned legacy manifest entry, not a reason to change the index or
reactivate the component in this review.

Old 3 QST giver #25224 repeats the Yodono dialogue and head-item pattern also
found in active Brass giver #139124 and archived Old 1 giver #139224. Old 3
retains R I #55271 and two local vial receive references; active Brass’s
corresponding block does not require #55271. This is shared quest text and
source-only dependency evidence, not proof of a whole-area replacement. Keep
Old 3 separately RETAIN IN PLACE pending builder, QST-index, WLD/reset,
code, editor, and persisted-identity closure. See the
[Old 3 audit](BRASS_OLD_3_AUDIT_REVIEW.md#followed-history-and-neighboring-brass-sources),
[builder questions](BRASS_OLD_3_BUILDER_QUESTIONS.md),
[candidate row 19](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L19), and
[coverage row 370](ZONE_COVERAGE_REGISTER.csv#L370).

### Checkpoint 457: BSGHOP room reuse and identity

Tracked history shows BSGHOP WLD at the same stem from the 2009 area-tree move; its MOB, OBJ, and
ZON also have later same-stem changes. No alternate-stem rename or explicit replacement decision
was identified in the followed paths. The unregistered darkop WLD repeats BSGHOP's room prose and
most exits for #29440–#29443, and darkop.obj has a second portal object with the same prose but
different numeric fields. This establishes shared content, not which source is canonical or whether
either replaced the other.

All four BSGHOP room and mobile VNUMs overlap active Harrow records with different identities;
BSGHOP object #29440 likewise overlaps Harrow's painting. The Tundra sidecar's object #29440
contract and C/C++ equipment slots for #29443 currently resolve to Harrow objects. Keep these
numeric collisions distinct from historical replacement evidence. Candidate row 20 remains RETAIN
IN PLACE pending owner, map, portal, zone, content, editor/tooling, and persisted-identity review.
See the [BSGHOP audit](BSGHOP_AUDIT_REVIEW.md#followed-history-and-replacement-review),
[builder questions](BSGHOP_BUILDER_QUESTIONS.md),
[candidate row 20](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L20), and
[coverage row 371](ZONE_COVERAGE_REGISTER.csv#L371).

### Checkpoint 458: Cabal Guildhall and Torg identity

Followed `cabal` history shows same-stem source paths, with no alternate-stem rename or explicit
replacement decision found. The unregistered `cabal_hall` zone has the near-identical title “The
Cabal Guildhall,” but it uses VNUMs #30100–#30109 and different room/mobile/shop content; recent
ZON and SHP edits do not prove it replaced `cabal`. Active `cbl_hall` is a separate registered Clan
BloodLust source.

All Cabal room VNUMs #29000–#29012 and mobile VNUMs #29000–#29002 overlap active Torg identities
with different content. Its shop record also uses keeper #29001 and room #29003, which the active
Torg catalog assigns to a drunk guard and an inner-compound room. Keep these number collisions
distinct from historical replacement evidence. Candidate row 21 remains RETAIN IN PLACE pending
owner, map, shop, title, editor/tooling, and persisted-identity review. See the
[Cabal Guildhall audit](CABAL_GUILDHALL_AUDIT_REVIEW.md#active-torg-ownership-of-the-same-room-and-mobile-vnums),
[builder questions](CABAL_GUILDHALL_BUILDER_QUESTIONS.md),
[candidate row 21](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L21), and
[coverage row 372](ZONE_COVERAGE_REGISTER.csv#L372).

### Checkpoint 459: Cabal Hall source identities and history

Followed Git paths show same-stem Cabal Hall WLD/MOB history, with the WLD last changed in 2013.
The ZON and SHP were edited in September 2026 to add gathering-bag VNUMs #491–#494; the tracked OBJ
file is empty. No alternate-stem rename or explicit replacement decision was found.

The Cabal Hall title “The Cabal Guildhall” closely matches unregistered `cabal`, which has different
room VNUMs and story content. Cabal Hall room VNUMs #30100–#30109 also overlap active Underworld3
rooms (“large underground lake/river” identities), as well as inactive `nah` and `shadow_hall` map
records. These source overlaps do not establish that one component replaced another. Candidate row 22
remains RETAIN IN PLACE pending owner, map, code, shop, content, editor/tooling, and persisted-identity
review. See the [Cabal Hall audit](CABAL_HALL_AUDIT_REVIEW.md#title-history-and-neighboring-guildhall-sources),
[builder questions](CABAL_HALL_BUILDER_QUESTIONS.md),
[candidate row 22](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L22), and
[coverage row 373](ZONE_COVERAGE_REGISTER.csv#L373).

### Checkpoint 460: Casino and Hall source identities

Followed Git paths show Casino WLD/MOB at the 2009 areas-tree move, same-stem OBJ changes through
2013, and a same-stem ZON edit in 2015. No alternate-stem rename or explicit replacement decision
was found in these paths.

Active Hall of the Ancients owns Casino's entire #77801–#77815 room range as a different map. Its
23 exits that numerically target the range and Hall object #77714's #77813 destination refer to
Hall's active room identities; they do not establish that Hall replaced Casino. Keep the two source
identities separate pending the owners' account of VNUM ownership, editor data, and persisted room
identity. Candidate row 23 remains RETAIN IN PLACE while the two imported #45 reset references, door
state comments/values, and intended activity are unresolved. See the
[Casino audit](CASINO_AUDIT_REVIEW.md#followed-history-and-replacement-review),
[builder questions](CASINO_BUILDER_QUESTIONS.md),
[Hall story map](../zone-stories/THE_HALL_OF_THE_ANCIENTS.md),
[candidate row 23](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L23), and
[coverage row 374](ZONE_COVERAGE_REGISTER.csv#L374).

### Checkpoint 461: CBL Outpost 2 source identities

Followed paths show the `cbl_outpost2` WLD, MOB, and ZON at the 2009 areas-tree move and same-stem
maintenance through the 2013 “stuff” commit; no alternate-stem rename or explicit replacement
decision was found. The unregistered `Monsteri` source has the same zone title, room/mobile VNUMs,
and four-mobile reset pattern. Its room prose and active CBL Hall's #29820–#29824 rooms share the
same content family, but no reviewed history establishes a canonical source.

The candidate room range #1000–#1004 has no active WLD owner or active inbound WLD edge. Its mobile
VNUMs #1000–#1003 collide with different active Heavens prototypes (one “unkillable bastard” and
three peasants). Keep room, mobile, and source-map identities separate until the owners reconcile
the legacy zone numbering, copy lineage, active prototype ownership, and persisted references. Row
24 remains RETAIN IN PLACE. See the
[CBL Outpost 2 audit](CBL_OUTPOST2_AUDIT_REVIEW.md#followed-history-and-replacement-review),
[builder questions](CBL_OUTPOST2_BUILDER_QUESTIONS.md),
[active CBL Hall audit](CBL_HALL_AUDIT_REVIEW.md#title-history-and-duplicate-content-review),
[candidate row 24](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L24), and
[coverage row 375](ZONE_COVERAGE_REGISTER.csv#L375).

### Checkpoint 462: BattleRager Hall, Newbie, and hall source identities

Followed `cbr_hall` history shows same-stem WLD/MOB/OBJ/ZON/SHP paths at the 2009 areas-tree move;
the ZON and SHP received 2026 same-stem edits that add Heavens gathering bags #491–#494. No
alternate-stem rename or explicit replacement decision was found.

The candidate's ten room VNUMs #29300–#29309 and its mobile/object VNUMs #29300–#29303 overlap
active Newbie records for different room, character, and object identities. The compiled inn at
#29305 and Ailvio mobile constants #29303/#29304 currently correspond to Newbie records. Several
other inactive WLDs reuse the same room-number band. `cbr_hall` is Clan BattleRage, distinct from
active `cbl_hall` of Clan BloodLust; the similar filename prefix does not establish a rename or
replacement. Preserve these source identities pending map, code, shop, and persisted-reference
decisions. See the
[BattleRager Hall audit](CBR_HALL_AUDIT_REVIEW.md#active-newbie-ownership-of-the-same-room-mobile-and-object-vnums),
[builder questions](CBR_HALL_BUILDER_QUESTIONS.md),
[active CBL Hall audit](CBL_HALL_AUDIT_REVIEW.md),
[candidate row 25](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L25), and
[coverage row 376](ZONE_COVERAGE_REGISTER.csv#L376).

### Checkpoint 463: Yggdrasil Outpost and Circle Guildhall source identities

The unregistered `circle_op` source is titled “Yggdrasil Outpost.” Its three
WLD blocks #45410–#45412 exactly match the same-numbered blocks inside the
larger unregistered `circlegh` “Yggdrasil Guild Hall” map. Its four MOB
prototypes #45410–#45413 and two OBJ prototypes #45410–#45411 are also exact
copies of the corresponding `circlegh` records. This is strong shared-content
evidence, but it does not identify a canonical map or establish that one
source can replace the other. Both WLD paths are present since the 2009
area-tree move and continue under their own stems; followed history shows no
rename or explicit replacement decision.

The outpost has a different ZON title and raw zone value, and only three rooms.
It also uses object #45410 whose destination #45407 is defined only in
`circlegh.wld`; its other portal object points to #45412 while its prose names
“Goodie Cont.” Preserve these source identities and typed relationships pending
an owner-approved map.

The other unregistered Yggdrasil guildhall sources `rf_gh` (#28900–#28912)
and `ygg` (#30400–#30412) have different VNUM bands; their similar titles do
not establish a rename or common room identity. The earlier [`ygg` review](ARDGRAL_AUDIT_REVIEW.md)
documents a separate Ard’gral collision. See the
[`circle_op` audit](CIRCLE_OP_AUDIT_REVIEW.md#exact-shared-records-with-the-separate-circlegh-source),
[27 builder questions](CIRCLE_OP_BUILDER_QUESTIONS.md),
[candidate row 26](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L26),
[coverage row 377](ZONE_COVERAGE_REGISTER.csv#L377), and the related
[coverage rows 440](ZONE_COVERAGE_REGISTER.csv#L440) and
[473](ZONE_COVERAGE_REGISTER.csv#L473).

### Checkpoint 464: Circle Guildhall and Yggdrasil Outpost shared source identities

The 16-room unregistered `circlegh` WLD contains exact copies of `circle_op`
rooms #45410–#45412. Its four MOB prototypes #45410–#45413 and two OBJ
prototypes #45410–#45411 are also exact copies of the corresponding outpost
records. `circlegh` additionally has thirteen rooms outside that shared
three-room set, nine other mobile prototypes, two additional object
prototypes, and one SHP shop record. Their different titles, raw zone values,
file paths, reset sets, and nonshared content preserve separate identities;
the shared subset does not establish a replacement.

Followed WLD, MOB, OBJ, ZON, and SHP histories remain on the `circlegh` stem
from the 2009 area-tree move, with no tracked rename or explicit whole-map
replacement. The 2026 same-stem ANSI cleanup to `circlegh.obj` is a text-format
edit, not a lineage statement. Other Yggdrasil-named sources `ygg`
(#30400–#30412) and `rf_gh` (#28900–#28912) have different VNUM bands and do
not identify a canonical version. Preserve all source identities pending an
owner-provided map. See the [`circlegh` audit](CIRCLEGH_AUDIT_REVIEW.md),
[30 builder questions](CIRCLEGH_BUILDER_QUESTIONS.md),
[outpost audit](CIRCLE_OP_AUDIT_REVIEW.md),
[candidate row 27](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L27),
[coverage row 378](ZONE_COVERAGE_REGISTER.csv#L378), and related
[coverage rows 440](ZONE_COVERAGE_REGISTER.csv#L440) and
[473](ZONE_COVERAGE_REGISTER.csv#L473).

### Checkpoint 465: `cript` empty shop-path identity

The only current `cript` source is a zero-byte `areas/shp/cript.shp`; it
contains no SHP records or numeric identities to compare. Followed history
reaches the 2005 initial revision, moves through the Players tree and into
`areas/shp` in 2009, and shows same-stem maintenance in 2013. The current file
is empty, and no WLD/MOB/OBJ/ZON/QST or title data establishes an area
predecessor, alias, or replacement. The non-mini fallback’s wildcard path can
encounter the filename, but its empty contents do not establish a shop
identity. Retain the file until its source and editor owners explain the
placeholder or truncation. See the [`cript` audit](CRIPT_AUDIT_REVIEW.md),
[10 builder questions](CRIPT_BUILDER_QUESTIONS.md),
[candidate row 28](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L28), and
[coverage row 379](ZONE_COVERAGE_REGISTER.csv#L379).

### Checkpoint 466: Dalvik title, map identity, and history

After color-token and whitespace normalization, “Dalvik’s Dockyard” is unique
among 442 current ZON title sources. The `areas/AREA` annotation is blank.
Dalvik’s 476 rooms have 50 same-number VNUMs in the active WLD set, with no
whole room block identical to its active same-number record. Candidate mobiles
#1–#3 differ from active Limbo #1–#3; candidate objects #1–#9 differ from
active Limbo and #10–#63 differ from active Heavens. All 63 candidate objects
have distinct full blocks from their current active numeric owners. Dalvik
#53 is a dazzling sapphire bracelet, whereas Heavens #53 is a rope.

Thetis #38095 north → #104 is the one active WLD numeric inbound reference;
both inactive Dalvik and Mavulsk define #104 with different room identities.
The distinct source-only edge Dalvik #298 south → active Surface #635660 is
consistent with the separate Surface code entry mapping port #635260 “Dalvik”
to arrival room #635660. That association does not prove that the unregistered
map is the active port area or a replacement for any other source.

Followed histories for the five current Dalvik paths show only the visible
source addition (`3f835d646`) and line-ending cleanup (`428ebfddf`) in this
checkout. No older alternate-stem lineage, path rename, predecessor, or
explicit replacement appears in that bounded history. The name “Dalvik” also
appears in separate Surface ship code and a boatseller prototype; neither use
establishes map identity. See the [Dalvik audit](DALVIK_AUDIT_REVIEW.md),
[28 builder questions](DALVIK_BUILDER_QUESTIONS.md),
[Thetis review](THETIS_AUDIT_REVIEW.md#unresolved-exit-to-104),
[candidate row 29](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L29), and
[coverage row 380](ZONE_COVERAGE_REGISTER.csv#L380).

### Checkpoint 467: Dark map, shop identity, and history

Normalized title “Guildhall of the Dark” is unique among 442 current ZON title
sources. Unregistered `dark` reuses active Harrow’s ten room VNUMs #29400–#29409,
three mobile VNUMs #29400–#29402, and object VNUMs #29400–#29401; every compared
room, mobile, and object record differs from its Harrow counterpart. Dark’s
SHP record #29401 is also present as a separate record key in `infernal_gh.shp`.

The next current candidate `dark_gh`, titled “the Dark Guildhall,” uses the
separate #29200–#29242 VNUM band. Similar wording does not establish a common
map owner, alias, predecessor, or replacement. Followed Dark WLD/MOB/OBJ/ZON/SHP
histories reach the 2009 area-tree move; later changes are same-stem edits,
including the 2015 ZON item-flag pass. No alternate-stem move or explicit
replacement appears in that bounded history.

The #29400–#29409 room identities and matching exit targets belong to the
active Harrow source in the current loader. Harrow’s active Surface boundaries
and compiled object uses therefore remain Harrow references. Dark’s separate
#29400 up exit to inactive Duris3 #156726 remains an unresolved source-only
edge, not a Harrow or active Duris3 route. The conditional shop fallback can
include both unregistered #29401 SHP records while mapping Dark keeper/room
numbers to active Harrow prototypes. See the [Dark audit](DARK_AUDIT_REVIEW.md),
[30 builder questions](DARK_BUILDER_QUESTIONS.md),
[Harrow audit](HARROW_AUDIT_REVIEW.md),
[candidate row 30](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L30), and
[coverage row 381](ZONE_COVERAGE_REGISTER.csv#L381).

### Checkpoint 468: Dark Guildhall map identity and history

The unregistered `dark_gh` ZON title is “the Dark Guildhall,” unique after
normalization among 442 tracked titles. The similarly named `dark` component
uses “guildhall of the dark” and a different #29400–#29409 band. No tracked
title or file history establishes an alias, merge, predecessor, or replacement.

Dark Guildhall reuses active Newbie identities: 21 of 22 room VNUMs, three of
four mobile VNUMs, and seven of eight object VNUMs overlap same-number Newbie
records, and every shared complete block differs. `newbie.wld` has 50 direction
records targeting those shared room numbers. These are active Newbie routes to
Newbie-owned records, not proof that Dark Guildhall is active or a replacement
map. The three overlapping mobiles and seven overlapping objects likewise
retain their active Newbie owners; preserve the different unregistered Dark
blocks separately until their builders decide how to resolve each namespace.

Followed histories for `dark_gh.wld`, `.mob`, `.obj`, `.zon`, and `.shp` reach
the 2009 area-tree move. Later visible edits remain under the same stem,
including a 2015 ZON item-flag pass and September 2026 ZON/SHP updates. No
alternate-stem rename or explicit whole-zone replacement appears in those
bounded histories; pre-2009 upstream lineage remains unknown. The four
source-external edges point to Duris3, inactive `surf`/`surface2011`, or
Surface Keeps #123619, with no reverse in the tracked target rooms. Keep their
target and owner ambiguity separate from the Newbie room collisions. See the
[Dark Guildhall audit](DARK_GH_AUDIT_REVIEW.md),
[32 builder questions](DARK_GH_BUILDER_QUESTIONS.md),
[candidate row 31](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L31), and
[coverage row 382](ZONE_COVERAGE_REGISTER.csv#L382).

### Checkpoint 469: Dark Outpost map identity and history

The unregistered `darkop` title “dark outpost” is unique after normalization
among 442 tracked ZON titles. Nearby `dark` (“guildhall of the dark”) and
`dark_gh` (“the Dark Guildhall”) use different stems and ranges. Dark Outpost’s
“Temple of Eternal Pertinacity” and the `dark` temple’s similarly worded title,
plus `dark.zon` resets for portal VNUMs #29410 and #29412–#29415, are reasons to
ask about a planned relationship. The current active table maps those five
imports to Harrow objects; their matching Dark Outpost portal definitions are
unregistered. This leaves a source-to-source identity question, not proof of a
dependency or replacement.

All 19 Dark Outpost room, 18 mobile, and ten object VNUMs overlap active Harrow,
with different complete same-number blocks. Active Harrow’s 44 directions
target the reused room numbers as Harrow identities. The unregistered BSGHOP
component also reuses the #29440–#29443 horror-map text: most room prose/exits
match Dark Outpost, while its #29440 up exit differs; the two #29440 portal
objects share prose but have different numeric fields. These are tracked content
copies and VNUM conflicts, not a canonical-owner or replacement decision. See
the [BSGHOP audit](BSGHOP_AUDIT_REVIEW.md#conflicting-numeric-ownership-and-copied-room-text)
and the [Harrow audit](HARROW_AUDIT_REVIEW.md#unregistered-wld-target-matches).

Followed histories for the four `darkop` paths reach the 2009 area-tree move
and later same-stem edits through 2013. No alternate-stem rename, predecessor,
or explicit replacement appears in those bounded paths; earlier upstream
lineage remains unknown. The four one-way target numbers resolve to active
Surface Keeps once and inactive Duris3/Oldmap8 records otherwise, with no
reverse to the candidate source rooms. Keep these route/identity findings
separate from the copied BSGHOP content. See the [Dark Outpost audit](DARKOP_AUDIT_REVIEW.md),
[32 builder questions](DARKOP_BUILDER_QUESTIONS.md),
[candidate row 32](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L32), and
[coverage row 383](ZONE_COVERAGE_REGISTER.csv#L383).

### Checkpoint 470: `ddguild` source identity and bounded history

The SHP-only stem has no AREA annotation or ZON title. Its tracked path history
reaches the 2009 area-tree move and a 2013 same-stem commit with a zero-line
file diff; no alternate-stem predecessor or whole-area replacement is shown.
An unregistered `player_guilds.shp` record repeats label #35300 and the same
keeper #35300 / room #35303 pair, but lists four additional products. This is a
cross-source identity question, not evidence that either file supersedes the
other. Keep `ddguild` RETAIN IN PLACE until shop, map, loader, content, editor,
and persisted-identity owners resolve the shared fields. See the
[technical audit](DDGUILD_AUDIT_REVIEW.md), [builder questions](DDGUILD_BUILDER_QUESTIONS.md),
[candidate row 33](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L33), and
[coverage row 384](ZONE_COVERAGE_REGISTER.csv#L384).

### Checkpoint 471: `drag` empty-source identity and history

No area annotation or ZON title accompanies the two zero-byte files. Their
followed paths reach `Players/Declined/...` in the 2005 initial revision,
`Players/justice_list` in 2006, the `areas/qst` and `areas/shp` paths in 2009,
and a 2013 same-stem zero-line change. The files contain no identity or title
data, and their path history proves neither a map alias nor a replacement.
Keep both RETAIN IN PLACE pending owner and external-tool confirmation. See the
[technical audit](DRAG_AUDIT_REVIEW.md), [builder questions](DRAG_BUILDER_QUESTIONS.md),
[candidate row 34](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L34), and
[coverage row 385](ZONE_COVERAGE_REGISTER.csv#L385).

### Checkpoint 472: `dranquest` identity, story-lineage, and history

The ZON title is the generic `Quest`, with no AREA annotation. Followed WLD,
MOB, OBJ, ZON, and QST histories reach the 2009 area-tree move and later
same-stem changes through 2015, with no alternate-stem predecessor or explicit
whole-area replacement. Candidate QST text asks a Balor to end the Blood War
by bringing Joukarth Forgath's head; active Heavens QST instead has Rozkadrath
help Joukarth seek Blood War dominance. This is a story-owner continuity
question, not replacement evidence. Keep `dranquest` RETAIN IN PLACE pending
map, quest, canon, reset, code, editor, and persisted-data decisions. See the
[technical audit](DRANQUEST_AUDIT_REVIEW.md), [builder questions](DRANQUEST_BUILDER_QUESTIONS.md),
[candidate row 35](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L35), and
[coverage row 386](ZONE_COVERAGE_REGISTER.csv#L386).

### Checkpoint 473: Duris3 map identity, history, and replacement

Followed history shows the 2009 area-tree move, the 2011 addition of the
current 1.89-million-line `Duris3.wld`, and a 2026 line-ending normalization.
No alternate-stem predecessor or explicit whole-map replacement was found.
The current WLD is absent from both full area manifests and has no companion
ZON title or story sidecar.

The 22,064 Duris3 room VNUMs shared with active WLDs are distinct source
identities: none of the compared pairs has an equal normalized title or an
identical complete room block. Keep the 90,000-room map separate from the
active same-number rooms, including the previously reviewed zone-specific
Duris3 route candidates. Source history and numeric overlap do not establish
supersession. Retain Duris3 in place pending map, editor, code, persisted
identity, and owner confirmation. See the [technical audit](DURIS3_AUDIT_REVIEW.md),
[builder questions](DURIS3_BUILDER_QUESTIONS.md), [candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36),
and [coverage row 386](ZONE_COVERAGE_REGISTER.csv#L387).

### Checkpoint 474: EOO Valhalla map identity and history

The only exact ZON title match for “Valhalla” is eoo.zon; the AREA annotation
is blank. Followed WLD, MOB, OBJ, ZON, and SHP histories reach the 2009
area-tree move and same-stem updates in 2012–2013; no alternate-stem
predecessor or explicit whole-area replacement was found.

All ten EOO room VNUMs are also defined by dark_gh and genocide_gh; nine also
have active Newbie room records. None of the 29 complete same-number room
comparisons is identical. EOO mobiles #29200–#29204 have ten same-number
comparisons with dark_gh, genocide_gh, or Newbie, and EOO objects #29200–#29201
have three same-number comparisons with dark_gh or Newbie; no complete
prototype record matches. Keep Valhalla, the two separate guildhall maps, and
active Newbie under their own identities. Jotun’s Valhalla reference is
thematic lore, not replacement proof. Retain EOO in place pending map, story,
shop, tooling, and owner decisions. See the [technical audit](EOO_AUDIT_REVIEW.md),
[builder questions](EOO_BUILDER_QUESTIONS.md), [candidate row 37](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L37),
and [coverage row 387](ZONE_COVERAGE_REGISTER.csv#L387).

### Checkpoint 475: eoo_op Muspelheim map identity and history

The ZON title “Eye of Odin Outpost” has one exact match among tracked ZON
headers; the raw AREA annotation is blank. Followed WLD/ZON paths reach the
2009 area-tree move and a 2013 same-stem commit with zero-line diffs in those
files; MOB/OBJ paths reach the 2009 move. No alternate-stem predecessor or
explicit whole-component replacement appears in these histories.

All three eoo_op room VNUMs also occur in active Newbie and inactive dark_gh;
none of the six complete room comparisons is identical. All four MOB and all
four OBJ VNUMs have different active Newbie prototypes. The portal target
numbers #29206, #29222, #29212, and #29225 also have active Newbie owners,
while EOO and eoo_op2 provide additional inactive room identities at #29206,
#29222, and #29225. These are map and item-owner conflicts, not proof of
replacement. Keep each source owner separate and retain eoo_op in place pending
map, story, portal, editor, and persisted-VNUM decisions. See the
[technical audit](EOO_OP_AUDIT_REVIEW.md), [24 builder questions](EOO_OP_BUILDER_QUESTIONS.md),
[candidate row 38](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L38), and
[coverage row 388](ZONE_COVERAGE_REGISTER.csv#L389).

### Checkpoint 475: eoo_op Muspelheim map identity and history

The ZON title “Eye of Odin Outpost” has one exact match among tracked ZON
headers; the raw AREA annotation is blank. Followed WLD/ZON paths reach the
2009 area-tree move and a 2013 same-stem commit with zero-line diffs in those
files; MOB/OBJ paths reach the 2009 move. No alternate-stem predecessor or
explicit whole-component replacement appears in these histories.

All three eoo_op room VNUMs also occur in active Newbie and inactive dark_gh;
none of the six complete room comparisons is identical. All four MOB and all
four OBJ VNUMs have different active Newbie prototypes. The portal target
numbers #29206, #29222, #29212, and #29225 also have active Newbie owners,
while EOO and eoo_op2 provide additional inactive room identities at #29206,
#29222, and #29225. These are map and item-owner conflicts, not proof of
replacement. Keep each source owner separate and retain eoo_op in place pending
map, story, portal, editor, and persisted-VNUM decisions. See the
[technical audit](EOO_OP_AUDIT_REVIEW.md), [24 builder questions](EOO_OP_BUILDER_QUESTIONS.md),
[candidate row 38](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L38), and
[coverage row 388](ZONE_COVERAGE_REGISTER.csv#L389).

### Checkpoint 476: eoo_op2 map identity and history

The raw AREA annotation is blank. “The Horrible Tracts of the Unnamed” occurs
as the exact ZON title for eoo_op2, astral, mtncastl, theurgist, and ud-inside.
The eoo_op2 stem itself is absent from full areas/AREA and areas/qst/AREA.
Followed WLD/MOB/OBJ histories reach the 2009 area-tree move; the ZON history
also includes a 2025 UTF-8 conversion. No alternate-stem predecessor or
explicit whole-component replacement appears in those paths.

All six eoo_op2 room numbers occur in active Newbie with different complete
blocks; #29220–#29222 also occur in dark_gh with different blocks. The
#29220/#29223 Niflheim pair, #29221/#29224 gate pair, and #29222/#29225
Inner Sanctum pair have matching title and prose but different VNUM/exit
records. Preserve both passages pending the owner’s explanation. Four MOB and
two OBJ VNUMs also overlap active Newbie with different prototypes; Newbie’s
QST, story sidecar, and Ailvio code use the same-number item identities for
its own content.

The EOO #29211/#29213 source portals and eoo_op2 #29220/#29221 pools form two
reciprocal-looking numeric pairs, while active Newbie owns all four endpoint
VNUMs. Inactive dark_gh portals also target #29221/#29222 for its own map.
These conflicts and the repeated generic ZON title do not establish alias or
replacement. Retain eoo_op2 in place pending map, portal, story, code, tooling,
and persisted-identity decisions. See the [technical audit](EOO_OP2_AUDIT_REVIEW.md),
[24 builder questions](EOO_OP2_BUILDER_QUESTIONS.md), [candidate row 39](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L39),
and [coverage row 389](ZONE_COVERAGE_REGISTER.csv#L390).

### Checkpoint 477: eskull_hall identity and bounded history

The raw AREA annotation is blank. The ZON title Ebon Skull Guildhall is unique
among tracked ZON titles. Followed WLD/ZON/SHP histories include a 2009
area-tree move and a 2013 zero-line same-stem commit; MOB reaches the 2009
move; OBJ also has a 2016 capitalization/punctuation edit. ZON and SHP were
edited again in 2026 to add gathering bags #491–#494 to the merchant resets
and product list. No alternate-stem predecessor or explicit whole-component
replacement appears in these paths.

All thirteen candidate room numbers have different Newbie room records.
Numbers #29300–#29309 also occur in inactive cbr_hall, loc_gh, and ravenmis;
#29310–#29312 occur in inactive ethdawn_op and loc_gh. None of the 49
complete same-number room comparisons is identical. Candidate mobiles
#29300–#29303 differ from the active Newbie prototypes and from the overlapping
inactive guild-hall records. Candidate portals #29300/#29301 differ from
Newbie’s Pain dagger/sign; Newbie QST and story item fields use Newbie’s active object
identities across the same #29300–#29312 numeric range.

Typed code consumers name Newbie’s active VNUM identities: the inn callback
uses room #29305, and Ailvio reserves mobile #29303/#29304. Candidate
#29305 is the Falling Star Inn, while active Newbie #29305 is A Secret Magic
Shop; candidate Khayne #29303 differs from Newbie’s bleeding man. These
collisions and later stock edits establish neither a shared owner nor a
replacement. Keep each source identity separate and retain eskull_hall in
place pending owner, code, shop, tool, and persisted-data decisions. See the
[technical audit](ESKULL_HALL_AUDIT_REVIEW.md),
[24 builder questions](ESKULL_HALL_BUILDER_QUESTIONS.md),
[candidate dependency row 40](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L40),
and [coverage row 390](ZONE_COVERAGE_REGISTER.csv#L391).

### Checkpoint 478: ethdawn_op identity and bounded history

The raw AREA annotation is blank; the ZON title Etherial Dawn Outpost is unique
among tracked titles. WLD history reaches the 2009 area-tree move and a 2011
one-word spelling correction; MOB, OBJ, and ZON histories reach the 2009 move.
No alternate-stem predecessor or explicit whole-component replacement
appears in these paths. The header #1 / room #29310–#29312 numeric-range
mismatch (all four exits originate in those out-of-range rooms) is a source
consistency question, not replacement evidence.

All three room VNUMs differ in complete records from active Newbie and
inactive loc_gh and eskull_hall: the three maps describe separate Mistywood,
Arvandor/Last Hope, and guildhall spaces at the same numbers. Candidate mobile
VNUMs #29310–#29312 have no other current same-type mobile definitions.
Candidate portals #29310/#29311 differ from active Newbie’s hefty bag/arrows;
Newbie also owns quiver #29312 and its QST/story items reference #29310–#29312.
The unregistered Ravenmis ZON imports object #29311 into #29307, while the
candidate #29310 portal targets #29307 and the Goodie-labeled #29311 portal
targets #29312. These are source-level identity questions, not proof of map
lineage or replacement. Retain the map and objects separately pending portal,
story, map, and persisted-data owner decisions. See the
[technical audit](ETHDAWN_OP_AUDIT_REVIEW.md),
[24 builder questions](ETHDAWN_OP_BUILDER_QUESTIONS.md),
[candidate dependency row 41](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L41),
and [coverage row 391](ZONE_COVERAGE_REGISTER.csv#L392).

### Checkpoint 479: fists_gh title, history, and source identity

The raw AREA annotation is blank. The ZON title is Fists of Fury Guildhall.
WLD/ZON/SHP histories reach the 2009 area-tree move and a 2013 mode-only
commit; MOB reaches the 2009 move. The 2016 OBJ cleanup removed the former
unnamed object #30500, leaving the current same-stem OBJ file empty. A 2026
commit added gathering bags #491-#494 to ZON and SHP. No alternate-stem
predecessor or explicit whole-component replacement appears in these paths.

All seven Fists room VNUMs #30500-#30506 differ from active Patrols room
records. The four candidate mobiles at #30500-#30503 also differ from
Patrols mobiles at those numbers. Current source text and later stock changes
do not establish a shared owner or a replacement relationship. The compiled
inn callback #30504 and conditional SHP fallback keeper/room fields map to
active Patrols IDs in the current tables; preserve them as typed ownership
questions. Keep fists_gh RETAIN IN PLACE pending map, code, shop, tooling,
and persisted-ID decisions. See the [technical audit](FISTS_GH_AUDIT_REVIEW.md),
[24 builder questions](FISTS_GH_BUILDER_QUESTIONS.md),
[candidate row 42](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L42), and
[coverage row 392](ZONE_COVERAGE_REGISTER.csv#L393).

### Checkpoint 480: fooquest title, history, and prototype sets

The raw AREA annotation is blank and the exact ZON title FooQuest occurs once
among tracked titles. WLD, the extra WLD-directory mobile file, ZON, and QST
histories reach the 2009 area-tree move and a 2013 mode-only commit. The
conventional MOB reaches the 2009 move. OBJ history includes the 2016 removal
of unnamed #65004 and edits to two descriptions. No alternate-stem
predecessor or explicit whole-component replacement appears in these paths.

The extra areas/wld/fooquest.mob has twelve pirate records #63600-#63611.
After substituting a +1400 VNUM offset, each record is textually identical
to the corresponding first twelve conventional MOB records #65000-#65011.
The ZON uses the latter IDs. This duplicate data and the #636 header / #65000
room-and-mobile VNUM mismatch warrant owner review, but do not establish
which file is authoritative or prove replacement. The QST's specific story,
its external Moonshae key dependency, and the compiled special bindings are
additional source relationships, not lineage evidence. Keep fooquest RETAIN
IN PLACE pending quest, map, and code-owner decisions. See the
[technical audit](FOOQUEST_AUDIT_REVIEW.md),
[24 builder questions](FOOQUEST_BUILDER_QUESTIONS.md),
[candidate row 43](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L43), and
[coverage row 393](ZONE_COVERAGE_REGISTER.csv#L394).

### Checkpoint 481: forestb title, history, and typed identities

The raw AREA annotation is blank, forestb is absent from both full areas/AREA
and areas/qst/AREA, and no same-stem ZON title exists. MOB history reaches the
2009 area-tree move and a 2010 typo-only commit; OBJ history reaches the 2009
move; QST and SHP histories reach the 2009 move and a 2013 commit titled
`stuff`. No alternate-stem predecessor or explicit whole-component
replacement evidence appears in these bounded paths.

Preserve the distinct types that reuse numbers: MOB and OBJ #38200, the SHP
keeper mobile #38235, and SHP room #38259 versus the same-number crude-comb
OBJ. The #38235 mobile, four stocked product prototypes, and #38259 room have
no other current same-type owner in the inspected MOB/OBJ/WLD files. File-list
exclusion and these numeric/type relationships do not establish replacement
or runtime ownership. Keep forestb RETAIN IN PLACE pending manifest, quest,
shop, editor, and persisted-ID decisions. See the
[technical audit](FORESTB_AUDIT_REVIEW.md),
[24 builder questions](FORESTB_BUILDER_QUESTIONS.md),
[candidate row 44](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L44), and
[coverage row 394](ZONE_COVERAGE_REGISTER.csv#L395).

### Checkpoint 482: forofcon map identity and history

The raw AREA annotation is blank; the exact ZON title “Skull City and the
Fortress of Conclusion” occurs once among current ZON files. forofcon is
absent from full areas/AREA and areas/qst/AREA. Every one of its 403 WLD room
VNUMs also occurs in inactive Duris3, with a different complete room block
and title. The first 53 additionally occur in active Smoke with different
room blocks and titles; same-number identities resolve to Smoke in current
active tables. Same-type scans found no other current MOB/OBJ owner for the
152 mobile or 165 object VNUMs.

Tracked `.capmob`, `.capobj`, and `.capqst` files are byte-identical to their
canonical files. `.capzon` differs only in the durisEdit version comment.
Followed WLD/MOB/OBJ histories show 2026 line-ending normalization; ZON also
has a 2026 ANSI cleanup; QST reaches a 2015 quest-item-flag overhaul. These
artifacts and same-number WLD sets do not establish a predecessor or
whole-component replacement. Keep forofcon RETAIN IN PLACE pending map,
quest, reset, editor, and persisted-ID decisions. See the
[technical audit](FOROFCON_AUDIT_REVIEW.md),
[24 builder questions](FOROFCON_BUILDER_QUESTIONS.md),
[candidate row 45](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L45), and
[coverage row 395](ZONE_COVERAGE_REGISTER.csv#L396).

### Checkpoint 483: genesis source identity and shop dependencies

Genesis and aeon_gh each define rooms #29900–#29909 and mobiles #29900/#29901,
but the records and identities differ. Genesis has a gargoyle at each shared
mobile number; Aeon has a massive mist golem and a merchant. Genesis also
defines keeper #29902 and gargoyle #29903. Both components are absent from
the full area list; the only matching annotation is *OPEN 299. Their unique
ZON titles do not prove that either source replaces the other.

Genesis's followed WLD/MOB/OBJ/ZON/SHP histories remain on that stem through
the 2009 tree move, 2012/2013 same-stem maintenance, and 2016 unnamed-object
cleanup on MOB/OBJ/ZON. No alternate-stem predecessor or whole-zone
replacement appears in these paths. Its empty OBJ file and one SHP record
leave nine imported stock IDs, including multiply defined #53/#108/#109 and
undeclared #6089. Three additional ZON imports belong to Player Castles.
Compiled code assigns an inn handler to shared room #29903, whose Genesis
sanctuary and Aeon niche are different rooms. Keep the two candidate sources
separate pending range, handler, shop, and persisted-ID ownership decisions.
See the [Genesis audit](GENESIS_AUDIT_REVIEW.md#shared-vnums-history-and-owner-decision),
[24 builder questions](GENESIS_BUILDER_QUESTIONS.md),
[Aeon audit](AEON_GH_AUDIT_REVIEW.md#vnum-collision-title-and-history),
[candidate row 46](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L46), and
[coverage row 396](ZONE_COVERAGE_REGISTER.csv#L397).

### Checkpoint 484: Genocide Guildhall identities and history

The unregistered genocide_gh map defines #29200–#29209, the same ten room
numbers found in unregistered dark_gh and eoo. Active Newbie defines
#29201–#29209; none of these same-number room blocks is identical. The
genocide_gh mobile VNUMs #29201–#29203 also collide with different active Newbie
prototypes; #29200–#29203 have other unregistered eoo identities, and
#29200/#29201 also occur in dark_gh. Keep all room and mobile records typed
by source. The #29200 D0 approach to Newbie #29201 is already counted in
Checkpoint 304 and does not add a new total.

The exact title “The Genocide Guildhall” occurs once among 442 tracked ZON
titles; the raw AREA annotation is blank and neither genocide_gh nor its
similarly named source genocide_hall is in the full area list. genocide_hall
uses zone #295 and rooms #29500–#29511, not the #2920/#29200–#29209 genocide_gh
range. Their ZON/SHP files share a 2026 gathering-bag stock update, but the
followed histories remain on their own stems and establish no alternate-stem
rename or explicit whole-zone replacement. A compiled inn callback is
assigned to shared room #29202, which current Newbie owns under a different
room title from the genocide_gh executioners’ resting place. Keep each source
separate until map, code, shop, and persisted-ID owners resolve the overlaps.
See the [genocide_gh audit](GENOCIDE_GH_AUDIT_REVIEW.md#same-number-identities-history-and-disposition),
[24 builder questions](GENOCIDE_GH_BUILDER_QUESTIONS.md),
[Newbie review](NEWBIE_AUDIT_REVIEW.md#active-graph-and-inactive-source-only-approaches),
[EOO review](EOO_AUDIT_REVIEW.md#reused-room-mobile-and-object-identities),
[candidate row 47](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L47), and
[coverage row 397](ZONE_COVERAGE_REGISTER.csv#L398).

### Checkpoint 485: Genocide Guild Hall identities and history

The unregistered genocide_hall WLD defines #29500–#29504 and #29510–#29511,
the same room numbers found in unregistered aod_gh and legion maps. rf_op
also defines #29510 and #29511. These source records have different room
titles and blocks. MOB identities overlap too: aod_gh and legion define
#29500–#29502, legion also defines #29503–#29504, and rf_op defines #29510
and #29511. genocide_hall’s OBJ file is empty, while aod_gh and legion have
different object definitions #29500–#29504 and rf_op has #29510–#29511.
Keep each typed record under its own source stem.

The active areas/AREA and qst/AREA entries assign the #294–#296 range to
Harrow, but neither lists genocide_hall and Harrow WLD has no #29500–#29511
room records. A compiled inn callback is assigned to `real_room0(29502)`;
that number is called “The Dark Skull Inn” in genocide_hall and has different
unregistered aod_gh/legion room records. The area-range and handler questions
need explicit owners.

Followed genocide_hall WLD/MOB/OBJ/ZON/SHP histories remain on the same stem
through the 2009 tree move and later edits. The 2026 kingdom commit added
gathering bags #491–#494 to both genocide_hall and genocide_gh shop/reset
lists. Their room ranges and histories remain separate, with no alternate-
stem rename or explicit whole-zone replacement. Keep the separate
genocide_gh map at #29200–#29209 and do not treat the common bag update as
replacement evidence. See the
[Genocide Guild Hall audit](GENOCIDE_HALL_AUDIT_REVIEW.md#same-number-identities-history-and-disposition),
[24 builder questions](GENOCIDE_HALL_BUILDER_QUESTIONS.md),
[genocide_gh review](GENOCIDE_GH_AUDIT_REVIEW.md),
[Harrow area list](../../../areas/AREA#L142),
[candidate row 48](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L48), and
[coverage row 398](ZONE_COVERAGE_REGISTER.csv#L399).

### Checkpoint 486: Gimluk’s Cottage identities and history

The current source scan found no competing WLD, MOB, OBJ, or QST declarations
for gianthou’s exact numeric identities. Its ZON title, “Gimluk’s Cottage,”
is uniquely found among current ZON titles, and its followed file histories
remain on the gianthou stem through the 2009 area-tree move and later
same-stem edits. No alternate-stem rename or explicit whole-zone replacement
was found. That history does not establish active ownership.

The candidate’s zone header #426 overlaps the active Pworm manifest band
#425–#426, and its rooms through #42870 extend into #427–#428 bands marked
open. This is a numeric manifest ownership conflict, not evidence of exact
same-number WLD records in Pworm. The external exit to #274141 has different
records in unlisted surf and surface2011 sources. Keep those target identities
distinct until their owners and the area-list owner resolve them. Preserve
gianthou under its existing stem and retain it in place. See the
[Gimluk’s Cottage audit](GENIANTHOU_AUDIT_REVIEW.md#identity-history-and-disposition),
[24 builder questions](GENIANTHOU_BUILDER_QUESTIONS.md),
[Pworm/open manifest range](../../../areas/AREA#L201),
[candidate row 49](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L49), and
[coverage row 399](ZONE_COVERAGE_REGISTER.csv#L400).

### Checkpoint 487: Guild Rooms identities and history

guild_rooms has one SHP file and no same-stem WLD, MOB, OBJ, ZON, or QST
source. Its record identifies #8552 as a keeper, #8555–#8559 as products,
and #8560 as the shop room. No current MOB prototype defines #8552 and no
current OBJ prototype defines those five product IDs. Active grave.wld
defines all seven referenced numbers as rooms; #8560 is “A Simple Stone Tomb.” This is a
cross-type numeric collision, not a matching shop keeper or product identity.
A separate compiled guild-transfer call to real_room(8556) points into that
active Grave map.

Followed SHP history remains on the guild_rooms stem through the 2009
area-tree move and later 2013 edit. No alternate-stem rename or explicit
whole-zone replacement was found. The active guildhalls #480–#489 entry
does not establish that this #8850-headed SHP record belongs to it. Keep the
record RETAIN IN PLACE until the shop, Grave, guildhall, and loader owners
resolve its identities and intended location. See the
[Guild Rooms audit](GUILD_ROOMS_AUDIT_REVIEW.md#identity-history-and-disposition),
[24 builder questions](GUILD_ROOMS_BUILDER_QUESTIONS.md),
[Grave manifest](../../../areas/AREA#L48),
[candidate row 50](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L50), and
[coverage row 400](ZONE_COVERAGE_REGISTER.csv#L401).

### Checkpoint 488: Halfcut Hills candidate and active source identities

halfcut_hills is unlisted while active areas/AREA and qst/AREA list halfcut
for #270–#279. Every candidate WLD room #27001–#27235 also exists in active
halfcut. After normalizing only each WLD record’s zone-number field, 194 of
235 room blocks match and 41 differ. All 45 candidate MOB vnums overlap
active halfcut, and all 45 full records differ. All 22 candidate OBJ vnums
overlap; 14 records match and eight differ. Candidate QST #27002 overlaps
but differs from active QST #27002; candidate #27001 has no active QST
counterpart. The active zone also has a longer 470-room map, 413 ZON
commands, a shop, a story sidecar, and its own quest/journal dossier.

Followed histories remain on their separate stems; no halfcut_hills-to-
halfcut rename or explicit whole-zone replacement was found. The current
[active source dossier](../zone-stories/THE_HALFCUT_HILLS.md#evidence-and-review-boundary)
explicitly identifies halfcut_hills as inactive and says it does not define
the active journal’s current behavior. The candidate’s old #27001 edge to
#256031 also differs from active halfcut’s reciprocal #629274 Surface
boundary. Treat the exact vnum overlap as conflicting parallel identities,
not as proof that the candidate can be deleted or merged. See the
[candidate audit](HALFCUT_HILLS_AUDIT_REVIEW.md#source-inventory-and-numeric-ownership),
[24 builder questions](HALFCUT_HILLS_BUILDER_QUESTIONS.md),
[active halfcut entry](../../../areas/AREA#L135),
[candidate row 51](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L51), and
[coverage row 401](ZONE_COVERAGE_REGISTER.csv#L402).

### Checkpoint 489: Hostel source identities and history

hostel is absent from both active area manifests. Its 102 room IDs include 45
current same-number records in Limbo and Heavens, and all 45 room blocks
differ. Sixty-five of its 66 mobile IDs and all 76 object IDs also overlap
active Limbo/Heavens prototypes; every compared block differs. Candidate
mobile #18 has no current active same-number prototype. Candidate QST #46
differs from the empty active Heavens QST #46 record; its other three giver
keys do not overlap active QST records.

Some current active content explicitly distinguishes the Hostel objects from
active Heavens identities. Candidate Hostel #41 is a twisted metal key while
Heavens #41 is a guild badge. Candidate Hostel #72 is dragon-scale leg armor
while Heavens #72 is a magical fountain. Existing Torrhan and Hall of
Knighthood reviews preserve those active Heavens objects for their resets;
do not retarget those dependencies based on an unlisted same-number record.

Followed Hostel file history begins with the 2012 import and includes separate
2015 ZON, 2016 OBJ text, and 2026 OBJ stat changes. No followed rename or
explicit whole-area replacement was found. Active Savannah still names a
sister in the Hostel and requests all six epic instruments, but the reviewed
Hostel QST records cover a demon blade, a missing councilor, and an Athos
weapon thread; the current Savannah review found no matching Hostel consumer.
Treat this as an unresolved story lead and a parallel source identity, not
proof that Hostel is active or safe to remove. See the
[Hostel audit](HOSTEL_AUDIT_REVIEW.md#source-inventory-and-current-identity),
[Savannah story review](../zone-stories/SAVANNAH_OF_BROKEN_TRUSTS.md#L197),
[24 builder questions](HOSTEL_BUILDER_QUESTIONS.md),
[candidate row 52](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L52), and
[coverage row 402](ZONE_COVERAGE_REGISTER.csv#L403).

### Checkpoint 490: IC3 source identity and history

The uppercase `IC3.wld`, `IC3.mob`, `IC3.obj`, and `IC3.zon` files remain in
the source tree, but no current `areas/AREA` or `areas/qst/AREA` entry lists
them. Their #420000–#420053 room, #420000–#420020 mobile, and #420000–#420049
object ranges have no same-type VNUM overlap with current active AREA records.
Do not infer that the open numeric range is free of persisted, editor, or
deployment ownership.

Commit `ef7e3f9b17f492a946425657f1bce8e91e241506` (April 6, 2026) removed
IC3's `*4200-4300` entry from `areas/AREA`, leaving that range `*OPEN`; it
retained all four source files. Commit `180978e3a5662eb113f6ece64c62aba766206ee0`
(April 7) removed the active Rice Fields south entrance from #77262 to IC3
#420000. The present destination is `-1`, so there is no active inbound edge.
This is explicit de-listing and route removal, not a whole-area replacement
or archive instruction.

IC3 still has one inactive-source north exit from #420000 to active Rice Fields
#77262, and its ZON imports active Heavens object #360. The four source files
were edited through January 2026; `IC3.obj` was also edited September 14, 2026,
after de-listing. Preserve those distinct route, typed-object, and editing
facts while identifying their current owners. The bounded history scan found
no whole-area replacement that supersedes IC3. See the
[IC3 audit](IC3_AUDIT_REVIEW.md#source-inventory-and-current-identity),
[24 builder questions](IC3_BUILDER_QUESTIONS.md),
[candidate row 53](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L53), and
[coverage row 403](ZONE_COVERAGE_REGISTER.csv#L404).

### Checkpoint 491: Infernal Guildhall source identities

`infernal_gh` is not listed in either active area manifest. All 16 of its room
VNUMs, three mobile VNUMs, and four object VNUMs have different same-number
records in active Harrow. The active Harrow range *294–296 is the current
numeric owner; its source already has documented Surface boundaries and code
references at #29400, #29401, and #29403. Preserve Harrow's existing identity
and consumers while the Infernal source remains unlisted.

Other unregistered files reuse portions of this band: `dark.wld` uses
#29400–#29409, `darkop.wld` uses #29410–#29412 and #29420–#29422, and
`sky_gh.wld` uses #29400–#29404. Candidate mobiles #29400–#29402 also occur
in `dark.mob` and `sky_gh.mob`; candidate objects #29400–#29401 occur in
`dark.obj`. Their content differs. The bounded history review found no
whole-area replacement or literal `infernal_gh` entry in tracked
`areas/AREA` history.

`infernal_gh.shp` and `dark.shp` both contain a separate shop record headed
#29401. Because this checkout has no `areas/world.shp`, the non-mini shop code
has a conditional fallback that globs `areas/shp/*.shp`, regardless of full
AREA membership. Review the source-file duplicate together with the current
Harrow keeper/room resolution, while keeping actual boot behavior unqualified.
The Infernal ZON was edited on September 6, 2026 to add Heavens products
#491–#494. That recent edit does not establish activation, but it makes current
editor/export ownership material. See the [Infernal audit](INFERNAL_GH_AUDIT_REVIEW.md#source-inventory-naming-and-competing-identities),
[30 builder questions](INFERNAL_GH_BUILDER_QUESTIONS.md),
[Harrow audit](HARROW_AUDIT_REVIEW.md#unregistered-wld-target-matches),
[candidate row 54](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L54), and
[coverage row 404](ZONE_COVERAGE_REGISTER.csv#L405).

### Checkpoint 492: Jungle source identities and manifest history

The current ZON title “Jungle of Khazan” is unique after color-code
normalization. No other tracked WLD, MOB, or OBJ source defines a same-type
VNUM in Jungle’s #67000–#67092 band. Followed WLD/MOB/OBJ/ZON histories stay
on the Jungle stem to the 2009 areas-tree move; later edits are recorded in
the [audit review](JUNGLE_AUDIT_REVIEW.md#identity-history-and-disposition).
No tracked alternate-stem predecessor or whole-zone replacement was found.

The two manifests now disagree: the full areas/AREA entry was commented on
April 16, 2026, but areas/qst/AREA still lists Jungle from the 2012 update.
No same-stem qst/jungle.qst is present. The production quest compiler reads
the full AREA list, not qst/AREA, so that historical row alone does not make
a Jungle quest part of the standard aggregate. Keep the list discrepancy
unresolved until its owner confirms intent; neither unique VNUMs nor list
exclusion clears a migration or activation decision. See the
[quest-input evidence](JUNGLE_AUDIT_REVIEW.md#source-inventory-and-manifest-state),
[27 builder questions](JUNGLE_BUILDER_QUESTIONS.md),
[candidate row 55](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L55), and
[coverage row 405](ZONE_COVERAGE_REGISTER.csv#L406).

### Checkpoint 904: Jungle retains its unlisted identity

The Checkpoint 904 recheck found no external same-type WLD/MOB/OBJ definition,
typed reset, QST item/giver, story-sidecar role, shop role, portal/switch
target, or exact direct code lookup for Jungle's #67000–#67092 numbers. This
does not change the CP492 history: the full-area list comment is an explicit
manifest change with sources preserved, and the old QST/AREA row is separate
historical metadata with no matching Jungle QST file. No alternate-stem
predecessor or whole-area replacement was found. Keep the four current source
identities **RETAIN IN PLACE** and leave editor/deployment copies and
persisted state for owner review. See the
[Jungle typed audit](JUNGLE_AUDIT_REVIEW.md#checkpoint-904-external-typed-role-refresh),
[27 builder questions](JUNGLE_BUILDER_QUESTIONS.md), and
[coverage row 405](ZONE_COVERAGE_REGISTER.csv#L406).

### Checkpoint 493: Kilfray QST lineage and giver ownership

The source-only kilfray.qst defines all five giver VNUMs #95019–#95023 that
active Mistywood defines in mistywood.mob. Its three Q contracts use active
Mistywood objects #95000–#95004 and parallel three contracts in
mistywood.qst. Kilfray omits the 25,000 experience reward for Riliatar and
10,000 experience reward for Jarnes; its Ascuren contract has the same
offer/reward fields. Daumis and Korred have dialogue but no Q contract.

Kilfray is absent from both areas/AREA and areas/qst/AREA. Followed QST
histories show the shared 2009 area-tree move and 2013 “stuff” edit, not a
direct rename or replacement decision. Keep the parallel source distinct
until its author and quest owner decide whether it is a draft, sibling, or
legacy copy. See the [Kilfray audit](KILFRAY_AUDIT_REVIEW.md#giver-and-contract-crosswalk),
[Mistywood audit](MISTYWOOD_AUDIT_REVIEW.md#typed-dependency-and-qst-lineage),
[20 builder questions](KILFRAY_BUILDER_QUESTIONS.md),
[candidate row 56](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L56), and
[coverage row 406](ZONE_COVERAGE_REGISTER.csv#L407).

### Checkpoint 905: Kilfray map names do not establish QST replacement

Active Mistywood rooms #95000–#95001 use the Kilfray River name, and three
already-counted source-only Surf approaches come from Lake Kilfray / The
Kilfray River rooms to those active destinations. This gives the parallel
Kilfray QST a plausible environmental relationship to Mistywood, but it does
not show that either QST was moved, renamed, or replaced. The five active
Mistywood giver and item prototypes remain the current identities; Kilfray's
parallel contracts and reward differences remain separate authorial evidence.
The tracked QST history still shows no direct rename or replacement decision.
Keep the QST **RETAIN IN PLACE** outside both area lists until source and quest
owners resolve its provenance, map setting, and persisted identity. See the
[Kilfray role audit](KILFRAY_AUDIT_REVIEW.md#checkpoint-905-cross-source-identity-environment-and-typed-role-refresh),
[Mistywood identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-284-mistywood-title-and-bounded-history),
[20 builder questions](KILFRAY_BUILDER_QUESTIONS.md), and [candidate row 56](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L56).

### Checkpoint 494: Kzkrkeep source identity and Worms typed overlap

The unlisted `kzkrkeep.qst` and `kzkrkeep.shp` paths remain under the same
stem through the 2009 area-tree move and 2013 same-path edits. No direct rename,
retirement, or whole-zone replacement evidence appears in those followed
histories. The QST name “the keep” has no matching map, ZON title, or AREA
annotation in this checkout.

The shared VNUMs have specific typed meanings in active Worms: QST items
#6900/#6912 are Worms wormskin strips, the two SHP locations are Worms rooms
#6915/#6919, and nine shop products are Worms objects. This overlap does not
establish that Kzkrkeep is a Worms predecessor or that the conflicting amulet
story is intentional. Both area lists omit the source. Keep it
**RETAIN IN PLACE** until its owner, keeper mobiles, stock, item identities,
and shop fallback behavior are resolved. See the
[Kzkrkeep audit](KZKRKEEP_AUDIT_REVIEW.md),
[20 builder questions](KZKRKEEP_BUILDER_QUESTIONS.md),
[Worms source findings](WORMS_AUDIT_REVIEW.md#external-typed-and-code-references),
[candidate row 57](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L57), and
[coverage row 407](ZONE_COVERAGE_REGISTER.csv#L408).

### Checkpoint 906: Kzkrkeep numeric overlaps remain Worms identities

The Kzkrkeep source-only QST/SHP continues to reuse active Worms identities
without evidence of a direct source move or replacement. Active Worms room
#6900 is the `real_room()` destination in Ixarkon's teleport table, while
Kzkrkeep QST treats the same VNUM as an item and Worms OBJ #6900 is grey
wormskin. That typed collision does not connect the quest to Ixarkon's room.
The #6917/#6924 keepers still have no active MOB prototypes, even though
same-number Worms room/object records exist. Heavens shop-stock overlaps and
the three room-only Worms products also remain separate typed facts. Followed
QST/SHP history retains the CP494 conclusion: same-path edits only, with no
direct rename, retirement, or whole-zone replacement evidence. Keep both files
**RETAIN IN PLACE** pending owner and deployment review. See the
[Kzkrkeep role audit](KZKRKEEP_AUDIT_REVIEW.md#checkpoint-906-cross-source-typed-role-refresh),
[Worms typed references](WORMS_AUDIT_REVIEW.md#external-typed-and-code-references),
[20 builder questions](KZKRKEEP_BUILDER_QUESTIONS.md), and [candidate row 57](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L57).

### Checkpoint 495: Labyrint spelling and active Labyrinth QST branch

The active `labyrinth` area is listed in both area lists; the one-file
`labyrint.qst` spelling is not. Both QSTs use Sir Uthor giver #5028 and the
same ten item-command IDs plus the same 50,000-experience command. The active
branch adds a proof-list response and fuller completion dialogue; its contract
action IDs are unchanged. This is a parallel source copy with a close stem,
not a second active consumer or an independently mapped zone.

Followed history for `labyrint.qst` shows the 2009 area-tree move and a 2013
same-path edit, but no direct rename or replacement commit. Preserve the
unlisted file **RETAIN IN PLACE** until its owner confirms whether it is
historical source or has another purpose. Do not add it beside the active
giver #5028 branch or delete it from this audit alone. See the
[Labyrinth typed audit](LABYRINTH_OF_NO_RETURN_AUDIT_REVIEW.md#typed-references-and-compiled-numeric-matches),
[existing builder question](LABYRINTH_OF_NO_RETURN_AUDIT_REVIEW.md#builder-questions-and-recommended-resolutions),
[candidate row 58](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L58), and
[coverage row 408](ZONE_COVERAGE_REGISTER.csv#L409).

### Checkpoint 907: Labyrint's parallel QST remains a separate source identity

The unlisted `labyrint.qst` repeats active `labyrinth.qst` giver #5028,
the same ten item-command VNUMs, and the 50,000-experience reward amount.
The active story sidecar records that same branch, and only the `labyrinth`
spelling appears in the production area lists. This closes the current
typed-role match but does not establish a direct rename: followed
`labyrint.qst` history still shows the shared 2009 tree move and a 2013
same-path edit, with no replacement commit. Keep the old file **RETAIN IN
PLACE** until its author or quest owner establishes provenance and intended
use. See the [Checkpoint 907 audit](LABYRINTH_OF_NO_RETURN_AUDIT_REVIEW.md#checkpoint-907-labyrint-qst-branch-retains-active-labyrinth-item-roles),
[embedded builder question](LABYRINTH_OF_NO_RETURN_AUDIT_REVIEW.md#builder-questions-and-recommended-resolutions),
[candidate row 58](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L58), and
[coverage row 409](ZONE_COVERAGE_REGISTER.csv#L410).

### Checkpoint 496: Legion Guildhall source identities and VNUM collisions

The normalized ZON title “Legion Guildhall” is unique, while its header #295
is shared by Legion, Genocide Hall, and Void Guildhall. Active Harrow's AREA
range spans *294–296, but no active room record uses Legion's #29500–#29523
band. This distinction does not resolve the unlisted source identities.

Three other inactive maps reuse Legion room VNUMs: AoD has all 16, Genocide
Hall has seven, and RFOP has three; every inspected same-number room has
different content. Legion mobiles #29500–#29502 overlap AoD and #29500–#29503
overlap Genocide Hall, with different definitions. Legion objects #29500–#29503
overlap AoD objects, including the badge-versus-portal collision at #29500.
The 26 Legion exits that numerically target AoD's range are local to the
unlisted Legion map; together with Genocide Hall's ten and RFOP's four they
are not active ingress into AoD.

Followed histories remain on the Legion source paths: the 2009 directory move
and 2013 generic “stuff” edit are shared path history, the WLD also has a 2011
typo correction, and MOB/OBJ/ZON also have the 2012 reset-format field update.
No direct rename, retirement, or whole-area replacement decision was found
in the bounded histories. Keep all competing identities **RETAIN IN PLACE**
until their builders resolve the maps, prototypes, header use, shop, code, and
ownership. See the [Legion audit](LEGION_GUILDHALL_AUDIT_REVIEW.md),
[25 builder questions](LEGION_GUILDHALL_BUILDER_QUESTIONS.md),
[AoD comparison](AOD_GH_AUDIT_REVIEW.md),
[candidate row 59](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L59), and
[coverage row 409](ZONE_COVERAGE_REGISTER.csv#L410).

### Checkpoint 908: Legion same-number portal and shop roles remain source-local

The typed-role refresh does not change Legion's CP496 bounded-history result:
no direct rename, retirement, or whole-area replacement evidence was found.
AoD's #29501–#29504 portal objects store destinations that AoD WLD also
defines; RFOP's #29511 destination #29512 has an RFOP WLD definition. These
records do not choose among the competing unlisted room identities or prove
active travel. AoD's shop product #29500 is its own badge object, while its
keeper/room and Genocide Hall's keeper/room remain source-local #29501/#29503
roles. Legion's #29500 keeper and #29504 room likewise stay on its unlisted
SHP record. No exact QST/story item consumer or additional direct code lookup
was found beyond the unresolved #29502 inn binding. Keep the competing sources
**RETAIN IN PLACE** pending map, reset, code, shop, and provenance owners. See
the [CP908 role audit](LEGION_GUILDHALL_AUDIT_REVIEW.md#checkpoint-908-typed-role-refresh),
[25 builder questions](LEGION_GUILDHALL_BUILDER_QUESTIONS.md),
[candidate row 59](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L59), and
[coverage row 409](ZONE_COVERAGE_REGISTER.csv#L410).

### Checkpoint 497: Light Guildhall room and prototype collisions

The normalized title “The Light's Guildhall” is unique, but header #300 is
shared with active Underworld3. All 16 Light Guildhall room VNUMs
#30000–#30012 and #30020–#30022 have active Underworld3 room definitions with
different map content. The active area list and quest-area list include
Underworld3; both omit light_gh. Moonshae #26272's active edge to #30000 is
the reciprocal boundary into Underworld3's vertical shaft, not an entrance
into the unlisted Light Guildhall entrance.

All four candidate mobiles and objects also share same-type VNUMs with active
Underworld3. The candidate's beams and Yariena differ from the active Scuren,
lizardman, and slime pools; its four portal objects differ from the active
ring, sword, headband, and armor. Underworld3's ZON has 33 M commands and four
E commands using those current active prototypes. Keep its room, mobile,
object, route, and reset ownership intact until map and source owners resolve
the parallel guildhall identities.

The Light title and current path history do not establish that it replaced or
was replaced by Underworld3. The candidate's September 2026 ZON/SHP edit adds
Heavens gathering bags #491–#494 to its reset and shop lists. Preserve this
recent unlisted-source change for editor/export review. See the
[Light Guildhall audit](LIGHT_GH_AUDIT_REVIEW.md),
[25 builder questions](LIGHT_GH_BUILDER_QUESTIONS.md),
[Underworld3 review](UNDERWORLD_AUDIT_REVIEW.md#underworld3-room-overlap-and-the-compiled-inn-binding),
[candidate row 60](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L60), and
[coverage row 410](ZONE_COVERAGE_REGISTER.csv#L411).

### Checkpoint 909: active Underworld3 reset roles remain separate from Light Guildhall

The exact-number refresh keeps the active Underworld3 definitions attached to
their current source: 33 M resets consume its mobiles #30000–#30003 and four E
resets consume its objects #30000–#30003. Two Underworld3 M-reset room operands
#30004 and #30005 also name active Underworld3 rooms, despite Light Guildhall
having distinct unlisted room records at those numbers. These source roles do
not resolve the competing room, mobile, or object identities or establish a
Light Guildhall route. External QST/story #30000 hits are currency or experience
amounts; no external shop or portal/switch destination selects the candidate.
The #30000 compiled-code matches are unrelated constants and map-band
arithmetic, not a direct identity binding. No bounded-history evidence changes
the CP497 no-replacement finding. Keep both sources **RETAIN IN PLACE** pending
map, prototype, shop, deployment, and provenance decisions. See the
[CP909 role audit](LIGHT_GH_AUDIT_REVIEW.md#checkpoint-909-typed-role-refresh),
[25 builder questions](LIGHT_GH_BUILDER_QUESTIONS.md),
[candidate row 60](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L60), and
[coverage row 410](ZONE_COVERAGE_REGISTER.csv#L411).

### Checkpoint 498: Lords of Chaos Guildhall source identities and VNUM collisions

The normalized title “Lords of Chaos GuildHall” is unique, but ZON header
#293 is shared with unlisted BattleRager Hall and Ebon Skull Guildhall. Active
Newbie's *292–293 area range defines 14 of loc_gh's 22 room numbers with
different map content. Other unlisted maps also overlap the candidate room
numbers: cbr_hall has ten, eskull_hall thirteen, ravenmis ten, and ethdawn_op
three. Representative same-number room descriptions identify distinct
sources, including the abandoned cave versus Newbie's Crypt of the Undead at
#29300 and the candidate Inn of the Cavern versus Newbie's Secret Magic Shop
at #29305.

Candidate mobile IDs #29300–#29303 and object IDs #29300–#29305 overlap
active Newbie prototypes with different content. The active QST rewards
Newbie objects #29300 and #29302, and its giver #29304 is also a Newbie
identity. Compiled code assigns bandage callbacks to Newbie mobiles #29303
and #29304 and assigns an inn callback to active room #29305. Keep those
Newbie prototypes, quest references, room, and code bindings attached to the
active map; do not resolve collisions by activating loc_gh. The candidate
mobile IDs #29330–#29331 have no matching active Newbie mobile definition.

Followed histories show the 2009 area-tree move, a 2013 permission-only edit
to WLD/ZON/SHP, punctuation updates to the four portal descriptions in
loc_gh.obj in 2016, and the 2026 Heavens bag additions to ZON/SHP. No direct
rename, retirement, or whole-area replacement decision appears in these
bounded file histories. Keep every competing source **RETAIN IN PLACE** until
its owner resolves room, typed-VNUM, header, code, quest, shop, and deployment
ownership. See the
[Lords of Chaos audit](LOC_GH_AUDIT_REVIEW.md),
[25 builder questions](LOC_GH_BUILDER_QUESTIONS.md),
[Newbie audit](NEWBIE_AUDIT_REVIEW.md),
[candidate row 60](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L61), and
[coverage row 411](ZONE_COVERAGE_REGISTER.csv#L412).

### Checkpoint 910: loc_gh reset, shop, and portal roles remain separate

A typed source sweep confirms the overlapping prototypes remain distinct.
Candidate mobile #29300–#29303 shares same-type definitions with active Newbie
and unlisted CBR Hall, Ebon Skull, and Ravenmis; candidate mobile #29330/#29331
has no outside same-type definition. Candidate objects #29300–#29305 overlap
Newbie, while CBR Hall #29300, Ebon Skull #29300/#29301, and Ravenmis
#29300/#29301 keep their own source definitions.

Active Newbie's reset references and quest/story item fields resolve to its own
prototypes. The active #29305 shop product and portal targets #29305/#29308
also remain Newbie roles. CBR Hall, Ebon Skull, Ravenmis, and loc_gh SHP records
retain separate keeper, room, and stock fields; the possible SHP glob fallback
can expose the unlisted N-format records but was not run. Unlisted Ebon Skull
portal targets #29312/#29309 remain local to that WLD; Ethdawn's #29310 target
#29307 is source-only and numerically matches active Newbie/loc_gh rooms. Code
continues to bind Newbie's #29303/#29304 mobiles and #29305 room. The corrected
16 Newbie WLD directions still target active Newbie identities; no replacement
or active loc_gh ingress follows from any shared VNUM. Preserve all sources
**RETAIN IN PLACE** pending owner decisions. See the
[CP910 role audit](LOC_GH_AUDIT_REVIEW.md#checkpoint-910-role-aware-source-closure),
[25 builder questions](LOC_GH_BUILDER_QUESTIONS.md),
[candidate row 60](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L61), and
[coverage row 411](ZONE_COVERAGE_REGISTER.csv#L412).

### Checkpoint 499: Luiren giver and legacy shop VNUM overlaps

Luiren has no WLD, MOB, OBJ, or ZON source and no title that defines a map
identity. Its 44 old QST M-dialogue giver headers all overlap active Pine
Hollow mobile VNUMs. Eight legacy O-format SHP records reuse eight Pine
Hollow keeper numbers, list 16 Pine Hollow object products, and contain two
room fields matching Pine Hollow rooms #16008 and #16058. These exact numeric
collisions do not make the source active, establish a Luiren route, or prove
that Luiren replaced Pine Hollow. The shop loader's old-format rejection and
conditional glob fallback keep the SHP ownership and input path material.

The QST and SHP file histories show the 2009 area-tree move and a 2013
permission-only change, with no content edit in that 2013 commit and no direct
rename or whole-area replacement decision. Preserve both Luiren sources
**RETAIN IN PLACE** until the Pine Hollow, QST, and shop owners resolve giver,
keeper, product, and room identity and the deployment input. See the
[Luiren audit](LUIREN_AUDIT_REVIEW.md),
[Pine Hollow audit](PINE_HOLLOW_AUDIT_REVIEW.md#luiren-candidate-identity-and-old-shop-data),
[20 builder questions](LUIREN_BUILDER_QUESTIONS.md),
[candidate row 61](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L62), and
[coverage row 412](ZONE_COVERAGE_REGISTER.csv#L413).

### Checkpoint 500: Magetower low-VNUM source identities and active Limbo and Heavens ownership

The normalized title “The Tower of the Adamantine Mage” is unique, but ZON
header #1000 is shared with cbl_outpost2, monsteri, and random. Magetower's
71 room IDs #1–#71 are outside that header's numeric range. Its editor map
file contains coordinates for all 71 IDs, but neither area list includes
Magetower.

Active Limbo defines room #1; active Heavens defines 42 other Magetower room
numbers (#3–#5, #10–#46, #66, and #69), all as different map records. The
active owners also cover 22 of the 23 Magetower mobiles: Limbo owns #1–#6,
Heavens owns #7–#17 and #19–#23, and #18 has no same-number active mobile.
All 20 candidate object IDs overlap active Limbo #1–#9 or Heavens #10–#20.
The separate areas_mini fixture shares 27 room, 17 mobile, and 20 object
numbers; that is a separate source input, not evidence of normal active map
ownership.

Compiled numeric behavior reinforces the active ownership: the global
necro_dracolich assignment covers mobile #3–#10, Heavens binds #15 and room
#17/#44, Limbo uses room #1 and object #4, and other room/object code sites
use IDs also defined by Magetower. Candidate #2/#64 edges to #1 are local to
its source graph but resolve numerically to active Limbo in the current table.
Followed histories contain no direct Magetower rename, retirement, or whole
area replacement decision. Keep Magetower, Limbo, Heavens, and the mini
fixture **RETAIN IN PLACE** under their present list and path ownership until
the map, code, reset, and editor owners reconcile the numbers. See the
[Magetower audit](MAGETOWER_AUDIT_REVIEW.md),
[25 builder questions](MAGETOWER_BUILDER_QUESTIONS.md),
[Limbo review](LIMBO_STORY_REVIEW.md),
[candidate row 62](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L63), and
[coverage row 413](ZONE_COVERAGE_REGISTER.csv#L414).

### Checkpoint 501: map1 empty file and ambiguous legacy path history

No room, mobile, object, QST, or zone prototypes exist in the zero-byte
`areas/mob/map1.mob`, so there are no numeric identities to collide or use
as replacement evidence. Git's rename-follow history connects it to two
earlier zero-byte `Players/` paths, but identical empty blobs and destination
additions do not establish a shared area purpose. Keep `map1` distinct from
the separate `ud_map1` sources and from the unrelated `MAP1_FILE` information
path. No direct map rename, retirement, or whole-area replacement decision is
established; retain the file pending an owner decision. See the
[map1 audit](MAP1_AUDIT_REVIEW.md#path-history-and-identity-limits),
[10 builder questions](MAP1_BUILDER_QUESTIONS.md),
[candidate row 63](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L64), and
[coverage row 414](ZONE_COVERAGE_REGISTER.csv#L415).

### Checkpoint 502: map22 template, random-lab code, and Duris3 path ownership

The tracked `map22.wld` has one 10,000-room grid identity, no same-stem area
prototypes or zone title, and no exact active WLD room collision or external
WLD target. The type-1 random-lab code conditionally connects Duris3 room
#145443 to map22 #905050 when the required room records exist and the create
path runs. That code binding is not a source rename, map merge, or current
WLD route. Followed history reaches the 2009 tracked area-tree import and
establishes no earlier replacement decision. Keep both WLD identities
separate pending their map, generator, and deployment owners' review. See the
[map22 audit](MAP22_AUDIT_REVIEW.md),
[Duris3 audit](DURIS3_AUDIT_REVIEW.md),
[20 builder questions](MAP22_BUILDER_QUESTIONS.md),
[candidate row 64](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L65), and
[coverage row 415](ZONE_COVERAGE_REGISTER.csv#L416).

### Checkpoint 503: map5 mobile text copies and distinct numeric identities

The unlisted `map5.mob` defines 40 VNUMs with no same-number definitions in
other tracked MOB sources. Some prototypes share full or descriptive text
with unlisted `surf.mob` and active Minizones and Killing Fields mobiles under
different VNUMs. In particular, map5 Maelron #150002 and merchant #150003
share descriptive text with active Killing Fields #33300 and #33308; the
active QST and shop remain keyed to their own VNUMs. The 45 `surf.zon` M
commands are source-only placements, not evidence of a map rename or
replacement. Followed history reaches the 2009 area-tree import only. Retain
the distinct mobile identities while owners reconcile code, Surf resets,
and shared prose. See the [map5 audit](MAP5_AUDIT_REVIEW.md),
[20 builder questions](MAP5_BUILDER_QUESTIONS.md),
[candidate row 65](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L66), and
[coverage row 416](ZONE_COVERAGE_REGISTER.csv#L417).

### Checkpoint 504: map8 empty-file path history does not establish zone identity

The zero-byte `areas/mob/map8.mob` defines no prototypes or numeric identities
to collide. Git's rename-follow heuristic connects its empty blob through
`Players/justice_list` and `Players/Declined/...`, paths also seen for the
separate empty map1 file; that similarity cannot establish a shared purpose,
rename, retirement, or replacement. Keep map8, map1, and the separately named
`oldmap8.wld` distinct pending owner evidence. See the
[map8 audit](MAP8_AUDIT_REVIEW.md#path-history-and-identity-limits),
[10 builder questions](MAP8_BUILDER_QUESTIONS.md),
[candidate row 66](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L67), and
[coverage row 417](ZONE_COVERAGE_REGISTER.csv#L418).

### Checkpoint 505: Mara Gh and Mara Op shared VNUMs do not establish a merge

The unlisted `mara_gh` and `mara_op` sources define the same WLD room VNUMs
#29610–#29612, mobile VNUMs #29610/#29611, and object VNUMs #29610/#29611.
The room names/exits match while room flags differ; mobile stat fields and
portal object fields also differ. Mara Guildhall additionally repeats the
complete mobile record bodies #29605/#29610 and #29606/#29611 under distinct
VNUMs. Keep both source families and each numeric identity separate until
their owners establish whether the copies are shared inputs, alternate
exports, or legacy data; no replacement or merge is established. See the
[Mara Guildhall audit](MARA_GH_AUDIT_REVIEW.md#shared-vnums-history-and-disposition),
[20 builder questions](MARA_GH_BUILDER_QUESTIONS.md),
[Mara Outpost candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L69),
[candidate row 67](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L68), and
[coverage row 418](ZONE_COVERAGE_REGISTER.csv#L419).

### Checkpoint 506: Mara Outpost remains distinct pending shared VNUM ownership

The unlisted `mara_op` and `mara_gh` sources each define WLD rooms #29610–#29612,
MOBs #29610/#29611, and OBJs #29610/#29611. Room text/exits match but flags
differ; mobile prose matches but stats differ; portal values share targets
while other fields differ. Both zone files reset the shared prototypes. The
Mara Outpost portal #29610 also targets Mara Guildhall room #29608. These
source conflicts and one typed cross-component reference establish neither
a live collision nor a replacement. Retain both identities until map/item
owners resolve them. See the [Mara Outpost audit](MARA_OP_AUDIT_REVIEW.md#shared-vnums-history-and-disposition),
[Mara Guildhall audit](MARA_GH_AUDIT_REVIEW.md#shared-vnums-history-and-disposition),
[20 builder questions](MARA_OP_BUILDER_QUESTIONS.md),
[candidate row 68](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L69), and
[coverage row 419](ZONE_COVERAGE_REGISTER.csv#L420).

### Checkpoint 507: Mavulsk VNUM overlap and boundary identity

Every Mavulsk WLD room number #1–#233, mobile #1–#98, and object #1–#110
has a same-type definition in at least one other source file. Active
same-number owners exist for 48 rooms, 97 mobiles, and all 110 objects; each
complete Mavulsk block at those IDs differs from its active owner. The
current active matches therefore do not supply Mavulsk room or prototype
identity. In particular, active Limbo #1 does not replace Mavulsk #1,
“Before the Stone Mansion.”

Mavulsk #104 is a quiet library, distinct from Dalvik #104, the Great Temple
of Ubtao. Both sources remain outside areas/AREA, and Thetis #38095 north →
#104 stays unresolved. Mavulsk #188 east → #228202 also remains unresolved:
surf and surface2011 each define #228202 under different titles, and neither
WLD returns to Mavulsk #188. Followed per-file history shows same-stem
maintenance/content edits but no alternate-stem rename or whole-zone
replacement. Preserve the five Mavulsk source files and both map identities.
See the [Mavulsk audit](MAVULSK_AUDIT_REVIEW.md#same-type-identities-history-and-disposition),
[Thetis audit](THETIS_AUDIT_REVIEW.md#active-graph-and-boundary-exits),
[Dalvik audit](DALVIK_AUDIT_REVIEW.md#static-wld-graph),
[25 builder questions](MAVULSK_BUILDER_QUESTIONS.md),
[candidate row 69](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L70), and
[coverage row 420](ZONE_COVERAGE_REGISTER.csv#L421).

### Checkpoint 508: Mentiri shared low VNUMs and no replacement evidence

Mentiri’s 72 room, 30 mobile, and 20 object VNUMs all have same-type owners
in other source files. Current active same-number owners exist for 43 rooms,
29 mobiles, and all 20 objects; each candidate block differs from its active
Limbo/Heavens owner. The separate areas_mini fixture also reuses 27 room,
24 mobile, and 20 object IDs, with different complete prototype blocks. Keep
these as competing numeric records; none establishes shared canonical
ownership.

The unique exact source title “Mentiri, the Prison of Dis” establishes a
setting, not a replacement for “Hell, Dis” or the other titled Hell planes.
Followed histories show same-stem edits and no alternate-stem rename,
retirement, or whole-area replacement. Preserve the unlisted source and map
JSON as a distinct component pending owner evidence. See the
[Mentiri audit](MENTIRI_AUDIT_REVIEW.md#resets-source-identity-and-history),
[22 builder questions](MENTIRI_BUILDER_QUESTIONS.md),
[candidate row 70](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L71), and
[coverage row 421](ZONE_COVERAGE_REGISTER.csv#L422).


### Checkpoint 509: Monsteri copies and empty-file history

Monsteri and cbl_outpost2 share the exact “CBL Guildhall Monster Island”
title, room VNUMs #1000–#1004, mobile VNUMs #1000–#1003, and four M reset
rows into room #1000. Monsteri supplies a detailed bloodied gatehouse at
#1000; cbl_outpost2 instead keeps “First Room” with an empty description.
The five Monsteri room descriptions match active cbl_hall #29820–#29824 after
color and whitespace normalization, with four exact normalized titles. This
is duplicate-content evidence across separate VNUMs, not proof that either
unregistered source replaced the active CBL map or the other candidate.

Monsteri’s mobile VNUMs also collide with active Heavens #1000–#1003 and the
separate areas_mini peasant fixtures; those identities differ from the four
golems. Its empty OBJ/QST rename-follow histories run through
Players/Declined/... and Players/justice_list, so the path heuristic does not
identify a Monsteri predecessor or editor owner. WLD/MOB/ZON histories stay
on the Monsteri stem from the 2009 tree move through later same-stem edits.
No canonical-source decision, alternate-stem rename, retirement, or whole-
area replacement is established. Preserve every source path and the active
CBL identities pending owner evidence. See the
[Monsteri audit](MONSTERI_AUDIT_REVIEW.md#duplicate-content-and-separate-prototype-identities),
[24 builder questions](MONSTERI_BUILDER_QUESTIONS.md),
[CBL Outpost 2 audit](CBL_OUTPOST2_AUDIT_REVIEW.md),
[candidate dependency line 72](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L72), and
[coverage line 423](ZONE_COVERAGE_REGISTER.csv#L423).


### Checkpoint 510: moriatst shop copy and loader path

The single unregistered `moriatst.shp` file carries three shop records with
the same keeper VNUMs, stock object VNUMs, and Moria `in_room` values as
active `moria.shp`. The records share their shop IDs, stock lists, and
transaction text; the raw buy multipliers differ, and candidate #99037 also
has sell multiplier 1.00 versus active Moria's 1.10. The loader clamps
candidate buy values above 0.80 to 0.80 but preserves the 1.00 sell value.
Minor trailing-space differences occur in three empty prompts in the third
shop. Tracked history stays on the
`moriatst` stem from the 2009 tree move through the 2013 update; it does not
establish a predecessor, successor, or replacement relation to Moria.

The two files are separate on the normal `AREA` build path, which omits
`moriatst`. The runtime fallback instead globs every `areas/shp/*.shp` file
when `areas/world.shp` cannot be opened, so it can concatenate both versions.
Those duplicate keeper-and-room records make the reset-side selector's
source match ambiguous. This conditional code path is not proof of a live
deployment. Keep the source identity and loader behavior distinct until an
owner resolves intent; see the [moriatst audit](MORIATST_AUDIT_REVIEW.md),
[Moria correction](MORIA_AUDIT_REVIEW.md#checkpoint-510-supplement-moriatst-shop-loader-path),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-510-moriatst-retained-pending-shop-loader-closure),
[candidate dependency line 73](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L73),
and [coverage row 424](ZONE_COVERAGE_REGISTER.csv#L424).


### Checkpoint 511: Mountain QST and active mount

The unregistered uppercase `Mountain.qst` and active lowercase `mount.qst`
are distinct current files. The full-area and QST manifests select `mount`,
whose quest file has different giver IDs and Lokpan/prison content. The
candidate file instead describes the Givnab/Sorlag smithing chain and Gulik's
order quest. Its same-path history runs through the area-tree import and
2013 update, but establishes no rename, predecessor, or replacement relation
to active `mount`. The name overlap alone cannot identify its missing map or
editor. Preserve the two source identities pending owner evidence; see the
[Mountain QST audit](MOUNTAIN_QST_AUDIT_REVIEW.md),
[active Mountain review](MOUNTAIN_OF_THE_BANISHED_AUDIT_REVIEW.md#checkpoint-511-separate-unregistered-mountain-qst),
[candidate dependency line 74](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L74),
and [coverage row 425](ZONE_COVERAGE_REGISTER.csv#L425).

### Checkpoint 512: mtncastl title and placeholder collisions

The exact title “The Horrible Tracts of the Unnamed” appears in the five
current ZON files astral, eoo_op2, mtncastl, theurgist, and ud-inside.
Separately, the complete mtncastl WLD block for room #1 is byte-identical to
the astral and ud-inside placeholders. These are distinct title and content
comparisons; neither identifies a canonical source or proves a replacement.
Room #1 is reserved as ROOM_LIMBO_VNUM, and active Limbo is its sole active
WLD owner. Followed mtncastl WLD/ZON paths show no explicit successor or
whole-zone replacement. Preserve all four candidate paths pending owner and
editor evidence; see the [mtncastl audit](MTNCASTL_AUDIT_REVIEW.md),
[candidate dependency line 75](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L75),
and [coverage row 426](ZONE_COVERAGE_REGISTER.csv#L426).
### Checkpoint 513: myranth1 provenance and story link

The tracked myranth1 QST and SHP paths reach the 2009 area-tree move and a
same-stem 2013 update; no WLD, MOB, OBJ, or ZON remains in this package and
the reviewed histories show no explicit successor or whole-area replacement.
Candidate Moorney’s quest text aligns with active Knight Zyla #70919’s
backstory, while the five candidate shop bindings point to active Moregeeth
mobile and room identities. Those narrative and typed references identify
relationships to review, not a replacement or one canonical owner. See the
[myranth1 audit](MYRANTH1_AUDIT_REVIEW.md), [candidate dependency line 76](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L76), and
[coverage row 427](ZONE_COVERAGE_REGISTER.csv#L427).
### Checkpoint 514: Nah and competing room identities

Nah defines 13 rooms #30100–#30112. Active Underworld3 currently owns every
one of those numeric rooms with different river/lake content. Unregistered
cabal_hall overlaps Nah on #30100–#30109, while shadow_hall overlaps on
#30100–#30102; their room blocks are also distinct. Nah mobiles #30100 and
#30101 separately collide with cabal_hall and shadow_hall mobiles of
different identities. Nah’s two Duris3 Ocean exits and followed same-stem
histories establish neither a successor nor whole-area replacement. Preserve
all competing sources; see the [NAH audit](NAH_AUDIT_REVIEW.md),
[Cabal Hall overlap review](CABAL_HALL_AUDIT_REVIEW.md#active-underworld3-ownership-of-room-vnums),
[candidate dependency line 77](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L77), and
[coverage row 428](ZONE_COVERAGE_REGISTER.csv#L428).

### Checkpoint 515: Newbiemaps source identity and copy history

The unregistered newbiemaps.wld has 89,847 rooms and 359,196 exit tuples.
Active Surface owns 59,851 matching room numbers, with no equal title strings
at those overlaps. surface2011.wld is a February 2011 Git copy from
newbiemaps.wld; surface2011-temp.wld is a May 2011 copy. The current temp
map shares every candidate room number and every candidate exit tuple, while
adding 153 rooms and 804 directions. Only 72 of the common room numbers have
equal titles. This establishes a copied-map family, not which source was
canonical or whether any path replaces active Surface. The 602 source-only
directions into Surface are already recorded in the
[Surface audit](SURFACE_AUDIT_REVIEW.md#inactive-wld-surface-approaches);
preserve each file’s map identity without counting the shared tuple set twice.
No explicit successor or retirement decision was found. See the
[NEWBIEMAPS audit](NEWBIEMAPS_AUDIT_REVIEW.md),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-515-newbiemaps-retain-both-map-identities),
[candidate dependency line 78](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L78), and
[coverage row 429](ZONE_COVERAGE_REGISTER.csv#L429).


### Checkpoint 516: Nexus56 title and VNUM conflicts

The exact ZON title “Nexus” is unique among the 442-title scan. Registered
`nexus` is a separate entry titled “the Mountain of Peril Peaks.” Nexus56
room IDs overlap active Alatorin on 659 VNUMs and Delwyn on 7, with no exact
title matches. Its mobile IDs overlap Alatorin on 102 prototypes and object
IDs on 168; none of those same-type candidate blocks is identical to the
active Alatorin block. Those 168 object IDs also appear in
`alatorin_objects.obj`, where none matches Nexus56 and 76 match active
Alatorin. The tracked Nexus56 file histories run from the 2009 area-tree move
through later same-path edits; no successor, rename, or whole-area
replacement was found. This is a set of competing identities, not a
replacement decision. Preserve the five source paths pending owner and editor
evidence. See the [Nexus56 audit](NEXUS56_AUDIT_REVIEW.md),
[candidate dependency row 78](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79),
and [coverage row 429](ZONE_COVERAGE_REGISTER.csv#L430).

### Checkpoint 517: Nexus_1 low-VNUM identity conflicts

The exact normalized ZON title `Nexus_1` is unique, but room IDs are heavily
reused. Active Limbo and Heavens own 35 candidate IDs, `areas_mini/mini.wld`
shares 26, and ten other WLD sources also overlap at least one ID. No
same-number room has an exact title or complete-block match in those files;
the two shared mini-map tuples are #19 east to #20 and #20 west to #19.
Nexus_1 is absent from both current area manifests. Bounded path history
shows the 2009 WLD move and a 2015 ZON edit, with no tracked successor or
retirement decision. Numeric overlap does not establish a copy, alias, or
replacement. Preserve this low-VNUM source identity in place pending owner,
editor, and persisted-reference evidence. See the
[Nexus_1 audit](NEXUS_1_AUDIT_REVIEW.md), [retention plan](ATTIC_MIGRATION_PLAN.md),
[candidate dependency row 79](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L80),
and [coverage row 430](ZONE_COVERAGE_REGISTER.csv#L431).

### Checkpoint 518: oldmap8 and Duris3 share a 10,000-VNUM grid

The source-only `oldmap8.wld` and `Duris3.wld` both define every room ID
#180000–#189999. No same-number pair has the same room title or complete
record. They share 19,759 exact direction/source/target tuples, while 20,159
oldmap8 and 20,241 Duris3 tuples are unique to one file. The shared grid edges
and VNUMs coexist with different terrain labels, room blocks, and neighbors.
Tracked path histories show no explicit whole-map successor or retirement
decision. Keep both source identities distinct; do not infer replacement from
the later oldmap8 commit or the shared edges. See the
[oldmap8 audit](OLDMAP8_AUDIT_REVIEW.md), [Duris3 audit](DURIS3_AUDIT_REVIEW.md),
[retention plan](ATTIC_MIGRATION_PLAN.md),
[candidate dependency row 80](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L81),
and [coverage row 431](ZONE_COVERAGE_REGISTER.csv#L432).

### Checkpoint 519: Petrified Castle story package with no same-type collision

Petrified Castle contributes 187 rooms, 42 mobiles, and 49 objects; the
repository scan found no current active or other source WLD/MOB/OBJ
same-type VNUM collision for those records. Its normalized ZON title “Tower
of Testing” is unique among 425 source ZON titles scanned. Bounded same-path
history shows the old area addition and later content/format edits but no
explicit successor, full replacement, or retirement decision. The WLD/QST
castle-and-kingdom story does not resolve whether the test title is an alias
or intended use. Preserve this package identity pending builder and editor
evidence; numeric absence from current manifests is not a replacement
decision. See the [Petrified Castle audit](PETRIFIED_CASTLE_AUDIT_REVIEW.md),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-519-petrified-castle-retain-pending-owner-and-reference-closure),
[candidate dependency row 81](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L82),
and [coverage row 432](ZONE_COVERAGE_REGISTER.csv#L433).

### Checkpoint 520: plane_avernus title collision and three source room identities

The inactive plane_avernus ZON title exactly matches active Avernus zone #320,
but its two WLD rooms (#135–#136) are outside active Avernus’s #32000–#32399
range and no active WLD owner or inbound target uses them. The same two room
numbers also appear in the unlisted Dalvik and Mavulsk WLD files, with
different titles, room blocks, and exits. Heavens has active OBJ prototypes
at #135/#136, a separate type of VNUM namespace; plane_avernus has no OBJ
prototypes. Bounded path history and Checkpoint 192 show no explicit
predecessor, rename, or replacement. Preserve all source identities pending
builder/editor provenance. See the
[plane_avernus audit](PLANE_AVERNUS_AUDIT_REVIEW.md),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-520-plane_avernus-retain-pending-map-identity-and-provenance),
[candidate dependency row 82](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L83),
and [coverage row 433](ZONE_COVERAGE_REGISTER.csv#L434).

### Checkpoint 521: Player Guilds shared shop sources, not zone replacement

player_guilds is a QST/SHP-only catalog with no room or population sources.
Its SHP file has 13 records and 97 product rows; bounded same-path history
shows the 2009 path move and 2013 “stuff” update, with no explicit successor
or retirement decision. Shop #35300 overlaps unlisted ddguild.shp at the same
Asunwa keeper #35300 and room #35303, but the stock and shop flags differ.
Active cross-area items and locations are source dependencies, not evidence
that player_guilds replaces their owner areas. See the
[Player Guilds audit](PLAYER_GUILDS_AUDIT_REVIEW.md),
[Asunwa dependency review](ASUNWA_AUDIT_REVIEW.md#typed-dependency-census),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-521-player-guilds-retain-shop-records-pending-owner-closure),
[candidate dependency row 83](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L84),
and [coverage row 434](ZONE_COVERAGE_REGISTER.csv#L435).

### Checkpoint 522: Ravenmis distinct guildhall source

The only exact current “ravenmist guildhall” ZON title is on the unlisted
Ravenmis component. Its ten rooms #29300–#29309 have different complete
records from same-number active Newbie rooms and from unlisted cbr_hall,
loc_gh, and eskull_hall. Its mobiles #29300–#29304 and objects #29300/#29301
also reuse VNUMs with distinct current source identities. Bounded path history
shows the 2009 area-tree move and 2013 “stuff” update; no explicit predecessor,
successor, or retirement decision was found. Keep these map identities
separate and RETAIN Ravenmis pending owner and editor provenance. See the
[Ravenmis audit](RAVENMIS_AUDIT_REVIEW.md),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-522-ravenmis-retain-source-identities-pending-owner-decisions),
[candidate dependency row 84](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L85),
and [coverage row 435](ZONE_COVERAGE_REGISTER.csv#L436).

### Checkpoint 523: Raxquest QST mirror and standalone pirate wreck identity

The raw ZON title Raxquest appears once in the current title scan. Same-type
scans found no outside WLD, MOB, or OBJ definitions using its VNUMs, and the
bounded followed-path history shows no alternate-stem predecessor, explicit
successor, or retirement decision. Separately, the 462-line Raxquest QST is an
exact normalized copy of the 462-line FooQuest and Black Pearl QST files:
normalizing each file's local 650xx, 705xx, or 142xxx tokens makes every line
equal. This is strong story-source reuse evidence, while not proving that
FooQuest or Black Pearl replaces the Raxquest map, mobiles, objects, or reset
file. At the 2012 Black Pearl creation commit Git copy detection also marks
blackpearl.obj as C091 from raxquest.obj. This is OBJ-level source reuse and
does not prove whole-zone replacement. Black Pearl is a full-area WLD source,
but its per-stem QST is not named in qst/AREA and the expected areas/world.qst
aggregate is absent. Keep the OBJ and QST lineage questions separate from
whole-zone identity and retain Raxquest
pending loader, compiled-special, editor, and owner review. See the
[Raxquest audit](RAXQUEST_AUDIT_REVIEW.md),
[FooQuest cross-audit note](FOOQUEST_AUDIT_REVIEW.md#checkpoint-523-shared-qst-template-with-raxquest-and-black-pearl),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-523-raxquest-retain-source-files-pending-route-and-code-closure),
[candidate dependency row 85](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L86),
and [coverage row 436](ZONE_COVERAGE_REGISTER.csv#L437).

### Checkpoint 524: realms_master shop records and typed VNUM collisions

The shop-only realms_master source has two unique SHP record headers, #11108
and #11109, and no matching current WLD or MOB definitions for its configured
rooms #11121/#11124 and keepers #11108/#11109. Same-number #11108/#11109 OBJ
prototypes in active Player Castles are bulletin boards, not keeper mobiles;
five outside source ZON object resets refer to those board VNUMs. The legacy
Realms Master room-special block is under #if 0, and the source history does
not document a replacement. The full-mode shop fallback can glob the file
outside areas/AREA membership, so resolve loader and missing-keeper behavior
before considering a path change. Retain the SHP source pending owner and
deployment review. See the [realms_master audit](REALMS_MASTER_AUDIT_REVIEW.md),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-524-realms_master-retain-shop-file-pending-loader-and-keeper-closure),
[candidate dependency row 86](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L87),
and [coverage row 437](ZONE_COVERAGE_REGISTER.csv#L438).

### Checkpoint 525: returns isolated mobile-source identity

The only current same-name records “silent golem” and “diminutive cleric” are
MOB #93400 and #93401 in unlisted returns.mob. No same-type outside prototype
or exact external numeric reference was found in the bounded source scan. The
golem text points to an unidentified tower, but no same-stem WLD/ZON or
followed-path predecessor exists in this checkout. Do not infer a map owner,
placement, or replacement from the generic filename. Retain the two records
pending owner and editor provenance. See the [returns audit](RETURNS_AUDIT_REVIEW.md),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-525-returns-retain-mobile-source-pending-tower-and-placement-owners),
[candidate dependency row 87](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L88),
and [coverage row 438](ZONE_COVERAGE_REGISTER.csv#L439).

### Checkpoint 526: rf_gh reuses active Torg VNUMs without replacement proof

All 13 rf_gh room numbers #28900–#28912, all seven mobile numbers
#28900–#28906, and all five object numbers #28900–#28904 have different
same-type prototypes in listed Torg. Examples include Organ Pit vs. Thin
Trail at WLD #28900, a Rotting Flesh sigil vs. Torg hand axe at OBJ #28902,
and the rf_gh vampire shopkeeper vs. Torg duergar miner at MOB #28904. Torg
is present in areas/AREA and qst/AREA; rf_gh is absent from both.

The followed path shows both rf_gh.wld and torg.wld changed in the same
November 13, 2013 “stuff” commit, but no explicit predecessor, successor, or
retirement decision. That establishes shared repository history, not which
map should own a VNUM or that Torg replaces the guildhall. A current
surface.wld target to #28900 resolves numerically to Torg; it is not an
inbound route to the absent rf_gh identity. The only exact C/C++ hit for
these numbers is #28911 in a generated item-VNUM table, matching Torg OBJ,
not a room binding. Retain the source files pending joint area and shop-owner
review. See the [rf_gh audit](RF_GH_AUDIT_REVIEW.md),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md),
[candidate dependency row 88](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L89),
and [coverage row 439](ZONE_COVERAGE_REGISTER.csv#L440).

### Checkpoint 527: rf_op reuses a cathedral fragment without replacement proof

After translating rf_gh rooms #28910–#28912 to rf_op #29510–#29512, the
last two WLD records match exactly and the first description differs by one
space. The two paired mobile records and portal-object descriptions and
statistics also match; portal destinations differ. Matching scene content
supports reuse of a three-room fragment, not a whole-zone predecessor or
successor relationship. The only followed-path history found for rf_op is
the 2009 area-tree move, with no explicit retirement or replacement decision.

The same room VNUMs also label different maps in unlisted aod_gh,
genocide_hall, and legion sources. Those are source collisions, not active
same-type definitions or inbound routes. “Father Benali” also appears in
active Surface mobile text, but that shared name does not identify a route or
prove character identity. Retain rf_op pending the fragment, portal,
namespace, and story-owner decisions. See the
[rf_op audit](RF_OP_AUDIT_REVIEW.md),
[rf_gh audit](RF_GH_AUDIT_REVIEW.md),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md),
[candidate dependency row 89](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L90),
and [coverage row 440](ZONE_COVERAGE_REGISTER.csv#L441).
### Checkpoint 528: scrubs_hall room/mobile overlaps and recent shop updates

All ten scrubs_hall room IDs #29100–#29109 have different same-type WLD
records in active Torg. The same range also appears in unlisted
afterlife_gh, solstice_gh, void_gh, and wlust_gh, again with different room
text. Scrubs MOB #29100–#29103 has different same-type prototypes in those
unlisted source sets; none of these four Scrubs mobile IDs has an active
same-type definition. Its tracked OBJ file is empty. These namespace
collisions do not establish that any whole map replaces another.

The Scrubs SHP/ZON and Heavens OBJ sources were updated together on
September 6, 2026 to add gathering-bag prototypes #491–#494 to the trader's
stock and carried inventory. This is recent maintenance evidence, not proof
that scrubs_hall is listed or loaded. The current code also assigns `inn`
to room #29103, which the active manifest resolves as Torg's escape tunnel,
while the candidate Scrubs description makes #29103 a pirate-ship corridor.
Followed history contains no explicit Scrubs retirement, predecessor, or
successor decision. Retain both data families pending owner review. See the
[scrubs_hall audit](SCRUBS_HALL_AUDIT_REVIEW.md),
[Afterlife Guildhall audit](AFTERLIFE_GH_AUDIT_REVIEW.md),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-528-scrubs_hall-retain-map-and-shop-source),
[candidate dependency row 90](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L91),
and [coverage row 441](ZONE_COVERAGE_REGISTER.csv#L442).
### Checkpoint 529: shadow_hall and Underworld3 room identity

Shadow Law WLD #30100–#30102 has different same-number room records in
active underworld3.wld and in unlisted cabal_hall.wld and nah.wld. Its one
MOB #30100 also differs from unlisted cabal_hall and nah mobile prototypes;
there is no active same-type MOB #30100. The two source ZON object resets
point to active Player Castles prototypes #11110 (bulletin board) and #11060
(magic mouth). These typed imports do not show that the map is active.

The four local Shadow Law exits resolve numerically to Underworld3 in the
current manifest. Eight active Underworld3 exits target the same room
numbers, and its zone resets a water weird in #30101; those remain
Underworld3-owned identities. The outlet to #151052 is only in unlisted
Duris3. Followed history has no explicit retirement or replacement decision.
Retain the candidate map and preserve the Player Castles imports pending
owner review. See the [shadow_hall audit](SHADOW_HALL_AUDIT_REVIEW.md),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-529-shadow_hall-retain-guildhall-source),
[candidate dependency row 91](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L92),
and [coverage row 442](ZONE_COVERAGE_REGISTER.csv#L443).

### Checkpoint 530: shady2 shared mobile VNUMs are distinct records

Unlisted `shady2.mob` and active `shady.mob` define the same 90 mobile
VNUMs #97500–#97589, with none of the full records identical. Eighty-five
records match across all four tilde text fields. Five keyword lists differ
(#97540, #97545, #97562, #97580, and #97582); #97580 also changes its short,
long, and detailed descriptions. All 90 stat/mechanical blocks differ.
Active `shady.mob` contains 16 additional mobile IDs #97590–#97605, and the
current ZON, SHP, QST, and code references resolve to active Shady
prototypes. Followed history contains no explicit retirement or replacement
decision. Treat this as a same-type source collision, not proof that one
file supersedes the other. Retain `shady2.mob` separately pending its
owner, editor, and deployment provenance. See the
[shady2 audit](SHADY2_AUDIT_REVIEW.md),
[Shady Grove history supplement](SHADY_GROVE_AUDIT_REVIEW.md#tracked-history-and-duplicate-source-boundary),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-530-shady2-preserve-the-mob-variant),
[candidate dependency row 92](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L93),
and [coverage row 443](ZONE_COVERAGE_REGISTER.csv#L444).

### Checkpoint 531: sky_gh and Harrow competing identities

All five sky_gh WLD rooms #29400–#29404 have different same-number records
in active Harrow and in unlisted `dark.wld` and `infernal_gh.wld`. Its four
MOBs #29400–#29403 also differ from active Harrow; the first three have
additional different records in unlisted `dark.mob` and `infernal_gh.mob`.
The tracked sky_gh OBJ file is empty. Current Harrow owns the active room
and mobile VNUMs; its room #29403 is Fountain Square with a code-bound inn
function, while sky_gh calls #29403 its Inn. The candidate shopkeeper
#29403 and `in_room` #29404 likewise resolve to active Harrow prototypes
if consumed under current number lookup. Recent September 2026 additions to
sky_gh's ZON/SHP records are source maintenance, not replacement or
manifest evidence. Retain all five files separately. See the
[sky_gh audit](SKY_GH_AUDIT_REVIEW.md),
[Harrow audit](HARROW_AUDIT_REVIEW.md#unregistered-wld-target-matches),
[Infernal Guildhall audit](INFERNAL_GH_AUDIT_REVIEW.md#source-inventory-naming-and-competing-identities),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-531-sky_gh-retain-map-shop-and-imports),
[candidate dependency row 93](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L94),
and [coverage row 444](ZONE_COVERAGE_REGISTER.csv#L445).

### Checkpoint 532: solstice_gh map and prototype identities

All ten solstice_gh room VNUMs #29100–#29109 have different records in active
Torg and in unlisted afterlife_gh, scrubs_hall, void_gh, and wlust_gh. Its
three MOB VNUMs #29100–#29102 also have different unlisted definitions in
afterlife_gh, scrubs_hall, void_gh, and wlust_gh; none is active in Torg.
Solstice OBJ #29100–#29105 is not active either and conflicts with distinct
unlisted afterlife_gh/void_gh objects. Torg's `inn` code binding on #29103
therefore applies to the active Torg escape tunnel, while Solstice describes
a winter stairway there. Followed history finds no explicit predecessor,
successor, or retirement record. The September 2026 ZON/SHP additions for
Heavens objects #491–#494 are recent maintenance, not replacement evidence.
Retain all five files separately. See the [solstice_gh audit](SOLSTICE_GH_AUDIT_REVIEW.md),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-532-solstice_gh-retain-map-shop-and-imports),
[candidate dependency row 94](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L95),
and [coverage row 445](ZONE_COVERAGE_REGISTER.csv#L446).

### Checkpoint 533: sunhall Underworld3 and Bastard source identities

All ten Sunhall WLD VNUMs #30200–#30209 have different active records in
Underworld3; Sunhall #30200–#30204 also has distinct unlisted counterparts
in `bastard.wld`. Sunhall MOB #30200–#30202 differs from Bastard's same-number
guards/Ilixctthu, and OBJ #30200 differs from Bastard's badge. None of these
mobile/object prototypes is active in Underworld3. Both unlisted SHP files
use key and keeper #30201 but name different candidate mobiles and rooms;
shop stock also differs. Active numeric ownership, separate header #302,
and a September 2026 bag-stock edit are not replacement proof. Followed
history finds no explicit predecessor, successor, or retirement record.
Retain both five-file sources separately. See the [sunhall audit](SUNHALL_AUDIT_REVIEW.md),
[related Bastard audit](BASTARD_AUDIT_REVIEW.md#typed-resets-shop-stock-and-overlaps),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-533-sunhall-retain-map-shops-and-imports),
[candidate dependency row 95](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L96),
and [coverage row 446](ZONE_COVERAGE_REGISTER.csv#L447).

### Checkpoint 534: Surf, surface2011, and underdark2011 map identities

Surf room IDs overlap 89,999 surface2011 rooms and 15,481 underdark2011
rooms; a full-record scan found no exact WLD room block in either source.
The 322 Surf mobile IDs overlap 213 surface2011 and 68 underdark2011
prototypes, with no exact full MOB records. All three are unlisted as
full-area contributors. The source files have different map and prototype
records; followed history has no explicit predecessor, successor, or
retirement decision. Preserve all three families as separate identities.

Surf also has 145 active inbound exit records, 134 ambiguous with
surface2011 and 11 ambiguous with both variants. Do not assign those target
numbers to Surf without map-owner confirmation. See the [Surf audit](SURF_AUDIT_REVIEW.md),
[Surface2011 coverage row 449](ZONE_COVERAGE_REGISTER.csv#L449),
[Surf dependency row 96](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97),
and [Surf coverage row 447](ZONE_COVERAGE_REGISTER.csv#L448).

### Checkpoint 535: surface2011 and Surf map and story identities

The 90,000-room surface2011 map overlaps 89,999 Surf rooms and 15,482
UnderDark2011 rooms, with no identical full WLD room blocks. All 213 of its
MOB VNUMs overlap Surf and 68 overlap UnderDark2011, again with no identical
full records; eight shared OBJ IDs with UnderDark2011 also differ. “Norabi
Map” is the title of both surface2011 and underdark2011.

Source history adds important context: areas/AREA listed surface2011 in May
2011, then removed the entry in a November 2013 merge-conflict resolution
whose commit gives no retirement or replacement reason. Sa'Dah’s 2011 QST
text substantially overlaps active Surface #500000, but item contracts and
map records differ. Keep historical manifest evidence, story continuity, and
map identity distinct; no whole-zone replacement is established. See the
[surface2011 audit](SURFACE2011_AUDIT_REVIEW.md),
[active Surface audit](SURFACE_AUDIT_REVIEW.md),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-535-surface2011-retain-map-and-qst),
[candidate dependency row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L98),
and [coverage row 448](ZONE_COVERAGE_REGISTER.csv#L449).

### Checkpoint 536: surface2011-temp and Surface room identities

surface2011-temp.wld has 90,000 room records. Its 60,004 VNUMs shared with
active Surface all have different full room records and room titles. Its
602 source-only boundary tuples also appear in newbiemaps.wld; the source
room titles differ. The two WLDs overlap 89,847 room IDs, with one exact
full block at #666059 “The Ocean.” This narrow match does not establish a
whole-map successor or predecessor. Preserve the temp and newbiemaps source
identities and their repeated route records separately. See the
[surface2011-temp audit](SURFACE2011_TEMP_AUDIT_REVIEW.md),
[Surface audit](SURFACE_AUDIT_REVIEW.md#inactive-wld-surface-approaches),
[candidate dependency row 98](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L99),
and [coverage row 449](ZONE_COVERAGE_REGISTER.csv#L450).

### Checkpoint 537: surfacekeeps_objects duplicate prototypes and stub history

The unlisted surfacekeeps_objects.obj repeats all 54 object VNUMs #120000–#120053
in active surfacekeeps.obj. Thirty-nine full bodies are line-identical and
fifteen differ, including the candidate ore deposit versus active Rringlor
no-show procedure prototype at #120048 and candidate pink versus active blue
glasses at #120016. Followed history says the file was added for stub loading
in January 2010. Preserve this distinct object source while its current build
use and differing bodies are reconciled. See the
[surfacekeeps_objects audit](SURFACEKEEPS_OBJECTS_AUDIT_REVIEW.md),
[SurfaceKeeps audit](SURFACEKEEPS_AUDIT_REVIEW.md),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-537-surfacekeeps_objects-retain-object-stub),
[candidate dependency row 99](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L100),
and [coverage row 450](ZONE_COVERAGE_REGISTER.csv#L451).

### Checkpoint 538: Sylvandawn and Temple of Laduguer share VNUMs, not rooms

No current active `.zon` title normalizes to Sylvandawn. The unlisted
Sylvandawn WLD shares all 103 room numbers #8100–#8202 with active Temple of
Laduguer, but none of the 103 full room records is identical and every paired
title differs. Candidate MOB and OBJ scans found no same-type VNUM collision
with a current active prototype. Temple is not established as a replacement
for the broader Sylvandawn source: it has a distinct room identity, while the
old city retains its own reset, shop, and code references. Followed source
history and the reviewed active-list changes do not establish a
Sylvandawn-specific retirement or replacement decision. Preserve the two
identities separately; see the [Sylvandawn audit](SYLVANDAWN_AUDIT_REVIEW.md),
[Temple audit](TEMPLE_LADUGUER_AUDIT_REVIEW.md#inactive-sylvandawn-approaches-and-room-identity),
[candidate dependency row 100](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L101),
and [coverage row 451](ZONE_COVERAGE_REGISTER.csv#L452).

### Checkpoint 540: tharnadia1 records relate to remapped shops, not an archive decision

The 18-record `tharnadia1.shp` has no matching map or area-list member. Sixteen
shops (#6000–#6117) have related records in active `tharnadia.shp` with
shop/keeper/local-stock VNUM remapping to #132500–#132617; fourteen stock
lists match after remapping, while active #132585 and #132617 contain later
stock additions. The other two records do not establish a full-catalog
replacement. Twelve old shop IDs are also present in `tharold.shp` and two in
`twinkeep.shp`; those comparisons share keeper and room fields but no complete
record is identical. Followed histories show the candidate's 2010 addition
and 2013 edit and later active Tharnadia shop changes, but no retirement or
whole-source replacement instruction. Keep `tharnadia1`, `tharold`, and
`twinkeep` identities separate until their owners confirm the source and
loader relationship. See the [tharnadia1 audit](THARNADIA1_AUDIT_REVIEW.md),
[active Tharnadia source](../../../areas/shp/tharnadia.shp#L356),
[candidate dependency row 102](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L103),
and [coverage row 453](ZONE_COVERAGE_REGISTER.csv#L454).

### Checkpoint 541: tharold and tharnadia1 keep separate shop identities

All twelve `tharold.shp` shop numbers (#6000–#6086) also occur in
`tharnadia1.shp` with the same keeper and room fields. Every full record
differs, although eight product lists match. All twelve are non-roaming,
which makes their shared keeper/room pairs relevant to fallback shop lookup.
The twelve records also have related active Tharnadia shops in a remapped
#132xxx band; eight product lists match after the local VNUM remap and four
have differing stock. The followed `tharold` history reaches the 2009 path
move and 2013 edit but does not establish a retirement, rename, or archive
decision. Keep the three source identities distinct pending owner review.
See the [tharold audit](THAROLD_AUDIT_REVIEW.md),
[tharnadia1 audit](THARNADIA1_AUDIT_REVIEW.md),
[active Tharnadia shops](../../../areas/shp/tharnadia.shp#L1),
[candidate dependency row 103](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L104),
and [coverage row 454](ZONE_COVERAGE_REGISTER.csv#L455).

### Checkpoint 539: temple2 and Longhollow share VNUM bands, not source records

Inactive `temple2` and active `long` both claim zone header #344 and the same
room-number band #34400–#34488. All 89 paired room records and titles differ.
The candidate also shares every one of its 30 mobile and 31 object VNUMs with
Longhollow, with no exact full-body matches in either type. Its source-authored
`Home of the Enlightened` title and forest rescue-and-letter story remain
separate from active `Defense of Longhollow`. Source history reaches the
2009 path move and later edits but establishes no retirement or replacement
decision. Preserve both identities and do not co-load, merge, renumber, or
infer replacement from the matching header or VNUMs. See the
[temple2 audit](TEMPLE2_AUDIT_REVIEW.md),
[Longhollow audit](LONGHOLLOW_AUDIT_REVIEW.md#typed-dependencies-and-source-identity),
[candidate dependency row 101](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L102),
and [coverage row 452](ZONE_COVERAGE_REGISTER.csv#L453).

### Checkpoint 542: theurgist notes keep a separate low-VNUM identity

The four-file source was added in 2010 under a commit titled “This is not a
live zone, it is theurgist notes.” No current active ZON title matches its
header title. Candidate room #1/#3/#4, MOB #1–#14, and OBJ #1–#3 numbers also
occur in active Limbo/Heavens records, but all compared same-type source bodies
differ. The matching map JSON does not prove a live-zone alias or replacement.
Preserve the class notes and map identity; do not merge, co-load, or infer
retirement from list exclusion. See the [theurgist audit](THEURGIST_AUDIT_REVIEW.md),
[20 builder questions](THEURGIST_BUILDER_QUESTIONS.md),
[candidate dependency row 104](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L105),
and [coverage row 455](ZONE_COVERAGE_REGISTER.csv#L456).

### Checkpoint 543: Azhural replacement and mixed legacy record identity

Commit `50b400005` explicitly comments `tiamats_lair` out of `areas/AREA` and
adds Azhural as the new world version. The current files retain mixed exactness:
41 of 83 same-number WLD room bodies and five of 20 MOB bodies match, while all
38 OBJ bodies match the active Azhural records. The QST file is byte-identical
to `azhural.qst` but `qst/AREA` still selects the `tiamats_lair` path. Preserve
the two source identities and keep the old QST path until its manifest owner
reconciles it. See the [Tiamats Lair audit](TIAMATS_LAIR_AUDIT_REVIEW.md),
[Azhural audit](AZHURAL_AUDIT_REVIEW.md),
[candidate dependency row 105](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L106),
and [coverage row 456](ZONE_COVERAGE_REGISTER.csv#L457).

### Checkpoint 544: Fehlnan and Torrhan share story evidence without a tracked replacement

The unlisted `torr` title is City of Fehlnan; the selected `torrhan` title is
City of Torrhan. Their same-type WLD, MOB, and OBJ VNUMs are disjoint from
each other and from every other corresponding source file. However, the old
market description and its QST share the active map's King Torrhan, decline,
dragon-scale, hidden-palace, Rhithel, and Zrilxa story material. The available
tracked source history begins at the 2009 directory move and does not record
an explicit `torr`→`torrhan` replacement. Keep the identity question open;
the narrative similarity alone does not establish a rename or retirement.
See the [Torr audit](TORR_AUDIT_REVIEW.md),
[City of Torrhan dossier question](../zone-stories/CITY_OF_TORRHAN.md#explicit-builder-questions-and-recommended-resolutions),
[20 builder questions](TORR_BUILDER_QUESTIONS.md),
[candidate dependency row 106](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L107),
and [coverage row 457](ZONE_COVERAGE_REGISTER.csv#L458).

### Checkpoint 545: Tradeskills old is a separate catalog copy with active VNUM collisions

`tradeskills.old.obj` is an object-only source added on 2013-11-18 while the
active `tradeskills.obj` was also edited. Followed history traces the old copy
to the active stem but does not document a replacement decision. At the current
snapshot, all 234 old VNUMs also exist in the 293-object active file: 218 whole
records match, 16 differ, and 59 active definitions are absent from the old
file. The old #400232 elven bag conflicts with the current #400232 turkey
innards binding. Keep both file identities separate and retain the old source
in place; do not merge it into the active catalog. See the
[old-catalog audit](TRADESKILLS_OLD_AUDIT_REVIEW.md),
[20 builder questions](TRADESKILLS_OLD_BUILDER_QUESTIONS.md),
[active Tradeskills review](TRADESKILLS_AUDIT_REVIEW.md),
[candidate dependency row 107](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L108),
and [coverage row 458](ZONE_COVERAGE_REGISTER.csv#L459).

### Checkpoint 546: Twinkeep and tharnadia1 share shop keys but differ

`twinkeep.shp` contains only shop keys #6500 and #6519. Both keys also occur
in `tharnadia1.shp` with matching keeper and `in_room` fields, but neither
full record is identical: Twinkeep #6519 omits `tharnadia1` product #3050,
and the records differ in prices, messages, and flags. Active Tharnadia
#132650 is related by the earlier remapped source family but has a different
keeper, room, and stock; the tracked histories provide no replacement
decision for Twinkeep. Keep the source files and shop identities separate.
See the [Twinkeep audit](TWINKEEP_AUDIT_REVIEW.md),
[20 builder questions](TWINKEEP_BUILDER_QUESTIONS.md),
[Tharnadia1 audit](THARNADIA1_AUDIT_REVIEW.md),
[candidate dependency row 108](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L109),
and [coverage row 459](ZONE_COVERAGE_REGISTER.csv#L460).

### Checkpoint 547: ud has a unique title and distinct WLD identities

After color stripping and whitespace normalization, “Underworld of Duris” is
unique among all 442 current ZON titles. The unregistered `ud.wld` defines
17,462 room VNUMs with no exact overlap in any other current WLD source.
Followed same-stem WLD/ZON history reaches the August 12, 2009 area-tree move
and shows no later tracked-file edits or replacement statement. Similar
underworld/underdark stems do not identify a predecessor or successor. Keep
`ud` as a separate retained map candidate; list exclusion is not replacement
evidence. See the [ud audit](UD_AUDIT_REVIEW.md#title-history-and-disposition),
[20 builder questions](UD_BUILDER_QUESTIONS.md),
[candidate dependency row 109](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L110),
and [coverage row 460](ZONE_COVERAGE_REGISTER.csv#L461).

### Checkpoint 548: ud-inside retains a Limbo-numbered placeholder

The exact normalized ZON title “The Horrible Tracts of the Unnamed” occurs in
five current files: `astral`, `eoo_op2`, `mtncastl`, `theurgist`, and
`ud-inside`. Each is outside the active full-area list. The single blank
`ud-inside.wld` room #1 block is byte-identical to the placeholders in
`astral.wld` and `mtncastl.wld`, while active Limbo’s room #1 body differs.
Twelve WLD files define #1 and Limbo alone is active; code reserves #1 as
`ROOM_LIMBO_VNUM`. Shared title and bytes do not establish a common owner or
replacement. Followed `ud-inside` paths show WLD content in 2011, older
MOB/OBJ history, ZON history back to the 2009 area-tree move, and no explicit
replacement decision. Keep the stub outside the manifests and retain it in
place. See the [ud-inside audit](UD_INSIDE_AUDIT_REVIEW.md),
[20 builder questions](UD_INSIDE_BUILDER_QUESTIONS.md),
[candidate dependency row 110](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L111),
and [coverage row 461](ZONE_COVERAGE_REGISTER.csv#L462).

### Checkpoint 549: ud_map0 is distinct from ud_map1 and ud_map2

The normalized title “deep below the surface” is unique among current ZON
titles; ud_map1 and ud_map2 also have distinct titles. The three maps’ room
sets are pairwise disjoint, and no ud_map0 room number appears in another
current WLD source. Followed ud_map0 WLD/ZON history reaches the 2009 area-tree
move and later 2013 edits without an explicit predecessor, successor, or
replacement decision. The six source-only exits to active maps and 26
source-only mobile resets establish dependencies, not map replacement or
active deployment. Keep ud_map0, ud_map1, and ud_map2 as separate retained
source candidates. See the
[ud_map0 audit](UD_MAP0_AUDIT_REVIEW.md),
[20 builder questions](UD_MAP0_BUILDER_QUESTIONS.md),
[candidate dependency row 111](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L112),
and [coverage row 462](ZONE_COVERAGE_REGISTER.csv#L463).

### Checkpoint 550: ud_map1 title and room identity

The normalized title “the dreadful underdark” is unique among 442 current ZON
titles. `ud_map0`, `ud_map1`, and `ud_map2` have distinct normalized titles,
and their room-number sets are pairwise disjoint; no `ud_map1` room VNUM
appears in another current WLD source. Followed `ud_map1` WLD/ZON history
reaches the 2009 area-tree move and a 2013 same-stem update, but identifies no
explicit predecessor, successor, or replacement. Similar Underworld/Underdark
names, source-only routes, and shared reset prototypes do not establish map
replacement. Keep all three source identities separate and retain `ud_map1` in
place. See the [ud_map1 audit](UD_MAP1_AUDIT_REVIEW.md), [20 builder
questions](UD_MAP1_BUILDER_QUESTIONS.md), [candidate dependency row
112](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L113), and [coverage row
463](ZONE_COVERAGE_REGISTER.csv#L464).

### Checkpoint 551: ud_map2 title and room identity

The normalized ZON title “the caverns of the underdark” is unique among 442 current ZON titles. ud_map0, ud_map1, and ud_map2 have distinct titles and pairwise-disjoint room-number sets; no ud_map2 room VNUM appears in another current WLD source. Followed WLD/ZON history reaches the 2009 area-tree move and 2013 same-stem update; empty MOB/OBJ history additionally reaches 2005/2006. No explicit predecessor, successor, alternate-stem rename, or replacement was found. Similar Underworld/Underdark names, shared mobile prototypes, and source-only routes do not prove replacement. Retain the three source identities separately. See the [ud_map2 audit](UD_MAP2_AUDIT_REVIEW.md), [builder questions](UD_MAP2_BUILDER_QUESTIONS.md), [candidate dependency row 113](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114), and [coverage row 464](ZONE_COVERAGE_REGISTER.csv#L465).


### Checkpoint 552: underdark2011 map and prototype identity

underdark2011 is not a unique identity for its VNUM range. Its 15,482 room numbers all occur in surface2011 and 15,481 also occur in surf, but no full WLD block matches across the maps. All 68 mobile numbers overlap both sources with different records; all eight object IDs overlap surface2011 with different object records. The normalized title Norabi Map is shared by surface2011 and underdark2011. Their tracked map-editor and manifest histories do not identify a predecessor, successor, retirement, or replacement. Keep all three source families separate from active underdark; preserve underdark2011 RETAIN IN PLACE. See the [audit supplement](UNDERDARK2011_AUDIT_REVIEW.md#duplicate-and-replacement-evidence), [30 builder questions](UNDERDARK2011_BUILDER_QUESTIONS.md), [checkpoint 534](#checkpoint-534-surf-surface2011-and-underdark2011-map-identities), and [coverage row 465](ZONE_COVERAGE_REGISTER.csv#L466).

### Checkpoint 553: underworld2 retains a distinct map identity

The normalized ZON title “Underdark of Duris” occurs once across current
titles, and all 8,256 underworld2 room VNUMs are disjoint from other current
WLD sources. Followed WLD/ZON history reaches the 2009 area-tree relocation;
the checked manifest histories contain no underworld2 entry. No tracked
predecessor, successor, retirement, or replacement decision was found. Similar
Underworld/Underdark names, source-only exits, and list exclusion do not prove
supersession. Keep underworld2 distinct and **RETAIN IN PLACE**. See the
[underworld2 audit](UNDERWORLD2_AUDIT_REVIEW.md#title-history-and-disposition),
[30 builder questions](UNDERWORLD2_BUILDER_QUESTIONS.md),
[coverage row 466](ZONE_COVERAGE_REGISTER.csv#L467), and
[candidate dependency row 115](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L116).

### Checkpoint 554: Venom GH and two guildhalls share numbers, not identity

The normalized title “Venom GH” occurs once among current ZON sources, but
all ten Venom room VNUMs #30300–#30309 also occur in `bb_gh.wld` and
`vodgh.wld`. No full room blocks match across those sources. Venom mobile
VNUMs #30300–#30302 also overlap both files’ MOB records, with no identical
full records. The same #303 zone header and numeric rooms/mobiles do not prove
that any one map is a predecessor or replacement. Followed same-stem history
reaches the 2009 tree move and later same-path edits; no manifest history or
explicit retirement/replacement decision was found. Keep Venom, BB GH, and
VoDs GH as distinct **RETAIN IN PLACE** sources. See the
[Venom audit](VENOM_AUDIT_REVIEW.md#room-and-prototype-identity),
[30 builder questions](VENOM_BUILDER_QUESTIONS.md),
[coverage row 467](ZONE_COVERAGE_REGISTER.csv#L468), and
[candidate dependency row 116](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L117).

### Checkpoint 555: vodgh shares room and prototype numbers with two guildhalls

The normalized title “VoDsGH” occurs once among 442 current ZON titles. All
14 `vodgh.wld` room VNUMs #30300–#30313 occur in `bb_gh.wld`, and the first
ten also occur in `venom.wld`; no full room blocks match across the sources.
`vodgh.mob` #30300–#30302 occur in both other MOB files, with no identical
full mobile records. `vodgh.obj` #30300/#30301 also overlap `bb_gh.obj`, with
no identical full object records. The three ZON headers all use #303. These
collisions establish competing numeric identities, not a predecessor or
replacement.

Followed `vodgh` WLD/MOB/OBJ/ZON/SHP history reaches the 2009 tree move and
same-path updates in 2011–2013. No tracked alternate-stem rename, explicit
predecessor, successor, retirement, or whole-zone replacement was found. Keep
VoDsGH, BB GH, and Venom as separate **RETAIN IN PLACE** source identities.
See the [VoDsGH audit](VODGH_AUDIT_REVIEW.md#room-and-prototype-identity),
[30 builder questions](VODGH_BUILDER_QUESTIONS.md),
[coverage row 468](ZONE_COVERAGE_REGISTER.csv#L469), and
[candidate dependency row 117](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L118).

### Checkpoint 556: Void Guildhall title history and shared identities

The normalized ZON title “Main Guildhall for The Void” occurs once among 442
current titles, but zone header #295 is shared with `genocide_hall` and
`legion`. All nineteen `void_gh.wld` room VNUMs #29100–#29118 also occur in
active `torg.wld` with different full room blocks. The first ten additionally
occur in `afterlife_gh`, `scrubs_hall`, `solstice_gh`, and `wlust_gh`; #29110–
#29112 occur in `afterlife_op` with different records. Void mobile IDs
#29100–#29104 overlap `afterlife_gh`, #29100–#29103 overlap `scrubs_hall`,
#29100–#29102 overlap `solstice_gh`, and #29100–#29101 overlap `wlust_gh`;
all paired full MOB blocks differ. Void objects #29100–#29102 overlap
`afterlife_gh`, and #29100–#29105 overlap `solstice_gh`; paired full OBJ
blocks differ. Active Torg has no mobile/object prototype in those Void
same-number bands. Compiled code binds `inn` to raw #29103 and Torg invasion
logic to raw #29116; both resolve to Torg rooms, while Void uses #29103 as a
shop and #29116 as a stairway.

Followed source paths reach the 2009 area-tree move; WLD/ZON/SHP also appear
in a 2013 commit with no text-line changes. September 2026 ZON/SHP edits add
four active Heavens shop-stock object IDs, demonstrating recent data
maintenance but not manifest membership. No tracked alternate-stem rename,
predecessor, successor, retirement, or whole-zone replacement was found.
These shared numbers and similar guildhall labels establish competing source
identities, not a predecessor/replacement relation. Keep Void, Torg, and each
unregistered guildhall source separate. See the [Void Guildhall audit](VOID_GH_AUDIT_REVIEW.md),
[36 builder questions](VOID_GH_BUILDER_QUESTIONS.md), [coverage row 469](ZONE_COVERAGE_REGISTER.csv#L470),
and [candidate dependency row 118](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L119).

### Checkpoint 557: Water Plane low-number identities and followed history

After color-code removal and whitespace normalization, “Water Plane” is unique
among the 442 current `.zon` titles. Its ZON header #1 is shared by 29 other
zone files and does not identify a contributor. Six Waterplane room numbers
overlap active WLD owners: #1 is Limbo; #3, #4, #5, #10, and #12 are Heavens.
Each paired full WLD block differs. The other Waterplane room numbers also
collide with multiple unregistered WLD sources: seven other files share #2,
#3, or #4; six share #5, #6–#10, or #12; and four share #72. No full room
block matches the Waterplane record at the same VNUM. Waterplane mobile IDs
#1–#3 collide with active Limbo and six other unregistered MOB files, with no
identical full mobile record.

The active `plane_water_one` area is titled “Plane of Water,” owns
#23201–#23325, and has a separate followed history and typed dependency set.
The all-95 inactive-WLD review found no Waterplane overlap or approach into
that range. The similar title and prior low-VNUM map do not establish a rename
or replacement.

Followed WLD, MOB, OBJ, and ZON logs remain on the `waterplane` stem. WLD
edits appear in 2011 and 2013, MOB edits in 2012 and 2013, the ZON has a 2015
quest-item-flag edit, and the zero-byte OBJ is present from the 2005 initial
revision. All paths appear in the 2009 repository tree move. These are
same-stem maintenance edits; no alternate-stem predecessor, successor,
retirement, or whole-zone replacement is established. Retain `waterplane`
separately from `plane_water_one` and the other low-number sources. See the
[Waterplane audit](WATERPLANE_AUDIT_REVIEW.md),
[24 builder questions](WATERPLANE_BUILDER_QUESTIONS.md),
[coverage row 470](ZONE_COVERAGE_REGISTER.csv#L471), and
[candidate dependency row 119](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L120).

### Checkpoint 558: Wanderlust Guild Hall title, room IDs, and source history

The normalized ZON title “Wanderlust Guild Hall” is unique among 442 current
titles. Header #291 is shared only with `scrubs_hall`, whose title is
“Ye' Old ScRubS Pirate Galleon.” All ten Wlust room IDs #29100–#29109 also
occur in active Torg and in unregistered `afterlife_gh`, `scrubs_hall`,
`solstice_gh`, and `void_gh`; Wlust's same-number room blocks differ from
Torg's. Shared guildhall room numbers and names do not establish a
predecessor or whole-zone replacement.

Wlust mobiles #29100/#29101 (Archangel/Titan) also have distinct same-number
prototypes in those four unregistered MOB sources; no active Torg MOB uses
either number. Its OBJ file is tracked but empty. No reviewed title, manifest,
or followed path history establishes active `plane_water_one` or the
`wander` source stem as an alias or predecessor of Wlust.

Followed history records WLD spelling edits in 2011 and 2013, with the 2013
commit changing “ceilings” back to “cielings” and changing file mode. MOB
and the empty OBJ show the 2009 area-tree move; ZON/SHP show a 2013
mode-only change. September 6, 2026 `ed94862f0` adds Heavens gathering-bag
VNUMs #491–#494 to Wlust's ZON and SHP. This is same-stem source maintenance,
not evidence of manifest membership, retirement, or replacement. No
explicit predecessor, successor, retirement, or whole-zone replacement was
found. Keep Wlust and the other #29100–#29109 sources distinct.

See the [Wlust audit](WLUST_GH_AUDIT_REVIEW.md), [28 builder
questions](WLUST_GH_BUILDER_QUESTIONS.md),
[coverage row 471](ZONE_COVERAGE_REGISTER.csv#L472), and
[candidate dependency row 120](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L121).

### Checkpoint 559: Yggdrasil title history and Ard’gral collisions

After color-code removal and whitespace normalization, “Yggdrasil Guild
Hall” is shared by exactly two of 442 current ZON titles: ygg and circlegh.
Both ZON headers are #9, but their room ranges differ (#30400–#30412 versus
the #45400 band). Yggdrasil’s thirteen room VNUMs also fall outside the
conventional #900–#999 band for header #9. The separate rf_gh source is in
the #28900 band. The exact title and header matches do not establish a rename,
canonical alias, range correction, or replacement. See the
[Yggdrasil audit](YGG_AUDIT_REVIEW.md), [28 builder
questions](YGG_BUILDER_QUESTIONS.md), and
[coverage row 472](ZONE_COVERAGE_REGISTER.csv#L473).

Yggdrasil rooms #30401–#30412 overlap active Ard’gral rooms with different
full WLD records. Yggdrasil mobile #30401 differs from Ard’gral’s nightmare
essence, and Yggdrasil object #30401 differs from Ard’gral’s mask of
nightmares. Yggdrasil room/mobile/object #30400 has no second active owner;
the other Yggdrasil-only mobile IDs #30402 and #30410–#30412 are not active
Ard’gral mobiles. These shared numbers are identity conflicts, not evidence
of succession.

Followed Yggdrasil paths remain on the ygg stem. The 2009 change is the
repository tree move. The 2012 and 2013 MOB/OBJ/ZON edits update field
layout/read-write data; the 2013 WLD and SHP changes are file-mode-only.
No alternate-stem rename, distinct predecessor, or whole-zone replacement
was established. Keep ygg distinct from circlegh and rf_gh until an owner
provides an identity crosswalk.

### Tempest Court (`airp`)

After color-code removal and case/space normalization, **The Tempest Court**
is unique among all 442 current `.zon` titles. The current full-area records
remain under `airp`; `plane_air_one` is the separately titled Plane of Air,
joined by a reciprocal WLD boundary rather than a rename. Followed WLD, MOB,
OBJ, ZON, QST, and story-sidecar paths show no alternate-stem rename, tracked
predecessor, or whole-zone replacement. Old commit-message references to
“AP Tempest Courts” do not establish a second active identity. Keep `airp`
active and **DO NOT MOVE**. The owner question about any additional
command-target alias is recorded as TEMP-01 in the [Tempest Court audit
review](TEMPEST_COURT_AUDIT_REVIEW.md). See the [source dossier](../zone-stories/TEMPEST_COURT.md),
[coverage row 2](ZONE_COVERAGE_REGISTER.csv#L3), and the reciprocal
[world-connectivity review](WORLD_CONNECTIVITY.md#checkpoint-565-tempest-court-graph-and-air-boundary).

### Mountain Valley of Dawndale (`airshipgrave`)

After color-code removal and case/space normalization, **The Mountain Valley
of Dawndale** is unique among all 442 current `.zon` titles. The parenthetical
`(Alachest)` in the AREA/QST area note is not the ZON title. Followed WLD, MOB,
OBJ, ZON, QST, and story-sidecar histories remain under the `airshipgrave`
stem; no tracked alternate-stem rename, same-title active duplicate, distinct
predecessor, or whole-zone replacement was established. Keep the registered
active area **DO NOT MOVE**. Whether `Alachest` is also a player-facing alias is
an open builder question, not an inferred identity crosswalk. See the
[Dawndale audit review](AIRSHIPGRAVE_AUDIT_REVIEW.md), [source dossier](../zone-stories/THE_MOUNTAIN_VALLEY_OF_DAWNDALE.md),
[coverage row 3](ZONE_COVERAGE_REGISTER.csv#L4), and [Surface boundary review](WORLD_CONNECTIVITY.md#checkpoint-566-dawndale-graph-and-surface-connections).

### Alatorin - the Forge City (`alatorin`)

After color-code removal and case/space normalization, **Alatorin - the Forge
City** is unique among all 442 current `.zon` titles. Followed current native
source paths remain under `alatorin`; no tracked alternate-stem rename,
distinct predecessor, or whole-zone replacement was established. The
unregistered `alatorin_objects.obj` is an object-only component with 408
records that overlap active Alatorin prototypes; 168 also overlap distinct
Nexus56 prototypes. Those saved-object conflicts do not show that either
complete area succeeded the other. Keep active `alatorin` **DO NOT MOVE** and
preserve the separate object-bank candidate row 5 **RETAIN IN PLACE** pending
identity/tooling closure. See the [Alatorin audit review](ALATORIN_AUDIT_REVIEW.md),
[object-bank review](ALATORIN_OBJECTS_AUDIT_REVIEW.md), [coverage row 5](ZONE_COVERAGE_REGISTER.csv#L6),
and [candidate row 5](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L6).

### Checkpoint 702: Ashrumite Village and Siege Quartermaster history

The normalized title scan finds “Ashrumite Village” unique among current
`.zon` titles. The full-area and quest-area indexes still list `ashrumite`, and
the source history keeps the active town under that stem. The distinct
[Ashrumite dossier](../zone-stories/ASHRUMITE_VILLAGE.md) covers its current
story and exchanges.

There is a bounded service move in the history. Commit `06d31630e` (2014-04-10)
removed Ashrumite mobile #66054 and changed the town reset to use Siege mobile
#401020. Commit `7ddf5432f` (2026-08-31) retired Siege and removed the stale
Ashrumite reset along with the other Siege quartermaster callers. The
[Siege audit](SIEGE_AUDIT_REVIEW.md) records its historical source and
callers. This is a quartermaster service migration and retirement, not an
Ashrumite area rename, duplicate, or replacement. Current references to
Ashrumite people or goods in other areas do not declare competing area
ownership. Keep the active Ashrumite identity and its separate source records;
see [coverage row 14](ZONE_COVERAGE_REGISTER.csv#L15), the
[full-area entry](../../../areas/AREA#L248), the
[quest-area entry](../../../areas/qst/AREA#L222), and the
[current title](../../../areas/zon/ashrumite.zon#L2).

### Checkpoint 703: Barovia and Barovia Continued

The current titles are distinct: “The Realm of Barovia” and “The Realm of
Barovia Continued.” The full-area index retains `barovia2 *588-589` and
`barovia *910-911`; the quest-area index separately retains `barovia2 *588`
and `barovia *910-911`. Their WLD, MOB, OBJ, ZON, and QST sources use separate
stems and disjoint VNUM ranges. See the full-area entries for
[`barovia2`](../../../areas/AREA#L241) and [`barovia`](../../../areas/AREA#L324),
the quest-area entries for [`barovia2`](../../../areas/qst/AREA#L218) and
[`barovia`](../../../areas/qst/AREA#L288), the [Barovia title](../../../areas/zon/barovia.zon#L2),
and the [Continued title](../../../areas/zon/barovia2.zon#L2).

Followed histories for both native stems reach the 2009 area-tree move and
continue under their respective paths. Commit `fd4345656` (2013-03-14) edits
the WLD, MOB, OBJ, ZON, and QST paths for both stems; its message refers to
“barovia1-2” and does not describe either as replacing the other. Later
tracked changes remain on the corresponding stem paths. This bounded review
found no alternate-stem rename, retirement, or whole-zone replacement.
The dossiers document cross-area keys, contacts, and item dependencies while
keeping the two contract owners separate; those references do not establish
zone replacement. The direct alias questions remain with the builders in the
[Barovia dossier](../zone-stories/THE_REALM_OF_BAROVIA.md) and [Continued
dossier](../zone-stories/THE_REALM_OF_BAROVIA_CONTINUED.md). See [coverage row
24](ZONE_COVERAGE_REGISTER.csv#L25) and [coverage row 25](ZONE_COVERAGE_REGISTER.csv#L26).

### Checkpoint 704: Basin Wastes title and tracked history

A fresh normalization of all 442 current `.zon` titles finds one owner of
“The Basin Wastes”: `basin_wa`. Both active indexes retain that stem at zone
340–341. The separate source-only `oldmap8.wld` contains repeated map cells
titled “Basin Wastes,” including room #182142. Its review treats these as
regional map labels and retains `oldmap8` as an unregistered map source, not
as another active `.zon` area or a replacement. See the [full-area
index](../../../areas/AREA#L164), [quest-area index](../../../areas/qst/AREA#L152),
[Basin title](../../../areas/zon/basin_wa.zon#L2), [oldmap8 map cell](../../../areas/wld/oldmap8.wld#L44960),
[oldmap8 review](OLDMAP8_AUDIT_REVIEW.md), and [candidate row 80](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L81).

Followed WLD, MOB, OBJ, ZON, and QST histories reach the 2009 area-tree move
and remain under the `basin_wa` stem. Commit `71d731c5b` (2013-11-13) left the
manifest membership unchanged; its WLD change is file-mode metadata, while
the ZON reset comment and QST prose receive spelling edits. Later commits
continue maintaining these same paths. The `basin_wa` story sidecar was added
on 2026-10-04 as a source-story map. This bounded history found no alternate-
stem rename, explicit predecessor, retirement, or whole-zone replacement.
See the [Basin audit supplement](BASIN_WASTES_AUDIT_REVIEW.md) and [coverage
row 26](ZONE_COVERAGE_REGISTER.csv#L27); its command-alias question remains
open for the area owner.

### Checkpoint 705: Bastine Castle title and tracked history

A fresh normalization of all 442 current `.zon` titles finds one owner for
“Bastine Castle”: `bastine`. Both active indexes retain that key at zone
76–77. The source-only `oldmap8.wld` has a regional cell #188272 with the same
title; its 10,000-room grid is independently reviewed and retained as a map
source, not a competing active `.zon` area. See the [full-area
index](../../../areas/AREA#L43), [quest-area index](../../../areas/qst/AREA#L39),
[Bastine title](../../../areas/zon/bastine.zon#L2), [oldmap8 map cell](../../../areas/wld/oldmap8.wld#L173395),
[oldmap8 review](OLDMAP8_AUDIT_REVIEW.md), and [candidate row 80](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L81).

Followed WLD, MOB, OBJ, ZON, and QST histories reach the 2009 area-tree move
and remain under the `bastine` stem. Later changes are same-path field,
quest-item-flag, typo, and mobile-warning maintenance. The structured story
sidecar first appears on 2026-10-02. This bounded review found no alternate-
stem rename, explicit predecessor, retirement, or whole-zone replacement.
The separate Bastine/Mir object references remain dependencies owned by their
respective active areas. See the [Bastine dossier](../zone-stories/BASTINE_CASTLE.md),
[Mir audit supplement](MIR_AUDIT_REVIEW.md), and [coverage row
27](ZONE_COVERAGE_REGISTER.csv#L28); the command-alias question remains open.

### Checkpoint 706: Black Pearl copy lineage and separate zone history

A fresh normalization of all 442 current `.zon` titles finds one exact owner
for “The Black Pearl”: `blackpearl`. The full-area index lists
`blackpearl *1422`; `qst/AREA` has no matching entry. The expected
`areas/world.qst` aggregate is absent, so the per-stem QST does not establish
that its contracts load. See the [full-area entry](../../../areas/AREA#L402),
[title source](../../../areas/zon/blackpearl.zon#L2), and [loader/aggregate
boundary](RAXQUEST_AUDIT_REVIEW.md#L9).

Commit `2e7d40d14` (2012-09-05) adds Black Pearl's WLD, OBJ, QST, and ZON
files; its message describes a “first phase of the new ship zone.” Git copy
detection marks the new OBJ as C091 from `raxquest.obj` to
`blackpearl.obj`. Same-day follow-up `807f8e341` adds the missed MOB file
and `areas/AREA` entry. The shared QST is separate file-level reuse: the
462-line Raxquest, FooQuest, and Black Pearl QST files match after their local
VNUM tokens are normalized, while 362 lines match without normalization.
Neither copied OBJ data nor shared QST text establishes that Black Pearl
replaced Raxquest or FooQuest as a whole area. Keep Black Pearl's active
full-area key distinct and resolve the OBJ/QST source and generation owners
before any copy removal, QST registration, or path decision.

The [Black Pearl dossier](../zone-stories/THE_BLACK_PEARL.md#direct-area-owner-questions-and-conservative-recommendations)
and [Raxquest builder questions](RAXQUEST_BUILDER_QUESTIONS.md) now ask for
the source lineage and loading decision. See the [Raxquest audit](RAXQUEST_AUDIT_REVIEW.md),
[Raxquest retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-523-raxquest-retain-source-files-pending-route-and-code-closure),
[candidate row 85](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L86),
[Black Pearl coverage row 31](ZONE_COVERAGE_REGISTER.csv#L32), and
[Raxquest coverage row 436](ZONE_COVERAGE_REGISTER.csv#L437).

### Checkpoint 707: Breale title and followed history

A fresh normalization of all 442 current `.zon` titles finds one exact owner
for “The Town of Breale”: `breale`. Both active indexes retain the same key
at zone #26. The name also appears in Bastine's map inscription and Highway
room prose as a neighboring town; these content references do not define a
second `.zon` identity. See the [full-area entry](../../../areas/AREA#L13),
[quest-area entry](../../../areas/qst/AREA#L13), [title source](../../../areas/zon/breale.zon#L2),
and [Bastine map inscription](../../../areas/wld/bastine.wld#L519).

Followed WLD, MOB, OBJ, ZON, QST, and SHP histories remain under the
`breale` stem from the 2009 area-tree move. Later commits maintain those same
paths; the 2014 shop correction does not record a new area identity. The
structured story sidecar first appears on 2026-10-02. This bounded history
found no alternate-stem rename, explicit predecessor, retirement, or
whole-zone replacement. See the [Breale dossier](../zone-stories/BREALE.md),
[Highway room reference](../../../areas/wld/highway.wld#L7348), and [coverage row
38](ZONE_COVERAGE_REGISTER.csv#L39); its alias question remains with the owner.

### Checkpoint 708: Bloodstone Keep title and followed history

A fresh scan of the second-line titles in all 442 current `.zon` files strips
Duris color tokens and finds one exact owner for “Bloodstone Keep”:
`areas/zon/bs.zon`. The full-area index retains `bs *740-749` from its first
tracked appearance in 2009; the current QST index lists the same stem and range
from its 2012 entry. See the [full-area index](../../../areas/AREA#L268),
[QST index](../../../areas/qst/AREA#L239), and [title source](../../../areas/zon/bs.zon#L2).

Followed WLD, MOB, OBJ, ZON, QST, and SHP histories continue under the `bs`
stem from the earliest tracked 2009 area-tree move through later maintenance.
The 2010–2026 edits include room and text corrections, object and reset updates,
and a prison-exit key; these stay on the existing paths. The structured story
sidecar first appears on 2026-10-02 as a source-mapping layer. This bounded
tracked history found no alternate-stem rename, explicit predecessor,
retirement, or whole-zone replacement. See the [Bloodstone dossier](../zone-stories/BLOODSTONE_KEEP.md)
and [coverage row 40](ZONE_COVERAGE_REGISTER.csv#L41).

### Checkpoint 709: Caertannad Tharnadia WLD content overlap

A fresh normalization of the second-line titles in all 442 current `.zon`
files finds one owner each for “The Twin Keeps of Devastated Tharnadia”
(`caertannad`) and “Tharnadia - City of Humans” (`tharnadia`). The active
indexes keep separate full-area ranges `*784-788` and `*1325-1330`, with
distinct current source stems. See the [full-area entries](../../../areas/AREA#L282)
and [title sources](../../../areas/zon/caertannad.zon#L2),
[Tharnadia entry](../../../areas/AREA#L382), and
[Tharnadia title](../../../areas/zon/tharnadia.zon#L2).

The separate titles conceal a substantial room-text overlap. After removing
Duris color codes and normalizing whitespace and case, current WLD room titles
and descriptions form 95 exact pairings; 68 of the 451 `caertannad` room
records have at least one exact counterpart in `tharnadia.wld`. For example,
“Inside the Gates of Caer Tannad” appears at #78423 and #132921, and the inner
keep walkway text appears at #78452 and #132965. See the [Caertannad rooms](../../../areas/wld/caertannad.wld#L481)
and [Tharnadia copies](../../../areas/wld/tharnadia.wld#L10196), with another
matched pair at [#78452](../../../areas/wld/caertannad.wld#L1305) and
[#132965](../../../areas/wld/tharnadia.wld#L11391).

The same 95 pairings appear when the pre-renumber Tharnadia WLD is compared;
#78423 matches its then-current room #6501. Commit `3a88fc6ce` renumbered the
same-stem Tharnadia data from zone header 60 to 1325 and added it to
`areas/AREA`; it did not remove or rename `caertannad`. Both identities
remain active now. The evidence establishes a copied or parallel subarea-text
relationship, while the intended relationship between the two room sets and
their route/reset roles remains unresolved. It does not establish that either
whole zone replaced the other. Keep both active contributors **DO NOT MOVE**
until the owners decide whether the repeated Caer Tannad rooms are intentional
parallel content or a stale copy. See the [Twin Keeps owner dossier](../zone-stories/TWIN_KEEPS_OF_DEVASTATED_THARNADIA.md),
[Tharnadia owner dossier](../zone-stories/THARNADIA.md),
[Caertannad coverage row 44](ZONE_COVERAGE_REGISTER.csv#L45), and
[Tharnadia coverage row 296](ZONE_COVERAGE_REGISTER.csv#L297).

### Checkpoint 710: Centaur Villages title and followed history

A fresh normalization of title lines in all 442 current `.zon` files finds
one exact “Centaur Villages” owner: `centaur_zone`. Both current indexes
retain `centaur_zone *933`; the full-area listing is tracked from 2009 and
the QST listing from 2012. See the [full-area index](../../../areas/AREA#L328),
[QST index](../../../areas/qst/AREA#L291), and
[title source](../../../areas/zon/centaur_zone.zon#L2).

Followed WLD, MOB, OBJ, ZON, and QST histories remain under the
`centaur_zone` stem from the earliest tracked 2009 area-tree move. Later
maintenance stays on those paths. The 2026-10-04 WLD/QST clue-alignment change
and story sidecar update the same area identity; they do not establish a
renamed or replacement zone. No alternate-stem predecessor, retirement, or
whole-zone replacement appears in the bounded history. The existing alias
question remains with the area owner. See the [Centaur Villages dossier](../zone-stories/CENTAUR_VILLAGES.md)
and [coverage row 49](ZONE_COVERAGE_REGISTER.csv#L50).

### Checkpoint 711: Cloister title and source-only name comparison

A fresh normalization of all 442 current `.zon` title lines finds one exact
“Father Tel's Holy Cloister” owner: `cloister`. Both current indexes retain
`cloister *671`. Followed WLD and ZON histories stay on that stem from the
earliest tracked 2009 area-tree move. No alternate-stem predecessor,
retirement, or whole-zone replacement appears in the bounded history. See the
[full-area entry](../../../areas/AREA#L253),
[QST entry](../../../areas/qst/AREA#L227), and
[title source](../../../areas/zon/cloister.zon#L2).

The semantic comparison found an unregistered `underworld2.wld` room #417216
titled “HOLY CLOISTER” with an empty description. Its southward source-only
map chain continues through room #417516, whose north exit targets active
Cloister room #67100. The room is a connector lead already recorded in the
[Cloister audit supplement](CLOISTER_AUDIT_REVIEW.md#active-and-source-only-wld-boundaries);
it is not a second active `.zon` title or a separate full-area contributor.
Tharnadia and Surface QST text also mentions Father Tel or “Temple Cloister” as
in-world character history, not as another area source. Those references do
not establish a separate predecessor or replacement.

Keep active `cloister` **DO NOT MOVE**. Its command-alias question and the
source-only approach's actual status remain separate owner/runtime questions.
See the [Cloister dossier](../zone-stories/FATHER_TELS_HOLY_CLOISTER.md),
[Underworld2 title](../../../areas/wld/underworld2.wld#L8672),
[Underworld2 target](../../../areas/wld/underworld2.wld#L10352), and
[coverage row 60](ZONE_COVERAGE_REGISTER.csv#L61).

### Checkpoint 712: Clawed Caverns title and followed history

A fresh normalization of title lines in all 442 current `.zon` files finds
one “The Clawed Caverns” owner: `clwcvrn`. Both current indexes list
`clwcvrn *807`. Followed WLD, MOB, OBJ, ZON, and QST paths stay on the
`clwcvrn` stem from the earliest tracked 2009 area-tree move. The story
sidecar first appears on 2026-10-02 as a map of that same source identity. No
alternate-stem predecessor, retirement, or whole-zone replacement appears in
the bounded history. See the [full-area entry](../../../areas/AREA#L287),
[QST entry](../../../areas/qst/AREA#L256), [title source](../../../areas/zon/clwcvrn.zon#L2),
and [Clawed Caverns dossier](../zone-stories/THE_CLAWED_CAVERNS.md).

The only located cross-map name/route overlap is the already recorded
source-only `ud_map2.wld` room #1124947 exit to active Clawed Caverns room
#80700. This unregistered WLD edge is not a second `.zon` identity or a
whole-area predecessor. Keep it classified as source-only; it does not prove
active ingress. See the [source-only WLD record](../../../areas/wld/ud_map2.wld#L59109),
[connectivity audit](UD_MAP2_AUDIT_REVIEW.md), and
[coverage row 61](ZONE_COVERAGE_REGISTER.csv#L62). The existing
command-alias question remains with the area owner.

### Checkpoint 713: Ultarium title and followed history

A fresh normalization of title lines in all 442 current `.zon` files finds
one exact “The Sky City of Ultarium” owner: `cosmic`. Both current indexes
list `cosmic *760-762 (Alachest)`. The same parenthetical regional label
appears on neighboring keys such as `roguerai *758-759`; it is a region
annotation, not a second area key or alternate title. See the [full-area
entries](../../../areas/AREA#L273) and [current title](../../../areas/zon/cosmic.zon#L2).

Followed WLD, MOB, OBJ, ZON, QST, and SHP paths remain under the `cosmic`
stem from the earliest tracked 2009 area-tree move. The January 2010 removal of
the Dwarven craftsman QST is a single quest retirement; the QST file and full
area remain, and later edits continue the same source paths. The current story
sidecar was added on 2026-10-03 as mapping for this identity. This bounded
history found no alternate-stem predecessor, area retirement, or whole-zone
replacement. See the [QST index](../../../areas/qst/AREA#L245),
[QST source](../../../areas/qst/cosmic.qst), and
[Ultarium dossier](../zone-stories/ULTARIUM.md).

Mentions of Ultarium in Brass item prose and White Harbor ambassador resets
are references to this active title, not other zone owners. Keep `cosmic`
**DO NOT MOVE**. The display-title and command-alias question remains with the
area owner. See [coverage row 63](ZONE_COVERAGE_REGISTER.csv#L64).

### Checkpoint 714: Court of the Muse creation and Faang shop-file lineage

A fresh normalization of the second-line titles in all 442 current `.zon`
files finds one exact “Court of the Muse” owner: `court`. The active full-area
index lists zone *67. The similar “The Tempest Court” title belongs to the
separate `airp` source; the names do not establish a shared or replacement
area. See the [Court title](../../../areas/zon/court.zon#L2),
[full-area entry](../../../areas/AREA#L36), and
[Tempest Court title](../../../areas/zon/airp.zon#L2).

Commit `fa22671e1` (2016-01-13) creates the Court WLD, MOB, OBJ, QST, and ZON
files and adds `court` to `areas/AREA` as new zone 67. Git's `C089` copy
detection identifies `areas/shp/court.shp` as derived from
`areas/shp/faang.shp`. The new shop uses keeper #6728 in room #6706 in place
of Faang's keeper #15226 and changes the stock and service fields. This is
shop-file lineage, not evidence that Faang was a predecessor or that its full
area was replaced. The later Court sidecar maps the same Court source
identity.

The Court QST file was created with the new area, but the current
`areas/qst/AREA` has no `court` entry; file existence alone does not establish
QST manifest membership or loading. Preserve this separate manifest question.
Keep active `court` **DO NOT MOVE**. The Court dossier now asks the owner to
confirm the intended Faang-to-Court shop reuse without merging the areas. See
the [Court owner dossier](../zone-stories/COURT_OF_THE_MUSE.md),
[Court shop](../../../areas/shp/court.shp#L1),
[Faang shop source](../../../areas/shp/faang.shp#L1), and
[coverage row 64](ZONE_COVERAGE_REGISTER.csv#L65).

### Checkpoint 715: Crakkaros' Liar title and followed history

A fresh normalization of title lines in all 442 current `.zon` files finds
one exact “Crakkaros' Liar” owner: `crakkaro`. Both current indexes list
`crakkaro *870-875`. The first tracked ZON and WLD sources already use the
same “Liar” spelling in the 2009 area-tree import; followed WLD, MOB, OBJ, ZON,
and QST paths remain on the `crakkaro` stem through later edits. No tracked
alternate title, alternate-stem predecessor, retirement, or whole-zone
replacement appears in the bounded history. The story sidecar was added on
2026-10-04 to map the same area identity. See the [full-area entry](../../../areas/AREA#L305),
[QST entry](../../../areas/qst/AREA#L271), [title source](../../../areas/zon/crakkaro.zon#L2),
and [WLD title](../../../areas/wld/crakkaro.wld#L8103).

The current title scan does not resolve whether “Liar” is the intended pun or
should be “Lair,” and it does not establish a command alias. Those spelling
and alias questions remain with the area owner as recorded in the
[Crakkaros' Liar dossier](../zone-stories/CRAKKAROS_LIAR.md). They do not
indicate a separate active area or a replacement. Keep `crakkaro` **DO NOT
MOVE**. See [coverage row 65](ZONE_COVERAGE_REGISTER.csv#L66).

### Checkpoint 716: Nakral's Crypt title and followed history

A fresh normalized scan of all 442 current `.zon` titles finds one exact
“Nakral's Crypt” owner: `crypt`. The other title containing “Crypts” is
“The Crypts of Rays” under a different owner. Both current indexes list
`crypt *143-145`. Followed WLD, MOB, OBJ, ZON, and QST paths remain on the
`crypt` stem from the 2009 area-tree move; no alternate-stem predecessor,
retirement, or whole-area replacement appears in those bounded histories. See
the [full-area entry](../../../areas/AREA#L79),
[QST entry](../../../areas/qst/AREA#L72),
[title source](../../../areas/zon/crypt.zon#L2), and
[Crypt history supplement](NAKRALS_CRYPT_AUDIT_REVIEW.md#title-and-tracked-wld-history).

Commit `5794d8067` temporarily changed room #14300's west destination from
Undead Outpost #98737 to #8604 while preparing surface-map connections.
`71d731c5b` restored #98737. An exact room-record scan of the tracked WLD
corpus at the first commit's parent found no #8604 room definition, so the
temporary endpoint is not evidence of a resolved route, alternate area, or
replacement. Keep active `crypt` **DO NOT MOVE**. See
[coverage row 66](ZONE_COVERAGE_REGISTER.csv#L67).

### Checkpoint 717: Darkfall Forest title and followed history

A fresh normalized scan of all 442 current `.zon` title lines finds one exact
“Darkfall Forest” owner: `darkfall`. The similarly worded “Dark Forest” title
belongs to distinct active stem `dark_forest`. Both indexes list
`darkfall *953`. The exact-title comparison and active membership are recorded
at the [full-area index](../../../areas/AREA#L340),
[QST index](../../../areas/qst/AREA#L300), and
[title source](../../../areas/zon/darkfall.zon#L2).

The `darkfall_forest` comment in the [archer assignment](../../../src/specs/specs.assign.c#L325)
resolves by exact VNUM to Dark Forest's mobile #75013 under `dark_forest`
([prototype](../../../areas/mob/dark_forest.mob#L187)); it is a naming question
already recorded in the [Darkfall Forest dossier](../zone-stories/DARKFALL_FOREST.md)
and [Dark Forest review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-204-dark-forest-title-and-history),
not evidence that one area replaced or aliases the other. Followed WLD, MOB,
OBJ, ZON, and QST histories remain on the `darkfall` stem from the 2009
area-tree move. The story sidecar was added on 2026-10-07. No alternate-stem
predecessor, retirement, or whole-area replacement was found. The already
reviewed 2013 Surface endpoint changes and the `surf`/`surface2011` map records
remain map-history evidence only; see the
[Surface and Darkfall lineage review](DUPLICATE_AND_REPLACED_ZONES.md#darkfall-forest-and-surface-map-lineage)
and [Darkfall supplement](DARKFALL_AUDIT_REVIEW.md). Keep active `darkfall`
**DO NOT MOVE**. See [coverage row 69](ZONE_COVERAGE_REGISTER.csv#L70).

### Checkpoint 718: The Motte and Bailey of Duke Delwyn title and history

A fresh normalized scan of all 442 current `.zon` title lines finds one exact
“The Motte and Bailey of Duke Delwyn” owner: `delwyn`. No other title contains
“Delwyn,” “Motte,” or “Bailey.” The active full-area and QST indexes both list
`delwyn *828-830`. The first tracked ZON file in 2009 already uses the current
title. See the [full-area index](../../../areas/AREA#L299),
[QST index](../../../areas/qst/AREA#L267), and
[title source](../../../areas/zon/delwyn.zon#L2).

Followed WLD, MOB, OBJ, ZON, and QST histories remain on the `delwyn` stem from
the 2009 area-tree move. Commit `a830dd38d` recodes the same ZON file to UTF-8
in 2025; it does not rename or replace the source. The story sidecar was added
on 2026-10-03. No alternate-stem predecessor, retirement, or whole-area
replacement was found. The dossier's command-target alias question remains
with the owner; it does not create a second title owner. Keep active `delwyn`
**DO NOT MOVE**. See the [Duke Delwyn dossier](../zone-stories/DUKE_DELWYN.md)
and [coverage row 70](ZONE_COVERAGE_REGISTER.csv#L71).

### Checkpoint 719: The Desert City of Venan'Trut title and history

A fresh normalized scan of all 442 current `.zon` title lines finds one exact
“The Desert City of Venan'Trut” owner: `desert`; no other current title
contains “Desert.” The full-area and QST indexes both list `desert *490-497`.
The `(scip)` annotation is retained source context and not a second area key.
The first tracked ZON file in 2009 already carries the current title. See the
[full-area index](../../../areas/AREA#L219),
[QST index](../../../areas/qst/AREA#L200),
[title source](../../../areas/zon/desert.zon#L2), and
[bounded access-history supplement](DESERT_CITY_AUDIT_REVIEW.md#current-title-and-tracked-access-history).

Followed WLD, MOB, OBJ, ZON, QST, and SHP histories remain on the `desert`
stem from the 2009 area-tree move. The 2026-10-04 story sidecar maps the same
identity. No alternate-stem predecessor, retirement, or whole-area
replacement was found. Commit `52bbee121` mentions a “Ruins of Desecration” /
“Temple of Flames” naming change, but its Desert-file diff changes only the
spelling in object #49140's description; it does not rename the Desert `.zon`
title or map.

The Abandoned Mines of Khul'Lor in Desert room #49572 say that the mage's
tower and citadel are where he resides now, then say his spirit lingers at the
haunted site. Current `lab` sources title a separate Swamp Laboratory of
Khul'Lor and reset mobile #82314 in room #82349. The text raises a chronology
and character-identity question already documented in the
[Lab owner review](LAB_AUDIT_REVIEW.md#builder-questions-and-recommended-resolutions);
it does not create a second Desert title owner or prove that either zone
replaced the other. Keep active `desert` **DO NOT MOVE**. See the
[Desert source dossier](../zone-stories/DESERT_CITY_OF_VENAN_TRUT.md),
[mine room](../../../areas/wld/desert.wld#L14357),
[Lab title](../../../areas/zon/lab.zon#L2),
[Khul'Lor reset](../../../areas/zon/lab.zon#L111), and
[coverage row 71](ZONE_COVERAGE_REGISTER.csv#L72).

### Checkpoint 720: Desolate title and invasion-zone relationship

A fresh normalized scan of all 442 current `.zon` title lines finds one exact
“Desolate” owner: `desolate`. “Desolate Under Fire” is a distinct title and
active source key `desolateinv`, listed separately as zone range *773–774
with the `(Invasion)` annotation. The full-area and QST indexes retain both
`desolate *222-223` and `desolateinv *773-774`; see the [Desolate
entry](../../../areas/AREA#L112), [Under Fire entry](../../../areas/AREA#L278),
[Desolate title](../../../areas/zon/desolate.zon#L2), and
[Under Fire title](../../../areas/zon/desolateinv.zon#L2).

Followed WLD, MOB, OBJ, ZON, QST, and SHP histories for `desolate` remain on
that stem from the 2009 area-tree move. Its story sidecar was added on
2026-10-04. The separate `desolateinv` files also have their own 2009 paths
and remain listed as an invasion scenario; no alternate-stem predecessor,
retirement, or whole-area replacement was found for `desolate`. The normal
city dossier keeps the invasion-phase relationship separate from its native
item receipts as an owner question. See [Desolate's invasion question](../zone-stories/DESOLATE.md#direct-area-owner-questions-and-conservative-recommendations),
the [Under Fire source dossier](../zone-stories/DESOLATE_UNDER_FIRE.md), and
[Under Fire history supplement](DESOLATE_UNDER_FIRE_AUDIT_REVIEW.md#active-title-and-tracked-history).

The 2026 Desolate Master change corrects two trial-direction descriptions;
it does not change the zone title or map identity. Keep both active keys
**DO NOT MOVE**. See [coverage row 72](ZONE_COVERAGE_REGISTER.csv#L73).

### Checkpoint 721: Desolate Under Fire title and followed history

A fresh normalized scan of all 442 current `.zon` title lines finds one exact
“Desolate Under Fire” owner: `desolateinv`. The related “Desolate” title is a
separate current source identity under `desolate`. Both full-area and QST
indexes retain their distinct keys and ranges: `desolateinv *773-774
(Invasion)` and `desolate *222-223`. See the [Under Fire full-area entry](../../../areas/AREA#L278),
[QST entry](../../../areas/qst/AREA#L248),
[Under Fire title](../../../areas/zon/desolateinv.zon#L2), and
[normal Desolate title](../../../areas/zon/desolate.zon#L2).

Followed WLD, MOB, OBJ, ZON, QST, and SHP histories remain on the `desolateinv`
stem from the 2009 area-tree move. The story sidecar was added on 2026-10-05.
The 2010 `shift_prime` removal and the 2026 trial-direction correction record
route-safety and prose maintenance; neither is a zone retirement or rename.
No alternate-stem predecessor, retirement, or whole-area replacement was
found. The normal Desolate dossier keeps the invasion-phase relationship
separate from native receipts and records it as an owner question. Keep both
active identities **DO NOT MOVE**. See the [Under Fire story dossier](../zone-stories/DESOLATE_UNDER_FIRE.md),
[title and history supplement](DESOLATE_UNDER_FIRE_AUDIT_REVIEW.md#active-title-and-tracked-history),
[normal Desolate question](../zone-stories/DESOLATE.md#direct-area-owner-questions-and-conservative-recommendations),
and [coverage row 73](ZONE_COVERAGE_REGISTER.csv#L74).

### Checkpoint 722: Home of the Divine title and followed history

A fresh normalized scan of all 442 current `.zon` title lines finds one exact
“Home of the Divine” owner: `divhome`. Both current indexes list
`divhome *407-408`. The `spshold.wld` MultiZone Load Room has a separate
wayfinding line “West = Divine Home”; it is a map label in another source file,
not a second active `.zon` title. The [full-area entry](../../../areas/AREA#L194),
[QST entry](../../../areas/qst/AREA#L179),
[title source](../../../areas/zon/divhome.zon#L2), and
[Home of the Divine dossier](../zone-stories/HOME_OF_THE_DIVINE.md) preserve
the current owner and its command-alias question.

Followed WLD, MOB, OBJ, ZON, QST, and SHP histories remain on the `divhome`
stem from the 2009 area-tree move. Both sides of the large 2012 update retain
the “Home of the Divine” title and `divhome` manifest key. That commit's
separate `smoke` and `Voluntown` entries and its artifact-content changes do
not identify a replacement for this area. The story sidecar was added on
2026-10-03. No alternate-stem predecessor, retirement, or whole-area
replacement was found. Keep active `divhome` **DO NOT MOVE**. See the
[map label](../../../areas/wld/spshold.wld#L989) and
[coverage row 75](ZONE_COVERAGE_REGISTER.csv#L76).

### Checkpoint 723: Domain of Lost Souls title and followed history

A fresh normalized scan of all 442 current `.zon` title lines finds one exact
“Domain of Lost Souls” owner: `dlsc`. The similar “Grumbar's Domain” title
belongs to `earthp`; “Pragtog's Domain” belongs to `pragtog`. These are
separate current titles and source owners. Both current indexes list
`dlsc *368-369`. See the [full-area entry](../../../areas/AREA#L178),
[QST entry](../../../areas/qst/AREA#L165),
[Domain title](../../../areas/zon/dlsc.zon#L2),
[Grumbar title](../../../areas/zon/earthp.zon#L2), and
[Pragtog title](../../../areas/zon/pragtog.zon#L2).

Followed WLD, MOB, OBJ, ZON, and QST histories remain on the `dlsc` stem from
the 2009 area-tree move; there is no local SHP file. The story sidecar was
added on 2026-10-06. No alternate-stem predecessor, retirement, or whole-zone
replacement was found. The unregistered `nexus56` reset and shop references
to Domain book #36822 remain typed source-only dependencies, as recorded in
the [Domain audit supplement](DLSC_AUDIT_REVIEW.md#title-history-and-disposition);
they do not establish a competing title or replacement. Keep active `dlsc`
**DO NOT MOVE**. See the [Domain dossier](../zone-stories/DOMAIN_OF_LOST_SOULS.md)
and [coverage row 76](ZONE_COVERAGE_REGISTER.csv#L77).


### Checkpoint 724: Drifting Realm title and followed history

A fresh normalized scan of all 442 current `.zon` title lines finds one exact “Drifting Realm” owner: `dream`. The other dream-containing titles belong to `ardgral` (“Ard'gral, plane of dreams”) and `eth2` (“The Fortress of Dreams”); they are separate title owners. Both current indexes list `dream *313 (Sizan)`. See the [full-area entry](../../../areas/AREA#L152), [QST entry](../../../areas/qst/AREA#L141), [Dream title](../../../areas/zon/dream.zon#L2), [Ard'gral title](../../../areas/zon/ardgral.zon#L2), and [Fortress title](../../../areas/zon/eth2.zon#L2).

Followed WLD, MOB, OBJ, ZON, QST, and SHP histories remain on the `dream` stem from the 2009 area-tree move through later source edits. The story sidecar was added on 2026-10-06. No alternate-stem predecessor, retirement, or whole-zone replacement was found. Keep active `dream` **DO NOT MOVE**; the dossier's command-target alias and owner-disambiguation questions remain separate. See the [Drifting Realm dossier](../zone-stories/DRIFTING_REALM.md), [title and history supplement](DRIFTING_REALM_AUDIT_REVIEW.md#title-history-and-disposition), and [coverage row 79](ZONE_COVERAGE_REGISTER.csv#L80).


### Checkpoint 725: Abandoned Elven Homestead title and history

A fresh normalized scan of all 442 current `.zon` title lines finds one exact “Abandoned Elven Homestead” owner: `elvish`. Both current indexes list `elvish *358-359`. The separate `elftomb` contributor at `*206-208` is titled “Barrow of the Quiosho”; it is not the Homestead title owner or a replacement. See the [full-area entry](../../../areas/AREA#L172), [QST entry](../../../areas/qst/AREA#L159), [Homestead title](../../../areas/zon/elvish.zon#L2), [Barrow entry](../../../areas/AREA#L107), and [Barrow title](../../../areas/zon/elftomb.zon#L2).

Followed WLD, MOB, OBJ, ZON, and QST histories remain on the `elvish` stem from the 2009 area-tree move. The 2016 ZON edit removes terminal punctuation and updates the editor marker; the normalized title remains unchanged. The story sidecar was added on 2026-10-02. No alternate-stem predecessor, retirement, or whole-zone replacement was found. Keep active `elvish` **DO NOT MOVE**. The Homestead dossier's title link and stem recommendation have been corrected; its command-target alias question remains open. See the [Homestead audit supplement](ELVISH_AUDIT_REVIEW.md#title-history-and-disposition), [source dossier](../zone-stories/ABANDONED_ELVEN_HOMESTEAD.md), and [coverage row 87](ZONE_COVERAGE_REGISTER.csv#L88).


### Checkpoint 726: Faang title and followed history

A fresh normalized scan of all 442 current `.zon` title lines finds one exact “Faang” owner: `faang`. Both current indexes list `faang *152-158`. “Old Faang” and “New Faang” do not appear as separate current `.zon` titles; their regional, alias, or predecessor identity remains an explicit owner question. See the [full-area entry](../../../areas/AREA#L84), [QST entry](../../../areas/qst/AREA#L77), and [Faang title](../../../areas/zon/faang.zon#L2).

Followed WLD, MOB, OBJ, ZON, and SHP histories remain on the `faang` stem from the 2009 area-tree move. The 2010 planetbound Illithid hometown commits add WLD content within Faang. No alternate-stem predecessor, retirement, or whole-zone replacement was found. The story sidecar was added on 2026-10-02. Keep active `faang` **DO NOT MOVE** while the “Old Faang” and “New Faang” owner question remains open. See the [Faang story and history review](FAANG_STORY_REVIEW.md#history-and-disposition) and [coverage row 93](ZONE_COVERAGE_REGISTER.csv#L94).


### Checkpoint 727: The Fields Between title and semantic comparison

A fresh normalized scan of all 442 current `.zon` title lines finds one exact “The Fields Between” owner: `fields_between`. The other current titles sharing the generic word “Fields” belong to `battlefi` (“The Battlefield”), `jademini` (“The Rice Fields”), `killing_fields` (“Killing Fields”), and `wander` (“The Wandering Fields”). These remain distinct current title owners with separate indexed ranges; lexical overlap does not establish a predecessor or replacement. Both current indexes list `fields_between *710-711 (kolut)`. See the [active-area entry](../../../areas/AREA#L265), [QST entry](../../../areas/qst/AREA#L237), [Fields Between title](../../../areas/zon/fields_between.zon#L2), [Battlefield title](../../../areas/zon/battlefi.zon#L2), [Rice Fields title](../../../areas/zon/jademini.zon#L2), [Killing Fields title](../../../areas/zon/killing_fields.zon#L2), and [Wandering Fields title](../../../areas/zon/wander.zon#L2).

Followed WLD/ZON histories remain on the `fields_between` stem from the 2009 area-tree move. The 2026 rift pickup corrections are object and quest maintenance; they do not change the zone title or establish a replacement. The story sidecar was added on 2026-10-04. No alternate-stem predecessor, retirement, or whole-zone replacement was found. Keep active `fields_between` **DO NOT MOVE**. The dossier's separate owner questions remain open. See the [Fields Between audit supplement](FIELDS_BETWEEN_AUDIT_REVIEW.md#active-title-and-tracked-history-boundary), [source dossier](../zone-stories/FIELDS_BETWEEN.md), and [coverage row 94](ZONE_COVERAGE_REGISTER.csv#L95).


### Checkpoint 728: The Altar of the Firesworn title and tracked history

A fresh normalized scan of all 442 current `.zon` title lines finds one exact “The Altar of the Firesworn” owner: `firesworn_altar`. Both current indexes list `firesworn_altar *1351`. Related fire-themed titles belong to distinct source owners: `plane_fire_one` (“Plane of Fire”), `brass` (“Plane of Fire, Brass”), `desolateinv` (“Desolate Under Fire”), and `temple` (“Temple of Flames”). These lexical neighbors do not establish an alternate owner or replacement. See the [full-area entry](../../../areas/AREA#L387), [QST entry](../../../areas/qst/AREA#L338), [Firesworn title](../../../areas/zon/firesworn_altar.zon#L2), [Plane of Fire](../../../areas/zon/plane_fire_one.zon#L2), [Brass](../../../areas/zon/brass.zon#L2), [Under Fire](../../../areas/zon/desolateinv.zon#L2), and [Temple](../../../areas/zon/temple.zon#L2).

Followed WLD/ZON history starts with the current `firesworn_altar` paths added on 2010-10-05 in the “Renumbers and uploads” commit. Later tracked changes remain on these paths; no alternate-stem predecessor, retirement, or whole-zone replacement was found in the reviewed repository history. The story sidecar was added on 2026-10-06. Keep `firesworn_altar` **DO NOT MOVE**. The same-numbered `Duris3.wld` rows remain inactive source maps and do not establish a replacement area. See the [Altar audit supplement](ALTAR_OF_THE_FIRESWORN_AUDIT_REVIEW.md#title-history-and-disposition), [story dossier](../zone-stories/THE_ALTAR_OF_THE_FIRESWORN.md), and [coverage row 96](ZONE_COVERAGE_REGISTER.csv#L97).


### Checkpoint 729: Fishermans Wharf title and history

A fresh normalized scan of all 442 current `.zon` title lines finds one exact “Fishermans Wharf” owner: `fishermans_wharf`. Dock and port wording in “Dalvik's Dockyard” (`dalvik`), “The Docks of Quietus Quay” (`quietus`), “Storm Port Stronghold” (`spshold`), “Storm Port” (`stormport`), and “The Ruins of Port Skythic” (`PortSkythic`) belongs to separate current owners. These are title similarities, not competing Wharf identities. Both current indexes list `fishermans_wharf *889`. See the [full-area entry](../../../areas/AREA#L315), [QST entry](../../../areas/qst/AREA#L281), [Wharf title](../../../areas/zon/fishermans_wharf.zon#L2), [Dalvik](../../../areas/zon/dalvik.zon#L2), [Quietus](../../../areas/zon/quietus.zon#L2), [Storm Port Stronghold](../../../areas/zon/spshold.zon#L2), [Storm Port](../../../areas/zon/stormport.zon#L2), and [Port Skythic](../../../areas/zon/PortSkythic.zon#L2).

Followed WLD, MOB, OBJ, ZON, QST, and SHP histories remain on the `fishermans_wharf` stem from the 2009 area-tree move through later edits. Recent 2025–2026 updates remain on that stem; the story sidecar was added on 2026-10-04. No alternate-stem predecessor, retirement, or whole-zone replacement was found. Keep active `fishermans_wharf` **DO NOT MOVE**. The dossier retains command-target alias disambiguation as an owner question. See the [Fishermans Wharf audit supplement](FISHERMANS_WHARF_AUDIT_REVIEW.md#title-history-and-disposition), [source dossier](../zone-stories/FISHERMANS_WHARF.md), and [coverage row 97](ZONE_COVERAGE_REGISTER.csv#L98).


### Checkpoint 730: The Town of Moregeeth title and goblin-themed neighbors

A fresh normalized scan of all 442 current `.zon` title lines finds one exact “The Town of Moregeeth” owner: `goblinht`. “The Goblin Camp” (`goblincamp`) and “Malch'Hor Ganl the Goblin City” (`malch`) are distinct current titles with indexed ranges *114 and *236-237. Their shared goblin theme does not establish alternate identity or replacement. Both current indexes list `goblinht *700-704`. See the [full-area entry](../../../areas/AREA#L261), [QST entry](../../../areas/qst/AREA#L234), [Moregeeth title](../../../areas/zon/goblinht.zon#L2), [Goblin Camp title](../../../areas/zon/goblincamp.zon#L2), and [Malch City title](../../../areas/zon/malch.zon#L2).

Followed WLD/ZON histories remain on the `goblinht` stem from the 2009 area-tree move. The 2014 Quartermaster transfer to `siege` is a specific mobile relocation while the Moregeeth title and range persist; the later 2026 letter-desk correction remains on the same stem. The story sidecar was added on 2026-10-04. No alternate-stem predecessor, retirement, or whole-zone replacement was found. Keep active `goblinht` **DO NOT MOVE**. See the [Moregeeth audit supplement](MOREGEETH_AUDIT_REVIEW.md#active-title-and-tracked-history-boundary), [source dossier](../zone-stories/THE_TOWN_OF_MOREGEETH.md), and [coverage row 110](ZONE_COVERAGE_REGISTER.csv#L111).


### Checkpoint 731: Golden Hall of the Crown title and history

A fresh normalized scan of all 442 current `.zon` title lines finds one exact “Golden Hall of the Crown” owner: `gold_hal`. The title words “gold” and “crown” occur nowhere else in the current title set. Both current indexes list `gold_hal *404-406`; the separately titled Halfling Silver Mine remains a different source identity. See the [full-area entry](../../../areas/AREA#L193), [QST entry](../../../areas/qst/AREA#L178), [Golden Hall title](../../../areas/zon/gold_hal.zon#L2), and [Halfling Silver Mine title](../../../areas/zon/mining.zon#L2).

Followed WLD, MOB, OBJ, ZON, QST, and SHP histories remain on the `gold_hal` stem from the 2009 area-tree move through later edits. The story sidecar was added on 2026-10-03. No alternate-stem predecessor, retirement, or whole-zone replacement was found. Keep active `gold_hal` **DO NOT MOVE**. See the [Golden Hall audit supplement](GOLDEN_HALL_OF_THE_CROWN_AUDIT_REVIEW.md#title-history-and-disposition), [source dossier](../zone-stories/GOLDEN_HALL_OF_THE_CROWN.md), and [coverage row 111](ZONE_COVERAGE_REGISTER.csv#L112).


### Checkpoint 732: Halfcut title article collision and parallel source history

A fresh normalized scan of all 442 current `.zon` title lines finds active `halfcut` titled “The Halfcut Hills” and unlisted `halfcut_hills` titled “Halfcut Hills”. The names differ only by the leading article. This is a close title match and a separate source-identity conflict; it does not by itself prove rename or replacement. Both active indexes list `halfcut *270-279`; neither lists `halfcut_hills`. See the [active-area entry](../../../areas/AREA#L135), [QST entry](../../../areas/qst/AREA#L127), [active title](../../../areas/zon/halfcut.zon#L2), and [unlisted candidate title](../../../areas/zon/halfcut_hills.zon#L2).

Checkpoint 488 records the source crosswalk: all 235 candidate rooms overlap active VNUMs, with 194 room blocks matching after zone-number normalization and 41 differing; all 45 candidate mobile IDs overlap but every full record differs; 22 object IDs overlap with 14 matching and eight differing; the candidate QST #27002 differs from its active counterpart. Followed histories stay on the two separate stems; no halfcut_hills-to-halfcut rename or explicit whole-zone replacement is found. Preserve active `halfcut` **DO NOT MOVE** and candidate `halfcut_hills` **RETAIN IN PLACE**; do not load both sources or remove either based on title resemblance. See the [candidate audit](HALFCUT_HILLS_AUDIT_REVIEW.md#source-history-and-disposition), [retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-488-halfcut-hills-retained-as-a-separate-inactive-source), [candidate row 51](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L51), [active dossier](../zone-stories/THE_HALFCUT_HILLS.md#evidence-and-review-boundary), and [coverage row 115](ZONE_COVERAGE_REGISTER.csv#L116).


### Checkpoint 733: Hall of the Ancients title and history

A fresh normalized scan of all 442 current `.zon` title lines finds one exact “The Hall of the Ancients” owner: `hall`. The nearest related title is “The Ancient Halls of Ironstar” (`lornecro`), a separate current contributor at *1389; generic Hall wording belongs to other distinct guildhall owners. Both current indexes list `hall *777-779`. See the [Hall title](../../../areas/zon/hall.zon#L2), [full-area entry](../../../areas/AREA#L280), [QST entry](../../../areas/qst/AREA#L250), [Ironstar title](../../../areas/zon/lornecro.zon#L2), and [Ironstar area entry](../../../areas/AREA#L396).

Followed WLD/ZON histories remain on the `hall` stem from the 2009 area-tree move through later updates. The story sidecar was added on 2026-10-03. No alternate-stem predecessor, retirement, or whole-zone replacement was found. A 2012 commit discusses an Ironstar zone-marker change and a separate Hall of the Ancients upgrade; that shared commit does not merge the titles or source paths. Keep active `hall` **DO NOT MOVE**. See the [Hall audit supplement](HALL_OF_THE_ANCIENTS_AUDIT_REVIEW.md#title-history-and-disposition), [source dossier](../zone-stories/THE_HALL_OF_THE_ANCIENTS.md), [Ironstar history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-267-ironstar-title-and-tracked-history), and [coverage row 116](ZONE_COVERAGE_REGISTER.csv#L117).


### Checkpoint 734: Mountain Settlement of the Harpies title and shared Alachest note

A fresh normalized scan of all 442 current `.zon` title lines finds one exact “The Mountain Settlement of the Harpies” owner: `harpyht`. Both current indexes list `harpyht *310-312 (Alachest)`. The same parenthetical appears on four other current contributors: `roguerai *758-759`, `cosmic *760-762`, `jade *766-771`, and `airshipgrave *775-776`. This shared area note does not identify a unique title owner, Harpy alias, predecessor, or replacement. See the [Harpy full-area entry](../../../areas/AREA#L151), [Harpy QST entry](../../../areas/qst/AREA#L140), [Harpy title](../../../areas/zon/harpyht.zon#L2), [Roguerai](../../../areas/AREA#L273), [Cosmic](../../../areas/AREA#L274), [Jade](../../../areas/AREA#L276), and [Airshipgrave](../../../areas/AREA#L279).

Followed WLD/ZON histories remain on the `harpyht` stem from the 2009 area-tree move through later edits. The story sidecar was added on 2026-10-05. No alternate-stem predecessor, retirement, or whole-zone replacement was found. Keep active `harpyht` **DO NOT MOVE** while the command-target alias and area-owner question remains open. See the [Harpy audit supplement](HARPYHT_AUDIT_REVIEW.md#active-title-and-tracked-history), [source dossier](../zone-stories/THE_MOUNTAIN_SETTLEMENT_OF_THE_HARPIES.md), and [coverage row 117](ZONE_COVERAGE_REGISTER.csv#L118).


### Checkpoint 735: Harrow -The Gnome Village title and shared area note

A fresh normalized scan of all 442 current `.zon` title lines finds one exact “Harrow -The Gnome Village” owner: `harrow`; “Harrow” and “Gnome” each occur only in this title. Both current indexes list `harrow *294-296 (haruko)`. The neighboring active `newbie *292-293 (haruko)` is titled “Ailvio, Duris Newbie Outpost”. The shared parenthetical and adjacent ranges do not establish a Harrow alias, predecessor, or replacement. See the [Harrow full-area entry](../../../areas/AREA#L142), [Harrow QST entry](../../../areas/qst/AREA#L132), [Harrow title](../../../areas/zon/harrow.zon#L2), [Newbie full-area entry](../../../areas/AREA#L141), [Newbie QST entry](../../../areas/qst/AREA#L131), and [Newbie title](../../../areas/zon/newbie.zon#L2).

Followed WLD history reaches the 2009 area-tree move and later room text edits without changing Harrow exit endpoints. ZON history changes editor-version and reset display wording while preserving reset values. No alternate-stem predecessor, retirement, or whole-zone replacement was found. The story sidecar was added on 2026-10-04. Keep active `harrow` **DO NOT MOVE** while the parenthetical note and command-target owner question remain unresolved. See the [Harrow audit supplement](HARROW_AUDIT_REVIEW.md#active-title-and-tracked-history), [source dossier](../zone-stories/HARROW_THE_GNOME_VILLAGE.md), and [coverage row 118](ZONE_COVERAGE_REGISTER.csv#L119).


### Checkpoint 736: Heaven title and tracked history

A fresh normalized scan of all 442 current `.zon` title lines finds one exact “Heaven” owner: `heavens`. Both current indexes retain the key with the `(heavens duh!)` annotation. The closest celestial-themed title is “Celestial Plane” under separate current `celestia`; it does not share the Heaven title or key. No other current title line contains “heaven”. See the [Heavens full-area entry](../../../areas/AREA#L2), [QST entry](../../../areas/qst/AREA#L2), [Heaven title](../../../areas/zon/heavens.zon#L2), [Celestia title](../../../areas/zon/celestia.zon#L2), [Celestia full-area entry](../../../areas/AREA#L214), and [Celestia QST entry](../../../areas/qst/AREA#L196).

Followed WLD, MOB, OBJ, ZON, and QST histories remain on the `heavens` stem from the 2009 area-tree move. SHP is a later same-stem path. Commit `6fad15aac` moves only Heavens room #1299 into `ships.wld`; the remaining Heavens title and key persist. No alternate-stem predecessor, retirement, or whole-zone replacement was found. Keep active `heavens` **DO NOT MOVE**. See the [Heavens audit supplement](HEAVENS_AUDIT_REVIEW.md#title-history-and-disposition), [source review](HEAVENS_STORY_REVIEW.md), and [coverage row 120](ZONE_COVERAGE_REGISTER.csv#L121).


### Checkpoint 737: Hell Three title, tracked history, and collision classification

A fresh normalized scan of all 442 current `.zon` titles finds one exact “Hell, Minauros” owner: `hell_three_minauros`. The four named Hell planes have distinct titles and source owners: “Hell, Avernus” (`hell_one_avernus`, *510–513), “Hell, Dis” (`hell_two_dis`, *514–517), “Hell, Minauros” (`hell_three_minauros`, *518–521), and “Hell, Phlegethos” (`hell_four_phlegethos`, *522–528). See the [Avernus](../../../areas/zon/hell_one_avernus.zon#L2), [Dis](../../../areas/zon/hell_two_dis.zon#L2), [Minauros](../../../areas/zon/hell_three_minauros.zon#L2), and [Phlegethos](../../../areas/zon/hell_four_phlegethos.zon#L2) titles and the [world roster](../../../areas/AREA#L221-L224).

Followed WLD, MOB, OBJ, and ZON histories remain on their current stems. The two different WLD definitions of room `#51800` are both present in the tracked 2009 source-tree move, with later same-stem maintenance in 2013 and 2015. No tracked rename, alternate-stem predecessor, or whole-zone replacement was found. This closes the static title/history and duplicate/replacement classification: the duplicate VNUM remains a real unresolved source collision, not proof that either full zone replaced the other. Keep the intended `#51800` owner, effective route, and runtime lookup unresolved for builder decision; make no source ownership repair from this audit. See the [Minauros audit supplement](HELL_THREE_MINAUROS_AUDIT_REVIEW.md#title-history-and-disposition) and [coverage row 123](ZONE_COVERAGE_REGISTER.csv#L124).


### Checkpoint 738: Hell Two title, tracked history, and collision classification

A fresh normalized scan of all 442 current `.zon` titles finds one exact “Hell, Dis” owner: `hell_two_dis`. The four named Hell-plane contributors have distinct titles and ranges: “Hell, Avernus” (`hell_one_avernus`, *510–513), “Hell, Dis” (`hell_two_dis`, *514–517), “Hell, Minauros” (`hell_three_minauros`, *518–521), and “Hell, Phlegethos” (`hell_four_phlegethos`, *522–528). “Mentiri, the Prison of Dis” is a separate contributor. See the [Avernus](../../../areas/zon/hell_one_avernus.zon#L2), [Dis](../../../areas/zon/hell_two_dis.zon#L2), [Minauros](../../../areas/zon/hell_three_minauros.zon#L2), and [Phlegethos](../../../areas/zon/hell_four_phlegethos.zon#L2) titles and the [world roster](../../../areas/AREA#L221-L224).

Followed WLD and ZON histories remain on `hell_two_dis` from the 2009 tracked area-tree move through 2013; MOB and OBJ histories remain on the same stem through 2012, 2013, and 2015. No tracked alternate-stem predecessor, title replacement, or whole-zone replacement was found. Hell Two and Minauros retain different active definitions of room `#51800`; this is a room-level collision, not evidence that either complete zone replaced the other. Static title/history and duplicate/replacement classification is complete, while the room owner, runtime lookup, and player route remain unresolved for builder decision. Keep active `hell_two_dis` **DO NOT MOVE**; see the [Hell Two audit supplement](HELL_TWO_DIS_AUDIT_REVIEW.md#title-history-and-disposition) and [coverage row 124](ZONE_COVERAGE_REGISTER.csv#L125).


### Checkpoint 739: Behemoth Herders title and tracked history

A normalized scan of all 442 current `.zon` titles finds one exact “The Behemoth Herders” owner: active `herders` ([title](../../../areas/zon/herders.zon#L2)). Followed Herders WLD and ZON histories remain on the `herders` stem from the 2009 area-tree relocation through later same-stem edits. No distinct predecessor, alternate-stem successor, title rename, or whole-zone replacement was found. The four Surf and Underdark source-only exits that target Herders room `#94300` are excluded source references, not old Herders maps or replacement evidence. Static title/history and duplicate/replacement classification is complete; semantic alias and command-target ownership questions remain separate. Keep active `herders` **DO NOT MOVE**; see the [Herders audit supplement](HERDERS_AUDIT_REVIEW.md#title-and-tracked-history) and [coverage row 125](ZONE_COVERAGE_REGISTER.csv#L126).


### Checkpoint 740: Limbo title, semantic neighbor, and tracked history

A fresh normalized scan of all 442 current `.zon` titles finds one exact “Limbo” owner: active `limbo` ([title](../../../areas/zon/limbo.zon#L2)). Limbo WLD room #0 is titled “The Void”; the separate unregistered `void_gh` candidate is titled “Main Guildhall for The Void” ([Limbo room](../../../areas/wld/limbo.wld#L1), [candidate title](../../../areas/zon/void_gh.zon#L2)). The candidate dependency register retains `void_gh` under its own identity, and its audit records 19 different room-number collisions with active Torg plus no tracked predecessor or whole-zone replacement ([candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L119), [candidate audit](VOID_GH_AUDIT_REVIEW.md)). Similar wording alone does not establish an alias, predecessor, or replacement; the command-target owner question stays separate.

Followed Limbo WLD/ZON paths reach the 2009 `areas/` relocation and the 2013 path-history entry; later MOB/OBJ edits remain on their own stems. No tracked retired Limbo predecessor, alternate-stem rename, or whole-zone replacement was found. This completes static title/history and duplicate/replacement classification while leaving Limbo active and the separate `void_gh` candidate retained in place; no move or source ownership change follows. See the [Limbo source review](LIMBO_STORY_REVIEW.md#tracked-history-and-owner-questions) and [coverage row 158](ZONE_COVERAGE_REGISTER.csv#L159).


### Checkpoint 741: Du'Maathe Castle title, inactive approaches, and tracked history

A normalized scan of all 442 current `.zon` titles finds one exact “Du'Maathe Castle” owner: active `mntcastl`, zone 371 ([title](../../../areas/zon/mntcastl.zon#L2)). Followed WLD/ZON history stays on the current stem and identifies no distinct predecessor or whole-zone replacement.

The five inactive `oldmap8.wld` and `surf.wld` records previously flagged as semantic evidence are separate room VNUMs outside Castle's *371–376 range, have empty room descriptions, and lead only to three described active boundary rooms: #37409 “A Slow Flowing River,” #37416 “A Little Used Dirt Road,” and #37425 “Chiltern Way.” Their titles identify bridge, DragonClaws path, Cheltenham Fields, and river approaches. Neither source file contains the literal `Du'Maathe` or `mntcastl`. They provide no alternate Castle interior, shared room identity, predecessor, or replacement evidence; they also do not count as active exits or prove safe entry/return. Static title/history and the bounded semantic comparison are complete. Keep active `mntcastl` **DO NOT MOVE**; see the [Castle audit supplement](DU_MAATHE_CASTLE_AUDIT_REVIEW.md#active-title-and-tracked-history-boundary) and [coverage row 183](ZONE_COVERAGE_REGISTER.csv#L184).


### Checkpoint 742: Lost Temple of the North title and tracked history

A fresh normalized scan of all 442 current `.zon` titles finds one exact “The Lost Temple of the North” owner: active `multiclass_temple` (*529). The similarly named title “Lost Temple of Tikitzopl” belongs to separate active `tikitt` (*441–443); both keys occur separately in the world and QST rosters. Followed WLD/MOB/OBJ/ZON/QST/SHP histories for `multiclass_temple` remain on that stem from the 2009 area-tree move through later same-stem edits, with no alternate-stem predecessor, tracked rename, or whole-zone replacement claim. This completes static duplicate/replacement classification; whether `tikitt` or another name is a command alias remains an owner question, not evidence that either area replaced the other. Keep both active area identities separate and **DO NOT MOVE**. See the [Temple audit supplement](MULTICLASS_TEMPLE_AUDIT_REVIEW.md#active-title-and-tracked-history), [North title](../../../areas/zon/multiclass_temple.zon#L2), [Tikitzopl title](../../../areas/zon/tikitt.zon#L2), and [coverage row 193](ZONE_COVERAGE_REGISTER.csv#L194).


### Checkpoint 743: Mountain of Peril Peaks title and Nexus-name sources

A fresh normalized scan of all 442 current `.zon` titles finds one exact “The Mountain of Peril Peaks” owner: active `nexus` (*575–580). Related current titles include “The Reliquary Nexus of Aggression,” “The Reliquary Nexus of the Corsairs,” “The Reliquary Nexus of the Roper Den,” and “The Reliquary Nexus of the Dead,” each under its own active `nexus_*` key and index entry. Separate unregistered candidates `nexus56` (“Nexus”) and `Nexus_1` (“Nexus_1”) retain their own low-VNUM source identities and have no tracked successor or whole-zone replacement evidence in their audits. Similar keys or titles do not establish that either candidate replaces the active Peril Peaks area.

Followed WLD/MOB/OBJ/ZON/QST/SHP history for active `nexus` remains on the same stem from the 2009 area-tree move through later same-stem edits; no alternate-stem predecessor, tracked rename, or whole-zone replacement was found. Static title/history and duplicate/replacement classification is complete. Keep active `nexus` **DO NOT MOVE** and retain both candidates under their existing identities; leave command-target alias/owner questions open separately. See the [Peril Peaks audit supplement](NEXUS_AUDIT_REVIEW.md#active-title-and-tracked-history), [Nexus56 audit](NEXUS56_AUDIT_REVIEW.md), [Nexus_1 audit](NEXUS_1_AUDIT_REVIEW.md), and [coverage row 206](ZONE_COVERAGE_REGISTER.csv#L207).


### Checkpoint 744: Nizari title, regional references, and tracked history

A normalized scan of all 442 current `.zon` titles finds one exact “Nizari” owner: active `nizari` (*400–401). The broader source scan finds regional references, not alternate Nizari maps: active Basin Wastes room #34008 describes paths from Nizari but its south edge targets local Basin room #34009; the unregistered `surface2011` and `surface2011-temp` maps each contain one empty-description room titled “Nizari,” whose four exits stay among neighboring grid VNUMs in those source maps. Neither map cell targets Nizari's active #40000–#40199 range. Active `desert`, “The Desert City of Venan'Trut” (*490–497), has a separate title and source identity; its shared desert/Sultan/palace themes are not replacement evidence. Other exact Nizari references identify an external sword, ambassador, or Nizarian origin, not another city map. See the [Nizari source review](NIZARI_STORY_REVIEW.md#cross-world-semantic-and-source-map-comparison), [world-connectivity note](WORLD_CONNECTIVITY.md#nizari-surface-route-and-phrase-open-doors), [Basin Wastes passage](../../../areas/wld/basin_wa.wld#L194), [two source-map labels](../../../areas/wld/surface2011.wld#L577249), and [Desert City title](../../../areas/zon/desert.zon#L2).

Followed Nizari WLD history reaches the 2009 area-path move and 2011/2013 same-stem content edits; no direct retirement, alternate-stem predecessor, or whole-zone replacement mapping was found. Static title/history and bounded semantic-source comparison are complete. Keep active `nizari` **DO NOT MOVE**; keep alias and command-target ownership questions open separately. See [coverage row 212](ZONE_COVERAGE_REGISTER.csv#L213).


### Checkpoint 745: Northern Lakes title, neighboring settlements, and tracked history

A normalized scan of all 442 current `.zon` titles finds one exact “Northern Lakes and Settlements” owner: active `nlakes` (*752–754). Similar northern/settlement titles have distinct owners and ranges, including `northern_wilderness` (*122–123), `teka2` (*755), `harpyht`, `kobold`, and `shaughin. A full WLD room-ID scan across all 446 current WLD files found the 219 Nlakes room VNUMs only in `nlakes.wld`; full MOB/OBJ same-type scans found no other definitions for its 63 mobile or 82 object VNUMs.

Followed WLD, MOB, OBJ, ZON, QST, and SHP histories remain on `nlakes` from the 2009 area-tree relocation through later same-stem edits. The 2026 WLD change corrects two directional words; it does not change the map identity. The story sidecar was added on 2026-10-04. No alternate-stem predecessor, tracked rename, or whole-zone replacement was found, and the attic candidate register contains no `nlakes` row. Static title/history and bounded semantic comparison are complete. Keep active `nlakes` **DO NOT MOVE**; see the [Northern Lakes source dossier](../zone-stories/NORTHERN_LAKES.md#title-nearby-areas-and-tracked-history), [title](../../../areas/zon/nlakes.zon#L2), and [coverage row 213](ZONE_COVERAGE_REGISTER.csv#L214).


### Checkpoint 746: Cimmerian Nomad Encampment title and map identity

A normalized scan of all 442 current `.zon` titles finds one exact “The Cimmerian Nomad Encampment” owner: active `nomads`. The only other current `.zon` title containing “Encampment” is the separate “Orog Encampment” (`orogs`, *458–459). Surface WLD room #582495 is titled “Cimmerian Nomads” but has an empty description and four exits among Surface map cells; it is a regional map tile, not another contributor or duplicate camp map. The adjacent Surface room #582494 is the actual reciprocal entry to active Nomads room #6221. A Savannah sword description refers to Cimmerian chieftain Dodecimus, which adds a separate historical reference but no alternate zone source. Global same-type scans found no WLD, MOB, or OBJ VNUM definition for any of the Nomads' 49 rooms, 21 mobiles, or 23 objects outside their own files.

Followed WLD/ZON paths remain on `nomads` from the 2009 area-tree relocation; the 2015 ZON edit changes only the editor-version comment. No alternate-stem predecessor, rename, or whole-zone replacement was found. Static title/history and bounded semantic/map comparison are complete; leave command-target alias and owner questions separate. Keep active `nomads` **DO NOT MOVE**. See the [Nomads audit supplement](CIMMERIAN_NOMAD_ENCAMPMENT_AUDIT_REVIEW.md#active-title-and-tracked-history), [Surface map tile](../../../areas/wld/surface.wld#L1731739), [camp boundary](../../../areas/wld/surface.wld#L1731718), [Savannah reference](../../../areas/obj/savannah.obj#L249), and [coverage row 214](ZONE_COVERAGE_REGISTER.csv#L215).


### Checkpoint 747: Enclave of the Opal Phoenix title and `aopal` separation

A normalized scan of all 442 current `.zon` titles finds one exact “Enclave of the Opal Phoenix” owner: active `opalphoenix`, zone 708. The similarly keyed unregistered candidate `aopal` has a different current title, “The Tharnadian Causeway,” and zone header #433; its existing history review finds no predecessor or replacement and keeps it **RETAIN IN PLACE**. Its known same-number room conflicts are with active Ship Yards, not the Opal Phoenix. Full same-type scans of all WLD/MOB/OBJ files found no external definition for any of the Opal Phoenix area's 76 rooms, 18 mobiles, or 24 objects.

Followed WLD/MOB/OBJ/ZON/QST/SHP histories remain on `opalphoenix` from the 2009 area-tree move through later same-stem maintenance. The 2026 OBJ edit makes the quest sand selectable and does not change area identity; the revision-one story sidecar was added on 2026-10-04. No alternate-stem predecessor, tracked rename, or whole-zone replacement was found. Static title/history classification is complete. Keep active `opalphoenix` **DO NOT MOVE** and preserve candidate `aopal` under its existing retention plan; leave command-target alias questions separate. See the [Opal Phoenix source dossier](../zone-stories/OPAL_PHOENIX.md#title-source-history-and-similarly-keyed-candidate), [Aopal checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-448-aopal-tharnadian-causeway-title-history-and-room-identities), [candidate row 9](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L10), and [coverage row 222](ZONE_COVERAGE_REGISTER.csv#L223).

### Checkpoint 748: Orog Encampment title, related Ogre names, and tracked history

A normalized scan of all 442 current `.zon` titles finds one exact “Orog Encampment” owner: active `orogs`, *458–459. The only other current title containing “Encampment” is “The Cimmerian Nomad Encampment” (`nomads`, *62); other related names belong to “Ogre Raiders” (`raiders`, *89) and “Valley of the Snow Ogres” (`snogres`, *877). All are separately listed current contributors with distinct `.zon` titles and ranges. In particular, Snow Ogres is its own 100-room valley map (#87700–#87799), QST, reset and procedure set; its reviewed story sources contain no Orog-name reference. Orog rooms #45800–#45923 have their own documented reciprocal Underdark boundaries. These related Ogre names do not identify duplicate room owners, a predecessor, or a replacement.

A wider source-name scan finds other Orog residents and lore under their own area identities: Underdark has the connected Orog entrance rooms and a separate dispersal room; other mentions occur in Faang, Dagger Peak, Ceothia, Alatorin and Surface. Current Surface resets Ruukoz mobile #500182 to room #660000, a teacher-distribution room; his description says he was sent out from a small Orog encampment. The retained source-only `surface2011` map has a parallel Ruukoz mobile #200161 and copied Gr'Tak quest text, but its WLD room #200161 is an empty-description “Azure Waters of a Stormy Sea” grid tile and its ZON has no reset for that mobile. Preserve `surface2011` under its existing candidate row 97 plan; these Surface NPC/story copies do not supply an alternate Orog home map or replacement evidence. See the [Orog source review](OROGS_STORY_REVIEW.md#title-related-area-identities-and-followed-history), [Surface2011 audit](SURFACE2011_AUDIT_REVIEW.md), and [candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L98).

Followed Orog WLD, MOB, OBJ, ZON, and SHP histories begin with the 2009 area-tree move; later tracked changes remain on the `orogs` stem. The story sidecar was added in 2026. No alternate-stem predecessor, tracked rename, or whole-zone replacement was found. Static title/history and bounded semantic classification are complete; keep active `orogs` **DO NOT MOVE** and leave command-target alias/owner questions separate. See the [Orog source review](OROGS_STORY_REVIEW.md#title-related-area-identities-and-followed-history), [Snow Ogres audit](SNOGRES_AUDIT_REVIEW.md#active-graph-and-source-identity), and [coverage row 223](ZONE_COVERAGE_REGISTER.csv#L224).

### Checkpoint 749: Orrak title, external story references, and retained approaches

A normalized scan of all 442 current `.zon` titles finds one exact “Orrak” owner: active `orrak`, zone 90, with 39 rooms (#9000–#9038). Other current titles with prison language have separate names and stems. White Harbor's Ambassador from Orrak and its “memory of Orrak” exchange refer back to Vellel and the active Orrak story; they are external NPC/item consumers, not another map. Charing's active and Sylvandawn's source-only G resets consume local Orrak object #9017, the black iron mask, and remain under their own area/candidate identities.

The full inactive-WLD review's Surf room #260629 is titled “A Stone Road Through a Dark Forest,” has an empty description, and has a source-only north edge to Orrak #9000. It is a retained approach, not an active route or alternate Orrak map; the active reciprocal route remains Surface #610629 to Orrak #9000. Followed Orrak WLD/ZON histories remain on the current stem, with no alternate-stem predecessor or whole-zone replacement. Keep active `orrak` **DO NOT MOVE**, preserve the existing Surf and Sylvandawn **RETAIN IN PLACE** plans, and leave command-target alias/owner questions separate. See the [Orrak semantic review](ORRAK_AUDIT_REVIEW.md#semantic-references-and-candidate-map-identity), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), [Sylvandawn candidate row 101](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L101), and [coverage row 224](ZONE_COVERAGE_REGISTER.csv#L225).

### Checkpoint 750: Randomness title, tracked history, and random-lab candidate

A normalized scan of all 442 current `.zon` titles finds one exact “Randomness” owner: active `random`. Its world entry is annotated *1000-? and its QST entry *1000; the 10,000-room WLD range is #100000–#109999. No other current zone title identifies this map. The empty `areas/RANDOM_AREA` path is a configuration file, not an alternate area or predecessor.

Unregistered `map22` is a distinct retained random-lab input: its only source is a 10,000-room WLD grid at #900000–#909999 titled “A labyrinth in the grave,” with no ZON or manifest entry. Conditional `reset_lab`/`create_lab` code can reference that grid, but the different title and non-overlapping VNUM range do not make it a second Randomness map or whole-zone replacement. Keep the existing `map22` candidate row 64 **RETAIN IN PLACE** and do not activate or merge it from this title review. See the [Randomness identity and history review](../zone-stories/RANDOMNESS.md#title-and-tracked-identity), [map22 audit](MAP22_AUDIT_REVIEW.md), and [candidate row 64](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L65).

Followed `random` WLD/MOB/OBJ/ZON/SHP histories remain on the current stem and show no alternate-stem predecessor or whole-zone replacement. The exact title/history classification is complete; command-target alias and owner questions remain separate. Keep active `random` **DO NOT MOVE** and the candidate's existing plan unchanged. See [coverage row 246](ZONE_COVERAGE_REGISTER.csv#L247).

### Checkpoint 751: Faerie Realm title and neighboring realm-named areas

A normalized scan of all 442 current `.zon` titles finds one exact “Faerie Realm” owner, active `realm` (*140–142). The other current titles with Realm/Realms wording are “The Realm of Barovia” (`barovia`), “The Realm of Barovia Continued” (`barovia2`), “The Great Realm of Duris” (`connectorzones`), “Drifting Realm” (`dream`), “The Surface Realm of Duris” (`surface`), “The Dark Stone Tower of the Northern Realms” (`teka2`), “Thetis's Realm” (`thetis`), and “The Lost Realms of Shadowfall” (`wolfen`). Each is a separate current key and title; `barovia2` is explicitly a separately listed continuation, not a Faerie Realm replacement.

The completed Realm audit found 15 generic room-title matches with no full-description matches, no duplicate same-type WLD/MOB/OBJ definitions for its VNUMs, and a followed `realm` history on the same stem back to the 2009 area-tree move. The only inactive-WLD approach is Surf #244463 south to Realm #14002, already preserved under Surf candidate row 97; it is not active ingress or alternate-zone evidence. Keep active `realm` **DO NOT MOVE**, preserve existing candidate plans, and leave command-target alias ownership separate. See the [Faerie Realm identity review](FAERIE_REALM_AUDIT_REVIEW.md#title-identity-and-tracked-history), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [coverage row 250](ZONE_COVERAGE_REGISTER.csv#L251).


### Checkpoint 752: Great Shaboath semantic identity and tracked history

A normalized scan of all 442 current `.zon` titles finds one exact “The Great Shaboath” owner: active `shabo` (*328–329). Both area rosters, `areas/map/shabo.json`, the story sidecar, and Pallistren's QST dialogue point to that same identity. The only external name matches are the active Underdark guard's city warning (also copied in retained `underdark2011` mobile source) and two Surface grid labels, “connector to shabo” (#511781 and #513384); neither defines another Shaboath area. The active cross-area register remains limited to ConnectorZones, Winterhaven, and reciprocal Underdark edges.

The completed CP353 review found no same-type active prototype overlap, inactive-WLD room overlap or approach, alternate-stem predecessor, distinct prior zone, or explicit whole-zone replacement. Followed histories remain on `shabo`; the semantic-name review adds no alternate map owner. Keep active `shabo` **DO NOT MOVE**, preserve existing candidate plans, and leave command-target alias/owner disambiguation separate. See the [Great Shaboath audit supplement](GREAT_SHABOATH_AUDIT_REVIEW.md#checkpoint-752-semantic-names-and-area-identity), [title](../../../areas/zon/shabo.zon#L2), [Surface labels](../../../areas/wld/surface.wld#L247411), [Underdark lore reference](../../../areas/mob/underdark.mob#L507), and [coverage row 264](ZONE_COVERAGE_REGISTER.csv#L265).


### Checkpoint 753: Orcish Slave Camp semantic identity and followed history

A normalized scan of all 442 current `.zon` titles finds one exact “The Orcish Slave Camp” owner: active `shortc` (*532). Nearby named areas remain separate current identities: “Bandit Camp” (`banditca`), “The Goblin Camp” (`goblincamp`), “The Cimmerian Nomad Encampment” (`nomads`), “Orog Encampment” (`orogs`), “Troll Slave Compound” (`torture`), and “Drustl's Yerdonia Enslaved” (`raxthan`). Other source phrases refer to the separate `llzazan` Slave Camp, Hunt's prisoner compound, Quietus/Raxthan/Thri Orcish-slave mobiles, or Herders/Khildarak compound descriptions. They do not identify a second `shortc` map or duplicate its room range. The attic candidate register has no `shortc` candidate row.

Followed WLD/MOB/OBJ/ZON/QST histories remain on `shortc` from the 2009 area-tree move; the story sidecar was added in 2026. WLD metadata edits did not change exits, and the ZON editor-comment update preserved resets. No alternate-stem predecessor, distinct prior area, or explicit whole-zone replacement was found. CP48's full inactive-WLD and typed scans found no same-number room, source-only approach, or external typed consumer. Keep active `shortc` **DO NOT MOVE**, preserve all existing candidate plans, and leave command-target alias/owner disambiguation separate. See the [Orcish Slave Camp identity audit](ORCISH_SLAVE_CAMP_AUDIT_REVIEW.md#semantic-titles-source-references-and-map-identity), [title](../../../areas/zon/shortc.zon#L2), [llzazan camp reference](../../../areas/zon/llzazan.zon#L17), [Hunt prisoner story](../../../areas/story/hunt.story.json#L285), and [coverage row 274](ZONE_COVERAGE_REGISTER.csv#L275).


### Checkpoint 754: Storm Port Stronghold, Storm Port, and Jade outpost identities

A normalized scan of all 442 current `.zon` titles finds one exact “Storm Port Stronghold” owner: active `spshold`, separately listed as *226–227 in both area rosters. Active `stormport` is the separate “Storm Port” area at *224–225. Stronghold room #22649 has one-way DOWN to Storm Port room #22423 and no static inbound to #22649; this connection demonstrates linked but distinct area identities. Jade's separately listed “The Jade Empire” contains its own Storm Port authority outpost at #76631–#76632, described as leased territory between the Jade capital and the outpost. Shared faction/settlement language does not make the Jade rooms or Storm Port city a Stronghold replacement.

SurfaceKeeps dialogue names the Stronghold as the Hunter-Killer origin, and a Herders mobile remembers captaining there. Other Storm Port mentions on a medallion, an orc, a traveler, or navy gear are references to the city/faction rather than another map. Followed WLD/MOB/OBJ/ZON/QST histories remain on `spshold` from the 2009 area-tree move; the 2026 WLD prose correction changes no exits, and its story sidecar was added in 2026. No alternate-stem predecessor or whole-zone replacement was found. The sole reviewed inactive-WLD approach remains Surf #285640 west to Stronghold #22634 under existing candidate row 97 **RETAIN IN PLACE**. Keep `spshold` **DO NOT MOVE**, preserve all candidate plans, and leave command-target alias ownership separate. See the [Storm Port Stronghold audit supplement](STORM_PORT_STRONGHOLD_AUDIT_REVIEW.md#checkpoint-754-storm-port-and-stronghold-identity-boundaries), [Storm Port title](../../../areas/zon/stormport.zon#L2), [Jade district](../../../areas/wld/jade.wld#L697), [Hunter-Killer reference](../../../areas/qst/surfacekeeps.qst#L546), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [coverage row 281](ZONE_COVERAGE_REGISTER.csv#L282).

### Checkpoint 755: The Depths of Duris and same-number retained maps

A normalized scan of all 442 current .zon titles finds one exact “The Depths of Duris” owner: active surfacekeeps. Other Duris-named titles belong to distinct title files, including “The Great Realm of Duris” (connectorzones), “Ailvio, Duris Newbie Outpost” (newbie), and “The Surface Realm of Duris” (surface); unlisted surf, ud, and underworld2 sources carry their own World/Underworld of Duris titles. The active SurfaceKeeps QST's Storm Port Stronghold clarification is a relationship to that separately rostered area, not an alternate Depths map.

Unregistered Duris3.wld declares the same 2,645 room VNUMs as active surfacekeeps.wld, but none of the paired titles match. Followed histories and current titles provide no explicit alternate-stem rename, predecessor, or whole-zone replacement. The exact map lineage remains builder question 9; preserve Duris3 under existing candidate row 36 RETAIN IN PLACE and do not treat its source-only exits as active ingress. Separate unregistered surfacekeeps_objects.obj repeats 54 active object VNUMs (39 identical and 15 differing prototypes), with stub-loading history; preserve it under candidate row 100 RETAIN IN PLACE. Keep active surfacekeeps DO NOT MOVE and leave command-target alias/owner disambiguation separate. See the [SurfaceKeeps audit supplement](SURFACEKEEPS_AUDIT_REVIEW.md#checkpoint-755-duris-title-families-and-retained-source-identity), [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), [object-stub candidate row 100](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L100), [title](../../../areas/zon/surfacekeeps.zon#L2), and [coverage row 286](ZONE_COVERAGE_REGISTER.csv#L287).

### Checkpoint 756: Surface Minizones related titles and source reference

A normalized scan of all 442 current .zon titles finds one exact “The Minizones of the Surface” owner, active surfacemini. The similarly worded “Mini Zones” (minizones) and “The Minizones of the Underdark” (udmini) belong to separately rostered areas at *57–59 and *885–886; “The Surface Realm of Duris” (surface) is separately listed at *5000–6600. Woodseer’s story sidecar refers to a hermit in the Surface minizones and assigns the salmon-steak return to that hermit’s zone; this is a cross-area story link, not another SurfMini title or map.

Tracked surfacemini histories remain on one stem, and the current title inventory shows no predecessor or explicit whole-zone replacement. Active and source-only same-type scans found no WLD/MOB/OBJ overlap; the all-inactive-WLD scan found no room overlap or approach. No SurfMini candidate row is in the attic register. Keep surfacemini DO NOT MOVE, preserve all existing candidate plans, and leave command-target alias/owner disambiguation separate. See the [Surface Minizones audit supplement](SURFACEMINI_AUDIT_REVIEW.md#checkpoint-756-related-minizones-titles-and-cross-area-story-reference), [Mini Zones title](../../../areas/zon/minizones.zon#L2), [Underdark Minizones title](../../../areas/zon/udmini.zon#L2), [Woodseer reference](../../../areas/story/woodseer.story.json#L18), and [coverage row 287](ZONE_COVERAGE_REGISTER.csv#L288).

### Checkpoint 757: Swamp One title semantics and Shadow Swamp source references

A normalized scan of all 442 current `.zon` titles finds one exact “Swamp” owner: active `swamp_one`, zone 260. “Shadow Swamp” is local room/reset wording for that contributor. Related current titles identify separate rostered areas: `swamp_two` (“The Lair of the Swamp Troll King”), `lizard` (“The Lizardman Swamps of Clavikord”), `pods` (“Pharr Valley Swamp”), and `lab` (“The Swamp Laboratory of Khul’Lor”). The broad substring result `bogentok` is titled “BogenTok” and is not a swamp-name match. The `qst/AREA` listing without a local QST file remains builder question 2, not evidence of a predecessor or replacement.

Unregistered `surf.wld` contains 22 “The Shadow Swamp Lake” room labels and room #267604 (“A Shadowy Swamp Filled with Rotted Trees”), with a source-only west exit to active `swamp_one` room #26001. Preserve that approach on Surf candidate row 97 **RETAIN IN PLACE**. The same number in `surface2011.wld` is a separate Storm Sea room with different title and exits; numeric coincidence does not establish shared identity. Followed `swamp_one` histories stay on the current stem, with no alternate-stem predecessor or explicit whole-zone replacement. Keep active `swamp_one` **DO NOT MOVE** and leave command-target alias ownership separate. See the [Swamp One audit supplement](SWAMP_ONE_AUDIT_REVIEW.md#checkpoint-757-semantic-title-and-source-name-comparison), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [coverage row 288](ZONE_COVERAGE_REGISTER.csv#L289).

### Checkpoint 758: Swamp Two related titles and cross-area king references

A normalized scan of all 442 current `.zon` titles finds one exact “The Lair of the Swamp Troll King” owner: active `swamp_two`, zone 261. Similar swamp titles belong to separate current contributors (`swamp_one`, `lizard`, `pods`, and `lab`); `troll_caves`, `troll_hills`, and `torture` are separately titled troll areas, and `gibber` is the Lair of the Gibberling King. Creature-specific Lair titles share only a generic word with the Swamp Two title.

Mobile #26006, “swamp troll king,” is defined by `swamp_one.mob`, while active `swamp_two.zon` resets that prototype at #26133. White Harbor’s dialogue calls him “my swamp king”; its structured story explicitly names The Lair of the Swamp Troll King and requests White Harbor object #55185, which Swamp Two resets on the king. Caves’ QST dialogue names the “king of swamp trolls,” and its optional sidecar return consumes Swamp One object #26013, which Swamp Two also resets on the king. These are cross-area story and typed item relationships; they do not establish another map. Keep Swamp One as mobile/object prototype owner, Swamp Two as map/reset owner, and White Harbor/Caves as their own contract owners. See the [Swamp Two semantic review](SWAMP_TWO_AUDIT_REVIEW.md#checkpoint-758-related-titles-and-cross-area-king-references), [White Harbor story](../../../areas/story/wh.story.json#L1043), [Caves story](../../../areas/story/caves_skelenak.story.json#L428), and [coverage row 289](ZONE_COVERAGE_REGISTER.csv#L290).

Followed same-stem histories remain unchanged: no alternate-stem rename, explicit predecessor, or whole-zone replacement is established. The missing local QST file remains builder question 2. No inactive-WLD approach or same-type collision was found; keep active `swamp_two` **DO NOT MOVE**, with no attic candidate plan change. Leave command-target alias ownership separate.

### Checkpoint 759: Tchan monastery names, character location, and Alterian identities

A normalized scan of all 442 current `.zon` titles finds one exact “The Monestary of Tranquility” owner: active `tchan`, zone 48. Storm Port's “Before a Small Monestary” (#22519) and “Inside a Small Monestary” (#22520) remain a separate active city room pair. Storm Port's reset table has no M reset for mobile #22428 there; Tchan's ZON instead imports that Storm Port mobile at #4809. The Storm Port story sidecar preserves the four-proof request under Storm Port and states that Tchan is currently stocked in the separate summit Monestary. Jade's Guardian Monestary and Trakkia's Rilla Monestary are separate contributors, not alternate Tchan maps.

Ship Yards mobile Ayden Stoneheart #43163 calls himself Tchan's last surviving disciple and says Tchan “has long since been lost.” The current Tchan reset and Storm Port sidecar give a different status/location; no source dates Ayden's prose. Record this as an unresolved character chronology question, not map-replacement evidence. Followed Tchan history documents a 2010 move to an icy summit but no alternate-stem rename, predecessor, or whole-zone replacement. See the [Tchan semantic review](TCHAN_AUDIT_REVIEW.md#checkpoint-759-cross-area-monastery-names-and-character-references), [Ship Yards dossier](../zone-stories/SHIP_YARDS.md), [Storm Port rooms](../../../areas/wld/stormport.wld#L2426), and [Tchan reset](../../../areas/zon/tchan.zon#L19).

The nine same-number Alterian rooms #4801–#4809 retain different room titles and text; its four source-only approaches remain on candidate row 8 **RETAIN IN PLACE**. Keep active `tchan` **DO NOT MOVE**. The source comparison adds no candidate approach or whole-zone replacement. See [candidate row 8](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L8), [coverage row 290](ZONE_COVERAGE_REGISTER.csv#L291), and [Alterian identity review](ALTERIAN_WILDERNESS_AUDIT_REVIEW.md#checkpoint-446-alterian-wilderness-map-and-prototype-identities).

## Checkpoint 760: Tundra regional labels and adjacent cold-area titles

The normalized scan of all 442 current `.zon` titles finds one exact “Tundra”
owner: active `tundra`, zone 137. Other cold-themed titles belong to separately
rostered areas, including Towering Glacier of Dayedenvale, IceCrag Castle,
Neverwinter Woods, Valley of the Snow Ogres, the Royal Mausoleum of Castle
IceCrag, and the City of Winterhaven. Followed WLD/ZON history still names no
distinct Tundra predecessor or replacement. See the [Tundra audit](TUNDRA_AUDIT_REVIEW.md#checkpoint-760-regional-wording-and-adjacent-cold-area-names)
and [active title](../../../areas/zon/tundra.zon#L2).

Surface's WLD repeats “Snow Capped Hills of the Tundra” across 1,065 room
descriptions. Surface QST/story content and its reset identify Grey Fang and
the Tundra-hunter paw/fang stories as Surface content; Grey Fang mobile #500095
resets in Surface room #543147. Black Pearl's QST uses “Tundra” regionally for
Verspin, while its story sidecar gives Derimous an intended Verspin destination
and its current ZON reset still places him in Black Pearl room #142200. Verspin
and Black Pearl have separate active area identities. The repeated phrase in
unlisted `fooquest.qst` and `raxquest.qst` does not establish another map or an
exact-copy relationship.

These name and story references do not establish an alternate Tundra map,
predecessor, or replacement. Keep active `tundra` **DO NOT MOVE**; preserve the
existing source-only Surf row 97 and Player Guilds row 84 plans **RETAIN IN
PLACE**. No candidate-specific plan or active approach count changes. Leave
runtime command-target alias ownership outside this title/source comparison.
See [Tundra source supplement](TUNDRA_AUDIT_REVIEW.md#checkpoint-760-regional-wording-and-adjacent-cold-area-names),
[candidate register row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97),
[candidate register row 84](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L84), and
[coverage row 318](ZONE_COVERAGE_REGISTER.csv#L318).

## Checkpoint 761: Turolopolis Zoo and Ruins identities

The normalized scan of all 442 current `.zon` titles finds one “Turolopolis
Zoo” owner, `turolzoo` zone 530. “The Ruins of Turolopolis” is a separate
current title and owner, `willem` zone 71. Their shared place name and
ruined-city context describe connected neighboring locations. The story
sidecars identify the Zoo as an approach to the ruined city and the ruined
city park as the Zoo's approach; the WLD map records the reciprocal Willem
#7123 ↔ Zoo #53000 boundary. The areas have distinct zone stems, titles, room
ranges and local story/quest sources. The followed same-stem Zoo WLD/ZON
history names no alternate-stem predecessor or whole-zone replacement.

This comparison resolves the broader name/adjacency question for these
Turolopolis titles without treating the Zoo as a replacement for Willem or
Willem as a replacement for the Zoo. Keep active `turolzoo` **DO NOT MOVE**,
preserve the reciprocal route and existing cage behavior, and add no attic
candidate row. Runtime command-target aliases and installed route qualification
remain outside this static review. See the [Zoo semantic review](TUROLZOO_AUDIT_REVIEW.md#checkpoint-761-zoo-and-ruins-of-turolopolis-identities),
[Ruins title/history review](TUROLOPOLIS_AUDIT_REVIEW.md#active-title-and-tracked-history-boundary),
[Zoo coverage row 319](ZONE_COVERAGE_REGISTER.csv#L319), and the
[Zoo retention review](ATTIC_MIGRATION_PLAN.md#checkpoint-406-turolopolis-zoo-retained-in-place).

## Checkpoint 762: Twin Towers Forest and The Twin Towers identities

The normalized scan of all 442 current `.zon` titles finds one “Twin Towers
Forest” owner: active `twin_towers_forest`, zone 135. “The Twin Towers” is a
separate title and contributor, active `ttowers`, zone 132. Their room ranges
are disjoint: Forest #13500–#13599 and Twin Towers #13200–#13298
([Forest roster](../../../areas/AREA#L76),
[Twin Towers roster](../../../areas/AREA#L74),
[Forest title source](../../../areas/zon/twin_towers_forest.zon#L2),
[Twin Towers title source](../../../areas/zon/ttowers.zon#L2)).

The maps and stories explicitly describe a connected forest-and-tower setting
with separate zone ownership. Forest QST text says the towers are reached by
the path through the forest; its WLD describes paths to the towers. The Twin
Towers sidecar says the tower zone is separate from the forest gardener's belt
and flower story and distinguishes its own master gardener
([master gardener](../../../areas/story/ttowers.story.json#L25)). Three reciprocal
WLD boundaries join the areas ([Forest QST](../../../areas/qst/twin_towers_forest.qst#L21),
[Forest route](../../../areas/wld/twin_towers_forest.wld#L1535),
[Twin Towers story](../../../areas/story/ttowers.story.json#L8),
[three boundary pairs](TWIN_TOWERS_AUDIT_REVIEW.md#active-wld-graph)). These
shared names and authored routes are adjacency evidence, not duplicate map or
replacement evidence.

Followed `twin_towers_forest.wld` and `.zon` histories reach the 2009-08-12
area-tree move (`642da5400`). The WLD has same-stem edits through 2013-11-13
(`71d731c5b`). The ZON history follows the same move and same-stem edits
through the 2015-10-13 quest-item flag pass (`f18992b8f`). No alternate-stem
rename, distinct predecessor, or whole-zone replacement appears in these
bounded paths. Keep active `twin_towers_forest` **DO NOT MOVE** and preserve
the three reciprocal boundaries; add no attic candidate row. Runtime
command-target alias ownership remains separate. See the [forest source
dossier](../zone-stories/TWIN_TOWERS_FOREST.md), [milestone review](TWIN_TOWERS_FOREST_MILESTONES.md),
[Twin Towers history review](TWIN_TOWERS_AUDIT_REVIEW.md#title-history-and-disposition),
and [coverage row 320](ZONE_COVERAGE_REGISTER.csv#L320).

## Checkpoint 763: Ugta title and source-only hometown grid

The normalized scan of all 442 current `.zon` titles finds one exact “Ugta”
owner: active area `ugta`, zone 391. Cross-area references use the name for
the homeland, people, and goods associated with that contributor; they do not
identify another active title or local Ugta map.

The unregistered `surf.wld` grid contains one “UGTA HOMETOWN” tile, #209431,
and repeated “The Ugta River” room text. Its south exit reaches source-only
Stone Road #209731, whose north exit targets active Ugta room #39335. Ugta's
active reciprocal Surface boundary remains a separate pair in the active
connectivity report. The Surf chain is candidate-to-active map context, not
active ingress or a second loaded Ugta map. Followed Ugta WLD/ZON histories
remain on the same stem from the 2009 area-tree move through the 2013 WLD and
2015 ZON changes; no distinct predecessor or whole-zone replacement appears
in those bounded paths.

Keep active `ugta` **DO NOT MOVE** and preserve the Surf map identity, its
source-only route, and the existing candidate row 97 **RETAIN IN PLACE**. Add
no Ugta attic candidate and no active ingress record. Runtime command-target
alias ownership remains separate. See the [Ugta source review](UGTA_STORY_REVIEW.md#checkpoint-763-title-and-regional-source-comparison),
[source-only hometown label](../../../areas/wld/surf.wld#L197920),
[candidate plan row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97),
[Ugta connectivity review](WORLD_CONNECTIVITY.md#checkpoint-410-ugta-boundaries-and-directed-access),
and [coverage row 323](ZONE_COVERAGE_REGISTER.csv#L323).

## Checkpoint 764: Undermountain names and neighboring maps

The normalized scan of all 442 current `.zon` titles finds one exact
“The Ruins of Undermountain” owner: active `undermountain`. The separately
rostered `underworld` and `underworld3` contributors share the title “The
Underworld”; active `underdark` has a separate Durian Underdark title.
`underworld2`, `ud`, and `ud_map1` carry other Underworld/Underdark titles as
unregistered source components, not full `areas/AREA` contributors. These
sources do not identify an alternate Undermountain owner, predecessor, or
replacement.

Undermountain room #92518 has a directed down exit to Underworld room #4557
with no direct reverse exit. The boundary is adjacency evidence between two
maps, not duplicate or replacement evidence. Other cross-area matches are
explicit consumer/lore references (IceCrag's quest and declared crate source,
Lortower's Grimlock prose) or local terrain descriptions in Desert City,
Nexus, and Mountain Tracks; none names another current Undermountain map.
Followed Undermountain WLD/ZON paths reach the 2009 area-tree relocation and
later same-stem edits, with no distinct predecessor or whole-zone replacement
in those bounded histories.

Keep active `undermountain` **DO NOT MOVE** and add no attic candidate row.
Runtime command-target alias ownership remains separate. See the
[semantic-name review](UNDERMOUNTAIN_AUDIT_REVIEW.md#checkpoint-764-semantic-names-and-map-identity),
[Undermountain source dossier](../zone-stories/THE_RUINS_OF_UNDERMOUNTAIN.md),
and [coverage row 324](ZONE_COVERAGE_REGISTER.csv#L325).

## Checkpoint 765: Unique Vault title and object ownership

The normalized scan of all 442 current `.zon` titles finds one exact
“The Unique's Vault” owner: active `unique`. Its source dossier describes a
small object-owner contributor with two inspection rooms and 84 object
prototypes. External G resets in Astral Main, New Cave City, and Swamp Two
consume those objects; typed object imports do not make the consuming maps
alternate owners of the Vault. Followed WLD and ZON histories remain on the
same stem from the 2009 area-tree move, with no distinct predecessor or
replacement in those bounded paths.

Keep `unique` **DO NOT MOVE** and add no attic candidate row. See the [zone
title](../../../areas/zon/unique.zon#L2), [source dossier](../zone-stories/UNIQUE.md),
and [coverage row 327](ZONE_COVERAGE_REGISTER.csv#L328).

## Checkpoint 766: Undead Outpost and Human Outpost identities

The all-442 title scan finds one exact “The Undead Outpost” owner: active
`unoutpst`. Active `outpost` is separately titled “The Human Outpost”; other
outpost-named files have distinct titles and source keys, including
unregistered components. The reciprocal Crypt boundary is an authored
connection between neighboring maps, not a duplicate or replacement
relationship. Followed `unoutpst` WLD/ZON histories reach the 2009 area-tree
move and show no distinct predecessor or replacement.

Keep `unoutpst` **DO NOT MOVE** and add no attic candidate row. Preserve the
Crypt and Surface boundaries. See the [Undead Outpost title](../../../areas/zon/unoutpst.zon#L2),
[Human Outpost title](../../../areas/zon/outpost.zon#L2), [Surface boundary](../zone-stories/UNDEAD_OUTPOST.md#L40),
[Crypt boundary](../zone-stories/UNDEAD_OUTPOST.md#L41), and [coverage row
328](ZONE_COVERAGE_REGISTER.csv#L329).

## Checkpoint 767: Valoisian Castle and Verspin references

The normalized scan of all 442 current `.zon` titles finds one exact
“The Valoisian Castle” owner: active `val`. Herders mobile prose names Verspin
and Valoisian Castle as separate possible origins for one character; it does
not assign both places the same map or owner. Verspin is its own active
contributor with a different title. Followed WLD/ZON history reaches the 2009
area-tree move; the 2016 steel-key reset addition is a same-stem update, not
replacement evidence.

Keep `val` **DO NOT MOVE** and add no attic candidate row. See the [Valoisian
Castle title](../../../areas/zon/val.zon#L2), [Herders reference](../../../areas/mob/herders.mob#L677),
[Verspin title](../../../areas/zon/verspin.zon#L2), [audit supplement](VALOISIAN_CASTLE_AUDIT_REVIEW.md#active-title-and-tracked-history),
and [coverage row 330](ZONE_COVERAGE_REGISTER.csv#L331).

## Checkpoint 768: Valinhaven and source-only map context

The all-442 title scan finds one exact “Valinhaven” owner: active `valinhav`.
Mush1 mobile prose identifies a svirfneblin as probably from that village; the
text is a provenance reference, not a second Valinhaven contributor. The
unregistered `ud_map1` grid has a source-only north exit from #1113221 to
Valinhaven #90800, already documented as candidate-to-active map context. It
is not active ingress or a replacement map. Followed Valinhaven WLD/ZON
histories reach the 2009 move and show no distinct predecessor or replacement.

Keep `valinhav` **DO NOT MOVE**, preserve the existing `ud_map1` candidate
plan, and add no Valinhaven candidate row. See the [Valinhaven title](../../../areas/zon/valinhav.zon#L2),
[Mush1 reference](../../../areas/mob/mush1.mob#L53), [source-only map review](UD_MAP1_AUDIT_REVIEW.md#authored-wld-graph),
and [coverage row 332](ZONE_COVERAGE_REGISTER.csv#L333).

## Checkpoint 769: Valley of Crushk neighboring titles

The normalized title scan finds one exact “Valley of Crushk” owner among the
442 current `.zon` files. Pharr Valley, Pharr Valley Swamp, Valley of the Snow
Ogres, and other similarly named valleys retain their own titles and source
keys. The reciprocal Pharr boundary and the unregistered Surf #232517 approach
are neighboring/source-only route evidence, not duplicate or replacement
maps. White Harbor's QST mentions “Crushk” in a list of places; it does not
identify another Valley owner. The followed WLD/ZON histories remain on the
same stem through the 2009 move and later content/metadata edits, with no
alternate-stem rename or whole-zone replacement.

Keep `valley_crushk` **DO NOT MOVE**, preserve Surf candidate row 97 **RETAIN
IN PLACE**, and add no Valley candidate row. See the [title and history
review](VALLEY_CRUSHK_AUDIT_REVIEW.md#title-and-bounded-history), [White Harbor
reference](../../../areas/qst/wh.qst#L1339), and [coverage row 333](ZONE_COVERAGE_REGISTER.csv#L334).

## Checkpoint 770: Vehicles title and scope

The all-442 title scan finds one exact lowercase `vehicles` title, owned by
active `vehicles`. Its source review describes one contributor spanning land
and flying mounts, ferry contracts, named vessels, and their shipboard crews.
These authored subsystems and the code that operates them do not establish
separate map owners or replacements. Followed WLD/ZON history remains on the
same stem from the 2009 area-tree move, with no distinct predecessor or
replacement in those bounded paths.

Keep `vehicles` **DO NOT MOVE** and add no attic candidate row. See the [zone
title](../../../areas/zon/vehicles.zon#L2), [vehicle scope question](../zone-stories/VEHICLES_BUILDER_QUESTIONS.md#L15),
[story review](VEHICLES_STORY_REVIEW.md#environmental-story-shape), and
[coverage row 336](ZONE_COVERAGE_REGISTER.csv#L337).

## Checkpoint 771: Verspin destination references

The normalized scan of all 442 current `.zon` titles finds one exact
“Verspin” owner: active `verspin`. The city story records a southern highway
approach and its return route. Black Pearl names Verspin as an intended
campaign destination, Desolate has a delegate from the city, and Herders
prose names it as a possible former home; each refers to the current city,
not to another map owner. Followed Verspin WLD/ZON histories reach the 2009
area-tree move and later same-stem changes, with no distinct predecessor or
replacement in those bounded paths.

Keep `verspin` **DO NOT MOVE** and add no attic candidate row. See the [zone
title](../../../areas/zon/verspin.zon#L2), [city route](../../../areas/story/verspin.story.json#L8),
[Black Pearl destination](../../../areas/story/blackpearl.story.json#L509),
[Desolate delegate](../../../areas/mob/desolate.mob#L927), [Herders reference](../../../areas/mob/herders.mob#L677),
and [coverage row 337](ZONE_COVERAGE_REGISTER.csv#L338).

## Checkpoint 821: Randomness and Duris3 keep distinct room identities

All 10,000 room numbers in active Randomness (#100000–#109999) also occur in unregistered Duris3.wld, but every paired title differs: Randomness uses “Unnamed,” and Duris3 uses “The Ocean.” The 602 non-overlapping source-only Duris3 directions into those numbers remain within its separately retained map source; they are not active Randomness ingress or evidence of a predecessor or replacement. Preserve active `random` **DO NOT MOVE** and Duris3 candidate row 35 / CSV line 36 **RETAIN IN PLACE**. See the [Randomness title/history review](../zone-stories/RANDOMNESS.md#checkpoint-821-encounter-selection-and-map-identity), [Randomness typed audit](RANDOMNESS_AUDIT_REVIEW.md#checkpoint-821-current-typed-dependency-closure), [Duris3 audit](DURIS3_AUDIT_REVIEW.md#checkpoint-821-exact-overlap-with-active-randomness), and [candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

## Checkpoint 877: Fateful Falls keeps its `beggin` source identity

The normalized title “Fateful Falls” appears only in current `beggin.zon`. No other tracked WLD source defines room #3300 or #3301, and no other MOB/OBJ file defines either same-type prototype VNUM. Followed WLD, MOB, OBJ, and ZON paths reach the 2011 addition commit `de80d800e` (“new zone stub”); the QST path begins in 2015, and the OBJ path has a later text cleanup in 2016. The bounded source-path and current-owner review identifies no alternate-stem predecessor, same-type replacement map, or explicit retirement/successor record. The numeric 3300 matches in C/C++ resolve to a dexterity table value and a random-instrument category constant, not the local Tome object. Keep unregistered `beggin` as a distinct two-room source candidate **RETAIN IN PLACE** under [candidate row 14](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L15). See the [Fateful Falls audit](FATEFUL_FALLS_AUDIT_REVIEW.md#title-history-and-disposition), [source dossier](../zone-stories/FATEFUL_FALLS.md), and [coverage row 366](ZONE_COVERAGE_REGISTER.csv#L366).

### Checkpoint 900: Halfcut Hills source-only reciprocal edge and history boundary

The unregistered halfcut_hills room #27001 has a north exit to #256031.
Unregistered surf.wld room #256031 has a south exit back to #27001, an exact
numeric reciprocal across the two source files. The competing
surface2011.wld definition of #256031 instead exits south to #256331. This
suggests a possible source pairing but does not establish shared ownership,
load state, a followed rename, or a whole-zone replacement. Active
halfcut.wld continues to use the separate reciprocal Surface boundary
#27001/#629274. Preserve active halfcut and inactive halfcut_hills as
distinct identities pending map-owner evidence. See the
[Halfcut audit](HALFCUT_HILLS_AUDIT_REVIEW.md#checkpoint-900-source-only-reciprocal-edge-and-typed-dependency-closure),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-900-halfcut-hills-reconciles-source-only-reciprocal-route),
and [builder questions](HALFCUT_HILLS_BUILDER_QUESTIONS.md).

### Checkpoint 901: Hostel parallel low-VNUM identities

Checkpoint 901's all-source exact same-type scan found additional matching-ID
counts beyond active Limbo and Heavens. WLD overlaps occur in unregistered
Astral (1), Dalvik (102), Magetower (71), Mavulsk (102), Mentiri (72),
Mtncastl (1), Nexus_1 (40), Theurgist (4), ud-inside (1), and Waterplane
(12). MOB overlaps occur in Dalvik (3), Magetower (23, including #18),
Mavulsk (66), Mentiri (30), Theurgist (14), and Waterplane (3). OBJ overlaps
occur in Dalvik (63), Magetower (20), Mavulsk (76), Mentiri (20), and
Theurgist (3). These are typed numeric overlaps in separate unregistered
source files; this scan did not establish identical records, common ownership,
or a replacement chain.

The established active overlap remains 45 of 102 WLD IDs, 65 of 66 MOB IDs,
and all 76 OBJ IDs, with each active same-number record differing from the
Hostel record. Candidate mobile #18 has no active same-number mobile, though
unregistered Magetower and Mavulsk have their own mobile #18 source identities.
Candidate OBJ #41/#72 remain the twisted key/dragon-scale leg armor while
active Heavens #41/#72 remain the guild badge/fountain. Direct code, active
reset, QST, and shop roles stay bound to current active identities; notably,
active Heavens mobile #46 is Xena with an `epic_teacher` assignment, while
Hostel mobile/QST #46 is a different source record and active Heavens QST #46
is empty. No follow-up history evidence changes the CP489 conclusion: no
Hostel rename, same-stem migration, or explicit whole-area replacement was
found in the bounded history. Keep every source identity separate and
**RETAIN IN PLACE** pending owner review. See the
[Hostel typed audit](HOSTEL_AUDIT_REVIEW.md#checkpoint-901-typed-cross-source-closure),
[24 builder questions](HOSTEL_BUILDER_QUESTIONS.md), and
[coverage row 402](ZONE_COVERAGE_REGISTER.csv#L403).

### Checkpoint 902: IC3 remains a distinct high-VNUM source

The exact same-type scan across current WLD, MOB, and OBJ source definitions
found no external record in IC3's #420000–#420053 room, #420000–#420020 mobile,
or #420000–#420049 object ranges. This supports the existing no-collision
finding in the current manifests but does not clear editor, deployment, or
persisted-state ownership. The April 6 removal of `IC3 *4200-4300` from
`areas/AREA` and April 7 Rice Fields entrance edit remain explicit de-listing
and route-change history; neither records a whole-area replacement. The
September 14, 2026 `IC3.obj` edit remains later source maintenance after
de-listing and still requires an owner explanation. The `420000` raw matches
in Caertannad are currency, while the untyped `nexus.mob` tuple remains open;
neither establishes IC3 identity or replacement. Preserve IC3 as a distinct
**RETAIN IN PLACE** source. See the
[IC3 role audit](IC3_AUDIT_REVIEW.md#checkpoint-902-typed-cross-source-role-refresh),
[24 builder questions](IC3_BUILDER_QUESTIONS.md), and
[coverage row 403](ZONE_COVERAGE_REGISTER.csv#L404).

### Checkpoint 903: Infernal keeps Harrow and sibling identities separate

Checkpoint 903 reclassifies the overlapping WLD, reset, QST, story, shop, and
code values by source role without changing the CP491 history conclusion.
Harrow's active WLD/MOB/OBJ records remain the current same-number owners;
unregistered `dark`, `darkop`, and `sky_gh` retain their own same-number WLD,
MOB, OBJ, and ZON values. The two Surface exits and source-only Surf exit to
#29400 resolve to active Harrow, and the candidate's own three D4 exits remain
one-way source references to Duris3. They do not prove shared authorship or
replacement.

Active Harrow QST and story-sidecar item fields #29400/#29402, active shop
keeper/room/product roles, and `real_room0(29403)` also bind to the active
Harrow identities. The separate `dark.shp`/`infernal_gh.shp` #29401 records
remain competing unlisted shop inputs under the conditional fallback, not
evidence that one zone replaces the other. Preserve Harrow's code, route,
quest, story, and shop consumers and keep all unregistered sources **RETAIN IN
PLACE** pending owner/history decisions. See the
[Infernal role audit](INFERNAL_GH_AUDIT_REVIEW.md#checkpoint-903-typed-cross-source-role-closure),
[30 builder questions](INFERNAL_GH_BUILDER_QUESTIONS.md), and
[coverage row 404](ZONE_COVERAGE_REGISTER.csv#L405).

### Checkpoint 911: Magetower retains separate low-VNUM map identities

The CP500 active overlaps remain Limbo room #1 and Heavens rooms #3–#5,
#10–#46, #66, and #69; Limbo mobiles #1–#6 and Heavens mobiles #7–#17 and
#19–#23; and Limbo objects #1–#9 and Heavens objects #10–#20. The all-source
WLD scan adds same-number room definitions in unlisted Dalvik, Hostel,
Mavulsk, Mentiri, Nexus_1, Theurgist, Waterplane, Astral, Mtncastl, and
ud-inside. Dalvik, Hostel, Mavulsk, and Mentiri each define all 71 candidate
room numbers. These are separate source files and do not show a common map
identity or replacement.

Same-type mobile definitions also occur in unlisted Dalvik (#1–#3), Hostel,
Mavulsk, and Mentiri (#1–#23), Theurgist (#1–#14), and Waterplane (#1–#3).
Same-type object definitions occur in unlisted Dalvik, Hostel, Mavulsk, and
Mentiri (#1–#20), and Theurgist (#1–#3). Keep these alongside the separate
areas_mini overlap record; source-number equality does not establish body
identity, active ownership, or shared authorship.

Role-aware QST, story, ZON, shop, portal, and compiled-reference scans tie
active item, giver, reset, and code roles to current Limbo/Heavens records.
Unlisted Dalvik, Hostel, Mavulsk, and other matching ZON sources retain their
own same-number records. The 869 unlisted WLD targets into #1–#71 are 868
source-local directions plus one Surf target to active Heavens #36; none
changes Magetower's active-inbound count. CP500's bounded histories still show
no direct Magetower rename, retirement, or whole-area replacement decision.
Retain Magetower and its 71-position coordinate file separately until map,
editor, and export owners resolve the overlapping identities. See the
[Magetower role-aware audit](MAGETOWER_AUDIT_REVIEW.md#checkpoint-911-role-aware-low-vnum-source-closure),
[25 builder questions](MAGETOWER_BUILDER_QUESTIONS.md), and
[coverage row 413](ZONE_COVERAGE_REGISTER.csv#L414).

### Checkpoint 912: map1 remains an empty path artifact

The only current `component:map1` source is the zero-byte
`areas/mob/map1.mob`; there are no room, mobile, object, or quest VNUM records
to collide. The exact path scan found no source reference to that MOB path.
`MAP1_FILE`/`MAP1A_FILE` are information-file constants and the `ud_map1`
WLD/MOB/OBJ/ZON package has a separate source identity. Similarity-based
zero-byte history still proves neither common purpose nor a rename or whole-
area replacement. Preserve the tracked empty path pending owner disposition.
See the [map1 audit](MAP1_AUDIT_REVIEW.md#checkpoint-912-exact-path-reference-refresh),
[10 builder questions](MAP1_BUILDER_QUESTIONS.md), and
[coverage row 414](ZONE_COVERAGE_REGISTER.csv#L415).

### Checkpoint 913: map22 reference closure does not establish a replacement

The all-WLD scan found no external definition or target in map22's
#900000–#909999 range. The code-created Duris3 #145443 to map22 #905050 link
remains a conditional type-1 random-lab relationship, not an authored WLD
route or evidence that either map replaces the other. Four external object
records store raw #900000 values, but their types are 10, 20, 8, and 8 rather
than portal/switch destinations. Code-selected object #68 remains a separate
identity question: active Heavens and unlisted Hostel/Mavulsk define it, with
Hostel and Mavulsk resetting their own prototypes. No new map identity,
replacement, or history evidence was found. Keep the separate
**RETAIN IN PLACE** decisions and owner questions. See the
[map22 role-aware audit](MAP22_AUDIT_REVIEW.md#checkpoint-913-role-aware-random-lab-dependency-closure) and
[candidate row 64](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L65).

### Checkpoint 914: map5 mobile and Duris3 room numbers remain distinct identities

Each of the 40 map5 mobile VNUMs also defines one room in unlisted
`Duris3.wld`; the match crosses MOB and WLD namespaces. No other tracked MOB
source defines those mobile VNUMs, and the WLD records do not establish a
map5 room identity, loaded relationship, rename, or replacement. `surf.zon`
adds 45 source-only `M` placements and nine `F` follower rows for map5 mobiles
in the separate Surf source family. Preserve map5, Duris3, and Surf as distinct
sources under their current retention decisions. See the
[map5 role-aware audit](MAP5_AUDIT_REVIEW.md#checkpoint-914-map5-cross-namespace-role-closure) and
[candidate row 65](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L66).

### Checkpoint 915: map8 path scan does not resolve empty-file identity

The bounded case-insensitive token and exact-path searches across `areas/`,
`src/`, `scripts/`, and `tests/` found no in-checkout map8 reference. The
zero-byte `map8.mob` still has no numeric identity. Empty-file rename-follow
matches through the historical `Players/` paths do not connect map8 with
`map1.mob`, and the separately named `oldmap8.wld` remains a distinct source.
No rename, retirement, replacement, or cleanup authorization follows. See the
[map8 path-reference audit](MAP8_AUDIT_REVIEW.md#checkpoint-915-exact-path-reference-refresh) and
[candidate row 66](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L67).

### Checkpoint 916: Mara Guildhall exact VNUM closure remains source-local

The exact #29600–#29617 WLD, MOB, OBJ, ZON, and SHP scan adds no third source
identity: existing overlap remains confined to unlisted `mara_gh` and
`mara_op`. QST/story sources contain no candidate-range reference, and the
only compiled hit is the existing room #29605 inn binding. No new rename,
retirement, or replacement evidence was found. Keep the two components and
their conflicting room/mobile/object definitions separate under their
existing **RETAIN IN PLACE** plans. See the
[Mara Guildhall role-aware audit](MARA_GH_AUDIT_REVIEW.md#checkpoint-916-exact-vnum-role-closure-refresh),
[Mara Outpost candidate row 68](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L69), and
[Mara Guildhall candidate row 67](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L68).

### Checkpoint 917: Mara Outpost exact VNUM scan confirms two source identities

The exact #29610–#29612 scan adds no third room, mobile, object, reset, or
shop owner. Same-number records remain confined to unlisted `mara_op` and
`mara_gh`, with the distinct room flags, mobile stats, and portal fields
already recorded. The #29610 ITEM_TELEPORT value to Mara Guildhall room
#29608 is a typed object reference, not replacement or WLD-ingress evidence.
No new rename, retirement, or replacement finding was established. Preserve
both source families separately. See the
[Mara Outpost role-aware audit](MARA_OP_AUDIT_REVIEW.md#checkpoint-917-mara-outpost-exact-vnum-role-closure),
[Mara Guildhall row 67](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L68), and
[Mara Outpost row 68](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L69).

### Checkpoint 918: Mavulsk role matches do not establish identity or replacement

The active Heavens reset for mobile #2 still resolves by number to active
Limbo #2; Mavulsk #2 and Mentiri #2 are separate unlisted definitions.
Active ZON
object arguments, QST item numbers, shop stock values, story `item_vnums`, and
17 type-25 destinations overlap Mavulsk’s low VNUM ranges, but none supplies
a Mavulsk source binding. The six active teleport destination numbers also
occur in other unlisted WLDs and have no active room definition. This scan adds
no rename, predecessor, retirement, or whole-area replacement evidence.
Preserve the candidate identities separately. See the
[Mavulsk role-aware audit](MAVULSK_AUDIT_REVIEW.md#checkpoint-918-mavulsk-typed-cross-source-role-closure),
[MV-14](MAVULSK_BUILDER_QUESTIONS.md), and
[candidate row 69](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L70).

### Checkpoint 919: Mentiri cross-source roles do not establish lineage

The exact role scan leaves active and unlisted identities separate: Heavens
M #2 resolves to current Limbo mobile #2; ZON object numbers #5/#17 resolve
to active Limbo/Heavens prototypes; and #2/#9/#50 type-25 destinations have
no active WLD owner. QST, N-format shop, and story item numbers likewise
resolve by their current active source records or remain source-only. No
Mentiri alias, alternate-stem rename, retirement, predecessor, or replacement
evidence was found. Preserve Mentiri and its editor map JSON as separate
source identities. See the
[Mentiri exact role audit](MENTIRI_AUDIT_REVIEW.md#checkpoint-919-mentiri-exact-cross-source-role-closure),
[MT-15 and MT-17](MENTIRI_BUILDER_QUESTIONS.md), and
[candidate row 70](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L71).

### Checkpoint 920: Monsteri cross-source roles remain distinct

Monsteri and cbl_outpost2 keep their separate four-mobile and five-room
prototype sets; their matching local M reset rows and room targets do not
establish precedence. Active Heavens mobiles #1000–#1004 remain different
records, including Heavens mobile #1004 at the number used by Monsteri room
#1004. Menden’s active ZON G and N-format shop stock references use object
#1000, whose current active owner is Heavens OBJ #1000; Monsteri OBJ is empty.
Those role matches add no Monsteri lineage, alias, predecessor, retirement,
or whole-zone replacement evidence. Keep the candidate, cbl_outpost2, and
active Heavens identities separate. See the
[Monsteri exact role audit](MONSTERI_AUDIT_REVIEW.md#checkpoint-920-monsteri-exact-cross-source-role-closure),
[MON-15 and MON-23](MONSTERI_BUILDER_QUESTIONS.md), and
[candidate row 71](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L72).

### Checkpoint 921: moriatst roles confirm duplicate source, not lineage

CP921 confirms that the three candidate shop fields point to active Moria
keeper mobiles, rooms, and stock objects. Moria ZON resets place the same
keepers in those rooms, and Moria-specific mobile callbacks bind the three
keepers by VNUM. This confirms the typed active owners while leaving source
precedence unresolved. Candidate shop #99037 also retains a 1.00 sell factor
versus active Moria's 1.10; that effective difference does not prove which
shop file is canonical. The conditional fallback still exposes duplicate
keeper/room records and reset-side selection ambiguity. No rename, retirement,
predecessor, or replacement evidence was found. Keep the files and current
Moria prototypes separate. See the
[moriatst exact role audit](MORIATST_AUDIT_REVIEW.md#checkpoint-921-moria-role-bindings-and-shop-factor-correction),
[MST-06, MST-07, and MST-09](MORIATST_BUILDER_QUESTIONS.md), and
[candidate row 72](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L73).

## Checkpoint 922: Mountain QST room-number overlap is not lineage

The unregistered Mountain QST's five mobile-header numbers and seven
unresolved item numbers coincide with WLD room numbers in active
mountaintracks. Current Mountaintracks resets use other mobile VNUMs in those
rooms, and there are no current MOB or OBJ definitions for the QST's typed
references. This cross-namespace number overlap establishes neither that
Mountain.qst belongs to mountaintracks nor that it is related to active
lowercase mount. The existing Mountain.qst history still shows no rename,
replacement, or owner relationship. See the
[Mountain QST role audit](MOUNTAIN_QST_AUDIT_REVIEW.md#checkpoint-922-typed-collision-with-active-mountaintracks-rooms).

## Checkpoint 923: mtncastl history and placeholder references

Followed history now covers all four mtncastl paths: WLD, MOB, OBJ, and ZON.
The tracked paths show the area-tree moves and same-stem updates but no
explicit rename to a successor or whole-zone replacement. The one-room WLD
is byte-identical to the #1 placeholders in astral and ud-inside, and its ZON
title is shared by four other current files; neither match determines an
owner. Numeric room #1 references also remain distinct from mtncastl: active
Limbo owns the loaded WLD VNUM, nine active Desert directions resolve there,
and the other fourteen WLD directions and four ZON room-field commands are
source-only. Preserve the current separate identities pending owner/editor
evidence. See the
[mtncastl exact role audit](MTNCASTL_AUDIT_REVIEW.md#checkpoint-923-exact-room-1-dependency-and-role-closure).

## Checkpoint 924: Nexus56 switch and shop VNUMs follow active Alatorin owners

Nexus56's chair #83148 is an ITEM_SWITCH targeting source room #83638. The
same-number active Alatorin object #83148 is a mithril sign, while active
Alatorin room #83638 is “Atop a High Mountain Rise” and Nexus56's room is
“The Central Audience Chamber.” Candidate shop #83184's keeper number also
resolves to an active Alatorin mobile rather than candidate Ctanmir; 18
other candidate shop keepers have no active prototype. These typed role and
room-content conflicts establish no alias, replacement, or shared editor
identity. Active Alatorin SHP record #83343 uses #83343 as its shop ID and
keeper, but places the shop in room #83681; the number does not identify the
candidate-only Nexus56 room #83343. Preserve the two source identities
pending owner evidence. See the
[Nexus56 typed room-role audit](NEXUS56_AUDIT_REVIEW.md#checkpoint-924-exact-room-role-endpoints-and-shop-fallback-bindings).

## Checkpoint 925: Player Guilds mobile roles do not establish replacement

The follow-up confirms that four Player Guilds keeper VNUMs also identify
mobiles used by current zone reset sources: Drst #34202, Longhollow and
Temple2 #34402, Newhaven #35252, and unlisted Asunwa #35300. Their reset
commands place those mobiles in other rooms than the corresponding Player
Guilds shop `in_room` values. The #34402 source has two different mobile
prototypes; this is an identity collision requiring owner and loader-order
resolution, not evidence that the shop catalog replaces either area.

Shop #35300 remains the only same-number Player Guilds shop-header record
found in another SHP fragment: `ddguild.shp` shares its Asunwa keeper/room
#35300/#35303 while retaining different stock and flags. The new reset
evidence does not add a successor, rename, or retirement decision to the
bounded history. Keep the files separate pending ownership and deployment
evidence. See the
[Player Guilds typed reset audit](PLAYER_GUILDS_AUDIT_REVIEW.md#checkpoint-925-typed-reset-and-numeric-role-closure).

## Checkpoint 926: Ravenmis shared numbers do not establish lineage

The typed follow-up found nine active WLD exits into the #29300–#29309
number range, all targeting active Newbie rooms. Six ITEM_TELEPORT objects
also point to Newbie room numbers in that range; four portal sources are
unlisted. Active Newbie QST/story sources use #29300/#29302 as item rewards
and #29304 as a mobile, while Ravenmis uses those numbers for different room,
object, and mobile identities. Other unlisted guildhall SHP/ZON sources reuse
several of the same numbers. These cross-source role collisions do not add
historical predecessor, successor, rename, or retirement evidence. Keep the
five map files separate pending owner and deployment provenance. See the
[Ravenmis typed role audit](RAVENMIS_AUDIT_REVIEW.md#checkpoint-926-shared-room-numbers-resolve-to-newbie-identities).

## Checkpoint 927: Raxquest's overlapping type IDs do not establish lineage

Raxquest's WLD, MOB, and OBJ sources reuse 62 numbers across all three typed
tables, with more room/object overlaps above the mobile range. The exact
source scan found no standalone 705xx reference outside the five Raxquest
files under `areas/`; the remaining literals are the typed callbacks and
dedicated mobile behavior in `specs.assign.c` and `specs.raxquest.c`. These
references confirm code/data coupling but do not identify an alternate
source owner, a whole-zone successor, or a replacement. The previously
established Black Pearl OBJ copy and normalized QST reuse remain file-level
lineage findings only. Keep Raxquest and Black Pearl identities separate
pending owner and loader evidence. See the
[Raxquest typed-role audit](RAXQUEST_AUDIT_REVIEW.md#checkpoint-927-cross-type-ids-keep-separate-source-and-code-roles).

## Checkpoint 928: realms_master board objects do not replace shop roles

The four `realms_master.shp` header, keeper, and room numbers also identify
Player Castles OBJ bulletin boards. Five source ZON O commands consume the
two keeper-number objects, while compiled ship code uses #11108 as a mobile
helper and the matching room callbacks are disabled. These typed number
matches neither supply the candidate's missing keeper/room prototypes nor
establish that Player Castles replaces the shop file. The followed SHP
history still has no explicit predecessor, successor, or retirement
decision. Keep the separate shop and object identities pending owner and
deployment evidence. See the
[Realms Master role audit](REALMS_MASTER_AUDIT_REVIEW.md#checkpoint-928-shop-keeper-room-and-board-numbers-remain-typed).

## Checkpoint 929: returns VNUMs have no external source identity match

The current exact-value scan finds #93400 and #93401 only in
`returns.mob`; no other source prototype or typed consumer uses either
number. The two descriptions and filename still provide no tower, map, or
shared-story identity, and the followed history remains limited to the
area-tree move. The expanded scan adds no alias, predecessor, successor,
retirement, or replacement evidence. Preserve the two mobile identities
pending owner and deployment provenance. See the
[returns role audit](RETURNS_AUDIT_REVIEW.md#checkpoint-929-both-mobile-vnums-remain-isolated-in-current-sources).

## Checkpoint 930: rf_gh's reciprocal portals do not resolve its Torg collisions

rf_gh OBJ #28903/#28904 form an authored ITEM_TELEPORT pair between rooms
#28905 and #28912, but listed Torg owns different same-number OBJ prototypes
for both portals and different WLD rooms at both destinations. The teleport
fields clarify a possible traversal between rf_gh's two WLD components; they
do not make the same-number Torg objects or rooms part of the candidate.
The rf_op cathedral-fragment reuse and changed portal destinations remain a
separate file-level relationship, not proof that either source replaces the
other. Keep the identities separate pending map/object and loader ownership.
See the [rf_gh portal audit](RF_GH_AUDIT_REVIEW.md#checkpoint-930-reciprocal-portal-objects-cross-the-two-wld-groups).

## Checkpoint 931: rf_op type and source roles remain distinct

The exact scan confirms rf_op #29510 and #29511 each have separate WLD,
MOB, and OBJ prototypes, while #29512 is its WLD room and an OBJ reset or
teleport value. Different WLD prototypes with these numbers occur in
unlisted aod_gh, genocide_hall, and legion; unlisted Legion OBJ #29500 also
targets its own WLD #29512. The scan found no external MOB/OBJ prototypes,
QST, SHP, story-sidecar, or C/C++ references establishing another owner.
These number overlaps and the copied rf_gh scene do not establish a shared
identity, predecessor, successor, or replacement. See the
[rf_op typed-role audit](RF_OP_AUDIT_REVIEW.md#checkpoint-931-same-number-records-keep-separate-typed-roles).

## Checkpoint 932: #6089 consumers do not identify one Scrubs successor

The checked-in source tree has no OBJ prototype #6089, while 40 SHP stock
rows in 33 files and 33 ZON G-reset rows in 31 files use the value. Some
consumers are full-manifest members; most belong to unlisted source files.
Active Lizard WLD room #6089 is a different prototype type and gives no
evidence about an object owner. This shared numeric dependency, like the
recent Scrubs stock edits, establishes no same-type predecessor, successor,
or replacement. See the
[scrubs_hall #6089 audit](SCRUBS_HALL_AUDIT_REVIEW.md#checkpoint-932-product-6089-is-a-shared-unresolved-source-reference).

## Checkpoint 933: Player Castles item uses do not join Shadow Hall identities

Player Castles owns the current OBJ #11060 magic mouth and #11110 bulletin
board prototypes. Their five ZON O uses are in shadow_hall, bastard, and
sky_gh; only Player Castles itself is listed among those source stems.
Object #11060 is code-bound to `magic_mouth`. Numeric #11110 also occurs in
mobile-helper lists, but there is no checked-in MOB #11110; that code role
does not merge the bulletin board with a mobile or establish a map lineage.
These references add no predecessor, successor, or replacement evidence.
See the
[shadow_hall import audit](SHADOW_HALL_AUDIT_REVIEW.md#checkpoint-933-player-castles-imports-have-separate-code-consumers).

## Checkpoint 934: shady2 item and mobile numbers remain type distinct

The active Shady and Alatorin quest/story item references to #97514,
#97549, #97570, #97572, and #97582 resolve to active Shady OBJ records.
Those numbers overlap the `shady2.mob` range but do not establish a
same-type duplicate or lineage. #97549 is also a baby-orc MOB in both
active `shady.mob` and unlisted `shady2.mob`; this type collision does not
turn the Alatorin sword-item contract into a mobile reference. Preserve the
mobile records and item contracts as separate typed identities. No
predecessor, successor, or replacement decision is established. See the
[shady2 typed-reference audit](SHADY2_AUDIT_REVIEW.md#checkpoint-934-quest-and-story-item-ids-remain-object-typed) and [Shady Grove supplement](SHADY_GROVE_AUDIT_REVIEW.md#checkpoint-934-97549-is-an-object-and-mobile-vnum).

## Checkpoint 935: sky_gh and Harrow field types do not imply lineage

Harrow story `mob_vnum: 29404` identifies active Harrow's Lomya mobile;
the same narrative contract uses #29404 as an OBJ reward. Harrow also has
a room and object at that number, while `sky_gh` defines its own Armory
room and shop location there. The shared value does not establish a common
identity or replacement between the active Harrow and unlisted sky_gh
records. Preserve source and field types separately pending owner decisions.
See the [sky_gh typed-role audit](SKY_GH_AUDIT_REVIEW.md#checkpoint-935-harrow-contact-and-item-roles-stay-separate).

## Checkpoint 936: Surf mobile and old Surface QST identities stay separate

The unlisted `surface2011.qst` and `underdark2011.qst` contain QST headers
and item operands whose numbers overlap `surf.mob` #200000–#200321. Their
QST files are absent from `areas/qst/AREA`; the item fields resolve to
source OBJ records in their own unlisted map families, while Surf has no
OBJ prototypes. Header and item roles do not establish that Surf is the
intended QST owner or that any of these source families replaces another.
The 37 Surface2011 giver keys and one UnderDark2011 giver key resolve within
their own MOB sources; their 56 and four item operands resolve within their
own OBJ sources. Preserve all three identities pending owner and loader
provenance. See the [Surf typed-boundary audit](SURF_AUDIT_REVIEW.md#checkpoint-936-old-surface-qst-values-do-not-identify-surf-mobiles), [Surface2011 reciprocal review](SURFACE2011_AUDIT_REVIEW.md#checkpoint-936-surf-range-qst-values-retain-their-source-owned-types), and [UnderDark2011 reciprocal review](UNDERDARK2011_AUDIT_REVIEW.md#checkpoint-936-underdark2011-qst-values-retain-their-source-owned-types).

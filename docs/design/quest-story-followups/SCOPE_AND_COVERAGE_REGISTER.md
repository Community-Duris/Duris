# Scope reconciliation and per-zone coverage register

**Checkpoint 116 — source inventory, static-exit census, replacement-history
pass, twenty-four new zone story dossiers, and the Temple of the Sun cross-zone
reconciliation; repository-wide audit supplements
for Mistywood, Darkfall, Kimordril, Shady Grove, Apocalypse Castle,
Ixxillikor, Moonshae Island, Prison of Fort Boyard, Lair of the Purple Worm,
Vargan, The Maze of Undead Army, Bandit Camp, Myrloch Vale, Cloud Giant Kingdom,
The Mountain of the Banished, Tiamat, and The Chasm of the Misty Vale;
Bugentol's source-comprehensive review, and Jotunheim, Ixarkon, Dirk’nspire, Troll Caves,
IceCrag, Cloister, Turolopolis,
Du'Maathe Castle, Tundra, The Fields Between, The Town of Moregeeth, and
Ceothia, Braddistock Mansion, The Desert City of Venan'Trut, Past Ceothia, The
Basin Wastes, Nakral's Crypt, The Valoisian Castle, Harrow, Mountain Tracts,
The Orcish Slave Camp, The Underground Lava Caves, The Cimmerian Nomad
Encampment, The Ruins of Undermountain, Desolate Under Fire, Storm Port
Stronghold, The Mountain Settlement of the Harpies, The Behemoth Herders,
The Great Shaboath, The Altar of the Firesworn, The Royal Mausoleum of
Castle IceCrag, Negative Material Plane, The Prisons of Carthapia, and Pits
of Cerberus
repository-wide audit supplements,
and source audit supplements
for Jotunheim, Temple of Flames,
Pharr Valley Swamp, The Citadel,
The Elemental Groves, Temple of the Earth, Githzerai Stronghold, The Caverns of
the Worms, Castle Ravenloft, The Realm of Barovia Continued, Village of Werrun,
The Village of New Hope, Drustl's Yerdonia Enslaved, The Arcaneum of
L'srillizzin, Mistywood, Pharr Valley, Tribal Oasis, The Great Realm of Duris,
The Battlefield, The Forgotten Mansion, Woodseer, Village of Refugees,
The Twin Towers, The Deep Ravine of Passage, Neverwind Valley, Grumbar's
Domain, The Temple to Skrentherlog, Caves of Mt. Skelenak, Southern Coastal
Highway, Labyrinth of No Return, Khildarak Stronghold, Strathor Valley of the
Storm Giants, The Gagga'Jobo Cave System, The Tharnadian Ruin, and The Forest
City of Aravne.**
This is not a completed
game-wide environmental-story, dynamic reachability, typed-dependency, or
migration audit. The row-level statuses in
[`ZONE_COVERAGE_REGISTER.csv`](ZONE_COVERAGE_REGISTER.csv) distinguish existing
comprehensive source dossiers from pending story reviews and record which
connectivity/history layers have been inventoried. The builder-question review
column separately tracks explicit, evidence-backed owner questions and
recommended resolutions for every registered source key.

## Snapshot and denominator

- Native source snapshot: commit
  `72ba3deff3bb30bde880018cdebe7c6db675b697` (`docs: add quest story follow-up
  research`), based on `2d6f5c96083351f0234b06be7c80bc7c87d6fb20`.
- Full world configuration: generated normal `areas/world.*` files, built from
  the un-commented entries of top-level [`areas/AREA`](../../../areas/AREA).
- Minimal development configuration: the separate checked-in
  [`areas_mini`](../../../areas_mini) aggregates selected by `--minimal`.
- Quest catalog configuration: `areas/AREA` plus existing `areas/qst/<key>.qst`
  sources. `areas/qst/AREA` is not the production list.
- Random-zone configuration: the independent [`areas/RANDOM_AREA`](../../../areas/RANDOM_AREA)
  input consumed by random-zone generation. It currently has zero entries.

The reconciled counts below describe different layers and must not be collapsed
into “the number of game zones.”

| Reconciled measure | Count | What it means |
| --- | ---: | --- |
| Un-commented top-level `areas/AREA` entries | 351 | Full-world area-file contributors, including administrative/shared sources, `limbo`, and the `end` marker. All 351 have `.wld`, `.mob`, `.obj`, and `.zon` files. |
| Nonnegative rows in the quest catalog registry | 350 | `zone_story_quest_catalog.py` omits negative zone number `limbo`; it still includes the special `end` record. This is a catalog denominator, not a count of ordinary reachable adventures. |
| Catalog rows with at least one native Q contract | 221 | Static native-quest coverage in the generated zone inventory. |
| Distinct native Q contracts | 2,668 | Contract definitions, not story chains or mandatory order. |
| Authored story sidecars | 232 | Sidecar files currently mapped to active `areas/AREA` names; their presence does not prove a complete narrative review or active accounting qualification. |
| Comprehensive source dossiers mapped to active area keys | 243 area dossiers | The original inventory had 220 dossiers covering 220 active area keys: 218 mapped by the priorities table and two additional dossiers whose source headers map to `newbie2` and `twin_towers_forest`. Checkpoints 181–189 add `AKO_VILLAGE.md` (`ako`), `ANCIENT_BRIDGE.md` (`ancientb`), `ANT_FOREST.md` (`ants`), `ARACHDRATHOS_WILDERNESS.md` (`arac_wild`), `ARACHDRATHOS_GUILDS.md` (`aracguil`), `ARDGRAL.md` (`ardgral`), `ARENA.md` (`arena`), `ASTRAL_AREAS.md` (`astral_areas`), and `ASTRAL_MAIN.md` (`astral_main`). Checkpoint 190 adds `ASTRAL_TIAMAT.md` (`astral_tiamat`), Checkpoint 191 adds `ASYLUM.md` (`asylum`), Checkpoint 192 adds `AVERNUS.md` (`avernus`), and Checkpoint 193 adds `BLACK_TUNNELS.md` (`black_tunnels`); Checkpoint 194 adds `BLOODY_PLAINS.md` (`bloody_plains`); Checkpoint 195 adds `BOGENTOK.md` (`bogentok`); Checkpoint 196 adds `BORDELLO.md` (`bordello`); Checkpoint 197 maps the existing `BUGENTOLEN.md` (`bugentol`) dossier; Checkpoint 198 adds `BUILDINGS.md` (`buildings`); Checkpoint 199 adds `CBL_HALL.md` (`cbl_hall`); Checkpoint 200 adds `CELESTIA.md` (`celestia`); Checkpoint 201 maps `CHARING.md` (`charing`); Checkpoint 202 adds `CITYRUIN.md` (`cityruin`). Checkpoint 381 maps TEMPLE_OF_LADUGUER.md (temple_laduguer). The directory has one other Markdown file not mapped to an active area key: `TROLL_SLAVE_COMPOUND.md`. These source dossiers do not by themselves satisfy the broader fresh narrative/connectivity/dependency audit.
| Detailed source-story reviews in this follow-up | 43 | `ako`, `ancientb`, `ants`, `arac_wild`, `vehicles`, `heavens`, `mobs_underdark`, `orogs`, `rtun`, `gith_ht`, `marigot`, `thri`, `llzazan`, `charing`, `ugta`, `nax`, `sea`, `ghore`, `faang`, `nizari`, `kvarkpass_connector`, `fortb`, `cityruin`, `githyanki_fortress`, `limbo`, `llyrath`, `torture`, `aracguil`, `ardgral`, `arena`, `astral_areas`, `astral_main`, `astral_tiamat`, `asylum`, `avernus`, `black_tunnels`, `bloody_plains`, `bogentok`, `bordello`, `buildings`, `cbl_hall`, `celestia`, and `temple_laduguer` have dedicated story/environmental reviews; they are tracked as a separate layer from mapped comprehensive source dossiers. Ako, Ancient Bridge, Ant Forest, Arachdrathos Wilderness, Arachdrathos Guilds, Ard'gral, Arena, Astral Side Areas, Astral Main, Astral Tiamat, Asylum, Avernus, Black Tunnels, Bloody Plains, and BogenTok have repository-wide connectivity/dependency/history supplements; Vella's Bordello, Buildings, CBL Hall, and Celestia have the same supplements; Checkpoint 381 adds the Temple of Laduguer supplement.
| Rows in `areas/qst/AREA` | 344 | A separate historical quest index. It has six keys absent from `areas/AREA`, omits thirteen active keys, and is not read by the production world generator. |
| Current source/data stems across `.wld`, `.mob`, `.obj`, `.zon`, `.qst`, `.shp`, and story sidecars | 472 | 351 active keys plus 121 other current component keys. The latter are unregistered source components, not yet safe-to-archive zones. |
| Rows in the reconciled CSV register | 476 | The 472 current source stems plus four history-only manifest/retirement crosswalks with no current files. |
| Current WLD/MOB/OBJ/ZON stems outside `areas/AREA` | 104 unique stems | Current unregistered world-source components; shared-prototype, portal, reset, and code references remain to be traced before any disposition. |

The existing static quest inventory says “350 catalog zones; 221 have native Q
contracts; 2,668 distinct Q contracts; 232 authored journals.” That is a useful
reproducible catalog result, but it is not complete game-wide story coverage:
it omits negative administrative `limbo`, includes the special `end` area,
does not establish ordinary reachability, and uses mapped journal count as
sidecar coverage rather than as narrative-review completion
([inventory generator](../../../scripts/zone_story_quest_zone_inventory.py#L88),
[catalog registry and filter](../../../scripts/zone_story_quest_catalog.py#L63)).

## Which files the supported configurations load

### Normal full-world build and runtime

The root [`Makefile`](../../../Makefile#L18) builds six area generators. The
world target identifies top-level `areas/AREA` and `areas/RANDOM_AREA` as direct
inputs ([build inputs](../../../Makefile#L49)); `areas/m_slow` runs the six
generators, then `areas/moveall` installs the resulting `world.mob`, `world.obj`,
`world.qst`, `world.shp`, `world.wld`, and `world.zon`. Each `make_*` program
reads an `AREA` list from its current working directory; when `m_slow` invokes
it from `areas/`, that is the top-level list. For example, `make_wld` and
`make_qst` declare `AREA` as the input and open the matching source file under
their data directory ([room generator](../../../areas/src/wld/make_wld.c#L6),
[quest generator](../../../areas/src/qst/make_qst.c#L6)). The shared file helper
resolves only the requested area filename and has explicit case handling
([area file helper](../../../areas/src/area_file.h#L50)).

The normal runtime opens generated `areas/world.mob`, `world.obj`, `world.wld`,
`world.zon`, and `world.qst` ([file constants](../../../src/world/db.h#L23),
[mobile/object branch](../../../src/world/db.c#L591), [room loader](../../../src/world/db.c#L1187),
[zone loader](../../../src/world/db.c#L1757), [quest file selection](../../../src/world/quest.c#L2104)).
Therefore a same-named `.wld`, `.mob`, `.obj`, `.zon`, `.qst`, or `.shp` file
under `areas/` is not, by filename alone, proof of full-world membership: the
key must be in the source list the relevant generator actually consumes.

`areas/qst/AREA` is a stale/narrow independent list, not that production input.
The catalog builder documents the distinction directly
([quest-source selection](../../../scripts/zone_story_quest_catalog.py#L36)).
The source list currently includes 344 keys, but 119 have no same-key `.qst`
file; 40 current quest files are absent from that index, including 30 whose
keys are not in the active full-world `areas/AREA` list. Full quest compilation
uses top-level `areas/AREA`, so neither the old index nor a `.qst` file alone
defines a loaded story.

`areas/RANDOM_AREA` is a separate random-zone generation input read by
[`randomeq.c`](../../../src/item/randomeq.c#L540), not a second full-world zone
list. It is empty in this snapshot. The `random` key remains one of the normal
`areas/AREA` contributors; its presence must not be confused with membership in
the empty random-zone template input.

### Minimal development mode

`scripts/cycle_mud.sh --minimal` skips full-world generation and requires the
tracked `areas_mini/mini.mob`, `.obj`, `.qst`, `.wld`, `.zon` plus shop, table,
and weather support files ([launcher boundary](../../../scripts/cycle_mud.sh#L329),
[README](../../../README.md#L309)). `db.c` and `quest.c` select those aggregate
files directly. The current minimal set contains 67 room records, 171 mobile
prototypes, 801 object prototypes, two quest-giver records, and one zone/reset
header. Exact VNUM overlap with full-area files is recorded by type in the CSV;
an overlap is not a provenance or source-ownership decision because IDs may be
copied into fixtures.

The old `areas/src/areas/mini.c` reads a separate `AREA.MINI`, but no such
manifest is present and the current area-tool build does not include that
program in its normal generator set. It is historical generator code, not the
supported `--minimal` world list. The current minimal dataset is one aggregate,
not 351 independently toggled zone memberships.

## Current source-component census

The current world/QST/shop/story source union has 472 distinct stems: 351 in
the full `AREA` list and 121 current component-only keys outside it. The CSV
adds four history-only rows for `plane_fire_two`, `plane_fire_three`,
`player_ships`, and retired `siege`, so it now has 476 rows. Per-type file
counts are `wld=446`, `mob=445`, `obj=442`, `zon=442`,
`qst=265`, `shp=180`, and `story sidecars=232`. Every active full-area key has
the four core world files. Current unregistered component counts overlap:
`wld=95`, `mob=94`, `obj=91`, and `zon=91`; their union is 104, not their sum.
The shop-source set has 55 keys outside `areas/AREA`; the QST-source set has 30.
These are inventories, not archival eligibility: an unregistered prototype may
still be supplied or consumed by loaded foreign resets, native Q contracts,
shops, scripts, generated content, or saved identities.

Of the 351 active full-area keys, 234 have a mapped comprehensive source
dossier: the original 220 plus fourteen mapped additions (`ako`, `ancientb`,
`ants`, `arac_wild`, `aracguil`, `ardgral`, `arena`, `astral_areas`,
`astral_main`, `astral_tiamat`, `asylum`, `avernus`, `black_tunnels`, and `bloody_plains`).
The separate follow-up layer now contains 37 detailed story/environment
reviews; some of those keys also have mapped dossiers, so these counts are not
additive. The remaining 117 active keys lack a mapped comprehensive source
dossier. The 232 sidecars and
2,668 native Q contracts are separate inventory measures; neither establishes
complete narrative, runtime, or topology coverage. Row-level statuses track the
outstanding per-area work.

`areas/map/` contains 21 map JSON inputs and `areas/src/`, `areas/de/`, and
`areas/dewin32/` contain generators/editor support. They are inventoried as
non-zone/tooling inputs and are not counted as extra geographical zones. Their
per-zone dependencies remain part of the later source-closure review. Generated
`areas/world.*` outputs are build products, not authoritative per-zone source
files.

## Historical attic boundary

The current checkout has no `attic/` tree. Git history records `junk/` renamed
to `attic/` on August 12, 2009 (`ccbaeff5967ec7068e13e5f568e872f3d5718b56`).
On August 27, 2026, commit `3cb3646e0b2fd1234db0d1148f9a267ed71a1613`, titled
“Remove the retired attic tree and stale runtime archives,” deleted that tree.
Its commit description says `attic/` held pre-wipe game data and superseded
sources (including an old Kingdom/player-castle source snapshot, temporary area
work, editor sources, and old libraries) and says those files were not
referenced by the server, build, or tests and remained in Git history. The same
commit removed binary player/ship/SQL archives; this review does not inspect
private player/account archives.

The historical `attic/areas/` root sat outside the current generator's working
root and was absent from top-level `areas/AREA`; the runtime reads generated
`areas/world.*`, so the old tree was excluded structurally rather than by a
runtime “attic” switch. After that deletion, prior files are recoverable only
from Git history. Current unregistered files still under `areas/{type}/` are a
different category and must not be moved on the basis of this history alone.

## Coverage state at this checkpoint

- **Inventory boundary:** reconciled above. The 476-row CSV has 351 active
  area contributors, 121 current unregistered component stems, and four
  no-current-file history rows.
- **Story and environment:** 234 active keys have mapped comprehensive source
  dossiers, including the original 220 and fourteen mapped additions. The
  separate follow-up layer contains 37 detailed story/environment reviews;
  some overlap the dossier layer. The remaining 117 active keys lack a mapped
  comprehensive source dossier. Authored sidecars, QST contracts, and partial
  follow-up reviews are separate evidence layers and do not establish complete
  environmental-story coverage. See the row-level status and dossier path in
  the CSV.
  The ten selective follow-up chains remain partial case notes, not whole-zone
  audits. See the row-level status and dossier path in the CSV.
  exit census and loader target classification. It found 253,261 loaded room
  records, 906,176 raw exits, 7,144 targets the loader prunes, 1,785
  cross-zone exits by numeric zone range, 836 directed zone pairs, and one
  duplicate room VNUM. Exact records and limitations are in
  [`WORLD_CONNECTIVITY.md`](WORLD_CONNECTIVITY.md) and its linked CSV ledgers.
  The Mistywood supplement adds 53 source-only records; Darkfall and Kimordril
  add two each; Bugentol adds one `surf.wld` record; Ixarkon adds two
  `ud_map2.wld` records; Troll Caves adds three from `surf.wld` and
  `underdark2011.wld`; IceCrag adds two from those same sources; Cloister adds
  two from `surf.wld` and `underworld2.wld`; Turolopolis adds one from `surf.wld`;
  Du'Maathe Castle adds five from `surf.wld` and `oldmap8.wld`; Tundra adds
  seven from `surf.wld`; The Fields Between, The Town of Moregeeth and Ceothia
  each add one from `surf.wld`. Braddistock Mansion adds 192 `Duris3.wld`
  records in four bands, with 48 source-room VNUMs overlapping its active
  range; see the [Mansion audit supplement](BRADDISTOCK_MANSION_AUDIT_REVIEW.md).
  The Desert City of Venan'Trut adds one `surf.wld` east exit into room 49003;
  see the [Desert City audit supplement](DESERT_CITY_AUDIT_REVIEW.md). The
  Past Ceothia scan found no unregistered WLD targets or source-room VNUM
  overlaps with its exact active rooms; see the
  [Past Ceothia audit supplement](PAST_CEOTHIA_AUDIT_REVIEW.md). The Basin
  Wastes supplement adds one down exit from `surf.wld` room 229308 to room
  34000, with no source-room VNUM overlap; see the
  [Basin Wastes audit supplement](BASIN_WASTES_AUDIT_REVIEW.md). Nakral's Crypt
  adds two source-only north exits into active room 14300, with no source-room
  VNUM overlap; see the [Crypt audit supplement](NAKRALS_CRYPT_AUDIT_REVIEW.md).
  The Valoisian Castle adds one source-only north exit from unregistered
  `surf.wld` room 205053 into active room 38575, with no source-room VNUM
  overlap; see the [Valoisian Castle audit supplement](VALOISIAN_CASTLE_AUDIT_REVIEW.md).
  Harrow adds one source-only north exit from unregistered `surf.wld` room
  214566 into active room 29400, with no source-room VNUM overlap; see the
  [Harrow audit supplement](HARROW_AUDIT_REVIEW.md). Mountain Tracts adds
  three source-only `surf.wld` exits into rooms 20951, 21110, and 20901; none
  of the 95 unregistered source files reuses a Mountain room VNUM. See the
  [Mountain Tracts audit supplement](MOUNTAIN_TRACTS_AUDIT_REVIEW.md). The
  curated source-only list now contains 586 individually reviewed records.
  The Orcish Slave Camp scan found no unregistered target match or source-room
  VNUM overlap. Its only active boundary remains the reciprocal Split Shield
  pair. See the
  [Orcish Slave Camp audit supplement](ORCISH_SLAVE_CAMP_AUDIT_REVIEW.md);
  the curated source-only count remains 586.
  Checkpoint 25's Torture review adds no source-only WLD
  inbound; its one Ghore boundary was already in the paired active-exit ledger.
  Checkpoint 27's Bugentol review adds
  one source-only `surf.wld` inbound to room 95800. Checkpoint 28's Jotunheim
  scan found no source-only WLD targets into its room range. Checkpoint 29's
  Ixarkon scan found two `ud_map2.wld` exits into room 96492. Checkpoint 30's
  Dirk’nspire scan found no targets in rooms 96800–96885; the curated count
  remained 362. Checkpoint 31's Troll Caves scan found three source-only exits
  into rooms 96900 and 96903, bringing the curated count to 365. Checkpoint 32
  found two further source-only exits into IceCrag room 97001, bringing the
  curated count to 367. Checkpoint 33 found two source-only exits into Cloister
  room 67100, bringing the curated count to 369. Checkpoint 34 found one north
  exit from `surf.wld` room 236469 into Turolopolis room 7102, bringing the
  curated count to 370.
  Checkpoint 35 found three `surf.wld` and two `oldmap8.wld` exits into
  Du'Maathe Castle rooms 37409, 37416, and 37425, bringing the curated count
  to 375.
  Checkpoint 36 found seven `surf.wld` exits into Tundra rooms 13703, 13712,
  13778, 13796, 13870, 13895, and 13936, bringing the curated count to 382.
  Checkpoint 37 found one `surf.wld` south exit into The Fields Between room
  71001, bringing the curated count to 383.
  Checkpoint 38 found one `surf.wld` west exit into The Town of Moregeeth room
  70198, bringing the curated count to 384.
  Checkpoint 39 found one north exit from `surf.wld` room 267058 into Ceothia
  room 80800, bringing the curated count to 385.
  Checkpoint 40 found 192 `Duris3.wld` exits whose numeric targets cover
  Braddistock Mansion rooms 135001–135048, plus 48 source-room VNUMs that
  overlap that active range. These are source-only map records, not active
  ingress; this checkpoint brought the curated count to 577. The repeated-title
  review also records
  that older `braddistock` and `brad` have distinct current routes and tracked
  path histories; the shared title does not establish replacement.
  Checkpoint 41 found one east exit from unregistered `surf.wld` room 202208
  into Desert City room 49003. The source-room VNUMs do not overlap Desert's
  active room records; this remains source-only, bringing the curated count to
  578. The active reciprocal Surface boundaries and Jussinload-to-Limbo links
  remain separately recorded in the active exit ledger.
  Checkpoint 42 found no active cross-zone WLD declarations and no source-only
  WLD target or source-room VNUM overlap for Past Ceothia. The known Present
  portal arrival and Past-to-Future portal remain typed boundaries, outside the
  ordinary WLD graph. Bounded typed scans also found two C/C++ chaos-kit table
  references to local bracer 81124, without establishing kit selection or
  issuance. The curated source-only count remains 578.
  Checkpoint 43 found one down exit from unregistered `surf.wld` room 229308
  into Basin Wastes room 34000. No source-room VNUM overlaps the active Basin
  range; the record remains source-only and brings the curated count to 579.
  Checkpoint 44 found north exits from unregistered `surf.wld` room 276166 and
  `underworld2.wld` room 422044 into active Crypt room 14300, with no
  source-room VNUM overlap. These two map records remain source-only and bring the curated
  count to 581. The bounded exact scans found no external QST, reset, SHP, or
  story-sidecar VNUM references to Crypt prototypes; static C/C++ findings
  include Phantom `shift_prime` room 14373 and seven chaos-kit table references
  to object 14479. Neither code declaration establishes runtime use or
  issuance. Tracked WLD history shows the active west endpoint changed and was
  later restored; no replacement is inferred.
  Checkpoint 45 found one north exit from unregistered `surf.wld` room 205053
  into active Val room 38575, with no source-room VNUM overlap. It remains
  source-only and brings the curated count to 582. The bounded exact scans
  found no external QST, SHP, or sidecar VNUM references to Val prototypes; a
  Verspin reset group references local dagger 38421. Exact C/C++ scans found 13
  static chaos-kit slot values for local objects 38444, 38450, and 38455,
  without establishing kit selection or issuance. The normalized active title
  is unique; tracked WLD history shows no endpoint replacement, and the ZON
  history includes a steel-key reset addition.
  Checkpoint 46 adds one source-only north exit from `surf.wld` room 214566 to
  active Harrow room 29400; the source room does not overlap Harrow's active
  room VNUMs. The other 110 numeric target matches come from five unregistered
  WLD maps whose source-room VNUMs overlap Harrow's active set. The active
  Harrow graph has 187/187 resolving exits and two reciprocal Surface pairs.
  Exact active typed scans found one Tundra story-sidecar contract for Bom's
  painting exchange and no external QST, ZON, or SHP reference; one unregistered
  `brass-old-1.zon` G reset names Harrow flute 29479. Exact C/C++ scans found
  83 static chaos-kit slot values for seven local objects plus direct fishing
  pole and inn references. The normalized active title is unique; reviewed
  tracked WLD/ZON history shows no boundary endpoint or reset-value change.
  The curated count rises to 583. See the
  [Harrow audit supplement](HARROW_AUDIT_REVIEW.md).
  Checkpoint 47 adds three source-only `surf.wld` exits to Mountain Tracts
  rooms 20951, 21110, and 20901, with no source-room VNUM overlap across the
  unregistered WLD set. Fifteen active reciprocal cross-area pairs and six
  unresolved local exits remain in the active ledger. One active Winterhaven
  QST exchange consumes local gloves 20950 and potion 20949; no other active
  external reset, shop, or story-sidecar reference was found. Static code
  scanning found three chaos-kit slot values for boots 20924 and leggings
  20937. The normalized title is unique; tracked WLD history includes the
  2009 Connectorzones endpoint addition. See the
  [Mountain Tracts audit supplement](MOUNTAIN_TRACTS_AUDIT_REVIEW.md). The
  curated source-only count rises to 586.
  Checkpoint 48 finds no source-only WLD ingress or VNUM overlap for The
  Orcish Slave Camp. Its 27/27 exits resolve, and its one reciprocal
  cross-area pair remains with Split Shield. Exact active and unregistered
  typed scans and the C/C++ literal scan found no external reference to local
  rooms, mobiles, or objects. The normalized active title is unique; tracked
  WLD history changes room metadata values but not exit endpoints, while ZON
  history changes only the editor comment. See the
  [Orcish Slave Camp audit supplement](ORCISH_SLAVE_CAMP_AUDIT_REVIEW.md).
  Checkpoint 49 finds no source-only WLD ingress or VNUM overlap for The
  Underground Lava Caves. Its 451/451 exits resolve, with one reciprocal
  Mountain Tracts pair. The bounded exact scans record a Ravenloft2 reset for
  local lamp #35589 and an unregistered `player_guilds.shp` shop location in
  local room #35552. The latter file can enter the shop loader's fallback when
  `areas/world.shp` is missing; its #35551 stock value is an unresolved object
  lookup, not a LavCav object reference. Structured sidecar scans found no
  exact numeric VNUM values; the sole C/C++ literal is a table value whose
  selector is commented out. The title is unique, and tracked WLD/ZON history
  shows only relocation, mode, and editor-comment changes. See the
  [Underground Lava Caves audit supplement](UNDERGROUND_LAVA_CAVES_AUDIT_REVIEW.md).
  Checkpoint 50 finds no source-only WLD ingress or VNUM overlap for The
  Cimmerian Nomad Encampment. Its 124/124 exits resolve in one 49-room weak
  component with four reciprocal Surface pairs; the curated source-only count
  remains 586. Exact scans find no external QST, reset, or structured
  story-sidecar VNUM fields. Two unregistered SHP files each place records in
  Nomads rooms #6242/#6243, with keeper mobile values that do not resolve; both
  can enter the shop-loader fallback when `areas/world.shp` is absent. Static
  code references include the Githzerai prime-shift destination #6224 and 24
  CHAOS starter-table crown #6222 entries. Three generic `wh.story.json` hits
  are generated-ID substrings, not VNUM references. The title is unique and
  tracked WLD/ZON history shows the 2009 relocation and a 2015 editor-comment
  change. See the
  [Cimmerian Nomad audit supplement](CIMMERIAN_NOMAD_ENCAMPMENT_AUDIT_REVIEW.md).
  A separate raw scan of the 95 unregistered WLD files found 572,824
  exit-target VNUM matches to loaded rooms, dominated by large unregistered
  map sources with extensive active source-room VNUM overlap. Those numeric matches remain
  unclassified and are not reported as runtime routes.
  Checkpoint 51 finds no source-only WLD ingress or VNUM overlap for The Ruins
  of Undermountain. All 1,059 exits resolve across 441 rooms in one weak
  component, with one reciprocal `bs` boundary and one down exit to Underworld
  #4557. Exact typed scans record IceCrag's two-wine input, 20 Caertannad
  mobile resets, active Brass reset/shop references to local picks, FirePlane's
  Flame reset, and source-only `brass-old-1`/`nexus56` reset and shop references.
  The latter sources may enter the shop-loader fallback when
  `areas/world.shp` is absent. Avernus code references local mobile #92076. The
  normalized title is unique and tracked WLD/ZON history identifies no
  distinct predecessor or replacement. See the
  [Undermountain audit supplement](UNDERMOUNTAIN_AUDIT_REVIEW.md).
  Checkpoint 52 records one positive-unresolved Desolate Under Fire exit:
  room #77301 south to #217609. That number is present in three unregistered
  WLD sources as a road, a grassland room, and a mossy cave with conflicting
  exits; the normal build omits the sources and prunes the exit. The all-95
  reverse scan found no ingress or room overlap. Exact typed scanning found two
  SurfaceKeeps resets for local potion #77366 and no external QST, SHP, story,
  or code prototype references. The normalized active title is unique; tracked
  WLD/ZON history identifies no distinct predecessor or replacement. See the
  [Desolate Under Fire audit supplement](DESOLATE_UNDER_FIRE_AUDIT_REVIEW.md).
  Checkpoint 53 records Storm Port Stronghold's three reciprocal Surface
  pairs and six one-way exits from load room #22649. One unregistered
  `surf.wld` exit from room #285640 reaches local room #22634, but the file is
  omitted from `areas/AREA` and does not provide active ingress. The all-95
  reverse scan found no source-room overlap; this adds one reviewed record,
  bringing the curated source-only count to 587 and the `surf.wld` subset to
  27. Exact scans map two `vehicles.zon` gear resets, foreign QST and
  story-sidecar references, and local code bindings; no SHP match was found.
  The normalized title is unique, and tracked history identifies no distinct
  predecessor or replacement. See the
  [Storm Port Stronghold audit supplement](STORM_PORT_STRONGHOLD_AUDIT_REVIEW.md).
  Checkpoint 54 records the Harpy Settlement's one reciprocal Surface pair and
  one unregistered `surf.wld` entry from room #249084 to local room #31100. The
  source-only record is not active ingress; the all-95 reverse scan found no
  source-room overlap. This adds one reviewed record, bringing the curated
  source-only count to 588 and the `surf.wld` subset to 28. Exact scans map
  Cosmic's plans/tail and Divine Home's egg QST references, three matching
  sidecar fields, and local custom-code bindings; no foreign reset or SHP match
  was found. The normalized title is unique. The shared “Alachest” manifest
  note is not a predecessor or replacement. See the
  [Harpy audit supplement](HARPYHT_AUDIT_REVIEW.md).
- **Reachability:** actual door/reset state, conditional secrets, special
  procedures, time/instance changes, transport, safe traversal, return routes,
  and ordinary versus staff access remain pending. The static graph is not
  presented as player reachability.
- **Duplicate/replacement history:** the normalized title scan found three
  repeated-title groups; manifest/Git history established the Brass, player
  ship, IC3, Asunwa/Jungle, and Siege cases; and the 51800 WLD collision is a
  concrete unresolved conflict. The Githyanki reviews record the 2010 “old
  gith fort” replacement claim while leaving the predecessor unidentified.
  These are recorded in
  [`DUPLICATE_AND_REPLACED_ZONES.md`](DUPLICATE_AND_REPLACED_ZONES.md). The
  Darkfall path-history and source-map comparison is now recorded. Kimordril's
  tracked 2013 boundary change and parallel map endpoints are also recorded.
  Bugentol's 2011/2013 endpoint changes and divergent parallel map records are
  recorded. Jotunheim's normalized active title is unique; tracked path history
  shows no distinct replacement. Ixarkon's title is unique; its 2010 Faang
  connection commit records a link addition, not replacement. Dirk’nspire's
  normalized title is unique; reviewed WLD/ZON path history names no distinct
  predecessor or replacement. Troll Caves' normalized active title is unique;
  no distinct predecessor or replacement is named in its reviewed history.
  IceCrag's normalized title is unique; its reviewed tracked WLD/ZON history
  names no distinct predecessor or replacement. Father Tel's Holy Cloister's
  normalized title is unique; its reviewed tracked WLD/ZON history names no
  distinct predecessor or replacement. The Ruins of Turolopolis' normalized
  title is unique; its tracked WLD/ZON history names no distinct predecessor
  or replacement. Du'Maathe Castle's normalized active title is unique; its
  tracked WLD/ZON history names no distinct predecessor or replacement.
  Tundra's normalized active title is unique; its reviewed tracked WLD/ZON
  history names no distinct predecessor or replacement.
  The Fields Between's normalized active title is unique; its reviewed tracked
  WLD/ZON history names no distinct predecessor or replacement.
  The Town of Moregeeth's normalized active title is unique; its reviewed
  tracked WLD/ZON history names no distinct predecessor or replacement.
  Broader per-zone semantic comparison remains pending.
- **Dependencies and attic plan:** the 121 current unregistered stems each
  have a retention entry. A first WLD-only screen found 201 active exit
  records targeting VNUMs found in unregistered WLD files, with duplicate
  file matches noted. Mistywood, Darkfall, and Kimordril now have bounded
  per-area typed closure. Mistywood's source-only Kilfray QST copy uses the
  same five giver VNUMs and three parallel contracts, but omits Mistywood's
  Riliatar 25,000 and Jarnes 10,000 experience terms; it
  has no documented replacement lineage and is excluded from the production
  catalog. Darkfall has two source-only `surf.wld` inbounds, no external
  literal reset/QST/shop references to its local prototypes, and a direct
  code-defined Epic Agility teacher binding for Grellinar #95304. Torture now
  has a source-comprehensive review, a paired Ghore boundary, four source fall
  fields, and bounded external typed-reference closure. Bugentol now has a
  source-comprehensive review, one paired Surface
  boundary, an unpaired Wh inbound, one source-only surf.wld inbound, two reset-
  closed door pairs, and bounded exact typed-reference closure. Jotunheim's source
  dossier and checkpoint 28 supplement map its active Astral portal, two foreign
  QST recipes, the Shabo reset group, and two source-only brass-old-1 reset refs
  to local objects; tooling and persisted-reference closure remain pending.
  Checkpoint 56 revalidated the all-95 WLD scan with no Jotunheim target or
  source-room overlap and found its #96012 Mistweave object in both Druid CHAOS
  equipment profiles. The curated source-only WLD total remains 592.
  Ixarkon's existing dossier and checkpoint 29 supplement map two active
  Ixxillikor resets, two source-only ud_map2 WLD inbounds, and its unique title
  history; no foreign quest/shop item consumer was found. Dirk’nspire's
  existing dossier and checkpoint 30 supplement map its reciprocal Valinhav
  boundary, two active Alatorin QST consumers of its Foe Hammer, and one active
  Alatorin reset source for local loose change; all 95 unregistered WLD files
  were checked and none targets its room range. Troll Caves' existing dossier
  and checkpoint 31 supplement map three source-only WLD inbounds, the active
  Surface Keeps shop/reset supply for three local gem prototypes, and the
  Graves fixed-teleport declaration; a correction attributes the gem dealer
  in the dossier to Surface Keeps rather than Alatorin; the local story sidecar
  still carries the earlier wording. Kimordril's existing
  comprehensive dossier records 11 external reset groups, one matching shop,
  two Alatorin QA blocks, and 18 compiled references; editor/tooling, persisted
  user-reference, and indirect/dynamic closure remain pending. Its two
  source-only WLD inbounds and tracked 2013 boundary edit do not establish
  reachability or replacement lineage. IceCrag's source-comprehensive dossier
  and checkpoint 32 supplement map two additional source-only WLD inbounds,
  active foreign reset/QST consumers and producers, two hard-coded equipment
  references, one inactive Brass reset reference, and its unique title and
  bounded path-history result; gameplay, tooling/editor, persisted-player,
  and indirect/dynamic closure remain pending. Cloister's comprehensive
  dossier and checkpoint 33 supplement map two additional source-only WLD
  inbounds, two active reciprocal boundaries, foreign Jade/Winterhaven item
  consumers, an Alatorin reset source, hard-coded chaos-kit and epic-teacher
  bindings, and its unique title/path-history finding; gameplay, tooling/editor,
  persisted-player, and indirect/dynamic closure remain pending. Turolopolis'
  comprehensive dossier and checkpoint 34 supplement map one source-only WLD
  inbound, active Surface and Zoo boundary pairs, a Githzerai `shift_prime`
  code candidate to room 7117, the active Surface reset for Lothrell, and
  unregistered Nexus56 reset/shop references to the local biography book;
  gameplay, editor/tooling, persisted-player, and indirect/dynamic closure
  remain pending. Du'Maathe Castle's comprehensive dossier and checkpoint 35
  supplement map five source-only WLD inbounds, active Surface and Pit of
  Dragons boundary pairs, Alatorin's totem consumer, a Surfacemini
  story-sidecar contact, Carmotee's epic-teacher binding, and two unregistered
  Brass reset references to local objects. Gameplay,
  editor/tooling, persisted-player, and indirect/dynamic closure remain pending.
  Tundra's comprehensive dossier and checkpoint 36 supplement map seven
  source-only `surf.wld` inbounds, active Surface and Underworld3 boundary
  pairs and the one-way staff-room edge, Ship Yards' ration quest input,
  Alatorin's reset/shop supply, Surface Keeps' ration/waterskin resets, and
  unregistered `surf.zon` mobile resets and `player_guilds.shp` stock for local
  prototypes. Its unique title and bounded path-history result are recorded;
  gameplay, editor/tooling, persisted-player, and indirect/dynamic closure
  remain pending.
  The Fields Between's comprehensive dossier and checkpoint 37 supplement map
  one source-only `surf.wld` inbound, a reciprocal Surface pair, one-way
  Juiblex, Scorched Valley and Winterhaven inbounds, and its outgoing Scorched
  Valley edge. They also map Juiblex and Scorched Valley item consumers, the
  Juiblex sidecar's Fields Between producer mapping, and code-defined chaos
  equipment references to two local objects; no active foreign reset or shop
  reference was found. Gameplay, editor/tooling, persisted-player, and
  indirect/dynamic closure remain pending.
  The Town of Moregeeth's existing source-comprehensive dossier and checkpoint
  38 supplement map one source-only `surf.wld` inbound and five active boundary
  records. Its coverage row now records all ten active local shops
  (#70029–#70083). An unregistered `myranth1.shp` has five source-only shop
  records referencing local mobile and room prototypes; normal AREA aggregation
  excludes it, while the shop loader's missing-catalog fallback reads every
  `areas/shp/*.shp` file. The bounded supplement also maps the code-defined
  room/mobile references and title/path history. Runtime, editor/tooling,
  persisted-player, and indirect/dynamic closure remain pending.
  Tribal Oasis's Checkpoint 72 supplement reconciles its reciprocal Surface
  edge and one source-only `surf.wld` ingress declaration; it adds one to the
  curated total (603 to 604). External typed-reference scans found no QST,
  shop, reset, sidecar, prototype, or portal target. Existing CHAOS starter
  gear and the legacy Bargor training-table entry remain classified in the
  dossier. Its title is unique, tracked WLD/ZON history names no replacement,
  and its current active-manifest disposition remains **DO NOT MOVE**. See
  [TRIBAL_OASIS_AUDIT_REVIEW.md](TRIBAL_OASIS_AUDIT_REVIEW.md).
  The Great Realm's Checkpoint 73 supplement separates 70 source-only `surf.wld`
  inbounds from its 34 unresolved active outbounds, bringing the curated total
  to 674 and the reviewed Surf subset to 105. It also maps the Surfmini QST and
  sidecar references, one external portal, 209 inactive Surf reset records,
  and unique title/history disposition. See
  [CONNECTORZONES_AUDIT_REVIEW.md](CONNECTORZONES_AUDIT_REVIEW.md).
  The Battlefield's Checkpoint 74 supplement adds one non-overlapping, source-only
  `surf.wld` west approach and distinguishes it from the reciprocal active
  Surface pair. The curated total is 675 and the reviewed Surf subset is 106.
  It records no external typed target and classifies four direct compiled
  references; title and bounded history identify no replacement. See
  [BATTLEFIELD_AUDIT_REVIEW.md](BATTLEFIELD_AUDIT_REVIEW.md).
  The Forgotten Mansion's Checkpoint 75 audit records 390/390 resolving exits,
  two active inbounds and one reciprocal Surface outbound. Two non-overlapping
  inactive WLD approaches, from `surf.wld` and `underworld2.wld`, raise the
  curated source-only total to 677 and the reviewed Surf subset to 107. Typed
  closure classifies two inactive Brass reset-role rows, CHAOS kit objects,
  Tyrlos's dormant high-drop entry, and an unassigned callback to room #3600.
  The title is unique, bounded history identifies no replacement, and the area
  remains **DO NOT MOVE**. See
  [MANSION_AUDIT_REVIEW.md](MANSION_AUDIT_REVIEW.md).
  Woodseer's Checkpoint 76 audit records 1,040/1,041 resolving exits, six
  reciprocal boundary pairs, two one-way inbounds, and the unresolved Duris3
  stair target. Two additional source-only Surf approaches raise the curated
  total to 679 and reviewed Surf subset to 109. Typed closure classifies the
  active Surface Mini and Alatorin references plus inactive Solstice shop/reset
  rows; direct compiled bindings, unique title, and bounded history are
  documented in [WOODSEER_AUDIT_REVIEW.md](WOODSEER_AUDIT_REVIEW.md).
  Woodseer remains **DO NOT MOVE**; the queue is 116/220, with 104 pending and
  Village of Refugees next.
  The Village of Refugees' Checkpoint 77 audit records 142/142 resolving exits
  in one component and two reciprocal Surface pairs. One non-overlapping
  source-only Surf approach raises the curated total to 680 and reviewed Surf
  subset to 110. Typed closure maps active Newhaven, Newbie, and Alatorin
  references plus Nexus56 source-only shop/reset references, including its
  possible shop-loader fallback. No external prototype, portal target, or
  compiled numeric match was found. The normalized title is unique, bounded
  history identifies no replacement, and the area remains **DO NOT MOVE**.
  See [VILLAGE_OF_REFUGEES_AUDIT_REVIEW.md](VILLAGE_OF_REFUGEES_AUDIT_REVIEW.md).
  The queue is now 117/220, with 103 pending and The Twin Towers next.
  The Twin Towers' Checkpoint 78 audit records 211/211 resolving exits in one
  component and four reciprocal active boundary pairs. The all-95 inactive WLD
  scan found no inbound exit, so the curated source-only total remains 680 and
  the reviewed Surf subset remains 110. Typed closure classifies four foreign
  QST heart commands, four structured sidecar item references, and two direct
  C/C++ code bindings; no foreign shop, reset, prototype, or portal target was
  found. Its title is unique, bounded history stays on the existing paths, and
  no replacement is identified. Twin Towers remains **DO NOT MOVE**. See
  [TWIN_TOWERS_AUDIT_REVIEW.md](TWIN_TOWERS_AUDIT_REVIEW.md). The queue is now
  118/220, with 102 pending and The Deep Ravine of Passage next.
  The Deep Ravine's Checkpoint 79 audit records 404/404 resolving exits across
  five weak components and six reciprocal active boundary pairs. Four
  non-overlapping inactive Surf approaches raise the curated source-only total
  to 684 and the reviewed Surf subset to 114. Typed closure maps 37 active
  imported-prototype reset commands, five source-only Brass reset rows,
  Alatorin's alternate symbol output, and six chaos-equipment code references;
  no external shop, prototype definition, or type-25 target matched. Recent
  bounded history and the normalized unique title are documented. The Deep
  Ravine remains **DO NOT MOVE**. See
  [DEEP_RAVINE_AUDIT_REVIEW.md](DEEP_RAVINE_AUDIT_REVIEW.md). The queue is now
  119/220, with 101 pending and Neverwind Valley next.
  Neverwind Valley's Checkpoint 80 audit records 199 rooms and 555/555
  resolving exits in one weak component, three reciprocal active outbounds,
  and an additional incoming-only Connector Zones edge. One non-overlapping
  source-only Surf approach raises the curated total to 685 and the reviewed
  Surf subset to 115. Typed scans find no foreign prototype, reset, QST, shop
  product, sidecar, or portal destination; the direct code match is the
  computed Githzerai arrival. Its normalized title is unique, bounded history
  identifies no replacement, and the area remains **DO NOT MOVE**. See
  [NEVERWIND_VALLEY_AUDIT_REVIEW.md](NEVERWIND_VALLEY_AUDIT_REVIEW.md). The
  queue is now 120/220, with 100 pending and Grumbar's Domain next.
  Grumbar's Domain's Checkpoint 81 audit records 199 rooms and 794/794
  resolving exits in one weak component, one reciprocal Elemental Plane
  outbound, and two incoming-only plane edges. The all-95 inactive-WLD scan
  found 400 non-overlapping Duris3 map exits into every active room and 199
  same-numbered inactive Ocean records. Those source-only exits raise the
  curated total to 1085 while the reviewed Surf subset remains 115. Typed and
  compiled scans find only the already-documented purple-worm binding among
  exact external matches; title and bounded history identify no replacement.
  Grumbar's Domain remains **DO NOT MOVE**. See
  [GRUMBARS_DOMAIN_AUDIT_REVIEW.md](GRUMBARS_DOMAIN_AUDIT_REVIEW.md). The queue
  is now 121/220, with 99 pending and The Temple to Skrentherlog next.
  The Temple to Skrentherlog's Checkpoint 82 audit records 56 rooms and
  104/104 resolving exits in a 55-room component plus isolated #80599. Two
  active inbounds and one outbound include a reciprocal Village of the Damned
  pair and a one-way Winterhaven inbound. The all-95 inactive-WLD scan found no
  approach or source-room VNUM overlap, so the curated total remains 1085.
  Typed scans classify two Astral-Tiamat coin resets for local #80571; no other
  external exact QST, shop, sidecar, prototype, or portal target matched. The
  title and bounded history identify no replacement; the Temple remains
  **DO NOT MOVE**. See
  [TEMPLE_TO_SKRENTHERLOG_AUDIT_REVIEW.md](TEMPLE_TO_SKRENTHERLOG_AUDIT_REVIEW.md).
  The queue is now 122/220, with 98 pending and Caves of Mt. Skelenak next.
  Caves of Mt. Skelenak remains **DO NOT MOVE**. See
  [CAVES_OF_MT_SKELENAK_AUDIT_REVIEW.md](CAVES_OF_MT_SKELENAK_AUDIT_REVIEW.md).
  Its 152 rooms have 323/324 resolving exits in components of 124, 22, and 6;
  five active boundary pairs are reciprocal, #4130 adds one resolved one-way
  outbound, and #4111 east to inactive Duris3 #153645 remains unresolved. Three
  non-overlapping source-only Surf approaches raise the curated total to 1088
  and reviewed Surf subset to 118. Typed scans find no external exact QST, shop,
  sidecar, prototype, or portal target; the supplement records direct code
  matches and bounded title/history findings. The queue is now 123/220, with 97
  pending and Southern Coastal Highway next. Southern Coastal Highway's
  Checkpoint 84 audit records 539 rooms and 1,410/1,411 exits resolving to active
  rooms in components of 533 and 6, five reciprocal boundary pairs, and the
  negative #-1 sentinel at #41488. Two non-overlapping source-only Surf
  approaches raise the curated total to 1090 and reviewed Surf subset to 120.
  Bastine owns the ten external QST item-command matches and seven exact sidecar
  references; Alatorin has one active reset for local horse #41362. No external
  shop, prototype, or type-25 destination matched; the direct code route and
  remaining references are classified in
  [SOUTHERN_COASTAL_HIGHWAY_AUDIT_REVIEW.md](SOUTHERN_COASTAL_HIGHWAY_AUDIT_REVIEW.md).
  The title is unique, bounded history identifies no replacement, and the
  Highway remains **DO NOT MOVE**. The queue is now 124/220, with 96 pending and
  Labyrinth of No Return next. No candidate is approved to move. See
  [`ATTIC_MIGRATION_PLAN.md`](ATTIC_MIGRATION_PLAN.md).

- **Checkpoint 85 — Labyrinth of No Return:** the 296-room graph has 602/602
  resolving exits in one component and one reciprocal Mountain Tracks boundary
  pair. No inactive WLD source room overlaps the area or approaches it; the
  curated source-only total remains 1090 and reviewed Surf subset remains 120.
  Typed scans classify Tower's exact three-item return, the unregistered
  duplicate Knight QST sequence, Caertannad and Troll Caves resets, and the
  compiled equipment/special references. The title is unique, bounded history
  identifies no replacement, and Labyrinth remains **DO NOT MOVE**. See the
  [audit supplement](LABYRINTH_OF_NO_RETURN_AUDIT_REVIEW.md) and
  [coverage row](ZONE_COVERAGE_REGISTER.csv#L154). The queue is now 125/220,
  with 95 pending and Khildarak Stronghold next. No candidate is approved to
  move. See [ATTIC_MIGRATION_PLAN.md](ATTIC_MIGRATION_PLAN.md).

- **Checkpoint 86 — Khildarak Stronghold:** 751 rooms have 1,582/1,582
  resolving exits in three components (749/1/1), with three reciprocal
  boundaries and one Heavens inbound-only edge. The all-95 inactive-WLD scan
  found no source-room overlap and one source-only `ud_map2.wld` approach to
  #17000; the curated source-only total is 1091 and reviewed Surf subset is
  120. Typed scans classify five active Alatorin mobile reset records, source-
  only Nexus56 and player-guild shop products, QST reward-number collisions,
  and compiled room/code references; no external QST item consumer, external
  prototype, active type-25 target, or sidecar item hit was found. The title is
  unique, bounded history identifies no replacement, and Khildarak remains
  **DO NOT MOVE**. See the [audit supplement](KHILDARAK_STRONGHOLD_AUDIT_REVIEW.md)
  and [coverage row](ZONE_COVERAGE_REGISTER.csv#L144). The queue is now 126/220,
  with 94 pending and Strathor Valley of the Storm Giants next. No candidate is
  approved to move. See [ATTIC_MIGRATION_PLAN.md](ATTIC_MIGRATION_PLAN.md).

- **Checkpoint 87 — Strathor Valley of the Storm Giants:** 136 rooms have
  297/297 resolving exits in one component and two reciprocal Surface boundary
  pairs. The all-95 inactive-WLD scan found no source-room overlap and one
  source-only `surf.wld` approach to #30800; the curated source-only total is
  1092 and reviewed Surf subset is 121. Typed scans classify Alatorin's active
  reset of local object #30820 and compiled quest-handler/ferry-stop references;
  no external QST item, SHP product, prototype, type-25 destination, or sidecar
  item hit was found. The title is unique, bounded history identifies no
  replacement, and Strathor remains **DO NOT MOVE**. See the [audit supplement](STRATHOR_VALLEY_AUDIT_REVIEW.md)
  and [coverage row](ZONE_COVERAGE_REGISTER.csv#L283). The queue is now 127/220,
  with 93 pending and The Gagga'Jobo Cave System next. No candidate is approved
  to move. See [ATTIC_MIGRATION_PLAN.md](ATTIC_MIGRATION_PLAN.md).

- **Checkpoint 88 — The Gagga'Jobo Cave System:** 23 rooms have 45/45 resolving
  exits in one component and one reciprocal Underdark boundary pair. The all-95
  inactive-WLD scan found no source-room overlap or approach; the curated
  source-only total remains 1092 and the reviewed Surf subset remains 121.
  Typed scans found no external QST item, SHP product, prototype, type-25
  destination, or sidecar item hit. The compiled scan classifies Ixarkon's
  dynamic veil destination at room #19022. The title is unique, bounded history
  identifies no replacement, and the cave remains **DO NOT MOVE**. See the
  [audit supplement](GAGGA_JOBO_CAVE_SYSTEM_AUDIT_REVIEW.md) and [coverage row](ZONE_COVERAGE_REGISTER.csv#L110).
  The queue is now 128/220, with 92 pending and The Tharnadian Ruin next. No
  candidate is approved to move. See [ATTIC_MIGRATION_PLAN.md](ATTIC_MIGRATION_PLAN.md).

- **Checkpoint 89 — The Tharnadian Ruin:** 179 rooms have 371/371 resolving
  exits in components of 168, 10, and 1, with three reciprocal boundaries.
  The all-95 inactive-WLD scan found no room overlap or approach; the curated
  source-only total remains 1092 and the reviewed Surf subset remains 121.
  Typed closure classifies Caertannad sidecar giver references and four active
  Heavens resets for key #5504; compiled closure classifies the dynamic portal
  and active local bindings. The title is unique, bounded history identifies
  no replacement, and the Ruin remains **DO NOT MOVE**. See the [audit
  supplement](THARNADIAN_RUIN_AUDIT_REVIEW.md) and [coverage row](ZONE_COVERAGE_REGISTER.csv#L298).
  The queue is now 129/220, with 91 pending and The Forest City of Aravne
  next. No candidate is approved to move. See
  [ATTIC_MIGRATION_PLAN.md](ATTIC_MIGRATION_PLAN.md).

- **Checkpoint 90 — The Forest City of Aravne:** 438 rooms have 923/923
  resolving exits in one component, with one reciprocal Surface boundary and
  a Winterhaven-only inbound. The all-95 inactive-WLD scan found one
  non-overlapping `surf.wld` approach; the curated source-only total is 1093
  and the reviewed Surf subset is 122. Typed closure records Airship Grave's
  sand QST and sidecar references, Nax's local-object shop product, and three
  foreign resets. Compiled closure distinguishes the dynamic death and epic
  teacher bindings from static profiles and inactive code. The title is unique,
  bounded history identifies no replacement, and Aravne remains **DO NOT
  MOVE**. See the [audit supplement](FOREST_CITY_OF_ARAVNE_AUDIT_REVIEW.md)
  and [coverage row](ZONE_COVERAGE_REGISTER.csv#L298). The queue is now
  130/220, with 90 pending and The Great Shaboath next. No candidate is
  approved to move. See [ATTIC_MIGRATION_PLAN.md](ATTIC_MIGRATION_PLAN.md).

- **Checkpoint 91 — The Great Shaboath:** 116 rooms have 243/243 resolving
  exits in components of 115 and one. Three active inbounds and one outbound
  form a reciprocal Underdark pair plus two inbound-only edges. The all-95
  inactive-WLD scan found no room overlap or approach; the curated source-only
  total remains 1093 and the reviewed Surf subset remains 122. Typed closure
  classifies two active foreign resets for local objects #32856 and #32857;
  no external QST item command, SHP product, prototype, type-25 target, or
  structured sidecar reference matched. The title is unique, bounded history
  identifies no replacement, and Shaboath remains **DO NOT MOVE**. See the
  [audit supplement](GREAT_SHABOATH_AUDIT_REVIEW.md), [coverage
  row](ZONE_COVERAGE_REGISTER.csv#L60), and [connectivity
  section](WORLD_CONNECTIVITY.md). The queue is now 131/220, with 89 pending
  and The Altar of the Firesworn next. No candidate is approved to move. See
  [ATTIC_MIGRATION_PLAN.md](ATTIC_MIGRATION_PLAN.md).

- **Checkpoint 92 — The Altar of the Firesworn:** 71 rooms have 259/259
  resolving exits in components of 44 and 27, with no active WLD boundary. The
  all-95 inactive-WLD scan found 284 `Duris3.wld` target records: 144
  non-overlapping map exits cover all 71 rooms, and 71 same-numbered source
  rooms contribute 140 additional matches. They are source-only map
  declarations, not active ingress; the curated source-only total is now 1237
  and the reviewed Surf subset remains 122. Typed closure finds one Heavens
  bookcase reset for local object #135129 and no external QST item command,
  SHP product, prototype, type-25 target, or structured sidecar reference. The
  title is unique, bounded history identifies no replacement, and Firesworn
  remains **DO NOT MOVE**. See the [audit supplement]
  (ALTAR_OF_THE_FIRESWORN_AUDIT_REVIEW.md), [coverage
  row](ZONE_COVERAGE_REGISTER.csv#L97), and [connectivity
  section](WORLD_CONNECTIVITY.md). The queue is now 132/220, with 88 pending
  and The Royal Mausoleum of Castle IceCrag next. No candidate is approved to
  move. See [ATTIC_MIGRATION_PLAN.md](ATTIC_MIGRATION_PLAN.md).

- **Checkpoint 93 — The Royal Mausoleum of Castle IceCrag:** 133 rooms have
  286/286 resolving exits in one weak component, with one reciprocal active
  IceCrag boundary. The all-95 inactive-WLD scan found 532 `Duris3.wld` target
  records: 268 non-overlapping map exits cover all 133 rooms, while 133
  same-numbered source rooms account for 264 additional matches. These are
  source-only map declarations, not active ingress; the curated total is now
  1505 and the reviewed Surf subset remains 122. Typed closure finds no
  external QST item command, SHP product, reset-role consumer, matching
  prototype, or type-25 target. Sidecars have no structured local VNUM; 24
  direct C/C++ source matches are eight local equipment IDs in the chaos tables. The
  title is unique, followed history identifies no distinct replacement, and
  the area remains **DO NOT MOVE**. See the [audit supplement]
  (ROYAL_MAUSOLEUM_OF_CASTLE_ICECRAG_AUDIT_REVIEW.md), [coverage
  row](ZONE_COVERAGE_REGISTER.csv#L340), and [connectivity
  section](WORLD_CONNECTIVITY.md). The queue is now 133/220, with 87 pending
  and Negative Material Plane (`negplane`) next. No candidate is approved to
  move. See [ATTIC_MIGRATION_PLAN.md](ATTIC_MIGRATION_PLAN.md).

- **Checkpoint 94 — Negative Material Plane:** 270 rooms have 999/999
  resolving exits in five weak components, with two one-way active WLD
  inbounds and no outbounds. The all-95 inactive-WLD scan found no approach
  or source-room overlap; the curated source-only total remains 1,505 and the
  reviewed Surf subset remains 122. Typed scans classify nine external QST
  consumers of orb #26614, Alatorin's #26631 reward, five active foreign reset
  records, and one active Ravenloft2 type-25 portal into room #26600. All 231
  external sidecars parse clean; the deeper token scan found 40 numeric-token
  occurrences across ten sidecars, mostly mirrored completion and step
  identifiers. The C/C++ scan classified 30 exact numeric matches. The title
  is unique, followed history identifies no distinct predecessor or
  replacement, and the area remains **DO NOT MOVE**.
  See the [audit supplement](NEGATIVE_MATERIAL_PLANE_AUDIT_REVIEW.md),
  [coverage row](ZONE_COVERAGE_REGISTER.csv#L200), and [connectivity
  section](WORLD_CONNECTIVITY.md). The queue is now 134/220, with 86 pending
  and The Prisons of Carthapia (`prison`) next. No candidate is approved to
  move. See [ATTIC_MIGRATION_PLAN.md](ATTIC_MIGRATION_PLAN.md).

- **Checkpoint 95 — The Prisons of Carthapia:** 199 rooms have 455/455
  resolving exits in components of 166, 17, and 16 rooms. Surface has the only
  active inbound and a reciprocal return; 35 other active exits leave seven
  planar rooms without authored WLD returns. The all-95 inactive-WLD scan
  found one non-overlapping `surf.wld` approach to #7497, not active ingress;
  the curated source-only total is now 1,506 and the reviewed Surf subset is
  123. Typed scans found 13 active foreign reset-role references, two
  source-only Nexus56 resets, and one source-only shop product. There is no
  external QST item command, external prototype, structured sidecar VNUM, or
  type-25 destination. The C/C++ scan returned 12 match lines across eight
  files; selected bindings were context-classified. The title is unique and
  followed history identifies no distinct predecessor or replacement. Prison
  remains **DO NOT MOVE**. See the [audit supplement]
  (PRISONS_OF_CARTHAPIA_AUDIT_REVIEW.md), [coverage row]
  (ZONE_COVERAGE_REGISTER.csv#L240), and [connectivity section]
  (WORLD_CONNECTIVITY.md). The queue is now 135/220, with 85 pending and Pits
  of Cerberus (`cerebusp`) next. No candidate is approved to move. See
  [ATTIC_MIGRATION_PLAN.md](ATTIC_MIGRATION_PLAN.md).

- **Checkpoint 96 — Pits of Cerberus:** 156 rooms have 323/323 resolving
  exits in components of 51, 36, 35, 20, 13, and 1 room. Five active WLD
  inbounds include three reciprocal Surface pairs and one-way MIRA/Spshold
  entrances. The all-95 inactive-WLD scan found two non-overlapping Surf
  approaches and one Underworld2 approach, not active ingress; the curated
  source-only total is now 1,509 and the reviewed Surf subset is 125. Typed
  scans found three active external QST commands, one SHP product, three
  active reset-role references, two active portal targets, and two sidecars
  with typed item-VNUM matches. The C/C++ scan returned 34 match lines across
  seven files. Local `cerebusp.zon` also resets four imported objects (#358,
  #55452, #40774, and #76713); local QST, SHP, and typed-sidecar item
  references add no outbound prototype dependencies. The title is unique, and
  followed history finds no distinct predecessor or replacement. Pits remains
  **DO NOT MOVE**. See the [audit
  supplement](PITS_OF_CERBERUS_AUDIT_REVIEW.md), [coverage
  row](ZONE_COVERAGE_REGISTER.csv#L54), and [connectivity
  section](WORLD_CONNECTIVITY.md). The queue is now 136/220, with 84 pending
  and Kimordril (`kimordril`) next. No candidate is approved to move. See
  [ATTIC_MIGRATION_PLAN.md](ATTIC_MIGRATION_PLAN.md).

- **Checkpoint 97 — Kimordril:** 159 rooms have 351/351 resolving exits in
  one weak component; the three active Surface pairs are reciprocal. The two
  non-overlapping `surf.wld` approaches were already counted in the reviewed
  Surf subset of 125 and curated source-only total of 1,509. Typed scans map
  two active Alatorin QST commands, one active Player Castles shop, twelve
  active foreign ZON reset-role rows, two typed Alatorin sidecar references,
  and eighteen context-classified C/C++ matches. Source-only Nexus56 and
  Player Guilds SHP/ZON records also reference Kimordril objects; their
  conditional shop-reader fallback is captured in the candidate plans.
  The normalized title is unique; followed history does not establish a
  predecessor or replacement. Kimordril remains **DO NOT MOVE**. See the
  [audit supplement](KIMORDRIL_AUDIT_REVIEW.md), [coverage
  row](ZONE_COVERAGE_REGISTER.csv#L146), and [connectivity
  section](WORLD_CONNECTIVITY.md). At Checkpoint 97, the priority audit queue
  stood at 137/220, with 83 pending and Shady Grove (`shady`) next. No
  candidate is approved to move; see [ATTIC_MIGRATION_PLAN.md](ATTIC_MIGRATION_PLAN.md).

- **Checkpoint 98 — Shady Grove:** 258 rooms have 561/561 resolving active
  exits in a 257-room weak component plus isolated pet-shop stock room #97758;
  five active inbounds include two one-way Heavens arrivals and three paired
  Surface routes. Three new non-overlapping source-only `surf.wld` approaches
  raise the reviewed Surf subset to 128 and curated source-only total to 1,512.
  Typed scans add the active `end.shp` location reference, Alatorin item
  references, six foreign mobile resets, and source-only Nexus56 stock/reset
  use. The unregistered `shady2.mob` has 90 overlapping mobile VNUMs with five
  differing alias lists; no predecessor or replacement is established. Shady
  remains **DO NOT MOVE**. See the [audit supplement](SHADY_GROVE_AUDIT_REVIEW.md),
  [coverage row](ZONE_COVERAGE_REGISTER.csv#L268), [connectivity report](WORLD_CONNECTIVITY.md),
  and [attic plans](ATTIC_MIGRATION_PLAN.md). The priority audit queue is now
  138/220, with 82 pending and Apocalypse Castle (`4horse`) next. No candidate
  is approved to move.

- **Checkpoint 99 — Apocalypse Castle:** 312 rooms have 777/777 resolving
  active exits in three weak components (254, 57, and 1 rooms). The Surface
  boundary is reciprocal; a separate Realm exit arrives at vault #34804. One
  new non-overlapping source-only `surf.wld` approach raises the reviewed Surf
  subset to 129 and curated source-only total to 1,513. Typed scans reconcile
  active QST and sidecar inputs, thirteen foreign reset-role rows, imported
  reset prototypes, and the followed history/title boundary. Apocalypse
  remains **DO NOT MOVE**. See the [audit
  supplement](APOCALYPSE_CASTLE_AUDIT_REVIEW.md), [coverage
  row](ZONE_COVERAGE_REGISTER.csv#L2), [connectivity
  report](WORLD_CONNECTIVITY.md), and [attic plans](ATTIC_MIGRATION_PLAN.md).
  The priority audit queue is now 139/220, with 81 pending and Ixxillikor
  (`ixxillikor`) next. No candidate is approved to move.

- **Checkpoint 100 — Ixxillikor:** 199 rooms have 456/456 resolving exits in
  one weak component, with four reciprocal active boundaries, one fall marker,
  and no currents. The all-95 inactive-WLD scan found no overlap or approach.
  Typed closure records the local Verz1/Ixarkon reset imports, effective epic
  bindings, chaos-equipment use, and legacy QST/story ingredients; no outside
  typed consumer or external prototype/portal match was found. The unique
  active title and followed history establish no distinct predecessor or
  replacement. Ixxillikor remains **DO NOT MOVE**. See the [audit
  supplement](IXXILLIKOR_AUDIT_REVIEW.md), [coverage
  row](ZONE_COVERAGE_REGISTER.csv#L136), and [connectivity
  report](WORLD_CONNECTIVITY.md). The priority queue is now 140/220, with 80
  pending and Moonshae Island (`moonshae`) next; no candidate is approved to
  move.

- **Checkpoint 101 — Moonshae Island:** reconciled 127 rooms/266 exits, with
  264 loader-resolving exits in one 127-room weak component, five reciprocal
  active boundaries, and two Port #26200 exits pruned because their targets
  occur only in inactive Duris3. The all-95 inactive-WLD sweep found one
  non-overlapping source-only Surf approach (#219584 west to #26208), raising
  the reviewed Surf subset to 130 and curated source-only total to 1,514.
  Typed closure records the active Black Pearl QST/sidecar consumer, Cityruin
  chance-20 reset reference, Myrloch Vale waterfall, missing local reset object
  #11100, and source-only Nexus56 references. The normalized title is unique;
  tracked history establishes no predecessor or replacement. Moonshae remains
  **DO NOT MOVE**. See the [audit supplement](MOONSHAE_ISLAND_AUDIT_REVIEW.md),
  [coverage row](ZONE_COVERAGE_REGISTER.csv), [connectivity report](WORLD_CONNECTIVITY.md),
  and [Surf/Duris3 candidate plans](ATTIC_MIGRATION_PLAN.md).

- **Checkpoint 102 — Prison of Fort Boyard:** reconciled 52 rooms and 108/108
  resolving exits (106 local, two outbound), two reciprocal Fort Boyard
  boundaries, and weak components of 23, 22, and 7 rooms. The all-95
  inactive-WLD scan found no source-room overlap or approach. Typed closure
  records Fortb's active reset of local ship #43014; the WH memory-of-Fort-
  Boyard card is a separate narrative reference whose item and recipient are
  WH prototypes. The normalized title is unique; tracked history establishes
  no predecessor or replacement, and active Carthapia `prison` remains a
  distinct zone. Prisonb remains **DO NOT MOVE** with fourteen builder
  follow-ups; no native repair ships. See the [audit
  supplement](PRISON_OF_FORT_BOYARD_AUDIT_REVIEW.md), [coverage
  row](ZONE_COVERAGE_REGISTER.csv), and [connectivity report](WORLD_CONNECTIVITY.md).
  The queue is 142/220 with 78 pending; Lair of the Purple Worm (`pworm`) is
  next.

- **Checkpoint 103 — Lair of the Purple Worm:** reconciled 98 rooms and
  216/216 resolving exits in one 98-room weak component, with four reciprocal
  Underdark/Underworld boundaries. The all-95 inactive-WLD sweep found three
  non-overlapping source-only `ud_map2.wld` approaches (#1123784 west to
  #42525, #1123975 east to #42542, #1124682 north to #42500); the curated
  source-only total is 1,517 and these are not active ingress. Typed review
  records nine active Alatorin M reset rows for local mobile #42501. No
  external typed QST/SHP/sidecar item consumer, prototype duplicate, or type-25
  target into `pworm` was found. The normalized title is unique; tracked
  history shows no predecessor or replacement, and active The Caverns of the
  Worms is a separate area. `pworm` remains **DO NOT MOVE** with fifteen
  builder follow-ups; the `ud_map2` candidate remains **RETAIN**. See the
  [audit supplement](LAIR_OF_THE_PURPLE_WORM_AUDIT_REVIEW.md), [coverage
  row](ZONE_COVERAGE_REGISTER.csv), [connectivity report](WORLD_CONNECTIVITY.md),
  and [attic plan](ATTIC_MIGRATION_PLAN.md). The queue is 143/220 with 77
  pending; Vargan (`vargan`) is next.

- **Checkpoint 104 — Vargan:** reconciled 94 rooms and 239/239 resolving exits
  in one 94-room weak component, with three reciprocal Surface/Vargan II
  boundaries. The all-95 inactive-WLD sweep found one non-overlapping
  source-only Surf approach (#266321 east to #2100); reviewed Surf rises to
  131 and the curated source-only total to 1,518. Typed review found no
  external QST item, SHP product, ZON reset-role, sidecar item, duplicate
  prototype, or type-25 target into Vargan. The normalized title is unique;
  tracked history establishes no predecessor or replacement, while active
  `v2` is a distinct connected area. Vargan remains **DO NOT MOVE** and `surf`
  remains **RETAIN**. The dossier now has fourteen builder follow-ups; no
  native repair or area move ships. See the [audit
  supplement](VARGAN_AUDIT_REVIEW.md), [coverage
  row](ZONE_COVERAGE_REGISTER.csv), [connectivity report](WORLD_CONNECTIVITY.md),
  and [attic plan](ATTIC_MIGRATION_PLAN.md). The queue is 144/220 with 76
  pending; The Maze of Undead Army (`maze_are`) is next.

- **Checkpoint 105 — The Maze of Undead Army:** reconciled 177 rooms and
  395/395 resolving exits, with 394 local/one outbound and one 177-room weak
  component. Directed authored exits from entrance #94177 reach 173 rooms; its
  only active boundary is reciprocal with Surface #571011. The all-95
  inactive-WLD sweep found one non-overlapping source-only `surf.wld` edge
  (#281856 east to #94177), raising the reviewed Surf subset to 132 and the
  curated source-only total to 1,519. Typed review records a separate inactive
  `brass-old-1.zon` G reset for Maze object #94009, no external sidecar/prototype/
  type-25 target, and the existing Spellbind binding to Kalroh. The normalized
  title is unique; tracked paths show no alternate stem, predecessor, or
  replacement. Maze remains **DO NOT MOVE**; `surf` and `brass-old-1` remain
  **RETAIN** pending their own closure. The dossier now has eighteen builder
  follow-ups. No native repair or source move ships. See the [Maze audit
  supplement](MAZE_OF_UNDEAD_ARMY_AUDIT_REVIEW.md), [coverage
  row](ZONE_COVERAGE_REGISTER.csv), [connectivity report](WORLD_CONNECTIVITY.md),
  and [attic plan](ATTIC_MIGRATION_PLAN.md). The queue is 145/220 with 75
  pending; Bandit Camp (`banditca`) is next.


- **Checkpoint 106 — Bandit Camp:** reconciled 127 rooms and 311/311
  resolving exits, with 310 local/one outbound and one 127-room weak component;
  directed exits from #14800 reach all Camp rooms. The active boundary set is
  the reciprocal Surface pair plus a one-way WH Incarnate dispersal edge. The
  all-95 inactive-WLD sweep found one non-overlapping `surf.wld` approach
  (#220524 north to #14800), raising the reviewed Surf subset to 133 and the
  curated source-only total to 1,520. Typed closure records the Tharnadian
  insignia and WH ale reset/shop, imported object #67262, and no outside QST
  item, exact sidecar VNUM, duplicate prototype, type-25 target, or direct
  numeric code binding. “Bandit Camp” is unique; `bandit` / The Bandit Canyons
  is a separate active identity with no established replacement relationship.
  Bandit Camp remains **DO NOT MOVE** and `surf` remains **RETAIN**. The dossier
  now has eighteen builder follow-ups; no native repair or source move ships.
  See the [audit supplement](BANDIT_CAMP_AUDIT_REVIEW.md), [coverage
  row](ZONE_COVERAGE_REGISTER.csv), [connectivity report](WORLD_CONNECTIVITY.md),
  [history register](DUPLICATE_AND_REPLACED_ZONES.md), and [attic
  plan](ATTIC_MIGRATION_PLAN.md). The queue is 146/220 with 74 pending;
  Myrloch Vale (`myrloch_vale`) is next.

- **Checkpoint 107 — Myrloch Vale:** reconciled 178 rooms and 373/373 resolving
  exits: 372 local and one outbound. The active boundary is reciprocal with
  Moonshae #26311; the local weak components contain 165 and 13 rooms, while
  directed authored exits from #26401 reach 13 rooms. The all-95 inactive-WLD
  scan found no approach into Myrloch; the reviewed Surf subset remains 133 and
  curated source-only total remains 1,520. Typed closure records Caves Q155 and
  sidecar shield #26438, imported reset objects #358/#55455, and local code
  bindings; no other outside exact quest, shop, reset, prototype, sidecar, or
  portal target was found. The unique active title and tracked histories show
  no explicit predecessor or replacement. Myrloch Vale remains **DO NOT MOVE**.
  Its dossier now has eighteen builder follow-ups. No native repair or area
  move ships. The queue is 147/220 with 73 pending; Cloud Giant Kingdom
  (`cldgt`) is next. See the [audit supplement](MYRLOCH_VALE_AUDIT_REVIEW.md),
  [coverage row](ZONE_COVERAGE_REGISTER.csv), [connectivity report]
  (WORLD_CONNECTIVITY.md), and [history register](DUPLICATE_AND_REPLACED_ZONES.md).

- **Checkpoint 108 — Cloud Giant Kingdom:** reconciled 181 rooms and 391/391
  resolving exits: 390 local and one outbound. The boundary is reciprocal with
  Surface #553314. Directed authored exits from #99500 reach all 181 rooms
  when ordinary gates are available. The all-95 inactive-WLD scan found one
  non-overlapping source-only `surf.wld` approach (#220537 west to #99500),
  raising the reviewed Surf subset to 134 and the curated source-only total to
  1,521. Typed closure records Q68’s heart/orb/tablet inputs and scroll output,
  the Empower Song teacher binding, and the Chaos-kit sleeves reference; no
  outside exact inbound quest/shop/reset/sidecar/prototype/portal target was
  found. The unique title and followed paths establish no explicit predecessor
  or replacement. Cloud Giant remains **DO NOT MOVE**; the dossier now has
  nineteen builder follow-ups. No native repair or area move ships. Queue: 148/220
  with 72 pending; The Mountain of the Banished (`mount`) is next. See the
  [audit supplement](CLOUD_GIANT_KINGDOM_AUDIT_REVIEW.md), [coverage row]
  (ZONE_COVERAGE_REGISTER.csv), [connectivity report](WORLD_CONNECTIVITY.md),
  [history register](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan]
  (ATTIC_MIGRATION_PLAN.md).

- **Checkpoint 109 — The Mountain of the Banished:** reconciled 67 rooms and
  135/135 resolving exits, with 134 local, one outbound, three inbound and one
  reciprocal Surface pair. The all-95 inactive-WLD sweep found non-overlapping
  source-only approaches from `oldmap8.wld` #184183 east and `surf.wld` #256676
  west into #9100. These are not active ingress; the reviewed Surf subset is
  135 and the curated source-only total is 1,523. Typed closure records four
  external reset imports and two direct shaman-kit uses of local object #9114;
  no other bounded external typed inbound target was found. The unique active
  title and followed paths show no explicit predecessor or replacement.
  Mountain remains **DO NOT MOVE**; preserve `oldmap8`, `surf`, and
  `surface2011` as **RETAIN** pending map ownership. The dossier now has
  nineteen builder follow-ups. No native repair or area move ships. Queue:
  149/220 with 71 pending; Tiamat (`tiamat`) is next. See the [audit supplement]
  (MOUNTAIN_OF_THE_BANISHED_AUDIT_REVIEW.md), [coverage row]
  (ZONE_COVERAGE_REGISTER.csv), [connectivity report](WORLD_CONNECTIVITY.md),
  [history register](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan]
  (ATTIC_MIGRATION_PLAN.md).

- **Checkpoint 110 — Tiamat:** reconciled 39 rooms and 116/116 resolving exits,
  with 109 local, seven outbound and two inbound active WLD boundaries. The
  Avernus entrance is reciprocal; Hell One of Avernus is one-way. Directed
  authored-local exits from #19600 reach 23/39 rooms with gates assumed
  passable. The all-95 inactive-WLD sweep found no approach or room overlap.
  Typed review records the Astral Tiamat pool target and outside barb reset,
  nine external reset object prototypes plus mobile #19700, and the stale
  #25106 reset reference to inactive `brass-old-3.obj`; the loader disables
  that unresolved command. No other bounded external QST/SHP/sidecar/prototype
  inbound match was found. The unique title and followed histories establish
  no predecessor or replacement. Tiamat remains **DO NOT MOVE**; keep
  `brass-old-3` **RETAIN** pending owner review. Its existing dossier retains
  twenty builder follow-ups. No native repair or area move ships. Queue:
  150/220 with 70 pending; The Obsidian Citadel (`obcita`) is next. See the
  [audit supplement](TIAMAT_AUDIT_REVIEW.md), [coverage row]
  (ZONE_COVERAGE_REGISTER.csv), [connectivity report](WORLD_CONNECTIVITY.md),
  [history register](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan]
  (ATTIC_MIGRATION_PLAN.md).

The source commit and full/minimal/random/index boundaries are frozen for every
row in [`ZONE_COVERAGE_REGISTER.csv`](ZONE_COVERAGE_REGISTER.csv). Each future
research batch should append evidence and update its specific status rather
than replacing `PENDING` with a blanket reviewed label.

### Checkpoint 111 — The Obsidian Citadel

Reconciled 179 rooms and 382/382 resolving exits: 379 local and three outbound, in one component. Three active inbounds/outbounds form reciprocal Surface and two Underdark pairs; the open-exit graph reaches all 179 rooms from #75600, while the dossier separately records gate-qualified reach. The all-95 inactive-WLD sweep found source-only `surf.wld` #262357 north to #75600 with zero room overlap; Surf subset 136 and curated source-only total 1,524. Typed review records active Heavens/Winterhaven imports #359/#55188 and Ravenloft resets to local #75638/#75669, with no other bounded external QST/SHP/sidecar/prototype/type-25 target. The exact title is unique and followed paths show no explicit predecessor or replacement. Keep Obcita **DO NOT MOVE** and `surf`/`surface2011` **RETAIN**. Queue: 151/220 complete, 69 pending; The Chasm of the Misty Vale (`mist_chasm`) is next. See the [audit supplement](OBCITA_AUDIT_REVIEW.md), [coverage row](ZONE_COVERAGE_REGISTER.csv#L221), [connectivity report](WORLD_CONNECTIVITY.md), and [attic plan](ATTIC_MIGRATION_PLAN.md).

Documentation validation passed for 46 changed Markdown/CSV files: 2,703 local links and 634 line anchors; CSV widths and targets passed, and `git diff --check` passed. No tests, build, server, gameplay session, database, migration, or operational script was run for this documentation checkpoint.

### Checkpoint 112 — The Chasm of the Misty Vale

Reconciled the active 100-room/209-exit graph, its reciprocal Surface boundary, and two non-overlapping inactive-WLD approaches from surf.wld and torr.wld. The passable-exit directed graph reaches and returns from all 100 rooms. Typed closure records exact Surface and Winterhaven uses of local objects, imported object #67273, no local SHP product, and no target among 872 active type-25 declarations. The normalized active title is unique and followed paths contain no explicit predecessor or replacement claim. Keep active mist_chasm **DO NOT MOVE** and surf, surface2011, and torr **RETAIN**. Queue: 152/220 complete, 68 pending; Varathorn Keep (kastle) is next. See the [audit supplement](MIST_CHASM_AUDIT_REVIEW.md), [coverage row](ZONE_COVERAGE_REGISTER.csv#L221), [connectivity report](WORLD_CONNECTIVITY.md), [history register](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan](ATTIC_MIGRATION_PLAN.md).

Checkpoint 112 documentation validation passed for 29 changed or added Markdown/CSV files: 2,303 local links, 459 line anchors, CSV widths/targets, and git diff --check. No tests, build, server, gameplay session, database, migration, or operational script was run for this documentation checkpoint.

### Checkpoint 113 — Varathorn Keep

Reconciled the active 100-room/222-exit graph, two reciprocal Surface pairs, and two non-overlapping inactive surf.wld approaches. The directed passable-exit graph from #99400 reaches 92 rooms and has authored returns from 99; room #99499 lacks a return. Typed closure records the two external talon inputs, Tharnadia's quill exchange, Raxthan/Surface Keeps/Valdrak stock and resets, and five Alatorin resets of mobile #99460. The normalized title is unique, and followed paths show no explicit predecessor or replacement. Keep active kastle **DO NOT MOVE** and surf/surface2011 **RETAIN**. Queue: 153/220 complete, 67 pending; Malch'Hor Ganl the Goblin City (malch) is next. See the [audit supplement](KASTLE_AUDIT_REVIEW.md), [coverage row](ZONE_COVERAGE_REGISTER.csv#L142), [connectivity report](WORLD_CONNECTIVITY.md), [history register](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan](ATTIC_MIGRATION_PLAN.md).

Checkpoint 113 documentation validation passed for 29 changed or added Markdown/CSV files: 2,352 local links, 496 line anchors, CSV widths/targets, and git diff --check. No tests, build, server, gameplay session, database, migration, or operational script was run for this documentation checkpoint.

### Checkpoint 114 — Malch'Hor Ganl the Goblin City

Reconciled Malch’s 161-room/360-exit active graph, four reciprocal Surface/Underdark/Troll Caves/Shipyards pairs, and the one non-overlapping inactive `surf.wld` approach. The directed source graph reaches and returns from 159 rooms with blocked controls unavailable and all 161 when the paired controls are allowed. Typed closure records external QST items #93901/#43131, reset imports #53610/#4410/#4404/#15213/#40474, and outside resets of local objects #23616/#23620; no outside exact QST item/SHP product, external prototype duplicate, typed sidecar VNUM, or type-25 target into Malch was found. The exact title is unique, followed paths show no explicit predecessor or replacement, and the coverage row now records all three shops. Keep `malch` **DO NOT MOVE** and `surf` / `surface2011` **RETAIN**. Queue: 154/220 complete, 66 pending; Miaeril Village (`mril`) is next. See the [audit supplement](MALCH_AUDIT_REVIEW.md), [coverage row](ZONE_COVERAGE_REGISTER.csv#L169), [connectivity report](WORLD_CONNECTIVITY.md), [history register](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan](ATTIC_MIGRATION_PLAN.md).

Checkpoint 114 documentation validation checked 50 changed/added Markdown files and two changed CSV registers: 2,747 local links and 765 line anchors; CSV widths (60 and 10 columns), 476 coverage records, 121 attic-candidate records, and local targets passed. `git diff --check` passed. No tests, build, server, gameplay session, database, migration, or operational script was run.

### Checkpoint 115 — Miaeril Village

Added the repository-wide [Miaeril audit supplement](MIAERIL_AUDIT_REVIEW.md) to its existing comprehensive story dossier. The active graph has 65 rooms and 190/190 resolving exits: 188 local, two outbound to Surface #522257, and one reverse edge. Directed authored traversal reaches 63 rooms from the tested entrances with the stump route unavailable and all 65 when all authored exits are treated as available; all 65 have authored returns. The all-95 inactive-WLD scan found no approach or room overlap, so the curated source-only total remains 1,529 and Surf subset remains 140. Typed closure classifies reset-imported Dream object #31317, the unrelated Winterhaven numeric collision on #33719, and Chaos starter-kit item numbers. Miaeril's `.shp` inventory is corrected to one shop (#33714). The normalized title is unique; followed paths show no alternate stem or explicit predecessor/replacement. Keep `mril` **DO NOT MOVE** and `surf` / `surface2011` **RETAIN**. No gameplay or source placement changed. The broader repository-wide queue is 155/220 complete, 65 pending; The Temple of the Sun (`suntmpl`) is next. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L193), [connectivity report](WORLD_CONNECTIVITY.md), [history register](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan](ATTIC_MIGRATION_PLAN.md).

Checkpoint 115 documentation validation checked 54 changed/added Markdown and CSV files: 2,995 local links and 808 line anchors. The 60-column coverage register has 477 records, the 10-column attic-candidate register has 122 records, all local targets and CSV widths passed, and `git diff --check` passed. No tests, build, server, gameplay session, database, migration, or operational script was run.

### Checkpoint 116 — The Temple of the Sun

Added the repository-wide [Temple audit supplement](TEMPLE_OF_THE_SUN_AUDIT_REVIEW.md) to its existing comprehensive story dossier. The active graph has 94 rooms and 217/217 resolving exits: 216 local and one outbound, paired with Surface #541275. Authored directed traversal from #82493 reaches all 94 and returns under an all-exits-available assumption; the dossier separately records key/search/pick gated layers. The all-95 inactive-WLD scan found one non-overlapping `surf.wld` approach into #82493, raising the reviewed Surf subset to 141 and curated source-only total to 1,530. Typed closure records active Heaven/unique reset imports, Winterhaven's QST and story-sidecar use of local ring #82405, Frolikk's epic-teacher code binding, and an unregistered `brass-old-1.zon` reset of local object #82416. No other bounded typed inbound target, external duplicate prototype, or type-25 arrival was found; the Temple has no shop file. The normalized title is unique and followed paths show no alternate stem or explicit predecessor/replacement. Keep `suntmpl` **DO NOT MOVE**, `surf`/`surface2011` **RETAIN**, and preserve the `brass-old-1` candidate. No gameplay or source placement changed. The broader reconciliation queue is 156/220 complete, 64 pending; Lair of the Gibberling King (`gibber`) is next. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L285), [connectivity report](WORLD_CONNECTIVITY.md), [history register](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan](ATTIC_MIGRATION_PLAN.md).

Checkpoint 116 documentation validation checked 56 changed/added Markdown and CSV files: 3,054 local links and 845 line anchors. The 60-column coverage register has 477 records, the 10-column attic-candidate register has 122 records, all local targets and CSV widths passed, and `git diff --check` passed. No tests, build, server, gameplay session, database, migration, or operational script was run.

### Checkpoint 117 — Lair of the Gibberling King

Added the repository-wide [Gibber audit supplement](GIBBERLING_KING_AUDIT_REVIEW.md) to the existing comprehensive story dossier. The active graph has 122 rooms and 406/406 resolving exits (404 local, two outbound) in one weak component; two reciprocal Underdark pairs and a one-way Realm edge enter from three active records. The dossier preserves gated search/key/pick/push access tiers and the alternate arrival. The all-95 inactive-WLD scan found one non-overlapping `ud_map2.wld` approach into #8700, taking the curated source-only total to 1,531; the Surf subset remains 141. `ud_map2` now has eight approaches, all source-only. Typed closure records the Airp item #8735 consumer, Heavens #359 and Winterhaven #55190 reset imports, and the separate WH memory receipt. SurfaceKeeps #8750 is an experience reward; the `randomeq.c` #8746 use is dormant because its only consumer is commented out. No other bounded outside QST item, reset-role, SHP product, duplicate prototype, typed sidecar item, type-25 target, or local shop was found. The normalized title is unique; followed paths show no alternate stem or explicit predecessor/replacement. Keep `gibber` **DO NOT MOVE** and `ud_map2` **RETAIN**. No gameplay or source placement changed. The broader reconciliation queue is 157/220 complete, 63 pending; Ice Tower (`icetower`) is next. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L104), [connectivity report](WORLD_CONNECTIVITY.md), [history register](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan](ATTIC_MIGRATION_PLAN.md).

Checkpoint 117 documentation validation checked 56 changed/added Markdown files and two changed CSV registers: 3,110 local links and 906 line anchors. The 60-column coverage register has 477 rows including its header, and the 10-column attic-candidate register has 122 rows including its header; all local targets and anchors passed. `git diff --check` passed. No tests, build, server, gameplay session, database, migration, or operational script was run.

### Checkpoint 118 — Ice Tower

Added the repository-wide [Ice Tower audit supplement](ICE_TOWER_AUDIT_REVIEW.md) to the existing source-comprehensive dossier. The active graph has 162 rooms and 359/359 resolving exits: 358 local and one outbound, paired with Surface #523117; all 162 rooms are in one weak component. The all-95 inactive-WLD scan found one non-overlapping source-only `surf.wld` approach from #206576 north to #31800, taking the reviewed Surf subset to 142 and curated source-only total to 1,532. Typed closure records 58 other active reset-role references to imported Heavens object #359, the separate White Hills QST/story receipt for imported memory #55192, and the disabled custom assignment for #67258. No bounded outside QST item, SHP product, sidecar item, reset-role or type-25 target into local prototypes was found. The normalized title is unique and followed paths show no alternate stem or explicit predecessor/replacement. Keep `icetower` **DO NOT MOVE** and `surf` / `surface2011` **RETAIN**. No gameplay or source placement changed.

### Repository-wide audit checkpoint 119 — Durian Underdark

The current `underdark` active graph, all-95 inactive-WLD sweep, bounded typed dependency closure, exact title/history, and retention decision are reconciled in the [repository-wide supplement](DURIAN_UNDERDARK_AUDIT_REVIEW.md) and [coverage row](ZONE_COVERAGE_REGISTER.csv#L324). The inactive sweep adds no source-only approach: totals remain 1,532 overall and 142 for `surf`. The 11 active exits into candidate `underdark2011` and its #217609 source-room identity conflict remain open; retain that candidate. The 20 story-specific builder follow-ups remain in the source-comprehensive dossier. No gameplay or area placement changed.

### Repository-wide audit checkpoint 120 — Bahamut's Palace

The current `bahamut` active graph, all-95 inactive-WLD sweep, bounded typed and compiled dependency closure, exact title/history, and retention decision are reconciled in the [repository-wide supplement](BAHAMUT_AUDIT_REVIEW.md) and [coverage row](ZONE_COVERAGE_REGISTER.csv#L22). The inactive sweep adds no approach; source-only totals remain 1,532 overall and 142 for Surf. Three active entrances, one reciprocal Plane of Air return, active reset uses, CHAOS profile item references, and random-zone potion references are recorded. Keep active `bahamut` **DO NOT MOVE**. No gameplay or area placement changed.

### Repository-wide audit checkpoint 121 — Clan Stoutdorf Settlement

Added the [Clan Stoutdorf repository-wide audit supplement](DRST_AUDIT_REVIEW.md) to its existing source-comprehensive dossier. The active graph has 199 rooms and 432/432 resolving exits, with two reciprocal boundary pairs in one weak component. The all-95 inactive-WLD sweep found one non-overlapping `ud_map2.wld` approach to #34200; this source-only approach raises the curated total to 1,533 while Surf remains 142. Typed closure records Alatorin's outside QST/story use of #34226, the outside reset of #34217, local imports #358/#55451, and the enabled DwarfSlayer binding; scoped negative checks found no external duplicate prototype, type-25 target, or local shop. The title is unique and followed history shows no alternate stem or explicit predecessor/replacement. Keep `drst` **DO NOT MOVE** and `ud_map2` **RETAIN IN PLACE**. No gameplay or area placement changed. The current source-comprehensive queue is 161/220, 59 pending; The Charcoal Palace (`firep`) is next. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L82), [connectivity report](WORLD_CONNECTIVITY.md), [history register](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan](ATTIC_MIGRATION_PLAN.md).

### Repository-wide audit checkpoint 122 — The Charcoal Palace

Added the [Charcoal Palace repository-wide audit supplement](FIREP_AUDIT_REVIEW.md) to its source-comprehensive dossier. The active graph has 151 rooms and 412/412 resolving exits, one inbound/outbound pair with different endpoints, and weak components of 150 and 1 rooms. The all-95 inactive-WLD sweep found no overlap or approach; source-only totals remain 1,533 overall and 142 for Surf. Typed closure records Airp's external cloak #88304 consumer, the unregistered Brass reset of #88322, imported reset objects #61/#92121/#359, the QST output #55270, and five CHAOS kit items. Bounded history records the 2025 #88327 Sword of Fire and Flame item move from old Brass without inferring whole-zone replacement. The normalized title is unique. Keep `firep` **DO NOT MOVE** and `brass-old-1` **RETAIN**. No gameplay or area placement changed. The current source-comprehensive queue is 162/220, 58 pending; Menden-on-the-Deep (`menden`) is next. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L96), [connectivity report](WORLD_CONNECTIVITY.md), [history register](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan](ATTIC_MIGRATION_PLAN.md).

### Repository-wide audit checkpoint 123 — Menden-on-the-Deep

Added the [Menden repository-wide audit supplement](MENDEN_AUDIT_REVIEW.md) to its existing source-comprehensive dossier. The active graph has 71 rooms and 165/165 resolving exits, with four reciprocal Surface/Ocean pairs; the raw WLD graph has components of 70 and 1 rooms. The all-95 inactive-WLD sweep found two non-overlapping source-only `surf.wld` approaches into #88846, raising the reviewed Surf subset to 144 and the curated total to 1,535. Typed closure records Underworld/Minopass use of unholy relic #88807, Alatorin resets of local foods #88808/#88809/#88823, local Q19 use of holy #4402, and local shop imports #1000/#397. The normalized title is unique and followed history shows no alternate stem or explicit predecessor/replacement. Keep `menden` **DO NOT MOVE** and `surf`/`surface2011` **RETAIN**. No gameplay or area placement changed. The current source-comprehensive queue is 163/220, 57 pending; Mitashi - Capital City of the Jade Empire (`mitashi`) is next. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L174), [connectivity report](WORLD_CONNECTIVITY.md), [history register](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan](ATTIC_MIGRATION_PLAN.md).

### Repository-wide audit checkpoint 124 — Mitashi

Added the [Mitashi audit supplement](MITASHI_AUDIT_REVIEW.md) alongside the source-comprehensive dossier. The active graph has 229 rooms and 572/572 resolving exits; two inbound and two outbound cross-zone records form one reciprocal pair, and directed authored traversal reaches and returns from all 229 local rooms under available-exit assumptions. The all-95 inactive-WLD scan found 462 non-overlapping source-only `Duris3.wld` approaches and 454 additional target records from source-room VNUMs overlapping Mitashi; the curated source-only total is now 1,997 and Surf remains 144. Typed closure records Air and Savannah consumers of the six swords, the CHAOS configured references, and bounded negatives for external resets, shops, duplicate prototypes and portal arrivals. The exact normalized title is unique. Followed history identifies a `stormport.shp` component copy, not a whole-zone predecessor or replacement; preserve active `mitashi` **DO NOT MOVE** and `Duris3` **RETAIN IN PLACE**. No gameplay or placement changed. The coverage register also carries the CP121 Stoutdorf count (one `ud_map2` edge) and CP123 Menden count (two `surf` edges). The current source-comprehensive queue is 164/220, 56 pending; Priority 165, The Dark Stone Tower of the Northern Realms (`teka2`), is next. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L183), [connectivity report](WORLD_CONNECTIVITY.md), [history register](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan](ATTIC_MIGRATION_PLAN.md).

### Repository-wide audit checkpoint 125 — The Dark Stone Tower of the Northern Realms

Added the [Teka2 audit supplement](TEKA2_AUDIT_REVIEW.md) beside the comprehensive story dossier. The active graph has 98 rooms and 198/198 resolving exits; its weak components contain 85 and 13 rooms. Plain directed traversal from #75546 reaches 11/98, with reverse reach from 85/98; the separate North Lakes pair is reciprocal. One fall field and three current fields remain explicit. The all-95 inactive-WLD sweep found no room overlap or approach, leaving curated source-only totals at 1,997 overall and 144 for Surf. Typed closure records imported #358/#55454, Alatorin reset use of local #75529, DLSC's experience constant, North Lakes orientation, compiled teacher bindings and the commented mace special. The normalized title is unique and same-stem history shows no explicit predecessor/replacement. Keep `teka2` **DO NOT MOVE**. No gameplay or placement changed. The current source-comprehensive queue is 165/220, 55 pending; Priority 166, The Troll Hills (`troll_hills`), is next. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L292), [connectivity report](WORLD_CONNECTIVITY.md), and [history register](DUPLICATE_AND_REPLACED_ZONES.md).

### Repository-wide audit checkpoint 126 — The Troll Hills

Added the [Troll Hills audit supplement](TROLL_HILLS_AUDIT_REVIEW.md) beside the source-comprehensive dossier. The active graph has 148 rooms and 460/460 resolving exits, with three reciprocal boundary pairs and one separate Lortower inbound record. The all-95 inactive-WLD sweep adds two non-overlapping source-only `surf.wld` approaches into #1852 and #1871; curated totals are now 1,999 overall and 146 for Surf. Typed closure records Heavens object #283 as imported reset gear, the stale compiled mobile #1919 bridge binding, and Airp's command-zero candidate portal to #1823; scoped negative checks found no exact external QST item, SHP product, duplicate prototype, or story-item dependency. The title is unique and bounded same-stem history shows no predecessor or whole-zone replacement. Keep active `troll_hills` **DO NOT MOVE** and Surf sources **RETAIN**. No gameplay or placement changed. The queue is 166/220 complete, 54 pending; Priority 167, A Dark and Twisted Wood (`twstwd`), is next. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L315), [connectivity report](WORLD_CONNECTIVITY.md), and [history register](DUPLICATE_AND_REPLACED_ZONES.md).

Validation: Checkpoint 126 documentation validation checked 74 changed/added Markdown files and two changed CSV registers: 3,641 local links, 1,275 line anchors, CSV widths and targets passed. The 60-column coverage register has 476 data rows and the 10-column attic-candidate register has 121 data rows. `git diff --check` passed. No tests, build, server, gameplay session, database operation, migration or operational script was run.

### Repository-wide audit checkpoint 127 — A Dark and Twisted Wood

Added the [Twisted Wood audit supplement](DARK_AND_TWISTED_WOOD_AUDIT_REVIEW.md) beside the source-comprehensive dossier. The active graph has 100 rooms and 335/335 resolving exits, with four reciprocal pairs and two separate Pine Hollow NPC-load inbound links. Raw directed traversal from #16300 reaches 97/100 and has authored returns from all 100; the three outgoing-only staging rooms remain distinct. The all-95 inactive-WLD sweep adds five non-overlapping source-only `surf.wld` approaches, raising the curated total to 2,004 overall and 151 for Surf. Typed closure records the faerie's external item inputs #16014/#16016 and their Pine Hollow definitions; bounded negatives find no external QST consumer, SHP product, duplicate prototype, type-25 destination, or external story-sidecar VNUM. Compiled SHIFT PRIME has one conditional #16302 arrival, while unbound guild room cases do not establish an active class gate. The title is unique and followed same-stem history has no explicit predecessor or whole-zone replacement. Keep `twstwd` **DO NOT MOVE** and Surf sources **RETAIN**. No gameplay or placement changed. The queue is 167/220 complete, 53 pending; Priority 168, The Underworld (`underworld`), is next. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L321), [connectivity report](WORLD_CONNECTIVITY.md), and [history register](DUPLICATE_AND_REPLACED_ZONES.md).

Validation: Checkpoint 127 documentation validation checked 76 changed/added Markdown files and two changed CSV registers: 3,698 local links, 1,314 line anchors, CSV widths and targets passed. The 60-column coverage register has 476 data rows and the 10-column attic-candidate register has 121 data rows. `git diff --check` passed. No tests, build, server, gameplay session, database operation, migration or operational script was run.

### Repository-wide audit checkpoint 128 — The Underworld

Added the [Underworld repository-wide audit supplement](UNDERWORLD_AUDIT_REVIEW.md) beside the source-comprehensive dossier. The active graph has 232 rooms and 511 authored exits; 510 resolve (484 local, 26 valid outbound, and one unresolved #4645 north to #4646). Its weak components contain 154, 70, 6, 1, and 1 rooms; raw directed traversal from #4401 reaches and returns from 154. The 30 inbound, 26 outbound, and 26 reverse boundary records include four inbound-only links. There are 13 current fields and no fall fields. The all-95 inactive-WLD scan adds three non-overlapping source-only approaches from `ud_map0` and `ud_map2`; curated totals are now 2,007 overall and 151 for Surf. Typed closure records imported #88807, outside QST consumers of local #4402/#4505/#4660, foreign sidecar links, and 533 active/attic mobile reset references; scoped negatives include no external SHP product, local SHP, type-25 destination, or duplicate mobile/object prototype. The exact title is shared by distinct active `underworld` and `underworld3` sources; bounded history finds no explicit replacement. Keep active `underworld` **DO NOT MOVE**, retain the attic sources, and preserve `underworld3` independently. No gameplay or placement changed. The queue is 168/220 complete, 52 pending; Priority 169, Zalkapfaan, City of the Headless Horde (`headless`), is next. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L326), [connectivity report](WORLD_CONNECTIVITY.md), [history register](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan](ATTIC_MIGRATION_PLAN.md).

Validation: Checkpoint 128 documentation validation checked 78 changed/added Markdown files and two changed CSV registers: 3,797 local links and 1,389 line anchors passed; CSV widths and targets passed. The 60-column coverage register has 476 data rows and the 10-column attic-candidate register has 121 data rows. `git diff --check` passed. No tests, build, server, gameplay session, database operation, migration or operational script was run.

### Repository-wide audit checkpoint 129 — Zalkapfaan, City of the Headless Horde

Added the [Zalkapfaan repository-wide audit supplement](ZALKAPFAAN_AUDIT_REVIEW.md) beside its source-comprehensive dossier. The active graph has 60 rooms and 132/132 resolving exits, with one 60-room weak component. Raw directed traversal from #2730 reaches 54 and returns from 59/60; six staging rooms sit outside that root traversal and #2789 lacks an authored return. Two outbound and two reverse records form the reciprocal Surface pairs. The all-95 inactive-WLD sweep adds one non-overlapping source-only `surf.wld` approach to #2730; curated totals are 2,008 overall and 152 for Surf. Typed closure records local reset imports #193/#67/#359/#55189; scoped checks find no external typed consumer or reset reference, outside SHP product, local SHP, type-25 destination, or duplicate prototype. The normalized title is unique, and bounded same-stem history shows no predecessor or whole-zone replacement. Keep `headless` **DO NOT MOVE** and Surf / `surface2011` **RETAIN**. No gameplay or placement changed. The queue is 169/220 complete, 51 pending; Priority 170, The Spires of the Elder Eternal Evil (`eternal`), is next. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L120), [connectivity report](WORLD_CONNECTIVITY.md), [history register](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan](ATTIC_MIGRATION_PLAN.md).

Validation: Checkpoint 129 documentation validation checked 80 changed/added Markdown files and two changed CSV registers: 3,867 local links and 1,439 line anchors passed; CSV widths and targets passed. The 60-column coverage register has 476 data rows and the 10-column attic-candidate register has 121 data rows. `git diff --check` passed. No tests, build, server, gameplay session, database operation, migration or operational script was run.

### Repository-wide audit checkpoint 130 — The Spires of the Elder Eternal Evil

Added the [Spires repository-wide audit supplement](SPIRES_OF_ELDER_EVIL_AUDIT_REVIEW.md) beside the source-comprehensive dossier. The active graph has 333 rooms and 708/708 resolving local exits, with no active WLD boundary links. Its 17 weak components contain 275 and 43 rooms plus 15 isolated rooms; raw reach/return from #135400 is 264/274 with blocked edges unavailable and 272/275 when all are assumed available. The all-95 inactive-WLD scan classifies 1,332 `Duris3.wld` target records: 602 non-overlapping source-only approaches and 730 records from overlapping source-room VNUMs. Curated totals are now 2,610 overall and 152 for Surf. Typed closure records four external CHAOS equipment references and no outside QST/reset/story-sidecar consumer, local SHP, external type-25 target, or duplicate prototype. The normalized title is unique. Same-stem history records a follow-up to Yuan-Ti2 without direct game connection, but no whole-zone replacement. Keep active `eternal` **DO NOT MOVE** and `Duris3` **RETAIN IN PLACE** pending map identity review. No gameplay or placement changed. The queue is 170/220 complete, 50 pending; Priority 171, The Shaughin Settlement (`shaughin`), is next. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L90), [connectivity report](WORLD_CONNECTIVITY.md), [history register](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan](ATTIC_MIGRATION_PLAN.md).

Validation: Checkpoint 130 documentation validation checked 82 changed/added Markdown files and two changed CSV registers: 3,937 local links and 1,487 line anchors passed; CSV widths and targets passed. The 60-column coverage register has 476 data rows and the 10-column attic-candidate register has 121 data rows. `git diff --check` passed. No tests, build, server, gameplay session, database operation, migration or operational script was run.

### Repository-wide audit checkpoint 131 — The Shaughin Settlement

Added the [Shaughin repository-wide audit supplement](SHAUGHIN_AUDIT_REVIEW.md) beside the source-comprehensive dossier. The active graph has 49 rooms and 135/135 resolving exits (131 local and 4 outbound); weak components are 45 and 4, and raw reach/return from #6500 is 45/43. The four outbound and inbound records form reciprocal pairs with Underdark and `shaughin2`; the upper WLD has two fall fields and three current fields. The all-95 inactive-WLD scan finds no room overlap or exit target into Shaughin. Curated source-only totals remain 2,610 overall and 152 for Surf. Typed closure records imported lower trophies and reset prototypes, active Surface Keeps reset/shop uses of #6508, two unregistered #6500 shop product references, two sibling type-25 portal targets, two CHAOS ring references, and the compiled #6535 inn binding. The title is unique; bounded history finds no explicit predecessor or whole-zone replacement. Keep active `shaughin` **DO NOT MOVE** and preserve `shaughin2` independently. No gameplay or placement changed. The queue is 171/220 complete, 49 pending; Priority 172, The Trakkia Mountains (`trakkia`), is next. See [coverage row](ZONE_COVERAGE_REGISTER.csv#L270), [connectivity](WORLD_CONNECTIVITY.md), [history](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan](ATTIC_MIGRATION_PLAN.md).

Validation: Checkpoint 131 documentation validation checked 84 changed/added Markdown files and two changed CSV registers: 4,011 local links and 1,540 line anchors passed; CSV widths and targets passed. `git diff --check` passed. The 60-column coverage register has 476 data rows and the 10-column attic-candidate register has 121 data rows. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 132 — The Trakkia Mountains

Added the [Trakkia repository-wide audit supplement](TRAKKIA_AUDIT_REVIEW.md) beside the source-comprehensive dossier. The active graph has 166 rooms and 387/387 resolving exits (385 local and 2 outbound); five weak components contain 109/28/16/12/1 rooms. Root #57000 reaches/returns 72/106 with four blocked exits unavailable and 104/106 when all are assumed available. Two active Surface pairs are reciprocal. There are four fall fields and no currents. The all-95 inactive-WLD scan adds one non-overlapping source-only Surf approach (#246406 west to #57000), bringing curated totals to 2,611 overall and 153 for Surf. Typed closure records two external Q24 rewards, two reset imports, active Alatorin and source-only Brass references, the local four-product shop, and compiled pet-shop binding; bounded negatives found no outside QST item consumer, external SHP product, duplicate prototype, type-25 target, or external typed sidecar VNUM. The normalized title is unique; bounded history finds no explicit predecessor or whole-zone replacement. Keep `trakkia` **DO NOT MOVE** and retain Surf sources. No gameplay or placement changed. The queue is 172/220 complete, 48 pending; Priority 173, The Stone Tomb of Kelek (`kelek`), is next. See [coverage row](ZONE_COVERAGE_REGISTER.csv#L310), [connectivity](WORLD_CONNECTIVITY.md), [history](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan](ATTIC_MIGRATION_PLAN.md).

Validation: Checkpoint 132 documentation validation checked 86 changed/added Markdown files and two changed CSV registers: 4,077 local links and 1,586 line anchors passed; CSV widths and targets passed. `git diff --check` passed. The 60-column coverage register has 476 data rows and the 10-column attic-candidate register has 121 data rows. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 133 — The Stone Tomb of Kelek

Added the [Kelek repository-wide audit supplement](KELEK_AUDIT_REVIEW.md) beside the current dossier. The active graph has 216 rooms and 430/430 resolving exits (428 local, 2 outbound) in components of 215 and 1. #87951 is isolated; root #87950 reaches and returns from 215 rooms. Two reciprocal boundary pairs connect Kelek to Underdark and Krethik. The all-95 inactive-WLD scan finds no overlap or approach, leaving curated totals at 2,611 overall and 153 for Surf. Typed closure records imported epic #359, Deliverer's special at object #87950, and conditional Prime Shift into #88122. Scoped negatives include no outside QST/SHP/reset/portal/prototype/typed-sidecar reference; Alatorin's #88131 sidecar hit is a string ID. The title is unique. Current Church contract ownership remains with Church after the separate 2026 story-sidecar correction; it does not replace the area. Keep `kelek` **DO NOT MOVE**. No gameplay or placement changed. The queue is 173/220 complete, 47 pending; Priority 174, Krethik Keep (`krethik`), is next. See [coverage row](ZONE_COVERAGE_REGISTER.csv#L143), [connectivity](WORLD_CONNECTIVITY.md), [history](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan](ATTIC_MIGRATION_PLAN.md).

Validation: Checkpoint 133 documentation validation checked 88 changed/added Markdown files and two changed CSV registers: 4,138 local links and 1,622 line anchors passed; CSV widths and targets passed. `git diff --check` passed. The 60-column coverage register has 476 data rows and the 10-column attic-candidate register has 121 data rows. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 134 — Krethik Keep

Added the [Krethik repository-wide audit supplement](KRETHIK_AUDIT_REVIEW.md) beside the source-comprehensive dossier. The active graph has 148 rooms and 307/307 resolving exits, with weak components 146/1/1; root #20000 reach/return is 142/146 with two blocked exits unavailable and 145/146 if assumed open. Four active inbounds include three reciprocal pairs with Surface/Kelek plus inbound-only Winterhaven #55633 to #20147. Three fall fields and no currents. The all-95 inactive-WLD sweep adds two oldmap8 and two Surf approaches; totals rise to 2,615 overall and 155 for Surf. Typed closure records active Winterhaven use of local head #20076 and imported memory #55176, four active Alatorin resets of local #20025, imported #72/#359, three local shops, and active combat/spell-pool/epic callbacks. The title is unique; same-stem history shows no whole-zone replacement. Keep `krethik` **DO NOT MOVE** and retain the source-only approaches. No gameplay or placement changed. The queue is 174/220 complete, 46 pending; Priority 175, Thetis's Realm (`thetis`), is next. See [coverage row](ZONE_COVERAGE_REGISTER.csv#L149), [connectivity](WORLD_CONNECTIVITY.md), [history](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan](ATTIC_MIGRATION_PLAN.md).

Validation: Checkpoint 134 documentation validation checked 90 changed/added Markdown files and two changed CSV registers: 4,228 local links and 1,691 line anchors passed; CSV widths and targets passed. `git diff --check` passed. The 60-column coverage register has 476 data rows and the 10-column attic-candidate register has 121 data rows. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 135 — Thetis's Realm

Added the [Thetis repository-wide audit supplement](THETIS_AUDIT_REVIEW.md) beside the existing source-comprehensive dossier. The active graph has 100 rooms and 217/218 loader-resolved/authored exits; #38000 reaches and returns from all 100 rooms. The two resolved Surface/Heavens exits are outbound-only; #38095 north → #104 remains registered as unresolved with inactive dalvik/mavulsk candidates. The all-95 inactive-WLD sweep adds source-only Surf #251543 north → #38000, bringing curated totals to 2,616 overall and 156 for Surf. Typed closure records the Jade Mini Zones resets and contacts for Thetis mobiles #38011/#38026, Thetis's imported map #77209, and optional Stronghold story references to Burgadan/spade #38037. Coverage row 300 is corrected to include local shop #38033. The title is unique and bounded WLD history shows no replacement. Keep thetis DO NOT MOVE and retain Surf/dalvik/mavulsk sources; no gameplay or placement changed.

Validation: Checkpoint 135 documentation validation checked 92 changed/added Markdown files: 4,087 local link targets and 1,718 line anchors passed. The two changed CSV registers are rectangular: 121 data rows by 10 columns and 476 data rows by 60 columns. git diff --check passed. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 136 — The Lair of Tiamat

Added the [Azhural repository-wide audit supplement](AZHURAL_AUDIT_REVIEW.md) beside the source-comprehensive dossier. The 89-room active graph has 216/216 resolved local exits in components of 80/8/1; raw reach/return from #135201 is 80/89. The all-95 inactive-WLD scan finds 180 non-overlapping Duris3 approaches and 172 file-specific overlaps, raising the curated total to 2,796 while Surf remains 156. Tracked commit 50b400005 explicitly makes Azhural the new version of Tiamat's Lair. The separate qst/AREA list still includes tiamats_lair and omits azhural; its 97-line QST file is byte-identical to azhural.qst, so current content is preserved through the old source path. The old WLD/ZON/MOB/OBJ copies remain source-only; the old QST remains selected by the QST source manifest. Keep azhural DO NOT MOVE and tiamats_lair RETAIN pending source ownership/tooling closure. No native source or gameplay changed.

Validation: Checkpoint 136 documentation validation checked 94 changed/added Markdown files: 4,154 local link targets and 1,765 line anchors passed. The two changed CSV registers are rectangular: 121 data rows by 10 columns and 476 data rows by 60 columns. git diff --check passed. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 137 — Quintaragon Castle

Added the [Castle repository-wide audit supplement](CASTLE_AUDIT_REVIEW.md) beside the source-comprehensive dossier. The 130-room active graph has 392/392 resolving exits (391 local and one cross-area), one weak component, and raw root #2401 reach/return of 123/130. The lone boundary reciprocates with Minizones #5760; the all-95 inactive-WLD sweep found no source-room overlap or target into Castle. Curated source-only totals remain 2,796 overall and 156 for Surf. Typed closure records imported Heavens rune stone #359 and Winterhaven memory #55417, plus Gilman #2428’s epic teacher-table handler; no further external QST item command, SHP product, outside prototype, type-25 target, or typed sidecar dependency was found. The normalized title is unique and bounded same-stem histories show no replacement. Keep castle DO NOT MOVE. No area or gameplay source changed.

Validation: Checkpoint 137 documentation validation checked 96 changed/added Markdown files: 4,218 local link targets and 1,806 line anchors passed. The two changed CSV registers are rectangular: 121 data rows by 10 columns and 476 data rows by 60 columns. `git diff --check` passed. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 138 — The Church of the Eternal Dusk

Added the [Church repository-wide audit supplement](CHURCH_AUDIT_REVIEW.md) beside the source-comprehensive dossier. The 33-room active graph has 69/69 resolving exits (68 local and one reciprocal Surface boundary), one weak component, and raw root #87820 reach/return of 33/33. The all-95 inactive-WLD sweep finds no source-room overlap or target into Church; curated totals remain 2,796 overall and 156 for Surf. Bounded typed closure finds one source-only `brass-old-1.zon` reset of active Church object #87859, not a live placement; both the Brass candidate row and unregistered component row now retain that dependency. The normalized title is unique and bounded same-stem history shows no replacement. Keep church DO NOT MOVE and brass-old-1 RETAIN. No area or gameplay source changed.

Validation: Checkpoint 138 documentation validation checked 98 changed/added Markdown files: 4,260 local link targets and 1,829 line anchors passed. The two changed CSV registers are rectangular: 121 data rows by 10 columns and 476 data rows by 60 columns. `git diff --check` passed. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 139 — Domain of Lost Souls

Added the [Domain repository-wide audit supplement](DLSC_AUDIT_REVIEW.md) beside the source-comprehensive dossier. The 100-room active graph has 218/218 resolving exits (217 local and one outbound), one weak component, and raw root #36800 reach/return of 91/100. The Underdark entrance is reciprocal; WH #55632 supplies a one-way inbound exit to #36879. The all-95 inactive-WLD scan found no room overlap and two source-only approaches from `ud_map1` and `ud_map2`, bringing curated totals to 2,798 overall and 156 for Surf. Typed closure documents Domain's imported Heavens, Unique, and Winterhaven objects; local Q47's Winterhaven-owned skull/sheath items; source-only Nexus56 book reset/shop records; and CHAOS, epic-teacher, and object-special code references. The normalized title is unique and bounded same-stem history shows no replacement. Keep dlsc DO NOT MOVE and nexus56 RETAIN. The queue is 179/220 complete, 41 pending; Priority 180, Drifting Realm (`dream`), is next. No area or gameplay source changed.

Validation: Checkpoint 139 documentation validation checked 100 changed/added Markdown files: 4,338 local link targets and 1,889 line anchors passed. The two CSV registers are rectangular: 121 data rows by 10 columns and 476 data rows by 60 columns. `git diff --check` passed. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 140 — Drifting Realm

Added the [Drifting Realm repository-wide audit supplement](DRIFTING_REALM_AUDIT_REVIEW.md) beside the source-comprehensive dossier. The 27-room active graph has 46/46 resolving exits (45 local, one outbound) in weak components of 24 and 3; raw root #31300 reach/return is 3/27. Its only WLD boundary is reciprocal with Ethereal #12442. The all-95 inactive-WLD scan found no room overlap or source-only approach, leaving curated totals at 2,798 overall and 156 for Surf. Non-WLD review identifies the active Arcium bed #85725 targeting Dream #31300; typed scans record four foreign reset sources for local skulls, one Alatorin dreamfoil import, a prose-only Wharf sidecar reference, and 59 CHAOS table source lines. The normalized title is unique and bounded same-stem history shows no replacement. Keep `dream` DO NOT MOVE. The queue is 180/220 complete, 40 pending; Priority 181, The Lizardman Swamps of Clavikord (`lizard`), is next. No area or gameplay source changed.

Validation: Checkpoint 140 documentation validation checked 102 changed/added Markdown files: 4,396 local link targets and 1,929 line anchors all passed. Both CSV registers are rectangular: coverage 476 data rows × 60 columns and attic candidates 121 × 10. `git diff --check` passed. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 141 — The Lizardman Swamps of Clavikord

Added the [Lizardman Swamps repository-wide audit supplement](LIZARDMAN_SWAMPS_AUDIT_REVIEW.md) beside the source-comprehensive dossier. The 120-room active graph has 328/328 resolving exits (326 local, two outbound) in weak components of 101 and 19; raw root #6000 reaches/returns 100/101. Surface and Underdark boundaries reciprocate; Heavens and WH provide one-way inbounds. The all-95 inactive-WLD sweep found no overlap or source-only approach, so curated totals remain 2,798 overall and 156 for Surf. Typed closure records active Randomness sources for local head #6008, source-only `sylvandawn` reset and `tharnadia1`/`tharold` shop records, no external QST item command or typed sidecar item VNUM, and no external type-25 target or duplicate mobile/object prototype. Compiled exact references include smith mobile #6002, Bemon teacher #6013, and arena room #6075. The normalized title is unique; bounded same-stem history shows no established replacement. Keep `lizard` DO NOT MOVE and preserve the three source-only candidates as RETAIN. The queue is 181/220 complete, 39 pending; Priority 182, Tower of High Sorcery (`tower`), is next. No area or gameplay source changed.

Validation: Checkpoint 141 documentation validation checked 104 changed/added Markdown files: 4,451 local link targets and 1,967 line anchors all passed. Both CSV registers are rectangular: coverage 476 data rows × 60 columns and attic candidates 121 × 10. `git diff --check` passed. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 142 — Tower of High Sorcery

Added the [Tower repository-wide audit supplement](TOWER_OF_HIGH_SORCERY_AUDIT_REVIEW.md) beside the source-comprehensive dossier. The active 100-room graph has 219/219 resolving exits (218 local, one outbound), one weak component, and root #9300 reach/return of 72/75 with the two flag-8 edges excluded or 92/99 when available. Surface reciprocates the active boundary. The all-95 inactive-WLD scan found two non-overlapping source-only `surf.wld` approaches into #9334 and #9379; curated totals are now 2,800 overall and 158 for Surf. Typed closure records three Labyrinth inputs, two foreign reset imports, Winterhaven #9375 QST/sidecar use, Raxthan reset/shop #9332, no outside type-25 target or external mobile/object prototype overlap, and direct compiled CHAOS/bulette/guard references. The normalized title is unique across 442 `.zon` files; bounded same-stem history shows no established replacement. Keep `tower` DO NOT MOVE and `surf`/`surface2011` RETAIN. The queue is 182/220 complete, 38 pending; Priority 183, Vecna's Tomb (`vecna`), is next. No area or gameplay source changed.

Validation: Checkpoint 142 documentation validation checked 106 changed/added Markdown files: 4,508 local link targets and 2,008 line anchors all passed. Both CSV registers are rectangular: coverage 476 data rows × 60 columns and attic candidates 121 × 10. `git diff --check` passed. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 143 — Vecna's Tomb

Added the [Vecna repository-wide audit supplement](VECNAS_TOMB_AUDIT_REVIEW.md) beside its source-comprehensive dossier. The 80-room active graph has 187/187 resolving exits (186 local, one outbound) in weak components of 78, 1, and 1; raw traversal from active entry rooms #130000 and #130021 reaches/returns 61/77. Surface reciprocates, while Realm supplies a separate one-way inbound. The all-95 inactive-WLD scan found 162 non-overlapping source-only Duris3.wld records into Vecna, plus 158 raw matches from 80 same-numbered rooms with different titles; curated totals rise to 2,962 overall while Surf remains 158. Typed closure records local Heavens/Negative Material Plane imports and an object #230038 with no current .obj definition, no external typed QST/SHP/sidecar/portal reference, and compiled CHAOS and special-procedure bindings. The normalized title is unique across 442 .zon files; bounded same-stem history shows no established replacement. Keep vecna DO NOT MOVE and Duris3 RETAIN IN PLACE. The queue is 183/220 complete, 37 pending; Priority 184, Killing Fields (killing_fields), is next. No area or gameplay source changed.

Validation: Checkpoint 143 documentation validation checked 108 changed/added Markdown files: 4,565 local link targets and 2,047 line anchors all passed. Both CSV registers are rectangular: coverage 476 data rows × 60 columns and attic candidates 121 × 10. `git diff --check` passed. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 144 — Killing Fields

Added the [Killing Fields repository-wide audit supplement](KILLING_FIELDS_AUDIT_REVIEW.md) beside its source-comprehensive dossier. The 198-room active graph has 737/737 resolving exits (730 local, seven outbound) in one weak component; raw root #33300 reaches and returns to all 198 rooms. Surface and five Bloody Plains boundaries reciprocate, while #33422 east to #33597 has no reverse edge. The all-95 inactive-WLD scan found one non-overlapping source-only Surf exit to #33302, raising curated totals to 2,963 overall and 159 for Surf. The unregistered surf.zon has one mobile and six object reset records for local prototypes. Typed review records the compiled do_shift_prime destination #33366, distinguishes external #33333 experience rewards from VNUMs, and finds no active outside QST item, shop product, sidecar item_vnums, or type-25 target. The normalized title is unique across 442 .zon files; bounded same-stem history shows no established replacement. Keep killing_fields DO NOT MOVE and surf/surface2011 RETAIN. The queue is 184/220 complete, 36 pending; Priority 185, The Para-Elemental Plane of Magma (magma), is next. No area or gameplay source changed.

Validation: Checkpoint 144 documentation validation checked 110 changed/added Markdown files; all 4,614 local link targets and 2,080 line anchors passed. The coverage register is rectangular at 476 data rows × 60 columns and the attic candidate register at 121 × 10. `git diff --check` passed, and `areas/` is unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 145 — The Para-Elemental Plane of Magma

Added the [Magma repository-wide audit supplement](MAGMA_AUDIT_REVIEW.md) beside its source-comprehensive dossier. The active 129-room graph has 612/612 resolving exits, all local, in weak components of 128 and 1; there are no active WLD inbounds or outbounds. The all-95 inactive-WLD scan found 517 incoming records: 260 non-overlapping Duris3 exits and one Vodgh exit, plus 256 Duris3 exits from same-numbered rooms titled The Ocean. The curated source-only total rises by 261 to 3,224; the reviewed Surf subset remains 159. Typed scans found no external exact QST/SHP/ZON/sidecar/type-25 or C/C++ numeric target. The authored magma.qst exists; qst/AREA omits it, but that legacy index is not the production list. The normalized title is unique across 442 .zon files; same-stem history reaches 2012 and 2013 without a distinct predecessor. Keep magma DO NOT MOVE and Duris3/vodgh RETAIN IN PLACE. The queue is 185/220 complete, 35 pending; Priority 186, Mazzolin (mazzolin), is next. No area or gameplay source changed. The CP143 coverage scalar for Vecna is also aligned to its existing 162 non-overlapping Duris3 records (Duris3), which were documented textually but had remained zero in that scalar field.

Validation: Checkpoint 145 documentation validation checked 112 changed/added Markdown files; all 4,651 local link targets and 2,100 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. `git diff --check` passed, and `areas/` is unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 146 — Mazzolin

Added the [Mazzolin repository-wide audit supplement](MAZZOLIN_AUDIT_REVIEW.md) beside its source-comprehensive dossier. The active 136-room map has 288/288 resolving exits (286 local, two outbound) in one weak component; the two UnderDark boundaries reciprocate at #21300. The all-95 inactive-WLD scan found one non-overlapping source-only `ud_map2.wld` approach. The curated total rises by one to 3,225; the reviewed `ud_map2` subset rises to 13 and Surf remains 159. Typed review records a local reset import of Heavens object #358, no external QST/SHP/type-25 or direct C/C++ target, and one raw WH story-ID substring that is not a typed reference. The normalized title is unique across 442 .zon files; bounded same-stem history shows no replacement. Keep mazzolin DO NOT MOVE and ud_map2 RETAIN IN PLACE. The queue is 186/220 complete, 34 pending; Priority 187, Myconid Mushroom Forest (myconid), is next. No area or gameplay source changed.

Validation: Checkpoint 146 documentation validation checked 114 changed/added Markdown files; all 4,683 local link targets and 2,118 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. `git diff --check` passed, and `areas/` is unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 147 — Myconid Mushroom Forest

Added the [Myconid repository-wide audit supplement](MYCONID_AUDIT_REVIEW.md) beside its source-comprehensive dossier. The active 93-room map has 224/224 resolving exits (222 local, two outbound) in one weak component, with two reciprocal UnderDark boundaries. The all-95 inactive-WLD scan found two non-overlapping source-only approaches into #2300: `ud_map0` #1102287 and `ud_map2` #1124242. They are not active ingress. Curated source-only totals rise to 3,227 overall; reviewed `ud_map0` and `ud_map2` subsets rise to two and 14, while Surf remains 159. Typed review records the imported #88502 story item and registered active sources, two compiled room destinations (#2381/#2368), no external typed reset/shop/QST/sidecar item reference, and no external type-25 target. Raw reset-number collisions are kept separate from room references. The normalized title is unique across 442 `.zon` files; same-stem history establishes no replacement. Keep myconid DO NOT MOVE and ud_map0/ud_map2 RETAIN IN PLACE. The queue is 187/220 complete, 33 pending; Priority 188, Vargan II (`v2`), is next. No area or gameplay source changed.

Validation: Checkpoint 147 documentation validation checked 116 changed/added Markdown files; all 4,723 local link targets and 2,145 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. `git diff --check` passed, and `areas/` is unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 148 — Vargan II

Added the [Vargan II repository-wide audit supplement](V2_AUDIT_REVIEW.md) beside its source-comprehensive dossier. The active 119-room map has 249/249 resolving exits (248 local, one outbound) in one weak component, with a reciprocal boundary to Vargan #2182. The all-95 inactive-WLD scan found no source-room overlap or approach. Curated source-only totals remain 3,227 overall and 159 for Surf. Typed scans found no outside QST/SHP/ZON/story item reference, duplicate prototype, or external type-25 room target; compiled-number scanning found no room destination. Three WH story-ID substrings and one floating-point constant are not typed or compiled references. The normalized title is unique across 442 `.zon` files; same-stem history establishes no replacement. Keep v2 DO NOT MOVE. The queue is 188/220 complete, 32 pending; Priority 189, Phantasmagoric Caverns (`valdrak`), is next. No area or gameplay source changed.

Validation: Checkpoint 148 documentation validation checked 118 changed/added Markdown files; all 4,754 local link targets and 2,160 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. `git diff --check` passed, and `areas/` is unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 149 — Phantasmagoric Caverns

Added the [Valdrak repository-wide audit supplement](VALDRAK_AUDIT_REVIEW.md) beside its source-comprehensive dossier. The active 100-room map has 234/234 resolving exits (232 local, two outbound) in one weak component and two reciprocal UnderDark boundaries. The all-95 inactive-WLD sweep adds one non-overlapping, source-only `ud_map2` approach to #53100, bringing curated totals to 3,228 overall and the reviewed `ud_map2` subset to 15; Surf remains 159. Typed closure records Raxthan's active shop/reset references to three local Valdrak objects, nine local Valdrak shop/reset imports from Smokev and Kastle, compiled travel destination #53122, and 29 compiled CHAOS-kit references to local object #53113. No external QST/story item, duplicate prototype, external type-25 target, or story-sidecar reference was found. The normalized title is unique and same-stem history shows no replacement. Keep valdrak DO NOT MOVE and ud_map2 RETAIN IN PLACE. The queue is 189/220 complete, 31 pending; Priority 190, The Bronze Citadel (`bctdl`), is next. No area or gameplay source changed.

Validation: Checkpoint 149 documentation validation checked 120 changed/added Markdown files; all 4,795 local link targets and 2,190 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. `git diff --check` passed, and `areas/` is unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 150 — The Bronze Citadel

Added the [Bctdl repository-wide audit supplement](BCTDL_AUDIT_REVIEW.md) beside its source-comprehensive dossier. The active 77-room map has 160/160 resolving local exits in one weak component, with one-way inbound WLD exits from Realm and Winterhaven; directed root reach is 71/77 with one raw flag-8 edge excluded and 76/77 if available, while all 77 can return. The all-95 inactive-WLD sweep finds no overlap or approach. Curated totals remain 3,228 overall, 15 for `ud_map2`, and 159 for Surf. Typed closure records nine active QST/sidecar uses of Bel's heart #32490, Ceothia's contact #32420, an Avernus type-25 gate to room #32420, local Bctdl references to Avernus reward object #32029 and Heavens/Astral Main/Winterhaven reset imports, and Alatorin's local #32445 reset. Compiled bindings include mobile #32420 and objects #32428/#32430/#32486. One of 871 external type-25 declarations targets Bctdl; no external SHP product or duplicate prototype was found. The title is unique, and the same-stem history plus shared `mabort` annotation establishes no replacement. Keep bctdl DO NOT MOVE. The queue is 190/220 complete, 30 pending; Priority 191, Brimeforge (`brimeforge`), is next. No area or gameplay source changed.

Validation: Checkpoint 150 documentation validation checked 122 changed/added Markdown files; all 4,863 local link targets and 2,247 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. `git diff --check` passed, and `areas/` is unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 151 — Brimeforge

Added the [Brimeforge repository-wide audit supplement](BRIMEFORGE_AUDIT_REVIEW.md) beside its source-comprehensive dossier. The active 49-room map has 98/98 resolving exits (96 local, two outbound) in one weak component, with two reciprocal boundaries to Connector Zones and Mini Zones; authored root traversal reaches and returns from all 49 rooms. The all-95 inactive-WLD sweep finds 196 Duris3 target records: 100 non-overlapping source-only approaches and 96 additional east/west matches from 49 same-numbered Duris3 map rooms. The latter are separate source identities and are excluded from the curated count. Curated totals rise to 3,328 overall; the reviewed `ud_map2` subset remains 15 and Surf remains 159, with no Surf approach into Brimeforge. Typed scans found no exact QST/SHP/STORY/ZON, local outbound prototype, or direct compiled numeric references involving local prototypes; no external duplicate mob/object or type-25 target was found, and 231 external sidecars parsed with zero local numeric-token hits. The normalized title is unique and bounded same-stem history shows no replacement. Keep active `brimeforge` DO NOT MOVE and Duris3 RETAIN IN PLACE. The queue is 191/220 complete, 29 pending; Priority 192, The Bugger Caves (`bugger`), is next. No area or gameplay source changed.

Validation: Checkpoint 151 documentation validation checked 124 changed/added Markdown files; all 4,888 local link targets and 2,265 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. `git diff --check` passed, and `areas/` is unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 152 — The Bugger Caves

Added the [Bugger repository-wide audit supplement](BUGGER_AUDIT_REVIEW.md) beside its source-comprehensive dossier. The active 28-room map has 56/56 resolving exits (54 local, two outbound) in one weak component, with reciprocal boundaries to UnderDark and Ixxillikor; authored root traversal reaches and returns from all 28 rooms. The all-95 inactive-WLD sweep finds no source-room overlap or target into #6400–#6427, so the curated total remains 3,328 overall, 15 reviewed ud_map2 approaches, and 159 Surf approaches. Typed scans found no exact external QST/SHP/STORY/ZON or local outbound prototype reference, external duplicate mobile/object definition, or external type-25 target. The direct compiled-number scan found room #6405 in do_shift_prime; 231 external story sidecars parse with zero local numeric-token hits. The normalized title is unique and bounded same-stem history shows no replacement. Keep bugger DO NOT MOVE. The queue is 192/220 complete, 28 pending; Priority 193, Dirk’nspire Stronghold (dirkn), is next. No area or gameplay source changed.

Validation: Checkpoint 152 documentation validation checked 126 changed/added Markdown files; all 4,916 local link targets and 2,281 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. `git diff --check` passed, and `areas/` is unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 153 — Dirk’nspire Stronghold

The existing [Dirk’nspire audit supplement](DIRKN_AUDIT_REVIEW.md) now includes a CP153 current-source reconciliation. The active 86-room map has 177/177 resolving exits (176 local, one outbound) in one weak component; root #96800 reaches and returns from all rooms, and the Valinhav boundary is reciprocal. The all-95 inactive-WLD sweep finds no room overlap or target, so curated totals remain 3,328 overall, 15 ud_map2 approaches, and 159 Surf approaches. Active Alatorin has two QST consumers of local object #96835 and one rubble reset of #96820; its story sidecar mirrors two #96835 item fields. The direct numeric scan found room #96803 in three compiled code locations, while no other direct external prototype use, duplicate mobile/object, or type-25 target was found. All 231 external sidecars parsed; the ten #96835 hits are in Alatorin’s file. The title is unique and bounded same-stem history shows no replacement. Keep dirkn DO NOT MOVE. The queue is 193/220 complete, 27 pending; Priority 194, Storm Port (`stormport`), is next. No area or gameplay source changed.

Validation: Checkpoint 153 documentation validation checked 128 changed/added Markdown files; all 4,957 local link targets and 2,306 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. `git diff --check` passed, and `areas/` is unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 154 — Storm Port

Added the [Storm Port repository-wide audit supplement](STORMPORT_AUDIT_REVIEW.md) beside its source-comprehensive dossier. The active map has 147 rooms and 319/320 resolving exits (314 local, five resolved outbound, one unresolved Duris3 target), in weak components of 139, 7, and 1; root-directed reach/return is 137/147 and 139/147. Four Surface pairs reciprocate, with one-way inbound records from Storm Port Stronghold and Winterhaven and a one-way Sarmiz outbound. Four non-overlapping source-only Surf exits enter Storm Port. Curated totals are now 3,332 overall and 163 Surf, while ud_map2 remains 15. Typed review records the active foreign Tchan reset, local imports, coded CHAOS/ship/ferry references, and no duplicate prototypes or external type-25 targets. The active title is unique and bounded history shows no replacement. Keep stormport DO NOT MOVE and surf/surface2011 RETAIN. The source-mapping queue is 194/220 complete, with 26 pending; Priority 195, Arachdrathos (arac-web), is next.

Validation: Checkpoint 154 documentation validation checked 130 changed/added Markdown files; all 5,009 local link targets and 2,351 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 155 — Arachdrathos

Added the [Arachdrathos repository-wide audit supplement](ARAC_WEB_AUDIT_REVIEW.md) beside its source-comprehensive dossier. The active map has 190 rooms and 457/457 resolving exits (446 local, 11 outbound) in one component; root-directed traversal reaches 189/190 and returns 190/190. All 11 outbound routes reciprocate with aracguil, llzazan, or arac_wild; two extra one-way Heavens inbounds target #36564, with stale Underdark exit text on one. The all-95 inactive-WLD sweep finds no overlap or approach. Typed review records seven active Alatorin reset references and source-only nexus56/ud_map0 candidate references. Curated source-only totals remain 3,332 overall, 163 Surf, and 15 ud_map2. The title is unique, bounded history shows no replacement, and the unrelated global #51800 room collision remains in Hell Two/Three. Keep arac-web DO NOT MOVE and nexus56/ud_map0 RETAIN. The source-mapping queue is now 195/220 complete, with 25 pending; Priority 196, Arcium (arcium), is next.

Validation: Checkpoint 155 documentation validation checked 132 changed/added Markdown files; all 5,057 local link targets and 2,392 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 156 — Arcium

Added the [Arcium repository-wide audit supplement](ARCIUM_AUDIT_REVIEW.md) beside its source-comprehensive dossier. The active map has 115 rooms and 268/268 resolving exits (266 local, two outbound) in one component; root-directed reach and return are 115/115. Both Surface boundaries reciprocate. The all-95 inactive-WLD sweep finds no overlap and one source-only Surf approach into #85700. Curated totals rise to 3,333 overall and 164 Surf; ud_map2 remains 15. Typed review records the active Alatorin mobile reset, source-only brass-old-1 heart reset, Arcium's Dream import/bed, and two Dream returns; no external duplicate prototypes were found. The title is unique and bounded same-stem history shows no replacement. Keep arcium DO NOT MOVE and brass-old-1/surf RETAIN. The queue is now 196/220 complete, with 24 pending; Priority 197, Future Ceothia (ceofutur), is next.


Validation: Checkpoint 156 documentation validation checked 134 changed/added Markdown files; all 5,096 local link targets and 2,421 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 157 — Future Ceothia

Added the [Future Ceothia repository-wide audit supplement](FUTURE_CEOTHIA_AUDIT_REVIEW.md) beside its comprehensive source dossier. The active map has 278 rooms and 1,016/1,016 resolving local exits, with weak components of 276 and 2; root #81400 reaches 271 rooms and 275 rooms can return. Realm #14210 supplies the only active WLD inbound to #81677; no Ceothia WLD exit leaves the area. The Past portal and two Future portals are separate typed time routes. The all-95 inactive-WLD sweep finds no source-room overlap or target into Future Ceothia, so source-only totals remain 3,333 overall, 164 Surf, and 15 ud_map2.

Typed review records active Alatorin QST/story use of Future object #81403; Present Ceothia QST/story references to #81410/#81423 and contacts #81429/#81454; Surface Mini's contact reference to #81454; the Past type-25 portal target; local reset imports; and compiled CHAOS/random-equipment references. No external duplicate mob/object prototypes, external SHP product, or external active reset-role reference to a Future prototype was found. The shared Ceothia title denotes three active period areas; bounded ceofutur history establishes no alternate-stem rename, predecessor, or whole-zone replacement. Keep ceofutur DO NOT MOVE and preserve all route and typed dependencies. No inactive-WLD candidate row is added. The source-mapping queue is now 197/220 complete, with 23 pending; Priority 198, Treasure Caves (dungeon), is next.

Validation: Checkpoint 157 documentation validation checked 136 changed/added Markdown files; all 5,151 local link targets and 2,466 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 158 — Treasure Caves

Added the [Treasure Caves repository-wide audit supplement](TREASURE_CAVES_AUDIT_REVIEW.md) beside its comprehensive source dossier. The active map has 116 rooms and 236/236 resolving exits (234 local, two outbound) in one component. Raw directed reach and return from #93000 are both 116/116, and both active boundaries reciprocate. The all-95 inactive-WLD sweep finds no source-room overlap or target into Treasure Caves; source-only totals remain 3,333 overall, 164 Surf, and 15 ud_map2.

Typed review records active White Harbor QST/story references and Surface Mini's two completion-key references to Dungeon object #93011; unregistered Mountain.qst's exact reference to object #93001; four active Tharnadian Ruin resets to mobile #93015; and compiled references to room #93001 and object #93007. The normalized title is unique, and bounded same-stem history establishes no alternate-stem rename, predecessor, or whole-zone replacement. Keep active dungeon DO NOT MOVE and retain the existing Mountain candidate with its typed dependency recorded. The source-mapping queue is now 198/220 complete, with 22 pending; Priority 199, Jindon the Deathwood Forest (jin), is next.

Validation: Checkpoint 158 documentation validation checked 138 changed/added Markdown files; all 5,195 local link targets and 2,500 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 159 — Jindon the Deathwood Forest

Added the [Jindon repository-wide audit supplement](JINDON_DEATHWOOD_AUDIT_REVIEW.md) beside its comprehensive source dossier. The active 90-room map has 193/193 resolving exits (192 local, one outbound), in weak components of 65 and 25. Authored directed reach and return from #82000 each cover 65 rooms; the separate component is #82065–#82089. The only active boundary reciprocates with Surface at #619653/#82063. The dossier's lower range bound was corrected from #81678 to #82000. The all-95 inactive-WLD sweep adds one non-overlapping source-only Surf exit, #244020 north to #82063; it is not active ingress. Curated source-only totals are 3,334 overall, 165 for Surf, and 15 for ud_map2.

Typed review records the active Otower reset to Jindon object #82018 and source-only Nexus56 G reset/shop product #82011. Conditional shop-reader fallback does not establish operation. The White Harbor #82067 token is only a string story ID. Compiled mobile #82000 binds to the ticket-master handler that can transfer to #82062 outside the WLD graph. No exact QST item command, external duplicate prototype, or external type-25 target was found. The normalized title is unique and bounded same-stem history establishes no alternate-stem rename, predecessor, or whole-zone replacement. Keep `jin` DO NOT MOVE and `surf`/`nexus56` RETAIN; no candidate row is added. The source-mapping queue is now 199/220 complete, with 21 pending; Priority 200, Lava Springs (`lava`), is next.

Validation: Checkpoint 159 documentation validation checked 140 changed/added Markdown files; all 5,234 local link targets and 2,529 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. `git diff --check` passed, and `areas/` and `src/` are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 160 — Lava Springs

Added the [Lava Springs repository-wide audit supplement](LAVA_SPRINGS_AUDIT_REVIEW.md) beside its comprehensive source dossier. The active 65-room map has 147/147 resolving exits (146 local, one outbound) in one component; authored directed reach and return from #99700 each cover all 65 rooms. Its only active boundary reciprocates with Surface #565027. The all-95 inactive-WLD sweep adds one non-overlapping source-only Surf exit, #259346 north to #99700; it is not active ingress. Curated source-only totals are 3,335 overall, 166 for Surf, and 15 for ud_map2.

Typed review records active Lava resets to Dream object #31318 and Verz1 counter #3097. Compiled references include Chaos equipment objects #99700/#99702, the CHAOS destination and inn room #99715, and smith mobile #99716. No external QST/SHP/sidecar or reset-role reference into Lava, external duplicate prototype, or external type-25 target was found. The normalized title is unique and bounded same-stem history establishes no alternate-stem rename, predecessor, or whole-zone replacement. Keep `lava` DO NOT MOVE and `surf` RETAIN; no candidate row is added. The source-mapping queue is now 200/220 complete, with 20 pending; Priority 201, Lylr-Meop (`lylr`), is next.

Validation: Checkpoint 160 documentation validation checked 142 changed/added Markdown files; all 5,278 local link targets and 2,563 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. `git diff --check` passed, and `areas/` and `src/` are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 161 — Lylr-Meop

Added the [Lylr-Meop repository-wide audit supplement](LYLR_MEOP_AUDIT_REVIEW.md) beside its comprehensive source dossier. The active 64-room map has 134/134 resolving exits (132 local, two outbound), in weak components of 40 and 24. Authored directed reach and return from #82200 each cover 40 rooms; the separate component is #82240–#82263. Both active exits lead one way to Surface, with no reverse active WLD records. The all-95 inactive-WLD sweep adds two non-overlapping source-only Surf exits, #222832 north to #82240 and #224629 south to #82200. They are not active ingress. Curated source-only totals are 3,337 overall, 168 for Surf, and 15 for ud_map2.

Typed review records active Lylr resets to Tharnadia object #132581 and Heavens objects #363/#368/#369; active Lylr shop #82204 also stocks the three Heavens items. Compiled references bind two world-quest mobiles, one money changer, and the inn room. No external QST/SHP/story or reset-role reference into Lylr, external duplicate prototype, or external type-25 target was found. The normalized title is unique and bounded same-stem history establishes no alternate-stem rename, predecessor, or whole-zone replacement. Keep `lylr` DO NOT MOVE and `surf` RETAIN; no candidate row is added. The source-mapping queue is now 201/220 complete, with 19 pending; Priority 202, The Halfling Silver Mine (`mining`), is next.

Validation: Checkpoint 161 documentation validation checked 144 changed/added Markdown files; all 5,319 local link targets and 2,594 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. `git diff --check` passed, and `areas/` and `src/` are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 162 — The Halfling Silver Mine

Added the [Mining repository-wide audit supplement](MINING_AUDIT_REVIEW.md) beside its comprehensive source dossier. The active 69-room map has 153/153 resolving exits (152 local, one outbound), in one weak component. Authored directed reach from #4900 covers 64 rooms and return paths cover 68; #4964–#4968 are outside root-forward reach and #4968 cannot return. The sole active WLD boundary reciprocates with Surface at #4900/#566915.

The all-95 inactive-WLD sweep finds 160 target matches into Mining and 69 same-number source-room records in unregistered Alterian Mountains/Wilderness. Nine non-overlapping source-only exits are curated (three, five, and one from Mountain, Wilderness, and Surf); the other 151 are excluded because their source rooms overlap Mining VNUMs. Totals are 3,346 overall, 169 for Surf, and 15 for ud_map2. Typed review records 16 and 17 source-only Alterian M-reset room destinations, the distinct source-only Alterian Mountains object #4900, and string-ID collisions in Alatorin and Shady. Compiled numeric hits are a float-table substring and a code comment, not direct references. The local `mining.qst` is not listed in `areas/qst/AREA`, and Mining has no local SHP. The normalized title is unique; bounded history beginning at the 2015 same-stem import establishes no alternate-stem rename, predecessor, or whole-zone replacement. Keep `mining` DO NOT MOVE; retain the existing Alterian and Surf candidates without new rows. The source-mapping queue is now 202/220 complete, with 18 pending; Priority 203, Forest of Mir (`mir`), is next.

Validation: Checkpoint 162 documentation validation checked 146 changed/added Markdown files; all 5,367 local link targets and 2,632 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. `git diff --check` passed, and `areas/` and `src/` are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 163 — Forest of Mir

Added the [Forest of Mir repository-wide audit supplement](MIR_AUDIT_REVIEW.md) beside its comprehensive source dossier. The active 281-room map has 558/558 resolving exits (556 local, two outbound) in weak components of 271 and 10. Raw authored reach from #41900 is 237/281 and return coverage is 271/281. Both active boundaries reciprocate with Highway. The all-95 inactive-WLD sweep finds no room overlap or exit target into Mir; curated source-only totals remain 3,346 overall, 169 for Surf, and 15 for ud_map2.

Typed review records active Bastine QST/story item references to Mir objects #41924, #41922, and #41920, an active Surface Mini QST/story reference to #41916, Highway reset references to #41915 and #41917, and Mir's active reset to Heavens object #359. Compiled references bind Mir procedure mobiles and selected object behaviors, and include Chaos equipment object #42169. No external duplicate prototype or external type-25 target was found. The normalized title is unique among 442 scanned .zon files; bounded same-stem history establishes no alternate-stem rename, predecessor, or whole-zone replacement. Keep mir DO NOT MOVE and add no candidate row. The queue is now 203/220 complete, with 17 pending; Priority 204, The Shadow Forest (mist), is next.


Validation: Checkpoint 163 documentation validation checked 148 changed/added Markdown files; all 5,420 local link targets and 2,675 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. git diff --check passed, and areas/ and src/ are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 164 — The Shadow Forest

Added the [Shadow Forest repository-wide audit supplement](SHADOW_FOREST_AUDIT_REVIEW.md) beside its comprehensive source dossier. The active 73-room map has 199/199 resolving exits (197 local, two outbound) in one weak component. Raw authored reach from #6300 is 67/73, with 72/73 return paths. Four active inbounds come from Minizones, NewFor, and Surface; the two Surface paths reciprocate. The all-95 inactive-WLD sweep adds two non-overlapping Alterian Wilderness exits and one Aopal exit to #6368; curated source-only totals are now 3,349 overall, 169 for Surf, and 15 for ud_map2.

Typed scans find no external QST item, SHP product, structured story item, or reset-role reference to local Mist prototypes. The tharnadia1.shp product number #6358 is a room/object namespace collision, not a typed Mist object dependency. Compiled code binds mobile #6307 to mist_protect and checks room #6300; other direct numeric hits are floating-point values or a hash constant. No external duplicate prototype or type-25 target was found. The normalized title is unique among 442 scanned .zon files; bounded same-stem history establishes no alternate-stem rename, predecessor, or whole-zone replacement. Keep mist DO NOT MOVE; update the existing Alterian Wilderness and Aopal candidate rows, add no new row, and preserve the tharnadia1 false-positive classification. The queue is now 204/220 complete, with 16 pending; Priority 205, Mosswood (moss), is next.


Validation: Checkpoint 164 documentation validation checked 150 changed/added Markdown files; all 5,468 local link targets and 2,714 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. git diff --check passed, and areas/ and src/ are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 165 — Mosswood

Added the [Mosswood repository-wide audit supplement](MOSSWOOD_AUDIT_REVIEW.md) beside its comprehensive source dossier. The active 161-room map has 399/399 resolving exits (398 local, one outbound) in one weak component. Raw authored reach from #23400 is 157/161, and all 161 rooms have return paths. Its sole active boundary reciprocates with Surface. The all-95 inactive-WLD sweep adds three non-overlapping Surf approaches, bringing curated source-only totals to 3,352 overall, 172 for Surf, and 15 for ud_map2.

Typed scans find no external QST item, SHP product, structured story item_vnums, ZON reset-role, or direct compiled numeric reference into Moss prototypes. Local QST/story/SHP/ZON fields stay within local prototypes. No external duplicate prototype or type-25 target was found. The normalized title is unique among 442 scanned .zon files; bounded same-stem history establishes no alternate-stem rename, predecessor, or whole-zone replacement. Keep moss DO NOT MOVE and update existing Surf candidate row 97; no new candidate row is added. The queue is now 205/220 complete, with 15 pending; Priority 206, The Reliquary Nexus of the Roper Den (nexus_roper), is next.


Validation: Checkpoint 165 documentation validation checked 152 changed/added Markdown files; all 5,496 local link targets and 2,734 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. git diff --check passed, and areas/ and src/ are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Repository-wide audit checkpoint 166 — The Reliquary Nexus of the Roper Den

Added the [Roper Den repository-wide audit supplement](NEXUS_ROPER_AUDIT_REVIEW.md) beside its comprehensive source dossier. The active 25-room map has 51/51 resolving exits (50 local, one outbound) in one weak component; authored reach and return from #130400 cover all 25 rooms. Its sole active boundary reciprocates with Underdark. The all-95 inactive-WLD sweep finds 100 Duris3 target records: 52 non-overlapping source-only exits and 48 matches from 25 same-numbered source rooms, which are excluded. Curated source-only totals rise to 3,404 overall; Surf remains 172 and ud_map2 15.

Typed scans find no external QST item, SHP product, structured story item_vnums, reset-role, or direct compiled numeric reference into Roper Den prototypes. No external duplicate prototype or type-25 target was found. The normalized title is unique among 442 scanned .zon files; bounded same-stem history establishes no alternate-stem rename, predecessor, or whole-zone replacement. Keep nexus_roper DO NOT MOVE and update the existing Duris3 candidate row 36; add no row. The queue is now 206/220 complete, with 14 pending; Priority 207, Ny’Neth’s Stronghold (nyneth2), is next.


Validation: Checkpoint 166 documentation validation checked 154 changed/added Markdown files; all 5,529 local link targets and 2,759 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. git diff --check passed, and areas/ and src/ are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Checkpoint 167: Ny’Neth’s Stronghold dependencies and no inactive-map approach

The active Nyneth2 map has 101 rooms and 210/210 resolving exits, all local, in seven weak components. Raw directed traversal from #23000 reaches 76 rooms and 77 have return paths. Its only active boundary is the one-way White Harbor #55632 north exit to #23098. The all-95 inactive-WLD sweep finds no room overlap or target exit, so curated totals remain 3,404 overall, Surf 172, and ud_map2 15.

Preserve the active Nyneth type-25 portal #22903 target to #23000; Nyneth2 resets of Nyneth mobile #22904 and object #22966 and Heavens object #360; Alatorin’s two G resets of Nyneth2 object #23045; and compiled references to local objects #23056 and #23064. No external QST item command, SHP product, structured story item reference, or duplicate mobile/object prototype was found. The normalized title is unique; `nyneth3` is a distinct continued title, and bounded history establishes no replacement. Keep active `nyneth2` DO NOT MOVE; no candidate row is added. See the [Ny’Neth’s Stronghold audit supplement](NYNETH2_AUDIT_REVIEW.md), coverage rows [Nyneth2](ZONE_COVERAGE_REGISTER.csv#L218), [Nyneth](ZONE_COVERAGE_REGISTER.csv#L217), [Alatorin](ZONE_COVERAGE_REGISTER.csv#L6), [Heavens](ZONE_COVERAGE_REGISTER.csv#L121), and [White Harbor](ZONE_COVERAGE_REGISTER.csv#L345).

Validation: Checkpoint 167 documentation validation checked 156 changed/added Markdown files; all 5,574 local link targets and 2,796 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. git diff --check passed, and areas/ and src/ are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Checkpoint 168: Ny’Neth’s Stronghold Continued dependencies and no inactive-map approach

The active Nyneth3 map has 118 rooms and 232/232 resolving exits, all local, in three weak components. Raw directed traversal from #38700 reaches 11 rooms and only two rooms have return paths. Two active inbounds from Realm #14208 southwest and White Harbor #55633 up enter Treasure Room #38811; neither has a reciprocal Nyneth3 exit. The all-95 inactive-WLD sweep finds no room overlap or target exit, so curated totals remain 3,404 overall, Surf 172, and ud_map2 15.

Preserve the active Nyneth portal #22904 target to #38700; Alatorin QST/story/ZON references to Nyneth3 object #38779; Barovia reset rows for object #38715; Nyneth3 resets of Heavens #17/#360/#903, White Harbor #55208, Negative Plane #26665, and Nyneth mobile #22904; plus compiled boss, Fury, Chaos-kit, and item-procedure bindings. No SHP product, external duplicate prototype, or further external story-sidecar VNUM match was found. The normalized title is unique; `nyneth2` is a separate title, and bounded history establishes no replacement. Keep active `nyneth3` DO NOT MOVE; no candidate row is added. See the [Ny’Neth’s Stronghold Continued audit supplement](NYNETH3_AUDIT_REVIEW.md), coverage rows [Nyneth3](ZONE_COVERAGE_REGISTER.csv#L219), [Nyneth](ZONE_COVERAGE_REGISTER.csv#L217), [Realm](ZONE_COVERAGE_REGISTER.csv#L251), [White Harbor](ZONE_COVERAGE_REGISTER.csv#L345), [Alatorin](ZONE_COVERAGE_REGISTER.csv#L6), [Barovia](ZONE_COVERAGE_REGISTER.csv#L25), [Heavens](ZONE_COVERAGE_REGISTER.csv#L121), and [Negative Plane](ZONE_COVERAGE_REGISTER.csv#L200).

Validation: Checkpoint 168 documentation validation checked 158 changed/added Markdown files; all 5,637 local link targets and 2,851 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. git diff --check passed, and areas/ and src/ are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Checkpoint 169 — Orrak

Added the [Orrak repository-wide audit supplement](ORRAK_AUDIT_REVIEW.md) and linked it from the comprehensive source dossier. The active 39-room map has 77/77 resolving exits in one component, full directed reach and return from #9000, and a reciprocal Surface boundary. The inactive-WLD sweep adds one non-overlapping source-only Surf approach to #9000; curated totals are 3,405 overall, 173 for Surf, and 15 for ud_map2. Typed dependencies record Orrak imports of Heavens #358 and White Harbor #55446, and Charing/Sylvandawn resets of Orrak object #9017. No external QST/SHP/structured-story prototype reference, type-25 target, duplicate prototype, or direct compiled Orrak VNUM reference was found; numeric false positives are described in the supplement. The unique title and bounded history show no replacement. Keep `orrak` DO NOT MOVE; preserve Surf row 97 and Sylvandawn row 101 as RETAIN candidates, with no new row.

Validation: Checkpoint 169 documentation validation checked 160 changed/added Markdown files; all 5,684 local link targets and 2,891 line anchors passed. Coverage is 476 data rows x 60 columns; the candidate register is 121 x 10. `git diff --check` passed, and `areas/` and `src/` are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Checkpoint 170 — Sea Kingdom

Added the [Sea Kingdom repository-wide audit supplement](SEAKNGDM_AUDIT_REVIEW.md) and linked it from the comprehensive source dossier. The 225-room active map has 558/558 resolving exits across five weak components, raw root reach/return of 113/225 and 23/225, a reciprocal Plane Water boundary, and one technical White Harbor inbound. The all-95 inactive-WLD sweep finds no overlap or approach; source-only totals stay 3,405 overall, 173 for Surf, and 15 for ud_map2. Typed review records Alatorin references to Sea Kingdom objects #31538/#31544, a Heavens reference to #31549, and Sea Kingdom's import of Heavens #359. Compiled bindings include Tsunami/necklace behavior, Chaos gear objects, and vault placement. No external SHP product, reset-role target, type-25 target, duplicate prototype, or direct compiled mobile use was found. The unique title and bounded history show no replacement. Keep `seakngdm` DO NOT MOVE; add no candidate row.

Validation: Checkpoint 170 documentation validation checked 162 changed/added Markdown files; all 5,752 local link targets and 2,949 line anchors passed. Coverage is 476 data rows x 60 columns; the candidate register is 121 x 10. `git diff --check` passed, and `areas/` and `src/` are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Checkpoint 171 — Turolopolis Zoo

Added the [Turolopolis Zoo repository-wide audit supplement](TUROLZOO_AUDIT_REVIEW.md) and linked it from the comprehensive source dossier. The active 62-room map has 141/141 resolving exits in one weak component and a reciprocal Willem boundary. With the closed/secret/blocked gorilla-cage exit unavailable, root reach/return is 60/62 and 62/62; treating authored exits as open yields 62/62 each. The all-95 inactive-WLD sweep finds no overlap or approach, so source-only totals remain 3,405 overall, 173 for Surf, and 15 for ud_map2. No typed external item/reset/shop/story reference, duplicate prototype, external type-25 target or direct compiled Zoo VNUM binding was found. Tower currency and Piergeiron movement-string matches are false positives. The unique title and bounded history show no replacement. Keep `turolzoo` DO NOT MOVE; add no candidate row.

Validation: Checkpoint 171 documentation validation checked 164 changed/added Markdown files; all 5,795 local link targets and 2,979 line anchors passed. Coverage is 476 data rows x 60 columns; the candidate register is 121 x 10. `git diff --check` passed, and `areas/` and `src/` are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Checkpoint 172 — Valley of Crushk

Added the [Valley of Crushk repository-wide audit supplement](VALLEY_CRUSHK_AUDIT_REVIEW.md) and linked it from the comprehensive source dossier. The active 85-room map has 182/182 resolving exits in one weak component; raw root reach/return is 85/85. Mini Zones has a one-way inbound to #2001, Pharr Valley has a reciprocal boundary at #2006, and Valley #2060 exits one-way to Surface #548133. The all-95 inactive-WLD sweep found no overlap and one non-overlapping source-only Surf approach, #232517 north to #2060, raising curated totals to 3,406 overall, 174 Surf, and 15 ud_map2. Typed closure records the Ship Yards shiv #43143 requirement, cross-area reset/shop references, Heavens #399/#606 imports, and unresolved local M#207 reset. No external QST/story item target, duplicate prototype, or type-25 target into Valley was found. The unique title and bounded history show no replacement. Keep `valley_crushk` DO NOT MOVE and retain Surf candidate row 97; add no candidate row.

Validation: Checkpoint 172 documentation validation checked 166 changed/added Markdown files; all 5,864 local link targets and 3,032 line anchors passed. Coverage is 476 data rows x 60 columns; the candidate register is 121 x 10. `git diff --check` passed, and `areas/` and `src/` are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Checkpoint 173 — The Forgotten Forest

Added the [Forgotten Forest repository-wide audit supplement](FORGOTTEN_FOREST_AUDIT_REVIEW.md) and linked it from the comprehensive source dossier. The active 92-room map has 283/283 resolving exits in one weak component, raw root reach of 90/92 and return of 91/92, and one reciprocal Surface boundary. The all-95 inactive-WLD sweep found no overlap and 15 non-overlapping source-only Surf approaches, raising curated totals to 3,421 overall, 189 Surf, and 15 ud_map2. Bounded typed scans found no external QST/story item, SHP product, reset-role, duplicate-prototype, or type-25 target into the Forest; the compiled Chaos druid equipment tables do reference local object #82707. The unique title and bounded same-stem history show no replacement. Keep `forgotten_forest` DO NOT MOVE; preserve the source-only records with Surf candidate row 97 and add no candidate row.

Validation: Checkpoint 173 documentation validation checked 168 changed/added Markdown files; all 5,921 local link targets and 3,073 line anchors passed. Coverage is 476 data rows x 60 columns; the candidate register is 121 x 10. `git diff --check` passed, and `areas/` and `src/` are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Checkpoint 174 — The Rice Fields

Added the [Rice Fields repository-wide audit supplement](RICE_FIELDS_AUDIT_REVIEW.md) and linked it from the comprehensive source dossier. The 63-room active map has 153 authored exits, 152 resolving targets, two weak components (58 and 5 rooms), and raw root reach/return of 23/63 and 58/63. Three reciprocal active Surface boundaries remain. The all-95 inactive-WLD sweep found no overlap and four source-only approaches (three Surf, one IC3), raising curated totals to 3,425 overall, 192 Surf, and 15 ud_map2. Typed review records local and foreign QST/story exchanges, Thetis mobile imports, and compiled archer/map bindings. No external shop, reset-role, duplicate-prototype, or type-25 target into the area was found. The unique title and bounded history show no replacement. Keep `jademini` DO NOT MOVE and retain IC3 row 53 and Surf row 97; add no candidate row.

Validation: Checkpoint 174 documentation validation checked 170 changed/added Markdown files; all 5,997 local link targets and 3,132 line anchors passed. Coverage is 476 data rows x 60 columns; the candidate register is 121 x 10. `git diff --check` passed, and `areas/` and `src/` are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Checkpoint 175 — The Bandit Canyons

Added the [Bandit Canyons repository-wide audit supplement](BANDIT_CANYONS_AUDIT_REVIEW.md) and linked it from the comprehensive source dossier. The 76-room active map has 163/163 resolving exits in one weak component; raw root reach/return is 75/76 and 76/76, with one reciprocal Surface boundary. The all-95 inactive-WLD sweep found no overlap and one source-only Surf approach, raising curated totals to 3,426 overall, 193 Surf, and 15 ud_map2. Typed review records local uses of BCTDL #32490, Negative Plane #26614, and Heavens #402/#403; Alatorin references local dagger #98505; compiled code maps object #98534 to Olat’s Epic Strength skill. No external shop, reset-role, duplicate-prototype, or type-25 target into the area was found. The unique title and bounded history show no replacement. Keep `bandit` DO NOT MOVE, retain Surf row 97, and add no candidate row.

Validation: Checkpoint 175 documentation validation checked 172 changed/added Markdown files; all 6,048 local link targets and 3,169 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. `git diff --check` passed, and `areas/` and `src/` are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Checkpoint 176 — Darkfall Forest

Darkfall already had a source-comprehensive story dossier and Checkpoint 24 connectivity, dependency, and title/history supplement, so this checkpoint reused those documents and added no duplicate review. The fresh active-WLD parse found 49 rooms and 97/97 resolving authored exits (94 internal, three cross-area), weak components of 48 and one, and raw root reach/return of 48/49. The all-95 inactive-WLD parse found no errors or room overlap and reconfirmed two source-only `surf.wld` inbounds; no new approaches were added and curated totals remain 3,426 overall, 193 Surf, and 15 ud_map2. [Coverage row 70](ZONE_COVERAGE_REGISTER.csv#L70) now records the two inbounds instead of zero and distinguishes authored reach from runtime safety. The queue is 216/220 complete with four pending; Priority 217, Barrow of the Quiosho (`elftomb`), is next.

Validation: Checkpoint 176 documentation validation checked 173 changed/added Markdown files; all 6,076 local link targets and 3,193 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. `git diff --check` passed, and `areas/` and `src/` are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Checkpoint 177 — Barrow of the Quiosho

Added the [Barrow repository-wide audit supplement](ELFTOMB_AUDIT_REVIEW.md) beside its existing comprehensive dossier. The active 207-room map has 667/667 resolving exits in weak components of 206 and 1; raw root reach/return is 206/207. Three active Surface pairs reciprocate. The all-95 inactive-WLD sweep adds four non-overlapping source-only Surf approaches to #20600, #20611, and #20615. Curated totals are 3,430 overall, 197 Surf, and 15 ud_map2. Typed closure records an active Caves QST/story input of local object #20604 and unregistered `bgtk.shp` products listing five local Barrow objects. The unique normalized title and bounded history show no replacement. Updated coverage rows 47, 87, and 367 and retained Surf candidate row 97 plus BGTK row 16. The queue is 217/220 complete with three pending; Priority 218, Keep of Evil (`evkeep`), is next.

Validation: Checkpoint 177 documentation validation checked 175 changed/added Markdown files; all 6,134 local link targets and 3,238 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. `git diff --check` passed, and `areas/` and `src/` are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Checkpoint 178 — Keep of Evil

Added the [Keep of Evil repository-wide audit supplement](EVKEEP_AUDIT_REVIEW.md) beside the existing comprehensive dossier. The active 147-room map has 401/401 resolving exits in components of 91 and 56; root reach/return is 91/147. The Surface pair reciprocates and White Harbor has a separate one-way inbound. The all-95 inactive-WLD scan adds one non-overlapping source-only Surf approach. Curated totals are 3,431 overall, 198 Surf, and 15 ud_map2. Typed closure records local Heavens/Winterhaven reset imports, a source-only Brass Old 1 reset of Keep object #44876, and compiled table references to local objects #44824/#44827/#44877. The normalized title and bounded history show no replacement. Updated Keep, Surface, White Harbor, Heavens, and Brass Old 1 coverage rows and retained Surf candidate row 97 plus Brass Old 1 row 17. The queue is 218/220 complete with two pending; Priority 219, New Cave city (`new_cavecity`), is next.

Validation: Checkpoint 178 documentation validation checked 177 changed/added Markdown files; all 6,193 local link targets and 3,284 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. `git diff --check` passed, and `areas/` and `src/` are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Checkpoint 179 — New Cave city

Added the repository-wide New Cave city audit supplement beside its comprehensive source dossier. The active 73-room map has 155/155 resolving exits, with all 73 rooms reachable from and returning to root #15100. Its three Surface/Underdark boundaries reciprocate. One inactive Surf approach adds to the curated totals of 3,432 overall, 199 Surf, and 15 ud_map2. Static typed closure records the three external recipe inputs, Heavens reward, three external local-reset imports, Twin Towers Forest wizard/reset ownership, and exact compiled prototype uses; external duplicate, sidecar, shop, reset-role, and portal scans found no other inbound prototype references. Unique title and bounded WLD/ZON history establish no replacement. Updated the New Cave city, Twin Towers Forest, Heavens, Bctdl, Negative Plane, White Harbor, and Unique coverage rows; retained Surf candidate row 97. The queue is 219/220 complete with one pending; Priority 220, Lost City of Tikitzopl (tikit), is next.

Validation: Checkpoint 179 documentation validation checked 179 changed/added Markdown files; all 6,741 local link targets and 3,342 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. GitHub-style heading anchors also passed. git diff --check passed, and areas/ and src/ are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Checkpoint 180 — Lost City of Tikitzopl

Added the repository-wide Lost City audit supplement beside its comprehensive source dossier. The active 340-room map has 855/855 resolving exits and full raw root reach/return. Its reciprocal Surface and Temple boundaries are separate from the Temple-owned type-25 return. One source-only Surf approach brings the curated totals to 3,433 overall, 200 Surf, and 15 ud_map2. Static typed closure records 15 City item VNUMs used by six Temple QST recipes and mirrored in its sidecar, the Surface Keeps reset of City object #43708, and the Temple portal target. Updated Lost City, Lost Temple, and Surface Keeps coverage rows; retained Surf candidate row 97. The ordered original-priority queue is complete at 220/220 with none pending.

Validation: Checkpoint 180 documentation validation checked 181 changed/added Markdown files; all 6,791 local link targets and 3,372 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. GitHub-style heading anchors also passed. git diff --check passed, and areas/ and src/ are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.

### Checkpoint 181 — Ako Village

Added a new source-comprehensive [Ako Village dossier](../zone-stories/AKO_VILLAGE.md) and [repository-wide audit supplement](AKO_AUDIT_REVIEW.md). The dossier maps the country-to-village setting, poverty and market life, the witchcraft trial and executions, the elders' reset-loaded objects, the absence of a local QST/story completion, and builder questions with recommended resolutions.

The 52-room active WLD has 107/107 resolving exits in one weak component. Authored local traversal from #3700 reaches and returns from all rooms; the only active boundary is a one-way departure from #3751 to Surface #544136. One non-overlapping inactive Surf source room, #224164, points east into #3751 but is not active ingress. Curated source-only totals are 3,434 overall, 201 for Surf, and 15 for `ud_map2`.

Exact typed closure found three Ako guard resets of Heavens sword #283 and no external quest/shop/story/reset/prototype/portal reference into Ako. The normalized title is unique among 442 scanned `.zon` titles, and bounded same-stem history shows no replacement. Ako remains DO NOT MOVE; retain the approach in Surf candidate row 97 and add no candidate row. The Heavens coverage entry now records the incoming reset consumer.

The original priority list is complete at 220/220, but the broader goal remains open. Current coverage still has 130 active rows without a mapped source dossier; status text still marks 230 narrative, 389 dynamic connectivity, 225 duplicate/history, and 393 typed-dependency rows incomplete. Continue with `ancientb`, the first remaining active contributor without a mapped dossier in register order.

Validation: Checkpoint 181 link validation checked seven checkpoint Markdown files plus Markdown links embedded in both registers: 2,751 destinations, with no missing local targets or line/heading anchors. Coverage is 476 data rows x 60 columns and the attic candidate register is 121 x 10. `git diff --check` passed; `areas/` and `src/` are unchanged. No tests, build, runtime session, database operation, migration, or operational script was run.

### Checkpoint 182 update — An Ancient Bridge

Added the [An Ancient Bridge source dossier](../zone-stories/ANCIENT_BRIDGE.md) and [repository-wide audit supplement](ANCIENTB_AUDIT_REVIEW.md), including the under-bridge kobold/troll setting, one-way Surface departures, three inactive source-only approaches, exact typed reset dependencies, and bounded title/history findings. The original 220-priority queue remains complete; the broader audit continues. There are now 222 comprehensive source dossiers mapped to active area keys, while 129 active rows still lack a mapped dossier. Coverage status text identifies pending scope on 229 narrative, 389 dynamic-connectivity, 224 duplicate/history, and 392 typed-dependency rows. Continue with `ants`, the next active contributor without a mapped dossier in register order.

Validation: Checkpoint 182 link validation checked seven checkpoint Markdown files plus Markdown links embedded in both registers: 2,783 destinations, with no missing local targets or line/heading anchors (1,149 anchors checked). Coverage remains 476 data rows x 60 columns and the attic candidate register 121 x 10. `git diff --check` passed; `areas/` and `src/` are unchanged. No tests, build, runtime session, database operation, migration, or operational script was run.

### Checkpoint 183 update — Ant Forest

Added the [Ant Forest source dossier](../zone-stories/ANT_FOREST.md) and [repository-wide audit supplement](ANTS_AUDIT_REVIEW.md), including its two native switch puzzles, active reciprocal Surface route, source-only Surf approach, unique title, and bounded same-stem history. The original 220-priority queue remains complete; the broader audit continues. There are now 223 comprehensive source dossiers mapped to active area keys, while 128 active rows still lack a mapped dossier. Coverage status text identifies pending scope on 228 narrative, 389 dynamic-connectivity, 223 duplicate/history, and 391 typed-dependency rows. Continue with `arac_wild`, the next active contributor without a mapped dossier in register order.

Validation: Checkpoint 183 link validation checked nine checkpoint Markdown files plus Markdown links embedded in both registers: 3,869 destinations and 1,198 line/heading anchors, with no missing local targets or anchors. Coverage remains 476 data rows x 60 columns and the attic candidate register 121 x 10. `git diff --check` passed; `areas/` and `src/` are unchanged. No tests, build, runtime session, database operation, migration, or operational script was run.

### Checkpoint 184 update — Arachdrathos Wilderness

Added the [Arachdrathos Wilderness source dossier](../zone-stories/ARACHDRATHOS_WILDERNESS.md) and [repository-wide audit supplement](ARAC_WILD_AUDIT_REVIEW.md), including the Half-Drow enclave and its stated city-reclamation goal, the shadow-dragon route, the Crescent Chasm, the map item, five reciprocal active boundaries, one source-only Surf approach, and the exact reset/compiled dependencies. The original 220-priority queue remains complete; the broader audit continues. There are now 224 comprehensive source dossiers mapped to active area keys, while 127 active rows still lack a mapped dossier. Coverage status text identifies pending scope on 227 narrative, 388 dynamic-connectivity, 222 duplicate/history, and 390 typed-dependency rows. Continue with `aracguil`, the next active contributor without a mapped dossier in register order.

Validation: Checkpoint 184 link validation checked eleven checkpoint Markdown files plus Markdown links embedded in both registers: 3,961 destinations and 1,270 line/heading anchors, with no missing local targets or anchors. Coverage remains 476 data rows x 60 columns and the attic candidate register 121 x 10. `git diff --check` passed; `areas/` and `src/` are unchanged. No tests, build, runtime session, database operation, migration, or operational script was run for this documentation-only checkpoint.

### Checkpoint 185 update — Arachdrathos Guilds

Added the [Arachdrathos Guilds source dossier](../zone-stories/ARACHDRATHOS_GUILDS.md) and [repository-wide audit supplement](ARACGUIL_AUDIT_REVIEW.md), covering the six houses, ten-switch passage network, seven reciprocal Arac-Web entrances, lost-shrine Jewel, imported Heavens mana stone, and compiled callbacks. The original 220-priority queue remains complete; the broader audit continues. There are now 225 comprehensive source dossiers mapped to active area keys, while 126 active rows still lack a mapped dossier. Coverage status text identifies pending scope on 226 narrative, 387 dynamic-connectivity, 221 duplicate/history, and 389 typed-dependency rows. Continue with `ardgral`, the next active contributor without a mapped dossier in register order. See coverage row [11](ZONE_COVERAGE_REGISTER.csv#L11) and the [Arachdrathos Guilds audit supplement](ARACGUIL_AUDIT_REVIEW.md).

Validation: Checkpoint 185 documentation link validation checked thirteen checkpoint Markdown files plus Markdown links embedded in both registers: 4,082 local destinations and 1,375 line/heading anchors, with no missing targets or anchors. Coverage remains 476 data rows x 60 columns and the attic candidate register 121 x 10. `git diff --check` passed, and `areas/` and `src/` are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run for this documentation-only checkpoint.

### Checkpoint 186 update — Ard'gral

Added the [Ard'gral source story dossier](../zone-stories/ARDGRAL.md) and [repository-wide audit supplement](ARDGRAL_AUDIT_REVIEW.md), covering the dream-plane atmosphere, nightmare essence and mask, complete local graph, twelve fixed rift destinations, six random surface gateways, the illusionist spell route, active Alatorin incoming rift, title and history, and the dormant Yggdrasil VNUM collision. The all-95 inactive-WLD sweep finds 12 overlapping Yggdrasil room IDs, 21 target hits into those colliding IDs, and one source-only #30400 D0 edge to #30401 whose owner is ambiguous. This is not confirmed active ingress; curated source-only totals remain 3,439 overall, 205 for Surf, and 15 for `ud_map2`. Keep `ardgral` DO NOT MOVE and `ygg` RETAIN IN PLACE with existing candidate row [122](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L122); add no Ard'gral candidate row. The Alatorin coverage entry now records its active type-25 rift dependency.

The original 220-priority queue remains complete; the broader audit continues. The coverage register maps 226 comprehensive source dossiers to active area keys, with 125 active rows still lacking one. Current explicit `PENDING` markers remain on 225 narrative, 383 connectivity, 220 title/history, and 388 typed-dependency rows. Continue with `arena`, the next active contributor without a mapped dossier in register order. See [Ard'gral coverage row 13](ZONE_COVERAGE_REGISTER.csv#L13) and the [Alatorin coverage row 6](ZONE_COVERAGE_REGISTER.csv#L6).

Validation: Checkpoint 186 documentation link validation checked sixteen Markdown files plus Markdown links embedded in both registers: 4,235 local destinations and 1,470 line/heading anchors, with no missing targets or anchors. Coverage remains 476 data rows x 60 columns and the attic candidate register 121 x 10. `git diff --check` passed; `areas/` and `src/` are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run for this documentation-only checkpoint.

### Checkpoint 187 update — Arena

Added the [Arena source story dossier](../zone-stories/ARENA.md) and [repository-wide audit supplement](ARENA_AUDIT_REVIEW.md). The 87-room zone has 214/214 resolving local WLD exits in components of 81, 2, 2, and 2; the battle-map component has directed reach and return of 81/81 from #67906. No active WLD boundaries or inactive-WLD approaches were found. The runtime entrance is compiled object #67900: active Tharnadia resets it in The Ogre's Foot Lounge, and its callback sends eligible players to one of three team staging pairs before the match transfer. Source title `CTF Tournament` is unique among 442 normalized current `.zon` titles, while compiled startup defaults are disabled Deathmatch. Return destinations for all three teams need maintainer review: Goodie/Evil comments map to rooms whose current titles are the Swamp of the Clavikord and Goodie Bitchin Room, while Undead is -1 and its staging pair has no portal reset. Keep `arena` DO NOT MOVE; no attic row is added. The Tharnadia coverage entry now records its active reset dependency.

The original 220-priority queue remains complete; the broader audit continues. The coverage register maps 227 comprehensive source dossiers to active area keys, with 124 active rows still lacking one. Current explicit `PENDING` markers remain on 224 narrative, 383 connectivity, 219 title/history, and 387 typed-dependency rows. Continue with `astral_areas`, the next active contributor without a mapped dossier in register order. See [Arena coverage row 14](ZONE_COVERAGE_REGISTER.csv#L14), [Tharnadia row 297](ZONE_COVERAGE_REGISTER.csv#L297), the [Arena audit supplement](ARENA_AUDIT_REVIEW.md), and the [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-187-arena-disposition).

Validation: Checkpoint 187 documentation link validation checked eighteen Markdown files plus Markdown links embedded in both registers: 4,297 local destinations and 1,514 line/heading anchors, with no missing targets or anchors. Coverage remains 476 data rows x 60 columns and the attic candidate register 121 x 10. `git diff --check` passed; `areas/` and `src/` are unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run for this documentation-only checkpoint.

### Checkpoint 188 update — Astral Plane, Side Areas

Added the [Astral Side Areas source dossier](../zone-stories/ASTRAL_AREAS.md)
and [repository-wide audit supplement](ASTRAL_AREAS_AUDIT_REVIEW.md). The
61-room map has 141/141 resolving WLD exits in four weak components of 39, 18,
2, and 2 rooms. The rift and sphere boundaries reciprocate with Astral Main;
the old-fortress and staging/failure components have no active WLD inbound.
The #19860 pool routes to active Ethereal #12479; the other reset pool targets
#163372, defined only in inactive `Duris3.wld`, and its `magic_pool` callback
cannot resolve that destination in the full-world configuration. The unique
normalized title and bounded same-stem history show no whole-zone retirement.
The side-area fortress is a likely but unconfirmed candidate for the “old
gith fort” in the 2010 `githyanki_fortress` commit. Keep `astral_areas`,
`githyanki_fortress`, and `gith_ht` active and unmoved; retain `Duris3` in
place with existing candidate row [36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36)
until #163372 is resolved. Add no candidate row for Astral Side Areas.

The broader audit now maps 228 comprehensive source dossiers to active area
keys, with 123 active rows still lacking a mapped dossier. Current explicit
`PENDING` markers remain on 223 narrative, 383 connectivity, 218 title/history,
and 386 typed-dependency rows. Continue with `astral_main`, the next active
contributor without a mapped dossier in register order. See [Astral Side
Areas coverage row 16](ZONE_COVERAGE_REGISTER.csv#L16), related rows
[17–18](ZONE_COVERAGE_REGISTER.csv#L17), [92](ZONE_COVERAGE_REGISTER.csv#L92),
[106](ZONE_COVERAGE_REGISTER.csv#L106), and
[121](ZONE_COVERAGE_REGISTER.csv#L121), plus the [connectivity
record](WORLD_CONNECTIVITY.md#checkpoint-188-astral-plane-side-areas),
[replacement-history record](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-188-astral-plane-side-areas-title-and-path-history),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-188-astral-plane-side-areas-disposition).

Validation: Checkpoint 188 documentation-link validation checked twenty
Markdown inputs plus links embedded in both registers: 4,386 local destinations
and 1,592 line/heading anchors, with no missing targets or anchors. Coverage
remains 476 data rows x 60 columns; the attic candidate register remains 121
data rows x 10 columns. `git diff --check` passed, and `areas/` and `src/` are
unchanged. No tests, build, server, gameplay session, database operation,
migration, or operational script was run for this documentation-only
checkpoint.

### Checkpoint 189 — Astral Main

Added a mapped Astral Main source dossier and detailed repository-wide audit. The active map has 125 rooms and 748/748 resolving WLD exits in one weak component; raw directed traversal from #19701 reaches and returns from all 125 rooms. Five outbound WLD boundaries reciprocate, and 21 active WLD edges enter from seven areas. The all-95 inactive-WLD sweep found no room overlap or approach. The review traces type-25 portals, plane-gate and innate routes, magic-pool targets, exact reset owners, sidecar contacts, code bindings, the empty QST file, title, and bounded history. It records invalid mobile #19860 and object #19997 resets, the missing #19890 room binding, the reciprocal pool route between #19945/#19946, and a Duris3-only pool destination. The story dossier also follows note #19815, a Githzerai spying clue reset inside the desk in Astral Side Areas' chief quarters. Keep `astral_main` DO NOT MOVE and preserve existing `nexus56` and `alatorin_objects` candidate rows. See coverage row [17](ZONE_COVERAGE_REGISTER.csv#L17), [Astral Main audit supplement](ASTRAL_MAIN_AUDIT_REVIEW.md), [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-189-astral-main), [history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-189-astral-main-title-and-path-history), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-189-astral-main-disposition).

There are now 229 comprehensive source dossiers mapped to active area keys, with 122 active contributors still lacking one. Dedicated source-story reviews cover 32 zones. Current explicit `PENDING` markers remain on 222 narrative, 383 connectivity, 217 title/history, and 385 typed-dependency rows. Continue with `astral_tiamat`, the next active contributor without a mapped dossier in register order.

### Checkpoint 190 — Astral Plane, Tiamat

Added the [Astral Tiamat source dossier](../zone-stories/ASTRAL_TIAMAT.md) and
[repository-wide audit supplement](ASTRAL_TIAMAT_AUDIT_REVIEW.md). They cover
the 31-room WLD graph, Redferne battle and incomplete inscription, Tiamat's
lair and Vault of Avernus, six reset-door pairs, the reciprocal Main pool
route, the unbound local type-25 object, all exact cross-area ZON imports,
external quest consumers, typed code bindings, title/path history, and builder
questions. Corrected the Tiamat supplement to classify #19911/#19916 as
Astral Tiamat-owned callback outputs and #19907 as an unbound prototype. Added
Realm and Winterhaven vault inbounds and Heavens/Winterhaven quest-item
consumers to the linked coverage rows. Keep `astral_tiamat` DO NOT MOVE; no
attic candidate row is added.

The audit now maps 230 comprehensive source dossiers to active area keys, with
121 active contributors still lacking one. Dedicated source-story reviews
cover 33 zones. Current explicit `PENDING` markers remain on 221 narrative,
383 connectivity, 216 title/history, and 384 typed-dependency rows. Continue
with `asylum`, the next active contributor without a mapped dossier in register
order. See coverage row [18](ZONE_COVERAGE_REGISTER.csv#L18), [connected rows
16–18](ZONE_COVERAGE_REGISTER.csv#L16), [Realm row
251](ZONE_COVERAGE_REGISTER.csv#L251), [Heavens row
121](ZONE_COVERAGE_REGISTER.csv#L121), [Tiamat row
302](ZONE_COVERAGE_REGISTER.csv#L302), [Winterhaven row
345](ZONE_COVERAGE_REGISTER.csv#L345), and the [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-190-astral-plane-tiamat-disposition).
Validation: the Checkpoint 190 documentation checker passed with 187 local
links and 135 line/heading anchors across 17 document/register excerpts. The
coverage register remains 476 data rows by 60 columns and the attic candidate
register 121 by 10. `git diff --check` passed, and `areas/` and `src/` are
unchanged. No tests, build, server, gameplay session, database operation,
migration, or operational script was run for this documentation checkpoint.

### Checkpoint 191 — Asylum

Added the [Asylum source story dossier](../zone-stories/ASYLUM.md) and
[repository-wide audit supplement](ASYLUM_AUDIT_REVIEW.md). The source review
maps all 89 rooms, 15 mobile prototypes, 49 object prototypes, and 225 reset
commands. The active graph has 202/202 resolved exits in one weak component;
raw directed local traversal reaches and returns from 89/89 rooms from each
active inbound anchor. Three WLD exits enter from Surface, Pineholl, and White
Harbor; one exit returns to Surface. All 56 door exits have matching reset
rows, including the keyed secret altar-niche pair.

Typed closure found the active `random` shop and reset references to Asylum
object #9225; a broad numeric code hit is the percentage `0.9261%`, not a
VNUM. Title and followed same-stem histories show no replacement. The inactive
Surf source adds one non-overlapping approach to Asylum, raising curated
source-only totals to 3,440 overall, 206 for Surf, and 15 for `ud_map2`. Keep
Asylum DO NOT MOVE and Surf RETAIN IN PLACE under its existing candidate row.

The audit now maps 231 comprehensive source dossiers to active area keys, with
120 active contributors still lacking one. Dedicated source-story reviews
cover 34 zones. Current explicit `PENDING` markers remain on 220 narrative,
383 connectivity, 215 title/history, and 383 typed-dependency rows. Continue
with the next unmatched active area in register order. See coverage row
[19](ZONE_COVERAGE_REGISTER.csv#L19), the [connectivity
checkpoint](WORLD_CONNECTIVITY.md#checkpoint-191-asylum), [history
checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-191-asylum-title-and-path-history),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-191-asylum-disposition).
Continue with The Plane of Avernus (`avernus`), the next unmatched active
contributor in register order.

### Checkpoint 192 — Avernus

Added the [Avernus source story dossier](../zone-stories/AVERNUS.md) and
[repository-wide audit supplement](AVERNUS_AUDIT_REVIEW.md). The source review
maps 392 rooms, 25 mobile prototypes, 33 object prototypes, one local shop
record, and 275 reset commands. Its 1,437 authored exits all resolve; local
weak components contain 375, 16, and one room. Static local traversal from
#32031 reaches 369/375 rooms in the main component, while all 375 can return.
The only active WLD boundary is reciprocal with Tiamat. Seven current records
exist; authored door/key fields, door resets, and falls are absent.

Typed closure records 72 imported M/F reset actions, the Bctdl type-25 pair,
six Ravenloft2 type-25 arrivals, the conditional compiled Hell gate, external
QST/story item uses, three Tiamat resets of Avernus object #32019, and the
shadow-staff code dependencies on Undermountain #92076 and Heavens #200. The
normalized title repeats on inactive `plane_avernus`, a two-room component
with no WLD overlap or approach; bounded histories show no replacement. Keep
`avernus` DO NOT MOVE and the existing `plane_avernus` candidate RETAIN IN
PLACE. Static source evidence does not qualify live reset state, access,
dynamic route resolution, arrival safety, or return.

The audit now maps 232 comprehensive source dossiers to active area keys, with
119 active contributors still lacking one. Dedicated source-story reviews
cover 35 zones. Current explicit `PENDING` markers remain on 219 narrative,
383 connectivity, 214 title/history, and 382 typed-dependency rows. Continue
with `black_tunnels`, the next unmatched active contributor in register order.
See coverage row [20](ZONE_COVERAGE_REGISTER.csv#L20), [connectivity
checkpoint](WORLD_CONNECTIVITY.md#checkpoint-192-avernus), [history
checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-192-avernus-title-and-path-history),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-192-avernus-disposition).

### Checkpoint 193 — Black Tunnels

Added the [Black Tunnels source story dossier](../zone-stories/BLACK_TUNNELS.md)
and [repository-wide audit supplement](BLACK_TUNNELS_AUDIT_REVIEW.md). The
active source set has 82 rooms, 16 mobile prototypes, 29 objects, and 84 ZON
commands. It has no local `.qst`, native Q/QA contract, `.shp`, or story
sidecar; its separate `areas/qst/AREA` membership is not a local quest-file
claim. Its 200 authored exits resolve (198 local, two outbound) in one 82-room
weak component. Raw directed traversal from both inbound anchors reaches and
returns from all 82 rooms when authored exits are available; a conservative
as-reset traversal reaches 59/82. Both Pragtog and Khildarak WLD boundaries
are reciprocal. Ten door exits have matching ZON resets, and Khildarak object
#17097 declares an additional open-command arrival into #10721. Static source
scans found no other exact external QST, SHP, sidecar, ZON reset, duplicate
prototype, switch, or code-callback reference. Title and bounded same-stem
history show no replacement. Keep `black_tunnels` DO NOT MOVE and add no attic
candidate row.

The audit now maps 233 comprehensive source dossiers to active area keys, with
118 active contributors still lacking one. Dedicated source-story reviews
cover 36 zones. Current explicit `PENDING` markers remain on 218 narrative,
383 connectivity, 213 title/history, and 381 typed-dependency rows. Continue
with `bloody_plains`, the next active contributor without a mapped dossier in
register order. See coverage row [31](ZONE_COVERAGE_REGISTER.csv#L31),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-193-black-tunnels),
[history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-193-black-tunnels-title-and-path-history),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-193-black-tunnels-disposition).

Validation: `git diff --check` passed. The coverage register remains 476 data
rows by 60 columns and the attic candidate register 121 by 10. The source tree
under `areas/` and `src/` is unchanged. No tests, build, server, gameplay
session, database operation, migration, or operational script was run for this
documentation checkpoint.

### Checkpoint 194 — Bloody Plains

Added the [Bloody Plains source-story dossier](../zone-stories/BLOODY_PLAINS.md)
and [repository-wide audit supplement](BLOODY_PLAINS_AUDIT_REVIEW.md). The
active source set has 199 rooms, nine mobile prototypes, eighteen objects, and
100 ZON commands. It has no local QST, native Q/QA contract, SHP file, or story
sidecar. Its 704 authored exits all resolve (698 local, six outbound) in one
199-room weak component. Seven inbound anchors each statically reach and
return from all 199 rooms; five Killing Fields boundaries and the Surface
boundary reciprocate, while the Killing Fields #33422-to-#33597 edge has no
direct return. With the reset-closed shack door pair excluded, 198 rooms are
reachable and returnable from each inbound anchor. Twelve fall markers range
from 78% to 100%; live fall, door, access, and safe-return behavior were not
qualified. The all-95 inactive-WLD scan adds one non-overlapping source-only
Surf approach, #280345 north to #33681, raising curated source-only totals to
3,441 overall and 207 for Surf; ud_map2 remains 15. Active mobs_surf declares
twenty resets of local mobile #33500 to room #31412, and unregistered surf.zon
declares fourteen more source-only reset imports. Exact typed scans found no
external QST/SHP/sidecar item references, duplicate prototypes, code callback,
or external type-25/type-29 target. The normalized title is unique and bounded
same-stem history shows no replacement. Keep bloody_plains DO NOT MOVE; preserve
Surf candidate row 97 RETAIN IN PLACE. Current coverage is 234 mapped source
dossiers, 117 active contributors without one, and 37 detailed source-story
reviews. Current case-insensitive PENDING counts are 217 narrative, 383
connectivity, 212 title/history, and 380 dependency rows. Continue with
bogentok, the next unmatched active contributor. See coverage row
[33](ZONE_COVERAGE_REGISTER.csv#L33), [connectivity
checkpoint](WORLD_CONNECTIVITY.md#checkpoint-194-bloody-plains), [history
checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-194-bloody-plains-title-and-path-history),
[attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-194-bloody-plains-disposition),
and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 195 — BogenTok

Added the [BogenTok source-story dossier](../zone-stories/BOGENTOK.md) and
[repository-wide audit supplement](BOGENTOK_AUDIT_REVIEW.md). The active area
has 205 rooms, 66 mobile prototypes, 46 objects, three shops, and 512 ZON
commands. Its 440 authored exits all resolve (438 local and two outbound) in
one weak component. Two active Underdark entries each statically reach and
return from all 205 rooms when authored exits are available. Excluding the 72
reset-closed door exits leaves 152 rooms reachable from each inbound anchor;
live access and door behavior remain unqualified. BogenTok has reset mode 2
and no local `.qst`, native Q/QA contract, or story sidecar, though its
bartender and inn use generic code callbacks.

The all-95 inactive-WLD scan adds one non-overlapping source-only approach:
`ud_map2.wld` #1128575 west to #18700. This is not active ingress. The
reviewed `ud_map2` subset rises to 16, curated totals rise to 3,442 overall,
and Surf remains 207. Active `alatorin.zon` imports local mobile and object
#18718 as distinct typed prototypes. The normalized title is unique and
bounded same-stem history shows no replacement. Keep `bogentok` **DO NOT
MOVE** and add no candidate row; preserve the inactive approach with existing
`ud_map2` row 114 **RETAIN IN PLACE**.

Coverage now maps 235 comprehensive source dossiers to active area keys, with
116 active contributors still lacking one. Dedicated source-story reviews
cover 38 zones. Case-insensitive PENDING counts are 216 narrative, 383
connectivity, 211 title/history, and 379 typed-dependency rows. Continue with
`bordello`, the next unmatched active contributor in register order. See
coverage row [34](ZONE_COVERAGE_REGISTER.csv#L34), [connectivity
checkpoint](WORLD_CONNECTIVITY.md#checkpoint-195-bogentok), [history
checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-195-bogentok-title-and-path-history),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-195-bogentok-disposition).

Documentation validation checked the checkpoint links and line anchors,
preserved the 476-row by 60-column coverage register and 121-row by 10-column
candidate register, and passed `git diff --check`. `areas/` and `src/` remain
unchanged. No tests, build, server, gameplay session, database operation,
migration, or operational script was run.

## Checkpoint 196 coverage reconciliation

The active Bordello contributor now has a mapped
[source-story dossier](../zone-stories/BORDELLO.md) and
[repository-wide supplement](BORDELLO_AUDIT_REVIEW.md). The coverage register
records its one shop, 69-room static graph, three active WLD arrivals, one
reciprocal Surface pair, reset-open hidden-room door state, code routes,
external object #45 import, title/history result, and **DO NOT MOVE**
disposition. The two inactive Surf approaches remain on Surf's existing
**RETAIN IN PLACE** candidate row.

The active dossier count rises to 236 of 351 contributors, leaving 115 without
a mapped dossier. Dedicated source-story reviews cover 39 zones. Case-insensitive
PENDING counts are now 215 narrative, 383 connectivity, 210 title/history,
and 378 typed-dependency rows. The two source-only approaches raise curated
totals to 3,444 overall and 209 for Surf; `ud_map2` remains 16. See [coverage
row 35](ZONE_COVERAGE_REGISTER.csv#L35), the [connectivity
checkpoint](WORLD_CONNECTIVITY.md#checkpoint-196-vellas-bordello), the
[history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-196-vellas-bordello-title-and-path-history),
and the [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

## Checkpoint 197 coverage reconciliation

The existing comprehensive
[Bugentolen dossier](../zone-stories/BUGENTOLEN.md) was present but missing
from its coverage-row dossier mapping. Checkpoint 197 adds that mapping and a
[repository-wide reconciliation](BUGENTOLEN_AUDIT_REVIEW.md) with refreshed
directed reach, reset-door, cross-boundary, typed-reference, and history
evidence. This maps an existing review; it does not add a new zone dossier.

The active dossier count is now 237 of 351 contributors, leaving 114 without
a mapped dossier. Current case-insensitive PENDING counts remain 215 narrative,
383 connectivity, 210 title/history, and 378 typed-dependency rows. The already
recorded Surf approach remains source-only and does not change curated totals:
3,444 overall, 209 for Surf, and 16 for `ud_map2`. See [coverage row
42](ZONE_COVERAGE_REGISTER.csv#L42), the [connectivity
checkpoint](WORLD_CONNECTIVITY.md#checkpoint-197-bugentolen-reconciliation),
the existing [title/path-history record](DUPLICATE_AND_REPLACED_ZONES.md#bugentol-and-parallel-surface-map-records),
and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Repository-wide audit checkpoint 198 — Buildings

Added the [Buildings source dossier](../zone-stories/BUILDINGS.md) and
[repository-wide audit supplement](BUILDINGS_AUDIT_REVIEW.md). The static WLD
has 100 identical rooms and no authored exits, active inbounds/outbounds, or
source-only approaches. C++ reserves these rooms for generated outpost
interiors and can create runtime entry, down-exit, and optional guildhall
portal links from three Surface locations. Compiled consumers and saved
outpost state make the active area **DO NOT MOVE**; no attic candidate row is
added.

The coverage register now maps 238 of 351 active contributors, leaving 113
without a mapped dossier. Case-insensitive PENDING counts are 214 narrative,
383 connectivity, 209 title/history, and 378 typed-dependency rows. Detailed
source-story reviews in this follow-up cover 40 zones. Curated source-only
totals remain 3,444 overall, 209 for Surf, and 16 for `ud_map2`; Surf's
existing candidate row retains 145 active inbound references. Continue with
`cbl_hall`, the next unmapped active contributor. See [coverage row
44](ZONE_COVERAGE_REGISTER.csv#L44), the [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-198-buildings-static-shells-and-runtime-outposts),
the [title/history record](DUPLICATE_AND_REPLACED_ZONES.md#buildings-and-generated-outpost-interiors),
and the [attic plan](ATTIC_MIGRATION_PLAN.md#checkpoint-198-buildings-shared-template-range).

Validation: `git diff --check`, local Markdown target/line-link checks, CSV
dimensions, mapped dossier counts, and review counts passed. `areas/` and
`src/` remain unchanged. No tests, build, server, gameplay session, database
read/write, migration, or operational script was run.


### Repository-wide audit checkpoint 199 — CBL Hall

Added the [CBL Hall source dossier](../zone-stories/CBL_HALL.md) and
[repository-wide supplement](CBL_HALL_AUDIT_REVIEW.md). The active CBL row now
records its two shops, 202/202 resolving exits, Heavens boundary, 13 local weak
components, 10/124 static directed reach from #29703, arena/inn code references,
and one inactive `ud_map2.wld` approach into CBL. The encoded destinations of
its eight reset-placed ITEM_TELEPORT objects remain outside established WLD
reachability pending runtime qualification. The source has no local QST file,
Q/QA contract, or story sidecar.

The duplicate review records Monsteri and cbl_outpost2 as unregistered copies
with matching titles, room/mobile VNUM ranges and reset patterns, plus content
shared with active CBL #29820–#29824. It establishes no precedence or
replacement. Keep active CBL **DO NOT MOVE**, and keep both candidate sources
**RETAIN IN PLACE**. The source-only `ud_map2` approach to #29703 is preserved
on candidate row 114. The two shops’ stale #6089 product reference also remains
an owner decision; its former Tharnadia prototype is now #132589.

The coverage register now maps 239 of 351 active contributors, leaving 112
without a mapped dossier. Detailed source-story reviews cover 41 zones.
Case-insensitive PENDING counts are 213 narrative, 383 connectivity, 208
title/history, and 378 typed-dependency rows. Curated source-only totals are
3,445 overall, 209 for Surf, and 17 for `ud_map2`. Continue with `celestia`,
the next unmapped active contributor. See [coverage row
48](ZONE_COVERAGE_REGISTER.csv#L48), [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-199-cbl-hall-static-graph-and-source-only-approach),
[duplicate-history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-199-cbl-hall-and-monster-island-copies),
and [attic plan](ATTIC_MIGRATION_PLAN.md#checkpoint-199-cbl-hall-and-monster-island-candidates).

### Repository-wide audit checkpoint 200 — Celestia

Added the [Celestia source dossier](../zone-stories/CELESTIA.md) and
[repository-wide audit supplement](CELESTIA_AUDIT_REVIEW.md). The active area
has 230 rooms, 88 mobiles, 46 objects, 896 resolving exits, two inbound WLD
arrivals, one outbound Avernus boundary, and three weak components sized
159/70/1. The separate 70-room Titan Palace component has no WLD or local
type-25 route to the main component in the inspected sources. Both inbound
arrivals reach #45718, whose only authored exit is reset closed, locked, and
secret; a reset-loaded portal encodes a possible route to #45500, but live
traversal and safety remain unqualified.

The `.zon` title “Celestial Plane” is unique among all 442 current source
titles. Followed same-stem history establishes no alternate-stem predecessor
or whole-zone replacement. Typed scans record Astral/Ethereal return-portal
resets, external potion and equipment consumers, compiled boss/set/random-zone
references, and the competing external definitions of reset object #63. Keep
active `celestia` **DO NOT MOVE**; no candidate row is added.

Coverage now maps 240 of 351 active contributors, leaving 111 without a mapped
dossier. Detailed source-story reviews cover 42 zones. Case-insensitive PENDING
counts are 212 narrative, 383 connectivity, 207 title/history, and 378
typed-dependency rows. Curated source-only totals remain 3,445 overall, 209 for
Surf, and 17 for `ud_map2`. Continue with `charing`, the next unmatched active
contributor. See [coverage row 49](ZONE_COVERAGE_REGISTER.csv#L49), the
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-200-celestia-static-graph-and-reset-placed-routes),
the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-200-celestia-title-and-path-history),
and the [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-200-celestia-active-zone-retention).

Documentation validation rechecked CSV dimensions, dossier mapping, review
counts, local Markdown targets, and `git diff --check`. No native source,
build, test, server, gameplay, database, migration, or operational change was
part of this checkpoint.

### Repository-wide audit checkpoint 201: Charing

Mapped the [Charing source dossier](../zone-stories/CHARING.md) and added a
[repository-wide audit supplement](CHARING_AUDIT_REVIEW.md). The checkpoint
reuses the existing [detailed Charing story review](CHARING_STORY_REVIEW.md),
so the dedicated review count remains 42. Charing has 210 rooms, 95 mobiles,
58 objects, 14 shops, and 496 ZON commands. Its WLD has 464 local exits and
three boundary records; two positive targets do not resolve in the active
loader. Reset-aware reach, compiled room/mobile bindings, source-only map
approaches, and typed object references are recorded in the supplement.

The title “Charing” is unique among 442 current zone sources. Tracked history
shows same-stem edits plus a partial 2014 Quartermaster move into `siege`, then
the 2026 retirement of that source and deletion of Charing’s corresponding
reset. No whole-zone predecessor or replacement is established. Keep active
`charing` **DO NOT MOVE**. Preserve the two `surf.wld` approaches on Surf’s
candidate row; keep `surf`, `surface2011`, and `nexus56` **RETAIN IN PLACE**.

Coverage maps 241 of 351 active contributors, leaving 110 without a mapped
dossier. Case-insensitive PENDING counts are 212 narrative, 383 connectivity,
206 title/history, and 378 typed-dependency rows. Curated source-only totals
are 3,447 overall, 211 for Surf, and 17 for `ud_map2`. Continue with
`cityruin`, the next unmapped active contributor. See [coverage row
55](ZONE_COVERAGE_REGISTER.csv#L55), the [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-201-charing-reset-aware-static-graph),
the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-201-charing-title-and-path-history),
and the [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-201-charing-and-source-only-dependencies).

### Repository-wide audit checkpoint 202: Cityruin

Mapped the [Cityruin source dossier](../zone-stories/CITYRUIN.md) and added a
[repository-wide audit supplement](CITYRUIN_AUDIT_REVIEW.md), reusing the
existing [detailed Cityruin story review](CITYRUIN_STORY_REVIEW.md). The
dedicated story-review count remains 42. Cityruin has 155 rooms and 327
resolving WLD exits in one local weak component. All-exits-available directed
reach from the active Tharnadian Ruin arrival #66200 covers all 155 rooms;
after unconditional door resets, 278 of 326 local exits are already
open/non-secret/non-blocked and strict reset-open reach from #66200 is 1/155.
The paired entry gate resets closed but unlocked, so that strict open-edge
count does not establish inaccessibility.

The audit reconciles Cityruin's active QST and story-sidecar dependencies from
Tharnadian Ruin, compiled guild-room and service references, imported objects,
and the already-counted source-only Surf approach. The title is unique among
442 current zone sources and same-stem histories establish no whole-zone
replacement. Keep active `cityruin` **DO NOT MOVE** and preserve Surf's source
group **RETAIN IN PLACE**. Keep `nexus56` candidate row 79 **RETAIN IN PLACE**
with its source-only reset and shop references to Cityruin object #66211.

Coverage now maps 242 of 351 active contributors, leaving 109 without a mapped
dossier. The existing 42 dedicated story reviews remain unchanged.
Case-insensitive PENDING counts remain 212 narrative, 383 connectivity, 206
title/history, and 378 typed-dependency rows. Curated source-only totals remain
3,447 overall, 211 for Surf, and 17 for `ud_map2`; Cityruin's Surf approach was
already counted. Continue with `cyrictunnel`, the next unmapped active
contributor. See [coverage row 58](ZONE_COVERAGE_REGISTER.csv#L58), the
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-202-cityruin-reset-aware-access),
the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-202-cityruin-title-and-path-history),
and the [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-202-cityruin-and-surf-retention).

### Repository-wide audit checkpoint 203: cyrictunnel

Mapped the [cyrictunnel source dossier](../zone-stories/CYRIC_TUNNELS.md) and
the [audit supplement](CYRICTUNNEL_AUDIT_REVIEW.md), reusing the existing
detailed story-review set; its count remains 42. The zone has 82 rooms and
195 resolving WLD exits in one local weak component. Five active reciprocal
crossings connect Surface, three Underdark locations, and Connector Zones.
Strict reset-open reach ranges from 1/82 to 40/82 by active arrival; the
source-defined cave-in and statue switches and matching keys are documented
without claiming live gameplay qualification.

The inactive-WLD scan adds five non-overlapping source-only Surf approaches.
Keep active cyrictunnel DO NOT MOVE and Surf candidate row 97 RETAIN IN PLACE.
The title is unique among 442 current sources, and followed same-stem history
establishes no whole-zone replacement. The QST/AREA entry without a local QST
source and the lava sector assignment remain owner questions.

Coverage now maps 243 of 351 active contributors, leaving 108 unmapped.
Case-insensitive PENDING counts are 211 narrative, 382 connectivity, 205
title/history, and 377 typed-dependency rows. Curated source-only totals are
3,452 overall, 216 for Surf, and 17 for ud_map2. Continue with dark_forest.
See [coverage row 68](ZONE_COVERAGE_REGISTER.csv#L68), the [connectivity
checkpoint](WORLD_CONNECTIVITY.md#checkpoint-203-cyrictunnel-reset-aware-access),
the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-203-cyrictunnel-title-and-history),
and the [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-203-cyrictunnel-and-surf-retention).

### Repository-wide audit checkpoint 204: dark_forest

Mapped the [dark_forest source dossier](../zone-stories/DARK_FOREST.md) and
the [audit supplement](DARK_FOREST_AUDIT_REVIEW.md), reusing the existing
detailed story-review set; its count remains 42. The zone has 171 rooms and
354 resolving WLD exits in one local weak component. All-exits-available
directed reach from arrival #75171 covers all 171 rooms; after unconditional
door resets, 340 of 352 local exits are already open and strict reach is
147/171. Three Surface inbounds converge on #75171 and two return edges are
reciprocal.

The inactive-WLD scan adds one non-overlapping source-only Surf approach.
Keep active dark_forest DO NOT MOVE and Surf row 97 RETAIN IN PLACE. The
normalized title is unique, Darkfall Forest is a separate active zone, and
followed same-stem history establishes no whole-zone replacement. The
compiled archer binding, river sector, and bridge flags are documented;
live travel and safety remain unqualified.

Coverage now maps 244 of 351 active contributors, leaving 107 unmapped.
Case-insensitive PENDING counts are 210 narrative, 382 connectivity, 204
title/history, and 376 typed-dependency rows. Curated source-only totals are
3,453 overall, 217 for Surf, and 17 for ud_map2. Continue with yuan_ti2 (Village of the Damned), the next unmapped active contributor. See [coverage row
69](ZONE_COVERAGE_REGISTER.csv#L69), the [connectivity
checkpoint](WORLD_CONNECTIVITY.md#checkpoint-204-dark-forest-reset-aware-access),
the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-204-dark-forest-title-and-history),
and the [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-204-dark-forest-and-surf-retention).

### Repository-wide audit checkpoint 205: yuan_ti2

Mapped the [Village of the Damned source dossier](../zone-stories/YUAN_TI2.md)
and the [audit supplement](YUAN_TI2_AUDIT_REVIEW.md), reusing the existing
detailed story-review set; its count remains 42. The 79-room area has 167/167
resolving exits, one local weak component, and two reciprocal active
boundaries. Four door resets leave 161/165 local exits open; strict directed
reach is 75/79 from #80600 and 1/79 from #80671, which retains an immediate
paired return to the neighboring `yuan_ti` area. The remaining two rooms are
source-labeled error/load rooms without inbound WLD exits, though #80677 has
a reset mobile.

Two non-overlapping source-only approaches from inactive `oldmap8.wld` and
`surf.wld` both target #80600. Keep active `yuan_ti2` DO NOT MOVE and preserve
the approaches with oldmap8 row 81 and Surf row 97 RETAIN IN PLACE. Static
typed scans find no external typed reference to a local prototype; local
imports, the two unplaced mobiles, and the note, Githyanki, and load-room
questions are documented. The unique normalized title and followed same-stem
history establish no whole-zone replacement.

Coverage now maps 245 of 351 active contributors, leaving 106 unmapped.
Case-insensitive PENDING counts are 209 narrative, 382 connectivity, 203
title/history, and 375 typed-dependency rows. Curated source-only totals are
3,455 overall, 218 for Surf, and 17 for `ud_map2`. Continue with `outpost`.
See [coverage row 352](ZONE_COVERAGE_REGISTER.csv#L352), the [connectivity
checkpoint](WORLD_CONNECTIVITY.md#checkpoint-205-yuan-ti2-reset-aware-access),
the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-205-yuan-ti2-title-and-history),
and the [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-205-yuan-ti2-and-source-only-retention).

### Repository-wide audit checkpoint 206: outpost

Mapped the [Human Outpost source dossier](../zone-stories/OUTPOST.md) and
[audit supplement](OUTPOST_AUDIT_REVIEW.md), reusing the existing detailed
story-review set; its count remains 42. The 40-room area has 81/81 resolving
exits, 80 local and one outbound, in one local weak component. Its single
active Surface boundary is reciprocal. Reset doors close 24 local directed
exits across 12 pairs; strict directed reach from #82124 is 25/40.

Lord Perrin's local QST dialogue, key #82110, and the locked Cathedral pair in
active Tharnadian Ruin establish a cross-area quest and key dependency. The
same key also locks the Outpost Treasury, while a nested holy symbol is
separately described as a Cathedral key. Local resets place all 22 mobiles
and all 21 objects; the shop imports three Heavens medical supplies.

The inactive-WLD scan found one source-only Surf approach, #206585 east to
#82124. Preserve it on Surf candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97)
**RETAIN IN PLACE**; it is not active ingress. Keep active outpost **DO NOT
MOVE**. The normalized title is unique among 442 current zone sources, and
followed same-stem history establishes no whole-zone replacement. Coverage
now maps 246 of 351 active contributors, leaving 105 unmapped. Case-insensitive
PENDING counts are 208 narrative, 382 connectivity, 202 title/history, and 374
typed-dependency rows. Curated source-only totals are 3,456 overall, 219 for
Surf, and 17 for `ud_map2`. Continue with `lab`. See [coverage row
227](ZONE_COVERAGE_REGISTER.csv#L227), the [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-206-outpost-reset-aware-access),
the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-206-outpost-title-and-history),
and the [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-206-outpost-and-surf-retention).

### Repository-wide audit checkpoint 207: lab

Mapped the [Swamp Laboratory source dossier](../zone-stories/LAB.md) and
[audit supplement](LAB_AUDIT_REVIEW.md); the existing detailed story-review
set remains 42. The 51-room area has 100/100 resolving WLD exits, 99 local
and one outbound, with two active inbounds and a reciprocal Kvarkpass
boundary. Its WLD-only graph has four weak components; ten closed-reset
directions across five pairs leave 89/99 local exits open and strict reach is
2/51 from #82350. Seven local enterable object portals link WLD-disconnected
groups.

The all-95 inactive-WLD scan adds source-only Surf and Underworld2 approaches
to #82350. Preserve them on candidate rows 97 and 116 RETAIN IN PLACE; neither
is active ingress. The audit records the Desert Mines of Khul'Lor textual
connection, frog note naming prince Uklor, imported Heavens rune stone, and
the locked local sphere/key/cube chain. Static typed scans find no external
typed prototype reference, and the normalized title and followed same-stem
history establish no whole-zone replacement. The current WLD inventory is
446 files: 351 active and 95 unregistered, correcting the prior checkpoint's
inactive-file denominator from 96 to 95.

Coverage now maps 247 of 351 active contributors, leaving 104 unmapped. The
existing 42 detailed story reviews remain unchanged. Case-insensitive PENDING
counts are 207 narrative, 382 connectivity, 201 title/history, and 373
typed-dependency rows. Curated source-only totals are 3,458 overall, 220 for
Surf, and 17 for `ud_map2`; Underworld2 now has seven recorded approaches.
Continue with `kuotoa`, the next unmapped active contributor. See [coverage
row 153](ZONE_COVERAGE_REGISTER.csv#L153), the [connectivity
checkpoint](WORLD_CONNECTIVITY.md#checkpoint-207-lab-reset-aware-access),
the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-207-lab-title-and-history),
and the [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-207-lab-and-source-only-retention).

### Repository-wide audit checkpoint 208: kuotoa

Added the [Kuo-Toa Temple source dossier](../zone-stories/KUOTOA.md) and [audit supplement](KUOTOA_AUDIT_REVIEW.md). The local source has 37 rooms, six mobiles, seven objects, and 67 reset commands. Its raw WLD graph has one weak component; 28 closed directional door resets across 14 pairs leave 14/37 rooms directed-reachable from #88200. The Underworld boundary at #4454/#88200 is reciprocal. Room #88228’s flags and city sector stand apart from the other 36 underground-inside rooms.

Exact repository scans found no external typed consumers, duplicated local prototypes, outside door-key references, or outside teleport/switch targets. One exact external WLD edge is Underworld #4454 east to #88200, the reciprocal active boundary; no inactive-WLD approach is found. The normalized zone title is unique among 442 current titles, and same-stem source history establishes no replacement. The deity has competing local and neighboring spellings. No local QST, shop, sidecar, or native Q/QA contract is present despite the QST/AREA listing.

Coverage maps 248 of 351 active contributors, leaving 103 unmapped; the 42-entry detailed story-review set is unchanged. Case-insensitive PENDING counts are 206 narrative, 382 connectivity, 200 title/history, and 372 typed-dependency rows. Curated source-only totals remain 3,458 overall, 220 for Surf, and 17 for ud_map2; Underworld2 has seven approaches. Continue with udmini. See [coverage row 151](ZONE_COVERAGE_REGISTER.csv#L151), the [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-208-kuotoa-reset-aware-access), the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-208-kuo-toa-temple-title-and-history), and the [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-208-kuotoa-and-source-only-retention).

### Repository-wide audit checkpoint 209: udmini

Added the [Minizones source dossier](../zone-stories/UDMINI.md) and [audit supplement](UDMINI_AUDIT_REVIEW.md). The 21-room local WLD has 39/39 resolving exits (38 local, one outbound), components of 19 and 2 rooms, two inbound active boundaries, and one outbound edge. Active arrivals reach the 19-room corpse-and-larvae cave component; the two mushroom rooms remain outside that WLD arrival graph. The F 100 fields in rooms #88502 and #88520 can start falling on entry through open downward exits.

Exact typed scans found two active outside QST consumers for spores object #88502: Myconid #2320 and Newbie’s Maerg #29218. Newbie also resets a second #88502 copy on its drow mother. The local zone resets spores nested in gills, a large crack teleporter to Kvarkpass #54811, and all four mobiles; mushroom teleporter object #88500 remains unreferenced by resets. Other external typed scans found no source reference, duplicate prototype, external portal target, or code reference to local VNUMs. The title and followed history establish no replacement; no inactive-WLD approach is found.

Coverage maps 249 of 351 active contributors, leaving 102 unmapped; the 42-entry detailed story-review set is unchanged. Case-insensitive PENDING counts are 205 narrative, 382 connectivity, 199 title/history, and 371 typed-dependency rows. Curated source-only totals remain 3,458 overall, 220 for Surf, and 17 for ud_map2; Underworld2 has seven approaches. Continue with morg. See [coverage row 322](ZONE_COVERAGE_REGISTER.csv#L322), the [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-209-udmini-reset-aware-access), the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-209-udmini-title-and-history), and the [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-209-udmini-and-source-only-retention).

### Repository-wide audit checkpoint 210: morg

Added the [Morg's Place source dossier](../zone-stories/MORG.md) and [audit supplement](MORG_AUDIT_REVIEW.md). The area contains 30 rooms, 19 mobiles, no object prototypes, 64 M reset commands, and 14 door-state commands. Its raw WLD graph is one 30-room component with one reciprocal boundary to UnderDark chasm #844836. The entrance door resets closed; two secret-door pairs connect the back room and wine cellar to a five-room wererat nest. The nearby UnderDark room titled “Morg's Palace” shares the chasm approach, while its identity relation remains unresolved.

Exact typed scans found no outside reset, quest, shop, sidecar, duplicate prototype, teleport/switch target, or code reference to local VNUMs. The WLD text has two unmapped-exit clues: a possible overhead hatch at #88614 and a northward inn hall at #88623. The proprietor's quarters describe a locked metal box although the local object file is empty. The normalized ZON title is unique among 442 titles, and followed history remains on the `morg` stem; no alternate-stem predecessor or zone replacement is established.

Coverage maps 250 of 351 active contributors, leaving 101 unmapped; the 43-entry detailed story-review set is current. Case-insensitive PENDING counts are 204 narrative, 382 connectivity, 198 title/history, and 370 typed-dependency rows. Curated source-only totals remain 3,458 overall, 220 for Surf, and 17 for ud_map2; Underworld2 has seven approaches. Continue with foggy_woods. See [coverage row 188](ZONE_COVERAGE_REGISTER.csv#L188), the [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-210-morg-reset-aware-access), the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-210-morgs-place-title-and-source-history), and the [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-210-morg-and-source-only-retention).

### Repository-wide audit checkpoint 211: foggy_woods

Added the [Fog Enshrouded Wood source dossier](../zone-stories/FOGGY_WOODS.md) and [audit supplement](FOGGY_WOODS_AUDIT_REVIEW.md); the detailed story-review set is now 44. The area has 213 rooms, 32 mobiles, 33 objects, and 136 non-S ZON commands. Its 607 source exits include 592 local exits, six active outbound edges, one Duris3-only target, and eight negative targets. The local raw graph is one component of 213 rooms, with six reciprocal active pairs: five to Surface and one to Underworld3. Twenty door commands leave static reset-open reach at 193/213 from each active arrival.

Exact typed references reveal outside uses of local objects: Icecrag uses wine #90017 as an input, Alatorin grants scalemail #90018, and Verspin resets coin object #90026. Unregistered Nexus56 ZON/SHP files also refer to #90000 and #90016. All local mobiles and 32 of 33 objects appear in local resets; #90025 is unused. Ulon is described as a shopkeeper, but no local SHP is present. No duplicated MOB/OBJ definition or external portal/switch target was found. The normalized title is unique among 442 current zone titles, and followed WLD/MOB/OBJ/ZON history establishes no replacement.

Coverage maps 251 of 351 active contributors, leaving 100 unmapped. Case-insensitive PENDING counts are 203 narrative, 382 connectivity, 197 title/history, and 369 typed-dependency rows. Curated source-only totals remain 3,458 overall, 220 for Surf, and 17 for ud_map2; Underworld2 retains seven approaches. Continue with dracopit. See [coverage row 100](ZONE_COVERAGE_REGISTER.csv#L100), the [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-211-foggy-woods-reset-aware-access), the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-211-fog-enshrouded-wood-title-and-source-history), and the [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-211-foggy-woods-and-source-only-retention).

### Repository-wide audit checkpoint 212: dracopit

Added the [Pit of Dragons source dossier](../zone-stories/DRACOPIT.md) and [audit supplement](DRACOPIT_AUDIT_REVIEW.md); the detailed story-review set is now 45. The area has 69 rooms, 16 mobiles, 15 objects, and 81 non-S ZON commands. Its 393 exits all resolve: 392 are local and one leads to Mountain Castle. Realm provides a one-way arrival at #15913; Mountain Castle’s reciprocal pair enters at #15968. The WLD has components of 64 and 5 rooms. Resetting the two closed/locked hallway exits leaves static reach at 64/69 from Realm and 1/69 from the Mountain Castle arrival.

Typed review records Mountain Castle’s outside key #37115 and its active quest reward, Caertannad’s active ITEM_TELEPORT to #15907, Heavens’ external object #359 reset to a local dragon lord, and compiled uses of local objects #15906 and #15907. The local portal targets pocket #15940. Reset mode 0 has a special stateful reset path, so actual reset and access remain unqualified. The dragon-hide cloak’s overlord-only description conflicts with placement on an undead follower. No local QST, SHP, story sidecar, or native Q/QA contract exists. No candidate row or curated source-only count changes.

Coverage maps 252 of 351 active contributors, leaving 99 unmapped. Case-insensitive PENDING counts are 202 narrative, 382 connectivity, 196 title/history, and 368 typed-dependency rows. Curated source-only totals remain 3,458 overall, 220 for Surf, and 17 for ud_map2; Underworld2 retains seven approaches. Continue with dragonnia. See [coverage row 78](ZONE_COVERAGE_REGISTER.csv#L78), the [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-212-dracopit-reset-aware-access), the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-212-pit-of-dragons-title-and-source-history), and the [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-212-dracopit-and-source-only-retention).

### Repository-wide audit checkpoint 213: dragonnia

Added the [Dragonnia source dossier](../zone-stories/DRAGONNIA.md) and [audit supplement](DRAGONNIA_AUDIT_REVIEW.md); the detailed story-review set is now 46. The area has 67 rooms, 32 mobiles, 67 objects, and 173 non-S ZON commands. Its 141 source exits all resolve in the active loader inventory: 139 local and two outbound Surface edges. Active inbound edges comprise two Surface arrivals at #6803, Realm #14210 to #6839, and Winterhaven #55633 to #6801; only the two Surface links are reciprocal. The local WLD has components of 66 and one room. Static strict reach after applying local door resets is 1/67 from #6801 and 52/67 from #6839 and #6803, while all-exit reach from each arrival target is 64/67. A local teleport item targets #6836. Reset mode 0 has special persisted-state behavior, so live access is unknown.

Typed review records the death-created Dragonnia heart used by active Winterhaven and Alatorin exchanges, Heavens object resets, compiled VNUM handlers, the local teleport object, three unmatched `G #4050` references, and a mismatch between the two palace-hall key fields. A source-only Sylvandawn shop product references local object #6834. No local QST, SHP, story sidecar, or native Q/QA contract exists. The title is unique and followed same-stem history establishes no replacement.

The all-95 inactive-WLD scan found two non-overlapping source-only approaches into Dragonnia #6803: Surf #252526 north and Underworld2 #431340 west. Both candidate files remain unregistered, and these edges are not active ingress. Coverage maps 253 of 351 active contributors, leaving 98 unmapped. Case-insensitive PENDING counts are 201 narrative, 382 connectivity, 195 title/history, and 367 typed-dependency rows. Curated source-only totals are 3,460 overall, 221 for Surf, and 17 for ud_map2; Underworld2 now has eight approaches. Continue with `dreggan_wood`. See [coverage row 79](ZONE_COVERAGE_REGISTER.csv#L79), the [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-213-dragonnia-reset-aware-access), the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-213-dragonnia-title-and-source-history), and the [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-213-dragonnia-and-source-only-retention).

### Repository-wide audit checkpoint 214: Dreggan Wood

Added the [Dreggan Woods source dossier](../zone-stories/DREGGAN_WOODS.md) and [audit supplement](DREGGAN_WOOD_AUDIT_REVIEW.md); the detailed story-review set is now 47. The area has 188 rooms, five mobiles, three objects, and 41 non-S ZON commands. Its 729 exits all resolve: 727 local and two outbound to Ethereal Plane and Shadamehr Keep. Both active cross-area pairs are reciprocal. The WLD is one weak component; directed reach from Ethereal room #41001 covers all 188 rooms, but from Shadamehr room #41186 covers 89. No door resets, falls, or current fields alter this static count; the mode-0 reset path leaves live reset state unknown.

Typed review records four Dreggan resets of Alatorin lily #83284, the active four-flower exchange returning #83285, a 2% carried lily on Mishanen, the hourly Cronus Flower swap in #41059, and Dreggan key #41003 used by the Shadamehr vault door. Shadamehr also loads Dreggan’s dark-heart object #67219; its `O #41002` reset has no matching area-listed object prototype. Active Brass adventuring notes give a route through Dreggan into Shadamehr Keep. Dreggan has no local QST, SHP, story sidecar, or native Q/QA contract. “Dreggan Woods” is unique among 442 current normalized titles; followed same-stem history establishes no replacement.

The all-95 inactive-WLD scan found no room VNUM overlap or source-only WLD approach into the area. Coverage maps 254 of 351 active contributors, leaving 97 unmapped. Case-insensitive PENDING counts are 200 narrative, 382 connectivity, 194 title/history, and 366 typed-dependency rows. Curated source-only totals remain 3,460 overall, 221 for Surf, and 17 for ud_map2; Underworld2 retains eight approaches. No candidate row or curated count changes. Continue with `end`, the next unmapped active contributor. See [coverage row 81](ZONE_COVERAGE_REGISTER.csv#L81), the [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-214-dreggan-wood-reset-aware-access), the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-214-dreggan-woods-title-and-source-history), and the [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-214-dreggan-wood-and-source-only-retention).

### Repository-wide audit checkpoint 215: end terminal marker

Added the [End of the World source dossier](../zone-stories/THE_END_OF_THE_WORLD.md) and [audit supplement](END_OF_THE_WORLD_AUDIT_REVIEW.md); the detailed story-review set is now 48. The active `end` contributor is a terminal build marker: source and loader review confirms no playable WLD room or active local MOB/OBJ/QST record, so the coverage inventory's former one-record sentinel counts are corrected to zero.

The active `end.shp` tail contains four shared dealer records at rooms #36329, #37718, #97685, and #132767. Each record uses Heavens mobile #101, matches one reset in Llzazan, Nax, Shady, or Tharnadia, and sells active Heavens objects #829–#838. Keep this cross-area catalog at the end of the shop build order. The source dossier maps these as the meaningful End-stem content; current keeper resets and shop operation are not qualified.

“The End of the World” is unique among 442 current normalized zone titles, and followed same-stem history shows no predecessor or replacement. The all-95 inactive-WLD scan found no approach to a playable End room because no such room exists. No candidate row or curated source-only count changes.

Coverage maps 255 of 351 active contributors, leaving 96 unmapped. Case-insensitive PENDING counts are 199 narrative, 381 connectivity, 193 title/history, and 366 typed-dependency rows. Curated source-only totals remain 3,460 overall, 221 for Surf, and 17 for ud_map2; Underworld2 retains eight approaches. Continue with `eth2`, the next narrative-pending contributor. See [coverage row 89](ZONE_COVERAGE_REGISTER.csv#L89), the [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-215-end-terminal-marker-and-connectivity), the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-215-the-end-of-the-world-title-and-source-history), and the [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-215-end-terminal-marker-and-shared-shop-catalog).

### Repository-wide audit checkpoint 216: eth2

Added the [Fortress of Dreams source dossier](../zone-stories/THE_FORTRESS_OF_DREAMS.md) and [audit supplement](ETH2_AUDIT_REVIEW.md); the detailed story-review set is now 49. The area has 111 rooms, 45 mobiles, 42 objects, and 158 non-S reset commands. All 383 exits resolve locally in three components of 57, 28, and 26 rooms. Its only active WLD arrival is Realm #14209 north to #32707; no local room has an outbound WLD boundary. Raw directed reach from #32707 is 12/111, and applying all 30 D resets as unavailable leaves 1/111. The six source-declared type-25 targets connect the map components and Ethereal Main, but runtime item presence is unqualified.

Typed review records Alatorin's QST/story and 2% carried-object reset reference to ethereal rose #32627; local use of Heavens object #360; Chaos kit use of #32624, #32628, and #32630; the external Ethereal Main portal #12410; and compiled mobile/object VNUM handlers. The cathedral-to-bell-tower trapdoor has a key-field mismatch: #32461 resolves to a BCTDL mirror with no reviewed reset placement; #20541 has no current object prototype. No duplicate prototype, outside shop product, local QST source, or story sidecar was found.

The normalized title is unique among 442 current ZON titles. Followed WLD/MOB/OBJ/ZON history remains on the eth2 stem, with no alternate-stem predecessor or whole-zone replacement established. The all-95 inactive-WLD sweep found no room overlap or approach into eth2. No candidate row or curated source-only count changes. Coverage maps 256 of 351 active contributors, leaving 95 unmapped. Case-insensitive PENDING counts are 198 narrative, 381 connectivity, 192 title/history, and 365 typed-dependency rows. Curated source-only totals remain 3,460 overall, 221 for Surf, and 17 for ud_map2; Underworld2 retains eight approaches. Continue with ethereal_main. See [coverage row 91](ZONE_COVERAGE_REGISTER.csv#L91), the [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-216-eth2-reset-aware-access), the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-216-the-fortress-of-dreams-title-and-source-history), and the [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-216-eth2-and-source-only-retention).


### Repository-wide source/story review checkpoint 217: The Ethereal Plane

Added the [Ethereal Plane source dossier](../zone-stories/THE_ETHEREAL_PLANE.md) and [audit supplement](ETHEREAL_MAIN_AUDIT_REVIEW.md); the detailed story-review set is now 50. The active area has 150 rooms, eight mobiles, 32 objects, and 90 non-S reset commands. Its 900 WLD exits resolve: 898 are local and two leave the zone. Seven active WLD exits enter from Dream, Dreggan Woods, and Prison. All three unique active WLD arrival rooms reach and return from the full 150-room graph. The all-95 inactive-WLD sweep found no room overlap or approach.

Typed review records ten active object routes into Ethereal rooms, local pool and portal departures to Astral, the elemental planes, Celestia, and eth2, generic plane-travel support, Heavens QST references to objects #12401 and #12405, and Ravenloft's reset of mobile #12401. Menzellon's helper list references seven local Ethereal mobiles. The local zone imports Celestia and Heavens objects and Winterhaven's Menzellon essence; its reset for object #18202 has no object prototype, while that VNUM belongs to a Wolfen room. No active SHP product or story sidecar references a local object or mobile.

The normalized title is unique among 442 current ZON titles. Followed history remains on ethereal_main and shows no predecessor or explicit replacement. Keep the active contributor DO NOT MOVE. No candidate row or curated source-only count changes. Coverage maps 257 of 351 active contributors, leaving 94 unmapped. Case-insensitive PENDING counts are 197 narrative, 381 connectivity, 191 title/history, and 364 typed-dependency rows. Curated source-only totals remain 3,460 overall, 221 for Surf, and 17 for ud_map2; Underworld2 retains eight approaches. Continue with flind. See [coverage row 92](ZONE_COVERAGE_REGISTER.csv#L92), the [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-217-ethereal-main-reset-and-transport), the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-217-the-ethereal-plane-title-and-source-history), and the [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-217-ethereal-main-and-source-only-retention).

### Repository-wide source/story review checkpoint 218: Flind

Added the [Flind source dossier](../zone-stories/FLIND.md) and [audit supplement](FLIND_AUDIT_REVIEW.md). The active contributor has 89 rooms, 34 mobiles, 41 objects, one shop record, and 125 non-S reset commands. Its 183/183 resolving WLD exits include one reciprocal boundary with active underdark; the local graph has four weak components. Six reset-loaded teleport objects connect the components when the authored WLD exits are available. A source-only ud_map2 exit approaches Flind #3475 but is not active ingress.

Typed review found the local shop’s five Flind products, the key and sentinel at the golden passageway, two compiled target tables containing room #3404, and no outside active reset, QST item, shop product, or ITEM_TELEPORT target pointing to local Flind prototypes. The title is unique among 442 current titles; followed same-stem history shows no predecessor or replacement. Keep the active contributor DO NOT MOVE.

Coverage maps 258 of 351 active contributors, leaving 93 unmapped; the detailed story-review set is 51. Case-insensitive PENDING counts are 196 narrative, 380 connectivity, 190 title/history, and 363 typed-dependency rows. Curated source-only totals are 3,461 overall, 221 for Surf, and 18 for ud_map2; Underworld2 retains eight approaches. Continue with glacier. See [coverage row 99](ZONE_COVERAGE_REGISTER.csv#L99), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-218-flind-reset-and-teleport-reach), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-218-the-minor-plane-of-planitude-title-and-history), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-218-flind-and-source-only-retention).

### Repository-wide source/story review checkpoint 219: Glacier

Added source-comprehensive documentation for active glacier. The coverage row records 105 rooms, ten mobiles, nine objects, no local QST source or SHP file, 257 WLD exits, one one-way Surface departure, one negative target, and a single 105-room weak component. The full room/mobile/reset narrative review identifies Princess Falina in #90505 and key #90402 on King Polar; no quest objective or outcome is authored locally. The title is unique among 442 current titles and followed history remains on the glacier stem. Keep Glacier DO NOT MOVE.

The inactive-WLD sweep adds one source-only surf.wld approach, #208853 north to #90401, with no room overlap; it is not active ingress. Coverage maps 259/351 contributors with 92 unmapped. Case-insensitive PENDING counts are 195 narrative, 379 connectivity, 189 title/history, and 362 typed-dependency rows. Curated source-only totals are 3,462 overall, 222 for Surf, and 18 for ud_map2; Underworld2 retains eight approaches. Continue with goblincamp. See [coverage row 108](ZONE_COVERAGE_REGISTER.csv#L108), [Glacier audit supplement](GLACIER_AUDIT_REVIEW.md), and [the next checkpoint](HANDOFF.md).

### Repository-wide source/story review checkpoint 220: Goblin Camp

Added source-comprehensive documentation for active goblincamp. The coverage row records 21 rooms, nine mobiles, four objects, no local QST source or SHP file, 65 resolving WLD exits, a reciprocal Surface boundary, and a separate one-way inbound from wh to the camp center. Both unique active arrivals can reach and return across the 21-room local graph. No authored door resets, falls, or currents were found. The room and mobile narrative follows a timber-gathering group from Malch’hor Ganl as it settles into and damages the surface forest. The normalized title is unique and followed history remains on the goblincamp stem. Keep Goblin Camp DO NOT MOVE.

The inactive-WLD sweep adds one non-overlapping source-only surf.wld approach, #264548 north to #11400; it is not active ingress. Coverage maps 260/351 contributors with 91 unmapped. Case-insensitive PENDING counts are 194 narrative, 378 connectivity, 188 title/history, and 361 typed-dependency rows. Curated source-only totals are 3,463 overall, 223 for Surf, and 18 for ud_map2; Underworld2 retains eight approaches. Continue with grave. See [coverage row 109](ZONE_COVERAGE_REGISTER.csv#L109), [Goblin Camp audit supplement](GOBLINCAMP_AUDIT_REVIEW.md), and the [current handoff](HANDOFF.md).

### Repository-wide source/story review checkpoint 221: The Old Graveyard

Added source-comprehensive documentation for active `grave`. The coverage row records 204 rooms, 28 mobiles, 15 objects, no local QST source or SHP file, 480 resolving exits, two reciprocal Surface boundaries, and one local weak component. Reset-aware static reachability is 3/204 from #8400 and 149/204 from #8427 with all reset-door edges unavailable. The narrative maps the Fryntok tragedy, Ogrebane, petrified oak, and necromancer’s laboratory; no local quest outcome is authored. The normalized title is unique and followed history stays on the `grave` stem. Keep Grave DO NOT MOVE.

The inactive-WLD sweep adds two source-only Surf approaches and two `underdark2011` approaches, all non-overlapping and not active ingress. A registered Phantom `shift_prime` list also contains Grave rooms #8486 and #8430. Coverage maps 261/351 contributors with 90 unmapped. Case-insensitive PENDING counts are 193 narrative, 377 connectivity, 187 title/history, and 360 typed-dependency rows. Curated source-only totals are 3,467 overall, 225 for Surf, and 18 for ud_map2; Underworld2 retains eight approaches. Continue with `graves`, “The Crypts of Rays.” See [coverage row 113](ZONE_COVERAGE_REGISTER.csv#L113), [Grave audit supplement](GRAVE_AUDIT_REVIEW.md), and [the current handoff](HANDOFF.md).


### Repository-wide source/story review checkpoint 222: The Crypts of Rays

Added the [Graves source dossier](../zone-stories/THE_CRYPTS_OF_RAYS.md) and [audit supplement](GRAVES_AUDIT_REVIEW.md), and backfilled explicit builder questions and recommended resolutions into the preceding Grave dossier. The active contributor has 25 room templates, 23 mobile prototypes, 16 objects, reset mode 0, and no local reset commands or QST outcome. Its static WLD records have no exits. The source generator can create connected dynamic copies, but automatic boot generation is excluded by the checked-in macro definition; a trusted staff command remains, and no generated instance was inspected. The current selector can request index 10 from a ten-column template array.

Typed review records 15 active external ZON potion resets, ten from Alatorin and five from Verspin, plus the shared random-zone template, entrance-procedure, and sigil references. No active or inactive WLD approach or other typed external dependency was found. The normalized title is unique among 442 current titles; followed same-stem history establishes no replacement. Keep graves DO NOT MOVE.

The coverage CSV now has a separate builder-question review column. Grave and Graves are marked complete in this checkpoint; 474 of 476 registered rows still need that explicit section verified or added. The detailed source/story-review set is now 55. Coverage maps 262 of 351 active contributors, leaving 89 unmapped. Case-insensitive PENDING counts are 192 narrative, 376 connectivity, 186 title/history, and 359 typed-dependency rows. Curated source-only totals remain 3,467 overall, 225 for Surf, and 18 for ud_map2; Underworld2 retains eight approaches. Continue with guildhalls. See [coverage row 114](ZONE_COVERAGE_REGISTER.csv#L114), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-222-graves-template-connectivity), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-222-the-crypts-of-rays-title-and-source-history), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-222-graves-and-source-only-retention).

### Repository-wide source/story review checkpoint 223: Guildhalls

Added the [Guildhalls source dossier](../zone-stories/GUILDHALLS.md) and [repository-wide audit supplement](GUILDHALLS_AUDIT_REVIEW.md), and completed the third explicit builder-question/recommendation section. The contributor has 1,000 room prototypes, three mobiles, and 49 objects; its 15 named room templates frame a persistent, customizable guild system, with four detailed workshop and store descriptions. No local QST, SHP, sidecar, or Q/QA outcome exists.

Static connectivity is 0/0 WLD exits, 1,000 singleton components, and no inbound target or source-room overlap in the other 445 active or inactive WLD files. The saved room graph is rebuilt from persistence and source code; no live guildhall records were inspected. The exact typed dependency scan found the active Brass reset of object #48012 and an inactive matching reset in brass-old-1; the latter is recorded in its existing candidate row. Code directly binds the hall templates, board objects, golems, kingdom workshops/store, and travel system. Keep guildhalls DO NOT MOVE.

The story-review set is 56. Coverage maps 263 of 351 active contributors, leaving 88 unmapped. Case-insensitive PENDING counts are 191 narrative, 375 connectivity, 185 title/history, and 358 typed-dependency rows. The builder-question column has three complete entries and 473 pending. Curated source-only WLD totals remain 3,467 overall, 225 for Surf, and 18 for ud_map2; Underworld2 retains eight approaches. Continue with hell_four_phlegethos, Hell, Phlegethos. See [coverage row 115](ZONE_COVERAGE_REGISTER.csv#L115), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-223-guildhalls-dynamic-room-pool-connectivity), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-223-guildhalls-title-and-source-history), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-223-guildhalls-retention-and-brass-imports).

Documentation validation checked the coverage and candidate CSV records, dossier/source link targets, local Markdown links, and git diff --check. areas/ and src/ remain unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.
### Repository-wide source/story review checkpoint 224: Hell, Phlegethos

Added the [Hell, Phlegethos source dossier](../zone-stories/HELL_FOUR_PHLEGETHOS.md) and [audit supplement](HELL_FOUR_PHLEGETHOS_AUDIT_REVIEW.md), with explicit builder questions and recommended resolutions. All 400 rooms reuse the same volcanic environmental description. The local mobile and object are placeholders, the ZON has no reset commands, and no local QST, SHP, story sidecar, or Q/QA outcome exists.

Static connectivity is one weak component of 400 with 1,600 internal exits, but directed reach and return from the analysis seed #52200 covers 396. Four rooms are outside that directed component and 86 exit records lack a reverse-direction edge. No other active or inactive WLD file overlaps or targets the range. The Bronze Citadel portal prototype is not reset-loaded and has an unresolved 99999 destination. Keep the active area DO NOT MOVE; it has no attic-candidate row.

The detailed story-review set is 57. Coverage maps 264 of 351 active contributors, leaving 87 unmapped. Case-insensitive PENDING counts are 190 narrative, 375 connectivity, 184 title/history, and 357 typed-dependency rows. Four builder-question sections are complete and 472 remain. Curated source-only WLD totals remain 3,467 overall, 225 for Surf, and 18 for ud_map2; Underworld2 retains eight approaches. Continue with hell_one_avernus, Hell, Avernus. See [coverage row 122](ZONE_COVERAGE_REGISTER.csv#L122), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-224-hell-phlegethos-directed-grid-and-ingress), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-224-hell-phlegethos-title-and-source-history), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-224-hell-phlegethos-retention-and-access-ambiguity).

Documentation validation checked both registers, the source and report links, local Markdown links, and whitespace. areas/ and src/ remain unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.


### Repository-wide source/story review checkpoint 225: Hell One, Avernus

Added the [Hell One source dossier](../zone-stories/HELL_ONE_AVERNUS.md) and [audit supplement](HELL_ONE_AVERNUS_AUDIT_REVIEW.md). The active contributor has 400 rooms, two duplicate swarm-devil mobile prototypes, and six objects; the same hostile environmental paragraph repeats across the room range. It has no local QST file, shop, story sidecar, or native Q/QA contract. Its 209 local ZON commands import River Styx mobiles and establish the swarm-devil, poison-stinger, and Hellspawn staff group.

Static connectivity review found a strongly connected local graph of all 400 rooms, seven active inbound WLD edges, and one outbound edge to Tiamat. All rooms can reach the outbound room; none of the inbound or outbound boundary records has a direct reciprocal cross-zone edge. Live player access, global return, reset-time availability, and safety remain unverified.

Typed closure records 18 active external reset rows using Hell One mobile/object prototypes, its imports of nine River Styx mobile prototypes, and direct code bindings for the potion, Death Rider sword, and Tiamat stinger. The title is unique; tracked same-stem history shows no explicit predecessor or replacement. Keep active hell_one_avernus DO NOT MOVE. No attic candidate row or migration plan was added.

The story-review set is 58. Coverage maps 265 of 351 active contributors, leaving 86 unmapped. Case-insensitive PENDING counts are 189 narrative, 375 connectivity, 183 title/history, and 356 typed-dependency rows. Five builder-question sections are complete and 471 remain. Curated source-only totals remain 3,467 overall, 225 for Surf, and 18 for ud_map2; Underworld2 retains eight approaches. See [coverage row 123](ZONE_COVERAGE_REGISTER.csv#L123), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-225-hell-one-avernus-static-graph-and-boundaries), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-225-hell-one-title-and-source-history), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-225-hell-one-retention-and-shared-dependencies).


### Repository-wide source/story review checkpoint 226: Hell, Minauros

Added the [Minauros source dossier](../zone-stories/HELL_THREE_MINAUROS.md) and [audit supplement](HELL_THREE_MINAUROS_AUDIT_REVIEW.md). The 400-room environmental story repeats one paragraph about rain, hail, fog, dead creatures, and Mammon's black fortress. All rooms are marked SECT_INSIDE; boot code adds ROOM_NO_PRECIP. Three hundred are ROOM_DARK, while #52000–#52099 are not. The one mobile and one object are placeholders; the zone has no reset commands or local quest contract.

The raw room graph is one weak component of 400 with 1,600 exits; directed traversal from analysis seed #51800 reaches 396 rooms. Four rooms (#51911–#51913 and #52199) are outside that seed's reach. Hell Two and Minauros both define room #51800. The cross-area register's four-in/seven-out figures are owner-classified edges that all touch that collision. Three active Hell Two exits also target #51800, while no other active source targets a unique Minauros room and no inactive/source-only approach exists. The effective runtime owner, player entrance, and safe return remain unresolved.

Typed reset and code scans found no external ZON, QST, SHP, story-sidecar, or direct C/H reference to the Minauros mobile, object, or room range. The normalized title is unique. Tracked histories preserve both distinct Hell Two and Minauros WLD records from the 2009 path move; no whole-zone replacement is shown. Keep active hell_three_minauros DO NOT MOVE. Leave connectivity and duplicate/history pending until the owner resolves #51800. No attic candidate row was added.

The detailed source-story set is 59. Coverage maps 266 of 351 active contributors, leaving 85 unmapped. Case-insensitive PENDING counts are 188 narrative, 375 connectivity, 183 title/history, and 355 typed-dependency rows. Six builder-question sections are complete and 470 remain. Curated source-only totals remain 3,467 overall, 225 for Surf, and 18 for ud_map2; Underworld2 retains eight approaches. See [coverage row 124](ZONE_COVERAGE_REGISTER.csv#L124), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-226-hell-three-minauros-owner-collision-and-room-graph), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-226-hell-three-title-history-and-room-vnum-collision), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-226-hell-three-retention-with-overlapping-room-vnum).


### Checkpoint 227: Hell Two, Dis

The [Hell Two source dossier](../zone-stories/HELL_TWO_DIS.md) and [audit supplement](HELL_TWO_DIS_AUDIT_REVIEW.md) cover its repeated environmental prose, room flags, Dispator reset, rod prototype, quest boundary, collision with Minauros at #51800, history, and typed dependencies. Static closure found active `4horse.zon` loading the rod in room #34804; `specs.assign.c` binds the object to the `dispator` procedure. No other external typed consumer was found. Connectivity and duplicate/replacement remain pending because all seven-in/four-out classified boundaries and the three additional Hell Two exits target the ambiguous #51800.

Keep `hell_two_dis` DO NOT MOVE. No attic candidate row is justified. See [coverage row 125](ZONE_COVERAGE_REGISTER.csv#L125), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-227-hell-two-dis-graph-and-shared-room-boundary), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-227-hell-two-title-history-and-shared-room-vnum), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-227-hell-two-retention-and-external-rod-dependency).


### Checkpoint 228: The Behemoth Herders

The [comprehensive Herders dossier](../zone-stories/THE_BEHEMOTH_HERDERS.md) and [Checkpoint 55 audit supplement](HERDERS_AUDIT_REVIEW.md) already cover source story, all native contracts, exact reset and item custody, access graph, hazards, external QST/SHP/ZON/story/code dependencies, title history, and active/source-only approaches. Checkpoint 228 adds nine explicit source-backed builder questions and recommended resolutions to the dossier. The coverage row is complete for builder-question review; implementation capability plans remain separate and no native gameplay was modified.

Keep active `herders` DO NOT MOVE. Its current source-only approaches do not establish active ingress and do not make it an attic candidate. See [coverage row 126](ZONE_COVERAGE_REGISTER.csv#L126), [connectivity](WORLD_CONNECTIVITY.md#the-behemoth-herders-and-four-source-only-approaches), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-228-herders-title-and-tracked-history), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-228-herders-retention-and-source-only-approaches).


### Checkpoint 229: Highmoor

The [Highmoor dossier](../zone-stories/THE_HIGH_MOOR_FOREST.md) and [audit supplement](HIGHMOOR_AUDIT_REVIEW.md) cover its full local story corpus, reset inventory, absent Q/QA contract, route graph, hazards, title history, and typed references. The Blueleaf tree and Yingin texts connect Goldrym to worsening forest conditions; Dr. Fester and Kirus add distinct research-related tragedies. Four rooms remain explicit proposed-zone stubs.

Highmoor has 258 rooms and 731 authored exits. Its active Surface pair is #19100/#526069; source-only Surf room #209922 also targets #19100. From #19100, 252 rooms are directionally reachable and 255 have authored returns. Three negative exits at #19327 are dropped. Forty-eight currents and two 40% waterfall fall fields remain source facts pending route and hazard review. An active Ixarkon code destination can select attic #19275, and Dr. Fester #19141 teaches Anatomy through the compiled skill table.

One source-only Brass-old-1 reset references object #19127. Keep Highmoor DO NOT MOVE and preserve both candidate dependencies in Surf and Brass-old-1’s existing rows. The title is unique among 442 current titles and same-stem history shows no explicit replacement. The detailed source-story set is 61. Coverage maps 267/351 active contributors, with 84 unmapped; case-insensitive PENDING counts are 186 narrative, 374 connectivity, 182 history, 353 typed dependencies, and 467 builder-question reviews. Nine builder-question sections are complete. Curated source-only totals are 3,468 overall, 226 for Surf, 18 for ud_map2, and eight for Underworld2. Continue with `highway`. See [coverage row 127](ZONE_COVERAGE_REGISTER.csv#L127), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-229-highmoor-directed-graph-and-hidden-arrivals), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-229-highmoor-title-and-tracked-history), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-229-highmoor-retention-and-cross-source-references).

### Checkpoint 230: Southern Coastal Highway review reconciliation

The existing source-comprehensive [Highway dossier](../zone-stories/SOUTHERN_COASTAL_HIGHWAY.md) now has eight source-backed builder questions and recommended resolutions. They cover the negative down exit at #41488, two source-only Surf approaches, the computed Githzerai destination #41767, horse-merchant intent, distinct switch verbs, the scrying organ's availability, trap data, and the wonder wand's source-item flag. Its local story sidecar represents three native returns; Victor's Bastine exchange and the six Bastine royal requests remain foreign ownership, not added Highway contracts.

Connectivity remains as reviewed: 539 rooms, 1,411 exits, 1,410 active-room resolutions, two weak components, and five reciprocal active pairs. The two inactive Surf approaches are not active ingress. Highway's active reset and compiled references to Mir prototypes #41915 and #41917, web/fire objects, and mobile helpers are recorded in the updated [audit supplement](SOUTHERN_COASTAL_HIGHWAY_AUDIT_REVIEW.md). Keep Highway **DO NOT MOVE** and add no attic candidate. Its title is unique among 442 current titles and bounded same-stem history shows no replacement.

The detailed source-story set remains 61. Coverage maps 267/351 active contributors, leaving 84 unmapped; case-insensitive PENDING counts remain 186 narrative, 374 connectivity, 182 history, and 353 typed dependencies, with 466 builder-question reviews pending. Ten builder-question sections are complete. Curated source-only totals remain 3,468 overall, 226 for Surf, 18 for ud_map2, and eight for Underworld2. Continue with `hunt`, The Caverns of Armageddon. See [coverage row 128](ZONE_COVERAGE_REGISTER.csv#L128), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-230-highway-boundaries-surf-approaches-and-computed-arrivals), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-230-southern-coastal-highway-title-and-tracked-history), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-230-highway-retention-and-mir-references).

### Checkpoint 231: The Caverns of Armageddon review reconciliation

The existing source-comprehensive [Hunt dossier](../zone-stories/CAVERNS_OF_ARMAGEDDON.md) now includes eleven explicit builder questions and recommended resolutions. The [audit supplement](HUNT_AUDIT_REVIEW.md) closes the static connectivity, title/history and typed-dependency reviews. It verifies 149 rooms and 320 exits, the 134-room forward and return WLD sets from #13307, two inbound and two outbound Surface edges with one reciprocal pair, the local portals and rescue ladder, and the active reset/QST/code dependencies. The all-95 inactive-WLD scan found no source-only approach or room overlap.

Keep `hunt` DO NOT MOVE and add no attic candidate. The normalized title is unique among 442 current titles, and tracked same-stem history shows no predecessor or explicit replacement. Preserve active Myrabolus, Cosmic, Desolate Invaders and Vehicles references. The detailed source-story set remains 61; coverage maps 267/351 contributors, leaving 84 unmapped. Case-insensitive PENDING counts are 186 narrative, 373 connectivity, 181 history, and 352 typed dependencies, with 465 builder-question reviews pending. Eleven builder-question sections are complete. Curated source-only totals remain 3,468 overall, 226 for Surf, 18 for ud_map2, and eight for Underworld2. Continue with `hwood`, The Haunted Woods. See [coverage row 129](ZONE_COVERAGE_REGISTER.csv#L129), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-231-hunt-directed-graph-and-boundaries), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-231-hunt-title-and-tracked-history), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-231-hunt-retention-and-cross-source-dependencies).

### Checkpoint 232: The Haunted Woods review reconciliation

Added the source-comprehensive [Haunted Woods dossier](../zone-stories/THE_HAUNTED_WOODS.md) and [audit supplement](HWOOD_AUDIT_REVIEW.md), with eleven builder questions. The source set has 58 rooms, 23 mobiles, 19 objects, two SHP records, and no local QST file or story sidecar. Room #10857 has a compiled `inn` binding. Static topology is complete: 157 exits, a reciprocal Surface pair, and the one-way-to-forest Mob Load Room #10835 outside forward reach from #10800.

The all-95 inactive-WLD scan found Surf room #209648 north to Hwood #10800. Preserve it in existing Surf candidate row 97 as source-only; it does not prove active ingress or justify a Hwood candidate. Typed closure also preserves active Heavens medical prototypes #363/#368/#369 in the general shop. Keep Hwood DO NOT MOVE. Its unique normalized title and same-stem history establish no explicit replacement.

The detailed source-story set is 62. Coverage maps 268/351 active contributors, leaving 83 unmapped. Case-insensitive PENDING counts are 185 narrative, 372 connectivity, 180 history, and 351 typed dependencies, with 464 builder-question reviews pending. Twelve builder-question sections are complete. Curated source-only totals are 3,469 overall, 227 for Surf, 18 for ud_map2, and eight for Underworld2. Continue with `icecrag`, IceCrag Castle. See [coverage row 130](ZONE_COVERAGE_REGISTER.csv#L130), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-232-hwood-directed-graph-and-surf-approach), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-232-hwood-title-and-tracked-history), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-232-hwood-retention-and-surf-reference).

### Checkpoint 233: IceCrag review reconciliation

The existing source-comprehensive [IceCrag dossier](../zone-stories/ICECRAG_CASTLE.md) now includes eleven builder questions with recommended resolutions. The [audit supplement](ICECRAG_AUDIT_REVIEW.md) closes the bounded static graph, direct typed-reference, title/history, and semantic comparison reviews. It records 243 rooms, 628 exits with 627 resolving, 220/243 forward reach and 239/243 authored returns from #97001, both reciprocal active pairs, the one-way Winterhaven and Lava Tubes records, the unresolved #97352 exit, and the two inactive-map approaches already retained in candidate rows.

The normalized title is unique among 442 current titles, and tracked same-stem history identifies no named predecessor or replacement. `Voluntown` and its Royal Mausoleum remain a separate connected contributor. Static typed closure retains the active Alatorin, Barovia2, Ravenloft, Air, Sarmiz, and chaos-table references, plus inactive Brass-old-1 and the local portal pair. Keep `icecrag` DO NOT MOVE; no candidate row is warranted.

The detailed source-story set remains 62. Coverage maps 268/351 active contributors, leaving 83 unmapped. Case-insensitive PENDING counts are 185 narrative, 371 connectivity, 179 title/history, 350 typed-dependency, and 463 builder-question rows. Thirteen builder-question sections are complete. Curated source-only totals remain 3,469 overall, 227 for Surf, 18 for `ud_map2`, and eight for Underworld2. Continue with `icetower`, Ice Tower. See [coverage row 131](ZONE_COVERAGE_REGISTER.csv#L131), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-233-icecrag-directed-graph-and-boundaries), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-233-icecrag-title-history-and-related-contributor), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-233-icecrag-retention-and-typed-dependencies).

### Checkpoint 234: Ice Tower builder review reconciliation

The existing source-comprehensive [Ice Tower dossier](../zone-stories/ICE_TOWER.md) now includes eleven builder questions with recommended resolutions. The earlier [audit supplement](ICE_TOWER_AUDIT_REVIEW.md) already completes the static graph, reciprocal Surface boundary, source-only Surf approach, bounded imported-dependency review, and same-stem title/history findings. The added questions make the remaining choices explicit while preserving current native item contracts, route rules, imported-object owners and active-area disposition.

Keep active `icetower` DO NOT MOVE and retain the source-only `surf.wld` approach on Surf's existing candidate row. Coverage row 132's narrative and builder-question reviews are complete; world-connectivity, title/history and bounded typed-dependency reviews remain as documented in Checkpoint 118.

The detailed source-story set remains 62. Coverage maps 268/351 active contributors, leaving 83 unmapped. Case-insensitive PENDING counts are 185 narrative, 371 connectivity, 179 title/history, 350 typed-dependency, and 462 builder-question rows. Fourteen builder-question sections are complete. Curated source-only totals remain 3,469 overall, 227 for Surf, 18 for `ud_map2`, and eight for Underworld2. Continue with `illithid_arena`, Arena of the Elder Brain. See [coverage row 132](ZONE_COVERAGE_REGISTER.csv#L132), [Ice Tower dossier](../zone-stories/ICE_TOWER.md), and [audit supplement](ICE_TOWER_AUDIT_REVIEW.md).

### Checkpoint 235: Illithid Arena source-story reconciliation

Added the source-comprehensive [Illithid Arena dossier](../zone-stories/ILLITHID_ARENA.md) and [audit supplement](ILLITHID_ARENA_AUDIT_REVIEW.md), including ten builder questions with recommended resolutions. The 18-room WLD has 40 local resolved exits, one weak component, and SCC sizes 16/1/1. From #11200, 16 rooms are reachable and all 18 return. The two named combatant rooms have only one-way exits back to center; the hidden pit route resets closed and secret. There are no active boundaries, source-only approaches, room overlaps, or attic candidates.

Typed closure records the `squid_arena` WILL refusal bound to all 18 rooms, two guard-code references to #11208 with no matching local guard or south/west exit, and no checked-in external typed reset/QST/SHP/story/type-25 reference. The normalized title and same-stem history identify no predecessor or replacement; related CTF Tournament, Ixarkon, and Ixxillikor sources remain distinct. Keep active `illithid_arena` DO NOT MOVE.

The detailed source-story set is 63. Coverage maps 269/351 active contributors, leaving 82 unmapped. Case-insensitive PENDING counts are 184 narrative, 370 connectivity, 178 title/history, 349 typed-dependency, and 461 builder-question rows. Fifteen builder-question sections are complete. Curated source-only totals remain 3,469 overall, 227 for Surf, 18 for `ud_map2`, and eight for Underworld2. Continue with `interspa`, Interspace. See [coverage row 133](ZONE_COVERAGE_REGISTER.csv#L133), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-235-illithid-arena-directed-graph-and-boundary), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-235-elder-brain-arena-title-and-history), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-235-illithid-arena-retention-and-compiled-handler).


### Checkpoint 236: Interspace source-story reconciliation

Added the source-comprehensive [Interspace dossier](../zone-stories/INTERSPACE.md) and [audit supplement](INTERSPACE_AUDIT_REVIEW.md), including twelve builder questions and recommendations. Its 100 ethereal rooms have 601 authored exits, one strongly connected local component, and a one-way outbound WLD edge to Astral Main. Active and non-active WLD scans found no inbound edge or room overlap.

Bounded typed review records the two local M resets of Limbo mobile #2, an unreferenced local mist mobile, empty local OBJ data, and the external Jander reward item #59310 whose destination value is #99999. Its activation value does not establish a usable route. The normalized title is unique, and same-stem history shows no predecessor or replacement. Keep active `interspa` DO NOT MOVE; no attic candidate is warranted.

The detailed source-story set is 64. Coverage maps 270/351 active contributors, leaving 81 unmapped. Case-insensitive PENDING counts are 183 narrative, 369 connectivity, 177 title/history, 348 typed-dependency, and 460 builder-question rows. Sixteen builder-question sections are complete. Curated source-only totals remain 3,469 overall, 227 for Surf, 18 for `ud_map2`, and eight for Underworld2. Continue with `ixarkon`. See [coverage row 134](ZONE_COVERAGE_REGISTER.csv#L134), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-236-interspace-grid-and-astral-boundary), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-236-interspace-title-and-tracked-history), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-236-interspace-retention-and-cross-area-references).

### Checkpoint 237: Ixarkon

Ixarkon's existing dossier now has thirteen explicit builder questions. Its 201-room active WLD has 468 resolving exits; the local graph has a 199-room strongly connected core and singleton veil-chamber and pet-shop-backend rooms. All four unique active inbound arrival rooms reach and return through the 199-room core. Five inbound records and four reverse-paired outbound records remain mapped. The two `ud_map2.wld` records into #96492 remain source-only on the existing candidate record.

The title/history review records the Faang connector and finds no tracked Pillithid-named area or distinct predecessor/replacement. Keep active Ixarkon DO NOT MOVE, with no Ixarkon candidate row. Preserve the local custom travel, service bindings, and typed dependencies. The detailed source-story set remains 64; coverage maps 270/351 active contributors, leaving 81 unmapped. PENDING counts are 183 narrative, 369 connectivity, 176 title/history, 348 typed-dependency, and 459 builder-question rows. Seventeen builder-question sections are complete. Curated source-only totals remain 3,469 overall, 227 Surf, 18 `ud_map2`, and eight Underworld2. Continue with `ixxillikor`. See [coverage row 135](ZONE_COVERAGE_REGISTER.csv#L135), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-237-ixarkon-directed-reachability-and-boundaries), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-237-ixarkon-title-and-semantic-boundary), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-237-ixarkon-retention-and-source-only-approaches).

### Checkpoint 238: Ixxillikor

The existing Ixxillikor dossier now asks thirteen explicit builder questions and recommends resolutions, with the original ZSQ findings retained as the evidence record. Its 199-room graph has one weak component and SCCs of 195/1/1/1/1. Each of three active arrivals reaches the 195-room core; all 199 rooms can return to all three. The brain/follower load rooms #4332–#4335 are outside forward WLD reach from those arrivals. All four active boundaries are reciprocal, and the inactive-WLD scan found no approach or room overlap.

Keep Ixxillikor DO NOT MOVE and add no candidate row. Preserve the local service and story contracts plus the imported Verz1 object #3097 and active Ixarkon mobile #96433 resets. Its normalized title is unique and followed history identifies no predecessor or replacement; related Elder Brain themes do not imply duplicate ownership. The detailed source-story set remains 64. Coverage maps 270/351 active contributors, leaving 81 unmapped. PENDING counts are 183 narrative, 369 connectivity, 176 title/history, 348 typed-dependency, and 458 builder-question rows. Eighteen builder-question sections are complete. Curated source-only totals remain 3,469 overall, 227 Surf, 18 `ud_map2`, and eight Underworld2. Continue with `jade`. See [coverage row 136](ZONE_COVERAGE_REGISTER.csv#L136), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-238-ixxillikor-directed-reachability-and-boundaries), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-238-ixxillikor-title-and-related-contributor-boundary), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-238-ixxillikor-retention-and-typed-imports).

### Checkpoint 239: The Jade Empire

The existing source-comprehensive [Jade dossier](../zone-stories/THE_JADE_EMPIRE.md) now contains seventeen explicit builder questions and recommended resolutions. Its [audit supplement](JADE_AUDIT_REVIEW.md) documents the 340-room directed graph, five active arrivals, four outbound edges, exact typed consumers/imports, same-stem history, and two new source-only Surf approaches. Keep `jade` DO NOT MOVE; retain those two exits on Surf's existing candidate row and add no Jade attic row.

The detailed source-story set remains 64. Coverage maps 270/351 active contributors, leaving 81 unmapped. PENDING counts are 183 narrative, 369 connectivity, 175 title/history, 347 typed-dependency, and 457 builder-question rows. Nineteen builder-question sections are complete. Curated source-only totals are 3,471 overall, 229 Surf, 18 `ud_map2`, and eight Underworld2. Continue with `jademini`, The Rice Fields. See [coverage row 137](ZONE_COVERAGE_REGISTER.csv#L137), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-239-jade-empire-directed-reachability-and-boundaries), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-239-jade-empire-title-history-and-related-contributors), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-239-jade-retention-and-typed-dependencies).

### Checkpoint 240: The Rice Fields builder review reconciliation

The existing [Rice Fields source dossier](../zone-stories/RICE_FIELDS.md) now has seventeen explicit builder questions and recommended resolutions, alongside its 46-row ZSQ owner and integration plan. The questions close the register's builder-review gap while keeping accepted native recipes, paid/service boundaries, rescue claims, source availability, archer behavior, key routes, type-25 movement, and four source-only approaches grounded in the prior [audit supplement](RICE_FIELDS_AUDIT_REVIEW.md). The static graph, typed dependencies, title/history review, and **DO NOT MOVE** disposition remain as recorded at Checkpoint 174; no native area or gameplay source changed.

The detailed source-story set remains 64. Coverage maps 270/351 active contributors, leaving 81 unmapped. PENDING counts are 183 narrative, 369 connectivity, 175 title/history, 347 typed-dependency, and 456 builder-question rows. Twenty builder-question sections are complete. Curated source-only totals remain 3,471 overall, 229 Surf, 18 `ud_map2`, and eight Underworld2. Continue with `jin`, Jindon the Deathwood Forest. See [coverage row 138](ZONE_COVERAGE_REGISTER.csv#L138), [connectivity review](WORLD_CONNECTIVITY.md#priority-214-checkpoint-174-the-rice-fields), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-174-the-rice-fields-title-and-history), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-174-rice-fields-boundaries-typed-dependencies-and-inactive-approaches).

### Checkpoint 241: Jindon the Deathwood Forest

The existing [Jindon dossier](../zone-stories/JINDON_DEATHWOOD.md) now asks seventeen explicit builder questions and recommends resolutions, mapped to its 44-item ZSQ follow-up matrix. The [audit supplement](JINDON_DEATHWOOD_AUDIT_REVIEW.md) expands the bounded typed scan across current mobile/object catalogs, QSTs, SHPs, reset roles, story sidecars, type-25 declarations, and direct compiled references. The title/history supplement compares related forest contributors and finds no duplicate owner, alternate-stem rename, predecessor, or replacement. Keep active `jin` DO NOT MOVE and retain the existing Surf/Nexus56 dependencies with their owners.

The detailed source-story set remains 64. Coverage maps 270/351 active contributors, leaving 81 unmapped. PENDING counts are 183 narrative, 369 connectivity, 174 title/history, 346 typed-dependency, and 455 builder-question rows. Twenty-one builder-question sections are complete. Curated source-only totals remain 3,471 overall, 229 Surf, 18 `ud_map2`, and eight Underworld2. Continue with `jotun`, Jotunheim. See [coverage row 139](ZONE_COVERAGE_REGISTER.csv#L139), [connectivity](WORLD_CONNECTIVITY.md#priority-199-checkpoint-159-jindon-the-deathwood-forest), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-241-jindon-title-history-and-semantic-comparison), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-159-jindon-source-only-approach-and-nexus56-dependency).

### Checkpoint 242: Jotunheim

The existing Jotunheim dossier now includes seventeen explicit builder questions and recommended resolutions. The [audit supplement](JOTUN_AUDIT_REVIEW.md#checkpoint-242-current-catalog-tool-and-semantic-revalidation) closes the bounded current-source catalogs, typed consumers/imports, checked-in code and test/tooling references, and semantic comparison. It finds no duplicate Jotun mobile/object owner, external SHP product, or outside type-25 target other than Astral Main's authored route to #96004. Fearfrost/Mundorno, Shabo, source-only Brass, current imported objects, code bindings, and 231 parsed outside sidecars remain documented. Storm Giants, Cloud Giant Kingdom, and unregistered Valhalla retain separate source identities. Persisted account/player records and live gameplay were not queried.

Keep `jotun` DO NOT MOVE; no candidate is added. Preserve its Astral object route, active Shabo resets, and source-only Brass reset dependencies with their existing owners. Static WLD findings are unchanged and actual reachability remains pending. No native area or gameplay source changed.

The detailed source-story set remains 64. Coverage maps 270/351 active contributors, leaving 81 unmapped. PENDING counts are 183 narrative, 369 connectivity, 173 title/history, 345 typed-dependency, and 454 builder-question rows. Twenty-two builder-question sections are complete. Curated source-only totals remain 3,471 overall, 229 for Surf, 18 `ud_map2`, and eight Underworld2. Continue with `juiblex`, The 222nd Layer of the Abyss. See [coverage row 140](ZONE_COVERAGE_REGISTER.csv#L140), [connectivity review](WORLD_CONNECTIVITY.md#jotunheims-negative-wld-target-and-typed-routes), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-242-jotunheim-title-history-and-related-source-identities), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-242-jotunheim-retention-and-bounded-source-closure).

### Checkpoint 243: Juiblex, The 222nd Layer of the Abyss

The existing [Juiblex dossier](../zone-stories/THE_222ND_LAYER_OF_THE_ABYSS.md) now contains twenty-one explicit builder questions and recommended resolutions. The [audit supplement](JUIBLEX_AUDIT_REVIEW.md) closes static WLD reachability and the bounded current-source typed, code/tooling, semantic-identity, and source-history review. It records no duplicate local prototype; no active outside reset or shop product; one active Winterhaven QST consumer of crystal #87510; five local imports from Heavens, Unique, and Winterhaven; one source-only Brass reset; and Ravenloft 2's outside type-25 target to #87528. Astral Tiamat's Juiblex form and the spelling/path history are distinct source identities and changes, not a replacement. Persisted player/account records and live gameplay were not queried.

Keep `juiblex` DO NOT MOVE and add no candidate. Preserve the active WLD boundaries, Ravenloft 2 portal, Winterhaven crystal dependency, five local imported objects, and the source-only Brass mandolin reset under their current owners. Static arrivals and the blocked phase transfer do not qualify live admission or safe return. No native area or gameplay source changed.

The detailed source-story set remains 64. Coverage maps 270/351 active contributors, leaving 81 unmapped. PENDING counts are 183 narrative, 369 connectivity, 172 title/history, 344 typed-dependency, and 453 builder-question rows. Twenty-three builder-question sections are complete. Curated source-only totals remain 3,471 overall, 229 for Surf, 18 for `ud_map2`, and eight for Underworld2. Continue with `kastle`, Varathorn Keep. See [coverage row 141](ZONE_COVERAGE_REGISTER.csv#L141), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-243-juiblex-directed-reachability-and-boundaries), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-243-juiblex-title-same-source-spelling-history-and-related-areas), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-243-juiblex-retention-and-typed-dependencies).

### Checkpoint 244: Kastle, Varathorn Keep builder reconciliation

The existing Varathorn Keep dossier now asks twenty explicit builder questions with recommended resolutions, mapped to its twenty-row ZSQ plan. The questions make the three independent returns, supplied proofs, secret-source history, optional switches and key, conditional fall, mode-2 encounter limits, captive lore, weapon dispatch conflict, external typed ownership, and two inactive Surf approaches explicit. The static graph, title/history review, typed dependency closure, and **DO NOT MOVE** disposition remain as recorded in the [Checkpoint 113 supplement](KASTLE_AUDIT_REVIEW.md). No native area or gameplay source changed.

Preserve Kastle's two active reciprocal Surface pairs and its two source-only Surf approaches on candidate row 97; no Kastle candidate is added. Keep shop/reset references and the Tharnadia quill exchange with their existing owners. Runtime source availability, weapon effects, hazards, and captive outcomes remain separate.

The detailed source-story set remains 64. Coverage maps 270/351 active contributors, leaving 81 unmapped. PENDING counts are 183 narrative, 369 connectivity, 172 title/history, 344 typed-dependency, and 452 builder-question rows. Twenty-four builder-question sections are complete. Curated source-only totals remain 3,471 overall, 229 for Surf, 18 for `ud_map2`, and eight for Underworld2. Continue with `kelek`, The Stone Tomb of Kelek. See [coverage row 142](ZONE_COVERAGE_REGISTER.csv#L142), [connectivity review](WORLD_CONNECTIVITY.md#varathorn-keep-two-reciprocal-surface-pairs-and-two-inactive-approaches), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#varathorn-keep), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-244-kastle-retention-after-builder-review).

### Checkpoint 245: The Stone Tomb of Kelek builder reconciliation

The existing [Kelek dossier](../zone-stories/STONE_TOMB_OF_KELEK.md) now asks twenty direct builder questions and records a source-backed resolution beside each existing technical follow-up. The questions distinguish Kelek's smith exchange from the Church contracts, physical Church encounters from quest ownership, supplied inputs from personal source history, and route conditions from recorded discovery or completion. The [audit supplement](KELEK_AUDIT_REVIEW.md) continues to hold the directed graph, typed-reference closure, and title/history evidence.

Keep `kelek` DO NOT MOVE. Preserve the Underdark and Krethik pairs, isolated #87951, imported #359 epic node, Deliverer binding, and current Church ownership correction. No inactive-WLD approach, whole-zone predecessor, new candidate, or native source change is indicated. Live key use, encounter stock, cross-zone journal exposure, and durable receipt outcomes remain unplayed.

The detailed source-story set remains 64. Coverage maps 270/351 active contributors, leaving 81 unmapped. PENDING counts are 183 narrative, 369 connectivity, 172 title/history, 344 typed-dependency, and 451 builder-question rows. Twenty-five builder-question sections are complete. Curated source-only totals remain 3,471 overall, 229 for Surf, 18 for `ud_map2`, and eight for Underworld2. Continue with `khildarak`, Khildarak Stronghold. See [coverage row 143](ZONE_COVERAGE_REGISTER.csv#L143), [connectivity](WORLD_CONNECTIVITY.md#priority-173-the-stone-tomb-of-kelek), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-133-kelek-title-history-and-current-quest-ownership), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-245-kelek-retention-after-builder-review).

### Checkpoint 246: Khildarak Stronghold builder reconciliation

The existing [Khildarak dossier](../zone-stories/KHILDARAK_STRONGHOLD.md) now asks sixteen direct builder questions and retains the source-backed recommendations from its sixteen-row ZSQ matrix. The questions distinguish the independent cook and priest returns from a possible optional campaign, track the unselected learned clue without inventing a responder, and separate exact supplied proofs from personal source history. They also surface unresolved shared controls and rock declarations, the bone-keyed case and hammer, service boundaries, missing prototypes, prose gaps, typed foreign owners, and live qualification requirements. The [Checkpoint 86 audit supplement](KHILDARAK_STRONGHOLD_AUDIT_REVIEW.md) remains the source for the graph, typed dependencies, and title/history review.

Keep active `khildarak` DO NOT MOVE and retain the one source-only `ud_map2` approach (#1124854 up to #17000) on its existing candidate row 114. Preserve the three reciprocal active boundaries, separate one-way Heavens arrival, external service/reset references and local typed identities. No new attic candidate, inactive-WLD approach, or native source change is indicated. Static room/reset/code review does not prove current supplies, chosen control behavior, successful rescue, service operation, or saved completion.

The detailed source-story set remains 64. Coverage maps 270/351 active contributors, leaving 81 unmapped. PENDING counts are 183 narrative, 369 connectivity, 172 title/history, 344 typed-dependency, and 450 builder-question rows. Twenty-six builder-question sections are complete. Curated source-only totals remain 3,471 overall, 229 for Surf, 18 for `ud_map2`, and eight for Underworld2. Continue with `killing_fields`, Killing Fields. See [coverage row 144](ZONE_COVERAGE_REGISTER.csv#L144), [Khildarak connectivity](WORLD_CONNECTIVITY.md#khildarak-stronghold-three-reciprocal-boundaries-and-a-one-way-inbound), [title/history](KHILDARAK_STRONGHOLD_AUDIT_REVIEW.md#title-history-and-disposition), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-246-khildarak-retention-after-builder-review).

### Checkpoint 247: Killing Fields builder reconciliation

The existing [Killing Fields dossier](../zone-stories/KILLING_FIELDS.md) now asks twenty-four direct builder questions and keeps its 24-row ZSQ plan's source findings and recommendations. The questions preserve the one accepted exact-note return and distinguish it from optional dialogue, coded-message decoding, personal note recovery, friend/rescue history, travel, road hazards, shop stock and equipment effects. The [Checkpoint 144 audit supplement](KILLING_FIELDS_AUDIT_REVIEW.md) retains the directed graph, Surf records, compiled travel reference, and title/history evidence.

Keep `killing_fields` DO NOT MOVE; preserve the source-only Surf approach and six object resets plus one mobile reset with the Surf RETAIN candidate. Keep the active Surface pair and five reciprocal Bloody Plains boundaries separate from the outward-only #33422 edge. No new attic candidate, source-only edge or native source change is indicated. Static documents do not establish reset, encounter, combat, merchant, travel or wallet behavior in play.

The detailed source-story set remains 64. Coverage maps 270/351 active contributors, leaving 81 unmapped. PENDING counts are 183 narrative, 369 connectivity, 172 title/history, 344 typed-dependency, and 449 builder-question rows. Twenty-seven builder-question sections are complete. Curated source-only totals remain 3,471 overall, 229 for Surf, 18 for `ud_map2`, and eight for Underworld2. Continue with `kimordril`, Kimordril. See [coverage row 145](ZONE_COVERAGE_REGISTER.csv#L145), [connectivity](WORLD_CONNECTIVITY.md#priority-184-killing-fields), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-144-killing-fields-title-history-and-map-records), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-247-killing-fields-retention-after-builder-review).

### Checkpoint 248: Kimordril builder reconciliation

The existing [Kimordril dossier](../zone-stories/KIMORDRIL.md) now states fourteen direct builder questions with recommendations. Its thirteen current ZSQ rows separate exact local proofs and source history, civic/guard behavior, services and the outside Alatorin exchanges; an added map-ownership question records the old #214592 endpoint and two source-only Surf links. The [Checkpoint 97 audit supplement](KIMORDRIL_AUDIT_REVIEW.md) continues to provide the graph, typed references and same-numbered legacy-map evidence.

Keep active `kimordril` DO NOT MOVE. Preserve its three reciprocal Surface pairs and two already-counted source-only Surf approaches with Surf candidate row 97. Keep source-only nexus56 ZON/SHP and player_guilds SHP references on their existing RETAIN candidate rows 79 and 84; active foreign resets/shops and Alatorin story consumers stay under their current owners. No new source-only edge, attic candidate or native source change is indicated.

The detailed source-story set remains 64. Coverage maps 270/351 active contributors, leaving 81 unmapped. PENDING counts are 183 narrative, 369 connectivity, 172 title/history, 344 typed-dependency, and 448 builder-question rows. Twenty-eight builder-question sections are complete. Curated source-only totals remain 3,471 overall, 229 for Surf, 18 for `ud_map2`, and eight for Underworld2. Continue with `knight`, The Hall of Knighthood. See [coverage row 146](ZONE_COVERAGE_REGISTER.csv#L146), [connectivity](WORLD_CONNECTIVITY.md#kimordril-one-authored-component-and-two-already-counted-surf-approaches), [map history](DUPLICATE_AND_REPLACED_ZONES.md#kimordril-and-parallel-map-endpoints), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-248-kimordril-retention-after-builder-review).


### Checkpoint 249: Hall of Knighthood builder reconciliation

The new [Hall of Knighthood dossier](../zone-stories/HALL_OF_KNIGHTHOOD.md) gives twenty-five direct builder questions with recommended resolutions. Its [audit supplement](KNIGHT_AUDIT_REVIEW.md) adds directed reachability, the reciprocal Surface boundary, the one source-only Surf approach, bounded typed consumers/imports, and the tracked 2013 endpoint change. Questions preserve the area as a test/map without inventing a local story and keep reset objects, outside QST inputs, compiled item callbacks, and map sources with their current owners.

Keep active `knight` DO NOT MOVE and do not add a Knight candidate. Preserve `surf.wld` #259810 north to #70900 on existing Surf candidate row 97 and keep the active Surface pair. No native area or gameplay source changed; static evidence does not qualify live access, spell/hazard behavior, mobile availability, loot, or return safety.

The detailed source-story set is now 65. Coverage maps 271/351 active contributors, leaving 80 unmapped. PENDING counts are 182 narrative, 368 connectivity, 171 title/history, 343 typed-dependency, and 447 builder-question rows. Twenty-nine builder-question sections are complete. Curated source-only totals are 3,472 overall, 230 Surf, 18 `ud_map2`, and eight Underworld2. Continue with `kobold`, Kobold Settlement. See [coverage row 147](ZONE_COVERAGE_REGISTER.csv#L147), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-249-hall-of-knighthood-directed-reachability-and-boundaries), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-249-the-hall-of-knighthood-title-endpoint-history-and-related-identities), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-249-hall-of-knighthood-retention-and-source-only-approach).

### Checkpoint 250: Kobold Settlement builder and register reconciliation

Kobold already had its source-comprehensive dossier and counted story mapping. This checkpoint completes its broader register reviews and adds twenty-two direct builder questions to the existing dossier through the [Kobold audit supplement](KOBOLD_AUDIT_REVIEW.md). Coverage row 148 now records the three active arrivals, two reciprocal outbound boundaries, one new inactive Surf approach, typed imports and consumers, same-stem history, and **DO NOT MOVE** disposition. Surf's existing candidate row 97 retains the source-only approach; Sylvandawn's row 101 retains its source-only reset/shop references to Kobold object #1403. No new candidate, native area source, or gameplay source changed.

The detailed source-story set remains 65. Coverage maps 271/351 active contributors, leaving 80 unmapped. PENDING counts are 182 narrative, 367 connectivity, 170 title/history, 342 typed-dependency, and 446 builder-question rows. Thirty builder-question sections are complete. Curated source-only totals are 3,473 overall, 231 Surf, 18 `ud_map2`, and eight Underworld2. Continue with `krethik`, The Stony Keep. See [coverage row 148](ZONE_COVERAGE_REGISTER.csv#L148), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-250-kobold-settlement-directed-reachability-and-boundaries), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-250-kobold-settlement-title-and-source-history), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-250-kobold-retention-and-source-only-dependencies).

### Checkpoint 251: Krethik Keep builder reconciliation

Krethik's earlier repository-wide audit already records the active/inactive graph, title history, typed dependencies, and retention plan. This checkpoint completes the pending direct builder review by adding twenty questions and recommendations to the [Krethik audit supplement](KRETHIK_AUDIT_REVIEW.md#builder-questions-and-recommended-resolutions), grounded in the existing [source-comprehensive dossier](../zone-stories/KRETHIK_KEEP.md). The recommendations keep the three local returns separate from optional provenance, the independent Winterhaven contracts, combat and service effects, and unqualified rescue, route, and political outcomes.

Coverage row 149 now records builder-question completion while retaining the existing **DO NOT MOVE** disposition and four source-only approaches on oldmap8/Surf candidate rows. No candidate or native area/gameplay source changed. Static graph and typed closure remain as documented at Checkpoint 134.

The detailed source-story set remains 65. Coverage maps 271/351 active contributors, leaving 80 unmapped. PENDING counts are 182 narrative, 367 connectivity, 170 title/history, 342 typed-dependency, and 445 builder-question rows. Thirty-one builder-question sections are complete. Curated source-only totals remain 3,473 overall, 231 Surf, 18 `ud_map2`, and eight Underworld2. Continue with `krimman`, Lord Krimeneha's Mansion. See [coverage row 150](ZONE_COVERAGE_REGISTER.csv#L150).

### Checkpoint 252: Lord Krimeneha's Mansion

The existing source-comprehensive mansion dossier now links to a new [repository-wide audit supplement](KRIMMAN_AUDIT_REVIEW.md). It completes directed reachability, the all-95 inactive-WLD scan, title/history, typed dependency closure, and twenty direct builder questions with recommended resolutions. The audit preserves the independent seven spirit rescues and final three-keepsake exchange, distinguishes supplied copies from source history, records Quietus's two object-specific consumers, and treats Eckraldu's unreachable load room and mobile routes as separate evidence.

Coverage row 150 now records one active reciprocal Twisted Wood pair, three outbound edges from the load room, no inactive-WLD approach or room overlap, and **DO NOT MOVE** disposition with no Krimman candidate. No native area or gameplay source changed.

The detailed source-story set remains 65. Coverage maps 271/351 active contributors, leaving 80 unmapped. PENDING counts are 182 narrative, 367 connectivity, 170 title/history, 342 typed-dependency, and 444 builder-question rows. Thirty-two builder-question sections are complete. Curated source-only totals remain 3,473 overall, 231 Surf, 18 `ud_map2`, and eight Underworld2. Continue with `kuotoa`, The Temple of Blibdoolpoolp. See [coverage row 150](ZONE_COVERAGE_REGISTER.csv#L150), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-252-krimman-directed-reachability-and-boundaries), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-252-lord-krimenehas-mansion-title-and-history), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-252-krimman-do-not-move-retention).

### Checkpoint 253: The Temple of Blibdoolpoolp builder reconciliation

The [Kuo-Toa Temple dossier](../zone-stories/KUOTOA.md) already records the environmental story, absence of a local quest, fourteen reset-closed pairs, the room #88228 flag/sector anomaly, and parallel Underworld/Gold Hall religious text. Its [audit supplement](KUOTOA_AUDIT_REVIEW.md#builder-questions-and-recommended-resolutions) now translates those source findings into ten direct builder questions and recommended resolutions. The decisions preserve current room identities, names, reset values, and **DO NOT MOVE** disposition pending owner intent.

Coverage row 151 remains complete for connectivity, history, typed closure, and source-only attic evidence. No inactive approach, room overlap, new candidate, native source, or gameplay source changed.

The detailed source-story set remains 65. Coverage maps 271/351 active contributors, leaving 80 unmapped. PENDING counts are 182 narrative, 367 connectivity, 170 title/history, 342 typed-dependency, and 443 builder-question rows. Thirty-three builder-question sections are complete. Curated source-only totals remain 3,473 overall, 231 Surf, 18 `ud_map2`, and eight Underworld2. Continue with `kvarkpass_connector`, Kvark's Great Connector. See [coverage row 151](ZONE_COVERAGE_REGISTER.csv#L151), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-208-kuotoa-reset-aware-access), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-208-kuo-toa-temple-title-and-history), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-208-kuotoa-and-source-only-retention).

### Checkpoint 254: Kvark's Great Connector builder reconciliation

The [Kvark source review](KVARKPASS_CONNECTOR_STORY_REVIEW.md) now states twelve direct builder questions with source-backed recommended resolutions. It retains the environmental story, static graph and active boundary findings, typed cross-area records, the 2013 Surface endpoint correction, and DO NOT MOVE disposition. The questions separate ambient nursery QST messages from player progress, the isolated #54833 larvae room from connected map routes, the one-way UDmini boundary from a proven return, source-only old endpoints from active ingress, and reset/shop declarations from live outcomes.

Coverage row 152 now records completion of the builder-question review while preserving Kvark's active ownership and its five unregistered-WLD approach records as source-only findings. No new candidate or source-only count was added. Existing foreign reset, nested object, and shop relationships remain under their typed owners. No native area or gameplay source changed; passability, reset results, current/fall effects, stock availability, and safe return remain unqualified.

The detailed source-story set remains 65. Coverage maps 271/351 active contributors, leaving 80 unmapped. PENDING counts remain 182 narrative, 367 connectivity, 170 title/history, and 342 typed-dependency; builder-question rows fall to 442. Thirty-four builder-question sections are complete. Curated source-only totals remain 3,473 overall, 231 Surf, 18 ud_map2, and eight Underworld2. Continue with lab, The Swamp Laboratory of Khul'Lor. See [coverage row 152](ZONE_COVERAGE_REGISTER.csv#L152), [connectivity and boundaries](KVARKPASS_CONNECTOR_STORY_REVIEW.md#resets-doors-shops-and-static-connectivity), [title/history](KVARKPASS_CONNECTOR_STORY_REVIEW.md#duplicate-and-replacement-history), and [attic disposition](KVARKPASS_CONNECTOR_STORY_REVIEW.md#duplicate-and-replacement-history).

### Checkpoint 255: The Swamp Laboratory of Khul'Lor builder reconciliation

The existing [Lab dossier](../zone-stories/LAB.md) now points to fifteen direct questions and source-backed recommended resolutions in [LAB_AUDIT_REVIEW.md](LAB_AUDIT_REVIEW.md#builder-questions-and-recommended-resolutions). The matrix preserves the authored environmental statements and source identities while separating WLD reach from the seven item-based entrances, reset-closed doors from proven access, and imported prototype text from a local gameplay contract.

Coverage row 153 now records the completed builder review. Keep active lab DO NOT MOVE; preserve the two existing source-only approaches with Surf candidate row 97 and Underworld2 candidate row 116, both RETAIN IN PLACE. The entries remain outside active ingress and do not add a candidate or source-only count. No native area or gameplay source changed. Door commands, key recovery, item use, imported mana-node effects, and return safety remain unqualified.

The detailed source-story set remains 65. Coverage maps 271/351 active contributors, leaving 80 unmapped. PENDING counts remain 182 narrative, 367 connectivity, 170 title/history, and 342 typed-dependency; builder-question rows fall to 441. Thirty-five builder-question sections are complete. Curated source-only totals remain 3,473 overall, 231 Surf, 18 ud_map2, and eight Underworld2. Continue with labyrinth, Labyrinth of No Return. See [coverage row 153](ZONE_COVERAGE_REGISTER.csv#L153), [connectivity](LAB_AUDIT_REVIEW.md#active-wld-graph-and-reset-state-reach), [title/history](LAB_AUDIT_REVIEW.md#title-followed-history-and-disposition), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-207-lab-and-source-only-retention).

### Checkpoint 256: Labyrinth of No Return builder reconciliation

The existing [Labyrinth dossier](../zone-stories/LABYRINTH_OF_NO_RETURN.md) now links to eighteen direct builder questions in [LABYRINTH_OF_NO_RETURN_AUDIT_REVIEW.md](LABYRINTH_OF_NO_RETURN_AUDIT_REVIEW.md#builder-questions-and-recommended-resolutions). The recommendations preserve its three independent local returns, exact proof bundles and source declarations, distinguish switches and directional door state from personal proof, and keep Tower's receipt and Heavens' epic-node lifecycle under their current owners.

Coverage row 154 now records completion of the builder review. Keep active labyrinth DO NOT MOVE and preserve the reciprocal Mountain Tracks edge. The bounded inactive-WLD scan found no approach or room overlap; no candidate or source-only count changes. Keep the duplicate labyrint QST file out of the active loaders and treat the brass_dragon match as unbound source code. No native area or gameplay source changed.

The detailed source-story set remains 65. Coverage maps 271/351 active contributors, leaving 80 unmapped. PENDING counts remain 182 narrative, 367 connectivity, 170 title/history, and 342 typed-dependency; builder-question rows fall to 440. Thirty-six builder-question sections are complete. Curated source-only totals remain 3,473 overall, 231 Surf, 18 ud_map2, and eight Underworld2. Continue with lava, Lava Springs. See [coverage row 154](ZONE_COVERAGE_REGISTER.csv#L154), [connectivity](LABYRINTH_OF_NO_RETURN_AUDIT_REVIEW.md#active-wld-graph), [title/history](LABYRINTH_OF_NO_RETURN_AUDIT_REVIEW.md#title-history-and-disposition), and [attic disposition](LABYRINTH_OF_NO_RETURN_AUDIT_REVIEW.md#inactive-wld-sources).

### Checkpoint 257: Lava Springs builder reconciliation

The comprehensive [Lava Springs dossier](../zone-stories/LAVA_SPRINGS.md) now links to twenty direct builder questions and recommended resolutions in [LAVA_SPRINGS_AUDIT_REVIEW.md](LAVA_SPRINGS_AUDIT_REVIEW.md#builder-questions-and-recommended-resolutions). The review preserves the exact native commission and supplied-item path while separating shop purchase, reset availability, shared controls, hazardous terrain, inn and locker services, foreign Dream ownership, and broader optional campaigns.

Coverage row 155 records builder completion and changes the typed-dependency review to complete. The imports of Verz1 object #3097 and Dream object #31318, compiled Chaos items and destination, inn binding, and smith binding remain under their exact owners. Keep active lava DO NOT MOVE, preserve the active Surface boundary, and retain the source-only Surf approach under candidate row 97. No new candidate, source-only count, or native area/gameplay change is indicated.

The detailed source-story set remains 65. Coverage maps 271/351 active contributors, leaving 80 unmapped. A fresh count of the 476 registered rows gives 182 PENDING narrative, 366 connectivity, 168 title/history, 340 typed-dependency, and 439 builder-question reviews. Thirty-seven builder-question sections are complete. Curated source-only totals remain 3,473 overall, 231 Surf, 18 ud_map2, and eight Underworld2. Continue with lava_tubes, Lava Tubes. See [coverage row 155](ZONE_COVERAGE_REGISTER.csv#L155), [connectivity](LAVA_SPRINGS_AUDIT_REVIEW.md#active-wld-boundaries-and-directed-reach), [title/history](LAVA_SPRINGS_AUDIT_REVIEW.md#title-and-bounded-history), and [attic disposition](LAVA_SPRINGS_AUDIT_REVIEW.md#migration-disposition).

### Checkpoint 258: Lava Tubes builder and register reconciliation

The new [Lava Tubes dossier](../zone-stories/LAVA_TUBES.md) and [audit supplement](LAVA_TUBES_AUDIT_REVIEW.md) complete the source-backed narrative, directed graph, typed-reference, title/history, and 23-question builder review. The moonstone chain is documented across local Xexos and mobile #12027, compiled fragment/core handling, the Cyric's Revenge loader, and White Haven's Ohnagra request. The review preserves source ownership and does not claim live traversal, item availability, or safe routes.

Coverage row 156 now links the distinct source dossier and closes the narrative, title/history, typed-dependency, and builder-question fields. It records four source-only Surf approaches and retains the connectivity qualification for live doors and dynamic routes. Keep lava_tubes DO NOT MOVE, preserve its active boundaries and unresolved #12138 east exit, and retain the four Surf records on existing candidate row 97. Keep lavcav separate; add no Lava Tubes candidate.

The detailed source-story set is now 66. Coverage maps 272/351 active contributors, leaving 79 unmapped. A fresh count of the 476 registered rows gives 181 PENDING narrative, 366 connectivity, 167 title/history, 339 typed-dependency, and 438 builder-question reviews. Thirty-eight builder-question sections are complete. Curated source-only totals are 3,477 overall, 235 Surf, 18 ud_map2, and eight Underworld2. Continue with lavcav, The Underground Lava Caves. See [coverage row 156](ZONE_COVERAGE_REGISTER.csv#L156), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-258-lava-tubes-directed-reach-and-surf-source-records), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-258-lava-tubes-title-history-and-contributor-distinction), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-258-lava-tubes-retention-and-source-only-approaches).

### Checkpoint 259: Underground Lava Caves builder and register reconciliation

The existing [Underground Lava Caves dossier](../zone-stories/THE_UNDERGROUND_LAVA_CAVES.md) already documents both native contracts, the missing reliable horn producer, isolated mobile spawn, keys and switches, fire-sector behavior, imported lamp, and unregistered shop fallback. Its [audit supplement](UNDERGROUND_LAVA_CAVES_AUDIT_REVIEW.md) now records 20 direct builder questions with recommended resolutions and the exact 140/142 forward reach, 141/142 local returns, and 140/1/1 SCC sizes.

Coverage row 157 now closes its related-area/title-history and builder-question reviews while retaining runtime reach, door, mobile-location, and safety qualifications. Keep active lavcav DO NOT MOVE. Keep the Ravenloft2 reset under its owner and the player_guilds SHP room reference on candidate row 84 RETAIN IN PLACE; no new source-only WLD route or attic candidate was found.

The detailed source-story set remains 66. Coverage maps 272/351 active contributors, leaving 79 unmapped. The 476-row register now has 181 PENDING narrative, 366 connectivity, 166 title/history, 339 typed-dependency, and 437 builder-question reviews. Thirty-nine builder-question sections are complete. Curated source-only totals remain 3,477 overall, 235 Surf, 18 ud_map2, and eight Underworld2. Continue with library, The Arcaneum of L'srillizzin. See [coverage row 157](ZONE_COVERAGE_REGISTER.csv#L157), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-259-lavcav-directed-reachability-and-the-isolated-adventurer-rooms), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-259-underground-lava-caves-title-history-and-lava-tubes-distinction), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-259-lavcav-retention-and-existing-player_guilds-dependency).

### Repository-wide source/story review checkpoint 260: The Arcaneum of L'srillizzin

The existing [Arcaneum dossier](../zone-stories/THE_ARCANEUM_OF_LSRILLIZZIN.md) already gives the source-comprehensive narrative and six exact receipt identities. Its [audit supplement](ARCANEUM_AUDIT_REVIEW.md) now records directed WLD reachability and twenty direct builder questions with recommended resolutions. The 150-room graph has two weak components (113 and 37); from #402000, 110 rooms are forward reachable and 112 have authored local returns. Two type-25 portals declare dynamic travel between the components.

Coverage row 158 completes the pending builder-question review and adds exact raw reach counts. The title/history crosswalk identifies no duplicate, alternate-stem predecessor, or replacement. Keep active `library` DO NOT MOVE; preserve the reciprocal Underdark pair and the local portals under their current owners. The all-95 inactive-WLD scan found no room overlap or source-only ingress; no attic candidate or source-only count changes. No native area or gameplay source changed.

The detailed source-story set remains 66; coverage maps 272/351 active contributors, leaving 79 unmapped. The 476-row register has 181 PENDING narrative, 366 connectivity, 166 title/history, 339 typed-dependency, and 436 builder-question reviews. Forty builder-question sections are complete. Curated source-only totals remain 3,477 overall, 235 Surf, 18 `ud_map2`, and eight Underworld2. Continue with `limbo`, Limbo. See [coverage row 158](ZONE_COVERAGE_REGISTER.csv#L158), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-260-arcaneum-directed-reachability-and-dream-portals), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-260-arcaneum-title-and-tracked-history), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-260-arcaneum-retention-and-dynamic-portals).

### Repository-wide source/story review checkpoint 261: Limbo builder reconciliation

The [Limbo source and dependency review](LIMBO_STORY_REVIEW.md) now adds eighteen direct builder questions to the existing source map. It keeps the two-room isolated area, the nine one-way Desert edges, and thirteen source-only WLD references separate from ordinary player access. The compiled `necro_dracolich` binding is reconciled across both owners: Limbo has mobiles #3-#6; active Heavens has #7-#10 and #1201. Room #1's artifact, locker, resurrection, and disconnect-restoration roles remain code-driven recovery paths.

Coverage row 159 completes the builder review and records the full typed/code reference set. Keep active Limbo DO NOT MOVE. Preserve the Desert, Heavens, shop/reset, and recovery references under their current owners; retain unregistered WLD and low-VNUM records as source-only evidence. No candidate, source-only count, native area source, or gameplay source changed.

The detailed source-story set remains 66; coverage maps 272/351 active contributors, leaving 79 unmapped. The 476-row register has 181 PENDING narrative, 366 connectivity, 166 title/history, 339 typed-dependency, and 435 builder-question reviews. Forty-one builder-question sections are complete. Curated source-only totals remain 3,477 overall, 235 Surf, 18 `ud_map2`, and eight Underworld2. Continue with `lizard`, The Lizardman Swamps of Clavikord. See [coverage row 159](ZONE_COVERAGE_REGISTER.csv#L159), [connectivity](WORLD_CONNECTIVITY.md), [builder questions](LIMBO_STORY_REVIEW.md#checkpoint-261-builder-questions-and-recommended-resolutions), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-261-limbo-retention-and-shared-low-vnum-consumers).

### Repository-wide source/story review checkpoint 262: The Lizardman Swamps of Clavikord

The existing [Clavikord dossier](../zone-stories/LIZARDMAN_SWAMPS_OF_CLAVIKORD.md) already covers its two independent exact-head contracts, source and recipient competition, typed dependencies, movement, access, services, and trap behavior. The [audit supplement](LIZARDMAN_SWAMPS_AUDIT_REVIEW.md) now adds twenty direct builder questions and recommended resolutions. These recommendations keep supplied-copy eligibility, separate D/QST outcomes, actual source/recipient availability, and unqualified travel distinct.

Coverage row 160 completes the builder review. Keep active `lizard` DO NOT MOVE; preserve the reciprocal Surface and Underdark pairs and the separate Heavens/Winterhaven inbound edges. Keep Sylvandawn, Tharnadia1, and Tharold references on existing RETAIN IN PLACE candidate rows 101, 103, and 104. No inactive-WLD approach or room overlap, new candidate, source-only count, native area, or gameplay change is indicated.

The detailed source-story set remains 66; coverage maps 272/351 active contributors, leaving 79 unmapped. The 476-row register has 181 PENDING narrative, 366 connectivity, 166 title/history, 339 typed-dependency, and 434 builder-question reviews. Forty-two builder-question sections are complete. Curated source-only totals remain 3,477 overall, 235 Surf, 18 `ud_map2`, and eight Underworld2. Continue with `llyrath`, Llyrath Forest. See [coverage row 160](ZONE_COVERAGE_REGISTER.csv#L160), [connectivity](WORLD_CONNECTIVITY.md), [title/history](DUPLICATE_AND_REPLACED_ZONES.md), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-262-clavikord-retention-and-existing-source-only-references).

### Repository-wide source/story review checkpoint 263: Llyrath Forest

The existing [Llyrath source review](LLYRATH_STORY_REVIEW.md) now records eighteen direct builder questions and exact raw directed reachability. Its 93-room WLD graph reaches 92 rooms from Moonshae arrival #94900; all 93 have returns. Room #94992 is unreachable by raw WLD from the entry but is the type-25 target of the reset onyx pillar #94906. The recommendations keep environmental story cues separate from accepted receipts and preserve the source-reported current and door risks.

Coverage row 161 completes the builder review. Keep active `llyrath` DO NOT MOVE, preserve its reciprocal Moonshae pair and local pillar destination, and add no candidate. The all-95 inactive-WLD scan found no overlap or source-only approach. No native area or gameplay source changed.

The detailed source-story set remains 66; coverage maps 272/351 active contributors, leaving 79 unmapped. The 476-row register has 181 PENDING narrative, 366 connectivity, 166 title/history, 339 typed-dependency, and 433 builder-question reviews. Forty-three builder-question sections are complete. Curated source-only totals remain 3,477 overall, 235 Surf, 18 `ud_map2`, and eight Underworld2. Continue with `llzazan`, Llzazan Ghetto of Arachdrathos. See [coverage row 161](ZONE_COVERAGE_REGISTER.csv#L161), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-263-llyrath-directed-wld-reachability), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-263-llyrath-title-and-tracked-path-history), and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-263-llyrath-retention-and-the-isolated-pillar-destination).

### Repository-wide source/story review checkpoint 264: Llzazan Ghetto of Arachdrathos

The [Llzazan source review](LLZAZAN_STORY_REVIEW.md) now adds 20 direct
builder questions and a typed/code dependency map to its environmental story
review. Raw directed traversal reaches and returns from all 197 rooms from
#36200; the map has one directed SCC, 42 active inbound exits to 40 distinct
rooms, and five paired outbound exits. Static code assigns rentacleric to
Talice #36203, inn to room #36334, and Arachdrathos start/home VNUM #36329 to
every class row.

Coverage row 162 completes the builder-question review. Keep active Llzazan
DO NOT MOVE; preserve all active boundaries and typed consumers. Retain both
unregistered ud_map0 reset records for mobile #36225 on candidate row 112.
The all-95 inactive-WLD scan found no overlap or source-only approach. No
candidate, source-only count, native area, or gameplay change is indicated.

The detailed source-story set remains 66; coverage maps 272/351 active
contributors, leaving 79 unmapped. The 476-row register has 181 PENDING
narrative, 366 connectivity, 166 title/history, 339 typed-dependency, and 432
builder-question reviews. Forty-four builder-question sections are complete.
Curated source-only totals remain 3,477 overall, 235 Surf, 18 ud_map2, and
eight Underworld2. Continue with lockers, Adventurer’s Vaults. See
[coverage row 162](ZONE_COVERAGE_REGISTER.csv#L162),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-264-llzazan-directed-reachability-and-compiled-code-destinations),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-264-llzazan-title-and-history-crosswalk), and
[attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-264-llzazan-retention-and-existing-ud_map0-reference).

### Repository-wide source/story review checkpoint 265: Adventurer's Vaults

The [locker source review](LOCKERS_STORY_REVIEW.md) maps the 99 empty
placeholder rooms, their zero-exit static graph, the compiled bank and
guildhall entry paths, the 98-room runtime allocation capacity, and SQL-backed
locker identity/item storage. It records the unresolved boot-time comparison
between loaded room indexes and locker VNUMs. Twenty builder and storage-owner
questions capture those decisions.

Coverage row 163 completes narrative, tracked title/history, and builder
review. Keep active lockers DO NOT MOVE and preserve its VNUM band, compiled
entry hooks, guild path, and persistence owner. The inactive-WLD sweep found no
approach or room overlap; no candidate, source-only count, native area, or
gameplay change is indicated.

The detailed source-story set remains 66; coverage maps 273/351 active
contributors, leaving 78 unmapped. The 476-row register has 180 PENDING
narrative, 366 connectivity, 165 title/history, 339 typed-dependency, and 431
builder-question reviews. Forty-five builder-question sections are complete.
Curated source-only totals remain 3,477 overall, 235 Surf, 18 ud_map2, and
eight Underworld2. Continue with long, Defense of Longhollow. See
[coverage row 163](ZONE_COVERAGE_REGISTER.csv#L163),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-265-adventurers-vaults-static-rooms-and-dynamic-locker-paths),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-265-adventurers-vaults-title-and-tracked-path-history), and
[attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-265-lockers-retention-and-storage-code).

### Repository-wide source/story review checkpoint 266: Defense of Longhollow

The [Longhollow audit review](LONGHOLLOW_AUDIT_REVIEW.md) reconciles the raw
directed WLD graph, both reciprocal Surface boundaries, the active foreign
moonstone producer, compiled mobile bindings, and the inactive temple2 and
player_guilds source-only ownership conflicts. It records twenty direct
builder/owner questions. The existing source-comprehensive Longhollow dossier
continues to cover the quest and environmental story. The coverage row now
records the local shop #34438 and three products.

Coverage row 164 completes title/history and builder-question review. Keep
active long DO NOT MOVE; preserve the two Surface pairs and distinct source
owners. Retain temple2 candidate row 102 and player_guilds candidate row 84
without claiming their source-only rows are active Longhollow dependencies.
Preserve source-only surf.wld #222252 south to #34401 on existing Surf
candidate row 97; it is not active ingress. No new candidate, gameplay, or
native area changes were made. Static directed reachability is complete;
player access, loaded state, dynamic routes, and safe travel remain pending.

The detailed source-story set remains 66; coverage maps 273/351 active
contributors, leaving 78 unmapped. The 476-row register has 180 PENDING
narrative, 366 connectivity, 164 title/history, 339 typed-dependency, and 430
builder-question reviews. Forty-six builder-question sections are complete.
Curated source-only totals are 3,478 overall, 236 Surf, 18 ud_map2, and
eight Underworld2. Continue with lornecro, Ancient Halls of Ironstar. See
[coverage row 164](ZONE_COVERAGE_REGISTER.csv#L164),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-266-defense-of-longhollow-directed-wld-graph),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-266-defense-of-longhollow-title-and-source-history),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-266-longhollow-retention-and-overlapping-inactive-source).

### Repository-wide source/story review checkpoint 267: Ancient Halls of Ironstar

The [Ironstar audit review](LORNECRO_AUDIT_REVIEW.md) reconciles its 100-room
directed graph, reciprocal Surface boundary, two isolated staging rooms,
current/fall fields, the Duris3 same-numbered Ocean records, and imported
tradeskill/Heavens and compiled epic-stone dependencies. It adds twenty direct
builder/owner questions. The existing Ironstar dossier continues to cover the
quest, environmental story, hazards, and qualification limits.

Coverage row 165 completes title/history and builder-question review. Keep
active lornecro DO NOT MOVE and retain Duris3 as its separate existing
candidate. No non-overlapping source-only WLD approach, new candidate, source,
or gameplay change is recorded. Effective routes, hazards, and live outcomes
remain pending.

The detailed source-story set remains 66; coverage maps 273/351 active
contributors, leaving 78 unmapped. The 476-row register has 180 PENDING
narrative, 366 connectivity, 163 title/history, 339 typed-dependency, and 429
builder-question reviews. Forty-seven builder-question sections are complete.
Curated source-only totals remain 3,478 overall, 236 Surf, 18 ud_map2, and
eight Underworld2. Continue with lortower, The Tower of Darkness. See
[coverage row 165](ZONE_COVERAGE_REGISTER.csv#L165),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-267-lornecro-directed-reachability-and-surface-pair),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-267-ironstar-title-and-tracked-history),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-267-ironstar-retention-and-duris3-room-identity).

### Repository-wide source/story review checkpoint 268: The Tower of Darkness

The [Tower audit review](LORTOWER_AUDIT_REVIEW.md) reconciles the raw directed
WLD graph and lack of active inbound exits, the Braddistock-owned physical
requests and sidecar references, Fenrral's shop, and the inactive Duris3 room
identity boundary. It records twenty direct builder/owner questions; the
existing Tower dossier remains the source of record for its stories, actors,
items, exact exchanges, passwords, teleport routes, and hazards.

Coverage row 166 completes static reachability, title/history, typed-source,
and builder-owner review. The coverage inventory now records shop #134161 and
its three products. Keep active lortower DO NOT MOVE. Preserve Duris3 on
existing candidate row 36: its 286 non-overlapping source exits into Tower are
inactive approaches, and 282 same-numbered-source matches remain excluded.
No new candidate, gameplay, or native area change was made. Live entry, reset
admission, actor renewal, portal use, and safe travel remain unqualified.

The detailed source-story set remains 66; coverage maps 273/351 active
contributors, leaving 78 unmapped. The 476-row register has 180 PENDING
narrative, 366 connectivity, 162 title/history, 339 typed-dependency, and 428
builder-question reviews. Forty-eight builder-question sections are complete.
Curated source-only totals rise to 3,764 overall, including 236 Surf, 18
ud_map2, and eight Underworld2. Continue with lylr, Lylr-Meop. See
[coverage row 166](ZONE_COVERAGE_REGISTER.csv#L166),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-268-the-tower-of-darkness-raw-exits-and-inactive-duris3-approaches),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-268-the-tower-of-darkness-title-history-and-duris3-identity),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-268-the-tower-of-darkness-retention-and-duris3-approaches).

### Repository-wide source/story review checkpoint 269: Lylr-Meop

The existing [Lylr-Meop audit supplement](LYLR_MEOP_AUDIT_REVIEW.md) reconciles
its raw directed graph, two selected Surface exits, the two existing source-only
Surf approaches, imported Tharnadia and Heavens prototypes, compiled service
bindings, and bounded source history. It now records twenty direct
builder/owner questions. The comprehensive Lylr story dossier remains the
source of record for its accepted exchange, hidden tree, prison, vault,
services, environmental stories, and qualification limits.

Coverage row 167 completes bounded title/history and builder-question review
and corrects the source inventory to three shop records. The active lylr map
remains DO NOT MOVE. Keep both Surf approaches on existing Surf candidate row
97; they are not active ingress and were already counted. No new candidate,
gameplay, or native area change was made. Actual selected-world entry,
actor/source renewal, hidden recovery, captive outcomes, service settlement,
falling/trap effects, and live safety remain unqualified.

The detailed source-story set remains 66; coverage maps 273/351 active
contributors, leaving 78 unmapped. The 476-row register has 180 PENDING
narrative, 366 connectivity, 161 title/history, 339 typed-dependency, and 427
builder-question reviews. Forty-nine builder-question sections are complete.
Curated source-only totals remain 3,764 overall, 236 Surf, 18 ud_map2, and
eight Underworld2. Continue with magma, The Parallel Elemental Plane of
Magma. See [coverage row 167](ZONE_COVERAGE_REGISTER.csv#L167),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-269-lylr-meops-split-graph-and-surface-approaches),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-269-lylr-meop-title-and-bounded-source-history),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-269-lylr-meop-retention-and-existing-surf-approaches).


### Repository-wide source/story review checkpoint 270: Magma

The existing [Magma audit supplement](MAGMA_AUDIT_REVIEW.md) reconciles the
129-room directed graph, active entry boundary, inactive Duris3 and Vodgh
approaches, typed dependencies, and bounded title history. It now records
twenty direct builder and owner questions. The comprehensive Magma dossier
remains the source of record for Palenian's accepted exchange, encounter
sources, planar controls, environmental prose, and qualification limits.

Coverage row 168 completes builder-question review; title/history, static
connectivity, and typed-source review were already recorded. Keep active magma
DO NOT MOVE. Preserve the existing Duris3 and Vodgh source references on
candidate row 36; their approaches are inactive, already counted, and do not
create a new candidate. No gameplay or native area change was made. Player
entry, return, encounter renewal, fire hazards, item settlement, and Citadel
rescue remain unqualified.

The detailed source-story set remains 66; coverage maps 273/351 active
contributors, leaving 78 unmapped. The 476-row register has 180 PENDING
narrative, 366 connectivity, 161 title/history, 339 typed-dependency, and 426
builder-question reviews. Fifty builder-question sections are complete.
Curated source-only totals remain 3,764 overall, 236 Surf, 18 ud_map2, and
eight Underworld2. Continue with malch, Malch'Hor Ganl the Goblin City. See
[coverage row 168](ZONE_COVERAGE_REGISTER.csv#L168),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-270-magmas-active-graph-and-inactive-approaches),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-270-magma-title-and-duris3-identity),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-270-magma-retention-and-inactive-approaches).


### Repository-wide source/story review checkpoint 271: Malch'Hor Ganl

The existing [Malch audit supplement](MALCH_AUDIT_REVIEW.md) reconciles its
161-room graph, four reciprocal crossings, one existing Surf source-only
approach, typed imports and consumers, three shops, and bounded title history.
It now includes twenty direct builder and owner questions. The comprehensive
Goblin City dossier remains the source of record for the three distinct exact
returns, secret access, hazards, services, and wider story leads.

Coverage row 169 completes builder-question review. Keep active malch DO NOT
MOVE and retain the Surf approach on existing candidate row 97; it is already
counted and adds no new candidate. Keep surf and surface2011 RETAIN. No native
area or gameplay change was made. Live source acquisition, shared search
outcomes, trap effects, teaching/purchase behavior, daily renewal, and safe
travel remain unqualified.

The detailed source-story set remains 66; coverage maps 273/351 active
contributors, leaving 78 unmapped. The 476-row register has 180 PENDING
narrative, 366 connectivity, 161 title/history, 339 typed-dependency, and 425
builder-question reviews. Fifty-one builder-question sections are complete.
Curated source-only totals remain 3,764 overall, 236 Surf, 18 ud_map2, and
eight Underworld2. Continue with mansion, The Forgotten Mansion. See
[coverage row 169](ZONE_COVERAGE_REGISTER.csv#L169),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-271-malchs-reciprocal-crossings-and-hidden-interior),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-271-malch-title-and-separate-goblin-camp-ownership),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-271-malch-retention-and-existing-surf-approach).


### Repository-wide source/story review checkpoint 272: The Forgotten Mansion

The existing [Mansion audit supplement](MANSION_AUDIT_REVIEW.md) reconciles
its 176-room graph, two active arrival sources, reciprocal Surface exit, two
inactive source-only approaches, typed dependencies, compiled bindings, and
bounded title history. It now includes twenty direct builder and owner
questions. The source-comprehensive dossier remains the authority for the three
separate accepted returns, room states, keys, hazards, imported effects, and
qualification limits.

Coverage row 170 completes builder-question review. Keep active mansion DO NOT
MOVE; retain the Surf and Underworld2 approaches on their existing candidates.
They were already counted and do not create new map access. No native area or
gameplay change was made. Curse and nursery persistence, White House return,
played access, fall outcomes, item effects, and fresh daily supply remain
unqualified.

The detailed source-story set remains 66; coverage maps 273/351 active
contributors, leaving 78 unmapped. The 476-row register has 180 PENDING
narrative, 366 connectivity, 161 title/history, 339 typed-dependency, and 424
builder-question reviews. Fifty-two builder-question sections are complete.
Curated source-only totals remain 3,764 overall, 236 Surf, 18 ud_map2, and
eight Underworld2. Continue with marigot, Fort Marigot. See
[coverage row 170](ZONE_COVERAGE_REGISTER.csv#L170),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-272-the-forgotten-mansion-graph-and-one-way-white-house-entry),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-272-the-forgotten-mansion-title-and-bounded-history),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-272-the-forgotten-mansion-retention-and-existing-approaches).


### Repository-wide source/story review checkpoint 273: Fort Marigot

The existing [Marigot source review](MARIGOT_STORY_REVIEW.md) reconciles the
118-room map, reciprocal Surface crossings, paired reset doors, seven shops,
Heavens and Verz1 imports, and active and source-only reset references. It now
includes twenty direct builder and owner questions. The normalized title and
same-stem history were checked against all 442 zone titles and followed paths.

Coverage row 171 completes title/history and builder-question review. Keep
active marigot DO NOT MOVE and preserve current typed ownership across Heavens,
Verz1, spshold, nexus56, and solstice_gh. Keep Surf and Underworld2 RETAIN on
their existing candidates; their approaches were already counted. No native
area or gameplay change was made. Live shop sales, normal travel, fall outcomes,
Mullet's story, and wider civic results remain unqualified.

The detailed source-story set remains 66; coverage maps 273/351 active
contributors, leaving 78 unmapped. The 476-row register has 180 PENDING
narrative, 366 connectivity, 160 title/history, 339 typed-dependency, and 423
builder-question reviews. Fifty-three builder-question sections are complete.
Curated source-only totals remain 3,764 overall, 236 Surf, 18 ud_map2, and
eight Underworld2. Continue with maze_are, The Maze of Undead Army. See
[coverage row 171](ZONE_COVERAGE_REGISTER.csv#L171),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-273-fort-marigots-reciprocal-surface-routes),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-273-fort-marigot-title-and-followed-history),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-273-fort-marigot-retention-and-source-dependencies).


### Repository-wide source/story review checkpoint 274: The Maze of Undead Army

The existing [Maze audit supplement](MAZE_OF_UNDEAD_ARMY_AUDIT_REVIEW.md)
reconciles its 177-room graph, reciprocal Surface boundary, unreachable carrier
staging rooms, inactive Surf approach, Brass reset reference, typed dependencies,
and tracked title history. It now includes twenty direct builder and owner
questions. The comprehensive Maze dossier remains the authority for seven exact
returns, item-selection order, hidden proof sources, reset restrictions, and
qualification limits.

Coverage row 172 completes builder-question review. Keep active maze_are DO NOT
MOVE. Preserve the inactive Surf and brass-old-1 references on their existing
source records; they were already counted and do not authorize map loading. No
native area or gameplay change was made. Fresh stock, source recovery, hidden
access, service availability, and live safe return remain unqualified.

The detailed source-story set remains 66; coverage maps 273/351 active
contributors, leaving 78 unmapped. The 476-row register has 180 PENDING
narrative, 366 connectivity, 160 title/history, 339 typed-dependency, and 422
builder-question reviews. Fifty-four builder-question sections are complete.
Curated source-only totals remain 3,764 overall, 236 Surf, 18 ud_map2, and
eight Underworld2. Continue with mazzolin, Mazzolin. See
[coverage row 172](ZONE_COVERAGE_REGISTER.csv#L172),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-274-maze-of-undead-army-entrance-and-carrier-staging-rooms),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-274-maze-of-undead-army-title-and-source-identity),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-274-maze-of-undead-army-retention-and-source-only-references).


### Repository-wide source/story review checkpoint 275: Mazzolin

The existing [Mazzolin audit supplement](MAZZOLIN_AUDIT_REVIEW.md) reconciles
its 136-room graph, reciprocal UnderDark boundaries, one inactive ud_map2
approach, imported Heavens object, external typed scans, and bounded history. It
now includes twenty direct builder and owner questions. The comprehensive
Mazzolin dossier remains the source of record for Aeirayne's five-piece return,
exact acquisition parents, protected access, object travel, and qualification
limits.

Coverage row 173 completes builder-question review. Keep active mazzolin DO NOT
MOVE and keep the ud_map2 approach on its existing source-owner candidate. The
edge was already counted and does not create active ingress. No native area or
gameplay change was made. Fresh stock, original acquisition, protected access,
spell arrival, item settlement, fire survival, and epic stone publication remain
unqualified.

The detailed source-story set remains 66; coverage maps 273/351 active
contributors, leaving 78 unmapped. The 476-row register has 180 PENDING
narrative, 366 connectivity, 160 title/history, 339 typed-dependency, and 421
builder-question reviews. Fifty-five builder-question sections are complete.
Curated source-only totals remain 3,764 overall, 236 Surf, 18 ud_map2, and
eight Underworld2. Continue with menden, Menden-on-the-Deep. See
[coverage row 173](ZONE_COVERAGE_REGISTER.csv#L173),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-275-mazzolins-reciprocal-underdark-boundaries),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-275-mazzolin-title-and-bounded-source-history),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-275-mazzolin-retention-and-ud_map2-approach).


### Repository-wide source/story review checkpoint 276: Menden-on-the-Deep

The existing [Menden audit supplement](MENDEN_AUDIT_REVIEW.md) reconciles its
71-room graph, four reciprocal boundaries, two existing Surf source-only
approaches, competing typed relic consumers, imported shop goods, external
reset consumers, and bounded title history. The coverage inventory now records
the three local shops and twenty direct builder and owner questions. The
comprehensive dossier remains the source of record for the relic receipt,
conditional house access, pool travel, altar, and dock service limits.

Coverage row 174 completes builder-question review and corrects the shop count
to three. Keep active menden DO NOT MOVE. Preserve both Surf approaches on
existing candidate row 97; they are already counted and do not add loaded access.
No native area or gameplay change was made. Live relic and giver renewal, spoken
door arrival, shop transactions, altar/pet publication, and ship settlement
remain unqualified.

The detailed source-story set remains 66; coverage maps 273/351 active
contributors, leaving 78 unmapped. The 476-row register has 180 PENDING
narrative, 366 connectivity, 160 title/history, 339 typed-dependency, and 420
builder-question reviews. Fifty-six builder-question sections are complete.
Curated source-only totals remain 3,764 overall, 236 Surf, 18 ud_map2, and
eight Underworld2. Continue with mining, The Halfling Silver Mine. See
[coverage row 174](ZONE_COVERAGE_REGISTER.csv#L174),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-276-menden-on-the-deeps-reciprocal-routes-and-special-landing),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-276-menden-on-the-deep-title-and-bounded-history),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-276-menden-retention-and-shop-inventory).


### Repository-wide source/story review checkpoint 277: Halfling Silver Mine

The existing [Mining audit supplement](MINING_AUDIT_REVIEW.md) reconciles its
69-room graph, reciprocal Surface boundary, nine curated source-only approaches,
151 overlap-source target matches, inactive Alterian reset groups, and separate
object #4900 collision. It now includes twenty direct builder and owner
questions. The comprehensive dossier remains the source of record for the exact
whip return, mine routes, shaft and fire hazards, vault, worker stories, and
service boundaries.

Coverage row 175 completes bounded title/history and typed-source review, fixes
the duplicated archive text, and completes builder-question review. Keep active
mining DO NOT MOVE; preserve all inactive Alterian and Surf identities on their
existing candidates. No native area or gameplay change was made. Live source
recovery, shaft/fire safety, random-grid staff access, industrial work, planar
closure, and beneficiary results remain unqualified.

The detailed source-story set remains 66; coverage maps 273/351 active
contributors, leaving 78 unmapped. The 476-row register has 180 PENDING
narrative, 366 connectivity, 159 title/history, 338 typed-dependency, and 419
builder-question reviews. Fifty-seven builder-question sections are complete.
Curated source-only totals remain 3,764 overall, 236 Surf, 18 ud_map2, and
eight Underworld2. Continue with minizones, Mini Zones. See
[coverage row 175](ZONE_COVERAGE_REGISTER.csv#L175),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-277-halfling-silver-mine-graph-and-alterian-overlaps),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-277-halfling-silver-mine-title-and-alterian-room-identities),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-277-halfling-silver-mine-retention-and-alterian-sources).

### Repository-wide source/story review checkpoint 278: Mini Zones

The existing [Mini Zones story dossier](../zone-stories/MINI_ZONES.md) and
[generated source index](../../reference/zone-story-audits/minizones.md) cover
the seven native contracts, five shops, 548 resets, six literal bindings, and
the reviewed procedures. Coverage row 176 now corrects the shop range to
#5713–#5812, count five; bounded typed-source and all-442 title/history reviews
are complete, and twenty direct builder questions are recorded.

The inactive-WLD sweep finds eleven non-overlapping Surf source exits into
Mini Zones. Keep them on Surf candidate row 97 as source-only; add no candidate
row. Together with the nine unresolved Mini Zones exits and cross-area
dependencies, they do not establish installed reachability or move clearance.
Keep active `minizones` DO NOT MOVE. The curated total is 3,775 overall, with
247 Surf, 18 ud_map2, and eight Underworld2.

The detailed source-story set remains 66; coverage maps 273/351 active
contributors, leaving 78 unmapped. The 476-row register has 180 PENDING
narrative, 366 connectivity, 158 title/history, 337 typed-dependency, and 418
builder-question reviews. Fifty-eight builder-question sections are complete.
Continue in `ZONE_COVERAGE_REGISTER.csv` order with `minopass`, The Deep Ravine
of Passage. See [coverage row 176](ZONE_COVERAGE_REGISTER.csv#L176),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-278-mini-zones-and-source-only-surf-approaches),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-278-mini-zones-exact-title-and-bounded-history),
[attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-278-mini-zones-retention-and-surf-candidate-sources),
and the [Mini Zones audit review](MINIZONES_AUDIT_REVIEW.md).

### Repository-wide source/story review checkpoint 279: The Deep Ravine of Passage

The existing [Deep Ravine dossier](../zone-stories/DEEP_RAVINE_OF_PASSAGE.md)
and [repository audit](DEEP_RAVINE_AUDIT_REVIEW.md) already reconcile its
four native contracts and ten QST actors, active graph, source-only Surf approaches, typed
foreign consumers and producers, compiled item references, and same-stem
history. Checkpoint 279 adds twenty direct builder questions and extends the
normalized title scan to all 442 `.zon` files. It also corrects the prior audit's
imprecise “north self-exit” label: #94772 exits south to #94773, which exits
north back to #94772.

The four source-only Surf approaches were already curated; no new route or
candidate row is added. Keep active `minopass` DO NOT MOVE. Source-only totals
remain 3,775 overall, 247 Surf, 18 ud_map2, and eight Underworld2. Live gameplay
and persistence qualification remain separate from these source findings.

The detailed source-story set remains 66; coverage maps 273/351 active
contributors, leaving 78 unmapped. The 476-row register has 180 PENDING
narrative, 366 connectivity, 158 title/history, 337 typed-dependency, and 417
builder-question reviews. Fifty-nine builder-question sections are complete.
Continue in `ZONE_COVERAGE_REGISTER.csv` order with `mir`, Forest of Mir. See
[coverage row 177](ZONE_COVERAGE_REGISTER.csv#L177),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-279-deep-ravine-reciprocal-routes),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-279-deep-ravine-title-and-current-source-history),
[attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-279-deep-ravine-retention-and-existing-surf-approaches),
and [twenty builder questions](DEEP_RAVINE_AUDIT_REVIEW.md#checkpoint-279-direct-builder-and-owner-questions).


### Repository-wide source/story review checkpoint 280: Forest of Mir

The existing [Forest of Mir dossier](../zone-stories/FOREST_OF_MIR.md) and
[repository audit](MIR_AUDIT_REVIEW.md) already reconcile its 281-room graph,
reciprocal Highway boundaries, inactive-WLD sweep, typed QST/story/reset
imports, compiled references, and bounded same-stem history. Checkpoint 280
adds twenty direct builder and owner questions. The all-442 title/history scan
and bounded typed-source closure were already complete; no source-only route or
candidate row is added.

Keep active `mir` DO NOT MOVE. Curated source-only totals remain 3,775 overall,
247 Surf, 18 ud_map2, and eight Underworld2. Actual maze and pool access,
hazard behavior, player contribution, reward persistence, and beneficiary
outcomes remain outside this static review.

The detailed source-story set remains 66; coverage maps 273/351 active
contributors, leaving 78 unmapped. The 476-row register has 180 PENDING
narrative, 366 connectivity, 158 title/history, 337 typed-dependency, and 416
builder-question reviews. Sixty builder-question sections are complete.
Continue in `ZONE_COVERAGE_REGISTER.csv` order with `mira`, Myrabolus. See
[coverage row 178](ZONE_COVERAGE_REGISTER.csv#L178),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-280-forest-of-mir-reciprocal-boundaries),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-280-forest-of-mir-title-and-bounded-history),
[attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-280-forest-of-mir-retention-and-highway-dependencies), and
[twenty builder questions](MIR_AUDIT_REVIEW.md#checkpoint-280-direct-builder-and-owner-questions).


### Repository-wide source/story review checkpoint 281: Myrabolus

The existing [Myrabolus dossier](../zone-stories/MYRABOLUS.md) already maps
sixteen native contracts, route/access state, shared services, and builder
gaps. Checkpoint 281 closes bounded typed-reference and same-stem
history/title review, corrects the two local shop records, and adds twenty
direct builder and owner questions. Two non-overlapping `surf.wld` exits into
rooms #82670 and #82500 are retained as source-only on Surf candidate row 97.

Keep active `mira` DO NOT MOVE. Curated source-only totals rise to 3,777
overall and 249 Surf, while ud_map2 remains 18 and Underworld2 eight. Active
services, typed dependencies, the missing #2267 reset, and source-only ingress
are recorded separately; this checkpoint does not qualify runtime behavior.

The detailed source-story set remains 66; coverage maps 273/351 active
contributors, leaving 78 unmapped. The 476-row register has 180 PENDING
narrative, 366 connectivity, 157 title/history, 336 typed-dependency, and 415
builder-question reviews. Sixty-one builder-question sections are complete.
Continue in `ZONE_COVERAGE_REGISTER.csv` order with `mist`. See
[coverage row 179](ZONE_COVERAGE_REGISTER.csv#L179),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-281-myrabolus-boundaries-and-two-surf-approaches),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-281-myrabolus-title-and-bounded-history),
[attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-281-myrabolus-retention-and-surf-candidate-sources), and
[twenty builder questions](MYRABOLUS_AUDIT_REVIEW.md#checkpoint-281-direct-builder-and-owner-questions).


### Repository-wide source/story review checkpoint 282: The Shadow Forest

The existing [Shadow Forest dossier](../zone-stories/SHADOW_FOREST.md) and
[repository audit](SHADOW_FOREST_AUDIT_REVIEW.md) already cover Palon's exact
head exchange, active graph, guardian procedure, three source-only approaches,
typed-reference closure, compiled binding, and bounded title/history.
Checkpoint 282 adds twenty direct builder and owner questions. The three
Alterian Wilderness/Aopal routes were already curated on candidate rows 8 and
10; no route or candidate row is added.

Keep active `mist` DO NOT MOVE. Current curated source-only totals remain
3,777 overall, 249 Surf, 18 ud_map2, and eight Underworld2. Actual guardian
encounters, NPC roaming, source custody, beneficiary travel, forest
interventions, reward persistence, and renewal remain unqualified.

The detailed source-story set remains 66; coverage maps 273/351 active
contributors, leaving 78 unmapped. The 476-row register has 180 PENDING
narrative, 366 connectivity, 157 title/history, 336 typed-dependency, and 414
builder-question reviews. Sixty-two builder-question sections are complete.
Continue in `ZONE_COVERAGE_REGISTER.csv` order with `mist_chasm`, The Chasm of
the Misty Vale. See [coverage row 180](ZONE_COVERAGE_REGISTER.csv#L180),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-282-shadow-forest-route-and-guardian-review),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-282-shadow-forest-title-and-bounded-history),
[attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-282-shadow-forest-retention-and-counted-approaches), and
[twenty builder questions](SHADOW_FOREST_AUDIT_REVIEW.md#checkpoint-282-direct-builder-and-owner-questions).


### Repository-wide source/story review checkpoint 283: The Chasm of the Misty Vale

The existing [Chasm dossier](../zone-stories/THE_CHASM_OF_THE_MISTY_VALE.md)
and [repository audit](MIST_CHASM_AUDIT_REVIEW.md) already map four native
exchanges, potion and switch mechanics, active graph, reciprocal Surface pair,
two source-only Surf/Torr approaches, typed foreign consumers, imported
banshee stock, and bounded title/history. Checkpoint 283 adds twenty direct
builder and owner questions and corrects a stale coverage-row link. No route or
candidate row is added.

Keep active `mist_chasm` DO NOT MOVE. Retain the two inactive approaches on
existing Surf and Torr candidate rows. Current curated totals remain 3,777
overall, 249 Surf, 18 ud_map2, and eight Underworld2. Runtime exchange, travel,
hazard, switch, and foreign-reward behavior remains unqualified.

The detailed source-story set remains 66; coverage maps 273/351 active
contributors, leaving 78 unmapped. The 476-row register has 180 PENDING
narrative, 366 connectivity, 157 title/history, 336 typed-dependency, and 413
builder-question reviews. Sixty-three builder-question sections are complete.
Continue in `ZONE_COVERAGE_REGISTER.csv` order with `mistywood`. See
[coverage row 181](ZONE_COVERAGE_REGISTER.csv#L181),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-283-chasm-routes-and-counted-inactive-approaches),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-283-chasm-title-and-bounded-history),
[attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-283-chasm-retention-and-existing-surftorr-approaches), and
[twenty builder questions](MIST_CHASM_AUDIT_REVIEW.md#checkpoint-283-direct-builder-and-owner-questions).


### Repository-wide source/story review checkpoint 284: Mistywood

The existing [Mistywood dossier](../zone-stories/MISTYWOOD.md) and [repository audit](MISTYWOOD_AUDIT_REVIEW.md) already document the three independent native contracts, their exact item and reward semantics, shared source/custody limits, door and hazard declarations, the 53 source-only Surf approaches, the 38 unresolved active exits, and source-only Kilfray QST similarity. Checkpoint 284 scans normalized titles across all 442 current .zon files, confirms one Mistywood owner, completes the bounded direct source-typed review, and adds twenty direct builder and owner questions.

Keep active mistywood DO NOT MOVE. The 53 Surf approaches were already counted on candidate row 97; no route or candidate row is added. Curated totals remain 3,777 overall, 249 Surf, 18 ud_map2, and eight Underworld2. Runtime travel, stock renewal, transaction outcomes, hazard safety, editor/tool ownership, indirect lookups, and persisted references remain outside the bounded source result.

The detailed source-story set remains 66; coverage maps 273/351 active contributors, leaving 78 unmapped. The 476-row register has 180 PENDING narrative, 366 connectivity, 157 title/history, 335 typed-dependency, and 412 builder-question reviews. Sixty-four builder-question sections are complete. Continue in ZONE_COVERAGE_REGISTER.csv order with mitashi. See [coverage row 182](ZONE_COVERAGE_REGISTER.csv#L182), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-284-mistywood-source-reconciliation), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-284-mistywood-title-and-bounded-history), [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-284-mistywood-retention-and-counted-surf-approaches), and [twenty builder questions](MISTYWOOD_AUDIT_REVIEW.md#checkpoint-284-direct-builder-and-owner-questions).


### Repository-wide source/story review checkpoint 285: Mitashi

The existing [Mitashi dossier](../zone-stories/MITASHI.md) and [repository audit](MITASHI_AUDIT_REVIEW.md) already map the single six-sword exchange, its exact sources and custody, competing Air/Savannah recipes, shop and inn bindings, directed Surface boundaries, and inactive Duris3 map records. Checkpoint 285 confirms that “Mitashi - Capital City of the Jade Empire” is unique among all 442 current .zon titles and adds twenty direct builder and owner questions.

Keep active mitashi DO NOT MOVE and Duris3 RETAIN IN PLACE. The 462 non-overlapping Duris3 approaches were already counted; keep the 454 same-numbered-source records separate as map-identity evidence. No route or candidate row is added. Curated totals remain 3,777 overall, 249 Surf, 18 ud_map2, and eight Underworld2. Runtime source custody, hidden-item visibility, trades, RENT saves, effect resolution, map ownership, and persistence remain unqualified.

The detailed source-story set remains 66; coverage maps 273/351 active contributors, leaving 78 unmapped. The 476-row register has 180 PENDING narrative, 366 connectivity, 157 title/history, 335 typed-dependency, and 411 builder-question reviews. Sixty-five builder-question sections are complete. Continue in ZONE_COVERAGE_REGISTER.csv order with mntcastl. See [coverage row 183](ZONE_COVERAGE_REGISTER.csv#L183), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-285-mitashi-boundaries-and-duris3-records), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-285-mitashi-title-and-map-identity), [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-285-mitashi-and-duris3-retention), and [twenty builder questions](MITASHI_AUDIT_REVIEW.md#checkpoint-285-direct-builder-and-owner-questions).


### Repository-wide source/story review checkpoint 286: Du'Maathe Castle

The existing [Castle dossier](../zone-stories/DU_MAATHE_CASTLE.md) and [repository audit](DU_MAATHE_CASTLE_AUDIT_REVIEW.md) already map four independent lord exchanges, four potion services, exact sources and keys, two active reciprocal boundaries, five source-only Surf/oldmap8 approaches, the Alatorin and Surface-mini typed consumers, and Carmotee's epic-teacher binding. Checkpoint 286 verifies the normalized title among all 442 current .zon files, closes the bounded direct typed-reference scan, and adds twenty direct builder and owner questions. The broader semantic source comparison remains open.

Keep active mntcastl DO NOT MOVE and the five inactive approaches on existing Surf and oldmap8 candidate entries. No route or candidate row is added. Curated totals remain 3,777 overall, 249 Surf, 18 ud_map2, and eight Underworld2. Live access, source recovery, potion effects, shop purchases, RENT saves, epic instruction, and persisted player history remain unqualified.

The detailed source-story set remains 66; coverage maps 273/351 active contributors, leaving 78 unmapped. The 476-row register has 180 PENDING narrative, 366 connectivity, 157 title/history, 334 typed-dependency, and 410 builder-question reviews. Sixty-six builder-question sections are complete. Continue in ZONE_COVERAGE_REGISTER.csv order with mobs_surf. See [coverage row 184](ZONE_COVERAGE_REGISTER.csv#L184), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-286-dumaathe-castle-active-pairs-and-counted-approaches), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-286-dumaathe-castle-title-and-bounded-history), [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-286-dumaathe-castle-retention-and-counted-approaches), and [twenty builder questions](DU_MAATHE_CASTLE_AUDIT_REVIEW.md#checkpoint-286-direct-builder-and-owner-questions).


### Repository-wide source/story review checkpoint 287: Mobs Surf

The [Mobs_Surf dossier](../zone-stories/MOBS_SURF.md) and [repository audit](MOBS_SURF_AUDIT_REVIEW.md) map four disconnected room groups, 24 unresolved outbound old-map targets, nine local mobile prototypes, the active and source-only reset owners, and the empty OBJ and absent QST/story files. Checkpoint 287 verifies the normalized title across all 442 current `.zon` files, closes the bounded direct source-typed scan and same-stem history review, and adds twenty direct builder and owner questions. The external map-owner decision remains open.

Keep active `mobs_surf` DO NOT MOVE. Retain the 25 inactive `surf.zon` mobile resets on existing Surf candidate row 97; no route or candidate row is added. Curated totals remain 3,777 overall, 249 Surf, 18 ud_map2, and eight Underworld2. Live access, reset population, map identity, indirect runtime references, editor custody, and persistence remain unqualified.

The detailed source-story set is 67; coverage maps 274/351 active contributors, leaving 77 unmapped. The 476-row register has 179 PENDING narrative, 366 connectivity, 156 title/history, 333 typed-dependency, and 409 builder-question reviews. Sixty-seven builder-question sections are complete. Continue in ZONE_COVERAGE_REGISTER.csv order with `mobs_underdark`. See [coverage row 185](ZONE_COVERAGE_REGISTER.csv#L185), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-287-mobs-surf-graph-and-legacy-targets), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-287-mobs-surf-title-and-bounded-history), [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-287-mobs-surf-retention-and-existing-references), and [twenty builder questions](MOBS_SURF_AUDIT_REVIEW.md#checkpoint-287-direct-builder-and-owner-questions).


### Repository-wide source/story review checkpoint 288: Mobs Underdark

The [Mobs_Underdark dossier](../zone-stories/MOBS_UNDERDARK.md) and existing [contract/reset review](MOBS_UNDERDARK_STORY_REVIEW.md) map three native House Zarbonesti contracts, the 14-component dispersal-room graph, the reciprocal #24013/#24014 pair, two one-way active exits, and 18 already-counted inactive `ud` target matches. Checkpoint 288 verifies the normalized title among all 442 current `.zon` files, closes the bounded direct typed-reference and same-stem history review, and adds twenty direct builder and owner questions. It records Alatorin's mobile reset import, Mushroom Caverns' sidecar contracts, and CHAOS kit use of the local seal as separate source owners.

Keep active `mobs_underdark` DO NOT MOVE and preserve the 18 unresolved `ud` matches on the existing candidate row. No route or candidate row is added. Curated totals remain 3,777 overall, 249 Surf, 18 ud_map2, and eight Underworld2. Live access, actor availability, reset population, exchange acceptance, reward persistence, and return/safety remain unqualified.

The detailed source-story set is 68; coverage maps 275/351 active contributors, leaving 76 unmapped. The 476-row register has 179 PENDING narrative, 366 connectivity, 155 title/history, 332 typed-dependency, and 408 builder-question reviews. Sixty-eight builder-question sections are complete. Continue in ZONE_COVERAGE_REGISTER.csv order with `moonshae`. See [coverage row 186](ZONE_COVERAGE_REGISTER.csv#L186), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-288-mobs-underdark-static-connectivity), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-288-mobs-underdark-title-and-bounded-history), [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-288-mobs-underdark-retention-and-existing-references), and [twenty builder questions](MOBS_UNDERDARK_AUDIT_REVIEW.md#checkpoint-288-direct-builder-and-owner-questions).


### Repository-wide source/story review checkpoint 289: Moonshae Island

The existing [Moonshae dossier](../zone-stories/MOONSHAE_ISLAND.md) and
[source audit review](MOONSHAE_ISLAND_AUDIT_REVIEW.md) preserve both native
returns, the five reciprocal current boundaries, port and sea-room questions,
and typed dependencies. Checkpoint 289 verifies the normalized title across
all 442 current .zon files, completes bounded same-stem WLD/ZON/MOB/OBJ/QST/
SHP history and direct typed-source closure, and adds twenty source-backed
builder and owner questions. It records the inactive-Duris3 port exits, the
Surf approach already counted on candidate row 97, active Black Pearl and
Cityruin references, the Myrloch Vale waterfall lead, and source-only QST and
Nexus56 references.

Keep active moonshae DO NOT MOVE. No route or candidate row is added; curated
totals remain 3,777 overall, 249 Surf, 18 ud_map2, and eight Underworld2.
Live port access, reset stock, contract acceptance, fishing grants, return and
safety, persistence, and played outcomes remain unqualified.

The detailed source-story set is 69; coverage maps 276/351 active contributors,
leaving 75 unmapped. The 476-row register has 179 PENDING narrative, 366
connectivity, 155 title/history, 331 typed-dependency, and 407 builder-question
reviews. Sixty-nine builder-question sections are complete. Continue in
ZONE_COVERAGE_REGISTER.csv order with morg. See [coverage row 187](ZONE_COVERAGE_REGISTER.csv#L187), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-289-moonshae-routes-and-source-only-approaches), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-289-moonshae-all-title-and-source-history-result), [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-289-moonshae-retention-and-existing-references), and [twenty builder questions](MOONSHAE_ISLAND_AUDIT_REVIEW.md#checkpoint-289-direct-builder-and-owner-questions).


### Repository-wide source/story review checkpoint 290: Morg's Place

The existing [Morg dossier](../zone-stories/MORG.md) and [source audit review](MORG_AUDIT_REVIEW.md) cover all 30 rooms, 19 mobiles, 64 mobile reset commands, 14 door-state commands, the reciprocal UnderDark boundary, and the hidden wererat route. Checkpoint 290 confirms the normalized title among all 442 current .zon files and the bounded same-stem WLD/MOB/OBJ/ZON history, reaffirms the direct typed scan, and adds twenty source-backed builder and owner questions. It keeps the Morg's Palace naming relationship unresolved and records the missing escape-hatch and reception-hall exits, empty OBJ catalog, and QST/AREA listing without a local QST.

Keep active morg DO NOT MOVE. No route or candidate row is added; curated totals remain 3,777 overall, 249 Surf, 18 ud_map2, and eight Underworld2. Actual access through the reset-closed entrance and secret doors, mobile population, and gameplay remain unqualified.

The detailed source-story set is 70; coverage maps 277/351 active contributors, leaving 74 unmapped. The 476-row register has 179 PENDING narrative, 366 connectivity, 155 title/history, 331 typed-dependency, and 406 builder-question reviews. Seventy builder-question sections are complete. Continue in ZONE_COVERAGE_REGISTER.csv order with area:moria, Neverwinter Woods. See [coverage row 188](ZONE_COVERAGE_REGISTER.csv#L188), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-290-morgs-place-boundary-and-reset-closed-routes), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-290-morgs-place-all-title-and-source-history-result), [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-290-morg-retention-and-existing-boundary), and [twenty builder questions](MORG_AUDIT_REVIEW.md#checkpoint-290-direct-builder-and-owner-questions).


### Repository-wide source/story review checkpoint 291: Neverwinter Woods

The [Neverwinter Woods dossier](../zone-stories/NEVERWINTER_WOODS.md) and
[repository audit review](MORIA_AUDIT_REVIEW.md) preserve the five-rune story,
the source-reviewed mirror-maze code, and the distinct Agatha and foreign
horseshoe context. Checkpoint 291 confirms the title among all 442 current
.zon files and followed same-stem WLD/MOB/OBJ/ZON/QST/SHP history. It completes
the bounded direct typed scan, records Divine Home's active #99061 consumer,
the source-only moriatst shop references on candidate row 73, and
do_shift_prime's room #99295 destination, and adds twenty builder questions.

Keep active moria DO NOT MOVE. Preserve both reciprocal Surface boundaries and
the source-only moriatst dependencies on the existing candidate row. No Moria
route or candidate row is added; curated totals remain 3,777 overall, 249 Surf,
18 ud_map2, and eight Underworld2. Live rune stock, exchanges, maze travel,
source acquisition, reward recovery, and return/safety remain unqualified.

The detailed source-story set is 71; coverage maps 278/351 active contributors,
leaving 73 unmapped. The 476-row register has 179 PENDING narrative, 366
connectivity, 154 title/history, 330 typed-dependency, and 405 builder-question
reviews. Seventy-one builder-question sections are complete. Continue in
ZONE_COVERAGE_REGISTER.csv order with area:moss, Mosswood. See [coverage row 189](ZONE_COVERAGE_REGISTER.csv#L189), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-291-moria-boundaries-and-compiled-room-travel), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-291-moria-all-title-and-tracked-history-result), [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-291-moria-retention-and-existing-typed-dependencies), and [twenty builder questions](MORIA_AUDIT_REVIEW.md#checkpoint-291-direct-builder-and-owner-questions).


### Repository-wide source/story review checkpoint 292: Mosswood

The [Mosswood dossier](../zone-stories/MOSSWOOD.md) preserves Ijale’s exact
beet request and maps nine broader story areas. Checkpoint 292 adds twenty
source-backed builder and owner questions, while retaining the existing
static graph, three source-only Surf approaches, bounded typed-reference
closure, and all-442 title/history result. It records the unreachable staging
rooms, dormant reset definitions, beet source wording discrepancy, and
unresolved services and progression endpoints as builder decisions.

Keep active moss DO NOT MOVE and retain the three inactive Surf approaches on
existing candidate row 97. No route or candidate row is added; curated totals
remain 3,777 overall, 249 Surf, 18 ud_map2, and eight Underworld2. Actual
arrival, stock, admission, useful story outcomes, beneficiary persistence,
and replay remain unqualified.

The detailed source-story set is 72; coverage maps 279/351 active
contributors, leaving 72 unmapped. The 476-row register has 179 PENDING
narrative, 366 connectivity, 154 title/history, 330 typed-dependency, and 404
builder-question reviews. Seventy-two builder-question sections are complete.
Continue in ZONE_COVERAGE_REGISTER.csv order with area:mount, The Mountain of
the Banished. See [coverage row 190](ZONE_COVERAGE_REGISTER.csv#L190),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-292-mosswood-boundary-and-source-only-approaches),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-292-mosswood-title-and-bounded-history),
[attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-292-mosswood-retention-and-existing-references), and [twenty builder questions](MOSSWOOD_AUDIT_REVIEW.md#checkpoint-292-direct-builder-and-owner-questions).

### Repository-wide source/story review checkpoint 293: The Mountain of the Banished

The existing [Mountain dossier](../zone-stories/THE_MOUNTAIN_OF_THE_BANISHED.md)
and [Checkpoint 109 supplement](MOUNTAIN_OF_THE_BANISHED_AUDIT_REVIEW.md)
preserve three native returns, the four-essence tribute, spoken door behavior,
six ENTER routes, imported services, two one-way NPC load paths, and
administrative-room questions. Checkpoint 293 confirms the exact normalized
title among all 442 current .zon files and adds twenty direct builder
questions with recommended resolutions.

Keep active mount DO NOT MOVE. Preserve both existing source-only approaches
on oldmap8 and Surf candidate records and keep surface2011 RETAIN pending map
ownership. No route or candidate row is added; curated totals remain 3,777
overall, 249 Surf, 18 ud_map2, and eight Underworld2. Actual player access,
reset stock, speech/opening/travel, service effects, useful outcomes, durable
recovery, and replay remain unqualified.

The detailed source-story set is 73; coverage maps 280/351 active contributors,
leaving 71 unmapped. The 476-row register has 179 PENDING narrative, 366
connectivity, 154 title/history, 330 typed-dependency, and 403 builder-question
reviews. Seventy-three builder-question sections are complete. Continue in
ZONE_COVERAGE_REGISTER.csv order with area:mountaintracks, Mountain Tracts of
the Untamed. See [coverage row 191](ZONE_COVERAGE_REGISTER.csv#L191),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-293-mountain-of-the-banished-boundaries-and-staging),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-293-mountain-of-the-banished-all-442-title-result),
[attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-293-mountain-retention-and-existing-candidate-references), and
[twenty builder questions](MOUNTAIN_OF_THE_BANISHED_AUDIT_REVIEW.md#checkpoint-293-all-442-title-and-direct-builder-questions).

### Repository-wide source/story review checkpoint 294: Mountain Tracts of the Untamed

The [Mountain Tracts dossier](../zone-stories/MOUNTAIN_TRACTS_OF_THE_UNTAMED.md)
and [Checkpoint 47 supplement](MOUNTAIN_TRACTS_AUDIT_REVIEW.md) cover four
native exchanges, the Winterhaven Ohnagra consumer, the fifteen reciprocal
active pairs, three source-only Surf approaches, six unresolved exits, and
compiled kit references. Checkpoint 294 confirms the exact title among all
442 current .zon files and adds twenty builder questions with recommendations.

Keep active mountaintracks DO NOT MOVE and preserve the three source-only Surf
approaches on existing candidate row 97. No route or candidate row is added;
curated totals remain 3,777 overall, 249 Surf, 18 ud_map2, and eight
Underworld2. Live stock, hidden proof recovery, door admission, travel,
potion effects, reward settlement, daily renewal, and durable outcomes remain
unqualified.

The detailed source-story set is 74; coverage maps 281/351 active contributors,
leaving 70 unmapped. The 476-row register has 179 PENDING narrative, 366
connectivity, 154 title/history, 330 typed-dependency, and 402 builder-question
reviews. Seventy-four builder-question sections are complete. Continue in
ZONE_COVERAGE_REGISTER.csv order with area:mril, Miaeril Village. See [coverage
row 192](ZONE_COVERAGE_REGISTER.csv#L192),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-294-mountain-tracts-connectivity-and-unresolved-exits),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-294-mountain-tracts-all-442-title-and-tracked-history),
[attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-294-mountain-tracts-retention-and-existing-approaches), and
[twenty builder questions](MOUNTAIN_TRACTS_AUDIT_REVIEW.md#checkpoint-294-all-442-title-and-builder-questions).

### Repository-wide source/story review checkpoint 295: Miaeril Village

The [Miaeril dossier](../zone-stories/MIAERIL_VILLAGE.md) and
[Checkpoint 115 supplement](MIAERIL_AUDIT_REVIEW.md) preserve three local
item exchanges, the two asymmetric Surface exits, the concealed stump route,
Gorlon’s imported Dream skull, Qin’s separate five-skull recipe, and the
Winterhaven #33719 namespace collision. Checkpoint 295 confirms the exact
normalized title among all 442 current .zon files and adds twenty direct
builder questions.

Keep active mril DO NOT MOVE. Preserve the two existing Surface outbound
edges, the one reciprocal pair, imported object #31317, and its separate
bounded Dream consumer. No route or candidate row is added; curated totals
remain 3,777 overall, 249 Surf, 18 ud_map2, and eight Underworld2. Live
arrival, heart/fish stock, keeper service, currents/falls, accepted foreign
trade, durable reward settlement, reset renewal, and persistence remain
unqualified.

The detailed source-story set is 75; coverage maps 282/351 active contributors,
leaving 69 unmapped. The 476-row register has 179 PENDING narrative, 366
connectivity, 154 title/history, 330 typed-dependency, and 401 builder-question
reviews. Seventy-five builder-question sections are complete. Continue in
ZONE_COVERAGE_REGISTER.csv order with area:multiclass_temple, The Lost Temple
of the North. See [coverage row 193](ZONE_COVERAGE_REGISTER.csv#L193),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-295-miaeril-asymmetric-surface-exits-and-stump-route),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-295-miaeril-all-442-title-and-followed-history),
[attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-295-miaeril-retention-and-imported-dream-material), and
[twenty builder questions](MIAERIL_AUDIT_REVIEW.md#checkpoint-295-all-442-title-and-direct-builder-questions).

### Checkpoint 296: The Lost Temple of the North

CP296 completes the Temple source dossier, all-442 exact-title and bounded-history review, typed-reference closure, and 20 builder questions. Keep the active contributor DO NOT MOVE; no route or candidate is added. The detailed source-story set is 76; coverage maps 283/351 active contributors, leaving 68 unmapped. The 476-data-row register has 178 PENDING narrative, 366 connectivity, 153 title/history, 329 typed-dependency, and 400 builder-question reviews; 76 builder-question sections are complete. Continue in register order with area:mush1, Mushroom Forest, coverage row 195. See [coverage row 194](ZONE_COVERAGE_REGISTER.csv#L194), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-296-lost-temple-of-the-north-active-boundaries), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-296-lost-temple-title-and-followed-history), [retention and dependencies](ATTIC_MIGRATION_PLAN.md#checkpoint-296-lost-temple-retention-and-static-dependencies), and [twenty builder questions](MULTICLASS_TEMPLE_AUDIT_REVIEW.md#checkpoint-296-all-442-title-and-direct-builder-questions).

### Checkpoint 297: The Mushroom Forest

CP297 adds the source dossier, all-442 title and bounded-history review, typed-reference closure, one preserved source-only ud_map2 approach, and 20 direct builder questions. Keep active mush1 DO NOT MOVE; no active route is added. Curated source-only totals are 3,778 overall, 249 Surf, 19 ud_map2, and eight Underworld2. The detailed source-story set is 77; coverage maps 284/351 active contributors, leaving 67 unmapped. The 476-data-row register has 177 PENDING narrative, 366 connectivity, 152 title/history, 328 typed-dependency, and 399 builder-question reviews; 77 builder-question sections are complete. Continue with area:mushroom_caverns, The Mushroom Caverns, coverage row 196. See [coverage row 195](ZONE_COVERAGE_REGISTER.csv#L195), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-297-mushroom-forest-graph-and-source-only-entrance), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-297-mushroom-forest-title-and-bounded-history), [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-297-mushroom-forest-retention-and-ud-map-2-approach), and [twenty builder questions](MUSHROOM_FOREST_AUDIT_REVIEW.md#checkpoint-297-all-442-title-and-direct-builder-questions).


### Checkpoint 298: The Mushroom Caverns

CP298 records the unique all-442 normalized title, bounded same-stem history, complete bounded typed-reference closure, the no-overlap/no-approach result from the all-95 inactive-WLD sweep, and twenty direct builder questions. Keep active mushroom_caverns DO NOT MOVE. Its three owned native giver contracts remain declared in shared mobs_underdark.qst; local same-stem QST counts remain zero. No route or candidate is added. Curated source-only totals remain 3,778 overall, 249 Surf, 19 ud_map2, and eight Underworld2. The detailed source-story set remains 77; coverage maps 284/351 active contributors, leaving 67 unmapped. The 476-data-row register has 177 PENDING narrative, 366 connectivity, 151 title/history, 327 typed-dependency, and 398 builder-question reviews; 78 builder-question sections are complete. Continue with area:myconid, Myconid Mushroom Forest, coverage row 197. See [coverage row 196](ZONE_COVERAGE_REGISTER.csv#L196), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-298-mushroom-caverns-graph-and-inactive-wld-sweep), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-298-mushroom-caverns-title-and-bounded-history), [retention and shared dependencies](ATTIC_MIGRATION_PLAN.md#checkpoint-298-mushroom-caverns-retention-and-shared-quest-dependencies), and [twenty builder questions](MUSHROOM_CAVERNS_AUDIT_REVIEW.md#checkpoint-298-all-442-title-and-direct-builder-questions).


### Checkpoint 299: Myconid Mushroom Forest

CP299 adds twenty direct source-backed builder questions to the existing CP147 comprehensive source and repository-wide reviews. Preserve active myconid, its two reciprocal UnderDark pairs, both inactive source-only approaches, the external #88502 sources, and compiled room destinations #2381/#2368. No source, route, or candidate changes. Curated source-only totals remain 3,778 overall, 249 Surf, 19 ud_map2, and eight Underworld2. The detailed source-story set remains 77; coverage maps 284/351 active contributors, leaving 67 unmapped. The 476-data-row register has 177 PENDING narrative, 366 connectivity, 151 title/history, 327 typed-dependency, and 397 builder-question reviews; 79 builder-question sections are complete. Continue with area:myrloch_vale, Myrloch Vale, coverage row 198. See [coverage row 197](ZONE_COVERAGE_REGISTER.csv#L197), [connectivity follow-ups](WORLD_CONNECTIVITY.md#checkpoint-299-myconid-builder-follow-ups-on-the-existing-graph), [title/history follow-ups](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-299-myconid-title-history-builder-follow-ups), [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-299-myconid-retention-and-builder-follow-ups), and [twenty builder questions](MYCONID_AUDIT_REVIEW.md#checkpoint-299-direct-builder-questions-and-recommendations).


### Checkpoint 300: Myrloch Vale

CP300 extends the existing full-area review to all 442 normalized .zon titles and followed current WLD/MOB/OBJ/ZON/QST/SHP histories, and adds twenty direct builder questions. Keep active myrloch_vale DO NOT MOVE. Preserve its reciprocal Moonshae boundary, the independent Caves shield #26438 dependency, imported objects #358/#55455, and local bindings. No route or candidate is added. Curated source-only totals remain 3,778 overall, 249 Surf, 19 ud_map2, and eight Underworld2. The detailed source-story set remains 77; coverage maps 284/351 active contributors, leaving 67 unmapped. The 476-data-row register has 177 PENDING narrative, 366 connectivity, 150 title/history, 327 typed-dependency, and 396 builder-question reviews; 80 builder-question sections are complete. Continue with area:nax, Nax, coverage row 199. See [coverage row 198](ZONE_COVERAGE_REGISTER.csv#L198), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-300-myrloch-vale-route-intent-follow-ups), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-300-myrloch-vale-all-442-title-and-history), [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-300-myrloch-vale-retention-and-builder-review), and [twenty builder questions](MYRLOCH_VALE_AUDIT_REVIEW.md#checkpoint-300-twenty-direct-builder-questions-and-recommendations).


### Checkpoint 301: Nax

CP301 completes the all-442 normalized title and bounded same-stem history, closes bounded source-typed and direct compiled references, and adds twenty direct builder questions. Keep active nax DO NOT MOVE; preserve active Surface #37800/#625238, the existing source-only Surf #256920 approach, Alatorin’s G reset for object #37742, external reset owners, and compiled direct bindings. No route or candidate is added. Curated source-only totals remain 3,778 overall, 249 Surf, 19 ud_map2, and eight Underworld2. The detailed source-story set remains 77; coverage maps 284/351 active contributors, leaving 67 unmapped. The 476-data-row register has 177 PENDING narrative, 366 connectivity, 149 title/history, 326 typed-dependency, and 395 builder-question reviews; 81 builder-question sections are complete. Continue with area:negplane, Negative Material Plane, coverage row 200. See [coverage row 199](ZONE_COVERAGE_REGISTER.csv#L199), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-301-nax-compiled-routes-and-static-graph), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-301-nax-all-442-title-and-bounded-history), [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-301-nax-retention-and-compiled-dependencies), and [twenty builder questions](NAX_AUDIT_REVIEW.md#checkpoint-301-twenty-direct-builder-questions-and-recommendations).


### Checkpoint 302: Negative Material Plane

CP302 extends the previous active-only title scan to all 442 current .zon files, completes followed history for every current same-stem source, and adds twenty direct builder questions. Preserve active negplane, its two one-way active WLD inbounds, the fixed Ravenloft2 portal to #26600, its nine external QST consumers of orb #26614, and outside reset sources for local objects. No route, candidate, or migration clearance is added. Curated source-only totals remain 3,778 overall, 249 Surf, 19 ud_map2, and eight Underworld2. The detailed source-story set remains 77; coverage maps 284/351 active contributors, leaving 67 unmapped. The 476-data-row register has 177 PENDING narrative, 366 connectivity, 148 title/history, 326 typed-dependency, and 394 builder-question reviews; 82 builder-question sections are complete. Continue with area:new_cavecity, New Cave City, coverage row 201. See [coverage row 200](ZONE_COVERAGE_REGISTER.csv#L200), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-302-negative-material-plane-builder-follow-ups), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-302-negative-material-plane-all-442-title-and-history), [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-302-negative-material-plane-retention-and-builder-review), and [twenty builder questions](NEGATIVE_MATERIAL_PLANE_AUDIT_REVIEW.md#checkpoint-302-twenty-direct-builder-questions-and-recommendations).


### Checkpoint 303: New Cave city

CP303 extends the existing all-442 title result to full followed history for the current WLD/MOB/OBJ/ZON/QST/story paths and adds twenty direct builder questions. Keep active new_cavecity DO NOT MOVE; preserve all three active reciprocal boundaries, source-only Surf #277907 on candidate row 97, the Q16 external recipe and reward, external reset owners, Twin Towers Forest’s wizard placement, and compiled bindings. No route or candidate is added. Curated source-only totals remain 3,778 overall, 249 Surf, 19 ud_map2, and eight Underworld2. The detailed source-story set remains 77; coverage maps 284/351 active contributors, leaving 67 unmapped. The 476-data-row register has 177 PENDING narrative, 366 connectivity, 147 title/history, 326 typed-dependency, and 393 builder-question reviews; 83 builder-question sections are complete. Continue with area:newbie, Ailvio, Duris Newbie Outpost, coverage row 202. See [coverage row 201](ZONE_COVERAGE_REGISTER.csv#L201), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-303-new-cave-city-route-and-builder-follow-ups), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-303-new-cave-city-full-followed-history), [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-303-new-cave-city-retention-and-builder-review), and [twenty builder questions](NEW_CAVECITY_AUDIT_REVIEW.md#checkpoint-303-twenty-direct-builder-questions-and-recommendations).


### Checkpoint 304: Ailvio

CP304 completes the exact title/followed-history, typed-dependency, and builder-question reviews for active newbie, Ailvio, Duris Newbie Outpost, coverage row 202. The source title is unique among 442 current .zon files. The 117-room graph has 234/234 local resolving exits; three approaches from inactive WLD sources remain source-only. Typed closure records the 76 local-QST foreign object identities, eight shop records with ten foreign stock VNUMs, 19 reset imports, one active Alatorin reset of local #29318, inactive references/collisions, and compiled bindings. The inventory row’s SHP count is corrected from 0 to 8. Keep newbie DO NOT MOVE; no route or candidate is added. Curated source-only totals remain 3,778 overall, 249 Surf, 19 ud_map2, and eight Underworld2. The detailed source-story set remains 77; coverage maps 284/351 active contributors, leaving 67 unmapped. The 476-data-row register has 177 PENDING narrative, 366 connectivity, 146 title/history, 325 typed-dependency, and 392 builder-question reviews; 84 builder-question sections are complete. Continue with area:newbie2, Plains of Life, coverage row 203. See [coverage row 202](ZONE_COVERAGE_REGISTER.csv#L202), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-304-ailvio-source-graph-and-inactive-approaches), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-304-ailvio-title-followed-history-and-inactive-identity-overlaps), [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-304-ailvio-retention-and-source-only-references), and [twenty builder questions](NEWBIE_AUDIT_REVIEW.md#checkpoint-304-twenty-direct-builder-questions-and-recommendations).


### Checkpoint 305: Plains of Life

CP305 completes the exact title/followed-history, typed-dependency, and builder-question reviews for active newbie2, Plains of Life, coverage row 203. The normalized title is unique among 442 current .zon files. The seven-room graph has 12/12 local resolving exits and is fully reachable and returnable from #22800; no inactive-WLD overlap or approach was found. Typed closure records seven Heavens objects in the paladin’s sack, no external QST/SHP/ZON/sidecar/prototype references, and the active Ailvio type-25/scripted transport pair. Keep newbie2 DO NOT MOVE; no route or candidate is added. Curated source-only totals remain 3,778 overall, 249 Surf, 19 ud_map2, and eight Underworld2. The detailed source-story set remains 77; coverage maps 284/351 active contributors, leaving 67 unmapped. The 476-data-row register has 177 PENDING narrative, 366 connectivity, 145 title/history, 324 typed-dependency, and 391 builder-question reviews; 85 builder-question sections are complete. Continue with the next active row after newbie2. See [coverage row 203](ZONE_COVERAGE_REGISTER.csv#L203), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-305-plains-of-life-scripted-connection), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-305-plains-of-life-title-and-followed-history), [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-305-plains-of-life-retention-and-transport), and [twenty builder questions](NEWBIE2_AUDIT_REVIEW.md#checkpoint-305-twenty-direct-builder-questions-and-recommendations).



### Checkpoint 306: Forest of Serenity

CP306 completes the source-based narrative, all-442 title/followed-history, typed-dependency, and twenty-question reviews for active newfor, Forest of Serenity, coverage row 204. Its 111-room map has 299/299 resolving exits; #53300 reaches and returns from 95 authored-WLD rooms, while a separate 16-room Gnoll Wood component connects through the two reciprocal reset type-25 mirrors. The only WLD boundary is one-way from #53300 to Mist #6366. The inactive-WLD sweep found no overlap or approach. Local resets import no foreign prototypes; no external typed reference or compiled numeric binding was found. Keep newfor DO NOT MOVE; the live mirror, hut doors, border return, and environmental claims remain unqualified. No source dossier, route, candidate, or migration change is made. Curated source-only totals remain 3,778 overall, 249 Surf, 19 ud_map2, and eight Underworld2. The detailed source-story set remains 77; coverage maps 284/351 active contributors, leaving 67 unmapped. The 476-data-row register has 176 PENDING narrative, 366 connectivity, 144 title/history, 323 typed-dependency, and 390 builder-question reviews; 86 builder-question sections are complete. Continue with area:newhaven, The City of Newhaven, coverage row 205. See [coverage row 204](ZONE_COVERAGE_REGISTER.csv#L204), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-306-forest-of-serenity-graph-and-mirror-boundary), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-306-forest-of-serenity-title-and-followed-history), [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-306-forest-of-serenity-retention-and-fixed-mirrors), and [twenty builder questions](NEWFOR_AUDIT_REVIEW.md#checkpoint-306-twenty-direct-builder-questions-and-recommendations).



### Checkpoint 307: The City of Newhaven

CP307 extends Newhaven's existing source-comprehensive dossier with the all-442 title/followed-history review, typed closure, directed graph, source-only approach, corrected SHP inventory, and twenty builder questions. The 100-room map has 217/217 resolving exits in weak components of 99 and one room; from #35200, 49 rooms are reachable and returnable, 50 more can return but are not reachable from it, and isolated #35272 exits seven times to Surface. Surface room #553247 and Newhaven #35200 form the only reciprocal WLD pair. Preserve the non-overlapping inactive Surf approach #221739 north to #35200 on candidate row 97 as source-only. Coverage row 205 is corrected to four local shops (#35204–#35216). No active route, new source-story dossier, or migration clearance is added. Curated source-only totals are 3,779 overall, 250 Surf, 19 ud_map2, and eight Underworld2. The detailed source-story set remains 77; coverage maps 284/351 active contributors, leaving 67 unmapped. The 476-data-row register has 176 PENDING narrative, 366 connectivity, 143 title/history, 322 typed-dependency, and 389 builder-question reviews; 87 builder-question sections are complete. Continue with area:newhope, The Village of New Hope, coverage row 206. See [coverage row 205](ZONE_COVERAGE_REGISTER.csv#L205), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-307-newhaven-wld-graph-and-source-only-approach), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-307-newhaven-title-and-followed-history), [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-307-newhaven-retention-and-source-only-surf-approach), and [twenty builder questions](NEWHAVEN_AUDIT_REVIEW.md#checkpoint-307-twenty-direct-builder-questions-and-recommendations).

### Checkpoint 308: The Village of New Hope

CP308 extends New Hope's existing comprehensive dossier and CP67 audit with the all-442 title/followed-history result, entrance reach/return confirmation, corrected SHP inventory, and twenty direct builder questions. Keep active newhope DO NOT MOVE. From #89000, authored exits reach and return from all 228 rooms; the map retains 550/550 resolving exits and two reciprocal Surface boundaries. Correct row 206 from zero to ten shop records (#89091, #89096, #89097, #89098, #89115, #89116, #89117, #89118, #89120, #89131). Preserve the existing bounded typed closure, four independent paid crafts, vault access distinctions, underwater travel details, and custom tentacler death path. The all-95 inactive-WLD sweep found no overlap or approach; no route, candidate, or migration disposition changes. Curated source-only totals remain 3,779 overall, 250 Surf, 19 ud_map2, and eight Underworld2. The detailed source-story set remains 77; coverage maps 284/351 active contributors, leaving 67 unmapped. The 476-data-row register has 176 PENDING narrative, 366 connectivity, 142 title/history, 322 typed-dependency, and 388 builder-question reviews; 88 builder-question sections are complete. Continue with area:nexus, The Mountain of Peril Peaks, coverage row 207. See [coverage row 206](ZONE_COVERAGE_REGISTER.csv#L206), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-308-new-hope-reachreturn-and-source-only-confirmation), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-308-new-hope-all-442-title-and-followed-history), [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-308-new-hope-retention-and-corrected-shop-inventory), and [twenty builder questions](NEWHOPE_AUDIT_REVIEW.md#checkpoint-308-twenty-direct-builder-questions-and-recommendations).

### Checkpoint 309: The Mountain of Peril Peaks

CP309 extends the existing comprehensive Peril Peaks dossier with the all-442 title and followed-history result, directed boundary reach/return, inactive-WLD sweep, bounded typed closure, corrected shop inventory, and twenty direct builder questions. Keep active `nexus` DO NOT MOVE. Its 196 rooms have 380 authored exit declarations: 379 resolve and one negative D8 target is rejected. The local weak components have sizes 188, 4, 2, 1, and 1. Reciprocal Mountain Tracks boundaries are #57536/#21144 and #57637/#21015. From #57536, 66 rooms are reachable and 95 can return; from #57637, 37 are reachable and 52 can return. Correct row 207 to two actual shop blocks, keepers #57537 and #57560. The all-95 inactive-WLD scan found no overlap or approach. Bounded exact scans found no external QST item, SHP product, story-sidecar, ZON reset, duplicate mobile/object, or type-25 inbound reference; the compiled #57576 CHAOS kit entry remains an equipment reference. No route, candidate, or migration disposition changes. Curated source-only totals remain 3,779 overall, 250 Surf, 19 ud_map2, and eight Underworld2. The detailed source-story set remains 77; coverage maps 284/351 active contributors, leaving 67 unmapped. The 476-data-row register has 176 PENDING narrative, 366 connectivity, 141 title/history, 321 typed-dependency, and 387 builder-question reviews; 89 builder-question sections are complete. Continue with area:nexus_ic, The Reliquary Nexus of Aggression, coverage row 208. See [coverage row 207](ZONE_COVERAGE_REGISTER.csv#L207), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-309-peril-peaks-directed-graph-and-inactive-wld-sweep), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-309-peril-peaks-all-442-title-and-history), [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-309-peril-peaks-retention-and-corrected-shop-inventory), and [twenty builder questions](NEXUS_AUDIT_REVIEW.md#checkpoint-309-twenty-direct-builder-questions-and-recommendations).


CP310 extends Nexus IC’s source record with its environmental story map, all-442 title/followed history, exact typed closure, inactive-WLD ingress classification, and twenty direct builder questions. Keep active nexus_ic DO NOT MOVE and Duris3 RETAIN IN PLACE. The 100-room graph has 258/258 resolving exits in two weak components of 76 and 24 rooms, with four reciprocal Surface boundaries. The all-95 inactive-WLD scan traversed 527,791 rooms and 2,080,773 exits and found 176 source-only Duris3 exits into #130100–#130199; 224 additional matches begin at active-owned source VNUMs. The Duris3 #130200 west-to-#130199 record starts at VNUM #130200, which is owned by active nexus_uc, so it is excluded from source-only ingress. No active route or migration disposition changes. Curated source-only totals rise to 3,955 overall; Surf remains 250, ud_map2 19, and Underworld2 eight. The detailed source-story set rises to 78; coverage maps 285/351 active contributors, leaving 66 unmapped. The 476-data-row register has 175 PENDING narrative, 366 connectivity, 140 title/history, 320 typed-dependency, and 386 builder-question reviews; 90 builder-question sections are complete. Continue with area:nexus_island, The Reliquary Nexus of the Corsairs, coverage row 209. See [coverage row 208](ZONE_COVERAGE_REGISTER.csv#L208), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-310-reliquary-nexus-of-aggression-graph-and-inactive-wld-sweep), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-310-reliquary-nexus-of-aggression-title-and-followed-history), [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-310-reliquary-nexus-of-aggression-retention-and-duris3-source-only-references), and [twenty builder questions](NEXUS_IC_AUDIT_REVIEW.md#checkpoint-310-twenty-direct-builder-questions-and-recommendations).


CP311 extends Nexus Island’s source record with its island-conflict and crater story map, all-442 title/followed history, exact typed closure, reach/return graph, inactive-WLD ingress classification, and twenty direct builder questions. Keep active nexus_island DO NOT MOVE and Duris3 RETAIN IN PLACE. The 100-room graph has 277 authored exits: 275 resolve, two D5 targets of -1 are dropped, and the 274 local resolving exits form one weak component. Its reciprocal Surface boundary connects #130301 and #519153; all 100 rooms are reachable and returnable from #130301 in raw traversal, while 39 rooms carry fall markers. The all-95 inactive-WLD scan traversed 527,791 rooms and 2,080,773 exits and found 121 source-only Duris3 exits into #130300–#130399; 279 additional matches begin at active-owned source VNUMs. No active route or migration disposition changes. Curated source-only totals rise to 4,076 overall; Surf remains 250, ud_map2 19, and Underworld2 eight. The detailed source-story set rises to 79; coverage maps 286/351 active contributors, leaving 65 unmapped. The 476-data-row register has 174 PENDING narrative, 366 connectivity, 139 title/history, 319 typed-dependency, and 385 builder-question reviews; 91 builder-question sections are complete. Continue with area:nexus_uc, The Reliquary Nexus of The Dead, coverage row 210. See [coverage row 209](ZONE_COVERAGE_REGISTER.csv#L209), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-311-reliquary-nexus-of-the-corsairs-graph-and-inactive-wld-sweep), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-311-reliquary-nexus-of-the-corsairs-title-and-followed-history), [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-311-reliquary-nexus-of-the-corsairs-retention-and-duris3-source-only-references), and [twenty builder questions](NEXUS_ISLAND_AUDIT_REVIEW.md#checkpoint-311-twenty-direct-builder-questions-and-recommendations).

CP312 completes the `nexus_uc` environmental-story dossier, WLD and conditional object-route topology, code-driven entry mapping, all-442 title/history, exact typed closure, and twenty builder questions. Keep active `nexus_uc` DO NOT MOVE and Duris3 RETAIN IN PLACE. The 85-room WLD has 177 resolving exits, one reciprocal Surface boundary, and eight raw weak components; WLD-only reach/return from #130200 is four rooms, while the 14 reset ITEM_TELEPORT destinations conditionally connect all 85. Five #468 rift placements route from Heavens, Tharnadia, Kimordril, Khildarak, and Shady to #130200 for levels 10–30. The all-95 inactive-WLD sweep found 171 source-only Duris3 approaches among 340 target matches. Curated source-only totals rise from 4,076 to 4,247 overall; Surf remains 250, ud_map2 19, and Underworld2 eight. The detailed source-story set rises to 80; coverage maps 287/351 active contributors, leaving 64 unmapped. Recounting the current 476-row register case-insensitively for `pending` after CP312 gives 173 narrative, 360 connectivity, 142 title/history, 318 typed-dependency, and 384 builder-question statuses; 92 rows are no longer flagged pending for builder questions. This is the current-register snapshot; earlier checkpoint totals remain historical. Continue with the next pending active contributor, `nhavan_island_one`. See [coverage row 210](ZONE_COVERAGE_REGISTER.csv#L211), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-312-nexus-upper-city-graph-object-routes-and-source-only-approaches), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-312-nexus-upper-city-title-and-followed-history), [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-312-nexus-uc-retention-and-external-entry-dependencies), and [twenty builder questions](NEXUS_UC_AUDIT_REVIEW.md#checkpoint-312-twenty-direct-builder-questions-and-recommendations).
### Checkpoint 313: Nhavan Island

CP313 completes the environmental-story dossier, directed graph and inactive-WLD sweep, normalized title/followed history, typed-dependency review, and twenty builder questions for active `nhavan_island_one`. Keep it **DO NOT MOVE**. The island has 23 rooms, 53 exits, 52 resolving local edges, one pruned Duris3 sea target, and no active external WLD boundary. The only compiled outside story mention is an active Menden fisherman line about turtle legs; it has no item check or route. Correct the coverage row's SHP inventory from zero to one shop, keeper #20303. Keep Duris3 **RETAIN IN PLACE** and preserve the #148782 reference on existing candidate row 36; its 26 active references stay unchanged. No new approach is found and the curated source-only total remains 4,247 overall, 250 Surf, 19 ud_map2, and eight Underworld2. The detailed source-story set is 81; coverage maps 288/351 active contributors, leaving 63 unmapped. The 476-data-row register has 172 PENDING narrative, 360 connectivity, 141 title/history, 317 typed-dependency, and 383 builder-question reviews; 93 builder-question sections are complete. Continue with `area:northern_wilderness`, Northern Wilderness, row 215. See [coverage row 211](ZONE_COVERAGE_REGISTER.csv#L212), [source dossier](../zone-stories/NHAVAN_ISLAND_ONE.md), [audit supplement](NHAVAN_ISLAND_ONE_AUDIT_REVIEW.md#twenty-direct-builder-questions-and-recommended-resolutions), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-313-nhavan-island-reachreturn-and-inactive-wld-sweep), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-313-nhavan-island-title-and-followed-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-313-nhavan-island-in-place-retention).

### Checkpoint 314: Northern Wilderness

CP314 completes the source dossier, directed graph and all-95 inactive-WLD sweep, normalized title/followed history, typed-dependency review, and twenty builder questions for active `northern_wilderness`. Keep it **DO NOT MOVE**. Its 149 rooms have 348 resolving exits, 347 local and one one-way boundary to Centaur #93300. Three weak components have sizes 112/26/11; from #12200, 101 rooms are reachable and 112 can return. Two rooms have F10 fall markers. The active Centaur story sidecar documents the route onward to Surface but not a return into Northern Wilderness. No inactive-WLD approach is found. Preserve the external reset dependency: ten active Troll Caves commands use small-bat mobile #12210. The title is unique among all 442 `.zon` titles. No candidate row or source-only approach is added, and curated totals remain 4,247 overall, 250 Surf, 19 ud_map2, and eight Underworld2. The detailed source-story set is 82; coverage maps 289/351 active contributors, leaving 62 unmapped. The 476-data-row register has 171 PENDING narrative, 360 connectivity, 140 title/history, 316 typed-dependency, and 382 builder-question reviews; 94 builder-question sections are no longer flagged pending. Continue with `area:nyneth`, row 216. See [coverage row 215](ZONE_COVERAGE_REGISTER.csv#L216), [dossier](../zone-stories/NORTHERN_WILDERNESS.md), [audit supplement](NORTHERN_WILDERNESS_AUDIT_REVIEW.md#checkpoint-314-twenty-direct-builder-questions-and-recommendations), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-314-northern-wilderness-directed-graph-and-inactive-wld-sweep), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-314-northern-wilderness-title-and-followed-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-314-northern-wilderness-in-place-retention).

### Checkpoint 315: Ny'Neth

CP315 completes the first-stage source dossier, directed graph and all-95 inactive-WLD sweep, normalized title/followed history, typed-dependency review, and twenty builder questions for active `nyneth`. Keep it **DO NOT MOVE**. Its 78 rooms have 175/175 resolving exits, with weak components of 67 and 11; raw WLD reach from #22930 reaches 65 rooms and 66 can return. Surface #581417 is the reciprocal entrance boundary; White Harbor #55633 has a one-way technical inbound to #22994. The inactive-WLD sweep found no overlapping rooms and one source-only `surf.wld` approach, #284533 north to #22900. Preserve it on existing Surf candidate row 97 and raise curated source-only totals to 4,248 overall and 251 Surf; ud_map2 remains 19 and Underworld2 eight. Preserve the four portal objects, the Heavens monolith import, the rareload configuration/text mismatch, 27 external reset references, and two direct compiled bindings. The normalized `Ny'Neth` title is unique among 442 `.zon` files; bounded history establishes no alternate-stem rename, predecessor, or replacement. The detailed source-story set is 83; coverage maps 290/351 active contributors, leaving 61 unmapped. The 476-data-row register now has 170 PENDING narrative, 359 connectivity, 139 title/history, 315 typed-dependency, and 381 builder-question reviews; 95 builder-question sections are no longer flagged pending. Continue with `area:olympus`, row 221. See [coverage row 216](ZONE_COVERAGE_REGISTER.csv#L217), [dossier](../zone-stories/NYNETH.md), [audit supplement](NYNETH_AUDIT_REVIEW.md#checkpoint-315-twenty-direct-builder-questions-and-recommendations), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-315-nyneth-directed-graph-and-inactive-approach), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-315-nyneth-title-and-followed-history), [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-315-nyneth-in-place-retention), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

### Checkpoint 316: Olympus

CP316 completes the source dossier, directed graph and all-95 inactive-WLD sweep, normalized title/followed history, exact typed-reference review, and twenty builder questions for active `olympus`. Keep it **DO NOT MOVE**. Its ten rooms have 19 authored exits: 18 local, one positive-unresolved exit west from #99809 to #218064. All ten rooms are reachable from and can return to #99800 on resolved local WLD. No active inbound/outbound boundary or inactive-WLD inbound is found. #218064 exists in two inactive candidate maps under distinct room titles; preserve the unresolved exit on existing `surf` and `surface2011` rows without changing either 145-reference count. The dormant portal handler, absent local QST/sidecar, and Herders' name-only “Mount Olympus” response remain explicit builder questions. The normalized title is unique among 442 `.zon` files, with no bounded-history replacement evidence. No candidate row or source-only approach is added; curated totals remain 4,248 overall, 251 Surf, 19 ud_map2, and eight Underworld2. The detailed source-story set is 84; coverage maps 291/351 active contributors, leaving 60 unmapped. The 476-data-row register now has 169 PENDING narrative, 358 connectivity, 138 title/history, 314 typed-dependency, and 380 builder-question reviews; 96 builder-question sections are no longer flagged pending. Continue with `area:otower`, row 225. See [coverage row 221](ZONE_COVERAGE_REGISTER.csv#L222), [dossier](../zone-stories/OLYMPUS.md), [audit supplement](OLYMPUS_AUDIT_REVIEW.md#checkpoint-316-twenty-direct-builder-questions-and-recommendations), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-316-olympus-directed-graph-and-inactive-target), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-316-olympus-title-and-followed-history), [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-316-olympus-in-place-retention), and [existing candidate rows](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


## Checkpoint 317: Otower

Active otower is fully reviewed at the source level and remains **DO NOT MOVE**. Its 51 rooms have 104 resolving exits, with all 51 rooms reachable and returnable in the raw local graph. Two active reciprocal boundaries reach Surface and Underworld3. Inactive surf.wld #225677 points north to #11300; preserve it on existing Surf candidate row 97. Surf's 145 active references are unchanged.

The dossier records the mountain, tower, swamp, and kuo-toan cave stories, keyed progression, and White Harbor's external #55447 ambassador delivery. Exact dependencies also include Heavens #359, Jindon #82018, Flog's compiled epic-teacher binding, and CHAOS kit uses of objects #11306, #11307, and #11319. The normalized title is unique among 442 zone titles; followed history shows no predecessor or replacement.

The reviewed source-story set is 85; coverage maps 292/351 active contributors, leaving 59 unmapped. Current PENDING reviews are 168 narrative, 357 connectivity, 137 title/history, 313 typed-dependency, and 379 builder-question sections; 97 builder-question sections are no longer flagged pending. Curated source-only totals are 4,249 overall, 252 reviewed Surf, 19 ud_map2, and eight Underworld2. Continue with area:patrols, data row 227. See [coverage row 225](ZONE_COVERAGE_REGISTER.csv#L226), [Otower dossier](../zone-stories/OTOWER.md), [audit review](OTOWER_AUDIT_REVIEW.md), and the [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

## Checkpoint 318: Patrols

Active Patrols is fully reviewed at the source level and remains **DO NOT MOVE**. Its seven rooms have 28 positive-unresolved WLD exits; no edge resolves locally or in the current loader. The all-95 inactive-WLD sweep found seven Fists of Fury room-VNUM collisions and 14 raw incoming records, of which two originate in non-overlapping source rooms and remain source-only, identity-ambiguous candidate references. Inactive Surf/Surface2011/UnderDark2011 room titles disagree, so preserve the unresolved targets without assigning a route owner.

The dossier records seven hometown forces, 21 mobile prototypes, captain equipment resets, blank room descriptions, no local quest or sidecar, and no named enemy or objective. Exact dependencies include the Fists of Fury reset/shop references and compiled room #30504/#30511 bindings; no active exact QST item, SHP product, story-sidecar VNUM, external active reset, or patrol-movement binding was found. The normalized title is unique among 442 zone titles, and followed history shows no predecessor or replacement.

The detailed source-story set is 86; coverage maps 293/351 active contributors, leaving 58 unmapped. Current PENDING reviews are 167 narrative, 356 connectivity, 136 title/history, 312 typed-dependency, and 378 builder-question sections; 98 builder-question sections are complete. Curated source-only totals are 4,251 overall, 252 reviewed Surf, 19 ud_map2, and eight Underworld2. Continue with area:pharrvly, data row 228. See [coverage row 228](ZONE_COVERAGE_REGISTER.csv#L228), [Patrols dossier](../zone-stories/PATROLS.md), [audit review](PATROLS_AUDIT_REVIEW.md), and [existing candidate rows](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L42).

## Checkpoint 319: Pharr Valley

Pharr Valley's existing source-comprehensive dossier is revalidated against the current source tree and remains **DO NOT MOVE**. It covers two independent QST receipts, exact object custody and accounting boundaries, 189 rooms and 408 resolving exits, two reciprocal cross-area pairs, native hazards, ship and pet bindings, and the limit between environmental lore and accepted quest outcomes. Twenty builder questions now record recommended resolutions.

The all-95 inactive-WLD result remains current: no source-room overlap or ingress to Pharr Valley. External Nexus56 shop #83032 and its G reset both reference apple #40208, but remain source-only candidate dependencies on row 79; shop-reader fallback is conditional. The normalized active title remains unique and bounded WLD/ZON history identifies no predecessor or replacement.

The detailed source-story set remains 86; coverage maps 293/351 active contributors, leaving 58 unmapped. Current PENDING reviews are 167 narrative, 356 connectivity, 136 title/history, 312 typed-dependency, and 377 builder-question sections; 99 builder-question sections are complete. Curated source-only totals remain 4,251 overall, 252 reviewed Surf, 19 ud_map2, and eight Underworld2. Continue with `area:pineholl`, data row 229. See [coverage row 229](ZONE_COVERAGE_REGISTER.csv#L229), [dossier](../zone-stories/PHARR_VALLEY.md), [audit review](PHARR_VALLEY_AUDIT_REVIEW.md#checkpoint-319-builder-questions-and-recommended-resolutions), and [Nexus56 candidate row 79](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79).

## Checkpoint 320: Pine Hollow

Pine Hollow's existing source-comprehensive dossier is reconciled with current graph, typed-reference, title, and bounded-history evidence. It remains **DO NOT MOVE**. The 166 rooms have 437 resolving exits; each active inbound anchor raw-reaches and can return from the 161-room main component. Five isolated quest-mob staging rooms include terminal holding room #16165. Two cross-area connections are reciprocal; the one-way Rftjngle inbound and six NPC staging outbounds remain separately classified.

Exact typed closure includes four active external QST item commands to Pine Hollow objects encoded by three external story sidecars, 16 old-format Luiren shop-product overlaps, eight Luiren keeper-VNUM overlaps, and two Luiren room-VNUM overlaps. No external ZON reset, active N-format SHP product, duplicate prototype, or type-25 portal target was found. Luiren remains on candidate row 62 with RETAIN IN PLACE; its old `O` records and conditional fallback remain unresolved source-only compatibility/identity evidence.

The detailed source-story set remains 86; coverage maps 293/351 active contributors, leaving 58 unmapped. Current PENDING reviews are 167 narrative, 356 connectivity, 135 title/history, 312 typed-dependency, and 376 builder-question sections; 100 builder-question sections are complete. Curated source-only totals remain 4,251 overall, 252 reviewed Surf, 19 ud_map2, and eight Underworld2. Continue with `area:plane_air_one`, data row 230. See [coverage row 230](ZONE_COVERAGE_REGISTER.csv#L230), [dossier](../zone-stories/PINE_HOLLOW.md), [audit review](PINE_HOLLOW_AUDIT_REVIEW.md), and [Luiren candidate row 62](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L62).

## Checkpoint 321: Plane of Air

Plane of Air's full WLD/MOB/OBJ/ZON source map, active and dynamic routes, bounded typed dependencies, normalized title, followed file history, and twenty builder questions are complete. Keep active `plane_air_one` **DO NOT MOVE**. Its 125-room WLD has 749 resolving exits, nine active inbound records, two reciprocal outbounds, and a 122-room directed core with three one-way rooms. The native source has no QST or story sidecar. Code-based planar entry, a damaging Ethereal pool, and the older reciprocal Smoke-pocket object route remain separate from ordinary WLD access.

The all-95 inactive-WLD review found ten south exits from Water Plane into Air. Five begin at unique Water Plane room VNUMs and remain source-only; five begin at Limbo/Heavens-owned room VNUMs and remain identity overlaps. Five further unique-source Water Plane exits reach Heavens. Keep Water Plane on existing candidate row 120 with RETAIN IN PLACE; add no active route or candidate. Its active-inbound count remains zero. Curated source-only totals rise by ten to 4,261 overall, with Surf 252, ud_map2 19, and Underworld2 eight.

The detailed source-story set is 87; coverage maps 294/351 active contributors, leaving 57 unmapped. The 476-data-row register now has 166 PENDING narrative, 356 connectivity, 134 title/history, 312 typed-dependency, and 375 builder-question reviews; 101 builder-question sections are complete. Continue with `area:plane_earth_one`, data row 231. See [coverage row 231](ZONE_COVERAGE_REGISTER.csv#L231), [dossier](../zone-stories/PLANE_OF_AIR.md), [audit review](PLANE_OF_AIR_AUDIT_REVIEW.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-321-plane-of-air-graph-and-source-only-approaches), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-321-plane-of-air-normalized-title-and-followed-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-321-plane-of-air-and-water-plane-retention).


## Checkpoint 322: Plane of Earth

Completed the source-comprehensive Plane of Earth dossier, directed WLD graph, exact active and code-based routes, all-95 inactive-WLD scan, typed dependency review, normalized title and followed history, and twenty builder questions. Keep active `plane_earth_one` **DO NOT MOVE**. Its 129-room WLD has 763 resolving exits, one weak component, a 121-room directed core, eight active inbound records, and three outbound records. One outbound pair reciprocates with Earthp; the other two are technical one-way links to Grumbar's lair. The Negative Plane fissure and Earth pool remain object/code paths outside the WLD register. No inactive-WLD approach or VNUM overlap was found.

Preserve Plane of Earth's item and mobile identities used by the Heavens QST, Temple of the Earth, Grumbar's Domain, and Tharnadian Ruin. Keep the technical room range, one-way load-room branches, helper-list room VNUMs, and described sector anomalies as builder questions. No route, reset, gameplay, or migration change is proposed. Curated source-only totals remain 4,261 overall (Surf 252, ud_map2 19, Underworld2 eight), and no candidate row changes.

The detailed source-story set is 88; coverage maps 295/351 active contributors, leaving 56 unmapped. The 476-data-row register now has 165 PENDING narrative, 356 connectivity, 133 title/history, 312 typed-dependency, and 374 builder-question reviews; 102 builder-question sections are complete. Continue with `area:plane_fire_one`, data row 232. See [coverage row 232](ZONE_COVERAGE_REGISTER.csv#L232), [dossier](../zone-stories/PLANE_OF_EARTH.md), [audit review](PLANE_OF_EARTH_AUDIT_REVIEW.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-322-plane-of-earth-graph-and-external-routes), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-322-plane-of-earth-normalized-title-and-followed-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-322-plane-of-earth-in-place-retention).
## Checkpoint 323: Plane of Fire

Plane of Fire's active map, typed dependencies, object routes, normalized title, followed source history, and twenty builder questions are complete. Keep active `plane_fire_one` **DO NOT MOVE**. Its 125 rooms have 750 resolving exits, nine active inbound records, two outbounds, and one weak component; raw traversal from active WLD arrival targets reaches 122 rooms. Preserve the reciprocal Brass boundary, the distinct Firep endpoints, Imix's reset and item references, the Brass guardian hook, Heavens' fixed objects, and the Fire/Smoke, Negative Plane, and Ethereal pool routes. None of these source records establishes live access, safe arrival, or return.

The inactive-WLD review found two approaches to Fire #25455. `brass-old-1` starts at reused room VNUM #139100 and remains identity-ambiguous; `brass-old-3` starts at unique source room #25201 and contributes one curated source-only approach. Retain both on existing candidate rows and keep each candidate's active-inbound count at zero. Sylvandawn's two potion product rows remain source-only. Curated source-only totals rise to 4,262 overall (Surf 252, ud_map2 19, Underworld2 eight).

The detailed source-story set is 89; coverage maps 296/351 active contributors, leaving 55 unmapped. The 476-data-row register now has 164 PENDING narrative, 355 connectivity, 132 title/history, 311 typed-dependency, and 373 builder-question reviews; 103 builder-question sections are complete. Continue with `area:plane_water_one`, data row 233. See [Fire coverage entry (CSV line 233)](ZONE_COVERAGE_REGISTER.csv#L233), [dossier](../zone-stories/PLANE_OF_FIRE.md), [audit review](PLANE_OF_FIRE_AUDIT_REVIEW.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-323-plane-of-fire-graph-and-external-routes), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-323-plane-of-fire-title-and-followed-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-323-plane-of-fire-and-retained-brass-candidates).

## Checkpoint 324: Plane of Water

Plane of Water's environmental story, WLD reachability, object/code routes, typed dependencies, title, and followed history are complete, with twenty builder questions recorded. Keep active `plane_water_one` **DO NOT MOVE**. Its 125-room map has 750 resolving exits, eight active inbound records, one reciprocal Sea Kingdom outbound pair, one 125-room strong component, and full raw reachability from the active WLD arrivals. Preserve the Heavens item VNUMs, Olhydra's active callback and ship-quest role, the Water pool, two external pool arrivals, and the Negative Plane and Orb routes. No live safety or player-access conclusion is implied.

The all-95 inactive-WLD scan found no Water map overlap or incoming source record. Candidate `waterplane` remains unchanged on row 120 for its existing source-only Air/Heavens edges; no count or active route changes. Curated source-only totals remain 4,262 overall (Surf 252, ud_map2 19, Underworld2 eight).

The detailed source-story set is 90; coverage maps 297/351 active contributors, leaving 54 unmapped. The 476-data-row register now has 163 PENDING narrative, 354 connectivity, 131 title/history, 310 typed-dependency, and 372 builder-question reviews; 104 builder-question sections are complete. Continue with `area:player_castles`, data row 234. See [Water coverage entry (CSV line 234)](ZONE_COVERAGE_REGISTER.csv#L234), [dossier](../zone-stories/PLANE_OF_WATER.md), [audit review](PLANE_OF_WATER_AUDIT_REVIEW.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-324-plane-of-water-graph-and-external-routes), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-324-plane-of-water-title-and-followed-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-324-plane-of-water-in-place-retention).

## Checkpoint 325: Player Castles

Added the source dossier and audit supplement for active `player_castles`, a three-room map with ten mobile and 59 object prototypes, no local QST or story sidecar, and shared guildhall objects and callbacks. The authored WLD has three exits: two resolving reciprocal local edges and one negative target, with two weak components of sizes two and one. The all-95 inactive-WLD sweep found no room overlap or approach. No active WLD boundary or source-only approach is added.

Typed closure records 20 active external ZON/SHP reset or product references, plus source-only imports on 44 existing attic candidates: 41 unregistered ZON files with 174 object resets, `surf.zon` with one mobile reset, and two SHP candidates with four product references. Active CBL Hall, Heavens, Surface, and Tharnadia consumers are documented. No candidate is created or cleared; candidate active-inbound counts and curated source-only totals remain 4,262 overall (Surf 252, ud_map2 19, Underworld2 eight). The title is unique across 442 current `.zon` files, and bounded followed history establishes no alternate-stem rename or replacement. Keep `player_castles` **DO NOT MOVE**.

The detailed source-story set is 91; coverage maps 298/351 active contributors, leaving 53 unmapped. The 476-data-row register now has 162 PENDING narrative, 353 connectivity, 130 title/history, 309 typed-dependency, and 371 builder-question reviews; 105 builder-question sections are complete. Continue with `area:PortSkythic`, data row 236. See [Player Castles coverage entry (CSV line 235)](ZONE_COVERAGE_REGISTER.csv#L235), [dossier](../zone-stories/PLAYER_CASTLES.md), [audit review](PLAYER_CASTLES_AUDIT_REVIEW.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-325-player-castles-graph-and-inactive-wld-sweep), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-325-player-castles-title-and-followed-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-325-player-castles-and-existing-candidate-importers).

## Checkpoint 326: Port Skythic

Completed the source-comprehensive Port Skythic dossier, active graph review, all-95 inactive-WLD sweep, typed dependency scan, normalized title and followed history, and twenty builder questions. Keep active `PortSkythic` **DO NOT MOVE**. Its 110-room map has 218/218 resolving exits and three strongly connected components of 103, six, and one room. The main city, six-room sunken ship, and isolated rift have no active WLD boundary into or out of the area.

The inactive scan found 224 distinct `Duris3.wld` approaches from 222 non-overlapping source rooms, plus 216 records beginning in the same-numbered `The Ocean` rooms. Preserve the latter as source-identity overlaps and exclude them from the curated count. Keep all 224 distinct records on the existing Duris3 candidate row 36; its active-inbound count remains 26. Curated source-only totals rise by 224 to 4,486 overall (Surf 252, ud_map2 19, Underworld2 eight). The normalized zone title is unique among 442 current `.zon` files, and no followed-path rename or whole-zone replacement is established.

The typed scan found no external exact QST, SHP, ZON reset-role, story-sidecar, duplicate-prototype, type-25 portal, type-29 switch, or direct code reference. PortSkythic has no local QST, SHP, or story sidecar; the local room titled “Within the Magic Shop” has no shop source.

The detailed source-story set is 92; coverage maps 299/351 active contributors, leaving 52 unmapped. The 476-data-row register now has 161 PENDING narrative, 352 connectivity, 129 title/history, 308 typed-dependency, and 370 builder-question reviews; 106 builder-question sections are complete. Continue with `area:potions`, data row 237. See [Port Skythic coverage entry (CSV line 237)](ZONE_COVERAGE_REGISTER.csv#L237), [dossier](../zone-stories/PORTSKYTHIC.md), [audit review](PORTSKYTHIC_AUDIT_REVIEW.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-326-port-skythic-active-graph-and-inactive-wld-sweep), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-326-port-skythic-title-and-followed-history), [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-326-port-skythic-and-existing-duris3-approaches), and [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

## Checkpoint 327: Potions

Completed the source dossier and review for active `potions`, including the
room/mobile story, the directed WLD graph, all-95 inactive-WLD sweep, typed
dependency scan, title and followed history, and twenty builder questions.
Keep `potions` **DO NOT MOVE**. Its 15 rooms form two weak components of
seven and eight rooms. All 86 authored exits are accounted for: 73 resolve
locally, seven lead one way to Ceothia #81094, and six target #25087 in
inactive `brass-old-2.wld`. The six unresolved Brass routes remain on the
existing candidate row alongside Realm's one reference; candidate row 18
still has seven references and **RETAIN IN PLACE**. No active WLD inbound,
inactive-WLD approach, room overlap, or source-only WLD count is added.

The typed scan found no external exact QST, SHP, ZON reset-role,
story-sidecar, duplicate prototype, portal, switch, or direct C/C++ reference
to Potions IDs. The local OBJ file is empty, and the ZON's object,
equipment, monolith, and Negative Plane examples are commented out. The
title is unique among current zone titles. Followed WLD/MOB/ZON history
remains on the same stem and establishes no whole-zone replacement.

The detailed source-story set is 93; coverage maps 300/351 active
contributors, leaving 51 unmapped. The 476-data-row register now has 160
PENDING narrative, 351 connectivity, 128 title/history, 307 typed-dependency,
and 369 builder-question reviews; 107 builder-question sections are
complete. Curated source-only totals remain 4,486 overall (Surf 252, ud_map2
19, Underworld2 eight). Continue with `area:pragtog`, data row 238. See
[Potions coverage row 238](ZONE_COVERAGE_REGISTER.csv#L238), [dossier](../zone-stories/POTIONS.md),
[audit review](POTIONS_AUDIT_REVIEW.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-327-potions-graph-and-inactive-wld-sweep),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-327-potions-title-and-followed-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-327-potions-and-the-retained-brass-candidate),
and [Brass candidate row 18](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L18).

## Checkpoint 328: Pragtog

Completed the source dossier and review for active pragtog: 94
rooms, 26 mobiles, 59 objects, the orc settlement and spirit clue thread,
item-triggered travel, door-reset analysis, exact typed-reference scans,
followed history, and twenty builder questions. Keep active pragtog DO NOT
MOVE. Its 198/198 WLD exits resolve, with two reciprocal boundary pairs to
Underdark and Black Tunnels. Raw directed WLD reach from each arrival is
56/94; three reset teleporter objects connect the authored WLD regions when
used. Initial-open reach after applying reset door states is three rooms
from #10649 and two from #10693, which does not model opening doors, finding
keys, or using item triggers.

The all-95 inactive-WLD scan found no room overlap and two distinct
source-only exits into #10649: ud_map1 #1115029 up and ud_map2 #1126541 up.
Preserve them on existing candidate rows 113 and 114; active inbound counts
remain one Shafts reference each. Keep both rows RETAIN IN PLACE. Curated
source-only totals rise to 4,488 overall (Surf 252, ud_map1 two, ud_map2 20,
Underworld2 eight). The title is unique; followed WLD/MOB/OBJ/ZON history
stays on one stem; and no whole-zone replacement is established. Bounded
typed scans found no exact external quest, shop, reset, sidecar, portal,
switch, duplicate-prototype, or direct active code dependency.

The detailed source-story set is 94; coverage maps 301/351 active
contributors, leaving 50 unmapped. The 476-data-row register now has 159
PENDING narrative, 350 connectivity, 127 title/history, 306 typed-dependency,
and 368 builder-question reviews; 108 builder-question sections are
complete. Continue with area:prison, data row 239. See
[Pragtog coverage row 239](ZONE_COVERAGE_REGISTER.csv#L239),
[dossier](../zone-stories/PRAGTOG.md), [audit review](PRAGTOG_AUDIT_REVIEW.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-328-pragtog-graph-and-inactive-wld-sweep),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-328-pragtog-title-and-followed-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-328-pragtog-and-two-existing-source-only-candidates),
[ud_map1 candidate row 113](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L113),
and [ud_map2 candidate row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114).


## Checkpoint 329: Prisons of Carthapia

The existing source-comprehensive dossier and repository audit now include
reset-aware arrival reach, a separate comparison with Fort Boyard prison,
and twenty explicit builder questions with recommended resolutions. Keep
active Prison DO NOT MOVE. Source review remains distinct from live access,
safety, return, and played qualification.

The 199-room WLD has 455/455 resolved exits, one reciprocal Surface pair and
35 additional one-way exits to seven zones, with two falls and no currents.
Raw directed arrival reach from #7497 is 166/199; strict initial-open reach
after 76 door resets is 5/199. The #263479 Surf approach stays source-only on
existing candidate row 97. Candidate counts and curated source-only totals
remain unchanged at 4,488 (Surf 252, ud_map1 two, ud_map2 20, Underworld2
eight). The unique normalized title and followed WLD/ZON history establish
no distinct predecessor or whole-zone replacement; current prisonb is a
separate zone with no exact room-title or full-description overlap.

The detailed source-story set remains 94; coverage maps 301/351 active
contributors, leaving 50 unmapped. The 476-data-row register now has 159
PENDING narrative, 350 connectivity, 127 title/history, 306 typed-dependency,
and 367 builder-question reviews; 109 builder-question sections are
complete. Continue with area:prisonb at CSV line 241. See [Prison coverage
line 240](ZONE_COVERAGE_REGISTER.csv#L240), [source
dossier](../zone-stories/PRISONS_OF_CARTHAPIA.md), [audit
review](PRISONS_OF_CARTHAPIA_AUDIT_REVIEW.md#checkpoint-329-builder-questions-and-recommended-resolutions),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-329-prisons-of-carthapia-reset-aware-reach-and-boundaries),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-329-prisons-of-carthapia-identity-and-followed-history),
and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-329-carthapia-retention-and-the-existing-surf-record).

## Checkpoint 330: Prison of Fort Boyard

The source-comprehensive dossier and Checkpoint 102 cross-area audit now
have a reset-aware traversal and twenty explicit builder questions with
recommended resolutions. Keep active prisonb DO NOT MOVE. Its 52-room WLD
has 108/108 resolved exits and components of 23, 22, and 7 rooms. The two
Fortb WLD arrivals have raw directed reach of 22/52 and 7/52; strict
initial-open reach after 44 door resets is 7/52 and 4/52. The external ship
lands at #43020, with raw reach 23/52 and strict initial-open reach 10/52.
These counts do not qualify live travel, switch use, or safe return.

The title occurs once among 442 current titles and 351 active contributors.
Followed history identifies no predecessor or replacement; the bounded
comparison with Carthapia found no exact normalized room-title or full-
description overlap. Keep both reciprocal Fortb boundaries and Fortb's
active reset of Prisonb ship #43014. The inactive-WLD scan found no Prisonb
approach, no attic candidate was added, and source-only totals remain 4,488.

The detailed source-story set remains 94; coverage maps 301/351 active
contributors, leaving 50 unmapped. The 476-data-row register now has 159
PENDING narrative, 350 connectivity, 127 title/history, 306 typed-dependency,
and 366 builder-question reviews; 110 builder-question sections are
complete. Continue with area:pworm at CSV line 242. See [Prisonb coverage
line 241](ZONE_COVERAGE_REGISTER.csv#L241), [dossier](../zone-stories/PRISON_OF_FORT_BOYARD.md),
[audit review](PRISON_OF_FORT_BOYARD_AUDIT_REVIEW.md#checkpoint-330-builder-questions-and-recommended-resolutions),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-330-prison-of-fort-boyard-door-state-reach),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-330-prison-of-fort-boyard-identity-and-separation),
and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-330-prisonb-in-place-retention-and-external-producer).
Next in order: [coverage line 242](ZONE_COVERAGE_REGISTER.csv#L242), area:pworm.

## Checkpoint 331: Lair of the Purple Worm

The source-comprehensive Lair dossier and Checkpoint 103 cross-area audit
now include reset-aware directed reach and twenty explicit builder questions
with recommended resolutions. Keep active pworm DO NOT MOVE. Its 98-room WLD
has 216/216 resolved exits in one weak component, with four reciprocal
Underworld/Underdark boundaries. Raw directed reach from each of the three
distinct active arrival rooms is 95/98. Strict initial-open reach after six
closed/unlocked D resets is 92/98 from #42588 and #42583, but 1/98 from
#42500. All three raw walks miss loader rooms #42594–#42596; this source
graph does not establish live loader behavior or safe travel.

The normalized title is unique among 442 current titles and 351 active
contributors. Followed WLD/ZON history shows no predecessor or replacement;
a normalized scan of pworm.wld and worms.wld found no exact room-title or
full-description overlap. Keep the four reciprocal routes and Alatorin's
nine active reset rows for local mobile #42501. Preserve the three existing
source-only ud_map2 approaches on candidate row 114; its 20 reviewed
approaches and one active Shafts reference remain unchanged. No candidate or
curated source-only total changes.

The detailed source-story set remains 94; coverage maps 301/351 active
contributors, leaving 50 unmapped. The 476-data-row register now has 159
PENDING narrative, 350 connectivity, 127 title/history, 306 typed-dependency,
and 365 builder-question reviews; 111 builder-question sections are
complete. Curated source-only totals remain 4,488 overall (Surf 252,
ud_map1 two, ud_map2 20, Underworld2 eight). Continue with area:pyramid at
CSV line 243. See [Purple Worm coverage line 242](ZONE_COVERAGE_REGISTER.csv#L242),
[dossier](../zone-stories/LAIR_OF_THE_PURPLE_WORM.md), [audit review](LAIR_OF_THE_PURPLE_WORM_AUDIT_REVIEW.md#checkpoint-331-builder-questions-and-recommended-resolutions),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-331-purple-worm-arrival-reach-and-reset-doors),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-331-lair-of-the-purple-worm-identity-and-comparison),
and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-331-purple-worm-lair-retention-and-ud-map2-evidence).
Next in order: [coverage line 243](ZONE_COVERAGE_REGISTER.csv#L243), area:pyramid.

## Checkpoint 332: Neverwind Valley

Completed the source-backed builder-question and reset-aware access review for active `pyramid`. Its 199 rooms and 555/555 exits form one weak component. Raw directed reach is 192/199 from #20400, #20466, and computed Githzerai arrival #20484; strict traversal after the 14 current D resets reaches 91/199 from each. The resets set eight closed/unlocked, one closed/locked, and five closed/secret exits. Seven raw WLD rooms (#20592–#20598) are outside those entry walks; these include reset-loaded actors. This static graph does not verify live access, fall safety, or played qualification.

Twenty explicit builder questions with recommended resolutions now accompany the dossier's ten existing source-story follow-ups. Keep Neverwind DO NOT MOVE. Preserve the three reciprocal Surface edges and the incoming-only Connector Zones edge. Surf candidate row 97 already records source-only #235767 north to #20400 and keeps 145 active references; no candidate or source-only total changes. The normalized title is unique among 442 current titles and 351 active contributors; followed WLD/ZON history shows no predecessor or replacement.

The detailed source-story set remains 94; coverage maps 301/351 active contributors, leaving 50 unmapped. The 476-data-row register has 159 PENDING narrative, 349 connectivity, 127 title/history, 306 typed-dependency, and 364 builder-question reviews; 112 builder-question sections are complete. Curated source-only totals remain 4,488 overall (Surf 252, `ud_map1` two, `ud_map2` 20, Underworld2 eight). See [coverage row 243](ZONE_COVERAGE_REGISTER.csv#L243), [dossier](../zone-stories/NEVERWIND_VALLEY.md), [audit review](NEVERWIND_VALLEY_AUDIT_REVIEW.md#checkpoint-332-builder-questions-and-recommended-resolutions), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-332-neverwind-valley-reset-aware-arrival-reach), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-332-neverwind-valley-identity-and-followed-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-332-neverwind-valley-retention-and-surf-approach). Next in order: [coverage line 244](ZONE_COVERAGE_REGISTER.csv#L244), area:quietus.

## Checkpoint 333: Quietus Quay

Completed Quietus's reset-aware WLD reach, typed dependency closure, identity/history review, and twenty builder decisions. Its 95-room WLD has 216 source exits and 214 resolving targets; two invalid `-1` exits remain in unused #1794. Raw reach from each of four active inbound roots is 91/95; strict post-reset reach is 5/95 from #1700 and 60/95 from #1723, #1736, and #1737 after 38 D resets. Preserve three reciprocal exits and the incoming-only Heavens edge.

No inactive-WLD approach, room overlap, candidate row, or curated source-only count changes. Preserve external typed consumers: 17 foreign reset placements and two shops use Quietus objects; compiled code binds quest, ship, inn, crew, ferry, portal, and Bard-kit behavior. The Quietus QST and sidecar use external objects from Ceothia, Sarmiz, and Krimman. The normalized title is unique among 442 titles, and followed history shows no predecessor or replacement. Keep Quietus DO NOT MOVE.

The detailed source-story set remains 94; coverage maps 301/351 active contributors, leaving 50 unmapped. The 476-data-row register has 159 PENDING narrative, 348 connectivity, 126 title/history, 305 typed-dependency, and 363 builder-question reviews; 113 builder-question sections are complete. Curated source-only totals remain 4,488 overall (Surf 252, `ud_map1` two, `ud_map2` 20, Underworld2 eight). See [coverage row 244](ZONE_COVERAGE_REGISTER.csv#L244), [dossier](../zone-stories/QUIETUS_QUAY.md), [audit](QUIETUS_QUAY_AUDIT_REVIEW.md#checkpoint-333-builder-questions-and-recommended-resolutions), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-333-quietus-quay-reset-aware-connectivity), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-333-quietus-quay-identity-and-followed-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-333-quietus-quay-retention-and-typed-consumers). Next in order: [coverage row 245](ZONE_COVERAGE_REGISTER.csv#L245), area:rabble.

## Checkpoint 334: Rabble

Completed Rabble's environmental-story, connectivity, identity/history, typed-dependency, retention, and builder-question review. Its six-room WLD has 11/11 resolving exits in one weak component. The reciprocal Heavens boundary enters #65100; directed raw and strict post-reset reach are 6/6 because no door resets block an exit. The all-95 inactive-WLD scan found no overlap or source-only approach.

Twenty builder questions with recommendations now accompany the new source dossier. Preserve the reciprocal Heavens boundary, the Frothe shop, and its reset list. Four imported inventory items resolve to active Heavens prototypes, with three source-only #53 definitions kept separate; reset VNUMs #6094 and #6098 have no current object prototype. Bounded scans found no external QST, SHP, sidecar, reset consumer, type-25 route, or compiled numeric reference to Rabble prototypes. Keep Rabble DO NOT MOVE. No candidate or curated source-only count changes.

The detailed source-story set is 95; coverage maps 302/351 active contributors, leaving 49 unmapped. The 476-data-row register has 158 PENDING narrative, 347 connectivity, 125 title/history, 304 typed-dependency, and 362 builder-question reviews; 114 builder-question sections are complete. Curated source-only totals remain 4,488 overall (Surf 252, `ud_map1` two, `ud_map2` 20, Underworld2 eight). See [coverage row 245](ZONE_COVERAGE_REGISTER.csv#L245), [setting dossier](../zone-stories/RABBLE.md), [audit](RABBLE_AUDIT_REVIEW.md#checkpoint-334-builder-questions-and-recommended-resolutions), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-334-rabble-reciprocal-boundary-and-six-room-reach), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-334-rabble-title-identity-and-followed-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-334-rabble-in-place-retention-and-heavens-boundary). Next in order: [coverage row 246](ZONE_COVERAGE_REGISTER.csv#L246), area:raiders.

## Checkpoint 335: Ogre Raiders

Completed the environmental-story, reset-aware connectivity, title/history, typed-dependency, retention, and builder-question review. The active area has 21 rooms and 59/59 resolving exits. Raw reach from Surface #593506 to #8900 is 21/21; strict post-reset reach is 20/21 after the fence into corral #8920 closes. Preserve the reciprocal Surface boundary and the active SurfaceKeeps consumer that places Raiders sword #8900 in sack #120028. The all-95 inactive-WLD scan found one non-overlapping source-only Surf approach, #277145 north to #8900; it is not active ingress and stays on existing candidate row 97.

The new source dossier and twenty builder questions accompany this checkpoint. Keep active raiders DO NOT MOVE. The normalized title is unique among 442 current titles and 351 active contributors; followed history shows no alternate-stem predecessor or replacement.

The detailed source-story set is 96; coverage maps 303/351 active contributors, leaving 48 unmapped. The 476-data-row register has 157 PENDING narrative, 346 connectivity, 124 title/history, 303 typed-dependency, and 361 builder-question reviews; 115 builder-question sections are complete. Curated source-only totals are 4,489 overall (Surf 253, ud_map1 two, ud_map2 20, Underworld2 eight). See [coverage row 246](ZONE_COVERAGE_REGISTER.csv#L246), [setting dossier](../zone-stories/RAIDERS.md), [audit](RAIDERS_AUDIT_REVIEW.md#checkpoint-335-builder-questions-and-recommended-resolutions), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-335-ogre-raiders-reset-aware-entry-and-fence), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-335-ogre-raiders-title-and-followed-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-335-ogre-raiders-retention-and-surf-approach). Next in order: [coverage row 247](ZONE_COVERAGE_REGISTER.csv#L247), area:random.

## Checkpoint 336: Castle Ravenloft

Reconciled Castle Ravenloft's six active arrivals with its 110 reset door states. The 269-room WLD has 689/689 resolving exits in five weak components. Each active arrival reaches 140 rooms raw; strict post-reset reach ranges from 1 to 14 rooms depending on the entry. Preserve the three reciprocal outbounds and the separate authored routes, puzzle controls, falls, and portals. The all-95 inactive-WLD scan found no overlap or approach.

The existing source-comprehensive dossier already covers all local narrative, QSTs, sidecar entries, prototypes, resets, code hooks, foreign deliveries, and proposed repair decisions. Twenty explicit builder questions now tie owner choices to the current source. Keep active ravenloft DO NOT MOVE; no candidate or area membership changes.

The detailed source-story set remains 96; coverage maps 303/351 active contributors, leaving 48 unmapped. The 476-data-row register has 157 PENDING narrative, 345 connectivity, 124 title/history, 303 typed-dependency, and 360 builder-question reviews; 116 builder-question sections are complete. Curated source-only totals remain 4,489 overall (Surf 253, ud_map1 two, ud_map2 20, Underworld2 eight). See [coverage row 248](ZONE_COVERAGE_REGISTER.csv#L248), [source dossier](../zone-stories/CASTLE_RAVENLOFT.md), [audit](RAVENLOFT_AUDIT_REVIEW.md#checkpoint-336-builder-questions-and-recommended-resolutions), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-336-castle-ravenloft-reset-aware-arrivals), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-336-castle-ravenloft-title-and-followed-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-336-castle-ravenloft-active-retention). Next in order: [coverage row 249](ZONE_COVERAGE_REGISTER.csv#L249), area:ravenloft2.

## Checkpoint 337: Ravenloft 2

Completed the Catacombs' reset-aware connectivity, title/history, typed-dependency, retention, and builder-question reconciliation against its existing comprehensive source dossier. The 400-room WLD has 961/961 resolving exits in six weak components. Castle arrivals #59057 and #59352 reach 83 and 383 rooms raw, then 47 and one room respectively after the current reset states are applied strictly. Preserve both reciprocal Castle edges, the seven one-way outbounds, current puzzle and travel controls, and the 80 fall-marked rooms; all inactive-WLD files scanned had no overlap or approach.

Twenty direct builder questions accompany the audit. Keep active `ravenloft2` DO NOT MOVE. Preserve its 37 QST exchanges, sidecar, two shops, imported reset references, Castle handprint consumer, and three separately owned type-25 declarations. The coverage row's stale shop count is corrected from zero to two. No candidate, area membership, or gameplay behavior changed, and curated source-only totals remain 4,489.

The detailed source-story set remains 96; coverage maps 303/351 active contributors, leaving 48 unmapped. The 476-data-row register has 157 PENDING narrative, 344 connectivity, 123 title/history, 302 typed-dependency, and 359 builder-question reviews; 117 builder-question sections are complete. Curated source-only totals remain 4,489 overall (Surf 253, ud_map1 two, ud_map2 20, Underworld2 eight). See [coverage row 249](ZONE_COVERAGE_REGISTER.csv#L249), [source dossier](../zone-stories/THE_RAVENLOFT_CATACOMBS.md), [audit](RAVENLOFT2_AUDIT_REVIEW.md#checkpoint-337-ravenloft-2-reset-aware-access-and-builder-decisions), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-337-ravenloft-2-reset-aware-access), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-337-ravenloft-2-title-and-bounded-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-337-ravenloft-2-in-place-retention). Next in order: [coverage row 250](ZONE_COVERAGE_REGISTER.csv#L250), area:raxthan.
## Checkpoint 338: Raxthan

Completed Raxthan's reset-aware arrival review and twenty builder questions against the existing source-comprehensive story dossier and typed-dependency audit. Its 100-room WLD has 227/227 resolving exits in one weak component. Both Surface arrivals at #42980 reach 100 rooms raw and 90 strictly after resets; the Underdark arrival at #42999 reaches 100 raw and 94 strict. Preserve three reciprocal boundaries, current door states, seven fall records, the source-only Surf approach, and existing code-owned travel.

The all-97 inactive-WLD scan confirms one source-only Surf edge, #266966 north to #42980, already counted at Checkpoint 68. It remains on Surf candidate row 97 and is not active ingress. Surf's 145 active references, reviewed source-only subset 253, and curated total 4,489 are unchanged. The coverage inventory now records the single shop #42963. Keep active raxthan DO NOT MOVE; no new candidate or gameplay change.

The detailed source-story set remains 96; coverage maps 303/351 active contributors, leaving 48 unmapped. The 476-data-row register has 157 PENDING narrative, 343 connectivity, 123 title/history, 302 typed-dependency, and 358 builder-question reviews; 118 builder-question sections are complete. Curated source-only totals remain 4,489 overall (Surf 253, ud_map1 two, ud_map2 20, Underworld2 eight). See [coverage row 250](ZONE_COVERAGE_REGISTER.csv#L250), [source dossier](../zone-stories/DRUSTLS_YERDONIA_ENSLAVED.md), [audit](YERDONIA_AUDIT_REVIEW.md#checkpoint-338-raxthan-reset-aware-access-and-builder-decisions), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-338-raxthan-reset-aware-access), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-338-drustls-yerdonia-enslaved-title-and-tracked-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-338-raxthan-active-retention-and-surf-source-only-approach). Next in order: [coverage row 251](ZONE_COVERAGE_REGISTER.csv#L251), area:realm.

## Checkpoint 339: Faerie Realm

Completed the Realm's reset-aware WLD review, current inactive-WLD sweep,
typed active and source-only dependency closure, normalized title/tracked
history review, in-place retention recommendation, and twenty builder
questions against the existing comprehensive story dossier. Its 211-room
WLD has 468 exits: 435 local and 23 resolved cross-area, with eight dropped
negative and two unresolved positive targets. From the only active arrival,
Surface #531694 to #14002, raw reach is 196/211 and strict post-reset baseline
reach is 6/211. The 22 resolved dispersal-room outbounds have no reciprocal
Realm WLD exits.

The current 446-file WLD inventory has 351 active and 95 non-active sources.
The non-active sweep found one source-only Surf approach, #244463 south to
#14002, with no room-ID overlap. It remains on Surf candidate row 97 and is
not active ingress; curated source-only totals rise to 4,490 overall (Surf
254, ud_map1 two, ud_map2 20, Underworld2 eight).

Keep realm DO NOT MOVE. The coverage inventory now records its single Fulbar
shop. Preserve the active reset and shop consumers, Alatorin's separate Fix
receipt, the two unresolved candidate references, and existing source-only
typed consumers. See [coverage row 251](ZONE_COVERAGE_REGISTER.csv#L251),
[audit review](FAERIE_REALM_AUDIT_REVIEW.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-339-faerie-realm-reset-aware-access),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-339-faerie-realm-title-and-tracked-history),
and [attic plan](ATTIC_MIGRATION_PLAN.md#checkpoint-339-faerie-realm-in-place-retention-and-dependencies).

The detailed source-story set remains 96; coverage maps 303/351 active
contributors, leaving 48 unmapped. The 476-data-row register has 157 PENDING
narrative, 342 connectivity, 122 title/history, 301 typed-dependency, and 357
builder-question reviews; 119 builder-question sections are complete.
Continue with [coverage row 252](ZONE_COVERAGE_REGISTER.csv#L252),
area:rftjngle.

## Checkpoint 340: Rift Valley Jungle

Completed reset-aware WLD reach, all current inactive-WLD approaches,
prototype and shop dependency closure, normalized title and tracked-history
review, in-place retention recommendation, and twenty builder questions
against the existing comprehensive story dossier. Its 470-room WLD has 1,229
source exits: 1,219 local, nine resolved outbound, and one positive unresolved
target. All four active arrivals are reciprocal. Each reaches 419 rooms raw and
385 under the strict 42-reset door snapshot. The five other resolved departures
are one-way: Pine Hollow and four elemental planes from load room #80444.

The current inactive-WLD sweep found no room overlap and two source-only Surf
approaches, #226279 south to #80469 and #228371 north to #80460. The latter is
not active return; Surf and Surface2011 define #228371 with distinct room
identity and topology. Preserve both approaches on Surf row 97 and the
unresolved active edge without selecting an owner by VNUM. Curated source-only
totals rise to 4,492 overall (Surf 256, ud_map1 two, ud_map2 20, Underworld2
eight). The coverage inventory now records all eight local SHP records.

Keep rftjngle DO NOT MOVE. Preserve two active-shop and shared CHAOS uses of
object #80186 and the two inactive Nexus56 shop references to #80074/#80002.
See [coverage row 252](ZONE_COVERAGE_REGISTER.csv#L252),
[story dossier](../zone-stories/RFTJNGLE.md),
[audit review](RFTJNGLE_AUDIT_REVIEW.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-340-rift-valley-jungle-reset-aware-connectivity),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-340-rift-valley-jungle-title-and-tracked-history),
and [attic plan](ATTIC_MIGRATION_PLAN.md#checkpoint-340-rift-valley-jungle-in-place-retention-and-dependencies).

The detailed source-story set remains 96; coverage maps 303/351 active
contributors, leaving 48 unmapped. The 476-data-row register has 157 PENDING
narrative, 341 connectivity, 121 title/history, 300 typed-dependency, and 356
builder-question reviews; 120 builder-question sections are complete.
Continue with [coverage row 253](ZONE_COVERAGE_REGISTER.csv#L253),
area:river_styx.

## Checkpoint 341: River Styx

Completed the River Styx source-story dossier and row 253 audit. The 100-room
WLD has 194 resolving exits, 193 local plus one outbound, in four weak
components. From dock #90700 the ordinary directed graph reaches 96 rooms;
strict reach after six chance-100 door resets is 92. No active or inactive
source WLD enters or duplicates a River Styx room. The area also has 71 current
fields and no fall chances.

The one-way exit leads to Hell One. Local portals connect the fountain hall to
four rooms beyond ordinary WLD reach. Room #90740 has a north self-loop, while
#90796 describes a swamp to the south without an authored south exit. Preserve
those builder decisions and the one-way boundary for review. The current
inactive-WLD sweep adds no candidate/source-only references; curated totals
remain 4,492 overall (Surf 256, ud_map1 two, ud_map2 20, Underworld2 eight).

Typed closure records 205 active external ZON mobile references: 193 M loads
and 12 F follow rows in Avernus, Hell One, and Ravenloft 2. No external QST
item, SHP product, or story-sidecar reference uses River Styx VNUMs. No outside
type-25 target uses a River Styx room. The local #90708 Charon and
bone-horn inscription are distinct from the compiled #60008/#60003 spectral
ferry initialized on a Surface route. Keep active `river_styx` DO NOT MOVE;
add no attic candidate or candidate-specific migration plan. The
[source dossier](../zone-stories/RIVER_STYX.md) contains twenty builder
questions and recommendations; the [audit supplement](RIVER_STYX_AUDIT_REVIEW.md)
provides source evidence.

The detailed source-story set rises to 97; coverage maps 304/351 active
contributors, leaving 47 unmapped. The 476-data-row register has 156 PENDING
narrative, 340 connectivity, 120 title/history, 299 typed-dependency, and 355
builder-question reviews; 121 builder-question sections are complete. See
[coverage row 253](ZONE_COVERAGE_REGISTER.csv#L253),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-341-river-styx-static-connectivity),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-341-river-styx-title-duplicate-prototypes-and-tracked-history),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-341-river-styx-in-place-retention-and-shared-mobiles).

## Checkpoint 342: Rogue Plains

The existing Rogue Plains dossier already mapped all rooms, mobiles, objects,
local quest exchanges, dialogue, and resets. This checkpoint adds the
reset-aware whole-world connectivity and typed ownership closure, completes
title/history review, and adds twenty explicit builder questions. It also
corrects the dossier's Windtalker statement: #75951 is a M-reset room with no
incoming WLD edge. Seven rooms titled “Unnamed” have no outgoing exits, and
#75955 is isolated.

The all-95-inactive-WLD sweep found no Rogue Plains VNUM overlap and one
non-overlapping source-only surf.wld approach (#201299 north to #75801).
Preserve it on existing Surf candidate row 97; it is not active ingress. The
unresolved Rogue north exit to #170047 remains under Duris3 candidate row 36.
Keep active roguerai DO NOT MOVE and add no candidate-specific plan. Curated
source-only totals rise to 4,493 overall (Surf 257, ud_map1 two, ud_map2 20,
Underworld2 eight).

The detailed source-story set remains 97; coverage maps 304/351 active
contributors, leaving 47 unmapped. The 476-data-row register has 156 PENDING
narrative, 339 connectivity, 120 title/history, 298 typed-dependency, and 354
builder-question reviews; 122 builder-question sections are complete. See
[coverage row 254](ZONE_COVERAGE_REGISTER.csv#L254),
[source dossier](../zone-stories/ROGUE_PLAINS.md),
[audit review](ROGUE_PLAINS_AUDIT_REVIEW.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-342-rogue-plains-reset-aware-connectivity),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-342-rogue-plains-title-and-followed-history),
[attic plan](ATTIC_MIGRATION_PLAN.md#checkpoint-342-rogue-plains-in-place-retention-and-surf-approach),
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and
[Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

## Checkpoint 343: Tunnels of Payang

Checkpoint 343 completes the rtun audit at coverage row 255. Review counts move to 338 connectivity, 119 title/history, 297 typed-dependency, and 353 builder-question reviews; 123 builder-question sections are complete. The narrative pending count remains 156; detailed story coverage remains 97 dossiers and full-area mapping remains 304/351. See [RTUN audit](RTUN_AUDIT_REVIEW.md), [coverage row 255](ZONE_COVERAGE_REGISTER.csv#L255), and [RTUN story dossier](RTUN_STORY_REVIEW.md).

The inactive-WLD sweep adds two non-overlapping source-only surf.wld approaches into rtun. Preserve them on existing Surf candidate row 97; Surf's reviewed source-only subset is 259, curated source-only total is 4,495, and Surf's 145 active references are unchanged. Continue with coverage row 256, area:ruins.

## Checkpoint 344: Village of Refugees

Checkpoint 344 completes the row 256 audit. Review counts move to 337 connectivity, 118 title/history, 296 typed-dependency, and 352 builder-question reviews; 124 builder-question sections are complete. The narrative pending count remains 156; mapped active contributors remain 304/351 and the detailed source-story set remains 97. See [Village of Refugees audit](VILLAGE_OF_REFUGEES_AUDIT_REVIEW.md#checkpoint-344-reset-aware-reach-refreshed-source-closure-and-builder-decisions) and [coverage row 256](ZONE_COVERAGE_REGISTER.csv#L256).

No new inactive-WLD approach was found; Village of Refugees' surf.wld #264438 edge was already counted. Curated source-only totals remain 4,495 overall (Surf 259, ud_map1 two, ud_map2 20, Underworld2 eight). Surf active references remain 145. Continue with coverage row 257, area:sarmiz.

## Checkpoint 345: Sarmiz'Duul

Checkpoint 345 completes the row 257 audit and corrects its shop inventory to
four source records (#9413–#9416). All five active arrivals share all 583 rooms
in the raw reach/return graph; conservative closed-door traversal leaves 249
rooms from #9400, six from #9853, 18 from #9975, and 249 from #9597 and #9401.
The three inbound-only/source distinctions and 50 reset-controlled door
records are documented in the [Sarmiz audit](SARMIZ_AUDIT_REVIEW.md). The
three new source-only Surf approaches are preserved on existing candidate row
97; the reviewed Surf subset rises to 262 and curated source-only totals rise
to 4,498 overall. Surf's 145 active references are unchanged.

The detailed story set remains 97 and mapped active contributors remain
304/351. The 476-data-row register now has 156 narrative, 336 connectivity,
117 title/history, 295 typed-dependency, and 351 builder-question reviews
pending; 125 builder-question sections are complete. See [coverage row
257](ZONE_COVERAGE_REGISTER.csv#L257), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-345-sarmizduul-reset-aware-connectivity),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-345-sarmizduul-title-duplicates-and-followed-history),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-345-sarmizduul-in-place-retention-and-surf-approaches).

Continue with [coverage row 258](ZONE_COVERAGE_REGISTER.csv#L258), area:savannah.

Checkpoint 346 completes Savannah's static connectivity, typed-dependency,
duplicate/history, and builder-question reviews. Its reciprocal Surface
boundary and three reset-closed secret pairs are recorded; inactive Duris3
shares all 167 room VNUMs, and 668 target-matching edges are retained as
identity ambiguous on candidate row 36 rather than counted as approaches.
Curated source-only totals remain 4,498 (Surf 262); Surf's 145 active
references are unchanged. The 476-data-row register now has 156 narrative,
335 connectivity, 116 title/history, 294 typed-dependency, and 350
builder-question reviews pending; 126 builder-question sections are complete.
See [coverage row 258](ZONE_COVERAGE_REGISTER.csv#L258),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-346-savannah-reset-aware-connectivity-and-source-identity),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-346-savannah-title-room-number-overlap-and-followed-history),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-346-savannah-in-place-retention).

Continue with [coverage row 259](ZONE_COVERAGE_REGISTER.csv#L259), area:scorchvalley.

Checkpoint 347 completes Scorched Valley's static connectivity, typed
dependencies, title/history, and builder questions. Its only ordinary
reciprocal boundary is the Surface route; three other inbounds and three
outbounds are load/holding routes. Preserve the single source-only
`surf.wld` approach #280633 west to #71201 on existing candidate row 97.
Curated source-only totals rise to 4,499 overall (Surf 263); Surf's 145 active
references are unchanged. The 476-data-row register now has 156 narrative,
334 connectivity, 115 title/history, 293 typed-dependency, and 349
builder-question reviews pending; 127 builder-question sections are complete.
See [coverage row 259](ZONE_COVERAGE_REGISTER.csv#L259),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-347-scorched-valley-reset-aware-connectivity-and-surf-approach),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-347-scorched-valley-title-duplicates-and-followed-history),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-347-scorched-valley-retention-and-surf-approach).

Continue with [coverage row 260](ZONE_COVERAGE_REGISTER.csv#L260), area:sea.

Checkpoint 348 completes Sea's static graph and reset-aware reach, typed
source dependencies, title/history, and twenty builder questions. Three
reciprocal surf.wld records into Sea were already included in the existing
source-only subset; preserve them without changing Surf 263 or the curated
global total of 4,499. The same-numbered surface2011.wld records are distinct
Storm Sea identities, not additional approaches. Keep Sea DO NOT MOVE.

The detailed story set remains 97 and mapped active contributors remain
304/351. The 476-data-row register now has 156 narrative, 333 connectivity,
115 title/history, 292 typed-dependency, and 348 builder-question reviews
pending; 128 builder-question sections are complete. See [coverage row
260](ZONE_COVERAGE_REGISTER.csv#L260), [Sea audit](SEA_AUDIT_REVIEW.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-348-sea-reset-aware-connectivity-and-inactive-map-identity),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-348-sea-title-vnum-identity-and-followed-history),
and [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-348-sea-in-place-retention-and-candidate-identities).

## Checkpoint 349: The Seacaves

Coverage row 261 adds the source dossier and completes Sea Caves' static
graph, reset-aware reach, typed dependencies, title/history, and twenty
builder questions. The active Surface boundary is reciprocal. The inactive
WLD sweep found no room overlap and one non-overlapping source-only Surf
approach, #212716 west to #2200; preserve it on existing Surf candidate row
97 as source-only, not ingress. Surf rises to 264 reviewed source-only
records and the curated total to 4,500; its 145 active references are
unchanged. Keep seacaves DO NOT MOVE.

The detailed source-story set is 98 and mapped active contributors are
305/351 (46 unmapped). The 476-data-row register has 155 narrative, 332
connectivity, 114 title/history, 291 typed-dependency, and 347 builder-question
reviews pending; 129 builder-question sections are complete. See [coverage
row 261](ZONE_COVERAGE_REGISTER.csv#L261), [source dossier](../zone-stories/THE_SEACAVES.md),
[audit](SEACAVES_AUDIT_REVIEW.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-349-sea-caves-reset-aware-connectivity-and-inactive-map-identity),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-349-the-seacaves-title-vnum-identity-and-followed-history),
and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-349-sea-caves-in-place-retention-and-candidate-approach).

## Checkpoint 350: Sea Kingdom

Coverage row 262 completes Sea Kingdom's reset-aware graph, typed dependency
closure, title and replacement review, and twenty builder questions. Its
225-room map has five weak components, 20 100%-chance D resets, four
unmatched EX_LOCKED-only exit directions, two active inbounds, and one
reciprocal Plane Water outbound. The all-inactive-WLD scan finds no overlap or
approach. Keep seakngdm DO NOT MOVE; no candidate row is added. Surf remains
at 264 reviewed source-only records and the curated global total remains
4,500.

The detailed source-story set remains 98 and mapped active contributors remain
305/351 (46 unmapped). The 476-data-row register now has 155 narrative, 331
connectivity, 113 title/history, 290 typed-dependency, and 346
builder-question reviews pending; 130 builder-question sections are complete.
See [coverage row 262](ZONE_COVERAGE_REGISTER.csv#L262), [source dossier](../zone-stories/SEAKNGDM.md),
[audit](SEAKNGDM_AUDIT_REVIEW.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-350-sea-kingdom-reset-aware-connectivity-and-inactive-map-identity),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-350-sea-kingdom-title-duplicates-and-followed-history),
and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-350-sea-kingdom-in-place-retention).

## Checkpoint 351: Sevenoaks

Coverage row 263 completes Sevenoaks' source-story map, active and reset-aware
connectivity, exact typed dependencies, title/followed-history review, and
twenty source-backed builder questions. The active graph has 77 rooms and
222 resolved exits in seven weak components (31/14/9/8/5/5/5). Surface room
#516252 has a reciprocal edge to #38600; White Harbor #55632 enters vault
room #38632 without a Sevenoaks reverse edge. The all-95 inactive-WLD scan
adds two source-only approaches into #38600, from Surf and Underworld2.
Preserve both on their existing candidate rows. Surf rises to 265 reviewed
source-only records, Underworld2 to nine, and the curated total to 4,502.
Keep active `seveno` DO NOT MOVE.

The detailed story set is now 99 and mapped active contributors are 306/351
(45 unmapped). The 476-data-row register has 154 narrative, 330 connectivity,
112 title/history, 289 typed-dependency and 345 builder-question reviews
pending; 131 builder-question sections are complete. See [coverage row
263](ZONE_COVERAGE_REGISTER.csv#L263), [source dossier](../zone-stories/SEVENOAKS.md),
[audit](SEVENO_AUDIT_REVIEW.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-351-sevenoaks-reset-aware-connectivity-and-inactive-map-identity),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-351-sevenoaks-title-vnum-identity-and-followed-history),
and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-351-sevenoaks-in-place-retention-and-candidate-approaches).

## Checkpoint 352: Arbre's Forest

Coverage row 264 completes Arbre's Forest's authored story, active graph,
typed dependency and replacement-history review, plus twenty source-backed
builder decisions. The active 24-room graph has 48 resolved exits and one
strongly connected component. Both active Surface edges are reciprocal. The
all-95 inactive-WLD scan adds two distinct source-only approaches from Surf;
preserve them on Surf's existing candidate row. Surf rises to 267 reviewed
source-only records and the curated total to 4,504. Keep active `sforest`
DO NOT MOVE.

The detailed story set is 100 and mapped active contributors are 307/351
(44 unmapped). The 476-data-row register has 153 narrative, 329 connectivity,
111 title/history, 288 typed-dependency and 344 builder-question reviews
pending; 132 builder-question sections are complete. See [coverage row
264](ZONE_COVERAGE_REGISTER.csv#L264), [source dossier](../zone-stories/SFOREST.md),
[audit](SFOREST_AUDIT_REVIEW.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-352-arbres-forest-reset-aware-connectivity-and-inactive-map-identity),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-352-arbres-forest-title-vnum-identity-and-followed-history),
and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-352-arbres-forest-in-place-retention-and-candidate-approaches).

Continue with [coverage row 265](ZONE_COVERAGE_REGISTER.csv#L265), area:shabo.

## Checkpoint 353: The Great Shaboath

Coverage row 265 completes the existing comprehensive Shaboath story dossier’s repository-wide graph, typed dependency, title/history, and builder-question reconciliation. The 116-room graph has 243 resolved exits, a strongly connected 115-room portion and isolated Control Room #32800. Its 64 flagged directions match 64 100%-chance D resets. Three active inbounds and one outbound include a reciprocal Underdark pair. The all-95 inactive-WLD sweep found no overlap or approach, so no candidate row is added. Keep active shabo DO NOT MOVE.

The detailed story set remains 100 and mapped active contributors remain 307/351 (44 unmapped). The 476-data-row register has 153 narrative, 328 connectivity, 110 title/history, 287 typed-dependency and 343 builder-question reviews pending; 133 builder-question sections are complete. See [coverage row 265](ZONE_COVERAGE_REGISTER.csv#L265), [source dossier](../zone-stories/THE_GREAT_SHABOATH.md), [audit](GREAT_SHABOATH_AUDIT_REVIEW.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-353-the-great-shaboath-reset-aware-connectivity-and-inactive-map-identity), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-353-the-great-shaboath-title-vnum-identity-and-followed-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-353-the-great-shaboath-in-place-retention-and-dependencies).

Continue with [coverage row 266](ZONE_COVERAGE_REGISTER.csv#L266), area:shadamehr_keep.

## Checkpoint 354: Shadamehr Keep

Coverage row 266 and the new source dossier complete Shadamehr Keep's environmental story, active graph, typed dependencies, title/history, and twenty builder-question recommendations. Its 25-room graph has 57 resolved exits in one strongly connected component and a reciprocal Dreggan Woods boundary. Six flagged door directions match six 100%-chance D resets. External dependencies include Dreggan Woods, Heavens, Unique, Alatorin, and Barovia; the unresolved object #41002 reset remains explicit. No inactive-WLD approach or overlap was found. Keep active shadamehr_keep DO NOT MOVE.

The detailed story set is 101 and mapped active contributors are 308/351 (43 unmapped). The 476-data-row register has 152 narrative, 327 connectivity, 109 title/history, 286 typed-dependency and 342 builder-question reviews pending; 134 builder-question sections are complete. See [coverage row 266](ZONE_COVERAGE_REGISTER.csv#L266), [source dossier](../zone-stories/SHADAMEHR_KEEP.md), [audit](SHADAMEHR_KEEP_AUDIT_REVIEW.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-354-shadamehr-keep-reset-aware-connectivity-and-inactive-map-identity), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-354-shadamehr-keep-title-vnum-identity-and-followed-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-354-shadamehr-keep-in-place-retention-and-external-item-dependencies).

Continue with [coverage row 267](ZONE_COVERAGE_REGISTER.csv#L267), area:shadowcl.

## Checkpoint 355: Shadowclave

Coverage row 267 and the new Shadowclave dossier complete the forest/cavern environmental story, active graph, shop inventory, typed external transactions, title/history, and twenty builder decisions. The 53-room graph has 108 exits with one negative target; three active reciprocal pairs connect Surface and one conditional Githzerai shift can arrive at #93919. Twenty flagged directions match 100%-chance D resets. Two inactive Surf approaches enter #93900 and #93952; preserve them with Surf. Keep active shadowcl DO NOT MOVE.

The detailed story set is 102 and mapped active contributors are 309/351 (42 unmapped). The 476-data-row register has 151 narrative, 326 connectivity, 108 title/history, 285 typed-dependency and 341 builder-question reviews pending; 135 builder-question sections are complete. See [coverage row 267](ZONE_COVERAGE_REGISTER.csv#L267), [source dossier](../zone-stories/SHADOWCLAVE.md), [audit](SHADOWCL_AUDIT_REVIEW.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-355-shadowclave-connectivity-scripted-arrival-and-inactive-map-identity), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-355-shadowclave-title-vnum-identity-and-followed-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-355-shadowclave-in-place-retention-and-cross-zone-dependencies).

Continue with [coverage row 268](ZONE_COVERAGE_REGISTER.csv#L268), area:sham.

### Checkpoint 356: Shady Grove builder-question coverage

Physical coverage line 268 (`area:shady`) already had the comprehensive story, connectivity, title/history, and typed-dependency evidence recorded at checkpoint 98. Checkpoint 356 verifies its three explicit builder questions and recommended resolutions; the review field is complete while builder decisions remain open. Builder-question reviews pending: 340 of 476; completed sections: 136. The previous continuation pointer to `area:sham` was stale: the next physical entry is coverage line 269, `area:shafts`. See [coverage row 268](ZONE_COVERAGE_REGISTER.csv#L268), [Shady Grove audit supplement](SHADY_GROVE_AUDIT_REVIEW.md#checkpoint-356-builder-question-coverage-reconciliation), and [coverage row 269](ZONE_COVERAGE_REGISTER.csv#L269).

## Checkpoint 357: Underworld Shafts

Physical coverage line 269, area:shafts, now has a source-comprehensive dossier, static reachability and hazard review, unique-title/history finding, typed dependency closure, and 20 builder questions with recommendations. The six active reciprocal world pairs remain distinct; four active exits into unregistered map sources and ten inactive map approaches are linked to candidate rows 112-114. The detailed story-review set is 103 and mapped active contributors are 310/351 (41 unmapped). The 476-data-row register now has 150 narrative, 325 connectivity, 107 title/history, 284 typed-dependency, and 339 builder-question reviews pending; 137 builder-question sections are complete. Curated source-only WLD total: 4,516 (Surf 269, ud_map2 reviewed subset 23, Underworld2 9). Continue at [coverage row 270](ZONE_COVERAGE_REGISTER.csv#L270), area:shaughin. See [coverage row 269](ZONE_COVERAGE_REGISTER.csv#L269), [Shafts story dossier](../zone-stories/SHAFTS.md), [audit supplement](SHAFTS_AUDIT_REVIEW.md), [connectivity report](WORLD_CONNECTIVITY.md), [title/history report](DUPLICATE_AND_REPLACED_ZONES.md), and [migration plan](ATTIC_MIGRATION_PLAN.md).

## Checkpoint 358: Shaughin Settlement

Physical coverage line 270 already had a source-comprehensive dossier and checkpoint 131 graph, title/history, and typed-dependency findings. Checkpoint 358 verifies 20 source-backed builder questions and recommended resolutions; answers remain open. The detailed story-review set remains 103 and mapped active contributors 310/351 (41 unmapped). Across 476 rows, pending reviews remain narrative 150, connectivity 325, title/history 107, typed dependency 284, and builder questions now 338; 138 builder-question sections are complete. Curated source-only WLD total remains 4,516. Continue at [coverage row 271](ZONE_COVERAGE_REGISTER.csv#L271), area:shaughin2. See [coverage row 270](ZONE_COVERAGE_REGISTER.csv#L270), [story dossier](../zone-stories/SHAUGHIN_SETTLEMENT.md), and [audit supplement](SHAUGHIN_AUDIT_REVIEW.md).

## Checkpoint 359: Shaughin2 Hunting Grounds

Physical coverage line 271 now has its own lower-zone story dossier, complete raw graph and four-boundary review, title/history finding, typed links to upper Shaughin contracts/portals/workers, active Alatorin resets of lower campfire #6603, and a Tharnadia1 shop-source listing of lower scenery #6609, plus twenty builder questions with recommendations. Its 100-room local WLD is strongly connected; four active reciprocal boundaries connect to upper Shaughin and the Underdark. One inactive Surf approach is preserved on existing candidate row 97.

The detailed story-review set is 104 and mapped active contributors are 311/351 (40 unmapped). Across 476 rows, pending reviews are narrative 149, connectivity 324, title/history 107, typed dependency 284, and builder questions 337; 139 builder-question sections are complete. Curated source-only WLD total is 4,517 (Surf 270, ud_map2 reviewed subset 23, Underworld2 9). Continue at [coverage row 272](ZONE_COVERAGE_REGISTER.csv#L272), area:ship. See [coverage row 271](ZONE_COVERAGE_REGISTER.csv#L271), [Shaughin2 dossier](../zone-stories/SHAUGHIN2_HUNTING_GROUNDS.md), [audit](SHAUGHIN2_AUDIT_REVIEW.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-359-shaughin2-hunting-grounds-connectivity), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-359-shaughin2-hunting-grounds-title-and-tracked-history), [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-359-shaughin2-and-the-inactive-surf-approach), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

## Checkpoint 360: Ship runtime area

Coverage row 272 and its new runtime-room dossier complete the static source review for `area:ship`: all 5,000 fixed room records, the three code-reserved rooms and 4,997 dynamic interior slots, local mobile/object prototype roles, title/history, typed dependencies, and twenty builder questions. The static WLD has no exits or inactive-map approaches; native ship code reconstructs exits for allocated saved layouts. Preserve the compile-time VNUM contracts and keep ship separate from `shipy`; no migration or live-route conclusion is implied.

The detailed story-review set is 105 and mapped active contributors are 312/351 (39 unmapped). Across 476 rows, pending reviews are narrative 148, connectivity 323, title/history 106, typed dependency 283, and builder questions 336; 140 builder-question sections are complete. Curated source-only WLD total remains 4,517 (Surf 270, ud_map2 subset 23, Underworld2 9). Continue at [coverage row 273](ZONE_COVERAGE_REGISTER.csv#L273), area:shipy. See [coverage row 272](ZONE_COVERAGE_REGISTER.csv#L272), [runtime-room dossier](../zone-stories/SHIP_RUNTIME_AREA.md), [audit](SHIP_AREA_AUDIT_REVIEW.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-360-ship-runtime-pool-and-fixed-ferry-rooms), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-360-ship-title-fixed-vnum-identity-and-followed-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-360-ship-runtime-pool-retention).

## Checkpoint 361: Ship Yards

Coverage row 273 is now complete across the existing Ship Yards source dossier, active graph, title and bounded history, typed cross-zone dependencies, and twenty builder questions. Its 229-room graph has 606 exits, 32 resolved active cross-area departures, ten active inbounds, and four positive unresolved targets. All 70 flagged directions match 100%-chance resets. The all-95 inactive-map sweep found four non-overlapping Aopal approaches and eight Surf approaches; preserve them on existing candidate rows 10 and 97. Twenty-eight additional Aopal room headers collide numerically with shipy and remain separate inactive-map evidence. Keep `shipy` DO NOT MOVE.

The detailed story-review set remains 105 and mapped active contributors remain 312/351 (39 unmapped), because Ship Yards already had its source-comprehensive dossier. Across 476 rows, pending reviews are narrative 147, connectivity 322, title/history 105, typed dependency 282, and builder questions 335; 141 builder-question sections are complete. The curated source-only total is 4,529; the reviewed Surf subset is 278 (ud_map2 subset 23 and Underworld2 9 remain unchanged). Continue at [coverage row 274](ZONE_COVERAGE_REGISTER.csv#L274), area:shortb. See [coverage row 273](ZONE_COVERAGE_REGISTER.csv#L273), [Ship Yards audit](SHIP_YARDS_AUDIT_REVIEW.md), [story dossier](../zone-stories/SHIP_YARDS.md), [connectivity checkpoint 361](WORLD_CONNECTIVITY.md#checkpoint-361-ship-yards-static-connectivity-and-inactive-map-identity), [title/history checkpoint 361](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-361-ship-yards-title-history-and-aopal-identity), and [retention checkpoint 361](ATTIC_MIGRATION_PLAN.md#checkpoint-361-ship-yards-in-place-retention-and-inactive-approaches).

## Checkpoint 362: The Undead Fog

Coverage row 274 and the new source dossier complete the map, narrative, title/history, typed dependency, and builder-question review for `area:shortb`. Its 13 rooms have 87 exits, including two reciprocal Surface boundary pairs; twelve fog rooms form the traversable authored component, while Web room #67809 connects only through the local portal objects. Every exit flag and key is zero; there are no door resets or static fall/current markers. The all-95 inactive sweep found three source-only Surf approaches and no VNUM overlap; keep them on existing candidate row 97. Keep active shortb DO NOT MOVE.

The detailed story-review set is 106 and mapped active contributors are 313/351 (38 unmapped). Across 476 rows, pending reviews are narrative 146, connectivity 321, title/history 104, typed dependency 281, and builder questions 334; 142 builder-question sections are complete. The curated source-only total is 4,532, with the reviewed Surf subset at 281 (ud_map2 subset 23 and Underworld2 9 unchanged). Continue at next pending [coverage row 276](ZONE_COVERAGE_REGISTER.csv#L276), area:skulldrach; row 275, shortc, was already reviewed at Checkpoint 48. See [coverage row 274](ZONE_COVERAGE_REGISTER.csv#L274), [Undead Fog dossier](../zone-stories/THE_UNDEAD_FOG.md), [audit](SHORTB_AUDIT_REVIEW.md), [connectivity checkpoint 362](WORLD_CONNECTIVITY.md#checkpoint-362-the-undead-fog-connectivity-and-inactive-approaches), [title/history checkpoint 362](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-362-the-undead-fog-title-vnum-identity-and-history), and [retention checkpoint 362](ATTIC_MIGRATION_PLAN.md#checkpoint-362-undead-fog-in-place-retention).

## Checkpoint 363: Skulldrach

Coverage row 276 and the new source map complete narrative, title/history, connectivity, typed dependency, and builder-question review for area:skulldrach. Its 252 rooms contain 691 exits, including reciprocal Citadel and Surface pairs, three negative targets, and a separate 27-room component. One inactive Surf edge reaches room #12859; preserve it on existing candidate row 97. Keep the active source in place and preserve the golem/shield dependency used by Bastine.

The detailed story-review set is now 107 and mapped active contributors are 314/351 (37 unmapped). Across 476 rows, pending reviews are narrative 145, connectivity 320, title/history 103, typed dependency 280, and builder questions 333; 143 builder-question sections are complete. The curated source-only total is 4,533, with the reviewed Surf subset at 282 (ud_map2 subset 23 and Underworld2 9 remain unchanged). Continue at next pending [coverage row 277](ZONE_COVERAGE_REGISTER.csv#L277), area:smoke. See [coverage row 276](ZONE_COVERAGE_REGISTER.csv#L276), [Skulldrach dossier](../zone-stories/SKULLDRACH.md), [audit](SKULLDRACH_AUDIT_REVIEW.md), [connectivity checkpoint 363](WORLD_CONNECTIVITY.md#checkpoint-363-skulldrach-connectivity-and-inactive-approaches), [title/history checkpoint 363](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-363-skulldrach-title-vnum-identity-and-history), and [retention checkpoint 363](ATTIC_MIGRATION_PLAN.md#checkpoint-363-skulldrach-in-place-retention-and-typed-consumers).

## Checkpoint 364: Para-Elemental Plane of Smoke

Physical coverage line 277 is complete for Smoke's graph, typed references, title/history, and twenty source-backed builder recommendations. The existing comprehensive story dossier remains in place. Keep active smoke DO NOT MOVE. Preserve its two foreign fixed portal destinations, Chaos starter-kit object references, four flagged/reset directions, and the #139818 vault-key relationship. The inactive Duris3 and forofcon maps reuse Smoke room VNUMs under different titles; preserve 310 + five unique-source directions on their existing candidate rows, not as active ingress. Their 302 + 106 overlap-source directions stay owner-conflicted. The curated source-only WLD total rises by 315 to 4,848; Surf remains at 282.

The detailed story set remains 107 and mapped active contributors remain 314/351. Across 476 data rows, pending reviews are narrative 145, connectivity 319, title/history 102, typed dependency 279, and builder questions 332; 144 builder-question sections are complete. **Resume at physical coverage line 278, `area:smokev`.** See [coverage row 277](ZONE_COVERAGE_REGISTER.csv#L277), [Smoke dossier](../zone-stories/PARA_ELEMENTAL_PLANE_OF_SMOKE.md), [audit](SMOKE_AUDIT_REVIEW.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-364-para-elemental-plane-of-smoke-connectivity-and-inactive-approaches), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-364-para-elemental-plane-of-smoke-title-identity-and-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-364-smoke-in-place-retention-and-typed-dependencies).

## Checkpoint 365: Shairak and Smokeveil Forest

Physical coverage line 278 is complete for Smokeveil's active graph, typed dependencies, title/history, and twenty source-backed builder recommendations. The existing comprehensive story map remains in place. The source contains one Otsurd shop, record #20234 with six products; the coverage inventory's previous zero-shop count is corrected. Keep active smokev DO NOT MOVE. Preserve the reciprocal Surface pairs, three state-4 directions with no D reset, the Alatorin Tentabeast-heart consumer, external ZON/shop references, and type-aware Chaos/Dorno/Keebo code dependencies. One Surf direction, #263026 west to #20201, remains source-only on candidate row 97. Surf rises to 283 reviewed records and the curated source-only total to 4,849.

The detailed story set remains 107 and mapped active contributors remain 314/351. Across 476 data rows, pending reviews are narrative 145, connectivity 318, title/history 101, typed dependency 278, and builder questions 331; 145 builder-question sections are complete. **Resume at physical coverage line 279, `area:snogres`.** See [coverage row 278](ZONE_COVERAGE_REGISTER.csv#L278), [Smokeveil story map](../zone-stories/SHAIRAK_AND_SMOKEVEIL_FOREST.md), [audit](SMOKEV_AUDIT_REVIEW.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-365-shairak-and-smokeveil-forest-connectivity-and-inactive-approach), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-365-smokeveil-title-same-type-identity-and-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-365-smokeveil-in-place-retention-and-external-dependencies).

## Checkpoint 366: Valley of the Snow Ogres

Physical coverage line 279 completes the existing Snow Ogres story map's repository-wide graph, typed-dependency, title/history, and twenty-question builder reconciliation. The 100-room graph has 281 resolved directions in one weak component; authored traversal from #87700 reaches all rooms. Surface is a reciprocal boundary, and Faerie Realm #14210 enters the smithy without a Snow Ogres reverse edge. Three flag-4 directions have no D reset; four other flagged directions match exact 100-percent resets. The inactive-WLD sweep found no overlap or approach. Keep `snogres` DO NOT MOVE.

Typed closure records five active foreign QST object consumers, Surface's Leppts mobile reset, two local reset references to active Heavens objects, and source-only brass-old-1's reset reference to local hide #87725. The legacy reset is preserved on candidate row 17, not counted as active supply. Title is unique among 442 current ZON titles and 351 active contributors; no same-type overlap, predecessor, or replacement evidence was found. Curated source-only WLD total remains 4,849 (Surf 283).

The detailed story set remains 107 and mapped active contributors remain 314/351. Across 476 data rows, pending reviews are narrative 145, connectivity 317, title/history 100, typed dependency 277, and builder questions 330; 146 builder-question sections are complete. **Continue at physical coverage line 280, `area:solonar`.** See [coverage row 279](ZONE_COVERAGE_REGISTER.csv#L279), [Snow Ogres dossier](../zone-stories/VALLEY_OF_THE_SNOW_OGRES.md), [audit](SNOGRES_AUDIT_REVIEW.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-366-valley-of-the-snow-ogres-connectivity-and-inactive-approach), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-366-snow-ogres-title-same-type-identity-and-followed-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-366-snow-ogres-in-place-retention-and-typed-dependencies).

## Checkpoint 367: Vast Hidden Grove

Physical coverage line 280 completes the existing Solonar story map's static graph, typed dependencies, title/history, and twenty-question builder review. The 100-room WLD has 182 resolved local directions in seven weak components (33/26/20/10/5/3/3), with no active or inactive WLD approach. Scenery item travel remains separate: the Elvish statue at Homestead #35810 enters Solonar #30648 and local statue #30604 returns to Homestead. The stale shop count is corrected to seven local shops, #30600–#30606. Alatorin resets local mobile #30611; its chain has no filament G reset.

The normalized title is unique among 442 current ZON titles and 351 active contributors. Same-type source overlap and replacement evidence are absent. The missing `inn` target #30511 remains a checked-target repair decision; no room is inferred from the derived span. No external QST, sidecar item, or SHP product reference to local objects was found. Curated source-only WLD total remains 4,849 (Surf 283).

The detailed story set remains 107 and mapped active contributors remain 314/351. Across 476 data rows, pending reviews are narrative 145, connectivity 316, title/history 99, typed dependency 276, and builder questions 329; 147 builder-question sections are complete. **Continue at physical coverage line 281, `area:split_shield`.** See [coverage row 280](ZONE_COVERAGE_REGISTER.csv#L280), [Vast Hidden Grove dossier](../zone-stories/VAST_HIDDEN_GROVE.md), [audit](SOLONAR_AUDIT_REVIEW.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-367-vast-hidden-grove-connectivity-and-scenery-travel), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-367-solonar-title-same-type-identity-and-followed-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-367-solonar-in-place-retention-and-cross-area-routes).

## Checkpoint 368: Village of Split Shield

Physical coverage line 281 and the new source dossier complete the settlement narrative, active graph, typed dependencies, title/history, and twenty builder-question reviews. The 99-room active graph has 213 resolved directions in one weak component; five active directions enter, four leave, and four reciprocal pairs include three Surface routes plus shortc. White Harbor supplies a separate one-way inbound to #10307. The inactive-WLD sweep adds two Surf source-only approaches into #10300 and #10334; their source IDs also occur under different titles in inactive surface2011/underdark2011, so preserve the `surf.wld` source identity on Surf candidate row 97. Curated total is 4,851 (Surf 285).

Typed closure records thirty local reset roles for Heavens objects #215/#376/#386/#398 and active Black Shroud reset/shop references to local objects #10382/#10340. Direct mobile procedures apply separate level checks at #10302 and #10320. No local native QST, sidecar, or SHP file exists; the local QST/AREA listing does not supply a contract. The title is unique among 442 current ZON titles and 351 active contributors; same-type overlap or replacement evidence was not found. Keep `split_shield` DO NOT MOVE.

The detailed story set is now 108 and mapped active contributors are 315/351. Across 476 data rows, pending reviews are narrative 144, connectivity 315, title/history 98, typed dependency 275, and builder questions 328; 148 builder-question sections are complete. Curated source-only WLD total: 4,851 (Surf 285). **Continue at physical coverage line 282, `area:spshold`.** See [coverage row 281](ZONE_COVERAGE_REGISTER.csv#L281), [Split Shield story map](../zone-stories/VILLAGE_OF_SPLIT_SHIELD.md), [audit](SPLIT_SHIELD_AUDIT_REVIEW.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-368-village-of-split-shield-connectivity-and-inactive-approach), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-368-split-shield-title-same-type-identity-and-followed-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-368-split-shield-in-place-retention-and-typed-map-references).


## Checkpoint 369: Storm Port Stronghold

Physical coverage line 282 was already complete for the source-comprehensive map, active graph, typed references, and followed title/history. This checkpoint closes its builder-question review with twenty conservative, source-backed recommendations in the [Storm Port Stronghold audit](STORM_PORT_STRONGHOLD_AUDIT_REVIEW.md#checkpoint-369-builder-question-coverage). Keep active `spshold` DO NOT MOVE. Preserve its three reciprocal Surface pairs, six one-way outbound exits to other active zones from load room #22649, which has no active incoming edge, and the known source-only Surf #285640 west approach on existing candidate row 97. The four native exchanges remain distinct from launch, travel, map acquisition, and later foreign chest access; retain the crew-shop debit defect as a separate required accounting repair.

The detailed story set remains 108 and mapped active contributors remain 315/351. Across 476 data rows, pending reviews are narrative 144, connectivity 315, title/history 98, typed dependency 275, and builder questions 327; 149 builder-question sections are complete. Curated source-only WLD remains 4,851 (Surf 285). **Continue at physical coverage line 283, `area:stormht`.** See [coverage row 282](ZONE_COVERAGE_REGISTER.csv#L282), [existing story map](../zone-stories/STORM_PORT_STRONGHOLD.md), [audit](STORM_PORT_STRONGHOLD_AUDIT_REVIEW.md), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


## Checkpoint 370: Strathor Valley of the Storm Giants

Physical coverage line 283 was already complete for its source-comprehensive story, 136-room/297-exit active graph, typed dependencies, and title/history; this checkpoint completes twenty conservative builder recommendations in the [Strathor Valley audit](STRATHOR_VALLEY_AUDIT_REVIEW.md#checkpoint-370-builder-question-coverage). Keep active `stormht` DO NOT MOVE. Preserve its two reciprocal Surface pairs and the known source-only Surf #216305 north approach on existing candidate row 97. Keep the king's sword proof, public response, generated bartender requests, periodic minstrel song, rune/control declarations, keyed containers, three shops, Alatorin's goat-leg reset, and Stromvok ferry reference within their authored owners and outcome boundaries.

The detailed story set remains 108 and mapped active contributors remain 315/351. Across 476 data rows, pending reviews are narrative 144, connectivity 315, title/history 98, typed dependency 275, and builder questions 326; 150 builder-question sections are complete. Curated source-only WLD remains 4,851 (Surf 285). **Continue at physical coverage line 284, `area:stormport`.** See [coverage row 283](ZONE_COVERAGE_REGISTER.csv#L283), [Strathor Valley story map](../zone-stories/STRATHOR_VALLEY.md), [audit](STRATHOR_VALLEY_AUDIT_REVIEW.md), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


## Checkpoint 371: Storm Port

Physical coverage line 284 was already complete for its source dossier, active graph, typed dependencies, title, and bounded history; this checkpoint adds twenty conservative builder recommendations in the [Storm Port audit](STORMPORT_AUDIT_REVIEW.md#checkpoint-371-builder-question-coverage). Keep active `stormport` DO NOT MOVE, separate from similarly named `spshold` and `stormht`. Preserve its four reciprocal Surface pairs, one-way Stronghold/Winterhaven inbounds, one-way Sarmiz outbound, unresolved Duris3 target #144664, and four source-only Surf approaches on existing candidate row 97. Keep the two QST exchanges distinct from the Tchan home/physical source, old ship access, modern ferry boarding/arrival, shops, repairs, and wider city-story leads.

The detailed story set remains 108 and mapped active contributors remain 315/351. Across 476 data rows, pending reviews are narrative 144, connectivity 315, title/history 98, typed dependency 275, and builder questions 325; 151 builder-question sections are complete. Curated source-only WLD remains 4,851 (Surf 285). **Continue at physical coverage line 285, `area:suntmpl`.** See [coverage row 284](ZONE_COVERAGE_REGISTER.csv#L284), [Storm Port story map](../zone-stories/STORMPORT.md), [audit](STORMPORT_AUDIT_REVIEW.md), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


## Checkpoint 372: The Temple of the Sun

Physical coverage line 285 was already complete for its story map, 94-room/217-exit active graph, typed references, title/history, and retention review; this checkpoint completes twenty builder recommendations in the [Temple of the Sun audit](TEMPLE_OF_THE_SUN_AUDIT_REVIEW.md#checkpoint-372-builder-question-coverage). Keep `suntmpl` DO NOT MOVE and preserve the reciprocal Surface pair, the source-only Surf approach on candidate row 97, and the `brass-old-1` reset reference on candidate row 17. Keep the three native item exchanges separate from SEARCH/key/PICK access, accepted history, restoration prose, imported services, outside recipes, and any daily policy.

The detailed story set remains 108 and mapped active contributors remain 315/351. Across 476 data rows, pending reviews are narrative 144, connectivity 315, title/history 98, typed dependency 275, and builder questions 324; 152 builder-question sections are complete. Curated source-only WLD remains 4,851 (Surf 285). **Continue at physical coverage line 286, `area:surface`.** See [coverage row 285](ZONE_COVERAGE_REGISTER.csv#L285), [Temple of the Sun story map](../zone-stories/TEMPLE_OF_THE_SUN.md), [audit](TEMPLE_OF_THE_SUN_AUDIT_REVIEW.md), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [`brass-old-1` candidate row 17](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17).

## Checkpoint 373: The Surface Realm of Duris

Physical coverage line 286 records the unique normalized title, bounded source identity/history evidence, typed references, and twenty source-backed builder questions in the [Surface audit](SURFACE_AUDIT_REVIEW.md); broader command-target alias disambiguation remains pending in the row. Keep surface DO NOT MOVE. Preserve the active graph and its two #33807 unresolved directions; record the 1,208 non-overlapping inactive approaches on existing asunwa, dalvik, jungle, newbiemaps, and surface2011-temp candidate rows. Their duplicate numeric targets and the 478,024 active-source-ID matches are not additional loaded routes. Keep Surface2011’s existing 145 references distinct. No active gameplay or migration decision is implied.

The detailed story set remains 108; mapped active contributors remain 315/351. Across 476 data rows, pending reviews are narrative 144, connectivity 315, title/history 97, typed dependency 274, and builder questions 323; 153 builder-question sections are complete. Curated source-only total is 6,059 (surf.wld subset 285). **Continue at physical coverage line 287, area:surfacekeeps.** See [coverage row 286](ZONE_COVERAGE_REGISTER.csv#L286), [Surface story map](../zone-stories/SURFACE_REALM.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-373-surface-connectivity-and-inactive-approaches), and [attic plan](ATTIC_MIGRATION_PLAN.md#checkpoint-373-surface-route-retention).
## Checkpoint 374: SurfaceKeeps (The Depths of Duris)

Physical coverage line 287 is complete for its source-comprehensive story dossier, static graph and inactive approaches, typed dependencies, bounded title/history review, and twenty source-backed builder questions. The SHP inventory is corrected from zero to ten shops. Keep active surfacekeeps DO NOT MOVE; preserve its six unresolved active exits, the 79/74 active boundaries, the Alatorin type-25 route, the 1,457 inactive source-only approaches on existing Duris3/oldmap8/ygg candidate rows, Duris3’s complete 2,645-room VNUM overlap with different titles, and the surfacekeeps_objects.obj stub source on candidate row 100. Broader command-target alias and owner disambiguation stays pending; no gameplay or migration decision is implied.

The detailed story set remains 108 and mapped active contributors remain 315/351. Across 476 data rows, pending reviews are narrative 144, connectivity 315, title/history 96, typed dependency 273, and builder questions 322; 154 builder-question sections are complete. Curated source-only WLD total is 7,516 (Surf 285). **Continue at physical coverage line 288, area:surfacemini.** See [coverage row 287](ZONE_COVERAGE_REGISTER.csv#L287), the [Depths of Duris dossier](../zone-stories/THE_DEPTHS_OF_DURIS.md), [audit supplement](SURFACEKEEPS_AUDIT_REVIEW.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-374-surfacekeeps-connectivity-and-inactive-approaches), and [retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-374-surfacekeeps-in-place-retention-and-typed-dependencies).


## Checkpoint 375: Surface Minizones

Physical coverage line 288 completes the existing source-comprehensive Surface Minizones dossier’s graph, typed-reference, bounded title/history, and twenty builder-question reconciliation. The active 119-room graph has 261 resolved directions, ten weak components, eight Surface inbounds, and eleven outward directions; four exits have no ordinary reverse, and crater #98014 has a second unmatched Surface inbound. Active Surface type-25 objects provide separate arrivals into four of those pockets. No inactive-WLD approach or room VNUM overlap was found. Preserve all 25 cross-type mobile/object ID pairs and the exact foreign QST, sidecar, ZON, shop, and procedure references. The local zero-shop count is correct. Keep `surfacemini` DO NOT MOVE; command-target alias/owner disambiguation, route/safety qualification, and the open repairs remain pending.

The detailed story set remains 108 and mapped active contributors remain 315/351. Across 476 data rows, pending reviews are narrative 144, connectivity 315, title/history 95, typed dependency 272, and builder questions 321; 155 builder-question sections are complete. Curated source-only WLD remains 7,516 (Surf 285). **Continue at physical coverage line 289, `area:swamp_one`.** See [coverage row 288](ZONE_COVERAGE_REGISTER.csv#L288), the [Surface Minizones dossier](../zone-stories/THE_MINIZONES_OF_THE_SURFACE.md), [audit supplement](SURFACEMINI_AUDIT_REVIEW.md), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-375-surface-minizones-connectivity-and-inactive-approaches), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-375-surface-minizones-title-same-type-identity-and-followed-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-375-surface-minizones-in-place-retention-and-typed-dependencies).

## Checkpoint 376: Shadow Swamp

Physical coverage line 289 completes the new source-comprehensive [Shadow Swamp dossier](../zone-stories/SHADOW_SWAMP.md), static graph census, bounded typed-reference closure, normalized title/same-type/tracked-history review, and twenty source-backed builder questions in the [Swamp One audit](SWAMP_ONE_AUDIT_REVIEW.md). Keep active swamp_one DO NOT MOVE. The 36-room WLD has 84/84 resolving directions in one weak component and two reciprocal active boundaries; retain the single inactive Surf source-only approach on existing Surf candidate row 97. Preserve Swamp Two’s cross-area reset ownership for the mob/key progression, the optional Caves sphere consumer, and the Nightbringer handler assignment. Secret discovery, quicksand and bog traversal, key availability, installed code behavior, and attic migration remain unqualified.

The detailed source-story set is 109 and mapped active contributors are 316/351. Across 476 data rows, pending reviews are narrative 143, connectivity 315, title/history 94, typed dependency 271, and builder questions 320; 156 builder-question sections are complete. The curated source-only WLD total is 7,517 (Surf subset 286). **Continue at physical coverage line 290, area:swamp_two.** See [coverage row 289](ZONE_COVERAGE_REGISTER.csv#L289), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-376-swamp-connectivity-and-inactive-approach), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-376-swamp-title-same-type-identity-and-followed-history), [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-376-swamp-in-place-retention-and-typed-dependencies), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).


## Checkpoint 377: The Lair of the Swamp Troll King

Physical coverage line 290 completes the new source-comprehensive [Swamp Troll King dossier](../zone-stories/SWAMP_TROLL_KING.md), static graph review, bounded typed-reference closure, normalized title/same-type/tracked-history review, and twenty source-backed builder questions in the [Swamp Two audit](SWAMP_TWO_AUDIT_REVIEW.md). Keep active swamp_two DO NOT MOVE. Its 35-room WLD has 85/85 resolving directions in one weak component, one reciprocal Swamp One boundary, and a raw directed path from the inbound target through all rooms to the outward edge. Preserve Swamp Two’s reset ownership for Swamp One troll-family prototypes and its typed G references to active Unique #67238, Heavens #359, and White Harbor object #55185. The normalized title is unique among 442 current .zon titles. No source-only WLD approach, same-type overlap, local QST, SHP, or story sidecar was found. The Mog text/reset mismatch and #26134 sealed-cavern/routes mismatch remain builder decisions; runtime access, safety, and migration remain unqualified.

The detailed source-story set is now 110. Reconciliation against the row-level dossier/story-sidecar union gives 316/351 mapped active contributors before Checkpoint 378; this corrects the earlier written 317/351. Across 476 data rows, pending reviews are narrative 142, connectivity 315, title/history 93, typed dependency 270, and builder questions 319; 157 builder-question sections are complete. Curated source-only WLD remains 7,517 (Surf subset 286). **Continue at physical coverage line 291, area:tchan.** See [coverage row 290](ZONE_COVERAGE_REGISTER.csv#L290), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-377-swamp-two-connectivity-and-cavern-routes), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-377-swamp-two-title-same-type-identity-and-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-377-swamp-two-in-place-retention-and-typed-dependencies).
## Checkpoint 378: Tchan and the Monastery of Tranquility

Physical coverage line 291 completes the source-comprehensive [Tchan monastery dossier](../zone-stories/TCHAN_MONASTERY.md), active graph and Surface boundary, typed reset/code references, normalized title and followed history, and twenty source-backed builder questions at CP378 in the [Tchan audit](TCHAN_AUDIT_REVIEW.md); CP759 later adds a 21st question about Ship Yards Ayden's statement that Tchan was lost and distinguishes Storm Port's older Small Monestary rooms. Keep active tchan DO NOT MOVE. Preserve the reciprocal Surface route, 10-room mountain trail, closed monastery threshold, and level-sensitive guard source binding. The four non-overlapping Alterian approaches remain source-only on candidate row 8; sixteen other target matches begin at overlapping room IDs #4801-#4809 and remain attached to their distinct inactive titles. Preserve Alterian's separate mobile/object #4800 definitions and source-only resets. No live passage, door use, or safety result is claimed.

The detailed source-story set is 111. The row-level dossier/story-sidecar union now maps 317/351 active contributors. Across 476 data rows, pending reviews are narrative 141, connectivity 315, title/history 92, typed dependency 269, and builder questions 318; 158 builder-question sections are complete. Curated source-only WLD total is 7,521 (Surf subset 286). **Continue at physical coverage line 292, area:teka2.** See [coverage row 291](ZONE_COVERAGE_REGISTER.csv#L291), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-378-tchan-connectivity-and-alterian-wilderness-approaches), [title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-378-tchan-and-alterian-identity-and-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-378-tchan-source-only-approaches-and-alterian-retention).
## Checkpoint 379: The Dark Stone Tower of the Northern Realms

Physical coverage line 292 keeps the existing [Dark Stone Tower dossier](../zone-stories/DARK_STONE_TOWER.md) and [Teka2 audit](TEKA2_AUDIT_REVIEW.md), and completes 22 explicit source-backed builder questions with conservative recommendations. No source, connectivity, typed-dependency, title/history, or disposition finding changed: keep active teka2 DO NOT MOVE. Preserve the single native Aerlyn receipt, separate source/access/travel/service outcomes, North Lakes pair, and all currently mapped key, secret-door, hazard, teleport, teacher, node and foreign-reset boundaries.

The detailed source-story set remains 111; the row-level dossier/story-sidecar union maps 317/351 active contributors. Across 476 data rows, pending reviews are narrative 141, connectivity 315, title/history 92, typed dependency 269, and builder questions 317; 159 builder-question sections are complete. Curated source-only WLD total remains 7,521 (Surf subset 286). **Continue at physical coverage line 293, area:temple.** See [coverage row 292](ZONE_COVERAGE_REGISTER.csv#L292), [Teka2 dossier](../zone-stories/DARK_STONE_TOWER.md), [Teka2 audit](TEKA2_AUDIT_REVIEW.md), and [the next coverage row](ZONE_COVERAGE_REGISTER.csv#L293).

## Checkpoint 380: Temple of Flames builder questions

Physical coverage line 293 keeps the existing [Temple of Flames dossier](../zone-stories/TEMPLE_OF_FLAMES.md), graph and source-only ingress review, typed dependency closure, title/history review, and DO NOT MOVE decision. It completes ten explicit source-backed builder questions with recommendations in the [Temple audit](TEMPLE_AUDIT_REVIEW.md). Preserve the current shared-route, finite-source, reward, and foreign-ownership boundaries until the builder resolves the documented intent questions; no native behavior or migration decision changed.

The detailed source-story set remains 111; mapped active contributors remain 317/351. Across 476 data rows, case-insensitive PENDING counts are narrative 142, connectivity 312, title/history 97, typed dependency 270, and builder questions 316; 160 builder-question sections are complete. Curated source-only WLD remains 7,521 (Surf subset 286). **Continue at physical coverage line 294, area:temple_laduguer.** See [coverage row 293](ZONE_COVERAGE_REGISTER.csv#L293), [Temple dossier](../zone-stories/TEMPLE_OF_FLAMES.md), [Temple audit](TEMPLE_AUDIT_REVIEW.md#checkpoint-380-ten-explicit-builder-questions-and-recommendations), and [the next coverage row](ZONE_COVERAGE_REGISTER.csv#L294).

## Checkpoint 381: Temple of Laduguer

Physical coverage line 294 is complete across the [Temple of Laduguer story/environment dossier](../zone-stories/TEMPLE_OF_LADUGUER.md), 103-room active graph, Sylvandawn source-only approach census, bounded typed/code closure, normalized title and followed-history review, and twenty source-backed builder questions in the [audit supplement](TEMPLE_LADUGUER_AUDIT_REVIEW.md). Keep active temple_laduguer DO NOT MOVE and preserve Sylvandawn RETAIN IN PLACE on candidate row 101.

The detailed source-story set is 112; the row-level dossier/story-sidecar union maps 318/351 active contributors. Across 476 data rows, case-insensitive PENDING counts are narrative 141, connectivity 311, title/history 96, typed dependency 269, and builder questions 315; 161 builder-question sections are complete. This recomputation from current register cells reconciles the immediately preceding Checkpoint 380 tally. Curated source-only WLD total is 7,538 (Surf subset 286). **Continue at physical coverage line 295, area:tentro2.** See [coverage row 294](ZONE_COVERAGE_REGISTER.csv#L294), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-381-temple-of-laduguer-graph-and-sylvandawn-approaches), [identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-381-temple-of-laduguer-title-identity-and-history), and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-381-temple-of-laduguer-retention-and-sylvandawn-dependencies).

## Checkpoint 382: The Mountain Village of Tentro

Physical coverage line 295 completes the source-comprehensive [Tentro dossier](../zone-stories/TENTRO.md), active graph and separate item-travel entry, Surf source-only approaches, typed-reference closure, title/history review, and twenty builder questions in [TENTRO2_AUDIT_REVIEW.md](TENTRO2_AUDIT_REVIEW.md). Keep active tentro2 DO NOT MOVE and preserve the four source-only Surf approaches on candidate row 97.

The detailed source-story set is 113; the row-level dossier/story-sidecar union maps 319/351 active contributors. Across 476 data rows, case-insensitive PENDING counts are narrative 140, connectivity 310, title/history 95, typed dependency 268, and builder questions 314; 162 builder-question sections are complete. Curated source-only WLD total is 7,542 (Surf subset 290). **Continue at physical coverage line 296, area:tezcat.** See [coverage row 295](ZONE_COVERAGE_REGISTER.csv#L295), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-382-tentro-connectivity-and-surf-approaches), [identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-382-tentro-title-vnum-identity-and-followed-history), [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-382-tentro-retention-and-surf-dependencies), and [next coverage row 296](ZONE_COVERAGE_REGISTER.csv#L296).

## Checkpoint 383: Tezcat

Physical coverage line 296 is complete across the [Ruined Temple of
Tezcatlipoca dossier](../zone-stories/TEZCAT.md), static graph and inactive
approaches, typed QST/story/code dependencies, unique title and followed
history, and twenty builder questions in [TEZCAT_AUDIT_REVIEW.md](TEZCAT_AUDIT_REVIEW.md).
Keep active `tezcat` DO NOT MOVE; preserve source-only Surf and Underworld2
approaches on their existing candidate rows 97 and 116.

The detailed source-story set is 114; the dossier/story-sidecar union maps
320/351 active contributors. Across 476 data rows, case-insensitive PENDING
counts are narrative 139, connectivity 309, title/history 94, typed
dependency 267, and builder questions 313; 163 builder-question sections are
complete. Curated source-only WLD total is 7,544 (Surf subset 291).
**Resume at physical coverage line 297, area:tharnadia.** See [coverage row
296](ZONE_COVERAGE_REGISTER.csv#L296), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-383-tezcat-connectivity-and-inactive-approaches),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-383-tezcat-title-identity-and-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-383-tezcat-retention-and-existing-map-candidates),
and [next coverage row 297](ZONE_COVERAGE_REGISTER.csv#L297).

## Checkpoint 384: Tharnadia

Physical coverage line 297 is complete across the [Tharnadia source-story
dossier](../zone-stories/THARNADIA.md), six-component active WLD graph,
item-teleport routes, 604 non-overlapping inactive Duris3 approaches, typed
story/reset/shop/code dependencies, unique normalized title and followed
history, and twenty builder recommendations in
[THARNADIA_AUDIT_REVIEW.md](THARNADIA_AUDIT_REVIEW.md). Keep active
`tharnadia` DO NOT MOVE; keep inactive Duris3 RETAIN IN PLACE on existing
candidate row 36. Its 1,732 matching directions from shared but differently
titled VNUMs remain identity evidence, not active ingress. Existing Duris3
active target-reference counts do not change.

The detailed source-story set remains 114; the dossier/story-sidecar union
remains 320/351 active contributors. Across 476 data rows, case-insensitive
PENDING counts are narrative 139, connectivity 308, title/history 93, typed
dependency 266, and builder questions 312; 164 builder-question sections are
complete. Curated source-only WLD total is 8,148 (Surf subset 291). Coverage
also corrects Tharnadia’s local shop inventory from zero to 19 (#132500–#132674).
**Continue at physical coverage line 298, area:tharnadian_ruin.** See [coverage row
297](ZONE_COVERAGE_REGISTER.csv#L297), [connectivity](WORLD_CONNECTIVITY.md#checkpoint-384-tharnadia-connectivity-and-duris3-approaches),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-384-tharnadia-title-identity-and-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-384-tharnadia-retention-and-duris3-candidate-update),
and [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

## Checkpoint 385: Tharnadian Ruin

Completed physical coverage row 298. The existing story dossier remains the
source for its three separate story cards and five native Q/QA histories. The
refreshed active graph has 179 rooms, 371/371 resolving exits, and weak
components of 168, 10, and 1. Raw directed WLD reach from its three active
arrival targets covers 165 rooms; 167 rooms can reach one of the three
outbound sources. The audit records exact unreachable sets, all 26 matching
door resets, two asymmetric paired door declarations, and the dynamic Surface
portal/return separately from WLD edges. It also reconciles Heavens,
Caertannad, Dungeon, Plane of Earth, QST, SHP, and compiled references.

The all-95 inactive-WLD scan found no overlap or source-only approach. The
normalized title is unique among 442 current zone files, and same-type active
VNUM comparisons find no room, mobile, or object overlap. Followed WLD/ZON
history shows same-stem route and quest edits and the 2009 path move, with no
distinct predecessor or whole-area replacement. Keep active tharnadian_ruin
DO NOT MOVE. No attic candidate or source-only total changes; global
source-only WLD remains 8,148 (Surf 291).

The detailed source-story set remains 114; the dossier/story-sidecar union
maps 320/351 active contributors. Across 476 data rows and 61 columns,
case-insensitive PENDING counts are narrative 139, connectivity 307,
title/history 93, typed dependency 266, and builder questions 311; 165
builder-question sections are complete. Continue at physical coverage row
299, area:tharnrifts. See [coverage row 298](ZONE_COVERAGE_REGISTER.csv#L298), [audit supplement](THARNADIAN_RUIN_AUDIT_REVIEW.md#checkpoint-385-directed-reachability-reset-state-and-builder-questions),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-385-tharnadian-ruin-directed-reach-and-route-state),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-385-tharnadian-ruin-title-identity-and-history),
and [retention](ATTIC_MIGRATION_PLAN.md#checkpoint-385-tharnadian-ruin-in-place-retention).

## Checkpoint 386: Tharnadia Rifts completed

Physical coverage row 299 is complete. The new source dossier maps all 41
color-stripped room-title families, the blank room-description field, all 62
foreign-owned M/O reset commands, and the silverleaf and hidden-crevice
connections. The refreshed WLD graph has 5,783 rooms, 22,252 authored
directions, seven weak components, directed arrival reach of 5,349/5,783,
and 5,781/5,783 rooms able to reach an active outbound. Preserve the
Caertannad and SurfaceKeeps boundaries and the two unresolved Duris3 targets;
runtime reach, reset timing, and route safety remain unqualified.

The all-95 inactive-WLD scan adds 1,323 Duris3 source-only directions and one
each from bb_gh and circlegh to the Tharnrifts source-only count. Keep the
21,809 Duris3 directions beginning at shared, differently titled room VNUMs
as identity evidence, not distinct approaches. The normalized title is unique
among 442 current ZON files; followed same-stem WLD/ZON history establishes no
separate predecessor or replacement. Keep active tharnrifts DO NOT MOVE and
Duris3 RETAIN IN PLACE on existing candidate row 36. Its 26 active unresolved
target references and 14 source-area keys are unchanged.

The detailed source-story set is 115; the dossier/story-sidecar union
maps 321/351 active contributors. Across 476 data rows and 61
columns, PENDING counts are narrative 138, connectivity
306, title/history 92, typed
dependency 265, and builder questions
310; 166 builder-question sections
are complete. Curated source-only WLD total is 9,473 (Surf subset 291).
Resume at physical coverage line 300, area:thetis. See
[coverage row 299](ZONE_COVERAGE_REGISTER.csv#L299),
[source dossier](../zone-stories/THARNADIA_RIFTS.md),
[audit review](THARNRIFTS_AUDIT_REVIEW.md), and the
[existing Duris3 candidate](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36). Next: [coverage row 300](ZONE_COVERAGE_REGISTER.csv#L300).

## Checkpoint 387: Thetis completed

Physical coverage row 300 is complete. The existing source-comprehensive
dossier maps three independent native requests, room/mobile/object prose,
doors and reset state, underwater hazards, the Salty shop, and foreign Jade
Mini Zones and Stronghold dependencies. The refreshed active graph has 100
rooms and 218 authored exits: 215 local, two outbound, and one unresolved
#104 target. There is no active WLD inbound; raw directed reach and return from
beach #38000 is 100/100. Route presence, actor access, reset availability,
and underwater safety remain separate.

The all-95 inactive-WLD scan adds one Surf source-only exit, #251543 north to
Thetis #38000, with no room-VNUM overlap. Keep Surf RETAIN IN PLACE on
candidate row 97 and do not count the route as active ingress. The curated
source-only total rises from 9,473 to 9,474 (Surf subset 292). The normalized
title is unique among 442 current ZON files; followed same-stem WLD/ZON history
establishes no separate predecessor or replacement. Keep active thetis DO NOT
MOVE.

The detailed source-story set remains 115; the dossier/story-sidecar union
maps 321/351 active contributors. Across 476 data rows and 61 columns,
PENDING counts are narrative 138,
connectivity 306,
title/history 92,
typed dependency 265, and builder questions
309; 167
builder-question sections are complete. Resume at physical coverage line
301, area:thri. See [coverage row 300](ZONE_COVERAGE_REGISTER.csv#L300),
[source dossier](../zone-stories/THETIS_REALM.md),
[audit review](THETIS_AUDIT_REVIEW.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-387-thetis-boundary-and-surf-approach-refresh),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-387-thetis-title-map-identity-and-followed-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-387-thetis-and-surf-source-map-retention),
and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

## Checkpoint 388: Taliccicopiid (Thri)

Physical coverage row 301 completes the existing Thri environmental story
review, static graph closure, inactive-source sweep, normalized title and
tracked-history review, typed source/code crosswalk, and twenty source-backed
builder recommendations. Keep active `thri` DO NOT MOVE. The 155-room WLD
has 339 authored exits (338 local, one outbound), one reciprocal Surface
arrival/departure pair, one weak component, and no unresolved targets. Raw
WLD reach from #67600 is 145/155; all rooms can return to #67600. Four local
type-25 destinations and the compiled inn/hometown/guild table hooks remain
separate from WLD reach and runtime qualification.

The all-95 inactive-WLD sweep adds Surf #250600 west to Thri #67600 as one
source-only approach, with no Thri room overlap. Keep it on existing Surf
candidate row 97, retain Surface2011's distinct same-VNUM source identity,
and do not count either as active ingress. Curated source-only WLD totals
rise to 9,475 overall and 293 for Surf. No Thri map move or candidate is
added. The normalized title is unique among 442 current ZON files, and
followed same-stem history supports no explicit predecessor or replacement.

The detailed source-story set remains 115; the row-level dossier/story-sidecar
union maps 321/351 active contributors. Across 476 data rows,
PENDING counts are narrative 138, connectivity
305, title/history
91, typed dependency
264, and builder questions
308; 168
builder-question sections are complete. Continue at physical coverage line
302, area:tiamat. See [coverage row 301](ZONE_COVERAGE_REGISTER.csv#L301),
[Thri review](THRI_STORY_REVIEW.md#checkpoint-388-graph-identity-and-typed-dependency-refresh),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-388-thri-connectivity-and-inactive-approach),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-388-thri-title-source-identity-and-followed-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-388-thri-and-surf-source-map-retention),
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and
[next coverage row 302](ZONE_COVERAGE_REGISTER.csv#L302).

## Checkpoint 389: Tiamat builder-question coverage

Physical coverage row 302 is complete for builder questions: the
existing dossier pairs twenty source-backed findings with explicit builder
questions and recommended resolutions, reconciled with the eight-story sidecar, selected QST source, and
Checkpoint 110 audit evidence. The authored map remains 39 rooms and 116
resolving exits, with 23/39 rooms reached from #19600 in the documented
ordinary-gates-open model. Keep active Tiamat DO NOT MOVE and keep
`brass-old-3` RETAIN pending its #25106 owner decision. No new map candidate
or migration is added; source-question completion makes no runtime or safe
return claim.

The detailed source-story set remains 115; the dossier/story-sidecar union
maps 321/351 active contributors. Across 476 coverage
rows, PENDING counts are narrative 138,
connectivity 305, title/history
91, typed dependency
264, and builder questions
307; 169
builder-question sections are complete. Continue at physical coverage line
303, area:tikit. See [coverage row 302](ZONE_COVERAGE_REGISTER.csv#L302),
[Tiamat question table](../zone-stories/TIAMAT.md#L54),
[Tiamat audit review](TIAMAT_AUDIT_REVIEW.md#checkpoint-389-builder-question-coverage-reconciliation),
and [next coverage row 303](ZONE_COVERAGE_REGISTER.csv#L303).

## Checkpoint 390: Lost City of Tikitzopl questions

Physical coverage row 303 is complete for builder-question review.
The dossier now pairs the fifteen broader progression stories with explicit
builder questions and conservative resolutions, plus one question for
contact-target aliases and ownership. Its forty-five technical ZSQ-TIKIT
follow-ups remain the detailed evidence and integration crosswalk. Existing
Surface and Lost Temple WLD pairs are reciprocal; the Surf #248527 approach
remains source-only on existing Surf candidate row 97, whose plan now records the exact source and target. It was already included in CP180 totals, so aggregate counts do not change. Keep active `tikit`
DO NOT MOVE; no new candidate or migration is added. Question completion does
not establish player-safe travel or new rescue and service behavior.

The detailed source-story set remains 115; the dossier/story-sidecar union
maps 321/351 active contributors. Across 476 coverage
rows, PENDING counts are narrative 138,
connectivity 305, title/history
91, typed dependency
264, and builder questions
306; 170
builder-question sections are complete. Continue at physical coverage line
304, area:tikitt. See [coverage row 303](ZONE_COVERAGE_REGISTER.csv#L303),
[question table](../zone-stories/LOST_CITY_OF_TIKITZOPL.md#explicit-builder-questions-and-recommended-resolutions),
[Lost City audit](TIKIT_AUDIT_REVIEW.md#checkpoint-390-builder-question-coverage-reconciliation),
and [next coverage row 304](ZONE_COVERAGE_REGISTER.csv#L304).

## Checkpoint 391: Lost Temple of Tikitzopl

Physical coverage row 304 now has a source-level graph and reach review,
completed typed prototype closure, followed title/history review, and twenty
explicit builder questions with recommendations. The Temple has 234 rooms,
616 resolving exits, full raw reach from its City entrance and a single
ordinary-WLD sink at the High Altar. City and Winterhaven are the two active
inbounds; only the City boundary reciprocates. The all-95 inactive-WLD scan
found no Temple-room overlap or approach.

Keep active tikitt DO NOT MOVE. Preserve its 15 City-owned recipe inputs,
Heavens object #360 reset at the altar, and type-25 object #44190 destination
#43704. No new candidate or migration is added. Static source review does not
qualify actual door access, reset availability, or safe travel. The detailed
source-story set remains 115; the dossier/story-sidecar union maps 321/351
active contributors. Across 476 rows and 61 columns, case-insensitive PENDING
counts are narrative 138, connectivity 304, title/history 90, typed
dependency 263, and builder questions 305; 171 builder-question sections are
complete. Curated source-only WLD totals remain 9,475 overall and 293 for
Surf. Continue at physical coverage line 305, area:torg. See
[coverage row 304](ZONE_COVERAGE_REGISTER.csv#L304),
[Temple review](TIKITT_AUDIT_REVIEW.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-391-lost-temple-connectivity-and-inbound-routes),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-391-lost-temple-title-map-identity-and-followed-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-391-lost-temple-retention-and-typed-dependencies),
and [next coverage row 305](ZONE_COVERAGE_REGISTER.csv#L305).

## Checkpoint 392: Torg source-backed review

Torg's active WLD is fully resolving at 624 exits across two weak components;
four active arrivals reach the 265-room and 17-room components, with one
reciprocal Surface return. Title and followed history are reviewed. Typed
source closure separates active QST, story, reset, shop, and code references
from unregistered reset and item-portal records. The inactive Surf route to
Torg #28900 is recorded on existing candidate row 97, increasing curated
source-only WLD counts to 9,476 overall and 294 Surf. Keep active Torg DO NOT
MOVE.

The detailed source-story set remains 115 and maps 321/351 active contributors
through dossier/story-sidecar union. Across 476 rows and 61 columns,
case-insensitive PENDING counts are narrative 138, connectivity 303,
title/history 89, typed dependency 262, and builder questions 304; 172
builder-question sections are complete. Continue at physical coverage line
306, area:torrhan. See [coverage row 305](ZONE_COVERAGE_REGISTER.csv#L305),
[Torg review](TORG_AUDIT_REVIEW.md), [connectivity](WORLD_CONNECTIVITY.md),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md), and
[retention](ATTIC_MIGRATION_PLAN.md).

## Checkpoint 393: Torrhan source-backed review

Torrhan's active WLD has 289 rooms, 623 resolving exits across one weak
component, four reciprocal Surface pairs, and a separate administrative
Winterhaven arrival. The directed WLD arrival set reaches 280/289 rooms.
Three inactive source-only Surf edges to Torrhan are now recorded on existing
Surf candidate row 97; curated totals rise to 9,479 overall and 297 for Surf.
Active Torrhan remains DO NOT MOVE.

Typed closure distinguishes the Shipy object #66723 contract consumer,
active/inactive reset and shop references, Heavens-owned active definitions
for imported #41/#72, inactive same-number prototypes, the missing #6087
prototype, and maintained code bindings. “City of Torrhan” is unique among
442 current titles, and no inactive WLD room overlap or active/inactive
external same-type duplicate was found. Twenty builder questions with
conservative recommendations are in the
[City of Torrhan dossier](../zone-stories/CITY_OF_TORRHAN.md#explicit-builder-questions-and-recommended-resolutions)
and [audit supplement](TORRHAN_AUDIT_REVIEW.md).

The detailed source-story set remains 115 and maps 321/351 active contributors
through dossier/story-sidecar union. Across 476 rows and 61 columns,
case-insensitive PENDING counts are narrative 138, connectivity 302,
title/history 88, typed dependency 261, and builder questions 303; 173
builder-question sections are complete. Continue at physical coverage line
307, area:torture. See [coverage row 306](ZONE_COVERAGE_REGISTER.csv#L306),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-393-torrhan-boundaries-and-directed-reach),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-393-torrhan-title-map-identity-and-followed-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-393-torrhan-in-place-retention-and-surf-approaches),
and [next coverage row 307](ZONE_COVERAGE_REGISTER.csv#L307).

## Checkpoint 394: Troll Slave Compound source-backed review

Torture's active WLD has 31 rooms and 61 resolving exit records: 60 internal
and one outbound to Ghore. All 31 rooms are mutually reachable from #95400 in
the raw directed graph. The Ghore boundary is a reciprocal no-door pair; the
separate interior gate resets closed. The all-95 inactive-WLD sweep found no
Torture room overlap or inbound edge, so source-only totals stay 9,479 overall
and 297 for Surf. Active Torture remains DO NOT MOVE.

Typed closure found no external reset, QST, SHP, story-sidecar, type-25 portal,
or exact maintained-code binding. The QST manifest lists `torture *954`, but no
Torture QST file exists. “Troll Slave Compound” is unique among 442 current
titles; followed source history shows no alternate-stem predecessor or zone
replacement in the available history. Twenty source-backed builder questions
with conservative recommendations are in the
[Troll Slave Compound dossier](../zone-stories/TROLL_SLAVE_COMPOUND.md#explicit-builder-questions-and-recommended-resolutions)
and [audit supplement](TORTURE_AUDIT_REVIEW.md).

The detailed story dossier/sidecar set remains 115 and maps 321/351 active
contributors. Across 476 rows and 61 columns, case-insensitive PENDING counts
are narrative 138, connectivity 301, title/history 87, typed dependency 261,
and builder questions 302; 174 builder-question sections are complete.
Continue at physical coverage line 308, area:tower. See [coverage row 307](ZONE_COVERAGE_REGISTER.csv#L307),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-394-troll-slave-compound-topology-and-ghore-boundary),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-394-troll-slave-compound-title-map-identity-and-followed-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-394-troll-slave-compound-in-place-retention),
and [next coverage row 308](ZONE_COVERAGE_REGISTER.csv#L308).

## Checkpoint 395: Tower of High Sorcery builder-question review

Coverage row 308 now contains 20 explicit source-backed builder questions with conservative recommendations in the [Tower dossier](../zone-stories/TOWER_OF_HIGH_SORCERY.md#explicit-builder-questions-and-recommended-resolutions). The existing Checkpoint 142 audit remains the source for Tower's graph, inactive Surf edges, typed and compiled references, title/history, and DO NOT MOVE disposition; no world or gameplay sources changed. Static questions do not qualify live access, encounters, hand-ins, rewards, or safe travel.

Across 476 rows and 61 columns, case-insensitive PENDING counts are narrative 138, connectivity 301, title/history 87, typed dependency 261, and builder questions 301; 175 builder-question sections are complete. Continue at physical coverage line 309, area:tradeskills. See [coverage row 308](ZONE_COVERAGE_REGISTER.csv#L308), [Tower audit supplement](TOWER_OF_HIGH_SORCERY_AUDIT_REVIEW.md), and [next coverage row 309](ZONE_COVERAGE_REGISTER.csv#L309).

## Checkpoint 396: Tradeskills source review

Coverage row 309 now documents the Tradeskills utility hub, its two-room graph,
dynamic corpse-portal route, shared 293-object catalog, Lout's single local
shop, and empty unregistered QST stub. Typed closure records active external
resets and shops, registered versus unregistered QST references, four story
sidecar consumers, compiled bindings, and the distinct `tradeskills.old.obj`
copy. Keep the area DO NOT MOVE and the older OBJ out of the loader. No
world/gameplay source changed. See the [dossier](../zone-stories/TRADESKILLS.md)
and [audit supplement](TRADESKILLS_AUDIT_REVIEW.md).

Across 476 rows and 61 columns, case-insensitive PENDING counts are narrative
137, connectivity 300, title/history 86, typed dependency 260, and builder
questions 300; 176 builder-question sections are complete. Continue at physical
coverage line 310, area:trakkia. See [coverage row 309](ZONE_COVERAGE_REGISTER.csv#L309),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-396-tradeskills-utility-hub-and-conditional-portal),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-396-tradeskills-title-catalog-identity-and-followed-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-396-tradeskills-in-place-retention),
and [next coverage row 310](ZONE_COVERAGE_REGISTER.csv#L310).

## Checkpoint 397: Trakkia builder-question review

Coverage row 310 now records 20 explicit source-backed builder questions and
conservative recommendations in the existing Trakkia dossier. The established
graph, inactive Surface approach, typed dependencies, title, history, and
DO NOT MOVE disposition remain unchanged. No world or gameplay source changed.
See the [dossier](../zone-stories/TRAKKIA_MOUNTAINS.md#explicit-builder-questions-and-recommended-resolutions)
and [audit supplement](TRAKKIA_AUDIT_REVIEW.md).

Across 476 rows and 61 columns, case-insensitive PENDING counts are narrative
137, connectivity 300, title/history 86, typed dependency 260, and builder
questions 299; 177 builder-question sections are complete. Continue at physical
coverage line 311, area:traoul. See [coverage row 310](ZONE_COVERAGE_REGISTER.csv#L310),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-397-trakkia-access-scenarios-retained),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-397-trakkia-title-and-bounded-history-retained),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-397-trakkia-in-place-retention),
and [next coverage row 311](ZONE_COVERAGE_REGISTER.csv#L311).

## Checkpoint 398: Traoul's Lair source reconciliation

Coverage row 311 now links a comprehensive source dossier and repository-wide
audit supplement for Traoul. The review maps the complete room/mobile/object
catalog, all local reset groups, Truk's reciprocal boundary and entrance key,
locked and secret routes, the missing local QST source despite its manifest
entry, typed external references, bounded history, and retention disposition.
It records 20 explicit builder questions. No world or gameplay source changed.
See the [dossier](../zone-stories/TRAOULS_LAIR.md) and
[audit supplement](TRAOUL_AUDIT_REVIEW.md).

Across 476 rows and 61 columns, case-insensitive PENDING counts are narrative
136, connectivity 299, title/history 85, typed dependency 259, and builder
questions 298; 178 builder-question sections are complete. The detailed story
dossier/sidecar map now covers 323/351 active contributors. Continue at physical
coverage line 312, area:tribal. See [coverage row 311](ZONE_COVERAGE_REGISTER.csv#L311),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-398-traouls-lair-doors-and-boundary),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-398-traoul-title-and-bounded-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-398-traoul-in-place-retention),
and [next coverage row 312](ZONE_COVERAGE_REGISTER.csv#L312).


## Checkpoint 399: Tribal Forest reconciliation

Physical coverage row 312 now has a source-linked repository-wide connectivity,
typed-dependency, title/history, and builder review. The raw WLD census is
462/462 resolving exits across 175 rooms, with three reciprocal Surface edges,
six Astral-bound WLD exits with no reverse WLD edge, and three local weak components. The dependency
closure includes the Heavens spell-pool import, local orb binding, and CHAOS
starter-kit consumer; no outside reset, QST, SHP, sidecar, or type-25 object
points into Tribal. The source dossier records 20 explicit builder questions.
Preserve active Tribal Forest DO NOT MOVE. No area or gameplay source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 rows and 61 columns, case-insensitive PENDING counts
are narrative 136, connectivity 298, title/history 84, typed dependency 258,
and builder questions 297; 179 builder-question sections are complete. Continue
at physical coverage line 313, area:trnsptow. See [coverage row 312](ZONE_COVERAGE_REGISTER.csv#L312),
[Tribal dossier](../zone-stories/TRIBAL_FOREST.md),
[audit supplement](TRIBAL_FOREST_AUDIT_REVIEW.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-399-tribal-forest-boundaries-and-directed-access),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-399-tribal-forest-title-and-bounded-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-399-tribal-forest-retained-in-place),
and [next coverage row 313](ZONE_COVERAGE_REGISTER.csv#L313).


## Checkpoint 400: Transparent Tower reconciliation

Physical coverage row 313 now has a source-linked repository-wide connectivity,
typed-dependency, title/history, retention, and builder review. The active
graph has 100 rooms and 248/248 resolving exits: 247 local and one outbound,
with a reciprocal ConnectorZones crossing and a one-way Realm arrival. One
source-only `surf.wld` north exit from #245010 enters Tower #16212; it is not
active ingress and raises curated source-only totals to 9,480 overall and 298
for Surf. The exact dependency closure includes three ConnectorZones equipment
recipes, four active outside reset suppliers, imported objects, CHAOS starter
profiles, conditional shared room-keyed code, and six source-only Luiren shop
room overlaps. The normalized title is unique and bounded history shows no
explicit predecessor or replacement. Twenty direct builder questions are in
the source dossier. Preserve active Tower DO NOT MOVE; no area or gameplay
source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 rows and 61 columns, case-insensitive PENDING counts
are narrative 136, connectivity 297, title/history 83, typed dependency 257,
and builder questions 296; 180 builder-question sections are complete.
Continue at physical coverage line 314, `area:troll_caves`. See [coverage row
313](ZONE_COVERAGE_REGISTER.csv#L313), [Tower dossier](../zone-stories/TRNSPTOW.md),
[audit supplement](TRNSPTOW_AUDIT_REVIEW.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-400-transparent-tower-boundaries-and-directed-access),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-400-transparent-tower-title-and-bounded-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-400-transparent-tower-retained-in-place),
and [next coverage row 314](ZONE_COVERAGE_REGISTER.csv#L314).

## Checkpoint 401: Troll Caves source-backed review

Coverage row 314 now records the completed static connectivity, bounded
typed-dependency, title/history, retention, and explicit builder-question
reviews. Troll Caves has 178/178 resolving exits in one 82-room local
component, two reciprocal active boundaries, and three previously cataloged
source-only WLD approaches. The exact native exchange boundaries, external
gem and reset dependencies, teleport declarations, and fall/control limits
remain source-qualified without live-play claims. Its dossier contains 20
direct-source builder questions with conservative resolutions; no area or
gameplay source changed. Keep Troll Caves DO NOT MOVE.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 136, connectivity 296, title/history 82, typed dependency
256, and builder questions 295; 181 builder-question sections are complete.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 315, `area:troll_hills`. See [coverage row 314](ZONE_COVERAGE_REGISTER.csv#L314),
[Troll Caves dossier](../zone-stories/TROLL_CAVES.md),
[builder questions](../zone-stories/TROLL_CAVES.md#explicit-builder-questions-and-recommended-resolutions),
[audit supplement](TROLL_CAVES_AUDIT_REVIEW.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-401-troll-caves-boundaries-and-directed-access),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#troll-caves),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-401-troll-caves-retained-in-place),
and [next coverage row 315](ZONE_COVERAGE_REGISTER.csv#L315).

## Checkpoint 402: Troll Hills builder questions

Coverage row 315 now contains twenty explicit source-linked builder questions
and conservative recommendations in the [Troll Hills dossier](../zone-stories/TROLL_HILLS.md#explicit-builder-questions-and-recommended-resolutions).
CP126 already records the complete static graph, bounded dependencies,
title/history, and DO NOT MOVE disposition. This checkpoint records the
builder-question review and clarifies that the static route map is complete
while actual gameplay access and safety remain unqualified. No area or
gameplay source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 136, connectivity 295, title/history 82, typed dependency
256, and builder questions 294; 182 builder-question sections are complete.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 316, area:truk. See [coverage row 315](ZONE_COVERAGE_REGISTER.csv#L315),
[Troll Hills audit](TROLL_HILLS_AUDIT_REVIEW.md),
[connectivity](WORLD_CONNECTIVITY.md),
and [next coverage row 316](ZONE_COVERAGE_REGISTER.csv#L316).

## Checkpoint 403: Truktakyagyo source-backed review

Coverage row 316 now has a source-comprehensive dossier and bounded static
connectivity, title/history, typed-dependency, retention, and explicit
builder-question reviews. The audit preserves three reciprocal active
boundaries, distinguishes the global world_quest bartender from local QST
content, records the QST AREA/file mismatch and dormant reset reference, and
keeps Truk DO NOT MOVE. Its dossier contains 20 direct-source builder questions
with conservative recommendations. No gameplay or area source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 135, connectivity 294, title/history 81, typed dependency
255, and builder questions 293; 183 builder-question sections are complete.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 317, area:ttowers. See [coverage row 316](ZONE_COVERAGE_REGISTER.csv#L316),
[Truktakyagyo dossier](../zone-stories/TRUK.md),
[builder questions](../zone-stories/TRUK.md#explicit-builder-questions-and-recommended-resolutions),
[audit supplement](TRUK_AUDIT_REVIEW.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-403-truktakyagyo-boundaries),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-403-truktakyagyo-title-and-bounded-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-403-truktakyagyo-retained-in-place),
and [next coverage row 317](ZONE_COVERAGE_REGISTER.csv#L317).

## Checkpoint 404: Twin Towers source-backed review

Coverage row 317 now has twenty explicit builder questions and recommended
resolutions in [TWIN_TOWERS.md](../zone-stories/TWIN_TOWERS.md#explicit-builder-questions-and-recommended-resolutions).
They retain the seven native Q identities and cover the exact heart bundles,
external item consumers, accepted versus proposed story outcomes, shared
access, source reset behavior, one authored fall, chest trap, and the computed
Jin Touch service. Existing connectivity, title/history, typed-dependency, and
DO NOT MOVE evidence remains in the [audit supplement](TWIN_TOWERS_AUDIT_REVIEW.md).
No area or gameplay source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 135, connectivity 294, title/history 81, typed dependency
255, and builder questions 292; 184 builder-question sections are complete.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 318, area: tundra. See [coverage row 317](ZONE_COVERAGE_REGISTER.csv#L317),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-404-twin-towers-boundaries-and-directed-access),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-404-twin-towers-title-and-bounded-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-404-twin-towers-retained-in-place),
and [next coverage row 318](ZONE_COVERAGE_REGISTER.csv#L318).

## Checkpoint 405: Tundra source-backed review

Coverage row 318 now has twenty explicit builder questions and recommended
resolutions in [TUNDRA.md](../zone-stories/TUNDRA.md#explicit-builder-questions-and-recommended-resolutions).
They retain the six separate story outcomes and cover the exact book and
seafood bundles, optional supplied items, actor retirement, paid-service
boundaries, fishing eligibility, shared controls, unavailable routes, foreign
typed consumers, and source-only WLD approaches. Existing connectivity,
title/history, typed-dependency, and DO NOT MOVE limits remain in the
[audit supplement](TUNDRA_AUDIT_REVIEW.md). No area or gameplay source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 135, connectivity 294, title/history 81, typed dependency
255, and builder questions 291; 185 builder-question sections are complete.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 319, area: turolzoo. See [coverage row 318](ZONE_COVERAGE_REGISTER.csv#L318),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-405-tundra-boundaries-and-directed-access),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-405-tundra-title-and-bounded-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-405-tundra-retained-in-place),
and [next coverage row 319](ZONE_COVERAGE_REGISTER.csv#L319).

## Checkpoint 406: Turolopolis Zoo source-backed review

Coverage row 319 now has twenty explicit builder questions and recommended
resolutions in [TUROLZOO.md](../zone-stories/TUROLZOO.md#explicit-builder-questions-and-recommended-resolutions).
They preserve the single native two-tooth exchange and separate its accepted
receipt from item custody, source history, departure, controller state, and
broader zoo-story outcomes. Existing connectivity, title/history,
typed-dependency, and DO NOT MOVE findings remain bounded in the
[audit supplement](TUROLZOO_AUDIT_REVIEW.md). No area or gameplay source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 135, connectivity 294, title/history 81, typed dependency
255, and builder questions 290; 186 builder-question sections are complete.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 320, area: twin_towers_forest. See [coverage row 319](ZONE_COVERAGE_REGISTER.csv#L319),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-406-turolopolis-zoo-boundaries-and-directed-access),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-406-turolopolis-zoo-title-and-bounded-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-406-turolopolis-zoo-retained-in-place),
and [next coverage row 320](ZONE_COVERAGE_REGISTER.csv#L320).

## Checkpoint 407: Twin Towers Forest source-backed review

Coverage row 320 now has twenty explicit builder questions and recommended
resolutions in [TWIN_TOWERS_FOREST.md](../zone-stories/TWIN_TOWERS_FOREST.md#explicit-builder-questions-and-recommended-resolutions).
They preserve the owner-approved gardener challenge and three-fact flower
achievement in any order, while separating access, item provenance, native
receipts, services, animal-care proposals, and reset ownership. Existing
connectivity, title/history, typed-dependency, and DO NOT MOVE limits remain.
See the [milestone review](TWIN_TOWERS_FOREST_MILESTONES.md). No area or
gameplay source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 135, connectivity 294, title/history 81, typed dependency
255, and builder questions 289; 187 builder-question sections are complete.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 321, area: twstwd. See [coverage row 320](ZONE_COVERAGE_REGISTER.csv#L320),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-407-twin-towers-forest-boundaries-and-directed-access),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-407-twin-towers-forest-title-scan),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-407-twin-towers-forest-retained-in-place),
and [next coverage row 321](ZONE_COVERAGE_REGISTER.csv#L321).

## Checkpoint 408: A Dark and Twisted Wood source-backed review

Coverage row 321 now has twenty explicit builder questions and recommended
resolutions in [DARK_AND_TWISTED_WOOD.md](../zone-stories/DARK_AND_TWISTED_WOOD.md#explicit-builder-questions-and-recommended-resolutions).
They preserve the single atomic faerie return and separate it from Pine Hollow
sources, report prose, personal history, conditional access, optional treasure,
and unqualified reset issuance. Existing connectivity, title/history,
typed-dependency, and DO NOT MOVE findings remain bounded in the
[audit supplement](DARK_AND_TWISTED_WOOD_AUDIT_REVIEW.md). No area or gameplay
source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 135, connectivity 294, title/history 81, typed dependency
255, and builder questions 288; 188 builder-question sections are complete.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 322, area: udmini. See [coverage row 321](ZONE_COVERAGE_REGISTER.csv#L321),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-408-a-dark-and-twisted-wood-boundaries-and-directed-access),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-408-a-dark-and-twisted-wood-title-and-bounded-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-408-a-dark-and-twisted-wood-retained-in-place),
and [next coverage row 322](ZONE_COVERAGE_REGISTER.csv#L322).

## Checkpoint 409: UDMINI source-backed review

Coverage row 322 now has twenty explicit builder questions and recommended
resolutions in [UDMINI.md](../zone-stories/UDMINI.md#explicit-builder-questions-and-recommended-resolutions).
They preserve the two WLD components, the unplaced mushroom teleporter, exact
outside spores consumers, and fall/teleport uncertainty while keeping access,
stock, and quest ownership bounded. Existing connectivity, title/history,
typed-dependency, and DO NOT MOVE findings remain in the [audit supplement](UDMINI_AUDIT_REVIEW.md).
No area or gameplay source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 135, connectivity 294, title/history 81, typed dependency
255, and builder questions 287; 189 builder-question sections are complete.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 323, area: ugta. See [coverage row 322](ZONE_COVERAGE_REGISTER.csv#L322),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-409-udmini-boundaries-and-directed-access),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-409-udmini-title-and-bounded-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-409-udmini-retained-in-place),
and [next coverage row 323](ZONE_COVERAGE_REGISTER.csv#L323).

## Checkpoint 410: Ugta source-backed review

Coverage row 323 now has twenty explicit builder questions and recommended
resolutions in [UGTA_STORY_REVIEW.md](UGTA_STORY_REVIEW.md#explicit-builder-questions-and-recommended-resolutions).
They preserve the gate and guardian cues, distinguish story-sidecar claims
from authored QST outcomes, keep imported object ownership and source-only
approaches separate, and leave live route/shop behavior unqualified. Existing
connectivity, title/history, typed-dependency, and DO NOT MOVE findings remain
in the [source review](UGTA_STORY_REVIEW.md). No area or gameplay source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 135, connectivity 294, title/history 81, typed dependency
255, and builder questions 286; 190 builder-question sections are complete.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 324, area: underdark. See [coverage row 323](ZONE_COVERAGE_REGISTER.csv#L323),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-410-ugta-boundaries-and-directed-access),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-410-ugta-title-and-bounded-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-410-ugta-retained-in-place),
and [next coverage row 324](ZONE_COVERAGE_REGISTER.csv#L324).

## Checkpoint 411: Durian Underdark source-backed review

Coverage row 324 now has twenty explicit builder questions and recommended
resolutions in [DURIAN_UNDERDARK.md](../zone-stories/DURIAN_UNDERDARK.md#explicit-builder-questions-and-recommended-resolutions).
They keep native item acceptance separate from source history and retain
the documented limits on active-accounting issuance, sparse-map travel,
dynamic portals, worm custody, relic mutation, and mining/guild outcomes.
Existing connectivity, title/history, typed-dependency, and DO NOT MOVE
findings remain in the [audit supplement](DURIAN_UNDERDARK_AUDIT_REVIEW.md).
No area or gameplay source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 135, connectivity 294, title/history 81, typed dependency
255, and builder questions 285; 191 builder-question sections are complete.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 325, area: undermountain. See [coverage row 324](ZONE_COVERAGE_REGISTER.csv#L324),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-411-durian-underdark-loader-qualified-graph),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-411-durian-underdark-title-and-bounded-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-411-durian-underdark-and-2011-candidate-retained),
and [next coverage row 325](ZONE_COVERAGE_REGISTER.csv#L325).

## Checkpoint 412: Undermountain source-backed review

Coverage row 325 now has twenty explicit builder questions and recommended
resolutions in [THE_RUINS_OF_UNDERMOUNTAIN.md](../zone-stories/THE_RUINS_OF_UNDERMOUNTAIN.md#explicit-builder-questions-and-recommended-resolutions).
They retain the source-comprehensive map's exact QST inputs and keep rescue,
source recovery, hidden access, fixed portals, foreign receipts, and dormant
systems distinct. Existing connectivity, title/history, typed-dependency, and
DO NOT MOVE findings remain in the [audit supplement](UNDERMOUNTAIN_AUDIT_REVIEW.md).
No area or gameplay source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 135, connectivity 294, title/history 81, typed dependency
255, and builder questions 284; 192 builder-question sections are complete.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 326, area: underworld. See [coverage row 325](ZONE_COVERAGE_REGISTER.csv#L325),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-412-undermountain-boundaries-and-directed-access),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-412-undermountain-title-and-bounded-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-412-undermountain-retained-in-place),
and [next coverage row 326](ZONE_COVERAGE_REGISTER.csv#L326).

## Checkpoint 413: Underworld source-backed review

Coverage row 326 now has twenty explicit builder questions and recommended
resolutions in [UNDERWORLD.md](../zone-stories/UNDERWORLD.md#explicit-builder-questions-and-recommended-resolutions).
They keep Zorta's native skill reward separate from its resurrection dialogue,
retain independent relic owners and outside consumers, and qualify pools,
keys, hidden routes, and current stock. Existing connectivity, title/history,
typed-dependency, and DO NOT MOVE findings remain in the [audit supplement](UNDERWORLD_AUDIT_REVIEW.md).
No area or gameplay source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 135, connectivity 294, title/history 81, typed dependency
255, and builder questions 283; 193 builder-question sections are complete.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 327, area: underworld3. See [coverage row 326](ZONE_COVERAGE_REGISTER.csv#L326),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-413-underworld-boundaries-and-directed-access),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-413-underworld-title-and-bounded-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-413-underworld-retained-in-place),
and [next coverage row 327](ZONE_COVERAGE_REGISTER.csv#L327).

## Checkpoint 414: Underworld3 source-backed review

Coverage row 327 now has twenty explicit builder questions and recommended
resolutions in [UNDERWORLD3.md](../zone-stories/UNDERWORLD3.md#explicit-builder-questions-and-recommended-resolutions).
They preserve the duplicate active title as distinct ownership, keep
environmental prose separate from unverified hazards and mechanics, and
record the active one-way inbound boundary without mislabeling it source-only.
The boulder pair, portal prototypes, reset locations, and live availability
remain distinct questions. No area or gameplay source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 135, connectivity 294, title/history 81, typed dependency
255, and builder questions 282; 194 builder-question sections are complete.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 328, area: unique. See [coverage row 327](ZONE_COVERAGE_REGISTER.csv#L327),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-414-underworld3-boundaries-and-directed-access),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-414-underworld3-title-and-bounded-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-414-underworld3-retained-in-place),
and [next coverage row 328](ZONE_COVERAGE_REGISTER.csv#L328).

## Checkpoint 415: Unique Vault source-backed review

Coverage row 328 now has twenty explicit builder questions and recommended
resolutions in [UNIQUE.md](../zone-stories/UNIQUE.md#explicit-builder-questions-and-recommended-resolutions).
They retain its two-room display map, keep the object prototypes with their
active owner across three documented external reset uses, and leave token
redemption, item placement, artifact rumors, and apparent powers bounded by
their sources. No area or gameplay source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 135, connectivity 294, title/history 81, typed dependency
255, and builder questions 281; 195 builder-question sections are nonpending.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 329, area: unoutpst. See [coverage row 328](ZONE_COVERAGE_REGISTER.csv#L328),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-415-unique-vault-boundaries-and-directed-access),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-415-unique-vault-title-and-bounded-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-415-unique-vault-retained-in-place),
and [next coverage row 329](ZONE_COVERAGE_REGISTER.csv#L329).

## Checkpoint 416: Undead Outpost source-backed review

Coverage row 329 now has twenty explicit builder questions and recommended
resolutions in [UNDEAD_OUTPOST.md](../zone-stories/UNDEAD_OUTPOST.md#explicit-builder-questions-and-recommended-resolutions).
They preserve the two active reciprocal boundaries, distinguish the one
negative dropped exit from live routes, and keep the F10 location, reset-closed
paths, reset-placed items, shop room text, and unresolved foreign object
within their source limits. No area or gameplay source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 135, connectivity 294, title/history 81, typed dependency
255, and builder questions 280; 196 builder-question sections are nonpending.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 330, area: v2. See [coverage row 329](ZONE_COVERAGE_REGISTER.csv#L329),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-416-undead-outpost-boundaries-and-directed-access),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-416-undead-outpost-title-and-bounded-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-416-undead-outpost-retained-in-place),
and [next coverage row 330](ZONE_COVERAGE_REGISTER.csv#L330).

## Checkpoint 417: Vargan II source-backed review

Physical coverage row 330 now links to twenty direct-source questions and
recommended resolutions in [V2_BUILDER_QUESTIONS.md](../zone-stories/V2_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).
The review bounds the exact sword contract and keeps its dialogue, item
recovery, doors, map, hazards, encounters, and later campaign as separate
builder decisions. No area or gameplay source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 135, connectivity 294, title/history 81, typed dependency
255, and builder questions 279; 197 builder-question sections are nonpending.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 331, area: val. See [coverage row 330](ZONE_COVERAGE_REGISTER.csv#L330),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-417-vargan-ii-routes-and-builder-question-scope),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-417-vargan-ii-title-and-bounded-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-417-vargan-ii-retained-in-place),
and [next coverage row 331](ZONE_COVERAGE_REGISTER.csv#L331).

## Checkpoint 418: Valoisian Castle source-backed review

Physical coverage row 331 now links to twenty explicit source questions and
recommended resolutions in [VALOISIAN_CASTLE_BUILDER_QUESTIONS.md](../zone-stories/VALOISIAN_CASTLE_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).
They preserve the exact native exchanges and separate them from dialogue
aliases, personal combat, keyed passage, unplaced prototypes, and the source-
only Surface exit. No area or gameplay source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 135, connectivity 294, title/history 81, typed dependency
255, and builder questions 278; 198 builder-question sections are nonpending.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 332, area: valdrak. See [coverage row 331](ZONE_COVERAGE_REGISTER.csv#L331),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-418-valoisian-castle-connectivity-and-source-only-ingress),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-418-valoisian-castle-title-and-bounded-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-418-valoisian-castle-retained-in-place),
and [next coverage row 332](ZONE_COVERAGE_REGISTER.csv#L332).

## Checkpoint 419: Phantasmagoric Caverns source-backed review

Physical coverage row 332 now links to twenty direct-source questions and
recommended resolutions in [VALDRAK_BUILDER_QUESTIONS.md](../zone-stories/VALDRAK_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).
They preserve the exact three-item return and treat the ritual, community,
shop, food, passage, and inactive-map approach as separate decisions. No area
or gameplay source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 135, connectivity 294, title/history 81, typed dependency
255, and builder questions 277; 199 builder-question sections are nonpending.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 333, area: valinhav. See [coverage row 332](ZONE_COVERAGE_REGISTER.csv#L332),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-419-phantasmagoric-caverns-routes-and-directed-access),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-419-phantasmagoric-caverns-title-and-bounded-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-419-phantasmagoric-caverns-retained-in-place),
and [next coverage row 333](ZONE_COVERAGE_REGISTER.csv#L333).

## Checkpoint 420: Valinhaven source-backed review

Physical coverage row 333 now links to twenty direct-source questions and
conservative recommendations in [VALINHAV_BUILDER_QUESTIONS.md](../zone-stories/VALINHAV_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).
They keep source-authored river and cave access, gates, mining, training,
administrative text, and encounters separate from unimplemented services,
hazards, quests, and loot. No area or gameplay source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 135, connectivity 294, title/history 81, typed dependency
255, and builder questions 276; 200 builder-question sections are nonpending.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 334, area: valley_crushk. See [coverage row 333](ZONE_COVERAGE_REGISTER.csv#L333),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-420-valinhaven-routes-and-cave-access),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-420-valinhaven-title-and-bounded-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-420-valinhaven-retained-in-place),
and [next coverage row 334](ZONE_COVERAGE_REGISTER.csv#L334).

## Checkpoint 421: Valley of Crushk source-backed review

Physical coverage row 334 now links to twenty direct-source questions and
conservative recommendations in [VALLEY_CRUSHK_BUILDER_QUESTIONS.md](../zone-stories/VALLEY_CRUSHK_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).
They distinguish the one native exact-item bounty from broader narrative
orientation, external source availability, route boundaries, keyed doors, and
village-service ideas. No area or gameplay source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 135, connectivity 294, title/history 81, typed dependency
255, and builder questions 275; 201 builder-question sections are nonpending.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 335, area: vargan. See [coverage row 334](ZONE_COVERAGE_REGISTER.csv#L334),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-421-valley-of-crushk-reciprocal-and-one-way-routes),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-421-valley-of-crushk-title-and-bounded-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-421-valley-of-crushk-retained-with-its-foreign-source),
and [next coverage row 335](ZONE_COVERAGE_REGISTER.csv#L335).

## Checkpoint 422: Vargan source-backed review

Physical coverage row 335 now links to twenty direct-source questions and
conservative recommendations in [VARGAN_BUILDER_QUESTIONS.md](../zone-stories/VARGAN_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).
They preserve the documented unavailable status for the paid mixed exchange,
the exact hidden-piece recipe, local movement and environment, all reciprocal
active boundaries, and the inactive source-only approach. No area or gameplay
source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 135, connectivity 294, title/history 81, typed dependency
255, and builder questions 274; 202 builder-question sections are nonpending.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 336, area: vecna. See [coverage row 335](ZONE_COVERAGE_REGISTER.csv#L335),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-422-vargan-reciprocal-boundaries-and-directed-access),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-422-vargan-title-history-and-identity),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-422-vargan-retained-with-source-only-surface-approach),
and [next coverage row 336](ZONE_COVERAGE_REGISTER.csv#L336).

## Checkpoint 423: Vecna's Tomb source-backed review

Physical coverage row 336 now links to twenty direct-source questions and
conservative recommendations in [VECNAS_TOMB_BUILDER_QUESTIONS.md](../zone-stories/VECNAS_TOMB_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).
They preserve the two source-defined item returns and distinguish them from
search, survival, combat, reward delivery, keys, portals, and inactive map
records. No area or gameplay source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 135, connectivity 294, title/history 81, typed dependency
255, and builder questions 273; 203 builder-question sections are nonpending.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 337, area: vehicles. See [coverage row 336](ZONE_COVERAGE_REGISTER.csv#L336),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-423-vecnas-tomb-routes-and-directed-access),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-423-vecnas-tomb-title-history-and-disposition),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-423-vecnas-tomb-retained-with-its-mapped-dependencies),
and [next coverage row 337](ZONE_COVERAGE_REGISTER.csv#L337).

## Checkpoint 424: Vehicles source-backed review

Physical coverage row 337 now has twenty direct-source builder questions and
conservative recommendations in [VEHICLES_BUILDER_QUESTIONS.md](../zone-stories/VEHICLES_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).
They keep the seven configured ferries distinct from the static WLD exit map,
preserve three independent captain flag exchanges and three crew food offers,
and call out ticket, route, shop-count, external-reset, crafting, and hazard
decisions. The current contributor remains **DO NOT MOVE**. No area or
gameplay source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 135, connectivity 294, title/history 81, typed dependency
255, and builder questions 272; 204 builder-question sections are nonpending.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 338, `area:verspin`. See [coverage row 337](ZONE_COVERAGE_REGISTER.csv#L337),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-424-vehicles-static-layout-and-configured-ferry-travel),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-424-vehicles-title-history-and-disposition),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-424-vehicles-retained-with-transport-and-story-dependencies),
and [next coverage row 338](ZONE_COVERAGE_REGISTER.csv#L338).

## Checkpoint 425: Verspin source-backed review

Physical coverage row 338 now has twenty direct-source builder questions and
conservative recommendations in [VERSPIN_BUILDER_QUESTIONS.md](../zone-stories/VERSPIN_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).
They preserve the six-story/six-service split and exact item contracts while
calling out source/capacity discrepancies, missing Lozin sign bindings, shop
census mismatch, entrance roles, guarded services, and pit/waterway/shaft
access. The current contributor remains **DO NOT MOVE**. No area or gameplay
source changed.

The detailed story dossier/sidecar map remains 117 and covers 323/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 135, connectivity 294, title/history 81, typed dependency
255, and builder questions 271; 205 builder-question sections are nonpending.
Curated source-only WLD totals remain 9,480 overall and 298 for Surf. Continue
at physical coverage row 339, `area:verz1`. See [coverage row 338](ZONE_COVERAGE_REGISTER.csv#L338),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-425-verspin-local-graph-entrances-and-hazards),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-425-verspin-title-and-disposition),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-425-verspin-retained-with-its-story-and-world-dependencies),
and [next coverage row 339](ZONE_COVERAGE_REGISTER.csv#L339).

## Checkpoint 426: Verzanan source story and dependency closure

Physical coverage row 339 now has a detailed static source-story and typed
dependency review in [VERZANAN_STORY_REVIEW.md](../zone-stories/VERZANAN_STORY_REVIEW.md),
plus twenty explicit builder questions in
[VERZANAN_BUILDER_QUESTIONS.md](../zone-stories/VERZANAN_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).
The WLD contains two reciprocal bookend rooms; the local ZON has no placements.
The source catalog has 158 mobile and 180 object prototypes, and the objects
have extensive external reset, shop, and QST consumers. The old mobile and
portal bindings are disabled; #3097 retains an active locker/balance binding.
No title match or source history proves replacement by Verspin. Keep Verz1 DO
NOT MOVE and retain both city identities.

The detailed source-story set rises from 117 to 118 and covers 324/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 134, connectivity 294, title/history 80, typed
dependency 254, and builder questions 270; 206 builder-question sections are
nonpending. Curated source-only WLD totals remain 9,480 overall and 298 for
Surf. Continue at physical coverage row 340, area:Voluntown. See [coverage
row 339](ZONE_COVERAGE_REGISTER.csv#L339),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-426-verzanan-static-room-graph-and-absent-ingress),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-426-verzanan-title-and-bounded-source-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-426-verzanan-preserved-as-an-active-dependency-source),
and [next coverage row 340](ZONE_COVERAGE_REGISTER.csv#L340).


## Checkpoint 427: Voluntown source review

Physical row 340 now has a completed builder-question review with twenty
source-backed decisions in the [Voluntown question file](../zone-stories/THE_ROYAL_MAUSOLEUM_OF_CASTLE_ICECRAG_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).
The active graph's open-edge traversal reaches 132/133 local rooms from
#142400 and returns to #142400 from all 133; unnamed room #142472 has no
incoming WLD edge. Keep the graph result separate from its locked entrance,
trap/fall state, and installed route qualification. Complete the bounded
title/history review: IceCrag is a connected, separately owned contributor,
the same-stem history begins with the 2012 source addition, and no predecessor
or replacement is shown. Preserve outbound Haratius imports #359 from
Heavens and #22070 from the Pits of Cerberus, as well as the 24 C/C++ table
references to eight local objects. Keep active Voluntown DO NOT MOVE; no
area/gameplay source changed.

The detailed source-story set remains 118 and covers 324/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 134, connectivity 294, title/history 80, typed
dependency 254, and builder questions 269; 207 builder-question sections are
nonpending. Curated source-only WLD totals remain 1,505 overall and 122 for
Surf. Continue at physical row 341, `area:vrzawild`, Crystalspyre Mountains.
See [coverage row 340](ZONE_COVERAGE_REGISTER.csv#L340),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-427-voluntown-directed-reach-and-return-paths),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-427-voluntown-title-history-and-icecrag-relationship),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-427-voluntown-retained-with-imported-object-dependencies),
and [next coverage row 341](ZONE_COVERAGE_REGISTER.csv#L341).


## Checkpoint 428: Crystalspyre source review

Physical row 341 now has a source-comprehensive environmental/story review in
the [Crystalspyre dossier](../zone-stories/CRYSTALSPYRE_MOUNTAINS_STORY_REVIEW.md)
and twenty direct builder questions in the
[question review](../zone-stories/CRYSTALSPYRE_MOUNTAINS_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).
The slab describes an unfinished road to Verzanan and a temporary high-level
encounter; the current city stub is still a separate owner with no active
WLD connection. The local end sequence is Zikata's outerworld keep, with three
33% fall rooms and a closed-door warming hut along the environmental route.
Keep the imported Heavens #358 callback and the inactive Surf approach under
their existing owners. Keep active `vrzawild` DO NOT MOVE. No area or gameplay
source changed.

The detailed source-story set rises to 119 and covers 325/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 133, connectivity 294, title/history 79, typed
dependency 253, and builder questions 268; 208 builder-question sections are
nonpending. Curated source-only WLD totals rise to 1,506 overall and 123 for
Surf. Continue at physical row 342, `area:wander`, The Wandering Fields. See
[coverage row 341](ZONE_COVERAGE_REGISTER.csv#L341),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-428-crystalspyre-directed-paths-falls-and-approaches),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-428-crystalspyre-and-the-planned-verzanan-road),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-428-crystalspyre-retained-with-separate-map-approaches),
and [next coverage row 342](ZONE_COVERAGE_REGISTER.csv#L342).


## Checkpoint 429: Wandering Fields source review complete

Completed the source-story, directed-graph, typed-reference, bounded
title/history, retention, and builder-question reviews for physical row 342,
area:wander. The 14-room source is explicitly a mobile-loading and
destination-selection layer for Llzazan. It has 54 resolving exits: 18 local
and 36 one-way exits into Llzazan, with no active or inactive WLD ingress.
The local graph from #36600 reaches six destination selectors; the probability
chain feeds back toward #36600 and ends at exitless #36608. The displayed
probability text has unresolved mismatches. Preserve imported Arachdrathos
mobile #36011, Llzazan objects #36204/#36205, and six source-only ud_map0
reset references. Keep active Wander DO NOT MOVE and ud_map0 RETAIN IN PLACE.
No area or gameplay source changed.

The detailed source-story set rises to 120 and covers 326/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 132, connectivity 294, title/history 78, typed
dependency 252, and builder questions 267; 209 builder-question sections are
nonpending. Curated source-only WLD totals remain 1,506 overall and 123 for
Surf. Continue at physical row 343, area:wemic, Wellvolen. See
[coverage row 342](ZONE_COVERAGE_REGISTER.csv#L342),
[source dossier](../zone-stories/WANDERING_FIELDS_STORY_REVIEW.md),
[builder questions](../zone-stories/WANDERING_FIELDS_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-429-wander-staging-graph-and-llzazan-exits),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-429-wander-title-history-and-llzazan-relationship),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-429-wander-retained-with-llzazan-and-arachdrathos-dependencies),
and [next coverage row 343](ZONE_COVERAGE_REGISTER.csv#L343).

## Checkpoint 430: Wellvolen source review complete

Completed the source-story, directed-graph, typed-reference, bounded
title/history, retention, and builder-question reviews for physical row 343,
area:wemic. The 47-room village has 93 resolving exits, with a strongly
connected local graph. Its active routes are the reciprocal Surface boundary
and a one-way WH room explicitly authored for mob dispersal. One inactive
surf.wld approach into Wemic remains source-only under the existing Surf
candidate row 97. Preserve the exact Shipyard use of wicked-mace object
#93501, whose VNUM is also used by a distinct Wemic warrior mobile. Keep
active Wemic DO NOT MOVE and inactive Surf RETAIN IN PLACE.

The detailed source-story set rises to 121 and covers 327/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 131, connectivity 294, title/history 77, typed
dependency 251, and builder questions 266; 210 builder-question sections are
nonpending. Curated source-only WLD totals rise to 1,507 overall and 124 for
Surf. Continue at physical row 344, area:werrun, Village of Werrun. See
[coverage row 343](ZONE_COVERAGE_REGISTER.csv#L343),
[source dossier](../zone-stories/WELLVOLEN_STORY_REVIEW.md),
[builder questions](../zone-stories/WELLVOLEN_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-430-wellvolen-directed-graph-and-approaches),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-430-wellvolen-title-history-and-owner-boundary),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-430-wellvolen-retained-with-typed-quest-and-map-dependencies),
and [next coverage row 344](ZONE_COVERAGE_REGISTER.csv#L344).

## Checkpoint 431: Village of Werrun builder review complete

The existing source-comprehensive dossier and Checkpoint 66 audit already
cover physical row 344, area:werrun: its four native QSTs, 84-room topology,
typed foreign consumers, bounded title/history review, and DO NOT MOVE
disposition. Added twenty explicit builder questions with recommended
resolutions for the native receipts, access mechanisms, generated missions,
foreign ownership, and conditional Nexus56 shop source. Existing player,
payment, recurrence, keeper-binding, and safe-return qualifications remain
open.

The detailed source-story set remains 121 and covers 327/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 131, connectivity 294, title/history 77, typed
dependency 251, and builder questions 265; 211 builder-question sections are
nonpending. Curated source-only WLD totals remain 1,507 overall and 124 for
Surf. Continue at physical row 345, area:wh, The City of Winterhaven. See
[coverage row 344](ZONE_COVERAGE_REGISTER.csv#L344),
[source dossier](../zone-stories/VILLAGE_OF_WERRUN.md),
[builder questions](../zone-stories/VILLAGE_OF_WERRUN_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[technical audit](WERRUN_AUDIT_REVIEW.md#checkpoint-431-builder-question-review),
[connectivity](WORLD_CONNECTIVITY.md#village-of-werrun-surface-pair-wh-arrival-and-source-only-roads),
and [next coverage row 345](ZONE_COVERAGE_REGISTER.csv#L345).

## Checkpoint 432: Winterhaven review complete

Physical row 345, area:wh, now links its existing source-comprehensive dossier
to a bounded typed-dependency and identity review and twenty direct builder
questions. The 1,558-exit WLD map resolves all targets; open-edge reach from the
five active arrivals covers 553/599 rooms, while 12 rooms have no directed
local path to an outbound edge. Player access, safety, loader choice, and
runtime availability remain unqualified. Winterhaven stays DO NOT MOVE; the
unregistered Brass references remain on existing candidate rows 17 and 19.

The detailed source-story set remains 121 dossiers covering 327/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 131, connectivity 295, title/history 76, typed dependency
251, and builder questions 264; 212 builder-question sections are nonpending.
Curated source-only WLD totals remain 1,507 overall and 124 for Surf. Continue
at physical row 346, area:wildland_trails. See [coverage row
345](ZONE_COVERAGE_REGISTER.csv#L345), [source dossier](../zone-stories/WINTERHAVEN.md),
[builder questions](../zone-stories/WINTERHAVEN_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[technical audit](WINTERHAVEN_AUDIT_REVIEW.md#checkpoint-432-source-boundary-and-prior-dossier),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-432-winterhaven-directed-graph-and-approaches),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-432-winterhaven-title-history-and-owner-boundary),
and [next coverage row 346](ZONE_COVERAGE_REGISTER.csv#L346).

## Checkpoint 433: Wildland Trails review complete

Physical row 346 now has an environmental story review, bounded technical
audit, and twenty source-backed builder questions. The 99-room map has one
weak component; open directed WLD reach from either source-only Surf approach
covers 70 rooms, while all rooms have a local path back to those approach
rooms. Two local exits remain unresolved because their VNUMs have different
room identities in inactive Surf and Surface2011 candidates. Static typed
source closure found no exact external data or same-type prototype references;
custom combat handling and live access/recovery remain unqualified. Keep the
active zone DO NOT MOVE and both existing candidates in place.

The detailed source-story set rises to 122 dossiers covering 328/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 130, connectivity 295, title/history 75, typed dependency
250, and builder questions 263; 213 builder-question sections are nonpending.
Curated source-only WLD totals remain 1,507 overall and 124 for Surf. Continue
at physical row 347, area:willem, The Ruins of Turolopolis. See [coverage row
346](ZONE_COVERAGE_REGISTER.csv#L346), [source review](../zone-stories/WILDLAND_TRAILS_STORY_REVIEW.md),
[builder questions](../zone-stories/WILDLAND_TRAILS_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[technical audit](WILDLAND_TRAILS_AUDIT_REVIEW.md#checkpoint-433-source-inventory-and-environment),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-433-wildland-trails-directed-graph-and-legacy-map-candidates),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-433-wildland-trails-title-history-and-owner-boundary),
and [next coverage row 347](ZONE_COVERAGE_REGISTER.csv#L347).

## Checkpoint 434: The Ruins of Turolopolis

Physical row 347 now links its existing source-comprehensive dossier and
repository-wide audit to twenty builder questions. Followed WLD, MOB, OBJ,
ZON, and QST histories reach the August 2009 areas-tree move; the same-stem
story sidecar was added in 2026. Bounded comparison with Turolopolis Zoo
closes the reviewed title/replacement question; alias disambiguation remains
separate. Active `willem` stays DO NOT MOVE, and Surf's
source-only approach remains under existing candidate row 97. Quest behavior,
foreign reset generation, live stock, arrival, access, and safe return remain
unqualified.

The detailed source-story set remains 122 dossiers covering 328/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 130, connectivity 295, title/history 74, typed dependency
250, and builder questions 262; 214 builder-question sections are nonpending.
Curated source-only WLD totals remain 1,507 overall and 124 for Surf. Continue
at physical row 348, area:wolfen, The Lost Realms of Shadowfall. See [coverage
row 347](ZONE_COVERAGE_REGISTER.csv#L347), [source dossier](../zone-stories/RUINS_OF_TUROLOPOLIS.md),
[builder questions](../zone-stories/RUINS_OF_TUROLOPOLIS_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[technical audit](TUROLOPOLIS_AUDIT_REVIEW.md#checkpoint-434-builder-question-review),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-434-turolopolis-title-history-and-owner-boundary),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-434-turolopolis-retained-with-source-only-approach),
and [next coverage row 348](ZONE_COVERAGE_REGISTER.csv#L348).

## Checkpoint 435: The Lost Realms of Shadowfall

Physical row 348 now links a source-comprehensive review, bounded map and
typed-reference audit, and twenty direct builder questions. The active
Surface boundary is reciprocal. Two WLD weak components (255 and 3 rooms)
are bridged by the paired tunnel rifts; the campsite wagon portal reaches the
two interior rooms. Static source-graph reach from the Surface arrival
covers all 258 rooms when the three local type-25 routes are included. Live
access, reset availability, outbreak outcome, shop eligibility, and safe
return remain unqualified. Keep `wolfen` DO NOT MOVE.
The coverage inventory now records the one local SHP record under keeper
#18019; live stock and faction eligibility remain unqualified.

The detailed source-story set rises to 123 dossiers covering 329/351 active
contributors. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 129, connectivity 295, title/history 73, typed dependency
249, and builder questions 261; 215 builder-question sections are nonpending.
Curated source-only WLD totals remain 1,507 overall and 124 for Surf. Continue
at physical row 349, area:woodseer, Woodseer. See [coverage row 348](ZONE_COVERAGE_REGISTER.csv#L348),
[source review](../zone-stories/WOLFEN_STORY_REVIEW.md),
[builder questions](../zone-stories/WOLFEN_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[technical audit](WOLFEN_AUDIT_REVIEW.md#checkpoint-435-builder-question-review),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-435-shadowfall-directed-graph-surface-pair-and-portal-routes),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-435-shadowfall-title-history-and-owner-boundary),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-435-shadowfall-retained-with-mapped-portal-dependencies),
and [next coverage row 349](ZONE_COVERAGE_REGISTER.csv#L349).

## Checkpoint 436: Woodseer builder review

Physical row 349 now links twenty builder questions to the existing
source-comprehensive dossier and Checkpoint 76 connectivity, title/history, and
typed-dependency audits. The register now records all twenty local SHP records,
keyed #16508–#16691. Keep active woodseer DO NOT MOVE. No area source or runtime
behavior changed. Across 476 data rows and 61 columns, case-insensitive PENDING
counts are narrative 129, connectivity 295, title/history 73, typed dependency
249, and builder questions 260; 216 builder-question sections are nonpending.
The detailed source-story set remains 123 dossiers covering 329/351 active
contributors, and curated source-only WLD totals remain 1,507 overall and 124
for Surf. Continue at physical row 350, area:worms, The Caverns of the Worms.
See [coverage row 349](ZONE_COVERAGE_REGISTER.csv#L349),
[source dossier](../zone-stories/WOODSEER.md),
[builder questions](../zone-stories/WOODSEER_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[technical audit](WOODSEER_AUDIT_REVIEW.md#checkpoint-436-builder-question-review),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-436-woodseer-remains-active),
and [next coverage row 350](ZONE_COVERAGE_REGISTER.csv#L350).

## Checkpoint 437: The Caverns of the Worms builder review

Physical row 350 now links twenty direct builder questions to the existing
sixteen-service source dossier and Checkpoint 63 static map, title/history, and
typed-dependency review. The questions preserve native bundle dispatch, exact
source identities, the switch route, foreign item/shop references, and the
Ixarkon travel boundary. Worms has no local SHP source, so its zero-shop
inventory remains unchanged. Keep active worms DO NOT MOVE. Across 476 data
rows and 61 columns, case-insensitive PENDING counts are narrative 129,
connectivity 295, title/history 73, typed dependency 249, and builder questions
259; 217 builder-question sections are nonpending. The detailed source-story
set remains 123 dossiers covering 329/351 active contributors, and curated
source-only WLD totals remain 1,507 overall and 124 for Surf. Continue at
physical row 351, area:yuan_ti, The Temple to Skrentherlog. See
[coverage row 350](ZONE_COVERAGE_REGISTER.csv#L350),
[source dossier](../zone-stories/THE_CAVERNS_OF_THE_WORMS.md),
[builder questions](../zone-stories/THE_CAVERNS_OF_THE_WORMS_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[technical audit](WORMS_AUDIT_REVIEW.md#checkpoint-437-builder-question-review),
[connectivity](WORLD_CONNECTIVITY.md#the-caverns-of-the-worms-and-an-unregistered-ud_map2-approach),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#lair-of-the-purple-worm),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-437-worms-remains-active),
and [next coverage row 351](ZONE_COVERAGE_REGISTER.csv#L351).

## Checkpoint 438: The Temple to Skrentherlog builder review

Physical row 351 now links twenty direct builder questions to the existing
source-comprehensive account and Checkpoint 82 static graph, typed-reference,
and title/history review. The recommendations keep the ranger's blood exchange,
the captive's hammer lead, keyed access, epic touch, equipment effects, and
Winterhaven memory return distinct. Keep active yuan_ti DO NOT MOVE. Across
476 data rows and 61 columns, case-insensitive PENDING counts are narrative
129, connectivity 295, title/history 73, typed dependency 249, and builder
questions 258; 218 builder-question sections are nonpending. The detailed
source-story set remains 123 dossiers covering 329/351 active contributors,
and curated source-only WLD totals remain 1,507 overall and 124 for Surf.
Continue at physical row 352, area:yuan_ti2, Village of the Damned. See
[coverage row 351](ZONE_COVERAGE_REGISTER.csv#L351),
[source dossier](../zone-stories/THE_TEMPLE_TO_SKRENTHERLOG.md),
[builder questions](../zone-stories/THE_TEMPLE_TO_SKRENTHERLOG_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[technical audit](TEMPLE_TO_SKRENTHERLOG_AUDIT_REVIEW.md#checkpoint-438-builder-question-review),
[connectivity](WORLD_CONNECTIVITY.md#the-temple-to-skrentherlog-one-reciprocal-village-pair-and-one-one-way-inbound),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-438-the-temple-to-skrentherlog-owner-boundary),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-438-the-temple-to-skrentherlog-remains-active),
and [next coverage row 352](ZONE_COVERAGE_REGISTER.csv#L352).

## Checkpoint 439: Village of the Damned builder review

Physical row 352 now links twenty direct builder questions to the existing
environmental dossier and Checkpoint 205 graph, typed-reference, and
title/history audit. The questions preserve the unresolved village violence,
unplaced mobiles, clue and door intent, imported Heavens stock, and separate
neighbor ownership. Keep active yuan_ti2 DO NOT MOVE; Surf row 97 and oldmap8
row 81 remain RETAIN IN PLACE with their source-only approaches. Across 476
data rows and 61 columns, case-insensitive PENDING counts are narrative 129,
connectivity 295, title/history 73, typed dependency 249, and builder questions
257; 219 builder-question sections are nonpending. The detailed source-story
set remains 123 dossiers covering 329/351 active contributors, and curated
source-only WLD totals remain 1,507 overall and 124 for Surf. Continue at
physical row 353, component:aeon_gh, Aeon Order Guildhall. See
[coverage row 352](ZONE_COVERAGE_REGISTER.csv#L352),
[source dossier](../zone-stories/YUAN_TI2.md),
[builder questions](../zone-stories/YUAN_TI2_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[technical audit](YUAN_TI2_AUDIT_REVIEW.md#checkpoint-439-builder-question-review),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-205-yuan-ti2-reset-aware-access),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-205-yuan-ti2-title-and-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-439-village-of-the-damned-remains-active),
and [next coverage row 353](ZONE_COVERAGE_REGISTER.csv#L353).

## Checkpoint 440: Aeon Order Guildhall

Physical row 353 is reviewed as the unregistered `aeon_gh` source component.
Its single local shop record is #29901 in #29909. Source review covers its
ten-room map, one source-only exit to a `Duris3.wld` Ocean room, imported
equipment and shop stock, and a direct compiled `inn` binding at #29903.
`aeon_gh` shares zone #299, room VNUMs #29900–#29909, and mobiles #29900/#29901
with the separate unregistered `genesis` candidate at row 397. Their content
and mobile definitions differ; neither title or source history establishes a
replacement. Keep existing attic rows 2 and 46 **RETAIN IN PLACE** with the
owner and code binding unresolved.

Across 476 data rows and 61 columns, case-insensitive PENDING counts are
narrative 128, connectivity 295, title/history 73, typed dependency 248, and
builder questions 256; 220 builder-question sections are nonpending. The
detailed source-story set remains 123 dossiers covering 329/351 active
contributors, and curated source-only WLD totals remain 1,507 overall and 124
for Surf. Continue at physical row 354, `component:afterlife_gh`, The Afterlife
Guildhall. See [coverage row 353](ZONE_COVERAGE_REGISTER.csv#L353),
[technical audit](AEON_GH_AUDIT_REVIEW.md#source-inventory-and-area-list-status),
[builder questions](../zone-stories/AEON_ORDER_GUILDHALL_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-440-aeon-order-guildhall-source-only-map-and-exit),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-440-aeon-order-guildhall-and-genesis-vnum-collision),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-440-aeon-and-genesis-retain-their-current-sources),
and [next coverage row 354](ZONE_COVERAGE_REGISTER.csv#L354).

## Checkpoint 441: Afterlife Guildhall reviewed

Physical row 354 records the unregistered `afterlife_gh` component, including
its ten-room story dossier, local M-only lore QST, one shop with fourteen
products, and the `areas/map/afterlife_gh.json` editor asset. Its local graph
has one component and one authored exit to `Duris3.wld` #123813 (“Rolling
Hills”); target loader status and return remain unresolved. All ten room
VNUMs overlap active Torg rooms with different authored content, and compiled
code binds the Players Guild `inn` handler at #29103. Keep candidate row 3
**RETAIN IN PLACE**; preserve the Player Castles and sibling-candidate object
imports and do not infer replacement or migration clearance.

Across 476 data rows and 61 columns, case-insensitive PENDING counts are
narrative 127, connectivity 295, title/history 73, typed dependency 247, and
builder questions 255; 221 builder-question sections are nonpending. The
detailed source-story set now contains 124 dossiers covering 329/351 active
contributors; curated source-only WLD totals remain 1,507 overall and 124 for
Surf. Continue at physical row 355, `component:afterlife_op`, “Afterlife GC
OP.” See [coverage row 354](ZONE_COVERAGE_REGISTER.csv#L354),
[technical audit](AFTERLIFE_GH_AUDIT_REVIEW.md#source-inventory-and-loader-membership),
[source dossier](../zone-stories/THE_AFTERLIFE_GUILDHALL.md),
[builder questions](../zone-stories/AFTERLIFE_GUILDHALL_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-441-afterlife-guildhall-map-and-torg-overlap),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-441-afterlife-guildhall-and-active-torg-identity),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-441-afterlife-guildhall-retains-its-current-candidate),
[candidate row 3](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L3), and
[next coverage row 355](ZONE_COVERAGE_REGISTER.csv#L355).

## Checkpoint 442: Afterlife GC OP reviewed

Physical row 355 now records the unregistered `afterlife_op` portal passage,
its three-room graph, editor map JSON, two type-25 objects, and ZON imports.
The WLD has four local exits and one D4 to `Duris3.wld` #157174 (“The
Ocean”), a source absent from the full area list; no direct return edge or
active WLD inbound edge was found. All three room VNUMs overlap active Torg
and unregistered `void_gh` rooms with different content. The two portal
objects encode #29106 and #29110 despite their Khomani-Kan and Goodie
Continent descriptions. Keep candidate row 4 **RETAIN IN PLACE** and preserve
the separate `afterlife_gh` mobile and object dependencies and Player Castles
object #11053.

Across 476 data rows and 61 columns, case-insensitive PENDING counts are
narrative 126, connectivity 295, title/history 73, typed dependency 246, and
builder questions 254; 222 builder-question sections are nonpending. The
detailed source-story set now contains 125 dossiers covering 329/351 active
contributors. Curated source-only WLD totals remain 1,507 overall and 124 for
Surf because this component's sole WLD boundary targets inactive `Duris3`,
not an active area. Continue at physical row 356, `component:afterlife_op2`,
“Afterlife GC OP.” See [coverage row 355](ZONE_COVERAGE_REGISTER.csv#L355),
[technical audit](AFTERLIFE_OP_AUDIT_REVIEW.md#inventory-and-source-membership),
[source dossier](../zone-stories/AFTERLIFE_GC_OP.md),
[builder questions](../zone-stories/AFTERLIFE_GC_OP_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-442-afterlife-op-source-graph-and-portal-boundaries),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-442-afterlife-op-and-vnum-ownership-conflicts),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-442-afterlife-op-retains-its-source-identity),
[candidate row 4](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L4), and
[next coverage row 356](ZONE_COVERAGE_REGISTER.csv#L356).


## Checkpoint 443: Afterlife GC OP2 reviewed

Physical row 356 now records the nine-room afterlife_op2 source component,
its three local maps, fourteen WLD direction records, editor map JSON, reset
imports, seven portal targets, and shared normalized title with afterlife_op.
All nine room VNUMs also collide with different active Torg definitions.
The local graph has three components; two D4 exits target unlisted Duris3
rooms #172709 and #163943 without direct returns. Torg's paths to the same
numeric range are not OP2 ingress. Keep candidate row 5 **RETAIN IN PLACE**.

Across 476 data rows and 61 columns, case-insensitive PENDING counts are
narrative 125, connectivity 295, title/history 73, typed dependency 245, and
builder questions 253; 223 builder-question sections are nonpending. The
detailed source-story set now contains 126 dossiers covering 329/351 active
contributors. Curated source-only WLD totals remain 1,507 overall and 124 for
Surf. Continue at physical row 357, component:alatorin_objects. See
[coverage row 356](ZONE_COVERAGE_REGISTER.csv#L356),
[technical audit](AFTERLIFE_OP2_AUDIT_REVIEW.md),
[source dossier](../zone-stories/AFTERLIFE_GC_OP2.md),
[builder questions](../zone-stories/AFTERLIFE_GC_OP2_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-443-afterlife-op2-three-components-and-source-only-boundaries),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-443-afterlife-gc-op2-title-history-and-room-ownership),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-443-afterlife-op2-retains-its-current-source),
[candidate row 5](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L5), and
[next coverage row 357](ZONE_COVERAGE_REGISTER.csv#L357).

## Checkpoint 444: Alatorin object bank reviewed

Physical row 357 now records the unregistered 408-object alatorin_objects
source, its Taark Shanat and dwarven-artifact lore, 24 type-25 targets,
historical AREA-stub membership, and all active typed dependencies. Every
VNUM also exists in active alatorin.obj; 204 prototypes match and 204 differ.
Another 168 IDs collide with different unlisted Nexus56 objects. The active
#83434 portal targets #83188, while this source copy targets #83831. Keep
candidate row 6 **RETAIN IN PLACE** until saved-object identity and prototype
ownership are resolved.

Across 476 data rows and 61 columns, case-insensitive PENDING counts are
narrative 124, connectivity 295, title/history 73, typed dependency 244, and
builder questions 252; 224 builder-question sections are nonpending. The
detailed source-story set now contains 127 dossiers covering 329/351 active
contributors. Curated source-only WLD totals remain 1,507 overall and 124 for
Surf because this component has no WLD source. Continue at physical row 358,
component:alterian_mountains. See [coverage row 357](ZONE_COVERAGE_REGISTER.csv#L357),
[technical audit](ALATORIN_OBJECTS_AUDIT_REVIEW.md),
[source dossier](../zone-stories/ALATORIN_OBJECTS.md),
[builder questions](../zone-stories/ALATORIN_OBJECTS_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-444-alatorin-object-bank-and-typed-travel-targets),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-444-alatorin-object-bank-history-and-overlap),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-444-alatorin-object-bank-retains-its-source),
[candidate row 6](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L6), and
[next coverage row 358](ZONE_COVERAGE_REGISTER.csv#L358).

## Checkpoint 445: Alterian Mountains component reviewed

Physical row 358 now captures the source-only Alterian Mountains map,
18 mobiles, the #4900 object collision, and its shared zone-number and room
VNUM conflicts with active Mining. The Mountain and Mining files use the same
zone number 49; all 49 shared room IDs have different room records. Static
review records two source-only exits to Alterian Wilderness, three
source-only Mountain exits targeting Mining numbers, 16 Mountain M-reset
destinations in the shared Mining range, 30 Wilderness resets using Mountain
mobiles, and a compiled `wanderer` callback for #4830. Keep candidate row 7
**RETAIN IN PLACE**.

Across 476 data rows and 61 columns, case-insensitive PENDING counts are
narrative 123, connectivity 295, title/history 73, typed dependency 243, and
builder questions 251; 225 builder-question sections are nonpending. The
detailed source-story set now contains 128 dossiers covering 329/351 active
contributors. This checkpoint adds no new curated world-exit records; the
three Mountain-to-Mining source-only edges were already recorded by the
Mining supplement. Continue at physical row 359,
`component:alterian_wilderness`. See [coverage row 358](ZONE_COVERAGE_REGISTER.csv#L358),
[technical audit](ALTERIAN_MOUNTAINS_AUDIT_REVIEW.md),
[source dossier](../zone-stories/ALTERIAN_MOUNTAINS.md),
[builder questions](../zone-stories/ALTERIAN_MOUNTAINS_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-445-alterian-mountains-source-graph-and-boundary-identities),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-445-alterian-mountains-and-mining-identity),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-445-alterian-mountains-retains-distinct-source-records),
[candidate row 7](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L7), and
[next coverage row 359](ZONE_COVERAGE_REGISTER.csv#L359).

## Checkpoint 446: Alterian Wilderness component reviewed

Physical row 359 now records the source-only 130-room Wilderness map, its
steam caves, portals, farmstead and wildlife, and its 17 mobile / 15 object
prototypes. Static review reconciles all 29 different room records that
overlap active Tchan and Mining, both same-type #4800 overlaps with Tchan,
the source-only links to Mountain and Surf, and outbound Mining, Shadow
Forest, and Tchan references. It also records the reciprocal Alterian reset
imports, active `bs.zon` object #4814 references, and compiled callbacks
#4800, #4812, #4801, and #4802. Keep candidate row 8 **RETAIN IN PLACE**.

Across 476 data rows and 61 columns, case-insensitive PENDING counts are
narrative 122, connectivity 295, title/history 73, typed dependency 242, and
builder questions 250; 226 builder-question sections are nonpending. The
detailed source-story set now contains 129 dossiers covering 329/351 active
contributors. This checkpoint adds no new curated source-only exits into
active areas; the previously counted Mining, Shadow Forest, and Tchan
approaches are now reconciled with their Wilderness source identity. Curated
source-only WLD totals remain 1,507 overall and 124 for Surf. Continue at
physical row 360, `component:aod_gh`. See [coverage row 359](ZONE_COVERAGE_REGISTER.csv#L359),
[technical audit](ALTERIAN_WILDERNESS_AUDIT_REVIEW.md),
[source dossier](../zone-stories/ALTERIAN_WILDERNESS.md),
[builder questions](../zone-stories/ALTERIAN_WILDERNESS_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-446-alterian-wilderness-source-graph-and-boundary-identities),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-446-alterian-wilderness-map-and-prototype-identities),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-446-alterian-wilderness-remains-a-separate-source),
[candidate row 8](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L8), and
[next coverage row 360](ZONE_COVERAGE_REGISTER.csv#L360).

## Checkpoint 447: AoD GH component reviewed

Physical row 360 now records the source-only 24-room AoD GH map, its
mausoleum, Tormented Souls and Alliance of Darkness lore, dark-jungle route,
and Bloodlust citadel. Static review records three WLD components and two
down exits into unlisted map identities; 26 different room records overlap
Genocide Hall, Legion, and `rf_op`; and the room #29502 Players Guild callback
has competing unlisted definitions. The source has three mobiles, five
objects, one shop record, no local QST, seven Player Castles object resets,
13 mobile reset commands, and 16 stock/G resets. Product #6089 lacks a source
OBJ definition. Keep candidate row 9 **RETAIN IN PLACE**.

The 17 builder questions cover source membership, duplicate room/mobile/
object/zone identities, both map boundaries, the disconnected rooms, portal
values, #6089, the Valrok inventory, and the external editor/persisted/runtime
evidence required before migration. Across 476 data rows and 61 columns,
case-insensitive PENDING counts are narrative 121, connectivity 295,
title/history 73, typed dependency 241, and builder questions 249; 227
builder-question sections are nonpending. The detailed source-story set now
contains 130 dossiers covering 329/351 active contributors. AoD adds no
curated source-only WLD exit into an active area; totals remain 1,507 overall
and 124 for Surf. Continue at physical row 361, `component:aopal`. See
[coverage row 360](ZONE_COVERAGE_REGISTER.csv#L360),
[AoD audit](AOD_GH_AUDIT_REVIEW.md),
[story dossier](../zone-stories/AOD_GH.md),
[builder questions](../zone-stories/AOD_GH_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-447-aod-gh-source-graph-and-boundary-identities),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-447-aod-gh-map-and-zone-identities),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-447-aod-gh-remains-a-separate-source),
[candidate row 9](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L9), and
[next coverage row 361](ZONE_COVERAGE_REGISTER.csv#L361).

## Checkpoint 448: Tharnadian Causeway (Aopal)

Physical coverage row 361 now records the source-only 44-room Aopal map,
15 mobiles, five objects, and its prose-driven causeway, keep, aqueduct,
reservoir, Samara haunting, and highway-patrol setting. The directed graph
has 94 exits in weak components of 43 and one room; six target-specific
source-only approaches into active maps are preserved, and candidate inbound
remains zero. The 28 shared Ship Yards rooms all differ. Typed closure records
37 mobile reset commands, missing mobile #6116, two source-only imports of
active Heavens mobile #445, 12 matching door resets, and the compiled
#43341 patrol handler. Tracked history shows no predecessor or whole-zone
replacement. Keep candidate row 10 **RETAIN IN PLACE**.

The new dossier and twenty builder questions complete the repository-bounded
story, graph, title/history, and typed dependency review. Across 476 data rows
and 61 columns, case-insensitive PENDING counts are narrative 120,
connectivity 294, title/history 73, typed dependency 240, and builder
questions 248; 228 builder-question sections are nonpending. The detailed
source-story set now contains 131 dossiers covering 329/351 active
contributors. Aopal adds no approach that was not already documented in the
Shadow Forest, Ancient Bridge, and Ship Yards reviews.

Checkpoint 448 also replaces the incompatible historical cumulative
source-only totals with a current all-95 recount under an explicit
destination-identity rule: 11,987 direction records overall, including 415
from Surf. Older notes state 4,529, 9,480, and 1,507 and do not form one
consistent increment chain. See the [coverage row 361](ZONE_COVERAGE_REGISTER.csv#L361),
[Aopal audit](AOPAL_AUDIT_REVIEW.md),
[story dossier](../zone-stories/AOPAL.md),
[builder questions](../zone-stories/AOPAL_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[connectivity recount](WORLD_CONNECTIVITY.md#checkpoint-448-aopal-source-graph-and-current-inactive-wld-recount),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-448-aopal-tharnadian-causeway-title-history-and-room-identities),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-448-aopal-retained-with-source-only-approaches),
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97),
[candidate row 10](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L10), and
[next coverage row 362](ZONE_COVERAGE_REGISTER.csv#L362).

## Checkpoint 449: Astral placeholder reviewed

Physical coverage row 362, `component:astral`, now records a one-room,
source-only placeholder with no prose, exits, reset commands, mobiles,
objects, quests, or shops. The tracked map JSON contains only the zone ID and
room coordinate. Active Limbo is the sole loaded owner of room #1; nine active
Desert edges and fourteen source-only edges to that number remain Limbo or
source-map numeric records, not Astral ingress. Keep candidate row 11
**RETAIN IN PLACE** while the map owner resolves its purpose and editor,
persisted, and runtime identity.

The dossier and fifteen builder questions complete the repository-bounded
story, static graph, title/history, and typed-source review. Current inactive
WLD recounts remain 11,987 eligible source-only directions overall and 415
from Surf. The coverage table has 476 data rows and 61 columns;
case-insensitive PENDING counts are narrative 119, connectivity 293,
title/history 73, typed dependency 239, and builder questions 247, with 229
builder sections nonpending. The source-story set now contains 132 dossiers
covering 329/351 active contributors; Astral is unregistered and does not
change that active coverage. Continue at physical row 363. See [coverage row 362](ZONE_COVERAGE_REGISTER.csv#L362),
[Astral audit](ASTRAL_AUDIT_REVIEW.md),
[story dossier](../zone-stories/ASTRAL.md),
[builder questions](../zone-stories/ASTRAL_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-449-astral-placeholder-has-no-candidate-specific-route),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-449-astral-placeholder-and-room-1-identities),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-449-astral-retained-in-place),
[candidate row 11](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L11), and
[next coverage row 363](ZONE_COVERAGE_REGISTER.csv#L363).

## Checkpoint 450: Asunwa Jaguar Lair

Physical coverage row 363, `component:asunwa`, now records the source-only
108-room area, 25 mobiles, 18 objects, Balgar's one native quest contract,
and one Anan shop. The WLD area annotation is commented out, `qst/AREA`
remains active, `world.qst` and `world.shp` aggregates are absent, and the
shop loader's source glob fallback remains separate evidence from live
loading. Keep candidate row 12 **RETAIN IN PLACE**.

The graph contains 267 directions (266 local and one outbound to active
Surface), one weak component, 106/108 entrance-forward-reachable rooms, six
fall-chance rooms, fourteen current rooms, and eight matching door resets.
No active WLD edge enters Asunwa; the separate Surf approach is source-only.
The typed review records two external SHP references and an empty same-number
Player Guilds QST key without merging them into the local shop or Balgar
contract. The source story, boundary/hazard review, title/history review, and
eighteen builder questions are complete; no native area or gameplay file
changed.

The 476-row, 61-column coverage table now has case-insensitive PENDING counts
of narrative 118, connectivity 292, title/history 73, typed dependency 238,
and builder questions 246; 230 builder sections are nonpending. The
source-story dossier tally is 133, while active coverage remains 329/351
because Asunwa is unregistered. The inactive-WLD recount remains 11,987
eligible directions overall / 415 from Surf. Continue at physical row 364;
read its source identity from the coverage register. See [coverage row
363](ZONE_COVERAGE_REGISTER.csv#L363), [Asunwa audit](ASUNWA_AUDIT_REVIEW.md),
[story dossier](../zone-stories/ASUNWA.md),
[builder questions](../zone-stories/ASUNWA_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-450-asunwa-jaguar-lair-boundaries-and-hazards),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-450-asunwa-title-history-and-source-identities),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-450-asunwa-retained-with-mixed-manifest-dependencies),
[candidate row 12](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L12), and
[next coverage row 364](ZONE_COVERAGE_REGISTER.csv#L364).

## Checkpoint 451: Die Hall (`bastard`)

Physical coverage row 364 now records the five-room source-only Die Hall,
three mobiles, one object, and one shop. Its title is unique, but ZON header
#302 is shared with unregistered Sunhall. Active Underworld3 already owns the
same five room VNUMs as dwarven tunnels, while candidate Sunhall also defines
those numbers as pyramid rooms. Preserve the source identities and the
source-only up exit to `ud_map2` #1120721. Candidate row 13 remains **RETAIN
IN PLACE**.

The typed review records the Player Castles reset imports, Heavens/Verz1
stock references, duplicate #53/#108/#109 object definitions, missing
prototypes #6084/#6089/#6094, and the separate same-key #30201 shop for
Sunhall. The technical audit, story dossier, and twenty builder questions
complete the repository-bounded narrative, graph, identity, and dependency
review. No area or gameplay source changed.

Across 476 coverage rows and 61 columns, case-insensitive PENDING counts are
narrative 117, connectivity 291, title/history 73, typed dependency 237, and
builder questions 245; 231 builder sections are nonpending. The source-story
dossier tally is 134, while active coverage remains 329/351 because this
candidate is unregistered. The eligible inactive-WLD recount remains 11,987
directions overall / 415 from Surf; this candidate's one outbound target is
unregistered `ud_map2`, not an active destination. Continue at physical row
365; read its identity from the coverage register. See [coverage row
364](ZONE_COVERAGE_REGISTER.csv#L364), [Bastard audit](BASTARD_AUDIT_REVIEW.md),
[story dossier](../zone-stories/BASTARD.md),
[builder questions](../zone-stories/BASTARD_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-451-bastard-die-hall-room-ownership-and-source-boundaries),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-451-bastard-die-hall-room-ownership-and-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-451-bastard-retained-with-room-and-shop-collisions),
[candidate row 13](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L13),
[Sunhall candidate row 96](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L96),
[ud_map2 candidate row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114), and
[next coverage row 365](ZONE_COVERAGE_REGISTER.csv#L365).

Checkpoint 452 completes physical row 365, component:bb_gh, with a source
dossier, technical audit, and twenty builder questions. The source has 44
rooms, five WLD components, four reciprocal type-25 portal pairs, four
boundary exits, one shop, six Player Castles object imports, and a direct
numeric inn callback at #30303. It shares room and typed VNUMs with
unregistered vodgh and venom; two different SHP files use key #30301.
Candidate row 14 remains RETAIN IN PLACE. No areas/AREA registration, QST
source, or story/map sidecar was found. The active tharnrifts approach was
already included in the eligible inactive-WLD recount, so totals remain
11,987 overall / 415 Surf.

Across 476 coverage rows and 61 columns, case-insensitive PENDING counts are
narrative 116, connectivity 290, title/history 73, typed dependency 236, and
builder questions 244; 232 builder sections are nonpending. The
source-story dossier tally is 135, while active coverage remains 329/351.
Continue at physical row 366 after reading its identity from the coverage
register. See [coverage row 365](ZONE_COVERAGE_REGISTER.csv#L365),
[BB GH audit](BB_GH_AUDIT_REVIEW.md#title-history-and-disposition),
[story dossier](../zone-stories/BB_GH.md),
[builder questions](../zone-stories/BB_GH_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-452-bb-gh-static-graph-and-source-boundaries),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-452-bb-gh-title-history-and-source-identity),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-452-bb-gh-retained-with-portal-and-shop-collisions),
[candidate row 14](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L14), and
[next coverage row 366](ZONE_COVERAGE_REGISTER.csv#L366).

Checkpoint 453 completes physical row 366, component:bgtk, with a technical
audit and twelve builder questions. The SHP-only source has three records,
eleven products (five current Barrow objects and six missing prototypes),
one active keeper whose reset room differs from its shop room, and two keeper
VNUMs with no active mobile prototypes. In this checkout, the default
non-mini loader includes bgtk.shp because areas/world.shp is absent; static
source inspection finds an unguarded negative mobile-index path during shop
assignment. No runtime boot or shop behavior was tested. The source has no
area narrative or WLD component, so the story-dossier tally stays 135 and
connectivity totals stay 11,987 overall / 415 Surf. Candidate row 16 remains
RETAIN IN PLACE pending owner/runtime resolution.

Across 476 coverage rows and 61 columns, case-insensitive PENDING counts are
narrative 115, connectivity 289, title/history 73, typed dependency 235, and
builder questions 243; 233 builder sections are nonpending. Active coverage
remains 329/351. Continue at physical row 367 after reading its identity
from the coverage register. See [coverage row 366](ZONE_COVERAGE_REGISTER.csv#L366),
[BGTK audit](BGTK_AUDIT_REVIEW.md#default-full-mode-loader-path-and-risk),
[builder questions](BGTK_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-453-bgtk-has-no-wld-component),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-453-bgtk-shop-source-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-453-bgtk-retained-pending-shop-loader-review),
[candidate row 16](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L16),
[Barrow coverage row 87](ZONE_COVERAGE_REGISTER.csv#L87), and
[next coverage row 367](ZONE_COVERAGE_REGISTER.csv#L367).

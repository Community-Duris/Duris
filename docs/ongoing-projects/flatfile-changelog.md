# Flat-file persistence changelog

This file contains completed implementation history moved from the restoration
directive and the former persistence assessment.

It is a historical record and does not define current scope or authorize further work.
The restoration directive remains authoritative.

## Restoration checkpoints

### 2026-08-29 - restored database-only shop sale history in flat mode

- **Concrete gap:** shop sale frequency was stored only in `shop_trophy`. Client-free
  `sql_shop_trophy()` and `sql_shop_sell()` returned failure, so flat-mode appraisals
  never reduced the value of repeatedly sold items and committed flat sales never
  entered the seven-day history used by later appraisals.
- **Restoration:** flat-primary mode now retains the table's item vnum, sale value,
  seller PID, and sale time in one bounded rolling history under the existing private
  metadata authority. It preserves the database query's seven-UTC-calendar-day item
  count and its exclusions for mined ore and the 400000-400201 object range. A missing
  record is empty; writes are locked and atomically published; corrupt, oversized,
  unsafe, or symlinked authority cannot be read or overwritten. Only a successfully
  committed flat sale is recorded. MariaDB mode keeps its existing table operations,
  while a MySQL-capable binary configured as flat-primary uses the flat record rather
  than contacting MariaDB.
- **Focused evidence:** `python3 tests/async/test_flatfile_shop_trophy_history.py`
  covers missing state, item separation, the exact calendar-day boundary, rolling
  retention, private permissions, checksum corruption, overwrite refusal, and symlink
  refusal. Its route contracts cover client-free compatibility functions, runtime
  backend selection, and committed-sale publication. The shop command, live route,
  runtime, and repository tests pass, and the new test is included in client-free CI.
- **Build evidence:** complete builds pass with both
  `PERSISTENCE_BACKEND=flatfile` (without MySQL headers or client library) and the
  default MariaDB backend. The no-MySQL compatibility and persistence-mode contracts,
  formatting check, and `git diff --check` pass.
- **Overall state:** shop devaluation now operates in both persistence modes. This was
  a historically database-only gap; it does not complete the prior flat-file
  restoration mission or remove the global incomplete-domain boot fence.

### 2026-08-29 - restored historically database-only nexus persistence

- **Concrete gap:** nexus stones were always stored in the `nexus_stones` table, and
  client-free builds compiled out the entire gameplay implementation behind inert
  stubs. No stones loaded; lookups and boon criteria failed; bonuses returned zero
  instead of the original amount; and touches, expiry, staff lists/reloads, and enemy
  nexus selection could not operate without MariaDB.
- **Restoration:** the existing nexus gameplay code now compiles in both modes. No-DB
  mode preserves the table's exact stored fields—ID, name, room, alignment, stat affect,
  affect amount, last-touch time, and bonus—in one bounded nine-row authority record
  using the existing private metadata lock and atomic store. Missing state establishes
  an empty record, matching both the checked-in schema and the archived development
  database, which contain no seed rows; no room, bonus, or stone definition was
  invented. Configured rows drive the existing boot, info/boon, bonus, staff,
  touch/alignment, expiry, and random-enemy paths. Corrupt or unsafe authority aborts
  flat boot and cannot be overwritten. Alignment-save failure rolls the live alignment
  back, and expiry is made durable before removing the live stone. MariaDB queries
  remain in place; their info-row mapping now preserves `affect_amount` and the actual
  last-touch column, invalid load rows/rooms fail safely, and random selection no longer
  indexes beyond its candidates.
- **Focused evidence:** `python3 tests/async/test_flatfile_nexus_repository.py` covers
  empty establishment, exact field round trips, owner-only permissions, lookup,
  alignment and touch-time updates, live info routing, side-specific bonus behavior,
  missing and invalid mutations, checksum corruption, failed-live-read preservation,
  overwrite refusal, and symlink refusal. Its client-free preprocessing contract proves
  the full gameplay implementation is present while all nexus SQL is absent, and the
  test is included in client-free CI. The existing boon reward, epic transaction,
  currency, MySQL-result, save-logging, no-MySQL compatibility, and persistence-mode
  contracts pass.
- **Build evidence:** complete builds pass with both `PERSISTENCE_BACKEND=flatfile`
  (without MySQL headers or client library) and `PERSISTENCE_BACKEND=mariadb`; the
  client-free boot preflight, formatting check, and `git diff --check` pass.
- **Overall state:** configured nexus gameplay and persistence now operate in both
  modes. Other incomplete domains remain under audit, so the global incomplete-domain
  boot fence remains.

### 2026-08-29 - restored historically database-only cargo-market persistence

- **Concrete gap:** the global ship cargo and contraband modifiers were historically
  database-only. Client-free `read_cargo` and `write_cargo` returned failure, so player
  trades and staff changes were not durable; the scheduled cargo-maintenance job also
  attempted to acquire a SQL connection and repeatedly failed in no-DB mode.
- **Restoration:** no-DB mode now stores the 100 cargo and 100 contraband modifiers in
  one fixed-size, versioned, checksummed record in the existing private metadata
  authority. It uses the existing metadata lock and atomic publication primitive; a
  missing record establishes the normal initialized values, while corrupt or unsafe
  authority fails boot and cannot be overwritten. Reads stage and validate every value
  before replacing live matrices, and initialize the delayed matrix from current cargo
  exactly as the database loader does. Direct player/staff writes and the existing
  scheduled maintenance callback now use the same record, with scheduled timer updates
  continuing through the already-restored flat timer hooks. Derived prices remain
  derived rather than being duplicated on disk. The MariaDB table and transaction path
  remain in place, with row bounds and finite-modifier checks added before indexing the
  live matrices.
- **Focused evidence:** `python3 tests/async/test_flatfile_cargo_market.py` covers fresh
  authority, exact modifier round trips, delayed initialization, owner-only
  permissions, checksum corruption, overwrite refusal, live-state preservation,
  scheduled publication, timer advancement, malformed work rejection, and symlink
  refusal. Its preprocessing contracts verify that client-free live and maintenance
  paths do not enter cargo SQL. The existing cargo transaction, ship repository,
  maintenance scheduler, maintenance slicing, and persistence-mode tests pass, and the
  cargo test is included in client-free CI.
- **Build evidence:** complete builds pass with both `PERSISTENCE_BACKEND=flatfile`
  (without MySQL headers or client library) and `PERSISTENCE_BACKEND=mariadb`; the
  client-free boot preflight, formatting check, and `git diff --check` pass.
- **Overall state:** ship aggregates and their cargo-market state now persist in both
  modes. Historically database-only nexus state and other listed gaps remain, so the
  global incomplete-domain boot fence remains.

### 2026-08-29 - restored live flat ship persistence

- **Concrete gap:** ships were historically stored in `Ships/ship_index` plus one
  version-3 text file per owner. Although the branch already contained a safer complete
  ship catalog, live client-free `write_ship`, `read_ships`, and durable deletion all
  returned failure or did nothing. Ships could neither load nor survive a save, sale,
  re-owner, character deletion, or shutdown without MariaDB.
- **Restoration:** the existing catalog is now connected directly to the old ship
  runtime hooks; no replacement repository or generalized persistence layer was added.
  It assigns durable ship IDs, preserves owner identity, class, name, anchor, combat and
  sail state, armor, internal damage, crew, money, flags, and all 16 equipment/cargo
  slots, and atomically publishes create, update, re-owner, and removal mutations under
  the existing authority lock. Fresh state establishes an empty catalog. When the
  catalog is absent but the historical `Ships/ship_index` exists, a bounded one-time
  importer reads the exact version-3 owner files, applies the historical weapon/cargo
  normalization, resolves owner PIDs, and publishes the safe catalog without deleting
  the source files. Partial, oversized, unsafe, symlinked, duplicate, unknown-owner, or
  invalid-slot input fails before publication. Corrupt current authority cannot be
  overwritten. Flat boot now fails on an invalid ship authority; clean shutdown saves
  without starting a SQL transaction; failed durable deletion preserves the live ship;
  and committed flat character deletion also removes its live ship. The MariaDB
  backend remains table-backed and its successful load/save/delete behavior is
  unchanged.
- **Focused evidence:** `python3 tests/async/test_flatfile_ship_repository.py` covers
  current-format round trips, create/update/re-owner/remove behavior, durable ID and
  revision allocation, ownership collisions, private permissions, checksum corruption,
  overwrite refusal, exact legacy version-3 import and normalization, idempotent import,
  missing legacy state, partial-record rejection, symlink refusal, and non-publication
  after failed import. Its client-free preprocessing contract verifies live
  load/save/delete/shutdown/import routes and rejects ship SQL calls. Existing ship save,
  rename, transaction, shutdown, and character-delete regression tests also pass.
- **Build evidence:** complete builds pass with both
  `PERSISTENCE_BACKEND=flatfile` (without MySQL headers or client library) and
  `PERSISTENCE_BACKEND=mariadb`; the client-free boot preflight, persistence-mode
  contract, formatting check, and `git diff --check` pass.
- **Overall state:** the historical per-owner ship aggregate and its current database
  fields now have working live persistence in both modes. The global ship cargo-market
  matrices were historically database-backed and their client-free read/write hooks
  still return failure; that remains a separate concrete gap. The global
  incomplete-domain boot fence remains.

### 2026-08-29 - connected safe flat siege-object persistence

- **Concrete gap:** the historical siege subsystem saved its live object list to
  `Players/siege`, but the SQL conversion compiled both load and save completely out of
  client-free builds. Purchased and deployed siege objects therefore disappeared on
  restart, while destroyed objects were removed only from memory and could reappear in
  either persistence mode.
- **Restoration:** the existing siege hooks now save and load in client-free mode using
  one bounded authority record in the private metadata directory. It retains durable
  room vnums and reuses the existing item snapshot codec and detached materializer for
  each complete object tree; no new repository, transaction framework, or generalized
  subsystem was added. Publication uses the existing lock and atomic store, validates a
  version and checksum, refuses to overwrite corrupt authority, and stages every object
  before changing live rooms or the siege list. Removal now saves the replacement list,
  and the MariaDB saver now passes a durable room vnum to its existing loader rather
  than an in-memory room index.
- **Focused evidence:** `python3 tests/async/test_flatfile_siege.py` covers missing state,
  nested-object and room round trips, owner-only permissions, checksum corruption,
  overwrite refusal, live-state preservation, unknown-room rejection, destruction-save
  routing, and the absence of siege database calls from the client-free path. The test
  is included in the client-free CI job.
- **Build evidence:** a complete `SIEGE_ENABLED` server build passes with
  `PERSISTENCE_BACKEND=flatfile`, the changed siege translation unit passes with
  `PERSISTENCE_BACKEND=mariadb`, and the normal client-free boot preflight and
  persistence-mode contracts pass. Formatting and `git diff --check` also pass.
- **Overall state:** fresh and current-format siege persistence is connected in both
  modes. Automatic import of the historical unlengthened mixed text/binary
  `Players/siege` format remains unresolved and fails closed instead of discarding or
  unsafely parsing data. The `siege` boot-fence entry therefore remains until that
  compatibility question is completed.

### 2026-08-29 - restored historical flat town persistence

- **Concrete gap:** towns originally used the eight-line-per-town `Players/towns`
  format, but the SQL conversion replaced both client-free compatibility functions with
  unconditional failure. In a `SIEGE_ENABLED` client-free build, town state could not
  load, and donations, deployment changes, and staff adjustments could not save.
- **Restoration:** the existing compatibility functions now read and atomically replace
  that exact historical text format in the private flat metadata directory. A missing
  authority imports an existing `Players/towns` first or establishes the restored ten
  tracked `defaults/towns` records. Parsing is bounded and complete before live state is
  replaced; unknown or duplicate zones, partial records, unsafe files, and corrupt
  authority fail without overwriting disk, and client-free town boot stops on failure.
  The MariaDB table path is unchanged, and no new repository or generalized subsystem
  was introduced.
- **Focused evidence:** `python3 tests/async/test_flatfile_towns.py` exercises fresh
  defaults, all historical fields, exact-format publication, owner-only permissions,
  mutation/reload, legacy `Players/towns` import precedence, partial-record corruption,
  overwrite refusal, live-state preservation, symlink refusal, and the absence of town
  SQL from the client-free path. The test is included in the client-free CI job.
- **Build evidence:** complete `SIEGE_ENABLED` server builds pass with both
  `PERSISTENCE_BACKEND=flatfile` and `PERSISTENCE_BACKEND=mariadb`; the normal
  client-free boot preflight, persistence-mode contracts, formatting check, and
  `git diff --check` also pass.
- **Overall state:** persistent town resources, offense, defense, and deployment state
  are restored in both modes. Persisted siege objects and other incomplete domains
  remain separate work, so the global incomplete-domain boot fence remains.

### 2026-08-29 - database-independent global timers

- **Concrete gap:** under `__NO_MYSQL__`, `set_timer` discarded every write and
  `get_timer` always returned zero. The database-backed `timers` table is the functional
  reference for these named date values; current callers use them for cargo maintenance,
  trophy reduction, and epic-zone timing.
- **Restoration:** flat-file-only mode now persists each named timer beneath the existing
  flat-file metadata authority. Records use the existing atomic file publication path,
  owner-only metadata validation, a bounded name, an explicit version, and a checksum.
  Database-backed mode remains unchanged.
- **Focused evidence:** `python3 tests/async/test_flatfile_timers.py` covers missing,
  create, replace, negative-value compatibility, corruption, symlink, unsafe-name, and
  private-permission behavior. The test is included in the client-free CI job.
- **Build evidence:** `make -C src -j2`,
  `python3 tests/async/test_flatfile_boot_preflight.py`, `./scripts/format.sh --check`,
  and `git diff --check` pass.
- **Overall state:** the full objective is not complete. The global incomplete-domain
  boot fence remains in place while other concrete DB-free gaps are restored.

### 2026-08-29 - database-independent in-game help

- **Concrete gap:** `wiki_help` returned only a disabled message under `__NO_MYSQL__`,
  even though the help pages imported into the database are generated from tracked flat
  files in this repository.
- **Restoration:** flat-file-only mode now loads those existing sources directly and
  caches a bounded catalog. It follows the importer's existing precedence across the
  individual information files, `help_index`, and `duris_help_parsed.hlp`, then provides
  the established case-insensitive exact lookup, substring search, sorted results,
  result limit, missing-topic response, and redirects. Database-backed help remains
  unchanged.
- **Focused evidence:** `python3 tests/async/test_flatfile_help_catalog.py` loads more
  than 1,500 real topics, verifies source precedence and search behavior, and directly
  invokes `wiki_help` in a client-free runtime harness to prove that real content—not the
  former disabled stub—is returned. The test is included in the client-free CI job.
- **Build evidence:** `make -C src -j2`,
  `python3 tests/async/test_flatfile_boot_preflight.py`, `./scripts/format.sh --check`,
  and `git diff --check` pass.
- **Overall state:** the full objective is not complete. The global incomplete-domain
  boot fence remains in place while other concrete DB-free gaps are restored.

### 2026-08-29 - database-independent launcher and pre-boot backup

- **Concrete gap:** `cycle_mud.sh` required database credentials and ran migrations,
  schema verification, and MySQL shutdown logging in every mode. Its pre-boot backup
  selected `mysqldump` from the unrelated `REDIS` switch, so a flat-file-only launch
  could not be operationally independent of database tools.
- **Restoration:** launcher validation and database operations now follow the selected
  persistence mode. `flatfile-primary` requires only its absolute state root, avoids
  migration/schema/MySQL paths, and snapshots that complete state root to an explicitly
  separate owner-only backup before boot. A failed or unsafe snapshot stops the launch.
  Existing database-backed and fallback paths retain their database requirements.
- **Focused evidence:** `python3 tests/async/test_flatfile_launcher.py` validates all
  three mode contracts and runs an isolated supervised flat-file cycle with `REDIS=1`.
  It proves the selected state is backed up, no database-only command path is entered,
  and an unsafe nested backup destination fails closed. The test is included in the
  client-free CI job.
- **Build evidence:** `make -C src -j2`,
  `python3 tests/async/test_flatfile_boot_preflight.py`,
  `bash -n scripts/cycle_mud.sh scripts/backup_pfiles.sh`,
  `./scripts/format.sh --check`, and `git diff --check` pass.
- **Overall state:** the full objective is not complete. The global incomplete-domain
  boot fence remains in place while other concrete DB-free gaps are restored.

### 2026-08-29 - database-independent multiplay whitelist

- **Concrete gap:** the multiplay whitelist was historically database-only. In a
  client-free build, reads always returned an empty list and the existing staff add and
  remove command paths always failed, so approved shared-network exceptions could not
  be administered or survive a restart.
- **Restoration:** the existing whitelist read/add/remove functions now preserve the
  same visible fields and exact-pattern deletion behavior in `flatfile-primary`. They
  use one bounded, versioned, checksummed record in the existing metadata directory,
  published through the existing owner-only atomic store and serialized under a local
  lock. Missing state is an empty whitelist; corrupt or unsafe state grants no login
  exception and cannot be overwritten by a staff mutation. The database-backed path is
  unchanged.
- **Focused evidence:**
  `python3 tests/async/test_flatfile_multiplay_whitelist.py` exercises add/list/match,
  duplicate patterns, exact-pattern removal, missing-pattern compatibility, private
  permissions, checksum corruption, fail-closed matching, and refusal to overwrite a
  corrupt record. The test is included in the client-free CI job.
- **Build evidence:** `make -C src -j2`,
  `python3 tests/async/test_flatfile_boot_preflight.py`,
  `./scripts/format.sh --check`, and `git diff --check` pass.
- **Overall state:** the full objective is not complete. The global incomplete-domain
  boot fence remains in place while other concrete DB-free gaps are restored.

### 2026-08-29 - restored flat information pages

- **Concrete gap:** the no-MySQL implementations of `get_mud_info` and
  `send_mud_info` returned empty content and did nothing. This blanked the tracked news,
  motd, and wizmotd during boot and disabled information commands that request credits,
  FAQ, rules, or wizlist, even though those flat files remain the database import
  sources.
- **Restoration:** the client-free path now reads those exact allow-listed tracked
  files through the bounded flat content reader already used by in-game help, and
  `send_mud_info` once again sends the result. Unknown names cannot select arbitrary
  paths. Database-backed lookup remains unchanged; no writable format was added.
- **Focused evidence:** `python3 tests/async/test_flatfile_help_catalog.py` now verifies
  case-insensitive news lookup, motd and credits content, and traversal-name rejection.
  A client-free runtime harness directly invokes `get_mud_info` and `send_mud_info`, in
  addition to the test's existing direct `wiki_help` runtime coverage.
- **Build evidence:** `make -C src -j2`,
  `python3 tests/async/test_flatfile_boot_preflight.py`,
  `./scripts/format.sh --check`, and `git diff --check` pass.
- **Overall state:** the full objective is not complete. The global incomplete-domain
  boot fence remains in place while other concrete DB-free gaps are restored.

### 2026-08-29 - database-independent operational audit logs

- **Concrete gap:** the client-free `sql_log` implementation formatted more than one
  hundred player, staff, quest, experience, and connection call sites as SQL and passed
  them to a no-op query stub, silently discarding every event. The separate session
  login/logout audit hook was also empty.
- **Restoration:** client-free mode now routes those existing events to the ordinary
  durable player, staff, and experience log files, retaining the database-visible kind,
  IP, player ID and name, zone, room, and message fields. Formatting remains bounded;
  invalid or oversized events are rejected and record control characters are flattened
  to prevent forged log lines. The database-backed SQL and transactional session-audit
  paths are unchanged.
- **Focused evidence:** `python3 tests/async/test_flatfile_sql_log.py` invokes the real
  client-free `sql_log` and login-audit paths and verifies field preservation, file
  routing, control-character handling, invalid-status rejection, and oversized-message
  rejection. The test is included in the client-free CI job.
- **Build evidence:** `make -C src -j2`,
  `python3 tests/async/test_flatfile_boot_preflight.py`,
  `./scripts/format.sh --check`, and `git diff --check` pass.
- **Overall state:** the full objective is not complete. The global incomplete-domain
  boot fence remains in place while other concrete DB-free gaps are restored.

### 2026-08-29 - database-independent IP activity and one-hour rule

- **Concrete gap:** IP connection history was historically database-only. In a
  client-free build, connect and disconnect updates were discarded, player/staff IP
  information was always blank, and the racewar one-hour-side check received an invalid
  sentinel without initialized side data. This silently removed an existing gameplay
  restriction and made its caller unsafe.
- **Restoration:** the existing SQL compatibility functions now use one focused private
  flat metadata record containing PID, IP, connect/disconnect times, and racewar side.
  It is bounded, versioned, checksummed, atomically replaced under the existing local
  lock, and refuses mutation when corrupt. Same-IP lookup preserves the database's most
  recent-session behavior. Boot closes sessions left active by an interrupted run, and
  unavailable or corrupt history now denies the affected login instead of bypassing the
  rule. The database-backed `ip_info` path is unchanged.
- **Focused evidence:** `python3 tests/async/test_flatfile_ip_activity.py` exercises
  connect/disconnect persistence, latest same-IP selection, interrupted-session boot
  reset, compatibility reads, active and recently disconnected one-hour enforcement,
  owner-only permissions, checksum corruption, fail-closed lookup, and refusal to
  overwrite corrupt state. The test is included in the client-free CI job.
- **Build evidence:** `make -C src -j2`,
  `python3 tests/async/test_flatfile_boot_preflight.py`,
  `./scripts/format.sh --check`, and `git diff --check` pass.
- **Overall state:** the full objective is not complete. The global incomplete-domain
  boot fence remains in place while other concrete DB-free gaps are restored.

### 2026-08-29 - restored world quests without MySQL

- **Concrete gap:** world-quest completion history was historically database-only. The
  client-free daily-allowance and prior-target checks both returned `-1`; every normal
  quest request was therefore rejected, and target selection treated every candidate as
  already completed. World quests could not operate at all without the database.
- **Restoration:** client-free completion writes and the two existing gameplay checks
  now use one focused per-player history alongside the restored player files. Each entry
  retains the target, completion level, and timestamp needed for the database's permanent
  target exclusion and UTC daily/level allowance semantics. The history is bounded,
  versioned, checksummed, owner-only, atomically replaced under a per-player lock, and
  refuses to read or overwrite corruption. Database-backed quest history is unchanged.
- **Focused evidence:** `python3 tests/async/test_flatfile_world_quest_history.py`
  invokes the real client-free SQL compatibility functions and verifies missing-history
  allowance, completion/target retention, below-level-50 filtering, level-50 aggregate
  counting, prior-day exclusion, allowance exhaustion, owner-only permissions, checksum
  corruption, fail-closed gameplay reads, and refusal to overwrite corrupt history. The
  test is included in the client-free CI job.
- **Lifecycle evidence:** `python3 tests/async/test_flatfile_character_delete.py`
  establishes the new history and verifies crash-recovered and direct character deletion
  remove it in the existing multi-authority transaction.
- **Build evidence:** `make -C src -j2`,
  `python3 tests/async/test_flatfile_boot_preflight.py`,
  `./scripts/format.sh --check`, and `git diff --check` pass.
- **Overall state:** the full objective is not complete. The global incomplete-domain
  boot fence remains in place while other concrete DB-free gaps are restored.

### 2026-08-29 - connected artifact binding to flat authority

- **Concrete gap:** soul-binding fields were already part of the canonical flat artifact
  catalog, but the client-free `sql_get_bind_data` always failed and
  `sql_update_bind_data` discarded every change. Existing artifact pickup, ownership
  switching, feeding checks, and staff repair paths therefore could not use the flat data
  that had already been built.
- **Restoration:** the two existing compatibility functions now read and update binding
  owner/timer fields in that catalog. Updates use its existing global authority lock,
  transaction recovery, atomic replacement, checksums, and record/catalog revisions.
  Missing artifacts, invalid values, overflow, and corrupt or unavailable authority fail
  closed instead of synthesizing or overwriting state. The database-backed functions are
  unchanged.
- **Focused evidence:** `python3 tests/async/test_flatfile_artifact_repository.py` covers
  missing authority and vnums, binding round trips and updates, idempotence, revision
  advancement, record preservation, invalid input, and corrupt-state read/write refusal.
  Its client-free runtime harness directly invokes the real SQL compatibility functions.
- **Build evidence:** a clean full MariaDB build, the clean client-free build and boot
  preflight, `./scripts/format.sh --check`, and `git diff --check` pass.
- **Overall state:** this connects only the demonstrated binding gap. The broader artifact
  gameplay/read/update routes still require audit and restoration, so the global
  incomplete-domain boot fence remains in place.

### 2026-08-29 - connected artifact gameplay reads and updates

- **Concrete gap:** the canonical flat artifact record already retained ownership,
  location, poof timer, type, and update time, but `get_artifact_data_sql` and both
  `artifact_update_sql` overloads still used MySQL unconditionally. In a client-free
  build, ordinary artifact movement and feeding paths therefore could neither read nor
  update those existing flat fields.
- **Restoration:** those live compatibility paths now perform keyed reads and gameplay
  upserts through the existing artifact catalog. Existing records retain binding fields
  while gameplay fields and revisions advance; a newly encountered vnum receives the
  database path's ready-to-bind defaults. A missing catalog is never synthesized, and
  corrupt or invalid state cannot be read or overwritten. Existing owned/unowned return
  behavior, corpse-owner location preservation, timer normalization, and the MariaDB path
  remain unchanged.
- **Focused evidence:** `python3 tests/async/test_flatfile_artifact_repository.py` now
  covers keyed reads, existing-record updates, idempotence, binding preservation, revision
  advancement, sorted insertion, safe defaults, invalid input, and corruption refusal.
  A second client-free runtime harness invokes the real `get_artifact_data_sql` and typed
  `artifact_update_sql` overload directly, including missing and corrupt authority.
- **Build evidence:** `make -C src -j2`, the clean client-free build and boot preflight,
  `./scripts/format.sh --check`, and `git diff --check` pass.
- **Overall state:** core keyed artifact reads and updates are connected, but boot
  establishment plus listing, expiry, war-limit, and repair paths still require focused
  restoration. The global incomplete-domain boot fence remains in place.

### 2026-08-29 - restored flat artifact world placement at boot

- **Concrete gap:** normal world boot calls `setupMortArtiList_sql`,
  `addOnGroundArtis_sql`, and `addOnMobArtis_sql`. All three still depended on MySQL, so
  the client-free build discarded mortal-list setup and could not reconstruct owned
  artifacts whose authoritative locations were a room or an NPC.
- **Restoration:** client-free boot now validates and derives the mortal view from the
  canonical catalog, then filters that same catalog for owned ground and NPC records.
  Valid objects are recreated in the recorded real room or on the matching live NPC;
  unowned and unrelated records are ignored, and invalid room/NPC references are logged
  and the temporary object is extracted. The copyover gate and database-backed boot path
  are unchanged.
- **Focused evidence:** the client-free runtime portion of
  `python3 tests/async/test_flatfile_artifact_repository.py` directly invokes all three
  boot functions against mixed owned, unowned, ground, NPC, and invalid-location records.
  It verifies exactly the eligible artifacts are placed and invalid temporary objects are
  cleaned up.
- **Build evidence:** `make -C src -j2`, the clean client-free build and boot preflight,
  `./scripts/format.sh --check`, and `git diff --check` pass.
- **Overall state:** persisted ground/NPC placement is restored, but catalog
  establishment plus player-facing listing, expiry, war-limit, and repair paths remain.
  The global incomplete-domain boot fence remains in place.

### 2026-08-29 - restored flat artifact expiry scheduling

- **Concrete gap:** the bounded periodic artifact-expiry job still selected and cleared
  rows exclusively through MySQL. Flat artifact timers could now be read and updated, but
  an artifact whose timer elapsed was never selected for the existing world/player/corpse
  poof workflow and its catalog row was never cleared.
- **Restoration:** the existing one-vnum-per-slice event now selects the next owned,
  positive, elapsed timer from the canonical catalog after its existing cursor. After the
  unchanged cleanup workflow succeeds, a checked update clears that still-expired record
  to unowned/not-in-game with a null-equivalent timer and advances its revisions. Future,
  null-equivalent, missing, changed, corrupt, and unavailable state is not overwritten;
  failures retain the periodic retry/failure behavior. The MariaDB query/update path is
  unchanged.
- **Focused evidence:** `python3 tests/async/test_flatfile_artifact_repository.py` covers
  ordered cursor selection, future-timer exclusion, conditional expiry, idempotence,
  gameplay-field clearing, binding/type preservation, revision advancement, corruption
  refusal, and source-contract routing from the real periodic event. The bounded periodic
  maintenance source contract also passes.
- **Build evidence:** `make -C src -j2`, the clean client-free build and boot preflight,
  `python3 tests/async/test_nevent_maintenance_slicing.py`,
  `./scripts/format.sh --check`, and `git diff --check` pass.
- **Overall state:** ordinary artifact expiry is connected, but catalog establishment,
  war penalties, binding maintenance, listings, and staff repair routes remain. The
  global incomplete-domain boot fence remains in place.

### 2026-08-29 - restored artifact-war enforcement without MySQL

- **Concrete gap:** the periodic artifact-war rule still grouped owners and burned
  artifact timers exclusively through MySQL, so it could not enforce duplicate-type
  limits in no-database mode even though the flat catalog held the required fields.
- **Restoration:** no-database mode now groups the same player-location and artifact-type
  rows in PID order using the existing four-owner maintenance slice, applies the existing
  modifier and punishment levels, and atomically burns the affected owner's catalog
  timers with the same floor calculation. Existing forced-drop behavior is unchanged and
  now persists through the previously restored flat location updates. Database-backed
  behavior is unchanged.
- **Focused evidence:** the artifact repository/runtime test covers grouping, violation
  filtering, bounded PID paging, the original SQL treatment of player-located rows whose
  owned flag is false, timer flooring, null timers, binding preservation, revision
  progression, and corrupt-authority refusal. The source-contract test confirms the real
  war event uses both flat repository paths, and the maintenance-slicing regression still
  passes.
- **Build evidence:** `make -C src -j2`, the clean client-free build and boot preflight,
  `python3 tests/async/test_flatfile_artifact_repository.py`,
  `python3 tests/async/test_nevent_maintenance_slicing.py`,
  `./scripts/format.sh --check`, and `git diff --check` pass.
- **Overall state:** artifact-war enforcement is restored, but catalog establishment,
  binding maintenance, listings, and staff repair routes remain. The global
  incomplete-domain boot fence remains in place.

### 2026-08-29 - restored artifact binding maintenance without MySQL

- **Concrete gap:** the periodic soul-binding reconciliation still selected its work and
  wrote all three binding outcomes exclusively through `artifact_bind`, leaving the
  event unusable in no-database mode despite keyed flat binding reads and writes.
- **Restoration:** no-database mode now reads the canonical flat artifact catalog in the
  existing eight-artifact, vnum-cursor slices and applies the existing reconciliation
  rules unchanged. Soul merge, future-timer correction, and stale-binding clearance now
  use the atomic flat binding update; database-backed mode retains its existing SQL
  selection and update.
- **Focused evidence:** the flat artifact runtime tests exercise keyed binding reads,
  updates, idempotence, preservation of gameplay fields, revision progression, missing
  rows, and corrupt-authority refusal. The source-contract test confirms that the real
  maintenance event both pages flat authority and persists all three binding outcomes;
  the bounded-maintenance regression confirms its eight-row continuation slice remains.
- **Build evidence:** `make -C src -j2`, the clean client-free build and boot preflight,
  `python3 tests/async/test_flatfile_artifact_repository.py`,
  `python3 tests/async/test_nevent_maintenance_slicing.py`,
  `./scripts/format.sh --check`, and `git diff --check` pass.
- **Overall state:** periodic binding maintenance is restored, but catalog establishment,
  listings, staff repair, and other artifact command paths remain. The global
  incomplete-domain boot fence remains in place.

### 2026-08-29 - restored owned-artifact removal without MySQL

- **Concrete gap:** permanent object extraction, theft handoff, and the legacy corpse
  path still called `remove_owned_artifact_sql`, whose implementation read and mutated
  only MySQL. In no-database mode it therefore reported failure and could leave catalog
  ownership or binding state stale.
- **Restoration:** no-database mode now applies the existing outcome in one atomic catalog
  replacement: an artifact removed from play becomes unowned and not-in-game, an artifact
  moved to a player corpse remains owned at that PID, existing timer/type data is
  preserved, and soul binding is cleared in the same write. The historical recovery of a
  missing corpse-held row remains supported only when a valid catalog already exists;
  missing or corrupt authority still fails closed. Database-backed behavior is unchanged.
- **Focused evidence:** repository tests cover out-of-game removal, corpse placement,
  idempotence, timer/type preservation, binding clearance, missing-row recovery, invalid
  input, revision progression, and corrupt-authority refusal. The client-free runtime
  harness directly invokes the real `remove_owned_artifact_sql` compatibility function
  and verifies its complete persisted outcome.
- **Build evidence:** `make -C src -j2`, the clean client-free build and boot preflight,
  `python3 tests/async/test_flatfile_artifact_repository.py`,
  `python3 tests/async/test_nevent_maintenance_slicing.py`,
  `./scripts/format.sh --check`, and `git diff --check` pass.
- **Overall state:** core owned-artifact removal is restored, but catalog establishment,
  listings, staff repair, and remaining direct-SQL artifact paths still require focused
  work. The global incomplete-domain boot fence remains in place.

### 2026-08-29 - restored artifact lists without MySQL

- **Concrete gap:** the player-visible major, unique, and ioun lists were disabled at
  compile time without MySQL even though the canonical flat catalog already contained
  their ownership, location, timer, type, and update fields.
- **Restoration:** no-database mode now renders those lists directly from the catalog. It
  preserves mortal versus staff visibility, owned/all filtering, player and corpse owner
  labels, ground and NPC locations, countdowns, local last-update timestamps, and the
  existing race-war ownership summary. Missing or corrupt authority is reported as
  unavailable rather than as an empty list. Database-backed Redis/SQL behavior is
  unchanged.
- **Focused evidence:** the repository runtime test covers ordered catalog reads and
  corrupt-authority refusal, while the artifact source contract confirms that the real
  command reads flat authority and no longer contains its disabled-backend response.
- **Build evidence:** `make -C src -j2`, the clean client-free build and boot preflight,
  `python3 tests/async/test_flatfile_artifact_repository.py`,
  `python3 tests/async/test_nevent_maintenance_slicing.py`,
  `./scripts/format.sh --check`, and `git diff --check` pass.
- **Overall state:** ordinary artifact lists are restored, but catalog establishment,
  player-specific staff listing, staff repair, and remaining direct-SQL artifact paths
  still require focused work. The global incomplete-domain boot fence remains in place.

### 2026-08-29 - restored per-player artifact lookup without MySQL

- **Concrete gap:** the staff `artifact player` lookup remained an explicit no-MySQL
  compile-time stub after the ordinary type lists were restored.
- **Restoration:** no-database mode now resolves the requested name or PID through the
  existing flat identity routes, filters canonical catalog rows to artifacts held by that
  player or their corpse, and renders the existing owner, countdown, last-update, object,
  and vnum report. Missing or corrupt authority fails visibly; database-backed behavior
  is unchanged.
- **Focused evidence:** the catalog runtime test covers ordered reads and corruption
  refusal, and the artifact source contract confirms the real per-player command reads
  flat authority and no longer returns its MySQL-required stub.
- **Build evidence:** `make -C src -j2`, the clean client-free build and boot preflight,
  `python3 tests/async/test_flatfile_artifact_repository.py`,
  `python3 tests/async/test_nevent_maintenance_slicing.py`,
  `./scripts/format.sh --check`, and `git diff --check` pass.
- **Overall state:** player-specific lookup is restored, but catalog establishment, staff
  repair, and remaining direct-SQL artifact paths still require focused work. The global
  incomplete-domain boot fence remains in place.

### 2026-08-29 - restored artifact soul reset without MySQL

- **Concrete gap:** the confirmed staff soul-reset command still wrote only
  `artifact_bind`, so both its single-vnum and reset-all forms falsely failed in
  no-database mode.
- **Restoration:** a single-vnum reset now uses the existing keyed atomic catalog update,
  while the already privilege- and confirmation-gated reset-all form clears every binding
  in one atomic catalog replacement. Both preserve all gameplay fields and report missing
  or corrupt authority as failure. Database-backed SQL behavior is unchanged.
- **Focused evidence:** repository tests cover reset-all mutation, gameplay-field
  preservation, per-record and catalog revision progression, idempotence, and corrupt
  authority refusal; keyed reset coverage already verifies missing-vnum and corruption
  behavior. The artifact source contract confirms both real staff command routes use the
  flat authority in client-free builds.
- **Build evidence:** `make -C src -j2`, the clean client-free build and boot preflight,
  `python3 tests/async/test_flatfile_artifact_repository.py`,
  `python3 tests/async/test_nevent_maintenance_slicing.py`,
  `./scripts/format.sh --check`, and `git diff --check` pass.
- **Overall state:** staff soul reset is restored, but catalog establishment, fix/sync
  repair paths, destructive clear semantics, and remaining direct-SQL artifact routes
  still require focused work. The global incomplete-domain boot fence remains in place.

### 2026-08-29 - restored artifact feeding without MySQL

- **Concrete gap:** both the “feed to at least this duration” path and the normal feed
  recovery for an untracked/unowned artifact still issued direct SQL. In no-database mode
  those calls could neither extend an existing timer nor establish the feed outcome.
- **Restoration:** the minimum-feed path now uses one locked catalog mutation that applies
  `max(current, requested)` and refreshes last-update time without changing ownership,
  location, type, or binding. When no row exists in an already established catalog, the
  existing live object-location rules create the same ground/NPC/player state through the
  restored gameplay updater. Normal player feeding likewise records its player-owned
  recovery through that updater. Missing or corrupt catalog authority remains fail-closed;
  database-backed behavior is unchanged.
- **Focused evidence:** repository tests cover non-decreasing timer extension,
  last-update refresh, non-timer preservation, idempotence, missing rows, invalid input,
  revision progression, and corrupt-authority refusal. The client-free runtime harness
  invokes the real minimum-feed compatibility function and verifies its persisted result;
  the source contract confirms normal missing-row feeding also uses flat authority.
- **Build evidence:** `make -C src -j2`, the clean client-free build and boot preflight,
  `python3 tests/async/test_flatfile_artifact_repository.py`,
  `python3 tests/async/test_nevent_maintenance_slicing.py`,
  `./scripts/format.sh --check`, and `git diff --check` pass.
- **Overall state:** normal artifact feeding is restored, but catalog establishment,
  fix/sync repair paths, destructive clear semantics, and remaining direct-SQL artifact
  routes still require focused work. The global incomplete-domain boot fence remains.

### 2026-08-29 - restored artifact binding repair without MySQL

- **Concrete gap:** the staff `artifact reset fixit` reconciliation still selected
  player-held artifacts and repaired their binding/timer state only through MySQL.
- **Restoration:** no-database mode now selects player-held rows from the canonical
  catalog and, only when the soul owner differs from the recorded holder, atomically sets
  the soul owner, binding time, ten-day artifact timer, and last-update time in the same
  record replacement. Already-correct rows are left untouched; missing, moved, or corrupt
  rows fail safely. Database-backed behavior is unchanged.
- **Focused evidence:** repository tests cover complete repair outcome, gameplay-field
  preservation, already-correct idempotence, missing rows, revision progression, and
  corrupt-authority refusal. The artifact source contract confirms the real fixit command
  uses both flat selection and the atomic repair mutation in client-free builds.
- **Build evidence:** `make -C src -j2`, the clean client-free build and boot preflight,
  `python3 tests/async/test_flatfile_artifact_repository.py`,
  `python3 tests/async/test_nevent_maintenance_slicing.py`,
  `./scripts/format.sh --check`, and `git diff --check` pass.
- **Overall state:** binding repair is restored, but catalog establishment, player-save
  sync, destructive clear semantics, and remaining direct-SQL artifact routes still
  require focused work. The global incomplete-domain boot fence remains in place.

### 2026-08-29 - restored artifact record clearing without MySQL

- **Concrete gap:** both the internal artifact-row removal helper and the existing staff
  `artifact clear` command still issued SQL deletes, so no-database mode left the
  canonical record intact while the command path could proceed as though it were gone.
- **Restoration:** client-free builds now erase the requested canonical catalog record
  under the authority lock and replace the checksummed catalog atomically. Because flat
  gameplay and binding state share that canonical record, the erase removes the complete
  artifact entry rather than leaving detached binding data. Missing or corrupt authority
  is never overwritten. Database-backed table behavior is unchanged.
- **Focused evidence:** repository tests cover successful erase, absence from subsequent
  lookup, repeated missing-row behavior, and corrupt-authority refusal. Source contracts
  confirm both the internal removal helper and real staff clear command route through the
  catalog erase in client-free builds.
- **Build evidence:** `make -C src -j2`, the clean client-free build and boot preflight,
  `python3 tests/async/test_flatfile_artifact_repository.py`,
  `python3 tests/async/test_nevent_maintenance_slicing.py`,
  `./scripts/format.sh --check`, and `git diff --check` pass.
- **Overall state:** record clearing is restored, but catalog establishment, player-save
  sync, deletion cleanup compatibility, and any remaining executable direct-SQL artifact
  routes still require focused work. The global incomplete-domain boot fence remains.

### 2026-08-29 - established artifact authority on fresh flat boot

- **Concrete gap:** every restored artifact route intentionally failed closed when the
  catalog was absent, but no live boot path created that catalog. Initial zone resets run
  from `ne_init_events()` before the later artifact setup call, so a fresh no-database
  state could not persist its first spawned artifact.
- **Restoration:** flat-primary boot now ensures a valid artifact catalog immediately at
  the start of `boot_db`, before event initialization and zone resets. A missing catalog is
  created as an empty, checksummed authority; any valid existing catalog, including a
  nonempty one, is preserved exactly; corrupt authority aborts boot rather than being
  replaced. Database-backed boot does not invoke this path.
- **Focused evidence:** repository tests cover fresh empty creation, first gameplay insert,
  idempotent recognition and preservation of a nonempty catalog, and corrupt-authority
  refusal. The source contract verifies the live boot call occurs before
  `ne_init_events()`, and the persistence-mode runtime/build contract remains green.
- **Build evidence:** `make -C src -j2`, the clean client-free build and boot preflight,
  `python3 tests/async/test_flatfile_artifact_repository.py`,
  `python3 tests/async/test_nevent_maintenance_slicing.py`,
  `python3 tests/async/test_persistence_mode.py`, `./scripts/format.sh --check`, and
  `git diff --check` pass.
- **Overall state:** fresh artifact catalog establishment is restored, but player-save
  sync, deletion cleanup compatibility, and any remaining executable direct-SQL artifact
  routes still require focused work. The global incomplete-domain boot fence remains.

### 2026-08-29 - restored saved-artifact reconciliation without MySQL

- **Concrete gap:** the existing staff `artifact reset syncdb` repair command still read
  `player_items` and rewrote artifact ownership exclusively through SQL. The legacy bulk
  deletion helper also retained an executable SQL update in client-free builds, even
  though normal flat character deletion used the newer atomic deletion path.
- **Restoration:** client-free `syncdb` now enumerates active player custody from the
  existing flat item-ownership catalog and reconciles known artifacts to those saved
  owners. It clears stale player/corpse locations and bindings, preserves artifact
  timers/types and unrelated world locations, and refuses duplicate saved ownership
  rather than selecting an arbitrary player. The deletion helper now commits the existing
  flat player-release mutation. Database-backed SQL behavior is unchanged.
- **Focused evidence:** artifact repository tests cover saved-owner recovery (including an
  offline owner), stale player/corpse cleanup, unrelated-location preservation, binding
  reset/rebuild, idempotent retry, duplicate-owner refusal without mutation, standalone
  player release, and corrupt-authority refusal. Item repository tests cover active-player
  enumeration and corrupt-authority refusal. Source contracts verify both live command
  routes use those flat authorities.
- **Build evidence:** `make -C src -j2`, the clean client-free build and boot preflight,
  `python3 tests/async/test_flatfile_artifact_repository.py`,
  `python3 tests/async/test_flatfile_item_repository.py`,
  `python3 tests/async/test_nevent_maintenance_slicing.py`,
  `python3 tests/async/test_persistence_mode.py`, `./scripts/format.sh --check`, and
  `git diff --check` pass. Preprocessing `artifact.c` for `__NO_MYSQL__` leaves no
  executable `qry_at` or `sql_trace_exec_at` call sites.
- **Overall state:** artifact saved-player repair and deletion compatibility are restored,
  and the no-MySQL artifact implementation has no executable direct-SQL paths. Other
  persistence domains still require focused audit; the global incomplete-domain boot
  fence remains.

### 2026-08-29 - restored trusted shop purchases in flat mode

- **Concrete gap:** the database-backed shop path lets trusted staff buy an item without
  charging their purse, but the flat path rejected that purchase with an explicit
  "not available" message because its durable command required every purchase to have a
  positive price.
- **Restoration:** trusted flat-file purchases now submit the existing durable shop trade
  with a zero transaction price. Player money remains unchanged while shop inventory,
  item custody, materialization evidence, and live publication follow the same existing
  atomic transfer as an ordinary purchase. Produced multi-item continuations retain the
  zero price. Ordinary players are still charged the computed sale price, zero-price
  sales remain invalid, and database-backed behavior is unchanged.
- **Focused evidence:** command-codec tests accept zero-price buys while rejecting
  zero-price sales; the live-route contract confirms the old flat-only rejection is gone
  and the trusted price reaches initial and continued purchases. The flat shop repository
  test commits a complimentary purchase and verifies unchanged durable money plus exact
  player/shop custody transfer.
- **Build evidence:** `make -C src -j2`,
  `python3 tests/async/test_shop_trade_command.py`,
  `python3 tests/async/test_shop_trade_live_route.py`,
  `python3 tests/async/test_shop_trade_runtime.py`,
  `python3 tests/async/test_flatfile_shop_trade_repository.py`,
  `python3 tests/async/test_minimal_boot.py`,
  `python3 tests/async/test_persistence_mode.py`, `./scripts/format.sh --check`, and
  `git diff --check` pass.
- **Overall state:** the explicit flat shop purchase disablement is removed. Other
  database-only or incomplete persistence behavior still requires focused audit; the
  global incomplete-domain boot fence remains.

### 2026-08-29 - restored live guild authority without MySQL

- **Concrete gap:** the historical flat-file implementation loaded and saved guild state
  through `Guild::initialize()` and `Guild::save()`, but both hooks had been changed to do
  nothing in a client-free build. Guild creation and every later mutation therefore lost
  their state, normal boot loaded no guilds, deletion did not affect the newer flat
  authority, and the prestige list still executed SQL.
- **Restoration:** client-free guild boot, mutation, creation, deletion, and prestige
  display now use the existing association catalog already present in this worktree.
  Missing authority establishes an empty catalog or imports the historical `asc.*` files;
  the old parser was bounded and corrected so a legitimate empty top-fragger field cannot
  consume the first rank title. Catalog materialization resolves canonical member IDs back
  to their player-name case and fails boot on corrupt or mismatched authority. Saves and
  deletes retain the catalog's existing locking, checksums, atomic publication, revisions,
  and cross-guild PID uniqueness. Database-backed guild behavior remains unchanged.
- **Focused evidence:** `python3 tests/async/test_flatfile_association_repository.py`
  covers unchanged and changed saves, member/association revisions, create, delete retry,
  duplicate-member refusal, corruption read/write refusal, historical-parser safety, and
  the preprocessed live no-MySQL routes. Character-deletion repository and manifest tests
  still pass with the same association authority.
- **Build evidence:** `make -C src -j2`, the clean client-free build and boot preflight,
  `python3 tests/async/test_flatfile_character_delete.py`,
  `python3 tests/async/test_flatfile_character_delete_manifest.py`,
  `python3 tests/async/test_persistence_mode.py`, `./scripts/format.sh --check`, and
  `git diff --check` pass.
- **Overall state:** core guild state is now durable and loadable without MySQL. The
  historically database-only guild transaction ledger is still the only executable SQL
  cluster in client-free `assocs.c`; alliances, guildhalls, and outposts remain separate
  demonstrated gaps. The global incomplete-domain boot fence therefore remains.

### 2026-08-29 - restored the database-only guild ledger in flat mode

- **Concrete gap:** guild deposit and withdrawal history was always stored in the
  `guild_transactions` table. A client-free build sent each transaction to the inert SQL
  stub, and the leader-only `soc ledger` command could neither retrieve player activity
  nor automated system withdrawals.
- **Restoration:** the existing association repository now stores the ledger alongside
  flat guild authority. Writes retain the established visible transaction text and
  player/system classification; reads return newest first and retain the command-visible
  latest 100 entries for each category. The record is bounded, versioned, checksummed,
  owner-only, locked with the existing authority, and atomically replaced. Missing history
  is an empty ledger, while corrupt history cannot be read or overwritten. The MariaDB
  insert and query paths remain unchanged.
- **Focused evidence:** `python3 tests/async/test_flatfile_association_repository.py`
  covers missing history, append, player/system filtering, newest-first ordering, the
  per-category 100-entry retention boundary, category independence, control-character
  rejection, checksum corruption, and write refusal after corruption. Its client-free
  source contract verifies both live ledger routes and rejects every remaining
  `guild_transactions` query.
- **Build evidence:** `make -C src -j2`, the clean client-free build and boot preflight,
  `python3 tests/async/test_flatfile_character_delete.py`,
  `python3 tests/async/test_flatfile_character_delete_manifest.py`,
  `python3 tests/async/test_persistence_mode.py`, `./scripts/format.sh --check`, and
  `git diff --check` pass.
- **Overall state:** preprocessing `assocs.c` for `__NO_MYSQL__` now leaves no executable
  `qry_at`, `db_query_at`, or `sql_trace_exec_at` calls. Guild core state and its
  historically database-only ledger are routed, but alliances, guildhalls, and outposts
  remain separate gaps; the global incomplete-domain boot fence remains.

### 2026-08-29 - restored database-only alliances in flat mode

- **Concrete gap:** alliances were historically stored only in the `alliances` table;
  both `load_alliances()` and `save_alliances()` returned without doing anything in a
  client-free build. Forged alliances therefore disappeared on restart, and severing an
  alliance could not durably update flat-only state.
- **Restoration:** the existing association repository now stores the complete set of
  forging/joining guild pairs and their tribute values alongside flat guild authority.
  Boot resolves every stored ID after guilds load and aborts on corrupt state or a
  missing guild reference. Replacement enforces the live rules that no guild appears in
  more than one alliance and no guild allies with itself, while retaining the existing
  authority lock, versioning, checksum, private atomic publication, and corruption
  refusal. Missing state means no alliances. The MariaDB load/save path remains
  unchanged.
- **Focused evidence:** `python3 tests/async/test_flatfile_association_repository.py`
  covers missing state, canonical round trips, tribute preservation, idempotence, empty
  replacement, duplicate-guild and self-alliance rejection, checksum corruption, and
  refusal to overwrite corrupt authority. Its client-free source contract verifies the
  live load/save routes and rejects all alliance queries from that build path.
- **Build evidence:** `make -C src -j2`, the clean client-free build and boot preflight,
  `python3 tests/async/test_flatfile_character_delete.py`,
  `python3 tests/async/test_flatfile_character_delete_manifest.py`,
  `python3 tests/async/test_persistence_mode.py`, `./scripts/format.sh --check`, and
  `git diff --check` pass.
- **Overall state:** alliances now survive forging, severing, and restart in either
  persistence mode. Guildhalls and outposts remain separate demonstrated gaps; the
  global incomplete-domain boot fence remains.

### 2026-08-29 - restored database-only guildhalls in flat mode

- **Concrete gap:** guildhalls and their generated rooms were historically stored only
  in the `guildhalls` and `guildhall_rooms` tables. In a client-free build, both loaders
  returned empty state, save and delete hooks reported success without persistence, and
  fresh ID allocation began at an invalid value. Construction appeared to work but could
  not survive reload or restart.
- **Restoration:** the existing association repository now stores the two tables' exact
  hall and room fields as one bounded guildhall catalog. A hall mutation publishes the
  hall and its at-most-50 rooms atomically under the existing authority lock, version,
  checksum, private permissions, and corruption refusal. Boot restores owning guilds,
  specialized room types, values, exits, names, and next IDs; missing authority means no
  halls, while corrupt or dangling ownership aborts boot. Create, room construction,
  upgrades, renames, moves, reloads, golem state, and deletion use the existing live
  hooks. Flat guild deletion erases owned halls before guild authority so it cannot
  create a dangling durable reference. The MariaDB path remains unchanged.
- **Focused evidence:** `python3 tests/async/test_flatfile_association_repository.py`
  covers missing state, canonical full-field round trips, idempotence, updates, private
  permissions, per-hall room limits, duplicate room ID/vnum rejection within and across
  halls, control-character rejection, room and hall erasure, checksum corruption, and
  read/write/delete refusal after corruption. Client-free source contracts verify every
  live load/save/delete route, ordered guild cleanup, one-snapshot saves, and the absence
  of guildhall SQL from the no-database build.
- **Build evidence:** `make -C src -j2`, the clean client-free build and boot preflight,
  `python3 tests/async/test_flatfile_character_delete.py`,
  `python3 tests/async/test_flatfile_character_delete_manifest.py`,
  `python3 tests/async/test_persistence_mode.py`, `./scripts/format.sh --check`, and
  `git diff --check` pass.
- **Overall state:** guildhall construction and administration are now durable in either
  persistence mode. Outposts remain a separate, broader database-only gap; the global
  incomplete-domain boot fence remains.

### 2026-08-29 - restored database-only outposts in flat mode

- **Concrete gap:** outposts were historically backed only by the fixed three-row
  `outposts` table. The client-free build replaced the entire implementation with
  do-nothing stubs: it loaded no towers, disabled the player/staff command and defenses,
  returned zero health/ownership, discarded damage and resource updates, and skipped
  upkeep. No outpost gameplay could operate without MariaDB.
- **Restoration:** the existing association repository now retains all 13 table fields
  for the three canonical outposts. Fresh flat boot establishes the schema defaults and
  then runs the existing building, room, wall, portal, guard, combat, repair, reset,
  command, and upkeep code. Ownership, hitpoints, golems, archers, meurtriere, portal,
  and reset mutations use the bounded, versioned, checksummed, private atomic authority;
  corrupt or incomplete state fails boot and cannot be overwritten. Neutral ownership
  no longer dereferences a null guild, and flat guild deletion durably neutralizes owned
  outposts before removing guild authority. The legacy wood/stone harvest branch remains
  intentionally inactive in both modes because its only gameplay caller is commented
  out. MariaDB retains the same table-backed behavior.
- **Focused evidence:** `python3 tests/async/test_flatfile_association_repository.py`
  covers missing authority, rejection of incomplete fixed sets, canonical establishment,
  schema defaults, all-field mutation round trips, idempotence, private permissions,
  invalid flag and negative-health rejection, checksum corruption, and read/write/create
  refusal after corruption. Client-free source contracts verify fresh establishment,
  every active mutation route, the full upkeep/gameplay implementation, ordered guild
  cleanup, and the absence of outpost or legacy resource SQL from the no-database build.
- **Build evidence:** `make -C src -j2`, the clean client-free build and boot preflight,
  `python3 tests/async/test_flatfile_character_delete.py`,
  `python3 tests/async/test_flatfile_character_delete_manifest.py`,
  `python3 tests/async/test_persistence_mode.py`, `./scripts/format.sh --check`, and
  `git diff --check` pass.
- **Overall state:** guilds, their historically database-only ledger, alliances,
  guildhalls, and outposts now have live persistence routes in both modes. Other
  incomplete domains remain under audit, so the global incomplete-domain boot fence
  remains.

### 2026-08-29 - restored database-only polls in flat mode

- **Concrete gap:** polls were historically stored only in the `polls`, `poll_options`,
  and `poll_votes` tables. In a client-free build, every list, lookup, create, close,
  expiry, account-vote check, and vote mutation returned empty or failure. The websocket
  vote handler separately rejected every request as database unavailable, so neither
  the in-game nor web poll system could operate without MariaDB.
- **Restoration:** the existing poll APIs now retain the same poll, option, and
  account-level vote fields in one bounded flat authority record. It uses the existing
  private metadata directory, an exclusive mutation lock, version and checksum
  validation, and atomic replacement. Missing state is an empty poll list; corrupt state
  is not accepted or overwritten. Poll and option IDs advance durably, active lists stay
  newest-first, vote totals count distinct accounts, multi-select option totals survive
  restart, and create, close, expiry, in-game voting, websocket voting, and websocket
  broadcasts use the existing command paths. MariaDB behavior remains table-backed and
  unchanged.
- **Focused evidence:** `python3 tests/async/test_flatfile_polls.py` covers missing state,
  invalid definitions, durable identity allocation, restart reads, active ordering,
  case-insensitive account voting, multi-select totals, duplicate refusal, expiry,
  private permissions, checksum corruption, and refusal to overwrite corrupt authority.
  Its client-free preprocessing check also rejects every poll SQL statement, and the
  test is included in the client-free CI job.
- **Build evidence:** `make -C src -j2`,
  `python3 tests/async/test_flatfile_boot_preflight.py`,
  `python3 tests/async/test_websocket_protocol_contract.py`,
  `python3 tests/async/test_persistence_mode.py`, `./scripts/format.sh --check`, and
  `git diff --check` pass.
- **Overall state:** the active poll system now has live persistence routes in both
  modes. Other incomplete domains remain under audit, so the global incomplete-domain
  boot fence remains.


## Historical assessment record

The assessment was performed on 2026-08-28 at revision `68a916ec`, using
`97a4166c3fa10448b778a35e16854ad5b3e5e294` as the last pre-player-migration comparison
point. Its current-state findings, proposed architecture, and implementation order were
later superseded by the restoration directive. The enduring historical evidence is
retained here.

### Player migration boundary

| Revision | Date | Historical significance |
|---|---|---|
| `97a4166c` | 2025-12-28 | Last revision before the player-to-database project; restoration comparison point. |
| `35f66dfc` | 2025-12-28 | Began phase-one player schema and `sql_player` work. |
| `732859d6` | 2025-12-28 | Added SQL player save/load. |
| `dd06c92e` | 2025-12-28 | Introduced dual pfile and SQL writes. |
| `6770ce74` | 2026-01-01 | Moved accounts and many player/world domains toward SQL. |
| `27ac3084` | 2026-04-19 | Replaced flat existence/delete/rename behavior with SQL equivalents. |
| `4f6b5fdf` | 2026-06-14 | Introduced asynchronous database persistence work. |
| `28735fde` / `a16731bb` | 2026-08-27 | Moved nonterminal saves to the snapshot pipeline and added terminal fences. |
| `f3e39720` | 2026-08-27 | Retired normal legacy flat save forks. |

SQL had existed for years before this boundary. The reference revision represents the
player/account flat-file era and was already a hybrid, MariaDB-dependent server.

### Historical player and account formats

Player files lived at `Players/<lowercase-first-character>/<lowercase-name>`. At the
reference revision the binary record declared player save version 5, status version 47,
skill version 2, item version 35, affect version 8, and witness version 2, with a nominal
240,000-byte maximum. It included identity and descriptions; stats, race, classes,
levels, flags, conditions, currencies, and bank values; trophies, languages,
introductions, timers, skills, witnesses, and affects; and recursive inventory and
equipment. Objects were prototype vnums plus differences, tying recovery to compatible
world prototypes and codec assumptions.

Account files lived at `Accounts/<lowercase-first-character>/<lowercase-account-name>`.
The line-oriented format retained account identity, email, credentials, known IPs,
flags, timestamps, and character membership metadata. Account and player files were
published independently and could become inconsistent.

### Other historical file-backed domains

| Domain | Representation at `97a4166c` | Important limitation |
|---|---|---|
| Lockers | Synthetic `<name>.locker` pfile records | Access lists already depended on SQL. |
| Corpses | `Players/Corpses/<owner><save-id>` object graphs | Backup rotation without fully durable publication. |
| Saved world items | `Players/SavedItems/item.<word>.<pointer>` | Process pointers were used as filename identity. |
| Shopkeepers | `Players/ShopKeepers/<shop-id>` aggregate files | Separate pfile-like whole-entity records. |
| Guilds | `Players/Assocs/asc.<id>` plus `.motd` | Sparse IDs could be missed; alliances were SQL-backed. |
| Towns | `Players/towns` | Direct whole-file rewrite. |
| Siege | `Players/siege` mixed text/binary | Direct rewrite and no atomic multi-record publication. |
| Ships | `Ships/ship_index` plus `Ships/<owner>` | Index and owner file lacked a cross-file transaction. |
| Recipes | `Players/Tradeskills/<letter>/<name>.crafting` | Independent from rename and player lifecycle. |
| Shapechange | `Players/Shapechange/<letter>/<Name>` | Direct rewrite without transaction with the pfile. |
| Pets | Intended `Players/Pets/<id>` | Writer path and item restoration were already incomplete. |
| Mail, boards, admin state | Independent binary/text stores | Inconsistent durability and no shared transaction boundary. |

The baseline already used SQL for alliances, outposts, nexus stones, cargo-market
state, locker/private-chest access metadata, artifacts, and other gameplay/economy
records. The migration utility in `src-migrate/` corroborates the old layouts but was a
one-way flat-to-SQL importer.

### Historical durability findings

- Native scalar layouts, prototype-delta objects, and independently versioned text
  formats limited portability and long-term schema compatibility.
- Most writers used direct truncation or simple `.bak` rotation without the full
  write-sync-rename-parent-sync sequence. Multi-file entities had no atomic boundary.
- Historical terminal character saves could extract live inventory before the new file
  was safely published, allowing save failure to destroy items.
- Cross-character and cross-domain operations had no general operation identity,
  atomicity, or idempotent retry contract.
- `Players/pc_idnumb` was a truncate-and-rewrite counter without safe concurrency or
  durable atomic publication; some saved-item names used process pointers.
- Static serializer buffers and late size validation provided weak bounds, concurrency,
  and corruption handling.
- Historical backups omitted accounts and other identity/domain state and copied a live
  changing tree without a consistent generation, so they were not complete recovery
  points.

The assessment's stale current-state snapshot and superseded speculative design/phase
plan were intentionally not retained here. The original remains available in Git
history before its deletion.

## Earlier implementation checkpoints

This section was the durable implementation handoff ledger in the original assessment.
Enduring requirements were folded into the restoration directive; embedded "next
action" text below remains historical and does not authorize work.

### Completed milestone

| Milestone | State | Evidence |
|---|---|---|
| P0 - real DB-free boundary | Complete | Client-free binary links without system MySQL dependencies; isolated boot preflight and no-MySQL CI job exist |

### Checkpoint 1 - assessment preserved and baseline reproduced

- **Revision started from:** `68a916ec`
- **Published revision:** `fefa7291`
- **Completed:** source/history assessment, production requirements, implementation
  order, and acceptance-test inventory.
- **Baseline command:** the isolated `make -C src ... EXTRA_CFLAGS=-D__NO_MYSQL__ -j2`
  command documented below was rerun on 2026-08-28.
- **Baseline result:** failed as expected. `sql.h` exposed unguarded `MYSQL` declarations,
  `ships/ship_base.c` could not see transaction helpers, the compiler command still used
  `/usr/include/mysql`, and the Makefile still selected `-lmysqlclient` for linking.
- **Current implementation state:** no production backend behavior is complete yet.
- **Next action:** implement the P0 compile-time boundary and explicit persistence-mode
  configuration, with focused source-contract tests before attempting a clean DB-free
  server build.

### Checkpoint 2 - explicit mode and dependency boundary

- **Completed:** added `PERSISTENCE_BACKEND=mariadb|flatfile` to the server build. The
  flat selection defines `__NO_MYSQL__` and removes both the MySQL include path and
  `-lmysqlclient` from its compile/link configuration.
- **Completed:** added runtime parsing for `mariadb-primary`,
  `mariadb-primary-flatfile-fallback`, and `flatfile-primary`. The MariaDB mode remains
  the default.
- **Completed:** flat modes require an absolute `FLATFILE_STATE_DIR`, reject symlinks,
  wrong ownership, and group/world permissions, and provision the initial authority
  topology at mode `0700`.
- **Completed:** incomplete flat modes fail boot with an explicit inventory of
  unimplemented durable domains instead of continuing with SQL no-op/empty stubs.
- **Checks passed:** `python3 tests/async/test_persistence_mode.py`,
  `./scripts/format.sh --check`, `git diff --check`, and the normal `make -C src -j2`
  server build.
- **Flat build evidence:** isolated `make -C src PERSISTENCE_BACKEND=flatfile ... -j2`
  commands no longer contain a MySQL include or client library. The build still fails
  because `sql.h` leaks `MYSQL` in three declarations and `ships/ship_base.c` calls
  transaction helpers that are absent under `__NO_MYSQL__`.
- **Files changed:** `src/Makefile`, `src/comm.c`, `src/persistence_mode.[ch]`,
  `tests/async/test_persistence_mode.py`, `docs/guides/BUILDING.md`, and
  `docs/operations/CONFIGURATION.md`.
- **Next action:** remove MySQL connection types from common interfaces, guard or route
  ship shutdown persistence through a backend interface, then iterate the isolated flat
  build to its next compile boundary.

### Checkpoint 3 - client-free mixed-module compile surface

- **Completed:** exposed the existing fail-closed `sql_player.c` no-MySQL declarations
  to common callers and added null/error query stubs for mixed modules. Ship shutdown now
  resolves its transaction calls to the existing false-returning no-MySQL implementation
  instead of failing compilation.
- **Completed:** added `src/no_mysql/` compatibility declarations for legacy units that
  still name MySQL result/statement types. The flat build selects these local headers;
  they are not a client implementation, allocate no connection, and make every client
  initialization, connection, query, and prepared-statement operation fail.
- **Completed:** repaired configuration-specific compile errors in account rewards,
  administrator IP lookup, artifact reporting, auction rooms, and CTF stubs without
  weakening the warning profile.
- **Checks passed:** `python3 tests/async/test_persistence_mode.py`,
  `python3 tests/async/test_no_mysql_compat.py`,
  `python3 tests/async/test_account_bound_reward_contract.py`,
  `./scripts/format.sh --check`, `git diff --check`, and `make -C src -j2`.
- **Flat build evidence:** the isolated client-free build now compiles through ships,
  accounts, rewards, administration, artifacts, associations, auctions, buildings, CTF,
  database boot support, crafting, and several gameplay modules. Its next failures are
  `__NO_MYSQL__` unused-parameter diagnostics in `epic.c`; it has not linked yet.
- **Files changed:** `src/no_mysql/mysql.h`, `src/no_mysql/mysql/mysql.h`, `src/sql.[ch]`,
  `src/sql_player.h`, `src/account_reward.c`, `src/actwiz.c`, `src/artifact.c`,
  `src/auction_houses.c`, `src/ctf.c`, `src/Makefile`, and
  `tests/async/test_no_mysql_compat.py`.
- **Next action:** continue the warning-clean client-free compile sweep from `epic.c`,
  then resolve missing no-MySQL link symbols and add a boot preflight test.

### Checkpoint 4 - warning-clean legacy stub sweep

- **Completed:** repaired strict-build defects in the no-MySQL branches for epic zone
  state, shopkeeper restore, guildhall loading, outposts, and artifact/guild hydration.
  The affected stubs now return the declared sentinel value, explicitly consume inputs,
  or omit SQL-only parsing helpers from the client-free build.
- **Safety behavior preserved:** these changes do not make an incomplete durable domain
  appear available. The runtime mode preflight still rejects flat-file boot with the
  complete unimplemented-domain inventory before world initialization.
- **Flat build evidence:** the same isolated `PERSISTENCE_BACKEND=flatfile` build now
  compiles through epic state, files, guildhalls, outposts, persistence coordination,
  artifact/guild state, zone touch, session auditing, and additional gameplay modules.
  Its next strict-build boundary is the group of SQL-disabled poll functions in
  `poll.c`; the binary has not linked yet.
- **Files changed:** `src/epic.c`, `src/files.c`, `src/guildhall_db.c`,
  `src/outposts.c`, and `src/artifact_guild_state.c`.
- **Next action:** continue from the `poll.c` no-MySQL diagnostics, finish compiling all
  translation units, then resolve client-free link symbols and add a boot preflight
  test.

### Checkpoint 5 - all client-free translation units compile

- **Completed:** repaired the remaining strict compile failures in polling, SQL and
  player SQL stubs, locker workers, timers, trophies, nexus stones, wiki help, and the
  multiplay whitelist. SQL-disabled mutation, ownership, hydration, and query sentinels
  now fail closed instead of reporting success.
- **Completed:** separated Redis compilation from the MariaDB guard. Redis remains an
  independently configured, optional service and no longer loses its hiredis types and
  implementation merely because `PERSISTENCE_BACKEND=flatfile` disables MySQL.
- **Checks passed:** `python3 tests/async/test_persistence_mode.py`,
  `python3 tests/async/test_no_mysql_compat.py`,
  `python3 tests/async/test_account_bound_reward_contract.py`,
  `./scripts/format.sh --check`, `git diff --check`, and `make -C src -j2`.
- **Flat build evidence:** every server translation unit now compiles with the normal
  strict warning profile and without system MySQL headers. The build reaches its final
  link command without `-lmysqlclient` and reports 41 distinct missing symbols.
- **Link inventory:** the remaining symbols fall into four bounded groups: shared
  environment/string helpers currently compiled inside the SQL-only branch; explicit
  no-MySQL SQL/player/locker sentinels; and CTF, outpost, nexus, auction, and related
  gameplay functions whose entire implementations are still compiled out.
- **Files changed:** `src/poll.c`, `src/redis.c`, `src/sql.c`, `src/sql_player.c`,
  `src/locker_async.c`, `src/timers.c`, `src/trophy.c`, `src/nexus_stones.c`,
  `src/wikihelp.c`, and `src/multiplay_whitelist.c`.
- **Next action:** expose `load_env_file()` and other backend-neutral helpers outside the
  SQL branch, implement fail-closed versions of the missing SQL APIs, then decouple or
  stub the remaining gameplay symbol groups until the client-free binary links.

### Checkpoint 6 - linked client-free binary and boot preflight

- **Completed:** moved secure `.env` loading into the backend-neutral `env_file` module,
  before persistence-mode selection, so the client-free binary can read its explicit
  backend and state-root configuration without any SQL implementation.
- **Completed:** closed the 41-symbol link inventory with explicit fail-closed SQL,
  player, locker, auction, CTF, outpost, and nexus implementations. The flat build now
  compiles and links the complete server without system MySQL headers, MySQL objects, or
  `-lmysqlclient`.
- **Completed:** added `test_flatfile_boot_preflight.py` and a dedicated CI job whose
  dependency list omits MySQL/MariaDB. The test performs a fresh isolated build, rejects
  any system MySQL include or client link flag, and launches the resulting binary with
  an otherwise empty environment.
- **Boot evidence:** the client-free binary exits with status 1 before world boot,
  identifies `flatfile-primary`, prints the unimplemented durable-domain inventory, and
  provisions exactly the expected authority directories at mode `0700`.
- **Checks passed:** `python3 tests/async/test_flatfile_boot_preflight.py`,
  `python3 tests/async/test_persistence_mode.py`,
  `python3 tests/async/test_no_mysql_compat.py`,
  `python3 tests/async/test_account_bound_reward_contract.py`,
  `./scripts/format.sh --check`, `git diff --check`, and `make -C src -j2`.
- **Files changed:** `.github/workflows/quality.yml`, `src/Makefile`, `src/comm.c`,
  `src/env_file.[ch]`, `src/sql.[ch]`, `src/sql_player.c`, `src/auction_houses.c`,
  `src/ctf.c`, `src/outposts.c`, `src/nexus_stones.c`, and
  `tests/async/test_flatfile_boot_preflight.py`.
- **Next action:** begin P1 with the typed/versioned flat account authority and its
  canonical account/name/PID indexes, then route account login and character discovery
  through the selected backend.

### Checkpoint 7 - atomic, versioned account authority foundation

- **Completed:** added a backend-neutral atomic file store that creates a same-directory
  temporary file at mode `0600`, writes and synchronizes its contents, atomically
  publishes it with `renameat`, and synchronizes the containing authority directory.
  Reads reject unsafe names, symlinks, non-regular files, wrong ownership, group/world
  access, and oversized records.
- **Completed:** added a typed account repository with an explicit `DURACCT` magic,
  version 1 schema, bounded little-endian fields, SHA-256 payload checksum, canonical
  case-insensitive account key, and monotonically increasing optimistic revision.
  The record covers the existing account scalars, unique-IP list, and account-character
  membership fields.
- **Completed:** added an adapter between the repository DTO and `acct_entry`; the
  client-free build now routes `read_account`, `write_account`, and `account_exists`
  through the flat authority while the normal build retains its MariaDB routes.
- **Completed:** added a strict standalone repository regression and included it in the
  client-free CI job. It verifies complete round trips, case-insensitive lookup, create
  and update revisions, stale-write rejection, missing lookup, unsafe-name rejection,
  checksum corruption, symlink and insecure-permission rejection, and temporary-file
  cleanup.
- **Checks passed:** `python3 tests/async/test_flatfile_account_repository.py`,
  `python3 tests/async/test_flatfile_boot_preflight.py`, `./scripts/format.sh --check`,
  `git diff --check`, and `make -C src -j2`.
- **Files changed:** `.github/workflows/quality.yml`, `src/Makefile`, `src/account.[ch]`,
  `src/flatfile_store.[ch]`, `src/flatfile_account_repository.[ch]`,
  `src/flatfile_account_adapter.[ch]`, and
  `tests/async/{test_flatfile_account_repository.py,flatfile_account_repository_harness.cpp}`.
- **Remaining P1 gap:** this is the account-record authority only. Canonical character
  name and PID indexes, durable PID allocation, rename/delete/block transactions,
  player snapshot materialization, terminal save fencing, and offline legacy import are
  not implemented, so flat-file-primary boot must continue to fail closed.
- **Next action:** implement the canonical character name/PID identity index and durable
  allocator, then make character membership changes publish consistently with those
  indexes.

### Checkpoint 8 - durable PID allocator and bidirectional identity catalog

- **Completed:** added a typed, versioned `DURIDEN` identity catalog under
  `identities/names`. One atomic publication contains the next PID and all PID/name/
  account mappings, so a committed catalog cannot expose a new high-water mark without
  its matching indexes or vice versa. Active canonical names and all PIDs are unique;
  deletion retains a PID tombstone while releasing the active name.
- **Completed:** implemented allocate, current-highest, claim, case-insensitive name
  lookup, PID lookup, rename-with-expected-name, block/unblock, and tombstone operations.
  The no-MySQL `sql_player_exists`, `sql_get_player_pid`, `getNewPCidNumb`, and startup
  high-water-mark routes now use this authority. Existence checks fail closed on catalog
  errors so corruption cannot be mistaken for an available character name.
- **Completed:** added owner-only advisory lock files to the atomic store. Catalog
  mutations and optimistic account updates are now serialized across processes as well
  as threads; files are still published with the same write/sync/rename/directory-sync
  sequence.
- **Completed:** added a strict identity repository regression and client-free CI step.
  It covers allocation, both lookup directions, duplicate rejection, rename collision,
  block state, tombstone semantics, unsafe names, checksum corruption, and forty
  allocations across four forked writers. The account regression now proves that two
  forked writers from revision 2 produce exactly one revision-3 winner and one conflict.
- **Checks passed:** `python3 tests/async/test_flatfile_identity_repository.py`,
  `python3 tests/async/test_flatfile_account_repository.py`,
  `python3 tests/async/test_flatfile_boot_preflight.py`, `./scripts/format.sh --check`,
  `git diff --check`, and `make -C src -j2`.
- **Files changed:** `.github/workflows/quality.yml`, `src/Makefile`,
  `src/flatfile_store.[ch]`, `src/flatfile_account_repository.c`,
  `src/flatfile_identity_repository.[ch]`, `src/flatfile_identity_adapter.[ch]`,
  `src/nanny.c`, `src/sql_player.c`, `src/persistence_mode.c`, and the focused account,
  identity, and boot tests.
- **Remaining P1 gap:** allocation and lookup authority exist, but character creation
  does not yet claim the identity together with account membership. Rename, block, and
  delete call sites are not routed to catalog transactions, and player snapshots remain
  unimplemented. Flat-file-primary boot therefore continues to fail closed.
- **Next action:** introduce a recoverable account-membership transaction that claims or
  updates the identity catalog and account record together, then route create, rename,
  block, and delete flows through it.

### Checkpoint 9 - single-authority account-character membership

- **Completed:** promoted the identity catalog to version 2 and made it the sole
  account-character membership authority. Each entry now carries every existing
  `acct_chars` field: account, PID, name, login count/time, block and racewar state,
  level, race, primary/secondary class, last room, and last save. Version 1 catalogs
  remain readable and upgrade on their next mutation.
- **Completed:** added atomic list-by-account and full-account synchronization. One
  catalog publication can claim new allocated PIDs, update or swap canonical names and
  metadata, and tombstone omitted memberships while validating final global name/PID
  uniqueness. It never needs a partially committed second index.
- **Completed:** flat account files no longer serialize character membership. Account
  loads materialize their linked character list from the catalog, and successful scalar
  account saves then synchronize the in-memory list to that authority. Existing account
  records containing the former cache remain readable, but the adapter deliberately
  ignores and removes that duplicate authority on the next save.
- **Runtime effect:** normal account character creation reaches this path through
  `write_account`, so the allocated PID, canonical name, account relationship, and list
  metadata become discoverable together. Account-list metadata changes and direct list
  removals use the same sync. The higher-level character rename and deletion commands
  still fail closed because player snapshots and their locker/ship/artifact side effects
  are not authoritative yet; they are not falsely reported as complete here.
- **Completed:** added an adapter-level regression and client-free CI step. It proves an
  account scalar record has no membership cache, reload materializes all catalog fields,
  rename/block publish together, removal releases the active name while preserving the
  PID tombstone, and catalog version 2 metadata round-trips.
- **Checks passed:** `python3 tests/async/test_flatfile_account_membership.py`,
  `python3 tests/async/test_flatfile_identity_repository.py`,
  `python3 tests/async/test_flatfile_account_repository.py`,
  `python3 tests/async/test_flatfile_boot_preflight.py`, `./scripts/format.sh --check`,
  `git diff --check`, and `make -C src -j2`.
- **Files changed:** `.github/workflows/quality.yml`,
  `src/flatfile_account_adapter.c`, `src/flatfile_identity_repository.[ch]`, and
  `tests/async/{test_flatfile_account_membership.py,flatfile_account_membership_harness.cpp,flatfile_identity_repository_harness.cpp}`.
- **Remaining P1 gap:** player load/save still has no flat materialized snapshot, so
  character login cannot continue past membership discovery and terminal saves are not
  fenced on a flat authority. Rename/delete command completion and offline legacy import
  must follow that player authority.
- **Next action:** implement a typed, versioned player snapshot repository keyed by PID
  and revision, reuse the existing capture/codec coverage where safe, and route the flat
  player load/save boundary to it.

### Checkpoint 10 - crash-safe player snapshot materialization

- **Completed:** added a versioned `DURPLYR` player repository under `players/`, reusing
  the existing host-layout-independent snapshot codec rather than defining a competing
  character model. Each PID file has redundant PID/revision/component metadata, a
  SHA-256 payload checksum, strict size and ownership validation, a per-PID process lock,
  and the established write/sync/rename/directory-sync publication sequence.
- **Completed:** full snapshots establish a baseline; later component snapshots merge
  into the last complete materialization without discarding untouched fields. Apply is
  revision-idempotent, reports already-applied and stale revisions, rejects partial
  creation, and turns an ambiguous post-rename retry into an exact revision read. The
  stored materialization always declares the complete component mask.
- **Completed:** equipment and inventory are normalized into one flat-backend checkpoint
  unit. A one-sided item replacement is rejected because its parent indexes cannot be
  safely spliced into the other half; the save pipeline expands either dirty bit to both
  before capture.
- **Completed:** the client-free save worker and journal replay now select the flat
  materializer while the normal build continues to select the MariaDB repository. Thus
  a complete flat baseline can accept, replay, merge, and acknowledge the existing
  asynchronous snapshot stream without a database client.
- **Completed:** added a strict repository regression and client-free CI step covering a
  representative complete snapshot (status, replacement rows, skills, affects, nested
  items, spellbook metadata, pet items, shapes, and trophies), partial merge retention,
  missing-baseline rejection, duplicate/stale revisions, torn item rejection, two
  forked writers, checksum corruption, and temporary-file cleanup.
- **Checks passed:** `python3 tests/async/test_flatfile_player_repository.py`,
  `python3 tests/async/test_player_save_pipeline.py`,
  `python3 tests/async/test_player_save_journal.py`,
  `python3 tests/async/test_flatfile_boot_preflight.py`, `./scripts/format.sh --check`,
  `git diff --check`, and `make -C src -j2`.
- **Files changed:** `.github/workflows/quality.yml`, `src/Makefile`,
  `src/flatfile_player_repository.[ch]`, `src/player_save_pipeline.c`,
  `src/persistence_mode.c`, and the focused player repository, pipeline, and boot tests.
- **Remaining P1 gap:** the synchronous first-save path does not yet create the complete
  flat baseline, and the load pipeline does not yet source this materialization or build
  its ownership/domain sidecars. Terminal saves therefore cannot claim flat authority
  yet, and boot remains fail closed.
- **Next action:** route synchronous full capture/apply for new characters, then adapt the
  load pipeline to produce a bounded `player_load_result` from the materialized snapshot
  with explicit handling for item identity and the still-external gameplay domains.

### Checkpoint 11 - first baseline and terminal materialization fence

- **Completed:** new client-free characters now initialize revision state at durable
  revision 0. Their first `writeCharacter` cannot fall into the legacy SQL component
  calls: it requests an all-component immutable capture through the existing terminal
  fence and waits up to five seconds for the flat repository to acknowledge the actual
  materialized revision.
- **Completed:** the flat synchronous branch captures before any legacy unequip, affect
  removal, or item extraction. It clears the no-baseline and dirty-container flags only
  after exact materialization acknowledgement. On failure it returns with the live
  character untouched; on a successful terminal save it performs the existing bounded
  unequip/extract/reapply sequence after durability.
- **Completed:** other terminal call sites disable journal-only handoff in the
  client-free build. A durable queue record is therefore not permission to discard live
  inventory; MariaDB mode retains its existing journal-handoff policy.
- **Checks passed:** `python3 tests/async/test_player_save_pipeline.py`,
  `python3 tests/async/test_player_snapshot_capture.py`,
  `python3 tests/async/test_character_persistence_gap.py`,
  `python3 tests/async/test_flatfile_boot_preflight.py`, `./scripts/format.sh --check`,
  `git diff --check`, and `make -C src -j2`.
- **Files changed:** `src/nanny.c`, `src/files.c`, `src/actoth.c`,
  `src/persistence_mode.c`, `tests/async/test_player_save_pipeline.py`, and the boot
  preflight.
- **Remaining P1 gap:** the load worker still queries MariaDB and expects SQL ownership,
  wallet, epic, frag, bank, and recent-gameplay sidecars. Until a flat load result can
  distinguish snapshot-owned state from those external domains, character login and
  safe rename/delete completion remain blocked.
- **Next action:** add a flat player-load adapter that verifies PID/account/name against
  the identity catalog, returns the complete materialized snapshot and revision, and
  fails closed—with explicit missing-domain state—where P2-owned sidecars are required.

### Checkpoint 12 - backend-selected, identity-verified player snapshot load

- **Completed:** the client-free load worker now selects the flat repository directly;
  its default path no longer initializes a MySQL worker thread, acquires a SQL pool
  connection, or calls the SQL load repository. MariaDB builds retain the existing
  consistent-snapshot transaction path, and injected test callbacks remain supported.
- **Completed:** flat loads resolve PID/account or canonical character name through the
  identity catalog, reject inactive or blocked identities and account/PID mismatches,
  authenticate the checksummed full snapshot, and require its embedded character name
  to match the catalog. Missing, corrupt, expired, inaccessible, and transient-read
  outcomes remain distinguishable to the game thread.
- **Safety boundary:** a verified snapshot is deliberately not reported as loadable yet.
  The adapter returns `external_domains` with `ENOTSUP` because item ownership plus
  wallet, bank, epic, frag, and recent-gameplay state still lack flat authorities.
  Character materialization therefore cannot fabricate zero balances, accept ambiguous
  item custody, or silently omit gameplay history.
- **Completed:** expanded the standalone player repository regression to cover catalog
  linkage, canonical-name resolution, account mismatch, request expiry, and the explicit
  external-domain fence. The pipeline source contract now verifies backend selection.
- **Checks passed:** `python3 tests/async/test_flatfile_player_repository.py`,
  `python3 tests/async/test_player_load_pipeline.py`,
  `python3 tests/async/test_flatfile_boot_preflight.py`,
  `python3 tests/async/test_persistence_mode.py`, `./scripts/format.sh --check`,
  `git diff --check`, and `make -C src -j2`.
- **Files changed:** `src/flatfile_player_repository.[ch]`,
  `src/player_load_pipeline.c`, `src/persistence_mode.c`, and the focused player
  repository, pipeline, and boot tests.
- **Remaining P1 gap:** snapshot-owned character state now has save, terminal-fence, and
  verified load paths, but login remains intentionally blocked until the external
  sidecars can be loaded consistently. Rename/delete completion and offline legacy
  import are also unfinished.
- **Next action:** define and implement the flat player-domain sidecar transaction for
  wallet/bank/epic/frag/gameplay reads and the item UID/ownership authority, then combine
  those revisions with the snapshot into one materializable load result.

### Checkpoint 13 - durable collision-free item UID allocation

- **Completed:** added a typed `DURUID` allocator authority under `metadata/`. Each
  reservation atomically advances a 64-bit high-water mark with a format version,
  monotonic authority revision, SHA-256 checksum, owner-only cross-process lock, and the
  shared write/sync/rename/directory-sync publication sequence.
- **Completed:** allocator ranges are reserved durably before any UID is returned. A
  crash may leave unused IDs in a reserved range but cannot cause reuse; overflow,
  corruption, unsafe files, failed locks, or ambiguous I/O fail closed. The existing
  in-memory fast allocator consumes only its already-published range.
- **Completed:** backend selection now reserves the boot UID range from this authority
  in flat mode while MariaDB mode retains its transactional allocator row. The boot
  blocker no longer lists UID allocation itself, but item ownership and its transfer
  ledger remain explicitly unimplemented.
- **Completed:** added a standalone regression and client-free CI step. It verifies the
  initial reservation, high-water/revision reads, zero-count rejection, four concurrent
  process reservations with no overlap, checksum corruption rejection, refusal to
  overwrite corrupt authority, and temporary-file cleanup.
- **Checks passed:** `python3 tests/async/test_flatfile_item_uid_allocator.py`,
  `python3 tests/async/test_item_ownership_contract.py`,
  `python3 tests/async/test_flatfile_boot_preflight.py`, `./scripts/format.sh --check`,
  `git diff --check`, and `make -C src -j2`.
- **Files changed:** `src/flatfile_item_uid_allocator.[ch]`,
  `src/item_uid_allocator.c`, `src/comm.c`, `src/Makefile`,
  `src/persistence_mode.c`, `.github/workflows/quality.yml`, and the focused allocator
  regression.
- **Remaining P2 gap:** allocating an ID does not establish custody. No durable flat
  owner index or idempotent transfer ledger exists yet, so item creation, movement,
  load identities, and cross-owner revision checks remain fenced.
- **Next action:** implement a versioned ownership catalog plus operation-ID ledger that
  applies the existing item-transfer command atomically and idempotently, then use it to
  construct authoritative item sidecars during player load.

### Checkpoint 14 - revisioned item custody and idempotent transfer ledger

- **Completed:** added a typed `DUROWN` authority under `domains/` containing the sorted
  owner-revision index, current UID/root/parent/owner/revision/custody records, and the
  operation-ID result ledger. The entire mutation and its replay result publish in one
  checksummed catalog revision under an owner-only cross-process lock, so no committed
  ownership move can exist without its idempotence record or vice versa.
- **Completed:** the repository applies the existing version-2 item-transfer command
  rather than a new flat-only mutation format. It enforces expected owner and item
  revisions, complete selected-subtree topology, target-parent ownership/revision,
  creation uniqueness, destruction custody state, revision overflow, and bounded
  catalog capacity. A command replay returns its original result; reusing an operation
  ID with different command bytes is rejected.
- **Completed:** the existing critical-command journal and coordinator now select this
  repository for client-free item transfers. The journal remains the pre-apply WAL;
  after a crash, replay reaches the catalog ledger and either applies once or returns
  `already_applied`. Non-item critical commands remain explicitly unsupported rather
  than reaching the SQL pool.
- **Completed:** added owner-load support for reconstructing an active custody sidecar.
  Player snapshot/ownership reconciliation is still a separate next step, so login is
  not enabled merely because an owner catalog exists.
- **Completed:** added a standalone repository regression and client-free CI step. It
  covers nested creation, owner/item revision results, owner reload, exact replay,
  conflicting operation-ID reuse, cross-owner movement, stale rejection and its replay,
  two-process replay of one creation, checksum corruption, refusal to overwrite corrupt
  authority, and temporary-file cleanup.
- **Checks passed:** `python3 tests/async/test_flatfile_item_repository.py`,
  `python3 tests/async/test_item_ownership_contract.py`,
  `python3 tests/async/test_flatfile_boot_preflight.py`, `./scripts/format.sh --check`,
  `git diff --check`, and `make -C src -j2`.
- **Files changed:** `src/flatfile_item_repository.[ch]`, `src/comm.c`,
  `src/Makefile`, `src/persistence_mode.c`, `.github/workflows/quality.yml`, and the
  focused ownership repository regression.
- **Operational limitation:** the embedded operation ledger is deliberately bounded and
  never forgets an operation ID. Production tooling must compact it only into another
  durable generation that preserves replay answers; reaching the bound fails closed.
- **Remaining P2 gap:** player/pet snapshot rows are not yet reconciled against custody
  records, other critical command types still use no flat repository, and external
  economy/gameplay sidecars remain absent.
- **Next action:** construct player-load item identity sidecars by matching every
  snapshot UID and parent edge to the player owner revision, then add the economy and
  gameplay-read authorities needed for a fully materializable result.

### Checkpoint 15 - snapshot-to-custody load reconciliation

- **Completed:** flat player loads now read the player owner revision and active custody
  set, then match every inventory, equipment, and pet-item snapshot row by stable UID.
  They require exact aggregate cardinality, unique UIDs, matching vnums, matching parent
  edges and roots, active player custody, and one shared owner revision. Extra catalog
  items and extra snapshot items both reject the load.
- **Completed:** successful reconciliation constructs the existing bounded
  `player_load_item_identity` and pet-identity sidecars with synthetic per-load row IDs,
  stable UID/root/parent metadata, item and owner revisions, quantity, custody state,
  and explicit metadata override coverage. The result is trimmed to the requested
  session component set without weakening reconciliation of the complete stored
  authority.
- **Completed:** the client-free critical pipeline no longer starts the SQL outbox
  worker. Item commands use the flat journal/coordinator/catalog result path directly;
  MariaDB mode retains its transactional outbox worker. This removes a latent loop that
  would otherwise poll a permanently unavailable SQL pool in flat mode.
- **Safety boundary:** the load still returns `external_domains / ENOTSUP` after item
  reconciliation. Wallet, bank, epic, frag, gameplay reads, trophy materialization, and
  initial custody publication for never-moved new-character items still prevent a safe
  login claim at this checkpoint.
- **Completed:** expanded the player repository regression with stable UIDs, nested
  player and pet custody creation, owner revision checks, parent identity checks, and
  aggregate item-sidecar verification.
- **Checks passed:** `python3 tests/async/test_flatfile_player_repository.py`,
  `python3 tests/async/test_flatfile_item_repository.py`,
  `python3 tests/async/test_player_load_pipeline.py`, `./scripts/format.sh --check`,
  `git diff --check`, and `make -C src -j2`.
- **Files changed:** `src/flatfile_player_repository.c`, `src/comm.c`,
  `src/persistence_mode.c`, and the focused player repository regression.
- **Next action:** publish initial item custody as part of the first durable player
  baseline, then implement revisioned player-economy/account-bank/gameplay-read
  authorities and combine them into the load result.

### Checkpoint 16 - first-baseline item custody fence

- **Completed:** the first full flat player materialization now derives the complete
  sorted UID/root/parent/vnum custody set from inventory, equipment, and pet snapshots
  and establishes player owner revision 1 before publishing the player snapshot. Empty
  inventories establish an explicit empty owner authority rather than leaving absence
  ambiguous.
- **Completed:** baseline custody is exact-state idempotent and guarded by the same
  owner-only catalog lock. A retry after custody publication but before snapshot
  publication observes the identical owner revision/set and continues; a different
  pre-existing owner revision or item set refuses the snapshot. Thus the supported
  partial outcome is recoverable custody-without-snapshot, never a loadable snapshot
  without custody.
- **Completed:** baseline validation rejects missing or duplicate UIDs, invalid vnums,
  forward/cyclic parent references, cross-pet/inventory UID collisions, oversized
  aggregate item sets, and inconsistent roots. Arbitrarily large valid player item sets
  are established in one catalog mutation instead of being split across the interactive
  transfer command's intentionally small subtree limit.
- **Completed:** expanded the ownership regression with direct baseline apply, exact
  replay, and conflict coverage. The player regression now proves its initial full
  snapshot creates the owner authority automatically before the load sidecar is built.
- **Checks passed:** `python3 tests/async/test_flatfile_item_repository.py`,
  `python3 tests/async/test_flatfile_player_repository.py`,
  `./scripts/format.sh --check`, `git diff --check`, and `make -C src -j2`.
- **Files changed:** `src/flatfile_item_repository.[ch]`,
  `src/flatfile_player_repository.c`, `src/persistence_mode.c`, and the focused item and
  player repository regressions.
- **Remaining login gap:** item identity/custody is now complete for the first baseline
  and later transfer operations, but economy/gameplay sidecars and trophy load coverage
  still keep the result fail closed.
- **Next action:** implement the typed player-economy/account-bank/gameplay-read
  authorities and their initial-baseline publication, then make their critical commands
  use the same revision/idempotence boundary.

### Checkpoint 17 - player economy and shared account-bank authority foundation

- **Completed:** added separate typed `DURPDOM` player-domain and `DURBANK`
  account/racewar-bank records under `domains/`. Player records cover wallet balances,
  epic and frag counters, their independent revisions, recent PvP death timestamps, and
  completed epic-zone IDs; bank balances and revision live once per canonical account
  and racewar rather than being duplicated across character records.
- **Completed:** both record types use bounded little-endian schemas, explicit format
  versions, SHA-256 payload checksums, owner-only file validation, and the shared atomic
  write/sync/rename/directory-sync store. A single owner-only domain lock serializes the
  two-file initial baseline so retries verify both records, including the shared bank,
  before reporting success. Corrupt or conflicting authority is never overwritten.
- **Completed:** added a standalone repository regression and client-free CI step. It
  covers full round trips, canonical account lookup, exact retry, conflicting player and
  same-player bank retries, two characters sharing one account bank, conflicting shared
  bank creation, account mismatch, checksum corruption, refusal to overwrite corrupt
  authority, and temporary-file cleanup.
- **Checks passed:** `python3 tests/async/test_flatfile_player_domain_repository.py`,
  `python3 tests/async/test_flatfile_boot_preflight.py`, the normal `make -C src -j2`
  build, `./scripts/format.sh --check`, and `git diff --check`.
- **Files changed:** `src/flatfile_player_domain_repository.[ch]`, `src/Makefile`,
  `.github/workflows/quality.yml`, and the focused domain repository regression.
- **Remaining login gap:** this checkpoint establishes the typed authority and its
  recoverable initial transaction only. Player snapshot baseline/load are not yet wired
  to it, mutation commands do not yet apply its revision boundary, and trophy load
  coverage remains fenced.
- **Next action:** derive and establish these records during the first full player
  snapshot, hydrate the bounded load result from them, then route wallet/bank/epic/frag
  critical commands through a durable idempotent operation ledger.

### Checkpoint 18 - player-domain baseline and load integration

- **Completed:** immutable status capture now includes the four carried currency values
  and epic balance, completing all 63 status fields expected by the load materializer.
  The first full flat snapshot extracts those values plus racewar/frags, verifies its
  PID/name/account/racewar against the identity catalog, and establishes the player
  domain record before publishing the snapshot.
- **Completed:** first-save recovery now has three ordered, idempotent authorities:
  item custody, player/account domain baseline, then the snapshot. A crash can leave
  custody or domains ahead of the snapshot, but an exact retry completes publication;
  conflicting, corrupt, or identity-mismatched state fails closed. New characters use
  an existing shared account/racewar bank as-is, while a missing bank is initialized to
  zero, so character creation cannot overwrite an account balance it did not capture.
- **Completed:** flat loads now verify snapshot racewar against identity, load the
  checksummed player and shared-bank records, and populate wallet/bank/epic/frag
  balances and revisions plus both bounded gameplay-read collections. Missing,
  corrupt, conflicting, and transient domain outcomes remain distinct failures.
- **Safety boundary:** the result still returns `ENOTSUP`, now specifically at
  `trophies`, because the load materializer does not apply the snapshot trophy rows.
  Domain mutations also remain unsupported in the flat critical-command coordinator;
  the boot blocker now names these narrower gaps instead of claiming the sidecars are
  absent.
- **Checks passed:** `python3 tests/async/test_flatfile_player_domain_repository.py`,
  `python3 tests/async/test_flatfile_player_repository.py`,
  `python3 tests/async/test_player_snapshot_capture.py`,
  `python3 tests/async/test_epic_transaction_contract.py`,
  `python3 tests/async/test_player_load_pipeline.py`,
  `python3 tests/async/test_flatfile_boot_preflight.py`,
  `./scripts/format.sh --check`, `git diff --check`, and `make -C src -j2`.
- **Files changed:** `src/flatfile_player_domain_repository.[ch]`,
  `src/flatfile_player_repository.c`, `src/player_snapshot_capture.c`,
  `src/persistence_mode.c`, and the focused domain, player, and boot regressions.
- **Next action:** materialize trophy rows so the now-complete flat read DTO can pass
  the login boundary, then implement wallet/bank/epic/frag command application and its
  idempotent ledger before allowing flat-file-primary boot.

### Checkpoint 19 - complete materializable flat player read

- **Completed:** the load materializer now validates trophy zone/experience rows,
  rejects duplicate zones, constructs the runtime zone-trophy vector with bounded
  allocation failure handling, and releases that vector with the rest of player-owned
  state. Empty trophy authority is represented by an initialized empty vector rather
  than an ambiguous null pointer.
- **Completed:** after identity, snapshot, custody, player-domain, shared-bank, gameplay
  read, and trophy validation all succeed, the flat repository now returns the existing
  `applied` load outcome with no failed component. PID-based and canonical-name requests
  therefore produce the same complete DTO that the game-thread materializer expects.
- **Safety boundary:** this enables the read/materialization contract, not backend boot.
  Wallet, bank, epic, and frag mutations still have no flat critical-command apply path,
  so preflight now names `player domain mutations` as the remaining player-specific
  blocker.
- **Compatibility repair:** removed a redundant direct system-MySQL include from the
  epic task catalog. Its SQL interface already supplies the selected client or local
  compatibility types; the duplicate include collided with client-free declarations in
  the focused gameplay-read build.
- **Checks passed:** `python3 tests/async/test_flatfile_player_repository.py`,
  `python3 tests/async/test_player_load_pipeline.py`,
  `python3 tests/async/test_player_load_pets.py`,
  `python3 tests/async/test_set_based_gameplay_reads.py`,
  `python3 tests/async/test_flatfile_boot_preflight.py`, `make -C src -j2`,
  `./scripts/format.sh --check`, and `git diff --check`.
- **Files changed:** `src/player_load_materialize.c`, `src/flatfile_player_repository.c`,
  `src/db.c`, `src/persistence_mode.c`, `src/epic_task_catalog.c`, and the focused
  player, pipeline, and boot regressions.
- **Next action:** extend the player-domain authority with an embedded operation ledger
  and apply the existing wallet/bank/epic/frag commands under their expected revisions,
  then select it from the client-free critical-command coordinator.

### Checkpoint 20 - idempotent epic mutation authority

- **Completed:** upgraded player-domain records to format version 2 with a bounded
  embedded operation ledger. Each entry stores the operation ID, full encoded-command
  digest, result code, and typed result payload in the same atomic player publication as
  the epic balance/revision. Version 1 player and bank records remain readable; the next
  publication upgrades them.
- **Completed:** the flat critical-command dispatcher now accepts the existing version-1
  epic command. It enforces expected revision, required funds, signed overflow, and
  revision overflow; successful and rejected decisions are both recorded. Exact replay
  returns the original typed result, while reuse of an operation ID for different bytes
  fails with `EEXIST`.
- **Completed:** load reads the mutated epic balance/revision directly from the upgraded
  player authority. The normal MariaDB repository and command format are unchanged.
  The player-specific boot blocker is narrowed to wallet/bank/frag mutations; compound
  commands that touch those domains remain unsupported.
- **Completed:** expanded the domain regression with successful mutation and load,
  exact replay, conflicting operation-ID reuse, stale-revision replay, and insufficient
  funds. Standalone item/player tests now compile the same selected dispatcher with the
  local no-MySQL compatibility surface.
- **Checks passed:** `python3 tests/async/test_flatfile_player_domain_repository.py`,
  `python3 tests/async/test_flatfile_item_repository.py`,
  `python3 tests/async/test_flatfile_player_repository.py`,
  `python3 tests/async/test_item_ownership_contract.py`,
  `python3 tests/async/test_epic_transaction_contract.py`,
  `python3 tests/async/test_player_load_pipeline.py`, `make -C src -j2`,
  `python3 tests/async/test_flatfile_boot_preflight.py`,
  `./scripts/format.sh --check`, and `git diff --check`.
- **Files changed:** `src/flatfile_player_domain_repository.[ch]`,
  `src/flatfile_item_repository.c`, `src/persistence_mode.c`, and the focused domain,
  item, player, and boot regressions.
- **Operational limitation:** the embedded ledger is deliberately bounded at 512
  operations per player and never forgets replay decisions. Exhaustion fails closed;
  future compaction must preserve operation answers in another durable generation.
- **Next action:** add a recoverable two-record transaction for wallet plus shared bank,
  then add frag mutation/ledger application and route the associated compound commands.

### Checkpoint 21 - durable transaction-intent cleanup primitive

- **Completed:** added an owner-validated atomic authority removal primitive for the
  forthcoming wallet/shared-bank transaction protocol. It opens the containing
  directory without following symlinks, requires owner-only regular-file metadata,
  unlinks by directory descriptor, and synchronizes the directory before reporting the
  intent cleared. Missing-file acknowledgement is explicit for crash-recovery retries.
- **Completed:** separated deterministic player-domain and shared-bank record encoding
  from their atomic publication. The transaction intent can therefore carry the exact
  checksummed after-images that recovery will republish, without rebuilding mutable
  state after a crash.
- **Safety behavior:** unsafe names, symlinks, wrong ownership/access modes, directory
  errors, unlink failures, and directory-sync failures refuse completion. A retry after
  unlink but before its acknowledgement can safely treat the already-missing intent as
  complete because transaction materializations must be synchronized first.
- **Checks passed:** the account repository regression now covers durable removal,
  idempotent missing removal, and symlink refusal; `./scripts/format.sh --check` and
  `git diff --check` are rerun before publication.
- **Files changed:** `src/flatfile_store.[ch]`,
  `src/flatfile_player_domain_repository.c`, the focused account/domain regressions,
  and this handoff ledger.
- **Next action:** encode the wallet/bank after-images in a checksummed transaction
  intent, publish both under the global domain lock, then durably clear the intent;
  every domain entry point will finish a surviving intent before reading or mutating.

### Checkpoint 22 - recoverable wallet/shared-bank transaction

- **Completed:** the flat critical dispatcher now applies the existing account-bank
  currency command. It validates canonical account/racewar ownership, both expected
  revisions, insufficient funds, the runtime `INT_MAX` balance bound, and revision
  overflow. Successful commands increment wallet and bank revisions together; rejected
  commands retain the original balances and durably record their typed result.
- **Completed:** successful two-record changes first publish a checksummed
  `.currency-transaction` containing the exact checksummed player and bank after-images.
  Under the global domain lock the repository publishes bank, then player, then durably
  removes the intent. Every baseline, load, epic, and currency entry point finishes a
  surviving intent before reading or mutating, so a crash cannot expose a permanently
  split wallet/bank decision.
- **Completed:** currency decisions share the player-domain operation ledger. Exact
  replay returns the original wallet, bank, and revisions; conflicting operation-ID
  reuse fails with `EEXIST`; stale and insufficient-funds decisions replay without
  reevaluation. Compound command formats remain untouched.
- **Completed:** the focused regression covers mutation/load, exact replay, operation-ID
  conflict, stale rejection replay, insufficient bank funds, and an injected interruption
  after bank publication. The subsequent load republishes both after-images, removes the
  intent, and the command then replays as already applied. The fault point is compiled
  only into that standalone regression.
- **Contract update:** full immutable status capture carries opening currency values for
  the flat first-baseline handoff. SQL checkpoint replay still filters those fields and
  load materialization still takes balances exclusively from the transaction domain, so
  later snapshots cannot overwrite currency authority.
- **Checks passed:** `python3 tests/async/test_flatfile_player_domain_repository.py`,
  `python3 tests/async/test_flatfile_item_repository.py`,
  `python3 tests/async/test_flatfile_player_repository.py`,
  `python3 tests/async/test_currency_transaction_contract.py`, `make -C src -j2`,
  `python3 tests/async/test_flatfile_boot_preflight.py`,
  `./scripts/format.sh --check`, and `git diff --check`.
- **Files changed:** `src/flatfile_player_domain_repository.c`,
  `src/flatfile_item_repository.c`, `src/persistence_mode.c`, the focused domain/item/
  player/currency/boot regressions, and this handoff ledger.
- **Remaining player mutation gap:** standalone epic and wallet/bank commands are now
  authoritative. Frag changes are carried by compound combat commands, which still need
  a multi-player/account atomic protocol and remain fenced with other non-item critical
  operations.
- **Next action:** implement the combat outcome command across participant frag, epic,
  wallet, and shared-bank records using a generalized multi-after-image intent, then
  route auction and boon compound mutations through the same recovery boundary.

### Checkpoint 23 - recoverable multi-participant combat outcomes

- **Completed:** the player-domain critical dispatcher now applies the existing combat
  outcome command without changing its wire format. It loads every participant and
  distinct shared bank under the global domain lock, validates frag, epic, wallet, and
  bank revisions against the captured gameplay snapshot, and applies signed frag/epic
  deltas plus canonical wallet rewards with overflow and runtime balance bounds.
- **Completed:** the wallet journal is generalized to a bounded checksummed
  `.player-domain-transaction`. It carries up to 15 player and 15 distinct bank
  after-images, rejects duplicate or mismatched targets, publishes all banks before all
  players, and durably removes the intent only after every after-image is synchronized.
  Recovery still recognizes the prior one-player `.currency-transaction` format, so an
  upgrade can finish an intent left by checkpoint 22; the interrupted-currency test now
  constructs and recovers that legacy format explicitly.
- **Completed:** combat decisions are recorded in every participant operation ledger.
  Exact replay requires the same digest and a complete ledger entry set, returns the
  original typed combat result and deterministic event identifier, and rejects
  operation-ID reuse with `EEXIST`. Stale and range decisions publish a no-mutation
  multi-player ledger transaction, preventing reevaluation on retry.
- **Completed:** shared-account rewards preserve the SQL transaction semantics: every
  rewarded participant validates the same opening bank revision, each reward advances
  that shared revision, and all result entries report the final bank revision. Frag
  changes preserve `old_frags` and advance the frag revision only when the delta is
  nonzero.
- **Completed:** the focused regression injects interruption after bank publication for
  a two-participant, shared-account outcome. A subsequent participant load republishes
  both player after-images and the bank, clears the intent, and exact replay proves the
  operation was not duplicated. It also covers conflicting IDs and durable stale
  rejection. Existing currency, item, player, and combat source-contract tests remain
  green.
- **Boot diagnostic:** transactional player frag mutation is no longer listed as a
  missing domain. Flat-file modes remain deliberately fenced by character lifecycle,
  other non-item critical commands, and the wider world/economy authorities.
- **Checks passed:** `python3 tests/async/test_flatfile_player_domain_repository.py`,
  `python3 tests/async/test_flatfile_item_repository.py`,
  `python3 tests/async/test_flatfile_player_repository.py`,
  `python3 tests/async/test_currency_transaction_contract.py`,
  `python3 tests/async/test_combat_outcome_transactional_cutover.py`,
  `python3 tests/async/test_combat_artifact_persistence.py`,
  `python3 tests/async/test_flatfile_boot_preflight.py`, `make -C src -j2`,
  `./scripts/format.sh --check`, and `git diff --check`.
- **Files changed:** `src/flatfile_player_domain_repository.c`,
  `src/flatfile_item_repository.c`, `src/persistence_mode.c`, the focused domain/item/
  player/boot regressions, and this handoff ledger.
- **Next action:** extend the recovery boundary to auction commands, which combine item
  custody with wallet/shared-bank state, then use the same multi-authority protocol for
  boon rewards and the remaining compound critical commands.

### Checkpoint 24 - cross-authority recovery foundation

- **Completed:** added a shared flat authority lock spanning player domains and item
  custody. Every player-domain baseline/load/command entry point and every item-custody
  baseline/load/command entry point now takes that process-and-file lock before reading
  authority. This gives compound commands one canonical serialization boundary without
  weakening either repository's existing internal synchronization.
- **Completed:** added a generic checksummed `.critical-authority-transaction` format.
  A transaction carries up to 16 uniquely named after-images under `domains/`, rejects
  unsafe or duplicate targets, caps aggregate input at 256 MiB, writes and synchronizes
  the intent before any target, and durably removes it only after every target is
  synchronized. The API requires the matching root's authority-lock token, preventing
  publication or recovery outside the serialization boundary.
- **Completed:** player and item reads/writes finish a surviving cross-authority intent
  before decoding their own records. A crash during an auction publication therefore
  cannot leave a durable split visible merely because the next operation enters through
  the wallet side rather than the custody side, or vice versa.
- **Completed:** the focused regression injects failure after the first of two unrelated
  authority images, verifies the split plus surviving intent, reacquires the lock as a
  restarted process would, and confirms recovery republishes both images and clears the
  intent. It also verifies unsafe-path refusal and fail-closed checksum corruption that
  neither advances the remaining image nor discards the damaged intent.
- **Checks passed:** `python3 tests/async/test_flatfile_authority_transaction.py`,
  `python3 tests/async/test_flatfile_player_domain_repository.py`,
  `python3 tests/async/test_flatfile_item_repository.py`,
  `python3 tests/async/test_flatfile_player_repository.py`, `make -C src -j2`,
  `python3 tests/async/test_flatfile_boot_preflight.py`,
  `./scripts/format.sh --check`, and `git diff --check`.
- **Files changed:** new `src/flatfile_authority_transaction.[ch]` and focused regression,
  `src/flatfile_player_domain_repository.c`, `src/flatfile_item_repository.c`,
  `src/Makefile`, the standalone repository build lists, and this handoff ledger.
- **Remaining auction gap:** the crash boundary is ready, but no flat auction catalog or
  auction-command state machine exists yet. Auction commands remain fenced.
- **Next action:** add the checksummed auction listing/pickup/operation catalog, prepare
  its catalog, custody, player, and bank after-images under the shared lock, and publish
  all four through the new cross-authority transaction.

### Checkpoint 25 - recoverable flat auction command authority

- **Completed:** added a bounded, checksummed `auction_catalog` containing listings,
  staged refunds and sale proceeds, item and money claim state, and an embedded operation
  ledger. Decode rejects checksum failures, oversized input, duplicate listing/money/
  operation identifiers, and duplicate item identifiers rather than selecting an
  ambiguous record.
- **Completed:** the flat critical dispatcher now applies all seven existing auction
  actions: list, bid, finalize, claim money, claim item, remove, and unknown-action
  rejection. The state machine validates seller/bidder identity, expected listing,
  wallet/bank/item/owner revisions, funds, price and fee ranges, expiry, and claim
  eligibility. Listing identifiers are deterministic from the operation identifier, and
  every accepted or rejected decision is durable and exactly replayable; conflicting
  operation-ID reuse fails with `EEXIST`.
- **Completed:** player-domain and item repositories expose preparation APIs that return
  exact checksummed after-images while the matching shared authority lock is held.
  Successful auction decisions publish the auction catalog together with any item
  custody, player wallet, and shared-bank after-images through one generic critical
  authority transaction. Decision rejection restores the original catalog before
  recording only the rejection, so a failed bid cannot leak a staged refund, balance,
  claim, or custody change.
- **Completed:** the focused lifecycle regression injects interruption after the first
  cross-authority publication during listing, then enters through a player load and
  proves recovery republishes catalog, custody, bank, and player after-images before
  exact replay. It also covers operation-ID conflict, durable stale and insufficient-
  funds rejections, bidding and refund staging, buy-now settlement, seller proceeds,
  buyer claim, expired finalization and seller reclaim, administrative removal, catalog
  corruption, and temporary-file cleanup.
- **Contract correction:** the auction transactional-cutover source test now scopes its
  legacy-mutation scan to the live MySQL branch instead of accidentally selecting the
  deliberately disabled no-MySQL prelude. The runtime auction command contract itself
  is unchanged.
- **Checks passed:** `python3 tests/async/test_flatfile_auction_repository.py`,
  `python3 tests/async/test_flatfile_authority_transaction.py`,
  `python3 tests/async/test_flatfile_player_domain_repository.py`,
  `python3 tests/async/test_flatfile_item_repository.py`,
  `python3 tests/async/test_flatfile_player_repository.py`,
  `python3 tests/async/test_auction_transactional_cutover.py`,
  `python3 tests/async/test_currency_transaction_contract.py`,
  `python3 tests/async/test_auction_persistence.py`,
  `python3 tests/async/test_auction_finalize_claim.py`,
  `python3 tests/async/test_auction_ship_txn_fixes.py`,
  `python3 tests/async/test_auction_transaction_engines.py`, `make -C src -j2`,
  `python3 tests/async/test_flatfile_boot_preflight.py`,
  `./scripts/format.sh --check`, and `git diff --check`.
- **Files changed:** new `src/flatfile_auction_repository.[ch]` and focused harness/test;
  the shared authority transaction, player-domain and item repository preparation APIs;
  `src/Makefile`; affected standalone repository build lists; the corrected auction
  source-contract test; and this handoff ledger.
- **Remaining auction gap:** command mutation is now authoritative and recoverable, but
  the interactive no-MySQL auction list/info/pickup surfaces still use disabled stubs or
  SQL reads. The wider auction/economy boot diagnostic must remain until those reads are
  routed to the flat catalog and exercised through the live command UI.
- **Next action:** add read/query projections over the flat auction catalog and route the
  no-MySQL auction browsing, inspection, and pickup-discovery surfaces through them;
  then use the same cross-authority preparation protocol for boon rewards.

### Checkpoint 26 - flat auction queries and claim discovery

- **Completed:** added bounded read projections for open listings, one open listing by
  identifier, and a player's staged money plus first pending item claim. Queries take
  the shared authority lock, finish any surviving cross-authority transaction before
  decoding, treat a missing catalog as empty, sort open listings by expiry and identifier,
  and fail closed on corrupt checksums or invalid records.
- **Completed:** the client-free auction command surface is no longer an unconditional
  disabled stub. `auction list` reads the flat catalog and supports all, exact-player,
  and stored-keyword filtering; `auction info` renders the catalog's durable seller,
  price, time, winner, and item-information projection without deserializing gameplay
  objects or issuing SQL.
- **Completed:** client-free `auction pickup` discovers staged money first, then the
  first pending item claim, and submits the existing typed claim commands through the
  critical coordinator. Item payloads carry the catalog's exact UID/revision/vnum and
  object blob; invalid or unterminated blobs refuse submission. Publication callbacks
  update runtime balances/custody and materialize claimed objects only after the durable
  command acknowledgement.
- **Safety behavior:** query failures produce an operator-visible catalog warning rather
  than reporting an empty auction house. The no-MySQL outbox publisher continues to
  return retryable failure instead of acknowledging notifications it cannot yet deliver.
- **Checks passed:** the auction repository harness now verifies empty-catalog reads,
  open list/detail fields, post-settlement hiding, refund/proceeds/item projections, and
  query refusal after corruption; `python3 tests/async/test_flatfile_auction_repository.py`,
  `python3 tests/async/test_auction_transactional_cutover.py`, `make -C src -j2`,
  `python3 tests/async/test_flatfile_boot_preflight.py`, `./scripts/format.sh --check`,
  and `git diff --check` pass.
- **Files changed:** `src/flatfile_auction_repository.[ch]`, the no-MySQL branch of
  `src/auction_houses.c`, the focused auction harness, the auction cutover source
  contract, and this handoff ledger.
- **Remaining auction gap:** no-MySQL offer/bid/remove command construction, expiry
  scheduling, committed-event/web/offline publication, and sorter initialization remain
  unavailable. The auction/economy boot diagnostic therefore remains in place.
- **Next action:** route offer, bid, and remove through the already-authoritative auction
  command dispatcher, add flat expiry finalization and catalog-backed committed-event
  publication, then reconsider only the auction portion of the boot diagnostic.

### Checkpoint 27 - client-free auction mutation and expiry routing

- **Completed:** the no-MySQL command UI now constructs typed list and bid payloads from
  bounded price/day/quantity input and the character's canonical account, wallet/bank
  revisions, and runtime item-custody revisions. Listing serializes the representative
  object before submission and removes live inventory only in the acknowledged completion
  callback; rejected or unsubmitted commands leave money and items untouched.
- **Completed:** trusted one-at-a-time removal uses the same typed command route. The
  client-free dispatcher restores fighting/destroying and recent-equipment-removal access
  checks, and exposes offer, bid, and remove alongside the catalog-backed list, info, and
  pickup surfaces.
- **Completed:** auction activity scans the flat open-listing projection for expiry and
  submits background finalize commands with the configured closing fee. An in-memory
  pending-ID set caps and deduplicates submissions while commands are outstanding; the
  durable catalog state and operation ledger remain the authority across restart.
- **Checks passed:** `python3 tests/async/test_auction_transactional_cutover.py` now checks
  that all client-free mutation and expiry routes submit typed commands without SQL or
  direct balance mutation; `python3 tests/async/test_flatfile_auction_repository.py`, the
  four adjacent auction transaction/source regressions, `make -C src -j2`,
  `python3 tests/async/test_flatfile_boot_preflight.py`, `./scripts/format.sh --check`,
  and `git diff --check` pass.
- **Files changed:** the no-MySQL branch of `src/auction_houses.c`, its focused source
  contract, and this handoff ledger.
- **Remaining auction gap:** committed auction events still cannot be safely acknowledged
  without a flat offline-message/publication contract. They remain retryable instead of
  being silently discarded. Stored item sort metadata is currently limited to object
  name keywords, and the richer SQL-era identify text is not generated in the no-MySQL
  listing route. The combined auction/economy boot diagnostic remains in place.
- **Next action:** implement a recoverable flat committed-event/offline-notification
  publisher (with idempotent outbox delivery), then separate the now-covered auction
  authority from the wider economy diagnostic and proceed to boon rewards.

### Checkpoint 28 - durable flat offline mailbox foundation

- **Completed:** added a bounded, checksummed per-player offline-message repository under
  the shared authority lock. Message IDs are 128-bit, records are stored in deterministic
  ID order, exact enqueue replay is idempotent, conflicting ID reuse fails closed, and
  each mailbox is capped at 4,096 messages with 4 KiB per message and a 32 MiB file limit.
- **Completed:** client-free `send_to_pid_offline` and `send_to_char_offline` now enqueue
  generated message IDs instead of discarding text. Login delivery loads messages in
  creation/ID order, sends each message, and atomically acknowledges it afterward. A
  crash after send but before acknowledgement may repeat a message but cannot lose it;
  corrupt or unwritable mailboxes raise persistence alerts and remain untouched.
- **Recovery behavior:** every mailbox entry point takes the same cross-authority lock
  and finishes a surviving critical-authority transaction before reading or publishing,
  so unrelated interrupted auction/player/item publication cannot be bypassed through
  offline delivery.
- **Checks passed:** `python3 tests/async/test_flatfile_offline_message_repository.py`
  covers missing-mailbox reads, deterministic enqueue/list, exact replay, ID conflict,
  single-message acknowledgement, invalid identities, checksum corruption, no-overwrite,
  and temporary-file cleanup; `make -C src -j2`,
  `python3 tests/async/test_flatfile_boot_preflight.py`, `./scripts/format.sh --check`,
  and `git diff --check` pass.
- **Files changed:** new `src/flatfile_offline_message_repository.[ch]` and focused
  harness/test, `src/sql.c`, `src/Makefile`, and this handoff ledger.
- **Remaining event gap:** auction operations do not yet retain a flat pending-publication
  marker, so their deterministic notifications are not staged into these mailboxes after
  command commit. Web publication also needs a replay/ack boundary.
- **Next action:** version the auction catalog operation ledger with pending-event state,
  stage deterministic recipient messages into the new mailbox, publish the web event,
  and acknowledge the catalog event only after both steps succeed.

### Checkpoint 29 - recoverable flat auction event publication

- **Completed:** auction catalog schema v2 adds a durable publication bit to each
  successful externally visible operation. Rejected decisions and money/item claims are
  born acknowledged; list, bid, sale, expiry, and removal remain discoverable as pending
  until publication succeeds. The reader accepts schema v1 and treats its historical
  operations as already published, while every subsequent write upgrades the catalog to
  v2 without replaying unknown historical notifications.
- **Completed:** the client-free activity loop drains up to 16 pending events per pass.
  Outbid, seller-proceeds, winner-claim, expiry, and removal messages receive IDs derived
  from the operation ID and recipient index and are idempotently staged in the durable
  flat mailbox before web publication. Online delivery acknowledges only that mailbox
  record; failed or offline delivery leaves it for login.
- **Completed:** list, bid, and close web events use the catalog projection and the
  operation result. The catalog publication bit is atomically acknowledged only after
  every required mailbox enqueue and broadcast call completes. A crash can repeat a web
  broadcast or live message, but deterministic mailbox IDs prevent duplicate durable
  notifications and no committed event is lost.
- **Completed:** event query/ack APIs take the shared authority lock, recover surviving
  compound transactions, reject a missing listing reference, derive a stable nonzero
  outbox ID, and make acknowledgement idempotent. The repository regression exercises
  pending discovery and acknowledgement for listing, bid, sale, expiry, and removal and
  reconstructs a real schema-v1 catalog to verify upgrade readability.
- **Boot diagnostic:** auction command authority, read/UI paths, expiry, claims, committed
  events, and offline mail are now backed by flat repositories. `auctions` and
  `offline messages` are removed from the unimplemented-domain list; the broader
  `economy` fence remains for boon and other uncovered ledgers.
- **Checks passed:** `python3 tests/async/test_flatfile_auction_repository.py`,
  `python3 tests/async/test_flatfile_offline_message_repository.py`,
  `python3 tests/async/test_auction_transactional_cutover.py`, `make -C src -j2`,
  `python3 tests/async/test_flatfile_boot_preflight.py`, `./scripts/format.sh --check`,
  and `git diff --check` pass; adjacent auction regressions are rerun before publication.
- **Files changed:** `src/flatfile_auction_repository.[ch]`, the no-MySQL auction activity
  and publisher in `src/auction_houses.c`, `src/persistence_mode.c`, the focused auction
  repository and source-contract tests, and this handoff ledger.
- **Next action:** implement the boon reward command against the flat player-domain and
  cross-authority preparation APIs, including durable replay and event publication;
  then audit the remaining economy producers before narrowing that diagnostic.

### Checkpoint 30 - flat boon definition/progress/shop authority

- **Completed:** added a bounded, checksummed `boon_catalog` holding full reward
  definitions, per-boon/player progress, per-player point/stat shop balances, and an
  operation-ID ledger with typed results. Decode rejects duplicate definitions,
  progress rows, shops, or operations; unknown progress definitions; invalid enums,
  flags, non-finite values, corrupt checksums, oversized sections, and ambiguous IDs.
- **Completed:** the flat critical dispatcher now applies typed boon reward commands.
  Eligibility matches the SQL route for racewar/player targeting, zone/level/mob/race/
  frag and event criteria, pet/conjured exclusions, progress increments, one-shot `-1`
  completion fences, repeat reset, targeted deactivation, and point/stat shop credits.
- **Completed:** exact command replay returns the original typed result; conflicting
  operation-ID reuse returns `EEXIST`. More than 32 matching definitions and numeric/
  capacity failures restore the original catalog and durably record only the rejection,
  preventing partial progress or shop credit.
- **Completed:** added an establish-once definition API for export/bootstrap and a bounded
  player shop projection. Boon command construction now rejects non-finite event values
  before they enter either backend.
- **Checks passed:** `python3 tests/async/test_flatfile_boon_repository.py` covers
  establish-once behavior, mob progress, one-shot and repeat completion, point/stat
  balances, replay/conflict, zone and frag eligibility, victim exclusion, durable
  oversized-match rejection, corruption refusal, and temporary-file cleanup;
  `python3 tests/async/test_boon_reward_zone_transactional_cutover.py`, the affected
  standalone auction/item/player repositories, `make -C src -j2`,
  `python3 tests/async/test_flatfile_boot_preflight.py`, `./scripts/format.sh --check`,
  and `git diff --check` pass.
- **Files changed:** new `src/flatfile_boon_repository.[ch]` and focused harness/test,
  `src/boon_reward_command.c`, the flat critical dispatcher, `src/Makefile`, affected
  standalone build lists, and this handoff ledger.
- **Remaining boon gap:** the catalog command durably decides completion, but EXP/epic/
  cash/level/power/spell/stat/item runtime reward effects are still published only from
  an in-memory completion callback. A crash or disconnect after catalog commit can lose
  or defer that reward, and boon admin/list/shop routes remain SQL-backed.
- **Next action:** add a durable pending-reward publication marker and idempotent reward
  handoff. Prepare authoritative wallet/epic/item/player after-images where possible and
  retain pending runtime-only effects until the player is loaded and acknowledgement is
  durable; then port boon read/admin/shop surfaces.

### Checkpoint 31 - recoverable flat boon reward publication

- **Completed:** upgraded the checksummed boon catalog to schema v2. Every successful
  command with reward entries now persists its event data and an unpublished marker in
  the same atomic catalog write as progress/shop state and the typed result. Legacy v1
  catalogs remain readable and their historical operations are treated as already
  published, so an upgrade cannot replay old rewards.
- **Completed:** added bounded pending-reward lookup and idempotent acknowledgement APIs.
  Both run behind the boon authority lock, recover an interrupted generic transaction
  before reading, reject corrupt catalogs, and atomically advance the catalog revision
  when acknowledgement changes state.
- **Completed:** an online completion publishes the runtime effects before acknowledging
  its durable marker. If the player is disconnected or the server restarts first, normal
  entry and reconnect drain up to 64 pending rewards for that PID and acknowledge each
  only after publication. Failed loads or writes retain the marker and emit a persistence
  alert rather than silently dropping the reward.
- **Checks passed:** `python3 tests/async/test_flatfile_boon_repository.py` now also covers
  exact event/result recovery, idempotent acknowledgement, absence after acknowledgement,
  and an actual v1 catalog fixture; the boon/zone source-contract suite guards publish-
  before-ack ordering and both player-ready hooks. The affected auction/item/player
  standalone repositories, `make -C src -j2`,
  `python3 tests/async/test_flatfile_boot_preflight.py`, `./scripts/format.sh --check`, and
  `git diff --check` pass.
- **Files changed:** `src/flatfile_boon_repository.[ch]`, the boon reward transaction and
  player entry/reconnect hooks, focused repository and source-contract tests, and this
  handoff ledger.
- **Remaining boon gap:** delivery is recoverable and at-least-once, not exactly-once. A
  crash after applying a runtime EXP/level/power/spell/stat/item effect (or submitting an
  epic/cash sub-operation) but before acknowledging the catalog can repeat that effect on
  reconnect. The effect families need operation-ID-aware durable after-images or their
  own idempotency ledgers before acknowledgement is crash-atomic. Boon admin/list/shop
  routes also remain SQL-backed.
- **Next action:** make boon effect application idempotent by carrying the parent boon
  operation ID into wallet/epic/item/player authority updates and recording runtime-only
  effects with the player snapshot; acknowledge only after every prepared effect is
  durable. Then port the remaining boon read/admin/shop surfaces.

### Checkpoint 32 - flat boon query primitives

- **Completed:** added locked, recovery-aware definition and per-player progress
  projections to the boon repository. Definitions preserve their canonical ID ordering;
  progress lookup uses the ordered `(boon_id, pid)` key and distinguishes an absent row
  from a stored zero counter. Existing shop projection remains bounded to legacy integer
  consumers at the adapter boundary.
- **Completed:** `is_boon_valid`, `count_boons`, `get_boon_data`,
  `get_boon_progress_data`, and `get_boon_shop_data` now select the flat catalog before
  issuing a query when flat-file primary mode is active. MariaDB-primary behavior is
  unchanged. This removes SQL from the shared definition/progress/shop lookup helpers
  used outside the monolithic display and mutation commands.
- **Checks passed:** the repository harness covers full definition projection, exact
  progress lookup and missing-player behavior in addition to the prior catalog cases;
  the boon/zone source-contract suite verifies that all five legacy helpers route to the
  flat repository before `qry`. `python3 tests/async/test_flatfile_boon_repository.py`,
  the affected auction/item/player standalone repositories, `make -C src -j2`,
  `python3 tests/async/test_flatfile_boot_preflight.py`, `./scripts/format.sh --check`, and
  `git diff --check` pass.
- **Files changed:** `src/flatfile_boon_repository.[ch]`, the lookup adapters in
  `src/boon.c`, focused repository and source-contract tests, and this handoff ledger.
- **Remaining boon gap:** `boon_display`, shop spending, definition create/remove/extend,
  randomization, and maintenance still issue SQL directly. Reward effect publication is
  still at-least-once for live-state effects as described in checkpoint 31.
- **Next action:** route the mortal boon list display over the canonical definition
  projection, including its active/manual/random/author/type/option/player filters and
  progress annotations. Then add typed flat shop spending and administrative mutations;
  design per-effect publication state before changing reward acknowledgement semantics.

### Checkpoint 33 - flat boon definition administration

- **Completed:** added locked create, deactivate, and extend operations to the flat boon
  authority. Create allocates the next monotonic definition ID, validates the complete
  record, and enforces the 99-active-boon limit while holding the authority lock.
  Deactivation is idempotent and preserves history while setting `active=false` and
  `duration=0`, matching the SQL behavior.
- **Completed:** extension rejects forever boons and unsafe time/author inputs, preserves
  any unexpired seconds, starts the requested added duration at that boundary, reactivates
  inactive definitions, and records the legacy `*author` marker. Every changed catalog
  advances its revision and is atomically replaced only after full re-encoding succeeds.
- **Completed:** the existing `create_boon`, `remove_boon`, and `extend_boon` command
  adapters select these operations in flat-primary mode before reaching SQL. Creation and
  extension retain the existing notification behavior, including the distinction between
  extending an active boon and reactivating an inactive one. MariaDB-primary routing is
  unchanged.
- **Checks passed:** the repository harness covers monotonic creation, invalid ID reuse,
  forever-extension rejection, idempotent deactivation, reactivation, expiry-boundary
  calculation, and author persistence. The source-contract suite verifies all three
  adapters route before `qry`; the normal warning-clean build, affected standalone
  repositories, isolated client-free build/boot preflight, formatting, and diff checks
  pass.
- **Files changed:** `src/flatfile_boon_repository.[ch]`, definition mutation adapters in
  `src/boon.c`, focused repository and source-contract tests, and this handoff ledger.
- **Remaining boon gap:** the list renderer, randomization/maintenance queries, legacy
  progress/shop creation helpers, and shop stat spending still issue SQL directly. The
  reward effect acknowledgement gap from checkpoint 31 also remains.
- **Next action:** route the mortal list renderer over the flat definition projection and
  make maintenance enumerate that projection. Then introduce typed, operation-ID-ledgered
  shop spending rather than directly decrementing the shop row and mutating a live stat.

### Checkpoint 34 - flat boon maintenance enumeration

- **Completed:** periodic boon maintenance now enumerates active definitions from the
  canonical catalog in flat-primary mode instead of unconditionally querying `boons`.
  The existing MariaDB query remains isolated to its backend branch. Each selected ID is
  still loaded through the backend-aware definition helper before expiry/completability
  checks, and a failed lookup is skipped rather than processing zero-initialized data.
- **Completed:** the enumeration uses a bounded vector of validated IDs. The catalog's
  locked create path already caps active definitions, while the SQL compatibility path
  retains its historical bounded collector. Expiry and void actions now reach the flat
  deactivate operation added in checkpoint 33.
- **Checks passed:** the boon source-contract suite verifies catalog enumeration and
  flat-before-SQL ordering; the normal warning-clean build, isolated client-free
  build/boot preflight, formatting, and diff checks pass.
- **Files changed:** `src/boon.c`, the focused boon source-contract test, and this handoff
  ledger.
- **Remaining boon gap:** `boon_display`, the disabled random-boon implementation, and
  shop spending remain SQL-coupled. Maintenance decisions for epic zones and nexus state
  also depend on those subsystems' current authority status.
- **Next action:** extract row formatting from `boon_display` so the same trusted/mortal
  renderer consumes either SQL rows or flat definitions without duplicating presentation
  logic. Then implement typed, ledgered shop spending.

### Checkpoint 35 - client-free boon list command

- **Completed:** `boon list` now branches to the canonical definition projection before
  constructing or issuing its SQL query in flat-primary mode. The adapter preserves the
  default active/manual/random selection and the full explicit filter set: active,
  inactive, manual, random, author `LIKE` patterns, type, option, assigned player,
  racewar visibility, and trusted-player visibility.
- **Completed:** author matching implements bounded, case-insensitive SQL-style `%` and
  `_` wildcards without recursive backtracking. Definition/type/option/player filters use
  typed vectors rather than constructing query fragments. Results remain ID ordered and
  report the same result count contract.
- **Completed:** flat trusted output exposes all canonical definition columns, assignment,
  repeat marker, and remaining duration. Mortal output exposes the qualifying definition's
  duration, racewar, type, option, and reward amount without leaking definitions assigned
  to other players or racewar sides. MariaDB-primary retains the existing detailed
  natural-language renderer unchanged.
- **Checks passed:** the source-contract suite verifies flat-before-SQL routing and every
  filter/visibility input; the normal strict build, isolated client-free build/boot
  preflight, formatting, and diff checks pass.
- **Files changed:** `src/boon.c`, the focused boon source-contract test, and this handoff
  ledger.
- **Remaining boon gap:** flat mortal list rows are intentionally concise and do not yet
  share the MariaDB renderer's zone/mob/item/spell natural-language expansion. Typed shop
  spending and exactly-once reward effect publication remain higher-risk gaps; the random
  boon implementation is disabled in both backends.
- **Next action:** implement operation-ID-ledgered boon shop spending with a durable player
  stat after-image, then return to a shared presentation-only row formatter. Do not make
  shop points durable separately from the selected player stat mutation.

### Checkpoint 36 - command-authoritative player base stats

- **Completed:** upgraded the player-domain sidecar to schema v3 with a base-stat
  revision and the ten canonical STR/DEX/AGI/CON/POW/INT/WIS/CHA/KARMA/LUCK values.
  Validation rejects values outside 0..100 and rejects nonzero stat data without an
  authority revision. The file revision now includes the stat revision alongside wallet,
  epic, and frag revisions.
- **Completed:** first player-domain establishment derives all ten values from the
  identity-verified full player snapshot and establishes stat revision 1. On load, the
  sidecar overrides checkpoint base stats only when that revision is present, following
  the same stale-snapshot protection already used for wallet/epic/frag state. MariaDB
  loads and legacy sidecars leave the snapshot values authoritative.
- **Completed:** v1/v2 player-domain files remain readable with stat revision zero, and
  pending v2 transaction envelopes remain recoverable after the v3 upgrade. New writes
  publish v3. This avoids manufacturing zero stats during upgrade and creates a safe
  command-authority target independent of ordinary checkpoint-save locking.
- **Checks passed:** the player-domain harness covers v3 stat round-trip and a real
  v3-to-v2 compatibility fixture; the full player repository harness proves initial
  snapshot derivation and load projection. Player-load/read contracts, affected
  auction/item/boon standalone repositories, the normal strict build, isolated
  client-free build/boot preflight, formatting, and diff checks pass.
- **Files changed:** the player load DTO, flat player-domain codec, initial-domain
  derivation, load materializer, focused player/domain harnesses and source contracts,
  and this handoff ledger.
- **Remaining shop gap:** no command mutates the new stat authority yet. Boon shop balance
  and base-stat updates must be prepared under one authority lock, committed as one
  generic transaction, and replayed by operation ID before the live character is changed.
- **Next action:** add a bounded boon-shop command/result codec and player-domain stat
  after-image preparation API; combine it with the boon catalog after-image in one
  recoverable transaction, then route `boon shop stat` through the coordinator.

### Checkpoint 37 - atomic flat boon stat redemption

- **Completed:** added a bounded, canonical boon-shop command/result codec keyed by the
  player and selected base-stat index. The durable result records the authoritative stat
  value/revision and remaining shop stat points, so retries and live publication consume
  the committed outcome rather than re-running purchase logic.
- **Completed:** upgraded the boon catalog to schema v3 with a separate operation-ID and
  command-digest ledger for shop purchases. V1 catalogs without reward-publication data
  and v2 catalogs with it remain readable; new writes emit v3. Successful and rejected
  purchases replay exactly, while operation-ID reuse with a different command is rejected.
- **Completed:** the player-domain repository can prepare one bounded base-stat increment
  as an after-image while the caller owns the global authority lock. The boon repository
  decrements one stat point and commits the catalog plus player-domain after-images in
  one generic recoverable authority transaction. Insufficient points, capped stats,
  legacy stat authority, corruption, and capacity failures do not partially mutate
  either side.
- **Completed:** flat-primary `boon shop stat` now submits the typed command before any
  live character, SQL, or catalog mutation. Completion publication applies the committed
  stat value to the connected character and recomputes affects; a disconnected player
  receives the authoritative value through normal player-domain materialization on the
  next load. MariaDB-primary retains the existing SQL path.
- **Checks passed:** the boon repository harness proves successful replay, conflicting
  identity rejection, durable insufficient-points rejection, v1 catalog compatibility,
  and forced interruption after the catalog image followed by recovery of both images
  exactly once. The codec/routing suite, player-domain suite, affected item/auction/player
  repository suites, normal strict build, isolated client-free build/boot preflight,
  formatting, and diff checks pass.
- **Files changed:** boon-shop command and live transaction modules, critical command and
  flat dispatcher registration, boon catalog/player-domain repositories, game-loop and
  shop routing, focused harnesses/source contracts, the build manifest, and this handoff
  ledger.
- **Remaining boon gap:** boon-point item purchases are not implemented (the shop exposes
  no items in either backend), and the flat mortal list still uses concise typed rows
  instead of the MariaDB renderer's natural-language zone/mob/item/spell expansion. The
  random-boon implementation remains disabled in both backends.
- **Next action:** extract a presentation-only boon row formatter shared by SQL and flat
  projections, then return to the remaining fail-closed durable-domain inventory rather
  than inventing a flat-only boon-point item catalog.

### Checkpoint 38 - shared typed boon presentation

- **Completed:** extracted duration, racewar, reward-type, completion-option, assignment,
  and row formatting from the MariaDB result loop into one presentation-only renderer
  over `BoonData`. MariaDB rows are converted to that typed DTO after visibility checks;
  filtered flat definitions already use the same DTO and now enter the identical renderer.
- **Completed:** flat mortal listings now receive the same natural-language reward and
  completion descriptions as MariaDB listings, including zone, mob, race, guildhall,
  outpost, nexus, CTF, cargo, auction, item, spell, affect, attribute, and currency labels.
  Trusted rows retain their canonical numeric columns and also receive the shared detail
  line. Backend-specific retrieval, filtering, and visibility rules remain outside the
  renderer.
- **Safety repair:** display-time affect lookup now walks each terminated flag table
  instead of indexing it with unchecked persisted data. Spell and attribute names are
  likewise bounded before lookup, and malformed values render explicit invalid labels.
  This removes three crash/read-overrun paths shared by corrupt SQL and flat definitions.
- **Checks passed:** the focused boon source-contract suite verifies flat-before-SQL
  routing, all filters, both calls into the shared renderer, natural-language lookup
  coverage, and invalid-value guards. The flat boon repository regression, normal strict
  build, isolated client-free build/boot preflight, formatting, and diff checks pass.
- **Files changed:** `src/boon.c`, the focused boon source-contract test, and this handoff
  ledger.
- **Remaining boon gap:** boon-point item purchases and random generation remain absent
  in both backends; no flat-only behavior was invented. The implemented boon query,
  maintenance, administration, reward, pending-publication, list, and stat-redemption
  surfaces now share their intended backend-neutral behavior.
- **Next action:** bring learned crafting recipes into the revisioned player snapshot and
  remove their external-sidecar fence, including typed flat runtime query/mutation routes
  and compatibility handling for snapshots that still declare recipes external.

### Checkpoint 39 - revisioned flat recipe authority

- **Design correction:** learned recipes are not resident on `char_data`; crafting reads
  them lazily through the existing `sql_*_player_recipe` API and mutates them individually
  when learned or wiped. Duplicating that set into an otherwise unused in-memory cache
  solely for snapshot capture would create two live authorities. Recipes therefore remain
  truthfully marked external in the player snapshot and now have one typed external
  authority behind the same gameplay API.
- **Completed:** added a `DURRCPE` v1 recipe catalog with SHA-256 validation, monotonic
  revision, canonical PID ordering, sorted unique positive recipe vnums, a 65,536-recipe
  per-player bound, a 1,048,576-player bound, and a 256 MiB file bound. A missing catalog
  fails closed; within an established catalog an absent PID canonically means an empty set.
- **Completed:** establish, list, contains, add, and clear operations share the global
  authority lock and generic transaction recovery boundary. Establishment is exact and
  idempotent for exporter/rehearsal use; add and clear are idempotent runtime mutations.
  Corrupt authority is never treated as empty and is never overwritten by a mutation.
- **Completed:** the client-free `sql_add_player_recipe`, `sql_delete_player_recipes`,
  `sql_has_player_recipe`, and `sql_get_player_recipes` routes now use the flat catalog and
  raise persistence alerts on read/write failures. MariaDB-primary retains its existing
  SQL. Flat-primary crafting never falls through to the name-keyed legacy `.crafting`
  files when the authoritative set is empty or unavailable, preventing stale resurrection.
- **Checks passed:** the standalone repository regression covers missing-catalog failure,
  canonical establishment, exact retry/conflict, empty-player projection, membership,
  idempotent add/clear, eight concurrent process writers, checksum corruption, and temp
  cleanup. The runtime source contract proves all four routes, alerts, legacy-file bypass,
  MariaDB query preservation, and build registration. The normal strict build, isolated
  client-free build/boot preflight, Python compile, formatting, and diff checks pass; CI
  now runs the repository regression in the client-free job.
- **Files changed:** the flat recipe repository and harness, client-free player SQL adapter,
  crafting legacy-import guard, build/CI manifests, focused runtime contract, and this
  handoff ledger.
- **Remaining recipe gap:** the future SQL-to-flat exporter must establish the catalog,
  and safe character deletion must clear the PID row in the same recoverable identity
  transaction. Rename needs no recipe mutation because authority is PID-keyed. Recipes
  therefore remain in the boot-time unimplemented inventory until export and deletion
  composition exist.
- **Next action:** inventory `player_spellbooks` learned-mob state and route it through a
  similarly bounded PID-keyed authority, then combine both external knowledge sets with
  the eventual character-delete transaction and exporter manifest.

### Checkpoint 40 - revisioned flat conjuration spellbook authority

- **Completed:** added a distinct `DURSPBK` v1 learned-minion catalog with SHA-256
  validation, monotonic revision, canonical PID ordering, sorted unique positive mob
  vnums, a 65,536-mob per-player bound, a 1,048,576-player bound, and a 256 MiB file
  bound. A missing catalog fails closed; an absent PID within established authority is an
  authoritative empty set.
- **Completed:** exact establishment, list, membership, add, single-minion remove, and
  clear operations share the global authority lock and generic transaction recovery
  boundary. Runtime mutations are idempotent, concurrent additions are serialized, and
  corrupt authority is neither projected as empty nor overwritten.
- **Completed:** every client-free `player_spellbooks` API now uses the typed catalog and
  raises persistence alerts on failures. MariaDB-primary retains its existing insert,
  membership, ordered-list, and full-delete behavior and gains the missing parameterized
  single-minion delete needed by gameplay.
- **Correctness fix:** `conjure remove` previously read and rewrote an uninitialized legacy
  filename even though all other learned-minion operations had moved to SQL. It now calls
  the authoritative single-minion delete API in both modes. Learning also requires the
  catalog/SQL insert to succeed before reporting success, preventing a failed persistence
  write from falsely telling the player that the minion was learned.
- **Checks passed:** the standalone repository regression covers missing-catalog failure,
  canonical establishment, exact retry/conflict, authoritative empty projection,
  membership, idempotent add/remove/clear, eight concurrent process writers, checksum
  corruption refusal, and temporary-file cleanup. The runtime source contract covers all
  five flat API routes, alerts, the legacy-file removal fix, persistence-before-success,
  MariaDB query preservation, and build registration. The strict normal build passes and
  the isolated client-free build/boot preflight passes; CI runs both focused spellbook
  checks in the client-free job.
- **Files changed:** the spellbook repository and harness, player SQL API/adapters,
  conjuration gameplay, build/CI manifests, focused runtime contract, and this handoff
  ledger.
- **Remaining spellbook gap:** the future SQL-to-flat exporter must establish the catalog,
  and safe character deletion must clear the PID row together with recipes in the same
  recoverable identity transaction. Rename requires no catalog mutation because both
  knowledge authorities are PID-keyed. Spellbooks therefore remain in the boot-time
  unimplemented inventory until export and deletion composition exist.
- **Next action:** design the exporter manifest/establishment pass for the now-typed recipe
  and learned-minion catalogs, then compose their clears with character deletion rather
  than leaving either catalog orphaned.

### Checkpoint 41 - recoverable cross-store authority removals

- **Completed:** upgraded the bounded `DURAUTH` transaction journal to version 2 with
  explicit write/remove operations and typed `domains/`, `players/`, or
  `identities/names/` targets. Target names remain single safe filenames, duplicate
  operations on the same store/name pair are rejected, writes require non-empty bounded
  after-images, and removes require empty payloads.
- **Compatibility:** recovery still decodes and replays version-1 write-only journals
  produced by prior checkpoints. New callers can submit mixed operations, while the
  existing after-image API remains a compatibility wrapper that emits version-2 domain
  writes. Corrupt or structurally invalid journals continue to fail closed in place.
- **Crash semantics:** the journal is durably published before any operation. Recovery
  replays writes and idempotent removals in order, synchronizes each target directory via
  the atomic store, and removes the journal only after every operation succeeds. Thus an
  interruption can no longer make cross-store deletion irrecoverable merely because one
  target was already removed.
- **Checks passed:** the focused fault-injection regression proves interrupted domain
  writes, interrupted `players/` plus `domains/` removals, idempotent missing-file replay,
  unsafe/payload-invalid rejection, version-1 journal recovery, corrupt-journal refusal,
  and success-last journal cleanup. The dependent player-domain, boon, recipe, spellbook,
  item, and auction repository regressions, strict normal build, and isolated client-free
  build/boot preflight also pass. CI now runs the transaction regression explicitly.
- **Files changed:** the generic authority transaction API/codec, its standalone harness,
  CI, and this handoff ledger.
- **Remaining deletion gap:** repositories must expose validated prepared operations for
  the identity tombstone, player snapshot, PID gameplay-domain record, recipe catalog,
  and spellbook catalog. The runtime delete route must then commit that exact set under
  one authority lock; independent clears remain intentionally unacceptable.
- **Next action:** add those repository prepare APIs and a typed character-delete
  coordinator, with fault injection proving recovery from every operation boundary.

### Checkpoint 42 - prepared identity and knowledge deletion primitives

- **Completed:** the identity repository now exposes an owner-only RAII lock and a
  prepare-only tombstone operation. Preparation verifies the global authority lock, PID,
  expected canonical name, active state, catalog checksum/schema, and revision bound,
  then returns the next checksummed identity-catalog image without publishing it.
- **Completed:** recipe and learned-minion repositories now expose equivalent prepare-only
  PID clears under an already-held global authority lock. They recover an outstanding
  journal first, fail closed on missing/corrupt authority, distinguish an absent PID in an
  established catalog from missing authority, and return revision-incremented catalog
  images without any independent write.
- **Locking correction:** normal identity mutations now use the same public identity lock
  implementation as coordinated deletion. This preserves the existing process mutex and
  `.identity.lock` serialization while allowing a coordinator to hold identity state
  stable, then acquire the global authority lock in one documented direction.
- **Checks passed:** identity and account-membership regressions still pass. Expanded
  identity, recipe, and spellbook harnesses commit their prepared operations through the
  generic journal and verify the resulting tombstone/empty projection. The cross-store
  harness now also applies an identity-catalog image in the interrupted mixed transaction.
  The strict normal build and isolated client-free build/boot preflight pass.
- **Files changed:** identity, recipe, spellbook, and authority transaction APIs/codecs;
  their focused harnesses/link manifests; and this handoff ledger.
- **Deletion-set correction:** snapshot/domain/knowledge alone is insufficient. Item
  ownership and per-player boon redemption are also PID-keyed flat authorities, while
  locker, ship, auction, artifact, and other character-delete side effects still need an
  explicit precondition or transactional disposition. The runtime delete route remains
  closed rather than claiming a partial delete.
- **Next action:** add prepared player-snapshot, player-domain, item-owner, and boon-player
  operations with compatible lock ordering, then inventory and fence the remaining
  world-side effects before exposing the coordinator through `sql_delete_player`.

### Checkpoint 43 - prepared player, custody, and boon deletion primitives

- **Completed:** promoted the existing per-PID snapshot mutex/file lock to a reusable RAII
  lock and made normal snapshot saves use it unchanged. Under that lock plus the global
  authority lock, deletion preparation authenticates the complete checksummed snapshot
  before returning a typed `players/<pid>.snapshot` removal operation.
- **Completed:** player-domain preparation authenticates the PID gameplay-domain envelope
  and returns a typed removal for `player-<pid>.domain` without touching account-scoped
  bank authority. Both snapshot and domain operations remain unpublished until a future
  coordinator commits the complete operation set.
- **Completed:** item-owner preparation loads and validates the global custody catalog,
  moves every item owned by the PID to the singleton destruction owner, marks custody
  destroyed, advances item/destruction/catalog revisions, removes the player owner row,
  and returns one checksummed catalog image. Item UIDs and historical command results are
  retained; a retry observes the absent player owner instead of destroying twice.
- **Completed:** boon-player preparation removes the PID's progress, shop balances,
  pending/replayed reward operations, and shop operations. Targeted boon definitions are
  deactivated and have their target PID cleared, preventing a tombstoned identity from
  remaining embedded in global active configuration.
- **Checks passed:** expanded player, item, and boon repository regressions verify prepared
  operation type/name/content, journal publication where applicable, authoritative empty
  projections, item-owner idempotency, and targeted-definition cleanup. Formatting and
  strict focused compilation, the normal server build, and the isolated client-free
  build/boot preflight pass.
- **Files changed:** player snapshot/domain, item ownership, and boon repositories and
  harnesses, plus this handoff ledger.
- **Remaining coordinator gap:** locker access/data, ships, auctions, artifacts/guild
  references, corpses, offline messages, and other name/PID world state still require an
  explicit transactional cleanup or a checked-empty precondition. The coordinator and
  `sql_delete_player` remain deliberately unrouted until that inventory is complete.
- **Next action:** derive the exact remaining character-delete side-effect manifest from
  current call sites and flat authorities, implement fail-closed preconditions for domains
  not yet transactionally deletable, then compose all prepared operations with identity
  last in one fault-injected coordinator.

### Checkpoint 44 - recoverable core character-delete coordinator

- **Completed:** added a typed core coordinator that locks one PID snapshot, then identity,
  then global authority; validates the identity/name tombstone plus player snapshot,
  player-domain, item custody, boon, recipe, and spellbook authorities; and commits their
  prepared operations in one bounded `DURAUTH` transaction. The identity catalog image is
  ordered last, so an interruption cannot visibly tombstone a character before the
  preceding core removals have been journaled for deterministic replay.
- **Idempotency and fail-closed semantics:** prepare-only repositories now distinguish
  “catalog exists and the PID is already absent” from “catalog is missing.” Recovery of an
  interrupted journal therefore converges to `already_deleted`, while a missing or corrupt
  authority aborts before publication. A tombstoned identity with residual core authority
  but no recovery journal is rejected as inconsistent rather than silently cleaned up.
- **Locking correction:** the coordinator acquires the snapshot lock before the identity
  lock. First-snapshot establishment already holds that snapshot lock while consulting
  identity authority, so this order avoids an inverse lock dependency; global authority is
  acquired last and remains the serialization boundary shared by domain/catalog writers.
- **Checks passed:** the focused coordinator regression covers a normal delete and retry,
  missing-boon fail-closed behavior with no journal or authority changes, interruption
  after the first operation, durable mixed-store journal recovery, identity tombstoning,
  snapshot/domain removals, knowledge clears, item-owner removal, and success-last journal
  cleanup. The five affected repository regressions, formatting, and strict normal server
  build pass. CI now runs the coordinator regression explicitly in the client-free job.
- **Files changed:** the core deletion coordinator, build and CI registration, repository
  result contracts, focused fault-injection harness, dependent exhaustive result mapping,
  and this handoff ledger.
- **Exposure remains fenced:** this coordinator is intentionally not connected to
  `sql_delete_player` or the live `deleteCharacter` call path. That path also mutates
  locker access/data, artifacts and guild references, association membership, account
  membership, ships, auctions/corpses, offline messages, and other world state. Each must
  receive a transactional disposition or a checked-empty precondition before the runtime
  can claim complete character deletion.
- **Next action:** turn the live delete call graph into a machine-checked side-effect
  manifest, implement fail-closed preconditions/preparers for the remaining authorities,
  and only then route the complete transaction through the backend-neutral delete API.

### Checkpoint 45 - offline-message cleanup and auction deletion fence

- **Completed:** per-PID offline messages now expose a prepare-only typed removal under the
  held global authority lock. A present file must pass its schema, bounds, ordering, and
  checksum validation before its domain removal joins the character-delete journal; a
  missing file is the repository's authoritative empty state and produces no operation.
- **Completed:** the auction repository now exposes a held-lock player-reference check.
  Deletion is rejected with a conflict if the PID occurs as any listing seller, winner,
  item claimant, or money-pickup owner. Missing auction authority retains the repository's
  established empty-catalog semantics, while corrupt or unreadable authority fails closed.
  The conservative check includes historical listings intentionally: deletion will remain
  fenced until a later auction disposition can distinguish and rewrite safe history.
- **Coordinator integration:** auction validation occurs before any prepared core image is
  accepted, and offline-message removal is ordered before the identity tombstone in the
  same recoverable transaction. The operation bound is now eight. Interrupted recovery
  verifies that the offline file disappears together with snapshot/domain/knowledge/item
  state and that the final identity tombstone remains success-last.
- **Call-graph inventory:** the live route still performs artifact release, locker-access
  deletion, association kick/save, account-character and leaderboard soft deletion,
  optional locker-content deletion, core player deletion, and name-keyed ship deletion.
  Auction and offline-message references were not explicit calls but are PID authorities
  that could otherwise outlive the identity; they are now represented in the coordinator.
- **Checks passed:** the offline-message repository, auction repository, expanded
  coordinator fault/retry regression, formatting, and strict normal server build pass.
  Auction tests prove referenced/clear preflight outcomes, and CI now runs both adjacent
  repository regressions alongside the coordinator in the client-free job.
- **Exposure remains fenced:** lockers/private chests, artifacts and bindings,
  guild/association authority, frag leaderboard history, ships/cargo, corpses/saved world
  items, and other schema-derived PID references still lack a complete disposition. The
  live `deleteCharacter` and `sql_delete_player` routes remain unchanged.
- **Next action:** codify the explicit call graph plus schema-derived references as a
  checked manifest, then implement the locker and artifact preparers/preconditions before
  addressing name-keyed ships and association authority.

### Checkpoint 46 - checked character-delete disposition manifest

- **Completed:** added a versioned machine-readable deletion inventory covering the live
  call graph, every authority currently composed by the core coordinator, and
  schema-derived references that are not explicit calls. Each entry records its identity
  key, source class, current disposition, and a repository/source/schema evidence token.
- **Drift enforcement:** the focused validator requires the exact inventory IDs, unique
  entries, known dispositions, safe repository-relative evidence paths, and live evidence
  tokens. It also checks the durable call order in `deleteCharacter`, proves the incomplete
  coordinator is not routed there, and verifies the prepared core order keeps identity
  success-last.
- **Explicit blockers:** the manifest currently reports eight unimplemented groups:
  artifacts, locker access, association membership, frag leaderboard state, locker
  contents, ships/cargo, corpses/saved items, and account-bound summons. While any entry is
  unimplemented, `runtime_exposure` must remain `fenced`; the validator rejects a readiness
  claim that contradicts those dispositions.
- **CI and handoff:** the validator runs beside the fault-injected coordinator regression
  in the client-free job. Adding, removing, or reordering a durable live delete call now
  requires an intentional manifest/test update instead of silently changing the deletion
  contract.
- **Files changed:** the deletion manifest, focused validator, CI registration, and this
  handoff ledger.
- **Next action:** implement flat locker-access/content authority or an authenticated
  checked-empty precondition, then add artifact/binding disposition; keep the manifest as
  the gate for reducing the blocker count.

### Checkpoint 47 - canonical flat artifact authority and deletion disposition

- **Completed:** added a `DURARTF` v1 artifact catalog that replaces three legacy SQL
  projections plus their revision state with one canonical record per vnum. Each record carries
  ownership, location type/key, artifact timer/type/update time, soul-binding owner/timer,
  and a monotonic record revision; the catalog adds its own revision, canonical vnum order,
  fixed-width encoding, SHA-256 validation, and bounded record/file counts.
- **Exporter boundary:** exact establishment canonicalizes ordering and is idempotent only
  for an identical catalog. Missing authority fails closed, conflicting establishment is
  rejected, and list reads never synthesize an empty catalog. This provides the typed
  target needed to reconcile `artifacts`, `artifacts_mortal`, `artifact_bind`, and
  `artifact_domain_state` during a future SQL-to-flat export rather than perpetuating four
  live authorities.
- **Deletion disposition:** under the held global authority lock, player-held artifacts are
  made unowned/not-in-game with cleared location/timer, and any soul binding to the PID is
  released. Every changed artifact revision and the catalog revision advance once, and the
  checksummed after-image joins the core character-delete journal before snapshot/item and
  identity removal. A retry is unchanged and cannot double-advance revisions.
- **Corpse safety:** an artifact recorded on the deleting PID's corpse returns a conflict
  before any image is published. Corpse item authority is still unimplemented, so silently
  releasing that catalog row would leave contradictory corpse contents. The existing
  corpse/saved-item blocker remains the required disposition boundary.
- **Checks passed:** the standalone regression covers missing-authority refusal, canonical
  exact establishment, conflicting establishment, round-trip decoding, unreferenced retry,
  prepared journal publication, held/bound release, untouched-record preservation,
  per-record revisions, corpse conflict/no mutation, checksum corruption refusal, and
  corrupt-delete refusal. The expanded coordinator fault/recovery test proves artifact
  release converges with all other core operations. Strict normal compilation passes and
  CI now runs the artifact regression in the client-free job.
- **Manifest and exposure:** artifact deletion advances from `unimplemented` to
  `prepared_rewrite`, reducing the checked runtime blocker count from eight to seven. The
  broader artifact gameplay/read/update domain is not yet routed to this catalog, so
  `artifacts/economy` correctly remains in boot's unimplemented-domain fence and the live
  delete route remains closed.
- **Files changed:** artifact repository/codec and harness, coordinator/build/CI wiring,
  deletion manifest/validator, and this handoff ledger.
- **Next action:** implement the player-locker catalog with item UID custody integration
  while keeping account lockers separate, then compose locker access/content removal into
  deletion without touching account-scoped storage.

### Checkpoint 48 - revisioned flat frag-leaderboard authority

- **Completed:** added a `DURFRAG` v1 PID-keyed leaderboard catalog covering account and
  character display names, total frags, racewar/race/class/level dimensions, soft-delete
  and update timestamps, and a monotonic row revision. The catalog has its own revision,
  canonical PID ordering, strict string/count/size bounds, fixed-width encoding, and
  SHA-256 validation.
- **Exporter and runtime writes:** exact establishment is canonical and idempotent only for
  identical source rows; missing/corrupt authority fails closed. Client-free
  `sql_update_frag_leaderboard` now builds a typed row from live character state and
  performs a revisioned upsert, while client-free `sql_modify_frags` refreshes that row
  after frag changes. Failed writes raise a persistence alert instead of disappearing in
  the former no-op stub.
- **Deletion composition:** the core coordinator prepares a soft-delete timestamp and row
  revision update under the held authority lock, then includes the catalog image before
  snapshot/domain/item removal and the success-last identity tombstone. An absent PID in
  an established catalog and a previously tombstoned row are idempotent; a missing catalog
  remains a hard failure.
- **Checks passed:** the repository regression covers missing-authority refusal, canonical
  exact establishment, conflicting establishment, signed frag totals, round-trip decoding,
  update/insert ordering and revisions, absent-PID tombstone, journal publication,
  timestamped soft deletion, retry stability, and corrupt read/write refusal. The expanded
  coordinator recovery test verifies the leaderboard tombstone, the runtime source
  contract rejects restoration of the no-op, and the strict normal build passes. CI runs
  the focused regression in the client-free job.
- **Manifest and remaining gap:** frag leaderboard deletion advances from `unimplemented`
  to `prepared_rewrite`, reducing the checked blocker count from seven to six. Native
  client-free ranking/filter/list projections in `fraglist.c`, Redis/web publication, and
  level-cap aggregation are not yet routed, so the broader economy/statistics boot fence
  remains truthful and the live delete route stays closed.
- **Files changed:** frag-leaderboard repository/codec and harness, client-free SQL update
  adapter, coordinator/build/CI wiring, deletion manifest/validator, and this handoff
  ledger.
- **Next action:** extract the nested item snapshot codec used by player persistence so a
  complete locker catalog can encode chest metadata and object payloads while the existing
  item ledger remains the sole UID/custody authority.

### Checkpoint 49 - reusable bounded nested-item snapshot codec

- **Completed:** exposed the player persistence item's existing binary list envelope as a
  typed encode/decode API. The shared boundary carries the complete item snapshot,
  including UID/generated identity, parent/equipment placement, mutable object fields,
  fixed and dynamic affects, extra descriptions, and embedded spell IDs. It does not
  introduce a second object representation or change the enclosing player snapshot wire
  format.
- **Fail-closed bounds:** standalone encoding and decoding apply the same maximum payload,
  object/row, string, and nesting limits as player snapshots. Both directions validate
  parent ordering and maximum depth; decoding also rejects empty input, truncation,
  trailing bytes, allocation failure, and malformed relationships before publishing an
  output vector.
- **Locker authority boundary:** the upcoming locker catalog can now persist a complete
  chest object tree as one checksummed, bounded payload. That payload is materialized
  object state only: the existing global item repository remains the sole authority for
  object UID ownership and custody, so locker publication must compose its catalog image
  with the corresponding item-owner image in one recoverable transaction.
- **Checks passed:** the focused regression round-trips a nested container and all major
  variable-width fields, and rejects truncation, trailing bytes, self-parenting,
  over-depth trees, and oversized strings. Existing player-repository and save-journal
  regressions pass unchanged, as does the strict normal server build. CI now runs the
  standalone item-codec regression beside player materialization.
- **Files changed:** shared player item codec API and focused harness, CI registration, and
  this handoff ledger.
- **Next action:** implement one canonical player/association locker catalog covering
  locker and chest metadata, name-keyed access, and per-chest nested object payloads; then
  compose locker deletion with item-custody removal without touching account lockers.

### Checkpoint 50 - canonical player and association locker catalog

- **Completed:** added a `DURLOCK` v1 catalog for the legacy `lockers`,
  `private_chests`, `locker_items` and child detail tables, and `locker_access` authority.
  Each locker records stable ID, canonical name, exactly one player or association owner,
  race dimensions, revision, and its public/private chests. Each chest records stable ID,
  canonical name, password hash, public marker, sort configuration, revision, and the full
  bounded nested-item payload introduced in Checkpoint 49. Access rows are canonical
  owner/visitor pairs with revisions.
- **Authority and bounds:** exact establishment canonicalizes locker, chest, and access
  order and is idempotent only for identical source state. Reads fail closed on missing,
  corrupt, oversized, non-canonical, or checksummed-invalid authority. Validation requires
  globally unique locker/chest IDs and item UIDs, unique names at their schema scopes,
  exactly one public chest per locker, valid access-to-locker references, one owner kind,
  and valid nested object placement with no equipped locker items.
- **Account boundary:** names under `account.*` are rejected deliberately. The separate
  `account_lockers`, `locker_chests`, `account_locker_items`, and account access/session/log
  tables are account-scoped and must not be deleted or silently imported as character
  locker state.
- **Custody boundary:** item payloads are materialized state, not an alternate ownership
  ledger. Establishment rejects duplicate object UIDs inside the locker catalog, but the
  exporter and runtime mutation paths must still reconcile and atomically compose these
  payloads with `item_ownership` locker owners `{locker_id, chest_id}` before either
  authority can be exposed for gameplay.
- **Checks passed:** the focused strict regression proves missing-authority refusal,
  canonical exact establishment and retry, player and association lockers, public/private
  metadata, access rows, nested container/detail round trip, conflict rejection, account
  boundary enforcement, duplicate-UID and structural rejection, dangling-access refusal,
  and checksum corruption refusal. CI runs it in the client-free authority suite and the
  normal server build includes the repository.
- **Exposure and deletion remain fenced:** current SQL locker save/load/access/chest calls
  are not yet routed to this catalog. Character deletion still needs one prepared catalog
  rewrite that removes visitor access and, when requested, the PID-owned locker together
  with one item-custody after-image. The checked deletion blocker count therefore remains
  six.
- **Next action:** add the held-lock locker deletion preparer and a combined item-owner
  removal API for all locker chest identities, then compose both images in the recoverable
  character-delete transaction and advance the two locker manifest entries together.

### Checkpoint 51 - transactional locker and item-custody deletion

- **Completed:** the locker repository now prepares one held-lock catalog rewrite for
  character deletion. It removes the PID-owned player locker, every access row owned by
  that locker, and every access row where the deleting canonical character name is the
  visitor. Association lockers and unrelated access remain intact. The catalog also
  enforces one locker per player PID or association ID.
- **Exact custody handoff:** the prepared locker result exposes every removed chest as the
  stable item owner `{locker_id, chest_id}` plus its sorted expected UID/vnum set. The item
  repository's combined player-and-locker removal validates that each chest owner exists
  and its active custody set matches exactly before preparing one `item_ownership` image.
  Missing owners, extra/missing items, UID mismatch, or vnum mismatch fail closed before a
  transaction journal is published; empty chests require an explicitly established empty
  owner as part of the authority.
- **Coordinator composition:** character deletion now prepares the locker plan before
  player snapshot removal, publishes the combined item-custody image followed by the
  locker catalog image, and still keeps the identity tombstone success-last in the same
  recoverable `DURAUTH` transaction. The bounded operation count is eleven. Recovery after
  any partial publication converges, and a retry sees absent locker/access/item owners as
  already deleted rather than advancing revisions again.
- **Account boundary preserved:** only PID-owned entries in `DURLOCK` participate. The
  account locker schema is neither represented by this catalog nor addressed by its
  deletion preparer, so deleting a character cannot remove account-scoped storage.
- **Checks passed:** locker tests cover prepared-but-unpublished state, exact custody
  extraction, owner/visitor access cleanup, association-locker preservation, transactional
  publication, retry stability, and duplicate owner rejection. Item tests cover exact
  UID/vnum reconciliation, empty chest owners, combined player/chest destruction, and
  mismatch refusal. The expanded fault-injected coordinator test proves journal recovery
  removes player and locker custody, the player locker, and both access directions while
  retaining the association locker. All four focused repository/coordinator/manifest
  regressions pass.
- **Manifest and exposure:** `locker_access` and `locker_contents` advance from
  `unimplemented` to `prepared_rewrite`, reducing the checked runtime blocker count from
  six to four. Live locker gameplay save/load/chest/access calls are still SQL-only, and
  association membership, ships/cargo, corpses/saved items, and account-bound summons
  still block runtime character-delete exposure.
- **Next action:** implement canonical association membership authority and its prepared
  character removal, then address name-keyed ships/cargo without weakening the corpse and
  account-bound summon fences.

### Checkpoint 52 - canonical association authority and membership deletion

- **Completed:** added a checksummed `DURASSC` v1 catalog consolidating the active `guilds`,
  `guild_ranks`, and `guild_members` projections. Each association carries its stable ID,
  display name, racewar/bits, prestige/construction, coin balances, total and top frag
  aggregates, fixed rank titles, revision, and canonical PID-keyed members. Each member
  carries canonical name, permission/rank bits, debt, online projection, captured frag
  contribution, and revision.
- **Exporter and integrity boundary:** exact establishment canonicalizes association and
  member ordering and is idempotent only for identical state. Validation enforces unique
  association IDs, globally unique member PIDs, printable bounded text, canonical member
  and top-fragger names, consistent empty top-fragger aggregates, bounded counts/files,
  revisions, and SHA-256 integrity. Missing or corrupt authority fails closed.
- **Deletion semantics:** under the held global authority lock, the PID/name member is
  removed, its captured frag contribution is subtracted with checked signed arithmetic,
  and matching top-fragger name/value state is cleared. The association and catalog
  revisions advance once; absent membership is an idempotent unchanged result, while a
  PID/name mismatch or arithmetic overflow conflicts before publication. This reproduces
  the durable effects of `Guild::kick` instead of deleting only the membership row and
  leaving guild aggregates stale.
- **Coordinator composition:** the prepared association image joins the same recoverable
  character-delete transaction before snapshot/domain/item removal and the success-last
  identity tombstone. The bounded operation count is now twelve. Fault recovery proves
  the remaining member and association survive while the deleting member, frag
  contribution, and top-fragger state converge exactly.
- **Checks passed:** the standalone strict regression covers canonical establishment/list,
  ranks/balances/members, retry/conflict behavior, duplicate cross-association PID refusal,
  aggregate validation, prepare-before-publish isolation, transactional member removal,
  frag/top-fragger updates, retry stability, and checksum corruption. The expanded
  coordinator and manifest regressions pass, and CI runs the association repository in the
  client-free authority suite.
- **Manifest and exposure:** `association_membership` advances from `unimplemented` to
  `prepared_rewrite`, reducing the checked deletion blocker count from four to three.
  General client-free guild gameplay/load/save remains unrouted and stays inside the wider
  guild domain boot fence; ships/cargo, corpses/saved items, and account-bound summons still
  prevent live character-delete exposure.
- **Next action:** implement canonical ship and cargo authority with name/PID ownership and
  exact item-custody composition, then retain explicit corpse and account-bound summon
  fences until their repositories are complete.

### Checkpoint 53 - canonical ship, crew, armor, slot, and cargo authority

- **Completed:** added a checksummed `DURSHIP` v1 catalog consolidating `ships`,
  `ship_armor`, `ship_crew`, and `ship_slots`. Each record carries stable ship and owner
  PIDs, canonical owner name, display name, class/frags/location/time/sail/race/money/flags,
  all armor/internal sides, fixed-point crew skills and chiefs, canonical indexed slots,
  and a revision. Slot type/item/position/timer/value fields include the cargo and
  contraband quantities and pricing state used by the runtime.
- **Authority boundary:** exact establishment canonicalizes ship and slot ordering and is
  idempotent only for identical state. Validation enforces unique ship IDs, owner PIDs and
  canonical owner names, unique bounded slot indexes, printable bounded names, revisions,
  maximum record/file sizes, and SHA-256 integrity. Missing, conflicting, oversized, or
  corrupt authority fails closed.
- **Deletion semantics:** under the held global authority lock, deletion resolves by owner
  PID and verifies the canonical expected character name before preparing removal of the
  complete ship aggregate. The catalog revision advances once; absent ownership is an
  idempotent unchanged result and a PID/name mismatch conflicts. Ship cargo is persisted as
  slot scalar state rather than object UID custody, so no global item-owner rewrite is
  required for this domain.
- **Coordinator composition:** the ship image joins the recoverable character-delete
  transaction after association cleanup and before player snapshot removal, with the
  identity tombstone still success-last. The bounded operation count is thirteen. Fault
  recovery proves the deleting player's ship and cargo slots do not survive.
- **Checks passed:** the standalone strict regression covers exact canonical establishment,
  metadata, armor, crew fixed-point fields, slot/cargo round trip, owner and slot uniqueness,
  conflict refusal, prepare-before-publish isolation, transactional removal, unrelated ship
  preservation, retry stability, and checksum corruption. The expanded coordinator and
  manifest regressions pass, and CI runs the ship repository in the client-free authority
  suite.
- **Manifest and exposure:** `ships_and_cargo` advances from `unimplemented` to
  `prepared_remove`, reducing the checked deletion blocker count from three to two. General
  ship gameplay/load/save remains SQL-only and stays in the wider ship domain boot fence;
  corpses/saved items and account-bound summons still prevent live character-delete
  exposure.
- **Next action:** implement corpse and saved-world-item authority with nested object and
  exact item-custody reconciliation, then address account-bound summon references before
  exposing the complete character-delete route.

### Checkpoint 54 - canonical corpse and saved-world-item catalog

- **Completed:** added a checksummed `DURWRLD` v1 `world_item_catalog` consolidating PC
  corpse metadata and its nested contents with saved ground-item trees. Corpse records
  carry owner PID, canonical owner name, save ID, room, descriptive fields, weight, all
  runtime value slots, revision, and complete nested item payloads. Saved-world records
  carry their runtime item key, room, revision, and complete nested item payloads.
- **Authority boundary:** both collections reuse the exact bounded player-item snapshot
  envelope, preserving stable UID/generated identity, prototype and mutable scalar state,
  dynamic affects, extra descriptions, spellbook metadata, and parent topology without a
  second object codec. Establishment canonicalizes corpse `(owner PID, save ID)` and saved
  item-key ordering and is idempotent only for identical state.
- **Validation:** the catalog rejects absent or conflicting corpse identities, inconsistent
  names across one PID, reuse of a canonical owner name by another PID, duplicate saved
  keys, more than one saved-item root, malformed or over-deep nesting, equipment placement,
  invalid vnums, missing or duplicate UIDs across either collection, invalid revisions,
  unsafe bounds, checksum corruption, and trailing data. Missing or corrupt authority
  fails closed.
- **Custody semantics:** corpse contents remain owned by the stable encoded
  `(owner PID, save ID)` corpse identity; saved ground trees remain room custody and are
  not attributed to a character merely because both legacy tables shared an object
  persistence shape. Character deletion will therefore remove only matching corpse
  aggregates and reconcile those exact corpse owners with the global item ledger while
  preserving unrelated saved room trees.
- **Checks passed:** the strict standalone regression covers canonical establishment/list,
  metadata and nested-state round trip, retry/conflict behavior, cross-collection UID
  collision refusal, malformed nesting and multiple-root refusal, duplicate corpse
  identity refusal, and checksum corruption. Changed-line formatting, the normal server
  build, and the client-free boot preflight pass; CI now runs the world-item repository.
- **Manifest and exposure:** the schema-derived deletion entry remains deliberately
  `unimplemented` in this checkpoint. Catalog authority alone does not prove atomic corpse,
  item-ledger, and artifact disposition, so both deletion blockers and the live route fence
  remain unchanged.
- **Next action:** prepare PID/name-verified corpse removal, return exact corpse custody
  evidence, compose it with item destruction and corpse-held artifact release in the
  recoverable character-delete transaction, and only then advance the manifest entry.

### Checkpoint 55 - transactional corpse, item-custody, and artifact deletion

- **Completed:** world-item deletion now resolves every corpse owned by the deleting PID,
  verifies the canonical expected character name, and prepares one catalog image that
  removes those aggregates while preserving unrelated corpses and all saved room-item
  trees. Each non-empty removed corpse returns a sorted, exact custody proof containing
  its encoded `(PID, save ID)` item-owner identity and every UID/vnum pair.
- **Exact custody reconciliation:** the item repository's composed deletion path now
  accepts both locker and corpse custody proofs. It requires every supplied owner to exist,
  requires its complete active ledger set to exactly match the materialized UID/vnum set,
  rejects duplicate or incorrectly typed owners, and only then moves all player, locker,
  and corpse items to destroyed custody in one ownership after-image. Empty corpses require
  no synthetic ledger owner.
- **Artifact disposition:** artifact release now covers both direct player placement and
  corpse placement keyed to the deleting PID, in addition to clearing binding state. Held
  artifacts become unowned/not-in-game with zero location and timers; unrelated artifact
  records are preserved. This removes the prior corpse-artifact conflict only because the
  corpse catalog and item-ledger images now join the same transaction.
- **Coordinator composition:** PID/name-verified corpse preparation precedes artifact
  release. The ownership image publishes before the locker and world-item catalog images,
  and the identity tombstone remains success-last. The bounded operation count is fourteen;
  retry recovery remains idempotent.
- **Checks passed:** strict world-item, artifact, item-ownership, and character-delete
  regressions pass. The fault-injected deletion test proves journal recovery removes the
  player corpse, corpse ledger owner, locker owner, and other character authorities,
  releases player/corpse artifacts, preserves an unrelated saved room tree, and leaves no
  recovery journal. Changed-line formatting, the normal build, and the client-free build
  and boot preflight also pass.
- **Manifest and exposure:** `corpses_and_saved_items` advances from `unimplemented` to
  `prepared_remove`, reducing the checked deletion blocker count from two to one. The live
  character-delete route remains fenced because account-bound summon references still lack
  a flat authority and explicit deletion disposition.
- **Next action:** implement the account-bound summon/reference authority, compose its
  player cleanup into deletion, and then audit the complete manifest and runtime exposure
  gate before routing the live terminal path.

### Checkpoint 56 - account-reward summon authority and zero-blocker deletion manifest

- **Completed:** added a checksummed `DURSUMN` v1
  `account_reward_summon_catalog` for the character-reference state formerly held in
  `account_bound_reward_summons`. Each canonical record carries stable account-grant ID,
  character PID, absolute last-summoned timestamp, recovery-ready state, and revision.
- **Authority boundary:** exact establishment sorts by the legacy `(grant ID, PID)` primary
  key and is idempotent only for identical state. Validation rejects zero identities,
  duplicate key pairs, negative timestamps, missing revisions, oversized catalogs,
  checksum corruption, and trailing data. The account-level reward grant itself is not
  deleted or reassigned by this character-reference repository.
- **Deletion semantics:** under the held global authority lock, character deletion prepares
  removal of every summon/cooldown row for the deleting PID regardless of grant, while
  preserving the same grants' rows for all other characters. An absent PID is an idempotent
  unchanged result; missing or corrupt authority fails closed.
- **Coordinator composition:** the summon catalog image joins the recoverable transaction
  immediately after the auction reference check. The bounded operation count is fifteen,
  and the identity tombstone remains success-last. Fault recovery proves the deleting
  PID's summon row is absent while another PID's row for the same grant survives.
- **Checks passed:** strict standalone establishment/list, canonical ordering, duplicate and
  timestamp validation, prepare-before-publish isolation, transactional removal, unrelated
  PID preservation, retry stability, and checksum corruption tests pass. The expanded
  fault-injected character-delete and manifest tests, changed-line formatting, normal
  build, and client-free build/boot preflight pass; CI includes the new repository.
- **Manifest and exposure:** `account_bound_summons` advances from `unimplemented` to
  `prepared_remove`. The machine-readable deletion inventory now has zero unimplemented
  entries. Runtime exposure intentionally remains `fenced` for a separate caller-semantics
  audit, particularly the legacy `bDeleteLocker=false` paths; zero inventory blockers alone
  do not authorize changing those call sites.
- **Next action:** classify every live `deleteCharacter` caller, define flat semantics for
  partial/transient deletion requests, then route only supported terminal deletion through
  the coordinator and update the exposure contract with focused source/runtime tests.

### Checkpoint 57 - live full-character deletion route

- **Completed:** `deleteCharacter` now selects the recoverable flat coordinator when the
  active backend is `flatfile-primary`. A successful or already-recovered deletion returns
  success and refreshes the descriptor's in-memory account-character list only after the
  durable transaction has completed. Any authority, identity, custody, corruption, or I/O
  failure returns failure without falling through to legacy SQL mutations.
- **Caller semantics:** the audit classified the default calls from player confirmation,
  forced deletion, hardcore terminal death, account menus, administration, and web routes
  as full lifecycle deletion. The two `bDeleteLocker=false` calls are partial cleanup of a
  failed just-created character and an old locker proxy during rename; flat-primary mode
  rejects those requests instead of incorrectly applying a full deletion transaction.
  MariaDB modes retain their existing behavior and call order.
- **Exposure contract:** the machine-readable manifest now declares `runtime_exposure` as
  `enabled`. Its regression proves the flat-primary mode guard, partial-request refusal,
  coordinator call, post-commit in-memory account refresh, and early return all precede the
  untouched legacy artifact/locker/association/soft-delete/account/player/ship sequence.
- **Checks passed:** the fault-injected coordinator regression still proves atomic recovery
  across all fifteen possible authority images, and the zero-blocker manifest/live-route
  contract passes. Changed-line formatting, the normal server build, and the isolated
  client-free build and flat boot preflight pass.
- **Scope note:** this completes the character-delete slice and safely exposes it through
  the selected backend; it does not remove the wider flat boot fence for unrelated durable
  domains or make the overall backend production-ready.
- **Next action:** return to the broader P2 inventory and implement the next boot-fenced
  world/domain authority and runtime routes, keeping the same exact export, fail-closed,
  recovery, and checkpoint discipline.

### Checkpoint 58 - canonical shopkeeper aggregate authority

- **Completed:** added a checksummed `DURSHOP` v1 `shopkeeper_catalog` consolidating the
  legacy `shopkeepers`, `shopkeeper_affects`, `shopkeeper_items`, item affects, and extra
  descriptions. Each aggregate carries stable shop ID, mob and room vnums, absolute save
  time, revision, canonical saveable affects, and one complete item topology covering both
  equipment slots and inventory/container contents.
- **Shared object fidelity:** shopkeeper objects reuse the bounded nested item snapshot
  envelope, preserving allocated UID/generated identity, mutable scalar and string state,
  fixed/dynamic affects, extra descriptions, prototype differences, and parent topology.
  Top-level equipment retains the legacy one-based slot representation; inventory and every
  contained item use the zero slot marker.
- **Validation:** exact establishment canonicalizes shops and affect ordering and is
  idempotent only for identical state. The catalog rejects duplicate shop IDs, invalid
  mob/room/save/revision values, malformed or over-deep nesting, nested equipment markers,
  duplicate top-level equipment slots, invalid vnums, and duplicate or absent UIDs across
  all shops, plus oversized, corrupt, or trailing data. Missing authority fails closed.
- **Migration requirement:** the legacy SQL shopkeeper writer commonly omitted `obj_uid`
  even after the column existed. Export must allocate and reconcile stable UIDs before
  establishing this authority; the flat catalog refuses to perpetuate anonymous objects.
- **Checks passed:** the strict standalone regression covers canonical establishment/list,
  affect ordering, equipment/inventory/nested item round trip, retry/conflict behavior,
  cross-shop UID collision refusal, invalid equipment topology, and checksum corruption.
  Changed-line formatting, the normal server build, and the client-free build/boot preflight
  pass; CI includes the repository.
- **Exposure:** shopkeeper runtime save/restore remains fenced. Catalog authority does not
  itself validate world prototypes, materialize NPCs/objects, or prove UID ownership at
  boot, so the wider `shopkeepers` boot blocker is unchanged.
- **Next action:** add prototype-aware shopkeeper capture/materialization adapters and
  reconcile exported UIDs with global item ownership before routing startup restore and
  dirty-save paths through this catalog.

### Checkpoint 59 - shopkeeper equipment-slot contract correction

- **Corrected before exposure:** adapter inspection showed that the existing capture and
  SQL shopkeeper paths use one-based equipment slots and `0` for inventory or contained
  objects. The v1 catalog validator and regression now enforce that exact convention rather
  than the initially documented `-1` marker.
- **Safety effect:** only top-level records may carry a positive equipment slot, positive
  slots remain unique within one shopkeeper, and every inventory or nested record must carry
  zero. This matches `sql_save_shopkeeper_item`, `capture_item_tree`, and the existing
  materializer's attachment contract without an implicit conversion layer.
- **Checks passed:** the strict repository regression and changed-line formatting pass.
  No shopkeeper runtime route or exported catalog was exposed under the earlier convention.
- **Next action:** expose a bounded shared capture adapter using the corrected slot contract,
  then build prototype-aware materialization and UID reconciliation.

### Checkpoint 60 - reusable bounded live item capture adapter

- **Completed:** exposed `player_item_snapshot_list_capture` as the shared live-object to
  value-DTO adapter. Callers select equipment and/or inventory traversal and explicitly
  choose whether `ITEM_NORENT` subtrees are omitted; successful capture returns the complete
  item list and optional conservative byte estimate.
- **Safety semantics:** the adapter reuses the existing player snapshot traversal, depth,
  object/row/byte/string limits, cycle detection, prototype validation, string-mask rules,
  fixed/dynamic affect capture, spellbook/extra-description conversion, parent topology,
  and one-based equipment-slot contract. It mutates no live object and assigns the caller's
  output only after the entire graph succeeds.
- **Compatibility:** player inventory/equipment and pet capture retain their prior
  `ITEM_NORENT` omission behavior by passing the policy explicitly. Shopkeeper capture can
  preserve the legacy SQL behavior by requesting all items without cloning traversal code.
- **Checks passed:** the expanded immutable-capture source/DTO contract, changed-line
  formatting, normal server build, and isolated client-free build/boot preflight pass.
- **Next action:** implement the shopkeeper capture adapter around this API, including
  saveable NPC affects and catalog revision handling, then add prototype-aware load
  materialization and UID ownership reconciliation.

### Checkpoint 61 - bounded live shopkeeper capture adapter

- **Completed:** added `flatfile_shopkeeper_capture`, which converts one validated live
  shopkeeper NPC into a detached catalog aggregate containing stable shop ID, prototype and
  room vnums, caller-supplied revision/save time, saveable affects, and the complete
  equipment/inventory object graph.
- **Safety semantics:** capture rejects PCs, non-shopkeeper NPCs, invalid prototype/room
  indexes, zero revisions, negative timestamps, cyclic affect lists, malformed item graphs,
  and anonymous zero-UID items. It skips `AFFTYPE_NOSAVE` affects and publishes the output
  only after both affect and item traversal succeed; allocation failure remains retryable.
- **Legacy fidelity:** the adapter uses the shared item capture with no-rent omission
  disabled, matching the existing SQL shopkeeper save behavior while retaining bounded
  traversal, one-based equipment slots, zero inventory/contained markers, and complete
  nested object state.
- **Checks passed:** the focused immutable/source contract and normal C++20 server build
  pass. CI now runs the capture contract beside the strict shopkeeper repository regression.
- **Exposure:** capture performs no repository I/O or live-world mutation. Runtime save and
  restore remain fenced until catalog revisions, prototype-aware materialization, and global
  UID ownership reconciliation are composed into the startup and dirty-save paths.
- **Next action:** add prototype-aware shopkeeper materialization and reconcile every
  catalog UID with the global ownership authority before exposing runtime restore.

### Checkpoint 62 - exact shopkeeper item custody reconciliation

- **Completed:** added an append-only `shopkeeper` item-owner type and a stable nonzero
  owner-ID mapping that covers the full zero-based `shop_id` range. Flat shop aggregates
  can now be joined to the global item authority without masquerading as rooms, players,
  or generic containers.
- **Exact reconciliation:** the adapter requires the shop aggregate and active custody set
  to have identical cardinality, UID, vnum, root, parent, owner, and revision evidence. It
  derives parent/root identities from the bounded parent-before-child topology and emits
  materializer identities only after every row agrees; missing, extra, anonymous,
  duplicated, foreign-owner, or structurally divergent custody fails closed.
- **Compatibility:** owner type 9 is appended after every published owner value. Fresh SQL
  schemas accept the widened range, and the guarded, re-runnable
  `shopkeeper_item_owner.sql` migration widens existing named checks without changing any
  stored owner value. Flat catalog decoding and runtime ownership validation accept the
  same identity.
- **Checks passed:** exact nested reconciliation, zero/maximum shop-ID mapping, atomic
  output on mismatch, item ownership source contracts, runtime ownership hydration, the
  strict flat item repository, all shopkeeper regressions, changed-line formatting, and
  the normal C++20 server build pass. CI includes the reconciliation regression.
- **Exposure:** this establishes custody evidence but does not yet instantiate mobile/object
  prototypes or place restored keepers into the world. Runtime shopkeeper restore remains
  fenced.
- **Next action:** generalize the proven item graph materializer to accept an explicit
  validated owner, then build an all-or-nothing detached shopkeeper materializer on the
  reconciled identities.

### Checkpoint 63 - owner-aware bounded item graph materialization

- **Completed:** extracted `player_load_item_graph_materialize_for_owner` from the proven
  player loader. It accepts one explicit validated active owner while retaining the same
  prototype lookup, metadata validation, topology/depth/operation bounds, staged object
  rollback, nesting checks, equipment/inventory attachment, and optional atomic runtime
  ownership hydration.
- **Compatibility and guards:** the existing PID API remains unchanged and still rejects
  nonpositive PIDs before constructing a player owner. The shared entry point rejects
  invalid, system, and destruction owners; every supplied item identity must exactly match
  the explicit owner and owner revision. Player and pet callers retain their prior paths.
- **Checks passed:** the expanded runtime regression materializes and hydrates a shopkeeper
  owned graph through the shared entry point, while the complete player and pet graph suites,
  changed-line formatting, and normal C++20 server build pass.
- **Exposure:** this is a reusable internal primitive only. No shopkeeper mobile is created
  or published by this checkpoint, and runtime restore remains fenced.
- **Next action:** compose detached mobile creation, saveable affect restoration, reconciled
  item materialization, and failure cleanup into a shopkeeper-specific materializer.

### Checkpoint 64 - detached prototype-aware shopkeeper materialization

- **Completed:** added a shopkeeper-specific materializer that resolves the saved mobile and
  room vnums, loads and exactly reconciles global item custody before allocating a mobile,
  instantiates the declared shopkeeper prototype, restores birthplace and saveable affects,
  and materializes the complete nested equipment/inventory graph under the shopkeeper owner.
- **All-or-nothing staging:** unknown prototypes/rooms, non-shopkeeper prototypes, invalid
  affect widths, missing/corrupt/divergent custody, object prototype failures, nesting or
  bound violations, allocation failures, and runtime ownership conflicts return a typed
  failure. A staged mobile is extracted—with its affects, events, equipment, and inventory—
  on every post-allocation failure. The caller output is assigned only after success and the
  mobile remains outside any room pending batch publication.
- **Flat snapshot fidelity:** complete-state mode additionally restores all six timers,
  anti/anti2/extra2 flags, and craftsmanship while SQL-backed player/pet callers retain
  their existing prototype-diff behavior. Dynamic object affects currently lack remaining
  duration in the shared v1 item envelope, so complete-state materialization rejects any
  such row instead of silently converting it to a permanent affect.
- **Checks passed:** focused ordering/rollback/source contracts, exact custody reconciliation,
  complete-state scalar restoration, the full player and pet item hydration suites,
  changed-line formatting, and the normal C++20 server build pass. CI includes the detached
  materializer contract.
- **Exposure:** individual detached materialization is ready, but world replacement is not
  yet routed. Runtime restore remains fenced until the entire catalog can be staged before
  existing keepers are replaced and rooms are published.
- **Next action:** implement batch staging/publication with duplicate shop/prototype checks,
  produced-item policy, existing-keeper replacement, rollback-safe cleanup, and explicit
  boot failure propagation.

### Checkpoint 65 - catalog-wide shopkeeper staging and publication

- **Completed:** added a catalog restore coordinator that loads the canonical authority,
  validates every saved shop ID against the live shop table and keeper prototype, rejects
  duplicate keeper vnums, and materializes the entire catalog before placing or replacing
  any world shopkeeper.
- **Produced-item policy:** every configured produced prototype must already appear as a
  top-level inventory object in the authoritative aggregate. The flat backend does not
  synthesize an anonymous replacement at boot; the captured instance retains its UID and
  remains the shop's produced/infinite-stock exemplar.
- **Publication and rollback:** all replacements are placed in their exact declared rooms
  before incumbents are touched. Placement failure or unexpected room redirection extracts
  every staged keeper and forgets all newly hydrated runtime UIDs, leaving incumbents in
  place. Only after complete placement does the coordinator extract matching old prototypes,
  excluding the replacement pointers, and mark the corresponding shops dirty.
- **Checks passed:** the focused source contract proves list-before-stage, complete
  stage-before-place, place-before-replace ordering, exact shop/produced-item checks,
  hydrated-UID unwind, replacement exclusion, and post-publication dirty marking. The normal
  C++20 server build and changed-line formatting pass; CI includes the coordinator contract.
- **Exposure:** the coordinator is compiled but not yet selected by `restore_shopkeepers`.
  Dynamic object affects remain a deliberate v1 materialization fence, and dirty-save
  publication still needs a recoverable catalog/custody transaction.
- **Next action:** add the recoverable dirty-save path, then route boot restore and periodic
  saves through flat authority only when both paths can fail closed.

### Checkpoint 66 - revisioned custody-fenced shopkeeper saves

- **Completed:** the shop catalog now supports compare-and-swap replacement of one aggregate.
  A replacement must advance exactly one record revision, match the expected stored revision,
  preserve whole-catalog validity and global UID uniqueness, and publish through the atomic
  checksummed catalog write. Stale writers and skipped revisions fail distinctly.
- **Dirty-save fence:** the live save adapter loads the current record, captures the keeper at
  `revision + 1`, and requires the captured UID/vnum/root/parent topology to match the active
  global shopkeeper custody set exactly before attempting the revisioned catalog update.
  Missing, extra, stale, foreign, anonymous, or otherwise divergent custody prevents the
  catalog write.
- **Retry behavior:** the batch dirty scanner locates each keeper only in its configured room
  and clears `shop_index[].dirty` only after capture, custody reconciliation, and catalog CAS
  all succeed. Missing keepers, capture/authority errors, or stale publication leave the bit
  set for an explicit retry and make the batch report failure.
- **Checks passed:** the strict repository regression covers successful replacement round
  trip, stale expected revision, and nonconsecutive revision refusal. The focused save
  contract proves capture-before-custody-before-publication-before-dirty-clear ordering;
  changed-line formatting and the normal C++20 server build pass. CI includes the save test.
- **Exposure:** boot and periodic wrappers remain unchanged. Current shop buy/sell code does
  not transfer UID custody, so the new save path correctly refuses those divergent live
  inventories rather than blessing them into the catalog.
- **Next action:** route every shop purchase, sale, duplicate destruction, and produced-item
  creation through the item transaction authority, then select the flat restore/save wrappers
  and remove the shopkeeper boot blocker.

### Checkpoint 67 - backward-compatible shop transfer reason vocabulary

- **Completed:** item-transfer payload version 3 appends explicit `shop_buy` and `shop_sell`
  reasons without renumbering any existing reason or changing the fixed payload layout. New
  commands publish version 3 while the decoder continues to accept version 2 commands using
  the original reason range; a version 2 payload cannot smuggle either new reason.
- **Shopkeeper key validity:** the general critical-command validator now recognizes the
  append-only shopkeeper entity type. This closes a gap where a shopkeeper transfer could be
  assembled but would fail the shared command validity gate before durable publication.
- **Checks passed:** the executable compatibility regression proves v3 shop-buy round trip,
  general command validation, legacy v2 player-transfer decode, and rejection of a v2
  shop-buy reason. The ownership source/runtime/repository regressions, changed-line
  formatting, and the normal C++20 server build pass. CI runs the compatibility regression
  with the flat-file item authority checks.
- **Exposure:** this checkpoint only establishes the durable protocol vocabulary. Live shop
  buy/sell still moves items and currency synchronously, and publishing an item transfer
  independently of payment would create a split money/item commit boundary. Shopkeeper death
  and duplicate/trash destruction also remain synchronous custody mutations.
- **Next action:** design and implement one recoverable shop operation boundary that couples
  payment with item custody, including produced-item creation and duplicate/trash destruction,
  before routing live shop commands or selecting the flat restore/save wrappers.

### Checkpoint 68 - bounded composite shop-trade command

- **Completed:** added an append-only `shop_trade` critical-command type with a versioned,
  pointer-free codec for the four durable outcomes: purchase of existing stock, purchase of
  produced stock, sale retained by the shop, and sale destroyed as duplicate/trash.
- **Atomic intent:** one command now carries the player/account identity, positive bounded
  price, wallet and bank revisions, shop aggregate revision, selected root UID, sorted bounded
  item graph and item revisions/states, plus a 128 KiB bounded item-snapshot blob. Its lock set
  covers the player, account, zero-safe shopkeeper identity, and every item UID; produced-item
  intents require absent revisions while all other actions require active non-absent custody.
- **Completion contract:** the fixed result envelope can publish post-commit wallet/bank
  balances and revisions, shop revision, both item-owner revisions, and every resulting item
  revision. Strict decoding rejects malformed topology, mismatched fences, unsupported actions,
  noncanonical creation state, nonzero reserved bytes, and trailing result data.
- **Checks passed:** the executable codec regression covers full critical-command
  encode/decode, shop ID zero mapping, fence tampering, produced-item absent-state enforcement,
  result round trip, and reserved-tail rejection. Changed-line formatting and the normal C++20
  server build pass; CI runs the codec with item-authority regressions.
- **Exposure:** neither MariaDB nor flat-file apply routing recognizes the new command yet, and
  no live shop path submits it. This is an intentionally inert protocol boundary until a single
  repository transaction can publish wallet, item custody, and shop aggregate after-images.
- **Next action:** add reusable locked prepare APIs for shop aggregate mutation and generic item
  transfer, then implement the flat repository apply path using one authority transaction and
  idempotent operation result before adding runtime submission/completion handling.

### Checkpoint 69 - produced-stock exemplar fence

- **Corrected:** shop-trade payload version 2 distinguishes the persistent produced-stock
  exemplar from the newly allocated purchased clone. Produced purchases now carry the exemplar
  UID, active item revision, and vnum in addition to the clone graph's absent UIDs, preventing a
  repository from approving creation merely because an unrelated item shares the requested
  prototype.
- **Lock coverage:** the exemplar is an additional item key and expected revision for produced
  purchases. Existing-stock purchases require the stock identity to be the selected root and
  to repeat its vnum/revision exactly; sale actions reject stray stock fields.
- **Compatibility:** v2 is the only emitted form. The decoder retains v1 support for existing
  purchases and both sale actions by reconstructing their unambiguous stock metadata, while
  rejecting v1 produced purchases because that format cannot identify or fence an exemplar.
- **Checks passed:** the strict codec regression covers v2 exemplar fences, v1 existing-stock
  compatibility, v1 produced-stock rejection, absent clone enforcement, full command/result
  round trips, and tamper detection. Changed-line formatting and the normal C++20 server build
  pass.
- **Exposure:** the command remains unrouted and therefore cannot mutate either backend. The
  correction landed before repository or runtime adoption, but compatibility is retained so an
  inert queued v1 intent cannot be misinterpreted after future activation.
- **Next action:** implement the lock-scoped shop aggregate prepare API, including exact item
  blob/topology validation and produced-exemplar comparison, then compose it with wallet and
  custody after-images.

### Checkpoint 70 - lock-scoped shop aggregate trade preparation

- **Completed:** the shopkeeper repository can now prepare, but not independently publish, a
  trade after-image while the caller holds the whole-authority lock. It recovers pending
  authority transactions first, validates the command envelope and canonical item snapshot
  blob, checks the exact shop record revision, advances the record/catalog revisions once, and
  returns the encoded catalog for a later multi-authority commit.
- **Inventory transitions:** existing purchases require the stored subtree to byte-match the
  submitted canonical snapshot before removing and reindexing exactly that subtree. Produced
  purchases require a top-level, unequipped exemplar with the fenced UID and vnum and leave it
  in place. Stored sales append and reindex the transferred graph; duplicate/trash sales advance
  the shop revision without adding inventory.
- **Fail-closed state:** snapshot UID/vnum/root/parent topology must match the sorted custody
  entries exactly, roots must be unequipped, stale/missing aggregates return explicit result
  codes, and whole-catalog UID/equipment invariants are revalidated before encoding. Stored
  sales with dynamic object affects remain rejected because complete shopkeeper materialization
  cannot yet restore their remaining duration safely.
- **Checks passed:** the strict repository harness prepares and commits each of the four action
  classes through the authority transaction layer, verifies produced exemplars remain, exact
  purchased stock disappears, stored sales appear, destroyed sales do not, stale revisions
  yield no after-image, and dynamic-affect sales fail closed. The command codec regression,
  changed-line formatting, and the normal C++20 server build pass.
- **Exposure:** this API never writes by itself and shop commands still cannot reach it. Item
  custody and wallet after-images, operation idempotency, and one aggregate commit remain
  prerequisites for routing the command.
- **Next action:** generalize the flat item repository's locked transfer preparation for all
  four shop actions, including produced creation and destruction, then compose it with the shop
  and wallet preparations in an idempotent shop-trade repository.

### Checkpoint 71 - lock-scoped shop custody preparation

- **Completed:** the item repository now translates each validated shop-trade action into the
  existing full-graph transfer engine while the caller holds the whole-authority lock. It
  derives current owner revisions from the locked ledger, enforces every submitted item
  revision/topology/state, and returns the ownership catalog after-image without writing it.
- **Action semantics:** existing purchases move shopkeeper custody to the player with
  `shop_buy`; produced purchases validate a top-level active exemplar and create the absent
  clone graph from system custody; retained sales move player custody to the shopkeeper with
  `shop_sell`; duplicate/trash sales move the graph to destruction. Produced purchases leave
  the exemplar and shop owner revision unchanged.
- **Completion state:** the prepare result exposes the post-mutation player/shop owner
  revisions plus every item UID and revision for eventual runtime publication. A stale exemplar,
  missing owner, conflicting UID, wrong source, malformed graph, or revision mismatch produces
  no after-image.
- **Checks passed:** the strict item repository harness commits all four prepared action classes
  through the authority transaction layer and verifies the exact final player/shop custody sets,
  produced clone revision, exemplar preservation, transfer revisions, destruction, and stale
  exemplar refusal. Shop command/catalog regressions, changed-line formatting, and the normal
  C++20 server build pass.
- **Exposure:** wallet, shop aggregate, and custody preparations now exist independently, but no
  shop-trade repository yet combines them or records an idempotent operation result. The command
  remains unrouted.
- **Next action:** add an idempotent flat shop-trade operation catalog, invoke all three prepare
  paths under one authority lock, encode the unified result, and publish every after-image in one
  recoverable authority transaction.

### Checkpoint 72 - atomic idempotent flat shop trades

- **Completed:** added a dedicated checksummed, versioned shop-trade operation catalog keyed by
  operation ID and the SHA-256 digest of the complete command. Exact retries return the stored
  result, reuse of an operation ID for different bytes fails with `EEXIST`, and semantic
  rejections are durably recorded without publishing any player, shop, or custody change.
- **Atomic mutation:** the repository acquires one whole-authority lock, recovers any interrupted
  transaction, preflights the expected wallet/bank identity and revisions, prepares item custody
  and shop inventory, applies the signed price delta, and commits the operation result, shop
  catalog, item ownership catalog, account bank, and player-domain after-images as one recoverable
  transaction. The fixed completion result records all resulting money, shop, owner, and item
  revisions.
- **Crash and replay evidence:** the executable integration regression interrupts publication
  after the operation-catalog image, proves the transaction intent remains, triggers recovery
  through a player-domain load, and verifies the exact debit, stock removal, custody transfer,
  revision advances, and idempotent result replay. It also proves digest conflict rejection and
  durable stale-command rejection without a second mutation.
- **Routing and checks:** the flat selected critical-command repository now routes `shop_trade`
  commands to the composite repository. The new regression runs in CI with the shopkeeper
  authority checks; affected item, auction, player, and character-delete link/regression harnesses,
  changed-line formatting, and the normal C++20 server build pass.
- **Exposure:** no live shop command submits or completes this command yet, and the MariaDB apply
  path does not implement the composite command. The flat boot blocker therefore remains; this
  repository is reachable only by an explicitly assembled critical command or test.
- **Next action:** implement the runtime shop buy/sell submission and completion adapter without
  mutating money or inventory ahead of the durable result, and add MariaDB command parity before
  selecting that route in normal operation.

### Checkpoint 73 - shop-trade completion publication boundary

- **Corrected owner result:** the second item-owner revision at the existing fixed wire offset is
  now explicitly the action-dependent counterparty revision: shopkeeper for existing buys and
  retained sales, system for produced creation, and destruction for duplicate/trash sales. This
  preserves the result size and field layout while ensuring every changed transfer endpoint can be
  published to the runtime registry; the prior shop-only interpretation omitted the changed
  system/destruction owner revision. New results carry an explicit version byte; legacy unversioned
  existing-buy/retained-sale results remain readable, while unsafe legacy produced/destroyed
  results fail closed because they cannot identify the changed counterparty revision.
- **Completed:** added a bounded per-player shop-trade transaction adapter that submits the
  composite command, retains completions while a player is offline, and releases them on normal
  login or reconnect. A decoded completion publishes authoritative wallet/bank state first, then
  translates the shop result back into the exact item-transfer direction and revisions before
  updating the in-memory custody registry.
- **Fail-closed publication:** malformed results, balance publication failures, or custody-registry
  conflicts do not report a successful gameplay completion. One pending trade per player prevents
  locally queued commands from reusing the same optimistic wallet and owner revisions.
- **Checks passed:** the executable transaction regression covers produced system-to-player
  publication, destroyed player-to-destruction publication, counterparty revision mapping,
  duplicate submission refusal, offline retention, and reconnect release. It also verifies the
  main-loop completion hook and both player-ready hooks; command, item repository, composite
  repository, changed-line formatting, and the normal C++20 build pass.
- **Exposure:** this adapter deliberately does not move or destroy live objects itself. The shop
  command still performs its legacy synchronous mutation and does not submit a composite command;
  there is also no runtime shop-revision registry and no MariaDB shop-trade apply implementation.
- **Next action:** add a bounded selected-item snapshot/payload builder plus runtime shop revision
  hydration, then replace the flat-mode buy/sell mutation with submit-now and callback-after-commit
  behavior while retaining the MariaDB path until repository parity exists.

### Checkpoint 74 - revision-fenced live shop-trade payload capture

- **Completed:** added a non-mutating selected-object tree capture adapter that reuses the bounded,
  cycle-aware player item snapshot traversal without capturing unrelated inventory or equipment.
  The shop payload builder encodes that exact tree, derives UID parent topology, sorts custody
  fences canonically, and verifies every adopted UID/vnum/revision/owner relationship against the
  runtime ownership registry before producing a command payload.
- **Produced-stock fence:** a produced clone must be entirely absent from runtime custody, while
  its separate stock exemplar must be a top-level active item owned by the requested shop with the
  same vnum. Existing purchases require the selected tree to be shop-owned; both sale outcomes
  require it to be player-owned. Account identity, wallet/bank revisions, price bounds, snapshot
  byte limits, and the current shop aggregate revision are captured in the same immutable payload.
- **Shop revision continuity:** catalog restore atomically replaces the runtime shop-revision map
  only after every replacement shopkeeper is staged and placed. Custody-fenced dirty saves
  compare/advance the same map, and successful composite completions preflight and publish the
  exact next shop revision alongside money and item custody.
- **Checks passed:** the runtime regression covers canonical revision hydration, exact monotonic
  advance, stale refusal, duplicate-catalog rollback, and maximum-revision refusal, and its source
  contract verifies bounded capture, canonical sorting, custody/exemplar fences, final command
  validation, and absence of live mutation. Player capture, restore/save, transaction, changed-line
  formatting, and the normal C++20 build pass; CI includes the new regression.
- **Exposure:** the builder and completion publisher are not yet called from `shopping_buy` or
  `shopping_sell`. Produced purchase setup still needs safe clone lifetime/UID handling, and live
  callbacks must revalidate the selected object before moving or extracting it after commit.
- **Next action:** route flat-mode existing buys and both sale outcomes through the builder and
  transaction adapter first, with pointer-safe UID lookup in completion callbacks; then add
  produced-clone submission and multi-buy sequencing.

### Checkpoint 75 - post-commit flat shop buy/sell publication

- **Live cutover:** `shopping_buy` now routes non-produced, non-trusted flat-primary purchases
  through the composite command before any payment or inventory mutation. `shopping_sell` routes
  both retained and duplicate/trash outcomes before the legacy SQL, wallet, keeper-cash, or object
  mutation path. MariaDB and fallback-selected operation retain the legacy behavior.
- **Pointer-safe completion:** pending state retains only the immutable payload and player PID.
  Completion resolves the selected object and current keeper by UID/shop ID, verifies the exact
  object snapshot bytes and expected physical carrier, and only then moves the object from keeper
  to player, player to keeper, or player to extraction. Authoritative wallet/bank, custody, and shop
  revisions are published before this callback runs.
- **Concurrency and failure behavior:** one pending trade per player blocks repeated shop commands,
  while coordinator shop/player/item fences reject overlapping durable submissions. Insufficient
  flat wallet balance no longer invokes gem debt mutation. Semantic rejection leaves the object in
  place; a durable commit that cannot be reflected in the live object graph raises an explicit
  persistence alert instead of performing an unverified move.
- **Checks passed:** the focused live-route contract proves both flat branches submit before every
  legacy money/SQL/object mutation, verifies UID/shop lookup and carrier/snapshot revalidation, and
  proves authoritative runtime publication precedes the callback. Runtime/transaction regressions,
  changed-line formatting, and the normal C++20 build pass; CI runs the new route contract.
- **Exposure:** produced stock, trusted free purchases, gem-for-debt purchases, purchase-to-container,
  and recursive multi-buy deliberately fail closed in flat-primary mode. Legacy cleanup of invalid
  artifact/encrusted or nonpositive shop stock also remains outside the composite boundary, and
  MariaDB has no shop-trade repository parity yet.
- **Next action:** implement produced-clone lifetime and UID allocation through completion, then
  sequence container placement/multi-buy as independent committed operations. After those pass,
  move invalid-stock destruction through custody authority and add MariaDB composite parity.

### Checkpoint 76 - commit-fenced produced-stock purchases

- **Produced clone staging:** flat-primary produced purchases now allocate one fresh object and UID
  in `LOC_NOWHERE`, capture its exact bounded object tree, and submit it with the separately fenced
  shop-stock exemplar. The clone is neither charged nor placed in player inventory before the
  composite operation commits. Container placement and recursive multi-buy arguments fail closed
  before allocation rather than partially reproducing the legacy recursive mutation path.
- **Completion and cleanup:** a successful callback resolves the staged clone by UID, requires it
  to remain nowhere, and compares its current snapshot bytes with the committed payload before
  moving it to the player. Synchronous submission failure and ordinary semantic rejection extract
  an unchanged staged clone without artifact side effects. A durable result that cannot publish
  the exact live object raises the existing persistence alert and leaves the object untouched for
  recovery rather than guessing at custody.
- **Checks passed:** the live-route regression now proves the produced action is routed through the
  transaction adapter, clone allocation occurs inside the flat branch before submission, nowhere
  custody is revalidated, and pre-commit cleanup exists. Runtime and transaction regressions,
  changed-line formatting, and the normal C++20 build pass.
- **Exposure:** durable destination materialization is still incomplete. A crash after the shop
  operation commits but before its live callback/player save can leave the player's object snapshot
  inconsistent with authoritative item ownership; retained sales have the corresponding source
  reconciliation gap. Trusted and gem purchases still fail closed, invalid stock destruction
  remains outside the composite boundary, and MariaDB has no shop-trade repository parity. These
  gaps prohibit removing the flat-primary boot blocker.
- **Next action:** add a durable, restart-replayable object materialization/reconciliation boundary
  for both sides of committed shop transfers. Only after restart tests prove player snapshots and
  ownership converge should container/multi-buy sequencing, invalid-stock cleanup, and MariaDB
  parity proceed.

### Checkpoint 77 - restart-replayable shop object materialization

- **Atomic recovery evidence:** every successful flat shop operation now appends its exact bounded
  item snapshot to a checksummed materialization catalog in the same authority transaction as the
  operation result, shop aggregate, item custody, and player money. Interrupted commits therefore
  recover the materialization evidence together with the ownership change; semantic rejections do
  not append evidence.
- **Ownership-directed player load:** item ownership and shop materializations are read under one
  authority lock before the existing exact player-item reconciliation. Current ownership remains
  the final arbiter: stale sold/destroyed rows are removed, missing still-player-owned purchases are
  reconstructed from their latest committed inbound snapshot, and already-present snapshots retain
  newer item state. Parent topology is rebuilt and cycle/missing-parent/duplicate/limit conflicts
  fail closed before any load result is published.
- **Lifecycle integration:** character deletion rewrites or removes the player's materialization
  events in the same success-last deletion transaction. The character-delete disposition manifest
  now inventories this authority explicitly, including interrupted deletion recovery and failed
  deletion preservation.
- **Checks passed:** the shop repository fault regression covers an interrupted nested purchase,
  exact result replay, restart reconstruction, preservation of newer object fields, subsequent sale
  removal, and checksum-corruption refusal. Character deletion covers recovery, direct removal,
  idempotence, and failure rollback. Player/item/auction adjacency suites, delete-manifest contract,
  changed-line formatting, and the normal C++20 build pass.
- **Exposure:** the global materialization catalog is bounded but remains append-only between
  character deletions, so it still needs compaction and capacity observability. A committed trade
  whose live callback fails is repaired on the next load/reconnect rather than in the active
  session. Non-shop item-transfer paths may have equivalent snapshot convergence gaps, and trusted
  or gem purchases, container/multi-buy, invalid-stock destruction, and MariaDB parity remain open.
  The flat-primary boot blocker remains required.
- **Next action:** sequence container placement and multi-buy as independent committed shop
  operations, then add safe materialization compaction/health reporting and audit other item-transfer
  domains for the same player-snapshot crash window.

### Checkpoint 78 - durable produced-item destinations and sequential multi-buy

- **Destination-fenced command:** shop-trade payload version 3 records the produced item's final
  ownership root, optional parent container, and expected parent revision. The parent joins the
  operation's item keys and revision fences; version 2 and safe version 1 commands remain readable.
  Generic item creation transfers can now attach beneath an existing destination parent, with both
  the repository and runtime publisher verifying its active owner, root, and revision.
- **Crash-safe container custody:** flat-primary produced purchases validate a top-level carried
  container before submission and commit the new item directly beneath that container in the
  authoritative ownership graph. Live completion uses the normal `put` path only after ownership
  publication. If the process stops after commit, the existing shop materialization reconciliation
  reconstructs the exact purchased object beneath the committed container on the player's next load.
- **Bounded multi-buy:** produced `buy <item> <container> <amount>` requests accept at most 50 items
  and submit one complete composite shop operation per item. Each successful callback rechecks the
  keeper, exemplar, container, carry/capacity constraints, and rebuilds current wallet, shop, stock,
  and container revisions before submitting the next item. The sequence stops at the first failure,
  so no uncommitted remainder is charged or materialized.
- **Checks passed:** the shop repository fault/restart regression proves a produced clone commits
  below an existing player container and is reconstructed there from materialization evidence. The
  command codec covers version 3 parent fields plus version 2/1 compatibility; ownership, runtime,
  transaction, item/shop repository, shopkeeper, and live-route focused regressions pass together
  with changed-line formatting and the normal C++20 server build.
- **Exposure:** the in-memory multi-buy remainder is intentionally not resumed after a crash; only
  already committed individual purchases recover. Container open/capacity state is not part of the
  authority revision, so completion rechecks it and raises a persistence alert if live placement can
  no longer be reproduced. Trusted/gem purchases, invalid-stock destruction, MariaDB parity,
  materialization compaction/health, and non-shop snapshot-convergence audits remain open. The
  flat-primary boot blocker remains required.
- **Next action:** add safe materialization compaction and capacity health reporting, then move
  invalid shop-stock cleanup through custody authority and audit non-shop item-transfer domains for
  the same player-snapshot crash window.

### Checkpoint 79 - semantics-preserving materialization compaction and health

- **Recovery-equivalent compaction:** each successful shop append now compacts the catalog while
  the authority lock is held. For every player/item pair it retains the latest event that mentions
  the item and the latest inbound snapshot—the exact two facts load reconciliation consumes. A
  multi-item event is retained when any member still needs it. Existing reclaimable rows are removed
  before enforcing event capacity, and the new event is compacted before its after-image is encoded.
- **Atomicity:** the compacted checksummed catalog remains one after-image in the same success-last
  authority transaction as shop result, money, shop aggregate, and item custody. An injected stop
  after the first authority image leaves the journal intact; recovery publishes the compacted
  catalog with the operation and exact replay then reports `already_applied`.
- **Bounded operator health:** a lock-scoped health read reports catalog revision, current/hard-limit
  event and encoded-byte counts, reclaimable events, and an 80% near-capacity flag. Trusted
  `world persistence` renders this as metadata-only `shop_materialization` state, using `disabled`,
  `unavailable`, `ready`, or `degraded` without exposing player, item, path, or payload data.
- **Checks passed:** the repository regression covers empty health, a nested buy/sell/repurchase
  cycle that shrinks from three stored events to two despite appending a fourth, restart recovery
  across interrupted compaction, zero remaining reclaimable rows, and corrupt-catalog health
  refusal. Character deletion, player-load adjacency, persistence-status source contract,
  changed-line formatting, and the normal C++20 server build pass.
- **Exposure:** compaction deliberately retains the final mention and inbound evidence until
  character deletion, even after a player snapshot has incorporated it; distinct historical item
  UIDs can therefore still consume capacity. The on-demand health read scans the bounded catalog
  while holding the authority lock. The separate shop-operation catalog is still append-only.
  Invalid-stock destruction, MariaDB shop parity, trusted/gem purchases, and non-shop
  snapshot-convergence audits remain open, so the flat-primary boot blocker remains required.
- **Next action:** move invalid shop-stock cleanup through the custody authority, then audit and add
  materialization evidence for non-shop item-transfer domains with player-snapshot crash windows.

### Checkpoint 80 - atomic invalid shop-stock destruction

- **Composite cleanup action:** shop payload version 4 adds a zero-value `discard_invalid` action.
  It validates the exact bounded stock subtree and atomically removes it from the revisioned
  shopkeeper aggregate while transferring shop custody to the destruction owner. Player wallet and
  bank records are read/fenced but neither mutated nor revision-advanced, and no player
  materialization event is written. Version 3, version 2, and safe version 1 payloads remain readable.
- **Commit-fenced live extraction:** flat-primary buy, peruse, purchase lookup, and list paths route
  nonpositive, artifact, and encrusted stock through this action, including direct-name and numeric
  selection. Submission failure leaves the live object untouched. Completion resolves the keeper
  and item by stable IDs, revalidates exact snapshot and keeper custody, publishes authoritative
  shop/item revisions, and only then calls artifact-aware extraction.
- **Crash recovery:** an injected interruption after the first authority image preserves the
  success-last transaction journal. Recovery publishes the operation result, shop aggregate, and
  ownership after-images together; replay returns the exact cleanup result with unchanged money
  revisions and the removed subtree cannot reappear from shopkeeper restore.
- **Checks passed:** the composite repository regression covers nested stock resale, interrupted
  cleanup, exact replay, unchanged wallet/bank revisions, empty shop inventory, inactive destruction
  custody, and unchanged materialization revision. Command tests cover version 4/3/2/1 decoding and
  zero-price validation; transaction publication covers shop-to-destruction revision orientation;
  live-route, runtime, item/shop repositories, ownership refresh, changed-line formatting, and the
  normal C++20 server build pass.
- **Exposure:** cleanup is demand-driven by a player shop interaction rather than an eager boot or
  maintenance sweep, so untouched invalid stock can remain until inspected. A durable cleanup whose
  live callback cannot reproduce extraction raises the existing alert and converges on shop restore.
  Trusted/gem purchases and MariaDB shop parity remain open. Non-shop transfer snapshot convergence
  is still unaudited, so the flat-primary boot blocker remains required.
- **Next action:** audit player-facing non-shop item transfers for the same post-commit/pre-snapshot
  crash window, beginning with give/drop/get and locker/corpse boundaries, then add reusable
  materialization evidence where ownership alone cannot reconstruct exact object state.

### Checkpoint 81 - exact restart evidence for generic item transfers

- **Exact command evidence:** item-transfer payload version 4 appends a bounded encoded snapshot of
  the selected object tree. The live movement transaction captures and validates that snapshot
  before command construction, so give, drop, get, put, locker, corpse, and adoption/creation routes
  using the shared transaction carry the object fields needed after a post-commit process stop.
  Version 3 and version 2 commands remain readable with no fabricated snapshot evidence.
- **Ownership-coupled recovery:** a successful flat item transfer converts player-facing directions
  into internal inbound/outbound materialization events and publishes their compacted catalog
  after-image in the same authority transaction as the ownership catalog. Player-to-player moves
  record both sides, player-to-non-player moves record removal evidence, and non-player-to-player
  moves record reconstruction evidence. Same-player reparenting records one inbound event and uses
  the authoritative ownership graph to normalize the final root and parent.
- **Crash recovery:** the repository regression injects an interruption after ownership is the first
  authority image. Replay recovers the pending materialization image before reporting
  `already_applied`; reconciliation then removes an exact stale source tree and reconstructs the
  missing nested destination tree with its encoded descriptions and parent topology. The version
  regression covers a nonempty version 4 blob plus version 3/2 compatibility, and the live source
  contract requires capture and encoding before command submission.
- **Checks passed:** the item command compatibility, item repository fault/restart, live movement,
  get-all, locker cutover, shop repository, player repository, and ownership-runtime focused tests
  pass. The shared catalog remains compatible with shop buy/sell recovery, changed-line formatting
  passes, and the normal strict C++20 server build completes.
- **Exposure:** historical version 3/2 replays and synthetic version 4 callers that omit a blob still
  publish ownership without exact materialization evidence. Non-player room, locker, corpse, auction,
  and shop aggregates are not reconstructed by this player catalog; their own durable authority and
  restart coupling still require separate audits. A committed move whose active-session callback
  cannot publish is repaired on the player's next load/reconnect. The shared compacted catalog still
  retains distinct historical item UIDs until deletion, and the flat-primary boot blocker remains
  required.
- **Next action:** audit corpse, room, locker, and auction aggregate publication against the generic
  ownership transaction, then close trusted/gem purchase and MariaDB shop parity gaps without
  weakening exact replay or the boot blocker.

### Checkpoint 82 - fail-closed non-player aggregate transfer boundary

- **Audit result:** the locker and corpse/world catalogs currently support exact establishment and
  prepared character deletion, but their live save/load routes remain SQL-only. Room objects have no
  flat restart materializer, while auction and shop custody are owned by their separate composite
  commands. A generic ownership commit involving any of those domains could therefore remove an
  exact player object without durably publishing the non-player aggregate that must restore it.
- **Fail-closed boundary:** new version 4 generic transfers now commit only when both owner types are
  within the self-contained player/system/destruction set. Container, room, corpse, locker, auction,
  and shopkeeper owner transitions return durable `EOPNOTSUPP` results before ownership or
  materialization changes. Their eventual flat routes must use an aggregate-specific composite
  repository that joins every after-image under the authority transaction rather than weakening
  this guard.
- **Replay behavior:** the unsupported result is recorded in the ownership operation catalog, so an
  identical retry returns the same error without mutation and operation-ID reuse remains protected.
  Commands already present in the catalog still replay their recorded result first. Version 3/2
  compatibility remains available for historical journal recovery; those formats predate the exact
  aggregate fence and retain their documented ownership-only exposure.
- **Checks passed:** the fault-injected item repository regression now also attempts an exact nested
  player-to-room move, observes `EOPNOTSUPP` twice, and proves the player's owner revision and full
  custody remain unchanged. Changed-line formatting and the strict normal C++20 build pass alongside
  the CP81 restart and compatibility suites.
- **Exposure:** this is an integrity fence, not live flat support for drop/get, locker, corpse, auction,
  or shopkeeper transfers. Those routes remain unavailable until their aggregate after-images and
  restart materializers are composed and tested. Historical version 3/2 commands can still lack
  exact evidence, and the flat-primary boot blocker remains required.
- **Next action:** design held-lock mutation preparers for corpse and locker aggregates, including
  creation/removal metadata and nested exact snapshots, then compose each with ownership and player
  materialization in one operation-specific flat repository.

### Checkpoint 83 - atomic locker deposit and withdrawal aggregates

- **Held-lock locker mutation:** the locker repository now prepares exact deposit and withdrawal
  after-images from the version 4 item snapshot. It resolves the stable `{locker ID, chest ID}` owner,
  accepts only correctly oriented `locker_deposit`/`locker_withdraw` player transitions, rejects
  target-parent ambiguity, and advances catalog, locker, and chest revisions together. Deposits append
  a normalized nested root without disturbing existing chest topology; withdrawals extract the exact
  encoded subtree and rebuild all remaining parent indexes.
- **Cross-authority reconciliation:** before publication, the item repository compares every active
  UID/vnum owned by the locker chest with the preparer's complete pre-mutation chest proof. Missing,
  extra, or mismatched ownership fails closed. Successful commands publish ownership first, then the
  locker aggregate, then compacted player materialization evidence in one recoverable authority
  transaction. The CP82 fence now admits only this narrowly typed player/locker composite while room,
  corpse, auction, shopkeeper, and arbitrary locker transitions remain rejected.
- **Crash and replay behavior:** the item repository regression begins with an unrelated nested locker
  tree, injects interruption after the ownership image of a nested player deposit, and verifies replay
  recovers the exact locker descriptions/topology before returning `already_applied`. A subsequent
  withdrawal removes only the transferred subtree, preserves the unrelated nested tree and unchanged
  item revisions, advances locker/chest revisions once per command, and restores player custody with
  incremented item revisions.
- **Checks passed:** the item, locker, auction, player, shop, and character-deletion repository
  harnesses pass with the new dependency; the live locker ownership contract proves the held-lock
  preparer is ordered before both its after-image and authority commit. Changed-line formatting and
  the strict normal C++20 server build pass.
- **Exposure:** the composite repository is ready for already-established flat locker/chest
  authorities, but general locker creation, access, session, asynchronous save, and load paths remain
  SQL-only and keep the flat-primary boot fence in place. Same-chest synchronous rearrangement is not
  revisioned by this path, and cross-chest moves still require two explicit operations or a future
  dual-chest command. Historical version 3/2 transfers retain their ownership-only exposure.
- **Next action:** define a corpse operation payload carrying creation/removal metadata in addition to
  the exact item subtree, then compose corpse aggregate, ownership, player materialization, and
  artifact disposition under one recoverable transaction before admitting corpse owner transitions.

### Checkpoint 84 - atomic established-corpse loot

- **Held-lock corpse mutation:** the world-item repository now prepares exact loot after-images for an
  already-established corpse. It resolves the stable `{owner PID, save ID}` identity, requires a
  correctly oriented version 4 `corpse_loot` command with no ambiguous context or target parent,
  proves the encoded selected subtree byte-for-byte, removes it, repairs every remaining parent
  index, and advances both world-item catalog and corpse revisions. The shared snapshot codec now
  owns subtree extraction, so locker and corpse mutations use the same topology checks.
- **Cross-authority reconciliation:** before publication, the item repository compares every active
  UID/vnum owned by the corpse with the preparer's complete pre-mutation corpse proof. It also reads
  artifact authority under the same lock and fails closed if the selected subtree contains any
  registered artifact. Successful ordinary loot publishes ownership, the corpse aggregate, and
  compacted player materialization evidence in one recoverable authority transaction. The generic
  fence admits only this corpse-to-player operation; corpse creation and arbitrary corpse transitions
  remain rejected.
- **Crash and replay behavior:** the repository regression starts from a corpse containing multiple
  nested trees, injects interruption after the ownership image while looting one tree, and verifies
  replay publishes the exact corpse and player after-images before returning `already_applied`.
  Unrelated corpse contents keep their fields and repaired topology, while the destination player
  reconstructs the selected nested tree with incremented item revisions. A registered artifact loot
  attempt returns durable `EOPNOTSUPP` without changing either custody authority.
- **Checks passed:** the item, artifact, world-item, snapshot-codec, locker, auction, player, shop,
  character-deletion, ownership, locker-cutover, and live movement focused tests pass. Changed-line
  formatting, the strict normal C++20 server build, and the client-free flat-file boot preflight also
  pass.
- **Exposure:** corpse creation still lacks the metadata needed to establish a complete flat corpse,
  and artifact loot lacks the player/corpse race context required to reproduce binding, feed, and
  disposition semantics safely. Empty established corpse records are retained because corpse removal
  is a separate lifecycle action. General live corpse save/load remains SQL-only, historical version
  3/2 transfers retain their ownership-only exposure, and the flat-primary boot blocker remains
  required.
- **Next action:** introduce a corpse-specific command or item-transfer payload version carrying
  creation/removal metadata and race context, then compose corpse establishment/removal and artifact
  disposition under the same authority transaction. Room aggregate transfers remain the next
  uncoupled world-item boundary after corpse lifecycle coverage.

### Checkpoint 85 - atomic corpse creation and artifact transfer context

- **Versioned corpse context:** item-transfer payload version 5 appends a bounded, versioned corpse
  context containing the stable room, post-move weight, all eight corpse values, canonical identity
  strings, and the acting player's racewar side. The codec validates the encoded corpse PID/save ID
  against the typed owner, bounds every string, preserves exact version 4/3/2 decoding, and rebuilds
  historical command entity keys without requiring current-version re-encoding. Live movement
  captures the context from a verified PC corpse immediately before submission and re-resolves only
  its UID across the adoption retry boundary.
- **Corpse aggregate composition:** the held-lock world-item preparer now handles both directions.
  The first durable non-money death item establishes the corpse metadata and its exact nested subtree;
  later durable death items append their trees with repaired parent indexes. Loot still proves and removes
  the exact encoded subtree. Existing records must match immutable owner name, PID, save ID, and
  racewar identity before mutable metadata is accepted. The ownership repository reconciles the
  preparer's complete pre-mutation custody proof, including the absent/revision-zero case for first
  establishment, before publishing ownership and world-item images together.
- **Artifact disposition:** the artifact repository decodes the same exact subtree and fails closed
  when an item marked as an artifact is missing from the artifact catalog. Death moves every selected
  registered artifact from player to corpse custody and clears binding state. Loot moves it from the
  corpse owner PID to the looter; cross-race loot clears the bound owner, records the accepted command
  time as the bind timer, and feeds the artifact to at least five days. Artifact catalog/revision,
  corpse aggregate, item custody, and player materialization after-images share one recoverable
  transaction. Historical version 4 artifact loot remains a deterministic durable `EOPNOTSUPP`.
- **Crash and replay behavior:** fault-injected repository cases cover interruption during first
  artifact-bearing corpse establishment, subsequent nested corpse appends, and cross-race artifact
  loot. Replay completes every missing authority image exactly once, preserves unrelated nested
  contents and artifact timers, and returns `already_applied` only after ownership, world, artifact,
  and player materialization agree. Live death staging now starts a PC corpse at body weight while
  items remain on the player, preventing each acknowledged move from double-counting inventory
  weight in both SQL and flat corpse snapshots.
- **Checks passed:** version compatibility, live movement/ownership, item, world-item, artifact,
  locker, auction, shop-trade, player, character-deletion, get-all, account-reward, and snapshot
  focused suites pass. Changed-line formatting, `git diff --check`, the strict normal C++20 server
  build, the guarded local MariaDB item-transfer schema/replay harness, and the isolated client-free
  flat-file boot preflight also pass.
- **Exposure:** version 5 establishes only PC corpses containing at least one durable non-money item.
  Legacy money objects moved into an otherwise established corpse are not yet represented by the flat
  aggregate. Money-only and empty corpses, metadata-only saves, relocation, explicit removal, and live
  flat-file corpse restore remain SQL-only and keep the flat-primary boot blocker in place. Empty
  established corpse records remain after the last item is looted because deletion is a distinct
  lifecycle event; historical version 3/2 commands retain their ownership-only exposure.
- **Next action:** add a dedicated idempotent corpse-lifecycle command and operation ledger for empty
  establishment, money contents, metadata updates, relocation, and deletion, then route live flat
  corpse load/restore through the completed authority. After that boundary, implement atomic room
  aggregate transfers.

### Checkpoint 86 - revisioned corpse lifecycle and money aggregates

- **Bounded lifecycle protocol:** a dedicated corpse-lifecycle critical command now carries an
  `upsert` or `remove` action, stable owner PID/save ID identity, expected corpse revision, room and
  weight, all eight corpse values, four money denominations, and bounded identity strings. Commands
  expose one canonical corpse entity key and one compare-and-swap revision; successful completions
  return the new corpse and world-catalog revisions in a fixed-size result.
- **Exactly-once flat authority:** the corpse repository maintains a separate checksummed operation
  catalog keyed by operation ID and the complete command digest. Under the shared authority lock it
  composes the world-item after-image with the operation result in one recoverable transaction.
  Exact retries return the recorded result, operation-ID reuse with different bytes is rejected, and
  stale, absent, or non-empty removal failures are themselves durable and deterministic.
- **Lifecycle semantics:** revision zero establishes an empty or money-only corpse; later matching
  revisions update money, mutable metadata, weight, and room placement while retaining every durable
  item subtree. Owner name, PID, save ID, and racewar identity remain immutable. Removal requires an
  exact revision and refuses to discard either durable items or any nonzero money denomination, so
  only an empty aggregate can disappear at this boundary.
- **Catalog compatibility:** world-item catalog version 2 stores the four money denominations beside
  corpse metadata. Version 1 catalogs remain readable and materialize a zero money aggregate, while
  all new writes use the checksummed version 2 form. Focused coverage rewrites a real version 2
  fixture into the historical byte layout and verifies both nested-item preservation and the zeroed
  legacy money state.
- **Checks passed:** the bounded command codec, corpse lifecycle/recovery, world-item compatibility,
  item ownership, auction, shop-trade, player, character-deletion, and critical-command coordinator
  suites pass. Fault injection covers interruption between the world and operation-ledger images;
  replay completes both exactly once. Changed-line formatting, `git diff --check`, the strict normal
  C++20 server build, and the isolated client-free flat-file boot preflight also pass.
- **Exposure:** this checkpoint supplies the durable command and repository primitive but does not
  yet route live `writeCorpse` calls or boot restoration through it, so those paths remain SQL-only
  and the flat-primary boot blocker remains. Non-empty corpse decay or extraction cannot be admitted
  until corpse contents and money can move into a revisioned room aggregate in the same authority
  transaction; those removals continue to fail closed.
- **Next action:** route live corpse creation/save/relocation/removal through lifecycle command
  submission with tracked revisions and completion handling, then restore version 2 corpse metadata,
  money, and durable contents during flat load. Follow that with the room aggregate needed for atomic
  non-empty decay and extraction.

### Checkpoint 87 - live corpse lifecycle and authoritative restart restore

- **Revision-aware live routing:** flat-primary `writeCorpse` now captures room or carrier-room
  placement, weight, values, identity strings, and recursively aggregated money, then stages a
  corpse-lifecycle command instead of calling SQL. The bounded runtime tracks each stable PID/save-ID
  corpse revision, coalesces repeated saves, waits behind item-transfer entity fences, and defers
  removal until the next pulse so an ordinary `obj_from_room`/`obj_to_room` relocation becomes one
  upsert rather than a transient durable delete. MariaDB modes retain their existing SQL routes.
- **Cross-command revision handoff:** item-transfer results now include the corpse aggregate revision
  produced by the atomic world mutation. The result codec accepts historical 40-byte results and
  writes the new 48-byte form. Item ownership catalog version 2 persists that completion field while
  retaining version 1 decoding. Death and loot completion callbacks publish the revision before
  their follow-up corpse save, preventing stale lifecycle compare-and-swap submissions.
- **Authoritative boot restore:** flat boot lists the version 2 world catalog, reconciles every corpse
  snapshot UID/vnum/root/parent against the item ownership authority, and refuses missing or
  contradictory custody. A detached item materializer restores exact nested object state, including
  dynamic object affects, without attaching it to a character. Restore rebuilds corpse metadata,
  identity, money, durable contents, placement, decay protection, ownership revisions, and lifecycle
  revisions; unknown rooms/prototypes, malformed topology, or publication failures stop boot.
- **Transport/storage normalization:** live item snapshots use inventory slot zero while corpse
  authority stores detached slot `-1`. The world preparer now canonicalizes both forms before
  creation or loot comparison, so real live captures round-trip while existing durable catalogs stay
  canonical.
- **Checks passed:** lifecycle transaction/coalescing, corpse command/repository, cross-authority
  ownership reconciliation, restore publication, live source routing, item result/catalog backward
  compatibility, world-item transport normalization, nested materialization, dynamic affects,
  critical-command ordering/replay, and existing corpse/item movement contracts. Changed-line
  formatting, `git diff --check`, the strict normal C++20 server build, and the isolated client-free
  build/boot preflight pass.
- **Exposure:** non-empty corpse decay or destructive extraction still refuses durable removal because
  no revisioned room aggregate can receive its items and money atomically. The retained corpse
  authority survives for restart recovery instead of discarding custody. Remaining unimplemented
  domains continue to keep the flat-primary boot blocker in place.
- **Next action:** implement the room aggregate and compose corpse-to-room item/money transfer with
  corpse removal for non-empty decay/extraction, then route saved floor-item live save and restore
  through that same world authority.

### Checkpoint 88 - atomic corpse-to-room release authority

- **Bounded release protocol:** corpse-lifecycle payload version 2 adds a `release` action and the
  expected destination-room revision. Release commands fence canonical corpse and room entity keys,
  while payload/result decoders retain version 1 and 32-byte result compatibility. Successful
  results expose the world-catalog, corpse-owner, room-owner, maximum item, and item-count revisions
  needed for deterministic completion and replay.
- **Revisioned room aggregate:** world-item catalog version 3 adds sorted room records containing a
  room VNUM, revision, four money denominations, and exact detached item snapshots. Version 2 money
  catalogs and version 1 catalogs remain readable. A held-lock release proves the corpse identity,
  revision, placement, and destination revision; preserves nested topology while appending to an
  existing room; accumulates money with overflow checks; and removes the corpse only in its prepared
  after-image.
- **Cross-authority composition:** item ownership proves every active corpse UID, VNUM, root, and
  parent against the world aggregate, advances both owner revisions, and moves the complete custody
  set to the room. Registered artifacts move from corpse custody to `ON_GROUND` in the same room at
  the command's accepted time. Ownership, world aggregate, optional artifact state, and the corpse
  operation ledger publish in one recoverable authority transaction; semantic stale/not-found
  failures and operation-ID reuse retain the existing exactly-once behavior.
- **Crash and compatibility coverage:** the focused repository regression interrupts publication
  after the ownership and world images, then verifies recovery removes the corpse, preserves exact
  nested room snapshots and money, transfers ownership revisions, grounds the artifact, and returns
  `already_applied`. It also covers a money-only corpse appended to an existing room and exact replay
  failures. Command coverage exercises release keys/results plus legacy payload/result decoding, and
  the world catalog fixture proves version 1 compatibility after the version 3 extension.
- **Checks passed:** changed-line formatting, `git diff --check`, the strict normal C++20 server
  build, and the focused command, world-item, artifact, item-ownership, corpse repository, lifecycle
  transaction, corpse ownership/restore, character deletion, world recovery, live routing contract,
  and retired SQL-corpse cleanup tests.
- **Exposure:** no live game path submits `release` yet, and room aggregates are not restored into the
  live world at boot. Non-empty decay/extraction therefore remains fail-closed rather than using this
  primitive. General player drop/get and saved floor-item persistence are still fenced because their
  corresponding room mutation and restart materialization have not been connected.
- **Next action:** extend the live lifecycle transaction with a non-coalescing release completion,
  route corpse decay/destructive extraction through it so live movement occurs only after durable
  success, and restore version 3 room aggregates at boot. Then connect historical saved floor-item
  save/restore to the same room authority.

### Checkpoint 89 - authoritative room aggregate boot restore

- **Exact cross-authority restore:** the existing corpse ownership reconciler now exposes its narrow
  world-item core so room records can prove every snapshot UID, VNUM, root, parent, and revision
  against active item custody. Money-only rooms still require their persisted room-owner revision;
  missing or contradictory ownership fails boot closed.
- **Whole-catalog staging:** flat boot reads version 3 room aggregates, validates their room and
  money fields, detached-materializes exact nested object graphs, and stages their currency. Every
  corpse and room aggregate is materialized before any live publication begins, and a failed item
  placement removes already-published staged objects rather than leaving a partial restored world.
- **Side-effect-safe publication:** restored room roots and all four money denominations publish only
  after fallible staging succeeds. Corpse-save and artifact-location callbacks are suppressed during
  restore so the durable authorities being loaded are not redundantly rewritten, then their prior
  runtime settings are restored.
- **Checks passed:** focused room/corpse ownership reconciliation and boot materialization tests,
  world-item and corpse repositories, immutable world-recovery contracts, live routing source
  contracts, changed-line formatting, and the strict normal C++20 server build.
- **Exposure:** live decay and destructive extraction do not yet submit the atomic release command,
  so gameplay cannot create these room aggregates through the normal corpse lifecycle. General
  player drop/get and historical saved floor-item persistence also remain unconnected.
- **Next action:** extend the lifecycle transaction with release submission/completion, route live
  corpse decay through it, and mutate the live object graph only after durable success. Audit the
  remaining destructive extraction callers separately before broadening that route.

### Checkpoint 90 - durable live corpse decay publication

- **Non-coalescing release completion:** the existing lifecycle tracker now admits a release only
  when the corpse has a committed revision and neither the corpse nor destination room is fenced.
  It captures both revisions, rejects competing lifecycle stages while release is in flight, calls
  its game-thread completion exactly once, and permits a new attempt after an `ESTALE` room race.
- **Durability-before-mutation:** flat-primary decay for ground, carried, and worn player corpses
  returns before emitting messages or touching the object graph. On committed release, publication
  finds the stable PID/save-ID corpse, verifies every live durable UID/VNUM/root/parent against its
  runtime custody, advances the complete custody set from corpse to room, then moves contents and
  money and extracts the corpse with corpse-save and artifact callbacks suppressed.
- **In-flight topology protection:** corpse get and put operations refuse briefly while lifecycle
  work is pending, covering direct coin movement as well as ownership-backed items. Submission
  failure rearms the decay event, a stale durable comparison rearms through the completion callback,
  and permanent integrity failures preserve the live corpse for operator recovery.
- **Runtime reconciliation:** the ownership cache applies a release atomically only when prior owner
  revisions, item count, and maximum post-transfer item revision match the durable result. It covers
  nested items and money-only corpses without inventing another movement subsystem.
- **Checks passed:** lifecycle completion/retry, runtime nested and money-only custody publication,
  command/repository, artifact, corpse ownership/restore, world-item, item-ownership, and live source
  contracts; changed-line formatting, `git diff --check`, and the strict normal C++20 server build.
- **Exposure:** generic `extract_obj` still cannot become asynchronous without breaking its many
  synchronous callers. Explicit destructive player-corpse callers require a bounded audit, and a
  corpse nested inside another object remains fail-closed because release currently targets rooms.
  General saved floor-item persistence is also still unconnected.
- **Next action:** classify and route explicit destructive corpse-extraction entry points without
  altering generic extraction semantics, then connect historical saved floor-item save/restore to
  the revisioned room aggregate.

### Checkpoint 91 - audited destructive corpse release routing

- **Narrow deferred contract:** `persistence_defer_corpse_room_release` is the single public guard
  for explicit callers whose historical result is an empty corpse removed after its contents are
  dropped in the same room. In flat-primary mode it recognizes an already-busy lifecycle as handled,
  stages the existing atomic release when possible, and retains/rearms the corpse with an operator
  alert when staging fails. It returns false for food, NPC corpses, and every MariaDB mode so their
  established synchronous behavior is unchanged.
- **Release-compatible callers:** the generic mobile devour proc, both Verzanan dog procs, Lightning
  Sword, flying daggers, and ochre jelly now consult the guard before their first content movement.
  A handled flat-primary corpse returns from the special without logging, messaging, moving a child,
  or extracting the root; the lifecycle completion performs the already-validated room publication
  only after durable success. Ordinary decay uses the same guard instead of duplicating submission
  and failure handling.
- **Bounded audit result:** generic `extract_obj` remains synchronous because recursive container
  teardown and numerous non-corpse callers depend on that contract. Resurrection and necromancy
  transfer contents to a player or created follower, unmaking and wall-of-bones couple corpse
  consumption to gameplay effects, and `very_angry_npc` intentionally destroys contained gear.
  None of those operations can be represented honestly by corpse-to-room release, so this checkpoint
  does not invent a generalized extraction transaction for them.
- **Checks passed:** the live source contract proves every routed special checks the deferred release
  before its first `obj_from_obj`, changed-line formatting and `git diff --check` pass, and the strict
  normal C++20 server build compiles and links successfully.
- **Exposure:** routed specials publish the existing durable decay messages after completion rather
  than their historical proc-specific flavor text. Immediate post-death release can also encounter
  the corpse-establishment fence and therefore retains the corpse for a later lifecycle attempt.
  Player/follower transfers, coupled spell effects, intentional contained-gear destruction, and
  nested-corpse extraction still need operation-specific durability sequencing before flat-primary
  mode can execute them safely.
- **Next action:** restore historical saved floor-item capture and restart materialization through the
  revisioned room aggregate, then return to the audited non-room corpse operations with the ownership
  destinations required by those concrete gameplay paths.

### Checkpoint 92 - atomic live room item transfers

- **Historical behavior restored:** the existing deferred player `drop`, floor/container `get`, and
  cross-owner `put` paths can now persist durable non-money items in flat-primary rooms instead of
  receiving the ownership repository's `EOPNOTSUPP` fence. MariaDB routing and same-owner inventory
  reparenting remain unchanged.
- **One recoverable publication:** the item repository composes ownership, the version 3 world-room
  aggregate, player materialization events, and any artifact catalog change under the existing
  authority lock. The room preparer compares the complete pre-move UID/VNUM/root/parent custody,
  advances the same room revision as ownership, canonicalizes live slot-zero snapshots to detached
  slot `-1`, and records the exact moved subtree before the completion mutates live pointers.
- **Nested container fidelity:** puts attach the transported root to the proven room-owned parent;
  gets remove only the selected subtree. Both directions reproduce the live handler's positive-weight
  propagation through all room-owned ancestors with checked arithmetic, preventing a restored floor
  container from carrying stale weight after content movement.
- **Artifact custody:** registered artifacts move atomically between `ON_PLAYER` and `ON_GROUND` at
  the command acceptance time while their timer and binding fields remain intact. The established
  live artifact callback can still perform its normal post-publication soul semantics.
- **Crash and regression coverage:** a focused isolated fixture interrupts a player-to-room drop
  between authority images, recovers it exactly once, then exercises nested put/get and confirms room
  topology, owner revisions, detached snapshots, and ancestor weights. Artifact coverage proves both
  room directions and binding preservation; the live source contract requires both room preparers and
  their after-images before the shared commit.
- **Checks passed:** focused item-ownership, world-item, artifact/runtime, and live movement suites;
  changed-line formatting, `git diff --check`, the strict normal C++20 server build, and the isolated
  client-free game-loop boot/clean-shutdown preflight.
- **Exposure:** money objects still use their historical cash path rather than item custody. This
  checkpoint does not establish or delete an administrative saved-storage root, so the historical
  `writeSavedItem`/`PurgeSavedItemFile` adapter and explicit saved-item restore entry point remain.
- **Next action:** give administrative `ITEM_STORAGE` roots stable-UID, revisioned room
  establishment/removal and route the three historical saved-item entry points through it. Reuse the
  room transfer above for all subsequent player content changes rather than adding another catalog.

### Checkpoint 93 - restored administrative saved world items

- **Historical lifecycle restored:** flat-primary `storage new`, `storage delete`, and `storage
  remove` now submit the existing item movement transaction before touching the live object graph.
  Establishment adopts a stable-UID storage subtree from system custody into its room; deletion moves
  the exact subtree to destruction custody; removal serially detaches each child to the room floor and
  destroys the empty root. Failed submissions and completions retain the live authority rather than
  reporting a discarded mutation as successful.
- **Shared room authority:** the item and world repositories admit only those three bounded room
  transfer shapes in addition to the checkpoint 92 player movements. Establishment appends an exact
  detached subtree, deletion removes that exact subtree, and same-room child detachment preserves
  nested topology while subtracting weight through every former ancestor. The adoption runtime now
  completes a same-owner system-to-room establishment without manufacturing a second no-op command.
- **Artifact and cleanup semantics:** an artifact-bearing establishment fails closed, same-room
  detachment leaves ground artifact custody unchanged, and deletion clears every selected registered
  artifact's ownership, location, and binding state in the same recoverable authority transaction.
  Recursive live cleanup recognizes already-destroyed item custody, avoiding false mutation alerts.
- **Historical entry-point routing:** `writeSavedItem` validates that flat storage is already tracked
  by room authority and returns before SQL; `restoreSavedItems` relies on the shared room catalog that
  boot has already staged; and `PurgeSavedItemFile` verifies destruction custody and returns before
  SQL. MariaDB behavior stays synchronous, while saved-item deletion now occurs before the live object
  is freed, repairing the prior use-after-free and missing SQL-row purge.
- **Restart and regression coverage:** repository fixtures establish and delete a nested storage
  subtree and detach a nested child with exact revision, topology, and ancestor-weight assertions.
  Artifact coverage exercises same-room detachment and room destruction. The boot harness restores an
  `ITEM_STORAGE` root and nested container from the room catalog, and a focused source contract proves
  all three legacy saved-item entry points branch before SQL and all admin mutations wait for durable
  acknowledgment.
- **Checks passed:** saved-item routing, item/world-item repositories, artifact/runtime, room/corpse
  boot restore, live movement, item ownership/runtime, and transfer-version compatibility suites;
  changed-line formatting, `git diff --check`, the strict normal C++20 server build, and the isolated
  client-free game-loop boot/clean-shutdown preflight.
- **Exposure:** the unsafe pointer-named native `Players/SavedItems` format is not revived as a writer;
  authoritative saved world items now use the bounded shared room catalog. Money objects retain their
  historical cash path. The checkpoint 91 non-room corpse operations remain fail-closed until their
  actual player, follower, destruction, effect, or nested-container destinations are represented.
- **Next action:** implement the smallest operation-specific durability boundary from the checkpoint
  91 corpse audit, beginning with intentional corpse-and-contents destruction, then return to the
  player/follower and coupled-effect transfers without broadening generic `extract_obj` semantics.

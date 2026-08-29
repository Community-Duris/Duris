# Flat-file restoration project directive

**Effective date:** 2026-08-28  
**Status:** authoritative direction for all remaining flat-file work

## Progress ledger

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

## Owner intent

The purpose of this project is to restore the previous flat-file persistence system,
while making that restored system safer, more reliable, and better-quality where needed.
After restoration, the work may fill concrete functional gaps required for the flat-file
system to operate correctly.

The required end state is that the server can run fully in either of two independently
usable modes:

1. **Database-backed mode**, retaining the existing database-backed behavior.
2. **Flat-file-only mode**, requiring no database server, database client library,
   database connection, or database-backed persistence service at build time or runtime.

Some server systems were always database-backed and therefore have no historical
flat-file implementation to restore. Those systems are part of the required gap-filling
work: they must receive safe, focused flat-file persistence implementations sufficient
for the complete server to operate in flat-file-only mode.

The implementation already built on the `flatfiles` branch will be used as the starting
point. It is not an instruction to discard all current work and begin again. It is also
not a mandate to continue expanding the architecture that has been built.

## Scope authority

This directive supersedes conflicting scope, design, implementation-order, and "next
action" statements in
[`flatfile-persistence-assessment.md`](./flatfile-persistence-assessment.md) and other
ongoing project notes. Those documents may be used as implementation history and
technical evidence, but they do not authorize additional scope.

The historical flat-file implementation, including the comparison point identified in
the assessment (`97a4166c3fa10448b778a35e16854ad5b3e5e294`), is the behavioral reference.
Its behavior should be recovered deliberately rather than replaced with a newly imagined
persistence product. For a system that was historically database-only, its existing
database-backed behavior is the functional reference for the required flat-file
counterpart; this does not authorize unrelated redesign.

## Required remaining work

Remaining work must be limited to:

1. Reconstructing concrete behavior and coverage from the previous flat-file system.
2. Connecting the implementation already built to the actual server paths needed for
   that restored behavior.
3. Correcting safety, data-integrity, corruption-handling, bounds, error-handling, and
   code-quality problems that affect the restored behavior.
4. Filling specific, demonstrated gaps that prevent functional parity or correct
   operation.
5. Implementing focused flat-file counterparts for historically database-only systems
   that would otherwise prevent complete no-database operation.
6. Preserving working database-backed behavior and backend selection while adding the
   flat-file path.
7. Adding focused regression tests for restored or corrected behavior in both relevant
   modes.

Existing new code may be reused when it directly serves these requirements. It may be
simplified, corrected, or bypassed when that is the smallest safe way to restore the
required behavior.

## Explicitly out of scope

Do not undertake any of the following without the owner's explicit approval:

- redesigning the persistence system again;
- adding speculative architecture, generalized frameworks, or future-proofing;
- creating new requirements merely because the current architecture makes them
  possible;
- broad refactors that are not necessary for a specific restoration or safety gap;
- replacing known historical behavior with a theoretically cleaner product design;
- turning each remaining gap into a new subsystem, transaction framework, catalog, or
  multi-checkpoint project when a narrow repair is sufficient;
- continuing work solely because an earlier assessment lists it as a planned phase or
  next action.

## Working rule for every change

Before implementation, identify the exact historical behavior or concrete missing
server behavior being restored. For a historically database-only system, identify the
existing database behavior that the flat-file path must provide. Then make the smallest
safe change that provides it, preserve compatible code already built, and verify it with
the narrowest useful test.

If a proposed change cannot be tied to historical restoration, a demonstrated gap, or a
required safety correction, it is outside this project's scope. If the smallest safe
solution would materially expand the design, stop and obtain explicit owner approval
before proceeding.

## Completion standard

The project is complete when the required previous flat-file behavior is restored, every
database-only system needed for normal server operation has a working flat-file
counterpart, the server can operate fully in both database-backed and flat-file-only
modes, and the affected paths have focused validation. Flat-file-only operation must not
silently disable required gameplay or persistence behavior merely to boot without a
database. Completion does not require an idealized persistence platform, exhaustive
architectural abstraction, or implementation of speculative future capabilities.

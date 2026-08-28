# Flat-file persistence assessment

- **Assessment date:** 2026-08-28
- **Current revision inspected:** `68a916ec` (`flatfiles`, identical to `master` at the time of inspection)
- **Historical comparison point:** `97a4166c3fa10448b778a35e16854ad5b3e5e294`
- **Status:** implementation in progress; the assessment below is the authoritative scope

## Implementation progress

This section is the durable handoff ledger for the implementation. A checked item means
the implementation and the evidence named here exist in the current branch; the design
sections below continue to describe the required end state.

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

### Milestone status

| Milestone | State | Evidence |
|---|---|---|
| P0 - real DB-free boundary | Complete | Client-free binary links without system MySQL dependencies; isolated boot preflight and no-MySQL CI job exist |
| P1 - identity and player continuity | In progress | Account/identity continuity, baseline creation, revisioned save, terminal fences, and identity-verified snapshot load exist; external sidecars still fence login |
| P2 - transactional gameplay and domains | In progress | Durable UID allocation plus revisioned item ownership/transfer replay exist; load reconciliation, non-item operations, and remaining domains remain |
| P3 - production operations | Not started | No exporter, whole-authority backup, or restore drill yet |

## Executive conclusion

The current server is **not capable of production operation with MySQL/MariaDB removed**.

- The normal build requires the MySQL client library, and normal boot treats database
  initialization or schema verification failure as fatal.
- The client-free `__NO_MYSQL__` build now compiles and links, but explicit boot
  preflight rejects it because player external-domain sidecars, safe rename/delete
  completion, and many world subsystems remain unimplemented.
- The current player and critical-command journals are durable handoff queues whose
  consumer is MySQL. They are not a searchable flat-file database, do not contain all
  durable domains, and cannot reconstruct all acknowledged state after database loss.
- The remaining pfile reader and writer code is compatibility code, not a complete
  fallback. Flat account records and identity lookups now exist, but positive-PID pfiles
  still have their objects skipped by the current item loader and are not an authoritative
  player snapshot store.

The last revision before the player-pfile-to-database project began did have working
flat-file account and character storage and a much wider collection of file-backed
domains. It was still **not a database-free server**: MySQL initialization was already a
default boot requirement, and alliances, outposts, nexus stones, ship cargo, parts of
locker state, artifact state, and other features already depended on SQL.

Therefore, restoring the old pfile writer wholesale would not produce a safe or complete
production persistence system. The viable direction is a first-class flat-file backend
that can serve either as a complete fallback for MariaDB or, when explicitly selected,
as the primary and only persistence path as though MariaDB had been removed entirely.
Both uses require the same full save/load domain coverage, repository interfaces,
typed/versioned snapshots, atomic publication, stable identifiers, operation journals
for transactional gameplay, and tested migration and backup paths. The old readers are
valuable as import/salvage tools and the current snapshot/journal codecs are valuable
implementation material, but neither is sufficient by itself.

## Scope and terminology

For this assessment, "the DB got ripped out" means that no usable MySQL/MariaDB server,
schema, or client-backed persistence path is available. Redis is considered separately:
it is not authoritative for player saves in the current design and cannot replace MySQL.
If "DB" also includes Redis, the conclusion is unchanged and world-recovery facilities
become unavailable as well.

"Fallback" means a complete, current flat-file authority that can be selected for the
whole server when MariaDB is unavailable. It does not mean writing a pfile only after an
individual SQL save fails, nor mixing SQL and flat-file authority within one gameplay
operation. Switching authority must be explicit, observable, and recoverable. The same
flat backend must also support a configured flatfile-primary mode in which every durable
read and write works without MariaDB being installed or reachable.

"Before the DB system got started" is necessarily scoped to the recent **player pfile to
SQL migration project**, not to the first SQL use in Duris. The exact boundary used here
is:

- `97a4166c` - last commit before the player migration project.
- `35f66dfc` - its direct child, titled `phase 1 pfile-to-db: schema + sql_player basics`.

SQL had existed for years before that boundary. Calling `97a4166c` a universally
flat-file or DB-free version would be inaccurate.

The investigation was source- and history-based. It did not start the game, connect to a
database, inspect production data, run migrations, or execute operational backup scripts.
The historical tree was inspected read-only through Git and a temporary detached export.

## Bottom-line behavior if MySQL disappears today

| Attempt | Observed result | Why |
|---|---|---|
| Start the normal binary without MySQL | Boot aborts | `comm.c` treats `initialize_mysql() < 0` as fatal; initialization also validates the exact schema and starts the SQL pool. |
| Rebuild with `-D__NO_MYSQL__` | Build fails | MySQL types and SQL transaction calls remain exposed outside effective guards, while the Makefile still adds MySQL include and link flags. |
| Mechanically fix the no-MySQL build only | Server remains non-viable | Account reads, existence checks, player repositories, item loads, and many domain repositories return failure, empty data, or no-op success. |
| Keep an already-running process alive after DB loss | Some snapshots may reach local journals, but durable gameplay progressively fences/fails | Journal workers still apply to SQL, player reload still comes from SQL, and critical commands require SQL transactions. |
| Reboot using old pfiles left on disk | Not a valid recovery path | Those files are no longer current authority; account routing is SQL-only, current item loading skips positive-PID pfiles, and many domains were never fully file-backed. |

### Default boot is deliberately database-fatal

The normal build leaves `__NO_MYSQL__` commented out, adds
`/usr/include/mysql`, and links `-lmysqlclient` in
[`src/Makefile`](../../src/Makefile). During boot,
[`src/comm.c`](../../src/comm.c) loads the environment, initializes MySQL, and calls
`fatal_boot_error` on failure. [`src/sql.c`](../../src/sql.c) does more than open a
socket: it validates the configured target and runtime schema, populates lookup state,
hydrates/reserves item identity state, and starts the connection pool.

This is consistent with the current architecture documentation, which explicitly states
that all durable state is in MySQL/MariaDB; see
[`docs/reference/DATABASE.md`](../reference/DATABASE.md).

### The no-MySQL target has decayed beyond a feature flag

An isolated build was attempted with:

```text
make -C src \
  BIN_ROOT=/tmp/duris-nomysql-build \
  OBJDIR=/tmp/duris-nomysql-build/objects/server \
  SERVER_BIN_DIR=/tmp/duris-nomysql-build/server \
  DMS_BINARY=/tmp/duris-nomysql-build/server/dms_new \
  EXTRA_CFLAGS=-D__NO_MYSQL__ -j2
```

It exited with status 2. Representative first-order blockers were:

- [`src/sql.h`](../../src/sql.h) declares functions using `MYSQL *` even when the
  header that defines `MYSQL` is excluded.
- [`src/ships/ship_base.c`](../../src/ships/ship_base.c) calls SQL transaction helpers
  whose declarations are absent in the no-MySQL branch.
- The Makefile still includes the MySQL header directory and links the client library.
- SQL-backed implementation units are still compiled unconditionally.

These are only the compile boundary. The no-MySQL branches in
[`src/sql_player.c`](../../src/sql_player.c) return failure or empty results for player,
account, locker, corpse, saved-item, shopkeeper, town, siege, ship, guild, pet, and other
state. A green build alone would not restore behavior.

### Login and player loading have no working flat route

The current account path in [`src/account.c`](../../src/account.c) is SQL-only:

- `read_account()` returns failure under `__NO_MYSQL__`.
- `write_account()` omits the durable write in that build.
- `account_exists()` can still notice an old `Accounts/<letter>/<name>` file, but the
  server cannot then deserialize it through the normal account path.
- Existing-character login resolves a positive PID and invokes the typed asynchronous
  player-load pipeline. That pipeline unconditionally acquires a SQL connection and
  invokes the database repository.

Name availability also depends on `sql_player_exists()`. With a no-MySQL stub returning
false, an otherwise mechanically repaired build could accept a name that already exists
in old data.

New character identity still uses the file `Players/pc_idnumb`, but that does not make
the rest of character persistence flat-file based. The allocator truncates and rewrites
one counter file without locking, an atomic rename, `fsync`, or inclusion in the current
flat backup set.

### The surviving pfile path is not a player fallback

[`src/files.c`](../../src/files.c) still contains the legacy binary codec and a private
`persistence_write_character_flat_fallback()` helper. Its current use is narrowly scoped
to a failed locker save. Normal player SQL failure logs that the flat fallback is retired
and returns failure.

Even if old pfiles are present:

- `restoreCharOnly()` can parse part of an old character under `__NO_MYSQL__`, but normal
  account/login routing no longer calls it.
- `restoreItemsOnly()` returns immediately for a PC with a positive PID because the
  current design assumes SQL already loaded the objects. Historical pfiles normally
  contain a positive PID, so inventory, equipment, affects, and shape data are silently
  omitted by this route.
- Shapechange state is currently SQL-only.
- Corpse, saved-world-item, shopkeeper, and other restore entry points are SQL-only.
- The legacy pet writer is not a viable fallback: its path construction is commented out
  while the resulting buffer is still used, and pet item restoration is also disabled.

The locker fallback is especially misleading in a database-loss scenario: it can write
a pfile-shaped locker after SQL failure, but the current locker loader reads SQL rather
than that file.

### Local journals are not a substitute database

The current player save path is:

```text
game state -> immutable player snapshot -> synced local journal -> SQL repository
```

The critical-command path is similar:

```text
accepted operation -> synced local command journal -> SQL inbox/domain/ledger/outbox transaction
```

The player journal is substantially safer than the legacy pfiles as a file format. It
has a magic/version, bounds, checksums, restrictive permissions, synchronized writes,
atomic compaction, directory synchronization, and quarantine behavior. The snapshot
model also explicitly represents many player components.

Its lifecycle is nevertheless a queue, not a primary store:

- records are applied through the SQL repository;
- acknowledged revisions are removed during checkpoint/compaction;
- it has no complete account index or general latest-record lookup contract;
- it does not cover every persistent domain;
- replay without SQL cannot commit or acknowledge records;
- the player loader never reconstructs a character from this journal.

An acknowledged current player may therefore have **no current flat snapshot remaining**.
An unacknowledged snapshot may be recoverable as payload data, but the current server
cannot use it to log the player in. The critical journal likewise preserves pending
commands, not the complete materialized state those commands depend on.

See [`PLAYER_SAVE_PIPELINE.md`](../persistence/PLAYER_SAVE_PIPELINE.md) and
[`PLAYER_SAVE_JOURNAL.md`](../persistence/PLAYER_SAVE_JOURNAL.md) for the intended
handoff semantics.

## What is still stored in files on the current branch

The current tree is mixed, not file-free. These stores are important operationally but
do not add up to a replacement database.

| Store | Current path or implementation | DB-loss value and limitation |
|---|---|---|
| Player save journal | `PLAYER_SAVE_JOURNAL_DIR`; `src/player_save_journal.c` | Can preserve unacknowledged typed snapshots. It is not a complete latest-player store and has no load path. |
| Critical command journal | `CRITICAL_COMMAND_JOURNAL_DIR` | Can preserve pending operations. Applying them still requires SQL transaction state. |
| Legacy player/locker pfiles | `Players/<letter>/...`; `src/files.c` | Old player files are untouched and a locker-only writer remains. Data may be stale and current login/load routing cannot use it completely. |
| Player PID counter | `Players/pc_idnumb` | Still allocates new PIDs, but is a fragile single file and is omitted from the flat backup script. |
| Mail | `Accounts/mail`, with a legacy `Players/mail` path | Independent fixed-block binary store. It lacks transactional linkage to account/character identity and has old durability characteristics. |
| Boards | `lib/boards/*`; `src/boards.c` | Independent binary board files, generally rewritten directly. |
| Guild MOTD | `Players/Assocs/asc.<id>.motd` | Only the MOTD remains file-backed; guild authority is SQL-backed. |
| Admin/configuration files | bans, accepted/declined markers, properties, Hall of Fame/relic scores, reports | Useful independent state, with varied formats and durability. Not player/account/world authority. |
| Copyover and maintenance state | configured runtime files | Process handoff or scheduler coordination only, not durable gameplay reconstruction. |
| Redis world recovery | Redis generations and floor deltas | Database-backed service, and explicitly not player authority. It is unavailable if all databases are removed. |

A fresh checkout also does not provision the complete historical runtime topology under
`Players/`, `Accounts/`, and `Ships/`. Most runtime content is intentionally ignored by
Git. A production flat backend would need an explicit, validated directory initializer
rather than relying on directories inherited from an old deployment.

## Historical flat-file system before player migration

### Commit boundary and migration timeline

| Revision | Date | Significance |
|---|---|---|
| `97a4166c` | 2025-12-28 | Last revision before the player-to-database project; historical comparison point used here. |
| `35f66dfc` | 2025-12-28 | Adds phase-one player schema and `sql_player` basics. |
| `732859d6` | 2025-12-28 | Adds SQL player save/load implementation. |
| `dd06c92e` | 2025-12-28 | Introduces dual pfile and SQL writes. |
| `6770ce74` | 2026-01-01 | Moves a broad set of accounts, corpses, saved items, ships, guilds, towns, shape data, and related domains toward SQL. |
| `27ac3084` | 2026-04-19 | Replaces pfile existence/delete/rename behavior with SQL equivalents. |
| `4f6b5fdf` | 2026-06-14 | Introduces asynchronous database persistence work. |
| `28735fde` / `a16731bb` | 2026-08-27 | Cuts nonterminal saves over to the snapshot pipeline and adds terminal fences. |
| `f3e39720` | 2026-08-27 | Retires normal legacy flat save forks. |

The old source can be inspected without changing the worktree with commands such as:

```text
git show 97a4166c:src/files.c
git show 97a4166c:src/account.c
git diff 97a4166c..35f66dfc -- src migrations
```

### Player pfiles

The historical primary character file was:

```text
Players/<lowercase first character>/<lowercase character name>
```

It was one binary record with offset-delimited sections. At `97a4166c`, the declared
versions were player save version 5, status version 47, skill version 2, item version 35,
affect version 8, and witness version 2, with a nominal 240,000-byte maximum.

The record covered a broad amount of character state:

- save/rent intent, room, timestamp, identity, descriptions, surname, and credential;
- stats, race, classes and levels, flags, conditions, currencies, and bank values;
- trophies, languages, introductions, timers, undead/forged/granted state;
- skills, witness records, and affects;
- recursive inventory and equipment, with containment and wear slots.

Objects were stored as a prototype vnum plus selected differences from the prototype,
rather than as a self-contained object schema. That saved space but tied recovery to the
exact object prototypes and codec assumptions of the server build.

The save procedure temporarily unequipped the player and removed effects, serialized the
record into a static buffer, then restored the live character for nonterminal saves. It
renamed the current file to `<name>.bak`, wrote a replacement, restored the backup on
some write failures, and removed the backup on success.

The load was split across phases. Account/login code called `restoreCharOnly()`, and the
nanny path later called `restoreItemsOnly()` according to rent/save type. There was no
transaction tying those phases, the account file, and other domain files together.

### Account files

Accounts were stored as line-oriented text at:

```text
Accounts/<lowercase first character>/<lowercase account name>
```

The file included account serial/name, email, credential and confirmation strings,
known IPs, account flags and timestamps, plus a list of characters with metadata such as
login count, last login, blocked state, and racewar side.

The writer rotated the existing account file to `.bak` and then wrote a new file, but it
did not reliably restore the backup if opening or writing the replacement failed. It
also did not verify every formatted write or final close. An account and its character
pfiles could be durably out of sync.

### Other historical file-backed domains

| Domain | Historical representation at `97a4166c` | Notable behavior or weakness |
|---|---|---|
| Lockers | Synthetic `<name>.locker` records using the player pfile codec | Locker contents were file-backed, but locker access lists already depended on SQL. |
| Corpses | `Players/Corpses/<owner><save-id>` binary object graphs | Directory scan at boot and `.bak` rotation; no fully durable publish protocol. |
| Saved floor/world items | `Players/SavedItems/item.<word>.<pointer>` binary files | Filename identity included a process pointer, not a stable durable item ID. Boot could unlink/rewrite entries. |
| Shopkeepers | `Players/ShopKeepers/<shop-id>` binary status/affect/item data | Separate whole-entity files with pfile-like codec and backup behavior. |
| Guilds | `Players/Assocs/asc.<id>` text plus `.motd` | Direct rewrite; initialization stopped after 20 consecutive missing IDs, so sufficiently sparse later IDs could be missed. Alliances were already SQL-backed. |
| Towns | `Players/towns` text | One directly rewritten file for the domain. |
| Siege state | `Players/siege`, mixed text and binary object encoding | One directly rewritten file; no atomic multi-record publication. |
| Ships | `Ships/ship_index` plus `Ships/<owner>` text files | A ship save rewrote both an owner file and the global index without a cross-file transaction. Cargo-market state was already SQL-backed. |
| Crafting recipes | `Players/Tradeskills/<letter>/<name>.crafting` vnum lists | Independent from the player save and renamed-character lifecycle. |
| Shapechange | `Players/Shapechange/<letter>/<Name>` text | Direct rewrite, no backup or transaction with the pfile. |
| Pets | Intended `Players/Pets/<id>` binary data | The writer's pathname setup was already commented out and item restore disabled; this was not a reliable production store even at the baseline. |
| Mail | Fixed-block account/player mail file | Independent binary store with no transaction linking it to account or character changes. |
| Boards and administrative state | `lib/boards/*` and multiple text/binary files | Independent legacy stores with inconsistent durability and recovery behavior. |

The migration utility under [`src-migrate/`](../../src-migrate) is useful corroborating
evidence of the old layout. It can import accounts, players, lockers, ships, guilds,
recipes, spellbooks, corpses, saved items, and shopkeepers into SQL. It is **one-way**;
there is no corresponding current SQL-to-flat export or cutover tool.

### The historical baseline was already hybrid

At `97a4166c`, normal boot still initialized MySQL and treated failure as fatal, the
Makefile still linked the MySQL client, and SQL calls were spread across the codebase.
Important already-SQL-backed areas included:

- alliances;
- outposts;
- nexus stones;
- ship cargo/market state;
- locker access lists and private-chest-related metadata;
- artifact and other economy/gameplay records.

There were nominal `__NO_MYSQL__` branches for some of these modules, but they generally
disabled the feature rather than replacing its persistence. The old revision is a useful
picture of the **player/account flat-file era**, not a ready DB-free release candidate.

## Legacy format and durability risks

### Portability and schema evolution

The pfile codec wrote native C/C++ scalar byte representations and recorded native
`short`, `int`, and `long` widths. The loader rejected width mismatches, but the format
did not provide a platform-neutral endianness/ABI contract. Version-specific loaders
and prototype-delta objects made long-term recovery depend on keeping old code and world
prototypes available.

The account, guild, town, ship, recipe, and other text formats were hand-parsed and
independently versioned, if versioned at all. A rename, identifier migration, or partial
schema change had to update several unrelated files correctly.

### Crash consistency

Most legacy writers used direct truncate/write or a simple `current -> .bak -> new`
sequence. They generally lacked the complete durable publication sequence:

1. create a same-filesystem temporary file with restrictive permissions;
2. write and validate a bounded record;
3. `fdatasync`/`fsync` the new file;
4. atomically rename it into place;
5. `fsync` the parent directory;
6. retain and verify a known-good prior generation.

Consequently, a successful `write()` or `fclose()` did not guarantee recovery after
power loss. Multi-file entities such as account plus character, or ship owner file plus
ship index, had no atomic commit boundary.

One particularly severe historical ordering bug existed in `writeCharacter()`:
terminal saves extracted/destroyed the live inventory before final size validation and
before the replacement file was successfully opened and written. A save failure could
therefore lose live items. The current code correctly defers terminal extraction until
the durable save path reports success; resurrecting the old function would reintroduce
the old risk unless that ordering is preserved.

### Identity and transactional integrity

The old system had no global transaction model for operations spanning characters or
domains. Examples include transfers, auctions, lockers, corpses, guild funds, ships, and
account/character membership. A process crash could publish one side and not the other.
Retries had no universal operation ID or idempotency record, creating duplication as
well as loss risk.

`Players/pc_idnumb` was a truncate-and-rewrite counter with no locking or durable atomic
publication. Saved-world-item filenames sometimes used pointers. Neither design is safe
for concurrent writers, restore from an incomplete backup, or durable global identity.

### Bounds, concurrency, and corruption handling

The character serializer used process-global/static buffers and mutable codec state,
which was suitable only for carefully serialized main-thread use. Its nominal size check
occurred after serialization, so the buffer discipline itself was not a robust boundary.
Some readers could quarantine or rename bad files, but there was no uniform checksum,
generation number, corruption catalogue, or repair workflow across domains.

### Backup completeness

[`scripts/backup_pfiles.sh`](../../scripts/backup_pfiles.sh) is not a safe production
flat-store backup contract:

- it chooses database versus file backup based on the `REDIS` flag rather than an
  authoritative persistence-backend setting;
- with Redis enabled and MySQL removed, it still selects `mysqldump`;
- the file branch copies player letter directories, ships, and selected `Players/`
  subdirectories, but omits `Accounts/`, account mail, `Players/pc_idnumb`, pets, towns,
  siege state, journals, and other root/state files;
- it copies a live, changing collection without a generation boundary or lock;
- retention is two days, with no manifest, checksums, or automated restore test.

The historical backup script had substantially the same coverage gap and also omitted
accounts. A restored set could therefore contain character pfiles without login
accounts and could rewind the PID allocator into collisions.

## Current versus historical versus production-ready

| Property | Current branch | Pre-player-DB revision | Required DB-free production state |
|---|---|---|---|
| Boots without MySQL | No | Not by default; already hybrid | Yes, with no MySQL headers, library, process, or schema required |
| Account/auth authority | SQL | Text account files | Versioned, secure, atomic account repository and indexes |
| Player authority | SQL component tables via snapshot pipeline | Binary pfile | Versioned latest snapshot plus revision history/recovery policy |
| Player local journal | Strong handoff journal, SQL consumer | None equivalent | WAL used with a flat materialized store and replayable without SQL |
| Item identity/ownership | SQL UID owner table and ledger | Incomplete/local conventions | Stable UID allocator, current owner index, immutable transfer ledger |
| Cross-entity operation atomicity | SQL transactions and idempotency inbox | None general | Operation-keyed WAL/transaction protocol with deterministic recovery |
| File crash durability | Strong for modern journals; mixed for legacy islands | Generally weak/inconsistent | Temp + sync + rename + parent sync, checksums and generations everywhere |
| Format portability | Typed modern codecs in selected pipelines | Native-width binary pfiles plus ad hoc text | Explicit byte order, limits, schemas, migrations, and compatibility tests |
| Domain coverage | Broad in SQL; partial files | Broad files but important SQL-only gaps | Inventory of every durable read/write site with no silent stubs |
| Backup/restore | Database-oriented; flat branch incomplete | Incomplete live copy | Generation-consistent whole-authority backup with routine restore drills |
| Concurrency | Designed around SQL transactions and bounded workers | Mostly assumes one serialized process | Declared single-writer or robust locks plus revision/conflict checks |

## Recommended production design

### 1. Support explicit fallback and flatfile-primary modes

Introduce an explicit boot setting such as:

```text
PERSISTENCE_MODE=mariadb-primary-flatfile-fallback
PERSISTENCE_MODE=flatfile-primary
```

In fallback mode, MariaDB may remain the normal authority, but the flat store must be kept
current enough to take over every durable domain under a defined replication/checkpoint
contract. Activation must switch the whole process at a known revision or manifest, not
silently route individual failed saves to pfiles. Per-save failover after some SQL writes
and some file writes creates two histories and invites item/currency duplication. Any
dual-write or journal-fed standby mechanism therefore needs explicit revisions,
comparison telemetry, lag/readiness reporting, reconciliation, and a controlled authority
switch and failback procedure.

In flatfile-primary mode, all durable reads and writes go through the flat backend and
MariaDB is neither required nor consulted. The flat implementation cannot be a reduced
feature mode: accounts, players, items, transactional commands, world domains, offline
mutations, administration, backup, and recovery must all retain their persistence
semantics.

The flat build must compile and link without MySQL headers or libraries. SQL-only source
files should either be excluded or implement repository interfaces behind a clean
backend boundary. CI should build and exercise both operating modes, including fallback
activation with MariaDB unreachable and flatfile-primary operation on a host with no
MySQL client support.

### 2. Separate durable repositories from gameplay code

At minimum, define complete interfaces for:

- account credentials, account metadata, IP history, character membership, name lookup,
  rename, block, and delete lifecycle;
- player latest-snapshot save/load by stable PID and monotonic revision;
- item UID allocation, current ownership, and immutable movement history;
- operation-keyed critical gameplay commands and their results;
- lockers/private chests and access control;
- corpses, saved floor items, shopkeepers, pets, shapechange, recipes, and spellbooks;
- guilds, alliances, guild halls, outposts, towns, siege, and nexus state;
- ships and cargo/market state;
- artifacts, auctions, economy ledgers, offline messages, and every other durable SQL
  producer found in the authority manifests and source audit.

No no-DB implementation should silently return success for a discarded write or return
an empty collection that changes gameplay semantics. Unsupported domains should fail
boot until implemented.

### 3. Reuse the modern snapshot model, not the old raw pfile contract

The current `PlayerSnapshot` capture/codec is a much stronger base than direct native
struct serialization. Extend it until the flat backend covers everything needed for an
authoritative reload. The present model includes status, languages, introductions,
timers, undead/forged/granted state, skills, affects, equipment, inventory, pets,
shapes, and trophies, but explicitly keeps recipes external and does not replace account
credentials, account membership, locker state, account bank, and all other domain data.

Use:

- explicit schema and codec versions;
- fixed byte order and fixed-width types;
- length, row-count, nesting-depth, and total-size bounds;
- checksums over headers and payloads;
- stable PID/item/operation IDs;
- monotonic revisions and compare-before-publish rules;
- strict file and directory permissions;
- quarantine plus observable recovery errors.

The legacy pfile reader should remain available as a bounded offline importer. It should
not be the normal writer for the new backend.

### 4. Use generations and atomic publication

A practical layout would give each authority a stable ID and immutable generation, with
a small atomic `CURRENT` pointer or manifest selecting the published generation. For
example:

```text
state/
  metadata/backend.json
  identities/accounts/<shard>/<account-id>/CURRENT
  identities/names/CURRENT
  players/<shard>/<pid>/CURRENT
  operations/wal/<segment>
  domains/<domain-name>/CURRENT
  manifests/CURRENT
```

Each file publication should use a same-directory temporary file, bounded encoding,
file sync, atomic rename, and parent-directory sync. A top-level manifest generation can
publish a consistent set for checkpoints involving several files. Retain known-good
prior generations according to an explicit recovery policy rather than one opportunistic
`.bak` file.

If production guarantees a single game process, document and enforce single-writer
ownership with a process lock. Internal workers still need per-entity serialization and
revision checks. Multiple independent writers require a more sophisticated coordinator
and should not be assumed safe merely because `rename()` is atomic.

### 5. Preserve transaction semantics with an operation WAL

Current critical operations rely on a SQL inbox, domain mutation, ownership ledger,
result, and outbox in one transaction. A flat backend must preserve the semantic
contract, not replace it with unrelated file writes.

For each operation:

1. assign a stable operation ID and expected entity revisions;
2. durably append the complete intent to a checksummed WAL;
3. deterministically materialize all affected entity generations;
4. atomically publish a manifest that selects all new revisions;
5. record the result/outbox state;
6. make replay idempotent and detect revision conflicts;
7. compact only after the materialized generation is included in a verified backup.

This is required for item/currency transfers, auctions, locker movement, guild funds,
corpse recovery, and similar operations where "save both sides later" is unsafe.

### 6. Make backup a property of the backend

The flat backend should expose a read-only snapshot/generation boundary. A backup must
include the complete selected authority-accounts, indexes and allocators, players,
operations, every domain, mail if retained separately, and the manifest describing it.
It should produce checksums, validate them, and regularly pass a cold restore test into
an empty directory.

Do not key backup selection from `REDIS`. Key it from the explicit persistence backend.
Do not copy an actively changing directory tree without a published immutable generation
or a coordinated writer pause.

## Migration, fallback, and cutover strategy

The current database is authoritative. Existing old pfiles are recovery artifacts, not
a safe export of current player state. A production cutover should therefore be driven
by a purpose-built **SQL-to-flat exporter** (or a server repository export), not by
turning old pfiles back on.

Recommended sequence:

1. Build a machine-readable inventory mapping every SQL table and remaining file store
   to one flat repository and owner.
2. Make `PERSISTENCE_MODE=flatfile-primary` compile, boot, and fail closed on missing domain
   implementations.
3. Implement account/name/PID authority and complete player snapshot load/save first.
4. Implement UID/ownership and critical-operation WAL semantics before enabling item or
   money movement.
5. Implement all remaining durable domains and offline/admin mutation routes.
6. Add a consistent SQL-to-flat export that records source schema/version, row counts,
   maximum revisions, IDs, checksums, and a cutover manifest.
7. Rehearse export and cold boot against a disposable restored database clone.
8. At production cutover, stop every writer, take a final database backup, run the final
   export, reconcile counts/balances/ownership/revisions, and publish one flat generation.
9. Start with the MySQL host blocked or client support absent, then exercise login,
   gameplay saves, restart, terminal transitions, and operational recovery.
10. Keep a tested rollback point and do not allow either side to accept divergent writes
    after the authority switch.

For fallback operation, continue from that same complete backend rather than creating a
smaller emergency pfile path. Define how MariaDB revisions reach the flat standby, how
operators determine that it is current and internally consistent, the exact command or
configuration change that transfers authority, and how writes are fenced on the old
authority. Regularly rehearse MariaDB loss, flat activation, cold restart, continued
gameplay, and reconciled failback. If the standby is incomplete or behind the declared
recovery point, the server must report that condition rather than claim fallback
readiness.

If the database has already been irretrievably lost, the available files are only a
partial salvage set:

- old pfiles may contain stale player/account-era data;
- unacknowledged journal records may contain newer player snapshots or operations;
- acknowledged journal records may already have been compacted;
- current account and many domain states are absent from those journals;
- the backup script may have omitted identity-critical files and entire domains.

Recovery in that case requires an offline forensic importer and explicit conflict/data
loss reporting. It cannot be made safe by starting the current binary with a flag.

## Suggested implementation order

### P0 - prove a real DB-free boundary

- Remove MySQL headers, objects, and link dependencies from the flat build.
- Add CI for a clean build on a host without MySQL development packages.
- Add explicit backend selection and validated/provisioned state directories.
- Make boot enumerate and reject every unimplemented durable domain.
- Add a database-free smoke boot with both MySQL and Redis unreachable.

### P1 - restore identity and player continuity

- Implement atomic account/auth, account-character membership, name index, PID allocation,
  rename, delete, and block flows.
- Implement current player snapshot materialization and load by PID/revision.
- Extend snapshot coverage for every character-owned component not currently captured.
- Make terminal save fences depend on flat materialization, not merely a queued record.
- Import legacy pfiles only through an offline, audited conversion path.

### P2 - restore transactional gameplay and domain coverage

- Implement stable item UID allocation, owner index, and transfer ledger.
- Implement the operation WAL and idempotent replay.
- Port lockers, corpses, saved items, shopkeepers, guild/alliance/outpost/town/siege,
  ship/cargo, recipes/spellbooks, artifacts, auctions, and remaining domain repositories.
- Audit every `qry`, `db_query`, SQL repository, and no-MySQL stub until none can silently
  discard or fabricate durable state.

### P3 - production operations

- Implement consistent export/cutover and whole-authority backups.
- Add corruption quarantine, inspection, repair, compaction, and capacity tooling.
- Add restore drills, revision/UID collision audits, monitoring, and alerting.
- Document upgrade compatibility and rollback for every flat schema version.

## Acceptance tests for production readiness

At minimum, the flat backend should pass all of the following with no MySQL server,
headers, client library, or schema present:

- clean compile, link, boot, shutdown, copyover, and restart;
- account create/login/credential update and character create/list/load/rename/delete;
- name and PID uniqueness across crash, restore, and concurrent/rapid creation attempts;
- player status, classes, skills, affects, languages, timers, trophies, witness data,
  equipment, deeply nested inventory, artifacts, pets, shapes, recipes, spellbooks,
  currency, and bank state across restart;
- terminal paths for inn, camp, link loss, death, artifact handling, and failed saves;
- lockers, private chests and access, corpses, saved floor items, and shopkeepers;
- guild/alliance/hall/outpost, town/siege/nexus, ship/cargo, auction, mail/offline, and
  other durable domain state;
- item and currency transfers with forced crash before and after every WAL/publication
  transition, proving neither loss nor duplication;
- disk-full, short-write, failed rename, failed sync, corrupt checksum, oversized record,
  invalid nesting, stale revision, and permission/symlink attack cases;
- rapid repeated saves of one player and simultaneous independent-player saves;
- backup during normal operation, restore into an empty directory, cold boot, and full
  reconciliation of accounts, PIDs, UIDs, revisions, ownership, balances, and domains;
- compatibility import tests for each supported historical pfile version and clear
  rejection/quarantine of unsupported or corrupt files.

Fallback readiness additionally requires forced MariaDB-loss tests at save and
transaction boundaries, a verified whole-authority switch to the flat store, continued
operation across restart, explicit recovery-point/lag reporting, fencing against
divergent MariaDB writes, and a rehearsed reconciliation/failback path.

## Source map

Current implementation entry points:

- Build and boot: [`src/Makefile`](../../src/Makefile),
  [`src/comm.c`](../../src/comm.c), [`src/sql.c`](../../src/sql.c),
  [`src/sql.h`](../../src/sql.h)
- Account/login and identity: [`src/account.c`](../../src/account.c),
  [`src/nanny.c`](../../src/nanny.c)
- Legacy pfile codec and compatibility routes: [`src/files.c`](../../src/files.c),
  [`src/files.h`](../../src/files.h)
- Player capture/load/save: [`src/player_snapshot.h`](../../src/player_snapshot.h),
  [`src/player_snapshot_capture.c`](../../src/player_snapshot_capture.c),
  [`src/player_load_pipeline.c`](../../src/player_load_pipeline.c),
  [`src/player_save_pipeline.c`](../../src/player_save_pipeline.c),
  [`src/player_snapshot_repository.c`](../../src/player_snapshot_repository.c)
- Journal: [`src/player_save_journal.c`](../../src/player_save_journal.c),
  [`src/player_save_journal.h`](../../src/player_save_journal.h)
- Representative migrated domains: [`src/storage_lockers.c`](../../src/storage_lockers.c),
  [`src/assocs.c`](../../src/assocs.c), [`src/alliances.c`](../../src/alliances.c),
  [`src/outposts.c`](../../src/outposts.c), [`src/nexus_stones.c`](../../src/nexus_stones.c),
  [`src/siege.c`](../../src/siege.c), [`src/ships/ship_base.c`](../../src/ships/ship_base.c),
  [`src/ships/ship_cargo.c`](../../src/ships/ship_cargo.c)
- Remaining file islands: [`src/mail.c`](../../src/mail.c),
  [`src/boards.c`](../../src/boards.c)
- Migration evidence: [`src-migrate/`](../../src-migrate)
- Backup behavior: [`scripts/backup_pfiles.sh`](../../scripts/backup_pfiles.sh)

Historical evidence is identified by commit because those implementations are no longer
present unchanged in the working tree. The most relevant views are:

```text
git show 97a4166c:src/files.c
git show 97a4166c:src/files.h
git show 97a4166c:src/account.c
git show 97a4166c:src/assocs.c
git show 97a4166c:src/siege.c
git show 97a4166c:src/ships/ship_base.c
git show 97a4166c:backup_pfiles.sh
```

## Decision statement

Implement one first-class, full-coverage flat-file backend with two supported uses: a
complete fallback authority when MariaDB fails, and an explicitly selectable primary
authority for operation with MariaDB fully absent. A fallback designation does not lower
the coverage requirement; it must handle all saving and loading needs before it can be
advertised as ready. The filesystem currently present beside the database is not such a
fallback or replica. Until the new backend, synchronization contract, and authority
switch are implemented and tested, an unexpected database outage still requires
restoring MariaDB from a verified backup if at all possible.

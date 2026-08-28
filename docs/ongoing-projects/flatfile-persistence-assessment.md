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

### Milestone status

| Milestone | State | Evidence |
|---|---|---|
| P0 - real DB-free boundary | In progress | Mode/root fail-closed contract and compile/link selection implemented; common source still exposes SQL types |
| P1 - identity and player continuity | Not started | No flat account/player authority yet |
| P2 - transactional gameplay and domains | Not started | No flat operation WAL/domain repositories yet |
| P3 - production operations | Not started | No exporter, whole-authority backup, or restore drill yet |

## Executive conclusion

The current server is **not capable of production operation with MySQL/MariaDB removed**.

- The normal build requires the MySQL client library, and normal boot treats database
  initialization or schema verification failure as fatal.
- The dormant `__NO_MYSQL__` compile switch does not currently build. Fixing its first
  compile errors would still leave account loading, character discovery, player loading,
  player saving, and many world subsystems stubbed out or disabled.
- The current player and critical-command journals are durable handoff queues whose
  consumer is MySQL. They are not a searchable flat-file database, do not contain all
  durable domains, and cannot reconstruct all acknowledged state after database loss.
- The remaining pfile reader and writer code is compatibility code, not a complete
  fallback. Normal players are no longer written to pfiles, account login is SQL-only,
  and positive-PID pfiles have their objects skipped by the current item loader.

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

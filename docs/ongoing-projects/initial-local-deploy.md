# Initial Local Flat-File Deployment Audit

## Objective

Make a fresh checkout build and start reliably as a local, client-free DurisMUD
deployment using `flatfile-primary`, with no MariaDB or Redis dependency. Find,
reproduce, document, and fix all errors, warnings, unsafe defaults, missing steps,
and operational gaps encountered from initial configuration through clean shutdown.

## Success criteria

- `.env.example` can be turned into a valid owner-only local flat-file `.env` by
  following documented steps.
- The client-free server builds from a clean checkout with the supported warning
  profile and no compiler warnings or errors.
- Minimal-world and full-world startup reach the game loop without a database.
- Listener, health, account/login, persistence, restart, backup, and clean shutdown
  behavior are exercised where the local environment permits.
- The safe non-database regression gate passes.
- Every discovered issue is either fixed and verified or recorded with exact
  evidence and an explicit remaining action.

## Safety boundaries

- Do not run migrations, wipes, or database-backed operational commands.
- Do not connect to production services.
- Keep `.env`, credentials, player data, logs, backups, and generated artifacts out
  of Git.
- Use repository-local runtime paths owned by the current user.

## Baseline

- Date: 2026-08-30 (Asia/Jerusalem)
- Checkout: fresh clone of `https://github.com/LuminariMUD/DurisMUD.git`
- Branch: `master`
- Initial commit: `7723c3db` (`Add production systemd service`)
- Initial tracked worktree: clean
- User-created local files before the audit: `.env` copied from `.env.example`;
  this empty audit journal

## Findings and fixes

### F001 - Copied example is not a no-database local configuration

- Status: Fixed locally and preflight verified
- Evidence: `.env.example` selects `PERSISTENCE_MODE=mariadb-primary`, enables
  Redis, leaves database credentials empty, and contains placeholder absolute
  paths.
- Impact: Copying the example unchanged cannot start the requested client-free
  flat-file deployment.
- Correction: configured the ignored `.env` for `flatfile-primary`, disabled Redis,
  cleared unused database allow-listing and the placeholder DurisWeb secret, and set
  absolute repository-local paths beneath ignored `bin/local/`.
- Verification: `./scripts/cycle_mud.sh --dev --check-config` exited zero and printed
  `Validated database-independent configuration for flatfile-primary`.

### F002 - Copied `.env` permissions are rejected

- Status: Fixed locally and verified
- Evidence: the copied `.env` was mode `0644`; `scripts/cycle_mud.sh` requires owner
  ownership and no group/other permission.
- Correction: changed `.env` to mode `0600` and created the state, backup, player
  journal, and critical-command journal directories with mode `0700`.
- Verification: `stat` confirmed the expected owner and modes for the file and all
  configured directories; launcher configuration preflight passed.

### F003 - Placeholder game account password is invalid shell syntax

- Status: Fixed and verified
- Evidence: `.env.example` contains `GAME_ACCOUNT_PASSWORD=<password>`, while
  `scripts/cycle_mud.sh` loads `.env` with Bash `source`; the angle brackets are
  parsed as shell redirection rather than a literal placeholder value.
- Correction: replaced the ignored local `.env` account fields with local test-only
  values. The checked-in example now uses blank, shell-safe test-account fields and
  explicitly warns against committing credentials. No credential value is recorded
  in this tracked journal.
- Runtime note: the first replacement password contained the configured account name
  and was correctly rejected by `valid_password`; it was replaced with a policy-valid
  value before any account record was created.
- Verification: `bash -n .env.example` and the updated flat-file launcher regression
  passed. A real local account was created, authenticated repeatedly, and used to
  create, save, restart, and reload the configured character.

### F004 - Direct full-world boot regression omits world generation

- Status: Fixed and verified
- Evidence: direct execution of
  `python3 tests/async/test_flatfile_full_world_boot.py` failed before the game loop
  with `Trouble opening mobile file world.mob: No such file or directory` on the
  fresh checkout. The test symlinked `areas/` into its isolated run root but did not
  first generate the ignored `areas/world.*` outputs.
- Impact: the advertised focused regression is not self-contained on a clean clone,
  and it cannot validate the full-world flat-file path until another command happens
  to generate the world.
- Correction: the regression now runs the maintained incremental `make world`
  prerequisite and fails with bounded captured output if generation fails.
- Verification: the focused full-world regression passed from an isolated build and
  again as test 338 of the complete regression gate.

### F005 - Missing full-world data terminates abnormally

- Status: Fixed and verified
- Evidence: after the missing `world.mob` diagnostic, the isolated server printed
  `terminate called without an active exception` rather than completing a controlled
  startup failure.
- Impact: direct server invocation with incomplete generated data aborts noisily and
  may bypass normal cleanup.
- Root cause: `run_the_game()` started the joinable player-load worker before
  `boot_db()`. Legacy fatal world-file errors call `exit(1)`, and destruction of the
  still-joinable global C++ thread invoked `std::terminate`, converting a clear
  configuration failure into SIGABRT.
- Correction: delayed player-load pipeline initialization until all fatal world-data
  loading and boot initialization has completed.
- Verification: an isolated missing-world run now reports the missing `world.mob`
  and exits 1 without `terminate called` or a signal. The boot preflight permanently
  exercises that failure path and passed in the full regression gate.

### F006 - Health endpoint always reports flat-file mode unhealthy

- Status: Fixed and verified
- Evidence: the configured minimal server reached `Entering game loop.` and listened
  on ports 4000, 4001, and 4050, but `scripts/healthcheck.sh` failed because `/health`
  returned HTTP 503. `src/websocket.c` based readiness only on
  `sql_pool_is_active()`, which is necessarily false in a client-free build.
- Impact: supervisors and operators cannot distinguish a healthy no-database server
  from a failed one, defeating readiness checks for the supported flat-file mode.
- Correction: health readiness now accepts a validated flat-file authority or an
  active MariaDB pool, and its JSON uses the backend-neutral `persistence` field.
  The checked-in probe, API documentation, source contract, and isolated client-free
  boot regression were updated together.
- Verification: the source contract, isolated minimal boot, and configured live
  server all passed. `scripts/healthcheck.sh` returned zero while the client-free
  server remained on the same PID, and the full regression gate passed.

### F007 - Account password hashing frees across incompatible allocators

- Status: Fixed and verified
- Evidence: live telnet account creation accepted the name and email, then the server
  logged `delmem: memory failed check!` and aborted with exit 134 when processing the
  password. `bcrypt_hash_password()` returns a standard `malloc()` allocation, but
  account creation, legacy account-password upgrade, WebSocket registration, and
  WebSocket password change released it through the custom `FREE` macro. With
  `MEMCHK=1`, `FREE` expects the repository allocation header and aborts on standard
  allocations.
- Impact: new account creation crashes the entire MUD; three other account password
  paths carry the same crash risk.
- Correction: all four bcrypt result owners now use standard `free()`, matching the
  API implementation and the existing SQL password callers. The hashing regression
  now enforces the allocator contract for account and WebSocket consumers.
- Verification: the allocator contract passed, and live account creation plus
  repeated password authentication completed without an abort or PID change.

### F008 - Flat-file account reload transfers incompatible allocations

- Status: Fixed and verified
- Evidence: after F007 was fixed, live account creation reached confirmation, then a
  second account save again logged `delmem: memory failed check!` and aborted with
  exit 134. `write_account()` reloads every matching live descriptor after a save.
  The flat-file adapter returned strings and list nodes allocated by `strdup` and
  `calloc`; `read_account()` transferred those pointers into a live account whose
  cleanup uses `FREE`.
- Impact: flat-file account creation and any repeated account update can crash the
  MUD even when password hashing ownership is correct.
- Correction: client-free `read_account()` now deep-copies the isolated DTO into
  live-account allocations, while a new adapter release API destroys the DTO with
  standard `free()`. MariaDB pointer-transfer behavior is unchanged. The flat-file
  membership regression now checks this ownership boundary.
- Verification: the adapter and membership regressions passed. Live account creation,
  account rewrites, disconnect, and reconnect authentication all completed on one
  server PID.

### F009 - Account character list loses persisted display metadata

- Status: Fixed and verified
- Evidence: after creating and saving a valid level-one Human Warrior, the account
  character list displayed level 0, `None`, `Unknown`, and an unknown room. The
  account membership record was created with only name, timestamps, and alignment,
  and later player saves did not refresh its denormalized projection.
- Impact: the saved character existed and could load, but the account menu presented
  misleading identity and location data after reconnect or restart.
- Correction: new membership records now include level, race, primary and secondary
  class. Successful terminal character saves refresh PID, identity fields, alignment,
  room, and save time in the account authority. Existing records self-repair after a
  successful character load, using the actual placed room after minimal-world
  fallback.
- Verification: a fresh login first repaired the saved character to level 1, Human,
  Warrior; after loading into the minimal world, a second independent account login
  displayed `Cage of Smoke` as the last room. The projection survived subsequent
  restarts, and the membership regression plus all 338 tests passed.

### F010 - Persisted character fallback indexes a room vnum as an array index

- Status: Fixed and verified
- Evidence: GDB reproduced SIGSEGV in `enter_game()` while evaluating
  `zone_table[world[r_room].zone]`. When a saved full-world location was absent from
  the minimal world, the fallback assigned `GET_ORIG_BIRTHPLACE(ch)` directly to
  `r_room`; that value is a virtual room number, not a `world[]` index.
- Impact: selecting a correctly persisted character could crash a minimal-world
  client-free server during login.
- Correction: convert the original birthplace with `real_room()` and enforce a final
  bounds guard before any `world[]` or `zone_table[]` access, falling back to the
  tracked minimal cage room when necessary.
- Verification: the same saved character loaded into `Cage of Smoke`, accepted a
  `look` command, and left health green on the same PID. The focused source contract,
  restart checks, and full regression gate passed.

### F011 - First frag-leaderboard write requires a nonexistent catalog

- Status: Fixed and verified
- Evidence: the first new-character save on a fresh flat-file authority attempted to
  update the frag leaderboard before its catalog existed. The repository returned
  `not_found`, so otherwise valid character persistence emitted an avoidable failure.
- Impact: initial player saves could not establish leaderboard state without a
  separate bootstrap action.
- Correction: a first upsert now treats a missing catalog as an empty catalog and
  publishes the initial record atomically. Existing catalog validation and revision
  behavior are unchanged.
- Verification: the repository harness now begins from an empty authority and proves
  the first upsert creates and round-trips the catalog. The focused harness and full
  regression gate passed.

### F012 - Flat-file entry reports failure from an unavailable SQL-only save

- Status: Fixed and verified
- Evidence: after the backend-neutral `writeCharacter()` succeeded, `enter_game()`
  unconditionally called `sql_save_player_core()`. The client-free stub necessarily
  failed, producing a false post-entry persistence alert on an otherwise successful
  flat-file save.
- Impact: clean local operation appeared unhealthy and obscured real persistence
  faults with a backend-inapplicable error.
- Correction: the backend-neutral save result is checked for every build, while the
  SQL-only core write and its alert are compiled only for MariaDB-capable builds.
- Verification: live flat-file entry completed without the false alert, the character
  persistence contract passed, and the final runtime log was clean.

### F013 - Starter-item grants use stale flat-file ownership revisions

- Status: Fixed and verified
- Evidence: every starter grant for a fresh character printed that the ownership
  authority did not commit, the item was discarded, inventory remained empty, and
  the critical-command journal recorded terminal integrity failures. The initial
  player baseline creates owner revision 1 while runtime remained at revision 0. A
  later restart also reset the runtime system-owner revision to 0 even after prior
  creation grants had advanced the persisted revision.
- Impact: new characters received no starter equipment on the first clean install;
  after any successful creation and restart, later character grants would fail for
  the same reason at the system-owner side.
- Correction: the first successful flat-file character save reloads and publishes the
  authoritative player-owner revision before grants begin. Client-free boot now
  hydrates the persisted system-owner revision before gameplay, treating a missing
  catalog as the valid revision-zero clean-install case and failing boot on malformed
  authority.
- Verification: a fresh character received its complete kit without discard or
  critical-command errors. A second fresh character created after a full server
  restart also succeeded, proving system-owner revision continuity. Health remained
  `healthy` / `ready`, and source, repository, boot, and full-suite contracts passed.

### F014 - New-character entry submits the starter kit twice

- Status: Fixed and verified
- Evidence: `do_start()` already called `load_obj_to_newbies()`. Deferring grants
  until after F013's durable baseline and then calling the grant routine again caused
  an exact doubled kit; a Human Warrior held 54 items instead of one 27-item set.
- Impact: the revision-order fix could overgrant every new character and persist the
  duplicate items.
- Correction: factored the existing start initialization behind a deferred-kit entry
  point. Normal administrative and multiclass callers preserve legacy behavior;
  first entry performs all level-one initialization without items, writes and
  synchronizes the baseline, then submits exactly one grant sequence.
- Verification: a fresh Human Warrior converged to one 27-item kit, survived a clean
  shutdown and restart with exactly 27 items, and the character-creation class-toggle
  and persistence contracts passed.

### F015 - Player-load cancellation races queued completion publication

- Status: Fixed and verified
- Evidence: the complete regression gate intermittently returned `applied` for a
  request cancelled immediately after submission. The worker could enqueue the
  completion just before the game thread inserted the cancellation marker, leaving
  the already-queued result unchanged.
- Impact: a disconnected or superseded login could publish a load result that the
  caller had already cancelled.
- Correction: cancellation now rewrites any already-queued matching completion under
  the pipeline mutex; cancellation markers are cleared when the completion is
  delivered. The worker-side cancellation path remains unchanged for jobs not yet
  published.
- Verification: the focused concurrent harness passed five consecutive runs, then
  `make test-all PERSISTENCE_BACKEND=flatfile` passed all 338 tests.

## Activity log

### 2026-08-30 - Orientation and baseline

- Read `AGENTS.md`, `README.md`, `.env.example`, `Makefile`, `src/Makefile`,
  `scripts/start_mud.sh`, and the flat-file references in project documentation and
  tests.
- Confirmed the supported client-free build selector is
  `make -C src PERSISTENCE_BACKEND=flatfile` and that it defines `__NO_MYSQL__`.
- Confirmed dedicated boot coverage exists in
  `tests/async/test_flatfile_boot_preflight.py` and
  `tests/async/test_flatfile_full_world_boot.py`.
- Audited local prerequisites. Compiler, Make, Python, formatting tools, OpenSSL,
  netcat, required library metadata, MariaDB client tooling, and Redis are installed;
  `telnet` is absent, but netcat can exercise the plain listener.
- Updated the ignored `.env` for client-free operation and secured all local
  configuration/state paths.
- Ran `./scripts/cycle_mud.sh --dev --check-config`: passed without database access.
- Ran `make PERSISTENCE_BACKEND=flatfile -j2`: passed. The client-free server,
  editor, and all six area generators were produced; the server warning profile
  includes `-Werror`.
- Inspected `bin/server/dms_new` with `ldd`: no MySQL or MariaDB client library is
  linked.
- Ran `python3 tests/async/test_flatfile_boot_preflight.py`: passed clean client-free
  build, minimal-world game-loop entry, exact private authority topology, SIGTERM,
  and normal shutdown.
- Ran `python3 tests/async/test_flatfile_full_world_boot.py`: failed because its
  world generation prerequisite was missing; F004 and F005 capture the findings.

### 2026-08-30 - Runtime fault isolation and correction

- Generated an owner-only localhost TLS certificate and key for the ignored local
  runtime; verified the private key and every flat-file authority record are mode
  `0600`, while authority, journal, and backup roots are mode `0700`.
- Started the configured checkout with `./scripts/cycle_mud.sh --minimal`; confirmed
  plain TCP on 4000, TLS on 4001, and HTTP health on 4050.
- Corrected the flat-file health contract, then verified HTTP 200 with the
  backend-neutral `persistence` response through the checked-in health probe.
- Exercised real account creation and isolated two allocator mismatches under the
  enabled memory checker: bcrypt result ownership and flat-file account DTO transfer.
  Corrected both and repeated account creation/authentication without a restart.
- Completed interactive character creation through rules acceptance and first entry,
  persisted the character and account state, and confirmed snapshots and authority
  files remained private.
- Used GDB to reproduce the saved-character minimal-world crash at the exact invalid
  room index. Corrected vnum conversion and bounds validation, then reloaded the same
  saved character successfully.
- Corrected account character projection creation/update and verified an independent
  reconnect displays `Level 1 | Human | Warrior | Cage of Smoke`.
- Reproduced the missing full-world startup as exit 134 with
  `terminate called without an active exception`; delayed worker creation and
  confirmed the same run now exits 1 with only the actionable missing-file error.

### 2026-08-30 - Final verification

- Rebuilt the client-free server with the supported `-Werror` warning profile: no
  compiler warning or error.
- Ran `python3 tests/async/test_flatfile_boot_preflight.py`: passed clean build,
  minimal boot, HTTP health, private authority topology, controlled missing-world
  failure, SIGTERM, and normal shutdown.
- Ran `python3 tests/async/test_flatfile_full_world_boot.py`: passed generated-world
  boot, player-corpse and shopkeeper restoration stages, and clean shutdown.
- Ran `make test-all PERSISTENCE_BACKEND=flatfile`: 338 passed, 0 failed. This also
  reran both isolated server boot tests from clean temporary build roots.
- Ran the updated flat-file launcher regression after correcting `.env.example`:
  passed, including shell syntax validation.
- Promoted the final staged binary through `cycle_mud.sh`, restarted the configured
  server, rechecked health and persisted account metadata, and performed a final
  clean Ctrl-C shutdown with exit status 0.
- Ran `git diff --check`: passed. Local credentials, state, logs, generated world
  output, binaries, certificates, and backups remain ignored and untracked.

### 2026-08-30 - Extended live persistence audit

- Exercised first frag-leaderboard publication from a missing catalog and corrected
  the bootstrap behavior.
- Removed a false SQL-only post-entry alert from the client-free path.
- Reproduced empty starter inventory and terminal item-transfer failures, traced them
  to unsynchronized player and system ownership revisions, and verified grants both
  before and after a server restart.
- Detected and removed a duplicate starter-kit submission introduced by the required
  baseline ordering. A fresh Human Warrior retained exactly one 27-item kit after
  restart.
- Reproduced and fixed the player-load cancellation/completion race found by the
  broad concurrent regression run.
- Ran formatting validation, focused repository and persistence contracts, five
  consecutive player-load race harness runs, and the final full gate: 338 passed,
  0 failed in 120.16 seconds.
- Rechecked live health, private file/directory modes, clean current logs, stable PID,
  backup creation, restart persistence, and normal exit-status-zero shutdown.

## Current checkpoint

- Completed: fresh-checkout configuration, secure local authority setup, clean
  client-free build, minimal and full-world startup, health, live account and
  character persistence, restart behavior, backups, controlled failure behavior,
  comprehensive regression coverage, final binary promotion, and clean shutdown.
- Open findings: none from the exercised initial flat-file installation and startup
  scope. The server is intentionally stopped after verification; start it with
  `./scripts/cycle_mud.sh --minimal` for the tracked smoke world or
  `./scripts/cycle_mud.sh` for the generated full world.

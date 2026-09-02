# Emergency production pull, burn-in, and deployment

Status: **complete**. Started and completed: 2026-09-02 UTC. Branch:
`master`. Production left running and healthy.

## Goal

Fast-forward the production checkout to `origin/master`, perform the complete
DurisMUD burn-in, repair every finding, deploy the qualified production build,
and prove it healthy with a non-destructive staff smoke test and log soak.

This file is the live operational ledger requested for the work. Credentials
and other secret `.env` values are intentionally never recorded here.

## Starting state

- Checkout: `/home/duris/duris`.
- Environment role: `production` (read from `.env` without exposing secrets).
- Starting commit: `e13ffd2ed` (`feat: finish Chaos infinite starting kit`).
- Starting worktree: one pre-existing tracked modification in
  `.agents/skills/burnin/SKILL.md`; it is unrelated user work and must be
  preserved.
- Production supervisor: PID `1427439`, command
  `/bin/bash /home/duris/duris/scripts/cycle_mud.sh --production`, cwd
  `/home/duris/duris`.
- Production game process: PID `1427637`, command `bin/server/dms 7777`, same
  cwd, child of PID `1427439`.
- Listeners owned by that game process: public telnet `7777` and loopback HTTP
  health `4050`. An unrelated listener on `4000` was observed and is explicitly
  out of scope.
- The first systemd query covered the system manager and did not find a Duris
  unit, so the running `cycle_mud.sh` supervisor initially appeared to be the
  runbook's fallback path. A later user-manager audit found the enabled
  `duris-mud-production.service`; its working directory and command point to
  this exact checkout and `scripts/cycle_mud.sh --production`.
- Initial `./scripts/healthcheck.sh`: pass.

## Operational timeline

All timestamps are UTC on 2026-09-02.

### Repository synchronization

- Inspected branch, remotes, status, root, and the pre-existing diff before any
  write.
- Read `AGENTS.md`, `README.md`, the root `Makefile`, the burn-in skill, and the
  runbook's starting/stopping and health sections.
- Ran `git fetch --prune origin`: only remote worktree branch
  `origin/worktree/20260902T050302Z` advanced; `origin/master` did not.
- Verified `git rev-list --left-right --count HEAD...origin/master` returned
  `0 0` and the upstream file diff was empty.
- Ran `git pull --ff-only origin master`: pass, `Already up to date`.
- Post-pull commit remains `e13ffd2ed`; the pre-existing skill edit remains
  untouched.

### Graceful production stop

- Identified the exact supervisor/child through PID, PPID, cwd, command line,
  executable, listeners, and health. No broad `kill`/`pkill` command was used.
- Probed the public login flow without credentials in command arguments or
  output, then loaded `GAME_ACCOUNT_NAME`, `GAME_ACCOUNT_PASSWORD`, and
  `GAME_ACCOUNT_CHARACTER_NAME` internally from `.env`.
- The first shutdown attempt did **not** execute: the character-selection flow
  asked `Play as <character>? (Y/N)`, and the command text was consumed as the
  answer. The socket then closed without changing server state.
- Reconnected, explicitly answered the character confirmation, and issued:
  `shutdown ok emergency burn-in qualification`.
- The staff command was accepted as an immediate, non-wipe shutdown; the game
  connection closed cleanly.
- Waited for both exact PIDs to disappear and confirmed listeners `7777` and
  `4050` closed. The unrelated `4000` listener was not touched.

### Canonical stopped-state gates

| Gate | Result | Evidence / finding |
| --- | --- | --- |
| `./scripts/format.sh --check` | pass | `Formatting OK` |
| `make test-all` (first attempt) | fail | Compiler could not find `cjson/cJSON.h`. |
| `sudo -n apt-get install --yes libcjson-dev` | unavailable | Account requires an interactive sudo password; no package was installed. |
| `make test-all` with verified local cJSON prefix | fail | Advanced past cJSON, then compiler could not find `hiredis/hiredis.h`. |
| `make test-all` with absent direct dev headers staged | fail | Advanced into `cmd/nq.c`; libxml2's header then required missing transitive `unicode/ucnv.h`. |
| `make test-all` with transitive libicu headers staged | fail | All server translation units compiled cleanly; final link required libbsd's hard dependency `-lmd`. |
| `make test-all` with libmd development/runtime staged | fail | Link selected dangling-prefix fallback `libgnutls.a`, yielding unresolved transitive crypto/asn1 symbols. |
| `make test-all` with matching shared-library targets staged | fail | Built server/editor/tools and generated the world; 369/385 Python tests passed. Eleven Redis live tests lacked `redis-server`/`redis-cli`; one WebSocket harness hard-coded the system cJSON include path; three isolated flat-file servers deliberately dropped `LD_LIBRARY_PATH`; one parallel journey build collided on its shared output (`ETXTBSY`). |
| `make test-all` after dependency and parallel-harness repair | pass | Maintained build and world generation passed; 386/386 Python tests passed in 481.64s; native signal-handler suite followed successfully. |
| `make test-db` | pass | All ten self-contained Docker/MySQL suites passed, including 143-step legacy migration/replay and bootstrap/runtime compatibility. No production DB endpoint was used. |
| `make clean-all && make` | pass | Deleted only reproducible outputs/caches; rebuilt the server, area editor, and every area generator/tool from scratch without error. |

## Prerequisite handling

The repository dependency manifest and CI require `libcjson-dev`, but the host
had neither `libcjson-dev`, `libcjson1`, `cjson/cJSON.h`, nor a loader-visible
`libcjson.so`. Docker is available (server version `29.7.2`).

Because passwordless sudo is unavailable, the signed Ubuntu packages were
downloaded and unpacked into a temporary non-repository prefix without changing
tracked source or weakening compiler flags:

- `libcjson-dev_1.7.17-1_amd64.deb` SHA-256
  `c498b1dda4805c9dddec19bbe04aaa3dd8229c01f16e9916379965ce1d497dce`.
- `libcjson1_1.7.17-1_amd64.deb` SHA-256
  `43d758e214612877ec9029f2384bdbb0a6eeba6e74bb2cd3e7a60f4108947c41`.

Both hashes exactly match configured APT metadata. The unchanged canonical
target is being run with `CPLUS_INCLUDE_PATH`, `LIBRARY_PATH`, and
`LD_LIBRARY_PATH` pointed at that prefix.

A later production-service audit found the host's existing durable dependency
prefix at `/home/duris/.local/opt/duris-deps`. Its cJSON, hiredis, and hiredis
SSL libraries are byte-for-byte identical to the APT-authenticated copies used
for the stopped-state qualification. The enabled user service has a checked-in
operator drop-in which injects that prefix's library directory through
`LD_LIBRARY_PATH`. Production therefore does not depend on either temporary
test prefix, and no runtime library needs to be copied into the checkout.

The cJSON-only rerun then exposed missing `hiredis/hiredis.h`. A complete
manifest/host audit found that the host also lacks the development headers for
ncurses, libxml2, GnuTLS, hiredis, and libbsd, although system runtime libraries
already exist for libxml2, GnuTLS, and libbsd. The following additional
APT-authenticated packages were downloaded and unpacked into the same temporary
prefix:

- `libhiredis-dev` 1.2.0-6ubuntu3, SHA-256
  `178926b5b14988c86f8f359f61894abd4ea6966e852a00349dbeaf3cccda09a2`.
- `libhiredis1.1.0` 1.2.0-6ubuntu3, SHA-256
  `912a25553f29a47d1524c0b58a3c5bff2d067109da2adae9fa9c4fb15efaa568`.
- `libncurses-dev` 6.4+20240113-1ubuntu2.2, SHA-256
  `2c32ce182e90e6636ddb037ba2810cace9e1f076b13fac89034277d8ba943c38`.
- `libxml2-dev` 2.9.14+dfsg-1.3ubuntu3.8, SHA-256
  `d8585797242a0171db244ca24527125233a02fd606c1e64e4e73ae0cffc58b2b`.
- `libgnutls28-dev` 3.8.3-1.1ubuntu3.6, SHA-256
  `b13e52a43f18e8fdb49e37150cd20c14ec79b42aead1facc5006cb217122a777`.
- `libbsd-dev` 0.12.1-1build1.1, SHA-256
  `d5de7e2205fc68167ecd07bc4fc0f65b53610ce48d514bf097cc076ceefa4b5b`.

The next test-all run will use the unpacked generic, libxml2, and
architecture-specific include roots plus its library directory.

That run progressed until libxml2's `encoding.h` included `unicode/ucnv.h`.
Recursive hard-dependency inspection confirmed `libicu-dev` is a required
dependency of `libxml2-dev`. `libicu-dev` 74.2-1ubuntu3.1 was therefore unpacked
into the same temporary prefix; its SHA-256 is
`612b98f4fcfc6ebc57a1b21c2695174694db5a0b7ff760b5d41032076c792398`.

With libicu available, every server translation unit compiled under the full
warning-as-error profile. The final link then followed Ubuntu's `libbsd.so`
linker script to `-lmd`, whose runtime library existed on the host but whose
unversioned development link did not. The matching packages were added to the
temporary prefix:

- `libmd-dev` 1.1.0-2build1.1, SHA-256
  `c0d71b72b480df106481a0fcb584f8d2ab0f1f165d91ff2d38781e9a28e29285`.
- `libmd0` 1.1.0-2build1.1, SHA-256
  `e5ba01d3c41f256aaf57ec59aa0554857162e3e7f97cdfbff1ed2c0e8d720ee7`.

The prefix's `libmd.so` symlink now resolves to the extracted matching runtime
library.

The next link resolved `-lmd`, but the staged `libgnutls.so` symlink still had
no target inside the prefix. The linker therefore selected `libgnutls.a` and
reported its expected unresolved Nettle, GMP, and ASN.1 dependencies. An audit
of all dangling development-package symlinks found the same incomplete pairing
for libxml2, ICU, and ncurses/tinfo. The matching installed-version runtime
packages were downloaded, hash-checked through APT, and extracted:

- `libgnutls30t64` SHA-256
  `b8944756260c5ea6b7fac019745f931f905aa7a4df0763676b1b25c911b692e2`.
- `libxml2` SHA-256
  `bfd07c01d6e5ab3e327f3ca5819409b1914bbfb3f1a016d53e4dabd5f96143bb`.
- `libicu74` SHA-256
  `c9a70989678660eed9a1e904c74fa043da8bec8e2036856fc16e31ced79b04f8`.
- `libncurses6` SHA-256
  `59e8527dfd14473393960bfb7c69566f1ad9d576423421dcb8b8dcdbb3e7fca1`.
- `libncursesw6` SHA-256
  `579c9488755d1ddd70f66480cafd2f68776f68314d15d9cf4b5449d0de0fc968`.
- `libtinfo6` SHA-256
  `b67a6df2bdab61d273eabebd208bb6a3fb12618a390a067e1ee305a409ff03d9`.

The development symlinks for the direct build libraries now resolve to their
matching shared-library files within the prefix.

With those targets present, the full maintained build completed and the
regression runner executed all 385 Python tests in 417.35 seconds. It reported
369 passes and 16 failures. Eleven are the Redis live suites, and the host has
neither `redis-server` nor `redis-cli`. Four are consequences of using a local
dependency prefix on a test suite designed for system-wide packages: the
WebSocket harness adds only `/usr/include/cjson`, while three isolated
flat-file runtime tests intentionally construct a minimal environment that
does not carry `LD_LIBRARY_PATH`. The remaining failure was `ETXTBSY` while a
parallel test attempted to replace `bin/tests/flatfile-combat/server/dms_new`;
both combat journeys build to that same path, so this is being reproduced
after the dependency repair to distinguish a real parallel-test defect from a
one-off collision.

APT-authenticated Redis 7.0.15 executables are now staged in a separate
temporary prefix. Package hashes are:

- `redis-server` 5:7.0.15-1ubuntu0.24.04.4 SHA-256
  `ac40bf48eefdbe285e62e3f183f95af5c3145bf141a545cb6de291dc51f5f0c0`.
- `redis-tools` 5:7.0.15-1ubuntu0.24.04.4 SHA-256
  `4e5afacdbaccf14d875ed61194a411d57c679744afd9be1946b481c2dbb2b425`.
- `liblzf1` 3.6-4 SHA-256
  `253182d4718271565d6c24125f27dd93da56fce34b34d8226615cb3553bbe73f`.
- `libjemalloc2` 5.3.0-2build1 SHA-256
  `0cccf7701765b5a633b964ed551c2bd1c074cad17daea8fa20a4e9cad0adb5a8`.

The staged `redis-server` and `redis-cli` both launch and report version
7.0.15. They will be used only by their isolated live test fixtures; no
production Redis state or service is involved.

Focused dependency-repair results:

- WebSocket parser/output harness: pass.
- Flat-file client-free build, health, boot, missing-world controlled failure,
  and clean shutdown preflight: pass.
- Full-world player and floor-item process-restart journey: pass.
- Combat, player death, corpse recovery, save, and reconnect journey: pass.
- Chaos new-character bag and generated class-kit journey: pass.
- All eleven Redis live suites: pass (`redis_live_failures=0`).

The two gameplay journeys both called the same builder and wrote
`bin/tests/flatfile-combat/server/dms_new`, even though the root runner executes
them concurrently. The observed `ETXTBSY` is therefore a real regression-suite
race. The builder now uses a per-process output beneath `bin/tests/`, and a
focused source-contract test protects that isolation.

The new isolation contract passes. The Chaos and combat journeys were then
launched simultaneously, each performed its full server build and runtime
journey, and both exited zero (`parallel_combat_status=0`,
`parallel_chaos_status=0`). This reproduces the root runner's concurrency while
proving the shared-binary race is removed.

The complete canonical `make test-all` gate was then repeated with eight
workers. All 386 Python regression tests passed in 481.64 seconds, including
the four full-server flat-file journeys and all Redis live tests. The target
continued into `tests/async/run_signal_handlers.sh` and exited zero.

The `make test-db` recipe and all ten scripts were inspected before execution;
each creates and destroys its own MySQL 8 container and addresses MySQL only
through `docker exec` or a Docker-published loopback port. The complete gate
passed: account rewards, corpse persistence, persistence contracts, lifecycle
archive, personal-data export, account erasure, immutable migration ledger,
lookup dataset, runtime compatibility, and the legacy migration suite. The
legacy suite applied all 143 steps with zero failures, then passed replay,
bootstrap-schema equivalence, and runtime metadata/engine/collation/index/FK
compatibility checks.

The mandatory stopped-state clean build then ran `make clean-all` followed by
`make`. Cleanup removed reproducible files under `bin/`, generated combined
world data, and tool/cache outputs; it did not touch configuration or runtime
data. The server was compiled from zero objects with the full hardening and
warning-as-error profile, then the area editor and every area generator/tool
were rebuilt. The complete command exited zero.

## Production artifact qualification

- Ran `./scripts/cycle_mud.sh --production --check-config`: pass; it loaded
  `.env` without printing secrets and validated the explicit
  `mariadb-primary` configuration.
- Built from zero production-profile server objects with
  `make -C src PERSISTENCE_BACKEND=mariadb BUILD_PROFILE=production clean`
  followed by the matching build target. Every translation unit used the full
  hardening/warning-as-error profile, `-D__NO_TESTS__`, and no test-mode define.
  The link and executable-mode step completed successfully.
- Build stamp: `mariadb/production`.
- `bin/server/dms_new` SHA-256 before promotion:
  `696e20f5696284a628e05eb499ff0b452e57d36cca549f84bdc28d38602d7a48`.
- An exact `strings` search found no `TEST_MUD 1` marker.
- `readelf` showed the expected MariaDB, cJSON, hiredis/hiredis SSL, TLS,
  XML, zlib, and system dependencies. `ldd` under the production service's
  durable `LD_LIBRARY_PATH` resolved every direct and transitive dependency;
  none were `not found`.
- Focused production contracts passed: all six production-service tests,
  legacy-profiler opt-in, supported MariaDB/client-free builds, and
  worktree-aware launcher safety.
- Repeated production configuration preflight: pass.
- Ran the production database's read-only
  `migrations/verify_runtime_compatibility.sh`: migration ledger, schema
  metadata, engines, collations, indexes, and foreign keys all verified. No
  migration or schema write was performed.

## Live production deployment

- Recorded the pre-start boundary at `2026-09-02 13:19:48 UTC`, including the
  MUD unit state, closed production listeners, and inode/size/mtime for every
  then-current regular file under `logs/log/`.
- Started the enabled user unit `duris-mud-production.service`. The unit became
  active at `13:19:54 UTC` with zero restarts. The launcher repeated its
  production configuration and database compatibility checks, created the
  normal authoritative-state MySQL backup, rebuilt all 350 areas and lookup
  data, promoted the qualified binary, and started the game.
- The game reported `Boot completed in: 3861 milliseconds` followed by
  `Entering game loop`.
- Supervisor PID: `3575540`, exact command
  `/home/duris/duris/scripts/cycle_mud.sh --production`.
- Game PID: `3575793`, exact executable
  `/home/duris/duris/bin/server/dms`, cwd `/home/duris/duris`.
- The promoted `bin/server/dms` retains stamp `mariadb/production` and the exact
  qualified SHA-256
  `696e20f5696284a628e05eb499ff0b452e57d36cca549f84bdc28d38602d7a48`.
- That PID owns the expected listeners: public plain telnet `7777`, public TLS
  telnet `4001`, and loopback-only WebSocket/health `4050`.
- MariaDB, Redis, Cloudflare Tunnel, and the MUD service are all active.
- Local and public health both returned exactly
  `{"status":"healthy","persistence":"ready"}`.
- `mud.duris.sbs:4001` completed a hostname-verified TLS 1.3 handshake using
  `TLS_AES_256_GCM_SHA384`; certificate verification and peer-name verification
  both passed.

## Staff smoke and log soak

The smoke client loaded the configured account, password, and character only
inside its process. None were placed in command arguments, printed, or written
to this ledger.

- The first protocol attempt assumed the TLS listener would show the
  terminal-type prompt. This listener instead immediately emitted its ANSI
  greeting, so the client timed out before sending credentials and closed.
- The next two attempts reached the password step but used a readiness-based
  SSL loop which failed to drain bytes already buffered inside the SSL layer.
  Neither selected nor entered the character; both sockets were closed.
- A blocking diagnostic proved authentication was succeeding and receiving the
  news page. The database-backed news is about 19 KB, while the legacy socket
  output window emits about 16 KB, so the trailing visible `PRESS RETURN`
  marker is truncated even though nanny state has advanced to
  `CON_ACCT_RMOTD`. This is existing behavior and is how the successful
  pre-burn-in shutdown login was able to continue by sending Return.
- The corrected client advanced from that verified state, selected the
  configured staff character, confirmed selection, and reached a stable game
  prompt over the publicly verified TLS transport.
- The randomized, non-destructive inspection commands were exactly: `score`,
  `equipment`, `time`, `who`, `commands`, `weather`, and `inventory`.
  Respective response sizes were 792, 84, 309, 165, 1512, 50, and 195 visible
  bytes. Every response was nonempty and none contained an unknown-command or
  authorization rejection.
- `quit` returned the descriptor to the account menu, option `0` completed the
  account disconnect, and the client observed the clean farewell.
- Health remained passing and the service retained the same supervisor/game
  PIDs with zero restarts throughout login, commands, logout, and soak.

Monitoring covered the systemd journal and every regular file created in the
new `logs/log/` run. `logs/duris-console.log` predates this run and remained
unchanged because the user service sends console output to the journal. The
startup journal contains one host/container limitation notice saying
`ProtectHostname=yes` could not create a UTS namespace; systemd explicitly
ignored only that namespace setup, and all other unit hardening and runtime
behavior continued.

The error-pattern scan found only TLS read/close records in `logs/log/comm`
at the exact times the diagnostic clients deliberately closed without a TLS
`close_notify`; these are fully attributable to the smoke-client iterations.
Expected positive matches showed `mariadb-primary`, initialized MySQL,
started locker persistence, Redis connection/cache activity, and two bounded
world-recovery floor handoffs acknowledged on their first attempts. A second
scan found no alert, invalid, missing, denied, refused, dropped, stalled,
deadlock, overflow, underflow, retry, unhealthy, degraded, or violation
markers.

## Completion snapshot

The final snapshot was taken at `2026-09-02 13:29:06 UTC`, more than nine
minutes after service activation and after the staff logout:

- Local health: `{"status":"healthy","persistence":"ready"}`.
- Public Cloudflare health: `{"status":"healthy","persistence":"ready"}`.
- Public TLS: TLS 1.3, trusted certificate, verified hostname
  `mud.duris.sbs`.
- MUD service: active/running, main PID `3575540`, `NRestarts=0`, successful
  main status.
- Game process/listeners: PID `3575793` still owns `7777`, `4001`, and `4050`
  with the intended public/loopback bindings.
- MariaDB, Redis, Cloudflare Tunnel, and MUD user services: all active.
- Runtime artifact: stamp `mariadb/production`; SHA-256 unchanged at
  `696e20f5696284a628e05eb499ff0b452e57d36cca549f84bdc28d38602d7a48`.
- The final risk-pattern count did not increase during the ending health/TLS
  probes; it remained limited to the seven already-attributed diagnostic TLS
  closes.
- Final repository checks: `./scripts/format.sh --check`, the focused
  parallel-build isolation contract, Python bytecode compilation for both
  touched test files, and `git diff --check` all passed.

The worktree intentionally remains uncommitted. It contains the focused
parallel-test output-isolation fix, its regression contract, and this ledger.
The pre-existing `.agents/skills/burnin/SKILL.md` modification is still present
and untouched. No credentials, local environment file, production data, log,
backup, generated world output, or compiled artifact was added to version
control.

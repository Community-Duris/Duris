# prod-install.md - Journal of Production Installation

## 2026-09-09

### Deployment outcome: COMPLETE

- Production is active and enabled under `duris-mud-production.service` with zero
  systemd restarts. The final server binary is the qualified
  `mariadb/production` artifact.
- `mud.newduris.com` serves the Duris greeting on plain TCP 7777 and
  hostname-verified native TLS 7778. HTTPS health returns HTTP 200 with
  `healthy`/`ready`; the allowed WebSocket origin upgrades with HTTP 101 and an
  unlisted origin is rejected with HTTP 403. Port 4050 remains loopback-only.
- A post-cutover external check found that Cloudflare still proxied the `mud` A
  record, which made raw TCP 7777/7778 time out even though the origin was healthy.
  Changed only that record to DNS-only at 2026-09-09 10:05 UTC. Cloudflare's
  authoritative DNS now returns `178.156.165.10`, and five independent external
  probes connect successfully to `mud.newduris.com:7777`.
- The configured staff account completed the final authenticated gameplay smoke,
  including staff-only access and an explicit save/logout. Final application logs
  are owner-only, the warning-or-higher service journal is empty, and the repaired
  SQL tracer appended no trace-burst lines.
- The final restart backup `db/Backup/1788946842.sql.gz` is a valid 533,348-byte,
  owner-only gzip with the core account and player schema markers. The protected,
  checksummed pre-deployment rollback binary remains available outside `bin/`.
- Completed at 2026-09-09 09:42:54 UTC. The deployment is live; the prominent test
  infrastructure finding immediately below remains required follow-up work.

### ⚠️ REVIEW REQUIRED: the regression suite has an excessively slow end-to-end tail

> **This is a real test-infrastructure performance gap, not normal assertion
> runtime. Do not miss it when reviewing the production work.**

- The ordinary phase completes 419 regressions in parallel, but the final six
  resource-intensive tests are serialized and each independently recompiles almost
  the entire C++ server into a unique temporary output tree, generally with only
  `-j2`. The repeated clean compilation, rather than the game assertions, dominates
  the wall-clock time.
- Measured in the current final qualification run: account recovery took 201.90
  seconds, flat-file boot preflight took 189.63 seconds, and the Chaos new-character
  kit journey took 409.37 seconds. The combat journey then took 478.41 seconds,
  including another full isolated compile, and full-world boot took 276.56 seconds.
  The item-movement sanitizer runtime took 63.09 seconds. The complete 425-test gate
  took 1,968.63 seconds despite the ordinary 419-test phase running in parallel.
- Serializing these tests was necessary to stop concurrent complete builds from
  exhausting their fixed timeouts, and no timeout was widened. That scheduler repair
  prevents false failures, but it exposes the underlying duplication: the tail alone
  consumes roughly 20--30 minutes on this host even when every assertion passes.
- Required follow-up after production deployment: group tests by compatible build
  configuration, build each test binary once, reuse it read-only across journeys,
  and retain a separate temporary state/data directory per test. Preserve the full
  behavioral coverage, compiler-hardening variants, and current timeout ceilings
  while eliminating redundant whole-server builds. Add timing evidence to prove the
  improvement rather than merely moving or hiding the delay.
- This finding did not weaken or waive the deployment gate. The complete
  qualification finished unchanged before production cutover, including every
  subsequent database, build, and runtime check recorded below.

### Initial host preparation

- Installed the complete non-database build/runtime toolchain and optional tooling: C++ libraries, Redis, Docker Engine with Compose/Buildx, clang-format, GDB, Valgrind, Ruby/RMagick, ImageMagick, ShellCheck, jq, and socat.
- Added `duris` to the `docker` group and enabled the repository Git hooks.
- Verified Docker/Redis/tool availability, formatting, and a successful full build of the server, area editor, and area generators.
- Started the non-database regression gate, then stopped it on request after 155/425 passing checks and no failures.
- At this stage, made no database queries, migrations, imports, or configuration changes.

### Production database provisioning

- Confirmed the host's existing MySQL 8.0.46 service is supported by the repository; MariaDB 12.3 is not yet qualified, so no second database engine was installed.
- Confirmed the existing application account was restricted to the old `duris_prod` schema and that the new database and account names were unused.
- Created an isolated `duris_game_prod` database and schema-scoped `duris_game`@`127.0.0.1` account with a generated password stored only in the owner-readable `.env`.
- Built the schema in a temporary `duris_game_seed` database, adopted the sealed fresh baseline, applied all 11 immutable migrations, verified the 177-table runtime contract, and promoted that verified schema into `duris_game_prod`.
- Ran the help-content dry run and live import, producing 2,156 non-empty `pages` rows and the three required `mud_info` records.
- Updated `.env` for the loopback production database and exact target allow-list, disabled unconfigured Redis, corrected shell parsing, and set mode `0600` without disclosing credentials.
- Created the player-save and critical-command journal directories with mode `0700`.
- Passed the production configuration preflight, runtime schema/fingerprint verification, migration-head check (`0011_player_death_disposition`), schema-scoped grant check, and `mysqlcheck`.
- Removed the temporary seed database and account after verification; the existing `duris_prod` database was not modified, and the game service was not started.
- Confirmed non-interactive passwordless sudo was available for subsequent administration.

### Redis provisioning

- Configured Redis before game-service setup: kept it loopback-only, disabled anonymous access, added five least-privilege Duris ACL identities, enabled AOF persistence with one-second fsync, and persisted the required kernel memory-overcommit setting.
- Enabled the production Redis settings in the owner-only `.env` with namespace `duris:production:default`, generated distinct credentials and HMAC secrets, and kept plaintext credentials out of Redis configuration.
- Verified the isolated and deployed ACL boundaries, authenticated all five identities, and proved an AOF-backed test write survived restart before deleting it through the maintenance identity.
- Found 18 untouched, expiring `geo:ip:*` keys from the prior site/server cache; their values were not read, and Redis dropped them when it initialized the clean AOF base instead of importing the old RDB.
- Did not start the game service or modify the production database during this step.

### Deployment qualification and cutover preparation

- Reconfirmed the production role, owner-only `.env`, loopback MySQL/Redis targets,
  exact database and Redis allow-lists, and an idle game state with no MUD process,
  listener, or installed Duris system service.
- Re-ran the read-only production migration manifest inspection and runtime
  compatibility verifier. The immutable head remains
  `0011_player_death_disposition`, and the production schema passed its migration,
  metadata, baseline, engine, collation, index, and foreign-key contract checks.
- Re-ran the offline production launch preflight successfully; no database or Redis
  state was changed.
- Confirmed `mud.newduris.com` resolves to this host's public IPv4 address, nginx has
  an HTTPS/WebSocket reverse proxy for the hostname, and its ECDSA certificate is
  valid through 2026-12-05.
- Confirmed MySQL, Redis, and Docker are active and enabled, with adequate disk and
  memory available. The MUD production service remains stopped pending a complete
  clean qualification pass.
- Identified and retained as open cutover findings: the production binary has not
  yet been built/stamped, the plain-text listener is still configured loopback-only,
  the WebSocket origin allow-list is still a placeholder, host firewall policy has
  not been enabled, the systemd production unit is not installed, and the configured
  smoke-test account fields are empty. These must be corrected and verified before
  deployment can be called complete.
- The first complete 425-test regression attempt exposed one portability defect:
  `test_log_directory_runtime.py` invoked MariaDB-only `mariadb_config` on this
  supported MySQL host. Updated the harness to use the cross-family `mysql_config`
  interface already used by the rest of the suite, and corrected the dependency
  metapackage to accept either the distribution's default MySQL development package
  or MariaDB's compatibility package instead of forcing conflicting client families.
- The focused log-directory and dependency-manifest regressions pass after the fix;
  the corrected metapackage also builds, and an APT resolution simulation preserves
  the installed MySQL family without removals or replacement.
- Repeated the read-only production schema gate after the repair, ran `make clean-all`,
  and completed a fresh build of every maintained target. The server compiled under
  `PERSISTENCE_BACKEND=mariadb BUILD_PROFILE=production` with the full hardened warning
  profile, no diagnostics, and the required `mariadb/production` artifact stamp.
- Corrected the production listener configuration before cutover: plain telnet now
  targets only the host's verified public IPv4 address on port 7777, native TLS is
  explicit on port 7778, the WebSocket/health listener remains loopback-only, and the
  browser Origin allow-list contains only `https://newduris.com` and
  `https://www.newduris.com`.
- Confirmed the production database was genuinely fresh with zero accounts and zero
  characters. Generated the configured smoke identity locally and stored all three
  required fields only in the owner-readable `.env`; its account and character still
  need to be created through the live game flow after the qualified service starts.
- Installed the current `mud.newduris.com` Let's Encrypt certificate into a protected
  runtime TLS directory and linked the ignored game certificate paths to it. A
  root-owned Certbot deploy hook validates the hostname, remaining validity, and
  certificate/key pairing, atomically refreshes the owner-readable runtime copies,
  and health-checks the game after renewal-triggered restarts. Certificate metadata,
  the active renewal timer, the initial copy, and Certbot's complete simulated renewal
  against the configured nginx authenticator all passed validation.
- The repaired canonical regression gate completed with all 425 Python tests passing
  and the native signal-handler check passing. The first `make test-db` invocation
  then stopped before creating a container because this existing login session did
  not inherit the `docker` supplementary group added during host setup. Docker itself
  was active and enabled, its socket retained secure `root:docker` mode `0660`, and a
  group-scoped Docker probe passed. The unchanged database matrix was restarted under
  that configured group; socket permissions were not weakened.
- Nine of the ten isolated database suites then passed. The final legacy-migration
  wrapper timed out while hiding every client diagnostic even though its MySQL 8
  container was healthy. A retained reproduction proved the installed MySQL client
  rejects that wrapper's hard-coded MariaDB-style `--skip-ssl` option. Replaced the
  remaining hard-coded loopback uses with the repository's capability-selected
  MySQL/MariaDB option, shared that selection with the three Python import/audit
  clients, and added focused integration contracts. The complete legacy migration,
  replay, bootstrap-equivalence, and runtime-compatibility wrapper now passes.
- Docker 29 also logged an auto-removal race because each detached database wrapper
  combined `docker run --rm` with an explicit cleanup trap. Removed only the redundant
  auto-remove flag from all 18 affected isolated wrappers; every trap remains solely
  responsible for deleting its uniquely named container, and a regression contract
  prevents the race from returning.
- The newly exercised legacy personal-locker repair rehearsal then found that an
  intentional zero-row pre-mutation dump is empty under MySQL's compact dump mode and
  was rejected as a missing backup. Added a valid SQL comment header so the protected
  artifact represents the empty pre-state without fabricating a row. Its safety
  contract and full isolated 21-item preservation/idempotency rehearsal now pass.
- Repeated the entire qualification sequence after the final repairs. The production
  schema gate, formatting check, and diff-integrity check passed; all 425 Python
  regressions and the native signal-handler test passed; and all ten isolated MySQL 8
  suites passed, including the complete 143-step legacy migration and replay. Every
  test container was removed, and the Docker daemon recorded no warning-or-higher
  entries during the final database run.
- Completed another clean build of every maintained target after those gates. The
  resulting PIE server binary has all dynamic dependencies resolved and carries the
  exact `mariadb/production` stamp; production schema, launch configuration,
  formatting, and diff-integrity checks all remained clean.
- The first explicit pre-cutover database backup exposed a MySQL 8 diagnostic: its
  logical dump tried to inspect global tablespace metadata even though the deliberately
  schema-scoped account has no `PROCESS` privilege. The client nevertheless returned
  success and the existing archive checks passed. Added capability-selected
  `--no-tablespaces` use so no broader database privilege is required, and added a
  regression proving the flag is required when advertised while preserving MariaDB
  compatibility. The focused backup test passed, and a fresh protected production
  backup then completed without warnings. Because this changed tracked deployment
  code, the complete qualification sequence will be repeated before cutover.
- The clean post-repair regression rerun exposed test-runner oversubscription rather
  than a product assertion failure: three complete flat-file server builds exhausted
  exact 180/600-second subprocess limits, and an unrelated sanitizer harness compile
  exhausted its exact 120-second limit while all four outer workers were compiling.
  Centralized the fix in the regression runner by serializing the five tests that
  build complete isolated flat-file servers after the bounded parallel phase. Added a
  harness contract and documented the resource boundary; focused and complete gates
  must pass with the normal four-worker setting before cutover resumes.
- The runner contract and previously starved sanitizer harness passed after the
  scheduler repair. Each failed full-build test was then repeated alone: combat and
  corpse recovery, client-free boot/health/shutdown, and all Warrior, Monk, Thief,
  and Sorcerer Chaos kit create/save/restart journeys passed without extending any
  timeout. This confirms the finding was outer-runner resource contention; the
  complete repaired gate remains required before cutover.
- The first full scheduler verification reached 419 passing tests with no failures,
  but the large item-movement sanitizer compile still consumed 102 of its 120-second
  limit alongside the ordinary parallel group. Stopped that non-final run before the
  already-focused whole-server phase and added this sixth known compiler-heavy test
  to the serialized resource set. No timeout was widened; the final complete gate
  will exercise the stricter scheduler from its first test.
- The final repaired `make test-all` gate passed all 425 Python regressions and the
  native signal-handler check. All 419 ordinary tests passed in the four-worker
  phase, followed by all six compiler-heavy tests in the serialized phase. The
  formerly marginal sanitizer harness fell from 102 to 63 seconds, boot preflight
  from a 600-second timeout to 187 seconds, and full-world boot from 567 to 284
  seconds, providing real execution margin without weakening any timeout.
- Repeated the complete isolated MySQL 8 qualification after the scheduler repair.
  All ten suites passed, including every 143-step legacy migration, immutable replay,
  bootstrap-schema equivalence, and runtime metadata/engine/collation/index/foreign-key
  check. Every disposable container was removed, and the Docker daemon recorded no
  warning-or-higher event during the run. Formatting and diff-integrity gates remain
  clean.
- Completed the final post-gate `make clean-all` and full maintained production build.
  The server linked without diagnostics, carries the exact `mariadb/production` stamp,
  has no unresolved dynamic library, and is a PIE with non-executable stack, RELRO,
  and immediate binding. The production configuration preflight and live runtime
  schema compatibility verifier passed again.
- Created a fresh 504 KiB production logical backup through the repaired capability
  path with no diagnostic. The gzip stream, core account/player schema markers,
  owner/group, and mode `0600` were verified before service installation.
- The first compound pre-install stop-state command was rejected by the local command
  safety guard because it unnecessarily created and removed a temporary process-list
  file; it did not execute or change host state. Rewrote the check with anchored
  process matching and no temporary file. It verified no installed service, MUD
  process, or 7777/7778/4050 listener before installation.
- Installed the rendered system `duris-mud-production.service` without enabling or
  starting it. Systemd parsed the unit without diagnostics; it retains the exact
  production launcher, `duris` identity, owner-only umask, empty capability set,
  namespace/kernel/device protections, and a systemd security exposure score of 3.1
  (`OK`).
- Enabled the host UFW firewall with default-deny incoming, allow outgoing, low-level
  logging, and only SSH, nginx HTTP/HTTPS, plain MUD 7777, and TLS MUD 7778 inbound
  rules for IPv4/IPv6. UFW is enabled for boot and the existing SSH session remained
  healthy. The service remains stopped until the next explicit start step.
- The first production service start reached database and world boot, but MySQL 8
  emitted five `MYSQL_OPT_RECONNECT` deprecation warnings while the connection pool
  initialized. Stopped the service immediately, confirmed all three listeners and the
  server process were gone, and disabled boot-start while repairing the connector
  contract. MySQL documents both that reconnect defaults off and that invoking the
  deprecated option emits a warning even when setting it false; the production path
  will omit that call while MariaDB builds retain the supported explicit disable.
  Full qualification and a clean boot-log check are required again before cutover.
- The first focused source-contract run exposed an assertion-selection mistake in
  the new regression: its backwards section helper selected the later TLS guard,
  which uses the same connector-family condition. Changed the test to select the
  first guard explicitly; no runtime code was changed by this test-only correction.
- The corrected reconnect-policy contract and boot-log hygiene test passed, and the
  repaired MySQL-linked production binary compiled and linked with the full warning
  profile. Repeated the canonical gate from its initial build: all 425 Python
  regressions passed in 1,935 seconds and the native signal-handler check passed.
  All 419 ordinary tests and each of the six serialized whole-server/compiler-heavy
  journeys passed without weakening a timeout.
- Repeated all ten isolated MySQL 8 suites after the reconnect repair. Account rewards,
  corpse persistence, persistence convergence, lifecycle archive, personal-data export,
  account erasure, the immutable ledger, lookup data, runtime compatibility, and the
  complete 143-step legacy migration/replay/bootstrap-equivalence path all passed.
  A direct post-run container listing again encountered this shell's known stale Docker
  group membership; the immediate group-scoped retry confirmed no container remained.
  The Docker daemon logged no warning-or-higher event during the final matrix.
- Completed the post-gate clean production build of every maintained target. The
  server has the exact `mariadb/production` stamp, all dynamic dependencies resolve,
  and the ELF is PIE with non-executable stack, RELRO, and immediate binding. Production
  configuration, live schema compatibility, formatting, diff integrity, and the
  installed systemd unit all passed their checks.
- Created and validated the final protected production backup
  `db/Backup/1788941107.sql.gz` (523,411 bytes, `duris:duris`, mode `0600`) with the
  required account and player schema markers. Also stored a checksummed pre-start
  binary/stamp rollback artifact under
  `/home/duris/.local/state/duris-prod-install-RP43bS7t`. Its directory initially
  inherited the parent's setgid bit while granting no group access; cleared that bit
  and verified exact mode `0700`, owner-only files, and the binary checksum.
- Enabled and started the fully requalified production service at 2026-09-09
  08:06:35 UTC. It reached the game loop in 3.489 seconds with zero systemd restarts
  and no warning, error, fatal, assertion, deprecation, corruption, denial, or
  traceback match in either the complete final-start journal or any fresh application
  log. The server owns the exact public 7777/7778 and loopback-only 4050 listeners;
  all newly created application logs are `duris:duris` mode `0600`.
- Exercised account and character creation over the public native-TLS game protocol.
  The client outlived its caller's first 30-second output window while completing the
  interactive flow, so no duplicate request was sent; the bounded client subsequently
  exited and the database contains exactly one account, one player, and one active
  mapping. The first identity-specific verification query used the obsolete column
  name `accounts.acct_name` and failed without changing data. Checked the authoritative
  production schema, corrected the probe to `accounts.account_name`, and retained the
  query mistake here before proceeding with the guarded promotion.
- The first guarded staff promotion was applied while the live server still held
  creation-era deferred checkpoints. The initial authenticated smoke passed all mortal
  commands but timed out waiting for the staff-only `users` header; a diagnostic rerun
  returned the ordinary prompt and the database showed that a later checkpoint had
  restored level 1 at save revision 50. Stopped the service cleanly, confirmed the MUD
  process and all listeners were absent, re-applied the one-row promotion while fully
  quiesced, and proved it remained stable before restart. This removes the stale-save
  race instead of repeatedly overwriting live state.
- The corrected offline promotion survived boot and the full authenticated native-TLS
  smoke passed: login, character selection, `look`, `time`, `weather`, `score`,
  `inventory`, `equipment`, `exits`, `who`, the staff-only `users` command, live health,
  explicit save, quit, and account logout. A delayed database check proved the staff
  invariants and a newer save revision both persisted; the restart journal and all
  application logs remain clean.
- Verified every public surface through `mud.newduris.com`: port 7777 returned the game
  greeting, port 7778 completed a hostname- and chain-verified TLS 1.3 connection with
  the game greeting and 87 certificate days remaining, HTTPS health returned exactly
  `healthy`/`ready`, HTTP redirected to the same HTTPS path, the allowed production
  WebSocket origin upgraded with HTTP 101, and an unlisted origin was rejected with
  HTTP 403.
- Performed a post-smoke service restart and proved the staff row, staff-only command,
  explicit save, and newer revision all survived. The new process was healthy on all
  three listeners with zero restarts, and its 532,429-byte restart backup was a valid
  owner-only gzip containing both core schema markers. Six 30-second checkpoints then
  completed a three-minute soak with an unchanged PID, zero restarts, three listeners,
  and exact healthy/ready responses from both the loopback and public endpoints.
- The final credential-leak audit found that the live `DURISWEB_SECRET` still equaled
  the public `.env.example` placeholder. Rotated it immediately to a random 256-bit
  value, left the previous-secret window empty, and restarted cleanly. A real public
  WSS challenge-response authenticated with the new key and rejected the former
  placeholder. Added production launcher and direct-runtime guards against the public
  placeholder and keys shorter than 32 characters, including previous keys, plus
  behavioral regressions and operator documentation. The focused launcher, runtime
  HMAC, source-contract, WebSocket hardening, configuration, formatting, and diff checks
  pass; the complete gates and final rebuilt-binary cutover are required again.
- The first WSS auth probe omitted the protocol's required top-level `type: "cmd"` and
  timed out while the server correctly ignored the incomplete envelope. It also exposed
  that every command example in the DurisWeb API reference omitted that field. Corrected
  all five examples, reran the probe under fail-fast handling, and verified both the
  positive rotated-key and negative placeholder-key paths.
- A runtime-log mode audit initially grouped the tracked `logs/log/.gitignore` placeholder
  with application output and reported its ordinary repository mode `0664`. Refined the
  audit to non-dot runtime files; all ten application logs are `0600`. Likewise, a first
  Redis PING used the intentionally non-production generic identity and was rejected.
  Retested the actual world, presence, cache, donation, and maintenance ACL identities;
  every role authenticated, selected the configured database, and returned PONG.
- Repeated the canonical qualification gate after adding the production DurisWeb-secret
  guards. All 425 Python regressions passed in 1,968.63 seconds and the native
  signal-handler check passed. The serialized tail measured 201.90 seconds for account
  recovery, 189.63 for boot preflight, 409.37 for the Chaos kit journey, 478.41 for the
  combat journey, 276.56 for full-world boot, and 63.09 for the sanitizer-backed item
  prompt runtime. The prominent finding at the top records the redundant-build root
  cause and required post-deployment remediation; no timeout or deployment gate was
  weakened.
- Repeated all ten disposable MySQL 8 qualification suites after that complete gate.
  Account rewards, corpse persistence, persistence convergence, lifecycle archive,
  personal-data export, account erasure, the immutable migration ledger, lookup data,
  runtime compatibility, and the complete 143-step legacy migration/replay/bootstrap
  equivalence path all passed. No test container remained, and Docker recorded no
  warning-or-higher journal event during the matrix.
- A post-build profile search included the nonexistent shell glob `src/*.mk`, causing
  `rg` to report that no such path exists. This was an operator inspection mistake,
  not a build diagnostic; repeated the inspection against the existing `src/Makefile`
  and build guide. They confirm that `-Og` is the documented hardening baseline and
  that `BUILD_PROFILE=production` correctly excludes `-DTEST_MUD` and isolates the
  production objects.
- Performed another `make clean-all` and fresh build of the server, area editor, and
  six area generators with the final secret guard. Every maintained target built
  without compiler diagnostics. The staged server has the exact
  `mariadb/production` stamp, all 47 dynamic dependencies resolve, and its ELF is PIE
  with a non-executable stack, RELRO, and immediate binding. Production configuration,
  authoritative live-schema compatibility, formatting, diff integrity, and installed
  systemd-unit parsing all passed again.
- Created and validated a fresh pre-cutover logical backup
  `db/Backup/1788946313.sql.gz` (533,046 bytes, `duris:duris`, mode `0600`); its gzip
  stream and core account/player schema markers are intact. The clean-all gate
  intentionally removed the stopped runtime copy under generated `bin/`, so the
  external pre-start rollback artifact remains the authoritative prior binary. Its
  directory is mode `0700`, its files are owner-only, its stamp is
  `mariadb/production`, and its recorded SHA-256 checksum passed again.
- Started the fully qualified staged binary at 09:33:44 UTC and verified that its
  promoted runtime checksum exactly matched the candidate. A first combined
  readiness command was rejected before execution because it used prohibited
  temporary-file cleanup; replaced it with an in-memory probe. The service remained
  active and owned public 7777/7778 plus loopback-only 4050 with zero restarts.
- The first combined public HTTP/WebSocket check made two operator mistakes: it
  requested a nonexistent `/ready` route even though `/health` already carries both
  health and persistence readiness, and its WebSocket nonce decoded to 17 bytes
  instead of RFC 6455's required 16. The service correctly returned HTTP 200 from
  `/health`, and the corrected WebSocket request returned 101 for the production
  origin and 403 for an invalid origin. A separate attempt to inspect nginx with
  `sudo rg` failed because sudo's restricted path omits `rg`; it changed no state and
  was unnecessary once the real public probes passed.
- The broad application-log scan initially treated every metadata field named
  `error_code` as an error and produced excessive output. It nevertheless exposed a
  real observability problem: legacy callers consume their result before the next
  defensive drain, and MySQL reports code 2014 when that drain calls
  `mysql_store_result()` a second time. Queries all had `outcome=success`, but the
  tracer misclassified the already-consumed state and continuously reset a 100-query
  debug burst, creating needless production log growth.
- Repaired that trace flood without changing query behavior: commands-out-of-sync
  from an already-consumed defensive drain is now visible only when `SQL_TRACE` is
  explicitly enabled and never activates a production burst. The focused event-loop
  observability regression, formatting, diff integrity, incremental hardened
  production compile, and relink all passed without diagnostics.
- Restarted onto the repaired artifact at 09:40:41 UTC. The first 20-second readiness
  poll ended just before the three listeners became visible; the same process then
  remained active, entered the game loop, and passed direct readiness. Final proof
  showed the authenticated staff smoke passing, public health at HTTP 200, allowed
  WSS at 101, rejected WSS at 403, no new SQL trace lines, no warning-or-higher
  service journal entries, owner-only log modes, exact listener binding, and zero
  systemd restarts. The automatic restart backup was also validated.

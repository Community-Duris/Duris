# Production migration and full burn-in

Status: **complete**. Started: 2026-09-03 UTC. Completed: 2026-09-03 16:36 UTC.
Branch: `master`.

## Goal

Qualify commit `08bb88e7e4d4c4345c4e522f8df13b571139b429` end to end against the
authorized production deployment: safely advance the production schema, run every canonical
regression and isolated database gate, prove a clean build, deploy the production-profile binary,
complete a non-destructive staff smoke test, inspect all current logs, and leave the service healthy.

The owner explicitly authorized the full production burn-in, including migrations and operational
service actions. Credentials and secret `.env` values are never recorded here.

## Starting state

- Checkout: `/home/duris/duris`; branch `master` at `08bb88e7e`.
- The worktree already contained tracked edits to `AGENTS.md` and
  `.agents/skills/burnin/SKILL.md`; both must be preserved.
- Environment role and required database/account fields were checked without printing values.
- Exact production manager: enabled user unit `duris-mud-production.service`.
- Unit working directory: `/home/duris/duris`.
- Unit launch path: `/home/duris/duris/scripts/cycle_mud.sh --production`.
- Initial supervisor PID: `861005`; unit active since 2026-09-03 06:08:43 UTC with zero restarts.
- Initial health check: pass.
- Known-good runtime: `bin/server/dms`, SHA-256
  `8a5b1fe9984c7e237bd078f5218b0c4a5710835cdce38b4e6a6412c369c27de3`, stamped
  `mariadb/production`.
- Read-only compatibility preflight found production at immutable migration head
  `0006_kingdom_realms`; this source requires `0008_statistics_date_index`. Pending migrations are
  `0007_pkill_event_stamp_contract` and `0008_statistics_date_index`.

## Safety and rollback plan

- Identify and snapshot the known-good runtime artifacts before stopping the service.
- Stop only the exact user unit and verify its supervisor, game child, and owned listeners close.
- Create and validate a fresh production database backup while every game writer is stopped.
- Restore that backup into a disposable local clone and qualify the exact pending migrations there
  before production application.
- Preserve the pre-migration backup for database rollback. If a later phase fails, restore the
  database and known-good runtime as needed, then restart through the same production user unit.
- Never route canonical DB tests to the configured game database; `make test-db` uses only isolated
  Docker/MySQL fixtures.

## Operational timeline

All times are UTC on 2026-09-03 unless noted otherwise.

- **14:27** — Created this credential-free live journal at the owner's request.
- **Before 14:27** — Read the burn-in skill, `AGENTS.md`, `README.md`, root `Makefile`, and the
  runbook start/stop procedure; inspected the worktree, service topology, listeners, runtime hash,
  required configuration-field presence, current health, and schema compatibility.
- **Before 14:27** — Confirmed the immutable runner intentionally rejects production targets. The
  production migration path is being resolved without relabeling the target, weakening guards, or
  editing migration history manually.
- **14:27** — Created a 363 MB private rollback bundle at
  `/tmp/duris-burnin-rollback.lS3wmb`. It contains the known-good runtime and build stamp, generated
  world/lookup artifacts, the exact user-service definition, and a Git archive of known-good source
  commit `180b6e4bc`. The copied runtime hash matches the active artifact.
- **14:28:07** — Stopped only `duris-mud-production.service` through its user service manager.
  Supervisor PID `861005`, game PID `861283`, and the `7777`, `4001`, and `4050` game listeners all
  closed; the unit reports a successful stop.
- **14:28:48** — Two remaining sleeping database connections were traced to Node PID `72801`, owned
  by `durisweb-production.service`. Recorded that exact unit and stopped it through the same user
  manager. The configured database then reported zero other connections.
- **14:28:54** — Created and validated the owner-only pre-migration database backup
  `/tmp/duris-burnin-rollback.lS3wmb/database/1788445734.sql.gz` (1,056,611 bytes), SHA-256
  `a72364b0c983f52f2233751b8e8734a7e9665d234831dbad24e1a58a823ebf2b`. The dedicated directory
  avoids pruning or modifying normal retained production backups.
- **14:29-14:31** — Started an isolated MariaDB 10.11 container and restored the exact backup. The
  first readiness probe used `MARIADB_PWD`, which the container client ignored; it made no database
  change, timed out, and was corrected to the standard `MYSQL_PWD` variable. Container-only
  `io_uring`/cgroup warnings were inspected and are expected sandbox fallbacks.
- **14:32** — Compared table and core-row count signatures between stopped production and the
  restored clone; they match. Applied migrations 0007 and 0008 on the clone, passed the complete
  runtime compatibility verifier, replayed the immutable runner as a no-op, and proved postcondition
  `8:0:1` (migration count 8, zero legacy pkill zero-dates, one exact statistics date index).
- **14:33** — Repaired the missing sanctioned production immutable-migration path. The runner now
  requires the exact confirmed target, allow-list membership, a fresh owner-only validated backup,
  secure remote transport, and zero other target connections; it still rejects production baseline
  adoption and any unconfirmed production use. Added focused regressions and documented the operator
  contract in the runbook, database guide, and immutable migration guide.
- **14:34** — Exercised the new guarded production path against a second restore of the production
  backup in the isolated container. Both migrations, ledger advancement, compatibility verification,
  and no-op replay passed with postcondition `8:0:1`.
- **14:35** — Reconfirmed both writer services inactive and zero target connections, then applied
  only immutable migrations 0007 and 0008 to production through the guarded runner. Production
  moved from postcondition `6:0:0` to `8:0:1`; the complete compatibility verifier passed and an
  immediate second runner invocation was a no-op.
- **14:36:28** — Restarted the unchanged `durisweb-production.service` through its exact user unit;
  it is active with PID `1529866` and zero restarts. The MUD remains stopped for canonical gates.
- **14:37-14:48** — The first canonical `make test-all TEST_JOBS=8` pass clean-built the maintained
  targets and generated the world, then reported 392 passes and five failures. All five were
  dependency-prefix portability defects in test harnesses: the WebSocket harness used a noncanonical
  cJSON include, and four isolated server journeys discarded the loader path required by the durable
  dependency prefix. No product/runtime assertion failed.
- **14:50** — Narrowed the repair to those test boundaries: the WebSocket harness now uses the same
  `cjson/cJSON.h` include as maintained sources, and each deliberately minimal isolated-server
  environment preserves only an explicitly configured `LD_LIBRARY_PATH`. Syntax and whitespace
  checks pass; focused runtime reruns follow.
- **14:50-15:07** — All five exact focused regressions passed under the durable prefix: WebSocket
  parser/output; client-free build/health/boot/shutdown; combat/death/corpse/save/reconnect; CHAOS
  starter-kit ownership and reload; and the full-world floor-item process-restart journey.
- **15:08** — Repeated the migration/compatibility gate first, as required after a repair. Briefly
  stopped the exact DurisWeb user unit, replayed the guarded production runner at zero writers (no
  pending migrations), and passed the full verifier at head 0008. Restarted the unchanged web unit;
  its API health reports database and cache healthy. Its auction bridge continues to log expected
  connection refusals while the deliberately stopped MUD WebSocket listener is absent.
- **14:47-14:48 (reconciled at 15:26)** — A production-service start was attempted while the staged
  server still had the canonical gate's development profile. The production launcher failed closed
  before promotion with its explicit `mariadb/production` build requirement. Systemd made four
  policy-driven retries before the exact unit was stopped. No game listener opened and the preserved
  known-good deployed runtime remained available in the rollback bundle.
- **15:08-15:14 (reconciled at 15:26)** — A post-repair `make test-all TEST_JOBS=8` rerun reached
  303/397 passing Python regressions, but its saved log ends without a terminal summary or exit
  record and no associated process remains. This is incomplete evidence and does not count as a
  canonical pass; the full gate will be repeated with explicit exit capture.
- **15:26** — Resumed from authoritative state and classified the previous goal turn as progress.
  Confirmed the production MUD unit inactive with all three game listeners closed, DurisWeb active,
  no burn-in build/test process live, and both the rollback bundle and database backups intact.
- **15:27-15:30** — A full rerun launched from the resumed shell without the previously established
  durable dependency prefix. It completed 370/397 tests and all 27 failures reported missing
  cJSON/hiredis/libbsd development files or missing `redis-server`/`redis-cli`; no behavioral
  assertion failed. This operator-environment run is not counted. Located the intact prefix at
  `/home/duris/.local/opt/duris-deps/usr`, explicitly restored its compiler, linker, loader,
  pkg-config, and executable paths, then passed a compile/link/tool preflight and six representative
  focused regressions: artifact cache, Redis cache live, terminal type, WebSocket runtime, and world
  recovery pipeline.
- **15:31-15:34** — The next explicit-prefix canonical rerun completed 393/397 tests. The only four
  failures were the large client-free/flat-file server builds, all at the same strict compilation
  point: using `CPATH` exposed hiredis's C flexible-array declarations as ordinary project headers,
  so `g++ -Wpedantic -Werror` rejected them. The other 393 regressions, including every live Redis,
  WebSocket, artifact, JSON, and world-recovery case, passed. The dependency include prefix must use
  GCC's language-specific system-header path semantics (`CPLUS_INCLUDE_PATH`), then the affected
  build and the full suite will be repeated.
- **15:35-15:39** — A strict dependency-header probe proved the hiredis warning issue resolved by
  system-header semantics, then the first focused full-server retry exposed the extracted libxml2
  package's non-relocated `/usr/include/libxml2` hint. Added the actual prefix's libxml2 and p11-kit
  include roots to the explicit `CPLUS_INCLUDE_PATH`. A combined strict cJSON/hiredis/libbsd/
  libxml2/GnuTLS probe passed, followed by the entire client-free build, health, game-loop boot, and
  clean-shutdown preflight.
- **15:40-15:50** — The canonical `make test-all TEST_JOBS=8` gate passed completely under the
  verified durable dependency environment: 397/397 Python regressions plus the native signal-
  handler gate, explicit exit 0. Reviewed the saved output; keyword matches were passing test names,
  with no compiler warning, runtime error, crash, sanitizer symptom, or other diagnostic.
- **15:51** — Stopped the exact now-idle production-backup clone container to release resources.
  It had Docker auto-remove enabled, so the ephemeral container was removed on stop; both compressed
  dumps, their hashes, clone state, and migration-qualification evidence remain in the owner-only
  rollback bundle and can recreate it. No unrelated container was touched.
- **15:51-15:54** — `make test-db` passed all ten isolated Docker/MySQL suites with explicit exit 0,
  including immutable ledger replay, runtime drift rejection on MySQL 8, and the 143-step legacy
  migration/replay/bootstrap-equivalence/runtime-compatibility path. Reviewed the complete output;
  it contains no warning, error, crash, or sanitizer finding.
- **15:55-16:03** — From the stopped state, `make clean-all` removed the inspected reproducible
  outputs with exit 0, followed by an exact from-zero root `make` with exit 0. The server, area
  editor, and all six area tools compiled and linked; the complete 369,508-byte build log has no
  warning, error, undefined reference, crash, sanitizer, or assertion diagnostic. Verified every
  maintained output executable. The rollback runtime remained intact outside the clean target.
- **16:03-16:12** — Built the server's distinct production object graph from zero with
  `PERSISTENCE_BACKEND=mariadb BUILD_PROFILE=production`: 376 translation units compiled and the
  link completed with explicit exit 0. The complete 320,634-byte output is diagnostic-free.
  Candidate `bin/server/dms_new` is executable, stamped `mariadb/production`, and has SHA-256
  `1abf739db7750a89210b785dc48b0d2a4ebc137478a2865e3ea2a6bfa5d37d74`. A bare-shell `ldd`
  correctly showed that the durable cJSON/hiredis prefix is not globally registered; the exact
  production unit supplies that protected loader path, under which every candidate and known-good
  dependency resolves. Production configuration preflight and the full read-only runtime schema
  verifier both passed before promotion.
- **16:12:35-16:12:49** — Started only the exact production user unit. The launcher passed its
  schema gate, promoted the byte-verified candidate, backed up authoritative state, regenerated all
  350 zones and lookup files after the clean build, and entered the game loop. A health request at
  16:12:42 correctly remained closed during world boot; the first post-loop request passed at
  16:12:49. The runtime owns the expected `7777`, `4001`, and `4050` listeners, and the supervisor
  made no new restart.
- **16:14-16:19** — Authenticated through the account menu with the protected configured test
  credentials. The first probe passed `look`, `time`, `weather`, `score`, `inventory`, `equipment`,
  `who`, and `users`; `where` returned no inspection body because the configured staff character is
  Avatar-tier while that command is Immortal-tier. This was a safe harness assumption, not a
  runtime failure. The resulting link-dead state recovered correctly. A first recovery client then
  expected the normal `Play as?` branch even though link-dead selection reconnects immediately; it
  timed out without issuing a game command. The corrected client accepted both branches and passed
  a randomized 11-command run in this exact order: `time`, `equipment`, `who`, `inventory`, `look`,
  `attributes`, `weather`, `users nonplaying`, `exits`, `users`, `score`. Every response contract,
  in-session health check, normal `quit`, account-menu `0`, and final disconnect passed.
- **16:19-16:29** — Continued health, supervisor, listener, core-file, and complete log monitoring
  through the post-logout soak. A broad final scan found four TLS handshakes aborted by one distinct
  external public peer, plus one intentionally failed self-probe that supplied the leaf/fullchain
  bundle as a trust root and one self-probe that waited for the plain listener's terminal prompt
  instead of the TLS listener's direct account prompt. The production certificate is current and
  its four-certificate chain verifies against the system roots. A corrected system-trusted TLS
  client completed the encrypted handshake, received the account prompt, sent an orderly TLS close,
  and left health green. The following 45-second quiet interval added zero diagnostics. These six
  classified transport rejections are the only conventional negative matches; there is no runtime,
  persistence, crash, or data-integrity finding.
- **16:23-16:29** — Repeated the complete read-only runtime compatibility verifier and asserted the
  production immutable ledger at `8:0008_statistics_date_index` with postcondition `8:0:1`. MUD
  health passed, and the supporting DurisWeb endpoint independently reported database and cache
  health at 16:22. Formatting, whitespace, artifact identity, and worktree-scope checks passed.
- **16:25-16:35** — A separately authorized concurrent DurisWeb burn-in cleanly stopped its own
  application unit at 16:25 for a qualified artifact and map-data cutover. This was detected at the
  final handoff check; no active deployment job was raced and this burn-in did not restart or alter
  that service. The concurrent transaction published its expected `206232:546` production map
  position/entrance invariant, then restarted DurisWeb at 16:32:24. Its new PID `1905463` held with
  zero restarts, structured database/cache health passed, and its MUD bridge authenticated with the
  current secret and applied hook state. Its sole warning was the known initial system frame arriving
  before challenge authentication; authentication succeeded immediately afterward. Both services
  then passed a further three-minute convergence soak. Final MUD compatibility, health, artifact,
  PID, log, and formatting checks all passed at 16:35:47.
- **16:36** — Five unrelated documentation moves appeared concurrently in this checkout after the
  runtime handoff. Each new destination is byte-identical to its deleted source. They were not made,
  modified, or reverted by this burn-in and remain preserved as out-of-scope user/parallel work.

## Gate results

| Gate | Result | Evidence / finding |
| --- | --- | --- |
| Production configuration preflight | pass | Explicit `mariadb-primary` configuration validated without exposing values. |
| Initial production health | pass | User service active; health endpoint passed. |
| Initial runtime compatibility | fail (expected prerequisite) | Production is at head 0006; source requires head 0008. |
| Writer quiescence | pass | Exact MUD and DurisWeb services stopped; database has zero other connections. |
| Pre-migration backup | pass | Dump completed, schema markers present, gzip integrity and SHA-256 verified. |
| Production-backup clone restore | pass | Stopped-production and clone core-count signatures match. |
| Clone migration and compatibility | pass | 0007/0008 applied; verifier and no-op replay passed at `8:0:1`. |
| Guarded production-path clone test | pass | Exact production-mode interface applied and replayed 0007/0008 on a second restore. |
| Focused migration runner tests | pass | 11/11 unit tests; documentation and legacy invocation contracts passed. |
| Production immutable migration | pass | 0007/0008 applied at zero connections; verifier and no-op replay passed at `8:0:1`. |
| `make test-all` (first pass) | fail | Clean build passed; 392 tests passed and five harness portability checks failed. |
| Five focused portability reruns | pass | Every exact failing harness passed under the durable dependency prefix. |
| Post-repair migration replay | pass | Zero-writer production replay was a no-op; compatibility remained fully valid at head 0008. |
| DurisWeb post-replay health | pass | Exact user unit active; API reports database and cache healthy. |
| `make test-all` (incomplete rerun) | not counted | 303/397 passes recorded, then the process disappeared without terminal status; full rerun required. |
| `make test-all` (missing-prefix rerun) | not counted | 370/397; all failures were missing dependency-prefix tools/headers/libraries, confirmed by focused passes after restoring the prefix. |
| `make test-all` (ordinary-header prefix) | fail | 393/397; four isolated full-server builds rejected hiredis C headers under project `-Wpedantic`; correcting prefix header semantics. |
| `make test-all` (post-repair final) | pass | 397/397 Python regressions and native signal-handler gate; output audited, explicit exit 0. |
| `make test-db` | pass | All ten isolated Docker/MySQL suites; output audited, explicit exit 0. |
| `make clean-all && make` | pass | From-zero server/editor/area-tools build; complete output audited, both explicit exits 0. |
| Production-profile server build | pass | 376 objects and final link from zero; stamped candidate, linkage, configuration, and schema compatibility verified. |
| Production service boot | pass | Candidate promoted unchanged; full world regenerated; game loop and three listeners ready; no added restart. |
| Authenticated staff smoke | pass | Randomized 11-command final run, live health, link-dead recovery, normal quit, and account disconnect all passed. |
| Production TLS listener | pass | System-trusted certificate chain, handshake, account prompt, and orderly close verified. |
| Post-logout log soak | pass | All current game logs and unit journal inspected; no unexplained diagnostic, persistence failure, core, or restart. |
| Final migration/runtime health | pass | Ledger head `0008`, postcondition `8:0:1`, compatibility, MUD health, and supporting web dependency health passed. |

## Fixes

- Added an explicit, fail-closed immutable production `run` interface to
  `scripts/migration_runner.py`; normal local behavior and the default production rejection remain.
- Added focused production confirmation, backup, allow-list, permission, and quiescence regression
  coverage in `tests/async/test_immutable_migration_runner.py`.
- Updated the production migration procedure in `docs/operations/RUNBOOK.md`,
  `docs/reference/DATABASE.md`, and `docs/persistence/IMMUTABLE_MIGRATIONS.md`.
- Removed the WebSocket harness's hard-coded system cJSON include assumption and preserved the
  configured runtime loader path in four isolated flat-file server journeys.

## Live smoke and log soak

The authoritative live console is the user-unit journal: `logs/duris-console.log` exists but has an
unchanged 2026-09-01 timestamp and received no output from this systemd boot. The handoff unit-journal
capture covers 16:12:35 through 16:35 UTC, contains 2,701 lines, and has SHA-256
`8c08aa1ea5af1b10d528859930a369084c08f6a6b44a089ee500088a9534152f`. Its sole platform notice is
the already-known container-manager refusal to create the optional `ProtectHostname` UTS namespace;
systemd explicitly continued with the other unit restrictions.

Every current regular game log was followed, including files created after boot: `artifact`,
`cmd.debug`, `comm`, `debug`, `file`, `kingdom`, `mob`, `obj`, `status`, and `sys`. Their final
10-entry SHA-256 manifest has digest
`2962eeaddffba5e8bd3784df46f3f678f407f57de03153b365f27e9c68692834`. The connection EOFs correspond
to health/smoke probes and normal disconnects. The status log's callback-address aliases, zone names
containing words such as “Lost,” and two expected stale WebSocket replacements were reviewed rather
than treated as keyword failures. Persistence-specific negative matches are zero, no core file was
created, and both the post-TLS quiet interval and final supporting-service convergence soak added
zero MUD diagnostics.

The credential-free successful smoke evidence has SHA-256
`75eaf1c1711fda05dfbbeb7115e44f10da98dcdf28f422915df23ece9435900c`. No credential, account name,
password, character name, player transcript, or private log content is stored in this journal or its
smoke evidence.

## Completion snapshot

- Source base: `master` / `origin/master` at `08bb88e7e4d4c4345c4e522f8df13b571139b429`,
  with the focused burn-in repairs listed above included in this change set.
- Production database: immutable ledger `8:0008_statistics_date_index`; verified postcondition
  `8:0:1`; complete runtime compatibility pass.
- Deployed runtime: `bin/server/dms`, stamp `mariadb/production`, SHA-256
  `1abf739db7750a89210b785dc48b0d2a4ebc137478a2865e3ea2a6bfa5d37d74`.
- Production service: enabled and active; supervisor PID `1854001`, game PID `1854306`; cumulative
  `NRestarts=4`, unchanged from the pre-start value caused by the earlier documented fail-closed
  profile attempt. No restart occurred during this successful boot or soak.
- Runtime: expected listeners on `7777`, `4001`, and `4050`; MUD health reports persistence ready;
  no configured test session remains established.
- Supporting web service: active at final PID `1905463` with zero restarts after its independently
  owned concurrent cutover; structured health reports both database and cache `ok`, and the MUD
  bridge is authenticated with hook state applied.
- Canonical qualification: `make test-all TEST_JOBS=8` passed 397/397 plus the native signal gate;
  all ten `make test-db` suites passed; required clean root build and distinct production build
  passed with fully reviewed output.
- Final source checks: `git diff --check` and `./scripts/format.sh --check` pass. Pre-existing edits to
  `AGENTS.md` and `.agents/skills/burnin/SKILL.md` remain preserved. Five byte-identical concurrent
  documentation relocations that appeared at handoff are likewise preserved and excluded from the
  burn-in change set.
- Rollback: private bundle `/tmp/duris-burnin-rollback.lS3wmb` remains intact with known-good runtime,
  source/world artifacts, fresh pre-migration database backup, hashes, and all burn-in evidence.
- Handoff state: the healthy production MUD is intentionally left running through its exact user
  service, as required.

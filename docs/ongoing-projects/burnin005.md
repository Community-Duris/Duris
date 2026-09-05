# Production burn-in 005

Status: complete. Final validation passed on 2026-09-05 UTC. Production MUD and DurisWeb are healthy and running. The source changes, regression coverage, and this report are recorded together in the burn-in commit. The original pre-burn-in executable was not retained; the final live-qualified executable is independently saved and its restoration was tested.

## Scope and initial evidence

- Checkout `/home/duris/duris`, initial commit `ea3d8f11c`; worktree initially clean.
- Read `AGENTS.md`, `README.md`, root `Makefile`, operations runbook, and `.agents/skills/burnin/SKILL.md`.
- Production target: `127.0.0.1:3307/duris`, explicitly allow-listed; `mariadb-primary`. Credentials remain private.
- Exact supervisor: **user** systemd manager, `duris-mud-production.service`, invoking `/home/duris/duris/scripts/cycle_mud.sh --production`. Initial supervisor PID 2201135, server PID 2201675; zero service restarts. Initial health check passed.
- Initial listeners: production telnet 74.208.126.44:7777, TLS 74.208.126.44:4001, health 127.0.0.1:4050. Other checkout/service listeners will not be stopped.
- Initial rollback-copy attempt was later found invalid: `cp -a /proc/2201675/exe` preserved the proc symlink rather than independent executable bytes. The initial digest was `f43f6f3da6ba02bc7edabc4e7fbf5edbfed38df9a1147968dd76ac7a6ca03049`; that original executable was not retained through clean-all. See the rollback artifact correction below.
- Restore/start procedure: use an independently verified executable and its matching build stamp as `bin/server/dms` and `.dms-backend`, ensure no unintended staged promotion remains, preserve current scheduler state, then `systemctl --user start duris-mud-production.service`. All production starts use this same service path.
- `bin/server/maintenance-scheduler.state` is runtime state inside the clean-all deletion boundary; preserve it privately after shutdown and restore before startup.
- Docker daemon available (29.7.2). Configured staff account fields present, values never recorded.
- `python3 scripts/migration_runner.py inspect`: sealed baseline `duris-schema-2026-08-27-session11`, nine immutable migrations, 170 baseline tables.

## Required completion evidence

1. Stop exact service and verify its processes/listeners close; preserve rollback and runtime data.
2. Fresh production backup; restore disposable loopback clone; qualify pending immutable migrations there, then guarded approved production application and read-only compatibility verification before building.
3. Clean canonical format, complete regression/native gate, isolated Docker database gate, and fresh root build of all maintained targets. Repair findings and repeat the full required sequence after the last change.
4. Start exact production service; monitor console, all current regular game logs (including new files), and service journal through boot, authenticated randomized inspection smoke, clean logout, and post-logout soak.
5. Record exact smoke commands, observation interval/log files, final health, process/restart state, and any unresolved findings. Leave healthy production running.

Private command outputs, backups, and smoke evidence are kept outside Git under `/home/duris/.local/state/duris-burnin005/`.

## Migration/readiness investigation (08:08–08:12 UTC)

- Stopped `duris-mud-production.service`; verified both original PIDs gone and ports 7777/4001/4050 closed. Preserved final scheduler state outside `bin/`.
- First backup `db/Backup/1788595697.sql.gz` restored into a disposable MariaDB 10.11 container. Production ledger was at step 8; clone applied step 9 successfully, but runtime readiness found **18 missing combat baselines** among 705 eligible characters (wallet/epic missing counts both zero).
- Four remaining production DB connections were mapped by socket ownership to DurisWeb PID 3763004. Stopped the exact user service `durisweb-production.service`; verified zero other target DB connections. Its restart path is `systemctl --user start durisweb-production.service`.
- Fresh fully quiescent backup: `db/Backup/1788595815.sql.gz` (27 MB). Recreating disposable clone from this exact backup before further qualification; initial clone evidence is diagnostic only.
- Protected classification of initial clone: all 18 missing rows are `safe_no_history`, zero history/revision ambiguities. Source already initializes combat baselines on character creation and safe account restoration; inspecting provenance before attributing a new code defect.
- Production migration has **not** run. Production remains quiesced while the documented repair and rollback are rehearsed; the current launcher's newer readiness/head contract would reject the unchanged stale database even with the known-good executable. No compatibility guard will be bypassed.

## Qualified production rollout (08:12–08:16 UTC)

- Quiescent backup SHA-256: `a9e1c715f83db04141ea56ef7fe5aa24c7351d8b2cd7a79db3af7e0e44ef251d` (`1788595815.sql.gz`). Disposable container `duris-burnin005-clone`, MariaDB 10.11, loopback-only random host port, private separate credentials/database `burnin005_test`.
- Applied immutable step 9 on restored clone and verified no-op replay. Full compatibility initially failed only on missing combat baselines; the private clone helper's early final message overstated compatibility after temporarily separating that check. Actual compatibility was subsequently run explicitly and passed after repair.
- Both frozen and production classifications matched exactly: 18 `safe_no_history` rows, revision zero, no ledger entries. Guarded baseline repair passed on clone with a private receipt; exact rollback and reviewed forward DML also passed. All per-PID evidence/decisions remain private.
- Combat/currency/epic/FK reconciliations passed. Item reconciliation found 14 stale parent/root links; no baseline, item-revision, owner-revision, or latest-owner mismatches. Rehearsed checked-in nesting repair, then exact reviewed inverse/forward DML; all five item reconciliation metrics became zero. All 14 rows retained owner identity, state, and revision; no ledger history changed.
- Baseline forward SHA-256 `8235c9703e753642a8f0a2f43f5bc6ff7990c638b74781483ac39c83007dcbb6`; rollback `40569ccce760e264ff234a85bec4808efa8168db1bef4fa648bd04a5e8ad7c9c`.
- Nesting forward SHA-256 `af68f76e3e354308351c8695182a5fd1f6e4727a879e32602b004a5797b8f642`; rollback `0988928756262ed64c95641e9541f8098557f2a1cde44c6b0f936ff4c9886b42`.
- With zero other production DB connections, ran guarded `migration_runner.py run --confirm-production-target "$DB_HOST/$DB_NAME" --production-backup /home/duris/duris/db/Backup/1788595815.sql.gz`, then the exact rehearsed guarded baseline and nesting DML. Runtime compatibility passed before starting any build.
- Production scripts' local-only repair guards were not altered or bypassed. Clone-only scripts ran in a private copy with a clone-only `.env`; approved production repair used separately reviewed SQL under the user's full authority.
- Canonical gates started with `./scripts/format.sh --check` and `make -j4 test-all BUILD_PROFILE=production`. A private Python launcher preserves every regression's full output for diagnostic inspection, including successful tests; it delegates to the unchanged canonical runner and preserves return codes.

## Upstream refresh and clean restart of qualification (08:18 UTC)

- Initial burn-in did not fetch first. On the user's question, `git fetch origin` proved master was 14 commits behind. Stopped the exact superseded test-all process tree (41 processes); its partial results are not final evidence.
- Fast-forwarded master with `git merge --ff-only origin/master` to `94a3d9eb4` (PR #152). Upstream includes persistence/shutdown fixes and already includes the migration-0009 fixture correction. Preserved private copies of our original fixture edit; retained only the additional garrison collation/replay/data-preservation regression beyond upstream.
- No migration files changed in this fast-forward. Repeated immutable inspect, clone no-op application/compatibility, guarded production no-op application/compatibility, and format gate against updated master before clean build.
- Initial build prerequisite failure: host default linker paths lack gnutls/cJSON/hiredis/bsd development libraries. Existing account-local packages are used via `CPLUS_INCLUDE_PATH=/home/duris/.local/opt/duris-deps/usr/include:/home/duris/.local/opt/duris-deps/usr/include/x86_64-linux-gnu`, `LIBRARY_PATH=/home/duris/.local/opt/duris-deps/usr/lib/x86_64-linux-gnu`, and matching `LD_LIBRARY_PATH` (also present in production service). No warnings or dependency guards were disabled.
- Ran `make clean-all`, restored preserved scheduler runtime state, then started fresh root `make -j4 BUILD_PROFILE=production`. All logs remain outside the clean-all boundary. Initial/obsolete test logs remain retained separately.
- Read-only full production reconciliation after the data repairs returned zero for all ten combat/currency/epic/FK metrics, four item ownership metrics, invalid nesting evidence, and nesting drift (`production-full-reconciliation.log`).

## Expanded offline qualification (08:24 UTC onward)

- Added the account-local `usr/include/libxml2` directory to `CPLUS_INCLUDE_PATH` after the first fresh compile reached `cmd/nq.c`; prerequisite build then passed. The final pass repeats clean-all and the complete build with this resolved dependency configuration.
- Updated canonical `make test-db` passed all ten wrappers, including the full 143-step historical upgrade, second application, bootstrap equivalence, and runtime compatibility. The added garrison convergence/replay preservation test passed on both MySQL 8 and MariaDB 10.11.
- To cover the full burn-in scope, also running all remaining `tests/async/run_*mysql.sh` harnesses. Five additional self-contained Docker wrappers run directly (bank delta, locker conversion, combat-baseline repair, personal-locker repair, replacement state), plus MariaDB combat-baseline parity.
- Twelve externally configured wrappers run from a private source copy with a separate synthetic MySQL 8 container `duris-burnin005-fixtures`. Each receives a newly bootstrapped `burnin005_fixture_test` schema; no configured production credentials or data are provided. The representative production clone stays separate.
- Final clean-pass evidence uses `clean-pass-*.log`; individual successful and failed regression output is preserved under `regression-outputs/`. Final full test count will be recorded from the completed discovery run.

## Additional fixture finding

- All five supplementary Docker wrappers and the MariaDB combat-baseline repair test passed.
- The externally configured account-projection harness failed on MySQL 8 with `Can't reopen table: currency_wallet_baseline`. Root cause: its temporary-table fixture cannot execute the production INSERT/SELECT shape on MySQL, although MariaDB permits it. Production SQL operates on ordinary tables.
- Updated `account_character_projection_mysql_harness.py` to map SQL table identifiers to unique ordinary fixture names and drop only those names in a `finally` cleanup. Build products now reside beneath `bin/tests/`. Production source queries and assertions remain intact. Focused reruns passed on both MySQL 8 and MariaDB 10.11.
- Because this is a new test repair, the currently running full regression pass remains diagnostic; the final uninterrupted canonical sequence will repeat after all findings are resolved.

- The external critical-command wrapper reached an outbox timeout. A direct isolated reproduction of `refresh_counts()` returned MySQL error 1137 (`Can't reopen table: critical_operation_inbox`), proving another temporary-table fixture limitation rather than scheduler timing. Updated its wrapper to create/destroy its own loopback Docker database and let the outbox harness use ordinary schema tables. It no longer reads the checkout's configured `.env`. Retry, delivery, dedupe, restart, dead-letter, and reconciliation assertions remain intact; dual-engine focused reruns are required.

- Critical command/outbox focused checks passed on MySQL 8 and MariaDB 10.11 after the fixture isolation repair.
- The player-load harness initially had no configured character in the synthetic schema. Supplied a separate clone-only environment with the configured staff identity (no password) and ran the full repository snapshot/topology fixture against the repaired representative clone: passed. This was fixture provisioning, not a production player-load defect.
- Remaining externally configured transaction wrappers passed: account identity, artifact/guild, auction, boon/reward/zone, combat outcome, currency, epic, item transfer, and session audit.
- Main regression diagnostics found Redis executables missing from the shell `PATH`; the installed account-local package supplies both. Added its `usr/bin` to the private build/test environment.
- `test_player_save_journal.py` assumed `mkdir(...,0755)` creates an unsafe directory under any umask. The burn-in's owner-only umask correctly produced 0700, so the test's rejection assertion was invalid. Added explicit `chmod(...,0755)` to establish the intended unsafe fixture. Focused run under umask 077 passed; the runtime permissions guard remains unchanged.

## Final repair-loop restart (08:38 UTC)

- Diagnostic updated-master regression pass completed: 399 passed, 12 failed in 639.35 seconds. Eleven Redis prerequisites and the umask-sensitive journal fixture explain all twelve failures; focused corrected runs passed (26 Redis tests, journal test). All full-instance flat-file boot/combat/character-kit journeys passed.
- Starting final `release-pass-*` sequence after the last source/test repair: clone and production migration/compatibility, format, clean-all, fresh production root build, all 411 discovered regressions plus native signal test, canonical Docker DB gate, and all affected supplementary DB gates. Prior partial/failing logs remain diagnostic evidence only.

- Final database inventory: all 27 `run_*mysql.sh` wrappers are covered. Sixteen are now self-contained Docker wrappers (ten canonical plus six supplementary); eleven use the private fixture checkout. Player-load's configured-character prerequisite uses the separate repaired production clone. Additional MariaDB parity runs cover runtime compatibility and both repaired projection/outbox paths plus combat-baseline repair.
- Final isolated projection rerun passed; all production runtime compatibility gates remained green before the release clean build. Both production MUD and DurisWeb remain deliberately stopped for offline qualification.

## Rollback artifact correction

- Audit caught an operator mistake: `cp -a /proc/2201675/exe .../dms.known-good` preserved the proc symlink, not executable bytes. Its initial digest matched the running executable, but it was **not an independent rollback copy**. The initial rollback-copy claim above is superseded. The symlink target is recorded in `dms.original-symlink.txt`; the dangling symlink was removed. The original executable was not retained through clean-all.
- Corrected the preparation with an independently copied, non-symlink `dms.release-candidate` and its real build stamp outside `bin/`, checked byte-for-byte by SHA-256 against the successful clean build: `1d40ddca7cecb19361cf7220016fe6c70b119c9c35da59acc32b438b65a96211`. This is a build-qualified candidate until live validation succeeds; it will then be the verified rollback artifact. No claim is made that the original executable remains recoverable.

## Final offline results accumulating

- Release clean root build passed, 577 output lines with no compiler/linker diagnostics. Server, editor, and six area generators all exist and are executable; server stamp is `mariadb/production`.
- Release canonical `make test-db` passed all ten wrappers, including the 143-step migration/replay/equivalence/compatibility gate.
- All eleven release external fixture wrappers passed; inspected their complete captured output with no warnings/errors/assertions. The representative-clone player-load checks are included.
- Supplementary Docker/dual-engine gates and the final complete Python/native regression gate remain running; completion is not yet claimed.

- Final DB gate process exited zero: all 27 wrappers passed (ten canonical, six supplementary Docker, eleven isolated external), plus MariaDB runtime compatibility, combat-baseline repair, and critical outbox parity. Account projection was separately verified on MariaDB after its repair.
- Verified the staged production executable still matches the clean-build SHA-256 and the preserved scheduler state is byte-identical before boot. No final-regression failure or emitted diagnostic has been found through the first 408 completed tests.

## Final diagnostic-output correction (08:52 UTC)

- The release regression command completed with **411 passed, 0 failed in 625.30 seconds**, including native signal checks. Inspection of retained successful-test output nevertheless found a GNU Make jobserver warning from `test_pfile_tool.py`: the outer `make -j4 test-all` exported jobserver metadata through Python without the corresponding descriptors.
- This is invocation-level build coordination, not a compiler/runtime failure. Returning to the prescribed plain `make test-all` removes the unused outer jobserver while retaining the Python runner's bounded eight-worker parallelism. No warning filter or check suppression was added.
- Repeating migration/compatibility, format, clean build, and the entire canonical sequence as `qualified-*` before live qualification. The independently copied release candidate remains outside clean-all and remains only build-qualified until live smoke passes.

- The `qualified-*` clean root build passed with no diagnostics and is byte-identical to the independent candidate (`1d40ddca7cecb19361cf7220016fe6c70b119c9c35da59acc32b438b65a96211`). The normal, non-parallel outer `make test-all` is running all 411 tests with the canonical runner's own bounded workers.

## Clean offline pass complete (09:04 UTC)

- `qualified-*` migration/compatibility, formatting, fresh root production build, and canonical database gate all passed.
- `make test-all BUILD_PROFILE=production` completed: **411 passed, 0 failed in 545.37 seconds**, followed by the native signal-handler test. All 411 full per-test outputs were retained and inspected: no warning/error/sanitizer diagnostics or skipped-test reports. The previous pfile jobserver warning is absent.
- All 27 DB wrappers and the additional dual-engine checks have passing final evidence after their source repairs.
- Pre-boot production garrison schema verifier passed. Re-ran the complete reviewed production reconciliation SQL: all **16** aggregate metrics are zero. Scheduler state and the clean-built production executable remain byte-identical to the preserved copies.
- Live qualification now remains: exact service startup, staff smoke, service-path rollback/restart verification using a real independent executable copy, and post-logout soak.

## Live finding and repair restart (09:12 UTC)

- Production boot passed health with zero service restarts. TLS staff authentication succeeded, but the historical news banner stopped mid-line before the return prompt. No character entered gameplay during this attempt.
- Root cause: `raw_write_to_descriptor()` ignored positive partial GnuTLS sends and discarded unsent bytes on both TLS and plain-socket backpressure. GnuTLS documents that sends can be shorter than the requested payload and interrupted records must be resumed: https://www2.gnutls.org/manual/html_node/Data-transfer-and-termination.html .
- Added a bounded descriptor-owned queue for actual transport bytes, including MCCP output. It retains partial writes across ticks, resumes interrupted TLS records before new data, preserves order, records actual transmitted bytes, closes on fatal/queue-limit failures, and frees queued memory on descriptor teardown.
- Added native regression covering 50 KB multi-record output, short sends, EAGAIN/EINTR, appended binary data, zero writes, fatal writes, bounded queue overflow, cleanup, and MCCP decompression fidelity for plain/TLS transports. Focused test passed with warnings treated as errors.
- Before stopping the exact production user service, independently copied actual running executable bytes to private `dms.pre-tls-fix` (regular file, SHA-256 `1d40ddca7cecb19361cf7220016fe6c70b119c9c35da59acc32b438b65a96211`) and its `.dms-backend` stamp. This artifact has a known TLS defect and is only the pre-repair fallback; the original initial executable remains unavailable as disclosed above.
- Verified service inactive, MainPID=0, and all three owned listener ports closed. Refreshed the saved maintenance scheduler state after shutdown. DurisWeb remains stopped.
- Live world-recovery generations 1945–1947 completed with acknowledgments, one attempt each; no persistence failure found. Final qualification restarts after this production source repair.

- The production compiler caught Linux aliasing `EWOULDBLOCK` to `EAGAIN` under `-Wlogical-op`; restored the platform-conditional comparison and added that warning flag to the focused sender test. Focused test passed again. The diagnostic build is retained separately; restarting the complete gate sequence after this correction.

- Corrected `tls-*` clean root build passed: 577 output lines, no warning/error diagnostics. All eleven external database wrappers, six supplementary Docker wrappers, and MariaDB runtime/outbox parity reruns passed; complete logs were inspected. Canonical regression discovery is now 412 tests, including the new Telnet/TLS runtime harness.
- First live observation evidence was archived intact under private `live-attempt1/`. Its one communication error is the smoke client closing TLS after the truncated-banner timeout. Scheduler `NEVENT BUDGET/CATCHUP` entries are the documented bounded-work telemetry; observed debt returned to zero rather than growing without recovery.

- Continued sender review found a second concrete output defect: `process_output()` combines multiple individually bounded queue entries, then `write_to_descriptor()` converted that unbounded combined string into a fixed 128 KB stack buffer. Replaced newline/CP437 conversion storage with bounded dynamic strings. Messages beyond the transport limit fail closed instead of overflowing.
- Extended the same native regression with 240 KB combined text, CRLF normalization, UTF-8 preservation, CP437 conversion, partial TLS delivery, and over-limit rejection. The actual Unicode converter is linked. Focused normal and AddressSanitizer/UndefinedBehaviorSanitizer executions passed, as did syntax checking under every production compiler diagnostic flag.
- The ongoing regression run is diagnostic because its initial production executable predates this additional repair. The full uninterrupted sequence will repeat after it completes; final success is not yet claimed.

- Preceding diagnostic regression pass completed: **412 passed, 0 failed in 550.14 seconds**, plus native signal checks. All 412 retained outputs were inspected without emitted diagnostics or skipped tests; keyword matches were descriptions of passing fault-injection/source-contract assertions. The final `transport-*` sequence began at 09:29 UTC after the dynamic text-buffer repair; clone and production migration/compatibility and formatting passed before clean-all/build. All supplementary and external DB wrappers are also rerunning against current source.

- Final `transport-*` clean build passed without diagnostics. Independent regular-file candidate SHA-256: `66aec51686ac32117b32b31d2c6e1e2221783a1716557dc2b0f43d2a13aad52a`; byte-for-byte copy and `mariadb/production` stamp saved outside `bin/`.
- All eleven external fixtures, six supplementary Docker wrappers, and MariaDB runtime/outbox parity reruns passed against the final source. Complete outputs contain no emitted diagnostics. The final canonical regression and DB sequence remains in progress.

- Final `transport-*` canonical regression run completed: **412 passed, 0 failed in 580.13 seconds**, followed by native signal checks. All 412 complete outputs were inspected; diagnostic keyword matches are passing fault-injection/source-contract descriptions, with no emitted compiler/runtime/sanitizer errors or skipped tests. Canonical isolated `make test-db` is now running.

## Final offline pass complete (09:45 UTC)

- Final `transport-*` sequence passed migration/compatibility, formatting, a fully fresh production root build, all **412** regressions plus native signal checks, and all ten canonical Docker database wrappers. Legacy upgrade, replay, bootstrap equivalence, and runtime compatibility all passed. Complete output was reviewed, including successful per-test stdout.
- All **27** database wrappers have passing evidence against the final source; additional MariaDB parity and the sanitized transport test passed.
- Pre-boot runtime compatibility and garrison schema checks passed, all **16** production reconciliation metrics remained zero, and both the production executable and scheduler state matched their preserved independent copies.
- Final live monitoring began at **2026-09-05 09:45:10 UTC**, following all current regular game logs, newly created/rotated files, console output, and the production user-service journal.

## Live account-disconnect finding (09:48 UTC)

- Final TLS boot was healthy with zero restarts. The complete historical news/return prompt arrived. Staff character loaded and all ten safe commands returned sensible responses: `who`, `look`, `users nonplaying`, `equipment`, `time`, `score`, `inventory`, `weather`, `attributes`, `users`.
- Gameplay `quit` returned to the account menu; option `0` delivered “Thank you for playing!” but did not close the socket. Root cause: `CON_FLUSH` teardown existed only in the input-driven nanny, so an otherwise idle client waited for another command or the 60-second login timeout. This is separate from the repaired TLS truncation.
- Moved normal `CON_FLUSH` teardown into the output loop, after application, Telnet/TLS, and WebSocket/control queues drain. Nanny no longer closes the descriptor prematurely when extra input arrives while transport bytes remain queued.
- Extended the real full-world regression journey to require the goodbye message and server EOF within five seconds without sending extra input, on both original-session and restored-session logouts.
- Saved the actual current executable and stamp independently as `dms.pre-logout-fix` (SHA-256 `66aec51686ac32117b32b31d2c6e1e2221783a1716557dc2b0f43d2a13aad52a`), then stopped the exact production user service, verified closed listeners and MainPID=0, and refreshed preserved scheduler state. The earlier ETA was extended for this newly observed defect.

- Focused real-server full-world regression passed after the disconnect repair. Both original-session and restored-session account logouts received the goodbye message and server EOF without additional input; durable player/item restart assertions also passed.
- Final `logout-*` sequence is now running. After its clean root build, the canonical regression and isolated DB gates run concurrently as independent checks against the same unchanged source. Supplementary/external wrappers are repeated as well. Prior live evidence is preserved under private `live-attempt2/`.

## Final logout-repair offline pass complete

- `logout-*` migration/compatibility, formatting, clean production root build, and both concurrent canonical gates passed: **412 regressions passed, 0 failed in 585.63 seconds**, native signal checks passed, and all ten canonical Docker database wrappers passed. All 412 complete outputs and full build/DB output were reviewed without emitted diagnostics or skipped tests.
- The final full-world regression explicitly observed two server-initiated account disconnects, before and after process restart. All 27 DB wrappers and supplementary MariaDB parity checks passed against final source.
- Independent final executable/stamp SHA-256: `865a1d42aa685dd2a23f98b3a640931fc4177b18893e2e4b31d2beea80b58d66`. Pre-boot compatibility, garrison, all 16 data reconciliations, executable integrity, and scheduler-state preservation passed again.
- The exact pre-migration backup is also retained privately as `production-pre-migration.sql.gz`, with original modification time and verified SHA-256; it has not been relabeled as a fresh backup.

## Successful live smoke and executable restoration

- First final TLS smoke completed at 2026-09-05T10:11:15.260312+00:00. Commands, in randomized execution order: `weather`, `inventory`, `who`, `attributes`, `equipment`, `users`, `users nonplaying`, `look`, `exits`, `score`. Complete news banner, responses, health, gameplay quit, account goodbye, and automatic socket closure passed.
- Copied the actual running executable bytes into a regular, independent private `dms.known-good`, with matching `dms.known-good-backend` stamp and SHA-256 `865a1d42aa685dd2a23f98b3a640931fc4177b18893e2e4b31d2beea80b58d66`. This is the final live-qualified release; it does not recover the original pre-burn-in executable.
- Stopped the exact production user service, verified MainPID=0 and closed owned listeners, restored the saved executable and stamp, preserved current scheduler state, and restarted through the same `cycle_mud.sh --production` service path. Restored health passed. World recovery generation 1950 was acknowledged on shutdown and restored on startup.
- Second TLS smoke completed at 2026-09-05T10:13:15.731981+00:00. Commands: `users nonplaying`, `users`, `who`, `equipment`, `score`, `weather`, `time`, `look`, `attributes`, `inventory`. All responses, health checks, clean gameplay quit, account goodbye, and automatic connection close passed again.
- Restarted `durisweb-production.service`; its HTTP health is `ok` and its MUD auction channel authenticated successfully. One companion-client startup warning rejected a pre-authentication `system` greeting, followed immediately by successful authentication with the current secret; this is the intentional pre-authentication message guard, not an authentication failure. No companion source changes were made.
- Removed only the three verified disposable containers labeled `duris.burnin=005` and their anonymous volumes. Production database containers and all backups/evidence remain intact. A three-minute post-logout soak is running with repeated MUD/Web health and stable supervisor/server/Web PID checks.

## Completion evidence

- Final state checked at **2026-09-05T10:19:10.347737+00:00**: production supervisor PID **1475682**, MUD server PID **1476236**, DurisWeb PID **1478825**. Both user services are active with **zero automatic restarts**; the same processes remained stable through all seven soak probes.
- Post-logout soak: **180.44 seconds**, seven successful MUD and Web health samples. World recovery generation **1951** completed successfully during the final observation, on its first attempt.
- Final production runtime compatibility passed and all **16** reviewed reconciliation metrics were **zero** after live smoke and restoration. The on-disk executable, actual running executable, and independent recovery copy share SHA-256 `865a1d42aa685dd2a23f98b3a640931fc4177b18893e2e4b31d2beea80b58d66`.
- Final log observation interval: **2026-09-05T10:10:40.454559+00:00 through 2026-09-05T10:20:02.310743+00:00**. Console and every current regular game log, including new files and rotated inodes, were followed continuously across both final boots, both authenticated smoke runs, controlled restoration, and the post-logout soak. The production service journal was also captured. No unexpected MUD warning/error/fatal/assertion/corruption/failure output was found. The companion startup guard message and successful authentication are classified above.
- Monitored paths: `logs/duris-console.log`, `logs/log/.gitignore`, `logs/log/artifact`, `logs/log/cmd.debug`, `logs/log/comm`, `logs/log/debug`, `logs/log/exit`, `logs/log/file`, `logs/log/kingdom`, `logs/log/mob`, `logs/log/obj`, `logs/log/status`, `logs/log/sys`, plus the production user-service journal. Full per-file boundaries/counts are preserved privately in `final-log-review.json` and `live-progress.json`.
- Validation commands: `./scripts/format.sh --check`; explicit clang-format check of the new native harness; `make clean-all`; `make -j4 BUILD_PROFILE=production`; `make test-all BUILD_PROFILE=production` (412 passing Python regressions plus native signal checks); `make test-db`; all 17 remaining isolated/external DB wrappers; additional MariaDB parity runs; focused normal and ASan/UBSan transport checks; full-world account-disconnect journey; production compatibility/reconciliation checks; two verified-TLS staff smokes; verified executable restoration through `systemctl --user start duris-mud-production.service`; repeated `./scripts/healthcheck.sh` and DurisWeb `/health` probes.
- No required gate was skipped. No source/build/data finding remains unresolved within this burn-in. The original executable-copy mistake remains an explicit historical limitation; the original binary has not been claimed recovered. Current recovery artifacts, original quiescent DB backup, inverse SQL, and full private evidence are preserved. No credentials, player data, logs, binaries, or generated assets were added to Git; no commit or push was performed.

## Publication

- After completion, the user requested committing all burn-in changes and pushing master. Refreshed origin and verified master matched origin/master before committing. The validation above covers the committed source; only this publication note changed afterward.

# Combined recovery acceptance checklist

Scope: local integration of the player-death restitution work with Collector recovery
notifications on `feat/combined-recovery-331-375`, based on `96f2a51a4` (#421).
Remote publication, PR creation, and production access are out of scope.

## Integration and provenance

- [x] Both workspaces resolve `96f2a51a482676e2c7f0794879463c2f87f6c9d7` as
  the merge base.
- [x] Donor notification change is the exact 21-path allowlist in commit
  `de29a0824fb7a6d1bf12b472b0836b717fe801ee` (`Add Collector recovery
  notifications`). Donor `git show --check` is clean and its worktree is clean;
  no duplicate donor commit was created.
- [x] Primary contains the donor commit as `5df64a7cafc33e11257ba458be9a1f8a25a5c280`
  and retains the restitution work from the pre-integration stash
  `c446782428f3419fd0cb362868bc38165a89b101`.
- [x] No unmerged index entries or conflict-marker lines were found in the
  integration source. `src/sql/sql.c` contains both the notification
  `send_to_pid_offline_deduplicated` changes and the restitution
  `duris_sql_exclusion_guard` changes; neither side was selected wholesale.
- [x] Migration `0020_player_death_restitution` remains restitution-owned.
  Collector notifications add no migration.

## Formatting and build

- [x] The repository formatter was run against all 30 changed C/C++ candidates
  with clang-format 18 from the isolated `duris-263-build` toolchain. A second
  formatter check passed after the formatted files were copied back to the
  primary worktree.
- [x] Full server build passed with `make -C src` in the isolated task path
  `/work/combined-recovery-331-375` inside `duris-263-build`; the resulting
  executable was `/work/combined-recovery-331-375/bin/server/dms_new`.

## Focused tests

- [x] Collector notification, death-enrollment, maintenance, service,
  repository, flatfile-repository, command, and runtime focused tests passed in
  the isolated build container.
- [x] Restitution CLI (9 tests), reconciliation (8 tests), target (8 tests with
  1 intentionally skipped native-DB case), and review (3 tests) passed locally.
- [x] Production-policy unit test was collected with its disposable-DB case
  skipped when no image variable was supplied; the real disposable journey is
  listed below.
- [x] Python compile, JSON parse, shell parse, and `git diff --check` passed for
  the changed restitution support files.

## Disposable SQL evidence

- [x] `run_player_death_restitution_mysql.sh` passed against disposable
  MariaDB 10.11, including schema replay/drift refusal, guarded apply,
  runtime repository integration, native SQL factory/pool exclusion, duplicate
  prevention, and SQL readback.
- [x] `run_player_death_restitution_reconciliation_mysql.sh` passed against
  disposable MariaDB 10.11, including legacy-Unknown approval, competing
  artifact no-write refusal, canonical baseline seeding, and exact identity
  readback.
- [x] `run_player_death_restitution_guard_mysql.sh` passed against a dedicated
  disposable MariaDB 10.11 session, proving competing startup refusal and lock
  release after session loss.
- [x] `run_player_death_restitution_production_policy_mysql.sh` passed against a
  production-named but disposable MariaDB 10.11 database; target identity,
  backup, guarded apply, and readback were exercised without a live target.
- [x] Artifact timing evidence is the remaining lifetime at loss, not the stale
  original expiration and not a fresh full-life grant: the SQL test verified
  usable lifetime `456`, source timer epoch `1700000456`, loss epoch
  `1700000000`, and delivery timer derived from delivery plus that remaining
  lifetime. No arbitrary compensation lifetime is used.
- [x] `run_runtime_compatibility_mysql.sh` passed against disposable MariaDB
  10.11 and MySQL 8.0. The pre-fix metadata fingerprints were rejected by both
  live engines; the manifest and C contract now use the observed fingerprints,
  and both engines pass the full-schema/drift checks.

## Remaining gates / handoff

- [x] Fresh issue-331 player gameplay journey passed with the newly built
  `dms_new` artifact (`0d073ec8265d53029844a9366e5d64c9a731db63de5ad25dcf8ff3ba42d08d81`),
  disposable DB/runtime, guarded offline recovery, authority readback, and
  restart replay without duplication.
- [ ] Independent auditor review is read-only on the primary and must be
  incorporated if findings arrive.
- [ ] Full restitution SQL suite against MySQL 8.0 was not run; only runtime
  compatibility parity was exercised there.
- [ ] Parent agent owns the single PR after #421, issue links 331/375, and the
  notification/timer issue links. No push or remote merge is authorized here.

## Evidence notes

All SQL resources used by the checks were task-owned disposable containers and
were removed by their runners. `/work267`, `/work421`, production services, and
configured production databases were not used or mutated.

# `master` and `codex/master-stable`: branch history and staging

Status: done for staging, 2026-09-23. `codex/master-stable` was merged into
`master` through [#625](https://github.com/Community-Duris/Duris/pull/625), and
staging has run the tag `staging-2026-09-23-1248` since 12:59. Production is
on hold. Times are UTC; Liskin's commit timestamps use -0600 and have been
converted.

The staging move itself is recorded in
[2026-09-23-server-environments.md](2026-09-23-server-environments.md).

## Summary

- Four code lines had drifted apart: `master`, Liskin's `codex/master-stable`,
  staging's local rollback branch and production's old checkout.
- The way back was one merge of `codex/master-stable` into `master`. Staging
  now runs tagged `master` commits, and production follows when the owner
  decides.
- The open incidents, #621, #622 and #623, were already present in the code
  staging ran. The merge covers only the death-path part of #622 and makes
  none of them worse. They block production, not the staging deploy.

## Progress

- [x] Archive staging's exact head on `origin` as the tag
  `archive/staging-20260923` (`afdee58cc`, tree `c0686f32d`, the same tree as
  the staging checkout).
- [x] Find out why the `72238a8f5` build was held. See
  [The held build](#the-held-build).
- [x] Merge `codex/master-stable` into `master` on `integrate/master-stable`,
  resolve the eight conflicts and format the merged code.
- [x] Review the commits that staging hasn't run yet.
- [x] Validate locally: build, `make test-all`, `make test-db` and the extra
  database legs. See [Validation](#validation).
- [x] Merge [#625](https://github.com/Community-Duris/Duris/pull/625). It
  landed at 12:48 as a fast-forward, so `master` is `36e44f172` and contains
  all of `codex/master-stable`.
- [x] Deploy the tag `staging-2026-09-23-1248` (`36e44f172`) to staging and
  smoke-test it. See [Staging deploy](#staging-deploy).
- [x] Production decision: hold. See [Production](#production).

## The branches as audited

The audit pinned `master` at `66e787027` and `codex/master-stable` at
`3403c1db9`. Their merge base is `c6e94be85`.

| Where | Commit | Code |
| --- | --- | --- |
| `master` | `66e787027` | The shared line, with the port change, login banner and Stromvok ferry. None of Liskin's commits. |
| `codex/master-stable` | `3403c1db9` | `master` through `c6e94be85`, plus Liskin's 27 commits (24 regular, 3 merges) and cherry-picks of the port change and banner. |
| Staging (`duris-staging` on the production host) | `afdee58cc` | `a6a2124c1` from `codex/master-stable`, plus cherry-picks of the port change, banner and ferry. |
| Production | `440248b17` | `master` from 2026-09-18, 145 commits behind `66e787027`. |

- Liskin's commits run from `b839cadbd` (2026-09-22 02:24) to `72238a8f5`
  (2026-09-23 04:37). Almost all concern item custody, copyover and death
  recovery. None went through a PR. The branch merged `master` twice:
  `e30e4a8e0` and `16582e101`.
- Staging has 18 of the 27. It lacks Liskin's last nine and four `master`
  commits that came in through `16582e101`: the Frost Beam fix (#617) and two
  god-list changes.
- `master` has 24 regular commits that the stable branch lacks. They cover the
  port change and banner (already there as cherry-picks), the ferry, test
  maintenance, an epic-zone seed refresh, dependency bumps and documentation.

The running executables matched their checkouts' `bin/server/dms`:
staging `170fb0dd1629a886…` and production `9d0ca409e44f289f…`.

## What staging ran on Plesk

These events come from the Plesk checkout's reflog, `bin/server/history`,
the game logs and the incident records under `~/.local/state/duris-incidents`.

| When | What happened |
| --- | --- |
| 2026-09-22, before 12:47 | Staging ran `master`. |
| 2026-09-22 06:37–06:55 | Two SIGABRT crashes on reboot, with gdb backtraces (`20260922-reboot-sigabrt`, `20260922-final-legacy-reboot-sigabrt`), and a custody recovery (`20260922-custody-recovery`). |
| 2026-09-22 12:47 | The checkout switched to `codex/master-stable` at `e30e4a8e0`, four minutes after Liskin merged `master` into it. |
| 2026-09-22 13:25–18:30 | Six fast-forwards along the branch, ending at `a6a2124c1`. |
| 2026-09-22 18:40 | The `a6a2124c1` process started. It ran until the move at 2026-09-23 07:37 and hit the save rejections in #622 and the failing world captures in #623. |
| 2026-09-23 05:26–05:33 | A deploy of `72238a8f5` was started and called off. See below. |

### The held build

The records show a planned reboot that was postponed, not a rejected build:

- 05:26: a copy of the running binary was saved as `~/rollback-dms-a6a2124c1`.
- 05:26:00: the staff character Veldrion, connected from the Plesk host itself,
  announced "Planned maintenance reboot in 5 minutes". Veldrion is the staff
  character of staging's test account, so the deploy was run from the host
  with that account, not by a player.
- 05:27–05:28: the checkout fast-forwarded to `72238a8f5` and the server was
  built. The binary is kept as
  `bin/server/history/dms_new.72238a8f5.rollback-held`.
- 05:29:55: Veldrion announced "Maintenance reboot postponed. Please continue
  playing".
- 05:33: the checkout moved to a new local branch,
  `codex/rollback-sbs-20260923`, at the running `a6a2124c1`.

No record gives a reason. At the time, one character's saves had failed 293
times in a row, a death recovery had waited about four hours, and every world
capture had failed since 03:10. A restart then would most likely have hit the
#621 SIGKILL and lost progress. Nothing found faults `72238a8f5` itself, so the
integration includes it after review.

The two stashes that came along with the move need nothing further:
"server shutdown-fix worktree recovery" is the same patch as `0bd954789`,
already on both branches, and "server runtime manifest worktree recovery" holds
only an old copy of the compatibility manifest from before `0029`. Both stay in
the staging checkout. The held binary and older executables stay on Plesk.

## Integration

The branch `integrate/master-stable` starts at `66e787027`:

- `d94d81387` merges `3403c1db9`. It has eight conflicts:
  - In five files, `master` had only the clang-format sweep `843ee795b`:
    `necromancy.h`, `flatfile_world_item_repository.c`,
    `item_ownership_runtime.c` and the two corpse repository harnesses. They
    take the stable content, clang-formatted. clang-format 18 turns the merge
    base's versions into exactly `master`'s, so no `master` change is lost.
  - `handler.c` takes the stable corpse-custody changes and keeps the ferry's
    `ferry_forget_object(obj)` call in `extract_obj()`. That line was
    `master`'s only change there apart from whitespace.
  - `test_difficulty_dials.py` and `test_kingdom_contract.py` keep `master`'s
    checks. The merged files are identical to `master`'s.
  - Every other file that both sides changed matches the stable content
    (ignoring whitespace), plus `master`'s ferry declarations and one harness
    stub.
- `706832694` formats eight stable files that merged cleanly. In
  `reset_zone()`, a ternary that clang-format split becomes an equivalent `if`.
- `36e44f172` drops a harness stub that both branches had added to
  `test_death_field_runtime.py`, which left it defined twice.

### What reaches staging

Compared with staging's `afdee58cc`, five of Liskin's commits change runtime
code. The other four are tests and a merge. From `master` come the Frost Beam
fix, the god list, the format sweep and a corrected seed hash for
`earthp.zon`. No migration files change, so the staging database needs no
migration.

| Commit | Change | Review |
| --- | --- | --- |
| `d7098aa77` | Copyover refuses to run unless every live connection is a playing plain-Telnet session. A hostile raise detaches durable items before deleting the corpse row, instead of letting the foreign key cascade them away. A decayed corpse keeps its coins in the room. | Fails closed. While anyone is on TLS or at the login menu, copyover refuses, so deploys use a cold restart. |
| `6380a4bc3` | Live custody follows the committed corpse outcome: coins stay, transient items are discarded, and the runtime ownership map is updated on every corpse path. | Consistent with the database side of `d7098aa77`. |
| `42877643c` | A dead character in the private recovery hold is re-held instead of abandoned when `update_pos()` turns it to sleeping. The legacy importer adds the offline-message identity column and index idempotently. The migration runner reports the last error line. | The legacy importer doesn't run at boot. |
| `c555ecca1` | A death save checks the stored items against its corpse payload instead of the live-inventory custody check. | Addresses the death-recovery waits in #622. |
| `72238a8f5` | A disputed death waits for database acknowledgement instead of accepting a journal write, so a reconnect can't race an unapplied death. | Fails closed. A slow database keeps the character in the hold longer. |

### Validation

On the workstation, at the merged head `36e44f172` unless noted:

- `make test-all` ran on the merge and formatting commits before the two fixes
  (`3e77812ae`, which is not on `master`): 660 passed and 3 failed.
  - Two failures were caused by the merge, and both are fixed:
    `test_death_field_runtime.py` (both branches had added the same harness
    stub) and `test_issue_552_local_shop_contract.py` (clang-format split the
    call the contract looks for, so `reset_zone()` now uses an equivalent
    `if`).
  - `test_account_recovery_journey.py` failed because `db.c` was edited while
    its server build was compiling. It passes on its own.
  - All three pass at `36e44f172`, and so do the 21 shop tests.
- The production-profile MariaDB server
  (`make -C src PERSISTENCE_BACKEND=mariadb BUILD_PROFILE=production`) and the
  flat-file server both build.
- `./scripts/format.sh --all --check` passes on all 1,070 files.
- These database legs passed at `3e77812ae`. Its runtime code matches
  `36e44f172` except for the `if` above:
  - `make test-db` (17 suites)
  - `run_player_death_disposition_mysql.sh` on MariaDB 10.11 and on MySQL 8.0
  - `run_corpse_lifecycle_repository_schema_mysql.sh` on MySQL 8.0
  - `run_runtime_compatibility_mysql.sh` on MariaDB 10.11

## Known incidents

| Issue | After the merge |
| --- | --- |
| [#621](https://github.com/Community-Duris/Duris/issues/621) service stop ends in SIGKILL | Unchanged. Workaround: stop when the in-flight world capture is 200–235 seconds old. |
| [#622](https://github.com/Community-Duris/Duris/issues/622) custody-mismatch saves stay rejected | The death path is covered by `c555ecca1` and `72238a8f5`. Ordinary saves still get only one recapture. |
| [#623](https://github.com/Community-Duris/Duris/issues/623) world captures stop completing after long uptime | Unchanged. Staging already runs world-recovery schema 13. On the new host a capture takes 285–295 seconds of its 300-second limit. During this deploy's build one expired, and every later publish failed until the restart ([details](https://github.com/Community-Duris/Duris/issues/623#issuecomment-5795324937)). |

## Production

Production stays on `440248b17` for now, 177 commits behind `master`. Moving
it needs the owner's go-ahead for two reasons. `master` applies migrations
`0029_critical_failure_stage` and `0030_telemetry_quarantine` to production's
MySQL 8.0 at boot, and #621, #622 and #623 should be fixed first. Production's
website, WebSocket and systemd setup also differ from staging's.

## Staging deploy

The checkout `/home/duris-staging/duris` is on `master`, tracking
`origin/master`. This deploy used these steps; later ones can repeat them.

1. Tag the `master` commit, for example `staging-2026-09-23-1248`, and push the
   tag.
2. On staging, run `git fetch origin --tags`, then
   `git switch -C master <tag>` and `git branch -u origin/master master`.
3. Build with
   `nice -n 19 make -C src PERSISTENCE_BACKEND=mariadb BUILD_PROFILE=production -j3`.
   This took 5 minutes and didn't slow production, but staging's own game loop
   slowed, so restart soon after the build (#623).
4. Copy the running binary (`cp -L bin/server/dms`), `bin/server/.dms-backend`,
   `lib/misc/event_names` and `bin/server/maintenance-scheduler.state` to
   `~/deploy-rollback/<old commit>/`, and check the binary's SHA-256.
5. If anyone is online, warn them five minutes ahead:
   `echoa *** Server update: restart in about 5 minutes... ***` from the
   staff test character.
6. Run `systemctl --user restart duris-mud-production` when the in-flight world
   capture is 205–230 seconds old, going by the last
   `starting bounded world recovery capture` line in `logs/log/sys`.
   `cycle_mud.sh` then moves `bin/server/dms_new` and its `.dms_new-backend`
   stamp into place, keeps the old binary in `bin/server/history`, and
   regenerates `lib/misc/event_names`.
7. Check that `/proc/<pid>/exe` matches the build, that 4000 and 4001 are
   listening, that the boot log and the first world capture are clean, and that
   the test account can log in and quit over plain telnet and over TLS.

This deploy, at 12:59, stopped in 3 seconds, entered the game loop at 12:59:37,
and passed every check in step 7. The first capture was acknowledged at 13:04.
The boot was a full zone boot rather than a world restore. The old process
couldn't record a clean-shutdown marker, because its world publishes had been
failing since 12:45 (#623).

To roll back:

1. Switch the checkout to `codex/rollback-sbs-20260923`, which is the tag
   `archive/staging-20260923`.
2. Copy `~/deploy-rollback/afdee58cc/dms` to `bin/server/dms_new` and
   `dms.backend` to `bin/server/.dms_new-backend`.
3. Restart the same way.

Both builds use the same database schema and world-recovery format.

## Rules from here

- Code reaches `master` through PRs.
- Staging deploys only tagged `master` commits.
- Every deploy, hold or rollback gets a line in the deploy log below.

## Deploy log

| UTC | Where | Event | Source |
| --- | --- | --- | --- |
| 2026-09-23 05:28 | Plesk staging | Built, then held; the announced reboot was postponed | `72238a8f5` |
| 2026-09-23 05:33 | Plesk staging | Checkout set back to the running code | `a6a2124c1` |
| 2026-09-23 07:52 | Staging | First boot on the new host | `0695680df` |
| 2026-09-23 09:33 | Staging | Login mode banner | `01f401b5a` |
| 2026-09-23 10:11 | Staging | Timed restart test, same binary | `01f401b5a` |
| 2026-09-23 11:12 | Staging | Stromvok ferry, binary `170fb0dd1629a886…` | `afdee58cc` |
| 2026-09-23 12:59 | Staging | `master` after #625, tag `staging-2026-09-23-1248`, binary `ec6d362be5e06197…`; full zone boot (#623) | `36e44f172` |

## Reproducing the audit

```bash
audit_master=66e787027d30f7855b8a3604b41ab41a7d910b67
audit_stable=3403c1db9b2a4649ace4383aa923fb0e05a3bf83
git merge-base "$audit_master" "$audit_stable"            # c6e94be85
git rev-list --left-right --count "$audit_master...$audit_stable"   # 29 29
git merge-tree --write-tree --name-only "$audit_master" "$audit_stable"
```

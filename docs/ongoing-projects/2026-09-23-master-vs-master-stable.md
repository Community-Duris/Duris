# `master` and `codex/master-stable`: branch history and staging

Status: findings, 2026-09-23. This explains which code staging and production run
and how staging came to run `codex/master-stable` instead of `master`. Times are
UTC. Liskin's commits are recorded at -0600 and have been converted.

The staging move itself is recorded in
[2026-09-23-server-environments.md](2026-09-23-server-environments.md).

## The two branches

- **`master`** is the shared main line where PRs land. Production deploys from
  it. Production's checkout is still `440248b17`, dated 2026-09-18, which is 136
  commits behind `master` (as of `69bf3ead5`).
- **`codex/master-stable`** is Liskin's stabilization branch.
  - It starts with `b839cadbd` at 2026-09-22 02:24 and runs through
    `72238a8f5` at 2026-09-23 04:37.
  - It holds 27 commits by Liskin. Almost all of them concern item custody,
    copyover and death recovery: atomic item transfers, corpse custody across
    copyover, recapturing saves after custody races, and disputed-death handling.
  - It has merged `master` twice: `e30e4a8e0` at 2026-09-22 12:43 and
    `16582e101` at 2026-09-23 03:56.
  - Nothing has gone back to `master`, and none of the 27 commits went through
    a PR. The branch is effectively staging's code line.

## What staging ran on Plesk

These events come from the Plesk checkout's reflog, `bin/server/history` and the
incident records under `~/.local/state/duris-incidents`. The reflog does not say
who ran each step.

| When | What happened |
| --- | --- |
| 2026-09-22, before 12:47 | Staging ran `master`. At 05:30, `feed29391` ("Ignore private/ incident recovery working sets") was committed on the staging checkout and later reached `master`. |
| 2026-09-22 06:37–06:55 | Staging crashed with SIGABRT on reboot twice: the `20260922-reboot-sigabrt` and `20260922-final-legacy-reboot-sigabrt` incident records, each with a gdb backtrace. A custody recovery was also performed: `20260922-custody-recovery`. |
| 2026-09-22 12:47 | The checkout switched from `master` to `codex/master-stable` at `e30e4a8e0`, four minutes after Liskin merged `master` into it. |
| 2026-09-22 13:25–18:30 | The checkout followed the branch through six fast-forwards: `4cb743b6a`, `3b337ee36`, `8eb9bf9ec`, `fc59cb570`, `eff289c76` and `a6a2124c1`. `bin/server/history` shows restarts at 16:22 and 18:40. |
| 2026-09-22 18:40 | `a6a2124c1` ("Preserve item text across copyover") started. This process ran until the staging move at 2026-09-23 07:37. Overnight it produced the custody save failures in #622, and its world snapshots stopped completing (#623). |
| 2026-09-23 05:27 | The checkout fast-forwarded to `72238a8f5`, which includes Liskin's disputed-death fixes `c555ecca1` and `72238a8f5`. |
| 2026-09-23 05:28 | That build was made and then held back instead of deployed. It is kept as `bin/server/history/dms_new.72238a8f5.rollback-held`. |
| 2026-09-23 05:33 | A local branch, `codex/rollback-sbs-20260923`, was created at `a6a2124c1`, matching the running binary. No reason for the hold is recorded. |

The fixes aimed at the overnight custody failures were built but never ran on
staging.

The checkout also carries two stashes from 2026-09-22: "server runtime manifest
worktree recovery" and "server shutdown-fix worktree recovery". Their purpose is
not recorded. Both came along with the move.

## What staging runs now

The staging move kept the checkout exactly as it was, on
`codex/rollback-sbs-20260923` at `a6a2124c1`, so as not to undo the hold.
Cherry-picks have been added since:

- `0695680df`: the configurable production-role port. It is `a7644b9f0` on
  `master` and `f2a1bcf18` on `codex/master-stable`.
- `01f401b5a`: the login mode banner. It is `be50f6c0e` on `master` and
  `3403c1db9` on `codex/master-stable`.
- `dbc3e5e34`, `4958a654f` and `afdee58cc`: the Stromvok ferry from #618,
  deployed at 2026-09-23 11:12. On `master` they are `e5c49f739`, `86fd07c15` and
  `89332a35f`. They reach `codex/master-stable` the next time Liskin merges
  `master`. If staging moves to the head of `codex/master-stable` before then,
  the ferry drops out until he does.

That branch exists only on the staging host, not on GitHub.

| Where | Code |
| --- | --- |
| `master` | The port change, banner and ferry (#618); none of Liskin's 27 commits |
| `codex/master-stable` | `master` as of `c6e94be85`, Liskin's 27 commits, the port change and banner |
| Staging (`duris-staging` on the production host) | `a6a2124c1`, the port change, banner and ferry |
| Production | `440248b17` (`master`, 2026-09-18) |

Staging lacks 13 commits that `codex/master-stable` has, and carries the three
ferry commits that it doesn't have yet. The missing 13 are:

- Liskin's last nine, including the corpse-custody and disputed-death fixes:
  `d7098aa77`, `13641d24e`, `6380a4bc3`, `e08b0f63d`, `cb8abae92`, `16582e101`,
  `42877643c`, `c555ecca1` and `72238a8f5`
- four from `master`, merged in through `16582e101`: the Frost Beam fix (#617,
  `c6e94be85` and `dd4d97fc4`), and the god-list changes `ad1639842` and
  `a71fdfee7`

## Open questions

- **Why was `72238a8f5` held?** Ask Liskin before moving staging to the head of
  `codex/master-stable`. Moving would bring in the #622 fixes and the four
  `master` commits.
- **Converging the lines.** #622 asks for Liskin's custody commits to reach
  `master` through PRs, so that `master`, staging and eventually production stop
  drifting apart.
- **Production.** Production is 136 commits behind `master`, and its next
  deploy is a separate decision.
- **Rebuilding staging's branch.** The branch isn't on GitHub. If the checkout
  is lost, recreate it as `a6a2124c1` plus the cherry-picks above, or push it to
  `origin`.

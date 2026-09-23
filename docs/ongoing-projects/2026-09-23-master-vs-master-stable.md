# `master` and `codex/master-stable`: branch history and staging

Status: findings and plan, 2026-09-23, updated about 11:30. This explains which
code staging and production run, how staging came to run `codex/master-stable`
instead of `master`, and how to bring the lines back together. Times are UTC.
Liskin's commits are recorded at -0600 and have been converted.

The staging move itself is recorded in
[2026-09-23-server-environments.md](2026-09-23-server-environments.md).

## The two branches

- **`master`** is the shared main line where PRs land. Production deploys from
  it. Production's checkout is still `440248b17`, dated 2026-09-18, which is 144
  commits behind `master` (as of `33837a300`).
- **`codex/master-stable`** is Liskin's stabilization branch.
  - It starts with `b839cadbd` at 2026-09-22 02:24 and runs through
    `72238a8f5` at 2026-09-23 04:37.
  - It holds 27 commits by Liskin, 24 regular commits and 3 merges, plus two
    cherry-picks from `master`: the port change `f2a1bcf18` and the banner
    `3403c1db9`.
  - Almost all of Liskin's commits concern item custody, copyover and death
    recovery: atomic item transfers, corpse custody across copyover, recapturing
    saves after custody races, and disputed-death handling.
  - It has merged `master` twice: `e30e4a8e0` at 2026-09-22 12:43 and
    `16582e101` at 2026-09-23 03:56.
  - None of Liskin's commits has reached `master`, and none went through a PR.
    The branch is effectively staging's code line.

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
  `89332a35f`. They reach `codex/master-stable` only when Liskin next merges
  `master`. If staging moves to the head of `codex/master-stable` before then,
  the ferry drops out until they do.

That branch exists only on the staging host, not on GitHub.

## Four code lines

As of about 11:30, four places run four different lines of code:

| Where | Commit | Code |
| --- | --- | --- |
| `master` | `33837a300` | Everything merged through PRs, plus the port change, banner and ferry. None of Liskin's 27 commits. |
| `codex/master-stable` | `3403c1db9` | `master` as of `c6e94be85`, Liskin's 27 commits, and the port change and banner. It lacks 23 regular commits from `master`, including the ferry. |
| Staging (`duris-staging` on the production host) | `afdee58cc` | `a6a2124c1`, the port change, banner and ferry. It exists only on the staging host. |
| Production | `440248b17` | `master` from 2026-09-18, 144 commits behind. |

Staging lacks 13 commits that `codex/master-stable` has, and carries the three
ferry commits that it doesn't have yet. The missing 13 are:

- Liskin's last nine, including the corpse-custody and disputed-death fixes:
  `d7098aa77`, `13641d24e`, `6380a4bc3`, `e08b0f63d`, `cb8abae92`, `16582e101`,
  `42877643c`, `c555ecca1` and `72238a8f5`
- four from `master`, merged in through `16582e101`: the Frost Beam fix (#617,
  `c6e94be85` and `dd4d97fc4`), and the god-list changes `ad1639842` and
  `a71fdfee7`

On the other side, `master` has 23 regular commits that `codex/master-stable`
lacks:

- the port change and banner, which are already on `codex/master-stable` as
  identical cherry-picks
- the three ferry commits
- test and contract maintenance: `843ee795b`, `63e2ae065`, `72fa2ffeb`,
  `260c63994` and `a270967b6`
- the epic-zone seed refresh `33b19169a`
- dependency and CI bumps
- documentation

## Merging `codex/master-stable` into `master`

A trial merge (`git merge-tree`, no refs changed) of `3403c1db9` into
`33837a300` touches 82 files (+2,322/−367) and conflicts in 8. The merge base is
`c6e94be85`. The port change and banner exist on both sides as identical
changes and do not conflict.

| Conflicting file | `master` commits | `codex/master-stable` commits | What resolves it |
| --- | --- | --- | --- |
| `src/world/handler.c` | `843ee795b` (clang-format sweep), `89332a35f` (ferry ship pointer) | `6380a4bc3`, `fb0c112dd`, `a6dd863a8` | A real code decision: keep the ferry fix and Liskin's corpse-custody changes together |
| `src/item/item_ownership_runtime.c` | `843ee795b` | `6380a4bc3`, `fb0c112dd` | Formatting only on `master`: take Liskin's version, re-run clang-format |
| `src/flatfile/flatfile_world_item_repository.c` | `843ee795b` | `fb0c112dd` | Formatting only |
| `src/classes/necromancy.h` | `843ee795b` | `fb0c112dd` | Formatting only |
| `tests/async/corpse_lifecycle_repository_mysql_harness.cpp` | `843ee795b` | `13641d24e`, `d7098aa77`, `fb0c112dd`, `0da7c6bbe` | Formatting only |
| `tests/async/flatfile_corpse_repository_harness.cpp` | `843ee795b` | `fb0c112dd` | Formatting only |
| `tests/async/test_difficulty_dials.py` | `72fa2ffeb` (contract values) | `42877643c` | Test contract: reconcile the assertions |
| `tests/async/test_kingdom_contract.py` | `260c63994` (refactored item code) | `42877643c` | Test contract: reconcile the assertions |

Six of the eight conflicts come from `master`'s clang-format sweep `843ee795b`
and can be resolved mechanically. The real decisions are `handler.c` and the two
contract tests. Every day the lines stay apart, both keep rewriting the same
custody and item code, and this list grows.

## Process problems found

- **No review.** 27 commits were deployed to a public server with no PR or
  review. None of them is on `master`.
- **Live iteration.** Several commits fix commits from hours earlier, and one
  title appears twice: `3b337ee36` and `742cefbd6` ("Fix divine claim custody
  revocation").
- **No deploy record.** The 05:27–05:33 build, hold and rollback left no note.
  The deployed branch was never pushed, and the two stashes are unexplained.
- **Leftovers on Plesk.**
  - Stale automation was left running: orphaned `tail`/`ugrep` processes and a
    `pgrep` loop that matches its own command line and never exits.
  - The staging database could only be administered by Plesk's `duris` login.
- **Overnight impact on staging (#622).**
  - 12 characters had saves refused, and six of them kept failing repeatedly.
  - Two lost hours of progress.
  - Logouts were refused up to 96 times an hour, and the service's shutdown
    cancelled itself and ended in a SIGKILL (#621).
  - The fix was built and then held without a note.

## Plan

1. **Stop the drift.** Nothing more is deployed from `codex/master-stable`.
   Changes reach `master` through PRs only.
2. **One PR from `codex/master-stable` into `master`.**
   - Resolve the eight conflicts above: six mechanically, then `handler.c` and
     the two contract tests by judgement.
   - Run CI, review and merge.
   - Settle the held `72238a8f5` in that PR.
3. **One chain after that.** PR, then `master`, then a tagged staging deploy,
   then production when chosen. Staging runs plain `master`; only its `.env`
   differs.
4. **Guardrails.**
   - Tag and push every staging deploy, for example `staging-2026-09-23-1112`.
   - Keep a deploy log with one line per deploy, hold or rollback: who, which
     commit and why.

## Decisions needed

- **Who resolves the conflicts.** Either Liskin, who knows the intent behind the
  conflicting hunks, or someone else with Liskin reviewing.
- **Why `72238a8f5` was held.** Ask Liskin before it reaches staging or `master`.
- **Production.** When to deploy. Production is 144 commits behind `master`.
- **Rebuilding staging's branch.** Until staging tracks `master`, its branch
  exists only on the staging host. If the checkout is lost, recreate it as
  `a6a2124c1` plus the cherry-picks above, or push it to `origin`.

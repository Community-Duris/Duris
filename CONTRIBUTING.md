# Contributing

## Working Branches

Make focused changes on a topic branch and open a pull request against `master`.
Keep unrelated worktree changes intact and do not commit generated files, credentials,
logs, player/account data, backups, or anything under `bin/`.

## Recommended Git Workflow

This repository uses `master` as its default branch, so the workflow below uses
`master` where a generic Git workflow might use `main`.

### The loop

1. **Sync first:** `git switch master && git pull --ff-only`
2. **Pick branch or worktree:** need two working copies checked out at once
   (parallel agents, long build, reviewing someone else's PR)? → worktree.
   Otherwise → branch.
3. **Branch:** `git switch -c feature/my-thing`
4. **Worktree:** `git worktree add -b feature/my-thing ../duris-my-thing origin/master`
   → `cd ../duris-my-thing` → copy `.env`, install dependencies (worktrees do
   not inherit untracked files).
5. Do the unit of work—code and tests. Nothing else belongs in this branch.
6. **Checks run as hooks,** not rituals: tests, lint, and Semgrep via
   `pre-commit` / `pre-push`.
7. Commit as you go—small commits are fine; they get squashed.
8. `git push -u origin feature/my-thing`
9. Open the PR and mark it ready for review. **The PR title becomes the commit
   message on `master`**—write it accordingly.
10. CI and review bots (CodeRabbit, SonarQube, DeepSource) run on the PR.
11. Fix review issues on the same branch: commit → push → **re-request review**
    explicitly.
12. **If `master` moved:** `git fetch origin && git merge origin/master`, then
    push normally. No rebase, no force-push. Escape hatch: `git merge --abort`.
13. Green checks → **Squash and merge**.
14. **Remove the worktree first** (if used): `git worktree remove ../duris-my-thing`.
15. `git switch master && git pull --ff-only && git fetch --prune`
16. `git branch -D feature/my-thing`—**`-D`, not `-d`**: squash merge leaves
    no ancestry link, so Git will not consider the branch merged.
17. Repeat from step 1.

### Rebase

Skip it entirely in this flow.

- **Syncing onto `master`** (step 12): merge, not rebase. Squash-merge discards
  branch history anyway, so rebasing buys nothing and costs a force-push—which
  detaches review comments and breaks GitHub's "changes since your last review"
  diff.
- **Cleaning history before merge:** unnecessary. Squash-merge collapses everything into one commit.
- Rebase is only worth it if you are rebase-merging or merge-committing. If you
  ever do force-push, use `--force-with-lease --force-if-includes` (lease alone
  is defeated by any background fetch).

### Rules

- Keep branches under approximately two days.
- Never rebase anything others have pulled.
- Squash-merge is the default for solo/feature work; merge commits and
  rebase-merge are for teams with specific history preferences.
- Set the repository's squash-message default to "PR title and description," not
  the concatenated commit list.
- Set `fetch.prune = true` globally.
- If branch protection requires branches to be up to date, sync whenever
  `master` moves—conflicts or not.
- Use one worktree per concurrent task or agent; removing it is the whole
  cleanup.

## Build and Test

Run the smallest relevant regression while iterating. After changing server C/C++,
build with `make -C src` and format touched lines with `./scripts/format.sh`.

Before handoff, run the complete safe gate:

```bash
make test-all
```

Database suites are separate because they require isolated databases:

```bash
make test-db
```

Never run migration, wipe, load, or fault tooling against production.

## Commits and Pull Requests

- Keep each commit to one logical, reviewable change.
- Describe what changed, why, and which checks passed.
- Add a focused regression whenever behavior changes.
- Do not add co-author, attribution, session, or signed-off-by trailers.
- Address review and CI findings without weakening existing safety or test contracts.

See [Building](docs/guides/BUILDING.md), [Testing](docs/guides/TESTING.md), and the
[Operations Runbook](docs/operations/RUNBOOK.md) for the verified commands and safety boundaries.

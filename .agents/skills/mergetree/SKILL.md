---
name: mergetree
description: >-
  Merge a DurisMUD feature worktree branch into master, resolve conflicts, validate the result,
  and publish master. Use when asked to integrate a worktree or finish an interrupted worktree
  merge in this repository.
---

# Merge a worktree into master

Read the repository `AGENTS.md` and `README.md` before acting. Invocation authorizes merge-related
commits and an ordinary push to `master`; it does not authorize unrelated commits, history rewrites,
force pushes, branch or worktree deletion, production operations, or exposing `.env`.

Proceed without routine approval when state and intent are clear. Stop rather than guess if the
source or feature intent is ambiguous, unrelated state cannot be preserved, no merge base exists,
or safe completion requires destructive cleanup, history rewriting, unavailable external state, or
a broader product decision.

## Preflight

1. Use `git worktree list --porcelain` and Git refs to identify the source worktree/commit and the
   worktree holding `master`; do not infer a ref from a directory name or choose among plausible
   sources without user direction.
2. Read only the `ENVIRONMENT` value from `.env`, without printing the file. Proceed only for a
   local or development environment.
3. Inspect status in both worktrees. Never discard, stash, or include unrelated edits. After focused
   validation, commit separable source work only when it is clearly part of the requested feature.
   If `master` has unrelated edits, leave them untouched and integrate on a temporary
   branch/worktree; update the canonical checkout afterward only when a fast-forward preserves them.
4. Fetch `origin`. Fast-forward a clean local `master` when it is only behind. Inspect local-only
   commits before publishing them. If they are unrelated, integrate from `origin/master` on an
   isolated branch when the source does not depend on them; otherwise stop for user direction. Merge
   remote changes into an in-scope diverged target; never rebase or force-update `master`.
5. Record the exact target and source commits.

## Review and merge

Require a merge base. Establish the feature intent from commits,
`git diff master...<source>`, tests, documentation, and call sites. Inspect name/status, stat, and
`git diff --check master...<source>`. If the source is already an ancestor of `master`, publish the
reviewed `master` if needed and stop without an empty merge commit.

Immediately before an actual merge, create a unique, non-overwriting
`backup/pre-merge-<timestamp>` ref at the target commit.

Give the user a concise progress update with the exact refs, feature intent, expected conflicts,
validation, and publication target. Then run:

```bash
git merge --no-ff --no-commit <source-ref>
```

Keep the merge state while resolving conflicts. For each conflict, inspect the base, ours, theirs,
relevant history, and consumers. Preserve compatible target behavior and the source feature with
the smallest semantic edit; never select a whole side merely to clear markers.

Merge-specific repository checks:

- When server sources are added or removed, keep the object/source lists in `src/Makefile` accurate.
- Resolve area-data conflicts in their source inputs, not generated `areas/world.*` outputs, then
  regenerate through the repository targets.

Before committing, require no unmerged paths or conflict markers, and review `git status`, the full
staged diff, its stat, and `git diff --cached --check` for scope and accidental regressions.

## Validate and publish

Run checks proportional to the merged change:

- C/C++ changes: `./scripts/format.sh --check`, the focused regression, and `make -C src`.
- Broader gameplay, build, or world changes: focused checks followed by `make test-all`.
- Documentation, skill, or isolated tooling changes: their focused validator/check; record why a
  server build is unnecessary.

Fix in-scope failures and rerun affected checks. Commit with a concise human-authored merge message
without bypassing hooks.

Fetch `origin` again before publication. If `origin/master` moved, integrate it without rebasing or
force, review the resulting diff, and rerun affected checks. Push with an ordinary non-force push
and verify `origin/master` contains the merge commit. Require zero divergence when the canonical
checkout can be updated safely; otherwise leave its unrelated edits intact and report its state.

Report the source and merge commits, notable resolutions, checks and results, any backup ref, and
the published ref.

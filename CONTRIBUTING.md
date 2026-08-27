# Contributing

## Working Branches

Make focused changes on a topic branch and open a pull request against `master`.
Keep unrelated worktree changes intact and do not commit generated files, credentials,
logs, player/account data, backups, or anything under `bin/`.

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

See [Development](docs/development.md), [Testing](docs/TESTING.md), and the
[Operations Runbook](docs/RUNBOOK.md) for the verified commands and safety boundaries.

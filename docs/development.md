# Development Guide

## Daily Commands

| Command | Purpose |
|---------|---------|
| `make -C src` | Build the C++20 server as `bin/server/dms_new`. |
| `./scripts/format.sh` | Format touched C/C++ lines. |
| `python3 tests/async/test_<feature>.py` | Run one focused regression. |
| `make test-all` | Build maintained targets and run the complete safe regression gate. |
| `make test-db` | Run isolated Docker/MySQL suites. |
| `make security-check` | Generate and validate the local security baseline. |
| `./scripts/start_mud.sh` | Start the configured local server. |
| `scripts/healthcheck.sh` | Verify game and database-pool readiness. |

## Working Rules

- Mutable game objects stay on the game thread; workers receive bounded values only.
- Keep migrations additive, guarded, and re-runnable where practical.
- Use the configured development database and never production for tests or migrations.
- Preserve unrelated worktree changes and keep generated artifacts below `bin/`.

See [Building](BUILDING.md), [Testing](TESTING.md), [Formatting](formatting.md), and
[Contributing](../CONTRIBUTING.md) for the detailed contracts.

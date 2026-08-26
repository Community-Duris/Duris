# AGENTS.md

## Repository guide

- `.env` will indicate whether this is local/dev or production/remote
- DB credentials is in `.env`
- Use `scripts/start_mud.sh` to start/run the game
- NEVER add co-authors, NEVER add attributions, NEVER add `Claude-Session`, NEVER add signed-off-by lines
- `src/` contains the server. Its `.c` files are compiled as C++20 with `g++`.
- `tests/async/` contains focused regression and source-contract tests.
- `areas/` holds world data and generators; `migrations/` is the authoritative schema-change location.
- Read `README.md` for setup, runtime, and database details.
- An in-game account is set up for testing, with credentials in `.env`: `GAME_ACCOUNT_NAME` `GAME_ACCOUNT_PASSWORD` `GAME_ACCOUNT_CHARACTER_NAME`

## Working conventions

- Keep changes narrow and follow the style of nearby legacy code. `.clang-format` is authoritative for touched C/C++ code. Run `./scripts/format.sh` (changed lines only) or `--check` to verify. `./scripts/install-hooks.sh` enforces it at commit time.
- Build with `make -C src`; the executable is `bin/server/dms_new`. All compiled
  artifacts belong under `bin/` and must not be committed.
- Run the smallest relevant test directly, for example `python3 tests/async/test_<feature>.py` or its `run_<feature>.sh` wrapper. Run `make -C src` after C/C++ changes.
- Add or update a focused regression test when behavior changes. Report any validation that could not be run.

## Safety

- Never run migrations, wipes, or operational scripts against production.
- Test schema changes on a backed-up clone; keep migrations additive, guarded, and re-runnable where practical.
- Do not edit or commit credentials, private keys, logs, player/account data, archives, generated area outputs, or local environment files unless the task explicitly requires it.
- Preserve unrelated worktree changes and avoid destructive Git commands.

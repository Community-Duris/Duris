# AGENTS.md

## Repository guide

- Never add co-authors, never add attributions, never add signed-off-by lines
- `src/` contains the server. Its `.c` files are compiled as C++20 with `g++`.
- `tests/async/` contains focused regression and source-contract tests.
- `areas/` holds world data and generators; `migrations/` is the authoritative schema-change location.
- Read `README.md` for setup, runtime, and database details.

## Working conventions

- Keep changes narrow and follow the style of nearby legacy code. Do not mass-format files; `.clang-format` is authoritative for touched C/C++ code.
- Build with `make -C src`; the executable is `src/dms_new`. Do not commit binaries or `obj/` artifacts.
- Run the smallest relevant test directly, for example `python3 tests/async/test_<feature>.py` or its `run_<feature>.sh` wrapper. Run `make -C src` after C/C++ changes.
- Add or update a focused regression test when behavior changes. Report any validation that could not be run.

## Safety

- Use a development database and a non-7777 port for testing. Never run migrations, wipes, or operational scripts against production.
- Test schema changes on a backed-up clone; keep migrations additive, guarded, and re-runnable where practical.
- Do not edit or commit credentials, private keys, logs, player/account data, archives, generated area outputs, or local environment files unless the task explicitly requires it.
- Preserve unrelated worktree changes and avoid destructive Git commands.

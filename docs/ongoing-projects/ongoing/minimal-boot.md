# Minimal boot end-to-end

## Goal

Make the existing minimal-world mode a supported, documented, and verified way to
start Duris with the smallest tracked world dataset. The result must boot through
the maintained startup path, accept a client connection, and place the configured
test character into a valid room without regenerating or loading the full world.

## Working contract

- `./scripts/cycle_mud.sh --dev --minimal` starts a foreground development server
  on the development ports and uses the tracked `areas_mini/mini.*` world files.
- `./scripts/start_mud.sh --dev --minimal` provides the corresponding supervised
  or background entry point.
- Minimal mode must not regenerate `areas/world.*` or mutate tracked world data.
- Boot must reach the game loop with a clear log message identifying minimal mode.
- A real client can connect and the configured test account/character can enter a
  valid room in the minimal world.
- Normal startup behavior remains unchanged.

## Findings

### 2026-08-28: baseline inspection

- `src/comm.c` already accepts the undocumented legacy `-m` flag and sets
  `mini_mode = 1`; `boot_db()` then reads `areas_mini/mini.mob`, `.obj`, `.wld`,
  and `.zon`.
- The tracked `areas_mini/` directory also contains minimal quest, shop, table,
  and weather data, although several boot helpers still reference `areas/world.*`.
- The main runtime already skips many full-world-only subsystems and persistence
  worker startup when `mini_mode` is active.
- `scripts/cycle_mud.sh` only recognizes `--dev`, always regenerates the full world,
  and never forwards `-m` to the server.
- `scripts/start_mud.sh` forwards arguments but may route a no-argument invocation
  through the user service. An explicit minimal invocation will continue to use the
  worktree-local cycle script.
- No focused regression test currently covers argument parsing or minimal startup.

### 2026-08-28: first runtime reproduction and implementation

- Direct `bin/server/dms_new -m 4090` reached the game loop in 14 ms and accepted
  a Telnet connection.
- Account authentication and character selection worked, but loading the configured
  character failed safely. The player-load log reported item materialization outcome
  `unknown_prototype`; a read-only query showed that both inventory records use object
  vnum `7`, while `areas_mini/mini.obj` jumped directly from `#6` to `#8`.
- Restored object vnum `7` (the master spellbook) from `areas/obj/limbo.obj`, which is
  the source used for that prototype in the generated full world.
- Added direct server support for the descriptive `--minimal` option while retaining
  the legacy `-m` spelling.
- Added `--minimal` to `scripts/cycle_mud.sh`. It implies development ports, validates
  the tracked minimal files, skips area-tool builds and full-world generation, and
  forwards minimal mode to the server.
- Isolated minimal boot from generated full-world support files: shops now read the
  empty `areas_mini/world.shp`, while weather and random tables read their
  `areas_mini/` equivalents and data-driven full-world triggers remain disabled.
- Removed stale zone reset commands that referenced many absent full-world prototypes
  and removed room 1207's exit to nonexistent room vnum `0`.
- Added `tests/async/test_minimal_boot.py` to protect the launcher, data-file routing,
  dataset framing, required test object, empty reset contract, and void-exit fix.
- A second direct boot exposed out-of-range precipitation values in
  `areas_mini/world.weather`; all affected values now match the valid upper index
  (`8`), and the regression test checks all 100 weather rows and bounds.
- Character entry then succeeded in room 1200, but disconnect could not complete a
  terminal save because the old mini mode skipped all durability workers. Minimal
  mode now starts player-save and critical-command pipelines, while still skipping
  locker and maintenance workers that depend on full-world runtime features.
- Minimal mode now also disables random-zone creation and skips full-world state
  restoration (ships, arena, wagons, corpses, saved ground items, shopkeepers,
  associations, justice, and artifacts) plus zone-database publication.

## Progress log

- [x] Created and published the `minimal-boot` branch.
- [x] Located the legacy server mode and minimal dataset.
- [x] Reproduce a direct `-m` boot and inventory failures.
- [x] Expose a safe `--minimal` option through the maintained startup scripts.
- [x] Add focused regression coverage.
- [x] Build and run formatting checks.
- [ ] Verify boot, client connection, and configured-character entry end to end.
- [ ] Document operator usage and final validation evidence.

## Validation evidence

- `make -C src` — passed after the implementation changes.
- `bash -n scripts/cycle_mud.sh` — passed.
- `./scripts/cycle_mud.sh --help` — passed and documents `--minimal`.
- `python3 tests/async/test_minimal_boot.py` — passed.
- `./scripts/format.sh --check` — passed.
- Direct `bin/server/dms_new --minimal 4090` boot — passed in 7 ms with 1 zone,
  67 rooms, 171 mobile prototypes, 805 object prototypes, no random zones, and no
  full-world state restoration.
- Real Telnet account authentication, configured-character selection, asynchronous
  snapshot/item load, entry into room 1200, and `look` — passed.
- Client disconnect produced no new terminal-save failure or deferred retry, and
  `SIGINT` then shut the server down normally with exit status 0.
- Maintained `scripts/cycle_mud.sh --minimal` startup path remains to be verified.

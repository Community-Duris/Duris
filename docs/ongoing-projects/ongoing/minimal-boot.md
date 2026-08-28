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

## Progress log

- [x] Created and published the `minimal-boot` branch.
- [x] Located the legacy server mode and minimal dataset.
- [ ] Reproduce a direct `-m` boot and inventory failures.
- [ ] Expose a safe `--minimal` option through the maintained startup scripts.
- [ ] Add focused regression coverage.
- [ ] Build and run formatting checks.
- [ ] Verify boot, client connection, and configured-character entry end to end.
- [ ] Document operator usage and final validation evidence.

## Validation evidence

No runtime validation has been completed yet.


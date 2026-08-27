# Architecture

DurisMUD is a single-process, event-driven MUD server derived from the DikuMUD
lineage, compiled as C++20 (`g++ -std=c++20`) from C-style sources. It serves
players over plain telnet, TLS telnet, and WebSocket; durable player state lives
in MySQL, while Redis optionally holds immutable crash-recovery world generations.

Related reading: [CODEBASE.md](CODEBASE.md) for the module map,
[DATABASE.md](DATABASE.md) for persistence details, [RUNBOOK.md](RUNBOOK.md)
for operations. A visual overview lives in
[diagrams/duris-server-architecture.html](diagrams/duris-server-architecture.html).

## Process model

One process (`bin/server/dms`, staged as `bin/server/dms_new`). There is no
fork-per-connection, player-save fork, or world-save fork; all socket I/O is
multiplexed in a single `select()` loop. Concurrency includes:

- Three asynchronous MySQL persistence worker threads (see [DATABASE.md](DATABASE.md)).
- A private player-journal append dispatcher and keyed player-save workers.
- One immutable world-recovery publisher worker when Redis recovery is enabled.
- The main game loop blocking signals (including `SIGSEGV`, handled internally)
  around each iteration so workers cannot interrupt pulse processing.

Legacy hostname lookup may still use a short-lived child. It is unrelated to
persistence and never receives player or world snapshot work.

`main()` (`src/comm.c:205`) parses flags, then `game_loop()` (`src/comm.c:704`)
runs until shutdown. Exit codes are meaningful — `scripts/cycle_mud.sh`
interprets them to decide whether to restart (see [RUNBOOK.md](RUNBOOK.md)).

### Command-line options

| Option | Effect |
|--------|--------|
| `[port]` | Listen port; must be > 1024. Default 7777 (`DFLT_PORT`, `src/config.h:21`). |
| `-C` | Copyover boot — recover player sockets from `copyover.dat`. |
| `-m` | Mini mode (reduced area set); also disables ferries. |
| `-z` | Mini mode with the area debugger on. |
| `-f` | Disable ferries. |
| `-l` | Disable random encounters. |
| `-s` | Suppress special-procedure assignment. |
| `-p` | Allow password change without the old password. |
| `-d <dir>` | Data directory (default `.`). |
| `--migrate-all` | Migration mode. |
| `--material-rarity-report[=dir]` | Generate material rarity report and exit. |

## Ports

| Port | Purpose | Defined in |
|------|---------|------------|
| 7777 | Plain telnet (production default) | `src/config.h:21` |
| 7778 | TLS telnet (`SSL_PORT`) | `src/config.h:22` |
| port+1 | TLS telnet when a non-default port is given | `src/comm.c` |
| 4050 | WebSocket listener (`WS_PORT`) | `src/websocket.h:14` |

The listen port also selects the database: 7777 uses production (`duris`),
any other port uses development (`duris_dev`). See [DATABASE.md](DATABASE.md).

## Game loop and timing

`game_loop()` is a classic pulse-based loop:

- Each iteration blocks at most `OPT_USEC` microseconds (250 ms,
  `src/config.h:82`) — nominally 4 pulses/second.
- Per-pulse work is dispatched by pulse counters using `PULSE_*` constants
  (`src/config.h:84-91`): combat rounds every 16 pulses, mobile updates every
  30, ships/vehicles every 2, spellcasting every 9, etc.
- Socket readiness comes from `select()` over input/output/exception sets.
- A per-pulse time budget is enforced between event-wheel callbacks (see
  below). Because the check happens *between* callbacks, one slow job overruns
  the pulse regardless of policy — an expensive callback has to be made cheaper
  or sliced, not merely deprioritized.

Descriptor structures come from a custom pooled allocator (`mm_create("SOCKET",
...)`) rather than raw malloc.

## Event wheel

Deferred and periodic work runs through the event system (`src/events.c`,
`src/new_events.c`): timed callbacks stored on a wheel, executed inside the
game loop between pulses. Budget telemetry is exposed via `NEVENT BUDGET` log
lines and `src/latency_trace.c`; `NEVENT SLOW` marks a loop over 50 ms.

Each pulse is bounded by a wall-clock budget (`NEVENT_BUDGET_USEC_DEFAULT`,
25 ms) and a callback count cap (`NEVENT_MAX_CALLBACKS_DEFAULT`). Both are
overridable at runtime — see [CONFIGURATION.md](CONFIGURATION.md#diagnostics).
The time budget is meant to be the binding limit; a count cap low enough to end
pulses at half the time budget starves the wheel.

Three properties of the wheel are load-bearing and were each a live incident:

- **Deferral covers the whole unscanned suffix.** When a pulse runs out of
  budget, every remaining due event moves to the next pulse, in order, and every
  event left behind still has its timer decremented. An earlier version moved
  only the leading contiguous run of due events, so a due event sitting behind a
  not-yet-due one was stranded in its ring bucket for a full revolution
  (300 pulses ≈ 75 s), repeatedly — and events the scan never reached lost a
  whole revolution off long timers on every saturated pulse.
- **Player-event promotion is not gated on the callback budget.** Promotion used
  to require `executed < max_callbacks`, which made the priority mechanism inert
  on exactly the saturated pulses it exists for. It now costs at most one
  over-cap callback per pulse.
- **Character-wide maintenance is sliced.** `generic_char_event` (`handler.c`)
  swept every character in one callback (17.8 ms average, 24.1 ms peak against a
  25 ms budget). It runs in four slices, one per invocation, rescheduled at a
  quarter of the old delay. The slice is a hash of the character's address, so
  it is stable for the character's lifetime: every character is still visited
  exactly once per 20 s, none skipped or done twice.

### Command gate

`comm.c` will not dequeue a descriptor's input while `PLR2_WAIT` is set
(`CAN_ACT(ch)`). The bit is set by `CharWait()` and cleared by the `event_wait`
event it schedules — so any path where that event is not scheduled, or is
starved, leaves the player silently unable to act with the connection still up.

`CharWait()` therefore clamps a negative delay, clears `PLR2_WAIT` again if
`add_event()` refused the event, and records `ch->specials.wait_until_pulse`, an
absolute deadline in `ne_event_tick` pulses (the delay plus a 2 s grace). The
gate in `comm.c` clears the bit before reading input if no `event_wait` is
scheduled *or* the deadline has passed, and logs which case it was. A player can
no longer be gated for longer than the wait that was actually requested,
independently of the health of the event system. `wait_until_pulse` is
runtime-only and never saved.

## Boot sequence

`boot_db()` (`src/db.c:406`) loads, in order: configuration/properties,
command tables, help command attributes, world files generated by the area
toolchain (`areas/world.*` — rooms, mobs, objects, zones, quests, shops,
triggers), then boots zones via `boot_zones()` (`src/db.c:1523`). On a fresh
checkout these combined files are produced by rebuilding the `make_*` helpers
in `areas/src/` and running `areas/m_slow` (see [BUILDING.md](BUILDING.md)).

After boot, two recovery paths may apply before the loop starts:

- **Copyover recovery** (`copyover_boot`, `src/copyover.c`): listening sockets
  and live player connections are re-inherited from `copyover.dat`; combat state
  is restored by `copyover_restore_combat()`.
- **Redis crash recovery**: if the previous process died uncleanly, the world
  state snapshot written to Redis is loaded back (`redis_load_world_state()`)
  and then cleared.

## Persistence

All durable state is MySQL. Writes are funneled through an async persistence
queue (`src/persistence_queue.c`) served by three worker threads — item,
scalar, and large-payload queues — backed by a fixed-size connection pool
(`src/sql_pool.c`). Raw SQL execution for the large-payload worker lives in
`src/sql_persistence_raw.c`. If pool initialization fails at boot, workers fall
back to synchronous execution rather than refusing to start. Required tables
are verified at boot; missing schema aborts startup instead of silently losing
saves.

Player checkpoints are captured into immutable, revisioned DTOs on the game thread.
A bounded append dispatcher durably frames them in the typed journal, then keyed
workers apply them transactionally with per-PID ordering and exact revision ACKs.
Ordinary mutation and checkpoint routes perform no MySQL, Redis, or filesystem I/O on
the simulation thread. Terminal transitions require either the matching database ACK
or a successful journal handoff before live state may be destroyed.

Redis complements MySQL with floor-delta tracking and immutable world-recovery
generations (`src/world_recovery_pipeline.c`, `src/redis.c`). World graph capture is
incremental and bounded on the game thread; the publisher receives owned bytes only.
It writes a sequence-keyed payload before atomically advancing the current pointer and
metadata. Restore validates schema, completeness, checksum, sequence, and age.

Non-idempotent Phase 02 gameplay effects use a separate critical-command coordinator
(`src/critical_command_coordinator.c`). Its immutable, non-coalescing commands carry a
stable 128-bit operation ID and sorted entity-key set. Conflicting key sets execute in
acceptance order, unrelated sets may run concurrently, and exact typed completion is
required to release a gameplay fence. A checksummed local journal preserves accepted
commands through retry and restart. A typed prepared-statement repository applies each
operation through one InnoDB inbox/state/outbox transaction, resolves duplicate or
ambiguous commits by stable operation ID, and classifies retryable database errors. A
bounded at-least-once dispatcher retains typed outbox rows through delivery, retry,
dead-letter, restart, and operator reconciliation. The generic mutation and destination
remain deliberately test-only until later Phase 02 sessions add gameplay domains.

Details and schema management: [DATABASE.md](DATABASE.md).

## Networking

- **Telnet** (`comm.c`): line-based, with MCCP compression support (`mccp.c`).
- **TLS telnet** (`ssl.c`): same protocol over TLS; certificate/key expected as
  `duris.crt` / `duris.key` in the repository root (symlinks recommended).
- **WebSocket** (`websocket.c`): RFC 6455 server on port 4050 with HTTP upgrade
  handshake, intended for browser clients; message payloads are JSON
  (`json_utils.c`).
- **GMCP** (`gmcp.c`): outbound game data to capable clients over telnet or
  WebSocket.
- Hostname resolution and login/nanny flow are in `nanny.c`; interpreter and
  command dispatch in `interp.c`.

## Studio procs (triggers)

Builder-authored behaviors (mob speech/give/death hooks, boot-time triggers)
are data-driven from `areas/world.trg` and dispatched by the studio-proc engine
(`src/studioproc.c`, `src/studioproclib.c`). Engine hooks are four one-line
call sites added to existing code paths; everything else is table-driven.
Design rationale: [STUDIOPROC.md](STUDIOPROC.md). Builder grammar:
[`src/howto_trg.txt`](src/howto_trg.txt).

## Ships

The ship simulation (sailing, cargo, naval combat, NPC crews/shops) is a
self-contained subsystem under `src/ships/`, built into its own object files
and linked into the main binary. Ship instances persist through the normal
persistence layer plus Redis ship snapshots.

## Help system

In-game `help` is database-backed: `wiki_help()` queries the `pages` table and
renders wiki-formatted text (`src/wikihelp.c`), augmented by command attributes
loaded from `docs/lib/information/command_attributes.txt`. Pipeline details:
[HELP_SYSTEM.md](HELP_SYSTEM.md).

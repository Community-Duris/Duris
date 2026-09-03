# Architecture

DurisMUD is a single-process, event-driven MUD server derived from the DikuMUD
lineage, compiled as C++20 (`g++ -std=c++20`) from C-style sources. It serves
players over plain telnet, TLS telnet, and WebSocket; durable player state lives
in MySQL, while Redis optionally holds immutable crash-recovery world generations.

Related reading: [CODEBASE.md](CODEBASE.md) for the module map,
[DATABASE.md](DATABASE.md) for persistence details, [RUNBOOK.md](../operations/RUNBOOK.md)
for operations. A visual overview lives in
[diagrams/duris-server-architecture.html](../diagrams/duris-server-architecture.html).

## Process model

One process (`bin/server/dms`, staged as `bin/server/dms_new`). There is no
fork-per-connection, player-save fork, or world-save fork; all socket I/O is
multiplexed in a single `select()` loop. Concurrency includes:

- One bounded player-load worker that owns a pooled connection and returns typed rows.
- A private player-journal append dispatcher and keyed revisioned-save workers.
- A non-coalescing critical-command coordinator, typed repository workers, and outbox
  dispatcher for economy, ownership, auction, and gameplay-outcome operations.
- One bounded maintenance worker for staggered recurring database and snapshot work.
- One immutable world-recovery publisher worker when Redis recovery is enabled.
- Legacy item, scalar, and large-payload queue modules for remaining compatibility
  producers; they are not the player snapshot or critical-operation authority.
- The main game loop blocking signals (including `SIGSEGV`, handled internally)
  around each iteration so workers cannot interrupt pulse processing.

Legacy hostname lookup may still use a short-lived child. It is unrelated to
persistence and never receives player or world snapshot work.

`main()` (`src/net/comm.c:205`) parses flags, then `game_loop()` (`src/net/comm.c:704`)
runs until shutdown. Exit codes are meaningful - `scripts/cycle_mud.sh`
interprets them to decide whether to restart (see [RUNBOOK.md](../operations/RUNBOOK.md)).

### Command-line options

| Option | Effect |
|--------|--------|
| `[port]` | Listen port; must be > 1024. Default 7777 (`DFLT_PORT`, `src/core/config.h:21`). |
| `-C` | Copyover boot - recover player sockets from `copyover.dat`. |
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
| 7777 | Plain telnet (production default) | `src/core/config.h:21` |
| 7778 | TLS telnet (`SSL_PORT`) | `src/core/config.h:22` |
| port+1 | TLS telnet when a non-default port is given | `src/net/comm.c` |
| 4050 by default | WebSocket and HTTP health listener (`DURIS_WEBSOCKET_PORT`) | `src/net/websocket.c`, `src/net/websocket.h` |

The explicit `ENVIRONMENT`, `DB_NAME`, and `DB_ALLOWED_TARGETS` settings select and
authorize the database. The port is a second safety boundary: production role requires
7777, and a non-default port redirects a production-like database name to `duris_dev`
before allow-list validation. A development port is not permission to use an arbitrary
target. See [CONFIGURATION.md](../operations/CONFIGURATION.md#persistence).

## Game loop and timing

`game_loop()` is a classic pulse-based loop:

- Each iteration blocks at most `OPT_USEC` microseconds (250 ms,
  `src/core/config.h:82`) - nominally 4 pulses/second.
- Per-pulse work is dispatched by pulse counters using `PULSE_*` constants
  (`src/core/config.h:84-91`): combat rounds every 16 pulses, mobile updates every
  30, ships/vehicles every 2, spellcasting every 9, etc.
- Socket readiness comes from `select()` over input/output/exception sets.
- A per-pulse time budget is enforced between event-wheel callbacks (see
  below). Because the check happens *between* callbacks, one slow job overruns
  the pulse regardless of policy - an expensive callback has to be made cheaper
  or sliced, not merely deprioritized.

Descriptor structures come from a custom pooled allocator (`mm_create("SOCKET",
...)`) rather than raw malloc.

## Event wheel

Deferred and periodic work runs through the event system (`src/world/new_events.c`,
`src/world/nevent_periodic.c`, and the callbacks in `src/world/events.c`): timed callbacks
with absolute deadlines are stored on a 300-bucket wheel and executed inside
the game loop between pulses. Budget telemetry is exposed via `NEVENT BUDGET`
log lines and `src/persistence/latency_trace.c`; `NEVENT SLOW` marks total scheduler work of
at least 50 ms.

[EVENTS.md](EVENTS.md) is the mechanism reference — absolute scheduling, the
three intrusive lists, typed payloads and handles, cancellation semantics,
periodic ownership, catch-up debt, and configuration. The rest of this section
records incident-derived constraints.

Each pulse is bounded by a wall-clock budget (`NEVENT_BUDGET_USEC_DEFAULT`,
25 ms) and a callback count cap (`NEVENT_MAX_CALLBACKS_DEFAULT`). Both are
overridable at runtime - see [CONFIGURATION.md](../operations/CONFIGURATION.md#diagnostics).
The time budget is meant to be the binding limit; a count cap low enough to end
pulses at half the time budget starves the wheel.

These properties of the wheel are load-bearing:

- **Deferral covers the whole unscanned suffix.** When a pulse runs out of
  budget, every remaining due event moves to the next pulse. Future-revolution
  records stay in place. All records retain their absolute `due_tick`, so an
  overload cannot silently add a 75-second revolution to a long timer.
- **Ordering is stable and starvation-resistant.** Records sort by absolute
  deadline, effective priority, and sequence. Player-timed work has priority by
  default, while ordinary work ages above it after two deferrals or two late
  ticks. Deferred count, estimated cost, and oldest deadline are repaid through
  bounded catch-up quotas.
- **Character-wide maintenance is sliced.** `generic_char_event` (`handler.c`)
  swept every character in one callback (17.8 ms average, 24.1 ms peak against a
  25 ms budget). It runs in four slices, one per invocation, rescheduled at a
  quarter of the old delay. The slice is a hash of the character's address, so
  it is stable for the character's lifetime: every character is still visited
  exactly once per 20 s, none skipped or done twice. Other heavy periodic jobs
  use one-tick continuations with stable cursors or runtime-ID snapshots.

### Command gate

`comm.c` will not dequeue a descriptor's input while `PLR2_WAIT` is set
(`CAN_ACT(ch)`). The bit is set by `CharWait()` and cleared by the `event_wait`
event it schedules - so any path where that event is not scheduled, or is
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

`boot_db()` (`src/world/db.c:406`) loads, in order: configuration/properties,
command tables, help command attributes, world files generated by the area
toolchain (`areas/world.*` - rooms, mobs, objects, zones, quests, shops,
triggers), then boots zones via `boot_zones()` (`src/world/db.c:1523`). On a fresh
checkout these combined files are produced by rebuilding the `make_*` helpers
in `areas/src/` and running `areas/m_slow` (see [BUILDING.md](../guides/BUILDING.md)).

Database compatibility is checked before `boot_db()` and before listeners. The main
connection establishes the required charset, UTC time zone, READ-COMMITTED isolation,
strict SQL mode, and bounded timeouts; remote targets require verified TLS. Boot then
verifies the immutable migration chain and complete required schema metadata. Only
after that read-only gate passes does one transaction publish a changed race/class
lookup dataset, followed by item UID reservation and pool initialization. See
[RUNTIME_COMPATIBILITY.md](../persistence/RUNTIME_COMPATIBILITY.md).

After world boot, two recovery paths may apply before socket input is accepted:

- **Copyover recovery** (`copyover_boot`, `src/persistence/copyover.c`): listening sockets
  and live player connections are re-inherited from `copyover.dat`; combat state
  is restored by `copyover_restore_combat()`.
- **Redis restart recovery**: after a graceful restart or an unclean exit, a world generation
  is restored only after schema, completeness, sequence, checksum, size, and age
  validation. The generation and bounded binary floor-item trees are combined into one
  semantic plan; every custody-bearing item UID/root/parent/VNUM/room is reconciled against
  SQL before rollback-capable materialization. Authenticated reconstructible world-pop
  objects carry an explicit non-custody marker, and player corpses remain owned by the
  separate authoritative corpse restore path. NPC inventory and equipment are deliberately omitted
  because they are not an authoritative identity-safe source. A fenced one-use sequence
  marker distinguishes a clean restart from a crash. The prior generation remains
  authoritative until publication ACK, and matching floor deltas are retained until that
  ACK. A failed restore performs a full normal zone boot.

Player-load initialization fails existing-character login closed. Before listeners,
the runtime also initializes revisioned player saves, critical commands/outbox, and
the maintenance scheduler. A failed typed pipeline fences its affected operation; it
does not silently convert the action to an unrelated raw SQL queue.

## Persistence

MySQL/MariaDB with InnoDB is the durable authority, but persistence is not one generic
queue. Each correctness domain has its own ordering, idempotency, and failure boundary.
The shared bounded connection pool (`src/sql/sql_pool.c`) establishes the same connection
contract as the main connection. When the pool is unavailable, typed load, snapshot,
critical-command, and maintenance routes report unavailable/retryable outcomes or fence
the affected action; only explicitly retained legacy compatibility producers can use
their historical synchronous fallback.

Existing-character login uses `src/player/player_load_pipeline.c` and
`src/player/player_load_repository.c`. A worker opens one consistent read transaction, fetches
required player, skill, affect, item-owner, item metadata, and pet graph rows in bounded
sets, and returns owned typed data. The game thread validates request identity, revision,
limits, graph integrity, and materializes in linear time. Any required-component or
stale result fails login cleanly; a partial character is never published.

Player checkpoints are captured into immutable, revisioned DTOs on the game thread.
A bounded append dispatcher durably frames them in the typed journal, then keyed
workers apply them transactionally with per-PID ordering and exact revision ACKs.
Ordinary mutation and checkpoint routes perform no MySQL, Redis, or filesystem I/O on
the simulation thread. Terminal transitions drain to a bounded deadline and require the
matching durable outcome before live state may be destroyed; a failed terminal save
retains the character and inventory for retry. The journal handoff is durable recovery
evidence, not a claim that the database already committed.

Redis complements MySQL with floor-delta tracking and immutable world-recovery
generations (`src/world/world_recovery_pipeline.c`, `src/redis/redis.c`). World graph capture is
incremental and bounded on the game thread; the publisher receives owned bytes only.
It writes a sequence-keyed payload before atomically advancing the current pointer and
metadata. Restore validates framing and semantics, reconciles all item custody in one
boot-only SQL transaction, creates entities with rollback tracking, atomically hydrates
runtime custody, and applies doors/zones last. Floor and world item trees share the same
12-node bounded binary representation; gameplay performs no recovery network or SQL I/O.

Non-idempotent gameplay effects use a separate critical-command coordinator
(`src/persistence/critical_command_coordinator.c`). Its immutable, non-coalescing commands carry a
stable 128-bit operation ID and sorted entity-key set. Conflicting key sets execute in
acceptance order, unrelated sets may run concurrently, and exact typed completion is
required to release a gameplay fence. A checksummed local journal preserves accepted
commands through retry and restart. A typed prepared-statement repository applies each
operation through one InnoDB inbox/state/outbox transaction, resolves duplicate or
ambiguous commits by stable operation ID, and classifies retryable database errors. A
bounded at-least-once dispatcher retains typed outbox rows through delivery, retry,
dead-letter, restart, and operator reconciliation. Epic, account/wallet, item movement,
locker, auction, combat, artifact/guild, boon/reward, and zone-touch domains use typed
repositories rather than unrestricted durable raw SQL messages.

Recurring database work uses `src/persistence/maintenance_scheduler.c`. Stable per-instance offsets
replace aligned modulus spikes; every job has row and time budgets, continuation state,
bounded retry, and game-thread completion. The lifecycle archive slot remains disabled
because the manifest's controller decisions are still pending.

Details and schema management: [DATABASE.md](DATABASE.md).

## Networking

- **Telnet** (`src/net/comm.c`): line-based, with MCCP compression support
  (`src/net/mccp.c`).
- **TLS telnet** (`src/net/ssl.c`): same protocol over TLS; certificate/key expected as
  `duris.crt` / `duris.key` in the repository root (symlinks recommended).
- **WebSocket** (`src/net/websocket.c`): RFC 6455 server on `DURIS_WEBSOCKET_PORT`
  (default 4050) with HTTP upgrade for browser clients and a value-free
  `GET /health` readiness response. Production binds the WebSocket listener to
  loopback behind a TLS reverse proxy and applies an exact browser-origin
  allow-list. Game messages use JSON (`src/core/json_utils.c`); the privileged DurisWeb
  peer uses the one-time challenge contract in
  [api/durisweb.md](api/durisweb.md).
- **GMCP** (`src/net/gmcp.c`): outbound `Room.Info`, `Room.Map`,
  `Char.Vitals`, `Char.Status`, `Char.Affects`, `Combat.Update`,
  `Comm.Channel`, `Quest.Status`, `Quest.Map`, `Group.Status`,
  `Ship.Contacts`, and `Ship.Info` packages to capable clients over telnet or
  WebSocket. `Char.Skills` and `Char.Items` names are reserved but are not
  emitted.
- Hostname resolution and login/nanny flow are in `src/account/nanny.c`;
  interpreter and command dispatch are in `src/cmd/interp.c`.

Terminal height is a saved player preference, not a negotiated connection
property. New characters and missing database values use 40 lines; the
`toggle screensize` command accepts 12 through 48 (`0`, `default`, or `off`
restores the default), and the pager reserves four lines for prompts and
controls. The server defines
the telnet NAWS option name but does not negotiate or consume NAWS dimensions,
so clients must set the preference manually. Existing characters keep their
saved value. The historical SQL column default of 24 is not the runtime
default; changing saved preferences would require an explicit data migration,
not an edit to sealed migration history.

## Studio procs (triggers)

Builder-authored behaviors (mob speech/give/death hooks, boot-time triggers)
are data-driven from `areas/world.trg` and dispatched by the studio-proc engine
(`src/mob/studioproc.c`, `src/mob/studioproclib.c`). Engine hooks are four one-line
call sites added to existing code paths; everything else is table-driven.
Design rationale: [STUDIOPROC.md](../content/STUDIOPROC.md). Builder grammar:
[`src/howto_trg.txt`](../legacy/src/howto_trg.txt).

## Ships

The ship simulation (sailing, cargo, naval combat, NPC crews/shops) is a
self-contained subsystem under `src/ships/`, built into its own object files
and linked into the main binary. Ship SQL and optional Redis snapshot routes remain
distinct from the revisioned player-save and critical-command authorities.

## Help system

In-game `help` is database-backed: `wiki_help()` queries the `pages` table and
renders wiki-formatted text (`src/cmd/wikihelp.c`), augmented by command attributes
loaded from `docs/lib/information/command_attributes.txt`. Pipeline details:
[HELP_SYSTEM.md](../content/HELP_SYSTEM.md).

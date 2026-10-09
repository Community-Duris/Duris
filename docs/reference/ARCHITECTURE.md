# Architecture

DurisMUD is an event-driven MUD server derived from the DikuMUD lineage,
compiled as C++20 (`g++ -std=c++20`) from C-style sources. One world thread
owns live characters, objects, commands, combat, and events. Bounded workers
handle typed persistence and external services. Clients connect over plain
telnet, TLS telnet, or WebSocket. The selected persistence backend owns durable
player and domain state; Redis optionally supplies caches and world recovery.

This guide incorporates the documentation audit into `experimental-accounting`
at implementation base `43807ab01fce32d10e6976739772d454618233c1`, reviewed on
October 9, 2026. The dated architecture assessment preserves its earlier source
pins; the [branch snapshots](#newer-branch-implementations) below retain that
historical review boundary. Documentation publication does not qualify the
combined implementation, enable accounting, or establish deployment.

Related reading: [CODEBASE.md](CODEBASE.md) for the module map,
[DATABASE.md](DATABASE.md) for persistence details, [RUNBOOK.md](../operations/RUNBOOK.md)
for operations. The supplementary
[visual overview](../diagrams/duris-server-architecture.html) follows the same
source boundary and distinguishes backend selection from live publication. The dated
[architecture review](../design/ARCHITECTURE_REVIEW_2026-10-07.md) records source
pins, design concerns, qualification limits, and proposed decisions.

## Process model

By default, one server process (`bin/server/dms`, staged as `bin/server/dms_new`)
owns client transports and the world. The game thread multiplexes sockets with
`poll()` in bounded network turns between nominal 250 ms simulation boundaries.
Command dispatch and world publication retain their named phases. There is no
fork-per-connection, player-save fork, or world-save fork.

With `--persistent-transport`, a persistent parent owns listeners, client sockets,
TLS, Telnet negotiation/MCCP and WebSocket framing/compression. Its single world
child owns authentication, characters, commands and persistence workers. Private
authenticated IPC carries logical sessions; durable copyover replaces the world
while eligible authenticated players retain their sockets. The parent does not
start hostname workers. See [Persistent transport](../network/PERSISTENT_TRANSPORT.md)
for eligibility, ordering, bounds and failure/recovery limits.

Workers receive owned requests or captured bytes and return typed results.
Game-thread consumers validate identity, revision, and current eligibility
before publishing results into live state. A worker queue does not transfer
ownership of a character or descriptor. The main boundaries are:

| Subsystem | Background responsibility | Publication or lifecycle owner |
| --- | --- | --- |
| Account-name reads | Bounded login-worker jobs return owned account rows. | Session continuations validate identity and resume authentication on the world thread; see [asynchronous account loading](../persistence/ASYNC_ACCOUNT_LOAD.md). |
| Player load | One bounded worker loads a consistent SQL snapshot or uses the selected flat-file repository. | Account/nanny continuations validate and materialize on the game thread. Synchronous fallback paths still exist. |
| Player checkpoints | Journal append dispatcher, then keyed revisioned-save workers. | Game-thread capture and exact revision completion; terminal transitions request and wait for their required durability evidence. |
| Critical commands | Serialized journal admission worker, keyed execution workers, and SQL outbox dispatcher. | Coordinator and domain handlers preserve operation identities, fences, receipts, and live publication. Flat-file builds select their authority adapter. |
| Locker and quest acknowledgements | Separate locker persistence and quest reward obligation workers. | Their domain pulse and recovery handlers retain completion state independently of ordinary checkpoints. |
| Content and Collector reads | Help, information, and Collector catalog refresh workers; bounded Collector listing worker. | Complete catalogs or typed listings publish on the game thread. Backend-specific local catalogs have their own load rules. |
| Passwords and recovery mail | Password hashing/verification worker and optional libcurl SMTP mail worker. | Descriptor authentication continuation or account-recovery pulse; mail is best effort and disabled unless configured. |
| Recurring persistence | Bounded maintenance worker with row/time budgets, cursors, and retries. | Scheduler submits work and consumes results on the game thread before due activities. |
| Optional Redis | World-recovery publisher plus separate floor, presence, report-cache, donation, and retired-ship cleanup workers. | Each service owns its connection, queues, admission, and teardown; Redis is not player-save durability. |
| Optional telemetry | Private sequential SQL writer consumes bounded value records. | Gameplay captures observations; the process lifecycle requests stop and reaps the writer before SQL teardown. |
| Persistence diagnostics | Bounded log worker writes queued diagnostic records. | The pulse consumes delivery status; lifecycle code drains diagnostics separately from durable gameplay. |

Legacy item, scalar, and large-payload queue modules remain compatibility code;
they do not own revisioned player checkpoints or critical operations. Historical
raw fallback records are not automatically replayed at normal boot. Their
inspection/quarantine uses the explicit operator tooling.

A nonblocking, close-on-exec pipe wakes network waits after worker queue
publication. Hints may coalesce; authoritative results remain in their queues.
Draining the pipe does not consume results, publish gameplay, or advance ticks.

Legacy hostname lookup may still use a short-lived child. It is unrelated to
persistence and never receives player or world snapshot work.

`main()`, `run_the_game()`, and `game_loop()` in
[`src/net/comm.c`](../../src/net/comm.c) own configuration, boot/teardown, and
pulse orchestration respectively. Shutdown signals stage lifecycle requests for
the connection phase. Interrupted waits do not advance simulation ticks. Exit codes are interpreted by
`scripts/cycle_mud.sh`; see [RUNBOOK.md](../operations/RUNBOOK.md).

### Command-line options

| Option | Effect |
|--------|--------|
| `[port]` | Plain-telnet port; must be > 1024. Default 7777 (`DFLT_PORT` in `src/core/config.h`). |
| `-C` | Copyover boot; recover eligible sockets/world state from `COPYOVER_STATE_FILE`, default `copyover.dat`. |
| `--persistent-transport` | Keep client transports in a parent while replacing the single world child. |
| `-m`, `--minimal` | Mini mode (reduced area set); disables random encounters and ferries. |
| `-z` | Mini mode with the area debugger on. |
| `-f` | Disable ferries. |
| `-l` | Disable random encounters. |
| `-s` | Suppress special-procedure assignment. |
| `-p` | Allow password change without the old password. |
| `-d <dir>` | Data directory (default `.`). |
| `--migrate-all` | Legacy player-file migration, then exit; separate from immutable schema migrations. |
| `--material-rarity-report[=dir]` | Generate material rarity report and exit. |

These are executable options. Launcher modes such as `--dev` and `--production`
belong to `scripts/start_mud.sh` / `scripts/cycle_mud.sh`; see the
[configuration reference](../operations/CONFIGURATION.md#persistence).

## Ports

| Port | Purpose | Defined in |
|------|---------|------------|
| 7777 by default | Plain telnet; positional executable port overrides it | `DFLT_PORT`, `src/core/config.h` |
| 7778, or plain port+1 with a positional port | TLS telnet; `DURIS_TLS_PORT` overrides either default | `SSL_PORT`, `src/core/config.h`; `main()`, `src/net/comm.c` |
| 4050 by default | WebSocket and HTTP health listener (`DURIS_WEBSOCKET_PORT`) | `src/net/websocket.c`, `src/net/websocket.h` |

`DURIS_PRODUCTION_PORT` defines the production-role plain-telnet guard, default
7777. It does not itself change the executable's listen port. `DURIS_TLS_PORT`
must be a valid TCP port distinct from the plain listener.

For SQL authority, `ENVIRONMENT`, `DB_NAME`, and `DB_ALLOWED_TARGETS` select the
trust role, requested database, and authorized resolved target. Production role
requires the configured production port. On another port, the production-like
names `duris` and `duris_prod` resolve to `duris_dev` before allow-list validation;
an explicitly named disposable database is not rewritten merely because of its
port. Flat-file authority uses its private state root rather than a database.
See [CONFIGURATION.md](../operations/CONFIGURATION.md#persistence).

## Game loop and timing

`OPT_USEC` in `src/core/config.h` sets a nominal 250 ms pulse, or four pulses
per second. `poll()` services bounded network turns between simulation boundaries;
the persistent parent owns those turns in persistent mode. Input staging and
retained wire output can progress between pulses, while gameplay handlers,
authentication completion, prompts and durable publication retain their phases.
Worker hints and signal interruptions do not advance ticks. A synchronous callback
can overrun a pulse; missed wall-clock slots are discarded and the next pulse
waits a full interval after the overrun. The 250 ms interval is a target, not a
bound on world work or a promise of command latency.

The named helpers execute on the same world thread in this order:

| Order | Phase | Work and publication boundary |
| --- | --- | --- |
| 1 | Connection | Lifecycle requests, persistence-log status, hostname answers, readiness, accepts, TLS negotiation, reads, and socket errors. |
| 2 | Session input | Login/liveness checks, password completions, wait/casting/transaction gates, then pager, editor, playing, or nanny dispatch. |
| 3 | Output | Retained transport bytes, new application output, prompts, WebSocket/control output, and close transitions. |
| 4 | Events | `ne_events()`, telemetry pulse accounting, creation-grant preparation, artifact mana, and device actions. |
| 5 | Recurring persistence | Every-two-pulse domain completions, outboxes, checkpoints, player loads, caches, and world recovery; maintenance results are serviced each pulse. |
| 6 | Activities, then combat | Due activities, `perform_violence()`, and descriptor-related map/group/movement updates. Combat has its own attack eligibility and cadence rules. |
| 7 | Pulse reset | Logical tick advance, due affect/point updates, diagnostics, and network turns until the next simulation deadline. |

Critical completions follow output and events on the nominal every-two-pulse
cadence, about 500 ms. A result can update live state while its text waits for a
later output phase or transport write. Queue admission, database completion,
live publication, and client delivery are separate timings. Moving work between
phases changes observable behavior. The detailed contract and source checks are
in [GAME_LOOP_PHASES.md](../network/GAME_LOOP_PHASES.md).

Descriptor structures come from a custom pooled allocator (`mm_create("SOCKET",
...)`) rather than raw malloc.

## Event wheel

Deferred and periodic work runs through the event system (`src/world/new_events.c`,
`src/world/nevent_periodic.c`, and the callbacks in `src/world/events.c`): timed callbacks
with absolute deadlines are stored on a 300-bucket wheel and executed in the
world pulse's event phase. Budget telemetry is exposed via `NEVENT BUDGET`
log lines and `src/persistence/latency_trace.c`; `NEVENT SLOW` marks total scheduler work of
at least 50 ms.

[EVENTS.md](EVENTS.md) is the mechanism reference — absolute scheduling, the
three intrusive lists, typed payloads and handles, cancellation semantics,
periodic ownership, catch-up debt, and configuration. The rest of this section
records incident-derived constraints.

Each event phase checks a wall-clock budget (`NEVENT_BUDGET_USEC_DEFAULT`,
25 ms) and a callback count cap (`NEVENT_MAX_CALLBACKS_DEFAULT`, 4,000). Both are
configurable - see [CONFIGURATION.md](../operations/CONFIGURATION.md#diagnostics).
The checks occur between callbacks and cannot preempt a running callback.
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
- **Character maintenance bodies are sliced.** `generic_char_event` in
  `src/world/handler.c` still walks the character list each invocation, but a
  stable address hash selects one of four groups for the maintenance body.
  The fixed-delay periodic job runs at nominal five-second intervals, so each
  group receives one turn per nominal 20-second cycle. Scheduler deferral or
  a slow world can extend elapsed time. Other heavy jobs use continuations
  with stable cursors or runtime-ID snapshots. The newer per-character timer
  design is separate branch work.

### Command gate

`CharWait()` controls `PLR2_WAIT` (`CAN_ACT(ch)`) and schedules `event_wait`
to clear it. It handles scheduling refusal, clamps negative delay, and records
an absolute `wait_until_pulse` deadline with two seconds of grace. The command
sweep clears a stuck wait when its event is absent or that deadline has passed.
This bounds the wait flag when the loop runs; it cannot prevent whole-loop stalls.
The deadline is runtime-only.

Casting and active item use select the restricted queue independently of
`PLR2_WAIT`. `casting_input_for_descriptor()` applies this rule to a playing
descriptor outside pager/editor input. The queue and interpreter share
`cmd_allowed_while_casting()` in `src/cmd/interp.c`: `petition` and `return`
are allowed; `abort` is allowed for active item use or when `PLR3_ABORT_CASTING`
permits it. Other type-ahead stays queued until it becomes eligible. The old
claim that clearing action-wait bypasses this selector no longer describes
the source.

Normal playing input also consults pending item/currency transaction gates.
Commands independent of unpublished state may pass dependent type-ahead;
dependent commands retain their order. Creation-grant admission can block
playing commands while nanny input continues through the pre-entry flow.

### Movement lifetime guard

`src/cmd/actmove.c::do_move()` snapshots `character_removal_generation` before
`do_simple_move()`. `extract_char()` and `free_char()` increment this unsigned
64-bit counter before nested work or teardown. If no removal occurred during
that synchronous call, the post-move global membership scan can be skipped.
Otherwise the original `char_in_list()` check runs before `IS_ALIVE()` can
read the mover. Removing an unrelated character must trigger the fallback scan,
not suppress valid post-move work.

The counter belongs to the single game-state thread. New movement-reachable
unlink/free paths must invalidate it before releasing storage. This preserves
pointer-membership semantics, including their existing address-reuse limitation;
it is not an object-identity registry. The separate room-procedure guard in
`char_to_room()` remains. `test_movement_liveness_runtime.py` exercises the
production movement tail with synthetic lifecycle outcomes under ASan/UBSan;
`test_kingdom_contract.py` pins the production invalidation hooks.

## Boot sequence

`main()` parses executable options, selects the data directory, reads the
protected environment file, validates TLS and persistence selection, and starts
persistence logging. SQL builds open validated connections and establish runtime
exclusion/economic lifecycle ownership, schema/season readiness, lookup data,
item UID reservation, and the pool before world boot. The SQL session contract
includes charset, UTC, READ-COMMITTED isolation, strict mode, bounded timeouts,
and verified TLS for remote targets. See
[RUNTIME_COMPATIBILITY.md](../persistence/RUNTIME_COMPATIBILITY.md).

Flat-file builds validate/provision the private state root, reserve item UIDs,
and hydrate the system item-owner revision without opening MySQL. Backend
selection must match the binary; it is not per-operation failover.

`run_the_game()` initializes signal handling and world boot.
`boot_db()` in `src/world/db.c` loads command tables, help attributes, generated
world data (`areas/world.*` - rooms, mobs, objects, zones, quests, shops,
triggers), and zones. On a fresh checkout, rebuild the `make_*` helpers in
`areas/src/` and run `areas/m_slow` as described in
[BUILDING.md](../guides/BUILDING.md).

After fatal world-data loading completes, the runtime initializes player loads,
quest acknowledgement, mail, content caches, revisioned player saves, critical
commands/outbox, Collector reads, and recurring maintenance as applicable.
Listener creation and copyover descriptor recovery belong to `game_loop()`.
Unavailable typed components stop their affected actions; player-load admission
has the explicit synchronous exceptions described below.

After world boot, two recovery paths may apply before socket input is accepted:

- **Copyover recovery** (`src/persistence/copyover.c`): world/combat state is
  captured through `COPYOVER_STATE_FILE` using portable version 18. Native
  versions 12-17 remain readable only on the compatible legacy ABI. Default
  mode inherits eligible plain-telnet sockets; TLS/WebSocket and non-playing
  descriptors still refuse that handoff. Persistent mode records logical
  sessions and verifies the live parent's committed identities and file digest.
  Persistence and death-recovery prerequisites still apply. See the
  [format contract](../persistence/COPYOVER_FORMAT.md),
  [persistent transport contract](../network/PERSISTENT_TRANSPORT.md), and
  [copyover qualification](../testing/COPYOVER_RUNTIME.md).
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
  ACK. A failed restore performs a full normal zone boot. Custody reconciliation
  in this checkout is SQL-backed: the client-free implementation of
  `sql_persistence_reconcile_world_recovery_items()` accepts only an empty
  authority set. Enabling Redis does not establish equivalent custody-bearing
  world restoration in flat-file mode.

## Persistence

### Backend and authority selection

[`src/persistence/persistence_mode.c`](../../src/persistence/persistence_mode.c)
selects one whole-server persistence mode at boot:

| Mode | Build and authority |
| --- | --- |
| `mariadb-primary` (default) | MariaDB client build, `PERSISTENCE_BACKEND=mariadb`; MySQL/MariaDB with InnoDB owns native durable state, receipts, and ledgers. |
| `flatfile-primary` | Client-free build, `PERSISTENCE_BACKEND=flatfile` / `__NO_MYSQL__`; selected repositories use private `FLATFILE_STATE_DIR` snapshots and checksummed authority transactions. |
| `mariadb-primary-flatfile-fallback` | Recognized historical token, rejected at configuration because mixed per-operation authority transfer is unsupported. |

The backend must match the executable. Selecting flat-file mode does not provide
SQL-only features such as telemetry, and an SQL error does not switch authority
to a file. Detailed configuration is in
[CONFIGURATION.md](../operations/CONFIGURATION.md#persistence).

Live world state and native durable state have different owners. For supported
accounting routes, native effects, economic evidence, and the receipt belong to
the same SQL or flat-file authority transaction. The read-only
`economic_gameplay_authority` projection carries verified lifecycle/admission
mappings; it is not a second balance store or proof of RAM authority. Writer
coverage and release qualification remain explicit in
[ECONOMY_ACCOUNTING.md](../persistence/ECONOMY_ACCOUNTING.md).

### Player loading and checkpoints

The shared bounded SQL pool (`src/sql/sql_pool.c`) establishes the same verified
connection contract as the main connection. A missing pool or unavailable
worker yields a typed outcome; its caller decides whether to refuse, retry,
retain a fence, or use an explicitly implemented synchronous path.

Existing-character login uses `src/player/player_load_pipeline.c` and
`src/player/player_load_repository.c`. A worker opens a consistent read transaction,
fetches player, skill, affect, item-owner, item metadata, and pet graph rows in
bounded sets, and returns owned typed data. The game thread validates request
identity and core status, checks the normal load's revision, limits, and graph
integrity, and materializes in linear time. Flat-file builds select their typed
load repository. Selected secondary failures can admit an explicitly degraded
character with affected state omitted or quarantined and ordinary saves fenced
to protect durable data. Invalid identity or core snapshot data is refused;
unresolved death-recovery conflicts also block normal admission. Some failures
are retried before the login flow decides. See
[consistent player load](DATABASE.md#consistent-player-load) for the admission
policy and narrow item-payload-gap death-disposition exception.

Interactive account character selection and legacy login still call
`player_load_pipeline_execute_sync()` if submission is not accepted, and certain
retry outcomes also reach synchronous loading. Other callers can wait for a load
before falling back. The SQL synchronous path acquires a pool connection and
executes repository reads on the calling thread. Account-name reads also retain
synchronous paths in this checkout; the newer account-read worker is separate
branch work. The guide therefore cannot promise that login never waits on SQL.
The [architecture review](../design/ARCHITECTURE_REVIEW_2026-10-07.md) recommends
a separate interactive admission change and its required validation.

Player checkpoints are captured into immutable, revisioned DTOs on the game thread.
A bounded append dispatcher durably frames them in the typed journal, then keyed
workers apply them transactionally with per-PID ordering and exact revision ACKs.
The ordinary checkpoint capture/completion path performs no MySQL, Redis, or
filesystem I/O on the simulation thread. This property does not cover every
login, legacy writer, staff command, or terminal transition. Terminal saves wait
to a bounded deadline for the requested durability evidence before live state
may be destroyed; failure retains the character and inventory for retry. A
synced journal record is recovery evidence, distinct from an applied database
revision. See [PLAYER_SAVE_PIPELINE.md](../persistence/PLAYER_SAVE_PIPELINE.md)
and [PLAYER_SAVE_JOURNAL.md](../persistence/PLAYER_SAVE_JOURNAL.md).

Redis complements MySQL with floor-delta tracking and immutable world-recovery
generations (`src/world/world_recovery_pipeline.c`, `src/redis/redis.c`). World graph capture is
incremental and bounded on the game thread; the publisher receives owned bytes only.
It writes a sequence-keyed payload before atomically advancing the current pointer and
metadata. Restore validates framing and semantics, reconciles all item custody in one
boot-only SQL transaction, creates entities with rollback tracking, atomically hydrates
runtime custody, and applies doors/zones last. Floor and world item trees share the same
12-node bounded binary representation; gameplay performs no recovery network or SQL I/O.

### Critical operations and publication

Non-idempotent gameplay effects use a separate critical-command coordinator
(`src/persistence/critical_command_coordinator.c`). Its immutable, non-coalescing commands carry a
stable 128-bit operation ID and sorted entity-key set. Conflicting key sets execute in
acceptance order, unrelated sets may run concurrently, and exact typed completion is
required for its terminal transition. Domain publication gates can outlive
coordinator execution. A checksummed local journal preserves commands through
retry and restart. Admission and publication follow distinct steps:

1. Submission reserves bounded memory and entity keys and returns
   `awaiting_durability`; it has not established durable success.
2. The admission worker appends and syncs the immutable command. Only a successful
   durability acknowledgement makes the command eligible for execution.
3. The selected typed transaction applies native state and retains its exact
   receipt. SQL routes use inbox/state/outbox transactions; flat-file routes use
   their authority adapter and recoverable after-images.
4. The world pulse consumes a matching completion. Domain handlers validate and
   publish live effects, retaining unresolved receipts and recovery work. Outbox
   delivery has its own at-least-once and deduplication boundary.

A typed prepared-statement SQL repository resolves duplicate or
ambiguous commits by stable operation ID, and classifies retryable database errors. A
bounded SQL outbox dispatcher retains typed rows through delivery, retry,
dead-letter, restart, and operator reconciliation. Epic, account/wallet, item movement,
locker, auction, combat, artifact/guild, boon/reward, and zone-touch domains use typed
repositories rather than unrestricted durable raw SQL messages. Admission still
depends on implemented route coverage and lifecycle readiness.

An ambiguous receipt keeps the original operation identity and applicable
fences. A stale completion, delayed outbox, or failed live publication cannot
justify a replacement operation or a duplicate reward. Journal durability,
native commit, live publication, receipt acknowledgement, and client delivery
must be diagnosed separately. See
[CRITICAL_COMMAND_PIPELINE.md](../persistence/CRITICAL_COMMAND_PIPELINE.md) and
[ITEM_COMMAND_PIPELINE.md](../persistence/ITEM_COMMAND_PIPELINE.md).

### Recurring persistence

Recurring database work uses `src/persistence/maintenance_scheduler.c`. Stable per-instance offsets
replace aligned modulus spikes; every job has row and time budgets, continuation state,
bounded retry, and game-thread completion. The lifecycle archive slot remains disabled
because the manifest's controller decisions are still pending.

Details and schema management: [DATABASE.md](DATABASE.md).

## Telemetry

Telemetry is opt-in (`TELEMETRY_ENABLED`) and independent of gameplay mutation
authority. Existing session/activity, progression, encounter, and combat hooks
capture bounded value records. A private worker writes SQL; external rollups and
read-only reports consume the observations and supported committed reward
projections. Capture does not query SQL, append a spool, or join a worker on the
world thread. Client-free flat-file builds report `flatfile_disabled` rather than
selecting another sink.

The lifecycle has a deadline-bounded stop request followed by mandatory final
reap. A timed-out request retains the worker and bindings; final reap may wait on
an in-flight repository callback. It runs after the game loop returns and before
global SQL teardown. See the
[telemetry configuration and shutdown contract](../operations/CONFIGURATION.md#telemetry-shutdown-request-and-final-reap),
[transport](../telemetry/TRANSPORT.md), and
[performance/teardown gate](../telemetry/PERFORMANCE_GATE.md). The larger balance
expansion is tracked separately below.

## Shutdown and copyover boundaries

Normal shutdown acquires the lifecycle guard, quiesces critical admission and
outbox work, drains them, requests terminal player saves, drains checkpoint and
world-recovery work, and saves dirty shopkeeper inventory before extraction and
socket teardown. A failed required gate cancels shutdown and resumes the live
loop. The player-wipe path has deliberately different handling; it is not the
ordinary shutdown contract.

Copyover has its own pre-exec gates: transport eligibility, pending starter/death
work, ship/locker/maintenance drains, critical commands and outboxes, terminal
player saves, a final locker drain, and world recovery. A failure keeps the
current process live. Successful exec replaces the process and follows the
copyover recovery path; it does not run ordinary post-loop teardown.

After ordinary loop exit, `run_the_game()` shuts down domain workers, Redis
services, caches, mail/password work, and save/coordinator dependencies before
freeing the world. Mail drops queued unsent work and joins its bounded in-flight
send; it is not a durable gameplay drain. `main()` then stops/reaps telemetry,
closes remaining SQL resources, and releases lifecycle ownership. Individual
drain deadlines do not establish a hard bound on total process teardown. Source
owners are `src/net/comm.c`, `src/persistence/copyover.c`, and their subsystem
shutdown functions; operator procedures remain in
[RUNBOOK.md](../operations/RUNBOOK.md).

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
(`src/mob/studioproc.c`, `src/mob/studioproclib.c`). Existing gameplay/lifecycle
hooks dispatch the builder-authored trigger definitions.
Design rationale: [STUDIOPROC.md](../content/STUDIOPROC.md). Builder grammar:
[`src/howto_trg.txt`](../legacy/src/howto_trg.txt).

## Ships

The ship simulation (sailing, cargo, naval combat, NPC crews/shops) is a
self-contained subsystem under `src/ships/`, built into its own object files
and linked into the main binary. Ship SQL and optional Redis snapshot routes remain
distinct from the revisioned player-save and critical-command authorities.

## Help system

In-game `help` searches an in-memory catalog and renders wiki-formatted text
(`src/cmd/wikihelp.c`), augmented by command attributes loaded from
`docs/lib/information/command_attributes.txt`. MySQL builds load `pages` in a
background worker and publish complete catalogs on the game loop; flat-file
builds load their local catalog once per process. Help browsing adds no
help-specific cooldown. Pipeline details: [HELP_SYSTEM.md](../content/HELP_SYSTEM.md).
Refresh commands, limits, and failure handling:
[Help catalog operation](../operations/help-cache.md).

## Output transport lifetime

In default mode, `src/net/comm.c` retains actual transport bytes, including compressed output,
in a bounded descriptor-owned queue. Partial plain/TLS sends and temporary
backpressure leave unsent bytes queued for a later pulse; interrupted TLS records
must resume before new data. Fatal writes or queue-limit violations close the
descriptor, whose teardown releases the buffer. Newline/CP437 conversion uses
bounded dynamic storage because combining individually bounded messages can
exceed a fixed stack buffer. Persistent mode moves physical transport queues
and framing to the parent; ordered world output and acknowledgements cross
the authenticated IPC boundary described in the persistent transport contract.

`CON_FLUSH` closes only after application, transport, and WebSocket/control
queues drain in the output loop. Logout must deliver its goodbye and server EOF
without another client command. `test_telnet_output_runtime.py` covers partial
writes, retries, bounds and compression fidelity; the full-world journey checks
account logout before and after process restart.

## Newer branch implementations

The following are historical snapshots from the October 7 investigation.
Network turns, persistent transport, portable copyover v18, account-name workers,
runtime-ID indexing and per-character maintenance now exist in the publication
base described above. The table preserves the status at each reviewed pin;
it does not imply that all work remains unmerged. Exact commits,
source links, independent reviews, and qualification limits are preserved in the
[dated architecture review](../design/ARCHITECTURE_REVIEW_2026-10-07.md).
Current combined accounting integration and its remaining gates are recorded in
the [October 9 candidate handoff](../persistence/economy_accounting/COMBINED_ACCOUNTING_CANDIDATE_HANDOFF_2026-10-09.md).

| Stream and reviewed pin | Additional architecture | Status boundary |
| --- | --- | --- |
| Accounting, `71e421d12def1171f5538a30f14bee7c18452974` | `poll()` network turns between pulses; completion wakeup hints; opt-in persistent transport parent with one world child; portable copyover v18; asynchronous account-name reads; runtime-ID index; per-character maintenance timers; external world-progress watchdog. | Implemented in the reviewed newer source. Network turns do not advance simulation or move authoritative publication between phases. Character hydration still has synchronous fallback paths. Planned transport replacement does not promise continuation after arbitrary crashes. |
| Domain separation, `c84786260d3eb7e2ea041f461bbb5450c91542ec` | Owned-input preparation connected to existing callers, including Collector images, crafting quotes, and currency deltas. | Separately delivered/reviewed work; current transaction, publication, and recovery owners remain. Primary adoption and combined-candidate qualification require their own evidence. |
| Telemetry expansion, `7b3e8c29b93e1b531a7edbbbca24ff168d855569` | Battle/progression context, control observations, identity history, and canonical reward publication extend the existing capture/writer/report boundaries. | Implementation is underway. Delivered coverage and remaining work are distinct; this is not automatic balance policy or new gameplay authority. |
| Discovered-zone dailies, `8e4b6d9222259f187c62b64cf794df80ceed910d` | Discovery/private journals and daily eligibility consume native quest evidence and verified accounting readiness. | Separate gated implementation. Daily eligibility remains disabled by default; a journal read or content dossier cannot grant a reward or prove source renewal/recovery. |

Branch availability does not establish integration or deployment. Promote a
change into the main description only after checking its actual combined source,
migration manifests, feature gates, and qualification evidence. The newer
maintenance benchmark documents higher total scheduling work as well as lower
maintenance-body work; it does not prove a production CPU or p99 improvement.

The proposed future RAM authority model would move exact outcome preparation and
durable ordering under a coherent world-owned domain, then project to SQL. It
requires separate decisions about all writers, coupled effects, durability before
acknowledgement, bounded outage backlogs, recovery, and rollback. The current
preparation extractions do not implement that conversion. Those decisions remain
in the dated review rather than changing this guide's description of current
authority.

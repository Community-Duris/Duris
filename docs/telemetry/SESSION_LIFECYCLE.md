# Gameplay session and activity hook inventory

This inventory is the traced #265 server-boundary map. Gameplay hooks pass only
stable pointers and typed enums to `telemetry_runtime_*`; they do not construct
session/classifier state, write SQL, or include command text, player names, or
other PII in telemetry records.

## Lifecycle paths

| Lifecycle fact | Actual source path | Adapter call | Ordering / notes |
|---|---|---|---|
| Runtime bootstrap | `src/net/comm.c:main` | `telemetry_runtime_options_from_environment()` then `telemetry_runtime_init()` | Configuration is resolved once at the server boundary; disabled/unsupported telemetry does not gate login. |
| Normal account, new character, and WebSocket entry | `src/account/nanny.c:enter_game` | `telemetry_runtime_game_enter`, then `telemetry_runtime_game_context` when `STATE(d) == CON_PLAYING` | `enter_game` is shared by the account and WebSocket flows. The call is after login/world initialization and before the function returns. |
| Legacy menu entry | `src/account/nanny.c:select_main_menu` | `telemetry_runtime_game_enter`, then `telemetry_runtime_game_context` | The legacy caller changes the descriptor to `CON_PLAYING` after `enter_game`; this is the only entry call outside the shared tail. |
| Link-loss detach | `src/net/comm.c:close_socket` | `telemetry_runtime_game_connection_transition(...detached)`, then `telemetry_runtime_game_evidence(...linkdead)` | Uses `descriptor.original` for switched immortals. The descriptor transition is recorded before existing disconnect/session-audit work. |
| Resident reconnect | `src/account/nanny.c:reconnect`, `src/account/account.c:is_char_in_game`, and the direct `src/net/ws_handlers.c` online-character attach | `telemetry_runtime_game_connection_transition(...attached)`, then `telemetry_runtime_game_context` | Runs after the descriptor is attached and marked `CON_PLAYING`; it does not open a second logical session. Legacy nanny, account-selection, and WebSocket reconnects are covered. |
| Explicit quit | `src/cmd/actoth.c:do_quit` | `telemetry_runtime_game_session_exit(...logout)` | Mortal quit changes the descriptor to `CON_PWD_D_CONF` before terminal save, so the explicit hook runs after a successful save and before extraction while the connection identity is still available. |
| Camp / rent / terminal unload | `src/cmd/actoth.c:do_camp`, rent callers, and `src/world/handler.c:extract_char_after_terminal_save` | Central `telemetry_runtime_game_session_exit` with `logout` for a live descriptor, otherwise `disconnect` | The handler runs only after the authoritative terminal save succeeds. A nonzero runtime session sequence prevents the explicit quit hook from being emitted twice. |
| Idle exit | `src/world/limits.c` idle-rent path | `close_socket` detach/linkdead, then the central terminal-save exit hook | Socket close remains before `RENT_LINKDEAD` save, preserving the existing persistence order. AFK transition also requests a context observation. |
| Admin kick / forced socket close | `src/net/comm.c:close_socket` | Same detach and linkdead adapters as link loss | This records the connection edge without treating a resident character as a logical logout. Any later terminal extraction closes the logical session. |
| Copyover / process handoff | `src/persistence/copyover.c` | Runtime handoff/resume adapters | Version 15 stores bounded optional telemetry records alongside the existing world format; versions 12–14 resume with absent handoffs. |

The copyover reader bounds telemetry allocation by `FD_SETSIZE`, the server's
accepted-socket ceiling. Corrupt record fields are consumed and discarded without
misaligning the following world section. Recovery visits every recovered player
once: missing, unmatched, or duplicate metadata becomes an absent handoff, and a
runtime-rejected handoff retries as absent after identity rollback. An absent
handoff starts a new session with uncertain/unclosed-tail quality, not fabricated
continuity. Allocation failure also consumes the framing and falls back to absent
handoffs. Truncated files and invalid framing still fail closed because the
mandatory world section cannot be located safely.

Handoff creation flushes the activity classifier and snapshots session counters
at one shared monotonic/UTC observation. If file publication or `exec` fails,
the original process can continue recording activity without overlapping or
rejecting the next counter delta. Failed flush/counter admission never returns
a valid continuity token. Candidates for all preserved players are buffered
within the descriptor ceiling, then one `telemetry_runtime_flush_for_copyover`
barrier waits at most 250ms for the existing worker. The game loop is already
paused by synchronous copyover: no further telemetry admission occurs during
this wait. Generation-tagged acknowledgement covers queued and in-flight
records, not merely queue depth, and requires applied/duplicate/stale totals to
match admitted totals with no permanent invalid/conflict results. The check is
conservative across the current producer's history. The worker checks time again
after synchronous repository work and accounting; a drained batch completed too
late is not a timely acknowledgement. The caller snapshots the acknowledgement
before sampling its clock so descheduling cannot admit a late publication using
a stale time reading. It does not stop the worker.
Timeout, unavailable clock/DB, rejected records, or buffer allocation failure
writes absent handoffs while preserving the ordinary game snapshot. These
paths claim neither telemetry durability nor session continuity. Failed file
publication/exec leaves normal capture and the writer usable without a restart.
The private telemetry SQL socket is close-on-exec so
the old producer cannot strand its writer advisory lock in the replacement
process. Failed exec leaves the socket and original writer usable.

Periodic checkpoints use each session's already-accounted classifier observation,
not the current pulse time for an unvisited staggered bucket. This preserves
contiguous deltas and prevents a later handoff from losing continuity.
The runtime counter boundary additionally checks the classifier cut and its
cumulative category totals before accepting a delivery. It is not a second
elapsed-time producer API: independently advancing session counters would poison
subsequent classifier deltas. Rejected pulse counter deliveries return their
failure outcome with sequence-gap quality, rather than reporting success.
Record-key exhaustion does not undo a classifier boundary already applied to
its in-memory state: detach, attach, and exit still complete the corresponding
session mutation. The call retains its record-loss outcome and gap quality;
the allocator remains exhausted and never wraps or fabricates a replacement key.

## Producer incarnation ownership

Default runtime identities use OpenSSL CSPRNG output for both nonzero producer
tokens, not wall-clock/PID combinations. Random-source failure or four invalid
zero-token draws fail telemetry bootstrap closed, without a weak fallback.
A fixed process-lifetime ledger retains up to 256 enabled incarnation pairs;
reinitialization with a used pair or exhausted ledger returns `invalid` rather
than resetting record sequences under an old identity. Final reap never clears
this ledger. A fresh runtime after shutdown must request fresh options.
The runtime passes its identity through transport's optional `fresh_producer`
repository setting. Zero remains reserved for generic immutable replay clients;
runtime capture always requests the new-incarnation check. On first SQL startup,
while holding the database writer lock, the repository probes retained immutable
facts through the `(boot_id, process_id)` prefix of the replay-key index. A prior
fact or failed lookup leaves telemetry unavailable; a successful check admits
only records from that producer. This detects collisions against retained facts,
not a permanent registry of identities whose facts have been removed.
Same-lifetime connection recovery preserves the successful check and immutable
pending batch so an ambiguous commit can resolve as `duplicate_identical`.
Post-join repository shutdown resets freshness for the next lifetime. The
`test_telemetry_repository.py --sql-fixture` disposable-database tests cover these
SQL collision/reconnect paths separately from the fake-repository runtime harness.

## Shutdown ownership

`telemetry_runtime_shutdown` closes admission and waits for worker completion
only until the supplied monotonic deadline. It returns `stopping` when a callback
is still in flight; even `accepted` does not release an enabled runtime's worker
or repository. The lifecycle owner must call `telemetry_runtime_final_reap`
before `shutdown_mysql`. Final reap joins the worker, closes its repository,
then resets transport/config/session state. It may block on a connector callback;
this is a bounded stop request, not a hard deadline for process teardown.

## Copyover metadata privacy

The local copyover file already contains player names and connections; its new
telemetry section uses the same name/fd pair solely for restoration. Those fields
are not sent to the telemetry SQL records. This section does not extend ordinary
player-save persistence.

## Activity and context paths

| Observation | Actual source path | Adapter call / classification | Guard |
|---|---|---|---|
| Recognized command activity | `src/cmd/interp.c:command_interpreter`, after parser, state, permission, special-proc, and item-teleport gates | `telemetry_runtime_game_evidence` with typed `communication`, `interaction`, `combat_participation`, or `player_action` kind | No call for unknown/rejected parser input, lifecycle commands, active pager input (`descriptor->showstr_count`; the `PLR_PAGING_ON` preference does not suppress ordinary commands), or editor input (`descriptor->str`). Raw command text is never passed. |
| Successful movement | `src/cmd/actmove.c:do_simple_move_skipping_procs`, immediately after `char_to_room` and liveness checks | `telemetry_runtime_game_context` followed by typed `movement` evidence | Follower movement is context-visible but is not counted as player movement evidence. Blocked exits do not create movement evidence. |
| Posture/status change | `src/cmd/actmove.c:do_stand`, `do_sit`, `do_kneel`, `do_recline`, `do_rest`, `do_alert`, `do_sleep`, and successful `do_wake` branches | `telemetry_runtime_game_context` rechecks dimensions, combat, and AFK | Called after mutation for a live player in `CON_PLAYING`. Posture alone neither proves activity nor adds a new context category. |
| AFK/status transition | `src/world/limits.c` when idle processing first sets `PLR_AFK` | Context adapter maps the AFK bit to canonical classifier `afk` evidence, ending active time immediately | Clearing AFK does not itself prove activity; a subsequent admitted command supplies typed evidence. |
| Combat context start/stop | `src/combat/fight.c:set_fighting` and `stop_fighting`, after opponent/list mutation | Context adapter describes `combat` while an opponent exists, otherwise `none` | No active evidence from automatic combat. Rejected/no-op mutations and NPC/disconnected observers are excluded. |
| Zone/group dimensions | `telemetry_runtime_game_context` at entry, reconnect, movement, status, and successful `src/guild/group.c` membership boundaries | Runtime derives stable config/version, zone vnum, and bounded group size from captured game state; no SQL or filesystem access occurs on the hook path. | Group add/remove notify surviving connected players; removal observes the departing member and the last survivor after auto-disband. Disband and merge reuse these mutations. |

Group membership mutates in `src/guild/group.c:group_add_member` and
`group_remove_member`. Their post-mutation observations segment stationary
members without reconstructing classifier state in commands. The observer has
a hard 256-node traversal cap; gameplay mutation and its success remain
independent of telemetry admission. Failed adds and no-op removals emit no
membership-change observation. Group merge reuses successful adds; explicit
disband reuses removes. Context rate caps still apply to repeated changes.

## Adapter contract coordinated with the runtime owner

The gameplay enum is compact (`linkdead=7`), while the canonical activity enum
reserves intermediate values (`linkdead=16`). The runtime uses an explicit
switch mapping for every admitted evidence kind and rejects unmapped values;
it does not cast between the enums. The enabled adapter regression executes
and accepts the real detach/linkdead path.


Every gameplay adapter is best-effort. The disabled, flat-file, uninitialized,
queue-full, or stopping outcomes are ignored by the gameplay caller; existing
socket, save, login, and extraction behavior continues. Runtime identity fields
are runtime-only and are cleared by the enabled exit/detach adapters. They are
not added to ordinary player saves; the explicit copyover handoff is the sole
restart bridge described above.

## Focused regressions

- `python3 tests/async/test_telemetry_gameplay_hooks.py` checks the traced source
  paths, ordering, pager/editor guards, no-raw-input contract, and disabled-path
  coverage markers.
- `python3 tests/async/test_telemetry_gameplay_adapters.py` compiles the real
  runtime adapters against `char_data`/`descriptor_data`, then exercises disabled
  entry/evidence/exit, entry, context, typed activity, detach/linkdead,
  reconnect, and final exit with runtime identity reset assertions. Its emitted
  interval checks prove idle combat is not human activity and AFK ends active
  accounting before the active-window timeout. Repeated handoff attempts followed
  by continued activity also remain accepted (failed-copyover continuation).
- `python3 tests/async/test_telemetry_combat_hooks.py` checks start placement after
  existing combat gates, executes the accepted-start mutation block and complete
  `stop_fighting` body, and exercises the observer's NPC/disconnected/no-op guards.
  It is a focused boundary fixture, not a simulated full combat engine.
- `python3 tests/async/test_telemetry_group_hooks.py` executes the actual group
  add/remove/disband bodies with isolated observation stubs. It checks all
  affected member snapshots, leader transfer, rejection/no-op behavior,
  two-member auto-disband and the observer's traversal cap.
- `python3 tests/async/test_telemetry_runtime_integration.py` exercises the real
  lifecycle APIs, including rejected-handoff identity rollback followed by absent
  resume and uncertain-tail evidence.
- `python3 tests/async/test_telemetry_copyover_format.py` compiles the actual wire
  helpers and exercises temporary-file byte round trips, unavailable telemetry,
  legacy absent recovery, damaged/unmatched/duplicate entries, invalid handoffs,
  allocation failure, count limits, and truncated/mismatched framing. A following
  world-section marker proves cursor preservation. Runtime observation stubs are
  explicit; this is not an end-to-end server `exec` journey.
- `python3 tests/async/test_copyover_save_guards.py` retains the existing gameplay
  save/publication/recovery ordering checks and executable pet ownership capture.
- `python3 tests/async/test_copyover_custody.py` exercises the production
  save/exec/recover path with a no-player/no-NPC fixture under ASan/UBSan. The
  version-15 zero-entry telemetry frame must preserve corpse/container/loot
  bytes and the live custody ledger; calling any player telemetry adapter aborts
  this fixture. This proves world-state compatibility, not a live-player journey.
- `python3 tests/async/run_telemetry_player_journey.py ABS_SQL_BINARY ENV_JSON`
  requires a fresh task-owned `duris_265_*test` SQL fixture in an isolated network.
  It exercises disabled creation/login/save/quit, enabled MCCP failed-copyover
  continuation, real exec into a new producer with one logical session,
  link-loss reconnect, durable exit and exact counter conservation. It then
  makes only the private telemetry connection unavailable and proves ordinary
  login/look/save/quit still work with no new telemetry facts. The fixture never
  reads a production environment file or restores production player data.

# Game-loop phase contract

`game_loop()` is the single game-thread owner of simulation pulses and network
turns. The loop's
top level is intentionally an orchestration boundary: it prepares the pulse
context, invokes the named phases below in order, and leaves shutdown and
copyover handling after the pulse loop.  The helpers do not introduce a second
event loop, an async execution framework, or a new owner for descriptors,
queues, prompts, world state, or persistence receipts.

## Normal pulse order

The order is observable behavior and must be preserved when a phase is edited
or moved.  A phase may call existing subsystem owners, but it must not move
work across this table without a separate behavior change and trace evidence.

| Order | Helper | Owns | Ordering and lifetime contract |
| --- | --- | --- | --- |
| 1 | `run_connection_phase` | Signal/lifecycle requests, persistence-log polling, hostname answers, one nonblocking `service_network_turn`, staged WebSocket application messages, and descriptor teardown | Ready accepts, TLS negotiation, bounded reads/parsing, retained transport bytes, and socket errors are serviced before session input. WebSocket application handlers and link-loss effects retain this boundary. An interrupted poll retries without advancing a logical tick; a hard poll/listener error requests shutdown. |
| 2 | `run_session_input_phase` | WebSocket/login timeouts and pings, wait/slow gates, asynchronous authentication completion, selective queue dequeue, and pager/editor/playing/nanny dispatch | Authentication type-ahead remains distinct from playing input. Casting/item-action and creation-grant gates are evaluated before dequeue; the existing command latency prologue ends before queue/gate work. |
| 3 | `run_output_phase` | Existing telnet/WebSocket bytes, partial writes, application output, control frames, prompt-related flush/close transitions, and queued ping completion | Retained bytes drain before new framing. Output remains before `ne_events()` and before recurring durable-completion publication. |
| 4 | `run_event_phase` | `ne_events()`, telemetry pulse accounting, creation-grant preparation, artifact mana, and device actions | `ne_events()` closes the current tick's pre-event scheduling window. Creation/artifact/device work remains after it. |
| 5 | `run_recurring_persistence_phase` | Every-two-pulse GMCP/ship, locker/corpse, critical completion routing, outbox publication, saves, load/recovery, caches, and maintenance completion delivery | Durable completions and outboxes retain their existing `pulse % 2` cadence and follow `ne_events()`. No completion is moved ahead of input or output. Maintenance completions remain before due activities. |
| 6 | `run_activity_phase`, then `run_combat_phase` | Due activities, violence, and descriptor-related map/group/movement updates | Activities precede combat exactly as before; descriptor traversal remains on the game thread and uses borrowed live pointers only for the duration of the phase. |
| 7 | `run_pulse_reset_phase` | Tick advance, due affect/point updates, latency diagnostics, trace snapshots, and `network_wait_until` | The event tick advances after combat. Ready network turns and completion hints service the remaining interval without returning to session/world phases early. Overruns discard missed wall-clock slots and wait a full interval before the next simulation pulse. |

The top-level call sequence is therefore:

```text
connection → session input → output → ne_events/artifact/device
          → recurring persistence/maintenance → activities → combat
          → tick/affects/points/diagnostics/wait
```

## Network turns and deadlines

`service_network_turn()` also runs inside the remaining-pulse wait. Its direct
`pollfd` entries remove the numeric `FD_SETSIZE` ceiling; admission still caps
live connections at `MAX_CONNECTIONS` (256), including TLS negotiations and
WebSocket upgrades. Each listener accepts at most 32 connections per turn. The
simulation boundary also probes each nonblocking listener, preserving the accept
watchdog. Every client receives at most one socket read per turn: Telnet/TLS input
retains its existing allowance of at most 4,799 bytes per simulation pulse;
WebSocket input retains 4,096 bytes per pulse and
parses at most 64 frames. WebSocket text messages share a FIFO dispatched only in
the connection phase. Reads pause at 64 pending messages or 1 MiB of pending text;
the last valid message can add at most another 1 MiB. Buffered frames resume after
dispatch releases that backpressure, while incomplete frames wait for more bytes.
Multiple ready reads can spend that allowance between boundaries. Once it is
spent, read interest pauses until the next connection phase completes; queued commands
remain intact, including the existing selective casting/transaction dequeue.
Telnet transport flushes are bounded to 16 KiB per
call; WebSocket queues retain their existing flush and memory limits.

Trusted PROXY v1 headers are parsed with the buffered WebSocket HTTP handshake,
so a prompt accept cannot race a fragmented header. The immediate peer must match
the configured trusted proxy; an IPv4 address and its IPv6 mapped form identify
the same peer. The source and destination addresses and ports are validated before
the source address is used by handshake access checks.

Ordinary write interest exists only for retained transport bytes. A handshake or
receive retry can also request write readiness through its captured GnuTLS
direction, and a retained send independently keeps
its direction and resumes the library-owned record before another TLS operation.
GnuTLS-buffered plaintext can trigger another bounded turn without another kernel
read event. A monotonic 40-second handshake admission deadline preserves the
existing GnuTLS default even when a peer stops sending. The wait chooses the nearer
handshake or simulation deadline and rounds up to `poll()`'s millisecond granularity.

These turns stage input and drain already-framed output. Gameplay command
selection, nanny/pager/editor dispatch, authentication completion, gameplay prompt
construction, events, recurring persistence, activities, and combat stay in the
table's simulation phases. WebSocket login, character entry, account actions, and
GMCP messages also retain the connection phase. Telnet negotiation, HTTP upgrade,
and WebSocket control frames keep their transport behavior. Socket failures stage
descriptor teardown for the connection phase, retaining link-loss/save semantics;
failed descriptors are excluded from the intervening waits. An orderly EOF after
newly staged bytes first retains their existing single connection/session boundary
opportunity; a normal WebSocket close likewise preserves application messages
queued before its close frame. Transport failures and EOF after previously
offered input get no extension. An interrupted
poll does not consume this opportunity. Queued gameplay output and prompts are
framed in `run_output_phase`; existing direct protocol responses, including
upgrade/welcome messages and Telnet negotiation, can send in network turns.
Partial wire bytes may finish between pulses. WebSocket
reconnect handlers can remove another descriptor, so connection-phase traversal uses the
existing close-aware `next_to_process` cursor. Readiness is assigned before any
accept/close operation and is never inherited by a reused descriptor number.

Password, account load, mail, hostname lookup, player load/save, critical commands, lockers, maintenance,
collector listings, world recovery, quest acknowledgements, and incoming donation
events notify a nonblocking process-owned pipe after queue publication. A full
pipe coalesces hints; authoritative queue results remain retained. Draining hints
wakes networking but does not consume or publish those results. In particular,
durable publication still follows `ne_events()` on `pulse % 2`. The pipe stays open
through worker teardown, closes on copyover exec, and is reconstructed on boot.

## Elapsed time, logical ticks, and backlog

Normal pulse starts remain 250 ms apart on `CLOCK_MONOTONIC`. Packets, writable
sockets, worker hints, and signal interruptions do not advance `ne_event_tick` or
make commands eligible early. Wall-clock time continues to drive existing login
and liveness policies independently of simulation ticks.

If a simulation pulse takes at least 250 ms, the next pulse is scheduled 250 ms
after that pulse finishes. Missed elapsed-time slots are discarded: there is no
tick skipping or catch-up burst. One completed world pulse still advances exactly
one logical event tick, so a long stall slows simulation time. Existing slow-tick
diagnostics measure world work before the network wait. A network turn that crosses
the deadline finishes its bounded sweep before the next pulse; it cannot preempt a
synchronous world callback. Scheduler backlog is a separate quantity: `ne_events()`
retains its existing due-tick ordering, budgets, deferral, and catch-up policy. No
scheduler callback is dispatched from a network turn, and no real-time debt is
converted into extra scheduler ticks.

## Session input decision boundary

`session_input_route` is a small decision result, not a session or descriptor
owner.  `select_session_input()` decides whether a line is kept queued, pulled
from the restricted casting queue, pulled from the transaction-aware playing
queue, or pulled from the ordinary queue.  `dispatch_session_input()` then
hands the line to the existing pager, editor, playing interpreter, or nanny
handler.  Authentication work is checked separately because a password line is
not a playing command.

The decision preserves the existing distinctions:

- `PLR2_WAIT`, scheduled `event_wait`, casting, and active item-action gates
  control command eligibility without dropping ordinary type-ahead.
- Creation-grant admission blocks only playing commands; pre-entry nanny input
  continues through the login/creation flow.
- Charm/original-descriptor rules remain part of the playing eligibility check.
- Pager and editor input is routed by descriptor state, not parsed as a world
  command.  Playing commands continue through paging-aware dispatch.
- A completion can update live state while its output remains queued; output
  backpressure and descriptor lifetime stay with networking.

The pulse context contains only the per-pulse references needed by the existing
code: listener descriptors, scratch buffers, signal/time state, debug counters,
and trace measurements.  It is created and consumed within one game-thread
iteration.  No worker or completion path receives a descriptor pointer through
this boundary.

## Non-goals and verification

This loop does not change command metadata, socket ownership, gameplay queue
capacity, transaction admission, durable journaling, receipt identity, output
formatting, or the number/cadence of world and persistence calls.  It does not
make synchronous journal or database work part of the pulse helper contract.

The phase contract is guarded by `tests/async/test_game_loop_phase_contract.py`
and the command-latency runtime/source contract.  Runtime coverage keeps the
existing casting, command-gate, authentication, item/currency queue, output,
telnet/WebSocket, and session-journey tests in the validation set.  Build and
sanitizer results are recorded with the implementation PR; a local compiler
limitation is reported separately rather than treated as passing coverage.
Readiness execution and before/after listener measurements are covered by
`test_network_readiness_runtime.py` and `test_network_readiness_journey.py`; results
and practical limits are recorded in [NETWORK_READINESS_VALIDATION.md](NETWORK_READINESS_VALIDATION.md).

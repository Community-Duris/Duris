# Game-loop phase contract

`game_loop()` is the single game-thread owner of one server pulse.  The loop's
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
| 1 | `run_connection_phase` | Signal/lifecycle requests, persistence-log polling, readiness sets, hostname answers, `select`, async I/O, accepts, descriptor reads, and socket errors | Nonblocking readiness and accept draining happen before session input. A failed readiness poll returns to the next pulse after restoring the signal mask. |
| 2 | `run_session_input_phase` | SSL negotiation, WebSocket/login timeouts and pings, wait/slow gates, asynchronous authentication completion, selective queue dequeue, and pager/editor/playing/nanny dispatch | Authentication type-ahead remains distinct from playing input. Casting/item-action and creation-grant gates are evaluated before dequeue; the existing command latency prologue ends before queue/gate work. |
| 3 | `run_output_phase` | Existing telnet/WebSocket bytes, partial writes, application output, control frames, prompt-related flush/close transitions, and queued ping completion | Retained bytes drain before new framing. Output remains before `ne_events()` and before recurring durable-completion publication. |
| 4 | `run_event_phase` | `ne_events()`, telemetry pulse accounting, creation-grant preparation, artifact mana, and device actions | `ne_events()` closes the current tick's pre-event scheduling window. Creation/artifact/device work remains after it. |
| 5 | `run_recurring_persistence_phase` | Every-two-pulse GMCP/ship, locker/corpse, critical completion routing, outbox publication, saves, load/recovery, caches, and maintenance completion delivery | Durable completions and outboxes retain their existing `pulse % 2` cadence and follow `ne_events()`. No completion is moved ahead of input or output. Maintenance completions remain before due activities. |
| 6 | `run_activity_phase`, then `run_combat_phase` | Due activities, violence, and descriptor-related map/group/movement updates | Activities precede combat exactly as before; descriptor traversal remains on the game thread and uses borrowed live pointers only for the duration of the phase. |
| 7 | `run_pulse_reset_phase` | Tick advance, due affect/point updates, latency diagnostics, trace snapshots, remaining-pulse wait, and end-of-pulse time refresh | The event tick advances after combat. The wait is bounded by the pulse budget and may return early on an interrupt; the next top-level pulse retries. |

The top-level call sequence is therefore:

```text
connection → session input → output → ne_events/artifact/device
          → recurring persistence/maintenance → activities → combat
          → tick/affects/points/diagnostics/wait
```

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

This extraction does not change command metadata, socket ownership, queue
capacity, transaction admission, durable journaling, receipt identity, output
formatting, or the number/cadence of world and persistence calls.  It does not
make synchronous journal or database work part of the pulse helper contract.

The phase contract is guarded by `tests/async/test_game_loop_phase_contract.py`
and the command-latency runtime/source contract.  Runtime coverage keeps the
existing casting, command-gate, authentication, item/currency queue, output,
telnet/WebSocket, and session-journey tests in the validation set.  Build and
sanitizer results are recorded with the implementation PR; a local compiler
limitation is reported separately rather than treated as passing coverage.

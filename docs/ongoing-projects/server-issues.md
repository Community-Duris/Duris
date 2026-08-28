# Server Issues & Diagnostic Findings

This document summarizes known runtime issues, performance bottlenecks, and configuration discrepancies identified during server builds, health monitoring, and log audits.

Items 1–4 were re-verified against the tree and remediated; each records what the
code actually did and what changed. Item 5 remains open investigation.

---

## 1. Periodic Game Tick Lag (`aff/pts` Spikes) — instrumentation fixed, scan under observation

* **Symptoms:**
  * Staff characters (Level 56+ with `toggle status` enabled) receive periodic in-game alerts:
    ```text
    *** STATUS: MUD TICK TOOK TOO LONG - loop time - 1.310889
      - aff/pts time - 1.289582
    ```
* **Findings:**
  1. **Multi-Threaded `clock()` Aggregation (confirmed, fixed):** `game_loop()` in `src/comm.c`
     measured every section with `clock()`. On POSIX/Linux `clock()` returns CPU time summed
     across all threads of the process, so the MySQL worker pool and the Redis subscriber were
     folded into the loop's own numbers. The reported figures were not tick latency at all.
  2. **Stale profile accumulators (found during the audit, fixed):** the `connections`,
     `commands`, `prompts`, `activities`, and `combat` lines of the stall report were derived
     from the `PROFILE_START`/`PROFILE_END` accumulators, which only move when `do_profile` is
     enabled. With profiling off — the normal case — those five lines reported stale values.
     `aff/pts` and the total were two of the few sections timed directly, which is why `aff/pts`
     always appeared to be the culprit.
  3. **Global Unindexed Mobile Scan (confirmed, not changed):** once per game tick,
     `affect_update()` (`src/affects.c`) walks all of `character_list` linearly. The per-mobile
     work is cheap — `StartRegen()` is guarded by a resource check and `get_scheduled()` walks
     only that character's own `nevents` list — so the scan is O(mobiles) with a small constant,
     not the multi-second cost the old metric implied.
* **Resolution:**
  * `src/comm.c` now times the loop and every reported section with a `loop_monotonic_seconds()`
    helper backed by `clock_gettime(CLOCK_MONOTONIC)`, unconditionally rather than through the
    profiler accumulators. `latency_trace_record()` and the stall report therefore carry real
    elapsed time.
  * The active-list refactor of `affect_update()` is deliberately **not** done yet: the only
    evidence for it was the discredited metric. Re-measure with the corrected instrumentation
    before restructuring affect ownership; the cost of that refactor is not justified until a
    monotonic measurement shows the scan actually dominates a tick.
* **Regression test:** `tests/async/test_tick_latency_instrumentation.py`

---

## 2. WebSocket Listener Address Binding Failure — fixed

* **Symptoms:**
  * `logs/log/status` records:
    ```text
    WebSocket listener address is invalid
    WARNING: WebSocket server failed to start on port 4050
    ```
  * `./scripts/healthcheck.sh` fails with `curl: (7) Failed to connect to 127.0.0.1 port 4050`.
* **Root Cause (confirmed):**
  * `websocket_listener_address()` in `src/websocket.c` required either
    `DURIS_WEBSOCKET_LISTEN_ADDRESS` or `LISTEN_ADDRESS` to be set and aborted `websocket_init()`
    otherwise. The telnet listener does not: `runtime_listener_address()` falls back to
    `in6addr_any`. A `.env` that configures only a port therefore started the game with no
    WebSocket listener and no health endpoint.
* **Resolution:**
  * `websocket_listener_address()` falls back to `127.0.0.1` when neither variable is set. That
    is strictly narrower than the telnet listener's unset default, and production still requires
    an exact loopback bind plus `DURIS_TRUSTED_PROXY_IP` and `DURIS_WEBSOCKET_ALLOWED_ORIGINS`.
    Explicit configuration still wins, and a non-numeric address is still rejected.
  * `docs/operations/CONFIGURATION.md` documents the default.
* **Regression test:** `tests/async/test_websocket_listener_default.py`

---

## 3. Hardcoded Legacy Latency Trace Path — fixed

* **Symptoms:**
  * Latency tracing log files are not generated in `logs/`.
* **Root Cause (confirmed):**
  * The periodic dump in `src/comm.c` opened the absolute path
    `/durismud/logs/latency_trace.log`, which does not exist outside the legacy deployment
    layout. The `fopen` failed silently and only the `stderr` copy survived.
* **Resolution:**
  * The dump now targets `logs/latency_trace.log`, relative to the server's working directory
    like every other log.
* **Regression test:** `tests/async/test_tick_latency_instrumentation.py`

---

## 4. Post-Death Login Failure / Item Ownership Desynchronization — fixed

* **Symptoms:**
  * A player character dies, disconnects, and upon reconnecting receives:
    ```text
    Sorry, I couldn't load that character!
    ```
  * `logs/log/debug` records:
    ```text
    player_load_materialize: component=snapshot pid=<PID> outcome=3 error=0 repository_component=items queries=15 rows=2 items=0
    ```
* **Root Cause (confirmed, with one correction):**
  1. The asynchronous submitter is `submit_next_corpse_item()` in `src/fight.c`, not
     `account_bound_reward_prepare_player_corpse()` — the latter only dissolves account-bound
     reward containers synchronously. `make_corpse()` submits one item transfer, and
     `corpse_item_completion()` submits the next when that one commits, so a full inventory
     takes one transaction round trip per item.
  2. `item_movement_transaction_handle_completions()` publishes a completion only when
     `find_player_by_pid()` still resolves the owner. `die()` proceeded straight to
     `persistence_save_character_terminal(ch, RENT_DEATH)` and `extract_char()`, so the chain
     stopped mid-flight and the pending entries were retained offline.
  3. The remaining items stayed as `active` rows in `item_current_owner` owned by the player,
     while the terminal save wrote an empty `player_items`.
  4. On the next login `load_items()` (`src/player_load_repository.c`) evaluates
     `ownership_summary_sql`, sees `owned_count != payload_count`, and fails the item custody
     invariant with `player_load_outcome::component_failure` (outcome 3).
* **Resolution:**
  * `die()` now defers the terminal save and the extraction while
    `item_movement_transaction_player_busy(ch)` is true, reusing the `schedule_death_extract_retry()`
    path that already existed for failed terminal saves. `event_death_extract_retry()` re-checks
    the same condition before saving. The corpse chain therefore drains against a live character,
    and the database rows and the saved payload stay in agreement. A rejected or unsubmittable
    transfer clears the pending entry, so the deferral cannot wedge a death permanently.
  * Reconciliation of pre-existing orphaned rows stays an operator action:
    `migrations/reconcile_item_ownership.sh` reports the mismatches. Automatic deletion of active
    `item_current_owner` rows during login recovery is intentionally not implemented — it would
    destroy legitimately owned items whenever a save is merely late.
* **Regression test:** `tests/async/test_death_item_custody_contract.py`

---

## 5. Pending / Active Investigations (from `todo.md`)

* **Storage Lockers:** Item deletion/loss reports under edge cases (e.g., during async persistence transitions).
* **In-Game Copyover Gear Retention:** Auditing player inventory preservation during copyovers (`RESULT=53` / `57`).
* **Redis Architecture:** Clarifying primary vs. caching/fallback roles for world-state recovery.

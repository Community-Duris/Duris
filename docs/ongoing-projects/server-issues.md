# Server Issues & Diagnostic Findings

This document summarizes known runtime issues, performance bottlenecks, and configuration discrepancies identified during server builds, health monitoring, and log audits.

---

## 1. Periodic Game Tick Lag (`aff/pts` Spikes)

* **Symptoms:**
  * Staff characters (Level 56+ with `toggle status` enabled) receive periodic in-game alerts:
    ```text
    *** STATUS: MUD TICK TOOK TOO LONG - loop time - 1.310889
      - aff/pts time - 1.289582
    ```
* **Root Causes:**
  1. **Global Unindexed Mobile Scan:** Once per game tick (`!pulse`, roughly every 60–75 seconds), [`affect_update()`](file:///home/aiwithapex/projects/duris/src/affects.c#L4196) runs a synchronous linear iteration across all 17,680+ mobiles in `character_list` across 352 zones to evaluate regen, falling states, disguise expiration, and affect duration ticks.
  2. **Multi-Threaded `clock()` Aggregation:** In [`src/comm.c`](file:///home/aiwithapex/projects/duris/src/comm.c#L1712), elapsed time is measured using standard C library `clock()`. On POSIX/Linux, `clock()` returns cumulative CPU time across all threads of the process. Because Duris runs background worker pools (MySQL worker threads, Redis subscriber), their concurrent CPU ticks get combined into `affect_and_points_time`, inflating the measured loop time.
* **Recommended Next Steps:**
  * Maintain active lists for entities with active non-short affects rather than scanning all ~17k+ world mobs every tick.
  * Switch timing diagnostics in [`src/comm.c`](file:///home/aiwithapex/projects/duris/src/comm.c) from `clock()` to monotonic wall-clock time (`clock_gettime(CLOCK_MONOTONIC)`).

---

## 2. WebSocket Listener Address Binding Failure

* **Symptoms:**
  * `logs/log/status` records:
    ```text
    WebSocket listener address is invalid
    WARNING: WebSocket server failed to start on port 4050
    ```
  * `./scripts/healthcheck.sh` fails with `curl: (7) Failed to connect to 127.0.0.1 port 4050`.
* **Root Cause:**
  * [`src/websocket.c`](file:///home/aiwithapex/projects/duris/src/websocket.c#L46-L68) strictly requires either `DURIS_WEBSOCKET_LISTEN_ADDRESS` or `LISTEN_ADDRESS` to be set in the environment (e.g., `127.0.0.1` or `::1`).
  * If `.env` only specifies `MUD_WEBSOCKET_PORT` without `DURIS_WEBSOCKET_LISTEN_ADDRESS`, socket initialization aborts early.
* **Recommended Next Steps:**
  * Provide a default fallback (e.g. `127.0.0.1` or `::1`) in `websocket_listener_address()` or document `DURIS_WEBSOCKET_LISTEN_ADDRESS=127.0.0.1` in `.env.example` / `.env`.

---

## 3. Hardcoded Legacy Latency Trace Path

* **Symptoms:**
  * Latency tracing log files are not generated in `logs/`.
* **Root Cause:**
  * In [`src/comm.c`](file:///home/aiwithapex/projects/duris/src/comm.c#L1734), the file write targets a hardcoded absolute path `/durismud/logs/latency_trace.log` instead of the local relative path `logs/latency_trace.log`.
* **Recommended Next Steps:**
  * Update [`src/comm.c`](file:///home/aiwithapex/projects/duris/src/comm.c#L1734) to use `logs/latency_trace.log`.

---

## 4. Post-Death Login Failure / Item Ownership Desynchronization

* **Symptoms:**
  * A player character dies, disconnects, and upon reconnecting receives:
    ```text
    Sorry, I couldn't load that character!
    ```
  * `logs/log/debug` records:
    ```text
    player_load_materialize: component=snapshot pid=<PID> outcome=3 error=0 repository_component=items queries=15 rows=2 items=0
    ```
* **Root Cause:**
  1. In [`make_corpse()`](file:///home/aiwithapex/projects/duris/src/fight.c#L1568), [`account_bound_reward_prepare_player_corpse()`](file:///home/aiwithapex/projects/duris/src/fight.c#L1681) submits an **asynchronous** transaction chain ([`item_movement_transaction_submit`](file:///home/aiwithapex/projects/duris/src/fight.c#L1555)) to transfer item ownership from `owner_type::player` to `owner_type::corpse` one item at a time.
  2. Meanwhile, [`die()`](file:///home/aiwithapex/projects/duris/src/fight.c#L3039) immediately proceeds synchronously, calls [`persistence_save_character_terminal(ch, RENT_DEATH)`](file:///home/aiwithapex/projects/duris/src/fight.c#L3039) (which saves `player_items` as empty), and extracts the character via [`extract_char()`](file:///home/aiwithapex/projects/duris/src/fight.c#L2959).
  3. Because the character is extracted/freed before the async corpse item transfer completion finishes, the remaining items are never transferred in the `item_current_owner` table. They remain registered as active items owned by the player in `item_current_owner`, while `player_items` has 0 rows.
  4. On subsequent login, [`load_items()`](file:///home/aiwithapex/projects/duris/src/player_load_repository.c#L860-L901) runs `ownership_summary_sql`. It finds `missing_count > 0` and `owned_count != payload_count`, flags a fatal item custody invariant violation (`player_load_outcome::component_failure`, outcome 3), and rejects the login.
* **Recommended Next Steps:**
  * Synchronously transfer item ownership to the corpse during `make_corpse()` or flush/await all corpse item transfers prior to terminal death saving and character extraction.
  * In the recovery pipeline, reconcile orphaned active `item_current_owner` entries when an extracted dead player is saved with an empty inventory.

---

## 5. Pending / Active Investigations (from `todo.md`)

* **Storage Lockers:** Item deletion/loss reports under edge cases (e.g., during async persistence transitions).
* **In-Game Copyover Gear Retention:** Auditing player inventory preservation during copyovers (`RESULT=53` / `57`).
* **Redis Architecture:** Clarifying primary vs. caching/fallback roles for world-state recovery.

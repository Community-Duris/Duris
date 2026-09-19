# Issue #299: world activity throttling

This document records the implementation and the follow-up performance work
identified while tracing the issue. The source issue is
[Community-Duris/Duris #299](https://github.com/Community-Duris/Duris/issues/299).

## Plan and status

1. Introduce a disabled-compatible policy/configuration boundary and keep the
   legacy cadence as the rollback contract. **Done.**
2. Maintain player, adjacent, grace, NPC-index, and PC-corpse membership from
   committed lifecycle hooks. **Done.**
3. Route ordinary mundane reschedules through one policy, promote affected
   work in bounded windows, and rebuild once after boot/recovery. **Done.**
4. Verify source/build contracts, then measure matched empty, sparse, busy,
   corpse, travel, wake, and recovery workloads before tuning defaults further.
   **Source/build checks done; runtime benchmark awaits a fully provisioned
   server build.**

## Implemented policy

`src/world/world_activity.c` owns the policy and its indexes. It deliberately
changes only the ordinary `event_mob_mundane` cadence. Combat, patrol, mobile
special procedures, room/object procedures, and other event families retain
their existing schedules.

The current defaults are:

| State | Ordinary mundane cadence |
| --- | --- |
| Active | `PULSE_MOBILE` (30 pulses, about 7.5 seconds) |
| Nearby/recent | `PULSE_MOBILE * 3` (90 pulses, about 22.5 seconds) |
| Distant idle | 60 seconds (240 pulses) |

The existing small random jitter is retained. The feature is enabled by
default, but the legacy behavior remains available with
`world.activity.enabled=0.000`: occupied zones use 1× and playerless zones
use `PLAYERLESS_ZONE_SPEED_MODIFIER` (3×), matching the old branch.

An NPC is forced back to the active cadence when it is fighting, pursuing or
remembering a target, controlled, mounted, casting/memorizing/scribing,
injured, patrolling, sentinel, trusted, or attached to an audited special or
quest procedure. This conservative eligibility boundary avoids delaying
timing-sensitive work merely because a zone is empty.

## Activity membership and wake behavior

- Live PCs contribute a direct player reason to their zone.
- Each reason also contributes a one-hop adjacent reason using actual room
  exits. A leaving reason leaves a configurable grace period (90 seconds by
  default), which prevents rapid movement and corpse transitions from
  repeatedly cold-starting a zone.
- NPCs are indexed by zone as they enter and leave rooms. A promotion only
  inspects scheduled mundane events for NPCs in affected zones; it does not
  scan `character_list` on every wake.
- A promotion advances delayed mundane events into a bounded window of at most
  one normal ambient interval, with a per-NPC spread. It does not replay
  missed work or fire an entire region at once.
- Combat start explicitly promotes both participants. The next ordinary
  schedule also re-evaluates the timing-sensitive eligibility boundary.

## PC-corpse contract

Only an object that is both `ITEM_CORPSE` and `PC_CORPSE` and is not marked
`NPC_CORPSE` contributes a corpse reason. The hooks cover the committed
transitions in `world/handler.c`:

- room, inventory, worn, and nested-container arrival/removal;
- character movement carrying or wearing a subtree;
- extraction, resurrection/loot paths, and corpse decay via the existing
  object lifecycle functions.

The effective room follows an inside-container chain and then the room of the
carrier/wearer. Failed publication paths do not call an arrival hook, so they
do not create a duplicate reason. Multiple corpses are counted independently.
The post-boot recovery rebuild walks top-level live object subtrees once and
reconstructs the NPC and activity indexes after copyover/Redis recovery.

Corpse reasons affect only NPC scheduling. They do not alter corpse timers,
loot/resurrection behavior, persistence, or extraction semantics.

## Runtime settings

The settings are read and reloaded through the existing property path:

| Property | Default | Bounds/meaning |
| --- | ---: | --- |
| `world.activity.enabled` | `1` | Feature switch; `0` restores the legacy cadence |
| `world.activity.distant.seconds` | `60` | Distant delay, bounded to at least one normal interval and at most one hour |
| `world.activity.grace.seconds` | `90` | Nearby grace after the last reason leaves, bounded to one hour |
| `world.activity.nearby.multiplier` | `3` | Nearby delay multiplier, bounded to 1–8 |
| `world.activity.wake.enabled` | `1` | Enables bounded promotion wakeups |
| `world.activity.diagnostics` | `0` | Logs aggregate membership and wake counters on reload/rebuild |

Disabling the feature advances already delayed indexed mundane events into the
normal window before clearing the index. Re-enabling performs a bounded
rebuild before the cold cadence is used again.

## Verification

The following checks were run in the Windows/WSL environment for this
worktree:

- strict `g++-12 -fsyntax-only` checks passed for the new activity unit and
  the modified `mobact.c`, `db.c`, `handler.c`, `properties.c`, `comm.c`, and
  `fight.c` translation units;
- the new activity unit passed the repository warning profile when compiled
  as an object;
- `git diff --check` passed;
- the complete server link was not available in this environment: the WSL
  image lacks the `hiredis/hiredis_ssl.h` dependency. The first native
  `make -C src` attempt is also unavailable because Windows has no `make`
  executable. The source-level checks therefore remain the verified gate
  here; CI or a fully provisioned Linux build should run the complete build,
  sanitizers, and runtime journeys.

## Consolidated follow-up performance fixes

The same implementation branch also carries two bounded follow-ups discovered
while tracing the wake path:

- each character records the ordinary mundane event pointer together with its
  NEVENT sequence, so an activity wake validates one handle instead of walking
  the character's complete event list; rebuild scans the list once after boot
  or recovery to repopulate the runtime-only cache;
- PC-corpse subtree publication uses a depth-fenced tree walk rather than
  allocating a temporary `std::unordered_set` for every object transfer. The
  containment API already rejects cycles, and the depth fence keeps malformed
  recovery input bounded.

Neither change alters event cadence, corpse ownership, extraction, or combat
semantics. The handle is invalidated by scheduler sequence validation, and the
tree walk's guard preserves a bounded failure mode for corrupt object graphs.

## Follow-up performance options

These are scoped candidates found while tracing issue #299. They should be
benchmarked with the existing NEVENT analytics and game-thread latency traces
before being enabled broadly.

| Option | Concrete scope | Likely benefit | Playability risk | Recommendation |
| --- | --- | --- | --- | --- |
| Central mundane-event handle | Add a validated mundane-event handle or direct lookup slot to `char_data`; replace repeated per-character event-list searches in the #299 wake path and mundane rescheduling. | Reduces wake/reschedule list walks and stale lookup work. | Medium: character lifetime/copyover invalidation must be exact. | Implemented in the consolidated follow-up; keep the sequence validation and recovery rebuild. |
| Reuse the NPC zone index | Let zone reset/purge and other targeted maintenance use the #299 per-zone NPC index instead of broad `character_list` scans where semantics permit. | Makes targeted maintenance proportional to affected zones. | Medium: reset semantics and extraction ordering vary by caller. | Audit one maintenance callback at a time; do not generalize blindly. |
| Replace `remember_array` allocations | Preserve its zone-message/tracking semantics with a counted or intrusive player-presence index, sharing the activity membership boundary. | Removes per-move node allocation/free and linked-list traversal for presence checks. | Medium/high: messaging masks, reconnects, and race-war bookkeeping are coupled. | Profile first; implement as a separate refactor with parity tests. |
| Audit remaining callback families | Use `NEVENT ANALYTICS CALLBACK`/window data to classify `event_mob_proc`, patrol, room, object, and transport callbacks as ambient or timing-sensitive, then add narrow policies only to measured ambient groups. | Avoids applying a blanket multiplier and can reduce idle callback volume beyond mundane work. | High if a hidden special/patrol contract is misclassified. | Highest potential, but only after callback-level measurements and scenario tests. |
| Cache hot property reads | Cache only immutable-per-pulse or explicitly reloadable hot keys after proving they occur in a hot callback. Invalidate from `apply_properties()`. | Removes repeated binary searches and key handling in measured hot paths. | Low/medium: stale balance/config values if invalidation is missed. | Small, low-risk optimization after a profile identifies real property lookups. |
| Cache subtree corpse presence | Add a runtime subtree bit/count so ordinary object moves can skip even the bounded corpse-tree walk. | Reduces the already-scoped object-hook cost when containers move. | Medium: every nesting, extraction, and rebuild mutation must maintain the cache. | A depth-fenced, allocation-free walk is implemented first; add a maintained subtree count only if object-movement profiling shows the walk itself matters. |
| Coalesce room dirty work | Batch duplicate GMCP room-dirty notifications during one game pulse. | Can reduce repeated output preparation in busy rooms. | Medium: output freshness/order and room observer behavior. | Explicitly outside #299; investigate separately with output metrics. |
| Async corpse persistence | Move corpse file/DB writes off the game thread. | Potentially removes synchronous I/O from death/loot paths. | Very high: lifecycle ordering, rollback, and crash recovery. | Do not pursue as a first optimization. |

The strongest low-risk sequence is: measure the current callback mix, finish a
validated mundane-event handle, reuse the existing NPC index in one audited
maintenance path, then consider the `remember_array` refactor. The distant
cadence is intentionally a configurable hypothesis rather than a promised
release percentage; the issue's rough 5–20% total-work range must be measured
against empty, sparse, busy, corpse-recovery, travel, and wake workloads.

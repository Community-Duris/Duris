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
   **Both maintained server backends and focused executable/sanitizer checks
   pass. A disposable sparse-world capture qualifies this implementation;
   broader workload and dynamic topology rollout evidence remains in #299.**

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

The existing small random jitter is retained. The feature is **disabled by
default**, including a missing property: occupied zones use 1× and playerless
zones use `PLAYERLESS_ZONE_SPEED_MODIFIER` (3×). Set
`world.activity.enabled=1.000` to opt in after local workload qualification.
Broad enablement remains an explicit follow-up in #299.

An NPC is forced back to the active cadence when it is fighting, pursuing or
remembering a target, controlled, mounted, casting/memorizing/scribing,
injured, patrolling, sentinel, trusted, or attached to an audited special or
quest procedure. This conservative eligibility boundary avoids delaying
timing-sensitive work merely because a zone is empty.

## Activity membership and wake behavior

- Zones of up to 64 rooms share one activity region. Larger zones are divided
  into connected regions of at most 64 rooms, with symmetric adjacency derived
  from actual exits (including one-way exits).
- Live PCs contribute a direct player reason to their region.
- Each reason also contributes a one-hop adjacent reason using actual room
  exits. A leaving reason leaves a configurable grace period (90 seconds by
  default), which prevents rapid movement and corpse transitions from
  repeatedly cold-starting a region. PC corpses protect their own region and
  neighboring regions at the normal cadence, so crossing the region boundary
  does not prematurely slow a predator during corpse recovery.
- NPCs are indexed by region as they enter and leave rooms. A promotion only
  inspects scheduled mundane events for NPCs in affected regions; it does not
  scan `character_list` on every wake.
- A promotion advances delayed mundane events into a bounded window of at most
  one normal ambient interval, with a per-NPC spread. Encounter-room events are
  advanced to the next eligible pulse, including nearby-to-active promotions.
  It does not replay
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
| `world.activity.enabled` | `0` | Opt-in feature switch; `0` preserves/restores the legacy cadence |
| `world.activity.distant.seconds` | `60` | Distant delay, bounded to at least one normal interval and at most one hour |
| `world.activity.grace.seconds` | `90` | Nearby grace after the last reason leaves, bounded to one hour |
| `world.activity.nearby.multiplier` | `3` | Nearby delay multiplier, bounded to 1–8 |
| `world.activity.wake.enabled` | `1` | Enables bounded promotion wakeups |
| `world.activity.diagnostics` | `0` | Logs aggregate membership and wake counters on reload/rebuild |

Disabling the feature advances already delayed indexed mundane events into the
normal window before clearing the index. Re-enabling performs a bounded
rebuild before the cold cadence is used again.

## Verification

Qualification uses the provisioned local Linux build/test containers:

- `make -C src -j8` for both MariaDB and flatfile backends (GCC 13.3).
- `test_world_activity_runtime.py` compiles the production policy and actual
  scheduler under ASan/UBSan. It covers bounded regions, one-way adjacency,
  next-pulse wake, independent PC corpses and halo, NPC corpse exclusion,
  nested/carried/worn placement, malformed graphs, cancellation, sequence and
  owner mismatch, repeated rebuild, reload bounds and disabled defaults.
  Four successive dispatched callbacks prove that scheduling a successor does
  not accidentally reschedule an event the scheduler is about to destroy.
- `test_nevent_scheduler_runtime.py` and `test_nevent_cancellation_runtime.py`
  exercise timing, pool reuse, cancellation and budgets under sanitizers.
- `test_npc_alchemist_runtime.py` preserves the approximately one-third caster
  cadence across 60 matched cases and the independent once-per-spawn vial rule.
- Focused equipment, publication retention, fresh-corpse adoption, coin custody,
  world recovery, zone purge, flatfile corpse restoration, item prompt,
  terminal extraction and training dummy checks.
- A disposable actual-server combat journey covers final-player death, corpse
  loot/removal and cold reload with the feature explicitly enabled.
- `run_world_activity_journey.py <flatfile-binary> <output.json>` verifies 1,000
  ordinary roaming NPCs in a 260-room corridor, compares disabled/enabled
  steady-state phases, and exercises live reload and copyover. It records
  process/game-thread CPU, command p95/p99, complete loop mean/max windows and
  complete mundane callback counts/cost/deferrals. Loop p95/p99 are not inferred
  from aggregate mean/max instrumentation.
- Authoritative clang-format and `git diff --check`. The WSL changed-line
  wrapper's NTFS temporary-checkout mode artifact has no textual difference.

This is local synthetic evidence, not a production performance guarantee.
Before enabling broadly, #299 retains matched full-world empty/sparse/busy,
corpse, travel, mass-wake and recovery captures, loop quantile instrumentation,
deterministic predator movement/sentinel controls, and dynamic exit/portal/
transport proximity qualification. Region topology is reconstructed at boot
and recovery; live topology mutation needs qualification before broad rollout.
No production configuration or persistence authority is changed.

## Consolidated follow-up performance fixes

The same implementation branch also carries two bounded follow-ups discovered
while tracing the wake path:

- each character records the ordinary mundane event pointer together with its
  NEVENT sequence, so an activity wake validates one handle instead of walking
  the character's complete event list; rebuild scans the list once after boot
  or recovery to repopulate the runtime-only cache;
- PC-corpse subtree publication uses a fixed-stack tree walk rather than
  allocating a temporary `std::unordered_set` for every object transfer. The
  containment API already rejects cycles. Parent validation, sibling-cycle
  checks, a 1,024-frame limit and 65,536-node budget fence malformed recovery
  graphs without unbounded recursion or allocation.

Neither change alters event cadence, corpse ownership, extraction, or combat
semantics. The handle is cleared before scheduler teardown, validated by owner,
callback and sequence, and replaced with a new successor during dispatch. The
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

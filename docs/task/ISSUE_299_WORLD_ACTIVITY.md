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
   pass. Live topology, controlled-body presence, deterministic wandering,
   recovery wakes and exact loop quantiles are implemented. Matched full-world
   workload captures are recorded below. The owner authorized default
   enablement after qualification and PR review; explicit disable remains
   available for rollback.**

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

The existing small random jitter is retained. The feature is **enabled by
default**, including when an older property file omits the key. Set
`world.activity.enabled=0.000` to restore the legacy cadence: occupied zones
use 1× and playerless zones use `PLAYERLESS_ZONE_SPEED_MODIFIER` (3×).
An existing explicit `0` remains disabled until the operator changes it.

Fighting, pursuing, remembering a target, controlled, mounted,
casting/memorizing/scribing, injured, patrolling, sentinel, trusted, and
special/quest/room-procedure NPCs are exempt from cold throttling. They retain
their legacy ordinary cadence outside protected regions and use the normal
cadence inside active regions. This also avoids speeding up remote scripted
NPC work as an unintended effect of enabling the throttle. Combat, casting,
preparation, regeneration, expiry, repop and accounting event clocks are
unchanged. Quick retries retain `PULSE_VIOLENCE`.

## Activity membership and wake behavior

- Zones of up to 64 rooms share one activity region. Larger zones are divided
  into connected regions of at most 64 rooms, with symmetric adjacency derived
  from actual exits (including one-way exits).
- Live PCs, including linkdead bodies, contribute a direct player reason.
  Player-owned pets, ridden mounts, descriptor-controlled NPCs and morphs with
  an original PC contribute independent reasons in their actual rooms. Pet and
  riding link publication/removal and staff switch/return refresh these reasons.
- Each reason also contributes a one-hop adjacent reason using actual room
  exits, supported portals/teleporters, ship hull/interior connections and ferry
  hull/interior connections. A leaving reason leaves a configurable grace period (90 seconds by
  default), which prevents rapid movement and corpse transitions from
  repeatedly cold-starting a region. PC corpses protect their own region and
  neighboring regions at the normal cadence, so crossing the region boundary
  does not prematurely slow a predator during corpse recovery.
- NPCs are indexed by region as they enter and leave rooms. A promotion only
  inspects scheduled mundane events for NPCs in affected regions; it does not
  scan `character_list` on every wake.
- Live exit publishers update the six cached destinations for the changed room.
  Physical region connections are counted: deleting one of several exits or
  transports does not remove their shared halo. Region partitions stay stable
  until the next rebuild; live changes update adjacency and reasons directly.
  Random-zone teleporters conservatively protect every possible destination
  region. Moving ships/ferries update their exterior/interior adjacency through
  committed object placement. No ordinary move scans the full world.
- A promotion advances delayed mundane events into a bounded window of at most
  one normal ambient interval, with a per-NPC spread. Encounter-room events are
  advanced to the next eligible pulse, including nearby-to-active promotions.
  It does not replay
  missed work or fire an entire region at once.
- Same-pulse leave/enter transitions preserve the existing region wake window;
  they still wake the arrival room next pulse. Moving within one region does not
  repeatedly advance every NPC's ordinary callback.
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
reconstructs the NPC and activity indexes after copyover/Redis recovery. It then
advances restored player/corpse encounter rooms to the next eligible pulse and
spreads the remaining affected region work through the normal interval.

Corpse reasons affect only NPC scheduling. They do not alter corpse timers,
loot/resurrection behavior, persistence, or extraction semantics.

## Runtime settings

The settings are read and reloaded through the existing property path:

| Property | Default | Bounds/meaning |
| --- | ---: | --- |
| `world.activity.enabled` | `1` | Enabled when absent; explicit `0` preserves/restores the legacy cadence |
| `world.activity.distant.seconds` | `60` | Distant delay, bounded to at least one normal interval and at most one hour |
| `world.activity.grace.seconds` | `90` | Nearby grace after the last reason leaves, bounded to one hour |
| `world.activity.nearby.multiplier` | `3` | Nearby delay multiplier, bounded to 1–8 |
| `world.activity.wake.enabled` | `1` | Required for cold throttling; `0` restores the legacy policy even if the feature switch is set |
| `world.activity.diagnostics` | `0` | Logs aggregate membership, wakes, stale handles, handle repairs and topology changes on reload/rebuild and every 300 pulses |

Disabling the feature advances already delayed indexed mundane events into the
normal window before clearing the index. Re-enabling performs a bounded
rebuild before the cold cadence is used again.

## Verification

Qualification uses the provisioned local Linux build/test containers:

- `make -C src -j4` for MariaDB and
  `make -C src -j4 PERSISTENCE_BACKEND=flatfile DMS_BINARY=/work/bin/server/dms_flatfile`
  for flatfile (GCC 13.3, maintained development profile).
- `test_world_activity_runtime.py` compiles the production policy and actual
  scheduler under ASan/UBSan. It covers bounded regions, one-way adjacency,
  next-pulse wake, independent PC corpses and halo, NPC corpse exclusion,
  nested/carried/worn placement, malformed graphs, cancellation, sequence and
  owner mismatch, repeated rebuild, reload bounds, missing-key enablement and
  explicit-disable rollback.
  It also executes the production legal-wandering block with injected direction
  choices: a predator crosses a corpse-protected region edge after the last
  player leaves, a sentinel stays put, and stay-zone, closed-door, no-mob and
  terrain constraints remain enforced. Native link getters qualify independent
  pet/mount/morph/switched-body presence and movement. Live exit retarget/removal,
  duplicate connections, portals, random-zone teleporters, ships and ferries are
  tested without rebuilding the regions.
  Four successive dispatched callbacks prove that scheduling a successor does
  not accidentally reschedule an event the scheduler is about to destroy.
- `test_nevent_scheduler_runtime.py` and `test_nevent_cancellation_runtime.py`
  exercise timing, pool reuse, cancellation and budgets under sanitizers.
  Nested activity-wake batches retain the same due ticks, sequence ordering and
  player priority; cancellation and replacement before the batch flush remain
  safe. A batch opened during dispatch waits for the existing dispatch boundary.
  Remaining-time queries and relative advances observe queued deadlines before
  the physical bucket update. A production-policy regression reproduces an
  encounter wake followed by a region promotion in the same callback and proves
  that the encounter still runs on the next pulse.
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
  complete mundane callback counts/cost/deferrals. `--full-world --scenario`
  selects empty, sparse, busy, corpse, travel, mass-wake or recovery fixtures.
  The full-world fixture retains tracked world data, reset behavior and scripts.
  The busy fixture keeps 20 real NPC combat pairs engaged, with native staff
  inspection before/after each capture and fresh combat after recovery. The runner builds and
  binds the verified native inspector before use. `--runtime-index` additionally
  runs the native full index/list and scheduler check at boot, after copyover,
  after staff NPC creation and after extraction.
- Loop p95/p99 come from the exact `total_tick` samples in each complete window.
  Fixed storage holds 512 samples; the normal window has 300. An overflowing or
  empty sample set reports unavailable quantiles, rather than a partial estimate.
  Tests verify nearest-rank quantiles, reset, overflow and concurrent recording.
  Complete NEVENT windows also record budget exhaustion, deferrals, lateness,
  allocation failures and callback cost. Optional activity diagnostics are
  outside the existing `total_tick` timing boundary and remain in process CPU.
- The authoritative `./scripts/format.sh --rev HEAD --check` passes in a native
  Linux formatting checkout containing the PR's touched C/C++ files and their
  `experimental-accounting` base (`1cf5c08b6`). Both changed lines and complete
  touched files are checked. `git diff --check` in the managed worktree passes.

The focused production-policy commands are:

```sh
python3 tests/async/test_world_activity_runtime.py
python3 tests/async/test_world_activity_contract.py
python3 tests/async/test_nevent_scheduler_runtime.py
python3 tests/async/test_nevent_cancellation_runtime.py
python3 tests/async/test_latency_trace_runtime.py
python3 tests/async/test_npc_alchemist_runtime.py
python3 tests/async/test_spell_ward_durability.py
python3 tests/async/test_issue_551_flatfile_craft.py
```

The actual-server combat and recipe journeys also pass with
`world.activity.enabled=1` in their disposable fixture. They cover final-player
death, both corpse publications, loot and cold restoration, plus mortal
Craft/Forge material/tool retirement, fresh output identities, XP, retained
pouch counters, copyover and two cold restarts. The active-accounting Alchemy
journey that requires injected fixture gates was not run on the maintained
server binary; the existing native #551 conservation fixture and NPC alchemist
cadence/once-per-spawn issuance regression passed. MariaDB was compile-qualified;
these live journeys used private flatfile state, without a live SQL deployment.

The default-enablement review also reproduced a wake-ordering bug: an encounter
wake queued during a callback could be overwritten by a subsequent region wake
that read the old physical deadline. Remaining-time queries and relative
advances now use the queued deadline. The regression first failed against the
previous code, then passed under ASan/UBSan and verified the actual next-pulse
callback. Both backend builds, scheduler/cancellation sanitizers, loop quantiles,
alchemist cadence and spell-ward durability checks pass after this correction.
The full-world recovery smoke journey now starts from the shipped enabled
setting, verifies explicit disable/enable across copyover, and checks the native
indexes after boot, reconstruction, staff creation and extraction.

A completed CI SQL-authority fixture reported a MySQL lock-cleanup assertion.
The SQL implementation and that harness are unchanged by this PR. Its five
runtime-authority scenarios pass locally against disposable MySQL 8.0.46 with
the existing ASan/UBSan harness, including the reported wrong-binding cleanup.
This check is separate from the flatfile gameplay journeys and does not qualify
a live SQL deployment. The seven workload measurements below retain their
recorded source/binary identity; this review did not repeat that full matrix.

This is local synthetic evidence, not a production performance guarantee.
No live server configuration or persistence authority is changed. Gameplay and
accounting correctness are acceptance conditions; a CPU reduction does not
justify changing a combat, corpse, crafting or issuance rule.

Full-world copyover has an existing recovery-writer limitation: cosmetic blood
in virtual room zero cannot be snapshotted because that writer requires a
positive room vnum. Both the retained pre-qualification base (`33d23aeec`) and
the candidate failed on that same object. Before each full-world copyover, this
private fixture uses the existing staff extraction command to remove only that
room's cosmetic blood, then returns the player to the workload arena. No
production serializer, corpse custody fence or recovery authority is bypassed
or changed. Successful full-world recovery captures qualify activity-index
reconstruction for the resulting valid scene; they do not claim to repair that
pre-existing copyover failure. The runner additionally waits for the successor
to enter the game loop after descriptor restoration before checking or measuring
the rebuilt index.

The busy fixture also exposed existing NPC-only combat restoration behavior:
`copyover_restore_combat()` restores links through playing descriptors and does
not relink independent NPC-versus-NPC pairs. All 20 pairs remained engaged
through both measured phases. Post-copyover qualification starts fresh combat
through ordinary commands and verifies the pairs again, together with the native
index/scheduler checks. No combat restoration or persistence format is changed
by #299.

### Full-world qualification (2026-10-03)

The final matrix uses source `e9763017f` integrated with
`experimental-accounting` at `3dbb8bc83`, GCC 13.3 and the maintained development
profile. The flatfile binary's SHA-256 is
`2921ecc8cd4198961a829a4a1a04aacb3c5078c4b06015a34b28b0900391ef24`.
The target's later payload-repair update (`1cf5c08b6`, PR #705) is integrated
after the matrix; activity policy, scheduler and loop instrumentation are
unchanged by that update. Both maintained backends build again, all 21 focused
restitution CLI tests pass, and formatting is checked against that new target.
A full-world recovery smoke run on the integrated binary also passes: both
persisted modes survive pre-phase copyover, the 1,000-NPC workload survives final
reconstruction, and native index/scheduler checks pass at boot, each copyover,
staff NPC creation and extraction.
Every scenario runs sequentially on this same binary with the feature disabled,
then enabled, in a fresh disposable full-world fixture. Each phase has an
80-second warm-up and at least 160 measured seconds. World data, ordinary NPC
wandering, scripts, resets and transport behavior remain active. The runner
verifies exactly 1,000 added workload NPCs before capture and after copyover;
the full resident NPC census is much larger.

Empty has no resident player during measurement. Sparse has one staff player
far from the added NPCs. Busy adds 20 genuine NPC combat pairs and checks their
opponents before and after each capture. Corpse adds 100 PC-classified corpse
objects for load measurement; the separate real-death journey qualifies the
authoritative publication and recovery paths. Travel alternates between distant
arena rooms at each command sample; mass wake alternates in bursts every 24
samples. Recovery performs copyover before each measured phase as well as the
final reconstruction check.

The runner persists the selected feature value only in its private fixture and
verifies it again after pre-capture copyover. Every measured activity-diagnostic
window must match the reported enabled/disabled mode. An initial recovery
capture was discarded after diagnostics showed that copyover had restored the
fixture's startup enabled value during the nominally disabled phase. The final
recovery comparison repeats both phases with the corrected startup property;
no production property behavior is changed.

CPU seconds come from the server process and its main game thread, with 10 ms
resolution. Callback rates/costs normalize complete NEVENT windows. Loop mean
is sample-weighted; reported p95/p99 are the worst exact quantiles among complete
300-sample windows, rather than percentiles pooled across windows. Staff echo
round trips measure command responsiveness. Shared host load, normal world
evolution and the single off/on pair per scenario limit causal precision;
these are local measurements, not promised production savings or group-combat
latency guarantees. No material RAM reduction is expected.

The first rapid-travel capture found a real regression before wake batching:
enabled game-thread CPU rate rose approximately 34%, with loop p95/p99 of
70.01/96.38 ms versus 22.05/26.43 ms disabled. Repeated early-wake insertions
walked the same large sorted event bucket separately for every NPC. The final
batching fix preserves the selected deadlines, player priority and sequence
ordering while merging each destination bucket once. The final matrix repeats
all seven workloads with this correction; the initial regression is retained
here as part of the qualification evidence.

All seven final workloads pass. Each retains exactly 1,000 workload NPCs after
copyover and passes the native index/list and scheduler checks at boot,
reconstruction, staff NPC creation and extraction. The enabled resident NPC
census ranges from 56,789 to 56,968 as ordinary world resets continue. Across
15 complete loop windows (4,500 samples), both modes report zero callback
allocation failures and zero callbacks four or more pulses late. Enabled
diagnostic windows report zero stale wake handles and zero handle repairs;
live topology counters advance throughout the captures. All 20 combat pairs
remain engaged through both busy captures, and fresh combat succeeds after
copyover. All 100 corpse reasons remain present throughout the corpse capture.

The pairs below are disabled/enabled. CPU reduction normalizes measured wall
time; callback rates normalize their own complete windows. Busy enabled has two
complete windows; every other phase has one. Quantile cells show disabled
p95/p99 followed by enabled p95/p99, in milliseconds.

| Scenario | Game CPU seconds, off/on | CPU rate reduction | Mundane calls/s, off/on | Loop mean ms, off/on | Loop p95/p99 ms, off; on | Command p99 ms, off/on |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Empty | 6.21/4.79 | 22.9% | 2291.7/1806.9 | 9.72/8.06 | 18.97/22.23; 15.09/21.88 | —/— |
| Sparse | 7.15/5.39 | 24.6% | 2397.6/1817.1 | 11.02/8.57 | 19.46/22.12; 14.31/19.16 | 191.51/174.21 |
| Busy, 20 NPC combat pairs | 8.61/6.56 | 23.8% | 2683.0/1705.1 | 14.07/10.28 | 28.00/34.33; 17.29/24.88 | 231.59/234.58 |
| Corpse, 100 PC-classified objects | 7.54/5.85 | 22.4% | 2382.2/1915.7 | 11.65/9.52 | 22.47/26.20; 15.75/18.52 | 210.45/190.54 |
| Rapid travel | 8.03/6.81 | 15.3% | 2383.1/1988.4 | 12.51/11.20 | 24.23/30.55; 19.72/27.71 | 241.85/237.64 |
| Mass wake | 7.42/5.75 | 22.1% | 2380.1/1889.0 | 11.45/9.72 | 21.15/25.22; 18.17/23.55 | 235.03/233.82 |
| Recovery | 8.16/4.77 | 41.5% | 2370.1/1463.4 | 12.30/7.54 | 26.94/30.72; 15.75/23.65 | 211.56/173.65 |

| Scenario | Mundane cost ms/s, off/on | Mundane deferrals/s, off/on | Budget-exhausted pulses/300, off/on | Lateness ≥4 pulses, off/on | Allocation failures, off/on |
| --- | ---: | ---: | ---: | ---: | ---: |
| Empty | 9.89/8.66 | 21.48/55.46 | 230/195 | 0/0 | 0/0 |
| Sparse | 12.02/7.85 | 9.87/19.10 | 239/203 | 0/0 | 0/0 |
| Busy | 16.10/12.00 | 77.87/22.33 | 228/198.5 | 0/0 | 0/0 |
| Corpse | 12.61/10.51 | 28.80/31.05 | 237/216 | 0/0 | 0/0 |
| Travel | 13.97/12.78 | 49.31/42.15 | 230/223 | 0/0 | 0/0 |
| Mass wake | 12.01/10.48 | 29.82/28.74 | 239/206 | 0/0 | 0/0 |
| Recovery | 12.35/6.62 | 124.49/262.49 | 227/87 | 0/0 | 0/0 |

| Scenario | Whole-process CPU seconds, off/on | Largest loop sample ms, off/on |
| --- | ---: | ---: |
| Empty | 6.30/4.87 | 24.120/35.091 |
| Sparse | 7.25/5.47 | 31.731/35.993 |
| Busy | 8.72/6.63 | 40.867/54.641 |
| Corpse | 7.64/5.92 | 39.590/43.686 |
| Travel | 8.14/6.91 | 34.141/34.083 |
| Mass wake | 7.52/5.84 | 34.208/33.362 |
| Recovery | 8.26/4.86 | 34.335/32.095 |

Every scenario reduces measured CPU rate, loop mean and loop p95/p99, but not
every metric improves. The busy command p99 increases 2.99 ms. Largest loop
samples increase in empty, sparse, busy and corpse captures. Mundane deferrals
increase in empty, sparse, corpse and recovery even though overall budget
exhaustion falls; neither mode has a callback at least four pulses late in the
captured windows. Recovery copyover availability is 4.69 seconds disabled and
5.24 seconds enabled, a 0.55-second increase in this one pair. These costs and
single-run limits remain visible; the measurements do not justify changing
gameplay deadlines or promising improvement in every latency sample.

### Earlier sparse capture (2026-10-01)

One sequential disabled/enabled run used 1,000 ordinary roaming NPCs far from
one staff player within a single large zone. Each phase had 80 seconds of
warm-up followed by approximately 180 measured seconds and 144 `look` requests.
The legacy occupied-zone predicate activates the whole zone; the candidate
keeps distant regions cold. This is a sparse large-zone comparison, rather
than an empty-zone benchmark of the 22.5-to-60-second formula.

| Recorded measure | Disabled | Enabled |
| --- | ---: | ---: |
| Process CPU seconds | 0.95 | 0.22 |
| Game-thread CPU seconds | 0.93 | 0.20 |
| Complete-window loop mean (microseconds) | 1,177 | 289.5 |
| Complete-window loop maximum (microseconds) | 2,701 | 3,856 |
| Command round-trip p95 (milliseconds) | 170.069 | 170.048 |
| Command round-trip p99 (milliseconds) | 170.290 | 171.550 |
| Mundane callbacks per second | 134.074 | 20.341 |
| Mundane callback deferrals | 0 | 0 |

Callback rate fell about 84.8%, and game-thread CPU fell about 78.5% in this
synthetic run. The largest recorded loop sample increased, and command p99
increased about 1.26 ms; do not describe every latency metric as improved.
Baseline had one complete 74.75-second callback window, candidate two totaling
149.5 seconds; rates normalize the different window lengths. Loop means/maxima
use one baseline and two candidate complete instrumentation windows. CPU
resolution is 10 ms, host load is shared, and this is a single run. These data
do not establish production savings or replace the full-world matrix above.
The final copyover plus repeated disable/enable retained 1,003 indexed NPCs
and one player; no callback debt replay or vial issuance was added.

## Consolidated follow-up performance fixes

The same implementation branch also carries three bounded follow-ups discovered
while tracing the wake path:

- each character records the ordinary mundane event pointer together with its
  NEVENT sequence, so an activity wake validates one handle instead of walking
  the character's complete event list; rebuild scans the list once after boot
  or recovery to repopulate the runtime-only cache;
- activity wakes batch scheduler reschedules by destination bucket, sort them
  using the existing due-tick/priority/sequence comparator, and merge each
  bucket once. Nested batches flush at the outer boundary; batches during an
  event callback retain the existing end-of-dispatch flush. This removes
  repeated walks through the same large sorted bucket without changing the
  selected wake deadlines or callback ordering. Allocation failure retains the
  individual reschedule path before any batch mutation;
- PC-corpse subtree publication uses a fixed-stack tree walk rather than
  allocating a temporary `std::unordered_set` for every object transfer. The
  containment API already rejects cycles. Parent validation, sibling-cycle
  checks, a 1,024-frame limit and 65,536-node budget fence malformed recovery
  graphs without unbounded recursion or allocation.

These changes preserve event cadence, corpse ownership, extraction, and combat
semantics. The handle is cleared before scheduler teardown, validated by owner,
callback and sequence, and replaced with a new successor during dispatch. The
tree walk's guard preserves a bounded failure mode for corrupt object graphs.
Missing or stale handles take an owner-list fallback capped at 64 entries; valid
handles keep the direct fast path. A found ordinary callback repairs the cache.
If the scan cannot rule out another pending callback, external scheduling refuses
to introduce a duplicate. The dispatched callback can still create its successor.
Diagnostics count repairs and incomplete/stale wake lookups; rebuilding recovers
the cache from the actual owner lists without adding persistence fields.

The independent release-style traversal fixture (`test_world_activity_runtime.py
--benchmark`, `-O2`, no sanitizers) transfers a 129-node container containing one
PC corpse 20,000 times without NPC wake cost. The first recorded result was
452 ns per transfer; the final policy and wake-batch code measured 461 ns per
transfer on 2026-10-03. Each transfer uses fixed traversal storage; unused frames
are no longer cleared. No maintained subtree-presence cache was added.

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

The remaining performance sequence is: measure the current callback mix,
reuse the existing NPC index in one audited
maintenance path, then consider the `remember_array` refactor. The distant
cadence is intentionally a configurable hypothesis rather than a promised
release percentage; the issue's rough 5–20% total-work range must be measured
against empty, sparse, busy, corpse-recovery, travel, and wake workloads.

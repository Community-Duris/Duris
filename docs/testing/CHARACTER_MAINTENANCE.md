# Scheduled character maintenance

The former `generic_char_event()` ran every five seconds and traversed
`character_list` on every invocation. Address hashing selected one quarter of
the population for its twenty-second maintenance body, but did not reduce
discovery. The NPC structural check also ran before slice selection.

`world/character_maintenance.c` replaces that periodic sweep with character
owners on the existing timing wheel. Only boot reconciliation traverses the
population. There is no second active-character registry or replacement event
framework.

## Obligations and cadence

| Character state | Required work | Cadence |
| --- | --- | --- |
| Every live NPC, including idle NPCs in empty zones | Detect a missing `only.npc` structure and extract the malformed NPC | Five seconds |
| Every live PC | Enforce taught/learned skill caps; retain sunlight checks and all other applicable body work | Twenty seconds |
| NPC below maximum hit points or ward | Restart existing resource regeneration | Twenty seconds |
| NPC with nonzero flight height or swimming state | Repair invalid room/posture combinations | Twenty seconds |
| Poisoned druid NPC above level 30 | Run the existing poison neutralization operation | Twenty seconds |
| NPC with pleasantry | Run the existing pleasantry operation | Twenty seconds |
| NPC with a burning light in the slots maintained by `update_char_objects()` | Consume fuel and update character/room lighting | Twenty seconds |
| Healthy idle NPC without these conditions | Structural check only; skip the maintenance body | Five seconds |

The mandatory five-second NPC check prevents dropping idle NPCs entirely.
Sunlight damage itself returns immediately for NPCs. The predicate follows the
three equipment slots that the existing light maintenance actually visits.
PC skill maintenance remains unconditional.

Runtime identity modulo 20 spreads NPC checks across five seconds; modulo 80
spreads body deadlines across twenty seconds (four pulses per second). On boot,
initial bodies span 20–39.75 seconds, replacing the old four waves at
20/25/30/35 seconds. Subsequent bodies remain twenty seconds apart. New admissions
join the next eligible phase. A delayed body starts its next period from actual
execution, so scheduler overload cannot cause compressed fuel/skill/body work.
State notifications preserve a pending deadline, including work already due or
deferred by the scheduler budget.

## Ownership and state changes

NPC creation registers only after initialization and excludes mobile probes.
Committed room entry registers PCs and covers movement/morph admission. Boot
initialization reconciles characters restored before the event system is ready.
Repeated entry is idempotent. `update_pos()` refreshes the owner after synchronous
status changes; death disarms maintenance, and revival can rearm it. The
outpost death/reset path also rearms after bulk event cancellation and its
committed return to a live position, before its existing synchronous death proc.
Staff restoration, outpost restoration, and training-dummy death recovery notify
after their direct status restoration as well.
The due predicate re-reads legacy direct resource, affect, posture, and equipment writes,
so these transitions cannot become stranded waiting for a notification.

Extraction unregisters after validation and the morph early return, before
cleanup. Direct free also unregisters before affect teardown. The scheduler's
existing character-owner links cancel callbacks synchronously. The cached
pointer/sequence handle is checked against its captured owner runtime identity;
detach clears the matching cache without disturbing a successor. Extraction
during body work cannot rearm the old owner. The callback performs no runtime-ID
lookup or population scan.

The only new state notification has a concrete maintenance consumer. Validation,
movement vetoes, and other synchronous operations keep their current order.
NPC hunts, scripts, mundane activity, room processes, and independent world
events continue on their existing schedules, including in zones with no PCs.
Removing maintenance membership cancels only that maintenance handle.

## Matched synthetic measurement

Five independent runs per mode used 30,000 NPCs and 120 PCs over 200 simulated
seconds, after owner phases warmed up. Only 120 NPCs had finite light fuel; PCs
required skill caps. Both complete implementations consumed ten fuel units and
produced the same skill caps. The executable compiles the production timing
wheel, maintenance body, and light work. Callback peak measurements use the
existing scheduler analytics and the real monotonic clock; outer measurements
include dispatch bookkeeping. Each table cell is the median of five runs.

| Measurement | Legacy sweep | Character owners |
| --- | ---: | ---: |
| Repeated `character_list` visits | 1,204,800 | 0 |
| Maintenance bodies executed | 301,200 | 2,400 |
| Maintenance callbacks | 40 | 1,201,200 |
| Total dispatch time | 30,454 µs | 238,548 µs |
| Peak callback time | 1,675 µs | 587 µs |
| Peak event-pulse time | 1,681 µs | 970 µs |

Population discovery is eliminated; body executions fall by 99.20%.
Median peak callback time falls by 64.96% and median peak event-pulse
time by 42.30%. Across the five runs, peak pulse time was
1,369–2,151 µs before and 712–4,859 µs after.
The maximum observed instrumented pulse was higher after the change; the
median improvement does not establish a lower maximum for every run.

An additional discovery-only control retains the old population walk, NPC
sanity check, and phase selection while skipping the body. It still makes
1,204,800 visits and takes a median 22,115 µs total, with a
1,855 µs peak callback and 1,880 µs peak pulse. That control's total is
about 73% of the legacy synthetic dispatch total, even when almost every NPC
has no body work.

**The tradeoff is higher total scheduler CPU and timer storage.** The peak
measurements above enable callback analytics on both variants; their instrumented
dispatch time rises about 7.83-fold. A separate control disables analytics, as in
the scheduler's default configuration, and measures process CPU for the repeated
dispatch window, including the harness loop/tick bookkeeping: 28,887 µs before
versus 232,964 µs after (medians of five runs). Retaining the five-second safety
obligation as individually budgetable callbacks raises that CPU measurement
8.06-fold, an increase of 0.204 seconds over 200 simulated seconds, about
0.102 percentage points of one CPU core. That control's median peak event-pulse
time is 1,351 µs before and 696 µs after; callback peaks are unavailable
without analytics and are printed as `-1`. Unprofiled peak-pulse ranges are
1,339–1,476 µs before and 559–1,350 µs after.
There is one pending maintenance timer per live character, plus runtime-only
owner fields in `char_data`. This change targets bounded peak work and discovery,
and does not demonstrate a whole-world CPU reduction.

This fixture uses heap event allocation and stubs external game operations such
as poison/pleasantry/regeneration; production uses its existing event pool and
real collaborators. The before control excludes periodic-registry rearm cost.
No production population, complete server CPU comparison, or production p99
latency claim follows from these numbers. Measure the intended population and
timer capacity before promoting the draft PR.

To reproduce on Linux/WSL from the repository root:

```sh
mkdir -p bin/tests/character-maintenance
git show 2a647b6ec7b21fba79f58b7af8fbc1cf90ec78da:src/world/handler.c \
  > bin/tests/character-maintenance/legacy-handler.c
python3 tests/async/test_character_maintenance_runtime.py --benchmark \
  --baseline bin/tests/character-maintenance/legacy-handler.c --samples 5
# Aggregate CPU/control run without per-callback profiling:
python3 tests/async/test_character_maintenance_runtime.py --benchmark \
  --baseline bin/tests/character-maintenance/legacy-handler.c --samples 5 --no-analytics
```

The test accepts the old full handler file or an extracted sweep. It instruments
that source at runtime rather than maintaining a duplicate legacy implementation.

## Executable verification

`python3 tests/async/test_character_maintenance_runtime.py` runs the production
owner agenda and scheduler under ASan/UBSan. It checks cadence, fuel, skill caps,
condition-specific operations, direct state changes, PC/NPC role changes,
death/revival, repeated admission, extraction during a callback, pending
extraction, cancellation, stale identities, reused storage, and malformed NPCs.
It executes the outpost death/reset helper to check rearming after bulk
cancellation and preservation of synchronous death-proc behavior, and the
training-dummy recovery branch. Extraction during poison removal or pleasantry
cannot send to or rearm the retired owner.
It also executes production NPC script recurrence and independent world work
with zero players.

Mass activation admits 4,000 lit NPCs, verifies twenty admission phases, and
extracts every seventh NPC while events are pending. The unbudgeted peak is
172 callbacks. A deterministic 450 µs time budget plus 200-callback limit peaks
at 150 callbacks; a separate 40-callback limit peaks at 40. Both budgeted modes
defer work, preserve pending deadlines under repeated notifications, and keep
successive bodies at least eighty pulses apart. Every surviving NPC performs
body work within the test window, removed NPCs retain their fuel, and a competing
production NPC script continues to make progress under each budget.

Relevant companion checks are the scheduler, cancellation, periodic rearm,
typed payload/hunt identity, world activity, regeneration, movement lifetime,
falling-skills executable tests, and maintenance/budget source contracts.
Build with `make -C src`; touched C/C++ uses `scripts/format.sh` and its check.
The disposable `run_world_activity_journey.py ... --smoke --runtime-index` exercises normal
wandering, live activity policy reload, and copyover with 1,000 autonomous NPCs
remote from the player. It verifies runtime-index/list agreement after boot,
copyover, and NPC creation/purge. Its generated accounts, state, and world never touch the
configured database or local player files.

The recorded run is based on runtime-ID index commit `2ca8826f5`, with the
legacy benchmark captured from `2a647b6ec`. Validation uses WSL Ubuntu 22.04 and
G++ 12 with the maintained warning/hardening profile. The host's Hiredis TLS
library was unavailable at relink, so Hiredis 1.2.0 was built under ignored
`bin/tests/character-maintenance/dependencies/` using its documented
[TLS build option](https://github.com/redis/hiredis/blob/v1.2.0/README.md#ssltls-support).
Both server backends were force-rebuilt against that local header/library prefix;
system dependency paths were not modified. Final relinking used GNU ld's
`--no-keep-memory` and `--reduce-memory-overheads` after the shared WSL host ran
out of memory during a default link.

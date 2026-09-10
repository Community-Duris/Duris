# Movement scan hotfix: shortest implementation plan

Date: 2026-09-10. Status: **implemented, pushed to master, clean-built, and
deployed to the configured local MUD; live movement/casting smoke passed**.

## Outcome and scope

Reduce CPU work in NPC wandering and patrol callbacks by avoiding the
unconditional global character-list scan after `do_simple_move()` in `do_move()`.
Preserve the existing protection against destination scripts deleting the mover,
as well as death checks, frost effects, and failed-movement tracking cleanup.

This is a small temporary optimization. It does not change NEVENT buckets,
budgets, event counts, NPC cadence, or gameplay rules. It does not repair the
separate [casting input queue defect](spellcasting-queue.md), and is not a
promise to resolve every source of current lag.

The [live profile](2026-09-10-live-casting-latency-findings.md#follow-up-measured-optimization-candidates-at-1104-utc)
put 61.7% of callback time inside mundane wandering, with patrol callbacks
accounting for another 7.2%. Neither figure measures this predicate alone or
predicts the percentage saved.

## Selected implementation: removal counter with existing fallback

Use one process-local unsigned 64-bit character-removal counter. Snapshot it
immediately before `do_simple_move()`. After movement, run `char_in_list(ch)`
only if the counter changed. Always retain the subsequent `IS_ALIVE(ch)` check,
with short-circuit ordering that prevents dereferencing an absent mover.

Conceptually, the existing post-move guard becomes:

```cpp
const auto removal_before = character_removal_generation;
const bool moved = do_simple_move(ch, cmd, MVFLG_DRAG_FOLLOWERS);
if ((removal_before != character_removal_generation && !char_in_list(ch)) ||
    !IS_ALIVE(ch))
    return;
```

Increment the counter immediately after the null guard in both `extract_char()`
and `free_char()`, before either can invoke nested work or release storage.
Conservative increments on extraction that later returns, or twice for an
extraction followed by free, are harmless: they merely retain the old scan.
The counter is shared by the single game-state thread, needs no allocation,
registration table, persistence, configuration, or atomic synchronization.
Use an unsigned 64-bit type; equality assumes fewer than 2^64 invalidations
during one synchronous movement call.

An unchanged counter means the caller's already-live mover has not been
removed through these paths. Any removal, including an unrelated character or
nested follower movement, restores the original membership check. Do not simply
return whenever the counter changes: that would suppress valid post-move work
when a script extracts someone else.

This preserves the old pointer-membership semantics. It does not introduce
per-character identity tracking or claim to fix the existing address-reuse
limitation of `char_in_list()`.

## Files

| File | Necessary change |
| --- | --- |
| `src/world/handler.c` | Define counter and invalidate at entry to non-null extraction. |
| `src/world/db.c` | Invalidate at entry to non-null `free_char()`. |
| `src/core/prototypes.h` | Declare the internal counter using an available unsigned 64-bit type. |
| `src/cmd/actmove.c` | Snapshot counter and condition only the existing post-move membership scan. |
| `tests/async/test_kingdom_contract.py` | Extend movement safety contracts to check snapshot and guard ordering, plus invalidation hooks. |
| `tests/async/test_movement_liveness_runtime.py` | Add focused executable coverage for this currently uncovered movement/extraction boundary, using existing source-extraction harness conventions. |

The new runtime test is a focused test, not new test infrastructure. The existing
`test_move_cost_runtime.py` covers terrain arithmetic and is not a suitable home
for character-lifetime tests.

## Execution order

1. **Confirm the counter covers movement invalidation.** Audit the actual
   `do_simple_move()` call chain and global-list removal/free sites before
   editing. The inspected world-list unlink is in `extract_char()`; direct
   destruction enters `free_char()`. Confirm there is no movement-reachable
   unlink, list reset, or character deallocation bypassing both hooks. Boot
   initialization and character insertions do not need counter maintenance.
   If a bypass exists, cover that concrete path before allowing the shortcut;
   do not assume the two hooks are sufficient solely from their names.
2. **Make the four-file patch.** Keep `char_in_list()` itself unchanged. Leave
   the conditional room-procedure scan in `char_to_room()` and checks in
   `MobHuntCheck()` and kingdom code unchanged. Keep all existing post-movement
   branches and their order.
3. **Prove safety and eliminated work in the same focused harness.** Extract
   the production movement tail and actual membership predicate; stub the
   movement action to exercise lifecycle outcomes. Check invalidation hook
   placement against production source. Cover successful and blocked movement,
   PCs and NPCs, mover extraction, unrelated extraction, nested movement, and
   a mover becoming dead without extraction. Use ASan/UBSan for the freed-mover
   case. No post-move character access is allowed after failed membership.
   Verify frost and tracking cleanup still run in their original cases.
4. **Compare scans deterministically.** With a large synthetic character list
   and mover near its tail, ordinary and blocked movement must call the
   membership predicate zero times when the generation is unchanged. Changed
   generation must take the original scan and return the same result. Exercise
   address reuse to establish parity with the old guard; do not label address
   membership as proof of original-object identity. No elapsed-time threshold
   is needed for the regression test.
5. **Run the focused gates and build.** Fix failures within this patch, then
   prepare its diff and test results for deployment review. Do not turn this
   into a full burn-in or an unrelated lifetime refactor.

```bash
python3 -B tests/async/test_kingdom_contract.py
python3 -B tests/async/test_movement_liveness_runtime.py
python3 -B tests/async/test_move_cost_runtime.py
python3 -B tests/async/test_studioproc_command_movement_contract.py
./scripts/format.sh --check
make -C src
git diff --check
```

## Deployment and operational proof

Implementation was subsequently requested and the patch is in the worktree.
Production replacement and restart have not happened. Prepare the tested patch
before seeking any deployment decision.

For an authorized rollout, preserve the current runtime binary as rollback,
use the repository's established production deployment procedure, and verify
health and ordinary movement/casting. Compare a short before/after capture
using the existing profiler and NEVENT budget records under reasonably similar
load. Record wandering callback time, deferred work, and lateness; verify the
profiler is off afterward. The old short sample alone is not a controlled
baseline. Roll back on a new movement fault, crash, or material latency regression.

The deterministic test proves avoided scans. Only the operational comparison
can establish whether the resulting CPU reduction materially relieves live lag.
The untouched secondary scans may still account for meaningful cost.

## Ablation decision

Removed the membership hash index and its many lifecycle registration hooks,
the secondary `char_to_room()` optimization, permanent profiling additions,
scheduler tuning, and a separate pre-implementation profiling project. None is
required to eliminate this unconditional scan safely. Keep the existing scan
as the compatibility fallback when a removal may have occurred. The invalidation
audit, extraction regression, and build remain because omitting them risks
reintroducing a dangling-pointer crash or shipping an unbuildable hotfix.

## Implementation evidence

The patch adds one `uint64_t` counter, two invalidation increments, and the
conditional scan in `do_move()`. The other movement membership checks and the
membership predicate itself are unchanged.

Removal audit:

- `do_simple_move()` runs special procedures and delegates to
  `do_simple_move_skipping_procs()`, which enters rooms through `char_to_room()`
  and can move followers. Both destination entry checks already return when
  room entry fails. The generation snapshot covers the complete synchronous
  call, including special procedures and nested movement.
- Live global-list unlinking is in `extract_char()` in `world/handler.c`, both
  head and interior removal. Invalidation precedes recovery hooks and all
  extraction work. The early morph/name return paths conservatively invalidate.
- `free_char()` in `world/db.c` invalidates before affect teardown and schedules
  `release_mob_mem()` for later pooled storage release. That release callback
  does not run inside the synchronous movement call.
- Other direct `dead_mob_pool` releases in account loading, nanny, copyover,
  mobile loading, and SQL locker loading handle allocation/materialization
  failures before world insertion. Direct `free(ch)` sites in SQL and WebSocket
  code handle separately allocated temporary or failed-load characters.
  The flatfile adapter's `free(character)` frees an account-list record, not
  a world character. The global-list reset is in `boot_world()` on the boot path;
  other assignments insert characters. No additional movement-reachable removal
  bypass was found.

Focused runtime verification extracts the actual movement tail and membership
predicate and compiles them with production character structures and macros.
Movement and lifecycle outcomes are synthetic fixtures; this is not a full
server or live room-script test. Source contracts separately enforce production
invalidation-hook placement and the snapshot/short-circuit guard ordering.

All 22 PC/NPC scenario combinations passed under ASan/UBSan, including successful
and blocked movement, extraction, freed mover, unrelated extraction, nested
movement, death without extraction, same-address replacement, conservative
invalidation without removal, direct free, and blocked movement with unrelated
extraction. Single-invalidation cases also cross the unsigned counter wrap.
Frost and tracking effects match explicit expectations and the old guard.
With 4,096 preceding list entries, ordinary and blocked movement go from one
scan / 4,097 node visits to zero scans / zero visits. Changed-generation cases
retain the old scan counts and results.

Commands run:

| Command | Result |
| --- | --- |
| `python3 -B tests/async/test_movement_liveness_runtime.py` | Passed, 22 scenarios under ASan/UBSan. |
| `python3 -B tests/async/test_kingdom_contract.py` | Passed, including extended removal contracts. |
| `python3 -B tests/async/test_move_cost_runtime.py` | Passed. |
| `python3 -B tests/async/test_studioproc_command_movement_contract.py` | Passed. |
| `./scripts/format.sh` | Applied the required guard line wrapping. |
| `./scripts/format.sh --check` | Passed after formatting. |
| `make -C src` | Passed; linked `bin/server/dms_new` with warnings treated as errors. |
| `python3 -B tests/async/test_documentation_contract.py` | Passed, 12 checks. |
| `git diff --check` | Passed. |
| `git diff --no-index --check /dev/null <new-file>` | No whitespace errors for this document or the new runtime test; exit 1 indicates the new-file diff. |

The initial implementation build used the Makefile's default MariaDB/development
profile. The later authorized local deployment is recorded below; no remote
production-profile release qualification was performed.

## Authorized rollout and live verification

The owner subsequently requested commit/push to `master`, a clean rebuild,
restart, and live testing. Code and regression tests were pushed as
`c535f2497d756a3c9f48911bf33a9a4bbe0ab234`.

Runtime discovery superseded the earlier production assumption: this checkout
has `ENVIRONMENT=local`, a `mariadb/development` runtime, loopback Telnet on
7777, and a foreground `cycle_mud.sh` supervisor rather than an installed
systemd production service. The rollout preserved that configuration.

Both the matching-profile clean server build and all focused gates passed.
Concurrent work subsequently replaced the shared staged binary with profiling
changes. Those unrelated source edits were preserved. To remove build ambiguity,
the final binary was built from a fresh detached checkout of `c535f2497`:

```bash
make -C src -j2 PERSISTENCE_BACKEND=mariadb BUILD_PROFILE=development
```

The isolated checkout started without compiled artifacts. Its build completed
388 compiler/link invocations with zero warning/error diagnostics, and its
22-case ASan/UBSan movement regression passed. The final runtime executable
and isolated build matched SHA-256:

```text
679497aaa2bae2c4f1fcc9edac730f7cba1fb53028929f94f311691642bded04
```

The pre-rollout executable was preserved outside the clean-build tree with
SHA-256 `3509b3a95bf934b7a5178bafefaef63bf5b5df869f7dda25a35ecf01f6413c6c`.
Its owner-only recovery directory is
`/home/aiwithapex/.local/state/duris/movement-hotfix-20260910T121909Z/`.

The first ordinary `shutdown reboot` restarted the existing binary: that cycle
path does not promote staged code. A subsequent `shutdown copyover` performed
code promotion. After detecting the concurrent staged build, the isolated
candidate was explicitly staged and loaded with the same code-reload command.
The generated `lib/misc/event_names` file was then refreshed using the launcher's
`nm --demangle` recipe, and the same verified executable was reloaded to refresh
in-memory callback labels. No scheduler tuning or gameplay configuration changed.

Live Telnet checks with the configured staff character passed:

- Authenticated through the account and character menus.
- `west` with no exit produced the expected blocked-movement response.
- `up` entered the adjacent room; `down` returned to the original room. Repeated
  those movement checks after refreshing runtime event labels.
- `look` remained responsive before and after movement.
- `cast 'armor' self` started, completed, and applied armor; a subsequent `look`
  ran normally. This does not claim a fix for the separate type-ahead defect.
- Nearby NPCs continued wandering between the rooms.
- HTTP health reported `healthy` / `ready`, and the running executable checksum
  matched the isolated build. Copyover preserved the game process PID.

No live extraction/deletion scenario was injected. Extraction, death, nested
movement, and address-reuse safety are covered by the focused synthetic runtime
test and production source contracts described above.

### Short live timing comparison

The pre-hotfix capture was saved at 12:23:11 UTC and the final capture at
12:35:20 UTC, each with approximately 35 seconds of profiling. Profiling was
explicitly turned off before each save and verified off afterward. The final
capture uses the regenerated event-name map; the intermediate capture with
stale callback labels was excluded from the comparison.

| Scope | Before calls | Before elapsed work | After calls | After elapsed work |
| --- | ---: | ---: | ---: | ---: |
| Wandering section | 44,458 | 3.583092 s | 64,466 | 0.077594 s |
| Mundane NPC callbacks | 49,829 | 3.901354 s | 67,296 | 0.410144 s |
| Patrol movement callbacks | 74 | 0.089945 s | 859 | 0.007388 s |
| All callbacks | 86,194 | 4.221179 s | 147,353 | 0.815235 s |

These timers measure elapsed work inside the profiled scopes, not operating
system CPU accounting. Wandering is nested inside mundane NPC callbacks;
the rows must not be added. Average wandering-section cost fell from 80.59 us
to 1.20 us per call (about 98.5% lower). Total callback work fell about 80.7%
while callback count increased about 71%. One final callback was unnamed
(15 us), rather than the nearly all-unknown intermediate sample.

For approximate corresponding 37-second status-log windows, pre-hotfix NEVENT
budget records numbered 145 with 40,110–50,818 deferred events and reported
maximum lateness of 79–84 ticks. The final window had 16 budget records,
1,490–13,442 deferred events, and reported maximum lateness of 0–2 ticks.
These are ranges within emitted budget records, not all-pulse percentiles;
the remaining records show that occasional deferral was not eliminated.

This is a short before/after observation on the local world, including profiler
overhead and intervening restarts. World activity and callback mix differed,
so the results are strong evidence of relief for this instance, not a controlled
estimate for every workload or a production-wide latency guarantee.

Final checks found the same supervisor/game PIDs, the verified executable
checksum, and healthy/ready HTTP status. No new panic, corruption, segfault,
sanitizer, or scheduling-failure matches appeared in the final observation's
new log content. The staff character returned to its original room, exited
with `quit`, and the server closed the test connection; the temporary client
then exited. The self-cast armor was left to expire normally. The normal
shutdown/copyover path also performs its built-in player restoration.

Unrelated profiling source edits and the other investigation document remain
in the main worktree, untouched and uncommitted by this rollout.

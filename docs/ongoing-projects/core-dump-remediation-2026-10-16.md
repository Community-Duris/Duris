# Core dump investigation and repair plan — 2026-10-16

Status: repaired and qualified locally on MariaDB and flat-file persistence. Production
rollout remains a separate owner-authorized operation. Investigation source reviewed at
`b149b3ada6e8c9c1e041ad864772195173fef7c6`.

**All three root-directory dumps expose the same fatal continuation after player
extraction during `get all corpse`.** The bulk selector calls a trap function that
can kill the actor, then publishes previously selected items using that extracted
actor. The immediate crash is a null player-data dereference in
`corpse_bulk_get()`. The repair must stop the operation at the invalidating side
effect, including its callers and cleanup, rather than merely tolerate null in
the reporting helper.

The historical trigger is a fire trap on the eleventh item in the source corpse.
Core state, death logs, source tracing, synthetic probes, and the completed real-server
trap/death reproduction agree on this explanation.

## Evidence and coverage

Every `core.*` file in the project root at investigation time was inspected with
GDB, including the faulting thread and up to 18 frames of every other thread.

| Dump | File bytes | Faulting LWP | Source corpse runtime UID | Command/death time in archived logs |
| --- | ---: | ---: | ---: | --- |
| `core.2026.10.16-11.18.46` | 2,263,871,488 | 2460411 | 27086158 | 11:18:32 |
| `core.2026.10.16-11.19.23` | 2,233,618,432 | 3126256 | 28032862 | 11:19:12 |
| `core.2026.10.16-11.20.15` | 2,235,392,000 | 3127170 | 29032846 | 11:20:04 |

The process command was `bin/server/dms 7777` in each dump. The GNU build ID
embedded in the executable's mapped ELF header in **each core** matches
`bin/server/dms`: `05e20de6943c3249d34d93f19cbee2a2a194b01e`.
`bin/server/dms_new` has a different build ID,
`3f61a168db749bf0cc7ddefa3454647af5f4ad96`, and was not used to symbolize them.
The deployed executable's Git revision was not independently established; source
references below describe the reviewed checkout and the matching GDB locations.

All three stop with SIGSEGV at the same instruction, reading `0x40(%rax)` with
`rax == 0`; the recorded fault address is `0x40`. Their common main-thread stack
is:

```text
corpse_bulk_get                         cmd/actobj.c:288
do_get_log_container_artifact_pickup    cmd/actobj.c:939
do_get_finalize_container_success       cmd/actobj.c:877
take_bulk_get_items                     cmd/actobj.c:1378
start_bulk_get                          cmd/actobj.c:1537
start_container_bulk_get                cmd/actobj.c:1554
do_get                                 cmd/actobj.c:1926
command_interpreter("get all corpse")   cmd/interp.c:2450
dispatch_playing_command / session input / game_loop
```

The remaining threads were waiting or performing ordinary Redis/SQL/persistence
work. These stacks do not establish a second crash family or a worker race.

Additional observations common to all three cores:

- `actor->only.pc == NULL`, `actor->desc == NULL`, `actor->in_room == -1`
  (`NOWHERE`), and `actor->specials.position == 4` (`STAT_DEAD`).
  `free_char()` releases the PC data and clears `only.pc` while retaining the
  outer character allocation for delayed release. A non-null `P_char` and
  `IS_PC(actor)` therefore do not establish a usable character.
- The batch's original room is VNUM **95514**. `total == 0`, `failed == true`,
  `announced == false`, and nine selected items remain after the current item
  was removed from the work vector. Publication has reached the first of ten
  selected items without transferring it.
- That first item is VNUM **903**, UID **21081987**, a dark yellow ioun stone,
  with no trap flags or charges. The helper's “artifact” name is misleading for
  attribution: its caller also routes ioun stones through it. The stone is the
  next operation after the invalidation, not evidence of a faulty stone effect.
- The eleventh linked corpse item is VNUM **58387**, UID **24086420**, with
  `trap_eff == 6` (`TRAP_EFF_OBJECT | TRAP_EFF_ROOM`), `trap_dam == 2` (fire),
  `trap_level == 100`, and `trap_charge == 0`. The same values and item order
  survive in all three dumps. `trapdamage()` decrements a positive charge before
  applying damage; this is consistent with a consumed room-wide fire trap.
- Each corresponding archive under `logs/old-logs/2026.10.16-<dump-time>/`
  ends its `cmd.debug` with `get all corpse` in room 95514. Its
  `player-log/death` records the looter killed by themselves at the same command
  timestamp, matching the trap's `spell_damage(wch, wch, ...)` call. The corpse
  log then records the ioun pickup; in the later two archives the looter's name
  has already become garbage. These entries are attempted-pickup logs, not proof
  of completed item transfer. Player names and unrelated log contents are omitted.

The log timestamps precede the core filenames by 11–14 seconds. Treat the
filenames as dump identifiers, not exact command execution timestamps. Restarts
restore the source corpse with the same item UIDs and expose the failure again;
this does not by itself prove an independent persistence defect or duplicated
items.

### Source explanation

1. [start_bulk_get()](../../src/cmd/actobj.c), around lines 1487–1543, captures
   the PID and selects a whole batch before transferring any item.
2. [select_bulk_get_item()](../../src/cmd/actobj.c), around line 1449, calls
   `checkgetput(actor, object)`. This is a side effect, despite occurring during
   selection. On a triggered trap it marks the batch failed and returns false;
   it neither requests a terminal stop nor checks whether the actor survived.
3. [checkgetput()/trapdamage()](../../src/combat/trap.c), around lines 466–478
   and 593–633, can damage and extract the actor through the room-fire path.
   [extract_char()](../../src/world/handler.c), around lines 3646–3957, removes
   the character and disconnects its descriptor; [free_char()](../../src/world/db.c),
   around lines 4206–4364, clears its player data and schedules delayed release.
4. Selection resumes with stale actor/source state. `start_bulk_get()` then
   inserts the batch and calls `take_bulk_get_items()`. The first ioun item
   reaches `corpse_bulk_get()`, whose `GET_PID(actor)` expands to
   `actor->only.pc->pid` after only a PC/NPC flag check.

The precise earlier trap call is no longer on the crash stack. Its attribution
is a strong inference from the matching state and logs. The invalid actor at
publication and the unsafe continuation in the current source are directly
observed.

### Tests checked during investigation

- `tests/async/test_corpse_haul.py` **passes** under its existing ASan/UBSan
  harness. It stubs `checkgetput()` to always return false, substitutes the
  finalizers, and uses a simplified character with an inline PID. Consequently,
  it cannot detect this death/extraction path. `test_bulk_get_publication.py`
  and `test_money_carry_count.py` also stub trap checks to false.
- A temporary probe built from the corpse-haul harness and current production
  selection/publication functions makes a later selection trap invalidate the
  actor. It **fails at `corpse_bulk_get()`'s PID lookup**, with the assertion
  `GET_PID after lethal selection trap` (process exit `-6`). This establishes
  the continuation defect independently of the historical player data. The
  probe models extraction; it does not execute real combat or persistence.
  Its source, executable, and result are in the ignored local directory
  `bin/analysis/core-dumps-2026-10-16/` as `lethal_selection_probe.cpp`,
  `lethal_selection_probe`, and `lethal_selection_probe.txt`.

## Implementation progress

Work started from `45b0a2119` on `fix/corpse-loot-core-dumps`. All three repair branches
now include the fetched `origin/master` tip `6d4d71e56`; the original dumps and
matching `bin/server/dms` are preserved.

**Outcome:** stop pickup at invalidating trap effects and retain correct ownership,
reporting, cleanup and persistence across death/reconnect/restart. **Non-goals:**
production rollout, global lifetime/GET_PID changes, unrelated object-command cleanup.
**Files:** `src/cmd/actobj.c`, player snapshot capture/load, the shared SQL item-metadata
codec and legacy loader, focused pickup tests and isolated journeys, this document and
the regression catalogue/runners. **Proof:** failing-before sanitizer regression,
normal-loot regressions, builds and actual command/persistence journeys on both backends.

Ablation: keep the existing identity lookups and a private continuation check. Check
selection cancellation in `start_bulk_get`, immediately after the selector returns a
rejection, because it returns directly from a triggered trap without further actor or
object accesses. This avoids new selection-result types and persistent cancellation
state. The batch publisher returns whether reporting is still valid; its owner erases
by the saved PID after references are no longer in use. No new registry or public API.

First chunk implemented:

- Capture the actor runtime ID and original room/source; abandon queued selection on
  death/extraction, sleep, teleport, or removed/inaccessible container.
- Resolve a rejected selection's saved sibling by UID (or the existing live-chain
  fallback for UID zero), and verify custody before following it.
- Recheck publication boundaries and suppress completion on cancellation; erase the
  inserted batch by saved PID and reset publication state on allocation failure.
- Extend `test_corpse_haul.py` with a PC-data lifecycle fixture and the bulk matrix.
  The initial lethal test failed before the repair at the same `corpse_bulk_get`
  PID boundary (SIGABRT from the strict null-PC assertion). The repaired matrix passes
  under ASan/UBSan. This still stubs finalizers/combat; the second chunk and real-server
  journey below provide those additional boundaries.

Checks completed for this chunk: `python3 tests/async/test_corpse_haul.py`,
`python3 tests/async/test_bulk_get_publication.py`,
`python3 tests/async/test_money_carry_count.py`,
`python3 tests/async/test_take_coins.py`,
`python3 tests/async/test_actobj_get_limits.py`, and `./scripts/format.sh --check`.
Formatting uses the repository-documented clang-format 18.1.8 installed in the
ignored `bin/analysis/core-dumps-2026-10-16/format-env`; prepend its `bin` directory
to `PATH` for formatting and commits. Host clang-format 21 changes untouched C++
namespace layout in `.c` files, including the unmodified baseline.
`make -C src` passed, including an incremental check after formatting. Source
`76b1369852a5fb1d8c8ebdd85965b72e68176bcc` produced MariaDB development executable
build ID `62fbc6bce12e61b687d4dc1fcf82ddbaad107d07`. Build output is under
`bin/analysis/core-dumps-2026-10-16/build-mariadb{,-final}.log`.
The existing three-player `run_corpse_haul_journey.py` passed against that executable
and a disposable loopback MariaDB: normal NPC/player corpse hauls, observer output,
wallet, saved custody and reconnect. Count-cap qualification subsequently passed on
both backends.

[PR #129](https://github.com/LuminariMUD/Duris/pull/129) contains the first chunk.
A second probe executes real `get()` and finalizers with actual character/object
structs and a trap/extraction fixture. Before the second chunk it failed under ASan at
`get()`'s post-trap debug log reading `world[NOWHERE]`; output is in
`bin/analysis/core-dumps-2026-10-16/finalizer-probe.log`.

Second chunk implemented on `fix/get-trap-finalizers`:

- Treat every `get()` attempt as rejected until an object or coins are actually
  delivered. A triggered trap now returns without reaching post-trap logging, and the
  caller stops counters, corpse/artifact logs, waits, lighting and dirty-state work
  unless delivery succeeded.
- Revalidate the actor, room and source after direct and legacy pickup calls. A lethal,
  teleporting or sleeping trap returns from `do_get()` immediately; rejected legacy
  loops resolve their saved sibling by UID and verify custody before continuing.
- Publish corpse logs and cross-race artifact changes only after delivery. Exact and
  limited coin transfers retain the amount before a consumed pile is extracted, so the
  audit records the amount actually delivered. Crumble-on-loot objects no longer count
  or log as inventory deliveries.
- Add `test_get_trap_lifecycle.py`, which compiles the production `get()` and finalizers
  with actual character/object structures under ASan/UBSan. It covers surviving,
  lethal, teleport and sleep traps; container and floor pickup; ioun delivery;
  crumble-on-loot; full, partial and capped coins; ownership/no-loot/hitched and wallet
  refusals; and NPC pickup. Its extraction fixture releases PC data, clears the
  descriptor and live runtime identity, and sets `NOWHERE`/`STAT_DEAD`.

The new finalizer test and the following affected regressions pass outside the managed
sandbox, where LeakSanitizer can run: `test_corpse_haul.py`,
`test_bulk_get_publication.py`, `test_money_carry_count.py`, `test_take_coins.py`,
`test_remove_crumbleloot.py`, `test_actobj_get_limits.py`, and
`test_account_bound_reward_access.py`. `python3 -m py_compile` and
`./scripts/format.sh --check` also pass. After merging current master, both builds pass:
the MariaDB executable has build ID `bf29ffa939122bc0bfad01f6df7db733c4839990`,
and the flatfile executable has build ID
`1ff39fbd8635558ce62715ca8a85e76e06d95c5f`. Their logs are
`build-latest-master.log` and `build-flatfile-latest-master.log` in the ignored analysis
directory.
The pre-existing MariaDB three-player corpse journey and the count-cap journey on both
MariaDB and flatfile also pass against these builds. Those journeys establish normal
loot behavior; the lethal real-server journey below covers the invalidating trap path.

Third chunk implemented on `fix/corpse-trap-persistence`:

- Add `test_corpse_trap_restart_journey.py`, which constructs an isolated full-world
  corpse with an ioun stone, ordinary item and deterministic lethal room-fire trap. It
  runs the actual command, death and account-menu path; checks that a same-room mortal
  bystander receives the room effect while a durable NPC, remote client and server stay
  live; reconnects and performs another get; restarts; and compares corpse custody and
  item UIDs before and after restart on both maintained backends.
- The journey failed before the persistence repair after the lethal trap had been
  consumed: the restored corpse had no persisted trap state, so it would reload the
  prototype's live charge. That failure is recorded in the ignored
  `corpse-trap-mariadb-before.log`.
- Persist nonzero runtime trap fields through the existing internal item
  extra-description channel as `DURIS_TRAP_STATE_V1`. The shared codec accepts exactly
  four bounded integer fields; capture emits one canonical record; both snapshot and
  legacy SQL materializers restore it without exposing it as an in-game description.
  Existing snapshots without the marker retain their prototype state.
- Save a player corpse immediately when a trap triggers during direct or bulk pickup,
  so the consumed charge reaches persistence even when the actor dies before normal
  command-end saving.
- Register the flat-file journey in `make test`, the MariaDB journey in `make test-db`,
  and document both backend commands in `REGRESSIONS.md`. The final MariaDB and
  flat-file journeys pass, including direct raw
  authority inspection of the three retained UIDs and the `6,2,0,100` spent state; after
  final pickup, exactly the trap UID moves and the other two remain. The completion audit
  also places a mortal bystander and durable NPC in the trap room: the bystander receives
  the fire effect and dies, the NPC survives, and neither outcome resumes the invalidated
  pickup; both backend journeys pass with these assertions.

The completion audit registered the flat-file leg with the normal regression discovery
and retained the MariaDB leg in `make test-db`. The exact dispatcher command
`python3 tests/run_regression_tests.py --jobs 1 --match corpse_trap_restart` passed its
single discovered flat-file journey in 278.67 seconds, including a verified server build.
The renamed MariaDB entry also passed when invoked with the qualified MariaDB executable
and disposable loopback database. The existing three-player and count-cap journeys
assert inventory and wallet behavior; the lethal journey enumerates the target UIDs
across all corpse authority, verifies the source corpse's visible contents, and on
MariaDB explicitly confirms none of those UIDs entered the looter's death corpse.

Focused codec, capture, load, routing, lifecycle and corpse-haul regressions pass, as do
the affected pickup, coin, removal, limits and account-bound access tests. The MariaDB
build has ID `d0161cd00fe5ac599356e48151309a8c80d1a920`; the flat-file build has ID
`8370195a339cd0efd9e4f19c6510c77add5b9871`.
[PR #131](https://github.com/LuminariMUD/Duris/pull/131) contains this third chunk,
whose qualified implementation commit is `c8a37fcc2` and whose base is the finalizer
repair in PR #130. Completion-audit coverage and both-backend runner registration are
in `f6dbe86b6`.

Local resource notes: Docker access is unavailable to this user, so a dedicated
MariaDB was initialized with `--no-defaults` in `/tmp/duris-core-repair-db-*`, bound
to loopback on an ephemeral port. Its path/port were recorded during qualification in
an ignored local file; the server was stopped and its directory removed afterward.
Journey environments explicitly set `ENVIRONMENT=local` and never source `.env`. A
separate flatfile build targets `bin/server/dms_core_repair_flatfile`; its output is in
`build-flatfile.log`.

The disposable database and all journey runtimes contained synthetic data and were
removed after qualification; no production configuration was sourced.

## Implementation sequence

### 1. Capture a failing regression at the actual boundary

- [x] Extend [test_corpse_haul.py](../../tests/async/test_corpse_haul.py), or add
  one focused native test beside it, to run a lethal trap during selection after
  at least one ordinary/ioun item has already been selected. Assert that no
  previously selected item is transferred, no subsequent item is inspected, and
  no success report or PID lookup occurs after extraction.
- [x] Exercise the real `do_get` finalizers and the PC-data lifecycle in at
  least one test. Use actual extraction or a faithful fixture that releases PC
  data, clears the descriptor, removes the actor's runtime identity, and sets
  `NOWHERE`/`STAT_DEAD`. A new stub that merely skips the crash site is inadequate.
- [x] Demonstrate the test fails on the reviewed source for this defect. Keep the
  existing normal-haul tests as coverage of behavior that must remain intact.

### 2. Stop selection as soon as its actor or source becomes invalid

Primary file: [src/cmd/actobj.c](../../src/cmd/actobj.c).

- [x] Capture the existing character runtime ID, PID, and source identity while
  the actor is valid. Reuse
  [find_character_by_runtime_id()](../../src/account/character_identity.c) and
  `find_live_item_uid()`; no new lifetime registry is needed.
- [x] Immediately after `checkgetput()`, establish that the original actor is
  still present and alive before reading any further actor field or resuming
  the selection loop. Propagate a terminal stop out of selection and
  `start_bulk_get()` if it has died or been extracted. Do not insert or publish
  the partially selected batch in this case.
- [x] Stop if the trap teleported the actor away from the original room or made
  the actor unable to act. Revalidate the source container/location before using
  a saved sibling pointer or publishing selected items: a trap is permitted to
  change the world. `bulk_get_source_matches()` currently checks object custody
  without checking that the actor can still reach that source.
- [x] Preserve nonlethal trap behavior for an actor that remains able to act in
  the original location: the triggered item is rejected and eligible untrapped
  items may still be collected. Preserve charge consumption, damage, carry
  limits, coin selection, and existing ownership restrictions.

Checking `only.pc` only inside `corpse_bulk_get()` is insufficient: selection,
logging, waits, lighting, and reporting also use the actor. `IS_ALIVE()` is useful
after establishing lifetime, not as a general check on an arbitrary freed pointer.

### 3. Carry cancellation through pickup, callers, and cleanup

This is the same lifetime contract in adjacent paths, not a separate claim that
the dumps crashed in all of them.

- [x] Audit `take_bulk_get_items()`, `get()`, `do_get_commit_pickup_core()`, the
  container/floor finalizers, and `do_get()` for continuation after trap effects.
  In the single-item path, `get()` returns when `checkgetput()` triggers, but its
  caller currently treats that as success unless `item_get_rejected` was set.
  Correct the rejected/terminal result and propagate it through these private
  helpers so counters, pickup logs, post-processing, and legacy loops stop
  correctly. Keep the public API changes limited to what this path needs.
- [x] Distinguish actual delivery from an attempted pickup. Do not report or
  log successful delivery after a lethal trap, or count a rejected item as
  acquired. Audit logs that need the actor's identity must use it while valid;
  cancellation must never format a name from extracted player storage.
- [x] Restore `item_get_ack_publication` on every exit from publication and
  remove any inserted `bulk_gets` entry using the saved PID. Separate cleanup
  from reporting: an extracted actor must not reach `report_bulk_get()`. Avoid
  erasing a map entry while another frame still uses its `bulk_get_state&`.
  A later login/get by the same PID must not inherit an abandoned batch.
- [x] Confirm the floor bulk and filtered bulk entry points return safely, and
  single-item/legacy paths cannot fall through into `do_get()`'s dirty marking,
  debug output, lighting, or wait updates after extraction. Verify the command
  interpreter's existing post-command death guard with a full command test.

Do not change global `GET_PID` semantics, defer all death processing, disable
traps, or refactor unrelated object commands to mask this failure. Expand into
`trap.c` or other callers only if the focused reproduction exposes a defect
there, recording that evidence separately.

### 4. Prove the relevant variants

| Case | Required result |
| --- | --- |
| Lethal room-fire trap after selected ioun/ordinary items | Actor is extracted once; selection stops; queued items stay in the original corpse; no stale read, pickup report, or leftover batch. |
| Trap first, last, and the only eligible item | Safe termination also holds with an empty selection and with many earlier selections. |
| `get all corpse`, `get all.<filter> corpse`, floor bulk, single item from corpse/floor | Every route through the same trap boundary returns safely, including the outer command's post-processing. |
| Surviving damage trap | One charge/effect, no delivery of the trapped item, accurate partial result, and ordinary eligible items still behave as before. |
| Teleport or sleep trap | No remote pickup from the old room and no further pickup while unable to act. |
| Room trap with another occupant | Actor and bystander death/survival combinations do not resume work on an extracted actor; room effects still occur as intended. |
| Source/item invalidated by a side effect | No stale source/sibling access or delivery from a changed source; use existing object identity resolution. |
| Cancellation after batch insertion; subsequent login/get | Batch and publication state are cleared without reporting to the old character or blocking the new operation. |
| Ordinary NPC/player corpse hauls, coins, caps, NPC pickup | Existing output, inventory, wallet, and ownership assertions continue to pass. |

Use deterministic damage and synthetic objects/accounts. Do not copy live player
records into fixtures. Cover extraction with ASan/UBSan and assertions on ownership,
remaining work, and state cleanup; an absence of SIGSEGV alone is not enough.

### 5. Verify with a real isolated server and persistence

- [x] Extend [run_corpse_haul_journey.py](../../tests/async/run_corpse_haul_journey.py)
  or add one adjacent journey using its disposable runtime/database pattern.
  Create a corpse with ordinary/ioun items before an object-triggered room-fire
  trap and a mortal looter with deterministic lethal health/damage. Run the
  actual `get all corpse` command. The player returns to the account menu, an
  observer remains connected, and the server continues processing commands.
- [x] Inspect the original corpse, death corpse, inventory, wallet, and item UIDs:
  no selected item was duplicated, lost, or transferred after death. Reconnect
  and perform another get to prove stale batch state cannot block the player.
- [x] Save and restart the isolated server, then verify corpse contents, ownership,
  wallet, and the trap's expected spent state. Repeat the recovery/loot sequence
  that produced the three historical dumps. If trap consumption or custody
  fails persistence independently of the crash, add the smallest reproducible
  persistence repair before closing this plan; do not assume a database bug from
  these snapshots alone.
- [x] Run the applicable journey on both maintained persistence backends, reusing
  [run_corpse_haul_count_cap_journey.py](../../tests/async/run_corpse_haul_count_cap_journey.py)
  for the existing backend fixture pattern. Register new executable coverage in
  the existing regression/database runners and describe it in
  [REGRESSIONS.md](../testing/REGRESSIONS.md).

### 6. Build, review, and hand off

After implementing the C/C++ changes, run the smallest relevant checks, including
the new regression and journey. Existing targeted commands are:

```sh
./scripts/format.sh --check
make -C src
python3 tests/async/test_corpse_haul.py
python3 tests/async/test_bulk_get_publication.py
python3 tests/async/test_money_carry_count.py
python3 tests/async/test_take_coins.py
python3 tests/async/test_get_trap_lifecycle.py
python3 tests/async/test_item_extra_descr_codec.py
python3 tests/async/test_player_load_items.py
python3 tests/async/run_corpse_haul_journey.py /absolute/path/to/test/server
python3 tests/async/run_corpse_haul_count_cap_journey.py /absolute/path/to/test/server mariadb
python3 tests/async/run_corpse_haul_count_cap_journey.py /absolute/path/to/test/server flatfile
python3 tests/async/test_corpse_trap_restart_journey.py
python3 tests/async/test_corpse_trap_restart_journey.py /absolute/path/to/test/server mariadb
```

Journey commands require explicitly configured disposable test resources and the
corresponding backend build. Run them from an isolated development/test checkout;
do not inherit this checkout's production configuration. Keep all compiled
artifacts under `bin/`. Record the exact new regression/journey commands and
before/after results when implemented. If shared trap/get behavior changes,
include the existing single-item, carry-limit, and removal regressions affected
by that change. Local executable and gameplay evidence is the completion gate;
waiting for CI is not required.

- [x] Review the final diff for a narrow lifetime/cancellation repair and remove
  unneeded abstractions or unrelated changes.
- [x] Record the qualified source revision and executable build ID. Preserve
  the original dumps and matching executable for comparison before replacing
  runtime artifacts.
- [x] Hand off the qualified fix for the existing deployment workflow. Production
  rollout requires separate owner authorization. After an authorized rollout,
  observe the relevant get/death path and new cores/logs; lack of a new core
  without exercising the path is not sufficient validation.

All three historical dumps now have this common disposition; the regression fails
before and passes after the repair; the listed variants are covered; both real-server
death/reconnect/restart journeys pass; and formatting/build checks pass. The runtime
repair is qualified locally. Production rollout and observation remain operational work
requiring separate owner authorization.

## Rechecking the dumps

This command reads a core without launching or attaching to the production server:

```sh
gdb -nx -q -batch \
  -iex 'set auto-load off' -iex 'set debuginfod enabled off' \
  bin/server/dms core.2026.10.16-11.20.15 \
  -ex 'bt 16' \
  -ex 'p $_siginfo' \
  -ex 'p/x $rax' -ex 'x/i $pc' \
  -ex 'p actor->only.pc' -ex 'p actor->desc' \
  -ex 'p actor->in_room' -ex 'p actor->specials.position' \
  -ex 'frame 3' -ex 'p state.total' -ex 'p state.failed' \
  -ex 'p state.announced' \
  -ex 'p state.synchronous_items._M_impl._M_finish-state.synchronous_items._M_impl._M_start' \
  -ex 'p obj_index[object->R_num].virtual_number'
```

Repeat for the other two filenames. Use the recorded matching build ID; a later
replacement of `bin/server/dms` must not silently change the symbol source.

The original investigation changed documentation only, plus ignored local probe
artifacts. The checkout identifies itself as `production`. Implementation progress
and validation are recorded above. No production server has been stopped, attached
to, restarted, or deployed; production databases, player data, logs and core dumps
remain untouched. Test databases and runtimes are disposable and explicitly local.

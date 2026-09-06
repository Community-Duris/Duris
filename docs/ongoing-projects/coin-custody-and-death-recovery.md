# Coin custody and death recovery

**Implemented and verified locally on 2026-09-06; not deployed.** All four
completion steps pass. NPC wallet and reset-created coins are lootable; rejected
deaths preserve durable evidence and quarantine disputed custody before release.
The final isolated flat-file journey measured **0.992s** from real `EMSGSIZE`
refusal to the account menu and passed process restart/re-entry checks.

## Implementation

- Committed in `5ee2b380c`: atomic wallet/pile coin transactions for MariaDB and
  flatfile, command/completion handling, persisted coin payloads, loader updates,
  and migration `0010_coin_custody_payload`. Covers pile creation, merge, partial
  pickup, retirement, overflow rejection, and operation-ID replay.
- Committed in `f08101cbf`: full-wallet conversion through one coin transaction
  in `money_to_inventory()` (no 32,000-per-denomination clamp), corpse handoff
  waiting on currency completion, and legacy corpse money totals excluding
  tracked piles.

### This session

- `src/combat/fight.c` did not compile as committed: `currency_transaction_player_busy()`
  was used without including `economy/currency_transaction.h`. Fixed.
- **Rejected-corpse-transfer recovery.** `corpse_item_completion()` records the
  owner in a fixed, allocation-free dispute table instead of leaving the retry to
  resubmit the same refusal forever. `event_death_extract_retry()` finalizes a
  disputed death through `save_disputed_death_disposition()`, which captures the
  immutable death record and waits up to `DEATH_DISPOSITION_TIMEOUT_MSEC` (2000)
  for durability before releasing the character through
  `release_after_terminal_death()`. `die()` defers to that recovery event while a
  dispute is outstanding, clears any stale dispute at the start of a new death,
  and the retry clears one when a recovery is abandoned.
- **Wallet-conversion rejection is no longer silent.** `money_to_inventory()`
  returns whether the conversion was submitted and alerts on rejection;
  `make_corpse()` disputes the death when it was not, and the disposition
  re-creates the unattached pile from the live wallet so the coins are recorded.
- **A missing live corpse can no longer let an ordinary empty save complete a
  disputed death**: the retry keeps the live character and its refused assets and
  backs off instead.
- **New `player_save_pipeline_terminal_death()`** captures the death snapshot
  against a freshly marked terminal revision, enqueues it, and waits on the same
  fence the ordinary terminal save uses. The shared reserve/await steps were
  factored into `begin_terminal_fence()` and `await_terminal_fence()`.
- **Both save backends now apply death snapshots.**
  - MariaDB: `player_snapshot_repository_apply()` accepts the death schema version
    and writes `player_death_disposition` (operation id, corpse UID and room,
    wallet before conversion, wallet pile UID, encoded payload) plus one
    `player_death_custody` row per disputed custody observation, inside the same
    transaction that clears the player's items and bumps `save_revision`.
  - Flatfile: `flatfile_player_snapshot_apply()` publishes the record to
    `player-deaths/<pid>-<revision>.death` before writing the player file, and
    strips the death from the player file so a later ordinary save cannot
    overwrite the record.
- **Migration `0011_player_death_disposition`** (apply + verify + manifest
  entries + runtime compatibility head). The two tables are created only by
  their migration, not by `bootstrap_multithread_safe.sql`. Step 4 below now
  registers them in runtime compatibility and data lifecycle, with fingerprints
  verified on disposable MySQL 8 and MariaDB 10.11 servers.
- `player-deaths` added to the provisioned flat-file directories.

### Idempotency

Retries and reconnects cannot repeat death consequences: the disposition commits
under the player's `save_revision` fence, so a replayed frame reports
`already_applied` and rewrites nothing; `REPLACE INTO` and a fixed flat-file name
make a re-applied record byte-identical rather than duplicated. The wallet, once
converted, is zero, and the death snapshot carries no inventory, so re-applying
clears an already-empty inventory.

## Verification before the completion plan

These results cover the earlier implementation. The original gameplay fixture
did not cover NPC coins or the disputed-death restart; current completion evidence
is recorded under the numbered plan below.

Build: `make -C src` clean (see "Build notes"). `./scripts/format.sh --check`
clean.

| What | How |
| --- | --- |
| MariaDB death apply, migration re-runnability, verify script, replay, ordinary save after death, malformed record rejection | `tests/async/run_player_death_disposition_mysql.sh` (new; disposable MariaDB 10.11 container) — passed |
| Flat-file death apply, empty-handed player file, disposition round-trip, replay `already_applied`, survives a later ordinary save | `tests/async/test_flatfile_player_repository.py` (extended harness) — passed |
| Terminal death fence, capture→enqueue→await ordering, invalid death without a corpse, timeout when neither backend can save | `tests/async/test_player_save_pipeline.py` (extended) — passed |
| Journal retains a death frame across restart and blocks replay until its own apply succeeds | `tests/async/test_player_save_journal.py` — passed (pre-existing coverage, still green) |
| Dispute, durable disposition, missing-corpse, wallet preservation, release ordering, stale-dispute clearing | `tests/async/test_death_item_custody_contract.py` (repaired and extended) — passed |
| Wallet-conversion dispute in `make_corpse()` | `tests/async/test_corpse_handoff_inflight_contract.py` (repaired and extended) — passed |
| Real in-game sequence on isolated flat-file storage: combat, NPC corpse loot, save, reconnect, player death, corpse recovery, reconnect | `tests/async/test_flatfile_combat_journey.py` — passed |
| Full-world flat-file boot, player and floor-item process-restart journey | `tests/async/test_flatfile_full_world_boot.py` — passed |
| Coin put / merge / partial pickup / retirement / replay identity | `tests/async/test_coin_custody_lifecycle.py`, `tests/async/test_currency_input_queue.py` — passed |

Both `test_death_item_custody_contract.py` and `test_corpse_handoff_inflight_contract.py`
were already failing against `f08101cbf` before any change here; they were
repaired as part of this work.

Tests updated because a contract they pin moved: `test_character_persistence_gap.py`,
`test_terminal_extract_item_retention.py` (extraction now happens inside
`release_after_terminal_death()`), `test_immutable_migration_runner.py`,
`test_runtime_boot_compatibility.py` (migration head 0011),
`test_flatfile_boot_preflight.py`, `test_flatfile_full_world_boot.py`,
`test_persistence_mode.py` (new `player-deaths` directory), and
`test_currency_input_queue.py` (`money_to_inventory()` now returns `bool` and
alerts, so its extracted-body harness needed the new signature and a
`persistence_alert()` stub).

New files: `migrations/immutable/0011_player_death_disposition.{sql,sh}`,
`tests/async/player_death_disposition_mysql_harness.cpp`,
`tests/async/run_player_death_disposition_mysql.sh`.

## NPC pickup diagnosis (fixed by step 2)

Before this fix, coins in an NPC corpse could not be looted. `do_get()` reaches `submit_coin_get()`,
which requires an **active custody row** for the pile
(`item_ownership_runtime_lookup()` in `submit_coin_get()`, and again in
`prepare_coin_pile()`); a pile that reached a mob through `money_to_inventory()`'s
NPC branch or a zone reset has no such row, so the pickup is refused with
"The coin transfer could not start; nothing changed."

Reproduced by adding a money object to the flat-file journey fixture, giving it to
the fixture mob, and trying `get coins corpse` after the kill. This predates this
session's changes; the fixture change was reverted at the time. It is now included
in the completion plan below. The earlier suggestion of a `creation`-reason coin
source is not supported by the current implementation:
`coin_transfer_command.c::validate_endpoint()` explicitly rejects creation on the
source side. Establishing custody for the existing pile must preserve the current
coin transaction's conservation and replay checks.

## Scopeguard completion plan

Implemented under [scopeguard](../../.agents/skills/scopeguard/SKILL.md). The steps
below record the completed work and its verification. Deployment remains separate.

**Outcome:** NPC corpse coins can be collected without loss or duplication;
rejected player-death handoffs preserve durable recovery evidence before release;
capacity failures preserve live assets; and both database engines verify the death
tables. Close each item with observed results, not source assertions alone.

**Non-goals:** redesigning currency or death persistence, automatic restitution of
disputed assets, new recovery services, generic fault-injection or benchmarking
infrastructure, new dependencies, unrelated cleanup, and production changes.

**Files:** the existing implementation and tests named below are inspection targets;
edit only those needed for a demonstrated gap, plus this document for results.

**Proof:** run each step's focused checks once after its changes, extending tests
only for uncovered acceptance criteria. After C/C++ changes, run `make -C src` and
`./scripts/format.sh --check`. Record exact results and unverified runtime behavior.

### 1. Repair the snapshot check and cover death-specific capacity failures

- [x] Fix `tests/async/test_player_snapshot_capture.py` to check publication once
  per capture function. Its whole-file assertion incorrectly counts ordinary and
  death capture together.
- [x] Extend that harness for actual death capture: a valid combined record and
  oversized records covering the shared byte, row, and object budgets across the
  corpse, refused inventory, wallet pile, and custody evidence. Check death-tree
  nesting and string limits where existing tests do not exercise the same path;
  avoid repeating every boundary for every asset source.
- [x] In `tests/async/test_player_save_pipeline.py`, make the existing capture stub
  return `limit_exceeded` and verify nothing is enqueued or authorized for release.
  Reuse the death custody contract's retention checks. Change
  `src/player/player_snapshot_capture.c`, `src/player/player_snapshot_codec.c`, or
  `src/player/player_save_pipeline.c` only if these cases expose a defect.

Acceptance: valid records succeed; oversized capture leaves the output and live
assets intact, with no partial save or release. Keep existing limits and retries.
Run `python3 tests/async/test_player_snapshot_capture.py`,
`python3 tests/async/test_player_save_pipeline.py`,
`python3 tests/async/test_player_item_snapshot_codec.py`, and
`python3 tests/async/test_death_item_custody_contract.py`.

Step 1 implementation (2026-09-06): the existing capture test now links the real
capture adapter, codec, and custody runtime. It verifies a combined corpse,
refused inventory, wallet pile and custody record; shared byte/row/object limits;
death-parent nesting and wallet-string limits; and unchanged published output and
live assets after rejection. The pipeline stub now returns `limit_exceeded` and
checks no enqueue, no acknowledged/journaled fence, and a recorded capture failure.
No production change was needed: although the death adapter starts a new row
counter, its final encode/decode validation enforces the combined record limits.
All four focused commands listed above passed, including a refined row-boundary
fixture whose ordinary snapshot fits but whose combined death record does not.

Plan ablation: all four steps protect explicit completion criteria. Reuse existing
harnesses and admission/completion APIs; add no test infrastructure or services.

### 2. Restore NPC corpse coin pickup using existing custody admission

- [x] Reinstate the money-bearing NPC fixture in
  `tests/async/test_flatfile_combat_journey.py`. Reproduce `get coins corpse` and
  trace both NPC wallet coins and reset-created coins into the failing pickup.
- [x] Inspect `src/cmd/actobj.c`, `src/world/handler.c`, and
  `src/item/item_movement_transaction.c`; reuse the existing untracked-item
  admission and completion path to establish custody before tracked coin pickup.
  Confirm it supports the corpse owner and parent. Avoid a second admission
  mechanism or a general refactor.
- [x] Add coverage only for the newly admitted pile: correct owner/parent, refusal
  of conflicting or retired custody, and retry/restart between admission and
  pickup. Use the existing coin lifecycle test and flat-file/MySQL item harnesses
  where the assertion belongs. Reuse their full/partial pickup and replay cases
  once the pile follows the tracked path.

Acceptance: both NPC coin sources are lootable; wallet plus remaining pile value
is conserved across pickup, reconnect, and replay. Admission never credits the
wallet, and pickup credits only after its atomic commit. A missing runtime entry
must not override durable custody. Preserve existing UID/revision and conservation
checks; if existing commands cannot express the fix, resolve that concrete scope
issue under scopeguard before changing public APIs or persisted/wire formats.
Run `python3 tests/async/test_coin_custody_lifecycle.py`,
`python3 tests/async/test_flatfile_item_repository.py`, and
`bash tests/async/run_item_transfer_schema_mysql.sh`; run the extended combat
journey with step 3.

Step 2 progress (2026-09-06): `submit_coin_get()` now uses the existing absent-item
admission and completion path before the tracked pickup. The queue harness covers
held, failed, and successful admission without early wallet credit. The flat-file
item harness passes admission replay, active/retired UID conflict, and pickup in a
new process. `test_coin_custody_lifecycle.py`, `test_currency_input_queue.py`,
`test_flatfile_item_repository.py`, and `run_item_transfer_schema_mysql.sh` passed.
The last script used the verified local development database (`duris_dev`).
An extended SQL currency case exposed a missing baseline after ordinary admission;
`item_transfer_repository.c` now writes the existing `coin_payload` column in that
transaction, sharing the coin transaction's writer. The disposable MariaDB
`bash tests/async/run_currency_transaction_schema_mysql.sh` now passes.
The live test exposed a second gap after successful coin pickup: custody advanced
its room revision without updating the room item projection, so the next ordinary
banana pickup failed materialization. The earlier item harness ended after coins
and therefore missed this interaction.

The room gap is now fixed through an internal helper in
`flatfile_world_item_repository`, called from `flatfile_coin_apply`. It prepares
both legs against one room catalog and includes the resulting image in the
existing authority transaction. It preserves revision checks, removes retired
piles, updates remaining amounts and parent weights, and retains the existing
loading path for imported piles without a room-transfer projection. No external
API or persisted format changed.

`python3 tests/async/test_flatfile_item_repository.py` now passes with partial
pickup, a two-pile merge in one room, replay, pickup in a new process, and an
ordinary banana admission afterward. `python3 tests/async/test_flatfile_world_item_repository.py`
also passes. The live journey now gets both NPC coins and the banana, saves and
reconnects, and recovers the banana and coins after an ordinary player death. The
reset-created pile also passes pickup, save, process restart and reconnect in the
separate reset fixture. The fixture disables optional boons through the normal
`toggle boon` command so reward fences do not redirect this ordinary-death case
to wallet-rejection recovery; that rejection retains its focused coverage.

A separate pre-existing fixture finding: fresh character creation leaves the live
bank revision at 0, while the flat-file account bank is at 1. This causes currency
`ESTALE` until reconnect. The journey now reconnects before combat and seeds an
empty boon catalog using the existing repository API. No character-creation or
reward gameplay code was changed. Fresh-character currency without reconnect is
not validated or fixed by this patch.

### 3. Close the in-game disputed-death gap and measure that same journey

- [x] Extend `tests/async/test_flatfile_combat_journey.py` with a conflicting-custody
  fixture that reaches the real `EMSGSIZE` refusal. Verify disposition durability
  precedes release, then restart/reconnect and inspect wallet, item UIDs, and
  custody evidence. If this fixture cannot reach the refusal, identify the exact
  gap before adding the smallest test-only extension under scopeguard. Harness
  results alone do not complete this in-game check.
- [x] Reuse `tests/async/test_player_save_journal.py` for durable handoff before
  apply, restart, and blocked replay, and the existing repository tests for
  duplicate apply and survival of a later ordinary save. Extend only a missing
  assertion needed to connect those checks to the refused-death record; do not
  duplicate these scenarios across every harness. Keep wallet-rejection,
  missing-corpse, and failed-durability checks in their existing focused tests.
- [x] Measure refusal-to-account-menu time with the journey's monotonic clock and
  report the backend and observed duration. One retry tick (normally 1s) plus
  the 2000ms durability wait is one successful attempt's budget, **not an overall
  worst-case bound**: pending transfers, failed saves, and missing corpses retry.

Acceptance: the refused death leaves durable evidence before release; restart and
replay preserve it without repeating death consequences or wallet changes. Failed
recovery retains live assets. A disposition records recovery evidence; it does
not establish that disputed items have been returned to a corpse.
Run `python3 tests/async/test_flatfile_combat_journey.py`,
`python3 tests/async/test_player_save_journal.py`,
`python3 tests/async/test_corpse_handoff_inflight_contract.py`,
`python3 tests/async/test_flatfile_player_repository.py`, and
`bash tests/async/run_player_death_disposition_mysql.sh`. Reuse step 1's pipeline
and death-contract results unless subsequent changes affect them.

Step 3 observed results (2026-09-06): the complete
`python3 tests/async/test_flatfile_combat_journey.py` run passes for both NPC coin
sources, ordinary player death/corpse recovery, real rejected handoff, durable
disposition before release, and restart using the same storage and journals.
The final run measured **0.992s** on the isolated flat-file primary backend.
The death counter increases exactly once; restart/re-entry and another save
preserve death count, experience, level, wallet balances/revision, and byte-identical
death records. Disputed inventory remains absent on re-entry.

The journey exposed three necessary integration fixes: the worker rejected the
death schema; terminal death capture did not queue its revision before dispatch;
and active custody let login restore disputed inventory. The worker and terminal
entry point now use their existing schema/revision paths. Death apply quarantines
remaining player custody using the existing state, preserving UID, parent, owner
and payload, including durable-only children. Flat-file apply publishes evidence
before committing custody and the empty player file together through the existing
authority transaction. SQL performs quarantine inside the existing death transaction.
No external API or persisted format changed.

The focused pipeline, worker, journal, custody/corpse and both repository checks
also pass. The flat-file repository uses existing fault hooks to cover failure
before commit and interrupted writes; retry recovers both authority images.
SQL verifies replay, retained custody identity and an unaffected second owner.
Wallet-rejection and missing-corpse cases retain their focused coverage. This
records evidence for operator recovery; it does not perform automatic restitution.

### 4. Register the existing death tables and verify both database engines

- [x] Fix `tests/async/run_runtime_compatibility_mysql.sh` to copy, apply, verify,
  and record migration 0011; its fixture currently stops at 0010 and
  `applied_count=10`, while the manifest requires 0011.
- [x] Add `player_death_disposition` and `player_death_custody` to
  `migrations/runtime_compatibility_manifest.json` and
  `migrations/data_lifecycle_manifest.json`, following the existing protected
  recovery-record policy. Include migration 0011 in the schema inputs of
  `scripts/validate_data_lifecycle.py` and its existing test.
- [x] Generate fingerprints on disposable MySQL 8 and MariaDB 10.11 databases;
  synchronize `src/core/runtime_compatibility_contract.h` and the count assertion
  in `tests/async/test_runtime_boot_compatibility.py`. Preserve the sealed baseline
  and immutable migrations. This registers existing tables and adds no schema.

Acceptance: fresh setup and migration replay pass on both engines; a missing death
table fails the compatibility gate. Run
`python3 tests/async/test_data_lifecycle_manifest.py`,
`python3 tests/async/test_runtime_boot_compatibility.py`, and
`RUNTIME_DB_IMAGE=mysql:8.0 bash tests/async/run_runtime_compatibility_mysql.sh`
plus the same command with `RUNTIME_DB_IMAGE=mariadb:10.11`. Keep this item open if
an engine is unavailable; do not invent fingerprints or weaken the boot gate.

Step 4 observed results (2026-09-06): both exact `RUNTIME_DB_IMAGE` commands
above passed on disposable containers, including 0011 replay and rejection when
either death table is renamed away. Measured normalized fingerprints are
`e2d802053dfdf981d3f72841bce675e467845ae347ae62bf3f89c9e4b065c1d2`
(MySQL 8) and
`edfe38e99686d863083ba936279ce4e2cb370e5a9d2a7c5395e6ba79208c5d62`
(MariaDB 10.11). The two tables are protected recovery records retained across
season/terminal actions; their disclosure and retention policy remains pending.
The lifecycle suite passed 11 tests and the runtime boot suite passed 7 tests.
The runtime validator's pinned count and post-baseline exclusion set also needed
synchronizing in `scripts/validate_runtime_compatibility.py`; the sealed baseline
and immutable migration files remain unchanged.

### Completion record

The implementation gates below supersede the earlier planning-only results.

| Command | Current observed result |
| --- | --- |
| `python3 tests/async/test_player_snapshot_capture.py` | Passed real death capture and combined capacity limits |
| `python3 tests/async/test_player_save_pipeline.py` | Passed terminal intent, death revision queue, capacity refusal and durability fences |
| `python3 tests/async/test_player_item_snapshot_codec.py` | Passed |
| `python3 tests/async/test_death_item_custody_contract.py` | Passed |
| `python3 tests/async/test_coin_custody_lifecycle.py` | Passed |
| `python3 tests/async/test_currency_input_queue.py` | Passed admission/completion and no early wallet credit |
| `python3 tests/async/test_flatfile_item_repository.py` | Passed admission, room projection, conservation, replay and process restart |
| `python3 tests/async/test_flatfile_world_item_repository.py` | Passed |
| `bash tests/async/run_item_transfer_schema_mysql.sh` | Passed on the verified local development database |
| `bash tests/async/run_currency_transaction_schema_mysql.sh` | Passed on disposable MariaDB |
| `python3 tests/async/test_flatfile_combat_journey.py` | Passed both coin sources, normal recovery, disputed death, restart and death-count/experience/level comparisons; 0.992s refusal-to-menu |
| `python3 tests/async/test_player_save_worker.py` | Passed death-schema dispatch and journal acknowledgement |
| `python3 tests/async/test_player_save_journal.py` | Passed durable death handoff, restart and blocked replay |
| `python3 tests/async/test_corpse_handoff_inflight_contract.py` | Passed |
| `python3 tests/async/test_flatfile_player_repository.py` | Passed death evidence, quarantine, interrupted commit recovery and replay |
| `bash tests/async/run_player_death_disposition_mysql.sh` | Passed on disposable MariaDB, including custody quarantine |
| `python3 tests/async/test_data_lifecycle_manifest.py` | Passed, 11 tests |
| `python3 tests/async/test_runtime_boot_compatibility.py` | Passed, 7 tests |
| `RUNTIME_DB_IMAGE=mysql:8.0 bash tests/async/run_runtime_compatibility_mysql.sh` | Passed including migration replay and both missing-death-table rejections |
| `RUNTIME_DB_IMAGE=mariadb:10.11 bash tests/async/run_runtime_compatibility_mysql.sh` | Passed the same gates |
| `make -C src -j4` | Passed |
| `./scripts/format.sh --check` and `git diff --check` | Passed |

Completion audit (2026-09-06): all numbered steps and listed gates pass against
the current implementation. The final gameplay run includes both NPC coin sources,
ordinary player death and corpse recovery, the real `EMSGSIZE` refusal, and
restart/re-entry comparisons. The measured 0.992s is one observed successful
recovery on isolated flat-file storage, not a worst-case bound. Live MariaDB
gameplay was not exercised; its death apply/replay and currency contracts were
verified in the repository suites.

The pre-existing fresh-character bank-revision issue and the fixture's explicit
boon opt-out are described in step 2. Production deployment, automatic restitution,
and fresh-character currency without reconnect remain outside this change.
The final diff contains the implementation, focused tests, required manifest
registration, this document, and the separately requested scopeguard clarification.
No temporary debugging remains in source. Compiled test artifacts stay under `bin/`;
isolated gameplay data and certificates are temporary. The unrelated pre-existing
`.agents/skills/plan-ablation/` work remains untouched.

## Build notes

Historical setup notes from the earlier implementation follow. On 2026-09-06,
the direct `make -C src -j4` build succeeds with the current host environment.

`make -C src` needs cJSON, GnuTLS, hiredis, libbsd, libxml2 and ICU headers, none
of which were installed on this host and none of which can be installed without
root. They were staged into `~/.local` (headers under `~/.local/include`,
libraries under `~/.local/lib`, with `~/.local/include/libxml` symlinked to
`~/.local/include/libxml2/libxml`) and the build was run as:

```
make -C src -j8 EXTRA_CFLAGS="-isystem$HOME/.local/include"
LIBRARY_PATH=$HOME/.local/lib LD_LIBRARY_PATH=$HOME/.local/lib
```

`EXTRA_CFLAGS` is the Makefile's own wrapper hook; no repository file was changed
for this. Tests that invoke `g++` directly need `CPATH=$HOME/.local/include`
instead — without it `tests/async/test_world_recovery_pipeline.py` fails to
compile `hiredis/hiredis.h` on this host, unrelated to any change here.

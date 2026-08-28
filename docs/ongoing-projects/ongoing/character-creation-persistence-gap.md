# Persistence, Load & Death Recovery Defects

**Date:** 2026-08-28
**Status:** **Completed** - all seven required fixes are implemented; the local-dev data
repairs and guarded migration have been applied and verified.
**Verified against:** current working tree and `duris_dev`, 2026-08-28
**Components:** Character Creation, Save Pipeline, Player Load Pipeline, Account Menu, Death Handler (`die`)

---

## 1. Bug: New Character Creation Bypasses Initial `player_data` Insert

### Summary
Newly created characters are registered on the account but never have their baseline record inserted into `player_data`. This causes all subsequent background and terminal saves for that character to fail indefinitely with `ENOENT`.

### Code Locations
* [`src/files.c:L1558-1594`](file:///home/aiwithapex/projects/duris/src/files.c#L1558-L1594) (`writeCharacter`, pipeline routing branch)
* [`src/nanny.c:L5382-5395`](file:///home/aiwithapex/projects/duris/src/nanny.c#L5382-L5395) (`init_char`, PID assignment and no-baseline flag)
* [`src/nanny.c:L6001`](file:///home/aiwithapex/projects/duris/src/nanny.c#L6001) (`writeCharacter` at end of creation; the same call recurs at [`L6045`](file:///home/aiwithapex/projects/duris/src/nanny.c#L6045) for `CON_WELCOME`)
* [`src/player_snapshot_repository.c:L544-620`](file:///home/aiwithapex/projects/duris/src/player_snapshot_repository.c#L544-L620) (`player_snapshot_repository_apply`)
* [`src/sql_player.c:L1211`](file:///home/aiwithapex/projects/duris/src/sql_player.c#L1211) (the only `INSERT INTO player_data` in the tree)

### Root Cause
1. During character creation, `init_char()` assigns a PID from the on-disk counter (`getNewPCidNumb()`, `src/nanny.c:L182`) — a file-only allocation that touches no database table. `add_char_to_account()` (`src/account.c:L2060`, called from `src/nanny.c:L5907`) then registers the character, producing the `account_characters` row.
2. At the final step of creation, `writeCharacter(d->character, 2, NOWHERE)` is called (`2` is `RENT_QUIT`).
3. Because `GET_PID(ch) > 0`, no transaction is open, and `RENT_QUIT` is non-terminal per `player_save_pipeline_is_nonterminal_type()` (`src/player_save_pipeline.c:L572`), `writeCharacter()` returns early through the async `player_save_pipeline_request()` path.
4. The synchronous fall-through it skips is the one that reaches `sql_save_player_status()`, which selects `INSERT INTO player_data (...)` when no row exists for the character's name.
5. The pipeline's apply step instead runs:
   ```sql
   SELECT save_revision FROM player_data WHERE pid = <pid> FOR UPDATE;
   ```
   and only ever issues `UPDATE player_data`. There is no insert fallback.
6. Because the initial `INSERT` never occurred, this query returns 0 rows, and the worker transaction rolls back with `terminal_failure` / `ENOENT` on every save attempt.

### Confirmed Impact (local dev DB, 2026-08-28)
`pid = 64` (`Druvv`, created 2026-08-27 19:30:26) is present in `account_characters` and absent from `player_data`. A `LEFT JOIN` over all non-deleted account characters returns exactly this one orphan, so the defect is real but so far affects a single character on this database.

---

## 2. Bug: Death Save Failure Leaves Player Stranded in `STAT_DEAD` Limbo

### Summary
When a player takes lethal damage and the synchronous terminal death save fails or times out, the server aborts character extraction (`extract_refused=1`) but leaves the character permanently trapped in the room in `STAT_DEAD` status with all player commands blocked.

### Code Locations
* [`src/fight.c:L3039-3052`](file:///home/aiwithapex/projects/duris/src/fight.c#L3039-L3052) (terminal failure branch inside `die`, which begins at [`L2479`](file:///home/aiwithapex/projects/duris/src/fight.c#L2479); recovery event at [`L2418-2477`](file:///home/aiwithapex/projects/duris/src/fight.c#L2418-L2477))
* [`src/actoth.c:L2016-2034`](file:///home/aiwithapex/projects/duris/src/actoth.c#L2016-L2034) (`persistence_save_character_terminal`)
* [`src/player_save_pipeline.c:L404-470`](file:///home/aiwithapex/projects/duris/src/player_save_pipeline.c#L404-L470) (`player_save_pipeline_terminal`)
* [`src/interp.c:L1565`](file:///home/aiwithapex/projects/duris/src/interp.c#L1565) (the command block for `STAT_DEAD`)

### Root Cause
1. In `die()`, the engine modifies character state *before* persistence:
   * Non-permanent affects, disease/poison conditions, and undead spell slots are stripped (`src/fight.c:L2929-2947`).
   * Position is set to `STAT_DEAD + GET_POS(ch)` (`src/fight.c:L2950`).
   * HP is reset to 1 immediately before the save (`src/fight.c:L2977`).
2. `die()` then attempts the synchronous terminal save:
   ```c
   if (!CHAR_IN_ARENA(ch) && !persistence_save_character_terminal(ch, RENT_DEATH))
   {
       persistence_alert(AVATAR, "player_save", "death", "none", "none",
                 "terminal_save_failed", "extract_refused=1");
       send_to_char("Your death could not be saved. You remain in the world for recovery.\r\n", ch);
       return;
   }
   ```
3. If the save fails (or times out on the 2000 ms fence passed to `player_save_pipeline_terminal`), `extract_char()` is skipped to protect memory state, and `die()` exits via early `return;`.
4. **The Recovery Gap:**
   * `persistence_save_character_terminal()` *does* schedule a deferred retry — `persistence_schedule_character_save(ch, RENT_CRASH, ..., "terminal-save-retry")` — so the save itself will be re-attempted. Nothing, however, re-attempts the *death*.
   * The character's position is never restored or unwound from `STAT_DEAD`.
   * No nevent exists to complete `extract_char()` once the deferred save acknowledges.
   * The player cannot issue any commands (blocked by `"Lie still; you are DEAD!!!"`), while remaining a live entity in the room that aggressive mobs continue to attack.

---

## 3. Bug: Every Character on an Account Fails to Load with "Sorry, I couldn't load that character!"

### Summary
On account `mosheh`, selecting any of the four characters produces `Loading character...` followed by `Sorry, I couldn't load that character!`. The asynchronous load pipeline succeeds; the failure is in the **materialize** step, which rejects the snapshot because `player_item_extra_descr` has accumulated exact duplicate rows. A second, independent data defect (orphaned `player_items` rows with no `item_current_owner`) blocks two of the four characters even after the duplicates are removed.

This is related to the persistence defects above only in that all three live in the save/load split: the save path writes state the load path then refuses to accept.

### Reproduction (local dev, 2026-08-28)
Driven through a scripted telnet session against `bin/server/dms` on port 7777, with GDB breakpoints on the load path:

```
Play as Zusuk? (Y/N) Loading character...
Sorry, I couldn't load that character!
```

Observed failure chain, confirmed live:

```
>>> player_load_materialize
<<< gameplay_read_state_publish        -> true
<<< player_revision_hydrate            -> true
<<< player_load_pets_stage             -> true
<<< player_load_item_graph_materialize -> false
    outcome=invalid_snapshot items=2 ops=6 depth=0
<<< player_load_materialize            -> false
```

### Code Locations
* [`src/account.c:L1093-1099`](file:///home/aiwithapex/projects/duris/src/account.c#L1093-L1099) (`account_confirm_char`, emits the message when `load_char_into_game` returns `NULL`)
* [`src/account.c:L1872`](file:///home/aiwithapex/projects/duris/src/account.c#L1872) (`load_char_into_game` calls `player_load_materialize`)
* [`src/player_load_materialize.c:L498-510`](file:///home/aiwithapex/projects/duris/src/player_load_materialize.c#L498-L510) (SESSION03 `player_load_item_graph_materialize` branch, now with diagnostic logging)
* [`src/player_load_items.c:L359-365`](file:///home/aiwithapex/projects/duris/src/player_load_items.c#L359-L365) (`valid_item_metadata` gate)
* [`src/player_load_items.c:L157-172`](file:///home/aiwithapex/projects/duris/src/player_load_items.c#L157-L172) (duplicate extra-description rejection)
* [`src/sql_player.c:L2133-2185`](file:///home/aiwithapex/projects/duris/src/sql_player.c#L2133-L2185) (`sql_save_item_extra_descr`: unconditional `INSERT`, no delete)
* [`src/sql_player.c:L2836-2860`](file:///home/aiwithapex/projects/duris/src/sql_player.c#L2836-L2860) (batch save re-inserts descriptions for items whose rows were only updated)
* [`src/sql_player.c:L2928-2940`](file:///home/aiwithapex/projects/duris/src/sql_player.c#L2928-L2940) (delete is skipped entirely on the incremental path and scoped to `equip_slot>0` on equipment-only saves)

### Root Cause
1. `valid_item_metadata()` builds a set of `keyword\0description` keys per item and returns `invalid` the first time an entry repeats. Exact duplicate extra descriptions are therefore treated as snapshot corruption.
2. `sql_save_item_extra_descr()` issues a bare `INSERT INTO player_item_extra_descr (...)` for every extra description on every save. The only thing that removes prior rows is the FK cascade from `DELETE FROM player_items WHERE pid=?`, which runs **only** on a full inventory save.
3. On the incremental path (`use_incremental`, all items already have DB ids and neither dirty bit is set) no delete runs at all, and on an equipment-only save the delete is scoped to `equip_slot>0`. Item rows survive and are updated in place while their descriptions are inserted again.
4. `player_item_extra_descr` has no unique constraint on `(item_id, keyword, description)`, so copies accumulate — up to **14 copies** of a single row on this database.
5. The load then rejects the character outright, and neither the repository nor the SESSION03 materialize branch logs a reason, so the failure is silent apart from the player-facing message. (The SESSION02 branch at `player_load_materialize.c:L462-470` *does* log; SESSION03 does not.)

### Confirmed Impact (local dev DB, 2026-08-28)
Duplicate `player_item_extra_descr` rows by owner:

| pid | character | duplicate groups | duplicate rows |
|-----|-----------|------------------|----------------|
| 1   | Zusuk     | 4                | 8              |
| 2   | Selwyn    | 26               | 226            |
| 58  | Amoz      | 17               | 101            |
| 59  | Pyret     | 6                | 12             |
| 60  | Druv      | 44               | 127            |

**Verified fix:** removing the duplicate rows for `pid = 1` only (keeping `MIN(id)` per `item_id, keyword, description`) made Zusuk load and enter the game at `A Walled in Corner of the Abyss [875:87557]`, and `player_load_item_graph_materialize` returned `applied items=2 ops=14 depth=1`. The 4 deleted rows are backed up in the session scratchpad. The other characters were left untouched.

### Secondary Defect: Orphaned `player_items` Rows
A direct harness run of `player_load_repository_execute` shows two characters failing one layer earlier, at the repository:

```
pid=1  outcome=applied           queries=22 items=2  auth=2
pid=2  outcome=component_failure queries=14 items=2  auth=0
pid=58 outcome=applied           queries=22 items=25 auth=25
pid=59 outcome=component_failure queries=14 items=3  auth=0
pid=64 outcome=not_found         queries=4            (Druvv - see section 1)
```

`load_items` LEFT JOINs `player_items` to `item_current_owner`; a missing owner row yields NULL columns, `parse_unsigned` fails, and the whole load aborts. Counts of `player_items` rows with no `item_current_owner`: pid 2 = 10, pid 59 = 1, pid 60 = 11. On pid 2 these orphans come in **pairs sharing the same `obj_uid`** (7807, 7808, 7810, 7804, 7805), i.e. `player_items` itself has duplicated rows — the same "insert without delete" family of bug. Duplicate `obj_uid` would also trip the `item_by_uid` uniqueness check in `load_items` even if the owner rows existed.

So Selwyn and Pyret need the orphan/duplicate item rows repaired as well; deduplicating extra descriptions alone will not unblock them.

### Note on Query Budget
`player_load_repository_execute` issues exactly **22** queries for a full SESSION03 load, and `PLAYER_LOAD_QUERY_MAX` is **22**. There is zero headroom: adding one query anywhere in the load path will fail every character load with `limit_exceeded`.

---

## 4. Regression: `display_account_menu` Dereferences NULL (server SIGSEGV)

### Summary
Commit `3ee140aa` ("Resolve code scanning alerts", 2026-08-28 02:19) changed a NULL guard into a NULL dereference. Every one of the ~20 `display_account_menu(d, NULL)` call sites now segfaults the server. A binary built from current `HEAD` crashes as soon as a player presses RETURN past the account MOTD — before character selection is even reachable.

### Code Locations
* [`src/account.c:L354-356`](file:///home/aiwithapex/projects/duris/src/account.c#L354-L356) (`display_account_menu`)
* [`src/nanny.c:L5691-5698`](file:///home/aiwithapex/projects/duris/src/nanny.c#L5691-L5698) (`CON_ACCT_RMOTD` — the first call site a player reaches)

### The Change
```diff
 void display_account_menu(P_desc d, char *arg)
 {
-	if (!arg)
+	if (!*arg)
 {
```

### Observed Crash
```
Thread 1 "dms_new" received signal SIGSEGV, Segmentation fault.
#0  display_account_menu (d=0x..., arg=arg@entry=0x0) at account.c:356
#1  nanny (d=..., arg=0x... "") at nanny.c:5686
#2  game_loop (port=7777, sslport=7778) at comm.c:1439
```

The same commit made the same `!arg` → `!*arg` substitution in several `do_*` command handlers; those are benign because the interpreter always passes a non-NULL buffer. `display_account_menu` is the only changed function whose call sites pass a literal `NULL`.

### Note
This regression postdates the character-load report in section 3, which was observed on the previous binary (`bin/server/dms`, built 2026-08-27 19:35). Section 3 was reproduced against that binary because a `HEAD` build cannot reach the character menu.

---

## 5. Required Fixes

1. **Initial Character Save Path**:
   * Ensure newly created characters execute an initial synchronous `INSERT INTO player_data` (or add an upsert/`INSERT` fallback in `player_snapshot_repository_apply` for the `ENOENT` case) before entering normal gameplay.
2. **Death Terminal Save Recovery State**:
   * Either roll back the position state upon failed terminal save and schedule a retry extraction nevent, or transition the descriptor to a controlled recovery state that re-attempts extraction once the deferred save pipeline acknowledges.
3. **Data Fix for Druvv**:
   * Insert the missing baseline record for `pid = 64` into `player_data` on the local dev database.
4. **Extra-Description Save Path** (section 3):
   * Make `sql_save_item_extra_descr()` delete existing rows for the item before inserting, or make the write an upsert, so the incremental and equipment-only save paths stop accumulating copies. The same applies to `sql_save_item_affects()` and the pet equivalents.
   * Add a unique index on `player_item_extra_descr (item_id, keyword, description(255))` (and the pet table) as a schema-level guard, via an additive, re-runnable migration.
   * Consider whether the load should tolerate exact duplicates rather than refusing the character, since the data is semantically identical.
5. **Load Diagnostics** (section 3):
   * The SESSION03 branch of `player_load_materialize()` and `player_load_repository_execute()` log nothing on failure. Add the same `logit(LOG_DEBUG, ...)` the SESSION02 branch already emits, plus the repository outcome and the failing component, so a load refusal is diagnosable without a debugger.
6. **Orphaned Item Rows** (section 3):
   * Repair the `player_items` rows on pids 2, 59 and 60 that have no `item_current_owner` row, and the duplicate `obj_uid` rows on pid 2. Investigate which save path emits them.
7. **`display_account_menu` NULL Deref** (section 4):
   * Revert `if (!*arg)` to `if (!arg)` in `src/account.c:356`, or guard as `if (!arg || !*arg)`. This is a server-crash regression on `HEAD` and should land before anything else here.

---

## 6. Progress Log

Live status of the fixes in section 5. Updated as work lands so an interrupted
session can resume without re-deriving state.

| # | Fix | Status |
|---|-----|--------|
| 7 | `display_account_menu` NULL deref | **done** (commit `34bbd5b4`, already on this branch; `src/account.c:356` reads `if (!arg)`) |
| 1 | Initial character save path | **done** (see below) |
| 2 | Death terminal save recovery | **done** (see below) |
| 3 | Data fix for Druvv (`pid=64`) | **done** (baseline player, epic and wallet rows inserted on `duris_dev`) |
| 4 | Extra-description save path + unique index | **done** (see below) |
| 5 | Load diagnostics (SESSION03 + repository) | **done** (see below) |
| 6 | Orphaned item rows (pids 2, 59, 60) | **done** (payload/custody repaired and every affected snapshot loads) |

### Fix 4 - extra-description / affect accumulation (landed)

* `src/sql_player.c` `sql_save_item_extra_descr()` now issues
  `DELETE FROM <table> WHERE item_id = ?` before writing, and the delete runs even when
  the object currently has no descriptions (so descriptions removed in-game are removed
  from the DB too). The batch save path no longer guards the call behind
  `obj->ex_description`, which was the path that left stale rows.
* `sql_save_item_affects()` and `sql_save_pet_item_affects()` got the same delete-first
  treatment; the pet variant also gained the duplicate `(location, modifier)` skip the
  player variant already had.
* `migrations/immutable/0002_player_item_metadata_uniqueness.sql` - additive,
  re-runnable and registered in the immutable migration ledger. Deduplicates
  `player_item_extra_descr`, `player_pet_item_extra_descr`, `player_item_affects` and
  `player_pet_item_affects` keeping `MIN(id)` per group, then adds the unique keys
  (`uk_item_descr`, `uk_pet_item_descr`, `uk_item_affect`, `uk_pet_item_affect`) guarded
  on `information_schema.statistics`. Description keys use a 255-byte prefix.
  Applied twice successfully to `duris_dev`; the second application verified the guards.
* Load now tolerates exact duplicates: `duplicate_description()` in
  `src/player_load_repository.c` drops an exact `(keyword, description)` repeat while
  building the snapshot, for both player and pet items, so the materialize step never
  sees the duplicate that used to make it refuse the character. Duplicates no longer
  count against `PLAYER_LOAD_ITEM_DESCRIPTION_MAX` either.

### Fix 5 - load diagnostics (landed)

* `player_load_result` gained `const char *failed_component` (`src/player_load_repository.h`).
  The repository worker runs off the game thread and cannot call `logit`, so
  `player_load_repository_execute()` now records which stage refused the load
  (`status`, `components`, `items`, `pets`, `gameplay_reads`, `bank`, `deadline`,
  `budget`, `commit`, `commit_deadline`) and hands it back to the game thread.
* `src/player_load_materialize.c` logs on every SESSION03 refusal that was previously
  silent: the `valid_snapshot` gate (with pid, outcome, error code, repository
  component and query/row counts), `player_load_item_graph_materialize`, and the three
  ownership-hydration failures.

No query was added to the load path, so the 22-query budget noted in section 3 is
untouched.

### Fix 1 - initial `player_data` insert (landed)

* New flag `CHAR_RFLAG_NO_DB_BASELINE` (`src/defines.h`, `BIT_3` of the runtime-only
  `runtime_flags`). `init_char()` sets it right where `getNewPCidNumb()` hands out a
  file-allocated pid, so the character is explicitly marked as having no `player_data`
  row. This covers all three creation entry points (`nanny.c`, `ws_handlers.c`,
  `wiz_newchar.c`) since they all call `init_char()`.
* `writeCharacter()` (`src/files.c`) refuses the async pipeline branch while that flag is
  set, so the first save falls through to the synchronous path - the only one that
  reaches `sql_save_player_status()` and its `INSERT INTO player_data`.
* `sql_save_player()` clears the flag only after the complete synchronous save succeeds
  (and its owned transaction commits). `sql_save_player_status()` logs
  `component=baseline outcome=inserted pid=<pid>` but deliberately does not clear the
  flag, because a later component can still roll the INSERT back.
* Belt and braces: `player_save_pipeline_pulse()` (game thread) now watches for
  `terminal_failure` completions carrying `ENOENT` - the exact signature of the missing
  baseline row - re-sets the flag on the live character so its next save goes back
  through the synchronous insert path, and logs
  `component=apply outcome=missing_baseline pid=<pid> sync_fallback=<0|1>`.

### Fix 2 - death terminal-save recovery (landed)

`src/fight.c` gained `schedule_death_extract_retry()` / `event_death_extract_retry()`.
When `die()` cannot save the death it still refuses to extract (unchanged - that
protects unsaved state), but it now schedules a recovery nevent instead of abandoning
the character in `STAT_DEAD`:

* the event re-attempts `persistence_save_character_terminal(ch, RENT_DEATH)`;
* on success it finishes exactly what `die()` would have done - reset the flee timer,
  `add_track()`, `update_ingame_racewar()` for non-immortals, `extract_char()` - and
  tells the player;
* on failure it reschedules with doubling backoff, clamped to
  `[DEATH_EXTRACT_RETRY_INITIAL=4, DEATH_EXTRACT_RETRY_MAX=60]` pulses;
* if the character is no longer `STAT_DEAD` (resurrected, restored by a god) or has
  entered the arena, the event abandons the recovery instead of extracting them.

Every branch emits a `persistence_alert` (`death_recovery_retry`,
`death_recovery_completed`, `death_recovery_abandoned`).

### Regression test

`tests/async/test_character_persistence_gap.py` (wrapper
`tests/async/run_character_persistence_gap.sh`) is a source-contract test covering
fixes 1, 2, 4, 5 and 7 - the runtime flag and its three touch points, the death
recovery event, the delete-before-insert on descriptions/affects, the migration's
four guarded unique keys and four dedupe steps, the repository stage names, the
materialize log lines, the unchanged 22-query ceiling, and the `display_account_menu`
NULL guard. Passing.

`make -C src` is clean and `./scripts/format.sh --check` reports formatting OK.

---

## 7. Completed local-dev data repairs

All repairs below were applied only to `duris_dev`. Before mutation, the affected
payload, metadata, ownership and Druvv rows were dumped to a mode-`0600` recovery
directory at `/tmp/duris-character-persistence-repair.G2zWxq`.

### 7.1 Druvv (`pid = 64`)

The surviving account and leaderboard evidence identified Druvv as a level-1 Lich
Necromancer (`race=21`, `m_class=11`, `racewar=3`). A transaction inserted:

* the missing `player_data` baseline with `save_revision=0`;
* `epic_balance_baseline (64,0,0)`;
* `currency_wallet_baseline (64,0,0,0,0,0,0)`.

The rolled stats were irretrievable, so all fields without surviving evidence use the
schema defaults. This preserves the account character and its name rather than deleting
it. The full SESSION03 repository load now returns `applied` for pid 64 in 22 queries.

### 7.2 Item payload and custody repair

The repair ran in asserted transactions and made the smallest identity changes possible:

* pid 2's five duplicate pairs retained the later payload row (the later rows contained
  the newest cost/metadata state) and preserved UIDs 7804, 7805, 7807, 7808 and 7810;
* pid 59's colliding UID 7810 received allocator UID 17293822569120704983;
* pid 60 kept UID 22403 for one item; the second, different-vnum item received allocator
  UID 17293822569120704984;
* all other unambiguous orphan UIDs were preserved;
* matching zero-revision `item_ownership_baseline` and active `item_current_owner` rows
  were inserted with roots/parents derived from the retained payload graph;
* the 13 original collision quarantine records were marked repaired;
* stale active custody UID 52390, which had no payload in the newer replacement save,
  was preserved as quarantined rather than deleted or reconstructed from incomplete
  data;
* UID 91674's zero-revision root/parent topology was aligned to the newer retained
  payload (`root=parent=149077`).

Final invariants: zero payload rows without custody, zero duplicate `(pid,obj_uid)`
groups on pids 2/59/60, zero open target collision quarantines, and exact active
payload/custody counts.

### 7.3 Origin of the item drift

The investigation found three related mechanisms:

1. The duplicated/colliding UIDs predated the ownership cutover. The 2026-08-27 baseline
   correctly quarantined them (`conflict_code=1`) instead of inventing ownership.
2. Replacement saves in both `sql_save_player_items()` and
   `src/player_snapshot_repository.c`'s `apply_items()` delete/reinsert `player_items`
   independently of `item_current_owner`. This changed payload row ids while preserving
   unresolved UIDs, which is why quarantine records referenced older source ids than the
   current rows.
3. Same-player container movement does not submit a cross-owner transaction, and legacy
   object removal can leave zero-revision custody behind. That explains the stale parent
   for UID 91674 and payload-less active custody UID 52390 found during verification.

The requested investigation is complete; no evidence showed the repaired high allocator
UIDs were colliding in another retained custody table.

### 7.4 Migration and load verification

`migrations/immutable/0002_player_item_metadata_uniqueness.sql` was applied twice.
Afterward all four
duplicate-group counts were zero and all four guarded unique indexes existed with the
expected signatures. It is registered as immutable migration sequence 2, its verifier
passes, and `mud_schema_migration_state.applied_count` is 2 on `duris_dev`.

`player_load_repository_execute()` was then run against pids 1, 2, 58, 59, 60 and 64.
Every load returned `applied` at the unchanged 22-query ceiling. The MySQL harness now
also supports `PLAYER_LOAD_REAL_ONLY=1` for this non-mutating configured-character check
and reports `failed_component` on a refusal.

---

## 8. Final validation

Passing on 2026-08-28:

* `python3 tests/async/test_character_persistence_gap.py`
* player load item, pet and pipeline source-contract suites
* player save pipeline, worker and snapshot-capture source-contract suites
* SQL persistence path and nevent death source-contract suites
* `tests/async/run_player_load_repository_mysql.sh`
* `python3 tests/async/test_immutable_migration_runner.py`
* immutable migration 0002 verifier and recorded-history check
* disposable full-schema runtime compatibility on MySQL 8.0 and MariaDB 10.11
* `migrations/reconcile_item_ownership.sh` (all four mismatch counts zero)
* real SESSION03 repository loads for pids 1, 2, 58, 59, 60 and 64
* `./scripts/format.sh --check`
* `git diff --check`
* `make -C src`

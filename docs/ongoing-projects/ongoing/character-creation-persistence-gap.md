# Persistence & Death Recovery Defects

**Date:** 2026-08-28
**Components:** Character Creation, Save Pipeline, Death Handler (`die`)

---

## 1. Bug: New Character Creation Bypasses Initial `player_data` Insert

### Summary
Newly created characters are added to `account_characters` but never have their initial baseline record inserted into `player_data`. This causes all subsequent background and terminal saves for that character to fail indefinitely with `ENOENT`.

### Code Locations
* [`src/files.c:L1584-1592`](file:///home/aiwithapex/projects/duris/src/files.c#L1584-L1592) (`writeCharacter`)
* [`src/nanny.c:L5995`](file:///home/aiwithapex/projects/duris/src/nanny.c#L5995) (`writeCharacter` on creation)
* [`src/player_snapshot_repository.c:L544-622`](file:///home/aiwithapex/projects/duris/src/player_snapshot_repository.c#L544-L622) (`player_snapshot_repository_apply`)

### Root Cause
1. During character creation ([`init_char()`](file:///home/aiwithapex/projects/duris/src/nanny.c#L5383)), a new PID is assigned, and [`add_char_to_account()`](file:///home/aiwithapex/projects/duris/src/account.c#L2060) creates the `account_characters` row.
2. At the final step of creation, `writeCharacter(d->character, 2, NOWHERE)` is called.
3. Because `GET_PID(ch) > 0` and `type == 2` (non-terminal), `writeCharacter()` routes the save into the async `player_save_pipeline_request()`.
4. In legacy code, `writeCharacter()` fell through to `sql_save_player_status()`, which executed `INSERT INTO player_data (...)` for new characters.
5. In the refactored save pipeline, `player_snapshot_repository_apply()` queries:
   ```sql
   SELECT save_revision FROM player_data WHERE pid = <pid> FOR UPDATE;
   ```
   and only executes `UPDATE player_data`.
6. Because the initial `INSERT` never occurred, this query returns 0 rows (`NULL`), and the worker transaction rolls back with `ENOENT` on every save attempt.

---

## 2. Bug: Death Save Failure Leaves Player Stranded in `STAT_DEAD` Limbo

### Summary
When a player takes lethal damage and the synchronous terminal death save fails or times out, the server aborts character extraction (`extract_refused=1`) but leaves the character permanently trapped in the room in `STAT_DEAD` status with all player commands blocked.

### Code Locations
* [`src/fight.c:L2949-2985`](file:///home/aiwithapex/projects/duris/src/fight.c#L2949-L2985) (`die`)
* [`src/actoth.c:L2016-2035`](file:///home/aiwithapex/projects/duris/src/actoth.c#L2016-L2035) (`persistence_save_character_terminal`)
* [`src/player_save_pipeline.c:L404-450`](file:///home/aiwithapex/projects/duris/src/player_save_pipeline.c#L404-L450) (`player_save_pipeline_terminal`)

### Root Cause
1. In `die()`, the engine modifies character state *before* persistence:
   * Position is set to `STAT_DEAD + GET_POS(ch)`.
   * HP is reset to 1.
   * Spell slots and non-permanent affects are stripped.
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
3. If the save fails (or times out on the 2000 ms fence), `extract_char()` is skipped to protect memory state, and `die()` exits via early `return;`.
4. **The Recovery Gap:**
   * The character's position is never restored or unwound from `STAT_DEAD`.
   * No automated retry nevent exists to complete `extract_char()` once the background save acknowledges.
   * The player cannot issue any commands (blocked by `"Lie still; you are DEAD!!!"`), while remaining a live entity in the room that aggressive mobs continue to attack.

---

## 3. Required Fixes

1. **Initial Character Save Path**:
   * Ensure newly created characters execute an initial synchronous `INSERT INTO player_data` (or add an `INSERT` fallback in `player_snapshot_repository`) before entering normal gameplay.
2. **Death Terminal Save Recovery State**:
   * Either roll back the position state upon failed terminal save and schedule a retry extraction nevent, or transition the descriptor to a controlled recovery state that re-attempts extraction once the save pipeline acknowledges.
3. **Data Fix for Druvv**:
   * Insert the missing baseline record for `pid = 64` into `player_data`.

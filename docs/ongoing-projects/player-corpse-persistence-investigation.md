# Player Corpse Persistence Investigation

**Incident date:** August 26, 2026

**Investigation date:** August 26, 2026

**Severity:** High (self-persisting corpse corruption; resurrection and race-war behavior can change after a restart)

**Status:** Fixed and fully validated on `bugfix-corpse`; implementation published through `563a9502`

**Target subsystem:** Player-corpse SQL persistence (`files.c`, `fight.c`, `sql_player.c`)

---

## 1. Executive summary

The `100` corpse shown in the supplied transcript was real and is fully explained by an off-by-one SQL result
mapping in `sql_load_all_corpses()`.

The loader at the time of the incident selected these adjacent fields:

```text
row[26] = corpse_items.obj_uid
row[27] = corpse_items.item_condition
row[28] = corpses.short_descr
row[29] = corpses.description
```

It then assigns:

```text
corpse.short_description <- row[27]  (item condition, normally "100")
corpse.description       <- row[28]  (the corpse's short description)
row[29]                              (the real room description is ignored)
```

That produces the exact split seen in game:

- the room shows `the corpse of Amoz` because room listings use `corpse->description`;
- `look in corpse` still says it belongs to Amoz because that path uses `corpse->action_description`;
- get, inventory, drop, and wizlog messages say `100` because they use `corpse->short_description`.

The corruption is not confined to memory. Looting, moving, preserving, or dropping a restored corpse calls
`writeCorpse()`, which saves the already-shifted strings back to SQL. The pre-repair local row for Amoz was
persisted as:

```text
player_name = Amoz
save_id     = 1787741833
room_vnum   = 96537
short_descr = 100
description = the corpse of Amoz
item_count  = 0
```

The empty corpse in the transcript is **not evidence of item loss**. The command and corpse logs show that
Amoz successfully removed the corpse contents shortly before Zusuk inspected it.

A separate, broader defect was also confirmed: the outer corpse's gameplay metadata was never stored in the
`corpses` table and was not reconstructed on load. A restored corpse lost its death-time level, resurrection
XP, owner PID, race-war side, race, humanoid/carving flags, owner-specific keywords, carved-part state, and
true weight. This can alter resurrection, necromancy, carving, race-war artifact handling, and movement after
any server restart.

## 2. Incident evidence

### Supplied in-game transcript

The transcript's three views of one object disagree:

```text
room:       [    2] the corpse of Amoz
look in:    It appears to be the corpse of Amoz. / Nothing.
take:       You get 100.
inventory:  [    2] 100
drop:       You drop 100.
```

The `[    2]` prefix is the immortal vnum display for corpse prototype `#2`, not a quantity.

### Local command and corpse logs

The current logs reproduce the supplied command sequence and explain why the corpse was empty:

| Time (IDT) | Evidence |
|---|---|
| 19:32:42 | Boot restore logs `Restored player corpse 100 from sql_load_all_corpses`. |
| 19:35:27 | Zusuk takes Amoz's corpse in room `130412`. |
| 19:37:36 | Zusuk runs `drop all.corpse`; wizlog records `Zusuk drops 100 [96537]`. |
| 19:38:00–19:38:12 | Amoz runs `take all corpse` three times; the corpse log records 26 recovered top-level items. |
| 19:40:28 | Zusuk runs `l in corpse`, after Amoz has emptied it. |
| 19:40:30 | Zusuk runs `take corpse`. |
| 19:40:35 | Zusuk runs `inv`. |
| 19:40:40 | Zusuk runs `drop 100`, which enters the coin parser and fails as shown in the transcript. |
| 19:40:43 | Zusuk runs `drop corpse`; wizlog again records `Zusuk drops 100 [96537]`. |

Relevant local evidence is in:

- `logs/log/cmd.debug`
- `logs/player-log/corpse`
- `logs/player-log/wizcmds`
- `logs/log/events`

The persistence event log shows the corpse's contents being serialized repeatedly while Amoz looted it.
The database had zero `corpse_items` rows for this corpse because those items were recovered, not because
the loader discarded them.

### Pre-repair live local database

A read-only query against the local development database found one corpse row and one corrupt display row:

```text
corpse rows                         1
numeric short_descr rows            1
short_descr exactly "100" rows      1
description beginning "the corpse" 1
```

The row's `created_at` is `2026-08-26 19:40:43`, the time of the final drop. This is not the original death
time: `sql_save_corpse()` deletes and reinserts the row on every save. The stable `save_id` decodes to
`2026-08-26 13:57:13 IDT` and is the better incident identifier.

No database rows were changed during this investigation.

## 3. Primary root cause: shifted result columns

The pre-fix query in `src/sql_player.c:7442-7456` returned 38 fields. The relevant tail began as follows:

| Index | Selected expression | Intended destination |
|---:|---|---|
| 24 | affect location | contained item affect |
| 25 | affect modifier | contained item affect |
| 26 | `ci.obj_uid` | contained item UID |
| 27 | `ci.item_condition` | contained item condition |
| 28 | `c.short_descr` | corpse short description |
| 29 | `c.description` | corpse room description |
| 30–37 | item diff fields | contained item fields |

The pre-fix corpse reconstruction at `src/sql_player.c:7583-7592` instead read rows 27 and 28. The item loader later
correctly used `row[27]` for `obj->condition` at `src/sql_player.c:7680-7681`, proving that the same value was
used for two unrelated destinations.

For a corpse with contents, the first joined item normally has condition `100`, so the defect is deterministic.
For an empty corpse, the left join supplies `NULL` for `row[27]`; the loader leaves the prototype short text in
place and still assigns the saved short description to the long-description field. Empty and non-empty corpses
therefore corrupt differently across successive restarts and saves.

### Why each command displayed a different value

- Room lists call `show_obj_to_char(..., LISTOBJ_LONGDESC, ...)`, which reads `object->description`
  (`src/actinf.c:725-728`).
- Inventory lists use `LISTOBJ_SHORTDESC`, which reads `object->short_description`
  (`src/actinf.c:729-732`).
- `look in` special-cases corpses and prints `action_description`
  (`src/actinf.c:2807-2814`).
- Get messages use `$p`, and drop/wizlog paths directly read `short_description`
  (`src/actobj.c:484-488`, `src/actobj.c:2190-2204`).

This is why the object remains findable as `corpse` while presenting as `100`: its prototype keyword and its
owner identity field survive, while its display fields are crossed.

## 4. How the bad value became durable

`make_corpse()` initially creates valid strings:

```text
name               = <player> corpse _pcorpse_
description        = The corpse of <article> <race> is lying here.
short_description  = the corpse of <player>
action_description = <player>
```

`sql_save_corpse()` correctly saves the in-memory short and long descriptions. The defect happens when
`sql_load_all_corpses()` reconstructs them after boot.

Once loaded incorrectly, several normal actions save the corpse again. In this incident, taking items caused
corpse rewrites, and `drop corpse` explicitly reached `writeCorpse()` through `src/actobj.c:2230-2235`.
`sql_save_corpse()` then deleted the old row and inserted the shifted in-memory strings
(`src/sql_player.c:7287-7304`). This changed a transient restore bug into durable SQL corruption.

The log line at 19:32:42 already called the restored corpse `100`, before the pasted interaction. The final
drop at 19:40:43 matches the replacement row's current `created_at` exactly.

## 5. Historical origin

The mapping defect predates the current branch:

1. Commit `a2a16faf` (April 22, 2026) added `c.short_descr` and `c.description` to the restore query. At that
   point they were columns 26 and 27, but the new loader read columns 27 and 28. The feature was off by one
   when introduced.
2. Commit `4f6b5fdf` (June 14, 2026) inserted `ci.obj_uid` and `ci.item_condition` before the corpse fields,
   moving the correct corpse columns to 28 and 29. The loader indexes remained 27 and 28. This made normal
   item condition `100` become the corpse short description.
3. The defect remained until `e0c3d79e`, which replaced numeric indexes with named columns and corrected the
   assignments.

Before this branch, no existing test asserted the result-column mapping or a corpse display round trip. The
focused corpse test in `tests/async/test_deferred_findings_repairs.py` covers a different issue: preventing
affect rows belonging to a skipped item from being applied to the previous loaded item.

## 6. Secondary confirmed defect: outer corpse state is not persisted

`make_corpse()` sets the following death-time state (`src/fight.c:1455-1518`):

| State | Field | Examples of consumers |
|---|---|---|
| Contents/body weight | `weight`, `value[CORPSE_WEIGHT]` | carry/drag and carving calculations |
| PC/humanoid/carved flags | `value[CORPSE_FLAGS]` | corpse classification and `do_carve()` |
| Death-time level | `value[CORPSE_LEVEL]` | animate-dead and other necromancy gates |
| Owner PID | `value[CORPSE_PID]` | own-corpse checks |
| Recoverable death XP | `value[CORPSE_EXP_LOSS]` | resurrect XP restoration |
| Race-war side | `value[CORPSE_RACEWAR]` | cross-side artifact rebinding/feed behavior |
| Persistence identity | `value[CORPSE_SAVEID]` | SQL identity and age checks |
| Corpse race | `value[CORPSE_RACE]` | corpse-form/race behavior |
| Owner keywords | `name` | owner-specific object lookup and `_pcorpse_` classification |

The pre-fix `corpses` table stored only `player_name`, `save_id`, `room_vnum`, `created_at`, `short_descr`, and
`description`. It had no outer corpse weight, keywords, flag set, or values 0–7. The contained objects were
fully normalized in `corpse_items`, but the corpse object itself was not.

Before the fix, `sql_load_all_corpses()` read prototype object `#2`, set `ITEM_CORPSE`, ORed in `PC_CORPSE`, set
the save ID, assigned the player name to `action_description`, and placed it in the room. It did not restore
the other state. Prototype `#2` has zero values, generic `corpse` keywords, and weight `200`.

Consequences confirmed from current call sites include:

- `spell_resurrect()` reads `obj->value[4]`; zero produces no recovered XP and the existing
  `MEMORY ERROR: Player corpse with zero exp!` wizlog (`src/magic.c:15707-15742`).
- Necromancy uses `CORPSE_LEVEL`; a restored level-zero corpse is rejected or treated as trivial
  (`src/necromancy.c:430-455` and other level gates).
- `do_carve()` requires `HUMANOID_CORPSE`; restore retains only `PC_CORPSE`, so a previously humanoid player
  corpse becomes uncarvable (`src/new_skills.c:2645-2655`). Any carved-part flags are also forgotten.
- Artifact looting checks the corpse's race-war side before rebinding/feeding the artifact
  (`src/actobj.c:567-580`). Restored `RACEWAR_NONE` skips that branch.
- The prototype weight replaces actual body-plus-content weight, changing carry/drag limits and carving
  output after a restart.

These follow-on behaviors are proven by the save schema, loader assignments, zero-valued prototype, and active
consumers. They were not individually exercised in game during this read-only investigation.

## 7. Recommended repair order

### P0: stop new display corruption

1. Correct corpse display mapping to `row[28]` and `row[29]`.
2. Replace raw numeric indexes with named column constants or a monotonically advanced column cursor.
3. Assert the expected result field count so a query edit cannot silently shift the loader again.
4. Replace prototype-owned strings through the repository's string setters/freeing convention and mark the
   corresponding `STRUNG_*` fields. Reconstruct owner keywords as well as display text.

### P0: preserve gameplay state

Add explicit, additive corpse columns (or a versioned serialized corpse-state record) for at least:

- outer `name`, `weight`, and values 0–7;
- the complete corpse flag value, including humanoid and carved-part bits;
- any decay/preservation state that is intended to survive a restart.

Save and restore those fields in the same transaction as `corpse_items`. Do not infer death-time level, XP loss,
or carved state from the player's current row when exact state is available at corpse creation.

### P0: repair existing rows after deploying the loader fix

The loader correction alone cannot repair rows already rewritten with shifted text. Back up the database, then
repair rows matching strong corruption signatures such as a numeric `short_descr` and a description beginning
with `the corpse of`. Rebuild display strings and owner keywords from `player_name` plus authoritative player
race data where available.

The current local database has exactly one matching row: Amoz's empty corpse. No automatic repair was run as
part of this investigation.

### P1: add regression coverage

Add a database-backed corpse round-trip test with deliberately distinct values:

1. A corpse containing an item whose condition is `37`; assert the item remains condition 37 while corpse
   short/long descriptions remain their own strings.
2. An empty corpse; assert both descriptions survive load and re-save unchanged.
3. Non-default outer weight and every corpse value/flag; assert an exact round trip.
4. Nested contents and item affects; retain the existing skipped-item affect regression.
5. Load followed by loot, drag, preserve, and drop; assert none of those actions mutate corpse identity fields.
6. Restart followed by resurrection and race-war artifact retrieval; assert the death-time metadata is used.

A source-contract test for named indexes is useful as an additional guard, but it is not a substitute for a
real save/load round trip.

## 8. Implementation progress

| Checkpoint | State | Evidence |
|---|---|---|
| Investigation baseline | Complete and published | `e677a046`; live DB and log evidence captured above |
| Display-field reconstruction | Complete and published | `e0c3d79e`; named result columns, field-count guard, owned-string setters, focused contract test, and strict build |
| Outer corpse state | Complete and published | `563a9502`; additive nullable columns preserve keywords, weight, values 0–5 and 7; `save_id` remains canonical value 6 |
| Existing-row repair | Complete and published | `563a9502`; exact owner keywords and PC classification reconstructed; descriptions repaired only for the numeric-short/exact-former-short signature |
| Schema validation | Complete | Isolated MySQL 8 migration/replay test and a production-derived database clone both passed, including double application |
| Full validation | Complete | Clean strict build; `make test-all` 159/159; `make test-db` 3/3; real server load → preserve/save → restart → reload retained every seeded field |
| Local deployment | Complete | `duris_dev` backed up and migrated; Amoz row repaired; service booted successfully on port 7777 and restored `the corpse of Amoz` |

## 9. Completion and legacy boundary

The display mapping, outer-state persistence, migration, and guarded legacy repair are complete and validated.
The production-derived runtime test used deliberately distinct values: weight `913`; values
`[401, 65537, 56, 58, -123456, 3, 1787741833, 55]`; and an extra `corpseprobe` keyword. The real server loaded
them, a preserve spell forced `writeCorpse()`, SQL retained them, and a second boot restored them unchanged.

Historical death-time state that was never stored cannot be recovered exactly. The migration therefore leaves
unknown legacy weight, level, PID, XP loss, race-war side, and race columns NULL instead of inventing values.
It safely reconstructs only facts guaranteed by the table: player-corpse classification and owner keywords.
New corpses store the complete state. Legacy rows continue with safe runtime fallbacks for fields whose
historical values are unknowable; the migration does not claim those values were recovered.

Before applying the migration to local development, two mode-600 SQL backups were created in `/tmp`, including
the final pre-apply snapshot `duris-corpse-persistence-pre-apply-20260826-2018.sql`. No production database was
queried or modified.

# Orphan `player_items` row locks a character out of the game — 2026-08-29

**Status: reader-side fixed and merged (2026-08-29, `5359723fb`, PR #22).** The
load path no longer refuses a character over an inconsistent item row, in either
persistence backend. See [What was fixed](#what-was-fixed) below. The
writer-side question — what created the orphan in the first place — is still
open. [Still open](#still-open) is the register of everything outstanding from
this session; most of it has since been resolved, and the remainder is work that
needs a running game rather than a source change. The document stays until that
register is empty.

A `player_items` row whose `obj_uid` has no matching `item_current_owner` row
made the owning character **permanently unloadable**. The player saw only
"Sorry, I couldn't load that character!", every retry failed identically, and the
account stayed locked out until someone edited the database by hand.

Found while driving the full command list under Valgrind; see
[the sweep notes](valgrind-command-sweep-2026-08-29.md) for that session.
The memory checker is incidental — this is a persistence bug and reproduces
without it.

## Symptom

Character selection accepts the name, prints `Loading character...`, then:

```
Sorry, I couldn't load that character!
```

and returns to the character menu. The only server-side evidence is one
`LOG_DEBUG` line in `logs/log/debug`:

```
player_load_materialize: component=snapshot pid=1 outcome=3 error=0
  repository_component=items queries=14 rows=2037 items=1
free_char called with no name. room: (-1)
```

`outcome=3` is `player_load_outcome::component_failure`; the failing component
is `items`.

## Database state that causes it

For the affected character (`pid=1`):

| Table | Rows |
| --- | --- |
| `player_items` | 2 (`obj_uid` …026 and …027) |
| `item_current_owner` | 1 (`item_uid` …027 only) |

`player_items` row …026 is the orphan: a payload row with no ownership row.

## Mechanism

Line numbers in this section describe the code *before* the fix; see
[What was fixed](#what-was-fixed) for what it looks like now.

`load_items()` in `src/player_load_repository.c:631` selects payload and
ownership together:

```sql
SELECT pi.id, pi.vnum, ..., own.item_uid, own.root_item_uid, own.parent_item_uid,
       own.owner_type, own.owner_id, own.owner_context_id, own.item_revision,
       own.vnum, own.state, owner_revision.revision
  FROM player_items pi
  LEFT JOIN item_current_owner own ON own.item_uid = pi.obj_uid
  LEFT JOIN item_owner_revision owner_revision ON ...
 WHERE pi.pid = <pid> ORDER BY pi.id
```

Because the join is a `LEFT JOIN`, an orphan payload row still comes back — with
every `own.*` column `NULL`. The row parser then does, at
`src/player_load_repository.c:769`:

```c
if (!parse_unsigned(row[31], UINT64_MAX, &identity.item_uid) || ...)
        return false;
```

`parse_unsigned()` returns false for a null pointer
(`src/player_load_repository.c:206`), so the row is rejected. Only `row[33]`
(`own.parent_item_uid`) is null-guarded; the rest are not. A rejected row aborts
`load_rows()`, which fails `load_items()`, which fails the `items` stage, which
sets `component_failure` — and `valid_snapshot()` refuses any result whose
outcome is not `applied`. The load is refused before it ever reaches the
`authoritative_item_count != item_identities.size()` check in
`src/player_load_materialize.c:106`.

The failure is deterministic: the same rows produce the same rejection on every
attempt, so the character never loads again.

**A tolerance path already exists and this case misses it.** Rows whose owner
does not match the loading player, or whose custody state is not `active`, are
*not* fatal — they are counted into `result->stale_item_rows`, skipped, and
logged as recoverable (`src/player_load_repository.c:796-815`,
`src/player_load_materialize.c:374`):

```
player_load_materialize: component=items pid=<pid> outcome=stale_rows_skipped
  count=<n> recovery=next_full_save
```

An item row with *no* ownership row at all falls outside that branch — it dies
in the parser one step earlier — and is treated as corruption of the whole
component rather than as one skippable row.

## How the orphan was created

In the observed session the character:

1. created two objects with the staff `load obj` command,
2. picked both up,
3. destroyed one with `junk` and one with `purge`,
4. saved, and quit.

The result was two `player_items` rows and one `item_current_owner` row.

**The original reading of this — that destruction removed the ownership row —
is wrong.** The ownership row was never created. Traced 2026-08-29:

- `item_current_owner` is written from exactly one place, the transfer pipeline
  in `src/item_transfer_repository.c:281`. Nothing else inserts into it.
- `read_object()` allocates a fresh `obj_uid` (`src/db.c:2745`) and registers no
  ownership at all.
- `do_load` puts the new object straight into the wizard's inventory with
  `obj_to_char()` (`src/actwiz.c:5297`) and submits no transfer. The object is
  now carried, has a uid, and has no ledger row.
- `get`, `put` and `drop` all go through `item_movement_transaction_submit()`
  (`src/actobj.c:477`, `:502`, `:525`), which is what creates the ledger row.

So the object that stayed in inventory from the moment it was created (…026)
never had an ownership row, and the one that reached the floor and was picked
back up (…027) got one from the `get`. `extract_obj()` never touches the ledger
either (`src/handler.c:2994`), so `junk` and `purge` did not remove anything —
they only removed the live object, leaving the save to write the payload row for
whichever one the character was still carrying.

That makes the trigger *object creation without an ownership record*, not object
destruction, and it means any path that hands a player an object without a
transfer produces the same orphan.

A full save rewrites the whole payload side (`DELETE FROM player_items WHERE
pid=…` followed by re-inserts, `src/player_snapshot_repository.c:434`), so the
orphan row was written *by* a save: the character's inventory held an object the
ledger had never heard of.

**Scope is bounded but not enumerated.** The question is no longer "which
destruction path drops the ledger row" but "which paths put an object into a
player's inventory without submitting a transfer". `do_load` is one, confirmed.
There are 10 `item_movement_transaction_submit()` call sites in the whole server
(`actobj.c` 5, `actwiz.c` 3, `handler.c` 1, `fight.c` 1) against 271
`obj_to_char()` calls across `src/*.c`, so the audit is a real piece of work
rather than a glance. Mob loot, corpse looting, quest and reward grants, and shop
purchases each need checking. Tracked as
[Still open item 2](#still-open).

## Repair (no longer required to unlock a character)

A character with an orphan row now loads on its own, and the next full save
rewrites the payload table and clears the orphan. The manual repair below is
still the way to inspect or remove one deliberately.

Development database (`duris_dev`) only. Find orphans:

```sql
SELECT pi.id, pi.pid, pi.vnum, pi.obj_uid
  FROM player_items pi
  LEFT JOIN item_current_owner own ON own.item_uid = pi.obj_uid
 WHERE own.item_uid IS NULL;
```

Delete the specific row, pinned by all three identifiers:

```sql
DELETE FROM player_items
 WHERE id = 27702 AND pid = 1 AND obj_uid = 17293822569163705026;
```

After the delete the character loaded normally and survived three further
connect/play/disconnect cycles. Dump the row before deleting it — that copy is
the only record of what the item was.

Do not run this against production without first deciding whether the orphan
represents an item a player should get back; a delete silently destroys it.

## What was fixed

The bug is one instance of a class: *a single inconsistent item row refuses the
whole load, and refusing the load locks the player out for good.* Every
instance of that class found in the load path was fixed the same way — the
ownership ledger is authoritative, the bad row is skipped or normalized, the row
is counted, and the character loads.

### MySQL backend (`src/player_load_repository.c`)

`parse_item_payload()` now returns a tri-state (`accepted` / `skipped` /
`invalid`) instead of a bool, and `load_items()` calls it rather than carrying a
second, near-identical copy of the same 120 lines.

1. **Payload row with no ownership row** (the reported bug). The `LEFT JOIN`
   leaves every `own.*` column null; that is now detected before parsing and
   routed to the existing `stale_item_rows` skip path.
2. **Pet payload row with no ownership row.** `player_pet_items` joins
   `item_current_owner` the same way, through the same parser, and had *no*
   tolerance path at all. It now shares the skip path, with its own
   `stale_pet_item_ids` set so the row's affects and extra descriptions are
   skipped with it instead of failing the metadata pass.
3. **Pet item owned by someone else, or not in `active` custody.** The
   character's own inventory has tolerated this since the stale-row path was
   added; pet inventories refused the load. They now match.
4. **Ownership row whose payload row is gone** (the inverse orphan). The
   summary query required `missing_count == 0`. The item cannot be rebuilt
   either way, so it is now counted into `result.missing_payload_rows` and
   reported instead of refusing the character.
5. **Contents of a skipped container.** Skipping a container row would have left
   its children pointing at a parent that is no longer in the snapshot, which
   fails parent resolution — the same lockout, one step later. Children of a
   skipped row are promoted to the top level (`parent_item_uid` cleared,
   `root_item_uid` reset), counted in `result.promoted_item_rows`, and the new
   root is propagated down the rest of the subtree.
6. **Owner revision read too eagerly.** `own.item_revision`'s companion
   `owner_revision.revision` was parsed strictly for every row, so a row owned by
   someone with no `item_owner_revision` entry failed before the skip check could
   run. It is now read only for rows that are actually accepted.

### Flat-file backend (`src/flatfile_player_repository.c`)

`build_item_identities()` had the identical class of failure against the
`item_ownership` catalog, which is written once at baseline and never rewritten
by later saves — so a later save can leave the two files disagreeing on its own.
It now compacts skipped items out of the snapshot in place (parents always
precede children there, so one forward pass with an index remap is enough),
promotes the contents of skipped containers, derives `root_item_uid` from the
parent chain, and reports `missing_payload_rows` instead of requiring the
payload and the catalog to match exactly.

### A ceiling on the tolerance

Skipping a row is not a read-only act. A full save rewrites the payload side
outright (`DELETE FROM player_items WHERE pid=…` then re-inserts,
`src/player_snapshot_repository.c:434`), so a row skipped at load is a **deleted
item** at the next save. The logging says `recovery=next_full_save`, but for
skipped payload rows that save is the deletion, not the recovery.

For one bad row that is the right trade against a permanent lockout. Unbounded,
it is not: a systemic ledger fault would silently destroy an entire inventory on
one login, and given [how the orphan is created](#how-the-orphan-was-created) —
every staff-created item a player is holding already has a payload row and no
ledger row — that is a live path, not a hypothetical. Before the fix those items
produced a visible lockout; unbounded skipping would have turned them into quiet,
irreversible loss.

`PLAYER_LOAD_ITEM_SKIP_MAX` (32) caps it. Below the cap the tolerance above
applies unchanged; above it the load is refused, because at that point the choice
is not "lockout versus one lost item" but "lockout versus deleting most of an
inventory", and only the first is recoverable. It is enforced in
`valid_snapshot()` (`src/player_load_materialize.c`), the single point both
backends pass through, and the refusal names the limit it tripped:

```
player_load_materialize: component=items pid=<pid> outcome=skip_limit_exceeded
  count=<n> limit=32 recovery=repair_item_current_owner
```

### Visibility

A refused load was a single `LOG_DEBUG` line. It now also writes `LOG_SYS` and
raises a `wizlog(OVERLORD, …)` naming the pid, because a refusal means a player
cannot enter the game. The three tolerated conditions (`stale_rows_skipped`,
`contents_promoted`, `missing_payload_rows`) are logged with
`recovery=next_full_save`.

The wizlog is throttled to **one alert per character per boot**
(`alert_refusal_once()`). The refusal path is reached once per login attempt and
a locked-out player retries, as does any reconnecting client, so an unthrottled
broadcast would flood the channel with the same line. `LOG_SYS` still records
every refusal as the audit trail.

### Regression coverage

- `tests/async/player_load_repository_mysql_harness.cpp` — orphan payload row,
  orphaned container with contents, pet orphan, pet item reassigned to another
  owner, and custody without payload. Three of these previously asserted
  `component_failure` as intended behaviour; those assertions were the bug,
  written down.
- `tests/async/flatfile_player_repository_harness.cpp` — the same three shapes
  against the flat-file catalog, built by saving a later revision that disagrees
  with the baseline ownership file.

- `tests/async/test_player_load_items.py` — source contracts for the skip cap,
  the named `skip_limit_exceeded` refusal outcome, and the single throttled
  wizlog. Verified to fail when the cap is reverted to `PLAYER_LOAD_ITEM_MAX`.

Run with `bash tests/async/run_player_load_repository_mysql.sh`,
`python3 tests/async/test_flatfile_player_repository.py` and
`python3 tests/async/test_player_load_items.py`.

### What was deliberately left strict

Value *disagreements* between the payload row and the ledger still refuse the
load: a custody row whose `vnum` differs from the payload row's, a payload row
whose `obj_uid` does not match `own.item_uid`, duplicate database ids or uids,
and out-of-range enum values. Those are not missing data — they are two records
that claim to describe the same object and do not agree, and silently picking
one would destroy or duplicate a real item. Only *missing* rows are tolerated.

One instance of the *missing* class is also still fatal, deliberately: an item
whose `serialized_parent_id` names a payload row that is genuinely absent — not
skipped, so not in `stale_database_ids` — fails parent resolution and refuses the
load. That case is unreachable through the schema, because
`fk_player_items_container` is `ON DELETE CASCADE`
(`migrations/bootstrap_multithread_safe.sql:1099`), so deleting a container takes
its contents with it. It is reachable only from a hand-edited row or a database
missing that constraint. The completeness of this fix therefore rests on a
foreign key rather than on the load path — noted in a comment at the branch so it
does not have to be rediscovered.

## Still open

### Start here

| | |
| --- | --- |
| Work lives on | branch `docs-open-register`, PR #23 (`LuminariMUD/DurisMUD`) |
| Base | `master` at `5359723fb` (PR #22, the reader-side fix) |
| State when written | PR #23 open, not merged; full suite 335 passed / 0 failed |

Check whether #23 merged before doing anything: `gh pr view 23 --repo
LuminariMUD/DurisMUD --json state`. If it merged, the "resolved" list below is
in `master` and only the three items here remain. If it did not, read the PR
body first — it explains the changes in more detail than this file does.

**14 of the 19 items in the previous revision are resolved**; what is left is
listed here and nothing else.

### Open

1. **Audit the remaining inventory-granting paths.** `do_load` is fixed (below),
   but any other path that calls `obj_to_char()` without submitting a transfer
   produces the same orphan. The candidate set is wide - 271 `obj_to_char()`
   calls across `src/*.c` against 10 `item_movement_transaction_submit()` sites,
   spread over `specs.object.c` (24), `actinf.c` (20), `specs.mobile.c` (18),
   `magic.c`, `tradeskill.c`, `shop.c`, `fight.c`, `salvage.c` and more. Reading
   all of them is not the way to do this, so it is now instrumented instead:
   `player_snapshot_capture.c` logs every object written into a player's payload
   that the ownership ledger does not know, with its vnum:

   ```
   player_snapshot_capture: component=items outcome=unowned_object
     uid=<uid> vnum=<vnum> recovery=audit_grant_path
   ```

   To collect it:

   ```bash
   ./scripts/cycle_mud.sh --minimal        # or ./scripts/start_mud.sh for the full world
   # log in with GAME_ACCOUNT_NAME / GAME_ACCOUNT_PASSWORD / GAME_ACCOUNT_CHARACTER_NAME
   # from .env, then exercise grants: buy, loot a corpse, complete a quest,
   # craft, salvage, get a mob drop. Save (`save`) after each - the detector
   # fires at snapshot capture, not at the grant.
   grep unowned_object logs/log/debug
   ```

   Each line's `vnum` identifies the object; `grep -rn "<vnum>" areas/` and the
   command you just ran together name the granting path. Fix each by submitting a
   transfer the way `do_load` now does (see `submit_wizard_load_establish()` in
   `src/actwiz.c` for the same-owner establish pattern).

   **Done when:** a full pass over the grant surface — mob loot, corpse looting,
   quest and reward grants, shop purchases, crafting, salvage — produces no
   `unowned_object` lines, and `./scripts/item_ownership_audit.sh` reports no
   orphan payload rows afterwards.

2. **Coverage this session never reached.** Needs a running game and, for the
   first item, a throwaway database - none of it is resolvable from source.
   - The 75 skipped commands (`shutdown`, `pwipe`, `sql`, `redis`, `switch`,
     `advance`, `ban`, `freeze`, world-reset family).
   - Combat: nothing was killed, so damage, death, corpse and looting paths are
     untested. Corpse looting is also one of the grant paths item 1 must audit,
     which makes these the same piece of work.
   - Copyover, deliberately excluded (`--trace-children=no`).
   - Helgrind and DRD. The Redis presence worker and the save/SQL worker threads
     are the obvious candidates.
   - Whether ordinary mortal item-destruction paths reproduce the orphan; the
     reproduction used staff commands throughout.

3. **The `do_load` ownership establish has not been exercised in a live game.**
   The change is covered by source contracts in
   `tests/async/test_orphan_item_session_regressions.py` and the server boots
   with it, but `load obj` was not driven end to end against a running world.
   It moves object creation onto the async transfer pipeline — the object is no
   longer placed by `do_load` itself but by `wizard_load_completion()` once the
   transfer commits — so this is the one change here that can misbehave without
   any test noticing.

   **The failure mode to watch for:** the wizard types `load obj <vnum>` and
   *nothing appears*. That means the transfer never committed and the completion
   discarded the object (by design — leaving it would recreate the orphan), or
   the completion ran but found the object no longer `OBJ_NOWHERE`. Both paths
   log to `logs/log/file`; look for `wizard load committed but live publication
   was stale`.

   **To verify:**

   ```bash
   ./scripts/cycle_mud.sh --minimal
   # log in as the staff character from .env, then in-game:
   #   load obj <a takeable vnum>     -> should appear in inventory
   #   load obj <a non-takeable vnum> -> should appear in the room
   ```

   Then confirm the ledger row exists, which is the entire point of the change:

   ```sql
   SELECT item_uid, owner_type, owner_id, state
     FROM item_current_owner
    WHERE item_uid = <the new obj_uid>;
   ```

   `owner_type` 1 is player, 2 is room. A takeable object loaded into inventory
   must be owned by the wizard's pid; a non-takeable one by the room's vnum.
   Finally `save`, quit, and log back in — the character must load, and
   `./scripts/item_ownership_audit.sh` must report no orphan payload rows.

   I got as far as booting the server to do this and ran out of context before
   driving the commands; nothing about the result is known either way.

### Resolved on this branch

- **`deathsdoor` aborted the server** (`specs.gellz.c`). The closing write
  claimed `MAX_STRING_LENGTH` from an advanced destination, so `_FORTIFY_SOURCE`
  aborted the process; any player could type one word and kill the server. Now
  sized by the room actually left, and it only trims a separator it wrote - a
  character with every base stat at 100 no longer backs over its own header.
- **20 further instances of that same defect**, found by extending
  `scripts/scan-append-bounds.py` to catch the shape that hid this one: an
  offset destination with a bare capacity constant, rather than a subtracted
  size. Fixed across `utility.c` (8), `actwiz.c` (6), `actobj.c` (2),
  `guild.c`, `specs.room.c`, `storage_lockers.c` and `tradeskill.c`. Two of them
  were worse than the pattern suggested: `guild.c` advanced into `buf2` by
  `strlen(buf)` - the wrong buffer entirely - and `tradeskill.c` claimed 65536
  bytes of a 1024-byte buffer.
- **`do_load` establishes ownership.** A wizard-created object is now submitted
  as a same-owner `creation` transfer and moved into place by the completion, so
  it can no longer arrive carrying a uid with no ledger row. An uncommitted
  transfer discards the object rather than leaving the orphan behind.
- **`generate_modif()` and `generate_desc()` leaks.** The generators' returns are
  owned and released, rejected candidates are freed, and the replaced short
  description is freed rather than dropped.
- **`free_char()` leaked `long_descr`** on the player path - the one player
  string the branch never released, leaked once per login that set one.
- **`do_build()` leaked** the `Building` it declined to keep.
- **`extract_obj()` and the ledger**: decided and recorded in the code. It stays
  out of the ledger deliberately, because extraction runs on teardown paths where
  a transaction is impossible; the load path tolerates the stale custody row.
- **Boot descriptors are by design**: `mob_f` and `obj_f` are held open for the
  process lifetime because `read_mobile()` and `read_object()` `fseek` into them
  on every instantiation. Recorded at the `fopen` site so the next Valgrind
  session does not re-file it. The boot-time one-shot allocations are world data
  that lives as long as the process.
- **`scripts/format.sh --check` no longer misses whole-file drift** in files the
  change touched - the gap that let this branch go green locally and red in CI.
- **`scripts/valgrind_mud.sh` gained `--minimal` and `--server-arg`**, so the
  session is reproducible from the checked-in wrapper.
- **A maintenance sweep exists**: `scripts/item_ownership_audit.sh` reports
  orphan payload rows, inert ledger rows, and any character over the load-time
  skip cap. Read-only. This is the only way to see the item loss the skip path
  now absorbs silently.
- **The raw Memcheck logs are removed** from `docs/ongoing-projects/`.
- **`shutdown ok` cancelling on issuer disconnect** is intentional and already
  documented at `timedShutdown()`; no change.

### Regression coverage added

- `tests/async/test_orphan_item_session_regressions.py` - the `do_load`
  establish, the `extract_obj` decision, both generator leaks, the `long_descr`
  free, `do_build`, the boot-descriptor rationale, and the save-time detector.
- `tests/async/test_append_bounds.py` - the `deathsdoor` site specifically, plus
  a self-test that feeds the scanner the offset-destination shape and requires a
  report, so the detector cannot silently regress to missing it again.

## Affected code

- `src/player_load_repository.c` — `parse_item_payload()` (tri-state row parse
  and ownership tolerance), `load_items()` (skip path, container promotion),
  `load_pets()` (pet skip path and metadata tolerance), the ownership summary
  query (`missing_payload_rows`)
- `src/player_load_repository.h` — `missing_payload_rows`, `promoted_item_rows`,
  `PLAYER_LOAD_ITEM_SKIP_MAX`
- `src/player_load_materialize.c` — `valid_snapshot()` bounds including the skip
  cap, `alert_refusal_once()`, the refusal log lines, and the
  tolerated-condition log lines
- `src/flatfile_player_repository.c` — `build_item_identities()`,
  `reconcile_item_ownership()`
- `tests/async/player_load_repository_mysql_harness.cpp`,
  `tests/async/flatfile_player_repository_harness.cpp` — regression coverage
- `tests/async/test_flatfile_shop_trade_repository.py` — source-contract token
  updated for the new `build_item_identities()` signature
- `src/specs.gellz.c`, `src/utility.c`, `src/actwiz.c`, `src/actobj.c`,
  `src/guild.c`, `src/specs.room.c`, `src/storage_lockers.c`,
  `src/tradeskill.c` — the append-bounds class
- `src/db.c` (`free_char`, boot rationale), `src/handler.c` (`extract_obj`
  rationale), `src/buildings.c` (`do_build`)
- `src/player_snapshot_capture.c` — the unowned-object detector
- `scripts/scan-append-bounds.py`, `scripts/format.sh`,
  `scripts/valgrind_mud.sh`, `scripts/item_ownership_audit.sh`
- `tests/async/test_player_load_items.py` — source contracts for the skip cap,
  the named `skip_limit_exceeded` refusal, and the single throttled wizlog

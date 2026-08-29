# Orphan `player_items` row locks a character out of the game — 2026-08-29

**Status: reader-side fixed (2026-08-29).** The load path no longer refuses a
character over an inconsistent item row, in either persistence backend. See
[What was fixed](#what-was-fixed) below. The writer-side question — what created
the orphan in the first place — is still open.

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

The result was two `player_items` rows and one `item_current_owner` row: item
destruction removed the ownership row but left the payload row behind. Nothing
reconciles the two afterwards, and the next save wrote the inconsistent pair.

**Scope is not established.** This reproduction used wizard-loaded objects and
staff destruction commands. Whether the ordinary mortal paths — `junk`,
`sacrifice`, selling to a shop, corpse decay, destruction of a container holding
items — can leave the same orphan has **not** been tested. That question now
decides how often the skip path fires, not whether players get locked out.

A full save rewrites the whole payload side (`DELETE FROM player_items WHERE
pid=…` followed by re-inserts, `src/player_snapshot_repository.c:434`), so the
orphan row was written *by* a save: the character's in-memory inventory still
held an object the ownership ledger had already destroyed. That points at the
runtime destruction path, not at the persistence layer.

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

### Visibility

A refused load was a single `LOG_DEBUG` line. It now also writes `LOG_SYS` and
raises a `wizlog(OVERLORD, …)` naming the pid, because a refusal means a player
cannot enter the game. The three tolerated conditions (`stale_rows_skipped`,
`contents_promoted`, `missing_payload_rows`) are logged with
`recovery=next_full_save`.

### Regression coverage

- `tests/async/player_load_repository_mysql_harness.cpp` — orphan payload row,
  orphaned container with contents, pet orphan, pet item reassigned to another
  owner, and custody without payload. Three of these previously asserted
  `component_failure` as intended behaviour; those assertions were the bug,
  written down.
- `tests/async/flatfile_player_repository_harness.cpp` — the same three shapes
  against the flat-file catalog, built by saving a later revision that disagrees
  with the baseline ownership file.

Run with `bash tests/async/run_player_load_repository_mysql.sh` and
`python3 tests/async/test_flatfile_player_repository.py`.

### What was deliberately left strict

Value *disagreements* between the payload row and the ledger still refuse the
load: a custody row whose `vnum` differs from the payload row's, a payload row
whose `obj_uid` does not match `own.item_uid`, duplicate database ids or uids,
and out-of-range enum values. Those are not missing data — they are two records
that claim to describe the same object and do not agree, and silently picking
one would destroy or duplicate a real item. Only *missing* rows are tolerated.

## Still open

1. **Root-cause the writer.** A save serialized an object the ledger had already
   destroyed. Reproduce with `load obj` → get → `junk` / `purge` → save, and find
   where the object survives in the character's inventory list after its custody
   row is gone. Not attempted here; it needs a live session.
2. **Determine the blast radius.** Test whether mortal destruction paths —
   `junk`, `sacrifice`, selling to a shop, corpse decay, destroying a full
   container — leave the same orphan. This no longer decides whether players get
   locked out; it decides how much silent item loss the skip path is absorbing.
3. **A maintenance sweep** reporting orphan payload rows is now optional rather
   than urgent: a character with an orphan loads, and its next full save removes
   the row. It would still be useful for spotting item loss.

## Affected code

- `src/player_load_repository.c` — `parse_item_payload()` (tri-state row parse
  and ownership tolerance), `load_items()` (skip path, container promotion),
  `load_pets()` (pet skip path and metadata tolerance), the ownership summary
  query (`missing_payload_rows`)
- `src/player_load_repository.h` — `missing_payload_rows`, `promoted_item_rows`
- `src/player_load_materialize.c` — `valid_snapshot()` bounds, the refusal log
  line, and the tolerated-condition log lines
- `src/flatfile_player_repository.c` — `build_item_identities()`,
  `reconcile_item_ownership()`
- `tests/async/player_load_repository_mysql_harness.cpp`,
  `tests/async/flatfile_player_repository_harness.cpp` — regression coverage
- `tests/async/test_flatfile_shop_trade_repository.py` — source-contract token
  updated for the new `build_item_identities()` signature

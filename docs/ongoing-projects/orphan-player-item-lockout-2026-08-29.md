# Orphan `player_items` row locks a character out of the game — 2026-08-29

A `player_items` row whose `obj_uid` has no matching `item_current_owner` row
makes the owning character **permanently unloadable**. The player sees only
"Sorry, I couldn't load that character!", every retry fails identically, and the
account stays locked out until someone edits the database by hand.

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
items — can leave the same orphan has **not** been tested, and that is the
question that decides how serious this is in production.

## Repair

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

## Suggested follow-ups, in order

1. **Determine the blast radius.** Test whether mortal item-destruction paths
   produce the orphan. If they can, this is a live player-lockout bug.
2. **Fix the writer.** Find the destroy/save path that removes the ownership row
   without removing the payload row, and make the two consistent.
3. **Make the reader survivable.** Route a payload row with no ownership row
   into the existing `stale_item_rows` skip-and-log path instead of failing the
   whole component. A character should not be locked out by one bad row.
4. **Make the failure visible.** "Couldn't load that character" is `LOG_DEBUG`
   today; staff will not see it. It deserves a level that surfaces.
5. **Add a regression test.** Save a character holding an item, destroy the
   item, save again, reload — assert the character loads.
6. **Consider a boot-time or maintenance sweep** that reports orphan payload
   rows, so an existing corrupted character is found before its owner tries to
   log in.

## Affected code

- `src/player_load_repository.c:631` — `load_items()` and its query
- `src/player_load_repository.c:769-786` — unguarded `own.*` column parsing
- `src/player_load_repository.c:796-815` — the `stale_item_rows` tolerance path
- `src/player_load_repository.c:204-215` — `parse_unsigned()` null handling
- `src/player_load_materialize.c:31-118` — `valid_snapshot()`
- `src/player_load_materialize.c:343-380` — the refusal log line

# Saved ground item recovery handoff

This protocol applies to the MariaDB-primary saved ground item path. The flatfile-primary room catalog has its own transaction and replay rules. The restored object is live game state; saved_items is the complete durable payload until a replacement generation and a saved_item_recovery_handoff receipt commit. Redis world and floor generations are independent snapshots and do not acknowledge SQL source retirement.

## Authority and sequence

1. Boot reads each saved_items root and its descendants. It verifies the full set of source row IDs, one root, parent links, prototype availability, room ownership, nested placement, UIDs, affects and extra descriptions. A malformed or incomplete graph stays in SQL and is not published.
2. The validated graph is placed in its room. Existing root/child UIDs are retained. Boot tracks published UIDs and never publishes a second SQL root containing one of them.
3. For legacy pointer-derived keys, a SQL transaction locks the exact source row set, checks that a stable UID key is unoccupied, inserts a complete replacement graph under that key, and commits a handoff receipt naming the source root ID, the SHA-256 digest of its sorted row IDs, and the replacement root ID. A failed insert rolls the replacement and receipt back together.
4. A separate transaction checks the receipt, exact source row ID digest and parent graph under row locks, deletes only the named source root (its descendants cascade), and marks the receipt retired. If this step fails or the process stops, the receipt and both generations remain. The next boot chooses the newer replacement and retries retirement only if the original source generation still matches.
5. Rows already using the stable UID key stay durable across boot. Gameplay saves/deletes use that same key, so later updates do not depend on a process address.

The receipt is durable proof of a complete replacement, not a signal that an item changed custody. The item_current_owner ledger remains the custody authority. A conflicting owner, duplicate UID, missing prototype, malformed link or incomplete source keeps its source rows for reconciliation and does not become a second live item.

## Failure boundaries

| Stop or failure | Durable state | Next boot |
| --- | --- | --- |
| Before materialization or after staging | Original source only | Revalidate and publish once |
| After live publication, before acknowledgment | Original source only | Revalidate and publish once |
| During replacement insert or receipt insert | Transaction rolls back; original source only | Retry complete handoff |
| After acknowledgment, before retirement | Original and replacement plus receipt | Publish replacement once, retire named source |
| During retirement | Delete and retired marker roll back together | Retry named source retirement |
| After retirement commit | Replacement plus retired receipt | Publish replacement once |
| Malformed source or authoritative custody conflict | Original source retained without publication | Retry only after repair or reconciliation |
| Concurrent application save replaces the source row IDs | Newer source retained; stale handoff attempt aborts | Replay newer graph, with no blanket deletion |
| A child is replaced after acknowledgment with the same root and row count | Replacement remains published; changed source is not retired | Keep one live graph and retain changed source for reconciliation |

The local-only DURIS_SAVED_ITEM_RESTORE_FAULT_STAGE and DURIS_SAVED_ITEM_RESTORE_PAUSE_STAGE fixture controls exercise these boundaries. They are ignored outside ENVIRONMENT=local. The pause is capped at five seconds.

## Migration and rollback

Immutable migration 0027_saved_item_recovery_handoff changes the saved_items item_key index from unique to nonunique so one root and its nested children can share a key. It creates the receipt table and preserves all existing rows. The migration SQL and verifier are rerunnable after interrupted DDL. Runtime compatibility inventory, history head and measured MySQL 8/MariaDB 10.11 schema fingerprints move with it.

Apply it only through the guarded migration runner after a verified backup on the intended target. To roll back an application release, first stop all writers and retain the receipt table and nonunique index. The prior runtime's boot-wide DELETE is unsafe against data written by this protocol, and restoring the unique index fails while nested rows share a key. Validate a backup on an isolated clone and either roll forward with a corrected runtime or restore that full pre-migration backup under the repository's recovery procedure. Do not drop receipts, delete source rows manually, or run a reverse ALTER on a live target. Production deployment and data repair require separate authorization.

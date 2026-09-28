# Player death-conflict evidence schema

Migration `0034_player_death_conflict_evidence` adds one InnoDB table for
first-observed evidence from a rejected player death-save conflict. The row is
keyed by operation identity and uniqueness-guards both `(pid, save_revision)` and
`(pid, corpse_item_uid)`. The request and payload hashes accompany the opaque v10
payload and its source revision.

This is forensic evidence only. The payload is not loadable inventory and is not
a death-save success receipt. Replays must compare the original `request_hash`
and must not replace the first observation. Evidence remains unresolved; any
future resolution belongs in a separate record. The table has no foreign keys or
cascades, so player deletion cannot erase it. There are no update, reset, replace,
or purge paths in this schema slice. Account activation remains unchanged and
NULL.

The additive migration uses `CREATE TABLE IF NOT EXISTS`; it does not repair or
bless an existing malformed table. The migration runner's exact verifier rejects
schema drift, while the runtime contract pins the table inventory and measured
MySQL 8.0 / MariaDB 10.11 structural fingerprints. Fresh bootstrap creates the
same table definition. This registration does not activate a capture or read
path.

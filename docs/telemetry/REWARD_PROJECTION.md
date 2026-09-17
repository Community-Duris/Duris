# Committed reward projection — #270

This document defines the analytics projection for rewards that have already
committed in the authoritative ledgers. It is an external, bounded worker
contract. It does not make telemetry authoritative, add a gameplay write, or
turn a live notification into a reward record.

## Authority and identity

`currency_ledger`, `epic_ledger`, and `combat_frag_ledger` are the amount
authorities in the first projection. Zone, PvP, and boon outcome tables are
read-only context;
their parent and participant rows are retained for coverage and lineage but
never added to reward totals. The old `world_quest_accomplished` and
`quest_trophy` tables do not contain a committed `operation_id`, so the worker
does not invent one. Their historical context remains unknown until a reviewed
authoritative source exists.

Every projected row has the following source identity:

```text
(source_kind, operation_id, entry_index, participant_pid)
```

Parent and participant sources have distinct `source_kind` values. This is
important when participant index zero has the same PID as the parent actor: a
parent row cannot collide with its first participant. The projection also
stores a payload digest. A retry with the same identity and digest is an
idempotent replay; a different payload for an existing identity is stored as a
fail-closed conflict with no amount.

`economic_operation_id` links context to a known ledger operation when a
trusted adapter has that relationship. A context row without that link has
status `unknown_context`, not a guessed amount. Parent/child outcomes and
participant receipts therefore cannot double-count a committed ledger row.

## Reward semantics

All amounts are signed integer units in the domain's native smallest unit.
Currency uses the game's canonical weights: copper = 1, silver = 10, gold =
100, and platinum = 1,000.

| Source | Gross | Net | Transfer | Creation |
| --- | ---: | ---: | ---: | --- |
| Currency ledger | `max(net, 0)` | weighted wallet + bank delta | zero-net wallet/bank movement | positive net only |
| Epic ledger | `max(delta, 0)` | committed `delta` | not applicable | positive delta only |
| Combat frag ledger | `max(delta, 0)` | committed `delta` | not applicable | positive delta only |
| Outcome context | none | none | none | descriptive only |

A wallet-to-bank movement with equal and opposite deltas has zero net reward
and is marked `transfer_not_creation`. A negative ledger delta is a sink, not a
negative creation. A positive ledger delta is counted exactly once even when a
live notification or outcome row describes the same gameplay result. The
separate transfer metric is visible to reports but is excluded from gross and
net creation totals.

## Fast processing and reconciliation

Each supported source adapter uses an explicit column list and a keyset query:

```sql
WHERE source_created_at <= :fixed_high_water
  AND (created_at, operation_id, entry_index, participant_pid) > :cursor
ORDER BY created_at, operation_id, entry_index, participant_pid
LIMIT :bounded_page_size
```

The implementation expands the tuple comparison for MySQL/MariaDB and binds
all values. It does not use `SELECT *`, `OFFSET`, `FOR UPDATE`, or a gameplay
connection. Joined context adapters qualify every ordering key. A zero
operation ID is an inclusive lower-bound sentinel for the reconciliation
floor; it is never accepted as a source identity.

The fast window covers recent source creation time and advances independently
of a reconciliation cycle. A fixed high-water timestamp makes a page stable
while later commits are left for a later page or cycle. A reconciliation cycle
starts at the retained source floor, uses the same fair keyset order, and
records:

- cycle ID and high-water timestamp;
- fast and reconciliation cursors;
- retained floor and acknowledged-through timestamp;
- backlog row count and quality flags;
- provisional state and start/completion timestamps.

The worker defaults to a 256-row page, permits at most 2,000 rows per page,
uses one private connection, and permits at most eight SQL statements in one
page transaction. Projection writes are one bulk `INSERT ... ON DUPLICATE KEY
UPDATE` statement. Equal payloads remain one row. A conflicting payload marks
the identity as conflict, clears amounts, and preserves the evidence bits.

The operation ID is not a commit-order cursor. The timestamp is a scan key,
not an assertion about worker commit order. For example, if a low-sorting
operation ID commits after a forward scan has passed its timestamp position,
the next bounded reconciliation sweep can find it. The overlap is finite and
the cycle remains provisional until the retained range is fully acknowledged;
the worker never claims that the overlap alone is perfectly complete.

## Retention, freshness, and incompleteness

Source pruning must not advance past `acknowledged_through`. Before a source
retention floor is moved, the state row must show a completed reconciliation
through that point. Reports surface these conditions instead of silently
turning missing rows into zero:

- `reconciliation_acknowledgement_behind_retention_floor`;
- `projection_cycle_provisional`;
- `reconciliation_backlog_nonzero`;
- `source_range_incomplete`;
- `unknown_context` and identity conflict quality flags.

The fast projection can be fresh and still provisional. A source-created row
may arrive after an outcome notification, after the current fast high-water,
or during a connection failure. The projection is therefore suitable for
freshness-aware analytics, not an immediate gameplay decision. A failed page
does not advance its cursor or acknowledgment. A lost commit acknowledgment
requires the caller to reread state and retry the same identity/payload; it is
not safe to blindly advance the cursor.

The initial retention policy is operationally explicit rather than guessed:
the source owner must retain at least the configured reconciliation horizon,
including the finite overlap, and must expose the actual floor to the report.
If a source cannot provide a retained range or a trustworthy created time, the
worker reports incomplete/unknown context and does not synthesize a reward.

## Report definition

`REWARD_REPORT_DEFINITION` and `build_reward_report_query` define a read-only
aggregate at `(source_kind, reward_kind, status)` grain. The report exposes
gross, net, transfer, conflict, unknown-context, source-count, min/max source
time, and quality flags, together with all projection state rows. The report
range is bounded to 31 days and the result group count is capped. A report
consumer must show the state metadata alongside totals; a total without its
cycle, retention, and provisional fields is incomplete.

The report role should have `SELECT` only on the two `telemetry_reward_*`
tables. The projection worker role needs `SELECT` on the listed source
tables and write access only to the two projection tables. Neither role needs
the critical outbox delivered-state columns. Deployment grants are external
to this change; the CLI never creates users, grants, tables, or migrations.

## Failure and lifecycle behavior

Projection transactions are short and contain no source row locks. A rollback
leaves both projection rows and state unchanged. A successful commit advances
the state only after the bulk upsert has been accepted. Re-running a committed
page is safe because the source identity is the primary key and the payload
digest distinguishes replay from corruption. The worker's single connection
is closed on an ambiguous commit so the next attempt must establish a new
session and reread state.

The migration is additive and rerunnable. Fresh bootstrap includes the same
tables, while migration `0023_telemetry_reward_projection` makes the durable
schema available to the existing baseline. Lifecycle inventory and runtime
compatibility metadata cover both tables. No production migration, retention
prune, rollout, or automatic balance change is part of #270.

# Native SQL source snapshots

`economic_sql_capture_sources` captures owning native evidence for baseline
preparation from a dedicated MySQL 8.0 or MariaDB 10.11 connection. It preserves
selected SQL cells exactly, rather than constructing accounting lifetimes,
opening entries or monetary totals. There is no production operator command or
activation caller in this increment.

## Transaction and authority

The connection must be idle, in autocommit mode, with automatic reconnect disabled.
The caller supplies finite connect/read timeouts. The capture sets only its next
transaction's isolation to REPEATABLE READ, starts one consistent READ ONLY
snapshot and opens every required table before reading metadata or contents.
Metadata locks are retained through rollback. Every source must be an InnoDB base
table; missing tables, views, nontransactional engines and unsupported server
versions refuse capture.

All source counts, lengths and selected rows use this same read view. Native
writers can continue committing; capture does not drain the coordinator, publish
pending results or establish the frozen boundary required by the baseline writer.
It never changes source rows, reserves identities, creates missing structures,
recovers receipts or activates accounting. It refuses an existing caller
transaction without rolling it back. A failed START reply is treated as possibly
having started a transaction and triggers cleanup.

A successful capture requires rollback acknowledgement and unchanged connection
identity. Output is replaced only after that point. Allocation, SQL, capacity,
connection or rollback failure returns an error and preserves the previous output.
The caller should discard its dedicated connection after any failure, including
uncertain session/rollback state. Client-free builds refuse with ENOTSUP.

## Selected source contract

The implementation's ordered `sources` registry is the authoritative field list.
All rows, including empty, zero-valued, offline, quarantined and historical rows,
are retained without a reachability filter. Each source is ordered by its native
primary key, including composite owner and auction-custody keys.

| Sources | Retained evidence |
| --- | --- |
| player_data | PID, account/racewar locator, exact denominations, wallet/save revisions |
| account_banks | Bank row ID, shared account/racewar locator, unsigned denominations and revision |
| ships | Row ID, native owner locator, nullable signed coffer money |
| auctions | Listing/seller/bidder IDs, state, prices, quantity, revision, custody state, listing operation, vnum and blob |
| auction_money_pickups / auction_item_pickups | Current aggregate money claims and revisions; legacy item claims, retrieval flag, quantity and blob |
| auction_item_custody | Listing/slot/UID, revision/vnum/blob, claim identity and claimed-presence flag |
| collector_catalog_state / collector_deaths / collector_listings | Native catalog/next-listing state, death policies and identities, complete listing identity/state/revisions/value and binary records |
| item_current_owner / item_owner_revision / item_uid_allocator | All custody states/topology, coin payloads, owner revisions and allocator state |
| Three reconciliation quarantines | Native IDs, conflict/evidence fields and repaired-presence flag |
| critical_operation_inbox / critical_outbox | Original operation identity/hashes/versions, result/payload/status, revisions, publication flags, attempts and error codes |

SQL NULL, empty bytes and zero are distinct. Cells can contain embedded NUL or
non-text binary bytes. Negative signed values and full unsigned 64-bit values
remain evidence, not parser defaults or wrapped/narrowed numbers. Character fields
are selected as BINARY to preserve their stored encoding rather than converting
them through the client's character set. No login credentials or player profile
text is selected. Evidence still contains private native locators, amounts and
payloads, and must not be printed or posted publicly.

This is explicitly a projection of named columns, not a whole-table backup.
Clock timestamps are omitted; selected completion/repair timestamps contribute
only an explicitly named `IS NOT NULL` flag. Auction deadlines, full physical
inventory projections, historical ledgers, prototypes, runtime state, complete
custody normalization and domain-specific receipt validation remain separate
requirements. A complete digest here does not prove complete economic coverage.

## Digests and bounds

SHA-256 framing uses unsigned little-endian 64-bit lengths/numbers. Text is its
length followed by bytes; fixed digests are 32 raw bytes. The definition digest
binds `ESD1`, table name, primary-key ordering, column count and every selected
column/expression. Each row digest binds `ESR1`, its definition digest and each
cell's 0/1 NULL/present tag plus length-framed bytes when present. A table digest
binds `EST1`, its definition, row count and ordered row digests. The snapshot digest
binds `ESM1`, table count and ordered table digests. Row hashes include native keys
but not row ordinals, so insertion of a different row does not change an existing
row's own digest. These digests bind the captured selected values; they neither
fingerprint complete DDL nor authenticate a manufactured/mutated DTO.

Limits default to and cannot exceed 262,144 total rows, 4,194,304 cells, 64 MiB of
cell bytes and 1 MiB per cell. Zero limits refuse. Each source's row count, byte
sum and largest cell are measured in the same snapshot before allocating/fetching
its payload. Results stream one row at a time; no full client result buffer is
used for source contents. Exact measured row/byte totals are checked after reading.
There is no truncation, continuation from another snapshot or partial success.

Cell bytes exclude DTO/vector/string allocator overhead and client working memory;
row/cell bounds independently constrain those allocations. The aggregate length
scans still perform database work. These resource limits are not measured latency,
contention, storage growth or production release qualification.

## Native normalization

`economic_sql_validate_sources` is a client-free integrity check for the owning
capture. It verifies the exact versioned registry, column expressions, limits,
counts, and all definition/row/table/snapshot hash framing before a consumer parses
cells. Matching hashes are not server authentication or a cutover capability.

`economic_sql_normalize_sources` is a pure typed consumer. It retains a source
reference (table, row, digest) for each holding, custody row and owner revision;
the original capture must remain available beside the report. Whole-table defects
such as a missing allocator use the table digest and a `SIZE_MAX` row reference.
Native IDs remain locators, not accounting lifetime IDs. It produces no aggregate
monetary total, account mapping, opening entry or activation decision.

- Wallet denominations stay signed; shared banks are read once per bank row and
  retain their unsigned native evidence. SQL NULL and unsigned amounts above
  `INT64_MAX` leave the normalized balance unavailable, with explicit diagnostics.
  Known negative components are still diagnosed even when another component is
  unknown. Exact vectors also undergo checked copper-value arithmetic.
- Ship coffers retain their signed scalar value; the absence of a native revision
  is explicit. Aggregate auction money claims retain their own balance/revision.
  CLOSED listing prices are historical, funded OPEN listings are current escrow,
  and funded REMOVED or unknown listing states remain unresolved. Unfunded OPEN
  and REMOVED listings are not current money holdings.
- Custody keeps existing UIDs, owner, root/parent, revision and state. Native owner
  revisions and the allocator's exclusive next-UID boundary are retained. Invalid
  custody fields, missing owner revisions, allocator defects and quarantined rows
  remain diagnostics rather than synthesized repairs.
- Present coin payloads use the production item snapshot codec and require one
  matching UID/vnum/ITEM_MONEY item. Each valid payload contributes one pile vector
  keyed by its existing UID; active, destroyed and quarantined piles remain
  current, historical and unresolved respectively. Empty, malformed and mismatched
  payloads are invalid. SQL NULL on an active row leaves monetary classification
  unknown; this does not declare every such row a coin object. Prototype and
  physical-projection reconciliation must settle that distinction.
- Open quarantine rows, incomplete receipts, unpublished/dead outbox rows and
  unretrieved legacy item claims stay explicit. A completed rejected receipt is
  not incomplete solely because its result code is nonzero. This is status
  screening, not command-specific receipt or required-publication validation.

Input limits are the hard capture limits. Stored diagnostic detail is bounded to
1..512 entries while all issue counts remain complete. Structural/hash/canonical
numeric errors and allocation failures preserve the caller's previous output.
Native value contradictions produce a report with diagnostics, not partial
success disguised as a clean baseline. The APIs perform no SQL or native writes.

Full custody graph consistency, auction/collector and physical projections,
retained operation semantics, lifetime mapping and the drained per-domain boundary
remain required before baseline preparation or activation. A successful typed
report alone proves none of those requirements.

## Verification

`tests/async/run_economic_sql_source_snapshot_mysql.py` creates a unique disposable
schema under explicit loopback test guards, loads the current bootstrap and builds
the real native source under ASan/UBSan, plus a client-free validation/refusal binary. The
harness uses actual connections for concurrent writes, DDL locking, session
ownership and native source reads. Final results and fault-injection counts are
reported with this increment only after completion. No production execution is implied.

The combined disposable accounting runner also invokes this focused suite.

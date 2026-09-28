# Economy accounting audit operations

The read-only reconciler is `scripts/reconcile_economy_accounting.py`. It accepts
one bounded, consistent snapshot of native authority and immutable evidence. It
never connects to a writer, posts an adjustment, or changes a balance or UID.
Exceptions mean **pause the affected writer/epoch and investigate**; a zero
exception count is meaningful only when the snapshot is complete and all route
coverage gates are independently qualified.

## Snapshot contract, version 1

Supply a UTF-8 JSON file of at most 32 MiB. Set `complete: true` only after
enumerating every row in the selected lineage and every admitted native holding
and UID without truncation. Set `quiescent: true` only for one consistent native
and evidence cut: a SQL read-only repeatable-read transaction or a flatfile
authority-locked copy after journal recovery. A restore that lost a segment,
receipt, opening witness, source claim, or native class must set `complete:
false`, and the audit reports `evidence_loss`. Neither flag may be inferred from
an empty directory or an absent row. Both flags are part of the exporter's
attestation; the JSON parser cannot independently authenticate them.

Top-level fields are `schema_version`, `lineage`, `epoch`, `backend`, `complete`,
`quiescent`, `native`, `account_origins`, `item_origins`, `operations`, `effects`,
`postings`, `children`, `item_references`, `ownership_events`, `source_claims`,
and `receipts`. Every collection is required and limited to 100,000 rows. A
source snapshot larger than either limit needs a reviewed partitioning method;
truncating and setting `complete: true` is prohibited.

The partial SQL exporter also supplies `native_mapping_coverage`. It counts
all `player_data` and `account_banks` rows, rows with no active SQL mapping in
any lineage, native rows with multiple active mappings in the selected lineage,
selected-lineage mappings whose native row is missing, and selected-lineage
mappings to rows with null balance or revision fields. The reconciler emits
coded, bounded exceptions for nonzero anomaly counts. These are database-wide
diagnostic counts; an
unmapped legacy row is a candidate for investigation, not proof that its
balance belongs to the selected epoch. The remaining native classes and their
lineage scope still require independent enumeration before `complete: true`.

Its `source_claims` collection covers nonbaseline claims across the selected
lineage, including claims from other epochs. The SQL rows carry their owning
operation's lineage, epoch, source and outcome so the reconciler can distinguish
a valid earlier-epoch claim from an orphan. `source_claim_coverage` counts all
committed nonbaseline operations in that lineage with a nonnull source event,
missing exact claims and source values reused by multiple such operations.
The reconciler emits coded counts for the latter two. This does not yet prove
that every other-epoch operation required a source event under its policy, and
baseline source claims are outside this collection.

- `native.holdings`: each admitted **ordinary** account key, current four-value
  denomination vector and native revision. Wallet and bank keys use retained
  mapping lifetime IDs; a pile key uses its UID. Escrow, pending claims, and
  treasuries require real durable lifetime identities and native balances.
- `account_origins`: the same version-1 40-byte account key (lowercase hex), initial
  vector/revision, and `origin: baseline` from an exact EAB1 witness or
  `origin: creation` with zero vector and revision zero from an authenticated
  first account operation. An unknown legacy opening is reported, never
  silently treated as zero. A retired mapping carries `retired_by` with the
  committed retirement root ID, must have a zero terminal balance, and must
  have no current native holding. Retained history for that key remains in the
  snapshot even after its native row is removed.
- `native.items`: each admitted live or tombstoned UID, current revision,
  root UID, nullable parent UID, numeric owner triple, and state. Alias/name
  fields are unnecessary. `item_origins` supply the opening position with
  `origin: baseline`, or a normalized `origin: creation` absence marker at
  revision zero (`root = uid`, no parent, owner `[0,0,0]`, state `absent`).
  That marker must be followed by an exact referenced creation event; it is
  not an assertion that a real native row existed before creation.
- `operations`: immutable root ID, lineage, epoch, numeric reason, committed or
  rejected outcome, result code, source event when required, exact row counts,
  optional original operation ID, and optional **realized** price in copper.
  Estimated inventory values do not belong here.
- `effects`: operation/account index, account key, before/after four-vectors and
  native revisions. `postings`: operation/line/account/child indexes, signed
  four-vector delta and checked copper value. `children`: root/child indexes,
  child operation ID and parent index.
- `item_references`: root/event index, UID, after revision, exact legacy
  ownership operation/event index, before and after revisions, and child
  index. `ownership_events`: the
  immutable UID history with its resulting owner/root/parent/state and action.
  `source_claims`: lineage, 48-byte source event and owning root ID. `receipts`:
  retained root ID, committed status and result code.

The source tables on SQL are `economic_accounting_*`, `economic_baseline_*`,
`critical_operation_inbox`, `item_current_owner`, `item_ownership_ledger`,
`player_data`, `account_banks`, auction/claim tables and any real treasury table.
The current SQL source capture can enumerate wallet/bank/coin-pile candidates,
but it does not yet produce this complete audit export. Flatfile evidence is
retained under `FLATFILE_ROOT/economic-evidence`; an exporter must verify all
bucket indexes, segments, authority files, journal recovery and native stores
before asserting completeness. **A hand-authored JSON fixture is synthetic
evidence only.** No live SQL or flatfile snapshot is currently certified by
this interface.

### SQL opening-origin extraction

`scripts/economic_sql_audit_origins.py` reads retained EAB1 baseline witnesses
through a dedicated repeatable-read, consistent-snapshot, read-only SQL
transaction. Give it an explicit host, database, user, lineage, epoch and new
output path; its password comes from the named environment variable (default
`DB_PASSWORD`). Use a database account with `SELECT` privilege only. It checks
InnoDB source tables, the control revision and terminal witness, each committed
baseline root/inbox receipt, stored SHA-256 and EAB1 framing, canonical row
order, nonzero source digests and unique account lifetimes/UIDs. A missing or
zero-revision baseline refuses. The output is bounded by the audit input limit
and contains only non-personal account keys, UID positions and revisions.

The `economic_sql_audit_origins_v1` artifact supplies **only**
`account_origins` and `item_origins` for a later full SQL exporter. It is not a
version-1 audit snapshot, does not attest current native balances or complete
ownership/economic history, and cannot be passed to the reconciler as a clean
release result. The disposable database runner
`tests/async/run_economic_sql_audit_origins_mysql.py` exercises the extractor
with a `SELECT`-only account; it requires an explicit disposable loopback flag.

`scripts/economic_sql_audit_snapshot.py` now reads these origins, the retained
nonbaseline accounting operations/effects/postings/children/item references,
source claims and inbox receipts, mapped SQL wallet and bank rows, and current
UID positions in **one** read-only consistent transaction. It writes a bounded
version-1 diagnostic snapshot with `backend: sql_partial`, `complete: false`
and a `capture_gaps` list. Pass that file to the reconciler to investigate
observed exceptions; its nonzero `evidence_loss` result is mandatory even when
all captured rows agree. The exporter omits baseline root effects because the
verified EAB1 witnesses provide their terminal opening origins. Its ownership
events are the legacy rows linked by captured accounting references, so it
cannot detect unlinked legacy events or certify the full UID scope. Its mapping
census can flag orphan candidates, duplicate active links and dangling links;
it does not resolve legacy admission or lineage ownership. Coin-pile payloads,
escrow/claim/treasury authority, postbaseline creation origins, retirement and
cross-epoch required-source policy still need independent enumeration and
proof. A partial snapshot cannot authorize activation or count as the Plan 5
full SQL audit acceptance.

The guarded disposable runner
`tests/async/run_economic_sql_audit_snapshot_mysql.py` inserts a baseline and a
committed wallet-to-bank root into a minimal InnoDB schema, checks the
`SELECT`-only CLI export and reconciler refusal, changes a native wallet from
another connection during the audit cut to verify repeatable-read isolation,
and injects a missing posting, stale native wallet balance, unmapped native
wallet, duplicate wallet mapping and dangling bank mapping. A wallet mapped to
another lineage is excluded from the unmapped count. A prior-epoch claim is
accepted, while a missing cross-epoch claim, reused source and orphan claim
produce exceptions. This tests the export mapping on both SQL engines;
fresh/upgrade schema and playable authority qualification remain separate
release gates.

## Reconciliation and bounded views

Run `python3 scripts/reconcile_economy_accounting.py snapshot.json`. Exit 0
means the supplied complete snapshot reconciles, 1 means coded exceptions, and
2 means malformed/oversized input. `--limit N` bounds detailed output to at
most 100 rows; totals remain complete. The tool checks root zero sum, posting
value and account effect agreement, account revision chains versus native
balances, exact UID/reference links, source claims, required source events,
child links, and receipt presence. It never clears an exception.

`--view holdings`, `--view supply`, `--view provenance --uid UID`, `--view
prices`, and `--view routes` produce bounded JSON rows. Supply and price views
report actual evidence only. Price data must come from the committed domain
result; a caller must never fill it with an appraisal. A rejected root carrying
a realized price is an audit exception and is excluded from the price view;
prices outside the signed 64-bit copper range are also refused. All views print
non-personal account keys, UIDs and operation IDs, never account names, player
names, email, or address. Run the tool only under the staff/operator account
that can read the protected snapshot. There is no public endpoint or player
command. An erased alias may be absent while its non-personal identity and
history remain; the tests check that an erased alias never enters a view.

The matrix's `refusal_source_evidence` records reviewed guard ordering for
individual unsupported routes. It is not backend gameplay proof. Such routes
remain `legacy` and the release gate stays closed until executable refusal
journeys and backend authority checks qualify them.

If a pet restore, NPC copyover, or login bank load produces a questionable
wallet view, pause that route and retain the original saved record, decoded
state, selected account/character identity, and read-only native/evidence
snapshot. Compare the four denominations and revisions before allowing a
publication or interpreting an apparent zero as a real sink. The current pet
restore discards saved cash, copyover can override decoded gold, and the SQL
login path can leave a zero bank projection after a failed read. These require
source-specific refusal and reconciliation evidence; the audit tool does not
repair the live state.

Correction and restitution submission are **disabled** in this tool. A future
operator writer must require authenticated authority, an expected native
revision/vector for every touched account or UID, the exact original committed
operation, a distinct durable source event, a policy-specific reason and an
atomic new root receipt. It must reject a changed state before mutation and
append new evidence rather than editing history. A manual SQL UPDATE, a missing
original operation, or an unlinked credit cannot resolve an audit exception.

## Qualification budgets

These are release gates to measure on each backend and the final integrated
commit. They are not claimed as measured results here.

| Measure | Limit and method |
| --- | --- |
| Single root shape | Existing EAI1 intent <= 8 KiB, EAP1 plan <= 4 MiB, <= 3,072 accounts, <= 6,144 posting lines, <= 3,000 item events, <= 64 children. Reject over limit before mutation. |
| Flatfile retained record | <= 4,722,762 bytes, <= 4,096 records and 256 MiB per bucket; never evict retained source/receipt evidence at capacity. |
| Audit snapshot and query | <= 32 MiB input, <= 100,000 rows per collection, <= 100 detailed output rows. A larger snapshot must refuse, not silently truncate. |
| Commit latency | At least 1,000 successful mixed real roots per backend after warm-up: p95 <= 250 ms, p99 <= 1 s, with route and domain counts recorded. Measure admission to durable receipt, and separately receipt to visible publication. |
| Storage growth | Record DB/table or flatfile evidence bytes before/after the same workload. Report bytes/root and total retained growth. No positive result if any row/segment/receipt disappears; review growth above 128 KiB/root average or the documented per-root bound. |
| Checkpoint/restart | Kill at each fault point, restart, and reconcile original IDs within 30 s for a 1,000-root fixture; zero duplicate source/UID effects and zero lost committed receipts. |
| Reconciliation | Complete 32 MiB snapshot within 30 s and <= 256 MiB peak process memory on the release host, with exception counts invariant under `--limit`. |

The release report must name exact tested commit, backend and image/version,
commands, sampled workload, route coverage and any intentionally unsupported
route. Synthetic fixtures and real player journeys are separate lines. Current
evidence and blockers are in [the release report](RELEASE_REPORT_2026-09-27.md).

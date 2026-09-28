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
result; a caller must never fill it with an appraisal. All views print
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
